/*
 * File      : board.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-01-05     Bernard      first implementation
 */
#include <stdint.h>
#include <stdarg.h>

#include "app_config.h"
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "board.h"
#include "ry_hal_inc.h"
#include "board_control.h"
#include "am_bsp_gpio.h"
#include "ryos.h"
#include "motar.h"
#include "scheduler.h"
#include "ry_statistics.h"
#include "timer_management_service.h"
#include "sensor_alg.h"

ryos_thread_t *dc_in_thread_handle;
ryos_semaphore_t *dc_in_sem;
static uint32_t charge_time_start;
static uint32_t charge_in_time;
static uint16_t charge_adc_start;
static uint8_t  charge_percent_start;

void auto_reset_stop(void);

/**
 * @brief   power_main_sys power control
 *
 * @param   enable: 0 power on, 1: power off
 *
 * @return  None
 */
void power_off_ctrl(uint8_t enable)
{
#if ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
    if (enable) {
        /* Power off */
        am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_OUTPUT);
        am_hal_gpio_out_bit_clear(AM_BSP_GPIO_OFF_POWER_CTRL);
    } else {
        /* Power on */
        am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_OUTPUT);
        am_hal_gpio_out_bit_set(AM_BSP_GPIO_OFF_POWER_CTRL);
    }
#endif
}

/**
 * @brief   power_main_sys power control
 *
 * @param   ctrl: 0 close power_main_sys, else: open
 *
 * @return  None
 */
void power_main_sys_on(uint8_t ctrl)
{
    if(ctrl != 0){
#if 0
        am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_INPUT);       //此设置屏不亮
#else        
        am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_OUTPUT);      //开电，屏可以点亮
        am_hal_gpio_out_bit_set(AM_BSP_GPIO_OFF_POWER_CTRL);
#endif        
    }
    else{
        am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_OUTPUT);       //关机
        am_hal_gpio_out_bit_clear(AM_BSP_GPIO_OFF_POWER_CTRL);
    }
    am_util_delay_ms(1);
}

/**
 * @brief   main 5v power control
 *
 * @param   ctrl: 0 close main 5v power, else: open
 *
 * @return  None
 */
void power_main_5v_on(uint8_t ctrl)
{
    if(ctrl != 0){
#if 0
	    am_hal_gpio_pin_config(AM_BSP_GPIO_5V_ON, AM_HAL_GPIO_OPENDRAIN);
#else        

        //am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_1V8_SW, AM_HAL_GPIO_OUTPUT);	//2
    	///am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_1V8_SW);
    	//am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_ESE_PWR_REQ, AM_HAL_GPIO_OUTPUT);		//16
    	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_ESE_PWR_REQ);
    	//am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_VEN, AM_HAL_GPIO_OUTPUT);	//25
    	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PN80T_VEN);
    	//am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SCL, AM_HAL_GPIO_DISABLE);	//8
    	//am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SDA, AM_HAL_GPIO_DISABLE);	//9
    	//am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_DISABLE);	//26	
    	//am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_PN80T_DNLD, AM_HAL_GPIO_OUTPUT);	//17
    	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_PN80T_DNLD);	
        //am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_WAKEUP_REQ, AM_HAL_GPIO_DISABLE);

        //am_hal_gpio_pin_config(AM_BSP_GPIO_5V_ON, AM_HAL_PIN_OUTPUT);
        //am_hal_gpio_out_bit_set(AM_BSP_GPIO_5V_ON);
#endif        
    }
    else{
        //am_hal_gpio_pin_config(AM_BSP_GPIO_5V_ON, AM_HAL_GPIO_3STATE);  
    }
    am_util_delay_ms(1);
}

/**
 * @brief   main hrm power control
 *
 * @param   ctrl: 0 close main hrm power, else: open
 *
 * @return  None
 */
void power_hrm_on(uint8_t ctrl)
{
    if(ctrl != 0){
        am_hal_gpio_pin_config(HRM_POWER_EN_PIN, AM_HAL_GPIO_OUTPUT);

#if (HRM_USE_LDO == TRUE)
        am_hal_gpio_out_bit_set(HRM_POWER_EN_PIN); // Using LDO
#else
        am_hal_gpio_out_bit_clear(HRM_POWER_EN_PIN);
#endif
    }
    else{
        am_hal_gpio_pin_config(HRM_POWER_EN_PIN, AM_HAL_GPIO_OUTPUT);  

#if (HRM_USE_LDO == TRUE)
        am_hal_gpio_out_bit_clear(HRM_POWER_EN_PIN);
#else
        am_hal_gpio_out_bit_set(HRM_POWER_EN_PIN);
#endif

    }
    ryos_delay_ms(300);
}

/**
 * @brief   oled power control
 *
 * @param   ctrl: 0 close oled power, else: open
 *
 * @return  None
 */
void power_oled_on(uint8_t ctrl)
{
    if (ctrl != 0) {
        /* Open OLED */
        am_hal_gpio_pin_config(AM_BSP_GPIO_LCD_ON, AM_HAL_GPIO_OUTPUT);
#if (HW_VER_CURRENT == HW_VER_SAKE_01)
	    am_hal_gpio_out_bit_clear(AM_BSP_GPIO_LCD_ON);
#elif (HW_VER_CURRENT == HW_VER_SAKE_02)
        am_hal_gpio_out_bit_set(AM_BSP_GPIO_LCD_ON);        
#elif (HW_VER_CURRENT == HW_VER_SAKE_03)
		am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_EN, AM_HAL_GPIO_OUTPUT);   //VCI     
        am_hal_gpio_out_bit_set(AM_BSP_GPIO_LCD_ON);
		am_util_delay_ms(15);
		am_hal_gpio_out_bit_set(AM_BSP_GPIO_CHG_EN);
#endif
    }
    else {
        /* Close OLED */
        
#if (HW_VER_CURRENT == HW_VER_SAKE_01)
        am_hal_gpio_pin_config(AM_BSP_GPIO_LCD_ON, AM_HAL_GPIO_OPENDRAIN);
#elif (HW_VER_CURRENT == HW_VER_SAKE_02)
        am_hal_gpio_out_bit_clear(AM_BSP_GPIO_LCD_ON);        
#elif (HW_VER_CURRENT == HW_VER_SAKE_03)
		 am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_EN); // VCI
		am_util_delay_ms(60);
        am_hal_gpio_out_bit_clear(AM_BSP_GPIO_LCD_ON);
#endif

    }
#if (HW_VER_CURRENT == HW_VER_SAKE_03)        
	am_util_delay_ms(15);
#else
	am_util_delay_ms(1);
#endif    
}

void dc_in_irq(void)
{
    ryos_semaphore_post(dc_in_sem);
}

/**
 * @brief   dc in detect and auto reset isr processing
 *
 * @param   None
 *
 * @return  None
 */
u8_t dc_in_flag = 0, dc_in_change = 0;
int  dc_in_rising_cnt;
int  dc_in_falling_cnt;
void dc_in_handler(void)
{
    am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));
    ry_hal_gpio_registing_int_cb(GPIO_IDX_DC_IN_IRQ, NULL);

#if (HW_VER_CURRENT == HW_VER_SAKE_03)
    if (am_hal_gpio_input_bit_read(AM_BSP_GPIO_DC_IN))	
#else
    if (!am_hal_gpio_input_bit_read(AM_BSP_GPIO_DC_IN))	
#endif    
    {
        //LOG_DEBUG("dc_in handler, Detect. Enable 32K.\r\n");
        dc_in_rising_cnt++;
    /* Charge detected, enable AUTO_RESET */
#if (HW_VER_CURRENT == HW_VER_SAKE_01)
        am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_OUTPUT);
        am_hal_gpio_out_bit_clear(AM_BSP_GPIO_AUTO_RESET);
        am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_RISING);
#elif (HW_VER_CURRENT == HW_VER_SAKE_02)
        //am_bsp_pin_enable(AUTO_RESET);    
        auto_reset_start();
        am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_RISING);
        am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));            
#elif (HW_VER_CURRENT == HW_VER_SAKE_03)
        //am_bsp_pin_enable(AUTO_RESET);
        auto_reset_start();
        am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_FALLING);
        am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));
#endif
        dc_in_flag = 1;
        motar_light_linear_working(200);
        if (ry_system_initial_status() != 0){
            ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
        }
        alg_sleep_scheduler_set_enable(0);
        //DEV_STATISTICS(charge_begin_count);
        charge_time_start = ry_hal_clock_time();
        charge_adc_start = sys_get_battery_adc();
        charge_percent_start = sys_get_battery();
    }
    else {
        //LOG_DEBUG("dc_in handler, Detect Leave. Disable 32K.\r\n");
        dc_in_falling_cnt++;
        //am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_OPENDRAIN);
        //am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_INPUT);
        auto_reset_stop();
#if (HW_VER_CURRENT == HW_VER_SAKE_03)        
        am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_RISING);
#else
        am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_FALLING);
#endif        
        am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));
        dc_in_flag = 0;

        if (charge_time_start != 0){
            charge_in_time += ry_hal_calc_ms(ry_hal_clock_time() - charge_time_start);
            charge_time_start = 0;
        }
        if (ry_system_initial_status() != 0){
            ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
            LOG_DEBUG("charge finished, count:%d, times:%ds, adc_start:%d, percent_start:%d, adc:%d, percent:%d\r\n", \
                    DEV_STATISTICS_VALUE(charge_begin_count), charge_in_time / 1000, \
                    charge_adc_start, charge_percent_start, sys_get_battery(), sys_get_battery_adc());
        }
        alg_sleep_scheduler_set_enable(1);        
    }
    dc_in_change |= 0x01;

    ry_hal_gpio_registing_int_cb(GPIO_IDX_DC_IN_IRQ, dc_in_irq);
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));
}





/**
 * @brief   dc in pin initial config
 *
 * @param   None
 *
 * @return  None
 */
void dc_in_hw_init(void)
{
    
#if (HW_VER_CURRENT == HW_VER_SAKE_03)      
    am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_INPUT/*|AM_HAL_GPIO_PULL24K*/);
#else
    am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_INPUT|AM_HAL_GPIO_PULL24K);
#endif    
    // Clear the GPIO Interrupt (write to clear).
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));

#if (DC_IN_IRQ_ENABLE == TRUE)
    ry_hal_gpio_registing_int_cb(GPIO_IDX_DC_IN_IRQ, dc_in_irq);
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));
#endif
    
    am_hal_interrupt_master_enable();
}


void dc_in_hw_disable(void)
{
#if (HW_VER_CURRENT == HW_VER_SAKE_03)      
    am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_DISABLE/*|AM_HAL_GPIO_PULL24K*/);
#else
    am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_DISABLE);
#endif    
    // Clear the GPIO Interrupt (write to clear).
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));

#if (DC_IN_IRQ_ENABLE == TRUE)
    am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN));
#endif

}




/**
 * @brief   dc_in_thread
 *
 * @param   None
 *
 * @return  None
 */
#include "device_management_service.h"
int dc_in_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;

    LOG_DEBUG("[dc_in_thread] Enter.\n");

    status = ryos_semaphore_create(&dc_in_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[dc_in_thread]: Semaphore Create Error.\r\n");
        while(1);
    }

    dc_in_hw_init();

    while(1) {
        
        status = ryos_semaphore_wait(dc_in_sem);
        if (RY_SUCC != status) {
            LOG_ERROR("[dc_in_thread]: Semaphore Wait Error.\r\n");
            continue;
        }
        ryos_delay_ms(50);

        if (dev_mfg_state_get() >= DEV_MFG_STATE_SEMI) {
            dc_in_handler();
            if(dc_in_flag){
                LOG_DEBUG("IPC_MSG_TYPE_CHARGING_START\n");
                ry_sched_msg_send(IPC_MSG_TYPE_CHARGING_START, 0, NULL);
                //DEV_STATISTICS(charge_begin_count);
                DEV_STATISTICS_SET_TIME(charge_begin_count, charge_begin_time, ryos_rtc_now());
            }else{
                LOG_DEBUG("IPC_MSG_TYPE_CHARGING_END\n");
                ry_sched_msg_send(IPC_MSG_TYPE_CHARGING_END, 0, NULL);
                //DEV_STATISTICS(charge_end_count);
                DEV_STATISTICS_SET_TIME(charge_end_count, charge_stop_time, ryos_rtc_now());
            }
        }
    }
}


/**
 * @brief   API to init Touch module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t dc_in_init(void)
{
    ryos_threadPara_t para;

#if (DC_IN_IRQ_ENABLE == TRUE)
    /* Do it once at power up to make sure generate 32K signal correct */
#if (HW_VER_CURRENT == HW_VER_SAKE_03)      
    am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_INPUT/*|AM_HAL_GPIO_PULL24K*/);
#else
    am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_INPUT|AM_HAL_GPIO_PULL24K);
#endif       
    dc_in_handler();
#endif /*  */
    
    /* Start BLE Thread */
    strcpy((char*)para.name, "dc_in_thread");
    para.stackDepth = 400;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 8;
    ryos_thread_create(&dc_in_thread_handle, dc_in_thread, &para);  

    return RY_SUCC;
}


void charge_gpio_init(void)
{
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_STA_DET, AM_HAL_GPIO_INPUT|AM_HAL_GPIO_PULL24K);	//上拉输入，识别是否在充电，低电平在充电，高电平没有充电
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_EN, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_EN);
}

bool charge_cable_is_input(void)
{
	if(am_hal_gpio_input_bit_read(AM_BSP_GPIO_DC_IN))
		return true;
	else
		return false;
}
// ensure pin AM_BSP_GPIO_CHG_EN is clear before call this function.
bool battery_is_charging(void)
{
	
	if(!am_hal_gpio_input_bit_read(AM_BSP_GPIO_CHG_STA_DET))
		return true;
	else
		return false;
}

/**
 * @brief   board power control initial
 *
 * @param   None
 *
 * @return  None
 */
void board_control_init(void)
{   

#if (HW_VER_CURRENT == HW_VER_SAKE_01)
    power_main_sys_on(1);       //系统供电开
    power_main_5v_on(1);        //5v供电开
#endif

    dc_in_init();
    charge_gpio_init();
    
    //power_hrm_on(1);            //心率供电开
    //power_oled_on(1);           //oled供电开    
}


void auto_reset_init(void)
{
    am_bsp_pin_enable(AUTO_RESET);

#if 0
    // Configure a timer to drive the LED.
    am_hal_ctimer_config_single(1, AM_HAL_CTIMER_TIMERB,                                
                                (AM_HAL_CTIMER_FN_PWM_REPEAT |                                 
                                 AM_HAL_CTIMER_XT_32_768KHZ |                                                                
                                 AM_HAL_CTIMER_PIN_ENABLE));    
    //    
    // Set up initial timer period.    
    //    
    am_hal_ctimer_period_set(1, AM_HAL_CTIMER_TIMERB, 4, 2);    
#endif

}


void auto_reset_start(void)
{
    auto_reset_init();
    //am_hal_ctimer_start(1, AM_HAL_CTIMER_TIMERB);
    am_bsp_pin_enable(AUTO_RESET);
}

void auto_reset_stop(void)
{
    //am_hal_ctimer_stop(1, AM_HAL_CTIMER_TIMERB);
    am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_INPUT);
}

