/*
* Copyright (c), NXP Semiconductors
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
* NXP reserves the right to make changes without notice at any time.
*
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
* particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
* arising from its use.
*/


/**
* \defgroup BleIntAPI	        BLE integration layer specific APIs
*/
#ifndef _BLE_HANDLER_H_
#define _BLE_HANDLER_H_

#ifndef NFC_UNIT_TEST

#include "ry_type.h"
#include "data_types.h"
#include "debug_settings.h"

/* @} */

/**
 *	\brief BLE integration layer specific APIs to be implemented to support Non NXP BLE Stack.
 */
/** \addtogroup BleIntAPI
 * @{ */

/**
 *  \brief Status used by the BLE Integration APIs.
 *
 * */
typedef uint8_t tBLE_STATUS;

/**
 *	\name Status codes
 */
/* @{ */
#define BLE_STATUS_OK                   0x00          ///< Command succeeded
#define BLE_STATUS_ALREADY_INITIALIZED  0x01		  ///< API called twice
#define BLE_STATUS_ALREADY_DEINITIALIZED  0x02		  ///< API called twice
#define BLE_STATUS_INVALID_PARAM        0x04		  ///< invalid parameter provided
#define BLE_STATUS_FAILED               0x09		  ///< failed
/* @} */

#define BLE_INACT_COUNT             200          ///< Increased to make sure standby is activated only after operation is over.
#define BLE_NAME_LENGTH             20           ///< Length of Ble device Name
#define BLE_ADDRESS_LENGTH          6            ///< Length of Ble device Address
#define BLE_CFM_TIMEOUT             5000         ///< Max time in ms to wait to get CFM event after writing to BLE UART

/**
 *  \name BLE advertising interval
 */
/* @{ */
#define ADV_INTV_MIN               (800/0.625)   ///< Minimum 800ms
#define ADV_INTV_MAX              (1000/0.625)   ///< Maximum 1s
/* @} */

/**
 *	\brief This data structure is used message exchange over BLE.
 *
 * */
typedef struct {
	unsigned char type;    ///< Type of the message
	unsigned int length;   ///< Length of the message's payload
	uint8_t* value;        ///< Payload of the message
} xTLVMsg;

/**
 *	\brief This data structure is used for the Storing Ble Specific Configuration Data.
 *
 * */
typedef struct phNfcBle_Context
{
    UINT32 bytesReceived; 						///<  Keeps Count of bytes received from companion device
	UINT32 requiredBytes;						///<  Total Count of bytes to be received from companion device
	UINT8 BD_Address[BLE_ADDRESS_LENGTH]; ///<  Ble device Address
	UINT8 BD_Name[BLE_NAME_LENGTH];		///<  Ble device Name
	bool isBleInitialized;                     ///<  BLE Init Status
	UINT32 timeoutBleTimerId;         ///< Ble Timer Id
	bool isFirstMessage;                 ///< First Ble Message
} phNfcBle_Context_t;

/**
 * \brief BLE Task function.
 *
 * \param pvParameters - [in] Parameters to the task function
 *
 * \return none
 */
void BLETask(void *pvParameters);
/**
 * \brief De-initialize Ble specific subsystem.
 *
 *
 * \param pContext - [in] context is place holder to pass necessary configs.
 *
 * \return BLE_STATUS_ALREADY_DEINITIALIZED
 */
tBLE_STATUS phNfcDeInitBleSubsystem(void* pContext);

/**
 * \brief Initialize Ble specific subsystem.
 *
 *
 * Initializes underlying BLE sub system. This includes initializing all required resources like transport layer init (Ex: UART),
 * Tasks and  hosting GATT server based  Raw Channel service advertising  (Ex: QPP Profile) for data exchange
 * between Companion and wearable device.
 * Initializes the DMA,configures the QN902X with the device name and address and starts Advertising.
 *
 * \param pContext - [in] context is place holder to pass necessary configs.
 *
 * \return SEAPI_STATUS_OK on success
 * \return BLE_STATUS_ALREADY_INITIALIZED if ble is already initialized
 * \return BLE_STATUS_FAILED otherwise
 */
tBLE_STATUS phNfcInitBleSubsystem(void* pContext);
/**
 * \brief Transmits response packets from NFC to BLE
 *
 * This function sends data response received from NFCC to companion device.
 * Ble channel can accept only 20 bytes at a time so first entire response has to be split in to 20 bytes before sending.
 *
 * \param pContext - [in] context is place holder to pass necessary configs.
 * \param pResposeBuffer - [in] response received from NFC
 * \return tBLE_STATUS
 */
tBLE_STATUS phNfcSendResponseToCompanionDevice(void* pContext,void* pResposeBuffer);

/**
 * \brief Starts Ble discovery
 *
 * Once NFCC and Se initialization is done ble discovery started using phNfcStartBleDiscovery
 *
 * \return none
 */
void phNfcStartBleDiscovery(void);

/* @} */
#endif
#endif//_BLE_HANDLER_H_

