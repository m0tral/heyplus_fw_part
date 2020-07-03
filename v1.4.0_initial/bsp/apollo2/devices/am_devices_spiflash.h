//*****************************************************************************
//
//! @file am_devices_spiflash.h
//!
//! @brief Generic spiflash driver.
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

#ifndef AM_DEVICES_SPIFLASH_H
#define AM_DEVICES_SPIFLASH_H

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Global definitions for flash commands
//
//*****************************************************************************
#if 0
#define AM_DEVICES_SPIFLASH_WREN        0x06        // Write enable
#define AM_DEVICES_SPIFLASH_WRDI        0x04        // Write disable
#define AM_DEVICES_SPIFLASH_RDID        0x9F        // Read Identification
#define AM_DEVICES_SPIFLASH_RDRSR       0x05        // Read status register
#define AM_DEVICES_SPIFLASH_WRSR        0x01        // Write status register
#define AM_DEVICES_SPIFLASH_READ        0x03        // Read data bytes
#define AM_DEVICES_SPIFLASH_PP          0x02        // Page program
#define AM_DEVICES_SPIFLASH_SE          0x20        // Sector Erase
#define AM_DEVICES_SPIFLASH_BE          0xD8        // Bulk Erase
#define AM_DEVICES_SPIFLASH_DS			0xB9        //deep sleep
#endif
#define AM_DEVICES_SPIFLASH_RDS         0xAB        // release deep sleep

#define AM_DEVICES_SPIFLASH_WREN        0x06        // Write enable
#define AM_DEVICES_SPIFLASH_WRDI        0x04        // Write disable
#define AM_DEVICES_SPIFLASH_RDID        0x9F        // Read Identification
#define AM_DEVICES_SPIFLASH_RDRSR       0x05        // Read status register
#define AM_DEVICES_SPIFLASH_WRSR        0x01        // Write status register
#define AM_DEVICES_SPIFLASH_READ        0x03        // Read data bytes
#define AM_DEVICES_SPIFLASH_PP          0x02        // Page program
#define AM_DEVICES_SPIFLASH_SE          0x20        // Sector Erase
#define AM_DEVICES_SPIFLASH_BE          0xD8        // Block Erase
#define AM_DEVICES_SPIFLASH_CE          0x60        // chip Erase
#define RY_DEVICES_SPIFLASH_DPD			0xB9		// deep Power down
#define RY_DEVICES_SPIFLASH_RDPD	    0xAB		// release From deep power down


//*****************************************************************************
//
// Global definitions for the flash status register
//
//*****************************************************************************
#define AM_DEVICES_SPIFLASH_WEL         0x02        // Write enable latch
#define AM_DEVICES_SPIFLASH_WIP         0x01        // Write in progress

//*****************************************************************************
//
// Global definitions for the flash size information
//
//*****************************************************************************
#define AM_DEVICES_SPIFLASH_PAGE_SIZE       0x100    //256 bytes, minimum program unit
#define AM_DEVICES_SPIFLASH_SUBSECTOR_SIZE  0x1000   //4096 bytes
#define AM_DEVICES_SPIFLASH_SECTOR_SIZE     0x10000  //65536 bytes

//*****************************************************************************
//
// Function pointers for SPI write and read.
//
//*****************************************************************************
typedef bool (*am_devices_spiflash_write_t)(uint32_t ui32Module,
                                            uint32_t ui32ChipSelect,
                                            uint32_t *pui32Data,
                                            uint32_t ui32NumBytes,
                                            uint32_t ui32Options);

typedef bool (*am_devices_spiflash_read_t)(uint32_t ui32Module,
                                           uint32_t ui32ChipSelect,
                                           uint32_t *pui32Data,
                                           uint32_t ui32NumBytes,
                                           uint32_t ui32Options);

//*****************************************************************************
//
// Device structure used for communication.
//
//*****************************************************************************
typedef struct
{
    //
    // Module number to use for IOM access.
    //
    uint32_t ui32IOMModule;

    //
    // Chip Select number to use for IOM access.
    //
    uint32_t ui32ChipSelect;
}
am_devices_spiflash_t;

//*****************************************************************************
//
// External function definitions.
//
//*****************************************************************************
extern uint8_t am_devices_spiflash_init(void);

extern uint8_t am_devices_spiflash_status(void);

extern uint32_t am_devices_spiflash_id(void);

extern void am_devices_spiflash_read(uint8_t *pui8RxBuffer,
                                     uint32_t ui32ReadAddress,
                                     uint32_t ui32NumBytes);

extern void am_devices_spiflash_write(uint8_t *ui8TxBuffer,
                                      uint32_t ui32WriteAddress,
                                      uint32_t ui32NumBytes);

extern void am_devices_spiflash_mass_erase(void);

extern void am_devices_spiflash_sector_erase(uint32_t ui32SectorAddress);

#ifdef __cplusplus
}
#endif

#endif // AM_DEVICES_SPIFLASH_H

