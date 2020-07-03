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

#include "SeSupport.h"
#include "AlaLib.h"
#include "mop.h"
#include "registry.h"
#include "JcDnld.h"
#include "nfa_api.h"
#ifndef COMPANION_DEVICE
#include "OverrideLog.h"
#endif
#if (SESUPPORT_INCLUDE == TRUE)

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG 			"SeSupport"

static IChannel_t	gIChannel;
static UINT8		gSeSupport_Initialized;

//------------------------------------------------------------------------------------
// Forward declarations - private functions, used internally only
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// General Public Functions
//------------------------------------------------------------------------------------
/**
 * \brief Initialize the system before using the LOADER SERVICE or the JCOPLOADER
 *
 * 			Pre-conditions: eSE must contain the loader service applet
 * 							NFC stack must be initialized
 * 							IChannel must be implemented
 *
 * \param IChannel - the user's custom implementation of the four functions required
 * 					to communicate with the eSE;
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_Init(IChannel_t *customIch)
{
    tJBL_STATUS status;
    tSESUPPORT_STATUS result;

    if(gSeSupport_Initialized == TRUE)
        return SESUPPORT_STATUS_OK;

    result = SESUPPORT_STATUS_FAILED;
    gSeSupport_Initialized = FALSE;

    if(NULL != customIch)
    {
        gIChannel.open = customIch->open;
        gIChannel.close = customIch->close;
        gIChannel.transceive = customIch->transceive;
        gIChannel.doeSE_Reset = customIch->doeSE_Reset;
        gIChannel.doeSE_JcopDownLoadReset = customIch->doeSE_JcopDownLoadReset;
        gIChannel.control = customIch->control;

#if (MIFARE_OP_ENABLE == TRUE)
        status = MOP_Init(&gIChannel);
        if( STATUS_SUCCESS != status )
        {
            MOP_DeInit();
            goto SeSupport_Init_bail;
        }
#endif

        result = SESUPPORT_STATUS_OK;
        gSeSupport_Initialized = TRUE;
    }

SeSupport_Init_bail:
    return result;
}

/**
 * \brief tear down the system after using the LOADER SERVICE or the JCOPLOADER
 *
 *
 * \return none
 */
void SeSupport_DeInit(void)
{
    if(gSeSupport_Initialized == TRUE)
    {
#if (MIFARE_OP_ENABLE == TRUE)
        MOP_DeInit();
#endif
        gSeSupport_Initialized = FALSE;
    }
}
#if (LS_ENABLE == TRUE)
//------------------------------------------------------------------------------------
// Loader Service
//------------------------------------------------------------------------------------
/**
 * \brief BUFFER VERSION
 * 			Process loader service script to update the eSE
 * 			BLOCKING - this will block until the loader service
 * 			script has finished processing
 *
 * 			Pre-conditions:  eSE must contain the loader service applet
 * 							NFC stack must be initialized
 *
 * \param scLen - number of bytes in the script buffer
 * \param pScData - pointer to a buffer containing the script
 * \param respLen - input: size of response buffer, output: bytes stored in response buffer
 * \param pRespData - pointer to a buffer to receive the response data
 * \param hashLen - For LS versions before 2.3, number of bytes in the SHA1 hash.
 *                       For LS 2.3, this value should be 20
 * \param pHashData - For LS versions before 2.3, pointer to a buffer containing the SHA1 hash
 *                         For LS 2.3, pointer to a buffer containing a constant 20-byte value
 * \param respSW - pointer to a 4 byte buffer to receive the response status word
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_LoaderServiceDoScript(INT32 scLen, const UINT8 *pScData,
                                        UINT16 *respLen, UINT8 *pRespData,
                                        UINT16 hashLen, const UINT8 *pHashData,
                                        UINT8 *respSW)
{
    tJBL_STATUS status;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;
    UINT8 chunkmarker;
    if(Ala_IsChunkingEnabled() == TRUE)
        chunkmarker = pScData[scLen- CHUNK_MARKER_OFFSET];
    else
        chunkmarker = LS_FIRST_CHUNK;

    if(respSW != NULL)
    {
        memset(respSW, 0, 4);
    }

    if(gSeSupport_Initialized)
    {
        if((chunkmarker == LS_FIRST_CHUNK) || (chunkmarker == LS_CHUNK_NOT_REQUIRED))
        {
            status = ALA_Init(&gIChannel);
            if( NFA_STATUS_OK != status )
            {
                goto SeSupport_LoaderServiceDoScriptExit;
            }
        }

        status = ALA_Start(scLen, (const char *) pScData, respLen, (const char *) pRespData, (UINT8 *) pHashData, hashLen, respSW);
        if(status == STATUS_BUFFER_OVERFLOW)
        {
            result = SESUPPORT_STATUS_BUFFER_FULL;
        }
        else if(NFA_STATUS_OK != status)
        {
            result = SESUPPORT_STATUS_FAILED;
            goto SeSupport_LoaderServiceDoScriptExit;
        }
        else
        {
            result = SESUPPORT_STATUS_OK;
        }
    }

SeSupport_LoaderServiceDoScriptExit:
    if(Ala_IsChunkingEnabled() == FALSE)
        chunkmarker = LS_LAST_CHUNK;

    if(((chunkmarker == LS_LAST_CHUNK) || (chunkmarker == LS_CHUNK_NOT_REQUIRED))  && (result != SESUPPORT_STATUS_BUFFER_FULL))
    {
        if(gSeSupport_Initialized)
            ALA_DeInit();
    }
    return result;
}
#endif

#if (JCOP_ENABLE == TRUE)
//------------------------------------------------------------------------------------
// JCOPLoader
//------------------------------------------------------------------------------------
/**
 * \brief BUFFER VERSION
 * 			Update the JCOP on the eSE
 *
 * \param len1 - number of bytes in the 1st JCOP update buffer
 * \param pBuf1 - pointer to a buffer containing the 1st JCOP update data
 * \param len2 - number of bytes in the 2nd JCOP update buffer
 * \param pBuf2 - pointer to a buffer containing the 2nd JCOP update data
 * \param len3 - number of bytes in the 3rd JCOP update buffer
 * \param pBuf3 - pointer to a buffer containing the 3rd JCOP update data
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_JCOPUpdate(UINT32 lenBuf1, const UINT8 *pBuf1,
                                       UINT32 lenBuf2, const UINT8 *pBuf2,
                                       UINT32 lenBuf3, const UINT8 *pBuf3,
                                       tJcopUpdate_StorageCb *cb)
{
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;
    tSESUPPORT_STATUS status;

    if(gSeSupport_Initialized)
    {
        ALOGD("--- STARTING JCDNLD_Init -------");
        status = JCDNLD_Init(&gIChannel, (tJcopOs_StorageCb *) cb);
        if( NFA_STATUS_OK != status )
        {
            goto SeSupport_JCOPUpdateExit;
        }

        ALOGD("--- STARTING JCDNLD_StartDownload -------");
        gIChannel.control();
        status = JCDNLD_StartDownload(lenBuf1, pBuf1, lenBuf2, pBuf2, lenBuf3, pBuf3);
        if(NFA_STATUS_OK != status)
        {
            result = SESUPPORT_STATUS_FAILED;
            goto SeSupport_JCOPUpdateExit;
        }
        else
        {
            result = SESUPPORT_STATUS_OK;
        }
    }

SeSupport_JCOPUpdateExit:
    if(gSeSupport_Initialized)
        JCDNLD_DeInit();

    return result;
}
#endif
//------------------------------------------------------------------------------------
// Mifare Open Platform
//------------------------------------------------------------------------------------
// This is used if the MOP is NOT included in the main stack.
// If it is included use the SeApi
#if (MIFARE_OP_ENABLE == TRUE)

/**
 * \brief Create a virtual MiFare card using the provided personalization data
 *
 * \param appName			[in] pointer to a c-string containing the name of the app creating the VC (max len is 32)
 * \param pVcData			[in] pointer to a buffer containing the data to use for the virtual card (must not be NULL)
 * \param vcDataSize		[in] amount of data in the pVcData buffer
 * \param pPersonalizeData  [in] pointer to a buffer containing the personalization data (can be NULL)
 * \param persoDataSize		[in] amount of data in the personalizeData buffer
 * \param pResp				[out] response containing information about the creation process
 * \param pRespLen			[in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_CreateVirtualCard(const char *appName, const UINT8 *vcData, UINT16 vcDataSize, const UINT8 *personalizeData, UINT16 persoDataSize, UINT8 *pResp, UINT16 *pRespLen)
{
    tJBL_STATUS status;
    UINT16	sw;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if((appName != NULL) && (vcData != NULL) && (vcDataSize != 0) && (pResp != NULL) && (*pRespLen >= 4))
    {
        memset(pResp, 0, 4);

        if(gSeSupport_Initialized)
        {
            status = MOP_setCurrentPackageInfo(appName);
            if(status != STATUS_SUCCESS)
                return result;

            sw = MOP_CreateVirtualCard(vcData, vcDataSize, personalizeData, persoDataSize, pResp, pRespLen);
            if(sw == 0x9000)
                result = SESUPPORT_STATUS_OK;
        }
    }

    return result;
}

/**
 * \brief Delete a the specified virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app reading from the VC (max len is 32)
 * \param vcEntry - [in] Identifier for the virtual card to be deleted.
 *                       This is the vcEntry value returned from CreateVirtualCard
 * \param pResp     [out] response containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_DeleteVirtualCard(const char *appName, UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 sw;
    tJBL_STATUS status;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if(gSeSupport_Initialized)
    {
        status = MOP_setCurrentPackageInfo(appName);
        if(status != STATUS_SUCCESS)
            return result;

        sw = MOP_DeleteVirtualCard(vcEntry, pResp, pRespLen);
        if(sw == 0x9000)
            result = SESUPPORT_STATUS_OK;
    }

    return result;
}

/**
 * \brief Activate the specified virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app reading from the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card to be activated
 * \param mode		[in] TRUE to enable the card, FALSE to disable the card
 * \param pResp     [out] response containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_ActivateVirtualCard(const char *appName, UINT16 vcEntry, UINT8 mode, UINT8 concurrentActivationMode, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 sw;
    tJBL_STATUS status;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if(gSeSupport_Initialized)
    {
        status = MOP_setCurrentPackageInfo(appName);
        if(status != STATUS_SUCCESS)
            return result;

        sw = MOP_ActivateVirtualCard(vcEntry, mode, concurrentActivationMode, pResp, pRespLen);
        if(sw == 0x9000)
            result = SESUPPORT_STATUS_OK;
    }

    return result;
}

/**
 * \brief Add and update the MDAC of the virtual MiFare card
 *
 * \param appName	[in] pointer to a c-string containing the name of the app reading from the VC (max len is 32)
 * \param vcEntry	[in] Identifier for the virtual card to be updated
 * \param pVcData	[in] MDAC data
 * \param cdDataLen	[in] len of the MDAC data
 * \param pResp     [out] response containing information about the creation process
 * \param pRespLen  [in/out] on input, maximum size of the pResp buff, on output, amount of data written into the buffer
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_AddAndUpdateMdac(const char *appName, UINT16 vcEntry, const UINT8  *pVcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 sw;
    tJBL_STATUS status;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if(gSeSupport_Initialized)
    {
        status = MOP_setCurrentPackageInfo(appName);
        if(status != STATUS_SUCCESS)
            return result;

        sw = MOP_AddAndUpdateMdac(vcEntry, pVcData, vcDataLen, pResp, pRespLen);
        if(sw == 0x9000)
            result = SESUPPORT_STATUS_OK;
    }

    return result;
}

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
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_ReadMifareData(const char *appName, UINT16 vcEntry, const UINT8 *pVcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 sw;
    tJBL_STATUS status;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if(gSeSupport_Initialized)
    {
        status = MOP_setCurrentPackageInfo(appName);
        if(status != STATUS_SUCCESS)
            return result;

        sw = MOP_ReadMifareData(vcEntry, pVcData, vcDataLen, pResp, pRespLen);
        if(sw == 0x9000)
            result = SESUPPORT_STATUS_OK;
    }

    return result;
}

/**
 * \brief Retrieve a list of installed MiFare virtual cards
 *
 * \param pVCList	 [out] buffer to receive the list
 * \param pVCListLen [in/out]  on input, maximum size of the pVCList buff, on output, amount of data written into the buffer
 *
 * \return SEAPI_STATUS_OK on success
 */
tSESUPPORT_STATUS SeSupport_MOP_GetVcList(UINT8* pVCList, UINT16* pVCListLen)
{
    UINT16 sw;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if(gSeSupport_Initialized)
    {
        sw = MOP_GetVcList(pVCList, pVCListLen);
        if((sw == 0x9000) || (sw == 0x6A88)) /*Treat 0x6A88 as success as it will be returned if there is no card present*/
            result = SESUPPORT_STATUS_OK;
    }

    return result;
}

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
tSESUPPORT_STATUS SeSupport_MOP_GetVcStatus(const char *appName, UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 sw;
    tJBL_STATUS status;
    tSESUPPORT_STATUS result = SESUPPORT_STATUS_FAILED;

    if(gSeSupport_Initialized)
    {
        status = MOP_setCurrentPackageInfo(appName);
        if(status != STATUS_SUCCESS)
            return result;

        sw = MOP_GetVirtualCardStatus(vcEntry, pResp, pRespLen);
        if(sw == 0x9000)
            result = SESUPPORT_STATUS_OK;
    }

    return result;
}
#endif/*MIFARE_OP_ENABLE*/
#endif/*SESUPPORT_INCLUDE*/
//------------------------------------------------------------------------------------
// Private Functions
//------------------------------------------------------------------------------------
