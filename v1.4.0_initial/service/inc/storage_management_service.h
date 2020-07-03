/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/



#ifndef __STORAGE_MANAGEMENT_SERVICE_H__

#define __STORAGE_MANAGEMENT_SERVICE_H__
/*********************************************************************
 * INCLUDES
 */

#include "ryos.h"
#include "ry_utils.h"
#include "Property.pb.h" 
#include "dip.h"





/*
 * CONSTANTS
 *******************************************************************************
 */


#define MAX_FILE_NAME_LEN                       30

#define EXFLASH_FS_HEAD_COPY_ADDR               (240*EXFLASH_SECTOR_SIZE)
#define EXFLASH_FS_HEAD_START_ADDR              (256*EXFLASH_SECTOR_SIZE)
#define EXFLASH_FS_HEAD_SECTOR_NUM              6

#define FACTORY_FS_HEAD_ADDR                    (250 * EXFLASH_SECTOR_SIZE)

#define FS_HEAD_CHECK_SUM_PAGE_ADDR             (248 * EXFLASH_SECTOR_SIZE)



/*
 * TYPES
 *******************************************************************************
 */


typedef enum{
    FS_HEAD_CUR,
    FS_HEAD_BACKUP,
}fs_head_type_e;

typedef struct{
    char *  file_name;
    u8_t    checkSum;
}checkFile_cell_t;



/*
 * FUNCTIONS
 *******************************************************************************
 */
 

void storage_check_all_files_recover(void);

int storage_get_check_file_numbers(void);

int storage_get_lost_file_numbers(void);

int storage_get_lost_file_status(void);

void storage_send_lost_file_list(void);

void recover_fs_head(void);

void recover_fs_head_to_factroy(void);

void check_fs_header(u8_t);

void save_fs_header(void);

void file_error_handle(const char * file_name, int error_code);

void storage_check_fs_head_status(void);

void storage_management_init(void);



#endif



