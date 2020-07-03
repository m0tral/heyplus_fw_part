/********************************************************************
  File Information:
    FileName:       mible.h
    Company:        Xiaomi Inc.
********************************************************************/

#ifndef __MIBLE_H__
#define __MIBLE_H__

#include "ry_type.h"

/*
 * CONFIGURATIONS
 ****************************************************************************************
 */
#define MIBLE_SUPPORT_CFG_WIFI                    1
#define MIBLE_SUPPORT_MI_BAND                     1

/*
 * INCLUDES
 ****************************************************************************************
 */


/*
 * DEFINES
 ****************************************************************************************
 */

#define MIBLE_ADDR_LEN                            6
#define MIBLE_EMPTY                               0xFF


#define MIBLE_VER_LEN                             10

#define MIBLE_TYPE_MIJIA                          0
#define MIBLE_TYPE_RYEEX                          1

#define MIBLE_AUTH_LEN                    4
#define MIBLE_SN_LEN                      20
#define MIBLE_TOKEN_LEN                   12
#define MIBLE_BEACONKEY_LEN               12
#define MIBLE_PSM_LEN                     (MIBLE_SN_LEN + MIBLE_TOKEN_LEN + MIBLE_BEACONKEY_LEN)


/*
 * Definition for BLE advertising type
 */
#define MIBLE_ADTYPE_FLAGS                        0x01 //!< Discovery Mode: @ref GAP_ADTYPE_FLAGS_MODES
#define MIBLE_ADTYPE_16BIT_MORE                   0x02 //!< Service: More 16-bit UUIDs available
#define MIBLE_ADTYPE_16BIT_COMPLETE               0x03 //!< Service: Complete list of 16-bit UUIDs
#define MIBLE_ADTYPE_32BIT_MORE                   0x04 //!< Service: More 32-bit UUIDs available
#define MIBLE_ADTYPE_32BIT_COMPLETE               0x05 //!< Service: Complete list of 32-bit UUIDs
#define MIBLE_ADTYPE_128BIT_MORE                  0x06 //!< Service: More 128-bit UUIDs available
#define MIBLE_ADTYPE_128BIT_COMPLETE              0x07 //!< Service: Complete list of 128-bit UUIDs
#define MIBLE_ADTYPE_LOCAL_NAME_SHORT             0x08 //!< Shortened local name
#define MIBLE_ADTYPE_LOCAL_NAME_COMPLETE          0x09 //!< Complete local name
#define MIBLE_ADTYPE_POWER_LEVEL                  0x0A //!< TX Power Level: 0xXX: -127 to +127 dBm
#define MIBLE_ADTYPE_OOB_CLASS_OF_DEVICE          0x0D //!< Simple Pairing OOB Tag: Class of device (3 octets)
#define MIBLE_ADTYPE_OOB_SIMPLE_PAIRING_HASHC     0x0E //!< Simple Pairing OOB Tag: Simple Pairing Hash C (16 octets)
#define MIBLE_ADTYPE_OOB_SIMPLE_PAIRING_RANDR     0x0F //!< Simple Pairing OOB Tag: Simple Pairing Randomizer R (16 octets)
#define MIBLE_ADTYPE_SM_TK                        0x10 //!< Security Manager TK Value
#define MIBLE_ADTYPE_SM_OOB_FLAG                  0x11 //!< Secutiry Manager OOB Flags
#define MIBLE_ADTYPE_SLAVE_CONN_INTERVAL_RANGE    0x12 //!< Min and Max values of the connection interval (2 octets Min, 2 octets Max) (0xFFFF indicates no conn interval min or max)
#define MIBLE_ADTYPE_SIGNED_DATA                  0x13 //!< Signed Data field
#define MIBLE_ADTYPE_SERVICES_LIST_16BIT          0x14 //!< Service Solicitation: list of 16-bit Service UUIDs
#define MIBLE_ADTYPE_SERVICES_LIST_128BIT         0x15 //!< Service Solicitation: list of 128-bit Service UUIDs
#define MIBLE_ADTYPE_SERVICE_DATA                 0x16 //!< Service Data
#define MIBLE_ADTYPE_APPEARANCE                   0x19 //!< Appearance
#define MIBLE_ADTYPE_MANUFACTURER_SPECIFIC        0xFF //!< Manufacturer Specific Data: first 2 octets contain the Company Identifier Code followed by the additional manufacturer specific data



#define MIBLE_RELATIONSHIP_STRONG                 0
#define MIBLE_RELATIONSHIP_WEAK                   1
#define MIBLE_RELATIONSHIP_NONE                   2

#define MIBLE_AUTH_SUCC                           0
#define MIBLE_AUTH_FAIL                           1

#define MIBLE_ROLE_CENTRAL                        0
#define MIBLE_ROLE_PERIPHERAL                     1

#define MISERVICE_UUID                            0xFE95
   
/**
 * Characteristic UUID
 */
// Mi Service Common Characteristics
#define MISERVICE_CHAR_TOKEN_UUID                 0x0001
#define MISERVICE_CHAR_PRODUCTID_UUID             0x0002
#define MISERVICE_CHAR_VER_UUID                   0x0004
#define MISERVICE_CHAR_WIFICFG_UUID               0x0005
#define MISERVICE_CHAR_EVENT_RULE_UUID            0x0007
#define MISERVICE_CHAR_AUTHENTICATION_UUID        0x0010
#define MISERVICE_CHAR_SN_UUID                    0x0013
#define MISERVICE_CHAR_BEACONKEY_UUID             0x0014


// Events Characteristics
#define MISERVICE_CHAR_KEY_CLIENT_UUID            0x1001
#define MISERVICE_CHAR_KEY_SERVER_UUID            0x1002
#define MISERVICE_CHAR_SLEEP_CLIENT_UUID          0x1003
#define MISERVICE_CHAR_SLEEP_SERVER_UUID          0x1004


/**
 * WIFI CONFIG TYPE
 */
#define MIBLE_WIFICFG_TYPE_UID                    0
#define MIBLE_WIFICFG_TYPE_SSID                   1
#define MIBLE_WIFICFG_TYPE_PW                     2
#define MIBLE_WIFICFG_RETRY                       3
#define MIBLE_WIFICFG_RESTORE                     4
#define MIBLE_WIFICFG_UTC                         5

/*
 * Definition for characteristic index of mi service
 */
typedef enum {
    MIBLE_CHAR_IDX_TOKEN,
    MIBLE_CHAR_IDX_TOKEN_CCC,
    MIBLE_CHAR_IDX_PRODUCTID,
    MIBLE_CHAR_IDX_VER,
    MIBLE_CHAR_IDX_WIFICFG,
    MIBLE_CHAR_IDX_WIFICFG_CCC,
    MIBLE_CHAR_IDX_AUTH,
    MIBLE_CHAR_IDX_SN,
    MIBLE_CHAR_IDX_BEACONKEY,
    MIBLE_CHAR_MAX,
    
} mible_charIdx_t;

/*
 * Definition for error code of MIBLE
 */
typedef enum {
    MIBLE_SUCC = 0,
    
    MIBLE_ERR_NO_MEM,
    MIBLE_ERR_INVALID_PARA,
    MIBLE_ERR_LEN_TOO_LONG,
    MIBLE_ERR_TASK_INIT_FAIL,

    MIBLE_ERR_AUTH_TABLE_FULL = 0x10,
    MIBLE_ERR_AUTH_EXISTED,
    MIBLE_ERR_AUTH_NOT_FOUND,
    MIBLE_ERR_AUTH_INIT_FAIL,
    MIBLE_ERR_AUTHINVALID_PARA,

    MIBLE_ERR_TABLE_FULL = 0x20,
    MIBLE_ERR_TABLE_EXISTED,
    MIBLE_ERR_TABLE_NOT_FOUND,

    MIBLE_ERR_BAND_TABLE_FULL = 0x30,
    MIBLE_ERR_BAND_EXISTED,
    MIBLE_ERR_BAND_NOT_FOUND,
    MIBLE_ERR_BAND_INIT_FAIL,
    MIBLE_ERR_BAND_INVALID_PARA,

    MIBLE_ERR_EVT_RULE_INIT_FAIL = 0x40,
    MIBLE_ERR_EVT_RULE_EXISTED,
    MIBLE_ERR_EVT_RULE_NOT_FOUND,
    MIBLE_ERR_EVT_RULE_INVALID_PARA,

    MIBLE_ERR_NBR_TABLE_FULL = 0x50,
    MIBLE_ERR_NBR_EXISTED,
    MIBLE_ERR_NBR_NOT_FOUND,
    MIBLE_ERR_NBR_INVALID_PARA,
    MIBLE_ERR_NBR_INIT_FAIL,
    
    MIBLE_ERR_CACHE_TABLE_FULL = 0x60,
    MIBLE_ERR_CACHE_EXISTED,
    MIBLE_ERR_CACHE_NOT_FOUND,
    MIBLE_ERR_CACHE_INVALID_PARA,
    MIBLE_ERR_CACHE_INIT_FAIL,

    MIBLE_ERR_MESH_TX_FAIL = 0x70,
    MIBLE_ERR_MESH_RX_FAIL,
    

    MIBLE_ERR_GATT_INVALID_PARAM = 0x81,
    MIBLE_ERR_GATT_NOT_WRITABLE = 0x87,
    MIBLE_ERR_GATT_UNEXPECTED_LEN = 0x8D,

    MIBLE_ERR_BEACON_ENC_INVALID_SERVICE_UUID = 0x90,
	MIBLE_ERR_BEACON_ENC_NOT_NEED_SEC,
	MIBLE_ERR_BEACON_ENC_FAIL,

    MIBLE_ERR_FASTPAIRING_TIMEOUT = 0xA0,
    MIBLE_ERR_FASTPAIRING_DISCONNECTED,

    MIBLE_ERR_CONN_DEVS_FULL = 0xB0,
    MIBLE_ERR_CONN_BUSY,
    MIBLE_ERR_CONN_DRIVER_FAIL,
    MIBLE_ERR_CONN_TIMEOUT,
    MIBLE_ERR_CONN_SERVICE_DISCOVERY_FAIL,
    MIBLE_ERR_CONN_UPDATE_PARA_FAIL,
    MIBLE_ERR_CONN_DEV_NOT_FOUND,

    MIBLE_ERR_GATT_BUSY = 0xC0,
        
} mible_sts_t;


/*
 * TYPES
 ****************************************************************************************
 */

/**
 *  @brief Authentication callback funciton definition.
 *
 *  @param status.
 */
typedef void (*authCb_t)( u8 status );

/**
 *  @brief Wifi infomation callback funciton definition.
 */
typedef void (*wifiInfoCb_t) (u8* ssid, u8 ssidLen, u8* pw, u8 pwLen, u8* uid, u8 uidLen);


/**
 *  @brief Type definition for application callbacks used in mi service.
 */
typedef struct {
    authCb_t authCbFunc;           
    wifiInfoCb_t wifiInfoCbFunc;
} mible_appCbs_t;


/**
 *  @brief Type definition for initialization parameters.
 */
typedef struct {
    u16    productID;                    /**< Two Byte Product ID. It is assigned by Xiaomi. */
    u8     relationship;                 /**< Relationship of the Mi Account and device. Strong or weak */
    u8     version[MIBLE_VER_LEN];       /**< Firmware version of the device. the format like 1.0.1_xxxx */
    mible_appCbs_t *cbs;                 /**< Callbacks used in application */
    u32    authTimeout;                  /**< Authentication timeout value in ms */
} mible_init_t;

typedef enum {
    CONN_STATE_NOTCONNECTED,
	CONN_STATE_CONNECTING,
	CONN_STATE_CONNECTED,
	CONN_STATE_ONLINE,
	CONN_STATE_FAILURE,
	CONN_STATE_PW_ERROR,
} mible_wifiSts_t;

/*
 * APIs
 ****************************************************************************************
 */

/**
 *@brief Initialize mi service.
 *
 *@param[in] initPara					Init parameter of mi service.
 *
 *@return Status
 */
u8 mible_init(mible_init_t *initPara, u8_t type);




/**
 *@brief Service registered handler.
 *
 *@param[in] startHandle		        Start handle of Mi serivce
 *@param[in] endHandle                  End handle of Mi Service
 *
 *@return None
 */
void mible_onServiceRegistered(u16 startHandle, u16 endHandle);

/**
 *@brief Connect event handler when device is connected by some Central device.
 *
 *@param[in] connHandle					Connection Handle
 *@param[in] peerAddrType               Address type of the connected peer device
 *@param[in] peerAddr                   Address of the connected peer address
 *@param[in] peerRole                   The role of the peer device. 0 - Central, 1 - Peripheral
 * 
 *@return None
 */
void mible_onConnect(u16 connHandle, u8* peerAddr, u8 peerAddrType, u8 peerRole);

/**
 *@brief Disconnect event handler when device is loss connection.
 *
 *@param[in] connHandle					Connection Handle
 *
 *@return None
 */
void mible_onDisconnect(u16 connHandle);

/**
 *@brief GATT Write handler.
 *
 *@param[in] handle			Characteristic Handle.
 *@param[in] index                      Index of characteristic
 *@param[in] len                        Length of written data.
 *@param[in] pData                      Pointer of the written data.
 *
 *@return Status, which should be used in write reponse.
 */
u8 mible_onCharWritten(u16 connHandle, mible_charIdx_t index, u8 len, u8* pData);

/**
 *@brief GATT Read handler.
 *
 *@param[in] handle                     Characteristic Handle.
 *@param[in] index                      Index of characteristic
 *@param[out] pLen                      Length of written data.
 *@param[out] pData                     Pointer of the written data.
 *
 *@return Status, which should be used in write reponse.
 */
u8 mible_onCharRead(u16 connHandle, mible_charIdx_t index, u8 *pLen, u8 *pData);

/*
 * mible_encrypt - Encryption API in mi service
 *
 * @param[in]   in     - The input plain text
 * @param[in]   inLen  - The length of input plain text
 * @param[out]  out    - The generated cipher text
 * 
 * @return      None
 */
void mible_encrypt(u8* in, int inLen, u8* out);

/*
 * mis_decrypt - Decryption API in mi service
 *
 * @param[in]   in     - The input cipher text
 * @param[in]   inLen  - The length of input cipher text
 * @param[out]  out    - The generated plain text
 * 
 * @return      None
 */
void mible_decrypt(u8* in, int inLen, u8* out);


void mible_setToken(u8_t* token);
void mible_getToken(u8_t* token);
void mible_getSn(u8_t* sn);
u8_t mible_isSnEmpty(void);
void mible_transfer(void);



void mible_wifiStatus_set(mible_wifiSts_t new_status);

mible_wifiSts_t mible_wifiStatus_get(void);

void mible_psm_reset(u8_t type);



#endif
