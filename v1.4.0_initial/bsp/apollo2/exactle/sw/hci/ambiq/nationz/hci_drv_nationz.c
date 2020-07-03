//*****************************************************************************
//
//! @file hci_drv.c
//!
//! @brief HCI driver interface.
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
#include <stdint.h>
#include <stdbool.h>

#include "wsf_types.h"
#include "bstream.h"
#include "wsf_msg.h"
#include "wsf_cs.h"
#include "hci_drv.h"
#include "hci_drv_apollo.h"
#include "hci_tr_apollo.h"
#include "hci_core.h"

//#include "hci_vs_nationz.h"

#include "am_mcu_apollo.h"
#include "am_util.h"
#include "am_bsp.h"
#include "am_devices_nationz.h"

#include "hci_apollo_config.h"

#include <string.h>

uint8_t g_BLEMacAddress[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

//*****************************************************************************
//
// Follow BSP settings for the IOM.
//
//*****************************************************************************
#define HCI_DRV_NATIONZ_IOM                     0
#define HCI_DRV_NATIONZ_CS                      1

//
// Take over the interrupt handler for whichever IOM we're using.
//
#define HciDrvSpiISR                                                          \
    HciDrvSpiISR2(HCI_DRV_NATIONZ_IOM)
#define HciDrvSpiISR2(n)                                                      \
    HciDrvSpiISR3(n)
#define HciDrvSpiISR3(n)                                                      \
    am_iomaster ## n ## _isr

//*****************************************************************************
//
// Static function prototypes.
//
//*****************************************************************************
static void hciDrvReadCallback(uint8_t *pui8Data, uint32_t ui32Length);

//*****************************************************************************
//
// Boot the radio.
//
//*****************************************************************************
void
HciDrvRadioBoot(uint32_t ui32Module)
{
    //
    // Enable the pins and IOM for the Nationz chip.
    //
    am_devices_nationz_setup();

    //
    // Apply any necessary patches here...
    //

    //
    // Tell the Nationz part that patching is complete.
    //
    am_devices_nationz_patches_complete();

    //
    // Make sure our read callback is appropriately registered.
    //
    am_devices_nationz_set_read_callback(hciDrvReadCallback);

    return;
}

//*****************************************************************************
//
// Send an HCI message.
//
//*****************************************************************************
uint16_t
hciDrvWrite(uint8_t type, uint16_t len, uint8_t *pData)
{
    am_devices_nationz_write(type, len, pData);

    return len;
}

//*****************************************************************************
//
// Nationz SPI ISR
//
// This function should be called in response to all SPI interrupts during
// operation of the Nationz radio.
//
//*****************************************************************************
void
HciDrvSpiISR(void)
{
    uint32_t ui32Status;

    //
    // Check to see which interrupt caused us to enter the ISR.
    //
    ui32Status = am_hal_iom_int_status_get(HCI_DRV_NATIONZ_IOM, true);

    //
    // Fill or empty the FIFO, and either continue the current operation or
    // start the next one in the queue. If there was a callback, it will be
    // called here.
    //
    am_hal_iom_queue_service(HCI_DRV_NATIONZ_IOM, ui32Status);

    //
    // Clear the interrupts before leaving the ISR.
    //
    am_hal_iom_int_clear(HCI_DRV_NATIONZ_IOM, ui32Status);
}

//*****************************************************************************
//
// GPIO ISR for Nationz data ready.
//
//*****************************************************************************
void
HciDataReadyISR(void)
{
    //
    // Start the HCI read action on the bus. When the data comes back, it will
    // come back through the read callback we set up during boot.
    //
    am_devices_nationz_read();
}

//*****************************************************************************
//
// Function for handling HCI read data from the Nationz chip.
//
//*****************************************************************************
static void
hciDrvReadCallback(uint8_t *pui8Data, uint32_t ui32Length)
{
    //
    // Pass the newly received data along to the Cordio stack.
    //
    hciTrSerialRxIncoming(pui8Data, ui32Length);
}
