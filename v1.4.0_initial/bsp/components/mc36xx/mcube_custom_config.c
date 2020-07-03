#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "app_config.h"

#include <ry_hal_inc.h>

#include "mcube_sample.h"
#include "mcube_custom_config.h" 
#include "mcube_drv.h"

static uint32_t ui32Module = 4;
static uint32_t ui32Module_scl = 6;

const S_M_DRV_MC_UTIL_OrientationReMap
			 g_MDrvUtilOrientationReMap[E_M_DRV_UTIL_ORIENTATION_TOTAL_CONFIG] =
				  {
					  {{  1,  1,  1 }, { M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_Z }},
					  {{ -1,  1,  1 }, { M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Z }},
					  {{ -1, -1,  1 }, { M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_Z }},
					  {{  1, -1,  1 }, { M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Z }},
					  
					  {{ -1,  1, -1 }, { M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_Z }},
					  {{  1,  1, -1 }, { M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Z }},
					  {{  1, -1, -1 }, { M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_Z }},
					  {{ -1, -1, -1 }, { M_DRV_MC_UTL_AXIS_Y, M_DRV_MC_UTL_AXIS_X, M_DRV_MC_UTL_AXIS_Z }},
				  };


uint8_t mcube_read_reg(uint8_t reg)
{
	uint8_t addr = 0xc0 | reg;
	uint8_t rx_data;

    u32_t r = ry_hal_irq_disable();
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_GSENSOR);   
#endif

	ry_hal_spi_txrx(SPI_IDX_GSENSOR, &addr, &rx_data, 2);

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_uninit(SPI_IDX_GSENSOR);   
#endif
    ry_hal_irq_restore(r);

	return rx_data;
}

uint8_t mcube_write_reg(uint8_t reg, uint8_t data)
{
	uint8_t addr = 0x40 | reg;
	uint8_t tx_data[2] = {addr, data};

    u32_t r = ry_hal_irq_disable();
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_GSENSOR);   
#endif

	ry_hal_spi_txrx(SPI_IDX_GSENSOR, tx_data, NULL, 2);

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_uninit(SPI_IDX_GSENSOR);   
#endif
    ry_hal_irq_restore(r);

	return 0;
}


void mcube_delay_ms(unsigned int ms)
{	
	am_util_delay_ms(ms); 
}

unsigned char mcube_write_regs(unsigned char register_address, unsigned char *value, unsigned char number_of_bytes)
{
	uint8_t addr = 0x40 | register_address;
	uint8_t tx_data[256] = {0};
	tx_data[0] = addr;
	ry_memcpy((tx_data + 1), value, number_of_bytes);
    u32_t r = ry_hal_irq_disable();
    
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_GSENSOR);   
#endif

	ry_hal_spi_txrx(SPI_IDX_GSENSOR, tx_data, NULL, number_of_bytes + 1);

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_uninit(SPI_IDX_GSENSOR);   
#endif

    ry_hal_irq_restore(r);

	return 0;
}

unsigned char mcube_read_regs(unsigned char register_address, unsigned char *destination, unsigned char number_of_bytes)
{
	uint8_t addr = 0xc0 | register_address;

    u32_t r = ry_hal_irq_disable();
    
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_GSENSOR);   
#endif

	ry_hal_spi_txrx(SPI_IDX_GSENSOR, &addr, destination, number_of_bytes + 1);

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_uninit(SPI_IDX_GSENSOR);   
#endif

    ry_hal_irq_restore(r);

	return 0;
}

unsigned long mcube_burst_read(unsigned long address, unsigned char *buf, unsigned long size)
{
	uint8_t addr = 0x40 | address;
    u32_t r = ry_hal_irq_disable();
    
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_GSENSOR);   
#endif

	ry_hal_spi_txrx(SPI_IDX_GSENSOR, &addr, buf, size + 1);

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_GSENSOR, __FUNCTION__);  
#else
    ry_hal_spim_uninit(SPI_IDX_GSENSOR);   
#endif
    ry_hal_irq_restore(r);
	return 0;
}

