//*****************************************************************************
//
//  ancc_main.c
//! @file
//!
//! @brief  apple notification center service client.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2018, Ambiq Micro
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
// 
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 1.2.12 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#include <string.h>
#include "wsf_types.h"
#include "wsf_assert.h"
#include "bstream.h"
#include "app_api.h"
#include "ancc_api.h"

#include "ry_utils.h"
#include "device_management_service.h"
#include "ble_task.h"
#include "ryos_timer.h"
#include "wsf_msg.h"

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

#define RY_ANCC_MSG_COUNTER   8

#ifndef ANCS_USE_RY_TIMER
  #define ANCS_USE_RY_TIMER   1
#endif

#define ANCC_TIMER_EVT_DEFAULT  ANCS_MSG_START

#define ANCC_HANDLE_PENDING_RETRY_MAX_TIMES     20      //等待一条指令write的resp超时20次仍然未收到则重置状态

/*!
 *  Apple Notification Center Service Client (ANCC)
 */

/* UUIDs */
static const uint8_t anccAncsSvcUuid[] = {ATT_UUID_ANCS_SERVICE};       /*! ANCS Service UUID */
static const uint8_t anccNSChUuid[] = {ATT_UUID_NOTIFICATION_SOURCE};    /*! Notification Source UUID */
static const uint8_t anccCPChUuid[] = {ATT_UUID_CTRL_POINT};             /*! control point UUID*/
static const uint8_t anccDSChUuid[] = {ATT_UUID_DATA_SOURCE};            /*! data source UUID*/
/* Characteristics for discovery */

/*! Proprietary data */
static const attcDiscChar_t anccNSDat =
{
    anccNSChUuid,
    ATTC_SET_REQUIRED | ATTC_SET_UUID_128
};

/*! Proprietary data descriptor */  //notification source
static const attcDiscChar_t anccNSdatCcc =
{
    attCliChCfgUuid,
    ATTC_SET_REQUIRED | ATTC_SET_DESCRIPTOR
};
// control point
static const attcDiscChar_t anccCtrlPoint =
{
    anccCPChUuid,
    ATTC_SET_REQUIRED | ATTC_SET_UUID_128
};

// data source
static const attcDiscChar_t anccDataSrc =
{
    anccDSChUuid,
    ATTC_SET_REQUIRED | ATTC_SET_UUID_128
};
/*! Proprietary data descriptor */  //data source
static const attcDiscChar_t anccDataSrcCcc =
{
    attCliChCfgUuid,
    ATTC_SET_REQUIRED | ATTC_SET_DESCRIPTOR
};

/*! List of characteristics to be discovered; order matches handle index enumeration  */
static const attcDiscChar_t *anccSvcDiscCharList[] =
{
    &anccNSDat,                    /*! Proprietary data */
    &anccNSdatCcc,                 /*! Proprietary data descriptor */
    &anccCtrlPoint,                /*! Control point */
    &anccDataSrc,                  /*! data source */
    &anccDataSrcCcc                /*! data source descriptor */
};

/* sanity check:  make sure handle list length matches characteristic list length */
WSF_CT_ASSERT(ANCC_HDL_LIST_LEN == ((sizeof(anccSvcDiscCharList) / sizeof(attcDiscChar_t *))));

/* Global variable */
anccCb_t    anccCb;
static uint32_t ancs_ry_timer;

static bool isAnccRySessionBlocked(void)
{
    dev_session_t currentSession = get_device_session();
    if((currentSession == DEV_SESSION_CARD)
        ||(currentSession == DEV_SESSION_OTA)
        ||(currentSession == DEV_SESSION_SURFACE_EDIT)
        ||(currentSession == DEV_SESSION_UPLOAD_DATA)
        ||(currentSession == DEV_SESSION_RUNNING)
        ) {
        LOG_DEBUG("[ancc] Session Blocked by session:%d\n", currentSession);
        return true;
    }else{
        return false;
    }
}

/*************************************************************************************************/
/*!
 *  \fn     AnccSvcDiscover
 *
 *  \brief  Perform service and characteristic discovery for ancs service.
 *          Parameter pHdlList must point to an array of length ANCC_HDL_LIST_LEN.
 *          If discovery is successful the handles of discovered characteristics and
 *          descriptors will be set in pHdlList.
 *
 *  \param  connId    Connection identifier.
 *  \param  pHdlList  Characteristic handle list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccSvcDiscover(dmConnId_t connId, uint16_t *pHdlList)
{
    AppDiscFindService(connId, ATT_128_UUID_LEN, (uint8_t *) anccAncsSvcUuid,
                     ANCC_HDL_LIST_LEN, (attcDiscChar_t **) anccSvcDiscCharList, pHdlList);
}

/*************************************************************************************************/
/*!
 *  \fn     AnccInit
 *
 *  \brief  Initialize the control variables of ancs client
 *
 *  \param  handlerId   App handler id for timer operation.
 *  \param  cfg         config variable.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccInit(wsfHandlerId_t handlerId, anccCfg_t* cfg)
{
    ry_memset(anccCb.anccList, 0, ANCC_LIST_ELEMENTS * sizeof(ancc_notif_t));
    anccCb.cfg.period = cfg->period;
    ry_timer_parm timer_para;
    //timer_para.timeout_CB = update_timeout_handle;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "ancc";
    anccCb.actionTimer.handlerId = handlerId;
    ancs_ry_timer = ry_timer_create(&timer_para);
}

/*************************************************************************************************/
/*!
 *  \fn     AnccConnOpen
 *
 *  \brief  Update the key parameters for control variables when connection open.
 *
 *  \param  connId      Connection identifier.
 *  \param  hdlList     Characteristic handle list.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccConnOpen(dmConnId_t connId, uint16_t* hdlList)
{
    anccCb.connId = connId;
    anccCb.hdlList = hdlList;
    anccCb.pendHandleRetryTimes = 0;
    anccCb.AttcWriteRespPendingHandle = 0;
}

/*************************************************************************************************/
/*!
 *  \fn     AnccConnClose
 *
 *  \brief  Clear the key parameters for control variables when connection close.
 *
 *  \param  None.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccConnClose(void)
{
    anccCb.connId = DM_CONN_ID_NONE;
    anccCb.pendHandleRetryTimes = 0;
    anccCb.AttcWriteRespPendingHandle = 0;
    ry_memset(anccCb.anccList, 0, ANCC_LIST_ELEMENTS * sizeof(ancc_notif_t));
}

/*************************************************************************************************/
/*!
 *  \fn     anccNoConnActive
 *
 *  \brief  Return TRUE if no connections with active measurements.
 *
 *  \return TRUE if no connections active.
 */
/*************************************************************************************************/
static bool_t anccNoConnActive(void)
{
    if (anccCb.connId != DM_CONN_ID_NONE)
    {
        return FALSE;
    }
    return TRUE;
}

/*************************************************************************************************/
/*!
 *  \fn     anccActionListPush
 *
 *  \brief  Return TRUE if element is added/updated to the list.
 *
 *  \return TRUE if element is added/updated to the list.
 *          FALSE if list is full.
 */
/*************************************************************************************************/
static bool_t anccActionListPush(ancc_notif_t* pElement)
{
    ancc_notif_t* p_notif = NULL;
    uint16_t index_matched_uid = 0xFFFF;
    uint16_t index_first_empty = 0xFFFF;
    uint32_t current_min_idle_uid = 0xFFFFFFFF;
    uint16_t current_min_idle_uid_index = 0xFFFF;
    uint32_t current_min_new_uid = 0xFFFFFFFF;
    uint16_t current_min_new_uid_index = 0xFFFF;

    for (int i = 0; i < ANCC_LIST_ELEMENTS; i++ ) {
        p_notif = &anccCb.anccList[i];
        uint8_t status = p_notif->noti_status;

        if(p_notif->notification_uid == pElement->notification_uid) {
            //找到相同的UID，替换和更新
            index_matched_uid = i;
            break;
        }

        if(p_notif->noti_status == ble_ancc_notif_usr_status_empty) {
            //记录最早的empty index
            if(index_first_empty == 0xFFFF) {
                index_first_empty = i;
            }
        }

        if((p_notif->noti_status == ble_ancc_notif_usr_status_idle)
//            ||(p_notif->noti_status == ble_ancc_notif_usr_status_pending_attr)
            ) {
            //记录早小的notify_uid,如果没有empty则移除比较早的notify
            if(p_notif->notification_uid < current_min_idle_uid) {
                current_min_idle_uid = p_notif->notification_uid;
                current_min_idle_uid_index = i;
            }
        }

        if(p_notif->noti_status == ble_ancc_notif_usr_status_pending_queue) {
            //记录早小的notify_uid,如果没有empty则移除比较早的notify
            if(p_notif->notification_uid < current_min_new_uid) {
                current_min_new_uid = p_notif->notification_uid;
                current_min_new_uid_index = i;
            }
        }
    }


    if(index_matched_uid < ANCC_LIST_ELEMENTS) {
        //填入更新
        LOG_DEBUG("[ANCS] ---- anccActionListPush ----Update, uid:%d\n", pElement->notification_uid);
        p_notif = &anccCb.anccList[index_matched_uid];
    } else if(index_first_empty < ANCC_LIST_ELEMENTS) {
        //填入empty
        LOG_DEBUG("[ANCS] ---- anccActionListPush ----insert, uid:%d\n", pElement->notification_uid);
        p_notif = &anccCb.anccList[index_first_empty];
    } else if(current_min_idle_uid_index < ANCC_LIST_ELEMENTS) {
        p_notif = &anccCb.anccList[current_min_idle_uid_index];
        //填入IDLE
        LOG_DEBUG("[ANCS] ---- anccActionListPush ---- uid:%d\t uid:%d removed\n",
            pElement->notification_uid,
            p_notif->notification_uid);
    } else if(current_min_new_uid_index < ANCC_LIST_ELEMENTS) {
        p_notif = &anccCb.anccList[current_min_new_uid_index];
        //cancel old ID
        LOG_DEBUG("[ANCS] ---- anccActionListPush ---- uid:%d\t uid:%d dropped\n",
            pElement->notification_uid,
            p_notif->notification_uid);
    } else {
        LOG_DEBUG("[ANCS] ---- anccActionListPush ----FULL, uid:%d\n", pElement->notification_uid);
        p_notif = NULL;
    }

    if(p_notif != NULL) {
        p_notif->notification_uid = pElement->notification_uid;
        p_notif->event_id = pElement->event_id;
        p_notif->event_flags = pElement->event_flags;
        p_notif->category_id = pElement->category_id;
        p_notif->category_count = pElement->category_count;
        p_notif->noti_status = pElement->noti_status;
        return true;
    } else {
        LOG_ERROR("[ANCS] notif Lost UID:%d\n", pElement->notification_uid);
        return false;   //no empty slot left
    }

}

/*************************************************************************************************/
/*!
 *  \fn     anccActionListPop
 *
 *  \brief  Return TRUE if element is popped out and removed from the list.
 *
 *  \return TRUE if element is popped out and removed from the list.
 *          FALSE if list is already empty.
 */
/*************************************************************************************************/
static ancc_notif_status_t anccActionListPopFirst(void)
{
    //LOG_DEBUG("---- anccActionListPop ----\n");
    uint32_t current_min_uid = 0xFFFFFFFF;
    uint16_t current_min_uid_index = 0xFFFF;

    for (uint16_t i = 0; i < ANCC_LIST_ELEMENTS; i++ ) {
        uint8_t status = anccCb.anccList[i].noti_status;
        if(( status == ble_ancc_notif_usr_status_empty)
            ||(status == ble_ancc_notif_usr_status_idle)
            ) {
            continue;
        }

        //有pending的user action优先处理
        if ((status == ble_ancc_notif_usr_status_pending_negative)
            ||(status == ble_ancc_notif_usr_status_pending_positive)
            ){
            current_min_uid = anccCb.anccList[i].notification_uid;
            current_min_uid_index = i;
            break;
        }

        //队列中使用优先处理最小的uid
        if ( status == ble_ancc_notif_usr_status_pending_queue ) {
            if(anccCb.anccList[i].notification_uid < current_min_uid) {
                current_min_uid = anccCb.anccList[i].notification_uid;
                current_min_uid_index = i;
            }
        }
    }

    if(current_min_uid_index == 0xFFFF) {
        return ble_ancc_notif_usr_status_empty;   //no element left in the list
    } else {
        // found a valid element in the list
        //anccCb.anccList[current_min_uid_index].noti_valid = false;
        anccCb.active.handle = current_min_uid_index;
        anccCb.active.attrId = BLE_ANCS_NOTIF_ATTR_RY_POPED;
        (*anccCb.attrCback)(&anccCb.active);
        return (ancc_notif_status_t)anccCb.anccList[current_min_uid_index].noti_status;
    }
}

#if ANCS_USE_RY_TIMER

void AnccRyTimerActionHandler(void)
{
    int len = sizeof(wsfMsgHdr_t);
    wsfMsgHdr_t* pMsg = (wsfMsgHdr_t*)WsfMsgAlloc(len);
    if (pMsg != NULL) {
        pMsg->event = ANCC_TIMER_EVT_DEFAULT;
        WsfMsgSend(anccCb.actionTimer.handlerId, pMsg);
        wakeup_ble_thread();
    }
}

#endif

/*************************************************************************************************/
/*!
 *  \fn     AnccActionStart
 *
 *  \brief  Start periodic ancc operation.  This function starts a timer to perform
 *          periodic actions.
 *
 *  \param  timerEvt    WSF event designated by the application for the timer.
 *
 *  \return None.
 */
/*************************************************************************************************/

void AnccActionStartWaitMore(uint8_t timerEvt, uint32_t timeout_ms)
{
    ry_sts_t ret = RY_SUCC;
    /* if this is first connection */
    if (anccNoConnActive() == FALSE) {
        /* initialize control block */
        anccCb.actionTimer.msg.event = timerEvt;
        /* (re-)start timer */
        ry_timer_stop(ancs_ry_timer);
        ret = ry_timer_start(ancs_ry_timer, timeout_ms, (ry_timer_cb_t)AnccRyTimerActionHandler, NULL);
        if(ret != RY_SUCC){
            LOG_ERROR("[ancc] start timer error:%d\n", ret);
        }
    } else {
        LOG_INFO("[ancc] err start timer when conn disconnected\n");
    }
}

void AnccActionStart(uint8_t timerEvt)
{
    if(!ry_timer_isRunning(ancs_ry_timer)) {
        AnccActionStartWaitMore(timerEvt, anccCb.cfg.period);
    }
}


/*************************************************************************************************/
/*!
 *  \fn     AnccActionStop
 *
 *  \brief  Stop periodic ancc action.
 *
 *  \param  connId      DM connection identifier.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccActionStop(void)
{
    /* stop timer */
    ry_timer_stop(ancs_ry_timer);
}

/*************************************************************************************************/
/*!
 *  \fn     AnccGetNotificationAttribute
 *
 *  \brief  Send a command to the apple notification center service control point.
 *
 *  \param  pHdlList  Characteristic handle list.
 *  \param  notiUid   NotificationUid.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccGetNotificationAttribute(uint16_t *pHdlList, uint32_t notiUid)
{
    LOG_DEBUG("---- AnccGetNotificationAttribute ----uid:%d\n", notiUid);
    // An example to get notification attributes
    uint16_t max_len = 256;
    uint8_t buf[19];   // retrieve the complete attribute list
    if (pHdlList[ANCC_CONTROL_POINT_HDL_IDX] != ATT_HANDLE_NONE)
    {
        buf[0] = BLE_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES;  // put command
        uint8_t * p = &buf[1];
        UINT32_TO_BSTREAM(p, notiUid);    // encode notification uid

        // encode attribute IDs
        buf[5] = BLE_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER;
        buf[6] = BLE_ANCS_NOTIF_ATTR_ID_TITLE;
        // 2 byte length
        buf[7] = max_len;
        buf[8] = max_len >> 8;
        buf[9] = BLE_ANCS_NOTIF_ATTR_ID_SUBTITLE;
        buf[10] = max_len;
        buf[11] = max_len >> 8;
        buf[12] = BLE_ANCS_NOTIF_ATTR_ID_MESSAGE;
        buf[13] = max_len;
        buf[14] = max_len >> 8;
        buf[15] = BLE_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE;
        buf[16] = BLE_ANCS_NOTIF_ATTR_ID_DATE;
        buf[17] = BLE_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL;
        buf[18] = BLE_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL;
        anccCb.AttcWriteRespPendingHandle = pHdlList[ANCC_CONTROL_POINT_HDL_IDX];
        AttcWriteReq(anccCb.connId, pHdlList[ANCC_CONTROL_POINT_HDL_IDX], sizeof(buf), buf);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     AncsGetAppAttribute
 *
 *  \brief  Send a command to the apple notification center service control point.
 *
 *  \param  pHdlList  Connection identifier.
 *  \param  pAppId    Attribute handle.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AncsGetAppAttribute(uint16_t *pHdlList, uint8_t *pAppId)
{
    // An example to get app attributes
    uint8_t buf[64];   // to hold the command, size of app identifier is unknown
    uint8_t count = 0;
    if (pHdlList[ANCC_CONTROL_POINT_HDL_IDX] != ATT_HANDLE_NONE)
    {
        buf[0] = BLE_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES;    // put command

        while (pAppId[count++] != 0);   // NULL terminated string
        if ( count > (64 - 2) )
        {
            // app identifier is too long
        }
        else
        {
            memcpy(&buf[1], pAppId, count);
        }

        buf[count + 1] = 0; // app attribute id
        anccCb.AttcWriteRespPendingHandle = pHdlList[ANCC_CONTROL_POINT_HDL_IDX];
        AttcWriteReq(anccCb.connId, pHdlList[ANCC_CONTROL_POINT_HDL_IDX], (count + 2), buf);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     AncsPerformNotiAction
 *
 *  \brief  Send a command to the apple notification center service control point.
 *
 *  \param  pHdlList  Characteristic handle list.
 *  \param  actionId  Notification action identifier.
 *  \param  notiUid   NotificationUid.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AncsPerformNotiAction(uint16_t *pHdlList, uint32_t notiUid, ancc_notif_action_id_values_t actionId)
{
    if( anccCb.AttcWriteRespPendingHandle != 0 )
    {   //put ancs operation in queue
        ancc_notif_t notify_op;
        notify_op.noti_status = ble_ancc_notif_usr_status_empty;

        for (int i = 0; i < ANCC_LIST_ELEMENTS; i++ )
        {
            if (anccCb.anccList[i].notification_uid == notiUid )
            {
                notify_op.category_count = anccCb.anccList[i].category_count;
                notify_op.category_id = anccCb.anccList[i].category_count;
                notify_op.event_flags = anccCb.anccList[i].category_count;
                notify_op.event_id = anccCb.anccList[i].category_count;
                notify_op.notification_uid = notiUid;
                break;
            }
        }

        if(notify_op.noti_status == ble_ancc_notif_usr_status_empty)
        {
            notify_op.notification_uid = notiUid;
            LOG_WARN("[ANCS] to operate UID:%d is not in list\n", notiUid);
        }

        if(actionId == BLE_ANCS_NOTIF_ACTION_ID_POSITIVE)
        {
           notify_op.noti_status = ble_ancc_notif_usr_status_pending_positive;
        }
        else if(actionId == BLE_ANCS_NOTIF_ACTION_ID_NEGATIVE)
        {
           notify_op.noti_status = ble_ancc_notif_usr_status_pending_negative;
        }

        anccActionListPush(&notify_op);

        AnccActionStart(ANCC_TIMER_EVT_DEFAULT);
        return;
    }

    //performs notification action
    uint8_t buf[6];   //to hold the command, size of app identifier is unknown
    if (pHdlList[ANCC_CONTROL_POINT_HDL_IDX] != ATT_HANDLE_NONE)
    {
        buf[0] = BLE_ANCS_COMMAND_ID_GET_PERFORM_NOTIF_ACTION;    // put command
        uint8_t * p = &buf[1];
        UINT32_TO_BSTREAM(p, notiUid);      // encode notification uid
        buf[5] = actionId;                  //action id
        anccCb.AttcWriteRespPendingHandle = pHdlList[ANCC_CONTROL_POINT_HDL_IDX];
        AttcWriteReq(anccCb.connId, pHdlList[ANCC_CONTROL_POINT_HDL_IDX], sizeof(buf), buf);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     AnccActionHandler
 *
 *  \brief  Routine to handle ancc related actions.
 *
 *  \param  actionTimerEvt  WsfTimer event indication.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccActionHandler(uint8_t actionTimerEvt)
{
    if(isAnccRySessionBlocked()) {
        AnccActionStartWaitMore(actionTimerEvt, 5*anccCb.cfg.period);
        LOG_DEBUG("[ancc] session blocked wait %d ms to continue\n", 5*anccCb.cfg.period);
        return;
    }

    // perform action
    if( anccCb.AttcWriteRespPendingHandle != 0) {   //GATTC write resp not ack gattc busy
        AnccActionStart(actionTimerEvt);
        anccCb.pendHandleRetryTimes++;
        if(anccCb.pendHandleRetryTimes > ANCC_HANDLE_PENDING_RETRY_MAX_TIMES){
            anccCb.pendHandleRetryTimes = 0;
            anccCb.AttcWriteRespPendingHandle= 0;
            LOG_ERROR("[ancc] err wait write resp\n");
        }
        return;
    } else {
        anccCb.pendHandleRetryTimes = 0;
    }
    ancc_notif_status_t ret_status = anccActionListPopFirst();
    ancc_notif_t* p_notif = &anccCb.anccList[anccCb.active.handle];

    if ( ret_status == ble_ancc_notif_usr_status_empty ) {
        //list empty
        AnccActionStop();
    } else if ( ret_status == ble_ancc_notif_usr_status_pending_queue) {
        if(p_notif->event_id == BLE_ANCS_EVENT_ID_NOTIFICATION_REMOVED) {   //notification removed, need no more attr;
            p_notif->noti_status = ble_ancc_notif_usr_status_empty;
            p_notif->notification_uid = 0xFFFF;
        } else {

#if TRACE_NOTIFY_ITEM
            static uint32_t ancs_pop_counter = 0;
            ancs_pop_counter++;
            if((p_notif->notification_uid % TRACE_NOTIFY_ITEM_SKIPS) == 0)
            {
                LOG_INFO("[ANCS] notify{%d} poped cnt:%d\n",
                        p_notif->notification_uid,
                        ancs_pop_counter
                        );
            }
#endif
            p_notif->noti_status = ble_ancc_notif_usr_status_idle;
            AnccGetNotificationAttribute(anccCb.hdlList, p_notif->notification_uid);
        }
        AnccActionStart(actionTimerEvt);
    } else if ( ret_status == ble_ancc_notif_usr_status_pending_positive) {
        p_notif->noti_status = ble_ancc_notif_usr_status_idle;
        AncsPerformNotiAction(anccCb.hdlList, 
            p_notif->notification_uid, 
            BLE_ANCS_NOTIF_ACTION_ID_POSITIVE);
        AnccActionStart(actionTimerEvt);
    } else if ( ret_status == ble_ancc_notif_usr_status_pending_negative) {
        p_notif->noti_status = ble_ancc_notif_usr_status_idle;
        AncsPerformNotiAction(anccCb.hdlList, 
            p_notif->notification_uid, 
            BLE_ANCS_NOTIF_ACTION_ID_NEGATIVE);
        AnccActionStart(actionTimerEvt);
    }
}

/*************************************************************************************************/
/*!
 *  \fn     anccAttrHandler
 *
 *  \brief  Static routine to handle attribute receiving and processing.
 *
 *  \param  pMsg      pointer to ATT message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static bool anccAttrHandler(attEvt_t * pMsg)
{
    uint8_t count = 0;
    uint16_t bytesRemaining = 0;

    switch(anccCb.active.attrState)
    {
        case NOTI_ATTR_NEW_NOTIFICATION:
            // new notification
            anccCb.active.commandId = pMsg->pValue[0];
            if ( anccCb.active.commandId == BLE_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES ) {
                BYTES_TO_UINT32(anccCb.active.notiUid, &(pMsg->pValue[1]));
                count = 4;
            } else if ( anccCb.active.commandId == BLE_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES ) {
                while(pMsg->pValue[count + 1] != 0) // NULL terminated string
                {
                    anccCb.active.appId[count] = pMsg->pValue[count + 1];
                    count++;
                }
                anccCb.active.appId[count + 1] = 0; // NULL terminated string
            } else {
                // BLE_ANCS_COMMAND_ID_GET_PERFORM_NOTIF_ACTION
                return false;
            }
            anccCb.active.parseIndex += count + 1;
            anccCb.active.attrState = NOTI_ATTR_NEW_ATTRIBUTE;
            anccCb.active.attrId = 0;
            anccCb.active.attrCount = 0;

            if ( pMsg->valueLen > ANCC_ATTRI_BUFFER_SIZE_BYTES ) {
                // notification size overflow
            } else {
                bytesRemaining = pMsg->valueLen;
            }
            // copy data
            memset(anccCb.active.attrDataBuf, 0, ANCC_ATTRI_BUFFER_SIZE_BYTES);
            anccCb.active.bufIndex = 0;
            for (uint16_t i = 0; i < bytesRemaining; i++) {
                anccCb.active.attrDataBuf[anccCb.active.bufIndex++] = pMsg->pValue[i];
            }
        // no break here by intention
        case NOTI_ATTR_NEW_ATTRIBUTE:
            // new attribute
            // check consistency of the attribute
            if ( anccCb.active.bufIndex - anccCb.active.parseIndex < 3 ) // 1 byte attribute id + 2 bytes attribute length
            {
                // attribute header not received completely
                anccCb.active.attrState = NOTI_ATTR_RECEIVING_ATTRIBUTE;
                return false;
            }

            anccCb.active.attrId = anccCb.active.attrDataBuf[anccCb.active.parseIndex];
            BYTES_TO_UINT16(anccCb.active.attrLength, &(anccCb.active.attrDataBuf[anccCb.active.parseIndex + 1]));

            if ( anccCb.active.attrLength > (anccCb.active.bufIndex - anccCb.active.parseIndex - 3) ) // 1 byte attribute id + 2 bytes attribute length
            {
                // attribute body not received completely
                anccCb.active.attrState = NOTI_ATTR_RECEIVING_ATTRIBUTE;
                return false;
            }

            // parse attribute
            anccCb.active.attrCount++;
            anccCb.active.parseIndex += 3; // 1 byte attribute id + 2 bytes attribute length

            if ( anccCb.active.attrId == BLE_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER )
            {
                uint8_t temp = 0;
                while(anccCb.active.attrDataBuf[anccCb.active.parseIndex + temp] != 0)  // NULL terminated string
                {
                    anccCb.active.appId[temp] = anccCb.active.attrDataBuf[anccCb.active.parseIndex + temp];
                    temp++;
                }
                anccCb.active.appId[temp] = 0;  // NULL terminated string
            }

            //
            // attribute received
            // execute callback function
            //
            (*anccCb.attrCback)(&anccCb.active);

            anccCb.active.parseIndex += anccCb.active.attrLength;

            if ( anccCb.active.attrCount >= RY_ANCC_MSG_COUNTER ) //custom criteria
            {
                //notification reception done
                anccCb.active.attrState = NOTI_ATTR_NEW_NOTIFICATION;
                anccCb.active.parseIndex = 0;
                anccCb.active.bufIndex = 0;
                //
                // notification received
                // execute callback function
                //
                (*anccCb.notiCback)(&anccCb.active, anccCb.active.notiUid);
                anccCb.anccList[anccCb.active.handle].noti_status = ble_ancc_notif_usr_status_empty;
                anccCb.anccList[anccCb.active.handle].notification_uid = 0;

                return false;
            }

            return true; // continue parsing
        // no need to break;
        case NOTI_ATTR_RECEIVING_ATTRIBUTE:
            // notification continuing
            bytesRemaining = 0;
            if ( anccCb.active.bufIndex + pMsg->valueLen > ANCC_ATTRI_BUFFER_SIZE_BYTES )
            {
                // notification size overflow
                anccCb.active.attrState = NOTI_ATTR_NEW_NOTIFICATION;
                anccCb.active.parseIndex = 0;
                anccCb.active.bufIndex = 0;
                LOG_DEBUG("[ANCS] notification size overflow\n");
                return false;
            } else {
                bytesRemaining = pMsg->valueLen;
            }
            // copy data
            for (uint16_t i = 0; i < bytesRemaining; i++) {
                anccCb.active.attrDataBuf[anccCb.active.bufIndex++] = pMsg->pValue[i];
            }
            anccCb.active.attrState = NOTI_ATTR_NEW_ATTRIBUTE;
            return true;
        // no need to break;
    }
    return false;
}
/*************************************************************************************************/
/*!
 *  \fn     AnccNtfValueUpdate
 *
 *  \brief  Routine to handle any ancc related notify.
 *
 *  \param  pHdlList  Characteristic handle list.
 *  \param  actionTimerEvt  WsfTimer event indication.
 *  \param  pMsg      pointer to ATT message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccNtfValueUpdate(uint16_t *pHdlList, attEvt_t * pMsg, uint8_t actionTimerEvt)
{
    const uint8_t EventFlagPreExisting = (1<<2);
    ancc_notif_t ancs_notif;
    uint8_t *p;

    //notification received
    if (pMsg->handle == pHdlList[ANCC_NOTIFICATION_SOURCE_HDL_IDX])
    {
        // process notificiation source (brief)
        p = pMsg->pValue;
        ancs_notif.event_id = p[0];
        ancs_notif.event_flags = p[1];
        ancs_notif.category_id = p[2];
        ancs_notif.category_count = p[3];
        ancs_notif.noti_status = ble_ancc_notif_usr_status_pending_queue;
        BYTES_TO_UINT32(ancs_notif.notification_uid, &p[4]);

#if TRACE_NOTIFY_ITEM
        static uint32_t ancs_notif_counter = 0;
        ancs_notif_counter++;
        if((ancs_notif.notification_uid % TRACE_NOTIFY_ITEM_SKIPS) == 0)
        {
            LOG_INFO("[ANCS] notify{%d} point cnt:%d evt_id:%d flag:0x%02X cat_id:%d cat_count:%d\n",
                    ancs_notif.notification_uid,
                    ancs_notif_counter,
                    ancs_notif.event_id,
                    ancs_notif.event_flags,
                    ancs_notif.category_id,
                    ancs_notif.category_count
                    );
        }
#endif

        if((ancs_notif.event_id == BLE_ANCS_EVENT_ID_NOTIFICATION_ADDED)
            &&((ancs_notif.event_flags & EventFlagPreExisting) != 0))
        {   //skip pre_existing new message
            LOG_DEBUG("[ANCS] Skip pre_existing message UID:%d\n", ancs_notif.notification_uid);
            return;
        }


#if 1//RY_BUILD == RY_DEBUG
        if((ancs_notif.event_id == BLE_ANCS_EVENT_ID_NOTIFICATION_REMOVED)
            &&(ancs_notif.category_id != BLE_ANCS_CATEGORY_ID_INCOMING_CALL))
        {   //skip removing message except for incoming
            LOG_DEBUG("[ANCS] Skip removing message UID:%d\n", ancs_notif.notification_uid);
            return;
        }
#endif

        if ( !anccActionListPush(&ancs_notif) )
        {
            // list full
            LOG_DEBUG("[ANCS] action list full...\n");
        }
        else
        {
            //
            // actions to be done with timer delays to avoid generating heavy traffic
            //
            AnccActionStart(actionTimerEvt);
//            LOG_INFO("added to action list UID:%d\n\tflag:0x%02X,\n\tevt_type:%d,\n\tcat_id:%d,\n\tcat_count:%d\n", 
//                    ancs_notif.notification_uid,
//                    ancs_notif.event_flags,
//                    ancs_notif.event_id,
//                    ancs_notif.category_id,
//                    ancs_notif.category_count);
        }
    }
    else if ( pMsg->handle == pHdlList[ANCC_DATA_SOURCE_HDL_IDX] )
    {
        // process notificiation/app attributes
        while(anccAttrHandler(pMsg));
    }
}

/*************************************************************************************************/
/*!
 *  \fn     AnccCbackRegister
 *
 *  \brief  Register the attribute received callback and notification completed callback.
 *
 *  \param  attrCback  Pointer to attribute received callback function.
 *  \param  notiCback  Pointer to notification completed callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AnccCbackRegister(anccAttrRecvCback_t attrCback, anccNotiCmplCback_t notiCback)
{
    anccCb.attrCback = attrCback;
    anccCb.notiCback = notiCback;
}

void AnccOnGattcWriteResp(uint16_t *pHdlList, attEvt_t *pMsg)
{
    if((anccCb.AttcWriteRespPendingHandle != 0)
        &&(anccCb.AttcWriteRespPendingHandle == pMsg->handle)
        ) {
        anccCb.AttcWriteRespPendingHandle = 0;
        AnccActionStop();   //加速ANCC notificaiton 获取
        AnccActionHandler(ANCC_TIMER_EVT_DEFAULT);
    }
}


