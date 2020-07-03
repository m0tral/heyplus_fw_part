/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#include "ry_type.h"
#include "ry_hal_inc.h"
#include "ry_utils.h"
#include "am_bsp_gpio.h"
#include "am_bsp_gpio.h"
#include "board.h"

/* Platform Dependency */

#if ((HW_VER_CURRENT == HW_VER_SAKE_01) || (HW_VER_CURRENT == HW_VER_SAKE_03))

#define AM_BSP_GPIO_PWM_VIBE            AM_BSP_GPIO_PWM_MOTOR
#define AM_BSP_GPIO_PWM_LED_R           22
#define AM_BSP_GPIO_PWM_LED_G           22
#define AM_BSP_GPIO_PWM_LED_B           22

#define AM_BSP_GPIO_CFG_PWM_VIBE        AM_HAL_PIN_19_TCTA1
#define AM_BSP_GPIO_CFG_PWM_LED_R       AM_HAL_PIN_22_TCTB1
#define AM_BSP_GPIO_CFG_PWM_LED_G       AM_HAL_PIN_22_TCTB1
#define AM_BSP_GPIO_CFG_PWM_LED_B       AM_HAL_PIN_22_TCTB1

#elif (HW_VER_CURRENT == HW_VER_SAKE_02)

#define AM_BSP_GPIO_PWM_VIBE            AM_BSP_GPIO_PWM_MOTOR
#define AM_BSP_GPIO_PWM_LED_R           23
#define AM_BSP_GPIO_PWM_LED_G           23
#define AM_BSP_GPIO_PWM_LED_B           23

#define AM_BSP_GPIO_CFG_PWM_VIBE        AM_HAL_PIN_19_TCTA1
#define AM_BSP_GPIO_CFG_PWM_LED_R       AM_HAL_PIN_23_TCTB3 //AM_HAL_PIN_23_TCTB1
#define AM_BSP_GPIO_CFG_PWM_LED_G       AM_HAL_PIN_23_TCTB3 //AM_HAL_PIN_23_TCTB1
#define AM_BSP_GPIO_CFG_PWM_LED_B       AM_HAL_PIN_23_TCTB3 //AM_HAL_PIN_23_TCTB1
#endif  /* (HW_VER_CURRENT == HW_VER_SAKE_01) */

#define AM_BSP_PWM_PATTERN_TIMER           	0
#define AM_BSP_PWM_PATTERN_TIMER_SEG        AM_HAL_CTIMER_TIMERA
#define AM_BSP_PWM_PATTERN_TIMER_INT        AM_HAL_CTIMER_INT_TIMERA0

#define AM_BSP_PWM_VIBE_TIMER               1
#define AM_BSP_PWM_VIBE_TIMER_SEG           AM_HAL_CTIMER_TIMERA
#define AM_BSP_PWM_VIBE_TIMER_INT           AM_HAL_CTIMER_INT_TIMERA1

#if (HW_VER_CURRENT == HW_VER_SAKE_02)

#define AM_BSP_PWM_LED_R_TIMER              3//1
#define AM_BSP_PWM_LED_R_TIMER_SEG          AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_R_TIMER_INT          AM_HAL_CTIMER_INT_TIMERB3//AM_HAL_CTIMER_INT_TIMERB1
#define AM_BSP_PWM_LED_G_TIMER              3//1
#define AM_BSP_PWM_LED_G_TIMER_SEG          AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_G_TIMER_INT          AM_HAL_CTIMER_INT_TIMERB3//AM_HAL_CTIMER_INT_TIMERB1
#define AM_BSP_PWM_LED_B_TIMER              3//1
#define AM_BSP_PWM_LED_B_TIMER_SEG          AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_B_TIMER_INT          AM_HAL_CTIMER_INT_TIMERB3//AM_HAL_CTIMER_INT_TIMERB1
#else
#define AM_BSP_PWM_LED_R_TIMER              1//3//1
#define AM_BSP_PWM_LED_R_TIMER_SEG          AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_R_TIMER_INT          AM_HAL_CTIMER_INT_TIMERB1//AM_HAL_CTIMER_INT_TIMERB1
#define AM_BSP_PWM_LED_G_TIMER              1//3//1
#define AM_BSP_PWM_LED_G_TIMER_SEG          AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_G_TIMER_INT          AM_HAL_CTIMER_INT_TIMERB1//AM_HAL_CTIMER_INT_TIMERB1
#define AM_BSP_PWM_LED_B_TIMER              1//3//1
#define AM_BSP_PWM_LED_B_TIMER_SEG          AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_B_TIMER_INT          AM_HAL_CTIMER_INT_TIMERB1//AM_HAL_CTIMER_INT_TIMERB1
#endif

#define LOG_DEBUG_PWM

/**
 * @brief pwm components structure definition
 */
uint32_t g_flg_swo_busy = 0;

pwm_controlblock_t pwmCb = {0};

const pwm_timer_t g_PwmTimers[PWM_IDX_NUM_MAX] = {
    { AM_BSP_PWM_VIBE_TIMER, AM_BSP_PWM_VIBE_TIMER_SEG, AM_BSP_PWM_VIBE_TIMER_INT },
    { AM_BSP_PWM_LED_R_TIMER, AM_BSP_PWM_LED_R_TIMER_SEG, AM_BSP_PWM_LED_R_TIMER_INT},
    { AM_BSP_PWM_LED_G_TIMER, AM_BSP_PWM_LED_G_TIMER_SEG, AM_BSP_PWM_LED_G_TIMER_INT },
    { AM_BSP_PWM_LED_B_TIMER, AM_BSP_PWM_LED_B_TIMER_SEG, AM_BSP_PWM_LED_B_TIMER_INT },
};

static bool pwm_pattern_routine(void);
uint8_t breathing_led(uint8_t index);

void pwm_output_enable(uint8_t ch)
{
	if(ch == PWM_IDX_LED_R)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_R, AM_BSP_GPIO_CFG_PWM_LED_R);
	}
	else if(ch == PWM_IDX_LED_G)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_G, AM_BSP_GPIO_CFG_PWM_LED_G);
	}
	else if(ch == PWM_IDX_LED_B)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_B, AM_BSP_GPIO_CFG_PWM_LED_B);
	}
	else if(ch == PWM_IDX_MOTOR)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_VIBE, AM_BSP_GPIO_CFG_PWM_VIBE);
	}
}

void pwm_output_disable(uint8_t ch)
{
	if(ch == PWM_IDX_LED_R)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_R, AM_HAL_PIN_DISABLE);
		//am_hal_gpio_out_bit_set(AM_BSP_GPIO_PWM_LED_R);
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_LED_R);
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_R, AM_HAL_PIN_OUTPUT);
	}
	else if(ch == PWM_IDX_LED_G)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_G, AM_HAL_PIN_DISABLE);
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_G, AM_HAL_PIN_OUTPUT);
		//am_hal_gpio_out_bit_set(AM_BSP_GPIO_PWM_LED_G);
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_LED_G);
	}
	else if(ch == PWM_IDX_LED_B)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_B, AM_HAL_PIN_DISABLE);
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED_B, AM_HAL_PIN_OUTPUT);
		//am_hal_gpio_out_bit_set(AM_BSP_GPIO_PWM_LED_B);
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_LED_B);
	}
	else if(ch == PWM_IDX_MOTOR)
	{
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_VIBE, AM_HAL_PIN_DISABLE);
		am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_VIBE, AM_HAL_PIN_OUTPUT);
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_VIBE);
	}
}


void pwm_start(uint8_t ch)
{
	//LOG_DEBUG_PWM("%s ch %d period %d set %d\n", "pwm_start", ch, pwmCb.ch[ch].period, pwmCb.ch[ch].set);
    am_hal_ctimer_period_set(g_PwmTimers[ch].instance, g_PwmTimers[ch].channel, pwmCb.ch[ch].period, MAX(1, pwmCb.ch[ch].set));
    am_hal_ctimer_start(g_PwmTimers[ch].instance, g_PwmTimers[ch].channel);
	pwm_output_enable(ch);

	pwmCb.ch[ch].pwm_on = true;
}

void pwm_stop(uint8_t ch)
{
	//LOG_DEBUG_PWM("%s ch %d\n", "pwm_stop", ch);
	am_hal_ctimer_stop(g_PwmTimers[ch].instance, g_PwmTimers[ch].channel);	
	am_hal_ctimer_clear(g_PwmTimers[ch].instance, g_PwmTimers[ch].channel);
	pwm_output_disable(ch);
	
	pwmCb.ch[ch].pwm_on = false;
}

void pwm_int_enable(uint8_t ch)
{
	am_hal_ctimer_int_enable(g_PwmTimers[ch].interrupt);	
}

void pwm_int_clear(uint8_t ch)
{
    am_hal_ctimer_int_clear(g_PwmTimers[ch].interrupt);
}

void pwm_int_disable(uint8_t ch)
{
	am_hal_ctimer_int_disable(g_PwmTimers[ch].interrupt);
}


// target in percentage
// speed in percentage/tick (os tick), if speed == 0, set target duty immediately
// call this function once to set the ramp, actions are done in ISR
static void pwm_ramp(uint8_t channel, uint8_t target, uint8_t speed)
{
    if(channel >= PWM_IDX_NUM_MAX)
    {
        // wrong channel number
        return;
    }

    pwmCb.ch[channel].ramp_step = speed * pwmCb.ch[channel].period / 100;   //percentage into clocks
    pwmCb.ch[channel].target = target * pwmCb.ch[channel].period / 100;     //percentage into clocks

    if(target == 100)
    {
        pwmCb.ch[channel].target--; //avoid timer overflow
    }
	
    if(speed != 0)
    {
        // ramp
        pwmCb.ch[channel].ramping = true;
        if(pwmCb.ch[channel].ramp_step == 0)
        {
            pwmCb.ch[channel].ramp_step = 1;
        }
    }
    else
    {
        // immediate
        pwmCb.ch[channel].ramping = false;
    }

}

// freq in Hz
void pwm_set_period(uint8_t channel, uint16_t freq)
{
    pwmCb.ch[channel].period = (PWM_TIMER_CLOCK_FREQ_HZ / freq);
}

// set period before setting duty
void pwm_set_duty(uint8_t channel, uint8_t duty, uint8_t ramp_speed)
{
    pwm_ramp(channel, duty, ramp_speed);

	if(pwmCb.ch[channel].ramping == false) 
	{
		pwmCb.ch[channel].set = pwmCb.ch[channel].target;
		//am_hal_ctimer_period_set(g_PwmTimers[channel].instance, g_PwmTimers[channel].channel, pwmCb.ch[channel].period, pwmCb.ch[channel].set);
	}
	else
	{
	    if((pwmCb.ch[channel].current == 0)&&(pwmCb.ch[channel].target != 0))
	    {
	        //pwm_start(channel);
			
		}
	}
}

void pwm_ch_int_service(uint32_t ui32Status)
{	
	uint8_t channel = 0xff;
	
	for(uint8_t i = 0; i < PWM_IDX_NUM_MAX; i++)
    {
    	if(!pwmCb.ch[i].enabled)
			continue;
		
		if(ui32Status & g_PwmTimers[i].interrupt)
	    {
	        channel = i;
			break;
	    }
	}
    if(channel != 0xff)
    {
		// LOG_DEBUG_PWM("--> pwm_ch_int_service ch %d duty set to %d \n", channel, pwmCb.ch[channel].set);
	    am_hal_ctimer_period_set(g_PwmTimers[channel].instance, g_PwmTimers[channel].channel, pwmCb.ch[channel].period, MAX(1, pwmCb.ch[channel].set)); 
	    pwmCb.ch[channel].current = pwmCb.ch[channel].set;
	    pwm_int_disable(channel);

	    if(pwmCb.ch[channel].current == 0)
	    {
	        pwm_stop(channel);
	        pwmCb.ch[channel].ramping = false;
            
#if (RY_BUILD == RY_DEBUG)
			if (g_flg_swo_busy){
            	g_flg_swo_busy = 0;			//restore swo for debug print
            	enable_print_interface();     
			}
	        //("%s %d\n", "vibe end", g_flg_swo_busy); 
#endif
	    }
    }
	else {
		//LOG_DEBUG_PWM("--> pwm_ch_int_service status 0x%x\n", ui32Status);
	}
		
}

void pwm_set_para(uint8_t channel, pwm_pattern_t* pattern)
{
		pwmCb.ch[channel].ctrl.target_cnt   = pattern->cnt;
    pwmCb.ch[channel].ctrl.on_ms        = pattern->on_ms;
    pwmCb.ch[channel].ctrl.off_ms       = pattern->off_ms;
    pwmCb.ch[channel].ctrl.duty         = pattern->duty;
    pwmCb.ch[channel].ctrl.ramp_speed   = pattern->ramp_speed;
    pwmCb.ch[channel].ctrl.cnt          = 0;
    pwmCb.ch[channel].ctrl.status       = true;

    pwm_set_duty(channel, pattern->duty, pattern->ramp_speed);
}

void pwm_set_pattern(uint8_t channel, pwm_pattern_t* pattern)
{/*
    pwmCb.ch[channel].ctrl.target_cnt   = pattern->cnt;
    pwmCb.ch[channel].ctrl.on_ms        = pattern->on_ms;
    pwmCb.ch[channel].ctrl.off_ms       = pattern->off_ms;
    pwmCb.ch[channel].ctrl.duty         = pattern->duty;
    pwmCb.ch[channel].ctrl.ramp_speed   = pattern->ramp_speed;
    pwmCb.ch[channel].ctrl.cnt          = 0;
    pwmCb.ch[channel].ctrl.status       = true;

    pwm_set_duty(channel, pattern->duty, pattern->ramp_speed);*/
	pwm_set_para(channel,pattern);
	if(!pwmCb.is_pattern_timer_running) {
		am_hal_ctimer_start(AM_BSP_PWM_PATTERN_TIMER, AM_BSP_PWM_PATTERN_TIMER_SEG);
		pwmCb.is_pattern_timer_running = true;
	}
}

static bool pwm_is_off(uint8_t channel)
{
    if( (pwmCb.ch[channel].pwm_on == false) && (pwmCb.ch[channel].ramping == false))
    {
        return true;
    }
    else
    {
        return false;
    }
}

static bool pwm_is_on(uint8_t channel)
{
    if((pwmCb.ch[channel].set == pwmCb.ch[channel].current) &&(pwm_is_off(channel) == false))
    {
        return true;
    }
    else
    {
        return false;
    }
}

// called when pwm_off event happens
static bool pwm_pattern_routine(void)
{
    for(uint8_t i = 0; i < PWM_IDX_NUM_MAX; i++)
    {
    	if(!pwmCb.ch[i].enabled)
			continue;

		if(pwmCb.ch[i].ctrl.status == true)
        {
			//LOG_DEBUG_PWM("--> pwm %d is off %d cnt %d target_cnt %d\n", i, pwm_is_off(i), pwmCb.ch[i].ctrl.cnt, pwmCb.ch[i].ctrl.target_cnt);
            if(pwm_is_off(i))
            {
                if(pwmCb.ch[i].ctrl.tick_cnt >= pwmCb.ch[i].ctrl.off_ms / PWM_OS_TICK_MS)
                {
                    // off period elapsed
                    pwmCb.ch[i].ctrl.tick_cnt = 0;
                    pwmCb.ch[i].ctrl.cnt++;
                    
                    if(pwmCb.ch[i].ctrl.cnt >= pwmCb.ch[i].ctrl.target_cnt)
                    {
						//if(breathing_led(i))
                        pwmCb.ch[i].ctrl.status = false;
                        
                        // count reached
                        // callback
                        
                    }
                    else
                    {
                        // turn on next pwm output
                        pwm_set_duty(i, pwmCb.ch[i].ctrl.duty, pwmCb.ch[i].ctrl.ramp_speed);
                    }
                }
                else
                {
                    pwmCb.ch[i].ctrl.tick_cnt++;
                }
            }
            else
            {
                if((pwmCb.ch[i].ctrl.on_ms)&&(pwmCb.ch[i].ctrl.tick_cnt >= pwmCb.ch[i].ctrl.on_ms / PWM_OS_TICK_MS))
                {
                    // on period elapsed
                    pwmCb.ch[i].ctrl.tick_cnt = 0;
                    
                    // turn on next pwm output
					if(breathing_led(i))
                    pwm_set_duty(i, 0, pwmCb.ch[i].ctrl.ramp_speed);
                }
                else
                {
                    pwmCb.ch[i].ctrl.tick_cnt++;
                }
            }
        }
        //LOG_DEBUG_PWM("--> pwm %d tick count = %d \n", i, pwmCb.ch[i].ctrl.tick_cnt);
    }

	bool turn_off_pattern_timer = true;
	for(uint8_t i = 0; i < PWM_IDX_NUM_MAX; i++)
	{
		if(!pwmCb.ch[i].enabled)
			continue;
	    if(pwmCb.ch[i].ctrl.status == true) {
			turn_off_pattern_timer = false;
			break;
	    }
	}
	if(turn_off_pattern_timer) {
		// everything is done
        am_hal_ctimer_int_clear(AM_BSP_PWM_PATTERN_TIMER_INT);
		am_hal_ctimer_int_disable(AM_BSP_PWM_PATTERN_TIMER_INT);
        am_hal_ctimer_stop(AM_BSP_PWM_PATTERN_TIMER, AM_BSP_PWM_PATTERN_TIMER_SEG);
		pwmCb.is_pattern_timer_running = false;
		//LOG_DEBUG_PWM("--> pwm pattern timer stopped\n");
	}
    return false;
}


//*****************************************************************************
//
// Determine pwm pattern status
//
//*****************************************************************************
void
pwm_pattern_handler(void)
{
    bool handle_flag = true;
    
    static uint32_t handler_count = 0;
    handler_count++;
    //LOG_DEBUG_PWM("==> pwm_pattern_handler() %d\n", handler_count);

    for(uint8_t i = 0; i < PWM_IDX_NUM_MAX; i++)
    {
    	if(!pwmCb.ch[i].enabled)
			continue;
		
        // load target
        pwmCb.ch[i].set = pwmCb.ch[i].target;
		
        if(pwmCb.ch[i].ramping == false)
        {
            //should not hit here when pwm pattern is configured
        }
        else
        {
            if(pwmCb.ch[i].set > pwmCb.ch[i].current)
            {
                pwmCb.ch[i].set = pwmCb.ch[i].current + pwmCb.ch[i].ramp_step;
                if(pwmCb.ch[i].set >= pwmCb.ch[i].period)
                {
                    pwmCb.ch[i].set = pwmCb.ch[i].period;
                }
                if(pwmCb.ch[i].set > pwmCb.ch[i].target)
                {
                    pwmCb.ch[i].set = pwmCb.ch[i].target;
                }
            }
            else if (pwmCb.ch[i].set < pwmCb.ch[i].current)
            {
                if(pwmCb.ch[i].current < pwmCb.ch[i].ramp_step)
                {
                    pwmCb.ch[i].set = 0;
                }
                else
                {
                    pwmCb.ch[i].set = pwmCb.ch[i].current - pwmCb.ch[i].ramp_step;
                    if(pwmCb.ch[i].set < pwmCb.ch[i].target)
                    {
                        pwmCb.ch[i].set = pwmCb.ch[i].target;
                    }
                }
            }
            else
            {
                // set == current
            }
        }
		//LOG_DEBUG_PWM("==> pwm_pattern_handler ch %d set %d target %d current %d\n", i, pwmCb.ch[i].set, pwmCb.ch[i].target, pwmCb.ch[i].current);
        if(pwmCb.ch[i].set != pwmCb.ch[i].current)
        {
            // turn on pwm interrupt
            //pwm_int_clear(i);
            pwm_int_enable(i);
            handle_flag = false;
        }

        if((pwmCb.ch[i].current == 0)&&(pwmCb.ch[i].set != 0))
        {
            pwm_start(i);
					
            // callback for current pwm channel
					  
        }
    }

//    if(handle_flag)
    {
        //LOG_DEBUG_PWM("==> pwm_handler(); %d\n", handler_count);
        pwm_pattern_routine();

	}
}

#if 1
//*****************************************************************************
//
// Interrupt handler for the CTIMER
//
//*****************************************************************************
void
am_ctimer_isr(void)
{
	uint32_t ui32Status;
	
	//
	// Read the interrupt status.
	//
	ui32Status = am_hal_ctimer_int_status_get(true);

    //
    // Clear ctimer Interrupt.
    //
    am_hal_ctimer_int_clear(ui32Status);

    am_hal_ctimer_int_service(ui32Status);

	pwm_ch_int_service(ui32Status);
	
}
#endif

void pwm_pattern_timer_init(void)
{
	am_hal_ctimer_clear(AM_BSP_PWM_PATTERN_TIMER, AM_BSP_PWM_PATTERN_TIMER_SEG);
	am_hal_ctimer_stop(AM_BSP_PWM_PATTERN_TIMER, AM_BSP_PWM_PATTERN_TIMER_SEG);
	
	// configure the pattern timer to repeat counting function
	am_hal_ctimer_config_single(AM_BSP_PWM_PATTERN_TIMER, AM_BSP_PWM_PATTERN_TIMER_SEG,
                                (AM_HAL_CTIMER_FN_REPEAT |
                                 AM_HAL_CTIMER_XT_32_768KHZ |
                                 AM_HAL_CTIMER_INT_ENABLE));

	// configure the timer frequency to 100Hz (10ms)
    am_hal_ctimer_period_set(AM_BSP_PWM_PATTERN_TIMER, AM_BSP_PWM_PATTERN_TIMER_SEG, 327, 0);

	am_hal_ctimer_int_register(AM_BSP_PWM_PATTERN_TIMER_INT, pwm_pattern_handler);
	am_hal_ctimer_int_clear(AM_BSP_PWM_PATTERN_TIMER_INT);
	am_hal_ctimer_int_enable(AM_BSP_PWM_PATTERN_TIMER_INT);

	am_hal_interrupt_enable(AM_HAL_INTERRUPT_CTIMER);

	pwmCb.is_pattern_timer_running = false;
}

/**
 * @brief   API to init specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_init(ry_pwm_t idx)
{
	uint16_t freq = 100;
    if(idx >= PWM_IDX_NUM_MAX)
		return;
	
    pwm_stop(idx);
	
	if (idx == PWM_IDX_MOTOR){
        freq = (uint16_t)100000;
		am_hal_ctimer_config_single(AM_BSP_PWM_VIBE_TIMER, AM_BSP_PWM_VIBE_TIMER_SEG,
                                (AM_HAL_CTIMER_FN_PWM_REPEAT |
                                 AM_HAL_CTIMER_HFRC_12MHZ |
                                 AM_HAL_CTIMER_INT_ENABLE |
                                 AM_HAL_CTIMER_PIN_ENABLE)); 
	}
	
	if (idx == PWM_IDX_LED_R){
		am_hal_ctimer_config_single(AM_BSP_PWM_LED_R_TIMER, AM_BSP_PWM_LED_R_TIMER_SEG,
                                (AM_HAL_CTIMER_FN_PWM_REPEAT |
                                 AM_HAL_CTIMER_HFRC_12MHZ |
                                 AM_HAL_CTIMER_INT_ENABLE |
                                 //AM_HAL_CTIMER_PIN_INVERT |
                                 AM_HAL_CTIMER_PIN_ENABLE));
	}

	if (idx == PWM_IDX_LED_G){
		am_hal_ctimer_config_single(AM_BSP_PWM_LED_G_TIMER, AM_BSP_PWM_LED_G_TIMER_SEG,
                                (AM_HAL_CTIMER_FN_PWM_REPEAT |
                                 AM_HAL_CTIMER_HFRC_12MHZ |
                                 AM_HAL_CTIMER_INT_ENABLE |
                                 //AM_HAL_CTIMER_PIN_INVERT |
                                 AM_HAL_CTIMER_PIN_ENABLE));
	}
	
	if (idx == PWM_IDX_LED_B){
		am_hal_ctimer_config_single(AM_BSP_PWM_LED_B_TIMER, AM_BSP_PWM_LED_B_TIMER_SEG,
                                (AM_HAL_CTIMER_FN_PWM_REPEAT |
                                 AM_HAL_CTIMER_HFRC_12MHZ |
                                 AM_HAL_CTIMER_INT_ENABLE |
                                 //AM_HAL_CTIMER_PIN_INVERT |
                                 AM_HAL_CTIMER_PIN_ENABLE));
	}

	pwmCb.ch[idx].enabled	  = true;
	pwmCb.ch[idx].current     = 0;
    pwmCb.ch[idx].period      = 0;
    pwmCb.ch[idx].ramping     = false;
    pwmCb.ch[idx].ramp_step   = 0;
    pwmCb.ch[idx].set         = 0;
    pwmCb.ch[idx].target      = 0;
    pwmCb.ch[idx].pwm_on      = false;

    pwm_set_period(idx, freq);
	
}


/**
 * @brief   API to config dutycycle of specified PWM module
 *
 * @param   idx -  The specified pwm module
 * @param   dutycycle - Unit of percent.
 *
 * @return  None
 */
void ry_hal_pwm_config(ry_pwm_t idx, u8_t dutycycle)
{
	if(idx >= PWM_IDX_NUM_MAX)
		return;

	pwm_set_duty(idx, dutycycle, 0);
}

/**
 * @brief   API to start specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_start(ry_pwm_t idx)
{
	if(idx >= PWM_IDX_NUM_MAX)
		return;

	pwm_start(idx);
}

/**
 * @brief   API to stop specified PWM module
 *
 * @param   idx, The specified pwm module
 *
 * @return  None
 */
void ry_hal_pwm_stop(ry_pwm_t idx)
{
	if(idx >= PWM_IDX_NUM_MAX)
		return;

	pwm_stop(idx);
}

/**
 * @brief   API to run the specified PWM module in a specific pattern
 *
 * @param   idx, The specified pwm module
 * @param   pattern, the specificed pwm output pattern
 *
 * @return  None
 */
void ry_hal_pwm_pattern_config(ry_pwm_t idx, pwm_pattern_t* pattern)
{
	if(idx >= PWM_IDX_NUM_MAX || !pattern)
		return;

	pwm_pattern_timer_init();
	pwm_set_pattern(idx, pattern);
}


#define  CYCLE_MILLISECONDS  5000// 5 second breathing cycle.
#define  TOTAL_STEPS         80
const uint8_t dutys[]  = {
  20,  21,  22,  24,  26,  28,  31,  34,  38,  41, 
  45,  50,  55,  60,  66,  73,  80,  87,  95,  103, 
  112, 121, 131, 141, 151, 161, 172, 182, 192, 202, 
  211, 220, 228, 236, 242, 247, 251, 254, 255, 254, 
  251, 247, 242, 236, 228, 220, 211, 202, 192, 182, 
  172, 161, 151, 141, 131, 121, 112, 103, 95,  87, 
  80,  73,  66,  60,  55,  50,  45,  41,  38,  34, 
  31,  28,  26,  24,  22,  21,  20,  20,  20,  20, 
  20,  20,  20,  20,  20,  20,  20, 
};

int breath_step=0;
int mode=0;
uint32_t total_time;
void breath_led_init(uint8_t breath, uint8_t max_bright, uint32_t time)
{
	ry_hal_pwm_init(PWM_IDX_LED_R);
	mode = breath;
	total_time = time;
#if 0	
	ry_hal_pwm_config(PWM_IDX_LED_R, 30);
#else
	breath_step=0;
	pwm_pattern_t test_pattern;
	
    test_pattern.cnt = 1;
	if(mode==0)
	{
		test_pattern.on_ms =  total_time;
		test_pattern.duty = max_bright;
	}
	else
	{
		test_pattern.on_ms =  CYCLE_MILLISECONDS/TOTAL_STEPS;
		test_pattern.duty = 100*dutys[breath_step]/256;
	}
    
    test_pattern.off_ms = 0;
    
    test_pattern.ramp_speed = 0;

	if(mode==0)
	{
		pwm_set_para(PWM_IDX_LED_R, &test_pattern);
		//pwm_set_duty(PWM_IDX_LED_R,max_bright,0);
	//	pwm_int_enable(PWM_IDX_LED_R);
	///	am_hal_interrupt_enable(AM_HAL_INTERRUPT_CTIMER);
	//	am_hal_interrupt_master_enable();
	}
	else
	{
		ry_hal_pwm_pattern_config(PWM_IDX_LED_R, &test_pattern);
	}
	
#endif
	
    g_flg_swo_busy = 1;		//lock swo for breath led
	ry_hal_pwm_start(PWM_IDX_LED_R);
}

void led_stop(void)
{
	 pwmCb.ch[PWM_IDX_LED_R].ctrl.tick_cnt=0;
	 pwm_stop(PWM_IDX_LED_R);
	 //pwm_int_disable(PWM_IDX_LED_R);
	 pwmCb.ch[PWM_IDX_LED_R].ramping = false;
	 //if (g_flg_swo_busy){
	 //	g_flg_swo_busy = 0;			//restore swo for debug print
	 //	enable_print_interface();  
	 //}
}

uint8_t breathing_led(uint8_t index)
{
	if((index==PWM_IDX_LED_R)&&(mode!=0)){
		if(total_time>CYCLE_MILLISECONDS/TOTAL_STEPS){
			pwm_pattern_t test_pattern;
			test_pattern.cnt = 1;
			test_pattern.on_ms = CYCLE_MILLISECONDS/TOTAL_STEPS;
			test_pattern.off_ms = 0;
			test_pattern.duty = 100*dutys[breath_step]/256;;
			test_pattern.ramp_speed = 0;
			pwm_set_pattern(PWM_IDX_LED_R,&test_pattern);
			breath_step++;
			if(breath_step>=TOTAL_STEPS)
				breath_step=0;
			total_time = total_time - CYCLE_MILLISECONDS/TOTAL_STEPS;
			return 0;
		}
	}
	return 1;
}

void vibe_test(void)
{
	ry_hal_pwm_init(PWM_IDX_MOTOR);
#if 0
	ry_hal_pwm_config(PWM_IDX_MOTOR, 1);
#else
	pwm_pattern_t test_pattern;
    test_pattern.cnt = 10;
    test_pattern.on_ms = 600;
    test_pattern.off_ms = 500;
    test_pattern.duty = 80;
    test_pattern.ramp_speed = 5;

    ry_hal_pwm_pattern_config(PWM_IDX_MOTOR, &test_pattern);
#endif

	ry_hal_pwm_start(PWM_IDX_MOTOR);
}

void led_test(void)
{
	ry_hal_pwm_init(PWM_IDX_LED_R);
#if 0	
	ry_hal_pwm_config(PWM_IDX_LED_R, 30);
#else
	pwm_pattern_t test_pattern;
    test_pattern.cnt = 100;
    test_pattern.on_ms = 600;
    test_pattern.off_ms = 0;
    test_pattern.duty = 40;
    test_pattern.ramp_speed = 0;//2;

    ry_hal_pwm_pattern_config(PWM_IDX_LED_R, &test_pattern);
#endif

	ry_hal_pwm_start(PWM_IDX_LED_R);
}

void vibe_start(uint16_t cnt,uint16_t on_ms,uint16_t off_ms,uint8_t duty,uint8_t ramp_speed)
{
	ry_hal_pwm_init(PWM_IDX_MOTOR);

	pwm_pattern_t pattern_cfg;
    pattern_cfg.cnt = cnt;
    pattern_cfg.on_ms = on_ms;
    pattern_cfg.off_ms = off_ms;
    pattern_cfg.duty = duty;
    pattern_cfg.ramp_speed = ramp_speed;

    ry_hal_pwm_pattern_config(PWM_IDX_MOTOR, &pattern_cfg);
	ry_hal_pwm_start(PWM_IDX_MOTOR);
}

void vibe_stop(void)
{
	ry_hal_pwm_stop(PWM_IDX_MOTOR);
	am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_MOTOR, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_MOTOR);
}

/* [] END OF FILE */
