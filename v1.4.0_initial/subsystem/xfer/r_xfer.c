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

#include "ry_type.h"
#include "ry_utils.h"
#include "app_config.h"
#include "r_xfer.h"
#include "ry_ble.h"

#include "pt.h"
#include "ryos.h"
#include "ryos_timer.h"

#include "ry_hal_inc.h"
#include "rbp.h"

/* Platform specified */
#include "ble_task.h"
#include "ble_task.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

#include "timer_management_service.h"

/*********************************************************************
 * MACROS
 */




#define R_XFER_STATE_IDLE             0
#define R_XFER_STATE_BUSY             1
#define R_XFER_STATE_RX               2
#define R_XFER_STATE_RX_RESEND        3
#define R_XFER_STATE_RX_DONE          4
#define R_XFER_STATE_TX_WAIT_ACK      5
#define R_XFER_STATE_TX_SENDING       6

#define R_XFER_EVENT_RX_REQ           0
#define R_XFER_EVENT_RX_DATA          1
#define R_XFER_EVENT_TX_REQ           2
#define R_XFER_EVENT_TX_ACK           3
#define R_XFER_EVENT_RX_TIMEOUT       4
#define R_XFER_EVENT_TX_TIMEOUT       5
#define R_XFER_EVENT_DISCONNECT       6

#define R_XFER_EVENT_APP_KILLED       0

#define R_XFER_MAX_RETRY_CNT          2
#define R_XFER_TX_MAX_RETRY_CNT       1     //set to 0 disable tx failed retry
#define R_XFER_TX_Q_MAX_RETRY_CNT     0     //set to 0 disable queue retry



/*********************************************************************
 * CONSTANTS
 */
#define R_XFER_RX_QUEUE_SIZE          10
#define R_XFER_TX_QUEUE_SIZE          10
#define R_XFER_MSG_QUEUE_SIZE         250

#define R_XFER_THREAD_STACK_SIZE      384
#define R_XFER_THREAD_PRIORITY        4


#define R_XFER_ATT_CHAR_SIZE          244 // 154 => 244   // <= RYEEXS_CHAR_MAX_DATA

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    uint16_t          amount;
    uint16_t          cur_sn;
    uint16_t          cur_pack_size;    //每个sn携带的payload长度 <= R_XFER_ATT_CHAR_SIZE
    uint8_t           mode;
    uint8_t           type;
    uint32_t          last_bytes;
    uint16_t          temp_last_sn;
    uint8_t           timeout_flag;
    uint8_t           retry_cnt;
    uint16_t          lost_sn_list[8];
    uint8_t           n_lost_sn_list;
    uint32_t          total_len;
    uint16_t          tx_sn;
    uint32_t          timer_id;
    ryos_semaphore_t* tx_sem;
    uint8_t*          pdata;
    uint32_t          start_time;
    uint32_t          end_time;
    uint8_t*          pack_recv_flag;
} r_xfer_sess_t;




//#pragma pack (push,1)

typedef struct {
    int              id;
    hw_write_fn_t    write;
    r_xfer_recv_cb_t readCb;
    r_xfer_send_cb_t writeDoneCb;
    void*            usrData;
    void*            writeUsrdata;

    r_xfer_sess_t*   session_tx;
    int              state_tx;
    int              lastTxDuration;
    
    r_xfer_sess_t*   session_rx;
    int              state_rx;
    int              lastRxDuration;

#ifdef R_XFER_STATISTIC_ENABLE
    int             writeSuccCnt;
    int             writeFailCnt;
    int             readSuccCnt;
    int             readFailCnt;
    int             sessCnt;
#endif

} r_xfer_inst_t;



typedef struct {
    uint32_t curNum;
    r_xfer_inst_t instance[MAX_R_XFER_INSTANCE_NUM];
} r_xfer_instTbl_t;




typedef struct {
    ryos_mutex_t     *mutex;
    ryos_thread_t    *thread;
    ry_queue_t       *txQ;
    ry_queue_t       *rxQ;
    ry_queue_t       *msgQ;
    r_xfer_instTbl_t tbl;
    
} r_xfer_t;

//#pragma pack (pop)

#pragma pack(1)
// Internal Message Format
typedef struct {
    r_xfer_inst_t*      instance;
    int                 flag;
    int                 len;
    uint8_t*            txData;
    r_xfer_send_cb_t    txCb;
    void*               txArg;
    uint32_t            ttl;        //send to queue and retry remaining times
    uint8_t             data[1];    //malloc extra size to use it as rx array
} r_xfer_msg_t;


/**
 * @brief Definitaion for frame control format of the R_XFER.
 */
typedef struct {
    uint8_t mode;
    uint8_t type;
    uint8_t arg[4];
} r_xfer_fctrl_t;

/**
 * @brief Definitaion for frame format of R_XFER.
 */
typedef struct {
    uint16_t sn;
    union {
        uint8_t           data[(R_XFER_ATT_CHAR_SIZE-2)];
        r_xfer_fctrl_t    ctrl;
    } f;
} r_xfer_frame_t;

#pragma pack()


typedef void ( *r_xfer_stateHandler_t )(r_xfer_inst_t* inst, uint8_t* data, int len, r_xfer_msg_t* pMsg); 


/** @brief  Reliable transfer state machine */
typedef struct
{
    uint8_t   curState;                /*! The current state of mi service */
    uint16_t  event;                   /*! The event ID */
    r_xfer_stateHandler_t handler; /*! The corresponding event handler */
} r_xfer_stateMachine_t;

typedef struct
{
    int instID;
    uint8_t* data;
    int len;
    r_xfer_send_cb_t sendCb;
    void* usrdata;
}r_xfer_sent_fifo_t;



/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void r_xfer_rxReqHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);
static void r_xfer_rxDataHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);
static void r_xfer_rxTimeoutHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);
static void r_xfer_rxBusyHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);

static void r_xfer_txReqHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);
static void r_xfer_txAckHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);
static void r_xfer_txTimeoutHandler(r_xfer_inst_t* inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);
static void r_xfer_txBusyHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg);


static void r_xfer_session_destroy(r_xfer_sess_t** pp_session);
static void r_xfer_msg_request_abort(r_xfer_msg_t* p_msg, uint32_t status);
static r_xfer_inst_t *r_xfer_instance_search(int id);
static void r_xfer_tx_check_queue(r_xfer_inst_t* inst);


/*********************************************************************
 * LOCAL VARIABLES
 */
r_xfer_t         r_xfer_v;

uint32_t  g_test_timestamp;

int g_r_xfer_busy = 0;


static const r_xfer_stateMachine_t r_xfer_stateMachine[] = 
{
     /*  current state,                       event,                       handler    */

#if 1 //region session_rx and state_rx
    { R_XFER_STATE_IDLE,               R_XFER_EVENT_RX_REQ,            r_xfer_rxReqHandler },
    { R_XFER_STATE_RX,                 R_XFER_EVENT_RX_DATA,           r_xfer_rxDataHandler },
    { R_XFER_STATE_RX,                 R_XFER_EVENT_RX_TIMEOUT,        r_xfer_rxTimeoutHandler },
    { R_XFER_STATE_RX,                 R_XFER_EVENT_RX_REQ,            r_xfer_rxBusyHandler },

    { R_XFER_STATE_RX_RESEND,          R_XFER_EVENT_RX_DATA,           r_xfer_rxDataHandler },
    { R_XFER_STATE_RX_RESEND,          R_XFER_EVENT_RX_TIMEOUT,        r_xfer_rxTimeoutHandler },
    { R_XFER_STATE_RX_RESEND,          R_XFER_EVENT_RX_REQ,            r_xfer_rxBusyHandler },
#endif

#if 1 //region session_tx and state_tx
    { R_XFER_STATE_IDLE,               R_XFER_EVENT_TX_REQ,            r_xfer_txReqHandler },
    { R_XFER_STATE_TX_WAIT_ACK,        R_XFER_EVENT_TX_ACK,            r_xfer_txAckHandler },
    { R_XFER_STATE_TX_WAIT_ACK,        R_XFER_EVENT_TX_TIMEOUT,        r_xfer_txTimeoutHandler },
    { R_XFER_STATE_TX_WAIT_ACK,        R_XFER_EVENT_TX_REQ,            r_xfer_txBusyHandler },
#endif
};


static void r_xfer_sn_buffer_set_bit(uint8_t* p_buffer, uint16_t sn)
{
    uint32_t buffer_index;
    uint8_t buffer_mask;
    buffer_index = (sn-1)/8;
    buffer_mask = 0x1 << ((sn-1)%8);
    p_buffer[buffer_index] &= (0xff ^ buffer_mask);
}

//return is already recived
static bool r_xfer_sn_buffer_check_bit_recived(uint8_t* p_buffer, uint16_t sn)
{
    uint32_t buffer_index;
    uint8_t buffer_mask;
    buffer_index = (sn-1)/8;
    buffer_mask = 0x1 << ((sn-1)%8);
    bool isRecived = ((p_buffer[buffer_index]&buffer_mask) == 0);
    return isRecived;
}

#define R_XFER_SET_STATE_TX(i, s)       (i->state_tx = s)
#define R_XFER_GET_STATE_TX(i)          (i->state_tx)
#define R_XFER_SET_STATE_RX(i, s)       (i->state_rx = s)
#define R_XFER_GET_STATE_RX(i)          (i->state_rx)
   
void r_xfer_BleDisconnectHandler(void)

{
    r_xfer_inst_t* inst = NULL;
    r_xfer_msg_t *p = NULL;//
    ry_sts_t status = RY_SUCC;
    int i = 0;

    /*for(i = 0; i < R_XFER_INST_ID_UART; i++){
        inst = r_xfer_instance_search( i );
        p = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t));
        
        inst->session->timeout_flag = 1;
        inst->session->retry_cnt ++;
        p->instance = inst;
        p->len = 0;
        p->flag = R_XFER_FLAG_DISCONNECT;

        if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, &p, 4))) {
            LOG_ERROR("[r_xfer_onBleReceived] Add to Queue fail. status = 0x%04x\n", status);
        }
    }*/

    LOG_WARN("[r_xfer] Disconnected Clear session.\r\n");

    p = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t));
    if(p == NULL){
        LOG_ERROR("[r_xfer] Disconnected-{%s}\n", ERR_STR_MALLOC_FAIL);
        return;
    }
    ry_memset(p, 0, sizeof(r_xfer_msg_t) );
    p->flag = R_XFER_FLAG_DISCONNECT;
    
    if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, &p, 4))) {
        LOG_ERROR("[r_xfer] Disconnected: Add to Queue fail. status = 0x%04x\n", status);
    }
}



static void r_xfer_inst_clear(r_xfer_msg_t* pMsg)
{
    //p->instance = r_xfer_instance_search( i );
    r_xfer_inst_t* inst = NULL;

    FREE_PTR(pMsg);

    int i = 0;

    for(i = 0; i < R_XFER_INST_ID_UART; i++){
        inst = r_xfer_instance_search( i );

        if(inst != NULL){
            if(inst->session_tx == NULL){

            }else{

                if(inst->writeDoneCb != NULL) {
                    uint32_t status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NOT_CONNECTED);
                    inst->writeDoneCb(status, inst->writeUsrdata);
                }

                inst->session_tx->end_time = ry_hal_clock_time();
                LOG_DEBUG("================ Tx Disconnected, duration:%d ms ==================\n", 
                    ry_hal_calc_ms(inst->session_tx->end_time - inst->session_tx->start_time));

                inst->lastRxDuration = inst->session_tx->end_time - inst->session_tx->start_time;
                r_xfer_session_destroy(&inst->session_tx);
            }

            if(inst->session_rx == NULL){

            }else{
                inst->session_rx->end_time = ry_hal_clock_time();
                LOG_DEBUG("================ Rx Disconnected, duration:%d ms ==================\n", 
                    ry_hal_calc_ms(inst->session_rx->end_time - inst->session_rx->start_time));

                inst->lastRxDuration = inst->session_rx->end_time - inst->session_rx->start_time;

                if (inst->session_rx->pdata) {
                    FREE_PTR(inst->session_rx->pdata);
                }

                r_xfer_session_destroy(&inst->session_rx);
            }

            R_XFER_SET_STATE_TX(inst, R_XFER_STATE_IDLE);
            R_XFER_SET_STATE_RX(inst, R_XFER_STATE_IDLE);
        }
    }

    r_xfer_msg_t* p_msg = NULL;
    uint32_t status = RY_SUCC;

    while(true) {
        status = ryos_queue_recv(&r_xfer_v.txQ, &p_msg, RY_OS_NO_WAIT);
        if(status == RY_SUCC) {
            status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NOT_CONNECTED);
            r_xfer_msg_request_abort(p_msg, status);
        } else {
            break;
        }
    }

}

bool r_xfer_is_tx_busy(r_xfer_inst_t* inst)
{
    return (inst->state_tx != R_XFER_STATE_IDLE);
}
bool r_xfer_is_rx_busy(r_xfer_inst_t* inst)
{
    return (inst->state_rx != R_XFER_STATE_IDLE);
}

void find_lost_sn_list(r_xfer_sess_t *session, uint16_t* list, uint8_t* p_num)
{
    int i;
    uint8_t *p = session->pdata;
    uint8_t count = 0;
    
    for (i = 0; i < session->amount; i++) {
        if (!r_xfer_sn_buffer_check_bit_recived(session->pack_recv_flag, i+1)){
            list[count] = i+1;
            count++;
            if (count >= 8) {
                break;
            }
        }
    }

    *p_num = count;
}

void r_xfer_tx_timeout_handler(void* arg)
{
    r_xfer_inst_t* inst = (r_xfer_inst_t*)arg;
    r_xfer_msg_t *p = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t));
    ry_sts_t status = RY_SUCC;
    
    LOG_DEBUG("[r_xfer_tx]timeout_handler: transfer timeout.\n");

    if(p == NULL){
        return;
    }

    if(inst == NULL){
        FREE_PTR(p);
        return ;
    }

    if(inst->session_tx == NULL){
        FREE_PTR(p);
        return;
    }
    

    inst->session_tx->timeout_flag = 1;
    inst->session_tx->retry_cnt ++;
    p->instance = inst;
    p->len = 0;
    p->flag = R_XFER_FLAG_TIMEOUT;

    if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, &p, 4))) {
        LOG_ERROR("[r_xfer_tx]timeout_handler: Add to Queue fail. status = 0x%04x\n", status);
    }
}

void r_xfer_rx_timeout_handler(void* arg)
{
    r_xfer_inst_t* inst = (r_xfer_inst_t*)arg;
    r_xfer_msg_t *p = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t));
    ry_sts_t status = RY_SUCC;
    
    LOG_DEBUG("[r_xfer_rx]timeout_handler: transfer timeout.\n");

    if(p == NULL){
        return;
    }

    if(inst == NULL){
        FREE_PTR(p);
        return ;
    }

    if(inst->session_rx == NULL){
        FREE_PTR(p);
        return;
    }

    inst->session_rx->timeout_flag = 1;
    inst->session_rx->retry_cnt ++;
    p->instance = inst;
    p->len = 0;
    p->flag = R_XFER_FLAG_TIMEOUT;

    if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, &p, 4))) {
        LOG_ERROR("[r_xfer_rx]timeout_handler: Add to Queue fail. status = 0x%04x\n", status);
    }
}


void r_xfer_timer_stop(int timer_id)
{
    ry_timer_stop(timer_id);
}


r_xfer_sess_t* r_xfer_tx_session_new(r_xfer_inst_t* inst, uint16_t amount, int last_bytes, uint8_t* data)
{
    ry_timer_parm timer_para;
    uint32_t timeout = 0;
    ry_sts_t status;
    int connInterval;
    
    /* Check packet number first */
    if (amount > 220) {
        /* Too large data, it should be seperate to more less packets. */
        return NULL;
    }

    r_xfer_sess_t* p = (r_xfer_sess_t*)ry_malloc(sizeof(r_xfer_sess_t));
    if (NULL == p) {
        LOG_ERROR("[r_xfer_tx]session_new:No Mem1.\n");
        return NULL;
    }

    ry_memset((void*)p, 0x00, sizeof(r_xfer_sess_t));

    inst->session_tx    = p;
    p->amount        = amount;
    p->pdata         = data;
    p->last_bytes    = last_bytes;
    p->tx_sn         = 1;
    p->retry_cnt     = 0;
    p->start_time    = ry_hal_clock_time();

    /* Create the Semaphore to Control TXD process */
    status = ryos_semaphore_create(&p->tx_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_tx]session_new: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
        FREE_PTR(p);
        inst->session_tx = NULL;
        return NULL;
    }

    /* Create the timer */
    timer_para.timeout_CB = r_xfer_tx_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "r_xfer_tx";
    p->timer_id = ry_timer_create(&timer_para);

    /* Calculate the transfer time */
    connInterval = ry_ble_get_connInterval();
    if (connInterval == 0) {
        connInterval = 100;
    }
    timeout = amount * ((connInterval*5)>>2)*2 + 2000;
    if (timeout > 5000) {
        timeout = 5000;
    }
    ry_timer_start(p->timer_id, timeout, r_xfer_tx_timeout_handler, (void*)inst);

    g_r_xfer_busy = 1;
    
    return p;
}



r_xfer_sess_t* r_xfer_session_new(r_xfer_inst_t* inst, uint16_t amount, uint8_t cmd, uint8_t mode)
{
    ry_timer_parm timer_para;
    uint32_t timeout = 0;
    ry_sts_t status;
    int connInterval;
    uint32_t flag_required_bytes;
    uint32_t requrired_rx_buffer_size;

    /* Check packet number first */
    if (amount > 237) {
        /* Too large data, it should be seperate to more less packets. */
        return NULL;
    }

    r_xfer_sess_t* p = (r_xfer_sess_t*)ry_malloc(sizeof(r_xfer_sess_t));
    if (NULL == p) {
        LOG_ERROR("[r_xfer_rx]session_new: No Mem1.\n");
        return NULL;
    }
    requrired_rx_buffer_size  = amount * R_XFER_ATT_CHAR_SIZE;
    if(requrired_rx_buffer_size > MAX_R_XFER_SIZE + R_XFER_ATT_CHAR_SIZE + DEFAULT_MTU_SIZE) {
        LOG_DEBUG("[r_xfer_rx] session new malloc: %d\n", requrired_rx_buffer_size);
        requrired_rx_buffer_size = MAX_R_XFER_SIZE + R_XFER_ATT_CHAR_SIZE + DEFAULT_MTU_SIZE;
    }

    ry_memset((void*)p, 0x00, sizeof(r_xfer_sess_t));

    inst->session_rx    = p;
    p->amount        = amount;
    p->type          = cmd;
    p->mode          = mode;
    p->pdata         = ry_malloc(requrired_rx_buffer_size);
    p->temp_last_sn  = amount;
    // p->retry_cnt     = 0;
    // p->total_len     = 0;
    // p->cur_pack_size = 0;
    p->start_time    = ry_hal_clock_time();
    
    if (p->pdata == NULL) {
        LOG_ERROR("[r_xfer_rx]session_new: No Mem2.\n");
        FREE_PTR(p);
        inst->session_rx = NULL;
        return NULL;
    }

    ry_memset(p->pdata, 0, requrired_rx_buffer_size);

    flag_required_bytes = (amount + 7)/8;

    p->pack_recv_flag= (uint8_t *)ry_malloc(flag_required_bytes);

    if(!p->pack_recv_flag) {
        LOG_ERROR("[r_xfer_rx]session_new: No Mem3.\n");
        FREE_PTR(p->pdata);
        FREE_PTR(p);
        inst->session_rx = NULL;
        return NULL;
    }else {
        memset(p->pack_recv_flag, 0xFF, flag_required_bytes);
    }

    /* Create the Semaphore to Control TXD process */
    status = ryos_semaphore_create(&p->tx_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_rx]session_new: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
        FREE_PTR(p->pdata);
        FREE_PTR(p);
        inst->session_rx = NULL;
        return NULL;
    } 

    /* Create the timer */
    timer_para.timeout_CB = r_xfer_rx_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "r_xfer_rx";
    p->timer_id = ry_timer_create(&timer_para);

    /* Calculate the transfer time */
    connInterval = ry_ble_get_connInterval();
    if (connInterval == 0) {
        connInterval = 100;
    }
    timeout = amount * ((connInterval*5)>>2)*2 + 2000;
    if (timeout > 5000) {
        timeout = 5000;
    }
    //LOG_DEBUG("r_xfer rx session timeout: %d sn:%d\r\n", timeout, p->temp_last_sn);
    ry_timer_stop(p->timer_id);
    ry_timer_start(p->timer_id, timeout, r_xfer_rx_timeout_handler, (void*)inst);

    g_r_xfer_busy = 1;
    
    return p;
}


static void r_xfer_msg_request_abort(r_xfer_msg_t* p_msg, uint32_t status)
{
    if(p_msg->txCb != NULL) {
        p_msg->txCb(status, p_msg->txArg);
    }
}

static void r_xfer_session_destroy(r_xfer_sess_t** pp_session)
{
    r_xfer_sess_t* p_session = *pp_session;

    ryos_semaphore_delete(p_session->tx_sem);
    ry_timer_stop(p_session->timer_id);
    ry_timer_delete(p_session->timer_id);
    //ry_free((void*)inst->curSession->pdata); // this one will be free in upper layer

    if (p_session->pack_recv_flag) {
        FREE_PTR(p_session->pack_recv_flag);
    }

    if (p_session != NULL) {
        FREE_PTR(p_session);
        *pp_session = NULL;
    }

    g_r_xfer_busy = 0;
}

ry_sts_t r_xfer_ack(r_xfer_inst_t* inst, fctrl_ack_t ack, uint8_t* user_data, uint8_t len)
{
    r_xfer_frame_t  frame;
    uint16_t        data_len;
    uint32_t        delta;

    
    memset((void*)&frame, 0, sizeof(frame));

    frame.f.ctrl.mode = R_XFER_MODE_ACK;
    frame.f.ctrl.type = ack;
    
    //ASSERT(len <= 4);
    if (len > 0) {
        ry_memcpy(frame.f.ctrl.arg, user_data, len);
    }
    data_len = 4 + len; // sizeof(frame.sn) + sizeof(frame.f.ctrl.type) + sizeof(frame.f.ctrl.mode) + 4;

    if(inst != NULL){
        inst->write((uint8_t*)&frame, data_len, (void*)inst->session_rx->tx_sem);
    }

    delta = ry_hal_clock_time() - g_test_timestamp;
    g_test_timestamp = ry_hal_clock_time();
    LOG_DEBUG("[r_xfer_rx] Send ACK. ack = %d  time:%d\n", ack, ry_hal_calc_ms(delta));
    
    //ryos_semaphore_wait(inst->session->tx_sem);

    return RY_SUCC;
}

ry_sts_t r_xfer_cmd(r_xfer_inst_t* inst, fctrl_cmd_t cmd, uint16_t amount, uint16_t session_id)
{
    r_xfer_frame_t       frame = {0};
    uint16_t             data_len;

    LOG_DEBUG("[r_xfer_tx] Send Command. cmd = %d \n", cmd);
    frame.f.ctrl.mode   = R_XFER_MODE_CMD;
    frame.f.ctrl.type   = cmd;
    frame.f.ctrl.arg[0] = LO_UINT16(amount);
    frame.f.ctrl.arg[1] = HI_UINT16(amount);
    frame.f.ctrl.arg[2] = LO_UINT16(session_id);
    frame.f.ctrl.arg[3] = HI_UINT16(session_id);

    data_len = 8;//sizeof(frame.sn) + sizeof(frame.f.ctrl) + session_id;

    if(inst != NULL){
        inst->write((uint8_t*)&frame, data_len, (void*)inst->session_tx->tx_sem);
    }
    //ryos_semaphore_wait(inst->session->tx_sem);
    return RY_SUCC;
}

ry_sts_t r_xfer_data(r_xfer_inst_t* inst, uint16_t sn)
{
    r_xfer_frame_t   frame = {0};
    uint16_t         data_len;
    ry_sts_t         status;

    r_xfer_sess_t *p = inst->session_tx;

    //LOG_DEBUG("[r_xfer] Send Data. sn = %d \n", sn);
    //uint8_t (*pdata)[R_XFER_ATT_PAYLOAD_SIZE] = (void*)p->pdata;

    uint32_t tx_index = (p->cur_pack_size - 2) * (sn - 1); 
    uint8_t* pdata = &p->pdata[tx_index];
    frame.sn = sn;

    if (sn == p->amount) {
        data_len = p->last_bytes;
    } else {
        data_len = p->cur_pack_size - 2;//sizeof(frame.f.data);
    }

    memcpy(frame.f.data, pdata, data_len);
    LOG_DEBUG("[r_xfer_tx] pack_len:%d pack_index:%d\r\n", data_len, tx_index);
    data_len += 2;//sizeof(frame.sn);

    if(inst == NULL){
        return RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_INVALIC_PARA);
    }

    if (RY_SUCC == inst->write((uint8_t*)&frame, data_len, inst->session_tx->tx_sem)) {
        //LOG_DEBUG("[r_xfer_data] block, waiting TX Ready...\r\n");
        status = ryos_semaphore_wait(p->tx_sem);
        return RY_SUCC;
    } else {
        LOG_WARN("[r_xfer_tx] data block, waiting TX Ready...\r\n");
        status = ryos_semaphore_wait(p->tx_sem);

        if (RY_SUCC != status) {
            LOG_ERROR("[r_xfer_tx] data Semaphore Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        } else {
            LOG_DEBUG("[r_xfer_data] Continue TX.\r\n");
        }
        return RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_BUSY);
    }

}

void r_xfer_onReceivedFinish(r_xfer_inst_t* inst)
{
    uint8_t* p   = inst->session_rx->pdata;
    uint32_t len = inst->session_rx->total_len;
    uint32_t delta;

    if(inst == NULL){
        return ;
    }

    if(inst->session_rx == NULL){
        return ;
    }

    inst->session_rx->end_time = ry_hal_clock_time();
    delta = ry_hal_clock_time() - g_test_timestamp;
    g_test_timestamp = ry_hal_clock_time();
    LOG_DEBUG("================End RX, duration:%dms===========time:%d=======\n", 
        ry_hal_calc_ms(inst->session_rx->end_time - inst->session_rx->start_time), ry_hal_calc_ms(delta));

    inst->lastRxDuration = inst->session_rx->end_time - inst->session_rx->start_time;

    R_XFER_SET_STATE_RX(inst, R_XFER_STATE_IDLE);

    /* Release related session and control block */
    r_xfer_session_destroy(&inst->session_rx);

    /* Callback to upper layer */
    inst->readCb(p, len, inst->usrData);
}

void r_xfer_onReceiveFail(r_xfer_inst_t* inst)
{
    if(inst == NULL){
        return ;
    }
    if(inst->session_rx == NULL){
        return ;
    }
    inst->session_rx->end_time = ry_hal_clock_time();
    LOG_DEBUG("================End RX, Timeout. duration:%dms==================\n",
        ry_hal_calc_ms(inst->session_rx->end_time - inst->session_rx->start_time));
    inst->lastRxDuration = inst->session_rx->end_time - inst->session_rx->start_time;
    R_XFER_SET_STATE_RX(inst, R_XFER_STATE_IDLE);

    if (inst->session_rx->pdata) {
        FREE_PTR(inst->session_rx->pdata);
    }
    
    /* Release related session and control block */
    r_xfer_session_destroy(&inst->session_rx);
}


void r_xfer_tx_onSentFinish(r_xfer_inst_t* inst, ry_sts_t status)
{
    if(inst == NULL){
        return ;
    }
    if(inst->session_tx == NULL){
        return ;
    }

    inst->session_tx->end_time = ry_hal_clock_time();
    LOG_DEBUG("================End TX, duration:%dms=====================\n",
        ry_hal_calc_ms(inst->session_tx->end_time - inst->session_tx->start_time));

    inst->lastTxDuration = inst->session_tx->end_time - inst->session_tx->start_time;
    R_XFER_SET_STATE_TX(inst, R_XFER_STATE_IDLE);

    /* Release related session and control block */
    r_xfer_session_destroy(&inst->session_tx);

    /* Callback to upper layer */
    inst->writeDoneCb(status, inst->writeUsrdata);

    /*r_xfer_sent_fifo_t * temp = get_tx_wait_data(inst);

    if(temp != NULL){
        r_xfer_send(temp->instID, temp->data, temp->len, temp->sendCb, temp->usrdata);
        
    }*/

    #if 0
    r_xfer_sent_fifo_t *tx_para = NULL;
    
    if(ryos_queue_recv(r_xfer_v.txQ, &tx_para, RY_OS_NO_WAIT) == RY_SUCC){
         r_xfer_send(tx_para->instID, tx_para->data, tx_para->len, tx_para->sendCb, tx_para->usrdata);
    }
    #endif
}

static void r_xfer_tx_check_queue(r_xfer_inst_t* inst)
{
    r_xfer_msg_t* p_msg;

    uint32_t status = ryos_queue_recv(r_xfer_v.txQ, &p_msg, RY_OS_NO_WAIT);

    if (status == RY_SUCC) {
        status = ryos_queue_send(r_xfer_v.msgQ, &p_msg, sizeof(r_xfer_msg_t*));
        if (status != RY_SUCC) {
            status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_QUEUE_SEND);
            LOG_ERROR("[r_xfer_tx] Add to msg Queue failed. status = 0x%04x\n", status);
            r_xfer_msg_request_abort(p_msg, status);
        } else {
            LOG_DEBUG("[r_xfer_tx] tx queue poped\n");
        }
    } else {
        // do nothing
        LOG_DEBUG("[r_xfer_tx] tx queue empty\n");
    }

}


#if 0
static int r_xfer_resend(r_xfer_inst_t *inst)
{
    static  uint16_t sn;
    uint16_t sn_list[8] = {0};
    uint8_t  n_list = 0;
    int i;

    find_lost_sn_list(inst->session, sn_list, &n_list);
    if (n_list == 0) {
        r_xfer_ack(inst, A_SUCCESS, NULL, 0);
        r_xfer_onReceivedFinish(inst);
        return 0;
    } else {
        LOG_DEBUG("lost sn: ");
        for (i = 0; i < n_list; i++) {
            LOG_DEBUG("%d ", sn_list[i]);
        }
        LOG_DEBUG("\n");
        
        r_xfer_ack(inst, A_LOST, (uint8_t*)sn_list, n_list*2);
        LOG_DEBUG("PT wait sn: %d\n", sn_list[n_list-1]);
        temp_last_sn = sn_list[n_list-1];
        //PT_WAIT_UNTIL(pt, inst->cur_sn == temp_last_sn);

        return 1;
    }
}
#endif

static void r_xfer_rxReqHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    r_xfer_frame_t *pFrame = (r_xfer_frame_t*)data;
    uint16_t curSn            = pFrame->sn;
    fctrl_cmd_t cmd        = (fctrl_cmd_t)pFrame->f.ctrl.type;
    uint16_t amount           = *(uint16_t*)pFrame->f.ctrl.arg;
    r_xfer_sess_t *sess    = NULL;
    uint32_t delta;

    if(inst->state_tx != R_XFER_STATE_IDLE) {
//        LOG_WARN("[r_xfer_rx] ----------------  tx_processing  ----------------\n");
    }

    delta = ry_hal_clock_time() - g_test_timestamp;
    g_test_timestamp = ry_hal_clock_time();
    LOG_DEBUG("================Start RX==============amount:%d=======\n", amount);
    sess = r_xfer_session_new(inst, amount, cmd, R_XFER_MODE_CMD);
    if (sess == NULL) {
        return;
    }

    /* Success, send ACK Ready back */
    r_xfer_ack(inst, A_READY, NULL, 0);
    R_XFER_SET_STATE_RX(inst, R_XFER_STATE_RX);

}

static void r_xfer_rxBusyHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    LOG_DEBUG("[r_xfer_rx] r_xfer_rxBusyHandler\n");
    r_xfer_ack(inst, A_BUSY, NULL, 0);
}

static void r_xfer_rxDataHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    r_xfer_frame_t *pFrame = (r_xfer_frame_t*)data;
    uint16_t            curSn = pFrame->sn;
    r_xfer_sess_t *session = inst->session_rx;

    uint16_t sn_list[8] = {0};
    uint8_t  n_list = 0;
    int i;

    if(session == NULL){
        return;
    }

    if (data != NULL) {
        if(session->cur_pack_size == 0) {
            session->cur_pack_size = len;
        }
//        else if(session->cur_pack_size < len) {
//            LOG_ERROR("[r_xfer_rxDataHandler] err payload \n");
//        }
        uint32_t payload_size = len - 2;
        uint32_t payload_index = (pFrame->sn - 1) * (session->cur_pack_size - 2);
        if(payload_index + payload_size > MAX_R_XFER_SIZE + R_XFER_ATT_CHAR_SIZE + DEFAULT_MTU_SIZE) {
            LOG_ERROR("[r_xfer_rxDataHandler] overflow!!!\n");  //should never be here
        }
        // LOG_DEBUG("[r_xfer_rxDataHandler] payload:%d,sn:%d,index:%d\n", payload_size, curSn, payload_index);

        ry_memcpy(&session->pdata[payload_index], pFrame->f.data, payload_size);
        if(r_xfer_sn_buffer_check_bit_recived(session->pack_recv_flag, pFrame->sn)) {
            LOG_WARN("[r_xfer_rxDataHandler] duplicate sn:%d\n", curSn);
        } else {
            session->total_len += payload_size;
        }

        r_xfer_sn_buffer_set_bit(session->pack_recv_flag, pFrame->sn);
    }

    session->cur_sn = curSn;

    if (session->cur_sn == session->temp_last_sn) {
        /* Check if there is any packet lost */
        //LOG_DEBUG("r_xfer_rxDataHandler temp_last_sn:%d\r\n", session->temp_last_sn);
        find_lost_sn_list(session, sn_list, &n_list);

        if (n_list == 0) {
            /* RX complete */
            r_xfer_ack(inst, A_SUCCESS, NULL, 0);
            r_xfer_onReceivedFinish(inst);
        } else {
            LOG_DEBUG("[r_xfer_rx] lost sn: \n");
            for (i = 0; i < n_list; i++) {
                LOG_DEBUG("%d \n", sn_list[i]);
            }
            LOG_DEBUG("\n");

            r_xfer_ack(inst, A_LOST, (uint8_t*)sn_list, n_list*sizeof(uint16_t));
            session->temp_last_sn = sn_list[n_list-1];
            R_XFER_SET_STATE_RX(inst, R_XFER_STATE_RX_RESEND);
        }
    }
}

static void r_xfer_rxTimeoutHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    int i;
    r_xfer_sess_t *session = inst->session_rx;
    uint16_t sn_list[8] = {0};
    uint8_t  n_list = 0;
    int timeout;
    uint16_t connInterval;

    LOG_DEBUG("[r_xfer_rx] r_xfer_rxTimeoutHandler\r\n");

    if (session == NULL) {
        return;
    }

    if (session->retry_cnt > R_XFER_MAX_RETRY_CNT) {
        LOG_ERROR("[r_xfer_rx] RX Timeout - Receive Fail.\r\n");
        r_xfer_onReceiveFail(inst);
        return;
    }

    find_lost_sn_list(session, sn_list, &n_list);

    if (n_list == 0) {
        /* RX complete */
        r_xfer_ack(inst, A_SUCCESS, NULL, 0);
        r_xfer_onReceivedFinish(inst);
        
    } else {
        LOG_ERROR("[r_xfer_rx] RX Timeout - Cnt:%d. \r\n", session->retry_cnt);
        LOG_DEBUG("lost sn: \n");
        for (i = 0; i < n_list; i++) {
            LOG_DEBUG("%d \n", sn_list[i]);
        }
        LOG_DEBUG("\n");

        r_xfer_ack(inst, A_LOST, (uint8_t*)sn_list, n_list*sizeof(uint16_t));
        session->temp_last_sn = sn_list[n_list-1];
        R_XFER_SET_STATE_RX(inst, R_XFER_STATE_RX_RESEND);

        connInterval = ry_ble_get_connInterval();
        if (connInterval == 0) {
            connInterval = 100;
        }
        timeout = n_list * ((connInterval*5)>>2) + 2000;
        if (timeout > 5000) {
            timeout = 5000;
        }

        ry_timer_stop(session->timer_id);
        ry_timer_start(session->timer_id, timeout, r_xfer_rx_timeout_handler, (void*)inst);
    }
}

static void r_xfer_txTimeoutHandler(r_xfer_inst_t* inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    if(inst->session_tx->retry_cnt > R_XFER_TX_MAX_RETRY_CNT) {
        LOG_ERROR("[r_xfer_tx] r_xfer_txTimeoutHandler retry failed\r\n");
        r_xfer_tx_onSentFinish(inst, RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_TIMEOUT));
        r_xfer_tx_check_queue(inst);
    } else {
        LOG_DEBUG("[r_xfer_tx] r_xfer_txTimeoutHandler retry:%d\r\n", inst->session_tx->retry_cnt);

        int connInterval;
        int amount = inst->session_tx->amount;
        int timeout;
        inst->session_tx->tx_sn = 1;
        connInterval = ry_ble_get_connInterval();
        if (connInterval == 0) {
            connInterval = 100;
        }
        timeout = amount * ((connInterval*5)>>2)*2 + 2000;
        if (timeout > 5000) {
            timeout = 5000;
        }
        r_xfer_cmd(inst, DEV_RAW, inst->session_tx->amount, 0);
        ry_timer_stop(inst->session_tx->timer_id);
        ry_timer_start(inst->session_tx->timer_id, timeout, r_xfer_tx_timeout_handler, (void*)inst);
    }
    
}

static void r_xfer_txReqHandler(r_xfer_inst_t* inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    r_xfer_sess_t *session = NULL;
    int pkt_num;
    int last_cnt;

    //LOG_DEBUG("[r_xfer_txReqHandler]\n");

    if(inst->state_tx != R_XFER_STATE_IDLE) {
//        LOG_WARN("[r_xfer_tx] ----------------  rx_processing  ----------------\n");
    }
    uint32_t mtu = ry_ble_get_cur_mtu();
    uint32_t char_max_size = mtu - 3;    //ATT header

    uint32_t pack_size = (char_max_size < R_XFER_ATT_CHAR_SIZE) ? char_max_size : R_XFER_ATT_CHAR_SIZE;
    uint32_t payload_size = pack_size - 2;  //sn

    /* Direct send */
    pkt_num  = len / payload_size;
    last_cnt = len % payload_size;
    pkt_num  = (last_cnt == 0) ? (len/payload_size) : (len/payload_size)+1;
    inst->writeUsrdata = pMsg->txArg;
    inst->writeDoneCb  = pMsg->txCb;
    session = r_xfer_tx_session_new(inst, pkt_num, (last_cnt == 0)?payload_size:last_cnt, pMsg->txData);
    if (session == NULL) {
        inst->writeDoneCb(RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_NO_MEM), inst->writeUsrdata);
        return;
    }
    session->cur_pack_size = pack_size;

    LOG_DEBUG("================Start TX, len:%d=====================\n", len);

    r_xfer_cmd(inst, DEV_RAW, inst->session_tx->amount, 0);
    R_XFER_SET_STATE_TX(inst, R_XFER_STATE_TX_WAIT_ACK);
}


static void r_xfer_txAckHandler(r_xfer_inst_t* inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    r_xfer_frame_t *pframe = (r_xfer_frame_t*)data;
    fctrl_ack_t ack = (fctrl_ack_t)pframe->f.ctrl.type;
    ry_sts_t status;
    int i;

    if (inst->session_tx == NULL) {
        return ;
    }
    
    LOG_DEBUG("[r_xfer_tx] ack received. type = %d\n", ack);
    inst->session_tx->mode = R_XFER_MODE_ACK;
    inst->session_tx->type = ack;
    switch (ack) {

        case A_SUCCESS:
            /* Transfer done. */
            r_xfer_tx_onSentFinish(inst, RY_SUCC);
            r_xfer_tx_check_queue(inst);
            break;
            
        case A_READY:
            inst->session_tx->cur_sn = 0;
            R_XFER_SET_STATE_TX(inst, R_XFER_STATE_TX_WAIT_ACK);

            while (inst->session_tx->tx_sn <= inst->session_tx->amount) {
                if(ry_ble_get_state() != RY_BLE_STATE_CONNECTED){
                    break;
                }
                status = r_xfer_data(inst, inst->session_tx->tx_sn);
                if (RY_SUCC == status) {
                    inst->session_tx->tx_sn++;
                }
                //ryos_delay_ms(5);
            }
            break;

        case A_LOST:
            inst->session_tx->cur_sn = *(uint16_t*)pframe->f.ctrl.arg;
            ry_memcpy(inst->session_tx->lost_sn_list, pframe->f.ctrl.arg, len - 4);
            inst->session_tx->n_lost_sn_list = (len-4) >> 1;
            LOG_DEBUG("[r_xfer_tx] lost sn num:%d.\n", inst->session_tx->n_lost_sn_list);
            R_XFER_SET_STATE_TX(inst, R_XFER_STATE_TX_WAIT_ACK);

            for (i = 0; i < inst->session_tx->n_lost_sn_list; i++) {
                r_xfer_data(inst, inst->session_tx->lost_sn_list[i]);
            }
        break;

        default:
            LOG_DEBUG("[r_xfer_tx] Unknown reliable ACK.\n");
    }
}



static void r_xfer_state_machine(r_xfer_msg_t* pMsg, uint16_t evt)
{
    int i, size;

    //LOG_DEBUG("[r_xfer_state_machine] inst_state:%d, evt:0x%04x\r\n", pMsg->instance->state, evt);

    size = sizeof(r_xfer_stateMachine)/sizeof(r_xfer_stateMachine_t);

    /* Search an appropriate event handler */
    if(evt != R_XFER_EVENT_RX_REQ) {
        for (i = 0; i < size; i++) {
            if (r_xfer_stateMachine[i].curState == pMsg->instance->state_tx &&
                r_xfer_stateMachine[i].event == evt) {
                /* call event handler */
                r_xfer_stateMachine[i].handler(pMsg->instance, pMsg->data, pMsg->len, pMsg);
                break;
             }
        }
    }

    if(evt != R_XFER_EVENT_TX_REQ) {
        for (i = 0; i < size; i++) {
            if (r_xfer_stateMachine[i].curState == pMsg->instance->state_rx &&
                r_xfer_stateMachine[i].event == evt) {
                /* call event handler */
                r_xfer_stateMachine[i].handler(pMsg->instance, pMsg->data, pMsg->len, pMsg);
                break;
             }
        }
    }

    FREE_PTR(pMsg);
    return;
}



/**
 * @brief   Handle recevied data from BLE.
 *
 * @param   data     - The received data.
 * @param   len      - The length of received data.
 * @param   charUUID - The ble channel
 *
 * @return  None
 */
uint32_t g_test_rx_tick = 0;
void r_xfer_onBleReceived(uint8_t* data, int len, uint16_t charUUID)
{
    ry_sts_t status = RY_SUCC;
    if(len > R_XFER_ATT_CHAR_SIZE) {
        LOG_DEBUG("[r_xfer_onBleReceived] Wrong Size %d more than %d.\n",
            len,
            R_XFER_ATT_CHAR_SIZE);
    }

    r_xfer_msg_t *p = (r_xfer_msg_t*)ry_malloc(len + sizeof(r_xfer_msg_t));
    if (NULL == p) {
        LOG_ERROR("[r_xfer_onBleReceived] No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }

    //LOG_DEBUG("[r_xfer_onBleReceived] Received len = %d, uuid=%x\n", len, charUUID);
    //ry_data_dump(data, len, ' ');

    ry_memset(p, 0, sizeof(r_xfer_msg_t));
    
    g_test_rx_tick = ry_hal_clock_time();

    p->len = len;
    p->flag = R_XFER_FLAG_RX;
    ry_memcpy(p->data, data, len);
    if (0xAA00 == charUUID) {
        p->instance = r_xfer_instance_search(R_XFER_INST_ID_BLE_SEC);
    } else if (0xAA01 == charUUID) {
        p->instance = r_xfer_instance_search(R_XFER_INST_ID_BLE_UNSEC);
    } else if (0xBA01 == charUUID) {
        p->instance = r_xfer_instance_search(R_XFER_INST_ID_BLE_JSON);
    }

    if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, &p, 4))) {
        LOG_ERROR("[r_xfer_onBleReceived] Add to Queue fail. status = 0x%04x\n", status);
    }
    
}

/**
 * @brief   Handle recevied data from UART.
 *
 * @param   data     - The received data.
 * @param   len      - The length of received data.
 *
 * @return  None
 */
void r_xfer_onUartReceived(uint8_t* data, int len)
{
    ry_sts_t status = RY_SUCC;
    r_xfer_msg_t *p = (r_xfer_msg_t*)ry_malloc(len + sizeof(r_xfer_msg_t));
    if (NULL == p) {

        LOG_ERROR("[r_xfer_onUartReceived] No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }   

    p->len      = len;
    p->flag     = R_XFER_FLAG_RX;
    p->instance = r_xfer_instance_search(R_XFER_INST_ID_UART);;
    ry_memcpy(p->data, data, len);

    if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, p, 4))) {
        LOG_ERROR("[r_xfer_onUartReceived] Add to Queue fail. status = 0x%04x\n", status);
    }
}

#if 0
__weak void r_xfer_on_appKilled_recived(void)
{
    LOG_INFO("[r_xfer_onReceived] app killed\n");
}
#else
// function should 
extern void r_xfer_on_appKilled_recived(void);
#endif


/**
 * @brief   Handle recevied data.
 *
 * @param   data     - The received data.
 * @param   len      - The length of received data.
 *
 * @return  None
 */
void r_xfer_onReceived(r_xfer_msg_t* pMsg)
{
    //ry_data_dump(pMsg->data, pMsg->len, ' ');

    r_xfer_frame_t *pFrame = (r_xfer_frame_t*)pMsg->data;
    uint16_t curSn = 0;
    uint16_t evt;
    uint32_t delta;

    curSn = pFrame->sn;
    //LOG_DEBUG("[r_xfer_onReceived] cur_sn: 0x%04x\n", curSn);

    if (0 == curSn) {
        if (pFrame->f.ctrl.mode == R_XFER_MODE_CMD) {
            /* Received frame control packet */
            evt = R_XFER_EVENT_RX_REQ;
            delta = ry_hal_clock_time() - g_test_rx_tick;
            g_test_timestamp = ry_hal_clock_time();
            LOG_DEBUG("[r_xfer_onReceived] first schedule tick: %d\r\n", ry_hal_calc_ms(delta));
        } else if(pFrame->f.ctrl.mode == R_XFER_MODE_ACK) {
            /* Received ACK packet */
            evt = R_XFER_EVENT_TX_ACK;
        } else if(pFrame->f.ctrl.mode == R_XFER_MODE_CRITICAL) {
            //R_XFER_CRIT_APP_DESTROYED
            LOG_DEBUG("[r_xfer_onReceived] ===critical cmd==== 0x%02X\n", pFrame->f.ctrl.type);
            if((pFrame->f.ctrl.type  == R_XFER_EVENT_APP_KILLED)
                &&(pMsg->instance->id == R_XFER_INST_ID_BLE_UNSEC)){
                r_xfer_on_appKilled_recived();
            }
            return;
        } else {
            LOG_ERROR("[r_xfer_onReceived] !!==== err ctrl mode:%d ====!!\n", pFrame->f.ctrl.mode);
            return;
        }
    } else {
        /* Received data packet */
        evt = R_XFER_EVENT_RX_DATA;
    }

done:
    /* Start state machine to handle data. */
    r_xfer_state_machine(pMsg, evt);
    
}


void r_xfer_onTimeout(r_xfer_msg_t* pMsg)
{
    //ry_data_dump(pMsg->data, pMsg->len, ' ');
    uint16_t evt;

    /* Check if it's a timeout event first */
    if (pMsg->instance->session_rx != NULL) {
        if (pMsg->instance->session_rx->timeout_flag == 1) {
            pMsg->instance->session_rx->timeout_flag = 0;
            evt = R_XFER_EVENT_RX_TIMEOUT;
            LOG_WARN("[r_xfer] Rx Timeout Event.\n");
            goto done;
        }
    }
    /* Check if it's a timeout event first */
    if (pMsg->instance->session_tx != NULL) {
        if (pMsg->instance->session_tx->timeout_flag == 1) {
            pMsg->instance->session_tx->timeout_flag = 0;
            evt = R_XFER_EVENT_TX_TIMEOUT;
            LOG_WARN("[r_xfer] Tx Timeout Event.\n");
            goto done;
        }
    }

done:
    /* Start state machine to handle data. */
    r_xfer_state_machine(pMsg, evt);
    
}

void r_xfer_on_send_cb(void* usrdata)
{
    ryos_semaphore_t* tx_sem = (ryos_semaphore_t*)usrdata;
    ryos_semaphore_post(tx_sem);
}


/**
 * @brief   API to send reliable data.
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t r_xfer_send(int instID, uint8_t* data, int len, r_xfer_send_cb_t sendCb, void* usrdata)
{
    ry_sts_t status = RY_SUCC;
    r_xfer_msg_t *p = NULL;

    r_xfer_inst_t* inst = r_xfer_instance_search(instID);
    if (!inst) {
        status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_INVALIC_PARA);
        goto exit;
    }

    /* Check if the instance is busy */

#if 0
    if(inst->session != NULL){
        //add_tx_wait_data(instID,  data, len, sendCb,  usrdata);
        r_xfer_sent_fifo_t *tx_para = (r_xfer_sent_fifo_t *)ry_malloc(sizeof(r_xfer_sent_fifo_t));
        tx_para->instID  = instID;
        tx_para->data    = data;
        tx_para->len     = len;
        tx_para->sendCb  = sendCb;
        tx_para->usrdata = usrdata;
        
        if(RY_SUCC != (status = ryos_queue_send(r_xfer_v.txQ, &tx_para, 4))){
            LOG_ERROR("tx queue is full\n");
            status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_QUEUE_FULL);
        }
        
        return status;
    }
#endif

    //r_xfer_msg_t *p = (r_xfer_msg_t*)ry_malloc(len + sizeof(r_xfer_msg_t));
    p = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t));
    if (NULL == p) {
        LOG_ERROR("[r_xfer_send] No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_NO_MEM); 
        goto exit;
    }

    p->instance     = inst;
    p->flag         = R_XFER_FLAG_TX;
    p->len          = len;
    p->txData       = data;
    p->txCb         = sendCb;
    p->txArg        = usrdata;
    p->ttl          = R_XFER_TX_Q_MAX_RETRY_CNT;

    if (R_XFER_GET_STATE_TX(inst) != R_XFER_STATE_IDLE) {
        status = ryos_queue_send(r_xfer_v.txQ, &p, sizeof(r_xfer_msg_t*));
    } else {
        status = ryos_queue_send(r_xfer_v.msgQ, &p, sizeof(r_xfer_msg_t*));
    }
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_send] Add to Queue fail. status = 0x%04x\n", status);
        goto exit;
    }

exit:
    if (status != RY_SUCC) {
        if(sendCb != NULL) {
            sendCb(status, usrdata);
        }
    }

    return status;
    
}


int r_xfer_txMsgHandler(r_xfer_msg_t* pMsg)
{
    //LOG_DEBUG("[r_xfer_txMsgHandler]\n");
    r_xfer_state_machine(pMsg, R_XFER_EVENT_TX_REQ);
    return 0;
}


static void r_xfer_txBusyHandler(r_xfer_inst_t *inst, uint8_t* data, int len, r_xfer_msg_t* pMsg)
{
    uint32_t status = RY_SUCC;

    if(pMsg->ttl > 0) {
        //as pMsg will always be freed after function
        r_xfer_msg_t* p_msg = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t));
        if(p_msg == NULL) {
            status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
            LOG_ERROR("[r_xfer_tx] Add to tx Queue fail.  no memory\n");
            r_xfer_msg_request_abort(pMsg, status);
        } else {
            *p_msg = *pMsg;
            p_msg->ttl--;   //decrease ttl
            status = ryos_queue_send(r_xfer_v.txQ, &p_msg, sizeof(r_xfer_msg_t*));
            if (status != RY_SUCC) {
                status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_QUEUE_SEND);
                LOG_ERROR("[r_xfer_tx] Add to tx Queue fail. ttl:%d\n", p_msg->ttl);
                r_xfer_msg_request_abort(p_msg, status);
                ry_free(p_msg);
            } else {
                LOG_DEBUG("[r_xfer_tx] busy send to queue \n");
            }
        }
        
    } else {
        LOG_DEBUG("[r_xfer_tx] too much retry times\n");
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_BUSY);
        r_xfer_msg_request_abort(pMsg, status);
    }


}

int r_xfer_getLastRxDuration(int instID)
{
    r_xfer_inst_t *p = r_xfer_instance_search(instID);
    return p->lastRxDuration;
}

/**
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int r_xfer_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    int tx_state = 0;
    int pt_sts;
    r_xfer_msg_t* msg = NULL;
    monitor_msg_t* p;
    
    LOG_INFO("[r_xfer_thread] Enter.\r\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_R_XFER);
#endif

    /* Create the R_XFER TX queue */
    status = ryos_queue_create(&r_xfer_v.txQ, R_XFER_TX_QUEUE_SIZE, sizeof(r_xfer_msg_t*));
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_thread] TX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    status = ryos_queue_create(&r_xfer_v.msgQ, R_XFER_MSG_QUEUE_SIZE, sizeof(r_xfer_msg_t*));
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_thread] Msg Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    while (1) {

        //LOG_DEBUG("[r_xfer_thread] Wait New Message...\n");
        status = ryos_queue_recv(r_xfer_v.msgQ, &msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[r_xfer_thread] Queue receive error. Status = 0x%x\r\n", status);
        }

        //LOG_DEBUG("[r_xfer_thread] New Message.\n");

        if (msg->flag & R_XFER_FLAG_RX) {
            r_xfer_onReceived(msg);
        } else if (msg->flag & R_XFER_FLAG_TX) {
            r_xfer_txMsgHandler(msg);
        } else if (msg->flag & R_XFER_FLAG_TIMEOUT) {
            r_xfer_onTimeout(msg);
        } else if (msg->flag & R_XFER_FLAG_DISCONNECT){
            r_xfer_inst_clear(msg);
        } else if (msg->flag & R_XFER_FLAG_MONITOR) {
            p = (monitor_msg_t*)msg->data;
            *(uint32_t*)p->dataAddr = 0;
            FREE_PTR(msg);
        }


        //ryos_delay_ms(10);
    }
}




void r_xfer_instance_restore(void)
{
#if R_XFER_STATISTIC_ENABLE
    
#endif  /* R_XFER_STATISTIC_ENABLE */
}

ry_sts_t r_xfer_instance_add(int instID, hw_write_fn_t write, r_xfer_recv_cb_t readCb, void* usrdata)
{
    r_xfer_inst_t newInst;
    ry_sts_t status;

    ryos_mutex_lock(r_xfer_v.mutex);

    newInst.id           = instID;
    newInst.write        = write;
    newInst.readCb       = readCb;
    newInst.usrData      = usrdata;
    newInst.session_tx      = NULL;
    newInst.session_rx      = NULL;
    newInst.state_tx        = R_XFER_STATE_IDLE;
    newInst.state_rx        = R_XFER_STATE_IDLE;

#ifdef R_XFER_STATISTIC_ENABLE
    newInst.writeSuccCnt = 0;
    newInst.writeFailCnt = 0;
    newInst.readSuccCnt  = 0;
    newInst.readFailCnt  = 0;
    newInst.sessCnt      = 0;
#endif

    status = ry_tbl_add((uint8_t*)&r_xfer_v.tbl, 
                 MAX_R_XFER_INSTANCE_NUM,
                 sizeof(r_xfer_inst_t),
                 (uint8_t*)&newInst,
                 4, 
                 0);


    ryos_mutex_unlock(r_xfer_v.mutex);   

    if (status == RY_SUCC) {
        //arch_event_rule_save((u8 *)&mible_evtRuleTbl, sizeof(mible_evtRuleTbl_t));
        LOG_DEBUG("Transfer instance add success.\r\n");
    } else {
        LOG_WARN("Transfer instance add fail, error code = %x\r\n", status);
    }


    return status;
}

static r_xfer_inst_t *r_xfer_instance_search(int id)
{
    return ry_tbl_search(MAX_R_XFER_INSTANCE_NUM, 
               sizeof(r_xfer_inst_t),
               (uint8_t*)&r_xfer_v.tbl.instance,
               0,
               4,
               (uint8_t*) &id);
}


/**
 * @brief   API to init reliable transfer module.
 *
 * @param   None
 *
 * @return  None
 */
void r_xfer_init(void)
{
    /* Init the Reliable_Xfer Task */
    ryos_threadPara_t para;
    ry_sts_t status;

    memset((void*)&r_xfer_v, 0, sizeof(r_xfer_t));

    /* Add Instance Table */
    //r_xfer_instance_add(ble_reliableSend, R_XFER_INST_ID_BLE_SEC);
    //r_xfer_instance_add(ble_reliableUnsecureSend, R_XFER_INST_ID_BLE_UNSEC);
    //r_xfer_instance_add(ry_hal_uart_send);
    ry_memset((uint8_t*)r_xfer_v.tbl.instance, 0xFF, sizeof(r_xfer_inst_t)*MAX_R_XFER_INSTANCE_NUM);

    status = ryos_mutex_create(&r_xfer_v.mutex);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_init] Mutex Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }

    /* Start Reliable Transfer Thread */
    strcpy((char*)para.name, "r_xfer_thread");
    para.stackDepth = R_XFER_THREAD_STACK_SIZE;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = R_XFER_THREAD_PRIORITY;
    status = ryos_thread_create(&r_xfer_v.thread, r_xfer_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[r_xfer_init] Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }
}

ryos_thread_t* r_xfer_get_thread_handle(void)
{
    return r_xfer_v.thread;
}


typedef struct {
    union {
        struct {
            uint32_t inst1_state          :4;
            uint32_t inst2_state          :4;
            uint32_t reserved             :24;
        } bf;
        uint32_t wordVal;
    } status; 
    
} r_xfer_diag_t;


uint32_t r_xfer_get_diagnostic(void)
{
    r_xfer_diag_t diag;
    r_xfer_inst_t *inst1;
    r_xfer_inst_t *inst2;
    
    inst1 = r_xfer_instance_search(R_XFER_INST_ID_BLE_SEC);
    inst2 = r_xfer_instance_search(R_XFER_INST_ID_BLE_UNSEC);

    diag.status.bf.inst1_state = (inst1->state_tx << 16) | (inst1->state_rx << 0);
    diag.status.bf.inst2_state = (inst2->state_tx << 16) | (inst2->state_rx << 0);

    return diag.status.wordVal;
}

ryos_thread_t* r_xfer_get_thread(void)
{
    return r_xfer_v.thread;
}


void r_xfer_msg_send(uint32_t msg, int len, void* data)
{
    ry_sts_t status;
    r_xfer_msg_t *p = (r_xfer_msg_t*)ry_malloc(sizeof(r_xfer_msg_t)+len);
    if(p == NULL) {
        LOG_ERROR("[r_xfer_msg_send]-{%s}\n", ERR_STR_MALLOC_FAIL);
        return;
    }
    
    ry_memset(p, 0, sizeof(r_xfer_msg_t)+len);
    p->flag = msg;
    p->len  = len;
    ry_memcpy(p->data, data, len);
    
    if (RY_SUCC != (status = ryos_queue_send(r_xfer_v.msgQ, &p, 4))) {
        LOG_ERROR("[r_xfer_msg_send] Add to Queue fail. status = 0x%04x\n", status);
    }
}


