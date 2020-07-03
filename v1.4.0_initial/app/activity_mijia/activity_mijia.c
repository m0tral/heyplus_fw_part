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
#include "activity_mijia.h"
#include "activity_card.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "mibeacon.h"
#include "dip.h"

/* resources */
#include "gui_img.h"
#include "gui_msg.h"
#include "ry_font.h"
#include "mijia_service.h"
#include "ry_statistics.h"
#include "scrollbar.h"


/*********************************************************************
 * CONSTANTS
 */ 
#define MIJIA_DEVICE_STATUS          2
#define MAX_MIJIA_SCENARIO_NUM       2
#define MAX_MIJIA_SCENARIO_NAME      70

#define ROLL_SPEED                  150
#define ROLL_TEXT_LENGTH            10

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    int8_t      dev_status;
    u8_t        index;
    u8_t        roll_offset;
    u8_t        roll_delay;
    char        roll_name[MAX_MIJIA_SCENARIO_NAME];
    mijia_scene_tbl_t *pTable;
} ry_mijia_ctx_t;
 
 typedef struct {
    const char* img;
    const char* str;
} mijia_disp_t;

 /*********************************************************************
 * VARIABLES
 */

ry_mijia_ctx_t activity_mijia_ctx_v;
scrollbar_t * mijia_scrollbar = NULL;
int mijia_roll_timer_id = 0;
int roll_draw_update_lock = 0;

activity_t activity_mijia = {
    .name      = "mijia",
    .onCreate  = activity_mijia_onCreate,
    .onDestroy = activity_mijia_onDestroy,
    .priority = WM_PRIORITY_M,
};

static const char* const mijia_scenario_names[MAX_MIJIA_SCENARIO_NUM] = 
{
    "Сценарий 1",
    "Сценарий 2"
};

static const char* const text_add_at_mijia [] = {
    "Добавьте",
    "в Mijia",
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */

void mijia_display_update_roll(void* args);

void prepare_roll_timer()
{
    if (mijia_roll_timer_id == 0) {
        ry_timer_parm timer_para;
        timer_para.timeout_CB = mijia_display_update_roll;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "mijia_roll";
        mijia_roll_timer_id = ry_timer_create(&timer_para);
    }
}

void execute_roll_timer()
{
    if (mijia_roll_timer_id != 0) {
        ry_timer_stop(mijia_roll_timer_id);
        ry_timer_start(mijia_roll_timer_id,  ROLL_SPEED, mijia_display_update_roll, NULL);
    }
}

void delete_roll_timer()
{
    activity_mijia_ctx_v.roll_offset = 0;
    activity_mijia_ctx_v.roll_delay = 0;
    memset(activity_mijia_ctx_v.roll_name, 0, MAX_MIJIA_SCENARIO_NAME);        
        
    if (mijia_roll_timer_id != 0) {
                
        ry_timer_stop(mijia_roll_timer_id);
        ry_timer_delete(mijia_roll_timer_id);
        
        mijia_roll_timer_id = 0;
    }
}

char ui_roll = '|';

ry_sts_t mijia_display_update(char* name)
{
    ry_sts_t ret = RY_SUCC;
    int startY;
    int delta;
    
    int len = strlen(name);
    if (len == 0)
        len = strlen(activity_mijia_ctx_v.roll_name);
    
    int is_roll_active = len > ROLL_TEXT_LENGTH;

    if(strcmp(wms_activity_get_cur_name(), activity_mijia.name) != 0){
        delete_roll_timer();
        return ret;
    }    
    
    roll_draw_update_lock = 1;
    
    LOG_DEBUG("[mijia_display_update] begin.\r\n");
    clear_buffer_black();
    
    if (is_mijia_bond() == 0 || activity_mijia_ctx_v.pTable->curNum == 0) {
        gdispFillStringBox( 0, 
                            136, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_add_at_mijia[0], 
                            (void*)font_roboto_20.font, 
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        gdispFillStringBox( 0, 
                            164, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_add_at_mijia[1], 
                            (void*)font_roboto_20.font, 
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        uint8_t w, h;
        draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"m_no_alarm.bmp",\
                                0, 68, &w, &h, d_justify_center);
    }
    else {
        
        char tmp[ROLL_TEXT_LENGTH + 1] = {0};        
        
        if (is_roll_active) {
            
            if (mijia_roll_timer_id == 0 && strlen(name) > 0)
                strcpy(activity_mijia_ctx_v.roll_name, name);
            if (strlen(name) == 0)
                strcpy(name, activity_mijia_ctx_v.roll_name);
            
            strncpy(tmp, name + activity_mijia_ctx_v.roll_offset, ROLL_TEXT_LENGTH);
            activity_mijia_ctx_v.roll_offset++;
            
            if (activity_mijia_ctx_v.roll_offset >= (len - ROLL_TEXT_LENGTH)) {
                
                activity_mijia_ctx_v.roll_offset = len - ROLL_TEXT_LENGTH;
                activity_mijia_ctx_v.roll_delay++;
                
                if (activity_mijia_ctx_v.roll_delay >= 4) {                
                    activity_mijia_ctx_v.roll_offset = 0;
                    activity_mijia_ctx_v.roll_delay = 0;
                }
            }
            
            prepare_roll_timer();
        }
        else {        
            snprintf(tmp, ROLL_TEXT_LENGTH, "%s", name);
        }
        
        gdispFillStringBox( 0, 
                            50, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            tmp, 
                            (void*)font_roboto_20.font, 
                            White, Black,
                            justifyCenter
                          ); 
                            
                            
        ry_time_t time;
        tms_get_time(&time);
        
        uint8_t alarm_str[10];
        sprintf((char*)alarm_str, "%02d:%02d", time.hour, time.minute);
        gdispFillString(3, 0, (const char *)alarm_str, font_roboto_20.font, Grey, Black);
        gdispFillString(70, 0, "Mijia", font_roboto_20.font, Grey, Black);
                            
        setHLine(0, RM_OLED_WIDTH - 1, 30, Grey); 

        uint8_t w, h;

        // gdispFillCircle(60, 168, 40, Green);
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_widget_execute.bmp", \
                                0, 156, &w, &h, d_justify_center);
    }

    if (activity_mijia_ctx_v.pTable->curNum > 1 && is_mijia_bond()) {
        delta = 200 / (activity_mijia_ctx_v.pTable->curNum - 1);
        startY = activity_mijia_ctx_v.index * delta;
        scrollbar_show(mijia_scrollbar, frame_buffer, startY);
    }

    if (get_gui_state() == GUI_STATE_ON)
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    
    roll_draw_update_lock = 0;
    
    if (is_roll_active)
        execute_roll_timer();

    LOG_DEBUG("[mijia_display_update] end.\r\n");
    return ret;    
}

void mijia_display_update_roll(void* args) {
    
    if(strcmp(wms_activity_get_cur_name(), activity_mijia.name) != 0){
        delete_roll_timer();
        return;
    }
    
    if (mijia_roll_timer_id == 0)
        return;
    
    mijia_display_update(activity_mijia_ctx_v.roll_name);
}

int mijia_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    char* scenarioName;
    MiSceneInfo* pScene;
    u8_t hex[20];
    int hex_cnt;
    
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
#if 1
            if (p->y>=4 && p->y<8) {

                if (is_mijia_bond() == 0 || activity_mijia_ctx_v.pTable->curNum == 0) {
                    return consumed;
                }

                pScene = &activity_mijia_ctx_v.pTable->scenes[activity_mijia_ctx_v.index];
                for (int i = 0; i < pScene->launchs_count; i++) {
                    
                    /* Check if the lanch condition is for our product. */
                    if (0 != strcmp(pScene->launchs[i].model, MIJIA_MODEL_SAKE)) {
                        continue;
                    }

                    /* Check which BLE event to send */
                    if (strcmp(pScene->launchs[i].event_str, MIJIA_EVT_STR_CLICK) == 0) {
                        hex_cnt = str2hex(pScene->launchs[i].event_value, strlen(pScene->launchs[i].event_value), hex);
                        mibeacon_click(hex, hex_cnt);
                    } else if (strcmp(pScene->launchs[i].event_str, MIJIA_EVT_STR_SLEEP) == 0) {
                        hex_cnt = str2hex(pScene->launchs[i].event_value, strlen(pScene->launchs[i].event_value), hex);
                        mibeacon_sleep(hex[0]);
                    }
                }

                
                DEV_STATISTICS(mijia_use);
                    
                mijia_display_update((char*)pScene->name);

                motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 100);
                
            }
#else   //add debug environment device, which should start by the device.
        extern void surface_manager_debug_port(void);
        surface_manager_debug_port();
#endif
            LOG_DEBUG("[%s] TP_Action Click, y:%d\r\n", __FUNCTION__, p->y);
            if ((p->y >= 8)){
                delete_roll_timer();
                wms_activity_back(NULL);
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            if (is_mijia_bond() == 0 || activity_mijia_ctx_v.pTable->curNum == 0) {
                return consumed;
            }
            
            if (activity_mijia_ctx_v.index < activity_mijia_ctx_v.pTable->curNum-1) {
                activity_mijia_ctx_v.index++;
            }

            delete_roll_timer();
            mijia_display_update((char*)activity_mijia_ctx_v.pTable->scenes[activity_mijia_ctx_v.index].name);
            DEV_STATISTICS(mijia_change);
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            if (is_mijia_bond() == 0 || activity_mijia_ctx_v.pTable->curNum == 0) {
                return consumed;
            }
            
            if (activity_mijia_ctx_v.index > 0) {
                activity_mijia_ctx_v.index--;
            }

            delete_roll_timer();
            mijia_display_update((char*)activity_mijia_ctx_v.pTable->scenes[activity_mijia_ctx_v.index].name);
            DEV_STATISTICS(mijia_change);
            break;

        default:
            break;
    }
    return consumed; 
}

int mijia_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}

int mijia_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}



/**
 * @brief   Entry of the Weather activity
 *
 * @param   None
 *
 * @return  activity_mijia_onCreate result
 */
ry_sts_t activity_mijia_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    LOG_DEBUG("[activity_mijia_onCreate]\r\n");
    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_mijia, mijia_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_mijia, mijia_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_mijia, mijia_onProcessedCardEvent);

    if (mijia_scrollbar == NULL){
        mijia_scrollbar = scrollbar_create(1, 40, 0x4A4A4A, 1000);
    }
    
    /* Add Layout init here */
    activity_mijia_ctx_v.index = 0;
    activity_mijia_ctx_v.roll_offset = 0;
    memset(activity_mijia_ctx_v.roll_name, 0, MAX_MIJIA_SCENARIO_NAME);
    
    activity_mijia_ctx_v.pTable = mijia_scene_get_table();   
    
    //mijia_display_update((char*)mijia_scenario_names[activity_mijia_ctx_v.index]);
    mijia_display_update((char*)activity_mijia_ctx_v.pTable->scenes[activity_mijia_ctx_v.index].name);

    return ret;
}

/**
 * @brief   API to exit mijia activity
 *
 * @param   None
 *
 * @return  Weather activity Destrory result
 */
ry_sts_t activity_mijia_onDestroy(void)
{
    delete_roll_timer();
    
    while (roll_draw_update_lock == 1)
        ryos_delay_ms(30);
    
    if (mijia_scrollbar != NULL){
        scrollbar_destroy(mijia_scrollbar);
        mijia_scrollbar = NULL;
    }
    
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_mijia);
    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_mijia);
    wms_event_listener_del(WM_EVENT_CARD, &activity_mijia);

    return RY_SUCC;    
}
