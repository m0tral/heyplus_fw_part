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

#include "ry_hal_inc.h"

/* Platform Dependency */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "ry_utils.h"

#include "am_hal_i2c_bit_bang.h"

/*
 * CONSTANTS
 *******************************************************************************
 */
#define IOM_MODULE          0 // This will have a side benefit of testing IOM4 in offset mode

// We need to shift one extra
#define GET_I2CADDR(cfg)    \
(((cfg) & AM_REG_IOSLAVE_CFG_I2CADDR_M) >> (AM_REG_IOSLAVE_CFG_I2CADDR_S + 1))

/* I2C slave address to communicate with */
#define IOS_ADDRESS         0x20
#define IOM_MODULE          0 // This will have a side benefit of testing IOM4 in offset mode

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
  u32_t ios_addr;
} handle_i2cm_t;


/*
 * Global variables
 *******************************************************************************
 */
handle_i2cm_t handle_i2cm[I2C_IDX_MAX];
u8_t touch_i2c_enable = 0;;

//
// IOM Queue Memory
//
// am_hal_iom_queue_entry_t g_psQueueMemory[32];

ryos_mutex_t* i2c_mutex;
static int is_i2c_initialized = 0;

//*****************************************************************************
//
// I2C Slave Configuration
//
//*****************************************************************************
am_hal_ios_config_t g_sIOSI2cConfig =
{
    // Configure the IOS in I2C mode.
    .ui32InterfaceSelect = AM_HAL_IOS_USE_I2C | AM_HAL_IOS_I2C_ADDRESS(IOS_ADDRESS),

    // Eliminate the "read-only" section, so an external host can use the
    // entire "direct write" section.
    .ui32ROBase = 0x78,

    // Set the FIFO base to the maximum value, making the "direct write"
    // section as big as possible.
    .ui32FIFOBase = 0x80,

    // We don't need any RAM space, so extend the FIFO all the way to the end
    // of the LRAM.
    .ui32RAMBase = 0x100,
    // FIFO Threshold - set to half the size
    .ui32FIFOThreshold = 0x40,
};

//*****************************************************************************
//
// I2C Master Configuration
//
//*****************************************************************************
static am_hal_iom_config_t g_sIOM0_I2cConfig =
{
    .ui32InterfaceMode = AM_HAL_IOM_I2CMODE,
    .ui32ClockFrequency = AM_HAL_IOM_1MHZ,
    .ui8WriteThreshold = 12,
    .ui8ReadThreshold = 120,
};

//*****************************************************************************
//
// I2C Master Configuration
//
//*****************************************************************************
static am_hal_iom_config_t g_sIOM1_I2cConfig =
{
    .ui32InterfaceMode = AM_HAL_IOM_I2CMODE,
    .ui32ClockFrequency = AM_HAL_IOM_400KHZ,
    .ui8WriteThreshold = 12,
    .ui8ReadThreshold = 120,
};

//*****************************************************************************
//
// I2C Master Configuration
//
//*****************************************************************************
static am_hal_iom_config_t g_sIOM2_I2cConfig =
{
    .ui32InterfaceMode = AM_HAL_IOM_I2CMODE,
    .ui32ClockFrequency = AM_HAL_IOM_1MHZ,
    .ui8WriteThreshold = 12,
    .ui8ReadThreshold = 120,
};

//*****************************************************************************
//
// I2C Master Configuration
//
//*****************************************************************************
static am_hal_iom_config_t g_sIOM3_I2cConfig =
{
    .ui32InterfaceMode = AM_HAL_IOM_I2CMODE,
    .ui32ClockFrequency = AM_HAL_IOM_200KHZ,
    .ui8WriteThreshold = 20,
    .ui8ReadThreshold = 4,
};

/*
 * FUNCTIONS
 *******************************************************************************
 */
ry_sts_t ry_hal_i2c_TestMasterI2C(uint8_t cmd);

//
//! Take over default ISR for IOM 0. (Queue mode service)
//
#if 0   //this function muti-defined at am_devices_em9304
void
am_iomaster0_isr(void)
{
    uint32_t ui32Status;
    ui32Status = am_hal_iom_int_status_get(IOM_MODULE, true);
    am_hal_iom_int_clear(IOM_MODULE, ui32Status);
    am_hal_iom_queue_service(IOM_MODULE, ui32Status);
}
#endif


//*****************************************************************************
//
// Configure the IOS as I2C slave.
//
//*****************************************************************************
static void
ios_set_up(void)
{
  // Configure I2C interface
    am_hal_gpio_pin_config(0, AM_HAL_PIN_0_SLSCL);
    am_hal_gpio_pin_config(1, AM_HAL_PIN_1_SLSDA);

    //
    // Configure the IOS interface and LRAM structure.
    //
    am_hal_ios_config(&g_sIOSI2cConfig);

    //
    // Clear out any IOS register-access interrupts that may be active, and
    // enable interrupts for the registers we're interested in.
    //
    am_hal_ios_access_int_clear(AM_HAL_IOS_ACCESS_INT_ALL);
    am_hal_ios_int_clear(AM_HAL_IOS_INT_ALL);

    //
    // Set the bit in the NVIC to accept access interrupts from the IO Slave.
    //
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOSACC);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOSLAVE);

    // Set up the IOSINT interrupt pin
    //am_hal_gpio_pin_config(4, AM_HAL_PIN_4_SLINT);
}


/**
 * @brief   API to init and enable specified I2C master module
 *
 * @param   idx  - The specified i2c instance
 *
 * @return  None
 */
void ry_hal_i2cm_init(ry_i2c_t idx)
{
    if (is_i2c_initialized == 0) {
        ryos_mutex_create(&i2c_mutex);
        is_i2c_initialized = 1;
    }

    if (idx == I2C_GSENSOR){
        handle_i2cm[idx].iom_module = 1;
        handle_i2cm[idx].ios_addr = 0x5b;
        am_hal_gpio_out_bit_set(8);                                         // Set pins high to prevent bus dips.  
        am_hal_gpio_out_bit_set(9);

        //am_bsp_pin_enable i2c function and pullup
        am_hal_gpio_pin_config(8, AM_HAL_PIN_8_M1SCL | AM_HAL_GPIO_PULLUP);
        am_hal_gpio_pin_config(9, AM_HAL_PIN_9_M1SDA | AM_HAL_GPIO_PULLUP);

        am_hal_iom_pwrctrl_enable(handle_i2cm[idx].iom_module);              // Enable power to IOM.
        am_hal_iom_config(handle_i2cm[idx].iom_module, &g_sIOM1_I2cConfig);  // Set the required configuration settings for the IOM.

        am_hal_iom_int_enable(handle_i2cm[idx].iom_module, 0xFF);
        am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOMASTER0 + handle_i2cm[idx].iom_module);
        am_bsp_iom_enable(handle_i2cm[idx].iom_module);                      // Turn on the IOM for this operation.
    }
    else if (idx == I2C_IDX_HR){
        handle_i2cm[idx].iom_module = 2;
        handle_i2cm[idx].ios_addr = 0x5b;
        am_hal_gpio_out_bit_set(0);                                        // Set pins high to prevent bus dips.  
        am_hal_gpio_out_bit_set(1);

        //am_bsp_pin_enable i2c function and pullup
        am_hal_gpio_pin_config(0, AM_HAL_PIN_0_M2SCL | AM_HAL_GPIO_PULLUP);
        am_hal_gpio_pin_config(1, AM_HAL_PIN_1_M2SDA | AM_HAL_GPIO_PULLUP);

        am_hal_iom_pwrctrl_enable(handle_i2cm[idx].iom_module);            // Enable power to IOM.
        am_hal_iom_config(handle_i2cm[idx].iom_module, &g_sIOM2_I2cConfig);// Set the required configuration settings for the IOM.

        am_hal_iom_int_enable(handle_i2cm[idx].iom_module, 0xFF);
        am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOMASTER0 + handle_i2cm[idx].iom_module);
        am_bsp_iom_enable(handle_i2cm[idx].iom_module);                    // Turn on the IOM for this operation.
    }
    else if (idx == I2C_TOUCH){
        handle_i2cm[idx].iom_module = 3;
        handle_i2cm[idx].ios_addr = 0x08;
		// if (idx == I2C_TOUCH_BTLDR)
		 handle_i2cm[I2C_TOUCH_BTLDR].iom_module = 0x03;
		 handle_i2cm[I2C_TOUCH_BTLDR].ios_addr = 0x18;	 
        am_hal_gpio_out_bit_set(42);                                        // Set pins high to prevent bus dips.  
        am_hal_gpio_out_bit_set(43);

        //am_bsp_pin_enable i2c function and pullup
        am_hal_gpio_pin_config(42, AM_HAL_PIN_42_M3SCL | AM_HAL_GPIO_PULLUP);
        am_hal_gpio_pin_config(43, AM_HAL_PIN_43_M3SDA | AM_HAL_GPIO_PULLUP);

        am_hal_iom_pwrctrl_enable(handle_i2cm[idx].iom_module);            // Enable power to IOM.
        am_hal_iom_config(handle_i2cm[idx].iom_module, &g_sIOM3_I2cConfig);// Set the required configuration settings for the IOM.

        //am_hal_iom_int_enable(handle_i2cm[idx].iom_module, 0xFF);
        //am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOMASTER0 + handle_i2cm[idx].iom_module);
        am_bsp_iom_enable(handle_i2cm[idx].iom_module);                    // Turn on the IOM for this operation.    
        touch_i2c_enable = 1;
    }    
    else if (idx == I2C_IDX_NFC){        
        handle_i2cm[idx].iom_module = AM_HAL_IOM_I2CBB_MODULE;
        handle_i2cm[idx].ios_addr = 0x28;
        am_hal_i2c_bit_bang_init(8, 9);
    } else if (idx == I2C_IDX_NFC_HW) {

        handle_i2cm[idx].iom_module = 1;
        handle_i2cm[idx].ios_addr = 0x28;

        am_hal_gpio_out_bit_set(8);                                        // Set pins high to prevent bus dips.  
        am_hal_gpio_out_bit_set(9);

        //am_bsp_pin_enable i2c function and pullup
        am_hal_gpio_pin_config(8, AM_HAL_PIN_8_M1SCL | AM_HAL_GPIO_PULLUP);
        am_hal_gpio_pin_config(9, AM_HAL_PIN_9_M1SDA | AM_HAL_GPIO_PULL1_5K);

        am_hal_iom_pwrctrl_enable(handle_i2cm[idx].iom_module);            // Enable power to IOM.
        am_hal_iom_config(handle_i2cm[idx].iom_module, &g_sIOM1_I2cConfig);// Set the required configuration settings for the IOM.

        am_hal_iom_int_enable(handle_i2cm[idx].iom_module, 0xFF);
        am_hal_interrupt_enable(AM_HAL_INTERRUPT_IOMASTER0 + handle_i2cm[idx].iom_module);
        am_bsp_iom_enable(handle_i2cm[idx].iom_module);  
        
    }
    //am_hal_interrupt_master_enable();                                       // Enable Interrupts.
   //ry_hal_i2c_TestMasterI2C(1);
}


u8_t ry_hal_is_i2cm_touch_enable(void)
{
    return touch_i2c_enable;
}


void ry_hal_i2cm_powerdown(ry_i2c_t idx)
{

}


void ry_hal_i2cm_uninit(ry_i2c_t idx)
{
    if (is_i2c_initialized == 0) {
        ryos_mutex_create(&i2c_mutex);
        is_i2c_initialized = 1;
    }

   if (idx == I2C_IDX_HR){
        handle_i2cm[idx].iom_module = 2;
        handle_i2cm[idx].ios_addr = 0x5b;
        am_hal_gpio_out_bit_set(0);                                        // Set pins high to prevent bus dips.  
        am_hal_gpio_out_bit_set(1);

        //am_bsp_pin_enable i2c function and pullup
        //am_hal_gpio_pin_config(0, AM_HAL_PIN_0_M2SCL | AM_HAL_GPIO_PULLUP);
        //am_hal_gpio_pin_config(1, AM_HAL_PIN_1_M2SDA | AM_HAL_GPIO_PULLUP);

        am_hal_gpio_pin_config(0, AM_HAL_PIN_DISABLE);
        am_hal_gpio_pin_config(1, AM_HAL_PIN_DISABLE);

        am_hal_iom_pwrctrl_disable(handle_i2cm[idx].iom_module);            // Enable power to IOM.
       // am_hal_iom_config(handle_i2cm[idx].iom_module, &g_sIOM2_I2cConfig);// Set the required configuration settings for the IOM.
		
        am_hal_iom_int_disable(handle_i2cm[idx].iom_module, 0xFF);
        am_hal_interrupt_disable(AM_HAL_INTERRUPT_IOMASTER0 + handle_i2cm[idx].iom_module);
      //  am_bsp_iom_enable(handle_i2cm[idx].iom_module);                    // Turn on the IOM for this operation.
    }
    else if (idx == I2C_TOUCH){
        handle_i2cm[idx].iom_module = 3;
        handle_i2cm[idx].ios_addr = 0x08;
    //    am_hal_gpio_out_bit_set(42);                                        // Set pins high to prevent bus dips.  
     //   am_hal_gpio_out_bit_set(43);

        //am_bsp_pin_enable i2c function and pullup
        am_hal_gpio_pin_config(42, AM_HAL_PIN_42_M3SCL | AM_HAL_GPIO_PULLUP);
        am_hal_gpio_pin_config(43, AM_HAL_PIN_43_M3SDA | AM_HAL_GPIO_PULLUP);

        am_hal_iom_pwrctrl_disable(handle_i2cm[idx].iom_module);            // Enable power to IOM.
     //   am_hal_iom_config(handle_i2cm[idx].iom_module, &g_sIOM3_I2cConfig);// Set the required configuration settings for the IOM.

        am_hal_iom_int_disable(handle_i2cm[idx].iom_module, 0xFF);
        am_hal_interrupt_disable(AM_HAL_INTERRUPT_IOMASTER0 + handle_i2cm[idx].iom_module);
        touch_i2c_enable = 0;
    }    
}

/**
 * @brief   API to send I2C data
 *
 * @param   idx   - The specified i2c instance
 * @param   pData - Pointer to the TX data to be sent
 * @param   len   - Length of buffer
 *
 * @return  Status
 */
ry_sts_t ry_hal_i2cm_tx(ry_i2c_t idx, u8_t* pData, u32_t len)
{
    am_hal_iom_status_e status;

    //u32_t r = ry_hal_irq_disable();
    

    //ryos_mutex_lock(i2c_mutex);
    status = am_hal_iom_i2c_write(handle_i2cm[idx].iom_module, handle_i2cm[idx].ios_addr,
                             (uint32_t *)pData, len, AM_HAL_IOM_RAW);

    //ryos_mutex_unlock(i2c_mutex);
    //ry_board_debug_printf("i2c tx %x\r\n", status);
    //ry_hal_irq_restore(r);
    return (status == AM_HAL_IOM_SUCCESS) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_I2C, status);  
    
}


/**
 * @brief   API to receive I2C data
 *
 * @param   idx   - The specified i2c instance
 * @param   pData - Pointer to put the received RX data
 * @param   len   - Length of buffer
 *
 * @return  Status
 */
ry_sts_t ry_hal_i2cm_rx(ry_i2c_t idx, u8_t* pData, u32_t len)
{      
    am_hal_iom_status_e status;

    //u32_t r = ry_hal_irq_disable();
    
    //ryos_mutex_lock(i2c_mutex);
    status = am_hal_iom_i2c_read(handle_i2cm[idx].iom_module, handle_i2cm[idx].ios_addr,
                             (uint32_t *)pData, len, AM_HAL_IOM_RAW);
    //ryos_mutex_unlock(i2c_mutex);

    //ry_hal_irq_restore(r);

    //ry_board_debug_printf("i2c rx %x\r\n", status);
    return (status == AM_HAL_IOM_SUCCESS) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_I2C, status);
}

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
ry_sts_t ry_hal_i2cm_tx_at_addr(ry_i2c_t idx, uint8_t reg_addr, u8_t* pData, u32_t len)
{
    am_hal_iom_status_e status;
    u32_t r;

    //r = ry_hal_irq_disable();
    //ry_board_debug_printf("i2c txA\r\n");

    ryos_mutex_lock(i2c_mutex);
    r = ry_hal_irq_disable();
    status = am_hal_iom_i2c_write(handle_i2cm[idx].iom_module, handle_i2cm[idx].ios_addr,
                             (uint32_t *)pData, len, AM_HAL_IOM_OFFSET(reg_addr));
    ry_hal_irq_restore(r);
    ryos_mutex_unlock(i2c_mutex);

    //ry_hal_irq_restore(r);
    
    return (status == AM_HAL_IOM_SUCCESS) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_I2C, RY_ERR_WRITE);
}

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
ry_sts_t ry_hal_i2cm_rx_at_addr(ry_i2c_t idx, uint8_t reg_addr, u8_t* pData, u32_t len)
{      
    am_hal_iom_status_e status;
    u32_t r;

    
    //ry_board_debug_printf("i2c rxA\r\n");
    ryos_mutex_lock(i2c_mutex);
    r = ry_hal_irq_disable();
    status = am_hal_iom_i2c_read(handle_i2cm[idx].iom_module, handle_i2cm[idx].ios_addr,
                             (uint32_t *)pData, len, AM_HAL_IOM_OFFSET(reg_addr));

    ry_hal_irq_restore(r);
    ryos_mutex_unlock(i2c_mutex);
    return (status == AM_HAL_IOM_SUCCESS) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_I2C, RY_ERR_READ);
}


/* [] END OF FILE */



