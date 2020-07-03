/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 */
#ifndef NFCADAPT_H
#define NFCADAPT_H

#include "phNxpNciHal.h"
#include "phNxpNciHal_Adaptation.h"

#ifndef UINT32
typedef unsigned long   UINT32;
#endif
#include "nfc_target.h"
#include "nfc_hal_api.h"

#ifndef uint8_t
typedef unsigned char  uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short  uint16_t;
#endif

#if 0
#include <hardware/nfc.h>
#endif

typedef uint8_t nfc_event_t;
typedef uint8_t nfc_status_t;


void    NfcA_Initialize(void);
void    NfcA_Finalize(void);
tHAL_NFC_ENTRY* NfcA_GetHalEntryFuncs (void);
UINT8   NfcA_DownloadFirmware (void);
UINT8   NfcA_ForcedFwDnd (void);
UINT8   NfcA_PrepDownloadFirmware (void);
int    NfcA_PostDownloadFirmware(void);
void   NfcA_InitializeHalDeviceContext (void);
void   NfcA_HalCleanup();
void NfcA_SetIsFactoryResetRequired(BOOLEAN);
UINT16 NfcA_Set_China_Region_Configs(void* pSettings);
UINT16 NfcA__Reset_Nfcc_Fw(void);
void NfcA_Set_Uicc_Listen_Mask(UINT8 mask);
BOOLEAN NfcA_HalIsFirmwareUptodate(void);

#endif /* NFCADAPT_H */
