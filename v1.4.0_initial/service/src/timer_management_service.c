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
#include "window_management_service.h"
#include "timer_management_service.h"
#include "data_management_service.h"
#include "device_management_service.h"
#include "card_management_service.h"
#include "string.h"
#include "Notification.pb.h"
#include "rbp.pb.h"
#include "rbp.h"
#include "ryos.h"
#include "ry_utils.h"
#include "ry_hal_inc.h"
#include "ryos_timer.h"
#include "pb.h"
#include "nanopb_common.h"
#include "scheduler.h"
#include "Location.pb.h"
#include "Sport.pb.h"
//#include "Health.pb.h"
#include "DataUpload.pb.h"
#include "sensor_alg.h"
#include "algorithm.h"
#include "ry_ble.h"
#include "board_control.h"

#include "am_hal_rtc.h"
#include "ry_hal_rtc.h"
#include "app_pm.h"
#include "led.h"
#include "hrm.h"
#include "ry_hal_wdt.h"
#include "ry_nfc.h"
#include "ry_ble.h"
#include "gui.h"
#include "ry_statistics.h"
#include "r_xfer.h"
#include "board_control.h"
#include "touch.h"
#include "am_devices_CY8C4014.h"
#include "task.h"
#include "ancs_api.h"
#include "mibeacon.h"
#include "surface_management_service.h"
#include "activity_alarm_ring.h"

#include <stdio.h>

extern ryos_thread_t *wms_thread_handle;
extern ryos_thread_t *gui_thread3_handle;
extern ryos_thread_t *sensor_alg_thread_handle;
extern ryos_thread_t *sched_thread_handle;
// extern ryos_thread_t *hrm_thread_handle;
extern ryos_thread_t *ry_ble_thread_handle;
extern ryos_thread_t *ry_ble_tx_thread_handle;
extern ryos_thread_t *ry_nfc_thread_handle;
extern ryos_thread_t *data_service_handle;

extern ryos_thread_t *ry_ble_ctrl_thread_handle;
extern ryos_thread_t *touch_thread_handle;
extern ryos_thread_t *hrm_detect_thread_handle;
extern ryos_thread_t *dc_in_thread_handle;
extern ryos_thread_t *ble_thread_handle;
extern ryos_thread_t *cms_thread_handle;

/*********************************************************************
 * CONSTANTS
 */ 


/*********************************************************************
 * TYPES
 */ 
/**
 * 
 */



/*********************************************************************
 * VARIABLES
 */ 

AlarmClockList clock_list = {0};
extern am_hal_rtc_time_t hal_time;

/*
 * @brief TMS Thread handle
 */
ryos_thread_t *tms_thread_handle;

/*
 * @brief Context control block.
 */
tms_ctx_t tms_ctx_v;

battery_info_t batt_v;

static u32_t management_timer_id;
static u32_t wear_detect_timer_id;
ry_queue_t*  ry_tms_monitorQ;

/*********************************************************************
 * FUNCTIONS
 */ 
#define IS_PERIODIC_TASK_EXIST(p)    ((p->handler) && (((u32_t)p->handler)!=0xFFFFFFFF))
#define IS_ALARM_EMPTY(p)            (p->id == 0xFFFFFFFF)
void log_statistics_info(void);


ry_sts_t alarm_tbl_reset(void)
{
    ry_sts_t status;

    LOG_DEBUG("[%s]\r\n", __FUNCTION__);

    ry_memset((u8_t*)&tms_ctx_v.alarmTbl, 0xFF, sizeof(tms_alarm_tbl_store_t));
    tms_ctx_v.alarmTbl.alarm_tbl.curNum = 0;
    
    status = ry_hal_flash_write(FLASH_ADDR_ALARM_TABLE, (u8_t*)&tms_ctx_v.alarmTbl, sizeof(tms_alarm_tbl_store_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_TMS, RY_ERR_PARA_SAVE);
    }
    return RY_SUCC;
}


ry_sts_t alarm_tbl_save(void)
{
    ry_sts_t status;

	tms_ctx_v.alarmTbl.magic = ALARM_STORE_MAGIC;
	tms_ctx_v.alarmTbl.version = ALARM_STORE_VERSION;

    status = ry_hal_flash_write(FLASH_ADDR_ALARM_TABLE, (u8_t*)&tms_ctx_v.alarmTbl, sizeof(tms_alarm_tbl_store_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_TMS, RY_ERR_PARA_SAVE);
    }

    tms_ctx_v.last_alarm_set_time = ry_hal_clock_time();
    return RY_SUCC;
}

void alarm_restore(void)
{
	u8_t ii = 0;
    u8_t num = 0;

	tms_alarm_tbl_old_t   alarmTblOld;
	tms_alarm_tbl_store_t alarmTblNew;

	ry_hal_flash_read(FLASH_ADDR_ALARM_TABLE, (u8_t*)&alarmTblNew, sizeof(tms_alarm_tbl_store_t));
	
    if (alarmTblNew.magic != ALARM_STORE_MAGIC){
		ry_memset(&alarmTblNew, 0xFF, sizeof(tms_alarm_tbl_store_t));
		alarmTblNew.magic = ALARM_STORE_MAGIC;
		alarmTblNew.version = ALARM_STORE_VERSION;
		ry_hal_flash_read(FLASH_ADDR_ALARM_TABLE, (u8_t*)&alarmTblOld, sizeof(tms_alarm_tbl_old_t));
		if (alarmTblOld.curNum > MAX_ALARM_NUM) {
			alarmTblNew.alarm_tbl.curNum = 0;
			if (RY_SUCC != ry_hal_flash_write(FLASH_ADDR_ALARM_TABLE, (u8_t*)&alarmTblNew, sizeof(tms_alarm_tbl_store_t))) {
				LOG_ERROR("alarm restore failure!\r\n");
			}
			return;
		}else {
			alarmTblNew.alarm_tbl.curNum = alarmTblOld.curNum;
			for(ii=0; ii<alarmTblOld.curNum; ii++) {
				if((alarmTblOld.alarms[ii].hour > 23)  || (alarmTblOld.alarms[ii].minute > 59)) {
					continue;                       //skip that abnormal alarm
				}                
				alarmTblNew.alarm_tbl.alarms[num].enable     = alarmTblOld.alarms[ii].enable;
				alarmTblNew.alarm_tbl.alarms[num].hour       = alarmTblOld.alarms[ii].hour;
				alarmTblNew.alarm_tbl.alarms[num].id         = alarmTblOld.alarms[ii].id;
				alarmTblNew.alarm_tbl.alarms[num].minute     = alarmTblOld.alarms[ii].minute;
				alarmTblNew.alarm_tbl.alarms[num].ringTimes  = 0;
				alarmTblNew.alarm_tbl.alarms[num].snoozeMode = 0;
				if((tms_ctx_v.systemTime.hour*60 + tms_ctx_v.systemTime.minute) < (alarmTblOld.alarms[ii].hour*60 + alarmTblOld.alarms[ii].minute)) {
					alarmTblNew.alarm_tbl.alarms[num].ringFlag   = 1;     //set snooze alarm time after systime, should ring
				}else {
					alarmTblNew.alarm_tbl.alarms[num].ringFlag   = 0;
				}
				memcpy(alarmTblNew.alarm_tbl.alarms[num].repeatType, alarmTblOld.alarms[ii].repeatType, MAX_ALARM_REPEAT_TYPE_NUM);
				memcpy(alarmTblNew.alarm_tbl.alarms[num].tag, alarmTblOld.alarms[ii].tag, 25);
                num ++;
			}
            alarmTblNew.alarm_tbl.curNum = num;
			if (RY_SUCC != ry_hal_flash_write(FLASH_ADDR_ALARM_TABLE, (u8_t*)&alarmTblNew, sizeof(tms_alarm_tbl_store_t))) {
				LOG_ERROR("alarm restore failure!\r\n");
			}
		}
	}else {
		return;
	}
}

ry_sts_t alarm_tbl_restore(void)
{
    ry_hal_flash_read(FLASH_ADDR_ALARM_TABLE, (u8_t*)&tms_ctx_v.alarmTbl, sizeof(tms_alarm_tbl_store_t));

    if (tms_ctx_v.alarmTbl.alarm_tbl.curNum <= 0 || tms_ctx_v.alarmTbl.alarm_tbl.curNum > MAX_ALARM_NUM) {
        // Using default
        ry_memset((u8_t*)&tms_ctx_v.alarmTbl, 0xFF, sizeof(tms_alarm_tbl_store_t));
        tms_ctx_v.alarmTbl.alarm_tbl.curNum = 0;
    }

    return RY_SUCC;
}

tms_alarm_t* alarm_search(int id)
{
    u8_t key[4] = {0};
    ry_memcpy(key, (u8_t*)&id, 4);
    return (tms_alarm_t*)ry_tbl_search(MAX_ALARM_NUM, 
                            sizeof(tms_alarm_t),
                            (u8_t*)&tms_ctx_v.alarmTbl.alarm_tbl.alarms,
                            0,
                            4,
                            key);
}

void alarm_set_enable(int id, int enable)
{
    tms_alarm_t *p = alarm_search(id);
    if (p) {
        p->enable = enable;
        alarm_tbl_save();
    }
}


ry_sts_t alarm_add(int id, char* tag, int tagLen, u8_t hour, u8_t min, bool enable, u8_t* repeatType, int repeatLen, bool sooneMode)
{
    tms_alarm_t newAlarm;
    ry_sts_t status;
    int len = strlen(tag);

    if (len > MAX_ALARM_TAG_LEN) {
        return RY_SET_STS_VAL(RY_CID_TMS, RY_ERR_INVALIC_PARA);
    }

    newAlarm.id     = id;
    newAlarm.hour   = hour;
    newAlarm.minute = min;
    newAlarm.enable = enable;
	newAlarm.snoozeMode = sooneMode;
	newAlarm.ringTimes = 0;
	if((tms_ctx_v.systemTime.hour*60 + tms_ctx_v.systemTime.minute) < (hour*60 + min)) {
		newAlarm.ringFlag = 1;
	}else {
		newAlarm.ringFlag = 0;
	}
    ry_memset(newAlarm.repeatType, 0xFF, MAX_ALARM_REPEAT_TYPE_NUM);
    ry_memcpy(newAlarm.repeatType, repeatType, repeatLen);
    strcpy(newAlarm.tag, tag);
    
    LOG_DEBUG("Alarm snoozeMode = %d\r\n",newAlarm.snoozeMode);
    status = ry_tbl_add((u8_t*)&tms_ctx_v.alarmTbl.alarm_tbl, 
                 MAX_ALARM_NUM,
                 sizeof(tms_alarm_t),
                 (u8_t*)&newAlarm,
                 4, 
                 0);


    if (status == RY_SUCC) {
        //arch_event_rule_save((u8 *)&mible_evtRuleTbl, sizeof(mible_evtRuleTbl_t));
        LOG_DEBUG("Alarm add success.\r\n");
    } else {
        LOG_WARN("Alarm add fail, error code = %x\r\n", status);
    }

    return status;
}

ry_sts_t alarm_del(int id)
{
    ry_sts_t ret;
    u8_t key[4]= {0};
    
    ry_memcpy(key, (u8_t*)&id, 4);
    ret = ry_tbl_del((u8*)&tms_ctx_v.alarmTbl.alarm_tbl,
                MAX_ALARM_NUM,
                sizeof(tms_alarm_t),
                key,
                4,
                0);


    if (ret != RY_SUCC) {
        LOG_WARN("Alarm del fail, error code = 0x02%x\r\n", ret);
    }

    return ret;
}

void alarm_list_get(AlarmClockList* pList)
{
    int i;
    ry_memset((u8_t*)pList, 0, sizeof(AlarmClockList));
    
    /* Encode result */
    int num = 0;
    tms_alarm_t* p;
    pList->items_count = tms_ctx_v.alarmTbl.alarm_tbl.curNum;
    for (i = 0; num < pList->items_count; i++) {
        if(i >=  MAX_ALARM_NUM){
            break;
        }
        p = &tms_ctx_v.alarmTbl.alarm_tbl.alarms[i];
        if (IS_ALARM_EMPTY(p)) {
            continue;
        }
        
        pList->items[num].id = p->id;
        pList->items[num].hour = p->hour;
        pList->items[num].minute = p->minute;
        pList->items[num].enable = p->enable;
        strcpy(pList->items[num].tag, p->tag);
        ry_memcpy(pList->items[num].repeats, p->repeatType, MAX_ALARM_REPEAT_TYPE_NUM);

        num++;
    }
    pList->items_count = num;
    tms_ctx_v.alarmTbl.alarm_tbl.curNum = num;
}


void tms_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x, heap:%d, minHeap:%d\r\n", \
            __FUNCTION__, (char*)arg, status, ryos_get_free_heap(), ryos_get_min_available_heap_size());
    } else {
        LOG_DEBUG("[%s] %s, error status: %x, heap:%d, minHeap:%d\r\n", \
            __FUNCTION__, (char*)arg, status, ryos_get_free_heap(), ryos_get_min_available_heap_size());
    }
}

void set_alarm_clock(u8_t * data , u32_t len)
{
    int status;
    tms_alarm_t *p;
    AlarmClockItem * alarm_item = (AlarmClockItem *)ry_malloc(sizeof(AlarmClockItem));
    ry_sts_t ry_sts = RY_SUCC;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    
    status = pb_decode(&stream_i, AlarmClockItem_fields, alarm_item);
    if(!status){
        LOG_ERROR("[%s] decoding failed.\r\n", __FUNCTION__);
    }
	
	ry_hal_flash_read(FLASH_ADDR_ALARM_TABLE, (u8_t*)&tms_ctx_v.alarmTbl, sizeof(tms_alarm_tbl_store_t));
    /* Check if the alarm is already exist */
    p = alarm_search(alarm_item->id);
    if (p) {
        /* Exist, update value */
        p->hour   = alarm_item->hour;
        p->minute = alarm_item->minute;
        p->enable = alarm_item->enable;
        p->ringTimes = 0;
		if(alarm_item->has_snoozeMode){
			p->snoozeMode = alarm_item->snoozeMode;
		}else {
			p->snoozeMode = 0;
		}
        ry_memset(p->tag, 0, MAX_ALARM_TAG_LEN);
        strcpy(p->tag, alarm_item->tag);
        ry_memset(p->repeatType, 0xFF, MAX_ALARM_REPEAT_TYPE_NUM);
        ry_memcpy(p->repeatType, alarm_item->repeats, alarm_item->repeats_count);
    } else {
        /* Not exist, Add a new */
        ry_sts = alarm_add(alarm_item->id, alarm_item->tag, strlen(alarm_item->tag), 
            alarm_item->hour, alarm_item->minute, alarm_item->enable, 
            alarm_item->repeats, alarm_item->repeats_count, alarm_item->snoozeMode);
    }

	//LOG_DEBUG("\r\n p->snoozeMode=%d  alarm_item->snoozeMode=%d \r\n",p->snoozeMode,alarm_item->snoozeMode);
    ry_free((u8_t*)alarm_item);

    if (ry_sts == RY_SUCC) {
        alarm_tbl_save();
        //rbp_send_resp(CMD_DEV_ALARM_CLOCK_SET, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);

        ry_sts = ble_send_response(CMD_DEV_ALARM_CLOCK_SET, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                tms_ble_send_callback, (void*)__FUNCTION__);
    } else {
        //rbp_send_resp(CMD_DEV_ALARM_CLOCK_SET, (u32_t)ry_sts, NULL, 0, 1);

        ry_sts = ble_send_response(CMD_DEV_ALARM_CLOCK_SET, (u32_t)ry_sts, NULL, 0, 1,
                tms_ble_send_callback, (void*)__FUNCTION__);
    }

    LOG_DEBUG("[%s]curNum:%d, hour:%d, min:%d, en:%d,  ringTimes:%d   snonzeMode:%d\r\n", \
            __FUNCTION__, tms_ctx_v.alarmTbl.alarm_tbl.curNum, p->hour, p->minute, p->enable,p->ringTimes,p->snoozeMode);

    if (ry_sts != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }
}



void delete_alarm_clock(u8_t * data , u32_t len)
{
    int status,i ,j;
    ry_sts_t ry_sts = RY_SUCC;
    AlarmClockDeleteItem delete_item = {0};

    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    
    status = pb_decode(&stream_i, AlarmClockDeleteItem_fields, &delete_item);
    if(!status){
        LOG_ERROR("[%s] decoding failed.\r\n", __FUNCTION__);
    }

    ry_sts = alarm_del(delete_item.id);

    if (ry_sts == RY_SUCC) {
        alarm_tbl_save();
        //rbp_send_resp(CMD_DEV_ALARM_CLOCK_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_ALARM_CLOCK_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                tms_ble_send_callback, (void*)__FUNCTION__);
    } else {
        //rbp_send_resp(CMD_DEV_ALARM_CLOCK_DELETE, (u32_t)ry_sts, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_ALARM_CLOCK_DELETE, (u32_t)ry_sts, NULL, 0, 1,
                tms_ble_send_callback, (void*)__FUNCTION__);
    }

    if (ry_sts != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }
}


void get_alarm_clock(void)
{
    int i, j;
    int status;
    ry_sts_t ret;
    AlarmClockList * result = (AlarmClockList *)ry_malloc(sizeof(AlarmClockList));
    if (!result) {
        LOG_ERROR("[get_alarm_clock] No mem.\r\n");
        RY_ASSERT(0);
    }

    ry_memset((u8_t*)result, 0, sizeof(AlarmClockList));
    
    /* Encode result */
    int num = 0;
    tms_alarm_t* p;
    result->items_count = tms_ctx_v.alarmTbl.alarm_tbl.curNum;
    for (i = 0; num < result->items_count; i++) {
        if(i >=  MAX_ALARM_NUM){
            break;
        }
        p = &tms_ctx_v.alarmTbl.alarm_tbl.alarms[i];
        if (IS_ALARM_EMPTY(p)) {
            continue;
        }
        
        result->items[num].id = p->id;
        result->items[num].hour = p->hour;
        result->items[num].minute = p->minute;
        result->items[num].enable = p->enable;
        strcpy(result->items[num].tag, p->tag);
		result->items[num].has_snoozeMode = 1;
		result->items[num].snoozeMode = p->snoozeMode;
        ry_memcpy(result->items[num].repeats, p->repeatType, MAX_ALARM_REPEAT_TYPE_NUM);

        num++;
    }

    result->items_count = num;
    tms_ctx_v.alarmTbl.alarm_tbl.curNum = num;

    for(j = 0; j < result->items_count; j++){
        for(i = 0; i < MAX_ALARM_REPEAT_TYPE_NUM; i++){
            if(result->items[j].repeats[i] == 0xFF){
                break;
            }
        }
        result->items[j].repeats_count = i;
    }
    

    u8_t* buf = ry_malloc(sizeof(AlarmClockList) + 20);

    size_t message_length;
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(AlarmClockList) + 20);

    status = pb_encode(&stream, AlarmClockList_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[get_alarm_clock]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        //rbp_send_resp(CMD_DEV_ALARM_CLOCK_GET_LIST, RBP_RESP_CODE_ENCODE_FAIL, buf, message_length, 1);
        ret = ble_send_response(CMD_DEV_ALARM_CLOCK_GET_LIST, RBP_RESP_CODE_ENCODE_FAIL, buf, message_length, 1,
                tms_ble_send_callback, (void*)__FUNCTION__);
        goto exit;
    }

    //rbp_send_resp(CMD_DEV_ALARM_CLOCK_GET_LIST, RBP_RESP_CODE_SUCCESS, buf, message_length, 1);
    ret = ble_send_response(CMD_DEV_ALARM_CLOCK_GET_LIST, RBP_RESP_CODE_SUCCESS, buf, message_length, 1,
                tms_ble_send_callback, (void*)__FUNCTION__);

    if (ret != RY_SUCC) {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, ret);
    }
    

exit:
    ry_free(buf);
    ry_free((u8_t*)result);
}

uint32_t sys_get_battery(void)
{
	return tms_ctx_v.batt_percent;
}

uint32_t battery_low_alert_en_get(void)
{
    uint32_t alert_en = (ry_system_initial_status() != 0) && \
                        !charge_cable_is_input() &&          \
                        !alg_status_is_in_sleep() &&         \
                        (get_device_session() != DEV_SESSION_RUNNING) &&\
                        !wms_is_in_break_activity();
    return alert_en;
}

void battery_low_processing(uint32_t level)
{
    static uint8_t last_level;
    switch (level){
        case BATT_LOW_PERCENT_NORMAL:
            set_device_powerstate(DEV_PWR_STATE_NORMAL);
            //should not send power status update msg, because the charging operation would send msg
            break;
            
        case BATT_LOW_PERCENT_0:
            if (last_level != BATT_LOW_PERCENT_0){
                if ((ry_system_initial_status() != 0) && !charge_cable_is_input() && !alg_status_is_in_sleep()){
                    // ry_sched_msg_send(IPC_MSG_TYPE_ALERT_BATTERY_LOW, 0, NULL);
                    if(0 == current_time_is_dnd_status()){
                        //motar_light_loop_working(3);
                        motar_strong_linear_twice(300);
                    }
                }
            }
            else{
                batt_v.off_level = 1;                                
            }    
            set_device_powerstate(DEV_PWR_STATE_OFF);
            break;

        case BATT_LOW_PERCENT_5:
            set_device_powerstate(DEV_PWR_STATE_LOW_POWER);
            if (last_level != BATT_LOW_PERCENT_5){
                if (battery_low_alert_en_get()){
                    ry_sched_msg_send(IPC_MSG_TYPE_ALERT_BATTERY_LOW, 0, NULL);
                    if(0 == current_time_is_dnd_status()){
                        //motar_light_loop_working(3);
                        motar_strong_linear_twice(300);
                    }
                }
            } 
            break;

        case BATT_LOW_PERCENT_10:
            set_device_powerstate(DEV_PWR_STATE_LOW_POWER);
            if (last_level != BATT_LOW_PERCENT_10){
                if (battery_low_alert_en_get()){
                    ry_sched_msg_send(IPC_MSG_TYPE_ALERT_BATTERY_LOW, 0, NULL);
                    if(0 == current_time_is_dnd_status()){
                        //motar_light_loop_working(3);
                        motar_strong_linear_twice(300);
                    }
                }
            }
            break;

        case BATT_LOW_PERCENT_15:
            set_device_powerstate(DEV_PWR_STATE_LOW_POWER);
            if (last_level != BATT_LOW_PERCENT_15){
                if (battery_low_alert_en_get()){
                    ry_sched_msg_send(IPC_MSG_TYPE_ALERT_BATTERY_LOW, 0, NULL);
                    if(0 == current_time_is_dnd_status()){
                        //motar_light_loop_working(3);
                        motar_strong_linear_twice(300);
                    }
                }
            }
            break;

        case BATT_LOW_PERCENT_20:
            set_device_powerstate(DEV_PWR_STATE_LOW_POWER);
            if (last_level != BATT_LOW_PERCENT_20){
                if (battery_low_alert_en_get()){
                    ry_sched_msg_send(IPC_MSG_TYPE_ALERT_BATTERY_LOW, 0, NULL);
                    if(0 == current_time_is_dnd_status()){
                        //motar_light_loop_working(3);
                        motar_strong_linear_twice(300);
                    }
                }
            }
            break;

        default :
            break;
    }
    last_level = level;
}

void battery_halt_set(uint8_t halt_en)
{
    batt_v.halt_en = halt_en;
}

void periodic_battery_task(void* arg)
{
    if (batt_v.halt_en ||
        (ry_nfc_get_state() == RY_NFC_STATE_READER_POLLING) ||
        (get_device_session() == DEV_SESSION_CARD)) {
        return;
    }

    tms_ctx_v.batt_percent = sys_get_battery_percent(BATT_DETECT_DIRECTED);
    if (tms_ctx_v.batt_percent_last_log >= (tms_ctx_v.batt_percent + 2)){
        //log_statistics_info();
    }
    else if (tms_ctx_v.batt_percent_last_log == 0){      //initial tms_ctx_v.batt_percent_last_log
        tms_ctx_v.batt_percent_last_log = sys_get_battery();
    }
    
    // LOG_DEBUG("[periodic_battery_task] batt_percent:%d, batt_percent_last_log:%d\r\n", \
            tms_ctx_v.batt_percent, tms_ctx_v.batt_percent_last_log);
            
    batt_v.off_level = 0;

    if ((dev_mfg_state_get() > DEV_MFG_STATE_FINAL) &&  ((u8_t)dev_mfg_state_get() != 0xFF)) {
        if (tms_ctx_v.batt_percent <= 0) {
            battery_low_processing(BATT_LOW_PERCENT_0);
        } 
        else if (tms_ctx_v.batt_percent <= 5) {
            battery_low_processing(BATT_LOW_PERCENT_5);
        } 
        else if (tms_ctx_v.batt_percent <= 10) {
            battery_low_processing(BATT_LOW_PERCENT_10);
        } 
        else if (tms_ctx_v.batt_percent <= 15) {
            battery_low_processing(BATT_LOW_PERCENT_15);
        } 
        else if (tms_ctx_v.batt_percent <= 20) {
            battery_low_processing(BATT_LOW_PERCENT_20);
        }
        else {
            battery_low_processing(BATT_LOW_PERCENT_NORMAL);
        }
    } else {
        battery_low_processing(BATT_LOW_PERCENT_NORMAL);
    }

    if (batt_v.off_level){
        if (batt_v.low_protect == 0){
            periodic_task_set_enable(periodic_touch_task, 0);
            periodic_task_set_enable(periodic_hr_task, 0);
            set_raise_to_wake_close();
            data_service_set_enable(0);
            motar_stop();
            ry_motar_enable_set(0);

            pcf_func_touch();  
            ry_hal_rtc_int_disable();
            ryos_thread_suspend(sensor_alg_thread_handle);
            ryos_thread_suspend(gui_thread3_handle);
            pcf_func_hrm();
            pcf_func_gsensor();
            ryos_thread_suspend(wms_thread_handle);            
            pcf_func_oled();
            ry_led_onoff(0);
            batt_v.low_protect = 1;
            LOG_DEBUG("[low_power_protect_enter]: batt_v.off_level: %d\n", batt_v.off_level);
        }
    }
    else{
        if (batt_v.low_protect){
            batt_v.low_protect = 0;      
            LOG_DEBUG("[low_power_restore]: batt_v.off_level: %d\n", batt_v.off_level);
            add_reset_count(BAT_RESTART_COUNT);
            ry_system_reset();
        }
    }
    // LOG_DEBUG("[periodic_battery_task] tms_ctx_v.batt_percent: %d\r\n", tms_ctx_v.batt_percent);
}

void periodic_touch_task(void* arg)
{
    if ((wms_touch_is_busy() == 0)) {// && (get_touch_state() == PM_ST_POWER_START)){
        touch_reset();
    }
}

void hrm_auto_sample_timeout_handler(void)
{
    //LOG_INFO("Auto HRM detect stop");
    ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_STOP, NULL);
}

void hrm_wear_detect_timeout_handler(void)
{
    u32_t weared = 0;
    if (get_hrm_weared_status() != WEAR_DET_SUCC){ 
        ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_HRM_CHECK_WAKEUP, 0, NULL);
    }
    else {
        weared = 1;
    }

    ry_hrm_msg_send(HRM_MSG_CMD_WEAR_DETECT_SAMPLE_STOP, NULL);    
    LOG_DEBUG("hrm_wear_detect_timeout_handler, weared: %d\r\n\r\n", weared);
}

void hrm_wear_detect_and_hrmLog_timeout_handler(void)
{
    u32_t weared = 0;
    if (get_hrm_weared_status() != WEAR_DET_SUCC){ 
        ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_HRM_CHECK_WAKEUP, 0, NULL);
    }
    else{
        weared = 1;
    }

    u8_t hrm = 255;
    alg_get_hrm_awake_cnt(&hrm);
    if (hrm > 120){
        LOG_INFO("slp_hrm weared: %d, hrm:%d\r\n", weared, hrm);
    }
    ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_STOP, NULL);
    LOG_DEBUG("hrm_wear_detect_timeout_handler, weared: %d\r\n\r\n", weared);
}

void periodic_sleep_hr_weared_detect_task(void* arg)
{
    if ((!charge_cable_is_input()) && (get_device_powerstate() != DEV_PWR_STATE_OFF)){
        if (alg_status_is_in_sleep() && (alg_get_work_mode() == ALGO_IMU)){
            ry_hrm_msg_send(HRM_MSG_CMD_WEAR_DETECT_SAMPLE_START, NULL);
            if (wear_detect_timer_id == 0){
                ry_timer_parm timer_para;
                /* Create the timer */
                timer_para.timeout_CB = (void (*)(void*))hrm_wear_detect_timeout_handler;
                timer_para.flag = 0;
                timer_para.data = NULL;
                timer_para.tick = 1;
                timer_para.name = "wear_detect_timer";
                wear_detect_timer_id = ry_timer_create(&timer_para);
            }

            if ((tms_ctx_v.sleep_hrm_log_count >= 5) && (is_auto_heart_rate_open() == false)){
                tms_ctx_v.sleep_hrm_log_count = 0;
                ry_timer_start(wear_detect_timer_id, 15000, (ry_timer_cb_t)hrm_wear_detect_and_hrmLog_timeout_handler, NULL);
            }
            else{
                ry_timer_start(wear_detect_timer_id, 1000, (ry_timer_cb_t)hrm_wear_detect_timeout_handler, NULL);
            }

            tms_ctx_v.sleep_hrm_log_count ++;
            LOG_DEBUG("wear_detect_timer start, sleep_hrm_log_count:%d\r\n", tms_ctx_v.sleep_hrm_log_count);
        }
    }
}


void periodic_hr_task(void* arg)
{
	static u32_t index = 0; 
	u32_t heart_rate_time = 0;
	if(is_auto_heart_rate_open() == true)
	{
		heart_rate_time = get_auto_heart_rate_time();
		
        LOG_INFO("heartTask interval=%d\r\n", heart_rate_time);
		
        if(heart_rate_time < AUTO_HR_DETECT_PERIOID)
		{
			heart_rate_time = AUTO_HR_DETECT_PERIOID;
		}
		else if(heart_rate_time == 0xffffffff)
		{
			heart_rate_time = 600; //10 minute
		}
		index++;
		
		if(heart_rate_time == (index*AUTO_HR_DETECT_PERIOID)){
			index = 0;
			//exclude device charging status // 
			if ((!charge_cable_is_input()) && (get_device_powerstate() != DEV_PWR_STATE_OFF))
			{
				// index == 0;
				/* Start auto heart detect */
				//LOG_INFO("Auto HRM detect start: interval %d", heart_rate_time);
				ry_hrm_msg_send(HRM_MSG_CMD_AUTO_SAMPLE_START, NULL);

				if (management_timer_id == 0){
					ry_timer_parm timer_para;
					/* Create the timer */
					timer_para.timeout_CB = (void (*)(void*))hrm_auto_sample_timeout_handler;
					timer_para.flag = 0;
					timer_para.data = NULL;
					timer_para.tick = 1;
					timer_para.name = "management_timer";
					management_timer_id = ry_timer_create(&timer_para);
				}
				ry_timer_start(management_timer_id, 8000, (ry_timer_cb_t)hrm_auto_sample_timeout_handler, NULL);
			}
		}
	}
	else{
		index = 0;
	}
	//end levi
}

void log_task_stack_waterMark(void)
{
    u16_t tms_stk = uxTaskGetStackHighWaterMark(tms_thread_handle);
    u16_t data_stk = uxTaskGetStackHighWaterMark(data_service_handle);
    u16_t alg_stk = uxTaskGetStackHighWaterMark(sensor_alg_thread_handle);
    u16_t wms_stk = uxTaskGetStackHighWaterMark(wms_thread_handle);
    u16_t gui_stk = uxTaskGetStackHighWaterMark(gui_thread3_handle);
    u16_t sched_stk = uxTaskGetStackHighWaterMark(sched_thread_handle);
//    u16_t hrm_stk = uxTaskGetStackHighWaterMark(hrm_thread_handle);
    u16_t hrm_detect_stk = uxTaskGetStackHighWaterMark(hrm_detect_thread_handle);

    u16_t ble_rx_stk = uxTaskGetStackHighWaterMark(ry_ble_thread_handle);
    u16_t ble_tx_stk = uxTaskGetStackHighWaterMark(ry_ble_tx_thread_handle);
    u16_t ble_ctrl_stk = uxTaskGetStackHighWaterMark(ry_ble_ctrl_thread_handle);
    u16_t ble_stk = uxTaskGetStackHighWaterMark(ble_thread_handle);
    u16_t nfc_stk = uxTaskGetStackHighWaterMark(ry_nfc_thread_handle);
    u16_t xferStk = uxTaskGetStackHighWaterMark(r_xfer_get_thread_handle());
    u16_t touch_stk = uxTaskGetStackHighWaterMark(touch_thread_handle);
    u16_t dc_in_stk = uxTaskGetStackHighWaterMark(dc_in_thread_handle);

    LOG_INFO("[stkA]:tms: %3d, dat:%3d, alg:%3d, wms:%3d, gui:%3d, sced:%3d, hrmD:%3d\r\n", \
              tms_stk, data_stk, alg_stk, wms_stk, gui_stk, sched_stk, hrm_detect_stk);
    LOG_INFO("[stkB]:bRx: %3d, bTx:%3d, bCt:%3d, ble:%3d, rxf:%3d, toch:%3d, dcIn:%3d, nfc:%3d\r\n", \
              ble_rx_stk, ble_tx_stk, ble_ctrl_stk, ble_stk, xferStk, touch_stk, dc_in_stk, nfc_stk) ;
}


void log_statistics_info(void)
{
    extern ry_device_setting_t device_setting;
    u8_t type;
    tms_ctx_v.batt_percent_last_log = sys_get_battery();
    //LOG_INFO("[logS], heap:%d, mHeap:%d, batt:%d, battAdc:%d, resetCnt:%d, step:%d, sports:%d, sptTims:%d, waise:%d, home:%d, menu:%d, oledT:%d,hrmT:%d\r\n", \
    //          ryos_get_free_heap(),  \
    //          ryos_get_min_available_heap_size(), \
    //          tms_ctx_v.batt_percent_last_log,    \
    //          sys_get_battery_adc(), \
    //          device_setting.dev_restart_count[DEV_RESTART_COUNT],\
    //          alg_get_step_today(),  \
    //          alg_get_sports_mode(), \
    //          alg_get_sports_times(),\
    //          DEV_STATISTICS_VALUE(raise_wake_count),\
    //          DEV_STATISTICS_VALUE(home_back_count), \
    //          DEV_STATISTICS_VALUE(menu_count),      \
    //          get_gui_on_time(),    \
    //          get_hrm_on_time());

    //LOG_INFO("[logS] tms_tick: %d, interval: %d, %02d:%02d:%02d", tms_ctx_v.tick, tms_ctx_v.interval_ms,
    //        tms_ctx_v.systemTime.hour, tms_ctx_v.systemTime.minute, tms_ctx_v.systemTime.second);
    
    LOG_INFO("[logS], heap:%d, mHeap:%d, batt:%d, battAdc:%d, resetCnt:%d, step:%d, lSit:%d\r\n", \
              ryos_get_free_heap(),  \
              ryos_get_min_available_heap_size(), \
              tms_ctx_v.batt_percent_last_log,    \
              sys_get_battery_adc(), \
              device_setting.dev_restart_count[DEV_RESTART_COUNT],\
              alg_get_step_today(), DEV_STATISTICS_VALUE(long_sit_count));

    
    type = card_get_current_card();
    uint32_t alg_st1 = 0, alg_st2 = 0;
    alg_thread_get_status(&alg_st1, &alg_st2);
    LOG_INFO("[logS], r_xfer:0x%x, ble:0x%x, nfc:0x%x, ancs:0x%x, curCard:%d, algS1:%d, algS2:0x%08x\r\n",
        r_xfer_get_diagnostic(),
        ry_ble_get_state(),
        ry_nfc_get_state(),
        ancsGetDiagnosisStatus(),
        type, alg_st1, alg_st2);
}

void periodic_app_data_save_task(void* arg)
{
    system_app_data_save();
    // log_task_stack_waterMark();
    mibeacon_on_save_frame_counter();
    log_statistics_info();
    surface_on_save_request();
}

periodic_task_t* periodic_task_search(periodic_taskHandler_t task)
{
    // u8_t key[4] = {0};
    // ry_memcpy(key, (void*)task, sizeof(periodic_taskHandler_t));
    return (periodic_task_t*)ry_tbl_search(MAX_PERIODIC_TASK_NUM, 
               sizeof(periodic_task_t),
               (u8_t*)&tms_ctx_v.ptt.tasks,
               0,
               4,
               (u8_t*)&task);
}


/**
 * @brief   Periodic task running
 *
 * @param   interval - Unit as 1 second.
 * @param   task - Periodic task handler.
 *
 * @return  None
 */
ry_sts_t periodic_task_add(int interval, periodic_taskHandler_t task, void* usrdata, bool atonce)
{
    periodic_task_t newTask;
    ry_sts_t status;

    if (task == NULL) {
        return RY_SET_STS_VAL(RY_CID_TMS, RY_ERR_INVALIC_PARA);
    }
    //ryos_mutex_lock(r_xfer_v.mutex);

    ry_memset((u8_t*)&newTask, 0, sizeof(periodic_task_t));

    newTask.interval = interval;
    newTask.handler  = task;
    newTask.usrdata  = usrdata;
    newTask.lastTick = tms_ctx_v.tick;

   
    status = ry_tbl_add((u8_t*)&tms_ctx_v.ptt, 
                 MAX_PERIODIC_TASK_NUM,
                 sizeof(periodic_task_t),
                 (u8_t*)&newTask,
                 sizeof(periodic_taskHandler_t), 
                 0);


    //ryos_mutex_unlock(r_xfer_v.mutex);   

    if (status == RY_SUCC) {
        //arch_event_rule_save((u8 *)&mible_evtRuleTbl, sizeof(mible_evtRuleTbl_t));
        LOG_DEBUG("[TMS] Periodic task add success.\r\n");
        if (atonce) {
            task(usrdata);
        }
    } else {
        LOG_WARN("[TMS] Periodic task add fail, error code = %x\r\n", status);
    }


    return status;
}


void periodic_task_set_enable(periodic_taskHandler_t task, u8_t fEnable)
{
    periodic_task_t* p = periodic_task_search(task);
    if (p == NULL) {
        return;
    }

    p->enable = fEnable;
}

monitor_info_t mnt_v;

void monitor_task(void)
{
    int free_heap;
    int min_available_heap;
    volatile static uint8_t cnt_watchdog;
    ry_sts_t status = RY_SUCC;
    dev_lifecycle_t life = 0;

    mnt_v.free_heap = ryos_get_free_heap();
    free_heap = mnt_v.free_heap;
    min_available_heap = ryos_get_min_available_heap_size();
    //uint32_t alg_st1 = 0, alg_st2 = 0;
    //alg_thread_get_status(&alg_st1, &alg_st2);
    //LOG_DEBUG("[Monitor] Free Heap %d, Min Avail Heap %d, alg_st1:0x%08x, alg_st2:0x%08x\r\n", mnt_v.free_heap, min_available_heap, alg_st1, alg_st2);
    // LOG_DEBUG("[Monitor] Free Heap %d, Min Avail Heap %d\r\n", mnt_v.free_heap, min_available_heap);
    if (free_heap < 10*1024) {
        LOG_ERROR("-----Low Memory-----!!! Rebooting system..\r\n");
        system_app_data_save();
        add_reset_count(MEM_RESTART_COUNT);
        ry_system_reset();
    }

#if 0
    if(get_online_raw_data_onOff() != 0){
        u8_t * test_buf = (u8_t *)ry_malloc(1024);
        ry_memset(test_buf , 0 , 1024);
        ry_memset(test_buf , 0x31 , 1000);
        rbp_send_req(1032, test_buf, 1024, 1);
        LOG_DEBUG("RAW data upload\n");
        ry_free(test_buf);
    }
#endif

#if LOG_MONITOR_STACK
    log_task_stack_waterMark();
    
    uint8_t* str_task_list = NULL; 
    str_task_list = (uint8_t *)ry_malloc(1024);
    if(str_task_list == NULL){
        LOG_ERROR("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
    }
    else{
        vTaskList((char*)str_task_list);
        LOG_DEBUG("task_list: %s\r\n", str_task_list);
        ry_free(str_task_list);
    }
#endif

    if ((++cnt_watchdog) & 0x0f){
        goto exit;
    }

    life = get_device_lifecycle();

    /* Check all other task is alive */
#if 1
    if (ryos_thread_get_state(sensor_alg_thread_handle) < RY_THREAD_ST_SUSPENDED){
        if ((batt_v.low_protect != 1) && (life != DEV_LIFECYCLE_UNBIND)){
            monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
            p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_ALG;
            p->dataAddr = (uint32_t)&mnt_v.dog_alg;
            ry_alg_msg_send(IPC_MSG_TYPE_SYSTEM_MONITOR_ALG, sizeof(monitor_msg_t), (u8_t*)p);
            ry_free((void*)p);  
            mnt_v.dog_alg ++;
        }
    }
#endif

    if (ryos_thread_get_state(wms_thread_handle) < RY_THREAD_ST_SUSPENDED){
        if (batt_v.low_protect != 1){
            monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
            p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_WMS;
            p->dataAddr = (uint32_t)&mnt_v.dog_wms;
            wms_msg_send(IPC_MSG_TYPE_SYSTEM_MONITOR_WMS, sizeof(monitor_msg_t), (u8_t*)p);
            ry_free((void*)p);                
            mnt_v.dog_wms ++;
        }
    }

    if (ryos_thread_get_state(gui_thread3_handle) < RY_THREAD_ST_SUSPENDED){
        if (batt_v.low_protect != 1){
            monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
            p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_GUI;
            p->dataAddr = (uint32_t)&mnt_v.dog_gui;
            ry_gui_msg_send(IPC_MSG_TYPE_SYSTEM_MONITOR_GUI, sizeof(monitor_msg_t), (u8_t*)p);
            ry_free((void*)p);        
            mnt_v.dog_gui ++;
        }
    }

    if (ryos_thread_get_state(sched_thread_handle) < RY_THREAD_ST_SUSPENDED){    
        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_SCHEDULE;
        p->dataAddr = (uint32_t)&mnt_v.dog_scedule;
        ry_sched_msg_send(IPC_MSG_TYPE_SYSTEM_MONITOR_SCHEDULE, sizeof(monitor_msg_t), (u8_t*)p);
        ry_free((void*)p);
        mnt_v.dog_scedule ++;    
    }

//    if (ryos_thread_get_state(hrm_thread_handle) < RY_THREAD_ST_SUSPENDED){    
//        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
//        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_HRM;
//        p->dataAddr = (uint32_t)&mnt_v.dog_hrm;
//        ry_hrm_msg_send(HRM_MSG_CMD_MONITOR, p);
//        mnt_v.dog_hrm ++;
//    }

    if (ryos_thread_get_state(ry_ble_thread_handle) < RY_THREAD_ST_SUSPENDED){    
        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_RX;
        p->dataAddr = (uint32_t)&mnt_v.dog_ble_rx;
        const uint8_t user_data = RY_BLE_TYPE_RX_MONITOR;
        ry_ble_msg_send((u8_t*)p, sizeof(monitor_msg_t), (void*)user_data, 0);
        mnt_v.dog_ble_rx ++;
    }

    if (ryos_thread_get_state(ry_ble_tx_thread_handle) < RY_THREAD_ST_SUSPENDED){    
        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_TX;
        p->dataAddr = (uint32_t)&mnt_v.dog_ble_tx;
        const uint32_t user_data = (RY_BLE_TYPE_TX_MONITOR << 16) | RY_BLE_TYPE_TX_MONITOR;
        ry_ble_msg_send((u8_t*)p, sizeof(monitor_msg_t), (void*)&user_data, 1);
        mnt_v.dog_ble_tx ++;
    }

    if (ryos_thread_get_state(ry_nfc_thread_handle) < RY_THREAD_ST_SUSPENDED){    
        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_NFC;
        p->dataAddr = (uint32_t)&mnt_v.dog_nfc;
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_MONITOR, p);
        mnt_v.dog_nfc ++;
    }

    if (ryos_thread_get_state(cms_thread_handle) < RY_THREAD_ST_SUSPENDED){    
        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_CMS;
        p->dataAddr = (uint32_t)&mnt_v.dog_cms;
        //ry_nfc_msg_send(CMS_CMD_MONITOR, p);
        cms_msg_send(CMS_CMD_MONITOR, sizeof(monitor_msg_t), (void*)p);
        ry_free((void*)p);
        mnt_v.dog_cms ++;
    }

    if (ryos_thread_get_state(r_xfer_get_thread()) < RY_THREAD_ST_SUSPENDED){    
        monitor_msg_t *p = (monitor_msg_t*)ry_malloc(sizeof(monitor_msg_t));
        p->msgType = IPC_MSG_TYPE_SYSTEM_MONITOR_R_XFER;
        p->dataAddr = (uint32_t)&mnt_v.dog_r_xfer;
        //ry_nfc_msg_send(CMS_CMD_MONITOR, p);
        r_xfer_msg_send(R_XFER_FLAG_MONITOR, sizeof(monitor_msg_t), (void*)p);
        ry_free((void*)p);
        mnt_v.dog_r_xfer ++;
    }


    if (ry_nfc_is_wired_enable() && (get_device_session() != DEV_SESSION_CARD)) {
        mnt_v.dog_se_wired ++;
    } else {
        mnt_v.dog_se_wired = 0;
    }

    if ((mnt_v.dog_alg >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_scedule >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_wms >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_gui >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_hrm >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_ble_rx >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_cms >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_ble_tx >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_nfc >= SOFT_WATCH_DOG_THRESH) || \
        (mnt_v.dog_se_wired >= SOFT_WATCH_DOG_SE_WIRED_THRESH) || \
        (mnt_v.dog_r_xfer >= SOFT_WATCH_DOG_THRESH)\
    ){
        LOG_ERROR("[%s]soft_dog_die - dog_alg:%d, dog_schedule:%d, dog_wms:%d, dog_gui:%d, dog_r_xfer:%d, dog_ble_rx:%d, dog_ble_tx:%d, dog_nfc:%d, dog_cms:%d, dog_se_wired:%d\r\n", __FUNCTION__, \
            mnt_v.dog_alg, mnt_v.dog_scedule, mnt_v.dog_wms, mnt_v.dog_gui, mnt_v.dog_r_xfer, mnt_v.dog_ble_rx, mnt_v.dog_ble_tx, mnt_v.dog_nfc, mnt_v.dog_cms, mnt_v.dog_se_wired);        
        system_app_data_save();
        add_reset_count(PRETECT_RESTART_COUNT);
        ry_system_reset();
    }
    // LOG_DEBUG("[%s]dog_alg:%d, dog_scedule:%d, dog_wms:%d, dog_gui:%d, dog_hrm:%d, dog_ble_rx:%d, dog_ble_tx:%d, dog_nfc:%d, dog_cms:%d\r\n", __FUNCTION__, \
        mnt_v.dog_alg, mnt_v.dog_scedule, mnt_v.dog_wms, mnt_v.dog_gui, mnt_v.dog_hrm, mnt_v.dog_ble_rx, mnt_v.dog_ble_tx, mnt_v.dog_nfc, mnt_v.dog_cms);
	
exit:
    return;
}




/**
 * @brief   Get current time info
 *
 * @param   time - Pointer to get 
 *
 * @return  None
 */
void tms_get_time(ry_time_t* time)
{
    time->weekday = hal_time.ui32Weekday;
    time->year    = hal_time.ui32Year;
    time->month   = hal_time.ui32Month;
    time->day     = hal_time.ui32DayOfMonth;
    time->hour    = hal_time.ui32Hour;
    time->minute  = hal_time.ui32Minute;
    time->second  = hal_time.ui32Second;
}

void get_next_alarm(tms_alarm_t** next_alarm)
{    
    int i, j, d;
    int num = 0;
    bool alarmFound = false;
    bool alarmToday = false;    
    bool alarmTomorrow = false;
    tms_alarm_t* alarm;
	u16_t sysMinuteCount   = 0;
	u16_t alarmMinuteCount = 0;
    u16_t nextAlarmDiff    = 0xFFFF;
    uint32_t alarm_weekday = 0;
    
	//alarm_tbl_restore();
    
    if (tms_ctx_v.alarmTbl.alarm_tbl.curNum == 0)
        return;
        
    /* Update time */
    ry_hal_rtc_get();
    tms_get_time(&tms_ctx_v.systemTime);
    	
	sysMinuteCount = tms_ctx_v.systemTime.hour*60 + tms_ctx_v.systemTime.minute;    
    uint32_t sys_weekday = (tms_ctx_v.systemTime.weekday == 0) ? 7 : tms_ctx_v.systemTime.weekday;
    
    if (tms_ctx_v.next_alarm != NULL) {
        if (tms_ctx_v.next_alarm_weekday > sys_weekday) {
            *next_alarm = tms_ctx_v.next_alarm;
            return;
        } else if (tms_ctx_v.next_alarm_weekday == sys_weekday) {
            alarmMinuteCount = tms_ctx_v.next_alarm->hour*60 + tms_ctx_v.next_alarm->minute;
            if (alarmMinuteCount > sysMinuteCount) {
                *next_alarm = tms_ctx_v.next_alarm;
                return;                
            }
        }
    }
    
    for (d = 0; d < 7; d++)
    {
        num = 0;
        nextAlarmDiff = 0xFFFF;
        alarmMinuteCount = 0;
        
        alarm_weekday = sys_weekday + d;
        if (alarm_weekday > 7) alarm_weekday -= 7;
        
        for (i = 0; num < tms_ctx_v.alarmTbl.alarm_tbl.curNum && i<MAX_ALARM_NUM; i++)
        {
            alarm = &tms_ctx_v.alarmTbl.alarm_tbl.alarms[i];
            
            if (IS_ALARM_EMPTY(alarm)) continue;

            num++;
            
            if (!alarm->enable) continue;
            
            alarmToday = false;
            
            for (j = 0; j < MAX_ALARM_REPEAT_TYPE_NUM; j++) {
                if (alarm->repeatType[j] == alarm_weekday) {
                    alarmToday = true;
                    break;
                }
                else if (alarm->repeatType[j] == 0) {
                    alarmToday = true;
                    break;
                }
            }
            
            if (alarmToday) {
                
                if (alarmTomorrow) {
                    sysMinuteCount = 0;
                }
                
                alarmMinuteCount = alarm->hour*60 + alarm->minute;
                if (sysMinuteCount <= alarmMinuteCount) {
                    if (alarmMinuteCount - sysMinuteCount < nextAlarmDiff)
                    {
                        nextAlarmDiff = alarmMinuteCount - sysMinuteCount;
                        *next_alarm = alarm;
                        tms_ctx_v.next_alarm = alarm;
                        alarmFound = true;
                    }
                }
            }
        } // for i
        
        if (alarmFound) return;
        alarmTomorrow = true;
    }   // for d
}

void alarm_task(void)
{
    int i, j;
    int num = 0;
    tms_alarm_t* p;
    bool doAlarm = FALSE;
    static uint8_t cur_alarm;
    int repeat_never = 0;
	extern u8_t alarmClockId;
	u16_t sysMinuteCout   = 0;
	u16_t alarmMinuteCout = 0;
	u16_t  ringCount      = 0;

	alarm_tbl_restore();
	
	sysMinuteCout = tms_ctx_v.systemTime.hour*60 + tms_ctx_v.systemTime.minute;
    
    int alarm_snooze_interval = get_alarm_snooze_interval();
	
    for (i = 0; num < tms_ctx_v.alarmTbl.alarm_tbl.curNum && i<MAX_ALARM_NUM; i++) {
        p = &tms_ctx_v.alarmTbl.alarm_tbl.alarms[i];
        if (IS_ALARM_EMPTY(p)) {
            continue;
        }

        num++;
		
		alarmMinuteCout = p->hour*60 + p->minute;
		if (sysMinuteCout >= alarmMinuteCout){
			ringCount = (sysMinuteCout - alarmMinuteCout)/alarm_snooze_interval;
		}else {
			//case:alarm set time 23:50  cur time 00:20
			ringCount = (1440 - (alarmMinuteCout - sysMinuteCout))/alarm_snooze_interval;
		}
			
		LOG_DEBUG("\r\nAlarm SnoozeMode=%d ringTimes=%d ringCount=%d hour=%d minute=%d repeatType=%d en=%d.\r\n",\
		p->snoozeMode,p->ringTimes,ringCount,p->hour,p->minute,p->repeatType[0],p->enable);
		if ((p->snoozeMode == 1) && (p->ringTimes == ALARM_SNOOZE_MAX_TIMES) && (ringCount > ALARM_SNOOZE_MAX_TIMES)) {
			p->ringTimes = 0;
			p->ringFlag = 0;
			if (p->repeatType[0] == 0){
				alarm_set_enable(p->id, 0);
			}         
			alarm_tbl_save();
		}
		//disable app icon
		else if ((p->snoozeMode == 1) && (p->ringTimes == ALARM_SNOOZE_MAX_TIMES) && (ringCount < ALARM_SNOOZE_MAX_TIMES)) {
			if ((p->repeatType[0] == 0) && (p->enable)){
				alarm_set_enable(p->id, 0);        
			}
		}
		
		if ((p->snoozeMode == 1) && (p->ringTimes < ALARM_SNOOZE_MAX_TIMES) && (p->ringFlag == 1) && (p->enable == 1)) {
		
			u8_t clockHour = 0;
			u8_t clockMinute = 0;
			
			if (ringCount < ALARM_SNOOZE_MAX_TIMES) {
				clockMinute = (ringCount * alarm_snooze_interval + p->minute) % 60;
				clockHour   = ((ringCount * alarm_snooze_interval + p->minute) / 60) + (p->hour);
				if (clockHour > 23) {
					clockHour = clockHour - 24;
				}
				if ((clockHour == tms_ctx_v.systemTime.hour) && (clockMinute == tms_ctx_v.systemTime.minute)) {
						for (j = 0; j < MAX_ALARM_REPEAT_TYPE_NUM; j++) {
						uint32_t sys_weekday = (tms_ctx_v.systemTime.weekday == 0) ? 7 : tms_ctx_v.systemTime.weekday;
						if (p->repeatType[j] == sys_weekday) {
							doAlarm = TRUE;
							cur_alarm = i;
							break;
						}
						else if (p->repeatType[j] == 0){
							doAlarm = TRUE;
							cur_alarm = i;
							if (ringCount == (ALARM_SNOOZE_MAX_TIMES - 1)) {
								alarm_set_enable(p->id, 0); 							
							}
							break;
						}         
					}
				}
			}
		}
		
        if (p->hour == tms_ctx_v.systemTime.hour &&
            p->minute == tms_ctx_v.systemTime.minute &&
            p->enable) {
            for (j = 0; j < MAX_ALARM_REPEAT_TYPE_NUM; j++) {
                uint32_t sys_weekday = (tms_ctx_v.systemTime.weekday == 0) ? 7 : tms_ctx_v.systemTime.weekday;
                if (p->repeatType[j] == sys_weekday) {
                    doAlarm = TRUE;
                    cur_alarm = i;
					p->ringTimes = 0;
					if(p->snoozeMode == 1) {
						p->ringFlag = 1;
					}
					alarm_tbl_save(); 
                    break;
                }
                else if (p->repeatType[j] == 0){
                    doAlarm = TRUE;
                    cur_alarm = i;
					p->ringFlag = 1;
					if(p->snoozeMode != 1) {
						alarm_set_enable(p->id, 0);        //alarm only once
					}
                    break;
                }         
            }
        }

        // check last two minutes alarm
        uint32_t alarm_time = (p->hour * 60) + p->minute;
        uint32_t sys_time = (tms_ctx_v.systemTime.hour * 60) + tms_ctx_v.systemTime.minute;
        
        uint32_t alarm_set_time = ry_hal_calc_second(ry_hal_clock_time() - tms_ctx_v.last_alarm_set_time);
        if ((alarm_time == (sys_time - 1)) &&  p->enable && (alarm_set_time > 70)) {
            /* Check repeat type */
            for (j = 0; j < MAX_ALARM_REPEAT_TYPE_NUM; j++) {
                uint32_t sys_weekday = (tms_ctx_v.systemTime.weekday == 0) ? 7 : tms_ctx_v.systemTime.weekday;
                //LOG_DEBUG("\r\n[alarm_task] 1 j: %d, p->repeatType[j]: %d, systemTime.weekday: %d\r\n", \
                        j, p->repeatType[j], sys_weekday);  
                if (p->repeatType[j] == sys_weekday) {
                    doAlarm = TRUE;
                    cur_alarm = i;
					p->ringFlag = 1;
                    //LOG_DEBUG("\r\n[alarm_task] 1 j: %d, p->repeatType[j]: %d, systemTime.weekday: %d\r\n", \
                        j, p->repeatType[j], tms_ctx_v.systemTime.weekday);  
                    break;
                }
                else if (p->repeatType[j] == 0){
                    doAlarm = TRUE;
                    cur_alarm = i;
					p->ringFlag = 1;
                    alarm_set_enable(p->id, 0);        //alarm only once
					//LOG_DEBUG("\r\n[alarm_taskx] 1 j: %d, p->repeatType[j]: %d, systemTime.weekday: %d\r\n", \
                        j, p->repeatType[j], tms_ctx_v.systemTime.weekday);  
                    break;
                }         
            }
        }
    }

    if (doAlarm){
        p = &tms_ctx_v.alarmTbl.alarm_tbl.alarms[cur_alarm];
        //LOG_INFO("\r\n[alarm] curNum:%d, hour:%d, min:%d, en:%d,  ringTimes:%d   snonzeMode:%d\r\n", \
            tms_ctx_v.alarmTbl.alarm_tbl.curNum, p->hour, p->minute, p->enable,p->ringTimes,p->snoozeMode);
        ry_sched_msg_send(IPC_MSG_TYPE_ALARM_ON, sizeof(uint32_t), &cur_alarm);
        tms_ctx_v.last_alarm_set_time = ry_hal_clock_time();
    }
}

uint8_t tms_alg_enable_get(void)
{
    return tms_ctx_v.alg_enable;
}


uint8_t tms_oled_max_brightness_percent(void)
{
    return tms_ctx_v.oled_max_brightness;
}

void set_tms_oled_max_brightness_percent(uint8_t rate)
{
    const uint8_t brightness_value[] = { OLED_BRIGHTNESS_HIGH_PERCENT, 
                                         OLED_BRIGHTNESS_HIGH_MIDDLE, 
                                         OLED_BRIGHTNESS_HIGH_LOW, 
                                         OLED_BRIGHTNESS_HIGH_NIGHT,
                                       };
    rate = (rate > E_BRIGHTNESS_MAX) ? 0 : rate;
    tms_ctx_v.oled_max_brightness = brightness_value[rate];
}

void ckeck_time_and_reset_data_init(void)
{

}

void oled_brightness_update_by_time(void)
{
    //reset wrist setting when 22:00 and 08:00
    if (tms_ctx_v.systemTime.hour >= 22 || tms_ctx_v.systemTime.hour < 6) {
        if (get_brightness_night() != 0){
            set_tms_oled_max_brightness_percent(E_BRIGHTNESS_NIGHT);
        }
        else{
            set_tms_oled_max_brightness_percent(get_brightness_rate());
        }
    }
    else {
        set_tms_oled_max_brightness_percent(get_brightness_rate());
    }
}

void check_time_and_reset_data(void)
{
    //reset alg data when 00:00
    static uint8_t f_data_cleared = 0;
    if (tms_ctx_v.systemTime.hour == 0 && tms_ctx_v.systemTime.minute <= 20) {
        if (f_data_cleared == 0){
            alg_clear_results_today();
            system_app_data_save();
            clear_gui_on_time();
            clear_hrm_on_time();
            save_dev_statistics_data();
            data_sports_distance_offset_clear(1);
            data_calorie_offset_clear(1);
            f_data_cleared = 1;
            LOG_DEBUG("[alg_clear_step_today] : s_alg.step_cnt_today: %d, s_alg.hr_cnt: %d, s_alg.long_sit_times: %d\n", \
            s_alg.step_cnt_today, s_alg.hr_cnt,  s_alg.long_sit_times); 
        }
    }
    else{
        f_data_cleared = 0;
    }
    
    oled_brightness_update_by_time();
	
	extern ry_device_setting_t device_setting;
	u16 start_time = 0;
	u16 end_time   = 0;
	u16 current_time = 0;
	
	start_time   = get_raise_towake_start_time();
	end_time     = get_raise_towake_end_time();
	current_time = tms_ctx_v.systemTime.hour*60 + tms_ctx_v.systemTime.minute;
	
	if(device_setting.raise_to_wake){
		//APP never connected start time is 0xffff
		if((start_time == 0xffff) && (tms_ctx_v.systemTime.hour >= 8 && tms_ctx_v.systemTime.hour < 22)){
			tms_ctx_v.alg_enable = 1;
		}
		else if ((start_time == current_time)
			|| ((start_time < current_time) && (current_time < end_time))
			|| ((start_time > end_time) && (current_time > start_time))
			|| ((start_time > end_time) && (current_time < end_time) && (end_time <= 1440))){
			//set_raise_to_wake_open();
			tms_ctx_v.alg_enable = 1;
			//LOG_DEBUG("start_time=%d end_time=%d levi!\r\n",start_time,end_time);
		}
		else{
			//set_raise_to_wake_close();
			tms_ctx_v.alg_enable = 0;
		}
	}
	else{
		tms_ctx_v.alg_enable = 0;
	}
}

void check_time_and_store_batt_log(void) {
    
    // if time is not set skip processing
    if (tms_ctx_v.systemTime.year == 0 && tms_ctx_v.systemTime.month == 0 && tms_ctx_v.systemTime.day == 0)
        return;    
    
    extern ry_device_setting_t device_setting;
    
    am_hal_rtc_time_t ctime;
    ry_hal_rtc_get_time(&ctime);
    
    am_hal_rtc_time_t* stime;
    stime = &device_setting.battery_log_store_time;
    
//    LOG_ERROR("time rtc: y:%d m:%d d:%d", ctime.ui32Year, ctime.ui32Month, ctime.ui32DayOfMonth);
//    LOG_ERROR("time bat: y:%d m:%d d:%d", stime->ui32Year, stime->ui32Month, stime->ui32DayOfMonth);
        
    if ((stime->ui32Year == 0 &&
        stime->ui32Month == 0 &&
        stime->ui32DayOfMonth == 0) ||
        (stime->ui32Year != ctime.ui32Year ||
        stime->ui32Month != ctime.ui32Month || 
        stime->ui32DayOfMonth != ctime.ui32DayOfMonth))
    {
        set_battery_log(tms_ctx_v.batt_percent);        
    }
}

/**
 * @brief   Periodic task running
 *
 * @param   None
 *
 * @return  None
 */
void periodic_task(void)
{
    int i;
    periodic_task_t *p;
    
    /* Periodic task handle */
    for (i = 0; i < MAX_PERIODIC_TASK_NUM; i++) {
        p = &tms_ctx_v.ptt.tasks[i];
        if (IS_PERIODIC_TASK_EXIST(p) && 
            (tms_ctx_v.tick - p->lastTick >= p->interval) && 
            p->enable)
        {
            p->handler(p->usrdata);
            p->lastTick = tms_ctx_v.tick;
        }
    }

    /* Monitor task */
    monitor_task();

    /* Check alarm list */
    static uint8_t alarm_checked;
    if ((tms_ctx_v.systemTime.second <= 50)){
        if (alarm_checked == 0){
            alarm_task();
            alarm_checked = 1;
        }
    }
    else{
        alarm_checked = 0;
    }
                 
    /* Broadcast specific time event to system */
    if (tms_ctx_v.systemTime.second == 0) {
        ry_sched_msg_send(IPC_MSG_TYPE_RTC_1MIN_EVENT, 0, NULL);
    }

    if (tms_ctx_v.systemTime.minute == 0 && tms_ctx_v.systemTime.second < 2) {
        ry_sched_msg_send(IPC_MSG_TYPE_RTC_1HOUR_EVENT, 0, NULL);
        
        //LOG_INFO("[batt_hour] %d", sys_get_battery_adc());
    }

    if (tms_ctx_v.systemTime.hour == 0 && tms_ctx_v.systemTime.minute == 0 && tms_ctx_v.systemTime.second < 2) {
        ry_sched_msg_send(IPC_MSG_TYPE_RTC_1DAY_EVENT, 0, NULL);
        
        check_time_and_store_batt_log();        
    }

    check_time_and_reset_data();
}


#include "json.h"
#include "ry_hal_i2c.h"
extern u32_t touch_raw_report;
extern rpc_ctx_t* touch_test_ctx;

u32_t touch_test_task(void)
{
    if (touch_raw_report) {
        
        u32_t* btn = (u32_t*)ry_malloc(48);
        u32_t r = ry_hal_irq_disable();
        ry_hal_i2cm_init(I2C_TOUCH);
        touch_get_diff(btn);
        ry_hal_i2cm_uninit(I2C_TOUCH);
        ry_hal_irq_restore(r);
        rpc_touch_raw_report(touch_test_ctx, btn, 12);
        ry_free((void*)btn);
        btn = NULL;
			
        return 1;
    } else {
        return 0;
    }
}


void tme_set_interval(int ms)
{
    LOG_INFO("[tms] set interval to %d msec", ms);
    tms_ctx_v.interval_ms = ms;
}


/**
 * @brief   Timer Service thread
 *
 * @param   None
 *
 * @return  None
 */
int tms_thread(void* arg)
{
    int tms_delay_ms;
    
    LOG_DEBUG("[tms_thread] Enter.\n");

    tms_ctx_v.tick = 0;
    tms_ctx_v.oled_max_brightness = 100;
    
    // use default inverval 1000 msec
    // if not set
    
    if (tms_ctx_v.interval_ms == 0)
        tms_ctx_v.interval_ms = 1000;

    ry_sts_t status = RY_SUCC;
	status = ryos_queue_create(&ry_tms_monitorQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[tms_thread]: ry_tms_monitorQ Queue Create Error.\r\n");
        while(1);
    }

    /* Init Alarm table */
	alarm_restore();
    alarm_tbl_restore();

    /* Init Periodic Task Table */
    ry_memset((u8_t*)tms_ctx_v.ptt.tasks, 0xFF, sizeof(periodic_task_t)*MAX_PERIODIC_TASK_NUM);
    tms_ctx_v.ptt.curNum = 0;

    /* Add System Task */
    periodic_task_add(BATTERY_CHECK_PERIOID, periodic_battery_task, NULL, 0);
    periodic_task_add(TOUCH_RESET_PERIOID, periodic_touch_task, NULL, 0);
    periodic_task_add(AUTO_HR_DETECT_PERIOID, periodic_hr_task, NULL, 0);
    periodic_task_add(AUTO_DATA_SAVE_PERIOID, periodic_app_data_save_task, NULL, 0);
    periodic_task_add(AUTO_SLEEP_HRM_WEARED_DETECT_PERIOID, periodic_sleep_hr_weared_detect_task, NULL, 0);
	
    periodic_task_set_enable(periodic_battery_task, 1);
    periodic_task_set_enable(periodic_app_data_save_task, 1);
    periodic_task_set_enable(periodic_sleep_hr_weared_detect_task, 0);

    ry_system_lifecycle_schedule_periodic_task_init();

    periodic_battery_task(NULL);
    
    /* Start RTC */
    ry_hal_rtc_start();
   // set_rtc_int_every_second();      
    ry_hal_rtc_get();
    oled_brightness_update_by_time();

    while (1) {
        
        /* Update time */
        ry_hal_rtc_get();
        tms_get_time(&tms_ctx_v.systemTime);

        /* Feed watchdog */
        ry_hal_wdt_reset();

        /* Execute all periodic tasks */
        periodic_task();

        if (1 == touch_test_task()) {
            tms_delay_ms = 100;
		} else {
            //tms_delay_ms = 1000;
            tms_delay_ms = tms_ctx_v.interval_ms;
		}

        // Running every 2/5 seconds
        ryos_delay_ms(tms_delay_ms);
        tms_ctx_v.tick++;
    }
}




/**
 * @brief   API to initialize timer service
 *
 * @param   None
 *
 * @return  None
 */
void tms_init(void)
{
    ryos_threadPara_t para;

    /* Start Test Demo Thread */
    strcpy((char*)para.name, "tms_thread");
    para.stackDepth = 512;
    para.arg        = NULL; 
    para.tick       = 20;
    para.priority   = 3;
    ryos_thread_create(&tms_thread_handle, tms_thread, &para);   
}
