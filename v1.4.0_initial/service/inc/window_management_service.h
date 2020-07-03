/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __WMS_H__
#define __WMS_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "scheduler.h"
#include "ry_list.h"
#include "ryos.h"

/*
 * CONSTANTS
 *******************************************************************************
 */

/**
 * @brief Maximum name length of a activity
 */
#define MAX_ACTIVITY_NAME_LEN       16

/**
 * @brief State definition of WMS state
 */
#define WMS_STATE_IDLE              0
#define WMS_STATE_INITIALIZED       1


/**
 * @brief Event type that UI interested in
 */
typedef enum {
    WM_EVENT_TOUCH_RAW,
    WM_EVENT_TOUCH_PROCESSED,
    WM_EVENT_BLE,
    WM_EVENT_RTC,
    WM_EVENT_DATA,
    WM_EVENT_MESSAGE,
    WM_EVENT_ALG,
    WM_EVENT_SYS,
    WM_EVENT_CARD,
    WM_EVENT_SURFACE,
    WM_EVENT_SPORTS,    
    WM_EVENT_ALIPAY,
    WM_EVENT_ALARM_RING,
} wms_event_type_t;

typedef enum{
    WM_PRIORITY_L = 1,//low
    WM_PRIORITY_M = 2,//middle
    WM_PRIORITY_H = 3,//high
    WM_PRIORITY_G = 4,//god
}wms_activity_priority_t;


typedef enum{
    IDX_UNTOUCH_TIMER,
    IDX_MENU_UNTOUCH_TIMER,
}wms_untouch_timer_idx_t;

/**
 * @brief Message type for WMS, which is compatible with IPC message
 */
typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t  data[1];
} wms_msg_t;

/*
 * TYPES
 *******************************************************************************
 */

/**
 * @brief Definitaion for UI Activity Create Funcion Type
 */
typedef ry_sts_t ( *create_fn_t )(void* usrdata); 

/**
 * @brief Definitaion for UI Activity Destroy Funcion Type
 */
typedef ry_sts_t ( *destroy_fn_t )(void); 

/**
 * @brief Definitaion for general event handler function type
 */
typedef int (*event_handler_t)(ry_ipc_msg_t* msg);


/**
 * @brief Definitaion for UI activity describe type
 */
typedef struct {
    char         name[MAX_ACTIVITY_NAME_LEN];
    void*        which_app;
    void*        usrdata;
    create_fn_t  onCreate;
    destroy_fn_t onDestroy;

    u8_t         disableUntouchTimer;
    u8_t         priority;
    u8_t         brightness;
    u8_t         disableWristAlg;
    u8_t         disableTouchMotar;
    struct list_head list;
    
} activity_t;

typedef struct {
    activity_t*      activity;
    event_handler_t  handler;
    struct list_head list;
} wms_evt_listener_list_t;


typedef struct {
    wms_evt_listener_list_t* which_tp_node;
    u32_t startT;
    int startY;
    int moveCnt;
    int direction;
    int lastY;
} wms_tp_session_t;



typedef wms_evt_listener_list_t wms_evt_listener_t;


typedef struct {
    u32_t                   state;
    activity_t*             cur_activity;
    activity_t*             active_activities;
    ryos_mutex_t*           mutex;
    
    wms_evt_listener_list_t touch_raw_listener_list;
    wms_evt_listener_list_t touch_processed_listener_list;
    wms_evt_listener_list_t rtc_listener_list;
    wms_evt_listener_list_t ble_listener_list;
    wms_evt_listener_list_t alg_listener_list;
    wms_evt_listener_list_t data_listener_list;
    wms_evt_listener_list_t message_listener_list;
    wms_evt_listener_list_t sys_listener_list;
    wms_evt_listener_list_t card_listener_list;
    wms_evt_listener_list_t surface_listener_list;
    wms_evt_listener_list_t sport_listener_list;
    wms_evt_listener_list_t alipay_listener_list;
	wms_evt_listener_list_t alarm_listener_list;

    int                     untouch_timer_id;
    int                     menu_untouch_timer_id;
    uint8_t                 screen_darker;
    uint8_t                 msg_enable; 
    uint8_t                 window_lock;   
} wms_ctx_t;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init window manager service
 *
 * @param   None
 *
 * @return  None
 */
void wms_init(void);


/**
 * @brief   API to goto specified activity
 *
 * @param   activity The specified activity to display
 *
 * @return  None
 */
void wms_activity_jump(activity_t* activity, void* usrdata);

/**
 * @brief   Return to last activity
 *
 * @param   None
 *
 * @return  None
 */
void wms_activity_back(void* usrdata);

/**
 * @brief   replace last activity
 *
 * @param   None
 *
 * @return  None
 */
void wms_activity_replace(activity_t* activity, void* usrdata);


/**
 * @brief   API to enable/disable msg_show
 *
 * @param   en, 1 enable msg_show, 0 disable
 *
 * @return  None
 */
void wms_msg_show_enable_set(int en);

/**
 * @brief   Subscribe the system event.
 *
 * @param   type Which event type to subscribe
 * @param   activity Specified the activity to watch the event
 * @param   handler Specified the event handler function.
 *
 * @return  Status
 */
ry_sts_t wms_event_listener_add(wms_event_type_t type, activity_t* activity, event_handler_t handler);

/**
 * @brief   Remove the subscribed event.
 *
 * @param   type Which event type to subscribe
 * @param   activity Specified the activity to watch the event
 *
 * @return  Status
 */
ry_sts_t wms_event_listener_del(wms_event_type_t type, activity_t* activity);


/**
 * @brief   API to send IPC message to Window management module
 *
 * @param   type Which event type to subscribe
 * @param   len  Length of the payload
 * @param   data Payload of the message
 *
 * @return  Status
 */
ry_sts_t wms_msg_send(u32_t type, u32_t len, u8_t *data);

/**
 * @brief   Return to the first activity
 *
 * @param   None
 *
 * @return  None
 */
void wms_activity_back_to_root(void* usrdata);

char * wms_activity_get_cur_name(void);
u8_t wms_activity_get_cur_priority(void);
int wms_touch_is_busy(void);
void wms_untouch_timer_start(void);
void wms_quick_untouch_timer_start(void);
void wms_untouch_timer_start_for_next_activty(void);

u8_t wms_activity_get_cur_disableUnTouchTime(void);

int wms_msg_show_enable_get(void);

void wms_screenOnOff(int onoff);

void wms_set_screen_darker(u8_t val);

void wms_untouch_timer_stop(void);

void wms_quick_untouch_timer_stop(void);

/**
 * @brief   API to get wms_window_lock_status
 *
 * @param   None
 *
 * @return  lock status - 0: un-locked, else: locked
 */
uint8_t wms_window_lock_status_get(void);

/**
 * @brief   API to set wms_window_lock_status
 *
 * @param   status: the status to set
 *
 * @return  None
 */
void wms_window_lock_status_set(uint8_t status);

u8_t wms_get_screen_darker(void);

uint32_t wms_is_in_break_activity(void);

#endif  /* __WMS_H__ */

