
#include "svc_wechat.h"

#include "wechat_api.h"

#include "ry_utils.h"
#include "ryos_timer.h"
#include "sensor_alg.h"
#include "ble_task.h"

extern uint8_t g_BLEMacAddress[];

static const uint32_t WechatUpdateIntervalMs = 30000;
static uint32_t weChatUpdateTimer = 0;
static uint16_t wechat_connHandle = 0xFFFF;
static uint32_t m_pedo_last_steps = 0;

/*! Application message type */
typedef union
{
    wsfMsgHdr_t     hdr;
    dmEvt_t         dm;
    attsCccEvt_t    ccc;
    attEvt_t        att;
} WechatMsg_t;

static uint32_t WechatApiGetCurrenPedoCounter(void)
{
    uint32_t steps =  alg_get_step_today();
    m_pedo_last_steps = steps;
    LOG_DEBUG("[wechat] wechat update steps:%d\n", steps);
    return steps;
}

static void WechatModifySteps(void)
{
    uint32_t now_steps = WechatApiGetCurrenPedoCounter();
    WechatServiceUpdatePedometerCounter(wechat_connHandle, now_steps);
}

static void WechatUpdateTimerInit(void)
{
    ry_timer_parm timer_para;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "wechat";
    weChatUpdateTimer = ry_timer_create(&timer_para);
}


static void WechatUpdateTimeoutHandle(void);



static void WechatUpdateTimerStart(void)
{
    ry_timer_start(weChatUpdateTimer,
        WechatUpdateIntervalMs,
        (ry_timer_cb_t)WechatUpdateTimeoutHandle,
        NULL);
}

static void WechatUpdateTimerStop(void)
{
    ry_timer_stop(weChatUpdateTimer);
}
static void WechatUpdateTimeoutHandle(void)
{
    WechatModifySteps();
    WechatUpdateTimerStop();
    WechatUpdateTimerStart();
    wakeup_ble_thread();
}

uint8_t WechatsReadCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                                   uint16_t offset, attsAttr_t *pAttr)
{
    uint8_t * p_target_handle = "";
    if(handle == WECHATS_SVC_CH_PEDO_MEASURE_VAL_HDL) {
        WechatModifySteps();
        LOG_INFO("[Wechat] Read Pedo result:%d\n", m_pedo_last_steps);
    } else if(handle == WECHATS_SVC_CH_AIRSYNC_READ_VAL_HDL) {
        LOG_DEBUG("[Wechat]WechatsReadCback on airsync\n");
    } else {
        LOG_DEBUG("[Wechat]WechatsReadCback on Others\n");
    }

    return 0;
}


uint8_t WechatsWriteCback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                       uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
    LOG_DEBUG("[Wechat]WechatsWriteCback\n");

    return 0;
}

void WechatServiceInit(void)
{
    WechatUpdateTimerInit();

    SvcWechatAddGroup();
    SvcWechatCbackRegister(WechatsReadCback, WechatsWriteCback);

    WechatServiceUpdateMacAddr(g_BLEMacAddress);
}

void WechatProcMsg(void* p_msg)
{
    WechatMsg_t* pMsg = (WechatMsg_t*)p_msg;
    switch(pMsg->hdr.event)
    {
        case ATTS_CCC_STATE_IND:
            if(pMsg->ccc.handle == WECHATS_SVC_CH_PEDO_MEASURE_CCC_HDL) {
                if(pMsg->ccc.value == ATT_CLIENT_CFG_NOTIFY) {
                    LOG_INFO("[Wechat] Wechat Char Notify Enabled\n");
                    WechatModifySteps();
                    WechatUpdateTimerStart();
                } else {
                    LOG_DEBUG("[Wechat] Wechat Char disabled\n");
                }
            }
            break;
        case DM_CONN_OPEN_IND:
            wechat_connHandle = pMsg->dm.connOpen.handle;
            break;
        case DM_CONN_CLOSE_IND:
            WechatUpdateTimerStop();
            wechat_connHandle = 0xFFFF;
            break;
    }
}







