/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
/*
* Copyright (c), GDI Inc.
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
#include "ry_hal_uart.h"
#include "ryos.h"


#if (BSP_MCU_TYPE == BSP_APOLLO2)

/* bsp specified */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_bsp_gpio.h"
#include "am_util.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "am_hal_uart.h"


/*********************************************************************
 * CONSTANTS
 */ 

/* 
 * @brief Transfer LOG Marco
 */
#define UART_TRANSFER_LOG_ENABLE

/* 
 * @brief Uart Pin Definition
 */
//#define GUP_UART_PORT                     BLE_UART_PORT //LPC_USART1
//#define GUP_UART_BAUDRATE                 (115200)
//#define GUP_UART_IRQN                     BLE_UART_IRQN
//#define GUP_UART_HAND                     BLE_UART_HAND

/* 
 * @brief Uart Pin Definition
 */
//#define GUP_UART_TX_PORT                  BLE_UTXD_PORT
//#define GUP_UART_TX_PIN                   BLE_UTXD_PIN

//#define GUP_UART_RX_PORT                  BLE_URXD_PORT
//#define GUP_UART_RX_PIN                   BLE_URXD_PIN

/* 
 * @brief GPIO IRQ Pin Definition
 */
//#define GUP_UART_IRQ_PORT                 0 //PORT_UART_INT
//#define GUP_UART_IRQ_PIN                  8 //PIN_UART_INT

/* 
 * @brief Transfer related marco definition
 */
//#define MAX_BUF                           10
//#define MAX_LEN                           100

//#define BLE_UART_IDLE                     0
//#define BLE_UART_RX_GUP                   1

/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */

//static u8_t uartBuff[MAX_BUF][MAX_LEN];
//static u8_t index      = 0;
//static u8_t uartStatus = BLE_UART_IDLE;

#define UART_BUFFER_SIZE    (128 * 2 + 100)
uint8_t g_pui8UartRxBuffer[UART_BUFFER_SIZE];
uint8_t g_pui8UartTxBuffer[UART_BUFFER_SIZE];
uint8_t g_ui8RxTimeoutFlag = 0;
uint8_t g_ui8BufferFullFlag = 0;


ryos_mutex_t* uart_mutex;
static int is_uart_initialized = 0;
ry_uart_rx_cb_t uart_rx_cb;


/* 
 * @brief Uart Configuration Settings
 */
static am_hal_uart_config_t gdi_uart_cfg =
{
    .ui32BaudRate = 115200,
    .ui32DataBits = AM_HAL_UART_DATA_BITS_8,
    .bTwoStopBits = false,
    .ui32Parity   = AM_HAL_UART_PARITY_NONE,
    .ui32FlowCtrl = AM_HAL_UART_FLOW_CTRL_NONE,
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */


/**
 * @brief   API to init Diagnostic module
 *
 * @param   None
 *
 * @return  None
 */
static void _uart_init(uint32_t ui32Module, uint8_t isBuffered)
{
    //
    // Power on the selected UART
    //
    am_hal_uart_pwrctrl_enable(ui32Module);

    //
    // Start the UART interface, apply the desired configuration settings, and
    // enable the FIFOs.
    //
    am_hal_uart_clock_enable(ui32Module);

    //
    // Disable the UART before configuring it.
    //
    am_hal_uart_disable(ui32Module);

    //
    // Configure the UART.
    //
    am_hal_uart_config(ui32Module, &gdi_uart_cfg);

    //
    // Configure the UART FIFO.
    //
    am_hal_uart_fifo_config(ui32Module, AM_HAL_UART_TX_FIFO_1_2 | AM_HAL_UART_RX_FIFO_1_2);
    

    //
    // Initialize the UART queues.
    //
    if (isBuffered) {
        am_hal_uart_init_buffered(ui32Module, g_pui8UartRxBuffer, UART_BUFFER_SIZE,
                                  g_pui8UartTxBuffer, UART_BUFFER_SIZE);
    }
}





/**
 * @brief   Enable Uart
 *
 * @param   None
 *
 * @return  None
 */
void _uart_enable(uint32_t ui32Module)
{
    //
    // Enable the UART clock.
    //
    am_hal_uart_clock_enable(ui32Module);

    //
    // Enable the UART.
    //
    am_hal_uart_enable(ui32Module);
    am_hal_uart_int_enable(ui32Module, AM_HAL_UART_INT_RX_TMOUT |
                                       AM_HAL_UART_INT_RX |
                                       AM_HAL_UART_INT_TXCMP);

    //
    // Enable the UART pins.
    //
    //am_bsp_pin_enable(BLE_UART_RX);
    //am_bsp_pin_enable(BLE_UART_TX);

    am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_UART + ui32Module, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_UART + ui32Module);
}

/**
 * @brief   Disable Uart
 *
 * @param   None
 *
 * @return  None
 */
static void _uart_disable(uint32_t ui32Module)
{
    //
    // Clear all interrupts before sleeping as having a pending UART interrupt
    // burns power.
    //
    am_hal_uart_int_clear(ui32Module, 0xFFFFFFFF);

    //
    // Disable the UART.
    //
    am_hal_uart_disable(ui32Module);

    //
    // Disable the UART pins.
    //
    //am_bsp_pin_disable(BLE_UART_RX);
    //am_bsp_pin_disable(BLE_UART_TX);

    //
    // Disable the UART clock.
    //
    am_hal_uart_clock_disable(ui32Module);

    am_hal_interrupt_disable(AM_HAL_INTERRUPT_UART + ui32Module);
}


/**
 * @brief   Transmit delay waits for busy bit to clear to allow
 *          for a transmission to fully complete before proceeding
 *
 * @param   None
 *
 * @return  None
 */
static void uart_transmit_delay(int32_t i32Module)
{
    //
    // Wait until busy bit clears to make sure UART fully transmitted last byte
    //
    while ( am_hal_uart_flags_get(i32Module) & AM_HAL_UART_FR_BUSY );
}




/**
 * @brief   API to init UART module
 *
 * @param   idx - which uart to be init.
 * @param   cb  - rx callback function. Note: the receive buffer.
 *
 * @return  None
 */
void ry_hal_uart_init(ry_uart_t idx, ry_uart_rx_cb_t cb)
{
    if (is_uart_initialized == 0) {
        ryos_mutex_create(&uart_mutex);
        is_uart_initialized = 1;
    }
    
    if (idx == UART_IDX_LOG) {
        //
        // Make sure the UART RX and TX pins are enabled.
        //
        //am_bsp_pin_enable(DBG_UART_RX);
        //am_bsp_pin_enable(DBG_UART_TX);

        /* Init Uart Module */
        //_uart_init(AM_BSP_UART_DBG_INST, 0);
        //am_hal_uart_enable(AM_BSP_UART_DBG_INST);

    } else if (idx == UART_IDX_TOOL) {
        //
        // Make sure the UART RX and TX pins are enabled.
        //
        am_bsp_pin_enable(TOOL_UART_RX);
        am_bsp_pin_enable(TOOL_UART_TX);

        /* Init Uart Module */
        _uart_init(AM_BSP_UART_TOOL_INST, 1);
        am_hal_uart_enable(AM_BSP_UART_TOOL_INST);

        uart_rx_cb = cb;
    }
}


/**
 * @brief   API to send UART data
 *
 * @param   buf - Buffer to be sent through UART
 * @param   len - Length of buffer
 *
 * @return  None
 */
static void _uart_send(uint32_t ui32Module, u8_t* buf, u32_t len)
{
	extern void am_hal_uart_buffer_transmit_polled(uint32_t,u8_t*,u32_t);
    /* Send data through UART */
    am_hal_uart_buffer_transmit_polled(ui32Module, buf, len);

    /* LOG */
#ifdef UART_TRANSFER_LOG_ENABLE
    LOG_DEBUG("[gup]: > ");
    ry_data_dump(buf, len, ' ');
#endif
}

/**
 * @brief   API to power up specified UART module
 *
 * @param   idx  - The specified uart instance
 *
 * @return  None
 */
void ry_hal_uart_powerup(ry_uart_t idx)
{
    ryos_mutex_lock(uart_mutex);
    
    if (idx == UART_IDX_LOG) {
        //
        // Make sure the UART RX and TX pins are enabled.
        //
        //am_bsp_pin_enable(DBG_UART_RX);
        //am_bsp_pin_enable(DBG_UART_TX);

        /* Enable Uart Module */
        //_uart_init(AM_BSP_UART_DBG_INST, 0);
        //am_hal_uart_enable(AM_BSP_UART_DBG_INST);
        //_uart_enable(AM_BSP_UART_DBG_INST);
    } else if (idx == UART_IDX_TOOL) {
        
        //
        // Make sure the UART RX and TX pins are enabled.
        //
        am_bsp_pin_enable(TOOL_UART_RX);
        am_bsp_pin_enable(TOOL_UART_TX);

        /* Init Uart Module */
        _uart_enable(AM_BSP_UART_TOOL_INST);
    }

    ryos_mutex_unlock(uart_mutex);
}

/**
 * @brief   API to power down specified UART module
 *
 * @param   idx  - The specified uart instance
 *
 * @return  None
 */
void ry_hal_uart_powerdown(ry_uart_t idx)
{
    //ryos_mutex_lock(uart_mutex);
    
    if (idx == UART_IDX_LOG) {
        //
        // Make sure the UART RX and TX pins are enabled.
        //
        //am_bsp_pin_disable(DBG_UART_RX);
        //am_bsp_pin_disable(DBG_UART_TX);

        /* Disable Uart Module */
        //_uart_disable(AM_BSP_UART_DBG_INST);
    } else if (idx == UART_IDX_TOOL) {
        
        //
        // Make sure the UART RX and TX pins are enabled.
        //
        am_bsp_pin_disable(TOOL_UART_RX);
        am_bsp_pin_disable(TOOL_UART_TX);

        /* Init Uart Module */
        _uart_disable(AM_BSP_UART_TOOL_INST);
    }

    //ryos_mutex_unlock(uart_mutex);
}


/**
 * @brief   API to send UART data
 *
 * @param   idx - Which port to send data.
 * @param   buf - Buffer to be sent through UART
 * @param   len - Length of buffer
 *
 * @return  None
 */
void ry_hal_uart_tx(ry_uart_t idx, u8_t* buf, u32_t len)
{
    ryos_mutex_lock(uart_mutex);
    
    if (idx == UART_IDX_LOG) {
        _uart_send(AM_BSP_UART_DBG_INST, buf, len);
    } else {
        _uart_send(AM_BSP_UART_TOOL_INST, buf, len);
    }

    ryos_mutex_unlock(uart_mutex);
}

/**
 * @brief   API to polling receive UART data
 *
 * @param   idx - Which port to send data.
 * @param   buf - Buffer to put received UART data
 * @param   len - Length of buffer
 *
 * @return  None
 */
void ry_hal_uart_rx(ry_uart_t idx, u8_t* buf, u32_t* len)
{
    if (idx == UART_IDX_TOOL) {
        *len = am_hal_uart_char_receive_buffered(AM_BSP_UART_TOOL_INST, (char*)buf, UART_BUFFER_SIZE);
    }

    /* LOG */
#ifdef UART_TRANSFER_LOG_ENABLE
    LOG_DEBUG("[gup]: < ");
    ry_data_dump(buf, *len, ' ');
#endif

    
}

/**
 * @brief   UART recevie IRQ
 *
 * @param   None
 *
 * @return  None
 */
void am_uart1_isr(void)
{
    uint32_t status;
    uint32_t rxSize, txSize;
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    status = am_hal_uart_int_status_get(1, false);

    am_hal_uart_int_clear(1, status);

    if (status & (AM_HAL_UART_INT_RX_TMOUT | AM_HAL_UART_INT_TX | AM_HAL_UART_INT_RX))
    {
        am_hal_uart_service_buffered_timeout_save(1, status);
    }

    if (status & (AM_HAL_UART_INT_RX_TMOUT))
    {
        g_ui8RxTimeoutFlag = 1;

        /* Send received message to GUP queue */
        //if (!(xQueueSendToBackFromISR(gup_queue, &g_pui8UartRxBuffer, &xHigherPriorityTaskWoken))) {
            
        //} else {
        //    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        //}
        //ry_board_debug_printf("TMOUT, size:%d\n", rxSize);

        if (uart_rx_cb) {
            uart_rx_cb(UART_IDX_TOOL);
        }
    }
    am_hal_uart_get_status_buffered(1, &rxSize, &txSize);
    if (status & (AM_HAL_UART_INT_RX))
    {
        ry_board_debug_printf("RX ");
    }
    if (status & (AM_HAL_UART_INT_TXCMP))
    {
        //LOG_DEBUG("TXCMP\n");
    }
    am_hal_uart_get_status_buffered(1, &rxSize, &txSize);
    if (rxSize >= UART_BUFFER_SIZE / 2)
    {
        g_ui8BufferFullFlag = 1;
        //LOG_DEBUG("RXF\n");
    }
    
}

/**
 * @brief   UART recevie IRQ
 *
 * @param   None
 *
 * @return  None
 */
void am_uart0_isr(void)
{

    uint32_t status;
    uint32_t rxSize, txSize;

    status = am_hal_uart_int_status_get(0, false);

    am_hal_uart_int_clear(0, status);

    if (status & (AM_HAL_UART_INT_RX_TMOUT | AM_HAL_UART_INT_TX | AM_HAL_UART_INT_RX))
    {
        am_hal_uart_service_buffered_timeout_save(1, status);
    }

    if (status & (AM_HAL_UART_INT_RX_TMOUT))
    {
        g_ui8RxTimeoutFlag = 1;
        LOG_DEBUG("TMOUT\n");
    }
    am_hal_uart_get_status_buffered(0, &rxSize, &txSize);
    if (status & (AM_HAL_UART_INT_RX))
    {
        LOG_DEBUG("RX ");
    }
    if (status & (AM_HAL_UART_INT_TXCMP))
    {
        LOG_DEBUG("TXCMP\n");
    }
    am_hal_uart_get_status_buffered(0, &rxSize, &txSize);
    if (rxSize >= UART_BUFFER_SIZE / 2)
    {
        g_ui8BufferFullFlag = 1;
        LOG_DEBUG("RXF\n");
    }
    
}



#endif /* (BSP_MCU_TYPE == BSP_APOLLO2) */

