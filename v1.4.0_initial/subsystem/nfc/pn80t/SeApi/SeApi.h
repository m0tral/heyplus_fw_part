/*
 * Copyright (C) 2015 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * \file  SeApi.h
 *  \brief This is main API for the Wearable stack.
 *
 *		  This API has all of the methods to control the Wearable stack,
 *		  including LoaderService, JCOPUpdate and MOP.
 *
 *		  Regarding LoaderService, JCOPUpdate and MOP.
 *		  If SESUPPORT_INCLUDE is TRUE, then SeApi will be build
 *		  with SeSupport integrated.  In this configuration, the
 *		  wearable application should use all SeApi methods and
 *		  the LoaderService, JCOPUpdate and MOP will directly
 *		  call through to SeSupport.
 *
 *		  If SESUPPORT_INCLUDE is FALSE, then LoaderService,
 *		  JCOPUpdate and MOP will be excluded from the SeApi.
 *		  In this configuration, SeSupport is intended to be
 *		  built and integrated on the HOST, not on the wearable
 *		  The host application should build on the SeSupport
 *		  code and the LoaderService, JCOPUpdate and MOP will
 *		  be called and executed from the host rather than
 *		  from the wearable.
 *
 *		  This split is done because the SeSupport features
 *		  require significantly more RAM and in some cases
 *		  require persistent memory.  This allows the integrator
 *		  to choose whether the wearable has enough resources
 *		  to build all of the functionality into the wearable
 *		  (SESUPPORT_INCLUDE = TRUE) or whether the wearable
 *		  resources are limited enough and the SeSupport
 *		  capabilities should exist in the host
 *		  (SESUPPORT_INCLUDE = FALSE)
 *
 *        All SeApi API functions are BLOCKING functions
 */

/**
 * \defgroup seapi	          SeApi
 * \defgroup seapi_sesupport  SeApi applet and OS update and control
 * \defgroup seapi_ls         LoaderService
 * \defgroup seapi_jc         JCOPUpdate
 * \defgroup seapi_mop        Mifare Open Platform
 * \defgroup seapi_hwselftest Antenna/Hardware Tests
 */
#ifndef SEAPI_H
#define SEAPI_H

#ifdef __cplusplus
extern "C"{
#endif

#ifdef COMPANION_DEVICE
#include "p61_data_types.h"
#else
#include "gki.h"
#endif
#if (MIFARE_OP_ENABLE == TRUE || SESUPPORT_INCLUDE == TRUE)
#include "SeSupport.h"
#endif
#include "AlaUtils.h"


/**
 *	\brief Control access and availability of the secure elements
 */

/** \addtogroup seapi
 * @{ */
/**
 * \brief set to TRUE if wanting to use a file system for storing JCOP and Cardlet Scripts
 * 	GKI OS abstraction functions for file access functions such as file open, close, seek, fprintf must be
 * 	available.
 *
 * 	set to FALSE if wanting to use buffers for passing the JCOP and Cardlet scripts to the
 * 	LoaderService and JCOPUpdate
 */
#define SEAPI_USE_FILE_IO (FALSE)

/**
 *	\name Status codes
 */
/* @{ */
#define SEAPI_STATUS_OK                   0x00          ///< Command succeeded
#define SEAPI_STATUS_BUFFER_FULL          0xE0          ///< buffer full
#define SEAPI_STATUS_FAILED               0x03          ///< failed
#define SEAPI_STATUS_NOT_INITIALIZED      0x04			///< API called with first being initialized
#define SEAPI_STATUS_NO_ACTION_TAKEN      0x08          ///< no action required
#define SEAPI_STATUS_INVALID_PARAM        0x09			///< invalid parameter provided
#define SEAPI_STATUS_REJECTED             0x93          ///< request is rejected
#define SEAPI_STATUS_UNEXPECTED_FW        0x3D          ///< Firmware in NFCC is not the one expected
#define SEAPI_STATUS_SMX_BAD_STATE        0xF0          ///< JCOP is in Bad state

/* @} */

/**
 *	\name Technology types
 */
/* @{ */
#define SEAPI_TECHNOLOGY_MASK_A           0x01          ///< NFC Technology A
#define SEAPI_TECHNOLOGY_MASK_B           0x02          ///< NFC Technology B
#define SEAPI_TECHNOLOGY_MASK_F           0x04          ///< NFC Technology F
/* @} */

/**
 *  \name Stack Capabilities and Versions
 */
/* @{ */
#define WEARABLE_SDK_VERSION          0x020800          ///< SDK Version
#define NFC_MIDDLEWARE_VERSION    ((((NXP_ANDROID_VER << 8) | NFC_NXP_MW_VERSION_MAJ) << 8) | NFC_NXP_MW_VERSION_MIN)  ///< NFC Middleware Version
#define ESE_LTSM_VERSION                0x0202          ///< eSE LTSM Version
/* @} */

/**
 * \brief  Status used by SeApi's
 */
typedef UINT8 tSEAPI_STATUS;

/**
 * \brief  Types of Secure Elements
 */
typedef enum {
    SE_NONE		= 0x000,          ///< None
    SE_ESE		= 0x4C0,          ///< eSE
    SE_UICC		= 0x402,          ///< UICC
    SE_UNDEF	= 0xFFFF          ///< Undefined
} eSEOption;

/**
 * \brief  Chunk Sizes
 */
typedef struct phChunk_Size
{
    UINT16 CmdBufSize;    ///< Command Chunk Buffer Size
    UINT16 RspBufSize;    ///< Response Chunk Buffer Size
}phChunk_Size_t;

/**
 * \brief  struct lists out the stack capabilities and versions
 */
typedef struct phStackCapabilities{
    UINT32 sdk_ver;          ///< SDK version in the format: RFU | Major Version | Minor Version | Patch Version
    UINT32 middleware_ver;   ///< Middleware version in the format: RFU | Branch Indicator | Major Version | Minor Version
    UINT32 firmware_ver;     ///< NFCC Firmware version in the format: RFU | ROM Version | Major Version | Minor Version
    UINT16 nci_ver;          ///< NFC NCI version in the format: Major Version | Minor Version
    UINT8  ese_ver;          ///< eSE Hardware version
    UINT32 jcop_ver;         ///< eSE JCOP version in the format: RFU | RFU | Major Version | Minor Version
    UINT32 ls_ver;           ///< LS version in the Format  : RFU | Major Version | Minor Version | Intermediate Version
    UINT16 ltsm_ver;         ///< LTSM version in the format: Major Version | Minor Version
    phChunk_Size_t chunk_sizes;     ///< Chunk Buffer(s) size
} phStackCapabilities_t;

/**
 * \brief  struct lists out the transit settings
 */
typedef struct phTransitSettings{
    UINT8 ese_listen_tech_mask;  ///< To configure required listen mode technologies dynamically
    UINT8 cfg_blk_chk_enable;     ///< To disable/enable block number check of MIFARE card in firmware
    UINT8 cityType;               ///< city type e.g 1,2,3 for City1..etc
    UINT8 rfSettingModified;      ///< To be set to 1 if customized settings need to be applied.
} phTransitSettings_t;

/**
 *  \name Firmware Download Status
 */
/* @{ */
#define FWDNLDSTATUS_COMPLETED    0x00    ///< Firmware Download was complete
#define FWDNLDSTATUS_INTERRUPTED  0x01    ///< Firmware Download was interrupted
/* @} */

/**
 *  \name Init Completion Status
 */
/* @{ */
#define INITSTATUS_COMPLETED                0x00    ///< Init is successful
#define INITSTATUS_FAILED                   0x01    ///< Init is failed
#define INITSTATUS_FAILED_TO_GET_ATR        0x02    ///< Could not get the ATR
/* @} */

/**
 * \brief  Device Status
 */
typedef struct phDevice_Status
{
    UINT8 fwdnldstatus;    ///< Firmware Download Status
    UINT8 initstatus;      ///< Init Completion Status
}phDevice_Status_t;


#ifndef COMPANION_DEVICE
/**
 * \brief  callback function used to receive notifications of a transaction.
 *
 * 			This data is provided by the stack, to the user.  The user should
 * 			implement this callback if they want to receive the NTF information.
 * 			If the caller wants to keep they data, it must be copied from the
 * 			provided buffers into the caller's own buffers.
 *
 * 			- If the callback function pointer
 * 			is NULL, the user will not receive the notifications.
 *
 * \param pAID - pointer to the stack buffer containing the AID
 * \param aidLen - length of the AID in the stack's buffer
 * \param pData - pointer to the stack buffer containing the data
 * \param dataLen - length of the data in the stack's buffer
 * \param src - the source of the information - SE_ESE or SE_UICC
 */
typedef void (tSeApi_TransCallback)(UINT8* pAID, UINT8 aidLen, UINT8* pData, UINT8 dataLen, eSEOption src);

/**
 * \brief  callback used to save session information.
 *
 *          The function will receive a buffer containing data and the number
 *          of bytes of data that should be written to non-volatile memory.
 *
 *			- IF this callback is NOT provided, the eSE and UICC
 *			will get fully initialized on each initialization of the stack,
 *			the session state information will NOT be stored.
 *
 * \param buf - pointer to the buffer containing the data to save
 * \param bytesRequested - number of bytes of data to save
 *
 * \return SUCCESS: will return a non-negative number of bytes successfully saved
 *         FAILURE: -1
 */
#if (ENABLE_SESSION_MGMT == TRUE)
typedef INT32 (tSeApi_SessionWriteCallback)(const UINT8* buf, UINT16 bytesToWrite);

/**
 * \brief  callback used to retrieve session information.
 *
 *          The function will receive a buffer and an expected number of
 *          bytes to be retrieved from non-volatile memory and placed into the buffer.
 *
 *			- IF callback is NOT provided, the eSE and UICC
 *			will get fully initialized on each initialization of the stack,
 *			the session state information will NOT be stored.
 *
 * \param buf - pointer to the buffer into which the data should be placed
 * \param bytesRequested - number of bytes that should be placed into the buffer
 *
 * \return SUCCESS: will return a non-negative number of bytes placed into the buffer
 *                  this number will be less than or equal to the number of bytes
 *                  requested.
 *         FAILURE: -1
 */
typedef INT32 (tSeApi_SessionReadCallback)(UINT8* buf, UINT16 bytesToRead);

/**
 * \brief  struct combining the two session callbacks
 *
 *		  - IF this structure is NOT provided, the eSE and UICC
 *			will get fully initialized on each initialization of the stack,
 *			the session state information will NOT be stored.
 *
 */
typedef struct
{
    tSeApi_SessionWriteCallback	*writeCb;  ///< Write callback function pointer
    tSeApi_SessionReadCallback *readCb;    ///< Read callback function pointer
}tSeApi_SessionSaveCallbacks;
#endif

/**
 * \brief  callback function used to receive notifications of Rf Field Status.
 *
 * 			This data is provided by the stack, to the user.  The user should
 * 			implement this callback if they want to receive the NTF information.
 * 			If the caller wants to keep they data, it must be copied from the
 * 			provided buffers into the caller's own buffers.
 *
 * 			- If the callback function pointer
 * 			is NULL, the user will not receive the notifications.
 *

 * \param pData - pointer to the stack buffer containing the data
 * \param event - SE Event
 */
typedef void (tSeApi_RfFieldStatusCallback)(void* pData, UINT8 event);
/**
 * \brief  callback function used to receive notifications connectivity status
 *
 *
 * 			This data is provided by the stack, to the user.  The user should
 * 			implement this callback if they want to receive the NTF information.
 * 			If the caller wants to keep they data, it must be copied from the
 * 			provided buffers into the caller's own buffers.
 *
 * 			- If the callback function pointer
 * 			is NULL, the user will not receive the notifications.
 *

 * \param pData - pointer to the stack buffer containing the data
 * \param event - SE Event
 */
typedef void (tSeApi_ConnectivityCallback)(void* pData, UINT8 event);
/**
 * \brief  callback function used to receive notifications of a transaction.
 *
 * 			This data is provided by the stack, to the user.  The user should
 * 			implement this callback if they want to receive the NTF information.
 * 			If the caller wants to keep they data, it must be copied from the
 * 			provided buffers into the caller's own buffers.
 *
 * 			- If the callback function pointer
 * 			is NULL, the user will not receive the notifications.
 *

 * \param pData - pointer to the stack buffer containing the data
 * \param event - SE Event
 */
typedef void (tSeApi_ActivationStatusCallback)(void* pData, UINT8 event);
/**
 * \brief  struct combining three Se events notification callback
 *
 *		  - IF this structure is NOT provided, we will get notified till app level
 *		  for any se events notifications.
 */
typedef struct
{
    tSeApi_RfFieldStatusCallback *rfFieldStatusCb;       ///<  RF Field Status callback
    tSeApi_ConnectivityCallback *connectivityCb;         ///<  connectivity status callback
    tSeApi_ActivationStatusCallback *activationStatusCb; ///<  Activation status callback
}tSeApi_SeEventsCallbacks;

/**
 * \brief Apply Transit settings based on city
 *
 * \param pTrasitSettings - [in] Pointer to structure of phTransitSettings_t
 * *\param bDefaultSettings - [in] default settings
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_NOT_INITIALIZED - if SE is not initialized
 * \return SEAPI_STATUS_INVALID_PARAM for invalid parameters
 *  \return SEAPI_STATUS_FAILED - otherwise
 */
tSEAPI_STATUS SeApi_ApplyTransitSettings(phTransitSettings_t * pTrasitSettings, BOOLEAN bDefaultSettings);

/**
 * \brief Initialize the system with possible firmware update during initialization
 * 		  This is provided as an ALTERNATE to SeApi_Init().
 *
 * 		  If SeApi_Init() is used,
 * 		  and one wants to do a firmware update, the caller would use
 * 		  SeApi_FirmwareDownload() when they wanted to perform the download.
 *
 * 		  This function will, during the initialization, test if the provided firmware
 * 		  is different from the current firmware and update it if they are different.
 *
 * \param fp - [in] Transaction callback
 * \param scb - [in] struct of session save callbacks
 * \param dataLen - [in] length of the firmware data
 * \param pData - [in] pointer to a buffer containing the firmware data
 * \param dataRecLen - [in] length of the recovery firmware data
 * \param pDataRec - [in] pointer to a buffer containing the recovery firmware data
 *
 * \return SEAPI_STATUS_OK on success
 * \return SEAPI_STATUS_INVALID_PARAM for invalid parameters
 * \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_InitWithFirmware(tSeApi_TransCallback *fp, void *scb, UINT16 dataLen, const UINT8 *pData, UINT16 dataRecLen, const UINT8 *pDataRec);

/**
 * \brief Initialize the system
 *
 * \param fp - [in] Transaction callback
 * \param sessioncb - [in] pointer to structure(tSeApi_SessionSaveCallbacks) of session save call-backs
 *
 * \return SEAPI_STATUS_OK on success
 * \return SEAPI_STATUS_INVALID_PARAM for invalid parameters
 * \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Init(tSeApi_TransCallback *fp, void *sessioncb);

/**
 * \brief Subscribe for SE Notification events
 *
 * \param secb - [in] Pointer to structure of SE events call-backs
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_NOT_INITIALIZED - if SE is not initialized
 */
tSEAPI_STATUS SeApi_SubscribeForSeEvents(tSeApi_SeEventsCallbacks *secb);

/**
 * \brief UnSubscribe for SE Notification events
 *
 * \return none
 */
void SeApi_UnSubscribeForSeEvents(void);

/**
 * \brief Get stack capabilities
 *
 * \param pStackCapabilities - [out] pointer to structure indicating stack capabilities
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_INVALID_PARAM - if invalid parameters are passed
 */
tSEAPI_STATUS SeApi_GetStackCapabilities(phStackCapabilities_t *pStackCapabilities);

/**
 * \brief Shutdown the system
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Shutdown(void);

/**
 * \brief Choose which secure element is accessible over the RF interface
 *
 * \param se - [in] enumeration of SE_NONE (no NFCEE available on RF), SE_UICC or SE_ESE
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_SeSelect(eSEOption se);

/**
 * \brief Choose which technologies are routed to which SE.
 *
 *        In the bitmasks,
 *        the technology masks must be mutually exclusive.  For example, it is
 *        NOT valid to have TECH_A for both the eSE and the UICC.
 *
 * \param eSE_TechMask - [in] a bitmask of which technologies should go to the eSE
 * \param UICC_TechMask - [in] a bitmask of which technologies should go to the UICC
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_SeConfigure(UINT8 eSE_TechMask, UINT8 UICC_TechMask);

/**
 * \brief write directly to the PN54x
 *                              BLOCKING - this will block until the write is complete
 *
 * \param dataLen - [in] number of bytes to be written to the PN547
 * \param pData - [in] pointer to a buffer containing the data that will be written
 * \param pRespSize - [in] the size of the response buffer being passed in,
 * 						[out] the number of bytes actually written into the respBuf
 * \param pRespBuf - [out] pointer to a buffer to receive the response from the PN547
 *
 * \return NFC_STATUS_OK - if successful
 * \return SEAPI_STATUS_INVALID_PARAM - if invalid parameters are passed
 * \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_NciRawCmdSend(UINT16 dataLen, UINT8 *pData, UINT16 *pRespSize, UINT8 *pRespBuf);

/**
 * \brief Enable/disable wired mode between the eSE and the host.
 *
 * 	      If the eSE is currently in virtual card mode when wired mode is enabled
 *        then virtual card mode will remain enabled when switching into wired mode.
 *        If the user wants virtual mode disabled, it must be explicitly disabled
 *        by calling SeApi_SeSelect()
 *
 * \param	enableWired - [in] TRUE or FALSE to enabled or disable wired mode
 *
 * \return SEAPI_STATUS_OK on success
 * \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return SEAPI_STATUS_NO_ACTION_TAKEN if SE is already in the requested state
 * \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_WiredEnable(UINT8 enableWired);
/**
 * \brief   Trigger a reset of the eSE.
 *
 *          - This is only required to support
 *          the iChannel eSE_Reset.
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return  SEAPI_STATUS_FAILED otherwise
 *
 */
tSEAPI_STATUS SeApi_WiredResetEse(void);

/**
 * \brief  GetAtr response from the connected eSE
 *
 * \param pRecvBuffer - [out] pointer to a buffer into which the ATR will be written
 * \param pRecvBufferSize - [in/out] pointer to a UINT16 that contains the size of the
 * 			recvBuffer and will receive the actual number of bytes written on
 * 			exit.
 *        - The recvBuffer will contain the ATR and recvBufferSize
 *        will contain the length of the ATR returned.
 *        - If the provided buffer is too small for the ATR returned,
 *        the ATR will be copied to the buffer up to the length
 *        passed in, BUT to allow the caller to detect that some
 *        data was not copied, the recvBufferSize parameter will
 *        contain the ACTUAL response size which, in this case,
 *        will be larger than the value passed in.
 *        - MAXIMUM ATR LENGTH = 40 bytes
 *
 * \return      SEAPI_STATUS_OK on success
 * \return	    SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return		SEAPI_STATUS_BUFFER_FULL if the ATR is larger than the provided buffer.
 * \return		SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * \return	    SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_WiredGetAtr (UINT8* pRecvBuffer, UINT16 *pRecvBufferSize);

/**
 * \brief  Send data to the secure element; read it's response.
 *
 * \param pXmitBuffer - [in] pointer to a buffer containing the APDU/command
 *                          to send to the eSE
 * \param xmitBufferSize - [in] number of bytes of data in the xmitBuffer that
 *                              should be sent to the eSE
 * \param pRecvBuffer - [out] pointer to a buffer that will receive the response
 *                              from the eSE.  This should generally be
 *                              260-1024 bytes in size
 * \param recvBufferMaxSize - [in] the size of the recvBuffer, the maximum amount of
 *                                 data that can be written into the buffer
 *                                 MAXIMUM RESPONSE LENGTH = 1024 bytes
 *                                 MAXIMUM INTERNAL RESPONSE LENGTH = 260 bytes
 * \param pRecvBufferActualSize - [out] pointer to an INT32 that will receive the
 *                                     actual number of bytes written into the
 *                                     recvBuffer.  This will always be less than
 *                                     or equal to recvBufferMaxSize
 * \param timeoutMillisec - [in] maximum amount of time, in milliseconds, to wait
 *                               for the eSE to provide it's response.
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * \return  SEAPI_STATUS_FAILED otherwise
 *
 */
tSEAPI_STATUS SeApi_WiredTransceive (UINT8 *pXmitBuffer, UINT16 xmitBufferSize, UINT8 *pRecvBuffer,
        UINT16 recvBufferMaxSize, UINT16 *pRecvBufferActualSize, UINT32 timeoutMillisec);


/**
 * \brief Update the PN54x with new firmware
 *
 * 			The application must call SeApi_Shutdown() prior to
 * 			calling SeApi_FirmwareDownload().
 * 			The application must call SeApi_Init() after the
 * 			firmware download completes.
 *
 * \param dataLen - [in] number of bytes to be written (size of firmware)
 * \param pData - [in] pointer to a buffer containing the PN54x firmware
 *
 * \return  SEAPI_STATUS_OK on success *
 * \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * \return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_FirmwareDownload(UINT16 dataLen, const UINT8 *pData);


tSEAPI_STATUS SeApi_Polling(UINT8 fEnable);
tSEAPI_STATUS SeApi_RFDiscovery(UINT8 fEnable);
UINT8 SeApi_isWiredEnable(void);
tSEAPI_STATUS SeApi_SeSelectWithReader(eSEOption se, UINT8 fReader);



/* @} */

//------------------------------------------------------------------------------------
// JCOPUpdate
//------------------------------------------------------------------------------------
#if (JCOP_ENABLE == TRUE)
/** \addtogroup seapi_sesupport
 * @{ */
/**
 *	\brief Update the JCOP OS on the embedded secure element.
 *         These functions are available only if SESUPPORT_INCLUDE == TRUE and JCOP_ENABLE == TRUE.
 */
/** \addtogroup seapi_jc
 * @{ */

/**
 * \brief Update the JCOP on the eSE.
 *
 *        The caller should allocate buffers for
 *        each of the three scripts and provide those buffers to the function.
 *        These are fairly large scripts (150k - 400k each).
 *        The update takes 1-2 minutes.  The only feedback during update
 *        will be calls to cbGet and cbSet.
 *
 * \pre  NFC Stack must be initialized
 *
 * \param lenBuf1 - [in] number of bytes in the 1st JCOP update buffer
 * \param pBuf1 - [in] pointer to a buffer containing the first JCOP update script
 * \param lenBuf2 - [in] number of bytes in the 2nd JCOP update buffer
 * \param pBuf2 - [in] pointer to a buffer containing the second JCOP update script
 * \param lenBuf3 - [in] number of bytes in the 3rd JCOP update buffer
 * \param pBuf3 - [in] pointer to a buffer containing the third JCOP update script
 * \param cb - [in] pointer to the tJcopUpdate_StorageCb struct containing the
 *                  status callback function pointers
 *
 * \return  SEAPI_STATUS_OK on success *
 * \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * \return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_JCOPUpdate(UINT32 lenBuf1, const UINT8 *pBuf1,
                                       UINT32 lenBuf2, const UINT8 *pBuf2,
                                       UINT32 lenBuf3, const UINT8 *pBuf3,
                                       tJcopUpdate_StorageCb *cb);
/* @} */
/* @} */
#endif

//------------------------------------------------------------------------------------
// Mifare Open Platform
//------------------------------------------------------------------------------------
#if (MIFARE_OP_ENABLE == TRUE)
/** \addtogroup seapi_sesupport
 * @{ */
/**
 *  \brief Access the Mifare Open Platform capabilities.
 *         These functions are available only if SESUPPORT_INCLUDE == TRUE and MIFARE_OP_ENABLE == TRUE.
 */
/** \addtogroup seapi_mop
 * @{ */

/**
 * \brief Initialization prior to using MOP calls.  Once MOP calls are complete,
 *        MOP deinit should be called.
 *
 *        Configure the buffer required for storing persistent data.  All of the callbacks
 * 		should also be configured here.  There are callbacks before and after both load process
 * 		and the save process.  The stack will write into the buffer and read from the buffer
 * 		the calling application is responsible for filling the buffer from the persistent store
 * 		before a load and copying to the persistent store after a write.
 *
 *        One should NOT mix MOP calls with LoaderService or JCOPUpdate calls.
 *        IF MOP_Init() has been called, MOP_Deinit() should be called before calling LoaderService or
 *        JCOPUpdate.
 *
 * \pre NFC stack must be initialized
 * \pre eSE must contain the LTSM applet
 *
 * \param pStore - [in] pointer to the data structure that has the buffer and the callbacks
 *
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * \return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_Init(tMOP_Storage* pStore);

/**
 * \brief clean up after using MOP calls
 *
 * \return none
 */
void SeApi_MOP_Deinit(void);

/**
 * \brief Create a virtual MiFare card using the provided personalization data
 *
 * \param appName			[in] pointer to a c-string containing the name of the app creating the VC (max len is 32)
 * \param vcData			[in] pointer to a buffer containing the data to use for the virtual card (must not be NULL)
 * \param vcDataSize		[in] amount of data in the pVcData buffer
 * \param personalizeData   [in] pointer to a buffer containing the personalization data (can be NULL)
 * \param persoDataSize		[in] amount of data in the personalizeData buffer
 * \param pResp				[in/out] pointer to a buffer that will receive the response data containing information about the creation process
 * \param pRespLen			[in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 *	Response Buffer TLVs (in pResp):
 *		SUCCESS:	40	2		VcEntry
 *		            41  4 or 7	UID
 *		            4E	2		StatusWord (9000)
 *      FAILURE:    4E  2       StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_CreateVirtualCard(const char *appName, const UINT8 *vcData, UINT16 vcDataSize, const UINT8 *personalizeData, UINT16 persoDataSize, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Delete a the specified virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app deleting the VC (max len is 32)
 * \param vcEntry - [in] Identifier for the virtual card to be deleted.
 *                       This is the vcEntry value returned from CreateVirtualCard
 * \param pResp     [in/out] pointer to a buffer that will receive the response data containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 *  Response Buffer TLVs (in pResp):
 *      SUCCESS:    4E  2       StatusWord (9000)
 *      FAILURE:    4E  2       StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_DeleteVirtualCard(const char *appName, UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Activate the specified virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app activating the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card to be activated
 * \param mode		[in] TRUE to enable the card, FALSE to disable the card
 * \param concurrentActivationMode [in] TRUE or FALSE
 * \param pResp     [in/out] pointer to a buffer that will receive the response data containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 *  Response Buffer TLVs (in pResp):
 *      SUCCESS:    4E  2       StatusWord (9000)
 *      FAILURE:    4E  2       StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_ActivateVirtualCard(const char *appName, UINT16 vcEntry, UINT8 mode, UINT8 concurrentActivationMode, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Add and update the MDAC of the virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app updating the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card to be updated
 * \param pVcData	[in] MDAC data
 * \param vcDataLen	[in] length of the MDAC data
 * \param pResp     [in/out] pointer to a buffer that will receive the response data containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 *  Response Buffer TLVs (in pResp):
 *      SUCCESS:    4E  2       StatusWord (9000)
 *      FAILURE:    4E  2       StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_AddAndUpdateMdac(const char *appName, UINT16 vcEntry, const UINT8  *pVcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen);
/**
 * \brief Read data from the virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app reading from the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card to be read
 * \param pVcData	[in] command to be send to the MiFare card
 * \param vcDataLen	[in] length of the data to be sent to the MiFare card
 * \param pResp		[out] buffer to receive the read data response
 * \param pRespLen	[in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 *	Response Buffer TLVs:
 *		SUCCESS:	Mifare Data (variable len)
 *		            4E	2		StatusWord (9000)
 *      FAILURE:    4E  2       StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_ReadMifareData(const char *appName, UINT16 vcEntry, const UINT8 *pVcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Retrieve a list of installed MiFare virtual cards
 *
 * \param pVCList	 [out] buffer to receive the list
 * \param pVCListLen [in/out]  on input, maximum size of the pVCList buff, on output, amount of data written into the buffer
 *
 *	Response Buffer TLVs:
 *		SUCCESS:	61  1A		VcEntry Data
 *						40   2	VcEntry
 *		            	4F   14	20 byte SHA of the app name
 *		            ... more 61 TLVs ...
 *		            4E  2       StatusWord (9000)
 *      FAILURE:    4E  2       StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_GetVcList(UINT8* pVCList, UINT16* pVCListLen);

/**
 * \brief Retrieve the status of the specified virtual card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app reading from the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card for which the status is being requested
 * \param pResp		[out] buffer to receive the status information
 * \param pRespLen	[in/out]  on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 *  Response Buffer TLVs:
 *      SUCCESS:    9F70  1     VcStatus (01 if Active, 00 if deactivated)
 *      FAILURE:    4E    2     StatusWord (2 byte)
 *
 *  \return  SEAPI_STATUS_OK on success
 *  \return  SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	 SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_MOP_GetVcStatus(const char *appName, UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen);
/* @} */
/* @} */
#endif
/* @} */

/**
 *	\brief Antenna and hardware test functions
 */
/** \addtogroup seapi_hwselftest
 * @{ */
#ifdef NXP_HW_SELF_TEST

/**
 *  \anchor PRBS_Modes
 *	\name PRBS modes
 *
 *	      Defines PRBS generating source
 */
/* @{ */
#define SEAPI_TEST_PRBS_FIRMWARE   0x00
#define SEAPI_TEST_PRBS_HARDWARE   0x01
/* @} */
/**
 *  \anchor PRBS_Types
 *	\name PRBS types
 *
 *	      Defines PRBS types
 */
/* @{ */
#define SEAPI_TEST_PRBS_TYPE_9     0x00
#define SEAPI_TEST_PRBS_TYPE_15    0x01
/* @} */
/**
 *  \anchor RF_Technologies
 *	\name RF Technologies
 */
/* @{ */
#define SEAPI_TEST_RF_TECHNOLOGY_A	0x00
#define SEAPI_TEST_RF_TECHNOLOGY_B  0x01
#define SEAPI_TEST_RF_TECHNOLOGY_F  0x02
/* @} */
/**
 *  \anchor Bit_Rates
 *	\name Bit rates
 *
 *	      Note that not all combinations
 *	       of technologies and bitrates are allowed.
 */
/* @{ */
#define SEAPI_TEST_BIT_RATE_106	 0x00
#define SEAPI_TEST_BIT_RATE_212  0x01
#define SEAPI_TEST_BIT_RATE_424  0x02
#define SEAPI_TEST_BIT_RATE_848  0x03
/* @} */

/**
 *	\brief This data structure is used for the antenna test functions.
 *
 *	       There are two use cases.  One is to read the current values,
 *	       the second to to make sure that the antenna values are with
 *	       specified tolerances.
 *
 *	       Case 1: read values - used with SeApi_Test_AntennaMeasure()
 *	       all of the parameters are outputs and only the following are
 *	       filled in:
 *	       wTxdoValue
 *	       wAgcValue
 *	       wAgcValuewithfixedNFCLD
 *	       wAgcDifferentialWithOpen1
 *	       wAgcDifferentailWithOpen2
 *
 *	       Case 2: test tolerances - used with SeApi_Test_AntennaSelfTest()
 *	       all of the parameters are inputs and should be filled in by
 *	       the caller before calling the function
 *	       all of the parameters are used and should be filled in prior to
 *	       calling the function.
 */

typedef struct
{
    /* Txdo Values */
    UINT16            wTxdoValue;                ///< Txdo Measured Value (mA)
    UINT16            wTxdoMeasuredRangeMin;     ///< Txdo Measured Range Max (mA) (range 20-100)
    UINT16            wTxdoMeasuredRangeMax;     ///< Txdo Measured Range Min (mA)
    UINT16            wTxdoMeasuredTolerance;    ///< Txdo Measured Range Tolerance (% - e.g. 5 for 5%)
    /* Agc Values */
    UINT16            wAgcValue;                 ///< Agc Min Value
    UINT16            wAgcValueTolerance;        ///< Txdo Measured Tolerance (% - e.g. 5 for 5%)
    /* Agc values with NFCLD */
    UINT16            wAgcValuewithfixedNFCLD;             ///< Agc Value with Fixed NFCLD Max
    UINT16            wAgcValuewithfixedNFCLDTolerance;    ///< Agc Value with Fixed NFCLD Tolerance
    /* Agc Differential Values With Open/Short RM */
    UINT16            wAgcDifferentialWithOpen1;             ///< Agc Differential With Open
    UINT16            wAgcDifferentialWithOpenTolerance1;    ///< Agc Differential With Open Tolerance 1
    UINT16            wAgcDifferentialWithOpen2;             ///< Agc Differential With Short
    UINT16            wAgcDifferentialWithOpenTolerance2;    ///< Agc Differential With Open Tolerance 2
}AntennaTest_t;      /* Instance of Transaction structure */

/**
 * \brief Test the connection for the specified SWP
 *
 * \param swp_line	[in] 0x01 for SWP1, 0x02 for SWP2 - all others are invalid
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return	SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Test_SWP(UINT8 swp_line);

tSEAPI_STATUS SeApi_Test_Field(UINT8 flag);

/**
 * \brief Perform the antenna measurements
 *
 * \param results - [out] pointer to a results structure.
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return	SEAPI_STATUS_FAILED otherwise
 */

tSEAPI_STATUS SeApi_Test_AntennaMeasure(AntennaTest_t *results);

/**
 * \brief Perform an automated pass/fail test on the antenna using the
 *        limits provided by the caller.
 *
 * \param bounds - [in] pointer to the bounds structure
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return	SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Test_AntennaSelfTest(AntennaTest_t *bounds);

/**
 * \brief Test pin connections.
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return	SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Test_Pins(void);
#endif /* NXP_HW_SELF_TEST */
/* @} */


#endif/*COMPANION_DEVICE*/

//------------------------------------------------------------------------------------
// Loader Service
//------------------------------------------------------------------------------------
#if (LS_ENABLE == TRUE)
/** \addtogroup seapi_sesupport
 * @{ */
/**
 *  \brief Install or delete applets on the embedded secure element.
 *         These functions are available only if SESUPPORT_INCLUDE == TRUE and LS_ENABLE == TRUE.
 */
/** \addtogroup seapi_ls
 * @{ */

/**
 * \brief Process loader service script to update the eSE
 *
 * \pre eSE must contain the loader service applet
 * \pre NFC Stack must be initialized
 *
 * \param scLen - [in] number of bytes in the script buffer
 * \param pScData - [in] pointer to a buffer containing the script
 * \param respLen - [in] size of response buffer, [out] number of bytes stored in response buffer
 * \param pRespData - [in/out] pointer to a buffer to receive the response data
 * \param hashLen - [in] For LS versions before 2.3, number of bytes in the SHA1 hash.
 *                       For LS 2.3, this value should be 20
 * \param pHashData - [in] For LS versions before 2.3, pointer to a buffer containing the SHA1 hash
 *                         For LS 2.3, pointer to a buffer containing a constant 20-byte value
 * \param respSW - [out] 4 byte buffer to receive the response status word
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 * 	\return	SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 * 	\return	SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_LoaderServiceDoScript(INT32 scLen, const UINT8 *pScData,
                                          UINT16 *respLen, UINT8 *pRespData,
                                          UINT16 hashLen, const UINT8 *pHashData,
                                          UINT8 *respSW);
/*
 *\param pChunkData        - [in] Input chunk data
 *\param pChunkDataSize    - [in] Size of the chunk
 *\param pResp             - [out] Pointer to a buffer to receive the response data - Status of each executed command.
 *\param pRespLen          - [in] size of response buffer, [out] number of bytes stored in response buffer
 *\param scriptLen         - [in] Total script length
 *\param pScriptOffset     - [in] Input script offset [out] The offset to the start of next APDU.
 *                           This must not be modified outside,
 *                           since this offset is always updated and aligned to the start of an APDU
 *\param pHashData         - [in] For LS versions before 2.3, pointer to a buffer containing the SHA1 hash
 *                           For LS 2.3, pointer to a buffer containing a constant 20-byte value
 *\param hashLen           - [in] For LS versions before 2.3, number of bytes in the SHA1 hash.
 *                           For LS 2.3, this value should be 20
 *\param pRespSW           - [out] 4 byte buffer to receive the response status word
 *\param pStackCapabilities- [In] Pointer to Stack capabilities
 *
 *\return STATUS_OK on success
 *\return SEAPI_STATUS_BUFFER_FULL : Given buffer is not enough to fit one APDU
 *\return SEAPI_STATUS_NO_ACTION_TAKEN : Nothing processed(may be only metadata) waiting of next data
 *\return STATUS_FAILED : LS execution failed
 *\breif     This function will process the Loader service script provided as chunks.
 *           This function can be used when user gets the data in chunks and sends it for processing.
 *           This function can be called from the wearable device or from the companion.
 *
 *
 *          CODE SNIPPET - Demonstrates reading data from a file and sending for processing in the wearable device.
 *          offset = 0;
 *          while(offset < file_len) {
 *              fseek(in_file, offset, SEEK_SET)
 *              fread(buffer, sizeof(char), chunk_len, in_file)
 *              status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pRespBuff, &respLen, file_len, &offset, pHashData, hashLen, pRespSW, &tackCapabilities);
 *              if(status == SEAPI_STATUS_OK) {
 *					for(j = 0; j< chunkRespLen; j++)
 *						phOsalNfc_Log("%c",*(pChunkResp+j));
 *						//Copy/append the responses for the executed chunk into another buffer as applicable (memcpy(&globalBuffer[actualOffset], pRespBuffer, respLen);
 *				}
 *				else if(status == SEAPI_STATUS_NO_ACTION_TAKEN) {
 *					continue;
 *				}
 *				else {
 *					break;
 *				}
 *          }
 */
tSEAPI_STATUS SeApi_LoaderServiceProcessInChunk(UINT8 *pChunkData, UINT32 *pChunkDataSize,
                                                UINT8 *pResp, UINT16 *pRespLen,
                                                UINT32 scriptLen, UINT32 *pScriptOffset,
                                                const UINT8 *pHashData, UINT16 hashLen,
                                                UINT8 *pRespSW, phStackCapabilities_t *pStackCapabilities);
#ifdef COMPANION_DEVICE
/**
 * \brief Send loader service script to update the remote eSE
 *
 * \pre eSE must contain the loader service applet
 * \pre NFC Stack must be initialized
 *
 * \param scLen - [in] number of bytes in the script buffer
 * \param pScData - [in] pointer to a buffer containing the script
 * \param respLen - [in] size of response buffer, [out] number of bytes stored in response buffer
 * \param pRespData - [in/out] pointer to a buffer to receive the response data
 * \param hashLen - [in] For LS versions before 2.3, number of bytes in the SHA1 hash.
 *                       For LS 2.3, this value should be 20
 * \param pHashData - [in] For LS versions before 2.3, pointer to a buffer containing the SHA1 hash
 *                         For LS 2.3, pointer to a buffer containing a constant 20-byte value
 * \param respSW - [out] 4 byte buffer to receive the response status word
 * \param pChunkSize - [in] struct indicating size of buffers holding the Command and Response Chunks
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 *  \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 *  \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_LoaderServiceRemoteDoScript(INT32 scLen, const UINT8 *pScData,
                                        UINT16 *respLen, UINT8 *pRespData,
                                        UINT16 hashLen, const UINT8 *pHashData,
                                        UINT8 *respSW, phStackCapabilities_t *pStackCapabilities);
#endif

/* @}  */
/* @}  */

#endif
#ifdef __cplusplus
}
#endif
typedef enum {
    SEAPI_SET_CHUNKING = 0,
    SEAPI_NUM_CONFIGS
}SeApiConfig_t;
/**
* \brief This function is used to set configuration parameter value for SeApi
* \param param : [in] Input parameter to set the configuration
* \param val   : [in] Parameter value
* */
tSEAPI_STATUS SeApi_SetConfig(SeApiConfig_t param, void *val);
/**
* \brief This function is used to get parameter value
* \param param : [in]  Input parameter to get the configuration
* \param val   : [out] Parameter value
* */
tSEAPI_STATUS SeApi_GetConfig(SeApiConfig_t param, void *val);
#endif // SEAPI_H
