/******************************************************************************
 *
 * @file ble_uart_irq.c
 *
 * @brief UART IRQ handler for communication with GDI
 *
 *
 ******************************************************************************/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "board.h"
//#include "types.h"
#include "gup.h"
#include "freeRTOS.h"
#include "queue.h"


#define MAX_BUF                   10
#define MAX_LEN                   100

#define BLE_UART_IDLE             0
#define BLE_UART_RX_GUP           1

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)


static uint8_t uartBuff[MAX_BUF][MAX_LEN];
static uint8_t index      = 0;
static uint8_t uartStatus = BLE_UART_IDLE;

extern QueueHandle_t gup_queue;


void BLE_UART_HAND(void)
{
    uint8_t bytes;
    bool bFullmsg = false;
    int  bytesToRead = 0;

    if ((Chip_UART_GetStatus(BLE_UART_PORT) & UART_STAT_RXRDY) != 0) {
        Chip_UART_ClearStatus(BLE_UART_PORT, UART_STAT_RXRDY);

        if (uartStatus == BLE_UART_IDLE) {
            bytes = Chip_UART_ReadBlocking(BLE_UART_PORT, uartBuff[index], 4);
            if ((uartBuff[index][0] == LO_UINT16(GUP_SOF)) && 
                (uartBuff[index][1] == HI_UINT16(GUP_SOF))) {

                /* OK. It's a GUP packet */
                uartStatus = BLE_UART_RX_GUP;
                bytesToRead = uartBuff[index][2] | (uartBuff[index][3]<<8);
                bytes = Chip_UART_ReadBlocking(BLE_UART_PORT, &uartBuff[index][4], bytesToRead);
                if (bytes == bytesToRead) {
                    bFullmsg = true;
                    uartStatus = BLE_UART_IDLE;
                }
                
            } else {
                /* Not a GUP packet, drop */
                uartStatus = BLE_UART_IDLE;
            }
        }
    }

    if (bFullmsg) {
        void *pGupMsg = uartBuff[index];
        BaseType_t xHigherPriorityTaskWokenByPost;

        /* Post to FreeRTOS Queue */
        xQueueSendFromISR(gup_queue, &pGupMsg, &xHigherPriorityTaskWokenByPost);

        /* Prepare for next msg */
        if (++index >= MAX_BUF) {
            index = 0;
        }

        /* Switching Task if needed */
        portEND_SWITCHING_ISR(xHigherPriorityTaskWokenByPost);
        
    }
    
}


