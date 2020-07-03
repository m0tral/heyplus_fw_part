//*****************************************************************************
//
//! @file am_bsp_gpio.h
//!
//! @brief Functions to aid with configuring the GPIOs.
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2017, Ambiq Micro
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision 1.2.8 of the AmbiqSuite Development Package.
//
//*****************************************************************************
#ifndef AM_BSP_GPIO_H
#define AM_BSP_GPIO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"

//*****************************************************************************
//
// Miscellaneous pins.
//
//*****************************************************************************

//*****************************************************************************
#if (HW_VER_CURRENT == HW_VER_SAKE_03)
#define AM_BSP_GPIO_OFF_POWER_CTRL      23	
#define AM_BSP_GPIO_CFG_OFF_POWER_CTRL  AM_HAL_PIN_23_GPIO
#else
#define AM_BSP_GPIO_OFF_POWER_CTRL      15	
#define AM_BSP_GPIO_CFG_OFF_POWER_CTRL  AM_HAL_PIN_15_GPIO
#endif

#if (HW_VER_CURRENT == HW_VER_SAKE_01)
#define AM_BSP_GPIO_5V_ON               16	
#define AM_BSP_GPIO_CFG_5V_ON           AM_HAL_PIN_OUTPUT
#endif

#define AM_BSP_GPIO_PWM_MOTOR           19	
#define AM_BSP_GPIO_CFG_PWM_MOTOR       AM_HAL_PIN_19_TCTB1	

#if ((HW_VER_CURRENT == HW_VER_SAKE_01) || (HW_VER_CURRENT == HW_VER_SAKE_03))
#define AM_BSP_GPIO_PWM_LED             22	
#define AM_BSP_GPIO_CFG_PWM_LED         AM_HAL_PIN_22_TCTB1	
#elif (HW_VER_CURRENT == HW_VER_SAKE_02)
#define AM_BSP_GPIO_PWM_LED             23
#define AM_BSP_GPIO_CFG_PWM_LED         AM_HAL_PIN_23_TCTB1	
#endif

#define AM_BSP_GPIO_LCD_ON              3
#define AM_BSP_GPIO_CFG_LCD_ON		    AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_BLE_EN           	40
#define AM_BSP_GPIO_CFG_BLE_EN		    AM_HAL_PIN_OUTPUT


#define AM_BSP_GPIO_NFC_1V8_SW              2
#define AM_BSP_GPIO_CFG_NFC_1V8_SW		    AM_HAL_PIN_OUTPUT
	
//*****************************************************************************
//
// Battery pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_CHG_EN              29	
#define AM_BSP_GPIO_CFG_CHG_END         AM_HAL_PIN_29_GPIO	

#if (HW_VER_CURRENT == HW_VER_SAKE_01)
#define AM_BSP_GPIO_BAT_DET_SW          30	
#define AM_BSP_GPIO_CFG_BAT_DET_SW      AM_HAL_PIN_30_GPIO	
#endif

#define AM_BSP_GPIO_CHG_STA_DET         31	
#define AM_BSP_GPIO_CFG_CHG_STA_DET     AM_HAL_PIN_31_GPIO	

#define AM_BSP_GPIO_DC_IN               32	
#define AM_BSP_GPIO_CFG_DC_IN           AM_HAL_PIN_32_GPIO

#if (HW_VER_CURRENT == HW_VER_SAKE_01)

#define AM_BSP_GPIO_CHG_LEV_DET         33	
#define AM_BSP_GPIO_CFG_CHG_LEV_DET     AM_HAL_PIN_33_ADCSE5

#define AM_BSP_GPIO_AUTO_RESET          34	
#define AM_BSP_GPIO_CFG_AUTO_RESET      AM_HAL_PIN_34_GPIO

#elif ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))

#define AM_BSP_GPIO_CHG_LEV_DET         34	
#define AM_BSP_GPIO_CFG_CHG_LEV_DET     AM_HAL_PIN_34_ADCSE6

#define AM_BSP_GPIO_AUTO_RESET          33	
#define AM_BSP_GPIO_CFG_AUTO_RESET      AM_HAL_PIN_33_32KHZ_XT//AM_HAL_PIN_33_TCTB1 //(AM_HAL_PIN_33_32KHZ_XT)

#endif /* HW_VER_CURRENT */

#define AM_BSP_GPIO_GSENSOR_INT         18
#define AM_BSP_GPIO_CFG_GSENSOR_INT     AM_HAL_PIN_INPUT


#if (HW_VER_CURRENT == HW_VER_SAKE_01)

//#define AM_BSP_GPIO_LED0                46
#define AM_BSP_GPIO_5V_ON               16
#endif

//*****************************************************************************
//
// CLKOUT pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_CLKOUT_PIN          4//7
#define AM_BSP_GPIO_CFG_CLKOUT_PIN      AM_HAL_PIN_4_CLKOUT//AM_HAL_PIN_7_CLKOUT

//*****************************************************************************
//
// COM_UART pins.
//
//*****************************************************************************
//#define AM_BSP_GPIO_COM_UART_CTS        37
//#define AM_BSP_GPIO_CFG_COM_UART_CTS    AM_HAL_PIN_37_UART0RTS
//#define AM_BSP_GPIO_COM_UART_RTS        38
//#define AM_BSP_GPIO_CFG_COM_UART_RTS    AM_HAL_PIN_38_UART0CTS
//#define AM_BSP_GPIO_COM_UART_RX         23
//#define AM_BSP_GPIO_CFG_COM_UART_RX     AM_HAL_PIN_23_UART0RX
//#define AM_BSP_GPIO_COM_UART_TX         22
//#define AM_BSP_GPIO_CFG_COM_UART_TX     AM_HAL_PIN_22_UART0TX

// LOG Uart
//#define AM_BSP_GPIO_DBG_UART_RX         11//23
//#define AM_BSP_GPIO_CFG_DBG_UART_RX     AM_HAL_PIN_11_UART0RX//AM_HAL_PIN_23_UART0RX
//#define AM_BSP_GPIO_DBG_UART_TX         1//16
//#define AM_BSP_GPIO_CFG_DBG_UART_TX     AM_HAL_PIN_1_UART0TX//AM_HAL_PIN_16_UART0TX

// BLE GUP Uart
#if (HW_VER_CURRENT == HW_VER_SAKE_01)

#define AM_BSP_GPIO_TOOL_UART_RX        13
#define AM_BSP_GPIO_CFG_TOOL_UART_RX    AM_HAL_PIN_13_UART1RX
#define AM_BSP_GPIO_TOOL_UART_TX        14
#define AM_BSP_GPIO_CFG_TOOL_UART_TX    AM_HAL_PIN_14_UART1TX

# elif ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))

#define AM_BSP_GPIO_TOOL_UART_RX        15
#define AM_BSP_GPIO_CFG_TOOL_UART_RX    AM_HAL_PIN_15_UART1RX
#define AM_BSP_GPIO_TOOL_UART_TX        14
#define AM_BSP_GPIO_CFG_TOOL_UART_TX    AM_HAL_PIN_14_UART1TX

#endif


#define HRM_POWER_EN_PIN                12
#define HRM_RESETZ_CTRL_PIN             27
#define PPS_INTERRUPT_PIN               28

// #define AM_BSP_GPIO_CFG_TOOL_UART_TX    AM_HAL_PIN_12_UART1TX
#define AM_BSP_GPIO_TP_INT              41
#define AM_BSP_GPIO_CFG_TP_INT          AM_HAL_PIN_INPUT

//*****************************************************************************
//
// RM67162 pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_RM67162_TE          36//13
#define AM_BSP_GPIO_CFG_RM67162_TE      AM_HAL_PIN_36_GPIO   

#define AM_BSP_GPIO_RM67162_DCX         38//42//38
#define AM_BSP_GPIO_CFG_RM67162_DCX     AM_HAL_PIN_38_GPIO 

#define AM_BSP_GPIO_RM67162_RES         37//43//37
#define AM_BSP_GPIO_CFG_RM67162_RES     AM_HAL_PIN_37_GPIO 

#define AM_BSP_GPIO_RM67162_CS          35//11//35//13
#define AM_BSP_GPIO_CFG_RM67162_CS      AM_HAL_PIN_35_M4nCE6//AM_HAL_PIN_11_M0nCE0 //    AM_HAL_PIN_35_M4nCE6//AM_HAL_PIN_35_GPIO

//*****************************************************************************
//
// EM9304 pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_EM9304_CS           46//45
#define AM_BSP_GPIO_CFG_EM9304_CS       AM_HAL_PIN_OUTPUT
#define AM_BSP_GPIO_EM9304_INT          45//26
#define AM_BSP_GPIO_CFG_EM9304_INT      AM_HAL_PIN_INPUT
#define AM_BSP_GPIO_EM9304_RESET        40//42
#define AM_BSP_GPIO_CFG_EM9304_RESET    AM_HAL_PIN_OUTPUT
#define AM_BSP_GPIO_EM9304_TM           30
#define AM_BSP_GPIO_CFG_EM9304_TM       AM_HAL_PIN_OUTPUT


//*****************************************************************************
//
// pps1262 pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_HRM_EN              12
#define AM_BSP_GPIO_CFG_HRM_EN          AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_HRM_RESET           27
#define AM_BSP_GPIO_CFG_HRM_RESET       AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_HRM_IRQ             28
#define AM_BSP_GPIO_CFG_HRM_IRQ         AM_HAL_PIN_OUTPUT



//*****************************************************************************
//
// PN80T pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_PN80T_VEN           25
#define AM_BSP_GPIO_CFG_PN80T_VEN       AM_HAL_PIN_OUTPUT
#define AM_BSP_GPIO_PN80T_DNLD          17//24
#define AM_BSP_GPIO_CFG_PN80T_DNLD      AM_HAL_PIN_OUTPUT
#define AM_BSP_GPIO_PN80T_INT           26//27
#define AM_BSP_GPIO_CFG_PN80T_INT       AM_HAL_PIN_INPUT
#define AM_BSP_GPIO_NFC_18V_EN          2//28
#define AM_BSP_GPIO_CFG_NFC_18V_EN      AM_HAL_PIN_OUTPUT

#if (HW_VER_CURRENT == HW_VER_SAKE_01)
#define AM_BSP_GPIO_NFC_5V_EN           16//29
#define AM_BSP_GPIO_CFG_NFC_5V_EN       AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_NFC_BAT_EN          3//30
#define AM_BSP_GPIO_CFG_NFC_BAT_EN      AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_SYS_5V_EN           15
#define AM_BSP_GPIO_CFG_SYS_5V_EN       AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_NFC_ESE_PWR_REQ     23
#define AM_BSP_GPIO_CFG_NFC_ESE_PWR_REQ AM_HAL_PIN_OUTPUT

#endif /* HW_VER_SAKE_01 */


#define AM_BSP_GPIO_NFC_WAKEUP_REQ      24
#define AM_BSP_GPIO_CFG_NFC_WAKEUP_REQ  AM_HAL_PIN_INPUT




#if ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
#define AM_BSP_GPIO_NFC_SW              30
#define AM_BSP_GPIO_CFG_NFC_SW          AM_HAL_PIN_OUTPUT

#define AM_BSP_GPIO_NFC_ESE_PWR_REQ     16 // not used now
#define AM_BSP_GPIO_CFG_NFC_ESE_PWR_REQ AM_HAL_PIN_OUTPUT

#endif /* HW_VER_SAKE_02 */


//*****************************************************************************
//
// IOM0 pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_IOM0_G_CS             10
#define AM_BSP_GPIO_CFG_IOM0_G_CS         AM_HAL_PIN_10_M0nCE6   
#define AM_BSP_GPIO_IOM0_FL_CS             11
#define AM_BSP_GPIO_CFG_IOM0_FL_CS         AM_HAL_PIN_11_M0nCE0 
#define AM_BSP_GPIO_IOM0_MISO           6
#define AM_BSP_GPIO_CFG_IOM0_MISO       AM_HAL_PIN_6_M0MISO
#define AM_BSP_GPIO_IOM0_MOSI           7
#define AM_BSP_GPIO_CFG_IOM0_MOSI       AM_HAL_PIN_7_M0MOSI
#define AM_BSP_GPIO_IOM0_SCK            5
#define AM_BSP_GPIO_CFG_IOM0_SCK        (AM_HAL_PIN_5_M0SCK | AM_HAL_GPIO_INPEN | AM_HAL_GPIO_HIGH_DRIVE | AM_HAL_GPIO_24MHZ_ENABLE)
#define AM_BSP_GPIO_IOM0_SCL            5
#define AM_BSP_GPIO_CFG_IOM0_SCL        AM_HAL_PIN_5_M0SCL
#define AM_BSP_GPIO_IOM0_SDA            6
#define AM_BSP_GPIO_CFG_IOM0_SDA        AM_HAL_PIN_6_M0SDA


//*****************************************************************************
//
// IOM1 pins. (nfc)
//
//*****************************************************************************
//#define AM_BSP_GPIO_IOM1_MISO           9
//#define AM_BSP_GPIO_CFG_IOM1_MISO       AM_HAL_PIN_9_M1MISO
//#define AM_BSP_GPIO_IOM1_MOSI           12
//#define AM_BSP_GPIO_CFG_IOM1_MOSI       AM_HAL_PIN_10_M1MOSI
//#define AM_BSP_GPIO_IOM1_SCK            8
//#define AM_BSP_GPIO_CFG_IOM1_SCK        AM_HAL_PIN_8_M1SCK
#define AM_BSP_GPIO_IOM1_SCL              8
#define AM_BSP_GPIO_CFG_IOM1_SCL          (AM_HAL_PIN_8_M1SCL | AM_HAL_GPIO_PULL6K)// | AM_HAL_GPIO_HIGH_DRIVE)
#define AM_BSP_GPIO_IOM1_SDA              9
#define AM_BSP_GPIO_CFG_IOM1_SDA          (AM_HAL_PIN_9_M1SDA | AM_HAL_GPIO_PULL6K)

//*****************************************************************************
//
// IOM2 pins.(PPS1262)
//
//*****************************************************************************
//#define AM_BSP_GPIO_IOM2_MISO           1
//#define AM_BSP_GPIO_CFG_IOM2_MISO       AM_HAL_PIN_1_M2MISO
//#define AM_BSP_GPIO_IOM2_MOSI           6
//#define AM_BSP_GPIO_CFG_IOM2_MOSI       AM_HAL_PIN_2_M2MOSI
//#define AM_BSP_GPIO_IOM2_SCK            0
//#define AM_BSP_GPIO_CFG_IOM2_SCK        AM_HAL_PIN_0_M2SCK
#define AM_BSP_GPIO_IOM2_SCL            0
#define AM_BSP_GPIO_CFG_IOM2_SCL        (AM_HAL_PIN_0_M2SCL | AM_HAL_GPIO_PULL6K)// | AM_HAL_GPIO_HIGH_DRIVE)
#define AM_BSP_GPIO_IOM2_SDA            1
#define AM_BSP_GPIO_CFG_IOM2_SDA        (AM_HAL_PIN_1_M2SDA | AM_HAL_GPIO_PULL6K)


//*****************************************************************************
//
// IOM4 pins. (OLED)
//
//*****************************************************************************
#define AM_BSP_GPIO_IOM4_CS             35
#define AM_BSP_GPIO_CFG_IOM4_CS         AM_HAL_PIN_35_M4nCE6
#define AM_BSP_GPIO_IOM4_MOSI           44
#define AM_BSP_GPIO_CFG_IOM4_MOSI       AM_HAL_PIN_44_M4MOSI
#define AM_BSP_GPIO_IOM4_SCK            39
#define AM_BSP_GPIO_CFG_IOM4_SCK        (AM_HAL_PIN_39_M4SCK | AM_HAL_GPIO_INPEN | AM_HAL_GPIO_HIGH_DRIVE | AM_HAL_GPIO_24MHZ_ENABLE)


//*****************************************************************************
//
// IOM5 pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_IOM5_MISO           49
#define AM_BSP_GPIO_CFG_IOM5_MISO       AM_HAL_PIN_49_M5MISO
#define AM_BSP_GPIO_IOM5_MOSI           47
#define AM_BSP_GPIO_CFG_IOM5_MOSI       AM_HAL_PIN_47_M5MOSI
#define AM_BSP_GPIO_IOM5_SCK            48
#define AM_BSP_GPIO_CFG_IOM5_SCK        AM_HAL_PIN_48_M5SCK

//*****************************************************************************
//
// ITM pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_ITM_SWO             22
#define AM_BSP_GPIO_CFG_ITM_SWO         AM_HAL_PIN_22_SWO


//*****************************************************************************
//
// NTC pins.
//
//*****************************************************************************
#define AM_BSP_GPIO_NTC_ADC_IN            	13
#define AM_BSP_GPIO_CFG_NTC_ADC_IN         AM_HAL_PIN_13_ADCD0PSE8
#define AM_BSP_GPIO_NTC_EN          		14
#define AM_BSP_GPIO_CFG_NTC_EN         		AM_HAL_PIN_14_GPIO

//*****************************************************************************
//
// Convenience macros for enabling and disabling pins by function.
//
//*****************************************************************************
#define am_bsp_pin_enable(name)                                               \
    am_hal_gpio_pin_config(AM_BSP_GPIO_ ## name, AM_BSP_GPIO_CFG_ ## name);

#define am_bsp_pin_disable(name)                                              \
    am_hal_gpio_pin_config(AM_BSP_GPIO_ ## name, AM_HAL_PIN_DISABLE);

#ifdef __cplusplus
}
#endif

#endif // AM_BSP_GPIO_H
