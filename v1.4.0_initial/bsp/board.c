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
#include <stdio.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "board.h"
#include "ry_hal_inc.h"
#include "board_control.h"
#include "scheduler.h"
#include "device_management_service.h"
#include "am_devices_CY8C4014.h"
#include "am_devices_rm67162.h"
#include "ff.h"

#if RY_BUILD == RY_DEBUG
#define USE_RTT     1
#else
#define USE_RTT     0
#endif


#if USE_RTT
#include "SEGGER_RTT_Conf.h"
#include "SEGGER_RTT.c"
#include "SEGGER_RTT.h"
    #if RY_SEGGER_SYSVIEW_ENABLE
        #include "SEGGER_SYSVIEW_FreeRTOS.c"
        #include "SEGGER_SYSVIEW_FreeRTOS.h"
        #include "SEGGER_SYSVIEW.c"
        #include "SEGGER_SYSVIEW_Config_FreeRTOS.c"
    #endif
#endif

void InitSPIComm();

#define RY_DEBUG_LOG_BUF_SIZE       1024

extern uint32_t g_flg_swo_busy;


ryos_mutex_t* log_mutex;
static char g_log_buffer [RY_DEBUG_LOG_BUF_SIZE];
static int	reset_state;


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler */
}

/**
 * This is the timer interrupt service routine.
 *
 */


//*****************************************************************************
//
// Enable printing to the console.
//
//*****************************************************************************
void
enable_print_interface(void)
{
    //
    // Initialize a debug printing interface.
    //
#if USE_RTT

#if RY_SEGGER_SYSVIEW_ENABLE
    SEGGER_SYSVIEW_Conf();
    SEGGER_SYSVIEW_Start();
#else
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    SEGGER_RTT_WriteString(0, "\n\n\n\n\n\n\n\n###### SEGGER DbgStart done. ######\r\n");
#endif
    SEGGER_RTT_WriteString(0, "\n\n\n\n\n\n\n\n###### SEGGER SysView done. ######\r\n");
#else
    am_hal_itm_enable();
    am_bsp_debug_printf_enable();
    am_util_debug_printf_init(am_hal_itm_print);
    am_util_stdio_terminal_clear();
#endif
}

void
am_hal_itm_print_test(u8 data)
{
    //
    // Print string out the ITM.
    //
    am_hal_itm_stimulus_reg_byte_write(1, (uint8_t)data);

}

#if 0
void ry_board_default_pins(void)
{
	
	//BLE
	am_hal_gpio_pin_config(AM_BSP_GPIO_BLE_EN, AM_HAL_GPIO_OUTPUT);			//40
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_BLE_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_EM9304_CS, AM_HAL_GPIO_OUTPUT);		//46
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_EM9304_CS);	
	am_hal_gpio_pin_config(AM_BSP_GPIO_EM9304_INT, AM_HAL_GPIO_DISABLE);	//45
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM5_MISO, AM_HAL_GPIO_DISABLE);		//49
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM5_MOSI, AM_HAL_GPIO_OUTPUT);		//47
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM5_MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_IOM5_SCK, AM_HAL_GPIO_OUTPUT);	//48
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_IOM5_SCK);

	//HRM
	am_hal_gpio_pin_config(HRM_POWER_EN_PIN, AM_HAL_GPIO_OUTPUT);	//12
	am_hal_gpio_out_bit_set(HRM_POWER_EN_PIN);
	am_hal_gpio_pin_config(HRM_RESETZ_CTRL_PIN, AM_HAL_GPIO_DISABLE);	//27
	am_hal_gpio_pin_config(PPS_INTERRUPT_PIN, AM_HAL_GPIO_DISABLE);		//28
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM2_SCL, AM_HAL_GPIO_DISABLE);	//0
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM2_SDA, AM_HAL_GPIO_DISABLE);	//1

	//NFC
#if (HW_VER_CURRENT == HW_VER_SAKE_01)
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_BAT_EN, AM_HAL_GPIO_OUTPUT);	//3
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_BAT_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_5V_ON, AM_HAL_GPIO_OUTPUT);		//16
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_5V_ON);
	am_hal_gpio_pin_config(23, AM_HAL_GPIO_DISABLE);	//23
#elif ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
  am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_SW, AM_HAL_GPIO_OUTPUT);	//30
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_SW);
#endif
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_1V8_SW, AM_HAL_GPIO_OUTPUT);	//2
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_NFC_1V8_SW);
	am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_VEN, AM_HAL_GPIO_OUTPUT);	//25
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PN80T_VEN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SCL, AM_HAL_GPIO_DISABLE);	//8
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SDA, AM_HAL_GPIO_DISABLE);	//9
	am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_DISABLE);	//26	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_PN80T_DNLD, AM_HAL_GPIO_DISABLE);	//17
	am_hal_gpio_pin_config(24, AM_HAL_GPIO_DISABLE);	//24
	
#if 1	
	//FLASH & GS
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_G_CS, AM_HAL_GPIO_OUTPUT);	//10
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_G_CS);	
	am_hal_gpio_pin_config(AM_BSP_GPIO_GSENSOR_INT, AM_HAL_GPIO_DISABLE);	//18

	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_HAL_GPIO_OUTPUT);	//11
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_FL_CS);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_DISABLE);	//6
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_IOM0_MOSI, AM_HAL_GPIO_OUTPUT);	//7
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_IOM0_MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_SCK, AM_HAL_GPIO_OUTPUT);	//5
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_SCK);

	//LCD
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_OUTPUT);		//35
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_CS);
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_TE, AM_HAL_GPIO_DISABLE);	//36
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_DCX, AM_HAL_GPIO_OUTPUT);	//38
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_DCX);
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_GPIO_OUTPUT);	//37
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_RES);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_SCK, AM_HAL_GPIO_OUTPUT);		//39
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM4_SCK);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_MOSI, AM_HAL_GPIO_OUTPUT);		//44
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM4_MOSI);
	
	//TP
	am_hal_gpio_pin_config(AM_BSP_GPIO_TP_INT, AM_HAL_GPIO_DISABLE);		//41
	am_hal_gpio_pin_config(42, AM_HAL_GPIO_DISABLE);		//42
	am_hal_gpio_pin_config(43, AM_HAL_GPIO_DISABLE);		//43

	
	//MISC
	am_hal_gpio_pin_config(AM_BSP_GPIO_NTC_EN, AM_HAL_GPIO_OUTPUT);		//14
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_NTC_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_NTC_ADC_IN, AM_HAL_GPIO_DISABLE);	//13

#if (HW_VER_CURRENT == HW_VER_SAKE_01)
	am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_OUTPUT);	//15
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_OFF_POWER_CTRL);
#endif
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_MOTOR, AM_HAL_GPIO_OUTPUT);	//19
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_MOTOR);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED, AM_HAL_GPIO_OUTPUT);	//22
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_LED);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_EN, AM_HAL_GPIO_OUTPUT);	//29
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_CHG_EN);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_BAT_DET_SW, AM_HAL_GPIO_OUTPUT);	//30
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_BAT_DET_SW);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CLKOUT_PIN, AM_HAL_GPIO_DISABLE);	//4
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_STA_DET, AM_HAL_GPIO_DISABLE);	//31
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_STA_DET);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_DISABLE);	//32
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_DC_IN);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_LEV_DET, AM_HAL_GPIO_DISABLE);	//33
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_LEV_DET);

	am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_DISABLE);	//34
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_AUTO_RESET);

#endif
	
	
}
#endif

void ry_board_default_pins(void)
{
	am_hal_iom_pwrctrl_disable(0);
    #if 0
    am_hal_iom_pwrctrl_disable(1);	
    am_hal_iom_pwrctrl_disable(2);	
    am_hal_iom_pwrctrl_disable(3);	
    am_hal_iom_pwrctrl_disable(4);	
    am_hal_iom_pwrctrl_disable(5);	


    _start_func_gsensor();
    pcf_func_gsensor();
    ry_hal_spim_uninit(SPI_IDX_GSENSOR);
    #endif

    
    //am_hal_itm_disable();
    
	//BLE
	am_hal_gpio_pin_config(AM_BSP_GPIO_BLE_EN, AM_HAL_GPIO_OUTPUT);			//40
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_BLE_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_EM9304_CS, AM_HAL_GPIO_OUTPUT);		//46
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_EM9304_CS);
    am_hal_gpio_out_bit_set(AM_BSP_GPIO_EM9304_CS);
	am_hal_gpio_pin_config(AM_BSP_GPIO_EM9304_INT, AM_HAL_GPIO_DISABLE);	//45
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM5_MISO, AM_HAL_GPIO_DISABLE);		//49
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM5_MOSI, AM_HAL_GPIO_OUTPUT);		//47
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM5_MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_IOM5_SCK, AM_HAL_GPIO_OUTPUT);	//48
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_IOM5_SCK);

	//HRM
	am_hal_gpio_pin_config(HRM_POWER_EN_PIN, AM_HAL_GPIO_OUTPUT);	//12
#if (HRM_USE_LDO == TRUE)
    am_hal_gpio_out_bit_clear(HRM_POWER_EN_PIN);
#else
    am_hal_gpio_out_bit_set(HRM_POWER_EN_PIN);
#endif


	am_hal_gpio_pin_config(HRM_RESETZ_CTRL_PIN, AM_HAL_GPIO_DISABLE);	//27
	am_hal_gpio_pin_config(PPS_INTERRUPT_PIN, AM_HAL_GPIO_DISABLE);		//28
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM2_SCL, AM_HAL_GPIO_DISABLE);	//0
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM2_SDA, AM_HAL_GPIO_DISABLE);	//1

	//NFC
	//am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_BAT_EN, AM_HAL_GPIO_OUTPUT);	//3
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_BAT_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_SW, AM_HAL_GPIO_OUTPUT);	//30
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_SW);
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_1V8_SW, AM_HAL_GPIO_OUTPUT);	//2
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_1V8_SW);
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_ESE_PWR_REQ, AM_HAL_GPIO_OUTPUT);		//16

#if 1
    am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_SW, AM_HAL_GPIO_OUTPUT);	//30
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_NFC_SW);
    am_hal_gpio_out_bit_set(AM_BSP_GPIO_NFC_1V8_SW);

    am_util_delay_ms(100);

    am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_SW);
    am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_1V8_SW);
#endif
    
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_ESE_PWR_REQ);
	am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_VEN, AM_HAL_GPIO_OUTPUT);	//25
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PN80T_VEN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SCL, AM_HAL_GPIO_DISABLE);	//8
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SDA, AM_HAL_GPIO_DISABLE);	//9
	am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_DISABLE);	//26	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_PN80T_DNLD, AM_HAL_GPIO_OUTPUT);	//17
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_PN80T_DNLD);	
	
	//am_hal_gpio_pin_config(23, AM_HAL_GPIO_DISABLE);	//23
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_WAKEUP_REQ, AM_HAL_GPIO_DISABLE);	//24
#if 1	
	//FLASH & GS
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_G_CS, AM_HAL_GPIO_OUTPUT);	//10
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_G_CS);	
	am_hal_gpio_pin_config(AM_BSP_GPIO_GSENSOR_INT, AM_HAL_GPIO_DISABLE);	//18

	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_HAL_GPIO_OUTPUT);	//11
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_FL_CS);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_DISABLE);	//6
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_IOM0_MOSI, AM_HAL_GPIO_OUTPUT);	//7
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_IOM0_MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_SCK, AM_HAL_GPIO_OUTPUT);	//5
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_SCK);

	//LCD
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_LCD_ON, AM_HAL_GPIO_OUTPUT);	//3
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_LCD_ON);

    am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_EN, AM_HAL_GPIO_OUTPUT);	//29 VCI
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_EN);
	
	//am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_OUTPUT);		//35
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_CS);
    am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_DISABLE);
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_TE, AM_HAL_GPIO_DISABLE);	//36
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_DCX, AM_HAL_GPIO_OUTPUT);	//38
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_DCX);
	//am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_GPIO_OUTPUT);	//37
	//am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_RES);
    am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_GPIO_DISABLE);	//37
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_SCK, AM_HAL_GPIO_OUTPUT);		//39
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM4_SCK);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_MOSI, AM_HAL_GPIO_OUTPUT);		//44
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM4_MOSI);
	
	//TP
    
	am_hal_gpio_pin_config(AM_BSP_GPIO_TP_INT, AM_HAL_GPIO_DISABLE);		//41
	am_hal_gpio_pin_config(42, AM_HAL_GPIO_DISABLE);		//42
	am_hal_gpio_pin_config(43, AM_HAL_GPIO_DISABLE);		//43

    //cy8c4011_gpio_init();
	//cy8c4011_gpio_uninit();

	
	//MISC
	am_hal_gpio_pin_config(AM_BSP_GPIO_NTC_EN, AM_HAL_GPIO_OUTPUT);		//14
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NTC_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_NTC_ADC_IN, AM_HAL_GPIO_DISABLE);	//13
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_TOOL_UART_RX, AM_HAL_GPIO_DISABLE);	//15
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_MOTOR, AM_HAL_GPIO_OUTPUT);	//19
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_MOTOR);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_ITM_SWO, AM_HAL_GPIO_OUTPUT);	//22
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_ITM_SWO);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CLKOUT_PIN, AM_HAL_GPIO_DISABLE);	//4
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_STA_DET, AM_HAL_GPIO_DISABLE);	//31
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_STA_DET);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_DISABLE);	//32
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_DC_IN);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_LEV_DET, AM_HAL_GPIO_DISABLE);	//34, BAT_LEVEL
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_LEV_DET);

	am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_DISABLE);	//33
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_AUTO_RESET);

#endif
	
	
}


void sake_default_pins(void)
{
		am_hal_gpio_pin_config(AM_BSP_GPIO_OFF_POWER_CTRL, AM_HAL_GPIO_OUTPUT);	//23
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_OFF_POWER_CTRL);
	am_hal_iom_pwrctrl_disable(0);	
	am_hal_iom_pwrctrl_disable(1);	
	am_hal_iom_pwrctrl_disable(2);	
	am_hal_iom_pwrctrl_disable(3);	
	am_hal_iom_pwrctrl_disable(4);	
	am_hal_iom_pwrctrl_disable(5);	
	
	am_hal_uart_disable(0);
	am_hal_uart_disable(1);
	am_hal_uart_clock_disable(0);
	am_hal_uart_clock_disable(1);
	
	#if 1
	//BLE
	am_hal_gpio_pin_config(AM_BSP_GPIO_BLE_EN, AM_HAL_GPIO_OUTPUT);			//40
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_BLE_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_EM9304_CS, AM_HAL_GPIO_OUTPUT);		//46
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_EM9304_CS);	
	am_hal_gpio_pin_config(AM_BSP_GPIO_EM9304_INT, AM_HAL_GPIO_DISABLE);	//45
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM5_MISO, AM_HAL_GPIO_DISABLE);		//49
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM5_MOSI, AM_HAL_GPIO_OUTPUT);		//47
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM5_MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_IOM5_SCK, AM_HAL_GPIO_OUTPUT);	//48
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_IOM5_SCK);

	//HRM
	am_hal_gpio_pin_config(HRM_POWER_EN_PIN, AM_HAL_GPIO_OUTPUT);	//12
	am_hal_gpio_out_bit_clear(HRM_POWER_EN_PIN);
	am_hal_gpio_pin_config(HRM_RESETZ_CTRL_PIN, AM_HAL_GPIO_DISABLE);	//27
	am_hal_gpio_pin_config(PPS_INTERRUPT_PIN, AM_HAL_GPIO_DISABLE);		//28
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM2_SCL, AM_HAL_GPIO_DISABLE);	//0
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM2_SDA, AM_HAL_GPIO_DISABLE);	//1

	//NFC
	//am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_BAT_EN, AM_HAL_GPIO_OUTPUT);	//3
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_BAT_EN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_1V8_SW, AM_HAL_GPIO_OUTPUT);	//2
//	am_hal_gpio_out_bit_set(AM_BSP_GPIO_NFC_1V8_SW);
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_1V8_SW);
	
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_ESE_PWR_REQ, AM_HAL_GPIO_DISABLE);		//16
//	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_5V_ON);
	am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_VEN, AM_HAL_GPIO_OUTPUT);	//25
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PN80T_VEN);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SCL, AM_HAL_GPIO_DISABLE);	//8
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM1_SDA, AM_HAL_GPIO_DISABLE);	//9
	am_hal_gpio_pin_config(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_DISABLE);	//26	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_PN80T_DNLD, AM_HAL_GPIO_DISABLE);	//17
	//am_hal_gpio_pin_config(23, AM_HAL_GPIO_DISABLE);	//23
	am_hal_gpio_pin_config(24, AM_HAL_GPIO_DISABLE);	//24

	//FLASH & GS
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_G_CS, AM_HAL_GPIO_OUTPUT);	//10
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_G_CS);	
	am_hal_gpio_pin_config(AM_BSP_GPIO_GSENSOR_INT, AM_HAL_GPIO_DISABLE);	//18

	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_HAL_GPIO_OUTPUT);	//11
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_FL_CS);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_DISABLE);	//6
	am_hal_gpio_pin_config(AM_BSP_GPIO_CFG_IOM0_MOSI, AM_HAL_GPIO_OUTPUT);	//7
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CFG_IOM0_MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_SCK, AM_HAL_GPIO_OUTPUT);	//5
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_SCK);

	//LCD
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_LCD_ON, AM_HAL_GPIO_OUTPUT);	//3
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_LCD_ON);
		am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_EN, AM_HAL_GPIO_OUTPUT);	//29
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_EN);
	
	//am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_OUTPUT);		////20180418 
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_CS);
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_DISABLE);    //35   //20180418 	
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_TE, AM_HAL_GPIO_DISABLE);	//36
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_DCX, AM_HAL_GPIO_OUTPUT);	//38
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_DCX);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_GPIO_DISABLE);
	//am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_GPIO_OUTPUT);	//37
	//am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_RES);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_SCK, AM_HAL_GPIO_OUTPUT);		//39
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM4_SCK);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_MOSI, AM_HAL_GPIO_OUTPUT);		//44
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM4_MOSI);
	
	//TP
	//
	cy8c4011_gpio_init();
	cy8c4011_gpio_uninit();
	
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_TP_INT, AM_HAL_GPIO_DISABLE);		//41
	//am_hal_gpio_pin_config(41, AM_HAL_GPIO_INPUT|AM_HAL_GPIO_PULL24K);		  //20180418
	//am_hal_gpio_out_bit_clear(41);

	//am_hal_gpio_pin_config(42, AM_HAL_GPIO_OUTPUT);		       
	//am_hal_gpio_out_bit_clear(42);
	//am_hal_gpio_pin_config(43, AM_HAL_GPIO_OUTPUT);		
	//am_hal_gpio_out_bit_clear(43);
	am_hal_gpio_pin_config(42, AM_HAL_GPIO_DISABLE);		 // 42    //20180418 
	am_hal_gpio_pin_config(43, AM_HAL_GPIO_DISABLE);		// 43   20180418 

	
	//MISC
	am_hal_gpio_pin_config(AM_BSP_GPIO_NTC_EN, AM_HAL_GPIO_OUTPUT);		//14
	
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NTC_EN);
		am_hal_gpio_pin_config(15, AM_HAL_GPIO_DISABLE);	//15
	//am_hal_gpio_out_bit_set(AM_BSP_GPIO_OFF_POWER_CTRL);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_NTC_ADC_IN, AM_HAL_GPIO_DISABLE);	//13
	

	
	am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_MOTOR, AM_HAL_GPIO_OUTPUT);	//19
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_MOTOR);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_ITM_SWO, AM_HAL_GPIO_OUTPUT);	//22
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_ITM_SWO);
	
//	am_hal_gpio_pin_config(AM_BSP_GPIO_PWM_LED, AM_HAL_GPIO_OUTPUT);	//23
//	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_PWM_LED);
	

	
	am_hal_gpio_pin_config(AM_BSP_GPIO_NFC_SW, AM_HAL_GPIO_OUTPUT);	//30
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_NFC_SW);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CLKOUT_PIN, AM_HAL_GPIO_DISABLE);	//4
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_STA_DET, AM_HAL_GPIO_DISABLE);	//31
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_STA_DET);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_DISABLE);	//32
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_DC_IN);
	
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_LEV_DET, AM_HAL_GPIO_DISABLE);	//33
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_CHG_LEV_DET);

	am_hal_gpio_pin_config(AM_BSP_GPIO_AUTO_RESET, AM_HAL_GPIO_DISABLE);	//34
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_AUTO_RESET);

#endif
	
	
}

/**
 * @brief   API to get_reset_stat
 *
 * @param   None
 *
 * @return  STAT Register Bits
 */
int ry_get_reset_stat(void)
{
	return reset_state;
}

/**
 * @brief   API to init LOG module
 *
 * @param   None
 *
 * @return  None
 */
void ry_board_init(void)
{
	reset_state = am_hal_reset_status_get();
#if ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
    /* disable power off first */
    power_off_ctrl(0);

    /* output 32k to makesure HW_WDG not make reset */
    //auto_reset_start();
#endif

    /* Delay 100ms to make sure the 32K crystal working */
    am_util_delay_ms(100);
   
    /* Set the clock frequency. */
    am_hal_clkgen_sysclk_select(AM_HAL_CLKGEN_SYSCLK_MAX);

    /* Initialize peripherals as specified in the BSP. */
    am_bsp_low_power_init();

	ry_hal_rtc_init();
	ry_hal_rtc_start();
	
    /* Work around for MCU bug */
    am_hal_sysctrl_buck_ctimer_isr_init(3);
	  
	/* set all GPIO to default state */
    ry_board_default_pins();
    //sake_default_pins();


#if (STATIC_POWER_TEST_ENABLE == TRUE)
	ry_hal_wdt_stop();
	cy8c4011_gpio_init();
	uint32_t tp_id = touch_get_id();
	touch_enter_low_power();
    // Set flash enter power down
    
    while(1) {
        am_freertos_sleep(0);
    }
#endif
	
    board_control_init();
}

/**
 * @brief   API to init LOG module
 *
 * @param   None
 *
 * @return  None
 */
void ry_log_init(void)
{
    ryos_mutex_create(&log_mutex);

#if (RY_BUILD == RY_DEBUG)

    ry_hal_uart_init(UART_IDX_LOG, NULL);
    enable_print_interface();
    
    //
    // Initialize plotting interface.
    //
    #if USE_RTT
    SEGGER_RTT_WriteString(0, "Welcome Ryeex!\n");
    #else
    am_util_debug_printf("Welcome Ryeex!\n");
    #endif
#endif
}

void ry_system_reset(void)
{
    f_mount(0, "", 0);
	
    ryos_enter_critical();
    
    am_devices_rm67162_powerdown_with_block_delay();	
    power_oled_on(0);
    add_reset_count(USR_RESTART_COUNT);
    //am_util_delay_ms(1000);
    ry_delay_ms(500);    
    am_hal_reset_poi();
    
    ryos_exit_critical(0);
}


void ry_spi_timeout(void)
{
    add_reset_count(SPI_TIMEOUT_RESTART);
    power_oled_on(0);
    //am_util_delay_ms(500);
    ry_delay_ms(500);
    am_hal_reset_poi();
}


void ry_assert_log(void)
{
    extern ry_device_setting_t device_setting;
    device_setting.last_restart_type = ASSERT_RESTART;
    ry_hal_flash_write_unlock(DEVICE_SETTING_ADDR, (u8_t *)&device_setting, sizeof(device_setting));
    //am_util_delay_ms(500);
    ry_delay_ms(500);
    am_hal_reset_poi();
}

/**
 * @brief   board specified printf API
 *
 * @param   None
 *
 * @return  None
 */
void ry_board_debug_printf(const char* fmt, ...)
{
    if (g_flg_swo_busy)
        return;
    
    uint32_t length = 0;
#if 1
    
    //ryos_mutex_lock(log_mutex);
    va_list ap;
    va_start(ap, fmt);
    length = vsnprintf(g_log_buffer, RY_DEBUG_LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    //ry_hal_uart_send(UART_LOG, buffer, length);
    //am_bsp_uart_string_print(buffer);
#if USE_RTT
    SEGGER_RTT_Write(0, g_log_buffer, length);
#else
    am_util_debug_printf(g_log_buffer);
#endif
    //while ( am_hal_uart_flags_get(AM_BSP_UART_PRINT_INST) & AM_HAL_UART_FR_BUSY );
    // ryos_mutex_unlock(log_mutex);
#else
    printf(fmt);
#endif
}




/**
 * @brief   board specified printf API
 *
 * @param   None
 *
 * @return  None
 */
void ry_board_printf(const char* fmt, ...)
{
      if (g_flg_swo_busy)
        return;
        
    uint32_t length = 0;
    u32_t r = ry_hal_irq_disable();
#if 1
    ryos_mutex_lock(log_mutex);
    va_list ap;
    va_start(ap, fmt);
    length = vsnprintf(g_log_buffer, RY_DEBUG_LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    //ry_hal_uart_send(UART_LOG, buffer, length);
    //am_bsp_uart_string_print(buffer);
    
#if USE_RTT
        SEGGER_RTT_Write(0, g_log_buffer, length);
#else
        am_util_debug_printf(g_log_buffer);
#endif

    //while ( am_hal_uart_flags_get(AM_BSP_UART_PRINT_INST) & AM_HAL_UART_FR_BUSY );
    ryos_mutex_unlock(log_mutex);
    ry_hal_irq_restore(r);
#else
    printf(fmt);
#endif
}


/*@}*/
