/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_mcu_config.h"
#include "app_config.h"
#include "PhoneApp.pb.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

#if (RY_BLE_ENABLE == TRUE)

#include "ble_task.h"

//#include "reliable_xfer.h"
#include "r_xfer.h"
#include "ryos.h"
#include "ryos_timer.h"
#include "scheduler.h"
#include "ble_task.h"


/* Proto Buffer Related */
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "rbp.pb.h"
#include "rbp.h"
#include "dip.h"
#include "json.h"
#include "ry_ble.h"
#include "timer_management_service.h"
#include "device_management_service.h"
#include "surface_management_service.h"

#include "crc32.h"
#include "mible.h"
#include "ancs_api.h"
#include "am_hal_reset.h"
#include "app_api.h"
#include "fit_api.h"

/*********************************************************************
 * CONSTANTS
 */ 
#define RY_BLE_FLAG_JSON                               0x80


/*********************************************************************
* TYPEDEFS
*/

typedef struct {
    u8_t* data;
    u32_t len;
    void* usrdata;
} ry_ble_msg_t;


typedef struct {
    ry_ble_state_t     curState;
    ry_ble_conn_para_t curConnPara;
    ry_ble_init_done_t initDoneCb;
    int                idleTimer;
    u8_t               idle_timer_started;    
    u8_t               idle;
    int                disconnectTimer;
    int                hciAutoResetTimer;
} ry_ble_ctx_t;

typedef struct{
    uint16_t intervalMin;   //unit 1.25ms
    uint16_t intervalMax;   //unit 1.25ms
    uint16_t timeout;   //unit 10ms
    uint16_t slaveLatency;
    uint8_t type;
} ry_ble_conn_parm_cfg_t;

static const ry_ble_conn_parm_cfg_t conn_param_config[] =
{
    {   //  Large data transmission requires
        //  more conservative parameters to ensure stability
        .type = RY_BLE_CONN_PARAM_SUPER_FAST,
        .intervalMin = 6,
        .intervalMax = 12,
        .timeout = 500,
        .slaveLatency = 0,
    },
    {
        .type = RY_BLE_CONN_PARAM_FAST,
        .intervalMin = 12,
        .intervalMax = 24,
        .timeout = 400,
        .slaveLatency = 1,
    },
    {
        .type = RY_BLE_CONN_PARAM_MEDIUM,
        .intervalMin = 100,
        .intervalMax = 150,
        .timeout = 600,
        .slaveLatency = 0,
    },
    {
        .type = RY_BLE_CONN_PARAM_SLOW,
        .intervalMin = 150,
        .intervalMax = 170,
        .timeout = 600,
        .slaveLatency = 4,
    },
};

static const ry_ble_conn_parm_cfg_t conn_param_no_mtu_fast = {
    .intervalMin = 6,
    .intervalMax = 12,  // initial was 24
    .timeout = 500,
    .slaveLatency = 0,
};

static const ry_ble_conn_parm_cfg_t conn_param_no_mtu_fast_Android = {
    .intervalMin = 8,
    .intervalMax = 12,
    .timeout = 600,
    .slaveLatency = 0,
};

static const ry_ble_conn_parm_cfg_t conn_param_no_mtu_fast_IOS = {
    .intervalMin = 12,
    .intervalMax = 24,
    .timeout = 500,
    .slaveLatency = 0,
};


/*********************************************************************
 * LOCAL VARIABLES
 */
ryos_thread_t    *ry_ble_thread_handle;
ryos_thread_t    *ry_ble_tx_thread_handle;
ryos_thread_t    *ry_ble_ctrl_thread_handle;

ry_queue_t       *ry_ble_rxQ;
ry_queue_t       *ry_ble_txQ;
ry_queue_t       *ry_ble_ctrlQ;
ry_ble_ctx_t     ry_ble_ctx_v;



/*********************************************************************
 * LOCAL FUNCTIONS
 */
void ry_ble_disconnect_timer_start(void);
void ry_ble_disconnect_timer_stop(void);
void  ry_ble_autoreset_hci_timer_start(void);
void  ry_ble_autoreset_hci_timer_stop(void);


static void ryble_disconnect_start_fast_adv(void)
{
    /* Start fast reconnect adv mode */
    u8_t para = RY_BLE_ADV_TYPE_WAIT_CONNECT;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
}
/**
 * @brief   Callback function when one packet received from reliable channel.
 *          Note: The data buffer is malloc by reliable transfer module. We
 *          need to free the data buffer here.
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
void ry_ble_msgHandler(ry_ble_msg_t *msg)
{
    int i = 0;
    u8_t flag = ((uint32_t)msg->usrdata)&0xFF;
    int decryptRetryCnt = 0;

#if 0
    LOG_DEBUG("[ry_ble_msgHandler] Recevied len:%d \r\n", msg->len);
    ry_data_dump(msg->data, msg->len, ' ');
#endif

    /* CRC verification */
    u32_t len = msg->len - sizeof(u16_t);
    u16_t crc_val_cur = calc_crc16( msg->data, len);
    u16_t *crc_val_rd = (u16_t *) (&(msg->data[len]));

    if (crc_val_cur == *crc_val_rd) {
        //LOG_DEBUG("CRC check SUCC \n");
    } else {
        monitor_msg_t* mnt_msg = (monitor_msg_t*)msg->data;
        if ((flag == RY_BLE_TYPE_RX_MONITOR) && (msg->len == sizeof(monitor_msg_t))
           && (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_RX)
            ){
                *(uint32_t *)mnt_msg->dataAddr = 0;  
        } else {
            ry_data_dump(msg->data, len, ' ');
            LOG_ERROR("CRC check fail %x---%x\n",crc_val_cur,*crc_val_rd);
        }
        goto exit;
    }

    if (flag == RY_BLE_FLAG_JSON) {
        json_parser(msg->data, len, JSON_SRC_BLE);
        goto exit;
    }


    /* Decryption */
    //LOG_DEBUG("usrdata:0x%x, 0x%x..\n", (u8_t)msg->usrdata, (u32_t)msg->usrdata);
    if ((((uint32_t)msg->usrdata)&0xFF)) {
        //LOG_DEBUG("Decrypting...\n");

decrypt_retry:

        if (decryptRetryCnt < 2) { 
            mible_decrypt(msg->data, len, msg->data);
            if (msg->data[0] != 0x08 && msg->data[1] != 0x01) {
                LOG_ERROR("Decrypt Fail.\r\n");
                mible_encrypt(msg->data, len, msg->data);
                mible_transfer();
                decryptRetryCnt ++;
                goto decrypt_retry;
            }
        }
    }

#if 0
    LOG_DEBUG("[ry_ble_msgHandler] After decrypt len:%d \r\n", msg->len);
    ry_data_dump(msg->data, msg->len, ' ');
#endif

    /* Message dispatcher */
    rbp_parser(msg->data, len);

exit:
    /* Always don't forget to free the data buffer */
    ry_free(msg->data);
    msg->data = NULL;
    ry_free(msg);
}


void ry_ble_txCallback(ry_sts_t status, void* para)
{
    rbp_para_t *rbp_para = (rbp_para_t*)para;
    ry_ble_send_cb_t cb;
    void* arg;

    LOG_DEBUG("[ry_ble_txCallback] status:%x, para: 0x%x\r\n", status, (u32_t)para);

    /* Release the RBP buffer if any */
    if (rbp_para->rbpData) {
        ry_free(rbp_para->rbpData);
        rbp_para->rbpData = NULL;
    }

    /* Send the callback to upper layer */
    cb  = rbp_para->sentCb;
    arg = rbp_para->cbArg;

    ry_free((void*)rbp_para);

    if (cb) {
        cb(status, arg);
    }
}



/**
 * @brief   Callback function when one packet received from reliable channel.
 *          Note: The data buffer is malloc by reliable transfer module. We
 *          need to free the data buffer here.
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
void ry_ble_txHandler(ry_ble_msg_t *msg)
{

    rbp_para_t *para = (rbp_para_t*)msg->usrdata;

    if (para->type == RY_BLE_TYPE_REQUEST) {
        //rbp_send_req(para->cmd, msg->data, msg->len, para->isSec); 
        rbp_send_req_with_cb(para->cmd, msg->data, msg->len, para->isSec, ry_ble_txCallback, (void*)para);
    } else if (para->type == RY_BLE_TYPE_RESPONSE) {
        //rbp_send_resp(para->cmd, para->code, msg->data, msg->len, para->isSec);   
        rbp_send_resp_with_cb(para->cmd, para->code, msg->data, msg->len, para->isSec, ry_ble_txCallback, (void*)para);
    }
    else if (para->type == RY_BLE_TYPE_TX_MONITOR){
        monitor_msg_t* mnt_msg = (monitor_msg_t*)msg->data;
        if ((msg->len == sizeof(monitor_msg_t)) && (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_TX)){
            *(uint32_t *)mnt_msg->dataAddr = 0;  
        }
    }


exit:
    /* Always don't forget to free the data buffer */
    if (msg->data) {
        ry_free(msg->data);
        msg->data = NULL;
    }
    
    ry_free(msg);
}



void ry_ble_adv(u8_t type)
{
    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        AppAdvStop();
        wakeup_ble_thread();
        ryos_delay_ms(100);
    }

    LOG_DEBUG("[%s] Enter ADV mode: %d\r\n", __FUNCTION__, type);

    if (type == RY_BLE_ADV_TYPE_NORMAL) {
        RyeexAdvNormalSend();
    } else if (type == RY_BLE_ADV_TYPE_WAIT_CONNECT) {
        RyeexAdvWaitConnectSend();
    } else if (type == RY_BLE_ADV_TYPE_SLOW) {
        RyeexAdvSlowSend();
    }

    wakeup_ble_thread();
}


static uint32_t ry_ble_update_param_with_cfg(ry_ble_conn_parm_cfg_t const* p_cfg)
{
    return ble_updateConnPara(ry_ble_ctx_v.curConnPara.connId,
        p_cfg->intervalMin,
        p_cfg->intervalMin,
        p_cfg->timeout,
        p_cfg->slaveLatency);
}

void ry_ble_ctrlHandler(ry_ble_ctrl_msg_t *msg)
{
    switch(msg->msgType) {
        case RY_BLE_CTRL_MSG_ADV_START:
            ry_ble_adv(msg->data[0]);
            break;
        case RY_BLE_CTRL_MSG_DISCONNECT:
            LOG_INFO("[%s] Disconnect ble. \r\n", __FUNCTION__);
            AppConnClose(1);
            wakeup_ble_thread();
            ry_ble_ctx_v.curState = RY_BLE_STATE_DISCONNECTED;
            ry_ble_ctx_v.curConnPara.mtu = 23;
            break;
        case RY_BLE_CTRL_MSG_RESET:
            ble_hci_reset();
            wakeup_ble_thread();
            break;
        case RY_BLE_CTRL_MSG_CONN_UPDATE:
        {
            ry_ble_conn_parm_cfg_t const* p_cfg = NULL;
            
            switch(msg->data[0]) {
                case RY_BLE_CONN_PARAM_SUPER_FAST:  // 1
                    if (ry_ble_ctx_v.curConnPara.mtu <= 23) {
                        p_cfg = &conn_param_no_mtu_fast;
                    } else {
                        if (ry_ble_ctx_v.curConnPara.peerPhoneInfo == PEER_PHONE_TYPE_ANDROID)
                            p_cfg = &conn_param_no_mtu_fast_Android;
                        else
                            p_cfg = &conn_param_no_mtu_fast_IOS;
                    }                 
                    break;
                case RY_BLE_CONN_PARAM_FAST:        // 2
                    p_cfg = &conn_param_config[1];
                    break;
                case RY_BLE_CONN_PARAM_MEDIUM:      // 3
                    p_cfg = &conn_param_config[2];
                    break;
                default:    // RY_BLE_CONN_PARAM_SLOW
                    p_cfg = &conn_param_config[3];
                    break;
            }
           
            if(p_cfg != NULL) {
                ry_ble_update_param_with_cfg(p_cfg);
            } else {
                LOG_DEBUG("[%s] conn param cfg invalid %d \n", __FUNCTION__, msg->data[0]);
            }
        }
            break;
        case RY_BLE_CTRL_MSG_CONN_UPDATE_WITH_CFG:
        {
            ry_ble_conn_parm_cfg_t param = {0};
            if(ry_ble_ctx_v.curConnPara.peerPhoneInfo == PEER_PHONE_TYPE_ANDROID) {
                param.intervalMin = 6;
                param.intervalMax = msg->data[0];
            } else {
                param.intervalMin = 12;
                param.intervalMax = 24;
            }
            param.timeout = 200;
            param.slaveLatency = 0;
            ry_ble_update_param_with_cfg(&param);
        }
            break;
        default:
            break;
    }

exit:
    /* Always don't forget to free the data buffer */
    ry_free(msg);
}



void ry_ble_ctrl_msg_send(u32_t msg_type, int len, void* data)
{
    ry_sts_t status = RY_SUCC;
    ry_ble_ctrl_msg_t *p = (ry_ble_ctrl_msg_t*)ry_malloc(sizeof(ry_ble_ctrl_msg_t)+len);
    if (NULL == p) {

        LOG_ERROR("[ry_ble_ctrl_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }

    p->msgType = msg_type;
    p->len = len;
    if (len>0) {
        ry_memcpy(p->data, data, len);
    }

    if (RY_SUCC != (status = ryos_queue_send(ry_ble_ctrlQ, &p, 4))) {
        LOG_ERROR("[ry_ble_ctrl_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
    }        
}



// iOS退出后台的时候需要模拟一条pb命令
void r_xfer_on_appKilled_recived(void)
{
    const uint8_t simulate_rbp_msg[] = {
        0x08, 0x01, 0x10, 0x8b, 0x07,
        0x18, 0x00, 0x22, 0x0a, 0x08,
        0x01, 0x10, 0x01, 0x1a, 0x04,
        0x08, 0x01, 0x10, 0x03,
    };
    uint32_t status = RY_SUCC;
    uint8_t* p_data = NULL;
    ry_ble_msg_t *p_msg = NULL;
    uint16_t crc = 0;
    uint32_t length = sizeof(simulate_rbp_msg)/sizeof(uint8_t);

    p_msg = (ry_ble_msg_t*)ry_malloc(sizeof(ry_ble_msg_t));
    if(p_msg == NULL) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
        goto exit;
    }
    ry_memset(p_msg, 0, sizeof(ry_ble_msg_t));

    p_data = ry_malloc(length + 4);
    if(p_data == NULL) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
        goto exit;
    }
    ry_memcpy(p_data, simulate_rbp_msg, length);

    crc = calc_crc16(p_data, length);
    *((uint16_t*)&p_data[length]) = crc;

    p_msg->len     = length + 2;
    p_msg->data    = p_data;
    p_msg->usrdata = NULL;

    status = ryos_queue_send(ry_ble_rxQ, &p_msg, sizeof(ry_ble_msg_t));
    if(status != RY_SUCC) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_QUEUE_FULL);
        goto exit;
    }

exit:
    if(status != RY_SUCC) {
        if(p_msg != NULL) {
            FREE_PTR(p_msg);
        }

        if(p_data != NULL) {
            FREE_PTR(p_data);
        }

        LOG_ERROR("[r_xfer_onReceived] app killed err status:%d\n", status);

    } else {
        LOG_DEBUG("[r_xfer_onReceived] app killed\n");
    }
}

void ry_ble_msg_send(u8_t* data, u32_t len, void* usrdata, u8_t fTxMsg)
{
    ry_sts_t status = RY_SUCC;
    ry_ble_msg_t *p = (ry_ble_msg_t*)ry_malloc(sizeof(ry_ble_msg_t));
    if (NULL == p) {

        LOG_ERROR("[ry_ble_onReliableReceived]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }

    p->len     = len;
    p->data    = data;
    p->usrdata = usrdata;

    //LOG_INFO("[ry_ble_msg_send], 0x%x, 0x%x, txrx:%d\n", (u32_t)usrdata, (u32_t)p->usrdata, fTxMsg);
    int is_tx_monitor = 0;
    int is_rx_monitor = 0;
    monitor_msg_t* mnt_msg = (monitor_msg_t*)p->data;
    if (((((uint32_t)p->usrdata)&0xFF) == RY_BLE_TYPE_RX_MONITOR) && (p->len == sizeof(monitor_msg_t)) && \
        (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_RX)){
        is_tx_monitor = 1;
    }

    if ((p->len == sizeof(monitor_msg_t)) && (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_TX) &&\
        (*(u32_t*)p->usrdata == (RY_BLE_TYPE_TX_MONITOR << 16) | RY_BLE_TYPE_TX_MONITOR)){
        is_rx_monitor = 1;  
    }
    
    if (!(is_tx_monitor || is_rx_monitor)){
        if (ry_ble_is_connect_idle_mode()) {
            ry_ble_connect_normal_mode();
        }
    }

    if (fTxMsg) {
        if (RY_SUCC != (status = ryos_queue_send(ry_ble_txQ, &p, 4))) {
            LOG_ERROR("[ry_ble_onReliableReceived]: TX Add to Queue fail. status = 0x%04x\n", status);
            ry_free(data);
            ry_free((void*)p);
        }
    } else {
        if (RY_SUCC != (status = ryos_queue_send(ry_ble_rxQ, &p, 4))) {
            LOG_ERROR("[ry_ble_onReliableReceived]: RX Add to Queue fail. status = 0x%04x\n", status);
            ry_free(data);
            ry_free((void*)p);
        }
    }
}




ry_sts_t ry_ble_txMsgReq(CMD cmd, 
            ry_ble_req_type_t type, 
            u32_t rspCode, 
            u8_t* data, 
            u32_t len, 
            u8_t isSec, 
            ry_ble_send_cb_t cb, 
            void* arg)
{
    rbp_para_t* usrdata = (rbp_para_t*)ry_malloc(sizeof(rbp_para_t));
    if (usrdata == NULL) {
        LOG_ERROR("[ry_ble_txMsgReq] No mem.\r\n");
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
    }
    
    ry_memset(usrdata, 0, sizeof(rbp_para_t));
    usrdata->cmd     = cmd;
    usrdata->type    = type;
    usrdata->code    = rspCode;
    usrdata->isSec   = isSec;
    usrdata->sentCb  = cb;
    usrdata->cbArg   = arg;
    usrdata->rbpData = NULL;
    
    ry_ble_msg_send(data, len, (void*)usrdata, 1);

    return RY_SUCC;
}



void ry_ble_onReliableReceived(u8_t* data, u32_t len, void* usrdata)
{
    ry_ble_msg_send(data, len, usrdata, 0);
    ry_ble_idle_timer_stop();
    if (ry_ble_is_connect_idle_mode()) {
        ry_ble_connect_normal_mode();
    }
    
    ry_ble_idle_timer_start();
}



/**
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int ry_ble_thread(void* arg)
{
    ry_ble_msg_t *rxMsg;
    ry_sts_t status = RY_SUCC;

    LOG_DEBUG("[ry_ble_thread] Enter.\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_RY_BLE);
#endif
    
    /* Create the RYBLE RX queue */
    status = ryos_queue_create(&ry_ble_rxQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_thread]: RX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    while (1) {

        status = ryos_queue_recv(ry_ble_rxQ, &rxMsg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_ble_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {
            ry_ble_msgHandler(rxMsg);
        }

        ryos_delay_ms(10);
    }
}

/**
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int ry_ble_tx_thread(void* arg)
{
    ry_ble_msg_t *txMsg;
    ry_sts_t status = RY_SUCC;

    LOG_DEBUG("[ry_ble_tx_thread] Enter.\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_RY_BLE);
#endif
    
    /* Create the RYBLE RX queue */
    status = ryos_queue_create(&ry_ble_txQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_tx_thread]: TX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    while (1) {

        status = ryos_queue_recv(ry_ble_txQ, &txMsg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_ble_tx_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {
            ry_ble_txHandler(txMsg);
        }

        ryos_delay_ms(10);
    }
}


/*
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int ry_ble_ctrl_thread(void* arg)
{
    ry_ble_ctrl_msg_t *ctrlMsg;
    ry_sts_t status = RY_SUCC;

    LOG_DEBUG("[ry_ble_ctrl_thread] Enter.\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_RY_BLE);
#endif
    
    /* Create the RYBLE RX queue */
    status = ryos_queue_create(&ry_ble_ctrlQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_tx_thread]: TX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    while (1) {

        status = ryos_queue_recv(ry_ble_ctrlQ, &ctrlMsg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_ble_tx_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {
            ry_ble_ctrlHandler(ctrlMsg);
        }

        ryos_delay_ms(10);
    }
}




/**
 * @brief   Callback function from ble stack.
 *
 * @param   None
 *
 * @return  None
 */
void ry_ble_stack_callback(u16_t evt, void* para)
{
    ble_stack_connected_t *pConnected;
    uint8_t conn_param_type;
    switch (evt) {
        case BLE_STACK_EVT_INIT_DONE:
            LOG_DEBUG("[%s] Ble init Done. \r\n", __FUNCTION__);
            ry_ble_ctx_v.curState = RY_BLE_STATE_INITIALIZED;
            ryos_delay_ms(100);
            ry_sched_msg_send(IPC_MSG_TYPE_BLE_INIT_DONE, 0, NULL);

            ry_ble_autoreset_hci_timer_stop();
            ry_ble_autoreset_hci_timer_start();
            break;

        case BLE_STACK_EVT_ADV_STARTED:
            //ry_ble_ctx_v.curState = RY_BLE_STATE_ADV;
            LOG_DEBUG("[%s] Adv started. \r\n", __FUNCTION__);
            //ry_ble_ctx_v.initDoneCb();
            break;

        case BLE_STACK_EVT_ADV_STOPED:
            //ry_ble_ctx_v.curState = RY_BLE_STATE_IDLE;
            LOG_DEBUG("[%s] Adv stopped. \r\n", __FUNCTION__);
            break;

        case BLE_STACK_EVT_CONNECTED:
            LOG_DEBUG("[%s] Connected. \r\n", __FUNCTION__);
            ry_ble_disconnect_timer_stop();

            ry_ble_autoreset_hci_timer_stop();


            pConnected = (ble_stack_connected_t*)para;
            ry_ble_ctx_v.curState                 = RY_BLE_STATE_CONNECTED;
            ry_ble_ctx_v.curConnPara.connId       = pConnected->connID;
            ry_ble_ctx_v.curConnPara.connInterval = pConnected->connInterval;
            ry_ble_ctx_v.curConnPara.timeout      = pConnected->timeout;
            ry_ble_ctx_v.curConnPara.slaveLatency = pConnected->latency;
            ry_ble_ctx_v.curConnPara.peerAppLifecycle = PEER_PHONE_APP_LIFECYCLE_DESTROYED;
            ry_ble_ctx_v.curConnPara.mtu          = 23; //ATT MTU DEFAULT
            ry_memcpy(ry_ble_ctx_v.curConnPara.peerAddr, pConnected->peerAddr, 6);
            ry_sched_msg_send(IPC_MSG_TYPE_BLE_CONNECTED, 0, NULL);

            conn_param_type = RY_BLE_CONN_PARAM_FAST;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);

            ry_ble_ctx_v.idle = 0;
            ry_ble_idle_timer_start();
            break;

        case BLE_STACK_EVT_DISCONNECTED:
            LOG_DEBUG("[%s] Disconnected. \r\n", __FUNCTION__);
            ry_ble_ctx_v.curState = RY_BLE_STATE_DISCONNECTED;
            ry_memset((void*)&ry_ble_ctx_v.curConnPara, 0, sizeof(ry_ble_conn_para_t));
            ry_sched_msg_send(IPC_MSG_TYPE_BLE_DISCONNECTED, 0, NULL);

            extern u32_t factory_test_start;
            if (dev_mfg_state_get() >= DEV_MFG_STATE_SEMI && dev_mfg_state_get() < DEV_MFG_STATE_DONE && factory_test_start) {
                add_reset_count(MAGIC_RESTART_COUNT);
                am_hal_reset_poi();
            }
            ry_ble_ctx_v.idle = 0;

            ry_ble_idle_timer_stop();
            ryble_disconnect_start_fast_adv();
            ry_ble_disconnect_timer_start();
            ry_ble_autoreset_hci_timer_start();
            break;

        case BLE_STACK_EVT_CONN_PARA_UPDATED:
            pConnected = (ble_stack_connected_t*)para;
            ry_ble_ctx_v.curConnPara.connId       = pConnected->connID;
            ry_ble_ctx_v.curConnPara.connInterval = pConnected->connInterval;
            ry_ble_ctx_v.curConnPara.timeout      = pConnected->timeout;
            ry_ble_ctx_v.curConnPara.slaveLatency = pConnected->latency;
            ry_ble_ctx_v.curState                 = RY_BLE_STATE_CONNECTED;

            LOG_DEBUG("[BLE] Conn Para Updated. Interval = %dms, Timeout = %ds, Latency = %d\r\n",
                ((ry_ble_ctx_v.curConnPara.connInterval*5)>>2),
                ry_ble_ctx_v.curConnPara.timeout/100,
                ry_ble_ctx_v.curConnPara.slaveLatency);
            break;

        case BLE_STACK_EVT_MTU_GOT:
        {
            ble_stack_mtu_updated_t* p_mtu_updated = (ble_stack_mtu_updated_t*)para;
            ry_ble_ctx_v.curConnPara.mtu = p_mtu_updated->mtu;
            LOG_DEBUG("Negotiated MTU %d on handle:%d\n",
                p_mtu_updated->mtu, p_mtu_updated->connID);
        }
            break;

        default:
            break;
    }
}

/**
 * @brief   API to get BLE status
 *
 * @param   None
 *
 * @return  None
 */
u8_t ry_ble_get_state(void)
{
    return ry_ble_ctx_v.curState;
}


/**
 * @brief   API to get current connect intervals
 *
 * @param   None
 *
 * @return  None
 */
u16_t ry_ble_get_connInterval(void)
{
    return ry_ble_ctx_v.curConnPara.connInterval;
}

void ry_ble_set_peer_phone_type(u8_t type)
{
    ry_ble_ctx_v.curConnPara.peerPhoneInfo = type;
}

u8_t ry_ble_get_peer_phone_type(void)
{
    return ry_ble_ctx_v.curConnPara.peerPhoneInfo;
}

void ry_ble_set_peer_phone_app_lifecycle(u8_t type)
{
    ry_ble_ctx_v.curConnPara.peerAppLifecycle = type;
}


u8_t ry_ble_get_peer_phone_app_lifecycle(void)
{
    return ry_ble_ctx_v.curConnPara.peerAppLifecycle;
}

uint32_t ry_ble_get_cur_mtu(void)
{
    return ry_ble_ctx_v.curConnPara.mtu;
}


ry_sts_t ry_ble_set_phone_info(u8_t * data, u32_t size)
{
    PhoneAppInfo result_data = {0};
    ry_sts_t status = RY_SUCC;
    u32_t code = RBP_RESP_CODE_SUCCESS;

    pb_istream_t stream = pb_istream_from_buffer(data, size);
    
    pb_decode(&stream, PhoneAppInfo_fields, &result_data);
    LOG_DEBUG("[%s] set phone type --- %d\n", __FUNCTION__, result_data.type);

    ry_ble_set_peer_phone_type(result_data.type);
    
    status = rbp_send_resp(CMD_DEV_SET_PHONE_APP_INFO, code, NULL, 0, 1);
    //status = ble_send_response(CMD_DEV_SET_PHONE_APP_INFO, code, NULL, 0, 1,
    //                    NULL, NULL);

    if((result_data.type == PEER_PHONE_TYPE_IOS)
        &&(result_data.has_appType)
        &&(result_data.appType == PhoneAPPType_RyHeyPlus)
        )
    {
        ancsOnRySetDeviceIOS();
    }
    else if(result_data.type == PEER_PHONE_TYPE_ANDROID)
    {
        ancsOnRySetDeviceNotIOS();
    }

exit:  
    return status;
}


void ryble_idle_timeout_handler(void* arg)
{
    if (ry_ble_ctx_v.idle == 0) {
        LOG_DEBUG("[ryble_idle_timeout_handler] Enter idle connection mode.\r\n");
        uint8_t conn_param_type = RY_BLE_CONN_PARAM_SLOW;
        ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
        ry_ble_ctx_v.idle = 1;
    }
    ry_ble_ctx_v.idle_timer_started = 0;
}    


void ry_ble_connect_normal_mode(void)
{
    if (ry_ble_ctx_v.idle == 1) {
        LOG_DEBUG("[ry_ble_connect_normal_mode] Enter normal connection mode.\r\n");
        ry_ble_ctx_v.idle = 0;
        ry_ble_idle_timer_stop();
        uint8_t conn_param_type = RY_BLE_CONN_PARAM_FAST;
        ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
    }
}


void ryble_disconnect_timeout_handler(void* arg)
{
    /* Back to normal adv mode */
    u8_t para = RY_BLE_ADV_TYPE_NORMAL;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
}


void ryble_autoreset_hci_timeout_handler(void* arg)
{
    LOG_DEBUG("[ry_ble] auto reset hci timeout\n");
    /* Back to normal adv mode */
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_RESET, 0, NULL);
}


void ry_ble_disconnect_timer_start(void)
{
    if (ry_ble_ctx_v.disconnectTimer) {
        ry_timer_start(ry_ble_ctx_v.disconnectTimer, 60000, ryble_disconnect_timeout_handler, NULL);
    }
}

void ry_ble_disconnect_timer_stop(void)
{
   if (ry_ble_ctx_v.disconnectTimer) {
       ry_timer_stop(ry_ble_ctx_v.disconnectTimer);
   }
}

void  ry_ble_autoreset_hci_timer_start(void)
{
    if(ry_ble_ctx_v.hciAutoResetTimer) {
       ry_timer_start(ry_ble_ctx_v.hciAutoResetTimer, (10*60*1000), ryble_autoreset_hci_timeout_handler, NULL);
    }
}

void ry_ble_autoreset_hci_timer_stop(void)
{
    if(ry_ble_ctx_v.hciAutoResetTimer) {
       ry_timer_stop(ry_ble_ctx_v.hciAutoResetTimer);
    }
}

u8_t ryble_idle_timer_is_running(void)
{
    return ry_ble_ctx_v.idle_timer_started;
}

u8_t ry_ble_is_connect_idle_mode(void)
{
    return ry_ble_ctx_v.idle;
}



void ry_ble_idle_timer_start(void)
{
    ry_ble_ctx_v.idle_timer_started = 1;
    if (ry_ble_ctx_v.idleTimer) {
        ry_timer_start(ry_ble_ctx_v.idleTimer, 30000, ryble_idle_timeout_handler, NULL);
    }
}

void ry_ble_idle_timer_stop(void)
{
   if (ry_ble_ctx_v.idleTimer) {
       ry_timer_stop(ry_ble_ctx_v.idleTimer);
   }
   ry_ble_ctx_v.idle_timer_started = 0;
}




/**
 * @brief   API to init BLE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_ble_init(ry_ble_init_done_t initDoneCb)
{
    ry_sts_t status;
    ryos_threadPara_t para;
    ry_timer_parm timer_para;
    u32_t sec;
    u32_t usrdata = 3;

    /* Init RY BLE context */
    ry_ble_ctx_v.curState   = RY_BLE_STATE_IDLE;
    ry_ble_ctx_v.initDoneCb = initDoneCb;
    ry_memset((void*)&ry_ble_ctx_v.curConnPara, 0, sizeof(ry_ble_conn_para_t));

    /* Init ble module */
    status = ble_init(ry_ble_stack_callback);
    LOG_DEBUG("[ry_ble_init] free heap size1:%d\r\n", ryos_get_free_heap());

    /* Init Reliable Transfer Module */
    //r_xfer_init(ble_reliableUnsecureSend, ry_ble_on_reliableReceive);

    r_xfer_init();
    LOG_DEBUG("[ry_ble_init] free heap size2:%d\r\n", ryos_get_free_heap());
    sec = 1;
    r_xfer_instance_add(R_XFER_INST_ID_BLE_SEC, (hw_write_fn_t)ble_reliableSend, ry_ble_onReliableReceived, (void*)sec);
    sec = 0;
    r_xfer_instance_add(R_XFER_INST_ID_BLE_UNSEC, (hw_write_fn_t)ble_reliableUnsecureSend, ry_ble_onReliableReceived, (void*)sec);
    sec = RY_BLE_FLAG_JSON;
    r_xfer_instance_add(R_XFER_INST_ID_BLE_JSON, (hw_write_fn_t)ble_jsonSend, ry_ble_onReliableReceived, (void*)sec);

    /* Create the timer */
    timer_para.timeout_CB = ryble_idle_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "ble_idle_timer";
    ry_ble_ctx_v.idleTimer = ry_timer_create(&timer_para);

    timer_para.timeout_CB = ryble_disconnect_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "ble_discon_timer";
    ry_ble_ctx_v.disconnectTimer = ry_timer_create(&timer_para);
    
    timer_para.timeout_CB = ryble_autoreset_hci_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "ble_reset_timer";
    ry_ble_ctx_v.hciAutoResetTimer = ry_timer_create(&timer_para);

    /* Start RY BLE Thread */
    strcpy((char*)para.name, "ry_ble_Rthread");
    para.stackDepth = 768;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 4;
    status = ryos_thread_create(&ry_ble_thread_handle, ry_ble_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_init]: Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }

    /* Start RY BLE Thread */
    strcpy((char*)para.name, "ry_ble_Tthread");
    para.stackDepth = 512;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 4;
    status = ryos_thread_create(&ry_ble_tx_thread_handle, ry_ble_tx_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_init]: Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }

    /* Start RY BLE Thread */
    strcpy((char*)para.name, "ry_ble_thread");
    para.stackDepth = 400;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 4;
    status = ryos_thread_create(&ry_ble_ctrl_thread_handle, ry_ble_ctrl_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_init]: Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }

    LOG_DEBUG("[ry_ble_init] free heap size3:%d\r\n", ryos_get_free_heap());

    return status;
}


/***********************************Test*****************************************/

#ifdef RY_BLE_TEST


#endif  /* RY_BLE_TEST */


#endif /* (RY_BLE_ENABLE == TRUE) */


