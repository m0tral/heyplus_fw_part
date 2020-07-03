/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_PM_H__
#define __RY_HAL_PM_H__

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

typedef enum {
    PM_IDX_ACTIVE = 0x00,
    PM_IDX_LP_ACTIVE,
    PM_IDX_SLEEP,
    PM_IDX_DEEP_SLEEP,
    PM_IDX_HIBERNATE,
    PM_IDX_MAX
} ry_pm_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */


/**
 * @brief   API to Enter system deep sleep
 *
 * @param   idx  - The specified i2c instance
 *
 * @return  None
 */
void ry_hal_pm_enter_lowpower(ry_pm_t mode);



#endif  /* __RY_HAL_PM_H__ */
