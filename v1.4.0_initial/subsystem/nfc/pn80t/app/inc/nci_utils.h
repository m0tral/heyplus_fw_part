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

#include <stdlib.h>
#include <stdio.h>
#ifdef ANDROID
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nfa_api.h>
#include <nfa_rw_api.h>
#include "nfa_snep_api.h"
#include <nfa_ce_api.h>
#include <nfa_ee_api.h>
#include <ndef_utils.h>
#ifdef ANDROID
#include <semaphore.h>
#endif
#include <NfcAdapt.h>
#include <SeApi.h>

#ifdef __cplusplus
}
#endif

//#include <NfcAdaptation.h>

void NFA_SNEP_EventCode (tNFA_SNEP_EVT event);
void NFA_EE_EventCode (tNFA_EE_EVT event);
void NFA_ProtocolCode (tNFC_PROTOCOL protocol);
void NFA_DiscoveryType (tNFC_DISCOVERY_TYPE mode);
void NFA_BitRate (tNFC_BIT_RATE bitrate);
void NFA_IntfType (tNFC_INTF_TYPE type);
void NFA_DM_EventCode(UINT8 event);
void NFA_CN_EventCode(UINT8 event);
void NFA_NDEF_EventCode(tNFA_NDEF_EVT event);
void NFA_StatusCode(tNFA_STATUS status);
