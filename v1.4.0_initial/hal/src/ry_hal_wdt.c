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
#include <ry_hal_inc.h>


/*
 * CONSTANTS
 *******************************************************************************
 */
#define WDT_IGNOREBITS      (3u)





/*
 * TYPES
 *******************************************************************************
 */
uint8_t g_ui8NumWatchdogInterrupts = 0;
am_hal_wdt_config_t g_sWatchdogConfig =
{
	//
	// Select the Apollo 1 Clock Rate.
	//
	.ui32Config = AM_REG_WDT_CFG_CLKSEL_16HZ | AM_HAL_WDT_ENABLE_RESET | AM_HAL_WDT_ENABLE_INTERRUPT,
	//
	// Set WDT interrupt timeout for 3/4 second.
	//
	.ui16InterruptCount = 128 * 3 / 4,

	//
	// Set WDT reset timeout for 1.5 seconds.
	//
	.ui16ResetCount = 128 * 3 / 2
};


/*
 * FUNCTIONS
 *******************************************************************************
 */
void am_watchdog_isr_c(uint32_t u32IsrSP);

__asm uint32_t
am_watchdog_isr(void)
{
    import  am_watchdog_isr_c
    push    {r7, lr}
	TST LR, #4
	ITE EQ
	MRSEQ R0, MSP
	MRSNE R0, PSP
    bl      am_watchdog_isr_c
    pop     {r0, pc}
}

//*****************************************************************************
//
// Interrupt handler for the watchdog.
//
//*****************************************************************************
void
am_watchdog_isr_c(uint32_t u32IsrSP)
{
	//
	// Clear the watchdog interrupt.
	//
	am_hal_wdt_int_clear();

	//
	// Catch the first four watchdog interrupts, but let the fifth through
	// untouched.
	//
	if ( g_ui8NumWatchdogInterrupts < 4 )
	{
		//
		// Restart the watchdog.
		//
		am_hal_wdt_restart();
	}

	//
	// Enable debug printf messages using ITM on SWO pin
	//
	//am_bsp_debug_printf_enable();

	//
	// Send a status message and give it some time to print.
	//
	//am_util_delay_ms(100);

	//
	// Increment the number of watchdog interrupts.
	//

	if(g_ui8NumWatchdogInterrupts >= 4){
		//ry_hal_wdt_stop();
#if RY_BUILD == RY_DEBUG

		void fault_info_last_words(uint32_t u32IsrSP);
		fault_info_last_words(u32IsrSP);
#endif

		//am_hal_wdt_start();
		g_ui8NumWatchdogInterrupts = 0;
	}
	g_ui8NumWatchdogInterrupts++;


}



/**
 * @brief   API to init WDT module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_wdt_init(void)
{
	am_hal_wdt_int_clear();
	am_hal_wdt_init(&g_sWatchdogConfig);
	am_hal_interrupt_enable(AM_HAL_INTERRUPT_WATCHDOG);
}

/**
 * @brief   API to start WDT module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_wdt_start(void)
{
	am_hal_wdt_start();
}

/**
 * @brief   API to start WDT module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_wdt_stop(void)
{
	am_hal_wdt_halt();
}

/**
 * @brief   API to get WDT value
 *
 * @param   None
 *
 * @return  None
 */
uint32_t ry_hal_wdt_feed(void)
{
	return am_hal_wdt_counter_get();
}


void ry_hal_wdt_reset(void)
{
	am_hal_wdt_restart();
}

void ry_hal_wdt_lock_and_start(void)
{
    am_hal_wdt_lock_and_start();
}


