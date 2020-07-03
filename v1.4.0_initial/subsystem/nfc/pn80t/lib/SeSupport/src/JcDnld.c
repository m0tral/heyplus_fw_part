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
#include "JcDnld.h"
#include "JcopOsDownload.h"
#include <p61_data_types.h>
#if(JCOP_ENABLE == TRUE)
#ifndef COMPANION_DEVICE
#include <OverrideLog.h>
#endif

IChannel_t *channel;
static BOOLEAN inUse = FALSE;
extern pJcopOs_Dwnld_Context_t gpJcopOs_Dwnld_Context;
/*******************************************************************************
**
** Function:        JCDNLD_Init
**
** Description:     Initializes the JCOP library and opens the DWP communication channel
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
unsigned char JCDNLD_Init(IChannel_t *channel, tJcopOs_StorageCb* cb)
{
    BOOLEAN stat = FALSE;
    ALOGD("%s: enter", __func__);

    if (inUse == TRUE)
    {
        return STATUS_INUSE;
    }
    else if((channel == NULL) || (cb == NULL))
    {
        return STATUS_FAILED;
    }
    /* inUse assignment can be with protection like using semaphore*/
    inUse = TRUE;
    stat = JOD_initialize (channel, cb);
    if(stat != TRUE)
    {
        ALOGE("%s: failed", __func__);
    }
    else
    {
        if((channel != NULL) &&
           (channel->open) != NULL)
        {
            stat = channel->open();
            if(stat != TRUE)
            {
                ALOGE("%s:Open DWP communication is failed", __func__);
            }
            else
            {
                ALOGE("%s:Open DWP communication is success", __func__);
            }
        }
        else
        {
            ALOGE("%s: NULL DWP channel", __func__);
            stat = FALSE;
        }
    }
    return (stat == TRUE)?STATUS_OK:STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        JCDNLD_StartDownload
**
** Description:     Starts the JCOP update
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
unsigned char JCDNLD_StartDownload(UINT32 lenBuf1, const UINT8 *pBuf1,
		                           UINT32 lenBuf2, const UINT8 *pBuf2,
		                           UINT32 lenBuf3, const UINT8 *pBuf3)
{
    tJBL_STATUS status = STATUS_FAILED;
    status = JOD_JcopOs_Download(lenBuf1, pBuf1, lenBuf2, pBuf2, lenBuf3, pBuf3);
    ALOGE("%s: Exit; status=0x0%X", __func__, status);
    return status;
}

/*******************************************************************************
**
** Function:        JCDNLD_DeInit
**
** Description:     Deinitializes the JCOP Lib
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
BOOLEAN JCDNLD_DeInit()
{
    BOOLEAN stat = FALSE;
    channel = gpJcopOs_Dwnld_Context->channel;
    ALOGD("%s: enter", __func__);
    if(channel != NULL)
    {
        if(channel->doeSE_JcopDownLoadReset != NULL)
        {
            channel->doeSE_JcopDownLoadReset();
            if(channel->close != NULL)
            {
                stat = channel->close();
                if(stat != TRUE)
                {
                    ALOGE("%s:closing DWP channel is failed", __func__);
                }
            }
            else
            {
                ALOGE("%s: NULL fp DWP_close", __func__);
                stat = FALSE;
            }
        }
    }
    else
    {
        ALOGE("%s: NULL dwp channel", __func__);
    }
    JOD_finalize();
    /* inUse assignment can be with protection like using semaphore*/
    inUse = FALSE;
    return stat;
}

/*******************************************************************************
**
** Function:        JCDNLD_CheckVersion
**
** Description:     Check the existing JCOP OS version
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
BOOLEAN JCDNLD_CheckVersion()
{

    /**
     * Need to implement in case required
     * */
    return TRUE;
}

#endif/*JCOP_ENABLE*/
