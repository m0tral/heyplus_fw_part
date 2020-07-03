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
 * vcDescription.h
 *
 *  Created on: May 18, 2015
 *      Author: Robert Palmer
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VCDESCRIPTION_H
#define VCDESCRIPTION_H

#include "p61_data_types.h"
#include "tlv.h"

tJBL_STATUS vcDescriptionFile_CreateVc(UINT16 vcEntry, UINT8* pVCData, UINT16 vcDataLen, UINT8* pSHA, UINT8* pResp, UINT16 pRespBufSize, UINT16* pRespLen);

tJBL_STATUS vcDescriptionFile_CreateF8Vc(UINT16 vcEntry, UINT8* pSHA, UINT8* mfdfaid, UINT8 mfdfaidLen, UINT8* cltecap, UINT8 cltecapLen, UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen);

tJBL_STATUS vcDescriptionFile_CreatePersonalizeData(UINT16 vcEntry, const UINT8* persoData, UINT16 persoDataLen, UINT8 pSha[20], UINT8* pResp, UINT16 pRespBufSize, UINT16* pRespLen);

tJBL_STATUS vcDescriptionFile_CreateVCResp(UINT16 vcEntry, UINT8* pUid, UINT8 UidLen, UINT8* pResp, UINT16 pRespBufSize, UINT16* pRespLen);

tJBL_STATUS vcDescriptionFile_DeleteVc(UINT16 vcEntry, UINT8 pSHA[20], UINT8* pResp, UINT16 respBufSize, UINT16* pRespLen);

tJBL_STATUS vcDescriptionFile_AddandUpdateMDAC(UINT16 vcEntry, const UINT8* pVCData, UINT16 vcDataLen, UINT8 pSha[20], UINT8* pResp, UINT16 pRespBufSize, UINT16* pRespLen);

UINT16 vcDescriptionFile_CreateTLV(UINT16 tag, const UINT8* data, UINT16 dataLen, UINT8* dest, UINT16 destSize);

UINT16 vcDescriptionFile_GetVC(UINT8* dest, UINT16 destSize);

UINT16 vcDescriptionFile_GetVcListResp(t_tlvList *getStatusRspTlvs, UINT8* pResp, UINT16 pRespSize);

#endif /* VCDESCRIPTION_H */

#ifdef __cplusplus
}
#endif
