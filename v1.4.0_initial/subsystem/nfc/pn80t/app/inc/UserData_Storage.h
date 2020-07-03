/*
 * @brief User data storage Protocol and APIs
 *
 * Copyright(C) NXP Semiconductors, 2016
 * All rights reserved.
 *
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef USERDATA_STORAGE_H_
#define USERDATA_STORAGE_H_

#include "ry_type.h"

#define packed_struct  struct
#define packed_union  union
#pragma pack(push)
#pragma pack(1)

/** \addtogroup userdata_storage
 * @{ */

#define CFG_HW_FLASH_SECT_SZ            0x8000        /*!< Size of a flash sector */
#define CFG_HW_FLASH_PAGE_SZ            0x100        /*!< Size of the flash page. */
#define CFG_HW_FLASH_END                0x80000        /*!< Total size of the flash. */

#define CFG_HW_FLASH_PAGE_PER_SECTOR (CFG_HW_FLASH_SECT_SZ/CFG_HW_FLASH_PAGE_SZ)
#define USERDATA_START_SECTOR 15
#define USERDATA_END_SECTOR 15

#define USERDATA_ERROR_NOERROR (0)
#define    USERDATA_ERROR_INVAILID_TYPE (-1)
#define USERDATA_ERROR_INVAILID_DATA_SIZE (-2)


enum Userdata_type
{
    /* The USERDATA_TYPE_DEVICE_BOOT_INFO and USERDATA_TYPE_SESSION_DETALS
       are designed to save in the 1st page of USER DATA sector considering they are unlikely to be changed */

    /* TRUE or FALSE ,
    TRUE indicates is booting first time.
    FALSE indicates .this is not first boot.
    */
    USERDATA_TYPE_DEVICE_BOOT_INFO,

    /* The following fields are likely to be changed */
    /*
    User can change the bluetooth device name
    */
    USERDATA_TYPE_QN9022_DEVNAME,
    /*
    Change with root entity change. May vary from customer to customer.
    To be customizable    during customer production.
    */
    USERDATA_TYPE_PN66T_KEYS,

    /*Transit information Based on Cities*/
    USERDATA_TYPE_TRANSIT_SETTINGS,
    /* Internal implementation requires below to exclude the data subtypes
     * However, user can send any value till USERDATA_SUBTYPE_MAX
     */
    USERDATA_TYPE_MAX,

    /* -- Below are the Data Sub-types packed with one of the above types -- */

    /* Contains HCI session and Device Boot session info.
    This gets changed when NFCC FW is downloaded.
    As HCI session size is unlikely to change, same page as of above is used.
    */
    USERDATA_TYPE_SESSION_DETAILS,

    USERDATA_SUBTYPE_MAX,
};

typedef struct Userdata_Header_s {
    uint8_t datatype;        /* Userdata_type */
    uint8_t page;            /* How many pages it takes */
    uint16_t length;        /* length of payload */
} Userdata_Header_t;

typedef struct Userdata_s {
    Userdata_Header_t header;
    uint8_t payload[1];
} Userdata_t;

#define BOOT_INFO_SIZE   1
#define SE_SESSION_INFO_SIZE 228
typedef packed_struct Session_Info_s{
    Userdata_Header_t header;
    uint8_t firstboot;
    uint8_t sesessioninfo[SE_SESSION_INFO_SIZE];
}Session_Info_t;

#define QN9022_DEVNAME_DATA_LENGTH 33 // 1 length + 32 characters
typedef packed_struct QN9022_DevName_Data_s {
    Userdata_Header_t header;
    uint8_t payload[QN9022_DEVNAME_DATA_LENGTH];
}QN9022_DevName_Data_t;

#define PN66T_PN66T_KEYS_LENGTH 1263
typedef packed_struct PN66T_PN66T_Key_s {
    Userdata_Header_t header;
    uint8_t payload[PN66T_PN66T_KEYS_LENGTH];
}PN66T_PN66T_Key_t;

#define PN66T_MOP_REGISTRY_DATA_LENGTH 900    /* 60 bytes per card */
typedef packed_struct PN66T_MOP_Registry_Data_s {
    Userdata_Header_t header;
    uint8_t payload[PN66T_MOP_REGISTRY_DATA_LENGTH];
}PN66T_MOP_Registry_Data_t;

#define TRANSIT_SETTINGS_DATA_LENGTH 8    /* 1 byte per setting */
typedef packed_struct Transit_Settings_Data_s {
    Userdata_Header_t header;
    uint8_t payload[TRANSIT_SETTINGS_DATA_LENGTH];
}Transit_Settings_Data_t;

/**
 * \brief Initialization of User Data Storage
 *
 *        This API should be called normally after a power reset to initialize the variables used by User Data Storage API's.
 */
void initUserdataStorage(void);

/**
 * \brief Write Data provided by User to the Storage
 *
 *        Data is written to user storage.
 *
 * \pre Data Storage must be initialized
 *
 * \param datatype - [in] enum specifying the type of data to be written
 * \param data     - [in] buffer containing the data to be written
 * \param datasize - [in] size of the data
 *
 * \return  USERDATA_ERROR_NOERROR on success
 * \return  USERDATA_ERROR_INVAILID_TYPE if data type is incorrect
 * \return  USERDATA_ERROR_INVAILID_DATA_SIZE if data size is incorrect
 */
int32_t writeUserdata(enum Userdata_type datatype, const uint8_t *data, uint32_t datasize);

/**
 * \brief Read from storage, data of the type requested by the user
 *
 *        Data is read from user storage.
 *
 * \pre Data Storage must be initialized
 *
 * \param datatype - [in] enum specifying the type of data to be read
 *
 * \return  pointer to Data on success
 * \return  NULL if data type is incorrect
 */
const Userdata_t* readUserdata(enum Userdata_type datatype);

/** @}  */

#pragma pack(pop)

#endif /* USERDATA_STORAGE_H_ */
