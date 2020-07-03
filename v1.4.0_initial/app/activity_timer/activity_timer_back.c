#include <stdio.h>
#include "ry_font.h"
#include "touch.h"
#include "led.h"
#include "gui.h"
#include "gui_bare.h"
#include "gui_img.h"
#include "gui_msg.h"
#include "scrollbar.h"

#include "ryos_timer.h"
#include "board_control.h"
#include "ry_hal_inc.h"

#include "timer_management_service.h"
#include "card_management_service.h"

#include "activity_card.h"
#include "activity_timer.h"

ry_sts_t timer_onCreate(void * usrdata);
ry_sts_t timer_onDestroy(void);

ry_sts_t timer_menu_display_update(u8_t top_sport);

void stopwatch_on_action(uint32_t y);
void countdown_on_action(uint32_t y);


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
    STATE_MENU,
    STATE_STOPWATCH,
    STATE_COUNTDOWN,
    STATE_COUNTDOWN_DONE,
    STATE_EXIT
} timer_mode_t;

typedef struct {
    int8_t       top_menu_item;   
    u8_t         sel_item;
    u32_t        start_time;
    u32_t        duration;
    u32_t        countdown;
    bool         is_ending;
    u32_t        motar_start_clock;     // MCU clock time, not msec
    timer_mode_t mode;
    bool         is_running;
    bool         cmd_run;
    u32_t        exit_timer_id;
    u32_t        ui_timer_id;
    u32_t        untouch_timer_id;
} timer_ctx_t;

typedef struct{
    const char* descrp;
    const char* img;
    void        (*on_action)(uint32_t y);        
} timer_item_t;

/*********************************************************************
 * VARIABLES
 */
const timer_item_t timer_array[] = {
    {"Секундомер",  "menu_stopwatch.bmp",  stopwatch_on_action},
    {"Таймер",      "menu_countdown.bmp",  countdown_on_action},
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
    "g_widget_03_stop2.bmp",
};

activity_t activity_timer = {
    .name = "timer_stopwatch", 
    .onCreate = timer_onCreate,
    .onDestroy = timer_onDestroy,
    .disableUntouchTimer = 0,
    .priority = WM_PRIORITY_M,
};


volatile timer_ctx_t    timer_ctx_v;
scrollbar_t*            timer_scrollbar = NULL;

volatile bool is_drawing = false;

/*********************************************************************
 * FUNCTIONS
 */

void start_timer(void);
void stop_timer(void);
void timer_untouch_timer_start(void);


ry_sts_t timer_menu_display_update(u8_t top_sport)
{
    //LOG_ERROR("[menu_disp] mode:%d\n", timer_ctx_v.mode);    
    
    u8_t w, h;
    clear_buffer_black();
    img_exflash_to_framebuffer((uint8_t*)timer_array[top_sport].img, \
                                22, 4, &w, &h, d_justify_center);
    gdispFillStringBox( 0,                    
                        90,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char*)timer_array[top_sport].descrp,        
                        (void *)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);
#if (TIMER_ITEM_NUM > 1)
    img_exflash_to_framebuffer((uint8_t*)timer_array[(top_sport + 1) % TIMER_ITEM_NUM].img, \
                                22, 128, &w, &h, d_justify_center);

    gdispFillStringBox( 0,                    
                        214,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char*)timer_array[(top_sport + 1) % TIMER_ITEM_NUM].descrp,        
                        (void *)font_roboto_20.font,
                        White, Black,
                        justifyCenter);
#endif

    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    return RY_SUCC;
}

void timer_countdown_disp(ry_time_t cur_time, u32_t duration_second, bool start)
{
    if (timer_ctx_v.mode == STATE_MENU) return;
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
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)action_icon[start],\
                            0, 140, &w, &h, d_justify_center);                        

    ry_free(info_str);

    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        
    is_drawing = false;
}

void timer_countdown_disp_partial(ry_time_t cur_time, u32_t duration_second, bool start)
{
    if (timer_ctx_v.mode == STATE_MENU) return;
    if (is_drawing) return;

    is_drawing = true;
    
    char * info_str = (char *)ry_malloc(40);
    clear_frame_area(0, RM_OLED_WIDTH - 1, 0, 110);
    
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

    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        
    is_drawing = false;
}

void timer_stopwatch_disp(ry_time_t cur_time, u32_t duration_second, bool start)
{
    if (timer_ctx_v.mode == STATE_MENU) return;
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
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)action_icon[start],\
                            0, 140, &w, &h, d_justify_center);                        

    ry_free(info_str);

    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        
    is_drawing = false;                        
}

void timer_stopwatch_disp_partial(ry_time_t cur_time, u32_t duration_second, bool start)
{
    if (timer_ctx_v.mode == STATE_MENU) return;
    if (is_drawing) return;    

    is_drawing = true;    
    
    char * info_str = (char *)ry_malloc(40);
    clear_frame_area(0, RM_OLED_WIDTH - 1, 0, 110);
    
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

    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        
    is_drawing = false;                        
}

void send_wms_message(void)
{
    wms_msg_t wms_param = {0};
    wms_param.msgType = IPC_MSG_TYPE_ACTIVITY_HRM_REFRESH;
    wms_msg_send(IPC_MSG_TYPE_ACTIVITY_HRM_REFRESH, sizeof(wms_msg_t), (u8_t*)&wms_param);    
}

void running_activity_timer_handler(void* arg)
{
//    if (timer_ctx_v.mode == STATE_COUNTDOWN_DONE)
//        LOG_ERROR("timer_tick: handler start");    
    
    if (timer_ctx_v.mode == STATE_MENU) return;
    if (timer_ctx_v.ui_timer_id == 0) return;
    
    if(strcmp(wms_activity_get_cur_name(), activity_timer.name) != 0) {
        
        //LOG_ERROR("timer_tick: exit timer, wrong activity..");
        
        motar_stop();
        stop_timer();
    }
    
    //send_wms_message();
    //wms_untouch_timer_stop();
    
    if (timer_ctx_v.mode == STATE_STOPWATCH) {
        stopwatch_on_action(0);
    }
    else if (timer_ctx_v.mode == STATE_COUNTDOWN) {
        countdown_on_action(0);
    }
    else if (timer_ctx_v.mode == STATE_COUNTDOWN_DONE)
    {
        //LOG_ERROR("timer_tick: STATE_COUNTDOWN_DONE");
        
        if (ry_hal_calc_ms(ry_hal_clock_time() - timer_ctx_v.motar_start_clock) > COUNTDOWN_MOTAR_THRESHOLD)
        {
            //LOG_ERROR("motar stopped, exit timer..");
            
            motar_stop();
            ry_led_onoff(0);
                        
            stop_timer();
            
            timer_untouch_timer_start();
            
            //ry_hal_wdt_reset();
            
            //send_wms_message(); // to prevent watchdog timer reset
            //return;
        }
    }
        
    if (timer_ctx_v.ui_timer_id) {
        ry_timer_stop(timer_ctx_v.ui_timer_id);
        ry_timer_start(timer_ctx_v.ui_timer_id, TIMER_UI_REFRESH_PERIOD, running_activity_timer_handler, NULL);       
    }
}

void start_timer(void)
{
    timer_ctx_v.is_ending = false;    
    
    if (timer_ctx_v.ui_timer_id == 0) {
        ry_timer_parm timer_para;
        timer_para.timeout_CB = running_activity_timer_handler;
        timer_para.flag = 0;    // multiply event timer
        timer_para.data = NULL;
        timer_para.tick = 1;    // doesnt matter what is here
        timer_para.name = "stopwatch_timer";
        timer_ctx_v.ui_timer_id = ry_timer_create(&timer_para);
	}    

//    LOG_ERROR("timer started, mode: %d, time: %d secs, timer_id: %d", timer_ctx_v.mode, timer_ctx_v.duration, timer_ctx_v.ui_timer_id);
    
    ry_timer_start(timer_ctx_v.ui_timer_id, TIMER_UI_REFRESH_PERIOD, running_activity_timer_handler, NULL);
}

void stop_timer(void)
{    
    timer_ctx_v.is_running = false;
    timer_ctx_v.cmd_run = false;
    
    if (timer_ctx_v.ui_timer_id) {

        ry_timer_stop(timer_ctx_v.ui_timer_id);
        ry_timer_delete(timer_ctx_v.ui_timer_id);
        timer_ctx_v.ui_timer_id = 0;
        
//        LOG_ERROR("timer stopped");        
    }
}

void stopwatch_on_action(uint32_t y)
{
    //LOG_ERROR("[stopwatch_on_action] mode:%d\n", timer_ctx_v.mode);    
    
    if (timer_ctx_v.mode == STATE_EXIT) return;
    
    bool full_update = false;
    bool got_cmd = false;
    if (timer_ctx_v.cmd_run != timer_ctx_v.is_running) {
        got_cmd = true;
        full_update = true;
    }
            
    //time now
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time); 
    u32_t duration_msec = 0;    
    
    if (timer_ctx_v.mode == STATE_MENU) {
        timer_ctx_v.mode = STATE_STOPWATCH;
        full_update = true;
    }
    
    if (got_cmd) {
        if (timer_ctx_v.cmd_run) {
            timer_ctx_v.start_time = ryos_rtc_now();            
            start_timer();
        }
        else {
            stop_timer();
        }
        timer_ctx_v.is_running = timer_ctx_v.cmd_run;
    }    
    else {
        if (timer_ctx_v.is_running)
            timer_ctx_v.duration = ryos_rtc_now() - timer_ctx_v.start_time;    
    }

    if (full_update)
        timer_stopwatch_disp(cur_time, timer_ctx_v.duration, timer_ctx_v.is_running);
    else
        timer_stopwatch_disp_partial(cur_time, timer_ctx_v.duration, timer_ctx_v.is_running);
}

void countdown_on_action(uint32_t y)
{
    //LOG_ERROR("[countdown_on_action] mode:%d\n", timer_ctx_v.mode);
    
    if (timer_ctx_v.mode == STATE_EXIT) return;
    
    bool full_update = false;
    bool got_cmd = false;
    if (timer_ctx_v.cmd_run != timer_ctx_v.is_running) {
        got_cmd = true;
        full_update = true;
    }
            
    //time now
    ry_time_t cur_time = {0};
    tms_get_time(&cur_time); 
    
    if (timer_ctx_v.mode == STATE_MENU) {
        timer_ctx_v.mode = STATE_COUNTDOWN;
        full_update = true;
    }
    
    timer_ctx_v.duration = timer_ctx_v.countdown;
    
    if (got_cmd) {
        if (timer_ctx_v.cmd_run) {
            timer_ctx_v.start_time = ryos_rtc_now();            
            start_timer();
        }
        else {
            stop_timer();
        }
        timer_ctx_v.is_running = timer_ctx_v.cmd_run;
    }    
    else {
        if (timer_ctx_v.is_running) {
            u32_t delta = ryos_rtc_now() - timer_ctx_v.start_time;
                        
            if (delta >= timer_ctx_v.countdown) {
                //LOG_ERROR("countdown complete. motar started..");
                
                full_update = true;
                timer_ctx_v.is_running = false;

                timer_ctx_v.duration = 0;
                timer_ctx_v.mode = STATE_COUNTDOWN_DONE;
                timer_ctx_v.motar_start_clock = ry_hal_clock_time();
                motar_set_work(MOTAR_TYPE_QUICK_LOOP, 3);
            }
            else {
                timer_ctx_v.duration = timer_ctx_v.countdown - delta;
                
                if (timer_ctx_v.duration < 9){
                    if (!timer_ctx_v.is_ending)
                    {
                        wms_screenOnOff(1);
                        timer_ctx_v.is_ending = true;
                    }
                }
            }
        }
    }

    if (full_update) {
        ry_hal_wdt_reset();
        timer_countdown_disp(cur_time, timer_ctx_v.duration, timer_ctx_v.is_running);
    }
    else
        timer_countdown_disp_partial(cur_time, timer_ctx_v.duration, timer_ctx_v.is_running);            
}

void timer_untouch_timeout_handler(void* arg)
{
    wms_screenOnOff(0);
    
    if (timer_ctx_v.mode == STATE_COUNTDOWN_DONE ||
        timer_ctx_v.mode == STATE_MENU)
    {
        wms_activity_back(NULL);
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
                
                timer_ctx_v.mode = STATE_MENU;

                if (timer_array[timer_ctx_v.sel_item].on_action != NULL){
                    timer_array[timer_ctx_v.sel_item].on_action(p->y);
                }
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
            if (timer_ctx_v.mode == STATE_COUNTDOWN &&
                !timer_ctx_v.is_running)
            {
                if (timer_ctx_v.countdown > 60 * 60)
                    timer_ctx_v.countdown -= 15 * 60;
                else if (timer_ctx_v.countdown > 10 * 60)
                    timer_ctx_v.countdown -= 5 * 60;
                else if (timer_ctx_v.countdown > 60)
                    timer_ctx_v.countdown -= 60;
                else if (timer_ctx_v.countdown > 10)
                    timer_ctx_v.countdown -= 10;
                
                timer_array[timer_ctx_v.sel_item].on_action(p->y);
            }
            
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            if (timer_ctx_v.mode == STATE_COUNTDOWN &&
                !timer_ctx_v.is_running)
            {
                if (timer_ctx_v.countdown >= 60 * 60)
                    timer_ctx_v.countdown += 15 * 60;
                else if (timer_ctx_v.countdown >= 10 * 60)
                    timer_ctx_v.countdown += 5 * 60;
                else if (timer_ctx_v.countdown >= 60)
                    timer_ctx_v.countdown += 60;
                else
                    timer_ctx_v.countdown += 10;
                    
                timer_array[timer_ctx_v.sel_item].on_action(p->y);
            }
            
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y < 8){
                if ((p->y <= 3)){
                }
                else if ((p->y > 3) && (p->y <= 7)){   
                    
                    if (timer_ctx_v.mode == STATE_COUNTDOWN_DONE)
                        timer_ctx_v.mode = STATE_COUNTDOWN;
                    
                    timer_ctx_v.cmd_run = !timer_ctx_v.is_running;
                    timer_array[timer_ctx_v.sel_item].on_action(p->y);
                }
            }
            else if ((p->y >= 8)){
                stop_timer();
                timer_ctx_v.mode = STATE_EXIT;
                timer_ctx_v.duration = 0;
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
    wms_untouch_timer_stop();
    timer_untouch_timer_start();
    
    return 0;
}

int timer_option_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;    
    
    switch (timer_ctx_v.mode)
    {
        case STATE_STOPWATCH:
            consumed = event_stopwatch_touch(p);
            break;
        case STATE_COUNTDOWN:
        case STATE_COUNTDOWN_DONE:
            consumed = event_stopwatch_touch(p);
            break;
        case STATE_MENU:
        case STATE_EXIT:
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
    ry_sts_t ret = RY_SUCC;
    ry_timer_parm timer_para;

    ry_memset((void*)&timer_ctx_v, 0, sizeof(timer_ctx_t));
    
    timer_ctx_v.mode = STATE_MENU;
    timer_ctx_v.countdown = COUNTDOWN_DEFAULT;
    
    if(charge_cable_is_input()){
        charging_timer_exit();
        return ret;
    }
    
    timer_para.timeout_CB = timer_untouch_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "timer_untouch_timer";
    timer_ctx_v.untouch_timer_id = ry_timer_create(&timer_para);    
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);    
    ret = timer_menu_display_update(timer_ctx_v.top_menu_item);
    
    if (timer_scrollbar == NULL) {
        timer_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);
    }

//    wms_event_listener_add(WM_EVENT_MESSAGE,            &activity_timer, timer_ui_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_RAW,          &activity_timer, timer_option_onTouchRawEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED,    &activity_timer, timer_option_onTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD,               &activity_timer, timer_ui_onProcessedCardEvent);
    
    return ret;
}


ry_sts_t timer_onDestroy(void)
{
    ry_sts_t ret = RY_SUCC;
    
    stop_timer();
    
    if (timer_ctx_v.untouch_timer_id) {
        ry_timer_stop(timer_ctx_v.untouch_timer_id);
        ry_timer_delete(timer_ctx_v.untouch_timer_id);    
    }

//    wms_event_listener_del(WM_EVENT_MESSAGE,            &activity_timer);
    wms_event_listener_del(WM_EVENT_TOUCH_RAW,          &activity_timer);    
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED,    &activity_timer);
    wms_event_listener_del(WM_EVENT_CARD,               &activity_timer);

    return ret;
}
