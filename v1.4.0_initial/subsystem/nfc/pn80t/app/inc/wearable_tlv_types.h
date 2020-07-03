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
* \defgroup BleIntTLV	        BLE integration layer specific TLVs
*/

#ifndef WEARABLE_TLV_TYPES_H_
#define WEARABLE_TLV_TYPES_H_

/**
 * @} */

/**
 *	\brief BLE integration layer specific TLVs to be used by companion and wearable device for communicating to each other.
 */
/** \addtogroup BleIntTLV
 * @{ */


/**
 *	\name TLV Types and sub types
 */
/** @{ */

#define SE_MODE               0x01 ///< This is TLV Type for all SE Mode related Operations
#define WIRED_TRANSCEIVE      0x10        ///< TLV Sub type for Wired Transceive
#define WIREDMODE_ENABLE      0x11        ///< TLV Sub type for Enabling /disabling wired mode
#define WIREDMODE_SERESET     0x12        ///< TLV Sub type for SE reset
#define SUB_SE_NTFN           0x13        ///< TLV Sub type for subscribing for SE notification
#define UNSUB_SE_NTFN         0x14        ///< TLV Sub type for unsubscribing for SE notification
#define TRANSACTION_SE_NTFN   0x15        ///< TLV Sub type for sending transaction notification to companion device
#define CONNECTIVITY_SE_NTFN  0x16        ///< TLV Sub type for sending connectivity notification to companion device
#define RFFIELD_SE_NTFN       0x17        ///< TLV Sub type for sending RF Field notification to companion device
#define ACTIVATION_SE_NTFN    0x18        ///< TLV Sub type for sending Activation/deactivation notification to companion device
#define WIREDMODE_GET_ATR     0x19        ///< TLV Sub type for get ATR

#define NCI_RAW_MODE          0x05        ///< This is TLV Type for all NCI raw mode related Operations
#define SEND_NCI_RAW_COMMAND  0x50        ///< TLV Sub type for invoking SeApi_NciRawCmdSend at wearable

#define GENERAL_MODE          0x07        ///< This is TLV Type for all General Operations
#define GETSTACK_CAP          0x70        ///< TLV Sub type for invoking SeApi_GetStackCapabilities at wearable
#define LOOPBACK_OVER_BLE     0x71        ///< TLV Sub type for invoking Loop back Data Exchange over BLE Link
#define DEVICE_STATUS_NTFN    0x72        ///< TLV Sub type for notifying wearable device status to companion
#define SET_TRANSIT_SETTINGS  0x73        ///< TLV sub type for setting Transit settings based on the city
#define GET_TRANSIT_SETTINGS  0x74        ///< TLV sub type for getting Transit settings from wearable

#define WEARABLE_LS_LTSM      0x08     ///< This is TLV Type for executing LS LTSM clients at wearable from companion.
#define LS_EXECUTE_SCRIPT     0x81       ///< TLV Sub type for executing LS script at wearable sent by companion.
#define CREATE_VC             0x82       ///< TLV Sub type for creating mifare card.
#define DELETE_VC             0x83       ///< TLV Sub type for deleting mifare card.
#define ACTIVATE_VC           0x84       ///< TLV Sub type for activating mifare card.
#define DEACTIVATE_VC         0x85       ///< TLV Sub type for deactivating mifare card.
#define SET_MDAC_VC           0x86       ///< TLV Sub type for update MDAC.
#define READ_VC               0x87       ///< TLV Sub type for read VC.
#define GET_VC_LIST           0x88       ///< TLV Sub type for VC List.
#define GET_VC_STATUS         0x89       ///< TLV Sub type for VC Status.
/** @} */
/** @} */

#endif /* WEARABLE_TLV_TYPES_H_ */
