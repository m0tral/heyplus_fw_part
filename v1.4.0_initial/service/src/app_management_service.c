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
#include "app_pm.h"
#include "app_management_service.h"
#include "window_management_service.h"
#include "device_management_service.h"
#include "ry_gui_disp.h"
#include "ry_hal_flash.h"
#include "ry_list.h"
#include "scheduler.h"
#include "string.h"
#include "ryos_timer.h"

#include "rbp.h"
#include "nanopb_common.h"
#include "DevApp.pb.h"
#include "touch.h"
#include "gui.h"
#include "gui_bare.h"
#include "am_util_debug.h"
#include "gui_img.h"
#include "ry_fs.h"
#include "ry_font.h"
#include "ry_hal_inc.h"


/*********************************************************************
 * CONSTANTS
 */ 




/*********************************************************************
 * TYPEDEFS
 */




typedef struct {
    int curNum;
    int app_seq[MAX_APP_NUM];
} app_ctx_t;

 
/*********************************************************************
 * LOCAL VARIABLES
 */
app_prop_t app_props[MAX_APP_NUM] = 
{
    {APPID_SYSTEM_CARD,    "menu_01_card.bmp",     "Карты"},
    {APPID_SYSTEM_DATA,    "menu_08_data.bmp",     "Статистика"},
    {APPID_SYSTEM_HRM,     "menu_02_hrm.bmp",      "Пульс"},
    {APPID_SYSTEM_SPORT,   "menu_03_sport.bmp",    "Спорт"},
    {APPID_SYSTEM_WEATHER, "menu_05_weather.bmp",  "Погода"},
    {APPID_SYSTEM_ALARM,   "menu_06_clock.bmp",    "Будильник"},
//    {APPID_SYSTEM_MIJIA,   "menu_04_mijia.bmp",    "Умный дом"},
    {APPID_SYSTEM_MIJIA,   "menu_04_mijia.bmp",    "Mijia"},    
    {APPID_SYSTEM_SETTING, "menu_09_setting.bmp",  "Настройки"},
    #if (CUR_APP_NUM >= 9)
    {APPID_SYSTEM_ALIPAY,  "menu_10_alipay.bmp",   "Alipay"},
    #endif
    #if (CUR_APP_NUM >= 10)
    {APPID_SYSTEM_TIMER,   "menu_07_stopwatch.bmp",    "Секундомер"},
    #endif
};

app_prop_t app_missing = { APPID_SYSTEM_NONE, "menu_01_card.bmp", "Missing" };


app_ctx_t app_ctx_v;



/*********************************************************************
 * LOCAL FUNCTIONS
 */


/**
 * @brief   API to get read exflash image resources to internal
 *
 * @param   reload - 1: load img very time, 0: auto detect
 *
 * @return  res  - result of exflash image resources move
 *			       reference - File function return code (FRESULT)
 */
extern ryos_mutex_t* spi_mutex;


int check_app_ctx(void)
{
    int flag = 0;
    int i = 0, j = 0;
    if(app_ctx_v.curNum != CUR_APP_NUM){
        //app_ctx_v.curNum = CUR_APP_NUM;

        if (app_ctx_v.curNum < CUR_APP_NUM ) {
            app_ctx_v.app_seq[CUR_APP_NUM - 1] = CUR_APP_NUM;
        } else {
            
        }
        app_ctx_v.curNum = CUR_APP_NUM;
    }
    
    for (i = 1; i <= CUR_APP_NUM; i++) {
        flag = 1;
        for(j = 0; j < CUR_APP_NUM; j++){
            if(app_ctx_v.app_seq[j] == i){
                flag = 0;
            }
        }

        if (flag == 1) {
            break;
        }
    }

    if(flag == 1){
        for(j = 0; j < CUR_APP_NUM; j++){
            app_ctx_v.app_seq[j] = j + 1;
        }
        
    }
    return flag;
}


uint8_t app_ui_update(uint8_t reload)
{
    touch_disable();
    
	uint8_t * buff = (uint8_t *)ry_malloc(IMG_MAX_BYTE_PER_LINE);
	uint32_t written_bytes;
	FRESULT res ;
	FIL* file = (FIL*)ry_malloc(sizeof(FIL));
  
	uint32_t start_addr = EXFLASH_ADDR_IMG_RESOURCE;
	uint8_t  buffer[CUR_APP_NUM], i;
	uint32_t write_len = IMG_MAX_WIDTH * IMG_MAX_BYTE_PER_LINE;
	uint8_t  resource_info = 0xff;
	uint32_t resource_info_addr = EXFLASH_ADDR_IMG_RESOURCE_INFO;
	uint8_t  resource_internal = 1;
    app_prop_t* p;

	//check flash resources
	ry_hal_spi_flash_read(buffer, resource_info_addr, CUR_APP_NUM);
    LOG_INFO("Flash Addr info: ");
    ry_data_dump(buffer, CUR_APP_NUM, ' ');
    
	for (int i = 0; i < CUR_APP_NUM; i++){
		ry_hal_spi_flash_read(buffer, resource_info_addr, 1);
		if (*buffer != i + 1){
			resource_internal = 0;
			LOG_INFO("idx: %d, info read: 0X%08X, start_addr: 0X%08X \n", i, *(uint32_t *)buffer, i);
			break;
		}
		resource_info_addr ++;
	}

	if (reload){
    	resource_internal = 0;
	}

	start_addr = EXFLASH_ADDR_IMG_RESOURCE;
	resource_info_addr = EXFLASH_ADDR_IMG_RESOURCE_INFO;
	for(i = 0; i < (43200*CUR_APP_NUM + 4095)/4096; i++){
        ry_hal_spi_flash_sector_erase((EXFLASH_ADDR_IMG_RESOURCE_INFO/4096)*4096 + i * 4096);
        if (i%10 == 0) {
            loading_refresh_handle();
        }
	}
    
    bool icon_read_success = false;

	for (int img_idx = 0; img_idx < app_ctx_v.curNum; img_idx++){
	    loading_refresh_handle();
	    ry_hal_wdt_reset();
		if (resource_internal == 0)            
        {
            p = app_prop_getById(app_ctx_v.app_seq[img_idx]);
            if (p == NULL) {
                LOG_ERROR("[app_ui_update] APP not found. appId: %d\r\n", img_idx);
                while(1);
            }
            
            icon_read_success = true;
                
			res = f_open(file, p->icon, FA_READ);
			int pos = 0;
			if (res != FR_OK) {
                
                icon_read_success = false;
                
				LOG_ERROR("[%s]file open failed: %d\n", p->icon, res);
				//goto exit;
				//start_addr += write_len;
        		//resource_info_addr ++;	
				//continue;
			}
            
            // skip draw icon if resx broken / missing
            if (icon_read_success) {
		
                int len = IMG_HEAD_LEN;
                int k = 0;
                res = f_read(file, buff, len, &written_bytes);
#if 0
                for (k = 0; k < len; k++){
                    am_util_debug_printf("0x%02X,", buff[k]);
                    pos ++;
                    if (pos % 16 == 0)
                        am_util_debug_printf("\n\r");
                }
#endif				
                if (buff[0] == 'B' && buff[1] == 'M'){
                    ;//am_util_debug_printf("\n\rgot: %s \n\r", img_external[img_idx]);
                }

                if (buff[18] > RM_OLED_WIDTH){
                    buff[18] = RM_OLED_WIDTH;
                }

                if (buff[22] > RM_OLED_HEIGHT){
                    buff[22] = RM_OLED_HEIGHT;
                }
                
                uint32_t width = buff[18];
                uint32_t height = buff[22];
                LOG_DEBUG("img %s: width %d, img height:%d  \n\r", p->icon, width, height);
                ry_memset(frame_buffer, 0, sizeof(frame_buffer));
                pos = 0;	
                len = width * 3;
                for (int i = 0; i < height; i++){
                    res = f_read(file, buff, len, &written_bytes);
    #if 0		
                    for (k = 0; k < len; k++){
                        am_util_debug_printf("0X%02X,", buff[k]);
                        pos ++;
                        if (pos % 16 == 0)
                            am_util_debug_printf("\n\r");
                    }	
    #endif
                    if (len & 3){
                        uint8_t testBuf_e[4];
                        f_read(file, testBuf_e, (4 - (len & 3)), &written_bytes);
                    }	
                    uint32_t buffer_start = ((height - i) * IMG_MAX_WIDTH + ((IMG_MAX_WIDTH - width) >> 1)) * 3;
                    ry_memcpy(frame_buffer + buffer_start, buff, len);
                }
                f_close(file);
            
            }
			
            gdispFillStringBox( 0, 
                                85, 
                                font_roboto_20.width,
                                font_roboto_20.height,
                                p->name, 
                                (void*)font_roboto_20.font, 
                                White,
                                Black,
                                justifyCenter
                                );
                                
			set_true_color_all_frame_buffer();	
			//p_draw_fb  = frame_buffer;
			//update_buffer_to_oled();

			resource_info = img_idx + 1;
            LOG_DEBUG("Write flash addr: %x, len:%d\r\n", (u32_t)resource_info_addr, 1);
            LOG_DEBUG("Write flash addr: %x, len:%d\r\n\r\n", (u32_t)start_addr, write_len);
			ry_hal_spi_flash_write(&resource_info, resource_info_addr, 1);
			ry_hal_spi_flash_write(frame_buffer, start_addr, write_len);
		}
		else {
			LOG_DEBUG("img %s is already in internal flash\n\r", p->icon);
		}
#if 0	
		p_draw_fb = (uint8_t*)start_addr;
		update_buffer_to_oled();
		ryos_delay_ms(100);		
#endif	
		start_addr += write_len;
		resource_info_addr ++;	
	}
#if 0
    img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE, RM_OLED_IMG_24BIT);
	p_draw_fb = frame_buffer;	
	update_buffer_to_oled();
#endif


exit:
    ry_free(file);
	ry_free(buff);
	
    touch_enable();

    clear_buffer_black();
    
	return res;
}



/**
 * @brief   API to get app property throuth APPID
 *
 * @param   appid - Application ID
 *
 * @return  Property of app
 */
app_prop_t* app_prop_getById(int appid)
{
    int i;
    for (i = 0; i < MAX_APP_NUM; i++) {
        if (appid == app_props[i].appid) {
            return &app_props[i];
        }
    }

    return &app_missing;
}

/**
 * @brief   API to get app property throuth Sequence No
 *
 * @param   appid - Application ID
 *
 * @return  Property of app
 */
int app_prop_getIdBySeq(int seq)
{
    return app_ctx_v.app_seq[seq];
}


ry_sts_t app_ctx_save(void)
{
    ry_sts_t status;

    status = ry_hal_flash_write(FLASH_ADDR_APP_CONTEXT, (u8_t*)&app_ctx_v, sizeof(app_ctx_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_AMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}


ry_sts_t app_ctx_restore(void)
{
    int i;
    ry_hal_flash_read(FLASH_ADDR_APP_CONTEXT, (u8_t*)&app_ctx_v, sizeof(app_ctx_t));

    check_app_ctx();

    if (((u32_t)app_ctx_v.curNum > MAX_APP_NUM) || (app_ctx_v.curNum == 0)) {
        app_ctx_v.curNum = CUR_APP_NUM;

        app_ctx_v.app_seq[0] = APPID_SYSTEM_CARD;
        app_ctx_v.app_seq[1] = APPID_SYSTEM_DATA;
        app_ctx_v.app_seq[2] = APPID_SYSTEM_HRM;
        app_ctx_v.app_seq[3] = APPID_SYSTEM_SPORT;
        app_ctx_v.app_seq[4] = APPID_SYSTEM_WEATHER;
        app_ctx_v.app_seq[5] = APPID_SYSTEM_ALARM;
        app_ctx_v.app_seq[6] = APPID_SYSTEM_MIJIA;
        app_ctx_v.app_seq[7] = APPID_SYSTEM_SETTING;

        #if (CUR_APP_NUM >= 9)
        app_ctx_v.app_seq[8] = APPID_SYSTEM_ALIPAY;
        #endif
        #if (CUR_APP_NUM >= 10)
        app_ctx_v.app_seq[9] = APPID_SYSTEM_TIMER;
        #endif        
    }

    return RY_SUCC;
}

void app_sequence_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    } else {
        LOG_DEBUG("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    }
}



/**
 * @brief   API to set application sequence
 *
 * @param   None
 *
 * @return  None
 */
void app_sequence_set(u8_t* data, int len)
{
    DevAppList msg;
    int status;
    ry_sts_t ry_sts;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, DevAppList_fields, &msg);

    LOG_DEBUG("[app_sequence_set] status: %d\n", msg.dev_app_infos_count);
    for (int i = 0; i < msg.dev_app_infos_count; i++) {
        LOG_DEBUG("[app_sequence_set] appid: %s\n", msg.dev_app_infos[i].id);
        app_ctx_v.app_seq[i] = msg.dev_app_infos[i].id;
    }

    /* Save to flash */
    app_ctx_save();

    _start_func_oled(1);
    extern ry_gui_ctx_t    ry_gui_ctx_v;
    ry_gui_ctx_v.curState = GUI_STATE_ON;
    create_loading(0);
    /* Update UI */
    app_ui_update(1);

    distory_loading();
    wms_activity_back_to_root(NULL);
    extern activity_t activity_menu;
    wms_activity_jump(&activity_menu, NULL);


    /* Send response */
    //rbp_send_resp(CMD_DEV_APP_SET_LIST, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_sts = ble_send_response(CMD_DEV_APP_SET_LIST, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                app_sequence_ble_send_callback, (void*)__FUNCTION__);

}


/**
 * @brief   API to get current application sequence
 *
 * @param   None
 *
 * @return  None
 */
void app_sequence_get(u8_t* data, int len)
{
    int i;
    DevAppList * result = (DevAppList *)ry_malloc(sizeof(DevAppList));
    if (!result) {
        LOG_ERROR("[app_sequence_get] No mem.\r\n");
        while(1);
    }

    check_app_ctx();
    /* Encode result */
    result->dev_app_infos_count = app_ctx_v.curNum;
    for (i = 0; i < result->dev_app_infos_count; i++) {
        result->dev_app_infos[i].id = app_ctx_v.app_seq[i];
    }

    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(DevAppList) + 10);
    if (!data_buf) {
        LOG_ERROR("[app_sequence_get] No mem.\r\n");
        while(1);
    }
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(DevAppList) + 10);

    status = pb_encode(&stream, DevAppList_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[app_sequence_get]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    //status = rbp_send_resp(CMD_DEV_APP_SET_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1);

    status = ble_send_response(CMD_DEV_APP_GET_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                app_sequence_ble_send_callback, (void*)__FUNCTION__);

exit:
    ry_free(data_buf);
    ry_free((u8_t*)result);
}


/**
 * @brief   API to init APP manager service
 *
 * @param   None
 *
 * @return  None
 */
void app_service_init(void)
{
    // Restore the application info from flash/exflash
    app_ctx_restore();
                                 
#if (RY_BUILD == RY_RELEASE)
    if (dev_mfg_state_get() >= DEV_MFG_STATE_FINAL) {
	    app_ui_update(1);
    }
#endif
}
