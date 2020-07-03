/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __GUP_H__
#define __GUP_H__


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ry_type.h"


/*
 * DEFINES
 ****************************************************************************************
 */


/**
 * @brief Definitaion SOF uart protocol
 */
#define PORT_UART_INT                                 0
#define PIN_UART_INT                                  8


/**
 * @brief Definitaion SOF uart protocol
 */
#define GUP_SOF                                       0xCE78

/**
 * @brief Definitaion for UART type
 */
#define GUP_TYPE_CMD                                  0x01
#define GUP_TYPE_EVT                                  0x02
#define GUP_TYPE_DATA                                 0x03


/**
 * @brief Definitaion for UART COMMAND ID
 */
#define CMD_CARD_BLE2MCU                              0x01
#define CMD_NFC_ENABLE                                0x02
#define CMD_FP_RESET                                  0x10
#define CMD_FP_SET                                    0x11
#define CMD_MCU_RESET                                 0x20
#define CMD_MCU_STATUS_GET                            0x21
#define CMD_MCU_LOG_SET                               0x22


/**
 * @brief Definitaion for UART EVENT ID
 */
#define EVT_CMD_COMPLETE                              0x00
#define EVT_CARD_MCU2BLE                              0x01
#define EVT_MCU_STATUS                                0x20
#define EVT_MCU_LOG                                   0x22
#define EVT_SE_INIT_DONE                              0x30

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/**
 * @brief Definitaion for UART Status Code
 */
typedef enum {
    GUP_SUCC                                         = 0x00,
    GUP_SUCC_NO_ACK                                  = 0x01,
} gup_sts_t;

/*
 * TYPES
 ****************************************************************************************
 */

/**
 * @brief Definitaion for UART Basic format type
 */
typedef struct {
    u16_t sof;
    u16_t len;
    u8_t  type;
    u8_t  id;
    u8_t  data[1]; 
} gup_fmt_t;


struct gup_rx_data 
{
    u8_t* data;
    int len;
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 * gup_init - Initialize the GUP task 
 *
 * @param   None.
 *
 * @return  Status.
 */
ry_sts_t gup_init(void);


/*
 * gup_pin_init - API to init MCU uart. Currently we use uart1 as default.
 *
 * @param   None
 */
void gup_pin_init(void);

void gup_send_card_evt(uint8_t* cardData, uint16_t len);



void gup_test(void);



#if 0
void app_uart_tx_test(uint8_t* buf, int len);

int app_uart_onRxData(ke_msg_id_t const msgid,
                      struct app_uart_rx_data const *param,
                      ke_task_id_t const dest_id,
                      ke_task_id_t const src_id);

#endif



#endif  /* __GUP_H__ */


