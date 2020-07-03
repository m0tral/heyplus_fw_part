/*************************************************************************************************/
/*!
 *  \file   fit_main.c
 *
 *  \brief  Fitness sample application for the following profiles:
 *            Heart Rate profile
 *
 *          $Date: 2016-12-28 16:12:14 -0600 (Wed, 28 Dec 2016) $
 *          $Revision: 10805 $
 *
 *  Copyright (c) 2011-2017 ARM Ltd., all rights reserved.
 *  ARM Ltd. confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 */
/*************************************************************************************************/

#include <string.h>
#include "wsf_types.h"
#include "bstream.h"
#include "wsf_msg.h"
#include "wsf_trace.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "app_db.h"
#include "app_ui.h"
#include "app_hw.h"
#include "svc_ch.h"
#include "svc_core.h"
#include "svc_hrs.h"
#include "svc_dis.h"
#include "svc_batt.h"
#include "svc_rscs.h"
#include "svc_alipay.h"

#include "bas_api.h"
#include "hrps_api.h"
#include "rscp_api.h"
#include "fit_api.h"

#include "svc_ryeex.h"
#include "svc_mi.h"
#include "svc_wechat.h"
#include "mible.h"
#include "ry_utils.h"

#include "device_management_service.h"

#include "app_config.h"

#include "ble_task.h"
#include "app_cfg.h"
#include "ancs_api.h"
#include "wechat_api.h"
#include "alipay_api.h"
#include "wsf_assert.h"

#include "Alipay_vendor.h"
#include "r_xfer.h"
void ry_ble_proc_msg(void* pStackMsg);
void RyeexWaitConnectAdvStart(uint8_t mode);
void RyeexConnectableAdvStart(uint8_t mode);
void RyeexUnbindAdvStart(uint8_t mode);
void RyeexUnconnectableAdvStart(uint8_t mode);

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! WSF message event starting value */

/* Default Running Speed and Cadence Measurement period (seconds) */
#define FIT_DEFAULT_RSCM_PERIOD        1

/*! WSF message event enumeration */
enum
{
  FIT_HR_TIMER_IND = FIT_MSG_START,       /*! Heart rate measurement timer expired */
  FIT_BATT_TIMER_IND,                     /*! Battery measurement timer expired */
  FIT_RUNNING_TIMER_IND                   /*! Running speed and cadence measurement timer expired */
};

extern uint8_t g_BLEMacAddress[];

/*! Application message type */
typedef union
{
  wsfMsgHdr_t     hdr;
  dmEvt_t         dm;
  attsCccEvt_t    ccc;
  attEvt_t        att;
} fitMsg_t;


/**************************************************************************************************
  Configurable Parameters
**************************************************************************************************/

/*! configurable parameters for advertising */
static appAdvCfg_t fitAdvCfg =
{
  {    0,     0,     0,    0},   //{60000,     0,     0},                  /*! Advertising durations in ms */
  {  338,     1636,     160,   4800},                   /*! Advertising intervals in 0.625 ms units */
  //{  2056,     1364,     160},                 /*! Advertising intervals in 0.625 ms units */
};

/*! configurable parameters for slave */
static const appSlaveCfg_t fitSlaveCfg =
{
  FIT_CONN_MAX,                           /*! Maximum connections */
};

/*! configurable parameters for security */
static const appSecCfg_t fitSecCfg =
{
  DM_AUTH_BOND_FLAG | DM_AUTH_MITM_FLAG,    /*! Authentication and bonding flags */
  0,                                      /*! Initiator key distribution flags */
  DM_KEY_DIST_LTK,                        /*! Responder key distribution flags */
  FALSE,                                  /*! TRUE if Out-of-band pairing data is present */
  FALSE,                                  /*! TRUE to initiate security upon connection */
};

/*! configurable parameters for connection parameter update */
static const appUpdateCfg_t fitUpdateCfg =
{
  6000,                                   /*! Connection idle period in ms before attempting
                                              connection parameter update; set to zero to disable */
  40,//640,                                    /*! Minimum connection interval in 1.25ms units */
  50,//800,                                    /*! Maximum connection interval in 1.25ms units */
  0,                                      /*! Connection latency */
  600,                                    /*! Supervision timeout in 10ms units */
  5                                       /*! Number of update attempts before giving up */
};

extern appDiscCfg_t ancsDiscCfg;    //in ancs_main.c

/*! heart rate measurement configuration */
static const hrpsCfg_t fitHrpsCfg =
{
  2000      /*! Measurement timer expiration period in ms */
};

/*! battery measurement configuration */
static const basCfg_t fitBasCfg =
{
  30,       /*! Battery measurement timer expiration period in seconds */
  1,        /*! Perform battery measurement after this many timer periods */
  100       /*! Send battery level notification to peer when below this level. */
};

/*! SMP security parameter configuration */
static const smpCfg_t fitSmpCfg =
{
  3000,                                   /*! 'Repeated attempts' timeout in msec */
  SMP_IO_DISP_ONLY | SMP_IO_NO_IN_NO_OUT,                    /*! I/O Capability */
  7,                                      /*! Minimum encryption key length */
  16,                                     /*! Maximum encryption key length */
  3,                                      /*! Attempts to trigger 'repeated attempts' timeout */
  0,                                      /*! Device authentication requirements */
};

/**************************************************************************************************
  Advertising Data
**************************************************************************************************/

/*! advertising data, discoverable mode */
uint8_t fitAdvDataDisc[] =
{
  /*! flags */
  2,                                      /*! length */
  DM_ADV_TYPE_FLAGS,                      /*! AD type */
  DM_FLAG_LE_GENERAL_DISC |               /*! flags */
  DM_FLAG_LE_BREDR_NOT_SUP,

  /*! service UUID list */
  7,                                      /*! length */
  DM_ADV_TYPE_16_UUID,                    /*! AD type */
  UINT16_TO_BYTES(ATT_UUID_MI_SERVICE),
  UINT16_TO_BYTES(ATT_UUID_RYEEX_SERVICE),
  UINT16_TO_BYTES(ATT_UUID_WECHAT_SERVICE),
  15,
  DM_ADV_TYPE_SERVICE_DATA,
  0x95, 0xFE,                             /*! Service UUID */  
  0x31, 0x20, 0x8F, 0x03, 0x00,           /*! Mi Beacon */ 
  0x0C, 0xF3, 0xEE, 0x52, 0xC1, 0x3F,     /*! Mi Beacon MAC */ 
  //0x0C, 0xF3, 0xEE, 0x52, 0x61, 0x47,     /*! Mi Beacon MAC */ 
  0x09                                    /*! Mi Beacon */ 
};

#define ADV_TOTAL_LEN       (sizeof(fitAdvDataDisc))
#define ADV_LAST_MAC_ADDR   ((ADV_TOTAL_LEN) - 2)

/*! scan data, discoverable mode */
uint8_t fitScanDataDisc[] =
{
  /*! device name */
  5,                                      /*! length */
  DM_ADV_TYPE_LOCAL_NAME,                 /*! AD type */
  'h',
  'e',
  'y',
  '+',

  9,                                             /*! length */
  DM_ADV_TYPE_SERVICE_DATA,                      /*! AD type */
  UINT16_TO_BYTES(ATT_UUID_ALIPAY_SERVICE_PART), /*! Service data - 16-bit UUID */
  //0x9C, 0xF6, 0xDD, 0x30, 0x01, 0xBD,            /*! mac address */ 
  0x00,0x00,0x00,0x00,0x00,0x00,

  11,
  DM_ADV_TYPE_MANUFACTURER,
  0x49, 0x06,
  0x00, 0x00,     //default 0x00 - debug wechat version
  0x9C, 0xF6, 0xDD, 0x30, 0x01, 0xBD
};

#define ADV_RESP_LEN                (sizeof(fitScanDataDisc))
#define ADV_RESP_WECHAT_MAC_ADDR    ((ADV_RESP_LEN) - 1)
#define ADV_RESP_ALIPAY_MAC_ADDR    ((ADV_RESP_LEN) - 1 - 12)

/**************************************************************************************************
  Client Characteristic Configuration Descriptors
**************************************************************************************************/


/*! client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t fitCccSet[FIT_NUM_CCC_IDX] =
{
  /* cccd handle          value range               security level */
  {GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE},          /* FIT_GATT_SC_CCC_IDX */
  {HRS_HRM_CH_CCC_HDL,    ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},          /* FIT_HRS_HRM_CCC_IDX */
  {BATT_LVL_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},          /* FIT_BATT_LVL_CCC_IDX */
  {RSCS_RSM_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},          /* FIT_RSCS_SM_CCC_IDX */
  {RYEEXS_SECURE_CH_CCC_HDL,   ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},     /* FIT_RYEEXS_SEC_CH_CCC_IDX */
  {RYEEXS_UNSECURE_CH_CCC_HDL, ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},     /* FIT_RYEEXS_UNSEC_CH_CCC_IDX */
  {RYEEXS_JSON_CH_CCC_HDL,     ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},     /* RYEEXS_JSON_CH_CCC_HDL */
  {MIS_TOKEN_CCC_HDL,          ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},     /* FIT_MIS_TOKEN_CCC_IDX */
  {MIS_AUTH_CCC_HDL,           ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},     /* FIT_MIS_AUTH_CCC_IDX */
  {WECHATS_SVC_CH_PEDO_MEASURE_CCC_HDL,  ATT_CLIENT_CFG_NOTIFY,    DM_SEC_LEVEL_NONE},      /* FIT_WECHAT_CCC_IDX */
  {ALIPAY_DATA_CH_CCC_HDL, ATT_CLIENT_CFG_NOTIFY, DM_SEC_LEVEL_NONE}

};

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! WSF handler ID */
wsfHandlerId_t fitHandlerId;

/* WSF Timer to send running speed and cadence measurement data */
wsfTimer_t     fitRscmTimer;

/* Running Speed and Cadence Measurement period - Can be changed at runtime to vary period */
static uint16_t fitRscmPeriod = FIT_DEFAULT_RSCM_PERIOD;


static const AlipayCfg_t alipayCfg =
{
    .reserved = 0,
};


/*************************************************************************************************/
/*!
 *  \fn     fitDmCback
 *
 *  \brief  Application DM callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitDmCback(dmEvt_t *pDmEvt)
{
  dmEvt_t *pMsg;
  uint16_t len;

  len = DmSizeOfEvt(pDmEvt);

  if ((pMsg = WsfMsgAlloc(len)) != NULL)
  {
    memcpy(pMsg, pDmEvt, len);
    WsfMsgSend(fitHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     fitAttCback
 *
 *  \brief  Application ATT callback.
 *
 *  \param  pEvt    ATT callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitAttCback(attEvt_t *pEvt)
{
  attEvt_t *pMsg;

  if ((pMsg = WsfMsgAlloc(sizeof(attEvt_t) + pEvt->valueLen)) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attEvt_t));
    pMsg->pValue = (uint8_t *) (pMsg + 1);
    memcpy(pMsg->pValue, pEvt->pValue, pEvt->valueLen);
    WsfMsgSend(fitHandlerId, pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     fitCccCback
 *
 *  \brief  Application ATTS client characteristic configuration callback.
 *
 *  \param  pDmEvt  DM callback event
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitCccCback(attsCccEvt_t *pEvt)
{
  attsCccEvt_t  *pMsg;
  appDbHdl_t    dbHdl;

  LOG_DEBUG("[fitCccCback] index:%d, value:%d\n", pEvt->idx, pEvt->value);

  /* if CCC not set from initialization and there's a device record */
  if ((pEvt->handle != ATT_HANDLE_NONE) &&
      ((dbHdl = AppDbGetHdl((dmConnId_t) pEvt->hdr.param)) != APP_DB_HDL_NONE))
  {
    /* store value in device database */
    AppDbSetCccTblValue(dbHdl, pEvt->idx, pEvt->value);
  }

  if ((pMsg = WsfMsgAlloc(sizeof(attsCccEvt_t))) != NULL)
  {
    memcpy(pMsg, pEvt, sizeof(attsCccEvt_t));
    WsfMsgSend(fitHandlerId, pMsg);
  }
}


/*************************************************************************************************/
/*!
*  \fn     fitSendRunningSpeedMeasurement
*
*  \brief  Send a Running Speed and Cadence Measurement Notification.
*
*  \param  connId    connection ID
*
*  \return None.
*/
/*************************************************************************************************/
static void fitSendRunningSpeedMeasurement(dmConnId_t connId)
{
  if (AttsCccEnabled(connId, FIT_RSCS_SM_CCC_IDX))
  {
    static uint8_t walk_run = 1;

    /* TODO: Set Running Speed and Cadence Measurement Parameters */

    RscpsSetParameter(RSCP_SM_PARAM_SPEED, 1);
    RscpsSetParameter(RSCP_SM_PARAM_CADENCE, 2);
    RscpsSetParameter(RSCP_SM_PARAM_STRIDE_LENGTH, 3);
    RscpsSetParameter(RSCP_SM_PARAM_TOTAL_DISTANCE, 4);
    
    /* Toggle running/walking */
    walk_run = walk_run? 0 : 1;
    RscpsSetParameter(RSCP_SM_PARAM_STATUS, walk_run);

    RscpsSendSpeedMeasurement(connId);
  }

  /* Configure and start timer to send the next measurement */
  fitRscmTimer.msg.event = FIT_RUNNING_TIMER_IND;
  fitRscmTimer.msg.status = FIT_RSCS_SM_CCC_IDX;
  fitRscmTimer.handlerId = fitHandlerId;
  fitRscmTimer.msg.param = connId;

  WsfTimerStartSec(&fitRscmTimer, fitRscmPeriod);
}

/*************************************************************************************************/
/*!
 *  \fn     fitProcCccState
 *
 *  \brief  Process CCC state change.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitProcCccState(fitMsg_t *pMsg)
{
  APP_TRACE_INFO3("ccc state ind value:%d handle:%d idx:%d\n", pMsg->ccc.value, pMsg->ccc.handle, pMsg->ccc.idx);

  /* handle heart rate measurement CCC */
  if (pMsg->ccc.idx == FIT_HRS_HRM_CCC_IDX)
  {
    if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      HrpsMeasStart((dmConnId_t) pMsg->ccc.hdr.param, FIT_HR_TIMER_IND, FIT_HRS_HRM_CCC_IDX);
    }
    else
    {
      HrpsMeasStop((dmConnId_t) pMsg->ccc.hdr.param);
    }
    return;
  }

  /* handle running speed and cadence measurement CCC */
  if (pMsg->ccc.idx == FIT_RSCS_SM_CCC_IDX)
  {
    if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      fitSendRunningSpeedMeasurement((dmConnId_t)pMsg->ccc.hdr.param);
    }
    else
    {
      WsfTimerStop(&fitRscmTimer);
    }
    return;
  }

  /* handle battery level CCC */
  if (pMsg->ccc.idx == FIT_BATT_LVL_CCC_IDX)
  {
    if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      BasMeasBattStart((dmConnId_t) pMsg->ccc.hdr.param, FIT_BATT_TIMER_IND, FIT_BATT_LVL_CCC_IDX);
    }
    else
    {
      BasMeasBattStop((dmConnId_t) pMsg->ccc.hdr.param);
    }
    return;
  }

  // Handle MI Service CCC
  if (pMsg->ccc.idx == FIT_MIS_TOKEN_CCC_IDX)
  {
    if (pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY)
    {
      //BasMeasBattStart((dmConnId_t) pMsg->ccc.hdr.param, FIT_BATT_TIMER_IND, FIT_BATT_LVL_CCC_IDX);
    }
    else
    {
      //BasMeasBattStop((dmConnId_t) pMsg->ccc.hdr.param);
    }
    return;
  }
}

/*************************************************************************************************/
/*!
 *  \fn     fitClose
 *
 *  \brief  Perform UI actions on connection close.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitClose(fitMsg_t *pMsg)
{
  /* stop heart rate measurement */
  HrpsMeasStop((dmConnId_t) pMsg->hdr.param);

  /* stop battery measurement */
  BasMeasBattStop((dmConnId_t) pMsg->hdr.param);

  /* Stop running speed and cadence timer */
  WsfTimerStop(&fitRscmTimer);
}

/*************************************************************************************************/
/*!
 *  \fn     fitSetup
 *
 *  \brief  Set up advertising and other procedures that need to be performed after
 *          device reset.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void RyAdvUpdateMacAddrPayload(void)
{
    fitAdvDataDisc[ADV_LAST_MAC_ADDR - 5] = g_BLEMacAddress[0];
    fitAdvDataDisc[ADV_LAST_MAC_ADDR - 4] = g_BLEMacAddress[1];
    fitAdvDataDisc[ADV_LAST_MAC_ADDR - 3] = g_BLEMacAddress[2];
    fitAdvDataDisc[ADV_LAST_MAC_ADDR - 2] = g_BLEMacAddress[3];
    fitAdvDataDisc[ADV_LAST_MAC_ADDR - 1] = g_BLEMacAddress[4];
    fitAdvDataDisc[ADV_LAST_MAC_ADDR - 0] = g_BLEMacAddress[5];

    fitScanDataDisc[ADV_RESP_WECHAT_MAC_ADDR - 0] = g_BLEMacAddress[0];
    fitScanDataDisc[ADV_RESP_WECHAT_MAC_ADDR - 1] = g_BLEMacAddress[1];
    fitScanDataDisc[ADV_RESP_WECHAT_MAC_ADDR - 2] = g_BLEMacAddress[2];
    fitScanDataDisc[ADV_RESP_WECHAT_MAC_ADDR - 3] = g_BLEMacAddress[3];
    fitScanDataDisc[ADV_RESP_WECHAT_MAC_ADDR - 4] = g_BLEMacAddress[4];
    fitScanDataDisc[ADV_RESP_WECHAT_MAC_ADDR - 5] = g_BLEMacAddress[5];

    fitScanDataDisc[ADV_RESP_ALIPAY_MAC_ADDR - 0] = g_BLEMacAddress[0];
    fitScanDataDisc[ADV_RESP_ALIPAY_MAC_ADDR - 1] = g_BLEMacAddress[1];
    fitScanDataDisc[ADV_RESP_ALIPAY_MAC_ADDR - 2] = ~g_BLEMacAddress[2];
    fitScanDataDisc[ADV_RESP_ALIPAY_MAC_ADDR - 3] = g_BLEMacAddress[3];
    fitScanDataDisc[ADV_RESP_ALIPAY_MAC_ADDR - 4] = g_BLEMacAddress[4];
    fitScanDataDisc[ADV_RESP_ALIPAY_MAC_ADDR - 5] = g_BLEMacAddress[5];
}

void RyeexAdvNormalSend(void)
{
    /* set advertising and scan response data for discoverable mode */
    RyAdvUpdateMacAddrPayload();

    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(fitAdvDataDisc), (uint8_t *) fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(fitScanDataDisc), (uint8_t *) fitScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);

    /* start advertising; automatically set connectable/discoverable mode and bondable mode */
    
//    if (is_deepsleep_active()) {
//        AppSetBondable(TRUE);
//        AppAdvStart(APP_MODE_DISCOVERABLE);
//    }
//    else {    
        AppAdvStart(APP_MODE_AUTO_INIT);
//    }
}

void RyeexAdvPayloadUpdate(void)
{
    /* set advertising and scan response data for discoverable mode */
    RyAdvUpdateMacAddrPayload();

    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(fitAdvDataDisc), (uint8_t *) fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(fitScanDataDisc), (uint8_t *) fitScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);
}

void RyeexAdvWaitConnectSend(void)
{
    /* set advertising and scan response data for discoverable mode */
    RyAdvUpdateMacAddrPayload();

    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(fitAdvDataDisc), (uint8_t *) fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(fitScanDataDisc), (uint8_t *) fitScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);

    /* start advertising; automatically set connectable/discoverable mode and bondable mode */
    RyeexWaitConnectAdvStart(APP_MODE_AUTO_INIT);
}

void RyeexAdvSlowSend(void)
{
     /* set advertising and scan response data for discoverable mode */
    RyAdvUpdateMacAddrPayload();
     
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(fitAdvDataDisc), (uint8_t *) fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(fitScanDataDisc), (uint8_t *) fitScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);

    RyeexUnbindAdvStart(APP_MODE_AUTO_INIT);
}

void ryeex_Alipay_service_adv_open(void)
{
    /* set advertising and scan response data for discoverable mode */
    LOG_DEBUG("[%s]-------open\n",__FUNCTION__);
    
    RyAdvUpdateMacAddrPayload();
    AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, sizeof(fitAdvDataDisc), (uint8_t *) fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(fitScanDataDisc), (uint8_t *) fitScanDataDisc);

    /* set advertising and scan response data for connectable mode */
    AppAdvSetData(APP_ADV_DATA_CONNECTABLE, sizeof(fitAdvDataDisc), (uint8_t *) fitAdvDataDisc);
    AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(fitScanDataDisc), (uint8_t *) fitScanDataDisc);
    RyeexWaitConnectAdvStart(APP_MODE_AUTO_INIT);

    /* start advertising; automatically set connectable/discoverable mode and bondable mode */
    
}

void ryeex_Alipay_service_adv_close(void)
{
  #if 0
  AppAdvStop();
  
  /* set advertising and scan response data for discoverable mode */
  AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, 7, (uint8_t *)alipayAdvDataDisc);
  AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, sizeof(alipayScanDataDisc), (uint8_t *)alipayScanDataDisc);

  /* set advertising and scan response data for connectable mode */
  AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 7, (uint8_t *)alipayAdvDataDisc);
  AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, sizeof(alipayScanDataDisc), (uint8_t *)alipayScanDataDisc);

  #endif

  /* start advertising; automatically set connectable/discoverable mode and bondable mode */
  //AppAdvStart(APP_MODE_AUTO_INIT);
}




static void fitSetup(fitMsg_t *pMsg)
{
    //RyeexAdvNormalSend();
}

/*************************************************************************************************/
/*!
 *  \fn     fitProcMsg
 *
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void fitProcMsg(fitMsg_t *pMsg)
{

//  if (is_deepsleep_active()) {
//    LOG_INFO("fitProcMsg event => %d", pMsg->hdr.event);
//  }
        
  switch(pMsg->hdr.event)
  {
    case FIT_RUNNING_TIMER_IND:
      fitSendRunningSpeedMeasurement((dmConnId_t)pMsg->ccc.hdr.param);
      break;

    case FIT_HR_TIMER_IND:
      HrpsProcMsg(&pMsg->hdr);
      break;

    case FIT_BATT_TIMER_IND:
      BasProcMsg(&pMsg->hdr);
      break;

    case ATTS_HANDLE_VALUE_CNF:
      HrpsProcMsg(&pMsg->hdr);
      BasProcMsg(&pMsg->hdr);
#if (RY_BLE_ENABLE == TRUE) 
      //ry_ble_proc_msg(&pMsg->hdr);
#endif
      break;

    case ATTS_CCC_STATE_IND:
      fitProcCccState(pMsg);
      break;

    case ATT_MTU_UPDATE_IND:
      //LOG_WARN("Negotiated MTU %d\n", ((attEvt_t *)pMsg)->mtu);
      break;
    case DM_RESET_CMPL_IND:
      DmSecGenerateEccKeyReq();
      //fitSetup(pMsg);
      break;

    case DM_ADV_START_IND:
      //LOG_INFO("fitProcMsg event => adv start");
      break;

    case DM_ADV_STOP_IND:
      //LOG_INFO("fitProcMsg event => adv stop");
      break;

    case DM_CONN_OPEN_IND:
      HrpsProcMsg(&pMsg->hdr);
      BasProcMsg(&pMsg->hdr);
      break;

    case DM_CONN_CLOSE_IND:
      fitClose(pMsg);
      break;

    case DM_SEC_PAIR_CMPL_IND:
      LOG_INFO("fitProcMsg event => pair complete");
      break;

    case DM_SEC_PAIR_FAIL_IND:
      LOG_INFO("fitProcMsg event => pair failed");
      break;

    case DM_SEC_ENCRYPT_IND:
      break;

    case DM_SEC_ENCRYPT_FAIL_IND:
      break;

    case DM_SEC_AUTH_REQ_IND:
      LOG_INFO("fitProcMsg event => sec auth request");
      LOG_INFO("auth_req.display => %d", &pMsg->dm.authReq.display);
      AppHandlePasskey(&pMsg->dm.authReq);
      break;

    case DM_SEC_ECC_KEY_IND:
      fitSetup(pMsg);
      DmSecSetEccKey(&pMsg->dm.eccMsg.data.key);
      break;

    case DM_SEC_COMPARE_IND:
      LOG_INFO("fitProcMsg event => sec compare");
      LOG_INFO("confirm value => %d", &pMsg->dm.cnfInd.confirm);
      AppHandleNumericComparison(&pMsg->dm.cnfInd);
      break;

    case DM_ERROR_IND:
        LOG_DEBUG("[BLE_HCI] DM_ERROR_IND General error\n");
        break;
        
    default:
      break;
  }
  
//  AppUiAction(pMsg->hdr.event);
}



/*************************************************************************************************/
/*!
 *  \fn     FitHandlerInit
 *
 *  \brief  Application handler init function called during system initialization.
 *
 *  \param  handlerID  WSF handler ID.
 *
 *  \return None.
 */
/*************************************************************************************************/
void FitHandlerInit(wsfHandlerId_t handlerId)
{
  //APP_TRACE_INFO0("FitHandlerInit");

  /* store handler ID */
  fitHandlerId = handlerId;

  /* Set configuration pointers */
  pAppAdvCfg = (appAdvCfg_t *) &fitAdvCfg;
  pAppSlaveCfg = (appSlaveCfg_t *) &fitSlaveCfg;
  pAppSecCfg = (appSecCfg_t *) &fitSecCfg;
  pAppUpdateCfg = (appUpdateCfg_t *) &fitUpdateCfg;

  /* Initialize application framework */
  AppSlaveInit();
  AppDiscInit();

  /* Set stack configuration pointers */
  pSmpCfg = (smpCfg_t *) &fitSmpCfg;

  /* initialize heart rate profile sensor */
  HrpsInit(handlerId, (hrpsCfg_t *) &fitHrpsCfg);
  HrpsSetFlags(CH_HRM_FLAGS_VALUE_8BIT | CH_HRM_FLAGS_ENERGY_EXP);

  /* initialize battery service server */
  BasInit(handlerId, (basCfg_t *) &fitBasCfg);
  AncsHandlerInit(handlerId);

  /* initialize alipay service server */
  AlipayHandleInit(handlerId);
}

/*************************************************************************************************/
/*!
 *  \fn     FitHandler
 *
 *  \brief  WSF event handler for application.
 *
 *  \param  event   WSF event mask.
 *  \param  pMsg    WSF message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void FitHandler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
  //LOG_DEBUG("[FitHandler]\n");
  if (pMsg != NULL)
  {
    //LOG_DEBUG("Fit got evt %d on handle 0x%04x\n", pMsg->event, ((attEvt_t *)pMsg)->handle);
    if ( pMsg->event <= ATT_CBACK_END )
    {   //process discovery-related ATT messages
        AppDiscProcAttMsg((attEvt_t *) pMsg);   //process ATT messages
    }
 
      if (pMsg->event >= DM_CBACK_START && pMsg->event <= DM_CBACK_END)
    {
        /* process advertising and connection-related messages */
        AppSlaveProcDmMsg((dmEvt_t *) pMsg);

        /* process security-related messages */
        AppSlaveSecProcDmMsg((dmEvt_t *) pMsg);

        AppDiscProcDmMsg((dmEvt_t *) pMsg);

    }

    /* perform profile and user interface-related operations */
    fitProcMsg((fitMsg_t *) pMsg);

    /* perform profile and user interface-related operations */
    ancsProcMsg((void*) pMsg);
    WechatProcMsg((void*) pMsg);
    alipayProcMsg((void*) pMsg);
    ry_ble_proc_msg((void*)pMsg);
  }
}

/*************************************************************************************************/
/*!
 *  \fn     RyeexsWriteCback
 *        
 *  \brief  ATTS write callback for heart rate service.  Use this function as a parameter
 *          to SvcHrsCbackRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t RyeexsWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                       uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{ 
  int i;
  
  //LOG_DEBUG("[RyeexsWriteCback] connId: %d\n", connId);
  //LOG_DEBUG("[RyeexsWriteCback] handle: 0x%04x\n", handle);
  //LOG_DEBUG("[RyeexsWriteCback] operation:%d\n", operation);
  //LOG_DEBUG("[RyeexsWriteCback] offset: %d\n", offset);
  //LOG_DEBUG("[RyeexsWriteCback] len: %d\n", len);
  //ry_data_dump(pValue, len, ' ');

  if (handle == RYEEXS_SECURE_CH_VAL_HDL) {
      //r_xfer_on_receive(pValue, len, 1);
      r_xfer_onBleReceived(pValue, len, ATT_UUID_RYEEX_SECURE_CH);
  } else if (handle == RYEEXS_UNSECURE_CH_VAL_HDL) {
      //r_xfer_on_receive(pValue, len, ATT_UUID_RYEEX_UNSECURE_CH);
      r_xfer_onBleReceived(pValue, len, ATT_UUID_RYEEX_UNSECURE_CH);
  } else if (handle == RYEEXS_JSON_CH_VAL_HDL) {
      r_xfer_onBleReceived(pValue, len, ATT_UUID_RYEEX_JSON_CH);
  }
  

  return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \fn     MisReadCback
 *        
 *  \brief  ATTS write callback for heart rate service.  Use this function as a parameter
 *          to SvcHrsCbackRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t MisReadCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                     uint16_t offset, attsAttr_t *pAttr)
{ 
  int i;
  uint8_t idx;
  
  LOG_DEBUG("[MisReadCback] connId: 0x%x\n", connId);
  LOG_DEBUG("[MisReadCback] handle: 0x%04x\n", handle);
  LOG_DEBUG("[MisReadCback] operation:%d\n", operation);
  LOG_DEBUG("[MisReadCback] offset: %d\n", offset);




  //mible_onCharRead(connId, idx, len, pValue);

  return ATT_SUCCESS;
}


/*************************************************************************************************/
/*!
 *  \fn     MisWriteCback
 *        
 *  \brief  ATTS write callback for heart rate service.  Use this function as a parameter
 *          to SvcHrsCbackRegister().
 *
 *  \return ATT status.
 */
/*************************************************************************************************/
uint8_t MisWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                       uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{ 
  int i;
  uint8_t idx;
  
  LOG_DEBUG("[MisWriteCback] connId: 0x%x\n", connId);
  LOG_DEBUG("[MisWriteCback] handle: 0x%04x\n", handle);
  LOG_DEBUG("[MisWriteCback] operation:%d\n", operation);
  LOG_DEBUG("[MisWriteCback] offset: %d\n", offset);
  LOG_DEBUG("[MisWriteCback] len: %d\n", len);
  ry_data_dump(pValue, len, ' ');


  switch (handle) {
    case MIS_TOKEN_VAL_HDL:
        idx = MIBLE_CHAR_IDX_TOKEN;
        break;

    case MIS_AUTH_VAL_HDL:
        idx = MIBLE_CHAR_IDX_AUTH;
        break;

    case MIS_SN_VAL_HDL:
        idx = MIBLE_CHAR_IDX_SN;
        break;

    case MIS_BEACON_KEY_VAL_HDL:
        idx = MIBLE_CHAR_IDX_BEACONKEY;
        break; 

    default:
        break;
  
  }

  mible_onCharWritten(connId, (mible_charIdx_t)idx, len, pValue);

  return ATT_SUCCESS;
}

/*************************************************************************************************/
/*!
 *  \fn     FitStart
 *
 *  \brief  Start the application.
 *
 *  \return None.
 */
/*************************************************************************************************/
void FitStart(void)
{
  /* Register for stack callbacks */
  DmRegister(fitDmCback);
  DmConnRegister(DM_CLIENT_ID_APP, fitDmCback);
  AttRegister(fitAttCback);
  AttConnRegister(AppServerConnCback);
  AttsCccRegister(FIT_NUM_CCC_IDX, (attsCccSet_t *) fitCccSet, fitCccCback); 
    
  AncsStart();
    
  /* Initialize attribute server database */
  SvcCoreAddGroup();
  SvcHrsCbackRegister(NULL, HrpsWriteCback);
  SvcHrsAddGroup();
  SvcDisAddGroup();
//  SvcBattCbackRegister(BasReadCback, NULL);
//  SvcBattAddGroup();
//  SvcRscsAddGroup();
  SvcRyeexsAddGroup();
  SvcRyeexsCbackRegister(NULL, RyeexsWriteCback);
  SvcMisAddGroup();
  SvcMisCbackRegister(MisReadCback, MisWriteCback);

  //SvcAlipayAddGroup();
  SvcAlipayCbackRegister(NULL, alipay_write_cback);

  WechatServiceInit();

  /* Set running speed and cadence features */
//  RscpsSetFeatures(RSCS_ALL_FEATURES);

  /* Reset the device */
  DmDevReset();

}
