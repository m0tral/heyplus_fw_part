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

#include "device_management_service.h"
#include "ry_hal_inc.h"
#include "ryos_update.h"
#include "ff.h"
#include "ry_font.h"
#include "timer_management_service.h"
#include "ry_nfc.h"
#include "stdlib.h"
#include "window_management_service.h"
#include "card_management_service.h"
#include "msg_management_service.h"
#include "activity_setting.h"
#include "sensor_alg.h"
#include "algorithm.h"
#include "device.pb.h"
#include "pb.h"
#include "pb_encode.h"
#include "ble_task.h"
#include "stdio.h"
#include "storage_management_service.h"
#include "data_management_service.h"

/*********************************************************************
 * CONSTANTS
 */ 


#define STORAGE_DEBUG               LOG_ERROR

/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */


u32_t g_fs_free_size;
extern FATFS *ry_system_fs;
u8_t * lost_file_flag = NULL;
const char * check_file_list[] = { DEV_RESOURCE_INFO_FILE, RY_FONT_HEIGHT_22, "touch.bin"/*, "converty.py"*/};

const checkFile_cell_t g_checkFlieList[] = {
{"ry_font22.ryf",		0xdc},
{"alarm_1.bmp",		0x33},
{"alarm_2.bmp",		0xa2},
{"alarm_3.bmp",		0x5a},
{"alipay_agree.bmp",		0xcf},
{"alipay_bind.bmp",		0xc8},
{"alipay_OK.bmp",		0x9a},
{"alipay_ubind.bmp",		0x5},
{"arrow_down.bmp",		0x10},
{"device.bmp",		0x50},
{"g_delete.bmp",		0x2f},
{"g_notification.bmp",		0xa6},
{"g_status_00_succ.bmp",		0x9d},
{"g_status_01_fail.bmp",		0x23},
{"g_widget_00_enter.bmp",		0x7},
{"g_widget_01_pending.bmp",		0x55},
{"g_widget_03_stop2.bmp",		0xc2},
{"g_widget_05_restore.bmp",		0x5e},
{"g_widget_06_cancel.bmp",		0xad},
{"g_widget_close_11.bmp",		0x64},
//{"g_widget_drop_12.bmp",		0x91},
{"g_widget_execute.bmp",		0xf6},
{"g_widget_finish_08.bmp",		0xda},
{"g_widget_on_09.bmp",		0x75},
{"g_widget_toggle_10.bmp",		0x56},
{"hrm_detect_01.bmp",		0xbf},
{"hrm_detect_02.bmp",		0xff},
{"hrm_detect_03.bmp",		0xa5},
{"hrm_detect_04.bmp",		0x47},
{"hrm_detect_05.bmp",		0xd3},
{"hrm_detect_06.bmp",		0xee},
{"hrm_detect_07.bmp",		0x3b},
{"hrm_detect_08.bmp",		0x80},
{"icon_02_shoes.bmp",		0x9},
{"icon_03_check.bmp",		0x72},
{"icon_1km.bmp",		0xb4},
{"icon_phone.bmp",		0xd2},
{"icon_phone1.bmp",		0xda},
{"icon_phone_answer.bmp",		0x26},
//{"loading_1.bmp",		0x6e},
//{"loading_2.bmp",		0xc6},
//{"loading_3.bmp",		0x12},
//{"loading_4.bmp",		0xe6},
//{"loading_5.bmp",		0x4a},
//{"loading_6.bmp",		0x56},
{"menu_01_card.bmp",		0xe0},
{"menu_02_hrm.bmp",		0xd8},
{"menu_03_sport.bmp",		0x17},
{"menu_04_mijia.bmp",		0x8e},
{"menu_05_weather.bmp",		0xe5},
{"menu_06_clock.bmp",		0xcb},
//{"menu_07_calendar.bmp",		0x27},
{"menu_08_data.bmp",		0xeb},
{"menu_09_setting.bmp",		0x1d},
{"menu_10_alipay.bmp",		0x23},
{"m_binding_01_doing.bmp",		0x19},
{"m_card_00_guangzhou.bmp",		0x35},
{"m_card_01_shenzhen.bmp",		0x4f},
{"m_card_02_beijing.bmp",		0xdb},
{"m_card_03_xiaomi.bmp",		0xb9},
{"m_card_04_shanghai.bmp",		0x74},
{"m_card_05_gate.bmp",		0x2},
{"m_card_05_gate0.bmp",		0x6f},
{"m_card_06_building.bmp",		0xf7},
{"m_card_07_home.bmp",		0x76},
{"m_card_08_yangcheng.bmp",		0x67},
{"m_card_changan.bmp",		0x67},
{"m_card_chongqing.bmp",		0xcc},
{"m_card_guangxi.bmp",		0xb9},
{"m_card_haerbin.bmp",		0xe6},
{"m_card_hangzhou.bmp",		0x4c},
{"m_card_hefei.bmp",		0xef},
{"m_card_jiangsu.bmp",		0x6a},
{"m_card_jiling.bmp",		0x64},
{"m_card_lvcheng.bmp",		0x37},
{"m_card_qingdao.bmp",		0xab},
{"m_card_suzhou.bmp",		0x70},
{"m_card_wuhan.bmp",		0xf2},
//{"m_data_00_step.bmp",		0xc6},
//{"m_data_01_calorie.bmp",		0x5e},
//{"m_data_02_km.bmp",		0x78},
//{"m_data_03_hrm.bmp",		0x76},
//{"m_data_04_sitting.bmp",		0xa0},
//{"m_data_05_sleeping.bmp",		0x54},
{"m_long_sit_1.bmp",		0xe3},
{"m_long_sit_2.bmp",		0x47},
{"m_long_sit_3.bmp",		0xe2},
{"m_long_sit_4.bmp",		0x1f},
{"m_long_sit_5.bmp",		0xe7},
{"m_long_sit_6.bmp",		0xae},
{"m_long_sit_7.bmp",		0xac},
//{"m_msg_01_milestone.bmp",		0xfe},
//{"m_msg_04_calendar.bmp",		0xfe},
{"m_notice_00_phone.bmp",		0x3c},
{"m_notice_01_mail.bmp",		0xbb},
{"m_notice_02_info.bmp",		0xd2},
{"m_notice_calender.bmp",		0xbd},
{"m_notice_dingding.bmp",		0x15},
{"m_notice_douyin.bmp",		0xc9},
{"m_notice_mijia.bmp",		0x6d},
{"m_notice_momo.bmp",		0xdc},
{"m_notice_qq.bmp",		0x2},
{"m_notice_toutiao.bmp",		0xcc},
{"m_notice_wechat.bmp",		0x2},
{"m_notice_weibo.bmp",		0x82},
{"m_notice_zhifubao.bmp",		0x3a},
{"m_notice_zhihu.bmp",		0x49},
{"m_no_alarm.bmp",		0x2e},
{"m_no_msg.bmp",		0x71},
{"m_relax_00_enter.bmp",		0xa4},
{"m_setting_00_doing.bmp",		0xd8},
{"m_sport_00_running.bmp",		0xac},
//{"m_sport_00_running_s.bmp",		0x5a},
{"m_sport_01_bike.bmp",		0xa3},
{"m_sport_01_running.bmp",		0xcf},
{"m_sport_02_jumping.bmp",		0x98},
{"m_sport_free.bmp",		0xb1},
{"m_sport_indoor_bike.bmp",		0x99},
{"m_sport_swimming_03.bmp",		0x30},
{"m_sport_walk.bmp",		0x43},
{"m_status_00_msg.bmp",		0x7c},
{"m_status_01_charging.bmp",		0x24},
{"m_status_02_charging.bmp",		0xa6},
{"m_status_02_charging2.bmp",		0xed},
{"m_status_03_forbid.bmp",		0x33},
{"m_status_offline.bmp",		0x48},
{"m_target_done_1.bmp",		0xca},
{"m_target_done_2.bmp",		0xd6},
{"m_target_done_3.bmp",		0xb8},
{"m_target_done_4.bmp",		0x1c},
{"m_target_done_5.bmp",		0x6f},
{"m_target_done_6.bmp",		0x25},
//{"m_upgrade_00_enter.bmp",		0x76},
{"m_upgrade_01_doing.bmp",		0x37},
{"m_weather_00_sunny.bmp",		0xce},
{"m_weather_01_rainning.bmp",		0x83},
{"m_weather_02_overcast.bmp",		0xa6},
{"m_weather_03_snow.bmp",		0x14},
{"m_weather_04_cloud.bmp",		0x7},
{"m_weather_07_haza.bmp",		0x4a},
{"m_weather_09_thunderstorm.bmp",		0x33},
{"m_weather_10_cloud_night.bmp",		0xe},
{"m_weather_10_rsvd.bmp",		0x66},
{"m_weather_10_sunny_night.bmp",		0xaa},
//{"nfc.bin",		0x63},
{"phone.bmp",		0x28},
{"power_on.bmp",		0xe5},
//{"resource_info.txt",		0xc2},
//{"sound_off.bmp",		0x52},
{"sport_animate_01.bmp",		0x2a},
{"sport_animate_02.bmp",		0xdf},
{"sport_animate_03.bmp",		0x58},
//{"surface_01_red_arron.bmp",		0x67},
//{"surface_colorful.bmp",		0x3f},
//{"surface_earth.bmp",		0x80},
//{"surface_galaxy.bmp",		0x9a},
//{"surface_greencurve.bmp",		0x9b},
//{"surface_redcurve.bmp",		0xd1},
//{"touch.bin",		0x90},
//{"transfer1.bmp",		0x1c},
//{"transfer2.bmp",		0x1c},
//{"transfer3.bmp",		0x1c},
{"up_unlock1.bmp",		0x5c},
{"up_unlock12.bmp",		0x1b},
{"up_unlock13.bmp",		0x32},
{"up_unlock14.bmp",		0x38},

};


#if 0
const u8_t* storage_minimal_delet_file[] = {
    "m_card_00_guangzhou.bmp",
    "m_card_01_shenzhen.bmp",
    "m_card_02_beijing.bmp",
    "m_card_03_xiaomi.bmp",
    "m_card_04_shanghai.bmp",
    "m_card_05_gate.bmp",
    "m_card_05_gate0.bmp",
    "m_card_06_building.bmp",
    "m_card_07_home.bmp",
    "m_card_08_yangcheng.bmp",
    "m_card_changan.bmp",
    "m_card_chongqing.bmp",
    "m_card_guangxi.bmp",
    "m_card_haerbin.bmp",
    "m_card_hangzhou.bmp",
    "m_card_hefei.bmp",
    "m_card_jiangsu.bmp",
    "m_card_jiling.bmp",
    "m_card_lvcheng.bmp",
    "m_card_qingdao.bmp",
    "m_card_suzhou.bmp",
    "m_card_wuhan.bmp",
};
#else
    const u8_t* storage_minimal_delet_file[] = {""};
#endif


/*********************************************************************
 * LOCAL FUNCTIONS
 */



bool storage_encode_repeated_file_names(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    int i;

    if(storage_get_lost_file_status() == 2){
        if (!pb_encode_tag_for_field(stream, field)){
            LOG_ERROR("[%s]-all type fail\n",__FUNCTION__);
            return false;
        }

        LOG_DEBUG("{%s}---%s\n",__FUNCTION__, ALL_FILE_REPAIR);
        
        if (!pb_encode_string(stream, (uint8_t*)ALL_FILE_REPAIR, strlen(ALL_FILE_REPAIR))){
            LOG_ERROR("[%s]-all encode fail\n",__FUNCTION__);
            return false;
        }
        
    }else{
    
        for (i = 0; i < storage_get_check_file_numbers(); i++)
        {
            if(lost_file_flag[i] == 1){
                if (!pb_encode_tag_for_field(stream, field)){
                    LOG_ERROR("[%s]-%s type fail\n",__FUNCTION__, g_checkFlieList[i].file_name);
                    return false;
                }

                LOG_DEBUG("{%s}---%s\n",__FUNCTION__, g_checkFlieList[i].file_name);
                
                if (!pb_encode_string(stream, (uint8_t*)g_checkFlieList[i].file_name, strlen(g_checkFlieList[i].file_name))){
                    LOG_ERROR("[%s]-%s encode fail\n",__FUNCTION__, g_checkFlieList[i].file_name);
                    return false;
                }
            }
        }

    }
    return true;
}



/**
 * @brief   API to check all file is OK 
 *
 * @param   None
 *
 * @return  None
 */
void storage_check_all_files_recover(void)
{
    FRESULT ret = FR_OK;
    u32_t read_bytes = 0;
    int checkSum = 0;
    int i = 0;
    const checkFile_cell_t * save_list = NULL;
    int save_list_num = 0;


    save_list = g_checkFlieList;
    save_list_num = sizeof(g_checkFlieList)/sizeof(checkFile_cell_t);
    
    for (i = 0; i < save_list_num; i++){
        ry_hal_wdt_reset();
        
        checkSum = get_file_check_sum(save_list[i].file_name);
        

        if( (checkSum&0xff) != save_list[i].checkSum ){
            LOG_DEBUG("[%s]-file lost:%s--%d\n", __FUNCTION__, save_list[i].file_name, ret);

            if(lost_file_flag == NULL){
                lost_file_flag = (u8_t *)ry_malloc(save_list_num * sizeof(u8_t *));
                ry_memset(lost_file_flag, 0, save_list_num * sizeof(u8_t *));
            }
            lost_file_flag[i] = 1;

            if(strstr(save_list[i].file_name, ".ryf")){
                ry_memset(lost_file_flag, 1, save_list_num * sizeof(u8_t *));
                break;
            }

            LOG_ERROR("[%s]---file lost: %s\n", __FUNCTION__, save_list[i].file_name);
        }else{
            LOG_DEBUG("[%s]-file OK:%s\n", __FUNCTION__, save_list[i].file_name);
        }

        LOG_DEBUG("[%s]-%s-0x%x\n",__FUNCTION__, save_list[i], checkSum);

    }
}

int storage_get_check_file_numbers(void)
{
    return sizeof(g_checkFlieList)/sizeof(checkFile_cell_t);
}

int storage_get_lost_file_numbers(void)
{
    int i = 0;
    int numbers = 0;
    
    for(int i = 0; i < storage_get_check_file_numbers(); i++){
        if(lost_file_flag[i] == 1){
            numbers++;
        }
    }

    return numbers;
}

/**
 * @brief   API to get file lost status 
 *
 * @param   None
 *
 * @return  0: no file lost  1:lost a little files   2: all file must recover
 */
int storage_get_lost_file_status(void)
{
	if(lost_file_flag == NULL){
	//no file lost
		return 0;
	}else{

	//have file lost
	    if(storage_get_lost_file_numbers() >= (storage_get_check_file_numbers()/2) ){
            return 2;
	    }
		return 1;
	}
}

void storage_file_list_send_result(ry_sts_t status, void* usr_data)
{
    if (status != RY_SUCC) {
        STORAGE_DEBUG("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)usr_data, status);     
    } else {
        LOG_DEBUG("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)usr_data, status);
    }
}



/**
 * @brief   API to send file lost list to app 
 *
 * @param   None
 *
 * @return  None
 */
void storage_send_lost_file_list(void)
{
    DeviceLostFileNames file_list = {0};
    int i = 0;
    int list_num = 0;
    u8_t * buf = NULL;
    pb_ostream_t stream_o = {0};
    u32_t version_len = 0;

    uint8_t conn_param_type;
    int  connInterval = 0;

    if(storage_get_lost_file_status() == 2){
        file_list.file_names = 1;
    }else{
        file_list.file_names = storage_get_lost_file_numbers();
    }
    
    file_list.file_names = 0;

    buf = (u8_t *)ry_malloc(file_list.file_names * FILE_NAME_PACK_SIZE + FILE_PACK_BASE_SIZE);

    extern void get_fw_ver(u8_t* des, u32_t* len);
    get_fw_ver((u8_t*)file_list.version, &version_len);
    
    file_list.name_list.funcs.encode = storage_encode_repeated_file_names;

    

    stream_o = pb_ostream_from_buffer(buf, (file_list.file_names * FILE_NAME_PACK_SIZE + FILE_PACK_BASE_SIZE));

    if (!pb_encode(&stream_o, DeviceLostFileNames_fields, &file_list)) {
        STORAGE_DEBUG("[%s]-- %d \n", __FUNCTION__, file_list.file_names );
        STORAGE_DEBUG("[%s]-{%s}-%s\n", __FUNCTION__, ERR_STR_ENCODE_FAIL, stream_o.errmsg );
        ry_hal_flash_erase(FILE_OPEN_CHECK_ADDR, 100);
        goto exit;
    }


    conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);


    ble_send_response(CMD_DEV_REPAIR_RES_GET_LOST_FILE_NAMES, RBP_RESP_CODE_SUCCESS, buf, stream_o.bytes_written, 
        0, storage_file_list_send_result, (void*)__FUNCTION__);

    ry_free(buf);
    

    return ;
    
exit:
    ry_free(buf);
    ble_send_response(CMD_DEV_REPAIR_RES_GET_LOST_FILE_NAMES, RBP_RESP_CODE_EXE_FAIL, NULL, 0, 
        0, NULL, NULL);
    return ;
}



void recover_fs_head(void)
{
    u8_t flag = FS_HEAD_OK;
    int i = 0;

    f_mount(0, "", 0);//cancel fs mount

    
    u8_t * temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        goto exit;
    }



    ry_hal_spi_flash_read(temp_buf, (EXFLASH_FS_HEAD_COPY_ADDR),EXFLASH_SECTOR_SIZE);

    if((temp_buf[0] != 0xEB) || (temp_buf[1] != 0xFE) || (temp_buf[0x1FE] != 0x55) || (temp_buf[0x1FF] != 0xAA)){
        goto exit;
    }
    
    for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
        ry_hal_spi_flash_read(temp_buf, (EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);
        ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE);
        ry_hal_spi_flash_write(temp_buf, (EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE), EXFLASH_SECTOR_SIZE);
    }
    

exit:

    ry_free(temp_buf);
    
}

void recover_fs_head_to_factroy(void)
{
    u8_t flag = FS_HEAD_OK;
    int i = 0;

    f_mount(0, "", 0);//cancel fs mount

    
    u8_t * temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        goto exit;
    }



    ry_hal_spi_flash_read(temp_buf, (FACTORY_FS_HEAD_ADDR),EXFLASH_SECTOR_SIZE);

    if((temp_buf[0] != 0xEB) || (temp_buf[1] != 0xFE) || (temp_buf[0x1FE] != 0x55) || (temp_buf[0x1FF] != 0xAA)){
        goto exit;
    }
    
    for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
        ry_hal_spi_flash_read(temp_buf, (FACTORY_FS_HEAD_ADDR + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);
        ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE);
        ry_hal_spi_flash_write(temp_buf, (EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE), EXFLASH_SECTOR_SIZE);

        ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE);
        ry_hal_spi_flash_write(temp_buf, (EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);
    }
    

exit:

    ry_free(temp_buf);
    
}

int storage_save_fs_check_sum(u8_t checkSum)
{
    u32_t checkSum32 = checkSum&0xff;

    u8_t *temp_buf = NULL;

    u32_t * temp_ptr = NULL;

    u32_t offset = 0;

    temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        STORAGE_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        return -1;
    }
    temp_ptr = (u32_t *)temp_buf;
    
    ry_hal_spi_flash_read(temp_buf, FS_HEAD_CHECK_SUM_PAGE_ADDR ,EXFLASH_SECTOR_SIZE);
    
    for(offset = 0; offset < EXFLASH_SECTOR_SIZE/sizeof(u32_t); offset++){
        if(temp_ptr[offset] == 0xFFFFFFFF){
            break;
        }
    }

    if(offset >= 1000){
        offset = 0;
        ry_hal_spi_flash_sector_erase(FS_HEAD_CHECK_SUM_PAGE_ADDR);
    }

    ry_hal_spi_flash_write((u8_t *)&checkSum32, (FS_HEAD_CHECK_SUM_PAGE_ADDR + offset * sizeof(u32_t)), sizeof(u32_t));
    //STORAGE_DEBUG("[%s] checkSum = 0x%x\n", __FUNCTION__, checkSum32);

    ry_free(temp_buf);


    return 0;
}

u32_t storage_get_fs_check_sum(void)
{
    u32_t checkSum32 = 0;

    u8_t *temp_buf = NULL;

    u32_t * temp_ptr = NULL;

    u32_t offset = 0;

    temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        STORAGE_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        return 0xFFFFFFFF;
    }
    temp_ptr = (u32_t *)temp_buf;
    
    ry_hal_spi_flash_read(temp_buf, FS_HEAD_CHECK_SUM_PAGE_ADDR ,EXFLASH_SECTOR_SIZE);
    
    for(offset = 0; offset < EXFLASH_SECTOR_SIZE/sizeof(u32_t); offset++){
        if(temp_ptr[offset] == 0xFFFFFFFF){
            break;
        }
    }

    if(offset > 0){
        checkSum32 = temp_ptr[offset - 1];
    }else{
        checkSum32 = 0xFFFFFFFF;
    }
    STORAGE_DEBUG("[%s] checkSum = 0x%x\n", __FUNCTION__, checkSum32);
    ry_free(temp_buf);
    
    return checkSum32;    
}


u32_t storage_cal_cur_fs_check_sum(fs_head_type_e type)
{
    u8_t i ;
    u8_t checkSum = 0;
    u32_t checkSum32 = 0;

    u32_t start_addr = 0;

    u8_t * temp_buf = NULL;

    if(type == FS_HEAD_CUR){
        start_addr = EXFLASH_FS_HEAD_START_ADDR;
    }else if(type == FS_HEAD_BACKUP){
        start_addr = EXFLASH_FS_HEAD_COPY_ADDR;
    }else{
        goto error;
    }

    
    temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        STORAGE_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto error;
        
    }
    
    for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
        ry_hal_spi_flash_read(temp_buf, (start_addr + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);

        checkSum += get_data_check_sum(temp_buf, EXFLASH_SECTOR_SIZE);

    }

    checkSum32 = checkSum & 0xFF;

    ry_free(temp_buf);
    
    STORAGE_DEBUG("[%s]type: %d checkSum = 0x%x\n", __FUNCTION__,type, checkSum32);
    
    return checkSum32;

error:

    return 0xFFFFFFFF;
}

/**
 * @brief   API to check fs head status 
 *
 * @param   None
 *
 * @return  None
 */
void storage_check_fs_head_status(void)
{
    u32_t cur_checkSum = 0, backup_checkSum = 0;
    u32_t backup_record_checkSum = 0;
    
    cur_checkSum = storage_cal_cur_fs_check_sum(FS_HEAD_CUR);
    
    backup_checkSum = storage_cal_cur_fs_check_sum(FS_HEAD_BACKUP);

    if(cur_checkSum == backup_checkSum){//fs head and backup is normal
        STORAGE_DEBUG("[%s] fs head is ok, checkSum = 0x%x\n", __FUNCTION__, cur_checkSum);
    }else{//fs head and backup is not normal
        backup_record_checkSum = storage_get_fs_check_sum();

        if(backup_record_checkSum == 0xFFFFFFFF){//not have backup
            STORAGE_DEBUG("[%s] fs head is fail, checkSum = 0x%x\n", __FUNCTION__, cur_checkSum);
            STORAGE_DEBUG("no backup to use\n");
        
        }else if(backup_record_checkSum == backup_checkSum){//backup is normal,recover fs head
            if(get_last_reset_type() != FS_RESTART_COUNT){
                STORAGE_DEBUG("[%s] fs head is fail, checkSum = 0x%x\n", __FUNCTION__, cur_checkSum);
                STORAGE_DEBUG("try to recover\n");
                recover_fs_head();
                add_reset_count(FS_RESTART_COUNT);
                ry_system_reset();
            }
        }else{//backup is not normal ,so can't recover
            STORAGE_DEBUG("[%s] fs head is fail, checkSum = 0x%x\n", __FUNCTION__, cur_checkSum);
            STORAGE_DEBUG("can't try to recover\n");            
        }        
    }    
}


void storage_minmal_init(void)
{
    FIL* fp = NULL;
    FRESULT ret = FR_OK;
    u32_t free_size = 0;

    fp = (FIL *)ry_malloc(sizeof(FIL));
    char ** delet_file_list = NULL;
    int i = 0;
    int delet_file_list_num = 0;

    delet_file_list = (char **)storage_minimal_delet_file;
    delet_file_list_num = sizeof(storage_minimal_delet_file)/sizeof(u8_t *);

    for (i = 0; i < delet_file_list_num; i++){
        ret = f_unlink ((const TCHAR*) delet_file_list[i]);
        f_getfree("", (DWORD*)&free_size, &ry_system_fs);
        LOG_DEBUG("[%s]---file i:%d, delet: %s, free_size:%d\n", __FUNCTION__, i, delet_file_list[i], free_size);
    }

    ry_free(fp);
}


void storage_management_init(void)
{
    if(ry_error_info_init() == 0){
        debug_info_print();
    }
    
#if (RAWLOG_SAMPLE_ENABLE == TRUE)
    ry_rawLog_info_init();
    storage_minmal_init();
#else
    //storage_check_fs_head_status();
    //storage_check_all_files_recover();
#endif
    
    // remove empty file if exist
    f_unlink("empty.bin");

    uint32_t free_size = 0;
    f_getfree("", (DWORD*)&free_size, &ry_system_fs);
    g_fs_free_size = free_size;
    LOG_ERROR("[storage_management_init] fs_free_size: %dkB\r\n", free_size << 2);
}


void save_fs_header(void)
{
    FRESULT ret = FR_OK;
    u8_t flag = FS_HEAD_OK;
    u8_t i ;
    u8_t checkSum = 0;

#if (RAWLOG_SAMPLE_ENABLE == TRUE)
    return;
#endif
    ry_hal_wdt_reset();

    if(get_cur_update_status() == UPDATING ){
        LOG_DEBUG("updating not save fs\n");
        return ;
    }

    if(get_running_flag()){
        LOG_DEBUG("running not save fs\n");
        return ;
    }

    if(flag == FS_HEAD_OK){
        u8_t * temp_buf = NULL;
        temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
        if(temp_buf == NULL){
            STORAGE_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
            return;
        }
        for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
            ry_hal_spi_flash_read(temp_buf, (EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);

            checkSum += get_data_check_sum(temp_buf, EXFLASH_SECTOR_SIZE);

            ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE);
            ry_hal_spi_flash_write(temp_buf, (EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE), EXFLASH_SECTOR_SIZE);
        }
        
        ry_free(temp_buf);

        storage_save_fs_check_sum(checkSum);
        
    }else if(flag == FS_HEAD_ERROR){
        
    }

}

void file_error_handle(const char * file_name, int error_code)
{
    u8_t i ;
    u8_t flag = 0;

    if(error_code == FR_DENIED){
        add_file_error_count();
    }else if(error_code == FR_NO_FILE){
        for(i = 0; i < sizeof(check_file_list)/sizeof(char *); i++){
            if(strcmp(file_name, check_file_list[i]) == 0){
                add_file_error_count();
            }
        }
    }
}



void check_fs_header(u8_t update_flag)
{
    FIL *fp = NULL;
    FRESULT ret = FR_OK;
    u8_t flag = FS_HEAD_OK;
    u8_t i ;
	u8_t * temp_buf = NULL;

    fp = (FIL*)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        goto exit;
    }

    temp_buf = (u8_t *)ry_malloc(EXFLASH_SECTOR_SIZE);
    if(temp_buf == NULL){
        goto exit;
    }

    //this is test code
    /*if(i == 99){
        for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
            ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE);
        }
    }*/
    
    
    for(i = 0; i < sizeof(check_file_list)/sizeof(char *); i++){
        ret = f_open(fp, check_file_list[i], FA_READ);
        if(ret != FR_OK){
			STORAGE_DEBUG(" %s open fail\n", check_file_list[i]);
            flag = FS_HEAD_ERROR;
            break;
        }

        if(f_size(fp) <= 0){
            STORAGE_DEBUG(" %s size fail\n", check_file_list[i]);
            flag = FS_HEAD_ERROR;
            break;
        }

        f_close(fp);
    }

    LOG_DEBUG("[%s] flag:%d status: %d\n", __FUNCTION__, update_flag, flag);

    if((update_flag == FS_CHECK_UPDATE)&&(flag == FS_HEAD_OK)){
        for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
            ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE);
        }
    }
    
    ry_hal_spi_flash_read(temp_buf, (EXFLASH_FS_HEAD_COPY_ADDR),EXFLASH_SECTOR_SIZE);

    if((temp_buf[0] != 0xEB) || (temp_buf[1] != 0xFE) || (temp_buf[0x1FE] != 0x55) || (temp_buf[0x1FF] != 0xAA)){
        if(flag == FS_HEAD_OK){
            save_fs_header();
        }
        goto exit;
    }

    if((flag == FS_HEAD_ERROR) && (update_flag == FS_CHECK_RESTORE)){
        
        for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
            ry_hal_spi_flash_read(temp_buf, (EXFLASH_FS_HEAD_COPY_ADDR + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);
            ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE);
            ry_hal_spi_flash_write(temp_buf, (EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE), EXFLASH_SECTOR_SIZE);
        }

        if((get_last_reset_type() != FS_RESTART_COUNT) &&(get_last_reset_type() != FACTORY_FS_RESTART_COUNT)){
            add_reset_count(FS_RESTART_COUNT);
            am_hal_reset_poi();
        }else if(get_last_reset_type() == FS_RESTART_COUNT){
            for(i = 0; i < EXFLASH_FS_HEAD_SECTOR_NUM; i++){
                ry_hal_spi_flash_read(temp_buf, (FACTORY_FS_HEAD_ADDR + i * EXFLASH_SECTOR_SIZE),EXFLASH_SECTOR_SIZE);

                if (i == 0){
                    if((temp_buf[0] != 0xEB) || (temp_buf[1] != 0xFE) || (temp_buf[0x1FE] != 0x55) || (temp_buf[0x1FF] != 0xAA)){
                        goto exit;
                    }
                }
                
                ry_hal_spi_flash_sector_erase(EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE);
                ry_hal_spi_flash_write(temp_buf, (EXFLASH_FS_HEAD_START_ADDR + i * EXFLASH_SECTOR_SIZE), EXFLASH_SECTOR_SIZE);
            }

            add_reset_count(FACTORY_FS_RESTART_COUNT);
            am_hal_reset_poi();
            
        }else{
            //nothing todo


        }
    }


exit:
    ry_free(fp);
    ry_free(temp_buf);

}
