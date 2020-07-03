/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_WDT_H__
#define __RY_HAL_WDT_H__

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
 * @brief   API to init WDT module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_wdt_init(void);

/**
 * @brief   API to start WDT module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_wdt_start(void);

/**
 * @brief   API to start WDT module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_wdt_stop(void);

/**
 * @brief   API to get WDT value
 *
 * @param   None
 *
 * @return  None
 */
uint32_t ry_hal_wdt_feed(void);

void ry_hal_wdt_reset(void);
void ry_hal_wdt_lock_and_start(void);


#endif  /* __RY_HAL_WDT_H__ */
