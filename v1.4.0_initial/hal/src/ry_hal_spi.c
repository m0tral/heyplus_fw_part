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

#include <ry_hal_inc.h>

/* Platform Dependency */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
/*
 * CONSTANTS
 *******************************************************************************
 */
#define SPI_TIMEOUT_THRESH 		1000	//unit us

/*
 * TYPEDEFS
 *******************************************************************************
 */
/**
 * @brief pwm components structure definition
 */
typedef struct
{
  u32_t iom_module;
  u32_t chip_select;
} handle_spi_t;

handle_spi_t handle_spi[SPI_IDX_MAX];

ryos_mutex_t* spi_mutex;
static int is_spi_initialized = 0;

/*
 * VARIABLES
 *******************************************************************************
 */
//static u8_t spi_module_oled = 0x0;

//*****************************************************************************
//
// Configuration structure for the IO Master.
//
//*****************************************************************************
am_hal_iom_config_t g_sIOM0Config =
{
    .ui32InterfaceMode = AM_HAL_IOM_SPIMODE,
    .ui32ClockFrequency = AM_HAL_IOM_24MHZ,
    .bSPHA = 1,
    .bSPOL = 1,
    .ui8WriteThreshold = 4,
    .ui8ReadThreshold = 60,
};

//*****************************************************************************
//
// Configuration structure for the IO Master.
//
//*****************************************************************************
const am_hal_iom_config_t g_sIOM4Config =
{
    .ui32InterfaceMode = AM_HAL_IOM_SPIMODE,
    .ui32ClockFrequency = AM_HAL_IOM_24MHZ,
    .bSPHA = 0,
    .bSPOL = 0,
    .ui8WriteThreshold = 20,
    .ui8ReadThreshold = 20,
};

/**
 * @brief   API to init and enable specified SPI master module
 *
 * @param   idx  - The specified spi instance
 *
 * @return  None
 */
#if SPI_LOCK_DEBUG
void ry_hal_spim_init(ry_spi_t idx, const char* caller)
#else 
void ry_hal_spim_init(ry_spi_t idx)
#endif
{
    if (is_spi_initialized == 0) {
        ryos_mutex_create(&spi_mutex);
        is_spi_initialized = 1;
    }
    
	if (SPI_IDX_GSENSOR == idx){
        ryos_mutex_lock(spi_mutex);
#if SPI_LOCK_DEBUG
        LOG_DEBUG("[ry_hal_spim_init] spi mutex lock. %s\r\n", caller);
#endif
		handle_spi[idx].iom_module = 0;		//iom0 module
		handle_spi[idx].chip_select = 6;	//cs n
		am_hal_gpio_pin_config(5, AM_HAL_PIN_5_M0SCK);
		am_hal_gpio_pin_config(6, AM_HAL_PIN_6_M0MISO);
		am_hal_gpio_pin_config(7, AM_HAL_PIN_7_M0MOSI);
		am_hal_gpio_pin_config(10, AM_HAL_PIN_10_M0nCE6);
        
        am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_HAL_PIN_OUTPUT);
        am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_FL_CS);
		g_sIOM0Config.ui32ClockFrequency = AM_HAL_IOM_2MHZ;
		
		am_hal_iom_pwrctrl_enable(handle_spi[idx].iom_module);		
		am_hal_iom_config(handle_spi[idx].iom_module, &g_sIOM0Config);
		am_bsp_iom_spi_pins_enable(handle_spi[idx].iom_module);		
		am_bsp_iom_enable(handle_spi[idx].iom_module);
	}
	else if (SPI_IDX_OLED == idx){
		handle_spi[idx].iom_module = 4;		//iom4 module
		handle_spi[idx].chip_select = 6;	//cs n
		am_hal_gpio_pin_config(39, AM_HAL_PIN_39_M4SCK);
		// am_hal_gpio_pin_config(40, AM_HAL_PIN_40_M4MISO);
		am_hal_gpio_pin_config(44, AM_HAL_PIN_44_M4MOSI);
		am_hal_gpio_pin_config(35, AM_HAL_PIN_35_M4nCE6);

		am_hal_iom_pwrctrl_enable(handle_spi[idx].iom_module);		
		am_hal_iom_config(handle_spi[idx].iom_module, &g_sIOM4Config);
		am_bsp_iom_spi_pins_enable(handle_spi[idx].iom_module);		
		am_bsp_iom_enable(handle_spi[idx].iom_module);
	}
	else if (SPI_IDX_FLASH == idx){
        ryos_mutex_lock(spi_mutex);
#if SPI_LOCK_DEBUG
        LOG_DEBUG("[ry_hal_spim_init] spi mutex lock. %s\r\n", caller);
#endif

		handle_spi[idx].iom_module = 0;		//iom4 module
		handle_spi[idx].chip_select = 0;	//cs n
		am_hal_gpio_pin_config(5, AM_HAL_PIN_5_M0SCK);
		am_hal_gpio_pin_config(6, AM_HAL_PIN_6_M0MISO);
		am_hal_gpio_pin_config(7, AM_HAL_PIN_7_M0MOSI);
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_BSP_GPIO_CFG_IOM0_FL_CS);

        am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_G_CS, AM_HAL_GPIO_OUTPUT);		//10
		am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_G_CS);	

        extern u32_t cur_spi_flash_frq;
		if((cur_spi_flash_frq != SPI_FLASH_READ_FREQUENCY)&&\
		    (cur_spi_flash_frq != SPI_FLASH_WRITE_FREQUENCY)){
		    
            cur_spi_flash_frq = SPI_FLASH_WRITE_FREQUENCY;
		}
		g_sIOM0Config.ui32ClockFrequency = cur_spi_flash_frq;

		am_hal_iom_pwrctrl_enable(handle_spi[idx].iom_module);		
		am_hal_iom_config(handle_spi[idx].iom_module, &g_sIOM0Config);
		am_bsp_iom_spi_pins_enable(handle_spi[idx].iom_module);		
		am_bsp_iom_enable(handle_spi[idx].iom_module);
	}
}

void ry_hal_spim_flash_specific_init(void)
{
  handle_spi[SPI_IDX_FLASH].iom_module = 0;		//iom4 module
	handle_spi[SPI_IDX_FLASH].chip_select = 0;	//cs n
	am_hal_gpio_pin_config(5, AM_HAL_PIN_5_M0SCK);
	am_hal_gpio_pin_config(6, AM_HAL_PIN_6_M0MISO);
	am_hal_gpio_pin_config(7, AM_HAL_PIN_7_M0MOSI);
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_BSP_GPIO_CFG_IOM0_FL_CS);

    am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_G_CS, AM_HAL_GPIO_OUTPUT);		//10
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_G_CS);	
	
	extern u32_t cur_spi_flash_frq;
	if((cur_spi_flash_frq != SPI_FLASH_READ_FREQUENCY)&&\
	    (cur_spi_flash_frq != SPI_FLASH_WRITE_FREQUENCY)){
	    
        cur_spi_flash_frq = SPI_FLASH_WRITE_FREQUENCY;
	}
	g_sIOM0Config.ui32ClockFrequency = cur_spi_flash_frq;
	//g_sIOM0Config.ui32ClockFrequency = AM_HAL_IOM_24MHZ;

	am_hal_iom_pwrctrl_enable(handle_spi[SPI_IDX_FLASH].iom_module);		
	am_hal_iom_config(handle_spi[SPI_IDX_FLASH].iom_module, &g_sIOM0Config);
	am_bsp_iom_spi_pins_enable(handle_spi[SPI_IDX_FLASH].iom_module);		
	am_bsp_iom_enable(handle_spi[SPI_IDX_FLASH].iom_module);
}


#if SPI_LOCK_DEBUG
void ry_hal_spim_uninit(ry_spi_t idx, const char* caller)
#else 
void ry_hal_spim_uninit(ry_spi_t idx)
#endif
{
    /*if (is_spi_initialized == 0) {
        ryos_mutex_create(&spi_mutex);
        is_spi_initialized = 1;
    }*/
    
	if (SPI_IDX_GSENSOR == idx){
		handle_spi[idx].iom_module = 0;		//iom0 module
		handle_spi[idx].chip_select = 6;	//cs n
		am_hal_iom_pwrctrl_disable(handle_spi[idx].iom_module);		
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_G_CS, AM_HAL_GPIO_OUTPUT);		//10
		am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_G_CS);	
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_DISABLE);		//6
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MOSI, AM_HAL_GPIO_OUTPUT);	//7
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_MOSI);
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_SCK, AM_HAL_GPIO_OUTPUT);		//5
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_SCK);
#if SPI_LOCK_DEBUG
        LOG_DEBUG("[ry_hal_spim_uninit] spi mutex unlock. %s\r\n", caller);
#endif

        ryos_mutex_unlock(spi_mutex);
	}
	else if (SPI_IDX_OLED == idx){
		handle_spi[idx].iom_module = 4;		//iom4 module
		handle_spi[idx].chip_select = 6;	//cs n
		am_hal_iom_pwrctrl_disable(handle_spi[idx].iom_module);	
		
		am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_DISABLE);		//35
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_SCK, AM_HAL_GPIO_DISABLE);		//39
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_MOSI, AM_HAL_GPIO_DISABLE);		//44
	}
	else if (SPI_IDX_FLASH == idx){
		handle_spi[idx].iom_module = 0;		//iom4 module
		handle_spi[idx].chip_select = 0;	//cs n
		am_hal_iom_pwrctrl_disable(handle_spi[idx].iom_module);		
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_FL_CS, AM_HAL_GPIO_OUTPUT);	//11
		am_hal_gpio_out_bit_set(AM_BSP_GPIO_IOM0_FL_CS);		
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MISO, AM_HAL_GPIO_DISABLE);	//6
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_MOSI, AM_HAL_GPIO_OUTPUT);	//7
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_MOSI);
		am_hal_gpio_pin_config(AM_BSP_GPIO_IOM0_SCK, AM_HAL_GPIO_OUTPUT);	//5
		am_hal_gpio_out_bit_clear(AM_BSP_GPIO_IOM0_SCK);
#if SPI_LOCK_DEBUG
        LOG_DEBUG("[ry_hal_spim_uninit] spi mutex unlock. %s\r\n", caller);
#endif

        ryos_mutex_unlock(spi_mutex);
	}
}


/**
 * @brief	function to do SPI "read & Write" Test
 *
 * @param	null
 *
 * @return	status - result of SPI read or wirte
 *			         0: RY_SUCC, else: fail
 */
ry_sts_t ry_hal_spi_testTxRx(void)
{
  ry_sts_t status = RY_ERR_INIT_DEFAULT;
 
  return status;
}

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
ry_sts_t ry_hal_spi_txrx(ry_spi_t idx, u8_t* pTxData, u8_t* pRxData, u32_t len)
{
	ry_sts_t status = RY_ERR_INIT_DEFAULT;
	u32_t timeout = SPI_TIMEOUT_THRESH;

    //ryos_mutex_lock(spi_mutex);
    
    u32_t r = ry_hal_irq_disable();

    uint32_t option = AM_HAL_IOM_RAW;
    
	if (NULL == pRxData){
	    if(idx == SPI_IDX_FLASH){
            option |= AM_HAL_IOM_CS_LOW;
	    }
		status = am_hal_iom_spi_write(handle_spi[idx].iom_module, handle_spi[idx].chip_select, (u32_t*)pTxData, len, option);
	}
	else if ((NULL != pTxData) && (NULL != pTxData)){
        option = AM_HAL_IOM_OFFSET(*pTxData);
		status = am_hal_iom_spi_read(handle_spi[idx].iom_module, handle_spi[idx].chip_select, (u32_t*)pRxData, len - 1, option);
	}

    

    ry_hal_irq_restore(r);
    //ryos_mutex_unlock(spi_mutex);
    
	return status;
}

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
ry_sts_t ry_hal_spi_write_read(ry_spi_t idx, u8_t* pTxData, u8_t* pRxData, u32_t len, uint32_t option)
{
	ry_sts_t status = RY_ERR_INIT_DEFAULT;
	u32_t timeout = SPI_TIMEOUT_THRESH;

    u32_t r = ry_hal_irq_disable();

    //ryos_mutex_lock(spi_mutex);
    
	if (NULL == pRxData){
		status = am_hal_iom_spi_write(handle_spi[idx].iom_module, handle_spi[idx].chip_select, (u32_t*)pTxData, len, AM_HAL_IOM_RAW | option);
	}
	else if ((NULL != pTxData) && (NULL != pTxData)){
		if (AM_HAL_IOM_RAW & option){
			status = am_hal_iom_spi_read(handle_spi[idx].iom_module, handle_spi[idx].chip_select, (u32_t*)pRxData, len, option);
		}
		else{
			status = am_hal_iom_spi_read(handle_spi[idx].iom_module, handle_spi[idx].chip_select, (u32_t*)pRxData, len - 1, AM_HAL_IOM_OFFSET(*pTxData));
		}
	}

    //ryos_mutex_unlock(spi_mutex);

    ry_hal_irq_restore(r);
	return status;
}


/* [] END OF FILE */
