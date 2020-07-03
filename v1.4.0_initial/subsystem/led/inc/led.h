/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __LED_H__
#define __LED_H__

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
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BREATHING,
} ry_led_state_t;

typedef enum {
    LED_EN_DISABLE,
    LED_EN_ENABLE,
} ry_led_enable_t;

typedef struct {
    ry_led_state_t  state;
    u8_t            enable;
} ry_led_ctx_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to turn LED on/off.
 *
 * @param   onoff, 0 indicating OFF, 1 indicating ON
 *
 * @return  None
 */
void ry_led_onoff(int onoff);

/**
 * @brief   API disable global interrupt
 *
 * @param   onoff, 0 indicating OFF, 1 indicating ON
 * @param   percent, the percent of light level
 *
 * @return  None
 */
void ry_led_onoff_with_effect(int onoff, int percent);

/**
 * @brief   API to start breath effect
 *
 * @param   onoff, 0 indicating OFF, 1 indicating ON
 *
 * @return  None
 */
void ry_led_breath(int onoff);

/**
 * @brief   API to get current LED state
 *
 * @param   None
 *
 * @return  State of LED.
 */
ry_led_state_t ry_led_state_get(void);

int ry_calculate_led_brightness(int oled_brightness);

/**
 * @brief   API to get enable setting state
 *
 * @param   None
 *
 * @return  State of LED.
 */
ry_led_state_t ry_led_enable_get(void);

/**
 * @brief   API to enable led on-off
 *
 * @param   en, 1 enable led on-off, 0 disable led for swo function
 *
 * @return  None
 */
void ry_led_enable_set(int en);

#endif  /* __LED_H__ */


