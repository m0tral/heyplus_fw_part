/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __SENSOR_ALG_H__
#define __SENSOR_ALG_H__

/*
 * CONSTANTS
 *******************************************************************************
 */
#define ALG_WRIST_FILT_UP     0
#define ALG_WRIST_FILT_DOWN   1
#define ALG_MAX_STEP_TODAY    99999

#define ALG_STILL_LONG_NO     0
#define ALG_STILL_LONG_YES    1

#define ALG_WEARED_OR_UNSTILL     1
#define ALG_UNWEARED_AND_STILL    0

/*
 * TYPES
 *******************************************************************************
 */
typedef struct
{
    uint32_t timeStamp;
    int32_t  step_cnt_today; 
    uint8_t  step_target;
    uint8_t  mode;    
    uint8_t  last_hrm_mode;
    int8_t   step_cnt;
    int16_t  step_cnt_update;
    uint8_t  step_freq;
    uint8_t  hr_cnt;
    uint8_t  hr_awake;   
    uint8_t  hr_min1;                                  
    uint8_t  hr_min2;                                      
    uint8_t  sports_mode;   
    uint8_t  sports_mode_last;     
    uint16_t sports_times;    
    uint8_t  still_state; 
    uint8_t  weared;  
    uint8_t  is_sleep;      
    uint8_t  sleep_last;          
    uint8_t  long_sit_times;   
    uint8_t  still_long_status;     
    uint32_t still_time;     
}sensor_alg_t;

typedef struct {
    uint8_t sstd;
    int8_t rri[3];
}sleep_calc_t;

typedef struct {
    uint8_t hrm;    
    uint8_t sstd;
    int16_t rri;
}sleep_data_t;

typedef struct {
    uint8_t head;
    uint8_t tail;
    uint8_t rd_ptr;
    uint8_t cnt;
}cbuff_t;

typedef struct {
    u32_t type;
} wrist_filt_t;


typedef struct {
    uint8_t info_type;
    void    *pdata;
} alg_info_t;


typedef struct {
    uint32_t sleep_total_dyn;     
    uint32_t sleep_total_last;        
    uint32_t sleep_total;    
    uint8_t sleep_hour;
    uint8_t sleep_minute;
    uint8_t wakeup_hour;
    uint8_t wakeup_minute;
    uint8_t f_sleep_updated;
    uint8_t f_wakeup_updated;
} alg_sleep_time_t;


typedef enum {
    ALG_MSG_GENERAL = 1,
    ALG_MSG_SLEEP_SESSION_BEGINNING,  
    ALG_MSG_SLEEP_SESSION_FINISHED,          
    ALG_MSG_SLEEP_LOG_START,
    ALG_MSG_SLEEP_LOG_END,            
    ALG_MSG_SLEEP,
    ALG_MSG_HRM_MANUAL_STOP,    
    ALG_MSG_LONG_SIT,
    ALG_MSG_10000_STEP,    
}alg_sports_msg_t;

/**
 * @brief Message type for alg, which is compatible with IPC message
 */
typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t  data[1];
} ry_alg_msg_t;

typedef struct {
    char version[8];    
    uint8_t wrist_now;
    uint8_t wrist_detect_cnt;
    uint8_t wrist_last;
    uint8_t wrist_thresh;
    uint8_t alg_thresh;
    uint8_t hr_test_mode;
    uint8_t data_report;
    uint8_t halt;
    uint8_t long_sit_enable; 
    uint8_t awake_hrm_en; 
    uint8_t sleep_day_mode;          
    uint8_t sleep_pro;          
    uint8_t sleep_hr_state;   
    uint16_t sleep_hr_sample_time;         
    uint8_t sleep_state;
    uint8_t sleep_enalbe;  
    uint8_t unWeared_and_still;      
    uint8_t scheduler_enable;
    uint8_t thread_st;    
    uint8_t wake_src;    
    uint16_t sleep_log_time;
    uint16_t sleep_log_period;    
    uint16_t idle_cnt;        
    uint32_t ws_trace;   
    uint32_t thread_time;   
    uint32_t time_start;                                   
} alg_ctx_t;


/*
 * VARIABLES
 *******************************************************************************
 */

extern cbuff_t cbuff_hr;
extern cbuff_t cbuff_acc;
extern sensor_alg_t s_alg;
extern alg_sleep_time_t sleep_time;
/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to send IPC message to senson_alg module
 *
 * @param   type Which event type to subscribe
 * @param   len  Length of the payload
 * @param   data Payload of the message
 *
 * @return  Status
 */
ry_sts_t ry_alg_msg_send(u32_t type, u32_t len, u8_t *data);

/**
 * @brief   API to init sensor algorithm module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t sensor_alg_init(void);

/**
 * @brief   API to enable/disable raw data report.
 *
 * @param   fEnable - 1 Enable and 0 disable
 *
 * @return  None
 */
void alg_report_raw_data(u32_t fEnable);

void sensor_alg_suspend(void);
void sensor_alg_resume(void);

/**
 * @brief   API to alg_get_step_today
 *
 * @param   None
 *
 * @return  value - step_cnt today 
 */
uint32_t alg_get_step_today(void);


/**
 * @brief   API to alg_get_step_freq_latest
 *
 * @param   None
 *
 * @return  value - step_freq_latest
 */
uint32_t alg_get_step_freq_latest(void);

/**
 * @brief   API to alg_get_step_cnt_latest
 *
 * @param   None
 *
 * @return  value - step_cnt_latest
 */
uint32_t alg_get_step_cnt_latest(void);

/**
 * @brief   API to alg_get_hrm_cnt_latest
 *
 * @param   None
 *
 * @@return  value - alg_get_hrm_cnt_latest 
 */
uint8_t  alg_get_hrm_cnt_latest(void);

/**
 * @brief   API to alg_get_work_mode
 *
 * @param   None
 *
 * @return  value - ALGO_IMU: 0, ALGO_HRS: 1
 */
uint8_t  alg_get_work_mode(void);

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
uint8_t  alg_get_sports_mode(void);

/**
 * @brief   API to alg_get_work_mode
 *
 * @param   None
 *
 * @return  state - 1: is still, 0: not still
 */
uint8_t  alg_get_still_state(void);

/**
 * @brief   API to alg_get_weard_state from alg when hrm is on
 *
 * @param   None
 *
 * @return  state - 1: is weaded, 0: not weaded
 */
uint8_t  alg_get_weard_state(void);

/**
 * @brief   API to alg_get_weard_and_still status
 *
 * @param   None
 *
 * @return  state - 1: ALG_WEARED_OR_UNSTILL, 0: ALG_UNWEARED_AND_STILL
 */
uint8_t  alg_get_weard_and_still_status(void);

/**
 * @brief   API to alg_get_sleep_state
 *
 * @param   None
 *
 * @return  state - 1: is sleep, 0: not sleep
 */
uint8_t  alg_get_sleep_state(void);

/**
 * @brief   API to alg_get_hrm_cnt
 *
 * @param   hrm_cnt - pointer to hrm_cnt
 *
 * @return  state - -1: fail, else: succ
 */
int32_t  alg_get_hrm_cnt(uint8_t* hrm_cnt);

/**
 * @brief   API to alg_long_sit_detect
 *
 * @param   None
 *
 * @return  long sit result - 0: not long_sit, else: long sit
 */
uint32_t  alg_long_sit_detect(void);

/**
* @brief API to alg_get_long_sit_times
*
* @param None
*
* @return value - long_sit_times today 
*/
uint32_t alg_get_long_sit_times(void);

/**
 * @brief   API to alg_clear_results_today
 *
 * @param   None
 *
 * @return  None
 */
void alg_clear_results_today(void);

/**
 * @brief   API to alg_get_distance_today
 *
 * @param   None
 *
 * @return  alg_get_distance_today
 */
float alg_get_distance_today(void);

/**
 * @brief   API to alg_calc_calorie
 *
 * @param   None
 *
 * @return  alg_calc_calorie
 */
float alg_calc_calorie(void);


/**
 * @brief   API to get_step_calorie
 *
 * @param   step
 *
 * @return  get_step_calorie
 */
float get_step_calorie(u32_t step);

float alg_get_step_distance(u32_t step);

/**
 * @brief   API to alg_get_weard_state
 *
 * @param   state - 1: is weaded, 0: not weaded
 *
 * @return  None
 */
void  alg_set_weard_state(uint8_t st);

/**
 * @brief   API to get_hrm_calorie
 *
 * @param   hrm: average hrm cnt per hour, minute: calculate time
 *
 * @return  calorie: result from: hrm, age, gender, sports time, weight
 */
float get_hrm_calorie(u32_t hrm, float minute);
float get_hrm_calorie_second(u32_t hrm, float seconds);

void alg_sleep_scheduler_set_enable(uint8_t f_enable);

void alg_long_sit_set_enable(uint8_t f_enable);

/**
 * @brief   API to alg_status_is_in_sleep
 *
 * @param   None
 *
 * @return  state - 1: is sleep, 0: not sleep
 */
uint32_t  alg_status_is_in_sleep(void);

/**
 * @brief   API to alg_get_hrm_awake_cnt
 *
 * @param   hrm_cnt - pointer to hrm_awake_cnt
 *
 * @return  state - -1: fail, else: succ
 */
int32_t  alg_get_hrm_awake_cnt(uint8_t* hrm_cnt);

/**
 * @brief   API to alg_get_sports_times
 *
 * @param   None
 *
 * @return  today sports_times
 */
uint16_t  alg_get_sports_times(void);

/**
 * @brief   API to get alg_still_longtime
 *
 * @param   None
 *
 * @return  alg_still_longtime more than 1 minute
 */
uint32_t  alg_still_longtime(void);

/**
 * @brief   API to alg_get_still_time
 *
 * @param   None
 *
 * @return  still total time from last un-still
 */
uint32_t  alg_get_still_time(void);
void alg_data_report_disable(void);
void alg_msg_transfer(uint32_t info_type);

/**
 * @brief   API to alg_thread_get_status
 *
 * @param   para_1: pointer to the first monitor para
 *          para_2: pointer to the second monitor para
 *
 * @return  None
 */
 void alg_thread_get_status(uint32_t* para_1, uint32_t* para_2);

float alg_calc_calorie_by_distance(float distance, uint32_t sport_mode);

uint32_t step_freq_ave_calc(uint8_t step_freq);
uint32_t step_freq_ave_get(void);
float alg_get_step_distance_on_stepFreq(u32_t step, u32_t stepFreq);
void step_freq_ave_reset(void);

int32_t sleep_time_dyn_calc(void);

#endif  /* __SENSOR_ALG_H__ */

