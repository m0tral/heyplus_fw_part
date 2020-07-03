
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

#ifndef __UART_INT_DMA_H_
#define __UART_INT_DMA_H_

#include "board.h"
#include <phOsalNfc_Log.h>

extern volatile bool tx_done;
extern volatile bool rx_done;

//extern void	uartrom_config(int);
extern void BLE_UART_CONFIG( int UART_BAUD_RATE);              /* Configure UART ROM Driver and pripheral */
extern void BLE_UART_InitForISP(void);
extern void BLE_UART_InitForEACI(void);

extern void UART1_Init(void);
#ifdef DTM_TEST_ENABLE
#include "FreeRTOS.h"
#include "task.h"

extern int  CheckTxTimeOut(portTickType ticks);
extern int  CheckRxTimeOut(portTickType ticks);

#endif

void ROM_UART_Receive(LPC_USART_T *pUART, uint8_t* rxbuffer, uint32_t len); //rxbuf to rxbuffer. rxbuf is global and compiler threw error.
void ROM_UART_Send(LPC_USART_T *pUART, uint8_t* txbuf, uint32_t len);

extern volatile uint8_t  rxbuff[], txbuff[];
extern volatile uint32_t ble_recv_cnt;

#endif /* __UART_INT_DMA_H_ */
