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
#define LOG_TAG 			"NXP_MOP_ChannelMgr"

#include "channelMgr.h"
#include "registry.h"
#include "utils.h"
#include "p61_data_types.h"
#if(MIFARE_OP_ENABLE == TRUE)
#ifndef COMPANION_DEVICE
#include "gki.h"
#include "OverrideLog.h"
#endif
typedef struct channelInfo{
	// channel ID stuff
	UINT8			channelId[10];
	UINT8			channelOpen[10];
	UINT8			channelCnt;
}tChannelMgr;

hCHANNELMGR channelMgr_Init(void)
{
	tChannelMgr *p;

	p = GKI_os_malloc(sizeof(tChannelMgr));
	if(p != NULL)
	{
		ALOGD("%s",__func__);
		memset(p->channelId, 0, 10);
		memset(p->channelOpen, 0, 10);
		p->channelCnt = 0;
	}

	return p;
}

void channelMgr_DeInit(hCHANNELMGR *reg)
{
	if(*reg != NULL)
	{
		ALOGD("%s",__func__);
		channelMgr_Clean(*reg);
		GKI_os_free(*reg);
		*reg = NULL;
	}
}

void channelMgr_Clean(hCHANNELMGR reg)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;

	if(pReg != NULL)
	{
		memset(pReg, 0, sizeof(tChannelMgr));
	}
}

tJBL_STATUS channelMgr_addOpenedChannelId(hCHANNELMGR reg, UINT8 id)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;

	// 10 channel IDs in an array.  Indicate whether it is open or closed
	//	keep track of the number of channelIDs
	if ((pReg != NULL) && (pReg->channelCnt < 10))
	{
		pReg->channelId[pReg->channelCnt] = id;
		pReg->channelOpen[pReg->channelCnt] = TRUE;
		pReg->channelCnt++;
		return STATUS_SUCCESS;
	}
	return STATUS_FAILED;
}

UINT8 channelMgr_getLastChannelId(hCHANNELMGR reg)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;

	// 10 channel IDs in an array.  Indicate whether it is open or closed
	//	keep track of the number of channelIDs
	if ((pReg != NULL))
	{
		return pReg->channelId[pReg->channelCnt-1];
	}

	return 0xFF;
}

UINT8 channelMgr_getChannelCount(hCHANNELMGR reg)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;

	// 10 channel IDs in an array.  Indicate whether it is open or closed
	//	keep track of the number of channelIDs
	if ((pReg != NULL))
	{
		return pReg->channelCnt;
	}

	return 0xFF;
}

UINT8 channelMgr_getIsOpened(hCHANNELMGR reg, UINT8 idx)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;

	// 10 channel IDs in an array.  Indicate whether it is open or closed
	//	keep track of the number of channelIDs
	if ((pReg != NULL) && idx < 10)
	{
		return pReg->channelOpen[idx];
	}

	return 0xFF;
}

UINT8 channelMgr_getChannelId(hCHANNELMGR reg, UINT8 idx)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;

	// 10 channel IDs in an array.  Indicate whether it is open or closed
	//	keep track of the number of channelIDs
	if ((pReg != NULL) && idx < 10)
	{
		return pReg->channelId[idx];
	}

	return 0xFF;
}

tJBL_STATUS channelMgr_closeChannel(hCHANNELMGR reg, UINT8 idx)
{
	tChannelMgr* pReg = (tChannelMgr *) reg;
	UINT8 nextFree = 0;
	int i;

	if ((pReg != NULL))
	{
		pReg->channelOpen[idx] = FALSE;
		pReg->channelId[idx] = 0;

		for(i=0;i<10;i++)
		{
			if(pReg->channelOpen[i] == TRUE)
				nextFree = i + 1;
		}

		pReg->channelCnt = nextFree;

		return STATUS_SUCCESS;
	}

	return STATUS_FAILED;
}

UINT16 channelMgr_getSize(void)
{
	return sizeof(tChannelMgr);
}

UINT16 channelMgr_save(hCHANNELMGR reg, UINT8* buf, UINT16 bufSize)
{
	UINT16 bytesWritten = 0;

	tChannelMgr *p = (tChannelMgr *) reg;

	if(buf != NULL && bufSize >= sizeof(tChannelMgr))
	{
		if(p != NULL)
		{
			memcpy(buf, p->channelId, 10);
			buf += 10; bytesWritten += 10;
			memcpy(buf, p->channelOpen, 10);
			buf += 10; bytesWritten += 10;
			*buf++ = p->channelCnt;
			bytesWritten++;
		}
		else
		{
			memset(buf, 0, 10);
			buf += 10; bytesWritten += 10;
			memset(buf, 0, 10);
			buf += 10; bytesWritten += 10;
			*buf++ = 0;
			bytesWritten++;
		}
	}

	return bytesWritten;
}

UINT16 channelMgr_load(hCHANNELMGR reg, UINT8* buf)
{
	tChannelMgr *p = (tChannelMgr *) reg;

	if(NULL == p)
		return 0;

	memcpy(p->channelId, buf, 10);
	buf += 10;
	memcpy(p->channelOpen, buf, 10);
	buf += 10;
	p->channelCnt = *buf++;

	return sizeof(tChannelMgr);
}

UINT16 channelMgr_Log(UINT8* buf)
{
	ALOGD("%s, ---- Channel Manager Data ----",__func__);
	utils_LogByteArrayToHex("     List of Channel IDs: ", buf, 10);
	buf += 10;
	utils_LogByteArrayToHex("     Status of Channel IDs: ", buf, 10);
	buf += 10;
	ALOGD("     %s, Number of stored Channel IDs: %d", __func__, *buf);

	return 21;
}
#endif/*MIFARE_OP_ENABLE*/
