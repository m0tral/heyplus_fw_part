/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_MCU_H__
#define __RY_HAL_MCU_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */




/*
 * TYPES
 *******************************************************************************
 */




/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to reset MCU
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_mcu_reset(void);

/**
 * @brief   API disable global interrupt
 *
 * @param   None
 *
 * @return  Previous irq status
 */
u32_t ry_hal_irq_disable(void);

/**
 * @brief   API to enable global interrupt
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_irq_enable(void);

/**
 * @brief   API to restore global interrupt status
 *
 * @param   value - The status to be restored
 *
 * @return  None
 */
void ry_hal_irq_restore(u32_t value);

/**
 * @brief   API to get current CPU time
 *
 * @param   None
 *
 * @return  The current cpu time in 1/32768 s
 */
u32_t ry_hal_clock_time(void);

/**
 * @brief   API to transfer the clock time to millsecond.
 *
 * @param   clock - The sepcified clock tick
 *
 * @return  The millsecond
 */
u32_t ry_hal_calc_ms(u32_t clock);

/**
 * @brief   API to transfer the clock time to second.
 *
 * @param   clock - The sepcified clock tick
 *
 * @return  The second
 */
u32_t ry_hal_calc_second(u32_t clock);

#endif  /* __RY_HAL_MCU_H__ */

