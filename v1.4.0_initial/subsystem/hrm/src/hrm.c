/*
* Copyright (c), Ryeex Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "app_config.h"
#include "ry_mcu_config.h"
#include "ryos_timer.h"
#include "app_interface.h"

/* Board */
#include "board.h"
#include "ry_hal_inc.h"

/* Application Specific */
#include "hqerror.h"
#include "ppsi26x.h"
#include "am_bsp_gpio.h"
#include "board_control.h"
#include "sensor_alg.h"
#include "hrm.h"
#include "data_management_service.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif


/*********************************************************************
 * CONSTANTS
 */ 

#define HRM_THREAD_PRIORITY    3

 /*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
// ryos_semaphore_t *hrm_sem;
// ryos_thread_t    *hrm_thread_handle;
ryos_thread_t    *hrm_detect_thread_handle;

hrm_raw_t         hrm_raw;
hrm_status_t      hrm_st;

ry_queue_t       *hrm_msgQ;
static u32_t      hrm_timer_id;

static void hrm_detect_process(void);

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to set_hrm_working_mode
 *
 * @param   hrm working mode, refer to hrm_mode_t
 *
 * @return  None
 */
void set_hrm_working_mode(uint8_t working_mode)
{
    hrm_st.working_mode = working_mode;
}

/**
 * @brief   API to get_hrm_working_mode
 *
 * @param   None
 *
 * @return  hrm working mode, refer to hrm_mode_t
 */
uint8_t get_hrm_working_mode(void)
{
    return hrm_st.working_mode;
}

/**
 * @brief   API to get_hrm_weared_status
 *
 * @param   None
 *
 * @return  hrm weared status
 */
uint8_t get_hrm_weared_status(void)
{
    return hrm_st.weared;
}

/**
 * @brief   API to get_hrm_st_is_prepare
 *
 * @param   None
 *
 * @return  hrm weared status
 */
uint8_t hrm_st_is_prepare(void)
{
    return hrm_st.is_prepare;
}

/**
 * @brief   API to clear_hrm_st_prepare
 *
 * @param   None
 *
 * @return  None
 */
void clear_hrm_st_prepare(void)
{
    hrm_st.is_prepare = 0;
}

void hrm_patch_for_gsensor_sample(void* arg)
{
    clear_hrm_st_prepare();
    // LOG_DEBUG("[hrm_patch_for_gsensor_sample] Enter.\n");
}

void hrm_thread_entry(uint32_t mode)
{
    power_hrm_on(1);

    ry_hal_i2cm_init(I2C_IDX_HR);
    uint8_t pData[16] = {0};
    uint8_t tx = 0x28;
		
	init_pps26x_sensor(mode);
	hrm_st.start_mode = mode;
    hrm_st.is_prepare = 1;
    ry_timer_start(hrm_timer_id, 2000, hrm_patch_for_gsensor_sample, NULL);
    ry_hal_i2cm_tx(I2C_IDX_HR, &tx, 1);
    ry_hal_i2cm_rx(I2C_IDX_HR, pData, 3);
    uint8_t sensor_id = (*(uint32_t*)pData >> 7) & 0xff;
    if (0x2a == sensor_id) {
        LOG_DEBUG("[hrm sensor]id right: 0x28 %x\r\n", sensor_id);
    }
    else {
        LOG_DEBUG("[hrm sensor]id at: 0x28 is %x\r\n", sensor_id);
    }
}


void ry_hrm_msg_send(u8_t msg_type, void* data)
{
    ry_sts_t status = RY_SUCC;
    ry_hrm_msg_t hrm_queue_msg;

    hrm_queue_msg.type = msg_type;
    if(msg_type == HRM_MSG_CMD_MONITOR) {
        hrm_queue_msg.data.monitor = *(monitor_msg_t*)data;
    } else {
        hrm_queue_msg.data.p_data = data;
    }

    // LOG_DEBUG("[ry_hrm_msg_send]----------evt:%d\n", msg_type);

    status = ryos_queue_send(hrm_msgQ, &hrm_queue_msg, sizeof(ry_hrm_msg_t));
    if (RY_SUCC != status) {
        //if(hrm_queue_msg.type == HRM_MSG_CMD_GPIO_ISR) {
        if(xPortIsInsideInterrupt()) {
            LOG_DEBUG("[ry_hrm_msg_send] Add Queue failed. evt:%d, status:0x%04x\n",msg_type, status);
        } else {
            LOG_ERROR("[ry_hrm_msg_send] Add Queue failed. evt:%d, status:0x%04x\n",msg_type, status);
        }
    }
}


void hrm_retry_timeout_handler(void* arg)
{
    LOG_DEBUG("[hrm_retry_timeout_handler]: transfer timeout.\n");
    _start_func_hrm(HRM_START_AUTO);
}

void hrm_msgHandler(ry_hrm_msg_t *msg)
{
    int i, size;
    monitor_msg_t* mnt_msg = NULL;

//    LOG_DEBUG("[ry_hrm_msg_recv] ++++++++++evt:%d\n", msg->type);

    // LOG_DEBUG("[hrm_msgHandler] msg->msg_type: %d, msg->data: %d\r\n", msg->type, msg->data);
    switch (msg->type){
        case HRM_MSG_CMD_AUTO_SAMPLE_STOP:
            if (get_hrm_working_mode() == HRM_MODE_AUTO_SAMPLE){
                hrm_st.session = 0;
                if (get_hrm_weared_status() == WEAR_DET_SUCC){ 
                    ry_data_msg_send(DATA_SERVICE_MSG_STOP_HEART_AUTO, 0, NULL);
                }
                pcf_func_hrm();
            }
            LOG_DEBUG("[HRM_MSG_CMD_AUTO_SAMPLE_STOP] stop: %d\r\n", msg->type);
            break;

        case HRM_MSG_CMD_AUTO_SAMPLE_START:
            LOG_DEBUG("[HRM_MSG_CMD_AUTO_SAMPLE_START] start: %d\r\n", msg->type);
            if (get_hrm_state() != PM_ST_POWER_START){
                _start_func_hrm(HRM_START_AUTO);
                set_hrm_working_mode(HRM_MODE_AUTO_SAMPLE);
                hrm_st.session = 1;
            }
            break;

        case HRM_MSG_CMD_WEAR_DETECT_SAMPLE_STOP:
            if (get_hrm_working_mode() == HRM_MODE_AUTO_SAMPLE){
                hrm_st.session = 0;
                pcf_func_hrm();   
            }
            LOG_DEBUG("[HRM_MSG_CMD_WEAR_DETECT_SAMPLE_STOP] stop: %d\r\n", msg->type);
            break;

        case HRM_MSG_CMD_WEAR_DETECT_SAMPLE_START:
            LOG_DEBUG("[HRM_MSG_CMD_WEAR_DETECT_SAMPLE_START] start: %d\r\n", msg->type);
            if (get_hrm_state() != PM_ST_POWER_START){
                _start_func_hrm(HRM_START_AUTO);
                set_hrm_working_mode(HRM_MODE_AUTO_SAMPLE);
                hrm_st.session = 1;                
            }
            break;

        case HRM_MSG_CMD_MANUAL_SAMPLE_START:
            LOG_DEBUG("[HRM_MSG_CMD_MANUAL_SAMPLE_START] start: %d\r\n", msg->type);
            _start_func_hrm(HRM_START_AUTO);
            set_hrm_working_mode(HRM_MODE_MANUAL);   
            hrm_st.session = 1;
            // ry_data_msg_send(DATA_SERVICE_MSG_START_HEART, 0, NULL);
            break;

        case HRM_MSG_CMD_MANUAL_SAMPLE_STOP:
            hrm_st.session = 0;            
            //send msg of data store here
            ry_data_msg_send(DATA_SERVICE_MSG_STOP_HEART, 0, NULL);
            pcf_func_hrm();   
            LOG_DEBUG("[HRM_MSG_CMD_MANUAL_SAMPLE_STOP] stop: %d\r\n", msg->type);
            break;

        case HRM_MSG_CMD_SLEEP_SAMPLE_START:
            LOG_DEBUG("[HRM_MSG_CMD_SLEEP_SAMPLE_START] start: %d\r\n", msg->type);
#if (SLEEP_CALC_DEBUG)
            _start_func_hrm(HRM_START_FIXED);
#else
            _start_func_hrm(HRM_START_AUTO);
#endif
            set_hrm_working_mode(HRM_MODE_SLEEP_LOG);   
            hrm_st.session = 1;
            // ry_data_msg_send(DATA_SERVICE_MSG_START_HEART, 0, NULL);
            break;

        case HRM_MSG_CMD_SLEEP_SAMPLE_STOP:
            //send msg of data store here
            hrm_st.session = 0;            
            ry_data_msg_send(DATA_SERVICE_MSG_STOP_HEART_AUTO, 0, NULL);
            pcf_func_hrm();   
            LOG_DEBUG("[HRM_MSG_CMD_SLEEP_SAMPLE_STOP] stop: %d\r\n", msg->type);
            break;

        case HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START:
            LOG_DEBUG("[HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START] start: %d\r\n", msg->type);
            _start_func_hrm(HRM_START_AUTO);
            set_hrm_working_mode(HRM_MODE_SPORT_RUN);   
            hrm_st.session = 1;
            // ry_data_msg_send(DATA_SERVICE_MSG_START_HEART, 0, NULL);
            break;

        case HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP:
            //send msg of data store here
            hrm_st.session = 0;            
            ry_data_msg_send(DATA_SERVICE_MSG_STOP_HEART_AUTO, 0, NULL);
            pcf_func_hrm();   
            LOG_DEBUG("[HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP] stop: %d\r\n", msg->type);
            break;

        case HRM_MSG_CMD_MONITOR:
            mnt_msg = &msg->data.monitor;
            if (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_HRM){
                *(uint32_t *)mnt_msg->dataAddr = 0;
            }
            break;

        case HRM_MSG_CMD_GPIO_ISR:
            hrm_detect_process();
            break;

        default:
            break;
    }

    return;
}


/**
 * @brief   HRM thread solve gpio isr
 *
 * @param   None
 *
 * @return  None
 */
static void hrm_detect_process(void)
{
    static uint8_t weared_last;
    uint8_t weared_st;

    ppsi26x_sensor_task(NULL);

    if (hrm_st.start_mode == HRM_START_AUTO){
        weared_st = alg_get_weard_state();
        if ((weared_st == 0) && (weared_st != weared_last)){
            if (get_hrm_working_mode() != HRM_MODE_SPORT_RUN){
                hrm_weared_dim_reset();
            }
#if 0           // hrm weared-2-unweared detect function not works well, disable it     
            if (alg_status_is_in_sleep()){
                LOG_INFO("wake up from hrm\n");
                ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_HRM_CHECK_WAKEUP, 0, NULL);
            }
#endif                
        }

        switch (get_ppg_stage()){
            case HRM_HW_ST_WEARE_DETECT:
                hrm_st.weared = ppg_weard_detect();
                alg_set_weard_state(1);
                break;

            case HRM_HW_ST_AUTO_DIM:
                if (hrm_st.weared == WEAR_DET_SUCC){
                    ppg_dim();
                }
                else if (hrm_st.weared == WEAR_DET_FAIL){
                    pcf_func_hrm();
                    hrm_st.is_prepare = 0;
                    // if (hrm_st.session != 0){
                    //     ry_timer_start(hrm_timer_id, 1000, hrm_retry_timeout_handler, NULL);
                    // }
                    // LOG_DEBUG("[hrm_detect_thread]: wear_detect_fail, close_hrm\r\n");
                }
                break;
                
            case HRM_HW_ST_MEASURE:
                hrm_get_raw(&hrm_raw);
                hrm_st.is_prepare = 0;
                break;
            
            default:
                break;
        }

        // LOG_DEBUG("[hrm_detect_thread]alg_st: %d, alg_st_last: %d, hrm_st.weared: %d\r\n", \
            weared_st, weared_last, hrm_st.weared);
        weared_last = weared_st;
    }
    else if (hrm_st.start_mode == HRM_START_FIXED){
        hrm_st.weared = WEAR_DET_SUCC;
        hrm_st.is_prepare = 0;            
        hrm_get_raw(&hrm_raw);
    }

    //LOG_DEBUG("[hrm_thread]the latest hrm is: %d %d %d %d\n", \
                    hrm_raw.led_green, hrm_raw.led_ir, hrm_raw.led_amb, hrm_raw.led_red); 
#if 0        
    hrm_raw_t *p_raw = &hrm_raw; 

    if (RY_SUCC != (status = ryos_queue_send(ry_queue_alg, &p_raw, RY_OS_NO_WAIT))){
        LOG_ERROR("[hrm_thread]: Add to Queue fail. status = 0x%04x\n", status);
    }  
#endif        

}

/**
 * @brief   HRM detect thread
 *
 * @param   None
 *
 * @return  None
 */
int hrm_detect_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    ry_hrm_msg_t hrm_msg;
    
    LOG_DEBUG("[hrm_detect_thread] Enter.\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_HRM);
#endif

    /* Create the RY hrm status switch message queue */
    status = ryos_queue_create(&hrm_msgQ, 20, sizeof(ry_hrm_msg_t));
    if (RY_SUCC != status) {
        LOG_ERROR("[hrm_thread]: RX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    /* Start HRM Hardware */
    // _start_func_hrm(HRM_START_AUTO);
    pcf_func_hrm();
    
    while (1) {
        status = ryos_queue_recv(hrm_msgQ, &hrm_msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[hrm_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {
            hrm_msgHandler(&hrm_msg);
        }

        //ryos_delay_ms(10);
    }
}


/**
 * @brief   API to init HRM module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t hrm_init(void)
{
    ryos_threadPara_t para;
    
    /* Start HRM Detect Thread */
    strcpy((char*)para.name, "hrm_det_thread");
    para.stackDepth = 320;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = HRM_THREAD_PRIORITY;
    ryos_thread_create(&hrm_detect_thread_handle, hrm_detect_thread, &para);  


#if 1
    ry_timer_parm timer_para;
    /* Create the timer */
    timer_para.timeout_CB = hrm_retry_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "hrm_timer";
    hrm_timer_id = ry_timer_create(&timer_para);
#endif
    return RY_SUCC;
}

ry_sts_t hrm_uninit(void)
{
    return ryos_thread_delete(hrm_detect_thread_handle);
}
/* [] END OF FILE */
