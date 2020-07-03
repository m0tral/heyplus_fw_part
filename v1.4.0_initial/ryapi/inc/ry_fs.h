#ifndef __RY_FS_H__

#define __RY_FS_H__


#include "ry_type.h"
#include "ff.h"
#include "ry_hal_spi_flash.h"
#include "ry_utils.h"

#define FS_USED_SECTOR_NUM          0x600//6M  //768 (3M)
#define FS_START_SECTOR             256


u32_t ry_filesystem_init(void);



int ry_fclose(FIL * fp);
FIL * ry_fopen(const char * path, const char * mode);
u32_t ry_fread( void * buffer, u32_t size, u32_t count, FIL * fp);
int ry_fseek(FIL *fp, long offset, int fromwhere);
u32_t ry_ftell(FIL * fp);















#endif

