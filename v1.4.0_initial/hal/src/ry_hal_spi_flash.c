

#include "app_config.h"
#include "am_mcu_apollo.h"
#include "am_devices_spiflash.h"
#include "ry_hal_spi.h"
#include "ry_utils.h"
#include "ry_hal_inc.h"
#include "ry_hal_spi_flash.h"
#include "ry_fs.h"
#include "am_util_delay.h"


//*****************************************************************************
//
// Global variables.
//
//*****************************************************************************
am_devices_spiflash_t *g_psIOMSettings;


am_devices_spiflash_t g_sGd25lq32d =
{
    .ui32IOMModule = 0,//AM_HAL_IOM_1MHZ,//AM_HAL_IOM_4MHZ,//AM_HAL_IOM_8MHZ, //AM_HAL_IOM_16MHZ,//AM_HAL_IOM_4MHZ,//
    .ui32ChipSelect = 0,
   
};

u32_t cur_spi_flash_frq = SPI_FLASH_READ_FREQUENCY;

//*****************************************************************************
//
//! @brief Initialize the spiflash driver.
//!
//! @param psIOMSettings - IOM device structure describing the target spiflash.
//! @param pfnWriteFunc - Function to use for spi writes.
//! @param pfnReadFunc - Function to use for spi reads.
//!
//! This function should be called before any other am_devices_spiflash
//! functions. It is used to set tell the other functions how to communicate
//! with the external spiflash hardware.
//!
//! The \e pfnWriteFunc and \e pfnReadFunc variables may be used to provide
//! alternate implementations of SPI write and read functions respectively. If
//! they are left set to 0, the default functions am_hal_iom_spi_write() and
//! am_hal_iom_spi_read() will be used.
//!
//! @return None.
//
//*****************************************************************************


void ry_hal_spi_flash_enter_powerdown(void)
{
#if 0
    am_hal_iom_buffer(4) psCommand;

    //
    // Send the write-enable command to prepare the external flash for program
    // operations.
    //
    u32_t r = ry_hal_irq_disable();
    psCommand.bytes[0] = AM_DEVICES_SPIFLASH_DS;
    am_hal_iom_spi_write(0,
                         0,
                         psCommand.words, 1, AM_HAL_IOM_RAW);
    ry_hal_irq_restore(r);

    am_util_delay_ms(1);

    ry_hal_spim_uninit(SPI_IDX_FLASH);

    
#endif
}

void ry_hal_spi_flash_wakeup(void)
{
    am_hal_iom_buffer(4) psCommand;
    u32_t r = ry_hal_irq_disable();

    //
    // Send the write-enable command to prepare the external flash for program
    // operations.
    //
    psCommand.bytes[0] = AM_DEVICES_SPIFLASH_RDS;
    am_hal_iom_spi_write(0,
                         0,
                         psCommand.words, 1, AM_HAL_IOM_RAW);
    ry_hal_irq_restore(r);

    am_util_delay_ms(1);
}


uint8_t 
ry_hal_spi_flash_init(void)
{
	uint32_t id;
    uint32_t pui32WriteBuffer[1];
    //
    // Initialize the IOM settings for the spiflash.
    //
    g_psIOMSettings = &g_sGd25lq32d;

#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_FLASH, __FUNCTION__);
#else 
    ry_hal_spim_init(SPI_IDX_FLASH);
#endif


  
#if 1
    ry_hal_spi_flash_wakeup();
#endif

	id = ry_hal_spi_flash_id();

    
	
#define FLASH_ID    0x25c2	//0x60c8
	if(id == FLASH_ID){
        //LOG_DEBUG("flash ID : 0x%06x\r\n", id);
        return 0;
	}
	else{
        //LOG_ERROR("flash ID : error -- %d\r\n",id);
        return 1;
	}
}

uint8_t
ry_hal_spi_flash_uninit(void)
{
#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_FLASH, __FUNCTION__);
#else 
    ry_hal_spim_uninit(SPI_IDX_FLASH);
#endif
	return 0;
}

//*****************************************************************************
//
//! @brief Reads the current status of the external flash
//!
//! @param ui32DeviceNumber - Device number of the external flash
//!
//! This function reads the status register of the external flash, and returns
//! the result as an 8-bit unsigned integer value. The processor will block
//! during the data transfer process, but will return as soon as the status
//! register had been read.
//!
//! Macro definitions for interpreting the contents of the status register are
//! included in the header file.
//!
//! @return 8-bit status register contents
//
//*****************************************************************************
uint8_t
ry_hal_spi_flash_status(void)
{
	u8_t rxbuf[2];
	u8_t txbuf;
	txbuf= AM_DEVICES_SPIFLASH_RDRSR;

    ry_hal_spim_flash_specific_init();
	
	ry_hal_spi_txrx(SPI_IDX_FLASH, &txbuf, rxbuf,2);
	
	//LOG_ERROR("spiflash_status:%02x,%02x\r\n",rxbuf[0],rxbuf[1]);
	
    return rxbuf[0];
}

//*****************************************************************************
//
//! @brief Reads the ID register for the external flash
//!
//! @param ui32DeviceNumber - Device number of the external flash
//!
//! This function reads the ID register of the external flash, and returns the
//! result as a 32-bit unsigned integer value. The processor will block during
//! the data transfer process, but will return as soon as the ID register had
//! been read. The ID contents for this flash only contains 24 bits of data, so
//! the result will be stored in the lower 24 bits of the return value.
//!
//! @return 32-bit ID register contents
//
//*****************************************************************************
uint32_t
ry_hal_spi_flash_id(void)
{
    u32_t rxbuf;
	u8_t txbuf;
	txbuf=AM_DEVICES_SPIFLASH_RDID;

	ry_hal_spi_txrx(SPI_IDX_FLASH, &txbuf, (u8_t*)&rxbuf, 3);
	
    return rxbuf & 0x00FFFFFF;
}

//*****************************************************************************
//
//! @brief Reads the contents of the external flash into a buffer.
//!
//! @param ui32DeviceNumber - Device number of the external flash
//! @param pui8RxBuffer - Buffer to store the received data from the flash
//! @param ui32ReadAddress - Address of desired data in external flash
//! @param ui32NumBytes - Number of bytes to read from external flash
//!
//! This function reads the external flash at the provided address and stores
//! the received data into the provided buffer location. This function will
//! only store ui32NumBytes worth of data.
//
//*****************************************************************************
void
ry_hal_spi_flash_read(uint8_t *pui8RxBuffer, uint32_t ui32ReadAddress,
                         uint32_t ui32NumBytes)
{
    uint32_t i, ui32BytesRemaining, ui32TransferSize, ui32CurrentReadAddress;
    uint8_t *pui8Dest;

    uint32_t pui32WriteBuffer[1];
    uint8_t *pui8WritePtr;

    uint32_t pui32ReadBuffer[16];
    uint8_t *pui8ReadPtr;

    //
    u32_t r = ry_hal_irq_disable();
    cur_spi_flash_frq = SPI_FLASH_READ_FREQUENCY;
    ry_hal_spi_flash_init();

    pui8WritePtr = (uint8_t *)(&pui32WriteBuffer);
    pui8ReadPtr = (uint8_t *)(&pui32ReadBuffer);

    //
    // Set the total number of bytes,and the starting transfer destination.
    //
    ui32BytesRemaining = ui32NumBytes;
    pui8Dest = pui8RxBuffer;
    ui32CurrentReadAddress = ui32ReadAddress;

    while ( ui32BytesRemaining )
    {
        //
        // Set the transfer size to either 64, or the number of remaining
        // bytes, whichever is smaller.
        //
        ui32TransferSize = ui32BytesRemaining > 2048 ? 2048 : ui32BytesRemaining;

        pui8WritePtr[0] = AM_DEVICES_SPIFLASH_READ;
        pui8WritePtr[1] = (ui32CurrentReadAddress & 0x00FF0000) >> 16;
        pui8WritePtr[2] = (ui32CurrentReadAddress & 0x0000FF00) >> 8;
        pui8WritePtr[3] = ui32CurrentReadAddress & 0x000000FF;
        //u32_t r = ry_hal_irq_disable();
        //
        // Send the read command.
        //
        /*am_hal_iom_spi_write(g_psIOMSettings->ui32IOMModule,
                             g_psIOMSettings->ui32ChipSelect,
                             pui32WriteBuffer, 4,
                             AM_HAL_IOM_CS_LOW | AM_HAL_IOM_RAW);*/
        ry_hal_spi_txrx(SPI_IDX_FLASH, (u8_t *)pui32WriteBuffer, NULL,4);

        am_hal_iom_spi_read(g_psIOMSettings->ui32IOMModule,
                            g_psIOMSettings->ui32ChipSelect, (uint32_t *)pui8Dest,
                            ui32TransferSize, AM_HAL_IOM_RAW);

        //u32_t tx_buf= AM_HAL_IOM_RAW>>8;
        //ry_hal_spi_txrx(SPI_IDX_FLASH, (u8_t *)&tx_buf, (u8_t *)&pui32ReadBuffer,ui32TransferSize+1);
        //ry_hal_irq_restore(r);
        //
        // Copy the received bytes over to the RxBuffer
        //
        /*for ( i = 0; i < ui32TransferSize; i++ )
        {
            pui8Dest[i] = pui8ReadPtr[i];
        }*/

        //
        // Update the number of bytes remaining and the destination.
        //
        ui32BytesRemaining -= ui32TransferSize;
        pui8Dest += ui32TransferSize;
        ui32CurrentReadAddress += ui32TransferSize;
    }

    ry_hal_spi_flash_enter_powerdown();
    ry_hal_spi_flash_uninit();
    ry_hal_irq_restore(r);
    
}

//*****************************************************************************
//
//! @brief Programs the given range of flash addresses.
//!
//! @param ui32DeviceNumber - Device number of the external flash
//! @param pui8TxBuffer - Buffer to write the external flash data from
//! @param ui32WriteAddress - Address to write to in the external flash
//! @param ui32NumBytes - Number of bytes to write to the external flash
//!
//! This function uses the data in the provided pui8TxBuffer and copies it to
//! the external flash at the address given by ui32WriteAddress. It will copy
//! exactly ui32NumBytes of data from the original pui8TxBuffer pointer. The
//! user is responsible for ensuring that they do not overflow the target flash
//! memory or underflow the pui8TxBuffer array
//
//*****************************************************************************
int
ry_hal_spi_flash_write(uint8_t *pui8TxBuffer, uint32_t ui32WriteAddress,
                          uint32_t ui32NumBytes)
{
    uint32_t i;
    uint32_t ui32DestAddress;
    uint32_t ui32BytesRemaining;
    uint32_t ui32TransferSize;
    uint8_t *pui8Source;
	am_hal_iom_status_e ui32Status;
	int result_status = 0;
    //
    u32_t r = ry_hal_irq_disable();

    cur_spi_flash_frq = SPI_FLASH_WRITE_FREQUENCY;
    ry_hal_spi_flash_init();

    am_hal_iom_buffer(1) psEnableCommand;
    am_hal_iom_buffer(64) psWriteCommand;
	u8_t read_check[64];
	int retry_count = 0;
    //
    // Prepare the command for write-enable.
    //
    psEnableCommand.bytes[0] = AM_DEVICES_SPIFLASH_WREN;

    //
    // Set the total number of bytes, and the starting transfer destination.
    //
    ui32BytesRemaining = ui32NumBytes;
    pui8Source = pui8TxBuffer;
    ui32DestAddress = ui32WriteAddress;

    while ( ui32BytesRemaining )
    {
        //
        // Set up a write command to hit the beginning of the next "chunk" of
        // flash.
        //
        psWriteCommand.bytes[0] = AM_DEVICES_SPIFLASH_PP;
        psWriteCommand.bytes[1] = (ui32DestAddress & 0x00FF0000) >> 16;
        psWriteCommand.bytes[2] = (ui32DestAddress & 0x0000FF00) >> 8;
        psWriteCommand.bytes[3] = ui32DestAddress & 0x000000FF;

        //
        // Set the transfer size to either 32, or the number of remaining
        // bytes, whichever is smaller.
        //
        ui32TransferSize = ui32BytesRemaining > 64 ? 64 : ui32BytesRemaining;

        //
        // Fill the rest of the command buffer with the data that we actually
        // want to write to flash.
        //
        /*for ( i = 0; i < ui32TransferSize; i++ )
        {
            psWriteCommand.bytes[4 + i] = pui8Source[i];
        }*/

        //
        // Send the write-enable command to prepare the external flash for
        // program operations, and wait for the write-enable latch to be set in
        // the status register.
        //
        /*am_hal_iom_spi_write(g_psIOMSettings->ui32IOMModule,
                             g_psIOMSettings->ui32ChipSelect,
                             psEnableCommand.words,
                             1, AM_HAL_IOM_RAW);*/
        ry_hal_spi_txrx(SPI_IDX_FLASH, (u8_t *)psEnableCommand.words, NULL,1);
        while ( !(ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WEL) );

        //
        // Send the write command.
        //
        /*am_hal_iom_spi_write(g_psIOMSettings->ui32IOMModule,
                             g_psIOMSettings->ui32ChipSelect,
                             psWriteCommand.words,
                             (ui32TransferSize + 4), AM_HAL_IOM_RAW);*/

        ry_hal_spi_txrx(SPI_IDX_FLASH, (u8_t *)psWriteCommand.words, NULL,(4));
        ui32Status = am_hal_iom_spi_write(g_psIOMSettings->ui32IOMModule,
                             g_psIOMSettings->ui32ChipSelect,
                             (uint32_t *)pui8Source,
                             (ui32TransferSize), AM_HAL_IOM_RAW | AM_HAL_IOM_CS_LOW);

        //
        // Wait for status to indicate that the write is no longer in progress.
        //
        while ( ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WIP );
        /*if(ui32Status){
			LOG_DEBUG("[%s]%d--error--%d\n",__FUNCTION__,__LINE__,ui32Status);
		}*/
        //
        // Update the number of bytes remaining, as well as the source and
        // destination pointers
        //

        //ry_hal_spi_flash_read(read_check , ui32DestAddress,ui32TransferSize);
		psWriteCommand.bytes[0] = AM_DEVICES_SPIFLASH_READ;
        psWriteCommand.bytes[1] = (ui32DestAddress & 0x00FF0000) >> 16;
        psWriteCommand.bytes[2] = (ui32DestAddress & 0x0000FF00) >> 8;
        psWriteCommand.bytes[3] = ui32DestAddress & 0x000000FF;

        ry_hal_spi_txrx(SPI_IDX_FLASH, (u8_t *)psWriteCommand.words, NULL,4);
		ry_memset(read_check , 0 , 64);
        am_hal_iom_spi_read(g_psIOMSettings->ui32IOMModule,
                            g_psIOMSettings->ui32ChipSelect, (uint32_t *)read_check,
                            ui32TransferSize, AM_HAL_IOM_RAW);
                            
		if(ry_memcmp(read_check, pui8Source, ui32TransferSize) != 0){
			LOG_DEBUG("[%s]%d--error--%d\n",__FUNCTION__,retry_count,ui32Status);
			for(i = 0; i < ui32TransferSize;i++){
				if(read_check[i] != 0xff){
					break;
				}
			}

			if(i == ui32TransferSize){
				retry_count++;
			}else{
				result_status = -1;
				goto exit;
			}

			if(retry_count > 10){
				result_status = -2;
				goto exit;
			}
			
		}else{
        	retry_count = 0;
	        ui32BytesRemaining -= ui32TransferSize;
	        pui8Source += ui32TransferSize;
	        ui32DestAddress += ui32TransferSize;
        }

        
    }

exit:

    ry_hal_spi_flash_enter_powerdown();
    ry_hal_spi_flash_uninit();

    cur_spi_flash_frq = SPI_FLASH_READ_FREQUENCY;

    ry_hal_irq_restore(r);

    return result_status;
}

//*****************************************************************************
//
//! @brief Erases the entire contents of the external flash
//!
//! @param ui32DeviceNumber - Device number of the external flash
//!
//! This function uses the "Bulk Erase" instruction to erase the entire
//! contents of the external flash.
//
//*****************************************************************************
void
ry_hal_spi_flash_mass_erase(void)
{
	uint8_t txbuf[4];
   // am_hal_iom_buffer(1) psCommand;

    //
    // Send the write-enable command to prepare the external flash for program
    // operations.
    //
    u32_t r = ry_hal_irq_disable();
    ry_hal_spi_flash_init();
 
	 txbuf[0] = AM_DEVICES_SPIFLASH_WREN;
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);

    //
    // Wait for the write enable latch status bit.
    //
    while ( !(ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WEL) );

    //
    // Send the bulk erase command.
    //	 				 
	 txbuf[0] = AM_DEVICES_SPIFLASH_BE;
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);


    //
    // Wait for status to indicate that the write is no longer in progress.
    //
    while ( ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WIP );

    ry_hal_spi_flash_enter_powerdown();
    ry_hal_spi_flash_uninit();
    ry_hal_irq_restore(r);

    return;
}

//*****************************************************************************
//
//! @brief Erases the contents of a single sector of flash
//!
//! @param ui32DeviceNumber - Device number of the external flash
//! @param ui32SectorAddress - Address to erase in the external flash
//!
//! This function erases a single sector of the external flash as specified by
//! ui32EraseAddress. The entire sector where ui32EraseAddress will
//! be erased.
//
//*****************************************************************************
void
ry_hal_spi_flash_sector_erase(uint32_t ui32SectorAddress)
{
   // am_hal_iom_buffer(4) psCommand;
	uint8_t txbuf[4];
    u32_t r = ry_hal_irq_disable();

    cur_spi_flash_frq = SPI_FLASH_READ_FREQUENCY;
	ry_hal_spi_flash_init();
    //
    // Send the write-enable command to prepare the external flash for program
    // operations.
    //
    txbuf[0] = AM_DEVICES_SPIFLASH_WREN;
    /*am_hal_iom_spi_write(g_psIOMSettings->ui32IOMModule,
                         g_psIOMSettings->ui32ChipSelect,
                         psCommand.words, 1, AM_HAL_IOM_RAW);*/
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);

    //
    // Wait for the write enable latch status bit.
    //
    while ( !(ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WEL) );

    //
    // Prepare the sector erase command, followed by the three-byte external
    // flash address.
    //
    txbuf[0] = AM_DEVICES_SPIFLASH_SE;
	txbuf[1] = (ui32SectorAddress & 0x00FF0000) >> 16;
	txbuf[2] = (ui32SectorAddress & 0x0000FF00) >> 8;
    txbuf[3] = ui32SectorAddress & 0x000000FF;

    //
    // Send the command to erase the desired sector.
    //
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,4);

    //
    // Wait for status to indicate that the write is no longer in progress.
    //
    while ( ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WIP );

    ry_hal_spi_flash_enter_powerdown();
    ry_hal_spi_flash_uninit();
    ry_hal_irq_restore(r);

    return;
}


void
ry_hal_spi_flash_chip_erase(void)
{
	uint8_t txbuf[4];
   // am_hal_iom_buffer(1) psCommand;

    //
    // Send the write-enable command to prepare the external flash for program
    // operations.
    //
    u32_t r = ry_hal_irq_disable();
    ry_hal_spi_flash_init();
	txbuf[0] = AM_DEVICES_SPIFLASH_WREN;
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);

    //
    // Wait for the write enable latch status bit.
    //
    while ( !(ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WEL) );

    //
    // Send the bulk erase command.
    //	 				 
	 txbuf[0] = AM_DEVICES_SPIFLASH_CE;
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);


    //
    // Wait for status to indicate that the write is no longer in progress.
    //
    while ( ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WIP );
    ry_hal_spi_flash_uninit();
    ry_hal_irq_restore(r);

    return;
}
void
ry_hal_spi_flash_config(uint8_t config)
{
	uint8_t txbuf[4];
   // am_hal_iom_buffer(1) psCommand;

    //
    // Send the write-enable command to prepare the external flash for program
    // operations.
    //
    u32_t r = ry_hal_irq_disable();
    ry_hal_spi_flash_init();
	txbuf[0] = AM_DEVICES_SPIFLASH_WREN;
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);

    //
    // Wait for the write enable latch status bit.
    //
    while ( !(ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WEL) );

    //
    // Send the bulk erase command.
    //	 				 
	 txbuf[0] = config;
	ry_hal_spi_txrx(SPI_IDX_FLASH,txbuf,0,1);


    //
    // Wait for status to indicate that the write is no longer in progress.
    //
    while ( ry_hal_spi_flash_status() & AM_DEVICES_SPIFLASH_WIP );
    ry_hal_spi_flash_uninit();
    ry_hal_irq_restore(r);

    return;
}




//int test_count = 0;
ry_sts_t ry_hal_sector_write_exflash(u32_t sector, u8_t* buf, int count)
{
	ry_sts_t status = RY_SUCC;
	u32_t real_sector = FS_START_SECTOR + sector;
	u32_t cur_setcor = real_sector;
	for (cur_setcor = real_sector ; cur_setcor < (real_sector + count) ; cur_setcor++) {

        u32_t r = ry_hal_irq_disable();
	    ry_hal_spi_flash_sector_erase(cur_setcor * EXFLASH_SECTOR_SIZE);
	
		ry_hal_spi_flash_write(&buf[(cur_setcor - real_sector) * EXFLASH_SECTOR_SIZE] , cur_setcor * EXFLASH_SECTOR_SIZE ,  EXFLASH_SECTOR_SIZE);

        /*if((cur_setcor - FS_START_SECTOR) < 6){

            test_count++;
            
            ry_hal_spi_flash_sector_erase((cur_setcor - 16) * EXFLASH_SECTOR_SIZE);
            
            ry_hal_spi_flash_write(&buf[(cur_setcor - real_sector) * EXFLASH_SECTOR_SIZE] , (cur_setcor - 16) * EXFLASH_SECTOR_SIZE ,  EXFLASH_SECTOR_SIZE);
        }*/
    
        ry_hal_irq_restore(r);
	}

	
	return status;
}




ry_sts_t ry_hal_sector_read_exflash(u32_t sector, u8_t* buf, int count)
{
	ry_sts_t status = RY_SUCC;
	u32_t real_sector = FS_START_SECTOR + sector;
	u32_t cur_setcor = real_sector;
	for (cur_setcor = real_sector ; cur_setcor < (real_sector + count) ; cur_setcor++) {
	    u32_t r = ry_hal_irq_disable();
		ry_hal_spi_flash_read(&buf[(cur_setcor - real_sector) * EXFLASH_SECTOR_SIZE] , cur_setcor * EXFLASH_SECTOR_SIZE ,  EXFLASH_SECTOR_SIZE);
        ry_hal_irq_restore(r);

	}
	return status;
}





