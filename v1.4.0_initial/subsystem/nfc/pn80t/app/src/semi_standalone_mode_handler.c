

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

#if(WEARABLE_LS_LTSM_CLIENT == TRUE) && (SESUPPORT_INCLUDE == TRUE)
extern UINT16 cbPreLoad(UINT8* buf, UINT16 bufSize);
extern void cbPostLoad(UINT8* buf, UINT16 dataSize);
extern void cbPreStore(UINT8* buf, UINT16 dataSize);
extern void cbPostStore(UINT8* buf, UINT16 dataSize);
#if(LS_ENABLE == TRUE)
extern const unsigned char hash[];
extern const int32_t hash_len;
#endif

void WearableLsLtsmTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{
    tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
    UINT8 pResp[4];
    UINT32 respCnt;
    int vcLength = 0;
    int vcPersoLength = 0;
    unsigned short CardAVCNumber = 0;
    BOOLEAN isChunking;
    *pRespTagType = 0x00;
    *pRespSize = RESP_BUFF_SIZE;
    memset(pRespBuf, 0, *pRespSize);

    switch(subType)
    {
        case LS_EXECUTE_SCRIPT:
            phOsalNfc_LogBle("\n\r--- Start Script execution ---\n");
            /* Enable chunking */
            isChunking = TRUE;
            SeApi_SetConfig(SEAPI_SET_CHUNKING, &isChunking);   /* Always return true */
            status = SeApi_LoaderServiceDoScript(length, value, pRespSize, pRespBuf, hash_len, hash, pResp);
            /* Disable chunking */
            isChunking = FALSE;
            SeApi_SetConfig(SEAPI_SET_CHUNKING, &isChunking);   /* Always return true */
            phOsalNfc_LogBle("\n\nLS Script Output length: %d\n", *pRespSize-RSP_CHUNK_FOOTER_SIZE);
            phOsalNfc_LogBle("Cmd Chunk Offset=%hu\n", (pRespBuf[*pRespSize-CMD_CHUNK_OFFSET_OFFSET] << 8 | pRespBuf[*pRespSize-CMD_CHUNK_OFFSET_OFFSET+1]));
            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("\n\r LOADER SERVICE: SUCCESS");
            }
            else if(status == SEAPI_STATUS_BUFFER_FULL)
            {
                phOsalNfc_LogBle("\n\r LOADER SERVICE: RESP BUFFER FULL");
            }
            else
            {
                phOsalNfc_LogBle("\n\r LOADER SERVICE: FAILURE %d", status);
                *pRespSize = 0;
            }
            //append resp status word
			pRespBuf[(*pRespSize)++] = pResp[0];
			pRespBuf[(*pRespSize)++] = pResp[1];
			pRespBuf[(*pRespSize)++] = pResp[2];
			pRespBuf[(*pRespSize)++] = pResp[3];
			pRespBuf[(*pRespSize)++] = status;
            phOsalNfc_LogBle("\n\r--- END Script execution ---\n");
            break;
        case CREATE_VC:
            phOsalNfc_LogBle("\n-------------------------------------------\n MiFare Create Virtual Card \n-------------------------------------------\n");
            SeApi_MOP_Init(NULL);
            //Perso data is appended to vc data
            if(value[2] == 0x02 || value[2] == 0x03)
            {
                //Calculating VC length from total data received; Perso data starts with tag "42"
                while(value[vcLength] != PERSO_SM_AID && vcLength < length)
                {
                    vcLength = vcLength + value[vcLength + 1] + 2;//Adding 2 Bytes for Tag type and Length fields.
                }

                vcPersoLength = length - vcLength;
                status = SeApi_MOP_CreateVirtualCard("TestMain", value, (vcLength) * sizeof(UINT8),
                    value + vcLength, vcPersoLength * sizeof(UINT8), pRespBuf, pRespSize);
            }
            else
            {
                status = SeApi_MOP_CreateVirtualCard("TestMain", value, length  * sizeof(UINT8), NULL, 0, pRespBuf, pRespSize);
            }

            CardAVCNumber = pRespBuf[3];
            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("VC Create SUCCESS: VCEntry Number = %d\n", CardAVCNumber);
                pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
                *pRespSize = *pRespSize + 1;
            }
            else
            {
                phOsalNfc_LogBle("VC Create FAILED:\n");
                if(*pRespSize == 0)
                {
                    *pRespSize = 1;
                }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data: \n");
            for(respCnt = 0; respCnt< *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("\n\r -------------------------------------------\n");
            break;
        case DELETE_VC:
            phOsalNfc_LogBle("\n-------------------------------------------\n MiFare Delete Virtual Card \n-------------------------------------------\n");

            SeApi_MOP_Init(NULL);
            phOsalNfc_LogBle("\n\r Deleting VCEntryNumber: %d\n", value[0]);
            status = SeApi_MOP_DeleteVirtualCard("TestMain", value[0], pRespBuf, pRespSize);

            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("VC Delete SUCCESS\n");
                pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
                *pRespSize = *pRespSize + 1;
            }
            else
            {
                phOsalNfc_LogBle("VC Delete FAILED\n");
                if(*pRespSize == 0)
                {
                    *pRespSize = 1;
                }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data: \n");
            for(respCnt = 0; respCnt < *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("\n\r -------------------------------------------\n");
            break;
        case ACTIVATE_VC:
            phOsalNfc_LogBle("\n-------------------------------------------\n MiFare Activate Virtual Card \n-------------------------------------------\n");
            SeApi_MOP_Init(NULL);
            phOsalNfc_LogBle("\n\r Activating VCEntryNumber: %d\n", value[0]);
            status = SeApi_MOP_ActivateVirtualCard("TestMain", value[0], TRUE, TRUE, pRespBuf, pRespSize);

            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("VC Activate SUCCESS\n");
                pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
                *pRespSize = *pRespSize + 1;
            }
            else
            {
                phOsalNfc_LogBle("VC Activate FAILED\n");
                if(*pRespSize == 0)
                {
                    *pRespSize = 1;
                }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data: \n");
            for(respCnt = 0; respCnt < *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("\n\r -------------------------------------------\n");
            break;
        case DEACTIVATE_VC:
            phOsalNfc_LogBle("\n-------------------------------------------\n MiFare Deactivate Virtual Card \n-------------------------------------------\n");

            SeApi_MOP_Init(NULL);
            phOsalNfc_LogBle("\n\r Activating VCEntryNumber: %d\n", value[0]);
            status = SeApi_MOP_ActivateVirtualCard("TestMain", value[0], FALSE, FALSE, pRespBuf, pRespSize);

            if(status == SEAPI_STATUS_OK)
            {
                phOsalNfc_LogBle("VC Deactivate SUCCESS\n");
                pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
                *pRespSize = *pRespSize + 1;
            }
            else
            {
                phOsalNfc_LogBle("VC Deactivate FAILED\n");
                if(*pRespSize == 0)
                {
                    *pRespSize = 1;
                }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data: \n");
            for(respCnt = 0; respCnt < *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("\n\r -------------------------------------------\n");
            break;
        case SET_MDAC_VC:
            phOsalNfc_LogBle("\n-------------------------------------------\nSet MDAC for Virtual Card  \n-------------------------------------------\n");

            SeApi_MOP_Init(NULL);

            phOsalNfc_LogBle("Updating VCEntryNumber: %d\n", value[0]);
            status = SeApi_MOP_AddAndUpdateMdac("TestMain", value[0], (value + 1), ((length - 1) * sizeof(UINT8)), pRespBuf, pRespSize);

            if(status == SEAPI_STATUS_OK)
            {
               phOsalNfc_LogBle("MDAC configure SUCCESS\n");
               pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
               *pRespSize = *pRespSize + 1;
            }
            else
            {
               phOsalNfc_LogBle("MDAC configure FAILED\n");
               if(*pRespSize == 0)
               {
                   *pRespSize = 1;
               }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data:\n ");
            for(respCnt = 0; respCnt < *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            phOsalNfc_LogBle("\n");

            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("-------------------------------------------\n");
            break;
        case READ_VC:
            phOsalNfc_LogBle("\n-------------------------------------------\n Read VC  \n-------------------------------------------\n");
            SeApi_MOP_Init(NULL);

            phOsalNfc_LogBle("Updating VCEntryNumber: %d\n", value[0]);
            status = SeApi_MOP_ReadMifareData("TestMain", value[0], value + 1, (length - 1) * sizeof(UINT8), pRespBuf, pRespSize);
            if(status == SEAPI_STATUS_OK)
            {
               phOsalNfc_LogBle("Read VC SUCCESS\n");
               pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
               *pRespSize = *pRespSize + 1;
            }
            else
            {
               phOsalNfc_LogBle("Read VC FAILED\n");
               if(*pRespSize == 0)
               {
                   *pRespSize = 1;
               }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data: \n");
            for(respCnt = 0; respCnt < *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            phOsalNfc_LogBle("\n");

            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("-------------------------------------------\n");
            break;
        case GET_VC_LIST:
            phOsalNfc_LogBle("\n-------------------------------------------\n GET VC  List\n-------------------------------------------\n");
            SeApi_MOP_Init(NULL);

            phOsalNfc_LogBle("Getting VC List\n");
            status = SeApi_MOP_GetVcList(pRespBuf, pRespSize);

            if(status == SEAPI_STATUS_OK)
            {
               phOsalNfc_LogBle("GET VC List SUCCESS\n");
               pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
               *pRespSize = *pRespSize + 1;
            }
            else
            {
               phOsalNfc_LogBle("GET VC List FAILED\n");
               if(*pRespSize == 0)
               {
                   *pRespSize = 1;
               }
            }

            phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
            phOsalNfc_LogBle("response data: \n");
            for(respCnt = 0; respCnt < *pRespSize; respCnt++)
            {
                phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
            }
            phOsalNfc_LogBle("\n");

            SeApi_MOP_Deinit();
            phOsalNfc_LogBle("-------------------------------------------\n");
            break;
        case GET_VC_STATUS:
            phOsalNfc_LogBle("\n-------------------------------------------\n GET VC Status\n-------------------------------------------\n");

			SeApi_MOP_Init(NULL);
			phOsalNfc_LogBle("Updating VCEntryNumber: %d\n", value[0]);
			status =  SeApi_MOP_GetVcStatus("TestMain", value[0], pRespBuf, pRespSize);

			if(status == SEAPI_STATUS_OK)
			{
			   phOsalNfc_LogBle("GET VC Status SUCCESS\n");
			   pRespBuf[*pRespSize] = SEAPI_STATUS_OK;
			   *pRespSize = *pRespSize + 1;
			}
			else
			{
			   phOsalNfc_LogBle("GET VC Status FAILED\n");
			   if(*pRespSize == 0)
			   {
				   *pRespSize = 1;
			   }
			}

			phOsalNfc_LogBle("response data size = %d\n", *pRespSize);
			phOsalNfc_LogBle("response data: \n");
			for(respCnt = 0; respCnt < *pRespSize; respCnt++)
			{
				phOsalNfc_LogBle("%02X ", pRespBuf[respCnt]);
			}
			phOsalNfc_LogBle("\n");

			SeApi_MOP_Deinit();
			phOsalNfc_LogBle("-------------------------------------------\n");
            break;
        default:
            phOsalNfc_LogBle("WearableLsLtsmTlvHandler: Default case; Wrong Sub-Type Sent\n");
    }
}
#endif
#endif
