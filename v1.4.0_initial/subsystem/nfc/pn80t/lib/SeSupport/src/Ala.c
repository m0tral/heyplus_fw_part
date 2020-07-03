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

#include "Ala.h"
#include "AlaLib.h"
#if (LS_ENABLE == TRUE)
#include <errno.h>
#include <string.h>
#include <ctype.h>
#ifndef  COMPANION_DEVICE
#include <cutils/log.h>
#include <gki.h>
#endif
#include <IChannel.h>

pAla_Dwnld_Context_t gpAla_Dwnld_Context=NULL;
BOOLEAN mIsInit;
#if(NXP_LDR_SVC_VER_2 == FALSE)
UINT8 Select_Rsp[1024];
UINT8 Jsbl_RefKey[256];
UINT8 Jsbl_keylen;
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
UINT8 StoreData[22];
#else
UINT8 StoreData[34];
#endif
#if(NXP_LDR_SVC_VER_2 == FALSE)
int Select_Rsp_Len;
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
UINT8 lsVersionArr[2];
UINT8 tag42Arr[17];
UINT8 tag45Arr[9];
UINT8 lsExecuteResp[4];
UINT8 AID_ARRAY[22];
INT32 resp_len = 0;
#if(ALA_WEARABLE_BUFFERED != TRUE)
FILE *fAID_MEM = NULL;
FILE *fLS_STATUS = NULL;
#endif
UINT8 lsGetStatusArr[2];

static tJBL_STATUS
ALA_OpenChannel(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo);

static tJBL_STATUS
ALA_SelectAla(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo);

static tJBL_STATUS
ALA_StoreData(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo);

static tJBL_STATUS
ALA_loadapplet(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info);

#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
static tJBL_STATUS ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo),
                                            INT32 nameLen, const char *name, UINT16 *destLen, const char *dest);
#else
static tJBL_STATUS
ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo), const char *name, const char *dest);
#endif/*ALA_WEARABLE_BUFFERED*/
#else
static tJBL_STATUS
ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo), const char *name);

static tJBL_STATUS
JsblCerId_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo));
#endif/* NXP_LDR_SVC_VER_2*/


#if(NXP_LDR_SVC_VER_2 == TRUE)
#define NOOFAIDS     0x03
#define LENOFAIDS    0x16

static UINT8 GetData[] = {0x80, 0xCA, 0x00, 0x46, 0x00};

#ifndef NXP_LS_AID
static UINT8 SelectAla[] = {0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x43, 0x4F, 0x52, 0x01};

static UINT8 ArrayOfAIDs[NOOFAIDS][LENOFAIDS] = {
        {0x12, 0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x4C, 0x44, 0x52, 0x01,0x00,0x00,0x00},
        {0x12, 0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x43, 0x4F, 0x52, 0x01,0x00,0x00,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
#else
static UINT8 SelectAla[] = {0x00, 0xA4, 0x04, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x54, 0x43, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B
, 0x00,0x01};
static UINT8 ArrayOfAIDs[NOOFAIDS][LENOFAIDS] = {
        {0x14, 0x00, 0xA4, 0x04, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x54, 0x43, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B,0x00,0x02,0x00},
        {0x14, 0x00, 0xA4, 0x04, 0x00, 0x0F, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x54, 0x43, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0B,0x00,0x01,0x00},
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};
#endif/*NXP_LS_AID*/
#else
static UINT8 SelectAla[] = {0x00, 0xA4, 0x04, 0x00, 0x0D, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x41, 0x4C, 0x41, 0x01, 0x43, 0x4F, 0x52, 0x01};
#endif /* NXP_LDR_SVC_VER_2*/

static UINT8 OpenChannel[] = {0x00, 0x70, 0x00, 0x00, 0x01};

tJBL_STATUS (*ls_GetStatus_seqhandler[])(Ala_ImageInfo_t *pContext, tJBL_STATUS status, Ala_TranscieveInfo_t *pInfo)=
{
    ALA_OpenChannel,
    ALA_SelectAla,
    ALA_getAppletLsStatus,
    ALA_CloseChannel,
    NULL
};
#endif
tJBL_STATUS (*Applet_load_seqhandler[])(Ala_ImageInfo_t *pContext, tJBL_STATUS status, Ala_TranscieveInfo_t *pInfo)=
{
    ALA_OpenChannel,
    ALA_SelectAla,
    ALA_StoreData,
    ALA_loadapplet,
    NULL
};

tJBL_STATUS (*Jsblcer_id_seqhandler[])(Ala_ImageInfo_t *pContext, tJBL_STATUS status, Ala_TranscieveInfo_t *pInfo)=
{
    ALA_OpenChannel,
    ALA_SelectAla,
    ALA_CloseChannel,
    NULL
};

#if(ALA_WEARABLE_BUFFERED == TRUE)

// Follow the convention for fscanf
// will return 1 if there is a match
// will return 0 if there is not a match
// will return -1 if the end of the file is reached before there is a match error.
INT32 getHexByteFromBuffer(const UINT8* pBuf, UINT32 bufSize, UINT32 *pOffset, UINT8* pByte)
{
    INT32 res = EOF;
    UINT8 dwVal;
    *pOffset = Ala_SkipMetaDataAndNonDigits(pBuf, bufSize, *pOffset);
    if((*pOffset+1) < bufSize)
    {
        res = sscanf((const char *)pBuf+*pOffset, "%2X", &dwVal);
        *pByte = (dwVal & 0xFF);
        *pOffset += 2;
        //ALOGD("%s: offset = %d, value = %2X", __func__, *pOffset, *pByte);
    }
    return res;
}

UINT32 countNonDigits(const UINT8* pBuf, UINT32 bufSize, UINT32 *pOffset)
{
    UINT32 nNonDigits = *pOffset;
    *pOffset = Ala_SkipMetaDataAndNonDigits(pBuf, bufSize, *pOffset);
    nNonDigits = *pOffset - nNonDigits;
    return nNonDigits;
}

void byteToHex(char* destBuf, UINT8 byte)
{
	UINT8 tmp;
	if(NULL == destBuf)
		return;

	tmp = ((byte & 0xF0) >> 4);
	if(tmp <= 9)
	{
		*destBuf = tmp + 0x30;
	}
	else
	{
		*destBuf = tmp + 0x37;
	}

	tmp = (byte & 0x0F);
	if(tmp <= 9)
	{
		*(destBuf+1) = tmp + 0x30;
	}
	else
	{
		*(destBuf+1) = tmp + 0x37;
	}

}
#endif
/*******************************************************************************
**
** Function:        initialize
**
** Description:     Initialize all member variables.
**                  native: Native data.
**
** Returns:         True if ok.
**
*******************************************************************************/
BOOLEAN initialize (IChannel_t* channel)
{
    static const char fn [] = "Ala_initialize";
    UNUSED(fn);
#if(ALA_WEARABLE_BUFFERED != TRUE)
    unsigned int temp;
#endif

    ALOGD ("%s: enter", fn);

    gpAla_Dwnld_Context = (pAla_Dwnld_Context_t)GKI_os_malloc(sizeof(Ala_Dwnld_Context_t));
    if(gpAla_Dwnld_Context != NULL)
    {
        memset((void *)gpAla_Dwnld_Context, 0, (UINT32)sizeof(Ala_Dwnld_Context_t));
    }
    else
    {
        ALOGD("%s: Memory allocation failed", fn);
        return (FALSE);
    }
    gpAla_Dwnld_Context->mchannel = channel;
    gpAla_Dwnld_Context->Image_info.last_status = STATUS_OK;
    gpAla_Dwnld_Context->Image_info.tagLen = 0;
    //--------------------------------------------------------------------
    // AID_MEM_PATH contains AN AID of another version of the LoaderApplet
    // 	If it is present, then the this AID is placed in the
    //	3rd ArrayOfAID entry. If it is not present, then the 3rd
    //	entry is filled with the AID found in SelectAla
    //
    // --- It is IGNORED/NOT USED in the buffered implementation
    //	the AID found in SelectAla will be used
    //--------------------------------------------------------------------
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
    memcpy(&ArrayOfAIDs[2][1],&SelectAla[0],sizeof(SelectAla));
    ArrayOfAIDs[2][0] = sizeof(SelectAla);
#else
    fAID_MEM = fopen(AID_MEM_PATH,"r");

    if(fAID_MEM == NULL)
    {
        ALOGD("%s: AID data file does not exists", fn);
        memcpy(&ArrayOfAIDs[2][1],&SelectAla[0],sizeof(SelectAla));
        ArrayOfAIDs[2][0] = sizeof(SelectAla);
    }
    else
    {
        /*Change is required aidLen = 0x00*/
        UINT8 aidLen = 0x00;
        INT32 wStatus = 0;

		while(!(feof(fAID_MEM)))
		{
			/*the length is not incremented*/
			wStatus = fscanf(fAID_MEM,"%2x",&temp);
			ArrayOfAIDs[2][aidLen++] = (unsigned char) (temp & 0x000000FF);
			if(wStatus == 0)
			{
				ALOGE ("%s: exit: Error during read AID data", fn);
				fclose(fAID_MEM);
				return FALSE;
			}
		}
		ArrayOfAIDs[2][0] = aidLen - 1;
		fclose(fAID_MEM);
    }
#endif
    lsExecuteResp[0] = TAG_LSES_RESP;
    lsExecuteResp[1] = TAG_LSES_RSPLEN;
    lsExecuteResp[2] = LS_ABORT_SW1;
    lsExecuteResp[3] = LS_ABORT_SW2;
#endif
    mIsInit = TRUE;
    ALOGD ("%s: exit", fn);
    return (TRUE);
}


/*******************************************************************************
**
** Function:        finalize
**
** Description:     Release all resources.
**
** Returns:         None
**
*******************************************************************************/
void finalize ()
{
    static const char fn [] = "Ala_finalize";
    UNUSED(fn);
    ALOGD ("%s: enter", fn);
    mIsInit       = FALSE;
    if(gpAla_Dwnld_Context != NULL)
    {
        gpAla_Dwnld_Context->mchannel = NULL;
        GKI_os_free(gpAla_Dwnld_Context);
        gpAla_Dwnld_Context = NULL;
    }

    ALOGD ("%s: exit", fn);
}

/*******************************************************************************
**
** Function:        Perform_ALA
**
** Description:     Performs the ALA download sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
tJBL_STATUS Perform_ALA(INT32 nameLen, const char *name, UINT16 *destLen, const char *dest, const UINT8 *pdata,
UINT16 len, UINT8 *respSW)
#else
tJBL_STATUS Perform_ALA(const char *name,const char *dest, const UINT8 *pdata,
UINT16 len, UINT8 *respSW)
#endif
#else
tJBL_STATUS Perform_ALA(const char *name, const UINT8 *pdata, UINT16 len)
#endif
{
    static const char fn [] = "Perform_ALA";
    UINT8 chunkmarker;
    if(Ala_IsChunkingEnabled() == TRUE)
        chunkmarker = name[nameLen- CHUNK_MARKER_OFFSET];
    else
        chunkmarker = LS_FIRST_CHUNK;
    UINT8 seq_offset = 0;
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter; sha-len=%d", fn, len);

    if(mIsInit == FALSE)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else if((pdata == NULL) ||
            (len == 0x00))
    {
        ALOGD ("%s: Invalid SHA-data", fn);
    }
    else
    {
        if((chunkmarker == LS_FIRST_CHUNK) || (chunkmarker == LS_CHUNK_NOT_REQUIRED))
        {
            StoreData[0] = STORE_DATA_TAG;
            StoreData[1] = len;
            memcpy(&StoreData[2], pdata, len);
        }
        else
        {
            seq_offset = 3;
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
        status = ALA_update_seq_handler(Applet_load_seqhandler+seq_offset, nameLen, name, destLen, dest);
#else
        status = ALA_update_seq_handler(Applet_load_seqhandler+seq_offset, name, dest);
#endif
        if((status != STATUS_OK)&&(lsExecuteResp[2] == 0x90)&&
        (lsExecuteResp[3] == 0x00))
        {
            lsExecuteResp[2] = LS_ABORT_SW1;
            lsExecuteResp[3] = LS_ABORT_SW2;
        }
        else if((status == STATUS_OK) && (gpAla_Dwnld_Context->Image_info.last_apducmd_failed) &&
        		(gpAla_Dwnld_Context->Image_info.last_status == STATUS_OK))
        {
        	 lsExecuteResp[2] = 0x90;
        	 lsExecuteResp[3] = 0x00;
        }
        memcpy(&respSW[0],&lsExecuteResp[0],4);
        ALOGD ("%s: lsExecuteScript Response SW=%2x%2x",fn, lsExecuteResp[2],
        lsExecuteResp[3]);
#else
        status = ALA_update_seq_handler(Applet_load_seqhandler+seq_offset, name);
#endif
    }

    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}
#if(NXP_LDR_SVC_VER_2 == FALSE)
/*******************************************************************************
**
** Function:        GetJsbl_Certificate_ID
**
** Description:     Performs the GetJsbl_Certificate_ID sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetJsbl_Certificate_Refkey(UINT8 *pKey, INT32 *pKeylen)
{
    static const char fn [] = "GetJsbl_Certificate_ID";
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter", fn);

    if(mIsInit == FALSE)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else
    {
        status = JsblCerId_seq_handler(Jsblcer_id_seqhandler);
        if(status == STATUS_SUCCESS)
        {
            if(Jsbl_keylen != 0x00)
            {
                *pKeylen = (INT32)Jsbl_keylen;
                memcpy(pKey, Jsbl_RefKey, Jsbl_keylen);
                Jsbl_keylen = 0;
            }
        }
    }

    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        JsblCerId_seq_handler
**
** Description:     Performs get JSBL Certificate Identifier sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JsblCerId_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo))
{
    static const char fn[] = "JsblCerId_seq_handler";
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = (Ala_ImageInfo_t )gpAla_Dwnld_Context->Image_info;
    Ala_TranscieveInfo_t trans_info = (Ala_TranscieveInfo_t )gpAla_Dwnld_Context->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status, &trans_info );
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}
#else

/*******************************************************************************
**
** Function:        GetLs_Version
**
** Description:     Performs the GetLs_Version sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetLs_Version(UINT8 *pVersion)
{
    static const char fn [] = "GetLs_Version";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter", fn);

    if(mIsInit == FALSE)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else
    {
        status = GetVer_seq_handler(Jsblcer_id_seqhandler);
        if(status == STATUS_SUCCESS)
        {
            pVersion[0] = 2;
            pVersion[1] = 0;
            memcpy(&pVersion[2], lsVersionArr, sizeof(lsVersionArr));
            ALOGD("%s: GetLsVersion is =0x0%x%x", fn, lsVersionArr[0],lsVersionArr[1]);
        }
    }
    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        Get_LsAppletStatus
**
** Description:     Performs the Get_LsAppletStatus sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS Get_LsAppletStatus(UINT8 *pVersion)
{
    static const char fn [] = "GetLs_Version";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD ("%s: enter", fn);

    if(mIsInit == FALSE)
    {
        ALOGD ("%s: ALA lib is not initialized", fn);
        status = STATUS_FAILED;
    }
    else
    {
        status = GetLsStatus_seq_handler(ls_GetStatus_seqhandler);
        if(status == STATUS_SUCCESS)
        {
            pVersion[0] = lsGetStatusArr[0];
            pVersion[1] = lsGetStatusArr[1];
            ALOGD("%s: GetLsAppletStatus is =0x0%x%x", fn, lsGetStatusArr[0],lsGetStatusArr[1]);
        }
    }
    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        GetVer_seq_handler
**
** Description:     Performs GetVer_seq_handler sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetVer_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t*
        pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo))
{
    static const char fn[] = "GetVer_seq_handler";
    UNUSED(fn);
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = gpAla_Dwnld_Context->Image_info;
    Ala_TranscieveInfo_t trans_info = gpAla_Dwnld_Context->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status, &trans_info );
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        GetLsStatus_seq_handler
**
** Description:     Performs GetVer_seq_handler sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetLsStatus_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t*
        pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo))
{
    static const char fn[] = "ls_GetStatus_seqhandler";
    UNUSED(fn);
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t update_info = gpAla_Dwnld_Context->Image_info;
    Ala_TranscieveInfo_t trans_info = gpAla_Dwnld_Context->Transcv_Info;
    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

    while((seq_handler[seq_counter]) != NULL )
    {
        status = STATUS_FAILED;
        status = (*(seq_handler[seq_counter]))(&update_info, status, &trans_info );
        if(STATUS_SUCCESS != status)
        {
            ALOGE("%s: exiting; status=0x0%X", fn, status);
            break;
        }
        seq_counter++;
    }

    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}
#endif
/*******************************************************************************
**
** Function:        ALA_update_seq_handler
**
** Description:     Performs the ALA update sequence handler sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
tJBL_STATUS ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])
        (Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t*
                pInfo), INT32 nameLen, const char *name, UINT16 *destLen, const char *dest)
#else
tJBL_STATUS ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])
        (Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t*
                pInfo), const char *name, const char *dest)
#endif
#else
tJBL_STATUS ALA_update_seq_handler(tJBL_STATUS (*seq_handler[])(Ala_ImageInfo_t* pContext, tJBL_STATUS status, Ala_TranscieveInfo_t* pInfo), const char *name)
#endif
{
    static const char fn[] = "ALA_update_seq_handler";
    UINT8 chunkmarker;
    UNUSED(fn);
    UINT16 seq_counter = 0;
    Ala_ImageInfo_t* pUpdate_info = &(gpAla_Dwnld_Context->Image_info);
    Ala_TranscieveInfo_t *pTrans_info = &gpAla_Dwnld_Context->Transcv_Info;
    if(Ala_IsChunkingEnabled() == TRUE)
    {
        chunkmarker = name[nameLen- CHUNK_MARKER_OFFSET];
        pUpdate_info->totalScriptLen |= (name[nameLen-TOTAL_SCRIPT_LEN_OFFSET] << 24);
        pUpdate_info->totalScriptLen |= (name[nameLen-TOTAL_SCRIPT_LEN_OFFSET+1] << 16);
        pUpdate_info->totalScriptLen |= (name[nameLen-TOTAL_SCRIPT_LEN_OFFSET+2] << 8);
        pUpdate_info->totalScriptLen |=  name[nameLen-TOTAL_SCRIPT_LEN_OFFSET+3];
    }
    else
        chunkmarker = LS_FIRST_CHUNK;

    tJBL_STATUS status = STATUS_FAILED;
    ALOGD("%s: enter", fn);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    if(dest != NULL)
    {
#if(ALA_WEARABLE_BUFFERED == TRUE)
    	memset((void *) dest, 0, *destLen);
        pUpdate_info->fls_RespBuf = (char *) dest;

    if(Ala_IsChunkingEnabled() == TRUE)
            pUpdate_info->fls_RespSize = *destLen - RSP_CHUNK_FOOTER_SIZE;/* 2-bytes are reserved for Command Chunk Buffer Offset */
    else
            pUpdate_info->fls_RespSize = *destLen;


        pUpdate_info->fls_RespCurPos = 0;
        ALOGD("Loader Service response data buffer provided: addr = 0x%X, size = %d", (unsigned int) dest, *destLen);
#else
        strcat(pUpdate_info->fls_RespPath, dest);
        ALOGD("Loader Service response data path/destination: %s", dest);
#endif
        pUpdate_info->bytes_wrote = 0xAA;
    }
    else
    {
        pUpdate_info->bytes_wrote = 0x55;
    }

    if(pUpdate_info->last_status == STATUS_BUFFER_OVERFLOW)
    {
        ALOGE("%s: Before Write Response", fn);
        status = Write_Response_To_OutFile(pUpdate_info, pTrans_info->sRecvData, pUpdate_info->recvLen, 0);/*Last param is ignored*/
        if(pUpdate_info->last_apducmd_failed)
        {
        	*destLen = pUpdate_info->fls_RespCurPos;
        	goto cleanup;
        }
    }
    if((pUpdate_info->last_status != STATUS_BUFFER_OVERFLOW) ||
       (pUpdate_info->last_status == STATUS_BUFFER_OVERFLOW && status == STATUS_OK))
    {

    //    if((ALA_UpdateExeStatus(LS_DEFAULT_STATUS))!= TRUE)
    //    {
    //        return FALSE;
    //    }
#endif
        //memcpy(update_info.fls_path, (char*)Ala_path, sizeof(Ala_path));
#if(ALA_WEARABLE_BUFFERED == TRUE)
        pUpdate_info->fls_buf = (char *) name;
    if(Ala_IsChunkingEnabled() == TRUE)
            pUpdate_info->fls_size = nameLen-CMD_CHUNK_FOOTER_SIZE;// 6-bytes are reserved for chunkmarker and numApdus and total lengh*/
    else
            pUpdate_info->fls_size = nameLen;

        ALOGD("Selected applet provided in buffer: : addr = 0x%X, size = %d", (unsigned int) name, pUpdate_info->fls_size);
#else
        strcat(pUpdate_info->fls_path, name);
        ALOGD("Selected applet to install is: %s", pUpdate_info->fls_path);
#endif

        while((seq_handler[seq_counter]) != NULL )
        {
            status = STATUS_FAILED;
            status = (*(seq_handler[seq_counter]))(pUpdate_info, status,
                    pTrans_info);
            if(STATUS_SUCCESS != status)
            {
                ALOGE("%s: exiting; status=0x0%X", fn, status);
                break;
            }
            seq_counter++;
        }

#if(ALA_WEARABLE_BUFFERED == TRUE)
        if(destLen != NULL)
        {
            *destLen = pUpdate_info->fls_RespCurPos;
            if(Ala_IsChunkingEnabled() == TRUE)
            {
                        pUpdate_info->bytes_read_before_failure += pUpdate_info->bytes_read;
                        if(pUpdate_info->bytes_read_before_failure == pUpdate_info->totalScriptLen)
                        {
                            pUpdate_info->last_apducmd_failed = TRUE;
                            pUpdate_info->apdus_processed--;/* Tweak to trigger another redundant call with already processed APDU,
                                                               just to return the response of this last APDU */
                        }
                        pUpdate_info->fls_RespBuf[(*destLen)++] = (UINT8)((pUpdate_info->apdus_processed >> 8) & 0xFF); //msb
                        pUpdate_info->fls_RespBuf[(*destLen)++] = (UINT8)(pUpdate_info->apdus_processed & 0x00FF);//lsb
            }
        }
#endif
    }
    cleanup:
    if(Ala_IsChunkingEnabled() == FALSE)
            chunkmarker = LS_LAST_CHUNK;

    if(((chunkmarker == LS_LAST_CHUNK) || (chunkmarker == LS_CHUNK_NOT_REQUIRED)) &&(status != STATUS_BUFFER_OVERFLOW))
    {
        ALA_CloseChannel(pUpdate_info, STATUS_FAILED, pTrans_info);
    }
    pUpdate_info->last_status = status;
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;

}
/*******************************************************************************
**
** Function:        ALA_OpenChannel
**
** Description:     Creates the logical channel with ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_OpenChannel(Ala_ImageInfo_t *Os_info, tJBL_STATUS status,
        Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_OpenChannel";
    UNUSED(fn);
    BOOLEAN stat = FALSE;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        Os_info->channel_cnt = 0x00;//CID10689
        pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
        pTranscv_Info->sSendlength = (INT32)sizeof(OpenChannel);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);
        memcpy(pTranscv_Info->sSendData, OpenChannel, pTranscv_Info->sSendlength);

        ALOGD("%s: Calling Secure Element Transceive", fn);
        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
        if(stat != TRUE &&
           (recvBufferActualSize < 0x03))
        {
#if(NXP_LDR_SVC_VER_2 == TRUE)
            if(recvBufferActualSize == 0x02)
            memcpy(&lsExecuteResp[2],
                    &pTranscv_Info->sRecvData[recvBufferActualSize-2],2);
#endif
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
        }
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] != 0x90) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] != 0x00)))
        {
#if(NXP_LDR_SVC_VER_2 == TRUE)
            memcpy(&lsExecuteResp[2],
                    &pTranscv_Info->sRecvData[recvBufferActualSize-2],2);
#endif
            status = STATUS_FAILED;
            ALOGE("%s: invalid response = 0x%X", fn, status);
        }
        else
        {
            UINT8 cnt = Os_info->channel_cnt;
            Os_info->Channel_Info[cnt].channel_id = pTranscv_Info->sRecvData[recvBufferActualSize-3];
            Os_info->Channel_Info[cnt].isOpend = TRUE;
            Os_info->channel_cnt++;
            status = STATUS_OK;
        }
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_SelectAla
**
** Description:     Creates the logical channel with ala
**                  Channel_id will be used for any communication with Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_SelectAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_SelectAla";
    UNUSED(fn);
    BOOLEAN stat = FALSE;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
#if(NXP_LDR_SVC_VER_2 == TRUE)
    UINT8 selectCnt = 3;
#endif
    ALOGD("%s: enter", fn);

    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        pTranscv_Info->sSendData[0] = Os_info->Channel_Info[0].channel_id;
        pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
        pTranscv_Info->sSendlength = (INT32)sizeof(SelectAla);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    while((selectCnt--) > 0)
    {
        memcpy(&(pTranscv_Info->sSendData[1]), &ArrayOfAIDs[selectCnt][2],
                ((ArrayOfAIDs[selectCnt][0])-1));
        pTranscv_Info->sSendlength = (INT32)ArrayOfAIDs[selectCnt][0];
        /*If NFC/SPI Deinitialize requested*/
#else
        memcpy(&(pTranscv_Info->sSendData[1]), &SelectAla[1], sizeof(SelectAla)-1);
#endif
        ALOGD("%s: Calling Secure Element Transceive with Loader service AID", fn);

        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
        if(stat != TRUE &&
           (recvBufferActualSize == 0x00))
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
#if(NXP_LDR_SVC_VER_2 == TRUE)
            break;
#endif
        }
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)))
        {
            status = Process_SelectRsp(pTranscv_Info->sRecvData, (recvBufferActualSize-2));
            if(status != STATUS_OK)
            {
                ALOGE("%s: Select Ala Rsp doesnt have a valid key; status = 0x%X", fn, status);
            }
#if(NXP_LDR_SVC_VER_2 == TRUE)
           /*If AID is found which is successfully selected break while loop*/
           if(status == STATUS_OK)
           {
               UINT8 totalLen = ArrayOfAIDs[selectCnt][0];
               UINT8 cnt  = 0;
               INT32 wStatus= 0;
               UNUSED(cnt)
               UNUSED(wStatus)
               UNUSED(totalLen)
               status = STATUS_FAILED;

               //--------------------------------------------------------------------
               // AID_MEM_PATH is where the LoaderApplet AID received in the response
               // 	should be stored.
               //
               // --- It is IGNORED/NOT USED in the buffered implementation
               //	the AID is not stored and only the SelectAla AID will be used
               //--------------------------------------------------------------------

#if(ALA_WEARABLE_BUFFERED == TRUE)
               // do nothing if running in buffered mode
               status = STATUS_OK;
               break;
#else
               fAID_MEM = fopen(AID_MEM_PATH,"w+");

               if(fAID_MEM == NULL)
               {
                   ALOGE("Error opening AID data file for writing: %s",
                           strerror(errno));
                   return status;
               }
               while(cnt <= totalLen)
               {
                   wStatus = fprintf(fAID_MEM, "%02x",
                           ArrayOfAIDs[selectCnt][cnt++]);
                   if(wStatus != 2)
                   {
                      ALOGE("%s: Error writing AID data to AID_MEM file: %s",
                              fn, strerror(errno));
                      break;
                   }
               }
               if(wStatus == 2)
                   status = STATUS_OK;
               fclose(fAID_MEM);
               break;
#endif
           }
#endif
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] != 0x90)))
        {
            /*Copy the response SW in failure case*/
            memcpy(&lsExecuteResp[2], &(pTranscv_Info->
                    sRecvData[recvBufferActualSize-2]),2);
        }
#endif
        else
        {
            status = STATUS_FAILED;
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
    }
#endif
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        ALA_StoreData
**
** Description:     It is used to provide the ALA with an Unique
**                  Identifier of the Application that has triggered the ALA script.
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_StoreData(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_StoreData";
    UNUSED(fn);
    BOOLEAN stat = FALSE;
    INT32 recvBufferActualSize = 0;
    INT32 xx=0, len = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        len = StoreData[1] + 2;  //+2 offset is for tag value and length byte
        pTranscv_Info->sSendData[xx++] = STORE_DATA_CLA | (Os_info->Channel_Info[0].channel_id);
        pTranscv_Info->sSendData[xx++] = STORE_DATA_INS;
        pTranscv_Info->sSendData[xx++] = 0x00; //P1
        pTranscv_Info->sSendData[xx++] = 0x00; //P2
        pTranscv_Info->sSendData[xx++] = len;
        memcpy(&(pTranscv_Info->sSendData[xx]), StoreData, len);
        pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
        pTranscv_Info->sSendlength = (INT32)(xx + sizeof(StoreData));
        pTranscv_Info->sRecvlength = 1024;

        ALOGD("%s: Calling Secure Element Transceive", fn);
        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
        if((stat != TRUE) &&
           (recvBufferActualSize == 0x00))
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
        }
        else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
        {
            ALOGE("STORE CMD is successful");
            status = STATUS_SUCCESS;
        }
        else
        {
#if(NXP_LDR_SVC_VER_2 == TRUE)
            /*Copy the response SW in failure case*/
            memcpy(&lsExecuteResp[2], &(pTranscv_Info->sRecvData
                    [recvBufferActualSize-2]),2);
#endif
            status = STATUS_FAILED;
        }
    }
    ALOGE("%s: exit; status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        ALA_loadapplet
**
** Description:     Reads the script from the file and sent to Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_loadapplet(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "ALA_loadapplet";
    int wResult = 0;
    INT32 wLen = 0;
    UINT8 *temp_buf;
    UINT8 len_byte=0, offset =0, chunkmarker = 0;
    UNUSED(wResult);
    UNUSED(fn);
    if(Os_info == NULL || pTranscv_Info == NULL)//CID10882
    {
        ALOGE("%s: invalid parameter", fn);
        return STATUS_FAILED;
    }
    if(Ala_IsChunkingEnabled() == TRUE)
        chunkmarker = Os_info->fls_buf[Os_info->fls_size+CMD_CHUNK_FOOTER_SIZE-CHUNK_MARKER_OFFSET];
    else
        chunkmarker = LS_FIRST_CHUNK;

    Os_info->bytes_read = 0;
    temp_buf = (UINT8*) GKI_os_malloc(1024);
    if(temp_buf == NULL)
    {
        ALOGD("%s: Memory allocation failed", fn);
        return status;
    }

#if(NXP_LDR_SVC_VER_2 == TRUE)
    BOOLEAN reachEOFCheck = FALSE;
    tJBL_STATUS tag40_found = STATUS_FAILED;

    ALOGD("%s: enter", fn);
    //--------------------------------------------------------------------
    // fls_RespPath is the path to a file where responses from
    //	the LoaderService exchanges will be stored.  This is
    //	an OPTIONAL file.
    //
    // --- fls_RespPath is IGNORED/NOT USED in the buffered implementation
    //	In the buffered implementation, a response buffer can be provided
    //	and the pointer to the buffer is stored in fls_RespBuf (with
    //	length fls_RespSize)
    //--------------------------------------------------------------------

    if(Os_info->bytes_wrote == 0xAA)
    {
#if(ALA_WEARABLE_BUFFERED == TRUE)
    	Os_info->fResp = NULL;
#else
        Os_info->fResp = fopen(Os_info->fls_RespPath, "a+");
        if(Os_info->fResp == NULL)
        {
            ALOGE("Error opening response recording file <%s> for reading: %s",
            Os_info->fls_path, strerror(errno));
            goto exit;
        }
        ALOGD("%s: Response OUT FILE path is successfully created", fn);
#endif
    }
    else
    {
        ALOGD("%s: Response Out file is optional as per input", fn);
    }
#endif
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGE("%s: invalid parameter", fn);
        goto exit;
    }

    //--------------------------------------------------------------------
    // fls_path contains the path to the script that will be processed
    //
    // --- fls_path and fp are IGNORED/NOT USED in the buffered implementation
    //	In the buffered implementation, the ALA_ReadScript() function
    //	will extract the script data from the buffer pointed to by
    //	fls_buf (with length fls_size)
    //--------------------------------------------------------------------
#if(ALA_WEARABLE_BUFFERED == TRUE)
    // do nothing if using buffered
    // Os_info->fls_size <-- set in the ALA_update_seq_handler function
    Os_info->fp = NULL;
    Os_info->fls_curPos = 0;
#if (NXP_LDR_SVC_SCRIPT_FORMAT == LS_SCRIPT_BINARY)
    Os_info->apdus_processed = 0;
#endif
#else
    Os_info->fp = fopen(Os_info->fls_path, "r");

    if (Os_info->fp == NULL) {
        ALOGE("Error opening OS image file <%s> for reading: %s",
                    Os_info->fls_path, strerror(errno));
        goto exit;
    }
    wResult = fseek(Os_info->fp, 0L, SEEK_END);
    if (wResult) {
        ALOGE("Error seeking end OS image file %s", strerror(errno));
        goto exit;
    }
    Os_info->fls_size = ftell(Os_info->fp);
    ALOGE("fls_size=%d", Os_info->fls_size);
    if (Os_info->fls_size < 0) {
        ALOGE("Error ftelling file %s", strerror(errno));
        goto exit;
    }
    wResult = fseek(Os_info->fp, 0L, SEEK_SET);
    if (wResult) {
        ALOGE("Error seeking start image file %s", strerror(errno));
        goto exit;
    }
#endif

    if((chunkmarker == LS_FIRST_CHUNK) || (chunkmarker == LS_CHUNK_NOT_REQUIRED))
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        status = ALA_Check_KeyIdentifier(Os_info, status, pTranscv_Info,
            NULL, STATUS_FAILED, 0);
#else
        status = ALA_Check_KeyIdentifier(Os_info, status, pTranscv_Info);
#endif
        if(status != STATUS_OK)
        {
            goto exit;
        }
    }
    else
    {
        status = STATUS_OK;
    }

#if(ALA_WEARABLE_BUFFERED == TRUE)
    while(Os_info->fls_curPos < Os_info->fls_size)
#else
    while(!feof(Os_info->fp) &&
            (Os_info->bytes_read < Os_info->fls_size))
#endif
    {
        len_byte = 0x00;
        offset = 0;
#if(NXP_LDR_SVC_VER_2 == TRUE)
        /*Check if the certificate/ is verified or not*/
        if(status != STATUS_OK)
        {
            goto exit;
        }
#endif
        memset(temp_buf, 0, 1024);
        ALOGE("%s; Start of line processing", fn);
        status = ALA_ReadScript(Os_info, temp_buf);
        if(status != STATUS_OK)
        {
            goto exit;
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if(status == STATUS_OK)
        {
            /*Reset the flag in case further commands exists*/
            reachEOFCheck = FALSE;
        }
#endif
        if(temp_buf[offset] == TAG_ALA_CMD_ID)
        {
            /*
             * start sending the packet to Ala
             * */
            offset = offset+1;
            len_byte = Numof_lengthbytes(&temp_buf[offset], &wLen);
#if(NXP_LDR_SVC_VER_2 == TRUE)
            /*If the len data not present or
             * len is less than or equal to 32*/
            if((len_byte == 0)||(wLen <= 32))
#else
            if((len_byte == 0))
#endif
            {
                ALOGE("Invalid length zero");
#if(NXP_LDR_SVC_VER_2 == TRUE)
                goto exit;
#else
                return status;
#endif
            }
            else
            {
#if(NXP_LDR_SVC_VER_2 == TRUE)
                tag40_found = STATUS_OK;
#endif
                offset = offset+len_byte;
                pTranscv_Info->sSendlength = wLen;
                memcpy(pTranscv_Info->sSendData, &temp_buf[offset], wLen);
            }
#if(NXP_LDR_SVC_VER_2 == TRUE)
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
            if(status != STATUS_OK)
            {

#if(NXP_LDR_SVC_VER_2 == TRUE)
                /*When the switching of LS 6320 case*/
                if(status == STATUS_FILE_NOT_FOUND)
                {
                    /*When 6320 occurs close the existing channels*/
                    ALA_CloseChannel(Os_info,status,pTranscv_Info);

                    status = STATUS_FAILED;
                    status = ALA_OpenChannel(Os_info,status,pTranscv_Info);
                    if(status == STATUS_OK)
                    {
                        ALOGD("SUCCESS:Post Switching LS open channel");
                        status = STATUS_FAILED;
                        status = ALA_SelectAla(Os_info,status,pTranscv_Info);
                        if(status == STATUS_OK)
                        {
                            ALOGD("SUCCESS:Post Switching LS select");
                            status = STATUS_FAILED;
                            status = ALA_StoreData(Os_info,status,pTranscv_Info);
                            if(status == STATUS_OK)
                            {
                                /*Enable certificate and signature verification*/
                                tag40_found = STATUS_OK;
                                lsExecuteResp[2] = 0x90;
                                lsExecuteResp[3] = 0x00;
                                reachEOFCheck = TRUE;
                                continue;
                            }
                            ALOGE("Post Switching LS store data failure");
                        }
                        ALOGE("Post Switching LS select failure");
                    }
                    ALOGE("Post Switching LS failure");
                }
                ALOGE("Sending packet to ala failed");
                goto exit;
#else
                return status;
#endif
            }
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if((temp_buf[offset] == (0x7F))&&(temp_buf[offset+1] == (0x21)))
        {
            ALOGD("TAGID: Encountered again certificate tag 7F21");
            if(tag40_found == STATUS_OK)
            {
            ALOGD("2nd Script processing starts with reselect");
            status = STATUS_FAILED;
            status = ALA_SelectAla(Os_info,status,pTranscv_Info);
            if(status == STATUS_OK)
            {
                ALOGD("2nd Script select success next store data command");
                status = STATUS_FAILED;
                status = ALA_StoreData(Os_info,status,pTranscv_Info);
                if(status == STATUS_OK)
                {
                    ALOGD("2nd Script store data success next certificate verification");
                    offset = offset+2;
                    len_byte = Numof_lengthbytes(&temp_buf[offset], &wLen);
                    status = ALA_Check_KeyIdentifier(Os_info, status, pTranscv_Info,
                    temp_buf, STATUS_OK, wLen+len_byte+2);
                    }
                }
                /*If the certificate and signature is verified*/
                if(status == STATUS_OK)
                {
                    /*If the certificate is verified for 6320 then new
                     * script starts*/
                    tag40_found = STATUS_FAILED;
                }
                /*If the certificate or signature verification failed*/
                else{
                  goto exit;
                }
            }
            /*Already certificate&Sginature verified previously skip 7f21& tag 60*/
            else
            {
                memset(temp_buf, 0, sizeof(temp_buf));
                status = ALA_ReadScript(Os_info, temp_buf);
                if(status != STATUS_OK)
                {
                    ALOGE("%s; Next Tag has to TAG 60 not found", fn);
                    goto exit;
                }
                if(temp_buf[offset] == TAG_JSBL_HDR_ID)
                continue;
                else
                    goto exit;
            }
        }
#endif
        else
        {
            /*
             * Invalid packet received in between stop processing packet
             * return failed status
             * */
            status = STATUS_FAILED;
            break;
        }
    }
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
    // do nothing for buffered
#else
    if(Os_info->bytes_wrote == 0xAA)
    {
        fclose(Os_info->fResp);
    }
//    ALA_UpdateExeStatus(LS_SUCCESS_STATUS);
#endif
#endif

#if(ALA_WEARABLE_BUFFERED == TRUE)
    // do nothing for buffered
#else
    wResult = fclose(Os_info->fp);
#endif
    GKI_os_free(temp_buf);
    ALOGE("%s exit;End of Load Applet; status=0x%x",fn, status);
    return status;
exit:
    GKI_os_free(temp_buf);
#if(ALA_WEARABLE_BUFFERED == TRUE)
    // do nothing for buffered
#else
    wResult = fclose(Os_info->fp);
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
    // do nothing for buffered
#else
    if(Os_info->bytes_wrote == 0xAA)
    {
        fclose(Os_info->fResp);
    }
#endif
    /*Script ends with SW 6320 and reached END OF FILE*/
    if(reachEOFCheck == TRUE)
    {
       status = STATUS_OK;
//       ALA_UpdateExeStatus(LS_SUCCESS_STATUS);
    }
#endif
    ALOGE("%s close fp and exit; status= 0x%X", fn,status);
    return status;

}
/*******************************************************************************
**
** Function:        ALA_ProcessResp
**
** Description:     Process the response packet received from Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_Check_KeyIdentifier(Ala_ImageInfo_t *Os_info, tJBL_STATUS status,
   Ala_TranscieveInfo_t *pTranscv_Info, UINT8* temp_buf, tJBL_STATUS flag,
   INT32 wNewLen)
#else
tJBL_STATUS ALA_Check_KeyIdentifier(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
#endif
{
    static const char fn[] = "ALA_Check_KeyIdentifier";
#if(NXP_LDR_SVC_VER_2 == TRUE)
    UINT16 offset = 0x00, len_byte=0;
#else
    UINT8 offset = 0x00, len_byte=0;
#endif
    tJBL_STATUS key_found = STATUS_FAILED;
    status = STATUS_FAILED;
    UINT8 *read_buf;
    INT32 wLen = 0;
    UNUSED(key_found)
    UNUSED(wLen)
    UNUSED(fn);
#if(NXP_LDR_SVC_VER_2 == TRUE)
    UINT8 certf_found = STATUS_FAILED;
    UINT8 sign_found = STATUS_FAILED;
#endif
    ALOGD("%s: enter", fn);

    read_buf = (UINT8*) GKI_os_malloc(1024);
    if(read_buf == NULL)
    {
        ALOGD("%s: Memory allocation failed", fn);
        goto exit;
    }
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
    while(Os_info->fls_curPos < Os_info->fls_size)
#else
    while(!feof(Os_info->fp) &&
            (Os_info->bytes_read < Os_info->fls_size))
#endif
    {
        offset = 0x00;
        wLen = 0;
        if(flag == STATUS_OK)
        {
            /*If the 7F21 TAG is already read: After TAG 40*/
            memcpy(read_buf, temp_buf, wNewLen);
            status = STATUS_OK;
            flag   = STATUS_FAILED;
        }
        else
        {
            /*If the 7F21 TAG is not read: Before TAG 40*/
            status = ALA_ReadScript(Os_info, read_buf);
        }
        if(status != STATUS_OK)
            goto exit;
        status = Check_Complete_7F21_Tag(Os_info,pTranscv_Info,
                read_buf, &offset);
        if(status == STATUS_OK || status == STATUS_BUFFER_OVERFLOW)
        {
            ALOGD("%s: Certificate is verified", fn);
            certf_found = STATUS_OK;
            break;
        }
        /*The Loader Service Client ignores all subsequent commands starting by tag
         * �7F21� or tag �60� until the first command starting by tag �40� is found*/
        else if(((read_buf[offset] == TAG_ALA_CMD_ID)&&(certf_found != STATUS_OK)))
        {
            ALOGE("%s: NOT FOUND Root entity identifier's certificate", fn);
            status = STATUS_FAILED;
            goto exit;
        }
    }
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
    memset(read_buf, 0, 1024);
    if(certf_found == STATUS_OK)
    {
#else
        while(!feof(Os_info->fp))
        {
#endif
        offset  = 0x00;
        wLen    = 0;
        status  = ALA_ReadScript(Os_info, read_buf);
        if(status != STATUS_OK)
            goto exit;
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else
            status = STATUS_FAILED;

        if((read_buf[offset] == TAG_JSBL_HDR_ID)&&
        (certf_found != STATUS_FAILED)&&(sign_found != STATUS_OK))
#else
        if(read_buf[offset] == TAG_JSBL_HDR_ID &&
           key_found != STATUS_OK)
#endif
        {
            ALOGD("TAGID: TAG_JSBL_HDR_ID");
            offset = offset+1;
            len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
            offset = offset + len_byte;
#if(NXP_LDR_SVC_VER_2 == FALSE)
            if(read_buf[offset] == TAG_JSBL_KEY_ID)
            {
                ALOGE("TAGID: TAG_JSBL_KEY_ID");
                offset = offset+1;
                wLen = read_buf[offset];
                offset = offset+1;
                key_found = memcmp(&read_buf[offset], Select_Rsp,
                Select_Rsp_Len);

                if(key_found == STATUS_OK)
                {
                    ALOGE("Key is matched");
                    offset = offset + wLen;
#endif
                    if(read_buf[offset] == TAG_SIGNATURE_ID)
                    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
                        offset = offset+1;
                        len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
                        offset = offset + len_byte;
#endif
                        ALOGE("TAGID: TAG_SIGNATURE_ID");

#if(NXP_LDR_SVC_VER_2 == TRUE)
                        pTranscv_Info->sSendlength = wLen+5;

                        pTranscv_Info->sSendData[0] = 0x00;
                        pTranscv_Info->sSendData[1] = 0xA0;
                        pTranscv_Info->sSendData[2] = 0x00;
                        pTranscv_Info->sSendData[3] = 0x00;
                        pTranscv_Info->sSendData[4] = wLen;

                        memcpy(&(pTranscv_Info->sSendData[5]),
                        &read_buf[offset], wLen);
#else
                        offset = offset+1;
                        wLen = 0;
                        len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
                        if(len_byte == 0)
                        {
                            ALOGE("Invalid length zero");
                            return STATUS_FAILED;
                        }
                        else
                        {
                            offset = offset+len_byte;
                            pTranscv_Info->sSendlength = wLen;
                            memcpy(pTranscv_Info->sSendData, &read_buf[offset],
                            wLen);
                        }
#endif
                        ALOGE("%s: start transceive for length %ld", fn,
                        pTranscv_Info->sSendlength);
#if(NXP_LDR_SVC_VER_2 == TRUE)
                        status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Sign);
#else
                        status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
                        if(status != STATUS_OK)
                        {
                            goto exit;
                        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
                        else
                        {
                            sign_found = STATUS_OK;
                        }
#endif
                    }
#if(NXP_LDR_SVC_VER_2 == FALSE)
                }
                else
                {
                    /*
                     * Discard the packet and goto next line
                     * */
                }
            }
            else
            {
                ALOGE("Invalid Tag ID");
                status = STATUS_FAILED;
                break;
            }
#endif
        }
#if(NXP_LDR_SVC_VER_2 == TRUE)
        else if(read_buf[offset] != TAG_JSBL_HDR_ID )
        {
            status = STATUS_FAILED;
        }
#else
        else if(read_buf[offset] == TAG_ALA_CMD_ID &&
                key_found == STATUS_OK)
        {
            /*Key match is success and start sending the packet to Ala
             * return status ok
             * */
            ALOGE("TAGID: TAG_ALA_CMD_ID");
            offset = offset+1;
            wLen = 0;
            len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
            if(len_byte == 0)
            {
                ALOGE("Invalid length zero");
                return STATUS_FAILED;
            }
            else
            {
                offset = offset+len_byte;
                pTranscv_Info->sSendlength = wLen;
                memcpy(pTranscv_Info->sSendData, &read_buf[offset], wLen);
            }

            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
            break;
        }
        else if(read_buf[offset] == TAG_JSBL_HDR_ID &&
                key_found == STATUS_OK)
        {
            /*Key match is success
             * Discard the packets untill we found header T=0x40
             * */
        }
        else
        {
            /*Invalid header*/
            status = STATUS_FAILED;
            break;
        }
#endif

#if(NXP_LDR_SVC_VER_2 == FALSE)
        }
#else
    }
    else
    {
        ALOGE("%s : Exit certificate verification failed", fn);
    }
#endif

exit:
    GKI_os_free(read_buf);
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_ReadScript
**
** Description:     Reads the current line if the script
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_ReadScript(Ala_ImageInfo_t *Os_info, UINT8 *read_buf)
{
    static const char fn[]="ALA_ReadScript";
    UNUSED(fn);
    INT32 wLen;
    UINT8 len_byte = 0;
    tJBL_STATUS status = STATUS_FAILED;
#if (NXP_LDR_SVC_SCRIPT_FORMAT == LS_SCRIPT_BINARY)
    UINT32 fls_prevcurPos;
#else
    INT32 wCount, wIndex = 0;
    int wResult = 0;
    INT32 lenOff = 1;
#endif

    ALOGD("%s: enter", fn);

#if (NXP_LDR_SVC_SCRIPT_FORMAT == LS_SCRIPT_BINARY)
    if(Os_info->fls_curPos + 2 > Os_info->fls_size)
        return STATUS_FAILED;

    fls_prevcurPos = Os_info->fls_curPos;

    /** Process the 'T' field **/
    if((Os_info->fls_buf[Os_info->fls_curPos]==0x7f) && (Os_info->fls_buf[Os_info->fls_curPos+1]==0x21))
    {
        Os_info->bytes_read += 2;
        Os_info->fls_curPos += 2;
        if(Os_info->fls_curPos + 1 >= Os_info->fls_size) //2 bytes length field
        {
            ALOGE("%s: Exit Read Script failed in 7F21 ", fn);
            return STATUS_FAILED;
        }
    }
    else if((Os_info->fls_buf[Os_info->fls_curPos] == 0x40)||(Os_info->fls_buf[Os_info->fls_curPos] == 0x60))
    {
        Os_info->bytes_read++;
        Os_info->fls_curPos++;
        if(Os_info->fls_curPos >= Os_info->fls_size) //1 byte length field
        {
            ALOGE("%s: Exit Read Script failed in 0x40 or 0x60 ", fn);
            return STATUS_FAILED;
        }
    }
    /*If TAG is neither 7F21 nor 60 nor 40 then ABORT execution*/
    else
    {
        ALOGE("Invalid TAG 0x%X found in the script", Os_info->fls_buf[Os_info->fls_curPos]);
        return STATUS_FAILED;
    }

    /** Process the 'L' field **/
    if(Os_info->fls_buf[Os_info->fls_curPos] == 0x00)
    {
        ALOGE("Invalid length zero");
        len_byte = 0x00;
        return STATUS_FAILED;
    }
    else if((Os_info->fls_buf[Os_info->fls_curPos] & 0x80) == 0x80)
    {
        len_byte = Os_info->fls_buf[Os_info->fls_curPos] & 0x0F;
        len_byte = len_byte +1; //1 byte added for byte 0x81
        Os_info->fls_curPos++;
        Os_info->bytes_read++;

        ALOGD("%s: Length byte Read from 0x80 is 0x%x ", fn, len_byte);
        if(len_byte == 0x02)
        {
            if(Os_info->fls_curPos >= Os_info->fls_size)
            {
                ALOGE("%s: Exit Read Script failed in length 0x02 ", fn);
                return STATUS_FAILED;
            }

            wLen = Os_info->fls_buf[Os_info->fls_curPos];
            Os_info->bytes_read++;
            Os_info->fls_curPos++;
            ALOGD("%s: Length of Read Script in len_byte= 0x02 is 0x%lx ", fn, wLen);
        }
        else if(len_byte == 0x03)
        {
            if(Os_info->fls_curPos + 1 >= Os_info->fls_size)
            {
                ALOGE("%s: Exit Read Script failed in length 0x03 ", fn);
                return STATUS_FAILED;
            }

            wLen = Os_info->fls_buf[Os_info->fls_curPos++]; //Length of the packet send to ALA
            wLen = ((wLen << 8) | (Os_info->fls_buf[Os_info->fls_curPos++]));
            Os_info->bytes_read += 2;
            ALOGD("%s: Length of Read Script in len_byte= 0x03 is 0x%lx ", fn, wLen);
        }
        else
        {
            /*Need to provide the support if length is more than 2 bytes*/
            ALOGE("Length recived is greater than 3");
            return STATUS_FAILED;
        }
    }
    else
    {
        len_byte = 0x01;
        wLen = Os_info->fls_buf[Os_info->fls_curPos++];
        Os_info->bytes_read++;
        ALOGE("%s: Length of Read Script in len_byte= 0x01 is 0x%lx ", fn, wLen);
    }

    /** Process the 'V' field **/
    if(Os_info->fls_curPos + wLen-1 >= Os_info->fls_size)
    {
        ALOGE("%s: Exit Read Script failed in fscanf function ", fn);
        return status;
    }
    else
    {
        memcpy(read_buf, Os_info->fls_buf+fls_prevcurPos, Os_info->fls_curPos-fls_prevcurPos+wLen);
        Os_info->bytes_read += wLen;
        Os_info->fls_curPos += wLen;
        Os_info->apdus_processed++;
        status = STATUS_OK;
    }

    ALOGD("%s: exit: status=0x%x; Num of bytes read=%d",
    fn, status, Os_info->bytes_read);


#else
#if(ALA_WEARABLE_BUFFERED == TRUE)
    for(wCount =0; (wCount < 2 && (Os_info->fls_curPos < (UINT32) (Os_info->fls_size) ) ); wCount++, wIndex++)
    {
    	wResult = getHexByteFromBuffer((const UINT8*) Os_info->fls_buf, Os_info->fls_size, &(Os_info->fls_curPos), &read_buf[wIndex]);
    }
#else
    for(wCount =0; (wCount < 2 && !feof(Os_info->fp)); wCount++, wIndex++)
    {
        wResult = fscanf(Os_info->fp,"%2hhX",(unsigned int*)&read_buf[wIndex]);
    }
#endif

    if(wResult == 0 || wResult == EOF)
        return STATUS_FAILED;

    Os_info->bytes_read = Os_info->bytes_read + (wCount*2);

#if(NXP_LDR_SVC_VER_2 == TRUE)
    if((read_buf[0]==0x7f) && (read_buf[1]==0x21))
    {
#if(ALA_WEARABLE_BUFFERED == TRUE)
        for(wCount =0; (wCount < 1 && (Os_info->fls_curPos < (UINT32) (Os_info->fls_size) ) ); wCount++, wIndex++)
        {
        	wResult = getHexByteFromBuffer((const UINT8*) Os_info->fls_buf, Os_info->fls_size, &(Os_info->fls_curPos), &read_buf[wIndex]);
        }
#else
        for(wCount =0; (wCount < 1 && !feof(Os_info->fp)); wCount++, wIndex++)
        {
            wResult = fscanf(Os_info->fp,"%2hhX",(unsigned int*)&read_buf[wIndex]);
        }
#endif
        if(wResult == 0 || wResult == EOF)
        {
            ALOGE("%s: Exit Read Script failed in 7F21 ", fn);
            return STATUS_FAILED;
        }
        /*Read_Script from wCount*2 to wCount*1 */
        Os_info->bytes_read = Os_info->bytes_read + (wCount*2);
        lenOff = 2;
    }
    else if((read_buf[0] == 0x40)||(read_buf[0] == 0x60))
    {
        lenOff = 1;
    }
    /*If TAG is neither 7F21 nor 60 nor 40 then ABORT execution*/
    else
    {
        ALOGE("Invalid TAG 0x%X found in the script", read_buf[0]);
        return STATUS_FAILED;
    }
#endif

    if(read_buf[lenOff] == 0x00)
    {
        ALOGE("Invalid length zero");
        len_byte = 0x00;
        return STATUS_FAILED;
    }
    else if((read_buf[lenOff] & 0x80) == 0x80)
    {
        len_byte = read_buf[lenOff] & 0x0F;
        len_byte = len_byte +1; //1 byte added for byte 0x81

        ALOGD("%s: Length byte Read from 0x80 is 0x%x ", fn, len_byte);

        if(len_byte == 0x02)
        {
#if(ALA_WEARABLE_BUFFERED == TRUE)
			for(wCount =0; (wCount < 1 && (Os_info->fls_curPos < (UINT32) (Os_info->fls_size) ) ); wCount++, wIndex++)
			{
	        	wResult = getHexByteFromBuffer((const UINT8*) Os_info->fls_buf, Os_info->fls_size, &(Os_info->fls_curPos), &read_buf[wIndex]);
			}
#else
            for(wCount =0; (wCount < 1 && !feof(Os_info->fp)); wCount++, wIndex++)
            {
                wResult = fscanf(Os_info->fp,"%2hhX",(unsigned int*)&read_buf[wIndex]);
            }
#endif
            if(wResult == 0 || wResult == EOF)
            {
                ALOGE("%s: Exit Read Script failed in length 0x02 ", fn);
                return STATUS_FAILED;
            }

            wLen = read_buf[lenOff+1];
            Os_info->bytes_read = Os_info->bytes_read + (wCount*2);
            ALOGD("%s: Length of Read Script in len_byte= 0x02 is 0x%lx ", fn, wLen);
        }
        else if(len_byte == 0x03)
        {
#if(ALA_WEARABLE_BUFFERED == TRUE)
			for(wCount =0; (wCount < 2 && (Os_info->fls_curPos < (UINT32) (Os_info->fls_size) ) ); wCount++, wIndex++)
			{
	        	wResult = getHexByteFromBuffer((const UINT8*) Os_info->fls_buf, Os_info->fls_size, &(Os_info->fls_curPos), &read_buf[wIndex]);
			}
#else
            for(wCount =0; (wCount < 2 && !feof(Os_info->fp)); wCount++, wIndex++)
            {
                wResult = fscanf(Os_info->fp,"%2hhX",(unsigned int*)&read_buf[wIndex]);
            }
#endif
            if(wResult == 0 || wResult == EOF)
            {
                ALOGE("%s: Exit Read Script failed in length 0x03 ", fn);
                return STATUS_FAILED;
            }

            Os_info->bytes_read = Os_info->bytes_read + (wCount*2);
            wLen = read_buf[lenOff+1]; //Length of the packet send to ALA
            wLen = ((wLen << 8) | (read_buf[lenOff+2]));
            ALOGD("%s: Length of Read Script in len_byte= 0x03 is 0x%lx ", fn, wLen);
        }
        else
        {
            /*Need to provide the support if length is more than 2 bytes*/
            ALOGE("Length recived is greater than 3");
            return STATUS_FAILED;
        }
    }
    else
    {
        len_byte = 0x01;
        wLen = read_buf[lenOff];
        ALOGE("%s: Length of Read Script in len_byte= 0x01 is 0x%lx ", fn, wLen);
    }


#if(ALA_WEARABLE_BUFFERED == TRUE)
	for(wCount =0; (wCount < wLen && (Os_info->fls_curPos < (UINT32) (Os_info->fls_size) ) ); wCount++, wIndex++)
	{
    	wResult = getHexByteFromBuffer((const UINT8*) Os_info->fls_buf, Os_info->fls_size, &(Os_info->fls_curPos), &read_buf[wIndex]);
	}
#else
    for(wCount =0; (wCount < wLen && !feof(Os_info->fp)); wCount++, wIndex++)
    {
        wResult = fscanf(Os_info->fp,"%2hhX",(unsigned int*)&read_buf[wIndex]);
    }
#endif

    if(wResult == 0 || wResult == EOF)
    {
        ALOGE("%s: Exit Read Script failed in fscanf function ", fn);
        return status;
    }
    else
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        Os_info->bytes_read = Os_info->bytes_read + (wCount*2) + countNonDigits((const UINT8*) Os_info->fls_buf, Os_info->fls_size, &(Os_info->fls_curPos));
#else
        Os_info->bytes_read = Os_info->bytes_read + (wCount*2)+2; //not sure why 2 added
#endif
        status = STATUS_OK;
    }

    ALOGD("%s: exit: status=0x%x; Num of bytes read=%d and index=%ld",
    fn, status, Os_info->bytes_read,wIndex);
    #ifdef COMPANION_DEVICE
        LogArray("EncryptedScriptCommand", read_buf, wIndex);
    #endif

#endif

    return status;
}

/*******************************************************************************
**
** Function:        ALA_SendtoEse
**
** Description:     It is used to send the packet to p61
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_SendtoEse(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "ALA_SendtoEse";
    UNUSED(fn);
    BOOLEAN stat =FALSE, chanl_open_cmd = FALSE;
    status = STATUS_FAILED;
    INT32 recvBufferActualSize=0;
    UINT8 cnt;
    ALOGD("%s: enter", fn);
    if(pTranscv_Info->sSendData[1] == 0x70)
    {
        if(pTranscv_Info->sSendData[2] == 0x00)
        {
            ALOGE("Channel open");
            chanl_open_cmd = TRUE;
        }
        else
        {
            ALOGE("Channel close");
            for(cnt=0; cnt < Os_info->channel_cnt; cnt++)
            {
                if(Os_info->Channel_Info[cnt].channel_id == pTranscv_Info->sSendData[3])
                {
                    ALOGE("Closed channel id = 0x0%x", Os_info->Channel_Info[cnt].channel_id);
                    Os_info->Channel_Info[cnt].isOpend = FALSE;
                }
            }
        }
    }
    pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
    pTranscv_Info->sRecvlength = 1024;
    stat = gpAla_Dwnld_Context->mchannel->transceive(pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
    if(stat != TRUE)
    {
        ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
    }
    else
    {
        if(chanl_open_cmd == TRUE)
        {
            if((recvBufferActualSize == 0x03) &&
               ((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)))
            {
                ALOGE("open channel success");
                UINT8 cnt = Os_info->channel_cnt;
                Os_info->Channel_Info[cnt].channel_id = pTranscv_Info->sRecvData[recvBufferActualSize-3];
                Os_info->Channel_Info[cnt].isOpend = TRUE;
                Os_info->channel_cnt++;
            }
            else
            {
                ALOGE("channel open faield");
            }
        }
        status = Process_EseResponse(pTranscv_Info, recvBufferActualSize, Os_info);
    }
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        ALA_SendtoAla
**
** Description:     It is used to forward the packet to Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_SendtoAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info, Ls_TagType tType)
#else
tJBL_STATUS ALA_SendtoAla(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
#endif
{
    static const char fn [] = "ALA_SendtoAla";
    UNUSED(fn);
    BOOLEAN stat =FALSE;
    status = STATUS_FAILED;
    INT32 recvBufferActualSize = 0;
    ALOGD("%s: enter", fn);
#if(NXP_LDR_SVC_VER_2 == TRUE)
    pTranscv_Info->sSendData[0] = (0x80 | Os_info->Channel_Info[0].channel_id);
#else
    pTranscv_Info->sSendData[0] = (pTranscv_Info->sSendData[0] | Os_info->Channel_Info[0].channel_id);
#endif
    pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
    pTranscv_Info->sRecvlength = 1024;

    stat = gpAla_Dwnld_Context->mchannel->transceive(pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
    if(stat != TRUE)
    {
        ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
    }
    else
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        status = ALA_ProcessResp(Os_info, recvBufferActualSize, pTranscv_Info, tType);
#else
        status = ALA_ProcessResp(Os_info, recvBufferActualSize, pTranscv_Info);
#endif
    }
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_CloseChannel
**
** Description:     Closes the previously opened logical channel
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_CloseChannel(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn [] = "ALA_CloseChannel";
    UNUSED(fn);
    status = STATUS_FAILED;
    BOOLEAN stat = FALSE;
    UINT8 xx =0;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;
    UINT8 cnt = 0;
    ALOGD("%s: enter",fn);

    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGE("Invalid parameter");
    }
    else
    {
        for(cnt =0; (cnt < Os_info->channel_cnt); cnt++)
        {
            if(Os_info->Channel_Info[cnt].isOpend == FALSE)
                continue;
            xx = 0;
            pTranscv_Info->sSendData[xx++] = Os_info->Channel_Info[cnt].channel_id;
            pTranscv_Info->sSendData[xx++] = 0x70;
            pTranscv_Info->sSendData[xx++] = 0x80;
            pTranscv_Info->sSendData[xx++] = Os_info->Channel_Info[cnt].channel_id;
            pTranscv_Info->sSendData[xx++] = 0x00;
            pTranscv_Info->sSendlength = xx;
            pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
            pTranscv_Info->sRecvlength = 1024;
            stat = mchannel->transceive(pTranscv_Info->sSendData,
                                        pTranscv_Info->sSendlength,
                                        pTranscv_Info->sRecvData,
                                        pTranscv_Info->sRecvlength,
                                        &recvBufferActualSize,
                                        pTranscv_Info->timeout);
            if(stat != TRUE &&
               recvBufferActualSize < 2)
            {
                ALOGE("%s: Transceive failed; status=0x%X", fn, stat);
            }
            else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                    (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
            {
                ALOGE("Close channel id = 0x0%x is success", Os_info->Channel_Info[cnt].channel_id);
                status = STATUS_OK;
            }
            else
            {
                ALOGE("Close channel id = 0x0%x is failed", Os_info->Channel_Info[cnt].channel_id);
            }
        }

    }
    ALOGD("%s: exit; status=0x0%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_ProcessResp
**
** Description:     Process the response packet received from Ala
**
** Returns:         Success if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
tJBL_STATUS ALA_ProcessResp(Ala_ImageInfo_t *image_info, INT32 recvlen, Ala_TranscieveInfo_t *trans_info, Ls_TagType tType)
#else
tJBL_STATUS ALA_ProcessResp(Ala_ImageInfo_t *image_info, INT32 recvlen, Ala_TranscieveInfo_t *trans_info)
#endif
{
    static const char fn [] = "ALA_ProcessResp";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    static INT32 temp_len = 0;
#if(NXP_LDR_SVC_VER_2 == FALSE)
    UINT8 xx =0;
#endif
    char sw[2];

    ALOGD("%s: enter", fn);

    if(trans_info->sRecvData == NULL &&
            recvlen == 0x00)
    {
        ALOGE("%s: Invalid parameter: status=0x%x", fn, status);
        return status;
    }
    else if(recvlen >= 2)
    {
        sw[0] = trans_info->sRecvData[recvlen-2];
        sw[1] = trans_info->sRecvData[recvlen-1];
    }
    else
    {
        ALOGE("%s: Invalid response; status=0x%x", fn, status);
        return status;
    }
#if(NXP_LDR_SVC_VER_2 == TRUE)
    /*Update the Global variable for storing response length*/
    resp_len = recvlen;
    if((sw[0] != 0x63))
    {
        lsExecuteResp[2] = sw[0];
        lsExecuteResp[3] = sw[1];
        ALOGD("%s: Process Response SW; status = 0x%x", fn, sw[0]);
        ALOGD("%s: Process Response SW; status = 0x%x", fn, sw[1]);
    }
#endif
    if((recvlen == 0x02) &&
       (sw[0] == 0x90) &&
       (sw[1] == 0x00))
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        tJBL_STATUS wStatus = STATUS_FAILED;
        ALOGE("%s: Before Write Response", fn);
        wStatus = Write_Response_To_OutFile(image_info, trans_info->sRecvData, recvlen, tType);
        if(wStatus == STATUS_BUFFER_OVERFLOW)
            status = wStatus;
        else if(wStatus != STATUS_FAILED)
#endif
            status = STATUS_OK;
    }
    else if((recvlen > 0x02) &&
            (sw[0] == 0x90) &&
            (sw[1] == 0x00))
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        tJBL_STATUS wStatus = STATUS_FAILED;
        ALOGE("%s: Before Write Response", fn);
        wStatus = Write_Response_To_OutFile(image_info, trans_info->sRecvData, recvlen, tType);
        if(wStatus == STATUS_BUFFER_OVERFLOW)
            status = wStatus;
        else if(wStatus != STATUS_FAILED)
            status = STATUS_OK;
#else
        if(temp_len != 0)
        {
            memcpy((trans_info->sTemp_recvbuf+temp_len), trans_info->sRecvData, (recvlen-2));
            trans_info->sSendlength = temp_len + (recvlen-2);
            memcpy(trans_info->sSendData, trans_info->sTemp_recvbuf, trans_info->sSendlength);
            temp_len = 0;
        }
        else
        {
            memcpy(trans_info->sSendData, trans_info->sRecvData, (recvlen-2));
            trans_info->sSendlength = recvlen-2;
        }
        status = ALA_SendtoEse(image_info, status, trans_info);
#endif
    }
#if(NXP_LDR_SVC_VER_2 == FALSE)
    else if ((recvlen > 0x02) &&
             (sw[0] == 0x61))
    {
        memcpy((trans_info->sTemp_recvbuf+temp_len), trans_info->sRecvData, (recvlen-2));
        temp_len = temp_len + recvlen-2;
        trans_info->sSendData[xx++] = image_info->Channel_Info[0].channel_id;
        trans_info->sSendData[xx++] = 0xC0;
        trans_info->sSendData[xx++] = 0x00;
        trans_info->sSendData[xx++] = 0x00;
        trans_info->sSendData[xx++] = sw[1];
        trans_info->sSendlength = xx;
        status = ALA_SendtoAla(image_info, status, trans_info);
    }
#endif
#if(NXP_LDR_SVC_VER_2 == TRUE)
    else if ((recvlen > 0x02) &&
            (sw[0] == 0x63) &&
            (sw[1] == 0x10))
    {
        if(temp_len != 0)
        {
            memcpy((trans_info->sTemp_recvbuf+temp_len), trans_info->sRecvData, (recvlen-2));
            trans_info->sSendlength = temp_len + (recvlen-2);
            memcpy(trans_info->sSendData, trans_info->sTemp_recvbuf,
                    trans_info->sSendlength);
            temp_len = 0;
        }
        else
        {
            memcpy(trans_info->sSendData, trans_info->sRecvData, (recvlen-2));
            trans_info->sSendlength = recvlen-2;
        }
        status = ALA_SendtoEse(image_info, status, trans_info);
    }
    else if ((recvlen > 0x02) &&
            (sw[0] == 0x63) &&
            (sw[1] == 0x20))
    {
        //--------------------------------------------------------------------
        // AID_MEM_PATH contains AN AID of another version of the LoaderApplet
        // 	If it is present, then the this AID is placed in the
        //	3rd ArrayOfAID entry. If it is not present, then the 3rd
        //	entry is filled with the AID found in SelectAla
        //
        // --- It is IGNORED/NOT USED in the buffered implementation
        //	the AID found in SelectAla will be used.  The response AID is NOT
    	//	stored.
        //-------------------------------------------------------------------
#if(ALA_WEARABLE_BUFFERED == TRUE)
    	return status;
#else
        UINT8 respLen = 0;
        INT32 wStatus = 0;

        AID_ARRAY[0] = recvlen+3;
        AID_ARRAY[1] = 00;
        AID_ARRAY[2] = 0xA4;
        AID_ARRAY[3] = 0x04;
        AID_ARRAY[4] = 0x00;
        AID_ARRAY[5] = recvlen-2;
        memcpy(&AID_ARRAY[6], &trans_info->sRecvData[0],recvlen-2);
        memcpy(&ArrayOfAIDs[2][0], &AID_ARRAY[0], recvlen+4);

        fAID_MEM = fopen(AID_MEM_PATH,"w");

        if (fAID_MEM == NULL) {
            ALOGE("Error opening AID data for writing: %s",strerror(errno));
            return status;
        }

        /*Updating the AID_MEM with new value into AID file*/
        while(respLen <= (recvlen+4))
        {
            wStatus = fprintf(fAID_MEM, "%2x", AID_ARRAY[respLen++]);
            if(wStatus != 2)
            {
                ALOGE("%s: Invalid Response during fprintf; status=0x%x",
                        fn, status);
                fclose(fAID_MEM);
                break;
            }
        }
        if(wStatus == 2)
        {
            status = STATUS_FILE_NOT_FOUND;
        }
        else
        {
           status = STATUS_FAILED;
        }
#endif
    }
    else if((recvlen >= 0x02) &&(
            (sw[0] != 0x90) &&
            (sw[0] != 0x63)&&(sw[0] != 0x61)))
    {
        tJBL_STATUS stat;
        stat = Write_Response_To_OutFile(image_info, trans_info->sRecvData, recvlen, tType);
        if(stat == STATUS_BUFFER_OVERFLOW)
            status = stat;
    }
#endif
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_SendtoEse
**
** Description:     It is used to process the received response packet from p61
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS Process_EseResponse(Ala_TranscieveInfo_t *pTranscv_Info, INT32 recv_len, Ala_ImageInfo_t *Os_info)
{
    static const char fn[] = "Process_EseResponse";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_OK;
    UINT8 xx = 0;
    ALOGD("%s: enter", fn);

    pTranscv_Info->sSendData[xx++] = (CLA_BYTE | Os_info->Channel_Info[0].channel_id);
#if(NXP_LDR_SVC_VER_2 == TRUE)
    pTranscv_Info->sSendData[xx++] = 0xA2;
#else
    pTranscv_Info->sSendData[xx++] = 0xA0;
#endif
    if(recv_len <= 0xFF)
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
        pTranscv_Info->sSendData[xx++] = 0x80;
#else
        pTranscv_Info->sSendData[xx++] = ONLY_BLOCK;
#endif
        pTranscv_Info->sSendData[xx++] = 0x00;
        pTranscv_Info->sSendData[xx++] = (UINT8)recv_len;
        memcpy(&(pTranscv_Info->sSendData[xx]),pTranscv_Info->sRecvData,recv_len);
        pTranscv_Info->sSendlength = xx+ recv_len;
#if(NXP_LDR_SVC_VER_2)
        status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
        status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
    }
    else
    {
        while(recv_len > MAX_SIZE)
        {
            xx = PARAM_P1_OFFSET;
#if(NXP_LDR_SVC_VER_2 == TRUE)
            pTranscv_Info->sSendData[xx++] = 0x00;
#else
            pTranscv_Info->sSendData[xx++] = FIRST_BLOCK;
#endif
            pTranscv_Info->sSendData[xx++] = 0x00;
            pTranscv_Info->sSendData[xx++] = MAX_SIZE;
            recv_len = recv_len - MAX_SIZE;
            memcpy(&(pTranscv_Info->sSendData[xx]),pTranscv_Info->sRecvData,MAX_SIZE);
            pTranscv_Info->sSendlength = xx+ MAX_SIZE;
#if(NXP_LDR_SVC_VER_2 == TRUE)
            /*Need not store Process eSE response's response in the out file so
             * LS_Comm = 0*/
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
            if(status != STATUS_OK)
            {
                ALOGE("Sending packet to Ala failed: status=0x%x", status);
                return status;
            }
        }
        xx = PARAM_P1_OFFSET;
        pTranscv_Info->sSendData[xx++] = LAST_BLOCK;
        pTranscv_Info->sSendData[xx++] = 0x01;
        pTranscv_Info->sSendData[xx++] = recv_len;
        memcpy(&(pTranscv_Info->sSendData[xx]),pTranscv_Info->sRecvData,recv_len);
        pTranscv_Info->sSendlength = xx+ recv_len;
#if(NXP_LDR_SVC_VER_2 == TRUE)
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info, LS_Comm);
#else
            status = ALA_SendtoAla(Os_info, status, pTranscv_Info);
#endif
    }
    ALOGD("%s: exit: status=0x%x", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        Process_SelectRsp
**
** Description:     It is used to process the received response for SELECT ALA cmd
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS Process_SelectRsp(UINT8* Recv_data, INT32 Recv_len)
{
    static const char fn[]="Process_SelectRsp";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    int i = 0, len=0;
    ALOGE("%s: enter", fn);

    if(Recv_data[i] == TAG_SELECT_ID)
    {
        ALOGD("TAG: TAG_SELECT_ID");
        i = i +1;
        len = Recv_data[i];
        i = i+1;
        if(Recv_data[i] == TAG_ALA_ID)
        {
            ALOGD("TAG: TAG_ALA_ID");
            i = i+1;
            len = Recv_data[i];
            i = i + 1 + len; //points to next tag name A5
#if(NXP_LDR_SVC_VER_2 == TRUE)
            //points to TAG 9F08 for LS application version
            if((Recv_data[i] == TAG_LS_VER1)&&(Recv_data[i+1] == TAG_LS_VER2))
            {
                UINT8 lsaVersionLen = 0;
                ALOGD("TAG: TAG_LS_APPLICATION_VER");

                i = i+2;
                lsaVersionLen = Recv_data[i];
                //points to TAG 9F08 LS application version
                i = i+1;
                memcpy(lsVersionArr, &Recv_data[i],lsaVersionLen);

                //points to Identifier of the Root Entity key set identifier
                i = i+lsaVersionLen;

                if(Recv_data[i] == TAG_RE_KEYID)
                {
                    UINT8 rootEntityLen = 0;
                    i = i+1;
                    rootEntityLen = Recv_data[i];
                    UNUSED(rootEntityLen);

                    i = i+1;
                    if(Recv_data[i] == TAG_LSRE_ID)
                    {
                        UINT8 tag42Len = 0;
                        i = i+1;
                        tag42Len = Recv_data[i];
                        //copy the data including length
                        memcpy(tag42Arr, &Recv_data[i], tag42Len+1);
                        i = i+tag42Len+1;

                        if(Recv_data[i] == TAG_LSRE_SIGNID)
                        {
                            UINT8 tag45Len = Recv_data[i+1];
                            memcpy(tag45Arr, &Recv_data[i+1],tag45Len+1);
                            status = STATUS_OK;
                        }
                        else
                        {
                            ALOGE("Invalid Root entity for TAG 45 = 0x%x; "
                            "status=0x%x", Recv_data[i], status);
                            return status;
                        }
                    }
                    else
                    {
                        ALOGE("Invalid Root entity for TAG 42 = 0x%x; "
                        "status=0x%x", Recv_data[i], status);
                        return status;
                    }
                }
                else
                {
                    ALOGE("Invalid Root entity key set TAG ID = 0x%x; "
                    "status=0x%x", Recv_data[i], status);
                    return status;
                }
            }
        }
        else
        {
            ALOGE("Invalid Loader Service AID TAG ID = 0x%x; status=0x%x",
            Recv_data[i], status);
            return status;
        }
    }
    else
    {
        ALOGE("Invalid FCI TAG = 0x%x; status=0x%x", Recv_data[i], status);
        return status;
    }
#else
            if(Recv_data[i] == TAG_PRO_DATA_ID)
            {
                ALOGE("TAG: TAG_PRO_DATA_ID");
                i = i+1;
                len = Recv_data[i];
                i = i + 1; //points to next tag name 61
            }
        }
    }
    else
    {
        /*
         * Invalid start of TAG Name found
         * */
        ALOGE("Invalid TAG ID = 0x%x; status=0x%x", Recv_data[i], status);
        return status;
    }

    if((i < Recv_len) &&
       (Recv_data[i] == TAG_JSBL_KEY_ID))
    {
        /*
         * Valid Key is found
         * Copy the data into Select_Rsp
         * */
        ALOGE("Valid key id is found");
        i = i +1;
        len = Recv_data[i];
        if(len != 0x00)
        {
            i = i+1;
            memcpy(Select_Rsp, &Recv_data[i], len);
            Select_Rsp_Len = len;
            status = STATUS_OK;
        }
        /*
         * Identifier of the certificate storing
         * JSBL encryption key
         * */
        i = i + len;
        if(Recv_data[i] == TAG_JSBL_CER_ID)
        {
            i = i+1;
            len = Recv_data[i];
            if(len != 0x00)
            {
                i = i+1;
                Jsbl_keylen = len;
                memcpy(Jsbl_RefKey, &Recv_data[i], len);
            }
        }
    }
#endif
    ALOGE("%s: Exiting status = 0x%x", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        Numof_lengthbytes
**
** Description:     Checks the number of length bytes and assigns
**                  length value to wLen.
**
** Returns:         Number of Length bytes
**
*******************************************************************************/
UINT8 Numof_lengthbytes(UINT8 *read_buf, INT32 *pLen)
{
    static const char fn[]= "Numof_lengthbytes";
    UNUSED(fn);
    UINT8 len_byte=0, i=0;
    INT32 wLen = 0;
    ALOGE("%s:enter", fn);

    if(read_buf[i] == 0x00)
    {
        ALOGE("Invalid length zero");
        len_byte = 0x00;
    }
    else if((read_buf[i] & 0x80) == 0x80)
    {
        len_byte = read_buf[i] & 0x0F;
        len_byte = len_byte +1; //1 byte added for byte 0x81
    }
    else
    {
        len_byte = 0x01;
    }
    /*
     * To get the length of the value field
     * */
    switch(len_byte)
    {
    case 0:
        wLen = read_buf[0];
        break;
    case 1:
        /*1st byte is the length*/
        wLen = read_buf[0];
        break;
    case 2:
        /*2nd byte is the length*/
        wLen = read_buf[1];
        break;
    case 3:
        /*1st and 2nd bytes are length*/
        wLen = read_buf[1];
        wLen = ((wLen << 8) | (read_buf[2]));
        break;
    case 4:
        /*3bytes are the length*/
        wLen = read_buf[1];
        wLen = ((wLen << 16) | (read_buf[2] << 8));
        wLen = (wLen | (read_buf[3]));
        break;
    default:
        ALOGE("default case");
        break;
    }

    *pLen = wLen;
    ALOGE("%s:exit; len_bytes=0x0%x, Length=%ld", fn, len_byte, *pLen);
    return len_byte;
}
#if(NXP_LDR_SVC_VER_2 == TRUE)
/*******************************************************************************
**
** Function:        Write_Response_To_OutFile
**
** Description:     Write the response to Out file
**                  with length recvlen from buffer RecvData.
**
** Returns:         Success if OK
**
*******************************************************************************/
tJBL_STATUS Write_Response_To_OutFile(Ala_ImageInfo_t *image_info, UINT8* RecvData,
    INT32 recvlen, Ls_TagType tType)
{
#if (NXP_LDR_SVC_SCRIPT_FORMAT != LS_SCRIPT_BINARY)
    INT32 respLen       = 0;
#endif
    tJBL_STATUS wStatus = STATUS_FAILED;
    static const char fn [] = "Write_Response_to_OutFile";
    UNUSED(fn);
#if (NXP_LDR_SVC_SCRIPT_FORMAT != LS_SCRIPT_BINARY)
    INT32 status = 0;
#endif
    UINT8 *tagBuffer = image_info->tagBuffer;
    INT32 tag44Len = 0;
    INT32 tag61Len = 0;
    UINT8 tag43Len = 1;
    UINT8 tag43off = 0;
    UINT8 tag44off = 0;
    UINT8 ucTag44[3] = {0x00,0x00,0x00};
    UINT8 tagLen = 0;
#if (NXP_LDR_SVC_SCRIPT_FORMAT != LS_SCRIPT_BINARY)
    UINT8 tempLen = 0;
    UINT8 bScalingFactor = 2;
#else
    UINT8 bScalingFactor = 1;
#endif

    ALOGE("%s: Enter", fn);
    if(image_info->last_status != STATUS_BUFFER_OVERFLOW)
    {
        tagBuffer[0]=0x61;
        memset(tagBuffer+1, 0, 11);
        tagLen=0;
        /*If the Response out file is NULL or Other than LS commands*/
        if((image_info->bytes_wrote == 0x55)||(tType == LS_Default))
        {
            return STATUS_OK;
        }
        /*Certificate TAG occupies 2 bytes*/
        if(tType == LS_Cert)
        {
            tag43Len = 2;
        }

        /* |TAG | LEN(BERTLV)|                                VAL                         |
         * | 61 |      XX    |  TAG | LEN |     VAL    | TAG | LEN(BERTLV) |      VAL     |
         *                   |  43  | 1/2 | 7F21/60/40 | 44  | apduRespLen | apduResponse |
         **/
        if(recvlen < 0x80)
        {
            tag44Len = 1;
            ucTag44[0] = recvlen;
            tag61Len = recvlen + 4 + tag43Len;

            if(tag61Len&0x80)
            {
                tagBuffer[1] = 0x81;
                tagBuffer[2] = tag61Len;
                tag43off = 3;
                tag44off = 5+tag43Len;
                tagLen = tag44off+2;
            }
            else
            {
                tagBuffer[1] = tag61Len;
                tag43off = 2;
                tag44off = 4+tag43Len;
                tagLen = tag44off+2;
            }
        }
        else if((recvlen >= 0x80)&&(recvlen <= 0xFF))
        {
            ucTag44[0] = 0x81;
            ucTag44[1] = recvlen;
            tag61Len = recvlen + 5 + tag43Len;
            tag44Len = 2;

            if((tag61Len&0xFF00) != 0)
            {
                tagBuffer[1] = 0x82;
                tagBuffer[2] = (tag61Len & 0xFF00)>>8;
                tagBuffer[3] = (tag61Len & 0xFF);
                tag43off = 4;
                tag44off = 6+tag43Len;
                tagLen = tag44off+3;
            }
            else
            {
                tagBuffer[1] = 0x81;
                tagBuffer[2] = (tag61Len & 0xFF);
                tag43off = 3;
                tag44off = 5+tag43Len;
                tagLen = tag44off+3;
            }
        }
        else if((recvlen > 0xFF) &&(recvlen <= 0xFFFF))
        {
            ucTag44[0] = 0x82;
            ucTag44[1] = (recvlen&0xFF00)>>8;
            ucTag44[2] = (recvlen&0xFF);
            tag44Len = 3;

            tag61Len = recvlen + 6 + tag43Len;

            if((tag61Len&0xFF00) != 0)
            {
                tagBuffer[1] = 0x82;
                tagBuffer[2] = (tag61Len & 0xFF00)>>8;
                tagBuffer[3] = (tag61Len & 0xFF);
                tag43off = 4;
                tag44off = 6+tag43Len;
                tagLen = tag44off+4;
            }
        }
        tagBuffer[tag43off] = 0x43;
        tagBuffer[tag43off+1] = tag43Len;
        tagBuffer[tag44off] = 0x44;
        memcpy(&tagBuffer[tag44off+1], &ucTag44[0],tag44Len);


        if(tType == LS_Cert)
        {
            tagBuffer[tag43off+2] = 0x7F;
            tagBuffer[tag43off+3] = 0x21;
        }
        else if(tType == LS_Sign)
        {
            tagBuffer[tag43off+2] = 0x60;
        }
        else if(tType == LS_Comm)
        {
            tagBuffer[tag43off+2] = 0x40;
        }
        else
        {
           /*Do nothing*/
        }
    }
    else
    {
        /* Retrieve the stored values */
        tagLen = image_info->tagLen;
        recvlen = image_info->recvLen;
        image_info->last_status  = STATUS_OK;
    }

    ALOGD("Before Write: image_info->fls_RespCurPos %d taglen %d recvlen %d", image_info->fls_RespCurPos, tagLen, recvlen);
    if(image_info->fls_RespCurPos + (tagLen + recvlen)*bScalingFactor > image_info->fls_RespSize)
    {
        /* Store for accessing later */
        image_info->tagLen = tagLen;
        image_info->recvLen = recvlen;
        /* Postpone the response writing as there is not enough space */
        ALOGE("%s: Out of space in response buffer", fn);
        wStatus = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
#if (NXP_LDR_SVC_SCRIPT_FORMAT == LS_SCRIPT_BINARY)
        memcpy(&image_info->fls_RespBuf[image_info->fls_RespCurPos], tagBuffer, tagLen);
        image_info->fls_RespCurPos += tagLen;
        memcpy(&image_info->fls_RespBuf[image_info->fls_RespCurPos], RecvData, recvlen);
        image_info->fls_RespCurPos += recvlen;
        ALOGE("%s: SUCCESS Response written to output buffer", fn);
        wStatus = STATUS_OK;
#else
        while(tempLen < tagLen)
        {
#if(ALA_WEARABLE_BUFFERED == TRUE)
            if(image_info->fls_RespCurPos + 2 < image_info->fls_RespSize)
            {
                byteToHex(&(image_info->fls_RespBuf[image_info->fls_RespCurPos]), tagBuffer[tempLen++]);
                image_info->fls_RespCurPos += 2;
                image_info->fls_RespBuf[image_info->fls_RespCurPos] = 0x00;
                status = 2;
            }
            else
            {
                status = 0;
                ALOGE("%s: Out of space in response buffer", fn);
                wStatus = STATUS_FAILED;
                break;
            }
#else
            status = fprintf(image_info->fResp, "%02X", tagBuffer[tempLen++]);
            if(status != 2)
            {
                ALOGE("%s: Invalid Response during fprintf; status=0x%lx", fn, (status));
                wStatus = STATUS_FAILED;
                break;
            }
#endif
        }
        /*Updating the response data into out script*/
        while(respLen < recvlen)
        {
#if(ALA_WEARABLE_BUFFERED == TRUE)
            if(image_info->fls_RespCurPos + 2 < image_info->fls_RespSize)
            {
                byteToHex(&(image_info->fls_RespBuf[image_info->fls_RespCurPos]), RecvData[respLen++]);
                image_info->fls_RespCurPos += 2;
                image_info->fls_RespBuf[image_info->fls_RespCurPos] = 0x00;
                status = 2;
            }
            else
            {
                status = 0;
                ALOGE("%s: Out of space in response buffer", fn);
                wStatus = STATUS_FAILED;
                break;
            }

#else
            status = fprintf(image_info->fResp, "%02X", RecvData[respLen++]);
            if(status != 2)
            {
                ALOGE("%s: Invalid Response during fprintf; status=0x%lx", fn, (status));
                wStatus = STATUS_FAILED;
                break;
            }
#endif
        }

        if((status == 2))
        {
#if(ALA_WEARABLE_BUFFERED == TRUE)
            if(image_info->fls_RespCurPos + 1 <= image_info->fls_RespSize)
            {
                image_info->fls_RespBuf[image_info->fls_RespCurPos] = '\n' ;
                image_info->fls_RespCurPos += 1;
                image_info->fls_RespBuf[image_info->fls_RespCurPos] = 0x00;
                ALOGE("%s: SUCCESS Response written to output buffer", fn);
                wStatus = STATUS_OK;
            }
            else
            {
                ALOGE("%s: Partial SUCCESS - Response written to output buffer, but no room for ending carriage return", fn);
                wStatus = STATUS_OK;
            }
#else
            fprintf(image_info->fResp, "%s\n", "");
            ALOGE("%s: SUCCESS Response written to script out file; status=0x%lx", fn, (status));
            wStatus = STATUS_OK;
#endif
        }
#endif
    }
    return wStatus;
}

/*******************************************************************************
**
** Function:        Check_Certificate_Tag
**
** Description:     Check certificate Tag presence in script
**                  by 7F21 .
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_Certificate_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 len_byte = 0;
    INT32 wLen = 0;
    UINT16 offset = *offset1;

    if(((read_buf[offset]<<8|read_buf[offset+1]) == TAG_CERTIFICATE))
    {
        ALOGD("TAGID: TAG_CERTIFICATE");
        offset = offset+2;
        len_byte = Numof_lengthbytes(&read_buf[offset], &wLen);
        offset = offset + len_byte;
        *offset1 = offset;
        if(wLen <= MAX_CERT_LEN)
        status = STATUS_OK;
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_SerialNo_Tag
**
** Description:     Check Serial number Tag presence in script
**                  by 0x93 .
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_SerialNo_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset = *offset1;
    static const char fn[] = "Check_SerialNo_Tag";
    UNUSED(fn);

    if((read_buf[offset] == TAG_SERIAL_NO))
    {
        ALOGD("TAGID: TAG_SERIAL_NO");
        UINT8 serNoLen = read_buf[offset+1];
        offset = offset + serNoLen + 2;
        *offset1 = offset;
        ALOGD("%s: TAG_LSROOT_ENTITY is %x", fn, read_buf[offset]);
        status = STATUS_OK;
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_LSRootID_Tag
**
** Description:     Check LS root ID tag presence in script and compare with
**                  select response root ID value.
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_LSRootID_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    INT32 result = 0;
    UINT16 offset      = *offset1;

    if(read_buf[offset] == TAG_LSRE_ID)
    {
        ALOGD("TAGID: TAG_LSROOT_ENTITY");
        if(tag42Arr[0] == read_buf[offset+1])
        {
            UINT8 tag42Len = read_buf[offset+1];
            offset = offset+2;
            result = memcmp(&read_buf[offset],&tag42Arr[1],tag42Arr[0]);
            ALOGD("ALA_Check_KeyIdentifier : TAG 42 verified");
            (result == 0) ? (status = STATUS_OK):(status = STATUS_FAILED);
            if(status == STATUS_OK)
            {
                ALOGD("ALA_Check_KeyIdentifier : Loader service root entity "
                "ID is matched");
                offset = offset+tag42Len;
                *offset1 = offset;
             }
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_CertHoldID_Tag
**
** Description:     Check certificate holder ID tag presence in script.
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_CertHoldID_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;

    if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_CERTFHOLD_ID)
    {
        UINT8 certfHoldIDLen = 0;
        ALOGD("TAGID: TAG_CERTFHOLD_ID");
        certfHoldIDLen = read_buf[offset+2];
        offset = offset+certfHoldIDLen+3;
        if(read_buf[offset] == TAG_KEY_USAGE)
        {
            UINT8 keyusgLen = 0;
            ALOGD("TAGID: TAG_KEY_USAGE");
            keyusgLen = read_buf[offset+1];
            offset = offset+keyusgLen+2;
            *offset1 = offset;
            status = STATUS_OK;
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        Check_Date_Tag
**
** Description:     Check date tags presence in script.
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_Date_Tag(UINT8 *read_buf, UINT16 *offset1)
{
    tJBL_STATUS status = STATUS_OK;
    UINT16 offset      = *offset1;

    if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_EFF_DATE)
    {
        UINT8 effDateLen = read_buf[offset+2];
        offset = offset+3+effDateLen;
        ALOGD("TAGID: TAG_EFF_DATE");
        if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_EXP_DATE)
        {
            UINT8 effExpLen = read_buf[offset+2];
            offset = offset+3+effExpLen;
            ALOGD("TAGID: TAG_EXP_DATE");
            status = STATUS_OK;
        }else if(read_buf[offset] == TAG_LSRE_SIGNID)
        {
            status = STATUS_OK;
        }
    }
    else if((read_buf[offset]<<8|read_buf[offset+1]) == TAG_EXP_DATE)
    {
        UINT8 effExpLen = read_buf[offset+2];
        offset = offset+3+effExpLen;
        ALOGD("TAGID: TAG_EXP_DATE");
        status = STATUS_OK;
    }else if(read_buf[offset] == TAG_LSRE_SIGNID)
    {
        status = STATUS_OK;
    }
    else
    {
    /*STATUS_FAILED*/
    }
    *offset1 = offset;
    return status;
}


/*******************************************************************************
**
** Function:        Check_45_Tag
**
** Description:     Check 45 tags presence in script and compare the value
**                  with select response tag 45 value
**
** Returns:         Success if Tag found
**
*******************************************************************************/
tJBL_STATUS Check_45_Tag(UINT8 *read_buf, UINT16 *offset1, UINT8 *tag45Len)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;
    INT32 result = 0;
    if(read_buf[offset] == TAG_LSRE_SIGNID)
    {
        *tag45Len = read_buf[offset+1];
        offset = offset+2;
        if(tag45Arr[0] == *tag45Len)
        {
            status = memcmp(&read_buf[offset],&tag45Arr[1],tag45Arr[0]);
            (result == 0) ? (status = STATUS_OK):(status = STATUS_FAILED);
            if(status == STATUS_OK)
            {
                ALOGD("ALA_Check_KeyIdentifier : TAG 45 verified");
                *offset1 = offset;
            }
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        Certificate_Verification
**
** Description:     Perform the certificate verification by forwarding it to
**                  LS applet.
**
** Returns:         Success if certificate is verified
**
*******************************************************************************/
tJBL_STATUS Certificate_Verification(Ala_ImageInfo_t *Os_info,
Ala_TranscieveInfo_t *pTranscv_Info, UINT8 *read_buf, UINT16 *offset1,
UINT8 *tag45Len)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT16 offset      = *offset1;
    INT32 wCertfLen = (read_buf[2]<<8|read_buf[3]);
    tJBL_STATUS certf_found = STATUS_FAILED;
    static const char fn[] = "Certificate_Verification";
    UINT8 tag_len_byte = Numof_lengthbytes(&read_buf[2], &wCertfLen);
    UNUSED(certf_found)
    UNUSED(fn);

    pTranscv_Info->sSendData[0] = 0x80;
    pTranscv_Info->sSendData[1] = 0xA0;
    pTranscv_Info->sSendData[2] = 0x01;
    pTranscv_Info->sSendData[3] = 0x00;
    /*If the certificate is less than 255 bytes*/
    if(wCertfLen <= 251)
    {
        UINT8 tag7f49Off = 0;
        UINT8 u7f49Len = 0;
        UINT8 tag5f37Len = 0;
        UNUSED(tag7f49Off)
        UNUSED(tag5f37Len)
        ALOGD("Certificate is greater than 255");
        offset = offset+*tag45Len;
        ALOGD("%s: Before TAG_CCM_PERMISSION = %x",fn, read_buf[offset]);
        if(read_buf[offset] == TAG_CCM_PERMISSION)
        {
            INT32 tag53Len = 0;
            UINT8 len_byte = 0;
            offset =offset+1;
            len_byte = Numof_lengthbytes(&read_buf[offset], &tag53Len);
            offset = offset+tag53Len+len_byte;
            ALOGD("%s: Verified TAG TAG_CCM_PERMISSION = 0x53",fn);
            if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == TAG_SIG_RNS_COMP)
            {
                tag7f49Off = offset;
                u7f49Len   = read_buf[offset+2];
                offset     = offset+3+u7f49Len;
                if(u7f49Len != 64)
                {
                    return STATUS_FAILED;
                }
                if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == 0x7f49)
                {
                    tag5f37Len = read_buf[offset+2];
                    if(read_buf[offset+3] != 0x86 || (read_buf[offset+4] != 65))
                    {
                        return STATUS_FAILED;
                    }
                }
                else
                {
                    return STATUS_FAILED;
                }
             }
             else
             {
                 return STATUS_FAILED;
             }
        }
        else
        {
            return STATUS_FAILED;
        }
        pTranscv_Info->sSendData[4] = wCertfLen+2+tag_len_byte;
        pTranscv_Info->sSendlength  = wCertfLen+7+tag_len_byte;
        memcpy(&(pTranscv_Info->sSendData[5]), &read_buf[0], wCertfLen+2+tag_len_byte);

        ALOGD("%s: start transceive for length %ld", fn, pTranscv_Info->
            sSendlength);
        status = ALA_SendtoAla(Os_info, status, pTranscv_Info,LS_Cert);
        if(status != STATUS_OK)
        {
            return status;
        }
        else
        {
            certf_found = STATUS_OK;
            ALOGD("Certificate is verified");
            return status;
        }
    }
    /*If the certificate is more than 255 bytes*/
    else
    {
        UINT8 tag7f49Off = 0;
        UINT8 u7f49Len = 0;
        UINT8 tag5f37Len = 0;
        ALOGD("Certificate is greater than 255");
        offset = offset+*tag45Len;
        ALOGD("%s: Before TAG_CCM_PERMISSION = %x",fn, read_buf[offset]);
        if(read_buf[offset] == TAG_CCM_PERMISSION)
        {
            INT32 tag53Len = 0;
            UINT8 len_byte = 0;
            offset =offset+1;
            len_byte = Numof_lengthbytes(&read_buf[offset], &tag53Len);
            offset = offset+tag53Len+len_byte;
            ALOGD("%s: Verified TAG TAG_CCM_PERMISSION = 0x53",fn);
            if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == TAG_SIG_RNS_COMP)
            {
                tag7f49Off = offset;
                u7f49Len   = read_buf[offset+2];
                offset     = offset+3+u7f49Len;
                if(u7f49Len != 64)
                {
                    return STATUS_FAILED;
                }
                if((UINT16)(read_buf[offset]<<8|read_buf[offset+1]) == 0x7f49)
                {
                    tag5f37Len = read_buf[offset+2];
                    if(read_buf[offset+3] != 0x86 || (read_buf[offset+4] != 65))
                    {
                        return STATUS_FAILED;
                    }
                }
                else
                {
                    return STATUS_FAILED;
                }
                pTranscv_Info->sSendData[4] = tag7f49Off;
                memcpy(&(pTranscv_Info->sSendData[5]), &read_buf[0], tag7f49Off);
                pTranscv_Info->sSendlength = tag7f49Off+5;
                ALOGD("%s: start transceive for length %ld", fn,
                pTranscv_Info->sSendlength);

                status = ALA_SendtoAla(Os_info, status, pTranscv_Info,LS_Default);
                if(status != STATUS_OK)
                {

                    UINT8* RecvData = pTranscv_Info->sRecvData;
                    tJBL_STATUS stat;
                    stat = Write_Response_To_OutFile(Os_info, RecvData,
                    resp_len, LS_Cert);
                    if(stat == STATUS_BUFFER_OVERFLOW)
                        status = stat;
                    return status;
                }

                pTranscv_Info->sSendData[2] = 0x00;
                pTranscv_Info->sSendData[4] = u7f49Len+tag5f37Len+6;
                memcpy(&(pTranscv_Info->sSendData[5]), &read_buf[tag7f49Off],
                    u7f49Len+tag5f37Len+6);
                pTranscv_Info->sSendlength = u7f49Len+tag5f37Len+11;
                ALOGD("%s: start transceive for length %ld", fn,
                    pTranscv_Info->sSendlength);

                status = ALA_SendtoAla(Os_info, status, pTranscv_Info,LS_Cert);
                if(status != STATUS_OK)
                {
                    return status;
                }
                else
                {
                    ALOGD("Certificate is verified");
                    certf_found = STATUS_OK;
                    return status;

                }
            }
            else
            {
                return STATUS_FAILED;
            }
        }
        else
        {
            return STATUS_FAILED;
        }
    }
/*return status;*/
}

/*******************************************************************************
**
** Function:        Check_Complete_7F21_Tag
**
** Description:     Traverses the 7F21 tag for verification of each sub tag with
**                  in the 7F21 tag.
**
** Returns:         Success if all tags are verified
**
*******************************************************************************/
tJBL_STATUS Check_Complete_7F21_Tag(Ala_ImageInfo_t *Os_info,
       Ala_TranscieveInfo_t *pTranscv_Info, UINT8 *read_buf, UINT16 *offset)
{
    static const char fn[] = "Check_Complete_7F21_Tag";
    UNUSED(fn);

    if(STATUS_OK == Check_Certificate_Tag(read_buf, offset))
    {
        if(STATUS_OK == Check_SerialNo_Tag(read_buf, offset))
        {
           if(STATUS_OK == Check_LSRootID_Tag(read_buf, offset))
           {
               if(STATUS_OK == Check_CertHoldID_Tag(read_buf, offset))
               {
                   if(STATUS_OK == Check_Date_Tag(read_buf, offset))
                   {
                       UINT8 tag45Len = 0;
                       if(STATUS_OK == Check_45_Tag(read_buf, offset,
                       &tag45Len))
                       {
                           tJBL_STATUS stat;
                           stat = Certificate_Verification(
                           Os_info, pTranscv_Info, read_buf, offset,
                           &tag45Len);
                           if(stat == STATUS_OK || stat == STATUS_BUFFER_OVERFLOW)
                           {
                               return stat;
                           }
                       }else{
                       ALOGE("%s: FAILED in Check_45_Tag", fn);}
                   }else{
                   ALOGE("%s: FAILED in Check_Date_Tag", fn);}
               }else{
               ALOGE("%s: FAILED in Check_CertHoldID_Tag", fn);}
           }else{
           ALOGE("%s: FAILED in Check_LSRootID_Tag", fn);}
        }else{
        ALOGE("%s: FAILED in Check_SerialNo_Tag", fn);}
    }
    else
    {
        ALOGE("%s: FAILED in Check_Certificate_Tag", fn);
    }
return STATUS_FAILED;
}

BOOLEAN ALA_UpdateExeStatus(UINT16 status)
{
    //--------------------------------------------------------------------
    // LS_STATUS_PATH contains the path to a file used to store the
	//	intermediate status of the operation.  This is intended to
	//	allow an external process to see what is happening going on
	//	at any given time.
    //
    // --- It is IGNORED/NOT USED in the buffered implementation
    //--------------------------------------------------------------------
#if(ALA_WEARABLE_BUFFERED != TRUE)
    fLS_STATUS = fopen(LS_STATUS_PATH, "w+");
    ALOGD("enter: ALA_UpdateExeStatus");
    if(fLS_STATUS == NULL)
    {
        ALOGE("Error opening LS Status file for backup: %s",strerror(errno));
        return FALSE;
    }
    if((fprintf(fLS_STATUS, "%04x",status)) != 4)
    {
        ALOGE("Error updating LS Status backup: %s",strerror(errno));
        fclose(fLS_STATUS);
        return FALSE;
    }
    ALOGD("exit: ALA_UpdateExeStatus");
    fclose(fLS_STATUS);
#endif
    return TRUE;
}

/*******************************************************************************
**
** Function:        ALA_getAppletLsStatus
**
** Description:     Interface to fetch Loader service Applet status to JNI, Services
**
** Returns:         SUCCESS/FAILURE
**
*******************************************************************************/
tJBL_STATUS ALA_getAppletLsStatus(Ala_ImageInfo_t *Os_info, tJBL_STATUS status, Ala_TranscieveInfo_t *pTranscv_Info)
{
    static const char fn[] = "ALA_getAppletLsStatus";
    UNUSED(fn);
    BOOLEAN stat = FALSE;
    INT32 recvBufferActualSize = 0;
    IChannel_t *mchannel = gpAla_Dwnld_Context->mchannel;

    ALOGD("%s: enter", fn);

    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGD("%s: Invalid parameter", fn);
    }
    else
    {
        pTranscv_Info->sSendData[0] = STORE_DATA_CLA | Os_info->Channel_Info[0].channel_id;
        pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
        pTranscv_Info->sSendlength = (INT32)sizeof(GetData);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);


        memcpy(&(pTranscv_Info->sSendData[1]), &GetData[1],
                ((sizeof(GetData))-1));
        ALOGD("%s: Calling Secure Element Transceive with GET DATA apdu", fn);

        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
        if((stat != TRUE) &&
           (recvBufferActualSize == 0x00))
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", fn, status);
        }
        else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
        {
            ALOGE("STORE CMD is successful");
            if((pTranscv_Info->sRecvData[0] == 0x46 )&& (pTranscv_Info->sRecvData[1] == 0x01 ))
            {
               if((pTranscv_Info->sRecvData[2] == 0x01))
               {
                   lsGetStatusArr[0]=0x63;lsGetStatusArr[1]=0x40;
                   ALOGE("%s: Script execution status FAILED", fn);
               }
               else if((pTranscv_Info->sRecvData[2] == 0x00))
               {
                   lsGetStatusArr[0]=0x90;lsGetStatusArr[1]=0x00;
                   ALOGE("%s: Script execution status SUCCESS", fn);
               }
               else
               {
                   lsGetStatusArr[0]=0x90;lsGetStatusArr[1]=0x00;
                   ALOGE("%s: Script execution status UNKNOWN", fn);
               }
            }
            else
            {
                lsGetStatusArr[0]=0x90;lsGetStatusArr[1]=0x00;
                ALOGE("%s: Script execution status UNKNOWN", fn);
            }
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_FAILED;
        }

    ALOGE("%s: exit; status=0x%x", fn, status);
    }
    return status;
}

/*******************************************************************************
**
** Function:        Get_LsStatus
**
** Description:     Interface to fetch Loader service client status to JNI, Services
**
** Returns:         SUCCESS/FAILURE
**
*******************************************************************************/
tJBL_STATUS Get_LsStatus(UINT8 *pStatus)
{
    tJBL_STATUS status = STATUS_FAILED;
    UINT8 lsStatus[2]    = {0x63,0x40};
    UINT8 loopcnt = 0;

    UNUSED(status)
    UNUSED(lsStatus)
    UNUSED(loopcnt)

    //--------------------------------------------------------------------
    // LS_STATUS_PATH contains the path to a file used to store the
	//	intermediate status of the operation.  This is intended to
	//	allow an external process to see what is happening going on
	//	at any given time.
    //
    // --- It is IGNORED/NOT USED in the buffered implementation
    //--------------------------------------------------------------------
#if(ALA_WEARABLE_BUFFERED != TRUE)
    fLS_STATUS = fopen(LS_STATUS_PATH, "r");
    if(fLS_STATUS == NULL)
    {
        ALOGE("Error opening LS Status file for backup: %s",strerror(errno));
        return status;
    }
    for(loopcnt=0;loopcnt<2;loopcnt++)
    {
        if((fscanf(fLS_STATUS, "%2hhx",(unsigned int*)&lsStatus[loopcnt])) == 0)
        {
            ALOGE("Error updating LS Status backup: %s",strerror(errno));
            fclose(fLS_STATUS);
            return status;
        }
    }
    ALOGD("enter: ALA_getLsStatus 0x%X 0x%X",lsStatus[0],lsStatus[1] );
    memcpy(pStatus, lsStatus, 2);
    fclose(fLS_STATUS);
    return STATUS_OK;
#else
    return STATUS_FAILED;
#endif
}
#endif
#endif/*LS_ENABLE*/
