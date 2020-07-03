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
#define LOG_TAG 			"MOPUtil"

#include "utils.h"
#include "p61_data_types.h"
#if(MIFARE_OP_ENABLE == TRUE)
#ifndef COMPANION_DEVICE
#include "gki.h"
#include "OverrideLog.h"
#endif
#include "sha1.h"
#include "statusCodes.h"

#define utils_swap16(X) ( ((((UINT16)(X)) & 0x00FF) << 8) | ((((UINT16)(X)) & 0xFF00) >> 8) )

UINT8 utils_isBigEndian()
{
	UINT16 tmp = 0x1234;
	if(*((UINT8 *)(&tmp)) == 0x12)
		return TRUE;

	return FALSE;
}

UINT16 utils_ntohs(UINT16 input)
{
	if(!utils_isBigEndian())
	{
		ALOGD("LITTLE ENDIAN");
		return utils_swap16(input);
	}

	return input;
}

UINT16 utils_htons(UINT16 input)
{
	if(!utils_isBigEndian())
	{
		return utils_swap16(input);
	}

	return input;
}

UINT16 utils_AsciiHexToBytes(const UINT8* data, UINT16 dataLen, UINT8* pOut, UINT16 outSize)
{
	UINT16 i = 0;
	UINT16 x = 0;
	const UINT8* p = data;

	if( ((dataLen / 2) > outSize) || ((dataLen & 0x0001) == 1) )
		return 0; /* Odd Number of Nibbles or insufficient output buffer, fail the operation */

	while(i < dataLen)
	{
		if( ((*p >= 0x30) && (*p <= 0x39)) )
		{
			if( (i & 0x0001) == 0 )
			{
				pOut[x] = (*p - 0x30) << 4;
			}
			else
			{
				pOut[x] |= (*p - 0x30);
			}
		}
		else if( ((*p >= 0x41) && (*p <= 0x46)) )
		{
			if( (i & 0x0001) == 0 )
			{
				pOut[x] = (*p - 0x37) << 4;
			}
			else
			{
				pOut[x] |= (*p - 0x37);
			}

		}
		else if( ((*p >= 0x61) && (*p <= 0x66)) )
		{
			if( (i & 0x0001) == 0 )
			{
				pOut[x] = (*p - 0x57) << 4;
			}
			else
			{
				pOut[x] |= (*p - 0x57);
			}
		}
		else
		{
			return 0; /* Non-Hex digit encountered, fail the operation */
		}

		if((i & 0x0001) == 1)
			x++;
		p++;
		i++;
	};

	return x;
}

void utils_LogByteArrayToHex(const char *label, const UINT8 *data, UINT16 dataLen)
{
	char 	*tmpString;
	UINT16	i = 0, j;

	tmpString = GKI_os_malloc(991);/* Extra one byte for String terminator */
	if(NULL != tmpString)
	{
		ALOGD("%s: --start", label);
		for(j=0; i < dataLen; j++) {
		    /* Log cannot print big strings. So let's print max 330 bytes+330 spaces = (330*2)+330=990 characters at a time */
			memset(tmpString, 0, 991);
			for (i = (j*330); ((i < ((j+1)*330)) && (i < dataLen)); i++) {
				sprintf(tmpString, "%s %02X", tmpString, data[i]);
			}
			ALOGD("%s", tmpString);
		}
		ALOGD("%s: --end", label);
		GKI_os_free(tmpString);
	}
}

UINT16 utils_GetSW(const UINT8 *buf, UINT16 bufLen)
{
	return ( ((UINT16) buf[bufLen-2]) << 8 ) | ( (UINT16) buf[bufLen-1] );
}

UINT16 utils_LenOfTLV(UINT8* tlv, UINT16 offset)
{
	UINT16 tlvLen = 0;

	if(tlv != NULL)
	{
		if(tlv[offset] == 0x82)
		{
			tlvLen = tlv[offset+1]<<8 | tlv[offset+2];
		}
		else if(tlv[offset] == 0x81)
		{
			tlvLen = tlv[offset+1];
		}
		else if(tlv[offset] <= 0x7F)
		{
			tlvLen = tlv[offset];
		}
		else
		{
			/*Error case*/
		}
	}
	return tlvLen;
}

UINT16 utils_SkipLenTLV(UINT8* tlv, UINT16 offset)
{
	UINT16 skipLen = 1;

	if(tlv != NULL)
	{
		if(tlv[offset] == 0x82)
		{
			skipLen = 3;
		}
		else if(tlv[offset] == 0x81)
		{
			skipLen = 2;
		}
		else if(tlv[offset] <= 0x7F)
		{
			skipLen = 1;
		}
		else
		{
			/*Error case*/
		}
	}
	return skipLen;
}

UINT8* utils_FindNextAID(UINT8* byteArr, UINT16 byteArrLen, UINT16 offset, UINT16* aidLen)
{
	UINT8* aid;

	if( (byteArr[offset] + 2) > byteArrLen)
		return NULL;

	// Include the 4F intro tag (e.g. 4F 10 A0...)
	*aidLen = byteArr[offset] + 2;
	aid = byteArr + offset - 1;

	return aid;
}

// Input is in data and len specified in dataLen
// result will always be 20 bytes put into buf
tJBL_STATUS utils_CreateSha(UINT8 *buf, UINT8* data, UINT8 dataLen)
{
	SHA1Context sha;
	INT8 i, j;

	SHA1Reset(&sha);
	SHA1Input(&sha, data, dataLen);
	if(!SHA1Result(&sha))
		return STATUS_FAILED;

	for(i=0; i<5; i++)
	{
		for(j=3; j>=0; j--)
		{
			*buf++ = (sha.Message_Digest[i] >> (8*j)) & 0x000000FF;
		}
	}

	return STATUS_SUCCESS;
}

tJBL_STATUS utils_CopyUid(UINT8* dest, UINT8 destMaxLen, UINT8 *src, UINT8 srcLen)
{
	if(dest != NULL && src != NULL)
	{
		if(srcLen <= destMaxLen)
		{
			memcpy(dest, src, srcLen);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_FAILED;
}

UINT16 utils_createStatusCode(UINT8* buf, UINT16 statusWord)
{
	if(buf == NULL)
		return 0;

	buf[0] = 0x4E;
	buf[1] = 2;
	buf[2] = (statusWord >> 8) & 0x00FF;
	buf[3] = statusWord & 0x00FF;

	return 4;
}

char* utils_getStringStatusCode(UINT16 statusWord)
{
    char *pStr = NULL;
	switch(statusWord)
	{
	case (UINT16)eCONDITION_OF_USE_NOT_SATISFIED:
		pStr = "eCONDITION_OF_USE_NOT_SATISFIED";
		break;
	case (UINT16)eSW_INVALID_VC_ENTRY:
		pStr = "eSW_INVALID_VC_ENTRY";
		break;
	case (UINT16)eSW_OPEN_LOGICAL_CHANNEL_FAILED:
		pStr = "eSW_OPEN_LOGICAL_CHANNEL_FAILED";
		break;
	case (UINT16)eSW_SELECT_LTSM_FAILED:
		pStr = "eSW_SELECT_LTSM_FAILED";
		break;
	case (UINT16)eSW_REGISTRY_FULL:
		pStr = "eSW_REGISTRY_FULL";
		break;
	case (UINT16)eSW_IMPROPER_REGISTRY:
		pStr = "eSW_IMPROPER_REGISTRY";
		break;
	case (UINT16)eSW_NO_SET_SP_SD:
		pStr = "eSW_NO_SET_SP_SD";
		break;
	case (UINT16)eSW_ALREADY_ACTIVATED:
		pStr = "eSW_ALREADY_ACTIVATED";
		break;
	case (UINT16)eSW_ALREADY_DEACTIVATED:
		pStr = "eSW_ALREADY_DEACTIVATED";
		break;
	case (UINT16)eSW_CRS_SELECT_FAILED:
		pStr = "eSW_CRS_SELECT_FAILED";
		break;
	case (UINT16)eSW_SE_TRANSCEIVE_FAILED:
		pStr = "eSW_SE_TRANSCEIVE_FAILED";
		break;
	case (UINT16)eSW_REGISTRY_IS_EMPTY:
		pStr = "eSW_REGISTRY_IS_EMPTY";
		break;
	case (UINT16)eSW_NO_DATA_RECEIVED:
		pStr = "eSW_NO_DATA_RECEIVED";
		break;
	case (UINT16)eSW_EXCEPTION:
		pStr = "eSW_EXCEPTION";
		break;
	case (UINT16)eSW_CONDITION_OF_USE_NOT_SATISFIED:
		pStr = "eSW_CONDITION_OF_USE_NOT_SATISFIED";
		break;
	case (UINT16)eSW_OTHER_ACTIVEVC_EXIST:
		pStr = "eSW_OTHER_ACTIVEVC_EXIST";
		break;
	case (UINT16)eSW_CONDITIONAL_OF_USED_NOT_SATISFIED:
		pStr = "eSW_CONDITIONAL_OF_USED_NOT_SATISFIED";
		break;
	case (UINT16)eSW_INCORRECT_PARAM:
		pStr = "eSW_INCORRECT_PARAM";
		break;
	case (UINT16)eSW_REFERENCE_DATA_NOT_FOUND:
		pStr = "eSW_REFERENCE_DATA_NOT_FOUND";
		break;
	case (UINT16)eSW_VC_IN_CONTACTLESS_SESSION:
		pStr = "eSW_VC_IN_CONTACTLESS_SESSION";
		break;
	case (UINT16)eSW_UNEXPECTED_BEHAVIOR:
		pStr = "eSW_UNEXPECTED_BEHAVIOR";
		break;
	case (UINT16)eSW_NO_ERROR:
		pStr = "eSW_NO_ERROR";
		break;
	case (UINT16)eSW_6310_COMMAND_AVAILABLE:
		pStr = "eSW_6310_COMMAND_AVAILABLE";
		break;
	case (UINT16)eERROR_NO_PACKAGE:
		pStr = "eERROR_NO_PACKAGE";
		break;
	case (UINT16)eERROR_NO_MEMORY:
		pStr = "eERROR_NO_MEMORY";
		break;
	case (UINT16)eERROR_BAD_HASH:
		pStr = "eERROR_BAD_HASH";
		break;
	case (UINT16)eERROR_GENERAL_FAILURE:
		pStr = "eERROR_GENERAL_FAILURE";
		break;
	case (UINT16)eERROR_INVALID_PARAM:
		pStr = "eERROR_INVALID_PARAM";
		break;
	}
	return pStr;
}

tJBL_STATUS utils_append(UINT8* buf, UINT16* bufLen, UINT16 maxLen, const UINT8* srcBuf, UINT16 start, UINT16 count)
{
	if(buf == NULL || bufLen == NULL || srcBuf == NULL)
		return STATUS_FAILED;

	if( (count + *bufLen) <= maxLen)
	{
		memcpy(buf+ *bufLen, srcBuf+start, count);
		*bufLen += count;
	}
	else
	{
		return STATUS_FAILED;
	}

	return STATUS_SUCCESS;
}

/*
 * \brief scans a byte stream for the specified TAG and then returns
 * 			the VALUE of the TAG based on the length
 *
 * \param [in] tag for which one is searching
 * \param [in] input byte stream
 * \param [in] length of the input bytestream
 * \param [out] pointer to a UINT8 that will receive the lenght of the VALUE field found
 *
 * \return a pointer to the VALUE field of the specified TLV item
 */
UINT8* utils_getValue(UINT8 Tag, UINT8* rapdu, UINT16 rapduLen, UINT16 *returnCount)
{
    UINT8 i;
    UINT8* retData = NULL;

    *returnCount = 0;
	for(i = 0 ; i< rapduLen-1 ; i++ )
	{
		if(Tag == rapdu[i])
		{
			retData = &(rapdu[i+2]);
			*returnCount = rapdu[i+1];
			ALOGI("%s: regData - ",__func__);
			utils_LogByteArrayToHex(" retData: ", retData, *returnCount);
			break;
		}
	}

	return retData;
}

/*
 * \brief get the value field of the TAG 0x10 from inside 0xA5 and 0x6F
 * 		0x6F( ...0xA5 (... 0x10 (...)))
 *
 * 	\return will return 4 bytes, which represent the initialVC entry
 * 		and the maximum number of VC entries
 */
UINT8* utils_getVcCount(UINT8* recvData, UINT16 recvDataLen)
{
	UINT16 tmpLen;
	UINT8* tmp;

	// get the 6F TLV value field
	tmp = utils_getValue(0x6F, recvData, recvDataLen, &tmpLen);

	// skip over the next TLV
	tmpLen -= (tmp[1] + 2);
	tmp += (tmp[1]+2);

	// get the A5 TLV value field
	tmp = utils_getValue(0xA5, tmp, tmpLen, &tmpLen);

	// get the 10 TLV value field
	tmp = utils_getValue(0x10, tmp, tmpLen, &tmpLen);

	// NOTE: not returning the length (4 bytes) because we've defined that
	//		the length will always be 4.
	return tmp;
}
UINT16 utils_MakeCAPDU(UINT8 cla, UINT8 ins, UINT8 p1, UINT8 p2, const UINT8* pData, UINT16 dataLen, UINT8* pDest, UINT16 destSize)
{
	UINT16 size;

	if( (pDest == NULL) || (destSize < (5 + dataLen)))
	{
		return 0;
	}

	*pDest++ = cla;
	*pDest++ = ins;
	*pDest++ = p1;
	*pDest++ = p2;

	if (pData == NULL)
    {
		*pDest++ = 0;
    	size = 5;
    }
    else
    {
    	*pDest++ = dataLen;
    	memcpy(pDest, pData, dataLen);
        size = dataLen + 5;
    }

	return size;
}

UINT8 utils_adjustCLA(UINT8 cla, UINT8 lcnum)
{
    return ((cla & ~0x03) | (lcnum & 0x03));
}

UINT16 utils_getTlvLen(UINT8* tlv, UINT8 offset)
{
	UINT16 tlvLen = 0;

	if(tlv != NULL)
	{
		if(tlv[offset] == 0x82)
		{
			tlvLen = (tlv[offset+1]<<8) | tlv[offset+2];
		}
		else if(tlv[offset] == 0x81)
		{
			tlvLen = tlv[offset+1];
		}
		else if(tlv[offset] <= 0x7F)
		{
			tlvLen = tlv[offset];
		}
		else
		{
			/*Error case*/
		}
	}
	return tlvLen;
}

UINT16 utils_getTlvLenSkip(UINT8* tlv, UINT8 offset)
{
	UINT16 skipLen = 1;

	if(tlv != NULL)
	{
		if(tlv[offset] == 0x82)
		{
			skipLen = 3;
		}
		else if(tlv[offset] == 0x81)
		{
			skipLen = 2;
		}
		else if(tlv[offset] <= 0x7F)
		{
			skipLen = 1;
		}
		else
		{
			/*Error case*/
		}
	}
	return skipLen;
}

#endif/*MIFARE_OP_ENABLE*/
