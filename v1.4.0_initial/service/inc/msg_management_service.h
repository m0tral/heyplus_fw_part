#ifndef __APP_MM_H__
#define __APP_MM_H__

#include "ryos.h"
#include "ff.h"

#define MSG_BUF_NUM      11
#define MAX_MSG_NUM      10
#define TELE_MSG_NUM     MAX_MSG_NUM

#define STORE_IN_INFLASH    1
#define STORE_IN_EXFLASH    1


#if STORE_IN_EXFLASH

#define MSG_MANAGER_FILE_NAME        "msg_manager"

#define MSG_DATA_FILE_NAME          "msg_data"

#endif



typedef struct _message_info{
    u16_t type;
    u16_t title_len;
    u16_t data_len;
    u32_t time;

    u8_t *title;
    u8_t *data;
    u8_t *app_id;

}message_info_t;



typedef struct _message_manage_info{
    u16_t version;
    u16_t cur_num;
    //u32_t *first_msg;
    
}message_manage_info_t;


extern message_info_t message_info[MSG_BUF_NUM];

extern message_manage_info_t message_manage_info;








void app_notify_manager_init(void);

bool is_app_notify_empty(void);

bool is_app_notify_full(void);

void app_notify_clear_all(void);

void app_notify_delete(u32_t select_num);

void app_notify_delete_last(void);

message_info_t * app_notify_get(int num);

message_info_t * app_notify_get_first(void);

message_info_t * app_notify_get_last(void);

void app_notify_store_all(void);

void app_notify_add_msg(u16_t type, u32_t time, u8_t* title, u8_t* data ,u8_t *app_id);

ry_sts_t notify_req_handle(u8_t * data, u32_t size);

ry_sts_t notify_cfg_iOS_write_req_handle(u8_t * data, u32_t size);
ry_sts_t notify_cfg_iOS_read_req_handle(u8_t * data, u32_t size);
void OnDeviceUploadNotificationSetting(void);


u32_t app_notify_num_get(void);

void app_notify_tele_answer(int num);

void app_notify_tele_mute(int num);

void app_notify_tele_reject(int num);


#endif
