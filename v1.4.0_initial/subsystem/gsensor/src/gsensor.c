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
#include "ry_mcu_config.h"
#include "app_config.h"

#if (RY_GSENSOR_ENABLE == TRUE)

#include "gsensor.h"
#include "ryos.h"
#include "ryos_timer.h"
#include "ry_hal_inc.h"
#include "am_bsp_gpio.h"
#include "mcube_sample.h"
#include "mcube_custom_config.h" 

/*********************************************************************
 * CONSTANTS
 */ 

 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
uint8_t gensor_mode = GSENSOR_POLLING;

/**
 * @brief   GSENSOR entry
 *
 * @param   None
 *
 * @return  None
 */
int gsensor_thread(void* arg)
{
    ry_sts_t ret = RY_SUCC;

    //be careful: do not read mutex-spi in interrupt
    gensor_mode = GSENSOR_POLLING;

    am_hal_gpio_pin_config(AM_BSP_GPIO_GSENSOR_INT, AM_HAL_PIN_INPUT);
    //set interrupt pin, and register irq service task    
    if (gensor_mode == GSENSOR_INTERRUPT){
        am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_GSENSOR_INT, AM_HAL_GPIO_FALLING);
        am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_GSENSOR_INT));
        am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_GSENSOR_INT));
        am_hal_gpio_int_register(AM_BSP_GPIO_GSENSOR_INT, mcube_fifo_irq_handler);
        am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);
    }

    /* Init hardware pin config  */
    ret = mcube_accel_init(gensor_mode);
    if (ret){
        LOG_ERROR("[gsensor] initial fail.\r\n");
    }
    else{
        LOG_DEBUG("[gsensor] initial done.\r\n");
    }
   return ret;
}

/**
 * @brief   API to get gsensor data
 *          should polling to get data when not in interrupt mode
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t gsensor_get_data(void)
{
    if (gensor_mode == GSENSOR_POLLING){
        mcube_fifo_timer_handle(0);
    }
    return RY_SUCC;
}

/**
 * @brief   API to init Gsensor module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t gsensor_init(void)
{
    return gsensor_thread(NULL);
}

#endif  /* (RY_GSENSOR_ENABLE == TRUE) */
