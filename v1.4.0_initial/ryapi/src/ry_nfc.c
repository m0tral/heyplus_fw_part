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
#include "app_config.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

#if (RY_SE_ENABLE == TRUE)


#include "ryos.h"
#include "ryos_timer.h"
#include "seApi.h"
#include "ry_nfc.h"
#include "nfc_pn80t.h"
#include "scheduler.h"

#include "ry_hal_i2c.h"
#include "timer_management_service.h"
#include "device_management_service.h"
#include "SeApi_Int.h"


/*********************************************************************
 * CONSTANTS
 */ 

/**
 * @brief  Definition for Max Cards
 */
#define MAX_CARD_NUM             4  
#define MAX_AID_LEN              16

#define CARD_STATUS_IDLE         0
#define CARD_STATUS_ACTIVATE     1

 /*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    u8_t             aid[MAX_AID_LEN];
    u8_t             aid_len;
    int              state;
} ry_nfc_card_t;





typedef struct {
    ry_nfc_state_t     curState;
    u16_t              nextOp;
    //ry_nfc_init_done_t initDoneCb;
    ry_nfc_card_t*     curCard;
} ry_nfc_ctx_t;


typedef void ( *ry_nfc_stateHandler_t )(u16_t msg_type, void* msg_data); 


/** @brief  Reliable transfer state machine */
typedef struct
{
    ry_nfc_state_t        curState;  /*! The current state of mi service */
    u16_t                 event;     /*! The event ID */
    ry_nfc_stateHandler_t handler;   /*! The corresponding event handler */
} ry_nfc_stateMachine_t;



 
/*********************************************************************
 * LOCAL VARIABLES
 */
ry_nfc_ctx_t     ry_nfc_ctx_v;
ryos_thread_t    *ry_nfc_thread_handle;
ry_queue_t              *ry_nfc_msgQ;
static ryos_semaphore_t *ry_nfc_sem;

extern ryos_thread_t	*gui_thread3_handle;
extern ryos_thread_t    *hrm_detect_thread_handle;
extern ryos_thread_t    *sensor_alg_thread_handle;
extern ryos_mutex_t     *se_mutex;
ry_nfc_init_done_t      ry_nfc_initdone_cb;


/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void ry_nfc_idle_handler(u16_t msg_type, void* msg_data);
static void ry_nfc_initialized_handler(u16_t msg_type, void* msg_data);
static void ry_nfc_ceStateHandler(u16_t msg_type, void* msg_data);
static void ry_nfc_readerStateHandler(u16_t msg_type, void* msg_data);
static void ry_nfc_lowpowerStateHandler(u16_t msg_type, void* msg_data);



static const ry_nfc_stateMachine_t ry_nfc_stateMachine[] = 
{
     /*  current state,                       event,                       handler    */
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_GET_CPLC,         ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_CE_ON,            ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_CE_OFF,           ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_READER_ON,        ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_READER_OFF,       ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_POWER_ON,         ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_POWER_OFF,        ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_POWER_DOWN,       ry_nfc_idle_handler },
    { RY_NFC_STATE_IDLE,               RY_NFC_MSG_TYPE_RESET,            ry_nfc_idle_handler },


    { RY_NFC_STATE_INITIALIZED,        RY_NFC_MSG_TYPE_GET_CPLC,         ry_nfc_initialized_handler },
    { RY_NFC_STATE_INITIALIZED,        RY_NFC_MSG_TYPE_CE_ON,            ry_nfc_initialized_handler },
    { RY_NFC_STATE_INITIALIZED,        RY_NFC_MSG_TYPE_READER_ON,        ry_nfc_initialized_handler },
    { RY_NFC_STATE_INITIALIZED,        RY_NFC_MSG_TYPE_POWER_OFF,        ry_nfc_initialized_handler },
    { RY_NFC_STATE_INITIALIZED,        RY_NFC_MSG_TYPE_POWER_DOWN,       ry_nfc_initialized_handler },

    { RY_NFC_STATE_CE_ON,              RY_NFC_MSG_TYPE_GET_CPLC,         ry_nfc_ceStateHandler },
    { RY_NFC_STATE_CE_ON,              RY_NFC_MSG_TYPE_CE_OFF,           ry_nfc_ceStateHandler },
    { RY_NFC_STATE_CE_ON,              RY_NFC_MSG_TYPE_READER_ON,        ry_nfc_ceStateHandler },
    { RY_NFC_STATE_CE_ON,              RY_NFC_MSG_TYPE_POWER_OFF,        ry_nfc_ceStateHandler },
    { RY_NFC_STATE_CE_ON,              RY_NFC_MSG_TYPE_POWER_DOWN,       ry_nfc_ceStateHandler },

     
    { RY_NFC_STATE_READER_POLLING,     RY_NFC_MSG_TYPE_GET_CPLC,         ry_nfc_readerStateHandler },
    { RY_NFC_STATE_READER_POLLING,     RY_NFC_MSG_TYPE_READER_OFF,       ry_nfc_readerStateHandler },
    { RY_NFC_STATE_READER_POLLING,     RY_NFC_MSG_TYPE_CE_ON,            ry_nfc_readerStateHandler },
    { RY_NFC_STATE_READER_POLLING,     RY_NFC_MSG_TYPE_POWER_OFF,        ry_nfc_readerStateHandler },
    { RY_NFC_STATE_READER_POLLING,     RY_NFC_MSG_TYPE_POWER_DOWN,       ry_nfc_readerStateHandler },

    { RY_NFC_STATE_LOW_POWER,          RY_NFC_MSG_TYPE_GET_CPLC,         ry_nfc_lowpowerStateHandler },
    { RY_NFC_STATE_LOW_POWER,          RY_NFC_MSG_TYPE_READER_OFF,       ry_nfc_lowpowerStateHandler },
    { RY_NFC_STATE_LOW_POWER,          RY_NFC_MSG_TYPE_CE_ON,            ry_nfc_lowpowerStateHandler },
    { RY_NFC_STATE_LOW_POWER,          RY_NFC_MSG_TYPE_POWER_OFF,        ry_nfc_lowpowerStateHandler },
    { RY_NFC_STATE_LOW_POWER,          RY_NFC_MSG_TYPE_READER_ON,        ry_nfc_lowpowerStateHandler },
    { RY_NFC_STATE_LOW_POWER,          RY_NFC_MSG_TYPE_POWER_ON,         ry_nfc_lowpowerStateHandler },
};



/**
 * @brief   API to Enable/Disable CE mode
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_nfc_ce(u8_t fEnable)
{
    tSEAPI_STATUS status;

    if (fEnable) {
        status = SeApi_SeSelectWithReader(SE_ESE, 0);
    } else {
        status = SeApi_SeSelectWithReader(SE_NONE, 0);
    }

    return (status == 0) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_SE,status);
}


/**
 * @brief   API to Enable/Disable Reader mode
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_nfc_reader(u8_t fEnable)
{
    tSEAPI_STATUS status;

    if (fEnable) {
        status = SeApi_SeSelectWithReader(SE_ESE, 1);
    }  else {
        status = SeApi_SeSelectWithReader(SE_NONE, 1);
    }

    return (status == 0) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_SE,status);

}


/**
 * @brief   Callback function from ble stack.
 *
 * @param   None
 *
 * @return  None
 */
void ry_nfc_stack_callback(u16_t evt, void* para)
{
    switch (evt) {
        
        case NFC_STACK_EVT_INIT_DONE:
            LOG_DEBUG("[nfc]: init done.\r\n");
            //touch_enable();
            ry_nfc_ctx_v.curState = RY_NFC_STATE_INITIALIZED;
            if (ry_nfc_initdone_cb) {
                ry_nfc_initdone_cb();
            }

            nfc_i2c_instance = I2C_IDX_NFC;
            ry_hal_i2cm_init((ry_i2c_t)nfc_i2c_instance);


            if (ry_nfc_ctx_v.nextOp) {
                ry_nfc_msg_send(ry_nfc_ctx_v.nextOp, NULL);
                ry_nfc_ctx_v.nextOp = RY_NFC_MSG_TYPE_NONE;
            }

            /* Notiify system scheduler */
            ry_sched_msg_send(IPC_MSG_TYPE_NFC_INIT_DONE, 0, NULL);

            if (sensor_alg_thread_handle) {
                ryos_thread_resume(sensor_alg_thread_handle);
            }
            break;

        default:
            break;
            
     
    }
}


void ry_nfc_msg_send(u8_t msg_type, void* data)
{
    ry_sts_t status = RY_SUCC;
    ry_nfc_msg_t *p = (ry_nfc_msg_t*)ry_malloc(sizeof(ry_nfc_msg_t));
    if (NULL == p) {

        LOG_ERROR("[ry_nfc_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }   

    if (msg_type != RY_NFC_MSG_TYPE_MONITOR) {
        LOG_DEBUG("[%s] NFC cmd: %d\r\n", __FUNCTION__, msg_type);
    }

    p->msg_type = msg_type;
    p->msg_data = data;

    if (RY_SUCC != (status = ryos_queue_send(ry_nfc_msgQ, &p, 4))) {
        LOG_ERROR("[ry_nfc_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
        //ry_free(data);
        ry_free((void*)p);
    }        
}


void ry_nfc_msgHandler(ry_nfc_msg_t *msg)
{
     int i, size;

    //LOG_DEBUG("[r_xfer_state_machine] inst_state:%d, evt:0x%04x\r\n", pMsg->instance->state, evt);

    size = sizeof(ry_nfc_stateMachine)/sizeof(ry_nfc_stateMachine_t);

    /* Search an appropriate event handler */
    for (i = 0; i < size; i++) {
        
        if (ry_nfc_stateMachine[i].curState == ry_nfc_ctx_v.curState &&
            ry_nfc_stateMachine[i].event == msg->msg_type) {
            /* call event handler */
            ry_nfc_stateMachine[i].handler(msg->msg_type, msg->msg_data);
            goto exit;
        }
    }

    if (msg->msg_type == RY_NFC_MSG_TYPE_MONITOR){
        monitor_msg_t* mnt_msg = (monitor_msg_t*)msg->msg_data;
        if (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_NFC){
            *(uint32_t *)mnt_msg->dataAddr = 0;    
			if (msg->msg_data) {
		        ry_free(msg->msg_data);
		        msg->msg_data = NULL;
		    }
        }
    }

exit:
    /* Always don't forget to free the data buffer */
    
    ry_free((u8*)msg);
    return;
}



void ry_nfc_do_init(u32_t needUpdate)
{
    //touch_disable();

    if (sensor_alg_thread_handle) {
        ryos_thread_suspend(sensor_alg_thread_handle);
    }
    
    nfc_init(ry_nfc_stack_callback, needUpdate);
}


void ry_nfc_idle_handler(u16_t msg_type, void* msg_data)
{
    if (msg_type >= RY_NFC_MSG_TYPE_GET_CPLC && 
        msg_type <= RY_NFC_MSG_TYPE_READER_OFF) {

        /* Do the init first */
        ry_nfc_ctx_v.curState = RY_NFC_STATE_OPERATING;
        ry_nfc_ctx_v.nextOp = msg_type;
        ry_nfc_do_init((u32_t)msg_data);
        
    } else if (msg_type == RY_NFC_MSG_TYPE_POWER_OFF) {
        /* TODO: should use deinit() */
        nfc_power_off();

    } else if (msg_type == RY_NFC_MSG_TYPE_POWER_DOWN) {
        /* TODO: should use deinit() */
        nfc_power_down();

    } else if (msg_type == RY_NFC_MSG_TYPE_POWER_ON) {
        ry_nfc_ctx_v.curState = RY_NFC_STATE_OPERATING;
        ry_nfc_do_init((u32_t)msg_data);
    } else if (msg_type == RY_NFC_MSG_TYPE_RESET) {
        nfc_deinit(1);
    }
}


void ry_nfc_initialized_handler(u16_t msg_type, void* msg_data)
{
    ry_sts_t status;
    switch (msg_type) {
        case RY_NFC_MSG_TYPE_CE_ON:
            status = ry_nfc_ce(1);
            if (status == RY_SUCC) {
                ry_nfc_ctx_v.curState = RY_NFC_STATE_CE_ON;
                ry_sched_msg_send(IPC_MSG_TYPE_NFC_CE_ON_EVT, 0, NULL);
            }
            break;


        case RY_NFC_MSG_TYPE_READER_ON:
            status = ry_nfc_reader(1);
            if (status == RY_SUCC) {
                ry_nfc_ctx_v.curState = RY_NFC_STATE_READER_POLLING;
                ry_sched_msg_send(IPC_MSG_TYPE_NFC_READER_ON_EVT, 0, NULL);
            }
            break;


        case RY_NFC_MSG_TYPE_GET_CPLC:
            break;

        case RY_NFC_MSG_TYPE_POWER_OFF:
            LOG_DEBUG("[ry_nfc_initialized_handler] power off.\r\n");
            nfc_deinit(1);
            ry_nfc_ctx_v.curState = RY_NFC_STATE_IDLE;
            ry_sched_msg_send(IPC_MSG_TYPE_NFC_POWER_OFF_EVT, 0, NULL);
            break;

        case RY_NFC_MSG_TYPE_POWER_DOWN:
            LOG_DEBUG("[ry_nfc_initialized_handler] power down.\r\n");
            nfc_deinit(0);
            ry_nfc_ctx_v.curState = RY_NFC_STATE_LOW_POWER;
            ry_sched_msg_send(IPC_MSG_TYPE_NFC_LOW_POWER_EVT, 0, NULL);
            break;

        default:
            break;
    }
}

void ry_nfc_readerStateHandler(u16_t msg_type, void* msg_data)
{
    ry_sts_t status;
    u8_t off_para = 0;
	
    switch (msg_type) {
        case RY_NFC_MSG_TYPE_CE_ON:
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_OFF, NULL);
            ry_nfc_ctx_v.nextOp = msg_type;
            break;


        case RY_NFC_MSG_TYPE_READER_OFF:
            status = ry_nfc_reader(0);
            if (status == RY_SUCC) {
                ry_nfc_ctx_v.curState = RY_NFC_STATE_INITIALIZED;
                ry_sched_msg_send(IPC_MSG_TYPE_NFC_READER_OFF_EVT, 0, NULL);
            }

            if (ry_nfc_ctx_v.nextOp) {
                ry_nfc_msg_send(ry_nfc_ctx_v.nextOp, NULL);
                ry_nfc_ctx_v.nextOp = RY_NFC_MSG_TYPE_NONE;
            }
            break;


        case RY_NFC_MSG_TYPE_GET_CPLC:
            break;

        case RY_NFC_MSG_TYPE_POWER_OFF:
        case RY_NFC_MSG_TYPE_POWER_DOWN:
            off_para = 0;
            if (msg_type == RY_NFC_MSG_TYPE_POWER_OFF) {
                off_para  = 1;
            }
            //ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_OFF, NULL);
            //ry_nfc_ctx_v.nextOp = msg_type;

            status = ry_nfc_reader(0);
            ryos_delay_ms(500);

            if (status == RY_SUCC) {
                ry_nfc_ctx_v.curState = RY_NFC_STATE_INITIALIZED;

                
                nfc_deinit(off_para);
                if (off_para) {
                    LOG_DEBUG("[ry_nfc_readerStateHandler] power off.\r\n");
                    ry_nfc_ctx_v.curState = RY_NFC_STATE_IDLE;
                    ry_sched_msg_send(IPC_MSG_TYPE_NFC_POWER_OFF_EVT, 0, NULL);
                } else {
                    LOG_DEBUG("[ry_nfc_readerStateHandler] low power.\r\n");
                    ry_nfc_ctx_v.curState = RY_NFC_STATE_LOW_POWER;
                    ry_sched_msg_send(IPC_MSG_TYPE_NFC_LOW_POWER_EVT, 0, NULL);
                }
            }
            break;

        default:
            break;
    }
}

void ry_nfc_ceStateHandler(u16_t msg_type, void* msg_data)
{
    ry_sts_t status;
    u8_t off_para = 0;
    switch (msg_type) {
        case RY_NFC_MSG_TYPE_CE_OFF:
            status = ry_nfc_ce(0);
            LOG_DEBUG("[ry_nfc_ceStateHandler] CE off. status = %d\r\n", status);
            if (status == RY_SUCC) {
                ry_nfc_ctx_v.curState = RY_NFC_STATE_INITIALIZED;
            }

            if (ry_nfc_ctx_v.nextOp) {
                ry_nfc_msg_send(ry_nfc_ctx_v.nextOp, NULL);
                ry_nfc_ctx_v.nextOp = RY_NFC_MSG_TYPE_NONE;
            }
            break;


        case RY_NFC_MSG_TYPE_READER_ON:
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_OFF, NULL);
            ry_nfc_ctx_v.nextOp = msg_type;
            break;


        case RY_NFC_MSG_TYPE_GET_CPLC:
            break;

        case RY_NFC_MSG_TYPE_POWER_OFF:
        case RY_NFC_MSG_TYPE_POWER_DOWN:
            off_para = 0;
            if (msg_type == RY_NFC_MSG_TYPE_POWER_OFF) {
                off_para  = 1;
            }
            
            LOG_DEBUG("[ry_nfc_ceStateHandler] Enter RY_NFC_MSG_TYPE_POWER_OFF\r\n");
            status = ry_nfc_ce(0);
            LOG_DEBUG("[ry_nfc_ceStateHandler] CE off. status = %d\r\n", status);
            ryos_delay_ms(500);
            
            if (status == RY_SUCC) {
                ry_nfc_ctx_v.curState = RY_NFC_STATE_INITIALIZED;

                
                nfc_deinit(off_para);
                if (off_para) {
                    LOG_DEBUG("[ry_nfc_ceStateHandler] power off.\r\n");
                    ry_nfc_ctx_v.curState = RY_NFC_STATE_IDLE;
                    ry_sched_msg_send(IPC_MSG_TYPE_NFC_POWER_OFF_EVT, 0, NULL);
                } else {
                    LOG_DEBUG("[ry_nfc_ceStateHandler] low power.\r\n");
                    ry_nfc_ctx_v.curState = RY_NFC_STATE_LOW_POWER;
                    ry_sched_msg_send(IPC_MSG_TYPE_NFC_LOW_POWER_EVT, 0, NULL);
                }
            }
            break;

        default:
            break;
    }
}


void ry_nfc_lowpowerStateHandler(u16_t msg_type, void* msg_data)
{
    ry_sts_t status;
    switch (msg_type) {
        
        case RY_NFC_MSG_TYPE_READER_OFF:
        case RY_NFC_MSG_TYPE_GET_CPLC:
            break;

        case RY_NFC_MSG_TYPE_POWER_OFF:
            nfc_power_off();
            ry_nfc_ctx_v.curState = RY_NFC_STATE_IDLE;
            break;

        case RY_NFC_MSG_TYPE_POWER_ON:
            ry_nfc_ctx_v.curState = RY_NFC_STATE_OPERATING;
            ry_nfc_do_init((u32_t)msg_data);
            break;

        case RY_NFC_MSG_TYPE_READER_ON:
        case RY_NFC_MSG_TYPE_CE_ON:
            /* Do the init first */
            ry_nfc_ctx_v.curState = RY_NFC_STATE_OPERATING;
            ry_nfc_ctx_v.nextOp = msg_type;
            ry_nfc_do_init((u32_t)msg_data);
            break;

        default:
            break;
    }
}


/**
 * @brief   API to get the SE wired state
 *
 * @param   None
 *
 * @return  1 indicating ENABLE and 0 indicating DISABLE
 */
u8_t ry_nfc_is_wired_enable(void)
{
    return SeApi_isWiredEnable();
}


/**
 * @brief   NFC thread
 *
 * @param   None
 *
 * @return  None
 */
extern int ble_inited;
int ry_nfc_thread(void* arg)
{
    ry_nfc_msg_t *msg;
    ry_sts_t status = RY_SUCC;

    LOG_DEBUG("[ry_nfc_thread] Enter.\n");
    
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_NFC);
#endif

    //nfc_deinit();

    //while (ble_inited == 0) {
    //    ryos_delay_ms(100);
    //}

    //nfc_init(ry_nfc_stack_callback);
    
    while (1) {

        status = ryos_queue_recv(ry_nfc_msgQ, &msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_nfc_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {           
            ry_nfc_msgHandler(msg);
        }

        //ryos_delay_ms(10);
    }
}


bool ry_nfc_is_initialized(void)
{
    return (ry_nfc_ctx_v.curState >= RY_NFC_STATE_INITIALIZED); 
}

u8_t ry_nfc_get_state(void)
{
    return (ry_nfc_ctx_v.curState); 
}


void ry_nfc_set_initdone_cb(ry_nfc_init_done_t initDone)
{
    ry_nfc_initdone_cb = initDone;
}



ry_sts_t ry_nfc_se_channel_open(void)
{
    tSEAPI_STATUS status;
    status = SeApi_WiredEnable(1);
    if (SEAPI_STATUS_OK == status || SEAPI_STATUS_NO_ACTION_TAKEN == status) {
        return RY_SUCC;
    } else {
        return RY_SET_STS_VAL(RY_CID_SE, status);
    }
}

ry_sts_t ry_nfc_se_channel_close(void)
{
    tSEAPI_STATUS status;
    status = SeApi_WiredEnable(0);
    if (SEAPI_STATUS_OK == status || SEAPI_STATUS_NO_ACTION_TAKEN == status) {
        return RY_SUCC;
    } else {
        return RY_SET_STS_VAL(RY_CID_SE, status);
    }
}


extern u8_t nfc_need_to_reset;

void ry_nfc_sync_operate(u8_t msg_type, ry_nfc_state_t expect_state)
{
    int cnt = 0;
    //LOG_DEBUG("[ry_nfc_sync_operate] Begin - Free Heap %d, Min Avail Heap %d\r\n", ryos_get_free_heap(), ryos_get_min_available_heap_size());
    ryos_mutex_lock(se_mutex);
    ry_nfc_msg_send(msg_type, NULL);
    while(ry_nfc_get_state() != expect_state) {
        ryos_delay_ms(100);
        if (nfc_need_to_reset) {
            break;
        }
        
        if (cnt++ >= 80) {
            add_reset_count(CARD_RESTART_COUNT);
            LOG_ERROR("[%s] NFC sync operation timeout. msg:%d, expect_state:%d\r\n",
                __FUNCTION__, msg_type, expect_state);
            ry_system_reset();
        }
    }
    ryos_mutex_unlock(se_mutex);

    if (nfc_need_to_reset) {
        SeApi_Int_CleanUpStack();
        //ryos_delay_ms(1000);
        char *buff = (char*)ry_malloc(4096);
        ryos_get_thread_list(buff);
        LOG_DEBUG("\n\n%s \n\n\n", buff);
        ry_free(buff);
    }

    LOG_DEBUG("[ry_nfc_sync_operate] End - Free Heap %d, Min Avail Heap %d\r\n", ryos_get_free_heap(), ryos_get_min_available_heap_size());
}

void ry_nfc_sync_poweroff(void)
{
    ry_nfc_sync_operate(RY_NFC_MSG_TYPE_POWER_OFF, RY_NFC_STATE_IDLE);
}

void ry_nfc_sync_lowpower(void)
{
    ry_nfc_sync_operate(RY_NFC_MSG_TYPE_POWER_DOWN, RY_NFC_STATE_LOW_POWER);
}


void ry_nfc_sync_poweron(void)
{
    ry_nfc_sync_operate(RY_NFC_MSG_TYPE_POWER_ON, RY_NFC_STATE_INITIALIZED);
}

void ry_nfc_sync_ce_on(void)
{
    ry_nfc_sync_operate(RY_NFC_MSG_TYPE_CE_ON, RY_NFC_STATE_CE_ON);
}

void ry_nfc_sync_reader_on(void)
{
    ry_nfc_sync_operate(RY_NFC_MSG_TYPE_READER_ON, RY_NFC_STATE_READER_POLLING);
}




/**
 * @brief   API to init Fingerprint module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_se_init(ry_nfc_init_done_t initDone)
{
    ryos_threadPara_t para;
    ry_sts_t status;
    
    ry_memset((void*)&ry_nfc_ctx_v, 0, sizeof(ry_nfc_ctx_t));

    ry_nfc_initdone_cb = initDone;


    /* Create the RYNFC message queue */
    status = ryos_queue_create(&ry_nfc_msgQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_nfc_thread]: RX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }


    /* Create the Semaphore to Control TXD process */
    status = ryos_semaphore_create(&ry_nfc_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_nfc_thread]: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
        return NULL;
    } 

    /* Start RY BLE Thread */
    strcpy((char*)para.name, "ry_nfc_thread");
    para.stackDepth = 300;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 4;
    status = ryos_thread_create(&ry_nfc_thread_handle, ry_nfc_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_se_init]: Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }
    
    return status; 
}


/***********************************Test*****************************************/

#ifdef RY_NFC_TEST

u32_t ry_nfc_test_timer_id;

void ry_nfc_test_timer_cb(u32_t timerId, void* userData)
{
    LOG_INFO("Timer callback. Timer id = %d, UserData = %d \r\n", timerId, userData);

    ry_fp_test_enrollment();
}

void ry_nfc_test(void)
{
    ry_nfc_test_timer_id = ry_timer_create(0);
    if (ry_nfc_test_timer_id == RY_ERR_TIMER_ID_INVALID) {
        LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
    }

    ry_timer_start(ry_nfc_test_timer_id, 1000, ry_nfc_test_timer_cb, NULL);
}

#endif  /* RY_NFC_TEST */


#endif /* (RY_SE_ENABLE == TRUE) */
