/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_RUNNING_H__
#define __ACTIVITY_RUNNING_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */
#define GET_LOCATION_TIME               10      //second
#define GET_LOCATION_TIME_COLD_START    3       //second 

#define SPORTS_ACCESS_GPS_TIME           40      //second
#define SPORTS_ACCESS_GPS_OUT_TIME       3       //second


/*
 * TYPES
 *******************************************************************************
 */
typedef struct {
    float        distance;
    u32_t        duration_second;
    u32_t        delta_step;
    u8_t         hrm;
    u8_t         hrm_ave;
}ui_disp_data_t;


typedef struct {
    float        distance_1km_start;
    u32_t        start_durationSec_1km;
    u32_t        all_km;
    u32_t        start_step_1km;
}milestone_data_t;

typedef enum {
    STATE_ACCESS_GPS = 0,           //获取GPS准备状态
    STATE_ACCESS_GPS_SKIP = 1,      //用户跳过获取GPS准备，用户跳过自动等待GPS等待
    STATE_GPS_RESULT = 2,           //GPS获取结果：失败 or 成功
    STATE_SHOW_COUNT = 3,           //运动开启前的倒计时
    STATE_RUNNING_DOING = 4,        //运动中
    STATE_ONE_KM_ALERT = 5,         //运动中达成目标，目标提示状态
    STATE_GPS_CHANGED = 6,          //运动中，GPS信号变化提示状态
    STATE_RUNNING_BREAK = 7,        //运动中，用户选择后退键的"跳出选择"状态
    STATE_RUNNING_PAUSING = 8,      //跳出选择状态后，选择的“暂停状态"
    STATE_RUNNING_RESULT = 9,       //运动结束结果状态，包括三个子状态（异常结束、结果A页，结果B页）
}running_state_t;

typedef enum {
    SUB_STATE_END_INIT, 
    SUB_STATE_END_A,
    SUB_STATE_END_B,    
    SUB_STATE_END_ABNORMAL,        
}sub_state_end_t;

/**
 * @brief Definitaion for a sport describe Funcion Type
 */
typedef void ( *setup_fn_t )(u32_t new_session); 
typedef void ( *pre_proc_fn_t )(void); 
typedef void ( *update_para_fn_t )(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart);
typedef void ( *update_disp_fn_t )(u32_t flush, ui_disp_data_t* pdata); 
typedef void ( *pause_fn_t )(void); 
typedef void ( *resume_fn_t )(void); 
typedef void ( *terminate_fn_t )(u32_t from_doing); 
typedef u32_t ( *result_pre_proc_fn_t )(u32_t duration_second, u32_t delta_step); 
typedef void ( *result_disp_fn_t )(u8_t screen_num, ui_disp_data_t* pdata); 
typedef u32_t ( *onGPS_handler_t )(ry_ipc_msg_t* msg); 
typedef u32_t ( *milestone_alert_t )(u32_t duration_second, float distance_now, milestone_data_t* pmile); 


/*
 * VARIABLES
 *******************************************************************************
 */



/*
 * FUNCTIONS
 *******************************************************************************
 */
ry_sts_t running_info_onCreate(void * usrdata);
ry_sts_t running_info_onDestroy(void);

void sport_common_ready_go_display_update(int ready_count);
void clear_last_sports_mode_and_set_new_mode(int mode);
uint32_t hrm_ave_calc(uint8_t hrm_new);
int sport_common_get_distance_by_step(float* p_distance, u32_t* p_step_distanceStart);
void sport_common_pre_proc(void);
u32_t sport_common_onGPS_handler(ry_ipc_msg_t* msg); 

float get_sport_distance(void);

/*
 * ------------------------------------------------------------------------------------------------------
 * outdoor running
 */
void outdoor_running_setup(u32_t new_session);
void outdoor_running_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart);
void outdoor_runing_info_display_update(uint32_t flush, ui_disp_data_t* pdata);
uint32_t outdoor_running_result_pre_proc(u32_t duration_second, u32_t delta_step);
void outdoor_running_result_disp(u8_t screen_num, ui_disp_data_t* pdata);
void outdoor_running_pause(void);
void outdoor_running_resume(void);
void outdoor_running_terminate(u32_t from_doing);
u32_t outdoor_running_milestone_alert(u32_t duration_second, float distance_now, milestone_data_t* pmile);

/*
 * ------------------------------------------------------------------------------------------------------
 * indoor running
 */
void indoor_running_setup(u32_t new_session);
void indoor_running_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart);
void indoor_running_terminate(u32_t from_doing);
uint32_t indoor_running_result_pre_proc(u32_t duration_second, u32_t delta_step);
void indoor_running_pause(void);
void indoor_running_resume(void);


/*
 * ------------------------------------------------------------------------------------------------------
 * outdoor bike
 */
void outdoor_bike_setup(u32_t new_session);
void outdoor_bike_info_display_update(uint32_t flush, ui_disp_data_t* pdata);
void outdoor_bike_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart);
uint32_t outdoor_bike_result_pre_proc(u32_t duration_second, u32_t delta_step);

/*
 * ------------------------------------------------------------------------------------------------------
 * free training
 */
 void activity_free_setup(u32_t new_session);
 void free_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart);
 uint32_t free_result_pre_proc(u32_t duration_second, u32_t delta_step);
 void free_pause(void);
 void free_resume(void);
 void free_terminate(u32_t from_doing);
 
 
#endif  /* __ACTIVITY_RUNNING_H__ */


