/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 */
#include "OverrideLog.h"

#if ANDROID
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#endif

#if(ENABLE_SESSION_MGMT == TRUE)
#include "buildcfg.h"
#include "nfa_mem_co.h"
#include "nfa_nv_co.h"
#include "nfa_nv_ci.h"
#include "SeApi.h"
extern char bcm_nfc_location[];

extern tSeApi_SessionSaveCallbacks *gCallback_SessionSave;

/*******************************************************************************
**
** Function         nfa_nv_co_read
**
** Description      This function is called by NFA to read in data from the
**                  previously opened file.
**
** Parameters       pBuffer   - buffer to read the data into.
**                  nbytes  - number of bytes to read into the buffer.
**
** Returns          void
**
**                  Note: Upon completion of the request, nfa_nv_ci_read() is
**                        called with the buffer of data, along with the number
**                        of bytes read into the buffer, and a status.  The
**                        call-in function should only be called when ALL requested
**                        bytes have been read, the end of file has been detected,
**                        or an error has occurred.
**
*******************************************************************************/
NFC_API extern void nfa_nv_co_read(UINT8 *pBuffer, UINT16 nbytes, UINT8 block)
{
    ALOGD ("%s: bytes to read=%u", __func__, nbytes);

    if(gCallback_SessionSave != NULL && gCallback_SessionSave->readCb != NULL)
    {
    	size_t actualReadData = gCallback_SessionSave->readCb(pBuffer, nbytes);

        if (actualReadData > 0)
        {
            ALOGD ("%s: data read=%u", __func__, actualReadData);
            nfa_nv_ci_read (actualReadData, NFA_NV_CO_OK, block);
        }
        else
        {
            ALOGE ("%s: failed to read", __func__);
            nfa_nv_ci_read (0, NFA_NV_CO_FAIL, block);
        }
    }
    else
    {
        ALOGD ("%s: callback pointer is NULL", __func__);
        nfa_nv_ci_read (0, NFA_NV_CO_FAIL, block);
    }
}

/*******************************************************************************
**
** Function         nfa_nv_co_write
**
** Description      This function is called by io to send file data to the
**                  phone.
**
** Parameters       pBuffer   - buffer to read the data from.
**                  nbytes  - number of bytes to write out to the file.
**
** Returns          void
**
**                  Note: Upon completion of the request, nfa_nv_ci_write() is
**                        called with the file descriptor and the status.  The
**                        call-in function should only be called when ALL requested
**                        bytes have been written, or an error has been detected,
**
*******************************************************************************/
NFC_API extern void nfa_nv_co_write(const UINT8 *pBuffer, UINT16 nbytes, UINT8 block)
{
    ALOGD ("%s: bytes to write=%u", __func__, nbytes);

    if(gCallback_SessionSave != NULL && gCallback_SessionSave->writeCb != NULL)
    {
    	size_t actualWrittenData = gCallback_SessionSave->writeCb(pBuffer, nbytes);
        ALOGD ("%s: %u bytes written", __func__, actualWrittenData);

        if (actualWrittenData == nbytes)
        {
            nfa_nv_ci_write (NFA_NV_CO_OK);
        }
        else
        {
            ALOGE ("%s: fail to write", __func__);
            nfa_nv_ci_write (NFA_NV_CO_FAIL);
        }

    }
    else
    {
        ALOGE ("%s: callback pointer is NULL", __func__);
        nfa_nv_ci_write (NFA_NV_CO_FAIL);
    }
}

#endif
