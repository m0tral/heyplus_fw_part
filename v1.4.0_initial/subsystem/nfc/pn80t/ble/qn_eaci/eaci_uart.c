/**
 ****************************************************************************************
 *
 * @file eaci_uart.c
 *
 * @brief UART transport module functions for Easy Application Controller Interface.
 *
 * Copyright(C) 2015 NXP Semiconductors N.V.
 * All rights reserved.
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"
#include "eaci.h"

uint8_t  payl_len     = 0 ;
uint8_t *payl_para    = NULL;
uint8_t  en_read_flag = 0;
volatile uint8_t eaci_enable_flag = 0;

extern volatile uint8_t eaci_enable_flag;

void eaci_uart_read_start(void)
{   
    eaci_env.rx_state = EACI_STATE_RX_START;       // Initialize UART in reception mode state

#if defined(CFG_HCI_UART)                          // Set the UART environment to message type 1 byte reception
//	ROM_UART_Receive(BLE_UART_PORT, &eaci_env.msg_type, 1);
#endif
}

void eaci_uart_read_hdr(void)                      // Read header
{
    eaci_env.rx_state = EACI_STATE_RX_HDR;         // Change Rx state - wait for message id and parameter length

#if defined(CFG_HCI_UART)                          // Set UART environment to header reception of EACI_MSG_HDR_LEN bytes

//    ROM_UART_Receive(BLE_UART_PORT, (uint8_t *)&eaci_env.msg_id, EACI_MSG_HDR_LEN);
#endif
}

void eaci_uart_read_payl(uint8_t len, uint8_t *par)
{
    eaci_env.rx_state = EACI_STATE_RX_PAYL;        // change rx state to payload reception

#if defined(CFG_HCI_UART)                          // set UART environment to payload reception of len bytes
//    ROM_UART_Receive(BLE_UART_PORT, par, len);
#endif
}

void eaci_uart_write(uint8_t len, uint8_t *par)
{
    volatile uint32_t status;
    tx_done = 0;

#if defined(CFG_HCI_UART)

    Chip_UART_SendBlocking(BLE_UART_PORT, par, len);               // send out the parameter with length
    tx_done = 1;
    while(1) {
        status = Chip_UART_GetStatus(BLE_UART_PORT);
        if (status & UART_STAT_TXIDLE)
            break;
    }
#endif
}

#ifdef MSG_BUFFER_EN
uint8_t msg_buffer[10][32];
uint8_t msg_index = 0;
#endif

void uartrom_regcb_eaci(void)
{
	eaci_enable_flag = 1;
}

#ifndef NFC_UNIT_TEST
/* IRQ Handler to deal with EACI bytes and send to FreeRTOS queue */
#include "queue.h"
#include <usr_config.h>

extern QueueHandle_t eaci_queue;

#ifdef CONNECTED_DEMO
#define MAX_BUF     10
#define MAX_LEN     32
#else
#define MAX_BUF     50
#define MAX_LEN     70
#endif /* CONNECTED_DEMO */

static uint8_t uartbuffer[MAX_BUF][MAX_LEN];
static uint8_t index   = 0;
static uint8_t bytepos = 0;

#if 0
void BLE_UART_HAND(void)
{
	static uint8_t eaci_rx_state = EACI_STATE_RX_START;

	uint8_t byte;
	bool bFullmsg = false;

	if ((Chip_UART_GetStatus(BLE_UART_PORT) & UART_STAT_RXRDY) != 0) {
		Chip_UART_ClearStatus(BLE_UART_PORT, UART_STAT_RXRDY);

		/* save the byte to buffer */
		byte = uartbuffer[index][bytepos++] = Chip_UART_ReadByte(BLE_UART_PORT);

		/* check UART state machine */
		switch (eaci_rx_state)
		{
		case EACI_STATE_RX_START:
			/* only response to EACI_MSG_TYPE_DATA_IND(0xEC) and EACI_MSG_TYPE_EVT(0xED) */
			if ((byte == EACI_MSG_TYPE_DATA_IND) || (byte == EACI_MSG_TYPE_EVT)) {
				eaci_rx_state = EACI_STATE_RX_HDR;
			} else {
				/* restart to search EACI_MSG_TYPE_DATA_IND(0xEC) and EACI_MSG_TYPE_EVT(0xED) */
				eaci_rx_state = EACI_STATE_RX_START;
				bytepos = 0;
			}
			break;
			
		case EACI_STATE_RX_HDR:
			if (bytepos >= 3) {
				if (byte == 0) {	
					/* Deal with 3 bytes length EACI packet, MSG_TYPE|MSG_ID|PARAM_LEN=0| */
					eaci_rx_state = EACI_STATE_RX_START;
					bFullmsg = true;
				} else {
					eaci_rx_state = EACI_STATE_RX_PAYL;
				}
			}
			break;
			
		case EACI_STATE_RX_PAYL:
			if (bytepos == (3 + uartbuffer[index][2])) {
				/* All Params received */
				eaci_rx_state = EACI_STATE_RX_START;
				bFullmsg = true;
			}
			break;

		default:
		    {
		    	phOsalNfc_LogBle("Bad message received in ISR \n");
		    }
			break;
		}

		if (bFullmsg) {
			void *p_eaci_msg = uartbuffer[index];
			BaseType_t xHigherPriorityTaskWokenByPost;

			/* Post to FreeRTOS Queue */
			xQueueSendFromISR(eaci_queue, &p_eaci_msg, &xHigherPriorityTaskWokenByPost);

			/* Prepare for next EACI msg */
			bytepos = 0;
            if (++index >= MAX_BUF) {
                index = 0;
            }

			/* Switching Task if needed */
		    portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
		}
	}
}
#endif
#endif

