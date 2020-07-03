/*
 * Copyright (C) 2014-2015 NXP Semiconductors
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
#define LOG_TAG 			"NXP_MOP_vcDescription"

#include "vcDescription.h"
#include "p61_data_types.h"
#if(MIFARE_OP_ENABLE == TRUE)
#include "utils.h"
#ifndef COMPANION_DEVICE
#include "gki.h"
#include "OverrideLog.h"
#endif
#include "statusCodes.h"
#include "tlv.h"

UINT16 vcDescriptionFile_CreateTLVHeader(UINT16 tag, UINT16 dataLen, UINT8* dest, UINT16 destSize);

tJBL_STATUS vcDescriptionFile_CreateVc(UINT16 vcEntry, UINT8* pVCData, UINT16 vcDataLen, UINT8* pSHA, UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen)
{
    UINT16 totalBytes = 0;
    UINT16 bytes;
    UINT16 netVc;
    UINT8* pTmpDst;
    UINT16 tmpDstAvailable;
    tJBL_STATUS status = STATUS_SUCCESS;
    UINT8 tmpBuf[32];

    ALOGD("--- %s ---: Enter",__func__);
    if(respBufSize < (22 + 4 + 4 + vcDataLen))
    {
    	status = STATUS_FAILED;
    	goto createVC_bail;
    }

    //-----
    // create the 0x4F TLV into tmpBuf[32] - should use 22 bytes
    //	original code uses the AppName, this implementation uses the SHA of the appName
    bytes = vcDescriptionFile_CreateTLV(0x4F, pSHA, 20, tmpBuf, 32);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createVC_bail;
    }

    totalBytes += bytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // create the 0x40 TLV appended to the 0x4F TLV - should use 4 bytes for a total of 26 bytes
    pTmpDst = tmpBuf + totalBytes;
    tmpDstAvailable = 32 - totalBytes;
    netVc = utils_htons(vcEntry);
    bytes = vcDescriptionFile_CreateTLV(0x40, (UINT8 *) &netVc, 2, pTmpDst, tmpDstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createVC_bail;
    }

    totalBytes += bytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // create the 0x70 TLV HEADER ONLY - (0x70.. (0x4F...)(0x40...)(vcData))
    bytes = vcDescriptionFile_CreateTLVHeader(0x70, totalBytes + vcDataLen, pResp, respBufSize);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createVC_bail;
    }

    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // Now copy the (0x4F...)(0x40...)(vcData) into the data area
    memcpy(pResp+bytes, tmpBuf, totalBytes);				// (0x4F...)(0x40...)
    memcpy(pResp+bytes+totalBytes, pVCData, vcDataLen);	// (vcData)

    *pRespLen = bytes + totalBytes + vcDataLen;

createVC_bail:
    	ALOGD("--- %s ---: Exit -- status = %s", __func__, (STATUS_SUCCESS == status)?"STATUS_SUCCESS":"STATUS_FAILED");
    	return status;
}

tJBL_STATUS vcDescriptionFile_CreateF8Vc(UINT16 vcEntry, UINT8* pSHA, UINT8* mfdfaid, UINT8 mfdfaidLen, UINT8* cltecap, UINT8 cltecapLen, UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen)
{
	UINT16 bytes;
	UINT16 totalBytes;
    UINT16 tmpDstAvailable;
    tJBL_STATUS status = STATUS_SUCCESS;
    UINT8 tmpBuf[64];
    UINT8* pTmpDst;
    UINT16 netVc;

    ALOGD("--- %s ---: Enter",__func__);

    if(respBufSize < (22 + 4 + 4 + (mfdfaidLen + 2) + (cltecapLen + 2)))  // 22: TLV 4F, 4: TLV 40, 4: TLV 70
    {
    	status = STATUS_FAILED;
    	goto createF8VC_bail;
    }

    totalBytes = 0;
    tmpDstAvailable = sizeof(tmpBuf);

    //-----
    //  TLV 0x4F -- SAME AS CreateVc --
    //	create the 0x4F TLV into tmpBuf[64] - should use 22 bytes
    //	original code uses the AppName, this implementation uses the SHA of the appName
    bytes = vcDescriptionFile_CreateTLV(0x4F, pSHA, 20, tmpBuf, 32);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createF8VC_bail;
    }

    totalBytes += bytes;
    tmpDstAvailable -= totalBytes;
    pTmpDst = tmpBuf + totalBytes;
    ALOGD("TLV 4F -- %s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // TLV 0x40 -- SAME AS CreateVc
    // create the 0x40 TLV appended to the 0x4F TLV - should use 4 bytes for a total of 26 bytes
    netVc = utils_htons(vcEntry);
    bytes = vcDescriptionFile_CreateTLV(0x40, (UINT8 *) &netVc, 2, pTmpDst, tmpDstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createF8VC_bail;
    }

    totalBytes += bytes;
    tmpDstAvailable -= totalBytes;
    pTmpDst = tmpBuf + totalBytes;
    ALOGD("TLV 40 -- %s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // TLV 0xF8 -- Specific to CreateF8Vc
    // create the 0xF8 TLV appended to the 0x40 TLV - should use XX bytes for a total of XX bytes
    bytes = vcDescriptionFile_CreateTLV(0xF8, mfdfaid, mfdfaidLen, pTmpDst, tmpDstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createF8VC_bail;
    }

    totalBytes += bytes;
    tmpDstAvailable -= totalBytes;
    pTmpDst = tmpBuf + totalBytes;
    ALOGD("TLV F8 -- %s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // TLV 0xE2 -- Specific to CreateF8Vc
    // create the 0xE2 TLV appended to the 0xF8 TLV - should use XX bytes for a total of XX bytes
    if(cltecapLen > 0)
    {
		bytes = vcDescriptionFile_CreateTLV(0xE2, cltecap, cltecapLen, pTmpDst, tmpDstAvailable);
		if(bytes == 0)
		{
			status = STATUS_FAILED;
			goto createF8VC_bail;
		}

		totalBytes += bytes;
		tmpDstAvailable -= totalBytes;
		ALOGD("TLV E2 -- %s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);
    }

    //-----
    // create the 0x74 TLV HEADER ONLY, leaving space for the rest: (0x74.. (0x4F...)(0x40...)(0xF8...)(0xE2...))
    bytes = vcDescriptionFile_CreateTLVHeader(0x74, totalBytes, pResp, respBufSize);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createF8VC_bail;
    }

    //-----
    // Now copy the (0x4F...)(0x40...)(0xF8...)(0xE2...) into the data area of TLV 0x74
    memcpy(pResp+bytes, tmpBuf, totalBytes);			// (0x4F...)(0x40...)

    *pRespLen = bytes + totalBytes;

createF8VC_bail:
    	ALOGD("--- %s ---: Exit -- status = %s", __func__, (STATUS_SUCCESS == status)?"STATUS_SUCCESS":"STATUS_FAILED");
    	return status;

}

tJBL_STATUS vcDescriptionFile_CreatePersonalizeData(UINT16 vcEntry, const UINT8* persoData, UINT16 persoDataLen, UINT8 pSha[20], UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen)
{
    UINT16 totalBytes;
    UINT16 bytes;
    UINT16 netVc;
    UINT8* pDst;
    UINT16 dstAvailable;
    tJBL_STATUS status = STATUS_SUCCESS;
    UINT8 tmpBuf[256];


    // length of persoData will be between 172 and 194 bytes, inclusive

    ALOGD("--- %s ---: Enter, persoDataLen = %d", __func__, persoDataLen);

    *pRespLen = 0;
    if(respBufSize < (22 + 4 + persoDataLen + 22 + 4))
    {
    	status = STATUS_FAILED;
    	goto createPerso_bail;
    }

    totalBytes = 0;
    pDst = tmpBuf + totalBytes;
    dstAvailable = sizeof(tmpBuf) - totalBytes;

    //-----
    // Start with SHA in tag (4F...) (22 bytes)
    bytes = vcDescriptionFile_CreateTLV(0x4F, pSha, 20, pDst, dstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createPerso_bail;
    }

    totalBytes += bytes;
    pDst = tmpBuf + totalBytes;
    dstAvailable = sizeof(tmpBuf) - totalBytes;

    //-----
    // append vcEntry in tag (40...) (4 bytes)
    netVc = utils_htons(vcEntry);
    bytes = vcDescriptionFile_CreateTLV(0x40, (UINT8 *) &netVc, 2, pDst, dstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createPerso_bail;
    }

    totalBytes += bytes;
    pDst = tmpBuf + totalBytes;
    dstAvailable = sizeof(tmpBuf) - totalBytes;

    //-----
    // append persoData (persoDataLen)
    memcpy(pDst, persoData, persoDataLen);

    totalBytes += persoDataLen;
    pDst = tmpBuf + totalBytes;
    dstAvailable = sizeof(tmpBuf) - totalBytes;

    //-----
    // append second SHA of ??? in tag (C1...) (22 bytes)
    bytes = vcDescriptionFile_CreateTLV(0xC1, pSha, 20, pDst, dstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createPerso_bail;
    }

    totalBytes += bytes;

    //-----
    // put all of the above into tag (73...(4F..)(40..)(persoData)(C1..))
    bytes = vcDescriptionFile_CreateTLV(0x73, tmpBuf, totalBytes, pResp, respBufSize);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createPerso_bail;
    }

    *pRespLen = bytes;

createPerso_bail:
	ALOGD("--- %s ---: Exit -- persoDataLen = %d, status = %s", __func__, *pRespLen, (STATUS_SUCCESS == status)?"STATUS_SUCCESS":"STATUS_FAILED");
	return status;
}

tJBL_STATUS vcDescriptionFile_DeleteVc(UINT16 vcEntry, UINT8 pSHA[20], UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen)
{
    UINT16 totalBytes;
    UINT16 bytes;
    UINT16 netVc;
    UINT8* pTmpDst;
    UINT16 tmpDstAvailable;
    tJBL_STATUS status = STATUS_SUCCESS;
    UINT8 tmpBuf[32];

    ALOGD("--- %s ---: Enter", __func__);
	*pRespLen = 0;
	totalBytes = 0;

    if(respBufSize < (22 + 4 + 4))
    {
    	status = STATUS_FAILED;
    	goto deleteVC_bail;
    }

    //-----
    // Start with SHA in tag (4F...) (22 bytes)
    bytes = vcDescriptionFile_CreateTLV(0x4F, pSHA, 20, tmpBuf, 32);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto deleteVC_bail;
    }

    totalBytes += bytes;
    pTmpDst = tmpBuf + totalBytes;
    tmpDstAvailable = 32 - totalBytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // append vcEntry in tag (40...) (4 bytes)
    netVc = utils_htons(vcEntry);
    bytes = vcDescriptionFile_CreateTLV(0x40, (UINT8 *) &netVc, 2, pTmpDst, tmpDstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto deleteVC_bail;
    }

    totalBytes += bytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // put all of the above into tag (71...(4F..)(40..))
    bytes = vcDescriptionFile_CreateTLV(0x71, tmpBuf, totalBytes, pResp, respBufSize);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto deleteVC_bail;
    }

    *pRespLen = bytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

deleteVC_bail:
	ALOGD("--- %s ---: Exit -- status = %s", __func__, (STATUS_SUCCESS == status)?"STATUS_SUCCESS":"STATUS_FAILED");
	return status;
}

tJBL_STATUS vcDescriptionFile_CreateVCResp(UINT16 vcEntry, UINT8* pUid, UINT8 UidLen, UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen)
{
    UINT16 totalBytes;
    UINT16 bytes;
    UINT16 netVc;
    UINT8* pDst;
    UINT16 dstAvailable;
    tJBL_STATUS status = STATUS_SUCCESS;
    UINT8 stat[] = {0x90, 0x00};

    ALOGD("--- %s ---: Enter",__func__);
    if(respBufSize < (4 + 2 + UidLen + 4))
    {
    	status = STATUS_FAILED;
    	goto createVcResp_bail;
    }
    ALOGD("--- Success on bufsize ---");

    totalBytes = 0;
    pDst = pResp + totalBytes;
    dstAvailable = respBufSize - totalBytes;

    //-----
    // TLV 0x40 -- vcEntry
    // create the 0x40 TLV as the first entry
    netVc = utils_htons(vcEntry);
    bytes = vcDescriptionFile_CreateTLV(0x40, (UINT8 *) &netVc, 2, pDst, dstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createVcResp_bail;
    }
    ALOGD("--- Success on creating VCEntry TLV ---");
    utils_LogByteArrayToHex("TLV: ", pDst, bytes);

    totalBytes += bytes;
    pDst = pResp + totalBytes;
    dstAvailable = respBufSize - totalBytes;

    //-----
    // TLV 0x41 -- vcEntry
    // append the 0x41 TLV to the 0x40 TLV
    bytes = vcDescriptionFile_CreateTLV(0x41, pUid, UidLen, pDst, dstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createVcResp_bail;
    }
    ALOGD("--- Success on creating UID TLV ---");
    utils_LogByteArrayToHex("TLV: ", pDst, bytes);

    totalBytes += bytes;
    pDst = pResp + totalBytes;
    dstAvailable = respBufSize - totalBytes;

    //-----
    // TLV 0x4E -- status (stat)
    // append the 0x4E TLV to the 0x41 TLV
    bytes = vcDescriptionFile_CreateTLV(0x4E, stat, 2, pDst, dstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto createVcResp_bail;
    }
    ALOGD("--- Success on creating response code TLV ---");
    utils_LogByteArrayToHex("TLV: ", pDst, bytes);

    totalBytes += bytes;

    utils_LogByteArrayToHex("complete TLV set: ", pResp, totalBytes);

    *pRespLen = totalBytes;

createVcResp_bail:
	return status;
}

tJBL_STATUS vcDescriptionFile_AddandUpdateMDAC(UINT16 vcEntry, const UINT8* pVCData, UINT16 vcDataLen, UINT8 pSha[20], UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen)
{
    UINT16 totalBytes = 0;
    UINT16 bytes;
    UINT16 netVc;
    UINT8* pTmpDst;
    UINT16 tmpDstAvailable;
    tJBL_STATUS status = STATUS_SUCCESS;
    UINT8 tmpBuf[32];

    ALOGD("--- %s ---: Enter",__func__);
    *pRespLen = 0;

    if(respBufSize < (22 + 4 + 4 + vcDataLen))
    {
    	status = STATUS_FAILED;
    	goto mdacVC_bail;
    }

    //-----
    // create the 0x4F TLV into tmpBuf[32] - should use 22 bytes
    bytes = vcDescriptionFile_CreateTLV(0x4F, pSha, 20, tmpBuf, 32);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto mdacVC_bail;
    }

    totalBytes += bytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // create the 0x40 TLV appended to the 0x4F TLV - should use 4 bytes for a total of 26 bytes
    pTmpDst = tmpBuf + totalBytes;
    tmpDstAvailable = 32 - totalBytes;
    netVc = utils_htons(vcEntry);
    bytes = vcDescriptionFile_CreateTLV(0x40, (UINT8 *) &netVc, 2, pTmpDst, tmpDstAvailable);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto mdacVC_bail;
    }

    totalBytes += bytes;
    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // create the 0x70 TLV HEADER ONLY - (0x72.. (0x4F...)(0x40...)(vcData))
    bytes = vcDescriptionFile_CreateTLVHeader(0x72, totalBytes + vcDataLen, pResp, respBufSize);
    if(bytes == 0)
    {
    	status = STATUS_FAILED;
    	goto mdacVC_bail;
    }

    ALOGD("%s: bytes = %d, totalBytes = %d", __func__, bytes, totalBytes);

    //-----
    // Now copy the (0x4F...)(0x40...)(vcData) into the data area
    memcpy(pResp+bytes, tmpBuf, totalBytes);			// (0x4F...)(0x40...)
    memcpy(pResp+bytes+totalBytes, pVCData, vcDataLen);	// (vcData)

    *pRespLen = bytes + totalBytes + vcDataLen;

mdacVC_bail:
    	ALOGD("--- %s ---: Exit -- status = %s", __func__, (STATUS_SUCCESS == status)?"STATUS_SUCCESS":"STATUS_FAILED");
    	return status;
}

//-------------
UINT16 vcDescriptionFile_CreateTLV(UINT16 tag, const UINT8* data, UINT16 dataLen, UINT8* dest, UINT16 destSize)
{
	UINT16 idx = 0;

	if (dataLen + 5 > destSize)
	{
		return 0;
	}
	else
	{
		if ((tag & 0xFF00) == 0x0000)
		{
			dest[idx++] = (UINT8) tag;
		}
		else
		{
			dest[idx++] = (UINT8) (tag >> 8);
			dest[idx++] = (UINT8) (tag & 0x00FF);
		}

		if (dataLen < 128)
		{
			dest[idx++] = (UINT8) dataLen;
		}
		else if (dataLen < 256)
		{
			dest[idx++] = 0x81;
			dest[idx++] = (UINT8) dataLen;
		}
		else
		{
			dest[idx++] = 0x82;
			dest[idx++] = (UINT8) (dataLen >> 8);
			dest[idx++] = (UINT8) (dataLen & 0x00FF);
		}

		if(dataLen > 0)
		{
			if(data != NULL)
			{
				memcpy(dest+idx, data, dataLen);
			}
			else
			{
				memset(dest+idx, 0, dataLen);
			}
		}

		return dataLen + idx;
	}
}

UINT16 vcDescriptionFile_CreateTLVHeader(UINT16 tag, UINT16 dataLen, UINT8* dest, UINT16 destSize)
{
	UINT16 idx = 0;

	if (dataLen + 5 > destSize)
	{
		return 0;
	}
	else
	{
		if ((tag & 0xFF00) == 0x0000)
		{
			dest[idx++] = (UINT8) tag;
		}
		else
		{
			dest[idx++] = (UINT8) (tag >> 8);
			dest[idx++] = (UINT8) (tag & 0x00FF);
		}

		if (dataLen < 128)
		{
			dest[idx++] = (UINT8) dataLen;
		}
		else if (dataLen < 256)
		{
			dest[idx++] = 0x81;
			dest[idx++] = (UINT8) dataLen;
		}
		else
		{
			dest[idx++] = 0x82;
			dest[idx++] = (UINT8) (dataLen >> 8);
			dest[idx++] = (UINT8) (dataLen & 0x00FF);
		}

		return idx;
	}
}

UINT16 vcDescriptionFile_GetVC(UINT8* dest, UINT16 destSize)
{
	return vcDescriptionFile_CreateTLV(0x75, NULL, 0, dest, destSize);
}

UINT16 vcDescriptionFile_GetVcListResp(t_tlvList *getStatusRspTlvs, UINT8* pResp, UINT16 pRespSize)
{
	UINT16 dataSize;
	UINT8 dataBuf[64];
	UINT8* pBuf;
	UINT16 respSize;
	UINT16 totalRespSize = 0;
	UINT8* pOut = pResp;

	t_tlvListItem* item = getStatusRspTlvs->head;
	for(;item != NULL; item = item->next)
	{
		t_TLV* tlvVCstate = tlvList_find(tlv_getNodes(item->data), kTAG_VC_STATE);
		if((tlvVCstate->value)[0] == kVC_CREATED)
		{
			t_TLV* tlvVCx = tlvList_find(tlv_getNodes(item->data), kTAG_VC_ENTRY);
			t_TLV* tlvApkId = tlvList_find(tlv_getNodes(item->data), kTAG_SP_AID);

			dataSize = sizeof(dataBuf);
			pBuf = dataBuf;
			tlv_getTlvAsBytes(tlvVCx, pBuf, &dataSize);

			pBuf = dataBuf + dataSize;
			dataSize = sizeof(dataBuf) - dataSize;
			tlv_getTlvAsBytes(tlvApkId, pBuf, &dataSize);

			dataSize = (pBuf + dataSize) - dataBuf;
			respSize = vcDescriptionFile_CreateTLV(0x61, dataBuf, dataSize, pOut, pRespSize);

			pRespSize -= respSize;
			pOut += respSize;
			totalRespSize += respSize;
		}
	}

	return totalRespSize;
}

#endif/*MIFARE_OP_ENABLE*/
