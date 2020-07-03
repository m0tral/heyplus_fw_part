
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

#include <WearableCoreSDK_BuildConfig.h>
#include <usr_config.h>                        // DEFINES USED PINS !!
#include "uart_int_dma.h"
#include "wakeup.h"
#include "port.h"
#include "phOsalNfc.h"

volatile uint8_t wakeup_source_nfc = 0;
static uint8_t g_blesleep_nesting = 0;


#if 0 // Kanjie, used for FP
void WU_IRQ0_HANDLER(void)
{
    Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH(WU_PININT_INDEX_BLE));
}
#endif

void WU_IRQ1_HANDLER(void)
{
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH(WU_PININT_INDEX_NFC));
}

/* Brown-out detector interrupt */
void BOD_IRQHandler(void)
{
    /*
    * Disable any more BODD interrupt unless
    * battery is down to lower threshold
    */
	NVIC_DisableIRQ(BOD_IRQn);
}


#if 0 /* Unused */
/* pin mux table, only items changing from their default state are in this table. */
static const PINMUX_GRP_T pinmuxing_lowpower[] =
{
	/* UART */
//  {0, 0,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* UART0 RX */
//  {0, 1,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* UART0 TX */

	{0, 2,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 3,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	
	{0, 4,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//	{0, 5,  (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//	{0, 6,  (IOCON_FUNC1 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//	{0, 7,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	
//	{0, 8,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 9,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//	{0, 10,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 11,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 12,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 13,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 14,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 15,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//  {0, 16,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//  {0, 17,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 18,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//	{0, 19,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 20,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 21,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 22,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 23,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 24,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 25,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 26,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 27,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{0, 28,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	
	{1, 9,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 10,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 11,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 12,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 13,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 14,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 15,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
	{1, 16,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
//    {1, 17,  (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN)}, /* NC */
};

static void SetupUnusedPins(void)                       // makes all unused GPIO pins output high
{
//  Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 0);           // P0_0 = U0_RxD
//  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 1);          // P0_1 = U0_TxD
//  Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, 0);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 2);          // P0_2 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 3);          // P0_3 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 3, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 4);          // P0_4 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 4, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 5);          // P0_5 = U1_RxD
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 5, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 6);          // P0_6 = U1_RxD
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 6, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 7);          // P0_7 = QN9021 state_1
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 7, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 8);          // P0_8 = QN9021 state change
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 8, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 9);          // P0_9 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 9, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 10);         // P0_10 = QN9021_WAKEUP
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 10, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 11);         // P0_11 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 11, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 12);         // P0_12 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 12, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 13);         // P0_13 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 13, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 14);         // P0_14 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 14, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 15);         // P0_15 = SWO
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 15, 0);
//  Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 16);         // P0_16 = SWCLK
//  Chip_GPIO_SetPinState(LPC_GPIO, 0, 16, 0);
// 	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 17);         // P0_17 = SWDIO
//  Chip_GPIO_SetPinState(LPC_GPIO, 0, 17, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 18);         // P0_18 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 18, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 19);         // P0_19 = NFC_IRQ
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 19, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);         // P0_20 = NFC_VEN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 21);         // P0_21 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 21, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 22);         // P0_22 not used
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 22, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 23);         // P0_23 = NCF_I2C_SCL
    Chip_GPIO_SetPinState(LPC_GPIO, 0, 23, 0);

//  Chip_GPIO_SetPinDIRInput(LPC_GPIO, 0, 24);          // P0_24 = NCF_I2C_SDA --> For button wakeup
//	Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, 0);

    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 25);         // P0_25 = NCF_DL_REQ
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 25, 0);
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 26);         // P0_26 = QN9021_RESETN
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 26, 1);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 27);         // P0_27 ?
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 27, 0);

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 9);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 9, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 10);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 10, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 11);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 11, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 12);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 12, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 13);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 13, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 14);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 14, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 15);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 15, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 16);
	Chip_GPIO_SetPinState(LPC_GPIO, 1, 16, 0);
//	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 1, 17);         // P1_17 = QN9021_host_wakeup --> ACTUAL QN WAKEUP PIN
//	Chip_GPIO_SetPinState(LPC_GPIO, 1, 17, 0);
}

static void ConfigureUnusedPins(void)
{
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_IOCON);
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_GPIO0);
	Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_GPIO1);
	Chip_IOCON_SetPinMuxing(LPC_IOCON, pinmuxing_lowpower, sizeof(pinmuxing_lowpower) / sizeof(PINMUX_GRP_T));
	SetupUnusedPins();
}
#endif

void wakeup_init(void)
{
    //ConfigureUnusedPins();

    // init output pin used to wake up the QN902x and make it high
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, HOST_WAKEUP_BLE_PORT, HOST_WAKEUP_BLE_PIN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, HOST_WAKEUP_BLE_PORT, HOST_WAKEUP_BLE_PIN,
			            (IOCON_FUNC0 | IOCON_DIGITAL_EN  | IOCON_GPIO_MODE));
    Chip_GPIO_SetPinState(LPC_GPIO, HOST_WAKEUP_BLE_PORT, HOST_WAKEUP_BLE_PIN, true);

    // LPC wake up pin is setup as a GPIO input
    Chip_IOCON_PinMuxSet(LPC_IOCON, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN, (IOCON_FUNC0 | IOCON_DIGITAL_EN | IOCON_GPIO_MODE));
	//Chip_IOCON_PinMuxSet(LPC_IOCON, NFC_WAKEUP_HOST_PORT, NFC_WAKEUP_HOST_PIN, (IOCON_FUNC1 | IOCON_DIGITAL_EN | IOCON_GPIO_MODE));

    Chip_GPIO_SetPinDIRInput(LPC_GPIO, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN);
	//Chip_GPIO_SetPinDIRInput(LPC_GPIO, NFC_WAKEUP_HOST_PORT, NFC_WAKEUP_HOST_PIN);


    // wait for pin to not be low
    //while ((Chip_GPIO_ReadPortBit(LPC_GPIO, BLE_wakeup_Host_PORT, BLE_WAKEUP_HOST_PIN) == 0) || (Chip_GPIO_ReadPortBit(LPC_GPIO, NFC_wakeup_Host_PORT, NFC_wakeup_Host_PIN)==0)) {} // THIS IN FACT NEEDS TIMEOUT AND ERROR
    while ((Chip_GPIO_ReadPortBit(LPC_GPIO, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN) == 0)) {} // THIS IN FACT NEEDS TIMEOUT AND ERROR


    // configure pin interrupt selection for the GPIO pin in Input Mux Block
    Chip_PININT_Init(LPC_PININT);
    Chip_INMUX_PinIntSel(WU_PININT_INDEX_BLE, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN);
	Chip_INMUX_PinIntSel(WU_PININT_INDEX_NFC, NFC_WAKEUP_HOST_PORT, NFC_WAKEUP_HOST_PIN);

    // configure channel interrupt as edge sensitive and falling edge interrupt
    Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH(WU_PININT_INDEX_BLE));
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH(WU_PININT_INDEX_NFC));

    Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH(WU_PININT_INDEX_BLE));
	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH(WU_PININT_INDEX_NFC));

    Chip_PININT_EnableIntLow(LPC_PININT, PININTCH(WU_PININT_INDEX_BLE));
	Chip_PININT_EnableIntHigh(LPC_PININT, PININTCH(WU_PININT_INDEX_NFC));

	NVIC_SetPriority(PININT_NVIC_NAME0,7);
    NVIC_EnableIRQ(PININT_NVIC_NAME0);                                  // enable PININT interrupt
    Chip_SYSCON_EnableWakeup(SYSCON_STARTER_PINT0);                    // enable wake up for PININT0

	NVIC_SetPriority(PININT_NVIC_NAME1,7);
    NVIC_EnableIRQ(PININT_NVIC_NAME1);                                  // enable PININT interrupt
    Chip_SYSCON_EnableWakeup(SYSCON_STARTER_PINT1);                    // enable wake up for PININT1
}

void BLEgotoSleep(void)
{
    uint32_t primask;

    INT_DISABLE(primask, 0);

    if (g_blesleep_nesting > 0) {
    	g_blesleep_nesting--;
		if (g_blesleep_nesting == 0) {
			Chip_GPIO_SetPinState(LPC_GPIO, HOST_WAKEUP_BLE_PORT, HOST_WAKEUP_BLE_PIN, true);
		}
    }

    INT_RESTORE(primask, 0);
}

void BLE_Reset(void)
{
	/*ble reset signal is low active,set reset pin output about a 4ms low level.*/
	/* Configure pin as GPIO */

	Chip_IOCON_PinMuxSet(LPC_IOCON, BLE_REST_PORT, BLE_REST_PIN,
						 (IOCON_FUNC0 | IOCON_DIGITAL_EN  | IOCON_GPIO_MODE));

	Chip_GPIO_SetPinDIROutput(LPC_GPIO, BLE_REST_PORT, BLE_REST_PIN);
	
    Chip_GPIO_SetPinState(LPC_GPIO, BLE_REST_PORT, BLE_REST_PIN, true);
    Chip_GPIO_SetPinState(LPC_GPIO, BLE_REST_PORT, BLE_REST_PIN, false);
    phOsalNfc_Delay(10);
    Chip_GPIO_SetPinState(LPC_GPIO, BLE_REST_PORT, BLE_REST_PIN, true);      
    phOsalNfc_Delay(50);
}

void BLE_wakeup(void)
{
    uint32_t primask;

    INT_DISABLE(primask, 0);

    g_blesleep_nesting++;
	if (g_blesleep_nesting == 1) {
		INT_RESTORE(primask, 0);

		Chip_GPIO_SetPinState(LPC_GPIO, HOST_WAKEUP_BLE_PORT, HOST_WAKEUP_BLE_PIN, false);
		phOsalNfc_Delay(5);                               // ? msec delay to make sure QN9020 woke up.
	} else {
		INT_RESTORE(primask, 0);
	}
}
