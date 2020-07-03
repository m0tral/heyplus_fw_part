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
#include <p61_data_types.h>
#ifndef COMPANION_DEVICE
#include <OverrideLog.h>
#include <cutils/log.h>
#endif
#include "string.h"

extern pAla_Dwnld_Context_t gpAla_Dwnld_Context;
/*static BOOLEAN Ala_inUse = FALSE;*/
/*******************************************************************************
**
** Function:        JCDNLD_Init
**
** Description:     Initializes the JCOP library and opens the DWP communication channel
**
** Returns:         STATUS_OK if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_Init(IChannel_t *channel)
{
    static const char fn[] = "ALA_Init";
    UNUSED(fn);
    BOOLEAN stat = FALSE;
    ALOGD("%s: enter", fn);

/*    if (Ala_inUse == true)
    {
        return STATUS_INUSE;
    }*/
    if(channel == NULL)
    {
        return STATUS_FAILED;
    }
    stat = initialize (channel);
    if(stat != TRUE)
    {
        ALOGE("%s: failed", fn);
    }
    else
    {
        channel = gpAla_Dwnld_Context->mchannel;
        if((channel != NULL) &&
           (channel->open) != NULL)
        {
            stat = channel->open();
            if(stat != TRUE)
            {
                ALOGE("%s:Open DWP communication is failed", fn);
            }
            else
            {
                ALOGE("%s:Open DWP communication is success", fn);
            }
        }
        else
        {
            ALOGE("%s: NULL DWP channel", fn);
            stat = FALSE;
        }
    }
    return (stat == TRUE)?STATUS_OK:STATUS_FAILED;
}

/*******************************************************************************
**
** Function:        ALA_Start
**
** Description:     Starts the ALA update over DWP
**
** Parameters:		name   - IN - path to the input script
** 					dest   - IN - path to the response output, stores script execution
** 							      commands and responses (data + SW)
** 					pdata  - IN - calculated SHA-1 of the application package name
** 					len    - IN - length of the SHA-1 data passed in pdata
** 					respSW - OUT - TLV of response from last executed script command
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
tJBL_STATUS ALA_Start(INT32 nameLen, const char *name, UINT16 *destLen, const char *dest, UINT8 *pdata, UINT16 len, UINT8 *respSW)
#else
tJBL_STATUS ALA_Start(const char *name, const char *dest, UINT8 *pdata, UINT16 len, UINT8 *respSW)
#endif
#else
tJBL_STATUS ALA_Start(const char *name, UINT8 *pdata, UINT16 len)
#endif
{
    static const char fn[] = "ALA_Start";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    if(name != NULL)
    {
#if(NXP_LDR_SVC_VER_2 == TRUE)
#if(ALA_WEARABLE_BUFFERED == TRUE)
        status = Perform_ALA(nameLen, name, destLen, dest, pdata, len, respSW);
#else
        ALOGE("%s: Dest is %s", fn, dest);
        status = Perform_ALA(name, dest, pdata, len, respSW);
#endif
#else
        status = Perform_ALA(name, pdata, len);
#endif
    }
    else
    {
        ALOGE("Invalid parameter");
    }
    ALOGE("%s: Exit; status=0x0%X", fn, status);
    return status;
}

/*******************************************************************************
**
** Function:        JCDNLD_DeInit
**
** Description:     Deinitializes the ALA module
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
BOOLEAN ALA_DeInit()
{
    static const char fn[] = "ALA_DeInit";
    UNUSED(fn);
    BOOLEAN stat = FALSE;
    IChannel_t* channel = gpAla_Dwnld_Context->mchannel;
    ALOGD("%s: enter", fn);
    if(channel != NULL)
    {
        if(channel->doeSE_Reset != NULL)
        {
            //channel->doeSE_Reset();
            if(channel->close != NULL)
            {
                stat = channel->close();
                if(stat != TRUE)
                {
                    ALOGE("%s:closing DWP channel is failed", fn);
                }
            }
            else
            {
                ALOGE("%s: NULL fp DWP_close", fn);
                stat = FALSE;
            }
        }
    }
    else
    {
        ALOGE("%s: NULL dwp channel", fn);
    }
    finalize();

    return stat;
}
#if(NXP_LDR_SVC_VER_2 != TRUE)
/*******************************************************************************
**
** Function:        ALA_GetlistofApplets
**
** Description:     Gets the list of applets present the pre-defined directory
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
void ALA_GetlistofApplets(char *list[], UINT8* num)
{
  static const char dir[] = "/data/ala/";
  struct dirent *dp;
  UINT8 xx =0;
  DIR *fd;

  if ((fd = opendir(dir)) == NULL)
  {
    fprintf(stderr, "listdir: can't open %s\n", dir);
    return;
  }
  while ((dp = readdir(fd)) != NULL)
  {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
       continue;    /* skip self and parent */

      ALOGE("%s/%s\n", dir, dp->d_name);
      list[xx] = (char *)malloc(strlen(dp->d_name)+1);
      if(list[xx] != NULL)
      {
          memset((void *)list[xx],0, strlen(dp->d_name)+1);
          memcpy(list[xx++], dp->d_name, strlen(dp->d_name)+1);
      }
      else
      {
          ALOGE("Memory allocation failed");
      }

  }
  *num = xx;
  ALOGD("%s: number of applets found=0x0%x", __FUNCTION__, *num);
  closedir(fd);
}

/*******************************************************************************
**
** Function:        ALA_GetCertificateKey
**
** Description:     Get the JSBL reference key
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_GetCertificateKey(UINT8 *pKey, INT32 *pKeylen)
{
    static const char fn[] = "ALA_GetCertificateKey";
    tJBL_STATUS status = STATUS_FAILED;
    IChannel_t *channel = gpAla_Dwnld_Context->mchannel;
    if(pKey != NULL)
    {
        status = GetJsbl_Certificate_Refkey(pKey, pKeylen);
    }
    else
    {
        ALOGE("Invalid parameter");
    }
    ALOGE("%s: Exit; status=0x0%X", fn, status);
    return status;
}
#else

/*******************************************************************************
**
** Function:        ALA_lsGetVersion
**
** Description:     Get the version of Loder service client and applet
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_lsGetVersion(UINT8 *pVersion)
{
    static const char fn[] = "ALA_lsGetVersion";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    if(pVersion!= NULL)
    {
        status = GetLs_Version(pVersion);
        ALOGE("%s: LS Lib lsGetVersion status =0x0%X%X", fn, *pVersion, *(pVersion+1));
    }
    else
    {
        ALOGE("Invalid parameter");
    }
    ALOGE("%s: Exit; status=0x0%X", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_lsGetStatus
**
** Description:     Get the version of Loder service client and applet
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_lsGetStatus(UINT8 *pVersion)
{
    static const char fn[] = "ALA_lsGetStatus";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    if(pVersion!= NULL)
    {
        status = Get_LsStatus(pVersion);
        ALOGE("%s: lsGetStatus ALALIB status=0x0%X 0x0%X", fn, pVersion[0], pVersion[1]);
    }
    else
    {
        ALOGE("Invalid parameter");
    }
    ALOGE("%s: Exit; status=0x0%X", fn, status);
    return status;
}
/*******************************************************************************
**
** Function:        ALA_lsGetAppletStatus
**
** Description:     Get the version of Loder service client and applet
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
tJBL_STATUS ALA_lsGetAppletStatus(UINT8 *pVersion)
{
    static const char fn[] = "ALA_lsGetStatus";
    UNUSED(fn);
    tJBL_STATUS status = STATUS_FAILED;
    if(pVersion!= NULL)
    {
        status = Get_LsAppletStatus(pVersion);
        ALOGE("%s: lsGetStatus ALALIB status=0x0%X 0x0%X", fn, pVersion[0], pVersion[1]);
    }
    else
    {
        ALOGE("Invalid parameter");
    }
    ALOGE("%s: Exit; status=0x0%X", fn, status);
    return status;
}

#endif
#endif/*LS_ENABLE*/
