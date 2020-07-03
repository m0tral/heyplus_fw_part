#include <stdio.h>
#include "ry_font.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "led.h"
#include "am_devices_cy8c4014.h"
#include "board_control.h"

#include "gui_img.h"
#include "ryos_timer.h"
#include "gui_msg.h"
#include "data_management_service.h"
#include "device_management_service.h"
#include "card_management_service.h"
#include "window_management_service.h"
#include "timer_management_service.h"
#include "ry_ble.h"
#include "sensor_alg.h"
#include "app_pm.h"
#include "hrm.h"
#include "activity_card.h"
#include "algorithm.h"
#include "motar.h"
#include "activity_sports.h"
#include "ry_statistics.h"
#include "scrollbar.h"
#include "icon_1km.h"
#include "sensor_alg.h"
#include "board_control.h"
#include "activity_running.h"

/*********************************************************************
 * Local function include
 */ 

/*********************************************************************
 * CONSTANTS
 */    
#define RUNNING_INFO_SCREEN_TIME_MAX    10
#define RUNNING_INFO_SCREEN_TIME_MIDDLE 5

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    u8_t         sport_now;    
    u8_t         state;   
    u8_t         sub_state_end;    
    u8_t         is_new_session;

    u8_t         ready_count;
    u8_t         gps_count;

    u32_t        duration_second;
    u32_t        count;
    
    u32_t        start_step;
    u32_t        step_distanceStart;    
    
    u32_t        hrm_all;
    u32_t        hrm_cnt;
    u32_t        hrm_ave;  
    u32_t        screen_off_cnt;
    u32_t        start_time;
    float        distance;
    float        step_distance;        
    
    u8_t         gps_changed;
    u8_t         screen_is_off;
    u8_t         milestone_flg;  

} running_ctx_t;

/**
 * @brief Definitaion for sport ui describe type
 */
typedef struct {
    u8_t                id;                 //当前运动的ID
    u8_t                gps_en;             //是否需要GPS信号
    u8_t                hrm_en;             //是否需要开启心率（游泳不需要）
    setup_fn_t          setup;              //运动进入后，活动类型保存，初始化参数，初始页面展示等
    pre_proc_fn_t       pre_proc;           //倒计时动画后，运动正式启动前的预处理
    update_para_fn_t    update_para;        //运动中的参数刷新，数据存储，手机参数请求
    update_disp_fn_t    update_disp;        //运行中的屏幕刷新
    onGPS_handler_t     onGPS_handler;      //GPS信号监听处理
    milestone_alert_t   milestone_alert;    //目标达成判断，显示
    result_pre_proc_fn_t result_pre_proc;   //异常结束判断
    result_disp_fn_t    result_disp;        //结果显示
    pause_fn_t          pause;              //暂停处理，主要是数据保存及app请求
    resume_fn_t         resume;             //从暂停中恢复，主要是恢复数据及app请求
    terminate_fn_t      terminate;          //运动结束（用户点击结束键，结束session的配置）
} sport_describe_t;

/*********************************************************************
 * VARIABLES
 */
const sport_describe_t sport_desc_array[] = {
    //outdoor_running
    {
        SPORT_OUTDOOR_RUNNING, 
        ENABLE, 
        ENABLE, 
        outdoor_running_setup, 
        sport_common_pre_proc,
        outdoor_running_update_para,
        outdoor_runing_info_display_update, 
        sport_common_onGPS_handler,
        outdoor_running_milestone_alert,
        outdoor_running_result_pre_proc,
        outdoor_running_result_disp,
        outdoor_running_pause,
        outdoor_running_resume,
        outdoor_running_terminate,
    },

    //indoor_running
    {
        SPORT_INDOOR_RUNNING, 
        DISABLE, 
        ENABLE, 
        indoor_running_setup, 
        sport_common_pre_proc,
        indoor_running_update_para,
        outdoor_runing_info_display_update, 
        NULL,
        outdoor_running_milestone_alert,
        indoor_running_result_pre_proc,
        outdoor_running_result_disp,
        indoor_running_pause,
        indoor_running_resume,
        indoor_running_terminate,
    },

    //outdoor_bike
    {
        SPORT_OUTDOOR_BIKE, 
        ENABLE, 
        ENABLE, 
        outdoor_bike_setup, 
        sport_common_pre_proc,
        outdoor_bike_update_para,
        outdoor_bike_info_display_update, 
        sport_common_onGPS_handler,
        outdoor_running_milestone_alert,
        outdoor_bike_result_pre_proc,
        outdoor_running_result_disp,
        outdoor_bike_pause,
        outdoor_bike_resume,
        outdoor_bike_terminate,
    },
    
    // free training
    {
        SPORT_FREETRAINNING, 
        DISABLE, 
        ENABLE, 
        activity_free_setup, 
        sport_common_pre_proc,
        free_update_para,
        free_info_display_update, 
        NULL,
        NULL,
        free_result_pre_proc,
        free_result_disp,
        free_pause,
        free_resume,
        free_terminate,
    },
};

activity_t activity_running_info = {
    .name = "running_info",
    .onCreate = running_info_onCreate,
    .onDestroy = running_info_onDestroy,
    .disableUntouchTimer = 1,
    .priority = WM_PRIORITY_M,
};

running_ctx_t running_ctx_v;
u32_t running_info_timer_id;
ui_disp_data_t disp_v;
milestone_data_t milestone_v;


static inline void ui_disp_data_prepare(void)
{
    disp_v.distance = running_ctx_v.distance;
    disp_v.duration_second = running_ctx_v.duration_second;
    disp_v.hrm = s_alg.hr_cnt;
    disp_v.hrm_ave = running_ctx_v.hrm_ave;
    disp_v.delta_step = alg_get_step_today() - running_ctx_v.start_step; 
}

uint32_t hrm_ave_calc(uint8_t hrm_new)
{
    running_ctx_v.hrm_all += hrm_new;
    running_ctx_v.hrm_cnt ++;
    running_ctx_v.hrm_ave = running_ctx_v.hrm_all / running_ctx_v.hrm_cnt;
    // LOG_DEBUG("running_ctx_v.hrm_all: %d, running_ctx_v.hrm_cnt: %d, running_ctx_v.hrm_ave: %d\n", \
        running_ctx_v.hrm_all, running_ctx_v.hrm_cnt, running_ctx_v.hrm_ave);
    return running_ctx_v.hrm_ave;
}


int sport_common_get_distance_by_step(float* p_distance, u32_t* p_step_distanceStart)
{
    int ret = 0;
    float distance_update = 0.0f;
    int step_delta = alg_get_step_today() - *p_step_distanceStart;
    int stepFreqAve = 0;

    if (step_delta > 0){
        stepFreqAve = step_freq_ave_get();
        distance_update = alg_get_step_distance_on_stepFreq(step_delta, stepFreqAve);
        *p_step_distanceStart = alg_get_step_today();
        step_freq_ave_reset();
    }
    running_ctx_v.step_distance += distance_update;
    if (*p_distance < running_ctx_v.step_distance){
        *p_distance = running_ctx_v.step_distance;
        ret = 1;
    }
    // LOG_DEBUG("[sport_common_get_distance_by_step]: distance:%.2f, step_distanceStart:%d, step_distance:%.2f\r\n", \
        *p_distance, *p_step_distanceStart, running_ctx_v.step_distance);
    return ret;
}

uint32_t got_gps_changed_status(void)
{
    return running_ctx_v.gps_changed;
}

static inline void reset_gps_changed_status(void)
{
    running_ctx_v.gps_changed = 0;
}


void running_refresh_ui_onOff_status_clear(void)
{
    running_ctx_v.screen_is_off = 0;
	running_ctx_v.screen_off_cnt = 0;
}

void running_info_timeout_handler(void* arg)
{
    u32_t msg = 0;
    
    if (running_info_timer_id) {
        ry_timer_start(running_info_timer_id, 1000, running_info_timeout_handler, NULL);
        if(strcmp(wms_activity_get_cur_name(), activity_running_info.name) == 0){
            ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);
        }
    }
} 

int running_info_onMSGEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    return consumed; 
}

int running_info_onBLEEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;

    if(msg->msgType == IPC_MSG_TYPE_BLE_CONNECTED){
        ry_data_msg_send(DATA_SERVICE_MSG_GET_LOCATION, 0, NULL);
    }
    
    return consumed; 
}

int running_info_onALGEvent(ry_ipc_msg_t* msg)
{
    wrist_filt_t* p = (wrist_filt_t*)msg->data;
    if (p->type == ALG_WRIST_FILT_UP) {
        running_refresh_ui_onOff_status_clear();
        LOG_DEBUG("[running_info_onALGEvent]:running_refresh_ui_onOff_status_clear\r\n");        
    } 
    return 0;
}

int running_doing_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            break;

        case TP_PROCESSED_ACTION_TAP:
            if ((p->y >= 8)){
                running_ctx_v.sub_state_end = SUB_STATE_END_INIT;
                running_ctx_v.state = STATE_RUNNING_BREAK;
                sport_common_break_option_display();
            }
            break;

        default:
            break;
    }
    return consumed;
}


int running_alert_gps_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            break;

        case TP_PROCESSED_ACTION_TAP:
            if ((p->y >= 3)){
                if ((running_ctx_v.screen_is_off == 0) && got_gps_changed_status()){      // stop gps alert and lock break funciton
                    motar_stop();
                    reset_gps_changed_status();
                    running_ctx_v.screen_off_cnt = 0; //count screen on from the beginning
                    running_ctx_v.state = STATE_RUNNING_DOING;
                    ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);  //let running_onSportServiceEvent refresh UI
                }
                return consumed;
            }
            break;

        default:
            break;
    }
    return consumed;
}


int running_alert_1km_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;

    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            break;

        case TP_PROCESSED_ACTION_TAP:
            if ((p->y >= 8)){
                motar_stop();
                running_ctx_v.milestone_flg = 0;
                running_ctx_v.screen_off_cnt = 0; //count screen on from the beginning
                running_ctx_v.state = STATE_RUNNING_DOING;
                ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);  //let running_onSportServiceEvent refresh UI
                return consumed;
            }
            break;

        default:
            break;
    }
    return consumed;
}

int running_ready_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y < 8){
                if(running_ctx_v.state == STATE_ACCESS_GPS_SKIP){
                    if (p->y > 4){
                        running_ctx_v.state = STATE_SHOW_COUNT;
                    }
                }
                else if(running_ctx_v.state == STATE_ACCESS_GPS){
                    if (p->y > 4){
                        running_ctx_v.state = STATE_ACCESS_GPS_SKIP;
                        sport_gps_skip_display_update(1);
                    }
                }
            }
            else if ((p->y >= 8)){
                sport_desc_array[running_ctx_v.sport_now].terminate(0);
                wms_activity_back(NULL);
                return consumed;
            }
            break;

        default:
            break;
    }
    return consumed;
}


int running_break_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y <= 3){
                running_ctx_v.state = STATE_RUNNING_PAUSING;                // pause ativity
                sport_desc_array[running_ctx_v.sport_now].pause();
                sport_common_pausing_display();                
            }
            else if ((p->y > 3) && (p->y <= 7)){                            // end of activity
                u32_t delta_step = alg_get_step_today() - running_ctx_v.start_step;
                u32_t result_abnormal = sport_desc_array[running_ctx_v.sport_now].result_pre_proc(running_ctx_v.duration_second, delta_step);
                if (result_abnormal != 0){
                    running_ctx_v.sub_state_end = SUB_STATE_END_ABNORMAL;
                }
                else{
                    running_ctx_v.sub_state_end = SUB_STATE_END_A; 
                    ui_disp_data_prepare();
                    sport_desc_array[running_ctx_v.sport_now].result_disp(0, &disp_v);                    
                }
                sport_desc_array[running_ctx_v.sport_now].terminate(1);
                running_ctx_v.state = STATE_RUNNING_RESULT;
            }
            else if ((p->y >= 8)){
                running_ctx_v.state = STATE_RUNNING_DOING;                  // back to running doing
                ui_disp_data_prepare();
                sport_desc_array[running_ctx_v.sport_now].update_disp(1, &disp_v);
            }
            break;
        
        default:
            break;
    }
exit:
    return consumed;
}


int running_pausing_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            break;

        case TP_PROCESSED_ACTION_TAP:
           if ((p->y > 3)){
                running_ctx_v.state = STATE_RUNNING_DOING;
                running_ctx_v.start_time = ryos_rtc_now() - running_ctx_v.duration_second;      //折算开始时间 = now - 持续时间
                sport_desc_array[running_ctx_v.sport_now].resume();    
                ui_disp_data_prepare();
                sport_desc_array[running_ctx_v.sport_now].update_disp(1, &disp_v);
                goto exit;
            }
            break;
        
        default:
            break;
    }
exit:
    return consumed;
}


int running_result_onTouchEvent(tp_processed_event_t *p)
{
    int consumed = 1;
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            if(running_ctx_v.sub_state_end == SUB_STATE_END_B){
                running_ctx_v.sub_state_end = SUB_STATE_END_A;  
                ui_disp_data_prepare();
                sport_desc_array[running_ctx_v.sport_now].result_disp(0, &disp_v);                    
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            if(running_ctx_v.sub_state_end == SUB_STATE_END_A){
                running_ctx_v.sub_state_end = SUB_STATE_END_B;   
                ui_disp_data_prepare() ;
                sport_desc_array[running_ctx_v.sport_now].result_disp(1, &disp_v);                  
            }
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y <= 3){
                
            }
            else if ((p->y > 3) && (p->y <= 7)){
                if ((running_ctx_v.sub_state_end == SUB_STATE_END_B)||\
                         (running_ctx_v.sub_state_end == SUB_STATE_END_ABNORMAL)){
                    wms_activity_back(NULL);
                    goto exit;
                }
            }
            else if ((p->y >= 8)){
                wms_activity_back(NULL);
                goto exit;
            }
            break;
        
        default:
            break;
    }
exit:
    return consumed;
}


int running_info_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;

    switch (running_ctx_v.state){
        case STATE_ACCESS_GPS:
        case STATE_ACCESS_GPS_SKIP: 
        case STATE_GPS_RESULT:
        case STATE_SHOW_COUNT:
            running_ctx_v.screen_off_cnt = 0;   //count screen on from the beginning            
            consumed = running_ready_onTouchEvent(p);
            break;

        case STATE_RUNNING_DOING:
            running_ctx_v.screen_off_cnt = 0;   //count screen on from the beginning             
            consumed = running_doing_onTouchEvent(p);            
            break;

        case STATE_GPS_CHANGED: 
            running_alert_gps_onTouchEvent(p);
            break;

        case STATE_ONE_KM_ALERT:
            consumed = running_alert_1km_onTouchEvent(p);
            break;

        case STATE_RUNNING_BREAK:
            running_ctx_v.screen_off_cnt = 0;   //count screen on from the beginning             
            consumed = running_break_onTouchEvent(p);
            break;

        case STATE_RUNNING_PAUSING:
            running_ctx_v.screen_off_cnt = 0;   //count screen on from the beginning            
            consumed = running_pausing_onTouchEvent(p);
            break;

        case STATE_RUNNING_RESULT:
            running_ctx_v.screen_off_cnt = 0;   //count screen on from the beginning             
            running_result_onTouchEvent(p);
            break;

        default:
            break;
    }

exit: 
    running_ctx_v.screen_is_off = 0;
    DEV_STATISTICS(sport_click_count);
    return consumed;
}

static inline void running_data_init(void)
{
    //get and reset start_of_running_sport_data            
    running_ctx_v.hrm_cnt = 0;
    running_ctx_v.hrm_all = 0;
    running_ctx_v.hrm_ave = 0;
    running_ctx_v.screen_off_cnt = 0;
    running_ctx_v.screen_is_off = 0;
    running_ctx_v.start_step = alg_get_step_today();
    running_ctx_v.step_distanceStart = alg_get_step_today();
    running_ctx_v.distance = 0.0;
    running_ctx_v.step_distance = 0.0f;
    running_ctx_v.duration_second = 0;
    running_ctx_v.start_time = ryos_rtc_now();
    running_ctx_v.milestone_flg = 0;
    running_ctx_v.gps_changed = 0;

    milestone_v.distance_1km_start = 0.0;
    milestone_v.start_durationSec_1km = 0;
    milestone_v.all_km = 0;
    milestone_v.start_step_1km = alg_get_step_today();
    
    step_freq_ave_reset();
}

void clear_last_sports_mode_and_set_new_mode(int mode)
{
    u32_t * new_status = (u32_t *)ry_malloc(sizeof(u32_t));
    *new_status = mode;
    ry_data_msg_send(DATA_SERVICE_BODY_STATUS_CHANGE, sizeof(u32_t), (u8_t *)new_status);
    ry_free(new_status);       
}

int running_onSportServiceEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;    
    static u8_t last_gui_st = GUI_STATE_OFF;
    u32_t gui_flush = 0;

    //duation_time refresh
    running_ctx_v.duration_second = ryos_rtc_now() - running_ctx_v.start_time;   
    if (running_ctx_v.duration_second > 48 * 3600){
        LOG_ERROR("[running]duration_second:%d, ryos_rtc_now:%d, startT:%d\r\n", \
            running_ctx_v.duration_second, ryos_rtc_now(), running_ctx_v.start_time);
        running_ctx_v.duration_second = 0;
        running_ctx_v.start_time = ryos_rtc_now();
    } 

    //screen on-off control
    if ((running_ctx_v.screen_is_off == 1) && (GUI_STATE_ON != last_gui_st) && (GUI_STATE_ON == get_gui_state())){
        running_ctx_v.screen_is_off = 0;
        running_ctx_v.screen_off_cnt = 0;
    }
    if (++running_ctx_v.screen_off_cnt > RUNNING_INFO_SCREEN_TIME_MAX){
        if ((running_ctx_v.screen_is_off == 0) && (running_ctx_v.state == STATE_RUNNING_DOING)){
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
            running_ctx_v.screen_is_off = 1;
        }
    }
    gui_flush = !running_ctx_v.screen_is_off;

    //state processing and display
    switch (running_ctx_v.state){
        case STATE_ACCESS_GPS:
        case STATE_ACCESS_GPS_SKIP: 
            if (running_ctx_v.gps_count <= SPORTS_ACCESS_GPS_OUT_TIME){
                running_ctx_v.state = STATE_GPS_RESULT;            
            }
            else if (running_ctx_v.gps_count < SPORTS_ACCESS_GPS_TIME){
                if (gps_location_succ_get()){
                    running_ctx_v.state = STATE_GPS_RESULT;
                    running_ctx_v.gps_count = 2;
                }
            }   
            if (running_ctx_v.gps_count > 0){
                running_ctx_v.gps_count --; 
            }
            break;

        case STATE_GPS_RESULT:
            sport_gps_acess_result_display_update(gps_location_succ_get());
            if (running_ctx_v.gps_count > SPORTS_ACCESS_GPS_OUT_TIME){
                running_ctx_v.gps_count = SPORTS_ACCESS_GPS_OUT_TIME;
            }
            if (running_ctx_v.gps_count == 0){
                running_ctx_v.state = STATE_SHOW_COUNT;            
            }
            if (running_ctx_v.gps_count > 0){
                running_ctx_v.gps_count --; 
            }
            break;

        case STATE_SHOW_COUNT:
            if (running_ctx_v.ready_count > 0){
                sport_common_ready_go_display_update(running_ctx_v.ready_count - 1);
            }
            else{
                motar_strong_linear_working(200);
                sport_desc_array[running_ctx_v.sport_now].pre_proc();
                running_ctx_v.state = STATE_RUNNING_DOING;            
            }
            running_ctx_v.ready_count--;        
            break;

        case STATE_RUNNING_DOING: 
            if (sport_desc_array[running_ctx_v.sport_now].onGPS_handler != NULL){
                u32_t gps_changed = sport_desc_array[running_ctx_v.sport_now].onGPS_handler(msg);
                if (gps_changed != 0){
                    running_ctx_v.gps_changed = gps_changed;
                    running_ctx_v.screen_off_cnt = RUNNING_INFO_SCREEN_TIME_MIDDLE;
                    running_ctx_v.screen_is_off = 0;
                    running_ctx_v.state = STATE_GPS_CHANGED;                            
                    LOG_DEBUG("[onSportServiceEvent]msg_get:%d, msg->len:%d\r\n", *msg->data, msg->len);
                }
            }

            if (sport_desc_array[running_ctx_v.sport_now].milestone_alert != NULL){
                if (sport_desc_array[running_ctx_v.sport_now].milestone_alert(running_ctx_v.duration_second,\
                                                                              running_ctx_v.distance,\
                                                                              &milestone_v)\
                                                                              != 0){
                    running_ctx_v.milestone_flg = 1;
                    running_ctx_v.screen_off_cnt = RUNNING_INFO_SCREEN_TIME_MIDDLE;
                    running_ctx_v.screen_is_off = 0;
                    running_ctx_v.state = STATE_ONE_KM_ALERT;            
                }
            }

            if (running_ctx_v.state == STATE_RUNNING_DOING){
                ui_disp_data_prepare();
                sport_desc_array[running_ctx_v.sport_now].update_disp(gui_flush, &disp_v);
            }
            break;

        case STATE_ONE_KM_ALERT: 
            if (running_ctx_v.screen_off_cnt > RUNNING_INFO_SCREEN_TIME_MAX){
                running_ctx_v.milestone_flg = 0;
                running_ctx_v.state = STATE_RUNNING_DOING;  
            }
            break;

        case STATE_GPS_CHANGED: 
            if (running_ctx_v.screen_off_cnt > RUNNING_INFO_SCREEN_TIME_MAX){
                reset_gps_changed_status();
                running_ctx_v.state = STATE_RUNNING_DOING;            
            }
            break;

        case STATE_RUNNING_BREAK: 
        case STATE_RUNNING_PAUSING:
            //do not period flush in this mode, waiting button operation         
            break;
            
        case STATE_RUNNING_RESULT: 
            //do not period flush in this mode, waiting button operation         
            break;

        default:
            break;
    }

    if (sport_desc_array[running_ctx_v.sport_now].update_para != NULL) {
        if (sport_desc_array[running_ctx_v.sport_now].id == SPORT_FREETRAINNING){
            sport_desc_array[running_ctx_v.sport_now].update_para(running_ctx_v.state, \
                                                                  running_ctx_v.count, \
                                                                  0,
                                                                  &running_ctx_v.duration_second\
                                                                  );
        }
        else {
            sport_desc_array[running_ctx_v.sport_now].update_para(running_ctx_v.state, \
                                                                  running_ctx_v.count, \
                                                                  &running_ctx_v.distance,
                                                                  &running_ctx_v.step_distanceStart\
                                                                  );
        }
    }

    running_ctx_v.count ++;
    // LOG_DEBUG("[onSportServiceEvent]state:%d, screen_is_off:%d, screen_off_cnt:%d\r\n", \
                running_ctx_v.state, running_ctx_v.screen_is_off, running_ctx_v.screen_off_cnt);
    
    return consumed;
}


ry_sts_t running_info_onCreate(void * usrdata)
{
    ry_sts_t ret = RY_SUCC;

    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_running_info, running_info_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_running_info, running_info_onTouchEvent);
    wms_event_listener_add(WM_EVENT_BLE,     &activity_running_info, running_info_onBLEEvent);
    wms_event_listener_add(WM_EVENT_ALG,     &activity_running_info, running_info_onALGEvent);
    wms_event_listener_add(WM_EVENT_SPORTS,  &activity_running_info, running_onSportServiceEvent);

    if(charge_cable_is_input()){
        sport_desc_array[running_ctx_v.sport_now].terminate(1);
        wms_activity_back(NULL);
        return ret;
    }
	else if (usrdata != NULL){
		running_ctx_v.sport_now = (u32_t )usrdata - SPORT_ITEM_BASE;
        running_ctx_v.is_new_session = 1;        
	}
    else{
        running_ctx_v.is_new_session = 0;
    }

    if (running_ctx_v.sport_now >= SPORTS_ITEM_NUM){
        LOG_ERROR("[running_info_onCreate]sport_now:%d\r\n",  running_ctx_v.sport_now);
        wms_activity_back(NULL);
        ret = RY_ERR_INVALIC_PARA;
        return ret;
    }
    
    if (running_info_timer_id == 0) {
        ry_timer_parm timer_para;
        timer_para.timeout_CB = running_info_timeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "running_info_timer";
        running_info_timer_id = ry_timer_create(&timer_para);
	}
    ry_timer_start(running_info_timer_id, 1000, running_info_timeout_handler, NULL);
    
    LOG_DEBUG("[running_info_onCreate] running_ctx_v.sport_now:%d, is_new_session: %d\r\n", \
            running_ctx_v.sport_now, running_ctx_v.is_new_session);
    
    running_ctx_v.start_time = ryos_rtc_now(); //this time should be overwrite when do data_init, but is must
    sport_desc_array[running_ctx_v.sport_now].setup(running_ctx_v.is_new_session);
    return ret;
}


ry_sts_t running_info_onDestroy(void)
{
	wms_event_listener_del(WM_EVENT_MESSAGE, &activity_running_info);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_running_info);
    wms_event_listener_del(WM_EVENT_ALG,     &activity_running_info);
    wms_event_listener_del(WM_EVENT_SPORTS,  &activity_running_info);
    wms_event_listener_del(WM_EVENT_BLE,     &activity_running_info);

    if (running_info_timer_id) {
        ry_timer_stop(running_info_timer_id);
        ry_timer_delete(running_info_timer_id);
        running_info_timer_id = 0;
    }

    if((charge_cable_is_input())){
        sport_desc_array[running_ctx_v.sport_now].terminate(1);
    }
    
    return RY_SUCC;
}

void sport_common_pre_proc(void)
{
    running_data_init();
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START, NULL);
}

u32_t sport_common_onGPS_handler(ry_ipc_msg_t* msg)
{
    u32_t gps_changed = 0;
    if ((msg->len == 1) && ((*msg->data == GPS_LOC_CHANGED_STATUS_RECOVERY) || (*msg->data == GPS_LOC_CHANGED_STATUS_LOST))){
        gps_changed = *msg->data;
        motar_light_loop_working(2);
        uint32_t gps_st = (gps_changed == GPS_LOC_CHANGED_STATUS_RECOVERY) ? 1 : 0;
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
        sport_gps_running_result_display_update(gps_st);
    }
    return gps_changed;
}


void outdoor_running_setup(u32_t new_session)
{
    if (new_session){
        running_ctx_v.state = STATE_ACCESS_GPS;
        running_ctx_v.ready_count = 3;
        running_ctx_v.gps_count = SPORTS_ACCESS_GPS_TIME;

        clear_last_sports_mode_and_set_new_mode(SPORT_RUN_MODE);
        clear_running();                        //if(get_device_session() != DEV_SESSION_UPLOAD_DATA) ?
        
        //set new running session and start to record
        if(get_sport_cur_session_id()){
            ry_data_msg_send(DATA_SERVICE_MSG_START_RUN, 0, NULL);
        }
        else{
            ry_data_msg_send(DATA_SERVICE_MSG_GET_LOCATION, 0, NULL);
        }
        set_running();
        
        start_record_run();                     //send rbp running start
        sport_gps_info_display_update(1);    
    }
    else{
        ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);  //let running_onSportServiceEvent refresh UI
    }
}

void outdoor_bike_setup(u32_t new_session)
{
    if (new_session){
        running_ctx_v.state = STATE_ACCESS_GPS;
        running_ctx_v.ready_count = 3;
        running_ctx_v.gps_count = SPORTS_ACCESS_GPS_TIME;

        clear_last_sports_mode_and_set_new_mode(SPORT_RIDE_MODE);
        clear_running();                        //if(get_device_session() != DEV_SESSION_UPLOAD_DATA) ?
        
        //set new running session and start to record
        if(get_sport_cur_session_id()){
            ry_data_msg_send(DATA_SERVICE_MSG_START_RUN, 0, NULL);
        }
        else{
            ry_data_msg_send(DATA_SERVICE_MSG_GET_LOCATION, 0, NULL);
        }
        set_running();
        
        start_record_ride();                     //send rbp running start
        sport_gps_info_display_update(1);    
    }
    else{
        ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);  //let running_onSportServiceEvent refresh UI
    }
}

void indoor_running_setup(u32_t new_session)
{
    if (new_session){
        running_ctx_v.state = STATE_SHOW_COUNT;
        running_ctx_v.ready_count = 3;
        running_ctx_v.gps_count = 0;

        clear_last_sports_mode_and_set_new_mode(SPORT_IN_DOOR_RUN_MODE);
        clear_running();                        //if(get_device_session() != DEV_SESSION_UPLOAD_DATA) ?
        
        //set new running session and start to record
        // if(get_sport_cur_session_id()){
        //     ry_data_msg_send(DATA_SERVICE_MSG_START_RUN, 0, NULL);
        // }
        // else{
        //     ry_data_msg_send(DATA_SERVICE_MSG_GET_LOCATION, 0, NULL);
        // }

        start_record_indoor_run();
        set_running();
        
        //start_record_run();                     //send rbp running start
    }
    
    ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);  //let running_onSportServiceEvent refresh UI
}

void activity_free_setup(u32_t new_session)
{
    if (new_session){
        running_ctx_v.state = STATE_SHOW_COUNT;
        running_ctx_v.ready_count = 3;
        running_ctx_v.gps_count = 0;

        clear_last_sports_mode_and_set_new_mode(SPORT_FREE);
        clear_running();      

        start_record_indoor_run();
        set_running();
    }
    
    //let running_onSportServiceEvent refresh UI
    ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 0, NULL);
}

float get_sport_distance(void)
{
    return running_ctx_v.distance;
}
