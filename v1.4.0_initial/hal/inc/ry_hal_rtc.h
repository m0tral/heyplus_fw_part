/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_RTC_H__
#define __RY_HAL_RTC_H__

/*********************************************************************
 * INCLUDES
 */
#include "ry_type.h"
#include "am_hal_rtc.h"

/*
 * CONSTANTS
 *******************************************************************************
 */


/*
 * TYPES
 *******************************************************************************
 */

/**
 * @brief  Type definition of RTC timer handle
 */
typedef u32_t    rtc_timer_t;

/**
 * @brief  Type definition of RTC callback function.
 */
typedef void (*ry_hal_rtc_cb_t)(void);

extern am_hal_rtc_time_t hal_time;

/*
 * FUNCTIONS
 *******************************************************************************
 */


/**
 * @brief   API to init RTC module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_init(void);

/**
 * @brief   API to start RTC module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_start(void);

/**
 * @brief   API to stop RTC module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_stop(void);

/**
 * @brief   API to get RTC current tick
 *
 * @param   None
 *
 * @return  Current tick value
 */
u32_t ry_hal_rtc_get(void);

u32_t ry_hal_rtc_get_time(am_hal_rtc_time_t* time);

/**
 * @brief   API to set RTC Timer, used for alarm
 *
 * @param   None
 *
 * @return  None
 */
rtc_timer_t ry_hal_rtc_set_timer(u32_t second, ry_hal_rtc_cb_t cb);

/**
 * @brief   API to cancel a existed RTC timer
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_cancel_timer(rtc_timer_t handle);

void ry_hal_rtc_int_disable(void);
void  set_rtc_int_every_second(void);
void ry_hal_update_rtc_time(void);
void ry_hal_rtc_int_enable(void);


#endif  /* __RY_HAL_RTC_H__ */
