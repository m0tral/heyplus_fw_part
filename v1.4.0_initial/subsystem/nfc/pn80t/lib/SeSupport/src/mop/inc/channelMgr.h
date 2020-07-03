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
 * channelMgr.h
 *
 *  Created on: May 18, 2015
 *      Author: Robert Palmer
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CHANNELMGR_H_
#define CHANNELMGR_H_

#include "p61_data_types.h"

#define hCHANNELMGR	void*

UINT16 channelMgr_getSize(void);

hCHANNELMGR channelMgr_Init(void);

void channelMgr_DeInit(hCHANNELMGR *reg);

void channelMgr_Clean(hCHANNELMGR reg);

tJBL_STATUS channelMgr_addOpenedChannelId(hCHANNELMGR pChMgr, UINT8 id);

UINT8 channelMgr_getLastChannelId(hCHANNELMGR pChMgr);

UINT8 channelMgr_getChannelCount(hCHANNELMGR pChMgr);

UINT8 channelMgr_getIsOpened(hCHANNELMGR pChMgr, UINT8 idx);

UINT8 channelMgr_getChannelId(hCHANNELMGR pChMgr, UINT8 idx);

tJBL_STATUS channelMgr_closeChannel(hCHANNELMGR reg, UINT8 idx);

UINT16 channelMgr_save(hCHANNELMGR reg, UINT8* buf, UINT16 bufSize);

UINT16 channelMgr_load(hCHANNELMGR reg, UINT8* buf);

UINT16 channelMgr_Log(UINT8* buf);

#endif /* CHANNELMGR_H_ */

#ifdef __cplusplus
}
#endif
