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
#ifndef COMPANION_DEVICE
#include <OverrideLog.h>
#include "gki.h"
#else
#include <unistd.h>
#endif
#ifdef ANDROID
#include <semaphore.h>
#endif
#include <JcopOsDownload.h>
#if(JCOP_ENABLE == TRUE)
#include <IChannel.h>
#include <errno.h>
#include <string.h>
#include "inc/Ala.h"

extern INT32 getHexByteFromBuffer(const UINT8* pBuf, UINT32 bufSize, UINT32 *pOffset, UINT8* pByte);

tJBL_STATUS (*JOD_JcopOs_dwnld_seqhandler[])(JcopOs_ImageInfo_t* pContext, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pInfo)={
       &JOD_TriggerApdu,
       &JOD_GetInfo,
       &JOD_load_JcopOS_image,
       &JOD_GetInfo,
       &JOD_load_JcopOS_image,
       &JOD_GetInfo,
       &JOD_load_JcopOS_image,
       NULL
   };

pJcopOs_Dwnld_Context_t gpJcopOs_Dwnld_Context = NULL;

static BOOLEAN mIsInit;
static UINT8 Trigger_APDU[] = {0x4F, 0x70, 0x80, 0x13, 0x04, 0xDE, 0xAD, 0xBE, 0xEF, 0x00};
static UINT8 GetInfo_APDU[] = {0x00, //CLA
                               0xA4, 0x04, 0x00, 0x0C, //INS, P1, P2, Lc
                               0xD2, 0x76, 0x00, 0x00, 0x85, 0x41, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,   //Data
                               0x00 //Le
                              };

tJBL_STATUS GetJcopOsState(JcopOs_ImageInfo_t *Os_info, UINT8 *counter);
tJBL_STATUS SetJcopOsState(JcopOs_ImageInfo_t *Os_info, UINT8 state);

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
BOOLEAN JOD_initialize (IChannel_t *channel, tJcopOs_StorageCb* cb)
{
    ALOGD ("%s: enter", __func__);

    gpJcopOs_Dwnld_Context = (pJcopOs_Dwnld_Context_t)GKI_os_malloc(sizeof(JcopOs_Dwnld_Context_t));
    if(gpJcopOs_Dwnld_Context != NULL)
    {
        memset((void *)gpJcopOs_Dwnld_Context, 0, (UINT32)sizeof(JcopOs_Dwnld_Context_t));
        gpJcopOs_Dwnld_Context->channel = (IChannel_t*)GKI_os_malloc(sizeof(IChannel_t));
        if(gpJcopOs_Dwnld_Context->channel != NULL)
        {
            memset(gpJcopOs_Dwnld_Context->channel, 0, sizeof(IChannel_t));
        }
        else
        {
            ALOGD("%s: Memory allocation for IChannel is failed", __func__);
            return (FALSE);
        }
        gpJcopOs_Dwnld_Context->pJcopOs_TransInfo.sSendData = (UINT8*)GKI_os_malloc(sizeof(UINT8)*JCOP_MAX_BUF_SIZE);
        if(gpJcopOs_Dwnld_Context->pJcopOs_TransInfo.sSendData != NULL)
        {
            memset(gpJcopOs_Dwnld_Context->pJcopOs_TransInfo.sSendData, 0, JCOP_MAX_BUF_SIZE);
        }
        else
        {
            ALOGD("%s: Memory allocation for SendBuf is failed", __func__);
            return (FALSE);
        }
    }
    else
    {
        ALOGD("%s: Memory allocation failed", __func__);
        return (FALSE);
    }
    mIsInit = TRUE;
    memcpy(gpJcopOs_Dwnld_Context->channel, channel, sizeof(IChannel_t));

    gpJcopOs_Dwnld_Context->Image_info.storeCb.cbGet = cb->cbGet;
    gpJcopOs_Dwnld_Context->Image_info.storeCb.cbSet = cb->cbSet;
    ALOGD ("%s: exit", __func__);
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
void JOD_finalize ()
{
    ALOGD ("%s: enter", __func__);
    mIsInit       = FALSE;
    if(gpJcopOs_Dwnld_Context != NULL)
    {
        if(gpJcopOs_Dwnld_Context->channel != NULL)
        {
            GKI_os_free(gpJcopOs_Dwnld_Context->channel);
            gpJcopOs_Dwnld_Context->channel = NULL;
        }
        if(gpJcopOs_Dwnld_Context->pJcopOs_TransInfo.sSendData != NULL)
        {
            GKI_os_free(gpJcopOs_Dwnld_Context->pJcopOs_TransInfo.sSendData);
            gpJcopOs_Dwnld_Context->pJcopOs_TransInfo.sSendData = NULL;
        }
        GKI_os_free(gpJcopOs_Dwnld_Context);
        gpJcopOs_Dwnld_Context = NULL;
    }
    ALOGD ("%s: exit", __func__);
}

/*******************************************************************************
**
** Function:        JcopOs_Download
**
** Description:     Starts the OS download sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JOD_JcopOs_Download(UINT32 lenBuf1, const UINT8 *pBuf1,
								UINT32 lenBuf2, const UINT8 *pBuf2,
								UINT32 lenBuf3, const UINT8 *pBuf3)
{
    tJBL_STATUS wstatus = STATUS_FAILED;
    UINT8 retry_cnt = 0x00;
    ALOGD("%s: enter:", __func__);
    if(mIsInit == FALSE)
    {
        ALOGD ("%s: JcopOs Dwnld is not initialized", __func__);
        wstatus = STATUS_FAILED;
    }
    else
    {

#if(JCDN_WEARABLE_BUFFERED == TRUE)
    	gpJcopOs_Dwnld_Context->Image_info.buf[0] = pBuf1;
    	gpJcopOs_Dwnld_Context->Image_info.buf[1] = pBuf2;
    	gpJcopOs_Dwnld_Context->Image_info.buf[2] = pBuf3;
    	gpJcopOs_Dwnld_Context->Image_info.lenBuf[0] = lenBuf1;
    	gpJcopOs_Dwnld_Context->Image_info.lenBuf[1] = lenBuf2;
    	gpJcopOs_Dwnld_Context->Image_info.lenBuf[2] = lenBuf3;
#endif

        do
        {
            wstatus = JOD_JcopOs_update_seq_handler();
            if(wstatus == STATUS_FAILED)
                retry_cnt++;
            else
                break;
        }while(retry_cnt < JCOP_MAX_RETRY_CNT);
    }
    ALOGD("%s: exit; status = 0x%x", __func__, wstatus);
    return wstatus;
}
/*******************************************************************************
**
** Function:        JcopOs_update_seq_handler
**
** Description:     Performs the JcopOS download sequence
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JOD_JcopOs_update_seq_handler()
{
    UINT8 seq_counter = 0;
    JcopOs_ImageInfo_t *pUpdate_info = &(gpJcopOs_Dwnld_Context->Image_info);
    JcopOs_TranscieveInfo_t *pTrans_info = &(gpJcopOs_Dwnld_Context->pJcopOs_TransInfo);
    tJBL_STATUS status = STATUS_FAILED;

    ALOGD("%s: enter", __func__);
    status = GetJcopOsState(pUpdate_info, &seq_counter);
    if(status != STATUS_SUCCESS)
    {
        ALOGE("Error in getting JcopOsState info");
    }
    else
    {
        /* Mark that a JCOP download was started by setting the state as 0 */
        if(pUpdate_info->cur_state == JCOP_UPDATE_STATE3)
        {
            SetJcopOsState(pUpdate_info, JCOP_UPDATE_STATE0);
            pUpdate_info->cur_state = JCOP_UPDATE_STATE0;
            pUpdate_info->index = JCOP_UPDATE_STATE0;
        }
        ALOGE("seq_counter %d", seq_counter);
        while((JOD_JcopOs_dwnld_seqhandler[seq_counter]) != NULL )
        {
            status = STATUS_FAILED;

            status = JOD_JcopOs_dwnld_seqhandler[seq_counter](pUpdate_info, status, pTrans_info );
            if(STATUS_SUCCESS != status)
            {
                ALOGE("%s: exiting; status=0x0%X", __func__, status);
                break;
            }
            seq_counter++;
        }
    }
    return status;
}

/*******************************************************************************
**
** Function:        TriggerApdu
**
** Description:     Switch to updater OS
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JOD_TriggerApdu(JcopOs_ImageInfo_t* pVersionInfo, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pTranscv_Info)
{
    BOOLEAN stat = FALSE;
    IChannel_t *mchannel = gpJcopOs_Dwnld_Context->channel;
    INT32 recvBufferActualSize = 0;

    ALOGD("%s: enter;", __func__);

    if(pTranscv_Info == NULL ||
       pVersionInfo == NULL)
    {
        ALOGD("%s: Invalid parameter", __func__);
        status = STATUS_FAILED;
    }
    else
    {
        pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
        pTranscv_Info->sSendlength = (INT32)sizeof(Trigger_APDU);
        pTranscv_Info->sRecvlength = 1024;//(INT32)sizeof(INT32);
        memcpy(pTranscv_Info->sSendData, Trigger_APDU, pTranscv_Info->sSendlength);

        ALOGD("%s: Calling Secure Element Transceive", __func__);
        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
        if (stat != TRUE)
        {
            status = STATUS_FAILED;
            ALOGE("%s: SE transceive failed status = 0x%X", __func__, status);//Stop JcopOs Update
        }
        else if(((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x68) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x81))||
               ((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))||
               ((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x6F) &&
               (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)))
        {
        	ALOGD("%s: ---- SE RESET ----",__func__);
            mchannel->doeSE_JcopDownLoadReset();
            status = STATUS_OK;
            ALOGD("%s: Trigger APDU Transceive status = 0x%X", __func__, status);
        }
        else
        {
            /* status {90, 00} */
            status = STATUS_OK;
        }
    }
    ALOGD("%s: exit; status = 0x%X", __func__, status);
    return status;
}
/*******************************************************************************
**
** Function:        GetInfo
**
** Description:     Get the JCOP OS info
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JOD_GetInfo(JcopOs_ImageInfo_t* pImageInfo, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pTranscv_Info)
{
#if(JCDN_WEARABLE_BUFFERED != TRUE)
    static const char *path[3] = {"/data/nfc/JcopOs_Update1.apdu",
                                 "/data/nfc/JcopOs_Update2.apdu",
                                 "/data/nfc/JcopOs_Update3.apdu"};
#endif
    BOOLEAN stat = FALSE;
    IChannel_t *mchannel = gpJcopOs_Dwnld_Context->channel;
    INT32 recvBufferActualSize = 0;

    ALOGD("%s: enter;", __func__);

    if(pTranscv_Info == NULL ||
       pImageInfo == NULL)
    {
        ALOGD("%s: Invalid parameter", __func__);
        status = STATUS_FAILED;
    }
    else
    {
#if(JCDN_WEARABLE_BUFFERED != TRUE)
        memcpy(pImageInfo->fls_path, (char *)path[pImageInfo->index], strlen(path[pImageInfo->index]));
#endif

        memset(pTranscv_Info->sSendData, 0, JCOP_MAX_BUF_SIZE);
        pTranscv_Info->timeout = TRANSCEIVE_TIMEOUT;
        pTranscv_Info->sSendlength = (UINT32)sizeof(GetInfo_APDU);
        pTranscv_Info->sRecvlength = 1024;
        memcpy(pTranscv_Info->sSendData, GetInfo_APDU, pTranscv_Info->sSendlength);

        ALOGD("%s: Calling Secure Element Transceive", __func__);
        stat = mchannel->transceive (pTranscv_Info->sSendData,
                                pTranscv_Info->sSendlength,
                                pTranscv_Info->sRecvData,
                                pTranscv_Info->sRecvlength,
                                &recvBufferActualSize,
                                pTranscv_Info->timeout);
        if (stat != TRUE)
        {
            status = STATUS_FAILED;
            pImageInfo->index =0;
            ALOGE("%s: SE transceive failed status = 0x%X", __func__, status);//Stop JcopOs Update
        }
        else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00))
        {
            pImageInfo->version_info.osid = pTranscv_Info->sRecvData[recvBufferActualSize-6];
            pImageInfo->version_info.ver1 = pTranscv_Info->sRecvData[recvBufferActualSize-5];
            pImageInfo->version_info.ver0 = pTranscv_Info->sRecvData[recvBufferActualSize-4];
            pImageInfo->version_info.OtherValid = pTranscv_Info->sRecvData[recvBufferActualSize-3];
#if 0
            if((pImageInfo->index != 0) &&
               (pImageInfo->version_info.osid == 0x01) &&
               (pImageInfo->version_info.OtherValid == 0x11))
            {
                ALOGE("3-Step update is not required");
                memset(pImageInfo->fls_path,0,sizeof(pImageInfo->fls_path));
                pImageInfo->index=0;
            }
            else
#endif
            {
                ALOGE("Starting 3-Step update");
#if(JCDN_WEARABLE_BUFFERED == TRUE)
                pImageInfo->curBuf = pImageInfo->buf[pImageInfo->index];
                pImageInfo->curLenBuf = pImageInfo->lenBuf[pImageInfo->index];
#else
                memcpy(pImageInfo->fls_path, path[pImageInfo->index], sizeof(path[pImageInfo->index]));
#endif
                pImageInfo->index++;
            }
            status = STATUS_OK;
            ALOGD("%s: GetInfo Transceive status = 0x%X", __func__, status);
        }
        else if((pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x6A) &&
                (pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x82) &&
                 pImageInfo->version_info.ver_status == STATUS_UPTO_DATE)
        {
            status = STATUS_UPTO_DATE;
        }
        else
        {
            status = STATUS_FAILED;
            ALOGD("%s; Invalid response for GetInfo", __func__);
        }
    }

    if (status == STATUS_FAILED)
    {
        ALOGD("%s; status failed, doing reset...", __func__);
        mchannel->doeSE_JcopDownLoadReset();
    }
    ALOGD("%s: exit; status = 0x%X", __func__, status);
    return status;
}
/*******************************************************************************
**
** Function:        load_JcopOS_image
**
** Description:     Used to update the JCOP OS
**                  Get Info function has to be called before this
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS JOD_load_JcopOS_image(JcopOs_ImageInfo_t *Os_info, tJBL_STATUS status, JcopOs_TranscieveInfo_t *pTranscv_Info)
{
    BOOLEAN stat = FALSE;
    int wResult=0;
    INT32 wIndex,wCount=0;
    INT32 wLen;

    IChannel_t *mchannel = gpJcopOs_Dwnld_Context->channel;
    INT32 recvBufferActualSize = 0;
    ALOGD("%s: enter", __func__);
    if(Os_info == NULL ||
       pTranscv_Info == NULL)
    {
        ALOGE("%s: invalid parameter", __func__);
        return status;
    }
#if(JCDN_WEARABLE_BUFFERED == TRUE)
    Os_info->curOffset = 0;
    while(Os_info->curOffset < Os_info->curLenBuf )
#else
    Os_info->fp = fopen(Os_info->fls_path, "r+");

    if (Os_info->fp == NULL) {
        ALOGE("Error opening OS image file <%s> for reading: %s",
                    Os_info->fls_path, strerror(errno));
        return STATUS_FILE_NOT_FOUND;
    }
    wResult = fseek(Os_info->fp, 0L, SEEK_END);
    if (wResult) {
        ALOGE("Error seeking end OS image file %s", strerror(errno));
        goto exit;
    }
    Os_info->fls_size = ftell(Os_info->fp);
    if (Os_info->fls_size < 0) {
        ALOGE("Error ftelling file %s", strerror(errno));
        goto exit;
    }
    wResult = fseek(Os_info->fp, 0L, SEEK_SET);
    if (wResult) {
        ALOGE("Error seeking start image file %s", strerror(errno));
        goto exit;
    }
    while(!feof(Os_info->fp))
#endif
    {
        ALOGE("%s; Start of line processing", __func__);

        wIndex=0;
        wLen=0;
        wCount=0;
        memset(pTranscv_Info->sSendData,0x00,JCOP_MAX_BUF_SIZE);
        pTranscv_Info->sSendlength=0;

        ALOGE("%s; wIndex = 0", __func__);
#if(JCDN_WEARABLE_BUFFERED == TRUE)
        for(wCount =0; (wCount < 5 && (Os_info->curOffset < Os_info->curLenBuf)); wCount++, wIndex++)
        {
        	wResult = getHexByteFromBuffer(Os_info->curBuf, Os_info->curLenBuf, &(Os_info->curOffset), &pTranscv_Info->sSendData[wIndex]);
        }
#else
        for(wCount =0; (wCount < 5 && !feof(Os_info->fp)); wCount++, wIndex++)
        {
            wResult = fscanf(Os_info->fp,"%2hhX", &pTranscv_Info->sSendData[wIndex]);
        }
#endif
        if(wResult > 0)
        {
            wLen = pTranscv_Info->sSendData[4];
            ALOGE("%s; Read 5byes success & len=%ld", __func__, wLen);
            if(wLen == 0x00)
            {
                ALOGE("%s: Extended APDU", __func__);
#if(JCDN_WEARABLE_BUFFERED == TRUE)
            	wResult = getHexByteFromBuffer(Os_info->curBuf, Os_info->curLenBuf, &(Os_info->curOffset), &pTranscv_Info->sSendData[wIndex++]);
            	wResult = getHexByteFromBuffer(Os_info->curBuf, Os_info->curLenBuf, &(Os_info->curOffset), &pTranscv_Info->sSendData[wIndex++]);
#else
				wResult = fscanf(Os_info->fp,"%2hhX",&pTranscv_Info->sSendData[wIndex++]);
				wResult = fscanf(Os_info->fp,"%2hhX",&pTranscv_Info->sSendData[wIndex++]);
#endif
                wLen = ((pTranscv_Info->sSendData[5] << 8) | (pTranscv_Info->sSendData[6]));
            }
#if(JCDN_WEARABLE_BUFFERED == TRUE)
            for(wCount =0; (wCount < wLen && (Os_info->curOffset < Os_info->curLenBuf)); wCount++, wIndex++)
            {
            	wResult = getHexByteFromBuffer(Os_info->curBuf, Os_info->curLenBuf, &(Os_info->curOffset), &pTranscv_Info->sSendData[wIndex]);
            }
#else
            for(wCount =0; (wCount < wLen && !feof(Os_info->fp)); wCount++, wIndex++)
            {
                wResult = fscanf(Os_info->fp,"%2hhX",&pTranscv_Info->sSendData[wIndex]);
            }
#endif
        }
        else if(EOF == wResult)
        {
        	ALOGE("%s: End of File or read error", __func__);
        	break;
        }
        else
        {
            ALOGE("%s: JcopOs image Read failed", __func__);
            goto exit;
        }

        pTranscv_Info->sSendlength = wIndex;
        ALOGE("%s: start transceive for length %ld", __func__, pTranscv_Info->sSendlength);
        if((pTranscv_Info->sSendlength != 0x03) &&
           (pTranscv_Info->sSendData[0] != 0x00) &&
           (pTranscv_Info->sSendData[1] != 0x00))
        {

            stat = mchannel->transceive(pTranscv_Info->sSendData,
                                    pTranscv_Info->sSendlength,
                                    pTranscv_Info->sRecvData,
                                    pTranscv_Info->sRecvlength,
                                    &recvBufferActualSize,
                                    pTranscv_Info->timeout);
        }
        else
        {
            ALOGE("%s: Invalid packet", __func__);
            continue;
        }
        if(stat != TRUE)
        {
            ALOGE("%s: Transceive failed; status=0x%X", __func__, stat);
            status = STATUS_FAILED;
            goto exit;
        }
        else if(recvBufferActualSize != 0 &&
                pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x90 &&
                pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)
        {
            //ALOGE("%s: END transceive for length %d", __func__, pTranscv_Info->sSendlength);
            status = STATUS_SUCCESS;
        }
        else if(pTranscv_Info->sRecvData[recvBufferActualSize-2] == 0x6F &&
                pTranscv_Info->sRecvData[recvBufferActualSize-1] == 0x00)
        {
            ALOGE("%s: JcopOs is already upto date-No update required exiting", __func__);
            Os_info->version_info.ver_status = STATUS_UPTO_DATE;
            status = STATUS_FAILED;
            break;
        }
        else
        {
            status = STATUS_FAILED;
            ALOGE("%s: Invalid response", __func__);
            goto exit;
        }
        ALOGE("%s: Going for next line", __func__);
    }

    if(status == STATUS_SUCCESS ||
       Os_info->version_info.ver_status == STATUS_UPTO_DATE) /* We probably have lost the JCOP state info and thus
       tried step1. However, it returned 6F00 indicating this was not the right step.
       Increment the step for the next retry */
    {
        Os_info->cur_state = (Os_info->cur_state + 1 ) % 4;
        Os_info->index = Os_info->cur_state;
        SetJcopOsState(Os_info, Os_info->cur_state);
    }

exit:
    mchannel->doeSE_JcopDownLoadReset();
    ALOGE("%s close fp and exit; status= 0x%X", __func__,status);
#if(JCDN_WEARABLE_BUFFERED != TRUE)
    wResult = fclose(Os_info->fp);
#endif
    return status;
}

/*******************************************************************************
**
** Function:        GetJcopOsState
**
** Description:     Used to update the JCOP OS state
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS GetJcopOsState(JcopOs_ImageInfo_t *Os_info, UINT8 *counter)
{
    tJBL_STATUS status = STATUS_SUCCESS;
    FILE *fp;
    UINT8 xx=0;
    UNUSED(fp)
    ALOGD("%s: enter", __func__);
    if(Os_info == NULL)
    {
        ALOGE("%s: invalid parameter", __func__);
        return STATUS_FAILED;
    }
#if(JCDN_WEARABLE_BUFFERED == TRUE)
    status = Os_info->storeCb.cbGet(&xx);
    if(status != STATUS_SUCCESS)
    {
    	Os_info->storeCb.cbSet(0);
        ALOGE("Error in open/reading JcopOsState, create the file and assume state as 0");
        status = STATUS_SUCCESS;
    }
    else
    {
        ALOGE("JcopOsState just retrieved: %d", xx);
    }
#else
    fp = fopen(JCOP_INFO_PATH, "r");

    if (fp == NULL) {
        ALOGE("file <%s> not exits for reading- creating new file: %s",
                JCOP_INFO_PATH, strerror(errno));
        fp = fopen(JCOP_INFO_PATH, "w+");
        if (fp == NULL)
        {
            ALOGE("Error opening OS image file <%s> for reading: %s",
                    JCOP_INFO_PATH, strerror(errno));
            return STATUS_FAILED;
        }
        fprintf(fp, "%u", xx);
        fclose(fp);
    }
    else
    {
        fscanf(fp, "%hhu", &xx); // read an unsigned BYTE
        ALOGE("JcopOsState %d", xx);
        fclose(fp);
    }
#endif

    switch(xx)
    {
    case JCOP_UPDATE_STATE0:
    case JCOP_UPDATE_STATE3:
        ALOGE("Starting update from step1");
        Os_info->index = xx;
        Os_info->cur_state = xx;
        *counter = 0;
        break;
    case JCOP_UPDATE_STATE1:
        ALOGE("Starting update from step2");
        Os_info->index = JCOP_UPDATE_STATE1;
        Os_info->cur_state = JCOP_UPDATE_STATE1;
        *counter = 3;
        break;
    case JCOP_UPDATE_STATE2:
        ALOGE("Starting update from step3");
        Os_info->index = JCOP_UPDATE_STATE2;
        Os_info->cur_state = JCOP_UPDATE_STATE2;
        *counter = 5;
        break;
    default:
        ALOGE("invalid state");
        status = STATUS_FAILED;
        break;
    }
    return status;
}

/*******************************************************************************
**
** Function:        SetJcopOsState
**
** Description:     Used to set the JCOP OS state
**
** Returns:         Success if ok.
**
*******************************************************************************/
tJBL_STATUS SetJcopOsState(JcopOs_ImageInfo_t *Os_info, UINT8 state)
{
    tJBL_STATUS status = STATUS_FAILED;
#if(JCDN_WEARABLE_BUFFERED == FALSE)
    FILE *fp;
    int fd, ret;
#endif
    ALOGD("%s: enter", __func__);
    if(Os_info == NULL)
    {
        ALOGE("%s: invalid parameter", __func__);
        return status;
    }
#if(JCDN_WEARABLE_BUFFERED == TRUE)
    status = Os_info->storeCb.cbSet(state);
    ALOGE("Current JcopOsState %d", state);
#else
    fp = fopen(JCOP_INFO_PATH, "w");

    if (fp == NULL) {
        ALOGE("Error opening OS image file <%s> for reading: %s",
                JCOP_INFO_PATH, strerror(errno));
    }
    else
    {
        fprintf(fp, "%u", state);
        fflush(fp);
        ALOGE("Current JcopOsState: %d", state);
        status = STATUS_SUCCESS;
        fd=fileno(fp);
        ret = fdatasync(fd);
        ALOGE("ret value: %d", ret);
        fclose(fp);
    }
#endif
    return status;
}

#if 0
void *JcopOsDwnld::GetMemory(UINT32 size)
{
    void *pMem;
    pMem = (void *)malloc(size);

    if(pMem != NULL)
    {
        memset(pMem, 0, size);
    }
    else
    {
        ALOGD("%s: memory allocation failed", __func__);
    }
    return pMem;
}

void JcopOsDwnld::FreeMemory(void *pMem)
{
    if(pMem != NULL)
    {
        free(pMem);
        pMem = NULL;
    }
}
#endif/*JCOP_ENABLE*/
#endif
