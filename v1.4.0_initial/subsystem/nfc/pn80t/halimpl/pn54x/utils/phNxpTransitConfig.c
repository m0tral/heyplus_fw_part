
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

/******************************************************************************
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
 ******************************************************************************/

#include <stdio.h>
#include "phNxpTransitConfig.h"
#include <phNfcTypes.h>
#include "phOsalNfc.h"
#ifndef COMPANION_DEVICE
#include <phNxpLog.h>
#else
#include "companion_log.h"
#endif

/* Customized settings for new city can be added as follows:
 * Only settings which are different from  default should be added
 * Do not copy entire configuration blob array e.g. cfgData_Nxp_Rf_Conf_Blk_1-6
 * only one array per city is provided*/

 /*Procedure for adding new city
  * 1. Append new city type in phNxpNciCityType_t
  * 2. Define corresponding settings
  * 3. Add it to NXP_Transit_Config
  * */

const uint8_t cfgData_Nxp_Rf_Conf_City1[] = {0x00 /* total length, nci set config command, refer cfgData_Nxp_Rf_Conf_Blk_1-6*/};
const uint8_t cfgData_Nxp_Rf_Conf_City2[] = {0x00};
const uint8_t cfgData_Nxp_Rf_Conf_City3[] = {0x00};

const NxpParam_t NXP_Transit_Config[] =
{
    /////////////////////////////////////////////////////////////////////////////
    //# NXP RF configuration city specific ALM/PLM settings
    //# This section needs to be updated with the correct values based on the platform
    {CFG_NXP_RF_CONF_BLK_CITY1,   TYPE_DATA, cfgData_Nxp_Rf_Conf_City1},
    {CFG_NXP_RF_CONF_BLK_CITY2,   TYPE_DATA, cfgData_Nxp_Rf_Conf_City2},
    {CFG_NXP_RF_CONF_BLK_CITY3,   TYPE_DATA, cfgData_Nxp_Rf_Conf_City3},
};


static const NxpParam_t* NxpTransitParamFind(const unsigned char key)
{
    int i;
    int listSize;

    listSize = (sizeof(NXP_Transit_Config) / sizeof(NxpParam_t));

    if (listSize == 0)
        return NULL;

    for (i = 0; i<listSize; ++i)
    {
        if (NXP_Transit_Config[i].key == key)
        {
            if(NXP_Transit_Config[i].type == TYPE_DATA)
            {
                NXPLOG_EXTNS_D("%s found key %d, data len = %d\n", __func__, key,  *((unsigned char *)(NXP_Transit_Config[i].val)));
            }
            else
            {
                NXPLOG_EXTNS_D("%s found key %d = (0x%lx)\n", __func__, key, (long unsigned int) NXP_Transit_Config[i].val);
            }
            return &(NXP_Transit_Config[i]);
        }
    }
    return NULL;
}
/*******************************************************************************
**
** Function:    GetNxpTransitByteArrayValue()
**
** Description: Read byte array value from the config file.
**
** Parameters:
**              name    - name of the config param to read.
**              pValue  - pointer to input buffer.
**              len     - input buffer length.
**              readlen - out parameter to return the number of bytes read from config file,
**                        return -1 in case bufflen is not enough.
**
** Returns:     TRUE[1] if config param name is found in the config file, else FALSE[0]
**
*******************************************************************************/
int GetNxpTransitByteArrayValue(unsigned char key, char* pValue,long len, long *readlen)
{
    if (!pValue)
        return FALSE;

    const NxpParam_t* pParam = NxpTransitParamFind(key);

    if (pParam == NULL)
        return FALSE;

    if ((pParam->type == TYPE_DATA) && (pParam->val != 0))
    {
        if( ((unsigned char *)(pParam->val))[0] <= len)
        {
            phOsalNfc_SetMemory(pValue, 0, len);
            phOsalNfc_MemCopy(pValue, &(((unsigned char *)(pParam->val))[1]), ((unsigned char *)(pParam->val))[0] );
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
