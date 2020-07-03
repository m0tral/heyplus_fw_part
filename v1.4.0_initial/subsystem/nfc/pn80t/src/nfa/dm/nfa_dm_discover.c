/******************************************************************************
 *
 *  Copyright (C) 2010-2014 Broadcom Corporation
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
/******************************************************************************
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
 ******************************************************************************/
/******************************************************************************
 *
 *  This file contains the action functions for device manager discovery
 *  function.
 *
 ******************************************************************************/
#include <string.h>
#include "nfa_sys.h"
#include "nfa_api.h"
#include "nfa_dm_int.h"
#include "nfa_p2p_int.h"
#include "nfa_sys_int.h"
#include "nci_hmsgs.h"
#if (NFC_NFCEE_INCLUDED == TRUE)
#include "nfa_ee_api.h"
#include "nfa_ee_int.h"
#endif
#include "nfa_rw_int.h"

#include "nfc_int.h"
#if(NXP_EXTNS == TRUE)
#include<config.h>
#endif
/*
**  static functions
*/
#if(NXP_EXTNS == TRUE)
static BOOLEAN reconnect_in_progress;
#endif

static UINT8 nfa_dm_get_rf_discover_config (tNFA_DM_DISC_TECH_PROTO_MASK dm_disc_mask,
                                            tNFC_DISCOVER_PARAMS disc_params[],
                                            UINT8 max_params);
static void nfa_dm_set_rf_listen_mode_raw_config (tNFA_DM_DISC_TECH_PROTO_MASK *p_disc_mask);
static tNFA_DM_DISC_TECH_PROTO_MASK nfa_dm_disc_get_disc_mask (tNFC_RF_TECH_N_MODE tech_n_mode,
                                                               tNFC_PROTOCOL       protocol);
static void nfa_dm_notify_discovery (tNFA_DM_RF_DISC_DATA *p_data);
static tNFA_STATUS nfa_dm_disc_notify_activation (tNFC_DISCOVER *p_data);
static void nfa_dm_disc_notify_deactivation (tNFA_DM_RF_DISC_SM_EVENT sm_event, tNFC_DISCOVER *p_data);
static void nfa_dm_disc_data_cback (UINT8 conn_id, tNFC_CONN_EVT event, tNFC_CONN *p_data);

#if (BT_TRACE_VERBOSE == TRUE)
static char *nfa_dm_disc_state_2_str (UINT8 state);
static char *nfa_dm_disc_event_2_str (UINT8 event);
#endif

#if(NXP_EXTNS == TRUE)
typedef struct nfa_dm_p2p_prio_logic
{
    BOOLEAN          isodep_detected; /* flag to check if ISO-DEP is detected */
    BOOLEAN          timer_expired;   /* flag to check whether timer is expired */
    TIMER_LIST_ENT   timer_list; /*timer structure pointer */
    UINT8            first_tech_mode;
}nfa_dm_p2p_prio_logic_t;

static nfa_dm_p2p_prio_logic_t p2p_prio_logic_data;

#define P2P_RESUME_POLL_TIMEOUT 16 /*mili second timeout value*/
static UINT8 gUiccListenMask = NFA_TECHNOLOGY_MASK_A;// |NFA_TECHNOLOGY_MASK_B ;
#endif

/*******************************************************************************
**
** Function         nfa_dm_get_rf_discover_config
**
** Description      Build RF discovery configurations from tNFA_DM_DISC_TECH_PROTO_MASK
**
** Returns          number of RF discovery configurations
**
*******************************************************************************/
static UINT8 nfa_dm_get_rf_discover_config (tNFA_DM_DISC_TECH_PROTO_MASK dm_disc_mask,
                                            tNFC_DISCOVER_PARAMS         disc_params[],
                                            UINT8 max_params)
{
    UINT8 num_params = 0;

    if (nfa_dm_cb.flags & NFA_DM_FLAGS_LISTEN_DISABLED)
    {
        NFA_TRACE_DEBUG1 ("nfa_dm_get_rf_discover_config () listen disabled, rm listen from 0x%x", dm_disc_mask);
        dm_disc_mask &= NFA_DM_DISC_MASK_POLL;
    }
    if (nfa_dm_is_p2p_paused ())
    {
        dm_disc_mask &= ~NFA_DM_DISC_MASK_NFC_DEP;
    }

    /* Check polling A */
    if (dm_disc_mask & ( NFA_DM_DISC_MASK_PA_T1T
                        |NFA_DM_DISC_MASK_PA_T2T
                        |NFA_DM_DISC_MASK_PA_ISO_DEP
                        |NFA_DM_DISC_MASK_PA_NFC_DEP
                        |NFA_DM_DISC_MASK_P_LEGACY) )
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_A;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->pa;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check polling B */
    if (dm_disc_mask & NFA_DM_DISC_MASK_PB_ISO_DEP)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_B;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->pb;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check polling F */
    if (dm_disc_mask & ( NFA_DM_DISC_MASK_PF_T3T
                        |NFA_DM_DISC_MASK_PF_NFC_DEP) )
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_F;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->pf;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check polling A Active mode  */
    if (dm_disc_mask & NFA_DM_DISC_MASK_PAA_NFC_DEP)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_A_ACTIVE;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->paa;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check polling F Active mode  */
    if (dm_disc_mask & NFA_DM_DISC_MASK_PFA_NFC_DEP)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_F_ACTIVE;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->pfa;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening A */
    if (dm_disc_mask & ( NFA_DM_DISC_MASK_LA_T1T
                        |NFA_DM_DISC_MASK_LA_T2T
                        |NFA_DM_DISC_MASK_LA_ISO_DEP
                        |NFA_DM_DISC_MASK_LA_NFC_DEP) )
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_A;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening B */
    if (dm_disc_mask & NFA_DM_DISC_MASK_LB_ISO_DEP)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_B;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening F */
    if (dm_disc_mask & ( NFA_DM_DISC_MASK_LF_T3T
                        |NFA_DM_DISC_MASK_LF_NFC_DEP) )
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_F;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening A Active mode */
    if (dm_disc_mask & NFA_DM_DISC_MASK_LAA_NFC_DEP)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening F Active mode */
    if (dm_disc_mask & NFA_DM_DISC_MASK_LFA_NFC_DEP)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check polling ISO 15693 */
    if (dm_disc_mask & NFA_DM_DISC_MASK_P_ISO15693)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_ISO15693;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->pi93;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check polling B' */
    if (dm_disc_mask & NFA_DM_DISC_MASK_P_B_PRIME)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_POLL_B_PRIME;
        disc_params[num_params].frequency = p_nfa_dm_rf_disc_freq_cfg->pbp;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening ISO 15693 */
    if (dm_disc_mask & NFA_DM_DISC_MASK_L_ISO15693)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_ISO15693;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    /* Check listening B' */
    if (dm_disc_mask & NFA_DM_DISC_MASK_L_B_PRIME)
    {
        disc_params[num_params].type      = NFC_DISCOVERY_TYPE_LISTEN_B_PRIME;
        disc_params[num_params].frequency = 1;
        num_params++;

        if (num_params >= max_params)
            return num_params;
    }

    return num_params;
}

/*******************************************************************************
**
** Function         nfa_dm_set_total_duration
**
** Description      Update total duration to NFCC
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_set_total_duration (void)
{
    UINT8 params[10], *p;

    NFA_TRACE_DEBUG0 ("nfa_dm_set_total_duration ()");

    p = params;

    /* for total duration */
    UINT8_TO_STREAM (p, NFC_PMID_TOTAL_DURATION);
    UINT8_TO_STREAM (p, NCI_PARAM_LEN_TOTAL_DURATION);
    UINT16_TO_STREAM (p, nfa_dm_cb.disc_cb.disc_duration);

    if (p > params)
    {
        nfa_dm_check_set_config ((UINT8) (p - params), params, FALSE);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_set_rf_listen_mode_raw_config
**
** Description      Set raw listen parameters
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_set_rf_listen_mode_raw_config (tNFA_DM_DISC_TECH_PROTO_MASK *p_disc_mask)
{
    tNFA_DM_DISC_TECH_PROTO_MASK disc_mask = 0;
    tNFA_LISTEN_CFG  *p_cfg = &nfa_dm_cb.disc_cb.excl_listen_config;
    UINT8 params[250], *p, xx;

    NFA_TRACE_DEBUG0 ("nfa_dm_set_rf_listen_mode_raw_config ()");

    /*
    ** Discovery Configuration Parameters for Listen A
    */
    if (  (nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_A] == NFA_DM_DISC_HOST_ID_DH)
        &&(p_cfg->la_enable)  )
    {
        p = params;

        UINT8_TO_STREAM (p, NFC_PMID_LA_BIT_FRAME_SDD);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LA_BIT_FRAME_SDD);
        UINT8_TO_STREAM (p, p_cfg->la_bit_frame_sdd);

        UINT8_TO_STREAM (p, NFC_PMID_LA_PLATFORM_CONFIG);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LA_PLATFORM_CONFIG);
        UINT8_TO_STREAM (p, p_cfg->la_platform_config);

        UINT8_TO_STREAM (p, NFC_PMID_LA_SEL_INFO);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LA_SEL_INFO);
        UINT8_TO_STREAM (p, p_cfg->la_sel_info);

        if (p_cfg->la_platform_config == NCI_PARAM_PLATFORM_T1T)
        {
            disc_mask |= NFA_DM_DISC_MASK_LA_T1T;
        }
        else
        {
            /* If T4T or NFCDEP */
            if (p_cfg->la_sel_info & NCI_PARAM_SEL_INFO_ISODEP)
            {
                disc_mask |= NFA_DM_DISC_MASK_LA_ISO_DEP;
            }

            if (p_cfg->la_sel_info & NCI_PARAM_SEL_INFO_NFCDEP)
            {
                disc_mask |= NFA_DM_DISC_MASK_LA_NFC_DEP;
            }

            /* If neither, T4T nor NFCDEP, then its T2T */
            if (disc_mask == 0)
            {
                disc_mask |= NFA_DM_DISC_MASK_LA_T2T;
            }
        }

        UINT8_TO_STREAM (p, NFC_PMID_LA_NFCID1);
        UINT8_TO_STREAM (p, p_cfg->la_nfcid1_len);
        ARRAY_TO_STREAM (p, p_cfg->la_nfcid1, p_cfg->la_nfcid1_len);

        nfa_dm_check_set_config ((UINT8) (p - params), params, FALSE);
    }

    /*
    ** Discovery Configuration Parameters for Listen B
    */
    if (  (nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_B] == NFA_DM_DISC_HOST_ID_DH)
        &&(p_cfg->lb_enable)  )
    {
        p = params;

        UINT8_TO_STREAM (p, NFC_PMID_LB_SENSB_INFO);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LB_SENSB_INFO);
        UINT8_TO_STREAM (p, p_cfg->lb_sensb_info);

        UINT8_TO_STREAM (p, NFC_PMID_LB_NFCID0);
        UINT8_TO_STREAM (p, p_cfg->lb_nfcid0_len);
        ARRAY_TO_STREAM (p, p_cfg->lb_nfcid0, p_cfg->lb_nfcid0_len);

        UINT8_TO_STREAM (p, NFC_PMID_LB_APPDATA);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LB_APPDATA);
        ARRAY_TO_STREAM (p, p_cfg->lb_app_data, NCI_PARAM_LEN_LB_APPDATA);

        UINT8_TO_STREAM (p, NFC_PMID_LB_SFGI);
        UINT8_TO_STREAM (p, 1);
        UINT8_TO_STREAM (p, p_cfg->lb_adc_fo);

        UINT8_TO_STREAM (p, NFC_PMID_LB_ADC_FO);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LB_ADC_FO);
        UINT8_TO_STREAM (p, p_cfg->lb_adc_fo);

        nfa_dm_check_set_config ((UINT8) (p - params), params, FALSE);

        if (p_cfg->lb_sensb_info & NCI_LISTEN_PROTOCOL_ISO_DEP)
        {
            disc_mask |= NFA_DM_DISC_MASK_LB_ISO_DEP;
        }
    }

    /*
    ** Discovery Configuration Parameters for Listen F
    */
    if (  (nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_F] == NFA_DM_DISC_HOST_ID_DH)
        &&(p_cfg->lf_enable)  )
    {
        p = params;

        UINT8_TO_STREAM (p, NFC_PMID_LF_CON_BITR_F);
        UINT8_TO_STREAM (p, 1);
        UINT8_TO_STREAM (p, p_cfg->lf_con_bitr_f);

        UINT8_TO_STREAM (p, NFC_PMID_LF_PROTOCOL);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LF_PROTOCOL);
        UINT8_TO_STREAM (p, p_cfg->lf_protocol_type);

        UINT8_TO_STREAM (p, NFC_PMID_LF_T3T_FLAGS2);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_LF_T3T_FLAGS2);
        UINT16_TO_STREAM(p, p_cfg->lf_t3t_flags);

        /* if the bit at position X is set to 0, SC/NFCID2 with index X shall be ignored */
        for (xx = 0; xx < NFA_LF_MAX_SC_NFCID2; xx++)
        {
            if ((p_cfg->lf_t3t_flags & (0x0001 << xx)) != 0x0000)
            {
                UINT8_TO_STREAM (p, NFC_PMID_LF_T3T_ID1 + xx);
                UINT8_TO_STREAM (p, NCI_SYSTEMCODE_LEN + NCI_NFCID2_LEN);
                ARRAY_TO_STREAM (p, p_cfg->lf_t3t_identifier[xx], NCI_SYSTEMCODE_LEN + NCI_NFCID2_LEN);
            }
        }

#if (NXP_EXTNS != TRUE)
        UINT8_TO_STREAM (p,  NFC_PMID_LF_T3T_PMM);
        UINT8_TO_STREAM (p,  NCI_PARAM_LEN_LF_T3T_PMM);
        ARRAY_TO_STREAM (p,  p_cfg->lf_t3t_pmm, NCI_PARAM_LEN_LF_T3T_PMM);
#endif

        nfa_dm_check_set_config ((UINT8) (p - params), params, FALSE);

        if (p_cfg->lf_t3t_flags != NCI_LF_T3T_FLAGS2_ALL_DISABLED)
        {
            disc_mask |= NFA_DM_DISC_MASK_LF_T3T;
        }
        if (p_cfg->lf_protocol_type & NCI_LISTEN_PROTOCOL_NFC_DEP)
        {
            disc_mask |= NFA_DM_DISC_MASK_LF_NFC_DEP;
        }
    }

    /*
    ** Discovery Configuration Parameters for Listen ISO-DEP
    */
    if ((disc_mask & (NFA_DM_DISC_MASK_LA_ISO_DEP|NFA_DM_DISC_MASK_LB_ISO_DEP))
      &&(p_cfg->li_enable))
    {
        p = params;

        UINT8_TO_STREAM (p, NFC_PMID_FWI);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_FWI);
        UINT8_TO_STREAM (p, p_cfg->li_fwi);

        if (disc_mask & NFA_DM_DISC_MASK_LA_ISO_DEP)
        {
            UINT8_TO_STREAM (p, NFC_PMID_LA_HIST_BY);
            UINT8_TO_STREAM (p, p_cfg->la_hist_bytes_len);
            ARRAY_TO_STREAM (p, p_cfg->la_hist_bytes, p_cfg->la_hist_bytes_len);
        }

        if (disc_mask & NFA_DM_DISC_MASK_LB_ISO_DEP)
        {
            UINT8_TO_STREAM (p, NFC_PMID_LB_H_INFO);
            UINT8_TO_STREAM (p, p_cfg->lb_h_info_resp_len);
            ARRAY_TO_STREAM (p, p_cfg->lb_h_info_resp, p_cfg->lb_h_info_resp_len);
        }

        nfa_dm_check_set_config ((UINT8) (p - params), params, FALSE);
    }

    /*
    ** Discovery Configuration Parameters for Listen NFC-DEP
    */
    if (  (disc_mask & (NFA_DM_DISC_MASK_LA_NFC_DEP|NFA_DM_DISC_MASK_LF_NFC_DEP))
        &&(p_cfg->ln_enable))
    {
        p = params;

        UINT8_TO_STREAM (p, NFC_PMID_WT);
        UINT8_TO_STREAM (p, NCI_PARAM_LEN_WT);
        UINT8_TO_STREAM (p, p_cfg->ln_wt);

        UINT8_TO_STREAM (p, NFC_PMID_ATR_RES_GEN_BYTES);
        UINT8_TO_STREAM (p, p_cfg->ln_atr_res_gen_bytes_len);
        ARRAY_TO_STREAM (p, p_cfg->ln_atr_res_gen_bytes, p_cfg->ln_atr_res_gen_bytes_len);

        UINT8_TO_STREAM (p, NFC_PMID_ATR_RSP_CONFIG);
        UINT8_TO_STREAM (p, 1);
        UINT8_TO_STREAM (p, p_cfg->ln_atr_res_config);

        nfa_dm_check_set_config ((UINT8) (p - params), params, FALSE);
    }

    *p_disc_mask = disc_mask;

    NFA_TRACE_DEBUG1 ("nfa_dm_set_rf_listen_mode_raw_config () disc_mask = 0x%x", disc_mask);
}

/*******************************************************************************
**
** Function         nfa_dm_disc_get_disc_mask
**
** Description      Convert RF technology, mode and protocol to bit mask
**
** Returns          tNFA_DM_DISC_TECH_PROTO_MASK
**
*******************************************************************************/
static tNFA_DM_DISC_TECH_PROTO_MASK nfa_dm_disc_get_disc_mask (tNFC_RF_TECH_N_MODE tech_n_mode,
                                                               tNFC_PROTOCOL       protocol)
{
    /* Set initial disc_mask to legacy poll or listen */
    tNFA_DM_DISC_TECH_PROTO_MASK disc_mask = ((tech_n_mode & 0x80) ? NFA_DM_DISC_MASK_L_LEGACY : NFA_DM_DISC_MASK_P_LEGACY);

    if (NFC_DISCOVERY_TYPE_POLL_A == tech_n_mode)
    {
        switch (protocol)
        {
        case NFC_PROTOCOL_T1T:
            disc_mask = NFA_DM_DISC_MASK_PA_T1T;
            break;
        case NFC_PROTOCOL_T2T:
            disc_mask = NFA_DM_DISC_MASK_PA_T2T;
            break;
        case NFC_PROTOCOL_ISO_DEP:
            disc_mask = NFA_DM_DISC_MASK_PA_ISO_DEP;
            break;
        case NFC_PROTOCOL_NFC_DEP:
            disc_mask = NFA_DM_DISC_MASK_PA_NFC_DEP;
            break;
        }
    }
    else if (NFC_DISCOVERY_TYPE_POLL_B == tech_n_mode)
    {
        if (protocol == NFC_PROTOCOL_ISO_DEP)
            disc_mask = NFA_DM_DISC_MASK_PB_ISO_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_POLL_F == tech_n_mode)
    {
        if (protocol == NFC_PROTOCOL_T3T)
            disc_mask = NFA_DM_DISC_MASK_PF_T3T;
        else if (protocol == NFC_PROTOCOL_NFC_DEP)
            disc_mask = NFA_DM_DISC_MASK_PF_NFC_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_POLL_ISO15693 == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_P_ISO15693;
    }
    else if (NFC_DISCOVERY_TYPE_POLL_B_PRIME == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_P_B_PRIME;
    }
    else if (NFC_DISCOVERY_TYPE_POLL_A_ACTIVE == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_PAA_NFC_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_POLL_F_ACTIVE == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_PFA_NFC_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_A == tech_n_mode)
    {
        switch (protocol)
        {
        case NFC_PROTOCOL_T1T:
            disc_mask = NFA_DM_DISC_MASK_LA_T1T;
            break;
        case NFC_PROTOCOL_T2T:
            disc_mask = NFA_DM_DISC_MASK_LA_T2T;
            break;
        case NFC_PROTOCOL_ISO_DEP:
            disc_mask = NFA_DM_DISC_MASK_LA_ISO_DEP;
            break;
        case NFC_PROTOCOL_NFC_DEP:
            disc_mask = NFA_DM_DISC_MASK_LA_NFC_DEP;
            break;
        }
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_B == tech_n_mode)
    {
        if (protocol == NFC_PROTOCOL_ISO_DEP)
            disc_mask = NFA_DM_DISC_MASK_LB_ISO_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_F == tech_n_mode)
    {
        if (protocol == NFC_PROTOCOL_T3T)
            disc_mask = NFA_DM_DISC_MASK_LF_T3T;
        else if (protocol == NFC_PROTOCOL_NFC_DEP)
            disc_mask = NFA_DM_DISC_MASK_LF_NFC_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_ISO15693 == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_L_ISO15693;
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_B_PRIME == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_L_B_PRIME;
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_LAA_NFC_DEP;
    }
    else if (NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE == tech_n_mode)
    {
        disc_mask = NFA_DM_DISC_MASK_LFA_NFC_DEP;
    }

    NFA_TRACE_DEBUG3 ("nfa_dm_disc_get_disc_mask (): tech_n_mode:0x%X, protocol:0x%X, disc_mask:0x%X",
                       tech_n_mode, protocol, disc_mask);
    return (disc_mask);
}

/*******************************************************************************
**
** Function         nfa_dm_disc_discovery_cback
**
** Description      Discovery callback event from NFC
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_discovery_cback (tNFC_DISCOVER_EVT event, tNFC_DISCOVER *p_data)
{
    tNFA_DM_RF_DISC_SM_EVENT dm_disc_event = NFA_DM_DISC_SM_MAX_EVENT;

    NFA_TRACE_DEBUG1 ("nfa_dm_disc_discovery_cback (): event:0x%X", event);

    switch (event)
    {
    case NFC_START_DEVT:
        dm_disc_event = NFA_DM_RF_DISCOVER_RSP;
        break;
    case NFC_RESULT_DEVT:
        dm_disc_event = NFA_DM_RF_DISCOVER_NTF;
        break;
    case NFC_SELECT_DEVT:
        dm_disc_event = NFA_DM_RF_DISCOVER_SELECT_RSP;
        break;
    case NFC_ACTIVATE_DEVT:
        dm_disc_event = NFA_DM_RF_INTF_ACTIVATED_NTF;
        break;
    case NFC_DEACTIVATE_DEVT:
        if (p_data->deactivate.is_ntf)
        {
            dm_disc_event = NFA_DM_RF_DEACTIVATE_NTF;
            if ((p_data->deactivate.type == NFC_DEACTIVATE_TYPE_IDLE) || (p_data->deactivate.type == NFC_DEACTIVATE_TYPE_DISCOVERY))
            {
                NFC_SetReassemblyFlag (TRUE);
                nfa_dm_cb.flags &= ~NFA_DM_FLAGS_RAW_FRAME;
            }
        }
        else
            dm_disc_event = NFA_DM_RF_DEACTIVATE_RSP;
        break;
    default:
        NFA_TRACE_ERROR0 ("Unexpected event");
        return;
    }

    nfa_dm_disc_sm_execute (dm_disc_event, (tNFA_DM_RF_DISC_DATA *) p_data);
}

/*******************************************************************************
**
** Function         nfa_dm_disc_notify_started
**
** Description      Report NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT or
**                  NFA_RF_DISCOVERY_STARTED_EVT, if needed
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_notify_started (tNFA_STATUS status)
{
    tNFA_CONN_EVT_DATA      evt_data;

    if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_NOTIFY)
    {
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_NOTIFY;

        evt_data.status = status;

        if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
            nfa_dm_conn_cback_event_notify (NFA_EXCLUSIVE_RF_CONTROL_STARTED_EVT, &evt_data);
        else
            nfa_dm_conn_cback_event_notify (NFA_RF_DISCOVERY_STARTED_EVT, &evt_data);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_conn_event_notify
**
** Description      Notify application of CONN_CBACK event, using appropriate
**                  callback
**
** Returns          nothing
**
*******************************************************************************/
void nfa_dm_disc_conn_event_notify (UINT8 event, tNFA_STATUS status)
{
    tNFA_CONN_EVT_DATA      evt_data;

    if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_NOTIFY)
    {
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_NOTIFY;
        evt_data.status               = status;

        if (nfa_dm_cb.flags & NFA_DM_FLAGS_EXCL_RF_ACTIVE)
        {
            /* Use exclusive RF mode callback */
            if (nfa_dm_cb.p_excl_conn_cback)
                (*nfa_dm_cb.p_excl_conn_cback) (event, &evt_data);
        }
        else
        {
            (*nfa_dm_cb.p_conn_cback) (event, &evt_data);
        }
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_force_to_idle
**
** Description      Force NFCC to idle state while waiting for deactivation NTF
**
** Returns          tNFC_STATUS
**
*******************************************************************************/
static tNFC_STATUS nfa_dm_disc_force_to_idle (void)
{
    tNFC_STATUS status = NFC_STATUS_SEMANTIC_ERROR;

    NFA_TRACE_DEBUG1 ("nfa_dm_disc_force_to_idle() disc_flags = 0x%x", nfa_dm_cb.disc_cb.disc_flags);

    if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF) /* do not execute more than one */
    {
        nfa_dm_cb.disc_cb.disc_flags &= ~(NFA_DM_DISC_FLAGS_W4_NTF);
        nfa_dm_cb.disc_cb.disc_flags |= (NFA_DM_DISC_FLAGS_W4_RSP);
        nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
        status = NFC_Deactivate (NFC_DEACTIVATE_TYPE_IDLE);
    }

    return (status);
}

/*******************************************************************************
**
** Function         nfa_dm_disc_deact_ntf_timeout_cback
**
** Description      Timeout while waiting for deactivation NTF
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_deact_ntf_timeout_cback (TIMER_LIST_ENT *p_tle)
{
    NFA_TRACE_ERROR0 ("nfa_dm_disc_deact_ntf_timeout_cback()");

    nfa_dm_disc_force_to_idle();
}

/*******************************************************************************
**
** Function         nfa_dm_send_deactivate_cmd
**
** Description      Send deactivate command to NFCC, if needed.
**
** Returns          NFC_STATUS_OK             - deactivate cmd is sent
**                  NCI_STATUS_FAILED         - no buffers
**                  NFC_STATUS_SEMANTIC_ERROR - this function does not attempt
**                                              to send deactivate cmd
**
*******************************************************************************/
static tNFC_STATUS nfa_dm_send_deactivate_cmd (tNFC_DEACT_TYPE deactivate_type)
{
    tNFC_STATUS status = NFC_STATUS_SEMANTIC_ERROR;
    tNFA_DM_DISC_FLAGS w4_flags = nfa_dm_cb.disc_cb.disc_flags & (NFA_DM_DISC_FLAGS_W4_RSP|NFA_DM_DISC_FLAGS_W4_NTF);
#if (NXP_EXTNS == TRUE)
    unsigned long num = 0;
#endif

    if (!w4_flags)
    {
        /* if deactivate CMD was not sent to NFCC */
        nfa_dm_cb.disc_cb.disc_flags |= (NFA_DM_DISC_FLAGS_W4_RSP|NFA_DM_DISC_FLAGS_W4_NTF);

        status = NFC_Deactivate (deactivate_type);

        if (!nfa_dm_cb.disc_cb.tle.in_use)
        {
            nfa_dm_cb.disc_cb.tle.p_cback = (TIMER_CBACK *)nfa_dm_disc_deact_ntf_timeout_cback;
#if (NXP_EXTNS == TRUE)
            num = NFA_DM_DISC_TIMEOUT_W4_DEACT_NTF;
            NFA_TRACE_DEBUG1("num_disc_maps=%d",nfc_cb.num_disc_maps);
            if (nfc_cb.num_disc_maps == 1)
            {
                NFA_TRACE_DEBUG1("Reading CFG_NFA_DM_DISC_NTF_TIMEOUT val   "
                        "nfc_cb.num_disc_maps = %d", nfc_cb.num_disc_maps);
                if ( GetNumValue ( CFG_NFA_DM_DISC_NTF_TIMEOUT, &num, sizeof ( num ) ) )
                {
                    num *= 1000;
                }
                else
                {
                    num = NFA_DM_DISC_TIMEOUT_W4_DEACT_NTF;
                    NFA_TRACE_DEBUG1("Overriding NFA_DM_DISC_NTF_TIMEOUT to use %d", num);
                }
            }
            NFA_TRACE_DEBUG1("Starting timer value %d", num);
            nfa_sys_start_timer (&nfa_dm_cb.disc_cb.tle, 0, num);
#else
            nfa_sys_start_timer (&nfa_dm_cb.disc_cb.tle, 0, NFA_DM_DISC_TIMEOUT_W4_DEACT_NTF);
#endif
        }
    }
    else
    {
        if (deactivate_type == NFC_DEACTIVATE_TYPE_SLEEP)
        {
            status = NFC_STATUS_SEMANTIC_ERROR;
        }
        else if (nfa_dm_cb.disc_cb.tle.in_use)
        {
            status = NFC_STATUS_OK;
        }
        else
        {
            status = nfa_dm_disc_force_to_idle ();
        }
    }

    return status;
}
void nfa_dm_set_uicc_listen_mask(UINT8 mask)
{
    gUiccListenMask = mask;
}
/*******************************************************************************
**
** Function         nfa_dm_start_rf_discover
**
** Description      Start RF discovery
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_start_rf_discover (void)
{
    tNFC_DISCOVER_PARAMS    disc_params[NFA_DM_MAX_DISC_PARAMS];
    tNFA_DM_DISC_TECH_PROTO_MASK dm_disc_mask = 0, poll_mask, listen_mask;
    UINT8                   num_params, xx;

    NFA_TRACE_DEBUG0 ("nfa_dm_start_rf_discover ()");
    /* Make sure that RF discovery was enabled, or some app has exclusive control */
    if (  (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_ENABLED))
        &&(nfa_dm_cb.disc_cb.excl_disc_entry.in_use == FALSE)  )
    {
        return;
    }

    /* get listen mode routing table for technology */
    nfa_ee_get_tech_route (NFA_EE_PWR_STATE_ON, nfa_dm_cb.disc_cb.listen_RT);

    if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
    {
        nfa_dm_set_rf_listen_mode_raw_config (&dm_disc_mask);
        dm_disc_mask |= (nfa_dm_cb.disc_cb.excl_disc_entry.requested_disc_mask & NFA_DM_DISC_MASK_POLL);
        nfa_dm_cb.disc_cb.excl_disc_entry.selected_disc_mask = dm_disc_mask;
    }
    else
    {
        /* Collect RF discovery request from sub-modules */
        for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
        {
            if (nfa_dm_cb.disc_cb.entry[xx].in_use)
            {
                poll_mask = (nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask & NFA_DM_DISC_MASK_POLL);
                NFA_TRACE_DEBUG2 ("requested_disc_mask = 0x%x, xx=%d", nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask, xx);
                /* clear poll mode technolgies and protocols which are already used by others */
                poll_mask &= ~(dm_disc_mask & NFA_DM_DISC_MASK_POLL);

                /* reset listen mask before updating it for every discovery callback entry */
                listen_mask = 0;

                /*
                ** add listen mode technolgies and protocols if host ID is matched to listen mode routing table
                */

                /* NFC-A */
                if (nfa_dm_cb.disc_cb.entry[xx].host_id == nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_A])
                {
                    listen_mask |= nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask
                                   & ( NFA_DM_DISC_MASK_LA_T1T
                                      |NFA_DM_DISC_MASK_LA_T2T
                                      |NFA_DM_DISC_MASK_LA_ISO_DEP
                                      |NFA_DM_DISC_MASK_LA_NFC_DEP
                                      |NFA_DM_DISC_MASK_LAA_NFC_DEP );
                }
                else
                {
                    /* host can listen ISO-DEP based on AID routing */
                    listen_mask |= (nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask  & NFA_DM_DISC_MASK_LA_ISO_DEP);

                    /* host can listen NFC-DEP based on protocol routing */
                    listen_mask |= (nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask  & NFA_DM_DISC_MASK_LA_NFC_DEP);
                    listen_mask |= (nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask  & NFA_DM_DISC_MASK_LAA_NFC_DEP);
                }
                NFA_TRACE_DEBUG1 ("listen_mask value after NFC-A = 0x%x", listen_mask);

                /* NFC-B */
                /* multiple hosts can listen ISO-DEP based on AID routing */
                listen_mask |= nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask
                               & NFA_DM_DISC_MASK_LB_ISO_DEP;
                NFA_TRACE_DEBUG1 ("listen_mask value after NFC-B = 0x%x", listen_mask);

                /* NFC-F */
                listen_mask |= nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask
                               & ( NFA_DM_DISC_MASK_LF_T3T
                                  |NFA_DM_DISC_MASK_LF_NFC_DEP
                                  |NFA_DM_DISC_MASK_LFA_NFC_DEP );
                NFA_TRACE_DEBUG1 ("listen_mask value after NFC-F = 0x%x", listen_mask);

                /* NFC-B Prime */
                if (nfa_dm_cb.disc_cb.entry[xx].host_id == nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_BP])
                {
                    listen_mask |= nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask
                                   & NFA_DM_DISC_MASK_L_B_PRIME;
                }
                NFA_TRACE_DEBUG1 ("listen_mask value after NFC-B Prime = 0x%x", listen_mask);
                /*
                ** clear listen mode technolgies and protocols which are already used by others
                */

                /* Check if other modules are listening T1T or T2T */
                NFA_TRACE_DEBUG1 ("dm_disc_mask = 0x%x", dm_disc_mask);
                if (dm_disc_mask & (NFA_DM_DISC_MASK_LA_T1T|NFA_DM_DISC_MASK_LA_T2T))
                {
                    listen_mask &= ~( NFA_DM_DISC_MASK_LA_T1T
                                     |NFA_DM_DISC_MASK_LA_T2T
                                     |NFA_DM_DISC_MASK_LA_ISO_DEP
                                     |NFA_DM_DISC_MASK_LA_NFC_DEP );
                }
                NFA_TRACE_DEBUG1 ("listen_mask value after T1T and T2T = 0x%x", listen_mask);

                /* T1T/T2T has priority on NFC-A */
                if (  (dm_disc_mask & (NFA_DM_DISC_MASK_LA_ISO_DEP|NFA_DM_DISC_MASK_LA_NFC_DEP))
                    &&(listen_mask & (NFA_DM_DISC_MASK_LA_T1T|NFA_DM_DISC_MASK_LA_T2T)))
                {
                    dm_disc_mask &= ~( NFA_DM_DISC_MASK_LA_ISO_DEP
                                      |NFA_DM_DISC_MASK_LA_NFC_DEP );
                }

                /* Don't remove ISO-DEP because multiple hosts can listen ISO-DEP based on AID routing */

                /* Check if other modules are listening NFC-DEP */
                if (dm_disc_mask & (NFA_DM_DISC_MASK_LA_NFC_DEP | NFA_DM_DISC_MASK_LAA_NFC_DEP))
                {
                    listen_mask &= ~( NFA_DM_DISC_MASK_LA_NFC_DEP
                                     |NFA_DM_DISC_MASK_LAA_NFC_DEP );
                }

                NFA_TRACE_DEBUG1 ("listen_mask value after NFC-DEP = 0x%x", listen_mask);

                nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask = poll_mask | listen_mask;

                NFA_TRACE_DEBUG2 ("nfa_dm_cb.disc_cb.entry[%d].selected_disc_mask = 0x%x",
                                   xx, nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask);

                dm_disc_mask |= nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask;
            }
        }
    }
#if(NXP_EXTNS == TRUE)
    NFA_TRACE_DEBUG1 ("dm_disc_mask before masking = 0x%x", dm_disc_mask);
    if((gUiccListenMask & 0x1) == 0x0)
    {
        NFA_TRACE_DEBUG0 ("nfa_dm_start_rf_discover (): try removing A passive listen for ISO-DEP");
        dm_disc_mask &= ~(NFA_DM_DISC_MASK_LA_ISO_DEP);
    }
    if((gUiccListenMask & 0x2) == 0x0)
    {
        NFA_TRACE_DEBUG0 ("nfa_dm_start_rf_discover (): try removing B passive listen for ISO-DEP");
        dm_disc_mask &= ~(NFA_DM_DISC_MASK_LB_ISO_DEP);
    }
    if((gUiccListenMask & 0x4) == 0x0)
    {
        NFA_TRACE_DEBUG0 ("nfa_dm_start_rf_discover (): try removing F passive listen for T3T");
        dm_disc_mask &= ~(NFA_DM_DISC_MASK_LF_T3T);
    }

    NFA_TRACE_DEBUG1 ("dm_disc_mask after masking = 0x%x", dm_disc_mask);
#endif

    /* Get Discovery Technology parameters */
    num_params = nfa_dm_get_rf_discover_config (dm_disc_mask, disc_params, NFA_DM_MAX_DISC_PARAMS);

    if (num_params)
    {
        /*
        ** NFCC will abort programming personality slots if not available.
        ** NFCC programs the personality slots in the following order of RF technologies:
        **      NFC-A, NFC-B, NFC-BP, NFC-I93
        */


        /* do not set rf listen mode config*/


        /* Set polling duty cycle */
        nfa_dm_set_total_duration ();
        nfa_dm_cb.disc_cb.dm_disc_mask = dm_disc_mask;

        NFC_DiscoveryStart (num_params, disc_params, nfa_dm_disc_discovery_cback);
        /* set flag about waiting for response in IDLE state */
        nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;

        /* register callback to get interface error NTF */
        NFC_SetStaticRfCback (nfa_dm_disc_data_cback);
    }
    else
    {
        /* RF discovery is started but there is no valid technology or protocol to discover */
        nfa_dm_disc_notify_started (NFA_STATUS_OK);
    }

	/* reset protocol and hanlde of activated sub-module */
	nfa_dm_cb.disc_cb.activated_protocol = NFA_PROTOCOL_INVALID;
	nfa_dm_cb.disc_cb.activated_handle   = NFA_HANDLE_INVALID;
}

/*******************************************************************************
**
** Function         nfa_dm_notify_discovery
**
** Description      Send RF discovery notification to upper layer
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_notify_discovery (tNFA_DM_RF_DISC_DATA *p_data)
{
    tNFA_CONN_EVT_DATA conn_evt;

    /* let application select a device */
    conn_evt.disc_result.status = NFA_STATUS_OK;
    memcpy (&(conn_evt.disc_result.discovery_ntf),
            &(p_data->nfc_discover.result),
            sizeof (tNFC_RESULT_DEVT));

    nfa_dm_conn_cback_event_notify (NFA_DISC_RESULT_EVT, &conn_evt);
}


/*******************************************************************************
**
** Function         nfa_dm_disc_notify_activation
**
** Description      Send RF activation notification to sub-module
**
** Returns          NFA_STATUS_OK if success
**
*******************************************************************************/
static tNFA_STATUS nfa_dm_disc_notify_activation (tNFC_DISCOVER *p_data)
{
    UINT8   xx, host_id_in_LRT;
    UINT8   iso_dep_t3t__listen = NFA_DM_DISC_NUM_ENTRIES;

    tNFC_RF_TECH_N_MODE tech_n_mode = p_data->activate.rf_tech_param.mode;
    tNFC_PROTOCOL       protocol    = p_data->activate.protocol;

    tNFA_DM_DISC_TECH_PROTO_MASK activated_disc_mask;

    NFA_TRACE_DEBUG2 ("nfa_dm_disc_notify_activation (): tech_n_mode:0x%X, proto:0x%X",
                       tech_n_mode, protocol);

    if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
    {
        nfa_dm_cb.disc_cb.activated_tech_mode    = tech_n_mode;
        nfa_dm_cb.disc_cb.activated_rf_disc_id   = p_data->activate.rf_disc_id;
        nfa_dm_cb.disc_cb.activated_rf_interface = p_data->activate.intf_param.type;
        nfa_dm_cb.disc_cb.activated_protocol     = protocol;
        nfa_dm_cb.disc_cb.activated_handle       = NFA_HANDLE_INVALID;

        if (nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)
            (*(nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)) (NFA_DM_RF_DISC_ACTIVATED_EVT, p_data);

        return (NFA_STATUS_OK);
    }

    /* if this is NFCEE direct RF interface, notify activation to whoever listening UICC */
    if (p_data->activate.intf_param.type == NFC_INTERFACE_EE_DIRECT_RF)
    {
        for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
        {
            if (  (nfa_dm_cb.disc_cb.entry[xx].in_use)
#if (NXP_EXTNS != TRUE)
                &&(nfa_dm_cb.disc_cb.entry[xx].host_id != NFA_DM_DISC_HOST_ID_DH)
#endif
               )
            {
                nfa_dm_cb.disc_cb.activated_rf_disc_id   = p_data->activate.rf_disc_id;
                nfa_dm_cb.disc_cb.activated_rf_interface = p_data->activate.intf_param.type;
                nfa_dm_cb.disc_cb.activated_protocol     = NFC_PROTOCOL_UNKNOWN;
                nfa_dm_cb.disc_cb.activated_handle       = xx;

                NFA_TRACE_DEBUG2 ("activated_rf_interface:0x%x, activated_handle: 0x%x",
                                   nfa_dm_cb.disc_cb.activated_rf_interface,
                                   nfa_dm_cb.disc_cb.activated_handle);

                if (nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)
                    (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_ACTIVATED_EVT, p_data);

                return (NFA_STATUS_OK);
            }
        }
        return (NFA_STATUS_FAILED);
    }

#if(NXP_EXTNS == TRUE)
    /*
     * if this is Proprietary RF interface, notify activation as START_READER_EVT.
     *
     * Code to handle the Reader over SWP.
     * 1. Pass this info to JNI as START_READER_EVT.
     * return (NFA_STATUS_OK)
     */
    if (p_data->activate.intf_param.type  == NCI_INTERFACE_UICC_DIRECT || p_data->activate.intf_param.type == NCI_INTERFACE_ESE_DIRECT)
    {
        for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
        {
            if (  (nfa_dm_cb.disc_cb.entry[xx].in_use)
                &&(nfa_dm_cb.disc_cb.entry[xx].host_id != NFA_DM_DISC_HOST_ID_DH))
            {
                nfa_dm_cb.disc_cb.activated_rf_interface = p_data->activate.intf_param.type;
                nfa_dm_cb.disc_cb.activated_handle       = xx;

                NFA_TRACE_DEBUG2 ("activated_rf_uicc-ese_interface:0x%x, activated_handle: 0x%x",
                                   nfa_dm_cb.disc_cb.activated_rf_interface,
                                   nfa_dm_cb.disc_cb.activated_handle);

                if (nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)
                    (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_ACTIVATED_EVT, p_data);

                return (NFA_STATUS_OK);
            }
        }
        return (NFA_STATUS_FAILED);
    }
#endif

    /* get bit mask of technolgies/mode and protocol */
    activated_disc_mask = nfa_dm_disc_get_disc_mask (tech_n_mode, protocol);

    /* get host ID of technology from listen mode routing table */
    if (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_A)
    {
        host_id_in_LRT = nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_A];
    }
    else if (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_B)
    {
        host_id_in_LRT = nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_B];
    }
    else if (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_F)
    {
        host_id_in_LRT = nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_F];
    }
    else if (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_B_PRIME)
    {
        host_id_in_LRT = nfa_dm_cb.disc_cb.listen_RT[NFA_DM_DISC_LRT_NFC_BP];
    }
    else    /* DH only */
    {
        host_id_in_LRT = NFA_DM_DISC_HOST_ID_DH;
    }

    if (protocol == NFC_PROTOCOL_NFC_DEP)
    {
        /* Force NFC-DEP to the host */
        host_id_in_LRT = NFA_DM_DISC_HOST_ID_DH;
    }

    for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
    {
        /* if any matching NFC technology and protocol */
        if (nfa_dm_cb.disc_cb.entry[xx].in_use)
        {
            if (nfa_dm_cb.disc_cb.entry[xx].host_id == host_id_in_LRT)
            {
                if (nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask & activated_disc_mask)
                {
                    break;
                }
            }
            else
            {
                /* check ISO-DEP listening even if host in LRT is not matched */
                if (protocol == NFC_PROTOCOL_ISO_DEP)
                {
                    if (  (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_A)
                        &&(nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask & NFA_DM_DISC_MASK_LA_ISO_DEP))
                    {
                        iso_dep_t3t__listen = xx;
                    }
                    else if (  (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_B)
                             &&(nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask & NFA_DM_DISC_MASK_LB_ISO_DEP))
                    {
                        iso_dep_t3t__listen = xx;
                    }
                }
                /* check T3T listening even if host in LRT is not matched */
                else if (protocol == NFC_PROTOCOL_T3T)
                {
                    if (  (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_F)
                        &&(nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask & NFA_DM_DISC_MASK_LF_T3T))
                    {
                        iso_dep_t3t__listen = xx;
                    }
                }
            }
        }
    }

    if (xx >= NFA_DM_DISC_NUM_ENTRIES)
    {
        /* if any ISO-DEP or T3T listening even if host in LRT is not matched */
        xx = iso_dep_t3t__listen;
    }

#if(NXP_EXTNS == TRUE)
    if(protocol == NFC_PROTOCOL_NFC_DEP &&
            (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_F_ACTIVE ||
                    tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_A_ACTIVE ||
                    tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_A
                    )
      )
    {
        if(appl_dta_mode_flag == 1 &&
          (tech_n_mode == NFC_DISCOVERY_TYPE_LISTEN_A))
        {
            NFA_TRACE_DEBUG0("DTA Mode Enabled : NFC-A Passive Listen Mode");
        }
    }

#endif

    if (xx < NFA_DM_DISC_NUM_ENTRIES)
    {
        nfa_dm_cb.disc_cb.activated_tech_mode    = tech_n_mode;
        nfa_dm_cb.disc_cb.activated_rf_disc_id   = p_data->activate.rf_disc_id;
        nfa_dm_cb.disc_cb.activated_rf_interface = p_data->activate.intf_param.type;
        nfa_dm_cb.disc_cb.activated_protocol     = protocol;
        nfa_dm_cb.disc_cb.activated_handle       = xx;

        NFA_TRACE_DEBUG2 ("activated_protocol:0x%x, activated_handle: 0x%x",
                           nfa_dm_cb.disc_cb.activated_protocol,
                           nfa_dm_cb.disc_cb.activated_handle);

        if (nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)
            (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_ACTIVATED_EVT, p_data);

        return (NFA_STATUS_OK);
    }
    else
    {
        nfa_dm_cb.disc_cb.activated_protocol = NFA_PROTOCOL_INVALID;
        nfa_dm_cb.disc_cb.activated_handle   = NFA_HANDLE_INVALID;
        return (NFA_STATUS_FAILED);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_notify_deactivation
**
** Description      Send deactivation notification to sub-module
**
** Returns          None
**
*******************************************************************************/
static void nfa_dm_disc_notify_deactivation (tNFA_DM_RF_DISC_SM_EVENT sm_event,
                                             tNFC_DISCOVER *p_data)
{
    tNFA_HANDLE         xx;
    tNFA_CONN_EVT_DATA  evt_data;
    tNFC_DISCOVER       disc_data;

    NFA_TRACE_DEBUG1 ("nfa_dm_disc_notify_deactivation (): activated_handle=%d",
                       nfa_dm_cb.disc_cb.activated_handle);

    if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_CHECKING)
    {
        NFA_TRACE_DEBUG0 ("nfa_dm_disc_notify_deactivation (): for sleep wakeup");
        return;
    }

    if (sm_event == NFA_DM_RF_DEACTIVATE_RSP)
    {
        /*
        ** Activation has been aborted by upper layer in NFA_DM_RFST_W4_ALL_DISCOVERIES or NFA_DM_RFST_W4_HOST_SELECT
        ** Deactivation by upper layer or RF link loss in NFA_DM_RFST_LISTEN_SLEEP
        ** No sub-module is activated at this state.
        */

        if (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_LISTEN_SLEEP)
        {
            if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
            {
                if (nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)
                {
                    disc_data.deactivate.type = NFA_DEACTIVATE_TYPE_IDLE;
                    (*(nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)) (NFA_DM_RF_DISC_DEACTIVATED_EVT, &disc_data);
                }
            }
            else
            {
                /* let each sub-module handle deactivation */
                for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
                {
                    if (  (nfa_dm_cb.disc_cb.entry[xx].in_use)
                        &&(nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask & NFA_DM_DISC_MASK_LISTEN)  )
                    {
                        disc_data.deactivate.type = NFA_DEACTIVATE_TYPE_IDLE;
                        (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_DEACTIVATED_EVT, &disc_data);
                    }
                }
            }
        }
        else if (  (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_STOPPING))
                 ||(nfa_dm_cb.disc_cb.deact_notify_pending)  )
        {
            xx = nfa_dm_cb.disc_cb.activated_handle;

            /* notify event to activated module if failed while reactivation */
            if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
            {
                if (nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)
                {
                    disc_data.deactivate.type = NFA_DEACTIVATE_TYPE_IDLE;
                    (*(nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)) (NFA_DM_RF_DISC_DEACTIVATED_EVT, p_data);
                }
            }
            else if (  (xx < NFA_DM_DISC_NUM_ENTRIES)
                     &&(nfa_dm_cb.disc_cb.entry[xx].in_use)
                     &&(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)  )
            {
                (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_DEACTIVATED_EVT, p_data);
            }
            else
            {
                /* notify deactivation to application if there is no activated module */
                evt_data.deactivated.type = NFA_DEACTIVATE_TYPE_IDLE;
                nfa_dm_conn_cback_event_notify (NFA_DEACTIVATED_EVT, &evt_data);
            }
        }
    }
    else
    {
        /* notify event to activated module */
        if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
        {
            if (nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)
            {
                disc_data.deactivate.type = NFA_DEACTIVATE_TYPE_IDLE;
                (*(nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)) (NFA_DM_RF_DISC_DEACTIVATED_EVT, p_data);
            }
        }
        else
        {
            xx = nfa_dm_cb.disc_cb.activated_handle;

            if ((xx < NFA_DM_DISC_NUM_ENTRIES) && (nfa_dm_cb.disc_cb.entry[xx].in_use))
            {
                if (nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)
                    (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_DEACTIVATED_EVT, p_data);
            }
        }
    }

    /* clear activated information */
    nfa_dm_cb.disc_cb.activated_tech_mode    = 0;
    nfa_dm_cb.disc_cb.activated_rf_disc_id   = 0;
    nfa_dm_cb.disc_cb.activated_rf_interface = 0;
    nfa_dm_cb.disc_cb.activated_protocol     = NFA_PROTOCOL_INVALID;
    nfa_dm_cb.disc_cb.activated_handle       = NFA_HANDLE_INVALID;
    nfa_dm_cb.disc_cb.deact_notify_pending   = FALSE;
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sleep_wakeup
**
** Description      Put tag to sleep, then wake it up. Can be used Perform
**                  legacy presence check or to wake up tag that went to HALT
**                  state
**
** Returns          TRUE if operation started
**
*******************************************************************************/
tNFC_STATUS nfa_dm_disc_sleep_wakeup (void)
{
    tNFC_STATUS status = NFC_STATUS_FAILED;

    if (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_POLL_ACTIVE)
    {
        /* Deactivate to sleep mode */
        status = nfa_dm_send_deactivate_cmd(NFC_DEACTIVATE_TYPE_SLEEP);
        if (status == NFC_STATUS_OK)
        {
            /* deactivate to sleep is sent on behalf of sleep wakeup.
             * set the sleep wakeup information in control block */
            nfa_dm_cb.disc_cb.disc_flags    |= NFA_DM_DISC_FLAGS_CHECKING;
            nfa_dm_cb.disc_cb.deact_pending = FALSE;
        }
    }

    return (status);
}

/*******************************************************************************
**
** Function         nfa_dm_is_raw_frame_session
**
** Description      If NFA_SendRawFrame is called since RF activation,
**                  this function returns TRUE.
**
** Returns          TRUE if NFA_SendRawFrame is called
**
*******************************************************************************/
BOOLEAN nfa_dm_is_raw_frame_session (void)
{
    return ((nfa_dm_cb.flags & NFA_DM_FLAGS_RAW_FRAME) ? TRUE : FALSE);
}

/*******************************************************************************
**
** Function         nfa_dm_is_p2p_paused
**
** Description      If NFA_PauseP2p is called sand still effective,
**                  this function returns TRUE.
**
** Returns          TRUE if NFA_SendRawFrame is called
**
*******************************************************************************/
BOOLEAN nfa_dm_is_p2p_paused (void)
{
    return ((nfa_dm_cb.flags & NFA_DM_FLAGS_P2P_PAUSED) ? TRUE : FALSE);
}

/*******************************************************************************
**
** Function         nfa_dm_disc_end_sleep_wakeup
**
** Description      Sleep Wakeup is complete
**
** Returns          None
**
*******************************************************************************/
static void nfa_dm_disc_end_sleep_wakeup (tNFC_STATUS status)
{

    if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_CHECKING)
    {
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_CHECKING;

        if (nfa_dm_cb.disc_cb.deact_pending)
        {
            nfa_dm_cb.disc_cb.deact_pending = FALSE;
            /* Perform pending deactivate command and on response notfiy deactivation */
            nfa_dm_cb.disc_cb.deact_notify_pending = TRUE;
            nfa_dm_disc_sm_execute (NFA_DM_RF_DEACTIVATE_CMD,
                                   (tNFA_DM_RF_DISC_DATA *) &nfa_dm_cb.disc_cb.pending_deact_type);
        }
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_data_cback
**
** Description      Monitoring interface error through data callback
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_data_cback (UINT8 conn_id, tNFC_CONN_EVT event, tNFC_CONN *p_data)
{
    NFA_TRACE_DEBUG0 ("nfa_dm_disc_data_cback ()");

    /* if selection failed */
    if (event == NFC_ERROR_CEVT)
    {
        nfa_dm_disc_sm_execute (NFA_DM_CORE_INTF_ERROR_NTF, NULL);
    }
    else if (event == NFC_DATA_CEVT)
    {
        GKI_freebuf (p_data->data.p_data);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_new_state
**
** Description      Processing discovery events in NFA_DM_RFST_IDLE state
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_disc_new_state (tNFA_DM_RF_DISC_STATE new_state)
{
    tNFA_CONN_EVT_DATA      evt_data;
    tNFA_DM_RF_DISC_STATE   old_state = nfa_dm_cb.disc_cb.disc_state;

#if (BT_TRACE_VERBOSE == TRUE)
    NFA_TRACE_DEBUG5 ("nfa_dm_disc_new_state (): old_state: %s (%d), new_state: %s (%d) disc_flags: 0x%x",
                       nfa_dm_disc_state_2_str (nfa_dm_cb.disc_cb.disc_state), nfa_dm_cb.disc_cb.disc_state,
                       nfa_dm_disc_state_2_str (new_state), new_state, nfa_dm_cb.disc_cb.disc_flags);
#else
    NFA_TRACE_DEBUG3 ("nfa_dm_disc_new_state(): old_state: %d, new_state: %d disc_flags: 0x%x",
                       nfa_dm_cb.disc_cb.disc_state, new_state, nfa_dm_cb.disc_cb.disc_flags);
#endif

    nfa_dm_cb.disc_cb.disc_state = new_state;

    if (  (new_state == NFA_DM_RFST_IDLE)
        &&(!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))  ) /* not error recovering */
    {
        if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_STOPPING)
        {
            nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_STOPPING;

            /* if exclusive RF control is stopping */
            if (nfa_dm_cb.flags & NFA_DM_FLAGS_EXCL_RF_ACTIVE)
            {
                if (old_state > NFA_DM_RFST_DISCOVERY)
                {
                    /* notify deactivation to application */
                    evt_data.deactivated.type = NFA_DEACTIVATE_TYPE_IDLE;
                    nfa_dm_conn_cback_event_notify (NFA_DEACTIVATED_EVT, &evt_data);
                }

                nfa_dm_rel_excl_rf_control_and_notify ();
            }
            else
            {
                evt_data.status = NFA_STATUS_OK;
                nfa_dm_conn_cback_event_notify (NFA_RF_DISCOVERY_STOPPED_EVT, &evt_data);
            }
        }
        if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_DISABLING)
        {
            nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_DISABLING;
            nfa_sys_check_disabled ();
        }
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_idle
**
** Description      Processing discovery events in NFA_DM_RFST_IDLE state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_idle (tNFA_DM_RF_DISC_SM_EVENT event,
                                 tNFA_DM_RF_DISC_DATA *p_data)
{
    UINT8              xx;

    switch (event)
    {
    case NFA_DM_RF_DISCOVER_CMD:
        nfa_dm_start_rf_discover ();
        break;

    case NFA_DM_RF_DISCOVER_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;

        if (p_data->nfc_discover.status == NFC_STATUS_OK)
        {
            nfa_dm_disc_new_state (NFA_DM_RFST_DISCOVERY);

            /* if RF discovery was stopped while waiting for response */
            if (nfa_dm_cb.disc_cb.disc_flags & (NFA_DM_DISC_FLAGS_STOPPING|NFA_DM_DISC_FLAGS_DISABLING))
            {
                /* stop discovery */
                nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;
                NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
                break;
            }

            if (nfa_dm_cb.disc_cb.excl_disc_entry.in_use)
            {
                if (nfa_dm_cb.disc_cb.excl_disc_entry.disc_flags & NFA_DM_DISC_FLAGS_NOTIFY)
                {
                    nfa_dm_cb.disc_cb.excl_disc_entry.disc_flags &= ~NFA_DM_DISC_FLAGS_NOTIFY;

                    if (nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)
                        (*(nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback)) (NFA_DM_RF_DISC_START_EVT, (tNFC_DISCOVER*) p_data);
                }
            }
            else
            {
                /* notify event to each module which is waiting for start */
                for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
                {
                    /* if registered module is waiting for starting discovery */
                    if (  (nfa_dm_cb.disc_cb.entry[xx].in_use)
                        &&(nfa_dm_cb.disc_cb.dm_disc_mask & nfa_dm_cb.disc_cb.entry[xx].selected_disc_mask)
                        &&(nfa_dm_cb.disc_cb.entry[xx].disc_flags & NFA_DM_DISC_FLAGS_NOTIFY)  )
                    {
                        nfa_dm_cb.disc_cb.entry[xx].disc_flags &= ~NFA_DM_DISC_FLAGS_NOTIFY;

                        if (nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)
                            (*(nfa_dm_cb.disc_cb.entry[xx].p_disc_cback)) (NFA_DM_RF_DISC_START_EVT, (tNFC_DISCOVER*) p_data);
                    }
                }

            }
            nfa_dm_disc_notify_started (p_data->nfc_discover.status);
        }
        else
        {
            /* in rare case that the discovery states of NFCC and DH mismatch and NFCC rejects Discover Cmd
             * deactivate idle and then start disvocery when got deactivate rsp */
            nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        break;

    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;

        /* if NFCC goes to idle successfully */
        if (p_data->nfc_discover.status == NFC_STATUS_OK)
        {
            /* if DH forced to go idle while waiting for deactivation NTF */
            if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF))
            {
                nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, &(p_data->nfc_discover));

                /* check any pending flags like NFA_DM_DISC_FLAGS_STOPPING or NFA_DM_DISC_FLAGS_DISABLING */
                nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
                /* check if need to restart discovery after resync discovery state with NFCC */
                nfa_dm_start_rf_discover ();
            }
            /* Otherwise, deactivating when getting unexpected activation */
        }
#if(NXP_EXTNS == TRUE)
        else if (p_data->nfc_discover.status == NCI_STATUS_SEMANTIC_ERROR)
        {
            /* check any pending flags like NFA_DM_DISC_FLAGS_STOPPING or NFA_DM_DISC_FLAGS_DISABLING */
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            /* check if need to restart discovery after resync discovery state with NFCC */
            nfa_dm_start_rf_discover ();
        }
#endif
        /* Otherwise, wait for deactivation NTF */
        break;

    case NFA_DM_RF_DEACTIVATE_NTF:
        /* if NFCC sent this after NFCC had rejected deactivate CMD to idle while deactivating */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF))
        {
            if (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_DISCOVERY)
            {
                /* stop discovery */
                nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;
                NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
            }
            else
            {
                nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, &(p_data->nfc_discover));
                /* check any pending flags like NFA_DM_DISC_FLAGS_STOPPING or NFA_DM_DISC_FLAGS_DISABLING */
                nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
                /* check if need to restart discovery after resync discovery state with NFCC */
                nfa_dm_start_rf_discover ();
            }
        }
        /* Otherwise, deactivated when received unexpected activation in idle state */
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_NTF;
        break;

    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        /* unexpected activation, deactivate to idle */
        nfa_dm_cb.disc_cb.disc_flags |= (NFA_DM_DISC_FLAGS_W4_RSP|NFA_DM_DISC_FLAGS_W4_NTF);
        NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        break;

    case NFA_DM_LP_LISTEN_CMD:
        nfa_dm_disc_new_state (NFA_DM_RFST_LP_LISTEN);
        break;

    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_idle (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_discovery
**
** Description      Processing discovery events in NFA_DM_RFST_DISCOVERY state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_discovery (tNFA_DM_RF_DISC_SM_EVENT event,
                                      tNFA_DM_RF_DISC_DATA *p_data)
{
    switch (event)
    {
    case NFA_DM_RF_DEACTIVATE_CMD:
        /* if deactivate CMD was not sent to NFCC */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))
        {
            nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;
            NFC_Deactivate (p_data->deactivate_type);
        }
        break;
    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;

        /* if it's not race condition between deactivate CMD and activate NTF */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF))
        {
            /* do not notify deactivated to idle in RF discovery state
            ** because it is internal or stopping RF discovery
            */

            /* there was no activation while waiting for deactivation RSP */
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        break;
    case NFA_DM_RF_DISCOVER_NTF:
#if(NXP_EXTNS == TRUE)
        /* Notification Type = NCI_DISCOVER_NTF_LAST_ABORT */
        if (p_data->nfc_discover.result.more == NCI_DISCOVER_NTF_LAST_ABORT)
        {
            /* Fix for multiple tags: After receiving deactivate event, restart discovery */
            NFA_TRACE_DEBUG0 ("Received NCI_DISCOVER_NTF_LAST_ABORT, sending deactivate command");
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        else
        {
#endif
            nfa_dm_disc_new_state (NFA_DM_RFST_W4_ALL_DISCOVERIES);
#if(NXP_EXTNS == TRUE)
        }
#endif
        nfa_dm_notify_discovery (p_data);
        break;
    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP)
        {
            NFA_TRACE_DEBUG0 ("RF Activated while waiting for deactivation RSP");
            /* it's race condition. DH has to wait for deactivation NTF */
            nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_NTF;
        }
        else
        {
            if (p_data->nfc_discover.activate.intf_param.type == NFC_INTERFACE_EE_DIRECT_RF)
            {
                nfa_dm_disc_new_state (NFA_DM_RFST_LISTEN_ACTIVE);
            }
#if(NXP_EXTNS == TRUE)
            /*
             * Handle the Reader over SWP.
             * Add condition UICC_DIRECT_INTF/ESE_DIRECT_INTF
             * set new state NFA_DM_RFST_POLL_ACTIVE
             * */
            else if (p_data->nfc_discover.activate.intf_param.type == NCI_INTERFACE_UICC_DIRECT ||
                    p_data->nfc_discover.activate.intf_param.type == NCI_INTERFACE_ESE_DIRECT )
            {
                nfa_dm_disc_new_state (NFA_DM_RFST_POLL_ACTIVE);
            }
#endif
            else if (p_data->nfc_discover.activate.rf_tech_param.mode & 0x80)
            {
                /* Listen mode */
                nfa_dm_disc_new_state (NFA_DM_RFST_LISTEN_ACTIVE);
            }
            else
            {
                /* Poll mode */
                nfa_dm_disc_new_state (NFA_DM_RFST_POLL_ACTIVE);
            }

            if (nfa_dm_disc_notify_activation (&(p_data->nfc_discover)) == NFA_STATUS_FAILED)
            {
                NFA_TRACE_DEBUG0 ("Not matched, restart discovery after receiving deactivate ntf");

                /* after receiving deactivate event, restart discovery */
                nfa_dm_cb.disc_cb.disc_flags |= (NFA_DM_DISC_FLAGS_W4_RSP|NFA_DM_DISC_FLAGS_W4_NTF);
                NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
            }
        }
        break;

    case NFA_DM_RF_DEACTIVATE_NTF:
        /* if there was race condition between deactivate CMD and activate NTF */
        if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF)
        {
            /* race condition is resolved */
            nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_NTF;

            if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))
            {
                /* do not notify deactivated to idle in RF discovery state
                ** because it is internal or stopping RF discovery
                */

                nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
                nfa_dm_start_rf_discover ();
            }
        }
        break;
    case NFA_DM_LP_LISTEN_CMD:
        break;
    case NFA_DM_CORE_INTF_ERROR_NTF:
        break;
    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_discovery (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_w4_all_discoveries
**
** Description      Processing discovery events in NFA_DM_RFST_W4_ALL_DISCOVERIES state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_w4_all_discoveries (tNFA_DM_RF_DISC_SM_EVENT event,
                                               tNFA_DM_RF_DISC_DATA *p_data)
{
    switch (event)
    {
    case NFA_DM_RF_DEACTIVATE_CMD:
        /* if deactivate CMD was not sent to NFCC */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))
        {
            nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;
            /* only IDLE mode is allowed */
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        break;
    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;
        /* notify exiting from w4 all discoverie state */
        nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_RSP, &(p_data->nfc_discover));

        nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
        nfa_dm_start_rf_discover ();
        break;
    case NFA_DM_RF_DISCOVER_NTF:
#if(NXP_EXTNS == TRUE)
        if(p_data->nfc_discover.result.protocol == NCI_PROTOCOL_UNKNOWN)
        {
            /* fix for p2p interop with Nexus5 */
            NFA_TRACE_DEBUG0("Unknown protocol - Restart Discovery");
            /* after receiving unknown protocol, restart discovery */
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
            return;
        }
#endif
        /* if deactivate CMD is already sent then ignore discover NTF */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))
        {
            /* Notification Type = NCI_DISCOVER_NTF_LAST or NCI_DISCOVER_NTF_LAST_ABORT */
            if (p_data->nfc_discover.result.more != NCI_DISCOVER_NTF_MORE)
            {
                nfa_dm_disc_new_state (NFA_DM_RFST_W4_HOST_SELECT);
            }
            nfa_dm_notify_discovery (p_data);
        }
        break;
    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        /*
        ** This is only for ISO15693.
        ** FW sends activation NTF when all responses are received from tags without host selecting.
        */
        nfa_dm_disc_new_state (NFA_DM_RFST_POLL_ACTIVE);

        if (nfa_dm_disc_notify_activation (&(p_data->nfc_discover)) == NFA_STATUS_FAILED)
        {
            NFA_TRACE_DEBUG0 ("Not matched, restart discovery after receiving deactivate ntf");

            /* after receiving deactivate event, restart discovery */
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        break;
    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_w4_all_discoveries (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_w4_host_select
**
** Description      Processing discovery events in NFA_DM_RFST_W4_HOST_SELECT state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_w4_host_select (tNFA_DM_RF_DISC_SM_EVENT event,
                                           tNFA_DM_RF_DISC_DATA *p_data)
{
    tNFA_CONN_EVT_DATA conn_evt;
    tNFA_DM_DISC_FLAGS  old_sleep_wakeup_flag = (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_CHECKING);
    BOOLEAN             sleep_wakeup_event = FALSE;
    BOOLEAN             sleep_wakeup_event_processed = FALSE;
    tNFA_STATUS         status;

    switch (event)
    {
    case NFA_DM_RF_DISCOVER_SELECT_CMD:
        /* if not waiting to deactivate */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))
        {
            NFC_DiscoverySelect (p_data->select.rf_disc_id,
                                 p_data->select.protocol,
                                 p_data->select.rf_interface);
        }
        else
        {
            nfa_dm_disc_conn_event_notify (NFA_SELECT_RESULT_EVT, NFA_STATUS_FAILED);
        }
        break;

    case NFA_DM_RF_DISCOVER_SELECT_RSP:
        sleep_wakeup_event = TRUE;
        /* notify application status of selection */
        if (p_data->nfc_discover.status == NFC_STATUS_OK)
        {
            sleep_wakeup_event_processed = TRUE;
            conn_evt.status = NFA_STATUS_OK;
            /* register callback to get interface error NTF */
            NFC_SetStaticRfCback (nfa_dm_disc_data_cback);
        }
        else
            conn_evt.status = NFA_STATUS_FAILED;

        if (!old_sleep_wakeup_flag)
        {
            nfa_dm_disc_conn_event_notify (NFA_SELECT_RESULT_EVT, p_data->nfc_discover.status);
        }
        break;
    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        nfa_dm_disc_new_state (NFA_DM_RFST_POLL_ACTIVE);
#if(NXP_EXTNS == TRUE)
        /* always call nfa_dm_disc_notify_activation to update protocol/interface information in NFA control blocks */
        status = nfa_dm_disc_notify_activation (&(p_data->nfc_discover));
#endif
        if (old_sleep_wakeup_flag)
        {
            /* Handle sleep wakeup success: notify RW module of sleep wakeup of tag; if deactivation is pending then deactivate  */
            nfa_dm_disc_end_sleep_wakeup (NFC_STATUS_OK);
        }
        else if (status == NFA_STATUS_FAILED)
        {
            NFA_TRACE_DEBUG0 ("Not matched, restart discovery after receiving deactivate ntf");

            /* after receiving deactivate event, restart discovery */
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        break;
    case NFA_DM_RF_DEACTIVATE_CMD:
        if (old_sleep_wakeup_flag)
        {
            nfa_dm_cb.disc_cb.deact_pending      = TRUE;
            nfa_dm_cb.disc_cb.pending_deact_type = p_data->deactivate_type;
        }
        /* if deactivate CMD was not sent to NFCC */
        else if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP))
        {
            nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_W4_RSP;
            /* only IDLE mode is allowed */
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        break;
    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;
        /* notify exiting from host select state */
        nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_RSP, &(p_data->nfc_discover));

        nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
        nfa_dm_start_rf_discover ();
        break;

    case NFA_DM_CORE_INTF_ERROR_NTF:
        sleep_wakeup_event    = TRUE;
        if (!old_sleep_wakeup_flag)
        {
            /* target activation failed, upper layer may deactivate or select again */
            conn_evt.status = NFA_STATUS_FAILED;
            nfa_dm_conn_cback_event_notify (NFA_SELECT_RESULT_EVT, &conn_evt);
        }
        break;
    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_w4_host_select (): Unexpected discovery event");
#if(NXP_EXTNS == TRUE)
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_w4_host_select (): Restarted discovery");
        NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
#endif
        break;
    }

    if (old_sleep_wakeup_flag && sleep_wakeup_event && !sleep_wakeup_event_processed)
    {
        /* performing sleep wakeup and exception conditions happened
         * clear sleep wakeup information and report failure */
        nfa_dm_disc_end_sleep_wakeup (NFC_STATUS_FAILED);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_poll_active
**
** Description      Processing discovery events in NFA_DM_RFST_POLL_ACTIVE state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_poll_active (tNFA_DM_RF_DISC_SM_EVENT event,
                                        tNFA_DM_RF_DISC_DATA *p_data)
{
    tNFC_STATUS status;
    tNFA_DM_DISC_FLAGS  old_sleep_wakeup_flag = (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_CHECKING);
    BOOLEAN             sleep_wakeup_event = FALSE;
    BOOLEAN             sleep_wakeup_event_processed = FALSE;
    tNFC_DEACTIVATE_DEVT deact;
    /*Changed part of warning fixes in wearable project*/
    UNUSED(status);

    switch (event)
    {
    case NFA_DM_RF_DEACTIVATE_CMD:
        if (nfa_dm_cb.disc_cb.activated_protocol == NCI_PROTOCOL_MIFARE)
        {
            nfa_dm_cb.disc_cb.deact_pending = TRUE;
            nfa_dm_cb.disc_cb.pending_deact_type    = p_data->deactivate_type;
            status = nfa_dm_send_deactivate_cmd(p_data->deactivate_type);
            break;
        }

        if (old_sleep_wakeup_flag)
        {
            /* sleep wakeup is already enabled when deactivate cmd is requested,
             * keep the information in control block to issue it later */
            nfa_dm_cb.disc_cb.deact_pending      = TRUE;
            nfa_dm_cb.disc_cb.pending_deact_type = p_data->deactivate_type;
        }
        else
        {
            status = nfa_dm_send_deactivate_cmd(p_data->deactivate_type);
        }

        break;
    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;
        /* register callback to get interface error NTF */
        NFC_SetStaticRfCback (nfa_dm_disc_data_cback);

        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF))
        {
            /* it's race condition. received deactivate NTF before receiving RSP */

            deact.status = NFC_STATUS_OK;
            deact.type   = NFC_DEACTIVATE_TYPE_IDLE;
            deact.is_ntf = TRUE;
            nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, (tNFC_DISCOVER*)&deact);

            /* NFCC is in IDLE state */
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        break;
    case NFA_DM_RF_DEACTIVATE_NTF:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_NTF;

        nfa_sys_stop_timer (&nfa_dm_cb.disc_cb.tle);

        if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP)
        {
            /* it's race condition. received deactivate NTF before receiving RSP */
            /* notify deactivation after receiving deactivate RSP */
            NFA_TRACE_DEBUG0 ("Rx deactivate NTF while waiting for deactivate RSP");
            break;
        }

        sleep_wakeup_event    = TRUE;

        nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, &(p_data->nfc_discover));

        if (  (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_SLEEP)
            ||(p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_SLEEP_AF)  )
        {
            nfa_dm_disc_new_state (NFA_DM_RFST_W4_HOST_SELECT);
            if (old_sleep_wakeup_flag)
            {
                sleep_wakeup_event_processed  = TRUE;
                /* process pending deactivate request */
                if (nfa_dm_cb.disc_cb.deact_pending)
                {
                    /* notify RW module that sleep wakeup is finished */
                    /* if deactivation is pending then deactivate  */
                    nfa_dm_disc_end_sleep_wakeup (NFC_STATUS_OK);
                }
                else
                {
                    /* Successfully went to sleep mode for sleep wakeup */
                    /* Now wake up the tag to complete the operation */
                    NFC_DiscoverySelect (nfa_dm_cb.disc_cb.activated_rf_disc_id,
                                         nfa_dm_cb.disc_cb.activated_protocol,
                                         nfa_dm_cb.disc_cb.activated_rf_interface);
                }

            }
        }
        else if (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_IDLE)
        {
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        else if (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_DISCOVERY)
        {
            nfa_dm_disc_new_state (NFA_DM_RFST_DISCOVERY);
            if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_STOPPING)
            {
                /* stop discovery */
                NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
            }
        }
        break;

    case NFA_DM_CORE_INTF_ERROR_NTF:
#if(NXP_EXTNS != TRUE)
        sleep_wakeup_event    = TRUE;
        if (  (!old_sleep_wakeup_flag)
            ||(!nfa_dm_cb.disc_cb.deact_pending)  )
        {
            nfa_dm_send_deactivate_cmd (NFA_DEACTIVATE_TYPE_DISCOVERY);
        }
#endif
        break;

    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_poll_active (): Unexpected discovery event");
        break;
    }

    if (old_sleep_wakeup_flag && sleep_wakeup_event && !sleep_wakeup_event_processed)
    {
        /* performing sleep wakeup and exception conditions happened
         * clear sleep wakeup information and report failure */
        nfa_dm_disc_end_sleep_wakeup (NFC_STATUS_FAILED);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_listen_active
**
** Description      Processing discovery events in NFA_DM_RFST_LISTEN_ACTIVE state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_listen_active (tNFA_DM_RF_DISC_SM_EVENT event,
                                          tNFA_DM_RF_DISC_DATA     *p_data)
{
    tNFC_DEACTIVATE_DEVT deact;

    switch (event)
    {
    case NFA_DM_RF_DEACTIVATE_CMD:
        nfa_dm_send_deactivate_cmd(p_data->deactivate_type);
        break;
    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF))
        {
            /* it's race condition. received deactivate NTF before receiving RSP */

            deact.status = NFC_STATUS_OK;
            deact.type   = NFC_DEACTIVATE_TYPE_IDLE;
            deact.is_ntf = TRUE;
            nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, (tNFC_DISCOVER*)&deact);

            /* NFCC is in IDLE state */
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        break;
    case NFA_DM_RF_DEACTIVATE_NTF:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_NTF;

        nfa_sys_stop_timer (&nfa_dm_cb.disc_cb.tle);

        if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_RSP)
        {
            /* it's race condition. received deactivate NTF before receiving RSP */
            /* notify deactivation after receiving deactivate RSP */
            NFA_TRACE_DEBUG0 ("Rx deactivate NTF while waiting for deactivate RSP");
        }
        else
        {
            nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, &(p_data->nfc_discover));

            if (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_IDLE)
            {
                nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
                nfa_dm_start_rf_discover ();
            }
            else if (  (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_SLEEP)
                     ||(p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_SLEEP_AF)  )
            {
                nfa_dm_disc_new_state (NFA_DM_RFST_LISTEN_SLEEP);
            }
            else if (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_DISCOVERY)
            {
                /* Discovery */
                nfa_dm_disc_new_state (NFA_DM_RFST_DISCOVERY);
                if (nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_STOPPING)
                {
                    /* stop discovery */
                    NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
                }
            }
        }
        break;

    case NFA_DM_CORE_INTF_ERROR_NTF:
        break;
    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_listen_active (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_listen_sleep
**
** Description      Processing discovery events in NFA_DM_RFST_LISTEN_SLEEP state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_listen_sleep (tNFA_DM_RF_DISC_SM_EVENT event,
                                         tNFA_DM_RF_DISC_DATA *p_data)
{
    switch (event)
    {
    case NFA_DM_RF_DEACTIVATE_CMD:
        nfa_dm_send_deactivate_cmd (p_data->deactivate_type);

        /* if deactivate type is not discovery then NFCC will not sent deactivation NTF */
        if (p_data->deactivate_type != NFA_DEACTIVATE_TYPE_DISCOVERY)
        {
            nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_NTF;
            nfa_sys_stop_timer (&nfa_dm_cb.disc_cb.tle);
        }
        break;
    case NFA_DM_RF_DEACTIVATE_RSP:
        nfa_dm_cb.disc_cb.disc_flags &= ~NFA_DM_DISC_FLAGS_W4_RSP;
        /* if deactivate type in CMD was IDLE */
        if (!(nfa_dm_cb.disc_cb.disc_flags & NFA_DM_DISC_FLAGS_W4_NTF))
        {
            nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_RSP, &(p_data->nfc_discover));

            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        break;
    case NFA_DM_RF_DEACTIVATE_NTF:
        /* clear both W4_RSP and W4_NTF because of race condition between deactivat CMD and link loss */
        nfa_dm_cb.disc_cb.disc_flags &= ~(NFA_DM_DISC_FLAGS_W4_RSP|NFA_DM_DISC_FLAGS_W4_NTF);
        nfa_sys_stop_timer (&nfa_dm_cb.disc_cb.tle);

        /* there is no active protocol in this state, so broadcast to all by using NFA_DM_RF_DEACTIVATE_RSP */
        nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_RSP, &(p_data->nfc_discover));

        if (p_data->nfc_discover.deactivate.type == NFC_DEACTIVATE_TYPE_IDLE)
        {
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        else if (p_data->nfc_discover.deactivate.type == NFA_DEACTIVATE_TYPE_DISCOVERY)
        {
            nfa_dm_disc_new_state (NFA_DM_RFST_DISCOVERY);
        }
        else
        {
            NFA_TRACE_ERROR0 ("Unexpected deactivation type");
            nfa_dm_disc_new_state (NFA_DM_RFST_IDLE);
            nfa_dm_start_rf_discover ();
        }
        break;
    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        nfa_dm_disc_new_state (NFA_DM_RFST_LISTEN_ACTIVE);
        if (nfa_dm_disc_notify_activation (&(p_data->nfc_discover)) == NFA_STATUS_FAILED)
        {
            NFA_TRACE_DEBUG0 ("Not matched, restart discovery after receiving deactivate ntf");

            /* after receiving deactivate event, restart discovery */
            NFC_Deactivate (NFA_DEACTIVATE_TYPE_IDLE);
        }
        break;
    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_listen_sleep (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_lp_listen
**
** Description      Processing discovery events in NFA_DM_RFST_LP_LISTEN state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_lp_listen (tNFA_DM_RF_DISC_SM_EVENT event,
                                           tNFA_DM_RF_DISC_DATA *p_data)
{
    switch (event)
    {
    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        nfa_dm_disc_new_state (NFA_DM_RFST_LP_ACTIVE);
        if (nfa_dm_disc_notify_activation (&(p_data->nfc_discover)) == NFA_STATUS_FAILED)
        {
            NFA_TRACE_DEBUG0 ("Not matched, unexpected activation");
        }
        break;

    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_lp_listen (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_lp_active
**
** Description      Processing discovery events in NFA_DM_RFST_LP_ACTIVE state
**
** Returns          void
**
*******************************************************************************/
static void nfa_dm_disc_sm_lp_active (tNFA_DM_RF_DISC_SM_EVENT event,
                                           tNFA_DM_RF_DISC_DATA *p_data)
{
    switch (event)
    {
    case NFA_DM_RF_DEACTIVATE_NTF:
        nfa_dm_disc_new_state (NFA_DM_RFST_LP_LISTEN);
        nfa_dm_disc_notify_deactivation (NFA_DM_RF_DEACTIVATE_NTF, &(p_data->nfc_discover));
        break;
    default:
        NFA_TRACE_ERROR0 ("nfa_dm_disc_sm_lp_active (): Unexpected discovery event");
        break;
    }
}

/*******************************************************************************
**
** Function         nfa_dm_disc_sm_execute
**
** Description      Processing discovery related events
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_disc_sm_execute (tNFA_DM_RF_DISC_SM_EVENT event, tNFA_DM_RF_DISC_DATA *p_data)
{
#if (BT_TRACE_VERBOSE == TRUE)
    NFA_TRACE_DEBUG5 ("nfa_dm_disc_sm_execute (): state: %s (%d), event: %s(%d) disc_flags: 0x%x",
                       nfa_dm_disc_state_2_str (nfa_dm_cb.disc_cb.disc_state), nfa_dm_cb.disc_cb.disc_state,
                       nfa_dm_disc_event_2_str (event), event, nfa_dm_cb.disc_cb.disc_flags);
#else
    NFA_TRACE_DEBUG3 ("nfa_dm_disc_sm_execute(): state: %d, event:%d disc_flags: 0x%x",
                       nfa_dm_cb.disc_cb.disc_state, event, nfa_dm_cb.disc_cb.disc_flags);
#endif

    switch (nfa_dm_cb.disc_cb.disc_state)
    {
    /*  RF Discovery State - Idle */
    case NFA_DM_RFST_IDLE:
        nfa_dm_disc_sm_idle (event, p_data);
        break;

    /* RF Discovery State - Discovery */
    case NFA_DM_RFST_DISCOVERY:
        nfa_dm_disc_sm_discovery (event, p_data);
        break;

    /*RF Discovery State - Wait for all discoveries */
    case NFA_DM_RFST_W4_ALL_DISCOVERIES:
        nfa_dm_disc_sm_w4_all_discoveries (event, p_data);
        break;

    /* RF Discovery State - Wait for host selection */
    case NFA_DM_RFST_W4_HOST_SELECT:
        nfa_dm_disc_sm_w4_host_select (event, p_data);
        break;

    /* RF Discovery State - Poll mode activated */
    case NFA_DM_RFST_POLL_ACTIVE:
        nfa_dm_disc_sm_poll_active (event, p_data);
        break;

    /* RF Discovery State - listen mode activated */
    case NFA_DM_RFST_LISTEN_ACTIVE:
        nfa_dm_disc_sm_listen_active (event, p_data);
        break;

    /* RF Discovery State - listen mode sleep */
    case NFA_DM_RFST_LISTEN_SLEEP:
        nfa_dm_disc_sm_listen_sleep (event, p_data);
        break;

    /* Listening in Low Power mode    */
    case NFA_DM_RFST_LP_LISTEN:
        nfa_dm_disc_sm_lp_listen (event, p_data);
        break;

    /* Activated in Low Power mode    */
    case NFA_DM_RFST_LP_ACTIVE:
        nfa_dm_disc_sm_lp_active (event, p_data);
        break;
    }
#if (BT_TRACE_VERBOSE == TRUE)
    NFA_TRACE_DEBUG3 ("nfa_dm_disc_sm_execute (): new state: %s (%d), disc_flags: 0x%x",
                       nfa_dm_disc_state_2_str (nfa_dm_cb.disc_cb.disc_state), nfa_dm_cb.disc_cb.disc_state,
                       nfa_dm_cb.disc_cb.disc_flags);
#else
    NFA_TRACE_DEBUG2 ("nfa_dm_disc_sm_execute(): new state: %d,  disc_flags: 0x%x",
                       nfa_dm_cb.disc_cb.disc_state, nfa_dm_cb.disc_cb.disc_flags);
#endif
}

/*******************************************************************************
**
** Function         nfa_dm_add_rf_discover
**
** Description      Add discovery configuration and callback function
**
** Returns          valid handle if success
**
*******************************************************************************/
tNFA_HANDLE nfa_dm_add_rf_discover (tNFA_DM_DISC_TECH_PROTO_MASK disc_mask,
                                    tNFA_DM_DISC_HOST_ID         host_id,
                                    tNFA_DISCOVER_CBACK         *p_disc_cback)
{
    UINT8       xx;

    NFA_TRACE_DEBUG1 ("nfa_dm_add_rf_discover () disc_mask=0x%x", disc_mask);

    for (xx = 0; xx < NFA_DM_DISC_NUM_ENTRIES; xx++)
    {
        if (!nfa_dm_cb.disc_cb.entry[xx].in_use)
        {
            NFA_TRACE_DEBUG2 ("nfa_dm_add_rf_discover () disc_mask=0x%x, xx=%d", disc_mask, xx);
            nfa_dm_cb.disc_cb.entry[xx].in_use              = TRUE;
            nfa_dm_cb.disc_cb.entry[xx].requested_disc_mask = disc_mask;
            nfa_dm_cb.disc_cb.entry[xx].host_id             = host_id;
            nfa_dm_cb.disc_cb.entry[xx].p_disc_cback        = p_disc_cback;
            nfa_dm_cb.disc_cb.entry[xx].disc_flags          = NFA_DM_DISC_FLAGS_NOTIFY;
            return xx;
        }
    }

    return NFA_HANDLE_INVALID;
}

/*******************************************************************************
**
** Function         nfa_dm_start_excl_discovery
**
** Description      Start exclusive RF discovery
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_start_excl_discovery (tNFA_TECHNOLOGY_MASK poll_tech_mask,
                                  tNFA_LISTEN_CFG *p_listen_cfg,
                                  tNFA_DISCOVER_CBACK  *p_disc_cback)
{
    tNFA_DM_DISC_TECH_PROTO_MASK poll_disc_mask = 0;

    NFA_TRACE_DEBUG0 ("nfa_dm_start_excl_discovery ()");

    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_A)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_PA_T1T;
        poll_disc_mask |= NFA_DM_DISC_MASK_PA_T2T;
        poll_disc_mask |= NFA_DM_DISC_MASK_PA_ISO_DEP;
        poll_disc_mask |= NFA_DM_DISC_MASK_PA_NFC_DEP;
        poll_disc_mask |= NFA_DM_DISC_MASK_P_LEGACY;
    }
    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_A_ACTIVE)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_PAA_NFC_DEP;
    }
    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_B)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_PB_ISO_DEP;
    }
    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_F)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_PF_T3T;
        poll_disc_mask |= NFA_DM_DISC_MASK_PF_NFC_DEP;
    }
    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_F_ACTIVE)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_PFA_NFC_DEP;
    }
    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_ISO15693)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_P_ISO15693;
    }
    if (poll_tech_mask & NFA_TECHNOLOGY_MASK_B_PRIME)
    {
        poll_disc_mask |= NFA_DM_DISC_MASK_P_B_PRIME;
    }

    nfa_dm_cb.disc_cb.excl_disc_entry.in_use              = TRUE;
    nfa_dm_cb.disc_cb.excl_disc_entry.requested_disc_mask = poll_disc_mask;
    nfa_dm_cb.disc_cb.excl_disc_entry.host_id             = NFA_DM_DISC_HOST_ID_DH;
    nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback        = p_disc_cback;
    nfa_dm_cb.disc_cb.excl_disc_entry.disc_flags          = NFA_DM_DISC_FLAGS_NOTIFY;

    memcpy (&nfa_dm_cb.disc_cb.excl_listen_config, p_listen_cfg, sizeof (tNFA_LISTEN_CFG));

    nfa_dm_disc_sm_execute (NFA_DM_RF_DISCOVER_CMD, NULL);
}

/*******************************************************************************
**
** Function         nfa_dm_stop_excl_discovery
**
** Description      Stop exclusive RF discovery
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_stop_excl_discovery (void)
{
    NFA_TRACE_DEBUG0 ("nfa_dm_stop_excl_discovery ()");

    nfa_dm_cb.disc_cb.excl_disc_entry.in_use       = FALSE;
    nfa_dm_cb.disc_cb.excl_disc_entry.p_disc_cback = NULL;
}

/*******************************************************************************
**
** Function         nfa_dm_delete_rf_discover
**
** Description      Remove discovery configuration and callback function
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_delete_rf_discover (tNFA_HANDLE handle)
{
    NFA_TRACE_DEBUG1 ("nfa_dm_delete_rf_discover () handle=0x%x", handle);

    if (handle < NFA_DM_DISC_NUM_ENTRIES)
    {
        nfa_dm_cb.disc_cb.entry[handle].in_use = FALSE;
    }
    else
    {
        NFA_TRACE_ERROR0 ("Invalid discovery handle");
    }
}

/*******************************************************************************
**
** Function         nfa_dm_rf_discover_select
**
** Description      Select target, protocol and RF interface
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_rf_discover_select (UINT8             rf_disc_id,
                                       tNFA_NFC_PROTOCOL protocol,
                                       tNFA_INTF_TYPE    rf_interface)
{
    tNFA_DM_DISC_SELECT_PARAMS select_params;
    tNFA_CONN_EVT_DATA conn_evt;

    NFA_TRACE_DEBUG3 ("nfa_dm_disc_select () rf_disc_id:0x%X, protocol:0x%X, rf_interface:0x%X",
                       rf_disc_id, protocol, rf_interface);

    if (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_W4_HOST_SELECT)
    {
        /* state is OK: notify the status when the response is received from NFCC */
        select_params.rf_disc_id   = rf_disc_id;
        select_params.protocol     = protocol;
        select_params.rf_interface = rf_interface;

        nfa_dm_cb.disc_cb.disc_flags |= NFA_DM_DISC_FLAGS_NOTIFY;
        nfa_dm_disc_sm_execute (NFA_DM_RF_DISCOVER_SELECT_CMD, (tNFA_DM_RF_DISC_DATA *) &select_params);
    }
    else
    {
        /* Wrong state: notify failed status right away */
        conn_evt.status = NFA_STATUS_FAILED;
        nfa_dm_conn_cback_event_notify (NFA_SELECT_RESULT_EVT, &conn_evt);
    }
}

/*******************************************************************************
**
** Function         nfa_dm_rf_deactivate
**
** Description      Deactivate NFC link
**
** Returns          NFA_STATUS_OK if success
**
*******************************************************************************/
tNFA_STATUS nfa_dm_rf_deactivate (tNFA_DEACTIVATE_TYPE deactivate_type)
{
    NFA_TRACE_DEBUG1 ("nfa_dm_rf_deactivate () deactivate_type:0x%X", deactivate_type);

    if (deactivate_type == NFA_DEACTIVATE_TYPE_SLEEP)
    {
        if (nfa_dm_cb.disc_cb.activated_protocol == NFA_PROTOCOL_NFC_DEP)
            deactivate_type = NFC_DEACTIVATE_TYPE_SLEEP_AF;
        else
            deactivate_type = NFC_DEACTIVATE_TYPE_SLEEP;
    }

    if (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_IDLE)
    {
        return NFA_STATUS_FAILED;
    }
    else if (nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_DISCOVERY)
    {
        if (deactivate_type == NFA_DEACTIVATE_TYPE_DISCOVERY)
        {
			/* it could be race condition. */
			NFA_TRACE_DEBUG0 ("nfa_dm_rf_deactivate (): already in discovery state");
			return NFA_STATUS_FAILED;
        }
        else if (deactivate_type == NFA_DEACTIVATE_TYPE_IDLE)
        {
            nfa_dm_disc_sm_execute (NFA_DM_RF_DEACTIVATE_CMD, (tNFA_DM_RF_DISC_DATA *) &deactivate_type);
            return NFA_STATUS_OK;
        }
        else
        {
            return NFA_STATUS_FAILED;
        }
    }
    else
    {
        nfa_dm_disc_sm_execute (NFA_DM_RF_DEACTIVATE_CMD, (tNFA_DM_RF_DISC_DATA *) &deactivate_type);
        return NFA_STATUS_OK;
    }
}

#if (BT_TRACE_VERBOSE == TRUE)
/*******************************************************************************
**
** Function         nfa_dm_disc_state_2_str
**
** Description      convert nfc discovery state to string
**
*******************************************************************************/
static char *nfa_dm_disc_state_2_str (UINT8 state)
{
    switch (state)
    {
    case NFA_DM_RFST_IDLE:
        return "IDLE";

    case NFA_DM_RFST_DISCOVERY:
        return "DISCOVERY";

    case NFA_DM_RFST_W4_ALL_DISCOVERIES:
        return "W4_ALL_DISCOVERIES";

    case NFA_DM_RFST_W4_HOST_SELECT:
        return "W4_HOST_SELECT";

    case NFA_DM_RFST_POLL_ACTIVE:
        return "POLL_ACTIVE";

    case NFA_DM_RFST_LISTEN_ACTIVE:
        return "LISTEN_ACTIVE";

    case NFA_DM_RFST_LISTEN_SLEEP:
        return "LISTEN_SLEEP";

    case NFA_DM_RFST_LP_LISTEN:
        return "LP_LISTEN";

    case NFA_DM_RFST_LP_ACTIVE:
        return "LP_ACTIVE";
    }
    return "Unknown";
}

/*******************************************************************************
**
** Function         nfa_dm_disc_event_2_str
**
** Description      convert nfc discovery RSP/NTF to string
**
*******************************************************************************/
static char *nfa_dm_disc_event_2_str (UINT8 event)
{
    switch (event)
    {
    case NFA_DM_RF_DISCOVER_CMD:
        return "DISCOVER_CMD";

    case NFA_DM_RF_DISCOVER_RSP:
        return "DISCOVER_RSP";

    case NFA_DM_RF_DISCOVER_NTF:
        return "DISCOVER_NTF";

    case NFA_DM_RF_DISCOVER_SELECT_CMD:
        return "SELECT_CMD";

    case NFA_DM_RF_DISCOVER_SELECT_RSP:
        return "SELECT_RSP";

    case NFA_DM_RF_INTF_ACTIVATED_NTF:
        return "ACTIVATED_NTF";

    case NFA_DM_RF_DEACTIVATE_CMD:
        return "DEACTIVATE_CMD";

    case NFA_DM_RF_DEACTIVATE_RSP:
        return "DEACTIVATE_RSP";

    case NFA_DM_RF_DEACTIVATE_NTF:
        return "DEACTIVATE_NTF";

    case NFA_DM_LP_LISTEN_CMD:
        return "NFA_DM_LP_LISTEN_CMD";

    case NFA_DM_CORE_INTF_ERROR_NTF:
        return "INTF_ERROR_NTF";

    }
    return "Unknown";
}
#endif /* BT_TRACE_VERBOSE */

/*******************************************************************************
**
** Function         P2P_Prio_Logic
**
** Description      Implements algorithm for NFC-DEP protocol priority over
**                  ISO-DEP protocol.
**
** Returns          True if success
**
*******************************************************************************/
BOOLEAN nfa_dm_p2p_prio_logic (UINT8 event, UINT8 *p, UINT8 event_type)
{
    if (!nfa_poll_bail_out_mode)
    {
        NFA_TRACE_DEBUG0 ("p2p priority is running under bail out mode ONLY.");
        return TRUE;
    }

    if((nfa_dm_cb.flags & NFA_DM_FLAGS_P2P_PAUSED) &&
       (nfa_dm_cb.flags & NFA_DM_FLAGS_LISTEN_DISABLED))
    {
        NFA_TRACE_DEBUG0("returning from nfa_dm_p2p_prio_logic  Disable p2p_prio_logic");
        return TRUE;
    }
#if(NXP_EXTNS == TRUE)
    if (TRUE == reconnect_in_progress)
    {
        NFA_TRACE_DEBUG0("returning from nfa_dm_p2p_prio_logic  reconnect_in_progress");
        return TRUE;
    }
    if(0x01 == appl_dta_mode_flag)
    {
        /*Disable the P2P Prio Logic when DTA is running*/
        return TRUE;
    }
#endif
    if(nfa_dm_cb.disc_cb.disc_state == NFA_DM_RFST_DISCOVERY)
    {
        UINT8 rf_disc_id = 0xFF;
        UINT8 type = 0xFF;
        UINT8 protocol = 0xFF;
        UINT8 tech_mode = 0xFF;

        /*Changed part of warning fixes in wearable project*/
        UNUSED(rf_disc_id);
        UNUSED(type);

        NFA_TRACE_DEBUG0 ("P2P_Prio_Logic");

        if (event == NCI_MSG_RF_INTF_ACTIVATED )
        {
            rf_disc_id    = *p++;
            type          = *p++;
            protocol      = *p++;
            tech_mode     = *p++;
        }

        if ((tech_mode < 0x80) &&
            event == NCI_MSG_RF_INTF_ACTIVATED &&
            protocol == NCI_PROTOCOL_ISO_DEP &&
            p2p_prio_logic_data.isodep_detected == FALSE)
        {
            memset(&p2p_prio_logic_data, 0x00, sizeof(nfa_dm_p2p_prio_logic_t));
            p2p_prio_logic_data.isodep_detected = TRUE;
            p2p_prio_logic_data.first_tech_mode = tech_mode;
            NFA_TRACE_DEBUG0 ("ISO-DEP Detected First Time  Resume the Polling Loop");
            nci_snd_deactivate_cmd(NFA_DEACTIVATE_TYPE_DISCOVERY);
            return FALSE;
        }

        else if(event == NCI_MSG_RF_INTF_ACTIVATED &&
                protocol == NCI_PROTOCOL_ISO_DEP &&
                p2p_prio_logic_data.isodep_detected == TRUE &&
                p2p_prio_logic_data.first_tech_mode != tech_mode)
        {
            p2p_prio_logic_data.isodep_detected = TRUE;
            p2p_prio_logic_data.timer_expired = FALSE;
            NFA_TRACE_DEBUG0 ("ISO-DEP Detected Second Time Other Techmode  Resume the Polling Loop");
            nfc_stop_quick_timer (&p2p_prio_logic_data.timer_list);
            nci_snd_deactivate_cmd(NFA_DEACTIVATE_TYPE_DISCOVERY);
            return FALSE;
        }

        else if (event == NCI_MSG_RF_INTF_ACTIVATED &&
                 protocol == NCI_PROTOCOL_ISO_DEP &&
                 p2p_prio_logic_data.isodep_detected == TRUE &&
                 p2p_prio_logic_data.timer_expired == TRUE)
        {
            NFA_TRACE_DEBUG0 ("ISO-DEP Detected TimerExpired, Final Notifying the Event");
            nfc_stop_quick_timer (&p2p_prio_logic_data.timer_list);
            memset(&p2p_prio_logic_data, 0x00, sizeof(nfa_dm_p2p_prio_logic_t));
        }

        else if (event == NCI_MSG_RF_INTF_ACTIVATED &&
                 protocol == NCI_PROTOCOL_ISO_DEP &&
                 p2p_prio_logic_data.isodep_detected == TRUE &&
                 p2p_prio_logic_data.first_tech_mode == tech_mode)
        {
            NFA_TRACE_DEBUG0 ("ISO-DEP Detected Same Techmode, Final Notifying the Event");
            nfc_stop_quick_timer (&p2p_prio_logic_data.timer_list);
            memset(&p2p_prio_logic_data, 0x00, sizeof(nfa_dm_p2p_prio_logic_t));
        }

        else if (event == NCI_MSG_RF_INTF_ACTIVATED &&
                 protocol != NCI_PROTOCOL_ISO_DEP &&
                 p2p_prio_logic_data.isodep_detected == TRUE)
        {
            NFA_TRACE_DEBUG0 ("ISO-DEP Not Detected  Giving Priority for other Technology");
            nfc_stop_quick_timer (&p2p_prio_logic_data.timer_list);
            memset(&p2p_prio_logic_data, 0x00, sizeof(nfa_dm_p2p_prio_logic_t));
        }

        else if (event == NCI_MSG_RF_DEACTIVATE &&
                 p2p_prio_logic_data.isodep_detected == TRUE &&
                 p2p_prio_logic_data.timer_expired == FALSE &&
                 event_type == NFA_DM_P2P_PRIO_RSP)
        {
            NFA_TRACE_DEBUG0 ("NFA_DM_RF_DEACTIVATE_RSP");
            return FALSE;
        }

        else if (event == NCI_MSG_RF_DEACTIVATE &&
                 p2p_prio_logic_data.isodep_detected == TRUE &&
                 p2p_prio_logic_data.timer_expired == FALSE &&
                 event_type == NFA_DM_P2P_PRIO_NTF)
        {
            NFA_TRACE_DEBUG0 ("NFA_DM_RF_DEACTIVATE_NTF");

            nfc_start_quick_timer (&p2p_prio_logic_data.timer_list,
                                   NFC_TTYPE_P2P_PRIO_RESPONSE,
                                   ((UINT32) 160 * QUICK_TIMER_TICKS_PER_SEC) / 1000 );

            NFA_TRACE_DEBUG0 ("P2P_Start_Timer");

            return FALSE;
        }
    }

    NFA_TRACE_DEBUG0("returning TRUE");
    return TRUE;
}

#if(NXP_EXTNS == TRUE)
void NFA_SetReconnectState (BOOLEAN flag)
{
    reconnect_in_progress = flag;
    NFA_TRACE_DEBUG1("NFA_SetReconnectState = 0x%x", reconnect_in_progress);
}
#endif

/*******************************************************************************
**
** Function         p2p_prio_logic_timeout
**
** Description      Callback function for p2p timer
**
** Returns          void
**
*******************************************************************************/
void nfa_dm_p2p_timer_event ()
{
    NFA_TRACE_DEBUG0 ("P2P_Timer_timeout NFC-DEP Not Discovered!!");

    p2p_prio_logic_data.timer_expired = 1;

    if(p2p_prio_logic_data.isodep_detected == 1)
    {
        NFA_TRACE_DEBUG0 ("Deactivate and Restart RF discovery");
        nci_snd_deactivate_cmd(NFC_DEACTIVATE_TYPE_IDLE);
    }
}
