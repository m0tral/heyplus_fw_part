/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __DIP_H__
#define __DIP_H__

#include "ry_type.h"
//#include "dev_info.pb.h"
#include "rbp.pb.h"
#include "scheduler.h"


/*
 * @ brief Length of device info
 */
#define CERT_LEN       128//1024
#define MAC_LEN        6
#define MODEL_LEN      50
#define DID_LEN        20
#define FW_VER_LEN     10
#define UID_LEN        10
#define DEV_SN_LEN     4
#define SERVER_SN_LEN  4
#define TIME_LEN       4
#define ACTIVATE_LEN   4
#define FACTORY_SN_LEN 20
#define HW_VER_LEN     10
#define AES_KEY_LEN    16
#define IV_LEN         16

#define MAX_MI_ID_LEN            32
#define MAX_RYEEX_ID_LEN         32



#define DEV_INFO_TYPE_MAC        0
#define DEV_INFO_TYPE_SN         1
#define DEV_INFO_TYPE_DID        2
#define DEV_INFO_TYPE_CERT       3
#define DEV_INFO_TYPE_MODEL      4
#define DEV_INFO_TYPE_VER        5
#define DEV_INFO_TYPE_UID        6
#define DEV_INFO_TYPE_HW_VER     7

#define DEV_INFO_SAFE_HEX_LEN       6526
#define DEV_INFO_SAFE_HEX_MD5_LEN   16

#define DEV_INFO_UNI_OFFSET         0x80



/**
 * @brief   Device Infomation Protocol Parser
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
ry_sts_t dip_parser(CMD cmd, uint8_t* data, uint32_t len);


/**
 * @brief   API to send device infomation data.
 *
 * @param   type, device infomation sub-type
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
//ry_sts_t dip_send(DEV_INFO_TYPE type, uint8_t* data, int len, uint8_t isSec);

/**
 * @brief   API to send mac address.
 *
 * @param   None
 *
 * @return  Status
 */
ry_sts_t dip_send_mac(void);


/**
 * @brief   API to send certification data.
 *
 * @param   None
 *
 * @return  Status
 */
ry_sts_t dip_send_cert(void);

/**
 * @brief   Device Infomation Property Initilization
 *
 * @param   None
 *
 * @return  None
 */
void device_info_init(void);

ry_sts_t device_info_send(void);
ry_sts_t device_status_send(void);
ry_sts_t device_unbind_token_send(void);
ry_sts_t device_info_cert(void);
ry_sts_t device_unbind_handler(void);
ry_sts_t device_unbind_handler_unsecure(uint8_t* data, int len);
ry_sts_t device_bind_ack_handler(uint8_t* data, int len);
void device_bind_ack_cancel_handler(void);

ry_sts_t device_time_sync(uint8_t* data, int len);
ry_sts_t device_timezone_sync(uint8_t* data, int len);

ry_sts_t get_upload_data_handler(void);
ry_sts_t get_property(uint8_t* data, int len);
ry_sts_t set_property(uint8_t* data, int len);

ry_sts_t device_info_get(u8_t type, u8_t* data, u8_t* len);

void get_device_id(u8_t * des, u32_t * len);
void get_sn(u8_t * des, u32_t * len);
void get_mac(u8_t * des, u32_t * len);
void get_safe_hex(u8_t * des, u32_t * len);
void set_fw_ver(u8_t* des);

void set_device_ids(u8_t* des, int len);
void set_device_sn(u8_t* des, int len);
void set_device_key(u8_t* des, int len);
void set_device_mem(int addr, u8_t* des, int len);

void dip_flash_write(u32_t addr, u32_t len, u8_t* data);
u8_t* device_info_get_mac(void);
u8_t* device_info_get_aes_key(void);
void device_activate_start(void);
void device_activate_session_key(uint8_t* data, int len);
bool is_bond(void);
void sign_gen(void);
void ry_device_unbind(ry_ipc_msg_t* msg);
void device_bind_mijia(u8_t bind);
void set_phone_app_info(u8_t * data , u32_t len);
bool is_mijia_bond(void);
bool is_ryeex_bond(void);
ry_sts_t device_state_send(void);
void dip_send_bind_ack(int code);
void repo_card_swiping(u8_t * data, pb_size_t * len);
ry_sts_t device_unbind_request(void);

ry_sts_t device_unbind_request(void);
void unbind(void);
void device_mi_unbind_token_handler(void);
void device_mi_ryeex_did_token_handler(void);
void device_se_activate_result(uint8_t* data, int len);
void device_start_bind_handler(void);
void device_mi_unbind_handler(uint8_t* data, int len);
void device_ryeex_bind_by_token_handler(uint8_t* data, int len);
ry_sts_t phone_app_status_handler(u8_t * data, u32_t size);



#endif  /* __DIP_H__ */
