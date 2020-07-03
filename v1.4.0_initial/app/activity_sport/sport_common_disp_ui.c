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
const char* const sports_animate[] = {"sport_animate_01.bmp",
                                "sport_animate_02.bmp", 
                                "sport_animate_03.bmp"};


const char* const hrm_level_str[]   = {"без нагр.", "разогрев", "фитнес", "кардио", "выс. нагр.", "макс. нагр."};
//const char* hrm_level_str[]   = {"idle", "warm-up", "fitness", "cardio", "hardcore", "maximum"};
const u32_t hrm_level_color[] = {0x808080, 0x6eb5ff, 0x85d115, 0xf8c11c, 0xf56923, 0xff1717};

const char* const gps_search = "Поиск GPS";
const char* const no_record_until_found_pos[] = {"Запись", "отменена"};
const char* const gps_pos_found[] = {"GPS коорд.", "найдены"};
const char* const gps_pos_failed[] = {"GPS коорд.", "не найдены.", "Без трека"};
const char* const paused = "На паузе";
const char* const too_short_to_record[] = {"Мало врем.", "Мала дист.", "для записи"};
const char* const one_km_alert[] = {"км ", "время"};

/*********************************************************************
 * FUNCTIONS
 */

uint32_t get_sports_hrm_level(uint32_t hrm, uint32_t age)
{
    u32_t max_hrm = ((age > 10) && (age < 60)) ? (208 - age * 7 / 10) : 190;
    u32_t percent = (hrm * 100) / max_hrm;
    uint8_t hrm_rate[] = {30, 60, 70, 80, 90, 100};
    int i;
    for (i = 0; (hrm_rate[i] < percent) && (i < sizeof(hrm_rate) - 1); i ++){
        ;
    }
    // LOG_DEBUG("get_sports_hrm_level: age: %d, max_hrm: %d, percent: %d, hrm: %d, i: %d\n", age, max_hrm, percent, hrm, i);;
    return i;
}

void sport_common_ready_go_display_update(int ready_count)
{
    u8_t w, h;
    clear_buffer_black();
    img_exflash_to_framebuffer((u8_t*)sports_animate[ready_count], \
                                60, 134, &w, &h, d_justify_bottom_center);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void sport_gps_info_display_update(int ready_count)
{
    u8_t w, h;
    clear_buffer_black(); // MSG_TITLE_FONT_HEIGHT
    gdispFillStringBox( 0,                    
                        36,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)gps_search,        
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyCenter);
//    gdispFillStringBox( 0,
//                        64,
//                        font_roboto_20.width,
//                        font_roboto_20.height,
//                        (char*)"Open App",
//                        (void*)font_roboto_20.font,
//                        White,
//                        justifyCenter);
    img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void sport_gps_skip_display_update(int ready_count)
{
    u8_t w, h;
    clear_buffer_black();
    gdispFillStringBox( 0,                    
                        20,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)gps_search,        
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyCenter);

    gdispFillStringBox( 0,                    
                        44,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)no_record_until_found_pos[0],
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyCenter);

    gdispFillStringBox( 0,                    
                        68,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)no_record_until_found_pos[1],
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyCenter);

    img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void sport_gps_acess_result_display_update(int result_succ)
{
    u8_t w, h;
    clear_buffer_black();

    if (result_succ){
        gdispFillStringBox( 0,                    
                            36,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_found[0],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);

        gdispFillStringBox( 0,                    
                            64,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_found[1],
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);
    }
    else{
        gdispFillStringBox( 0,                    
                            20,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_failed[0],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);

        gdispFillStringBox( 0,                    
                            44,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_failed[1],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);

        gdispFillStringBox( 0,                    
                            68,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_failed[1],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);
    }
    img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}


void sport_gps_running_result_display_update(int result_succ)
{
    u8_t w, h;
    clear_buffer_black();

    if (result_succ){
        gdispFillStringBox( 0,                    
                            36,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_found[0],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);

        gdispFillStringBox( 0,                    
                            64,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_found[1],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);
    }
    else{
        gdispFillStringBox( 0,                    
                            36,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_failed[0],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);

        gdispFillStringBox( 0,                    
                            64,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)gps_pos_failed[1],        
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyCenter);
    }
    img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void sport_common_pausing_display(void)
{
    u8_t w, h;
    clear_buffer_black();
    gdispFillStringBox( 0,                    
                        60,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)paused,        
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyCenter);
    
    img_exflash_to_framebuffer("g_widget_05_restore.bmp", \
                                20, 130, &w, &h, d_justify_left);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

uint32_t sport_common_abnormal_result_processing(u32_t second_ok, u32_t step_ok)
{
    uint32_t abnormal_result = 0;
    // LOG_INFO("[%s]second:%d,step:%d",__FUNCTION__,sport_ctx_v.start_second,(alg_get_step_today() - sport_ctx_v.start_step));
    if (second_ok == 0){
        DEV_STATISTICS(sport_short_time);
        abnormal_result = 1;
        u8_t w, h;
        clear_buffer_black();
        gdispFillStringBox( 0,                    
                            36,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)too_short_to_record[0],
                            (void*)font_roboto_20.font,               
                            White, Black,
                            justifyCenter);
        gdispFillStringBox( 0,                    
                            64,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)too_short_to_record[2],   
                            (void*)font_roboto_20.font,               
                            White, Black,
                            justifyCenter);
        img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                    60, 134, &w, &h, d_justify_center);
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
    else if (step_ok == 0){
        DEV_STATISTICS(sport_short_distance);
        abnormal_result = 1;
        u8_t w, h;
        clear_buffer_black();
        gdispFillStringBox( 0,                    
                            36,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)too_short_to_record[1],
                            (void*)font_roboto_20.font,               
                            White, Black,
                            justifyCenter);
        gdispFillStringBox( 0,                    
                            64,    
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)too_short_to_record[2],
                            (void*)font_roboto_20.font,               
                            White, Black,
                            justifyCenter);
        img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                    60, 134, &w, &h, d_justify_center);
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }

    return abnormal_result;
}

void sport_common_break_option_display(void)
{
    u8_t w, h;
    clear_buffer_black();
    img_exflash_to_framebuffer("g_widget_01_pending.bmp", \
                                20, 30, &w, &h, d_justify_left);

    img_exflash_to_framebuffer("g_widget_03_stop2.bmp", \
                                20, 130, &w, &h, d_justify_left);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

ry_sts_t sport_common_one_km_alert_display_update(uint32_t time_1km, uint8_t all_km)
{
    u8_t w, h;
    char * info_str = (char *)ry_malloc(40);
    
    clear_buffer_black();
    const int width = 40;
    const int height = 32;
    extern const unsigned char gImage_icon_1km[3840];
    draw_raw_img24_to_framebuffer((120 - width) / 2, 48, width, height, (uint8_t*)gImage_icon_1km);

    gdispFillStringBox( 0,                    
                        100,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)one_km_alert[0],        
                        (void*)font_roboto_20.font,
                        0x50E3C2, Black,
                        justifyCenter);

    sprintf(info_str, "%d", all_km);
    int len = strlen((const char *)info_str);
    gdispFillString(30 - (len - 1) * 6, 102, info_str, font_roboto_16.font, HTML2COLOR(0x50E3C2), Black);

    gdispFillStringBox( 0,                    
                        183,    
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)one_km_alert[1],        
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyCenter);

    sprintf(info_str, "%02d:%02d:%02d", time_1km/3600, (time_1km%3600)/60, time_1km%60);
    gdispFillString(3, 208,
                    info_str,
                    (void*)font_roboto_32.font,
                    White, Black);

    ry_free(info_str);
                            
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    return RY_SUCC;
}
