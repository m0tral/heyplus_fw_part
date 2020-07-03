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
#include "ry_font.h"

#include "scheduler.h"

/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "window_management_service.h"
#include "device_management_service.h"
#include "card_management_service.h"
#include "data_management_service.h"

#include "activity_card.h"
#include "activity_data.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "scrollbar.h"


/* resources */
#include "gui_img.h"
#include"gui_msg.h"
#include "ry_statistics.h"




/*********************************************************************
 * CONSTANTS
 */ 
#define DATA_ITEM_NUM           7
#define DATA_ITEM_NUM_ONCE      3
#define data_calc_step          (alg_get_step_today())
#define data_calc_km            (10) 
#define data_calc_bpm           (s_alg.hr_cnt) 
#define data_calc_sitting       5
#define data_calc_slee_h        7
#define data_calc_slee_m        30
#define SCROLLBAR_HEIGHT        40
#define SCROLLBAR_WIDTH         1


/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    int8_t    list_head;
} ry_data_ctx_t;

typedef struct
{
    char* title;
    const char* unit;
    uint8_t     pos_x;
    u32_t       color;
}data_unit_info_t;

/*********************************************************************
 * VARIABLES
 */
extern activity_t activity_surface;

ry_data_ctx_t data_ctx_v;

activity_t activity_data = {
    .name      = "data",
    .onCreate  = activity_data_onCreate,
    .onDestroy = activity_data_onDestrory,
    .priority = WM_PRIORITY_M,
};

const char* const data_icon[] = {
    "m_data_00_step.png", 
    "m_data_01_calorie.png",    
    "m_data_02_km.png",
    "m_data_03_hrm.png",
    "m_data_04_sitting.png",  
    "m_data_05_sleeping.png"
};

const char* const data_icon_bpm[] = {
    "m_data_00_step.bmp", 
    "m_data_01_calorie.bmp",    
    "m_data_02_km.bmp",
    "m_data_03_hrm.bmp",
    "m_data_04_sitting.bmp",  
    "m_data_05_sleeping.bmp"
};

const data_unit_info_t data_unit_info[] = {
    {"Шаги",          "",       68,  0x15D080},
    {"Калории",		  "ккал",   68,  0x15D080},
    {"Расстояние",	  "км",     0,   0x15D080},
    {"Пульс",         "уд/мин", 30,  0x15D080},
    {"Без движ.",     "раз",    30,  0x15D080},     
    {"Длит. сна",     "",       30,  0x15D080},
#if (DATA_ITEM_NUM >= 7)    
    {"Подъем в",      "",       30,  0x15D080},    
#endif    
};

scrollbar_t * data_scrollbar = NULL;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

ry_sts_t data_display_update(int8_t list_head)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t img_y;
    uint8_t str_y;
    gdispImage myImage;
    char strs[DATA_ITEM_NUM][20];
    uint8_t offset[DATA_ITEM_NUM] = {0};    //offset for 0.x values display
    
    clear_buffer_black();
    
    //prepare data
    sprintf(strs[0], "%d",  data_calc_step);
    sprintf(strs[1], "%d",  ((int)app_get_calorie_today())/1000);
    float distance = app_get_distance_today();
    distance = app_get_distance_today() / 1000.0f;
    if (distance < 0.0999f) {   
        sprintf(strs[2], "%d",  0);
    } 
    else {
        sprintf(strs[2], "%.01f",  distance);
        offset[2] = 8;
    }
    
    sprintf(strs[3], "%d",   data_calc_bpm);
#if (DATA_ITEM_NUM >= 5)
    sprintf(strs[4], "%d",   alg_get_long_sit_times());
#endif
    // sprintf(strs[5], "%d",   sleep_time.sleep_hour);
#if (DATA_ITEM_NUM >= 6)
    if (1) { // ((sleep_time.f_sleep_updated == 1) && (sleep_time.f_wakeup_updated == 1)){
        sprintf(strs[5], "%02d:%02d", sleep_time.sleep_total / 60, sleep_time.sleep_total % 60);
    }
    else {
        sprintf(strs[5], "--");
    }
#if (DATA_ITEM_NUM >= 7)
    if (1) { // sleep_time.f_wakeup_updated == 1){
        sprintf(strs[6], "%02d:%02d", sleep_time.wakeup_hour, sleep_time.wakeup_minute);
    }
    else {
        sprintf(strs[6], "--");
    }
#endif
#endif
    for (int i = 0; i < DATA_ITEM_NUM_ONCE; i++) {
        img_y = i * 80;
        str_y = img_y + 35;

        uint8_t w, h;
        /*draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)data_icon_bpm[i + list_head], \
                                    4, img_y, &w, &h, d_justify_left);*/

        gdispFillString(0, 
                        img_y, 
                        data_unit_info[i + list_head].title, 
                        (void*)font_roboto_20.font, 
                        White, Black);
                        
        color_t cl = ((data_unit_info[i + list_head].color & 0xff) << 16) \
                   | ((data_unit_info[i + list_head].color & 0xff0000) >> 16)\
                   | ((data_unit_info[i + list_head].color & 0x00ff00) >> 0);

        gdispFillString( 0, 
                        str_y - 2, 
                        strs[i + list_head], 
                        (void*)font_roboto_32.font,  
                        cl,
                        Black
                        );  
        gdispFillString( 18 * strlen(strs[i + list_head]) - offset[i + list_head], 
                        str_y, 
                        (char *)data_unit_info[i + list_head].unit, 
                        (void*)font_roboto_20.font, 
                        data_unit_info[i + list_head].color,
                        Black
                        );  
#if 0 & (DATA_ITEM_NUM >= 6)            
        //patch for sleep time display
        if ((i + list_head) >= (sizeof(data_unit_info)/sizeof(data_unit_info_t) - 1)){
            uint32_t pos = 17 * strlen(strs[i + list_head]) + 24;
            if ((i + list_head) == (sizeof(data_unit_info)/sizeof(data_unit_info_t) - 1)){
                if ((sleep_time.f_sleep_updated == 1) && (sleep_time.f_wakeup_updated == 1)){
                    sprintf(strs[5], "%02d",   sleep_time.sleep_total % 60);
                }
                else{
                    sprintf(strs[5], "--");
                }
            }
#if (DATA_ITEM_NUM >= 7)            
            else{
                if (sleep_time.f_wakeup_updated == 1){
                    sprintf(strs[6], "%02d",   sleep_time.wakeup_minute);
                }
                else{
                    sprintf(strs[6], "--");
                }
            }
#endif
            gdispFillString( pos, 
                                str_y, 
                                strs[i + list_head], 
                                (void*)font_roboto_20.font,  
                                cl,  
                                Black
                            );  
            gdispFillString( pos + 17 * 2, 
                                str_y, 
                                (char *)"мин", 
                                (void*)font_roboto_20.font, 
                                data_unit_info[i + list_head].color,
                                Black
                            );                                
        }
#endif
    }

    if(data_scrollbar != NULL){
        
        int startY, delta;
        
        delta = 200 / (DATA_ITEM_NUM - DATA_ITEM_NUM_ONCE);
        startY = list_head * delta;        
        
        scrollbar_show(data_scrollbar, frame_buffer, startY);
    }

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    //update_buffer_to_oled();
    return ret;
}


int data_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[data_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if ((p->y >= 8)){
                wms_activity_back(NULL);
                return consumed;
            }

            if ((p->y <= 5)){
                // card_info_update(data_ctx_v.list_head);
            }

            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG	("[data_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            data_ctx_v.list_head ++;
            if (data_ctx_v.list_head >= DATA_ITEM_NUM - DATA_ITEM_NUM_ONCE){
                data_ctx_v.list_head = DATA_ITEM_NUM - DATA_ITEM_NUM_ONCE;
            }
            DEV_STATISTICS(data_slide_up);
            data_display_update(data_ctx_v.list_head);
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            // To see message list_head
            LOG_DEBUG("[data_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);
            data_ctx_v.list_head --;
            if (data_ctx_v.list_head <= 0){
                data_ctx_v.list_head = 0;
            }
            DEV_STATISTICS(data_slide_down);
            data_display_update(data_ctx_v.list_head);
            break;

        default:
            break;
    }
    return consumed; 
}
int data_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}


int data_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}



/**
 * @brief   Entry of the Data activity
 *
 * @param   None
 *
 * @return  acvivity_data_onCreate result
 */
ry_sts_t activity_data_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    if(data_scrollbar == NULL){
        data_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);
    }
    
    ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_DATA_CHECK_WAKEUP, 0, NULL);
    
    LOG_DEBUG("[activity_data_onCreate]\r\n");
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_data, data_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_data, data_onMSGEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_data, data_onProcessedCardEvent);

    data_ctx_v.list_head = 0;
    /* Add Layout init here */
    data_display_update(data_ctx_v.list_head);

    return ret;
}

/**
 * @brief   API to exit data activity
 *
 * @param   None
 *
 * @return  data activity Destrory result
 */
ry_sts_t activity_data_onDestrory(void)
{
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_data);
    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_data);
    wms_event_listener_del(WM_EVENT_CARD, &activity_data);

    if(data_scrollbar != NULL){
        scrollbar_destroy(data_scrollbar);
        data_scrollbar = NULL;
    }

    return RY_SUCC;    
}





