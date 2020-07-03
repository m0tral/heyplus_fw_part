/*
 * Copyright (C) 2015 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * utils.h
 *
 *  Created on: May 18, 2015
 *      Author: Robert Palmer
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UTILS_H
#define UTILS_H

#include "p61_data_types.h"

void utils_LogByteArrayToHex(const char *label, const UINT8 *data, UINT16 dataLen);

UINT16 utils_GetSW(const UINT8 *buf, UINT16 bufLen);

// buf [out] - recieves exactly 20 bytes of SHA-1
// data [in] - input data to be hashed
// datalen [in] - length of data to be hashed
tJBL_STATUS utils_CreateSha(UINT8 *buf, UINT8* data, UINT8 dataLen);

tJBL_STATUS utils_CopyUid(UINT8* dest, UINT8 destMaxLen, UINT8 *src, UINT8 srcLen);

UINT16 utils_createStatusCode(UINT8* buf, UINT16 statusWord);
UINT16 utils_LenOfTLV(UINT8* tlv, UINT16 offset);
UINT16 utils_SkipLenTLV(UINT8* tlv, UINT16 offset);
UINT8* utils_FindNextAID(UINT8* byteArr, UINT16 byteArrLen, UINT16 offset, UINT16* aidLen);

char* utils_getStringStatusCode(UINT16 statusWord);

tJBL_STATUS utils_append(UINT8* buf, UINT16* bufLen, UINT16 maxLen, const UINT8* srcBuf, UINT16 start, UINT16 count);

UINT8* utils_getValue(UINT8 Tag, UINT8* rapdu, UINT16 rapduLen, UINT16 *returnCount);
UINT8* utils_getVcCount(UINT8* recvData, UINT16 recvDataLen);

UINT16 utils_MakeCAPDU(UINT8 cla, UINT8 ins, UINT8 p1, UINT8 p2, const UINT8* pData, UINT16 dataLen, UINT8* pDest, UINT16 destSize);

UINT8 utils_adjustCLA(UINT8 cla, UINT8 chnl_id);
UINT16 utils_getTlvLen(UINT8* tlv, UINT8 offset);
UINT16 utils_getTlvLenSkip(UINT8* tlv, UINT8 offset);

UINT16 utils_ntohs(UINT16 input);
UINT16 utils_htons(UINT16 input);

UINT16 utils_AsciiHexToBytes(const UINT8* data, UINT16 dataLen, UINT8* pOut, UINT16 outSize);

#endif /* UTILS_H */

#ifdef __cplusplus
}
#endif
