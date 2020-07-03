
/*
* Copyright (c), NXP Semiconductors
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
* NXP reserves the right to make changes without notice at any time.
*
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
* particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
* arising from its use.
*/

#include "driver_config.h"
#include "board.h"
#include "tml.h"
#include "phOsalNfc.h"
#include <phOsalNfc_Log.h>

/* Initializes pin muxing for I2C interface - note that SystemInit() may
 already setup your pin muxing at system startup */
static void Init_I2C_PinMux(void) {
#if defined(BOARD_NXP_LPCXPRESSO_54102)
	/* Connect the I2C_SDA and I2C_SCL signals to port pins */
	Chip_IOCON_PinMuxSet(LPC_IOCON, PORT_PIN_I2C, PIN_I2C_SCL, (IOCON_FUNC1 | IOCON_DIGITAL_EN | IOCON_STDI2C_EN)); /*SCL*/
	Chip_IOCON_PinMuxSet(LPC_IOCON, PORT_PIN_I2C, PIN_I2C_SDA, (IOCON_FUNC1 | IOCON_DIGITAL_EN | IOCON_STDI2C_EN)); /*SDA*/
    
    phOsalNfc_Log("\n\r I2C scl:%d, sda:%d ---\n", PIN_I2C_SCL, PIN_I2C_SDA);

	// NFC_IRQ
	Chip_IOCON_PinMuxSet(LPC_IOCON, PORT_PIN_IRQ, PIN_IRQ, (IOCON_FUNC0 | IOCON_DIGITAL_EN));
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, PORT_PIN_IRQ, PIN_IRQ);

	// VEN_IO
	Chip_IOCON_PinMuxSet(LPC_IOCON, PORT_PIN_VEN, PIN_VEN, (IOCON_FUNC0 | IOCON_DIGITAL_EN));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, PORT_PIN_VEN, PIN_VEN);

	//Download_IO
	Chip_IOCON_PinMuxSet(LPC_IOCON, PORT_PIN_DNLD, PIN_DNLD, (IOCON_FUNC0 | IOCON_DIGITAL_EN));
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, PORT_PIN_DNLD, PIN_DNLD);


#else
	/* Configure your own I2C pin muxing here if needed */
#warning "No I2C pin muxing defined"
#endif
}

uint8_t tml_Init(void) {
#if 0
	// I2C initialization.
	Board_I2C_Init(I2C_NPC);
	Chip_I2C_Init(I2C_NPC);
	Chip_I2C_SetClockRate(I2C_NPC,I2C_SPEED);
	NVIC_DisableIRQ(I2C_IRQ);
	Chip_I2C_SetMasterEventHandler(I2C_NPC, Chip_I2C_EventHandlerPolling);

	// IO's initialization.
	Chip_IOCON_Init(LPC_IOCON);
	Chip_GPIO_Init(LPC_GPIO);

	// NPC_IRQ
	Chip_IOCON_PinMuxSet(LPC_IOCON,PORT_PIN_IRQ,PIN_IRQ,IOCON_FUNC0);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO,PORT_PIN_IRQ,PIN_IRQ);

	// VEN_IO
	Chip_IOCON_PinMuxSet(LPC_IOCON,PORT_PIN_VEN,PIN_VEN,IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,PORT_PIN_VEN,PIN_VEN);

#ifndef DEVKIT_BOARD
	// DWL_IO
	Chip_IOCON_PinMuxSet(LPC_IOCON,PORT_PIN_DWL,PIN_DWL,IOCON_FUNC0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO,PORT_PIN_DWL,PIN_DWL);
	Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_DWL,PIN_DWL,0);
#endif // DEVKIT_BOARD

	// Enable timer 0 clock
	Chip_TIMER_Init(LPC_TIMER0);

	// Reset the NPC100 chip.
	Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,0);
	sleep(VEN_DIVIDER);
	Chip_GPIO_WritePortBit(LPC_GPIO,PORT_PIN_VEN,PIN_VEN,1);
	sleep(VEN_DIVIDER);
#endif

	/* Initialize GPIO grouped interrupt Block */
	Chip_GPIOGP_Init(LPC_GINT);

	/* Setup I2C pin muxing, enable I2C clock and reset I2C peripheral */
	Init_I2C_PinMux();
	Chip_Clock_EnablePeriphClock(LPC_I2CM_CLOCK);
	Chip_SYSCON_PeriphReset(LPC_I2CM_RESET);

	//init Download_IO
	Chip_GPIO_WritePortBit(LPC_GPIO, PORT_PIN_DNLD, PIN_DNLD, 0);
	phOsalNfc_Delay(20);

	// Reset the NFC chip.
	Chip_GPIO_WritePortBit(LPC_GPIO, PORT_PIN_VEN, PIN_VEN, 0);
	phOsalNfc_Delay(200);
	Chip_GPIO_WritePortBit(LPC_GPIO, PORT_PIN_VEN, PIN_VEN, 1);
	phOsalNfc_Delay(200);
	return SUCCESS;
}
