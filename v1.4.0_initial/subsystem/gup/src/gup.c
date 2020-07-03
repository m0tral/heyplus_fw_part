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
#include "ryos.h"
#include "ry_hal_uart.h"

/* Application */
#include "gup.h"

/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * CONSTANTS
 */
#define GUP_QUEUE_SIZE              20

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    int status;
    int rxCnt;
    int txCnt;
} gup_ctrl_t;


/*********************************************************************
 * GLOBAL VARIABLES
 */
//extern QueueHandle_t eaci_queue;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static gup_ctrl_t       gup_v;
static ryos_thread_t *gup_thread_handle;
ry_queue_t*            gup_queue;


/*********************************************************************
 * LOCAL VARIABLES
 */

/*
 * calcFcs - populates the Frame Check Sequence of the payload.
 *
 * @param   msg   - pointer to the RPC message
 * @param   size  - size of the message
 *
 * @return  FCS of the message
 */
u8_t calcFcs(u8_t *msg, int size)
{
	u8_t result = 0;
	int idx = 0;
	int len = (size - 1);  // skip FCS

	while ((len--) != 0) {
		result ^= msg[idx++];
	}

	msg[(size-1)] = result;
    return result;
}

/*
 * gup_send - API to send APP UART command.
 *
 * @param   type    - CMD, event or data.
 * @param   cmdID   - Command or event ID.
 * @param   para    - Payload of command and event.
 * @param   paraLen - The length of command and event
 *
 * @return  None.
 */
void gup_send(u8_t type, u8_t cmdID, u8_t* para, u8_t paraLen)
{
    u8_t buf[40];
    volatile u32_t status;
    gup_fmt_t* p = (gup_fmt_t*) buf;
    int i;

    p->sof  = GUP_SOF;
    p->len  = paraLen + 3; /* 3 = evtid + type + FCS */
    p->type = type;
    p->id   = cmdID;

    if (paraLen > 0) {
        memcpy(p->data, para, paraLen);
    }
    calcFcs(buf, p->len+4);

    /* Send data through uart */
    ry_hal_uart_gup_send(buf, p->len+4);
}


/*
 * gup_send - API to send card related command.
 *
 * @param   cardData - The payload of card data.
 * @param   len      - The length of card data.
 *
 * @return  None.
 */
void gup_send_card_cmd(u8_t* cardData, u16_t len)
{
    gup_send(GUP_TYPE_CMD, CMD_CARD_BLE2MCU, cardData, len);
}

/*
 * gup_send - API to send card related command.
 *
 * @param   cardData - The payload of card data.
 * @param   len      - The length of card data.
 *
 * @return  None.
 */
void gup_send_card_evt(u8_t* cardData, u16_t len)
{
    //vTaskDelay(50);
    gup_send(GUP_TYPE_EVT, EVT_CARD_MCU2BLE, cardData, len);
}

/*
 * gup_send_se_init_done_evt - API to send SE init done event.
 *
 * @param   None.
 *
 * @return  None.
 */
void gup_send_se_init_done_evt(void)
{
    gup_send(GUP_TYPE_EVT, EVT_SE_INIT_DONE, NULL, 0);
}


/*
 * gup_send - API to send card related command.
 *
 * @param   cardData - The payload of card data.
 * @param   len      - The length of card data.
 *
 * @return  None.
 */
void gup_send_ack(u8_t cmdID, u8_t status)
{
    u8_t ack[2];
    ack[0] = cmdID;
    ack[1] = status;
    gup_send(GUP_TYPE_EVT, EVT_CMD_COMPLETE, ack, 2);
}

/*
 * gup_cmd_handler - Parse the received GUP command
 *
 * @param   cmdId  - Command ID
 * @param   len    - Length of command payload
 * @param   cmd    - Pointer of received command payload.
 *
 * @return  GUP Status
 */
ry_sts_t gup_cmd_handler(u8_t cmdId, u16_t len, u8_t* cmd)
{
    ry_sts_t status = RY_SUCC;

    switch (cmdId) {
        case CMD_CARD_BLE2MCU:
            //phOsalNfc_LogBle("[gup]: card cmd handler.\r\n");

            //if (xQueueSend(eaci_queue, &cmd, portMAX_DELAY) == pdTRUE) {
                //printf("Send to eaci Queue Success\r\n");
            //} else {
            //    printf("Send to eaci Queue FAIL\r\n");
            //}
            
            break;

        case CMD_NFC_ENABLE:
            LOG_DEBUG("[gup]: nfc enable cmd. \r\n");

            /* Pull high the NFC VEN gpio */

            //Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 20, (IOCON_FUNC0 | IOCON_GPIO_MODE | IOCON_DIGITAL_EN ));
            //Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 20);
            //Chip_GPIO_SetPinState(LPC_GPIO, 0, 20, true);
            
            break;

        case CMD_FP_RESET:
            LOG_DEBUG("[gup]: fp reset cmd handler.\r\n");
            
            /* Stop the current fingerprint operation and clear the RAM */
            break;

        case CMD_FP_SET:
            LOG_DEBUG("[gup]: fp set cmd handler.\r\n");

            /* Start a new fingerprint operation */
            break;

        case CMD_MCU_RESET:
            LOG_DEBUG("[gup]: msu reset cmd handler.\r\n");

            /* Reset the MCU. Make sure all operation be stopped first */
            break;

        case CMD_MCU_STATUS_GET:
            LOG_DEBUG("[gup]: mcu status get cmd handler.\r\n");

            /* Send the status back */
            status = GUP_SUCC_NO_ACK;
            break;

        case CMD_MCU_LOG_SET:
            LOG_DEBUG("[gup]: mcu log set cmd handler.\r\n");
            break;

        default:
            break;
    }

    return status;
}

/*
 * gup_evt_handler - Parse the received GUP event
 *
 * @param   cmdId  - Event ID
 * @param   len    - Length of event payload
 * @param   cmd    - Pointer of received event payload.
 *
 * @return  GUP Status..
 */
ry_sts_t gup_evt_handler(u8_t evtId, u16_t len, u8_t* evt)
{
    ry_sts_t status = RY_SUCC;
    LOG_DEBUG("[gup]: evt handler.\r\n");

    return status;
}

/*
 * gup_data_handler - Parse the received GUP data
 *
 * @param   len    - Length of data payload
 * @param   cmd    - Pointer of received data payload.
 *
 * @return  GUP Status.
 */
ry_sts_t gup_data_handler(u16_t len, u8_t* data)
{
    ry_sts_t status = RY_SUCC;
    LOG_DEBUG("[gup]: data handler.\r\n");
    return status;
}

/*
 * gup_msg_handler - Parse the received GUP message
 *
 * @param   gupMsg - Received GUP message
 *
 * @return  None.
 */
void gup_msg_handler(gup_fmt_t* gupMsg)
{
    gup_fmt_t *p = gupMsg;
    ry_sts_t status = RY_SUCC;
    u8_t fcs;
    u8_t fcs_calc;
    u8_t* raw = (u8_t*)gupMsg;
    int i;

    LOG_DEBUG("[gup]: < ");
    for (i = 0; i < p->len+4; i++) {
        printf("%02x ", raw[i]);
    }
    printf("\r\n");

    /* Verify the fcs */
    fcs = p->data[p->len-3];
    fcs_calc = calcFcs((u8_t*)p, p->len+4);
    if (fcs != fcs_calc) {
        LOG_ERROR("[Gup]: Wrong crc. rcv fcs: %02x, calc fcs: %02x.\r\n", fcs, fcs_calc);
        status = RY_SET_STS_VAL(RY_CID_GUP, RY_ERR_INVALID_FCS);
        goto done;
    }

    /* Dispatch msg to coresponding parser */
    switch (p->type) {
        case GUP_TYPE_CMD:
            status = gup_cmd_handler(p->id, p->len-3, p->data);
            break;

        case GUP_TYPE_EVT:
            status = gup_evt_handler(p->id, p->len-3, p->data);
            break;

        case GUP_TYPE_DATA:
            status = gup_data_handler(p->len-3, p->data);
            break;

        default:
            LOG_ERROR("[Gup]: Invalid type. Drop \r\n");
            status = RY_SET_STS_VAL(RY_CID_GUP, RY_ERR_INVALID_TYPE);
            goto done;
            break;
    }

done:
    if (status !=  RY_SUCC) {
        //gup_send_ack(p->id, status);
    }
}

/*
 * gup_task - GUP task 
 *
 * @param   arg   - Task parameters. 
 *
 * @return  None.
 */
static int gup_task(void *arg) 
{
    gup_fmt_t *pGupMsg = NULL;
    ry_sts_t status = RY_SUCC;

    LOG_INFO("[Gup]: Task Initialized \r\n");

    while (1) {
        
        status = ryos_queue_recv(gup_queue, &pGupMsg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[GUP]: Queue receive error. Status = 0x%x\r\n", status);
        } else {
            gup_msg_handler(pGupMsg);
        }
    }
}

/*
 * gup_init - Initialize the GUP task 
 *
 * @param   None.
 *
 * @return  Status.
 */
ry_sts_t gup_init(void)
{
    ry_sts_t status = RY_SUCC;
    ryos_threadPara_t para;

    /* Init Uart */
    ry_hal_uart_gup_init();

    /* Init the control context */
    memset(&gup_v, 0, sizeof(gup_ctrl_t));

    /* Create the GUP received queue */
    status = ryos_queue_create(&gup_queue, GUP_QUEUE_SIZE, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[GUP]: Queue Create Error.\r\n");
        return status;
    }

    /* Start the GUP task */
    strcpy((char*)para.name, "GUPTask");
    para.stackDepth = 256;
    para.priority   = 2;
    para.arg        = NULL; 
    status = ryos_thread_create(&gup_thread_handle, gup_task, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[GUP]: Task Create Error.\r\n");
        return status;
    }

    return status;

}

/***********************************Test*****************************************/

void gup_test(void)
{
    uint8_t testData[10] = {0};
    int i;

    for (i = 0; i < 10; i++) {
        testData[i] = i;
    }
    
    // Test send card command to MCU
    gup_send_card_cmd(testData, 10);

    //gup_send_ack(0x01, 0x00);
}


