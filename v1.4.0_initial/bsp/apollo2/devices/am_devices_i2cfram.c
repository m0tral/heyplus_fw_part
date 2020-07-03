//*****************************************************************************
//
//! @file am_devices_spifram.c
//!
//! @brief Generic spifram driver.
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

#include <string.h>
#include "am_mcu_apollo.h"
#include "am_devices_i2cfram.h"

//! Maximum supported by IOM device in one transfer.
#define MAX_I2C_TRANSFERSIZE (255)
// Support max 4 queued transactions
#define MAX_I2C_QUEUED_WRITES   4

//*****************************************************************************
//
// Global variables.
//
//*****************************************************************************
am_devices_i2cfram_t g_sI2cIOMSettings;

//! Write buffer when single write must be used. Otherwise use caller's buffer.
am_hal_iom_buffer(MAX_I2C_TRANSFERSIZE) g_i2cXferBuffer[MAX_I2C_QUEUED_WRITES];
uint32_t                            g_i2cWrBufIdx = 0;
am_hal_iom_buffer(4)                g_i2cTempBuffer[MAX_I2C_QUEUED_WRITES];
uint32_t                            g_i2cTempBufIdx = 0;

am_hal_iom_buffer(4)         g_i2cCmdResponse;

//*****************************************************************************
//
//! @brief Initialize the i2cfram driver.
//!
//! @param psIOMSettings - IOM device structure describing the target spifram.
//!
//! This function should be called before any other am_devices_i2cfram
//! functions. It is used to set tell the other functions how to communicate
//! with the external i2cfram hardware.
//!
//! @return None.
//
//*****************************************************************************
void
am_devices_i2cfram_init(am_devices_i2cfram_t *psIOMSettings)
{
    //
    // Initialize the IOM settings from the application.
    //
    g_sI2cIOMSettings.ui32IOMModule = psIOMSettings->ui32IOMModule;
    g_sI2cIOMSettings.ui32BusAddress = psIOMSettings->ui32BusAddress;

}

uint32_t
am_devices_i2cfram_id(uint16_t *manufacturerID, uint16_t *productID)
{
    //
    // Set the CMD and copy the data into the same buffer.
    //
    g_i2cTempBuffer[g_i2cTempBufIdx].bytes[0] = g_sI2cIOMSettings.ui32BusAddress << 1;

    //
    // Send a command to read the ID register in the external fram.
    am_hal_iom_i2c_write(g_sI2cIOMSettings.ui32IOMModule,
                        0xF8 >> 1,
                        g_i2cTempBuffer[g_i2cTempBufIdx].words, 1,
                        AM_HAL_IOM_NO_STOP | AM_HAL_IOM_RAW);
    g_i2cCmdResponse.words[0] = 0;
    am_hal_iom_i2c_read(g_sI2cIOMSettings.ui32IOMModule,
                        0xF9 >> 1,
                        g_i2cCmdResponse.words, 3,
                        AM_HAL_IOM_RAW);

    //
    // Return the ID
    //
#if 0
    *manufacturerID = ((g_i2cCmdResponse.words[0] & 0x000000FF) << 4) +
                      ((g_i2cCmdResponse.words[0] & 0x0000F000) >> 12);
    *productID = ((g_i2cCmdResponse.words[0] & 0x00FF0000 ) >> 16) +
                 ((g_i2cCmdResponse.words[0] & 0x00000F00 ) );
#else
    *manufacturerID = ((g_i2cCmdResponse.bytes[0]) << 4) +
                      ((g_i2cCmdResponse.bytes[1] & 0xF0) >> 4);
    *productID = (g_i2cCmdResponse.bytes[2]) +
                 ((g_i2cCmdResponse.bytes[1] & 0xF) << 8);
#endif
    return g_i2cCmdResponse.words[0];
}

//*****************************************************************************
//
//! @brief Reads the contents of the external fram into a buffer.
//!
//! @param ui32DeviceNumber - Device number of the external fram
//! @param pui8RxBuffer - Buffer to store the received data from the fram
//! @param ui32ReadAddress - Address of desired data in external fram
//! @param ui32NumBytes - Number of bytes to read from external fram
//!
//! This function reads the external fram at the provided address and stores
//! the received data into the provided buffer location. This function will
//! only store ui32NumBytes worth of data.
//
//*****************************************************************************
void
am_devices_i2cfram_read(uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress,
                        uint32_t ui32NumBytes)
{
    uint32_t ui32BytesRemaining, ui32CurrentReadAddress;
    uint8_t *pui8Dest;
    //
    // Set the total number of bytes,and the starting transfer destination.
    //
    ui32BytesRemaining = ui32NumBytes;
    pui8Dest = pui8RxBuffer;
    ui32CurrentReadAddress = ui32ReadAddress;

    while ( ui32BytesRemaining > 0)
    {
        //
        // Set the transfer size to either TRANSFERSIZE,
        // or the number of remaining bytes, whichever is smaller.
        //
        uint32_t ui32TransferSize =
            (ui32BytesRemaining > MAX_I2C_TRANSFERSIZE) ? MAX_I2C_TRANSFERSIZE: ui32BytesRemaining;

        //
        // READ is a 2-byte command (16-bit address)
        //
        g_i2cTempBuffer[g_i2cTempBufIdx].bytes[0] = (ui32CurrentReadAddress & 0x0000FF00) >> 8;
        g_i2cTempBuffer[g_i2cTempBufIdx].bytes[1] = (ui32CurrentReadAddress & 0x000000FF);

        //
        // Send Read Command to I2CFRAM, Hold CS Low, then SPI Read to recieve data.
        //
        am_hal_iom_spi_write(g_sI2cIOMSettings.ui32IOMModule,
                             g_sI2cIOMSettings.ui32BusAddress,
                             g_i2cTempBuffer[g_i2cTempBufIdx].words, 2,
                             AM_HAL_IOM_NO_STOP | AM_HAL_IOM_RAW);

        am_hal_iom_spi_read(g_sI2cIOMSettings.ui32IOMModule,
                            g_sI2cIOMSettings.ui32BusAddress, (uint32_t *)pui8Dest,
                            ui32TransferSize, AM_HAL_IOM_RAW);

        //
        // Update the number of bytes remaining and the destination.
        //
        ui32BytesRemaining -= ui32TransferSize;
        pui8Dest += ui32TransferSize;
        ui32CurrentReadAddress += ui32TransferSize;
    }
}

//*****************************************************************************
//
//! @brief Programs the given range of fram addresses.
//!
//! @param ui32DeviceNumber - Device number of the external fram
//! @param pui8TxBuffer - Buffer to write the external fram data from
//! @param ui32WriteAddress - Address to write to in the external fram
//! @param ui32NumBytes - Number of bytes to write to the external fram
//!
//! This function uses the data in the provided pui8TxBuffer and copies it to
//! the external fram at the address given by ui32WriteAddress. It will copy
//! exactly ui32NumBytes of data from the original pui8TxBuffer pointer. The
//! user is responsible for ensuring that they do not overflow the target fram
//! memory or underflow the pui8TxBuffer array
//
//*****************************************************************************
void
am_devices_i2cfram_write(uint8_t *pui8TxBuffer, uint32_t ui32WriteAddress,
                         uint32_t ui32NumBytes)
{
    uint32_t ui32DestAddress;
    uint32_t ui32BytesRemaining;
    uint8_t *pui8Source;

    //
    // Set the total number of bytes, and the starting transfer destination.
    //
    ui32BytesRemaining = ui32NumBytes;
    pui8Source = pui8TxBuffer;
    ui32DestAddress = ui32WriteAddress;

    while ( ui32BytesRemaining )
    {
        //
        // Set the transfer size to either TRANSFERSIZE,
        // or the number of remaining bytes, whichever is smaller.
        //
        uint32_t ui32TransferSize =
            (ui32BytesRemaining > MAX_I2C_TRANSFERSIZE - 2) ? \
                (MAX_I2C_TRANSFERSIZE - 2): \
                ui32BytesRemaining;

        //
        // Set the CMD and copy the data into the same buffer.
        // Command is 2 byte offset, followed by the data
        //
        g_i2cXferBuffer[g_i2cWrBufIdx].bytes[0] = (ui32DestAddress & 0x0000FF00) >> 8;
        g_i2cXferBuffer[g_i2cWrBufIdx].bytes[1] = (ui32DestAddress & 0x000000FF);

        memcpy(&g_i2cXferBuffer[g_i2cWrBufIdx].bytes[2], pui8Source, ui32TransferSize);

        //
        // Send Write Offset followed by data to I2CFRAM.
        //
        am_hal_iom_spi_write(g_sI2cIOMSettings.ui32IOMModule,
                             g_sI2cIOMSettings.ui32BusAddress,
                             g_i2cTempBuffer[g_i2cTempBufIdx].words, ui32TransferSize + 2,
                             AM_HAL_IOM_RAW);
        //
        // Update the number of bytes remaining, as well as the source and
        // destination pointers
        //
        ui32BytesRemaining -= ui32TransferSize;
        pui8Source += ui32TransferSize;
        ui32DestAddress += ui32TransferSize;
    }
    return;
}



