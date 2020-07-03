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
void runing_info_display(ry_time_t cur_time, u32_t duration_second, float distance, u8_t hrm_cnt)
{
    char * info_str = (char *)ry_malloc(40);
    clear_buffer_black();

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void*)font_roboto_20.font, Grey, Black);
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 30, info_str, (void *)font_roboto_32.font, 0x15d185, Black);

    //speed title
    gdispFillString(3, 58, "Скорость", (void*)font_roboto_20.font, 0xc0c0c0, Black);

    //speed info
    u8_t offset = 0;
    u8_t offset_y = 3;
    {
        int second_per_km = 0;
        int km_per_hour = 0;
        if (distance > 0.1f){
            second_per_km = (duration_second * 1000) / distance;
            if (second_per_km < 6000){
                sprintf(info_str, "%02d:%02d", (int)(second_per_km / 60), second_per_km % 60);
            }
            else{
                sprintf(info_str, "-:--");
            }
        }
        else{
            sprintf(info_str, "-:--");
        }
        // LOG_DEBUG("duration_second: %d, distance: %f, second_per_km: %d\n", duration_second, distance, second_per_km);
    }

    gdispFillString(3, 84 + offset_y, info_str, (void *)font_roboto_32.font, 0xf09049, Black);

    //distance title
    gdispFillString(3, 115 + offset_y, "Расстояние", (void*)font_roboto_20.font, White, Black);

    //distance info
    if (distance <= 0.1f) {
        sprintf(info_str, "%d", (uint32_t)distance);
    }
    else {
        sprintf(info_str, "%.1f", distance/1000);
    }
    gdispFillString(3, 144 + offset_y, info_str, (void *)font_roboto_32.font, 0xC2E350, Black);
    
    int dist_width = getStringWidthInnerFont(info_str, font_roboto_32.font);
    gdispFillString(8 + dist_width, 144 + offset_y, "км", (void*)font_roboto_20.font, 0xC2E350, Black);

    //hrm title
    gdispFillString(3, 180 + offset_y, "BPM", (void*)font_roboto_20.font, White, Black);

    //hrm count    
    if (duration_second < 8) {
        sprintf(info_str, "-");
    } else if(hrm_cnt != 0) {
        sprintf(info_str, "%d", hrm_cnt);
    } else {
        sprintf(info_str, "-");
    }
    
    user_data_birth_t* birth = get_user_birth();
    uint32_t age = cur_time.year + 2000 - birth->year;
    u8_t hrm_level = get_sports_hrm_level(s_alg.hr_cnt, age);
    
    color_t cl = ((hrm_level_color[hrm_level] & 0xff) << 16)
                | ((hrm_level_color[hrm_level] & 0xff0000) >> 16)
                | ((hrm_level_color[hrm_level] & 0x00ff00) >> 0);
    
    gdispFillString(RM_OLED_WIDTH - 50, 182, info_str, (void *)font_roboto_32.font, cl, Black);
    
    //hrm status text
    if (duration_second >= 8){
        sprintf(info_str, "%s", hrm_level_str[hrm_level]);
        offset = (hrm_level == 0) ? 22 : 0;
        gdispFillString(4, 206 + offset_y, info_str, (void*)font_roboto_20.font, hrm_level_color[hrm_level], Black);
    }
    ry_free(info_str);
}

const char* training_result[] = {"Расстояние", "км", "Время", "Скорость", "Расход", "ккал", "Сред. BPM", "уд/мин"};

void running_result_display(u8_t screen_num, float distance, u32_t duration_second, u8_t hrm_ave, float calorie)
{
    u8_t image_width, image_height;
    char * str_buf = (char*)ry_malloc(50);
    clear_buffer_black();
    
    if (screen_num == 0) {
        gdispFillString( 0, 0, (char*)training_result[0], (void*)font_roboto_20.font, White, Black);
        
        if (distance < 0.1f)
            sprintf(str_buf, "%d", (uint32_t)distance);
        else
            sprintf(str_buf, "%.1f", ((float)distance)/1000);
        
        coord_t offset_km = gdispGetStringWidth(str_buf, font_roboto_32.font);
        gdispFillString( 1, 30, 
                            str_buf,
                            (void*)font_roboto_32.font,              
                            0xC2E350, Black);

        gdispFillString( offset_km + 4, 35,      
                            (char*)training_result[1],
                            (void*)font_roboto_20.font,               
                            0x50E3C2, Black);
                            
        // time
        gdispFillString( 0, 75,    
                            (char*)training_result[2],        
                            (void*)font_roboto_20.font,               
                            White, Black);

        sprintf(str_buf, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
        gdispFillString( 1, 75 + 25,    
                            str_buf,        
                            (void*)font_roboto_32.font,             
                            0x15d185, Black);
        // speed
        gdispFillString( 1, 140,    
                            (char*)training_result[3],        
                            (void*)font_roboto_20.font,               
                            White, Black);

        int second_per_km = 0;
        if (distance > 0.1f){
            second_per_km = (duration_second * 1000) / distance;
            sprintf(str_buf, "%02d:%02d", (int)(second_per_km / 60), second_per_km % 60);
        }
        else{
            sprintf(str_buf, "--");
        }
        gdispFillString( 1, 140 + 25,
                            str_buf,        
                            (void*)font_roboto_32.font,               
                            0xF09049, Black);

        if(sport_scrollbar != NULL){
            scrollbar_show(sport_scrollbar, frame_buffer, 0);
        }
        
    }
    else if(screen_num == 1) {
        // calories
        gdispFillString( 0, 0,
                            (char*)training_result[4],        
                            (void*)font_roboto_20.font,               
                            White, Black);
        
        sprintf(str_buf, "%d", (int)(calorie*0.001f));
        coord_t offset_pstr = gdispGetStringWidth(str_buf, font_roboto_32.font);

        gdispFillString( 1, 30,
                            str_buf,        
                            (void*)font_roboto_32.font,              
                            0x1CE7F8, Black);

        gdispFillString( offset_pstr + 4, 35,    
                            (char *)training_result[5],        
                            (void*)font_roboto_20.font,              
                            0xF8E71C, Black);
                            
        // avg BPM
        gdispFillString( 0, 75,    
                            (char*)training_result[6],        
                            (void*)font_roboto_20.font,               
                            White, Black);

        sprintf(str_buf, "%d", hrm_ave);
        offset_pstr = gdispGetStringWidth(str_buf, font_roboto_32.font);
                            
        gdispFillString( 1, 75 + 30,    
                            str_buf,        
                            (void*)font_roboto_32.font,          
                            0x1B02D0, Black);

        gdispFillString( offset_pstr + 4, 75 + 35,    
                            (char*)training_result[7],        
                            (void*)font_roboto_20.font,               
                            0xD0021B, Black);

        img_exflash_to_framebuffer((uint8_t*)"g_widget_00_enter.bmp", 60, 150,
                                    &image_width, &image_height, d_justify_center);

        if (sport_scrollbar != NULL) {
            scrollbar_show(sport_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT));
        }
    }
    
    ry_free(str_buf);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void outdoor_running_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart)
{
    extern LocationResult dev_locat;
    static int tick_step_distance;
    static float last_gps_distance;

    //data access and report
    if ((state == STATE_RUNNING_DOING) || \
        (state == STATE_ONE_KM_ALERT) || \
        (state == STATE_RUNNING_BREAK)){
        if (count % GET_LOCATION_TIME == 0){
            update_run_param();
        }

        if (gps_location_succ_get()){
            if (*p_distance < dev_locat.last_distance){
                *p_distance = dev_locat.last_distance;          //update distance from gps
                *p_step_distanceStart = alg_get_step_today();
                tick_step_distance = 0;
            }
            else if (tick_step_distance++ >= 9){                //update distance from step cacluation
                if (dev_locat.last_distance < last_gps_distance + 0.5f){
                    if (sport_common_get_distance_by_step(p_distance, p_step_distanceStart) != 0){
                        set_run_point(*p_distance);            //save distance point
                    }
                    tick_step_distance = 0;
                }
            }
            last_gps_distance = dev_locat.last_distance;
        }
        else if (tick_step_distance++ >= 9){                //update distance from step cacluation
            if (sport_common_get_distance_by_step(p_distance, p_step_distanceStart) != 0){
                set_run_point(*p_distance);                 //save distance point
            }
            tick_step_distance = 0;
        }
    }

    //access gps status
    if ((state == STATE_ACCESS_GPS) || \
        (state == STATE_ACCESS_GPS_SKIP) || \
        (state == STATE_SHOW_COUNT)){
        if (count % GET_LOCATION_TIME_COLD_START == 0){
            update_run_param();
        }
    }
}

void outdoor_runing_info_display_update(uint32_t flush, ui_disp_data_t* pdata)
{
    //time now
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time);

    if(pdata->hrm != 0){
        hrm_ave_calc(pdata->hrm);
    }

    runing_info_display(cur_time, pdata->duration_second, pdata->distance, pdata->hrm);
    if (flush){
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
}

uint32_t outdoor_running_result_pre_proc(u32_t duration_second, u32_t delta_step)
{
    u32_t ret = 0;

    if (sport_common_abnormal_result_processing((duration_second > 120), (delta_step > 200))){
        set_current_running_disable();
        ret = 1;     // do not display running result.
    }
    return ret;
}

void outdoor_running_result_disp(u8_t screen_num, ui_disp_data_t* pdata)
{
    float step_distance = alg_get_step_distance(pdata->delta_step);
    if (pdata->distance > step_distance) {
        float offset = pdata->distance - step_distance;
        data_sports_distance_offset_update(offset);
    }

    float calorie = get_step_calorie(pdata->delta_step);
    float calorie_distance = alg_calc_calorie_by_distance(pdata->distance, 1);
    if (calorie_distance > calorie){
        calorie = calorie_distance;        
    }

    running_result_display(screen_num, pdata->distance, pdata->duration_second, pdata->hrm_ave, calorie);
}

void outdoor_running_pause(void)
{
    pause_record_run();
//    DEV_STATISTICS(sport_pause);
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);  
}


void outdoor_running_resume(void)
{
    resume_record_run();                                        
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START, NULL);
}


void outdoor_running_terminate(u32_t from_doing)
{
//    DEV_STATISTICS(sport_stop);
//    DEV_STATISTICS(sport_finish);        
    clear_running();
    if (from_doing != 0){
	    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);
    }
    alg_msg_transfer(ALG_MSG_GENERAL);
    stop_record_run();    
}

u32_t outdoor_running_milestone_alert(u32_t duration_second, float distance_now, milestone_data_t* pmile)
{
    u32_t ret = 0;

    if ((distance_now - pmile->distance_1km_start) >= 1000.0f){
    // if ((alg_get_step_today() - pmile->start_step_1km) > 20){
        pmile->all_km ++;
        motar_light_loop_working(2);
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);                
        sport_common_one_km_alert_display_update(duration_second - pmile->start_durationSec_1km, pmile->all_km);
        pmile->distance_1km_start = distance_now;
        pmile->start_step_1km = alg_get_step_today();        
        pmile->start_durationSec_1km = duration_second;   
        ret = 1;           
    }
    return ret;
}
