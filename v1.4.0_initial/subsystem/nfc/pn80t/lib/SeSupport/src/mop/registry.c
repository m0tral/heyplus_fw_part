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
#define LOG_TAG 			"NXP_MOP_Registry"

#include "registry.h"
#include "channelMgr.h"
#include "utils.h"
#include "p61_data_types.h"
#if(MIFARE_OP_ENABLE == TRUE)
#ifndef COMPANION_DEVICE
#include "gki.h"
#include "OverrideLog.h"
#endif
#define kNumDeleteVCEntriesMax	(15)

typedef struct registry{
	// main registry stuff
	tRegistryEntry	*HEAD;
	tRegistryEntry	*TAIL;
	hCHANNELMGR		chMgr;
	UINT8			maxCount;
	UINT8			count;
}tRegistry;

//static UINT8 *gRawRegistry;
//static UINT16 gRawRegistrySize;
//static tRegistryCB	gCB;

//---------------------------------------
// Forward declarations - only used internally
//---------------------------------------
void registry_removeAllEntries(hREGISTRY pReg);


#if 0
//---------------------------------------
// Public APIs
//---------------------------------------
/**
 * \brief RawRegistry is the raw buffer into which the registry
 *        will be stored in FLASH or on disk.  It is the serialized
 *        version of the registry.
 *
 *        This must be configured FIRST before other registry functions
 *
 * \param rawRegistry is a pointer to a buffer that will store the
 *        serialized registry
 *
 */
void registry_setRawRegistry(UINT8 *rawRegistry, UINT16 size)
{

	gRawRegistry = rawRegistry;
	gRawRegistrySize = size;
}

void registry_setCallbacks(tRegistryCB *callbacks)
{
	gCB.cbPostLoad = callbacks->cbPostLoad;
	gCB.cbPostStore = callbacks->cbPostStore;
	gCB.cbPreLoad = callbacks->cbPreLoad;
	gCB.cbPreStore = callbacks->cbPreStore;
}

/**
 * \brief check to make sure that the RawRegistry has been configured
 *
 * \return STATUS_SUCCESS or STATUS_FAILED
 */
tJBL_STATUS  registry_checkRegistryAvailable(void)
{
	tJBL_STATUS status = STATUS_SUCCESS;

	if(NULL == gRawRegistry)//CID 10621
	{
		return STATUS_FAILED;
	}

	if((gRawRegistry[0] != 'M') || (gRawRegistry[1] != 'O') || (gRawRegistry[2] != 'P')  || (gRawRegistry[3] != '1') )
		status = STATUS_FAILED;

	ALOGD("%s: status = %s", __func__, ( (status==STATUS_SUCCESS) ? "STATUS_SUCCESS" : "STATUS_FAILED"));

	return status;
}
#endif
/**
 * \brief Initialize the dynamic, in memory, registry
 *
 *	\param reg is a pointer to a registry struct that will be managed
 *	\param maxVC is the maximum number of VirtualCards that will be
 *		   supported
 */
hREGISTRY registry_Init(void)
{
	tRegistry *reg;

	reg = GKI_os_malloc(sizeof(tRegistry));
	if(reg != NULL)
	{
		ALOGD("%s",__func__);
		reg->chMgr = channelMgr_Init();
		if(reg->chMgr != NULL)
		{
			reg->maxCount = 15;
			reg->count	= 0;
			reg->HEAD = NULL;
			reg->TAIL = NULL;
			return (hREGISTRY) reg;
		}
		else
		{
			GKI_os_free(reg);
		}
	}

	return NULL;
}

/**
 * \brief clean up the registry - deleteing all memory for it
 *
 *	\param pReg is a pointer to the pointer holding the registry struct
 */
void registry_DeInit(hREGISTRY *pReg)
{
	tRegistry *reg;

	reg = (tRegistry *) *pReg;
	if(reg != NULL)
	{
		ALOGD("%s",__func__);
		channelMgr_DeInit(&(reg->chMgr));
		registry_Clean((hREGISTRY) reg);
		GKI_os_free(reg);
		*pReg = NULL;
	}
}

void registry_Clean(hREGISTRY reg)
{
	tRegistry* pReg = (tRegistry *) reg;

	if(pReg != NULL)
	{
		registry_removeAllEntries(pReg);
		pReg->maxCount = 0;
		pReg->count	= 0;
	}
}
hCHANNELMGR registry_GetChannelMgr(hREGISTRY reg)
{
	tRegistry* pReg = (tRegistry *) reg;
	if(pReg != NULL)
	{
		return pReg->chMgr;
	}

	return NULL;
}
/**
 * \brief Return the number of entries in the registry
 *
 *	\param reg is a pointer to a registry struct that is being managed
 *
 *	\return The number of entries in the registry is returned
 */
UINT8 registry_GetCount(hREGISTRY reg)
{
	tRegistry* pReg = (tRegistry *) reg;
	if(pReg != NULL)
	{
		return pReg->count;
	}

	return 255;
}


/**
 * \brief Add a registry entry to the registry.  Calling this will give
 *        ownership of the memory to the Registry functions.  The caller
 *        should NOT delete the memory of registry entries after adding
 *        the entry to the registry
 *
 *	\param reg is a pointer to a registry struct that is being managed
 *	\param pEntry is the registry entry object being added to the registry
 *
 * \return STATUS_SUCCESS or STATUS_FAILED
 */
tJBL_STATUS registry_addEntry(hREGISTRY reg, tRegistryEntry *pEntry)
{
	tRegistry* pReg = (tRegistry *) reg;

	if((pReg != NULL) && (pEntry != NULL))
	{
		if(pReg->HEAD == NULL)
		{
			pReg->HEAD = pEntry;
			pReg->TAIL = pEntry;
		}
		else
		{
			(pReg->TAIL)->NEXT = pEntry;
			pReg->TAIL = pEntry;
		}

		pEntry->NEXT = NULL;
		pEntry->idx = pReg->count;
		pReg->count++;
		return STATUS_SUCCESS;
	}

	return STATUS_FAILED;
}

/**
 * \brief delete all of the entries in the registry and free all associated memory
 *
 *	\param reg is a pointer to a registry struct that is being managed
 *
 */
void registry_removeAllEntries(hREGISTRY reg)
{
	tRegistryEntry *p, *t;
	tRegistry* pReg = (tRegistry *) reg;

	if(pReg != NULL)
	{
		p = pReg->HEAD;
		while(p != NULL)
		{
			t = p->NEXT;
			memset(p, 0, sizeof(tRegistryEntry));
			GKI_os_free(p);
			p = t;
		};

		pReg->HEAD = NULL;
		pReg->TAIL = NULL;
		pReg->count = 0;
	}
}

tJBL_STATUS registry_removeEntryByIdx(hREGISTRY reg, UINT8 idx)
{
	tRegistryEntry *p, *prev;
	tRegistry* pReg = (tRegistry *) reg;
	UINT8 counter;
	tJBL_STATUS status = STATUS_FAILED;

	if(pReg != NULL)
	{
		p = pReg->HEAD;
		prev = NULL;
		counter = 0;
		while(p != NULL)
		{
			if(counter == idx)
			{
				// found item to remove
				if(prev == NULL)
				{
					// update head
					pReg->HEAD = p->NEXT;
					pReg->count -= 1;
					memset(p, 0, sizeof(tRegistryEntry));
					GKI_os_free(p);
					status = STATUS_SUCCESS;
					break;
				}
				else
				{
					// update middle item
					prev->NEXT = p->NEXT;
					pReg->count -= 1;
					memset(p, 0, sizeof(tRegistryEntry));
					GKI_os_free(p);
					status = STATUS_SUCCESS;
					break;
				}
			}

			prev = p;
			p = p->NEXT;
			counter++;

		}
	}

	return status;
}

/**
 * \brief return the registry entry specified by the index
 *
 *	\param reg is a pointer to a registry struct that is being managed
 *	\param idx is the index of the registry entry to return
 *
 * \return NULL on failure, otherwise, pointer to the registry entry.
 *         the memory for the registry entry should NOT be free'd by the caller
 */
tRegistryEntry* registry_getEntry(hREGISTRY reg, UINT8 idx)
{
	tRegistry* pReg = (tRegistry *) reg;
	tRegistryEntry* p;
	UINT8 cnt = 0;

	if((pReg != NULL) && (idx < pReg->count))
	{
		p = pReg->HEAD;
		while(p != NULL)
		{
			if(cnt == idx)
			{
				return p;
			}
			else
			{
				p = p->NEXT;
				cnt++;
			}
		};
	}
	return NULL;
}

/**
 * \brief return the last item in the registry
 *
 *	\param reg is a pointer to a registry struct that is being managed
 *
 * \return NULL on failure, otherwise, pointer to the registry entry.
 *         the memory for the registry entry should NOT be free'd by the caller
 */
tRegistryEntry* registry_getLastEntry(hREGISTRY reg)
{
	tRegistry* pReg = (tRegistry *) reg;

	if(pReg != NULL)
	{
		return pReg->TAIL;
	}
	return NULL;
}


UINT8* registry_getShaWalletNameByIdx(hREGISTRY reg, UINT8 idx)
{
	tRegistryEntry *re = registry_getEntry(reg, idx);

	if(NULL != re)
	{
		return re->shaWalletName;
	}

	return NULL;
}

tJBL_STATUS registry_setMaxVcEntries(hREGISTRY reg, UINT16 entries)
{
	tRegistry* pReg = (tRegistry *) reg;

	if ((pReg != NULL) && (entries <= 15))
	{
		pReg->maxCount = entries;
		return STATUS_SUCCESS;
	}
	return STATUS_FAILED;
}

UINT16 registry_getMaxVcEntries(hREGISTRY reg)
{
	tRegistry* pReg = (tRegistry *) reg;

	if ((pReg != NULL))
	{
		return pReg->maxCount;
	}

	return 0;
}
/*
tJBL_STATUS registry_saveRegistry(hREGISTRY pReg)
{
	UINT8 *pCur;
	tRegistry *p = (tRegistry *) pReg;
	tRegistryEntry *pRE;
	UINT16	dataSize;
	UINT16  bytesAvailable;
	UINT16  bytesWritten;
	UINT16  i;

	ALOGD("------- %s -------",__func__);
	if((gRawRegistry == NULL) || (gRawRegistrySize == 0))
		return STATUS_FAILED;

	if(p != NULL)
	{
		// 4 bytes: MOP1
		// 2 bytes: Endian Marker
		// 2 bytes: Data size
		// 21 bytes: Channel manager
		// 20 bytes: SHA of app name
		// 32 bytes: app name as null terminated string
		dataSize = 4 + 4 + channelMgr_getSize() + p->count * (20 + 32);
	}
	else
	{
		dataSize = 4 + 4 + channelMgr_getSize();
	}

	if(dataSize > gRawRegistrySize)
	{
		return STATUS_FAILED;
	}

	//----------------
	// notify caller we're about to write to buffer
	//----------------
	if(gCB.cbPreStore != NULL)
		gCB.cbPreStore(gRawRegistry, dataSize);

	if(gCB.cbPostStore == NULL)
	{
		ALOGD("%s -- PostStore callback is NULL registry cannot be saved",__func__);
		return STATUS_FAILED;
	}

	pCur = gRawRegistry;

	// --- Identifier and Endian Marker ---
	*pCur++ = 'M';
	*pCur++ = 'O';
	*pCur++ = 'P';
	*pCur++ = '1';
	*((UINT16 *) pCur) = 0x3234;
	pCur += 2;
	*((UINT16 *) pCur) = dataSize;
	pCur += 2;

	// --- Non Repeating Item - channelManager ---
	bytesAvailable = gRawRegistrySize - (pCur - gRawRegistry);
	bytesWritten = channelMgr_save(p->chMgr, pCur, bytesAvailable);
	if(bytesWritten == 0)
	{
		gCB.cbPostStore(gRawRegistry, 0);
		return STATUS_FAILED;
	}

	pCur += bytesWritten;

	if(pReg != NULL)
	{
		pRE = p->HEAD;
		if(pRE != NULL)
		{
			for(i=0; i<p->count; i++)
			{
				memcpy(pCur, pRE->shaWalletName, 20);
				pCur += 20;

				memcpy(pCur, pRE->walletName, 32);
				pCur += 32 * sizeof(UINT8);

				pRE = pRE->NEXT;
				if(pRE == NULL)
					break;
			}
		}
	}

	if(dataSize != ((UINT32) pCur - (UINT32) gRawRegistry))
	{
		// calculated size differs from written size, something is wrong
		ALOGD("%s: returning STATUS_FAILED", __func__);
		gCB.cbPostStore(gRawRegistry, 0);
		return STATUS_FAILED;
	}

	//----------------
	// notify caller we're finished writing to buffer
	//----------------
	gCB.cbPostStore(gRawRegistry, dataSize);

	ALOGD("SAVE SUCCESS");
	return STATUS_SUCCESS;
}


tJBL_STATUS registry_LoadRegistry(hREGISTRY pReg)
{
	tJBL_STATUS res;
	UINT8 *pCur = NULL;
	tRegistry *p;
	tRegistryEntry *pRE;
	UINT16	dataSize = 0;
	UINT16 i;
	UINT8 entryCount;
	UINT16 registryDataSize;
	UINT16 rawDataSize;
	UINT16 bytesRead = 0;

	ALOGD("------- %s -------",__func__);

	p = (tRegistry *) pReg;
	res = STATUS_FAILED;

	if((p == NULL) || (p->chMgr == NULL))
		goto loadBail;

	//----------------
	// notify caller we're about to read from buffer
	//	Caller should fill the buffer with data
	//	Tell the caller the buffer we're going to read
	//		from and how big that buffer is
	//----------------
	if(gCB.cbPreLoad == NULL)
	{
		ALOGD("%s -- PreLoad callback is NULL registry cannot be loaded",__func__);
		goto loadBail;
	}

	rawDataSize = gCB.cbPreLoad(gRawRegistry, gRawRegistrySize);

	if((gRawRegistry == NULL) || (gRawRegistrySize == 0) || (rawDataSize == 0))
		goto loadBail;

	pCur = gRawRegistry;

	// --- identifier ----
	if( (pCur[0] != 'M') || (pCur[1] != 'O') || (pCur[2] != 'P') )
	{
		goto loadBail;
	}
	else if (pCur[3] == '0')
	{
		ALOGD("%s -- registry file is for LTSM1.0, please delete registry file. You must start over for LTSM2.0",__func__);
		goto loadBail;
	}
	else if (pCur[3] != '1')
	{
		goto loadBail;
	}
	pCur += 4;

	// --- Endian Marker ---
	if(0x3234 != *((UINT16 *) pCur))
		goto loadBail;
	pCur += 2;

	// --- data size ----
	dataSize = *((UINT16 *) pCur);
	pCur += 2;
	if(gRawRegistrySize < dataSize || rawDataSize < dataSize)
		goto loadBail;

	// --- Non Repeating Item - channelManager ---
	bytesRead = channelMgr_load(p->chMgr, pCur);
	if(bytesRead != channelMgr_getSize())
		goto loadBail;

	pCur += bytesRead;

	// only the channelMgr was stored, stop here
	if( dataSize == (4 + 3 + channelMgr_getSize()) )
		goto loadBail;

	registry_Clean(p);

	registryDataSize = dataSize - (pCur - gRawRegistry);
	entryCount =  registryDataSize / (20 + 32);
	if( registryDataSize != (entryCount * (20 + 32)) )
	{
		ALOGD("%s -- File size is inconsistent, there is not an integer number of data entries",__func__);
		goto loadBail;
	}

	for(i=0; i < entryCount; i++)
	{
		pRE = GKI_os_malloc(sizeof(tRegistryEntry));

		if(pRE == NULL)
		{
			goto loadBail;
		}

		memcpy(pRE->shaWalletName, pCur, 20);
		pCur += 20;
		memcpy(pRE->walletName, pCur, 32);
		pCur += 32;

		registry_addEntry(p, pRE);
	}

loadBail:
	if(p != NULL)
	{
		if(dataSize != ((UINT32) pCur - (UINT32) gRawRegistry))
		{
			// calculated size differs from loaded size, something is wrong
			channelMgr_Clean(p->chMgr);
			registry_Clean(p);
		}
		else
		{
			res = STATUS_SUCCESS;
		}
	}

	//----------------
	// notify caller we're finished reading from buffer
	//	provide the buffer we read from and the amount of data we read
	//----------------
	if(res == STATUS_FAILED)
	{
		if(gCB.cbPostLoad != NULL)
			gCB.cbPostLoad(gRawRegistry, 0);
		ALOGD("------- %s ------- FAILED",__func__);
	}
	else
	{
		if(gCB.cbPostLoad != NULL)
			gCB.cbPostLoad(gRawRegistry, dataSize);
		ALOGD("------- %s ------- SUCCESS",__func__);
	}

	return res;
}

void registry_Log(void)
{
	UINT8 *pCur;
	UINT16	dataSize = 0;
	UINT16 i;
	UINT8 entryCount;
	UINT16 registryDataSize;
	UINT16 bytesRead;

	// File structure:
	//	4 bytes - A - (MOP1 or MOP0) file identifier (LTSM1.0 == MOP0, LTSM2.0 == MOP1)
	//	2 bytes - A - 24 big endian/ 42 little endian - endian marker
	//	2 bytes - B - total size of file, including file identifier, endian marker and size
	// ---- channel manager ----
	//	10 bytes - B - array[10] of channel IDs
	//  10 bytes - B - array[10] of status for channel IDs
	//  1 byte   - B - number of open channel IDs
	//  --- repeating ---
	//  20 bytes - B - SHA-1 of the application name
	//  32 bytes - A - name of app
	//
	ALOGD("------- Enter %s -------",__func__);

	//----------------
	// notify caller we're about to read from buffer
	//	Caller should fill the buffer with data
	//----------------

	ALOGD("%s: gRawRegistry = %X, gRawRegistrySize = %d", __func__,  (unsigned int) gRawRegistry, gRawRegistrySize);
	if((gRawRegistry == NULL) || (gRawRegistrySize == 0))
	{
		goto LogRegistryBail;
	}

	pCur = gRawRegistry;

	// --- identifier ----
	// --- identifier ----
	if( (pCur[0] != 'M') || (pCur[1] != 'O') || (pCur[2] != 'P') )
	{
		goto LogRegistryBail;
	}
	else if (pCur[3] == '0')
	{
		ALOGD("%s -- registry file is for LTSM1.0, please delete registry file. You must start over for LTSM2.0",__func__);
		goto LogRegistryBail;
	}
	else if (pCur[3] != '1')
	{
		goto LogRegistryBail;
	}
	ALOGD("%s, MOP found", __func__);
	pCur += 4;

	// --- Endian Marker ---
	if(0x3234 != *((UINT16 *) pCur))
		goto LogRegistryBail;
	ALOGD("%s, Endian Marker: %X", __func__, *((UINT16 *) pCur));
	pCur += 2;

	// --- data size ----
	dataSize = *((UINT16 *) pCur);
	pCur += 2;
	ALOGD("%s, DataSize: %d", __func__, dataSize);
	if(gRawRegistrySize < dataSize)
		goto LogRegistryBail;

	// --- Non Repeating Item - channelManager ---
	bytesRead = channelMgr_Log(pCur);
	pCur += bytesRead;

	// only the channelMgr was stored, stop here
	if( dataSize == (3 + 4 + bytesRead) )
		goto LogRegistryBail;

	registryDataSize = dataSize - (pCur - gRawRegistry);
	entryCount =  registryDataSize / (20 + 32);
	if( registryDataSize != (entryCount * (20 + 32)) )
	{
		ALOGD("%s -- data size is inconsistent, there is not an integer number of data entries",__func__);
		goto LogRegistryBail;
	}

	for(i=0; i < entryCount; i++)
	{
		ALOGD("%s, ---- VC Entry[%d] ----",__func__, i);

		utils_LogByteArrayToHex("     SHA of WalletName: ", pCur, 20);
		pCur += 20;
		ALOGD("     %s, WalletName: %s", __func__, pCur);
		pCur += 32;
	}

LogRegistryBail:
	ALOGD("------- EXIT %s -------",__func__);
	return;
}
*/
#endif/*MIFARE_OP_ENABLE*/
