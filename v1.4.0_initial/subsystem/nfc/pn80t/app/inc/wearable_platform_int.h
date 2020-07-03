/* *
 * Copyright(C) NXP Semiconductors, 2016
 * All rights reserved.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef WEARABLE_PLATFORM_INT_H_
#define WEARABLE_PLATFORM_INT_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "ble_handler.h"

#define STORAGE_SIZE    1024
#define PERSO_SM_AID     0x42

#if(WEARABLE_LS_LTSM_CLIENT == TRUE)
/* when LS and LTSM clients running at wearable only LS scripts,
   LTSM VC data will be sent over BLE, that needs more memory */
#define TLV_BUFFER_SIZE LS_CMD_CHUNK_SIZE

#else
#if(WEARABLE_MEMORY_ALLOCATION_TYPE == NORMAL_MODE)
#define TLV_BUFFER_SIZE 570 /*JCOP3.x Download script issues the maximum of 567 bytes */
#else
#define TLV_BUFFER_SIZE 8250 /*JCOP 4.x Download script issues the maximum of 8247 bytes */
#endif
#endif

#define RESP_BUFF_SIZE   512//LS_RSP_CHUNK_SIZE


#define NFC_QUEUE_SIZE 10
#define EACI_QUEUE_SIZE 50

/* Internal TLV List - Start */

                                          ///<TLV Type to be used is SE_MODE(0x01)
#define JCOP_SERESET          0x1A        ///< TLV Sub type for SE reset during JCOP OS update depending on Power schemes configured
#define WIREDMODE_CONTROL     0x1B        ///< TLV Sub type for wired control

#define FWDND_MODE            0x02        ///< This is TLV Type for all Firmware Download related Operations
#define FW_DND_PREP           0x20        ///< TLV Sub type for Preparing wearable for firmware download
#define NFCC_MODESET          0x21        ///< TLV Sub type for setting mode (normal/ download) of wearable
#define READ_ABORT            0x22        ///< TLV Sub type for aborting pending TML read
#define TML_WRITE_REQ         0x23       ///< TLV Sub type for invoking phTmlNfc_Write at wearable
#define TML_READ_REQ          0x24        ///< TLV Sub type for invoking phTmlNfc_Read at wearable
#define FW_DND_POST           0x25        ///< TLV Sub type for for reinitializing wearable post to firmware download
#define GET_NFCC_DEVICE_INFO  0x26        ///< TLV Sub type for for getting wearable device info before firmware download
/*#define CLEANUP_ON_BLE_INTERRUPTION  0x27 ///< TLV Sub type for for cleaning up stack on BLE interruption during Fw Dnd.*/

#define API_TEST_MODE         0x03        ///< This is TLV Type for all API Testing related Operations
#define SEAPI_INIT            0x30        ///< TLV Sub type for invoking SeApi_Init at wearable
#define SEAPI_SHUTDOWN        0x31        ///< TLV Sub type for invoking SeApi_Shutdown at wearable
#define SEAPI_SESELECT        0x32        ///< TLV Sub type for invoking SeApi_SeSelect at wearable
#define SEAPI_SECONFIGURE     0x33        ///< TLV Sub type for invoking SeApi_SeConfigure at wearable
#define SEAPI_FW_DNLD         0x34        ///< TLV Sub type for invoking SeApi_InitWithFirmware/SeApi_FirmwareDownload at wearable
#define LS_NEGATIVE           0x36        ///< TLV Sub type for LS related negative testing
#define LTSM_NEGATIVE         0x37        ///< TLV Sub type for LTSM related negative testing
#define FIRMWARE_NEGATIVE     0x38        ///< TLV Sub type for Firmware Download related negative testing
#define GET_ATR_NEGATIVE      0x39        ///< TLV Sub type for GET ATR related negative testing
#define WIRED_TRANSCEIVE_NEGATIVE    0x3A ///< TLV Sub type for Wired Transceive related negative testing
#define LS_CHUNK_API_TEST     0x3B        ///< TLV Sub type for LS Chunk API related testing
#define BLE_API_TEST          0x3D        ///< TLV Sub type for Ble Api related tests
#define SEAPI_INIT_UNINITIALIZED     0x3E ///< TLV Sub type for SeApi Init Uninitialized scenarios
#define SEAPI_NEGATIVE        0x3F        ///< TLV Sub type for SeApi negative scenarios testing

#define NVMEM_RW_MODE         0x04        ///< This is TLV Type for all NV Mem related Operations
#define JCOP_NVMEM_READ       0x42        ///< TLV Sub type for invoking readUserdata at wearable
#define JCOP_NVMEM_WRITE      0x43        ///< TLV Sub type for invoking writeUserdata at wearable

                                            ///< TLV Type to be used is GENERAL_MODE(0x07)
#define RESTART_BLE_NFC_BRIDGE  0x75        ///< TLV sub type for restarting BleNfcBridge task
/* Internal TLV List - End */

#ifndef NFC_UNIT_TEST
extern phDevice_Status_t DeviceStatus;
extern phNfcBle_Context_t gphNfcBle_Context;
#endif
extern QueueHandle_t xQueueNFC;
extern SemaphoreHandle_t nfcQueueLock;
extern xTaskHandle xHandleBLETask;                        /* FreeRTOS taskhandle for BLE Task */
extern int8_t GeneralState;
extern BOOLEAN bBleInterrupted;
extern BOOLEAN  bComSubForSeNotfn;
/*extern QueueHandle_t ble_nfc_semaphore;*/
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
extern const uint8_t gphDnldNfc_DlSequence[];
extern const uint16_t gphDnldNfc_DlSeqSz;
extern const uint8_t gphDnldNfc_DlSequenceDummyFw[];
extern const uint16_t gphDnldNfc_DlSeqDummyFwSz;
#endif

extern void BleNfcBridge(void *pvParameters);
extern void notifyDeviceStatus(void* data);
extern void transactionOccurred(UINT8* pAID, UINT8 aidLen, UINT8* pData, UINT8 dataLen, eSEOption src);
extern INT32 sessionRead(UINT8* pData, UINT16 numBytes);
extern INT32 sessionWrite(const UINT8* pData, UINT16 numBytes);
extern UINT16 transitSettingRead(UINT8* pData, UINT16 numBytes);
extern UINT16 transitSettingWrite(const UINT8* pData, UINT16 numBytes);
extern void connectivityCallback(void* pData, UINT8 event);
extern void activationStatusCallback(void* pData, UINT8 event);
extern void rfFieldStatusCallback(void* pData, UINT8 event);
extern void NciRawModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* respSize, uint8_t* pRespBuf, unsigned char* respTagType);
extern void NciRawModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* respSize, uint8_t* pRespBuf, unsigned char* respTagType);
extern void GeneralModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* respSize, uint8_t* pRespBuf, unsigned char* respTagType);
extern void FwDndTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRspSize, uint8_t* pRespBuf, unsigned char* respTagType);
extern void NvMemTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* pRespTagType);
extern void SeModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRspSize, uint8_t* pRespBuf, unsigned char* pRespTagType);
extern void WearableLsLtsmTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* respTagType);
extern void ApiTestTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* respTagType);
extern void NfcA_Finalize(void);
extern void NfcA_HalCleanup(void);
extern void cleanupOnBleInterruption(void);
extern void disableWiredandSEreset(void);
extern INT32 BLEAddressRead(UINT8* data, UINT16 numBytes);
extern INT32 BLEAddressWrite(UINT8* data, UINT16 numBytes);
extern INT32 ble_LoadParameters(UINT8 *address, UINT8 *name);

#ifndef NFC_UNIT_TEST
#if (ENABLE_SESSION_MGMT == TRUE)
extern tSeApi_SessionSaveCallbacks sessionCb;
#endif
#endif

#endif /* WEARABLE_PLATFORM_INT_H_ */
