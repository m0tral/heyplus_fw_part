/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_GPIO_H__
#define __RY_HAL_GPIO_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "app_config.h"


/*
 * CONSTANTS
 *******************************************************************************
 */

/**
 * @brief  Direction definition
 */
#define RY_HAL_GPIO_DIR_INPUT             0
#define RY_HAL_GPIO_DIR_OUTPUT            1


/**
 * @brief  GPIO Value Definition
 */
#define RY_HAL_GPIO_LEVEL_LOW             0
#define RY_HAL_GPIO_LEVEL_HIGH            1
#define RY_HAL_GPIO_TRI_STATE             2
#define RY_HAL_GPIO_PULL_UP               3
#define RY_HAL_GPIO_PULL_DOWN             4


/**
 * @brief  Interrupt Trigger Level definition
 */
#define RY_HAL_GINT_LEVEL_LOW             0
#define RY_HAL_GINT_LEVEL_HIGH            1


/*
 * TYPES
 *******************************************************************************
 */

/**
 * @brief  GPIO index definition
 */
typedef enum
{
    GPIO_IDX_LED_ORANGE   = 0x00,
    GPIO_IDX_LED_RED      = 0x01,
    GPIO_IDX_ADV_BUTTON   = 0x02,

    GPIO_IDX_EM9304_IRQ,

    GPIO_IDX_NFC_VEN,
    GPIO_IDX_NFC_DNLD,
    GPIO_IDX_NFC_IRQ,

#if (HW_VER_CURRENT == HW_VER_SAKE_01)
    GPIO_IDX_NFC_BAT_EN,
    GPIO_IDX_NFC_5V_EN,
    GPIO_IDX_SYS_5V_EN,
#endif

    GPIO_IDX_NFC_18V_EN,

    GPIO_IDX_NFC_ESE_PWR_REQ,
    GPIO_IDX_NFC_WKUP_REQ,
    

#if ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
    GPIO_IDX_NFC_SW,
#endif 

    GPIO_IDX_HRM_EN,
    GPIO_IDX_HRM_RESET,
    GPIO_IDX_HRM_IRQ,

    GPIO_IDX_TOUCH_IRQ,
    
    GPIO_IDX_DC_IN_IRQ,

    GPIO_IDX_UART_RX,

    GPIO_IDX_MAX
} ry_gpio_t;

/**
 * @brief  Type definition of interrupt callback function.
 */
typedef void (*ry_hal_int_cb_t)(void);


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init specific GPIO
 *
 * @param   idx  - The specified pin
 * @param   dir  - input or output mode
 *
 * @return  None
 */
void ry_hal_gpio_init(ry_gpio_t idx, u32_t dir);

/**
 * @brief   API to set a GPIO to High
 *
 * @param   idx  - The specified pin
 * @param   val  - The level to be set to the GPIO
 *
 * @return  None
 */
void ry_hal_gpio_set(ry_gpio_t idx, u32_t val);

/**
 * @brief   API to get a GPIO level
 *
 * @param   idx  - The specified pin
 *
 * @return  Level of GPIO
 */
u32_t ry_hal_gpio_get(ry_gpio_t idx);

/**
 * @brief   API to enable GPIO Interrupt
 *
 * @param   idx  - The specified pin
 * @param   cb   - Callback of the interrupt
 *
 * @return  None
 */
void ry_hal_gpio_int_enable(ry_gpio_t idx, ry_hal_int_cb_t cb);

/**
 * @brief   API to enable GPIO Interrupt
 *
 * @param   idx  - The specified pin
 * @param   cb   - Callback of the interrupt
 *
 * @return  None
 */
void ry_hal_gpio_registing_int_cb(ry_gpio_t idx, ry_hal_int_cb_t cb);


/**
 * @brief   API to enable GPIO Interrupt
 *
 * @param   idx  - The specified pin
 *
 * @return  None
 */
void ry_hal_gpio_int_disable(ry_gpio_t idx);


#endif  /* __RY_HAL_GPIO_H__ */
