/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_FLASH_H__
#define __RY_HAL_FLASH_H__

#include "ry_type.h"
#include "am_multi_boot.h"


extern am_multiboot_flash_info_t g_intFlash;





#define RY_FLASH_ADDR_BASE         	0x00000000

#define RY_FLASH_ADDR_END 			0x00100000

#define RY_FLASH_IAP_SECTOR        	14 // 32K Byte per sector

#define RY_FILESYSTEM_START_ADDR	0x000A0000

#define RY_DEVICE_INFO_ADDR         0x8000

#define RY_DEVICE_FW_VER_ADDR       0x9C00

#define RY_XIAOMI_CERT              0x7000


#define RY_FLASH_SIZEOF_ROW			(AM_HAL_FLASH_PAGE_SIZE)
#define SECTOR_SIZE                 RY_FLASH_SIZEOF_ROW


#define RY_FILESYSTEM_START_SECTOR	((RY_FILESYSTEM_START_ADDR - RY_FLASH_ADDR_BASE)/SECTOR_SIZE)


/**
 * @brief   API to init UART module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_flash_init(void);

/**
 * @brief   API to init Erase specified flash
 *
 * @param   start - Start address to erase
 * @param   size - The size to be erased
 *
 * @return  Status
 */
ry_sts_t ry_hal_flash_erase(u32_t start, u32_t size);

/**
 * @brief   API to write data to flash
 *
 * @param   addr - Start address to write
 * @param   buf - The data to be saved
 * @param   len - The size to be saved
 *
 * @return  Status
 */
ry_sts_t ry_hal_flash_write(u32_t addr, u8_t* buf, int len);

/**
 * @brief   API to read data from flash
 *
 * @param   addr - Start address to read
 * @param   buf - The RAM buffer to put data
 * @param   len - The length of data
 *
 * @return  Status
 */
ry_sts_t ry_hal_flash_read(u32_t addr, u8_t* buf, int len);


/**
 * @brief   API to get bank number
 *
 * @param   Addr - address to get bank number
 *
 * @return  bank number
 */
uint32_t GetBank(uint32_t Addr);

/**
 * @brief   API to get page number
 *
 * @param   Addr - address to get page number
 *
 * @return  page number
 */
uint32_t GetPage(uint32_t Addr);


/**
 * @brief   API to write data to flash
 *
 * @param   sector - Start sector number to write
 * @param   buf - The RAM buffer to put data
 * @param   len - The length of data
 *
 * @return  Status
 */
ry_sts_t ry_hal_sector_write(u32_t sector, u8_t* buf, int count);

/**
 * @brief   API to read data from flash
 *
 * @param   addr - Start sector number to read
 * @param   buf - The RAM buffer to put data
 * @param   len - The length of data
 *
 * @return  Status
 */
ry_sts_t ry_hal_sector_read(u32_t sector, u8_t* buf, int count);


ry_sts_t ry_hal_flash_write_unlock(u32_t addr, u8_t* buf, int len);




#endif  /* __RY_HAL_FLASH_H__ */
