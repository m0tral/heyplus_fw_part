
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

/* Header files */

#include "nci_utils.h"
#ifndef NFC_UNIT_TEST
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
//#include "port.h"
#include <phOsalNfc.h>
#include <phOsalNfc_Thread.h>
#include <phOsalNfc_Log.h>
#include "UserData_Storage.h"
#include "ble_handler.h"
#include "debug_settings.h"
#include "wearable_platform_int.h"
#include "wearable_tlv_types.h"
#include "phNxpNciHal.h"
#include <eaci_uart.h>
//#include "lpc_res_mgr.h"

#include "ry_hal_gpio.h"
#include "nfc_pn80t.h"
#include "SeApi_Int.h"
#include "ryos.h"
#include "device_management_service.h"
#include "card_management_service.h"
#include "led.h"


#if (GDI_GUP_ENABLE == TRUE)
//#include "gup.h"
#endif


//global variables
#if(WEARABLE_LS_LTSM_CLIENT == TRUE)
static uint8_t pRespBuf[RESP_BUFF_SIZE];
#else
static uint8_t pRespBuf[RESP_BUFF_SIZE];
#endif
static UINT16 respSize = 32;
static UINT8 respTagType = 0;
#if (ENABLE_SESSION_MGMT == TRUE)
tSeApi_SessionSaveCallbacks sessionCb;
#endif
phNfcBle_Context_t gphNfcBle_Context;
phDevice_Status_t DeviceStatus;
BOOLEAN bBleInterrupted = FALSE;

extern nfc_stack_cb_t nfc_stack_cb;
extern u8_t card_getTypeByAID(u8_t* aid, int aidLen);
extern void build_select_apdu(u8_t* apdu, u8_t* aid, u8_t len);


void cleanupOnBleInterruption()
{
    GeneralState |= NFCINIT_MASK;
    //BLE Disconnection during firmware update
    if(GeneralState & FWDNLD_MASK)
    {
        //cleanupOnFwDndInterruption();
        NfcA_HalCleanup();
        NfcA_Finalize();
        if(SeApi_PrepFirmwareDownload() != SEAPI_STATUS_OK) {
           phOsalNfc_Log("\n --- SeApi_PrepFirmwareDownload ---\n");
          }
        bBleInterrupted = TRUE;
    }
    else
    {
        disableWiredandSEreset();
    }
}
/*
void cleanupOnFwDndInterruption()
{
    static uint8_t tmpBuff[2]; //buffer used to hold tlv values. During dequeing the values should be intact.
    BaseType_t xStatus;
    BaseType_t xSemStatus;
    static xTLVMsg tlv_clenup;
    tlv_clenup.type = FWDND_MODE;
    tlv_clenup.length = 2;
    tlv_clenup.value = tmpBuff;
    tlv_clenup.value[0] = CLEANUP_ON_BLE_INTERRUPTION;

    if(xSemaphoreTake(nfcQueueLock, portMAX_DELAY) == pdTRUE) {
        xStatus = xQueueSendToBack( xQueueNFC, &tlv_clenup, 0 );
        xSemStatus = xSemaphoreGive(nfcQueueLock);
        if(( xStatus != pdPASS ) || (xSemStatus != pdPASS ))
        {
            phOsalNfc_LogBle("ERROR in cleanupOnFwDndInterruption.\r\n" );
        }
    }
}*/

#define STATUS_OK          		0x00
#define STATUS_INVALID_PARAM   	0x01
#define STATUS_INVALID_LENGTH	0x02
#define STATUS_PN80T_ERROR		0x03
#define STATUS_MATCHER_ERROR	0x04

#define WIRED_MODE_ENABLE 1
#define WIRED_MODE_DISABLE 0




#define APDU_SIZE 265
#define APDU_DATA_SIZE 240

#define FP_APDU_CLA 0x80
#define FP_APDU_INS 0xE2 // Store Data

//static uint8_t fpPN80T_Data[100];
//static uint16_t fpPN80T_DataSize = 255;
static UINT8 apdu[APDU_SIZE];
uint8_t cplc[100];
uint16_t cplc_len;
int nfc_init_flag = 0;


#if (MIFARE_OP_ENABLE == FALSE)
UINT16 utils_MakeCAPDU(UINT8 cla, UINT8 ins, UINT8 p1, UINT8 p2, const UINT8* pData, UINT16 dataLen, UINT8* pDest, UINT16 destSize)
{
	UINT16 size;

	if( (pDest == NULL) || (destSize < (5 + dataLen)))
	{
		return 0;
	}

	*pDest++ = cla;
	*pDest++ = ins;
	*pDest++ = p1;
	*pDest++ = p2;

	if (pData == NULL)
    {
		*pDest++ = 0;
    	size = 5;
    }
    else
    {
    	*pDest++ = dataLen;
    	memcpy(pDest, pData, dataLen);
        size = dataLen + 5;
    }

	return size;
}
#endif



UINT8 create_apdu(UINT16 dataLen, const UINT8 *data, UINT8 index, UINT8 *apdu, UINT16 *apduSize)
{
	UINT16 dataSize;
	bool isLastAPDU = false;

	if(index * APDU_DATA_SIZE > dataLen)
		return STATUS_INVALID_LENGTH;

	// Number of APDUs needed to encapsulate the data
	if(dataLen > (index + 1) * APDU_DATA_SIZE) {
		dataSize = APDU_DATA_SIZE;
	} else {
		dataSize = dataLen - (index * APDU_DATA_SIZE);
		isLastAPDU = true;
	}

	*apduSize = utils_MakeCAPDU(
			FP_APDU_CLA,
			FP_APDU_INS,
			isLastAPDU ? 0x90 : 0x10,
			index,
			data + (index * APDU_DATA_SIZE),
			dataSize,
			apdu,
			APDU_SIZE);

	return STATUS_OK;
}

UINT8 Select_CRS_SendData(UINT16 data_size, UINT8 *data, UINT16* pRspSize, UINT8* pRespBuf) {
	uint8_t status;
	*pRspSize = 0;

    LOG_DEBUG("[%s] Select CRS Enter\r\n", __FUNCTION__);

	// Open Wired mode
	LOG_DEBUG("[%s] Open Wired\r\n", __FUNCTION__);
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select CRS
	LOG_DEBUG("[%s] Wired Transceive\r\n", __FUNCTION__);
	status = SeApi_WiredTransceive((UINT8*)SelectCRS, sizeof(SelectCRS), pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
        status = SeApi_WiredEnable(0);
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			// Clear CVM Status
			status = SeApi_WiredTransceive(data, data_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
			if(status != SEAPI_STATUS_OK) {
				phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
				status = STATUS_PN80T_ERROR;
			}
		}
	}

    LOG_DEBUG("[%s] Close Wired\r\n", __FUNCTION__);
	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

    LOG_DEBUG("[%s] Select CRS Exit\r\n", __FUNCTION__);

	return status;
}


u8_t select_aid(const UINT8 *aid, UINT8 aid_size) 
{
	uint8_t status;
    uint8_t* resp = ry_malloc(255);
    if (resp == NULL) {
        return STATUS_PN80T_ERROR;
    }
    ry_memset(resp, 0, 255);

    
    uint16_t resp_size = 0;
    uint8_t ret_status;

	// Open Wired mode
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
        ry_free(resp);
		return STATUS_PN80T_ERROR;
	}

	// Select CRS
	status = SeApi_WiredTransceive((UINT8*)aid, aid_size, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
        ry_free(resp);
        resp = NULL;
		status = STATUS_PN80T_ERROR;
        ret_status = 0x09;
	} else {
	    LOG_DEBUG("[select_aid] resp dump: ");
	    ry_data_dump(resp, resp_size, ' ');
		// Confirm that the applet is available in the Secure Element
		if((resp[resp_size-2] == 0x90) && (resp[resp_size-1] == 0x00)) {
			ret_status = SEAPI_STATUS_OK;
		} else if((resp[resp_size-2] == 0x6a) && (resp[resp_size-1] == 0x82)){
            ret_status = 0x09; // Not found.
        } else {
            ret_status = 0x09;
        }
	}
    

	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
        ry_free(resp);
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

    ry_free(resp);

	return ret_status;
}



u8_t activate_and_deactivate_aid(const u8_t* prefix, u8_t prefix_len, const u8_t* aid, u8_t len)
{
    u8_t *p = NULL;
    u16_t apdu_len;
    u8_t  status;
    u8_t* resp = NULL;
    u16_t resp_len;
    u8_t* apdu = (u8_t*)ry_malloc(len + 7);
    
    if (NULL == apdu) {
        LOG_WARN("[activate_and_deactivate_aid] NO MEM. \r\n");
        status = 0xFF;
        goto exit;
    }

    resp = (u8_t*)ry_malloc(100);
    if (NULL == resp) {
        LOG_WARN("[activate_and_deactivate_aid] NO MEM. \r\n");
        status = 0xFF;
        goto exit;
    }

    /* Construct APDU */
    p = apdu;
    ry_memcpy(p, prefix, prefix_len);
    p += prefix_len;
    *p++ = len + 2;
    *p++ = 0x4F;
    *p++ = len;
    ry_memcpy(p, aid, len);
    apdu_len = len+7;

    /* Send through CRS */
    status = Select_CRS_SendData(apdu_len, apdu, &resp_len, resp);
    if (SEAPI_STATUS_OK != status) {
        goto exit;
    }

    /* Check response status */
    if((resp[resp_len-2] == 0x90) && (resp[resp_len-1] == 0x00)) {
        status = SEAPI_STATUS_OK;
    } else {
        status = 0xFE;
    }
    

exit:
    if (apdu) ry_free(apdu);
    if (resp) ry_free(resp);
    return status;
    
}



int query_balance(const UINT8 *aid, UINT8 aid_size)
{
    int ret = 0;
    uint8_t status;
    u8_t* apdu;
    u8_t type;
    
    apdu = ry_malloc(aid_size + 5);
    if (apdu == NULL) {
        return -1;
    }
    
    uint8_t* resp = ry_malloc(255);
    if (resp == NULL) {
        return -1;
    }
    ry_memset(resp, 0, 255);

    type = card_getTypeByAID((u8_t*)aid, aid_size);

    
    uint16_t resp_size = 0;
    uint8_t ret_status = STATUS_PN80T_ERROR;

	// Open Wired mode
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
        ry_free(resp);
		return -1;
	}

	// Select Transit Card
	build_select_apdu(apdu, (u8_t*)aid, aid_size);
	status = SeApi_WiredTransceive((UINT8*)apdu, aid_size+5, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
        ry_free(apdu);
        ry_free(resp);
        resp = NULL;
        apdu = NULL;
		status = STATUS_PN80T_ERROR;
        ret = -2;
	} else {
	    LOG_DEBUG("[select_aid] resp dump: ");
	    ry_data_dump(resp, resp_size, ' ');
		// Confirm that the applet is available in the Secure Element
		if((resp[resp_size-2] == 0x90) && (resp[resp_size-1] == 0x00)) {
			ret_status = SEAPI_STATUS_OK;
		} else if((resp[resp_size-2] == 0x67) && (resp[resp_size-1] == 0x00)){
            ret = 0; // Not found.
            ret_status = SEAPI_STATUS_OK;
        } else {
            ret = -3;
            ret_status = 0x09;
        }
	}

    if (ret_status == SEAPI_STATUS_OK) {

        if (type == CARD_ID_LNT || type == CARD_ID_YCT) {
            /* LNT City */
            status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_LNT1, 7, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
            if(status != SEAPI_STATUS_OK) {
                phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
                ry_free(resp);
                resp = NULL;
                status = STATUS_PN80T_ERROR;
                ret = -7;
                ret_status = 0x09;
                goto exit;
            } else {
                status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_LNT2, 5, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
                if(status != SEAPI_STATUS_OK) {
                    phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
                    ry_free(resp);
                    resp = NULL;
                    status = STATUS_PN80T_ERROR;
                    ret = -8;
                    ret_status = 0x09;
                    goto exit;
                } else {
                    status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_LNT3, 7, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
                }
            }
        } else if (type == CARD_ID_CQT || type == CARD_ID_CAT || type == CARD_ID_HEB || type == CARD_ID_QDT) {
            /* Chongqi Tong */
            status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_CQT1, sizeof(SELECT_FOLDER_CQT1), resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
            if(status != SEAPI_STATUS_OK) {
                phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
                ry_free(resp);
                resp = NULL;
                status = STATUS_PN80T_ERROR;
                ret = -7;
                ret_status = 0x09;
                goto exit;
            } else {
                status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_CQT2, sizeof(SELECT_FOLDER_CQT2), resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
                
            }
        } else {
            /* Common City */
            status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER, 7, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
        }
        
    	if(status != SEAPI_STATUS_OK) {
    		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
            ry_free(resp);
            resp = NULL;
    		status = STATUS_PN80T_ERROR;
            ret = -4;
            ret_status = 0x09;
    	} else {
    	    LOG_DEBUG("[select folder] resp dump: ");
    	    ry_data_dump(resp, resp_size, ' ');
    		// Confirm that the applet is available in the Secure Element
    		if((resp[resp_size-2] == 0x90) && (resp[resp_size-1] == 0x00)) {
    			ret_status = SEAPI_STATUS_OK;
    		} else if((resp[resp_size-2] == 0x67) && (resp[resp_size-1] == 0x00)){
                ret_status = 0x09; // Not found.
                ret = -5;
            } else {
                ret_status = 0x09;
                ret = -5;
            }
    	}
        
        status = SeApi_WiredTransceive((UINT8*)QUERY_BALANCE, 5, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
    	if(status != SEAPI_STATUS_OK) {
    		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
            ry_free(resp);
            resp = NULL;
    		status = STATUS_PN80T_ERROR;
            ret_status = 0x09;
    	} else {
    	    LOG_DEBUG("[query_response] resp dump: ");
    	    ry_data_dump(resp, resp_size, ' ');
    		// Confirm that the applet is available in the Secure Element
    		if((resp[resp_size-2] == 0x90) && (resp[resp_size-1] == 0x00)) {
    			ret_status = SEAPI_STATUS_OK;
                ret = (resp[1]<<16)|(resp[2]<<8)|(resp[3]);
    		} else if((resp[resp_size-2] == 0x6a) && (resp[resp_size-1] == 0x82)){
                ret_status = 0x09; // Not found.
                ret = -6;
            } else {
                ret_status = 0x09;
                ret = -6;
            }
    	}
    }
    
exit:
	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
        ry_free(resp);
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return -1;
	}

    ry_free(apdu);
    ry_free(resp);

	return ret;
}



int query_city(const UINT8 *aid, UINT8 aid_size)
{
    int ret = 0;
    uint8_t status;
    u8_t* apdu;
    u8_t type;
    
    apdu = ry_malloc(aid_size + 5);
    if (apdu == NULL) {
        return -1;
    }
    
    uint8_t* resp = ry_malloc(255);
    if (resp == NULL) {
        return -1;
    }
    ry_memset(resp, 0, 255);

    type = card_getTypeByAID((u8_t*)aid, aid_size);

    
    uint16_t resp_size = 0;
    uint8_t ret_status = STATUS_PN80T_ERROR;

	// Open Wired mode
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
        ry_free(resp);
		return -1;
	}

	// Select Transit Card
	build_select_apdu(apdu, (u8_t*)aid, aid_size);
	status = SeApi_WiredTransceive((UINT8*)apdu, aid_size+5, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
        ry_free(apdu);
        ry_free(resp);
        apdu = NULL;
        resp = NULL;
		status = STATUS_PN80T_ERROR;
        ret = -2;
	} else {
	    LOG_DEBUG("[select_aid] resp dump: ");
	    ry_data_dump(resp, resp_size, ' ');
		// Confirm that the applet is available in the Secure Element
		if((resp[resp_size-2] == 0x90) && (resp[resp_size-1] == 0x00)) {
			ret_status = SEAPI_STATUS_OK;
		} else if((resp[resp_size-2] == 0x67) && (resp[resp_size-1] == 0x00)){
            ret = 0; // Not found.
            ret_status = SEAPI_STATUS_OK;
        } else {
            ret = -3;
            ret_status = 0x09;
        }
	}
	

    if (ret_status == SEAPI_STATUS_OK) {

        if (type == CARD_ID_LNT || type == CARD_ID_YCT) {
            status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_LNT1, 7, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
            if(status != SEAPI_STATUS_OK) {
                phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
                ry_free(resp);
                resp = NULL;
                status = STATUS_PN80T_ERROR;
                ret = -7;
                ret_status = 0x09;
                goto exit;
            } else {
                status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER_LNT4, sizeof(SELECT_FOLDER_LNT4), resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
                if(status != SEAPI_STATUS_OK) {
                    phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
                    ry_free(resp);
                    resp = NULL;
                    status = STATUS_PN80T_ERROR;
                    ret = -8;
                    ret_status = 0x09;
                    goto exit;
                } else {
                    status = SeApi_WiredTransceive((UINT8*)QUERY_CITY, sizeof(QUERY_CITY), resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
                }
            }
        } else {
            status = SeApi_WiredTransceive((UINT8*)SELECT_FOLDER, 7, resp, (APDU_DATA_SIZE-1), &resp_size, 10000);
        }
        
    	if(status != SEAPI_STATUS_OK) {
    		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
            ry_free(resp);
            resp = NULL;
    		status = STATUS_PN80T_ERROR;
            ret = -4;
            ret_status = 0x09;
    	} else {
    	    LOG_DEBUG("[query city] resp dump: ");
    	    ry_data_dump(resp, resp_size, ' ');
    		// Confirm that the applet is available in the Secure Element
    		if((resp[resp_size-2] == 0x90) && (resp[resp_size-1] == 0x00)) {
                ret = resp[0];
    			ret_status = SEAPI_STATUS_OK;
    		} else if((resp[resp_size-2] == 0x67) && (resp[resp_size-1] == 0x00)){
                ret_status = 0x09; // Not found.
                ret = -5;
            } else {
                ret_status = 0x09;
                ret = -5;
            }
    	}
        
        
    }
    
exit:
	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
        ry_free(resp);
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return -1;
	}

    ry_free(apdu);
    ry_free(resp);

	return ret;
}



UINT8 Get_CPLC_SendData(UINT16* pRspSize, UINT8* pRespBuf) {
	uint8_t status;
	*pRspSize = 0;

	// Open Wired mode
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select CRS
	status = SeApi_WiredTransceive((UINT8*)GETCPLC, sizeof(GETCPLC), pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			
		}
	}

	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	return status;
}



UINT8 TestData[] = {0x00, 0x70, 0x00, 0x00, 0x01};

UINT8 Test_SendData(UINT16 data_size, UINT8 *data, UINT16* pRspSize, UINT8* pRespBuf)
{
    uint8_t status;
	*pRspSize = 0;

	// Open Wired mode
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select CRS
	status = SeApi_WiredTransceive((UINT8*)data, data_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			
		}

        ry_data_dump(pRespBuf, *pRspSize, ' ');
	}

	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	return status;
}



UINT8 isd_aid[] = {0x00, 0xA4, 0x04, 0x00, 0x08, 0xa0, 0x00, 0x01, 0x51, 0x00, 0x00, 0x00};

UINT8 select_isd(UINT16 data_size, UINT8 *data, UINT16* pRspSize, UINT8* pRespBuf)
{
    uint8_t status;
	*pRspSize = 0;

	// Open Wired mode
	status = SeApi_WiredEnable(1);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select CRS
	status = SeApi_WiredTransceive((UINT8*)data, data_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			
		}

        ry_data_dump(pRespBuf, *pRspSize, ' ');
	}

	status = SeApi_WiredEnable(0);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	return status;
}



#if 0
UINT8 fpPNx_Matcher_SendData(UINT16 data_size, UINT8 *data, UINT16* pRspSize, UINT8* pRespBuf) {
	uint8_t status, num_apdus;
	uint16_t apdu_size;

	*pRspSize = 0;

	// Open Wired mode
	status = SeApi_WiredEnable(WIRED_MODE_ENABLE);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select Matcher applet
	status = SeApi_WiredTransceive((UINT8*)SelectMatcher, sizeof(SelectMatcher), pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			// Number of APDUs needed to encapsulate the data
			if(data_size % APDU_DATA_SIZE == 0)
				num_apdus = data_size / APDU_DATA_SIZE;
			else
				num_apdus = (data_size / APDU_DATA_SIZE) + 1;

			for(int i = 0; i < num_apdus; i++) {
				create_apdu(data_size, data, i, apdu, &apdu_size);

				status = SeApi_WiredTransceive(apdu, apdu_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
				if(status != SEAPI_STATUS_OK) {
					phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
					return status;
				}

				if((pRespBuf[*pRspSize-2] != 0x90) || (pRespBuf[*pRspSize-1] != 0x00))
					break;
			}
		}
	}

	status = SeApi_WiredEnable(WIRED_MODE_DISABLE);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	return status;
}

UINT8 fpPNx_Matcher_SendVerifiedCmd(UINT16 match_data_size, UINT8* match_data, UINT16 data_size, UINT8 *data, UINT16* pRspSize, UINT8* pRespBuf) {
	uint8_t status, num_apdus;
	uint16_t apdu_size;

	*pRspSize = 0;

	// Open Wired mode
	status = SeApi_WiredEnable(WIRED_MODE_ENABLE);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select Matcher applet
	status = SeApi_WiredTransceive((UINT8*)SelectMatcher, sizeof(SelectMatcher), pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			// Number of APDUs needed to encapsulate the data
			if(match_data_size % APDU_DATA_SIZE == 0)
				num_apdus = match_data_size / APDU_DATA_SIZE;
			else
				num_apdus = (match_data_size / APDU_DATA_SIZE) + 1;

			for(int i = 0; i < num_apdus; i++) {
				create_apdu(match_data_size, match_data, i, apdu, &apdu_size);

				status = SeApi_WiredTransceive(apdu, apdu_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
				if(status != SEAPI_STATUS_OK) {
					phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
					return status;
				}

				if((pRespBuf[*pRspSize-2] != 0x90) || (pRespBuf[*pRspSize-1] != 0x00))
					break;
            }
            
            
            if ((pRespBuf[*pRspSize-2] == 0x90) || (pRespBuf[*pRspSize-1] == 0x00)) {
                phOsalNfc_Log("[fpPNx_Matcher_SendVerifiedCmd]: Match Success.\r\n");
                // Number of APDUs needed to encapsulate the data
                if(data_size % APDU_DATA_SIZE == 0)
                    num_apdus = data_size / APDU_DATA_SIZE;
                else
                    num_apdus = (data_size / APDU_DATA_SIZE) + 1;

                for(int i = 0; i < num_apdus; i++) {
                    create_apdu(data_size, data, i, apdu, &apdu_size);

                    status = SeApi_WiredTransceive(apdu, apdu_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
                    if(status != SEAPI_STATUS_OK) {
                        phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
                        return status;
                    }

                    if((pRespBuf[*pRspSize-2] != 0x90) || (pRespBuf[*pRspSize-1] != 0x00))
                        break;
                }
            }
		}
	}

	status = SeApi_WiredEnable(WIRED_MODE_DISABLE);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	return status;
}


UINT8 fpPNx_Matcher_SendRawData(UINT16 data_size, UINT8 *data, UINT16* pRspSize, UINT8* pRespBuf) {
	uint8_t status, num_apdus;
	uint16_t apdu_size;

	*pRspSize = 0;

	// Open Wired mode
	status = SeApi_WiredEnable(WIRED_MODE_ENABLE);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Enable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	// Select Matcher applet
	status = SeApi_WiredTransceive((UINT8*)SelectMatcher, sizeof(SelectMatcher), pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
	if(status != SEAPI_STATUS_OK) {
		phOsalNfc_Log("Wired Transceive error. Status: %x\n", status);
		status = STATUS_PN80T_ERROR;
	} else {
		// Confirm that the applet is available in the Secure Element
		if((pRespBuf[*pRspSize-2] == 0x90) && (pRespBuf[*pRspSize-1] == 0x00)) {
			// Number of APDUs needed to encapsulate the data
			status = SeApi_WiredTransceive((UINT8*)data, data_size, pRespBuf, (APDU_DATA_SIZE-1), pRspSize, 10000);
            if(status != SEAPI_STATUS_OK) {
                phOsalNfc_Log("Wired Transceive error 2. Status: %x\n", status);
                status = STATUS_PN80T_ERROR;
            } else {
            }
        }
	}

	status = SeApi_WiredEnable(WIRED_MODE_DISABLE);
	if(status != SEAPI_STATUS_OK && status != SEAPI_STATUS_NO_ACTION_TAKEN) {
		phOsalNfc_Log("Wired Disable error. Status: %x\n", status);
		return STATUS_PN80T_ERROR;
	}

	return status;
}

#endif



void BleNfcBridge(void *pParams)
{
	BaseType_t xStatus;
	xTLVMsg tlv;
	xTLVMsg tlvAck;
	tSEAPI_STATUS status;
    phTransitSettings_t settings;
	bool transit_settings_present;
	DeviceStatus.initstatus = INITSTATUS_FAILED;
	gphNfcBle_Context.isBleInitialized = FALSE;
    //vTaskSuspend(xHandleBLETask);                  // SUSPEND THE BLE TASK


    u32_t needUpdate = (u32_t)pParams;

    
 #if 0 // Kanjie
    if (phNfcInitBleSubsystem(&gphNfcBle_Context) != SEAPI_STATUS_OK)
    {
        phOsalNfc_Log("\n ble init subsystem failed \n");
    }
    
 #endif
    GeneralState |= NFCINIT_MASK;					// Set NFCINIT bit

    transit_settings_present = true;
    phOsalNfc_SetMemory(&settings, 0, sizeof(phTransitSettings_t));
	DeviceStatus.initstatus = INITSTATUS_FAILED;

    status = transitSettingRead((UINT8*)&settings, sizeof(settings));
    if(status != SEAPI_STATUS_OK)
    {
        settings.cfg_blk_chk_enable = 1;
        settings.rfSettingModified = 0;
        transit_settings_present = false;
    }
    phNxpNciHal_set_transit_settings(&settings);

#if (ENABLE_SESSION_MGMT == TRUE)
	sessionCb.writeCb = &sessionWrite;
	sessionCb.readCb = &sessionRead;
#endif

	tSeApi_SeEventsCallbacks seEventsCb;
	seEventsCb.rfFieldStatusCb = &rfFieldStatusCallback;
	seEventsCb.connectivityCb = &connectivityCallback;
	seEventsCb.activationStatusCb = &activationStatusCallback;

    phOsalNfc_LogBle("\n--- LIBNFC_NCI_INIT  ---\n");
    
#if (ENABLE_SESSION_MGMT == TRUE)

    if (needUpdate) {
        status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
        //status = SeApi_Init(&transactionOccurred, &sessionCb);
	} else {
        status = SeApi_Init(&transactionOccurred, &sessionCb);
        //status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
    }

    #if 0
    if (dev_mfg_state_get() == DEV_MFG_STATE_PCBA) {
        LOG_DEBUG("[BleNfcBridge] SE API Init without update.\r\n");
        status = SeApi_Init(&transactionOccurred, &sessionCb);
        
    } else {
        LOG_DEBUG("[BleNfcBridge] SE API Init with update.\r\n");
        //status = SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
        status = SeApi_Init(&transactionOccurred, &sessionCb);
    }
    #endif

    if (status != SEAPI_STATUS_OK) {

    // if((SeApi_Init(&transactionOccurred, &sessionCb)) != SEAPI_STATUS_OK) {
    // if((SeApi_InitWithFirmware(&transactionOccurred, &sessionCb, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw)) != SEAPI_STATUS_OK) {
#else

    if (needUpdate) {
        status = SeApi_InitWithFirmware(&transactionOccurred, NULL, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
    } else {
        status = SeApi_Init(&transactionOccurred, NULL);
    }

    #if 0

    if (dev_mfg_state_get() == DEV_MFG_STATE_PCBA) {
        LOG_DEBUG("[BleNfcBridge] SE API Init without update.\r\n");
        status = SeApi_Init(&transactionOccurred, NULL);
    } else {
        LOG_DEBUG("[BleNfcBridge] SE API Init with update.\r\n");
        //status = SeApi_InitWithFirmware(&transactionOccurred, NULL, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw);
        status = SeApi_Init(&transactionOccurred, &sessionCb);
    }
    #endif

    if (status != SEAPI_STATUS_OK) {

    // if((SeApi_Init(&transactionOccurred, NULL)) != SEAPI_STATUS_OK) {
    // if((SeApi_InitWithFirmware(&transactionOccurred, NULL, gphDnldNfc_DlSeqSz, gphDnldNfc_DlSequence, gphDnldNfc_DlSeqDummyFwSz, gphDnldNfc_DlSequenceDummyFw)) != SEAPI_STATUS_OK) {
#endif
        phOsalNfc_LogBle("\n --- SeAPI init Failed ---\n");
        phOsalNfc_LogBle("\n --- END LIBNFC_NCI_INIT ---\n");
    }
    else
    {
        phOsalNfc_LogBle("\n --- END LIBNFC_NCI_INIT ---\n");
#if defined(MALLOC_TRACE) && (MALLOC_TRACE == TRUE)
        phOsalNfc_LogBle("\n === After NFC Init, Allocated Heap %d ===\n", cumalloc);
#endif

        phOsalNfc_LogBle("\n--- SeApi_SubscribeForSeEventsNotfn  ---\n");
        if (SeApi_SubscribeForSeEvents(&seEventsCb) != SEAPI_STATUS_OK)
        {
             phOsalNfc_LogBle("\n --- SeAPI Subscribe for SE FAILED ---\n");
        }
        phOsalNfc_LogBle("\n--- END SeApi_SubscribeForSeEventsNotfn  ---\n");

        phOsalNfc_LogBle("\n\r--- NFC eSE Wired ENABLE ---\n");
        status = SeApi_WiredEnable(TRUE);
        if((SEAPI_STATUS_OK != status) && (SEAPI_STATUS_NO_ACTION_TAKEN != status))
        {
            phOsalNfc_LogBle("\n --- NFC eSE Wired ENABLE Failed ---\n");
        }
        else
        {
            phOsalNfc_LogBle("\n\r--- END NFC eSE Wired ENABLE ---\n");

            phOsalNfc_LogBle("\n\r--- NFC eSE get ATR ---\n");
            respSize = sizeof(pRespBuf);
            if(SEAPI_STATUS_OK == SeApi_WiredGetAtr(pRespBuf, &respSize))
            {
                int i;
                phOsalNfc_LogBle ("\n\r ATR length: %d, ATR: ", respSize);
                for(i=0; i<respSize; i++)
                {
                    phOsalNfc_LogBle("0x%02X ", pRespBuf[i]);
                }
                phOsalNfc_LogCrit("\n");
                DeviceStatus.initstatus = INITSTATUS_COMPLETED;
            }
            else
            {
                phOsalNfc_LogBle("\n\r--- NFC eSE get ATR Failed ---\n");
                DeviceStatus.initstatus = INITSTATUS_FAILED_TO_GET_ATR;
            }
            phOsalNfc_LogBle("\n\r--- END NFC eSE get ATR ---\n");

            phOsalNfc_LogBle("\n\r--- NFC eSE Wired DISABLE ---\n");
            //if(SeApi_WiredEnable(FALSE) != SEAPI_STATUS_OK) {
            //    phOsalNfc_LogBle("\n --- SeAPI Wired disable FAILED ---\n");
            //}
            //phOsalNfc_LogBle("\n\r--- END NFC eSE Wired DISABLE ---\n");
        }
        
		/* Get CPLC */
        Get_CPLC_SendData(&cplc_len, cplc);
        LOG_DEBUG("[nfc thread] CPLC Got.\r\n");
        ry_data_dump(cplc, cplc_len, ' ');

        LOG_DEBUG("[nfc thread] FW Version:0x%x\r\n", wFwVerRsp);

        if (wFwVerRsp == 0x00110114 && dev_mfg_state_get()==0xFF) {
            LOG_DEBUG("[nfc thread] NFC download FW done.\r\n");
            ry_led_onoff(1);
        }

		/* Activate CRS - UnionPay */
		//phOsalNfc_LogBle ("\n\r--- NFC Select CRS ---\n");
        //uint8_t ret = Select_CRS_SendData(sizeof(PBOCAID), (UINT8*)PBOCAID, &fpPN80T_DataSize, fpPN80T_Data);

        /* Activate CRS - SZT */
        //phOsalNfc_LogBle ("\n\r--- NFC Select CRS ---\n");
        //uint8_t ret = Select_CRS_SendData(sizeof(SelectSZT), (UINT8*)SelectSZT, &fpPN80T_DataSize, fpPN80T_Data);

        //ACTIVATE_SZT();

        //DEACTIVATE_BJT();
#if 0
        u8_t ret = ACTIVATE_BJT();
        if (ret == 0) {
            LOG_DEBUG(" BJT exist.\r\n");
					  DEACTIVATE_BJT();
        }

        ret = ACTIVATE_SZT();
        if (ret == 0) {
            LOG_DEBUG(" SZT exist.\r\n");
					  DEACTIVATE_SZT();
        }

        ret = ACTIVATE_LNT();
        if (ret == 0) {
            LOG_DEBUG(" LNT exist.\r\n");
					  DEACTIVATE_LNT();
        }
#endif
				
        #if 0
        u8_t ret = DEACTIVATE_SZT();
        if (ret != 0) {
            LOG_WARN("Deactivate SZT failed. status = 0x%x\r\n", ret);
        }


        ret = ACTIVATE_LNT();
        if (ret != 0) {
            LOG_WARN("Activate BJT failed. status = 0x%x\r\n", ret);
        } else {
            LOG_DEBUG("Activate BJT success.\r\n");
        }
        #endif
				
		/* Test Enrollment */
		//phOsalNfc_LogBle ("\n\r--- NFC Test Enrollment Applet ---\n");
        //phNfcFp_PerformEnrollment();				

		/* Test Match */
		//phOsalNfc_LogBle ("\n\r--- NFC Test Match Applet ---\n");
        //phNfcFp_PerformMatching();
        
        /* Test Delete */
        //phOsalNfc_LogBle ("\n\r--- NFC Test Delete Applet ---\n");
        //phNfcFp_PerformDeleteFinger();
        
        /* Test Get Data */
        //phOsalNfc_LogBle ("\n\r--- NFC Test Get Data ---\n");
        //phNfcFp_PerformGetData();
				
				
		#if 0
        phOsalNfc_LogBle ("\n\r--- NFC eSE Virtual Operation ---\n");
        if(SeApi_SeSelect(SE_ESE) != SEAPI_STATUS_OK) {
            phOsalNfc_LogBle("\n --- SeAPI select FAILED ---\n");
        }
        phOsalNfc_LogBle("\n\r--- END eSE Virtual Operation ---\n");
        #endif
			
				
		nfc_init_flag = 1;

#if 0
        thread_status_str = (char*)ry_malloc(1024);
        if (thread_status_str) {
            ryos_get_thread_list(thread_status_str);
            LOG_DEBUG("[nfc thread] \n%s\n", thread_status_str);
            ry_free(thread_status_str);

            phOsalNfc_LogBle("\n\r---Free Heap Size: %d --- \n", xPortGetFreeHeapSize());	
        }
        
#endif
        //status = SeApi_WiredEnable(TRUE);
        //if((SEAPI_STATUS_OK != status) && (SEAPI_STATUS_NO_ACTION_TAKEN != status))
        //{
        //    phOsalNfc_LogBle("\n --- NFC eSE Wired ENABLE Failed ---\n");;
        //}

        //phOsalNfc_LogBle ("\n\r--- NFC Test Data ---\n");
        //Test_SendData(sizeof(TestData), TestData, &nfc_test_data_len, nfc_test_data_resp);


        //phOsalNfc_LogBle ("\n\r--- NFC Test Data2 ---\n");
        //Test_SendData2(sizeof(TestData2), TestData2, &nfc_test_data_len, nfc_test_data_resp);
				
        //gup_send_se_init_done_evt();
    }

#if defined(MALLOC_TRACE) && (MALLOC_TRACE == TRUE)
    phOsalNfc_LogBle("\n === After NFC Virtual mode, Allocated Heap %d ===\n", cumalloc);
#endif


    //phNfcStartBleDiscovery(); // Make Wearable device discover-able only after NFCC and Se is initialized.
	GeneralState &= ~NFCINIT_MASK;					// Clear NFCINIT bit
    //ResMgr_EnterNormalMode(TRUE);

    if (nfc_stack_cb) {
        nfc_stack_cb(NFC_STACK_EVT_INIT_DONE, NULL);
    }
    

	//vTaskResume(xHandleBLETask);                  // RESUME THE BLE TASK

	while (1) // (1)
	{
		xStatus = xQueueReceive( xQueueNFC, &tlv, portMAX_DELAY);
		if( xStatus == pdPASS )
		{
			// Filter the TLV based on the type and complete the required operation
			//phOsalNfc_LogBle("TLV TYPE: %d and Length %d\r\n", tlv.type, tlv.length);

			phOsalNfc_LogBle(".");
			unsigned char subType = tlv.value[0];
			tlv.value = tlv.value + 1;
			tlv.length = tlv.length - 1;

			/* When Init failed or previous FW download is interrupted, disallow all TLVs other than
			 * FW Dnld TLV, GetStackCapabilities TLV, BLE Echo TLV */
			if((DeviceStatus.fwdnldstatus != FWDNLDSTATUS_COMPLETED || DeviceStatus.initstatus != INITSTATUS_COMPLETED) &&
			    tlv.type != FWDND_MODE && tlv.type != GENERAL_MODE && tlv.type != 0xE0 )
			{
			    respTagType = 0x00;
			    respSize = 0x01;
			    pRespBuf[0] = SEAPI_STATUS_REJECTED; /* rejected */
			}
			else
			{
                switch(tlv.type)
                {
                    case 0xE0:
                    {
                        /*
                         * For handling BLE echo only.
                         */
                        phOsalNfc_LogBle("\n\r--- Start ECHO test---\n");
                        respTagType = 0xE0;
                        respSize = (UINT16)tlv.length;
                        memcpy(pRespBuf, tlv.value, tlv.length);
                        phOsalNfc_LogBle("\n\r--- END ECHO test---\n");
                    }
                    break;
                    case GENERAL_MODE:
                    {
                        GeneralModeTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
                    case SE_MODE:
                    {
                        SeModeTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
                    case FWDND_MODE:
                    {
                        FwDndTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
#if (API_TEST_ENABLE == TRUE)
                    case API_TEST_MODE:
                    {
                        ApiTestTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
#endif
                    case NVMEM_RW_MODE:
                    {
                        NvMemTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
                    case NCI_RAW_MODE:
                    {
                        NciRawModeTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
#if(WEARABLE_LS_LTSM_CLIENT == TRUE) && (SESUPPORT_INCLUDE == TRUE)
                    case WEARABLE_LS_LTSM:
                    {
                        WearableLsLtsmTlvHandler(subType, tlv.value, tlv.length, &respSize, pRespBuf, &respTagType);
                    }
                    break;
#endif
                    default:
                        respSize = 1;
                        pRespBuf[0] = SEAPI_STATUS_FAILED;
                        phOsalNfc_LogCrit("\n\r Invalid option\n");
                }
#if DEBUG_MEMORY != 1
                // We are done processing the tlv so we can freed it
                free(tlv.value);
#endif
            }

			if(respSize > 0) {
				tlvAck.type = respTagType;
				tlvAck.length = respSize;
				tlvAck.value = pRespBuf;

#ifdef COPY_ARRAY
				int j;
				for(j = 0; j < tlvAck.length; j++) {
					tlvAck.value[j] = pRespBuf[j];
				}
#endif
				if ((GeneralState & BLE_CONNECTED_MASK) != 0)
				{
					//tBLE_STATUS tResToCompDevstatus = phNfcSendResponseToCompanionDevice(&gphNfcBle_Context, &tlvAck);
	                //if(tResToCompDevstatus != BLE_STATUS_OK) {
	                	//Response to companion device is successful
                    //    phOsalNfc_LogBle("\n --- Sending response to companion FAILED ---\n");
	                //}
				}
			}
			else
				phOsalNfc_LogCrit("Nothing to send");

			phOsalNfc_LogCrit("\r\n---------------------- COMMAND EXECUTION COMPLETED ----------------------\n");
		}
	}
}

#endif /* not define NFC_UNIT_TEST */
