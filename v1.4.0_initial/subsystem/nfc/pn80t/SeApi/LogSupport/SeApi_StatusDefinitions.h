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
/**
 * \file  SeApi_StatusDefinitions.h
 * \brief Helper functions to return string descriptions of various
 *        numeric identifiers
 */

#ifndef SEAPI_STATUSDEFINITIONS_H
#define SEAPI_STATUSDEFINITIONS_H

#include "NfcAdapt.h"
#include "nfa_hci_api.h"

//---------------------------------------------------------------------
// Utility functions to convert numeric codes into readable strings
// for logging purposes
//---------------------------------------------------------------------

// output a text description of the activation event
void process_NFA_ACTIVATE_EVT (tNFA_CONN_EVT_DATA *p_data);

char * getString_NFA_CN_EventCode(UINT8 event);
char * getString_NFA_StatusCode(tNFA_STATUS status);
char * getString_NFA_DM_EventCode(UINT8 event);
char * getString_NFA_ProtocolCode (tNFC_PROTOCOL protocol);
char * getString_NFA_DiscoveryType (tNFC_DISCOVERY_TYPE mode);
char * getString_NFA_BitRate (tNFC_BIT_RATE bitrate);
char * getString_NFA_IntfType (tNFC_INTF_TYPE type);
char * getString_NFA_EE_EventCode (tNFA_EE_EVT event);
char * getString_NFA_HCI_EventCode(tNFA_HCI_EVT xevent);
char * getString_NFA_Buffer(UINT8 len, UINT8 *pData);
char * getString_NFA_Technology (tNFC_RF_TECH protocol);
char * getString_NFA_Trigger (tNFA_EE_TRIGGER trigger);

#endif
