/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_SPI_H__
#define __RY_HAL_SPI_H__


/*********************************************************************
 * INCLUDES
 */
#include "ry_type.h"

/*
 * CONSTANTS
 *******************************************************************************
 */

/* None */

/*
 * TYPES
 *******************************************************************************
 */

typedef enum {
    SPI_IDX_GSENSOR = 0x00,
    SPI_IDX_OLED,
	SPI_IDX_FLASH,
    SPI_IDX_BLE,
    SPI_IDX_MAX
} ry_spi_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init and enable specified SPI master module
 *
 * @param   idx  - The specified spi instance
 *
 * @return  None
 */
#if SPI_LOCK_DEBUG
void ry_hal_spim_init(ry_spi_t idx, const char* caller);  
#else
void ry_hal_spim_init(ry_spi_t idx);   
#endif

/**
 * @brief   API to init and enable specified SPI master module
 *
 * @param   idx  - The specified spi instance
 *
 * @return  None
 */
#if SPI_LOCK_DEBUG
void ry_hal_spim_uninit(ry_spi_t idx, const char* caller);  
#else
void ry_hal_spim_uninit(ry_spi_t idx);   
#endif

uint8_t ry_hal_spi_flash_uninit(void);


/**
 * @brief   API to power up specified SPI module
 *
 * @param   idx  - The specified SPI instance
 *
 * @return  None
 */
void ry_hal_spim_powerup(ry_spi_t idx);

/**
 * @brief   API to power down specified SPI module
 *
 * @param   idx  - The specified SPI instance
 *
 * @return  None
 */
void ry_hal_spim_powerdown(ry_spi_t idx);

/**
 * @brief   API to send/receive SPI data
 *
 * @param   idx     - The specified spi instance
 * @param   pTxData - Pointer to the TX data to be sent
 * @param   pRxData - Pointer to the RX data to receive
 * @param   len     - Length of buffer
 *
 * @return  Status  - result of SPI read or wirte
 *			          0: RY_SUCC, else: fail
 */
ry_sts_t ry_hal_spi_txrx(ry_spi_t idx, u8_t* pTxData, u8_t* pRxData, u32_t len);

/**
 * @brief	function to do SPI "read & Write" Test
 *
 * @param	null
 *
 * @return	status - result of SPI read or wirte
 *			         0: RY_SUCC, else: fail
 */
ry_sts_t ry_hal_spi_testTxRx(void);

/**
 * @brief   API to send/receive SPI data
 *
 * @param   idx     - The specified spi instance
 * @param   pTxData - Pointer to the TX data to be sent
 * @param   pRxData - Pointer to the RX data to receive
 * @param   len     - Length of buffer
 * @param   option  - 
 *
 * @return  Status  - result of SPI read or wirte
 *			          0: RY_SUCC, else: fail
 */
ry_sts_t ry_hal_spi_write_read(ry_spi_t idx, u8_t* pTxData, u8_t* pRxData, u32_t len, uint32_t option);


void ry_hal_spim_flash_specific_init(void);

#endif  /* __RY_HAL_SPI_H__ */
