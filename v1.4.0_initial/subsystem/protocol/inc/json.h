/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __JSON_H__
#define __JSON_H__

#include "ry_type.h"
#include "jsmn.h"
#include "jsmn_api.h"


#define MAX_JSON_TOKEN              280



#define JSON_METHOD_IMG_ADD         "img_add"
#define JSON_METHOD_IMG_REFEASH     "img_refresh"
#define JSON_METHOD_IMG_BRIGHTNESS  "img_adj_brightness"
#define JSON_METHOD_IMG_RESIZE      "img_resize"
#define JSON_METHOD_NFC_FACTORY     "nfc_factory_test"
#define JSON_METHOD_TOUCH_FACTORY   "touch_factory_test"
#define JSON_METHOD_TOUCH_SIMU      "touch_simu"
#define JSON_METHOD_MOTAR           "motar"
#define JSON_METHOD_LED             "led"
#define JSON_METHOD_PM_MODE         "pm_mode"
#define JSON_METHOD_NFC_CE          "nfc_ce"
#define JSON_METHOD_NFC_READER      "nfc_reader"

#define JSON_METHOD_GET_DEVICE_INFO				"get_device_info"
#define JSON_METHOD_GSENSOR						"gsensor"
#define JSON_METHOD_HEARTRATE					"heartrate"
#define JSON_METHOD_NFC							"nfc"
#define JSON_METHOD_VIBRATOR					"vibrator"
#define JSON_METHOD_BLE							"ble"
#define JSON_METHOD_EXFLASH						"exflash"
#define JSON_METHOD_OLED						"oled"
#define JSON_METHOD_TOUCH						"touchpanel"


#define JSON_PARA_KEY							"para"
#define JSON_PARA_ON							"on"
#define JSON_PARA_OFF							"off"
#define JSON_PARA_GET_ID						"get_id"
#define JSON_PARA_GET_VALUE						"get_value"
#define JSON_PARA_START							"start"
#define JSON_PARA_STOP							"stop"
#define JSON_PARA_GET_CPLC						"get_cplc"



#define JSON_ERR_INVALID_PARA                   1001
#define JSON_ERR_METHOD_NOT_FOUND               1002
#define JSON_ERR_DEV_NOT_FOUND                  1003
#define JSON_ERR_METHOD_EXE_FAIL                1004 // for this error, the error string should be the error code of ry_sts
#define JSON_ERR_METHOD_NOT_IMPLEMENTED         1005


#define JSON_ERR_STR_INVALID_PARA              "invalid para"
#define JSON_ERR_STR_METHOD_NOT_FOUND          "method not found"
#define JSON_ERR_STR_METHOD_NOT_IMPLEMENTED    "method not implemented"
#define JSON_ERR_STR_METHOD_EXE_TIMEOUT        "method execute timeout"


#define JSON_SRC_BLE                            0
#define JSON_SRC_UART                           1



typedef enum{
    JSON_METHOD_ID_SAKESYS = 0,
    JSON_METHOD_ID_GSENSOR,
    JSON_METHOD_ID_HRM,
    JSON_METHOD_ID_OLED,
    JSON_METHOD_ID_TOUCH,
    JSON_METHOD_ID_EXFLASH,
    JSON_METHOD_ID_NFC,
    JSON_METHOD_ID_EM9304,
    JSON_METHOD_ID_MOTOR,
    JSON_METHOD_ID_LED,

    JSON_METHOD_ID_IMG_ADD,
    JSON_METHOD_ID_IMG_REFEASH,
    JSON_METHOD_ID_IMG_BRIGHTNESS,
    JSON_METHOD_ID_IMG_RESIZE,
    JSON_METHOD_ID_NFC_FACTORY,
    JSON_METHOD_ID_TOUCH_FACTORY,
    JSON_METHOD_ID_TOUCH_SIMU,
    JSON_METHOD_ID_MOTAR,
    JSON_METHOD_ID_PM_MODE,

    JSON_METHOD_ID_NFC_CE,
    JSON_METHOD_ID_NFC_READER,

    JSON_METHON_ID_END
}json_method_id_t;

typedef struct json_method{
    json_method_id_t id;
    char * method_string;
}json_method_t;



typedef ry_sts_t ( *rpc_write_t )(void* ctx, bool ok, char* ok_str, int err_code, char* err_str);


typedef struct {
    int         session_id;
    int         from;
    rpc_write_t write;
    jsmntok_t   *tok;
} rpc_ctx_t;

/**
 * @brief   Json parser
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
void json_parser(u8_t* data, u32_t len, u8_t from);
bool check_safe_hex_md5(u8_t *input, int len, u8_t *hash);
void get_safe_hex_md5(u8_t * md5_hash, u8_t* len);
ry_sts_t rpc_touch_data_report(rpc_ctx_t* ctx, u32_t data, u32_t len);
ry_sts_t rpc_hr_data_report(rpc_ctx_t* ctx, u32_t data, u32_t len);
void rpc_alg_raw_data_report(int32_t* data, u32_t len);
ry_sts_t rpc_touch_raw_report(rpc_ctx_t* ctx, u32_t* data, u32_t len);




#endif  /* __JSON_H__ */

