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
#include "ry_hal_inc.h"
#include "led.h"
#include "device_management_service.h"
#include "app_interface.h"

#if (BSP_MCU_TYPE == BSP_APOLLO2)

/* bsp specified */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_bsp_gpio.h"
#include "am_util.h"
#include "am_hal_interrupt.h"
#include "am_hal_reset.h"
#include "am_hal_stimer.h"
#include "ry_hal_pwm.h"


/*********************************************************************
 * CONSTANTS
 */ 



/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */

ry_led_ctx_t led_ctx_v = {
    LED_STATE_OFF, 
#if ((HW_VER_CURRENT == HW_VER_SAKE_03) && (RELEASE_INTERNAL_TEST == FALSE) && (RELEASE_FACTORY_PRODUCT == FALSE))   
    LED_EN_DISABLE
#else
    LED_EN_ENABLE
#endif
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to enable led on-off
 *
 * @param   en, 1 enable led on-off, 1 disable led for swo function
 *
 * @return  None
 */
void ry_led_enable_set(int en)
{
    led_ctx_v.enable = en;
    if (led_ctx_v.enable == LED_EN_DISABLE){
        am_bsp_pin_enable(ITM_SWO);
    }
    else{
        am_bsp_pin_disable(ITM_SWO);     
    }
}

/**
 * @brief   API to get enable setting state
 *
 * @param   None
 *
 * @return  State of LED.
 */
uint8_t ry_led_enable_get(void)
{
    return led_ctx_v.enable;
}

/**
 * @brief   API to turn LED on/off.
 *
 * @param   onoff, 0 indicating OFF, 1 indicating ON
 *
 * @return  None
 */
void ry_led_onoff(int onoff)
{
    if (led_ctx_v.enable == LED_EN_ENABLE){
        if (onoff) {
            led_ctx_v.state = LED_STATE_ON;
            am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED, AM_HAL_PIN_OUTPUT);
            am_hal_gpio_out_bit_set(AM_BSP_GPIO_PWM_LED);
            //ry_hal_i2cm_init(I2C_TOUCH);
            //touch_set_led_state();
            //ry_hal_i2cm_uninit(I2C_TOUCH);
        } else {
            led_ctx_v.state = LED_STATE_OFF;
            am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED, AM_HAL_PIN_OUTPUT);
            am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_LED);
        }
    }
}

/**
 * @brief   API disable global interrupt
 *
 * @param   onoff, 0 indicating OFF, 1 indicating ON
 * @param   percent, the percent of light level
 *
 * @return  None
 */
void ry_led_onoff_with_effect(int onoff, int percent)
{
    if (led_ctx_v.enable == LED_EN_ENABLE){
        if (onoff) {
            led_ctx_v.state = LED_STATE_ON;
            led_set_working(0, percent, 0);
            //ry_hal_i2cm_init(I2C_TOUCH);
            //touch_set_led_state();
            //ry_hal_i2cm_uninit(I2C_TOUCH);
        } else {
            if (dev_mfg_state_get() == 0xFF)
                return;

            led_ctx_v.state = LED_STATE_OFF;
            led_stop();
        }
    }
}

/**
 * @brief   API to start breath effect
 *
 * @param   onoff, 0 indicating OFF, 1 indicating ON
 *
 * @return  None
 */
void ry_led_breath(int onoff)
{
    if (onoff) {
        led_ctx_v.state = LED_STATE_BREATHING;
    } else {
        led_ctx_v.state = LED_STATE_OFF;
    }
}

/**
 * @brief   API to get current LED state
 *
 * @param   None
 *
 * @return  State of LED.
 */
ry_led_state_t ry_led_state_get(void)
{
    return led_ctx_v.state;
}


/**
 * @brief   API to calculate LED brightness based on OLED brightness
 *
 * @param   oled_brightness, brightness of OLED
 *
 * @return  LED brightness value
 */
int ry_calculate_led_brightness(int oled_brightness)
{
    int led_brightness;
    switch (oled_brightness) {
        case 100:
            led_brightness = 50;
            break;
        case 50:
            led_brightness = 15;
            break;
        case 30:
            led_brightness = 5;
            break;
        default:
            led_brightness = 50;
            break;
    }

    return led_brightness;
}

#endif /* (BSP_MCU_TYPE == BSP_APOLLO2) */

