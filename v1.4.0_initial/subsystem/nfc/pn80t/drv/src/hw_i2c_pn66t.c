/*
 * @brief I2CM bus master example using interrupt mode
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "board.h"
#include <stdlib.h>
#include "timer.h"
#include "driver_config.h"
#include "tool.h"
#include "tml.h"
#include "port.h"

#if 0 /* Enable for I2C driver unit testing */
#include "task.h"
#endif
#include "phOsalNfc.h"

#include "phNxpNciHal_SelfTest.h"
#include "phOsalNfc_Timer.h"
#include <phOsalNfc_Log.h>

extern uint8_t wakeup_source_nfc;

/** @defgroup I2CM_INT_5410X I2C master (interrupt mode) ROM API example
 * @ingroup EXAMPLES_ROM_5410X
 * @include "rom\i2cm_int\readme.txt"
 */

/**
 * @}
 */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* 100KHz I2C bit-rate */
#define I2C_CLK_DIVIDER     (12)
#define I2C_BITRATE         SPEED_400KHZ

/** I2C interface setup */

#define OK              1
#define NOK             0

void (*i2cmcallback)(void);

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* Made global so data can be seen in the debuuger */
uint8_t rx[10], tx[4];

volatile bool done;
uint32_t actualRate;
uint32_t pin_stat;
uint32_t int_cpt;
bool i2cm_checkpres;
void * ni2cRomAccessMutex;
//void * RxIrqSem = NULL;
//volatile bool RxIrqReceived = FALSE;
//volatile bool isirqEnabled = FALSE;
/* I2CM transfer record */
I2CM_XFER_T          i2cmXferRec;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Function to wait for I2CM transfer completion */
static void WaitForI2cXferComplete(I2CM_XFER_T *xferRecPtr)
{
	/* Test for still transferring data */
	while (xferRecPtr->status == I2CM_STATUS_BUSY) {
		/* Sleep until next interrupt */
		__WFI();
	}
}

#if 0
/**
 * @brief	Handle I2C1 interrupt by calling I2CM interrupt transfer handler
 * @return	Nothing
 */
void LPC_I2C_INTHAND(void)
{
	uint32_t state = Chip_I2C_GetPendingInt(LPC_I2C_PORT);

	/* Error handling */
	if (state & (I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR)) {
		Chip_I2CM_ClearStatus(LPC_I2C_PORT, I2C_STAT_MSTRARBLOSS | I2C_STAT_MSTSTSTPERR);
	}

	/* Call I2CM ISR function with the I2C device and transfer rec */
	if (state & I2C_INTENSET_MSTPENDING) {
		state = Chip_I2CM_GetMasterState(LPC_I2C_PORT);
		if (state == I2C_STAT_MSTCODE_RXREADY) {
			Chip_I2CM_XferHandler(LPC_I2C_PORT, &i2cmXferRec);
		}
		else if (state == I2C_STAT_MSTCODE_TXREADY) {
			Chip_I2CM_XferHandler(LPC_I2C_PORT, &i2cmXferRec);
		}
		else {
			Chip_I2CM_XferHandler(LPC_I2C_PORT, &i2cmXferRec);
		}

		if (i2cmXferRec.status == I2CM_STATUS_OK) {
			Chip_I2C_DisableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);
		}
	}
}
#endif

#if 0
void Enable_GPIO0_IRQ(void)
{
    /* PIN_IRQ Setup for rising edge*/
    Chip_GPIOGP_SelectHighLevel(LPC_GINT, 0, PORT_PIN_IRQ, 1UL << PIN_IRQ);
    Chip_GPIOGP_EnableGroupPins(LPC_GINT, 0, PORT_PIN_IRQ, 1UL << PIN_IRQ);

    /*Set up trigger*/
    Chip_GPIOGP_SelectAndMode(LPC_GINT, 0);
    Chip_GPIOGP_SelectEdgeMode(LPC_GINT, 0);

    isirqEnabled = TRUE;
    NVIC_EnableIRQ(GINT0_IRQn);
}

void Clear_GPIO0_IRQ(void)
{
    Chip_GPIOGP_ClearIntStatus(LPC_GINT, PORT_PIN_IRQ);
}

void Disable_GPIO0_IRQ(void)
{
    NVIC_DisableIRQ(GINT0_IRQn);
    isirqEnabled = FALSE;
}

#endif

#if 0
void GINT0_IRQHandler(void)
{
	int_cpt++;
	/*Board_LED_Toggle(0);*/
    /* As its configured for Edge Trigger, consider only the rising edge */
    pin_stat = Chip_GPIO_GetPinState(LPC_GPIO, PORT_PIN_IRQ, PIN_IRQ);
    if (1 == pin_stat)
    {
        phOsalNfc_ProduceSemaphore(RxIrqSem);
        RxIrqReceived = TRUE;
        Disable_GPIO0_IRQ();
    }
    wakeup_source_nfc = 1;
}
#endif

int i2cmhandler_send(uint8_t *pBuff, uint16_t buffLen) {
    uint8_t ret = NOK;

    phOsalNfc_LockMutex(ni2cRomAccessMutex);

	/* Setup I2C transfer record */
	i2cmXferRec.slaveAddr = I2C_ADDR_7BIT;
	i2cmXferRec.status    = 0;
	i2cmXferRec.txSz      = buffLen;
	i2cmXferRec.rxSz      = 0;
	i2cmXferRec.txBuff    = pBuff;
	i2cmXferRec.rxBuff    = 0;
	done = false;
//	DEBUGOUT("\r\n--------i2c send addr %x tx %d rx %x\r\n", i2cmXferRec.slaveAddr, i2cmXferRec.txSz, i2cmXferRec.rxSz);
	Chip_I2CM_Xfer(LPC_I2C_PORT, &i2cmXferRec);
	Chip_I2C_EnableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);
	WaitForI2cXferComplete(&i2cmXferRec);
	Chip_I2C_DisableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);

	LPC_I2C_PORT->MSTCTL = I2C_MSTCTL_MSTSTOP; // send stop
	while(!(LPC_I2C_PORT->STAT & I2C_STAT_MSTPENDING));

	if (i2cmXferRec.status != I2CM_STATUS_OK) {
		DEBUGOUT("\r\nI2C error: %d\r\n", i2cmXferRec.status);
	}

	if (i2cmXferRec.status != LPC_OK) {
//	    phOsalNfc_Log("\r\n -------------tran I2C transfer failed: status = %x\r\n", i2cmXferRec.status);
		ret = (NOK);
	}
	else {
		done = true;
//		phOsalNfc_Log("\r\n ------------tran I2C transfer OK \r\n");
		ret = (OK);
	}

	phOsalNfc_UnlockMutex(ni2cRomAccessMutex);

	return ret;
}

int i2cmhandler_receive(uint8_t *pBuff, uint16_t *pBytesRead, uint16_t timeout) {
	uint8_t ret = NOK;

#ifdef TML_DEBUG
	ext_comm_intfs_printf_out("npcR\n");
#endif // TML_DEBUG

    phOsalNfc_LockMutex(ni2cRomAccessMutex);

    /* Setup I2C transfer record */
    i2cmXferRec.slaveAddr = I2C_ADDR_7BIT;
    i2cmXferRec.status    = 0;
    i2cmXferRec.txSz      = 0;
    i2cmXferRec.rxSz      = *pBytesRead;
    i2cmXferRec.txBuff    = 0;
    i2cmXferRec.rxBuff    = pBuff;

    done = false;
    Chip_I2CM_Xfer(LPC_I2C_PORT, &i2cmXferRec);
    Chip_I2C_EnableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);
    WaitForI2cXferComplete(&i2cmXferRec);
    Chip_I2C_DisableInt(LPC_I2C_PORT, I2C_INTENSET_MSTPENDING | I2C_INTENSET_MSTRARBLOSS | I2C_INTENSET_MSTSTSTPERR);

    if (i2cmXferRec.status != LPC_OK) {
        phOsalNfc_Log("\r\n------ recv I2C transfer failed: status = %x\r\n", i2cmXferRec.status);
        *pBytesRead = 0;
        ret = (NOK);
    }
    else {
        done = true;
//		    phOsalNfc_Log("\r\n------ recv I2C transfer OK Size %d  %x %x %x\r\n", *pBytesRead, pBuff[0], pBuff[1], pBuff[2]);
        ret = (OK);
    }
    phOsalNfc_UnlockMutex(ni2cRomAccessMutex);
	return ret;
}

/**
 * @brief	Main routine for I2C example
 * @return	Function should not exit
 */
int i2cmhandler_init() {

	//tml_Init();
	phOsalNfc_CreateMutex(&ni2cRomAccessMutex);
	phOsalNfc_CreateSemaphore(&RxIrqSem, 0);
	isirqEnabled = FALSE;
	RxIrqReceived = FALSE;

	/* Enable I2C clock and reset I2C peripheral */
	Chip_I2C_Init(LPC_I2C_PORT);

	/* Setup clock rate for I2C */
	Chip_I2C_SetClockDiv(LPC_I2C_PORT, I2C_CLK_DIVIDER);

	/* Setup I2CM transfer rate */
	Chip_I2CM_SetBusSpeed(LPC_I2C_PORT, I2C_BITRATE);
	phOsalNfc_Log("\r\nActual I2C master rate = %dHz\r\n", I2C_BITRATE);
	/* Enable I2C master interface */
	Chip_I2CM_Enable(LPC_I2C_PORT);
	/* Enable the interrupt for the I2C */
	NVIC_EnableIRQ(LPC_IRQNUM);

	/* Set the priority for GPIO group0 interrupt */
	/* must be low priority than configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (bigger in numbering) */
	NVIC_SetPriority(GINT0_IRQn, 7);

	/* Code never reaches here. Only used to satisfy standard main() */
	return 1;
}

int i2cmhandler_close() {
    phOsalNfc_DeleteMutex(&ni2cRomAccessMutex);
    phOsalNfc_DeleteSemaphore(&RxIrqSem);
    return OK;
}

int i2cmhandler_checkNFCpresence(void) {
	uint16_t pBytesRead = 0;

	tx[0] = 0x20;
	tx[1] = 0x00;
	tx[2] = 0x01;
	tx[3] = 0x00;

	/*Do not inform upper layer of interrupt*/

	i2cm_checkpres = TRUE;
	int res = i2cmhandler_send(&tx[0], 4);

	if (res != OK) {
		/*errorOut("Error during I2CM transfer\r\n");*/
		return (NOK);
	}

	res = i2cmhandler_receive(rx, &pBytesRead, TIMEOUT_100MS);
	i2cm_checkpres = FALSE;

	if ((pBytesRead == 6) && (res == OK)) {
		phOsalNfc_Log("NFC module present\r\n");

		return (OK);
	} else {
		/*errorOut("Error during I2CM transfer\r\n");*/
		return (NOK);
	}
}
#if 0

void NFC_InitProcess (void)
{
	i2cmhandler_init();
}

static void NFCTask (void *pParam)
{
	NFC_InitProcess();
	while(1);
}


int main(void) {
	i2cmcallback = NULL;
	pin_stat = 0;
	int_cpt = 0;
	i2cm_checkpres = FALSE;

	/* Start NFC task */


	xTaskCreate( NFCTask, ( const signed char * ) "NFCTask", 1200, NULL, ( tskIDLE_PRIORITY + 2 ), NULL );

	/* Start the kernel.  From here on, only tasks and interrupts will run. */
	vTaskStartScheduler();


	/* Since the application has nothing else to do ... just loop here */
	while(1);


	return (0);
}

#endif










