/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __BMA_DRV_H__
#define __BMA_DRV_H__

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
uint16_t bma_accel_init(void);
uint16_t bma_accel_polling(void);
void bma_accel_test_step(void);


#endif  /* __BMA_DRV_H__ */
