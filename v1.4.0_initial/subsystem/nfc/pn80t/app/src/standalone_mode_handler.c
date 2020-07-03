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

/* Header files */

#include <ctype.h>
#include <WearableCoreSDK_BuildConfig.h>
#ifdef NFC_UNIT_TEST
#include <stdlib.h>
#include "FreeRTOS.h"
#include "nci_utils.h"
#include <phOsalNfc_Log.h>
#include <phOsalNfc_Thread.h>
#include "SeApi.h"
#include "UserData_Storage.h"
#include "debug_settings.h"
#include "wearable_platform_int.h"
//#include "lpc_res_mgr.h"

#if (TEST_STANDALONE == TRUE)
extern const uint8_t LS_CreateSD[];
extern const int32_t LS_CreateSD_len;
extern const uint8_t LS_LoadEcho[];
extern const int32_t LS_LoadEcho_len;
extern const uint8_t LS_InstallEcho[];
extern const int32_t LS_InstallEcho_len;
extern const uint8_t LS_DeleteEcho[];
extern const int32_t LS_DeleteEcho_len;
extern const uint8_t vcData[];
extern const uint8_t Read_Vc[];
extern const uint8_t MDAC[];
extern const uint16_t MDAC_Len;
extern const uint16_t ReadVc_Len;
extern const uint16_t vcData_Len;
#endif

/*
 * Called by nxp low power implementation wrapper.
 */
extern void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );
/*
 *
UINT16 (*cbPreLoad)(UINT8*, UINT16);// called before MOP attempts to load from the buffer, should return the amount of data in the buffer
                                    // Input is the buffer be read from and the size of the buffer
void (*cbPostLoad)(UINT8*, UINT16); // called after MOP loads from the buffer, receiving a 0 indicates a failure reading from the buffer, non-zero indicates how much was read from the buffer
                                    // Input is the buffer read from and the amount read
void (*cbPreStore)(UINT8*, UINT16); // called before MOP attempts to store data into the buffer, input parameter is the amount of data expected to be stored
                                    // Input is the buffer we're writing to and the amount to write
void (*cbPostStore)(UINT8*, UINT16);// called after the MOP stores into the buffer, input parameter is the amount of data actually stored
*/
//extern UINT16 cbPreLoad(UINT8* buf, UINT16 bufSize);
//extern void cbPostLoad(UINT8* buf, UINT16 dataSize);
//extern void cbPreStore(UINT8* buf, UINT16 dataSize);
//extern void cbPostStore(UINT8* buf, UINT16 dataSize);

extern INT32 sessionRead(UINT8* data, UINT16 numBytes);
extern INT32 sessionWrite(const UINT8* data, UINT16 numBytes);

#if (LS_ENABLE == TRUE)
extern const unsigned char hash[];
extern const int32_t hash_len;
#endif

#if(MIFARE_OP_ENABLE == TRUE)
//UINT8* gMOPBuffer;
extern const unsigned char TC_5_2_1_3[];
extern const int32_t TC_5_2_1_3_len;
extern const unsigned char TC_5_9_1_3[];
extern const int32_t TC_5_9_1_3_len;
extern const unsigned char TC_5_10_1_3[];
extern const int32_t TC_5_10_1_3_len;
extern const unsigned char HyattVCCreateData[];
extern const int32_t HyattVCCreateData_len;
extern const unsigned char SonyVCCreateData[];
extern const int32_t SonyVCCreateData_len;
extern const unsigned char DF_NP_4K_7U_AES [];
extern const int32_t DF_NP_4K_7U_AES_len;
extern const unsigned char DF_P_4K_7U_AES [];
extern const int32_t DF_P_4K_7U_AES_len;
extern const unsigned char DF_PERSO [];
extern const int32_t DF_PERSO_len;
extern const unsigned char DF_MDAC [];
extern const int32_t DF_MDAC_len;
extern const unsigned char DF_READ [];
extern const int32_t DF_READ_len;
tJBL_STATUS MOP_setCurrentPackageInfo(const char *packageName);
#endif/*MIFARE_OP_ENABLE*/

void SeApi_Test_Task(void *pParams)
{

    int sel = 0;
    unsigned char rawTestBuf[] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x62, 0x01, 0x01};
    unsigned short rawTestBufLen = 8;
#ifdef NXP_HW_SELF_TEST
    AntennaTest_t results;
#endif
    unsigned char pRespBuf[4200];/* Increased from 2000 to handle 4k bytes data read from MOP_ReadMifareData() */
    unsigned short respSize = sizeof(pRespBuf);
    int i;
#if (TEST_STANDALONE == TRUE)
    unsigned char pResp[4];
    unsigned short CardAVCNumber = 0;
    unsigned short CardHyattVCNumber = 0;
    int cnt = 0;
    UINT16 vcEntryNumber;
#endif
    phTransitSettings_t settings = {0};
    tSEAPI_STATUS status;
    phStackCapabilities_t stackCapabilities;

    /*
     * Run at 12MHz clock frequency
     */
    //ResMgr_EnterNormalMode(TRUE);

#if !defined(DEBUG) && defined(NFC_UNIT_TEST) && !defined(LOGS_TO_CONSOLE)
    Board_Debug_Init();
#endif
    phOsalNfc_Log("--------------------------------------------------------------------\n");
    phOsalNfc_Log("---------------- Wearable Core version 2.08.00 ----------------------\n");
    phOsalNfc_Log("------------- LibNfcTest v2.08.00 - test app libnfc_nci -------------\n");
    phOsalNfc_Log("--------------------------------------------------------------------\n");

    status = transitSettingRead((UINT8*)&settings, sizeof(settings));
    if(status != SEAPI_STATUS_OK)
    {
        settings.cfg_blk_chk_enable = 1;
        settings.rfSettingModified = 0;
    }
    phNxpNciHal_set_transit_settings(&settings);
#if (ENABLE_SESSION_MGMT == TRUE)
    tSeApi_SessionSaveCallbacks sessionCb;
    sessionCb.writeCb = sessionWrite;
    sessionCb.readCb = sessionRead;
#endif
    while (1)
    {
        status = SEAPI_STATUS_FAILED;
#if defined(DEBUG) || defined(NFC_UNIT_TEST) /* In Debug build or 'Nfc Unit Test in Release build'*/
        phOsalNfc_Log("\n****************************************\n");
        phOsalNfc_Log("1. Initialize NFCC\n");
        phOsalNfc_Log("2. DeInitialize NFCC\n");
        phOsalNfc_Log("3. NFC all NFCEE off\n");
        phOsalNfc_Log("4. NFC eSE Virtual Operation\n");
        phOsalNfc_Log("5. NFC UICC Virtual Operation\n");
        phOsalNfc_Log("6. ConfigAPI: NFC all NFCEE off\n");
        phOsalNfc_Log("7. ConfigAPI: NFC eSE Virtual Operation\n");
        phOsalNfc_Log("8. ConfigAPI: NFC UICC Virtual Operation\n");
        phOsalNfc_Log("9. ConfigAPI: NFC combined virtual operation eSE:TechA, UICC:TechB\n");
        phOsalNfc_Log("****************************************\n");
        phOsalNfc_Log("10. NFC eSE Wired ENABLE\n");
        phOsalNfc_Log("11. NFC eSE get ATR\n");
        phOsalNfc_Log("12. NFC eSE Wired DISABLE\n");
        phOsalNfc_Log("****************************************\n");
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
        phOsalNfc_Log("20. Update PN66T/PN548 Firmware\n");
        phOsalNfc_Log("21. Update PN66T/PN548 with same Firmware\n");
        phOsalNfc_Log("22. Update PN66T/PN548 Firmware using InitWithFirmware\n");
#endif
        phOsalNfc_Log("****************************************\n");
        phOsalNfc_Log("30. LoaderService install test script\n");
        phOsalNfc_Log("31. LoaderService delete test script\n");
        phOsalNfc_Log("32. LoaderService test script in chunks\n");
        phOsalNfc_Log("****************************************\n");
        phOsalNfc_Log("40. Get Stack Capabilities\n");
        phOsalNfc_Log("41. Test SeApi_NciRawCmdSend command to NFCC\n");
        phOsalNfc_Log("42. Set Transit Settings for Beijing\n");
        phOsalNfc_Log("43. Set Transit Settings for Shenzhen\n");
        phOsalNfc_Log("44. Set Transit Settings for Guangzhou\n");
        phOsalNfc_Log("45. Set Default Transit Settings\n");
        phOsalNfc_Log("****************************************\n");
#if(MIFARE_OP_ENABLE == TRUE)
#if 0
        phOsalNfc_Log("49. Delete MOP backing file MOP.dat\n");
#endif
        phOsalNfc_Log("50. Get VC List\n");
        phOsalNfc_Log("51. Create MiFare Virtual Card A\n");
        phOsalNfc_Log("52. Create MiFare Virtual Card Hyatt\n");
        phOsalNfc_Log("53. Create MiFare Virtual Card Sony\n");
        phOsalNfc_Log("54. Create MiFare Virtual Card with Personalization\n");
        phOsalNfc_Log("55. Create MiFare DESFire VC - NoPerso - 4KB - 7byte UID - AES\n");
        phOsalNfc_Log("56. Create MiFare DESFire VC - withPerso - 4KB - 7byte UID - AES\n");
        phOsalNfc_Log("57. Set MDAC for Virtual Card @ VCEntryNumber - pre-condition: VC type must be Virtual Card A\n");
        phOsalNfc_Log("58. Read Virtual Card @ VCEntryNumber - pre-condition: VC type must be Virtual Card A, MDAC set\n");
        phOsalNfc_Log("59. Set MDAC for DESFire Virtual Card @ VCEntryNumber - pre-condition: VC type must be Desfire \n");
        phOsalNfc_Log("60. Read  DESFire Virtual Card @ VCEntryNumber - pre-condition: VC type must be Desfire; MDAC should be set\n");
        phOsalNfc_Log("****************************************\n");
        phOsalNfc_Log("70. Get VC Status for VC @ VCEntryNumber\n");
        phOsalNfc_Log("71. Activate VC using VCEntryNumber\n");
        phOsalNfc_Log("72. Activate Virtual Card @ VCEntryNumber in Concurrent Mode (If VC is MFC, it should be VC A/Hyatt)\n");
        phOsalNfc_Log("73. Deactivate VC using VCEntryNumber\n");
        phOsalNfc_Log("74. Delete VC using VCEntryNumber\n");
#endif
#ifdef NXP_HW_SELF_TEST
        phOsalNfc_Log("****************************************\n");
        phOsalNfc_Log ("90. TEST - SWP 1\n");
        phOsalNfc_Log ("91. TEST - SWP 2\n");
        phOsalNfc_Log ("92. TEST - Antenna Measurement test\n");
        phOsalNfc_Log ("93. TEST - Pins\n");
#endif /* NXP_HW_SELF_TEST */
        phOsalNfc_Log("****************************************\n");
        phOsalNfc_Log("0. Exit Standalone Mode Test application\n");
        phOsalNfc_Log("****************************************\n");

        phOsalNfc_Log("Select the option: ");
        sel = phOsalNfc_ScanU32();
#else /* defined(DEBUG) || defined(NFC_UNIT_TEST) */

        if(sel == 0) /* First iteration of the loop */
        {
            sel = 1; /* Perform SeApi_Init */
        }
        else if(sel == 1) /* Init Done */
        {
            sel = 10; /* Set to Wired mode */
        }
        else if(sel == 10) /* In Wired Mode */
        {
            sel = 11; /* Get ATR */
        }
        else if(sel == 11) /* Got ATR */
        {
            sel = 13; /* Disable wired mode */
        }
        else if(sel == 13) /* Wired Mode Disabled */
        {
            sel = 4; /* Set to Virtual Mode */
        }
        else if(sel == 4) /* In Virtual mode */
        {
            phOsalNfc_Delay(5*60*1000); /* Allow 5 mins for Reader to read the tag */
            sel = 0; /* Exit the test application */
        }
#endif /* defined(DEBUG) || defined(NFC_UNIT_TEST) */
        switch (sel)
        {
            case 0:
                phOsalNfc_Log("\nExiting test application\n");
                goto clean_and_return;
            break;

            case 1:
                phOsalNfc_Log("\n-------------------------------------------\n LIBNFC_NCI_INIT  \n-------------------------------------------\n");
                GeneralState |= NFCINIT_MASK;
#if (ENABLE_SESSION_MGMT == TRUE)
                status = SeApi_Init(&transactionOccurred, &sessionCb);
#else
                status = SeApi_Init(&transactionOccurred, NULL);
#endif
                GeneralState &= ~NFCINIT_MASK;                  // Clear NFCINIT bit
                if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_UNEXPECTED_FW)
                {
                       NVIC_SystemReset();
                }
                phOsalNfc_Log("-------------------------------------------\n END LIBNFC_NCI_INIT \n-------------------------------------------\n");
            break;

            case 2:
                phOsalNfc_Log("\n-------------------------------------------\n LIBNFC_NCI_DEINIT  \n-------------------------------------------\n");
                GeneralState |= NFCINIT_MASK;
                status = SeApi_Shutdown();
                GeneralState &= ~NFCINIT_MASK;                  // Clear NFCINIT bit
                phOsalNfc_Log("-------------------------------------------\n END LIBNFC_NCI_DEINIT \n-------------------------------------------\n");
            break;

            case 3:
                phOsalNfc_Log("\n-------------------------------------------\n NFC all NFCEE Virtual Operation off \n-------------------------------------------\n");
                status = SeApi_SeSelect(SE_NONE);
                phOsalNfc_Log("-------------------------------------------\n END NFC all NFCEE Virtual Operation off \n-------------------------------------------\n");
            break;

            case 4:
                phOsalNfc_Log("----------------------------------------\n NFC eSE Virtual Operation \n-------------------------------------\n");
                status = SeApi_SeSelect(SE_ESE);
                if(status == SEAPI_STATUS_OK)
                {
                    //Board_LED_Set(0, TRUE); /* Turn On LED #0 */
                }
                phOsalNfc_Log("-------------------------------------------\n END eSE Virtual Operation \n-------------------------------------------\n");
            break;

            case 5:
                phOsalNfc_Log("\n-------------------------------------------\n NFC UICC Virtual Operation \n-------------------------------------------\n");
                status = SeApi_SeSelect(SE_UICC);
                phOsalNfc_Log("-------------------------------------------\n END NFC UICC Virtual Operation \n-------------------------------------------\n");
               break;

            case 6:
                phOsalNfc_Log("\n-------------------------------------------\n ConfigAPI: NFC all NFCEE Virtual Operation off \n-------------------------------------------\n");
                status = SeApi_SeConfigure(0x00, 0x00);
                phOsalNfc_Log("-------------------------------------------\n ConfigAPI: END NFC all NFCEE Virtual Operation off \n-------------------------------------------\n");
            break;

            case 7:
                phOsalNfc_Log("----------------------------------------\n ConfigAPI: NFC eSE Virtual Operation \n-------------------------------------\n");
                status = SeApi_SeConfigure(SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B, 0x00);
                phOsalNfc_Log("-------------------------------------------\n ConfigAPI: END eSE Virtual Operation \n-------------------------------------------\n");
            break;

            case 8:
                phOsalNfc_Log("\n-------------------------------------------\n ConfigAPI: NFC UICC Virtual Operation \n-------------------------------------------\n");
                status = SeApi_SeConfigure(0x00, SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B | SEAPI_TECHNOLOGY_MASK_F);
                phOsalNfc_Log("-------------------------------------------\n ConfigAPI: END NFC UICC Virtual Operation \n-------------------------------------------\n");
               break;

            case 9:
                phOsalNfc_Log("\n-------------------------------------------\n ConfigAPI: combined virtual operation eSE:TechA, UICC:TechB \n-------------------------------------------\n");
                status = SeApi_SeConfigure(SEAPI_TECHNOLOGY_MASK_A, SEAPI_TECHNOLOGY_MASK_B);
                phOsalNfc_Log("-------------------------------------------\n ConfigAPI: combined virtual operation eSE:TechA, UICC:TechB \n-------------------------------------------\n");
               break;

            case 10:
                phOsalNfc_Log("\n-------------------------------------------\n NFC eSE Wired ENABLE \n-------------------------------------------\n");
                status = SeApi_WiredEnable(TRUE);
                phOsalNfc_Log("-------------------------------------------\n END NFC eSE Wired ENABLE \n-------------------------------------------\n");
                break;

            case 11:
                phOsalNfc_Log("\n-------------------------------------------\n NFC eSE get ATR \n-------------------------------------------\n");
                respSize = sizeof(pRespBuf);
                status = SeApi_WiredGetAtr(pRespBuf, &respSize);
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("ATR length: %d, ATR: ", respSize);
                    for(i=0; i<respSize; i++)
                    {
                        phOsalNfc_Log("0x%02X ", pRespBuf[i]);
                    }
                    phOsalNfc_Log("\n");
                }
                phOsalNfc_Log("-------------------------------------------\n END NFC eSE get ATR \n-------------------------------------------\n");
                break;
            case 12:
                phOsalNfc_Log("\n-------------------------------------------\n NFC eSE Wired DISABLE \n-------------------------------------------\n");
                status = SeApi_WiredEnable(FALSE);
                phOsalNfc_Log("-------------------------------------------\n END NFC eSE Wired DISABLE \n-------------------------------------------\n");
                break;

#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
            case 20:
                phOsalNfc_Log("\n-------------------------------------------\n Update PN66T/PN548 Firmware \n-------------------------------------------\n");
                GeneralState |= NFCINIT_MASK;
                phOsalNfc_Log("shutting down the stack\n");
                status = SeApi_Shutdown();
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("ShutDown Success; Starting firmware download\n");

                    status = SeApi_FirmwareDownload(gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence);
                    if(status == SEAPI_STATUS_OK)
                    {
                        phOsalNfc_Log("Firmware Download Success; Restarting the stack\n");
#if (ENABLE_SESSION_MGMT == TRUE)
                        status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
                        status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
                        if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_UNEXPECTED_FW)
                        {
                             phOsalNfc_Log("SeApi_Init Failed with %d, retrying ...\n",status);
                             SeApi_Int_CleanUpStack();
#if (ENABLE_SESSION_MGMT == TRUE)
                             status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
                             status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
                             if(status != SEAPI_STATUS_OK)
                             {
                                 phOsalNfc_Log("SeApi_Init Failed again with %d,...soft resetting MCU\n",status);
                                 //NVIC_SystemReset();
                             }
                        }
                    }
                    else
                    {
                        phOsalNfc_Log("Firmware Download Failed; Exiting\n");
                    }
                }
                else
                {
                    phOsalNfc_Log("ShutDown Failed; Exiting\n");
                }

                GeneralState &= ~NFCINIT_MASK;
                phOsalNfc_Log("-------------------------------------------\n END Update Firmware \n-------------------------------------------\n");
                break;
            case 21:
                phOsalNfc_Log("\n-------------------------------------------\n Update PN66T/PN548 with same firmware Firmware \n-------------------------------------------\n");
                GeneralState |= NFCINIT_MASK;
                status = SeApi_ForcedFirmwareDownload();
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Forced Firmware Download Success; Restarting the stack\n");
#if (ENABLE_SESSION_MGMT == TRUE)
                    status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
                    status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
                    if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_UNEXPECTED_FW)
                    {
                        phOsalNfc_Log("SeApi_Init Failed with %d, retrying ...\n",status);
                        SeApi_Int_CleanUpStack();
#if (ENABLE_SESSION_MGMT == TRUE)
                        status = SeApi_Init(&transactionOccurred, &sessionCb);
#else /* ENABLE_SESSION_MGMT == TRUE */
                        status = SeApi_Init(&transactionOccurred, NULL);
#endif /* ENABLE_SESSION_MGMT == TRUE */
                        if(status != SEAPI_STATUS_OK)
                        {
                            phOsalNfc_Log("SeApi_Init Failed again with %d,...soft resetting MCU\n",status);
                            //NVIC_SystemReset();
                        }
                    }
                }
                else
                    phOsalNfc_Log("SeApi_ForcedFirmwareDownload Failed\n");

                GeneralState &= ~NFCINIT_MASK;
                phOsalNfc_Log("-------------------------------------------\n END Update Same Firmware \n-------------------------------------------\n");
                break;
            case 22:
                phOsalNfc_Log("\n-------------------------------------------\n Update PN66T/PN548 Firmware: InitWithFirmware \n-------------------------------------------\n");
                GeneralState |= NFCINIT_MASK;
                phOsalNfc_Log("shutting down the stack\n");
				status = SeApi_Shutdown();
				if(status == SEAPI_STATUS_OK)
				{
#if (ENABLE_SESSION_MGMT == TRUE)
                    status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
#else /* ENABLE_SESSION_MGMT == TRUE */
                    status = SeApi_InitWithFirmware(&transactionOccurred, NULL, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
#endif /* ENABLE_SESSION_MGMT == TRUE */
				}
				else
				{
					phOsalNfc_Log("ShutDown Failed; Exiting\n");
				}
                GeneralState &= ~NFCINIT_MASK;
                phOsalNfc_Log("-------------------------------------------\n END Update Firmware: InitWithFirmware \n-------------------------------------------\n");
                break;
#endif /* FIRMWARE_DOWNLOAD_ENABLE == TRUE */

#if (LS_ENABLE == TRUE)
            case 30:
                phOsalNfc_Log("\n-------------------------------------------\n LoaderService test Install applet \n-------------------------------------------\n");
                //----------------------------------------------
                // SystemTest1.txt -- loads a script
                // 1.       test_createsd_encrypted.txt
                // 2.       test_loadecho_encrypted.txt
                // 3.       test_installecho_encrypted.txt

#if (TEST_STANDALONE == TRUE)
               respSize = sizeof(pRespBuf);
                status = SeApi_LoaderServiceDoScript(LS_CreateSD_len, LS_CreateSD, &respSize, pRespBuf, hash_len, hash, pResp);
                phOsalNfc_Log("\n\nLS Script Output length: %d\n", respSize);
                for(i = 0; i< respSize; i++)
                    phOsalNfc_Log("%c",*(pRespBuf+i));
                phOsalNfc_Log("\n\r LOADER SERVICE: Create SD %s %d SW: 0x%02X 0x%02X 0x%02X 0x%02X\n", (status == SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status,pResp[0], pResp[1], pResp[2], pResp[3]);

                respSize = sizeof(pRespBuf);
                status = SeApi_LoaderServiceDoScript(LS_LoadEcho_len, LS_LoadEcho, &respSize, pRespBuf, hash_len, hash, pResp);
                phOsalNfc_Log("\n\nLS Script Output length: %d\n", respSize);
                for(i = 0; i< respSize; i++)
                    phOsalNfc_Log("%c",*(pRespBuf+i));
                phOsalNfc_Log("\n\r LOADER SERVICE: Load Echo %s %d SW: 0x%02X 0x%02X 0x%02X 0x%02X\n", (status == SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status,pResp[0], pResp[1], pResp[2], pResp[3] );

                respSize = sizeof(pRespBuf);
                status = SeApi_LoaderServiceDoScript(LS_InstallEcho_len, LS_InstallEcho, &respSize, pRespBuf, hash_len, hash, pResp);
                phOsalNfc_Log("\n\nLS Script Output length: %d\n", respSize);
                for(i = 0; i< respSize; i++)
                    phOsalNfc_Log("%c",*(pRespBuf+i));
                phOsalNfc_Log("\n\r LOADER SERVICE: Install Echo %s %d SW: 0x%02X 0x%02X 0x%02X 0x%02X\n", (status == SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status,pResp[0], pResp[1], pResp[2], pResp[3] );

#else /* TEST_STANDALONE == TRUE */
 // file system is not supported
#endif /* TEST_STANDALONE == TRUE */
                phOsalNfc_Log("-------------------------------------------\n END LoaderService test Install applet \n-------------------------------------------\n");
                break;

            case 31:
                phOsalNfc_Log("\n-------------------------------------------\n LoaderService test Delete applet \n-------------------------------------------\n");
                //----------------------------------------------
                // SystemTest2.txt -- loads a script
                // 4.       test_deleteall_encrypted.txt
#if (TEST_STANDALONE == TRUE)
                respSize = sizeof(pRespBuf);
                status = SeApi_LoaderServiceDoScript(LS_DeleteEcho_len, LS_DeleteEcho, &respSize, pRespBuf, hash_len, hash, pResp);
                phOsalNfc_Log("\n\nLS Script Output length: %d\n", respSize);
                for(i = 0; i< respSize; i++)
                    phOsalNfc_Log("%c",*(pRespBuf+i));
                phOsalNfc_Log("\n\r LOADER SERVICE: Delete Echo %s %d SW: 0x%02X 0x%02X 0x%02X 0x%02X\n", (status == SEAPI_STATUS_OK)?"SUCCESS":"FAILURE", status,pResp[0], pResp[1], pResp[2], pResp[3] );
#else  /*TEST_STANDALONE == TRUE*/
                // file system is not supported
#endif /* TEST_STANDALONE == TRUE */
                phOsalNfc_Log("-------------------------------------------\n END LoaderService test Delete applet \n-------------------------------------------\n");
                break;
            case 32:
			{
				UINT8 i;
				UINT16 j;
				UINT8 *buffer, pChunkResp[LS_RSP_CHUNK_SIZE + CMD_CHUNK_FOOTER_SIZE];
				UINT32 offset = 0;
				UINT32 lsSript_len[] = {LS_CreateSD_len, LS_LoadEcho_len, LS_InstallEcho_len, LS_DeleteEcho_len};
				UINT32 script_len;
				UINT16 chunkRespLen;
				const UINT8 *pLsScripts[] = {LS_CreateSD, LS_LoadEcho, LS_InstallEcho, LS_DeleteEcho};
				const UINT8 *pScData;
				UINT32 chunk_len;
				phStackCapabilities_t stackCapabilities;
				/* Allocate the buffer */

				buffer = (UINT8 *)phOsalNfc_GetMemory(1024 + CMD_CHUNK_FOOTER_SIZE);
				status = SeApi_GetStackCapabilities(&stackCapabilities);
				for(i = 0; i < 4; i++) {
					offset = 0;
					pScData = pLsScripts[i];
					script_len = lsSript_len[i];
					while(offset < script_len) {
						/* Get the chunk length */
						chunk_len = ((script_len - offset) > 1024) ? 1024 : (script_len - offset);
						/* Read the data */
						(void)phOsalNfc_MemCopy(buffer, &pScData[offset], chunk_len);

						/* Loader service process
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
				}
				phOsalNfc_FreeMemory(buffer);
			}
			break;
            case 40:
                phOsalNfc_Log("\n ----------- START: SeApi Get Stack Capabilities -------------\n");
                status = SeApi_GetStackCapabilities(&stackCapabilities);
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("\nSeApi_GetStackCapabilities: SUCCESS\n");
                    phOsalNfc_Log("SDK Version: 0x%06X\n", stackCapabilities.sdk_ver);
                    phOsalNfc_Log("MiddleWare Version: 0x%06X\n", stackCapabilities.middleware_ver);
                    phOsalNfc_Log("Firmware Version: 0x%06X\n", stackCapabilities.firmware_ver);
                    phOsalNfc_Log("NCI Version: 0x0%X\n", stackCapabilities.nci_ver);
                    phOsalNfc_Log("eSE Version: 0x%X\n", stackCapabilities.ese_ver);
                    phOsalNfc_Log("JCOP Version: 0x%04X\n", stackCapabilities.jcop_ver);
                    phOsalNfc_Log("LS Version:0x%06X\n", stackCapabilities.ls_ver);
                    phOsalNfc_Log("LTSM Version: 0x%X\n", stackCapabilities.ltsm_ver);
                    phOsalNfc_Log("Command Chunk Size: %d(dec)\n", stackCapabilities.chunk_sizes.CmdBufSize);
                    phOsalNfc_Log("Response Chunk Size: %d(dec)\n", stackCapabilities.chunk_sizes.RspBufSize);
                }
                break;

            case 41:
                phOsalNfc_Log("\n-------------------------------------------\n Test SeApi_NciRawCmdSend to NFCC \n-------------------------------------------\n");
                respSize = 32;
                phOsalNfc_Log("SeApi_NciRawCmdSend: Sending data [%d bytes] - ", rawTestBufLen);
                for (i=0; i<rawTestBufLen; i++)
                {
                    phOsalNfc_Log("0x%02X ", *(rawTestBuf + i));
                }
                phOsalNfc_Log("\n");

                status = SeApi_NciRawCmdSend(rawTestBufLen, rawTestBuf, &respSize, pRespBuf);
                if(SEAPI_STATUS_OK == status)
                {
                    phOsalNfc_Log("SeApi_NciRawCmdSend: SUCCESS\n");
                    phOsalNfc_Log("Response: [%d bytes] - ", respSize);
                    for (i=0; i<respSize; i++)
                    {
                        phOsalNfc_Log("0x%02X ", *(pRespBuf + i));
                    }
                    phOsalNfc_Log("\n");
                }
                else
                {
                    phOsalNfc_Log("SeApi_NciRawCmdSend: FAILED\n");
                }
                phOsalNfc_Log("-------------------------------------------\n END Test SeApi_NciRawCmdSend to NFCC \n-------------------------------------------\n");
                break;

            case 42:
                phOsalNfc_Log("\n-------------------------------------------\n Set Transit Settings for Beijing \n-------------------------------------------\n");
                settings.ese_listen_tech_mask = SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B;
                settings.cfg_blk_chk_enable = 0x00;
                status = SeApi_ApplyTransitSettings(&settings, FALSE);
                phOsalNfc_Log("-------------------------------------------\n END Set Transit Settings for Beijing \n-------------------------------------------\n");
                break;

            case 43:
                phOsalNfc_Log("\n-------------------------------------------\n Set Transit Settings for Shenzhen \n-------------------------------------------\n");
                settings.ese_listen_tech_mask = SEAPI_TECHNOLOGY_MASK_A | SEAPI_TECHNOLOGY_MASK_B;
                settings.cfg_blk_chk_enable = 0x01;
                status = SeApi_ApplyTransitSettings(&settings, FALSE);
                phOsalNfc_Log("-------------------------------------------\n END Set Transit Settings for Shenzhen\n-------------------------------------------\n");
                break;

            case 44:
                phOsalNfc_Log("\n-------------------------------------------\n Set Transit Settings for Guangzhou \n-------------------------------------------\n");
                settings.ese_listen_tech_mask = SEAPI_TECHNOLOGY_MASK_A;
                settings.cfg_blk_chk_enable = 0x01;
                status = SeApi_ApplyTransitSettings(&settings, FALSE);
                phOsalNfc_Log("-------------------------------------------\n END Set Transit Settings for Guangzhou \n-------------------------------------------\n");
                break;

            case 45:
                phOsalNfc_Log("\n-------------------------------------------\n Set Default Transit Settings\n-------------------------------------------\n");
                status = SeApi_ApplyTransitSettings(&settings, TRUE);
                phOsalNfc_Log("-------------------------------------------\n END Set Default Transit Settings \n-------------------------------------------\n");
                break;
#endif
#if (TEST_STANDALONE == TRUE)
#if(MIFARE_OP_ENABLE == TRUE)

            case 49:
#if 0
                phOsalNfc_Log("\n-------------------------------------------\n Delete MOP Data File \n-------------------------------------------\n");
                status = unlink ("/data/data/MOP.dat");
                if(status == 0)
                    phOsalNfc_Log("/data/data/MOP.dat successfully deleted\n");
                phOsalNfc_Log("-------------------------------------------\n");
#else
                phOsalNfc_Log("unlink API is not supported by FreeRTOS\n");
#endif
                break;

            case 50:
                phOsalNfc_Log("\n-------------------------------------------\n Get VC List \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_GetVcList(pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC List SUCCESS: ");
                    if(respSize == 4)
                    {
                        if(pRespBuf[2] == 0x6A && pRespBuf[3] == 0x88)
                        {
                            phOsalNfc_Log(" no cards present\n");
                        }
                    }
                }
                else
                {
                    phOsalNfc_Log("VC List FAILED");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");
                i = 0;
                cnt = 0;
                while (i < respSize) {
                    if ((pRespBuf[i + 0] == 0x61)
                     && (pRespBuf[i + 2] == 0x40)
                     && (pRespBuf[i + 3] == 0x02))
                    {
                        cnt++;
                        phOsalNfc_Log("%d card present at VC Entry No: 0x%02X\n",cnt,pRespBuf[i + 5]);
                    }
                    i++;
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("------------------------------------------\n");
                break;

            case 51:
                phOsalNfc_Log("\n-------------------------------------------\n MiFare Create Virtual Card A \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_CreateVirtualCard("TestMain", TC_5_2_1_3, TC_5_2_1_3_len, NULL, 0, pRespBuf, &respSize);
                CardAVCNumber = pRespBuf[3];

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Create SUCCESS: VCEntry Number = %d\n", CardAVCNumber);
                }
                else
                {
                    phOsalNfc_Log("VC Create FAILED: \n");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 52:
                phOsalNfc_Log("\n-------------------------------------------\n Create MiFare Virtual Card Hyatt \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_CreateVirtualCard("TestMain", HyattVCCreateData, HyattVCCreateData_len, NULL, 0, pRespBuf, &respSize);

                CardHyattVCNumber = pRespBuf[3];

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Create SUCCESS:  VCEntryNumber = %d\n", CardHyattVCNumber);
                }
                else
                {
                    phOsalNfc_Log("VC Create FAILED:\n");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 53:
                phOsalNfc_Log("\n-------------------------------------------\n Create MiFare Virtual Card Sony \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_CreateVirtualCard("TestMain", SonyVCCreateData, SonyVCCreateData_len, NULL, 0, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Create SUCCESS:  VCEntryNumber = %d\n", pRespBuf[3]);
                }
                else
                {
                    phOsalNfc_Log("VC Create FAILED:\n");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 54:
                phOsalNfc_Log("\n-------------------------------------------\n Create MiFare Virtual Card with Personalization \n-------------------------------------------\n");
                phOsalNfc_Log("NOT CURRENTLY IMPLEMENTED\n");
                phOsalNfc_Log("-------------------------------------------\n");
                break;

                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_CreateVirtualCard("TestMain", SonyVCCreateData, SonyVCCreateData_len, NULL, 0, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Create SUCCESS:  VCEntryNumber = %d\n", pRespBuf[3]);
                }
                else
                {
                    phOsalNfc_Log("VC Create FAILED:\n");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 55:
                phOsalNfc_Log("\n-------------------------------------------\n MiFare DESFire -  Create VC NoPerso - 4KB - 7byte UID - AES \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_CreateVirtualCard("TestMain", DF_NP_4K_7U_AES, DF_NP_4K_7U_AES_len, NULL, 0, pRespBuf, &respSize);

                CardAVCNumber = pRespBuf[3];

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Create SUCCESS: VCEntry Number = %d\n", CardAVCNumber);
                }
                else
                {
                    phOsalNfc_Log("VC Create FAILED: \n");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 56:
                phOsalNfc_Log("\n-------------------------------------------\n MiFare DESFire -  Create VC withPerso - 4KB - 7byte UID - AES \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_CreateVirtualCard("TestMain", DF_P_4K_7U_AES, DF_P_4K_7U_AES_len, DF_PERSO, DF_PERSO_len, pRespBuf, &respSize);

                CardAVCNumber = pRespBuf[3];

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Create SUCCESS: VCEntry Number = %d\n", CardAVCNumber);
                }
                else
                {
                    phOsalNfc_Log("VC Create FAILED: \n");
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 57:
                phOsalNfc_Log("\n-------------------------------------------\n Set MDAC for Virtual Card A \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: Pre-Condition VCEntryNumber is obtained during VC Creation. Use Same VCEntryNumber for this Test Case\n");
                phOsalNfc_Log("If VcEntryNumber of Pre-Condition VC is not known, FAIL this Test with random VcEntryNumber and Re-Try with Correct VcEntryNumber\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber for VC of type \"Virtual Card A\":");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nUpdating VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_AddAndUpdateMdac("TestMain", vcEntryNumber, TC_5_9_1_3, TC_5_9_1_3_len, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("MDAC configure at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("MDAC configure at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 58:
                phOsalNfc_Log("\n-------------------------------------------\n Read Virtual Card A after Set MDAC \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: Pre-Condition VCEntryNumber is obtained during VC Creation. Use Same VCEntryNumber for this Test Case\n");
                phOsalNfc_Log("If VcEntryNumber of Pre-Condition VC is not known, FAIL this Test with random VcEntryNumber and Re-Try with Correct VcEntryNumber\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber for VC of type \"Virtual Card A\":");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nReading VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_ReadMifareData("TestMain", vcEntryNumber, TC_5_10_1_3, TC_5_10_1_3_len, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Read VC at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("Read VC VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 59:
                phOsalNfc_Log("\n-------------------------------------------\n Set MDAC for DESFire Virtual Card @ VcEntryNumber \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: Pre-Condition VCEntryNumber is obtained during VC Creation. Use Same VCEntryNumber for this Test Case\n");
                phOsalNfc_Log("If VcEntryNumber of Pre-Condition VC is not known, FAIL this Test with random VcEntryNumber and Re-Try with Correct VcEntryNumber\n");
                phOsalNfc_Log("\nSpecify the Desfire VCEntryNumber to Set MDAC:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nSetting MDAC of Desfire Virtual Card @ VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_AddAndUpdateMdac("TestMain", vcEntryNumber, DF_MDAC, DF_MDAC_len, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("MDAC configure at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("MDAC configure at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 60:
                phOsalNfc_Log("\n-------------------------------------------\n Read DESFire Virtual Card @ VcEntryNumber \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: Pre-Condition VCEntryNumber is obtained during VC Creation. Use Same VCEntryNumber for this Test Case\n");
                phOsalNfc_Log("If VcEntryNumber of Pre-Condition VC is not known, FAIL this Test with random VcEntryNumber and Re-Try with Correct VcEntryNumber\n");
                phOsalNfc_Log("\nSpecify the Desfire VCEntryNumber to Read:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nReading Desfire Virtual Card @ VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_ReadMifareData("TestMain", vcEntryNumber, DF_READ, DF_READ_len, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("\nRead VC at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("\nRead VC at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 70:
                phOsalNfc_Log("\n-------------------------------------------\n Get VC Status for Virtual Card @ VCEntryNumber\n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: VCEntryNumber can be obtained from executing Test Case 50\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber to get Virtual Card Status:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nGetting status of Virtual Card @ VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status =  SeApi_MOP_GetVcStatus("TestMain", vcEntryNumber, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Get VC at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                    if(pRespBuf[3] == TRUE)
                    {
                        phOsalNfc_Log("VC Status at VCEntryNumber %d is ACTIVATED\n", vcEntryNumber);
                    }
                    else
                    {
                        phOsalNfc_Log("VC Status at VCEntryNumber %d is DEACTIVATED\n", vcEntryNumber);
                    }
                }
                else
                {
                    phOsalNfc_Log("Get VC at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");
                SeApi_MOP_Deinit();
                phOsalNfc_Log("------------------------------------------\n");
                break;

            case 71:
                phOsalNfc_Log("\n-------------------------------------------\n Activate VC at VCEntryNumber \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: VCEntryNumber can be obtained from executing Test Case 50\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber to Activate:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nActivating VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_ActivateVirtualCard("TestMain", vcEntryNumber, TRUE, FALSE, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Activate VC at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("Activate VC at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 72:
                phOsalNfc_Log("\n-------------------------------------------\n Activate Virtual Card @ VcEntryNumber in Concurrent Mode \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo:Desfire VC, MIFARE VC A or MIFARE VC HYATT can be activated in this mode\n");
                phOsalNfc_Log("If VcEntryNumber of such VC is not known, FAIL this Test with random VcEntryNumber and Re-Try with Correct VcEntryNumber\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber to Activate VC in Concurrent Mode:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nActivate Virtual Card @ VcEntryNumber %d in Concurrent Mode \n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_ActivateVirtualCard("TestMain", vcEntryNumber, TRUE, TRUE, pRespBuf, &respSize);


                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Concurrent Mode VC Activate at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("Concurrent Mode VC Activate at VCEntryNumber %d is FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 73:
                phOsalNfc_Log("\n-------------------------------------------\n Deactivate VC at VCEntryNumber \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: VCEntryNumber can be obtained from executing Test Case 50\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber to Deactivate:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nDeactivating VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_ActivateVirtualCard("TestMain", vcEntryNumber, FALSE, FALSE, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Deactivate VC at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("Deactivate VC at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

             case 74:
                phOsalNfc_Log("\n-------------------------------------------\n Delete VC at VCEntryNumber \n-------------------------------------------\n");
                phOsalNfc_Log("Initializing MOP\n");
                SeApi_MOP_Init(NULL);

                phOsalNfc_Log("\nInfo: VCEntryNumber can be obtained from executing Test Case 50\n");
                phOsalNfc_Log("\nSpecify the VCEntryNumber to Delete:");
                vcEntryNumber = phOsalNfc_ScanU32();
                phOsalNfc_Log("\nDeleting VCEntryNumber: %d\n", vcEntryNumber);
                respSize = sizeof(pRespBuf);
                status = SeApi_MOP_DeleteVirtualCard("TestMain", vcEntryNumber, pRespBuf, &respSize);

                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("VC Delete at VCEntryNumber %d is SUCCESS\n", vcEntryNumber);
                }
                else
                {
                    phOsalNfc_Log("VC Delete at VCEntryNumber %d FAILED\n", vcEntryNumber);
                }

                phOsalNfc_Log("response data size = %d\n", respSize);
                phOsalNfc_Log("response data: ");
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_Log("%02X ", pRespBuf[i]);
                }
                phOsalNfc_Log("\n");

                SeApi_MOP_Deinit();
                phOsalNfc_Log("-------------------------------------------\n");
                break;

#endif
#endif /* TEST_STANDALONE == TRUE */
#ifdef NXP_HW_SELF_TEST
            case 90:
                phOsalNfc_Log("\n-------------------------------------------\n TEST - SWP 1 \n-------------------------------------------\n");
                GeneralState |= HWSELF_TEST_MASK;
                status = SeApi_Test_SWP(0x01);
                GeneralState &= ~HWSELF_TEST_MASK;
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("SWP 1: SUCCESS\n");
                }
                else
                {
                    phOsalNfc_Log("SWP 1: FAILED\n");
                }
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 91:
                phOsalNfc_Log("\n-------------------------------------------\n TEST - SWP 2 \n-------------------------------------------\n");
                GeneralState |= HWSELF_TEST_MASK;
                status = SeApi_Test_SWP(0x02);
                GeneralState &= ~HWSELF_TEST_MASK;
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("SWP 2: SUCCESS\n");
                }
                else
                {
                    phOsalNfc_Log("SWP 2: FAILED\n");
                }
                phOsalNfc_Log("-------------------------------------------\n");
                break;

            case 92:
                phOsalNfc_Log("\n-------------------------------------------\n TEST - Antenna Measurement \n-------------------------------------------\n");
                GeneralState |= HWSELF_TEST_MASK;
                phOsalNfc_SetMemory(&results, 0x00, sizeof(results));
                status = SeApi_Test_AntennaMeasure(&results);
                GeneralState &= ~HWSELF_TEST_MASK;
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Antenna Measurement: SUCCESS\n");
                    phOsalNfc_Log("  %d mA  - TxLDO\n", results.wTxdoValue);
                    phOsalNfc_Log("  %d     - AGC\n", results.wAgcValue);
                    phOsalNfc_Log("  %d     - AGC with fixed NFCLD\n", results.wAgcValuewithfixedNFCLD);
                    phOsalNfc_Log("  %d     - AGC with Open RM\n", results.wAgcDifferentialWithOpen1);
                    phOsalNfc_Log("  %d     - AGC with Short RM\n", results.wAgcDifferentialWithOpen2);
                }
                else
                {
                    phOsalNfc_Log("Antenna Measurement: FAILED\n");
                }
                phOsalNfc_Log("-------------------------------------------\n");
                break;
            case 93:
                phOsalNfc_Log("\n-------------------------------------------\n TEST - Pins \n-------------------------------------------\n");
                GeneralState |= HWSELF_TEST_MASK;
                status = SeApi_Test_Pins();
                GeneralState &= ~HWSELF_TEST_MASK;
                if(status == SEAPI_STATUS_OK)
                {
                    phOsalNfc_Log("Test Pins: SUCCESS\n");
                }
                else
                {
                    phOsalNfc_Log("Test Pins: FAILED\n");
                }
                phOsalNfc_Log("-------------------------------------------\n");
                break;
#endif /* NXP_HW_SELF_TEST */
           default:
                phOsalNfc_Log("Invalid option\n");
        }
        if(status == SEAPI_STATUS_OK)
        {
            phOsalNfc_Log("Test PASSED\n");
        }
        else
        {
            phOsalNfc_Log("Test FAILED; Status: 0x%02X\n",status);
        }
    }

clean_and_return:
    //Board_LED_Set(0, FALSE); /* Turn Off LED #0 */
    SeApi_Shutdown();
    phOsalNfc_Log_DeInit();
    phOsalNfc_Thread_Delete(NULL);
    return;
}
#endif /*NFC_UNIT_TEST*/
