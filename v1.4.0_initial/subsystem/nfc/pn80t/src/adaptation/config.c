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
#include "config.h"
#include <stdio.h>
#include "phNfcTypes.h"
#include <libnfc-common-conf.h>

#undef LOG_TAG
#define LOG_TAG "NfcAdaptation"

#define config_name             "libnfc-common.conf"
#define extra_config_base       "libnfc-brcm-"
#define extra_config_ext        ".conf"
#define     IsStringValue       0x80000000

/*******************************************************************************
**
** Function:    CNfcConfig::find()
**
** Description: search if a setting exist in the setting array
**
** Returns:     pointer to the setting object
**
*******************************************************************************/
const ConfigParam_t* ParamFind(const unsigned char key)
{
	int i;
	int listSize;

    listSize = (sizeof(MainConfig) / sizeof(ConfigParam_t));

    if (listSize == 0)
    	return NULL;

    for (i = 0; i<listSize; ++i)
    {
        if (MainConfig[i].key == key)
        {
            if(MainConfig[i].type == TYPE_DATA)
            {
                ALOGD("%s found key %d, data len = %d\n", __func__, key,  *((unsigned char *)(MainConfig[i].val)));
            }
            else
            {
                ALOGD("%s found key %d = (0x%lx)\n", __func__, key, (long unsigned int) MainConfig[i].val);
            }
            return &(MainConfig[i]);
        }
    }
    return NULL;
}



/*******************************************************************************
**
** Function:    GetByteArrayValue()
**
** Description: Read byte array value from the config file.
**
** Parameters:
**              name    - name of the config param to read.
**              pValue  - pointer to input buffer.
**              len 	- input buffer length.
**              readlen - out parameter to return the number of bytes read from config file,
**                        return -1 in case bufflen is not enough.
**
** Returns:     TRUE[1] if config param name is found in the config file, else FALSE[0]
**
*******************************************************************************/
extern int GetByteArrayValue(unsigned char key, char* pValue,long len, long *readlen)
{
    if (!pValue)
        return FALSE;

    const ConfigParam_t* pParam = ParamFind(key);

    if (pParam == NULL)
        return FALSE;

    if ((pParam->type == TYPE_DATA) && (pParam->val != 0))
    {
        if( ((unsigned char *)(pParam->val))[0] <= len)
        {
            memset(pValue, 0, len);
            memcpy(pValue, &(((unsigned char *)(pParam->val))[1]), ((unsigned char *)(pParam->val))[0] );
            *readlen = (long) ((unsigned char *)(pParam->val))[0];
        }
        else
        {
            *readlen = -1;
        }

        return TRUE;
    }
    return FALSE;
}

/*******************************************************************************
**
** Function:    GetNumValue
**
** Description: API function for getting a numerical value of a setting
**
** Returns:     true, if successful
**
*******************************************************************************/
extern int GetNumValue(unsigned char key, void* pValue, unsigned long len)
{
    if (!pValue)
        return FALSE;

    const ConfigParam_t* pParam = ParamFind(key);

    if (pParam == NULL)
        return FALSE;

    unsigned long v = (unsigned long) pParam->val;

    switch (len)
    {
    case sizeof(unsigned long):
        *((unsigned long*)(pValue)) = (unsigned long)v;
        break;
    case sizeof(unsigned short):
        *((unsigned short*)(pValue)) = (unsigned short)v;
        break;
    case sizeof(unsigned char):
        *((unsigned char*)(pValue)) = (unsigned char)v;
        break;
    default:
        return FALSE;
    }

    return TRUE;
}
