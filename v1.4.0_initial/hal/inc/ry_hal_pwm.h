/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_PWM_H__
#define __RY_HAL_PWM_H__

/*
 * INCLUDES
 *******************************************************************************
 */
#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */
	
#define PWM_OS_TICK_MS                      10 //WSF_MS_PER_TICK
#define PWM_TIMER_CLOCK_FREQ_HZ             12000000
	


/*
 * TYPES
 *******************************************************************************
 */
typedef enum {
    PWM_IDX_MOTOR = 0x00,
    PWM_IDX_LED_R,
    PWM_IDX_LED_G,
    PWM_IDX_LED_B,
    PWM_IDX_NUM_MAX,
} ry_pwm_t;

typedef struct pwm_ctrl{
	uint16_t target_cnt;	// target PWM output on counts
	uint16_t cnt;
	uint16_t on_ms; 		// PWM output on time in ms
	uint16_t off_ms;		// PWM output off interval in ms
	uint16_t tick_cnt;		// PWM tick counter
	uint16_t duty;			// in % saved for pattern control
	uint16_t ramp_speed;	// in % saved for pattern control
	uint8_t  status;		// PWM output status (on or off)
}pwm_ctrl_t;

typedef struct pwm_data{
	bool enabled;
	bool ramping;
	uint16_t period;	// in clocks
	uint16_t target;	// duty target, in clocks
	uint16_t set;		// duty to set in current cycle, in clocks
	uint16_t current;	// duty current, in clocks
	uint16_t ramp_step; // ramping step, in clocks (clocks/os tick)
	bool pwm_on;		// PWM output status on this channel
	pwm_ctrl_t ctrl;	// channel control parameters
}pwm_data_t;

typedef struct pwm_pattern{
	uint16_t cnt;
	uint16_t on_ms;
	uint16_t off_ms;
	uint8_t duty;
	uint8_t ramp_speed;
}pwm_pattern_t;

typedef struct pwm_controlblock{
	pwm_data_t ch[PWM_IDX_NUM_MAX];
	bool is_pattern_timer_running;
}pwm_controlblock_t;

typedef struct pwm_timer{
	uint16_t instance;
	uint32_t channel;
	uint32_t interrupt;
}pwm_timer_t;

typedef enum pwm_event{
	Pwm_Evt_None,
	Pwm_Evt_Start,
	Pwm_Evt_Stop
}pwm_event_t;

extern uint8_t pwm_evt;
extern pwm_controlblock_t pwmCb;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_init(ry_pwm_t idx);

/**
 * @brief   API to power up specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_powerup(ry_pwm_t idx);

/**
 * @brief   API to power down specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_powerdown(ry_pwm_t idx);

/**
 * @brief   API to config dutycycle of specified PWM module
 *
 * @param   idx -  The specified pwm module
 * @param   dutycycle - Unit of percent.
 *
 * @return  None
 */
void ry_hal_pwm_config(ry_pwm_t idx, u8_t dutycycle);

/**
 * @brief   API to start specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_start(ry_pwm_t idx);

/**
 * @brief   API to stop specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_stop(ry_pwm_t idx); 

/**
 * @brief   API to run the specified PWM module in a specific pattern
 *
 * @param   idx, The specified pwm module
 * @param   pattern, the specificed pwm output pattern
 *
 * @return  None
 */
void ry_hal_pwm_pattern_config(ry_pwm_t idx, pwm_pattern_t* pattern);

void vibe_start(uint16_t cnt,uint16_t on_ms,uint16_t off_ms,uint8_t duty,uint8_t ramp_speed);
void vibe_stop(void);
void led_stop(void);
void breath_led_init(uint8_t breath, uint8_t max_bright, uint32_t time);

void breath_led_init(uint8_t breath, uint8_t max_bright, uint32_t time);

#endif  /* __RY_HAL_PWM_H__ */
