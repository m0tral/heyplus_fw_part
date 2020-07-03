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

/* Platform specified */
#include "am_bsp.h"
#include "am_bsp_gpio.h"
#include "am_mcu_apollo.h"

/*
 * TYPES
 *******************************************************************************
 */

ry_hal_int_cb_t  pn80t_int_cb  = NULL;
ry_hal_int_cb_t  em9304_int_cb = NULL;
ry_hal_int_cb_t  hrm_int_cb    = NULL;
ry_hal_int_cb_t  dc_in_int_cb  = NULL;
ry_hal_int_cb_t  rx_in_int_cb  = NULL;
/*********************************************************************
 * LOCAL VARIABLES
 */
 
/**
 * @brief   API to init specific GPIO
 *
 * @param   idx  - The specified pin
 * @param   dir  - input or output mode
 *
 * @return  None
 */
void ry_hal_gpio_init(ry_gpio_t idx, u32_t dir)
{
	switch (idx) {
        case GPIO_IDX_NFC_VEN:
		    am_bsp_pin_enable(PN80T_VEN);
			am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_PN80T_VEN);
			am_hal_gpio_out_bit_replace(AM_BSP_GPIO_PN80T_VEN, 0);
		    break;

	    case GPIO_IDX_NFC_DNLD:
		    am_bsp_pin_enable(PN80T_DNLD);
			am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_PN80T_DNLD);
			am_hal_gpio_out_bit_replace(AM_BSP_GPIO_PN80T_DNLD, 0);
		    break;

	    case GPIO_IDX_NFC_IRQ:
		    am_bsp_pin_enable(PN80T_INT);
			am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_RISING);
            am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT));
            am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT));
            am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);
		    break;

#if (HW_VER_CURRENT == HW_VER_SAKE_01)

        case GPIO_IDX_NFC_BAT_EN:
            am_bsp_pin_enable(NFC_BAT_EN);
            am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_NFC_BAT_EN);
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_BAT_EN, 0);
            break;

        case GPIO_IDX_NFC_5V_EN:
            am_bsp_pin_enable(NFC_5V_EN);
            am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_NFC_5V_EN);
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_5V_EN, 0);
            break;

        case GPIO_IDX_SYS_5V_EN:
            am_bsp_pin_enable(SYS_5V_EN);
            break;
#endif

        case GPIO_IDX_NFC_ESE_PWR_REQ:
            am_bsp_pin_enable(NFC_ESE_PWR_REQ);
            am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_NFC_ESE_PWR_REQ);
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_ESE_PWR_REQ, 0);
            break;

        case GPIO_IDX_NFC_18V_EN:
            am_bsp_pin_enable(NFC_18V_EN);
            am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_NFC_18V_EN);
#if  (HW_VER_CURRENT == HW_VER_SAKE_03)            
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_18V_EN, 0);
#else
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_18V_EN, 1);
#endif            
            break;

        case GPIO_IDX_NFC_WKUP_REQ:
            am_bsp_pin_enable(NFC_WAKEUP_REQ);
            break;
        
        

#if ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
        case GPIO_IDX_NFC_SW:
            am_bsp_pin_enable(NFC_SW);
            am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_NFC_SW);
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_SW, 0);
            break;
#endif 

        case GPIO_IDX_HRM_EN:
            am_bsp_pin_enable(HRM_EN);
            am_hal_gpio_out_enable_bit_clear(AM_BSP_GPIO_HRM_EN);
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_HRM_EN, 0);
            break;

        case GPIO_IDX_HRM_RESET:
            am_bsp_pin_enable(HRM_RESET);
            am_hal_gpio_out_enable_bit_set(AM_BSP_GPIO_HRM_RESET);
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_HRM_RESET, 1);
            break;


	    default:
		    break;
	}
}

/**
 * @brief   API to set a GPIO to High
 *
 * @param   idx  - The specified pin
 * @param   val  - The level to be set to the GPIO
 *
 * @return  None
 */
void ry_hal_gpio_set(ry_gpio_t idx, u32_t val)
{
	switch (idx) {
		case GPIO_IDX_NFC_VEN:
		    am_hal_gpio_out_bit_replace(AM_BSP_GPIO_PN80T_VEN, val);
		    break;

		case GPIO_IDX_NFC_DNLD:
		    am_hal_gpio_out_bit_replace(AM_BSP_GPIO_PN80T_DNLD, val);
		    break;

#if (HW_VER_CURRENT == HW_VER_SAKE_01)

        case GPIO_IDX_NFC_BAT_EN:
		    am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_BAT_EN, val);
		    break;

        case GPIO_IDX_NFC_5V_EN:
		    am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_5V_EN, val);
		    break;

        case GPIO_IDX_SYS_5V_EN:
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_SYS_5V_EN, val);
            break;
#endif

        case GPIO_IDX_NFC_18V_EN:
		    am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_18V_EN, val);
		    break;

        

        case GPIO_IDX_NFC_ESE_PWR_REQ:
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_ESE_PWR_REQ, val);
		    break;
        

#if ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))
        case GPIO_IDX_NFC_SW:
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_NFC_SW, val);
            break;
#endif 

        case GPIO_IDX_HRM_EN:
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_HRM_EN, val);
            break;

        case GPIO_IDX_HRM_RESET:
            am_hal_gpio_out_bit_replace(AM_BSP_GPIO_HRM_RESET, val);
            break;


	    default:
		    break; 
	}
}

/**
 * @brief   API to get a GPIO level
 *
 * @param   idx  - The specified pin
 *
 * @return  Level of GPIO
 */
u32_t ry_hal_gpio_get(ry_gpio_t idx)
{
    switch (idx) {
        case GPIO_IDX_NFC_IRQ:
            return (am_hal_gpio_input_read() & AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT))? 1:0;

        case GPIO_IDX_TOUCH_IRQ:
            return am_hal_gpio_input_bit_read(AM_BSP_GPIO_TP_INT) ? 1 : 0;
            
	    default:
		    break; 
    }
    
    return 0;
}

/**
 * @brief   API to enable GPIO Interrupt
 *
 * @param   idx  - The specified pin
 * @param   cb   - Callback of the interrupt
 *
 * @return  None
 */
void ry_hal_gpio_int_enable(ry_gpio_t idx, ry_hal_int_cb_t cb)
{
    #if 1
	switch (idx) {
	    case GPIO_IDX_NFC_IRQ:
			am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_RISING);
            am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT));
            am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT));
			pn80t_int_cb = cb;
		    break;

		case GPIO_IDX_EM9304_IRQ:
			am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_EM9304_INT, AM_HAL_GPIO_RISING);
            am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_EM9304_INT));
            am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_EM9304_INT));
			em9304_int_cb = cb;
		    break;

        case GPIO_IDX_HRM_IRQ:
            am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_HRM_IRQ, AM_HAL_GPIO_RISING);
            am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_HRM_IRQ));
            am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_HRM_IRQ));
            break;

        case GPIO_IDX_DC_IN_IRQ:
            break;

	    default:
		    break;
	}
    #endif
}

void ry_hal_gpio_registing_int_cb(ry_gpio_t idx, ry_hal_int_cb_t cb)
{
    switch (idx) {
	    case GPIO_IDX_NFC_IRQ:
			pn80t_int_cb = cb;
		    break;

		case GPIO_IDX_EM9304_IRQ:
			em9304_int_cb = cb;
		    break;

        case GPIO_IDX_HRM_IRQ:
            hrm_int_cb = cb;
            break;

        case GPIO_IDX_DC_IN_IRQ:
            dc_in_int_cb = cb;
            break;

        case GPIO_IDX_UART_RX:
            rx_in_int_cb = cb;
            break;

	    default:
		    break;
	}
}


/**
 * @brief   API to disable GPIO Interrupt
 *
 * @param   idx  - The specified pin
 *
 * @return  None
 */
void ry_hal_gpio_int_disable(ry_gpio_t idx)
{
    switch (idx) {
	    case GPIO_IDX_NFC_IRQ:
			am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_PN80T_INT, AM_HAL_GPIO_RISING);
            am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT));
            am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT));
		    break;

	    default:
		    break;
	}
}

//*****************************************************************************
//
// Interrupt handler for the GPIO pins.
//
//*****************************************************************************
void am_gpio_isr(void)
{
    uint64_t ui64Status;

    //
    // Read and clear the GPIO interrupt status.
    //
    ui64Status = am_hal_gpio_int_status_get(false);
    am_hal_gpio_int_clear(ui64Status);

    //
    // 1. should first check if the interrupt is DC_in, DC_in should process first
    // 2. Check to see if this was a wakeup event from the BLE radio.
    //
    if (ui64Status & AM_HAL_GPIO_BIT(AM_BSP_GPIO_DC_IN)){
        if (dc_in_int_cb) {
            dc_in_int_cb();
        }
    }

    if ( ui64Status & AM_HAL_GPIO_BIT(AM_BSP_GPIO_EM9304_INT) ) {
        //wsfMsgHdr_t  *pMsg;
        //if ( (pMsg = WsfMsgAlloc(0)) != NULL ) {
        //    WsfMsgSend(g_bleDataReadyHandlerId, pMsg);
        //}
        if (em9304_int_cb) {
		    em9304_int_cb();
        }
    }

    if (ui64Status & AM_HAL_GPIO_BIT(AM_BSP_GPIO_TP_INT)){
        extern void touch_isr_handler();
        touch_isr_handler();
    }

    if (ui64Status & AM_HAL_GPIO_BIT(AM_BSP_GPIO_PN80T_INT)) {
        if (pn80t_int_cb) {
            pn80t_int_cb();
        }
	}

    

    if (ui64Status & AM_HAL_GPIO_BIT(PPS_INTERRUPT_PIN)){
        extern void hrm_gpio_isr_task(void);
        hrm_gpio_isr_task();
    }


    if (ui64Status & AM_HAL_GPIO_BIT(AM_BSP_GPIO_TOOL_UART_RX)) {
        if (rx_in_int_cb) {
            rx_in_int_cb();
        }
	}
    //
    // Call the individual pin interrupt handlers for any pin that triggered an
    // interrupt.
    //

}


