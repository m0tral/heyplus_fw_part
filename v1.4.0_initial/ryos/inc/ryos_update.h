
#ifndef __RYOS_UPDATE_H__
#define __RYOS_UPDATE_H__


#include "ry_utils.h"
#include "rbp.h"
#include "scheduler.h"

#define USE_INTERNAL_FLASH

#define FIRMWARE_ADDR				0x00008000
#define SYSTEM_STATUS_INFO_ADDR  	0x00006000

#define FLAG_PAGE_LOCATION          0x00006000
#define FIRMWARE_STORE_ADDR         0x00000000

#define FILE_START_MAGIC            0xDEAD

#define NO_FIRMWAVE_FILE            0xff

#define FIRMWAVE_FILE_MAX_SECTOR_NUM        240

#define FIRMWAVE_HEAD_SIZE                  64

#define LOW_POWER_LEVEL                     10


typedef struct{
	u32_t firmware_update_status;
	
}system_status_info_t;

#define UPDATE_RES      0
#define UPDATE_MCU      1
#define UPDATE_BT       5
#define UPDATE_NB_IOT   3
#define UPDATE_GPS      4
#define BOOTLOADER      2


typedef enum{
    UPDATE_DISPLAY_MODE_ALL = 0,
    UPDATE_DISPLAY_MODE_HALF = 1,
}update_display_mode_e;



//system File Head ,64 bytes
typedef struct{
	u32_t file_size;
	u32_t data_offset;
	u32_t type;
	u32_t crc;
	u32_t version;
	u8_t no_used[44];	
}ry_systemFileHead_t;



typedef enum {

	UPDATE_IDLE = 0,
	UPDATE_START,
	UPDATING,
	UPDATE_FINISH

}RY_UPDATE_STATE;


typedef enum{
    RESOURCE_FILE_HEAD = 0,
    RESOURCE_FILE_NAME,
    RESOURCE_FILE_DATE,

}RY_RESOURCE_UPDATE_STATUS;


typedef struct{
	
	u32_t version;
	u32_t length;
}ry_update_info_t;


typedef struct res_file_head{
	unsigned short magic;
	unsigned short name_len;
	unsigned int file_len;
	
}res_file_head;


typedef struct {
    u32_t file_length;
}file_repair_ctx_t;


u32_t get_cur_update_status(void);

ry_sts_t update_start(RbpMsg *msg);
ry_sts_t update_handle(RbpMsg *msg);
void restore_file(u8_t * data, int len);
void get_boot_bin_data(u8_t * data, u32_t len);
void update_boot_file(void);
void update_timeout_handle(void* arg);
void update_stop(void);
void ryos_update_onConnected(void);
ry_sts_t download_resource_file(RbpMsg *msg);
ry_sts_t update_token_handler(uint8_t* data, int len);

u8_t get_data_check_sum(u8_t * data, int size);

int get_file_check_sum(char * file_name);

ry_sts_t file_repair_start(RbpMsg *msg);

ry_sts_t file_repair_handle(RbpMsg *msg);

void resource_file_update_status_clear(void);

void suspend_other_task_for_update_start(void);

void resume_other_task_for_update_end(void);









#endif



