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
 * \file  SeSupport.h
 *  \brief SeSupport header file.  This is main API for LoaderService,
 *         JCOPUpdate and MiFare OpenPlatform.  This library can be compiled
 *         either independently for the host, or inetegrated with the wearable.
 *         SeSupport has much higher memory buffer requirements than the main
 *         SeApi, because of this, it has been designed such that, if the
 *         wearable itself does not have the resources, SeSupport can be
 *         implemented on the host where one would expence to have more
 *         resources.
 *
 *         When compiled independently, SeSupport would be linked into
 *         the code on the wearable's host device. In this configuration,
 *         the Integrator is responsible for implementing the iChannel
 *         which would provide communication between the host device and
 *         the wearable itself.
 *
 *         When compiled for the wearable itself, this library becomes
 *         part of the SeApi project and is called by SeApi functions.
 *         In this configuration, NXP has provided an implementation of
 *         the iChannel.
 *         The NXP iChannel implementation can be used as a reference
 *         for the integrator.
 *
 *         All SeSupport API functions are BLOCKING functions
 */

/**
 * \defgroup sesupport LoaderService JCOPUpdate and Mifare Open Platform
 */
#ifndef SESUPPORT_H
#define SESUPPORT_H

#ifdef __cplusplus
extern "C"{
#endif
#include "p61_data_types.h"
#include "IChannel.h"
/** \ingroup sesupport
 * @{ */

/**
 * \brief set to TRUE if using a file system for storing JCOP and Cardlet Scripts
 * 	GKI OS abstraction functions for file access functions such as file open, close, seek, fprintf must be
 * 	available.
 *
 * 	set to FALSE if using buffers for passing the JCOP and Cardlet scripts to the
 * 	LoaderService and JCOPUpdate
 *
 * 	Currently, only the buffered version is tested and supported.
 */
#if (SESUPPORT_INCLUDE == TRUE)
#define SESUPPORT_USE_FILE_IO (FALSE)

#define SESUPPORT_STATUS_OK                   0x00          /* Command succeeded    */
#define SESUPPORT_STATUS_BUFFER_FULL          0xE0          /* buffer full          */
#define SESUPPORT_STATUS_FAILED               0x03          /* failed               */

#define MAX_APP_NAME_LENGHTH        32

typedef UINT8 tSESUPPORT_STATUS;

//------------------------------------------------------------------------------------
// General Public Functions
//------------------------------------------------------------------------------------
/**
 * \brief Initialize the system before using the LOADER SERVICE, JCOPLOADER or MOP
 *
 * 			Pre-conditions: NFC stack must be initialized
 * 							IChannel must be implemented
 *
 * \param customIch - [in] the user's custom implementation of the four functions required
 * 					to communicate with the eSE;
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_Init(IChannel_t *customIch);

/**
 * \brief tear down the system after using the LOADER SERVICE, JCOPLOADER or MOP
 *
 *
 * \return none
 */
void SeSupport_DeInit(void);



#if (LS_ENABLE == TRUE)
//------------------------------------------------------------------------------------
// Loader Service
//------------------------------------------------------------------------------------
/**
 * \brief Process loader service script to update the eSE
 *
 *          Pre-condition: eSE must contain the loader service applet
 *                         must initialize SeSupport
 *
 * \param scLen - [in] number of bytes in the script buffer
 * \param pScData - [in] pointer to a buffer containing the script
 * \param respLen - [in] size of response buffer, [out] number of bytes stored in response buffer
 * \param pRespData - [in/out] pointer to a buffer to receive the response data
 * \param hashLen - [in] For LS versions before 2.3, number of bytes in the SHA1 hash.
 *                       For LS 2.3, this value should be 20
 * \param pHashData - [in] For LS versions before 2.3, pointer to a buffer containing the SHA1 hash
 *                         For LS 2.3, pointer to a buffer containing a constant 20-byte value
 * \param respSW - [in/out]pointer to a 4 byte buffer to receive the response status word
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_LoaderServiceDoScript(INT32 scLen, const UINT8 *pScData,
										UINT16 *respLen, UINT8 *pRespData,
										UINT16 hashLen, const UINT8 *pHashData,
										UINT8 *respSW);
#endif
#if (JCOP_ENABLE == TRUE)
//------------------------------------------------------------------------------------
// JCOPUpdate
//------------------------------------------------------------------------------------
/**
 * \brief Structure containing the callback function pointers used by JCOP Update
 *
 *        JCOP Update will call the cbGet function prior to starting the update
 *        process.  The value returned by cbGet will determine where the update
 *        process should begin.  The cbSet function is called after each stage
 *        of the update process, thus keeping track of the successfully completed
 *        update phases.
 *
 *        cbSet - store the byte received in the input parameter into persistent
 *                storage. This is used in case the process is interrupted before
 *                completing.  It will resume at the appropriate stage based on
 *                this value.  This value should persist across reboots and power
 *                failures.  It should be stored in a file or in FLASH
 *
 *       cbGet - retrieve the byte stored by cbSet and return it to the caller
 */
typedef struct
{
	UINT16 (*cbSet)(UINT8); 	// callback to set the status byte
	UINT16 (*cbGet)(UINT8*);	// callback to get the status byte
}tJcopUpdate_StorageCb;

/**
 * \brief Update the JCOP on the eSE.  The caller should allocate buffers for
 *        each of the three scripts and provide those buffers to the function.
 *        These are fairly large scripts (150k - 400k each).
 *        The update takes 1-2 minutes.  The only feedback during update
 *        will be calls to cbGet and cbSet.
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
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_JCOPUpdate(UINT32 lenBuf1, const UINT8 *pBuf1,
		                               UINT32 lenBuf2, const UINT8 *pBuf2,
		                               UINT32 lenBuf3, const UINT8 *pBuf3,
		                               tJcopUpdate_StorageCb *cb);
#endif

/**
 *	\brief Pre-conditions:  eSE must contain the LTSM applet
 * 					NFC stack must be initialized
 */
//------------------------------------------------------------------------------------
// Mifare Open Platform
//------------------------------------------------------------------------------------
#if (MIFARE_OP_ENABLE == TRUE)

/**
 * \brief Structure containing a user supplied scratch buffer used by MOP for storing
 *        status information and callbacks that are used to write that status
 *        information to persistent storage and read it from persistent storage.
 *        This information should be stored in a file or in FLASH
 *
 *        The buffer shall be allocated by the caller.
 *        The buffer should be at least 64 + N(68) where N is the number of virtual cards
 *        in the eSE.
 *
 *        cbPreLoad: REQUIRED - called before MOP attempts to read from the buffer.
 *                   This function should read the data from persistent storage and write it in
 *                   to the provided buffer (param1).  The size of the buffer is provided by the
 *                   caller as (param2).  The function should return the number of bytes written
 *                   into the buffer.  MOP will load from the buffer for it's internal processing
 *
 *       cbPostLoad: OPTIONAL (may be NULL) - called after MOP has read from the buffer - this function does not need
 *                   to perform any action but is provided for status purposes.
 *                   The first parameter is the data buffer from which MOP attempted to read data
 *                   The second parameter is the actual amount of data MOP read from the buffer,
 *                   a zero (0) indicates that MOP failed reading datat from the buffer
 *
 *      cbPreStore: OPTIONAL - (may be NULL) called by MOP before it attempts to write new data into the buffer. - this
 *                  function does not need to perform any action but is provided for status purposes.
 *                  The provided buffer is the one into which MOP will be serializing (writing) it's
 *                  internal data.  The second parameter is how much data it will be writing.
 *
 *      cbPostStore: REQUIRED - called by MOP after it serializes (writes) it's internal data into the buffer
 *                   This function should write the data from the provided buffer into persistent storage.
 *                   This data is used by MOP during it's processing. The amount of data in the buffer
 *                   is provided as the second paramter
 */
typedef struct
{
	UINT8*	buffer;				// pointer to a buffer
	UINT16	bufferSize;			// size of the buffer
	//UINT16 (*cbPreLoad)(UINT8*, UINT16);// called by MOP before it attempts to read data from the buffer
	                                    // param1 is the buffer to be filled in by the callback
	                                    // param2 is the size of the buffer
	                                    // return should be the number of bytes written into the buffer
	//void (*cbPostLoad)(UINT8*, UINT16);	// called after MOP read from the buffer,
	                                    // receiving a 0 (param2) indicates a failure reading from the buffer
	                                    // non-zero (param2) indicates how much was read from the buffer
	//void (*cbPreStore)(UINT8*, UINT16); // called before MOP attempts to store data into the buffer,
	                                    // input param2 is the amount of data expected to be stored into the buffer param1
	//void (*cbPostStore)(UINT8*, UINT16);// called after the MOP stores into the buffer,
	                                    // input param2 is the amount of data actually stored
										// Input param1 is the buffer MOP wrote data into
}tMOP_Storage;

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
 *	    FAILURE:	2 byte StatusWord
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_CreateVirtualCard(const char *appName, const UINT8 *vcData, UINT16 vcDataSize, const UINT8 *personalizeData, UINT16 persoDataSize, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Delete a the specified virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app deleting the VC (max len is 32)
 * \param vcEntry - [in] Identifier for the virtual card to be deleted.
 *                       This is the vcEntry value returned from CreateVirtualCard
 * \param pResp     [in/out] pointer to a buffer that will receive the response data containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_DeleteVirtualCard(const char *appName, UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen);

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
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_ActivateVirtualCard(const char *appName, UINT16 vcEntry, UINT8 mode, UINT8 concurrentActivationMode, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Add and update the MDAC of the virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app updating the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card to be updated
 * \param pVcData	[in] MDAC data
 * \param vcDataLen	[in] len of the MDAC data
 * \param pResp     [in/out] pointer to a buffer that will receive the response data containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_AddAndUpdateMdac(const char *appName, UINT16 vcEntry, const UINT8  *pVcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen);

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
 *	    FAILURE:	2 byte StatusWord
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_ReadMifareData(const char *appName, UINT16 vcEntry, const UINT8 *pVcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen);

/**
 * \brief Retrieve a list of installed MiFare virtual cards
 *
 * \param pVCList	[out] buffer to receive the list
 * \param pVCListLen	[in/out]  on input, maximum size of the pVCList buff, on output, amount of data written into the buffer
 *
 *	Response Buffer TLVs:
 *		SUCCESS:	61  1E		VcEntry Data
 *						40   2	VcEntry
 *		            	4F   14	20 byte SHA of the app name
 *		            	9F70 1  status of the virtual card
 *		            ... more 61 TLVs ...
 *		            4E  2  StatusWord
 *	    FAILURE:	2 byte StatusWord
 *
 * \return SESUPPORT_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_GetVcList(UINT8* pVCList, UINT16* pVCListLen);

/**
 * \brief Retrieve the status of the specified virtual card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app reading from the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card for which the status is being requested
 * \param pResp		[out] buffer to receive the status information
 * \param pRespLen	[in/out]  on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_GetVcStatus(const char *appName, UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen);

#endif/*MIFARE_OP_ENABLE*/
#endif/*SESUPPORT_INCLUDE*/
/** @} */

#ifdef __cplusplus
}
#endif

#endif // SESUPPORT_H
