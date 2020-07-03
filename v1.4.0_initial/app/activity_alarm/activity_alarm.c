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
#include "app_config.h"
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"
#include "ryos_timer.h"
#include <ry_hal_inc.h>
#include "scheduler.h"


/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>
#include "timer_management_service.h"
#include "window_management_service.h"
#include "card_management_service.h"
#include "AlarmClock.pb.h"
#include "activity_alarm.h"
#include "activity_card.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "ry_statistics.h"

/* resources */
#include "gui_img.h"
#include "ry_font.h"
#include"gui_msg.h"
#include "scrollbar.h"



/*********************************************************************
 * CONSTANTS
 */ 
#define ALARM_ITEM_NUM           10
#define ALARM_ITEM_NUM_ONCE      2

/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    ALARM_LIST_DISP,  
    ALARM_SETTING,
} alarm_disp_state_t;

typedef struct {
    int8_t    head;
    int8_t    setting_item;    
    uint8_t   cur_num;
    uint8_t   state;
    AlarmClockList* alarm_list;
} alarm_ui_t;


typedef struct {
    int8_t* descrp;
    uint8_t repeat;
    uint8_t day;    
    uint8_t hour;
    uint8_t min; 
    uint8_t active;
}alarm_t;
 
/*********************************************************************
 * VARIABLES
 */

activity_t activity_alarm = {
    .name      = "alarm",
    .onCreate  = activity_alarm_onCreate,
    .onDestroy = activity_alarm_onDestroy,
    .priority  = WM_PRIORITY_H,
};

alarm_ui_t alarm_ui_v = {0, 0, 5, ALARM_LIST_DISP};
scrollbar_t * alarm_scrollbar = NULL;


alarm_t alarm_array[] = {
    {"Wakeup",  1, 1, 8,  30, 1},
    {"Run",  0, 7, 9,  55, 0},
    {"Bejing",  1, 8, 10, 30, 1},
    {"Mijia",  0, 3, 14, 05, 0},
    {"upgrade",  0, 5, 22, 55, 1},    
};

const color_t active_color[] = {
    HTML2COLOR(0x505050),   //in-active color
    HTML2COLOR(0x85D115),   //active color
};

const uint8_t* const active_icon[] = {
    "g_widget_close_11.bmp",
    "g_widget_on_09.bmp",
};

const char* const text_add_alarm [] = {
		"Добавьте",
		"в телефоне",
};

const char* const repeat_day[] = {
#if 1
    "Один раз",
    "Ежедневно",
    "Будни",
    "Выходные",
        
    "ПН",
    "ВТ",
    "СР",
    "ЧТ",
    "ПТ",
    "СБ",
    "ВС",  

    "ПН...",
    "ВТ...",
    "СР...",
    "ЧТ...",
    "ПТ...",
    "СБ...",
    "ВС...",  
#else
    "never",
    "every day",
    "working day",
    "non-working day",

    "monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday",
    "sunday",

    "monday...",
    "Tuesday...",
    "Wednesday...",
    "Thursday...",
    "Friday...",
    "Saturday...",
    "sunday...",
#endif
};


#if 0
char repeat_debug[][9] = {
    {0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {4, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {5, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},

    {1, 2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {4, 5, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {6, 7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {1, 2, 3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    {1, 2, 3, 4, 0xff, 0xff, 0xff, 0xff, 0xff},
    {1, 2, 3, 4, 5, 0xff, 0xff, 0xff, 0xff},
    {1, 2, 3, 4, 5, 6, 0xff, 0xff, 0xff},
    {1, 2, 3, 4, 5, 6, 7, 0xff, 0xff},
};
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to get_alarm_repeat_string
 *
 * @param   repeat: pointer to the alarm repeat paraments
 *
 * @return  pointer to the alarm repeat string
 */
char* get_alarm_repeat_string(uint8_t* repeat)
{
    int repeat_cnt = 0;
    int repeat_next = 0;
    int repeat_max = 0;
    int repeat_min = 0;
    char* repeat_str = NULL;
    int weekday;

    //for( int k = 0; k < sizeof(repeat_debug)/9; k ++)
    {
        // repeat = repeat_debug[0] + k * 9;
        repeat_cnt = 0;
        repeat_next = 0;
        repeat_max = 0;
        repeat_min = 0;
        for (int i = MAX_ALARM_REPEAT_TYPE_NUM - 1; i >= 0 ; i--) {
            for(weekday = 1; weekday <= 7; weekday ++){
                if (repeat[i] == weekday){
                    repeat_cnt ++;
                    if (repeat_max == 0){
                        repeat_max = weekday;
                    }
                    repeat_min = weekday;    
                }
            }

            // find the next repeat day
            for(weekday = hal_time.ui32Weekday; weekday < 8 + hal_time.ui32Weekday; weekday ++){
                // LOG_DEBUG("[next_repeat] weekday: %d, repeat[%d]: %d, weekday_mod_8: %d\r\n",\
                    weekday, i, repeat[i], weekday%8);
                if (repeat[i] == (weekday % 8)){
                    repeat_next = weekday % 8;   
                    break;  
                }
            }
        }
        uint32_t idx = 0;

        if (repeat_cnt == 0){
            repeat_str = (char *)repeat_day[0];                     //only once
            idx = 0;
        }
        else if (repeat_cnt == 7){
            repeat_str = (char *)repeat_day[1];                     //all day
            idx = 1;
        }
        else if (repeat_cnt == 1){
            repeat_str = (char *)repeat_day[3 + repeat_next];       //every one day 
            idx = 3 + repeat_next;
        }
        else if (repeat_cnt > 1){
            if ((repeat_cnt == 5) && (repeat_max == 5)){
                repeat_str = (char *)repeat_day[2];                 //working day 
                idx = 2;
            }
            else if ((repeat_cnt == 2) && (repeat_max == 7) && (repeat_min == 6)){
                repeat_str = (char *)repeat_day[3];                 //not working day 
                idx = 3;
            }
            else{
                repeat_str = (char *)repeat_day[10 + repeat_next];  //more than one day
                idx = 10 + repeat_next;
            }
        }

        //LOG_DEBUG("[get] k: %d, repeat_cnt: %d, repeat_next: %d, repeat_max: %d, repeat_min: %d, idx_string: %d\r\n\r\n", \
            0, repeat_cnt, repeat_next, repeat_max, repeat_min, idx);
    }
    return repeat_str;
}

ry_sts_t alarm_display_update(int8_t list_head)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t str_y;
    char time[16];
    AlarmClockItem *p;
    u8_t repeatType;
    int startY, delta;

    LOG_DEBUG("[alarm_display_update] begin.list_head:%d, alarm_ui_v.cur_num: %d\r\n", list_head, alarm_ui_v.cur_num);
    clear_buffer_black();

    if (alarm_ui_v.cur_num == 0) {
        gdispFillStringBox( 0, 
                            136, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_add_alarm[0], 
                            (void*)font_roboto_20.font,
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        gdispFillStringBox( 0, 
                            164, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_add_alarm[1], 
                            (void*)font_roboto_20.font,
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 


        uint8_t w, h;
        draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"m_no_alarm.bmp",\
                                0, 68, &w, &h, d_justify_center);
        goto display;
    }


    str_y = (alarm_ui_v.cur_num > 1) ? 0 : 60;
    for (int i = 0; (i < ALARM_ITEM_NUM_ONCE) && (i < alarm_ui_v.cur_num); i++){
        p = &alarm_ui_v.alarm_list->items[i + list_head];
        if (p->repeats[8] != 0xFF) {
            repeatType = p->repeats[8];
        } else {
            repeatType = p->repeats[0];
        }
        sprintf((char*)time, "%02d:%02d", p->hour, p->minute); 
        gdispFillStringBox( 0, 
                            str_y + 4 + 128 * i, 
                            font_roboto_32.width,
                            font_roboto_32.height,
                            time, 
                            (void*)font_roboto_32.font, 
                            active_color[p->enable],  
                            Black,
                            justifyCenter
                            );  
        gdispFillStringBox( 0, 
                            str_y + 48 + 128 * i, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            get_alarm_repeat_string(p->repeats), 
                            (void*)font_roboto_20.font, 
                            White,
                            Black,
                            justifyCenter
                            ); 
        gdispFillStringBox( 0, 
                            str_y + 80 + 128 * i, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)p->tag, 
                            (void*)font_roboto_20.font, 
                            White,  
                            Black,
                            justifyCenter
                            ); 
    }

    if (alarm_ui_v.cur_num >= ALARM_ITEM_NUM_ONCE){
        gdispDrawLine(4, 124, 120 - 4, 124, HTML2COLOR(0x303030));                    
    }

display:
    if (alarm_ui_v.cur_num > 2) {
        delta = 200 / (alarm_ui_v.cur_num - 2);
        startY = list_head * delta;
        scrollbar_show(alarm_scrollbar, frame_buffer, startY);
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    LOG_DEBUG("[alarm_display_update] end.\r\n");
    return ret;
}


ry_sts_t alarm_setting_update(int8_t setting_item)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t str_y;
    uint8_t time[16];
    AlarmClockItem* p;
    u8_t repeatType;

    LOG_DEBUG("[alarm_setting_update] begin.\r\n");
    clear_buffer_black();
    str_y = 0;
    {
        p = &alarm_ui_v.alarm_list->items[setting_item];
        if (p->repeats[8] != 0xFF) {
            repeatType = p->repeats[8];
        } else {
            repeatType = p->repeats[0];
            
        }
        if (repeatType == 0xFF) {
            repeatType = 0;
        }

        sprintf((char*)time, "%02d:%02d", p->hour, p->minute); 
        gdispFillStringBox( 0, 
                            str_y + 4, 
                            font_roboto_32.width,
                            font_roboto_32.height,
                            (char*)time, 
                            (void*)font_roboto_32.font, 
                            active_color[p->enable],
                            Black,
                            justifyCenter
                            );  
        gdispFillStringBox( 0, 
                            str_y + 48, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            get_alarm_repeat_string(p->repeats),
                            (void*)font_roboto_20.font, 
                            White, Black,
                            justifyCenter
                            ); 
        gdispFillStringBox( 0, 
                            str_y + 80, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            p->tag, 
                            (void*)font_roboto_20.font, 
                            White, Black, 
                            justifyCenter
                            ); 
    }

    uint8_t w, h;
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)active_icon[!p->enable],\
                            0, 140, &w, &h, d_justify_center);

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    LOG_DEBUG("[alarm_setting_update] end.\r\n");
    return ret;
}


int alarm_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;

    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[alarm_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if ((p->y >= 8) || (alarm_ui_v.cur_num == 0)){
                if (alarm_ui_v.state == ALARM_SETTING){
                    alarm_ui_v.state = ALARM_LIST_DISP;
                    alarm_display_update(alarm_ui_v.head);
                }
                else{
                    wms_activity_back(NULL);
                }
                return consumed;
            }

            if ((p->y <= 3)){
                if (alarm_ui_v.state == ALARM_LIST_DISP){
                    alarm_ui_v.state = ALARM_SETTING;
                    alarm_ui_v.setting_item = alarm_ui_v.head;
                    alarm_setting_update(alarm_ui_v.setting_item);
                    DEV_STATISTICS(alarm_info_count);
                }
            } else if ((p->y > 3) && (p->y <= 7)){
                if (alarm_ui_v.state == ALARM_LIST_DISP){
                    alarm_ui_v.state = ALARM_SETTING;
                    alarm_ui_v.setting_item = (alarm_ui_v.cur_num >= ALARM_ITEM_NUM_ONCE) ? \
                        (alarm_ui_v.head + 1) : alarm_ui_v.head;
                    alarm_setting_update(alarm_ui_v.setting_item);
                    DEV_STATISTICS(alarm_info_count);                                        
                }
                else if (alarm_ui_v.state == ALARM_SETTING){
                    alarm_ui_v.alarm_list->items[alarm_ui_v.setting_item].enable = !alarm_ui_v.alarm_list->items[alarm_ui_v.setting_item].enable;
                    if (alarm_ui_v.alarm_list->items[alarm_ui_v.setting_item].enable){
                        DEV_STATISTICS(alarm_open_count);                                        
                    }
                    else{
                        DEV_STATISTICS(alarm_close_count);                                        
                    }
                    alarm_set_enable(alarm_ui_v.alarm_list->items[alarm_ui_v.setting_item].id, alarm_ui_v.alarm_list->items[alarm_ui_v.setting_item].enable);                    
                    alarm_ui_v.state = ALARM_LIST_DISP;
                    alarm_display_update(alarm_ui_v.head);
                }
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG	("[alarm_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            if ((alarm_ui_v.state == ALARM_LIST_DISP) && (alarm_ui_v.cur_num >= ALARM_ITEM_NUM_ONCE)){
                alarm_ui_v.head ++;
                if (alarm_ui_v.head >= alarm_ui_v.cur_num - ALARM_ITEM_NUM_ONCE){
                    alarm_ui_v.head = alarm_ui_v.cur_num - ALARM_ITEM_NUM_ONCE;
                }
                alarm_display_update(alarm_ui_v.head);
            }    
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            // To see message list_head
            LOG_DEBUG("[alarm_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);
            if ((alarm_ui_v.state == ALARM_LIST_DISP) && (alarm_ui_v.cur_num >= ALARM_ITEM_NUM_ONCE)){
                alarm_ui_v.head --;
                if (alarm_ui_v.head <= 0){
                    alarm_ui_v.head = 0;
                }
                alarm_display_update(alarm_ui_v.head);
            }
            break;

        default:
            break;
    }
    LOG_DEBUG("[alarm_onProcessedTouchEvent] TP_Action Click, state: %d\r\n", alarm_ui_v.state);
    
    return consumed; 
}


int alarm_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}

int alarm_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}


/**
 * @brief   Entry of the Alarm activity
 *
 * @param   None
 *
 * @return  activity_alarm_onCreate result
 */
ry_sts_t activity_alarm_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    LOG_DEBUG("[activity_alarm_onCreate]\r\n");
    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_alarm, alarm_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_alarm, alarm_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_alarm, alarm_onProcessedCardEvent);

    alarm_ui_v.state = ALARM_LIST_DISP;
    alarm_ui_v.alarm_list = (AlarmClockList*)ry_malloc(sizeof(AlarmClockList));
    alarm_list_get(alarm_ui_v.alarm_list);
    alarm_ui_v.cur_num = alarm_ui_v.alarm_list->items_count;

    alarm_ui_v.head = 0;

    if (alarm_scrollbar == NULL){
        alarm_scrollbar = scrollbar_create(1, 40, 0x4A4A4A, 1000);
    }
    
    /* Add Layout init here */
    alarm_display_update(alarm_ui_v.head);

    //if (alarm_ui_v.cur_num != 0){
    //    alarm_display_update(alarm_ui_v.head);
    //}
    return ret;
}

/**
 * @brief   API to exit Alarm activity
 *
 * @param   None
 *
 * @return  alarm activity Destrory result
 */
ry_sts_t activity_alarm_onDestroy(void)
{
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_alarm);
    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_alarm);
    wms_event_listener_del(WM_EVENT_CARD, &activity_alarm);

    if (alarm_scrollbar != NULL){
        scrollbar_destroy(alarm_scrollbar);
        alarm_scrollbar = NULL;
    }

    if (alarm_ui_v.alarm_list) {
        ry_free((u8_t*)alarm_ui_v.alarm_list);
        alarm_ui_v.alarm_list = NULL;
    }

    return RY_SUCC;   
}
