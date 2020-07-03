

/*
* Copyright (c), NXP Semiconductors
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
* NXP reserves the right to make changes without notice at any time.
*
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
* particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
* arising from its use.
*/

#include "nci_utils.h"
#ifndef NFC_UNIT_TEST
#include "wearable_tlv_types.h"
#include <phOsalNfc_Log.h>
#include "SeApi_Int.h"
#include "debug_settings.h"
#include "wearable_platform_int.h"

static uint8_t ValueBuff [10];

void notifyDeviceStatus(void* data)
{
    phDevice_Status_t* pDeviceStatus = (phDevice_Status_t*) data;
    phOsalNfc_Log("------Notify Companion device: Firmware Download status= %d, Init Status= %d ------\n",
            pDeviceStatus->fwdnldstatus, pDeviceStatus->initstatus);
    BaseType_t xSemStatus;
    static xTLVMsg tlv;
    BaseType_t xStatus;
    tlv.type = GENERAL_MODE;// update type
    tlv.length = 3;
    tlv.value = ValueBuff;
    tlv.value[0] = DEVICE_STATUS_NTFN;// update subtype
    tlv.value[1] = pDeviceStatus->fwdnldstatus;
    tlv.value[2] = pDeviceStatus->initstatus;
    // Send the TLV to the NFC Stack
    xSemStatus = xSemaphoreTake(nfcQueueLock, portMAX_DELAY);
    if(xSemStatus == pdTRUE) {
        xStatus = xQueueSendToBack( xQueueNFC, &tlv, 0 );
        xSemStatus = xSemaphoreGive(nfcQueueLock);
        if (( xStatus != pdPASS ) || (xSemStatus != pdPASS ))
        {
            phOsalNfc_LogBle( "Could not send to the NFC queue.\r\n" );
        }
    }
}

void copyToUint8Buffer(uint32_t value, uint8_t numOfBytes, uint8_t *pRespBuf, uint16_t *pRespSize)
{
    int i;

    for(i = 0; i < numOfBytes; i++)
    {
        pRespBuf[*pRespSize] = (UINT8)((value >> ((8*(numOfBytes-1)) - (8*i))) & 0xFF);
        (*pRespSize)++;
    }
}
void GeneralModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{
    tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
    phStackCapabilities_t stackCapabilities = {0};
    phTransitSettings_t settings = {0};
    *pRespTagType = 0x00;
    *pRespSize = RESP_BUFF_SIZE;
    memset(pRespBuf, 0, *pRespSize);       // PRQA S 3200

    switch(subType)
    {
        case GETSTACK_CAP:
            phOsalNfc_LogCrit("\n ----------- START: SeApi Get Stack Capabilities -------------\n");
            status = SeApi_GetStackCapabilities(&stackCapabilities);
            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("\nSeApi_GetStackCapabilities: SUCCESS\n");
                phOsalNfc_LogBle("SDK Version: 0x%06X\n", stackCapabilities.sdk_ver);
                phOsalNfc_LogBle("MiddleWare Version: 0x%06X\n", stackCapabilities.middleware_ver);
                phOsalNfc_LogBle("Firmware Version: 0x%06X\n", stackCapabilities.firmware_ver);
                phOsalNfc_LogBle("NCI Version: 0x0%X\n", stackCapabilities.nci_ver);
                phOsalNfc_LogBle("eSE Version: 0x%X\n", stackCapabilities.ese_ver);
                phOsalNfc_LogBle("JCOP Version: 0x%04X\n", stackCapabilities.jcop_ver);
                phOsalNfc_LogBle("LS Version:0x%06X\n", stackCapabilities.ls_ver);
                phOsalNfc_LogBle("LTSM Version: 0x%X\n", stackCapabilities.ltsm_ver);
                phOsalNfc_LogBle("Chunk Size CmdBufSize 0x%X RspBufSize 0x%X\n", stackCapabilities.chunk_sizes.CmdBufSize,stackCapabilities.chunk_sizes.RspBufSize);

                *pRespSize = 0;
                phOsalNfc_LogBle("Copy the Stack Capabilities in Resp Buffer and Send it to Companion Device\n");
                copyToUint8Buffer(stackCapabilities.sdk_ver, sizeof(stackCapabilities.sdk_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.middleware_ver, sizeof(stackCapabilities.middleware_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.firmware_ver, sizeof(stackCapabilities.firmware_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.nci_ver, sizeof(stackCapabilities.nci_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.ese_ver, sizeof(stackCapabilities.ese_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.jcop_ver, sizeof(stackCapabilities.jcop_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.ls_ver, sizeof(stackCapabilities.ls_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.ltsm_ver, sizeof(stackCapabilities.ltsm_ver), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.chunk_sizes.CmdBufSize, sizeof(stackCapabilities.chunk_sizes.CmdBufSize), pRespBuf, pRespSize);
                copyToUint8Buffer(stackCapabilities.chunk_sizes.RspBufSize, sizeof(stackCapabilities.chunk_sizes.RspBufSize), pRespBuf, pRespSize);
            }
            else
            {
                phOsalNfc_LogBle("\nSeApi_GetStackCapabilities returns: 0x%02X\n", status);
            }
            pRespBuf[(*pRespSize)++] = status;// to send status to companion device
            phOsalNfc_LogCrit("\n ----------- END: SeApi Get Stack Capabilities -------------\n");
            break;

        case LOOPBACK_OVER_BLE:
            phOsalNfc_LogCrit("\n ----------- START: Loop Back Data Exchange Over BLE -------------\n");
            *pRespSize = (UINT16)length;
            memcpy(pRespBuf, value, length);           // PRQA S 3200
            pRespBuf[(*pRespSize)++] = SEAPI_STATUS_OK;// to send status to companion device
            phOsalNfc_LogCrit("\n ----------- END: Loop Back Data Exchange Over BLE -------------\n");
            break;

        case DEVICE_STATUS_NTFN:
            *pRespSize = 3;
            *pRespTagType = DEVICE_STATUS_NTFN;
            memset(pRespBuf, 0, *pRespSize);       // PRQA S 3200
            pRespBuf[0] = DEVICE_STATUS_NTFN;
            memcpy(&pRespBuf[1], value, length);           // PRQA S 3200
            break;
        case SET_TRANSIT_SETTINGS:
            *pRespSize = 1;
            status = SEAPI_STATUS_OK;
            settings.ese_listen_tech_mask = value[0];
            settings.cfg_blk_chk_enable = value[1];
            settings.cityType = value[2];
            settings.rfSettingModified = value[3];

            if(settings.rfSettingModified)
            {
                //apply customized rf settings
                if(phNxpNciHal_set_city_specific_rf_configs(settings.cityType) != NFCSTATUS_OK)
                {
                    status = SEAPI_STATUS_FAILED;
                }
            }
            if(status == SEAPI_STATUS_OK)
            {
                status = SeApi_ApplyTransitSettings(&settings, FALSE);
                if(status == SEAPI_STATUS_OK)
                {
                    // write settings in to flash
                    status = transitSettingWrite((UINT8*)&settings, sizeof(settings));
                    if(status == SEAPI_STATUS_OK)
                        phNxpNciHal_set_transit_settings(&settings);
                }
            }
            pRespBuf[0] = status;// to indicate status to companion device
            break;
        case GET_TRANSIT_SETTINGS:
            status = SEAPI_STATUS_OK;
            status = transitSettingRead((UINT8*)&settings, sizeof(settings));
            if(status != SEAPI_STATUS_OK)
            {
                //default settings
                settings.ese_listen_tech_mask = SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B;
                settings.cfg_blk_chk_enable = 1;
                settings.rfSettingModified = 0;
                settings.cityType = 0;
            }
            *pRespSize = 0;
            copyToUint8Buffer(settings.ese_listen_tech_mask, sizeof(settings.ese_listen_tech_mask), pRespBuf, pRespSize);
            copyToUint8Buffer(settings.cfg_blk_chk_enable, sizeof(settings.cfg_blk_chk_enable), pRespBuf, pRespSize);
            copyToUint8Buffer(settings.cityType, sizeof(settings.cityType), pRespBuf, pRespSize);
            copyToUint8Buffer(settings.rfSettingModified, sizeof(settings.rfSettingModified), pRespBuf, pRespSize);
            break;
        case RESTART_BLE_NFC_BRIDGE:
            /* This is handled in Ble Nfc Bridge after sending a response to the companion */
            status = SEAPI_STATUS_OK;
            pRespBuf[0] = status;
            *pRespSize = 1;
            break;
        default:
            phOsalNfc_LogCrit("\nGeneralModeTlvHandler: Wrong subType received\n");
    }
}
#endif
