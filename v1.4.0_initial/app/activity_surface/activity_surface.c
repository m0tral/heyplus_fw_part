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
#include "ry_ble.h"
#include "board_control.h"
#include "ry_font.h"


/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "device_management_service.h"
#include "window_management_service.h"
#include "card_management_service.h"
#include "surface_management_service.h"
#include "data_management_service.h"

#include "activity_surface.h"
#include "activity_card.h"
#include "activity_weather.h"
#include "activity_menu.h"
#include "activity_qrcode.h"
#include "activity_setting.h"

#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "led.h"

/* resources */
#include "gui_img.h"
#include "gui_msg.h"
#include "motar.h"
#include "ry_statistics.h"
#include "board_control.h"
#include "icon_lock.h"

/*********************************************************************
 * CONSTANTS
 */ 
#define SURFACE_AUTO_NEXT_ACTIVITY     0
#define SURFACE_IMG_RES_V3             0
#define COLOR_BRIGHT_GREEN_RGB          (HTML2COLOR(0x15D080))
#define COLOR_BRIGHT_GREEN             (HTML2COLOR(0x80D015))
#define SURFACE_UNLOCK_ANIMATE         0

#if (SURFACE_DISPLAY_SIX == 0)
    #define SURFACE_RED_CURVES             0
    #define SURFACE_GREEN_CURVES           0
    #define SURFACE_COLOFUL                0
#endif

#define TEMPERATURE_CHECK(x)    (((-100 < x) && (x < 100))?(true):(false))

/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    SURFACE_OFF,
    SURFACE_ON,
    SURFACE_DARKER,
    SURFACE_OPTION,    
} surface_status_t;


/*********************************************************************
 * VARIABLES
 */
extern activity_t app_ui_activity;
extern activity_t activity_hrm;
extern activity_t activity_weather;
extern activity_t activity_card;
extern activity_t activity_data;
extern activity_t activity_setting;
extern activity_t activity_mijia;
extern activity_t activity_sfc_option;
extern activity_t activity_alarm;

extern int debug_false_color;

activity_t* activity_auto_next = &activity_alarm;

ry_surface_ctx_t surface_ctx_v = {
    .list = SURFACE_RED_ARRON,
    .runtime_update_enable = 1,    
};

static u32_t app_surface_touch_timer_id;
ryos_thread_t* unlock_thread_handle = NULL;
ry_queue_t *   unlock_queue = NULL;

ryos_mutex_t* surface_mutex = NULL;


activity_t activity_surface = {
    .name      = "surface",
    .onCreate  = surface_onCreate,
    .onDestroy = surface_onDestroy,
#if 1
    .disableUntouchTimer = 0,
#endif
    .priority = WM_PRIORITY_L,
};

const char* const text_slide_to_unlock1 = "Вверх для";
const char* const text_slide_to_unlock2 = "разблок.";

const char* const text_code_unlock1 = ">> код <<";
//const char* const text_code_unlock2 = "код";


static const char* const week_string_chs[] ={
    "ВС",
    "ПН", 
    "ВТ",
    "СР",
    "ЧТ",
    "ПТ",
    "СБ",
};

static const char* const unlock_code[] = {
    "unlock_code1.bmp",
    "unlock_code2.bmp",
    "unlock_code3.bmp",
};

static const char* const unlock_code_active[] = {
    "unlock_code1a.bmp",
    "unlock_code2a.bmp",
    "unlock_code3a.bmp",
};

static const raw_png_descript_t icon_step = {
    .p_data = step_png,
    .width = 17,
    .height = 15,
    .fgcolor = White,
};
static const raw_png_descript_t icon_cal = {
    .p_data = cal_png,
    .width = 14,
    .height = 18,
    .fgcolor = White,
};
static const raw_png_descript_t icon_heart = {
    .p_data = heart_png,
    .width = 17,
    .height = 16,
    .fgcolor = White,
};
static const raw_png_descript_t icon_step1 = {
    .p_data = step1_png,
    .width = 19,
    .height = 18,
    .fgcolor = White,
};
static const raw_png_descript_t icon_status_00_msg = {
    .p_data = m_status_00_msg_bmp,
    .width = 18,
    .height = 18,
    .fgcolor = White,
};
static const raw_png_descript_t icon_status_01_charging = {
    .p_data = m_status_01_charging_bmp,
    .width = 18,
    .height = 18,
    .fgcolor = HTML2COLOR(0xD0021B),
};
static const raw_png_descript_t icon_status_02_charging = {
    .p_data = m_status_02_charging_bmp,
    .width = 18,
    .height = 18,
    .fgcolor = HTML2COLOR(0x17C778),
};
static const raw_png_descript_t icon_battery = {
    .p_data = battery_bmp,
    .width = 30,
    .height = 18,
    .fgcolor = HTML2COLOR(0x979797),
};
static const raw_png_descript_t icon_status_sleep = {
    .p_data = m_status_sleep_bmp,
    .width = 18,
    .height = 18,
    .fgcolor = HTML2COLOR(0x979797),
};
static const raw_png_descript_t icon_status_dnd = {
    .p_data = m_status_dnd_bmp,
    .width = 18,
    .height = 18,
    .fgcolor = HTML2COLOR(0x979797),
};
static const raw_png_descript_t icon_status_offline = {
    .p_data = m_status_offline_bmp,
    .width = 18,
    .height = 18,
    .fgcolor = HTML2COLOR(0x979797),
};
static const raw_png_descript_t icon_status_locked = {
    .p_data = m_status_locked_bmp,
    .width = 16,
    .height = 16,
    .fgcolor = HTML2COLOR(0x979797),
};
static const raw_png_descript_t icon_alarm = {
    .p_data = m_alarm_bmp,
    .width = 20,
    .height = 20,
    .fgcolor = HTML2COLOR(0x979797),
};
static const raw_png_descript_t icon_time_am = {
    .p_data = m_time_am_png,
    .width = 29,
    .height = 18,
    .fgcolor = HTML2COLOR(0xFFFFFF),
};
static const raw_png_descript_t icon_time_pm = {
    .p_data = m_time_pm_png,
    .width = 29,
    .height = 18,
    .fgcolor = HTML2COLOR(0xFFFFFF),
};

static ry_sts_t surface_display_header(int source, surface_header_t const* p_header);

static int last_min;

u32_t surface_timeout_timer_id;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void surface_timeout_handler(void* arg);

/**
 * @brief   API to set surface display option
 *
 * @param   option: user surface display option
 *
 * @return  set_surface_option result
 */
ry_sts_t set_surface_option(surface_list_t option)
{
    ry_sts_t ret = RY_SUCC;
    if (option < SURFACE_OPTION_MAX){
        surface_ctx_v.list = option;
    }
    else{
        ret = RY_ERR_WRITE;
    }
    return ret;
}

/**
 * @brief   API to get surface display option
 *
 * @param   None
 *
 * @return  get_surface_option result
 */
surface_list_t get_surface_option(void)
{
    return surface_ctx_v.list;
}

void app_surface_untouch_timeout_handler(void* arg)
{
    // LOG_DEBUG("[app_surface_untouch_timeout_handler]: transfer timeout.\n");
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
    surface_ctx_v.state = SURFACE_OFF;
    ry_timer_stop(app_surface_touch_timer_id);
}    

void app_surface_idle_timeout_handler(void* arg)
{
    // LOG_DEBUG("[app_surface_idle_timeout_handler]: transfer timeout.\n");

#if (SURFACE_DISPLY_DEMO)
    return;     //continue display
#endif

#if (SURFACE_AUTO_NEXT_ACTIVITY)
    wms_activity_jump(activity_auto_next, NULL);
#else
    surface_ctx_v.state = SURFACE_DARKER;
    ry_timer_start(app_surface_touch_timer_id, 5000, app_surface_untouch_timeout_handler, NULL);    
#endif
}

void app_surface_untouch_timer_start(void)
{
#if (SURFACE_AUTO_NEXT_ACTIVITY)
    if (app_surface_touch_timer_id) {
        ry_timer_start(app_surface_touch_timer_id, 1000, app_surface_idle_timeout_handler, NULL);
    }
#endif
}

void app_surface_untouch_timer_stop(void)
{
#if 0
   if (app_surface_touch_timer_id) {
       ry_timer_stop(app_surface_touch_timer_id);
   }
#endif
}

void unlock_alert_info_display(void)
{
    LOG_DEBUG("[unlock_alert_info_display] begin.\r\n");
    
    clear_buffer_black();
    
    uint8_t w, h;
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, 50, &w, &h, d_justify_center);

    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock13.bmp", \
                            0, 70, &w, &h, d_justify_center);

    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, 90, &w, &h, d_justify_center);

    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock1.bmp", \
                            0, 116, &w, &h, d_justify_center);
                            
    gdispFillStringBox( 0, 
                        170, 
                        font_roboto_20.width, font_roboto_20.height,
                        (char *)text_slide_to_unlock1, 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyCenter
                        );

    gdispFillStringBox( 0, 
                        200, 
                        font_roboto_20.width, font_roboto_20.height,
                        (char *)text_slide_to_unlock2, 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyCenter
                        );

    // ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);    
    LOG_DEBUG("[unlock_alert_info_display] end.\r\n");    
}

void fast_menu_display(void)
{
    clear_buffer_black();
    
    uint8_t w, h;
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"stopwatch_start.bmp", \
                            0, 5, &w, &h, d_justify_center);
    
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"stopwatch_stop.bmp", \
                            0, 65, &w, &h, d_justify_center);
    
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"stopwatch_continue.bmp", \
                            0, 125, &w, &h, d_justify_center);

    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"stopwatch_clear.bmp", \
                            0, 185, &w, &h, d_justify_center);
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

uint8_t touch_action = 0;
uint8_t touch_position = 0;
uint32_t touch_time_event = 0;
uint32_t touch_time_start = 0;

uint8_t code_current = 0;
uint8_t code_sequence[] = {1,3,2};
uint8_t code_sequence_len = 3;
uint8_t code_digit = 0;
uint8_t code_success = 0;

void processCodeEvent() {
    
    if (touch_time_start == 0 && touch_time_event == 0)
        return;
    
    if (touch_action == 1) {
    
        code_current = 0;
        if (touch_position <= 3)            
            code_current = 1;
        if (touch_position > 3 && touch_position <= 5)
            code_current = 2;
        if (touch_position > 5)
            code_current = 3;
        
        // enter first digit
        if (touch_time_start == 0) {
            code_digit = 0;
            touch_time_start = touch_time_event;
            if (code_current == code_sequence[0])
                code_digit = 1;
        }
        else {
            // code enter timeout
            if ((touch_time_event - touch_time_start) < 40){
                // just skip
            }
            else if ((touch_time_event - touch_time_start) > 3000){
                touch_time_start = 0;
                code_digit = 0;
            }
            else {
                if (code_current == code_sequence[code_digit]){
                    touch_time_start = touch_time_event;
                    code_digit++;
                    
                    if (code_digit > (code_sequence_len - 1)) {
                        touch_time_start = 0;
                        code_success = 1;
                    }
                } else {
                    code_digit = 0;
                }
            }
        }
    }
}

void setRectRound(int max_x,int max_y, int min_x, int min_y, int round, int color)
{   
    int x, y;
    for(x = min_x; x <= max_x; x++){
        for(y = min_y; y <= max_y; y++){
            if ((x < min_x + round || x > max_x - round)
                && ((y > min_y + round) || (y < max_y - round)))
                ;
            else
                setDip(x, y, color);
        }
    }
}

void unlock_code_info_display_animate(uint8_t stage)
{
    uint8_t w, h;    
    const u8_t unlock_y[] = {30, 100, 170}; // 3 buttons
    const u8_t btn_height = 65;
    const u8_t btn_margin = 10;    
    
    if (touch_action != 1)
        return;
    
    clear_buffer_black();

    int color = (touch_position <= 3) ? White : Gray;
    const char* img = color == White ? unlock_code_active[0] : unlock_code[0];
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)img, btn_margin, unlock_y[0], &w, &h, d_justify_center);
    
    color = (touch_position > 3 && touch_position <= 5) ? White : Gray;
    img = color == White ? unlock_code_active[1] : unlock_code[1];
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)img, btn_margin, unlock_y[1], &w, &h, d_justify_center);
    
    color = (touch_position > 5 && touch_position < 9) ? White : Gray;
    img = color == White ? unlock_code_active[2] : unlock_code[2];
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)img, btn_margin, unlock_y[2], &w, &h, d_justify_center);

//    char tmp[11] = {0};
//    
//    sprintf(tmp, "%d %d %d", surface_lock_status_get(), code_digit, code_current);
//                            
//    gdispFillStringBox( 0, 
//                        35, 
//                        font_roboto_20.width,
//                        font_roboto_20.height,
//                        tmp, 
//                        (void*)font_roboto_20.font, 
//                        White, Black,
//                        justifyCenter
//                      );

    int x1 = 22;
    int x2 = 22 + 18;
    int y1 = 7;
    int y2 = 7 + 18;
    for (int i=0; i < code_sequence_len; i++) {        
        color = (i < code_digit) ? Green : Gray;
        setRectRound(x2 + (i * 30), y2, x1 +  (i * 30), y1, 1, color);
    }
												
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void unlock_alert_info_display_animate(uint8_t stage)
{
    // LOG_DEBUG("[unlock_alert_info_display] begin.stage:%d\r\n", stage);
    uint8_t w, h;
    const u8_t unlock_y[] = {40, 65, 90, 115};
    #define OFFSET_UNLOCK_ARROW     4
    clear_buffer_black();
    
    switch (stage) {
         case 1:
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock1.bmp", \
                            0, unlock_y[0] - OFFSET_UNLOCK_ARROW, &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, unlock_y[1], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, unlock_y[2], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, unlock_y[3], &w, &h, d_justify_center);
            break;    

        case 2:
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, unlock_y[0], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock1.bmp", \
                            0, unlock_y[1] - OFFSET_UNLOCK_ARROW, &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, unlock_y[2], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, unlock_y[3], &w, &h, d_justify_center);
            break;

        case 3:
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, unlock_y[0], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, unlock_y[1], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock1.bmp", \
                            0, unlock_y[2] - OFFSET_UNLOCK_ARROW, &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock14.bmp", \
                            0, unlock_y[3], &w, &h, d_justify_center);
            break;

        case 4:
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, unlock_y[0], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, unlock_y[1], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock12.bmp", \
                            0, unlock_y[2], &w, &h, d_justify_center);
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"up_unlock1.bmp", \
                            0, unlock_y[3], &w, &h, d_justify_center);
            break;

        default:
            break;
    }

                            
    gdispFillStringBox( 0, 
                        170, 
                        font_roboto_20.width, font_roboto_20.height,
                        (char *)text_slide_to_unlock1, 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,  
                        Black,
                        justifyCenter
                        );

    gdispFillStringBox( 0, 
                        200, 
                        font_roboto_20.width, font_roboto_20.height,
                        (char *)text_slide_to_unlock2, 
                        (void*)font_roboto_20.font, 
                        COLOR_BRIGHT_GREEN_RGB,
                        Black,
                        justifyCenter
                        );
												
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    // LOG_DEBUG("[unlock_alert_info_display] end.stage:%d\r\n", stage);    
}

void unlock_display_animate_by_type(surface_unlock_type_t unlock_type, uint8_t stage)
{
    switch (unlock_type) {
        case SURFACE_UNLOCK_CODE:
            unlock_code_info_display_animate(stage);        
            break;
        case SURFACE_UNLOCK_SWIPE:
            unlock_alert_info_display_animate(stage);
            break;
        case SURFACE_UNLOCK_NONE:
        case SURFACE_UNLOCK_NUM_MAX:
            break;
    }
}

uint8_t surface_lock_status_get(void)
{
    return surface_ctx_v.lock_st;
}

void init_code_lock() {
    code_success = 0;
    code_digit = 0;
    touch_time_start = 0;
    touch_position = 0;
}

void surface_timeout_handler(void* arg)
{
    wms_window_lock_status_set(0);
    stop_unlock_alert_animate();
    //surface_ctx_v.lock_st = E_LOCK_OUT;    
    
    init_code_lock();    
    
    ryos_mutex_lock(surface_mutex);
    activity_surface_on_event(SURFACE_DISPLAY_FROM_CREATE);
    ryos_mutex_unlock(surface_mutex);
}

void stop_fast_menu_timer(void)
{
    if (surface_ctx_v.press_hold_timer_id != 0) {
        
        surface_ctx_v.press_hold_start = 0;
        surface_ctx_v.press_hold_end = 0;
                
        ry_timer_stop(surface_ctx_v.press_hold_timer_id);        
        ry_timer_delete(surface_ctx_v.press_hold_timer_id);
        
        surface_ctx_v.press_hold_timer_id = 0;        
    }
}

void surface_fast_menu_handler(void* arg)
{
#warning temporary turn off fast menu    
    return;
    
    bool activate = false;
    
    if (surface_ctx_v.press_hold_start == 0)
        return;
    
    if (surface_ctx_v.press_hold_end > 0) {
        if (ry_hal_calc_ms(surface_ctx_v.press_hold_end - surface_ctx_v.press_hold_start) > FAST_MENU_ACTIVATE_INTERVAL) {
            activate = true;
        } else {
            stop_fast_menu_timer();
            return;
        }
    } else {    
        if (ry_hal_calc_ms(ry_hal_clock_time() - surface_ctx_v.press_hold_start) > FAST_MENU_ACTIVATE_INTERVAL)
            activate = true;
    }
    
    if (activate) {
        stop_fast_menu_timer();
        
        if (surface_ctx_v.display_mode == MAIN)
        {
            surface_ctx_v.display_mode = FAST_MENU;
            surface_onRTC_runtime_enable(0);        
            fast_menu_display();                
        }
    }
    else {    
        ry_timer_start(surface_ctx_v.press_hold_timer_id, 200, surface_fast_menu_handler, NULL);
    }
}

void start_fast_menu_timer(void)
{
    if (surface_ctx_v.press_hold_timer_id == 0) {
        ry_timer_parm timer_para;
        timer_para.timeout_CB = surface_fast_menu_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "surface_fast_menu_timer";
        surface_ctx_v.press_hold_timer_id = ry_timer_create(&timer_para);
    }
    
    ry_timer_start(surface_ctx_v.press_hold_timer_id, 200, surface_fast_menu_handler, NULL);
}

int surface_onRawTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    
    tp_event_t *p = (tp_event_t*)msg->data;    
    
#warning lock surface display for unbound device
    if(QR_CODE_DEBUG || is_bond()){
        ;// bond mode
    }
    else{
        wms_activity_replace(&activity_qrcode, NULL);
        return consumed;
    }
    
    // if lock display activated
    // process key code events
    if (surface_ctx_v.lock_st == E_LOCK_READY)
    {    
        switch (p->action) {
            case TP_ACTION_UP:
                touch_action = 1;
                touch_position = p->y;
                touch_time_event = ry_hal_calc_ms(p->t);
                break;
            case TP_ACTION_DOWN:
                touch_action = 2;
                touch_position = p->y;
                touch_time_event = ry_hal_calc_ms(p->t);
                break;
            case TP_ACTION_MOVE:
                //touch_action = 3;
                //touch_position = p->y;
                break;
            default:
                touch_action = 0x0F;
                touch_position = 0x0F;
                break;
        }

        processCodeEvent();

        if (code_success) {
            if (surface_timeout_timer_id) {
                ry_timer_stop(surface_timeout_timer_id);
                ry_timer_start(surface_timeout_timer_id, 300, surface_timeout_handler, NULL);
            }
        }
    }
    else {
        switch (p->action)
        {
            case TP_ACTION_UP:
                surface_ctx_v.press_hold_end = p->t;
                break;
            case TP_ACTION_DOWN:
                if (p->y < 8) {
                    surface_ctx_v.press_hold_start = p->t;
                    surface_ctx_v.press_hold_end = 0;
                    start_fast_menu_timer();
                }
                break;
            default:
                break;
        }
    }
    
    return consumed;        
}

int surface_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    u32_t msg_num;
    static u32_t unlock_time_start;

    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;

#warning lock surface display for unbound device
    if(QR_CODE_DEBUG || is_bond()){
        ;// bond mode
    }
    else{
        wms_activity_replace(&activity_qrcode, NULL);
        goto exit;
    }

    if (surface_ctx_v.lock_st == E_LOCK_IN) {
        unlock_animate_thread_setup();
    }
    //LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action:%d, y:%d, state %d, lock_st:%d\r\n", \
                p->action, p->y, surface_ctx_v.state, surface_ctx_v.lock_st); 
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            
            if (surface_ctx_v.lock_st == E_LOCK_IN) {
                surface_ctx_v.lock_st = E_LOCK_READY;
            }
            else if (surface_ctx_v.lock_st == E_LOCK_READY){
                start_unlock_alert_animate();
            }
            else if (surface_ctx_v.display_mode == FAST_MENU)
            {
                if (p->y >= 8) {
                    surface_ctx_v.display_mode = MAIN;
                    surface_onRTC_runtime_enable(1);
                    
                    ryos_mutex_lock(surface_mutex);
                    activity_surface_on_event(SURFACE_DISPLAY_FROM_CREATE);
                    ryos_mutex_unlock(surface_mutex);                    
                }
            }

            break;
            
        case TP_PROCESSED_ACTION_EXTRA_LONG_TAP:
            
//            if (surface_ctx_v.display_mode == MAIN)
//            {
//                surface_ctx_v.display_mode = FAST_MENU;
//                surface_onRTC_runtime_enable(0);        
//                fast_menu_display();                
//            }
        
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
                
            if (is_lock_enable() && surface_ctx_v.unlock_type == SURFACE_UNLOCK_SWIPE) {
                stop_unlock_alert_animate();
            }
        
            if (get_gui_state() == GUI_STATE_OFF) {
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                return consumed;
            }
            
            if (surface_ctx_v.display_mode == FAST_MENU)
                return consumed;
            
            stop_fast_menu_timer();            

            if (is_lock_enable() == 0){
                menu_index_sequence_reset();
                wms_activity_jump(&activity_menu, NULL);
                goto exit;
            }
            else {
                
                if (surface_ctx_v.unlock_type == SURFACE_UNLOCK_CODE) {
                
                    if (surface_ctx_v.lock_st == E_LOCK_IN) {
                        surface_ctx_v.lock_st = E_LOCK_READY;
                    }
                    else if (surface_ctx_v.lock_st == E_LOCK_READY){
                        start_unlock_alert_animate();
                    }
                    else if (surface_ctx_v.lock_st == E_LOCK_OUT){
                        if (ry_hal_calc_ms(ry_hal_clock_time() - unlock_time_start) >= 200){
                            menu_index_sequence_reset();
                            wms_activity_jump(&activity_menu, NULL);
                            goto exit;
                        }
                    }
                }
                else {
                    if (surface_ctx_v.lock_st == E_LOCK_OUT){
                        if (ry_hal_calc_ms(ry_hal_clock_time() - unlock_time_start) >= 200){
                            menu_index_sequence_reset();
                            wms_activity_jump(&activity_menu, NULL);
                            goto exit;
                        }
                    }                
                
                    if (surface_ctx_v.lock_st != E_LOCK_OUT){
                        surface_ctx_v.lock_st = E_LOCK_OUT;

                        wms_window_lock_status_set(0);

                        ryos_mutex_lock(surface_mutex);
                        activity_surface_on_event(SURFACE_DISPLAY_FROM_CREATE);
                        ryos_mutex_unlock(surface_mutex);
                        
                        surface_ctx_v.animate_play_unlock = 239;
                        unlock_time_start = ry_hal_clock_time();
                    }
                    else {
                        unlock_time_start = 0;                    
                    }
                }
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
                        
            if (get_gui_state() == GUI_STATE_OFF) {
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                return consumed;
            }
            
            if (surface_ctx_v.display_mode == FAST_MENU)
                return consumed;            
            
            stop_fast_menu_timer();            

            if ((surface_ctx_v.lock_st == E_LOCK_OUT) || (is_lock_enable() == 0)){
                wms_activity_jump(&drop_activity, (void *)(DROP_START_COVER_OFFSET));
                goto exit;
            }
            else if (surface_ctx_v.lock_st == E_LOCK_IN){
                surface_ctx_v.lock_st = E_LOCK_READY;
            }
            else if (surface_ctx_v.lock_st == E_LOCK_READY){
                start_unlock_alert_animate();
            }
            break;

        default:
            break;
    }
    surface_ctx_v.state = SURFACE_ON;
  
    app_surface_untouch_timer_start();

exit:    
    return consumed; 
}

void surface_onRTC_runtime_enable(uint32_t enable)
{
    surface_ctx_v.runtime_update_enable = enable;
}

uint32_t surface_runtime_enable_get(void)
{
    return surface_ctx_v.runtime_update_enable;
}

int surface_onRTCEvent(ry_ipc_msg_t* msg)
{
    // LOG_DEBUG("[surface_onRTCEvent]surface_ctx_v.lock_st:%d\r\n", surface_ctx_v.lock_st);
    ryos_mutex_lock(surface_mutex);
    if (surface_ctx_v.state == SURFACE_ON)
    {
        if ((GUI_STATE_ON == get_gui_state()) || surface_ctx_v.runtime_update_enable)
        {
            activity_surface_on_event(SURFACE_DISPLAY_FROM_RTC);
        }
    }
    ryos_mutex_unlock(surface_mutex);    
    // LOG_DEBUG("[surface_onRTCEvent] donesurface_ctx_v.lock_st:%d\r\n", surface_ctx_v.lock_st);
    return 1;
}


int surface_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    ryos_mutex_lock(surface_mutex);    
    ryos_mutex_unlock(surface_mutex);
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}


int surface_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    ryos_mutex_lock(surface_mutex);    
    ryos_mutex_unlock(surface_mutex);
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}


int surface_onSurfaceServiceEvent(ry_ipc_msg_t* msg)
{
    // LOG_DEBUG("[surface_onSurfaceServiceEvent]surface_ctx_v.lock_st:%d\r\n", surface_ctx_v.lock_st);
    ryos_mutex_lock(surface_mutex);
    if ((msg->len == sizeof(uint32_t)) && (IPC_MSG_TYPE_SURFACE_UPDATE_STEP == *(uint32_t*)msg->data))
    {
        if ((GUI_STATE_ON == get_gui_state()) 
            && (surface_ctx_v.animate_play_lock == 0))
        {
            activity_surface_on_event(SURFACE_DISPLAY_FROM_CREATE);
        }
    }
    else
    {
        activity_surface_on_event(SURFACE_DISPLAY_FROM_CREATE);
    }
    ryos_mutex_unlock(surface_mutex);
    // LOG_DEBUG("[surface_onSurfaceServiceEvent]done surface_ctx_v.lock_st:%d\r\n", surface_ctx_v.lock_st);
    return 1;
}

void start_unlock_alert_animate(void)
{
    u32_t msg = E_MSG_CMD_START_LOCK;
    ryos_queue_send(unlock_queue , &msg, sizeof(msg));
}

void stop_unlock_alert_animate(void)
{
    u32_t msg = E_MSG_CMD_STOP_LOCK;
    u32_t lock_done = 0;
    //LOG_INFO("[lock_display_task]: animate check:%d, surface_ctx_v.lock_st:%d\n", \
        surface_ctx_v.animate_lock_finished, surface_ctx_v.lock_st);
    
    if (surface_ctx_v.animate_lock_finished == 1){
        surface_ctx_v.animate_play_lock = 0;
    }
    else{
        while(lock_done == 0){
            if (surface_ctx_v.animate_lock_finished == 1){
                surface_ctx_v.animate_play_lock = 0;
                lock_done = 1;
                break;
            }
            ryos_delay_ms(30);
            // LOG_INFO("[lock_display_task]: animate not finished:%d\n");
        }
    }

    uint32_t destrory = 0;
    if (unlock_thread_handle != NULL){
        while (destrory == 0){
            if (surface_ctx_v.animate_finished){
                ryos_thread_delete(unlock_thread_handle);
                if (unlock_queue != NULL){
                    ryos_semaphore_delete(unlock_queue);
                    unlock_queue = NULL;
                }
                unlock_thread_handle = NULL;
                destrory = 1;
            }
            //LOG_DEBUG("[surface_onDestrory]destrory:%d, surface_ctx_v.animate_finished:%d, st:%d\n", \
                destrory, surface_ctx_v.animate_finished, surface_ctx_v.lock_st);
            ryos_delay_ms(30);
        }
    }
    surface_ctx_v.lock_st = (wms_window_lock_status_get() != 0) ? E_LOCK_IN : E_LOCK_OUT;
}

int unlock_display_task(void* arg)
{
    ry_sts_t status = RY_SUCC;
    img_pos_t pos;

    pos.x_start = 0;
    pos.x_end = 119;

    if(surface_mutex == NULL){
        ry_sts_t ret = ryos_mutex_create(&surface_mutex);
        if (RY_SUCC != ret) {
            
        } 
    }

    static u8_t cnt;
    while (1) {
        u32_t msg = 0;
        ryos_queue_recv(unlock_queue , &msg, RY_OS_NO_WAIT); 

        ryos_mutex_lock(surface_mutex);
        if (msg == E_MSG_CMD_START_UNLOCK){
            surface_ctx_v.animate_play_unlock = 239;
        }       
        if (msg == E_MSG_CMD_START_LOCK){
            surface_ctx_v.animate_play_lock = 4;
        }
        if (msg == E_MSG_CMD_STOP_LOCK){
            surface_ctx_v.animate_play_lock = 0;
        }    

        if (surface_ctx_v.animate_play_lock > 0){
            surface_ctx_v.animate_finished = 0;
#if 1
            surface_ctx_v.animate_lock_finished = 0;
            if (surface_ctx_v.animate_play_lock > 0){
                if (surface_ctx_v.animate_play_lock > 4){
                    surface_ctx_v.animate_play_lock = 4;
                }
                
                unlock_display_animate_by_type(surface_ctx_v.unlock_type, surface_ctx_v.animate_play_lock);

                if (surface_ctx_v.animate_play_lock <= 1){
                    surface_ctx_v.animate_play_lock = 5;
                }
                surface_ctx_v.animate_play_lock --;
            }
            surface_ctx_v.animate_lock_finished = 1;
#else            
            pos.y_start = surface_ctx_v.animate_play_lock - 1;
            pos.y_end = surface_ctx_v.animate_play_lock + 8;         
            pos.buffer = frame_buffer + pos.y_start * 360;  
            if (surface_ctx_v.animate_play_lock < 239){
                surface_ctx_v.animate_play_lock += 10;
            }
            else{
                surface_ctx_v.animate_play_lock = 0;
            }
            if (pos.y_start > 239)
                pos.y_start = 239;
            if (pos.y_end > 239)
                pos.y_end = 239;    
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
#endif            
        }

#if SURFACE_UNLOCK_ANIMATE        
        // LOG_DEBUG("[unlock_display_task]: begin, cnt:%d, surface_ctx_v.animate_finished:%d\n", cnt, surface_ctx_v.animate_finished);
        if (surface_ctx_v.animate_play_unlock > 0) {
            surface_ctx_v.animate_play_lock = 0;            
            surface_ctx_v.animate_finished = 0;
            pos.y_start = surface_ctx_v.animate_play_unlock - 9;
            pos.y_end = surface_ctx_v.animate_play_unlock;         
            pos.buffer = frame_buffer + pos.y_start * 360;  
            if (surface_ctx_v.animate_play_unlock > 20){
                surface_ctx_v.animate_play_unlock -= 20;
            }
            else{
                surface_ctx_v.animate_play_unlock = 0;
            }
            if (pos.y_start > 239)
                pos.y_start = 0;
            if (pos.y_end > 239)
                pos.y_end = 0;    
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
        } 
#endif
        ryos_mutex_unlock(surface_mutex);
        surface_ctx_v.animate_finished = 1;
        // LOG_DEBUG("[unlock_display_task]: end cnt:%d, surface_ctx_v.animate_finished:%d\n", cnt ++, surface_ctx_v.animate_finished);
        
        if (surface_ctx_v.animate_play_lock > 0) {
            ryos_delay_ms(100);
        }
        else{
            ryos_delay_ms(200);
        }
    }
}

void unlock_animate_thread_setup(void)
{    
    init_code_lock();
    
    ryos_threadPara_t para;
    ry_sts_t status = RY_SUCC;

    strcpy((char*)para.name, "unlock_task");
    para.stackDepth = 1024;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 3;
    if (unlock_thread_handle == NULL) {
        ryos_thread_create(&unlock_thread_handle, unlock_display_task, &para);
    }
    if (unlock_queue == NULL) {
        status = ryos_queue_create(&unlock_queue, 5, sizeof(u32_t));
    }
    if (RY_SUCC != status) {
        LOG_ERROR("[animate_thread_setup]: Queue Create Error status: %d\r\n", status);
        while(1);
    }
    //LOG_DEBUG("[unlock_animate_thread_setup]:surface_ctx_v.lock_st:%d, freeHeap:%d\n", \
        surface_ctx_v.lock_st, ryos_get_free_heap());
}

static void disp_rawLog_status(void)
{
    uint32_t g_time_raw = 0;
    extern uint32_t rawLog_time_start;
    extern uint32_t rawLog_time_stop;
    extern uint32_t rawLog_size_total;
    extern uint32_t rawLog_overflow;
    extern u32_t rawLog_info_save_en;
    extern u32_t g_fs_free_size;
    
    if (rawLog_overflow){
        gdispFillString(60, 26, (const char *)"Full", font_roboto_20.font, Red,  Black);        
    }
    else if (rawLog_info_save_en){
        gdispFillString(56, 26, (const char *)"inRaw", font_roboto_20.font, COLOR_BRIGHT_GREEN,  Black);
    }
    else{
        gdispFillString(56, 26, (const char *)"noRaw", font_roboto_20.font, White,  Black);        
    }
    
    // time
    if (rawLog_info_save_en && !rawLog_overflow){
        rawLog_time_stop = ry_hal_clock_time();
    }
    g_time_raw = ry_hal_calc_second(rawLog_time_stop - rawLog_time_start);
    
    if (!rawLog_info_save_en){
        g_time_raw = 0;
    }

    uint32_t time_second = (g_time_raw % 60);
    uint32_t time_min = (g_time_raw / 60) % 60;
    uint32_t time_hour = (g_time_raw / 3600);
    
    uint8_t timer_str[24];
    sprintf((char *)timer_str, "%02d:%02d:%02d", time_hour, time_min, time_second);

    gdispFillString(2, 140, (const char *)timer_str, font_roboto_20.font, COLOR_BRIGHT_GREEN,  Black);

    uint32_t storage = (rawLog_size_total >> 10);
    uint32_t storage_total = 1024;

    storage_total = g_fs_free_size << 2;
    // storage
    uint8_t storage_str[16];
    sprintf((char*)storage_str, "%dk", storage);
    gdispFillString(2, 171, (const char *)storage_str, font_roboto_20.font, COLOR_BRIGHT_GREEN,  Black); 

    uint8_t storage_total_str[16];
    sprintf((char*)storage_total_str, "%dk", storage_total);
    gdispFillString(2, 195, (const char *)storage_total_str, font_roboto_20.font, COLOR_BRIGHT_GREEN,  Black);      
   
    uint8_t percent_str[16];
    sprintf((char*)percent_str, "%d%%", 100 * storage / storage_total);
    gdispFillString(2, 219, (const char *)percent_str, font_roboto_20.font, COLOR_BRIGHT_GREEN,  Black);   
}

const int batt_width = 20;
const int batt_height = 10;

void draw_battery_percent(uint16_t x, uint16_t y, int percent) {

    int start_x = x + 5;
    int start_y = y + 8;

    int end_x = start_x + ((percent * batt_width)/100);
    int end_y = start_y + batt_height;

    int i, j, offsetX, offsetY;

    int color = White;
    
    char str_temp[20];
    
    sprintf(str_temp, "%d", percent);
    
    offsetY = y;
    if (percent < 10) {
        offsetX = x + 10;
        offsetY = y + 3;
    } else if (percent == 100) {
        offsetX = x + 2;
        offsetY = y + 2;
    } else {
        offsetX = x + 7;
        offsetY = y + 2;
    }    
    
    gdispFillString(offsetX, offsetY, (char *)str_temp, (void*)font_roboto_12.font, color, Black);

//    for(i = start_y; i < end_y; i++){
//        for(j = start_x; j < end_x; j++){
//            set_dip_at_framebuff(j, i, color);
//        }
//    }
}

static void disp_next_alarm(uint32_t source, int16_t x, int16_t y, uint32_t color)
{
    if (!is_surface_nextalarm_active())
        return;
    
    raw_png_descript_t const* p_icon = NULL;    
    tms_alarm_t* next_alarm = NULL;

    get_next_alarm(&next_alarm);
    
    if (next_alarm != NULL) {
        p_icon = &icon_alarm;
        //x = 5; y = 20;
        //draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);
        draw_raw_png_to_framebuffer(p_icon, x, y, color);
        
        uint8_t alarm_str[10];
        sprintf((char*)alarm_str, "%02d:%02d", next_alarm->hour, next_alarm->minute);
        gdispFillString(x + 20, y + 3, (const char *)alarm_str, font_roboto_14.font, color, Black);
    }
}

static void disp_sys_status_at_surface(void)
{
    uint8_t w, h;
    uint8_t msg_icon = 0;
    uint8_t dnd_icon = 0;
    uint8_t batt_icon = 0;
    uint8_t offline_icon = 0;
    uint8_t sleep_icon = 0;
    uint16_t x = 0;
    uint16_t y = 0;
    raw_png_descript_t const* p_icon = NULL;

    if (is_red_point_open() && app_notify_num_get()) {
        p_icon = &icon_status_00_msg;
        msg_icon = 1;
        x = 0;
        y = 2;
        draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);
    }

    if (setting_get_dnd_status() != 0) {
        p_icon = &icon_status_dnd;
        x = 20 * msg_icon + 2;
        y = 2;
        draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);
    }    

    if (is_surface_battery_open()) {

        uint32_t percent = sys_get_battery();
        
        int color = White;
        
        if (percent < 10) {
            color = Red;
        } else if (percent < 20) {
            color = Orange;
        } else {
            color = HTML2COLOR(0x979797);
        }
        
        p_icon = &icon_battery;
        batt_icon = 1;
        x = 88;
        y = 0;
        draw_raw_png_to_framebuffer(p_icon, x, y, color);
    
        if (charge_cable_is_input()) { 
            p_icon = &icon_status_02_charging;
            batt_icon = 1;
            x = 94;
            y = 0;
            draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);
        } else if (get_device_powerstate() == DEV_PWR_STATE_LOW_POWER) {
            draw_battery_percent(x, y, percent);				
        } else {									
            draw_battery_percent(x, y, percent);				
        }
    }
    
    if(ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        p_icon = &icon_status_offline;
        offline_icon = 1;
        x = 88 - 20 * batt_icon;
        y = 2;
        draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);		
    }

    if (alg_get_sleep_state() != 0) {
        p_icon = &icon_status_sleep;
        sleep_icon = 1;
        x = 88 - 20 * batt_icon - 20 * offline_icon;
        y = 2;
        draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);
    }

    if (wms_window_lock_status_get() != 0) {
        p_icon = &icon_status_locked;
        uint32_t lock_offset = (batt_icon & offline_icon & sleep_icon) * 20;
        x = (120 - p_icon->width) / 2 - lock_offset;
        y = 2;
        draw_raw_png_to_framebuffer(p_icon, x, y, p_icon->fgcolor);
    }
    
//    uint8_t alarm_str[10];
//    sprintf((char*)alarm_str, "%d", surface_ctx_v.lock_st);
//    gdispFillString(2, 2, (const char *)alarm_str, font_roboto_14.font, White, Black);         
    
    
#if (RAWLOG_SAMPLE_ENABLE == TRUE)
    disp_rawLog_status();
#endif
}

FIL* weather_sfp = NULL;
WeatherGetInfoResult* sf_weather_info = NULL;

static void ask_weather_from_phone(void)
{
    if((ryos_utc_now() - surface_ctx_v.weather_update_time > WEATHER_UPDATE_INTERVAL)
            &&(ry_ble_get_state() == RY_BLE_STATE_CONNECTED))
    {
        surface_ctx_v.weather_update_time = ryos_utc_now();
        ry_data_msg_send(DATA_SERVICE_MSG_GET_REAL_WEATHER, 0, NULL);
        DEV_STATISTICS(weather_req_count);
        LOG_DEBUG("get weather cmd send\n");
    }
}

static void preload_weather_data(void)
{
    ask_weather_from_phone();

    u32_t read_bytes = 0;

    if(weather_sfp == NULL){
        weather_sfp = (FIL *)ry_malloc(sizeof(FIL));
    }
    
    if(weather_sfp != NULL){

        FRESULT state = f_open(weather_sfp, WEATHER_FORECAST_DATA, FA_READ);
        if(state != FR_OK){
            ry_free(weather_sfp);
            weather_sfp = NULL;
        } else {
            if(sf_weather_info == NULL){
                sf_weather_info = (WeatherGetInfoResult *)ry_malloc(sizeof(WeatherGetInfoResult));
            }

            if(sf_weather_info == NULL){

            } else {
                state = f_read(weather_sfp, sf_weather_info, sizeof(WeatherGetInfoResult), &read_bytes);

                if((state != FR_OK) || (read_bytes < sizeof(WeatherGetInfoResult))){
                    ry_free(sf_weather_info);
                    sf_weather_info = NULL;
                }

                f_close(weather_sfp);
                ry_free(weather_sfp);
                weather_sfp = NULL;
            }
        }
    } else {
        sf_weather_info = NULL;
    }	
}


//
// display weather info on surface
//
static void disp_weather_at_surface(int32_t source, int16_t x, int16_t y, uint32_t color, uint16_t align)
{
    // update a weather automatically anyway
    preload_weather_data();    
    
    if (!is_surface_weather_active())
        return;
	
    if (source == surface_item_element_data_source_temp_realtime ||
        source == surface_item_element_data_source_temp_forecast ||
        source == surface_item_element_data_source_weather_city ||
        source == surface_item_element_data_source_weather_type_realtime ||
        source == surface_item_element_data_source_weather_type_forecast ||
        source == surface_item_element_data_source_weather_wind)
    {				
        // pass
//      uint8_t debug_str[20];
//      sprintf((char*)debug_str, "w: %d, %d, %d", source, color, align);
//      gdispFillStringBox( 0, 130, 0, 22, (char *)debug_str, NULL, White, Black|(1<<30), justifyLeft);	
    } else {
        return;
    }
    
    if (align == 0)
        align = 1;

    extern const wth_phenomena_t wth_phnm_bmp[];
    extern const int wth_phnm_bmp_len;

    WeatherInfo* city_weather = NULL;

    if (sf_weather_info != NULL && city_list.city_items_count > 0) {

        int i = 0;
        const int city_index = 0; // first city in list
        int date_offset = 0;			// day offset in list
        int weather_index = 0; 
        char* str_city = NULL;
    
        for(i = 0; i < sf_weather_info->infos_count; i++){
            if(strcmp(&(city_list.city_items[city_index].city_id[0]), &(sf_weather_info->infos[i].city_id[0]) ) == 0){
                city_weather = &(sf_weather_info->infos[i]);
                break;
            }
        }
        
        if (city_weather != NULL) {
            for (i = 0; i < city_weather->daily_infos_count; i++){
                if(ryos_utc_now_notimezone() - city_weather->daily_infos[i].date <= 24*60*60){
                    date_offset = i;
                    break;
                }
            }					
        }
        
        // city title
        if (source == surface_item_element_data_source_weather_city) {
            str_city = city_list.city_items[city_index].city_name;
        
            // display city name
            //gdispFillStringBox( 0, 130, 0, 22, str_city, NULL, White, Black|(1<<30), justifyCenter);
            gdispFillString( x, y,
                            (char *)str_city,
                            (void*)font_roboto_20.font,
                            color, Black);
        }
        
        // day forecast
        if (source == surface_item_element_data_source_temp_forecast) {
    
            uint8_t temp_str[20];
        
            if (TEMPERATURE_CHECK((int)(city_weather->daily_infos[date_offset].forecast_day_temp)) &&
                TEMPERATURE_CHECK((int)(city_weather->daily_infos[date_offset].forecast_night_temp)))
            {
                sprintf((char*)temp_str, "%d/%d°C",
                        (int)city_weather->daily_infos[date_offset].forecast_night_temp,
                        (int)city_weather->daily_infos[date_offset].forecast_day_temp);
            } else {
                sprintf((char*)temp_str, "--");
            }

            //gdispFillStringBox( 0, 154, 0, 22, (char *)temp_str, NULL, White, Black|(1<<30), justifyCenter);
            gdispFillString( x, y,
                            (char *)temp_str,
                            (void*)font_roboto_20.font,
                            color, Black);
        }
                        
        if (city_weather->daily_infos[date_offset].realtime_temp > -70) {						
            
            int max_type = wth_phnm_bmp_len;
            int weather_type = 0;
            ry_time_t time;
            tms_get_time(&time);

            if (source == surface_item_element_data_source_weather_type_realtime) {							
                    weather_type = city_weather->daily_infos[date_offset].realtime_type;							
            } else if (source == surface_item_element_data_source_weather_type_forecast) {
                if (time.hour > 18) {
                    weather_type = city_weather->daily_infos[date_offset].forecast_night_type;
                } else {
                    weather_type = city_weather->daily_infos[date_offset].forecast_day_type;
                }
            }
            
            for (int i = 0; i < max_type; i++) {
                if(weather_type == wth_phnm_bmp[i].type_num){
                    weather_index = i;
                    break;
                }
            }
            
            // weather type
            if (source == surface_item_element_data_source_weather_type_realtime ||
                source == surface_item_element_data_source_weather_type_forecast) {
                if (weather_index < max_type) {
                    uint8_t w, h;
                    //img_exflash_to_framebuffer_transparent((u8_t*)wth_phnm_bmp[weather_index].img, \
                    //										2, 134, &w, &h, d_justify_left);
                    img_exflash_to_framebuffer_transparent((u8_t*)wth_phnm_bmp[weather_index].img, \
                                                            x, y, &w, &h, d_justify_left);
                }
            }
            
            // realtime temp						
            if (source == surface_item_element_data_source_temp_realtime) {
        
                uint8_t temp_str[20];							
            
                sprintf((char*)temp_str, "%d°C", (int)(city_weather->daily_infos[date_offset].realtime_temp));								
                //gdispFillStringBox( 60, 150, 0, 22, (char *)temp_str, NULL, White, Black|(1<<30), justifyLeft);
//                gdispFillString(x, y,
//                                (char *)temp_str,
//                                font_roboto_20.font,
//                                color,
//                                Black);
                gdispFillString( x, y,
                                (char *)temp_str,
                                (void*)font_roboto_20.font,
                                color, Black);                
            }
        }
    }
}


static ry_sts_t surface_display_header(int source, surface_header_t const* p_header)
{
    ry_sts_t ret = RY_SUCC;

    if (!gui_init_done || (surface_ctx_v.state == SURFACE_OFF))
        return RY_SUCC;

    if ((source == SURFACE_DISPLAY_FROM_RTC) && (last_min == hal_time.ui32Minute)) {
        // LOG_DEBUG("[surface_display_red_arron] no min update\r\n");
        return RY_SUCC;
    }
    
    code_success = 0;

    last_min = hal_time.ui32Minute;

    clear_buffer_black();
    uint8_t w, h;
    uint32_t hour = 0;
    uint32_t icon_color = 0;

    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t*)p_header->bg_pic_filename, \
                            0,  0, &w, &h, d_justify_center);

    if(p_header->items_count > MAX_SURFACE_ITEM_NUM) {
        LOG_ERROR("[surface] invalid header_counter:%d\n", p_header->items_count);
    }

    for(int i = 0; i < p_header->items_count; i++) {
        surface_item_t* p_item = &p_header->p_items[i];

        gui_font_t* p_font = &font_roboto_44;
        coord_t x;
        coord_t y;
        raw_png_descript_t const* p_icon = NULL;

        char p_render_string_buffer[16] = {0};

        switch(p_item->source) {
            case surface_item_element_data_source_date_year:
                sprintf(p_render_string_buffer, "%04d", hal_time.ui32Year);
                break;
            case surface_item_element_data_source_date_month:
                sprintf(p_render_string_buffer, "%02d", hal_time.ui32Month);
                break;
            case surface_item_element_data_source_date_week:
                strcpy(p_render_string_buffer,
                (const char *)week_string_chs[hal_time.ui32Weekday]);
                break;
            case surface_item_element_data_source_date_day:
                sprintf(p_render_string_buffer, "%02d", hal_time.ui32DayOfMonth);
                break;
            case surface_item_element_data_source_time_hour:
                hour = hal_time.ui32Hour;
                if (is_time_fmt_12hour_active()){
                    if (hour == 0) hour = 12;
                    else if (hour > 12) hour -= 12;
                }
                sprintf(p_render_string_buffer, "%02d", hour); 
                break;
            case surface_item_element_data_source_time_minute:
                sprintf(p_render_string_buffer, "%02d", hal_time.ui32Minute);
                break;
            case surface_item_element_data_source_time_seconds:
                sprintf(p_render_string_buffer, "%02d", hal_time.ui32Second);
                break;
            case surface_item_element_data_source_step_today:
                sprintf(p_render_string_buffer, "%d", alg_get_step_today());
                break;
            case surface_item_element_data_source_cal_today:
                sprintf(p_render_string_buffer, "%d", (int)(alg_calc_calorie()/1000));
                break;
            case surface_item_element_data_source_hrm_last_value:
                sprintf(p_render_string_buffer, "%d", s_alg.hr_cnt);
                break;
            case surface_item_element_symbol_type_comma:
                sprintf(p_render_string_buffer, "%s", ",");
                break;
            case surface_item_element_symbol_type_colon:
				sprintf(p_render_string_buffer, "%s", ":");
                break;
            case surface_item_element_symbol_type_dot:
                sprintf(p_render_string_buffer, "%s", ".");
                break;
            case surface_item_element_icon_steps:
            case surface_item_element_icon_cal:
            case surface_item_element_icon_hrm:
            case surface_item_element_icon_step1:
            case surface_item_element_data_source_time_ampm:                
                goto display_icons;
            case surface_item_element_data_source_temp_realtime:
            case surface_item_element_data_source_temp_forecast:
            case surface_item_element_data_source_weather_city:
            case surface_item_element_data_source_weather_type_realtime:
            case surface_item_element_data_source_weather_type_forecast:
            case surface_item_element_data_source_weather_wind:
                goto display_weather;
            case surface_item_element_data_source_nextalarm:
                goto display_nextalarm;
            default:
                continue;
        }

        switch(p_item->font.type) {
            case surface_item_elemet_supported_font_roboto:
                if(p_item->font.size == 72) {
                    p_font = &font_roboto_60;
                } else if(p_item->font.size == 44) {
                    p_font = &font_roboto_44;
                } else if(p_item->font.size == 32) {
                    p_font = &font_roboto_32;
                } else if(p_item->font.size == 20) {
                    p_font = &font_roboto_20;
                } else if(p_item->font.size == 16) {
                    p_font = &font_roboto_16;
                } else {
                    LOG_DEBUG("[surface] err robot font size load\n");
                    continue;
                }
                break;
            case surface_item_elemet_supported_font_langting_lb:
                if(p_item->font.size == 16) {
                    p_font = &font_roboto_20;
                } else {
                    LOG_DEBUG("[surface] err langting font size load\n");
                    continue;
                }
                break;
            default:
                break;
        }

        if(p_item->style.align == surface_item_element_style_align_middle) {
            uint32_t string_width_bit = gdispGetStringWidth(p_render_string_buffer, p_font->font);
            int32_t start_x = p_item->pos.x - string_width_bit/2;
            if(start_x < 0) {
                x = 0;
            } else {
                x = start_x;
            }
            y = p_item->pos.y;
        } else {
            x = p_item->pos.x;
            y = p_item->pos.y;
        }
//        LOG_DEBUG("[surface] TEXT (%d,%d), %s, f:%d-%d, %06X\n",\
                x,y,\
                p_render_string_buffer,\
                p_item->font.type,\
                p_item->font.size,\
                p_item->font.color);
				
        gdispFillString(x, y,
                p_render_string_buffer,
                p_font->font,
                p_item->font.color,
                Black);

        continue;

display_weather:        
        disp_weather_at_surface(p_item->source, p_item->pos.x, p_item->pos.y, p_item->style.color, p_item->style.align);
        continue;
        
display_nextalarm: 
        disp_next_alarm(p_item->source, p_item->pos.x, p_item->pos.y, p_item->style.color);
        continue;

display_icons:

        icon_color = p_item->style.color;
        x = p_item->pos.x;
        y = p_item->pos.y;        
        
        switch(p_item->source) {
            case surface_item_element_icon_steps:
                p_icon = &icon_step;
                break;
            case surface_item_element_icon_cal:
                p_icon = &icon_cal;
                break;
            case surface_item_element_icon_hrm:
                p_icon = &icon_heart;
                break;
            case surface_item_element_icon_step1:
                p_icon = &icon_step1;
                break;
            case surface_item_element_data_source_time_ampm:                
                if (is_time_fmt_12hour_active())
                {                    
                    hour = hal_time.ui32Hour;
                    if (hour == 0 || hour > 12)
                        //p_icon = &icon_time_pm;
                        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"time_pm.bmp", \
                            x, y, &w, &h, d_justify_left);
                    else
                        //p_icon = &icon_time_am;
                        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"time_am.bmp", \
                            x, y, &w, &h, d_justify_left);
                    
//                    icon_color = p_icon->fgcolor;
                }
                else {
                    continue;
                }
                break;
            default:
                continue;
        }

        draw_raw_png_to_framebuffer(p_icon, x, y, icon_color);
        continue;
    }

    disp_sys_status_at_surface();
		//disp_weather_at_surface();

    if (source != SURFACE_DISPLAY_FROM_UNLOCK){
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
    return ret;
}


/**
 * @brief   Entry of the surface activity
 *
 * @param   None
 *
 * @return  surface_onCreate result
 */
ry_sts_t surface_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    surface_ctx_v.lock_st = (wms_window_lock_status_get() != 0) ? E_LOCK_IN : E_LOCK_OUT;
    
    LOG_DEBUG("[surface_onCreate]surface_ctx_v.lock_st:%d\r\n", surface_ctx_v.lock_st);

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    if(surface_mutex == NULL){
        ret = ryos_mutex_create(&surface_mutex);
        if (RY_SUCC != ret) {
            LOG_ERROR("[ryos_mutex_create]Error status: %d\r\n", ret);
        } 
    }

    surface_ctx_v.state = SURFACE_ON;
    surface_ctx_v.animate_finished = 1;
    surface_ctx_v.animate_lock_finished = 1;
    surface_ctx_v.animate_play_lock = 0;
    surface_ctx_v.animate_play_unlock = 0;
    surface_ctx_v.display_mode = MAIN;

    if(get_device_session() == DEV_SESSION_RUNNING){
        set_device_session(DEV_SESSION_IDLE);
    }
    device_set_powerOn_mode(0);
    
    if (is_lock_enable()){        
        surface_ctx_v.unlock_type = get_surface_unlock_type();
    }
    else {
        if (surface_ctx_v.unlock_type != SURFACE_UNLOCK_NONE)
            surface_ctx_v.unlock_type = SURFACE_UNLOCK_NONE;
        
        set_surface_unlock_type(SURFACE_UNLOCK_NONE);
    }

    activity_surface_on_event(SURFACE_DISPLAY_FROM_CREATE);

    wms_event_listener_add(WM_EVENT_RTC, &activity_surface, surface_onRTCEvent);

    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_surface, surface_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_RAW, &activity_surface, surface_onRawTouchEvent);    
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_surface, surface_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD,    &activity_surface, surface_onProcessedCardEvent);
    wms_event_listener_add(WM_EVENT_SURFACE, &activity_surface, surface_onSurfaceServiceEvent);
    
    if (surface_timeout_timer_id == 0){
        ry_timer_parm timer_para;
        /* Create the timer */
        timer_para.timeout_CB = surface_timeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "surface_lock_timer";
        surface_timeout_timer_id = ry_timer_create(&timer_para);
    }      

    return ret;
}


/**
 * @brief   API to exit surface activity
 *
 * @param   None
 *
 * @return  surface activity Destroy result
 */
ry_sts_t surface_onDestroy(void)
{
    ry_sts_t ret = RY_SUCC;

    // LOG_DEBUG("[surface_onDestrory]begin.\r\n");
    stop_unlock_alert_animate();
    
    ry_timer_delete(surface_timeout_timer_id);
    surface_timeout_timer_id = 0;

    surface_ctx_v.state = SURFACE_OFF;
    wms_event_listener_del(WM_EVENT_RTC, &activity_surface);
    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_surface);
    wms_event_listener_del(WM_EVENT_TOUCH_RAW, &activity_surface);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_surface);
    wms_event_listener_del(WM_EVENT_CARD, &activity_surface);
    wms_event_listener_del(WM_EVENT_SURFACE, &activity_surface);

    // LOG_DEBUG("[surface_onDestroy] end\r\n");
    
    return ret;
}

ry_sts_t activity_surface_on_event(int evt)
{
    return surface_display_header(evt, surface_get_current_header());

}

void debug_port_surface_manager(void)
{
    if (wms_activity_get_cur_disableUnTouchTime() == 0){
        wms_msg_send(IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE, 0, NULL);
    }
    ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
}
