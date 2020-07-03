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
#include "app_interface.h"
#include "ry_hal_inc.h"
#include "am_util_debug.h"
#include "board_control.h"
#include <stdio.h>

#if (RY_GSENSOR_ENABLE || RY_HRM_ENABLE)
    #define RY_SENSOR_ALG_ENABLE        TRUE
#else
    #define RY_SENSOR_ALG_ENABLE        FALSE
#endif

#include "ryos.h"
#include "ryos_timer.h"
#include "gsensor.h"
#include "mcube_sample.h"
#include "mcube_custom_config.h" 
#include "ppsi26x.h"
#include "algorithm.h"
#include "sensor_alg.h"
#include "scheduler.h"

#include "json.h"
#include "ry_ble.h"
#include "ry_hal_rtc.h"
#include "scheduler.h"
#include "sleepAlgorithm.h"
#include "hrm.h"
#include "data_management_service.h"
#include "device_management_service.h"
#include "timer_management_service.h"
#include "window_management_service.h"

#include "activity_setting.h"

#include "motar.h"
#include "ble_task.h"
#include "board_control.h"
#include "ry_statistics.h"
#include "activity_running.h"

#if DBG_ALG_RAW_DATA_CACL
#include "gsensor_raw_data.h"
#endif

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

extern ry_queue_t*   ry_queue_alg;


/*
 * TYPEDEFS
 *******************************************************************************
 */


/**
 * 
 */
typedef enum {
    SLP_DETECT_HR_WAITING = 0,
    SLP_DETECT_HR_BEGIN = 1,
    SLP_DETECT_HR_RUNNING = 2,   
    SLP_DETECT_HR_CONTINUE = 3,   
    SLP_DETECT_HR_STOP,                   
}slp_hr_state_t;


typedef enum {
    ALG_RESAULT_NONE,
    ALG_RESAULT_IMU,
    ALG_RESAULT_HRM,
}alg_result_type_t;


typedef enum {
    ALG_WAKEUP_FROM_ALG,
    ALG_WAKEUP_FROM_DATA_SYNC,
    ALG_WAKEUP_FROM_CHECK,
    ALG_WAKEUP_FROM_HRM,    
    ALG_WAKEUP_SRC_MAX = 3,                   
}alg_wakeUp_src_t;

/*********************************************************************
 * CONSTANTS
 */ 
#ifndef RY_SENSOR_ALG_ENABLE
#define RY_SENSOR_ALG_ENABLE              1
#endif

#define SENSOR_ALG_THREAD_PRIORITY        3
#define SENSOR_ALG_THREAD_STACK_SIZE      640 //*4 Bytes
#define WRIST_THRESH                      10
#define LONG_SIT_TIME_THRESH              50   //50 min
#define ALG_SLEEP_TIME_TOTAL_CALC         1
#define ALG_THRESH_STILL_LONG             60
#define ALG_IDLE_CNT_ONE_SECONDE          16
#define ALG_IDLE_CNT_THRESH_RESET         (ALG_IDLE_CNT_ONE_SECONDE * 300)  //cnt

#define HRM_NEED_CAL_MIN                  170
#define HRM_NEED_CAL_MAX                  239

#define ALG_RECORD_LOG_TIME_MIN           (2 * 60)    //minute
#define ALG_RECORD_LOG_TIME_MAX           (60 * 60)   //minute
#define ALG_RECORD_LOG_STEP_MAX           1000

#define LOG_DEBUG_ALG


#if SLEEP_CALC_DEBUG
#define SLEEP_SAMPLE_THRESH              28
#else
#define SLEEP_SAMPLE_THRESH              630
#endif

#define SLEEP_IS_IN_DAY_MODE             (alg_ctx_v.sleep_day_mode == 1)

/*********************************************************************
 * VARIABLES
 */
cbuff_t cbuff_hr;
cbuff_t cbuff_acc;
cbuff_t cbuff_wrist;

algo_body_t body_inifo = {
    18,         //age
    1,          //sex
    60,         //weight
    170         //height
};

alg_ctx_t alg_ctx_v = {
    .wrist_thresh = 10,
    .alg_thresh   = 128,
    .sleep_log_time = 250,
    .sleep_log_period = 300,
    .unWeared_and_still = ALG_WEARED_OR_UNSTILL,
};

alg_sleep_time_t sleep_time ={
    0,
    22,
    55,
    7,
    05,
    0,
    0,
};

sensor_alg_t   s_alg;
sleep_calc_t   sleep_calc_v;

ryos_thread_t *sensor_alg_thread_handle;
ry_queue_t    *ry_queue_alg;

static rpc_ctx_t* hr_test_ctx  = NULL;
static u32_t sleep_log_timer_id;
static alg_info_t  alg_info;
gsensor_acc_t     acc_raw;

static uint32_t long_sit_start_time;
static uint8_t  long_sit_still_min_this_hour;
static uint8_t  long_sit_minute_acculate;
static uint32_t step_freq_all, step_freq_cnt, step_freq_ave;

#if ALG_SLEEP_CALC_DESAY                                        
SleepList sleep_list_v;
#endif

const uint8_t hrm_cal_tbl[] = {
    170, 171, 171, 172, 172, 
    173, 173, 174, 174, 175,
    175, 176, 176, 177, 177,
    178, 178, 179, 179, 180, 
    180, 180, 181, 181, 181,
    182, 182, 183, 183, 183,
    184, 184, 184, 185, 185,
    185, 186, 186, 186, 187,
    187, 187, 187, 188, 188,
    188, 188, 189, 189, 189,
    189, 189, 190, 190, 190,
    190, 190, 191, 191, 191,
    191, 191, 192, 192, 192,
    192, 192, 192, 192, 192,
    193, 193, 193, 193, 193,
};

const char* wake_src_str[] = {"alg", "sync", "check", "hrm"};


/*********************************************************************
 * LOCAL FUNCTIONS
 */

void sleep_log_timeout_handler(void);
void sleep_deep_sample_hrm_stop(void);
void alg_long_sit_reset(void);

uint32_t alg_hrm_calibration(uint8_t* hrm)
{
    uint32_t ret = 0;
    uint8_t hrm_cal = *hrm;
    if (hrm_cal > HRM_NEED_CAL_MAX){
        hrm_cal = HRM_NEED_CAL_MAX;
    }
    if (hrm_cal >= HRM_NEED_CAL_MIN){
        hrm_cal = hrm_cal_tbl[hrm_cal - HRM_NEED_CAL_MIN];
        ret  = 1;
    }
    else if (hrm_cal < 35){
        hrm_cal = 35 + (hrm_cal & 0x07);
    }
    *hrm = hrm_cal;
    return ret;
}

void alg_sleep_time_clear(void)
{
    static uint8_t f_data_cleared = 0;
    if (hal_time.ui32Hour == 18 && hal_time.ui32Minute <= 20) {
        if (f_data_cleared == 0){
            f_data_cleared = 1;
            sleep_time.f_wakeup_updated = 0;
            sleep_time.f_sleep_updated = 0;
            sleep_time.sleep_total = 0;
            sleep_time.sleep_total_dyn = 0;
            sleep_time.sleep_total_last = 0;
        }
    }
    else{
        f_data_cleared = 0;
    }
}


void alg_sleep_set_mode_on_time(void)
{
    if (hal_time.ui32Hour >= 9 && hal_time.ui32Hour < 21) {
        algo_setSlpMode(1);     //day mode
        alg_ctx_v.sleep_day_mode = 1;
    }
    else{
        algo_setSlpMode(0);     //night mode
        alg_ctx_v.sleep_day_mode = 0;
    }
}

/**
 * @brief   API to set touch enter test mode.
 *
 * @param   None
 *
 * @return  None
 */
void hr_set_test_mode(u32_t fEnable, rpc_ctx_t* ctx)
{
    if (fEnable) {
        alg_ctx_v.hr_test_mode = 1;
        hr_test_ctx = ctx;
    } else {
        alg_ctx_v.hr_test_mode = 0;
        hr_test_ctx  = NULL;
    }
}

/**
 * @brief   API to enable/disable raw data report.
 *
 * @param   fEnable - 1 Enable and 0 disable
 *
 * @return  None
 */
void alg_report_raw_data(u32_t fEnable)
{
    alg_ctx_v.data_report = fEnable;
    uint8_t conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
}

void report_raw_data_keep_connection(void)
{
    if (ryble_idle_timer_is_running()){
        ry_ble_idle_timer_stop();
        ry_ble_connect_normal_mode();
    }
    if (ry_ble_is_connect_idle_mode()) {
        ry_ble_connect_normal_mode();
    }
}

/**
 * @brief   API to send IPC message to senson_alg module
 *
 * @param   type Which event type to subscribe
 * @param   len  Length of the payload
 * @param   data Payload of the message
 *
 * @return  Status
 */
ry_sts_t ry_alg_msg_send(u32_t type, u32_t len, u8_t *data)
{
    ry_sts_t status = RY_SUCC;
    ry_alg_msg_t *p = (ry_alg_msg_t*)ry_malloc(sizeof(ry_alg_msg_t)+len);
    if (NULL == p) {
        LOG_ERROR("[ry_alg_msg_send]: No mem.\n");
        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_NO_MEM); 
    }   

    p->msgType = type;
    p->len     = len;
    if (data) {
        ry_memcpy(p->data, data, len);
    }

    // LOG_DEBUG("[ry_alg_msg_send] type: 0x%x\r\n", type);

    if (RY_SUCC != (status = ryos_queue_send(ry_queue_alg, &p, 4))) {
        LOG_ERROR("[ry_alg_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
        ry_free((void*)p);        
        return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_QUEUE_FULL);
    } 
    return RY_SUCC;
}


/**
 * @brief   API to alg_msg_transfer
 *
 * @param   info_type - msg type
 *
 * @return  None
 */
void alg_msg_transfer(uint32_t info_type)
{
    alg_info.info_type = info_type;
    switch (info_type) {
        case ALG_MSG_GENERAL:
            {
                alg_info.pdata = (void*)&s_alg;
                u32_t * new_status = (u32_t *)ry_malloc(sizeof(u32_t));
                *new_status = s_alg.sports_mode;
                if(get_running_flag() == 0){
                    ry_data_msg_send(DATA_SERVICE_BODY_STATUS_CHANGE, sizeof(u32_t), (u8_t *)new_status);
                }
                ry_free(new_status);            
                break;
            }

        case ALG_MSG_SLEEP:
            {
                static sleep_data_t sleep_data;

                //post-process rri data
                if (sleep_calc_v.rri[0] == -1){
                    sleep_calc_v.rri[1] = sleep_calc_v.rri[2] = 0;
                }
                if ((uint8_t)sleep_calc_v.rri[1] >= 0x1f){
                sleep_calc_v.rri[1] = 0;     
                }
                if ((uint8_t)sleep_calc_v.rri[2] >= 0x1f){
                sleep_calc_v.rri[2] = 0;
                }
                
                sleep_data.hrm = s_alg.hr_cnt;
                sleep_data.sstd = sleep_calc_v.sstd;
                sleep_data.rri = sleep_calc_v.rri[0] & 0x3f;
                sleep_data.rri |= (sleep_calc_v.rri[1] & 0x1f) << 6;
                sleep_data.rri |= (sleep_calc_v.rri[2] & 0x1f) << 11;
                alg_info.pdata = (void*)&sleep_data;
                //LOG_DEBUG("send_ry_queue_alg, type: %d, hrm: %d, sstd: %d, rri_0: %d, rri_1: %d, rri_2: %d \r\n",\
                        alg_info.info_type, sleep_data.hrm, sleep_data.sstd, sleep_calc_v.rri[0], sleep_calc_v.rri[1], sleep_calc_v.rri[2]);

                u32_t *msg_data = (u32_t *)ry_malloc(sizeof(u32_t));
                memcpy(msg_data, &sleep_data, sizeof(u32_t));
                ry_data_msg_send(DATA_SERVICE_MSG_SET_SLEEP_DATA, sizeof(sleep_data_t), (u8_t*)msg_data);
                ry_free(msg_data);
                break;
            }

        case ALG_MSG_SLEEP_SESSION_BEGINNING:
           ry_data_msg_send(DATA_SERVICE_MSG_SLEEP_SESSION_BEGINNING, 0, NULL);
           break;

        case ALG_MSG_SLEEP_SESSION_FINISHED:           
           ry_data_msg_send(DATA_SERVICE_MSG_SLEEP_SESSION_FINISHED, 0, NULL);
           break;

        case ALG_MSG_SLEEP_LOG_START:
           alg_info.pdata = (void*)&hal_time; 
           ry_data_msg_send(DATA_SERVICE_MSG_START_SLEEP, 0, NULL);
           break;

        case ALG_MSG_SLEEP_LOG_END:
           alg_info.pdata = (void*)&hal_time;
           ry_data_msg_send(DATA_SERVICE_MSG_STOP_SLEEP, 0, NULL);
           break;

        case ALG_MSG_HRM_MANUAL_STOP:
           alg_info.pdata = (void*)&s_alg;
           ry_data_msg_send(DATA_SERVICE_MSG_STOP_HEART, 0, NULL);
           break;

        case ALG_MSG_LONG_SIT:
            ry_sched_msg_send(IPC_MSG_TYPE_ALERT_LONG_SIT, 0, NULL);
            break;

        case ALG_MSG_10000_STEP:
            ry_sched_msg_send(IPC_MSG_TYPE_ALERT_10000_STEP, 0, NULL);
            break;

        default:
            break;
    }
    LOG_DEBUG("[alg_msg_transfer]info_type: %d, ryos_get_free_heap: %d.\r\n", info_type, ryos_get_free_heap());
}

void sleep_wakeup_to_detect_awake_hrm_end(void)
{
    alg_ctx_v.awake_hrm_en = 0;
    ry_data_msg_send(DATA_SERVICE_MSG_STOP_HEART_RESTING, 0, NULL);
    ry_hrm_msg_send(HRM_MSG_CMD_SLEEP_SAMPLE_STOP, NULL);
} 

void sleep_wakeup_to_detect_awake_hrm_begin(void)
{
    alg_ctx_v.awake_hrm_en = 1;
    ry_timer_start(sleep_log_timer_id, 10000, (ry_timer_cb_t)sleep_wakeup_to_detect_awake_hrm_end, NULL);
}

void alg_hrm_awake_ave_calc(void)
{
    uint8_t hrm_cnt = 255;
    if (alg_ctx_v.awake_hrm_en){
        if (alg_get_hrm_cnt(&hrm_cnt) != -1){ 
            s_alg.hr_awake = (s_alg.hr_awake > hrm_cnt) ? hrm_cnt : s_alg.hr_awake;
            if ((s_alg.hr_awake <= 30) || (s_alg.hr_awake >= 100)){
                s_alg.hr_awake = 69 + (hal_time.ui32Second & 0x0f);     //set a random value when hrm_cnt is wrong 
            }
        }
    }
    // LOG_DEBUG("[alg_hrm_awake_ave_calc]: awake_hrm_en: %d, hrm_cnt: %d, s_alg.hr_awake: %d\n", \
        alg_ctx_v.awake_hrm_en, hrm_cnt, s_alg.hr_awake);
}

void sleep_log_start_check(void)
{
    if (get_hrm_weared_status() == WEAR_DET_SUCC){ 
        if (alg_ctx_v.sleep_pro){
            ry_timer_start(sleep_log_timer_id, alg_ctx_v.sleep_log_time * 1000, (ry_timer_cb_t)sleep_log_timeout_handler, NULL);
            alg_msg_transfer(ALG_MSG_SLEEP_LOG_START);      
        }
        else{
            if (alg_ctx_v.sleep_hr_state != SLP_DETECT_HR_STOP){
                ry_timer_start(sleep_log_timer_id, alg_ctx_v.sleep_log_time * 1000, (ry_timer_cb_t)sleep_log_timeout_handler, NULL);
            }            
        }
        if (alg_ctx_v.sleep_hr_state == SLP_DETECT_HR_WAITING){
            alg_ctx_v.sleep_hr_state = SLP_DETECT_HR_BEGIN;
        }  
    }
    else{
        algo_sleep_enable(0);
        alg_ctx_v.sleep_enalbe = 0;  
    }
    LOG_DEBUG("[sleep_log_start_check] ALG_MSG_SLEEP_LOG_START or closed, sleep_state: %d\r\n", alg_ctx_v.sleep_state);
}

void sleep_log_suspend_timeout_handler(void)
{
    LOG_DEBUG("[sleep_log_suspend_timeout_handler]: transfer timeout.\r\n\r\n");
    if (s_alg.is_sleep & SLP_SLEEP_STAGE){         
        if (alg_ctx_v.sleep_pro){
            ry_hrm_msg_send(HRM_MSG_CMD_SLEEP_SAMPLE_START, NULL);
            ry_timer_start(sleep_log_timer_id, 1000, (ry_timer_cb_t)sleep_log_start_check, NULL);        
        }
        else{
            if (alg_ctx_v.sleep_hr_state != SLP_DETECT_HR_STOP){
                uint8_t alg_hrMean = 255;
                uint8_t alg_hrMedian = 255;
                uint8_t alg_hrRef = 255;
                uint8_t cacheLen = 255;
                algo_sleep_getParams(&alg_hrMean, &alg_hrMedian, &alg_hrRef, &cacheLen);
                LOG_DEBUG("[sleep_timer]: start hrm, slpSt:%d, hrSt:%d, hrSamp:%d, hrMean:%d, hrMedian:%d, hrRef:%d, cachLen:%d, hrmT:%d\r\n", \
                    alg_ctx_v.sleep_state, alg_ctx_v.sleep_hr_state, alg_ctx_v.sleep_hr_sample_time, \
                    alg_hrMean, alg_hrMedian, alg_hrRef, cacheLen, get_hrm_on_time());
                ry_hrm_msg_send(HRM_MSG_CMD_SLEEP_SAMPLE_START, NULL);
                ry_timer_start(sleep_log_timer_id, 1000, (ry_timer_cb_t)sleep_log_start_check, NULL);        
            }
        }
    }
    else{
        ry_timer_stop(sleep_log_timer_id);
    }
}

void sleep_log_timeout_handler(void)
{
    LOG_DEBUG("[sleep_log_timeout_handler]: transfer timeout.\n");
    ry_hrm_msg_send(HRM_MSG_CMD_SLEEP_SAMPLE_STOP, NULL);
    ry_timer_start(sleep_log_timer_id, 
                   (alg_ctx_v.sleep_log_period - alg_ctx_v.sleep_log_time) * 1000,
                   (ry_timer_cb_t)sleep_log_suspend_timeout_handler, 
                   NULL);
}

void sleep_deep_sample_hrm_start(void)
{
    if (alg_ctx_v.sleep_state == SLP_SLEEP_DEEP){
        if (!alg_ctx_v.sleep_pro){     
            if (s_alg.mode == ALGO_IMU){
                ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_START, NULL);
                ry_timer_start(sleep_log_timer_id, 20000, (ry_timer_cb_t)sleep_deep_sample_hrm_stop, NULL);
            }
        }   
    }  
}

void sleep_deep_sample_hrm_stop(void)
{
    ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_STOP, NULL);       
}

void sleep_calc_processing(void)
{
    static uint32_t sleep_calc_time;
    if (ry_hal_calc_ms(ry_hal_clock_time() - sleep_calc_time) >= 700){
        int32_t rri_cnt = algo_sleep_calc(sleep_calc_v.rri, &sleep_calc_v.sstd);

        LOG_DEBUG("[algo_sleep_calc]time: %d, rri_cnt: %d, rri: %d, %d, %d, sleep_calc_v.sstd: %d.\r\n", \
            ry_hal_calc_ms(ry_hal_clock_time() - sleep_calc_time), rri_cnt, \
            sleep_calc_v.rri[0], sleep_calc_v.rri[1], sleep_calc_v.rri[2], sleep_calc_v.sstd);

        //add sleep data storage [rri sstd hrm]code here   
        uint8_t hrm_cnt = 60;
        if (alg_get_hrm_cnt(&hrm_cnt) != -1){ 
            alg_msg_transfer(ALG_MSG_SLEEP);
        }
        sleep_calc_time = ry_hal_clock_time();
    }
}

void alg_sleep_scheduler_set_enable(uint8_t f_enable)
{
    alg_ctx_v.scheduler_enable = f_enable ? 0xff : 0;
}

void hrm_minimal_uptate(void)
{
    static uint8_t hr_min_today = 255;
    static uint8_t f_hm_uptated = 0;
    hr_min_today = (hr_min_today > s_alg.hr_cnt) ? s_alg.hr_cnt : hr_min_today;
    hr_min_today = (hr_min_today <= 30) ? 60: hr_min_today;
    if ((hal_time.ui32Hour == 8) && (hal_time.ui32Minute <= 20)) {
        if (f_hm_uptated == 0){
            s_alg.hr_min2 = s_alg.hr_min1;
            s_alg.hr_min1 = hr_min_today;
            algo_slp_updateHrMin(s_alg.hr_min1, s_alg.hr_min2);            
            f_hm_uptated = 1;
            hr_min_today = 255;
            LOG_DEBUG("[hrm_minimal_uptate] : s_alg.hr_min2: %d, s_alg.hr_min1: %d, hr_min_today: %d\n", \
                       s_alg.hr_min2, s_alg.hr_min1,  hr_min_today); 
        }
    }
    else{
        f_hm_uptated = 0;
    }
}

void sleep_enter_time_update(void)
{
    if (ALG_SLEEP_TIME_TOTAL_CALC || sleep_time.f_sleep_updated == 0){
        sleep_time.sleep_hour = hal_time.ui32Hour;
        sleep_time.sleep_minute = hal_time.ui32Minute;
        sleep_time.f_sleep_updated = 1;
    }
}

void wakeup_time_update(void)
{
    uint32_t f_update = (sleep_time.f_wakeup_updated == 0) || \
                        (sleep_time.f_wakeup_updated && (hal_time.ui32Hour < 10));
    if (ALG_SLEEP_TIME_TOTAL_CALC || f_update){
        sleep_time.wakeup_hour = hal_time.ui32Hour;
        sleep_time.wakeup_minute = hal_time.ui32Minute;
        sleep_time.f_wakeup_updated = 1;
    }
}

int32_t sleep_session_time_calc(void)
{
    int32_t sleep_total = 0;
    uint32_t wakeup_point = sleep_time.wakeup_hour * 60 + sleep_time.wakeup_minute;
    uint32_t sleep_point = sleep_time.sleep_hour * 60 + sleep_time.sleep_minute;
    if ((sleep_time.f_sleep_updated == 1) && (sleep_time.f_wakeup_updated == 1)){
        if (wakeup_point >= sleep_point){
            sleep_total = wakeup_point - sleep_point; 
        }
        else{
            sleep_total = (wakeup_point + 24 * 60) - sleep_point; 
        }
    }
    if (sleep_total < 0){
        sleep_total = 0;
    }
    return sleep_total;
}


int32_t sleep_time_dyn_calc(void)
{
    int32_t sleep_total = 0;
    uint32_t wakeup_point = hal_time.ui32Hour * 60 + hal_time.ui32Minute;
    uint32_t sleep_point = sleep_time.sleep_hour * 60 + sleep_time.sleep_minute;
    if (alg_status_is_in_sleep()){
        if (wakeup_point >= sleep_point){
            sleep_total = wakeup_point - sleep_point; 
        }
        else{
            sleep_total = (wakeup_point + 24 * 60) - sleep_point; 
        }
    }
    else{
        sleep_time.sleep_total_last = sleep_time.sleep_total_dyn;
    }

    if (sleep_total < 0){
        sleep_total = 0;
    }
    sleep_time.sleep_total_dyn = sleep_time.sleep_total_last + sleep_total;
    return sleep_time.sleep_total_dyn;
}

void sleep_hrm_stop_from_continue(void)
{
    if (alg_ctx_v.sleep_hr_state == SLP_DETECT_HR_CONTINUE){
        if (!alg_ctx_v.sleep_pro){
            if ((alg_ctx_v.sleep_hr_sample_time >= SLEEP_SAMPLE_THRESH) || \
                (alg_ctx_v.sleep_state == SLP_SLEEP) || (alg_ctx_v.sleep_state == SLP_SLEEP_DEEP)){
                ry_hrm_msg_send(HRM_MSG_CMD_SLEEP_SAMPLE_STOP, NULL);
                alg_ctx_v.sleep_hr_state = SLP_DETECT_HR_STOP;                        
            }
        }
    }
}

void save_wakeup_src(char wake_src)
{
    alg_ctx_v.wake_src = wake_src;
}

void sleep_wake_up_from(uint32_t wake_src)
{
    if (s_alg.is_sleep & SLP_SLEEP_STAGE){
        algo_sleep_enable(0);
        alg_ctx_v.sleep_enalbe = 0;  
        save_wakeup_src(wake_src);
        LOG_DEBUG("[sleep_wake_up_from] wake_src:%d\n", wake_src);
    }
}

void alg_sleep_processing(uint32_t mode, uint32_t still_state)
{
    //recovery sleep enable when not still from un-weared
    if ((still_state == 0) && (alg_ctx_v.sleep_enalbe == 0)){
        algo_sleep_enable(1);
        alg_ctx_v.sleep_enalbe = 1;
    } 
    hrm_minimal_uptate();
    alg_sleep_time_clear();
    alg_sleep_set_mode_on_time();

#if SLEEP_CALC_DEBUG
	static uint8_t debug_slp_cnt;   
    if (debug_slp_cnt >= 36){ 
        algo_sleep_dbgEnable();             //静止60秒进入，送28个心率值55，最后一个送54，，动30秒退出
    } 
    else{
        debug_slp_cnt ++;
    }           
#endif

    u16_t second_slp_delay = 0;
    volatile uint32_t f_sleep = algo_isSleep(&second_slp_delay) & alg_ctx_v.scheduler_enable;
    if (charge_cable_is_input()){
        f_sleep = SLP_WAKEUP;
    }
    switch (f_sleep & SLP_SLEEP_STAGE){
        case SLP_WAKEUP:
            if (alg_status_is_in_sleep() != 0){
                if ((sleep_time.f_wakeup_updated == 0) && (sleep_time.f_sleep_updated != 0)){
                    s_alg.hr_awake = 0xff;
                }
                ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_START, NULL);
                ry_timer_start(sleep_log_timer_id, 10000, (ry_timer_cb_t)sleep_wakeup_to_detect_awake_hrm_begin, NULL);
                
                //wakeup from sleep send ALG_MSG_SLEEP_SESSION_FINISHED
                if (alg_ctx_v.sleep_pro){
                    if (alg_ctx_v.sleep_enalbe) {
                        alg_msg_transfer(ALG_MSG_SLEEP_SESSION_FINISHED);
                    }
                }
                periodic_task_set_enable(periodic_sleep_hr_weared_detect_task, 0);
                wakeup_time_update();
                sleep_time.sleep_total += sleep_session_time_calc();
                ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_WAKEUP, 0, NULL);

                LOG_INFO("[wakeup]slpSt:%d, hrSt:%d, slpT:%d, slpHour:%d, slpMin:%d, wakeSrc:%s, dl:%d\r\n", \
                    alg_ctx_v.sleep_state, alg_ctx_v.sleep_hr_state, sleep_time.sleep_total, \
                    sleep_time.sleep_hour, sleep_time.sleep_minute, wake_src_str[alg_ctx_v.wake_src & ALG_WAKEUP_SRC_MAX], second_slp_delay); 
            }
            
            if (mode == ALGO_HRS){
                alg_hrm_awake_ave_calc();
            }
            alg_ctx_v.sleep_hr_sample_time = 0;
            alg_ctx_v.sleep_hr_state = SLP_DETECT_HR_WAITING; 
            alg_ctx_v.sleep_state = SLP_WAKEUP;
            break;

        case SLP_SUSPECT:
            if (alg_ctx_v.sleep_state == SLP_WAKEUP){
                if (mode == ALGO_IMU){
                    ry_hrm_msg_send(HRM_MSG_CMD_SLEEP_SAMPLE_START, NULL);
                }
                //wait for weared detect and change sleep_state
                ry_timer_start(sleep_log_timer_id, 1000, (ry_timer_cb_t)sleep_log_start_check, NULL);
            }
            if ((alg_ctx_v.sleep_hr_state == SLP_DETECT_HR_RUNNING) || \
               (alg_ctx_v.sleep_hr_state == SLP_DETECT_HR_CONTINUE)){
                if (mode == ALGO_HRS){
#if SLEEP_CALC_DEBUG 
                    if (alg_ctx_v.sleep_hr_sample_time < SLEEP_SAMPLE_THRESH - 1){
                        s_alg.hr_cnt = 55;
                        algo_slp_addToHrBuf(s_alg.hr_cnt);                        
                    }        
                    else if (alg_ctx_v.sleep_hr_sample_time == SLEEP_SAMPLE_THRESH - 1){
                        s_alg.hr_cnt = 54;                        
                        algo_slp_addToHrBuf(s_alg.hr_cnt);
                    }  
#else
                    algo_slp_addToHrBuf(s_alg.hr_cnt);
#endif                        
                    if (alg_ctx_v.sleep_pro){
                        sleep_calc_processing(); 
                    }
                    if (alg_ctx_v.sleep_hr_sample_time <= SLEEP_SAMPLE_THRESH){
                        alg_ctx_v.sleep_hr_sample_time ++;
                    }                        
                }
            }

            sleep_hrm_stop_from_continue();

            if (alg_ctx_v.sleep_hr_state == SLP_DETECT_HR_BEGIN){
                if (alg_ctx_v.sleep_pro){
                    alg_msg_transfer(ALG_MSG_SLEEP_SESSION_BEGINNING);
                    alg_ctx_v.sleep_hr_state = SLP_DETECT_HR_RUNNING;
                }
                else{
                    alg_ctx_v.sleep_hr_state = SLP_DETECT_HR_CONTINUE;
                }
                if (!SLEEP_IS_IN_DAY_MODE){
                    periodic_task_set_enable(periodic_sleep_hr_weared_detect_task, 1);
                    ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_SLEEP, 0, NULL);
                    sleep_enter_time_update();
                    save_wakeup_src(ALG_WAKEUP_FROM_ALG);   //default set wakeup from 
                    LOG_ERROR("[suspect]aMode:%d, sMode:%d, slpSt:%d, hrSt:%d, fSleep:%d, slpCnt:%d, hr_min1:%d, hr_min2: %d\r\n", \
                        mode, s_alg.sports_mode, alg_ctx_v.sleep_state, alg_ctx_v.sleep_hr_state, f_sleep, slpCntSum, s_alg.hr_min1, s_alg.hr_min2); 
                }
            }

            if (alg_ctx_v.sleep_hr_state != SLP_DETECT_HR_WAITING){
                alg_ctx_v.sleep_state = SLP_SUSPECT;                
            }
            else{
                alg_ctx_v.sleep_state = SLP_WAKEUP_TO_SUSPECT;
            }
            break;

        case SLP_SLEEP:
            sleep_hrm_stop_from_continue();
            if (mode == ALGO_HRS){
                if (alg_ctx_v.sleep_pro){
                    sleep_calc_processing(); 
                }
            }
            if (alg_ctx_v.sleep_state != SLP_SLEEP){    
                uint8_t alg_hrMean = 255;
                uint8_t alg_hrMedian = 255;
                uint8_t alg_hrRef = 255;
                uint8_t cacheLen = 255;
                algo_sleep_getParams(&alg_hrMean, &alg_hrMedian, &alg_hrRef, &cacheLen);
                LOG_INFO("[sleep_enter] sMode:%d, slpSt:%d, dl:%d, hrSamp:%d, hrMean:%d, hrMedian:%d, hrRef:%d, cachLen:%d\r\n", \
                        s_alg.sports_mode, alg_ctx_v.sleep_state, second_slp_delay, alg_ctx_v.sleep_hr_sample_time, \
                        alg_hrMean, alg_hrMedian, alg_hrRef, cacheLen);       
                if (SLEEP_IS_IN_DAY_MODE){
                    periodic_task_set_enable(periodic_sleep_hr_weared_detect_task, 1);
                    ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_SLEEP, 0, NULL);
                    sleep_enter_time_update();
                    save_wakeup_src(ALG_WAKEUP_FROM_ALG);   //default set wakeup from 
                    // LOG_ERROR("[suspect]aMode:%d, sMode:%d, slpSt:%d, hrSt:%d, fSleep:%d, slpCnt:%d, hr_min1:%d, hr_min2: %d\r\n", \
                        mode, s_alg.sports_mode, alg_ctx_v.sleep_state, alg_ctx_v.sleep_hr_state, f_sleep, slpCntSum, s_alg.hr_min1, s_alg.hr_min2); 
                }
            }
            alg_ctx_v.sleep_state = SLP_SLEEP;
            break;

        case SLP_SLEEP_DEEP:
            sleep_hrm_stop_from_continue();
            if (alg_ctx_v.sleep_state != SLP_SLEEP_DEEP){
                LOG_DEBUG("[deepsleep] aMode:%d, sMode:%d, slpSt:%d, hrSt:%d, hrSamp:%d, slpCnt:%d, fSleep:%d\r\n", \
                        mode, s_alg.sports_mode, alg_ctx_v.sleep_state, alg_ctx_v.sleep_hr_state, alg_ctx_v.sleep_hr_sample_time, slpCntSum, f_sleep); 
                if (!alg_ctx_v.sleep_pro){     
                    if (mode == ALGO_IMU){
                        if (sleep_time.f_wakeup_updated == 0){
                            ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_START, NULL);
                            ry_timer_start(sleep_log_timer_id, 30000, (ry_timer_cb_t)sleep_deep_sample_hrm_stop, NULL);
                        }
                    }
                }
            }
            if (mode == ALGO_HRS){
                if (alg_ctx_v.sleep_pro){
                    sleep_calc_processing(); 
                }
            }
            alg_ctx_v.sleep_state = SLP_SLEEP_DEEP;
            break;

        default:
            break;
    }

    s_alg.is_sleep = alg_ctx_v.sleep_state;
    //LOG_DEBUG("[sleep]alg_mode:%d, sports_mode:%d, sleep_state:%d, sleep_hr_state:%d, hr_sample:%d, slpCntSum:%d, f_sleep:%d, hrm_cnt:%d.\r\n", \
              mode, s_alg.sports_mode, alg_ctx_v.sleep_state, alg_ctx_v.sleep_hr_state, alg_ctx_v.sleep_hr_sample_time, slpCntSum, f_sleep, s_alg.hr_cnt);       
}


/**
 * @brief   API to get alg_sport_mode_update_from_sleep
 *
 * @param   None
 *
 * @return  status - 0: no sleep, 1: in sleep, 2: wakeup from sleep
 */
uint32_t alg_sport_mode_update_from_sleep(void)
{
    uint32_t updated = 0;
    if (alg_ctx_v.sleep_pro){
        if ((s_alg.is_sleep == SLP_SUSPECT) || (s_alg.is_sleep == SLP_SLEEP)){
            s_alg.sports_mode = SPORTS_SLEEP_LIGHT;
            updated = 1;
        }
        else if (s_alg.is_sleep == SLP_SLEEP_DEEP){
            s_alg.sports_mode = SPORTS_SLEEP_DEEP;
            updated = 1;
        }
        else if (s_alg.is_sleep == SLP_WAKEUP_TO_SUSPECT){
            updated = 1;
        }
        else if (s_alg.sleep_last != s_alg.is_sleep){
            updated = 2;
        }
    }
    else{
        if (((s_alg.is_sleep == SLP_SUSPECT) && (!SLEEP_IS_IN_DAY_MODE)) || (s_alg.is_sleep == SLP_SLEEP)){
            s_alg.sports_mode = SPORTS_SLEEP_LIGHT;
            updated = 1;
        }
        else if (s_alg.is_sleep == SLP_SLEEP_DEEP){
            s_alg.sports_mode = SPORTS_SLEEP_DEEP;
            updated = 1;
        }
        else if (s_alg.sleep_last != s_alg.is_sleep){
            updated = 2;
        }
    }
    s_alg.sleep_last = s_alg.is_sleep;
    return updated;  
}

void alg_sport_mode_processing(uint32_t alg_mode)
{
    static uint32_t mode_detet_time; 
    static uint32_t sport_step_last; 
    
    uint32_t sleep_updated = 0;;                   
#if DEBUG_ALG_STATE_DETECT
    static int status_changed_cnt;
    static int count_this_status;       
    uint8_t mode = s_alg.sports_mode;
    if (ry_hal_calc_ms(ry_hal_clock_time() - mode_detet_time) >= 60 * 1000){
        static int count_start;
        count_this_status = alg_get_step_today() - count_start;
        count_start = alg_get_step_today();
        mode = (mode + 1) % SPORTS_SLEEP_LIGHT;
        status_changed_cnt ++;
        mode_detet_time = ry_hal_clock_time();

        // LOG_DEBUG("[alg_sport_mode]: status_changed_cnt: %d, count_this_status: %d, alg_get_step_today: %d\r\n", \
                   status_changed_cnt, count_this_status, alg_get_step_today());
    }
	s_alg.step_cnt_update += s_alg.step_cnt + (SPORTS_SLEEP_LIGHT - mode);            
    s_alg.sports_mode = mode;
    if (s_alg.sports_mode != s_alg.sports_mode_last){
        if (SPORTS_NOT_DETECT != s_alg.sports_mode_last){
            alg_msg_transfer(ALG_MSG_GENERAL);
        }
        LOG_DEBUG("[spt_md]: is_still:%d, mode:%d, mode_last:%d, mode_cnt:%d, step:%d\r\n", \
            s_alg.is_sleep, s_alg.sports_mode, s_alg.sports_mode_last, status_changed_cnt, s_alg.step_cnt_today);
        s_alg.sports_mode_last = s_alg.sports_mode;        
    }
#else
    uint32_t sleep_mode_update = alg_sport_mode_update_from_sleep();
    if (sleep_mode_update == 1){
        sleep_updated = 1;                    //update all sleep status change
    }
    else if (sleep_mode_update == 2){           
        sleep_updated = 1;                    //update all sleep status change
        s_alg.sports_mode = algo_state_cal();           
    }
    else{
        s_alg.sports_mode = algo_state_cal();           
    }
    
    // LOG_DEBUG("[alg_sport_mode_processing]: is_sleep: %d, sleep_updated:%d, mode: %d, mode_last: %d\r\n", \
        s_alg.is_sleep, sleep_updated, s_alg.sports_mode, s_alg.sports_mode_last);

    uint32_t alg_record_wr = 0;
    uint32_t delta_detect_time = ry_hal_calc_second(ry_hal_clock_time() - mode_detet_time);
    uint32_t sleep_conditon = sleep_updated && (s_alg.sports_mode != s_alg.sports_mode_last);
    uint32_t mode_conditon = ((s_alg.sports_mode != s_alg.sports_mode_last) && (delta_detect_time >= ALG_RECORD_LOG_TIME_MIN));
    uint32_t time_conditon = (delta_detect_time >= ALG_RECORD_LOG_TIME_MAX);
    uint32_t step_conditon = (s_alg.step_cnt_today > (sport_step_last + ALG_RECORD_LOG_STEP_MAX));

    //filter the mode changed to still from light_active whitin 10 min
    if ((mode_conditon != 0) && (s_alg.sports_mode_last == SPORTS_LIGHT_ACTIVE) && \
        (s_alg.sports_mode == SPORTS_STILL) && (delta_detect_time <= ALG_RECORD_LOG_TIME_MIN * 5)){
        mode_conditon = 0; 
        mode_detet_time = ry_hal_clock_time();          //clear detect time from beginning
    }

    alg_record_wr = (sleep_conditon || mode_conditon || time_conditon || step_conditon);
    if (alg_record_wr){
        if (SPORTS_NOT_DETECT != s_alg.sports_mode_last){
            alg_msg_transfer(ALG_MSG_GENERAL);
            LOG_DEBUG("s_alg.sports_times:%d, sleep_conditon:%d, mode_conditon:%d, time_conditon:%d, step_conditon:%d\r\n", \
                        s_alg.sports_times, sleep_conditon, mode_conditon, time_conditon, step_conditon);
            LOG_DEBUG("[algMode]algMode:%d, isSlp:%d, mode:%d, mode_last:%d, step:%d, delta_detect_time:%d\r\n", \
                alg_mode, s_alg.is_sleep, s_alg.sports_mode, s_alg.sports_mode_last, s_alg.step_cnt_today, delta_detect_time);
        }
        s_alg.sports_mode_last = s_alg.sports_mode;
        s_alg.sports_times ++;
        sport_step_last = s_alg.step_cnt_today;
        mode_detet_time = ry_hal_clock_time();
    }
#endif     
}

void alg_long_sit_set_enable(uint8_t f_enable)
{
    alg_ctx_v.long_sit_enable = f_enable;
}

void alg_long_sit_processing(void)
{
    uint32_t long_sit_active = 0;
	uint32_t time_start = get_sit_alert_start_time();
    uint32_t time_end = get_sit_alert_end_time();
	
    ry_time_t cur_time = {0};

    if (alg_ctx_v.long_sit_enable){
        tms_get_time(&cur_time);
        if (time_start > time_end){
            if ( ((cur_time.hour * 60 + cur_time.minute) >= time_start) || \
                ((cur_time.hour * 60 + cur_time.minute) <= time_end)){
                long_sit_active = 1;    
            }
        }
        else if(time_start < time_end)
		{
            if ( ((cur_time.hour * 60 + cur_time.minute) >= time_start) && \
                ((cur_time.hour * 60 + cur_time.minute) <= time_end)){
                long_sit_active = 1;    
            }
        }
		else
		{
			long_sit_active = 0; 
		}
		
		if(time_start == 0xffffffff)
		{
			if ((cur_time.hour > 22) || (cur_time.hour < 8)){
                long_sit_active = 0;
            }
			else{
				long_sit_active = 1;
			}
		}

        uint32_t long_sit = alg_long_sit_detect() && !s_alg.is_sleep;
        if (long_sit){
            if (long_sit_active && (setting_get_dnd_status() == 0) && (get_device_session() != DEV_SESSION_CARD)){
                s_alg.long_sit_times ++;
				
				if(current_time_is_dnd_status() != 1){
					motar_light_linear_working(200);
					alg_msg_transfer(ALG_MSG_LONG_SIT);
                    DEV_STATISTICS(long_sit_count);
				}
                //motar_light_linear_working(200);
                //alg_msg_transfer(ALG_MSG_LONG_SIT);
            }
        }
    }
    //LOG_DEBUG("[alg_long_sit_processing]: alg_ctx_v.long_sit_enalbe: %d, long_sit_active: %d, time_start: %d, time_end: %d, cur_time.hour: %d, cur_time.minute: %d, long_sit_times: %d\r\n", \
        alg_ctx_v.long_sit_enalbe,long_sit_active, time_start, time_end, cur_time.hour, cur_time.minute, s_alg.long_sit_times);
}


void rbp_alg_raw_data_report(int32_t* data, u32_t point)
{
    int nlen, total_len = 0;
    int max_str_len = 2048;
    u16_t crc_val;
    char* result = (char*)ry_malloc(max_str_len);
    if (result == NULL) {
        LOG_ERROR("[rbp_alg_raw_data_report] No mem.\r\n");
        return;
    }

    nlen = 0;
    for (int i = 0; i < point; i++) {
        nlen += snprintf(result+nlen, max_str_len, "%d,%d,%d,%d,%d,%d,%.2f\r\n", 
            data[i*6+0], data[i*6+1], data[i*6+2],
            data[i*6+3], data[i*6+4], data[i*6+5], get_sport_distance());
    }
    total_len = strlen(result);
    
    //LOG_DEBUG("raw data: point:%d\r\n %s", total_len, result);
    rbp_send_req(CMD_APP_LOG_RAW_ONLINE_DATA, (u8_t *)result, total_len, 1);
    ry_free(result);
}


/**
 * @brief   heat rate & imu processing task
 *
 * @param   None
 *
 * @return  0: no result_updated; 2: result_updated
 */
uint32_t alg_hrm_processing(void)
{
    uint32_t ret = ALG_RESAULT_NONE;
    uint8_t flg_adding_data = 128;
    static uint8_t acc_ptr = 0;            
    static uint8_t hrm_ptr = 0;
        
    do{
        if (cbuff_acc.cnt < alg_ctx_v.alg_thresh) {
            if (cbuff_acc.rd_ptr != cbuff_acc.head){
                cbuff_acc.cnt ++;
                cbuff_acc.rd_ptr ++;
                if (cbuff_acc.rd_ptr >= (FIFO_BUFF_DEEP << 1)){
                    cbuff_acc.rd_ptr = 0;
                }
                acc_ptr = (cbuff_acc.rd_ptr >= FIFO_BUFF_DEEP) ? (cbuff_acc.rd_ptr - FIFO_BUFF_DEEP) : cbuff_acc.rd_ptr; 
                algorithm_accAddToBuf(  &p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_X], \
                                        &p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Y], \
                                        &p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Z], \
                                        NULL,              \
                                        NULL,              \
                                        NULL,              \
                                        1);
            }
            else{
                flg_adding_data = 0;
            }
        }

        if (cbuff_hr.cnt < alg_ctx_v.alg_thresh){
            if (cbuff_hr.rd_ptr != cbuff_hr.head){
                cbuff_hr.cnt ++;
                cbuff_hr.rd_ptr ++;
                if (cbuff_hr.rd_ptr >= (AFE_BUFF_LEN << 1)){
                    cbuff_hr.rd_ptr = 0;
                }
                hrm_ptr = (cbuff_hr.rd_ptr >= AFE_BUFF_LEN) ? (cbuff_hr.rd_ptr - AFE_BUFF_LEN) : cbuff_hr.rd_ptr;
                algorithm_lightAddToBuf(&hr_led_ir[hrm_ptr],     \
                                        NULL,                    \
                                        &hr_led_green[hrm_ptr],  \
                                        &hr_led_amb[hrm_ptr],    \
                                        1);
            }
            else{
                flg_adding_data = 0;
            }
        }
       
        LOG_RAW("%d,%d,%d,%d,%d,%d\n",    \
                (int16_t)p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_X], \
                (int16_t)p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Y], \
                (int16_t)p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Z], \
                (int32_t)hr_led_green[hrm_ptr],\
                (int32_t)(hr_led_amb[hrm_ptr] * 1000 + s_alg.hr_cnt), \
                (int32_t)hr_led_ir[hrm_ptr] \
                );
        // LOG_DEBUG_PPS("add buffer cbuff_acc.rd_ptr: %d, %7d, %7d, %7d, %5d, %5d, %5d, %2d, %2d, %2d, %d\n",    \
                        cbuff_acc.rd_ptr & (FIFO_BUFF_DEEP - 1), hr_led_ir[hrm_ptr], hr_led_green[hrm_ptr], hr_led_amb[hrm_ptr],\
                        p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_X], \
                        p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Y], \
                        p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Z], \
                        s_alg.step_freq, s_alg.step_cnt, s_alg.hr_cnt, s_alg.weared      \
                        );

        // when acc is less than hrm
        if ((cbuff_hr.cnt >= alg_ctx_v.alg_thresh) && (cbuff_acc.cnt < alg_ctx_v.alg_thresh)){
            do{
                cbuff_acc.cnt ++;
                algorithm_accAddToBuf(  &p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_X], \
                                        &p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Y], \
                                        &p_acc_data->RawData[acc_ptr][M_DRV_MC_UTL_AXIS_Z], \
                                        NULL,              \
                                        NULL,              \
                                        NULL,              \
                                        1);
                // LOG_DEBUG_PPS("add acc - cbuff_acc.cnt: %d\n", cbuff_acc.cnt);
            }while((cbuff_acc.cnt < alg_ctx_v.alg_thresh));
        }

        if ((cbuff_hr.cnt < alg_ctx_v.alg_thresh) && (cbuff_acc.cnt >= alg_ctx_v.alg_thresh)){
            uint8_t over_sample = cbuff_acc.cnt - cbuff_hr.cnt;
            while(over_sample){
                over_sample --;
                cbuff_acc.rd_ptr ++;  //skip one acc data when acc is more than hrm
                // LOG_DEBUG_PPS("skip acc - over_sample: %d, cbuff_acc.cnt: %d\n", over_sample, cbuff_acc.cnt);    
            }
        }

        //    alg_ctx_v.data_report = 1;
        if (alg_ctx_v.hr_test_mode) {
            static int hr_test_cnt = 0;
            if (hr_test_cnt++ == 30) {
                hr_test_cnt = 0;
                rpc_hr_data_report(hr_test_ctx, s_alg.hr_cnt, 1);
                //LOG_DEBUG_PPS("[hrm_task]alg_rst: step_freq: %d, step_cnt: %d hr_cnt: %d\n", \
                     s_alg.step_freq, s_alg.step_cnt, s_alg.hr_cnt);
            }
        }
        
        if (algorithm_isBufFull()) { 
            if ((get_online_raw_data_onOff() != 0) && (ry_ble_get_state() == RY_BLE_STATE_CONNECTED) && (alg_ctx_v.alg_thresh == 25)) {
                report_raw_data_keep_connection();
                int32_t *data = ry_malloc(25*4*6);

                for (int i = 24; i >= 0; i--) {
                    int8_t acc_ptr_history = acc_ptr - i;
                    acc_ptr_history = (acc_ptr_history < 0) ? (acc_ptr_history + FIFO_BUFF_DEEP) : acc_ptr_history;
                    int8_t hrm_ptr_history = hrm_ptr - i;
                    hrm_ptr_history = (hrm_ptr_history < 0) ? (hrm_ptr_history + AFE_BUFF_LEN) : hrm_ptr_history;
                    
                    // LOG_DEBUG_PPS("[history]:cbuff_acc.head:%d, acc_ptr:%d, acc_ptr_history:%d, %6d, %6d, %6d, %5d, %5d, %5d\r\n", \
                        cbuff_acc.head, acc_ptr, acc_ptr_history,\
                        hr_led_ir[hrm_ptr_history], \
                        hr_led_green[hrm_ptr_history], \
                        hr_led_amb[hrm_ptr_history], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_X], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Y], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Z]); 

                    data[(24-i)*6+0] = (int32_t)p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_X];         // x
                    data[(24-i)*6+1] = (int32_t)p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Y];         // y
                    data[(24-i)*6+2] = (int32_t)p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Z];         // z
                    data[(24-i)*6+3] = (int32_t)hr_led_green[hrm_ptr_history]; // Green
                    data[(24-i)*6+4] = (int32_t)(hr_led_amb[hrm_ptr_history] * 1000 + s_alg.hr_cnt);   // Environment
                    data[(24-i)*6+5] = (int32_t)hr_led_ir[hrm_ptr_history];    // IR
                }

                rbp_alg_raw_data_report(data, 25);
                ry_free((void*)data);
            }
            
#if 0
            char *result;
            size_t len;
            result = (char*)ry_malloc(200);
            snprintf(result, 200, "%d\r\n", hr_led_green[hrm_ptr]);
            len = strlen(result);
            ry_hal_uart_tx(UART_IDX_TOOL, result, len);
            ry_free(result);            
#endif  
#if 0
            for (int i = 24; i >= 0; i--) {
                int8_t acc_ptr_history = acc_ptr - i;
                acc_ptr_history = (acc_ptr_history < 0) ? (acc_ptr_history + FIFO_BUFF_DEEP) : acc_ptr_history;
                int8_t hrm_ptr_history = hrm_ptr - i;
                hrm_ptr_history = (hrm_ptr_history < 0) ? (hrm_ptr_history + AFE_BUFF_LEN) : hrm_ptr_history;
            
            LOG_DEBUG_PPS("cbuff_acc.rd_ptr: %d, acc_ptr_history:%d, %6d, %6d, %6d, %5d, %5d, %5d,\r\n", \
                cbuff_acc.rd_ptr & (FIFO_BUFF_DEEP - 1), acc_ptr_history & (FIFO_BUFF_DEEP - 1), hr_led_ir[hrm_ptr_history], \
                hr_led_green[hrm_ptr_history], \
                hr_led_amb[hrm_ptr_history], \
                p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_X], \
                p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Y], \
                p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Z]); 
            }
#endif
            algorithm_preProcess();
            algorithm_hr_cal(&s_alg.hr_cnt);
            alg_hrm_calibration(&s_alg.hr_cnt);
            algorithm_sc_cal(&s_alg.step_freq, &s_alg.step_cnt);        
            s_alg.still_state = algo_isStill();   
            alg_sleep_processing(ALGO_HRS, s_alg.still_state);
            alg_sport_mode_processing(ALGO_HRS);
            s_alg.weared = algo_isWeared(ALGO_WEARED_HRM_ON, NULL);

            static uint32_t sample_start_alg;
            //LOG_DEBUG("[hrm_task]alg_rst: ms:%d, step_freq: %d, step_cnt: %d hr_cnt: %d, alg_is_weared: %d, step_today: %d, is_sleep: %d\r\n", \
                ry_hal_calc_ms(ry_hal_clock_time() -sample_start_alg), s_alg.step_freq, s_alg.step_cnt, s_alg.hr_cnt, s_alg.weared, s_alg.step_cnt_today, s_alg.is_sleep);
            sample_start_alg = ry_hal_clock_time();	    
            algorithm_clearBufFullFlag();            
            cbuff_hr.cnt = 0;
            cbuff_acc.cnt = 0;
            flg_adding_data = 0;
            alg_ctx_v.alg_thresh = 25;                 
            s_alg.step_cnt_update += s_alg.step_cnt;
            alg_ctx_v.idle_cnt = 0;
            ret = ALG_RESAULT_HRM;
        }

        if ((cbuff_hr.cnt >= alg_ctx_v.alg_thresh) && (cbuff_acc.cnt >= alg_ctx_v.alg_thresh)){
            alg_ctx_v.alg_thresh = 25;       
        }

        if (cbuff_hr.rd_ptr == cbuff_hr.head){
            break;  //bread when hrm buff is null
        }

        if (cbuff_acc.rd_ptr == cbuff_acc.head){
            break;  //bread when acc buff is null
        }
        flg_adding_data --;
    }while(flg_adding_data);
    return ret;
}


/**
 * @brief   simple imu processing task
 *
 * @param   None
 *
 * @return  0: no result_updated; else: result_updated
 */
uint32_t alg_imu_processing(void)
{
    uint32_t ret = ALG_RESAULT_NONE;
    static uint8_t cbuff_ptr = 0;
    do{ 
        uint8_t reading = 128;
                
#if DBG_ALG_RAW_DATA_CACL
        static int raw_data_idx = 0;
        while(reading && (cbuff_acc.cnt < alg_ctx_v.alg_thresh)) {
            if (raw_data_idx < sizeof(gsensor_raw_data)/6){
                algorithm_accAddToBuf(  (short*)(&gsensor_raw_data[raw_data_idx][0]), \
                                        (short*)(&gsensor_raw_data[raw_data_idx][1]), \
                                        (short*)(&gsensor_raw_data[raw_data_idx][2]), \
                                        NULL,              \
                                        NULL,              \
                                        NULL,              \
                                        1);
                // ry_board_debug_printf("%5d, %5d, %5d, %2d, %5d, %4d\n",    \
                        gsensor_raw_data[raw_data_idx][0], \
                        gsensor_raw_data[raw_data_idx][1], \
                        gsensor_raw_data[raw_data_idx][2], \
                        s_alg.step_freq, s_alg.step_cnt + s_alg.sports_mode * 10000, \
                        s_alg.step_cnt_today      \
                        );
                raw_data_idx ++;
                cbuff_acc.cnt ++;
            }
        }
#else
        while(reading && (cbuff_acc.cnt < alg_ctx_v.alg_thresh)) {
            if (cbuff_acc.rd_ptr != cbuff_acc.head){
                reading --;
                cbuff_acc.cnt ++;
                cbuff_acc.rd_ptr ++;
                if (cbuff_acc.rd_ptr >= (FIFO_BUFF_DEEP << 1)){
                    cbuff_acc.rd_ptr = 0;
                }
                cbuff_ptr = (cbuff_acc.rd_ptr >= FIFO_BUFF_DEEP) ? (cbuff_acc.rd_ptr - FIFO_BUFF_DEEP) : cbuff_acc.rd_ptr;            
                algorithm_accAddToBuf(  &p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_X], \
                                        &p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Y], \
                                        &p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Z], \
                                        NULL,              \
                                        NULL,              \
                                        NULL,              \
                                        1);
#if ALG_SLEEP_CALC_DESAY                                        
                SLEEP_SampWrite(p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_X] >> 2,\
                          p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Y] >> 2,\
                          p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Z] >> 2);
#endif                          
                LOG_RAW("%d,%d,%d,%d,%d,%d\n",    \
                        (int16_t)p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_X], \
                        (int16_t)p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Y], \
                        (int16_t)p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Z], \
                        s_alg.step_freq, \
                        (s_alg.step_cnt + s_alg.sports_mode * 10000 + alg_ctx_v.wrist_last * 1000), \
                        s_alg.step_cnt_today\
                        );
            }
            else{
                reading = 0;
            }
        }
#endif
        // LOG_DEBUG_PPS("[acc] dataToAlg: cbuff_acc.head: %d, cbuff_acc.tail: %d, cbuff_acc.cnt: %d, cbuff_acc.rd_ptr: %d\n", \
            cbuff_acc.head, cbuff_acc.tail, cbuff_acc.cnt, cbuff_acc.rd_ptr);

        if (cbuff_acc.cnt >= alg_ctx_v.alg_thresh){
            alg_ctx_v.alg_thresh = 25;   
        }

        //    alg_ctx_v.data_report = 1;
        if (algorithm_isBufFull()){
#if ALG_SLEEP_CALC_DESAY                                                    
            get_SleepList(&sleep_list_v, 0);
            LOG_DEBUG("[get_SleepList]list: LNo: %d, sleep.Dur: %d\r\n", sleep_list_v.LNo, sleep_list_v.SDur);
#endif            
            //    alg_ctx_v.data_report = 1;
            if ((get_online_raw_data_onOff() != 0) && (ry_ble_get_state() == RY_BLE_STATE_CONNECTED) && (alg_ctx_v.alg_thresh == 25)) {
                uint8_t acc_ptr = (cbuff_acc.rd_ptr >= FIFO_BUFF_DEEP) ? (cbuff_acc.rd_ptr - FIFO_BUFF_DEEP) : cbuff_acc.rd_ptr;            
                uint8_t hrm_ptr = (cbuff_hr.rd_ptr >= AFE_BUFF_LEN) ? (cbuff_hr.rd_ptr - AFE_BUFF_LEN) : cbuff_hr.rd_ptr;
                report_raw_data_keep_connection();
                int32_t *data = ry_malloc(25*4*6);
                
                for (int i = 24; i >= 0; i--) {
                    int8_t acc_ptr_history = acc_ptr - i;
                    acc_ptr_history = (acc_ptr_history < 0) ? (acc_ptr_history + FIFO_BUFF_DEEP) : acc_ptr_history;
                    int8_t hrm_ptr_history = hrm_ptr - i;
                    hrm_ptr_history = (hrm_ptr_history < 0) ? (hrm_ptr_history + AFE_BUFF_LEN) : hrm_ptr_history;
                    
                    // LOG_DEBUG_PPS("cbuff_acc.rd_ptr: %d, %6d, %6d, %6d, %5d, %5d, %5d\r\n", \
                        cbuff_acc.rd_ptr & (FIFO_BUFF_DEEP - 1), hr_led_ir[hrm_ptr_history], \
                        hr_led_green[hrm_ptr_history], \
                        hr_led_amb[hrm_ptr_history], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_X], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Y], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Z]); 

                    data[(24-i)*6+0] = (int32_t)p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_X];         // x
                    data[(24-i)*6+1] = (int32_t)p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Y];         // y
                    data[(24-i)*6+2] = (int32_t)p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Z];         // z
                    data[(24-i)*6+3] = (int32_t)s_alg.step_freq; 
                    data[(24-i)*6+4] = (int32_t)(s_alg.step_cnt + s_alg.sports_mode * 10000 + alg_ctx_v.wrist_last * 1000);   
                    data[(24-i)*6+5] = (int32_t)s_alg.step_cnt_today;    
                }
                rbp_alg_raw_data_report(data, 25);

                ry_free((void*)data);
            }
#if 0
            {
                uint8_t acc_ptr = (cbuff_acc.rd_ptr >= FIFO_BUFF_DEEP) ? (cbuff_acc.rd_ptr - FIFO_BUFF_DEEP) : cbuff_acc.rd_ptr;            
                for (int i = 24; i >= 0; i--) {
                    int8_t acc_ptr_history = acc_ptr - i;
                    acc_ptr_history = (acc_ptr_history < 0) ? (acc_ptr_history + FIFO_BUFF_DEEP) : acc_ptr_history;
                    LOG_DEBUG_PPS("cbuff_acc.rd_ptr: %d, acc_history: %d, %5d, %5d, %5d\r\n", \
                        cbuff_acc.rd_ptr & (FIFO_BUFF_DEEP - 1), acc_ptr_history, \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_X], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Y], \
                        p_acc_data->RawData[acc_ptr_history][M_DRV_MC_UTL_AXIS_Z]); 
                }
            }
#endif
            algorithm_preProcess();
            algorithm_sc_cal(&s_alg.step_freq, &s_alg.step_cnt); 
            s_alg.step_cnt_update += s_alg.step_cnt;
            s_alg.still_state = algo_isStill();
            alg_sleep_processing(ALGO_IMU, s_alg.still_state);
            alg_sport_mode_processing(ALGO_IMU);
            //extern uint8_t tempState;
            // LOG_DEBUG("[acc_task]alg_rst: step_today: %d, step_freq: %d, step_cnt: %d, algo_isStill: %d, tempState: %d , alg_sports_mode: %d \r\n", \
                s_alg.step_cnt_today, s_alg.step_freq, s_alg.step_cnt, s_alg.still_state, 1, s_alg.sports_mode);
            algorithm_clearBufFullFlag();            
            cbuff_acc.cnt = 0;
            alg_ctx_v.alg_thresh = 25;  
            alg_ctx_v.idle_cnt = 0;
            ret = ALG_RESAULT_IMU;                              
        }
        
        if (cbuff_acc.rd_ptr == cbuff_acc.head){
            // LOG_DEBUG_PPS("[acc] ataToAlg buffer is null, cbuff_acc.head: %d, cbuff_acc.rd_ptr: %d\n", \
                cbuff_acc.head, cbuff_acc.rd_ptr);
            break;
        }

    }while(0);
    return ret;
}



void alg_data_report_disable(void)
{
    alg_ctx_v.data_report = 0;
}


/**
 * @brief   simple wrist processing
 *
 *
 * @param   None
 *
 * @return  wrist detect result - 0: not detect, 1: detected
 */

/**
 * @brief   simple wrist processing
 *
 *
 * @param   None
 *
 * @return  wrist detect result - 0: not detect, 1: detected
 */
uint8_t alg_wrist_detect(void)
{
    do{ 
        uint8_t reading = 10;
        while(reading && (cbuff_wrist.cnt < alg_ctx_v.wrist_thresh)) {
            if (cbuff_wrist.rd_ptr != cbuff_acc.head){
                reading --;
                uint16_t head = (cbuff_acc.head > cbuff_wrist.rd_ptr) ? cbuff_acc.head : (cbuff_acc.head + (FIFO_BUFF_DEEP<<1) - cbuff_wrist.rd_ptr);
                if (head - cbuff_wrist.rd_ptr > WRIST_THRESH_RUN){
                    cbuff_wrist.rd_ptr = cbuff_acc.head - WRIST_THRESH_RUN;
                }
                uint8_t cbuff_ptr = (cbuff_wrist.rd_ptr >= FIFO_BUFF_DEEP) ? (cbuff_wrist.rd_ptr - FIFO_BUFF_DEEP) : cbuff_wrist.rd_ptr;          
                cbuff_ptr = cbuff_ptr & (FIFO_BUFF_DEEP - 1);
                if (cbuff_ptr != (cbuff_acc.head & (FIFO_BUFF_DEEP - 1))){
                    cbuff_wrist.cnt ++;
                    cbuff_wrist.rd_ptr ++;
                    if (cbuff_wrist.rd_ptr >= (FIFO_BUFF_DEEP << 1)){
                        cbuff_wrist.rd_ptr = 0;
                    }
                    algo_accAddToWristBuf(  &p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_X], \
                                            &p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Y], \
                                            &p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Z], \
                                            1);
                    // LOG_DEBUG("[ws_add] acc.head:%d, rdPtr_init:%d, rdPtr_last:%d, ws_ptr:%d, head:%d, rd_ptr:%d, x: %d, y:%d, z:%d, result: %d, trace:%d, traceCnt:%d, sstd:%.1f\r\n", \
                        cbuff_acc.head, rdPtr_init, rdPtr_last, cbuff_wrist.rd_ptr, head, cbuff_ptr,\
                        p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_X], p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Y], p_acc_data->RawData[cbuff_ptr][M_DRV_MC_UTL_AXIS_Z],\
                        wrist_last, trace_last, alg_ctx_v.ws_trace, ws_sstd);
                }
                else{
                    reading = 0;
                }
            }
            else{
                reading = 0;
            }
        }

        // LOG_DEBUG("[wrist] dataToAlg: cbuff_acc.head: %d, cbuff_acc.tail: %d, cbuff_wrist.cnt: %d, cbuff_wrist.rd_ptr: %d\n", \
            cbuff_acc.head, cbuff_acc.tail, cbuff_wrist.cnt, cbuff_wrist.rd_ptr);

        if (cbuff_wrist.cnt >= alg_ctx_v.wrist_thresh){   
            alg_ctx_v.wrist_thresh = WRIST_THRESH_RUN;                                      
        }

        if (algo_isAccWristBufFull()){
            // algorithm_preProcess();
            alg_ctx_v.wrist_now = algo_ws_cal();
            uint8_t trace = 0;
            float ws_sstd = 0.0f;

            //algo_ws_dbg(&trace, &ws_sstd);
            
            //LOG_DEBUG("[ws] result: %d, trace:%d, traceCnt:%d, sstd:%.1f\r\n", \
                alg_ctx_v.wrist_now, trace, alg_ctx_v.ws_trace, ws_sstd);
            alg_ctx_v.ws_trace = (trace != 0) ? 0 : (++alg_ctx_v.ws_trace);
            
            // algorithm_clearBufFullFlag();            
            cbuff_wrist.cnt = 0;
            alg_ctx_v.wrist_thresh = WRIST_THRESH_RUN;                                      
        }
        
        if (cbuff_wrist.rd_ptr == cbuff_acc.head){
            // LOG_DEBUG_PPS("[acc] ataToAlg buffer is null, cbuff_acc.head: %d, cbuff_wrist.rd_ptr: %d\n", \
                cbuff_acc.head, cbuff_wrist.rd_ptr);
            break;
        }
    }while(0);
    return alg_ctx_v.wrist_now;
}


void wrist_detect_processing(void)
{
    static int continue_wrist_down_cnt = 0;
    wrist_filt_t wf;
    
    if (alg_ctx_v.wrist_detect_cnt > 32){
        int alg_wrist = alg_wrist_detect();
        if (alg_wrist == 1){       
            if (1 || !alg_ctx_v.wrist_last){
                wf.type = ALG_WRIST_FILT_UP;
                LOG_DEBUG("[alg_thread] alg_wrist_detect Yes %d\r\n", alg_ctx_v.wrist_detect_cnt);
                ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_WRIST_FILT, sizeof(wrist_filt_t), (u8_t*)&wf);
            }
            alg_ctx_v.wrist_last = 1;
            continue_wrist_down_cnt = 0;
        }
        else if (alg_wrist == 3){
            if (1 || alg_ctx_v.wrist_last  && continue_wrist_down_cnt >= 0){
                wf.type = ALG_WRIST_FILT_DOWN;
                LOG_DEBUG("[alg_thread] alg_wrist_detect No, %d\r\n", alg_ctx_v.wrist_detect_cnt);
                ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_WRIST_FILT, sizeof(wrist_filt_t), (u8_t*)&wf);
                alg_ctx_v.wrist_last = 0;
                continue_wrist_down_cnt = 0;
            } else {
                continue_wrist_down_cnt++;
            }            
        }
    }
    else{
        alg_ctx_v.wrist_detect_cnt ++;
    }
}


void alg_application_processing(uint32_t result_type)
{
    static uint32_t step_last;        

    if ((s_alg.step_cnt_update >= 25) || (s_alg.step_cnt_update <= -25)){
        s_alg.step_cnt_update = 0;
    }

    step_freq_ave_calc(s_alg.step_freq);

    if (s_alg.step_cnt_update > 0){
        s_alg.step_cnt_today += s_alg.step_cnt_update;
        s_alg.step_cnt_update = 0;
    }

    if (s_alg.step_cnt_today > ALG_MAX_STEP_TODAY){
        s_alg.step_cnt_today = ALG_MAX_STEP_TODAY;
    }
    if (step_last != s_alg.step_cnt_today){
        if (ry_system_initial_status() != 0){
            uint32_t type = IPC_MSG_TYPE_SURFACE_UPDATE_STEP;
            ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, sizeof(uint32_t), (uint8_t *)&type);
        }
        step_last = s_alg.step_cnt_today;
    }

    if ((s_alg.step_target == 0) && (s_alg.step_cnt_today >= get_target_step())){
		if((current_time_is_dnd_status() != 1) && (get_goal_remind_enable())){
			motar_light_linear_working(200);
			alg_msg_transfer(ALG_MSG_10000_STEP);
            DEV_STATISTICS_SET_VALUE(target_finish_time, ryos_rtc_now());
		}
        s_alg.step_target = 1;
        //LOG_DEBUG("[sensor_alg] 10000 step: %d, target:%d\n", \
                   s_alg.step_cnt_today, get_target_step()); 
    }
    alg_long_sit_processing();

    if (alg_get_still_state()){
        s_alg.still_time ++;
    }
    else{
        s_alg.still_time = 0;
    }

    if (s_alg.still_time >= ALG_THRESH_STILL_LONG){
        if (s_alg.still_long_status != ALG_STILL_LONG_YES){
            s_alg.still_long_status = ALG_STILL_LONG_YES; 
            // LOG_DEBUG("[alg_app] 1 s_alg.still_time: %d, s_alg.still_long_status:%d\n", \
                s_alg.still_time, s_alg.still_long_status);            
            ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_STILL_LONG, 1, &s_alg.still_long_status);
        }
    }
    else{
        if (s_alg.still_long_status != ALG_STILL_LONG_NO){
            s_alg.still_long_status = ALG_STILL_LONG_NO;
            // LOG_DEBUG("[alg_app] 2 s_alg.still_time: %d, s_alg.still_long_status:%d\n", \
                s_alg.still_time, s_alg.still_long_status); 
            ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_STILL_LONG, 1, &s_alg.still_long_status);
        }
    }

    if (alg_ctx_v.sleep_enalbe == 0){
        if (alg_ctx_v.unWeared_and_still != ALG_UNWEARED_AND_STILL){
            alg_ctx_v.unWeared_and_still = ALG_UNWEARED_AND_STILL;
            ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_WEARED_CHANGE, 1, &alg_ctx_v.unWeared_and_still);
            alg_long_sit_reset();
        }
    }
    else{
        if (alg_ctx_v.unWeared_and_still != ALG_WEARED_OR_UNSTILL){
            alg_ctx_v.unWeared_and_still = ALG_WEARED_OR_UNSTILL;
            ry_sched_msg_send(IPC_MSG_TYPE_ALGORITHM_WEARED_CHANGE, 1, &alg_ctx_v.unWeared_and_still);
            alg_long_sit_reset();            
        }
    }

    // LOG_DEBUG("[alg_app] alg_get_still_state:%d, alg_ctx_v.sleep_enalbe: %d, alg_ctx_v.unWeared_and_still:%d, dist:%f\n", \
                alg_get_still_state(), alg_ctx_v.sleep_enalbe, alg_ctx_v.unWeared_and_still, app_get_distance_today()); 
#if 0
    if (result_type == ALG_RESAULT_HRM){
        /* Send hrps data through BLE */
        if (ry_ble_get_state() == RY_BLE_STATE_CONNECTED) {
            static u16_t hrpsDataIndex = 0;
            u8_t hrpsBuf[20];
            hrpsBuf[0] = 0x06;
            hrpsBuf[1] = s_alg.hr_cnt;
            hrpsBuf[2] = LO_UINT16(s_alg.step_cnt_today);
            hrpsBuf[3] = HI_UINT16(s_alg.step_cnt_today);
            hrpsBuf[4] = s_alg.step_freq;
            hrpsBuf[5] = 0;
            hrpsBuf[6] = LO_UINT16(hrpsDataIndex);
            hrpsBuf[7] = HI_UINT16(hrpsDataIndex);
            hrpsDataIndex++;
            ble_hrDataSend(hrpsBuf, 8);
        }
    }
#endif

    // gsensor_get_xyz(&acc_raw);
    // LOG_DEBUG("[sensor_alg] latest gsensor xyz: %d %d %d %d\n", \
        s_alg.mode, acc_raw.acc_x, acc_raw.acc_y, acc_raw.acc_z);   
}

static inline void alg_reset_hrm_mode(void)
{
    algorithm_setMode(ALGO_HRS);
    algorithm_clearBufFullFlag();    
    alg_ctx_v.alg_thresh = 128;
    alg_ctx_v.wrist_thresh = WRIST_THRESH_INIT;
    s_alg.mode = ALGO_HRS;
    cbuff_hr.cnt = 0;
    cbuff_acc.cnt = 0;   
    alg_ctx_v.wrist_detect_cnt = 0;
}

static inline void alg_reset_imu_mode(void)
{
    algorithm_setMode(ALGO_IMU);
    algorithm_clearBufFullFlag();   
    cbuff_acc.cnt = 0; 
    alg_ctx_v.alg_thresh = 128;
    alg_ctx_v.wrist_thresh = WRIST_THRESH_INIT;
    s_alg.mode = ALGO_IMU;
    alg_ctx_v.wrist_detect_cnt = 0;
}

static inline void alg_hrm_imu_mode_deteimine(void)
{
    if (get_hrm_state() != s_alg.last_hrm_mode){
        if ((s_alg.mode != ALGO_HRS) && (get_hrm_state() == PM_ST_POWER_START)){
            alg_reset_hrm_mode();
            LOG_DEBUG("[sensor_alg]: new_set_hrm_mode\r\n");
        }
        if ((s_alg.mode == ALGO_HRS) && (get_hrm_state() != PM_ST_POWER_START)){
            alg_reset_imu_mode();
            LOG_DEBUG("[sensor_alg]: new_set_imu_mode\r\n");
        }
        s_alg.last_hrm_mode = get_hrm_state();
    }
}

void set_algo_hrm_mode(void)
{
    alg_reset_hrm_mode();
}

void set_algo_imu_mode(void)
{
    alg_reset_imu_mode();
}

uint8_t get_algo_mode(void)
{
    return s_alg.mode;
}


/**
 * @brief   Message handler from Scheduler
 *
 * @param   msg - IPC alg message
 *
 * @return  None
 */
void ry_alg_msg_handler(ry_alg_msg_t *msg)
{
    switch (msg->msgType) {
        case IPC_MSG_TYPE_SYSTEM_MONITOR_ALG:
            {
                monitor_msg_t* mnt_msg = (monitor_msg_t*)msg->data;
                if (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_ALG){
                    *(uint32_t *)mnt_msg->dataAddr = 0;
                }
                break;
            }
        case IPC_MSG_TYPE_ALGORITHM_DATA_SYNC_WAKEUP:
            sleep_wake_up_from(ALG_WAKEUP_FROM_DATA_SYNC);
            break;

        case IPC_MSG_TYPE_ALGORITHM_DATA_CHECK_WAKEUP:
            sleep_wake_up_from(ALG_WAKEUP_FROM_CHECK);        
            break;
        
        case IPC_MSG_TYPE_ALGORITHM_HRM_CHECK_WAKEUP:
            sleep_wake_up_from(ALG_WAKEUP_FROM_HRM);        
            break;

        default:
            break;
    }
    // LOG_DEBUG("[ry_alg_msg_handler], msg->msgType: 0x%x \r\n", msg->msgType);
    ry_free((void*)msg);
    return;
}

/**
 * @brief   SENSOR_ALG Main task
 *
 * @param   None
 *
 * @return  None
 */
int sensor_alg_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    uint32_t alg_result_update = 0;
    ry_alg_msg_t *msg = NULL;

#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_ALGO);
#endif
    algorithm_getVersion(alg_ctx_v.version);
    LOG_DEBUG("sensor_alg start, algorithm version is %s.\n\r", alg_ctx_v.version);
    
    algorithm_init();
    algo_ws_init();
    algorithm_setBodyInfo(body_inifo);
    algorithm_setMode(ALGO_IMU);
    //algorithm_setMode(ALGO_HRS);
#if ALG_SLEEP_CALC_DESAY                                        
    InitSleepAlgorithm();       //initial Desay sleep algorithm
#endif

	status = ryos_queue_create(&ry_queue_alg, 5, sizeof(void*));

    /* Create sleep log timer */
    ry_timer_parm timer_para;
    timer_para.timeout_CB = (void (*)(void*))sleep_log_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "sleep_log_timer";
    sleep_log_timer_id = ry_timer_create(&timer_para);

    //s_alg.mode = ALGO_HRS;
    s_alg.mode = ALGO_IMU;
    s_alg.sports_mode_last = SPORTS_LIGHT_ACTIVE;
    s_alg.sports_mode = SPORTS_LIGHT_ACTIVE;
    s_alg.last_hrm_mode = 0xff; 
    s_alg.still_long_status = ALG_STILL_LONG_NO;

    s_alg.hr_min1 = (s_alg.hr_min1 < 30) ? 60 : s_alg.hr_min1;
    s_alg.hr_min2 = (s_alg.hr_min2 < 30) ? 60 : s_alg.hr_min2;
    algo_slp_updateHrMin(s_alg.hr_min1, s_alg.hr_min2);  

    while(1) {   
        alg_ctx_v.time_start = ry_hal_clock_time();
        if (RY_SUCC == ryos_queue_recv(ry_queue_alg, &msg, RY_OS_NO_WAIT)) {
            ry_alg_msg_handler(msg);
        }

        alg_hrm_imu_mode_deteimine();
        gsensor_get_data();
        if (s_alg.mode == ALGO_HRS){
            if (get_ppg_stage() == HRM_HW_ST_MEASURE){
                alg_result_update = alg_hrm_processing();
            }
            else{
                cbuff_hr.head = 0;
                cbuff_hr.rd_ptr = 0;
                cbuff_acc.head = 0;
                cbuff_acc.rd_ptr = 0;
            }
        }
        else if (s_alg.mode == ALGO_IMU){
            alg_result_update = alg_imu_processing();
        }
        
        wrist_detect_processing();
        if (alg_result_update != ALG_RESAULT_NONE){
            alg_application_processing(alg_result_update);  
        }
        
        if (alg_ctx_v.idle_cnt >= ALG_IDLE_CNT_THRESH_RESET){
            LOG_ERROR("[alg_error]: idle_cnt:%d, algTh:%d, wrisTh:%d, hrCnt:%d, aCnt:%d.\r\n", \
                alg_ctx_v.idle_cnt, alg_ctx_v.alg_thresh, alg_ctx_v.wrist_thresh, cbuff_hr.cnt, cbuff_acc.cnt);  
            cbuff_hr.head = 0;
            cbuff_hr.rd_ptr = 0;
            cbuff_acc.head = 0;
            cbuff_acc.rd_ptr = 0;
            if (s_alg.mode == ALGO_HRS){
                alg_reset_hrm_mode();
            }
            else{
                alg_reset_imu_mode();
            }
            alg_ctx_v.idle_cnt = 0;        
        }
        alg_ctx_v.thread_time += ry_hal_calc_ms(ry_hal_clock_time() - alg_ctx_v.time_start);
        if (tms_alg_enable_get() && is_raise_to_wake_open()){
            alg_ctx_v.idle_cnt ++;
            alg_ctx_v.thread_st = 1;
            ryos_delay_ms(60);
        }
        else{
            alg_ctx_v.idle_cnt += ALG_IDLE_CNT_ONE_SECONDE;
            alg_ctx_v.thread_st = 0;
            ryos_delay_ms(1000);
        }

#if 0
        static int cnt_ctrl = 0;
        cnt_ctrl++;
        if((cnt_ctrl > 10)
            ||(alg_ctx_v.thread_time > 200)
            ){
            LOG_DEBUG("[alg_sensor] cnt:%d thread_time:%d ms\n", cnt_ctrl, alg_ctx_v.thread_time);
            alg_ctx_v.thread_time = 0;
            cnt_ctrl = 0;
        }
#endif
    }
}

/**
 * @brief   API to init gsensor module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t sensor_alg_init(void)
{
    ryos_threadPara_t para;
    
    /* Start a thread */
    strcpy((char*)para.name, "sensor_alg_thread");
    para.stackDepth = SENSOR_ALG_THREAD_STACK_SIZE;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = SENSOR_ALG_THREAD_PRIORITY;
    return ryos_thread_create(&sensor_alg_thread_handle, sensor_alg_thread, &para);
}

/**
 * @brief   API to alg_thread_get_status
 *
 * @param   para_1: pointer to the first monitor para
 *          para_2: pointer to the second monitor para
 *
 * @return  None
 */
 void alg_thread_get_status(uint32_t* para_1, uint32_t* para_2)
{
    *para_1 = sleep_time_dyn_calc();
    *para_2 = (s_alg.sports_mode << 24) | (s_alg.mode << 16) | (alg_ctx_v.idle_cnt);
}

/*
===================================alg results api=======================================
*/

/**
 * @brief   API to alg_get_step_today
 *
 * @param   None
 *
 * @return  value - step_cnt today 
 */
uint32_t alg_get_step_today(void)
{
    return (s_alg.step_cnt_today > 0) ? s_alg.step_cnt_today : 0;
}

void alg_clear_results_today(void)
{
    u32_t * new_status = (u32_t *)ry_malloc(sizeof(u32_t));
    *new_status = s_alg.sports_mode;
    if(get_running_flag() == 0){
        ry_data_msg_send(DATA_SERVICE_BODY_STATUS_CHANGE, sizeof(u32_t), (u8_t *)new_status);
    }
    ry_free(new_status);  
    ryos_delay_ms(20);
    s_alg.step_target = 0;    
    s_alg.step_cnt_today = 0;
    s_alg.hr_awake = 255;  
    s_alg.long_sit_times = 0;  
}

/**
* @brief API to alg_get_long_sit_times
*
* @param None
*
* @return value - long_sit_times today 
*/
uint32_t alg_get_long_sit_times(void)
{
    return s_alg.long_sit_times;
}

/**
 * @brief   API to alg_get_step_freq_latest
 *
 * @param   None
 *
 * @return  value - step_freq_latest
 */
uint32_t alg_get_step_freq_latest(void)
{
    return s_alg.step_freq;
}

/**
 * @brief   API to alg_get_step_cnt_latest
 *
 * @param   None
 *
 * @return  value - step_cnt_latest
 */
uint32_t alg_get_step_cnt_latest(void)
{
    return s_alg.step_cnt;
}

/**
 * @brief   API to alg_get_hrm_cnt_latest
 *
 * @param   None
 *
 * @@return  value - alg_get_hrm_cnt_latest 
 */
uint8_t  alg_get_hrm_cnt_latest(void)
{
    return s_alg.hr_cnt;
}

/**
 * @brief   API to alg_get_work_mode
 *
 * @param   None
 *
 * @return  value - ALGO_IMU: 0, ALGO_HRS: 1
 */
uint8_t  alg_get_work_mode(void)
{
    return s_alg.mode;
}

/**
 * @brief   API to alg_get_sports_mode
 *
 * @param   None
 *
 * @return  value - sports_mode
 *          typedef enum {
 *                SPORTS_STILL = 0,
 *                SPORTS_RUN,
 *                SPORTS_NORMAL_WALK,
 *                SPORTS_FAST_WALK,
 *                SPORTS_BIKE = 7,
 *                SPORTS_NONE = 9
 *          }sports_mode_t;
 */
uint8_t  alg_get_sports_mode(void)
{
    return s_alg.sports_mode;
}

/**
 * @brief   API to alg_get_sports_times
 *
 * @param   None
 *
 * @return  today sports_times
 */
uint16_t  alg_get_sports_times(void)
{
    return s_alg.sports_times;
}

/**
 * @brief   API to alg_get_work_mode
 *
 * @param   None
 *
 * @return  state - 1: is still, 0: not still
 */
uint8_t  alg_get_still_state(void)
{
    return s_alg.still_state;
}

/**
 * @brief   API to alg_get_still_time
 *
 * @param   None
 *
 * @return  still total time from last un-still
 */
uint32_t  alg_get_still_time(void)
{
    return s_alg.still_time;
}

/**
 * @brief   API to get alg_still_longtime
 *
 * @param   None
 *
 * @return  alg_still_longtime more than 1 minute
 */
uint32_t  alg_still_longtime(void)
{
    return (s_alg.still_time >= ALG_THRESH_STILL_LONG);
}

/**
 * @brief   API to alg_get_weard_state from alg when hrm is on
 *
 * @param   None
 *
 * @return  state - 1: is weaded, 0: not weaded
 */
uint8_t  alg_get_weard_state(void)
{
    return s_alg.weared;
}

/**
 * @brief   API to alg_get_weard_and_still status
 *
 * @param   None
 *
 * @return  state - 1: ALG_WEARED_OR_UNSTILL, 0: ALG_UNWEARED_AND_STILL
 */
uint8_t  alg_get_weard_and_still_status(void)
{
    return alg_ctx_v.unWeared_and_still;
}

/**
 * @brief   API to alg_get_weard_state
 *
 * @param   state - 1: is weaded, 0: not weaded
 *
 * @return  None
 */
void  alg_set_weard_state(uint8_t st)
{
    s_alg.weared = st;
}

/**
 * @brief   API to alg_get_sleep_state
 *
 * @param   None
 *
 * @return  state - 1: is sleep, 0: not sleep
 */
uint8_t  alg_get_sleep_state(void)
{
    return s_alg.is_sleep;
}


/**
 * @brief   API to alg_status_is_in_sleep
 *
 * @param   None
 *
 * @return  state - 1: is sleep, 0: not sleep
 */
uint32_t  alg_status_is_in_sleep(void)
{
    u32_t is_in_sleep = 0;
    if (SLEEP_IS_IN_DAY_MODE){
        is_in_sleep = ((s_alg.is_sleep == SLP_SLEEP) || \
                       (s_alg.is_sleep == SLP_SLEEP_DEEP));
    } else {
        is_in_sleep = ((s_alg.is_sleep == SLP_SUSPECT) || \
                     (s_alg.is_sleep == SLP_SLEEP) || \
                     (s_alg.is_sleep == SLP_SLEEP_DEEP));
    }
    return is_in_sleep;
}

/**
 * @brief   API to alg_get_hrm_cnt
 *
 * @param   hrm_cnt - pointer to hrm_cnt
 *
 * @return  state - -1: fail, else: succ
 */
int32_t  alg_get_hrm_cnt(uint8_t* hrm_cnt)
{
    int32_t ret = -1;
    if ((get_hrm_state() == PM_ST_POWER_START) && (alg_get_work_mode() == ALGO_HRS)){
        *hrm_cnt = s_alg.hr_cnt;
        // LOG_DEBUG("[alg_get_hrm_cnt] succ, hrm_cnt: %d\r\n", *hrm_cnt);        
        ret = 0;
    }
    else{
        LOG_DEBUG("[alg_get_hrm_cnt] fail\r\n");
    }
    return ret;
}

/**
 * @brief   API to alg_get_hrm_awake_cnt
 *
 * @param   hrm_cnt - pointer to hrm_awake_cnt
 *
 * @return  state - -1: fail, else: succ
 */
int32_t  alg_get_hrm_awake_cnt(uint8_t* hrm_cnt)
{
    int32_t ret = -1;
    if ((sleep_time.f_wakeup_updated != 0) && (sleep_time.f_sleep_updated != 0)){
        *hrm_cnt = s_alg.hr_awake;
        // LOG_DEBUG("[alg_get_hrm_cnt] succ, hrm_cnt: %d\r\n", *hrm_cnt);        
        ret = 0;
    }
    else{
        LOG_DEBUG("[alg_get_hrm_cnt] fail\r\n");
    }
    return ret;
}


void alg_long_sit_reset(void)
{
    long_sit_minute_acculate = 0;
    long_sit_still_min_this_hour = 0;
    long_sit_start_time = ry_hal_clock_time();
}



/**
 * @brief   API to alg_long_sit_detect
 *
 * @param   None
 *
 * @return  long sit result - 0: not long_sit, else: long sit
 */
uint32_t  alg_long_sit_detect(void)
{
    uint32_t long_sit = 0;
    static uint32_t calc_start_time_per_min;
    static uint8_t  alg_second_this_min;
    static uint8_t  still_second_this_min;
    static uint8_t  noStep_second_this_min;

    alg_second_this_min ++;
    if (alg_get_still_state()){
        still_second_this_min ++;
    }

    if (s_alg.step_cnt == 0){
        noStep_second_this_min ++;
    }
    if (ry_hal_calc_second(ry_hal_clock_time() - calc_start_time_per_min) >= 60){
        if (noStep_second_this_min > alg_second_this_min - 10){
            long_sit_minute_acculate ++;
        }
        else{
            alg_long_sit_reset();
        }

        if (still_second_this_min >= alg_second_this_min - 10){
            long_sit_still_min_this_hour ++;
        }

        if (long_sit_minute_acculate >= LONG_SIT_TIME_THRESH){
            if (ry_hal_calc_second(ry_hal_clock_time() - long_sit_start_time) <= (LONG_SIT_TIME_THRESH + 5) * 60){
                if (long_sit_still_min_this_hour < LONG_SIT_TIME_THRESH - 5){
                    long_sit = 1; 
                    LOG_DEBUG("[longSit] Detected. time:%d, stillMin:%d, sitMin:%d\r\n", \
                        ry_hal_calc_second(ry_hal_clock_time() - long_sit_start_time), long_sit_still_min_this_hour, long_sit_minute_acculate);
                }
            }
            alg_long_sit_reset();
        }
        noStep_second_this_min = 0;
        still_second_this_min = 0;
        alg_second_this_min = 0;
        calc_start_time_per_min = ry_hal_clock_time();
    }
    // LOG_DEBUG("[long_sit_det]time: %d, noStep_second_this_min: %d, long_sit_minute_acculate: %d, long_sit_still_min_this_hour: %d, still_second_this_min: %d, alg_second_this_min: %d\r\n", \
            ry_hal_calc_ms(ry_hal_clock_time() - calc_start_time_per_min), noStep_second_this_min, long_sit_minute_acculate,\
            long_sit_still_min_this_hour, still_second_this_min, alg_second_this_min);
    
    return long_sit;
}

#define STEP_COEFFICIENT            0.3f

float alg_get_correction_distance_coefficient()
{
    return get_step_distance_coef() * 0.1f;
}

/**
 * @brief   API to alg_get_distance_today
 *
 * @param   None
 *
 * @return  alg_get_distance_today
 */
float alg_get_distance_today(void)
{
    return ((alg_get_step_today() * get_user_height() * STEP_COEFFICIENT
        * alg_get_correction_distance_coefficient())) * 0.01f;
}

/**
 * @brief   API to alg_calc_calorie
 *
 * @param   None
 *
 * @return  alg_calc_calorie
 */
float alg_calc_calorie(void)
{
    // LOG_DEBUG("%f,%f=====\n", get_user_weight(), alg_get_distance_today());
    return (get_user_weight() * alg_get_distance_today() * 0.86f);
}

float get_step_calorie(u32_t step)
{
    return (get_user_weight() * ((step * get_user_height() * STEP_COEFFICIENT
        * alg_get_correction_distance_coefficient()) * 0.01f) * 0.86f);
}

float get_step_calorie_sport(u32_t step)
{
    return (get_user_weight() * ((step * get_user_height() * STEP_COEFFICIENT
        * alg_get_correction_distance_coefficient()) * 0.01f) * 1.06f);
}

float alg_calc_calorie_by_distance(float distance, uint32_t sport_mode)
{
    const float para_k[] = {0.86f, 1.06f};
    sport_mode = (sport_mode > 1) ? 0 : sport_mode;
    float calorie = get_user_weight() * distance * para_k[sport_mode];
    return calorie;
}

/**
 * @brief   API to get_hrm_calorie
 *
 * @param   hrm: average hrm cnt per hour, minute: calculate time
 *
 * @return  calorie: result from: hrm, age, gender, sports time, weight
 */
float get_hrm_calorie(u32_t hrm, float minute)
{
    user_data_birth_t* birth = get_user_birth();
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time);
    uint32_t age = cur_time.year + 2000 - birth->year;
    uint32_t weight = get_user_weight();
    uint32_t gender = (get_user_gender() == 0) ? 0 : 1; 
    float calorie = 0;

    if ((age > 60) || (age < 10)){
        age = 30;
    }

    if ((weight > 200) || (weight < 10)){
        weight = 60;
    }

    //calorie reference: http://www.boohee.com/posts/view/261912
    //male
    if (gender == 1){
        calorie = (-55.0969 + 0.6309 * hrm + 0.1988 * weight + 0.2017 * age) * minute / 4.184;
    }
    else{
        calorie = (-20.4022 + 0.4472 * hrm + 0.1263 * weight + 0.074 * age) * minute / 4.184;
    }
    
    if (calorie < 0){
        calorie = 0;
    }
    
    // LOG_DEBUG("gender: %d, weight: %d, age: %d, hrm: %d, minute: %.4f, calorie: %.1f\r\n", \
        gender, weight, age, hrm, minute, calorie);
    return calorie;
}

float get_hrm_calorie_second(u32_t hrm, float seconds)
{
    user_data_birth_t* birth = get_user_birth();
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time);
    uint32_t age = cur_time.year + 2000 - birth->year;
    uint32_t weight = get_user_weight();
    uint32_t gender = (get_user_gender() == 0) ? 0 : 1; 
    float calorie = 0;

    if ((age > 60) || (age < 10)){
        age = 30;
    }

    if ((weight > 200) || (weight < 10)){
        weight = 60;
    }

    //calorie reference: http://www.boohee.com/posts/view/261912
    //male
    if (gender == 1) {
        calorie = (-55.0969f + 0.6309f * hrm + 0.1988f * weight + 0.2017f * age) * (seconds * 1000 / 60.0f) / 4.184f;
    }
    else {
        calorie = (-20.4022f + 0.4472f * hrm + 0.1263f * weight + 0.074f * age) * (seconds * 1000 / 60.0f) / 4.184f;
    }
    
//    if (calorie < 0){
//        calorie = 0;
//    }
    
    // LOG_DEBUG("gender: %d, weight: %d, age: %d, hrm: %d, minute: %.4f, calorie: %.1f\r\n", \
        gender, weight, age, hrm, minute, calorie);
    return calorie;
}

float alg_get_step_distance(u32_t step)
{
    return (step * get_user_height() * STEP_COEFFICIENT
        * alg_get_correction_distance_coefficient()) / 100;;
}


float alg_get_step_distance_on_stepFreq(u32_t step, u32_t stepFreq)
{
    if (stepFreq < 60){
        stepFreq = 60;    
    }
    else if (stepFreq > 180){
        stepFreq = 180;
    }
    float step_coef = 0.002747f * stepFreq + 0.1594f;
    return (step * get_user_height() * step_coef) / 100;
}


uint32_t step_freq_ave_calc(uint8_t step_freq)
{
    step_freq_all += step_freq;
    step_freq_cnt ++;
    step_freq_ave = step_freq_all / step_freq_cnt;
    extern uint32_t flg_running_start;
    extern int point_cnt;

    // SEGGER_RTT_printf(0, "step_freq_cnt:%d, step_freq:%d, step_freq_all:%d, step_today:%d. step_freq_ave:%d\r\n", \
        step_freq_cnt, s_alg.step_freq, step_freq_all, s_alg.step_cnt_today, step_freq_ave);

    return step_freq_ave;
}

uint32_t step_freq_ave_get(void)
{
    return step_freq_ave;
}


void step_freq_ave_reset(void)
{
    step_freq_all = 0;
    step_freq_cnt = 0;
    step_freq_ave = 0;
}
