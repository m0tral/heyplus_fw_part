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
// #include "icon_1km.h"
#include "sensor_alg.h"

/*********************************************************************
 * Local function include
 */ 
ry_sts_t sports_onCreat(void * usrdata);
ry_sts_t sports_onDestrory(void);

ry_sts_t sports_menu_display_update(u8_t top_sport);

void sports_running_on_action(uint32_t y);
void swimming_on_action(uint32_t y);
void jumping_on_action(uint32_t y);
void bike_on_action(uint32_t y);
void sports_next_time_on_action(uint32_t y);

/*********************************************************************
 * CONSTANTS
 */      

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    int8_t       top_sport;   
    u8_t         sel_item;
} sports_ctx_t;

typedef struct{
    const char* descrp;
    const char* img;
    void        (*on_action)(uint32_t y);        
}sports_item_t;

/*********************************************************************
 * VARIABLES
 */
const sports_item_t sports_array[] = {
    {"Бег",				"m_sport_00_running.bmp",  sports_running_on_action},
    {"Дорожка",			"m_sport_01_running.bmp",  sports_running_on_action},
    {"Велосипед",		"m_sport_01_bike.bmp",     sports_running_on_action},    
    {"Тренировка",		"m_sport_free.bmp",        sports_running_on_action},
#if SPORTS_ITEM_NUM > 4
    {"Плавание",		"m_sport_swimming_03.bmp", sports_next_time_on_action},
    {"Прыжки",			"m_sport_02_jumping.bmp",  sports_next_time_on_action},
#endif    
};

const char* const text_not_available_yet = "Не доступно";

const char* const text_training_break_cuz_charge[] = {
	"Подключена",
	"зарядка.",
	"Тренировка",
	"остановлена."
};

activity_t activity_sports = {
    .name = "sports", 
    .onCreate = sports_onCreat, 
    .onDestroy = sports_onDestrory,
    .priority = WM_PRIORITY_M,
};


u32_t           sports_ready_timer_id;
sports_ctx_t    sports_ctx_v;
scrollbar_t*    sport_scrollbar = NULL;

/*********************************************************************
 * FUNCTIONS
 */
#include "sport_common_disp_ui.c"
#include "outdoor_running_disp_ui.c"
#include "indoor_running_disp_ui.c"
#include "outdoor_bik_disp_ui.c"
#include "free_disp_ui.c"
#include "activity_running.c"

ry_sts_t sports_menu_display_update(u8_t top_sport)
{
    u8_t w, h;
    clear_buffer_black();
    img_exflash_to_framebuffer((uint8_t*)sports_array[top_sport].img, \
                                22, 4, &w, &h, d_justify_center);
    gdispFillStringBox( 0,                    
                        90,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char*)sports_array[top_sport].descrp,        
                        (void *)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);
#if (SPORTS_ITEM_NUM > 1)
    img_exflash_to_framebuffer((uint8_t*)sports_array[(top_sport + 1) % SPORTS_ITEM_NUM].img, \
                                22, 128, &w, &h, d_justify_center);

    gdispFillStringBox( 0,                    
                        214,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char*)sports_array[(top_sport + 1) % SPORTS_ITEM_NUM].descrp,        
                        (void *)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);
#endif                            
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    return RY_SUCC;
}

void sports_next_time_disp(void)
{
    clear_buffer_black();
    
    gdispFillStringBox( 0,                    
                        110,    
                        font_roboto_20.width,             
                        font_roboto_20.height,            
                        (char *)text_not_available_yet,      
                        (void *)font_roboto_20.font,               
                        White, Black,                  
                        justifyCenter);

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}


void sports_running_on_action(uint32_t y)
{
//    DEV_STATISTICS(sport_out_door_run);

    wms_activity_jump(&activity_running_info, (void *)(sports_ctx_v.sel_item + SPORT_ITEM_BASE));
    LOG_DEBUG("[sports_running_on_action] sel_item:%d\r\n", sports_ctx_v.sel_item);
}

void sports_next_time_on_action(uint32_t y)
{
    sports_next_time_disp();
}

int sports_option_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    LOG_DEBUG("[sports_option_onTouchEvent] 0 top_sport: %d\r\n", sports_ctx_v.top_sport);
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            sports_ctx_v.top_sport --;
            if (sports_ctx_v.top_sport < 0){
                sports_ctx_v.top_sport = 0;
            }
            sports_menu_display_update(sports_ctx_v.top_sport);
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            sports_ctx_v.top_sport ++;
            if (sports_ctx_v.top_sport >= SPORTS_ITEM_NUM - 2){
                sports_ctx_v.top_sport = (SPORTS_ITEM_NUM - 2);
                if (sports_ctx_v.top_sport < 0){
                    sports_ctx_v.top_sport = 0;
                }
            }
            sports_menu_display_update(sports_ctx_v.top_sport);
            break;

        case TP_PROCESSED_ACTION_TAP:
            if (p->y < 8){
                if ((p->y <= 3)){
                    sports_ctx_v.sel_item = sports_ctx_v.top_sport;
                }
                else if ((p->y > 3) && (p->y <= 7)){   
                    sports_ctx_v.sel_item = (sports_ctx_v.top_sport + 1) % SPORTS_ITEM_NUM;
                }      

                if (sports_array[sports_ctx_v.sel_item].on_action != NULL){
                    sports_array[sports_ctx_v.sel_item].on_action(p->y);
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

    LOG_DEBUG("[sports_option_onTouchEvent] 1 top_sport: %d\r\n", sports_ctx_v.top_sport);
    return consumed;
}


int sports_ui_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);
        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}

int sports_ui_onMSGEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    extern activity_t msg_activity;
    wms_activity_jump(&msg_activity, (void *)0 );
    return consumed; 
}


void sports_charging_timeout(void* usrdata)
{
    ry_timer_stop(sports_ready_timer_id);
    ry_timer_delete(sports_ready_timer_id);
    sports_ready_timer_id = 0;
    wms_activity_back(NULL);
}

void charging_sports_exit(void)
{
    if (sports_ready_timer_id == 0) {
        ry_timer_parm timer_para;
        timer_para.timeout_CB = NULL;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "sports_ready_timer";
        sports_ready_timer_id = ry_timer_create(&timer_para);
    }
    ry_timer_start(sports_ready_timer_id, 2000, sports_charging_timeout, NULL);
    
    clear_buffer_black();
    
    gdispFillStringBox( 0, 40, font_roboto_20.width, font_roboto_20.height,           
                        (char *)text_training_break_cuz_charge[0],        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
    gdispFillStringBox( 0, 70, font_roboto_20.width, font_roboto_20.height,
                        (char *)text_training_break_cuz_charge[1],        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
    gdispFillStringBox( 0, 110, font_roboto_20.width, font_roboto_20.height,            
                        (char *)text_training_break_cuz_charge[2],        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
    gdispFillStringBox( 0, 140, font_roboto_20.width, font_roboto_20.height,
                        (char *)text_training_break_cuz_charge[3],
                        (void*)font_roboto_20.font,
                        White, Black, justifyCenter);
                        
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);    
}

ry_sts_t sports_onCreat(void * usrdata)
{
    ry_sts_t ret = RY_SUCC;

    sports_ctx_v.top_sport = 0;
    if(charge_cable_is_input()){
        charging_sports_exit();
        return ret;
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);    
    ret = sports_menu_display_update(sports_ctx_v.top_sport);
    
    if(sport_scrollbar == NULL){
        sport_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);
    }

    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_sports, sports_ui_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_sports, sports_option_onTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD,    &activity_sports, sports_ui_onProcessedCardEvent);
    
    return ret;
}


ry_sts_t sports_onDestrory(void)
{
    ry_sts_t ret = RY_SUCC;

    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_sports);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_sports);
    wms_event_listener_del(WM_EVENT_CARD,    &activity_sports);
    wms_event_listener_del(WM_EVENT_SPORTS,  &activity_sports);

    return ret;
}
