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

const char* free_training_result[] = {"Расстояние", "км", "Время", "Скорость", "Расход", "ккал", "Сред. BPM", "уд/мин"};

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
void free_info_display(ry_time_t cur_time, u32_t duration_second, float calorie, u8_t hrm_cnt)
{
    char * info_str = (char *)ry_malloc(40);
    u8_t offset_y = 3;
    
    clear_buffer_black();

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void*)font_roboto_20.font, Grey, Black);
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 30, info_str, (void *)font_roboto_32.font, 0x15d185, Black);

    //calories title
    gdispFillString(3, 60 + offset_y, free_training_result[4], (void*)font_roboto_20.font, White, Black);

    //calories info
    if (calorie < 1000)
        sprintf(info_str, "--");
    else
        sprintf(info_str, "%d", (u32_t)(calorie * 0.001f));
    
    gdispFillString(3, 89 + offset_y, info_str, (void *)font_roboto_32.font, 0xC2E350, Black);
    
    int dist_width = getStringWidthInnerFont(info_str, font_roboto_32.font);
    gdispFillString(8 + dist_width, 89 + offset_y, free_training_result[5], (void*)font_roboto_20.font, 0xC2E350, Black);

    //hrm title
    gdispFillString(3, 125 + offset_y, "BPM", (void*)font_roboto_20.font, White, Black);

    //hrm count    
    if (duration_second < 8) {
        sprintf(info_str, "--");
    } else if(hrm_cnt != 0) {
        sprintf(info_str, "%d", hrm_cnt);
    } else {
        sprintf(info_str, "--");
    }
    
    coord_t offset_pstr = gdispGetStringWidth(info_str, font_roboto_32.font) + 4;
    
    user_data_birth_t* birth = get_user_birth();
    uint32_t age = cur_time.year + 2000 - birth->year;
    u8_t hrm_level = get_sports_hrm_level(s_alg.hr_cnt, age);
    
    color_t cl = ((hrm_level_color[hrm_level] & 0xff) << 16)
                | ((hrm_level_color[hrm_level] & 0xff0000) >> 16)
                | ((hrm_level_color[hrm_level] & 0x00ff00) >> 0);
    
    gdispFillString(RM_OLED_WIDTH - offset_pstr, 127, info_str, (void *)font_roboto_32.font, cl, Black);
    
    //hrm status text
    if (duration_second >= 8){
        sprintf(info_str, "%s", hrm_level_str[hrm_level]);
        gdispFillString(4, 151 + offset_y, info_str, (void*)font_roboto_20.font, hrm_level_color[hrm_level], Black);
    }
    
    ry_free(info_str);
}

void free_result_display(u8_t screen_num, u32_t duration_second, u8_t hrm_ave, float calorie)
{
    u8_t image_width, image_height;
    char * str_buf = (char*)ry_malloc(50);
    clear_buffer_black();
    
    if (screen_num == 0) {                            
        // time
        gdispFillString( 0, 0,    
                            (char*)free_training_result[2],        
                            (void*)font_roboto_20.font,               
                            White, Black);

        sprintf(str_buf, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
        gdispFillString( 1, 30,
                            str_buf,        
                            (void*)font_roboto_32.font,             
                            0x15d185, Black);
        // calories
        gdispFillString( 0, 70,
                            (char*)free_training_result[4],        
                            (void*)font_roboto_20.font,               
                            White, Black);
        
        sprintf(str_buf, "%d", (u32_t)(calorie * 0.001f));
        coord_t offset_pstr = gdispGetStringWidth(str_buf, font_roboto_32.font);

        gdispFillString( 1, 70 + 30,
                            str_buf,        
                            (void*)font_roboto_32.font,              
                            0x1CE7F8, Black);

        gdispFillString( offset_pstr + 4, 70 + 32,    
                            (char *)free_training_result[5],        
                            (void*)font_roboto_20.font,              
                            0xF8E71C, Black);
                            
        // avg BPM
        gdispFillString( 0, 140,    
                            (char*)free_training_result[6],        
                            (void*)font_roboto_20.font,               
                            White, Black);

        sprintf(str_buf, "%d", hrm_ave);
        offset_pstr = gdispGetStringWidth(str_buf, font_roboto_32.font);
                            
        gdispFillString( 1, 140 + 30,    
                            str_buf,        
                            (void*)font_roboto_32.font,          
                            0x1B02D0, Black);

        gdispFillString( offset_pstr + 4, 140 + 32,    
                            (char*)free_training_result[7],        
                            (void*)font_roboto_20.font,               
                            0xD0021B, Black);                            

        if(sport_scrollbar != NULL){
            scrollbar_show(sport_scrollbar, frame_buffer, 0);
        }        
    }
    else if(screen_num == 1) {
        img_exflash_to_framebuffer((uint8_t*)"g_widget_00_enter.bmp", 60, 140,
                                    &image_width, &image_height, d_justify_center);

        if (sport_scrollbar != NULL) {
            scrollbar_show(sport_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT));
        }
    }
    
    ry_free(str_buf);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void free_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart)
{
    // data access and report
    if ((state == STATE_RUNNING_DOING) || \
        (state == STATE_ONE_KM_ALERT) || \
        (state == STATE_RUNNING_BREAK)){
        if (count % GET_LOCATION_TIME == 0){
            // update_run_param();
            //if (sport_common_get_distance_by_step(p_distance, p_step_distanceStart) != 0){
                // save distance point
            
            // p_step_distanceStart contains duration in seconds
            set_free_point(*p_step_distanceStart);
        }
    }
}

void free_info_display_update(uint32_t flush, ui_disp_data_t* pdata)
{
    //time now
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time);

    uint32_t avg_hrm = 0;
    
    if (pdata->hrm != 0) {
        avg_hrm = hrm_ave_calc(pdata->hrm);
    }
    
    float calorie = 0;
    
    if (pdata->duration_second > 0)
        calorie = get_hrm_calorie_second(avg_hrm, pdata->duration_second);
    //float calorie_sport = get_step_calorie_sport(pdata->delta_step);

    free_info_display(cur_time, pdata->duration_second, calorie, pdata->hrm);
    if (flush) {
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
}

uint32_t free_result_pre_proc(u32_t duration_second, u32_t delta_step)
{
    u32_t ret = 0;

    if (sport_common_abnormal_result_processing((duration_second > 120), 1)){
        // set_current_running_disable();
        ret = 1;     // do not display running result.
    }
    return ret;
}

void free_result_disp(u8_t screen_num, ui_disp_data_t* pdata)
{
    float step_distance = alg_get_step_distance(pdata->delta_step);
    if (pdata->distance > step_distance) {
        float offset = pdata->distance - step_distance;
        data_sports_distance_offset_update(offset);
    }

    float calorie = 0;    
    if (pdata->duration_second > 0)
        calorie = get_hrm_calorie_second(pdata->hrm_ave, pdata->duration_second);

    free_result_display(screen_num, pdata->duration_second, pdata->hrm_ave, calorie);
}

void free_pause(void)
{
    // pause_record_run();
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);  
}


void free_resume(void)
{
    // resume_record_run();                                        
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START, NULL);
}


void free_terminate(u32_t from_doing)
{
    clear_running();
    if (from_doing != 0) {
	    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);
    }
    alg_msg_transfer(ALG_MSG_GENERAL);
    // stop_record_run();    
}
