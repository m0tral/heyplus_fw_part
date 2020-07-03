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

#if (API_TEST_ENABLE == TRUE)
#if (TEST_STANDALONE == TRUE)
extern const uint8_t LS_CreateSD[];
extern const int32_t LS_CreateSD_len;
extern const uint8_t LS_LoadEcho[];
extern const int32_t LS_LoadEcho_len;
extern const uint8_t LS_InstallEcho[];
extern const int32_t LS_InstallEcho_len;
extern const uint8_t LS_DeleteEcho[];
extern const int32_t LS_DeleteEcho_len;
extern const uint8_t LS_LoadEcho_Lt1kMetadata[];
extern const int32_t LS_LoadEcho_Lt1kMetadata_len;
extern const uint8_t LS_InstallEcho_Gt1kMetaData[];
extern const int32_t LS_InstallEcho_Gt1kMetaData_len;
extern const uint8_t LS_DeleteEcho_1023MetaData[];
extern const int32_t LS_DeleteEcho_1023MetaData_len;
#endif

#if(MIFARE_OP_ENABLE == TRUE)
//UINT8* gMOPBuffer;
const unsigned char vcData[] = {0x46, 0x01, 0x00,
		                      0xA5, 0x07, 0x02, 0x02, 0x01, 0x04, 0x03, 0x01, 0x00,
		                      0xA8, 0x12, 0x20, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
							  0x00, 0xF0, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
							  0xFF, 0xFF };

const unsigned char Read_Vc[] = {0xE7, 0x0F, 0xD0, 0x02, 0x00, 0x01,
							   0xD1, 0x01, 0x00,
							   0xD4, 0x01, 0x01,
							   0xD5, 0x03, 0x01, 0x00, 0x0F,
                               0xB0, 0x04, 0xD5, 0x02, 0x00, 0x01 };

const unsigned char MDAC[] = {0x60, 0x41,
							0x04, 0x76, 0x6E, 0x07, 0x6A, 0xAA, 0xEF, 0xD3,
							0xDE, 0x4D, 0x1F, 0x88, 0xFC, 0xD6, 0x2F, 0x50,
							0xEE, 0x5A, 0x0C, 0x67, 0x2D, 0xB1, 0xB1, 0x8E,
							0x37, 0x6D, 0x5B, 0x61, 0x4F, 0xFD, 0x00, 0xD6,
							0x19, 0x68, 0xF1, 0xAB, 0xEF, 0xFE, 0x9C, 0x42,
							0xAA, 0xCC, 0xE1, 0xEF, 0x4C, 0x9A, 0xAA, 0x25,
							0x4D, 0x50, 0x74, 0x6C, 0xA1, 0x9C, 0xE0, 0x9C,
							0x5B, 0xA8, 0x74, 0x35, 0x56, 0xAE, 0xF3, 0x32,
							0x27,
                            0x62, 0x81,
							0x80, 0x52, 0xFE, 0x60, 0x98, 0xF6, 0x9E, 0x81,
							0x0A, 0xED, 0x53, 0xAC, 0x40, 0x35, 0x7B, 0x88,
							0x54, 0xA4, 0x62, 0x38, 0x61, 0x08, 0xFD, 0x08,
							0x59, 0x39, 0x42, 0xA8, 0x57, 0x21, 0x93, 0xB3,
							0x2F, 0x9D, 0x28, 0xE1, 0x8F, 0x3E, 0xB8, 0x5F,
							0x0C, 0x62, 0x61, 0xBE, 0x92, 0x3A, 0x9D, 0x77,
							0xF1, 0xAE, 0xFC, 0x60, 0xB2, 0x3C, 0x5D, 0x6F,
							0xBB, 0xF5, 0x7F, 0x0E, 0xAD, 0x17, 0x1E, 0xA7,
							0xCC, 0xC3, 0x0A, 0xF7, 0xB6, 0x6F, 0xFA, 0x7A,
							0x55, 0x8A, 0xA0, 0x46, 0x69, 0x1A, 0x02, 0x8E,
							0x70, 0x4B, 0x25, 0x35, 0xC2, 0x88, 0x17, 0x8A,
							0x61, 0xD2, 0x54, 0x65, 0xF1, 0x1B, 0x15, 0x63,
							0x67, 0xC9, 0x00, 0x41, 0x0E, 0x94, 0x3E, 0x32,
							0xA6, 0x04, 0x24, 0xBC, 0x36, 0x42, 0x4D, 0x58,
							0x1D, 0xAA, 0x23, 0x44, 0xD2, 0x32, 0x00, 0x15,
							0x64, 0x57, 0xC6, 0x17, 0xDB, 0x89, 0xCB, 0x52,
							0x3A};
const uint16_t ReadVc_Len =sizeof(Read_Vc);
const uint16_t vcData_Len =sizeof(vcData);
const uint16_t MDAC_Len =sizeof(MDAC);
static UINT16 vcEntry;
#endif
#if(LS_ENABLE == TRUE)
extern const unsigned char hash[];
extern const int32_t hash_len;
#endif


extern UINT16 cbPreLoad(UINT8* buf, UINT16 bufSize);
extern void cbPostLoad(UINT8* buf, UINT16 dataSize);
extern void cbPreStore(UINT8* buf, UINT16 dataSize);
extern void cbPostStore(UINT8* buf, UINT16 dataSize);

extern void GeneralModeTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* respSize, uint8_t* pRespBuf, unsigned char* respTagType);


void ApiTestTlvHandler(unsigned char subType, uint8_t* value, unsigned int length, unsigned short* pRespSize, uint8_t* pRespBuf, unsigned char* pRespTagType)
{
	tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
	*pRespTagType = 0x00;
	*pRespSize = RESP_BUFF_SIZE;
	memset(pRespBuf, 0, *pRespSize);

	switch(subType)
	{
		case WIRED_TRANSCEIVE:
		case WIREDMODE_ENABLE:
		case WIREDMODE_SERESET:
		case WIREDMODE_GET_ATR:
			SeModeTlvHandler(subType, value, length, pRespSize, pRespBuf, pRespTagType);
			break;

		case SEAPI_INIT:
			phOsalNfc_LogBle("\n------------ Start SeApi_Init  -------------\n");
			if(value[0] == 0x00)
			{
#if (ENABLE_SESSION_MGMT == TRUE)
				status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
				status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
				if(status == SEAPI_STATUS_OK)
				{
					phOsalNfc_LogBle("\nSeApi_Init  Passed\n");
				}
				else
				{
					phOsalNfc_LogBle("\nSeApi_Init Failed");
				}
				*pRespSize = 0x01;
				pRespBuf[0] = status; /*To-Do; Currently return type is not defined.So, assuming passed always*/
				phOsalNfc_LogBle("\n ----------- END SeApi_Init -------------\n");
			}
			else
			{
				phOsalNfc_LogBle("\n------------ Start: SeApi_Init Negative Testing -------------\n");
				if(value[0] == 0x01)
				{
					phOsalNfc_LogBle("\n---------------\n SeApi_Init Negative: Transaction Callback as NULL \n---------------\n");
					status = SeApi_Shutdown();
					if(status == SEAPI_STATUS_OK)
					{
						phOsalNfc_LogBle("SeApi_Shutdown Passed; Starting Init with Transaction Callback as NULL \n");
#if (ENABLE_SESSION_MGMT == TRUE)
					    status = SeApi_Init(NULL, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
					    status = SeApi_Init(NULL, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
					}
				}
				else if(value[0] == 0x02)
				{
					status = SeApi_Shutdown();
					if(status == SEAPI_STATUS_OK)
					{
#if (ENABLE_SESSION_MGMT == TRUE)
					    phOsalNfc_LogBle("\n---------------\n SeApi_Init Negative: Session Callback as NULL \n---------------\n");
						status = SeApi_Init(&transactionOccurred, NULL);
#else /* ENABLE_SESSION_MGMT == TRUE */
						phOsalNfc_LogBle("\n SeApi_Init Negative: Session Callback as NULL is expected when SESSION_MGMT == FALSE\n");
						status = SEAPI_STATUS_INVALID_PARAM;
#endif /* ENABLE_SESSION_MGMT == TRUE */
					}
				}
				else
				{
					phOsalNfc_LogBle("\nSeApi_Init Wrong Value Passed: 0x%02X\n",value[0]);
				}

				if(status == SEAPI_STATUS_OK)
				{
					phOsalNfc_LogBle("\nSeApi_Init returns SEAPI_STATUS_OK; Negative Test Failed\n");
				}
				else
				{
					phOsalNfc_LogBle("\nSeApi_Init returns: 0x%02X\n",status);
				}
				*pRespSize = 0x01;
				pRespBuf[0] = status; /*To-Do; Currently return type is not defined.So, assuming passed always*/
				if(status!=SEAPI_STATUS_OK)
				{
#if (ENABLE_SESSION_MGMT == TRUE)
				    status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
				    status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
				    if(status == SEAPI_STATUS_OK)
				    {
					    phOsalNfc_LogBle("\nSeApi_Init  Passed\n");
				    }
				    else
				    {
					    phOsalNfc_LogBle("\nSeApi_Init Failed");
				    }
				}
				phOsalNfc_LogBle("\n ----------- END: SeApi_Init Negative Testing -------------\n");
			}
			break;

		case SEAPI_SHUTDOWN:
			phOsalNfc_LogBle("\n---------------\n LIBNFC_NCI_DEINIT  \n------------------\n");
			status = SeApi_Shutdown();
			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nSeApi_Shutdown  Passed\n");
			}
			else
			{
				phOsalNfc_LogBle("\nSeApi_Shutdown Failed");
			}
			*pRespSize = 0x01;
			pRespBuf[0] = status;
			if(status==SEAPI_STATUS_OK)
			{
#if (ENABLE_SESSION_MGMT == TRUE)
			    status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
			    status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
			    if(status == SEAPI_STATUS_OK)
			    {
				    phOsalNfc_LogBle("\nSeApi_Init  Passed\n");
			    }
			    else
			    {
				    phOsalNfc_LogBle("\nSeApi_Init Failed");
			    }
			}
			phOsalNfc_LogBle("----------------\n END LIBNFC_NCI_DEINIT \n------------------\n");
			break;

		case SEAPI_SESELECT:
			phOsalNfc_LogCrit("\n-----------------\n Start SeApi_SeSelect \n-----------------\n");

			UINT16 data = ((value[0] << 8) & 0xFF00) | (value[1] & 0x00FF);
			phOsalNfc_LogBle("eSeSelect : 0x%02X ", data);

			eSEOption eSeSelect;

			if(data == 0x00)
				eSeSelect = SE_NONE;
			else if(data == 0x4C0)
				eSeSelect = SE_ESE;
			else if(data == 0x402)
				eSeSelect = SE_UICC;
			else
				eSeSelect = SE_UNDEF;

			status = SeApi_SeSelect(eSeSelect);
			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nSeApi_SeSelect  Passed\n");
			}
			else
			{
				phOsalNfc_LogBle("\nSeApi_SeSelect Failed");
			}
			*pRespSize = 0x01;
			pRespBuf[0] = status;

			phOsalNfc_LogCrit("------------------\n END SeApi_SeSelect \n-----------------\n");
			break;

		case SEAPI_SECONFIGURE:
			phOsalNfc_LogCrit("\n---------------\n START: SeApi_SeConfigure \n----------------\n");
			if(value[0] == 0x00)
			{
				phOsalNfc_LogBle("\n---------------\n SeApi_SeConfigure: NFC all NFCEE off \n---------------\n");
				status = SeApi_SeConfigure(0x00, 0x00);
			}
			else if(value[0] == 0x01)
			{
				phOsalNfc_LogBle("\n---------------\n SeApi_SeConfigure: NFC eSE Virtual Operation \n---------------\n");
				status = SeApi_SeConfigure(SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B, 0x00);
			}
			else if(value[0] == 0x02)
			{
				phOsalNfc_LogBle("\n-------------------\n SeApi_SeConfigure: NFC UICC Virtual Operation \n---------------\n");
				status = SeApi_SeConfigure(0x00, SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B | SEAPI_TECHNOLOGY_MASK_F);
			}
			else if(value[0] == 0x03)
			{
				phOsalNfc_LogBle("\n------------------\n SeApi_SeConfigure: Combined Virtual Operation eSE:TechA, UICC:TechB \n--------------\n");
				status = SeApi_SeConfigure(SEAPI_TECHNOLOGY_MASK_A, SEAPI_TECHNOLOGY_MASK_B);
			}
			else
			{
				phOsalNfc_LogBle("\nSEAPI_SECONFIGURE: Wrong Value passed : 0x%02X\n",value[0]);
			}

			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nSeApi_SeConfigure  Passed\n");
			}
			else
			{
				phOsalNfc_LogBle("\nSeApi_SeConfigure  Failed\n");
			}
			*pRespSize = 0x01;
			pRespBuf[0] = status;// to indicate status to companion device
			phOsalNfc_LogCrit("---------------\n END: SeApi_SeConfigure \n-------------\n");
			break;

#if(FIRMWARE_DOWNLOAD_ENABLE == TRUE)
		case SEAPI_FW_DNLD:
			phOsalNfc_LogCrit("\n---------------\n Start: Firmware Download\n ------------------\n");
			status = SeApi_Shutdown();
			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nSeApi_Shutdown  Passed; Starting Firmware Download\n");
				status = SeApi_ForcedFirmwareDownload();
				if(status == SEAPI_STATUS_OK)
				{
					phOsalNfc_LogBle("SeApi_ForcedFirmwareDownload Success; Initializing Stack\n");
#if (ENABLE_SESSION_MGMT == TRUE)
					status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
					status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
					if(status == SEAPI_STATUS_OK)
					{
						phOsalNfc_LogBle("\nSeApi_Init After Forced FW Download Passed\n");
					}
					else
					{
						phOsalNfc_LogBle("\nSeApi_Init After Forced FW Download Failed");
					}
				}
				else
				{
					phOsalNfc_LogBle("Firmware Download Failed\n");
				}
			}
			else
			{
				phOsalNfc_Log("ShutDown Failed; Exiting\n");
			}
			*pRespSize = 0x01;
            pRespBuf[0] = status;// to indicate status to companion device
			phOsalNfc_LogCrit("\n -------------- END: Firmware Download ------------\n");
			break;
#endif /*FIRMWARE_DOWNLOAD_ENABLE == TRUE */

		case GETSTACK_CAP:
			GeneralModeTlvHandler(subType, value, length, pRespSize, pRespBuf, pRespTagType);
			break;

#if (TEST_STANDALONE == TRUE)
#if (LS_ENABLE == TRUE)
		unsigned char pResp[4];
		case LS_NEGATIVE:
			phOsalNfc_LogBle("\n------------ START: LS Negative API Testing -------------\n");

			*pRespSize = 0x01;
			if(value[0] == 0x00)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Script Data Buffer as NULL\n");
				status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, NULL, pRespSize, pRespBuf, hash_len, hash, pResp);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",pResp[0], pResp[1], pResp[2], pResp[3]);
			}
			else if(value[0] == 0x01)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Script Data Buffer Length as 0\n");
				status = SeApi_LoaderServiceDoScript(0, LS_CreateSD, pRespSize, pRespBuf, hash_len, hash, pResp);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",pResp[0], pResp[1], pResp[2], pResp[3]);
			}
			else if(value[0] == 0x02)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Response Buffer as NULL\n");
				status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, LS_CreateSD, pRespSize, NULL, hash_len, hash, pResp);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",pResp[0], pResp[1], pResp[2], pResp[3]);
			}
			else if(value[0] == 0x03)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Response Buffer Length as 0\n");
				status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, LS_CreateSD, 0, pRespBuf, hash_len, hash, pResp);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",pResp[0], pResp[1], pResp[2], pResp[3]);
			}
			else if(value[0] == 0x04)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Hash Buffer as NULL\n");
				status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, LS_CreateSD, pRespSize, pRespBuf, hash_len, NULL, pResp);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",pResp[0], pResp[1], pResp[2], pResp[3]);
			}
			else if(value[0] == 0x05)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Hash Buffer Length as 0\n");
				status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, LS_CreateSD, pRespSize, pRespBuf, 0, hash, pResp);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",pResp[0], pResp[1], pResp[2], pResp[3]);
			}
			else if(value[0] == 0x06)
			{
				phOsalNfc_LogBle("\n\r LS Negative: Status Word Buffer Length < 4\n");
				unsigned char pRespLessSize[2];
				status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, LS_CreateSD, pRespSize, pRespBuf, hash_len, hash, pRespLessSize);
				phOsalNfc_LogBle("\n\r LS Negative: SW: 0x%02X 0x%02X\n",pRespLessSize[0], pRespLessSize[1]);
				*pRespSize = 1;
			}
			else
			{
				phOsalNfc_LogBle("\nLS_NEGATIVE : Wrong Value passed\n");
			}

			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nSeApi_LS return SEAPI_STATUS_OK; Negative test Failed\n");
			}
			else
			{
				phOsalNfc_LogBle("\nSeApi_LS returns: 0x%02X\n",status);
			}
			pRespBuf[0] = status;// to send status to companion device
			phOsalNfc_LogBle("\n ----------- END: LS Negative API Testing -------------\n");
			break;

        case LS_CHUNK_API_TEST:
        {
            /* API Parameters for SeApi_LoaderServiceProcessInChunk
             * Loader service process
             * buffer - input data
             * chunk_len - input data length
             * pChunkResp - response chunk buffer
             * chunkRespLen - response chunk length
             * script_len - total script length, constant
             * offset - total executed script length
             * hash - hash
             * hash_len - hash length
             * pResp - response SW
             * stackCapabilities = stack capabilities */
            UINT8 i;
            UINT16 j;
            UINT8 *buffer, pChunkResp[LS_RSP_CHUNK_SIZE + CMD_CHUNK_FOOTER_SIZE];
            UINT32 offset = 0;
            UINT32 lsSript_len[] = {LS_CreateSD_len, LS_LoadEcho_len, LS_InstallEcho_len, LS_DeleteEcho_len};
            UINT32 lsSript_len_md[] = {LS_CreateSD_len, LS_LoadEcho_Lt1kMetadata_len, LS_InstallEcho_Gt1kMetaData_len, LS_DeleteEcho_1023MetaData_len};
            UINT32 lsSript_len_negative[] = {LS_DeleteEcho_len};
            UINT32 script_len;
            UINT16 chunkRespLen;
            const UINT8 *pLsScripts[] = {LS_CreateSD, LS_LoadEcho, LS_InstallEcho, LS_DeleteEcho};
            const UINT8 *pLsScripts_md[] = {LS_CreateSD, LS_LoadEcho_Lt1kMetadata, LS_InstallEcho_Gt1kMetaData, LS_DeleteEcho_1023MetaData};
            const UINT8 *pLsScripts_negative[] = {LS_DeleteEcho};
            const UINT8 *pScData;
            UINT32 chunk_len;
            phStackCapabilities_t stackCapabilities;
            status = SeApi_GetStackCapabilities(&stackCapabilities);
            if (status != SEAPI_STATUS_OK)
            {
                phOsalNfc_Log( "\n GetStackcapabilities failed with status %s\n", status);
                return;
            }
            status = SEAPI_STATUS_FAILED;
            if(value[0]==0x00){
                /* Allocate the buffer */
                buffer = (UINT8 *)phOsalNfc_GetMemory(1024 + CMD_CHUNK_FOOTER_SIZE);
                for(i = 0; i < 4; i++) {
                    offset = 0;
                    pScData = pLsScripts[i];
                    script_len = lsSript_len[i];
                    while(offset < script_len) {
                        /* Get the chunk length */
                        chunk_len = ((script_len - offset) > 1024) ? 1024 : (script_len - offset);
                        /* Read the data */
                        (void)phOsalNfc_MemCopy(buffer, &pScData[offset], chunk_len);

                        chunkRespLen = sizeof(pChunkResp);
                        status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                  &chunkRespLen, script_len, &offset,
                                                                  hash, hash_len, pResp, &stackCapabilities);

                        phOsalNfc_Log( "Processed %d / %d:\n", offset, script_len );

                        if(status == SEAPI_STATUS_OK) {
                            for(j = 0; j< chunkRespLen; j++)
                                phOsalNfc_Log("%c",*(pChunkResp+j));
                        }
                        else if(status == SEAPI_STATUS_NO_ACTION_TAKEN) {
                            continue;
                        }
                        else {
                            break;
                        }
                    }

                    phOsalNfc_Log( "\n\r LOADER SERVICE: %s %d SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",
                                       (SEAPI_STATUS_OK == status) ? "SUCCESS" : "FAILURE", status, pResp[0], pResp[1], pResp[2], pResp[3]);
                    if((i==0)&& (status !=SEAPI_STATUS_OK))
                    {
                        i=2; /* Create SD has failed; Skip Load and Install; Setting Index to Delete SD */
                    }
                }
                phOsalNfc_FreeMemory(buffer);
            }

            else if(value[0]==0x01){
                /* Testing LS scripts with Meta data scenario, Greater than 1k, Less than 1k and 1023 byte of MetaData*/
                buffer = (UINT8 *)phOsalNfc_GetMemory(1024 + CMD_CHUNK_FOOTER_SIZE);
                for(i = 0; i < 4; i++) {
                    offset = 0;
                    pScData = pLsScripts_md[i];
                    script_len = lsSript_len_md[i];
                    while(offset < script_len) {
                        /* Get the chunk length */
                        chunk_len = ((script_len - offset) > 1024) ? 1024 : (script_len - offset);
                        /* Read the data */
                        (void)phOsalNfc_MemCopy(buffer, &pScData[offset], chunk_len);

                        chunkRespLen = sizeof(pChunkResp);
                        status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                  &chunkRespLen, script_len, &offset,
                                                                  hash, hash_len, pResp, &stackCapabilities);

                        phOsalNfc_Log( "Processed %d / %d:\n", offset, script_len );

                        if(status == SEAPI_STATUS_OK) {
                            for(j = 0; j< chunkRespLen; j++)
                                phOsalNfc_Log("%c",*(pChunkResp+j));
                        }
                        else if(status == SEAPI_STATUS_NO_ACTION_TAKEN) {
                            continue;
                        }
                        else {
                            break;
                        }
                    }

                    phOsalNfc_Log( "\n\r LOADER SERVICE: %s %d SW: 0x%02X 0x%02X 0x%02X 0x%02X\n",
                                       (SEAPI_STATUS_OK == status) ? "SUCCESS" : "FAILURE", status, pResp[0], pResp[1], pResp[2], pResp[3]);
                    if(status !=SEAPI_STATUS_OK)
                    {
                        phOsalNfc_Log( "\n\r***** LS Script at index [%d] failed to execute; Setting Index to Delete SD", i);
                        i=2;
                    }

                }
                phOsalNfc_FreeMemory(buffer);
            }

            else if (value[0]==0x02){

                buffer = (UINT8 *)phOsalNfc_GetMemory(1024 + CMD_CHUNK_FOOTER_SIZE);
                offset = 0;
                pScData = pLsScripts_negative[0];
                script_len = lsSript_len_negative[0];
                chunk_len = ((script_len - offset) > 1024) ? 1024 : (script_len - offset);
                (void)phOsalNfc_MemCopy(buffer, &pScData[offset], chunk_len);
                chunkRespLen = sizeof(pChunkResp);
                if(value[1] == 0x00){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk Data Buffer as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(NULL, &chunk_len, pChunkResp,
                                                                &chunkRespLen, script_len, &offset,
                                                                hash, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x01){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk Data Size as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, NULL, pChunkResp,
                                                                &chunkRespLen, script_len, &offset,
                                                                hash, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x02){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk RESP buffer as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, NULL,
                                                                &chunkRespLen, script_len, &offset,
                                                                hash, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x03){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk RESP length as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                NULL, script_len, &offset,
                                                                hash, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x04){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk script length as 0\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                &chunkRespLen, 0, &offset,
                                                                hash, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x05){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk script offset as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                &chunkRespLen, script_len, NULL,
                                                                hash, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x06){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk script hash as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                &chunkRespLen, script_len, &offset,
                                                                NULL, hash_len, pResp, &stackCapabilities);
                }
                else if(value[1]==0x07){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk script hash_len as 0\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                &chunkRespLen, script_len, &offset,
                                                                hash, 0, pResp, &stackCapabilities);
                }
                else if(value[1]==0x08){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk script pResp as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                &chunkRespLen, script_len, &offset,
                                                                hash, hash_len, NULL, &stackCapabilities);
                }
                else if(value[1]==0x09){
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Chunk script stackCapabilities as NULL\n");
                    status = SeApi_LoaderServiceProcessInChunk(buffer, &chunk_len, pChunkResp,
                                                                &chunkRespLen, script_len, &offset,
                                                                hash, hash_len, pResp, NULL);
                }
                else {
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Invalid Scenario\n");
                }

                if(status != SEAPI_STATUS_INVALID_PARAM) {
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Test case Fail\n");
                }
                else  {
                    phOsalNfc_LogBle("\n\r LS Chunk Negative: Test Case Pass\n");
                }
                phOsalNfc_FreeMemory(buffer);
            }
            *pRespSize = 0x01;
            pRespBuf[0] = status;// to indicate status to companion device
        }
        break;
#endif/*LS_ENABLE*/

#if (MIFARE_OP_ENABLE == TRUE)
		case LTSM_NEGATIVE:
			phOsalNfc_LogBle("\n------------ START: LTSM Negative API Testing -------------\n");
			*pRespSize = RESP_BUFF_SIZE;
			vcEntry = value[1];
			switch (value[0])
			{

			case 0xFF:
				phOsalNfc_Log("Initializing MOP\n");
				status = SeApi_MOP_Init(NULL);
				break;
			case 0x00:
				phOsalNfc_LogBle("\n\r LTSM: Create Mifare VC with vcData Buffer as NULL\n");
				status = SeApi_MOP_CreateVirtualCard("TestMain", NULL,vcData_Len , NULL, 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x01:
				phOsalNfc_LogBle("\n\r LTSM: Create Mifare VC with vcData Buffer Length as 0\n");
				status = SeApi_MOP_CreateVirtualCard("TestMain", vcData, 0 , NULL, 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x02:
				phOsalNfc_LogBle("\n\r LTSM: Delete Mifare VC with RespBuffer as NULL\n");
				status = SeApi_MOP_DeleteVirtualCard("TestMain", 1, NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x03:
				*pRespSize = 0;
				phOsalNfc_LogBle("\n\r LTSM: Delete Mifare VC with RespBuffer Length as 0\n");
				status = SeApi_MOP_DeleteVirtualCard("TestMain", 1, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x04:
				phOsalNfc_LogBle("\n\r LTSM: Delete MiFare Virtual Card 0\n");
				status = SeApi_MOP_DeleteVirtualCard("TestMain", 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
			break;
			case 0x05:
				phOsalNfc_LogBle("\n\r LTSM: Activate Mifare Virtual card 0\n");
				status = SeApi_MOP_ActivateVirtualCard("TestMain", 0, TRUE, FALSE, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
			break;
			case 0x06:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC with MDAC Data Buffer as NULL\n");
				status = SeApi_MOP_AddAndUpdateMdac("TestMain", 1 , NULL , MDAC_Len, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
	        break;
			case 0x07:
				phOsalNfc_LogBle("\n\r LTSM: Read Mifare Data with Resp Buffer as NULL\n");
				status = SeApi_MOP_ReadMifareData("TestMain", 1, Read_Vc, ReadVc_Len, NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x08:
				*pRespSize = 0;
				phOsalNfc_LogBle("\n\r LTSM: Read Mifare Data with Resp Buffer Length as 0\n");
				status = SeApi_MOP_ReadMifareData("TestMain", 1, Read_Vc, ReadVc_Len , NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x09:
				phOsalNfc_LogBle("\n\r LTSM: Get VC Status for Mifare VC 0\n");
				status = SeApi_MOP_GetVcStatus("TestMain", 0,pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
			break;
			case 0x0A:
				*pRespSize = 0;
				phOsalNfc_LogBle("\n\r LTSM: Get VC List with Response Buffer Length as 0\n");
				status = SeApi_MOP_GetVcList(pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x0B:
			    phOsalNfc_LogBle("\n\r LTSM: Read Mifare Data with Appname greater than 32 bytes\n");
			    status = SeApi_MOP_ReadMifareData("000102030405060708090a0b0c0d0e0f1011125131415161718191a1b1c1d1e1f20", 1, Read_Vc, ReadVc_Len , pRespBuf, pRespSize);
			    phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x0C:
			    phOsalNfc_LogBle("\n\r LTSM: Read Mifare Data with Appname NULL\n");
			    status = SeApi_MOP_ReadMifareData(NULL, 1, Read_Vc, ReadVc_Len , pRespBuf, pRespSize);
			    phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x0D:
			    phOsalNfc_LogBle("\n\r LTSM: Delecte VC Card with Appname greater than 32 bytes\n");
			    status = SeApi_MOP_DeleteVirtualCard("000102030405060708090a0b0c0d0e0f1011125131415161718191a1b1c1d1e1f20", 1, pRespBuf, pRespSize);
			    phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x0E:
			    phOsalNfc_LogBle("\n\r LTSM: Delecte VC Card with Appname NULL\n");
			    status = SeApi_MOP_DeleteVirtualCard(NULL, 1, pRespBuf, pRespSize);
			    phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x0F:
			    phOsalNfc_LogBle("\n\r LTSM: Activate VC Card with Appname greater than 32 bytes\n");
			    status = SeApi_MOP_ActivateVirtualCard("000102030405060708090a0b0c0d0e0f1011125131415161718191a1b1c1d1e1f20", 1, TRUE, FALSE, pRespBuf, pRespSize);
			    phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x10:
			    phOsalNfc_LogBle("\n\r LTSM: Activate VC Card with Appname NULL\n");
			    status = SeApi_MOP_ActivateVirtualCard(NULL , 1, TRUE, FALSE, pRespBuf, pRespSize);
			    phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			break;
			case 0x11:
				phOsalNfc_LogBle("\n\r LTSM: Get VC Status with Appname NULL\n");
				status = SeApi_MOP_GetVcStatus(NULL, 1, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x12:
				phOsalNfc_LogBle("\n\r LTSM: Get VC Status with Appname greater than 32 bytes\n");
				status = SeApi_MOP_GetVcStatus("000102030405060708090a0b0c0d0e0f1011125131415161718191a1b1c1d1e1f20", 1, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x13:
				phOsalNfc_LogBle("\n\r LTSM: Get VC Status with Response Buffer as NULL\n");
				status = SeApi_MOP_GetVcStatus("TestMain", 1, NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x14:
				phOsalNfc_LogBle("\n\r LTSM: Create Virtual Card with Appname greater than 32 bytes\n");
				status = SeApi_MOP_CreateVirtualCard("000102030405060708090a0b0c0d0e0f1011125131415161718191a1b1c1d1e1f20", vcData,vcData_Len , NULL, 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x15:
				phOsalNfc_LogBle("\n\r LTSM: Create Virtual Card with Appname NULL\n");
				status = SeApi_MOP_CreateVirtualCard(NULL, vcData,vcData_Len , NULL, 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x16:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC with App name greater than 32 bytes\n");
				status = SeApi_MOP_AddAndUpdateMdac("000102030405060708090a0b0c0d0e0f1011125131415161718191a1b1c1d1e1f20", 1 , MDAC , MDAC_Len, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x17:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC with MDAC App name as NULL\n");
				status = SeApi_MOP_AddAndUpdateMdac(NULL, 1 , MDAC , MDAC_Len, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x18:
				*pRespSize = 0;
				phOsalNfc_LogBle("\n\r LTSM: Create Mifare VC with Response Buffer Length as 0\n");
				status = SeApi_MOP_CreateVirtualCard("TestMain", vcData, vcData_Len , NULL, 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x19:
				phOsalNfc_LogBle("\n\r LTSM: Delecte VC Card with VC entry value greater than the maximunm card supported\n");
				status = SeApi_MOP_DeleteVirtualCard("TestMain", 20, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x1A:
				phOsalNfc_LogBle("\n\r LTSM: Get VC List with Response Buffer as NULL\n");
				status = SeApi_MOP_GetVcList(NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x1B:
				phOsalNfc_LogBle("\n\r LTSM: Read Mifare Data with Vc Entry greater than the maximum number of cards\n");
				status = SeApi_MOP_ReadMifareData("TestMain", 20, Read_Vc, ReadVc_Len , pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x1C:
				phOsalNfc_LogBle("\n\r LTSM: Read Mifare Data with Read_VC Buffer as NULL\n");
				status = SeApi_MOP_ReadMifareData("TestMain", 1, NULL, ReadVc_Len , pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x1D:
				phOsalNfc_LogBle("\n\r LTSM: Activate VC Card with Maximum response length equal to 0\n");
				*pRespSize =0 ;
				status = SeApi_MOP_ActivateVirtualCard("TestMain" , 1, TRUE, FALSE, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x1E:
				phOsalNfc_LogBle("\n\r LTSM: Create Mifare VC with Response Buffer as NULL\n");
				status = SeApi_MOP_CreateVirtualCard("TestMain", vcData, vcData_Len , NULL, 0, NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x1F:
				phOsalNfc_LogBle("\n\r LTSM: Create Mifare VC with Response Buffer Length as NULL\n");
				status = SeApi_MOP_CreateVirtualCard("TestMain", vcData, vcData_Len , NULL, 0, pRespBuf, NULL);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x20:
				phOsalNfc_LogBle("\n\r LTSM: Get VC Status with Resp Buffer Length as NULL\n");
				status = SeApi_MOP_GetVcStatus("TestMain", 1, pRespBuf, NULL);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x21:
				phOsalNfc_LogBle("\n\r LTSM: Activate VC Card with Vc entry greater than the maximum value\n");
				*pRespSize = 1;
				status = SeApi_MOP_ActivateVirtualCard("TestMain" ,20, TRUE, FALSE, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x22:
				phOsalNfc_LogBle("\n\r LTSM: Activate VC with Response Buffer as NULL\n");
				status = SeApi_MOP_ActivateVirtualCard("TestMain" ,1, TRUE, FALSE, NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x23:
				phOsalNfc_LogBle("\n\r LTSM: Activate VC with Response Buffer Length as NULL\n");
				status = SeApi_MOP_ActivateVirtualCard("TestMain" ,1, TRUE, FALSE, pRespBuf, NULL);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x24:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC with MDAC Data Buffer Length as 0\n");
				status = SeApi_MOP_AddAndUpdateMdac("TestMain", 1 , MDAC , 0, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			    break;
			case 0x25:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC with Response Buffer as NULL\n");
				status = SeApi_MOP_AddAndUpdateMdac("TestMain", 1 , MDAC , MDAC_Len, NULL, pRespSize);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
			    break;
			case 0x26:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC with Response Buffer Length as NULL\n");
				status = SeApi_MOP_AddAndUpdateMdac("TestMain", 1 , MDAC , MDAC_Len, pRespBuf, NULL);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status == SEAPI_STATUS_INVALID_PARAM)?"SUCCESS":"FAILURE", status);
				break;
			case 0x27:
				phOsalNfc_LogBle("\n\r LTSM: Get VC List with Response Buffer Length as NULL\n");
				status = SeApi_MOP_GetVcList(pRespBuf, NULL);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x28:
				phOsalNfc_LogBle("\n\r LTSM: Delete VC with different App name\n");
				status = SeApi_MOP_DeleteVirtualCard("ApiTest", vcEntry, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n Delete VC Response: 0x%02X 0x%02X",pRespBuf[2], pRespBuf[3]);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x29:
				phOsalNfc_LogBle("\n\r LTSM: Activate VC with different App name\n");
				status = SeApi_MOP_ActivateVirtualCard("ApiTest", vcEntry, TRUE, FALSE, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n Activate Response: 0x%02X 0x%02X",pRespBuf[2], pRespBuf[3]);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0x2A:
				phOsalNfc_LogBle("\n\r LTSM: Add and Update MDAC VC with different App name\n");
				status = SeApi_MOP_AddAndUpdateMdac("ApiTest", vcEntry, MDAC, MDAC_Len, pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n Add and Update MDAC Response: 0x%02X 0x%02X",pRespBuf[2], pRespBuf[3]);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
                break;
			case 0x2B:
				phOsalNfc_LogBle("\n\r LTSM: Read VC with different App name\n");
				status = SeApi_MOP_ReadMifareData("ApiTest", vcEntry, Read_Vc, ReadVc_Len , pRespBuf, pRespSize);
				phOsalNfc_LogBle("\n Read VC Response: 0x%02X 0x%02X",pRespBuf[2], pRespBuf[3]);
				phOsalNfc_LogBle("\n\r LTSM: %s %d ", (status != SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status);
				break;
			case 0xFE:
				phOsalNfc_LogBle("\n\r LTSM: Deinitialize MOP\n");
				SeApi_MOP_Deinit();
                status = SEAPI_STATUS_OK;
				break;

			default:
				phOsalNfc_LogBle("\nSeApi_LTSM : Wrong TLV Value passed\n");
				break;
			}
			*pRespSize = 0x01;

			pRespBuf[0] = status;// to send status to companion device
			phOsalNfc_LogBle("\n ----------- END LTSM Negative API Testing -------------\n");
			break;
#endif /*MIFARE_OP_ENABLE*/
#endif /*TEST_STANDALONE == TRUE*/
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
		case FIRMWARE_NEGATIVE:
			phOsalNfc_LogBle("\n------------ START: Firmware Negative API Testing -------------\n");

			if(value[0] == 0x00)
			{
				phOsalNfc_LogBle("\n\r FW Download: FW Download with NFC Initialized\n");
				status = SeApi_FirmwareDownload(gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence);
			}
			else
			{
				status = SeApi_Shutdown();
				if(status == SEAPI_STATUS_OK)
				{
					if(value[0] == 0x01)
					{
						phOsalNfc_LogBle("\n\r FW Download: FW Buffer as NULL\n");
						status = SeApi_FirmwareDownload(gphDnldNfc_DlSeqSz, NULL);
					}
					else if(value[0] == 0x02)
					{
						phOsalNfc_LogBle("\n\r FW Download: FW Buffer Length as 0\n");
						status = SeApi_FirmwareDownload(0, gphDnldNfc_DlSequence);
					}
#if (ENABLE_SESSION_MGMT == TRUE)
					else if(value[0] == 0x03)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: NFC in Initialized state\n");
						SeApi_Init(&transactionOccurred, &sessionCb);
						status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
					}
					else if(value[0] == 0x04)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: Transaction Callback as NULL\n");
						status = SeApi_InitWithFirmware(NULL, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
					}
					else if(value[0] == 0x05)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: Session Callback as NULL\n");
						status = SeApi_InitWithFirmware(&transactionOccurred, NULL, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
					}
					else if(value[0] == 0x06)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: FW buffer Length as 0\n");
						status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, 0, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
					}
					else if(value[0] == 0x07)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: FW buffer as NULL\n");
						status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, NULL, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
					}
					else if(value[0] == 0x08)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: Recovery FW buffer Length as 0\n");
						status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, 0, gphDnldNfc_DlSequenceDummyFw);
					}
					else if(value[0] == 0x09)
					{
						phOsalNfc_LogBle("\n\r InitWithFirmware: Recovery FW buffer as NULL\n");
						status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, NULL);
					}
#endif /*ENABLE_SESSION_MGMT == TRUE*/
					else
					{
						phOsalNfc_LogBle("\nFirmware Download : Wrong Value passed\n");
					}
				}
			}
			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nFirmware Download Negative Scenario returns : SEAPI_STATUS_OK; Test Failed\n");
			}
			else
			{
				phOsalNfc_LogBle("\nFirmware Download returns : 0x%02X\n", status);
			}

			*pRespSize = 0x01;
			pRespBuf[0] = status;// to report companion device about the status of the test
#if (ENABLE_SESSION_MGMT == TRUE)
			SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
			SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
			phOsalNfc_LogBle("\n------------ END: Firmware Negative API Testing -------------\n");
			break;
#endif/*(FIRMWARE_DOWNLOAD_ENABLE == TRUE)*/

		case GET_ATR_NEGATIVE:
			phOsalNfc_LogBle("\n------------ START: GET_ATR Negative API Testing -------------\n");

			if(value[0] == 0x00)
			{
				phOsalNfc_LogBle("\n\rGet_Atr_Negative: Get Atr with Wired Mode Disabled\n");
				status = SeApi_WiredGetAtr(pRespBuf, pRespSize);
			}
			if(value[0] == 0x01)
			{
				phOsalNfc_LogBle("\n\rGet_Atr_Negative: Recv Buffer as Null\n");
				status = SeApi_WiredGetAtr(NULL, pRespSize);
			}
			else if(value[0] == 0x02)
			{
				*pRespSize = 0;
				phOsalNfc_LogBle("\n\rGet_Atr_Negative: Recv Buffer Size as 0\n");
				status = SeApi_WiredGetAtr(pRespBuf, pRespSize);
			}
			else if(value[0] == 0x03)
			{
				phOsalNfc_LogBle("\n\rGet_Atr_Negative: Recv Buffer Size as NULL\n");
				status = SeApi_WiredGetAtr(pRespBuf, NULL);
			}
			else
			{
				phOsalNfc_LogBle("\nGet_Atr_Negative : Wrong Value passed\n");
			}

			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nGet_Atr_Negative Negative Scenario returns : SEAPI_STATUS_OK; Test Failed\n");
			}
			else
			{
				phOsalNfc_LogBle("\nGet_Atr_Negative returns : 0x%02X\n", status);
			}

			*pRespSize = 0x01;
			pRespBuf[0] = status;// to report companion device about the status of the test
			phOsalNfc_LogBle("\n------------ END: GET_ATR Negative API Testing -------------\n");
			break;

		case WIRED_TRANSCEIVE_NEGATIVE:
			phOsalNfc_LogBle("\n-------- START: SeApi_WiredTransceive Negative API Testing --------\n");

			UINT8 *xmitBuffer = value + 1;
			UINT16 xmitBufferSize = length - 1;

			if(value[0] == 0x00)
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Transceive with Wired Mode Disabled\n");
				status = SeApi_WiredTransceive(xmitBuffer, xmitBufferSize, pRespBuf, RESP_BUFF_SIZE, pRespSize, 10000);
			}
			else if(value[0] == 0x01)
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Transceive with Input Buffer as NULL\n");
				status = SeApi_WiredTransceive(NULL, xmitBufferSize, pRespBuf, RESP_BUFF_SIZE, pRespSize, 10000);
			}
			else if(value[0] == 0x02)
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Transceive with Input Buffer Length as 0\n");
				status = SeApi_WiredTransceive(xmitBuffer, 0, pRespBuf, RESP_BUFF_SIZE, pRespSize, 10000);
			}
			else if(value[0] == 0x03)
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Wired Transceive with Resp Buffer as NULL\n");
				status = SeApi_WiredTransceive(xmitBuffer, xmitBufferSize, NULL, RESP_BUFF_SIZE, pRespSize, 10000);
			}
			else if(value[0] == 0x04)
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Wired Transceive with Resp Buffer Length as 0\n");
				status = SeApi_WiredTransceive(xmitBuffer, xmitBufferSize, pRespBuf, 0, pRespSize, 10000);
			}
			else if(value[0] == 0x05)
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Wired Transceive with Resp Actual Size as NULL\n");
				status = SeApi_WiredTransceive(xmitBuffer, xmitBufferSize, pRespBuf, RESP_BUFF_SIZE, NULL, 10000);
			}
			else
			{
				phOsalNfc_LogBle("\n\rTransceive Negative: Wrong value passed\n");
			}

			if(status == SEAPI_STATUS_OK)
			{
				phOsalNfc_LogBle("\nSeApi_WiredTransceive Negative Scenario returns : SEAPI_STATUS_OK; Test Failed\n: ");
			}
			else
			{
				phOsalNfc_LogBle("\nSeApi_WiredTransceive Negative Scenario returns 0x%02X", status);
			}
			*pRespSize = 0x01;
			pRespBuf[0] = status;/*To indicate status to the companion device*/
			phOsalNfc_LogBle("\n-------- END: SeApi_WiredTransceive Negative API Testing --------\n");
			break;

		case SEND_NCI_RAW_COMMAND:
			phOsalNfc_LogBle("\n-------- START: NCI Raw Command --------\n");
			UINT8 *cmd = value + 1;
			UINT16 cmdLength = length - 1;

			if(value[0] == 0x00)
			{
				phOsalNfc_LogBle("\n-------- NCI Raw Command API Testing --------\n");
				NciRawModeTlvHandler(subType, cmd, cmdLength,pRespSize, pRespBuf, pRespTagType);
			}
			else
			{
				phOsalNfc_LogBle("\n-------- START: NCI Raw Command Negative API Testing --------\n");
				if(value[0] == 0x01)
				{
					phOsalNfc_LogBle("\n-------- NCI Raw Command: NFC is in De-Initialized state ---------\n");
					status = SeApi_NciRawCmdSend(cmdLength, cmd, pRespSize, pRespBuf);
				}
				else if(value[0] == 0x02)
				{
					phOsalNfc_LogBle("\n-------- NCI Raw Command: Data buffer length as 0 ---------\n");
					status = SeApi_NciRawCmdSend(0, cmd, pRespSize, pRespBuf);
				}
				else if(value[0] == 0x03)
				{
					phOsalNfc_LogBle("\n-------- NCI Raw Command: Input Data buffer as NULL ---------\n");
					status = SeApi_NciRawCmdSend(cmdLength, NULL, pRespSize, pRespBuf);
				}
				else if(value[0] == 0x04)
				{
					phOsalNfc_LogBle("\n-------- NCI Raw Command: Response buffer Length as NULL ---------\n");
					status = SeApi_NciRawCmdSend(cmdLength, cmd, NULL, pRespBuf);
				}
				else if(value[0] == 0x05)
				{
					*pRespSize = 0;
					phOsalNfc_LogBle("\n-------- NCI Raw Command: Response buffer Length as 0 ---------\n");
					status = SeApi_NciRawCmdSend(cmdLength, cmd, pRespSize, pRespBuf);
				}
				else if(value[0] == 0x06)
				{
					phOsalNfc_LogBle("\n-------- NCI Raw Command: Response buffer as NULL ---------\n");
					status = SeApi_NciRawCmdSend(cmdLength, cmd, pRespSize, NULL);
				}
				else
				{
					phOsalNfc_LogBle("\nNCI Raw Command Negative Scenarios: Wrong value passed\n");
				}

				if(status == SEAPI_STATUS_OK)
				{
					phOsalNfc_LogBle("\nNCI RawCommand_Negative Scenario returns : SEAPI_STATUS_OK; Test Failed\n");
				}
				else
				{
					phOsalNfc_LogBle("\nNCI RawCommand_Negative Scenario returns : 0x%02X\n", status);
				}
				*pRespSize = 0x01;
				pRespBuf[0] = status;/*To indicate status to the companion device*/
				phOsalNfc_LogBle("\n-------- END: NCI Raw Command Negative API Testing --------\n");
			}
			break;

		case SEAPI_INIT_UNINITIALIZED:
			phOsalNfc_LogBle("\n-------- START: SeAPI Init Uninitialized Scenarios Tests --------\n");
			phTransitSettings_t settings = {0};
			tSeApi_SeEventsCallbacks seEventsCb = {0};
			phStackCapabilities_t stackCapabilities = {0};
			tMOP_Storage* pStore = {0};
			UINT8 *cmd_1 = value + 1;
			UINT16 cmd1_Length = length - 1;
			UINT8 *xmitBuffer_1 = value + 1;
			UINT16 xmitBuffer1_Size = length - 1;
			SeApi_UnSubscribeForSeEvents();
			status = SeApi_Shutdown();
			if(status == SEAPI_STATUS_OK)
			{
				if(value[0] == 0x00)
				{
					phOsalNfc_LogBle("\n-------- SeApi_GetStackCapabilities: NFC is in De-Initialized state ---------\n");
					status = SeApi_GetStackCapabilities(&stackCapabilities);
				}
				else if(value[0] == 0x01)
				{
					phOsalNfc_LogBle("\n-------- SeApi_ApplyTransitSettings: NFC is in De-Initialized state ---------\n");
					status = SeApi_ApplyTransitSettings(&settings, FALSE);
				}
				else if(value[0] == 0x02)
				{
					phOsalNfc_LogBle("\n-------- SeApi_SubscribeForSeEvents: NFC is in De-Initialized state ---------\n");
					status = SeApi_SubscribeForSeEvents(&seEventsCb);
				}
				else if(value[0] == 0x03)
				{
					phOsalNfc_LogBle("\n-------- SeApi_SeSelect: NFC is in De-Initialized state ---------\n");
					status = SeApi_SeSelect(SE_ESE);
				}
				else if(value[0] == 0x04)
				{
					phOsalNfc_LogBle("\n-------- SeApi_SeConfigure: NFC is in De-Initialized state ---------\n");
					status = SeApi_SeConfigure(0x00, 0x00);
				}
				else if(value[0] == 0x05)
				{
					phOsalNfc_LogBle("\n-------- SeApi_NciRawCmdSend: NFC is in De-Initialized state ---------\n");
					status = SeApi_NciRawCmdSend(cmd1_Length, cmd_1, pRespSize, pRespBuf);
				}
				else if(value[0] == 0x06)
				{
					phOsalNfc_LogBle("\n-------- SeApi_WiredEnable: NFC is in De-Initialized state ---------\n");
					status = SeApi_WiredEnable(TRUE);
				}
				else if(value[0] == 0x07)
				{
					phOsalNfc_LogBle("\n-------- SeApi_WiredResetEse: NFC is in De-Initialized state ---------\n");
					status = SeApi_WiredResetEse();
				}
				else if(value[0] == 0x09)
				{
					phOsalNfc_LogBle("\n-------- SeApi_WiredGetAtr: NFC is in De-Initialized state ---------\n");
					status = SeApi_WiredGetAtr(pRespBuf, pRespSize);
				}
				else if(value[0] == 0x0A)
				{
					phOsalNfc_LogBle("\n-------- SeApi_WiredTransceive: NFC is in De-Initialized state ---------\n");
					status = SeApi_WiredTransceive(xmitBuffer_1, xmitBuffer1_Size, pRespBuf, RESP_BUFF_SIZE, pRespSize, 10000);
				}
				else if(value[0] == 0x0B)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_Init: NFC is in De-Initialized state ---------\n");
					status = SeApi_MOP_Init(pStore);
				}
				else if(value[0] == 0x0C)
				{
					phOsalNfc_LogBle("\n-------- SeApi_LoaderServiceDoScript: NFC is in De-Initialized state ---------\n");
					status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, NULL, pRespSize, pRespBuf, hash_len, hash, pResp);
				}
				else
				{
					phOsalNfc_LogBle("\nSeAPI Init Uninitialized Scenarios Tests: Wrong value passed\n");
					status = SEAPI_STATUS_REJECTED;
				}

				if(status == SEAPI_STATUS_NOT_INITIALIZED)
				{
					phOsalNfc_LogBle("\nSeAPI Init Uninitialized Scenario returns : SEAPI_STATUS_NOT_INITIALIZED; Test Passed\n");
				}
				else
				{
					phOsalNfc_LogBle("\nSeAPI Init Uninitialized Scenario returns : 0x%02X; Test Failed\n", status);
				}
			}
			*pRespSize = 0x01;
			pRespBuf[0] = status;// to report companion device about the status of the test
			SeApi_Init(&transactionOccurred, &sessionCb);
			break;

		case SEAPI_NEGATIVE:
			phOsalNfc_LogBle("\n-------- START: SeAPI Negative Scenarios Tests --------\n");
			tMOP_Storage* pStore1 = {0};
			if(value[0] == 0x01)
			{
				phOsalNfc_LogBle("\n-------- SeApi_ApplyTransitSettings: SeAPI Negative Scenarios Tests ---------\n");
				status = SeApi_ApplyTransitSettings(NULL, FALSE);
			}
			else if(value[0] == 0x02)
			{
				phOsalNfc_LogBle("\n-------- SeApi_MOP_Init Twice: SeAPI Negative Scenarios Tests ---------\n");
				SeApi_MOP_Init(pStore1);
				status = SeApi_MOP_Init(pStore1);
			}
			else if(value[0] == 0x03)
			{
				phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit Twice: SeAPI Negative Scenarios Tests ---------\n");
				SeApi_MOP_Deinit();
				SeApi_MOP_Deinit();
				SeApi_MOP_Init(pStore1);
			}
			else if(value[0] == 0x04)
			{
				phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle: SeAPI Negative Scenarios Tests ---------\n");
				SeApi_MOP_Deinit();
				if(value[1] == 0x00)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_CreateVirtualCard ---------\n");
					status= SeApi_MOP_CreateVirtualCard("TestMain", vcData, vcData_Len , NULL, 0, pRespBuf, pRespSize);
				}
				else if (value[1] == 0x01)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_DeleteVirtualCard ---------\n");
					status= SeApi_MOP_DeleteVirtualCard("TestMain", 1, pRespBuf, pRespSize);
				}
				else if (value[1] == 0x02)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_ActivateVirtualCard ---------\n");
					status= SeApi_MOP_ActivateVirtualCard("TestMain", 0, TRUE, FALSE, pRespBuf, pRespSize);
				}
				else if (value[1] == 0x03)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_AddAndUpdateMdac ---------\n");
					status= SeApi_MOP_AddAndUpdateMdac("TestMain", 1 , NULL , MDAC_Len, pRespBuf, pRespSize);
				}
				else if (value[1] == 0x04)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_ReadMifareData ---------\n");
					status= SeApi_MOP_ReadMifareData("TestMain", 1, Read_Vc, ReadVc_Len, NULL, pRespSize);
				}
				else if (value[1] == 0x05)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_GetVcStatus ---------\n");
					status= SeApi_MOP_GetVcStatus("TestMain", 0,pRespBuf, pRespSize);
				}
				else if (value[1] == 0x06)
				{
					phOsalNfc_LogBle("\n-------- SeApi_MOP_DeInit VC LifeCycle Test: SeApi_MOP_GetVcList ---------\n");
					status= SeApi_MOP_GetVcList(pRespBuf, pRespSize);
				}
				SeApi_MOP_Init(pStore1);
			}
			else if(value[0] == 0x05)
			{
				phOsalNfc_LogBle("\n-------- SeApi_WiredResetEse: SeAPI Negative Scenarios Tests ---------\n");
				SeApi_WiredEnable(FALSE);
				status = SeApi_WiredResetEse();
				SeApi_WiredEnable(TRUE);
			}
			else if(value[0] == 0x06)
			{
				phOsalNfc_LogBle("\n-------- SeApi_GetStackCapabilities: SeAPI Negative Scenarios Tests ---------\n");
				status = SeApi_GetStackCapabilities(NULL);
			}
			else if(value[0] == 0x07)
			{
				phOsalNfc_LogBle("\n-------- SeApi_SeConfigure: eSE_TechMask and UICC_TechMask : Tech A ---------\n");
				status = SeApi_SeConfigure(SEAPI_TECHNOLOGY_MASK_A, SEAPI_TECHNOLOGY_MASK_A);
			}
			else
			{
				phOsalNfc_LogBle("\nSeAPI Negative Scenarios Tests: Wrong value passed\n");
			}
			*pRespSize = 0x01;
			pRespBuf[0] = status;// to report companion device about the status of the test
			break;

		default:
			phOsalNfc_LogBle("\nApiTestTlvHandler: Default case; Wrong Sub-Type sent");
			*pRespSize = 0x01;
			pRespBuf[0] = SEAPI_STATUS_REJECTED;
	}
}
#endif /*API_TEST_ENABLE == TRUE*/
#endif
