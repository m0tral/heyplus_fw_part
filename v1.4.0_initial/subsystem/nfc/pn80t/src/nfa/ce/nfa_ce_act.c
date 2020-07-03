/******************************************************************************
 *
 *  Copyright (C) 2011-2014 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 */


/******************************************************************************
 *
 *  This file contains the action functions the NFA_CE state machine.
 *
 ******************************************************************************/
#include <string.h>
#include "nfa_ce_int.h"
#include "nfa_dm_int.h"
#include "nfa_sys_int.h"
#include "nfa_mem_co.h"
#include "ce_api.h"
#if (NFC_NFCEE_INCLUDED == TRUE)
#include "nfa_ee_int.h"
#endif

/*****************************************************************************
* Protocol-specific event handlers
*****************************************************************************/

/*****************************************************************************
* Discovery configuration and discovery event handlers
*****************************************************************************/

/*******************************************************************************
**
** Function         nfa_ce_discovery_cback
**
** Description      Processing event from discovery callback
**
** Returns          None
**
*******************************************************************************/
void nfa_ce_discovery_cback (tNFA_DM_RF_DISC_EVT event, tNFC_DISCOVER *p_data)
{
    tNFA_CE_MSG ce_msg;
    NFA_TRACE_DEBUG1 ("nfa_ce_discovery_cback(): event:0x%02X", event);

    switch (event)
    {
    case NFA_DM_RF_DISC_START_EVT:
        NFA_TRACE_DEBUG1 ("nfa_ce_handle_disc_start (status=0x%x)", p_data->start);
        break;

    case NFA_DM_RF_DISC_ACTIVATED_EVT:
#if (NXP_EXTNS == TRUE)
        /*
         * Handle the Reader over SWP.
         * Pass this info to JNI as START_READER_EVT.
         * Handle in nfa_ce_activate_ntf.
         */
#endif
        ce_msg.activate_ntf.hdr.event = NFA_CE_ACTIVATE_NTF_EVT;
        ce_msg.activate_ntf.p_activation_params = &p_data->activate;
        nfa_ce_hdl_event ((BT_HDR *) &ce_msg);
        break;

    case NFA_DM_RF_DISC_DEACTIVATED_EVT:
        /* DM broadcasts deactivaiton event in listen sleep state, so check before processing */
        if (nfa_ce_cb.flags & NFA_CE_FLAGS_LISTEN_ACTIVE_SLEEP)
        {
            ce_msg.hdr.event = NFA_CE_DEACTIVATE_NTF_EVT;
            ce_msg.hdr.layer_specific = p_data->deactivate.type;
            nfa_ce_hdl_event ((BT_HDR *) &ce_msg);
        }
        break;

    default:
        NFA_TRACE_ERROR0 ("Unexpected event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfc_ce_t3t_set_listen_params
**
** Description      Set t3t listening parameters
**
** Returns          Nothing
**
*******************************************************************************/
void nfc_ce_t3t_set_listen_params (void)
{
    UINT8 i;
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    UINT8 tlv[32], *p_params;
    UINT8 tlv_size;
    UINT16 t3t_flags2_mask = 0xFFFF;        /* Mask of which T3T_IDs are disabled */
    UINT8 t3t_idx = 0;

    /* Point to start of tlv buffer */
    p_params = tlv;

    /* Set system code and NFCID2 */
    for (i=0; i<NFA_CE_LISTEN_INFO_MAX; i++)
    {
        if ((p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_IN_USE) &&
            (p_cb->listen_info[i].protocol_mask & NFA_PROTOCOL_MASK_T3T))
        {
            /* Set tag's system code and NFCID2 */
            UINT8_TO_STREAM (p_params, NFC_PMID_LF_T3T_ID1+t3t_idx);                 /* type */
            UINT8_TO_STREAM (p_params, NCI_PARAM_LEN_LF_T3T_ID);                     /* length */
            UINT16_TO_BE_STREAM (p_params, p_cb->listen_info[i].t3t_system_code);    /* System Code */
            ARRAY_TO_BE_STREAM (p_params,  p_cb->listen_info[i].t3t_nfcid2, NCI_RF_F_UID_LEN);

            /* Set mask for this ID */
            t3t_flags2_mask &= ~((UINT16) (1<<t3t_idx));
            t3t_idx++;
        }
    }

    /* For NCI draft 22+, the polarity of NFC_PMID_LF_T3T_FLAGS2 is flipped */
    t3t_flags2_mask = ~t3t_flags2_mask;

    UINT8_TO_STREAM (p_params, NFC_PMID_LF_T3T_FLAGS2);      /* type */
    UINT8_TO_STREAM (p_params, NCI_PARAM_LEN_LF_T3T_FLAGS2); /* length */
    UINT16_TO_STREAM (p_params, t3t_flags2_mask);            /* Mask of IDs to disable listening */
    tlv_size = (UINT8) (p_params-tlv);
    nfa_dm_check_set_config (tlv_size, (UINT8 *)tlv, FALSE);
}

/*******************************************************************************
**
** Function         nfa_ce_t3t_generate_rand_nfcid
**
** Description      Generate a random NFCID2 for Type-3 tag
**
** Returns          Nothing
**
*******************************************************************************/
void nfa_ce_t3t_generate_rand_nfcid (UINT8 nfcid2[NCI_RF_F_UID_LEN])
{
    UINT32 rand_seed = GKI_get_tick_count ();

    /* For Type-3 tag, nfcid2 starts witn 02:fe */
    nfcid2[0] = 0x02;
    nfcid2[1] = 0xFE;

    /* The remaining 6 bytes are random */
    nfcid2[2] = (UINT8) (rand_seed & 0xFF);
    nfcid2[3] = (UINT8) (rand_seed>>8 & 0xFF);
    rand_seed>>=(rand_seed&3);
    nfcid2[4] = (UINT8) (rand_seed & 0xFF);
    nfcid2[5] = (UINT8) (rand_seed>>8 & 0xFF);
    rand_seed>>=(rand_seed&3);
    nfcid2[6] = (UINT8) (rand_seed & 0xFF);
    nfcid2[7] = (UINT8) (rand_seed>>8 & 0xFF);
}

/*******************************************************************************
**
** Function         nfa_ce_start_listening
**
** Description      Start listening
**
** Returns          NFA_STATUS_OK if successful
**
*******************************************************************************/
tNFA_STATUS nfa_ce_start_listening (void)
{
    tNFA_DM_DISC_TECH_PROTO_MASK listen_mask;
    tNFA_CE_CB    *p_cb = &nfa_ce_cb;
    tNFA_HANDLE   disc_handle;
    UINT8         listen_info_idx;

    /*************************************************************************/
    /* Construct protocol preference list to listen for */

    /* First, get protocol preference for active NDEF (if any) */
    if (  (p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].flags & NFA_CE_LISTEN_INFO_IN_USE)
        &&(p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].rf_disc_handle == NFA_HANDLE_INVALID))
    {
        listen_mask = 0;

        if (p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].protocol_mask & NFA_PROTOCOL_MASK_T3T)
        {
            /* set T3T config params */
            nfc_ce_t3t_set_listen_params ();

            listen_mask |= NFA_DM_DISC_MASK_LF_T3T;
        }

        if (p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].protocol_mask & NFA_PROTOCOL_MASK_ISO_DEP)
        {
            listen_mask |= nfa_ce_cb.isodep_disc_mask;
        }

        disc_handle = nfa_dm_add_rf_discover (listen_mask, NFA_DM_DISC_HOST_ID_DH, nfa_ce_discovery_cback);

        if (disc_handle == NFA_HANDLE_INVALID)
            return (NFA_STATUS_FAILED);
        else
            p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].rf_disc_handle = disc_handle;
    }

    /* Next, add protocols from non-NDEF, if any */
    for (listen_info_idx=0; listen_info_idx<NFA_CE_LISTEN_INFO_IDX_INVALID; listen_info_idx++)
    {
        /* add RF discovery to DM only if it is not added yet */
        if (  (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_IN_USE)
            &&(p_cb->listen_info[listen_info_idx].rf_disc_handle == NFA_HANDLE_INVALID))
        {
            if (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_FELICA)
            {
                /* set T3T config params */
                nfc_ce_t3t_set_listen_params ();

                disc_handle = nfa_dm_add_rf_discover (NFA_DM_DISC_MASK_LF_T3T,
                                                      NFA_DM_DISC_HOST_ID_DH,
                                                      nfa_ce_discovery_cback);

                if (disc_handle == NFA_HANDLE_INVALID)
                    return (NFA_STATUS_FAILED);
                else
                    p_cb->listen_info[listen_info_idx].rf_disc_handle = disc_handle;
            }
            else if (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_T4T_AID)
            {
                disc_handle = nfa_dm_add_rf_discover (nfa_ce_cb.isodep_disc_mask,
                                                       NFA_DM_DISC_HOST_ID_DH,
                                                       nfa_ce_discovery_cback);

                if (disc_handle == NFA_HANDLE_INVALID)
                    return (NFA_STATUS_FAILED);
                else
                    p_cb->listen_info[listen_info_idx].rf_disc_handle = disc_handle;
            }
#if (NFC_NFCEE_INCLUDED == TRUE)
            else if (p_cb->listen_info[listen_info_idx].flags &
#if(NXP_EXTNS == TRUE)
                    (NFA_CE_LISTEN_INFO_UICC | NFA_CE_LISTEN_INFO_ESE)
#else
                    NFA_CE_LISTEN_INFO_UICC
#endif
                    )
            {
                listen_mask = 0;
                if (nfa_ee_is_active (p_cb->listen_info[listen_info_idx].ee_handle))
                {
                    if (p_cb->listen_info[listen_info_idx].tech_mask & NFA_TECHNOLOGY_MASK_A)
                    {
                        listen_mask |= NFA_DM_DISC_MASK_LA_ISO_DEP;
                    }
                    if (p_cb->listen_info[listen_info_idx].tech_mask & NFA_TECHNOLOGY_MASK_B)
                    {
                        listen_mask |= NFA_DM_DISC_MASK_LB_ISO_DEP;
                    }
                    if (p_cb->listen_info[listen_info_idx].tech_mask & NFA_TECHNOLOGY_MASK_F)
                    {
                        listen_mask |= NFA_DM_DISC_MASK_LF_T3T;
                    }
                    if (p_cb->listen_info[listen_info_idx].tech_mask & NFA_TECHNOLOGY_MASK_B_PRIME)
                    {
                        listen_mask |= NFA_DM_DISC_MASK_L_B_PRIME;
                    }
#if(NXP_EXTNS == TRUE)
                    if (p_cb->listen_info[listen_info_idx].tech_mask & NFA_TECHNOLOGY_MASK_A_ACTIVE)
                    {
                        listen_mask |= NFA_DM_DISC_MASK_LAA_NFC_DEP;
                    }
                    if (p_cb->listen_info[listen_info_idx].tech_mask & NFA_TECHNOLOGY_MASK_F_ACTIVE)
                    {
                        listen_mask |= NFA_DM_DISC_MASK_LFA_NFC_DEP;
                    }
#endif
                }

                if (listen_mask)
                {
                    /* Start listening for requested technologies */
                    /* register discovery callback to NFA DM */
                    disc_handle = nfa_dm_add_rf_discover (listen_mask,
                                                          (tNFA_DM_DISC_HOST_ID) (p_cb->listen_info[listen_info_idx].ee_handle &0x00FF),
                                                          nfa_ce_discovery_cback);

                    if (disc_handle == NFA_HANDLE_INVALID)
                        return (NFA_STATUS_FAILED);
                    else
                    {
                        p_cb->listen_info[listen_info_idx].rf_disc_handle  = disc_handle;
                        p_cb->listen_info[listen_info_idx].tech_proto_mask = listen_mask;
                    }
                }
                else
                {
#if(NXP_EXTNS == TRUE)
                    NFA_TRACE_ERROR1 ("UICC/ESE[0x%x] is not activated",
                                       p_cb->listen_info[listen_info_idx].ee_handle);
#else
                    NFA_TRACE_ERROR1 ("UICC[0x%x] is not activated",
                                       p_cb->listen_info[listen_info_idx].ee_handle);
#endif
                }
            }
#endif
        }
    }

    return NFA_STATUS_OK;
}

/*******************************************************************************
**
** Function         nfa_ce_restart_listen_check
**
** Description      Called on deactivation. Check if any active listen_info entries to listen for
**
** Returns          TRUE if listening is restarted.
**                  FALSE if listening not restarted
**
*******************************************************************************/
BOOLEAN nfa_ce_restart_listen_check (void)
{
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    UINT8 listen_info_idx;

    /* Check if any active entries in listen_info table */
    for (listen_info_idx=0; listen_info_idx<NFA_CE_LISTEN_INFO_MAX; listen_info_idx++)
    {
        if (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_IN_USE)
            break;
    }

    /* Restart listening if there are any active listen_info entries */
    if (listen_info_idx != NFA_CE_LISTEN_INFO_IDX_INVALID)
    {
        /* restart listening */
        nfa_ce_start_listening ();
    }
    else
    {
        /* No active listen_info entries */
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         nfa_ce_remove_listen_info_entry
**
** Description      Remove entry from listen_info table. (when API deregister is called or listen_start failed)
**
**
** Returns          Nothing
**
*******************************************************************************/
void nfa_ce_remove_listen_info_entry (UINT8 listen_info_idx, BOOLEAN notify_app)
{
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    tNFA_CONN_EVT_DATA conn_evt;

    NFA_TRACE_DEBUG1 ("NFA_CE: removing listen_info entry %i", listen_info_idx);

    /* Notify app that listening has stopped  if requested (for API deregister) */
    /* For LISTEN_START failures, app has already notified of NFA_LISTEN_START_EVT failure */
    if (notify_app)
    {
        if (listen_info_idx == NFA_CE_LISTEN_INFO_IDX_NDEF)
        {
            conn_evt.status = NFA_STATUS_OK;
            (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_LOCAL_TAG_CONFIGURED_EVT, &conn_evt);
        }
#if (NFC_NFCEE_INCLUDED == TRUE)
        else if (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_UICC)
        {
            conn_evt.status = NFA_STATUS_OK;
            (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
        }
#if(NXP_EXTNS == TRUE)
        else if (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_ESE)
        {
            conn_evt.status = NFA_STATUS_OK;
            (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_ESE_LISTEN_CONFIGURED_EVT, &conn_evt);
        }
#endif

#endif
        else
        {
            conn_evt.ce_deregistered.handle = NFA_HANDLE_GROUP_CE | listen_info_idx;
            (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_DEREGISTERED_EVT, &conn_evt);
        }
    }

    if (p_cb->listen_info[listen_info_idx].rf_disc_handle != NFA_HANDLE_INVALID )
    {
        nfa_dm_delete_rf_discover (p_cb->listen_info[listen_info_idx].rf_disc_handle);
        p_cb->listen_info[listen_info_idx].rf_disc_handle = NFA_HANDLE_INVALID;
    }

    /* Remove entry from listen_info table */
    p_cb->listen_info[listen_info_idx].flags = 0;
}

/*******************************************************************************
**
** Function         nfa_ce_free_scratch_buf
**
** Description      free scratch buffer (if one is allocated)
**
** Returns          nothing
**
*******************************************************************************/
void nfa_ce_free_scratch_buf (void)
{
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    if (p_cb->p_scratch_buf)
    {
    	GKI_os_free (p_cb->p_scratch_buf);
        p_cb->p_scratch_buf = NULL;
    }
}

/*******************************************************************************
**
** Function         nfa_ce_realloc_scratch_buffer
**
** Description      Set scratch buffer if necessary (for writable NDEF messages)
**
** Returns          NFA_STATUS_OK if successful
**
*******************************************************************************/
tNFA_STATUS nfa_ce_realloc_scratch_buffer (void)
{
    tNFA_STATUS result = NFA_STATUS_OK;

    /* If current NDEF message is read-only, then we do not need a scratch buffer */
    if (nfa_ce_cb.listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].flags & NFC_CE_LISTEN_INFO_READONLY_NDEF)
    {
        /* Free existing scratch buffer, if one was allocated */
        nfa_ce_free_scratch_buf ();
    }
    else
    {
        /* If no scratch buffer allocated yet, or if current scratch buffer size is different from current ndef size, */
        /* then allocate a new scratch buffer. */
        if ((nfa_ce_cb.p_scratch_buf == NULL) ||
            (nfa_ce_cb.scratch_buf_size != nfa_ce_cb.ndef_max_size))
        {
            /* Free existing scratch buffer, if one was allocated */
            nfa_ce_free_scratch_buf ();

            if ((nfa_ce_cb.p_scratch_buf = (UINT8 *) GKI_os_malloc(nfa_ce_cb.ndef_max_size)) != NULL)
            {
                nfa_ce_cb.scratch_buf_size = nfa_ce_cb.ndef_max_size;
            }
            else
            {
                NFA_TRACE_ERROR1 ("Unable to allocate scratch buffer for writable NDEF message (%i bytes)", nfa_ce_cb.ndef_max_size);
                result=NFA_STATUS_FAILED;
            }
        }
    }

    return (result);
}


/*******************************************************************************
**
** Function         nfa_ce_activate_ntf
**
** Description      Action when activation has occured (NFA_CE_ACTIVATE_NTF_EVT)
**
**                  - Find the listen_info entry assocated with this activation
**                      - get the app callback that registered for this listen
**                      - call CE_SetActivatedTagType with activation parameters
**
** Returns          TRUE (message buffer to be freed by caller)
**
*******************************************************************************/
BOOLEAN nfa_ce_activate_ntf (tNFA_CE_MSG *p_ce_msg)
{
    tNFC_ACTIVATE_DEVT *p_activation_params = p_ce_msg->activate_ntf.p_activation_params;
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    tNFA_CONN_EVT_DATA conn_evt;
    UINT8 listen_info_idx = NFA_CE_LISTEN_INFO_IDX_INVALID;
    UINT8 i;
    /*Changed part of warning fixes in wearable project*/
    //UINT8 *p_nfcid2 = NULL;
    //UINT16 t3t_system_code = 0xFFFF;
    //BOOLEAN t4t_activate_pending = FALSE;

    NFA_TRACE_DEBUG1 ("nfa_ce_activate_ntf () protocol=%d", p_ce_msg->activate_ntf.p_activation_params->protocol);

    /* Tag is in listen active state */
    p_cb->flags |= NFA_CE_FLAGS_LISTEN_ACTIVE_SLEEP;

    /* Store activation parameters */
    memcpy (&p_cb->activation_params, p_activation_params, sizeof (tNFC_ACTIVATE_DEVT));

#if(NXP_EXTNS == TRUE)
    if (p_cb->activation_params.intf_param.type == NCI_INTERFACE_UICC_DIRECT || p_cb->activation_params.intf_param.type == NCI_INTERFACE_ESE_DIRECT )
    {
        memcpy (&(conn_evt.activated.activate_ntf), &p_cb->activation_params, sizeof (tNFC_ACTIVATE_DEVT));
        for (i=0; i<NFA_CE_LISTEN_INFO_IDX_INVALID; i++)
        {
            if (p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_UICC)
            {
                listen_info_idx = i;
                NFA_TRACE_DEBUG1 ("listen_info found for this activation. listen_info_idx=%d", listen_info_idx);
                /* Get CONN_CBACK for this activation */
                p_cb->p_active_conn_cback = p_cb->listen_info[listen_info_idx].p_conn_cback;
                break;
            }
        }

        (*p_cb->p_active_conn_cback) (NFA_ACTIVATED_EVT, &conn_evt);
    }
    /* Find the listen_info entry corresponding to this activation */
    else if (p_cb->activation_params.intf_param.type == NFC_INTERFACE_EE_DIRECT_RF)
#else
    /* Find the listen_info entry corresponding to this activation */
    if (p_cb->activation_params.intf_param.type == NFC_INTERFACE_EE_DIRECT_RF)
#endif
    {
        /* search any entry listening UICC */
        for (i=0; i<NFA_CE_LISTEN_INFO_IDX_INVALID; i++)
        {
            if (  (p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_IN_USE)
                &&(p_cb->listen_info[i].flags &
#if(NXP_EXTNS == TRUE)
                        (NFA_CE_LISTEN_INFO_UICC | NFA_CE_LISTEN_INFO_ESE)
#else
                        NFA_CE_LISTEN_INFO_UICC
#endif
                ))
            {
                listen_info_idx = i;
                break;
            }
        }
    }

    /* Check if valid listen_info entry was found */
    if (  (listen_info_idx == NFA_CE_LISTEN_INFO_IDX_INVALID)
        ||((listen_info_idx == NFA_CE_LISTEN_INFO_IDX_NDEF) && !(p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].flags & NFA_CE_LISTEN_INFO_IN_USE)))
    {
        NFA_TRACE_DEBUG1 ("No listen_info found for this activation. listen_info_idx=%d", listen_info_idx);
        return (TRUE);
    }

    p_cb->listen_info[listen_info_idx].flags &= ~NFA_CE_LISTEN_INFO_T4T_ACTIVATE_PND;

    /* Get CONN_CBACK for this activation */
    p_cb->p_active_conn_cback = p_cb->listen_info[listen_info_idx].p_conn_cback;
    p_cb->idx_cur_active = listen_info_idx;

    if (  (p_cb->idx_cur_active == NFA_CE_LISTEN_INFO_IDX_NDEF)
        ||(p_cb->listen_info[p_cb->idx_cur_active].flags &
#if(NXP_EXTNS == TRUE)
                        (NFA_CE_LISTEN_INFO_UICC | NFA_CE_LISTEN_INFO_ESE)
#else
                        NFA_CE_LISTEN_INFO_UICC
#endif
        ))
    {
        memcpy (&(conn_evt.activated.activate_ntf), &p_cb->activation_params, sizeof (tNFC_ACTIVATE_DEVT));

        (*p_cb->p_active_conn_cback) (NFA_ACTIVATED_EVT, &conn_evt);
    }
    else
    {
        conn_evt.ce_activated.handle =   NFA_HANDLE_GROUP_CE | ((tNFA_HANDLE)p_cb->idx_cur_active);
        memcpy (&(conn_evt.ce_activated.activate_ntf), &p_cb->activation_params, sizeof (tNFC_ACTIVATE_DEVT));
        conn_evt.ce_activated.status = NFA_STATUS_OK;

        (*p_cb->p_active_conn_cback) (NFA_CE_ACTIVATED_EVT, &conn_evt);
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         nfa_ce_deactivate_ntf
**
** Description      Action when deactivate occurs. (NFA_CE_DEACTIVATE_NTF_EVT)
**
**                  - If deactivate due to API deregister, then remove its entry from
**                      listen_info table
**
**                  - If NDEF was modified while activated, then restore
**                      original NDEF contents
**
**                  - Restart listening (if any active entries in listen table)
**
** Returns          TRUE (message buffer to be freed by caller)
**
*******************************************************************************/
BOOLEAN nfa_ce_deactivate_ntf (tNFA_CE_MSG *p_ce_msg)
{
    tNFC_DEACT_TYPE deact_type = (tNFC_DEACT_TYPE) p_ce_msg->hdr.layer_specific;
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    tNFA_CONN_EVT_DATA conn_evt;
    UINT8 i;

    NFA_TRACE_DEBUG1 ("nfa_ce_deactivate_ntf () deact_type=%d", deact_type);

    /* Check if deactivating to SLEEP mode */
    if (  (deact_type == NFC_DEACTIVATE_TYPE_SLEEP)
        ||(deact_type == NFC_DEACTIVATE_TYPE_SLEEP_AF)  )
    {
        if ( nfa_ce_cb.idx_wild_card == NFA_CE_LISTEN_INFO_IDX_INVALID)
        {
            /* notify deactivated as sleep and wait for reactivation or deactivation to idle */
            conn_evt.deactivated.type =  deact_type;

            /* if T4T AID application has not been selected then p_active_conn_cback could be NULL */
            if (p_cb->p_active_conn_cback)
                (*p_cb->p_active_conn_cback) (NFA_DEACTIVATED_EVT, &conn_evt);
        }
        else
        {
            conn_evt.ce_deactivated.handle = NFA_HANDLE_GROUP_CE | ((tNFA_HANDLE)nfa_ce_cb.idx_wild_card);
            conn_evt.ce_deactivated.type   = deact_type;
            if (p_cb->p_active_conn_cback)
                (*p_cb->p_active_conn_cback) (NFA_CE_DEACTIVATED_EVT, &conn_evt);
        }

        return TRUE;
    }
    else
    {
        deact_type = NFC_DEACTIVATE_TYPE_IDLE;
    }

    /* Tag is in idle state */
    p_cb->flags &= ~NFA_CE_FLAGS_LISTEN_ACTIVE_SLEEP;

    /* First, notify app of deactivation */
    for (i=0; i<NFA_CE_LISTEN_INFO_IDX_INVALID; i++)
    {
        if (p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_IN_USE)
        {
            if (  (p_cb->listen_info[i].flags &
#if(NXP_EXTNS == TRUE)
                        (NFA_CE_LISTEN_INFO_UICC | NFA_CE_LISTEN_INFO_ESE)
#else
                        NFA_CE_LISTEN_INFO_UICC
#endif
            )
                &&(i == p_cb->idx_cur_active)  )
            {
                conn_evt.deactivated.type =  deact_type;
                (*p_cb->p_active_conn_cback) (NFA_DEACTIVATED_EVT, &conn_evt);
            }
            else if (  (p_cb->activation_params.protocol == NFA_PROTOCOL_ISO_DEP)
                     &&(p_cb->listen_info[i].protocol_mask & NFA_PROTOCOL_MASK_ISO_DEP))
            {
                /* Don't send NFA_DEACTIVATED_EVT if NFA_ACTIVATED_EVT wasn't sent */
                if (!(p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_T4T_ACTIVATE_PND))
                {
                    if (i == NFA_CE_LISTEN_INFO_IDX_NDEF)
                    {
                        conn_evt.deactivated.type =  deact_type;
                        (*p_cb->p_active_conn_cback) (NFA_DEACTIVATED_EVT, &conn_evt);
                    }
                    else
                    {
                        conn_evt.ce_deactivated.handle = NFA_HANDLE_GROUP_CE | ((tNFA_HANDLE)i);
                        conn_evt.ce_deactivated.type   = deact_type;
                        (*p_cb->p_active_conn_cback) (NFA_CE_DEACTIVATED_EVT, &conn_evt);
                    }
                }
            }
            else if (  (p_cb->activation_params.protocol == NFA_PROTOCOL_T3T)
                     &&(p_cb->listen_info[i].protocol_mask & NFA_PROTOCOL_MASK_T3T))
            {
                if (i == NFA_CE_LISTEN_INFO_IDX_NDEF)
                {
                    conn_evt.deactivated.type = deact_type;
                    (*p_cb->p_active_conn_cback) (NFA_DEACTIVATED_EVT, &conn_evt);
                }
                else
                {
                    conn_evt.ce_deactivated.handle = NFA_HANDLE_GROUP_CE | ((tNFA_HANDLE)i);
                    conn_evt.ce_deactivated.type   = deact_type;
                    (*p_cb->p_active_conn_cback) (NFA_CE_DEACTIVATED_EVT, &conn_evt);
                }
            }
        }
    }

    /* Check if app initiated the deactivation (due to API deregister). If so, remove entry from listen_info table. */
    if (p_cb->flags & NFA_CE_FLAGS_APP_INIT_DEACTIVATION)
    {
        p_cb->flags &= ~NFA_CE_FLAGS_APP_INIT_DEACTIVATION;
        nfa_ce_remove_listen_info_entry (p_cb->idx_cur_active, TRUE);
    }

    p_cb->p_active_conn_cback = NULL;
    p_cb->idx_cur_active      = NFA_CE_LISTEN_INFO_IDX_INVALID;

    /* Restart listening (if any listen_info entries are still active) */
    nfa_ce_restart_listen_check ();

    return TRUE;
}

/*******************************************************************************
**
** Function         nfa_ce_disable_local_tag
**
** Description      Disable local NDEF tag
**                      - clean up control block
**                      - remove NDEF discovery configuration
**
** Returns          Nothing
**
*******************************************************************************/
void nfa_ce_disable_local_tag (void)
{
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    tNFA_CONN_EVT_DATA evt_data;

    NFA_TRACE_DEBUG0 ("Disabling local NDEF tag");

    /* If local NDEF tag is in use, then disable it */
    if (p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].flags & NFA_CE_LISTEN_INFO_IN_USE)
    {
        /* NDEF Tag is in not idle state */
        if (  (p_cb->flags & NFA_CE_FLAGS_LISTEN_ACTIVE_SLEEP)
            &&(p_cb->idx_cur_active == NFA_CE_LISTEN_INFO_IDX_NDEF)  )
        {
            /* wait for deactivation */
            p_cb->flags |= NFA_CE_FLAGS_APP_INIT_DEACTIVATION;
            nfa_dm_rf_deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        else
        {
            /* Notify DM to stop listening for ndef  */
            if (p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].rf_disc_handle != NFA_HANDLE_INVALID)
            {
                nfa_dm_delete_rf_discover (p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].rf_disc_handle);
                p_cb->listen_info[NFA_CE_LISTEN_INFO_IDX_NDEF].rf_disc_handle = NFA_HANDLE_INVALID;
            }
            nfa_ce_remove_listen_info_entry (NFA_CE_LISTEN_INFO_IDX_NDEF, TRUE);
        }
    }
    else
    {
        /* Notify application */
        evt_data.status = NFA_STATUS_OK;
        nfa_dm_conn_cback_event_notify (NFA_CE_LOCAL_TAG_CONFIGURED_EVT, &evt_data);
    }
}


/*******************************************************************************
**
** Function         nfa_ce_api_reg_listen
**
** Description      Register listen params for Felica system code, T4T AID,
**                  or UICC
**
** Returns          TRUE (message buffer to be freed by caller)
**
*******************************************************************************/
BOOLEAN nfa_ce_api_reg_listen (tNFA_CE_MSG *p_ce_msg)
{
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    tNFA_CONN_EVT_DATA conn_evt;
    UINT8 i;
    UINT8 listen_info_idx = NFA_CE_LISTEN_INFO_IDX_INVALID;

#if(NXP_EXTNS == TRUE)
    NFA_TRACE_DEBUG1 ("Registering UICC/ESE/Felica/Type-4 tag listener. Type=%i", p_ce_msg->reg_listen.listen_type);
#else
    NFA_TRACE_DEBUG1 ("Registering UICC/Felica/Type-4 tag listener. Type=%i", p_ce_msg->reg_listen.listen_type);
#endif

    /* Look for available entry in listen_info table                                        */
    /* - If registering UICC listen, make sure there isn't another entry for the ee_handle  */
    /* - Skip over entry 0 (reserved for local NDEF tag)                                    */
    for (i=1; i<NFA_CE_LISTEN_INFO_MAX; i++)
    {
        if (  (p_ce_msg->reg_listen.listen_type == NFA_CE_REG_TYPE_UICC)
            &&(p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_IN_USE)
            &&(p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_UICC)
            &&(p_cb->listen_info[i].ee_handle == p_ce_msg->reg_listen.ee_handle)  )
        {

            NFA_TRACE_ERROR1 ("UICC (0x%x) listening already specified", p_ce_msg->reg_listen.ee_handle);
            conn_evt.status = NFA_STATUS_FAILED;
            nfa_dm_conn_cback_event_notify (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
            return TRUE;
        }
#if(NXP_EXTNS == TRUE)
        else if (  (p_ce_msg->reg_listen.listen_type == NFA_CE_REG_TYPE_ESE)
            &&(p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_IN_USE)
            &&(p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_ESE)
            &&(p_cb->listen_info[i].ee_handle == p_ce_msg->reg_listen.ee_handle)  )
        {

            NFA_TRACE_ERROR1 ("ESE (0x%x) listening already specified", p_ce_msg->reg_listen.ee_handle);
            conn_evt.status = NFA_STATUS_FAILED;
            nfa_dm_conn_cback_event_notify (NFA_CE_ESE_LISTEN_CONFIGURED_EVT, &conn_evt);
            return TRUE;
        }
#endif
        /* If this is a free entry, and we haven't found one yet, remember it */
        else if (  (!(p_cb->listen_info[i].flags & NFA_CE_LISTEN_INFO_IN_USE))
                 &&(listen_info_idx == NFA_CE_LISTEN_INFO_IDX_INVALID)  )
        {
            listen_info_idx = i;
        }
    }

    /* Add new entry to listen_info table */
    if (listen_info_idx == NFA_CE_LISTEN_INFO_IDX_INVALID)
    {
        NFA_TRACE_ERROR1 ("Maximum listen callbacks exceeded (%i)", NFA_CE_LISTEN_INFO_MAX);

        if (p_ce_msg->reg_listen.listen_type == NFA_CE_REG_TYPE_UICC)
        {
            conn_evt.status = NFA_STATUS_FAILED;
            nfa_dm_conn_cback_event_notify (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
        }
#if(NXP_EXTNS == TRUE)
        else if (p_ce_msg->reg_listen.listen_type == NFA_CE_REG_TYPE_ESE)
        {
            conn_evt.status = NFA_STATUS_FAILED;
            nfa_dm_conn_cback_event_notify (NFA_CE_ESE_LISTEN_CONFIGURED_EVT, &conn_evt);
        }
#endif
        else
        {
            /* Notify application */
            conn_evt.ce_registered.handle = NFA_HANDLE_INVALID;
            conn_evt.ce_registered.status = NFA_STATUS_FAILED;
            (*p_ce_msg->reg_listen.p_conn_cback) (NFA_CE_REGISTERED_EVT, &conn_evt);
        }
        return TRUE;
    }
    else
    {
        NFA_TRACE_DEBUG1 ("NFA_CE: adding listen_info entry %i", listen_info_idx);

        /* Store common parameters */
        /* Mark entry as 'in-use', and NFA_CE_LISTEN_INFO_START_NTF_PND */
        /* (LISTEN_START_EVT will be notified when discovery successfully starts */
        p_cb->listen_info[listen_info_idx].flags = NFA_CE_LISTEN_INFO_IN_USE | NFA_CE_LISTEN_INFO_START_NTF_PND;
        p_cb->listen_info[listen_info_idx].rf_disc_handle = NFA_HANDLE_INVALID;
        p_cb->listen_info[listen_info_idx].protocol_mask = 0;

        /* Store type-specific parameters */
        switch (p_ce_msg->reg_listen.listen_type)
        {
#if (NFC_NFCEE_INCLUDED == TRUE)
        case NFA_CE_REG_TYPE_UICC:

#if(NXP_EXTNS == TRUE)

            for (i=1; i<NFA_CE_LISTEN_INFO_MAX; i++)
            {
                UINT8 tech = p_cb->listen_info[listen_info_idx].tech_mask &
                        p_ce_msg->reg_listen.tech_mask;
                if( (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_ESE) &&
                        (tech)
                       )
                {
                    NFA_TRACE_ERROR1 ("NFA_CE: Technology %0x listening already specified for ESE", tech);
                    conn_evt.status = NFA_STATUS_FAILED;
                    nfa_dm_conn_cback_event_notify (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
                    return TRUE;
                }
            }
#endif
            p_cb->listen_info[listen_info_idx].flags |= NFA_CE_LISTEN_INFO_UICC;
            p_cb->listen_info[listen_info_idx].p_conn_cback = &nfa_dm_conn_cback_event_notify;

            /* Store EE handle and Tech */
            p_cb->listen_info[listen_info_idx].ee_handle = p_ce_msg->reg_listen.ee_handle;
            p_cb->listen_info[listen_info_idx].tech_mask = p_ce_msg->reg_listen.tech_mask;
            break;

#if(NXP_EXTNS == TRUE)
        case NFA_CE_REG_TYPE_ESE:

            for (i=1; i<NFA_CE_LISTEN_INFO_MAX; i++)
            {
                UINT8 tech = p_cb->listen_info[listen_info_idx].tech_mask &
                        p_ce_msg->reg_listen.tech_mask;
                if( (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_UICC) &&
                        (tech)
                       )
                {
                    NFA_TRACE_ERROR1 ("NFA_CE: Technology %0x listening already specified for UICC", tech);
                    conn_evt.status = NFA_STATUS_FAILED;
                    nfa_dm_conn_cback_event_notify (NFA_CE_ESE_LISTEN_CONFIGURED_EVT, &conn_evt);
                    return TRUE;
                }
            }

            p_cb->listen_info[listen_info_idx].flags |= NFA_CE_LISTEN_INFO_ESE;
            p_cb->listen_info[listen_info_idx].p_conn_cback = &nfa_dm_conn_cback_event_notify;

            /* Store EE handle and Tech */
            p_cb->listen_info[listen_info_idx].ee_handle = p_ce_msg->reg_listen.ee_handle;
            p_cb->listen_info[listen_info_idx].tech_mask = p_ce_msg->reg_listen.tech_mask;
            break;
#endif

#endif
        }
    }

    /* Start listening */
    if ((conn_evt.status = nfa_ce_start_listening ()) != NFA_STATUS_OK)
    {
        NFA_TRACE_ERROR0 ("nfa_ce_api_reg_listen: unable to register new listen params with DM");
        p_cb->listen_info[listen_info_idx].flags = 0;
    }

    /* Nofitify app of status */
    if (p_ce_msg->reg_listen.listen_type == NFA_CE_REG_TYPE_UICC)
    {
        (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
    }
#if(NXP_EXTNS == TRUE)
    else if (p_ce_msg->reg_listen.listen_type == NFA_CE_REG_TYPE_ESE)
    {
        (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_ESE_LISTEN_CONFIGURED_EVT, &conn_evt);
    }
#endif
    else
    {
        conn_evt.ce_registered.handle = NFA_HANDLE_GROUP_CE | listen_info_idx;
        NFA_TRACE_DEBUG1 ("nfa_ce_api_reg_listen: registered handle 0x%04X", conn_evt.ce_registered.handle);
        (*p_cb->listen_info[listen_info_idx].p_conn_cback) (NFA_CE_REGISTERED_EVT, &conn_evt);
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         nfa_ce_api_dereg_listen
**
** Description      Deregister listen params
**
** Returns          TRUE (message buffer to be freed by caller)
**
*******************************************************************************/
BOOLEAN nfa_ce_api_dereg_listen (tNFA_CE_MSG *p_ce_msg)
{
    tNFA_CE_CB *p_cb = &nfa_ce_cb;
    UINT8 listen_info_idx;
    tNFA_CONN_EVT_DATA conn_evt;

#if (NFC_NFCEE_INCLUDED == TRUE)
    /* Check if deregistering UICC/ESE , or virtual secure element listen */
    if (p_ce_msg->dereg_listen.listen_info
#if(NXP_EXTNS == TRUE)
                        & (NFA_CE_LISTEN_INFO_UICC | NFA_CE_LISTEN_INFO_ESE)
#else
                        == NFA_CE_LISTEN_INFO_UICC
#endif
    )
    {
        /* Deregistering UICC listen. Look for listen_info for this UICC ee handle */
        for (listen_info_idx = 0; listen_info_idx < NFA_CE_LISTEN_INFO_MAX; listen_info_idx++)
        {
            if (  (p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_IN_USE)
                &&(p_cb->listen_info[listen_info_idx].flags &
#if(NXP_EXTNS == TRUE)
                        (NFA_CE_LISTEN_INFO_UICC | NFA_CE_LISTEN_INFO_ESE)
#else
                        NFA_CE_LISTEN_INFO_UICC
#endif
                )
                &&(p_cb->listen_info[listen_info_idx].ee_handle == p_ce_msg->dereg_listen.handle)  )
            {
                /* UICC is in not idle state */
                if (  (p_cb->flags & NFA_CE_FLAGS_LISTEN_ACTIVE_SLEEP)
                    &&(p_cb->idx_cur_active == listen_info_idx)  )
                {
                    /* wait for deactivation */
                    p_cb->flags |= NFA_CE_FLAGS_APP_INIT_DEACTIVATION;
                    nfa_dm_rf_deactivate (NFA_DEACTIVATE_TYPE_IDLE);
                }
                else
                {
                    /* Stop listening */
                    if (p_cb->listen_info[listen_info_idx].rf_disc_handle != NFA_HANDLE_INVALID)
                    {
                        nfa_dm_delete_rf_discover (p_cb->listen_info[listen_info_idx].rf_disc_handle);
                        p_cb->listen_info[listen_info_idx].rf_disc_handle = NFA_HANDLE_INVALID;
                    }

                    /* Remove entry and notify application */
                    nfa_ce_remove_listen_info_entry (listen_info_idx, TRUE);
                }
                break;
            }
        }

        if (listen_info_idx == NFA_CE_LISTEN_INFO_MAX)
        {
            conn_evt.status = NFA_STATUS_INVALID_PARAM;
#if(NXP_EXTNS == TRUE)
            NFA_TRACE_ERROR0 ("nfa_ce_api_dereg_listen (): cannot find listen_info for UICC/ESE");
            if(p_ce_msg->dereg_listen.listen_info & NFA_CE_LISTEN_INFO_UICC )
            {
                nfa_dm_conn_cback_event_notify (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
            }
            else if(p_ce_msg->dereg_listen.listen_info & NFA_CE_LISTEN_INFO_ESE)
            {
                nfa_dm_conn_cback_event_notify (NFA_CE_ESE_LISTEN_CONFIGURED_EVT, &conn_evt);
            }
#else
            NFA_TRACE_ERROR0 ("nfa_ce_api_dereg_listen (): cannot find listen_info for UICC");
            nfa_dm_conn_cback_event_notify (NFA_CE_UICC_LISTEN_CONFIGURED_EVT, &conn_evt);
#endif
        }
    }
    else
#endif
    {
        /* Deregistering virtual secure element listen */
        listen_info_idx = p_ce_msg->dereg_listen.handle & NFA_HANDLE_MASK;
        if (nfa_ce_cb.idx_wild_card == listen_info_idx)
        {
            nfa_ce_cb.idx_wild_card     = NFA_CE_LISTEN_INFO_IDX_INVALID;
        }

        if (  (listen_info_idx < NFA_CE_LISTEN_INFO_MAX)
            &&(p_cb->listen_info[listen_info_idx].flags & NFA_CE_LISTEN_INFO_IN_USE))
        {
            /* virtual secure element is in not idle state */
            if (  (p_cb->flags & NFA_CE_FLAGS_LISTEN_ACTIVE_SLEEP)
                &&(p_cb->idx_cur_active == listen_info_idx)  )
            {
                /* wait for deactivation */
                p_cb->flags |= NFA_CE_FLAGS_APP_INIT_DEACTIVATION;
                nfa_dm_rf_deactivate (NFA_DEACTIVATE_TYPE_IDLE);
            }
            else
            {
                /* Stop listening */
                if (p_cb->listen_info[listen_info_idx].rf_disc_handle != NFA_HANDLE_INVALID)
                {
                    nfa_dm_delete_rf_discover (p_cb->listen_info[listen_info_idx].rf_disc_handle);
                    p_cb->listen_info[listen_info_idx].rf_disc_handle = NFA_HANDLE_INVALID;
                }

                /* Remove entry and notify application */
                nfa_ce_remove_listen_info_entry (listen_info_idx, TRUE);
            }
        }
        else
        {
            NFA_TRACE_ERROR0 ("nfa_ce_api_dereg_listen (): cannot find listen_info for Felica/T4tAID");
            conn_evt.status = NFA_STATUS_INVALID_PARAM;
            nfa_dm_conn_cback_event_notify (NFA_CE_DEREGISTERED_EVT, &conn_evt);
        }
    }

    return TRUE;
}

/*******************************************************************************
**
** Function         nfa_ce_api_cfg_isodep_tech
**
** Description      Configure the technologies (NFC-A and/or NFC-B) to listen for
**                  ISO-DEP
**
** Returns          TRUE (message buffer to be freed by caller)
**
*******************************************************************************/
BOOLEAN nfa_ce_api_cfg_isodep_tech (tNFA_CE_MSG *p_ce_msg)
{
    nfa_ce_cb.isodep_disc_mask  = 0;
    if (p_ce_msg->hdr.layer_specific & NFA_TECHNOLOGY_MASK_A)
        nfa_ce_cb.isodep_disc_mask  = NFA_DM_DISC_MASK_LA_ISO_DEP;

    if (p_ce_msg->hdr.layer_specific & NFA_TECHNOLOGY_MASK_B)
        nfa_ce_cb.isodep_disc_mask |= NFA_DM_DISC_MASK_LB_ISO_DEP;
    return TRUE;
}
