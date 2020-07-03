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
#include "ry_type.h"
#include "ry_hal_inc.h"
/*
 * CONSTANTS
 *******************************************************************************
 */
	
am_hal_rtc_time_t hal_time;

/*
 * TYPES
 *******************************************************************************
 */

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
void ry_hal_rtc_init(void)
{
#if 0
	hal_time.ui32Hour = 17;
    hal_time.ui32Minute = 52;
    hal_time.ui32Second = 00;
    hal_time.ui32Hundredths = 00;
    hal_time.ui32Weekday = 4;
    hal_time.ui32DayOfMonth = 1;
    hal_time.ui32Month = 2;
    hal_time.ui32Year = 18;
    hal_time.ui32Century = 0;

    am_hal_rtc_time_set(&hal_time);
#endif
	//
	// Enable the XT for the RTC.
	//
	am_hal_clkgen_osc_start(AM_HAL_CLKGEN_OSC_XT);

	//
	// Select XT for RTC clock source
	//
	am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);

}

void ry_hal_update_rtc_time(void)
{
    u16_t y;
    u8_t mon, d, h, min, s, weekday;
    u32_t ms;

    u32_t utc = ryos_utc_now();
    ry_utc_parse(utc, &y, &mon, &d, &weekday, &h, &min, &s);   

    hal_time.ui32Hour = h;
    hal_time.ui32Minute = min;
    hal_time.ui32Second = s;
    hal_time.ui32Hundredths = 00;
    hal_time.ui32Weekday = weekday;
    hal_time.ui32DayOfMonth = d;
    hal_time.ui32Month = mon;
    hal_time.ui32Year = y-2000;
    hal_time.ui32Century = 0;

    am_hal_rtc_time_set(&hal_time);
    
}

/**
 * @brief   API to start RTC module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_start(void)
{
	//
    // Enable the RTC.
    //
    am_hal_rtc_osc_enable();
}

/**
 * @brief   API to stop RTC module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_stop(void)
{
	am_hal_rtc_osc_disable();
}

/**
 * @brief   API to get RTC current tick
 *
 * @param   None
 *
 * @return  Current tick value
 */
u32_t ry_hal_rtc_get(void)
{
	return am_hal_rtc_time_get(&hal_time);
}

u32_t ry_hal_rtc_get_time(am_hal_rtc_time_t* time)
{
    return am_hal_rtc_time_get(time);
}

void ry_hal_rtc_int_enable(void)
{
    am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_SEC);
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); 
    am_hal_rtc_int_enable(AM_HAL_RTC_INT_ALM);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_CLKGEN);
}


void ry_hal_rtc_int_disable(void)
{
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); 
    am_hal_rtc_int_disable(AM_HAL_RTC_INT_ALM);
    am_hal_interrupt_disable(AM_HAL_INTERRUPT_CLKGEN);
}

/**
 * @brief   API to set RTC Timer, used for alarm
 *
 * @param   None
 *
 * @return  None
 */
rtc_timer_t ry_hal_rtc_set_timer(u32_t second, ry_hal_rtc_cb_t cb)
{
	am_hal_rtc_time_set(&hal_time);
	return 0;
}

/**
 * @brief   API to cancel a existed RTC timer
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_rtc_cancel_timer(rtc_timer_t handle)
{
	
}

void am_clkgen_isr(void)
{
    // Clear the RTC alarm interrupt.
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
	extern void rtc_event_cb(void);
	rtc_event_cb();
}

void  set_rtc_int_every_second(void)
{
    am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_SEC);
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM); 
    am_hal_rtc_int_enable(AM_HAL_RTC_INT_ALM);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_CLKGEN);
}


