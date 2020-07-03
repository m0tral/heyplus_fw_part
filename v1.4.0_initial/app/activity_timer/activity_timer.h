#ifndef __TIMER_UI_H_
#define __TIMER_UI_H_

#include "ry_type.h"

/*
 * CONSTANTS
 *******************************************************************************
 */
#define TIMER_ITEM_NUM             2


/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    TIMER_ITEM_BASE = 1,
    TIMER_STOPWATCH = 1,
    TIMER_COUNTDOWN,
} timer_id_t;

/*
 * VARIABLES
 *******************************************************************************
 */
extern activity_t activity_timer;


ry_sts_t timer_onCreate(void* usrdata);
ry_sts_t timer_onDestroy(void);

ry_sts_t timer_menu_display_update(u8_t menu_idx);

void stopwatch_ui_update(void);
void countdown_ui_update(void);

ry_sts_t timer_ui_thread_init(void);

#endif  //#ifndef __SPORT_UI_H_
