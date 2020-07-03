// ****************************************************************************
//
//  ancs_main.c
//! @file
//!
//! @brief Ambiq Micro's demonstration of ANCSC.
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

#include "ancs_api.h"

#include <string.h>
#include <stdint.h>
#include "bstream.h"
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_msg.h"
#include "hci_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "gatt_api.h"
#include "svc_core.h"

#include "ancc_api.h"
#include "app_cfg.h"
#include "app_api.h"

#include "ry_utils.h"
#include "ryos.h"

#include "ry_hal_mcu.h"
#include "ry_hal_flash.h"
#include "ryos_timer.h"

#include "ble_task.h"
#include "msg_management_service.h"
#include "Notification.pb.h"
#include "gui_msg.h"
#include "app_main.h"
#include "device_management_service.h"
#include "timer_management_service.h"

#ifndef ANCS_ENABLED
#define ANCS_ENABLED    1
#endif

#if ANCS_ENABLED
/**************************************************************************************************
  Macros
**************************************************************************************************/

#define APPID_RY_MAX_SIZE       32

/*! Start of each service's handles in the the handle list */
#define ANCS_DISC_GATT_START        0
#define ANCS_DISC_ANCS_START        (ANCS_DISC_GATT_START + GATT_HDL_LIST_LEN)
#define ANCS_DISC_HDL_LIST_LEN      (ANCS_DISC_ANCS_START + ANCC_HDL_LIST_LEN)

#define NOTIFICAITON_STORED_DEFAULT_COUNT (ancs_appid_type_counter - 2)

#define ANCS_WHITELIST_MAX_ENRTY    20
#define ANCS_FLASH_MAGIC_NUMBER     0x20180814
#define ANCS_FLASH_INFO_ADDR        0xFB000

/*! WSF message event enumeration */
enum
{
    ANCC_ACTION_TIMER_IND = ANCS_MSG_START,            /*! ANCC action timer expired */
    ANCS_ACTION_REJECT_TELE,         // try reject call from
    ANCS_ACTION_RY_BIND_FINISHED,
    ANCS_ACTION_RY_SET_IOS_APP,
    ANCS_ACTION_RY_SET_NOT_IOS_APP, //Android ANCS Disabled
    ANCS_ACTION_RY_SET_ENABLED,
    ANCS_ACTION_RY_SET_DISABLED,
    ANCS_ACTION_RY_ENABLE_ALL,
    ANCS_ACTION_DEVICE_UPLOAD,
    ANCS_ACTION_SERVICE_CHANGED,    // 0xB9 service changed maybe iOS 11.4 -> 12.0
};

typedef union
{
    uint32_t raw;
    struct
    {
        unsigned status_ancs:8;
        
        unsigned isConnected:1;
        unsigned isSecureLink:1;
        unsigned isNotifyEnabled:1;
        unsigned isAncsEnabled:1;

        unsigned isAnccPending:1;
        unsigned isDBstored:1;
        unsigned isDBHandleStored:1;
        unsigned isCurrentHandleValid:1;

        unsigned isConnectedIOS:1;
        unsigned isConnectedAndroid:1;

        unsigned reserved:14;
    }payload;
}ancsDiagnosis_t;


typedef enum
{
    ancs_tel_recorder_status_idle = 0,
    ancs_tel_recorder_status_pending_attribute,
    ancs_tel_recorder_status_calling, //can be reject
}ancs_tel_recorder_status_t;

typedef struct
{
    ancs_tel_recorder_status_t status;
    ancc_notif_t notify;
    struct{
        uint8_t title[64];
    }msg;
}ancs_tel_recorder_t;


typedef enum
{
    ancs_appid_type_tele = 0,
    ancs_appid_type_sms,
    ancs_appid_type_wx,
    ancs_appid_type_qq,
    ancs_appid_type_weibo,

    ancs_appid_type_taobao,
    ancs_appid_type_toutiao,
    ancs_appid_type_netease_news,
    ancs_appid_type_zhihu,
    ancs_appid_type_alipay,

    ancs_appid_type_douyin,
    ancs_appid_type_dingding,
    ancs_appid_type_mijia,
    ancs_appid_type_momo,
    ancs_appid_type_tim,

    ancs_appid_type_hey_plus,

    ancs_appid_type_counter,
    
    ancs_appid_type_others = 0xFE,
}ancs_appid_type_t;

/*! Application message type */
typedef union
{
    wsfMsgHdr_t     hdr;
    dmEvt_t         dm;
    attsCccEvt_t    ccc;
    attEvt_t        att;
} ancsMsg_t;

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} ry_ancs_convert_time_t;


/*! Discovery states:  enumeration of services to be discovered */
enum
{
  ANCS_DISC_DISCONNECTED = 0,
  ANCS_DISC_CONNECTED,
  ANCS_DISC_CONNECTED_NOT_IOS,  //ANCS Never Enable
  ANCS_DISC_RY_BIND_FINISHED,
//  ANCS_DISC_BLE_BIND_PERMITTED, //ANCS DISC STARTED
  ANCS_DISC_GATT_SVC = 0x80,      /* GATT service */
  ANCS_DISC_ANCS_SVC,      /* discover ANCS service */
  ANCS_DISC_SVC_MAX,        /* Discovery complete */
  ANCS_DISC_CFG_CMPL,       // all done
};

/*! enumeration of client characteristic configuration descriptors */
enum
{
    ANCS_GATT_SC_CCC_IDX,               /*! GATT service, service changed characteristic */
    ANCS_NUM_CCC_IDX
};

#pragma pack(1)

typedef struct
{
    uint8_t appid[APPID_RY_MAX_SIZE];
    uint8_t isEnabled;
} notificaion_whitelist_resource_t;

typedef struct
{
    uint32_t curNum;
    notificaion_whitelist_resource_t records[ANCS_WHITELIST_MAX_ENRTY];
} notificaion_whitelist_tbl_t;

#define NOTIFICAION_SETTING_CFG_CURRENT_VERSION         (2)
#define NOTIFICAION_SETTING_CFG_CURRENT_SUB_VERSION     (6)
typedef struct
{
    uint32_t magic_number;
    uint32_t version;       //version 1 store iOS app id directly, version 2 save ry_appid
    uint32_t op_times;
    uint8_t isOthersEnabled;
    uint8_t isAncsOpend;
    uint8_t isAllOpend;
    uint8_t subVersion;     // use for debug to force clear all stored info
    notificaion_whitelist_tbl_t tbl;
} notification_whitelist_stored_t;

#pragma pack()

/*! application control block */
static struct
{
  /* ancs tracking variables */
  uint16_t          hdlList[APP_DB_HDL_LIST_LEN];
  wsfHandlerId_t    handlerId;
  uint32_t          ancsRyTimerID;      //检查绑定相关，准备添加检查BLE绑定逻辑
  uint16_t          connHandle;         //用于弹出绑定请求
  uint8_t           discState;
  uint8_t           disc_retry_times;   //服务发现重试次数
} ancsCb = {
    .discState = 0,  //ANCS_DISC_NOT_STARTED
    .disc_retry_times = 0,
    .connHandle = 0xFFFF,
};

static ancs_tel_recorder_t m_ancs_tel_recorder = {
    .status = ancs_tel_recorder_status_idle,
};


static char const* const p_appid_source[ancs_appid_type_counter] =  {
    [ancs_appid_type_tele]      = "com.apple.mobilephone",      //呼入电话 和 未接来电
    [ancs_appid_type_sms]       = "com.apple.MobileSMS",        //短信
    [ancs_appid_type_wx]        = "com.tencent.xin",            //微信
    [ancs_appid_type_qq]        = "com.tencent.mqq",            //手机QQ
    [ancs_appid_type_weibo]     = "com.sina.weibo",             //新浪微博

    [ancs_appid_type_taobao]    = "com.taobao.taobao4iphone",   //淘宝
    [ancs_appid_type_toutiao]   = "com.ss.iphone.article.News", //今日头条
    [ancs_appid_type_netease_news] = "com.netease.news",        //网易新闻
    [ancs_appid_type_zhihu]     = "com.zhihu.ios",              //知乎
    [ancs_appid_type_alipay]    = "com.alipay.iphoneclient",    //支付宝

    [ancs_appid_type_douyin]    = "com.ss.iphone.ugc.Aweme",    //抖音
    [ancs_appid_type_dingding]  = "com.laiwang.DingTalk",       //钉钉
    [ancs_appid_type_mijia]      = "com.xiaomi.mihome",          //米家
    [ancs_appid_type_momo]      = "com.wemomo.momoappdemo1",    //陌陌
    [ancs_appid_type_tim]       = "com.tencent.tim",            //TIM

    [ancs_appid_type_hey_plus]  = "com.ryeex.groot",            //如一APP
};


static char const* const p_appid_ry[ancs_appid_type_counter] = {
    [ancs_appid_type_tele]      = APP_TELE,             //呼入电话 和 未接来电
    [ancs_appid_type_sms]       = APP_SMS,              //短信
    [ancs_appid_type_wx]        = APP_WEIXIN,           //微信
    [ancs_appid_type_qq]        = APP_QQ,               //手机QQ
    [ancs_appid_type_weibo]     = APP_WEIBO,            //新浪微博

    [ancs_appid_type_taobao]    = APP_TAOBAO,           //淘宝
    [ancs_appid_type_toutiao]   = APP_TOUTIAO,          //今日头条
    [ancs_appid_type_netease_news] = APP_NETEASE,       //网易新闻
    [ancs_appid_type_zhihu]     = APP_ZHIHU,            //知乎
    [ancs_appid_type_alipay]    = APP_ZHIFUBAO,         //支付宝

    [ancs_appid_type_douyin]    = APP_DOUYIN,           //抖音
    [ancs_appid_type_dingding]  = APP_DINGDING,         //钉钉
    [ancs_appid_type_mijia]     = APP_MIJIA,            //米家
    [ancs_appid_type_momo]      = APP_MOMO,             //陌陌
    [ancs_appid_type_tim]       = APP_OTHERS,          //TIM

    [ancs_appid_type_hey_plus]  = APP_OTHERS,          //如一APP
};

/*! Pointers into handle list for each service's handles */
static uint16_t *pAncsGattHdlList = &ancsCb.hdlList[ANCS_DISC_GATT_START];
static uint16_t *pAncsAnccHdlList = &ancsCb.hdlList[ANCS_DISC_ANCS_START];

/**************************************************************************************************
  ATT Client Configuration Data
**************************************************************************************************/

/*
 * Data for configuration after service discovery
 */

/* Default value for CCC notifications */
static const uint8_t ancsCccNotifyNtfVal[] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_NOTIFY)};

static uint8_t ancsCccUpdateNtfVal[] = {UINT16_TO_BYTES(ATT_CLIENT_CFG_NOTIFY)};

/* List of characteristics to configure after service discovery */
static const attcDiscCfg_t ancsDiscCfgList[] =
{
  /* Write:  GATT service changed ccc descriptor */
  {ancsCccNotifyNtfVal, sizeof(ancsCccNotifyNtfVal), (GATT_SC_CCC_HDL_IDX + ANCS_DISC_GATT_START)},

  /* Write:  ANCS setting ccc descriptor */
  {ancsCccUpdateNtfVal, sizeof(ancsCccUpdateNtfVal), (ANCC_NOTIFICATION_SOURCE_CCC_HDL_IDX + ANCS_DISC_ANCS_START)},

  /* Write:  ANCS setting ccc descriptor */
  {ancsCccNotifyNtfVal, sizeof(ancsCccNotifyNtfVal), (ANCC_DATA_SOURCE_CCC_HDL_IDX + ANCS_DISC_ANCS_START)},
};

/* Characteristic configuration list length */
#define ANCS_DISC_CFG_LIST_LEN   (sizeof(ancsDiscCfgList) / sizeof(attcDiscCfg_t))

WSF_CT_ASSERT(ANCS_DISC_CFG_LIST_LEN <= ANCS_DISC_HDL_LIST_LEN);


static const attcDiscCfg_t ancsUpdateCfgList[] = 
{
    /* Write:  ANCS setting ccc descriptor */
    {ancsCccUpdateNtfVal, sizeof(ancsCccUpdateNtfVal), (ANCC_NOTIFICATION_SOURCE_CCC_HDL_IDX + ANCS_DISC_ANCS_START)},
};

/*! client characteristic configuration descriptors settings, indexed by above enumeration */
static const attsCccSet_t ancsCccSet[ANCS_NUM_CCC_IDX] =
{
    /* cccd handle          value range               security level */
    {GATT_SC_CH_CCC_HDL,    ATT_CLIENT_CFG_INDICATE,  DM_SEC_LEVEL_NONE},    /* ANCS_GATT_SC_CCC_IDX */
};


static void ancsSendEventThroughBleThread(uint8_t evt_type);

/* sanity check:  make sure configuration list length is <= handle list length */

/*! Configurable parameters for service and characteristic discovery */
static const appDiscCfg_t ancsDiscCfg =
{
    .waitForSec = TRUE,                                   /*! TRUE to wait for a secure connection before initiating discovery */
};

/*! ANCC Configurable parameters */
static const anccCfg_t ancsAnccCfg =
{
   200         /*! action timer expiration period in ms */
};

static void ancsCheckDbStatusBeforeCCC(void)
{
    if((ancsCb.hdlList[ancsDiscCfgList[1].hdlIdx] == 0)
        ||((ancsCb.hdlList[ancsDiscCfgList[2].hdlIdx] == 0))
        ) {
        LOG_ERROR("[ANCS] Remote handle DB error !!! ccc will fail\n");
    }
}

static void ancsOnUpdateLoaclCCCConfig(bool isOpen)
{
    if(isOpen) {
        UINT16_TO_BUF(ancsCccUpdateNtfVal, ATT_CLIENT_CFG_NOTIFY);
    } else {
        UINT16_TO_BUF(ancsCccUpdateNtfVal, 0x0);
    }
    LOG_DEBUG("[ANCS] ccc set local:%d\n", isOpen);
}

static void ancsUpdateCCCStatus(bool isOpen)
{
    uint8_t secLevel = DmConnSecLevel(ancsCb.connHandle);

    LOG_DEBUG("[ANCS] ccc set on seclevel:%d\n", secLevel);

    ancsCheckDbStatusBeforeCCC();
    ancsOnUpdateLoaclCCCConfig(isOpen);

    AppDiscConfigure(ancsCb.connHandle, 0xFF, ANCS_DISC_CFG_LIST_LEN,
                     (attcDiscCfg_t *) ancsDiscCfgList, ANCS_DISC_HDL_LIST_LEN, ancsCb.hdlList);
}

static void ancsOnTrySendGuiActiveEvnet(void)
{
    if(current_time_is_dnd_status() != 1) {
        ry_sched_msg_send(IPC_MSG_TYPE_MM_NEW_NOTIFY, 0, NULL);
    }
}


static void ancsOnSendNotifyMsgToGUI(uint16_t type,
            uint32_t time,
            uint8_t* title,
            uint8_t* data,
            uint8_t *app_id)
{
    if(data == NULL) {
        data = " ";
    }
    if(title == NULL) {
        title  = " ";
    }
    app_notify_add_msg(type, time, title, data, app_id);
}


notification_whitelist_stored_t const* const p_stored_notify_whitelist = (notification_whitelist_stored_t*)ANCS_FLASH_INFO_ADDR;

static void ancs_whitelist_init(void)
{
    if((p_stored_notify_whitelist->magic_number == ANCS_FLASH_MAGIC_NUMBER)
        &&(p_stored_notify_whitelist->version >= NOTIFICAION_SETTING_CFG_CURRENT_VERSION)
#if RY_BUILD == RY_DEBUG
        &&(p_stored_notify_whitelist->subVersion == NOTIFICAION_SETTING_CFG_CURRENT_SUB_VERSION)
#endif
        ) {  //valid record
        LOG_INFO("[ANCS] check passed op_times:%d\n", p_stored_notify_whitelist->op_times);
    } else {
        LOG_ERROR("[ANCS]-[%s] whitelist reset to default\n", __FUNCTION__);
        notification_whitelist_stored_t* p_ancs_whitelist_default = ry_malloc(sizeof(notification_whitelist_stored_t));
        RY_ASSERT(p_ancs_whitelist_default != NULL);
        ry_memset(p_ancs_whitelist_default, 0, sizeof(notification_whitelist_stored_t));
        p_ancs_whitelist_default->magic_number = ANCS_FLASH_MAGIC_NUMBER;
        p_ancs_whitelist_default->version = NOTIFICAION_SETTING_CFG_CURRENT_VERSION;
        p_ancs_whitelist_default->op_times = 1;
        p_ancs_whitelist_default->isOthersEnabled = 0;
        p_ancs_whitelist_default->isAncsOpend = 0;
        p_ancs_whitelist_default->isAllOpend= 0;
        p_ancs_whitelist_default->subVersion = NOTIFICAION_SETTING_CFG_CURRENT_SUB_VERSION;
        p_ancs_whitelist_default->tbl.curNum = NOTIFICAITON_STORED_DEFAULT_COUNT;

        for(int i=0;i<p_ancs_whitelist_default->tbl.curNum;i++) {
            strcpy((void*)p_ancs_whitelist_default->tbl.records[i].appid, p_appid_ry[i]);
            p_ancs_whitelist_default->tbl.records[i].isEnabled = 0;
        }
            
        p_ancs_whitelist_default->tbl.records[0].isEnabled = 1;
        p_ancs_whitelist_default->tbl.records[1].isEnabled = 1;
        p_ancs_whitelist_default->tbl.records[2].isEnabled = 1;

        uint32_t irq_stored = ry_hal_irq_disable();

        ry_hal_flash_write(ANCS_FLASH_INFO_ADDR, (void*)p_ancs_whitelist_default, sizeof(notification_whitelist_stored_t));

        ry_hal_irq_restore(irq_stored);

        ry_free(p_ancs_whitelist_default);
    } 
}

static uint32_t ancs_whitelist_save(notification_whitelist_stored_t* p_whitelist_new)
{
    if((p_whitelist_new == NULL)
        ||(p_whitelist_new->version <= 1)
        ) {
        return RY_ERR_INVALIC_PARA;
    }

    p_whitelist_new->magic_number = p_stored_notify_whitelist->magic_number;
    p_whitelist_new->subVersion = p_stored_notify_whitelist->subVersion;
    uint32_t irq_stored = ry_hal_irq_disable();
    p_whitelist_new->op_times = p_stored_notify_whitelist->op_times + 1;

    uint32_t ret = ry_hal_flash_write(ANCS_FLASH_INFO_ADDR, (void*)p_whitelist_new, sizeof(notification_whitelist_stored_t));

    ry_hal_irq_restore(irq_stored);
    return ret;
}

static notificaion_whitelist_resource_t* ancs_whitelist_search(const uint8_t* p_appid, uint32_t len)
{
    void* p_result = ry_tbl_search(ANCS_WHITELIST_MAX_ENRTY,
        sizeof(notificaion_whitelist_resource_t),
        (void*)p_stored_notify_whitelist->tbl.records,
        0,
        len,
        (uint8_t*)p_appid);
    return p_result;
}

static void ancsBleBondStatusCheckTimerHandle(void)
{
    LOG_ERROR("[ANCS] Remote Bonded Check Timeout\n");
    ancsSendEventThroughBleThread(ANCS_ACTION_RY_ENABLE_ALL);
}

static void ancsBleBondStatusCheckTimerInit(void)
{
    //timer_para.timeout_CB = update_timeout_handle;
    ry_timer_parm timer_para;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "ancc";
    ancsCb.ancsRyTimerID = ry_timer_create(&timer_para);
}

static void ancsBleBondStatusCheckTimerStart(void)
{
    uint32_t const detectAncsBondTimeOutIOS = 8000;
    uint32_t ret = ry_timer_start(ancsCb.ancsRyTimerID, detectAncsBondTimeOutIOS, (ry_timer_cb_t)ancsBleBondStatusCheckTimerHandle, NULL);
}

static void ancsBleBondStatusCheckTimerStop(void)
{
    ry_timer_stop(ancsCb.ancsRyTimerID);
}

static void ancsSendEventThroughBleThread(uint8_t evt_type)
{
    int len = sizeof(ancsMsg_t);
    ancsMsg_t* pMsg = WsfMsgAlloc(len);
    if (pMsg != NULL) {
        pMsg->hdr.event = evt_type;
        WsfMsgSend(ancsCb.handlerId, pMsg);
        wakeup_ble_thread();
    }
}

static void ancsMsgToGUIIndicateCallingRemoved(ancc_notif_t* p_notify)
{
    uint8_t* p_title = NULL;
    
    if(m_ancs_tel_recorder.status == ancs_tel_recorder_status_calling) {
        p_title = m_ancs_tel_recorder.msg.title;
        LOG_DEBUG("[ANCS] calling msg removed with UID:%d from %s\n",
                p_notify->notification_uid,
                p_title);
    } else {
        LOG_DEBUG("[ANCS] calling msg removed with UID:%d\n",p_notify->notification_uid);
        p_title = (uint8_t*)"CallRemoved";
    }
    uint8_t* p_text  = (uint8_t*)"CallRemoved";
    const uint8_t telephone_msg_type_calling = Telephony_Status_CONNECTED;
    uint8_t p_ry_app_id[1];
    *p_ry_app_id = telephone_msg_type_calling;
    ancsOnSendNotifyMsgToGUI(Notification_Type_TELEPHONY, ryos_rtc_now(), p_title, p_text, p_ry_app_id);

    ancsOnTrySendGuiActiveEvnet();
}


static uint32_t remap_from_iOS_appid_to_gui_msg_id(uint8_t const * const p_iOS_appid,
        ancc_notif_t* p_notify_evt,
        uint8_t* p_ry_msg_id, 
        Notification_Type* p_ry_notify_type)
{
    const uint8_t telephone_msg_type_incoming = Telephony_Status_RINGING_UNANSWERABLE;
//    const uint8_t telephone_msg_type_calling = Telephony_Status_CONNECTED;
    const uint8_t telephone_msg_type_missed = Telephony_Status_DISCONNECTED;

    if((p_ry_msg_id == NULL)
        ||(p_ry_notify_type == NULL)
        ) {
        return RY_ERR_INVALIC_PARA;
    }

    int index_matched = ancs_appid_type_others;
    for(int i =0;i < ancs_appid_type_counter; i++) {
        if(strcmp((const char*)p_iOS_appid, p_appid_source[i]) == 0) {
            index_matched = i;
            break;
        }
    }

    bool isInDependsOnOthers;

    char const* p_cfg_key = NULL;

    if(index_matched == ancs_appid_type_others) {
        isInDependsOnOthers = true;
    } else if(strcmp(p_appid_ry[index_matched], APP_OTHERS) == 0) {
        LOG_DEBUG("[ANCS] appid:%s in GrayList\n", p_iOS_appid);
        isInDependsOnOthers = true;
    } else {
        isInDependsOnOthers = false;
    }

    if(isInDependsOnOthers) {
        if(p_stored_notify_whitelist->isOthersEnabled == 0) {
            // Configed Others.
            LOG_DEBUG("[ANCS] appid:%s Others NOT ENABLED\n", p_iOS_appid);
            return RY_ERR_INVALID_TYPE;
        } else {
            //check passed
            p_cfg_key = APP_OTHERS;
        }
    } else {   //exactly enabled or not or default.
        p_cfg_key = p_appid_ry[index_matched];

        notificaion_whitelist_resource_t* p_whitelist = ancs_whitelist_search((uint8_t const*)p_cfg_key, strlen(p_cfg_key));
        if((p_whitelist != NULL)
            &&(p_whitelist->isEnabled == 0)
            ) {
            // this notification not enabled.
            LOG_DEBUG("[ANCS] appid:%s Cfg NOT ENABLED\n", p_iOS_appid);
            return RY_ERR_INVALID_TYPE;
        }

        if(p_whitelist == NULL) {
        #if 1
            // with icon and not in whitelist cfg
            LOG_DEBUG("[ANCS] appid:%s default NOT ENABLED\n", p_iOS_appid);
            return RY_ERR_INVALID_TYPE;
        #else
            LOG_DEBUG("[ANCS] appid:%s default ENABLED\n", p_iOS_appid);
        #endif
        }
    }

    switch(index_matched) {
      case ancs_appid_type_tele:
          *p_ry_notify_type = Notification_Type_TELEPHONY;
          if(p_notify_evt->category_id == BLE_ANCS_CATEGORY_ID_INCOMING_CALL) {
              *p_ry_msg_id = telephone_msg_type_incoming;   //only use first byte
          } else if(p_notify_evt->category_id == BLE_ANCS_CATEGORY_ID_MISSED_CALL) {
              *p_ry_msg_id = telephone_msg_type_missed;     //only use first byte
          }
          break;
      case ancs_appid_type_sms:
          ry_memcpy(p_ry_msg_id, (uint8_t*)p_cfg_key, (strlen((const char*)p_cfg_key) + 1));
          *p_ry_notify_type = Notification_Type_SMS;
          break;
      case ancs_appid_type_wx:
//          break;  //fall through
      case ancs_appid_type_qq:
//          break;  //fall through
      case ancs_appid_type_weibo:
//          break;  //fall through
      case ancs_appid_type_taobao:
//          break;  //fall through
      case ancs_appid_type_toutiao:
//          break;  //fall through
      case ancs_appid_type_netease_news:
//          break;  //fall through
      case ancs_appid_type_zhihu:
//          break;  //fall through
      case ancs_appid_type_alipay:
//          break;  //fall through
      case ancs_appid_type_douyin:
//          break;  //fall through
      case ancs_appid_type_dingding:
//          break;  //fall through
      case ancs_appid_type_mijia:
//          break;  //fall through
      case ancs_appid_type_momo:
//          break;  //fall through
      case ancs_appid_type_tim:
          ry_memcpy(p_ry_msg_id, (uint8_t*)p_cfg_key, (strlen(p_cfg_key) + 1));
          *p_ry_notify_type = Notification_Type_APP_MESSAGE;
          break;
      case ancs_appid_type_hey_plus:
//          break;  //fall through
      default:
      {   //default msg icon
          uint8_t* p_const_Others_id = "Others";
          ry_memcpy(p_ry_msg_id, (uint8_t*)p_const_Others_id, (strlen((void*)p_const_Others_id)+1));
          *p_ry_notify_type = Notification_Type_APP_MESSAGE;
      }
        break;
    }

    return RY_SUCC;
}

static void ancsAnccAttrCback(active_notif_t* pAttr)
{
    if(pAttr->attrId == BLE_ANCS_NOTIF_ATTR_RY_POPED) {
        ancc_notif_t ancs_notif = anccCb.anccList[pAttr->handle];
        LOG_DEBUG("[ANCS] poped action UID:%d\n\tflag:0x%02X,\n\tevt_type:%d,\n\tcat_id:%d,\n\tcat_count:%d\n", 
                ancs_notif.notification_uid,
                ancs_notif.event_flags,
                ancs_notif.event_id,
                ancs_notif.category_id,
                ancs_notif.category_count);
        if((ancs_notif.event_id == BLE_ANCS_EVENT_ID_NOTIFICATION_REMOVED)
            &&(ancs_notif.category_id == BLE_ANCS_CATEGORY_ID_INCOMING_CALL)
            ) {
            ancsMsgToGUIIndicateCallingRemoved(&ancs_notif);
            if(m_ancs_tel_recorder.status != ancs_tel_recorder_status_calling) {
                LOG_WARN("[ANCS] error remove incoming_call\n");
            }
            if(m_ancs_tel_recorder.notify.notification_uid == ancs_notif.notification_uid) {
                m_ancs_tel_recorder.status = ancs_tel_recorder_status_idle;
            }
        } else if((ancs_notif.event_id == BLE_ANCS_EVENT_ID_NOTIFICATION_ADDED)
            &&(ancs_notif.category_id == BLE_ANCS_CATEGORY_ID_INCOMING_CALL)
            ) {
            if(m_ancs_tel_recorder.status != ancs_tel_recorder_status_idle) {
                LOG_WARN("[ANCS] error repeat incoming_call\n");
            }
            m_ancs_tel_recorder.status = ancs_tel_recorder_status_pending_attribute;
            m_ancs_tel_recorder.notify = ancs_notif;
        }
    } else if(pAttr->attrId == BLE_ANCS_NOTIF_ATTR_ID_TITLE) {
          if((m_ancs_tel_recorder.status == ancs_tel_recorder_status_pending_attribute)
            &&(anccCb.anccList[pAttr->handle].notification_uid == m_ancs_tel_recorder.notify.notification_uid)
            ) {
                m_ancs_tel_recorder.status = ancs_tel_recorder_status_calling;
                if((pAttr->parseIndex + pAttr->attrLength) >= ANCC_ATTRI_BUFFER_SIZE_BYTES) {
                    LOG_ERROR("[ANCS] att parser\n");
                }
                ry_memcpy(m_ancs_tel_recorder.msg.title, 
                    &(pAttr->attrDataBuf[pAttr->parseIndex]), 
                    pAttr->attrLength);
          }
    }
}

static uint32_t convert_from_iOS_DataTime_to_ry_UTC(uint8_t* p_str)
{
    if((p_str == NULL)
        ||(p_str[0] != '2')
        ) {
        return ryos_rtc_now();
    }

    const uint8_t month_day[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    ry_ancs_convert_time_t time = {0};
    //
    // NotificationAttributeIDDate constant is a string that uses the Unicode Technical Standard
    // (UTS) #35 date format pattern yyyyMMdd'T'HHmmSS . The format of all the other constants in Table
    // 3-5 are UTF-8 strings.
    // sample as "20180725T152630" -> 2018.7.25 Thursday 15:26:30
    //
    time.year    = (p_str[2] - '0')*10  + (p_str[3] - '0');
    time.month   = (p_str[4] - '0')*10  + (p_str[5] - '0');
    time.day     = (p_str[6] - '0')*10  + (p_str[7] - '0');
    time.hour    = (p_str[9] - '0')*10  + (p_str[10] - '0');
    time.minute  = (p_str[11] - '0')*10 + (p_str[12] - '0');
    time.second  = (p_str[13] - '0')*10 + (p_str[14] - '0');
    uint32_t days = (time.year  + 30)*365 + (time.year + 30)/4;

    for(uint8_t i = 0; i < time.month - 1; i++) {
        days += month_day[i];
    }
    days += time.day - 1;
    
    uint64_t tick = (uint64_t)days * 86400 - 8 *3600 +time.hour * 3600 + time.minute *60 + time.second;

    return tick;
}

static void AncsNotifySolveContent(active_notif_t* pAttr)
{
    if(pAttr == NULL) {
        LOG_ERROR("[ANCS] NULL Pointer\n");
        return;
    }

    uint8_t attrCount =  pAttr->attrCount;
    if(attrCount > 8) {
        LOG_ERROR("[ANCS] Error Notification Attr Length with %d\n", pAttr->attrCount);
        return;
    }

    uint32_t ret = RY_SUCC;
    uint8_t* p_dataBuf = pAttr->attrDataBuf;
    ancc_notif_t ancs_notify_evt = anccCb.anccList[pAttr->handle];
    uint8_t p_ry_app_id[APPID_RY_MAX_SIZE] = {0};

    const uint8_t EventFlagSilent = (1<<0);
    const uint8_t EventFlagImportant = (1<<1);
    const uint8_t EventFlagPreExisting = (1<<2);
    const uint8_t EventFlagPositiveAction = (1<<3);
    const uint8_t EventFlagNegativeAction = (1<<4);

    if((ancs_notify_evt.event_flags & EventFlagPreExisting) != 0) {   //will not display the preExisting Notifications
        LOG_DEBUG("[ANCS] PreExisting UID:%d\n", ancs_notify_evt.notification_uid);
        return;
    }

    register uint16_t index = 0;
    register int index_str = 0;
    register int length = 0;
    register uint8_t* p_str = 0;

    uint8_t attrCounted = 0;

    uint8_t index_fix_zero_counter = 0;
    uint16_t index_fix_zero_arr[8];

    uint8_t* p_appid_iOS = 0;
    uint8_t* p_title = 0;
    uint8_t* p_sub_title = 0;
    uint8_t* p_text = 0;
    uint8_t* p_msg_size = 0;
    uint8_t* p_msg_date = 0;
    uint8_t* p_positive = 0;
    uint8_t* p_negative = 0;

    uint32_t ry_utc_stamp = 0;
    uint8_t temp_id = 0;
    Notification_Type ry_notify_type = Notification_Type_APP_MESSAGE;

    for(index = 5;index < ANCC_ATTRI_BUFFER_SIZE_BYTES;) {
        length = p_dataBuf[index + 2] * 256 + p_dataBuf[index + 1];
        index_str = index+3;

        attrCounted ++;

        if((length != 0) 
            &&(length + index_str < ANCC_ATTRI_BUFFER_SIZE_BYTES)
            ) {
            p_str = &p_dataBuf[index_str];
            if(length + index_str > ANCC_ATTRI_BUFFER_SIZE_BYTES) {
                LOG_ERROR("[ANCS] err parser fix zero\n");
            } else {
                index_fix_zero_arr[index_fix_zero_counter] = length + index_str;
                index_fix_zero_counter++;
            }
        } else {
            p_str = 0;
            // don't need to fix zero
        }

        switch(p_dataBuf[index]) {
            case BLE_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER:
              p_appid_iOS = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_TITLE:
              // ANCS raw telephone message will add a \u202d prefix and \u202c suffix
              p_title = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_SUBTITLE:
              p_sub_title = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_MESSAGE:
              p_text = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE:
              p_msg_size = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_DATE:
              p_msg_date = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL:
              p_positive = p_str;
              break;
            case BLE_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL:
              p_negative = p_str;
              break;
              default:break;
        }
        index = length + index_str;
        
        if(attrCounted >= attrCount) {
            break;
        }
    }

    for(int i = 0;i<index_fix_zero_counter ;i++) {
        p_dataBuf[index_fix_zero_arr[i]] = '\0';
    }

    LOG_WARN("[ANCS] app_notify_add_msg UID[%d] from[%s]:\n\ttitle:%s,\n\ttext:%s\n", 
          ancs_notify_evt.notification_uid, 
          p_appid_iOS,
          p_title,
          p_text);

    ret = remap_from_iOS_appid_to_gui_msg_id(p_appid_iOS, &ancs_notify_evt, p_ry_app_id, &ry_notify_type);
    
    if(ret == RY_SUCC) {
        ry_utc_stamp = convert_from_iOS_DataTime_to_ry_UTC(p_msg_date);


#if TRACE_NOTIFY_ITEM
        static uint32_t ancs_gui_counter = 0;
        ancc_notif_t* p_notif = &ancs_notify_evt;
        ancs_gui_counter++;
        if((p_notif->notification_uid % TRACE_NOTIFY_ITEM_SKIPS) == 0)
        {
            LOG_INFO("[ANCS] notify{%d} gui cnt:%d from[%s]\n",
                    p_notif->notification_uid,
                    ancs_gui_counter,
                    p_appid_iOS
                    );
        }
#endif
        ancsOnSendNotifyMsgToGUI(ry_notify_type, ry_utc_stamp, p_title, p_text, p_ry_app_id);

        ancsOnTrySendGuiActiveEvnet();
    }else {
        
    
#if TRACE_NOTIFY_ITEM
        static uint32_t ancs_callback_counter = 0;
        ancc_notif_t* p_notif = &anccCb.anccList[anccCb.active.handle];
        ancs_callback_counter++;
        if((p_notif->notification_uid % TRACE_NOTIFY_ITEM_SKIPS) == 0)
        {
            LOG_INFO("[ANCS] notify{%d} skipped cnt:%d from[%s], status:%d\n",
                    p_notif->notification_uid,
                    ancs_callback_counter,
                    p_appid_iOS,
                    ret
                    );
        }
#endif
    }
    //
    // removes notifications received
    //
    //AncsPerformNotiAction(pAncsAnccHdlList, ancs_notify_evt.notification_uid, BLE_ANCS_NOTIF_ACTION_ID_NEGATIVE);
}

static void ancsAnccNotifCback(active_notif_t* pAttr, uint32_t notiUid)
{
    AncsNotifySolveContent(pAttr);
}

//主要控制BLE绑定状态
static void ancsOnServiceStatusChanged(bool nowOpened)
{
    if(nowOpened
        &&(p_stored_notify_whitelist->isAncsOpend != 0)
        ) {
        //如果BLE 配对已经完成 不care 通知总开关
        return; //skip already Opend
    }
    if((!nowOpened)
        &&(p_stored_notify_whitelist->isAncsOpend == 0)
        &&(p_stored_notify_whitelist->isAllOpend == 0)
        ) {
        //如果BLE配对未完成，通知总开关同样需要为关闭
        return; //skip already Closed
    }
        
    uint32_t ret = RY_SUCC;
    notification_whitelist_stored_t* p_ancs_new_whitelist = NULL;

    p_ancs_new_whitelist = ry_malloc(sizeof(notification_whitelist_stored_t));
    if(p_ancs_new_whitelist == NULL) {
        LOG_ERROR("[ANCS]-[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    ry_memcpy((void*)p_ancs_new_whitelist, (void*)p_stored_notify_whitelist, sizeof(notification_whitelist_stored_t));
    p_ancs_new_whitelist->isAncsOpend = nowOpened?1:0;
    if(p_ancs_new_whitelist->isAncsOpend == 0) {
        //如果BLE配对未完成，通知总开关同样需要为关闭
        p_ancs_new_whitelist->isAllOpend = 0;
    }

    ret = ancs_whitelist_save(p_ancs_new_whitelist);
    if(ret != RY_SUCC) {
        LOG_ERROR("[ANCS]-[%s] save failed\n", __FUNCTION__);
    }

    ancsOnUpdateLoaclCCCConfig(p_stored_notify_whitelist->isAllOpend);

exit:
    ry_free(p_ancs_new_whitelist);
    return;
}
/*************************************************************************************************/
/*!
 *  \fn     ancsValueUpdate
 *
 *  \brief  Process a received ATT read response, notification, or indication.
 *
 *  \param  pMsg    Pointer to ATT callback event message.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void ancsValueUpdate(attEvt_t *pMsg)
{
    /* iOS notification */
    if ((pMsg->handle == pAncsAnccHdlList[ANCC_NOTIFICATION_SOURCE_HDL_IDX]) 
        || (pMsg->handle == pAncsAnccHdlList[ANCC_DATA_SOURCE_HDL_IDX]) 
        || (pMsg->handle == pAncsAnccHdlList[ANCC_CONTROL_POINT_HDL_IDX])
        ) {
        AnccNtfValueUpdate(pAncsAnccHdlList, pMsg, ANCC_ACTION_TIMER_IND);
    } else {
        LOG_DEBUG("Data received from other other handle\n");
    }

    /* GATT */
    if (GattValueUpdate(pAncsGattHdlList, pMsg) == ATT_SUCCESS) {
        return;
    }
}

/*************************************************************************************************/
/*!
 *  \fn     ancsDiscCback
 *
 *  \brief  Discovery callback.
 *
 *  \param  connId    Connection identifier.
 *  \param  status    Service or configuration status.
 *
 *  \return None.
 */
/*************************************************************************************************/
static void ancsDiscCback(dmConnId_t connId, uint8_t status)
{
    bool isAncsDiscFailed = false;
    switch(status) {
        case APP_DISC_INIT:
        LOG_DEBUG("[ANCS] ===APP_DISC_INIT\n");
        /* set handle list when initialization requested */
        AppDiscSetHdlList(connId, ANCS_DISC_HDL_LIST_LEN, ancsCb.hdlList);
        break;

        case APP_DISC_SEC_REQUIRED:
        LOG_DEBUG("[ANCS] ===APP_DISC_SEC_REQUIRED\n");
        /* request security */
        AppSlaveSecurityReq(connId);
        break;

        case APP_DISC_START:
        LOG_DEBUG("[ANCS] ===APP_DISC_START\n");
        /* initialize discovery state */
        ancsCb.discState = ANCS_DISC_GATT_SVC;

        /* discover GATT service */
        GattDiscover(connId, pAncsGattHdlList);
        break;

        case APP_DISC_FAILED:
        LOG_INFO("[ANCS] !!!!!Disc Failed. discState = %d!!!!!\n", ancsCb.discState);
        isAncsDiscFailed = true;
        case APP_DISC_CMPL:
            LOG_DEBUG("[ANCS] ===APP_DISC_CMPL\n");
            //expecting only ancs service to be discovered
            ancsCb.discState++;
            if (ancsCb.discState == ANCS_DISC_ANCS_SVC) {
                /* discover ANCS service */
                AnccSvcDiscover(connId, pAncsAnccHdlList);
                LOG_DEBUG("[ANCS] ===Discovering ANCS.\n");
            } else {
                if(!isAncsDiscFailed) {
                    LOG_DEBUG("[ANCS] After Disc and Set Phone Type iOS\n");
                    ry_ble_set_peer_phone_type(PEER_PHONE_TYPE_IOS);
                    ancsCb.disc_retry_times = 0;
                } else if(ry_ble_get_peer_phone_type() == PEER_PHONE_TYPE_IOS) {
                    LOG_ERROR("[ANCS] Ry set iOS and ANCS disc FAILED!! times:%d\n", ancsCb.disc_retry_times);
                    ancsSendEventThroughBleThread(ANCS_ACTION_SERVICE_CHANGED);
                }
                /* discovery complete */
                AppDiscComplete(connId, APP_DISC_CMPL);

                /* start configuration */
                LOG_DEBUG("[ANCS] Finished ANCS discovering. Disc CFG start.\n");

                ancsOnUpdateLoaclCCCConfig(p_stored_notify_whitelist->isAllOpend);
                AppDiscConfigure(connId, APP_DISC_CFG_START, ANCS_DISC_CFG_LIST_LEN,
                         (attcDiscCfg_t *) ancsDiscCfgList, ANCS_DISC_HDL_LIST_LEN, ancsCb.hdlList);
            }
            break;

        case APP_DISC_CFG_START:
            LOG_DEBUG("[ANCS] ===APP_DISC_CFG_START\n");
            /* start configuration */
            AppDiscConfigure(connId, APP_DISC_CFG_START, ANCS_DISC_CFG_LIST_LEN,
                   (attcDiscCfg_t *) ancsDiscCfgList, ANCS_DISC_HDL_LIST_LEN, ancsCb.hdlList);
            break;

        case APP_DISC_CFG_CMPL:
            if(ancsCb.discState != ANCS_DISC_CFG_CMPL) {
                LOG_INFO("[ANCS] ===APP_DISC_CFG_CMPL\n");
                AppDiscComplete(connId, status);
                ancsCb.discState = ANCS_DISC_CFG_CMPL;
            } else {
                LOG_DEBUG("[ANCS] ===cfg_complete\n");
            }
            break;

        case APP_DISC_CFG_CONN_START:
            LOG_DEBUG("[ANCS] ===APP_DISC_CFG_CONN_START\n");
            /* start connection setup configuration */
            AppDiscConfigure(connId, APP_DISC_CFG_CONN_START, ANCS_DISC_CFG_LIST_LEN,
                   (attcDiscCfg_t *) ancsDiscCfgList, ANCS_DISC_HDL_LIST_LEN, ancsCb.hdlList);
            break;

        default:
        break;
    }
}

/*************************************************************************************************/
/*!
 *  \fn     ancsProcMsg
 *
 *  \brief  Process messages from the event handler.
 *
 *  \param  pMsg    Pointer to message.
 *
 *  \return None.
 */
/*************************************************************************************************/
void ancsProcMsg(void* p_msg)
{
    ancsMsg_t* pMsg = (ancsMsg_t*)p_msg;

    switch(pMsg->hdr.event) {
        case ANCC_ACTION_TIMER_IND:
            AnccActionHandler(ANCC_ACTION_TIMER_IND);
            break;
        case ANCS_ACTION_REJECT_TELE:
            if(m_ancs_tel_recorder.status == ancs_tel_recorder_status_calling) {
                LOG_DEBUG("[ANCS] ANCS_ACTION_TYR_REJECT exec\n");
                AncsPerformNotiAction(pAncsAnccHdlList, 
                    m_ancs_tel_recorder.notify.notification_uid,
                    BLE_ANCS_NOTIF_ACTION_ID_NEGATIVE);
            }
            break;
        case ANCS_ACTION_RY_BIND_FINISHED:
            if(ancsCb.discState == ANCS_DISC_CONNECTED) {
                ancsCb.discState = ANCS_DISC_RY_BIND_FINISHED;
            } else {
                LOG_WARN("[ANCS] error status triggerd ry_bind\n");
            }
            break;
        case ANCS_ACTION_RY_SET_IOS_APP:
            if(ancsCb.discState == ANCS_DISC_RY_BIND_FINISHED) {
                DmSecSlaveReq((dmConnId_t) ancsCb.connHandle, 
                    pAppSecCfg->auth);
            } else if(ancsCb.discState == ANCS_DISC_CONNECTED) {
                uint8_t secLevel = DmConnSecLevel(ancsCb.connHandle);

                bool isBonded = (secLevel == DM_SEC_LEVEL_ENC_LESC);

                if(!isBonded) {
                    if(p_stored_notify_whitelist->isAllOpend) {
                        //iOS app已经连接,连接未Secure,并且通知已经打开
                        //可能已经被手机忽略配对 重新绑定
                        LOG_INFO("[ANCS] Reconnect and ReSend SecureReq\n");
                        ancsSendEventThroughBleThread(ANCS_ACTION_RY_ENABLE_ALL);
                    } else {
                        //iOS app已经连接,连接未Secure,并且通知未打开
                        ancsOnServiceStatusChanged(false);
                        LOG_ERROR("[ANCS] Reset Local Bond Status\n");
                    }
                } else {
                    LOG_DEBUG("[ANCS] Work allright skipped set iOS app cmd on status:%d\n", ancsCb.discState);
                }
            } else {
                LOG_DEBUG("[ANCS] skipped set iOS app cmd on status:%d\n", ancsCb.discState);
            }
            break;
        case ANCS_ACTION_RY_SET_NOT_IOS_APP:
            if((ancsCb.discState == ANCS_DISC_RY_BIND_FINISHED)
                ||(ancsCb.discState == ANCS_DISC_CONNECTED)
                ) {
                ancsCb.discState = ANCS_DISC_CONNECTED_NOT_IOS;
                LOG_DEBUG("[ANCS] Now working without ANCS\n");
            }
            break;
        case ANCS_ACTION_RY_SET_ENABLED:
            if(ancsCb.discState != ANCS_DISC_CONNECTED_NOT_IOS) {
                LOG_DEBUG("[ANCS] ANCS_ACTION_RY_SET_ENABLED\n");
                ancsUpdateCCCStatus(true);
            }
            break;
        case ANCS_ACTION_RY_SET_DISABLED:
            if(ancsCb.discState != ANCS_DISC_CONNECTED_NOT_IOS) {
                LOG_DEBUG("[ANCS] ANCS_ACTION_RY_SET_DISABLED\n");
                ancsUpdateCCCStatus(false);
            }
            break;
        case ANCS_ACTION_RY_ENABLE_ALL:
            if(ancsCb.discState != ANCS_DISC_CONNECTED_NOT_IOS) {
                LOG_DEBUG("[ANCS] ANCS_ACTION_RY_ENABLE_ALL\n");
                ancsCheckDbStatusBeforeCCC();
                ancsOnUpdateLoaclCCCConfig(p_stored_notify_whitelist->isAllOpend);
                DmSecSlaveReq((dmConnId_t) ancsCb.connHandle, 
                    pAppSecCfg->auth);
            }
            break;
        case ANCS_ACTION_DEVICE_UPLOAD:
            if(ancsCb.discState != ANCS_DISC_CONNECTED_NOT_IOS) {
                OnDeviceUploadNotificationSetting();
            }
            break;
        case ANCS_ACTION_SERVICE_CHANGED:
            if(ancsCb.discState == ANCS_DISC_CFG_CMPL) {
                LOG_DEBUG("[ANCS] ANCS_ACTION_SERVICE_CHANGED\n");
                ancsCb.discState = ANCS_DISC_ANCS_SVC;
                /* discover ANCS service */
                AnccSvcDiscover(ancsCb.connHandle, pAncsAnccHdlList);
            } else {
                ancsCb.disc_retry_times++;
                if(ancsCb.disc_retry_times > 5) {
                    LOG_ERROR("[ANCS] retry times too much .. %d\n", ancsCb.disc_retry_times);
                } else {
                    ryos_delay_ms(100);
                    AnccSvcDiscover(ancsCb.connHandle, pAncsAnccHdlList);
                    ryos_delay_ms(100);
                }
            }
            break;
        case ATTC_READ_RSP:
        case ATTC_HANDLE_VALUE_NTF:
        case ATTC_HANDLE_VALUE_IND:
            ancsValueUpdate((attEvt_t *) pMsg);
            break;

        case ATTC_WRITE_RSP:    // write respose after Control point operation.
            {
                uint16_t handle_write_resp = ((attEvt_t *) pMsg)->handle;
                uint8_t handle_status = ((attEvt_t *) pMsg)->hdr.status;
                uint8_t* p_str_handle = "";
                bool isTargetCCCindex = true;
                if(handle_write_resp == ancsCb.hdlList[ancsDiscCfgList[0].hdlIdx]) {
                    isTargetCCCindex = false;
                    p_str_handle = "ancc_notify_source_service_changed";
                } else if(handle_write_resp == ancsCb.hdlList[ancsDiscCfgList[1].hdlIdx]) {
                    p_str_handle = "ancc_notify_source_control_point";
                } else if(handle_write_resp == ancsCb.hdlList[ancsDiscCfgList[2].hdlIdx]) {
                    p_str_handle = "ancc_notify_source_data_point";
                } else {
                    isTargetCCCindex = false;
                    p_str_handle = "unknows";
                }

                if(isTargetCCCindex
                    &&(handle_status != ATT_SUCCESS)
                    ) {
                    if((handle_status == ATT_ERR_WRITE)
                        ||(handle_status == ATT_ERR_HANDLE)
                        ) {
                        ancsSendEventThroughBleThread(ANCS_ACTION_SERVICE_CHANGED);
                    }
                    LOG_ERROR("[ANCS] ccc write failed handle:%d, %s, 0x%02X\n",
                            handle_write_resp,
                            p_str_handle,
                            handle_status);
                }
            }
            AnccOnGattcWriteResp(pAncsAnccHdlList, (attEvt_t *) pMsg);
            break;

        case ATTS_HANDLE_VALUE_CNF:
            break;

        case ATTS_CCC_STATE_IND:
//            LOG_DEBUG("[ANCS] ccc state ind value:%d handle:%d idx:%d\n", pMsg->ccc.value, pMsg->ccc.handle, pMsg->ccc.idx);
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
            ancsCb.discState = ANCS_DISC_CONNECTED;
            AnccConnOpen(pMsg->hdr.param, pAncsAnccHdlList);
            ancsCb.connHandle = pMsg->dm.connOpen.handle;
            ancsOnUpdateLoaclCCCConfig(p_stored_notify_whitelist->isAllOpend);
            break;

        case DM_CONN_CLOSE_IND:
            AnccConnClose();
            ancsCb.discState = ANCS_DISC_DISCONNECTED;
            ancsCb.disc_retry_times = 0;
            ancsBleBondStatusCheckTimerStop();
            if(((dmEvt_t*)pMsg)->connClose.reason == 0x3D) {
                  //err DB force re-init
                  LOG_ERROR("[ANCS] detect 0x3D try reinit BLE DB\n");
                  AppDbInit();
            }
            break;

        case DM_CONN_UPDATE_IND:        
            // LOG_DEBUG("[ANCS] DM_CONN_UPDATE_IND\n");
            break;

        case DM_SEC_PAIR_CMPL_IND:
            LOG_INFO("[ANCS] ------DM_SEC_PAIR_CMPL_IND user permitted\n");
            ancsOnServiceStatusChanged(true);
            break;

        case DM_SEC_PAIR_FAIL_IND:
            LOG_INFO("[ANCS] ------DM_SEC_PAIR_FAIL_IND user rejected\n");
            ancsOnServiceStatusChanged(false);
            ancsCb.discState = ANCS_DISC_CONNECTED;
            ancsSendEventThroughBleThread(ANCS_ACTION_DEVICE_UPLOAD);
            break;
        case DM_SEC_ENCRYPT_IND:
            LOG_DEBUG("[ANCS] ------DM_SEC_ENCRYPT_IND\n");
            ancsBleBondStatusCheckTimerStop();
            ancsOnServiceStatusChanged(true);
//            ancsCb.discState = ANCS_DISC_BLE_BIND_PERMITTED;
            break;

        case DM_SEC_ENCRYPT_FAIL_IND:
            LOG_DEBUG("[ANCS] ------DM_SEC_ENCRYPT_FAIL_IND\n");
            break;

        case DM_SEC_AUTH_REQ_IND:
            LOG_DEBUG("[ANCS] ------DM_SEC_AUTH_REQ_IND\n");
            AppHandlePasskey(&pMsg->dm.authReq);  
            break;

        case DM_SEC_LTK_REQ_IND:
            LOG_DEBUG("[ANCS] ------DM_SEC_LTK_REQ_IND\n");
            ancsBleBondStatusCheckTimerStart();
            break;

        case DM_SEC_PAIR_IND:
            LOG_DEBUG("[ANCS] ------DM_SEC_PAIR_IND\n");
            break;

        default:
            break;
    }
}


void AncsHandlerInit(wsfHandlerId_t handlerId)
{
    ancs_whitelist_init();
    ancsBleBondStatusCheckTimerInit();
    pAppDiscCfg = (appDiscCfg_t *) &ancsDiscCfg;
    AnccInit(handlerId, (anccCfg_t*)(&ancsAnccCfg));
    ancsCb.handlerId = handlerId;
    ancsCb.discState = 0;
}

void AncsStart(void)
{
    /* Register for app framework discovery callbacks */
    AppDiscRegister(ancsDiscCback);

    //
    // Register for ancc callbacks
    //
    AnccCbackRegister(ancsAnccAttrCback, ancsAnccNotifCback);
}


void ancsTryRejectCurrentCalling(void)
{
    if(m_ancs_tel_recorder.status == ancs_tel_recorder_status_calling)
    {
        LOG_DEBUG("[ANCS] ancsTryRejectCurrentCalling\n");
        ancsSendEventThroughBleThread(ANCS_ACTION_REJECT_TELE);
    }
}

void ancsOnRemoteRyUnbindFinished(void)
{
    //TODO FORCE DISCONNECT 
    ancsOnServiceStatusChanged(false);
}

void ancsOnRySetBondFinished(void)
{
    LOG_DEBUG("[ANCS] ancsOnRySetDeviceIOS\n");
    ancsSendEventThroughBleThread(ANCS_ACTION_RY_BIND_FINISHED);
}

void ancsOnRySetDeviceIOS(void)
{
    LOG_DEBUG("[ANCS] ancsOnRySetDeviceIOS\n");
    ancsSendEventThroughBleThread(ANCS_ACTION_RY_SET_IOS_APP);
}

void ancsOnRySetDeviceNotIOS(void)
{
    LOG_DEBUG("[ANCS] ancsOnRySetDeviceNotIOS\n");
    ancsSendEventThroughBleThread(ANCS_ACTION_RY_SET_NOT_IOS_APP);
}


uint32_t ancsWriteConfig(NotificationSetting* pList)
{
    uint32_t ret = RY_SUCC;
    uint8_t AncsActionCCCPending = 0;
    notification_whitelist_stored_t* p_ancs_new_whitelist = NULL;

    uint8_t secLevel = DmConnSecLevel(ancsCb.connHandle);
    bool isNowSecureLink = (secLevel==DM_SEC_LEVEL_ENC_LESC);

    if(pList->version < NOTIFICAION_SETTING_CFG_CURRENT_VERSION) {
        LOG_WARN("Skip ancs err config version 1\n");
        ret = RY_ERR_INVALID_TYPE;
        goto exit;
    }

    p_ancs_new_whitelist = ry_malloc(sizeof(notification_whitelist_stored_t));
    if(p_ancs_new_whitelist == NULL) {
        LOG_ERROR("[ANCS]-[%s] malloc failed", __FUNCTION__);
        ret = RY_ERR_NO_MEM;
        goto exit;
    }
    ry_memset(p_ancs_new_whitelist, 0, sizeof(notification_whitelist_stored_t));

    if(isNowSecureLink
        &&(p_stored_notify_whitelist->isAncsOpend == 0)
        ) {
        LOG_ERROR("[ANCS] ancs wrong pairing local status\n");
    }
    
    p_ancs_new_whitelist->version = pList->version;
    p_ancs_new_whitelist->isAncsOpend = isNowSecureLink?1:0;
    p_ancs_new_whitelist->isOthersEnabled = pList->is_others_open;
    p_ancs_new_whitelist->isAllOpend = pList->is_all_open;
    p_ancs_new_whitelist->tbl.curNum = pList->items_count;

    if(!isNowSecureLink) {
        if(p_ancs_new_whitelist->isAllOpend) {
            //trigger pair to makesure all enabled
            AncsActionCCCPending = ANCS_ACTION_RY_ENABLE_ALL;
        } else {
            //do nothing
        }
    } else {
        if(p_stored_notify_whitelist->isAllOpend) {
            if(!p_ancs_new_whitelist->isAllOpend) {   //update ccc
                AncsActionCCCPending = ANCS_ACTION_RY_SET_DISABLED;
            } else {
                //do nothing
            }
        } else {
            if(!p_ancs_new_whitelist->isAllOpend) {
                //do nothing
            } else {   //update ccc
                AncsActionCCCPending = ANCS_ACTION_RY_SET_ENABLED;
            }
        }
    }

    for(int i =0; i < pList->items_count; i++) {
        uint8_t* p_str = (void*)pList->items[i].app_id;
        uint32_t length = strlen((void*)p_str);
        uint8_t isOpened = pList->items[i].is_open?1:0;
        p_ancs_new_whitelist->tbl.records[i].isEnabled = isOpened;
        ry_memcpy(p_ancs_new_whitelist->tbl.records[i].appid, p_str, length);
    }

    ret = ancs_whitelist_save(p_ancs_new_whitelist);
    
    if(ancsCb.discState == ANCS_DISC_CONNECTED_NOT_IOS) {
    } else {
        if(AncsActionCCCPending != 0x0) {
            LOG_DEBUG("[ANCS] user operate 0x%02X\n, ", AncsActionCCCPending);
            ancsSendEventThroughBleThread(AncsActionCCCPending);
        }
    }


exit:
    ry_free(p_ancs_new_whitelist);
    return ret;
}

void ancsReadConfig(NotificationSetting* pList)
{
    pList->version = NOTIFICAION_SETTING_CFG_CURRENT_VERSION;
    pList->is_ios_ancs_open = p_stored_notify_whitelist->isAncsOpend==0?false:true;
    pList->is_others_open = p_stored_notify_whitelist->isOthersEnabled==0?false:true;
    pList->is_all_open = p_stored_notify_whitelist->isAllOpend;
    pList->items_count = p_stored_notify_whitelist->tbl.curNum;

    for(int i=0; i < pList->items_count; i++) {
        uint8_t const* p_str = p_stored_notify_whitelist->tbl.records[i].appid;
        uint32_t length = strlen((void*)p_str);
        bool isOpened = p_stored_notify_whitelist->tbl.records[i].isEnabled?true:false;
        ry_memcpy(pList->items[i].app_id, p_str, length);
        pList->items[i].is_open = isOpened;
    }
}

uint32_t ancsGetDiagnosisStatus(void)
{
    ancsDiagnosis_t t;

    const bdAddr_t default_addr_0xFF_addr = 
    {
        0xFF, 0xFF, 0xFF, 0xFF,0xFF,0xFF
    };

    uint8_t* p_stored_db_addr = AppDbGetFirstStoredMacAddr();
    bool isDBNotStored = BdaCmp(default_addr_0xFF_addr, p_stored_db_addr);

    bool isConnected = (ancsCb.connHandle == 0xFFFF)?false:true;
    bool isSecureLink = false;
    bool isConnectedAndroid = (ancsCb.discState == ANCS_DISC_CONNECTED_NOT_IOS);
    bool isConnectedIOS = (ancsCb.discState > ANCS_DISC_CONNECTED_NOT_IOS);
    uint16_t* p_hdl_local = ancsCb.hdlList;
    uint16_t* p_hdl_stored = AppDbGetFirstStoredHdlList();
    bool isLocalHdlValid = false;
    bool isStoredHdlValid = ((p_hdl_stored[ancsDiscCfgList[1].hdlIdx] != 0)
            &&(p_hdl_stored[ancsDiscCfgList[2].hdlIdx] != 0));

    if(isConnected) {
        uint8_t secLevel = DmConnSecLevel(ancsCb.connHandle);
        isSecureLink = (secLevel == DM_SEC_LEVEL_ENC_LESC);
        isLocalHdlValid = ((p_hdl_local[ancsDiscCfgList[1].hdlIdx] != 0)
            &&(p_hdl_local[ancsDiscCfgList[2].hdlIdx] != 0));
    }

    t.raw = 0x0;
    t.payload.status_ancs = ancsCb.discState;

    t.payload.isConnected = isConnected?1:0;
    t.payload.isSecureLink = isSecureLink?1:0;
    t.payload.isNotifyEnabled = p_stored_notify_whitelist->isAllOpend?1:0;
    t.payload.isAncsEnabled = p_stored_notify_whitelist->isAncsOpend?1:0;

    t.payload.isAnccPending = (anccCb.AttcWriteRespPendingHandle == 0)?0:1;
    t.payload.isDBstored = isDBNotStored?0:1;
    t.payload.isDBHandleStored = isStoredHdlValid?1:0;
    t.payload.isCurrentHandleValid = isLocalHdlValid?1:0;

    t.payload.isConnectedIOS = isConnectedIOS?1:0;
    t.payload.isConnectedAndroid = isConnectedAndroid?1:0;

    return t.raw;
}


void ancsDebugPort(void)
{
    ancsSendEventThroughBleThread(ANCS_ACTION_SERVICE_CHANGED);
}

#else

void ancsProcMsg(void* p_msg)
{
}

void AncsHandlerInit(wsfHandlerId_t handlerId)
{
}

void AncsStart(void)
{
}

void ancsTryRejectCurrentCalling(void)
{
}

void ancsOnRemoteRyUnbindFinished(void)
{

}

void ancsOnRySetBondFinished(void)
{
}

void ancsOnRySetDeviceIOS(void)
{
}
void ancsOnRySetDeviceNotIOS(void)
{
}



uint32_t ancsWriteConfig(NotificationSetting* pList)
{
}
void ancsReadConfig(NotificationSetting* pList)
{
}



#endif


