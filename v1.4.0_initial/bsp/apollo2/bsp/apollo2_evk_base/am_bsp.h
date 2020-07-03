//*****************************************************************************
//
//! @file am_bsp.h
//!
//! @brief Functions to aid with configuring the GPIOs.
//!
//! @addtogroup BSP Board Support Package (BSP)
//! @addtogroup apollo2_evk_base_bsp BSP for the Apollo2 EVK base board
//! @ingroup BSP
//! @{
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
// This is part of revision 1.2.6 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#ifndef AM_BSP_H
#define AM_BSP_H

#include <stdint.h>
#include <stdbool.h>
#include "am_mcu_apollo.h"
#include "am_devices.h"
#include "am_bsp_gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// PWM_LED peripheral assignments.
//
//*****************************************************************************
#define AM_BSP_PWM_LED_TIMER                0
#define AM_BSP_PWM_LED_TIMER_SEG            AM_HAL_CTIMER_TIMERB
#define AM_BSP_PWM_LED_TIMER_INT            AM_HAL_CTIMER_INT_TIMERB0

//*****************************************************************************
//
// Button definitions.
//
//*****************************************************************************
#define AM_BSP_NUM_BUTTONS                  3
extern am_devices_button_t am_bsp_psButtons[AM_BSP_NUM_BUTTONS];

//*****************************************************************************
//
// LED definitions.
//
//*****************************************************************************
#define AM_BSP_NUM_LEDS                     4
extern am_devices_led_t am_bsp_psLEDs[AM_BSP_NUM_LEDS];

//*****************************************************************************
//
// UART definitions.
//
//*****************************************************************************
//
// Apollo2 has two UART instances.
//
#define AM_BSP_UART_IOS_INST                0
#define AM_BSP_UART_PRINT_INST              0//1
#define AM_BSP_UART_BOOTLOADER_INST         1

#define AM_BSP_UART_DBG_INST                0
#define AM_BSP_UART_BLE_INST                1

//*****************************************************************************
//
// Touch CP3155 peripheral assignments.
//
//*****************************************************************************
#define AM_BSP_TOUCH_IOM                    2

//*****************************************************************************
//
// NFC/SE PN80T peripheral assignments.
//
//*****************************************************************************
#define AM_BSP_SE_IOM                       1

//*****************************************************************************
//
// Fingerprint peripheral assignments.
//
//*****************************************************************************
#define AM_BSP_FP_IOM                       5
#define AM_BSP_FP_CS                        1


//*****************************************************************************
//
// GSensor peripheral assignments.
//
//*****************************************************************************
#define AM_BSP_GSENSOR_IOM                  4
#define AM_BSP_GSENSOR_CS                   0

//*****************************************************************************
//
// GSensor peripheral assignments.
//
//*****************************************************************************
#define AM_BSP_SCREEN_IOM                   3
#define AM_BSP_SCREEN_CS                    3

//*****************************************************************************
//
// Structure for containing information about the UART configuration while
// it is powered down.
//
//*****************************************************************************
typedef struct
{
    bool     bSaved;
    uint32_t ui32TxPinNum;
    uint32_t ui32TxPinCfg;
}
am_bsp_uart_pwrsave_t;

//*****************************************************************************
//
// Structure for containing information about the IOM configuration while
// it is powered down.
// Each IOM can select up to 8 SPI devices.
//
//*****************************************************************************
typedef struct
{
    bool     bSaved[8];
    uint32_t ui32CsPinNum[8];
    uint32_t ui32CsPinCfg[8];
}
am_bsp_iom_pwrsave_t;

//*****************************************************************************
//
// External data definitions.
//
//*****************************************************************************
extern am_bsp_uart_pwrsave_t am_bsp_uart_pwrsave[AM_REG_UART_NUM_MODULES];
extern am_bsp_iom_pwrsave_t  am_bsp_iom_pwrsave[AM_REG_IOMSTR_NUM_MODULES];

//*****************************************************************************
//
// External function definitions.
//
//*****************************************************************************
extern void am_bsp_iom_spi_pins_enable(uint32_t ui32Module);
extern void am_bsp_iom_spi_pins_disable(uint32_t ui32Module);
extern void am_bsp_iom_i2c_pins_enable(uint32_t ui32Module);
extern void am_bsp_iom_i2c_pins_disable(uint32_t ui32Module);
extern void am_bsp_low_power_init(void);
extern void am_bsp_uart_power_on_restore(uint32_t ui32Module);
extern void am_bsp_uart_power_off_save(uint32_t ui32Module);
extern void am_bsp_iom_power_on_restore(uint32_t ui32Module);
extern void am_bsp_iom_power_off_save(uint32_t ui32Module);
extern void am_bsp_debug_printf_enable(void);
extern void am_bsp_debug_printf_disable(void);
extern void am_bsp_itm_string_print(char *pcString);
extern void am_bsp_uart_string_print(char *pcString);

#ifdef __cplusplus
}
#endif

#endif // AM_BSP_H
//*****************************************************************************
//
// End Doxygen group.
//! @}
//
//*****************************************************************************
