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
#include "sensor_alg.h"
#include "board_control.h"
#include "activity_running.h"


/*********************************************************************
 * CONSTANTS
 */   
#define SCROLLBAR_HEIGHT                40
#define SCROLLBAR_WIDTH                 1


/*
 * TYPES
 *******************************************************************************
 */


/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */


void outdoor_bike_info_display(ry_time_t cur_time, u32_t duration_second, float distance, u8_t hrm_cnt)
{
    char * info_str = (char *)ry_malloc(40);
    clear_buffer_black();

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void *)font_roboto_20.font, Grey, Black|(1<<30));
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 30, info_str, (void *)font_roboto_32.font, 0x15d185,Black|(1<<30));

    //speed title
    gdispFillString(3, 58, "Скорость", (void *)font_roboto_20.font, 0xc0c0c0, Black|(1<<30));

    //speed info
    u8_t offset = 0;
    u8_t offset_y = 3;
    {
        int second_per_km = 0;
        float km_per_hour = 0.1f;
        if (distance > 0.1f){
            km_per_hour = (distance * 3600)/ (duration_second * 1000);
            if ((duration_second != 0) && (km_per_hour > 0.01f)){
                sprintf(info_str, "%.1f", km_per_hour);
            }                
            else{
                sprintf(info_str, "0");
            }
        }
        else{
            sprintf(info_str, "--");
        }
        // LOG_DEBUG("duration_second: %d, distance: %f, km_per_hour: %f\n", duration_second, distance, km_per_hour);
    }

    gdispFillString(3, 84 + offset_y, info_str, (void *)font_roboto_32.font, 0xf09049, Black|(1<<30));

    //distance title
    gdispFillString(3, 115 + offset_y, "Расстояние", (void *)font_roboto_20.font, White, Black|(1<<30));

    //distance info
    if (distance < 0.1f) {
        sprintf(info_str, "%d", (int)distance);
    } 
    else {
        sprintf(info_str, "%.1f", distance/1000);
    }
    gdispFillString(3, 144 + offset_y, info_str, (void *)font_roboto_32.font, 0xC2E350, Black|(1<<30));
    
    int dist_width = getStringWidthInnerFont(info_str, font_roboto_32.font);
    gdispFillString(8 + dist_width, 144 + offset_y, "км", (void*)font_roboto_20.font, 0xC2E350, Black);    

    //hrm title
    gdispFillString(3, 180 + offset_y, "BPM", (void *)font_roboto_20.font, White, Black|(1<<30));

    //hrm count    
    if (duration_second < 8){
        sprintf(info_str, "-");
    }
    else if(hrm_cnt != 0){
        sprintf(info_str, "%d", hrm_cnt);
    }
    else{
        sprintf(info_str, "-");
    }
    
    user_data_birth_t* birth = get_user_birth();
    uint32_t age = cur_time.year + 2000 - birth->year;
    u8_t hrm_level = get_sports_hrm_level(s_alg.hr_cnt, age);
    
    color_t cl = ((hrm_level_color[hrm_level] & 0xff) << 16) \
                | ((hrm_level_color[hrm_level] & 0xff0000) >> 16)\
                | ((hrm_level_color[hrm_level] & 0x00ff00) >> 0);
    
    gdispFillString(RM_OLED_WIDTH - 50, 182, info_str, (void *)font_roboto_32.font, cl, Black|(1<<30));
    
    //hrm status text
    if (duration_second >= 8){
        sprintf(info_str, "%s", hrm_level_str[hrm_level]);
        offset = (hrm_level == 0) ? 22 : 0;
        gdispFillString(4, 206 + offset_y, info_str, (void *)font_roboto_20.font, hrm_level_color[hrm_level], Black|(1<<30));
    }
    ry_free(info_str);
}

void outdoor_bike_info_display_update(uint32_t flush, ui_disp_data_t* pdata)
{
    //time now
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time);

    if(pdata->hrm != 0){
        hrm_ave_calc(pdata->hrm);
    }

    outdoor_bike_info_display(cur_time, pdata->duration_second, pdata->distance, pdata->hrm);
    if (flush){
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
}


void outdoor_bike_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart)
{
    extern LocationResult dev_locat;
    static int tick_step_distance;

    //data access and report
    if ((state == STATE_RUNNING_DOING) || \
        (state == STATE_ONE_KM_ALERT) || \
        (state == STATE_RUNNING_BREAK)){
        if (count % GET_LOCATION_TIME == 0){
            update_ride_param();
        }
//#if 1
        if (gps_location_succ_get()){
            if (*p_distance < dev_locat.last_distance){
                *p_distance = dev_locat.last_distance;        //update distance from gps
                *p_step_distanceStart = alg_get_step_today();
                tick_step_distance = 0;
            }
        }
//        else if (tick_step_distance++ >= 9){                //update distance from step cacluation
//            if (sport_common_get_distance_by_step(p_distance, p_step_distanceStart) != 0){
//                set_ride_point(*p_distance);                 //save distance point
//            }
//            tick_step_distance = 0;
//        }
//#else
//        //debug data to display and report
//        *p_distance += 10.0f;
//        if (tick_step_distance++ >= 9){
//            set_ride_point(*p_distance);                 //save distance point
//            tick_step_distance = 0;
//        }
//#endif      

    //access gps status
    if ((state == STATE_ACCESS_GPS) || \
        (state == STATE_ACCESS_GPS_SKIP) || \
        (state == STATE_SHOW_COUNT)){
        if (count % GET_LOCATION_TIME_COLD_START == 0){
            update_ride_param();
        }
    }
    
    // LOG_DEBUG("[outdoor_bike_update_para]: distance:%.2f, step_distanceStart:%d\r\n", \
        *p_distance, *p_step_distanceStart);
    }
}

uint32_t outdoor_bike_result_pre_proc(u32_t duration_second, u32_t delta_step)
{
    u32_t ret = 0;

    if (sport_common_abnormal_result_processing((duration_second > 120), 1)){
        set_current_running_disable();
        ret = 1;     // do not display running result.
    }
    return ret;
}

void outdoor_bike_pause(void)
{
    pause_record_ride();
//    DEV_STATISTICS(sport_pause);
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);  
}


void outdoor_bike_resume(void)
{
    resume_record_ride();                                        
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START, NULL);
}


void outdoor_bike_terminate(u32_t from_doing)
{
//    DEV_STATISTICS(sport_stop);
//    DEV_STATISTICS(sport_finish);        
    clear_running();
    if (from_doing != 0){
	    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);
    }
    alg_msg_transfer(ALG_MSG_GENERAL);
    stop_record_ride();    
}
