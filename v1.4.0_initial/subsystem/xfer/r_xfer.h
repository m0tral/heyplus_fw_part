/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __R_XFER_H__
#define __R_XFER_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "ryos.h"

/*
 * CONSTANTS
 *******************************************************************************
 */


//#define R_XFER_STATISTIC_ENABLE


/**
 * @brief  Definition for Max Reliable Transfer Module number
 */
#define MAX_R_XFER_INSTANCE_NUM          4  


/**
 * @brief  Definition for Max Reliable Transfer Module number
 */

#define R_XFER_INST_ID_BLE_SEC           0
#define R_XFER_INST_ID_BLE_UNSEC         1
#define R_XFER_INST_ID_BLE_JSON          2
#define R_XFER_INST_ID_UART              3


/**
 * @brief Definitaion for GTM moudle error code base.
 */
#define R_XFER_MODE_CMD                  0
#define R_XFER_MODE_ACK                  1
#define R_XFER_MODE_CRITICAL             0x7F   //紧急命令 仅一条 适用于APP被杀后台,

/**
 * @brief  Definition for Transfer flag
 */
#define R_XFER_FLAG_TX                0x1
#define R_XFER_FLAG_RX                0x2
#define R_XFER_FLAG_DISCONNECT        0x4
#define R_XFER_FLAG_MONITOR           0x8
#define R_XFER_FLAG_TIMEOUT           0x10

#define R_XFER_FLAG_UNKNOWN           0x20  // some new flag


typedef enum {
	DEV_RAW = 0x00,
	DEV_CERT,
	DEV_MANU_CERT,
	DEV_PUBKEY,
	DEV_SIGNATURE,
	DEV_SHARE_INFO
} fctrl_cmd_t;


typedef enum {
	A_SUCCESS = 0x00,
	A_READY,
	A_BUSY,
	A_TIMEOUT,
	A_CANCEL,
	A_LOST
} fctrl_ack_t;



/*
 * TYPES
 *******************************************************************************
 */

/**
 * @brief  Type definition of interrupt callback function.
 */
typedef ry_sts_t (*hw_write_fn_t)(u8_t* data, u32_t len, void* usr_data);

typedef void (*r_xfer_read_fn_t)(u8_t* data, u32_t len, void* usr_data);
typedef void (*r_xfer_ioctl_fn_t)(void);
typedef void (*r_xfer_recv_cb_t)(u8_t* p_data, u32_t length, void* usr_data);

typedef void (*r_xfer_send_cb_t)(ry_sts_t status, void* usr_data);



/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init reliable transfer module.
 *
 * @param   None
 *
 * @return  None
 */
void r_xfer_init(void);



// APIs for lower transfer channel.
void r_xfer_on_receive(uint8_t* pdata, uint16_t len, uint8_t isSecure);
void r_xfer_on_send_cb(void* usrdata);

void r_xfer_onBleReceived(u8_t* data, int len, u16_t charUUID);
void r_xfer_onUartReceived(u8_t* data, int len);

// APIs for upper layer modules
ry_sts_t r_xfer_instance_add(int instID, hw_write_fn_t write, r_xfer_recv_cb_t readCb, void* usrdata);
ry_sts_t r_xfer_send(int instID, u8_t* data, int len, r_xfer_send_cb_t sendCb, void* usrdata);
int r_xfer_getLastRxDuration(int instID);
void r_xfer_BleDisconnectHandler(void);

ryos_thread_t* r_xfer_get_thread_handle(void);
u32_t r_xfer_get_diagnostic(void);

ryos_thread_t* r_xfer_get_thread(void);
void r_xfer_msg_send(u32_t msg, int len, void* data);



#endif  /* __R_XFER_H__ */
