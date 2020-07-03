
#ifndef __RY_HAL_SPI_FLASH_H__
#define __RY_HAL_SPI_FLASH_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ry_type.h"

#define EXFLASH_SECTOR_SIZE         4096
#define EXFLASH_SECTOR_NUM          1024

#define EXFLASH_BYTES_SIZE          (1024*1024*4)

//*****************************************************************************
//
// Global definitions for flash commands
//
//*****************************************************************************
#if 0 
#define AM_DEVICES_SPIFLASH_WREN        0x06        // Write enable
#define AM_DEVICES_SPIFLASH_WRDI        0x04        // Write disable
#define AM_DEVICES_SPIFLASH_RDID        0x9F        // Read Identification
#define AM_DEVICES_SPIFLASH_RDRSR       0x05        // Read status register
#define AM_DEVICES_SPIFLASH_WRSR        0x01        // Write status register
#define AM_DEVICES_SPIFLASH_READ        0x03        // Read data bytes
#define AM_DEVICES_SPIFLASH_PP          0x02        // Page program
#define AM_DEVICES_SPIFLASH_SE          0x20       // Sector Erase
#define AM_DEVICES_SPIFLASH_BE          0xD8        // Block Erase
#define AM_DEVICES_SPIFLASH_CE          0x60        // chip Erase
#define RY_DEVICES_SPIFLASH_DPD					0xB9				//deep Power down
#define RY_DEVICES_SPIFLASH_RDPD				0xAB				//release From deep power down

#endif

//*****************************************************************************
//
// Global definitions for the flash status register
//
//*****************************************************************************
#define AM_DEVICES_SPIFLASH_WEL         0x02        // Write enable latch
#define AM_DEVICES_SPIFLASH_WIP         0x01        // Write in progress

//*****************************************************************************
//
// Global definitions for the flash size information
//
//*****************************************************************************
#define AM_DEVICES_SPIFLASH_PAGE_SIZE       0x100    //256 bytes, minimum program unit
#define AM_DEVICES_SPIFLASH_SUBSECTOR_SIZE  0x1000   //4096 bytes
#define AM_DEVICES_SPIFLASH_SECTOR_SIZE     0x10000  //65536 bytes

#define ry_hal_spi_flash_power_off()        ry_hal_spi_flash_config(RY_DEVICES_SPIFLASH_DPD)

#define ry_hal_spi_flash_power_on()        ry_hal_spi_flash_config(RY_DEVICES_SPIFLASH_RDPD)

#define SPI_FLASH_WRITE_FREQUENCY       AM_HAL_IOM_4MHZ
#define SPI_FLASH_READ_FREQUENCY        AM_HAL_IOM_24MHZ




//*****************************************************************************
//
// Function pointers for SPI write and read.
//
//*****************************************************************************
typedef bool (*am_devices_spiflash_write_t)(uint32_t ui32Module,
                                            uint32_t ui32ChipSelect,
                                            uint32_t *pui32Data,
                                            uint32_t ui32NumBytes,
                                            uint32_t ui32Options);

typedef bool (*am_devices_spiflash_read_t)(uint32_t ui32Module,
                                           uint32_t ui32ChipSelect,
                                           uint32_t *pui32Data,
                                           uint32_t ui32NumBytes,
                                           uint32_t ui32Options);

//*****************************************************************************
//
// Device structure used for communication.
//
//*****************************************************************************
/*typedef struct
{
    //
    // Module number to use for IOM access.
    //
    uint32_t ui32IOMModule;

    //
    // Chip Select number to use for IOM access.
    //
    uint32_t ui32ChipSelect;
}
am_devices_spiflash_t;*/
extern u32_t cur_spi_flash_frq;


//*****************************************************************************
//
// External function definitions.
//
//*****************************************************************************
extern uint8_t ry_hal_spi_flash_init(void);

extern uint8_t ry_hal_spi_flash_status(void);

extern uint32_t ry_hal_spi_flash_id(void);

extern void ry_hal_spi_flash_read(uint8_t *pui8RxBuffer,
                                     uint32_t ui32ReadAddress,
                                     uint32_t ui32NumBytes);

extern int ry_hal_spi_flash_write(uint8_t *ui8TxBuffer,
                                      uint32_t ui32WriteAddress,
                                      uint32_t ui32NumBytes);

extern void ry_hal_spi_flash_mass_erase(void);

extern void ry_hal_spi_flash_sector_erase(uint32_t ui32SectorAddress);

void ry_hal_spi_flash_chip_erase(void);

ry_sts_t ry_hal_sector_write_exflash(u32_t sector, u8_t* buf, int count);

ry_sts_t ry_hal_sector_read_exflash(u32_t sector, u8_t* buf, int count);

void ry_hal_spi_flash_config(uint8_t config);
void ry_hal_spi_flash_wakeup(void);
uint8_t ry_hal_spi_flash_uninit(void);



#ifdef __cplusplus
}
#endif

#endif // AM_DEVICES_SPIFLASH_H













