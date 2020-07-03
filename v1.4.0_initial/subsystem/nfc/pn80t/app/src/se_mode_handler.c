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
#include "FreeRTOS.h"
#include "semphr.h"
#include "wearable_tlv_types.h"
#include <phOsalNfc_Log.h>
#include "SeApi.h"
#include "SeApi_Int.h"
#include <ble_handler.h>
#include "wearable_platform_int.h"
//#include "lpc_res_mgr.h"
#include "scheduler.h"

BOOLEAN  bComSubForSeNotfn = FALSE;

void unsubscribeCompanionForSeEvents(void)
{
    bComSubForSeNotfn = FALSE;
}

/*
 * Will send messages to BLE_NFC_Bridge task to
 * disable wired mode and do ese reset.
 * This is required in case of abrupt disruption of BLE
 * connection.
 */
void disableWiredandSEreset(void)
{
    static uint8_t tmpBuff[4]; //buffer used to hold tlv values. During dequeing the values should be intact.
    BaseType_t xStatus;
    BaseType_t xSemStatus;
    static xTLVMsg tlv;
    static xTLVMsg tlv_reset;
    tlv.type = SE_MODE;
    tlv.length = 2;
    tlv.value = tmpBuff;
    tlv.value[0] = WIREDMODE_ENABLE;
    tlv.value[1] = FALSE;

     
    if(xSemaphoreTake(nfcQueueLock, portMAX_DELAY) == pdTRUE) {
		xStatus = xQueueSendToBack( xQueueNFC, &tlv, 0 );
		xSemStatus = xSemaphoreGive(nfcQueueLock);
		if(( xStatus != pdPASS ) || (xSemStatus != pdPASS ))
		{
			phOsalNfc_LogBle("ERROR in disableWiredandSEreset.\r\n" );
		}
    }

    tlv_reset.type = SE_MODE;
    tlv_reset.length = 2;
    tlv_reset.value = &tmpBuff[2];
    if(GeneralState & JCOPDNLD_MASK)
        tlv_reset.value[0] = JCOP_SERESET;
    else
        tlv_reset.value[0] = WIREDMODE_SERESET;

    xSemStatus = xSemaphoreTake(nfcQueueLock, portMAX_DELAY);
    if(xSemStatus == pdTRUE) {
		xStatus = xQueueSendToBack( xQueueNFC, &tlv_reset, 0 );
		xSemStatus = xSemaphoreGive(nfcQueueLock);
		if(( xStatus != pdPASS ) || (xSemStatus != pdPASS ))
		{
			phOsalNfc_LogBle("ERROR in disableWiredandSEreset..\r\n");
		}
    }
}

void SeModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRspSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{
    tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
    *pRespTagType = 0x00;
    int i;
    switch(subType)
    {
        case WIRED_TRANSCEIVE:
            phOsalNfc_LogCrit("\n\r--- Send APDU to the SE on wired mode ---\n");

            *pRspSize = 0;
            memset(pRespBuf, 0, RESP_BUFF_SIZE);   //PRQA S 3200

            //passing RESP_BUFF_SIZE-1 as Maximum size of buffer so that in the pRspSize will always be less than or
            //equal to RESP_BUFF_SIZE-1. With this buffer overrun of pRespBuf will be avoided at the time of setting
            //status of operation.

            status = SeApi_WiredTransceive  (value, (UINT16)length, pRespBuf, (RESP_BUFF_SIZE-1), pRspSize, 10000);
            if(status == SEAPI_STATUS_OK)
            {
                //phOsalNfc_LogBle("\nSeApi_WiredTransceive  Successful: ");
            }
            else
            {
                phOsalNfc_LogBle("\nSeApi_WiredTransceive  Failed: ");
                *pRspSize = 0; // reset respSize to zero in case of failure.
            }
            pRespBuf[(*pRspSize)++] = status;/*To indicate status to the companion device*/
            phOsalNfc_LogCrit("\n\r--- END Send APDU to the SE on wired mode ---\n");

            break;
        case WIREDMODE_ENABLE:
#ifdef DYNAMIC_FREQUENCY_SWITCH
        	//if (value[0])
        	    //ResMgr_EnterPLLMode(TRUE);
            //else
                //ResMgr_EnterNormalMode(TRUE);
#endif /* DYNAMIC_FREQUENCY_SWITCH */
        	*pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);        // PRQA S 3200
            status = SeApi_WiredEnable(value[0]);
            if(status == SEAPI_STATUS_OK || status == SEAPI_STATUS_NO_ACTION_TAKEN)
            {
                status = SEAPI_STATUS_OK;
                phOsalNfc_LogBle("\n\r SeApi_Wired %s\n", (value[0] == TRUE)?"ENABLE":"DISABLE");
            }
            pRespBuf[0] = status;// to indicate status to companion device
            break;
        case WIREDMODE_SERESET:
            *pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);       // PRQA S 3200
            status = SeApi_WiredResetEse();
            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("\nSeApi_WiredResetEse Passed");
            }
            else
            {
                phOsalNfc_LogBle("\nSeApi_WiredResetEse Failed; Status = 0x%02X", status);
            }
            /*xSemaphoreGive(ble_nfc_semaphore);*/
            if(GeneralState & NFCINIT_MASK)
            {
               GeneralState &= ~NFCINIT_MASK;
            }
            pRespBuf[0] = status;// to indicate status to companion device
            break;
        case JCOP_SERESET:
            *pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);
            status = SeApi_JcopDnd_WiredResetEse();
            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("\nSeApi_JcopDnd_WiredResetEse Passed");
            }
            else
            {
                phOsalNfc_LogBle("\nSeApi_JcopDnd_WiredResetEse Failed; Status = 0x%02X", status);
            }
            pRespBuf[0] = status;// to indicate status to companion device
            break;
        case WIREDMODE_CONTROL:
            *pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);
            phOsalNfc_LogBle("Stopping Any ongoing RF Discovery before Starting Jcop Download");
			status = SeApi_SeSelect(SE_NONE);
			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("SeApi_SeSelect Passed");
			}
			else
			{
				phOsalNfc_LogBle("SeApi_SeSelect Failed; Status = 0x%02X", status);
			}
            pRespBuf[0] = status;// to indicate status to companion device
            break;
        case SUB_SE_NTFN:
            bComSubForSeNotfn = value[0];
            *pRspSize = 1;
            memset(pRespBuf, 0, *pRspSize);       // PRQA S 3200
            pRespBuf[0] = 0x01;// to indicate companion device that subscribe/unsubscribe for se notification is successful
            break;
        case TRANSACTION_SE_NTFN:
            *pRspSize = length+1;//// one byte extra for event type
            *pRespTagType = SUB_SE_NTFN;
            memset(pRespBuf, 0, *pRspSize);       // PRQA S 3200
            pRespBuf[0] = TRANSACTION_SE_NTFN;
            phOsalNfc_MemCopy(&pRespBuf[1], value, (*pRspSize)-1);
            break;
        case CONNECTIVITY_SE_NTFN:
            *pRspSize = 2;
            *pRespTagType = SUB_SE_NTFN;
            memset(pRespBuf, 0, *pRspSize);       // PRQA S 3200
            pRespBuf[0] = CONNECTIVITY_SE_NTFN;
            pRespBuf[1]= value[0];
            break;
        case RFFIELD_SE_NTFN:
            *pRspSize = 2;
            *pRespTagType = SUB_SE_NTFN;
            memset(pRespBuf, 0, *pRspSize);      // PRQA S 3200
            pRespBuf[0] = RFFIELD_SE_NTFN;
            pRespBuf[1]= value[0];
            break;
        case ACTIVATION_SE_NTFN:
            *pRspSize = 2;
            *pRespTagType = SUB_SE_NTFN;
            memset(pRespBuf, 0, *pRspSize);      // PRQA S 3200
            pRespBuf[0] = ACTIVATION_SE_NTFN;
            pRespBuf[1]= value[0];
            break;

        case WIREDMODE_GET_ATR:
            phOsalNfc_LogCrit("\n-----------------\n Start : SeApi_WiredGetAtr \n-------------------\n");
            *pRspSize = RESP_BUFF_SIZE;
            memset(pRespBuf, 0, *pRspSize);     // PRQA S 3200
            status = SeApi_WiredGetAtr(pRespBuf, pRspSize);
            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("\nSeApi_WiredGetAtr  Passed");
                phOsalNfc_Log("\nATR length: %d, ATR: ", *pRspSize);
                for(i=0; i<*pRspSize; i++)
                {
                    phOsalNfc_LogBle("0x%02X ", pRespBuf[i]);
                }
            }
            else
            {
                phOsalNfc_LogBle("\nSeApi_WiredGetAtr  Failed");
                *pRspSize = 0; // reset respSize to zero in case of failure.
            }
            pRespBuf[(*pRspSize)++] = status;/*To indicate SUCCESS to the companion device*/
            phOsalNfc_LogCrit("----------------\n END : SeApi_WiredGetAtr \n-----------------\n");
            break;
        default:
            phOsalNfc_LogBle("SeModeTlvHandler: Default case; Wrong Sub-Type %d Sent\n", subType);
            break;
    }
}
#endif
