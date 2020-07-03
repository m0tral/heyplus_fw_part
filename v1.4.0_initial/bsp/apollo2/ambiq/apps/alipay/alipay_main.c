// ****************************************************************************
//
//  alipay_main.c
//! @file
//!
//! @brief Ambiq Micro's demonstration of alipay service.
//!
//! @{
//
// ****************************************************************************

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
// This is part of revision v1.2.11-734-ga9d9a3d-stable of the AmbiqSuite
// Development Package.
//
//*****************************************************************************
#include <string.h>
#include "wsf_types.h"
#include "wsf_msg.h"
#include "att_api.h"
#include "app_api.h"
#include "app_db.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "alipay.h"
#include "alipay_api.h"
#include "alipay_vendor.h"
#include "svc_alipay.h"
#include "am_util.h"
#include "ry_utils.h"
#include "ble_task.h"
#include "ry_ble.h"
#include "ryos.h"
#include "ry_hal_inc.h"

extern void RyeexAlipayHookChangeMacAddr(uint8_t* p_temp_addr);
extern void RyeexAdvPayloadUpdate(void);


/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*! Application message type */
typedef union {
    wsfMsgHdr_t hdr;
    dmEvt_t dm;
    attsCccEvt_t ccc;
    attEvt_t att;
} alipayMsg_t;


static const AlipayCfg_t alipayCfg =
{
    .reserved = 0,
};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/
/*! WSF handler ID */
static wsfHandlerId_t alipayHandlerId;


/*************************************************************************************************/
/*!
 *  \fn     alipayProcCccState
 *
 *  \brief  Process CCC state change.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void alipayProcCccState(alipayMsg_t *pMsg)
{
//  LOG_DEBUG("ccc state ind value:%d handle:%d idx:%d\n", pMsg->ccc.value, pMsg->ccc.handle, pMsg->ccc.idx);

  /* ALIPAYS TX CCC */
  if (pMsg->ccc.idx == FIT_ALIPAY_CCC_IDX)
  {
    if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      // notify enabled
      LOG_DEBUG("[%s]---alipay_start\n");
      alipay_start((dmConnId_t)pMsg->ccc.hdr.param, ALIPAY_TIMER_IND);
    }
    else
    {
      // notify disabled
      alipay_stop((dmConnId_t)pMsg->ccc.hdr.param);
    }
    return;
  }
}

/*************************************************************************************************/
/*!
 *  \fn     alipayClose
 *
 *  \brief  Perform UI actions on connection close.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void alipayClose(alipayMsg_t *pMsg)
{
  ;
}

/*************************************************************************************************/
/*!
 *  \fn     alipay service advertisement close
 *
 *  \brief  hide alipay service from the ble advertisement
 *
 *  \return None.
 */
/*************************************************************************************************/
void Alipay_service_adv_close(void)
{
#if 0
    AppAdvStop();
    wakeup_ble_thread();
    ryos_delay_ms(10);
    if(ry_ble_get_state() != RY_BLE_STATE_CONNECTED)
    {
        uint8_t para = RY_BLE_ADV_TYPE_NORMAL;
        wakeup_ble_thread();    //don't know why but help to stop adv
        ryos_delay_ms(100);
        ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
    }
    else
    {
        LOG_DEBUG("[alipay] wait finished adv should already stopped --> update normal\n");
        RyeexAdvPayloadUpdate();
    }
    wakeup_ble_thread();
#endif
}

/*************************************************************************************************/
/*!
 *  \fn     alipayProcMsg
 *
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void alipayProcMsg(void* p_msg)
{
  uint32_t irq = 0;
  uint8_t macAddrTemp[6];
  alipayMsg_t *pMsg = (alipayMsg_t*)p_msg;
//  LOG_DEBUG("[%s]---- %d\n", __FUNCTION__, (pMsg->hdr.event));

  switch (pMsg->hdr.event)
  {
  case ALIPAY_TIMER_IND:
    alipay_proc_msg(&pMsg->hdr);
    break;

  case ALIPAY_SERVICE_ADD:
    LOG_WARN("[alipay]--- add service start \n");
    alipay_register_service(alipay_recv_data_handle);
    extern uint8_t g_BLEMacAddress[6];
    macAddrTemp[0] = g_BLEMacAddress[0];
    macAddrTemp[1] = g_BLEMacAddress[1];
    //g_BLEMacAddress[2] = ~g_BLEMacAddress[2];   //bit revert hook makesure different
    macAddrTemp[2] = ~g_BLEMacAddress[2];
    macAddrTemp[3] = g_BLEMacAddress[3];
    macAddrTemp[4] = g_BLEMacAddress[4];    //0xF6
    macAddrTemp[5] = g_BLEMacAddress[5];    //0x9C

    irq = ry_hal_irq_disable();
    RyeexAlipayHookChangeMacAddr(macAddrTemp);
    ry_hal_irq_enable();

    
    ryos_delay_ms(10);
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_DISCONNECT, 0, NULL);
    ryos_delay_ms(10);
    LOG_INFO("[alipay]--- add service finished \n");
    break;

  case ALIPAY_SERVICE_REMOVE:
    LOG_INFO("[alipay]--- remove service start \n");
    alipay_unregister_service();
    ble_hci_reset();
    LOG_WARN("[alipay]--- remove service finished \n");
    break;

  case ATTS_HANDLE_VALUE_CNF:
    alipay_proc_msg(&pMsg->hdr);
    break;

  case ATTS_CCC_STATE_IND:
    alipayProcCccState(pMsg);
    break;

  case ATT_MTU_UPDATE_IND:
    break;

  case DM_RESET_CMPL_IND:
    break;

  case DM_ADV_START_IND:
    break;

  case DM_ADV_STOP_IND:
    break;

  case DM_CONN_OPEN_IND:
    alipay_proc_msg(&pMsg->hdr);
    break;

  case DM_CONN_CLOSE_IND:
    alipayClose(pMsg);
    alipay_proc_msg(&pMsg->hdr);
    break;

  case DM_CONN_UPDATE_IND:
    alipay_proc_msg(&pMsg->hdr);
    break;

  case DM_SEC_AUTH_REQ_IND:
    break;

  case DM_SEC_ECC_KEY_IND:
    break;

  case DM_SEC_COMPARE_IND:
    break;

  default:
    break;
  }
}

/*************************************************************************************************/
/*!
 *  \fn     AlipayStart
 *
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/

void AlipaySendEventThroughBleThread(uint8_t evt_type)
{
    int len = sizeof(alipayMsg_t);
    alipayMsg_t* pMsg = (void*)WsfMsgAlloc(len);
    if (pMsg != NULL) {
        pMsg->hdr.event = evt_type;
        WsfMsgSend(alipayHandlerId, pMsg);
        wakeup_ble_thread();
        ryos_delay_ms(100);
    }
}

void AlipayHandleInit(uint16_t handle)
{
    alipayHandlerId = handle;

    /* initialize alipay service server */
    alipay_init(handle, (AlipayCfg_t *)&alipayCfg);
}



