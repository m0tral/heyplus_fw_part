
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
#include "debug_settings.h"
#include "wearable_platform_int.h"

void NciRawModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{

    UINT32 i;
    tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
    *pRespTagType = 0x00;
    switch(subType)
    {
        case SEND_NCI_RAW_COMMAND:
        phOsalNfc_LogCrit("\n-------------------------------------------\n SeApi_NciRawCmdSend to NFCC \n-------------------------------------------\n");
        *pRespSize = RESP_BUFF_SIZE - 1;
        memset(pRespBuf, 0, RESP_BUFF_SIZE);     // PRQA S 3200
        phOsalNfc_LogBle("SeApi_NciRawCmdSend to NFCC: Sending data [%d bytes] - ", length);
        for (i=0; i<length; i++)
        {
            phOsalNfc_LogBle("0x%02X ", *(value + i));
        }
        phOsalNfc_LogBle("\n");

        status = SeApi_NciRawCmdSend((UINT16)length, value, pRespSize, pRespBuf);
        if(SEAPI_STATUS_OK == status)
        {
            phOsalNfc_LogBle("SeApi_NciRawCmdSend SUCCESS\n");
            phOsalNfc_LogBle("Response: [%d bytes] - ", *pRespSize);
            for (i=0; i<*pRespSize; i++)
            {
                phOsalNfc_LogBle("0x%02X ", *(pRespBuf + i));
            }
            phOsalNfc_LogBle("\n");
        }
        else
        {
            phOsalNfc_LogBle("SeApi_NciRawCmdSend: FAILED. Status %d\n", status);
        }
        pRespBuf[(*pRespSize)++] = status;/*To indicate status to the companion device*/
        phOsalNfc_LogCrit("-------------------------------------------\n END SeApi_NciRawCmdSend to NFCC \n-------------------------------------------\n");
        break;
        default:
            phOsalNfc_LogBle("NciRawModeTlvHandler: Default case; Wrong Sub-Type %d Sent\n", subType);
        break;
    }

}
#endif
