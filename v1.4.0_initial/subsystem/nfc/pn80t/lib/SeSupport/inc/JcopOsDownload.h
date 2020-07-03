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

#ifndef JCOPOSDOWNLOAD_H_
#define JCOPOSDOWNLOAD_H_

#include "p61_data_types.h"
#include "IChannel.h"
#include "JcDnld.h"
#include <stdio.h>

typedef struct JcopOs_TranscieveInfo
{
    INT32 timeout;
    UINT8 sRecvData[1024];
    UINT8 *sSendData;//[10240];
    INT32 sSendlength;
    int sRecvlength;
}JcopOs_TranscieveInfo_t;
#define JCOP_MAX_BUF_SIZE 10240
typedef struct JcopOs_Version_Info
{
    UINT8 osid;
    UINT8 ver1;
    UINT8 ver0;
    UINT8 OtherValid;
    UINT8 ver_status;
}JcopOs_Version_Info_t;

#if(JCDN_WEARABLE_BUFFERED == TRUE)
typedef struct JcopOs_ImageInfo
{
    const UINT8 *buf[3];		// pointers and lengths of buffers containing the JCOP download image
    UINT32   	lenBuf[3];
    const UINT8 *curBuf;		// pointer and length of the currently active download buffer
    UINT32		curLenBuf;
    UINT32		curOffset;	        // offset into the currently active download buffer
    tJcopOs_StorageCb storeCb;
    int   index;
    UINT8 cur_state;
    JcopOs_Version_Info_t    version_info;
}JcopOs_ImageInfo_t;
#else
typedef struct JcopOs_ImageInfo
{
    FILE *fp;
    int   fls_size;
    char  fls_path[256];
    int   index;
    UINT8 cur_state;
    JcopOs_Version_Info_t    version_info;
}JcopOs_ImageInfo_t;
#endif
typedef struct JcopOs_Dwnld_Context
{
    JcopOs_Version_Info_t    version_info;
    JcopOs_ImageInfo_t       Image_info;
    JcopOs_TranscieveInfo_t  pJcopOs_TransInfo;
    IChannel_t               *channel;
}JcopOs_Dwnld_Context_t,*pJcopOs_Dwnld_Context_t;

#define OSID_OFFSET  9
#define VER1_OFFSET  10
#define VER0_OFFSET  11
#define JCOPOS_HEADER_LEN 5

#define JCOP_UPDATE_STATE0 0
#define JCOP_UPDATE_STATE1 1
#define JCOP_UPDATE_STATE2 2
#define JCOP_UPDATE_STATE3 3
#define JCOP_MAX_RETRY_CNT 3
#define JCOP_INFO_PATH  "/data/nfc/jcop_info.txt"

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
BOOLEAN JOD_initialize (IChannel_t *channel, tJcopOs_StorageCb* cb);

/*******************************************************************************
**
** Function:        finalize
**
** Description:     Release all resources.
**
** Returns:         None
**
*******************************************************************************/
void JOD_finalize ();

tJBL_STATUS JOD_JcopOs_Download(UINT32 lenBuf1, const UINT8 *pBuf1, UINT32 lenBuf2, const UINT8 *pBuf2, UINT32 lenBuf3, const UINT8 *pBuf3);

tJBL_STATUS JOD_TriggerApdu(JcopOs_ImageInfo_t* pVersionInfo, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pTranscv_Info);

tJBL_STATUS JOD_GetInfo(JcopOs_ImageInfo_t* pVersionInfo, tJBL_STATUS status, JcopOs_TranscieveInfo_t* pTranscv_Info);

tJBL_STATUS JOD_load_JcopOS_image(JcopOs_ImageInfo_t *Os_info, tJBL_STATUS status, JcopOs_TranscieveInfo_t *pTranscv_Info);

tJBL_STATUS JOD_JcopOs_update_seq_handler();

#endif
