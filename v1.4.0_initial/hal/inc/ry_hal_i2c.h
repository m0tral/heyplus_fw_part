/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_I2C_H__
#define __RY_HAL_I2C_H__

/*********************************************************************
 * INCLUDES
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
    I2C_IDX_NFC = 0x00,
    I2C_IDX_HR,
    I2C_TOUCH,
    I2C_GSENSOR,
	I2C_TOUCH_BTLDR,
	I2C_IDX_NFC_HW,
    I2C_IDX_MAX
} ry_i2c_t;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init and enable specified I2C master module
 *
 * @param   idx  - The specified i2c instance
 *
 * @return  None
 */
void ry_hal_i2cm_init(ry_i2c_t idx);


/**
 * @brief   API to power up specified I2C module
 *
 * @param   idx  - The specified i2c instance
 *
 * @return  None
 */
void ry_hal_i2cm_powerup(ry_i2c_t idx);

/**
 * @brief   API to power down specified I2C module
 *
 * @param   idx  - The specified i2c instance
 *
 * @return  None
 */
void ry_hal_i2cm_powerdown(ry_i2c_t idx);

/**
 * @brief   API to send I2C data
 *
 * @param   idx   - The specified i2c instance
 * @param   pData - Pointer to the TX data to be sent
 * @param   len   - Length of buffer
 *
 * @return  Status
 */
ry_sts_t ry_hal_i2cm_tx(ry_i2c_t idx, u8_t* pData, u32_t len);

/**
 * @brief   API to receive I2C data
 *
 * @param   idx   - The specified i2c instance
 * @param   pData - Pointer to put the received RX data
 * @param   len   - Length of buffer
 *
 * @return  Status
 */
ry_sts_t ry_hal_i2cm_rx(ry_i2c_t idx, u8_t* pData, u32_t len);

/**
 * @brief   API to send I2C data at reg_addr
 *
 * @param   idx   - The specified i2c instance
 * @param   reg_addr - master sends the one byte register address
 * @param   pData - Pointer to the TX data to be sent
 * @param   len   - Length of buffer
 *
 * @return  Status
 */
ry_sts_t ry_hal_i2cm_tx_at_addr(ry_i2c_t idx, uint8_t reg_addr, u8_t* pData, u32_t len);

/**
 * @brief   API to receive I2C data at reg_addr
 *
 * @param   idx   - The specified i2c instance
 * @param   reg_addr - write tx data, usually the read reg address
 * @param   pData - Pointer to put the received RX data
 * @param   len   - Length of buffer
 *
 * @return  Status
 */
ry_sts_t ry_hal_i2cm_rx_at_addr(ry_i2c_t idx, uint8_t reg_addr, u8_t* pData, u32_t len);

/**
 * @brief	API to do I2C "tx & rx" Test
 * I2C master sends commands packets using high level PDL function
 * Master reads the reply from the slave to know whether commands are received properly.
 *
 * @param	cmd    - 0xaa: comand to I2C tx & rx, else: do nothing
 *
 * @return	status - result of i2c_TestMasterI2C
 *			         0: RY_SUCC, else: fail
 */
ry_sts_t ry_hal_i2c_TestMasterI2C(uint8_t cmd);

/**
 * @brief   API to init and enable specified I2C master module
 *
 * @return  None
 */
void ry_hal_i2c_master_init(void);

u8_t ry_hal_is_i2cm_touch_enable(void);

void ry_hal_i2cm_uninit(ry_i2c_t idx);

#endif  /* __RY_HAL_I2C_H__ */
