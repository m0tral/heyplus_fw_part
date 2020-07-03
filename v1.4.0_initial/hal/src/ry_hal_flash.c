/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_mcu_config.h"
#include "ry_hal_flash.h"
#include "string.h"

#if (BSP_MCU_TYPE == BSP_APOLLO2)
#include "board.h"

#include "ryos.h"


/*********************************************************************
 * CONSTANTS
 */ 
//#define RY_FLASH_ADDR_BASE         	0x10000000
//#define RY_FLASH_IAP_SECTOR        	14 // 32K Byte per sector

//#define SECTOR_SIZE                 32768

///#define CY_FLASH_SIZEOF_ROW			(512u)

/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
static ryos_mutex_t *flash_mutex = NULL;


static am_multiboot_flash_info_t *g_FlashHandle = &g_intFlash;

//static u8_t temp_buf[RY_FLASH_SIZEOF_ROW] = {0};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to init UART module
 *
 * @param   None
 *
 * @return  None
 */
void ry_hal_flash_init(void)
{
    ry_sts_t status;
    
    if (flash_mutex) {
        return;
    }

    status = ryos_mutex_create(&flash_mutex);

    if (RY_SUCC != status) {
        RY_ASSERT(0);
    }
}

/**
 * @brief   API to init Erase specified flash
 *
 * @param   start - Start address to erase
 * @param   size - The size to be erased
 *
 * @return  Status
 */
ry_sts_t ry_hal_flash_erase(u32_t start, u32_t size)
{
    ry_sts_t status = RY_SUCC;
    u8_t ret_code;

    u32_t i = 0;
     
    ryos_mutex_lock(flash_mutex);


	while ( size )
    {
        status = g_FlashHandle->flash_erase_sector(start);
        if ( size > g_FlashHandle->flashSectorSize )
        {
            size -= g_FlashHandle->flashSectorSize;
            start += g_FlashHandle->flashSectorSize;
        }
        else
        {
            break;
        }
    }


exit:

    
    ryos_mutex_unlock(flash_mutex);

    return status;
}

/**
 * @brief   API to write data to flash
 *
 * @param   addr - Start address to erase
 * @param   buf - The data to be saved
 * @param   len - The size to be saved
 *
 * @return  Status
 */
ry_sts_t ry_hal_flash_write(u32_t addr, u8_t* buf, int len)
{
	ry_sts_t status = RY_SUCC;
	u32_t fWriteTwice = 0;
	u32_t i = 0;
	u32_t result = 0;
	
	ryos_mutex_lock(flash_mutex);

	u32_t start_row = (addr / RY_FLASH_SIZEOF_ROW);
	u32_t start_row_addr = start_row * RY_FLASH_SIZEOF_ROW;
	u32_t end_row = ((addr + len )  + RY_FLASH_SIZEOF_ROW -1)/ RY_FLASH_SIZEOF_ROW;
	u32_t used_row_num = end_row - start_row;

	u32_t cp_len = (used_row_num >1) ? (RY_FLASH_SIZEOF_ROW - (addr % RY_FLASH_SIZEOF_ROW)) : (len);

	u8_t *temp_buf = (u8_t *)ry_malloc(RY_FLASH_SIZEOF_ROW);
	if(NULL == temp_buf){
        LOG_ERROR("flash malloc failed!!! \n");
        status = RY_SET_STS_VAL(RY_CID_HAL_FLASH, RY_ERR_NO_MEM);
        goto exit;
	}

	for (i = 0; i < used_row_num ; i++){

		if(i == 0) {

			ry_memcpy(temp_buf, (void *)start_row_addr, RY_FLASH_SIZEOF_ROW);
			
			ry_memcpy(&temp_buf[addr % RY_FLASH_SIZEOF_ROW], (void *)buf, cp_len);

			result = g_FlashHandle->flash_erase_sector(start_row_addr);
			result = g_FlashHandle->flash_write_page(start_row_addr, (u32_t *)&temp_buf[0], RY_FLASH_SIZEOF_ROW);
			
		}else if (i == used_row_num - 1){

			ry_memcpy(temp_buf, (void *)(start_row_addr + i*RY_FLASH_SIZEOF_ROW), RY_FLASH_SIZEOF_ROW);

			ry_memcpy(temp_buf, &buf[cp_len + (i - 1) * RY_FLASH_SIZEOF_ROW], 
							(len - cp_len - (i - 1) * RY_FLASH_SIZEOF_ROW));

			result = g_FlashHandle->flash_erase_sector(start_row_addr + i * RY_FLASH_SIZEOF_ROW);

			result = g_FlashHandle->flash_write_page((start_row_addr + i * RY_FLASH_SIZEOF_ROW), (u32_t *)&temp_buf[0], RY_FLASH_SIZEOF_ROW);

		}else{

			result = g_FlashHandle->flash_erase_sector(start_row_addr + i * RY_FLASH_SIZEOF_ROW);
			
			result = g_FlashHandle->flash_write_page((start_row_addr + i * RY_FLASH_SIZEOF_ROW), (u32_t *)&buf[cp_len + (i - 1) * RY_FLASH_SIZEOF_ROW], RY_FLASH_SIZEOF_ROW);
		}

		if( result != 0){
			status = RY_SET_STS_VAL(RY_CID_HAL_FLASH, RY_ERR_WRITE);
		}

	}
	
    ry_free(temp_buf);
	
exit:
	
	ryos_mutex_unlock(flash_mutex);

	return status;
}


ry_sts_t ry_hal_flash_write_unlock(u32_t addr, u8_t* buf, int len)
{
    ry_sts_t status = RY_SUCC;
    u32_t fWriteTwice = 0;
    u32_t i = 0;
    u32_t result = 0;

    u32_t start_row = (addr / RY_FLASH_SIZEOF_ROW);
    u32_t start_row_addr = start_row * RY_FLASH_SIZEOF_ROW;
    u32_t end_row = ((addr + len )  + RY_FLASH_SIZEOF_ROW -1)/ RY_FLASH_SIZEOF_ROW;
    u32_t used_row_num = end_row - start_row;

    u32_t cp_len = (used_row_num >1) ? (RY_FLASH_SIZEOF_ROW - (addr % RY_FLASH_SIZEOF_ROW)) : (len);

    u8_t *temp_buf = (u8_t *)ry_malloc(RY_FLASH_SIZEOF_ROW);
    if(NULL == temp_buf){
        LOG_ERROR("flash malloc failed!!! \n");
        status = RY_SET_STS_VAL(RY_CID_HAL_FLASH, RY_ERR_NO_MEM);
        goto exit;
    }

    for (i = 0; i < used_row_num ; i++){

        if(i == 0) {

            ry_memcpy(temp_buf, (void *)start_row_addr, RY_FLASH_SIZEOF_ROW);
            
            ry_memcpy(&temp_buf[addr % RY_FLASH_SIZEOF_ROW], (void *)buf, cp_len);

            result = g_FlashHandle->flash_erase_sector(start_row_addr);
            result = g_FlashHandle->flash_write_page(start_row_addr, (u32_t *)&temp_buf[0], RY_FLASH_SIZEOF_ROW);
            
        }else if (i == used_row_num - 1){

            ry_memcpy(temp_buf, (void *)(start_row_addr + i*RY_FLASH_SIZEOF_ROW), RY_FLASH_SIZEOF_ROW);

            ry_memcpy(temp_buf, &buf[cp_len + (i - 1) * RY_FLASH_SIZEOF_ROW], 
                            (len - cp_len - (i - 1) * RY_FLASH_SIZEOF_ROW));

            result = g_FlashHandle->flash_erase_sector(start_row_addr + i * RY_FLASH_SIZEOF_ROW);

            result = g_FlashHandle->flash_write_page((start_row_addr + i * RY_FLASH_SIZEOF_ROW), (u32_t *)&temp_buf[0], RY_FLASH_SIZEOF_ROW);

        }else{

            result = g_FlashHandle->flash_erase_sector(start_row_addr + i * RY_FLASH_SIZEOF_ROW);
            
            result = g_FlashHandle->flash_write_page((start_row_addr + i * RY_FLASH_SIZEOF_ROW), (u32_t *)&buf[cp_len + (i - 1) * RY_FLASH_SIZEOF_ROW], RY_FLASH_SIZEOF_ROW);
        }

        if( result != 0){
            status = RY_SET_STS_VAL(RY_CID_HAL_FLASH, RY_ERR_WRITE);
        }

    }
    

    ry_free(temp_buf);
    
exit:

    return status;
}


/**
 * @brief   API to read data from flash
 *
 * @param   addr - Start address to erase
 * @param   buf - The RAM buffer to put data
 * @param   len - The length of data
 *
 * @return  Status
 */
ry_sts_t ry_hal_flash_read(u32_t addr, u8_t* buf, int len)
{
    u8_t* p = (u8_t*) (addr);

    ryos_mutex_lock(flash_mutex);
    ry_memcpy(buf, p, len);
    ryos_mutex_unlock(flash_mutex);

    return RY_SUCC;
}

uint32_t GetBank(uint32_t Addr)
{
  uint32_t bank = 0;
  
  
  return bank;
}
uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;
  
  return page;
}

ry_sts_t ry_hal_sector_write(u32_t sector, u8_t* buf, int count)
{
	ry_sts_t status = RY_SUCC;
	u32_t cur_setcor = sector;

	for (cur_setcor = sector ; cur_setcor < (sector + count) ; cur_setcor++) {
	
		status = ry_hal_flash_write(RY_FILESYSTEM_START_ADDR + cur_setcor * SECTOR_SIZE , &buf[(cur_setcor - sector) * SECTOR_SIZE] , SECTOR_SIZE);

		if (status != RY_SUCC) {
			break;
		}

	}
	
	return status;
}




ry_sts_t ry_hal_sector_read(u32_t sector, u8_t* buf, int count)
{
	ry_sts_t status = RY_SUCC;
	u32_t cur_setcor = sector;

	for (cur_setcor = sector ; cur_setcor < (sector + count) ; cur_setcor++) {
	
		status = ry_hal_flash_read(RY_FILESYSTEM_START_ADDR + cur_setcor * SECTOR_SIZE , &buf[(cur_setcor - sector) * SECTOR_SIZE] , SECTOR_SIZE);

		if (status != RY_SUCC) {
			break;
		}

	}
	return status;
}


void ry_hal_flash_test(void)
{
#define TEST_FLASH_ADDR    0x0009E000


    u8_t temp[100] = {0};
    u8_t x = 0x55;
    ry_hal_flash_init();
    
    ry_hal_flash_read(TEST_FLASH_ADDR, temp, 100);
    LOG_DEBUG("Test Flash Read: \n");
    ry_data_dump(temp, 100, ' ');

    LOG_DEBUG("Test Flash Write: \n");
    ry_memset(temp, temp[0]+1, 100);
    ry_hal_flash_write(TEST_FLASH_ADDR, temp, 100);
    ry_data_dump(temp, 100, ' ');

    LOG_DEBUG("Test Flash Read Again: \n");
    ry_hal_flash_read(TEST_FLASH_ADDR, temp, 100);
    ry_data_dump(temp, 100, ' ');
    
}



#endif /* (BSP_MCU_TYPE == BSP_LPC54101) */
