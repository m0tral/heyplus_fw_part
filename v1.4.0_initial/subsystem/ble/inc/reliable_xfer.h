#ifndef __RELIABLE_XFER_H__
#define __RELIABLE_XFER_H__


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
 * @brief Definitaion for GTM moudle error code base.
 */
#define R_XFER_MODE_CMD                               0
#define R_XFER_MODE_ACK                               1
#define PACK_TIMEOUT_MS								  700

/**
 * @brief Definitaion for FLAG of the reliable data, bitmap
 */
#define R_XFER_FLAG_SEC_BIT                           0x01


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

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


typedef enum {
	R_XFER_READY = 0x01,
	R_XFER_BUSY,

	R_XFER_TXD,
	R_XFER_RXD,

	R_XFER_DONE,
	R_XFER_ERROR = 0xFF
} r_xfer_stat_t;

typedef enum {
    R_XFER_SUCC = 0x00,
        
} r_xfer_sts_t;


/*
 * TYPES
 ****************************************************************************************
 */
    

typedef ry_sts_t (*r_xfer_send_t) (uint8_t * p_data, uint16_t length);
typedef void (*r_xfer_recv_cb_t)(uint8_t* p_data, uint32_t length, uint8_t flag);





typedef void (*r_xfer_tx_b_t)(ry_sts_t status, void* usrdata);



/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */



void r_xfer_schedule_init(void);


/*
 * r_xfer_init - API to init reliable transfer module
 *
 * @param   Status
 */
void r_xfer_init(r_xfer_send_t send_func, r_xfer_recv_cb_t recv_cb);



/*
 * r_xfer_on_receive - Handle recevied data by reliable xfer.
 *
 * @param   pdata     - The received data from peer device.
 * @param   len       - The length of received data.
 * @param   isSecure  - Flag to indicate it's a secure channel.
 *
 * @return  None
 */
void r_xfer_on_receive(uint8_t* pdata, uint16_t len, uint8_t isSecure);


/**
 * @brief   API to register Reliable Transfer function
 *
 * @param   send_func - The specified TX function
 *
 * @return  None
 */
void r_xfer_register_tx(r_xfer_send_t send_func);


void r_xfer_tx_recover(void);


void r_xfer_send_start(u8_t* data, int len);



/**
 * @brief   API to send data through reliable protocol.
 *
 * @param   data, the data to be sent
 * @param   len,  the length of data
 * @param   txCb, callback function of transmit status.
 *
 * @return  Status.
 */
void r_xfer_send(u8_t* data, int len, r_xfer_tx_b_t txCb, void* usrdata);




#endif  /* __RELIABLE_XFER_H__ */



