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
 * mop.h
 *
 *  Created on: May 18, 2015
 *      Author: Robert Palmer
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MOP_H_
#define MOP_H_

#include "IChannel.h"
#include "registry.h"

#define MAX_APP_NAME	32

#define kVCTYPE_NONE	0

tJBL_STATUS MOP_setCurrentPackageInfo(const char *packageName);
tJBL_STATUS MOP_Init(IChannel_t *channel);
void MOP_DeInit(void);
UINT16 MOP_CreateVirtualCard(const UINT8 *vcData, UINT16 vcDataLen, const UINT8 *persoData, UINT16 persoDataLen, UINT8 *pResp, UINT16 *pRespLen);
UINT16 MOP_DeleteVirtualCard(UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen);
UINT16 MOP_ActivateVirtualCard(UINT16 vcEntry, UINT8 mode, UINT8 concurrentActivationMode, UINT8 *pResp, UINT16 *pRespLen);
UINT16 MOP_AddAndUpdateMdac(UINT16 vcEntry, const UINT8* vcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen);
UINT16 MOP_ReadMifareData(UINT16 vcEntry, const UINT8* vcData, UINT16 vcDataLen, UINT8 *pResp, UINT16 *pRespLen);
UINT16 MOP_GetVcList(UINT8 *pVcList, UINT16 *pVcListLen);
UINT16 MOP_GetVirtualCardStatus(UINT16 vcEntry, UINT8 *pResp, UINT16 *pRespLen);
#endif /* MOP_H_ */

#ifdef __cplusplus
}
#endif
