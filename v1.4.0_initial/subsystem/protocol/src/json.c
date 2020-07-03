/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"

#include "r_xfer.h"
#include "json.h"
#include "ry_hal_uart.h"
#include "ry_hal_flash.h"
#include "ry_tool.h"
#include "ry_hal_adc.h"
#include "ryos_update.h"

//#include "testbench.h"
#include "ry_nfc.h"
#include "nfc_pn80t.h"
#include "ry_gui.h"
#include "dip.h"
#include "md5.h"
#include "app_interface.h"
#include "ry_ble.h"
#include "ff.h"
#include "sensor_alg.h"
#include "board_control.h"
#include "led.h"
#include "am_bsp_gpio.h"
#include "gui.h"
#include "device_management_service.h"
#include "timer_management_service.h"
#include "card_management_service.h"
#include "app_management_service.h"
#include "data_management_service.h"
#include "hrm.h"
#include "touch.h"
#include "gsensor.h"
#include "ry_hal_spi.h"
#include "ryos_timer.h"
#include <stdio.h>
#include "crc32.h"
#include "gui_bare.h"
#include "activity_qrcode.h"
#include "mible.h"
#include <stdlib.h>
#include "ry_font.h"
#include "app_interface.h"

#include "em9304_patches.h"

/*********************************************************************
 * CONSTANTS
 */ 


/*********************************************************************
 * TYPEDEFS
 */

typedef void ( *handler_t )(rpc_ctx_t* ctx, char* msg, int msg_len);

typedef struct {
    char* method;
    handler_t handler;
} cmd_t;


/*********************************************************************
 * VARIABLES
 */
rpc_ctx_t rpc_ctx;
char * json_id = NULL;
int debug_false_color = RELEASE_INTERNAL_TEST;

extern ryos_thread_t    *ry_tool_thread_handle;

extern void touch_set_test_mode(u32_t fEnable, rpc_ctx_t* ctx);
extern void touch_set_raw_report_mode(u32_t fEnable, rpc_ctx_t* ctx);
extern void hr_set_test_mode(u32_t fEnable, rpc_ctx_t* ctx);
extern void gdisp_update(void);

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void rpc_nfc_ce_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_nfc_reader_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_oled_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_devinfo_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_device_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_nfc_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_touch_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_motar_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_exflash_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_hr_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_gsensor_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_set_gsensor_freq_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_ble_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_led_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_set_mode_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_charge_detect_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_set_time_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_get_time_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_reset_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_debug_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_alg_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_setprod_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_xtrim_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_uart_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_color_test_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_upgrade_handler(rpc_ctx_t* ctx, char* msg, int msg_len);

static void rpc_alarm_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_surface_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_app_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_memory_handler(rpc_ctx_t* ctx, char* msg, int msg_len);
static void rpc_home_vibrate_handler(rpc_ctx_t* ctx, char* msg, int msg_len);



static const cmd_t rpc_cmds[] = 
{
	//{ "color",               rpc_color_test_handler },
    //{ "nfc_ce",               rpc_nfc_ce_handler },
    //{ "nfc_reader",           rpc_nfc_reader_handler },
    { "oled",                 rpc_oled_handler},
    
    //{ "img",                  rpc_img_handler},
    { "nfc",                  rpc_nfc_handler},
    { "touch",                rpc_touch_handler},
    { "motar",                rpc_motar_handler},
    { "exflash",              rpc_exflash_handler},
    { "heartrate",            rpc_hr_handler},
    { "gsensor",              rpc_gsensor_handler},
    { "set_gsensor_freq",     rpc_set_gsensor_freq_handler},
    { "ble",                  rpc_ble_handler},
    { "led",                  rpc_led_handler},
    { "set_mode",             rpc_set_mode_handler},
    { "charge_detect",        rpc_charge_detect_handler},
    { "set_time",             rpc_set_time_handler},
    { "get_time",             rpc_get_time_handler},
    { "reset",                rpc_reset_handler},
    { "uart",                 rpc_uart_handler},
    { "debug",                rpc_debug_handler},

    { "device",               rpc_device_handler},
    { "get_device_info",      rpc_devinfo_handler},

    { "alg_raw_switch",       rpc_alg_handler},
    { "set_prod_test_mode",	  rpc_setprod_handler},
    { "set_xtrim",	          rpc_xtrim_handler},

    { "alarm",	              rpc_alarm_handler},
    { "surface",	          rpc_surface_handler},
    { "app",	              rpc_app_handler},
    { "memory",               rpc_memory_handler},   
	{ "home",				  rpc_home_vibrate_handler},
//	{ "touch",				  rpc_upgrade_handler},
};


static bool is_rpc_method(jsmntok_t* tok, const char* msg, char* method)
{
    size_t len;
    char this_method[100] = {0};
    
    if (0 == jsmn_key2val_str(msg, tok, "method", this_method, 100, &len)) {
        if (0 == strcmp(this_method, method)) {
            return TRUE;
        }
    }

    return FALSE;
}


static bool get_rpc_id(jsmntok_t* tok, const char* msg, int *id)
{
    return jsmn_key2val_uint(msg, tok, "id", (u32_t*)id);
}


void rpc_json_tx_cb(ry_sts_t status, void* usrdata)
{
    if(usrdata != NULL) {
        ry_free(usrdata);
    }
    LOG_DEBUG("[rpc_json_tx_cb] TX Callback, status:0x%04x freeheap:%d\r\n", status, ryos_get_free_heap());
    return;
}


static ry_sts_t rpc_ble_ack(void* ctx, bool ok, char* ok_str, int err_code, char* err_str)
{
    char *result;
    int s;
    size_t len, ok_str_len, max_str_len;
    u16_t crc_val;
    rpc_ctx_t *context = (rpc_ctx_t*)ctx;

    if (ok) {
        ok_str_len = strlen(ok_str);
    } else {
        ok_str_len = 0;
    }

    max_str_len = 200 + ok_str_len;

    result = (char*)ry_malloc(max_str_len);
    if (result == NULL) {
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
    }

    ry_memset(result, 0, max_str_len);

    if (ok) {
        snprintf(result, max_str_len, "{\"id\":%d, \"result\":\"%s\"}", context->session_id, ok_str);
    } else {
        snprintf(result, max_str_len, "{\"id\":%d, \"error\":{\"code\":%d,\"message\":\"%s\"}}",
            context->session_id, err_code, err_str);
    }

    len = strlen(result);
    
    /* Add CRC */
    crc_val = calc_crc16((u8_t*)result, len);
    ry_memcpy(&result[len], &crc_val, sizeof(crc_val));
    len += 2;

    return r_xfer_send(R_XFER_INST_ID_BLE_JSON, (u8_t*)result, len, rpc_json_tx_cb, (void*)result);
}


static ry_sts_t rpc_uart_ack(void* ctx, bool ok, char* ok_str, int err_code, char* err_str)
{
    char *result;
    rpc_ctx_t *context = (rpc_ctx_t*)ctx;
    size_t len;

    result = (char*)ry_malloc(200);
    if (result == NULL) {
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
    }

    ry_memset(result, 0, 200);

    if (ok) {
        snprintf(result, 200, "{\"id\":%d, \"result\":\"%s\"}", context->session_id, ok_str);
    } else {
        snprintf(result, 200, "{\"id\":%d, \"error\":{\"code\":%d,\"message\":\"%s\"}}",
            context->session_id, err_code, err_str);
    }

    len = strlen(result);
    
    ry_hal_uart_tx(UART_IDX_TOOL, (u8_t*)result, len);
    ry_free(result);

    return RY_SUCC;
}


char * forceGetValue(char* js, jsmntok_t* tokens, const char* key)
{
	jsmntok_t * key_tk = NULL;
	char * key_str = NULL;
	u32_t pwm = 0;
	if (NULL == (key_tk = (jsmntok_t *)jsmn_key_value_force(js, tokens, key))) {
        LOG_ERROR("error, no value found\r\n");
        return key_str;
    }
    key_str = js + key_tk->start;
    js[key_tk->end] = '\0';

	return key_str;	
}


/**
 * @brief   Json parser
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
void json_parser(u8_t* data, u32_t len, u8_t from)
{
    int  s;
    int  val;
    int  i;
    static int id;
    jsmntok_t  tk_buf[40] = {0};
    jsmn_parser psr;
    int ret = 0;
    int tk_size = 32;
    bool  ok = TRUE;
    int   err_code;
    char* err_str;

    data[len] = 0;
    LOG_DEBUG("[json_parser] len:%d, json:%s\n", len, data);

    if (from == JSON_SRC_BLE) {
        rpc_ctx.from  = JSON_SRC_BLE;
        rpc_ctx.write = rpc_ble_ack;
    } else {
        rpc_ctx.from  = JSON_SRC_UART;
        rpc_ctx.write = rpc_uart_ack;
    }

    jsmn_init(&psr);

    /* Verify Json */
    ret = jsmn_parse(&psr, (const char*)data, len, tk_buf, tk_size);
    if(ret != JSMN_SUCCESS) {
        LOG_ERROR("json parse failed!!!\n");
        ok = FALSE;
        err_code = JSON_ERR_INVALID_PARA;
        err_str  = JSON_ERR_STR_INVALID_PARA;
        goto err_exit;
    }

    rpc_ctx.tok = tk_buf;
    
    if (0 == get_rpc_id(tk_buf, (const char*)data, &id)) {
        if ((id == rpc_ctx.session_id)&&(id != 0xff)) {
            LOG_WARN("duplicate message. drop. \n");
            return;
        }
        rpc_ctx.session_id = id;
    }
    
    /* Search method and process */
    for (i = 0; i < sizeof(rpc_cmds) / sizeof(cmd_t); i++) {
        if (is_rpc_method(tk_buf, (const char*)data, rpc_cmds[i].method)) {
            rpc_cmds[i].handler(&rpc_ctx, (char*)data, len);
            return;
        }
    }

    /* Method not found */
    ok       = FALSE;
    err_code = JSON_ERR_METHOD_NOT_FOUND;
    err_str  = JSON_ERR_STR_METHOD_NOT_FOUND;


err_exit:

    /* Send error response */
    rpc_ctx.write(&rpc_ctx, ok, NULL, err_code, err_str);    
}


ry_sts_t rpc_touch_data_report(rpc_ctx_t* ctx, u32_t data, u32_t len)
{
    char ret_str[20];

    if (!ctx) {
        LOG_ERROR("[rpc_touch_data_report] Invalid parameter.\r\n");
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_INVALIC_PARA);
    }

    if (data == 0) {
    }

    //snprintf(ret_str, 20, "%d", data);
    LOG_DEBUG("rpc_touch_data_report: ok\n\r");
    return ctx->write(ctx, TRUE, "ok", 0, NULL);
}

ry_sts_t rpc_touch_raw_report(rpc_ctx_t* ctx, u32_t* data, u32_t len)
{
    char ret_str[100];

    if (!ctx) {
        LOG_ERROR("[rpc_touch_data_report] Invalid parameter.\r\n");
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_INVALIC_PARA);
    }

    snprintf(ret_str, 100, "%d %d %d %d %d %d %d %d %d %d %d %d", data[0], data[1], data[2], data[3], data[4],
        data[5], data[6], data[7], data[8], data[9], data[10], data[11]);

    //snprintf(ret_str, 20, "%d", data);
    LOG_DEBUG("rpc_touch_data_report: ok\n\r");
    return ctx->write(ctx, TRUE, ret_str, 0, NULL);
}


ry_sts_t rpc_hr_data_report(rpc_ctx_t* ctx, u32_t data, u32_t len)
{
    char ret_str[40];
	hrm_raw_t hrm_raw;

    if (!ctx) {
        LOG_ERROR("[rpc_hr_data_report] Invalid parameter.\r\n");
        return RY_ERR_INVALIC_PARA;
    }
	hrm_get_raw(&hrm_raw);//hr_set_test_mode
    //snprintf(ret_str, 20, "%d", data);
	 
	snprintf(ret_str, 40, "%d %d %d", hrm_raw.led_green,hrm_raw.led_ir,hrm_raw.led_amb);
    return ctx->write(ctx, TRUE, ret_str, 0, NULL);
}

void rpc_alg_raw_data_report(int32_t* data, u32_t len)
{
    int nlen, total_len = 0;
    int max_str_len = 2048;
    u16_t crc_val;
    char* result = (char*)ry_malloc(max_str_len);
    if (result == NULL) {
        LOG_ERROR("[rpc_alg_raw_data_report] No mem.\r\n");
        return;
    }

    nlen = 0;
    nlen += snprintf(result, max_str_len, "{\"id\":1, \"method\":\"alg_raw_data\",\"result\":[");
    
    for (int i = 0; i < 25; i++) {
        nlen += snprintf(result+nlen, max_str_len, "\"%d,%d,%d,%d,%d,%d\",", 
            data[i*6+0], data[i*6+1], data[i*6+2],
            data[i*6+3], data[i*6+4], data[i*6+5]);
    }
    nlen --; // remove last ','
    nlen += snprintf(result+nlen, max_str_len, "]}");
    total_len = strlen(result);
    
    LOG_DEBUG("raw data: len:%d %s\r\n", total_len, result);

    //ry_free((void*)result);
#if 1
    /* Add CRC */
    crc_val = calc_crc16((u8_t*)result, total_len);
    ry_memcpy(&result[total_len], &crc_val, sizeof(crc_val));
    total_len += 2;

    r_xfer_send(R_XFER_INST_ID_BLE_JSON, (u8_t*)result, total_len, rpc_json_tx_cb, (void*)result);
#endif
}



//{"id":123, "method":"oled", "para":"on"}
//{"id":123, "method":"oled", "para":"off"}
//{"id":123, "method":"oled", "para":{"bright":100}}
void rpc_oled_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[40] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20];
    uint32_t value;
    jsmntok_t * param_tk = NULL;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            LOG_DEBUG("OLED mode: ON\n");
            //status = ry_gui_onoff(1);
            //_start_func_oled();
		    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else if (0 == strcmp("off", para_buf)){
            LOG_DEBUG("OLED mode: OFF\n");
            //status = ry_gui_onoff(0);
            //pd_func_oled();
			ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    } else if (NULL != (param_tk = jsmn_key_value(msg, ctx->tok, "para"))) {
        if (STATE_OK == jsmn_key2val_uint(msg, param_tk, "set_rgb", &value)) {
            LOG_DEBUG("set_rgb: 0x%06X\n\r", value);
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_COLOR, 4, (uint8_t*)&value);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else {
        
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "OLED Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


//{"id":123, "method":"img", "para":"on"}
void rpc_img_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status = RY_SUCC;;
    int   err_code;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("add", para_buf)) {
            LOG_DEBUG("OLED mode: ON\n");
            //status = ry_gui_onoff(1);
        } else if (0 == strcmp("refresh", para_buf)){
            LOG_DEBUG("OLED mode: OFF\n");
            //status = ry_gui_onoff(0);
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, "ok", 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "OLED_img Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}

void hex2str_mac(char *buf, const char *hex, int hexlen)
{
	int i;
	for (i = 0; i < hexlen; i++) {
        if (i == 5) {
            sprintf(buf + 2 * i, "%02X", hex[i]);
        } else {
            sprintf(buf + 2 * i, "%02X:", hex[i]);
        }
    }
}


void hex2str(char *buf, const char *hex, int hexlen)
{
	int i;
	for (i = 0; i < hexlen; i++)
		sprintf(buf + 2 * i, "%02x", hex[i]);
}

int tolower(int c)  
{  
    if (c >= 'A' && c <= 'Z')  {  
        return c + 'a' - 'A';  
    }  
    else  {  
        return c;  
    }  
} 

int htoi(char s[])  
{  
    int i;  
    int n = 0;  
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))  {  
        i = 2;  
    }  
    else  {  
        i = 0;  
    }  
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)  {  
        if (tolower(s[i]) > '9')  {  
            n = 16 * n + (10 + tolower(s[i]) - 'a');  
        }  
        else  {  
            n = 16 * n + (tolower(s[i]) - '0');  
        }  
    }  
    return n;  
}

static void rpc_device_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status = JSON_ERR_INVALID_PARA;
    int   err_code;
    char  ret_str[20] = {0};
    u8_t  id;
    u32_t int_value = 0;
    
    jsmntok_t* param_tk = NULL;
    
    if (NULL != (param_tk = jsmn_key_value(msg, ctx->tok, "para")))
    {        
        if (STATE_OK == jsmn_key2val_str(msg, param_tk, "deepsleep", para_buf, 20, &para_len))
        {
            if (0 == strcmp("on", para_buf)) {
                set_deepsleep(1);
                status = RY_SUCC;
                strcpy(ret_str, "on ok"); 
            } else if (0 == strcmp("off", para_buf)){
                set_deepsleep(0);
                status = RY_SUCC;
                strcpy(ret_str, "off ok"); 
            } else if (0 == strcmp("get_state", para_buf)){
                u8_t state = is_deepsleep_active();
                status = RY_SUCC;
                sprintf((char*)ret_str, "state: %d", state);
            }
        }
        else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "time_12hour", para_buf, 20, &para_len))
        {
            if (0 == strcmp("on", para_buf)) {
                set_time_fmt_12hour(1);
                status = RY_SUCC;
                strcpy(ret_str, "on ok"); 
            } else if (0 == strcmp("off", para_buf)){
                set_time_fmt_12hour(0);
                status = RY_SUCC;
                strcpy(ret_str, "off ok"); 
            } else if (0 == strcmp("get_state", para_buf)){
                u8_t state = is_time_fmt_12hour_active();
                status = RY_SUCC;
                sprintf((char*)ret_str, "state: %d", state);
            }
        }
        else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "step_alg", para_buf, 20, &para_len))
        {
            if (0 == strcmp("get_state", para_buf)){
                u8_t state = 0; // get_algo_mode();
                status = RY_SUCC;
                sprintf((char*)ret_str, "state: %d", state);
            }
        }
        else if (STATE_OK == jsmn_key2val_uint(msg, param_tk, "step_alg", &int_value))
        {
            switch (int_value) {
                case 1:
                    //set_algo_hrm_mode();
                    //strcpy(ret_str, "1"); 
                    strcpy(ret_str, "999"); // off state
                    break;
                case 0:
                default:
                    //set_algo_imu_mode();
                    //strcpy(ret_str, "0");
                    strcpy(ret_str, "999"); // off state
                    break;
            }
            
            status = RY_SUCC;
        } 
        else if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len))
        {
            if (0 == strcmp("get_version", para_buf)) {
                status = RY_SUCC;
                snprintf(ret_str, 20, "%s", get_device_version());
            } else if (0 == strcmp("clear_batt_log", para_buf)) {
                status = RY_SUCC;
                extern ry_device_setting_t device_setting;
                
                clear_battery_log();
                ry_memset(&device_setting.battery_log_store_time, 0xFF, sizeof(device_setting.battery_log_store_time));
                set_device_setting(); 
            } else {
                err_code = JSON_ERR_INVALID_PARA;
                strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
                goto err_exit;
            }
        }        
    }    

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "device fail. reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_INVALID_PARA, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}

//{"id":123, "method":"get_device_info", "para":"version"}
void rpc_devinfo_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status = RY_SUCC;
    int   err_code;
    u8_t  ret_len;
    u8_t  ret_data[50];
    u8_t  *ret_str;
    int i, n;
    int isHex = 0;
    jsmntok_t * param_tk = NULL;
    u32_t set_version = 0;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("version", para_buf)) {
            status = device_info_get(DEV_INFO_TYPE_VER, ret_data, &ret_len);
        } else if (0 == strcmp("sn", para_buf)){
            status = device_info_get(DEV_INFO_TYPE_SN, ret_data, &ret_len);
        } else if (0 == strcmp("did", para_buf)){
            status = device_info_get(DEV_INFO_TYPE_DID, ret_data, &ret_len);
        } else if (0 == strcmp("hex_conf", para_buf)){
            get_safe_hex_md5(ret_data, &ret_len);
            isHex = 1;
            status = RY_SUCC;
        }else if (0 == strcmp("hardwareVersion", (const char*)para_buf)){
            ry_memcpy((void*)ret_data, (void *)DEVICE_HARDWARE_VERSION_ADDR, strlen((const char*)DEVICE_HARDWARE_VERSION_ADDR) + 1);
            isHex = 0;
            status = RY_SUCC;
        } else if (0 == strcmp("mac", para_buf)){
            status = device_info_get(DEV_INFO_TYPE_MAC, ret_data, &ret_len);
            isHex = 1;
        } else if (0 == strcmp("update", para_buf)){
            set_fw_ver("0.1.66");
            status = RY_SUCC;
        } else if (0 == strcmp("set_new_ids", para_buf)){
            	
            u8_t data_ids[264] = {
                0x31, 0x33, 0x33, 0x30, 0x39, 0x31, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
//                0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,                 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0x36, 0x42, 0x30, 0x38, 0x4A, 0x36, 0x35, 0x38, 0x33, 0x35, 0x36, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                0x9C, 0xF6, 0xDD, 0x30, 0xFA, 0xF0, 0xFF, 0xFF
            };            
            
            set_device_ids(data_ids, sizeof(data_ids));
            status = RY_SUCC;
        } else if (0 == strcmp("set_new_key", para_buf)){
            	
            u8_t data_ids[56] = {
                0x71, 0x98, 0x6F, 0x17, 0x65, 0x5B, 0x84, 0x14, 0x94, 0x82, 0x01, 0x8E, 0xB1, 0x99, 0xBF, 0xC6, 
                0x78, 0x42, 0x9C, 0xF6, 0xBB, 0x96, 0x88, 0x47, 0x0C, 0x8C, 0x08, 0x58, 0xE7, 0x80, 0x22, 0x9E, 
                0x06, 0xC3, 0x49, 0x60, 0xCA, 0x0C, 0xD7, 0xD0, 0xE4, 0xCE, 0xDA, 0xF8, 0x72, 0x22, 0xCF, 0x78, 
                0x7C, 0x1B, 0xC8, 0x2F, 0xB2, 0x3C, 0xE7, 0xA9
            };                        
            set_device_key(data_ids, sizeof(data_ids));
            
            status = RY_SUCC;
        } else if (0 == strcmp("set_new_sn", para_buf)){
            	
            u8_t data_ids[4] = { 0x1A, 0x03, 0x00, 0x00 };                        
            set_device_sn(data_ids, sizeof(data_ids));
            
            status = RY_SUCC;
        } else if (0 == strcmp("write_mem", para_buf)){
            
            u32_t addr = 0;
            u32_t data = 0;
            u8_t bin[4] = {0};
            
            if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "addr", para_buf, 20, &para_len)) {
                addr = htoi(para_buf);
            }
            if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "data", para_buf, 20, &para_len)) {
                data = htoi(para_buf);
            }
            
            bin[0] = data & 0xFF;
            bin[1] = (data >> 8) & 0xFF;
            bin[2] = (data >> 16) & 0xFF;
            bin[3] = (data >> 24) & 0xFF;
            
            sprintf((char*)ret_data, "addr:0x%x, data:0x%08x", addr, data);

            //u8_t data[4] = { 0x1A, 0x03, 0x00, 0x00 };
            set_device_mem(addr, bin, sizeof(bin));
            
            status = RY_SUCC;            
        } else if (0 == strcmp("cplc", para_buf)){
            //status = device_info_get(DEV_INFO_TYPE_VER, ret_data, &ret_len);
            err_code = JSON_ERR_METHOD_NOT_IMPLEMENTED;
            strcpy(err_str, JSON_ERR_STR_METHOD_NOT_IMPLEMENTED);
            goto err_exit;
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }
    else if (NULL != (param_tk = jsmn_key_value(msg, ctx->tok, "para"))) {
        if (STATE_OK == jsmn_key2val_uint(msg, param_tk, "set_version", &set_version)) {
            LOG_DEBUG("set_version: %d\n\r", set_version);
            sprintf((char*)ret_data, "%d.%d.%02d", (set_version/10000), ((set_version%10000)/100), set_version%100);
            set_fw_ver(ret_data);
            status = RY_SUCC;
        } 
        else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "memory_dump", para_buf, 20, &para_len)) {
            u32_t addr = htoi(para_buf);
            LOG_DEBUG("string:%s, addr: %d\n\r", para_buf, addr);
            sprintf((char*)ret_data, "addr:0x%x, data:0x%08x", addr, *(u32_t*)addr);
            status = RY_SUCC;
        }
        else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "read_mem", para_buf, 20, &para_len)) {
            u32_t addr = htoi(para_buf);
            LOG_DEBUG("string:%s, addr: %d\n\r", para_buf, addr);
            sprintf((char*)ret_data, "addr:0x%x, data:0x%08x%08x", addr, *(u32_t*)addr, *(u32_t*)(addr + 4));
            status = RY_SUCC;
        }
        else {        
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
    }

    if (status == RY_SUCC) {
        ret_str = ry_malloc(ret_len*2+1);
        if (isHex) {
            if(ret_len > 6){
                hex2str((char*)ret_str, (const char*)ret_data, ret_len);
            }else{
                hex2str_mac((char*)ret_str, (const char*)ret_data, ret_len);
            }
        }
        rpc_ctx.write(ctx, TRUE, isHex?(char*)ret_str:(char*)ret_data, 0, NULL);
        ry_free(ret_str);
        return;
    } else {
        snprintf(err_str, 40, "device_info Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}

extern u8_t cplc[];
extern u8_t g_test_nfc_id[];
void rpc_nfc_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[100] = {0};
    int   cnt = 0;
    u8_t* rf_para;
    int   rf_para_len;
	u8_t* rf_hex;
	jsmntok_t * param_tk = NULL;
	u8_t flag = 0;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        
        if (0 == strcmp("se_open", para_buf)){
            card_onSEOpen();
            status = RY_SUCC;
            strcpy(ret_str, "ok");            
        }
        else if (0 == strcmp("se_close", para_buf)) {
            card_onSEClose();
            status = RY_SUCC;
            strcpy(ret_str, "ok");            
        }
        else if (0 == strcmp("on", para_buf)) {
            LOG_DEBUG("nfc_cmd_start.\n\r");
            
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
            while(ry_nfc_is_initialized() == FALSE) {
                ryos_delay_ms(100);
                if (cnt ++ == 50) {
                    break;
                }
            }
            LOG_DEBUG("nfc_info_got.\n\r");
            
            if (cnt >= 50) {
                err_code = JSON_ERR_METHOD_EXE_FAIL;
                strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                LOG_DEBUG("nfc_cmd_fail.\n\r");
                goto err_exit;
            } else {
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
                LOG_DEBUG("nfc_cmd_end.\n\r");
            }
        } else if (0 == strcmp("off", para_buf)){
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
            while(ry_nfc_get_state() != RY_NFC_STATE_IDLE) {
                ryos_delay_ms(100);
                if (cnt ++ == 50) {
                    break;
                }
            }
            
            if (cnt >= 50) {
                err_code = JSON_ERR_METHOD_EXE_FAIL;
                strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                goto err_exit;
            } else {
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
            }
        } else if (0 == strcmp("ce_on", para_buf)){
            if (dev_mfg_state_get() == DEV_MFG_STATE_ACTIVATE) {
                
                
                /*while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
                    ryos_delay_ms(100);
                    if (cnt ++ == 400) {
                        break;
                    }
                }
                if (cnt >= 400) {
                    err_code = JSON_ERR_METHOD_EXE_FAIL;
                    strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                    goto err_exit;
                } else {
                    status = RY_SUCC;
                    strcpy(ret_str, "ok"); 
                    flag = 1;
                }*/
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
                flag = 1;

                clear_buffer_black();
                char * mac = (char*)device_info_get_mac();
                char * qr_code_str = (char * )ry_malloc(100);
                sprintf(qr_code_str, "%s?mac=%02x%%3a%02x%%3a%02x%%3a%02x%%3a%02x%%3a%02x&pid=%d", APP_DOWNLOAD_URL, \
                            mac[0],\
                            mac[1],\
                            mac[2],\
                            mac[3],\
                            mac[4],\
                            mac[5],\
                            911);

                //sprintf(qr_code_str, "http://t.cn/RuMSKQ4");


                display_qrcode(qr_code_str, 30);
                ry_free(qr_code_str);

                rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
                ryos_delay_ms(100);
                ry_hal_uart_powerdown(UART_IDX_TOOL);
                ryos_delay_ms(100);

                ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_ON, NULL);

                while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
                    ryos_delay_ms(100);
                    if (cnt ++ == 400) {
                        break;
                    }
                }

                ryos_delay_ms(2000);

                ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_DOWN, NULL);
                while(ry_nfc_get_state() != RY_NFC_STATE_LOW_POWER) {
                    ryos_delay_ms(100);
                    if (cnt ++ == 800) {
                        break;
                    }
                }


                if (cnt >= 800) {
                    sprintf(ret_str, " 失败 \n");
                } else {
                    sprintf(ret_str, "准备刷卡 \n");
                }

                gdispFillStringBox( 0, 
                        150, 
                        22,
                        22,
                        ret_str, 
                        (void *)NULL, 
                        White,  
                        Black,
                        justifyCenter
                      );

                extern u32_t wFwVerRsp;
                sprintf(ret_str, "%02x%02x%02x", ((wFwVerRsp&0xff0000)>>16), ((wFwVerRsp&0xff00)>>8), (wFwVerRsp&0xff));// Using NFC version, for test
                gdispFillStringBox( 0, 
                        180, 
                        22,
                        22,
                        ret_str, 
                        (void *)NULL, 
                        White,  
                        Black,
                        justifyLeft
                      );
                _start_func_oled(1);
                gdisp_update();
                uint8_t brightness = 30;
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);
                touch_disable();
                ry_hal_wdt_stop();
                pd_func_touch();
                touch_halt(1);
                ry_hal_rtc_stop();    
                extern ryos_thread_t *wms_thread_handle;
                ryos_thread_suspend(wms_thread_handle);
                //ry_hal_uart_powerdown(UART_IDX_TOOL);
            } else {
                ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_ON, NULL);
                while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
                    ryos_delay_ms(100);
                    if (cnt ++ == 50) {
                        break;
                    }
                }
                
                if (cnt >= 50) {
                    err_code = JSON_ERR_METHOD_EXE_FAIL;
                    strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                    goto err_exit;
                } else {
                    status = RY_SUCC;
                    strcpy(ret_str, "ok"); 
                }
            }
        } else if (0 == strcmp("ce_off", para_buf)){
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_OFF, NULL);
            while(ry_nfc_get_state() != RY_NFC_STATE_INITIALIZED) {
                ryos_delay_ms(100);
                if (cnt ++ == 50) {
                    break;
                }
            }
            
            if (cnt >= 50) {
                err_code = JSON_ERR_METHOD_EXE_FAIL;
                strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                goto err_exit;
            } else {
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
            }
        } else if (0 == strcmp("reader_on", para_buf)){
            g_test_nfc_id[0] = 0;
            g_test_nfc_id[1] = 0;
            g_test_nfc_id[2] = 0;
            g_test_nfc_id[3] = 0;

            LOG_DEBUG("nfc reader on. \r\n");
            ryos_delay_ms(100);
            
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_ON, NULL);
            while(ry_nfc_get_state() != RY_NFC_STATE_READER_POLLING) {
                ryos_delay_ms(100);
                if (cnt ++ == 50) {
                    break;
                }
            }
            
            if (cnt >= 50) {
                err_code = JSON_ERR_METHOD_EXE_FAIL;
                strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
                goto err_exit;
            } else {
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
                cnt = 0;
                while ((g_test_nfc_id[0]+g_test_nfc_id[1]+g_test_nfc_id[2]+g_test_nfc_id[3])==0) {
                    ryos_delay_ms(100);
                    if (cnt ++ == 150) {
                        break;
                    }
                }

                ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);

                if (cnt >= 150) {
                    err_code = JSON_ERR_METHOD_EXE_FAIL;
                    strcpy(err_str, "card not detect");
                    goto err_exit;
                } else {
                    status = RY_SUCC;
                    sprintf(ret_str, "%02x%02x%02x%02x", g_test_nfc_id[0],
                        g_test_nfc_id[1], g_test_nfc_id[2], g_test_nfc_id[3]);
									#if 0
                    clear_buffer_black();
                    gdispFillStringBox( 0, 
                            120, 
                            22,
                            22,
                            ret_str, 
                            NULL, 
                            White,  
                            Black,
                            justifyCenter
                          );
                    gdisp_update();
									#endif
                    
                }
            }
        } else if (0 == strcmp("get_cplc", para_buf)){
            status = RY_SUCC;
            hex2str(ret_str, (char*)cplc+3, 42);
        } else if (0 == strcmp("start", para_buf)){
            LOG_DEBUG("nfc_off_cmd_start.\n\r");
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_ON, NULL);
            while(ry_nfc_get_state() != RY_NFC_STATE_READER_POLLING) {
                ryos_delay_ms(100);
                if (cnt ++ == 50) {
                    break;
                }
            }
            LOG_DEBUG("nfc_off_cmd_end.\n\r");            
            if (cnt >= 50) {
                err_code = JSON_ERR_METHOD_EXE_FAIL;
                strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                goto err_exit;
            } else {
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
            }
        } else if (0 == strcmp("stop", para_buf)){
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_OFF, NULL);
            while(ry_nfc_get_state() != RY_NFC_STATE_INITIALIZED) {
                ryos_delay_ms(100);
                if (cnt ++ == 50) {
                    break;
                }
            }
            
            if (cnt >= 50) {
                err_code = JSON_ERR_METHOD_EXE_FAIL;
                strcpy(err_str, JSON_ERR_STR_METHOD_EXE_TIMEOUT);
                goto err_exit;
            } else {
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
            }
        } else if (0 == strcmp("get_value", para_buf)){
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        } else if (0 == strcmp("apply", para_buf)){
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
            nfc_para_block_save();
            ry_system_reset();
            
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    } else if (NULL != (param_tk = jsmn_key_value(msg, ctx->tok, "para"))) {
        rf_para = (u8_t*)ry_malloc(600);
        rf_hex  = (u8_t*)ry_malloc(300);
        if (STATE_OK == jsmn_key2val_str(msg, param_tk, "rf1", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("RF1 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
			nfc_para_len_set(NFC_PARA_TYPE_RF1, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_RF1, rf_hex, cnt);
            
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "rf2", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("RF2 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_RF2, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_RF2, rf_hex, cnt);
            
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "rf3", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("RF3 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_RF3, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_RF3, rf_hex, cnt);
            
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "rf4", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("RF4 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_RF4, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_RF4, rf_hex, cnt);
					  
            //ry_hal_flash_write(0xFE000, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "rf5", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("RF5 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_RF5, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_RF5, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "rf6", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("RF6 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_RF6, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_RF6, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "tvdd_cfg_1", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("TVDD1 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_TVDD1, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_TVDD1, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "tvdd_cfg_2", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("TVDD2 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_TVDD2, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_TVDD2, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "tvdd_cfg_3", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("TVDD3 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_TVDD3, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_TVDD3, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "nxp_core_conf", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("TVDD3 %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_data_dump(rf_hex, cnt, ' ');
            nfc_para_len_set(NFC_PARA_TYPE_NXP_CORE, cnt);
            nfc_para_save(FLASH_ADDR_NFC_PARA_NXP_CORE, rf_hex, cnt);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "mac", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("mac %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_hal_flash_write(0x8100, rf_hex, 6);
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "did", (char*)rf_para, 600, (size_t*)&rf_para_len)) {
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            LOG_DEBUG("did %s. len:%d\r\n", rf_para, strlen((const char*)rf_para));
            int cnt = str2hex((char*)rf_para, strlen((const char*)rf_para), rf_hex);
            LOG_DEBUG("cnt = %d\r\n", cnt);
            ry_hal_flash_write(0x8000, rf_hex, 6);
        } else {
            ry_free(rf_para);
            ry_free(rf_hex);
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }

        ry_free(rf_para);
        ry_free(rf_hex);
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);

        if(flag){
            ry_hal_uart_powerdown(UART_IDX_TOOL);    
        }
        return;
    } else {
        snprintf(err_str, 40, "nfc Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}



void rpc_touch_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20];
    u32_t id;
    int delay_ack = 0;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
		
	    if (0 == strcmp("upgrade", para_buf)) {
	        uint32_t tp_id = touch_get_id();
	        if (tp_id != get_tp_version()) {
                if(tp_firmware_upgrade()==0)
                {
                    status = RY_SUCC;
                    strcpy(ret_str, "ok"); 
                }
                else{
                    err_code = JSON_ERR_INVALID_PARA;
                    strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
                    goto err_exit;
                } 
            }else{
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
            }
        } else if (0 == strcmp("on", para_buf)) {
            touch_enable();
            touch_set_test_mode(0, NULL);
            touch_set_raw_report_mode(0, NULL);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("off", para_buf)){            
            touch_disable();
            //pd_func_touch();
            touch_set_test_mode(0, NULL);
            touch_set_raw_report_mode(0, NULL);
            //clear_buffer_black();
            //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
            //gdisp_update();
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("get_id", para_buf)){
            id = touch_get_id();
            snprintf(ret_str, 20, "%04x", id);
            status = RY_SUCC;
        } else if (0 == strcmp("get_data", para_buf)){
            u32_t value = 0;
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_COLOR, 4, (uint8_t*)&value);
            touch_set_test_mode(1, ctx);
            strcpy(ret_str, "ok"); 
            status = RY_SUCC;
            delay_ack = 0;
        } else if (0 == strcmp("get_raw", para_buf)){
            touch_set_raw_report_mode(1, ctx);
            strcpy(ret_str, "ok"); 
            status = RY_SUCC;
            delay_ack = 0;
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }

    if (delay_ack) {
        return;
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "touch Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


void rpc_hr_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20] = {0};
    u8_t  id;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            _start_func_hrm(HRM_START_FIXED);
            set_hrm_working_mode(HRM_MODE_MFG_JSON);
			//set_auto_heart_rate_time(20);
			//set_auto_heart_rate_open();
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("off", para_buf)){
            pcf_func_hrm();
            hr_set_test_mode(0, NULL);
			//set_auto_heart_rate_close();
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("get_data", para_buf)){
            hr_set_test_mode(1, ctx);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("get_id", para_buf)){
            //id = hrm_get_id();
            id = 123;
            status = RY_SUCC;
            snprintf(ret_str, 20, "%02x", id);
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }

     // LOG_DEBUG("[rpc_hr_handler]return_string: %s\n\r", ret_str);                    
    // LOG_DEBUG("[rpc_hr_handler]error_string: %s\n\r", err_str);   
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "hrm Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


void rpc_motar_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
		char ret_str[20];
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            motor_set_working(100, 1000);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("off", para_buf)){
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, "ok", 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "motor Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}



void rpc_ble_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[80] = {0};
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("get_id", para_buf)) {
            if (ry_ble_get_state() > RY_BLE_STATE_IDLE) {
                status = RY_SUCC;
                strcpy(ret_str, "ok");
            } else {
                strcpy(err_str, "BLE communication fail.");
            }
        }
        else if (0 == strcmp("get_patch_list", para_buf)) {
            status = RY_SUCC;
            
            char temp[5] = "";
            snprintf(temp, 5, "%02x|", em9304init_status);
            strcat(ret_str, temp);
            
            for (uint32_t i = 0; i < EM9304_PATCHINFO_NUM_PATCHES; i++)
            {
                char temp[8] = "";
                u8_t cid = g_pEm9304PatchesInfo[i].containerID;
                u8_t cloc = g_pEm9304PatchesInfo[i].location;
                u8_t cgot = g_pEm9304PatchesInfo[i].present;
                snprintf(temp, 8, "%02d:%01d:%01d|", cid, cgot, cloc);
                strcat(ret_str, temp);
            }
        }
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "BLE Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}



void rpc_gsensor_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[40] = {0};
    gsensor_acc_t acc_data;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            status = gsensor_init();
            if (RY_SUCC == status) {
                strcpy(ret_str, "ok");
            } 
        } else if (0 == strcmp("off", para_buf)){
            pd_func_gsensor();
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else if (0 == strcmp("get_id", para_buf)){
            id = gsensor_get_id();
            snprintf(ret_str, 20, "%02x", id);
            status = RY_SUCC;
        } else if (0 == strcmp("get_data", para_buf)){
            gsensor_get_xyz(&acc_data);
            snprintf(ret_str, 40, "%d:%d:%d", acc_data.acc_x, acc_data.acc_y, acc_data.acc_z);
            status = RY_SUCC;
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }         
    }

    // LOG_DEBUG("[rpc_gsensor_handler]return_string: %s\n\r", ret_str);                    
    // LOG_DEBUG("[rpc_gsensor_handler]error_string: %s\n\r", err_str);                    
    
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "gsensor Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


void rpc_exflash_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20];
    u32_t id;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("get_id", para_buf)) {
            //LOG_DEBUG("exflash_cmd_start.\n\r");            
            id = exflash_get_id();
            snprintf(ret_str, 20, "%08x", id);
            status = RY_SUCC;
        } else if (0 == strcmp("verify_content", para_buf)){
            FIL * fp = ry_malloc(sizeof(FIL));
            FRESULT res ;
            res = f_open(fp,"resource_info.txt",FA_READ);
            if(res == FR_OK){
                f_close(fp);
            }    

            if(res != FR_OK){
                //LOG_DEBUG("exflash_open_fail_1.%d\n\r",res);   
                res = f_open(fp,"test_fs.txt",FA_READ);
                if(res == FR_OK){
                    f_close(fp);
                }    
            }
            ry_free(fp);
            if(res != FR_OK){
                //LOG_DEBUG("exflash_open_fail.%d\n\r",res); 
                strcpy(ret_str, "open fail"); 
                status = 1;
            }else{
                status = RY_SUCC;
                strcpy(ret_str, "ok"); 
            }
        } else if (0 == strcmp("clear_fw_flash", para_buf)){            
            int j = 0;
            for(j = 0 ; j < FIRMWAVE_FILE_MAX_SECTOR_NUM ; j++){
                ry_hal_wdt_reset();
                ry_hal_spi_flash_sector_erase(FIRMWARE_STORE_ADDR + j * EXFLASH_SECTOR_SIZE);
            }
            strcpy(ret_str, "ok"); 
            status = RY_SUCC;            
        } else if (0 == strcmp("restore_fw_flash", para_buf)){            
//            int j = 0;
//            for(j = 0 ; j < FIRMWAVE_FILE_MAX_SECTOR_NUM ; j++){
//                ry_hal_wdt_reset();
//                ry_hal_spi_flash_sector_erase(FIRMWARE_STORE_ADDR + j * EXFLASH_SECTOR_SIZE);
//            }
            strcpy(ret_str, "ok"); 
            status = RY_SUCC;
        } else if (0 == strcmp("format_fs", para_buf)){                        
            u8_t *work = (u8_t *)ry_malloc(4096 *2);
            f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096 *2);
            ry_free(work);
            
//            extern ry_device_setting_t device_setting;
//            
//            clear_battery_log();
//            ry_memset(&device_setting.battery_log_store_time, 0 , sizeof(device_setting.battery_log_store_time));
//            set_device_setting();            
            
            strcpy(ret_str, "ok"); 
            status = RY_SUCC;            
        } else if (0 == strcmp("mass_erase", para_buf)) {
            
            ry_hal_wdt_reset();
            exflash_mass_erase();
            
            strcpy(ret_str, "ok");
            status = RY_SUCC;
        } else if (0 == strcmp("chip_erase", para_buf)) {
            
            ry_hal_wdt_reset();
            exflash_chip_erase();
            
            strcpy(ret_str, "ok");
            status = RY_SUCC;
        } else {
            //
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
    }

    // LOG_DEBUG("[rpc_exflash_handler]return_string:%s\n\r", ret_str);   
    // LOG_DEBUG("[rpc_exflash_handler]eeeor_string:%s\n\r", err_str);                    
    
    if (status == RY_SUCC) {
        //LOG_DEBUG("exflash_cmd_end.\n\r");                    
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "exflash Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


void rpc_set_gsensor_freq_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            err_code = JSON_ERR_METHOD_NOT_IMPLEMENTED;
            strcpy(err_str, JSON_ERR_STR_METHOD_NOT_IMPLEMENTED);
            goto err_exit;
        } else if (0 == strcmp("off", para_buf)){
            err_code = JSON_ERR_METHOD_NOT_IMPLEMENTED;
            strcpy(err_str, JSON_ERR_STR_METHOD_NOT_IMPLEMENTED);
            goto err_exit;
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }

    // LOG_DEBUG("[rpc_set_gsensor_freq_handler]return_string:%s\n\r", ret_str);   
    // LOG_DEBUG("[rpc_set_gsensor_freq_handler]eeeor_string:%s\n\r", err_str);    
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, "ok", 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "gsensor_freq Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}

bool check_safe_hex_md5(u8_t *input, int len, u8_t *hash)
{
    MD5_CTX md5;

	MD5Init(&md5);
	MD5Update(&md5, input, len);
	MD5Final(hash, &md5);

    return true;
}

void get_safe_hex_md5(u8_t * md5_hash, u8_t* len)
{
    u32_t version_check = 0;
    u32_t version_flag = 0;
    u32_t size = 0;

    u32_t * head = ((u32_t *)(RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3 + sizeof(u32_t)));

    if ((head[0] == 0)&&(head[1] == 1)) {
        version_flag = 1;
    }


    if(version_flag == 0){
        size = *((u32_t *)(RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3));
        u8_t * safe_hex = (u8_t *)(RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3 + sizeof(u32_t) );
        *len = DEV_INFO_SAFE_HEX_MD5_LEN;

        ry_memcpy(md5_hash, &safe_hex[size - DEV_INFO_SAFE_HEX_MD5_LEN], DEV_INFO_SAFE_HEX_MD5_LEN);
    }else if(version_flag == 1){
        head = (u32_t *)(RY_XIAOMI_CERT);
        size = head[1];
        u8_t * safe_hex = (u8_t *)&head[2];
        *len = DEV_INFO_SAFE_HEX_MD5_LEN;
        ry_memcpy(md5_hash, &safe_hex[size], DEV_INFO_SAFE_HEX_MD5_LEN);
    }

    
}


static void rpc_led_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[40] = {0};
    gsensor_acc_t acc_data;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("enable", para_buf)) {
            ry_led_enable_set(1);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } 
        else  if (0 == strcmp("disable", para_buf)) {
            ry_led_enable_set(0);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } 
        else if (0 == strcmp("on", para_buf)) {
            //led_set_working(1, 100, 3000);
            clear_hrm_st_prepare();
            ry_led_enable_set(1);
            ry_led_onoff(1);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else if (0 == strcmp("off", para_buf)){
            ry_led_enable_set(1);
            ry_led_onoff(0);
            //ry_led_enable_set(0);
            status = RY_SUCC;
            strcpy(ret_str, "ok");

        }else if (0 == strcmp("10", para_buf)) {

            
            led_set_working(0, 10, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        /*}else if (0 == strcmp("20", para_buf)) {
            led_set_working(0, 20, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("30", para_buf)) {
            led_set_working(0, 30, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("40", para_buf)) {
            led_set_working(0, 40, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("50", para_buf)) {
            led_set_working(0, 50, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("60", para_buf)) {
            led_set_working(0, 60, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("70", para_buf)) {
            led_set_working(0, 70, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("80", para_buf)) {
            led_set_working(0, 80, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("90", para_buf)) {
            led_set_working(0, 90, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");*/
        } else {
            int a = atoi(para_buf);
            led_set_working(0, a, 30000);
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            /*err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;*/
        }
         
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "LED Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}

void set_mode(void)
{
    pcf_func_hrm();
    hr_set_test_mode(0, NULL);
    pd_func_touch();
    touch_set_test_mode(0, NULL);
    pd_func_oled();
    ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
}



static void rpc_set_mode_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[40] = {0};

    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("mfc_test_mode", para_buf)) {
            status = RY_SUCC;
            //suspend ui touch wms
            pd_func_touch();      
            ry_hal_rtc_stop();                  
            extern ryos_thread_t *wms_thread_handle;
            extern ryos_thread_t  *gui_thread3_handle;
            ryos_thread_suspend(wms_thread_handle);
            ryos_thread_suspend(gui_thread3_handle);
            LOG_DEBUG("[mfc_test_mode] start.\n\r");
            strcpy(ret_str, "ok");
        } 
        else if (0 == strcmp("idle", para_buf)) {
            set_mode();
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }else if (0 == strcmp("off", para_buf)) {
            //set_mode();
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }
        else if (0 == strcmp("reboot", para_buf)) {
            //set_mode();
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        }
        else if (0 == strcmp("mfg_status", para_buf)) {
            status = RY_SUCC;
            snprintf(ret_str, 20, "0x%x", dev_mfg_state_get());
        }
        else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        if (0 == strcmp("off", para_buf)){
            ryos_delay_ms(200);            
            motor_set_working(100, 1000);
            ryos_delay_ms(1000);                        
            // ry_hal_mcu_reset();   
            power_off_ctrl(1);
        }

        if (0 == strcmp("reboot", para_buf)) {
             ryos_delay_ms(200);            
            motor_set_working(100, 1000);
            ryos_delay_ms(1000);   
            ry_hal_mcu_reset();   
        }
        return;
    } else {
        snprintf(err_str, 40, "set mode Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}

static rpc_ctx_t* dc_test_ctx  = NULL;
void charge_repo(void)
{
    char  ret_str[40] = {0};
    extern u8_t dc_in_flag, dc_in_change;
    // LOG_DEBUG("charge_repo.0x%2x\n\r", dc_in_change);
    if ((dc_in_change & 0x0f)&&(dc_in_change & 0xf0)){
        if(dc_in_flag){
             strcpy(ret_str, "in");
        }else{
             strcpy(ret_str, "none");
        }
        dc_in_change &= 0x0f;
        rpc_ctx.write(dc_test_ctx, TRUE, ret_str, 0, NULL);
    }
    
}

static void rpc_charge_detect_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[40] = {0};
    int   bOn = 0;
    extern u8_t dc_in_flag, dc_in_change;

    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            //LOG_DEBUG("charge_start.0x%2x\n\r", dc_in_change);
            dc_in_change |= 0x10;
            status = RY_SUCC;
            dc_test_ctx = ctx;
            bOn = 1;
            strcpy(ret_str, "ok");
        } else if (0 == strcmp("off", para_buf)){
            // dc_in_change = 0x0;
            bOn = 0;
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
         
    }
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        //LOG_DEBUG("charge_send_cmd_1.0x%2x\n\r", dc_in_change);
        if (bOn) {
            ryos_delay_ms(100);
            am_hal_gpio_pin_config(AM_BSP_GPIO_DC_IN, AM_HAL_GPIO_INPUT/*|AM_HAL_GPIO_PULL24K*/);
            if (am_hal_gpio_input_bit_read(AM_BSP_GPIO_DC_IN))	{
                strcpy(ret_str, "in");
                rpc_ctx.write(dc_test_ctx, TRUE, ret_str, 0, NULL);
            } else {
                strcpy(ret_str, "none");
                rpc_ctx.write(dc_test_ctx, TRUE, ret_str, 0, NULL);
            }
        }

        return;
    } else {
        snprintf(err_str, 40, "charge Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);

}


static void rpc_set_time_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[40] = {0};
    u32_t tick = 0;

    if (STATE_OK == jsmn_key2val_uint(msg, ctx->tok, "para", &tick)) {
        ryos_utc_set(tick);
        ry_hal_update_rtc_time();
        status = RY_SUCC;
        strcpy(ret_str, "ok");
    }
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "set_time. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);

}



static void rpc_get_time_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[40] = {0};
    u32_t tick = 0;

    snprintf(ret_str, 40, "%d", ryos_utc_now());
    rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
    return;
}


static void rpc_reset_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    ry_led_onoff(1);
    ryos_delay(300);
    ry_led_onoff(0);
    ryos_delay(200);
    ry_led_onoff(1);
    ryos_delay(300);
    
    ry_system_reset();
}


static void rpc_debug_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    u8_t  id;
    char  ret_str[60] = {0};
    extern u8_t dc_in_flag, dc_in_change;
    uint8_t battery_percent;

    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("set_true_color", para_buf)) {
            debug_false_color = 0;
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else if (0 == strcmp("set_false_color", para_buf)){
            debug_false_color = 1;
            status = RY_SUCC;
            strcpy(ret_str, "ok");
        } else if (0 == strcmp("set_hw_ver", para_buf)){
            set_hardwarVersion(4);
            status = RY_SUCC;
        } else if (0 == strcmp("reset_bind_status", para_buf)){
            extern ry_device_bind_status_t device_bind_status;
            device_bind_status.status.wordVal = 0;
            ry_memset(device_bind_status.ryeex_uid, 0, MAX_RYEEX_ID_LEN);
            ry_memset(device_bind_status.mi_uid, 0, MAX_MI_ID_LEN);
            
            status = RY_SUCC;
        } else if (0 == strcmp("get_adc", para_buf)){
            battery_percent = sys_get_battery_percent(BATT_DETECT_AUTO);
            //snprintf(ret_str, 40, "\"adc\":0x%x, \"percent\":%d", g_test_adc_result, battery_percent);
            status = RY_SUCC;
        } else if (0 == strcmp("update_tp", para_buf)){
            
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            tp_firmware_upgrade();
        } else if (0 == strcmp("unbind", para_buf)){
            
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            unbind();
        } else if (0 == strcmp("start_uart_tool", para_buf)){
            if (ry_tool_thread_handle == NULL){
                ry_tool_thread_init();
            }
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            ry_tool_init();
        } else if (0 == strcmp("reset_cards", para_buf)){
            
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            card_accessCards_reset();
            card_transitCards_reset();
            
        } else if (0 == strcmp("get_memory", para_buf)){
            
            snprintf(ret_str, 20, "%d", ryos_get_free_heap());
            status = RY_SUCC;
            
        } else if (0 == strcmp("get_token", para_buf)){
            u8_t token[12];
            mible_getToken(token);
            snprintf(ret_str, 20, "%02x%02x%02x%02x", token[0], token[1], token[3], token[4]);
            status = RY_SUCC;
            
        } else if (0 == strcmp("mkfs", para_buf)){
            
            status = RY_SUCC;
            strcpy(ret_str, "ok");
            u8_t *work = (u8_t *)ry_malloc(4096 *2);
		    f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096 *2);
            ry_free(work);
            add_reset_count(FACTORY_TEST_RESTART);
            //set_fw_ver("0.0.2");
            
        } else if (0 == strcmp("check_resource", para_buf)){
            int j = 0;
            u32_t start_addr = 1024*1024;
            u32_t end_addr = 7 * 1024 * 1024;
            u32_t sector_count = (end_addr - start_addr)/4096;
            u8_t * temp_buf = (u8_t *)ry_malloc(4096);
            u16_t crc= 0;

            for(j = 0; j < sector_count; j++){
                ry_hal_spi_flash_read(temp_buf, start_addr + j * 4096, 4096);
                crc = calc_crc16_r(temp_buf, 4096, crc);
            }

            if(crc == 21122){
                status = RY_SUCC;
                strcpy(ret_str, "ok");
            }else{

                status = crc;
            }
            
        } else if (0 == strcmp("start_data_upload", para_buf)){
            status = RY_SUCC;
            snprintf(ret_str, 20, "ok, count: %d", get_data_set_num());
            rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
            ry_data_msg_send(DATA_SERVICE_UPLOAD_DATA_START, 0, NULL);
            return;
            
        }else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }         
    }
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        if ((0 == strcmp("set_true_color", para_buf)) || (0 == strcmp("set_false_color", para_buf))){
            ryos_delay_ms(50);
            app_ui_update(1);
        }
        return;
    } else {
        snprintf(err_str, 40, "debug Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}



static void rpc_alg_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20] = {0};
    u8_t  id;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
            alg_report_raw_data(1);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("off", para_buf)){
            alg_report_raw_data(0);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "ALG Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


u32_t factory_test_start = 0;

static void rpc_setprod_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20] = {0};
    u8_t  id;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("semi_finish", para_buf)) {
            uart_gpio_mode_init();
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("finish", para_buf)){
            set_factory_test_finish(DEV_MFG_STATE_DONE);
            dev_mfg_state_set(DEV_MFG_STATE_DONE);
            set_device_setting();
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("pcba", para_buf)){
            dev_mfg_state_set(DEV_MFG_STATE_PCBA);
            set_device_setting();
            /* Close some periodic task */
            periodic_task_set_enable(periodic_touch_task, 0);
            periodic_task_set_enable(periodic_hr_task, 0);

            /* Disable wrist filt */
            set_raise_to_wake_close();
            ry_led_onoff(0);
            touch_mode_set(TP_MODE_BTN_ONLY);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("activate", para_buf)){
            dev_mfg_state_set(DEV_MFG_STATE_ACTIVATE);
            ry_hal_wdt_stop();
            dc_in_hw_disable();
            pd_func_touch();
            touch_halt(1);
            extern ryos_thread_t *tms_thread_handle;
            extern ryos_thread_t *wms_thread_handle;
            ryos_thread_suspend(tms_thread_handle);
            ryos_thread_suspend(wms_thread_handle);
            extern int qrcode_timer_id;
            ry_timer_stop(qrcode_timer_id);
            set_device_setting();
            //ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("semitest", para_buf)){
            dev_mfg_state_set(DEV_MFG_STATE_SEMI);
            set_device_setting();
            ry_hal_wdt_stop();
            //extern int touch_halt_flag;
            //touch_halt_flag = 0;
            //touch_halt(1);
            cy8c4011_gpio_uninit();
            ry_hal_rtc_stop();                  
            extern ryos_thread_t *wms_thread_handle;
            extern ryos_thread_t *tms_thread_handle;
            //extern ryos_thread_t  *gui_thread3_handle;
            extern int qrcode_timer_id;
            ry_timer_stop(qrcode_timer_id);
            ryos_thread_suspend(wms_thread_handle);
            ryos_thread_suspend(tms_thread_handle);
            //ryos_thread_suspend(gui_thread3_handle);
            LOG_DEBUG("[mfc_test_mode] start.\n\r");
            //clear_buffer_black();
            //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
            //am_devices_rm67162_display_normal_area(0,119,0,239);
            //am_devices_rm67162_fixed_data_888(0, 120*240*3);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else if (0 == strcmp("finaltest", para_buf)){
            dev_mfg_state_set(DEV_MFG_STATE_FINAL);
            set_device_setting();
            ry_hal_wdt_stop();
            pd_func_touch();      
            ry_hal_rtc_stop();                  
            extern ryos_thread_t *wms_thread_handle;
            extern ryos_thread_t  *gui_thread3_handle;
            extern int qrcode_timer_id;
            ry_timer_stop(qrcode_timer_id);
            ryos_thread_suspend(wms_thread_handle);
            //ryos_thread_suspend(gui_thread3_handle);
            //LOG_DEBUG("[mfc_test_mode] start.\n\r");
            //clear_buffer_black();
            //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } else {
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
    }

    factory_test_start = 1;

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "setprod Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}



extern uint32_t xtrim_regValue;

static void rpc_xtrim_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20] = {0};
    u32_t  xdeviation;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) 
	{     
		// xdeviation=atoi(para_buf);
		// para_buf[0]=0x00;
		// para_buf[1]=0xf0;
		// para_buf[2]=0x04;
		// para_buf[3]=0x24;
		
		// para_buf[4]=0x05;
		// para_buf[5]=0x00;
		// para_buf[6]=xdeviation;
		// para_buf[7]=0xe0;
		
		// HciVendorSpecificCmd(0xFC022,8,para_buf);
		
		// set_factory_test_finish(xdeviation);
		
        extern uint32_t xtrim_regValue;
        sprintf((char*)ret_str, "ok: 0x%08x", xtrim_regValue);

        // set_factory_test_finish(1);
        status = RY_SUCC;
        // strcpy(ret_str, "ok");      
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "setprod Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


static void rpc_uart_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
    char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code;
    char  ret_str[20] = {0};
    u8_t  id;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("off", para_buf)) {
            //uart_gpio_mode_init();
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
        } 
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        ry_hal_uart_powerdown(UART_IDX_TOOL);
        return;
    } else {
        snprintf(err_str, 40, "setprod Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}


static void rpc_alarm_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
}

static void rpc_surface_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
	char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status = -1;
    int   err_code = 0;
    char  ret_str[20] = {0};
    u8_t  id;
    jsmntok_t* param_tk = NULL;
    
    if (NULL != (param_tk = jsmn_key_value(msg, ctx->tok, "para")))
    {        
        if (STATE_OK == jsmn_key2val_str(msg, param_tk, "weather_main", para_buf, 20, &para_len))
        {
            if (0 == strcmp("on", para_buf)) {
                set_surface_weather(1);
                status = RY_SUCC;
                strcpy(ret_str, "on ok"); 
            } else if (0 == strcmp("off", para_buf)){
                set_surface_weather(0);
                status = RY_SUCC;
                strcpy(ret_str, "off ok"); 
            } else if (0 == strcmp("get_state", para_buf)){
                u8_t state = is_surface_weather_active();
                status = RY_SUCC;
                sprintf((char*)ret_str, "state: %d", state);
            }
        } else if (STATE_OK == jsmn_key2val_str(msg, param_tk, "alarm", para_buf, 20, &para_len)) {
            if (0 == strcmp("on", para_buf)) {
                set_surface_nextalarm(1);
                status = RY_SUCC;
                strcpy(ret_str, "on ok"); 
            } else if (0 == strcmp("off", para_buf)){
                set_surface_nextalarm(0);
                status = RY_SUCC;
                strcpy(ret_str, "off ok"); 
            } else if (0 == strcmp("get_state", para_buf)){
                u8_t state = is_surface_nextalarm_active();
                status = RY_SUCC;
                sprintf((char*)ret_str, "state: %d", state);
            }
        }        
    }
    
    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    }
    else {
        err_code = JSON_ERR_INVALID_PARA;
        strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
        rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);    
    }
}

static void rpc_app_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{}

static void rpc_memory_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{}

static void rpc_home_vibrate_handler(rpc_ctx_t* ctx, char* msg, int msg_len)
{
	char para_buf[20] = {0};
    char err_str[40] = {0};
    size_t para_len;
    ry_sts_t status;
    int   err_code = 0;
    char  ret_str[20] = {0};
    u8_t  id;
    
    if (STATE_OK == jsmn_key2val_str(msg, ctx->tok, "para", para_buf, 20, &para_len)) {
        if (0 == strcmp("on", para_buf)) {
			ry_motar_enable_set(1);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
			//LOG_DEBUG("Home on success!\r\n");
        } else if (0 == strcmp("off", para_buf)){
			ry_motar_enable_set(0);
            status = RY_SUCC;
            strcpy(ret_str, "ok"); 
			//LOG_DEBUG("Home off success!\r\n");
        } else {
			LOG_DEBUG("Home error success!\r\n");
            err_code = JSON_ERR_INVALID_PARA;
            strcpy(err_str, JSON_ERR_STR_INVALID_PARA);
            goto err_exit;
        }
    }

    if (status == RY_SUCC) {
        rpc_ctx.write(ctx, TRUE, ret_str, 0, NULL);
        return;
    } else {
        snprintf(err_str, 40, "Home vibrate Fail. Reason=0x%04x", status);
        rpc_ctx.write(ctx, FALSE, NULL, JSON_ERR_METHOD_EXE_FAIL, err_str);
        return;
    }

err_exit:
    rpc_ctx.write(ctx, FALSE, NULL, err_code, err_str);
}
