#include "ry_fs.h"
#include "device_management_service.h"
#include "data_management_service.h"
#include "storage_management_service.h"
#include "activity_qrcode.h"
#include "ry_font.h"

//u8_t work[4096];
//FATFS ry_system_fs = {0};


FATFS *ry_system_fs;




u32_t ry_filesystem_init(void)
{
    FRESULT res ;
    u8_t *work;
    u32_t free_size = 0;

    ry_system_fs = (FATFS*)ry_malloc(sizeof(FATFS));
    if (NULL == ry_system_fs) {
        res = FR_NO_FILESYSTEM;
        LOG_DEBUG("[ry_filesystem_init]: No mem.\r\n");
        goto exit;
    }

    
	res = f_mount(ry_system_fs, "", 1);
	//res = FR_NO_FILESYSTEM;

	if (res == FR_NO_FILESYSTEM) {
         
        work = (u8_t *)ry_malloc(4096 *2);
        if (NULL == work) {
            res = FR_NO_FILESYSTEM; // TODO: change to no mem.
            ry_free((u8_t*)ry_system_fs);
            goto exit;
        }
        
		res = f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096 *2);

        ry_free(work);

		if (res == FR_OK) {
			
			res = f_mount(ry_system_fs, "", 1);
		}
		else {
			LOG_DEBUG("file system create failed \n");
            ry_free((u8_t*)ry_system_fs);
			goto exit;
		}
	}
	
	if (res != FR_OK) {
        ry_free((u8_t*)ry_system_fs);
		LOG_DEBUG("file system mount failed \n");
		goto exit;
	}


exit:
    free_size = 0;
    f_getfree("",(DWORD*)&free_size,&ry_system_fs);
    LOG_INFO("rv = %d ,free_size == %d, %d%%\n",get_resource_version(), free_size, free_size *100 /(6*1024/4));

    
    
    
	return (u32_t)res;
	

}

#define RT_FS_LOG   LOG_DEBUG

int ry_fclose(FIL * fp)
{
    FRESULT res = f_close(fp);
    ry_free(fp);
    if (res != FR_OK) {

		return -1;
	} else{
        return 0;
	}

	
}

FIL * ry_fopen(const char * path, const char * mode)
{
    FIL * fp = (FIL *)ry_malloc(sizeof(FIL));

    FRESULT res = (FRESULT)0;

    BYTE fat_mode = 0;

    if (strcmp("r", mode) == 0){
    
        fat_mode = FA_READ ;
        
    } else if (strcmp("rb", mode) == 0){
    
        fat_mode = FA_READ ;
        
    } else{
        RT_FS_LOG("[ry_fopen]: (%s) open mode is wrong \n", path);
        goto error;
    }

    
    res = f_open(fp, path, fat_mode);
    if(res != FR_OK) {
        RT_FS_LOG("[ry_fopen]: (%s) open failed \n", path);
        goto error;
        
    }

    return fp;

error:

    ry_free(fp);
    
    return NULL;
}

u32_t ry_fread( void * buffer, u32_t size, u32_t count, FIL * fp) 
{
    FRESULT res = (FRESULT)0;
    u32_t read_size = 0;

    res = f_read(fp, buffer, size * count, &read_size);
    if(res != FR_OK) {
        RT_FS_LOG("[ry_fread]:  read failed \n");
        goto error;
        
    }

    return read_size;

error :

    return 0;
}

int ry_fseek(FIL *fp, long offset, int fromwhere)
{
    FRESULT res = (FRESULT)0;
    u32_t start_addr = 0;
    
    if(fromwhere == 0){
        start_addr = 0;
        
    } else if(fromwhere == 1){
        start_addr = f_tell(fp);
        
    } else if(fromwhere == 2){
        start_addr = f_size(fp);
        
    } else{
        RT_FS_LOG("[ry_fseek]:  seek from failed \n");
        goto error;
    }
    
    res = f_lseek(fp, offset + start_addr);
    if(res != FR_OK) {
        RT_FS_LOG("[ry_fseek]:  seek failed \n");
        goto error;
        
    }

    

    return 0;
error:
    return -1;
}

u32_t ry_ftell(FIL * fp)
{
    FRESULT res = (FRESULT)0;
    return f_tell(fp);
}


#if FATFS_COMM_DOTEST


u8_t testBuf[4096 * 3];

void test_FATfs(void)
{
	
	u32_t written_bytes;
	
	
	FRESULT res ;

	res = f_mount(&ry_system_fs, "", 1);
	//res = FR_NO_FILESYSTEM;

	if (res == FR_NO_FILESYSTEM) {

		res = f_mkfs("", FM_ANY| FM_SFD, 4096, work, sizeof(work));

		if (res == FR_OK) {
			
			res = f_mount(&ry_system_fs, "", 1);
		}
		else {
			LOG_DEBUG("file system create failed \n");
			goto exit;
		}
	}
	
	if (res != FR_OK) {

		LOG_DEBUG("file system mount failed \n");
		goto exit;
	}

	FIL file;

	res = f_open(&file, "hello.txt", FA_CREATE_NEW | FA_WRITE);

	if (res != FR_OK) {

		LOG_DEBUG("file open failed \n");
		goto exit;
	}

	res = f_write(&file, "Hello, lixueliang ! \n", 15, &written_bytes);

	if (res != FR_OK) {

		LOG_DEBUG("file write failed \n");
		goto exit;
	}


	res = f_close(&file);

	if (res != FR_OK) {

		LOG_DEBUG("file close failed \n");
		goto exit;
	}

	res = f_open(&file, "hello.txt",  FA_READ);

	if (res != FR_OK) {

		LOG_DEBUG("file open failed \n");
		goto exit;
	}

	res = f_read(&file, testBuf, 10, &written_bytes);

	if (res != FR_OK) {

		LOG_DEBUG("file read failed \n");
		goto exit;
	}

	res = f_close(&file);

	if (res != FR_OK) {

		LOG_DEBUG("file close failed \n");
		goto exit;
	}
	

exit:

	return ;
	
}

#endif





























