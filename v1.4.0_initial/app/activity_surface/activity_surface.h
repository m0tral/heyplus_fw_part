/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_SURFACE_H__
#define __ACTIVITY_SURFACE_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */

#define SURFACE_DISPLAY_FROM_CREATE     1
#define SURFACE_DISPLAY_FROM_RTC        2
#define SURFACE_DISPLAY_FROM_SFC_OPTION 3
#define SURFACE_DISPLAY_FROM_UNLOCK     4

#define SURFACE_DISPLAY_SIX             1

#define WEATHER_UPDATE_INTERVAL		2*60*60 // 2 hours

#define FAST_MENU_ACTIVATE_INTERVAL     1500 // msec

/*
 * TYPES
 *******************************************************************************
 */
typedef enum {
 
    SURFACE_RED_ARRON,
    SURFACE_RED_EARTH,
    SURFACE_GALAXY, 
    SURFACE_COLOFUL,
    SURFACE_RED_CURVES,
    SURFACE_GREEN_CURVES,
    SURFACE_OPTION_MAX,    
} surface_list_t;

typedef struct
{
    uint8_t     list_id;
    const char* bg_img;
    ry_sts_t    (*display)(int);
} surface_disp_t;

typedef enum {
    E_LOCK_IN,      // locked state
    E_LOCK_READY,   // pre-unlock
    E_LOCK_OUT,     // unlocked
} lock_status_t;

typedef enum {
    E_MSG_CMD_STOP_NULL = 0,
    E_MSG_CMD_STOP_LOCK = 1,        //停止解锁提醒动画
    E_MSG_CMD_START_LOCK = 4,       //开始解锁提醒动画
    E_MSG_CMD_START_UNLOCK = 239,   //开始解锁动画，自动停止    
} e_unlock_msg_cmd_t;

typedef enum {
    MAIN,
    FAST_MENU
} display_mode_t;


typedef struct {
    u8_t            state;
    u8_t            list;
    u8_t            disp_lock;
    u8_t            gui_initialed;
    u8_t            lock_st;
    u8_t            unlock_type;            // 0 - default, none, 1 - up swipe,  2 - code, default 1324
    u8_t            animate_finished;
    u8_t            animate_lock_finished;
    u8_t            animate_play_lock;
    u8_t            animate_play_unlock;
    u8_t            runtime_update_enable;
    display_mode_t  display_mode;           // 0 - main, 1 - fast_menu
    u32_t           weather_update_time;
    u32_t           press_hold_start;
    u32_t           press_hold_end;
    u32_t           press_hold_timer_id;
} ry_surface_ctx_t;

/*
 * VARIABLES
 *******************************************************************************
 */

extern activity_t activity_surface;
extern ry_surface_ctx_t surface_ctx_v;

ry_sts_t activity_surface_on_event(int evt);

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to set surface display option
 *
 * @param   option: user surface display option
 *
 * @return  set_surface_option result
 */
ry_sts_t set_surface_option(surface_list_t option);


/**
 * @brief   API to get surface display option
 *
 * @param   None
 *
 * @return  get_surface_option result
 */
surface_list_t get_surface_option(void);

/**
 * @brief   Entry of the surface activity
 *
 * @param   None
 *
 * @return  surface_onCreate result
 */
ry_sts_t surface_onCreate(void* usrdata);

/**
 * @brief   API to exit surface activity
 *
 * @param   None
 *
 * @return  surface activity Destroy result
 */
ry_sts_t surface_onDestroy(void);

/**
 * @brief   API to get surface_lock_status
 *
 * @param   None
 *
 * @return  lock status - ref: lock_status_t
 */
uint8_t surface_lock_status_get(void);

/**
 * @brief   API to stop_unlock_alert_animate
 *
 * @param   None
 *
 * @return  None
 */
void stop_unlock_alert_animate(void);

void start_unlock_alert_animate(void);

uint32_t surface_runtime_enable_get(void);

void surface_onRTC_runtime_enable(uint32_t enable);

void unlock_animate_thread_setup(void);

#endif  /* __ACTIVITY_SURFACE_H__ */


