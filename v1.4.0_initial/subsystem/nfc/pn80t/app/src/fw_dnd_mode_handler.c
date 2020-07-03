
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
#include "SeApi.h"
#include "debug_settings.h"
#include "wearable_platform_int.h"
#include "SeApi_Int.h"

extern uint32_t wFwVerRsp; //its required to provide FW version to Companion device
extern uint16_t wFwVer; // This is set by the companion device.
extern phNxpNciProfile_Control_t nxpprofile_ctrl;//its required to provide bClkSrcVal and bClkFreqVal to Companion device
void phTml_DummyWriteCallback(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    phNxpNciHal_Sem_t *p_cb_data = (phNxpNciHal_Sem_t*) pContext;

    if (pInfo->wStatus == NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log(" write successful status = 0x%x", pInfo->wStatus);
    }
    else
    {
        phOsalNfc_Log(" write error status = 0x%x", pInfo->wStatus);
    }

    p_cb_data->status = pInfo->wStatus;

    if(SEM_POST(p_cb_data) != NFCSTATUS_SUCCESS)
    {
    	phOsalNfc_Log(" semaphore post failed in dummy write callback \n");
    }
}
void phTml_DummyReadCallback(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    phNxpNciHal_Sem_t *p_cb_data = (phNxpNciHal_Sem_t*) pContext;

    if (pInfo->wStatus == NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log(" read successful status = 0x%x", pInfo->wStatus);
    }
    else
    {
        phOsalNfc_Log(" read error status = 0x%x", pInfo->wStatus);
    }

    p_cb_data->status = pInfo->wStatus;
    p_cb_data->pContext = pInfo;

    if(SEM_POST(p_cb_data) != NFCSTATUS_SUCCESS)
    {
    	phOsalNfc_Log("semaphore post failed in dummy read callback  \n");
    }
}
NFCSTATUS phTmlWrite(uint8_t* buff, uint32_t length, unsigned short* pRespSize, uint8_t* pRespBuf)
{
    NFCSTATUS nfcStatus;
    phOsalNfc_Log("Start phTmlWrite \n");
    *pRespSize = 2;
    memset(pRespBuf, 0, *pRespSize);       // PRQA S 3200
    phNxpNciHal_Sem_t cb_data;
    /* Create the local semaphore */
    nfcStatus = phNxpNciHal_init_cb_data(&cb_data, NULL);
    if (nfcStatus != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log("phNxpNciHal_init_cb_data Create cb data failed");
        *pRespSize = 2;
        pRespBuf[0] = (uint8_t) ((nfcStatus >> 8) & 0xff);
        pRespBuf[1] = (uint8_t) (nfcStatus & 0xff);
        return nfcStatus;
    }

    nfcStatus = phTmlNfc_Write(buff, (UINT16)length, (pphTmlNfc_TransactCompletionCb_t)&phTml_DummyWriteCallback, (void *) &cb_data);
    if (nfcStatus != NFCSTATUS_PENDING)
    {
        phOsalNfc_Log("phTmlNfc_Write status error");
        goto cleanup;
    }
    //wait for callback
    nfcStatus = SEM_WAIT(cb_data);
    if (nfcStatus != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log("SEM_WAIT error");
        goto cleanup;
    }
    //nfcStatus = cb_data.status;
    cleanup:
    *pRespSize = 2;
    pRespBuf[0] = (uint8_t) ((nfcStatus >> 8) & 0xff);
    pRespBuf[1] = (uint8_t) (nfcStatus & 0xff);
    phNxpNciHal_cleanup_cb_data(&cb_data);
    phOsalNfc_Log("\nEnd  phTmlWrite  status 0x%x\n",nfcStatus);
    return nfcStatus;
}
NFCSTATUS phTmlRead(unsigned short* pRespSize, uint8_t* pRespBuf)
{
    NFCSTATUS nfcStatus;
    uint8_t tempbuff[260];
    phOsalNfc_Log("Start phTmlRead \n");
    *pRespSize = RESP_BUFF_SIZE;
    memset(pRespBuf, 0, *pRespSize);        // PRQA S 3200
    phNxpNciHal_Sem_t cb_data;
    /* Create the local semaphore */
    nfcStatus = phNxpNciHal_init_cb_data(&cb_data, NULL);
    if (nfcStatus != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log("phNxpNciHal_init_cb_data Create cb data failed");
        *pRespSize = 2;
        pRespBuf[0] = (uint8_t) ((nfcStatus >> 8) & 0xff);
        pRespBuf[1] = (uint8_t) (nfcStatus & 0xff);
        return nfcStatus;
    }

    nfcStatus = phTmlNfc_Read(tempbuff, sizeof(tempbuff), (pphTmlNfc_TransactCompletionCb_t)&phTml_DummyReadCallback, (void *) &cb_data);
    if (nfcStatus != NFCSTATUS_PENDING)
    {
        phOsalNfc_Log("phTmlNfc_Read failed error 0x%x",nfcStatus);
        goto cleanup;
    }

    //wait for callback
    nfcStatus = SEM_WAIT(cb_data);
    if (nfcStatus != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log("SEM_WAIT error");
        goto cleanup;
    }
    nfcStatus = cb_data.status;

    cleanup:
    if(nfcStatus != NFCSTATUS_SUCCESS)
    {
        *pRespSize = 2;
        pRespBuf[0] = (uint8_t) ((nfcStatus >> 8) & 0xff);
        pRespBuf[1] = (uint8_t) (nfcStatus & 0xff);
    }
    else
    {
        phTmlNfc_TransactInfo_t *pInfo = (phTmlNfc_TransactInfo_t*)cb_data.pContext;
        *pRespSize = pInfo->wLength + 4;//(4 bytes for storing msb and lsb of status and wLength )
        pRespBuf[0] = (uint8_t) ((pInfo->wStatus >> 8) & 0xff);//msb
        pRespBuf[1] = (uint8_t) (pInfo->wStatus & 0xff);//lsb
        memcpy(&pRespBuf[2], pInfo->pBuff,pInfo->wLength);             // PRQA S 3200
        pRespBuf[2 + pInfo->wLength] = (uint8_t) ((pInfo->wLength >> 8) & 0xff);//msb
        pRespBuf[3 + pInfo->wLength] = (uint8_t) (pInfo->wLength & 0xff);//lsb
    }
    phNxpNciHal_cleanup_cb_data(&cb_data);
    phOsalNfc_Log("\nEnd  phTmlRead  status 0x%x\n",nfcStatus);
    return nfcStatus;
}
void FwDndTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRspSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{
    tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
    NFCSTATUS nfcStatus = NFCSTATUS_FAILED;
    UINT8 fwRec = 0;
    bool transit_settings_present = true;
    phTransitSettings_t settings = {0};
    *pRespTagType = 0x00;
    switch(subType)
    {
        case FW_DND_PREP:
            phOsalNfc_Log("\n------------ Start FW_DND_PREP  -------------\n");
            *pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);                    // PRQA S 3200
            //prepare wearable for firmware download
            GeneralState |= FWDNLD_MASK;
            DeviceStatus.fwdnldstatus = FWDNLDSTATUS_INTERRUPTED;
            if(bBleInterrupted || DeviceStatus.initstatus == INITSTATUS_FAILED)
            {
                bBleInterrupted = FALSE;
                SeApi_Int_CleanUpStack();
            }
            status = SeApi_PrepFirmwareDownload();
            pRespBuf[0] = status;
            phOsalNfc_Log("\nSeApi_PrepFirmwareDownload returned 0x%x", status);
            if(status == SEAPI_STATUS_OK)
            {
            	if(phTmlNfc_ReadAbort() != NFCSTATUS_SUCCESS) {
            		phOsalNfc_Log("\n Read abort failed  \n");
            	}
            }
            else
                GeneralState &= ~FWDNLD_MASK;
            phOsalNfc_Log("\n ----------- END FW_DND_PREP -------------\n");
            break;
        case NFCC_MODESET:
        {
            uint32_t next = 0;
            uint32_t mode = 0;

            phOsalNfc_Log("\n------------ Start phTmlNfc_IoCtl  -------------\n");
            *pRspSize =2;
            memset(pRespBuf, 0, *pRspSize);            // PRQA S 3200

            mode |= (value[next++] << 24);
            mode |= (value[next++] << 16);
            mode |= (value[next++] << 8);
            mode |= value[next++];
            phOsalNfc_LogBle("0x%X ", mode);

            nfcStatus = phTmlNfc_IoCtl((phTmlNfc_ControlCode_t)mode);
            if(nfcStatus == NFCSTATUS_SUCCESS)
            {
                if(mode == phTmlNfc_e_EnableDownloadMode)
                    if(phTmlNfc_ReadAbort() != NFCSTATUS_SUCCESS)
                    {
                        phOsalNfc_Log("\n Read abort failed  \n");
                    }
            }
            phOsalNfc_Log("\nphTmlNfc_IoCtl returned 0x%x", nfcStatus);
            pRespBuf[0] = (uint8_t) ((nfcStatus >> 8) & 0xff);
            pRespBuf[1] = (uint8_t) (nfcStatus & 0xff);
            phOsalNfc_Log("\n ----------- END phTmlNfc_IoCtl -------------\n");
        }
            break;
        case READ_ABORT:
            *pRspSize = 2;
            memset(pRespBuf, 0, *pRspSize);          // PRQA S 3200
            nfcStatus = phTmlNfc_ReadAbort();
            pRespBuf[0] = (uint8_t) ((nfcStatus >> 8) & 0xff);
            pRespBuf[1] = (uint8_t) (nfcStatus & 0xff);
            break;
        case TML_WRITE_REQ:
            if( (nfcStatus = phTmlWrite(value, length, pRspSize, pRespBuf)) != NFCSTATUS_SUCCESS) {
                phOsalNfc_Log("\n TML write failed %x\n",nfcStatus);
            }
            break;
        case TML_READ_REQ:
            nfcStatus = phTmlRead(pRspSize, pRespBuf);
            if(nfcStatus != NFCSTATUS_SUCCESS) {
                phOsalNfc_Log("\n TML Read failed %x\n",nfcStatus);
            }
            break;
        case FW_DND_POST:
            phOsalNfc_Log("\n------------ Start POST DND Init  -------------\n");
            *pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);        // PRQA S 3200
            //retrieve recovery flag which is set to TRUE after writing dummy firmware
            fwRec = value[0];
            status = SeApi_PostFirmwareDownload();
            if(status == SEAPI_STATUS_OK)
            {
#if (ENABLE_SESSION_MGMT == TRUE)
                status = SeApi_Init(&transactionOccurred, &sessionCb);
#else
                status = SeApi_Init(&transactionOccurred, NULL);
#endif

                if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_UNEXPECTED_FW)
                {
                    SeApi_Int_CleanUpStack();
                    phOsalNfc_Log("\n SeApi_Init failed try again\n");
#if (ENABLE_SESSION_MGMT == TRUE)
                    status = SeApi_Init(&transactionOccurred, &sessionCb);
#else
                    status = SeApi_Init(&transactionOccurred, NULL);
#endif
                }
                if(fwRec == FALSE)
                {
                    if(SEAPI_STATUS_OK == status)
                    {
                        // post to download enable wired channel to get JCOP version.
                        status = SeApi_WiredEnable(TRUE);
                        if((SEAPI_STATUS_OK == status) || (SEAPI_STATUS_NO_ACTION_TAKEN == status))
                        {
                            *pRspSize = RESP_BUFF_SIZE;
                             memset(pRespBuf, 0, *pRspSize);           // PRQA S 3200
                             if (SeApi_WiredGetAtr(pRespBuf, pRspSize) == SEAPI_STATUS_OK)
                             {
                                 DeviceStatus.initstatus = INITSTATUS_COMPLETED;
                                 if(SeApi_WiredEnable(FALSE) != SEAPI_STATUS_OK)
                                 {
                                  //Wired mode enable failed
                                  phOsalNfc_Log("\n Disabling wired mode failed \n");
                                 }
                             }
                        }
                        *pRspSize = 1;
                        status = transitSettingRead((UINT8*)&settings, sizeof(settings));
                        if(status != SEAPI_STATUS_OK)
                        {
                            settings.cfg_blk_chk_enable = 1;
                            settings.rfSettingModified = 0;
                            transit_settings_present = false;
                        }
                        if(transit_settings_present == true)
                        {
                            status = SeApi_ApplyTransitSettings(&settings, FALSE);
                        }
                        else
                        {
                            //apply default settings
                            status = SeApi_ApplyTransitSettings(&settings, TRUE);
                        }
                    }
                    else
                    {
                        phOsalNfc_Log("\n SeApi_Init failed with 0x%x\n",status);
                        DeviceStatus.initstatus = INITSTATUS_FAILED;
                    }
                }
                else
                {
                    if(SEAPI_STATUS_OK == status)
                    {
                        DeviceStatus.initstatus = INITSTATUS_COMPLETED;
                    }
                    else
                    {
                        phOsalNfc_Log("\n SeApi_Init failed with 0x%x after dummy firmware\n",status);
                        DeviceStatus.initstatus = INITSTATUS_FAILED;
                    }
                }
                pRespBuf[0] = status;
            }
            GeneralState &= ~FWDNLD_MASK;
            DeviceStatus.fwdnldstatus = FWDNLDSTATUS_COMPLETED;
			if(GeneralState & NFCINIT_MASK)
            {
                GeneralState &= ~NFCINIT_MASK;
            }
            phOsalNfc_Log("\n ----------- END POST DND Init -------------\n");
            break;
        case GET_NFCC_DEVICE_INFO:
        {
            uint8_t respLen = 0;
            phOsalNfc_Log("\n------------ Start GET_DEVICE_INFO  -------------\n");
            GeneralState |= FWDNLD_MASK;
            *pRspSize = RESP_BUFF_SIZE;
            memset(pRespBuf, 0, *pRspSize);           // PRQA S 3200
            // get System clock source and freq read from Wearable
            phNxpNciHal_get_clk_freq_wrapper();
            phOsalNfc_LogBle("\nbClkSrcVal 0x%X ", nxpprofile_ctrl.bClkSrcVal);
            phOsalNfc_LogBle("\nbClkFreqVal 0x%X ", nxpprofile_ctrl.bClkFreqVal);
            pRespBuf[respLen++] = nxpprofile_ctrl.bClkSrcVal;
            pRespBuf[respLen++] = nxpprofile_ctrl.bClkFreqVal;

            // get current FW version of wearable device
            pRespBuf[respLen++] = (UINT8)((wFwVerRsp >> 24) & 0xFF);
            pRespBuf[respLen++] = (UINT8)((wFwVerRsp >> 16) & 0xFF);
            pRespBuf[respLen++] = (UINT8)((wFwVerRsp >> 8) & 0xFF);
            pRespBuf[respLen++] = (UINT8)(wFwVerRsp & 0xFF);

            wFwVer = (value[0] << 8) | value[1];

            phOsalNfc_Log("\nFrimware version sent by Companion  0x%x", wFwVer);
            nfcStatus = phNxpNciHal_CheckValidFwVersionWrapper();
            phOsalNfc_Log("\nphNxpNciHal_CheckValidFwVersionWrapper returned 0x%x", nfcStatus);
            pRespBuf[respLen++] = (uint8_t) ((nfcStatus >> 8) & 0xff);
            pRespBuf[respLen++] = (uint8_t) (nfcStatus & 0xff);
            *pRspSize = respLen;
            GeneralState &= ~FWDNLD_MASK;
            phOsalNfc_Log("\n ----------- END GET_DEVICE_INFO -------------\n");
        }
        break;

        default:
            phOsalNfc_LogBle("FwDndTlvHandler: Default case; Wrong Sub-Type %d Sent\n", subType);
            break;
    }
}
#endif
