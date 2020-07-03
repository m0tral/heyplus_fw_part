/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __BLE_TASK_H__
#define __BLE_TASK_H__

#include "ry_type.h"



/*
 * TYPES
 *******************************************************************************
 */

#define BLE_STACK_EVT_ADV_STARTED              0x0001
#define BLE_STACK_EVT_ADV_STOPED               0x0002
#define BLE_STACK_EVT_CONNECTED                0x0003
#define BLE_STACK_EVT_DISCONNECTED             0x0004
#define BLE_STACK_EVT_CONN_PARA_UPDATED        0x0005
#define BLE_STACK_EVT_MTU_GOT                  0x0006
#define BLE_STACK_EVT_INIT_DONE                0x0007


#define FIT_MSG_START                           0xA0
#define ANCS_MSG_START                          0xB0
#define ALIPAY_MSG_START                        0xC0


/**
 * @brief Definitaion for ble stack
 */
typedef void (*ble_stack_cb_t)(u16_t evt, void* para);


typedef struct {
    uint16_t connID;
    uint16_t connInterval;
    uint16_t timeout;
    uint16_t latency;
    uint8_t  peerAddr[6];
} ble_stack_connected_t;

typedef struct {
    uint16_t connID;
    uint16_t reason;
} ble_stack_disconnected_t;

typedef struct {
    uint16_t connID;
    uint32_t mtu;
} ble_stack_mtu_updated_t;

/*! enumeration of client characteristic configuration descriptors */
enum
{
  FIT_GATT_SC_CCC_IDX,                    /*! GATT service, service changed characteristic */
  FIT_HRS_HRM_CCC_IDX,                    /*! Heart rate service, heart rate monitor characteristic */
  FIT_BATT_LVL_CCC_IDX,                   /*! Battery service, battery level characteristic */
  FIT_RSCS_SM_CCC_IDX,                   /*! Runninc speed and cadence measurement characteristic */
  FIT_RYEEXS_SEC_CH_CCC_IDX,
  FIT_RYEEXS_UNSEC_CH_CCC_IDX,
  FIT_RYEEXS_JSON_CH_CCC_IDX,
  FIT_MIS_TOKEN_CCC_IDX,
  FIT_MIS_AUTH_CCC_IDX,
  FIT_WECHAT_CCC_IDX,
  FIT_ALIPAY_CCC_IDX,
  FIT_NUM_CCC_IDX
};

/*
 * FUNCTIONS
 *******************************************************************************
 */


/**
 * @brief   API to send data through reliable protocol.
 *
 * @param   data, the data to be sent
 * @param   len,  the length of data
 *
 * @return  Status.
 */
ry_sts_t ble_reliableSend(uint8_t * data, uint16_t len, void *usrdata);

/**
 * @brief   API to send data through reliable protocol.
 *
 * @param   data, the data to be sent
 * @param   len,  the length of data
 *
 * @return  Status.
 */
ry_sts_t ble_reliableUnsecureSend(uint8_t* data, uint16_t len, void *usrdata);


/**
 * @brief   API to send data through json.
 *
 * @param   data, the data to be sent
 * @param   len,  the length of data
 *
 * @return  Status.
 */
ry_sts_t ble_jsonSend(uint8_t* data, uint16_t len, void *usrdata);


/**
 * @brief   API to send heart data through HRPS
 *
 * @param   data, the data to be sent
 * @param   len,  the length of data
 *
 * @return  Status.
 */
ry_sts_t ble_hrDataSend(uint8_t* data, uint16_t len);


/**
 * @brief   API to update connection parameters
 *
 * @param   intervalMin, minimal connection interval
 * @param   intervalMax, maximal connection interval
 * @param   timeout,     timeout of the connection
 * @param   slaveLatency, slave lantency value
 *
 * @return  Status.
 */
ry_sts_t ble_updateConnPara(uint32_t connId, u16_t intervalMin, u16_t intervalMax, u16_t timeout, u16_t slaveLatency);

void ry_ble_sendNotify(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data);



/**
 * @brief   API to init BLE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ble_init(ble_stack_cb_t stack_cb);


/**
 * @brief   API to wakeup BLE thread to do something 
 *
 * @param   None
 *
 * @return  None
 */
void wakeup_ble_thread(void);
void ble_hci_reset(void);


#endif  /* __BLE_TASK_H__ */

