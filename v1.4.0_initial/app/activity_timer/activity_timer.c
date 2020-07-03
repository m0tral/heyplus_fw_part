#include <stdio.h>
#include "ry_font.h"
#include "touch.h"
#include "led.h"
#include "gui.h"
#include "gui_bare.h"
#include "gui_img.h"
#include "gui_msg.h"
#include "scrollbar.h"

#include "ryos.h"
#include "ryos_timer.h"
#include "board_control.h"
#include "ry_hal_inc.h"

#include "app_pm.h"
#include "app_interface.h"

#include "timer_management_service.h"
#include "card_management_service.h"

#include "activity_card.h"
#include "activity_timer.h"


/*********************************************************************
 * CONSTANTS
 */
 
#define SCROLLBAR_HEIGHT                40
#define SCROLLBAR_WIDTH                 1

#define COUNTDOWN_DEFAULT               5 * 60 // 5 mins
#define COUNTDOWN_MOTAR_THRESHOLD       3 * 1000   // 3 secs
#define COUNTDOWN_SCREENOFF_THRESHOLD   10 * 1000  // 10 secs
#define TIMER_UI_REFRESH_PERIOD         200

/*********************************************************************
 * TYPEDEFS
 */
 
typedef enum {
    MODE_MENU,
    MODE_STOPWATCH,
    MODE_COUNTDOWN,
} timer_mode_t;

typedef enum {
    STATE_TIMER_PREP,
    STATE_TIMER_IDLE,
    STATE_TIMER_STARTED,
    STATE_TIMER_STOPPED,
} timer_state_t;

typedef enum {
    CMD_TIMER_NONE,
    CMD_TIMER_START,
    CMD_TIMER_STOP,
    CMD_TIMER_RELOAD,
} timer_cmd_t;

typedef struct {
    int8_t       top_menu_item;   
    u8_t         sel_item;
    u8_t         sel_timer;
    u32_t        start_time;
    u32_t        duration;
    u32_t        countdown;
    bool         is_ending;
    u32_t        motar_start_clock;     // MCU clock time, not msec
    timer_mode_t mode;
    timer_state_t state;
    timer_cmd_t  cmd;
    u32_t        exit_timer_id;
    u32_t        ui_timer_id;
    u32_t        untouch_timer_id;
} timer_ctx_t;

typedef struct {
    timer_id_t  id;  
    const char* descrp;
    const char* img;
    void        (*ui_update)();        
} timer_item_t;

/*********************************************************************
 * VARIABLES
 */
const timer_item_t timer_array[] = {
    {TIMER_STOPWATCH, "Секундомер",  "menu_stopwatch.bmp",  stopwatch_ui_update},
    {TIMER_COUNTDOWN, "Таймер",      "menu_countdown.bmp",  countdown_ui_update},
};

const char* const feature_not_available_yet = "Не доступно";
const char* const timer_countdown = "Таймер";
const char* const timer_stopwatch = "Секундомер";

const char* const text_timer_break_cuz_charge[] = {
	"Подключена",
	"зарядка.",
	"Таймер",
	"остановлен."
};

const char* const action_icon[] = {
    "g_widget_execute.bmp",
    "g_widget_execute.bmp",
    "g_widget_03_stop2.bmp",
    "g_widget_repeat.bmp",
};

activity_t activity_timer = {
    .name = "timer_stopwatch", 
    .onCreate = timer_onCreate,
    .onDestroy = timer_onDestroy,
    .disableUntouchTimer = 1,
    .disableTouchMotar = 1,    
    .priority = WM_PRIORITY_H,
};


volatile timer_ctx_t    timer_ctx_v;
extern tms_ctx_t        tms_ctx_v;

//scrollbar_t*            timer_scrollbar = NULL;

ryos_thread_t*      timer_ui_thread_handle;
ryos_semaphore_t*   timer_ui_start_sem;
ryos_semaphore_t*   timer_ui_stop_sem;

volatile bool is_drawing = false;

/*********************************************************************
 * FUNCTIONS
 */

void timer_untouch_timer_start(void);


ry_sts_t timer_menu_display_update(u8_t menu_idx)
{
    u8_t w, h;
    clear_buffer_black();
    img_exflash_to_framebuffer((uint8_t*)timer_array[menu_idx].img, \
                                22, 4, &w, &h, d_justify_center);
    gdispFillStringBox( 0,                    
                        90,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char*)timer_array[menu_idx].descrp,        
                        (void *)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);

    img_exflash_to_framebuffer((uint8_t*)timer_array[(menu_idx + 1) % TIMER_ITEM_NUM].img, \
                                22, 128, &w, &h, d_justify_center);

    gdispFillStringBox( 0,                    
                        214,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char*)timer_array[(menu_idx + 1) % TIMER_ITEM_NUM].descrp,        
                        (void *)font_roboto_20.font,
                        White, Black,
                        justifyCenter);

    //must protect here, or screen off would reset system
    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    return RY_SUCC;
}

void timer_countdown_disp(ry_time_t cur_time, u32_t duration_second, timer_state_t state)
{
    if (timer_ctx_v.mode == MODE_MENU) return;
    if (is_drawing) return;

    is_drawing = true;
    
    char * info_str = (char *)ry_malloc(40);
    clear_buffer_black();
    
    gdispFillStringBox( 0,                    
                        27,
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char *)timer_countdown,
                        (void *)font_roboto_20.font,               
                        White, Black,                  
                        justifyCenter);                        

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void*)font_roboto_20.font, Grey, Black);
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 67, info_str, (void*)font_roboto_32.font, 0x15d185, Black);                        
                        
    uint8_t w, h;
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)action_icon[state],\
                            0, 140, &w, &h, d_justify_center);                        

    ry_free(info_str);

    //must protect here, or screen off would reset system
    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        
    is_drawing = false;
}

void timer_countdown_disp_partial(ry_time_t cur_time, u32_t duration_second)
{
    if (timer_ctx_v.mode == MODE_MENU) return;
    if (is_drawing) return;

    is_drawing = true;    
    
    img_pos_t pos = {0};
    pos.x_end = RM_OLED_WIDTH - 1;
    pos.y_end = 110;
    pos.buffer = frame_buffer;      
    
    char * info_str = (char *)ry_malloc(40);
    
    clear_frame(0, 110);
    
    gdispFillStringBox( 0,                    
                        27,
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char *)timer_countdown,
                        (void *)font_roboto_20.font,               
                        White, Black,                  
                        justifyCenter);                        

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void*)font_roboto_20.font, Grey, Black);
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 67, info_str, (void*)font_roboto_32.font, 0x15d185, Black);                        
                        
    ry_free(info_str);

    //must protect here, or screen off would reset system
    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
                        
    is_drawing = false;
}

void timer_stopwatch_disp(ry_time_t cur_time, u32_t duration_second, timer_state_t state)
{
    if (timer_ctx_v.mode == MODE_MENU) return;
    if (is_drawing) return;    

    is_drawing = true;    
    
    char * info_str = (char *)ry_malloc(40);
    clear_buffer_black();
    
    gdispFillStringBox( 0,                    
                        27,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char *)timer_stopwatch,      
                        (void *)font_roboto_20.font,               
                        White, Black,                  
                        justifyCenter);                        

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void*)font_roboto_20.font, Grey, Black);
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 67, info_str, (void *)font_roboto_32.font, 0x15d185, Black);                        
                        
    uint8_t w, h;
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)action_icon[state],\
                            0, 140, &w, &h, d_justify_center);                        

    ry_free(info_str);

    //must protect here, or screen off would reset system
    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        
    is_drawing = false;                        
}

void timer_stopwatch_disp_partial(ry_time_t cur_time, u32_t duration_second)
{
    if (timer_ctx_v.mode == MODE_MENU) return;
    if (is_drawing) return;

    is_drawing = true;    
    
    img_pos_t pos = {0};
    pos.x_end = RM_OLED_WIDTH - 1;
    pos.y_end = 110;
    pos.buffer = frame_buffer;        
    
    char * info_str = (char *)ry_malloc(40);
    
    clear_frame(0, 110);
    
    gdispFillStringBox( 0,
                        27,
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char *)timer_stopwatch,      
                        (void *)font_roboto_20.font,               
                        White, Black,                  
                        justifyCenter);                        

    sprintf(info_str, "%02d:%02d", cur_time.hour, cur_time.minute);
    gdispFillString(3, 0, info_str, (void*)font_roboto_20.font, Grey, Black);
    
    //time conter
    sprintf(info_str, "%02d:%02d:%02d", duration_second/3600, (duration_second%3600)/60, duration_second%60);
    gdispFillString(3, 67, info_str, (void *)font_roboto_32.font, 0x15d185, Black);

    ry_free(info_str);

    //must protect here, or screen off would reset system
    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
                        
    is_drawing = false;                        
}

void turnon_screen_and_led_direct(void)
{
    _start_func_oled(1);
    uint8_t brightness = tms_oled_max_brightness_percent();
    oled_set_brightness(brightness);
    uint8_t led_brightness = ry_calculate_led_brightness(brightness);
    ry_led_onoff_with_effect(1, led_brightness);

    extern ry_gui_ctx_t    ry_gui_ctx_v;
    ry_gui_ctx_v.curState = GUI_STATE_ON;
}

void stopwatch_ui_update(void)
{    
    bool full_update = false;
            
    //time now
    ry_time_t cur_time = {0};
    ry_hal_rtc_get();   // update time
    tms_get_time(&cur_time);
    
    if (timer_ctx_v.cmd != CMD_TIMER_NONE) {
        
        switch (timer_ctx_v.cmd) {
            case CMD_TIMER_START:
                timer_ctx_v.start_time = ryos_rtc_now();
                timer_ctx_v.state = STATE_TIMER_STARTED;
                break;
            case CMD_TIMER_STOP:
                timer_ctx_v.state = STATE_TIMER_STOPPED;
                break;
            case CMD_TIMER_RELOAD:
                timer_ctx_v.state = STATE_TIMER_IDLE;
                break;
            default:
                break;
        }
        
        timer_ctx_v.cmd = CMD_TIMER_NONE;
        full_update = true;
    }
    
    if (timer_ctx_v.state == STATE_TIMER_PREP) {
        timer_ctx_v.state = STATE_TIMER_IDLE;
        full_update = true;
    }
    
    if (timer_ctx_v.state == STATE_TIMER_STARTED) {
        timer_ctx_v.duration = ryos_rtc_now() - timer_ctx_v.start_time;
    }
    else if (timer_ctx_v.state == STATE_TIMER_IDLE) {
        timer_ctx_v.duration = 0;
    }

    if (full_update)
        timer_stopwatch_disp(cur_time, timer_ctx_v.duration, timer_ctx_v.state);
    else
        timer_stopwatch_disp_partial(cur_time, timer_ctx_v.duration);
}

void countdown_ui_update(void)
{    
    bool full_update = false;

    //time now
    ry_time_t cur_time = {0};
    ry_hal_rtc_get();   // update time
    tms_get_time(&cur_time);

//    ry_hal_wdt_reset();
            
    if (timer_ctx_v.cmd != CMD_TIMER_NONE) {
        
        switch (timer_ctx_v.cmd) {
            case CMD_TIMER_START:
                motar_stop();
                timer_ctx_v.start_time = ryos_rtc_now();
                timer_ctx_v.is_ending = false;
                timer_ctx_v.state = STATE_TIMER_STARTED;
            break;
            case CMD_TIMER_STOP:
                timer_ctx_v.state = STATE_TIMER_STOPPED;
                break;
            case CMD_TIMER_RELOAD:
                timer_ctx_v.state = STATE_TIMER_IDLE;
                break;
            default:
                break;
        }
        
        timer_ctx_v.cmd = CMD_TIMER_NONE;
        full_update = true;
    }
    
    if (timer_ctx_v.state == STATE_TIMER_PREP) {
        timer_ctx_v.state = STATE_TIMER_IDLE;
        full_update = true;
    }

    if (timer_ctx_v.state == STATE_TIMER_IDLE)
        timer_ctx_v.duration = timer_ctx_v.countdown;
    else if (timer_ctx_v.state == STATE_TIMER_STARTED) {
        
        ryos_enter_critical();
        u32_t delta = ryos_rtc_now() - timer_ctx_v.start_time;
                    
        if (delta >= timer_ctx_v.countdown) {
            
            ryos_exit_critical(0);
            
            if (!timer_ctx_v.is_ending) {
                turnon_screen_and_led_direct();
                timer_ctx_v.is_ending = true;
            }
            
            full_update = true;
            timer_ctx_v.state = STATE_TIMER_STOPPED;

            timer_ctx_v.duration = 0;
            timer_ctx_v.motar_start_clock = ry_hal_clock_time();
            motar_set_work(MOTAR_TYPE_QUICK_LOOP, 3);
            
            timer_untouch_timer_start();
            
//            char* taskInfo;
//            taskInfo = ry_malloc(40*20);    
//            ryos_get_thread_stats(taskInfo);
//            LOG_ERROR("[task stats tmrend]\r\n%s\r\n", taskInfo);
//            ry_free(taskInfo);            
        }
        else {
            timer_ctx_v.duration = timer_ctx_v.countdown - delta;            
            
            ryos_exit_critical(0);
            
            if (timer_ctx_v.duration < 7) {
                if (!timer_ctx_v.is_ending) {
                    turnon_screen_and_led_direct();
                    timer_ctx_v.is_ending = true;
                }
            }
        }
    }

    if (full_update)
        timer_countdown_disp(cur_time, timer_ctx_v.duration, timer_ctx_v.state);
    else
        timer_countdown_disp_partial(cur_time, timer_ctx_v.duration);            
}

void timer_untouch_timeout_handler(void* arg)
{
    ry_timer_stop(timer_ctx_v.untouch_timer_id);
    wms_screenOnOff(0);
    
    if (timer_ctx_v.state != STATE_TIMER_STARTED)
    {
        if (timer_ctx_v.mode != MODE_MENU)
        {
            timer_ctx_v.mode = MODE_MENU;
            ryos_semaphore_wait(timer_ui_stop_sem);            
        }
                
//        LOG_INFO("[activity_timer] timer untouch done");        
        
        wms_msg_send(IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE, 0, NULL);        
    }
}

void timer_untouch_timer_start(void)
{
    if (timer_ctx_v.untouch_timer_id) {
        ry_timer_stop(timer_ctx_v.untouch_timer_id);
        ry_timer_start(timer_ctx_v.untouch_timer_id, COUNTDOWN_SCREENOFF_THRESHOLD, timer_untouch_timeout_handler, NULL);
    }
}

int event_menu_touch(tp_processed_event_t* p)
{
    int consumed = 1;

    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            timer_ctx_v.top_menu_item --;
            if (timer_ctx_v.top_menu_item < 0){
                timer_ctx_v.top_menu_item = 0;
            }
            timer_menu_display_update(timer_ctx_v.top_menu_item);
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            timer_ctx_v.top_menu_item ++;
            if (timer_ctx_v.top_menu_item >= TIMER_ITEM_NUM - 2){
                timer_ctx_v.top_menu_item = (TIMER_ITEM_NUM - 2);
                if (timer_ctx_v.top_menu_item < 0){
                    timer_ctx_v.top_menu_item = 0;
                }
            }
            timer_menu_display_update(timer_ctx_v.top_menu_item);
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y < 8){
                if ((p->y <= 3)){
                    timer_ctx_v.sel_item = timer_ctx_v.top_menu_item;
                }
                else if ((p->y > 3) && (p->y <= 7)){   
                    timer_ctx_v.sel_item = (timer_ctx_v.top_menu_item + 1) % TIMER_ITEM_NUM;
                }
                
                timer_ctx_v.sel_timer = timer_array[timer_ctx_v.sel_item].id;
                
                if (timer_ctx_v.sel_timer == TIMER_STOPWATCH)
                    timer_ctx_v.mode = MODE_STOPWATCH;
                if (timer_ctx_v.sel_timer == TIMER_COUNTDOWN)
                    timer_ctx_v.mode = MODE_COUNTDOWN;
                
                timer_ctx_v.state = STATE_TIMER_PREP;
                timer_ctx_v.cmd = CMD_TIMER_NONE;
                
                ryos_semaphore_post(timer_ui_start_sem);
            }
            else if ((p->y >= 8)){
                wms_activity_back(NULL);
                return consumed;
            }
            break;
        
        default:
            break;
    }
    
    return consumed;
}

int event_stopwatch_touch(tp_processed_event_t* p)
{
    int consumed = 1;

    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            if (timer_ctx_v.mode == MODE_COUNTDOWN &&
                timer_ctx_v.state == STATE_TIMER_IDLE)
            {
                if (timer_ctx_v.countdown > 60 * 60)
                    timer_ctx_v.countdown -= 15 * 60;
                else if (timer_ctx_v.countdown > 10 * 60)
                    timer_ctx_v.countdown -= 5 * 60;
                else if (timer_ctx_v.countdown > 60)
                    timer_ctx_v.countdown -= 60;
                else if (timer_ctx_v.countdown > 10)
                    timer_ctx_v.countdown -= 10;                
            }            
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            if (timer_ctx_v.mode == MODE_COUNTDOWN &&
                timer_ctx_v.state == STATE_TIMER_IDLE)
            {
                if (timer_ctx_v.countdown >= 60 * 60)
                    timer_ctx_v.countdown += 15 * 60;
                else if (timer_ctx_v.countdown >= 10 * 60)
                    timer_ctx_v.countdown += 5 * 60;
                else if (timer_ctx_v.countdown >= 60)
                    timer_ctx_v.countdown += 60;
                else
                    timer_ctx_v.countdown += 10;                    
            }            
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y < 8){
                if ((p->y <= 3)){
                }
                else if ((p->y > 3) && (p->y <= 7)){   
                    
                    if (timer_ctx_v.state == STATE_TIMER_IDLE)
                        timer_ctx_v.cmd = CMD_TIMER_START;
                    if (timer_ctx_v.state == STATE_TIMER_STARTED)
                        timer_ctx_v.cmd = CMD_TIMER_STOP;
                    if (timer_ctx_v.state == STATE_TIMER_STOPPED)
                        timer_ctx_v.cmd = CMD_TIMER_RELOAD;                    
                }
            }
            else if ((p->y >= 8)){
                timer_ctx_v.duration = 0;
                timer_ctx_v.mode = MODE_MENU;

                ryos_semaphore_wait(timer_ui_stop_sem);
                
                timer_menu_display_update(timer_ctx_v.top_menu_item);
                return consumed;
            }
                        
            break;
        
        default:
            break;
    }
    
    //timer_untouch_timer_start();
    
    LOG_DEBUG("[timer_option_onTouchEvent] 1 top_menu_item: %d\r\n", timer_ctx_v.top_menu_item);
    return consumed;
}

int timer_option_onTouchRawEvent(ry_ipc_msg_t* msg)
{
    timer_untouch_timer_start();
    
    return 0;
}

int timer_option_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;    
    
    switch (timer_ctx_v.mode)
    {
        case MODE_STOPWATCH:
            consumed = event_stopwatch_touch(p);
            break;
        case MODE_COUNTDOWN:
            consumed = event_stopwatch_touch(p);
            break;
        case MODE_MENU:
        default:
            consumed = event_menu_touch(p);
            break;
    }
           
    return consumed;
}

int timer_ui_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);
        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}

int timer_ui_onMSGEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    extern activity_t msg_activity;

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);    
    wms_activity_jump(&msg_activity, (void *)0 );
    
    return consumed; 
}


void timer_charging_timeout(void* usrdata)
{
    ry_timer_stop(timer_ctx_v.exit_timer_id);
    ry_timer_delete(timer_ctx_v.exit_timer_id);
    timer_ctx_v.exit_timer_id = 0;
    
    wms_activity_back(NULL);
}

void charging_timer_exit(void)
{
    if (timer_ctx_v.exit_timer_id == 0) {
        ry_timer_parm timer_para;
        timer_para.timeout_CB = NULL;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "timer_ready_timer";
        timer_ctx_v.exit_timer_id = ry_timer_create(&timer_para);
    }
    ry_timer_start(timer_ctx_v.exit_timer_id, 2000, timer_charging_timeout, NULL);
    
    clear_buffer_black();
    
    gdispFillStringBox( 0, 40, font_roboto_20.width, font_roboto_20.height,           
                        (char *)text_timer_break_cuz_charge[0],        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
    gdispFillStringBox( 0, 70, font_roboto_20.width, font_roboto_20.height,
                        (char *)text_timer_break_cuz_charge[1],        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
    gdispFillStringBox( 0, 110, font_roboto_20.width, font_roboto_20.height,            
                        (char *)text_timer_break_cuz_charge[2],        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
    gdispFillStringBox( 0, 140, font_roboto_20.width, font_roboto_20.height,
                        (char *)text_timer_break_cuz_charge[3],
                        (void*)font_roboto_20.font,
                        White, Black, justifyCenter);
                        
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);    
}

ry_sts_t timer_onCreate(void * usrdata)
{
    if (usrdata)
        ry_free(usrdata);
    
    ry_sts_t ret = RY_SUCC;
    ry_timer_parm timer_para;

    ry_memset((void*)&timer_ctx_v, 0, sizeof(timer_ctx_t));
    
    timer_ctx_v.mode = MODE_MENU;
    timer_ctx_v.countdown = COUNTDOWN_DEFAULT;
    
    if(charge_cable_is_input()){
        charging_timer_exit();
        return ret;
    }
    
//    LOG_INFO("[activity_timer] create");
    
    tms_ctx_v.disable_deepsleep = 1;
    
    timer_ui_thread_init();    
    
    timer_para.timeout_CB = timer_untouch_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "timer_untouch_timer";
    timer_ctx_v.untouch_timer_id = ry_timer_create(&timer_para);    
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    ry_delay_ms(100);
    
//    char* taskInfo;
//    taskInfo = ry_malloc(40*20);    
//    ryos_get_thread_stats(taskInfo);
//    LOG_ERROR("[task stats create]\r\n%s\r\n", taskInfo);
//    ry_free(taskInfo);
    
    ret = timer_menu_display_update(timer_ctx_v.top_menu_item);
    
//    wms_event_listener_add(WM_EVENT_MESSAGE,            &activity_timer, timer_ui_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_RAW,          &activity_timer, timer_option_onTouchRawEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED,    &activity_timer, timer_option_onTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD,               &activity_timer, timer_ui_onProcessedCardEvent);
    
//    ry_hal_wdt_stop();
    
    return ret;
}


ry_sts_t timer_onDestroy(void)
{
    ry_sts_t ret = RY_SUCC;
    
    if (timer_ui_start_sem != NULL)
        ryos_semaphore_delete(timer_ui_start_sem);
    
    if (timer_ui_stop_sem != NULL)
        ryos_semaphore_delete(timer_ui_stop_sem);    
    
    if (timer_ui_thread_handle != NULL)
        ryos_thread_delete(timer_ui_thread_handle);
    
    if (timer_ctx_v.untouch_timer_id) {
        ry_timer_stop(timer_ctx_v.untouch_timer_id);
        ry_timer_delete(timer_ctx_v.untouch_timer_id);    
    }   
    

//    wms_event_listener_del(WM_EVENT_MESSAGE,            &activity_timer);
    wms_event_listener_del(WM_EVENT_TOUCH_RAW,          &activity_timer);    
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED,    &activity_timer);
    wms_event_listener_del(WM_EVENT_CARD,               &activity_timer);

    motar_stop();
    
//    LOG_INFO("[activity_timer] destroy");
    
    ry_hal_wdt_reset();
    tms_ctx_v.disable_deepsleep = 0;
    
    return ret;
}

/**
 * @brief   Timer UI thread
 *
 * @param   None
 *
 * @return  None
 */
int timer_ui_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;

    status = ryos_semaphore_create(&timer_ui_start_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[timer_ui_thread]: start, sem create error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    status = ryos_semaphore_create(&timer_ui_stop_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[timer_ui_thread]: stop, sem create error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    bool motar_enable = false;
    
    while (1) {
        status = ryos_semaphore_wait(timer_ui_start_sem);
        if (RY_SUCC != status) {
            LOG_ERROR("[timer_ui_thread]: Semaphore Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        }
        
        uint32_t ui_update_took = 0;
        
        while (1) {
            
            /* Feed watchdog */
            ry_hal_wdt_reset();
            
            if (timer_ctx_v.mode == MODE_MENU) break;
            
            //ui_update_took = ry_hal_clock_time();
            
            timer_array[timer_ctx_v.sel_item].ui_update();
            
            //ui_update_took = ry_hal_calc_ms(ry_hal_clock_time() - ui_update_took);
            //LOG_ERROR("[timer_ui_update] time used %d ms\n", ui_update_took);

            if (timer_ctx_v.mode == MODE_MENU) break;
            
            ryos_delay_ms(200);
        }
        
        ryos_semaphore_post(timer_ui_stop_sem);
    }
}

ry_sts_t timer_ui_thread_init(void)
{
    ryos_threadPara_t para;
    
    /* Start timer UI thread */
    strcpy((char*)para.name, "timer_ui_thread");
    para.stackDepth = 768;
    para.arg        = NULL; 
    para.tick       = 5;
    para.priority   = 5;
    ryos_thread_create(&timer_ui_thread_handle, timer_ui_thread, &para); 
 
    return RY_SUCC;
}
