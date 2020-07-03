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
//#include "port.h"
#include "UserData_Storage.h"
#include <WearableCoreSDK_BuildConfig.h>

//#include "chip.h"

#include "ry_hal_flash.h"
#include "ry_utils.h"

#define USER_DATA_STORAGE_FLASH_ADDR        0x0009E400
#define USER_DATA_STORAGE_FLASH_LEN         




#define BLANK_FLASH 0xFF

#if(WEARABLE_LS_LTSM_CLIENT == TRUE)
static Userdata_t *active_user_data[USERDATA_TYPE_MAX]; // ALIGN(4)
static QN9022_DevName_Data_t devname_data;
static PN66T_PN66T_Key_t rt_key_data;
static Session_Info_t session_info;
static Transit_Settings_Data_t transit_settings_info;
#else
static Userdata_t *active_user_data[USERDATA_TYPE_MAX];
static QN9022_DevName_Data_t devname_data;
static PN66T_PN66T_Key_t rt_key_data;
static Session_Info_t session_info;
static Transit_Settings_Data_t transit_settings_info;
#endif

enum Userdata_type findTypeFromSubtype(enum Userdata_type datatype)
{
    switch(datatype)
    {
        case USERDATA_TYPE_SESSION_DETAILS:
            return USERDATA_TYPE_DEVICE_BOOT_INFO;
        default:
            return datatype; /* Its not a subtype, conversion not required */
    }
}

void initUserdataStorage(void) {
#if 0
    session_info.header.datatype = USERDATA_TYPE_MAX;
    session_info.header.page = 1;
    session_info.header.length = BOOT_INFO_SIZE + SE_SESSION_INFO_SIZE;
    active_user_data[USERDATA_TYPE_DEVICE_BOOT_INFO] = (Userdata_t *)&session_info;
    /* As this is page contains multiple sub-types, a write request for a particular sub-type
     * should include latest data available in the flash for the other sub-types */
    pData = readUserdata(USERDATA_TYPE_DEVICE_BOOT_INFO);
    if(pData != NULL && pData->header.length > 0)
        memcpy(&session_info.firstboot, &pData->payload, pData->header.length);

    devname_data.header.datatype = USERDATA_TYPE_MAX;
    devname_data.header.page = 1;
    devname_data.header.length = QN9022_DEVNAME_DATA_LENGTH;
    active_user_data[USERDATA_TYPE_QN9022_DEVNAME] = (Userdata_t *)&devname_data;

    rt_key_data.header.datatype = USERDATA_TYPE_MAX;
    rt_key_data.header.page = 5;
    rt_key_data.header.length = PN66T_PN66T_KEYS_LENGTH;
    active_user_data[USERDATA_TYPE_PN66T_KEYS] = (Userdata_t *)&rt_key_data;

    transit_settings_info.header.datatype = USERDATA_TYPE_MAX;
    transit_settings_info.header.page = 1;
    transit_settings_info.header.length = TRANSIT_SETTINGS_DATA_LENGTH;
    active_user_data[USERDATA_TYPE_TRANSIT_SETTINGS] = (Userdata_t *)&transit_settings_info;
#endif
}

const Userdata_t* readUserdata(enum Userdata_type datatype)
{
#if 0
    uint32_t pageCounter = 0;
    uint32_t offset = 0;
    uint32_t dstAdd = USERDATA_START_SECTOR * CFG_HW_FLASH_SECT_SZ;
    bool found = false;
    Userdata_t *pData = NULL;
    Userdata_Header_t *pHeader = NULL;

    datatype = findTypeFromSubtype(datatype);

    for (pageCounter = 0;
        pageCounter < (CFG_HW_FLASH_PAGE_PER_SECTOR * (USERDATA_END_SECTOR - USERDATA_START_SECTOR + 1)); )
    {
        pData = (Userdata_t *)dstAdd;
        pHeader = &(pData->header);
        if (pHeader->datatype == BLANK_FLASH)
            break;

        if (pHeader->datatype == datatype) {
            offset = dstAdd;
            found = true;
        }

        dstAdd += (uint32_t)CFG_HW_FLASH_PAGE_SZ * pHeader->page;
        pageCounter += pHeader->page;
    }

    if (found == false)
    {
        offset = 0;
    }

    LOG_DEBUG("[readUserdata] datatype:%d return pointer:0x%x\n", datatype, offset);
    return (Userdata_t*)offset;
#endif

#if 1
    return 0;
    
#endif


}

uint8_t writeUserdata_internal(const Userdata_t* userdata)
{
#if 0
    uint8_t rc = 0xFF; /* An error code not defined in iap.h *//* Fix for CID 10885 */
    uint32_t uitype = userdata->header.datatype;
    uint32_t pageCounter=0;
    uint32_t i;
    bool erase = false;
    uint32_t dstAdd = USERDATA_START_SECTOR * CFG_HW_FLASH_SECT_SZ;
    uint8_t page_write_iap[4] = {16, 4, 2, 1}; /* IAP method supports 256|512|1024|4096 bytes written */
    uint8_t page_write_sel = 0, page_written = 0;
    uint32_t offset = 0;
    uint32_t primask;
    const Userdata_t *backup = NULL;
    Userdata_Header_t *header = NULL;

    for (pageCounter = 0;
        pageCounter < (CFG_HW_FLASH_PAGE_PER_SECTOR * (USERDATA_END_SECTOR - USERDATA_START_SECTOR + 1)); )
    {
        header = (Userdata_Header_t *)dstAdd;
        if (header->datatype == BLANK_FLASH)
            break;

        dstAdd += (uint32_t)(CFG_HW_FLASH_PAGE_SZ * header->page);
        pageCounter += header->page;

    }

    // if nothing saced so far, or if we used up all area, erase all area
    if (pageCounter + userdata->header.page >
        (CFG_HW_FLASH_PAGE_PER_SECTOR * (USERDATA_END_SECTOR - USERDATA_START_SECTOR + 1)))
    {
        /* Backup the latest values to active_user_data before performing sector erase */
        for (i = 0; i < USERDATA_TYPE_MAX; i++) {
            if (active_user_data[i]->header.datatype == USERDATA_TYPE_MAX) {
                backup = readUserdata((enum Userdata_type)i);
                if (backup) {
                    memcpy(active_user_data[i], backup, backup->header.length + sizeof(Userdata_Header_t));  //PRQA S 3200
                }
            }
        }

        /* Erase all sectors occupied by UDT data */
        INT_DISABLE(primask, 0);
        rc = Chip_IAP_PreSectorForReadWrite(USERDATA_START_SECTOR, USERDATA_END_SECTOR);
        if (rc == LPC_OK) {
            rc = Chip_IAP_EraseSector(USERDATA_START_SECTOR, USERDATA_END_SECTOR);
        }
        INT_RESTORE(primask, 0);
        if (rc != LPC_OK) {
            return rc;
        }
        erase = true;
        dstAdd = USERDATA_START_SECTOR * CFG_HW_FLASH_SECT_SZ;
    }

    for (i = 0; i < USERDATA_TYPE_MAX; i++) {
        if((erase == true || i == uitype) && active_user_data[i]->header.datatype < USERDATA_TYPE_MAX) {
            INT_DISABLE(primask, 0);
            page_written = 0;
            while (page_written < active_user_data[i]->header.page) {
                page_write_sel = 0;
                offset = (uint32_t)(((uint32_t)active_user_data[i]) + (uint32_t)(page_written * CFG_HW_FLASH_PAGE_SZ));
                while (page_write_sel < 4 &&
                    page_write_iap[page_write_sel] > active_user_data[i]->header.page - page_written) {
                    page_write_sel++;
                }
                rc = Chip_IAP_PreSectorForReadWrite((dstAdd / CFG_HW_FLASH_SECT_SZ),(dstAdd / CFG_HW_FLASH_SECT_SZ));
                if (rc == LPC_OK) {
                    rc = Chip_IAP_CopyRamToFlash(dstAdd, (uint32_t *)offset,
                            (uint32_t)CFG_HW_FLASH_PAGE_SZ * page_write_iap[page_write_sel]);
                }
                page_written += page_write_iap[page_write_sel];
                dstAdd += (uint32_t)(CFG_HW_FLASH_PAGE_SZ * page_write_iap[page_write_sel]);
            }
            INT_RESTORE(primask, 0);
        }
    }
    return rc;
#endif 


#if 0
    ry_hal_flash_write(u32_t addr,u8_t * buf,int len);
#endif


    return 0;
}

int32_t writeUserdata(enum Userdata_type datatype, const uint8_t *data, uint32_t datasize)
{
    Userdata_t *userdata = NULL;
    int32_t ret = USERDATA_ERROR_NOERROR;

    LOG_DEBUG("[writeUserdata] datatype:%d, len:%d\n", datatype, datasize);
#if 0
    switch (datatype)
    {
        case USERDATA_TYPE_DEVICE_BOOT_INFO:
            if (datasize == BOOT_INFO_SIZE) /* first boot set*/ {
                session_info.firstboot = *data;
            } else if (datasize == SE_SESSION_INFO_SIZE){
                memcpy(session_info.sesessioninfo, data, datasize);   //PRQA S 3200
            } else {
                ret = USERDATA_ERROR_INVAILID_DATA_SIZE;
            }
        break;

        case USERDATA_TYPE_QN9022_DEVNAME:
            userdata = (Userdata_t *)active_user_data[datatype];
            memcpy(userdata->payload, data, datasize);                //PRQA S 3200
        	break;

        case USERDATA_TYPE_PN66T_KEYS:
            userdata = (Userdata_t *)active_user_data[datatype];
            if (datasize != userdata->header.length) {
                ret = USERDATA_ERROR_INVAILID_DATA_SIZE;
                break;
            }
            memcpy(userdata->payload, data, datasize);                //PRQA S 3200
        break;

        case USERDATA_TYPE_TRANSIT_SETTINGS:
            userdata = (Userdata_t *) active_user_data[datatype];
            userdata->header.length = (uint16_t)datasize;
            if (datasize > TRANSIT_SETTINGS_DATA_LENGTH) {
                ret = USERDATA_ERROR_INVAILID_DATA_SIZE;
                break;
            }
            memcpy(userdata->payload, data, datasize);
            break;

        default:
            ret = USERDATA_ERROR_INVAILID_TYPE;
    }

    datatype = findTypeFromSubtype(datatype);

    if(ret == USERDATA_ERROR_NOERROR)
    {
        active_user_data[datatype]->header.datatype = datatype;
        ret = (int32_t)writeUserdata_internal((const Userdata_t *)active_user_data[datatype]);
    }
#endif
    return ret;
}

