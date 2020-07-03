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
#include <stdio.h>
#include "ry_type.h"
#include "ry_utils.h"
#include "app_config.h"
#include "ryos.h"
#include "ryos_timer.h"
#include <ry_hal_inc.h>
#include "scheduler.h"
#include "board_control.h"

/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "window_management_service.h"
#include "card_management_service.h"
#include "timer_management_service.h"
#include "device_management_service.h"
#include "data_management_service.h"
#include "activity_setting.h"
#include "activity_surface.h"
#include "activity_card.h"
#include "activity_find_phone.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include <dip.h>
#include "motar.h"
#include "board_control.h"
#include "ry_statistics.h"
#include "scrollbar.h"
#include "hci_api.h"
#include "em9304_patches.h"

/* resources */
#include "gui_img.h"
#include "ry_nfc.h"
#include "ry_font.h"
#include "bstream.h"

void setting_about_display_update(void);
void setting_dnd_display_update(void);
void setting_unlock_display_update(void);
void setting_find_phone_display_update(void);
void setting_reset_device_display_update(void);
void setting_reboot_device_display_update(void);
void setting_brightness_display_update(void);
void setting_flashlight_display_update(void);
void setting_stepcoef_display_update(void);
void setting_nfc_display_update(void);
void setting_lcd_display_update(void);
void setting_alarm_display_update(void);
void setting_sleep_display_update(void);
void setting_data_display_update(void);

void setting_about_on_action(uint32_t y);
void setting_dnd_on_action(uint32_t y);
void setting_unlock_on_action(uint32_t y);
void setting_find_phone_on_action(uint32_t y);
void setting_reset_device_on_action(uint32_t y);
void setting_reboot_device_on_action(uint32_t y);
void setting_reboot_device_on_action_light(uint32_t y);
void setting_brightness_on_action(uint32_t y);
void setting_flashlight_on_action(uint32_t y);
void setting_stepcoef_on_action(uint32_t y);
void setting_nfc_on_action(uint32_t y);
void setting_lcd_on_action(uint32_t y);
void setting_alarm_on_action(uint32_t y);
void setting_sleep_on_action(uint32_t y);
void setting_data_on_action(uint32_t y);

void about_display_battery_info(uint32_t position);
void about_flash_info(uint32_t position);
void about_last_charge_info(uint32_t position);
void about_display_version(uint32_t position);
void about_ble_info(uint32_t position);
void about_display_device_id_info(uint32_t position);
void about_display_mac(uint32_t position);

void brightness_display_high(uint32_t position);
void brightness_display_middle(uint32_t position);
void brightness_display_low(uint32_t position);
void brightness_display_night(uint32_t position);

/*********************************************************************
 * CONSTANTS
 */ 
#define COLOR_BRIGHT_GREEN      (HTML2COLOR(0x80D015))
#define COLOR_BRIGHT_GREEN_RGB  (HTML2COLOR(0x15D080))

#define SETTING_STRING_COLOR    (White)

#define NFC_TEST        0
#define NFC_PAGE_COUNT  2

#define SCROLLBAR_HEIGHT                    40
#define SCROLLBAR_WIDTH                     1
#if (RAWLOG_SAMPLE_ENABLE == FALSE)
#define DND_MODE_ENABLE                     1
#endif

/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    ACTIVITY_SETTING_READY, 
    ACTIVITY_SETTING_ACTION,    
}activity_setting_status_t;

typedef enum {
    BATTERY_STATE_PERCENT, 
    BATTERY_STATE_LOGVIEW,    
}battery_info_status_t;

typedef struct {
    int8_t        head;
    uint8_t       sel_item;
    u8_t          state;
    u8_t          state_battery;
    int8_t        sel_about;
    int8_t        sel_brightness;
    int8_t        sel_nfc;
    u8_t          lcd_state;
    u8_t          sleep_off;
    u8_t          data_off;
    u8_t          alarm_repeat;         //  0 - 10min (default), 1 - 5 min, 2 - 15min
    int8_t        brightness_rate;   
    u8_t          dnd_st;
    u8_t          dnd_manual_mode;
    u8_t          reset_confirm; 
    u8_t          reset_click;    
} ry_setting_ctx_t;
 
enum {
    SETTING_ABOUT,
#if NFC_TEST == 1
    SETTING_NFC,
#endif    
    SETTING_BRIGHTNESS,
    SETTING_FLASHLIGHT,
    SETTING_DND, 
	SETTING_FIND_PHONE,
    SETTING_UNLOCK,
	SETTING_STEP_COEF,
    SETTING_LCD_DIMMING,
    SETTING_ALARM_REPEAT,
    SETTING_SLEEP_OFF,
    SETTING_DATA_OFF,
    SETTING_MAGIC,
    SETTING_RESET,
    SETTING_NUM_MAX,
} setting_item;

typedef struct {
    const char* discription;
    void        (*display)(void);            
    void        (*on_action)(uint32_t y);        
} setting_item_t;

typedef struct {         
    void    (*display)(uint32_t position);        
} sub_item_display_t;

/*********************************************************************
 * VARIABLES
 */
extern activity_t activity_surface;
extern ry_surface_ctx_t surface_ctx_v;

static u32_t      app_setting_timer_id;
ry_setting_ctx_t  setting_ctx_v;

activity_t        activity_setting = {
    .name      = "setting",
    .onCreate  = setting_onCreate,
    .onDestroy = setting_onDestroy,
    .priority = WM_PRIORITY_M,
};

u8_t  setting_dnd_underway = 0;

const char* const setting_about_str[]  = {"Батарея", "Свободно", "От зарядки", "Версия FW", "ID", "MAC", "Ble state"};   
const char* const setting_dnd_info[]  =  {"Включить", "Выключить", "режим"};
const char* const setting_dnd_st_info[]  =  {"Выключен", "Включен"};
const char* const setting_dnd_tag_info[]  =  {"Выкл.", "Вкл."};
const char* const setting_lcd_info[]  =  {"Плавно", "Сразу"};
const char* const setting_sleep_info[]  =  {"Выкл.", "Вкл."};
const char* const setting_alarm_repeat_info[]  = {"10 мин", "5 мин", "15 мин"};
static u8_t const setting_alarm_repeat_value[] = { 10, 5, 15};
const char* const setting_unlock_tag_info[]  =  {"Выкл.", "Слайд", "Код"};

const char* const text_charging = "зарядка";
const char* const text_reboot_confirm1 = "Перезагруз.";
const char* const text_reboot_confirm2 = "браслет?";
const char* const text_rebooting = "Перезапуск";
const char* const text_reset_confirm1 = "Выполнить";
const char* const text_reset_confirm2 = "сброс?";
const char* const text_reset_complete = "Выполнено";

const char* const text_brightness_high = "100%";
const char* const text_brightness_middle = "50%";
const char* const text_brightness_low = "0%";
const char* const text_brightness_Auto1 = "Ночной";
const char* const text_brightness_Auto2 = "режим";
const char* const night_brightness_str[] = {"выкл.", "вкл."};

const setting_item_t setting_array[] = {
   {"О браслете",   setting_about_display_update,		    setting_about_on_action},
#if NFC_TEST == 1
   {"NFC Test",		setting_nfc_display_update,		        setting_nfc_on_action},
#endif   
   {"Яркость",		setting_brightness_display_update,		setting_brightness_on_action},
   {"Фонарик",		setting_flashlight_display_update,		setting_flashlight_on_action},
   {"Не бесп.",     setting_dnd_display_update,				setting_dnd_on_action},
   {"Найти тел.",	setting_find_phone_display_update,		setting_find_phone_on_action},
   {"Блокировка",   setting_unlock_display_update,		    setting_unlock_on_action},
   {"Коэф. раст.",  setting_stepcoef_display_update,		setting_stepcoef_on_action},
   {"Режим LCD",	setting_lcd_display_update,		        setting_lcd_on_action},
   {"Повтор буд.",	setting_alarm_display_update,		    setting_alarm_on_action},
   {"Монитор сна",	setting_sleep_display_update,		    setting_sleep_on_action},
   {"Мон. данных",	setting_data_display_update,		    setting_data_on_action},
   {"Перезагруз.",	setting_reboot_device_display_update,	setting_reboot_device_on_action},
//   {"Reboot",       setting_reboot_device_display_update,	setting_reboot_device_on_action_light},
   {"Сбросить",		setting_reset_device_display_update,	setting_reset_device_on_action},
};

const char* const text_find_phone[] = {"Найти", "телефон"};
const char* const text_sleep_monitor[2] = {"Монитор", "сна"};
const char* const text_data_monitor[2] = {"Монитор", "данных"};

const u8_t setting_array_size =  sizeof(setting_array)/sizeof(setting_array[0]);

const sub_item_display_t about_item_array[] = {
    about_display_battery_info,
    about_last_charge_info,
    about_display_version,
    about_ble_info,
    about_flash_info,
    about_display_device_id_info,
    about_display_mac,
};

const sub_item_display_t brightness_item_array[] = {
    brightness_display_high,
    brightness_display_middle,
    brightness_display_low,
    brightness_display_night,
};

const color_t bightness_str_color[] = {White, COLOR_BRIGHT_GREEN_RGB};

scrollbar_t * setting_scrollbar = NULL;

/*********************************************************************
 * functions
 */

void activity_setting_timeout_handler(void* arg)
{
    // LOG_DEBUG("[activity_setting_timeout_handler]: transfer timeout.\n");
    // ry_timer_start(app_setting_timer_id, 5000, activity_setting_display_off_timeout_handler, NULL);
}

ry_sts_t setting_option_display_update(int8_t list_head)
{
    ry_sts_t ret = RY_SUCC;

    clear_buffer_black();
    
    if (list_head == SETTING_FIND_PHONE)
    {
        gdispFillStringBox( 0, 
                            54, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_find_phone[0], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
        gdispFillStringBox( 0, 
                            82, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_find_phone[1], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
    }
    else if (list_head == SETTING_SLEEP_OFF ||
        list_head == SETTING_DATA_OFF)
    {
        const char* const (*text_monitor)[2] = &text_sleep_monitor;
        if (list_head == SETTING_DATA_OFF)
            text_monitor = &text_data_monitor;
        
        gdispFillStringBox( 0, 
                            44, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)(*text_monitor)[0], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
        gdispFillStringBox( 0, 
                            72, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)(*text_monitor)[1], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );        
    }
    else {
        gdispFillStringBox( 0, 
                            68, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)setting_array[list_head].discription, 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
    }
    
    gdispDrawLine(4, 140, 120 - 4, 140, HTML2COLOR(0x404040));

    if ((list_head + 1) == SETTING_FIND_PHONE)
    {
        gdispFillStringBox( 0, 
                            158, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_find_phone[0], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
        gdispFillStringBox( 0, 
                            186, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_find_phone[1], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
    }
    else if ((list_head + 1) == SETTING_SLEEP_OFF ||
        (list_head + 1) == SETTING_DATA_OFF)
    {
        const char* const (*text_monitor)[2] = &text_sleep_monitor;
        if ((list_head + 1) == SETTING_DATA_OFF)
            text_monitor = &text_data_monitor;
        
        gdispFillStringBox( 0, 
                            148, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)(*text_monitor)[0], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );
        gdispFillStringBox( 0, 
                            176, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)(*text_monitor)[1], 
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR,  
                            Black,
                            justifyCenter
                          );         
    }    
    else {
        gdispFillStringBox( 0, 
                            172, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)setting_array[list_head + 1].discription,
                            (void *)font_roboto_20.font, 
                            SETTING_STRING_COLOR, 
                            Black,
                            justifyCenter
                          );
    }
    
    // do not disturb status
    color_t dnd_color = (setting_ctx_v.dnd_st != 0) ? COLOR_BRIGHT_GREEN_RGB : Gray;    
    
    int offsetY = 102;
    if ((list_head + 1) == SETTING_DND)
        offsetY = 204;
    
    if ((list_head + 1) == SETTING_DND || list_head == SETTING_DND)
        gdispFillStringBox( 0, offsetY, 
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)setting_dnd_tag_info[setting_ctx_v.dnd_st],
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
        
    // unlock display status
    offsetY = 102;
    if ((list_head + 1) == SETTING_UNLOCK)
        offsetY = 204;
    
    int unlock_option = surface_ctx_v.unlock_type;
    dnd_color = (unlock_option != 0) ? COLOR_BRIGHT_GREEN_RGB : Gray;   
    
    if ((list_head + 1) == SETTING_UNLOCK || list_head == SETTING_UNLOCK)
        gdispFillStringBox( 0, offsetY,
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)setting_unlock_tag_info[unlock_option],
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
                            
    // distance correction coef display    
    if ((list_head + 1) == SETTING_STEP_COEF || list_head == SETTING_STEP_COEF)
    {
        offsetY = 102;
        if ((list_head + 1) == SETTING_STEP_COEF)
            offsetY = 204;
        
        float step_coef = get_step_distance_coef() * 0.1f;
        dnd_color = COLOR_BRIGHT_GREEN_RGB;
                
        char step_str[20] = {0};
        
        sprintf(step_str, "%.1f", step_coef);
        
        gdispFillStringBox( 0, offsetY,
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)step_str,
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
    }
    
    // LCD dimming 
    if ((list_head + 1) == SETTING_LCD_DIMMING || list_head == SETTING_LCD_DIMMING)
    {
        offsetY = 102;
        if ((list_head + 1) == SETTING_LCD_DIMMING)
            offsetY = 204;

        dnd_color = COLOR_BRIGHT_GREEN_RGB;

        int lcd_state = setting_ctx_v.lcd_state;

        gdispFillStringBox( 0, offsetY,
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)setting_lcd_info[lcd_state],
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
    }
    
    // alarm repeat timeout setting
    if ((list_head + 1) == SETTING_ALARM_REPEAT || list_head == SETTING_ALARM_REPEAT)
    {
        offsetY = 102;
        if ((list_head + 1) == SETTING_ALARM_REPEAT)
            offsetY = 204;

        dnd_color = COLOR_BRIGHT_GREEN_RGB;

        int alarm_repeat = setting_ctx_v.alarm_repeat;

        gdispFillStringBox( 0, offsetY,
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)setting_alarm_repeat_info[alarm_repeat],
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
    }
    
    // sleep monitor on/off
    if ((list_head + 1) == SETTING_SLEEP_OFF || list_head == SETTING_SLEEP_OFF)
    {
        offsetY = 102;
        if ((list_head + 1) == SETTING_SLEEP_OFF)
            offsetY = 204;

        dnd_color = COLOR_BRIGHT_GREEN_RGB;

        int sleep_off = setting_ctx_v.sleep_off;

        gdispFillStringBox( 0, offsetY,
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)setting_sleep_info[sleep_off],
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
    }
    
    // data monitor on/off
    if ((list_head + 1) == SETTING_DATA_OFF || list_head == SETTING_DATA_OFF)
    {
        offsetY = 102;
        if ((list_head + 1) == SETTING_DATA_OFF)
            offsetY = 204;

        dnd_color = COLOR_BRIGHT_GREEN_RGB;

        int data_off = setting_ctx_v.data_off;

        gdispFillStringBox( 0, offsetY,
                            font_roboto_20.width, font_roboto_20.height,
                            (char *)setting_sleep_info[data_off],
                            (void *)font_roboto_20.font, 
                            dnd_color, Black, justifyCenter
                        );
    }     

    if (setting_scrollbar != NULL) {
        u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (SETTING_NUM_MAX - 2);
        int32_t posY = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) - (((SETTING_NUM_MAX - 2) - list_head) * sp_posix);
        scrollbar_show(setting_scrollbar, frame_buffer, posY);
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    return ret;
}


void draw_rect(int x, int y, int width, int value)
{
    int i, j, color;
    
    // limit max value
    if (value > 15) value = 15;
    
    int height = value * 2;

    if (value > 10)
        color = HTML2COLOR(0x0000FF); // Red;
    else if (value > 8)
        color = HTML2COLOR(0x00A5FF); // Orange;
    else if (value > 6)
        color = ~Yellow;
    else if (value > 4)
        color = HTML2COLOR(0x80D015); // Olive;
    else
        color = HTML2COLOR(0x15D080); // Green;    
    
    for(i = y; i > y - height; i--){
        for(j = x; j < x + width; j++){
            set_dip_at_framebuff(j, i, color);
        }
    }    
}

void take_battery_log(u8_t* log)
{
    int i, j, startDay, totalDays, batteryValue;
    int daysToShow = 10;
    extern ry_device_setting_t device_setting;
    
//    device_setting.battery_log[0] = 98;
//    device_setting.battery_log[1] = 92;
//    device_setting.battery_log[2] = 82;
//    device_setting.battery_log[3] = 71;
    
    for (i=0; i< BATTERY_LOG_MAX_DAYS; i++) {
        if (device_setting.battery_log[i] == 0xFF) break;        
    }

    totalDays = i;
    if (totalDays <= daysToShow) {
        startDay = 0;
        daysToShow = totalDays;
    }
    else {
        startDay = totalDays - daysToShow;
    }
    
    batteryValue = 100;
    if (startDay > 0)
        batteryValue = device_setting.battery_log[startDay - 1];
    
    j = 0;
    for (i = startDay; j < daysToShow; i++) {
        log[j] = batteryValue - device_setting.battery_log[i]; 
        batteryValue = device_setting.battery_log[i];
        j++;
    }
}

void about_display_battery_info(uint32_t position)
{
    //battery title
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[0], 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyLeft
                      );    
    
    if (setting_ctx_v.state_battery == BATTERY_STATE_LOGVIEW)
    {
        //u8_t battery_log[10] = {4, 10, 6, 3, 5, 4, 5, 3, 8, 0};
        u8_t battery_log[10] = {0xFF};
        take_battery_log(battery_log);

        int len = sizeof(battery_log);
        int x = 10;
        int y = 75;
        int day_width = 10;
        
        for (int i = 0; i < len; i++) {
            if (battery_log[i] == 0xFF) break;
            draw_rect(x, y, day_width - 1, battery_log[i]);
            x += day_width;
        }
    }
    else
    {
        char battery[16];
        sprintf(battery, "%d%%", sys_get_battery());

        if (charge_cable_is_input()){
            gdispFillStringBox( 4, 
                                position + 44, 
                                font_roboto_20.width,
                                font_roboto_20.height,
                                (char *)text_charging, 
                                (void*)font_roboto_20.font, 
                                White,  
                                Black,
                                justifyLeft
                                );
        }
        else{
            gdispFillStringBox( 4, 
                                position + 44, 
                                font_roboto_20.width,
                                font_roboto_20.height,
                                battery, 
                                (void*)font_roboto_20.font, 
                                White,
                                Black,
                                justifyLeft
                                );
        }
    }
}

void about_display_version(uint32_t position)
{    
    //version title
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[3], 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyLeft
                      );
    
    //version info
    gdispFillStringBox( 4, 
                        position + 44, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)get_device_version(), 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyLeft
                        );
}

void about_ble_info(uint32_t position)
{
    char ble_status[10] = {0};
    sprintf(ble_status, "%02x", em9304init_status);
        
    // ble title
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[6], 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyLeft
                      );
    
    // ble status
    gdispFillStringBox( 4, 
                        position + 44, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)ble_status, 
                        (void*)font_roboto_20.font, 
                        White,
                        Black,
                        justifyLeft
                        );
}

void about_display_device_id_info(uint32_t position)
{
    char did[50] = {0};
    uint8_t ret_len;    
    char did_string[16] = {0};    
    device_info_get(DEV_INFO_TYPE_DID, (u8_t*)did, &ret_len);
    strcpy(did_string, did);
    extern u32_t wFwVerRsp;
    //sprintf(did_string, "%02x%02x%02x", ((wFwVerRsp&0xff0000)>>16), ((wFwVerRsp&0xff00)>>8), (wFwVerRsp&0xff));// Using NFC version, for test

    //device id title
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[4], 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyLeft
                      );

    //device id info
    gdispFillStringBox( 4, 
                        position + 44, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        did_string, 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyLeft
                      );

}

void about_display_mac(uint32_t position)
{
    char mac_string[16] = {0};
    uint8_t mac[6];         
    uint8_t ret_len;    
                        
    extern uint8_t* device_info_get_mac(void);
    device_info_get(DEV_INFO_TYPE_MAC, mac, &ret_len);
    sprintf(mac_string, "%02X:%02X:%02X", mac[0], mac[1], mac[2]);
    
    //mac titile
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[5],
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB, 
                        Black,
                        justifyLeft
                      );
    
    //mac info
    gdispFillStringBox( 4, 
                        position + 44, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        mac_string, 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyLeft
                        );

    if (position < 200){
        //mac info_2
        sprintf(mac_string, "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
        gdispFillStringBox( 4, 
                            position + 44 + 32, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            mac_string, 
                            (void*)font_roboto_20.font, 
                            White,  
                            Black,
                            justifyLeft
                            );
    }
}

void about_flash_info(uint32_t position)
{	
    extern FATFS *ry_system_fs;
		
    u32_t free_size = 0;
    f_getfree("",(DWORD*)&free_size,&ry_system_fs);	
    
    char str_free[20];
    sprintf(str_free, "%d Kb", free_size << 2); // total size: free_size *100 /(6*1024/4)

    //flash free title
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[1], 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyLeft
                      );
    gdispFillStringBox( 4, 
                        position + 44, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        str_free, 
                        (void*)font_roboto_20.font, 
                        White,
                        Black,
                        justifyLeft
                        );
}

void about_last_charge_info(uint32_t position)
{	
    u32_t time = get_time_since_last_charge();
    
    int days = time / (24 * 3600); 
  
    time = time % (24 * 3600); 
    int hours = time / 3600; 
  
    time %= 3600; 
    int minutes = time / 60 ; 
  
    time %= 60; 
    int seconds = time;
    
    char str_time[20];
    
    if (days > 100) {
        sprintf(str_time, "--");
    } else if (days > 0) {
        sprintf(str_time, "%d дн. %02d:%02d", days, hours, minutes);
    }
    else if (hours > 0) {
        sprintf(str_time, "%02d:%02d час", hours, minutes);
    }
    else {
        sprintf(str_time, "%02d:%02d мин", minutes, seconds);
    }

    //flash free title
    gdispFillStringBox( 4, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_about_str[2], 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyLeft
                      );
    gdispFillStringBox( 4, 
                        position + 44, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        str_time, 
                        (void*)font_roboto_20.font, 
                        White,
                        Black,
                        justifyLeft
                        );
}

void setting_about_display_update(void)
{
    int startY, delta;
    int itemCnt = sizeof(setting_about_str)/sizeof(setting_about_str[0]);
	int itemCntMinus1 = itemCnt - 1;
    clear_buffer_black();    
    DEV_STATISTICS(about_dev_count);
    
    for (int i = setting_ctx_v.sel_about; (i <= itemCntMinus1) && (i < itemCntMinus1 + setting_ctx_v.sel_about); i ++){
        about_item_array[i].display((i - setting_ctx_v.sel_about) * 80);
        // LOG_DEBUG("[setting_about_display_update] item_i: %d, (i - setting_ctx_v.sel_about): %d\r\n", \
            i, (i - setting_ctx_v.sel_about));
        uint32_t line_pos = (i - setting_ctx_v.sel_about) + 1;
        if ((line_pos < itemCntMinus1) && (i < itemCntMinus1)){
            gdispDrawLine(4, 80 * line_pos, 120 - 4, 80 * line_pos, HTML2COLOR(0x303030));
            // LOG_DEBUG("[setting_about_display_update] line_pos: %d\r\n", line_pos);
        }        
    }

    if(setting_scrollbar != NULL){
        if (itemCnt > 2) {
            delta = 200 / (itemCnt - 2);
            startY = setting_ctx_v.sel_about * delta;
            scrollbar_show(setting_scrollbar, frame_buffer, startY);
        }
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

u8_t nfc_action = 0;
extern u8_t nfc_status[];
extern u8_t g_test_nfc_id[];

void setting_nfc_display_update(void)
{
    char card_id[20] = {0};
    clear_buffer_black();
    
    if (setting_ctx_v.sel_nfc == 0) {
    
        gdispFillStringBox( 0, 10, font_roboto_20.width, font_roboto_20.height,
                            (char *)"NFC reader",
                            (void*)font_roboto_20.font, White, Black, justifyCenter);
                            
        gdispDrawLine(4, 42, 120 - 4, 42, HTML2COLOR(0x505050));
        
        if (nfc_action == 1) {
            gdispFillStringBox( 0, 50, font_roboto_20.width, font_roboto_20.height,
                                (char *)"reading..",
                                (void*)font_roboto_20.font, White, Black, justifyCenter);        
        } else if (nfc_action == 2) {
            gdispFillStringBox( 0, 50, font_roboto_20.width, font_roboto_20.height,
                                (char *)"card id",
                                (void*)font_roboto_20.font, White, Black, justifyCenter);

            sprintf(card_id, "%02x:%02x:%02x:%02x", g_test_nfc_id[1], g_test_nfc_id[2], g_test_nfc_id[3], g_test_nfc_id[4]);
            gdispFillStringBox( 0, 85,  font_roboto_20.width, font_roboto_20.height,
                                (char *)card_id,
                                (void*)font_roboto_20.font, White, Black, justifyCenter);
                
            if (g_test_nfc_id[0] == 7) {
                sprintf(card_id, "%02x:%02x:%02x", g_test_nfc_id[5], g_test_nfc_id[6], g_test_nfc_id[7]);
                gdispFillStringBox( 0, 115,  font_roboto_20.width, font_roboto_20.height,
                                    (char *)card_id,
                                    (void*)font_roboto_20.font, White, Black, justifyCenter);                            
            }
                                
        } else if (nfc_action == 3 || nfc_action == 4) {
            
            gdispFillStringBox( 0, 50, font_roboto_20.width, font_roboto_20.height,
                                (char *)"failed",
                                (void*)font_roboto_20.font, White, Black, justifyCenter);        
            gdispFillStringBox( 0, 80, font_roboto_20.width, font_roboto_20.height,
                                (char *)"retry",
                                (void*)font_roboto_20.font, White, Black, justifyCenter);        
        }
    }
    else if (setting_ctx_v.sel_nfc == 1) {
        
        // Check NFC state
        u8_t is_se_on = ry_nfc_is_wired_enable();
        
        if (is_se_on)
            sprintf(card_id, "%s", "se is on");
        else
            sprintf(card_id, "%s", "se is off");

        gdispFillStringBox( 0, 50, font_roboto_20.width, font_roboto_20.height,
                            (char *)card_id,
                            (void*)font_roboto_20.font, White, Black, justifyCenter);
                            
        gdispDrawLine(4, 82, 120 - 4, 82, HTML2COLOR(0x505050));                            
                            
        if (is_se_on)
            sprintf(card_id, "%s", "turn it off");
        else
            sprintf(card_id, "%s", "turn it on");

        gdispFillStringBox( 0, 85, font_roboto_20.width, font_roboto_20.height,
                            (char *)card_id,
                            (void*)font_roboto_20.font, White, Black, justifyCenter);                            
    }
        
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);    
}

void setting_lcd_display_update(void) {}
    
void setting_alarm_display_update(void) {}
    
void setting_sleep_display_update(void) {}
    
void setting_data_display_update(void) {}

void setting_dnd_display_update(void)
{
    uint8_t w, h;
    clear_buffer_black();
    //DEV_STATISTICS(forbid_count);

    gdispFillStringBox( 0, 
                        50, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_dnd_info[setting_ctx_v.dnd_st],
                        (void*)font_roboto_20.font, 
                        White, 
                        Black,
                        justifyCenter
                        );

    gdispFillStringBox( 0, 
                        80, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_dnd_info[2],
                        (void*)font_roboto_20.font, 
                        White, 
                        Black,
                        justifyCenter
                        );

    if (setting_ctx_v.dnd_st == 0){
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_widget_00_enter.bmp", \
                    0, 156, &w, &h, d_justify_center);
    }
    else{
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_widget_05_restore.bmp", \
                    0, 156, &w, &h, d_justify_center);
    }
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);    
}


void setting_dnd_succ_display_update(void)
{
    uint8_t w, h;
    clear_buffer_black();
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_status_00_succ.bmp", \
                                0, 92, &w, &h, d_justify_center);
    gdispFillStringBox( 0, 
                        180, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)setting_dnd_st_info[setting_ctx_v.dnd_st],
                        (void*)font_roboto_20.font, 
                        White, 
                        Black,
                        justifyCenter
                        );
                        
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);   
}

void setting_unlock_display_update(void) {}

void setting_stepcoef_display_update(void) {}

void setting_find_phone_display_update(void)
{
	uint32_t userdata = 0;
	
	if (ry_ble_get_state() == RY_BLE_STATE_CONNECTED) {
		if (get_device_session() != DEV_SESSION_IDLE) {
			LOG_ERROR("[find_phone], session state=%d.",get_device_session());
		}
		else {
			find_phone_request(DeviceAlertType_find_phone_start);
		}
	}
	
	userdata = IPC_MSG_TYPE_FIND_PHONE_START;
	wms_activity_jump(&activity_find_phone, &userdata);
}


void setting_reset_device_display_update(void)
{
    uint8_t w, h;
    clear_buffer_black();
    if (setting_ctx_v.reset_confirm == 0){
        gdispFillStringBox( 0, 
                            50, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_reset_confirm1,
                            (void *)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );
														
        gdispFillStringBox( 0, 
                            80, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_reset_confirm2,
                            (void *)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );
														
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_widget_00_enter.bmp", \
                                0, 156, &w, &h, d_justify_center);
    }
    else{
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_status_00_succ.bmp", \
                                0, 92, &w, &h, d_justify_center);
        gdispFillStringBox( 0, 
                            180, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_reset_complete,
                            (void *)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );                    
    }
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);   
}


void setting_reboot_device_display_update(void)
{
    uint8_t w, h;
    clear_buffer_black();
    if (setting_ctx_v.reset_click == 0){
        gdispFillStringBox( 0, 
                            50, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_reboot_confirm1,
                            (void *)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );
														
        gdispFillStringBox( 0, 
                            80, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_reboot_confirm2,
                            (void *)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );														

        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_widget_00_enter.bmp", \
                                0, 156, &w, &h, d_justify_center);
    }
    else{
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_status_00_succ.bmp", \
                                0, 92, &w, &h, d_justify_center);
        gdispFillStringBox( 0, 
                            180, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)text_rebooting,
                            (void *)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );
    }
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);   
}


void brightness_display_high(uint32_t position)
{
    uint32_t bright_high = (get_brightness_rate() == E_BRIGHTNESS_HIGH);
    gdispFillStringBox( 0, 
                        position + 30, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_brightness_high, 
                        (void *)font_roboto_20.font,
                        bightness_str_color[bright_high],  
                        Black,
                        justifyCenter
                      );
}


void brightness_display_middle(uint32_t position)
{
    uint32_t bright_middle = (get_brightness_rate() == E_BRIGHTNESS_MIDDLE);
    gdispFillStringBox( 0, 
                        position + 30, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_brightness_middle, 
                        (void *)font_roboto_20.font, 
                        bightness_str_color[bright_middle],  
                        Black,
                        justifyCenter
                      );
}


void brightness_display_low(uint32_t position)
{
    uint32_t bright_low = (get_brightness_rate() == E_BRIGHTNESS_LOW);    
    gdispFillStringBox( 0, 
                        position + 30, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_brightness_low, 
                        (void *)font_roboto_20.font, 
                        bightness_str_color[bright_low],  
                        Black,
                        justifyCenter
                      );
}


void brightness_display_night(uint32_t position)
{
    color_t night_brightness_color = (get_brightness_night() != 0) ? COLOR_BRIGHT_GREEN_RGB : Gray;
    uint8_t night_brightness_en = (get_brightness_night() != 0) ? 1 : 0;
    gdispFillStringBox( 0, 
                        position + 10, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_brightness_Auto1, 
                        (void *)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );

    gdispFillStringBox( 0, 
                        position + 40, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_brightness_Auto2, 
                        (void *)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );

    if (setting_ctx_v.sel_brightness >= E_BRIGHTNESS_LOW){
        gdispFillStringBox( 0, 
                            position + 70, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)night_brightness_str[night_brightness_en], 
                            (void *)font_roboto_20.font, 
                            night_brightness_color,  
                            Black,
                            justifyCenter
                        );
    }

}

void setting_brightness_display_update(void)
{
    clear_buffer_black();    
    DEV_STATISTICS(about_dev_count);
    
    for (int i = setting_ctx_v.sel_brightness; (i <= 3) && (i < 3 + setting_ctx_v.sel_brightness); i ++){
        brightness_item_array[i].display((i - setting_ctx_v.sel_brightness) * 80);
        // LOG_DEBUG("[setting_about_display_update] item_i: %d, (i - setting_ctx_v.sel_brightness): %d\r\n", \
            i, (i - setting_ctx_v.sel_brightness));
        uint32_t line_pos = (i - setting_ctx_v.sel_brightness) + 1;
        if ((line_pos < 3) && (i < 3)){
            gdispDrawLine(4, 80 * line_pos, 120 - 4, 80 * line_pos, HTML2COLOR(0x303030));
        }
    }

    if(setting_scrollbar != NULL){
        u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (E_BRIGHTNESS_MAX + 1 - 2);
        int32_t posY = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) - (((E_BRIGHTNESS_MAX + 1 - 2) - setting_ctx_v.sel_brightness) * sp_posix);
        scrollbar_show(setting_scrollbar, frame_buffer, posY);
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}



void setting_flashlight_display_update(void)
{
    u8_t w, h;
    clear_buffer_white();
    
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"flashlight.bmp", 0, 74, &w, &h, d_justify_center);    
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    setting_flashlight_on_action(0);
}

void setting_about_on_action(uint32_t y)
{
    // LOG_DEBUG("[setting_about_on_action] TP_Action Click, y:%d\r\n", y);
#if (RAWLOG_SAMPLE_ENABLE == TRUE)
    raw_log_files_reset();
	motar_light_linear_working(200);
#endif
}

void setting_return_home_timeout_handler(void* usrdata)
{
    setting_ctx_v.state = ACTIVITY_SETTING_READY; 
#if (RAWLOG_SAMPLE_ENABLE != TRUE)
    setting_option_display_update(setting_ctx_v.head); 
#endif
    setting_dnd_underway = 0;
}


void setting_return_surface_timeout_handler(void* usrdata)
{
    setting_ctx_v.state = ACTIVITY_SETTING_READY; 
    ry_sched_msg_send(IPC_MSG_TYPE_DEV_UNBIND, 0, NULL);
}

void setting_dnd_on_action(uint32_t en)
{
//    setting_ctx_v.dnd_st = !setting_ctx_v.dnd_st;
//    ry_rawLog_info_save_enable_set(setting_ctx_v.dnd_st);
    
    if (setting_get_dnd_manual_status() == 0)
        setting_set_dnd_manual_status(1);
    else
        setting_set_dnd_manual_status(0);
        
	setting_dnd_underway = 1;
    // wms_msg_show_enable_set(!setting_ctx_v.dnd_st);
    setting_dnd_succ_display_update();
    
    ry_timer_start(app_setting_timer_id, 1000, setting_return_home_timeout_handler, NULL);
	motar_light_linear_working(200);    
}

void setting_unlock_on_action(uint32_t y)
{
    int old_unlock = get_surface_unlock_type();
    
    int new_unlock = old_unlock + 1;
    
    if (new_unlock == SURFACE_UNLOCK_NUM_MAX)
        new_unlock = 0;
    
    if (new_unlock == 0) // disable lock
        set_lock_type(0);
    if (old_unlock == 0) // enable lock
        set_lock_type(1);
    
    surface_ctx_v.unlock_type = new_unlock;
    set_surface_unlock_type(new_unlock);
}

void setting_stepcoef_on_action(uint32_t y)
{
    u8_t old_coef = get_step_distance_coef();
    
    u8_t new_coef = old_coef + 1;
    
    if (new_coef == 21)
        new_coef = 5;

    set_step_distance_coef(new_coef);
}

/* Unbind device */
void setting_reset_device_on_action(uint32_t y)
{
    LOG_INFO("[activity_setting], Unbind device.\r\n");
    //send find phone msg here
    setting_ctx_v.reset_confirm = 1;
    setting_reset_device_display_update();
    DEV_STATISTICS(user_replace_count);
    
    /* Disable touch */
    touch_disable();

    /* Do unbind */
    device_unbind_request();
    ryos_delay_ms(1000);
    setting_ctx_v.reset_confirm = 0;     
    ry_timer_start(app_setting_timer_id, 1000, setting_return_surface_timeout_handler, NULL);   
}

void setting_find_phone_on_action(uint32_t y)
{
	LOG_DEBUG("[setting_find_phone_on_action] TP_Action Click, y:%d\r\n", y);
}

void setting_reboot_device_on_action(uint32_t y)
{
    // LOG_DEBUG("[setting_reboot_device_on_action] TP_Action Click, y:%d\r\n", y);

    setting_ctx_v.reset_click ++;
    setting_reboot_device_display_update();
    if (setting_ctx_v.reset_click >= 1){
        //power_off_ctrl(1);
        DEV_STATISTICS(user_reset_count);
        setting_reboot_device_display_update();
        system_app_data_save();
        add_reset_count(MAGIC_RESTART_COUNT);
        ry_system_reset();
    }      
}


void setting_brightness_on_action(uint32_t y)
{
    uint32_t bright_setting = 0xff;
    if (y < 3){
        bright_setting = setting_ctx_v.sel_brightness;
    }
    else if ((y >= 3) && (y < 6)){
        bright_setting = setting_ctx_v.sel_brightness + 1;         
    }
    else if ((y >= 6) && (y < 8)){
        bright_setting = setting_ctx_v.sel_brightness + 2;         
    }
    
    if (bright_setting <= E_BRIGHTNESS_LOW){
        set_brightness_rate(bright_setting);
    }
    else{
        if (get_brightness_night() != 0){
            set_brightness_night(0);
        }
        else{
            set_brightness_night(1);        
        }
    }
    oled_brightness_update_by_time();
    uint8_t brightness = tms_oled_max_brightness_percent();
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);
    setting_brightness_display_update();
    // LOG_INFO("[setting_brightness_on_action], sel_brightness:%d, y:%d\r\n", \
        setting_ctx_v.sel_brightness, y);
}

uint8_t flashlight_evt = 0;
void setting_flashlight_on_action(uint32_t y)
{
    uint8_t brightness = 100;
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);    
    activity_setting.disableUntouchTimer = 1;
    wms_untouch_timer_stop();
    
    flashlight_evt = 1;
}

void setting_nfc_on_action(uint32_t y)
{   
    if (setting_ctx_v.sel_nfc == 0) {
        // disable backlight off
        activity_setting.disableUntouchTimer = 1;
        wms_untouch_timer_stop();        
        
        nfc_action = 1;    
        setting_nfc_display_update();
        ryos_delay_ms(100);
        
        ry_memset(nfc_status, 0, 4);
        ry_memset(g_test_nfc_id, 0, 11);

        int cnt = 0;
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_ON, NULL);
        while(ry_nfc_get_state() != RY_NFC_STATE_READER_POLLING) {
            ryos_delay_ms(100);
            if (cnt++ >= 50) {
                break;
            }
        }
        
        if (cnt >= 50) {
            nfc_action = 3;
        }
        else {
            nfc_action = 2;
            
            cnt = 0;
            while ((nfc_status[0]+nfc_status[1]+nfc_status[2]+nfc_status[3])==0) {
                ryos_delay_ms(100);
                if (cnt++ >= 50) {
                    nfc_action = 4;
                    break;
                }
            }
        }
        
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_OFF, NULL);
        motar_set_work(MOTAR_TYPE_STRONG_TWICE, 200);        
        
        // enable backlight off
        activity_setting.disableUntouchTimer = 0;
        wms_untouch_timer_start();
    }
    else if (setting_ctx_v.sel_nfc == 1) {
        
        if (ry_nfc_is_wired_enable()) {
            ry_nfc_se_channel_close();
        } else {
            ry_nfc_se_channel_open();
        }
        
        ryos_delay_ms(500);        
    }
    
    setting_nfc_display_update();
}

void setting_lcd_on_action(uint32_t y)
{
    u8_t old_lcd = setting_ctx_v.lcd_state;
    
    u8_t new_lcd = old_lcd == 0 ? 1 : 0;   
        
    setting_ctx_v.lcd_state = new_lcd;
    
    set_lcd_off_type(new_lcd);
}

void setting_alarm_on_action(uint32_t y)
{
    u8_t alarm_repeat = setting_ctx_v.alarm_repeat;
    
    u8_t alarm_repeat_new = alarm_repeat + 1;
    if (alarm_repeat_new >= 3)
        alarm_repeat_new = 0;
        
    setting_ctx_v.alarm_repeat = alarm_repeat_new;    
    set_alarm_snooze_interval(setting_alarm_repeat_value[alarm_repeat_new]);
}

void setting_sleep_on_action(uint32_t y)
{
    u8_t sleep_old = setting_ctx_v.sleep_off;
    
    u8_t sleep_new = sleep_old == 0 ? 1 : 0;   
        
    setting_ctx_v.sleep_off = sleep_new;
    
    set_sleep_monitor_state(sleep_new);
}

void setting_data_on_action(uint32_t y)
{
    u8_t data_old = setting_ctx_v.data_off;
    
    u8_t data_new = data_old == 0 ? 1 : 0;   
        
    setting_ctx_v.data_off = data_new;
    
    set_data_monitor_state(data_new);
}

int setting_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[setting_onProcessedTouchEvent] TP_Action Click, y:%d, setting_ctx_v.state: %d\r\n", \
                p->y, setting_ctx_v.state);

            if (setting_dnd_underway) {
                return consumed;
            }

            if (setting_ctx_v.state == ACTIVITY_SETTING_ACTION){
                if ((p->y < 8)){
                    
                    if (setting_ctx_v.sel_item == SETTING_ABOUT) {
                        if (setting_ctx_v.sel_about == 0) {
                            setting_ctx_v.state_battery = !(bool)setting_ctx_v.state_battery;
                            setting_about_display_update();
                        }
                    }
                    else {
                        setting_array[setting_ctx_v.sel_item].on_action(p->y);
                    }
                }                
                else{
                    
                    if (flashlight_evt == 1) {
                        activity_setting.disableUntouchTimer = 0;
                        uint8_t brightness = tms_oled_max_brightness_percent();
                        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);
                        wms_untouch_timer_start();
                        flashlight_evt = 0;
                    }
                    
                    setting_ctx_v.state = ACTIVITY_SETTING_READY;
                    setting_option_display_update(setting_ctx_v.head);
                    LOG_DEBUG("[setting_activity_back] TP_Action Click, y:%d, setting_ctx_v.state: %d\r\n", \
                        p->y, setting_ctx_v.state);
                    return consumed;
                }
            }

            if (setting_ctx_v.state == ACTIVITY_SETTING_READY){
                if ((p->y <= 3)){
                    setting_ctx_v.sel_item = setting_ctx_v.head;                    
                    if (setting_ctx_v.sel_item == SETTING_UNLOCK ||
                        setting_ctx_v.sel_item == SETTING_STEP_COEF ||
                        setting_ctx_v.sel_item == SETTING_LCD_DIMMING ||
                        setting_ctx_v.sel_item == SETTING_ALARM_REPEAT ||
                        setting_ctx_v.sel_item == SETTING_SLEEP_OFF ||
                        setting_ctx_v.sel_item == SETTING_DATA_OFF)
                    {
                        setting_array[setting_ctx_v.sel_item].on_action(p->y);
                        setting_option_display_update(setting_ctx_v.head);
                    }
                    else {
                        setting_array[setting_ctx_v.sel_item].display();
                        setting_ctx_v.state = ACTIVITY_SETTING_ACTION;
                    }
                }
                else if ((p->y > 3) && (p->y <= 7)){
                    setting_ctx_v.sel_item = setting_ctx_v.head + 1;    
                    if (setting_ctx_v.sel_item == SETTING_UNLOCK ||
                        setting_ctx_v.sel_item == SETTING_STEP_COEF ||
                        setting_ctx_v.sel_item == SETTING_LCD_DIMMING ||
                        setting_ctx_v.sel_item == SETTING_ALARM_REPEAT ||
                        setting_ctx_v.sel_item == SETTING_SLEEP_OFF ||
                        setting_ctx_v.sel_item == SETTING_DATA_OFF)
                    {
                        setting_array[setting_ctx_v.sel_item].on_action(p->y);
                        setting_option_display_update(setting_ctx_v.head);
                    }
                    else {                    
                        setting_array[setting_ctx_v.sel_item].display();
                        setting_ctx_v.state = ACTIVITY_SETTING_ACTION;
                    }
                }
                else if ((p->y >= 8)){
                    wms_activity_back(NULL);
                    LOG_DEBUG("[wms_activity_back] TP_Action Click, y:%d, setting_ctx_v.state: %d\r\n", \
                        p->y, setting_ctx_v.state);
                    return consumed;
                }
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG("[setting_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);        
            if (setting_ctx_v.state == ACTIVITY_SETTING_READY){
                setting_ctx_v.head ++;
                if (setting_ctx_v.head > SETTING_NUM_MAX - 2){
                    setting_ctx_v.head = SETTING_NUM_MAX - 2;
                }  
                setting_option_display_update(setting_ctx_v.head);
            }
            if (setting_ctx_v.state == ACTIVITY_SETTING_ACTION){
                if (setting_ctx_v.sel_item == SETTING_ABOUT){
                    
                    int itemCnt = sizeof(setting_about_str)/sizeof(setting_about_str[0]);
                    int itemCntMinus2 = itemCnt - 2;
                    
                    setting_ctx_v.sel_about ++;
                    if (setting_ctx_v.sel_about > itemCntMinus2){
                        setting_ctx_v.sel_about = itemCntMinus2;
                    }
                    setting_about_display_update();
                }
                else if (setting_ctx_v.sel_item == SETTING_BRIGHTNESS){
                    setting_ctx_v.sel_brightness += 2;
                    if (setting_ctx_v.sel_brightness > E_BRIGHTNESS_MAX - 1){
                        setting_ctx_v.sel_brightness = E_BRIGHTNESS_MAX - 1;
                    }
                    setting_brightness_display_update();
                }
#if NFC_TEST == 1                
                else if (setting_ctx_v.sel_item == SETTING_NFC){
                    
                    if (setting_ctx_v.sel_nfc < NFC_PAGE_COUNT - 1)
                        setting_ctx_v.sel_nfc++;
                    
                    setting_nfc_display_update();
                }
#endif
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            LOG_DEBUG("[setting_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);
            if (setting_ctx_v.state == ACTIVITY_SETTING_READY){
                setting_ctx_v.head --;
                if (setting_ctx_v.head < 0){
                    setting_ctx_v.head = 0;
                } 
                setting_option_display_update(setting_ctx_v.head);
            }
            if (setting_ctx_v.state == ACTIVITY_SETTING_ACTION){
                if (setting_ctx_v.sel_item == SETTING_ABOUT){
                    
                    if (setting_ctx_v.sel_about > 0)
                        setting_ctx_v.sel_about--;

                    setting_about_display_update();
                }
                else if (setting_ctx_v.sel_item == SETTING_BRIGHTNESS){
                    setting_ctx_v.sel_brightness -= 2;
                    if (setting_ctx_v.sel_brightness < 0){
                        setting_ctx_v.sel_brightness = 0;
                    }
                    setting_brightness_display_update();
                }
#if NFC_TEST == 1                
                else if (setting_ctx_v.sel_item == SETTING_NFC) {
                    
                    if (setting_ctx_v.sel_nfc > 0)
                        setting_ctx_v.sel_nfc--;
                                        
                    setting_nfc_display_update();
                }
#endif
            }
            break;

        default:
            break;
    }
    
    if (setting_ctx_v.state == ACTIVITY_SETTING_READY){
        setting_ctx_v.reset_click = 0;
    } 
    return consumed; 
}

int setting_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}

void send_hci_read_cmd(void)
{
    LOG_DEBUG("ready to send_hci_read_cmd\r\n");
    uint8_t temp[5];
    uint8_t* p = temp;
    UINT32_TO_BSTREAM(p, 0x00f00424);
    UINT8_TO_BSTREAM(p, 0x4);
    HciVendorSpecificCmd(0xFC20, 5, temp);
    LOG_DEBUG("send_hci_read_cmd done\r\n");    
}

u8_t cast_snooze_interval_value(u8_t value)
{
    u8_t ret = 0;
    u8_t snooze_val = get_alarm_snooze_interval();
    
    for (int i =0; i < sizeof(setting_alarm_repeat_value); i++)
    {
        if (setting_alarm_repeat_value[i] == snooze_val)
            ret = i;
    }
    
    return ret;
}

/**
 * @brief   Entry of the setting activity
 *
 * @param   None
 *
 * @return  setting_onCreate result
 */
ry_sts_t setting_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    setting_ctx_v.state = ACTIVITY_SETTING_READY;
    setting_ctx_v.reset_click = 0;
    setting_ctx_v.reset_confirm = 0;
    setting_ctx_v.sel_item = 0;
    setting_ctx_v.sel_about = 0;
    setting_ctx_v.sel_brightness = 0;
    setting_ctx_v.head = 0;
    setting_ctx_v.sleep_off = get_sleep_monitor_state();
    setting_ctx_v.data_off = get_data_monitor_state();
    setting_ctx_v.lcd_state = get_lcd_off_type();
    setting_ctx_v.alarm_repeat = cast_snooze_interval_value(get_alarm_snooze_interval());   
    
    ry_timer_parm timer_para;

    /* Create the timer */
    timer_para.timeout_CB = activity_setting_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "app_setting_timer";
    app_setting_timer_id = ry_timer_create(&timer_para);
	//ry_timer_start(app_setting_timer_id, 10000, find_phone_cb, NULL);

    if(setting_scrollbar == NULL){
        setting_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);
    }
    
    flashlight_evt = 0;
    
    // clear_buffer_black();
    // ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_setting, setting_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_setting, setting_onProcessedCardEvent);
    
    ret = setting_option_display_update(setting_ctx_v.head);

    //send_hci_read_cmd();

    return ret;
}


/**
 * @brief   API to exit setting activity
 *
 * @param   None
 *
 * @return  setting activity Destrory result
 */
ry_sts_t setting_onDestroy(void)
{
    ry_sts_t ret = RY_SUCC;
    ry_timer_delete(app_setting_timer_id);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_setting);
    wms_event_listener_del(WM_EVENT_CARD, &activity_setting);

    if(setting_scrollbar != NULL){
        scrollbar_destroy(setting_scrollbar);
        setting_scrollbar = NULL;
    }

    return ret;
}

/**
 * @brief   API to setting_get_dnd_status
 *
 * @param   None
 *
 * @return  dnd_status - 0: can be disturbed, else: do not disturb
 */
uint8_t setting_get_dnd_status(void)
{
#if (RAWLOG_SAMPLE_ENABLE != TRUE)
    static u8_t flag = 0;
	if(flag == 0){
		flag = 1;
	}else{
		//setting_set_dnd_status(current_time_is_dnd_status());
        get_dnd_mode();
	}
#endif
    return setting_ctx_v.dnd_st;
}


/**
 * @brief   API to setting_set_dnd_status
 *
 * @param   dnd_status - 0: can be disturbed, 1: do not disturb
 *
 * @return  None 
 */
void setting_set_dnd_status(u8_t en)
{
	setting_ctx_v.dnd_st = en;
}

u8_t setting_get_dnd_manual_status(void)
{
    return setting_ctx_v.dnd_manual_mode;
}

void setting_set_dnd_manual_status(u8_t en)
{
    setting_ctx_v.dnd_manual_mode = en;
}
