/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RBP_H__
#define __RBP_H__

#include "ry_type.h"
#include "rbp.pb.h"
#include "ry_ble.h"

#define MAX_R_XFER_SIZE         4096    // 2048 => 4096
#define DEFAULT_MTU_SIZE        23
typedef enum{
	RBP_RESP_CODE_SUCCESS = 0,
	RBP_RESP_CODE_LOW_POWER = 1,
	RBP_RESP_CODE_DECODE_FAIL = 2,
	RBP_RESP_CODE_NOT_FOUND = 3,
	RBP_RESP_CODE_INVALID_PARA = 4,
	RBP_RESP_CODE_NO_MEM = 5,
	RBP_RESP_CODE_EXE_FAIL = 6,
	RBP_RESP_CODE_TBL_FULL = 7,
	RBP_RESP_CODE_ENCODE_FAIL = 8,
	RBP_RESP_CODE_SIGN_VERIFY_FAIL = 9,
	RBP_RESP_CODE_NO_BIND = 10,
	RBP_RESP_CODE_ALREADY_EXIST = 11,
	RBP_RESP_CODE_INVALID_STATE = 12,
	RBP_RESP_CODE_INVALID_STATE_RUNNING = 13,
	RBP_RESP_CODE_INVALID_STATE_OTA = 14,
	RBP_RESP_CODE_INVALID_STATE_CARD = 15,
	RBP_RESP_CODE_INVALID_STATE_UPLOAD_DATA = 16,
	RBP_RESP_CODE_BUSY = 17,
	RBP_RESP_CODE_OPEN_FAIL = 18,

}rbp_resp_code_t;


typedef struct {
    CMD   cmd;
    u8_t  type;
    u8_t  isSec;
    u32_t code;
    ry_ble_send_cb_t sentCb;
    void*  cbArg;
    int   timerid;
    
    u8_t*  rbpData;
} rbp_para_t;


/**
 * @brief   Ryeex Bluetooth Protocol Parser
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
ry_sts_t rbp_parser(uint8_t* data, uint32_t len);


/**
 * @brief   API to send data through RBP
 *
 * @param   cmd,  which command to send.
 * @param   type, which type to send.
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
//ry_sts_t rbp_send(CMD cmd, TYPE type, uint8_t* data, uint32_t len, uint8_t isSec);

ry_sts_t rbp_send_resp(CMD cmd, uint32_t code, uint8_t* data, uint32_t len, uint8_t isSec);
ry_sts_t rbp_send_req(CMD cmd, uint8_t* data, uint32_t len, uint8_t isSec);
ry_sts_t rbp_send_req_with_cb(CMD cmd, uint8_t* data, uint32_t len, uint8_t isSec, ry_ble_send_cb_t cb, void* arg);
ry_sts_t rbp_send_resp_with_cb(CMD cmd, uint32_t code, uint8_t* data, uint32_t len, uint8_t isSec, ry_ble_send_cb_t cb, void* arg);
void set_online_raw_data_onOff(uint32_t flgOn);


uint8_t get_online_raw_data_onOff(void);

#endif  /* __RBP_H__ */
