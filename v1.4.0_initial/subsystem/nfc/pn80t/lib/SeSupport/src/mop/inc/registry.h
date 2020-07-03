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
 * registry.h
 *
 *  Created on: May 18, 2015
 *      Author: Robert Palmer
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef REGISTRY_H_
#define REGISTRY_H_

#include "p61_data_types.h"
#include "channelMgr.h"

typedef struct registryEntry{
	UINT8	shaWalletName[20];
	char	walletName[32];

	// private -- do not modify
	UINT8	idx;
	struct registryEntry *NEXT;
}tRegistryEntry;

typedef struct
{
	UINT16 (*cbPreLoad)(UINT8*, UINT16); // buffer to read from, size of buffer
	void (*cbPostLoad)(UINT8*, UINT16);	// buffer read from, amount read
	void (*cbPreStore)(UINT8*, UINT16); // buffer we're writing to, amount to write
	void (*cbPostStore)(UINT8*, UINT16); // buffer we stored into, amount stored
}tRegistryCB;

#define hREGISTRY	void*

void
registry_setRawRegistry(UINT8 *rawRegistry, UINT16 size);

void registry_setCallbacks(tRegistryCB *callbacks);

tJBL_STATUS
registry_checkRegistryAvailable(void);

hREGISTRY
registry_Init(void);

void registry_DeInit(hREGISTRY *pReg);

//----
// get count of entries in the registry
//----
//----
// Get the number of entries in the registry
UINT8 registry_GetCount(hREGISTRY reg);
void registry_Clean(hREGISTRY reg);

//
tJBL_STATUS registry_getNextAvailableVCEntry(hREGISTRY reg, UINT16 initialVCEntry, UINT16 *pVCEntry);
hCHANNELMGR registry_GetChannelMgr(hREGISTRY reg);
tJBL_STATUS registry_addVcEntryToDeleteList(hREGISTRY pReg, UINT16 tempVc);

UINT16 registry_GetVcEntryByIdxInDeleteList(hREGISTRY pReg, UINT8 idx);

tJBL_STATUS registry_addEntry(hREGISTRY pReg, tRegistryEntry *pEntry);

void registry_removeAllEntries(hREGISTRY reg);

tJBL_STATUS registry_removeEntryByIdx(hREGISTRY reg, UINT8 idx);

tRegistryEntry* registry_GetVcEntry(hREGISTRY pReg, UINT8 entry);
tRegistryEntry* registry_getEntry(hREGISTRY pReg, UINT8 idx);

tRegistryEntry* registry_getLastEntry(hREGISTRY reg);

UINT16 registry_GetVcEntryByIdx(hREGISTRY reg, UINT8 idx);

UINT8* registry_getShaWalletNameByIdx(hREGISTRY reg, UINT8 idx);

tJBL_STATUS registry_setUidByIndex(hREGISTRY reg, UINT8 idx, UINT8* uid, UINT8 uidLen);

tJBL_STATUS registry_setMaxVcEntries(hREGISTRY reg, UINT16 entries);
UINT16 registry_getMaxVcEntries(hREGISTRY reg);

tJBL_STATUS registry_SetVcActivatedStatus(hREGISTRY reg, UINT8 idx, UINT8 flag);
tJBL_STATUS registry_GetVcActivatedStatus(hREGISTRY reg, UINT8 idx, UINT8 *flag);

tJBL_STATUS registry_setVcCreateStatus(hREGISTRY reg, UINT8 idx, UINT8 flag);
tJBL_STATUS registry_getVcCreateStatus(hREGISTRY reg, UINT8 idx, UINT8 *flag);

tJBL_STATUS registry_saveRegistry(hREGISTRY pReg);

tJBL_STATUS registry_LoadRegistry(hREGISTRY pReg);
void registry_Log(void);


#endif /* REGISTRY_H_ */

#ifdef __cplusplus
}
#endif
