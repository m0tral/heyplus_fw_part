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
#include <stdio.h>
#include "app_config.h"
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"
#include "ryos_timer.h"
#include <ry_hal_inc.h>
#include "scheduler.h"
#include "board.h"

#include "data_management_service.h"
#include "ry_statistics.h"
#include "ryos_protocol.h"
#include "dip.h"


/********************************************************************************
 * CONSTANTS
 */ 

/*
 * VARIABLES
 *******************************************************************************
 */
ry_DevStatisticsResult_t dev_statistics;

/*
 * FUNCTIONS
 *******************************************************************************
 */


 
void dev_statistics_clear(void)
{
    ry_memset(&dev_statistics, 0, sizeof(ry_DevStatisticsResult_t));
    //system_app_data_save();
    dev_statistics.start_time = ryos_rtc_now();
}


void ry_statistics_init(void)
{
    dev_statistics_clear();
}


void save_dev_statistics_data(void)
{
    FIL * fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;
    u32_t file_size = 0;
    u32_t i = 0;

    char * today_file_name = NULL;

    today_file_name = (char *)ry_malloc(80);
    if(today_file_name == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        return;
    }
    dev_statistics.end_time = ryos_rtc_now();
    //rintf(today_file_name, "%d_%s",get_day_number(), BODY_STATUS_DATA);

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(today_file_name);
        return;
    }


    sprintf(today_file_name, "%d_%s",get_day_number(), RY_STATISTICS_FILE_NAME);
    f_unlink(today_file_name);
    ret = f_open(fp, today_file_name, FA_OPEN_APPEND | FA_WRITE | FA_READ);

    if(ret != FR_OK){
        LOG_ERROR("[%s] file open failed, ret: %d\n", __FUNCTION__, ret);
        goto exit;
    }

    ret = f_write(fp, &dev_statistics, sizeof(dev_statistics) , &written_bytes);
    if(ret != FR_OK){
        LOG_ERROR("[%s] file write failed, ret: %d\n", __FUNCTION__, ret);
        goto exit;
    }
    
    
    dev_statistics_clear();
exit:
    f_close(fp);
    ry_free(today_file_name);
    ry_free(fp);

}


int get_dev_statistics_data(ry_DevStatisticsResult_t * desk_data, int day)
{
    FIL * fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;
    u32_t file_size = 0;
    u32_t i = 0;
    int result = RY_ERR_INVALIC_PARA;

    char * today_file_name = NULL;

    if(desk_data != NULL){
        ry_memset(desk_data, 0, sizeof(ry_DevStatisticsResult_t));
    }else{
        goto exit;
    }

    today_file_name = (char *)ry_malloc(80);
    if(today_file_name == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    //rintf(today_file_name, "%d_%s",get_day_number(), BODY_STATUS_DATA);

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(today_file_name);
        today_file_name = NULL;
        goto exit;
    }


    sprintf(today_file_name, "%d_%s", day, RY_STATISTICS_FILE_NAME);
    ret = f_open(fp, today_file_name, FA_READ);
    if(ret != FR_OK){
        LOG_DEBUG("<%s>\n", today_file_name);
        desk_data->start_time = ryos_rtc_now();
        goto exit;
    }
    
    ret = f_read(fp, desk_data, sizeof(ry_DevStatisticsResult_t) , &written_bytes);
    if(ret != FR_OK){
        LOG_ERROR("[%s] file read failed, ret: %d\n", __FUNCTION__, ret);
        goto exit;
    }

    
    result = RY_SUCC;
exit:
    f_close(fp);
    f_unlink(today_file_name);
    ry_free(today_file_name);
    ry_free(fp);
    return result;
}

void data_transform_counter(DevStatisticsResultRecord * record, int cmd, int val)
{
    ry_memset(record , 0 , sizeof(DevStatisticsResultRecord));
    record->cmd = (STATISTICS_CMD)cmd;
    record->desc_type = DESC_TYPE_DESC_COUNTER;
    record->has_counter = 1;
    record->counter = val;
    
}

void data_transform_timestamp(DevStatisticsResultRecord * record, int cmd, u32_t count, int32_t * val)
{
    int i = 0;
    ry_memset(record , 0 , sizeof(DevStatisticsResultRecord));
    record->cmd = (STATISTICS_CMD)cmd;
    record->desc_type = DESC_TYPE_DESC_TIMESTAMP_VALUE_PAIR;
    record->has_counter = 0;
    
    record->time_value_count = count;

    for(i = 0; i < count%RY_STATISTICS_MAX_COUNT; i++){
        record->time_value[i].timestamp = val[i];

    }
    
}

u32_t pack_statistic_data(u32_t read_record, u8_t ** desk_data, u32_t * pack_size, u8_t day)
{
    u32_t pack_data_size = 0;
    DevStatisticsResult * statistics_result = (DevStatisticsResult *)ry_malloc(sizeof(DevStatisticsResult));

    ry_DevStatisticsResult_t * data_result = NULL;

    pb_ostream_t stream_o = {0};

    data_result = (ry_DevStatisticsResult_t *)ry_malloc(sizeof(ry_DevStatisticsResult_t));

    if(statistics_result == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }

    
    if(data_result == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    if(day == get_day_number()){
        
        *data_result = dev_statistics;
        dev_statistics.end_time = ryos_rtc_now();
        
    }else{        
        
        if(get_dev_statistics_data(data_result, day) != RY_SUCC){
            goto exit;
        }
        
    }
    ry_memset(statistics_result, 0 , sizeof(DevStatisticsResult));

    statistics_result->start_time = data_result->start_time;
    statistics_result->end_time = data_result->end_time;
    statistics_result->records_count = STATISTICS_CMD_STATISTICS_MAX_ENTRY - 1;

    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_CHARGE_FULL_DURATION],STATISTICS_CMD_DEV_CHARGE_FULL_DURATION,  data_result->charge_full);
    
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_RAISE_WAKEUP_COUNT],STATISTICS_CMD_DEV_RAISE_WAKEUP_COUNT,  data_result->raise_wake_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_LONG_SIT_COUNT],STATISTICS_CMD_DEV_LONG_SIT_COUNT,  data_result->long_sit_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SCREEN_WAKEUP_COUNTER],STATISTICS_CMD_UI_CLICK_SCREEN_WAKEUP_COUNTER,  data_result->screen_on_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_HOME_WAKEUP_COUNTER],STATISTICS_CMD_UI_CLICK_HOME_WAKEUP_COUNTER,  data_result->home_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SLIDE_UP_UNLOCK_COUNTER],STATISTICS_CMD_UI_SLIDE_UP_UNLOCK_COUNTER,  data_result->unlock_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_INCOMING_COUNTER],STATISTICS_CMD_UI_INCOMING_COUNTER,  data_result->incoming_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_RECV_MSG_COUNTER],STATISTICS_CMD_UI_RECV_MSG_COUNTER,  data_result->msg_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_RECV_MSG_COUNTER],STATISTICS_CMD_UI_CLICK_RECV_MSG_COUNTER,  data_result->msg_click_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SHOW_MSG_DROP_COUNTER],STATISTICS_CMD_UI_SHOW_MSG_DROP_COUNTER,  data_result->drop_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_MSG_DROP_COUNTER],STATISTICS_CMD_UI_CLICK_MSG_DROP_COUNTER,  data_result->drop_click_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_MSG_DROP_CLEAR_COUNTER],STATISTICS_CMD_UI_CLICK_MSG_DROP_CLEAR_COUNTER,  data_result->drop_clear_all_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SHOW_MENU_COUNTER],STATISTICS_CMD_UI_SHOW_MENU_COUNTER,  data_result->menu_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_HOME_BACK_COUNTER],STATISTICS_CMD_UI_CLICK_HOME_BACK_COUNTER,  data_result->home_back_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_CARDS_COUNTER],STATISTICS_CMD_UI_CLICK_CARDS_COUNTER,  data_result->card_bag_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_CHANGE_CARD_COUNTER],STATISTICS_CMD_UI_CLICK_CHANGE_CARD_COUNTER,  data_result->card_change_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_CHANGE_CARD_FAILE_COUNTER],STATISTICS_CMD_UI_CLICK_CHANGE_CARD_FAILE_COUNTER,  data_result->card_change_fail_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SPORT_COUNTER],STATISTICS_CMD_UI_CLICK_SPORT_COUNTER,  data_result->sport_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_SPORT_RAISE_WAKEUP_COUNTER],STATISTICS_CMD_DEV_SPORT_RAISE_WAKEUP_COUNTER,  data_result->sport_rasie_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_CLICK_WAKEUP_COUNTER],STATISTICS_CMD_UI_SPORT_CLICK_WAKEUP_COUNTER,  data_result->sport_click_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_OUT_DOOR_RUN_COUNTER],STATISTICS_CMD_UI_SPORT_OUT_DOOR_RUN_COUNTER,  data_result->sport_out_door_run);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_PAUSE_COUNTER],STATISTICS_CMD_UI_SPORT_PAUSE_COUNTER,  data_result->sport_pause);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_RUN_CLICK_HOME_COUNTER],STATISTICS_CMD_UI_SPORT_RUN_CLICK_HOME_COUNTER,  data_result->sport_back);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SPORT_HOME_BACK_COUNTER],STATISTICS_CMD_UI_CLICK_SPORT_HOME_BACK_COUNTER,  data_result->sport_stop);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_TOO_SHORT_TIME_COUNTER],STATISTICS_CMD_UI_SPORT_TOO_SHORT_TIME_COUNTER,  data_result->sport_short_time);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_TOO_SHORT_DISTANCE_COUNTER],STATISTICS_CMD_UI_SPORT_TOO_SHORT_DISTANCE_COUNTER,  data_result->sport_short_distance);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_SPORT_FINISH_COUNTER],STATISTICS_CMD_UI_SPORT_FINISH_COUNTER,  data_result->sport_finish);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_HRM_CLICK_COUNTER],STATISTICS_CMD_UI_HRM_CLICK_COUNTER,  data_result->hrm_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_HRM_FINISH_COUNTER],STATISTICS_CMD_DEV_HRM_FINISH_COUNTER,  data_result->hrm_finish_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_WEATHER_COUNTER],STATISTICS_CMD_UI_CLICK_WEATHER_COUNTER,  data_result->weather_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_WEATHER_CHANGE_COUNTER],STATISTICS_CMD_UI_WEATHER_CHANGE_COUNTER,  data_result->weather_change_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_REQ_WEATHER_INFO_COUNTER],STATISTICS_CMD_UI_REQ_WEATHER_INFO_COUNTER,  data_result->weather_req_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_GET_WEATHER_INFO_COUNTER],STATISTICS_CMD_UI_GET_WEATHER_INFO_COUNTER,  data_result->weather_get_info);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_ALARM_COUNTER],STATISTICS_CMD_UI_CLICK_ALARM_COUNTER,  data_result->alarm_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_ALARM_INFO_COUNTER],STATISTICS_CMD_UI_CLICK_ALARM_INFO_COUNTER,  data_result->alarm_info_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_ALARM_OPEN_COUNTER],STATISTICS_CMD_UI_CLICK_ALARM_OPEN_COUNTER,  data_result->alarm_open_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_ALARM_CLOSE_COUNTER],STATISTICS_CMD_UI_CLICK_ALARM_CLOSE_COUNTER,  data_result->alarm_close_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_DATA_COUNTER],STATISTICS_CMD_UI_CLICK_DATA_COUNTER,  data_result->data_info_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_DATA_SLIDE_UP_COUNTER],STATISTICS_CMD_UI_CLICK_DATA_SLIDE_UP_COUNTER,  data_result->data_slide_up);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_DATA_SLIDE_DOWN_COUNTER],STATISTICS_CMD_UI_CLICK_DATA_SLIDE_DOWN_COUNTER,  data_result->data_slide_down);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_MIJIA_COUNTER],STATISTICS_CMD_UI_CLICK_MIJIA_COUNTER,  data_result->mijia_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_MIJIA_EXEC_COUNTER],STATISTICS_CMD_UI_CLICK_MIJIA_EXEC_COUNTER,  data_result->mijia_use);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_MIJIA_CHANGE_COUNTER],STATISTICS_CMD_UI_CLICK_MIJIA_CHANGE_COUNTER,  data_result->mijia_change);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SETTING_COUNTER],STATISTICS_CMD_UI_CLICK_SETTING_COUNTER,  data_result->setting_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SETTING_ABOUT_COUNTER],STATISTICS_CMD_UI_CLICK_SETTING_ABOUT_COUNTER,  data_result->about_dev_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SETTING_RESET_COUNTER],STATISTICS_CMD_UI_CLICK_SETTING_RESET_COUNTER,  data_result->user_reset_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_UI_CLICK_SETTING_REPLACE_COUNTER],STATISTICS_CMD_UI_CLICK_SETTING_REPLACE_COUNTER,  data_result->user_replace_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_BIND_REQ_COUNTER],STATISTICS_CMD_DEV_BIND_REQ_COUNTER,  data_result->bind_req_count);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_BIND_REQ_OK_COUNTER],STATISTICS_CMD_DEV_BIND_REQ_OK_COUNTER,  data_result->bind_req_ok);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_BIND_SUCCESS_COUNTER],STATISTICS_CMD_DEV_BIND_SUCCESS_COUNTER,  data_result->bind_success);
    data_transform_counter(&statistics_result->records[STATISTICS_CMD_DEV_DISCONNECT_COUNTER],STATISTICS_CMD_DEV_DISCONNECT_COUNTER,  data_result->bt_disconnect_count);

    data_transform_timestamp(&statistics_result->records[STATISTICS_CMD_DEV_DISCONNECT_TIME_COUNTER],STATISTICS_CMD_DEV_DISCONNECT_TIME_COUNTER,  data_result->bt_disconnect_count, (int32_t *)&(data_result->bt_disconnect_time[0]));
    data_transform_timestamp(&statistics_result->records[STATISTICS_CMD_DEV_CHARGE_BEGIN_TIME],STATISTICS_CMD_DEV_CHARGE_BEGIN_TIME,  data_result->charge_begin_count, (int32_t *)&(data_result->charge_begin_time[0]));
    data_transform_timestamp(&statistics_result->records[STATISTICS_CMD_DEV_CHARGE_STOP_TIME],STATISTICS_CMD_DEV_CHARGE_STOP_TIME,  data_result->charge_end_count, (int32_t *)&(data_result->charge_stop_time[0]));


    *desk_data = (u8_t *)ry_malloc(1500);

    if(*desk_data == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
    }

    stream_o = pb_ostream_from_buffer(*desk_data, (1500));


    if (!pb_encode(&stream_o, DevStatisticsResult_fields, (statistics_result))) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    /*LOG_DEBUG("-----------------------------------------\n");
    ry_data_dump(*desk_data, stream_o.bytes_written, ' ');
    LOG_DEBUG("-----------------------------------------\n");*/

    LOG_ERROR("waise:%d, clickON:%d, longSit:%d\r\n", dev_statistics.raise_wake_count, dev_statistics.screen_on_count, dev_statistics.long_sit_count);
    if(day == get_day_number()){
        dev_statistics_clear();
    }else{
        

    }

    *pack_size = stream_o.bytes_written;
    pack_data_size = stream_o.bytes_written;
exit:
    ry_free(statistics_result);
    ry_free(data_result);
    return pack_data_size;
}


u32_t pack_statistic_data_record(DataRecord * data_record, u8_t day)
{
    u32_t read_size = 0;
    msg_param_t * param = (msg_param_t *)ry_malloc(sizeof(msg_param_t));
    if(param == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }else{
        ry_memset(param, 0, sizeof(msg_param_t));
    }
    if(data_record == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    

    if(data_record == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }else{
        ry_memset(data_record, 0, sizeof(DataRecord));
    }
    
    data_record->data_type = DataRecord_DataType_DEV_STATISTICS;

    data_record->val.funcs.encode = encode_data_record;
    data_record->val.arg = param;
    
    //read_size = pack_body_status(1, &(param->buf), (u32_t *)&(param->len) , day);
    read_size = pack_statistic_data(1, &(param->buf), (u32_t *)&(param->len) , day);


    if(read_size == 0){
        ry_free(param);
        data_record->val.arg = NULL;
    }


exit:
    return read_size;
}


int get_statistics_dataSet_num(void)
{
    int i =0, day = 0;
    char * file_name = NULL;

    FRESULT ret = FR_OK;

    int data_set_num = 0;
    FIL* fp = NULL;

    if(ryos_rtc_now() - dev_statistics.start_time < RY_STATISTICS_UPLOAD_TIME_THRESHOLD){
        return 0 ;
    }

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }
     
    file_name = (char *)ry_malloc(80);
    for(i = 0; i < 7; i++){
        day = (i + get_day_number())%7;   

        if(i == 0){
            data_set_num++;
            continue;
        }
        
        if(file_name == NULL){
            LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
            break;
        }
        sprintf(file_name, "%d_%s", day, RY_STATISTICS_FILE_NAME);


        

        ret = f_open(fp, file_name, FA_READ);
        if(ret != FR_OK){
            LOG_ERROR("[%s] %s open failed\n",__FUNCTION__, file_name);
            f_close(fp);
            f_unlink(file_name);
        }else{
            data_set_num++;
            f_close(fp);
        }

    }

    LOG_INFO("[%s]--get statistics\n",__FUNCTION__);

exit:
    ry_free(file_name);
    ry_free(fp);
    return data_set_num;
}

DataSet * get_statistics_dataSet(u8_t day)
{
    //DataRecord * dataRecord = pack_data_record(data_type);

    static int last_day = -1;
    if(last_day == day){
        return NULL;
    }
    last_day= day;

    if(ryos_rtc_now() - dev_statistics.start_time < RY_STATISTICS_UPLOAD_TIME_THRESHOLD){
        return NULL;
    }
    
    u32_t i = 0;
    DataSet * dataSet = (DataSet *)ry_malloc(sizeof(DataSet));
    u8_t len = 0;
    u32_t read_size = 0;
    dataSet_repeat_param_t * param = NULL;
    u32_t dataSet_len = 0;
    //LOG_DEBUG("[%s]---\n\n\n\n\n\n",__FUNCTION__);

    if(dataSet == NULL){
        LOG_DEBUG("[%s] malloc failed \n", __FUNCTION__);
        return NULL;
    }
    ry_memset(dataSet, 0, sizeof(DataSet));

    device_info_get(DEV_INFO_TYPE_DID, (u8_t*)dataSet->did, &len);
    //ry_memcpy(dataSet->did, device_info.did, DID_LEN);

    param = (dataSet_repeat_param_t *)ry_malloc(sizeof(dataSet_repeat_param_t));
    if(param == NULL){
        LOG_DEBUG("[%s] malloc failed \n", __FUNCTION__);
        goto error;
    }
    ry_memset(param, 0, sizeof(dataSet_repeat_param_t));
    
    param->data[0] = (DataRecord *)ry_malloc(sizeof(DataRecord));
    if(param->data[0] == NULL){
        goto error;
    }
    param->len[0] = pack_statistic_data_record((DataRecord *)param->data[0], day);
    
    
    dataSet->records.arg = param;
    dataSet->records.funcs.encode = encode_repeated_record;

    if(param->len[0] != 0){
        //LOG_DEBUG("statistics size is %d\n", param->len[0]);
        param->num = 1;
    }
    

    LOG_DEBUG("[%s]---record count %d\n", __FUNCTION__, param->num);

    if((param->num == 0)){
        FREE_PTR(param->data[0]);
        FREE_PTR(dataSet);
        FREE_PTR(param);

    }

    
    //FREE_PTR(param);
    return dataSet;

error:
    FREE_PTR(dataSet);
    FREE_PTR(param);
    return NULL;
}









