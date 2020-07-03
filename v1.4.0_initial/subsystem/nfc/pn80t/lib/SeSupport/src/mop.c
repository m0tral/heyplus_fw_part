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

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG 			"NXP_MOP"

#include "registry.h"
#include "channelMgr.h"
#include "utils.h"
#include "vcDescription.h"
#include "mop.h"
#include "p61_data_types.h"
#if(MIFARE_OP_ENABLE == TRUE)
#include "statusCodes.h"
#include "IChannel.h"
#include "tlv.h"
#ifndef COMPANION_DEVICE
#include "OverrideLog.h"
#include "gki.h"
#endif
typedef enum ProcessState
{
	eProcess_None				= 0,
	eProcess_CreateVirtualCard	= 1,
	eProcess_HandlePersonalize	= 2,
	eProcess_DeleteVirtualCard	= 3,
	eProcess_AddAndUpdateMdac	= 4
}tProcessState;

tProcessState		gCurrentProcess;	// what is currently happening
hREGISTRY 	gRegistry;			// memory object for storing the VC registry
char*      	gCurrentPackage;	// name of current "package" - this is an NULL TERMINATED arbitrary name provided by the caller
UINT16		gStatusCode;		// status word of the last transaction
UINT16		gInitialVCEntry;
UINT16		gCurrentVCEntry;
UINT16      gDeleteVCEntry;
UINT8		gTmpRespBuf[1024]; /* WorstCase: GetVCList with 15 VCs can occupy 68*15+2=1022 bytes */
UINT8		gApduBuf[265];
IChannel_t	*gIChannel;
UINT8		gInitialized;
UINT8		gVcUid[10];
UINT16		gVcUidLen;
UINT8		gPreviousConcurrentActivationMode;

static const UINT8 data_openChannelAPDU[]    = {0x00, 0x70, 0x00, 0x00, 0x01};
static const UINT8 data_selectAPDU[]         = {0x00, 0xA4, 0x04, 0x00};
static const UINT8 data_SELECT_CRS[]         = {0x00, 0xA4, 0x04, 0x00, 0x09, 0xA0, 0x00, 0x00, 0x01, 0x51, 0x43, 0x52, 0x53, 0x00, 0x00 };

static const UINT8 data_AID_M4M_LTSM[]       = {0xA0, 0x00, 0x00, 0x03, 0x96, 0x4D, 0x46, 0x00, 0x4C, 0x54, 0x53, 0x4D, 0x01};
/* data_ACTV_LTSM_AID is unused - so commenting it off */
/*static const UINT8 data_ACTV_LTSM_AID[]      = {0xA0, 0x00, 0x00, 0x03, 0x96, 0x4D, 0x34, 0x4D, 0x14, 0x00, 0x81, 0xDB, 0x69, 0x00};*/
static const UINT8 data_SERVICE_MANAGER_AID[]= {0xA0, 0x00, 0x00, 0x03, 0x96, 0x4D, 0x34, 0x4D, 0x24, 0x00, 0x81, 0xDB, 0x69, 0x00};
static const UINT8 data_VC_MANAGER_AID[]     = {0xA0, 0x00, 0x00, 0x03, 0x96, 0x4D, 0x34, 0x4D, 0x14, 0x00, 0x81, 0xDB, 0x69, 0x00};
static const UINT8 data_NONCONCURRENT_AID_PARTIAL[] = {0xA0, 0x00, 0x00, 0x03, 0x96, 0x4D, 0x34, 0x4D, 0x14};
static const UINT8 data_DESFIRE_APP_AID[] = {0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x00};


static const UINT8 data_CLA_M4M_LTSM                     = 0x80;
static const UINT8 data_INS_M4M_LTSM_PROCESS_COMMAND     = 0xA0;
static const UINT8 data_INS_M4M_LTSM_PROCESS_SE_RESPONSE = 0xA2;

static const UINT16 data_PrimitiveTagSet[]        = {0x00A8, 0x00E2, 0x00F8};
static const UINT16 data_RemoveTagSet[]			  = {0x00E2, 0x00F8};

static UINT32 gMOPTransceiveTimeout = 120000;

#define ARRAY_COUNT(X)	(sizeof(X) / sizeof(X[0]))
//------------------------------------------------------
// Forward Declarations
//------------------------------------------------------
UINT16 ltsmStart(void);
UINT16 ltsmSelect(void);
UINT16 openLogicalChannel(void);
void closeLogicalChannel(void);
tJBL_STATUS openConnection(void);
tJBL_STATUS closeConnection(void);

UINT16 handlePersonalize(const UINT8 *vcData, UINT16 vcDataLen, const char *pkg, UINT16 pkgLen, UINT16 intValue);
void exchangeLtsmProcessCommand(UINT8* pCData, UINT16 cDataLen, UINT8* rapdu, UINT16* rapduLen);

tJBL_STATUS exchangeApdu(const UINT8* apdu, UINT16 apduLen, UINT8* rapdu, UINT16* rapduLen);
tJBL_STATUS exchangeWithSe(UINT8* cApdu, UINT16 cApduLen, UINT8* rapdu, UINT16* rapduLen);
tJBL_STATUS exchange(UINT8* cApdu, UINT16 cApduLen, int chnl_id, UINT8* rapdu, UINT16* rapduLen);

UINT16 deleteTempVc(UINT16 vcEntry, UINT8* shaWalletName);
void processCreateVcResponse(UINT8* rapdu, UINT16 rapduLen);
UINT16 crsSelect(void);
void removeTags(const UINT8* vcData, UINT16 vcDataLen, const UINT16* tags, UINT8 tagsCount, UINT8* resultData, UINT16* resDataLen);
UINT16 manageConcurrencyActivation(UINT16 vcEntry);
UINT16 manageNonConcurrencyActivation(UINT8* vcmAid, UINT16 vcmAidLen);

void getStatus(UINT8* data, UINT16 dataLen, UINT8* status, UINT16 *pStatusLen);
void setStatus(UINT8* data, UINT16 dataLen, UINT8 mode, UINT8* pResp, UINT16 respBufSize, UINT16 *pRespActSize);

UINT16 deactivateAllContactlessApplications(void);
t_TLV* getTlvA0A2(UINT8* setStatusRsp, UINT16 setStatusRspLen);
UINT16 getSubsequentTlvs(int startNode, UINT16 tagToFind, t_tlvList* parseTlvs, UINT8* foundTlvs, UINT16 foundTlvSize);


//------------------------------------------------------
// Public APIs
//------------------------------------------------------

tJBL_STATUS MOP_setCurrentPackageInfo(const char *packageName)
{
	if(packageName == NULL || (strlen(packageName) >= MAX_APP_NAME) )
	{
		gCurrentPackage = NULL;
		return STATUS_FAILED;
	}

	gCurrentPackage = (char *) packageName;

	return STATUS_SUCCESS;
}

tJBL_STATUS MOP_Init(IChannel_t *channel)
{
	if(channel != NULL)
	{
		gIChannel = channel;
		gRegistry = registry_Init();
		gInitialized = TRUE;
		gCurrentVCEntry = 0;
		gInitialVCEntry = 0;
		gPreviousConcurrentActivationMode = TRUE;
		return STATUS_SUCCESS;
	}

	return STATUS_FAILED;
}

void MOP_DeInit()
{
	gIChannel = NULL;
	registry_DeInit(&gRegistry);
	gInitialized = FALSE;
}
/**
 *
 *  Creates the Virtual Card
 *  The second argument-Personalise Data is optional and it is not mandatory to pass.
 *  If VC Creation configuration is set to 1, then Personalise Data shall be also provided.
 *
 *@param    vcData		 [in] virtual card data
 *@param    vcDataLen	 [in] length of the virtual card data
 *@param    persoData    [in] personalization data
 *@param    persoDataLen [in] length of the personalization data
 *@param    pResp		 [out] pointer to a buffer to receive the response data
 *@param    pRespLen	 [in/out] pointer to a UINT16.  On input, this is the max size of the pResp buffer
 *                                on output, this is the size of the data written into the pResp buffer
 *
 *@return   Status Word
 *@return   If success then the VC entry number and VCUID are returned
 *          to the SP Application along with status word in the pResp buffer. Otherwise an error code.
 *
 */
UINT16 MOP_CreateVirtualCard(const UINT8 *vcData, UINT16 vcDataLen, const UINT8 *persoData, UINT16 persoDataLen, UINT8 *pResp, UINT16 *pRespLen)
{
    tJBL_STATUS	res;
    UINT16 status;
    UINT16 rStatus;
    UINT8 personalize;
    UINT16 respBufSize;
    UINT16 bufSize;
    UINT8 *pBuf;
    UINT16 vcEntry;
    UINT8 sha[20];
    UINT8 vcDataTemp[265];
    t_tlvList* tlvParsed;
    t_TLV* tlvResult;
    t_TLV* tlvResultF8;
    t_TLV* tlvResultE2;
    UNUSED(personalize);

    // Data Initialization
	gCurrentProcess = eProcess_CreateVirtualCard;

	gStatusCode = 0;
	res = STATUS_FAILED;
	personalize = FALSE;

    ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);
	ALOGI("%s: vcData :", __func__);
    utils_LogByteArrayToHex("CreateVirtualCard Input Data: vcData", vcData, vcDataLen);

    if(vcData == NULL || pResp == NULL || *pRespLen < 4)
    {
    	*pRespLen = 0;
    	status = eERROR_INVALID_PARAM;
    	goto MOP_CreateVirtualCard_BAIL;
    }

    if(gCurrentPackage == NULL)
    {
		ALOGI("%s: Package Name not specified", __func__);
		status = eERROR_NO_PACKAGE;
		goto MOP_CreateVirtualCard_BAIL;
    }

    respBufSize = *pRespLen;
    *pRespLen = 0;

	//----------------------------------------------------------------
	// Initiate the LTSM. Open Logical Channel and Select LTSM
	rStatus = ltsmStart();
	if(rStatus != eSW_NO_ERROR){
		ALOGI("%s: LTSMStart Failed", __func__);
		status = rStatus;
		goto MOP_CreateVirtualCard_BAIL;
	}

	//----------------------------------------------------------------
	// Get VC Entry number from the LTSM 2.0 applet on eSE
	bufSize = vcDescriptionFile_GetVC(gApduBuf, sizeof(gApduBuf));
	respBufSize = sizeof(gTmpRespBuf);
    exchangeLtsmProcessCommand(gApduBuf, bufSize, gTmpRespBuf, &respBufSize);
	ALOGI("%s: getVCEntry:", __func__);
    utils_LogByteArrayToHex("getVCEntry: ", gApduBuf, bufSize);
    utils_LogByteArrayToHex("getVCEntry RESP: ", gTmpRespBuf, respBufSize);

    tlvParsed = tlv_parse(gTmpRespBuf, respBufSize-2);
    tlvResult = tlvList_find(tlvParsed, 0x44);	// check that the operation was successful

    tlv_getValue(tlvResult, &pBuf, &respBufSize);
    status = utils_htons(*((UINT16 *)pBuf));
    if(respBufSize != 2 || pBuf == NULL || status != eSW_NO_ERROR)
    {
    	ALOGI("%s: VCEntry Error : ", __func__);
        if(pBuf != NULL)// CID10870
        {
            utils_LogByteArrayToHex("getVCEntry: ", pBuf, respBufSize);
            status = utils_htons(*((UINT16 *)pBuf));
        }
        // free memory allocated to tlvParsed
        tlvList_delete(tlvParsed);
        tlvParsed = NULL;
        tlvResult = NULL;
		goto MOP_CreateVirtualCard_BAIL;
    }

    // Get the actual VC number
    tlvResult = tlvList_find(tlvParsed, 0x40);
    tlv_getValue(tlvResult, &pBuf, &respBufSize);
    vcEntry = utils_htons(*((UINT16 *)pBuf));
    tlvList_delete(tlvParsed);
    tlvParsed = NULL;
    tlvResult = NULL;

	utils_CreateSha(sha, (UINT8 *) gCurrentPackage, strlen(gCurrentPackage));

	//----------------------------------------------------------------
	// Perform the personalization
	ALOGD("%s: -- starting the personalization process", __func__);
	if(persoData != NULL){
		rStatus = handlePersonalize(persoData, persoDataLen, (const char *) gCurrentPackage, strlen(gCurrentPackage), vcEntry);
		if(rStatus != eSW_NO_ERROR){
			ALOGD("%s: Error Personalization : %s", __func__, utils_getStringStatusCode(gStatusCode));
			status = rStatus;
			goto MOP_CreateVirtualCard_BAIL;
		}
	}

	//----------------------------------------------------------------
	// Build the CreateF8VC command if needed

	// check for the F8 tag in the INPUT data
	tlvParsed = tlv_parseWithPrimitiveTags(vcData, vcDataLen, data_PrimitiveTagSet, ARRAY_COUNT(data_PrimitiveTagSet));
	tlvResultF8 = tlvList_find(tlvParsed, kTAG_MF_DF_AID);
	tlvResultE2 = tlvList_find(tlvParsed, kTAG_MF_AID);

	// strip the E2/F8 tags if present
	respBufSize = sizeof(gTmpRespBuf);
	removeTags(vcData, vcDataLen, data_RemoveTagSet, ARRAY_COUNT(data_RemoveTagSet), gTmpRespBuf, &respBufSize);
	memcpy(vcDataTemp, gTmpRespBuf, respBufSize);
	memset(vcDataTemp + respBufSize, 0, vcDataLen - respBufSize);
	vcDataLen = respBufSize;

	if(tlvResultE2 != NULL || tlvResultF8 != NULL)
	{
		UINT8 mfdfaid[16];
		UINT8 cltecap[16];
		UINT16 mfdfaidLen;
		UINT16 cltecapLen;

		if(tlvResultF8 != NULL)
		{
			tlv_getValue(tlvResultF8, &pBuf, &mfdfaidLen);
			memcpy(mfdfaid, pBuf, mfdfaidLen);
		}
		else
		{
			mfdfaidLen = 0;
			memset(mfdfaid, 0, 16);
		}

		if(tlvResultE2 != NULL)
		{
			tlv_getValue(tlvResultE2, &pBuf, &cltecapLen);
			memcpy(cltecap, pBuf, cltecapLen);
		}
		else
		{
			cltecapLen = 0;
			memset(cltecap, 0, 16);
		}

		ALOGD("%s: E2/F8 Present: ", __func__);
		bufSize = sizeof(gApduBuf);
		vcDescriptionFile_CreateF8Vc(vcEntry, sha,
				mfdfaid, mfdfaidLen, cltecap, cltecapLen,
				gApduBuf, sizeof(gApduBuf), &bufSize);

		respBufSize = sizeof(gTmpRespBuf);
	    exchangeLtsmProcessCommand(gApduBuf, bufSize, gTmpRespBuf, &respBufSize);
	    status = utils_GetSW(gTmpRespBuf, respBufSize);
	    if(status != eSW_NO_ERROR)
	    {
	    	/*
	    	 * Fix for CID 10687
	    	 */
	    	tlvList_delete(tlvParsed);
	    	tlvParsed = NULL;
	    	tlvResultF8 = NULL;
	    	tlvResultE2 = NULL;
			goto MOP_CreateVirtualCard_BAIL;
	    }
	}
	tlvList_delete(tlvParsed);
	tlvParsed = NULL;
	tlvResultF8 = NULL;
	tlvResultE2 = NULL;

	//----------------------------------------------------------------
	// Configure the current process state used during the process
	// response if CreateVC succeeds
	gCurrentProcess = eProcess_CreateVirtualCard;


	//----------------------------------------------------------------
	// Build the CreateVC command
	res = vcDescriptionFile_CreateVc(vcEntry, vcDataTemp, vcDataLen, sha, gApduBuf, sizeof(gApduBuf), &bufSize);
	if(res != STATUS_SUCCESS)
	{
	    goto MOP_CreateVirtualCard_BAIL;
	}
	ALOGI("%s: CreateVC : ", __func__);
    utils_LogByteArrayToHex("data: ", gApduBuf, bufSize);

	respBufSize = sizeof(gTmpRespBuf);
    exchangeLtsmProcessCommand(gApduBuf, bufSize, gTmpRespBuf, &respBufSize);
    status = utils_GetSW(gTmpRespBuf, respBufSize);

    if(status == eSW_NO_ERROR)
    {
    	processCreateVcResponse(gTmpRespBuf, respBufSize);
		vcDescriptionFile_CreateVCResp(vcEntry, gVcUid, gVcUidLen, pResp, sizeof(gTmpRespBuf), pRespLen);
		status = utils_GetSW(pResp, *pRespLen);
    }

MOP_CreateVirtualCard_BAIL:

    closeLogicalChannel();
    if(eSW_NO_ERROR != status)
    {
        *pRespLen = utils_createStatusCode(pResp, status);
    }
    ALOGD("%s: StatusWord: %04X", __func__, status);
	ALOGI(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
	return status;
}

/**
 *
 *  Deletes the specified Virtual Card
 *
 *@param    vcEntry		 [in] entry number of the VC to delete (returned by CreateVirtualCard)
 *@param    pResp        [out] pointer to a buffer to receive the response data
 *@param    pRespLen     [in/out] pointer to a UINT16.  On input, this is the max size of the pResp buffer
 *                                on output, this is the size of the data written into the pResp buffer
 *
 *@return   Status Word - 9000 for success, other for error
 *
 */
UINT16 MOP_DeleteVirtualCard(UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen)
{
	UINT16 	status, rStatus;
    UINT16	respLen;
    UINT8	sha[20];

    ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);
    ALOGD("%s: vcEntry = %d", __func__, vcEntry);

    if(gCurrentPackage == NULL)
    {
		ALOGI("%s: Package Name not specified", __func__);
		status = eERROR_NO_PACKAGE;
		goto MOP_DeleteVirtualCard_BAIL;
    }

    //-----
    // Initialize globals
    gStatusCode = 0;
    gCurrentProcess = eProcess_DeleteVirtualCard;
    utils_CreateSha(sha, (UINT8 *)gCurrentPackage, strlen(gCurrentPackage));

    //-----
    // Open a connection to the LTSM applet on the SE
    rStatus = ltsmStart();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: LTSMStart Failed", __func__);
		status = rStatus;
		goto MOP_DeleteVirtualCard_BAIL;
    }

    //-----
    // create the deletion string based on the input info
    rStatus = vcDescriptionFile_DeleteVc(vcEntry, sha, gApduBuf, sizeof(gApduBuf), &respLen);

    //-----
    // perform the deletion process
	exchangeLtsmProcessCommand(gApduBuf, respLen, gTmpRespBuf, &respLen);
	status = utils_GetSW(gTmpRespBuf, respLen);

	if(status == eSW_NO_ERROR)
	{
		// The delete was successful, now clean up the registery (i.e. remove the
		//	entry from the registry IF there are no more VCs for the current app)

		respLen = sizeof(gTmpRespBuf);
		getStatus(NULL, 0, gTmpRespBuf, &respLen);

		t_tlvList *getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
		if(getStatusRspTlvs != NULL)
		{
			// Iterate through the list of remaining VCs, if the ID of the one we just
			// deleted is found, then match_found will be set to true.  It should NOT
			// be found because we just deleted it, so match_found SHOULD be FALSE
			// when we get done with this.
			t_tlvListItem* item = getStatusRspTlvs->head;
			for(; item != NULL; item = item->next)
			{
				t_TLV *tlvApkId = tlvList_find(tlv_getNodes(item->data), kTAG_SP_AID);

				UINT8* apkId;
				UINT16 apkIdLen;

				tlv_getValue(tlvApkId, &apkId, &apkIdLen);
				ALOGD("%s -- ", __func__);
			    utils_LogByteArrayToHex("  SeApkId: ", apkId, apkIdLen);
			    utils_LogByteArrayToHex("  regApkId: ", sha, 20);
			    if( 0 == memcmp(apkId, sha, 20) )
			    {
			    	break;
			    }
			}

			tlvList_delete(getStatusRspTlvs);
			getStatusRspTlvs = NULL;
		}
	}

    //-----
    // cleanup
MOP_DeleteVirtualCard_BAIL:
	closeLogicalChannel();
	*pRespLen = utils_createStatusCode(pResp, status);
    ALOGD("%s: StatusWord: %04X", __func__, status);
    ALOGD(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
    return status;
}

/**
 *
 *  Retrive the activation state of a Virtual Card
 *
 *@param    vcEntry		 [in] entry number of the VC to activate/deactivate (returned by CreateVirtualCard)
 *@param    mode		 [in] whether the VC should be activated (TRUE) or deactivated (FALSE)
 *
 *@return   Status Word - 9000 for success, other for error
 *
 */
UINT16 MOP_GetVirtualCardStatus(UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen)
{
	UINT16 status;
	UINT16 rStatus;
	UINT16 netVc;
	UINT16 apduLen;
	UINT16 respLen;
	UINT8* pBuf;
	UINT8  sha[20];

	t_tlvList* getStatusRspTlvs = NULL;
	t_tlvList* getStatusRspTlvs2;
	t_TLV *tlvResult;
    UINT8 tmpArray[] = {0x5C, 0x02, 0x9F, 0x70};

    ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);

    if(gCurrentPackage == NULL)
    {
		ALOGI("%s: Package Name not specified", __func__);
		status = eERROR_NO_PACKAGE;
    	goto MOP_GetVirtualCardStatus_BAIL;
    }
    ALOGD("%s: vcEntry = %d, package = %s", __func__, vcEntry, gCurrentPackage);

    gStatusCode = 0;
    utils_CreateSha(sha, (UINT8 *) gCurrentPackage, strlen(gCurrentPackage));

    //-----
    // Selecting the LTSM Application
    rStatus = ltsmStart();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: LTSMStart Failed", __func__);
		status = rStatus;
        goto MOP_GetVirtualCardStatus_BAIL;
    }


    //-----
    // If VC detail isn't available, then status will not succeed
    netVc = utils_htons(vcEntry);
    apduLen = vcDescriptionFile_CreateTLV(kTAG_VC_ENTRY, (UINT8 *) &netVc, 2, gApduBuf, sizeof(gApduBuf));
    respLen = sizeof(gTmpRespBuf);
	getStatus(gApduBuf, apduLen, gTmpRespBuf, &respLen);
	if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
	{
		status = eSW_REFERENCE_DATA_NOT_FOUND;
        goto MOP_GetVirtualCardStatus_BAIL;
	}

	getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
	getStatusRspTlvs2 = tlv_getNodes(tlvList_get(getStatusRspTlvs, 0));

	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_SP_AID);
	if(tlvResult != NULL)
	{
		tlv_getValue(tlvResult, &pBuf, &respLen);
		ALOGD("%s -- ", __func__);
	    utils_LogByteArrayToHex("  spAid: ", pBuf, respLen);
	    utils_LogByteArrayToHex("  appid: ", sha, 20);
	    if( 0 != memcmp(pBuf, sha, 20) )
	    {
	        status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
	    	goto MOP_GetVirtualCardStatus_BAIL;
	    }
	}
    else
    {
        status = eSW_NO_DATA_RECEIVED;
        goto MOP_GetVirtualCardStatus_BAIL;
    }
	tlvList_delete(getStatusRspTlvs);
	getStatusRspTlvs = NULL;

    //Step6:Select CRS.If it is not selected return the CRS Select status failed
	rStatus = crsSelect();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: crsSelect FAILED", __func__);
    	status = eSW_CRS_SELECT_FAILED;
    	goto MOP_GetVirtualCardStatus_BAIL;
    }

    //Step7:LTSM Client sends a GET STATUS to retrieve the state of VC Manager
    // -- TLV 4F (vcmAid)
    // -- Append VCM AID with the vcEntry number for which Status has to be obtained
    memcpy(gApduBuf, data_VC_MANAGER_AID, sizeof(data_VC_MANAGER_AID));
    respLen = sizeof(data_VC_MANAGER_AID);
    utils_append(gApduBuf, &respLen, sizeof(gApduBuf), (UINT8 *) &netVc, 0, sizeof(netVc));
    respLen = vcDescriptionFile_CreateTLV(0x4F, gApduBuf, respLen, gTmpRespBuf, sizeof(gTmpRespBuf));

    // -- TLV 5C (9F70)
    utils_append(gTmpRespBuf, &respLen, sizeof(gTmpRespBuf), tmpArray, 0, sizeof(tmpArray));

    // -- build the CAPDU from this
	apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, 0x00, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));

	// -- issue the actual get status command
	respLen = sizeof(gTmpRespBuf);
	exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
	if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
	{
		status = utils_GetSW(gTmpRespBuf, respLen);
    	goto MOP_GetVirtualCardStatus_BAIL;
	}

	// -- process response of GET STATUS for VCM
	getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
	getStatusRspTlvs2 = tlv_getNodes(tlvList_get(getStatusRspTlvs, 0));

	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_VC_STATE);
	if(tlvResult != NULL)
	{
		tlv_getValue(tlvResult, &pBuf, &respLen);
		status = eSW_NO_ERROR;
	}
	else
	{
		status = eSW_REFERENCE_DATA_NOT_FOUND;
	}

	if(status == eSW_NO_ERROR)
	{
	    UINT32 taglength = *pRespLen;
	    UINT8 respBuff[] = {0x90, 0x00};
		*pRespLen = vcDescriptionFile_CreateTLV(kTAG_VC_STATE, &pBuf[1], 1, pResp, *pRespLen);
	    taglength = vcDescriptionFile_CreateTLV(0x4E, respBuff, 2, pResp+(*pRespLen), taglength);
	    *pRespLen +=  taglength;
	    utils_LogByteArrayToHex("  getVirtualCardStatus RDATA: ", pResp, *pRespLen);
	}

    //-----
    // cleanup
MOP_GetVirtualCardStatus_BAIL:
	tlvList_delete(getStatusRspTlvs);
	closeLogicalChannel();
    if(eSW_NO_ERROR != status)
    {
        *pRespLen = utils_createStatusCode(pResp, status);
    }
    ALOGD("%s: StatusWord: %04X", __func__, status);
    ALOGD(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
    return status;
}


/**
 *
 *  Activate or Deactivate the specified Virtual Card
 *
 *@param    vcEntry		 [in] entry number of the VC to activate/deactivate (returned by CreateVirtualCard)
 *@param    mode		 [in] whether the VC should be activated (TRUE) or deactivated (FALSE)
 *@param    pResp        [out] pointer to a buffer to receive the response data
 *@param    pRespLen     [in/out] pointer to a UINT16.  On input, this is the max size of the pResp buffer
 *                                on output, this is the size of the data written into the pResp buffer
 *
 *@return   Status Word - 9000 for success, other for error
 *
 */
UINT16 MOP_ActivateVirtualCard(UINT16 vcEntry, UINT8 mode, UINT8 concurrentActivationMode, UINT8 *pResp, UINT16 *pRespLen)
{
	UINT16  status;
	UINT16  rStatus;
    UINT16	netVc;
    UINT16	respLen;
    UINT8	sha[20];
    UINT8	*pBuf;
    UINT8	smAid[16];
    UINT8	vcmAid[16];
    UINT16	smAidLen;
    UINT16	vcmAidLen;

	t_tlvList *getStatusRspTlvs = NULL;
	t_tlvList *getStatusRspTlvs2;
	t_TLV *tlvResult;

    ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);
    ALOGD("%s: vcEntry = %d, mode = %d, CAM = %d", __func__, vcEntry, mode, concurrentActivationMode);

    if(gCurrentPackage == NULL)
    {
		ALOGI("%s: Package Name not specified", __func__);
		status = eERROR_NO_PACKAGE;
    	goto MOP_ActivateVirtualCard_BAIL;
    }

    //-----
    // Initialization
    status = eSW_REFERENCE_DATA_NOT_FOUND;
    utils_CreateSha(sha, (UINT8 *)gCurrentPackage, strlen(gCurrentPackage));

    //-----
    // Selecting the LTSM Application
    rStatus = ltsmStart();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: LTSMStart Failed", __func__);
        status = rStatus;
        goto MOP_ActivateVirtualCard_BAIL;
    }

    //-----
    // If VC detail isn't available, then status will not succeeed
    UINT16 apduLen;

    netVc = utils_htons(vcEntry);
    apduLen = vcDescriptionFile_CreateTLV(kTAG_VC_ENTRY, (UINT8*) &netVc, 2, gApduBuf, sizeof(gApduBuf));
    respLen = sizeof(gTmpRespBuf);
	getStatus(gApduBuf, apduLen, gTmpRespBuf, &respLen);
	if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
	{
		status = eCONDITION_OF_USE_NOT_SATISFIED;
        goto MOP_ActivateVirtualCard_BAIL;
	}

	getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
	getStatusRspTlvs2 = tlv_getNodes(tlvList_get(getStatusRspTlvs, 0));

	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_SP_AID);
	if(tlvResult != NULL)
	{
		tlv_getValue(tlvResult, &pBuf, &respLen);
		ALOGD("%s -- ", __func__);
	    utils_LogByteArrayToHex("  spAid: ", pBuf, respLen);
	    utils_LogByteArrayToHex("  appid: ", sha, 20);
	    if( 0 != memcmp(pBuf, sha, 20) )
	    {
	        status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
	        goto MOP_ActivateVirtualCard_BAIL;
	    }
	}
    else
    {
        status = eSW_UNEXPECTED_BEHAVIOR;
        goto MOP_ActivateVirtualCard_BAIL;
    }

	//-----
	// Check VC state
	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_VC_STATE);
	if(tlvResult != NULL)
	{
		tlv_getValue(tlvResult, &pBuf, &respLen);

        /* possible values for vcState,
         *  x x x x 0 0 1 0 - VC_CREATED
         *  x x x x 0 0 0 1 - VC_IN_PROGRESS
         *  x x x x 0 1 1 0 - VC_DEAD
         * */

		if( (pBuf[0] & 0x02) != 0x02 )
		{
			status = eSW_INVALID_VC_ENTRY;
	    	goto MOP_ActivateVirtualCard_BAIL;
		}
		else
		{
			tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_SM_AID);
            if (tlvResult != NULL)
            {
                tlv_getValue(tlvResult, &pBuf, &respLen);
                memcpy(smAid, pBuf, respLen);
                smAidLen = respLen;
            }else
            {
    		    smAidLen = 0;
            	utils_append(smAid, &smAidLen, sizeof(smAid), data_SERVICE_MANAGER_AID, 0, sizeof(data_SERVICE_MANAGER_AID));
            	utils_append(smAid, &smAidLen, sizeof(smAid), (UINT8*) &netVc, 0, 2);
            }

		    vcmAidLen = 0;
        	utils_append(vcmAid, &vcmAidLen, sizeof(vcmAid), data_VC_MANAGER_AID, 0, sizeof(data_VC_MANAGER_AID));
        	utils_append(vcmAid, &vcmAidLen, sizeof(vcmAid), (UINT8*) &netVc, 0, 2);
		}
	}
	else
	{
		status = eSW_UNEXPECTED_BEHAVIOR;
        goto MOP_ActivateVirtualCard_BAIL;
	}
	tlvList_delete(getStatusRspTlvs);
	getStatusRspTlvs = NULL;


    //-----
    // Select the CRS prior to activating
    rStatus = crsSelect();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: crsSelect FAILED", __func__);
    	status = eSW_CRS_SELECT_FAILED;
    	goto MOP_ActivateVirtualCard_BAIL;
    }

    //-----
    // process mode = ACTIVATTION
    if(mode == TRUE)
    {
    	// Checking to determine if VCM can be activated
    	// 4.a Issue GET STATUS (for VCM)

    	// -- TLV 4F (vcmAid)
    	respLen = vcDescriptionFile_CreateTLV(0x4F, vcmAid, vcmAidLen, gTmpRespBuf, sizeof(gTmpRespBuf));

        // -- TLV 5C (9F70)
        UINT8 tmpArray[] = {0x5C, 0x02, 0x9F, 0x70};
        utils_append(gTmpRespBuf, &respLen, sizeof(gTmpRespBuf), tmpArray, 0, sizeof(tmpArray));

        // -- build the CAPDU from this
    	apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, 0x00, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));

    	// -- issue the actual get status command
    	respLen = sizeof(gTmpRespBuf);
    	exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
    	if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
    	{
    		status = eSW_REFERENCE_DATA_NOT_FOUND;
	    	goto MOP_ActivateVirtualCard_BAIL;
    	}

    	// -- process response of GET STATUS for VCM
    	getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
    	getStatusRspTlvs2 = tlv_getNodes(tlvList_get(getStatusRspTlvs, 0));

    	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_VC_STATE);

    	UINT16 vcStateLen;
    	if(tlvResult != NULL)
    	{
    		tlv_getValue(tlvResult, &pBuf, &vcStateLen);
    		if(vcStateLen == 0x02)
    		{
                if(((pBuf[0] & 0x80)==0x80)          //check if VCM_LOCKED
                        || ((pBuf[1] & 0x80)==0x80))  // Check if VCM_NON_ACTIVABLE
                {
                    status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
        	    	goto MOP_ActivateVirtualCard_BAIL;
                }

                if(((pBuf[0] & 0x80)!=0x80)
                        && (pBuf[1] == 0x01)) //Check if Already Activated
                {
                    ALOGI("%s: Already Activated", __func__);
                    status = eSW_NO_ERROR;
        	    	goto MOP_ActivateVirtualCard_BAIL;
                }

    		}
    		else
    		{
    			status = eCONDITION_OF_USE_NOT_SATISFIED;
    	    	goto MOP_ActivateVirtualCard_BAIL;
    		}
    	}
    	else
    	{
    		status = eSW_UNEXPECTED_BEHAVIOR;
	    	goto MOP_ActivateVirtualCard_BAIL;
    	}
        tlvList_delete(getStatusRspTlvs);
        getStatusRspTlvs = NULL;

    	// Check if SM can be activated
    	// -- TLV 4F (smAid)
    	respLen = vcDescriptionFile_CreateTLV(0x4F, smAid, smAidLen, gTmpRespBuf, sizeof(gTmpRespBuf));

        // -- TLV 5C (9F70)
        utils_append(gTmpRespBuf, &respLen, sizeof(gTmpRespBuf), tmpArray, 0, sizeof(tmpArray));

        // -- build the CAPDU from this
    	apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, 0x00, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));

    	// -- issue the actual get status command
    	respLen = sizeof(gTmpRespBuf);
    	exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
    	if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
    	{
    		status = eSW_REFERENCE_DATA_NOT_FOUND;
	    	goto MOP_ActivateVirtualCard_BAIL;
    	}

    	// -- process response of GET STATUS for SM
    	getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
    	getStatusRspTlvs2 = tlv_getNodes(tlvList_get(getStatusRspTlvs, 0));

        tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_VC_STATE);

    	if(tlvResult != NULL)
    	{
    		tlv_getValue(tlvResult, &pBuf, &vcStateLen);
    		if(vcStateLen == 0x02)
    		{
                if(((pBuf[0] & 0x80)==0x80)          //check if VCM_LOCKED
                        || ((pBuf[1] & 0x80)==0x80))  // Check if VCM_NON_ACTIVABLE
                {
                    status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
        	    	goto MOP_ActivateVirtualCard_BAIL;
                }
    		}
    	}
    	else
    	{
    		status = eSW_UNEXPECTED_BEHAVIOR;
	    	goto MOP_ActivateVirtualCard_BAIL;
    	}
    	tlvList_delete(getStatusRspTlvs);
    	getStatusRspTlvs = NULL;

        if(concurrentActivationMode == TRUE)
        {
            status = manageConcurrencyActivation(vcEntry);
        }
        else
        {
            status = manageNonConcurrencyActivation(vcmAid, vcmAidLen);
        }

    }
    //-----
    // process mode = DEACTIVATION
    else
    {
    	// -- TLV 4F (vcmAid)
    	apduLen = vcDescriptionFile_CreateTLV(0x4F, vcmAid, vcmAidLen, gApduBuf, sizeof(gApduBuf));

        setStatus(gApduBuf, apduLen, FALSE, gTmpRespBuf, sizeof(gTmpRespBuf), &respLen);
        status =  utils_GetSW(gTmpRespBuf, respLen);
    }

MOP_ActivateVirtualCard_BAIL:
    //-----
    // cleanup
	tlvList_delete(getStatusRspTlvs);
	closeLogicalChannel();
	*pRespLen = utils_createStatusCode(pResp, status);
    ALOGD("%s: StatusWord: %04X", __func__, status);
    ALOGD(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
    return status;
}

/**
 *
 *  Add and updates MDAC used to define the MIFARE data that
 *  may be retrieved by the SP application
 *
 *@param    vcEntry		 [in] entry number of the VC to add/update
 *@param    vcData		 [in] buffer containing the data to use
 *@param    vcDataLen	 [in] length of the data in vcData buffer
 *@param    pResp        [out] pointer to a buffer to receive the response data
 *@param    pRespLen     [in/out] pointer to a UINT16.  On input, this is the max size of the pResp buffer
 *                                on output, this is the size of the data written into the pResp buffer
 *
 *@return   Status Word - 9000 for success, other for error
 *
 */
UINT16 MOP_AddAndUpdateMdac(UINT16 vcEntry, const UINT8* vcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 status;
    UINT16 rStatus;
    UINT16 respLen;
    UINT16 apduLen;
    UINT8 sha[20];

    ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);
    ALOGD("%s: vcEntry = %d", __func__, vcEntry);
    utils_LogByteArrayToHex("AddAndUpdateMdac Input Data: vcData", vcData, vcDataLen);

    if(gCurrentPackage == NULL)
    {
		ALOGI("%s: Package Name not specified", __func__);
		status = eERROR_NO_PACKAGE;
        goto MOP_AddAndUpdateMdac_BAIL;
    }

    if(vcData == NULL || pResp == NULL || *pRespLen < 4)
    {
        status = eERROR_INVALID_PARAM;
        goto MOP_AddAndUpdateMdac_BAIL;
    }

    //-----
    // Initialize state variables
    gStatusCode = 0;
    utils_CreateSha(sha, (UINT8 *)gCurrentPackage, strlen(gCurrentPackage));

    //-----
    // open communication with LTSM applet
    rStatus = ltsmStart();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: LTSMStart Failed", __func__);
		status = rStatus;
        goto MOP_AddAndUpdateMdac_BAIL;
    }

    rStatus = vcDescriptionFile_AddandUpdateMDAC(vcEntry, vcData, vcDataLen, sha, gApduBuf, sizeof(gApduBuf), &apduLen);
	if(rStatus != STATUS_SUCCESS)
	{
		status = eERROR_GENERAL_FAILURE;
        goto MOP_AddAndUpdateMdac_BAIL;
	}

    //-----
    // perform the deletion process
	respLen = sizeof(gTmpRespBuf);
	exchangeLtsmProcessCommand(gApduBuf, apduLen, gTmpRespBuf, &respLen);
	status = utils_GetSW(gTmpRespBuf, respLen);

    //-----
    // cleanup
MOP_AddAndUpdateMdac_BAIL:
	closeLogicalChannel();
    //-----
    // set the return code
    *pRespLen = utils_createStatusCode(pResp, status);
    ALOGD("%s: StatusWord: %04X", __func__, status);
    ALOGD(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
    return status;
}

/**
 *
 *  Read Mifare Data is used to retrieve MIFARE data from a VC
 *  under the condition that the MIFARE Data Access Control(s)
 *  have been provided with Add and update MDAC
 *
 *@param    vcEntry		 [in] entry number of the VC to read from
 *@param    vcData		 [in] buffer containing the tags that indicate
 *                            what data is to be retrieved
 *@param    vcDataLen	 [in] length of the data in vcData buffer
 *@param    pResp		 [out] pointer to a buffer to receive the response data
 *@param    pRespLen	 [in/out] pointer to a UINT16.  On input, this is the max size of the pResp buffer
 *                                on output, this is the size of the data written into the pResp buffer
 *
 *@return   Status Word - 9000 for success, other for error
 *
 */
UINT16 MOP_ReadMifareData(UINT16 vcEntry, const UINT8* vcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen)
{
    UINT16 status;
    UINT16 rStatus;
    UINT16 respLen;
    UINT8 sha[20];
    UINT8	tmpBuf[32];
    UINT16	tmpBufLen;
    UINT16  netVc;
    UINT16  apduLen;
    UINT16  respBufSize;
	UINT8 	*pBuf;
	UINT16 	pBufLen;
    UINT8  run;

	UNUSED(respBufSize)

	t_tlvList *getStatusRspTlvs = NULL;
	t_tlvList *getStatusRspTlvs2;
	t_TLV *tlvResult;

    ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);
    ALOGD("%s: vcEntry = %d", __func__, vcEntry);

    respBufSize = *pRespLen;
    *pRespLen = 0;


    if(gCurrentPackage == NULL)
    {
		ALOGI("%s: Package Name not specified", __func__);
		status = eERROR_NO_PACKAGE;
		goto MOP_ReadMifareData_BAIL;
    }

    //-----
    // Initialize globals
    gStatusCode = 0;
    utils_CreateSha(sha, (UINT8 *)gCurrentPackage, strlen(gCurrentPackage));

    //-----
    // Selecting the LTSM Application
    rStatus = ltsmStart();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: LTSMStart Failed", __func__);
		status = rStatus;
        goto MOP_ReadMifareData_BAIL;
    }

    //-----
    // GetStatus of the VC and check against calling app
    netVc = utils_htons(vcEntry);
    apduLen = vcDescriptionFile_CreateTLV(kTAG_VC_ENTRY, (UINT8*) &netVc, 2, gApduBuf, sizeof(gApduBuf));
    respLen = sizeof(gTmpRespBuf);
	getStatus(gApduBuf, apduLen, gTmpRespBuf, &respLen);
	if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
	{
		status = utils_GetSW(gTmpRespBuf, respLen);
        goto MOP_ReadMifareData_BAIL;
	}

	getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2);
	getStatusRspTlvs2 = tlv_getNodes(tlvList_get(getStatusRspTlvs, 0));

	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_SP_AID);
	if(tlvResult != NULL)
	{
		tlv_getValue(tlvResult, &pBuf, &pBufLen);
		ALOGD("%s -- ", __func__);
	    utils_LogByteArrayToHex("  spAid: ", pBuf, pBufLen);
	    utils_LogByteArrayToHex("  appid: ", sha, 20);
	    if( 0 != memcmp(pBuf, sha, 20) )
	    {
	        status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
	        goto MOP_ReadMifareData_BAIL;
	    }
	}
    else
    {
        status = eSW_UNEXPECTED_BEHAVIOR;
        goto MOP_ReadMifareData_BAIL;
    }

    //-----
    // Select the VC
	// --
	// code in the original LTSM Comunicator repeats the getStatus - I don't think it is needed (RP)
	tlvResult = tlvList_find(getStatusRspTlvs2, kTAG_SM_AID);
	if(tlvResult != NULL)
	{
		tlv_getValue(tlvResult, &pBuf, &pBufLen);
		memcpy(tmpBuf, pBuf, pBufLen);
		tmpBufLen = pBufLen;
	}
	else
	{
	    tmpBufLen = 0;
	    utils_append(tmpBuf, &tmpBufLen, sizeof(tmpBuf), data_SERVICE_MANAGER_AID, 0, sizeof(data_SERVICE_MANAGER_AID));
	    netVc = utils_htons(vcEntry);
	    utils_append(tmpBuf, &tmpBufLen, sizeof(tmpBuf), (UINT8*) &netVc, 0, 2);
	}
	tlvList_delete(getStatusRspTlvs);
	getStatusRspTlvs = NULL;

	apduLen = utils_MakeCAPDU(0x00, 0xA4, 0x04, 0x00, tmpBuf, tmpBufLen, gApduBuf, sizeof(gApduBuf));

	respLen = sizeof(gTmpRespBuf);
	rStatus = exchange(gApduBuf, apduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), gTmpRespBuf, &respLen);
	if(STATUS_SUCCESS != rStatus)
	{
    	ALOGD("%s: SE transceive failed", __func__);
    	status = eSW_NO_DATA_RECEIVED;
    	goto MOP_ReadMifareData_BAIL;
	}

	ALOGD("%s -- ", __func__);
    utils_LogByteArrayToHex("  Received Data : ", gTmpRespBuf, respLen);

    //-----
    // Send the read command
	apduLen = utils_MakeCAPDU(0x80, 0xB0, 0x00, 0x00, vcData, vcDataLen, gApduBuf, sizeof(gApduBuf));
	respLen = sizeof(gTmpRespBuf);
	rStatus = exchange(gApduBuf, apduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), gTmpRespBuf, &respLen);

    run = TRUE;
    while(run && (STATUS_SUCCESS == rStatus))
    {
        switch(utils_GetSW(gTmpRespBuf, respLen))
        {
            case (UINT16)(0x6310):
                utils_append(pResp, pRespLen, respBufSize, gTmpRespBuf, 0, respLen-2);
                break;

            case (UINT16)(0x9000):
                utils_append(pResp, pRespLen, respBufSize, gTmpRespBuf, 0, respLen);
                run = FALSE;
                break;

            case (UINT16)(0x6A88):
                utils_append(pResp, pRespLen, respBufSize, gTmpRespBuf, 0, respLen);
                run = FALSE;
                break;

            default:
                utils_append(pResp, pRespLen, respBufSize, gTmpRespBuf, 0, respLen);
                run = FALSE;
                break;
        }

        if(run)
        {
            apduLen = utils_MakeCAPDU(0x80, 0xB0, 0x00, 0x01, NULL, 0, gApduBuf, sizeof(gApduBuf));
            respLen = sizeof(gTmpRespBuf);
            rStatus = exchange(gApduBuf, apduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), gTmpRespBuf, &respLen);
        }
    }

	status = utils_GetSW(pResp, *pRespLen);

MOP_ReadMifareData_BAIL:
	tlvList_delete(getStatusRspTlvs);
    closeLogicalChannel();
	if(eSW_NO_ERROR == status)
	{
	    unsigned char pRespSW[4];
	    unsigned short respSWLen = 4;

        //-----
        // set the return code
        respSWLen = utils_createStatusCode(pRespSW, status);

        //-----
        // append the return code to the MIFARE data
        *pRespLen -= 2; //Exclude the SW1SW2 as 4E tag will be appended
        utils_append(pResp, pRespLen, 1024, pRespSW, 0, respSWLen);
    }
    else
    {
        *pRespLen = utils_createStatusCode(pResp, status);
    }
    ALOGD("%s: StatusWord: %04X", __func__, status);
    ALOGD(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
    return status;
}

/**
 *
 *  getVcList will return the list of VC entries in use
 *  and the associated SP application
 *
 *@param    pVcList		 [out] pointer to a buffer to receive the response data
 *@param    pVcListLen	 [in/out] pointer to a UINT16.  On input, this is the max size of the pVcList buffer
 *                                on output, this is the size of the data written into the pVcList buffer
 *
 *@return   Status Word - 9000 for success, other for error
 *
 */
UINT16 MOP_GetVcList(UINT8 *pVcList, UINT16 *pVcListLen)
{
	UINT16 status;
	UINT16 rStatus;
	UINT16 respLen;
	UINT16 listLen;
    t_tlvList *getStatusRspTlvs = NULL;

	ALOGD(">>>>>>>>> %s: Enter <<<<<<<<<<", __func__);

	gStatusCode = 0;
	listLen = 0;

    //-----
    // open communication with LTSM applet
    rStatus = ltsmStart();
    if( rStatus != eSW_NO_ERROR )
    {
    	ALOGD("%s: LTSMStart Failed", __func__);
    	status = rStatus;
        goto MOP_GetVcList_BAIL;
    }


    respLen = sizeof(gTmpRespBuf);
    getStatus(NULL, 0, gTmpRespBuf, &respLen);
	rStatus = utils_GetSW(gTmpRespBuf, respLen);
	if( rStatus == eSW_NO_ERROR)
	{
		getStatusRspTlvs = tlv_parse(gTmpRespBuf, respLen-2); // -2 is to remove the StatusWord
		if(getStatusRspTlvs != NULL)
		{
			listLen = vcDescriptionFile_GetVcListResp(getStatusRspTlvs, pVcList, *pVcListLen);
		}

		if(listLen != 0)
		{
			status = rStatus;
		}
		else
		{
			status = eSW_REFERENCE_DATA_NOT_FOUND;
		}
	}
	else
	{
		status = rStatus;
	}

	tlvList_delete(getStatusRspTlvs);

MOP_GetVcList_BAIL:
	*pVcListLen = listLen;
    closeLogicalChannel();
    if(eSW_NO_ERROR == status)
    {
        unsigned char pRespSW[4];
        unsigned short respSWLen = 4;

        //-----
        // set the return code
        respSWLen = utils_createStatusCode(pRespSW, status);

        //-----
        // append the return code to the MIFARE data
        utils_append(pVcList, pVcListLen, 1024, pRespSW, 0, respSWLen);
    }
    else
    {
        *pVcListLen = utils_createStatusCode(pVcList, status);
    }
    ALOGD("%s: StatusWord: %04X", __func__, status);
    ALOGD(">>>>>>>>> %s: Exit <<<<<<<<<<", __func__);
    return status;
}

//----------------------------------------------------------------------------------
//	Private Support Functions
//----------------------------------------------------------------------------------


/*******************************************************
 *Selects CRS during Activate VC
 ********************************************************/

UINT16 crsSelect(void)
{
	UINT16 respLen;
	tJBL_STATUS res;
	UINT8 apdu[32];
	UINT8 resp[64];

    ALOGD("%s: Enter", __func__);
	respLen = sizeof(resp);
	memcpy(apdu, data_SELECT_CRS, sizeof(data_SELECT_CRS));
	res = exchangeWithSe(apdu, sizeof(data_SELECT_CRS), resp, &respLen);
    if(STATUS_SUCCESS != res || respLen == 0)
    {
    	ALOGD("%s: SE transceive failed", __func__);
    	return eSW_NO_DATA_RECEIVED;
    }

	ALOGD("%s: --",__func__);
    utils_LogByteArrayToHex("rData", resp, respLen);
	ALOGD("%s: Exit", __func__);
    return utils_GetSW(resp, respLen);
}



/*******************************************************
 * \brief Initiate the LTSM. Opens the Logical Channel and Selects the LTSM
 *
 * Side effects: modifies gStatusCode (openLogicalChannel)
 *               modifies gRegistry (openLogicalChannel)
 *
 * \return: !eSW_NO_ERROR
 *         eSW_NO_ERROR
 ********************************************************/

UINT16 ltsmStart(void)
{
	UINT16 status;

    ALOGI("%s: Enter", __func__);

    status = openLogicalChannel();
	if(status != eSW_NO_ERROR){
		ALOGI("%s LTSM Open Channel Failed", __func__);
		goto ltsmStart_exit;
	}

	status = ltsmSelect();
	if(status != eSW_NO_ERROR){
		ALOGI("%s LTSM select Failed", __func__);
		closeLogicalChannel();
		goto ltsmStart_exit;
	}

	status = eSW_NO_ERROR;

ltsmStart_exit:
	ALOGI("%s Exit", __func__);
	return status;
}

/*******************************************************
 * \brief Opens Logical Channel
 *
 * Side effects: modifies gStatusCode
 *               modifies gRegistry (channelMgr)
 *
 * \return: NOT eSW_NO_ERROR - gStatusCode will contain same StatusWord
 *          eSW_NO_ERROR - gStatusCode will be 0
 ********************************************************/

UINT16 openLogicalChannel(void)
{
    UINT8 recvData[255];
    UINT16 recvDataLen;
    UINT16 status;
    UINT16 rStatus;

    ALOGI("%s: Enter", __func__);

 	gStatusCode = 0;

 	rStatus = openConnection();
	if(STATUS_FAILED == rStatus)
	{
		closeLogicalChannel();
		status = eSW_SE_TRANSCEIVE_FAILED;
		goto openLogicalChannel_BAIL;
	}

	utils_LogByteArrayToHex("openChannelAPDU:", data_openChannelAPDU, sizeof(data_openChannelAPDU));
	recvDataLen = sizeof(recvData);
	rStatus = exchangeApdu(data_openChannelAPDU, sizeof(data_openChannelAPDU), recvData, &recvDataLen);
	if(STATUS_FAILED == rStatus)
	{
		closeLogicalChannel();
		status = eSW_SE_TRANSCEIVE_FAILED;
		goto openLogicalChannel_BAIL;
	}

	utils_LogByteArrayToHex("openChannelAPDU Response:", recvData, recvDataLen);
	if(recvDataLen == 0)
	{
		ALOGI("%s: SE transceive failed ", __func__);
		status = eSW_SE_TRANSCEIVE_FAILED;
		goto openLogicalChannel_BAIL;
	}
	else if(utils_GetSW(recvData, recvDataLen) != eSW_NO_ERROR)
	{
		gStatusCode = utils_GetSW(recvData, recvDataLen);
		ALOGI("%s: Invalid Response", __func__);
		status = gStatusCode;
		goto openLogicalChannel_BAIL;
	}
	else
	{
		ALOGI("%s: openLogicalChannel SUCCESS", __func__);
		if(STATUS_SUCCESS == channelMgr_addOpenedChannelId(registry_GetChannelMgr(gRegistry), recvData[recvDataLen - 3]))
		{
			ALOGI("%s: addChannelId SUCCESS", __func__);
			status = eSW_NO_ERROR;
		}
		else
		{
			ALOGD("%s: addChannelId FAILED, chMgr = %X", __func__, (unsigned int) registry_GetChannelMgr(gRegistry));
			closeLogicalChannel();
			status = eSW_OPEN_LOGICAL_CHANNEL_FAILED;
		}
	}

openLogicalChannel_BAIL:
	ALOGI("%s: Exit", __func__);
	return status;
}

/*******************************************************
 *Selects LTSM with currently opened Logical channel
 ********************************************************/
/*
 * \brief Selects the LTSM applet on the eSE using the currently
 *        opened logical channel.  If successful,
 *        gets the count of installed VCs and maximum VCs
 *        Makes sure there is REGISTRY object to store the VCs
 *        Saves this object in a backing store (file/buffer/etc)
 *
 * Side effects: modifies gRegistry
 *
 * \return STATUS_SUCCESS if successful
 * 		   STATUS_FAILED AND sets gStatusCode with the SW of the SELECT
 */
UINT16 ltsmSelect(void) {
    UINT16 status;
    UINT16 rStatus;
    UINT8  apduBuf[24];
    UINT16 apduLen;
    UINT8  respBuf[255];
    UINT16 respLen;

    ALOGI("%s: Enter", __func__);

    gStatusCode = 0;

    apduLen = 0;
    utils_append(apduBuf, &apduLen, sizeof(apduBuf), data_selectAPDU, 0, sizeof(data_selectAPDU));
    apduBuf[apduLen++] = sizeof(data_AID_M4M_LTSM);
    utils_append(apduBuf, &apduLen, sizeof(apduBuf), data_AID_M4M_LTSM, 0, sizeof(data_AID_M4M_LTSM));

    respLen = sizeof(respBuf);
    rStatus = exchange(apduBuf, apduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), respBuf, &respLen);
    if(STATUS_SUCCESS != rStatus)
    {
    	gStatusCode = eSW_SE_TRANSCEIVE_FAILED;
    	status = gStatusCode;
    	goto ltsmSelect_BAIL;
    }
    else
    {
        ALOGD("%s -- ", __func__);
        utils_LogByteArrayToHex(" ReceiveData:", respBuf, respLen);

        if(utils_GetSW(respBuf, respLen) == eSW_NO_ERROR)
        {
            ALOGD("%s ltsmSelect SUCCESS", __func__);
            status = eSW_NO_ERROR;
        }else{
            gStatusCode = utils_GetSW(respBuf, respLen);
            status = gStatusCode;
        }
    }

ltsmSelect_BAIL:
    ALOGI("%s: Exit",__func__);
    return status;
}

/*******************************************************
 * \brief Closes currently opened Logical Channels
 *
 * side effects: modifies gRegistry
 ********************************************************/
void closeLogicalChannel(void)
{
    tJBL_STATUS	res;
    UINT8	chCount;
    UINT8	cnt, id;
    UINT8	xx;
    UINT8	send[5];
    UINT16	recvDataLen;
    UINT8   recvData[255];

    ALOGI("%s: Enter", __func__);

    chCount = channelMgr_getChannelCount(registry_GetChannelMgr(gRegistry));

    for(cnt=0; cnt < chCount; cnt++)
    {
    	if(channelMgr_getIsOpened(registry_GetChannelMgr(gRegistry), cnt) == TRUE)
    	{
    		xx = 0;
    		id = channelMgr_getChannelId(registry_GetChannelMgr(gRegistry), cnt);
    		send[xx++] = id;
    		send[xx++] = 0x70;
    		send[xx++] = 0x80;
    		send[xx++] = id;
    		send[xx++] = 0x00;

    		recvDataLen = sizeof(recvData);
    		res = exchangeApdu(send, xx, recvData, &recvDataLen);
    		if(res != STATUS_SUCCESS)
    		{
    			ALOGI("%s: exchangeApdu FAILED (STATUS) entry = %d",__func__, cnt);
    			goto closeLogicalChannel_BAIL;
    		}

    		if(recvDataLen == 0)
    		{
    			ALOGI("%s: exchangeApdu FAILED (return count) entry = %d, %s",__func__, cnt, utils_getStringStatusCode(eSW_NO_DATA_RECEIVED));
    		}
    		else if(utils_GetSW(recvData, recvDataLen) == eSW_NO_ERROR)
    		{
    			ALOGI("%s: close channel id = %d SUCCEEDED", __func__, id);
    			channelMgr_closeChannel(registry_GetChannelMgr(gRegistry), cnt);
    		}
    		else
    		{
    			ALOGI("%s: close channel id = %d FAILED", __func__, id);
    		}
    	}
    }

closeLogicalChannel_BAIL:
    closeConnection();

    ALOGI("%s: EXIT",__func__);
}


/*******************************************************
 * Get the virtual card status from SE
 ********************************************************/
void getStatus(UINT8* data, UINT16 dataLen, UINT8* ltsmStatus, UINT16 *pStatusLen)
{
    UINT16 rStatus;
    UINT8  apduBuf[32];
    UINT16 apduLen;
    UINT8  respBuf[255];
    UINT16 respLen;
    UINT8  run;
    UINT16 maxStatusLen = *pStatusLen;

    ALOGI("%s: Enter", __func__);

    apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, 0x00, data, dataLen, apduBuf, sizeof(apduBuf));
    respLen = sizeof(respBuf);
    rStatus = exchange(apduBuf, apduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), respBuf, &respLen);

    *pStatusLen = 0;
	run = TRUE;
	while(run && (STATUS_SUCCESS == rStatus))
	{
		switch(utils_GetSW(respBuf, respLen))
		{
			case (UINT16)(0x6310):
				utils_append(ltsmStatus, pStatusLen, maxStatusLen, respBuf, 0, respLen-2);
				break;

			case (UINT16)(0x9000):
				utils_append(ltsmStatus, pStatusLen, maxStatusLen, respBuf, 0, respLen);
				run = FALSE;
				break;

			case (UINT16)(0x6A88):
				utils_append(ltsmStatus, pStatusLen, maxStatusLen, respBuf, 0, respLen);
				run = FALSE;
				break;

			default:
				run = FALSE;
				break;
		}

		if(run)
		{
			apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, 0x01, NULL, 0, apduBuf, sizeof(apduBuf));
			respLen = sizeof(respBuf);
			rStatus = exchange(apduBuf, apduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), respBuf, &respLen);
		}
	};

	ALOGD("%s: --", __func__);
    utils_LogByteArrayToHex(" status:", ltsmStatus, *pStatusLen);
    ALOGI("%s: Exit", __func__);
    return;
}

/*******************************************************
 * Set the virtual card status in SE
 ********************************************************/
void setStatus(UINT8* data, UINT16 dataLen, UINT8 mode, UINT8* pResp, UINT16 respBufSize, UINT16 *pRespActSize)
{
	UINT8 apdu[256];
	UINT16 apduLen;

	ALOGI("%s: Enter", __func__);

	if (mode == TRUE) {
		apduLen = utils_MakeCAPDU(0x80, 0xF0, 0x01, 0x01, data, dataLen, apdu, sizeof(apdu));
	}else{
		apduLen = utils_MakeCAPDU(0x80, 0xF0, 0x01, 0x00, data, dataLen, apdu, sizeof(apdu));
	}

	exchangeWithSe(apdu, apduLen, pResp, &respBufSize);
	*pRespActSize = respBufSize;

    ALOGI("%s: Enter", __func__);
}

/*******************************************************
 *
 *Manage concurrency Activation Procedure
 * @param vcEntry
 * @return status
 *
 ********************************************************/
UINT16 manageConcurrencyActivation(UINT16 vcEntry)
{
	UINT16 netVc;
	UINT8 vcmAidToActivate[24];
	UINT16 vcmAidLen;
	UINT16 apduLen;
	UINT16 respLen;
	UINT16 status;
	UINT8  *pVal;
	UINT16 valLen;
	UINT8  skipFirstTag48;
	t_tlvList *parseTlvs;
	t_tlvList *parseTlvs2;
	t_TLV *tlvResult;
	t_TLV *tlvResultA0A2 = NULL;
	t_TLV *tlv4F, *tlv;
    UINT8 tmpArray[] = {0x5C, 0x01, 0xA6};
    UINT16 tagBuf = 0x00F1;

	parseTlvs = NULL;
	skipFirstTag48 = 1;

	memcpy(vcmAidToActivate, data_VC_MANAGER_AID, sizeof(data_VC_MANAGER_AID));
	vcmAidLen = sizeof(data_VC_MANAGER_AID); // length of data currently in buffer
	netVc = utils_htons(vcEntry);
	utils_append(vcmAidToActivate, &vcmAidLen, sizeof(vcmAidToActivate), (UINT8 *) &netVc, 0, 2);

	if(!gPreviousConcurrentActivationMode)
	{
		//Deactivate all
		deactivateAllContactlessApplications();
	}
	gPreviousConcurrentActivationMode = TRUE;

	//-------
	// Try to activate the intended VC
	while(1)
	{
		apduLen = vcDescriptionFile_CreateTLV(0x4F, vcmAidToActivate, vcmAidLen, gApduBuf, sizeof(gApduBuf));
		setStatus(gApduBuf, apduLen, TRUE, gTmpRespBuf, sizeof(gTmpRespBuf), &respLen);

		switch(utils_GetSW(gTmpRespBuf, respLen))
		{
		case eSW_NO_ERROR:
			ALOGD("%s: Activated Successfully", __func__);
			status = eSW_NO_ERROR;
			goto manageConcurrencyActivation_BAIL;
			// break - while(1)

		case eSW_6330_AVAILABLE:
		case eSW_6310_AVAILABLE:
			// Activation not successful, conflicting AIDs present
			tlvResultA0A2 = getTlvA0A2(gTmpRespBuf, respLen);

			switch(tlv_getTag(tlvResultA0A2))
			{
			case kTAG_A0_AVAILABLE:
				tlv_getValue(tlvResultA0A2, &pVal, &valLen);
				setStatus(pVal, valLen, FALSE, gTmpRespBuf,sizeof(gTmpRespBuf), &respLen);
				if(utils_GetSW(gTmpRespBuf, respLen) == eSW_NO_ERROR)
				{
					// deActivation of conflictingAidsTagA0 successful
					// try to activate initial intended aid
					break;
				}
				else
				{
					ALOGD("%s: Deactivation of conflicting AIDs of TagA0 FAILED. Status: %04X",
							__func__, utils_GetSW(gTmpRespBuf, respLen));
					status = eSW_UNEXPECTED_BEHAVIOR;
					goto manageConcurrencyActivation_BAIL;
					// break - while(1)
				}

			case kTAG_A2_AVAILABLE:
				tlv_getValue(tlvResultA0A2, &pVal, &valLen);
				parseTlvs = tlv_parse(pVal, valLen);
				tlvResult = tlvList_find(parseTlvs, kTAG_48_AVAILABLE);

				tlv_getValue(tlvResult, &pVal, &valLen);
				switch( utils_ntohs(*((UINT16 *) pVal)) )
				{
				case eREASON_CODE_800B:
				case eREASON_CODE_800C:
					status = eSW_VC_IN_CONTACTLESS_SESSION;
					goto manageConcurrencyActivation_BAIL;
					// break - while(1)

				case eREASON_CODE_8002:
				case eREASON_CODE_8003:
				case eREASON_CODE_8004:
				case eREASON_CODE_8005:
                    /*Tag format
                     * A2 --
                     *       48 -- Mandatory
                     *       4F -- Mandatory
                     *       4F -- Optional
                     *       ..... .....
                     *       48 -- Optional
                     *       4F -- Optional
                     *       4F -- Optional
                     *       ..... .....
                     *       ..... .....
                     *       Get 4F Tag corresponding to only one Tag48
                     * */
					apduLen = getSubsequentTlvs(skipFirstTag48, kTAG_4F_AVAILABLE, parseTlvs, gApduBuf, sizeof(gApduBuf));
					setStatus(gApduBuf, apduLen, FALSE, gTmpRespBuf, sizeof(gTmpRespBuf), &respLen);
					if( utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR )
					{
						ALOGD("%s: Deactivation of conflicting AIDs of TagA2 FAILED. Status: %04X",
								__func__, utils_GetSW(gTmpRespBuf, respLen));
						status = eSW_UNEXPECTED_BEHAVIOR;
						goto manageConcurrencyActivation_BAIL;
						// break - while(1)
					}
					// deactivation of conflictingAidsTagA2 successful
					// try to activate initial intended AID
					break;

				case eREASON_CODE_8009:
					// Deactivate the VC that contains a Mifare Desfire AID conflicting with
					// Mifare Desfire AID of the VC to be activated.

					tlv4F = tlvList_find(parseTlvs, kTAG_4F_AVAILABLE);
					tlv_getValue(tlv4F, &pVal, &valLen);

					// appending 0x5C, 0xA6 will restrict the return Tag to only A6 (avoids all unnecessary tags)
					//
					// -- TLV 4F (aid)
				    respLen = vcDescriptionFile_CreateTLV(0x4F, pVal, valLen, gTmpRespBuf, sizeof(gTmpRespBuf));

				    // -- TLV 5C (A6)
				    utils_append(gTmpRespBuf, &respLen, sizeof(gTmpRespBuf), tmpArray, 0, sizeof(tmpArray));

				    // -- build the CAPDU from this
					apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, 0x00, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));

					// -- issue the actual get status command
					respLen = sizeof(gTmpRespBuf);
					exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
				    utils_LogByteArrayToHex(" REASON_CODE_8009 response: ", gTmpRespBuf, respLen);

				    tlvList_delete(parseTlvs); parseTlvs = NULL;

				    parseTlvs = tlv_parseWithPrimitiveTags(gTmpRespBuf, respLen-2, &tagBuf, 1);
				    tlv = tlvList_find(parseTlvs, 0x61);
				    parseTlvs2 = tlv_getNodes(tlv);
				    tlv = tlvList_find(parseTlvs2, kTAG_A6_AVAILABLE);
				    if(tlv == NULL)
				    {
				    	ALOGD("%s: Tag A6 missing", __func__);
				    	status = eSW_UNEXPECTED_BEHAVIOR;
						goto manageConcurrencyActivation_BAIL;
						// break - while(1)
				    }

				    parseTlvs2 = tlv_getNodes(tlv);
				    tlv = tlvList_find(parseTlvs2, kTAG_F1_AVAILABLE);
				    if(tlv == NULL)
				    {
				    	ALOGD("%s: Tag F1 missing", __func__);
				    	status = eSW_UNEXPECTED_BEHAVIOR;
						goto manageConcurrencyActivation_BAIL;
						// break - while(1)
				    }

					tlv_getValue(tlv, &pVal, &valLen);
				    apduLen = vcDescriptionFile_CreateTLV(0x4F, pVal, valLen, gApduBuf, sizeof(gApduBuf));
					setStatus(gApduBuf, apduLen, FALSE, gTmpRespBuf, sizeof(gTmpRespBuf), &respLen);
					if(utils_GetSW(gTmpRespBuf, respLen) != eSW_NO_ERROR)
					{
				    	status = eSW_UNEXPECTED_BEHAVIOR;
						goto manageConcurrencyActivation_BAIL;
						// break - while(1)
					}
					break;

				default:
					status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
					goto manageConcurrencyActivation_BAIL;
					// break - while(1)
				}
				tlvList_delete(parseTlvs);
				parseTlvs = NULL;

				break; // TAG_K2_AVAILABLE

			default:
				status = eSW_UNEXPECTED_BEHAVIOR;
				goto manageConcurrencyActivation_BAIL;
				// break - while(1)
			}
			tlv_delete(tlvResultA0A2);
			tlvResultA0A2 = NULL;
			break; // SW_6310_AVAILABLE

		default:
			status = eSW_UNEXPECTED_BEHAVIOR;
			goto manageConcurrencyActivation_BAIL;
			// break - while(1)

		}
	}

manageConcurrencyActivation_BAIL:
    tlv_delete(tlvResultA0A2);//CID10685
    tlvResultA0A2 = NULL;
	tlvList_delete(parseTlvs);
	parseTlvs = NULL;
	ALOGD("%s: EXIT - Status: %04X", __func__, status);
	return status;
}

/*******************************************************
 *
 *Manage Non-concurrency Activation Procedure
 * @param vcEntry
 * @return status
 *
 ********************************************************/
UINT16 manageNonConcurrencyActivation(UINT8* vcmAid, UINT16 vcmAidLen)
{
	UINT16 status;
	UINT16 respLen;
	UINT16 apduLen;

	ALOGD("%s: Enter", __func__);

	status = deactivateAllContactlessApplications();
	if(status == eSW_NO_ERROR)
	{
		gPreviousConcurrentActivationMode = FALSE;
		respLen = vcDescriptionFile_CreateTLV(0x4F, vcmAid, vcmAidLen, gTmpRespBuf, sizeof(gTmpRespBuf));
		apduLen = utils_MakeCAPDU(0x80, 0xF0, 0x01, 0x01, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));
		respLen = sizeof(gTmpRespBuf);
		exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
		status = utils_GetSW(gTmpRespBuf, respLen);
	}
	return status;
}

/*******************************************************
 * Determine if the provided AID is the VC Manager AID
 ********************************************************/
UINT8 isVCManagerAID(UINT8* aid, UINT16 aidLen)
{
	if (aidLen < sizeof(data_NONCONCURRENT_AID_PARTIAL))
	{
	   return FALSE;
	}

	if( 0 != memcmp(aid, data_NONCONCURRENT_AID_PARTIAL, sizeof(data_NONCONCURRENT_AID_PARTIAL) - 1) )
	{
		return FALSE;
	}

	return ( (aid[sizeof(data_NONCONCURRENT_AID_PARTIAL)-1] & 0xF0) == 0x10 );
}

/*******************************************************
 * Determine if the provided AID is DESFire APP AID
 ********************************************************/
UINT8 isDESFireAppAID(UINT8* aid, UINT16 aidLen)
{
	if (aidLen < sizeof(data_DESFIRE_APP_AID))
	{
	   return FALSE;
	}

	if( 0 != memcmp(aid, data_DESFIRE_APP_AID, sizeof(data_DESFIRE_APP_AID) - 1) )
	{
		return FALSE;
	}
	return TRUE;
}

/*******************************************************
 * Determine if the provided AID is the VC Manager AID
 ********************************************************/
UINT16 deactivateAllContactlessApplications(void)
{
	UINT8 more;
	UINT16 respLen;
	UINT16 apduLen;
	UINT8 *pVal;
	UINT16 valLen;
	t_tlvList *tlvs;

	tlvs = NULL;
	// First, deactivate all VCMs; Then deactivate all other applications.

	//---------------
	// deactivate all VCMs
	UINT16 status = 0;
	UINT8 p2 = 0x00;	// first pass in loop

	for(more = TRUE; more; p2 = 0x01)
	{
		// 5. Get the list of activated VCMs
		// -- TLV 4F (aid)
		UINT8 tmpBuf[] = {0xA0, 0x00, 0x00, 0x03, 0x96};
	    respLen = vcDescriptionFile_CreateTLV(0x4F, tmpBuf, sizeof(tmpBuf), gTmpRespBuf, sizeof(gTmpRespBuf));

	    // -- TLV 5C (A6)
	    UINT8 tmpArray[] = {0x9F, 0x70, 0x02, 0x07, 0x01, 0x5C, 0x01, 0x4F};
	    utils_append(gTmpRespBuf, &respLen, sizeof(gTmpRespBuf), tmpArray, 0, sizeof(tmpArray));

	    // -- build the CAPDU from this
		apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, p2, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));

		respLen = sizeof(gTmpRespBuf);
		exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);

		switch(utils_GetSW(gTmpRespBuf, respLen))
		{
		case eSW_6310_AVAILABLE:
			more = TRUE;
			break;

		case eSW_NO_ERROR:
			more = FALSE;
			status = eSW_NO_ERROR;
			break;

		case eSW_REFERENCE_DATA_NOT_FOUND:
			more = FALSE;
			continue;

		default:
			status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
			goto deactivateAllContactlessApplications_BAIL;
		}

	    UINT16 tagBuf = 0x00F1;
	    /*
	     * Fix for CID 10683
	     */
	    if (tlvs != NULL)
	    {
	    	tlvList_delete(tlvs);
	    	tlvs = NULL;
	    }
		tlvs = tlv_parseWithPrimitiveTags(gTmpRespBuf, respLen-2, &tagBuf, 1);

		t_tlvListItem* item = tlvs->head;
		for(;item != NULL; item = item->next)
		{
			if(tlv_getTag(item->data) != 0x61)
			{
				continue;
			}

			t_tlvList *tlv61s = tlv_getNodes(item->data);
			if(tlv61s == NULL)
			{
				status = 0x00;
				goto deactivateAllContactlessApplications_BAIL;
			}

			t_tlvListItem* tlvAidItem = tlv61s->head;
			for(;tlvAidItem != NULL; tlvAidItem = tlvAidItem->next)
			{
				if( tlv_getTag(tlvAidItem->data) == 0x4F )
				{
					tlv_getValue(tlvAidItem->data, &pVal, &valLen);
					if(!isVCManagerAID(pVal, valLen))
					{
						continue;
					}

					// Deactivate VCM (Do not check SW)
				    respLen = vcDescriptionFile_CreateTLV(0x4F, pVal, valLen, gTmpRespBuf, sizeof(gTmpRespBuf));
					apduLen = utils_MakeCAPDU(0x80, 0xF0, 0x01, 0x00, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));
					exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
				}
			}
		}
	}
	tlvList_delete(tlvs); tlvs = NULL;

	//---------------
	// deactivate all other applications
	status = 0;
	p2 = 0x00;	// first pass in loop

	for(more = TRUE; more; p2 = 0x01)
	{
		// 6. List all remaining activated contactless applications
	    UINT8 tmpArray[] = {0x4F, 0x00, 0x9F, 0x70, 0x02, 0x07, 0x01, 0x5C, 0x01, 0x4F};
		apduLen = utils_MakeCAPDU(0x80, 0xF2, 0x40, p2, tmpArray, sizeof(tmpArray), gApduBuf, sizeof(gApduBuf));

		respLen = sizeof(gTmpRespBuf);
		exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);

		switch(utils_GetSW(gTmpRespBuf, respLen))
		{
		case eSW_6310_AVAILABLE:
			more = TRUE;
			break;

		case eSW_NO_ERROR:
			more = FALSE;
			status = eSW_NO_ERROR;
			break;

		case eSW_REFERENCE_DATA_NOT_FOUND:
			more = FALSE;
			continue;

		default:
			status = eSW_CONDITION_OF_USE_NOT_SATISFIED;
			goto deactivateAllContactlessApplications_BAIL;
		}

	    UINT16 tagBuf = 0x00F1;
	    if (tlvs != NULL)
	    {
	        tlvList_delete(tlvs);
	        tlvs = NULL;
	    }
		tlvs = tlv_parseWithPrimitiveTags(gTmpRespBuf, respLen-2, &tagBuf, 1);

		t_tlvListItem* item = tlvs->head;
		for(;item != NULL; item = item->next)
		{
			if(tlv_getTag(item->data) != 0x61)
			{
				continue;
			}

			t_tlvList *tlv61s = tlv_getNodes(item->data);
			if(tlv61s == NULL)
			{
				status = 0x00;
				goto deactivateAllContactlessApplications_BAIL;
			}

			t_tlvListItem* tlvAidItem = tlv61s->head;
			for(;tlvAidItem != NULL; tlvAidItem = tlvAidItem->next)
			{
				if( tlv_getTag(tlvAidItem->data) == 0x4F )
				{
					tlv_getValue(tlvAidItem->data, &pVal, &valLen);
                    // Skip De-Activation of DESFire App AID for contactless operations to work.
					if(isDESFireAppAID(pVal, valLen))
					{
						continue;
					}

					// Deactivate VCM (Do not check SW)
					respLen = vcDescriptionFile_CreateTLV(0x4F, pVal, valLen, gTmpRespBuf, sizeof(gTmpRespBuf));
					apduLen = utils_MakeCAPDU(0x80, 0xF0, 0x01, 0x00, gTmpRespBuf, respLen, gApduBuf, sizeof(gApduBuf));
					exchangeWithSe(gApduBuf, apduLen, gTmpRespBuf, &respLen);
				}
			}
		}
	}
	status = eSW_NO_ERROR;

deactivateAllContactlessApplications_BAIL:
    if (tlvs != NULL)
    {
        tlvList_delete(tlvs);
        tlvs = NULL;
    }
	return status;
}

/*******************************************************
 *Returns subsequent Tags in parseTlvs
 ********************************************************/
UINT16 getSubsequentTlvs(int startNode, UINT16 tagToFind, t_tlvList* parseTlvs, UINT8* foundTlvs, UINT16 foundTlvSize)
{
	UINT8 tmpBuf[128];
	UINT16 tmpBufLen;
	UINT16 currentOffset = 0;
	UINT16 cnt;

	t_tlvListItem* item = parseTlvs->head;
	for(cnt = 0; item != NULL; item = item->next, cnt++)
	{
		if(cnt >= startNode)
		{
			if( tlv_getTag(item->data) == tagToFind )
			{
				tmpBufLen = sizeof(tmpBuf);
				tlv_getTlvAsBytes(item->data, tmpBuf, &tmpBufLen);
				utils_append(foundTlvs, &currentOffset, foundTlvSize, tmpBuf, 0, tmpBufLen);
			}
			else
			{
				break;
			}
		}
	}

	return currentOffset;
}

/*******************************************************
 *Returns either TLV Tag A0 or Tag A2
 ********************************************************/
t_TLV* getTlvA0A2(UINT8* setStatusRsp, UINT16 setStatusRspLen)
{
	t_tlvList *tlvs;
	t_tlvList *setStatusRspTlvs;
	t_TLV *tlvResult;
	t_TLV *retTlv;

	retTlv = NULL;

	ALOGD("%s: Enter", __func__);
    utils_LogByteArrayToHex("  getTlvA0A2 setStatusRsp: ", setStatusRsp, setStatusRspLen-2);

    tlvs = tlv_parse(setStatusRsp, setStatusRspLen-2);
    setStatusRspTlvs = tlv_getNodes(tlvList_get(tlvs, 0));
	tlvResult = tlvList_find(setStatusRspTlvs, kTAG_A0_AVAILABLE);

	if(tlvResult != NULL)
	{
		// create a copy of the result TLV so it can be returned to the caller
		retTlv = tlv_newTLV(tlvResult->tag, tlvResult->length, tlvResult->value);
	}
	else
	{
		tlvResult = tlvList_find(setStatusRspTlvs, kTAG_A2_AVAILABLE);
		if(tlvResult != NULL)
		{
			// create a copy of the result TLV so it can be returned to the caller
			retTlv = tlv_newTLV(tlvResult->tag, tlvResult->length, tlvResult->value);
		}
	}

	tlvList_delete(tlvs);
	ALOGD("%s: Exit", __func__);
	return retTlv;
}

/*******************************************************
 *Handles Personalisation During CreateVC
 * persoData     - personalization data buffer
 * persoDataLen  - length of the data in the perso buffer
 * pkg           - name of the current package
 * pkgLen        - length of the current package name
 * vcEntry       - current VC Entry
 *******************************************************/
UINT16 handlePersonalize(const UINT8 *persoData, UINT16 persoDataLen, const char *pkg, UINT16 pkgLen, UINT16 vcEntry)
{
	UINT8 sha[20];
	UINT16 respLen;
	UINT16 apduLen;

	ALOGD("%s: Enter", __func__);

	gCurrentProcess = eProcess_HandlePersonalize;

	utils_CreateSha(sha, (UINT8 *)gCurrentPackage, strlen(gCurrentPackage));
	vcDescriptionFile_CreatePersonalizeData(vcEntry, persoData, persoDataLen, sha, gApduBuf, sizeof(gApduBuf), &apduLen);

	respLen = sizeof(gTmpRespBuf);
	exchangeLtsmProcessCommand(gApduBuf, apduLen, gTmpRespBuf, &respLen);

	ALOGD("%s: Exit", __func__);
	return utils_GetSW(gTmpRespBuf, respLen);
}

/*******************************************************
 * \brief Handles Exchange of Data during LTSM Process Command
 *
 * \param pCData [IN] buffer containing the command data to send to eSE
 * \prarm cDataLen [IN] length of the data in the buffer
 *
 * Side effects: modifies gTmpRespBuf
 *               modifies gApduBuf
 *               modifies gRegistry (via process commands)
 *               modifies gStatusCode (via process commands)
 *
 * \return STATUS WORD
 *         eSW_PROCESSING_ERROR
 *         eSW_NO_ERROR
 ********************************************************/
void exchangeLtsmProcessCommand(UINT8* pCData, UINT16 cDataLen, UINT8* rapdu, UINT16* rapduLen)
{
    UINT8  cApdu[270];
    UINT16 cApduLen;
    UINT8* rapdu1;
    UINT16 rapdu1Len;
    UINT8  rapdu2[300];	// required as a temp storage when responding to a response greater than 255 bytes
    UINT16 rapdu2Len;
    UINT8  process;

    ALOGI("%s: Enter", __func__);

    if(cDataLen + 5 > (UINT16) sizeof(cApdu))
    {
		ALOGD("%s: cData is too big for the transmit buffer, cDataLen = %d, xmitBuffer = %d", __func__, cDataLen, sizeof(cApdu));
		*rapduLen = utils_createStatusCode(rapdu, eERROR_GENERAL_FAILURE);
    }

    //----
    // LTSM process command
    cApdu[0] = data_CLA_M4M_LTSM;
    cApdu[1] = data_INS_M4M_LTSM_PROCESS_COMMAND;
    cApdu[2] = 0;
    cApdu[3] = 0;
    cApdu[4] = cDataLen;
    memcpy(cApdu+5, pCData, cDataLen);
    cApduLen = cDataLen + 5;

    exchange(cApdu, cApduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), rapdu, rapduLen);

    switch(utils_GetSW(rapdu, *rapduLen))
    {
    case eSW_NO_ERROR:
    	ALOGI("%s: SW_NO_ERROR (A) ", __func__);
    	memcpy(cApdu, rapdu, *rapduLen-2);
    	cApduLen = *rapduLen - 2;
    	break;

    case eSW_6310_COMMAND_AVAILABLE:
    	process = TRUE;
    	while(process)
    	{
    		if(*rapduLen == 2)
    		{
    			process = FALSE;
    			break;
    		}

    		memcpy(cApdu, rapdu, *rapduLen-2);
    		cApduLen = *rapduLen - 2;
        	ALOGI("%s: SW_6310_AVAILABLE: Send Data back to SE ", __func__);

    	    *rapduLen = sizeof(gTmpRespBuf);
    		exchangeWithSe(cApdu, cApduLen, rapdu, rapduLen);
    		if(*rapduLen < 256)
    		{
    			cApduLen = utils_MakeCAPDU(data_CLA_M4M_LTSM,
    											data_INS_M4M_LTSM_PROCESS_SE_RESPONSE,
    											0x00, 0x80, rapdu, *rapduLen, cApdu, sizeof(cApdu));

    		    *rapduLen = sizeof(gTmpRespBuf);
    			exchange(cApdu, cApduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), rapdu, rapduLen);

    			switch(utils_GetSW(rapdu,*rapduLen))
    			{
					case eSW_NO_ERROR:
				    	ALOGI("%s: SW_NO_ERROR (B) ", __func__);
				    	process = FALSE;
						break;

					case eSW_6310_COMMAND_AVAILABLE:
				    	ALOGI("%s: SW_6310_AVAILABLE: Send Data back to SE Again (B)", __func__);
						break;

					default:
						process = FALSE;
						break;
    			}
    		}
    		else
    		{
    			rapdu1 = rapdu;
    			rapdu1Len = 255;

    			memcpy(rapdu2, rapdu + 255, *rapduLen - 255);
    			rapdu2Len = *rapduLen - 255;

    			//----- First Part
    			cApduLen = utils_MakeCAPDU(data_CLA_M4M_LTSM,
    											data_INS_M4M_LTSM_PROCESS_SE_RESPONSE,
    											0x00, 0x00, rapdu1, rapdu1Len, cApdu, sizeof(cApdu));

    		    *rapduLen = sizeof(gTmpRespBuf);
    		    exchange(cApdu, cApduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), rapdu, rapduLen);

    			if(utils_GetSW(rapdu,*rapduLen) != eSW_NO_ERROR)
    			{
    				ALOGD("%s: FAILED APDU exchange (PROCESS SE RESPONSE) (1 of 2) failed!", __func__);
    				*rapduLen = utils_createStatusCode(rapdu, eERROR_GENERAL_FAILURE);
    			}

    			//----- Second Part
    			cApduLen = utils_MakeCAPDU(data_CLA_M4M_LTSM,
    											data_INS_M4M_LTSM_PROCESS_SE_RESPONSE,
    											0x00, 0x80, rapdu2, rapdu2Len, cApdu, sizeof(cApdu));

    		    *rapduLen = sizeof(gTmpRespBuf);
    			exchange(cApdu, cApduLen, channelMgr_getLastChannelId(registry_GetChannelMgr(gRegistry)), rapdu, rapduLen);

    			switch(utils_GetSW(rapdu,*rapduLen))
    			{
					case eSW_NO_ERROR:
				    	ALOGI("%s: SW_NO_ERROR (C) ", __func__);
						process = FALSE;
						break;

					case eSW_6310_COMMAND_AVAILABLE:
                        // FALL THROUGH - do not Exit loop
						break;

					default:
						process = FALSE;
						break;
    			}

    		}
    	}
    	break;

    default:
    	break;
    }
    ALOGI("%s: Exit",__func__);
}


/*******************************************************
 * Deletes VC's which are not Created Properly
 *
 * \param vcEntry [IN] entry to be deleted
 * \param shaWalletName [IN] SHA of the app name used to create the entry
 *
 * Side effects: modifies gStatusCode (via ltsmStart and exhcnageLtsmProcessCommand)
 * 				 modifies gRegistry (via ltsmStart and exhcnageLtsmProcessCommand)
 * 				 modifies gTmpRespBuf (via and exhcnageLtsmProcessCommand)
 * 				 modifies gApduBuf (via and exhcnageLtsmProcessCommand)
 *
 * \return: STATUS_FAILED
 *         STATUS_SUCCESS
 ********************************************************/

UINT16 deleteTempVc(UINT16 vcEntry, UINT8* shaWalletName)
{
    UINT16	respLen;
    UINT16  apduLen;

    ALOGI("%s: Enter", __func__);

	gDeleteVCEntry = vcEntry;
	gCurrentProcess = eProcess_DeleteVirtualCard;

    ALOGI("%s: DeleteVCEntry = vcEntry: %d", __func__, vcEntry);

	vcDescriptionFile_DeleteVc(vcEntry, shaWalletName, gApduBuf, sizeof(gApduBuf), &apduLen);

	respLen = sizeof(gTmpRespBuf);
	exchangeLtsmProcessCommand(gApduBuf, apduLen, gTmpRespBuf, &respLen);

	ALOGI("%s: Exit", __func__);
	return utils_GetSW(gTmpRespBuf, respLen);
}

/*******************************************************
 * Opens Connection for Communication with SE
 * -- enable WIRED mode
 *
 * Return: STATUS_FAILED
 *         STATUS_SUCCESS
 ********************************************************/
tJBL_STATUS openConnection(void)
{
    tJBL_STATUS status = STATUS_FAILED;
    ALOGI("%s: Enter", __func__);

    if(gIChannel != NULL && gIChannel->open())
    {
        status = STATUS_SUCCESS;
    }
    ALOGI("%s: Exit", __func__);
    return status;
}

/*******************************************************
 * Close Connection for Communication with SE
 * -- disables WIRED mode
 *
 * Return: STATUS_FAILED
 *         STATUS_SUCCESS
 ********************************************************/
tJBL_STATUS closeConnection(void)
{
    tJBL_STATUS status = STATUS_FAILED;
    ALOGI("%s: Enter", __func__);

    if(gIChannel != NULL && gIChannel->close())
    {
        status = STATUS_SUCCESS;
    }
    ALOGI("%s: Exit", __func__);
    return status;
}

/*******************************************************
 * Transceives the APDU - LOWEST LEVEL - Raw Transceive
 * \param apdu [IN] buffer containing the APDU data to send
 * \param apduLen [IN] length of APDU data in buffer
 * \param addLeByte [IN] flag indicating whether APDU should be
 *                       updated with Le byte before being sent
 * \param rapdu [OUT] buffer into which the response will be written
 * \param rapduLen [IN/OUT] IN: size of rapdu buffer
 *                          OUT: number of bytes written into rapdu buffer
 *
 * Side effects: none
 *
 * Return: STATUS_FAILED
 *         STATUS_SUCCESS: rapdu will contain the response data
 *                         rapduLen will contain the lenght of the respons data
 ********************************************************/
tJBL_STATUS exchangeApdu(const UINT8* apdu, UINT16 apduLen, UINT8* rapdu, UINT16* rapduLen)
{
    INT32 bytesReceived;
    UINT8 stat;
    UNUSED(bytesReceived)

    ALOGI("%s: Enter", __func__);

    if(gIChannel == NULL)
    {
    	*rapduLen = 0;
    	return STATUS_FAILED;
    }

	ALOGD("%s, Sending Data: rapduLen = %d", __func__, *rapduLen);
    utils_LogByteArrayToHex("Send", apdu, apduLen);

    stat = gIChannel->transceive ((UINT8 *) apdu, apduLen, rapdu, *rapduLen, &bytesReceived, gMOPTransceiveTimeout);
    if(!stat)
    {
    	*rapduLen = 0;
    	return STATUS_FAILED;
    }

    if(bytesReceived <= *rapduLen)
    {
    	*rapduLen = bytesReceived;
    }

	ALOGD("%s, rData:", __func__);
    utils_LogByteArrayToHex("rData", rapdu, *rapduLen);

    ALOGI("%s: Exit", __func__);
    return STATUS_SUCCESS;
}

/*******************************************************
 * Exchange APDU With Secure Element
 *  -- calls exchange() which calls exchangeApdu()
 *
 * \param cApdu [IN] buffer containing the APDU data to send
 * \param cApduLen [IN] length of APDU data in buffer
 * \param rapdu [OUT] buffer into which the response will be written
 * \param rapduLen [IN/OUT] IN: size of rapdu buffer
 *                          OUT: number of bytes written into rapdu buffer
 *
 * Side effects: none
 *
 * Return: STATUS_FAILED
 *         STATUS_SUCCESS: rapdu will contain the response data
 *                         rapduLen will contain the length of the response data
 ********************************************************/
tJBL_STATUS exchangeWithSe(UINT8* cApdu, UINT16 cApduLen, UINT8* rapdu, UINT16* rapduLen)
{
    tJBL_STATUS status = STATUS_FAILED;

    ALOGI("%s: Enter", __func__);

    if(cApdu[1] == 0x70) // Manage channel ?
    {
    	if(cApdu[2] == 0x00) // [open]
    	{
    		cApdu[4] = 0x01;
    	}
    }

    status = exchange(cApdu, cApduLen,  (cApdu[0] & 0x03), rapdu, rapduLen);

    ALOGI("%s: Exit", __func__);
	return status;
}


/*******************************************************
 * Common Exchange method for Exchange with SE and Process LTSM Command
 *  - handles inserting the channel ID
 *  - then callse exchangeApdu()
 *
 * \param cApdu [IN] buffer containing the APDU data to send
 * \param cApduLen [IN] length of APDU data in buffer
 * \param chnl_id [IN] ID of the channel on which the message should be sent
 * \param rapdu [OUT] buffer into which the response will be written
 * \param rapduLen [IN/OUT] IN: size of rapdu buffer
 *                          OUT: number of bytes written into rapdu buffer
 *
 * Side effects: none
 *
 * Return: STATUS_FAILED:
 *         STATUS_SUCCESS: rapdu will contain the response data
 *                         rapduLen will contain the length of the response data
 ********************************************************/
tJBL_STATUS exchange(UINT8* cApdu, UINT16 cApduLen, int chnl_id, UINT8* rapdu, UINT16* rapduLen)
{
    INT32 bytesReceived;
    tJBL_STATUS res;
    UNUSED(bytesReceived)

    ALOGI("%s: Enter", __func__);

    cApdu[0] = utils_adjustCLA(cApdu[0], chnl_id);

    res = exchangeApdu(cApdu, cApduLen, rapdu, rapduLen);
    if(STATUS_SUCCESS != res)
    {
    	*rapduLen = 0;
    	return STATUS_FAILED;
    }

    if(*rapduLen == 0)
    {
    	ALOGD("%s, SE Transceive Failed", __func__);
    	*rapduLen = utils_createStatusCode(rapdu, eSW_NO_DATA_RECEIVED);
    }

    ALOGI("%s: Exit", __func__);
    return STATUS_SUCCESS;
}


/*******************************************************
 * \brief Processes Create VC Response
 * 	      - find the registry entry with the current AppName AND the current VCEntry
 * 	      - extract the UID from the response and store it in the entry
 * 	      - set the create status of the entry to TRUE
 *
 * \param rapdu [IN] buffer containing the data to parse
 * \param rapduLen [IN] length of data in the buffer
 *
 * Side effects: gRegistry updated
 ********************************************************/
void processCreateVcResponse(UINT8* rapdu, UINT16 rapduLen)
{
    UINT8 *value;
    UINT16 valSize;

    ALOGI("%s: Enter", __func__);
    ALOGI("%s: currentPackage : %s", __func__, gCurrentPackage);

	value = utils_getValue(0x41, rapdu, rapduLen, &valSize);
	memcpy(gVcUid, value, valSize);
	gVcUidLen = valSize;

	ALOGI("%s: After getValue()",__func__);
    utils_LogByteArrayToHex("  gVcUid: ", gVcUid, gVcUidLen);
}

UINT8 inList(UINT16 value, const UINT16* tags, UINT8 tagsCount)
{
	int i;

	for(i=0; i<tagsCount; i++)
	{
		if (tags[i] == value)
		{
			return TRUE;
		}
	}
	return FALSE;
}

void removeTags(const UINT8* vcData, UINT16 vcDataLen, const UINT16* tags, UINT8 tagsCount, UINT8* resultData, UINT16* resDataLen)
{
	t_tlvList* tlvs = tlvList_newList();	// working list into which tags to keep are placed

	// parse the data
	t_tlvList* inputTags = tlv_parseWithPrimitiveTags(vcData, vcDataLen, data_PrimitiveTagSet, ARRAY_COUNT(data_PrimitiveTagSet));

	// iterate through the parsed tags and add them to the working list, only if they are NOT
	// in the list of tags to be removed
	t_tlvListItem* tmp;
	t_tlvListItem* item = inputTags->head;
	while(item != NULL)
	{
        tmp = item->next;
		if( !inList(tlv_getTag(item->data), tags, tagsCount) )
		{
			tlvList_add(tlvs, item->data);
	        GKI_os_free(item);
		}
		else
		{
		    tlvListItem_delete(item);
		}
        item = tmp;
	}

	tlvList_make(tlvs, resultData, resDataLen);
	tlvList_delete(tlvs);
	GKI_os_free(inputTags);
}
#endif/*MIFARE_OP_ENABLE*/
