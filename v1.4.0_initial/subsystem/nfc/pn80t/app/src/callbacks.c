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



#include <WearableCoreSDK_BuildConfig.h>
#include <phNfcTypes.h>
#include "SeApi.h"
#include "UserData_Storage.h"
#include <phOsalNfc_Log.h>
#include "nci_utils.h"
#ifndef NFC_UNIT_TEST
#include "FreeRTOS.h"
#include "semphr.h"
#include "wearable_tlv_types.h"
#include <ble_handler.h>
#include "wearable_platform_int.h"
#include "ry_nfc.h"
#include "scheduler.h"
#include "ryos.h"

static uint8_t seEventBuff [10];
static uint8_t seTransEventBuff[25];
#endif
void connectivityCallback(void* data, UINT8 event)
{
    phOsalNfc_Log("--------------------- connectivityCallback Start ------------------------\n");
    phOsalNfc_Log("--------------------- connectivityCallback End ------------------------\n");
}

u8_t nfc_status[4];
u8_t g_test_nfc_id[11];
void activationStatusCallback(void* data, UINT8 event)
{
    phOsalNfc_Log("--------------------- activationStatusCallback Start ------------------------\n");
    //ry_data_dump(data, 50, ' ');
    u8_t* p = (u8_t*)data;
   
    switch(event)
        {
            case NFA_ACTIVATED_EVT:
                phOsalNfc_Log("RF_INTF_ACTIVATED_NTF \n");
                if ((*(p+6) == 0) && (*(p+7) == 0) && (*(p+8) == 0) && (*(p+9) == 0)) {
                    phOsalNfc_Log("ID All Zero. Jump \n");
                    break;
                }
                
                tNFA_CONN_EVT_DATA* evt_data = data;
                                
                u8_t mode = evt_data->activated.activate_ntf.rf_tech_param.mode;
                u8_t len = 4;
                switch(mode) {
                    case NFC_DISCOVERY_TYPE_POLL_A:
                    case NFC_DISCOVERY_TYPE_POLL_A_ACTIVE:
                    case NFC_DISCOVERY_TYPE_LISTEN_A:
                        len = evt_data->activated.activate_ntf.rf_tech_param.param.pa.nfcid1_len;
                        g_test_nfc_id[0] = len;
                        ry_memcpy(g_test_nfc_id+1, evt_data->activated.activate_ntf.rf_tech_param.param.pa.nfcid1, len);
                        break;
                    case NFC_DISCOVERY_TYPE_POLL_B:
                    case NFC_DISCOVERY_TYPE_LISTEN_B:
                        len = 4;
                        g_test_nfc_id[0] = len;
                        ry_memcpy(g_test_nfc_id+1, evt_data->activated.activate_ntf.rf_tech_param.param.pb.nfcid0, len);
                        break;
                    default:
                        g_test_nfc_id[0] = len;
                        ry_memcpy(g_test_nfc_id+1, p+6, len);
                        break;
                }
                
                nfc_status[0] = evt_data->activated.activate_ntf.protocol;
                nfc_status[1] = evt_data->activated.activate_ntf.rf_disc_id;
                nfc_status[2] = evt_data->activated.activate_ntf.data_mode;
                nfc_status[3] = evt_data->activated.activate_ntf.rf_tech_param.mode;                
                
                ry_sched_msg_send(IPC_MSG_TYPE_NFC_READER_DETECT_EVT, 4, p+6);
                break;
            case NFA_DEACTIVATED_EVT:
                 phOsalNfc_Log("RF_DEACTIVATE_NTF \n");
                break;
            default:
                phOsalNfc_Log("Unexpected Event %d Received\n", event);
                break;
        }

    phOsalNfc_Log("--------------------- activationStatusCallback End ------------------------\n");
}
void rfFieldStatusCallback(void* data, UINT8 event)
{
    tNFA_DM_CBACK_DATA* pRfStatus = (tNFA_DM_CBACK_DATA*) data;
    phOsalNfc_Log("--------------------- rfFieldStatusCallback Start ------------------------\n");
    phOsalNfc_Log("RF Operation status 0x%x, RF Field Status0x%x\n", pRfStatus->rf_field.status,pRfStatus->rf_field.rf_field_status );
    phOsalNfc_Log("--------------------- rfFieldStatusCallback End ------------------------\n");
}
// tSeApi_TransCallback
void transactionOccurred(UINT8* pAID, UINT8 aidLen, UINT8* pData, UINT8 dataLen, eSEOption src)
{
    int i;
    phOsalNfc_Log("--------------------- TRANSACTION RECEIVED ------------------------\n");
    if(SE_ESE == src)
    {
        phOsalNfc_Log("SOURCE: eSE\n");
    }
    else if(SE_UICC == src)
    {
        phOsalNfc_Log("SOURCE: UICC\n");
    }
    else
    {
        phOsalNfc_Log("SOURCE: Unknown\n");
    }

#if 0
    phOsalNfc_Log("AID length: %d, AID: ", aidLen);
    for(i=0; i<aidLen; i++)
    {
        phOsalNfc_Log("0x%02X ", pAID[i]);
    }
    phOsalNfc_Log("\n");
#endif
    LOG_DEBUG("AID length: %d\n, AID: ");
    ry_data_dump(pAID, aidLen, ' ');

#if 0
    phOsalNfc_Log("Data length: %d, Data: ", dataLen);
    for(i=0; i<dataLen; i++)
    {
        phOsalNfc_Log("0x%02X ", pData[i]);
    }
    phOsalNfc_Log("\n");
#endif
    LOG_DEBUG("Data length: %d\n, Data: ");
    ry_data_dump(pData, dataLen, ' ');
}

#if (ENABLE_SESSION_MGMT == TRUE)
INT32 sessionRead(UINT8* data, UINT16 numBytes)
{
    const Userdata_t *pBootData;
    pBootData = readUserdata(USERDATA_TYPE_SESSION_DETAILS);
    if(pBootData != NULL && (pBootData->header.length - BOOT_INFO_SIZE) <= numBytes)
    {
        memcpy(data, (pBootData->payload+BOOT_INFO_SIZE), numBytes);               // PRQA S 3200
    }
    else
    {
        numBytes = 0;
    }
    phOsalNfc_Log("---------------------------------\n\n");
    return numBytes;
}

INT32 sessionWrite(const UINT8* data, UINT16 numBytes)
{
    INT32 ret = -1;
    ret = writeUserdata(USERDATA_TYPE_SESSION_DETAILS, data, numBytes);
    if(USERDATA_ERROR_NOERROR != ret)
        return -1;
    phOsalNfc_Log("---------------------------------\n\n");
    return numBytes;
}
#endif

UINT16 transitSettingRead(UINT8* data, UINT16 numBytes)
{
    const Userdata_t *pTransitSettings;
    UINT16 status = SEAPI_STATUS_FAILED;
    pTransitSettings = readUserdata(USERDATA_TYPE_TRANSIT_SETTINGS);
    if(pTransitSettings != NULL && (pTransitSettings->header.length <= numBytes))
    {
        memcpy(data, pTransitSettings->payload, pTransitSettings->header.length);
        status = SEAPI_STATUS_OK;
    }
    return status;
}

UINT16 transitSettingWrite(const UINT8* data, UINT16 numBytes)
{
    UINT16 status = SEAPI_STATUS_FAILED;
    phOsalNfc_Log("----------transitSettingWrite Start--------\n\n");
    if(writeUserdata(USERDATA_TYPE_TRANSIT_SETTINGS, data, numBytes) == USERDATA_ERROR_NOERROR)
        status = SEAPI_STATUS_OK;

    phOsalNfc_Log("----------transitSettingWrite End status %d \n\n",status);
    return status;
}
