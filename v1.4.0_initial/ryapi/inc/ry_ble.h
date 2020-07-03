/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_BLE_H__
#define __RY_BLE_H__

#include "ry_type.h"
#include "rbp.pb.h"


#define PEER_PHONE_TYPE_ANDROID            1
#define PEER_PHONE_TYPE_IOS                2


#define RY_BLE_CTRL_MSG_ADV_START                      0x01
#define RY_BLE_CTRL_MSG_ADV_STOP                       0x02
#define RY_BLE_CTRL_MSG_DISCONNECT                     0x03
#define RY_BLE_CTRL_MSG_CONNECT                        0x04
#define RY_BLE_CTRL_MSG_CONN_UPDATE                    0x05
#define RY_BLE_CTRL_MSG_RESET                          0x06
#define RY_BLE_CTRL_MSG_CONN_UPDATE_WITH_CFG           0x07
#define RY_BLE_CTRL_MSG_WTF1                           0x08
#define RY_BLE_CTRL_MSG_WTF2                           0x09

#define RY_BLE_ADV_TYPE_NORMAL                         0x01  // adv interval = 1s
#define RY_BLE_ADV_TYPE_WAIT_CONNECT                   0x02  // adv interval = 200ms
#define RY_BLE_ADV_TYPE_SLOW                           0x03  // adv interval = 3s
#define RY_BLE_ADV_TYPE_X1                             0x04
#define RY_BLE_ADV_TYPE_X2                             0x05
#define RY_BLE_ADV_TYPE_X3                             0x06

#define RY_BLE_CONN_PARAM_SUPER_FAST                   0x01  //trigger by some user action
#define RY_BLE_CONN_PARAM_FAST                         0x02  //request by ry_ble 
#define RY_BLE_CONN_PARAM_MEDIUM                       0x03  //no slave latency
#define RY_BLE_CONN_PARAM_SLOW                         0x04  //very low power cost

#define PEER_PHONE_APP_LIFECYCLE_UNKOWN                0
#define PEER_PHONE_APP_LIFECYCLE_FOREGROUND            1
#define PEER_PHONE_APP_LIFECYCLE_BACKGROUND            2
#define PEER_PHONE_APP_LIFECYCLE_DESTROYED             3


typedef enum {
    RY_BLE_TYPE_REQUEST,
    RY_BLE_TYPE_RESPONSE,
    RY_BLE_TYPE_INDICAT,
	RY_BLE_TYPE_RX_MONITOR,
	RY_BLE_TYPE_TX_MONITOR,
} ry_ble_req_type_t;



/**
 * @brief Definitaion for RY_BLE state.
 */
typedef enum {
    RY_BLE_STATE_IDLE,
    RY_BLE_STATE_INITIALIZED,
//    RY_BLE_STATE_ADV,
    RY_BLE_STATE_CONNECTED,
    RY_BLE_STATE_DISCONNECTED,
} ry_ble_state_t;



/**
 * @brief Definitaion connection parameters of one connection
 */
typedef struct {
    uint16_t connId;
    uint16_t connInterval;
    uint16_t timeout;
    uint16_t slaveLatency;
    uint8_t  peerAddr[6];
    uint8_t  peerPhoneInfo;
    uint8_t  peerAppLifecycle;
    uint32_t mtu;
} ry_ble_conn_para_t;


typedef struct {
    uint32_t msgType;
    uint32_t len;
    uint8_t  data[1];
} ry_ble_ctrl_msg_t;


/**
 * @brief Definitaion for ble stack
 */
typedef void (*ry_ble_init_done_t)(void);


/**
 * @brief Definitaion for TX callback
 */
typedef void (*ry_ble_send_cb_t)(ry_sts_t status, void* usr_data);




/**
 * @brief   API to init BLE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_ble_init(ry_ble_init_done_t initDoneCb);

/**
 * @brief   API to get BLE status
 *
 * @param   None
 *
 * @return  None
 */
uint8_t ry_ble_get_state(void);

/**
 * @brief   API to get current connect intervals
 *
 * @param   None
 *
 * @return  None
 */
uint16_t ry_ble_get_connInterval(void);

/**
 * @brief   API to get peer phone type
 *
 * @param   None
 *
 * @return  None
 */
uint8_t ry_ble_get_peer_phone_type(void);

/**
 * @brief   API to get peer phone type
 *
 * @param   None
 *
 * @return  None
 */
void ry_ble_set_peer_phone_type(uint8_t type);

ry_sts_t ry_ble_set_phone_info(uint8_t * data, uint32_t size);

void ry_ble_set_peer_phone_app_lifecycle(uint8_t type);

uint8_t ry_ble_get_peer_phone_app_lifecycle(void);

uint32_t ry_ble_get_cur_mtu(void);

/**
 * @brief   API to send RBP packet request
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_ble_txMsgReq(CMD cmd, ry_ble_req_type_t type, uint32_t rspCode, uint8_t* data, uint32_t len, uint8_t isSec, ry_ble_send_cb_t cb, void* arg);

uint8_t ryble_idle_timer_is_running(void);
void ry_ble_idle_timer_start(void);
void ry_ble_idle_timer_stop(void);
void ry_ble_connect_normal_mode(void);
u8_t ry_ble_is_connect_idle_mode(void);
void ry_ble_ctrl_msg_send(uint32_t msg_type, int len, void* data);


void ry_ble_msg_send(uint8_t* data, uint32_t len, void* usrdata, uint8_t fTxMsg);




#endif  /* __RY_BLE_H__ */

