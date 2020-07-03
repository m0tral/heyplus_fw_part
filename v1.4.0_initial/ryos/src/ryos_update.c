#include "ryos_update.h"
#include "ry_hal_flash.h"
#include "ryos.h"
#include "ryos_protocol.h"
#include "firmware.pb.h"
#include "device.pb.h"
#include "rbp.h"
#include "board.h"
#include "ryos_timer.h"
#include "ry_hal_spi_flash.h"
#include "ff.h"
#include "r_xfer.h"
#include "dip.h"
#include "device_management_service.h"
#include "data_management_service.h"
#include "gui_bare.h"
#include "gui.h"
#include "ry_hal_wdt.h"
#include "ry_hal_rtc.h"
#include "ble_task.h"
#include "app_pm.h"
#include "crc32.h"
#include "mible.h"
#include "gui_img.h"
#include "ry_gui_disp.h"
#include "ry_font.h"
#include "storage_management_service.h"


#define OTA_IDLE_TIMEOUT               1000//30000
#define OTA_IDLE_TIMEOUT_COUNT          30

#define PERCENT_START_Y                 185

typedef struct ry_update_ctx{
    update_display_mode_e display_mode;
    u8_t resource_update_state;
    u16_t mcuFw_crc;
    u32_t pack_recved_size;
    u32_t update_state;
    char new_version[50];
}ry_update_ctx_t;


//u32_t update_state = UPDATE_IDLE;

ry_systemFileHead_t update_info;
u32_t recv_size = 0;

//uint32_t FW_pack_size = 0;

FwUpdateItem  updateItem[10];

FwUpdateItem * mcuFw_item = NULL;
u8_t mcuFw_num = 0;
u32_t recved_num = 0;
//char new_version[50];
//u16_t mcuFw_crc = 0;

//u32_t pack_recved_size = 0;
u32_t pack_total_size = 0;
u8_t update_boot_flag = 0;
u8_t update_pack_num = 0;
static uint32_t current_read_size = 0;

int ota_idle_timer = 0;
u8_t ota_idle_count = 0;


ry_update_ctx_t update_ctx = {0};


extern void gdisp_update(void);


u32_t reset_timer = 0;

#define test_boot_addr		0x60000

u32_t get_cur_update_status(void)
{
    return update_ctx.update_state;
}


void ota_idle_timeout_handler(void* arg)
{
    ota_idle_count++;

    if(ota_idle_count >= OTA_IDLE_TIMEOUT_COUNT){
        if(update_ctx.update_state > UPDATE_IDLE){
            LOG_ERROR("OTA idle timeout.\n");
            resume_other_task_for_update_end();

        }

        if (ota_idle_timer) {
            ry_timer_stop(ota_idle_timer);
            ry_timer_delete(ota_idle_timer);
            ota_idle_timer = 0;
            ota_idle_count = 0;
        }
    }else{
        LOG_ERROR("[%s]----%d\n", __FUNCTION__, ota_idle_count);
        ry_hal_wdt_reset();
        ry_timer_start(ota_idle_timer, OTA_IDLE_TIMEOUT, ota_idle_timeout_handler, NULL);
    }
}   



void ota_idle_timer_start(void)
{
    ry_timer_parm timer_para;
    LOG_WARN("[%s] \r\n", __FUNCTION__);
    if (ota_idle_timer == 0) {
        /* Create the timer */
        timer_para.timeout_CB = ota_idle_timeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "ota_idle_timer";
        ota_idle_timer = ry_timer_create(&timer_para);

        if (ota_idle_timer) {
            ry_timer_start(ota_idle_timer, OTA_IDLE_TIMEOUT, ota_idle_timeout_handler, NULL);
        }
    }else{
        ry_timer_stop(reset_timer);
        ry_timer_start(ota_idle_timer, OTA_IDLE_TIMEOUT, ota_idle_timeout_handler, NULL);
    }
    ota_idle_count = 0;
}


void ota_idle_timer_stop(void)
{
    LOG_WARN("[%s] \r\n", __FUNCTION__);
    if (ota_idle_timer) {
        ry_timer_stop(ota_idle_timer);
        ry_timer_delete(ota_idle_timer);
        ota_idle_timer = 0;
    }
}



void ryos_update_onConnected(void)
{
    if (update_ctx.update_state > UPDATE_IDLE) {
        ota_idle_timer_stop();
    }
}



void update_timeout_handle(void* arg)
{
    if (update_ctx.update_state > UPDATE_IDLE) {
        ota_idle_timer_start();
    }
}

void suspend_other_task_for_update_start(void)
{
    extern void stop_unlock_alert_animate(void);
    
    //ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
    ryos_delay_ms(100);
    stop_unlock_alert_animate(); 
    ry_sched_msg_send(IPC_MSG_TYPE_UPGRADE_START, 0, NULL);
	ryos_delay_ms(100);
	
    ry_hal_rtc_int_disable();
    extern ryos_thread_t *gui_thread3_handle,*sensor_alg_thread_handle, *tms_thread_handle;
    extern ryos_thread_t *wms_thread_handle, *ry_nfc_thread_handle;
    ryos_thread_suspend(gui_thread3_handle);
    ryos_thread_suspend(sensor_alg_thread_handle);
    ryos_thread_suspend(tms_thread_handle);
    ryos_thread_suspend(wms_thread_handle);
    ryos_thread_suspend(ry_nfc_thread_handle);
    pcf_func_gsensor();
    
}

void resume_other_task_for_update_end(void)
{

    ry_sched_msg_send(IPC_MSG_TYPE_UPGRADE_FAIL, 0, NULL);		
    ry_hal_rtc_int_enable();
    extern ryos_thread_t *gui_thread3_handle,*sensor_alg_thread_handle,*tms_thread_handle;
    extern ryos_thread_t *wms_thread_handle,*ry_nfc_thread_handle;
    _start_func_gsensor();
    ryos_thread_resume(gui_thread3_handle);
    ryos_thread_resume(sensor_alg_thread_handle);
    ryos_thread_resume(tms_thread_handle);
    ryos_thread_resume(wms_thread_handle);
    ryos_thread_resume(ry_nfc_thread_handle);
    update_ctx.update_state = UPDATE_IDLE;
    if(reset_timer != 0){
        ry_timer_stop(reset_timer);
    }
    set_device_session(DEV_SESSION_IDLE);
}

#define UI_SYS_IMG_X                                    24
#define UI_SYS_IMG_Y                                    54

#define UI_SYS_TITLE_X                                    0
#define UI_SYS_TITLE_Y                                    176

const char* const text_update_uploading = "Загрузка..";
const char* const text_update_complete = "Выполнена";
const char* const text_update_rebooting = "Перезапуск";

void update_percent_display(int cur, int total, update_display_mode_e display_mode)
{
    u8_t w, h;

    if(display_mode == UPDATE_DISPLAY_MODE_ALL){
    
        clear_buffer_black();
        img_exflash_to_framebuffer((u8_t*)"m_upgrade_01_doing.bmp", UI_SYS_IMG_X, UI_SYS_IMG_Y, &w, &h, d_justify_left);
        
        if(total != 0){
            if(cur != total){
                gdispFillStringBox(UI_SYS_TITLE_X, 156,
                                    font_roboto_20.width, font_roboto_20.height,
                                    (char*)text_update_uploading,
                                    (void*)font_roboto_20.font,
                                    White, Black|(1<<30), justifyCenter);
                //clear_frame(PERCENT_START_Y - 1,239 - PERCENT_START_Y + 2);
                ry_gui_disp_percent(NULL, 22, 22, 60, PERCENT_START_Y, White, cur*100/total);
                //gdisp_update();
                //ry_gui_refresh_area_y(PERCENT_START_Y - 1, 239);
            }else{
                //clear_frame(156, 239 - 156 + 1);
                gdispFillStringBox( 0, 
                                    156, 
                                    font_roboto_20.width, font_roboto_20.height,
                                    (char*)text_update_complete, 
                                    (void*)font_roboto_20.font, 
                                    White,  
                                    Black,
                                    justifyCenter
                                    ); 

                gdispFillStringBox( 0, 
                                    PERCENT_START_Y, 
                                    font_roboto_20.width, font_roboto_20.height,
                                    (char*)text_update_rebooting, 
                                    (void*)font_roboto_20.font, 
                                    White,  
                                    Black,
                                    justifyCenter
                                    ); 
                //ry_gui_refresh_area_y(156, 239);
            }
        }

        gdisp_update();
    }else{
        if(total != 0){
            if(cur != total){
                //gdispFillStringBox(UI_SYS_TITLE_X, 156,  0, 22, "升级中 ", 0, White,  Black|(1<<30),justifyCenter);
                clear_frame(PERCENT_START_Y - 1,239 - PERCENT_START_Y + 2);
                ry_gui_disp_percent(NULL, 22, 22, 60, PERCENT_START_Y, White, cur*100/total);
                ry_gui_refresh_area_y(PERCENT_START_Y - 1, 239);
            }else{
                clear_frame(156, 239 - 156 + 1);
                gdispFillStringBox( 0, 
                                    156, 
                                    font_roboto_20.width, font_roboto_20.height,
                                    (char*)text_update_complete, 
                                    (void*)font_roboto_20.font, 
                                    White,  
                                    Black,
                                    justifyCenter
                                    ); 

                gdispFillStringBox( 0, 
                                    PERCENT_START_Y, 
                                    font_roboto_20.width, font_roboto_20.height,
                                    (char*)text_update_rebooting,
                                    (void*)font_roboto_20.font, 
                                    White,  
                                    Black,
                                    justifyCenter
                                    ); 
                ry_gui_refresh_area_y(156, 239);
            }
        }
    }
}


ry_sts_t update_start(RbpMsg *msg)
{
	int i = 0;
	FwUpdateInfo*  info = (FwUpdateInfo * )ry_malloc(sizeof(FwUpdateInfo));
	size_t message_length;
	u8_t * data_buf = NULL;
    u8_t phoneType;
    int  connInterval = 0;	
	
	ry_sts_t status = RY_SUCC;

	uint32_t resp_code = RBP_RESP_CODE_SUCCESS;
	
	FwUpdateItem  Item /*= (FwUpdateItem *)tb_item->buf*/;
	pb_istream_t stream = {0};
	pb_ostream_t out_stream = {0};

	suspend_other_task_for_update_start();

    stream = pb_istream_from_buffer(msg->message.req.val.bytes, msg->message.req.val.size);

    if (0 == pb_decode(&stream, FwUpdateInfo_fields, info)){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        resp_code = RBP_RESP_CODE_DECODE_FAIL;
        rbp_send_resp(CMD_DEV_FW_UPDATE_START, resp_code, NULL, 0, 0);
        return status;
    }
    update_ctx.display_mode = UPDATE_DISPLAY_MODE_ALL;
    if(strcmp(update_ctx.new_version, info->version) == 0){
        FwUpdateStartResult start_ret = {0};
        start_ret.transfered_length = update_ctx.pack_recved_size;

        if(update_ctx.pack_recved_size == 0){
            resource_file_update_status_clear();
        }
        
        data_buf = (u8_t *)ry_malloc(sizeof(FwUpdateStartResult) +10);

        out_stream = pb_ostream_from_buffer(data_buf, (sizeof(FwUpdateStartResult) +10));
        pb_encode(&out_stream, FwUpdateStartResult_fields, &start_ret);
        LOG_ERROR("pack_recved_size == %d \n", update_ctx.pack_recved_size);
    }else{
        pack_total_size = 0;
        strcpy(update_ctx.new_version, info->version);
        mcuFw_num = NO_FIRMWAVE_FILE;
    	for(i = 0;i <  info->items_count; i++){

    		if(info->items[i].fw_id == UPDATE_MCU){
    		    mcuFw_item = &(info->items[i]);
    		    mcuFw_num = i;
                int j = 0;
                for(j = 0 ; j < FIRMWAVE_FILE_MAX_SECTOR_NUM ; j++){
                    ry_hal_wdt_reset();
    		        ry_hal_spi_flash_sector_erase(FIRMWARE_STORE_ADDR + j * EXFLASH_SECTOR_SIZE);
    		    }
    		} else if(info->items[i].fw_id == BOOTLOADER){
    		    LOG_DEBUG("unlink boot file\n");
                f_unlink("boot.bin");
    		} else if(info->items[i].fw_id == UPDATE_RES){

    		    if(info->items[i].length >= 4096 * 1024){
        		    LOG_DEBUG("mkfs\n");
                    u8_t *work = (u8_t *)ry_malloc(4096 *2);
        		    f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096 *2);
                    ry_free(work);

                    update_ctx.display_mode = UPDATE_DISPLAY_MODE_HALF;
                }
                
    		}
            pack_total_size += info->items[i].length;
    	}
    	memcpy(updateItem, info->items, (sizeof(FwUpdateItem)*(info->items_count)));
        update_pack_num = (info->items_count);
    	/*if (mcuFw_item == NULL) {
            resp_code = 22;
    	}*/

    	recved_num = 0;
    	update_ctx.mcuFw_crc = 0xFFFF;
    	update_ctx.pack_recved_size = 0;
    	current_read_size = 0;
	}

	update_ctx.update_state = UPDATE_START;

    if (info->has_ota_conn_interval) {
        connInterval = info->ota_conn_interval;
    }

    ry_free(info);


    uint8_t conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
//    phoneType = ry_ble_get_peer_phone_type();
//    switch (phoneType) {
//        case PEER_PHONE_TYPE_ANDROID:
//            if (connInterval == 0) {
//                connInterval = 6;
//            }
//            ble_updateConnPara(6, connInterval, 500, 0);
//            break;

//        case PEER_PHONE_TYPE_IOS:
//            ble_updateConnPara(6, 24, 500, 0);
//            break;

//        default:
//            ble_updateConnPara(6, 24, 500, 0);
//            break;
//    }


//#warning todo: make update commands unsecured for firmware restore from unbinded state
	status = rbp_send_resp(CMD_DEV_FW_UPDATE_START, resp_code, data_buf, out_stream.bytes_written, 0);
	ry_free(data_buf);

//----add by lixueliang 5-27
    ry_hal_wdt_reset();
	if(reset_timer == 0){
        
        ry_timer_parm timer_para;
        timer_para.timeout_CB = update_timeout_handle;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "update";
        reset_timer = ry_timer_create(&timer_para);


        //ry_timer_start(reset_timer, 20000, ry_system_reset, NULL);

        //reset_timer = ry_timer_create(0);
        if (reset_timer == RY_ERR_TIMER_ID_INVALID) {
            LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
            add_reset_count(UPGRADE_RESTART_COUNT);
            ry_system_reset();
        }
    }
    ry_timer_stop(reset_timer);
    ry_timer_start(reset_timer, 10000, update_timeout_handle, NULL);
//----end

	return status;
}


void update_stop(void)
{
    LOG_DEBUG("%s\r\n", __FUNCTION__);

    
    ble_send_response(CMD_DEV_FW_UPDATE_STOP, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, NULL, NULL);
    
    
    if(update_ctx.update_state > UPDATE_IDLE){
        LOG_ERROR("OTA idle timeout.\n");
        ry_sched_msg_send(IPC_MSG_TYPE_UPGRADE_FAIL, 0, NULL);		
        ry_hal_rtc_int_enable();
        extern ryos_thread_t *gui_thread3_handle,*sensor_alg_thread_handle,*tms_thread_handle, *wms_thread_handle;
        _start_func_gsensor();
        ryos_thread_resume(gui_thread3_handle);
        ryos_thread_resume(sensor_alg_thread_handle);
        ryos_thread_resume(tms_thread_handle);
        ryos_thread_resume(wms_thread_handle);
        update_ctx.update_state = UPDATE_IDLE;
        if(reset_timer != 0){
            ry_timer_stop(reset_timer);
        }
        set_device_session(DEV_SESSION_IDLE);
    }

    if (ota_idle_timer) {
        ry_timer_stop(ota_idle_timer);
        ry_timer_delete(ota_idle_timer);
        ota_idle_timer = 0;
    }
}


ry_sts_t update_handle(RbpMsg *msg)
{
	ry_sts_t status = RY_SUCC;

	int write_status = 0;
	msg_param_t * tb;

	am_bootloader_image_t boot_info = {0};

    u32_t sn = msg->message.req.sn;
    u32_t total = msg->message.req.total;

    update_ctx.pack_recved_size += msg->message.req.val.size;
    update_ctx.update_state = UPDATING;
    ry_hal_wdt_reset();
    if(reset_timer == 0){
        
        ry_timer_parm timer_para;
        timer_para.timeout_CB = update_timeout_handle;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "update";
        reset_timer = ry_timer_create(&timer_para);


        //ry_timer_start(reset_timer, 20000, ry_system_reset, NULL);

        //reset_timer = ry_timer_create(0);
        if (reset_timer == RY_ERR_TIMER_ID_INVALID) {
            LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
            add_reset_count(UPGRADE_RESTART_COUNT);
            ry_system_reset();
        }
    }
    ry_timer_stop(reset_timer);
    ry_timer_start(reset_timer, 10000, update_timeout_handle, NULL);


    update_percent_display(update_ctx.pack_recved_size, pack_total_size, update_ctx.display_mode);

    if(ry_timer_isRunning(ota_idle_timer)){
        ry_timer_stop(ota_idle_timer);
    }    

    if(recved_num != mcuFw_num){

        if(updateItem[recved_num].fw_id == UPDATE_RES){
            restore_file(msg->message.req.val.bytes, msg->message.req.val.size);
        }else if(updateItem[recved_num].fw_id == BOOTLOADER){
            get_boot_bin_data(msg->message.req.val.bytes, msg->message.req.val.size);
            update_boot_flag = 1;
        }
           
        if ((total - 1) == (sn)){

            update_ctx.update_state = UPDATE_IDLE;
            if(updateItem[recved_num].fw_id == UPDATE_RES){
                //check_fs_header(FS_CHECK_UPDATE);

            }
            recved_num ++;
            if(recved_num == update_pack_num){
                system_app_data_save();
                set_fw_ver((u8_t*)(update_ctx.new_version));
                LOG_DEBUG("resource update succ \n");
                add_reset_count(UPGRADE_RESTART_COUNT);
                ry_timer_start(reset_timer, 5000, (ry_timer_cb_t)ry_system_reset, NULL);
            }
            rbp_send_resp(CMD_DEV_FW_UPDATE_FILE, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);
        }
	
        return status;
    }

	//static uint32_t current_read_size = 0;

    LOG_DEBUG("bytes : %x\n", msg->message.req.val.bytes);
    LOG_DEBUG("addr : %x\n", FIRMWARE_STORE_ADDR + current_read_size);
    LOG_DEBUG("size : %x\n", msg->message.req.val.size);

    if(current_read_size >= (FIRMWAVE_FILE_MAX_SECTOR_NUM - 3) * EXFLASH_SECTOR_SIZE){
        LOG_ERROR("FW size error : %d\n", current_read_size);
        rbp_send_resp(CMD_DEV_FW_UPDATE_FILE, RBP_RESP_CODE_INVALID_PARA, NULL, 0, 0);
        return RY_SET_STS_VAL(RY_CID_OTA, RY_ERR_WRITE);;
    }

	write_status = ry_hal_spi_flash_write(msg->message.req.val.bytes, FIRMWARE_STORE_ADDR + current_read_size, msg->message.req.val.size);
	if(write_status != 0){
		LOG_ERROR("flash write fail : %d\n", write_status);
		rbp_send_resp(CMD_DEV_FW_UPDATE_FILE, RBP_RESP_CODE_INVALID_PARA, NULL, 0, 0);
		
		add_reset_count(UPGRADE_RESTART_COUNT);
        ry_timer_start(reset_timer, 5000, (ry_timer_cb_t)ry_system_reset, NULL);
        goto exit;
	
	}

	
    if((sn == 0) &&(update_ctx.mcuFw_crc == 0xffff)){
        update_ctx.mcuFw_crc = calc_crc16_r(&msg->message.req.val.bytes[FIRMWAVE_HEAD_SIZE], msg->message.req.val.size - FIRMWAVE_HEAD_SIZE, 0);
    }else{
    
	    update_ctx.mcuFw_crc = calc_crc16_r(msg->message.req.val.bytes, msg->message.req.val.size, update_ctx.mcuFw_crc);
	}

	current_read_size += msg->message.req.val.size;


	if (sn == (total - 1)) {
	    update_ctx.update_state = UPDATE_IDLE;
		boot_info.pui32LinkAddress = (uint32_t *)0x0000A000;
		boot_info.ui32NumBytes = current_read_size;
		boot_info.ui32UpdateFlag = 1;
		boot_info.ui32FirmwareStoreAddr = test_boot_addr;

        current_read_size = 0;

        ry_systemFileHead_t fileHead = {0};
        ry_hal_spi_flash_read((u8_t *)&fileHead, FIRMWARE_STORE_ADDR, sizeof(fileHead) );
        
        if(update_ctx.mcuFw_crc != fileHead.crc){
            LOG_ERROR("\n\n\ncrc check fail\n\n\n");
            add_reset_count(UPGRADE_RESTART_COUNT);
            ry_timer_start(reset_timer, 5000, (ry_timer_cb_t)ry_system_reset, NULL);
            goto exit;
        }


        int i = 0;
		u8_t *temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
		update_ctx.mcuFw_crc = 0;
		u32_t data_addr = FIRMWARE_STORE_ADDR + sizeof(fileHead);
		if(temp_buf != NULL){
			while(i < fileHead.file_size){
				if(fileHead.file_size - i > EXFLASH_SECTOR_SIZE){
					ry_hal_spi_flash_read(temp_buf, data_addr + i, EXFLASH_SECTOR_SIZE );
					i+=EXFLASH_SECTOR_SIZE;
					update_ctx.mcuFw_crc = calc_crc16_r(temp_buf, EXFLASH_SECTOR_SIZE, update_ctx.mcuFw_crc);
				}else{
					ry_hal_spi_flash_read(temp_buf, data_addr + i, fileHead.file_size%EXFLASH_SECTOR_SIZE );
					update_ctx.mcuFw_crc = calc_crc16_r(temp_buf, fileHead.file_size%EXFLASH_SECTOR_SIZE, update_ctx.mcuFw_crc);
					break;
				}
				
			}

			if(update_ctx.mcuFw_crc != fileHead.crc){
	            LOG_ERROR("\n\n\ncrc check fail\n\n\n");
	            add_reset_count(UPGRADE_RESTART_COUNT);
	            ry_timer_start(reset_timer, 5000, (ry_timer_cb_t)ry_system_reset, NULL);
	            goto exit;
	        }
		}
		ry_free(temp_buf);
		temp_buf = NULL;
        
		ry_hal_flash_write(SYSTEM_STATUS_INFO_ADDR, (u8_t*)&boot_info, sizeof(am_bootloader_image_t));
		set_fw_ver((u8_t*)(update_ctx.new_version));

		
        if (reset_timer == RY_ERR_TIMER_ID_INVALID) {
            LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
        }

        //update boot
        if(update_boot_flag){
            update_boot_file();
        }
        save_fs_header();
        system_app_data_save();
        LOG_DEBUG("firmware update succ \n");
        add_reset_count(UPGRADE_RESTART_COUNT);
        ry_timer_start(reset_timer, 5000, (ry_timer_cb_t)ry_system_reset, NULL);
	}


    //if (r_xfer_getLastRxDuration(R_XFER_INST_ID_BLE_SEC) > 800) {
    //    ble_updateConnPara(6, 12, 500, 0);
    //}
    
	LOG_DEBUG("--------%d-------%d\n",msg->message.req.total, msg->message.req.sn);
    LOG_DEBUG("Free heap %d\n", ryos_get_free_heap());
    

exit:


	if ((total - 1) == (sn)){
        recved_num ++;
        rbp_send_resp(CMD_DEV_FW_UPDATE_FILE, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);
    }
	
	return status;
}


res_file_head resHead = {0};
FIL * cur_fp = NULL;

void resource_file_update_status_clear(void)
{
    ry_memset(&resHead, 0, sizeof(resHead));
    update_ctx.resource_update_state = RESOURCE_FILE_HEAD;
    FREE_PTR(cur_fp);
    recv_size = 0;
}

void restore_file(u8_t * data, int len)
{
    u8_t * t_data = data;
    int t_len = len;

    //static int state = RESOURCE_FILE_HEAD;

    //update_ctx.resource_update_state = RESOURCE_FILE_HEAD;
    u32_t written_bytes;
    FRESULT res ;

    static char * fileName = NULL;
    while(t_len > 0){
        switch(update_ctx.resource_update_state){
            case RESOURCE_FILE_HEAD:
                {
                    if(t_len < sizeof(res_file_head)){
                        ry_memcpy(&resHead, t_data, t_len);
                        t_data += t_len;
                        
                        recv_size = t_len;
                        t_len = 0;

                    }else{
                        if(recv_size == 0){
                            ry_memcpy(&resHead, t_data, sizeof(res_file_head));
                            if(resHead.magic != FILE_START_MAGIC){
                                t_data += 1;
                                t_len -= 1;
                                continue;
                            }
                            
                            t_data = &t_data[sizeof(res_file_head)];
                            t_len = t_len - sizeof(res_file_head);
                        }else{
                            u8_t * t_head = (u8_t*)&resHead;
                            ry_memcpy(&t_head[recv_size], t_data, sizeof(res_file_head) - recv_size);
                            t_data = &t_data[(sizeof(res_file_head) - recv_size)];
                            t_len = t_len - (sizeof(res_file_head) - recv_size);

                        }

                        recv_size = 0;
                        update_ctx.resource_update_state = RESOURCE_FILE_NAME;
                    }
                }
                break;

            case RESOURCE_FILE_NAME:
                {
                    if(t_len < resHead.name_len){
                        if(resHead.name_len > MAX_FILE_NAME_LEN){
                            LOG_ERROR("file name length error\n");
                            goto error;
                        }
                        fileName = ry_malloc(resHead.name_len);
                        if(fileName == NULL){
                            LOG_ERROR("fileName malloc %d failed\n", resHead.name_len);
                        }
                        ry_memcpy(fileName, t_data, t_len);
                        recv_size = t_len;
                        t_len = 0;
                    }else{

                        if(recv_size  == 0){
                            if(resHead.name_len > MAX_FILE_NAME_LEN){
                                LOG_ERROR("file name length error\n");
                                goto error;
                            }
                            fileName = ry_malloc(resHead.name_len);
                            if(fileName == NULL){
                                LOG_ERROR("fileName malloc %d failed\n", resHead.name_len);
                            }
                            ry_memcpy(fileName, t_data, resHead.name_len);
                            
                            t_data = &t_data[resHead.name_len];
                            t_len = t_len - resHead.name_len;
                            
                            /*if(resHead.file_len > 0){
                                cur_fp = ry_malloc(sizeof(FIL));
                                if(cur_fp == NULL){
                                    LOG_ERROR("cur_fp malloc %d failed\n", sizeof(FIL));
                                }
                                res = f_open(cur_fp, fileName, FA_CREATE_ALWAYS | FA_WRITE);
                                if(res != FR_OK){
                                    LOG_ERROR("file->%s open error\n", fileName);
                                }

                                recved_num = 0;
                                state = 2;
                                
                            }else{//delete file
                                f_unlink(fileName);
                                recved_num = 0;
                                state = 0;
                            }*/
                        }else{
                            ry_memcpy(&fileName[recv_size], t_data, (resHead.name_len - recv_size));
                            t_data = &t_data[(resHead.name_len - recv_size)];
                            t_len = t_len - (resHead.name_len - recv_size);
                        }

                        if(resHead.file_len > 0){
                            if(cur_fp != NULL){
                                ry_free(cur_fp);
                            }
                            cur_fp = ry_malloc(sizeof(FIL));
                            if(cur_fp == NULL){
                                LOG_ERROR("cur_fp malloc %d failed\n", sizeof(FIL));
                            }
                            LOG_DEBUG("{%s}--start\n", fileName);

                            u32_t free_size = 0;
                            extern FATFS *ry_system_fs;
                            f_getfree("",(DWORD*)&free_size,&ry_system_fs);
                            f_unlink(fileName);
                            LOG_DEBUG("disk size : %d \n", free_size);
                            res = f_open(cur_fp, fileName, FA_CREATE_ALWAYS | FA_WRITE);
                            if(res != FR_OK){
                                LOG_ERROR("file->%s open error, code = 0x%d\n", fileName, res);
                            }

                            recv_size = 0;
                            update_ctx.resource_update_state = RESOURCE_FILE_DATE;
                            
                        }else{//delete file
                            LOG_DEBUG("{%s}--unlink\n", fileName);
                            f_unlink(fileName);
                            recv_size = 0;
                            update_ctx.resource_update_state = RESOURCE_FILE_HEAD;
                        }                       
                    }
                }
                break;

            case RESOURCE_FILE_DATE:
                {
                    if(t_len < resHead.file_len - recv_size){//continue
                        
                        res = f_write(cur_fp, t_data, t_len, &written_bytes);
                        if(res != FR_OK){
                             LOG_ERROR("file->%s write error, code = 0x%x\n", fileName, res);
                        }

                        
                        recv_size += t_len;
                        t_len = 0;
                        
                        
                    }else{//finish
                        res = f_write(cur_fp, t_data, (resHead.file_len - recv_size), &written_bytes);
                        if(res != FR_OK){
                             LOG_ERROR("file->%s write error, code = 0x%x\n", fileName, res);
                        }
                        t_len = t_len - (resHead.file_len - recv_size);
                        t_data = &t_data[(resHead.file_len - recv_size)];

                        f_close(cur_fp);
                        ry_free(fileName);
                        ry_free(cur_fp);
                        cur_fp = NULL;
                        recv_size = 0;
                        update_ctx.resource_update_state = RESOURCE_FILE_HEAD;                        
                    }
                }
								break;            
        }
    }
		
		return;

error:
    ble_send_response(CMD_DEV_FW_UPDATE_FILE,  RBP_RESP_CODE_ENCODE_FAIL, NULL, 0, 1, NULL, NULL);    
}


void get_boot_bin_data(u8_t * data, u32_t len)
{
    FIL * fp = NULL;
    FRESULT res ;
    u32_t written_bytes;
    u32_t file_size = 0, i = 0;
    u32_t boot_start_addr = 0;

    LOG_DEBUG("\n\n[%s]\n\n", __FUNCTION__);

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] - malloc failed \n", __FUNCTION__);
        goto exit;
    }

    res = f_open(fp, "boot.bin", FA_OPEN_APPEND | FA_WRITE);
    if(res != FR_OK){
        LOG_ERROR("[%s] - boot file open fail\n", __FUNCTION__);
        goto exit;
    }

    //res = f_lseek()

    res = f_write(fp, data, len, &written_bytes);
    if(res != FR_OK){
        LOG_ERROR("[%s] - boot file write fail\n", __FUNCTION__);
        goto exit;
    }

    LOG_DEBUG("[%s] boot file is %d\n",__FUNCTION__, f_size(fp));

exit:
    f_close(fp);

    ry_free(fp);

}

void update_boot_file(void)
{
    FIL * fp = NULL;
    FRESULT res ;
    u32_t written_bytes;
    u32_t file_size = 0, i = 0;
    u32_t boot_start_addr = 0;
    u8_t * buf = NULL;

    LOG_DEBUG("\n\n[%s]\n\n", __FUNCTION__);

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] - malloc failed \n", __FUNCTION__);
        goto exit;
    }

    res = f_open(fp, "boot.bin", FA_READ);
    if(res != FR_OK){
        LOG_ERROR("[%s] - boot file open fail\n", __FUNCTION__);
        goto exit;
    }
    file_size = f_size(fp);
    LOG_DEBUG("boot file size: %d\n", file_size);
    if((file_size < 8 *1024) || (file_size > 24 * 1024)){
        
        LOG_ERROR("[%s] - boot file is error\n", __FUNCTION__);
        goto exit;
    }

    buf = (u8_t *)ry_malloc(RY_FLASH_SIZEOF_ROW);
    if(buf == NULL){
        LOG_ERROR("[%s] - malloc failed \n", __FUNCTION__);
        goto exit;
    }

    for(i = 0; i <= file_size/RY_FLASH_SIZEOF_ROW; i++){

        

        res = f_read(fp, buf, RY_FLASH_SIZEOF_ROW, &written_bytes);
        if(res != FR_OK){
            LOG_ERROR("[%s] - boot file read fail\n", __FUNCTION__);
        }

        if(written_bytes == 0){
            LOG_ERROR("BOOT read failed\n");
            break;
        }

        //lxl : not safety
        am_bootloader_program_flash_page(boot_start_addr + (i*RY_FLASH_SIZEOF_ROW), (u32_t *)buf, written_bytes);

    }

exit:
    f_close(fp);
    ry_free(fp);
    ry_free(buf);
    
    f_unlink("boot.bin");
}



#define FILE_TRANSFAR_MAGIC         "12344321"
u8_t * file_name_copy;
ry_sts_t download_resource_file(RbpMsg *msg)
{
    //UploadFileParam * file_param = (UploadFileParam *)ry_malloc(sizeof(UploadFileParam));
    u32_t sn = msg->message.req.sn;
    u32_t total = msg->message.req.total;
    u32_t written_bytes;
    FRESULT res ;
    u8_t image_width, image_height;
    
    static FIL* fp = NULL;
    #if 0
    UploadFileMeta *fileMeta = (UploadFileMeta *)ry_malloc(sizeof(UploadFileMeta));

    u8_t *ptr = &(msg->message.req.val.bytes[0]);
    u32_t i;

    /*for(i = 0; i < msg->message.req.val.size; i++){
        if(msg->message.req.val.bytes[i] == '1'){
            if( ry_memcmp(&(msg->message.req.val.bytes[i]), "12344321", strlen(12344321)) == 0 ){
                break;
            }
        }
    }*/

    u8_t * offset_p = strstr(ptr, "12344321");
    u32_t offset = offset_p - ptr;
    u8_t * data_buf = (u8_t *)ry_malloc(offset+ 20);
    ry_memcpy(data_buf, ptr, offset);

    
    pb_istream_t stream = pb_istream_from_buffer(data_buf, offset);
    if (0 == pb_decode(&stream, UploadFileMeta_fields, fileMeta)) {
        LOG_ERROR("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        goto exit;
    }
    #endif

    if(sn == 0){

        UploadFileMeta *fileMeta = (UploadFileMeta *)ry_malloc(sizeof(UploadFileMeta));

        u8_t *ptr = &(msg->message.req.val.bytes[0]);
        u32_t i;

        /*for(i = 0; i < msg->message.req.val.size; i++){
            if(msg->message.req.val.bytes[i] == '1'){
                if( ry_memcmp(&(msg->message.req.val.bytes[i]), "12344321", strlen(12344321)) == 0 ){
                    break;
                }
            }
        }*/

        u8_t * offset_p = (u8_t*)strstr((const char*)ptr, FILE_TRANSFAR_MAGIC);
        u32_t offset = offset_p - ptr;
        u8_t * data_buf = (u8_t *)ry_malloc(offset+ 20);
        ry_memcpy(data_buf, ptr, offset);

        
        pb_istream_t stream = pb_istream_from_buffer(data_buf, offset);
        if (0 == pb_decode(&stream, UploadFileMeta_fields, fileMeta)) {
            LOG_ERROR("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            //status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
            goto exit;
        }
        
        fp = (FIL*)ry_malloc(sizeof(FIL));
        if(fp == NULL){
            LOG_ERROR("[%s] malloc fail\n",__FUNCTION__);
            goto exit;
        }
        f_unlink(fileMeta->file_name);
        res = f_open(fp, fileMeta->file_name, FA_CREATE_ALWAYS | FA_WRITE);
        if(res != FR_OK){
            goto exit;
        }
        offset = offset + strlen(FILE_TRANSFAR_MAGIC);
        LOG_DEBUG("[%s]open ok\n", fileMeta->file_name);
        res = f_write(fp, &(msg->message.req.val.bytes[offset]), msg->message.req.val.size - offset, &written_bytes);
        LOG_DEBUG("%d--write %d\n",msg->message.req.val.size, written_bytes);
        if(res != FR_OK){
            goto exit;
        }
        file_name_copy = ry_malloc(100);
        ry_memcpy(file_name_copy, fileMeta->file_name, 100);
        ry_free(data_buf);
        ry_free(fileMeta);
        
    }else{
        
        res = f_write(fp, &(msg->message.req.val.bytes[0]), msg->message.req.val.size, &written_bytes);
        LOG_DEBUG("%d--write %d\n",msg->message.req.val.size, written_bytes);
        if(res != FR_OK){
            goto exit;
        }
    }
    LOG_DEBUG("pack %d-----%d\n", total, sn);
    if(sn == total - 1){

        
        f_close(fp);
        ry_free(fp);
        rbp_send_resp(CMD_DEV_UPLOAD_FILE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);

        if(strstr((const char*)file_name_copy, "bmp")){
            
            clear_buffer_black();
            img_exflash_to_framebuffer(file_name_copy, 0, 0, &image_width, &image_height, 0);
            gdisp_update();
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
            f_unlink((const TCHAR*)file_name_copy);
            ry_free(file_name_copy);
        }

        if(strcmp((const char*)file_name_copy, "boot.bin") == 0){
            update_boot_file();
        }
    }
    
exit:

    return RY_SUCC;
}


ry_sts_t ota_token_verify(u8_t* data, u16_t len, u8_t* token)
{
    u8_t this_iv[16];
    ry_sts_t status;
    u32_t decrypt_nonce;
    u32_t decrypt_sn;
    u8_t* cipher = data + IV_LEN;
    u8_t* plain  = data + IV_LEN;
    u8_t* mac;
    u8_t* did;
    u32_t timestamp;

    ry_memcpy(this_iv, data, IV_LEN);
    status = ry_aes_cbc_decrypt(device_info_get_aes_key(), this_iv, len, cipher, plain);
    if (status != RY_SUCC) {
        LOG_ERROR("[ota_token_verify] Fail. %x\r\n", status);
        return status;
    } else {
        LOG_DEBUG("[ota_token_verify] Decrypt data: ");
        ry_data_dump(plain, len, ' ');

        mac = plain;
        ry_memcpy(token, plain+10, 12);
        ry_memcpy((u8_t*)&timestamp, plain+22, 4);
        did = plain + 26;

        /* Verify mac */
        if (ry_memcmp(mac, device_info_get_mac(), 6)) {
            LOG_ERROR("[ota_token_verify] MAC error.\r\n");
            return RY_SET_STS_VAL(RY_CID_OTA, RY_ERR_DECRYPTION);
        }

        /* Verify did */
        //if (ry_memcmp(did, device_info_get_mac, 6)) {
        //    LOG_ERROR("[ota_token_verify] MAC error.\r\n");
        //    return RY_SET_STS_VAL(RY_CID_OTA, RY_ERR_DECRYPTION);
        //}
    

        /* Verify timestamp */
        LOG_DEBUG("[ota_token_verify] timestamp: %d, cur_time: %d\r\n", timestamp, ryos_utc_now());
        if (timestamp < ryos_utc_now()) {
            LOG_ERROR("[ota_token_verify] Timestamp error.\r\n");
            //return RY_SET_STS_VAL(RY_CID_OTA, RY_ERR_TIMESTAMP);
            return RY_SUCC;
        }

    }
   
    return RY_SUCC;
}


ry_sts_t update_token_handler(uint8_t* data, int len)
{
    ry_sts_t verify_result;
    u8_t token[12];


    FwUpdateTokenResult param = {0};
    uint8_t buf[30] = {0};
    ry_sts_t status;

    
    LOG_DEBUG("[update_token_handler] token: ");
    ry_data_dump(data, len, ' ');

    verify_result = ota_token_verify(data, len, token);
    if (verify_result == RY_SUCC) {
        /* Verify, set token */
        mible_setToken(token);

        pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(FwUpdateTokenResult )));

        ry_memcpy(param.device_token.bytes, token, 12);
        param.device_token.size = 12;

        /* Now we are ready to encode the message! */
        if (!pb_encode(&stream_o, FwUpdateTokenResult_fields, &param)) {
            LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
        }
        //message_length = stream.bytes_written;
        
        status = rbp_send_resp(CMD_DEV_FW_UPDATE_TOKEN, RBP_RESP_CODE_SUCCESS, buf, stream_o.bytes_written, 0);
        
    } else {
    
        rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SIGN_VERIFY_FAIL, NULL, 0, 0);

        pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(FwUpdateTokenResult )));


        /* Now we are ready to encode the message! */
        if (!pb_encode(&stream_o, FwUpdateTokenResult_fields, &param)) {
            LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
        }
        //message_length = stream.bytes_written;
        
        status = rbp_send_resp(CMD_DEV_FW_UPDATE_TOKEN, RBP_RESP_CODE_SIGN_VERIFY_FAIL, NULL, 0, 0);
    }
		
		return RY_SUCC;
   
}

u8_t get_data_check_sum(u8_t * data, int size)
{
    u8_t sum = 0;
    int i = 0;

    for(i = 0; i< size; i++){
        sum += data[i];
    }
    return sum;
}


int get_file_check_sum(char * file_name)
{
    FIL * fp = NULL;
    u8_t * temp_buf = NULL;

    FRESULT ret = FR_OK;
    u32_t read_size = 0;
    int i = 0;
    int file_size = 0;
    int sector_num = 0;
    u8_t sum = 0;
    
    
    fp = (FIL*)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        goto exit;
    }

    temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        goto exit;
    }

    ret = f_open(fp, file_name, FA_READ);
    if(ret != FR_OK){
        goto exit;
    }

    file_size = f_size(fp);
    sector_num = (file_size + EXFLASH_SECTOR_SIZE - 1)/EXFLASH_SECTOR_SIZE;

    for(i = 0; i < sector_num; i++){
        ret = f_read(fp, temp_buf, EXFLASH_SECTOR_SIZE, &read_size);
        if(ret != FR_OK){
            goto exit;
        }

        sum += get_data_check_sum(temp_buf, read_size);
    }

    f_close(fp);
    ry_free(fp);
    ry_free(temp_buf);
    
    
    return (int)sum;

exit:
    f_close(fp);
    ry_free(fp);
    ry_free(temp_buf);
    
    
    return -1;

}


file_repair_ctx_t file_repair_ctx_v = {0};




void file_repair_timeout_handle(void* arg)
{
    
    LOG_ERROR("file repair timeout.\n");
    resume_other_task_for_update_end();
    resource_file_update_status_clear();
}




ry_sts_t file_repair_start(RbpMsg *msg)
{
    ry_sts_t ret;
    uint32_t resp_code = RBP_RESP_CODE_SUCCESS;
    RepairResStartParam start_param = {0};
    u8_t phoneType;
    int  connInterval = 0;
    pb_istream_t stream = {0};

    stream = pb_istream_from_buffer(msg->message.req.val.bytes, msg->message.req.val.size);

    suspend_other_task_for_update_start();
    
    if (0 == pb_decode(&stream, RepairResStartParam_fields, &start_param)){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        resp_code = RBP_RESP_CODE_DECODE_FAIL;
    }

    file_repair_ctx_v.file_length = start_param.length;

    if(start_param.length >= 4096 * 1024){
	    LOG_DEBUG("mkfs\n");
        u8_t *work = (u8_t *)ry_malloc(4096);
	    f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096);
        ry_free(work);
        //clear_check_file_list();
        //free_check_file_list();
    }


    uint8_t conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
//    phoneType = ry_ble_get_peer_phone_type();
//    switch (phoneType) {
//        case PEER_PHONE_TYPE_ANDROID:
//            if (connInterval == 0) {
//                connInterval = 6;
//            }
//            ble_updateConnPara(6, connInterval, 500, 0);
//            break;

//        case PEER_PHONE_TYPE_IOS:
//            ble_updateConnPara(6, 24, 500, 0);
//            break;

//        default:
//            ble_updateConnPara(6, 24, 500, 0);
//            break;
//    }
    
    
    ret = ble_send_response(CMD_DEV_REPAIR_RES_START, resp_code, NULL,  0, 1,NULL, NULL);

    ry_hal_wdt_reset();
	if(reset_timer == 0){
        
        ry_timer_parm timer_para;
        timer_para.timeout_CB = file_repair_timeout_handle;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "update";
        reset_timer = ry_timer_create(&timer_para);


        //ry_timer_start(reset_timer, 20000, ry_system_reset, NULL);

        //reset_timer = ry_timer_create(0);
        if (reset_timer == RY_ERR_TIMER_ID_INVALID) {
            LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
            add_reset_count(UPGRADE_RESTART_COUNT);
            ry_system_reset();
        }
    }
    ry_timer_stop(reset_timer);
    ry_timer_start(reset_timer, 20000, file_repair_timeout_handle, NULL);
    ota_idle_timer_stop();

    resource_file_update_status_clear();

    return ret;
}





ry_sts_t file_repair_handle(RbpMsg *msg)
{
	ry_sts_t status = RY_SUCC;


	msg_param_t * tb;


    u32_t sn = msg->message.req.sn;
    u32_t total = msg->message.req.total;

    //pack_recved_size += msg->message.req.val.size;
    update_ctx.update_state = UPDATING;
    ry_hal_wdt_reset();
    if(reset_timer == 0){
        
        ry_timer_parm timer_para;
        timer_para.timeout_CB = file_repair_timeout_handle;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "update";
        reset_timer = ry_timer_create(&timer_para);


        //ry_timer_start(reset_timer, 20000, ry_system_reset, NULL);

        //reset_timer = ry_timer_create(0);
        if (reset_timer == RY_ERR_TIMER_ID_INVALID) {
            LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
            add_reset_count(UPGRADE_RESTART_COUNT);
            ry_system_reset();
        }
    }
    ry_timer_stop(reset_timer);
    ry_timer_start(reset_timer, 20000, file_repair_timeout_handle, NULL);


    update_percent_display(sn, total-1, UPDATE_DISPLAY_MODE_ALL);

    if(sn == 0){
        //todo:
    }


    restore_file(msg->message.req.val.bytes, msg->message.req.val.size);

    if((msg->message.req.val.size == 0)&&(total == 1)){
        update_stop();
        return status;
    }
        
    
    if ((total - 1) == (sn)){

        update_ctx.update_state = UPDATE_IDLE;
        
        //check_fs_header(FS_CHECK_UPDATE);
        //save_check_file_list(FILE_CHECK_SAVE_NORMAL);
        file_lost_flag_clear();
        system_app_data_save();
        LOG_DEBUG("resource update succ \n");
        add_reset_count(UPGRADE_RESTART_COUNT);
        save_fs_header();
        ry_timer_start(reset_timer, 5000, (ry_timer_cb_t)ry_system_reset, NULL);
        rbp_send_resp(CMD_DEV_REPAIR_RES_FILE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    

        return status;
    }

    return status;

	//static uint32_t current_read_size = 0;


exit:


	if ((total - 1) == (sn)){
        rbp_send_resp(CMD_DEV_FW_UPDATE_FILE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    }
	
	return status;
}






