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


#if (BSP_MCU_TYPE == BSP_APOLLO2)

/* bsp specified */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_bsp_gpio.h"
#include "am_util.h"
#include "am_hal_interrupt.h"
#include "am_hal_reset.h"
#include "am_hal_stimer.h"


/*********************************************************************
 * CONSTANTS
 */ 



/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */



/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to reset MCU
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_mcu_reset(void)
{
    am_hal_reset_poi();
}

/**
 * @brief   API disable global interrupt
 *
 * @param   None
 *
 * @return  Previous irq status
 */
u32_t ry_hal_irq_disable(void)
{
    return am_hal_interrupt_master_disable();
}

/**
 * @brief   API to enable global interrupt
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_irq_enable(void)
{
    am_hal_interrupt_master_enable();
}

/**
 * @brief   API to restore global interrupt status
 *
 * @param   value - The status to be restored
 *
 * @return  None
 */
void ry_hal_irq_restore(u32_t value)
{
    if (value) {
        am_hal_interrupt_master_disable();
    } else {
        am_hal_interrupt_master_enable();
    }
}

/**
 * @brief   API to get current CPU time
 *
 * @param   None
 *
 * @return  The current cpu time in 1/32768 s
 */
u32_t ry_hal_clock_time(void)
{
    return am_hal_stimer_counter_get();
}

/**
 * @brief   API to transfer the clock time to millsecond.
 *
 * @param   clock - The sepcified clock tick
 *
 * @return  The millsecond
 */
u32_t ry_hal_calc_ms(u32_t clock)
{
    return (clock*1000)/32768;
}


/**
 * @brief   API to transfer the clock time to second.
 *
 * @param   clock - The sepcified clock tick
 *
 * @return  The second
 */
u32_t ry_hal_calc_second(u32_t clock)
{
    return (clock>>15);
}

#endif /* (BSP_MCU_TYPE == BSP_APOLLO2) */


