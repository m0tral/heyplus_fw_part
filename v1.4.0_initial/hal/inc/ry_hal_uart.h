/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#ifndef __RY_HAL_UART_H__
#define __RY_HAL_UART_H__

/*
 * INCLUDES
 *******************************************************************************
 */
#include "ry_type.h"

/*
 * CONSTANTS
 *******************************************************************************
 */
 

/*
 * TYPES
 *******************************************************************************
 */

typedef enum {
	UART_IDX_LOG = 0x00,
	UART_IDX_TOOL,
	UART_IDX_BLE,
	UART_IDX_MAX,
} ry_uart_t;


/**
 * @brief  Type definition of UART receiver callback function.
 */
typedef void (*ry_uart_rx_cb_t)(ry_uart_t idx);


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init UART module
 *
 * @param   idx - which uart to be init.
 * @param   cb  - rx callback function. Note: the receive buffer.
 *
 * @return  None
 */
void ry_hal_uart_init(ry_uart_t idx, ry_uart_rx_cb_t cb);

/**
 * @brief   API to power up specified UART module
 *
 * @param   idx  - The specified uart instance
 *
 * @return  None
 */
void ry_hal_uart_powerup(ry_uart_t idx);

/**
 * @brief   API to power down specified UART module
 *
 * @param   idx  - The specified uart instance
 *
 * @return  None
 */
void ry_hal_uart_powerdown(ry_uart_t idx);


/**
 * @brief   API to send UART data
 *
 * @param   idx - Which port to send data.
 * @param   buf - Buffer to be sent through UART
 * @param   len - Length of buffer
 *
 * @return  None
 */
void ry_hal_uart_tx(ry_uart_t idx, u8_t* buf, u32_t len);

/**
 * @brief   API to polling receive UART data
 *
 * @param   idx - Which port to send data.
 * @param   buf - Buffer to put received UART data
 * @param   len - Length of buffer
 *
 * @return  None
 */
void ry_hal_uart_rx(ry_uart_t idx, u8_t* buf, u32_t* len);




#endif /* __RY_HAL_UART_H__ */
