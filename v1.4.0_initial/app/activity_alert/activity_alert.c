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
#include "activity_alert.h"
#include "activity_card.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"

/* resources */
#include "gui_img.h"
#include "ry_font.h"
#include "gui_msg.h"
#include "ry_statistics.h"

#include "motar.h"

ry_sts_t alert_sitting_too_long_disp_update(void);
ry_sts_t alert_batt_low_disp_update(void);
ry_sts_t alert_goal_reached_disp_update(void);

/*********************************************************************
 * CONSTANTS
 */ 


/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    int8_t      evt;
    u8_t        screen_onOff;
    u8_t        animate_play;
    u8_t        animate_finished;  
    u8_t        animate_img_idx;
} alert_ctx_t;

typedef struct{
    uint32_t    exit_time;
    const char* img;
    ry_sts_t    (*display)(void);
}alert_info_t;

/*********************************************************************
 * VARIABLES
 */

activity_t activity_alert = {
    .name      = "alert",
    .onCreate  = activity_alert_onCreate,
    .onDestroy = activity_alert_onDestrory,
    .disableUntouchTimer = 1,
    .priority  = WM_PRIORITY_H,
    .disableWristAlg = 1,    
};

static alert_ctx_t  alert_ctx_v;
static u32_t        app_alert_timer_id;
const  alert_info_t alert_array[] = {
    {8000,	 "m_status_02_charging2.bmp",   alert_batt_low_disp_update},
    {10000,  "m_long_sit_2.bmp",            alert_sitting_too_long_disp_update},
    {10000,  "m_target_done_1.bmp",         alert_goal_reached_disp_update},    
};

const char* long_sit_img[] = {
    "m_long_sit_1.bmp",
    "m_long_sit_2.bmp",
    "m_long_sit_3.bmp",
    "m_long_sit_2.bmp",
};

const u8_t height_long_sit_img[] = {39, 73, 39, 73};

const char* target_dong_img[] = {
    "m_target_done_1.bmp",
    "m_target_done_2.bmp",
};

static const char* text_too_long_sitting[] = {
	"Вы долго",
	"сидите.",
	"Пройдитесь",
};

static char const* const text_target_done[] = {
	"Вы",
    "достигли",
	"цели!",
};

static char const* const text_low_battery[] = {
	"Батарея",
	"разряжена",
};


ryos_thread_t* animate_thread_handle = NULL;
ryos_mutex_t* alert_mutex = NULL;

/*********************************************************************
 * LOCAL FUNCTIONS
 */


ry_sts_t alert_sitting_too_long_disp_update(void)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
    // LOG_DEBUG("[alert_sitting_too_long_disp_update] begin.\r\n");
    clear_buffer_black();
 
    if (alert_ctx_v.animate_img_idx >= sizeof(long_sit_img) / sizeof(const char*)){
        alert_ctx_v.animate_img_idx = 0;
    }
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)long_sit_img[alert_ctx_v.animate_img_idx],\
                            0, 100 - height_long_sit_img[alert_ctx_v.animate_img_idx], &w, &h, d_justify_center);

    alert_ctx_v.animate_img_idx ++;
    
    gdispFillStringBox( 0, 
                        140, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)text_too_long_sitting[0], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  

    gdispFillStringBox( 0, 
                        170, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_too_long_sitting[1], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  

    gdispFillStringBox( 0, 
                        200, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_too_long_sitting[2], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  
    
		ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    // LOG_DEBUG("[alert_sitting_too_long_disp_update] end.\r\n");
    
    return ret;
}



ry_sts_t alert_goal_reached_disp_update(void)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t w, h;

    // LOG_DEBUG("[alert_goal_reached_disp_update] begin.\r\n");
    clear_buffer_black();
 
    if (alert_ctx_v.animate_img_idx >= sizeof(target_dong_img) / sizeof(const char*)){
        alert_ctx_v.animate_img_idx = 0;
    }

    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)target_dong_img[alert_ctx_v.animate_img_idx],\
                            0, 20, &w, &h, d_justify_center);

    alert_ctx_v.animate_img_idx ++;
    
    gdispFillStringBox( 0, 
                        140, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_target_done[0], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );
                        
    gdispFillStringBox( 0, 
                        170, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_target_done[1], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  

    gdispFillStringBox( 0, 
                        200, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_target_done[2], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );
                        
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    // LOG_DEBUG("[alert_goal_reached_disp_update] end.\r\n");

    return ret;
}

ry_sts_t alert_batt_low_disp_update(void)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
    const uint8_t icon_start_x = 0;
    const uint8_t icon_start_y = 40;
    char str[20] = {0};

    // LOG_DEBUG("[alert_batt_low_disp_update] begin.\r\n");
    clear_buffer_black();
 
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)alert_array[0].img,\
                            icon_start_x, icon_start_y, &w, &h, d_justify_center);

    uint32_t percent = sys_get_battery();
    uint8_t color_start_x = ((120 - w) >> 1) + 3;
    uint8_t color_len_x = percent * 45 / 100;
    
    int x, y;
    for(y = icon_start_y + 4; y < icon_start_y + 24; y++){
        for(x = color_start_x; x < color_start_x + color_len_x; x++){
            set_dip_at_framebuff(x, y, HTML2COLOR(0x0000FF));
        }
    }

    sprintf(str, "%d%%", percent);
    gdispFillStringBox(0, 80, font_roboto_20.width, font_roboto_20.height,
                        str, (void*)font_roboto_20.font, White, Black|(1<<30), justifyCenter);

    gdispFillStringBox( 0, 
                        140, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_low_battery[0], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  

    gdispFillStringBox( 0, 
                        170, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_low_battery[1], 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    // LOG_DEBUG("[alert_batt_low_disp_update] end.\r\n");
    return ret;
}

int alert_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;

    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[alert_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if (p->y >= 8){
                ;
            }
            ryos_mutex_lock(alert_mutex);
            ry_timer_stop(app_alert_timer_id);
            alert_ctx_v.animate_play = 0;
            ryos_mutex_unlock(alert_mutex);
            
            wms_activity_back(NULL);
            
            return consumed;
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG	("[alert_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            ryos_mutex_lock(alert_mutex);
            ry_timer_stop(app_alert_timer_id);
            alert_ctx_v.animate_play = 0;
            ryos_mutex_unlock(alert_mutex);
            
            wms_activity_back(NULL);
            
            return consumed;
            break;

        default:
            break;
    }
    LOG_DEBUG("[alert_onProcessedTouchEvent], p->action:%d, evt: %d\r\n", p->action, alert_ctx_v.evt);
    
    return consumed; 
}

void alert_timeout_handler(void* arg)
{
    // LOG_DEBUG("[alert_timeout_handler]: transfer timeout.\n");
    ryos_mutex_lock(alert_mutex);    
    alert_ctx_v.animate_play = 0;
    if (alert_ctx_v.screen_onOff == GUI_STATE_OFF){
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
    }
    ryos_mutex_unlock(alert_mutex);    
    wms_activity_back(NULL);
}

int animate_display_task(void* arg)
{
    ry_sts_t status = RY_SUCC;
    static u8_t cnt;
    
    if(alert_mutex == NULL){
        ry_sts_t ret = ryos_mutex_create(&alert_mutex);
        if (RY_SUCC != ret) {
            LOG_ERROR("[ryos_mutex_create]Error status: %d\r\n", ret);
        } 
    }

    while (1) {
        ryos_mutex_lock(alert_mutex);
        if (alert_ctx_v.animate_play){
            alert_ctx_v.animate_finished = 0;
            if (alert_ctx_v.evt == EVT_ALERT_LONG_SIT){
                alert_sitting_too_long_disp_update();
            }
            else if (alert_ctx_v.evt ==  EVT_ALERT_10000_STEP){
                alert_goal_reached_disp_update();
            }
        }
        ryos_mutex_unlock(alert_mutex);
        alert_ctx_v.animate_finished = 1;
        // LOG_DEBUG("[animate_display_task]: cnt:%d\n", cnt ++);
        ryos_delay_ms(20);
    }
}

static inline void animate_thread_setup(void)
{
    ryos_threadPara_t para;
    strcpy((char*)para.name, "animate_task");
    para.stackDepth = 400;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 3;
    if (animate_thread_handle == NULL){
        ryos_thread_create(&animate_thread_handle, animate_display_task, &para);
    }
}

/**
 * @brief   Entry of the alert activity
 *
 * @param   None
 *
 * @return  activity_alert_onCreate result
 */
ry_sts_t activity_alert_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;
    
    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        alert_ctx_v.evt = *(uint32_t*)usrdata;        
        ry_free(usrdata);
        if (alert_ctx_v.evt > EVT_ALERT_10000_STEP){
            return (RY_CID_WMS | RY_ERR_INVALIC_PARA);
        }
    }
    else{
        wms_activity_back(NULL);
    }

    if(alert_mutex == NULL){
        ret = ryos_mutex_create(&alert_mutex);
        if (RY_SUCC != ret) {
            LOG_ERROR("[ryos_mutex_create]Error status: %d\r\n", ret);
        } 
    }

    LOG_DEBUG("[activity_alert_onCreate]: cnt:%d\n", alert_ctx_v.evt);
    
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_alert, alert_onProcessedTouchEvent);

    if (app_alert_timer_id == 0){
        ry_timer_parm timer_para;
        /* Create the timer */
        timer_para.timeout_CB = alert_timeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "alert_timer";
        app_alert_timer_id = ry_timer_create(&timer_para);
    }
    ry_timer_start(app_alert_timer_id, \
                   alert_array[alert_ctx_v.evt].exit_time, alert_timeout_handler, NULL);
    
    /* Add Layout init here */
    alert_ctx_v.animate_finished = 1;
    alert_ctx_v.animate_play = 1;
    alert_ctx_v.animate_img_idx = 0;

    if (alert_ctx_v.evt == EVT_ALERT_BATTERY_LOW){
        alert_array[alert_ctx_v.evt].display();
    }
    else if (alert_ctx_v.evt == EVT_ALERT_LONG_SIT){
        animate_thread_setup();
    }
    else if (alert_ctx_v.evt == EVT_ALERT_10000_STEP){
        animate_thread_setup();
    }

    if (get_gui_state() != GUI_STATE_ON) {
        alert_ctx_v.screen_onOff = GUI_STATE_OFF;
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    }
    else{
        alert_ctx_v.screen_onOff = GUI_STATE_ON;
    }

    return ret;
}

/**
 * @brief   API to exit alert activity
 *
 * @param   None
 *
 * @return  alert activity Destrory result
 */
ry_sts_t activity_alert_onDestrory(void)
{
    /* Release activity related dynamic resources */
    ryos_mutex_lock(alert_mutex);
    alert_ctx_v.animate_play = 0;
    ryos_mutex_unlock(alert_mutex);

    uint32_t destrory = 0;
    if (animate_thread_handle != NULL){
        while (destrory == 0){
            if (alert_ctx_v.animate_finished){
                ryos_thread_delete(animate_thread_handle);
                animate_thread_handle = NULL;
                destrory = 1;
            }
            ryos_delay_ms(30);
        }
    }

    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_alert);

    if(app_alert_timer_id != 0){
        ry_timer_delete(app_alert_timer_id);
        app_alert_timer_id = 0;
    }
    motar_stop();
    wms_untouch_timer_start();        
    LOG_DEBUG("[activity_alert_onDestrory]: cnt:%d\n", 0);
    
    return RY_SUCC;   
}
