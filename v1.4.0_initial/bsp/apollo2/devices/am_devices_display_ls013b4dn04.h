//*****************************************************************************
//
//! @file am_devices_display_ls013b4dn04.h
//!
//! @brief Driver for handling low-level graphics and display operations for
//!        the AdaFruit Sharp LS013B4DN04 Memory LCD.
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
// This is part of revision v1.2.10-2-gea660ad-hotfix2 of the AmbiqSuite Development Package.
//
//*****************************************************************************

#ifndef AM_DEVICES_DISPLAY_LS013B4DN04_H
#define AM_DEVICES_DISPLAY_LS013B4DN04_H

#include "am_mcu_apollo.h"

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// C99
//
//*****************************************************************************
#include <stdint.h>
#include <stdbool.h>

//*****************************************************************************
//
// Specific display macro definitions
//
//*****************************************************************************
//
// Define the physical size of the screen (inches)
//
#define AM_DISPLAY_SHARP_LS013B4DN04_SCREEN_SIZE    1.35F

//
// Define the screen dimensions for the Sharp LS013B4DN04 memory display.
//
#define AM_DISPLAY_LS013B4DN04_WIDTH        96
#define AM_DISPLAY_LS013B4DN04_HEIGHT       96
#define AM_DISPLAY_LS013B4DN04_POLARITY     1

//
// Define SHARP Memory LCD commands
//
#define SHARP_LS013B4DN04_CMD_WRLN          0x80    // Write line command
#define SHARP_LS013B4DN04_CMD_CLRMEM        0x20    // Clear memory command
#define SHARP_LS013B4DN04_CMD_NOP           0x00    // NOP command (VCOM)

//
// The above commands with LSB reversed, which work well with the Apollo IOM.
//
#define SHARP_LS013B4DN04_CMD_WRLN_LSB      0x01    // Write line command
#define SHARP_LS013B4DN04_CMD_CLRMEM_LSB    0x04    // Clear memory command
#define SHARP_LS013B4DN04_CMD_NOP_LSB       0x00    // NOP command (VCOM)

//
// Define the VCOM bit in the command word
//
#define SHARP_LS013B4DN04_VCOM_HI           0x40
#define SHARP_LS013B4DN04_VCOM_LO           0x00
#define SHARP_LS013B4DN04_VCOM_TOGGLE       0x40

#define SHARP_LS013B4DN04_VCOM_HI_LSB       0x02
#define SHARP_LS013B4DN04_VCOM_LO_LSB       0x00

//
// Define the value written to the framebuffer to clear the screen.
//
#define SHARP_LS013B4DN04_FB_CLEAR_VALUE    0xFF

//*****************************************************************************
//
// Device data structure used for display devices.
//
//*****************************************************************************
typedef struct
{
    //*************************************************************************
    // Parameters supplied by application.
    //*************************************************************************

    //
    // Framebuffer pointer.
    // Note - framebuffer must be guaranteed to be 32-bit aligned.
    //
    uint8_t *pui8Framebuffer;

    //
    // IOM SPI information.
    // The order of the following 6 members are critical.  Do Not Change!
    // The first is the SPI chip select pin number.
    // The next 5 members will also be used for calls to am_hal_iom_spi_write()
    // and are named exactly the same as the HAl function parameters.  The order
    // is critical and must be maintained as is, and all must be 32-bit aligned.
    // ui32Module and ui32ChipSelect must be filled in by the application.
    //
    uint32_t    ui32ChipSelectPin;  // AM_BSP_GPIO_IOM1_DISPLAY_CS (app)
    uint32_t    ui32Module;         // AM_BSP_IOM_DISPLAY_INST (app supplied)
    uint32_t    ui32ChipSelect;     // AM_BSP_IOM_DISPLAY_NCE  (app supplied)
    uint32_t    *pui32Data;         // Pointer to the data     (driver supplied)
    uint32_t    ui32NumBytes;       // Number of bytes to xfer (driver supplied)
    uint32_t    ui32Options;        // e.g. AM_HAL_IOM_RAW     (driver supplied)

    //*************************************************************************
    // Parameters supplied by the display driver after call to
    // am_devices_display_ls013b4dn04_init().
    //*************************************************************************
    //
    // Display width (X dimension).
    //
    uint32_t ui32DisplayWidth;

    //
    // Display height (Y dimension).
    //
    uint32_t ui32DisplayHeight;

    //
    // Display background polarity.  The frame buffer handler will use this to
    //  determine the polarity used in the frame buffer.
    //  Set to 1 if the background clears with 1s (0xffs).
    //  Set to 0 if the background clears with 0s (0x00s).
    //
    uint32_t ui32DisplayBkgndPolarity;

    //
    // Hardware refresh period (in ms).
    // This is the required number of milliseconds between calls
    // to am_devices_display_ls013b4dn04_hw_refresh().
    //
    uint32_t ui32HwRefreshMs;
}
am_devices_display_ls013b4dn04_t;

//*****************************************************************************
//
// External variable definitions
//
//*****************************************************************************

//*****************************************************************************
//
// External function definitions
//
//*****************************************************************************
extern uint32_t am_devices_display_ls013b4dn04_init(
                        am_devices_display_ls013b4dn04_t *psDisplayContext);
extern void am_devices_display_ls013b4dn04_callbacks_config(
                                              am_hal_iom_callback_t pfnYield,
                                              am_hal_iom_callback_t pfnCallback);
extern void am_devices_display_ls013b4dn04_screen_clear(
                            am_devices_display_ls013b4dn04_t *psDisplayContext);
extern bool am_devices_display_ls013b4dn04_hw_refresh(
                            am_devices_display_ls013b4dn04_t *psDisplayContext,
                            bool bFullScreenRefresh);
extern void  am_devices_display_ls013b4dn04_lines_copy(
                            am_devices_display_ls013b4dn04_t *psDisplayContext,
                            uint32_t u32BegLineNum,
                            uint32_t u32EndLineNum);

#ifdef __cplusplus
}
#endif

#endif // AM_DEVICES_DISPLAY_LS013B4DN04_H

