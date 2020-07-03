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
#include <string.h>
#include <stdbool.h>

#include "qn_isp_api.h"
#include "application.h"
#include <phOsalNfc.h>
#include <phOsalNfc_Log.h>


int read_app(uint32_t offset, size_t len, uint8_t *pdata)
{    
    // read application data. It just read from a memory which will loop with 0x1000101C for ever
    memcpy(pdata, &application[offset], len);
    return len;
}

qerrno_t isp(int dev_index, device_type_t device_type, crystal_type_t device_crystal)
{
  qdevice_t     *device = NULL;
  bool           res = false;
  nvds_tag_len_t len = NVDS_LEN_BD_ADDRESS;                       // if the NVDS of device valid, operate it
  uint8_t        buf[NVDS_LEN_BD_ADDRESS];

    // open "DEVICE_0" device
    phOsalNfc_LogCrit("[%d] open device...\n", dev_index);
    device = qn_api_open(dev_index, device_type, device_crystal, 115200 * 4, 10000);
			
    if (device != NULL)
    {
        device->app_protect = false;                              // disable application protect function

        // Download application
        phOsalNfc_LogCrit("[%d] download application...\n", dev_index);
        if (!qn_api_download_app(device, sizeof(application), read_app))
        {
            phOsalNfc_LogBle("Downloading Fail\n");                    // Download application failed
        }
        else
        {
        	phOsalNfc_LogCrit("Finished Downloading \n");

            res = qn_api_nvds_get(device, NVDS_TAG_BD_ADDRESS, &len, buf);      // get Bluetooth address

            phOsalNfc_Log("BD address:%x:%x:%x:%x:%x:%x\n",buf[5],buf[4],buf[3],buf[2],buf[1],buf[0]);

            if (!res)
                qn_api_errno(dev_index);                 // getting address fail, get the error code

            buf[0]++;
            len = NVDS_LEN_BD_ADDRESS;
            if (qn_api_nvds_put(device, NVDS_TAG_BD_ADDRESS, len, buf))                // Set a new bluetooth address
            {
            	phOsalNfc_LogCrit("[%d] download nvds...\n", dev_index);                    // Download NVDS
                if (qn_api_download_nvds(device))
                {
                	phOsalNfc_LogCrit("[%d] close device...\n", dev_index);                        // Close device
                    qn_api_close(device);
                }
            }
        }
    }
    return qn_api_errno(dev_index);
}

void QN_isp_download()
{
    isp(0, ISP_DEVICE_QN9020, ISP_CRYSTAL_16MHZ);         // launch ISP download thread

    phOsalNfc_Delay(10);
}
