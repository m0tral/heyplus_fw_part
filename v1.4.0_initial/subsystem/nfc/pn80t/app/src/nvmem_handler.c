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
#include "UserData_Storage.h"
#include "SeApi_Int.h"
#include "debug_settings.h"
#include "wearable_platform_int.h"

void NvMemTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{
    *pRespTagType = 0x00;
    switch(subType)
    {
        case JCOP_NVMEM_READ:
        {
            GeneralState |= JCOPDNLD_MASK;
            *pRespSize = 1;
            tSEAPI_STATUS status = SeApi_GetJcopState(pRespBuf);
            if (status == SEAPI_STATUS_OK)
            {
                pRespBuf[(*pRespSize)++] = SEAPI_STATUS_OK;// to indicate companion device that writeUserdata was successful
            }
            else
            {
                pRespBuf[0] = SEAPI_STATUS_FAILED;
                GeneralState &= ~JCOPDNLD_MASK;
            }
        }
        break;

        case JCOP_NVMEM_WRITE:
        {
            *pRespSize = 1;
            tSEAPI_STATUS status = SeApi_SetJcopState(value[0]);
            if (status == SEAPI_STATUS_OK)
            {
                pRespBuf[0] = SEAPI_STATUS_OK;// to indicate companion device that writeUserdata was successful
            }
            else
            {
                pRespBuf[0] = SEAPI_STATUS_FAILED;
            }
            if(value[0] == 3)
            {
                GeneralState &= ~JCOPDNLD_MASK;
            }
        }
        break;

        default:
            phOsalNfc_LogBle("NvMemTlvHandler: Default case; Wrong Sub-Type %d Sent\n", subType);
        break;

    }
}
#endif
