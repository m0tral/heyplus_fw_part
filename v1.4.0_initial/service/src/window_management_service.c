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
#include "ry_type.h"
#include "ry_utils.h"


#include "window_management_service.h"
#include "timer_management_service.h"
#include "device_management_service.h"
#include "data_management_service.h"
#include "app_management_service.h"

#include "ry_list.h"
#include "scheduler.h"
#include "ryos_timer.h"
#include "touch.h"
#include "string.h"
#include "app_pm.h"
#include "gui.h"
#include "ry_hal_inc.h"
#include "sensor_alg.h"
#include "led.h"
#include "app_interface.h"
#include "dip.h"

#include "activity_surface.h"
#include "activity_hrm.h"
#include "activity_setting.h"
#include "activity_system.h"
#include "activity_charge.h"
#include "activity_qrcode.h"
#include "activity_alarm_ring.h"
#include "activity_alert.h"
#include "activity_menu.h"
#include "activity_loading.h"
#include "ry_statistics.h"
#include "activity_find_phone.h"
#include <task.h>


/*********************************************************************
 * CONSTANTS
 */ 

//#define  WMS_TEST

/*
 * @brief Stack depth of the wms thread
 */
#define WMS_STACK_DEPTH             768
#define WMS_OLED_TO_DARKER_TIME     5000
#define WMS_OLED_DARKER_TO_OFF_TIME 100

/*
 * @brief MAX message queue required by wms thread
 */
#define WMS_MSG_QUEUE_LENTH         10

#define WMS_GET_CUR_ACTIVITY()      (wms_ctx_v.cur_activity)
#define WMS_SET_CUR_ACTIVITY(a)     (wms_ctx_v.cur_activity = a)

#define WMS_LOCK()                  (ryos_mutex_lock(wms_ctx_v.mutex))
#define WMS_UNLOCK()                (ryos_mutex_unlock(wms_ctx_v.mutex))

#ifndef DBG_WIRST_UP_ALWAYS
#define DBG_WIRST_UP_ALWAYS         0
#endif

/*********************************************************************
 * TYPEDEFS
 */






 
/*********************************************************************
 * LOCAL VARIABLES
 */

/*
 * @brief Wms Thread handle
 */
ryos_thread_t *wms_thread_handle;

/*
 * @brief Wms message queue, which is used to receive ry IPC message.
 */
ry_queue_t *wms_msgQ;

/*
 * @brief Context control block.
 */
wms_ctx_t wms_ctx_v;

/*
 * @brief Common touch process session
 */
wms_tp_session_t *wms_tp_sess_v;


activity_t weather_activity = {
    .name = "weather",
    .onCreate = NULL,
    .onDestroy = NULL,
    .priority = WM_PRIORITY_M,
};

activity_t mijia_activity = {
    .name = "mijia",
    .onCreate = NULL,
    .onDestroy = NULL,
    .priority = WM_PRIORITY_M,
};

extern activity_t activity_card_session;

int g_lastTouchTime;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
void wms_start_first_activity(activity_t* activity);
void wms_list_activities(void);
void wms_quick_untouch_timer_stop(void);



bool is_activity_empty(void)
{
    return list_empty(&wms_ctx_v.active_activities->list);
}


void activity_push(activity_t* activity)
{
    struct list_head *pos;
    int flag = 0;
    WMS_LOCK();
    list_for_each(pos,&wms_ctx_v.active_activities->list){
        if(pos == &(activity->list)){
            flag = 1;
            break;
        }else{

        }
    }
    if(flag){
        list_move(&(activity->list), &wms_ctx_v.active_activities->list);
    }else{
        list_add(&activity->list, &wms_ctx_v.active_activities->list);
    }
    WMS_UNLOCK();
}



activity_t* activity_pop(void)
{
    activity_t *ret = NULL;
    int offset = 0;
    
    if (is_activity_empty()) {
        LOG_WARN("[activity_pop] List Empty!!! Help!!! \r\n");
        return NULL;
    }
    
    WMS_LOCK();
    struct list_head* p = wms_ctx_v.active_activities->list.next;
    list_del(p);
    WMS_UNLOCK();
 
    offset = offsetof(activity_t, list);
    ret = (activity_t*)((u32_t)p-offset);
    //LOG_DEBUG("[activity_pop] 0x%x, offset:%d 0x%x\r\n", (u32_t)p, offset, ret);
    return ret;
}



/**
 * @brief   API to add activity to window manager service
 *
 * @param   None
 *
 * @return  None
 */
void wms_activity_add(activity_t* activity)
{
    
}

/**
 * @brief   API to goto specified activity
 *
 * @param   activity The specified activity to display
 *
 * @return  None
 */
void wms_activity_jump(activity_t* activity, void* usrdata)
{
    int count = 0;
    ry_sts_t status = RY_SUCC;

    activity->usrdata = usrdata;
    activity_push(activity);
    activity->usrdata = usrdata;

    // Clear the resource of current one.
    if (WMS_GET_CUR_ACTIVITY()->onDestroy) {
        WMS_GET_CUR_ACTIVITY()->onDestroy();
    }
    //LOG_DEBUG("[WMS jump]-----After ON Destroy. free heap size:%d\r\n", ryos_get_free_heap());

    WMS_SET_CUR_ACTIVITY(activity);

    LOG_DEBUG("jump to activity: %s\r\n", activity->name);

    // Send msg to schedule next activity
    status = wms_msg_send(IPC_MSG_TYPE_WMS_SCHEDULE, 0, NULL);
    if (status != RY_SUCC) {
        // TODO: send error to diagnostic module.
        LOG_ERROR("[wms_activity_jump] Send msg to wms schedule fail. %x\r\n", status);
    }

}

/**
 * @brief   Return to last activity
 *
 * @param   None
 *
 * @return  None
 */
void wms_activity_back(void* usrdata)
{
    ry_sts_t status = RY_SUCC;
    int count = 0;
    
    // Clear the resource of current one.
    if (WMS_GET_CUR_ACTIVITY()->onDestroy) {
        if ((uint32_t)WMS_GET_CUR_ACTIVITY()->onDestroy <= DEVICE_INTERNAL_FLASH_ADDR_MAX){
            WMS_GET_CUR_ACTIVITY()->onDestroy();
        }
        else{
            LOG_ERROR("[wms]: %s destroy overflow.addr:%d\r\n", \
                WMS_GET_CUR_ACTIVITY()->name, (uint32_t)WMS_GET_CUR_ACTIVITY()->onDestroy);
        }
    }
    //LOG_DEBUG("[WMS back]-----After ON Destroy. free heap size:%d\r\n", ryos_get_free_heap());

    
    activity_pop();
    activity_t * first = (activity_t *)container_ptr(wms_ctx_v.active_activities->list.next, activity_t, list);
    //RY_ASSERT(first->onCreate);
    if (first->onCreate == NULL) {
        LOG_ERROR("[wms_activity_back] Empty Activity!!!!!!!!!!!!!!!!!!!!!!");
        first = &activity_surface;
    }
    else if ((uint32_t)first->onCreate > DEVICE_INTERNAL_FLASH_ADDR_MAX){
        LOG_ERROR("[wms]: first %s creat overflow.addr:%d\r\n", \
            first->name, (uint32_t)first->onCreate);
    }

    WMS_SET_CUR_ACTIVITY(first);
    first->usrdata = usrdata;
    
    // Send msg to schedule 
    status = wms_msg_send(IPC_MSG_TYPE_WMS_SCHEDULE, 0, NULL);
    if (status != RY_SUCC) {
        // TODO: send error to diagnostic module.
        LOG_ERROR("[wms_activity_back] Send msg to wms schedule fail. %x\r\n", status);
    }

    wms_untouch_timer_start();

}

uint8_t wms_window_lock_status_get(void)
{
    return wms_ctx_v.window_lock;
}


void wms_window_lock_status_set(uint8_t status)
{
    if (status == 0){
        wms_ctx_v.window_lock = 0;        
    }
    else if (is_lock_enable()){
        wms_ctx_v.window_lock = status;
    }
    LOG_DEBUG("[wms_window_lock_status_set] is_lock_enable:%d, lock_status:%d. heap: %d\r\n\r\n", \
        is_lock_enable(), status, ryos_get_free_heap());
    
}

uint32_t wms_is_in_break_activity(void)
{
    return (0 == strcmp(WMS_GET_CUR_ACTIVITY()->name, "running_break"));
}

/**
 * @brief   Return to the first activity
 *
 * @param   None
 *
 * @return  None
 */
void wms_activity_back_to_root(void* usrdata)
{
#if 1
    ry_sts_t status = RY_SUCC;
    activity_t * first = NULL;

    LOG_DEBUG("[wms_activity_back_to_root] Enter. heap: %d\r\n", ryos_get_free_heap());

    wms_list_activities();

    if (WMS_GET_CUR_ACTIVITY() == &activity_card_session) {
        return;
    }

    /*if (get_device_session() == DEV_SESSION_RUNNING) {
        return;
    }*/
    

    do {
        // Clear the resource of current one.
        if (WMS_GET_CUR_ACTIVITY()->onDestroy) {
            WMS_GET_CUR_ACTIVITY()->onDestroy();
        }
        //LOG_DEBUG("[wms_activity_back_to_root]-----After ON Destroy. free heap size:%d, %s\r\n", 
        //    ryos_get_free_heap(), WMS_GET_CUR_ACTIVITY()->name);

        
        activity_pop();
        
        if (0 == strcmp(WMS_GET_CUR_ACTIVITY()->name, "surface")) {
            //LOG_DEBUG("[wms_activity_back_to_root] Already surface activity\r\n");
            break;
        }
        
        first = (activity_t *)container_ptr(wms_ctx_v.active_activities->list.next, activity_t, list);
        if (first) {
            //LOG_DEBUG("[wms_activity_back_to_root] first: %s\r\n", first->name);
        }
        //RY_ASSERT(first->onCreate);
        if (is_activity_empty()) {
            LOG_ERROR("[wms_activity_back_to_root] Empty Activity!!!!!!!!!!!!!!!!!!!!!!\r\n");
            break;
        }
        else{
            WMS_SET_CUR_ACTIVITY(first);
        }
    } while(!is_activity_empty());

    wms_window_lock_status_set(1);
    
    if (is_activity_empty()){
        wms_start_first_activity(&activity_surface);
    }
    // wms_untouch_timer_start();
    
#endif
}

void wms_activity_replace(activity_t* activity, void* usrdata)
{
    ry_sts_t status = RY_SUCC;

    if (WMS_GET_CUR_ACTIVITY()->onDestroy) {
        WMS_GET_CUR_ACTIVITY()->onDestroy();
    }
    LOG_DEBUG("[WMS replace]-----After ON Destroy. free heap size:%d\r\n", ryos_get_free_heap());

    activity->usrdata = usrdata;

    WMS_SET_CUR_ACTIVITY(activity);
    LOG_DEBUG("replace to activity: %s\r\n", activity->name);

    // Send msg to schedule next activity
    status = wms_msg_send(IPC_MSG_TYPE_WMS_SCHEDULE, 0, NULL);
    if (status != RY_SUCC) {
        // TODO: send error to diagnostic module.
        while(1);
    }
}



char * wms_activity_get_cur_name(void)
{
    return WMS_GET_CUR_ACTIVITY()->name;
}

u8_t wms_activity_get_cur_priority(void)
{
    return WMS_GET_CUR_ACTIVITY()->priority;
}

u8_t wms_activity_get_cur_disableUnTouchTime(void)
{
    return WMS_GET_CUR_ACTIVITY()->disableUntouchTimer;
}

wms_evt_listener_t* wms_event_list_search(struct list_head* head, activity_t* activity)
{
    struct list_head *pos;  
    wms_evt_listener_t *tmp = NULL;  
    int consumed = 0;

    WMS_LOCK();
    
    list_for_each(pos, head)  
    {  
        tmp = list_entry(pos, wms_evt_listener_t, list);  
        if (tmp->activity == activity) {
            WMS_UNLOCK();
            return tmp;
        }
    } 

    WMS_UNLOCK();

    return NULL;
}


ry_sts_t wms_event_listener_doAdd(struct list_head* head, activity_t* activity, event_handler_t handler)
{
    wms_evt_listener_t *tmp;
    
    /* Check if the event is already in the list */
    tmp = wms_event_list_search(head, activity);
    if (tmp) {
        /* Already exist */
        LOG_DEBUG("[wms_event_listener_doAdd]: Already Exist\r\n");
        return RY_SUCC;
    }

    /* Allocate a new node */
    tmp = (wms_evt_listener_t*)ry_malloc(sizeof(wms_evt_listener_t));
    if (tmp == NULL) {
        LOG_ERROR("wms_event_listener_add: No Mem.\r\n");
        return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_NO_MEM);
    }

    tmp->activity = activity;
    tmp->handler  = handler;

    WMS_LOCK();
    list_add_tail(&tmp->list, head);
    WMS_UNLOCK();

    //LOG_DEBUG("[WMS] event listener add succ. head:0x%x\r\n", (u32_t)tmp);

    return RY_SUCC;
}


ry_sts_t wms_event_listener_doDel(struct list_head* head, activity_t* activity)
{
    wms_evt_listener_t *tmp;
    
    /* Check if the event is already in the list */
    tmp = wms_event_list_search(head, activity);
    if (!tmp) {
        /* Not found */
        return RY_SUCC;
    }

    /* Delete from list */
    WMS_LOCK();
    list_del(&tmp->list);
    WMS_UNLOCK();

    /* Release the resource */
    ry_free((void*)tmp);
    return RY_SUCC;
}

/**
 * @brief   API to enable/disable msg_show
 *
 * @param   en, 1 enable msg_show, 0 disable
 *
 * @return  None
 */
void wms_msg_show_enable_set(int en)
{
    wms_ctx_v.msg_enable = en;
}

int wms_msg_show_enable_get(void)
{
    return wms_ctx_v.msg_enable;
}

/**
 * @brief   Subscribe the system event.
 *
 * @param   type Which event type to subscribe
 * @param   activity Specified the activity to watch the event
 * @param   handler Specified the event handler function.
 *
 * @return  Status
 */
ry_sts_t wms_event_listener_add(wms_event_type_t type, activity_t* activity, event_handler_t handler)
{
    ry_sts_t status;

    switch (type) {
        case WM_EVENT_TOUCH_RAW:
            status = wms_event_listener_doAdd(&wms_ctx_v.touch_raw_listener_list.list, activity, handler);
            break;

        case WM_EVENT_TOUCH_PROCESSED:
            status = wms_event_listener_doAdd(&wms_ctx_v.touch_processed_listener_list.list, activity, handler);
            break;

        case WM_EVENT_BLE:
            status = wms_event_listener_doAdd(&wms_ctx_v.ble_listener_list.list, activity, handler);
            break;

        case WM_EVENT_RTC:
            status = wms_event_listener_doAdd(&wms_ctx_v.rtc_listener_list.list, activity, handler);
            break;

        case WM_EVENT_DATA:
            status = wms_event_listener_doAdd(&wms_ctx_v.data_listener_list.list, activity, handler);
            break;

        case WM_EVENT_MESSAGE:
            if (wms_ctx_v.msg_enable){
                status = wms_event_listener_doAdd(&wms_ctx_v.message_listener_list.list, activity, handler);
            }
            break;

        case WM_EVENT_ALG:
            status = wms_event_listener_doAdd(&wms_ctx_v.alg_listener_list.list, activity, handler);
            break;

        case WM_EVENT_SYS:
            status = wms_event_listener_doAdd(&wms_ctx_v.sys_listener_list.list, activity, handler);
            break;

        case WM_EVENT_CARD:
            status = wms_event_listener_doAdd(&wms_ctx_v.card_listener_list.list, activity, handler);
            break;

        case WM_EVENT_SURFACE:
            status = wms_event_listener_doAdd(&wms_ctx_v.surface_listener_list.list, activity, handler);
            break;
        
        case WM_EVENT_SPORTS:
            status = wms_event_listener_doAdd(&wms_ctx_v.sport_listener_list.list, activity, handler);
            break;
		
        case WM_EVENT_ALARM_RING:
            status = wms_event_listener_doAdd(&wms_ctx_v.alarm_listener_list.list, activity, handler);
            break;

        case WM_EVENT_ALIPAY:
            status = wms_event_listener_doAdd(&wms_ctx_v.alipay_listener_list.list, activity, handler);
            break;
        default:
            break;
    }

    return RY_SUCC;
    
}

/**
 * @brief   Remove the subscribed event.
 *
 * @param   type Which event type to subscribe
 * @param   activity Specified the activity to watch the event
 *
 * @return  Status
 */
ry_sts_t wms_event_listener_del(wms_event_type_t type, activity_t* activity)
{
    
    ry_sts_t status;
    
    switch (type) {
        case WM_EVENT_TOUCH_RAW:
            status = wms_event_listener_doDel(&wms_ctx_v.touch_raw_listener_list.list, activity);
            break;

        case WM_EVENT_TOUCH_PROCESSED:
            status = wms_event_listener_doDel(&wms_ctx_v.touch_processed_listener_list.list, activity);
            break;

        case WM_EVENT_BLE:
            status = wms_event_listener_doDel(&wms_ctx_v.ble_listener_list.list, activity);
            break;

        case WM_EVENT_RTC:
            status = wms_event_listener_doDel(&wms_ctx_v.rtc_listener_list.list, activity);
            break;

        case WM_EVENT_DATA:
            status = wms_event_listener_doDel(&wms_ctx_v.data_listener_list.list, activity);
            break;

        case WM_EVENT_MESSAGE:
            status = wms_event_listener_doDel(&wms_ctx_v.message_listener_list.list, activity);
            break;

        case WM_EVENT_ALG:
            status = wms_event_listener_doDel(&wms_ctx_v.alg_listener_list.list, activity);
            break;

        case WM_EVENT_SYS:
            status = wms_event_listener_doDel(&wms_ctx_v.sys_listener_list.list, activity);
            break;

        case WM_EVENT_CARD:
            status = wms_event_listener_doDel(&wms_ctx_v.card_listener_list.list, activity);
            break;

        case WM_EVENT_SURFACE:
            status = wms_event_listener_doDel(&wms_ctx_v.surface_listener_list.list, activity);
            break;
        case WM_EVENT_SPORTS:
            status = wms_event_listener_doDel(&wms_ctx_v.sport_listener_list.list, activity);
            break;
		
	    case WM_EVENT_ALARM_RING:
            status = wms_event_listener_doDel(&wms_ctx_v.alarm_listener_list.list, activity);
            break;

        case WM_EVENT_ALIPAY:
            status = wms_event_listener_doDel(&wms_ctx_v.alipay_listener_list.list, activity);
            break;
        default:
            break;
    }

    return RY_SUCC;
}


void wms_screenOnOff(int onoff)
{
    if (onoff) {
        //LOG_DEBUG("[wms_screenOnOff] ON\r\n");
        wms_ctx_v.screen_darker = 0;
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    } 
    else {
        //LOG_DEBUG("[wms_screenOnOff] OFF\r\n");
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
        //ry_led_onoff_with_effect(0, 0);
		// wms_window_lock_status_set(1);
    }
}

void wms_untouch_back2_surface_timeout_handler(void* arg)
{
    ry_sts_t status = RY_SUCC;
    if (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0) {
        // wms_activity_back_to_root(NULL);
        // Send msg to schedule next activity
        status = wms_msg_send(IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE, 0, NULL);
        if (status != RY_SUCC) {
            // TODO: send error to diagnostic module.
            RY_ASSERT(0);
        }
    }
} 

void wms_untouch_timeout_handler(void* arg)
{
    if (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0) {
        //LOG_DEBUG("[wms_untouch_timeout_handler]: transfer timeout.\n");
        wms_screenOnOff(0);
		//for unlock_alert fresh function
		if (strcmp(wms_activity_get_cur_name(), activity_surface.name) == 0){
			if (surface_lock_status_get() == E_LOCK_READY){
                stop_unlock_alert_animate();
			    ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
            }
		}
        ry_timer_start(wms_ctx_v.untouch_timer_id, 60000, \
            wms_untouch_back2_surface_timeout_handler, NULL);        
    }
}    

void wms_idle_timeout_handler(void* arg)
{
    if (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0) {
        //LOG_DEBUG("[wms_idle_timeout_handler]: transfer timeout.\n");
        wms_ctx_v.screen_darker = 1;
        ry_timer_start(wms_ctx_v.untouch_timer_id, WMS_OLED_DARKER_TO_OFF_TIME, wms_untouch_timeout_handler, NULL);
    }
}

void wms_untouch_timer_stop(void)
{
   if (wms_ctx_v.untouch_timer_id) {
       ry_timer_stop(wms_ctx_v.untouch_timer_id);
   }
   //LOG_DEBUG("[wms_untouch_timer_stop].\n");
}


void wms_untouch_timer_start(void)
{
    if (wms_ctx_v.untouch_timer_id && (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0)) {
        wms_quick_untouch_timer_stop();
        ry_timer_start(wms_ctx_v.untouch_timer_id, WMS_OLED_TO_DARKER_TIME, wms_idle_timeout_handler, NULL);
    }
    //LOG_DEBUG("[wms_untouch_timer_start].\n");
}

void wms_untouch_timer_start_for_next_activty(void)
{
    if (wms_ctx_v.untouch_timer_id) {
        wms_quick_untouch_timer_stop();
        ry_timer_start(wms_ctx_v.untouch_timer_id, WMS_OLED_TO_DARKER_TIME, wms_idle_timeout_handler, NULL);
    }
    //LOG_DEBUG("[wms_untouch_timer_start].\n");
}


void wms_quick_untouch_timeout_handler(void* arg)
{
    if (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0) {
        //LOG_DEBUG("[wms_quick_untouch_timeout_handler]: transfer timeout.\n");
        wms_screenOnOff(0);
        ry_timer_start(wms_ctx_v.menu_untouch_timer_id, 10000, \
            wms_untouch_back2_surface_timeout_handler, NULL);        
    }
}    

void wms_quick_idle_timeout_handler(void* arg)
{
    if (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0) {
        //LOG_DEBUG("[wms_quick_idle_timeout_handler]: transfer timeout.\n");
        wms_ctx_v.screen_darker = 1;
        ry_timer_start(wms_ctx_v.menu_untouch_timer_id, WMS_OLED_DARKER_TO_OFF_TIME, wms_quick_untouch_timeout_handler, NULL);
    }
}

void wms_quick_untouch_timer_start(void)
{
    if (wms_ctx_v.menu_untouch_timer_id && (WMS_GET_CUR_ACTIVITY()->disableUntouchTimer == 0)) {
        wms_untouch_timer_stop();
        ry_timer_start(wms_ctx_v.menu_untouch_timer_id, WMS_OLED_TO_DARKER_TIME, wms_quick_idle_timeout_handler, NULL);
    }
    //LOG_DEBUG("[wms_quick_untouch_timer_start]: transfer timeout.\n");
}

void wms_quick_untouch_timer_stop(void)
{
   if (wms_ctx_v.menu_untouch_timer_id) {
       ry_timer_stop(wms_ctx_v.menu_untouch_timer_id);
   }
}

u8_t wms_get_screen_darker(void)
{
    return wms_ctx_v.screen_darker;
}

void wms_set_screen_darker(u8_t val)
{
    wms_ctx_v.screen_darker = val;
}

int wms_touch_is_busy(void)
{
    return wms_tp_sess_v ? 1: 0;
}

/**
 * @brief   API to get wms tp session
 *
 * @return  None
 */
void wms_tp_sess_obtain(void)
{
    if (wms_tp_sess_v) {
        /* Busy */
        return;
    }
    
    wms_tp_sess_v = (wms_tp_session_t*)ry_malloc(sizeof(wms_tp_session_t));
    if (!wms_tp_sess_v) {
        LOG_WARN("[wms_tp_sess_obtain] No mem.\r\n");
        return;
    }

    ry_memset(wms_tp_sess_v, 0, sizeof(wms_tp_session_t));
}

/**
 * @brief   API to free wms tp session
 *
 * @return  None
 */
void wms_tp_sess_recycle(void)
{
    if (wms_tp_sess_v) {
        ry_free((u8_t*)wms_tp_sess_v);
    }

    wms_tp_sess_v = NULL;
}


int wms_touchCommonProcess(ry_ipc_msg_t* msg, wms_evt_listener_list_t* tpNode)
{
    int consumed = 1;

    //static int   moveCnt = 0;
    //static int   direction = 0;
    //static u16_t lastY;
    //wms_tp_session_t* tp_sess = wms_tp_sess_v;
    u16_t point;
    u32_t deltaT;
    tp_event_t *p = (tp_event_t*)msg->data;
    tp_processed_event_t *p_processed_event;
    ry_ipc_msg_t* newMsg;
    ry_sts_t status;
    int done = 0;
    

    switch (p->action) {
        case TP_ACTION_DOWN:
            //LOG_DEBUG("[WMS] TP Action Down, y:%d, t:%u\r\n", p->y, p->t);
            wms_tp_sess_obtain();
            if (!wms_tp_sess_v) {
                return consumed;
            }
            wms_tp_sess_v->which_tp_node = tpNode;
            wms_tp_sess_v->startT = p->t;
            wms_tp_sess_v->startY = p->y;
            wms_tp_sess_v->moveCnt = 0;
            wms_tp_sess_v->lastY = p->y;

            if (get_oled_state() != PM_ST_POWER_START) {
                if (surface_runtime_enable_get() == 0){
                    surface_onRTC_runtime_enable(1);
                    ry_sched_msg_send(IPC_MSG_TYPE_RTC_1MIN_EVENT, 0, NULL);
                }
                wms_screenOnOff(1);
                wms_untouch_timer_start();
            }
            
            if ((wms_ctx_v.screen_darker == 1) && (get_gui_state() == GUI_STATE_ON)){
                wms_ctx_v.screen_darker = 0;
            }

            if (p->y == 9) {
                if (WMS_GET_CUR_ACTIVITY()->disableTouchMotar == 0) {
                    if(get_home_vibrate_enable())
                    {
                        if((get_dnd_home_vibrate_enbale() == 1)&&(current_time_is_dnd_status() == 1));
                        else{
                            motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
                        }
                    }
                }
            }

            if (strcmp(WMS_GET_CUR_ACTIVITY()->name, "qrcode") == 0 && p->y == 0) {
                wms_tp_sess_v->which_tp_node->handler(msg);
            }
            
            break;

        case TP_ACTION_MOVE:
            if (p->y == 0) {
                return consumed;
            }
            
            //LOG_DEBUG("[WMS] TP Action Move, y:%d, t:%u\r\n", p->y, p->t);
            if (!wms_tp_sess_v) {
                // For some reason, the DOWN event missing.
                return consumed;
            }
            wms_tp_sess_v->moveCnt++;

            if (p->y > wms_tp_sess_v->lastY) {
                wms_tp_sess_v->direction++;
            }

            if (p->y < wms_tp_sess_v->lastY) {
                wms_tp_sess_v->direction--;
            }

            wms_tp_sess_v->lastY = p->y;
            wms_untouch_timer_start();

            if ((wms_ctx_v.screen_darker == 1) && (get_gui_state() == GUI_STATE_ON)){
                wms_ctx_v.screen_darker = 0;
            }
            
            break;

        case TP_ACTION_UP:
            if (!wms_tp_sess_v) {
                // For some reason, the DOWN event missing.
                //LOG_ERROR("\n\nwms_tp_sess_v is NULL\n\n\n");
                return consumed;
            }

            if (wms_tp_sess_v->which_tp_node == NULL){
                //LOG_ERROR("\n\wms_tp_sess_v->which_tp_node is NULL\n\n\n");
            }

            ryos_delay_ms(1);

            if (wms_tp_sess_v->which_tp_node != tpNode) {
                LOG_WARN("\n\ntp node diff. cur activity = %s\n\n\n", tpNode->activity->name);
                wms_tp_sess_v->which_tp_node = tpNode;
            }
            
            deltaT = ry_hal_calc_ms(p->t - wms_tp_sess_v->startT);
            //LOG_DEBUG("[WMS] TP Action Up, y:%d, t:%u, startT:%u, duration:%u ms\r\n", p->y, p->t, wms_tp_sess_v->startT, deltaT);

            p_processed_event = (tp_processed_event_t*)msg->data;
            msg->msgType = IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT;

            if ((p->y == wms_tp_sess_v->startY)) {// && (wms_tp_sess_v->moveCnt <= 3)) {

                #if 0
                if (deltaT < 50) {
                    if (wms_tp_sess_v->startY < 4) {
                        LOG_DEBUG("[WMS] -->SLIDE DOWN 1. d:%d:%d\r\n", wms_tp_sess_v->startY);
                        p_processed_event->action = TP_PROCESSED_ACTION_SLIDE_DOWN;
                        p_processed_event->y = p->y;
                        if(wms_tp_sess_v->which_tp_node != NULL){
                            touch_bypass_set(1);
                            wms_tp_sess_v->which_tp_node->handler(msg);
                            touch_bypass_set(0);
                        }
                    } else if (wms_tp_sess_v->startY > 4) {
                        LOG_DEBUG("[WMS] -->SLIDE UP 1. point:%d\r\n", wms_tp_sess_v->startY);
                        p_processed_event->action = TP_PROCESSED_ACTION_SLIDE_UP;
                        p_processed_event->y = p->y;
                        if(wms_tp_sess_v->which_tp_node != NULL){
                            touch_bypass_set(1);
                            wms_tp_sess_v->which_tp_node->handler(msg);
                            touch_bypass_set(0);
                        }
                    }
                    done = 1;
                }
                #endif

                if(done == 0){
                    if (wms_tp_sess_v->startY == 9) {
                        static uint32_t t_start;
                        //LOG_DEBUG("[WMS] -->Click 1. point:%d, delta_T:%d\r\n", point, ry_hal_calc_ms(ry_hal_clock_time() - t_start));
                        t_start = ry_hal_clock_time();
                        p_processed_event->action = TP_PROCESSED_ACTION_TAP;
                        p_processed_event->y = wms_tp_sess_v->startY;

                        if(WMS_GET_CUR_ACTIVITY()->disableTouchMotar == 1){
                            LOG_DEBUG("Disalbe touch motar. activity:%s\r\n", WMS_GET_CUR_ACTIVITY()->name);
                        }else {
                            //motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
                        }
                        
                        if(wms_tp_sess_v->which_tp_node != NULL){
                            touch_bypass_set(1);
                            wms_tp_sess_v->which_tp_node->handler(msg);
                            touch_bypass_set(0);
                        }
                        
                        DEV_STATISTICS(home_back_count);
                        done = 1;
                    } else if (deltaT > DELAY_PROCESSED_ACTION_EXTRA_LONG_TAP) {
                        //LOG_DEBUG("[WMS] -->Click 2. point:%d\r\n", point);
                        p_processed_event->action = TP_PROCESSED_ACTION_EXTRA_LONG_TAP;
                        p_processed_event->y = wms_tp_sess_v->startY;
                        if(wms_tp_sess_v->which_tp_node != NULL){
                            touch_bypass_set(1);
                            wms_tp_sess_v->which_tp_node->handler(msg);
                            touch_bypass_set(0);
                        }
                        done = 1;                        
                    } else if (deltaT > DELAY_PROCESSED_ACTION_LONG_TAP) {
                        //LOG_DEBUG("[WMS] -->Click 2. point:%d\r\n", point);
                        p_processed_event->action = TP_PROCESSED_ACTION_LONG_TAP;
                        p_processed_event->y = wms_tp_sess_v->startY;
                        if(wms_tp_sess_v->which_tp_node != NULL){
                            touch_bypass_set(1);
                            wms_tp_sess_v->which_tp_node->handler(msg);
                            touch_bypass_set(0);
                        }
                        done = 1;
                    } else if (deltaT > DELAY_PROCESSED_ACTION_TAP) {
                        //LOG_DEBUG("[WMS] -->Click 2. point:%d\r\n", point);
                        p_processed_event->action = TP_PROCESSED_ACTION_TAP;
                        p_processed_event->y = wms_tp_sess_v->startY;
                        if(wms_tp_sess_v->which_tp_node != NULL){
                            touch_bypass_set(1);
                            wms_tp_sess_v->which_tp_node->handler(msg);
                            touch_bypass_set(0);
                        }
                        done = 1;
                    } 
                }
                
                
            } else if ((ABS(p->y - wms_tp_sess_v->startY)<2) && (wms_tp_sess_v->moveCnt<2)) {
                if (wms_tp_sess_v->startY == 4) {
                    point = p->y;
                }

                if (MAX(wms_tp_sess_v->startY,p->y) > 4) {
                    point = MAX(wms_tp_sess_v->startY,p->y);
                } 

                if (MIN(wms_tp_sess_v->startY,p->y) < 4) {
                    point = MIN(wms_tp_sess_v->startY,p->y);
                } 
                //LOG_DEBUG("[WMS] -->Click 3. point:%d\r\n", point);

                if ((wms_tp_sess_v->startY == 9) && (deltaT>80)) {
                    p_processed_event->action = TP_PROCESSED_ACTION_TAP;
                    p_processed_event->y = point;
                    if(wms_tp_sess_v->which_tp_node != NULL){
                        touch_bypass_set(1);
                        wms_tp_sess_v->which_tp_node->handler(msg);
                        touch_bypass_set(0);
                    }
                    //motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
                    done = 1;
                } else if (deltaT > DELAY_PROCESSED_ACTION_LONG_TAP) {
                    p_processed_event->action = TP_PROCESSED_ACTION_LONG_TAP;
                    p_processed_event->y = point;
                    //if (point == 9) {
                    //    motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
                    //}
                    if(wms_tp_sess_v->which_tp_node != NULL){
                        touch_bypass_set(1);
                        wms_tp_sess_v->which_tp_node->handler(msg);
                        touch_bypass_set(0);
                    }
                    done = 1;
                } else if (deltaT > DELAY_PROCESSED_ACTION_TAP) {
                    p_processed_event->action = TP_PROCESSED_ACTION_TAP;
                    p_processed_event->y = point;
                    //if (point == 9) {
                    //    motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
                    //}
                    if(wms_tp_sess_v->which_tp_node != NULL){
                        touch_bypass_set(1);
                        wms_tp_sess_v->which_tp_node->handler(msg);
                        touch_bypass_set(0);
                    }
                    done = 1;
                } 
            }

            if (done == 0) {
                if (wms_tp_sess_v->direction >= 1) {
                    //LOG_DEBUG("[WMS] -->SLIDE DOWN 2. d:%d\r\n", wms_tp_sess_v->direction);
                    p_processed_event->action = TP_PROCESSED_ACTION_SLIDE_DOWN;
                    p_processed_event->y = wms_tp_sess_v->startY;
                    if(wms_tp_sess_v->which_tp_node != NULL){
                        touch_bypass_set(1);
                        wms_tp_sess_v->which_tp_node->handler(msg);
                        touch_bypass_set(0);
                    }
                        
                } else if (wms_tp_sess_v->direction <= -1) {
                    //LOG_DEBUG("[WMS] -->SLIDE UP 2. d:%d\r\n", wms_tp_sess_v->direction);

                    p_processed_event->action = TP_PROCESSED_ACTION_SLIDE_UP;
                    p_processed_event->y = wms_tp_sess_v->startY;
                    if(wms_tp_sess_v->which_tp_node != NULL){
                        touch_bypass_set(1);
                        wms_tp_sess_v->which_tp_node->handler(msg);
                        touch_bypass_set(0);
                    }
                }
            }
exit:
            wms_tp_sess_recycle();
            wms_untouch_timer_start();
            break;

        default:
            break;
    }

    return consumed;
}


int wms_on_event_common(struct list_head* head, ry_ipc_msg_t* msg)
{
    struct list_head *pos;  
    wms_evt_listener_t *tmp;  
    int consumed = 0;

    WMS_LOCK();
    
    list_for_each(pos, head)  
    {  
        if(pos == NULL){
            break;
        }
        tmp = list_entry(pos, wms_evt_listener_t, list);  
        if (tmp->activity == WMS_GET_CUR_ACTIVITY()) {
            if (tmp->handler) {
                WMS_UNLOCK();
                consumed = tmp->handler(msg);
                WMS_LOCK();
                if (consumed) {
                    break;
                }
            }
        }

        
    } 

    WMS_UNLOCK();

    return consumed;
}


void wms_on_event_tp_processed(struct list_head* head, ry_ipc_msg_t* msg)
{
    struct list_head *pos;  
    wms_evt_listener_t *tmp;  
    int consumed = 0;

    WMS_LOCK();
    
    list_for_each(pos, head)  
    {  
        if (pos == NULL) {
            break;
        }
        
        tmp = list_entry(pos, wms_evt_listener_t, list);  
        if (tmp->activity == WMS_GET_CUR_ACTIVITY()) {
            if (tmp->handler) {
                WMS_UNLOCK();
                consumed = wms_touchCommonProcess(msg, tmp);
                WMS_LOCK();
                if (consumed) {
                    break;
                }
            }
        }
    } 

    WMS_UNLOCK();
}


ry_sts_t wms_ipc_msg_transfer(ry_ipc_msg_t* msg)
{
    ry_sts_t status = RY_SUCC;
    if (ryos_thread_get_state(wms_thread_handle) < RY_THREAD_ST_SUSPENDED){
        wms_msg_t *p = (wms_msg_t*)ry_malloc(sizeof(wms_msg_t)+msg->len);
        if (NULL == p) {
            LOG_ERROR("[wms_msg_send]: No mem.\n");

            // For this error, we can't do anything. 
            // Wait for timeout and memory be released.
            return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_NO_MEM); 
        }   

        ry_memcpy(p, msg, sizeof(wms_msg_t)+msg->len);

        //LOG_DEBUG("[wms_ipc_msg_transfer] type: 0x%x\r\n", msg->msgType);

        if (RY_SUCC != (status = ryos_queue_send(wms_msgQ, &p, 4))) {
            LOG_DEBUG("[wms_ipc_msg_transfer]: Add to Queue fail. type = %x, status = 0x%04x, %s\n", msg->msgType, status,
                WMS_GET_CUR_ACTIVITY()->name);
            return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_QUEUE_FULL);
        } 
    }
    return RY_SUCC;
}



int wms_onRTCEvent(ry_ipc_msg_t* msg)
{
    //LOG_DEBUG("[wms_onRTCEvent]\r\n");
    wms_ipc_msg_transfer(msg);
    return 1;
}

int wms_onTouchEvent(ry_ipc_msg_t* msg)
{
    //LOG_DEBUG("[wms_onTouchEvent]\r\n");
    //wms_on_event_tp_processed(&wms_ctx_v.touch_processed_listener_list.list, msg);
    //wms_on_event_common(&wms_ctx_v.touch_raw_listener_list.list, msg);

    wms_ipc_msg_transfer(msg);
    return 1;
}

int wms_onCardEvent(ry_ipc_msg_t* msg)
{
    wms_ipc_msg_transfer(msg);
    return 1;
}

int wms_onBleEvent(ry_ipc_msg_t* msg)
{
    wms_ipc_msg_transfer(msg);
    return 1;
}



int wms_onSurfaceEvent(ry_ipc_msg_t* msg)
{
    wms_ipc_msg_transfer(msg);
    return 1;
}


int wms_onSportEvent(ry_ipc_msg_t* msg)
{
    wms_ipc_msg_transfer(msg);
    return 1;
}

int wms_onAlarmRingEvent(ry_ipc_msg_t* msg)
{
    wms_ipc_msg_transfer(msg);
    return 1;
}


int wms_onFindPhoneEvent(ry_ipc_msg_t* msg)
{
    if(strcmp(wms_activity_get_cur_name(), activity_system.name) == 0 ){
        //skip all when system activity
        return 1;
    }

    uint32_t* p_userdata = ry_malloc(sizeof(uint32_t));
    *p_userdata = msg->msgType;

    if(strcmp(wms_activity_get_cur_name(), activity_find_phone.name) == 0 ){
        if(msg->msgType == IPC_MSG_TYPE_FIND_PHONE_STOP) {
            LOG_DEBUG("[wms] find_phone stop\n");
            wms_activity_back(NULL);
            ry_free(p_userdata);
        } else {
            LOG_DEBUG("[wms] find phone update 0x%X\n", msg->msgType);
            WMS_GET_CUR_ACTIVITY()->onCreate(p_userdata);
        }
    } else {
        if(msg->msgType == IPC_MSG_TYPE_FIND_PHONE_START){
            LOG_DEBUG("[wms] find phone start\n");
            wms_activity_jump(&activity_find_phone, p_userdata);
        }
        else
        {
            ry_free(p_userdata);
        }
    }

    return 1;
}

int wms_onAlipayEvent(ry_ipc_msg_t* msg)
{
    wms_ipc_msg_transfer(msg);
    return 1;
}

void wms_onLoadingEventReal(ry_ipc_msg_t* msg)
{
    if(strcmp(wms_activity_get_cur_name(), activity_system.name) == 0 ){
        //skip all when system activity
        return;
    }

    uint32_t* p_userdata = ry_malloc(sizeof(uint32_t));
    *p_userdata = msg->msgType;

    if(strcmp(wms_activity_get_cur_name(), activity_loading.name) == 0 ){
        if(msg->msgType == IPC_MSG_TYPE_LOADING_STOP) {
            LOG_DEBUG("[wms] loading stop\n");
            wms_activity_back(NULL);
            ry_free(p_userdata);
        } else {
//            LOG_DEBUG("[wms] loading update 0x%X\n", msg->msgType);
            WMS_GET_CUR_ACTIVITY()->onCreate(p_userdata);
        }
    } else {
        if((msg->msgType != IPC_MSG_TYPE_LOADING_STOP)
            &&(msg->msgType != IPC_MSG_TYPE_LOADING_TIMER)
            ){
            LOG_DEBUG("[wms] loading start\n");
            wms_activity_jump(&activity_loading, p_userdata);
        }
        else
        {
            ry_free(p_userdata);
        }
    }
}

int wms_onAlgEvent(ry_ipc_msg_t* msg)
{
    LOG_DEBUG("[wms_onAlgEvent]\r\n");
 
    //wms_on_event_common(&wms_ctx_v.alg_listener_list.list, msg);

    wms_ipc_msg_transfer(msg);
    return 1;
}


int wms_onMsgEvent(ry_ipc_msg_t* msg)
{
    //LOG_DEBUG("[wms_onMsgEvent]\r\n");
    return wms_on_event_common(&wms_ctx_v.message_listener_list.list, msg);
}

int wms_onSysEvent(ry_ipc_msg_t* msg)
{
    //LOG_DEBUG("[wms_onSysEvent]\r\n");

    u32_t * usrdata = (u32_t *)ry_malloc(sizeof(u32_t));

    if (strcmp(wms_activity_get_cur_name(), activity_system.name) ==0 ){
        if(msg->msgType == IPC_MSG_TYPE_BIND_ACK_CANCEL){
            wms_activity_back(NULL);
            ry_free(usrdata);
            return 1;
        }
    }
    

    if(strcmp(wms_activity_get_cur_name(), activity_system.name) !=0 ){
        if(msg->msgType == IPC_MSG_TYPE_UPGRADE_START){
            *usrdata = UI_SYS_UPGRADING;
            wms_activity_jump(&activity_system, usrdata);
            return 1;
        }
        else if(msg->msgType == IPC_MSG_TYPE_UNBIND_START){
            *usrdata = UI_SYS_UNBINDING;
            wms_activity_jump(&activity_system, usrdata);
            return 1;
        }
        else if(msg->msgType == IPC_MSG_TYPE_BIND_WAIT_ACK){
            *usrdata = UI_SYS_BIND_ACK;
            wms_activity_jump(&activity_system, usrdata);
            return 1;
        }
        else if(msg->msgType == IPC_MSG_TYPE_ALARM_ON){
            *usrdata = *(uint32_t*)msg->data;
            if(strcmp(wms_activity_get_cur_name(), activity_alarm_ring.name) != 0){
                wms_activity_jump(&activity_alarm_ring, usrdata);
            }else{
                wms_activity_replace(&activity_alarm_ring, usrdata);
                motar_set_work(MOTAR_TYPE_STRONG_LOOP, 200);
            }
            return 1;
        }
        else if(msg->msgType == IPC_MSG_TYPE_ALERT_BATTERY_LOW){
            *usrdata = EVT_ALERT_BATTERY_LOW;
            if (strcmp(wms_activity_get_cur_name(), activity_alert.name) != 0){
                wms_activity_jump(&activity_alert, usrdata);
            }
            return 1;
        }
        else if(msg->msgType == IPC_MSG_TYPE_ALERT_LONG_SIT){
            *usrdata = EVT_ALERT_LONG_SIT;
            if (strcmp(wms_activity_get_cur_name(), activity_alert.name) != 0){
                wms_activity_jump(&activity_alert, usrdata);
            }
            return 1;
        }
        else if(msg->msgType == IPC_MSG_TYPE_ALERT_10000_STEP){
            *usrdata = EVT_ALERT_10000_STEP;
            if (strcmp(wms_activity_get_cur_name(), activity_alert.name) != 0){
                wms_activity_jump(&activity_alert, usrdata);
            }
            return 1;
        }

        /*else if(msg->msgType == IPC_MSG_TYPE_UNBIND_SUCC){
            *usrdata = UI_SYS_UNBIND_SUCC;
        }else if(msg->msgType == IPC_MSG_TYPE_UNBIND_FAIL){
            *usrdata = UI_SYS_UNBIND_FAIL;
        }else if(msg->msgType == IPC_MSG_TYPE_UPGRADE_SUCC){
            *usrdata = UI_SYS_UPGRADE_SUCC;
        }else if(msg->msgType == IPC_MSG_TYPE_UPGRADE_FAIL){
            *usrdata = UI_SYS_UPGRADE_FAIL;
        }else if(msg->msgType == IPC_MSG_TYPE_BIND_SUCC){
            *usrdata = UI_SYS_BIND_SUCC;
        }else if(msg->msgType == IPC_MSG_TYPE_BIND_FAIL){
            *usrdata = UI_SYS_BIND_FAIL;
        }*/
    }

    if(msg->msgType == IPC_MSG_TYPE_CHARGING_START){
        ry_free(usrdata);
        usrdata = NULL;
        char * cur_name = wms_activity_get_cur_name();
        if(strcmp(cur_name, activity_charge.name) != 0){
            if(strcmp(cur_name, activity_menu.name) != 0){
                charging_swithing_timer_set(IDX_UNTOUCH_TIMER);
            }
            else{
                charging_swithing_timer_set(IDX_MENU_UNTOUCH_TIMER);
            } 
            if(strcmp(cur_name, activity_alarm_ring.name) != 0){
                wms_activity_jump(&activity_charge, usrdata);
            }
        }else{
            WMS_GET_CUR_ACTIVITY()->onCreate(NULL);
        }
    }else if(msg->msgType == IPC_MSG_TYPE_CHARGING_END){
        
        clear_time_since_last_charge();
        ry_free(usrdata);
        usrdata = NULL;
        char * cur_name = wms_activity_get_cur_name();
        if(strcmp(cur_name, activity_charge.name) != 0){
    
        }else{
            charging_activity_back_processing();
        }
    }

    wms_on_event_common(&wms_ctx_v.sys_listener_list.list, msg);
    
    return 1;
}



void wms_printTouchActivityName(void)
{
    struct list_head *pos;  
    wms_evt_listener_t *tmp;  
    int consumed = 0;

    WMS_LOCK();
    list_for_each(pos, &wms_ctx_v.touch_raw_listener_list.list)  
    {  
        tmp = list_entry(pos, wms_evt_listener_t, list);  
        LOG_DEBUG("activity name: %s\r\n", tmp->activity->name);
    } 
    WMS_UNLOCK();
}

void wms_list_activities(void)
{
    struct list_head *pos;  
    activity_t *tmp;  
    int consumed = 0;

    WMS_LOCK();
    list_for_each(pos, &wms_ctx_v.active_activities->list)  
    {  
        tmp = list_entry(pos, activity_t, list);  
        LOG_DEBUG("activity name: %s\r\n", tmp->name);
    } 
    WMS_UNLOCK();
}


void wms_start_first_activity(activity_t* activity)
{
    activity_push(activity);
    WMS_SET_CUR_ACTIVITY(activity);
    if (activity->onCreate) {
        activity->onCreate(activity->usrdata);
    }
}



/**
 * @brief   API to send IPC message to Window management module
 *
 * @param   type Which event type to subscribe
 * @param   len  Length of the payload
 * @param   data Payload of the message
 *
 * @return  Status
 */
ry_sts_t wms_msg_send(u32_t type, u32_t len, u8_t *data)
{
    ry_sts_t status = RY_SUCC;
    wms_msg_t *p = (wms_msg_t*)ry_malloc(sizeof(wms_msg_t)+len);
    if (NULL == p) {

        LOG_ERROR("[wms_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_NO_MEM); 
    }   

    p->msgType = type;
    p->len     = len;
    if (data) {
        ry_memcpy(p->data, data, len);
    }

    //LOG_DEBUG("[wms_msg_send] type: %x\r\n", type);

    if (RY_SUCC != (status = ryos_queue_send(wms_msgQ, &p, 4))) {
        TaskHandle_t taskHandle = xTaskGetCurrentTaskHandle();
        LOG_ERROR("[wms_msg_send]: Add msg 0x%X to Queue fail. status = 0x%04x, %s, %s\n",\
            p->msgType,
            status,
            pcTaskGetName(taskHandle),
            WMS_GET_CUR_ACTIVITY()->name);
        ry_free((void*)p);        
        return RY_SET_STS_VAL(RY_CID_WMS, RY_ERR_QUEUE_FULL);
    } 

    return RY_SUCC;
}


int wms_last_touch_time;
int wms_last_wrist_time;
int wms_last_wrist_type;

void wms_wrist_filt_handler(u8_t wristType)
{
    if (ALG_WRIST_FILT_UP) {
        
    } else {

    }
}



/**
 * @brief   Window managerment thread
 *
 * @param   None
 *
 * @return  None
 */
int wms_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    wms_msg_t *msg = NULL;
    ry_timer_parm timer_para;
    int consumed;
    int screen_flag = 0;
    monitor_msg_t* mnt_msg;

    LOG_DEBUG("[wms_thread] Enter.\n");

    /* Create the message queue */
    status = ryos_queue_create(&wms_msgQ, WMS_MSG_QUEUE_LENTH, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[wms_thread]: RX Queue Create Error.\r\n");
        while(1);
    }

    status = ryos_mutex_create(&wms_ctx_v.mutex);
    if (RY_SUCC != status) {
        LOG_ERROR("[wms_thread]: Mutex Init Fail.\n");
        while(1);
    }

    /* Create the timer */
    timer_para.timeout_CB = wms_untouch_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "wms_untouch_timer";
    wms_ctx_v.untouch_timer_id = ry_timer_create(&timer_para);

    timer_para.name = "wms_menuUntou_timer";
    wms_ctx_v.menu_untouch_timer_id = ry_timer_create(&timer_para);

    wms_ctx_v.msg_enable = 1;           //power on default enable wms_ctx_v.msg_enable

    /* Subscribe message type from scheduler */
    wm_addTouchListener(wms_onTouchEvent);
    ry_sched_addRTCEvtListener(wms_onRTCEvent);
    ry_sched_addAlgEvtListener(wms_onAlgEvent);
    ry_sched_addMSGEvtListener(wms_onMsgEvent);
    ry_sched_addSysEvtListener(wms_onSysEvent);
    ry_sched_addCardEvtListener(wms_onCardEvent);
    ry_sched_addSurfaceEvtListener(wms_onSurfaceEvent);
    ry_sched_addBLEEvtListener(wms_onBleEvent);
    ry_sched_addSportEvtListener(wms_onSportEvent);
    ry_sched_addFindPhoneEvtListener(wms_onFindPhoneEvent);
    ry_sched_addAlipayEvtListener(wms_onAlipayEvent);
    ry_sched_addAlarmEvtListener(wms_onAlarmRingEvent);

    screen_flag = clear_update_app_flag();
    if(screen_flag){
        ryos_delay_ms(10);
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
        motar_light_linear_working(200);
    }
    else{
        oled_hw_off();
        ry_led_onoff_with_effect(0, 0);      
        ry_led_onoff(0);
        device_set_powerOn_mode(0);
        LOG_DEBUG("[gui_display_startup_logo] end\r\n");
    }
    
    app_service_init();

    /* Start first activity */
    //extern activity_t app_ui_activity;
		extern activity_t activity_sport;
    //wms_start_first_activity(&app_ui_activity);
		
#warning	here is a place -> qrcode or surface activity to choose on start
    if (QR_CODE_DEBUG || is_bond()) {
        wms_start_first_activity(&activity_surface);
    } else {
        wms_start_first_activity(&activity_qrcode);
    }
    //wms_start_first_activity(&activity_hrm);

    /* Enable touch */
    //touch_enable();
    //touch_mode_set(TP_MODE_BTN_ONLY);

    if (screen_flag) {
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
        wms_untouch_timer_start();
    }
    uint32_t msg_handle_recived_stamp = 0;
    uint32_t msg_handle_time_ms_used = 0;
    char* p_last_activity_name = " ";
    const uint32_t msg_handle_too_long_ms = 1000;

    while (1) {
        status = ryos_queue_recv(wms_msgQ, &msg, RY_OS_WAIT_FOREVER);
        msg_handle_recived_stamp = ry_hal_clock_time();
        if (RY_SUCC != status) {
            LOG_ERROR("[wms_thread]: Queue receive error. Status = 0x%x\r\n", status);
            continue;
        } 
        //LOG_DEBUG("[wms_thread] msg: %x, activity: %s\r\n", msg->msgType, WMS_GET_CUR_ACTIVITY()->name);

        switch(msg->msgType) {
            case IPC_MSG_TYPE_WMS_SCHEDULE:
                //LOG_DEBUG("[wms_thread] Schedule activity, on Create.\r\n");
                if (WMS_GET_CUR_ACTIVITY()->onCreate) {
                    //LOG_DEBUG("[WMS]-----Before ON Create. free heap size:%d\r\n", ryos_get_free_heap());
                    if ((uint32_t)WMS_GET_CUR_ACTIVITY()->onCreate <= DEVICE_INTERNAL_FLASH_ADDR_MAX){
                        WMS_GET_CUR_ACTIVITY()->onCreate(WMS_GET_CUR_ACTIVITY()->usrdata);
                    }
                    else{
                        LOG_ERROR("[wms]: %s creat overflow.addr:%d\r\n", \
                            WMS_GET_CUR_ACTIVITY()->name, (uint32_t)WMS_GET_CUR_ACTIVITY()->onCreate);
                        system_app_data_save();
                        add_reset_count(WMS_CREAT_START);
                        ry_system_reset();
                    }
                } else {
                    LOG_WARN("[wms_thread] Current Activity Empty!!!!\r\n");
                }
                break;

            case IPC_MSG_TYPE_TOUCH_RAW_EVENT:
            case IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT:
                wms_on_event_tp_processed(&wms_ctx_v.touch_processed_listener_list.list, (ry_ipc_msg_t*)msg);
                wms_on_event_common(&wms_ctx_v.touch_raw_listener_list.list, (ry_ipc_msg_t*)msg);
                break;

            case IPC_MSG_TYPE_RTC_1S_EVENT:
            case IPC_MSG_TYPE_RTC_1MIN_EVENT:
            case IPC_MSG_TYPE_RTC_1HOUR_EVENT:
            case IPC_MSG_TYPE_RTC_1DAY_EVENT:
                wms_on_event_common(&wms_ctx_v.rtc_listener_list.list, (ry_ipc_msg_t*)msg);
                break;

            case IPC_MSG_TYPE_CMS_CARD_CREATE_EVT:
                wms_on_event_common(&wms_ctx_v.card_listener_list.list, (ry_ipc_msg_t*)msg);
                break;
			
            case IPC_MSG_TYPE_ALARM_RING:
                wms_on_event_common(&wms_ctx_v.alarm_listener_list.list, (ry_ipc_msg_t*)msg);
                break;

            case IPC_MSG_TYPE_SPORTS_READY:
                wms_on_event_common(&wms_ctx_v.sport_listener_list.list, (ry_ipc_msg_t*)msg);
                break;
            case IPC_MSG_TYPE_SURFACE_UPDATE:
                wms_on_event_common(&wms_ctx_v.surface_listener_list.list, (ry_ipc_msg_t*)msg);
                break;

            case IPC_MSG_TYPE_BLE_CONNECTED:
            case IPC_MSG_TYPE_BLE_DISCONNECTED:
                wms_on_event_common(&wms_ctx_v.ble_listener_list.list, (ry_ipc_msg_t*)msg);
                break;

            case IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE:
                wms_activity_back_to_root(NULL);
                break;

            case IPC_MSG_TYPE_ALGORITHM_WRIST_FILT:
                consumed = wms_on_event_common(&wms_ctx_v.alg_listener_list.list, (ry_ipc_msg_t*)msg);
                if (consumed) {
                    break;
                }
                if (dev_mfg_state_get() >= DEV_MFG_STATE_SEMI) 
								{
                if (tms_alg_enable_get() && is_raise_to_wake_open() && (WMS_GET_CUR_ACTIVITY()->disableWristAlg == 0)) 
                {
                    if((get_dnd_wrist_enable() == 1)&&(current_time_is_dnd_status() == 1))
                    {
                        break;
                    } else {
                        /* Common operation for all UI activity */
                        wrist_filt_t* p = (wrist_filt_t*)msg->data;
                        if (p->type == ALG_WRIST_FILT_UP) {
                            DEV_STATISTICS(raise_wake_count);
                            //LOG_DEBUG("[wms] wrist on.\r\n.");

                            if((get_device_session() == DEV_SESSION_RUNNING)){
                                DEV_STATISTICS(sport_rasie_count);
                            }
                            wms_screenOnOff(1);
                            if (WMS_GET_CUR_ACTIVITY() == &activity_menu) {
                                wms_quick_untouch_timer_start();
                            } else {
                                wms_untouch_timer_start();
                            }
                        } 
                        else if (p->type == ALG_WRIST_FILT_DOWN) {
                            //LOG_DEBUG("[wms] wrist down.\r\n.");

                            if (ry_hal_calc_ms(ry_hal_clock_time() - g_lastTouchTime) < 3000) {
                                //LOG_DEBUG("ignore wrist down\r\n");
                                break;
                            }
                            
                            wms_screenOnOff(0);
                            if (WMS_GET_CUR_ACTIVITY() == &activity_menu) {
                                wms_quick_untouch_timer_stop();
                            } else {
                                wms_untouch_timer_stop();
                            }
                            if (strcmp(wms_activity_get_cur_name(), activity_surface.name) == 0){
                                if (surface_lock_status_get() == E_LOCK_READY){
                                    stop_unlock_alert_animate();
                                    ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
                                }
                            }
                            ry_timer_start(wms_ctx_v.untouch_timer_id, 60000, \
                                wms_untouch_back2_surface_timeout_handler, NULL);  
                            }
                        }
                    }
                }
 
                break;

            case IPC_MSG_TYPE_ACTIVITY_HRM_REFRESH:
                // hrm_display_update();
                break;

            case IPC_MSG_TYPE_SYSTEM_MONITOR_WMS:
                mnt_msg = (monitor_msg_t*)msg->data;
                if (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_WMS){
                    *(uint32_t *)mnt_msg->dataAddr = 0;
                }
                break;       

            case IPC_MSG_TYPE_ALIPAY_EVENT:
                wms_on_event_common(&wms_ctx_v.alipay_listener_list.list, (ry_ipc_msg_t*)msg);
                break;

            case IPC_MSG_TYPE_LOADING_START:
            case IPC_MSG_TYPE_LOADING_CONTINUE:
            case IPC_MSG_TYPE_LOADING_TIMER:
            case IPC_MSG_TYPE_LOADING_STOP:
                wms_onLoadingEventReal((ry_ipc_msg_t*)msg);
                break;

            default:
                break;
        }

        msg_handle_time_ms_used = ry_hal_calc_ms(ry_hal_clock_time() - msg_handle_recived_stamp);
        if(msg == NULL) {
//                LOG_ERROR("[wms_thread] time used %d ms msg:NULL cur:%s\n",
//                    msg_handle_time_ms_used,
//                    WMS_GET_CUR_ACTIVITY()->name);
        } else {
            if(msg_handle_time_ms_used > msg_handle_too_long_ms) {
                LOG_INFO("[wms_thread] time used %d ms msg:0x%X cur:%s, last:%s\n",
                    msg_handle_time_ms_used,
                    msg->msgType,
                    WMS_GET_CUR_ACTIVITY()->name,
                    p_last_activity_name);
            } else {
                LOG_DEBUG("[wms_thread] time used %d ms msg:0x%X cur:%s, last:%s\n",
                    msg_handle_time_ms_used,
                    msg->msgType,
                    WMS_GET_CUR_ACTIVITY()->name,
                    p_last_activity_name);
            }
        }
        p_last_activity_name = WMS_GET_CUR_ACTIVITY()->name;

        ry_free((void*)msg);
        msg = NULL;
    }
}



/**
 * @brief   API to int window manager service
 *
 * @param   None
 *
 * @return  None
 */
void wms_init(void)
{
    ryos_threadPara_t para;

    /* Initialize context block */
    wms_ctx_v.state             = WMS_STATE_IDLE;
    wms_ctx_v.cur_activity      = NULL;
    wms_ctx_v.active_activities = NULL;

    wms_ctx_v.active_activities = (activity_t*)ry_malloc(sizeof(activity_t));
    if (wms_ctx_v.active_activities == NULL) {
        while(1); // FATAL error
    }

    INIT_LIST_HEAD(&wms_ctx_v.active_activities->list);
    INIT_LIST_HEAD(&wms_ctx_v.touch_raw_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.touch_processed_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.rtc_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.alg_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.data_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.message_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.sys_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.card_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.surface_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.ble_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.sport_listener_list.list);
    INIT_LIST_HEAD(&wms_ctx_v.alipay_listener_list.list);
	INIT_LIST_HEAD(&wms_ctx_v.alarm_listener_list.list);

    /* Initialize common touch process session */
    wms_tp_sess_v = NULL;
    
    /* Start APP_UI Thread */
    strcpy((char*)para.name, "wms_thread");
    para.stackDepth = WMS_STACK_DEPTH;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 5;
    ryos_thread_create(&wms_thread_handle, wms_thread, &para);  
}


#ifdef WMS_TEST
/**
 * @brief   Test function
 *
 * @param   None
 *
 * @return  None
 */
void wms_test(void)
{
    /* Add Activitys */
    wms_event_listener_add(WM_EVENT_TOUCH, &menu_activity, NULL);
    wms_event_listener_add(WM_EVENT_TOUCH, &surface_activity, NULL);
    wms_event_listener_add(WM_EVENT_TOUCH, &weather_activity, NULL);
    wms_event_listener_add(WM_EVENT_TOUCH, &mijia_activity, NULL);

    activity_push(&menu_activity);
    activity_push(&surface_activity);
    activity_push(&weather_activity);
    activity_push(&mijia_activity);

    activity_t* p = wms_ctx_v.active_activities;
    LOG_DEBUG("[wms_thread]: head 0x%x %s\r\n", (u32_t)p, p->name);

    wms_list_activities();
    ryos_delay_ms(1000);

    p = activity_pop();
    LOG_DEBUG("[wms_thread]: The pop one: 0x%x  %s\r\n", (u32_t)p, p->name);
    activity_pop();
    activity_pop();
    wms_list_activities();
    ryos_delay_ms(1000);

    activity_pop();
    wms_list_activities();
    ryos_delay_ms(1000);

    activity_pop();
    wms_list_activities();
    ryos_delay_ms(1000);
}

#endif  /* WMS_TEST */




