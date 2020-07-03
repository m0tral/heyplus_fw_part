#include "data_management_service.h"
#include "timer_management_service.h"
#include "card_management_service.h"

#include "string.h"
#include "Notification.pb.h"
#include "rbp.pb.h"
#include "rbp.h"
#include "ryos.h"
#include "ry_utils.h"
#include "ry_hal_inc.h"
#include "pb.h"
#include "nanopb_common.h"
#include "scheduler.h"
#include "Location.pb.h"
#include "Sport.pb.h"
//#include "Health.pb.h"
#include "DataUpload.pb.h"
#include "sensor_alg.h"
#include "ry_ble.h"
#include "AlarmClock.pb.h"
#include "gui_bare.h"
#include "BodyStatus.pb.h"
#include "dip.h"
#include "Algorithm.h"
#include "device_management_service.h"
#include "surface_management_service.h"
#include "ry_statistics.h"
#include "window_management_service.h"
#include "ryos_timer.h"
#include "board.h"
#include "ryos_protocol.h"
#include "mibeacon.h"
#include <stdio.h>
#include "ble_task.h"
#include "gui.h"

/*********************************************************************
 * CONSTANTS
 */ 
#define DID_LEN                         20
#define DATA_SERVICE_QUEUE_SIZE         20
#define GPS_ACCESS_CNT_MAX              10  // time = cnt *10 second
#define GPS_STATUS_LOST_THRESH          3   // time = cnt *10 second


/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    u8_t          enable;
    u8_t          last_file_day;
    FIL*          cur_data_file_p;
} data_service_ctx_t;

#pragma pack(4) 
typedef struct sys_app_data{
    WeatherCityList city_list;
    WeatherGetRealtimeInfoResult weather_info;
    //AlarmClockList clock_list;
    u32_t time;
    sensor_alg_t   s_alg;
    alg_sleep_time_t sleep_time;
    int            curSurfaceId;
    u32_t reserved_0[6];
    //vStatisticsResult dev_statistics;
    u8_t  batt_last;
    u32_t last_hrm_time;
    u32_t reserved[30];
}sys_app_data_t;

typedef struct sys_app_data_new{
    u32_t magic;
    u32_t version;
    u32_t reserved_0[10];
    WeatherCityList city_list;
    u32_t reserved_1[50];
    WeatherGetRealtimeInfoResult weather_info;
    u32_t reserved_2[50];
    u32_t time;
    sensor_alg_t   s_alg;
    u32_t reserved_3[19];
    alg_sleep_time_t sleep_time;
    u32_t reserved_4[10];
    int   curSurfaceId;
    u32_t reserved_5[16];
    u8_t  batt_last;
    u32_t last_hrm_time;
    float data_sports_offset_distance_today;
    float data_calorie_offset_today;
    u32_t reserved[28];
}sys_app_data_new_t;
#pragma pack()

typedef struct data_offset{
    float distance_app;
    float distance_today;
    float calorie_today;
}data_offset_t;

typedef struct gps_location_s{
    float    distance_app;
    uint32_t  gps_got_cnt;
    uint32_t  gps_changed_st;    
    uint32_t gps_request_cnt;    
}gps_location_t;

/*********************************************************************
 * LOCAL VARIABLES
 */

data_service_ctx_t data_service_ctx_v;
gps_location_t gps_location_v;

LocationResult dev_locat;
//epRecord dev_step;
//StepRecord cur_stepRecord = {0};
//HeartRateRecord cur_heartRateRecord = {0};
//SleepRecord cur_sleepRecord = {0};
u8_t running_flag = 0;

BodyStatusRecord cur_status;
sleep_record_rri_t * cur_sleep_record_data = NULL;

//u32_t cur_sleep_id = 0;
u8_t upload_file_mask = 0;
u32_t upload_cur_file_size = 0;
char* data_file_name_arr[] = {BODY_STATUS_DATA, RUN_DATA_FILE_NAME, SLEEP_DATA_FILE_NAME};
FIL * upload_fp = NULL;
sports_global_status_t sports_global = {0};
u32_t getup_tick_normal = (60*60);
u32_t getup_tick_quick = (60*10);
u32_t data_upload_timer = 0;
u32_t data_device_restart_type;

data_offset_t data_sports_offset;

//extern dev_info_t device_info;
extern sensor_alg_t   s_alg;
extern AlarmClockList clock_list;


/*********************************************************************
 * FUNCTIONS
 */
void data_sports_distance_offset_update(float offset)
{
    data_sports_offset.distance_today += offset;
    data_sports_offset.distance_app += offset;    
}

void data_sports_distance_offset_today_restore(float offset)
{
    if (((uint32_t)offset != 0xffffffff) && ((uint32_t)offset != 0xA5A5A5A5) && (offset > 0.1f) && (offset < 100000.0f)){
        data_sports_offset.distance_today = offset;    
    }
}

void data_sports_distance_offset_clear(uint32_t flg_all)
{
    if (flg_all){
        data_sports_offset.distance_today = 0;
    }
    data_sports_offset.distance_app = 0;    
}

float data_sports_distance_app_offset_get(void)
{
    return data_sports_offset.distance_app;    
}

float data_sports_distance_today_offset_get(void)
{
    return data_sports_offset.distance_today;
}

float app_get_step_distance(uint32_t step)
{
    float distance = alg_get_step_distance(step) + data_sports_distance_app_offset_get();
    data_sports_distance_offset_clear(0);
    return distance;
}

float app_get_distance_today(void)
{
  return (alg_get_distance_today() + data_sports_distance_today_offset_get());
}


void data_calorie_offset_update(float calorie)
{
    data_sports_offset.calorie_today += calorie;
    LOG_DEBUG("[calorie_offset_update] calorie_offset_today:%0.1f, calorie:%0.1f\r\n", \
        data_sports_offset.calorie_today, calorie);
}

void data_calorie_offset_clear(uint32_t flg_all)
{
    data_sports_offset.calorie_today = 0.0f;
}

float data_sports_calorie_offset_get(void)
{
    return data_sports_offset.calorie_today;
}

float app_get_calorie_today(void)
{
    return (alg_calc_calorie() + data_sports_offset.calorie_today);
}

void data_calorie_offset_today_restore(float offset)
{
    if (((int)offset != 0xffffffff) && (offset > 0.1f) && (offset < 10000.0f)){
        data_sports_offset.calorie_today = offset;   
    } 
}


void data_service_set_enable(uint8_t f_enable)
{
    data_service_ctx_v.enable = f_enable;
}

uint8_t data_service_enable_status_get(void)
{
    return  data_service_ctx_v.enable;
}

/*FRESULT remove_sport_data(int data_type)
{
    FRESULT result = FR_OK;
    char * file_name = NULL;
    if(data_type == (int)SPORT_DATA_STEP){
        file_name = STEP_DATA_FILE_NAME;
        //data_size = sizeof(StepRecord);
    }else if(data_type == (int)SPORT_DATA_HEART){
        file_name = HEART_DATA_FILE_NAME;
        //data_size = sizeof(HeartRateRecord);
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        file_name = SLEEP_DATA_FILE_NAME;
        //data_size = sizeof(SleepRecord);
    }else if(data_type == (int)SPORT_DATA_RUN){
        file_name = RUN_DATA_FILE_NAME;
        //data_size = sizeof(RunRecord);
    }
    result = f_unlink(file_name);

    
    return result;
}
*/

void set_running(void)
{
    running_flag = 1;
    set_device_session(DEV_SESSION_RUNNING);
}
void clear_running(void)
{
    running_flag = 0;

    if(get_device_session() != DEV_SESSION_UPLOAD_DATA){
        set_device_session(DEV_SESSION_IDLE);
    }
}
u8_t get_running_flag(void)
{
    return running_flag;
}
void set_current_running_disable(void)
{
    sports_global.run_disable = 1;
}

void set_current_running_enable(void)
{
    sports_global.run_disable = 0;
}

u8_t get_current_running_status(void)
{
    return sports_global.run_disable;
}


u32_t get_day_number(void)
{
    ry_time_t time;
    tms_get_time(&time);
    return (time.hour > 18)?(time.weekday):( (time.weekday + 7 - 1)%7 );
}

void clear_today_data(void)
{
    DIR * dir = NULL;
    char * dir_str = NULL;
    FRESULT ret = FR_OK;

    dir_str = (char*)ry_malloc(100);
    sprintf(dir_str, "%d", get_day_number());

    ret = f_mkdir(dir_str);

    if(ret != FR_OK){
        f_unlink(dir_str);
        ret = f_mkdir(dir_str);
    }

    if(ret != FR_OK){
        LOG_ERROR("[%s] make fs failed\n", __FUNCTION__);
    }    
}

void data_ble_upload_callback(ry_sts_t status, void* arg)
{
    static u8_t resend_count = 0;
    u8_t * buf = NULL;
    pb_ostream_t stream_o = {0};

    msg_param_t* retry_param = (msg_param_t *)arg;

    if(arg == NULL){
        LOG_ERROR("retry arg is NULL\n");
        return ;
    }
    
    if (status != RY_SUCC) {
    
        LOG_ERROR("[%s] error status: %x\r\n", __FUNCTION__, status);
        LOG_ERROR("retry: %d\n", resend_count);
        if((resend_count < DATA_UPLOAD_RESEND_MAX) && (ry_ble_get_state() == RY_BLE_STATE_CONNECTED)){

            LOG_ERROR("retry send request, buf: 0x%x  arg: 0x%x, heap: %d\n", (int)retry_param->buf, (int)arg, ryos_get_free_heap());
            
            status = ble_send_request(CMD_APP_UPLOAD_DATA_LOCAL,  retry_param->buf, retry_param->len, 1,data_ble_upload_callback, (void *)arg);
            
            //ry_free(buf);
            resend_count++;
            
        } else {

            if(arg != NULL){
                FIL * fp = NULL;
                FRESULT ret = FR_OK;
                u32_t written_byte = 0;

                fp = (FIL *)ry_malloc( sizeof(FIL) );
                if(fp == NULL){
                    LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
                    goto exit;
                }
                
                ret = f_open(fp, RETRY_UPLOAD_DATA, FA_WRITE | FA_OPEN_APPEND);
                if(ret != FR_OK){
                    LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
                    ry_free(fp);
                    fp = NULL;
                    goto exit;
                }

                ret = f_write(fp, retry_param->buf, retry_param->len, &written_byte);
                if(ret != FR_OK){
                    LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_WRITE);
                }
                f_close(fp);
                ry_free(fp);

                LOG_INFO("{%s}save data ok\n", __FUNCTION__);

            }
            ry_free(retry_param->buf);
            ry_free(arg);
            resend_count = 0;

            data_upload_stop_handle(NULL);
        }
        
    } else {
        resend_count = 0;
        ry_free(retry_param->buf);
        ry_free(arg);
        LOG_DEBUG("[%s]%s, error status: %x, heap:%d, minHeap:%d\r\n", \
            __FUNCTION__, (char*)arg, status, ryos_get_free_heap(), ryos_get_min_available_heap_size());
    }

exit:
    //ry_free(arg);
    return;
}

void data_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);        
    } else {
        LOG_INFO("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    }
}



ry_sts_t location_result(u8_t * data, u32_t size)
{
    LocationResult * result_data = (LocationResult *)ry_malloc(sizeof(LocationResult));
    ry_sts_t status = RY_SUCC;
    u32_t code = RBP_RESP_CODE_SUCCESS;
    pb_istream_t stream;

    if(result_data == NULL){
        goto exit;
    }else{
        memset(result_data, 0, sizeof(LocationResult));
    }
    
    stream = pb_istream_from_buffer(data, size);

    if (0 == pb_decode(&stream, LocationResult_fields, result_data)){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        //return status;
        code = RBP_RESP_CODE_DECODE_FAIL;
        goto exit;
    }
    dev_locat = *result_data;
    //dev_locat.longitude = result_data->longitude;

    /*if(dev_locat.has_last_distance == 0){
        dev_locat.last_distance = 0;
    }*/
    

    LOG_DEBUG("[%s] update location --- %d\n", __FUNCTION__, dev_locat.last_distance);
    status = rbp_send_resp(CMD_DEV_START_LOCATION_RESULT, code, NULL, 0, 1);

    if(get_running_flag() == 1){
        set_run_point(0);
    }


exit:  
    ry_free(result_data);
    return status;
    
}

void get_location(void )
{
    u8_t * buf = NULL;
    pb_ostream_t stream;
    LocationRequest * req = (LocationRequest *)ry_malloc(sizeof(LocationRequest));
    if(req == NULL){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    ry_memset(req, 0, sizeof(LocationRequest));

    buf = (u8_t *)ry_malloc(sizeof(LocationRequest));
    if(buf == NULL){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }

    stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(LocationRequest) );
    
    req->session_id = 1;
    if (dev_locat.latitude != 0) {
        req->last_latitude = dev_locat.latitude;
        req->has_last_latitude = 1;
    }
    if (dev_locat.longitude != 0) {
        req->last_longitude = dev_locat.longitude;
        req->has_last_longitude = 1;
    }
    
    if (!pb_encode(&stream, LocationRequest_fields, req)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    rbp_send_req(CMD_APP_START_LOCATION_REQUEST, buf, stream.bytes_written, 1);
    
exit:
    ry_free(buf);
    ry_free(req);
    return ;
}


void check_save_data(void)
{
    char * file_name = NULL;
    FRESULT status = FR_OK;
    FIL * fp = NULL;
    ry_sts_t ret = RY_SUCC;
    u32_t written_bytes = 0;
    u32_t data_size = 0;
    sport_data_type_t data_type = (sport_data_type_t)0;
    u8_t day = 0;

    fp = (FIL *)ry_malloc(sizeof(FIL));

    file_name = (char *)ry_malloc(80);
    
    for(day = 0; day < 7; day++){
        if(file_name == NULL){
            LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        sprintf(file_name, "%d_%s", day, BODY_STATUS_DATA);

        status = f_open(fp, file_name, FA_READ);
        if(status != FR_OK){
            LOG_DEBUG("%s file is empty\n", file_name);
            continue;
        }else{
            
            LOG_ERROR("%s size: %d\n", file_name, f_size(fp));
            if(f_size(fp) >= 200*1024){
                f_close(fp);
                f_unlink(file_name);
            }else{
                f_close(fp);
            }           
        }
    }

    for(day = 0; day < 7; day++){
        if(file_name == NULL){
            LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        sprintf(file_name, "%d_%s", day, SLEEP_DATA_FILE_NAME);

        status = f_open(fp, file_name, FA_READ);
        if(status != FR_OK){
            LOG_DEBUG("%s file is empty\n", file_name);
            continue;
        }else{
            LOG_ERROR("%s size: %d\n", file_name, f_size(fp));
            if(f_size(fp) >= 80*1024){
                f_close(fp);
                f_unlink(file_name);
            }else{
                f_close(fp);
            }            
        }
    }

exit:
    ry_free(fp);
    ry_free(file_name);
    
}
void clear_save_data(void)
{
    char * file_name = NULL;
    FRESULT status = FR_OK;
    FIL * fp = NULL;
    ry_sts_t ret = RY_SUCC;
    u32_t written_bytes = 0;
    u32_t data_size = 0;
    sport_data_type_t data_type = (sport_data_type_t)0;
    u8_t day = 0;


    file_name = (char *)ry_malloc(80);

    f_unlink(RETRY_UPLOAD_DATA);
    
    for(day = 0; day < 7; day++){
        if(file_name == NULL){
            LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        sprintf(file_name, "%d_%s", day, BODY_STATUS_DATA);
        f_unlink(file_name);
    }

    for(day = 0; day < 7; day++){
        if(file_name == NULL){
            LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        sprintf(file_name, "%d_%s", day, SLEEP_DATA_FILE_NAME);

        f_unlink(file_name);
    }


    /*for(day = 0; day < 7; day++){
        if(file_name == NULL){
            LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        sprintf(file_name, "%d_%s", day, RY_STATISTICS_FILE_NAME);

        f_unlink(file_name);
    }*/

    f_unlink(RETRY_UPLOAD_DATA);

    LOG_INFO("[%s]---success\n", __FUNCTION__);

exit:
    ry_free(file_name);
    
}

void delete_one_day_data(int day_num)
{
    char * today_file_name = NULL;
    today_file_name = (char *)ry_malloc(80);
    if(today_file_name == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        return;
    }
    sprintf(today_file_name, "%d_%s",day_num , BODY_STATUS_DATA);
    LOG_INFO("unlink file : %s\n", today_file_name);
    f_unlink(today_file_name);
    sprintf(today_file_name, "%d_%s",day_num , STEP_DATA_FILE_NAME);
    LOG_INFO("unlink file : %s\n", today_file_name);
    f_unlink(today_file_name); 

    ry_free(today_file_name);

}


ry_sts_t save_sport_data(int data_type, void * data)
{
    /*char * file_name = NULL;
    FRESULT status = FR_OK;
    FIL * fp = NULL;
    ry_sts_t ret = RY_SUCC;
    u32_t written_bytes = 0;
    u32_t data_size = 0;
    
    if(data_type == (int)SPORT_DATA_STEP){
        file_name = STEP_DATA_FILE_NAME;
        data_size = sizeof(StepRecord);
    }else if(data_type == (int)SPORT_DATA_HEART){
        file_name = HEART_DATA_FILE_NAME;
        data_size = sizeof(HeartRateRecord);
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        file_name = SLEEP_DATA_FILE_NAME;
        data_size = sizeof(SleepRecord);
    }else if(data_type == (int)SPORT_DATA_RUN){
        file_name = RUN_DATA_FILE_NAME;
        data_size = sizeof(RunRecord);
    }else{
        return RY_SUCC;
    }

    fp = (FIL *)ry_malloc(sizeof(FIL));
    
    status = f_open(fp, file_name, FA_OPEN_APPEND | FA_WRITE);
    if(status != FR_OK){
        LOG_ERROR("[%s-%d]---file open failed\n", __FUNCTION__, __LINE__);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_WRITE);
        goto exit;
    }

    if(f_size(fp) > 30*1024){
        f_close(fp);
        f_unlink(file_name);
        f_open(fp, file_name, FA_OPEN_APPEND | FA_WRITE);
    }

    status = f_write(fp, data, data_size, &written_bytes);
    if(status != FR_OK){
        LOG_ERROR("[%s-%d]---file write failed\n", __FUNCTION__, __LINE__);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_WRITE);
        goto exit;
    }
    status = f_close(fp);
    if(status != FR_OK){
        LOG_ERROR("[%s-%d]---file close failed\n", __FUNCTION__, __LINE__);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_WRITE);
        goto exit;
    }*/

/*
exit:
    f_close(fp);
    ry_free(fp);
    return ret;    */
		return RY_SUCC;
}


void start_record_data(int data_type, void * para)
{
    /*void *data;
    u32_t data_size = 0;
    if(data_type == (int)SPORT_DATA_STEP){
        data_size = sizeof(StepRecord);
        cur_stepRecord.start_time = ryos_utc_now();
        
        cur_stepRecord.step = alg_get_step_today();
    }else if(data_type == (int)SPORT_DATA_HEART){
        data_size = sizeof(HeartRateRecord);
        cur_heartRateRecord.time = ryos_utc_now();
        cur_heartRateRecord.status = HeartRateRecord_Status_RUN;
        cur_heartRateRecord.measure_type = HeartRateRecord_MeasureType_MANUAL;
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        data_size = sizeof(SleepRecord);
        cur_sleepRecord.start_time = ryos_utc_now();
        cur_sleepRecord.sleep_status = SleepRecord_SleepStatus_DEEP;
    }else if(data_type == (int)SPORT_DATA_RUN){
        data_size = sizeof(RunRecord);
        cur_runRecord = (RunRecord *)ry_malloc(data_size);
        cur_runRecord->start_time = ryos_utc_now();
    }*/
}


ry_sts_t get_sport_data(int data_type, void *data, u32_t offset, u32_t len)
{
    /*char * file_name = NULL;
    FRESULT status = FR_OK;
    FIL * fp = (FIL *)ry_malloc(sizeof(FIL));
    ry_sts_t ret = RY_SUCC;
    u32_t written_bytes = 0;
    u32_t data_size = 0;
    u8_t get_all_flag = 0;

    if(data_type == (int)SPORT_DATA_STEP){
        file_name = STEP_DATA_FILE_NAME;
        data_size = sizeof(StepRecord);
    }else if(data_type == (int)SPORT_DATA_HEART){
        file_name = HEART_DATA_FILE_NAME;
        data_size = sizeof(HeartRateRecord);
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        file_name = SLEEP_DATA_FILE_NAME;
        data_size = sizeof(SleepRecord);
    }else if(data_type == (int)SPORT_DATA_RUN){
        file_name = RUN_DATA_FILE_NAME;
        data_size = sizeof(RunRecord);
    }


    status = f_open(fp, file_name, FA_READ);
    if(status != FR_OK){
        LOG_ERROR("[%s-%d]---file open failed\n", __FUNCTION__, __LINE__);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_READ);
        goto exit;
    }
    
    status = f_lseek(fp, offset);
    if(status != FR_OK){
        LOG_ERROR("[%s-%d]---file seek failed\n", __FUNCTION__, __LINE__);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_READ);
        goto exit;
    }

    status = f_read(fp, data, len, &written_bytes);
    if(status != FR_OK){
        LOG_ERROR("[%s-%d]---file read failed\n", __FUNCTION__, __LINE__);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_READ);
        goto exit;
    }

    if( f_tell(fp) == f_size(fp) ){
        get_all_flag = 1;
    }

    f_close(fp);

    if(get_all_flag){
        f_unlink(file_name);
    }
    
exit:

    ry_free(fp);
    return ret;*/
		return RY_SUCC;
}


u32_t get_sport_data_size(int data_type)
{
    char * file_name = NULL;
    FRESULT status = FR_OK;
    FIL * fp = (FIL *)ry_malloc(sizeof(FIL));
    ry_sts_t ret = RY_SUCC;
    u32_t file_size = 0;
    u32_t data_size = 0;

    if(data_type == (int)SPORT_DATA_STEP){
        file_name = STEP_DATA_FILE_NAME;
        //data_size = sizeof(StepRecord);
    }else if(data_type == (int)SPORT_DATA_HEART){
        file_name = HEART_DATA_FILE_NAME;
        //data_size = sizeof(HeartRateRecord);
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        file_name = SLEEP_DATA_FILE_NAME;
        //data_size = sizeof(SleepRecord);
    }else if(data_type == (int)SPORT_DATA_RUN){
        file_name = RUN_DATA_FILE_NAME;
        //data_size = sizeof(RunRecord);
    }

    status = f_open(fp, file_name, FA_READ);
    if(status != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        ret = RY_SET_STS_VAL(RY_CID_SPI, RY_ERR_WRITE);
        goto exit;
    }

    file_size = f_size(fp);
    f_close(fp);
    
exit:

    ry_free(fp);
    return file_size;
    
}


#if 0
ry_sts_t syc_data_to_app(int data_type)
{

    HealthRecord * health_record= (HealthRecord *)ry_malloc(sizeof(HealthRecord));
    char * file_name = NULL;
    u32_t data_size = 0;
    if(data_type == (int)SPORT_DATA_STEP){
        file_name = STEP_DATA_FILE_NAME;
        data_size = sizeof(RunRecord);
    }else if(data_type == (int)SPORT_DATA_HEART){
        file_name = HEART_DATA_FILE_NAME;
        data_size = sizeof(HeartRateRecord);
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        file_name = SLEEP_DATA_FILE_NAME;
        data_size = sizeof(SleepRecord);
    }else if(data_type == (int)SPORT_DATA_RUN){
        file_name = RUN_DATA_FILE_NAME;
        data_size = sizeof(RunRecord);
    }
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(dev_step) + 10);
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(dev_step) + 10);


    status = pb_encode(&stream, StepRecord_fields, &dev_step);
    message_length = stream.bytes_written;

    /* Then just check for any errors.. */
    if (!status) {
        LOG_ERROR("[syc_data_to_app]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        //return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        goto exit;
    }

    DataRecord * record = (DataRecord *)ry_malloc(sizeof(DataRecord));
    ry_memset(record, 0, sizeof(DataRecord));
    
    record->data_type = DataRecord_DataType_HEALTH;
    record->val.size = message_length;
    ry_memcpy(record->val.bytes, data_buf, message_length);

    ry_free(data_buf);

    data_buf = (u8_t *)ry_malloc(sizeof(DataRecord) + 10);
    
    stream = pb_ostream_from_buffer(data_buf, sizeof(DataRecord) + 10);

    status = pb_encode(&stream, DataRecord_fields, record);
    message_length = stream.bytes_written;

    //ry_free(record);

    
    DataSet * dataSet = (DataSet *)ry_malloc(sizeof(DataSet));

    memset(dataSet, 0, sizeof(DataSet));


    //get_device_id(dataSet->did, DID_LEN);
    char* test_ = "test_did";
    memcpy(dataSet->did, test_, strlen(test_) + 1);
    
    dataSet->records_count = 1;

    dataSet->records[0].data_type = DataRecord_DataType_HEALTH;

    ry_memcpy(&(dataSet->records[0]), record, sizeof(DataRecord));

    ry_free(data_buf);

    data_buf = (u8_t *)ry_malloc(sizeof(DataSet) + 10);

    stream = pb_ostream_from_buffer(data_buf, (sizeof(DataSet)));

    if (!pb_encode(&stream, DataSet_fields, dataSet)) {
        LOG_ERROR("[get_upload_data_handler] Encoding failed.\r\n");
    }
    message_length = stream.bytes_written;

    ry_free(dataSet);

    status = rbp_send_req(CMD_APP_UPLOAD_DATA, data_buf, message_length, 1);

    ry_free(data_buf);


exit:
    return status;
    
}
#endif


void syc_health_data_to_app(int data_type)
{
    /*HealthRecord * health_record= (HealthRecord *)ry_malloc(sizeof(HealthRecord));
    char * file_name = NULL;
    u32_t data_size = 0;
    u32_t read_len = 0;
    if(data_type == (int)SPORT_DATA_STEP){
        file_name = STEP_DATA_FILE_NAME;
        data_size = sizeof(StepRecord);
        health_record->which_message = HealthRecord_step_tag;
    }else if(data_type == (int)SPORT_DATA_HEART){
        file_name = HEART_DATA_FILE_NAME;
        data_size = sizeof(HeartRateRecord);
        health_record->which_message = HealthRecord_hr_tag;
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        file_name = SLEEP_DATA_FILE_NAME;
        data_size = sizeof(SleepRecord);
        health_record->which_message = HealthRecord_sleep_tag;
    }
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = NULL;

    FIL * fp = (FIL *)ry_malloc(sizeof(FIL));

    f_open(fp, file_name, FA_READ);
    
    DataRecord * dataRecord = (DataRecord *)ry_malloc(sizeof(DataRecord));

    DataSet * dataSet = (DataSet *)ry_malloc(sizeof(DataSet));


    if(data_type == (int)SPORT_DATA_STEP){
        //f_read();
    }else if(data_type == (int)SPORT_DATA_HEART){
        
    }else if(data_type == (int)SPORT_DATA_SLEEP){
        
    }
    */

}


 

DataRecord * pack_data_record(int data_type)
{
    /*HealthRecord * health_record = pack_health_record(data_type);
    if(health_record == NULL){
        return NULL;
    }
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = NULL;
    pb_ostream_t stream;
    DataRecord * dataRecord = (DataRecord *)ry_malloc(sizeof(DataRecord));
    if(dataRecord == NULL){
        goto exit;
    }
    
    ry_memset(dataRecord, 0 , sizeof(DataRecord));


    
    data_buf = (u8_t *)ry_malloc(sizeof(HealthRecord) + 10);
    if(data_buf == NULL){
        goto exit;
    }
    
    stream = pb_ostream_from_buffer(data_buf, sizeof(HealthRecord) + 10);
    status = pb_encode(&stream, HealthRecord_fields, health_record);
    message_length = stream.bytes_written;
    if (!status) {
        LOG_ERROR("[syc_data_to_app]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    dataRecord->val.size = message_length;
    ry_memcpy(dataRecord->val.bytes, data_buf, dataRecord->val.size);
    dataRecord->data_type = DataRecord_DataType_HEALTH;

exit:

    ry_free(health_record);
    ry_free(data_buf);

    return dataRecord;*/
		return RY_SUCC;
    
}


/*DataSet * pack_data_set(int data_type)
{
    //DataRecord * dataRecord = pack_data_record(data_type);
    
    DataSet * dataSet = (DataSet *)ry_malloc(sizeof(DataSet));

    if(dataSet == NULL){
        return NULL;
    }
    ry_memset(dataSet, 0, sizeof(DataSet));

    u32_t i = 0;
    DataRecord * temp = NULL;

    for(i = 0; i < DATASET_MAX_RECORD; i++){
        temp = pack_data_record(data_type);
        
        
        if(temp == NULL){
            dataSet->records_count = i;
            break;
        }else{
            dataSet->records[i] = *temp;
            dataSet->records_count = i + 1;
        }
    }

    
    return dataSet;
    
    
}*/

int get_file_record_num(char * file_name, int type)
{
    FRESULT ret = FR_OK;
    FIL * fp = NULL;
    int record_num = 0;
    int file_size = 0;

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    ret = f_open(fp, file_name, FA_READ);
    if(ret != FR_OK){
        if(FR_NO_FILE != ret){
            LOG_ERROR("[%s]-%s,%s:%d\n",__FUNCTION__, ERR_STR_FILE_OPEN,file_name, ret);
        }
        goto exit;
    }
    file_size = f_size(fp);
    if(type == DataRecord_DataType_BODY_STATUS){
        record_num = file_size/(sizeof(BodyStatusRecord));
    }else if(DataRecord_DataType_SLEEP_STATUS == type){
        record_num = file_size/(sizeof(sleep_record_rri_t));
    }else{
        LOG_ERROR("[%s]type error\n",__FUNCTION__);
        record_num = 0;
    }

exit:
    f_close(fp);
    ry_free(fp);    
    return record_num;
}

int get_file_dataSet_num(char * file_name, int type)
{
    FRESULT ret = FR_OK;
    FIL * fp = NULL;
    int record_num = 0;
    int file_size = 0;
    u32_t magic_num = 0;
    u32_t read_len = 0;
    u32_t record_size = 0;
    u32_t i = 0;
    u32_t dataSet_num = 0;
    u32_t cur_record_len = 0;
    u16_t cur_dataSet_len = 0;

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    ret = f_open(fp, file_name, FA_READ);
    if(ret != FR_OK){
        if(FR_NO_FILE != ret){
            LOG_ERROR("[%s]-%s,%s:%d\n",__FUNCTION__, ERR_STR_FILE_OPEN,file_name, ret);
        }
        goto exit;
    }
    file_size = f_size(fp);

    ret = f_read(fp, &magic_num, sizeof(u32_t), &read_len);

    if(magic_num == DATA_FILE_MAGIC){
        if(type == DataRecord_DataType_BODY_STATUS){
            record_size = sizeof(BodyStatusRecord) + sizeof(u32_t);
            record_num = (file_size - sizeof(u32_t))/(record_size);
            for(i = 0; i < record_num; i++){
                ret = f_lseek(fp, sizeof(u32_t) + i * record_size);
                if(ret != FR_OK){
                    goto exit;
                }
                ret = f_read(fp, &cur_record_len, (sizeof(u32_t)), &read_len);
                if(ret != FR_OK){
                    goto exit;
                }

                if(cur_record_len >= DATA_SET_BYTE_MAX_SIZE){
                    LOG_ERROR("[%s]--file record size fail\n", __FUNCTION__);
                    f_close(fp);
                    f_unlink(file_name);
                    goto exit;
                }
                
                if( (cur_dataSet_len + cur_record_len) >= DATA_SET_BYTE_MAX_SIZE){
                    LOG_DEBUG("[%s]---SetLen:%d---num :%d\n", __FUNCTION__, cur_dataSet_len, dataSet_num + 1);
                    cur_dataSet_len = 0;
                    dataSet_num++;
                    //break;
                }else{
                    cur_dataSet_len += cur_record_len;
                }                
            }

            if(cur_dataSet_len != 0){
                dataSet_num++;
                LOG_INFO("[%s] file is ok: %s, records: %d", __FUNCTION__, file_name, cur_dataSet_len);
            }
        }else if(DataRecord_DataType_SLEEP_STATUS == type){
            
        }
    }else{
        
        f_close(fp);
        f_unlink(file_name);
        ry_free(fp);
        
        LOG_ERROR("[%s] failed to read file: %s, deleting..", __FUNCTION__, file_name);        
        return 0;
    }

exit:
    f_close(fp);
    ry_free(fp);    
    return dataSet_num;
}


int get_data_set_num(void)
{
    u8_t i = 0;
    u8_t day = 0;
    FRESULT ret = FR_OK;
    FIL * fp = NULL;
    //BodyStatusRecord * body_status[] = desk_data;
    u32_t record_num = 0;
    char * file_name = NULL;
    int dataSet_num = 0;

    if(get_running_flag()){
        LOG_INFO("running is active, do not upload data\n");
        return 0;
    }

    if(cur_status.type == BodyStatusRecord_Type_SLEEP_LITE){
        LOG_INFO("sleep is active, do not upload data\n");
        return 0;
    }

    /*if(get_sleep_status()){
        LOG_INFO("sleeping not upload data\n");
        return 0;
    }*/
    /*fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc fail\n", __FUNCTION__);
        goto exit;
    }*/

    file_name = (char *)ry_malloc(100);
    if(file_name == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    for(i = 0; i < 7; i++){
        day = (i + get_day_number())%7;
        
        sprintf(file_name, "%d_%s", day, BODY_STATUS_DATA);
        dataSet_num += get_file_dataSet_num(file_name, DataRecord_DataType_BODY_STATUS);
        //LOG_INFO("[%s]-%d-%d", __FUNCTION__, day, dataSet_num);

        if((i == 0) && (get_sleep_status())){
            LOG_DEBUG("sleep is active, do not upload\n");
        }else{
            sprintf(file_name, "%d_%s", day, SLEEP_DATA_FILE_NAME);
            dataSet_num += get_file_dataSet_num(file_name, DataRecord_DataType_SLEEP_STATUS);
        }
    }

    ry_free(file_name);

    dataSet_num += check_retry_data();

    //LOG_INFO("1.[%s] - %d\n", __FUNCTION__, dataSet_num);

    dataSet_num += get_statistics_dataSet_num();
    //LOG_INFO("2.[%s] - %d\n", __FUNCTION__, dataSet_num);
    
    return dataSet_num;
exit:

    return -1;
}

bool encode_repeated_record(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    dataSet_repeat_param_t * param = (dataSet_repeat_param_t *)(*((u32_t *)arg));
    msg_param_t * record_param = NULL;
    int i = 0;
    
    
    for (i = 0; i < param->num; i++)
    {
        if (!pb_encode_tag_for_field(stream, field)){
            LOG_DEBUG("[%s]-{%s}1\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
            goto error;
        }
        
        /*if (!pb_encode_string(stream, (uint8_t*)param->data[i], param->len[i] )){
            LOG_DEBUG("[%s]-{%s}2\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
            return false;
        }*/
        if (!pb_encode_submessage(stream, DataRecord_fields, param->data[i])){
            LOG_DEBUG("[%s]-{%s}2\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
            goto error;
        }
        /*if (!param->data[i]->val.funcs.encode(stream, DataRecord_fields, param->data[i]->val.arg)){
            LOG_DEBUG("[%s]-{%s}2\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
            goto error;
        }*/
        record_param = (msg_param_t *)(param->data[i]->val.arg);
        ry_free(record_param->buf);
        ry_free(param->data[i]->val.arg);
        //ry_free(param->data[i]);
        ry_free(param->data[i]);
        param->data[i]= NULL;
    }

    ry_free(param);
    return true;

error:
    for(i = 0; i < param->num; i++){
        ry_free(param->data[i]->val.arg);
        ry_free(param->data[i]);
        param->data[i]= NULL;
    }
    ry_free(param);
    return false;
}


DataSet * pack_data_set(int data_type, u8_t day)
{
    //DataRecord * dataRecord = pack_data_record(data_type);
    u32_t i = 0;
    DataSet * dataSet = (DataSet *)ry_malloc(sizeof(DataSet));
    u8_t len = 0;
    u32_t read_size = 0;
    dataSet_repeat_param_t * param = NULL;
    u32_t dataSet_len = 0;

    if(dataSet == NULL){
        LOG_ERROR("[%s] malloc failed \n", __FUNCTION__);
        return NULL;
    }
    ry_memset(dataSet, 0, sizeof(DataSet));

    device_info_get(DEV_INFO_TYPE_DID, (u8_t*)dataSet->did, &len);
    //ry_memcpy(dataSet->did, device_info.did, DID_LEN);

    param = (dataSet_repeat_param_t *)ry_malloc(sizeof(dataSet_repeat_param_t));
    if(param == NULL){
        LOG_ERROR("[%s] malloc failed \n", __FUNCTION__);
        goto error;
    }
    ry_memset(param, 0, sizeof(dataSet_repeat_param_t));

    //todo:
    open_body_status_read(day);
    for(i = 0; i < DATA_SET_MAX_RECORD; i++){
        param->data[i] = (DataRecord *)ry_malloc(sizeof(DataRecord));
        if(param->data[i] == NULL){
            goto error;
        }
        param->len[i] = pack_body_data_record((DataRecord *)param->data[i], day);
        if(param->len[i] == 0){
            ry_free(param->data[i]);
            /*if(dataSet_len > 0){
                param->num++;
            }*/
            param->num = i;
            break;
        }
        dataSet_len += param->len[i];

        if(dataSet_len >= DATA_SET_BYTE_MAX_SIZE){
            //param->num++;
            param->num = i+1;
            break;
        }
    }
    close_body_status_read();

    dataSet->records.arg = param;
    dataSet->records.funcs.encode = encode_repeated_record;
    
    LOG_DEBUG("[%s]---record count %d\n", __FUNCTION__, param->num);

    if((param->num == 0)){
        FREE_PTR(dataSet);
        FREE_PTR(param);

        dataSet = get_statistics_dataSet(day);
    }
  
    return dataSet;

error:
    FREE_PTR(dataSet);
    FREE_PTR(param);
    return NULL;
}


int get_record_pack_len(BodyStatusRecord * body_record)
{
    pb_ostream_t stream_o = {0};
    u8_t * buf = (u8_t *)ry_malloc(sizeof(BodyStatusRecord));

    if(buf == NULL){
        goto error;
    }

    stream_o = pb_ostream_from_buffer(buf, (sizeof(BodyStatusRecord)));

    if (!pb_encode(&stream_o, BodyStatusRecord_fields, (body_record))) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        goto error;
    }
    
    ry_free(buf);

    LOG_DEBUG("[%s]---%d\n", __FUNCTION__, stream_o.bytes_written);

    return stream_o.bytes_written;

error:
    ry_free(buf);
    return -1;
}


void open_body_status_read(u8_t day)
{
    char * file_name = NULL;

    FRESULT ret = FR_OK;
     
    file_name = (char *)ry_malloc(80);
    if(file_name == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto error;
    }
    sprintf(file_name, "%d_%s", day, BODY_STATUS_DATA);

    if(data_service_ctx_v.cur_data_file_p != NULL){
        ry_free(data_service_ctx_v.cur_data_file_p);
        data_service_ctx_v.cur_data_file_p = NULL;
    }

    data_service_ctx_v.cur_data_file_p = (FIL *)ry_malloc(sizeof(FIL));
    if(data_service_ctx_v.cur_data_file_p == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto error;
    }

    ret = f_open(data_service_ctx_v.cur_data_file_p, file_name, FA_READ | FA_WRITE );
    if(ret != FR_OK){
        LOG_DEBUG("[%s] %s open failed\n",__FUNCTION__, file_name);
    }

    ry_free(file_name);
    return ;
error:
    LOG_ERROR("[%s]err, ret:%x\n",__FUNCTION__, ret);    
    ry_free(file_name);
    ry_free(data_service_ctx_v.cur_data_file_p);
    data_service_ctx_v.cur_data_file_p = NULL;
    return ;
}

void close_body_status_read(void)
{
    if(data_service_ctx_v.cur_data_file_p != NULL){
        f_close(data_service_ctx_v.cur_data_file_p);
        ry_free(data_service_ctx_v.cur_data_file_p);
    }
    data_service_ctx_v.cur_data_file_p = NULL;
    LOG_DEBUG("[%s]--success\n",__FUNCTION__);
}

#define MAX_BODY_RECORD         4
u32_t pack_body_status(u32_t read_record, u8_t ** desk_data, u32_t * pack_size, u8_t day)
{
    char * file_name = NULL;
    u32_t data_size = 0,file_size = 0;
    u32_t read_len = 0, read_size = 0;
    u16_t record_count = 0;
    FRESULT ret = FR_OK;
    FIL * fp = NULL;
    //BodyStatusRecord * body_status[] = desk_data;
    u32_t record_num = 0;
    u32_t offset = 0;
    pb_ostream_t stream_o = {0};
    BodyStatusRecord * body_record = NULL;
    int magic_num = 0;
    u32_t record_size = sizeof(BodyStatusRecord) + sizeof(u32_t);
    u32_t cur_record_pack_len = 0;
    u32_t real_sn = 0;

    static u32_t run_total = 0,run_session = 0;

    if(data_service_ctx_v.cur_data_file_p == NULL){
        LOG_INFO("[%s]--slow\n",__FUNCTION__);
        file_name = (char *)ry_malloc(80);
        if(file_name == NULL){
            LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        sprintf(file_name, "%d_%s", day, BODY_STATUS_DATA);

        *pack_size = 0;

        fp = (FIL *)ry_malloc(sizeof(FIL));
        if(fp == NULL){
            LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }

        ret = f_open(fp, file_name, FA_READ | FA_WRITE );
        if(ret != FR_OK){
            LOG_ERROR("[%s] %s open failed\n",__FUNCTION__, file_name);

            #if 0
            static u8_t count_i = 99;
            if(count_i != day){
                
                count_i = day;
                
                body_record = (BodyStatusRecord *)ry_malloc(sizeof(BodyStatusRecord));
                if(body_record == NULL){
                    LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
                    goto exit;
                }
                body_record->type = BodyStatusRecord_Type_STEP;
                body_record->message.step.common.end_time = ryos_utc_now();
                body_record->message.step.common.start_time = body_record->message.step.common.end_time - 500;
                body_record->message.step.common.heart_rates_count = 1;
                body_record->message.step.common.heart_rates[0].time = body_record->message.step.common.end_time - 300;
                body_record->message.step.common.heart_rates[0].rate = 90 + ryos_utc_now()%11;
                body_record->message.step.common.heart_rates[0].measure_type= HeartRateRecord_MeasureType_MANUAL;
                body_record->message.step.step = 1000 + ryos_utc_now()%17;
                body_record->message.step.freq = alg_get_step_freq_latest();
                body_record->which_message = BodyStatusRecord_step_tag;

                stream_o = pb_ostream_from_buffer(desk_data, (sizeof(BodyStatusRecord)));

                if (!pb_encode(&stream_o, BodyStatusRecord_fields, (body_record))) {
                    LOG_ERROR("[%s]-%d Encoding failed.\r\n\n\n\n\n", __FUNCTION__, __LINE__);
                }
                
                LOG_DEBUG("{%s} stream byte ---%d\n",__FUNCTION__, stream_o.bytes_written);
                *pack_size = stream_o.bytes_written;

            }


            #endif
            
            
            goto exit;
        }
    }else{
        fp = data_service_ctx_v.cur_data_file_p;
    }

    file_size = f_size(fp);

    if(file_size == 0){
        goto exit;
    }

    f_lseek(fp, 0);
    ret = f_read(fp, &magic_num, sizeof(int), &read_len);
    if(ret != FR_OK){
        //LOG_ERROR("[%s] - file read error %d\n",__FUNCTION__, ret);
        f_close(fp);
        f_unlink(file_name);
        goto exit;
    }

    if(magic_num == DATA_FILE_MAGIC){
        file_size = (file_size - sizeof(int));
    }else{
        f_close(fp);
        LOG_ERROR("[%s] - file error\n",__FUNCTION__);
        f_unlink(file_name);
        goto exit;
    }
    //file_size = (file_size - sizeof(int));

    if(file_size % record_size != 0){
        LOG_DEBUG("[%s] file format error\n",__FUNCTION__);
        f_close(fp);
        f_unlink(file_name);
        goto exit;
    }
    record_num = file_size / record_size;

    if(record_num == 0){
        LOG_DEBUG("[%s] no data need upload\n",__FUNCTION__);
        goto exit;
    }

    if(read_record >= record_num){
        offset = sizeof(u32_t);
        read_size = sizeof(BodyStatusRecord);
    }else{
        offset = file_size - (record_size * read_record) + sizeof(int);
        read_size = (sizeof(BodyStatusRecord) * read_record);
    }

    ret = f_lseek(fp, offset);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file seek failed\n",__FUNCTION__);
        goto exit;
    }

    ret = f_read(fp, &cur_record_pack_len, sizeof(u32_t), &read_len);
    if(ret != FR_OK){
        LOG_ERROR("[%s] file read failed\n",__FUNCTION__);
        goto exit;
    }
    
    /*body_status = (BodyStatusRecord *)ry_malloc(sizeof(BodyStatusRecord));
    if(body_status == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }*/
    body_record = (BodyStatusRecord *)ry_malloc(sizeof(BodyStatusRecord));
    if(body_record == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }
    

    ret = f_read(fp, body_record, read_size, &read_len);
    if(ret != FR_OK){
        LOG_ERROR("[%s] file read failed\n",__FUNCTION__);
        goto exit;
    }

    if(read_len != read_size){
        LOG_ERROR("[%s] file read size failed\n", __FUNCTION__);
        goto exit;
    }

    ret = f_lseek(fp, offset);
    if(ret != FR_OK){
        LOG_ERROR("[%s] file seek failed\n",__FUNCTION__);
        //status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    ret = f_truncate(fp);
    if(ret != FR_OK){
        LOG_ERROR("[%s] %s file truncate fail\n",__FUNCTION__, file_name);
        goto exit;
    }
    if(desk_data == NULL){
        LOG_ERROR(" desk_data is NULL\n");
        goto exit;
    }
    *desk_data = (u8_t *)ry_malloc(cur_record_pack_len + 10);
    if(*desk_data == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }
    
    stream_o = pb_ostream_from_buffer(*desk_data, (cur_record_pack_len + 10));

    if (body_record->type == BodyStatusRecord_Type_RUN){
        real_sn = body_record->message.run.common.sn;
        if(body_record->message.run.common.total != 0){
            run_total = body_record->message.run.common.total;
            //run_session = body_record->message.run.common.session_id;
            
        }else{
            //body_record->message.run.common.session_id = run_session;
            body_record->message.run.common.total = run_total + 1;
        }
        if(run_total >= body_record->message.run.common.sn){
            body_record->message.run.common.sn = run_total - body_record->message.run.common.sn;
        
        }else{
            run_total = body_record->message.run.common.sn;
            body_record->message.run.common.total = run_total + 1;
            body_record->message.run.common.sn = run_total - body_record->message.run.common.sn;

            /*if(body_record->message.run.common.session_id == 0){
                body_record->message.run.common.session_id = body_record->message.run.common.end_time;
                //run_session = body_record->message.run.common.session_id;
            }*/
        }

        //LOG_INFO("\n\nrun sn:%d , total:%d\n",body_record->message.run.common.sn,run_total);
        //LOG_INFO("session : %d\n\n\n\n",body_record->message.run.common.session_id);
    } else if(body_record->type == BodyStatusRecord_Type_SLEEP_LITE){
        real_sn = body_record->message.sleep_lite.common.sn;
        LOG_DEBUG("sleep sn : %d\n",body_record->message.sleep_lite.common.sn);
        if(body_record->message.sleep_lite.common.total != 0){
            run_total = body_record->message.sleep_lite.common.total;            
        }else{
            body_record->message.sleep_lite.common.total = run_total + 1;
        }
        if(run_total >= body_record->message.sleep_lite.common.sn){
            body_record->message.sleep_lite.common.sn = run_total - body_record->message.sleep_lite.common.sn;
        
        }else{
            run_total = body_record->message.sleep_lite.common.sn;
            body_record->message.sleep_lite.common.total = run_total + 1;
            body_record->message.sleep_lite.common.sn = run_total - body_record->message.sleep_lite.common.sn;
        }

        //LOG_INFO("\n\nsleep sn:%d , total:%d\n",body_record->message.sleep_lite.common.sn,run_total);
        //LOG_INFO("session : %d\n\n\n\n",body_record->message.sleep_lite.common.session_id);


        //test by lixueliang
        /*ry_memset(&(body_record->message.sleep_lite.common), 0, sizeof(CommonRecord));
        body_record->message.sleep_lite.step = 99;
        body_record->message.sleep_lite.type = 1;
        body_record->message.sleep_lite.version = 1;*/
        
    } else if(body_record->type == BodyStatusRecord_Type_INDOOR_RUN){
        real_sn = body_record->message.indoor_run.common.sn;
        if(body_record->message.indoor_run.common.total != 0){
            run_total = body_record->message.indoor_run.common.total;
            //run_session = body_record->message.run.common.session_id;
            
        }else{
            //body_record->message.run.common.session_id = run_session;
            body_record->message.indoor_run.common.total = run_total + 1;
        }
        if(run_total >= body_record->message.indoor_run.common.sn){
            body_record->message.indoor_run.common.sn = run_total - body_record->message.indoor_run.common.sn;
        
        }else{
            run_total = body_record->message.indoor_run.common.sn;
            body_record->message.indoor_run.common.total = run_total + 1;
            body_record->message.indoor_run.common.sn = run_total - body_record->message.indoor_run.common.sn;

            /*if(body_record->message.run.common.session_id == 0){
                body_record->message.run.common.session_id = body_record->message.run.common.end_time;
                //run_session = body_record->message.run.common.session_id;
            }*/
        }

        //LOG_INFO("\n\nrun sn:%d , total:%d\n",body_record->message.run.common.sn,run_total);
        //LOG_INFO("session : %d\n\n\n\n",body_record->message.run.common.session_id);
    } else if(body_record->type == BodyStatusRecord_Type_OUTDOOR_RIDE){
        real_sn = body_record->message.outdoor_ride.common.sn;
        if(body_record->message.outdoor_ride.common.total != 0){
            run_total = body_record->message.outdoor_ride.common.total;
            //run_session = body_record->message.run.common.session_id;
            
        }else{
            //body_record->message.run.common.session_id = run_session;
            body_record->message.outdoor_ride.common.total = run_total + 1;
        }
        if(run_total >= body_record->message.outdoor_ride.common.sn){
            body_record->message.outdoor_ride.common.sn = run_total - body_record->message.outdoor_ride.common.sn;
        
        }else{
            run_total = body_record->message.outdoor_ride.common.sn;
            body_record->message.outdoor_ride.common.total = run_total + 1;
            body_record->message.outdoor_ride.common.sn = run_total - body_record->message.outdoor_ride.common.sn;
        }
    } else if(body_record->type == BodyStatusRecord_Type_FREE){
        real_sn = body_record->message.free.common.sn;
        if(body_record->message.free.common.total != 0){
            run_total = body_record->message.free.common.total;
            //run_session = body_record->message.run.common.session_id;
            
        }else{
            //body_record->message.run.common.session_id = run_session;
            body_record->message.free.common.total = run_total + 1;
        }
        if(run_total >= body_record->message.free.common.sn){
            body_record->message.free.common.sn = run_total - body_record->message.free.common.sn;
        
        }else{
            run_total = body_record->message.free.common.sn;
            body_record->message.free.common.total = run_total + 1;
            body_record->message.free.common.sn = run_total - body_record->message.free.common.sn;

            /*if(body_record->message.run.common.session_id == 0){
                body_record->message.run.common.session_id = body_record->message.run.common.end_time;
                //run_session = body_record->message.run.common.session_id;
            }*/
        }

        //LOG_INFO("\n\nrun sn:%d , total:%d\n",body_record->message.run.common.sn,run_total);
        //LOG_INFO("session : %d\n\n\n\n",body_record->message.run.common.session_id);
    } else {
        run_total = 0;
        run_session = 0;
    }

    if(real_sn == 0){
        //LOG_INFO("{%s}-session upload ok-%d\n",__FUNCTION__, run_total);
        run_total = 0;
        run_session = 0;
        
    }

    if (!pb_encode(&stream_o, BodyStatusRecord_fields, (body_record))) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    /*LOG_DEBUG("-----------------------------------------\n");
    ry_data_dump(*desk_data, stream_o.bytes_written, ' ');
    LOG_DEBUG("-----------------------------------------\n");*/

    *pack_size = stream_o.bytes_written;


exit:
    if(data_service_ctx_v.cur_data_file_p == NULL){
        f_close(fp);
        ry_free(fp);
    }
    ry_free(file_name);
    ry_free(body_record);
    //ry_free(body_status);
    return *pack_size;

}

u32_t pack_sleep_status(u32_t read_record, u8_t * desk_data, u32_t * pack_size, u8_t day)
{
    pb_ostream_t stream_o = {0};
    u32_t get_data_ret = 0;
    *pack_size = 0;
    SleepStatusRecord * sleep_record = (SleepStatusRecord *)ry_malloc(sizeof(SleepStatusRecord));
    if(sleep_record == NULL){
        LOG_DEBUG("[%s] malloc failed \n", __FUNCTION__);
        goto exit;
    }

    memset(sleep_record, 0, sizeof(SleepStatusRecord));

    get_data_ret = get_sleep_record_data(sleep_record, day);

    stream_o = pb_ostream_from_buffer(desk_data, (sizeof(SleepStatusRecord)));

    if (!pb_encode(&stream_o, SleepStatusRecord_fields, (sleep_record))) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }
    *pack_size = stream_o.bytes_written;

    if(get_data_ret != GET_DATARECORD_RESULT_OK){
        *pack_size = 0;
    }

exit:

    ry_free(sleep_record);
    return *pack_size; 
}



u32_t get_last_body_status(BodyStatusRecord * body_record, u8_t day)
{
    char * file_name = NULL;
    u32_t data_size = 0,file_size = 0;
    u32_t read_len = 0, read_size = 0;
    u16_t record_count = 0;
    FRESULT ret = FR_OK;
    FIL * fp = NULL;
    //BodyStatusRecord * body_status[] = desk_data;
    u32_t record_num = 0;
    u32_t offset = 0;

    u32_t status = 0;

    file_name = (char *)ry_malloc(80);
    if(file_name == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }
    sprintf(file_name, "%d_%s", day, BODY_STATUS_DATA);
    

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        status = GET_DATARECORD_RESULT_MALLOC_FAILED;
        goto exit;
    }

    ret = f_open(fp, file_name, FA_READ );
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file open failed\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    file_size = f_size(fp);

    if(file_size % sizeof(BodyStatusRecord) != 0){
        LOG_DEBUG("[%s] file format error\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FORMAT_ERROR;
        goto exit;
    }
    record_num = file_size / sizeof(BodyStatusRecord);

    if(record_num == 0){
        LOG_DEBUG("[%s] no data need upload\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_EMPTY;
        goto exit;
    }

    offset = file_size - (sizeof(BodyStatusRecord));
    read_size = (sizeof(BodyStatusRecord));

    ret = f_lseek(fp, offset);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file seek failed\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }
    
    /*body_status = (BodyStatusRecord *)ry_malloc(sizeof(BodyStatusRecord));
    if(body_status == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }*/

    ret = f_read(fp, body_record, read_size, &read_len);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file read failed\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    if(read_len != read_size){
        LOG_DEBUG("[%s] file read size failed\n", __FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    

exit:
    f_close(fp);
    ry_free(fp);
    ry_free(file_name);
    return status;

}


void delete_body_status(u32_t delete_num)
{
    FIL * fp = NULL;
    char * file_name = BODY_STATUS_DATA;
    u32_t data_size = 0,file_size = 0;
    FRESULT ret = FR_OK;
    u32_t offset = 0;

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    ret = f_open(fp, file_name, FA_READ | FA_WRITE);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file open failed\n",__FUNCTION__);
        goto exit;
    }

    file_size = f_size(fp);

    if(delete_num * sizeof(BodyStatusRecord) >= file_size){
        offset = 0;
    }else{
        offset = file_size - (sizeof(BodyStatusRecord) * delete_num);
    }

    ret = f_lseek(fp, offset);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file seek failed\n",__FUNCTION__);
        goto exit;
    }

    
    ret = f_truncate(fp);
    if(ret != FR_OK){
        LOG_ERROR("[%s] %s file truncate fail\n",__FUNCTION__, file_name);
        goto exit;
    }

exit:
    f_close(fp);
    ry_free(fp);
    
}
bool encode_data_record(pb_ostream_t *stream, const pb_field_t *field, void * const * arg)
{
    msg_param_t * param = (msg_param_t *)(*((u32_t *)arg));
    if (!pb_encode_tag_for_field(stream, field)){
        LOG_DEBUG("[%s]-{%s}1\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        goto error;
    }
    
    if (!pb_encode_string(stream, (uint8_t*)param->buf, param->len )){
        LOG_DEBUG("[%s]-{%s}2\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        return false;
    }
    /*if (!pb_encode_submessage(stream, DataRecord_fields, param->buf)){
        LOG_DEBUG("[%s]-{%s}2\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        goto error;
    }*/


    //ry_free(param->buf);
    //param->buf = NULL;
    return true;
error:
    //ry_free(param->buf);
    //param->buf = NULL;
    return false;
}


u32_t pack_body_data_record(DataRecord * data_record, u8_t day)
{
    u32_t read_size = 0;
    msg_param_t * param = (msg_param_t *)ry_malloc(sizeof(msg_param_t));
    if(param == NULL){
        goto exit;
    }else{
        ry_memset(param, 0, sizeof(msg_param_t));
    }
    if(data_record == NULL){
        goto exit;
    }
    

    if(data_record == NULL){
        goto exit;
    }else{
        ry_memset(data_record, 0, sizeof(DataRecord));
    }
    
    data_record->data_type = DataRecord_DataType_BODY_STATUS;

    data_record->val.funcs.encode = encode_data_record;
    data_record->val.arg = param;
    
    read_size = pack_body_status(1, &(param->buf), (u32_t *)&(param->len) , day);
    //data_record->val.size = pack_body_status(1, data_record->val.bytes, &read_size , day);

    if(read_size == 0){
        ry_free(param);
        data_record->val.arg = NULL;
    }


exit:
    return read_size;
}

u32_t pack_sleep_data_record(DataRecord * data_record, u8_t day)
{
    u32_t read_size = 0;
	
    if(data_record == NULL){
        goto exit;
    }
    
    data_record->data_type = DataRecord_DataType_SLEEP_STATUS;
    
    //read_size = pack_sleep_status(1, data_record->val.bytes, (u32_t *)&(data_record->val.size) , day);

   /* if((day == get_day_number()) && (get_sleep_status())){
        data_record->val.size = 0;
        read_size = 0;
    }else{
        data_record->val.size = pack_sleep_status(1, data_record->val.bytes, &read_size , day);
    }*/


exit:
    return read_size;
}


/*void data_upload(int data_type)
{
    DataSet * dataSet = pack_data_set(data_type);
    size_t message_length;
    ry_sts_t status;
    u8_t *data_buf = NULL;
    pb_ostream_t stream;
    if(dataSet == NULL){
        LOG_ERROR("[%s] DataSet is NULL \n", __FUNCTION__);
        goto exit;
    }

    data_buf = (u8_t *)ry_malloc(sizeof(DataSet) + 10);
    if(data_buf == NULL){
        LOG_ERROR("[%s] malloc fail %d\n", __FUNCTION__, sizeof(DataSet) + 10);
        goto exit;
    }
    
    
    stream = pb_ostream_from_buffer(data_buf, (sizeof(DataSet)));

    if (!pb_encode(&stream, DataSet_fields, dataSet)) {
        LOG_ERROR("[get_upload_data_handler] Encoding failed.\r\n");
    }
    message_length = stream.bytes_written;

    LOG_DEBUG("data upload type --- %d\n", data_type);
    
    status = rbp_send_req(CMD_APP_UPLOAD_DATA, data_buf, message_length, 1);

exit:
    ry_free(dataSet);
    ry_free(data_buf);
}*/


u32_t check_retry_data(void)
{
    FIL * fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_byte = 0;  
    u32_t dataSetNum = 0;

    fp = (FIL *)ry_malloc( sizeof(FIL) );
    if(fp == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    ret = f_open(fp, RETRY_UPLOAD_DATA, FA_WRITE | FA_OPEN_APPEND);
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        dataSetNum = 0;
    }else{
        //dataSetNum = f_size(fp)/sizeof(DataSet);
        if(f_size(fp) > sizeof(DataSet)){
            dataSetNum = 1;
        }else{
            dataSetNum = 0;
        }
    }

exit:
    f_close(fp);
    ry_free(fp);

    return dataSetNum;
}

u8_t * pack_retry_data(u32_t * size)
{
    
    FIL * fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_byte = 0;  
    u8_t * dataSet = NULL;

    dataSet = (u8_t *)ry_malloc(DATA_SET_MAX_SIZE);
    if(dataSet == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__,ERR_STR_MALLOC_FAIL);
        goto exit;
    }

    fp = (FIL *)ry_malloc( sizeof(FIL) );
    if(fp == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__,ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    ret = f_open(fp, RETRY_UPLOAD_DATA, FA_WRITE | FA_READ);
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        goto exit;
    }else{
        *size = f_size(fp);
    }

    /*ret = f_lseek(fp, f_size(fp) - sizeof(DataSet));
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_SEEK);
        goto exit;
    }*/

    ret = f_read(fp, dataSet, f_size(fp), &written_byte);
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_READ);
        goto exit;
    }
    
    f_close(fp);
    ry_free(fp);

    f_unlink(RETRY_UPLOAD_DATA);
    
    return dataSet;

exit:
    f_close(fp);
    ry_free(fp);
    ry_free(dataSet);
    return NULL;
    
}


void data_upload_stop_handle(void* arg)
{
    LOG_INFO("[%s]\n", __FUNCTION__);

    if(data_upload_timer != 0){
        ry_timer_stop(data_upload_timer);    
        ry_timer_delete(data_upload_timer);
        data_upload_timer = 0;
    }
    set_device_session(DEV_SESSION_IDLE);
}

u32_t data_upload(u32_t upload_status)
{

    DataSet * dataSet = NULL;
    static u8_t i = 0;
    pb_ostream_t stream_o = {0};
    ry_sts_t status = 0;
    u8_t *buf = NULL;

    if(data_upload_timer == 0){
        
        ry_timer_parm timer_para;
        timer_para.timeout_CB = data_upload_stop_handle;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "upload";
        data_upload_timer = ry_timer_create(&timer_para);

        if (data_upload_timer == RY_ERR_TIMER_ID_INVALID) {
            add_reset_count(CARD_RESTART_COUNT);            
            // LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
            ry_system_reset();
        }
    }

    ry_timer_stop(data_upload_timer);
    ry_timer_start(data_upload_timer, 30000, data_upload_stop_handle, NULL);
    
    //ble_updateConnPara(6, 24, 500, 0);

    ryos_delay_ms(10);

    set_device_session(DEV_SESSION_UPLOAD_DATA);

    if(check_retry_data() == 0){
    
        for(; i < 7; i++){
            LOG_DEBUG("day : %d --- %d\n", i, ryos_get_free_heap());
            dataSet = pack_data_set(0,(i + get_day_number())%7);

            if(dataSet == NULL){
                LOG_DEBUG("[%s] empty----%d\n", __FUNCTION__,ryos_get_free_heap());
                if(i == 6){
                    i = 0;
                    
                    ble_send_request(CMD_APP_UPLOAD_DATA_REMOTE,  NULL, 0, 1, data_ble_send_callback, (void *)__FUNCTION__);
                    LOG_ERROR("[%s] upload stop - no data\n", __FUNCTION__);
                    data_upload_stop_handle(NULL);
                    clear_save_data();
                    return 0;
                }
                continue;
            }else{
                
                buf = (u8_t *)ry_malloc(2048);
                if(buf == NULL){
                    ry_free(dataSet);
                    continue;
                }
                stream_o = pb_ostream_from_buffer(buf, 2048);
                msg_param_t * retry_param = (msg_param_t *)ry_malloc(sizeof(msg_param_t));
                
                if (!pb_encode(&stream_o, DataSet_fields, (dataSet))) {
                    LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
                    //goto exit;
                    ry_free(buf);
                    continue;
                }

                LOG_DEBUG("-----------------------------------------\n");
                ry_data_dump(buf, stream_o.bytes_written, ' ');
                LOG_DEBUG("-----------------------------------------\n");

                LOG_DEBUG("stream bytes ---- %d----%d\n", stream_o.bytes_written,ryos_get_free_heap());

                retry_param->len = stream_o.bytes_written;
                retry_param->buf = buf;
                //ry_data_msg_send(DATA_SERVICE_UPLOAD_DATA_START, 0, NULL);
                status = ble_send_request(CMD_APP_UPLOAD_DATA_LOCAL,  buf, stream_o.bytes_written, 1,data_ble_upload_callback, (void *)retry_param);
                //ry_free(buf);
                ry_free(dataSet);

                return status;
            }
        }
    }else{
        u32_t retry_size = 0;
        LOG_DEBUG("retry upload---%d\n",ryos_get_free_heap());
        buf = pack_retry_data(&retry_size);
        
        if(buf == NULL){
            f_unlink(RETRY_UPLOAD_DATA);
        }else{
            msg_param_t * retry_param = (msg_param_t *)ry_malloc(sizeof(msg_param_t));
            retry_param->buf = buf;
            retry_param->len = retry_size;
            
            LOG_DEBUG("stream bytes ---- %d\n", retry_param->len);
            //ry_data_msg_send(DATA_SERVICE_UPLOAD_DATA_START, 0, NULL);
            status = ble_send_request(CMD_APP_UPLOAD_DATA_LOCAL,  buf, retry_param->len, 1,data_ble_upload_callback, (void *)retry_param);
            //ry_free(buf);
            //ry_free(dataSet);

            return status;
        }
    }

    if(i == 7){
        ble_send_request(CMD_APP_UPLOAD_DATA_REMOTE,  NULL, 0, 1, data_ble_send_callback, (void *)__FUNCTION__);
        LOG_ERROR("[%s] upload stop - no data 2\n", __FUNCTION__);
        data_upload_stop_handle(NULL);
        i = 0;
    }

    return 0;
}

/*void data_upload_loop(void)
{
    static sport_data_type_t i;

    data_upload((sport_data_type_t)i++);

}*/



ry_queue_t *ry_data_service_msgQ;

ryos_thread_t *data_service_handle;

ry_sts_t ry_data_msg_send(u32_t msgType, u32_t len, u8_t* data)
{
    ry_sts_t status = RY_SUCC;

    if (data_service_enable_status_get() == 0){
        return status;
    }

    if(ry_data_service_msgQ == NULL){
        LOG_DEBUG("[%s] queue is NULL\n",__FUNCTION__);
        return status;
    }
    ry_data_msg_t *p = (ry_data_msg_t*)ry_malloc(sizeof(ry_data_msg_t)+len);
    if (NULL == p) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        return RY_SET_STS_VAL(RY_CID_SCHED, RY_ERR_NO_MEM);
    }   

    p->msgType = msgType;
    p->len     = len;
    p->data    = (u8_t *)ry_malloc(len);
    if (data) {
        ry_memcpy(p->data, data, len);
    }

    if (RY_SUCC != (status = ryos_queue_send(ry_data_service_msgQ, &p, 4))) {
        LOG_ERROR("[ry_data_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
        ry_free(p->data);
        ry_free((void*)p);
    } 

    return status;
}


void stop_record_sleep(void)
{
    FIL * fp = NULL;
    FRESULT ret;
    u32_t written_bytes = 0;
    u32_t file_size;
    //cur_sleep_record_data->end_time = ryos_utc_now();
    char * today_file_name = NULL;
    int i = 0;

    if(cur_sleep_record_data != NULL){
        //cur_sleep_record_data->end_time = ryos_rtc_now();
        cur_sleep_record_data->end_time = ryos_rtc_now_notimezone();
        cur_sleep_record_data->step = alg_get_step_today() - cur_sleep_record_data->step;
        
        LOG_INFO("sleep end, time: %d, step: %d",
            cur_sleep_record_data->end_time,
            cur_sleep_record_data->step);
        //cur_sleep_record_data->session_id = sports_global.sleep_session_id;
    }else{
        LOG_ERROR("[%s]can't stop and save data\n", __FUNCTION__);
        goto exit;
    }

    today_file_name = (char *)ry_malloc(80);
    if(today_file_name == NULL){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }

    //sprintf(today_file_name, "%d_%s",get_day_number(), SLEEP_DATA_FILE_NAME);
    

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }

    i = 0;
    do{
        sprintf(today_file_name, "%d_%s",get_day_number(), SLEEP_DATA_FILE_NAME);
        ret = f_open(fp, today_file_name, FA_OPEN_APPEND | FA_WRITE);
        if(ret == FR_DENIED){ //full,delete data 
            delete_one_day_data((7-i+6+get_day_number())%7);
        }
        i++;
    }while((ret != FR_OK) && (i != 7));
    
    if(ret != FR_OK){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        goto exit;
    }
    file_size = f_size(fp);

    i= 0;
    do{
        ret = f_write(fp, cur_sleep_record_data, sizeof(sleep_record_rri_t), &written_bytes);
        if(ret == FR_DENIED){ //full,delete data 
            delete_one_day_data((7-i+6+get_day_number())%7);
        }
        i++;
    }while((ret != FR_OK) && (i != 7));
    
    if(ret != FR_OK){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_WRITE);
        goto exit;
    }

    if(written_bytes != sizeof(sleep_record_rri_t)){
        f_lseek(fp, file_size);
        ret = f_truncate(fp);
        if(ret != FR_OK){
            f_close(fp);
            f_unlink(SLEEP_DATA_FILE_NAME);
            LOG_ERROR("[%s] sleeps data is failed and was clear\n", __FUNCTION__);
        }else{
            LOG_ERROR("[%s] back up data success\n", __FUNCTION__);
        }
    }

    LOG_INFO("sleep record stopped");    
    
exit:
    FREE_PTR(cur_sleep_record_data);
    f_close(fp);
    ry_free(fp);
    ry_free(today_file_name);
    LOG_DEBUG("{%s}\n",__FUNCTION__);
    return; 
}

u32_t get_sleep_status(void)
{
    u32_t getup_tick = 0;
    if(sports_global.is_sleep == 0){
        
    }else{
        ry_time_t time;
        tms_get_time(&time);
        if((10 >= time.hour)&&(time.hour >= 7)){
            getup_tick = getup_tick_quick;
        }else{
            getup_tick = getup_tick_normal;
        }
        if(sports_global.end_sleep_tick != 0){
            if((ryos_utc_now() - sports_global.end_sleep_tick) >= getup_tick){
                //exit sleep status
                sports_global.is_sleep = 0;
                sports_global.end_sleep_tick = 0;
                sports_global.sleep_sn = 0;
                LOG_DEBUG("[%s]SN clear %d\n",__FUNCTION__,__LINE__);
                sports_global.sleep_session_id = 0;
            }
        }

    }

    if(sports_global.is_sleep){
        LOG_DEBUG("\n\n sleeping\n\n");
    }else{
        LOG_DEBUG("\n\nnot sleeping\n\n");

    }

    if(cur_status.type == BodyStatusRecord_Type_SLEEP_LITE){
        return 1;
    }

    return sports_global.is_sleep;
}

void set_sleep_status(void)
{
    sports_global.is_sleep = 1;
    sports_global.end_sleep_tick = 0;
    sports_global.sleep_session_id = ryos_utc_now();
    sports_global.sleep_sn = 0;
    LOG_DEBUG("[%s]SN clear %d\n",__FUNCTION__,__LINE__);
}


void start_record_sleep(void)
{
    if(cur_sleep_record_data == NULL){
        
        
    }else{
        stop_record_sleep();
    }
    //sports_global.sleep_session_id = ryos_rtc_now()/86400;
    
    cur_sleep_record_data = (sleep_record_rri_t *)ry_malloc(sizeof(sleep_record_rri_t));
    if(cur_sleep_record_data == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    memset(cur_sleep_record_data, 0, sizeof(sleep_record_rri_t));
    sports_global.sleep_sn++;
    cur_sleep_record_data->sn = sports_global.sleep_sn;
    //cur_sleep_record_data->start_time = ryos_rtc_now();
    cur_sleep_record_data->start_time = ryos_rtc_now_notimezone();
    cur_sleep_record_data->session_id = sports_global.sleep_session_id;
    cur_sleep_record_data->step = alg_get_step_today();
    LOG_DEBUG("{%s}\n",__FUNCTION__);
    
    LOG_INFO("sleep record started");

exit:
    return;
}

void set_record_sleep_data(u32_t rri_data)
{
    if(cur_sleep_record_data == NULL){
        LOG_DEBUG("{%s} sleep not start\n",__FUNCTION__);
        return;
    }
    if(cur_sleep_record_data->rri_data_count == 90){
        LOG_DEBUG("{%s}--%d---%x\n",__FUNCTION__, cur_sleep_record_data->rri_data_count ,rri_data);
        return ;
    }
    cur_sleep_record_data->rri_data[cur_sleep_record_data->rri_data_count] = rri_data;
    cur_sleep_record_data->rri_data_count = (cur_sleep_record_data->rri_data_count + 1);
    LOG_DEBUG("{%s}--%d---%x\n", __FUNCTION__, cur_sleep_record_data->rri_data_count, rri_data);
}

u32_t get_sleep_record_data(SleepStatusRecord * sleep_record, u8_t day)
{
    char * file_name = NULL;
    u32_t data_size = 0,file_size = 0;
    u32_t read_len = 0, read_size = 0;
    u16_t record_count = 0;
    FRESULT ret = FR_OK;
    FIL * fp = NULL;
    //BodyStatusRecord * body_status[] = desk_data;
    u32_t record_num = 0;
    u32_t offset = 0;
    u16_t i;
    static u32_t data_total = 0;

    u32_t status = 0;
    sleep_record_rri_t * temp_sleep_record_data = NULL;

    file_name = (char *)ry_malloc(80);
    if(file_name == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        status = GET_DATARECORD_RESULT_MALLOC_FAILED;
        goto exit;
    }
    sprintf(file_name, "%d_%s", day, SLEEP_DATA_FILE_NAME);
    
    temp_sleep_record_data = (sleep_record_rri_t *)ry_malloc(sizeof(sleep_record_rri_t));
    if(temp_sleep_record_data == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        status = GET_DATARECORD_RESULT_MALLOC_FAILED;
        goto exit;
    }

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        status = GET_DATARECORD_RESULT_MALLOC_FAILED;
        goto exit;
    }

    ret = f_open(fp, file_name, FA_READ | FA_WRITE);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] %s open failed\n",__FUNCTION__, file_name);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    file_size = f_size(fp);

    if(file_size % sizeof(sleep_record_rri_t) != 0){
        LOG_DEBUG("[%s] file format error\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FORMAT_ERROR;
        goto exit;
    }
    record_num = file_size / sizeof(sleep_record_rri_t);
    /*if(data_total == 0){
        data_total = record_num;
    }*/

    if(record_num == 0){
        LOG_DEBUG("[%s] no data need upload\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_EMPTY;
        data_total = 0;
        goto exit;
    }

    offset = file_size - (sizeof(sleep_record_rri_t));
    read_size = (sizeof(sleep_record_rri_t));

    ret = f_lseek(fp, offset);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file seek failed\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }
    
    /*body_status = (BodyStatusRecord *)ry_malloc(sizeof(BodyStatusRecord));
    if(body_status == NULL){
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }*/

    ret = f_read(fp, temp_sleep_record_data, read_size, &read_len);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file read failed\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    ret = f_lseek(fp, offset);
    if(ret != FR_OK){
        LOG_DEBUG("[%s] file seek failed\n",__FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    ret = f_truncate(fp);
    if(ret != FR_OK){
        LOG_ERROR("[%s] %s file truncate fail\n",__FUNCTION__, file_name);
        goto exit;
    }
    

    if(read_len != read_size){
        LOG_DEBUG("[%s] file read size failed\n", __FUNCTION__);
        status = GET_DATARECORD_RESULT_FILE_FAILED;
        goto exit;
    }

    if((data_total == 0)||(data_total < temp_sleep_record_data->sn)){
        data_total = temp_sleep_record_data->sn;
    }

    sleep_record->rris_count = temp_sleep_record_data->rri_data_count;

    for(i = 0; i < temp_sleep_record_data->rri_data_count; i++){
        //memcpy(&(sleep_record->rris[i].bytes), &(temp_sleep_record_data->rri_data[i]), 4);
        sleep_record->rris[i] = temp_sleep_record_data->rri_data[i];
        //sleep_record->rris[i].size = 4;
    }
    /*if(record_num > data_total){
        data_total = record_num;
    }*/
    sleep_record->version = 1;
    sleep_record->common.total = data_total;
    sleep_record->common.sn = data_total - temp_sleep_record_data->sn;
    sleep_record->common.end_time = temp_sleep_record_data->end_time;
    sleep_record->common.start_time = temp_sleep_record_data->start_time;
    sleep_record->common.has_deviation = 1;
    sleep_record->common.deviation = get_timezone();
    sleep_record->common.heart_rates_count = 0;
    sleep_record->common.session_id = temp_sleep_record_data->session_id;
    sleep_record->step = temp_sleep_record_data->step;

    if(temp_sleep_record_data->sn == 1){
        data_total = 0;
    }

exit:
    f_close(fp);
    ry_free(fp);
    ry_free(temp_sleep_record_data);
    ry_free(file_name);
    return status;
}




void stop_record_run(void)
{
    SportRunStopParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    buf = (u8_t *)ry_malloc(sizeof(SportRunStopParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRunStopParam) + 10 );

    if (!pb_encode(&stream, SportRunStopParam_fields, &param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}++++++++++++++++++++++++++%d\n", __FUNCTION__,param.session_id);
    memset(&dev_locat, 0, sizeof(dev_locat));
    rbp_send_req(CMD_APP_SPORT_RUN_STOP, buf, stream.bytes_written, 1);

    sports_global.cur_session_id = 0;

    ry_free(buf);
    
}

void stop_record_ride(void)
{
    SportRideStopParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    buf = (u8_t *)ry_malloc(sizeof(SportRideStopParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRideStopParam) + 10 );

    if (!pb_encode(&stream, SportRideStopParam_fields, &param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}++++++++++++++++++++++++++%d\n", __FUNCTION__,param.session_id);
    memset(&dev_locat, 0, sizeof(dev_locat));
    rbp_send_req(CMD_APP_SPORT_RIDE_STOP, buf, stream.bytes_written, 1);

    sports_global.cur_session_id = 0;

    ry_free(buf);
    
}

void pause_record_run(void)
{
    SportRunPauseParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    buf = (u8_t *)ry_malloc(sizeof(SportRunPauseParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRunPauseParam) + 10 );

    if (!pb_encode(&stream, SportRunPauseParam_fields, &param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d\n", __FUNCTION__,param.session_id);
    rbp_send_req(CMD_APP_SPORT_RUN_PAUSE, buf, stream.bytes_written, 1);

    ry_free(buf);
}

void pause_record_ride(void)
{
    SportRidePauseParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    buf = (u8_t *)ry_malloc(sizeof(SportRidePauseParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRidePauseParam) + 10 );

    if (!pb_encode(&stream, SportRidePauseParam_fields, &param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d\n", __FUNCTION__,param.session_id);
    rbp_send_req(CMD_APP_SPORT_RIDE_PAUSE, buf, stream.bytes_written, 1);

    ry_free(buf);
}


void resume_record_run(void)
{
    SportRunResumeParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    buf = (u8_t *)ry_malloc(sizeof(SportRunResumeParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRunResumeParam) + 10 );

    if (!pb_encode(&stream, SportRunResumeParam_fields, &param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d\n", __FUNCTION__, param.session_id);
    rbp_send_req(CMD_APP_SPORT_RUN_RESUME, buf, stream.bytes_written, 1);

    ry_free(buf);    
}

void resume_record_ride(void)
{
    SportRideResumeParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    buf = (u8_t *)ry_malloc(sizeof(SportRideResumeParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRideResumeParam) + 10 );

    if (!pb_encode(&stream, SportRideResumeParam_fields, &param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d\n", __FUNCTION__, param.session_id);
    rbp_send_req(CMD_APP_SPORT_RIDE_RESUME, buf, stream.bytes_written, 1);

    ry_free(buf);    
}


void update_run_param(void)
{
    SportRunUpdateParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    if (gps_location_v.gps_request_cnt > 0){
         if (gps_location_v.gps_got_cnt > 0){
            if (gps_location_v.gps_got_cnt == GPS_STATUS_LOST_THRESH){
                gps_location_changed_status_set(GPS_LOC_CHANGED_STATUS_LOST);
                if (1){
                    u8_t msg = GPS_LOC_CHANGED_STATUS_LOST;
                    LOG_DEBUG("+++++++++++++++++++++++++++%d, send GPS_LOC_CHANGED_STATUS_LOST\n\r", msg);                    
                    ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 1, &msg);
                }
            }
            gps_location_v.gps_got_cnt --;
        }
        else{
            gps_location_changed_status_set(GPS_LOC_CHANGED_STATUS_NONE);            
        }
    }
    gps_location_v.gps_request_cnt ++;
    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d, gps_request_cnt:%d, gps_got_cnt:%d\n", \
                __FUNCTION__, param.session_id, gps_location_v.gps_request_cnt, gps_location_v.gps_got_cnt);

    if(ry_ble_get_state() == RY_BLE_STATE_CONNECTED){
        buf = (u8_t *)ry_malloc(sizeof(SportRunUpdateParam) + 10);
        pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRunUpdateParam) + 10 );

        if (!pb_encode(&stream, SportRunUpdateParam_fields, &param)) {
            LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        }

        if (get_device_connection() == DEV_BLE_LOGIN){
            rbp_send_req(CMD_APP_SPORT_RUN_UPDATE, buf, stream.bytes_written, 1);
        }

        ry_free(buf);
    }    
}

void update_ride_param(void)
{
    SportRideUpdateParam param = {0};
    u8_t * buf = NULL;
    param.session_id = sports_global.cur_session_id;

    if (gps_location_v.gps_request_cnt > 0){
         if (gps_location_v.gps_got_cnt > 0){
            if (gps_location_v.gps_got_cnt == GPS_STATUS_LOST_THRESH){
                gps_location_changed_status_set(GPS_LOC_CHANGED_STATUS_LOST);
                if (1){
                    u8_t msg = GPS_LOC_CHANGED_STATUS_LOST;
                    LOG_DEBUG("+++++++++++++++++++++++++++%d, send GPS_LOC_CHANGED_STATUS_LOST\n\r", msg);                    
                    ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 1, &msg);
                }
            }
            gps_location_v.gps_got_cnt --;
        }
        else{
            gps_location_changed_status_set(GPS_LOC_CHANGED_STATUS_NONE);            
        }
    }
    gps_location_v.gps_request_cnt ++;
    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d, gps_request_cnt:%d, gps_got_cnt:%d\n", \
                __FUNCTION__, param.session_id, gps_location_v.gps_request_cnt, gps_location_v.gps_got_cnt);

    if(ry_ble_get_state() == RY_BLE_STATE_CONNECTED){
        buf = (u8_t *)ry_malloc(sizeof(SportRideUpdateParam) + 10);
        pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRideUpdateParam) + 10 );

        if (!pb_encode(&stream, SportRideUpdateParam_fields, &param)) {
            LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        }

        if (get_device_connection() == DEV_BLE_LOGIN){
            rbp_send_req(CMD_APP_SPORT_RIDE_UPDATE, buf, stream.bytes_written, 1);
        }

        ry_free(buf);
    }    
}

uint32_t gps_location_succ_get(void)
{
    return (gps_location_v.gps_got_cnt >= GPS_STATUS_LOST_THRESH);
}


uint32_t gps_location_changed_status_get(void)
{
    return gps_location_v.gps_changed_st;
}

void gps_location_changed_status_set(uint32_t changed_st)
{
    gps_location_v.gps_changed_st = changed_st;
}

void gps_location_succ_clear(void)
{
    gps_location_v.gps_got_cnt = 0;
}

ry_sts_t update_run_result(u8_t * data, u32_t size)
{
    SportRunUpdateResult result_data = {0};
    ry_sts_t status = RY_SUCC;
    u32_t code = RBP_RESP_CODE_SUCCESS;

    pb_istream_t stream = pb_istream_from_buffer(data, size);
    
    if (0 == pb_decode(&stream, SportRunUpdateResult_fields, &result_data)){
        LOG_ERROR("Update run Decoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        //return status;
        code = RBP_RESP_CODE_DECODE_FAIL;
        goto exit;
    }

    gps_location_v.gps_request_cnt = 0;

    if (result_data.gps_status){
        if (gps_location_v.gps_got_cnt < GPS_ACCESS_CNT_MAX){
            if (gps_location_v.gps_got_cnt == 0){
                gps_location_changed_status_set(GPS_LOC_CHANGED_STATUS_RECOVERY);
                if (1){
                    u8_t msg = GPS_LOC_CHANGED_STATUS_RECOVERY;
                    LOG_DEBUG("+++++++++++++++++++++++++++%d, send GPS_LOC_CHANGED_STATUS_RECOVERY\n\r", msg);                    
                    ry_sched_msg_send(IPC_MSG_TYPE_SPORTS_READY, 1, &msg);
                }
            }
            else{
                gps_location_changed_status_set(GPS_LOC_CHANGED_STATUS_NONE);            
            }
            gps_location_v.gps_got_cnt ++;            
        }
    }
    else{
        if (gps_location_v.gps_got_cnt > 0){
            gps_location_v.gps_got_cnt --;
        }
    }

    LOG_DEBUG("[%s] update location --- %f, gps_status:%d, gps_got_cnt:%d\n", \
        __FUNCTION__, dev_locat.last_distance, result_data.gps_status, gps_location_v.gps_got_cnt);
    //status = rbp_send_resp(CMD_APP_SPORT_RUN_UPDATE, code, NULL, 0, 1);
    // LOG_DEBUG("{%s}\n", __FUNCTION__);
    if(get_running_flag() == 1){
        dev_locat.last_distance = result_data.distance;
        set_run_point(result_data.distance);
    }


exit:  
    
    return status;
}


#if 0
void set_run_point(void)
{
    //cur_status.message.run.point
    u32_t index = cur_status.message.run.point_count;
    cur_status.message.run.point[cur_status.message.run.point_count].time = ryos_utc_now();

    if(cur_status.message.run.point_count == 0){
        cur_status.message.run.point[cur_status.message.run.point_count].step = \
            alg_get_step_today() - cur_status.message.run.point[cur_status.message.run.point_count].step;
    }else{
        cur_status.message.run.point[cur_status.message.run.point_count].step = \
            alg_get_step_today() - cur_status.message.run.point[cur_status.message.run.point_count - 1].step;
    }
    cur_status.message.run.point[cur_status.message.run.point_count].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    cur_status.message.run.point[cur_status.message.run.point_count].calorie = get_step_calorie(cur_status.message.run.point[index].step);

    cur_status.message.run.point[cur_status.message.run.point_count].has_longitude = 1;
    cur_status.message.run.point[cur_status.message.run.point_count].longitude = dev_locat.longitude;

    cur_status.message.run.point[cur_status.message.run.point_count].has_latitude = 1;
    cur_status.message.run.point[cur_status.message.run.point_count].latitude = dev_locat.latitude;

    cur_status.message.run.point[cur_status.message.run.point_count].has_distance = 1;

    if(cur_status.message.run.point_count == 0){
        cur_status.message.run.point[cur_status.message.run.point_count].distance = dev_locat.last_distance;
    }else{
        cur_status.message.run.point[cur_status.message.run.point_count].distance = \
            dev_locat.last_distance + cur_status.message.run.point[cur_status.message.run.point_count - 1].distance;
    }
    

    cur_status.message.run.point_count++;

    if(cur_status.message.run.point_count >= 5){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        stop_cur_record();
        start_cur_record(SPORT_RUN_MODE);
    }
    
}
#else
void set_run_point(float distance)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static float point_distance_last;

    if (point_distance_last > distance){
        point_distance_last = distance;
        point_cnt = 0;
    }
    u32_t index = cur_status.message.run.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.run.point[index].time = ryos_rtc_now();
    cur_status.message.run.point[index].time = ryos_rtc_now_notimezone();    
    uint32_t cur_today_step = alg_get_step_today();

    if(index == 0){
        cur_status.message.run.point[index].step = \
            (cur_today_step >= cur_status.message.run.point[index].step)? \
                (cur_today_step - cur_status.message.run.point[index].step) : (cur_today_step);
    }else{
        cur_status.message.run.point[index].step = \
            (cur_today_step >= sports_global.last_step) ? \
            (cur_today_step - sports_global.last_step) : (cur_today_step);
    }
    cur_status.message.run.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    float calorie_report = get_step_calorie(cur_status.message.run.point[index].step);
    float calorie_distance = alg_calc_calorie_by_distance((distance - point_distance_last), 1);
    if (calorie_distance > calorie_report){
        data_calorie_offset_update(calorie_distance - calorie_report);
        calorie_report = calorie_distance;        
    }
    cur_status.message.run.point[index].calorie = (int)calorie_distance;

    cur_status.message.run.point[index].has_longitude = 0;

    cur_status.message.run.point[index].has_latitude = 0;

    cur_status.message.run.point[index].has_distance = 1;

    cur_status.message.run.point[index].distance = distance;
    cur_status.message.run.point[index].gps_status = gps_location_succ_get();
    
    //LOG_DEBUG("point_cnt:%d, cur_Point:%d, distance:%d, step:%d, step_freq:%d, step_today:%d\n\r", \
        point_cnt, cur_status.message.run.point_count, \
        (int)(cur_status.message.run.point[index].distance),\
        cur_status.message.run.point[index].step,\
        cur_status.message.run.point[index].calorie,\
        cur_today_step);

    point_cnt ++;
    point_distance_last = distance; 
    cur_status.message.run.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.run.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_RUN_MODE);
    }    
}

#endif

void set_ride_point(float distance)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static float point_distance_last;

    if (point_distance_last > distance){
        point_distance_last = distance;
        point_cnt = 0;
    }
    u32_t cur_today_step = alg_get_step_today();
    u32_t index = cur_status.message.outdoor_ride.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.outdoor_ride.point[index].time = ryos_rtc_now();
    cur_status.message.outdoor_ride.point[index].time = ryos_rtc_now_notimezone();    

    if(index == 0){
        cur_status.message.outdoor_ride.point[index].step = \
            cur_today_step - cur_status.message.outdoor_ride.point[index].step;
    }else{
        cur_status.message.outdoor_ride.point[index].step = \
            cur_today_step - sports_global.last_step;
    }
    cur_status.message.outdoor_ride.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    float calorie_report = get_step_calorie(cur_status.message.outdoor_ride.point[index].step);
    float calorie_distance = alg_calc_calorie_by_distance((distance - point_distance_last), 1);
    if (calorie_distance > calorie_report){
        data_calorie_offset_update(calorie_distance - calorie_report);
        calorie_report = calorie_distance;        
    }
    cur_status.message.outdoor_ride.point[index].calorie = (int)calorie_distance;

    cur_status.message.outdoor_ride.point[index].has_longitude = 0;

    cur_status.message.outdoor_ride.point[index].has_latitude = 0;

    cur_status.message.outdoor_ride.point[index].has_distance = 1;

    cur_status.message.outdoor_ride.point[index].distance = distance;
    cur_status.message.outdoor_ride.point[index].gps_status = gps_location_succ_get();
    
    //LOG_DEBUG("point_cnt:%d, cur_Point:%d, distance:%d, step:%d, step_freq:%d, step_today:%d\n\r", \
        point_cnt, cur_status.message.outdoor_ride.point_count, \
        (int)(cur_status.message.outdoor_ride.point[index].distance),\
        cur_status.message.outdoor_ride.point[index].step,\
        cur_status.message.outdoor_ride.point[index].calorie,\
        cur_today_step);

    point_cnt ++;
    point_distance_last = distance; 
    cur_status.message.outdoor_ride.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.outdoor_ride.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_RIDE_MODE);
    }
}

void set_outdoor_walk_point(float distance)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static float point_distance_last;

    if (point_distance_last > distance){
        point_distance_last = distance;
        point_cnt = 0;
    }
    u32_t cur_today_step = alg_get_step_today();
    u32_t index = cur_status.message.outdoor_walk.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.outdoor_walk.point[index].time = ryos_rtc_now();
    cur_status.message.outdoor_walk.point[index].time = ryos_rtc_now_notimezone();

    if(index == 0){
        cur_status.message.outdoor_walk.point[index].step = \
            cur_today_step - cur_status.message.outdoor_walk.point[index].step;
    }else{
        cur_status.message.outdoor_walk.point[index].step = \
            cur_today_step - sports_global.last_step;
    }
    cur_status.message.outdoor_walk.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    float calorie_report = get_step_calorie(cur_status.message.outdoor_walk.point[index].step);
    float calorie_distance = alg_calc_calorie_by_distance((distance - point_distance_last), 1);
    if (calorie_distance > calorie_report){
        data_calorie_offset_update(calorie_distance - calorie_report);
        calorie_report = calorie_distance;        
    }
    cur_status.message.outdoor_walk.point[index].calorie = (int)calorie_distance;

    cur_status.message.outdoor_walk.point[index].has_longitude = 0;

    cur_status.message.outdoor_walk.point[index].has_latitude = 0;

    cur_status.message.outdoor_walk.point[index].has_distance = 1;

    cur_status.message.outdoor_walk.point[index].distance = distance;
    cur_status.message.outdoor_walk.point[index].gps_status = gps_location_succ_get();
    
    //LOG_DEBUG("point_cnt:%d, cur_Point:%d, distance:%d, step:%d, step_freq:%d, step_today:%d\n\r", \
        point_cnt, cur_status.message.outdoor_walk.point_count, \
        (int)(cur_status.message.outdoor_walk.point[index].distance),\
        cur_status.message.outdoor_walk.point[index].step,\
        cur_status.message.outdoor_walk.point[index].calorie,\
        cur_today_step);

    point_cnt ++;
    point_distance_last = distance; 
    cur_status.message.outdoor_walk.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.outdoor_walk.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_OUT_DOOR_WALK);
    }
    
}

void set_indoor_run_point(float distance)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static float point_distance_last;

    if (point_distance_last > distance){
        point_distance_last = distance;
        point_cnt = 0;
    }
    u32_t cur_today_step = alg_get_step_today();
    u32_t index = cur_status.message.indoor_run.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.indoor_run.point[index].time = ryos_rtc_now();
    cur_status.message.indoor_run.point[index].time = ryos_rtc_now_notimezone();    

    if(index == 0){
        cur_status.message.indoor_run.point[index].step = \
            (cur_today_step >= cur_status.message.indoor_run.point[index].step)? \
                (cur_today_step - cur_status.message.indoor_run.point[index].step) : (cur_today_step);
    }else{
        cur_status.message.indoor_run.point[index].step = \
            (cur_today_step >= sports_global.last_step) ? \
            (cur_today_step - sports_global.last_step) : (cur_today_step);
    }
    cur_status.message.indoor_run.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    float calorie_report = get_step_calorie(cur_status.message.indoor_run.point[index].step);
    float calorie_distance = alg_calc_calorie_by_distance((distance - point_distance_last), 1);
    if (calorie_distance > calorie_report){
        data_calorie_offset_update(calorie_distance - calorie_report);
        calorie_report = calorie_distance;        
    }
    cur_status.message.indoor_run.point[index].calorie = (int)calorie_distance;

    cur_status.message.indoor_run.point[index].has_distance = 1;
    cur_status.message.indoor_run.point[index].distance = distance;
    cur_status.message.indoor_run.point[index].step_freq = s_alg.step_freq;
    
    point_cnt ++;
    point_distance_last = distance; 
    cur_status.message.indoor_run.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.indoor_run.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_IN_DOOR_RUN_MODE);
    }
    LOG_DEBUG("[%s]\n", __FUNCTION__);
}

void set_indoor_ride_point(float distance)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static float point_distance_last;

    if (point_distance_last > distance){
        point_distance_last = distance;
        point_cnt = 0;
    }
    u32_t cur_today_step = alg_get_step_today();
    u32_t index = cur_status.message.indoor_ride.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.indoor_ride.point[index].time = ryos_rtc_now();
    cur_status.message.indoor_ride.point[index].time = ryos_rtc_now_notimezone();

    if(index == 0){
        cur_status.message.indoor_ride.point[index].step = \
            cur_today_step - cur_status.message.indoor_ride.point[index].step;
    }else{
        cur_status.message.indoor_ride.point[index].step = \
            cur_today_step - sports_global.last_step;
    }
    cur_status.message.indoor_ride.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    float calorie_report = get_step_calorie(cur_status.message.indoor_ride.point[index].step);
    float calorie_distance = alg_calc_calorie_by_distance((distance - point_distance_last), 1);
    if (calorie_distance > calorie_report){
        data_calorie_offset_update(calorie_distance - calorie_report);
        calorie_report = calorie_distance;        
    }
    cur_status.message.indoor_ride.point[index].calorie = (int)calorie_distance;
    
    point_cnt ++;
    point_distance_last = distance; 
    cur_status.message.indoor_ride.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.indoor_ride.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_IN_DOOR_RIDE_MODE);
    }
    
}


void set_swimming_point(float distance)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static float point_distance_last;

    if (point_distance_last > distance){
        point_distance_last = distance;
        point_cnt = 0;
    }
    u32_t cur_today_step = alg_get_step_today();
    u32_t index = cur_status.message.indoor_swim.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.indoor_swim.point[index].time = ryos_rtc_now();
    cur_status.message.indoor_swim.point[index].time = ryos_rtc_now_notimezone();    

    if(index == 0){
        cur_status.message.indoor_swim.point[index].step = \
            cur_today_step - cur_status.message.indoor_swim.point[index].step;
    }else{
        cur_status.message.indoor_swim.point[index].step = \
            cur_today_step - sports_global.last_step;
    }
    cur_status.message.indoor_swim.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
    float calorie_report = get_step_calorie(cur_status.message.indoor_swim.point[index].step);
    float calorie_distance = alg_calc_calorie_by_distance((distance - point_distance_last), 1);
    if (calorie_distance > calorie_report){
        data_calorie_offset_update(calorie_distance - calorie_report);
        calorie_report = calorie_distance;        
    }
    cur_status.message.indoor_swim.point[index].calorie = (int)calorie_distance;
    cur_status.message.indoor_swim.point[index].distance = distance;
    //cur_status.message.swim.point[index].strokes_cnt = ;
    //cur_status.message.swim.point[index].swolf = ;
    //cur_status.message.swim.point[index].SwimmingPosture = ;
    
    point_cnt ++;
    point_distance_last = distance; 
    cur_status.message.indoor_swim.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.indoor_swim.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_ALG_SWIMMING);
    }
    
}

void set_free_point(u32_t duration_seconds)
{
    //cur_status.message.run.point
	static uint32_t point_cnt;
    static u32_t point_time_last;

    if (point_time_last > duration_seconds){
        point_time_last = duration_seconds;
        point_cnt = 0;
    }
    
    u32_t cur_today_step = alg_get_step_today();
    u32_t index = cur_status.message.free.point_count % SPORT_RECORD_MAX_POINT;
//    cur_status.message.free.point[index].time = ryos_rtc_now();
    cur_status.message.free.point[index].time = ryos_rtc_now_notimezone();
    
    //if (point_time_last == 0)
    //    point_time_last = cur_status.message.free.point[index].time;

    if(index == 0){
        cur_status.message.free.point[index].step = \
            cur_today_step - cur_status.message.free.point[index].step;
    }else{
        cur_status.message.free.point[index].step = \
            cur_today_step - sports_global.last_step;
    }
    cur_status.message.free.point[index].heart_rate = s_alg.hr_cnt;

    //LXL TODO:
//    float calorie_report = get_step_calorie(cur_status.message.free.point[index].step);
//    float calorie_sport = get_step_calorie_sport(cur_status.message.free.point[index].step);
//    if (calorie_sport > calorie_report){
//        data_calorie_offset_update(calorie_sport - calorie_report);
//        calorie_report = calorie_sport;
//    }
    
    float calorie_report = 0;
    float step_seconds = duration_seconds - point_time_last;
    if (step_seconds > 15) {
        calorie_report = get_hrm_calorie_second(cur_status.message.free.point[index].heart_rate, step_seconds);
        u32_t calorie_step = (u32_t)calorie_report;
        
        if (calorie_step == 0) // do not save this point
            return;
        
        point_time_last = duration_seconds;
    }
    
    cur_status.message.free.point[index].calorie = (u32_t)calorie_report;
        
    point_cnt++;
    cur_status.message.free.point_count++;
    sports_global.last_step = cur_today_step;

    if(cur_status.message.free.point_count >= SPORT_RECORD_MAX_POINT){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        //stop_cur_record();
        start_cur_record(SPORT_FREE);
    }
}


void set_sport_point(float distance)
{
    if(cur_status.type == BodyStatusRecord_Type_RUN){
        set_run_point(distance);
    }else if(cur_status.type == BodyStatusRecord_Type_OUTDOOR_RIDE){
        set_ride_point(distance);
    }else if(cur_status.type == BodyStatusRecord_Type_SWIM){
        set_swimming_point(distance);
    }else if(cur_status.type == BodyStatusRecord_Type_INDOOR_RIDE){
        set_indoor_ride_point(distance);
    }else if(cur_status.type == BodyStatusRecord_Type_INDOOR_RUN){
        set_indoor_run_point(distance);
    }else if(cur_status.type == BodyStatusRecord_Type_OUTDOOR_WALK){
        set_outdoor_walk_point(distance);
    }else if(cur_status.type == BodyStatusRecord_Type_FREE){
        set_free_point(distance);
    }else{
        LOG_ERROR("[%s]---type error\n", __FUNCTION__);
    }
}


void start_record_run(void)
{
    SportRunStartParam run_start_param = {0};
    u8_t * buf = NULL;
    run_start_param.session_id = ryos_rtc_now();
    sports_global.cur_session_id = run_start_param.session_id;
    sports_global.last_sn = 0;

    buf = (u8_t *)ry_malloc(sizeof(SportRunStartParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRunStartParam) + 10 );

    if (!pb_encode(&stream, SportRunStartParam_fields, &run_start_param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d\n", __FUNCTION__, run_start_param.session_id);
    rbp_send_req(CMD_APP_SPORT_RUN_START, buf, stream.bytes_written, 1);
    ry_free(buf);
}

void start_record_ride(void)
{
    SportRideStartParam ride_start_param = {0};
    u8_t * buf = NULL;
    ride_start_param.session_id = ryos_rtc_now();
    sports_global.cur_session_id = ride_start_param.session_id;
    sports_global.last_sn = 0;

    buf = (u8_t *)ry_malloc(sizeof(SportRideStartParam) + 10);

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(SportRideStartParam) + 10 );

    if (!pb_encode(&stream, SportRideStartParam_fields, &ride_start_param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("{%s}+++++++++++++++++++++++++++%d\n", __FUNCTION__, ride_start_param.session_id);
    rbp_send_req(CMD_APP_SPORT_RIDE_START, buf, stream.bytes_written, 1);
    ry_free(buf);
}

void start_record_indoor_run(void)
{
    sports_global.cur_session_id = ryos_rtc_now();
    sports_global.last_sn = 0;
} 

void stop_record_step(void)
{
    
}


void start_record_step(void)
{
    /*if(cur_stepRecord.start_time != 0){
        stop_record_step();
    }
    cur_stepRecord.step = alg_get_step_today();
    cur_stepRecord.start_time = ryos_utc_now();*/
    cur_status.type = BodyStatusRecord_Type_STEP;
    cur_status.message.step.common.start_time = ryos_rtc_now_notimezone();
    cur_status.message.step.common.has_deviation = 1;
    cur_status.message.step.common.deviation = get_timezone();
}

void stop_heart_manual(void)
{
    
}


void start_heart_manual(void)
{
    
}

void save_cur_record(void)
{
    FIL * fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;
    u32_t file_size = 0;

    char * today_file_name = NULL;
    CommonRecord * common_ptr = (CommonRecord *)&(cur_status.message);
    static u32_t cnt_data_saving, cnt_data_saved;
    static u32_t time_bak;
    int i,j;
    u32_t magic_num = 0;
    int record_pack_len = 0;
    record_save_type_t * save_data = NULL;
    u32_t correction_data_size = 0;

    time_bak = common_ptr->start_time;

    LOG_DEBUG("[record] cntSaved:%d, msg:%d, cal:%d, dist:%d, step:%d, started:%d\r\n",\
            cnt_data_saving*10000 + cnt_data_saved, cur_status.which_message, \
            common_ptr->calorie, 100 + common_ptr->distance, alg_get_step_today(), common_ptr->start_time);
            
    cnt_data_saving ++;

    u8_t cur_day_num = get_day_number();

    if(data_service_ctx_v.last_file_day != cur_day_num){
        if(is_continuous_body_status(cur_status.type)){
            cur_day_num = data_service_ctx_v.last_file_day;
        }
    }
    

    today_file_name = (char *)ry_malloc(80);
    if(today_file_name == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        return;
    }

    //rintf(today_file_name, "%d_%s",get_day_number(), BODY_STATUS_DATA);

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(today_file_name);
        return;
    }

    //touch_disable();
    i = 0;
    do {
        sprintf(today_file_name, "%d_%s", cur_day_num, BODY_STATUS_DATA);
        ret = f_open(fp, today_file_name, FA_OPEN_APPEND | FA_WRITE | FA_READ);
        
        if(ret == FR_DENIED){ //delete data 
            delete_one_day_data((7-i+6+ cur_day_num)%7);
        }else if(ret != FR_OK){
            LOG_ERROR("[%s]-file open error %d\n", __FUNCTION__, ret);
        }
        i++;
    }while((ret != FR_OK) && (i != 7));
    
    if(ret != FR_OK){
        LOG_ERROR("[%s] file open failed, ret: %d\n", __FUNCTION__, ret);
        goto exit;
        // ry_free((void*)fp);
        //touch_enable();
        // return;
    }

    file_size = f_size(fp);

    if(file_size == 0){
        magic_num = DATA_FILE_MAGIC;
        ret = f_write(fp, &magic_num, sizeof(u32_t), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("[%s] file write failed, ret: %d\n", __FUNCTION__, ret);
            goto exit;
        }
    }
    correction_data_size = cur_status.message.run.common.sn * sizeof(record_save_type_t);
    if(cur_status.type == BodyStatusRecord_Type_RUN){
        if((get_current_running_status()) && (cur_status.message.run.common.sn > 0)){
            if(correction_data_size > file_size ){
                //nothing to do
            }else{
                
                save_data = (record_save_type_t *)ry_malloc(correction_data_size);
                if(save_data != NULL){
                    f_lseek(fp,file_size - correction_data_size);

                    ret = f_read(fp, save_data, correction_data_size, &written_bytes);
                    if(ret != FR_OK){
                        //ry_free(save_data);
                        //save_data = NULL;
                    
                    }else{
                        int temp_step = 0;
                        for(i = 0; i < cur_status.message.run.common.sn; i++){
                            save_data[i].record.which_message = BodyStatusRecord_step_tag;
                            save_data[i].record.type = BodyStatusRecord_Type_STEP;
                            
                            for(j = 0; j < save_data[i].record.message.run.point_count;j++){
                                temp_step += save_data[i].record.message.run.point[j].step;
                            }

                            save_data[i].record.message.step.step = temp_step;
                            
                            //save_data[i].record.message
                            save_data[i].pack_len = get_record_pack_len(&(save_data[i].record));
                        }
                        f_lseek(fp,file_size - correction_data_size);
                        ret = f_write(fp, save_data, correction_data_size, &written_bytes);
                        if(ret != FR_OK){
                            LOG_ERROR("save write error \n");
                        }else{
                            cur_status.which_message = BodyStatusRecord_step_tag;
                            cur_status.type = BodyStatusRecord_Type_STEP;

                            for(j = 0; j < cur_status.message.run.point_count;j++){
                                temp_step += cur_status.message.run.point[j].step;
                            }
                            cur_status.message.step.step = temp_step;
                            cur_status.message.step.common = cur_status.message.run.common;
                            set_current_running_enable();
                        }

                        //ry_free(save_data);
                        //save_data = NULL;
                    }

                    ry_free(save_data);
                    save_data = NULL;
                }else{
                    //f_lseek(fp, file_size);
                }
            }
            
        }
    }

    cur_status.has_today_step = 1;
    cur_status.today_step = alg_get_step_today();

    if(data_service_ctx_v.last_file_day != cur_day_num ){
        if(cur_status.type == BodyStatusRecord_Type_RUN){
            common_ptr->sn = 0;
            common_ptr->total = 0;
            common_ptr->session_id = ryos_utc_now();
            sports_global.cur_session_id = common_ptr->session_id;
            sports_global.last_sn = common_ptr->sn;
        }else if(cur_status.type == BodyStatusRecord_Type_SLEEP_LITE){
            common_ptr->sn = 0;
            common_ptr->total = 0;
            common_ptr->session_id = ryos_utc_now();
            sports_global.sleep_session_id = common_ptr->session_id;
            sports_global.last_sn = common_ptr->sn;
        }

        LOG_DEBUG("[%s]-file change\n",__FUNCTION__);
    }


    file_size = f_size(fp);
    f_lseek(fp, file_size);
    record_pack_len = get_record_pack_len(&cur_status);

    if(record_pack_len == -1){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        goto exit;
    }else{
        ret = f_write(fp, &record_pack_len, sizeof(u32_t), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("[%s] file write failed, ret: %d\n", __FUNCTION__, ret);
            goto exit;
        }
    }

    i = 0;
    do {
        ret = f_write(fp, &cur_status, sizeof(cur_status), &written_bytes);
        if(ret == FR_DENIED){ //full,delete data 
            delete_one_day_data((7 - i + 6 + cur_day_num )%7);
        }
        i++;
    }while((ret != FR_OK) && (i != 7));
    
    if(ret != FR_OK){
        LOG_ERROR("[%s] file write failed, ret: %d\n", __FUNCTION__, ret);
        goto exit;
    }

    if(written_bytes != sizeof(cur_status)){
        f_lseek(fp, file_size);
        ret = f_truncate(fp);
        if(ret != FR_OK){
            f_close(fp);
            f_unlink(today_file_name);
            LOG_ERROR("[%s] body status data is failed and was clear, ret: %d\n", __FUNCTION__, ret);
            // ry_free(today_file_name);
            // ry_free((void*)fp);
            //touch_enable();
            // return;
            goto exit;
        }else{
            LOG_ERROR("[%s] back up data success, ret: %d\n", __FUNCTION__, ret);
        }
    }

    common_ptr->start_time = 0;

    LOG_DEBUG("[%s]file size = %d\n", __FUNCTION__, f_size(fp));
    f_close(fp);
    cnt_data_saved ++;

    LOG_DEBUG("[%s]sn : %d\n", __FUNCTION__, common_ptr->sn);
    LOG_DEBUG("{%s}, sn: %d,type:%d\n",__FUNCTION__,common_ptr->sn,cur_status.type);
exit:
    ry_free(fp);
    ry_free(today_file_name);

    data_service_ctx_v.last_file_day = cur_day_num;

    //touch_enable();
    
    
    return;
}

void test_save_upload_data(void)
{
    int i;
    for(i = 0; i < 222; i++ ){
        cur_status.which_message = BodyStatusRecord_rest_tag;
        cur_status.type = BodyStatusRecord_Type_REST;
        cur_status.message.rest.step = alg_get_step_today() - cur_status.message.rest.step;
        cur_status.message.rest.common.sn = i;
        save_cur_record();
    }
}

int is_sport_continue(u32_t status)
{
    if(is_sport_start(status)){
        if(sports_global.last_sport_mode == status){
            return 1;
        }else{
            return 0;
        }
    }
    return 0;
}

int is_sport_start(u32_t status)
{
    if((status == SPORT_RUN_MODE)||(status == SPORT_IN_DOOR_RUN_MODE)\
        ||(status == SPORT_RIDE_MODE)||(status == SPORT_IN_DOOR_RIDE_MODE)\
        ||(status == SPORT_ALG_SWIMMING)||(status == SPORT_OUT_DOOR_WALK)\
        ||(status == SPORT_FREE)){


        return 1;
    }else{
        return 0;
    }
   
}

int is_continuous_body_status(BodyStatusRecord_Type type)
{
    if((BodyStatusRecord_Type_RUN == type)||\
        (BodyStatusRecord_Type_RIDE == type)||\
        (BodyStatusRecord_Type_SWIM == type)||\
        (BodyStatusRecord_Type_SLEEP_LITE == type)||\
        (BodyStatusRecord_Type_INDOOR_RIDE == type)||\
        (BodyStatusRecord_Type_INDOOR_RUN == type)||\
        (BodyStatusRecord_Type_OUTDOOR_WALK == type)||\
        (BodyStatusRecord_Type_FREE == type)||\
        (BodyStatusRecord_Type_OUTDOOR_RIDE == type)||\
        (BodyStatusRecord_Type_INDOOR_SWIM == type)\
        ){
        return 1;
    }else{
        return 0;
    }
}


void start_cur_record(u32_t status)
{
    
    //memset(&cur_status, 0, sizeof(cur_status));
    CommonRecord * common_ptr = (CommonRecord *)&(cur_status.message);
    
    sports_global.last_status = cur_status.type;
    //sports_global.last_sn = common_ptr->sn;

    if(common_ptr->start_time != 0){
        LOG_DEBUG("[%s] stop_cur_record\n", __FUNCTION__);
        stop_cur_record();
        memset(&cur_status, 0, sizeof(cur_status));
    }
    memset(&cur_status, 0, sizeof(cur_status));
    

#if 0
    if(sports_global.last_status == BodyStatusRecord_Type_RUN){
        if(status == SPORT_RUN_MODE){
            sports_global.last_sn += 1;
            common_ptr->sn = sports_global.last_sn;
            common_ptr->total = 0;
            common_ptr->session_id = sports_global.cur_session_id;
        }else{
            /*sports_global.last_sn += 1;
            common_ptr->sn = sports_global.last_sn;
            common_ptr->total = common_ptr->sn;*/
            
        }
        
    }else{
        if(status == SPORT_RUN_MODE){
            common_ptr->sn = 0;
            sports_global.last_sn = 0;
        }else{

        }
    }
#else
    if(is_sport_continue(status)){
        LOG_DEBUG("sport continue --- %d\n", status);
        sports_global.last_sn += 1;
        common_ptr->sn = sports_global.last_sn;
        common_ptr->total = 0;
        common_ptr->session_id = sports_global.cur_session_id;
    }else if(is_sport_start(status)){
        LOG_DEBUG("sport start --- %d\n", status);
        common_ptr->sn = 0;
        sports_global.last_sn = 0;
    }

#endif

    
    if(sports_global.last_status == BodyStatusRecord_Type_SLEEP_LITE){
        if((status == SPORTS_SLEEP_LIGHT) || (status == SPORTS_SLEEP_DEEP)){
            sports_global.last_sn += 1;
            sports_global.is_sleep = 1;
            common_ptr->sn = sports_global.last_sn;
            common_ptr->total = 0;
            common_ptr->session_id = sports_global.sleep_session_id;
        }else{
            /*sports_global.last_sn += 1;
            common_ptr->sn = sports_global.last_sn;
            common_ptr->total = common_ptr->sn;*/
            
            sports_global.is_sleep = 0;
        }
        
    }else{
        if((status == SPORTS_SLEEP_LIGHT) || (status == SPORTS_SLEEP_DEEP)){
            common_ptr->sn = 0;
            sports_global.last_sn = 0;
            sports_global.is_sleep = 1;
        }else{
            sports_global.is_sleep = 0;
        }
    }
    
    u32_t cur_today_step = alg_get_step_today();

    if(status == SPORTS_STILL){
        cur_status.type = BodyStatusRecord_Type_REST;
        cur_status.message.rest.step = cur_today_step;
    }else if(status == SPORTS_LIGHT_ACTIVE){
    
        cur_status.type = BodyStatusRecord_Type_LITE_ACTIVITY;
        cur_status.message.lite_activity.step = cur_today_step;
        
    }else if(status == SPORTS_WALK){

        cur_status.type = BodyStatusRecord_Type_STEP;
        cur_status.message.step.step = cur_today_step;
        
    }else if(status == SPORTS_RUN){

        cur_status.type = BodyStatusRecord_Type_BRISK_WALK;
        cur_status.message.brisk_walk.step = cur_today_step;
        
    }else if(status == SPORT_RUN_MODE){
    
        cur_status.type = BodyStatusRecord_Type_RUN;
        cur_status.message.run.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;
    }else if(status == SPORTS_SLEEP_DEEP){
        if((sports_global.last_status != BodyStatusRecord_Type_SLEEP_LITE)){
            common_ptr->session_id = ryos_utc_now();
            sports_global.sleep_session_id = common_ptr->session_id;
        }else{
            common_ptr->session_id = sports_global.sleep_session_id;
        }
        cur_status.type = BodyStatusRecord_Type_SLEEP_LITE;
        cur_status.message.sleep_lite.type = SLEEP_STATUS_DEEP;
        cur_status.message.sleep_lite.version = SLEEP_LITE_VERSION;
        cur_status.message.sleep_lite.step = cur_today_step;
        
    }else if(status == SPORTS_SLEEP_LIGHT){
        if((sports_global.last_status != BodyStatusRecord_Type_SLEEP_LITE)){
            common_ptr->session_id = ryos_utc_now();
            sports_global.sleep_session_id = common_ptr->session_id;
        }else{
            common_ptr->session_id = sports_global.sleep_session_id;
        }
        cur_status.type = BodyStatusRecord_Type_SLEEP_LITE;
        cur_status.message.sleep_lite.type = SLEEP_STATUS_LIGHT;
        cur_status.message.sleep_lite.version = SLEEP_LITE_VERSION;
        cur_status.message.sleep_lite.step = cur_today_step;
    }else if(status == SPORT_IN_DOOR_RUN_MODE){
        cur_status.type = BodyStatusRecord_Type_INDOOR_RUN;
        cur_status.message.indoor_run.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;

    }else if(status == SPORT_RIDE_MODE){

        cur_status.type = BodyStatusRecord_Type_OUTDOOR_RIDE;
        cur_status.message.outdoor_ride.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;

    }else if(status == SPORT_IN_DOOR_RIDE_MODE){
        cur_status.type = BodyStatusRecord_Type_INDOOR_RIDE;
        cur_status.message.indoor_ride.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;
    }else if(status == SPORT_ALG_SWIMMING){
        cur_status.type = BodyStatusRecord_Type_SWIM;
        cur_status.message.indoor_swim.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;
    }else if(status == SPORT_OUT_DOOR_WALK){
        cur_status.type = BodyStatusRecord_Type_OUTDOOR_WALK;
        cur_status.message.outdoor_walk.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;
    }else if(status == SPORT_FREE){
        cur_status.type = BodyStatusRecord_Type_FREE;
        cur_status.message.free.point[0].step = cur_today_step;
        common_ptr->session_id = sports_global.cur_session_id;
    }else{
        return;
    }

    LOG_INFO("[start] type: %d -step:%d\n",cur_status.type, cur_today_step);
    /*if(sports_global.last_status == BodyStatusRecord_Type_RUN){
        common_ptr->sn = sports_global.last_sn + 1;
        common_ptr->session_id = sports_global.cur_session_id;
    }else{
        
    }*/

    //cur_status.message.step.common.start_time = ryos_utc_now();
    common_ptr->start_time = ryos_rtc_now_notimezone();
    common_ptr->has_deviation = 1;
    common_ptr->deviation = get_timezone();

    sports_global.last_sport_mode = status;
}

void set_cur_record_heart_rate(int type)
{
    CommonRecord * common_ptr = (CommonRecord *)&(cur_status.message);

    /*if(cur_status.type >= BodyStatusRecord_Type_RUN){
        common_ptr->heart_rates[common_ptr->heart_rates_count].measure_type = HeartRateRecord_MeasureType_AUTO;
    }else{
        common_ptr->heart_rates[common_ptr->heart_rates_count].measure_type = HeartRateRecord_MeasureType_MANUAL;
    }*/
    common_ptr->heart_rates[common_ptr->heart_rates_count].measure_type = (HeartRateRecord_MeasureType)type;

    if(type == TYPE_RESTING_HEART){
        common_ptr->heart_rates[common_ptr->heart_rates_count].measure_type = (HeartRateRecord_MeasureType)0;
        common_ptr->heart_rates[common_ptr->heart_rates_count].is_resting = 1;
        alg_get_hrm_awake_cnt((uint8_t *)&(common_ptr->heart_rates[common_ptr->heart_rates_count].rate));
    }else{
        common_ptr->heart_rates[common_ptr->heart_rates_count].is_resting = 0;
        common_ptr->heart_rates[common_ptr->heart_rates_count].rate = alg_get_hrm_cnt_latest();
    }
    
    common_ptr->heart_rates[common_ptr->heart_rates_count].time =
            ryos_rtc_now_notimezone() + get_timezone_china() - get_timezone();
    
    common_ptr->has_deviation = 1;
    common_ptr->deviation = get_timezone();
    
//    LOG_INFO("set record heartrate, time_notz: %ld, time: %d, rate: %d, %d", 
//        common_ptr->heart_rates[common_ptr->heart_rates_count].time, ryos_rtc_now(),
//        common_ptr->heart_rates[common_ptr->heart_rates_count].rate, 0);

    sports_global.last_hrm_time = ryos_rtc_now_notimezone();

    common_ptr->heart_rates_count++;
    
    if(common_ptr->heart_rates_count == sizeof(common_ptr->heart_rates)/sizeof(HeartRateRecord)){
        // TODO
        LOG_INFO("[%s] stop_cur_record\n", __FUNCTION__);
        stop_cur_record();
        start_cur_record(s_alg.sports_mode);
    }    
}

int test_step_sum = 0;

void stop_cur_record(void)
{

    CommonRecord * common_ptr = (CommonRecord *)&(cur_status.message);

    common_ptr->end_time = ryos_rtc_now_notimezone();
    common_ptr->has_calorie = 1;
    common_ptr->calorie = 0;
    common_ptr->has_distance = 1;
    common_ptr->distance = 0;
    common_ptr->has_long_sit = 1;
    common_ptr->long_sit = alg_long_sit_detect();
    common_ptr->has_deviation = 1;
    common_ptr->deviation = get_timezone();

    int cur_step = 0;
    int i = 0;
    u32_t cur_today_step = alg_get_step_today();

    switch(cur_status.type){
        /*case BodyStatusRecord_Type_SLEEP:
            {
                //cur_status.message.sleep.
                cur_status.which_message = 
            }
            break;*/
        case BodyStatusRecord_Type_REST:
            {
                //cur_status.message.rest
                cur_status.which_message = BodyStatusRecord_rest_tag;
                cur_status.message.rest.step = cur_today_step - cur_status.message.rest.step;
                cur_step = cur_status.message.rest.step;
                if(cur_status.message.rest.step < 0){
                    cur_status.message.rest.step = 0;
                }
                common_ptr->calorie = get_step_calorie(cur_status.message.rest.step);
                common_ptr->distance = app_get_step_distance(cur_status.message.rest.step);
            }
            break;
        case BodyStatusRecord_Type_LITE_ACTIVITY:
            {
                cur_status.message.lite_activity.step = cur_today_step - cur_status.message.lite_activity.step;
                cur_step = cur_status.message.lite_activity.step;
                if(cur_status.message.lite_activity.step < 0){
                    cur_status.message.lite_activity.step = 0;
                }
                cur_status.which_message = BodyStatusRecord_lite_activity_tag;
                common_ptr->calorie = get_step_calorie(cur_status.message.lite_activity.step);
                common_ptr->distance = app_get_step_distance(cur_status.message.lite_activity.step);
            }
            break;
        case BodyStatusRecord_Type_STEP:
            {
                cur_status.message.step.step = cur_today_step - cur_status.message.step.step;
                cur_step = cur_status.message.step.step;
                if(cur_status.message.step.step < 0){
                    cur_status.message.step.step = 0;
                }
                cur_status.message.step.freq = alg_get_step_freq_latest();
                cur_status.which_message = BodyStatusRecord_step_tag;
                common_ptr->calorie = get_step_calorie(cur_status.message.step.step);
                common_ptr->distance = app_get_step_distance(cur_status.message.step.step);
            }
            break;
        case BodyStatusRecord_Type_BRISK_WALK:
            {
                cur_status.message.brisk_walk.step = cur_today_step - cur_status.message.brisk_walk.step;
                cur_step = cur_status.message.brisk_walk.step;
                if(cur_status.message.brisk_walk.step < 0){
                    cur_status.message.brisk_walk.step = 0;
                }
                cur_status.which_message = BodyStatusRecord_brisk_walk_tag;
                common_ptr->calorie = get_step_calorie(cur_status.message.brisk_walk.step);
                common_ptr->distance = app_get_step_distance(cur_status.message.brisk_walk.step);
            }
            break;
        case BodyStatusRecord_Type_STAIR:
            {
                cur_status.message.stair.step = cur_today_step - cur_status.message.stair.step;
                cur_step = cur_status.message.stair.step;
                if(cur_status.message.stair.step < 0){
                    cur_status.message.stair.step = 0;
                }
                cur_status.which_message = BodyStatusRecord_stair_tag;
                common_ptr->calorie = get_step_calorie(cur_status.message.stair.step);
                common_ptr->distance = app_get_step_distance(cur_status.message.stair.step);
            }
            break;
        case BodyStatusRecord_Type_RUN:
            {
                //cur_status.message.run
                cur_status.which_message = BodyStatusRecord_run_tag;
                LOG_DEBUG("\nRUN\n\n");
                LOG_DEBUG("\n\nsn : %d   total : %d \n\n\n",common_ptr->sn, common_ptr->total);

                for(i = 0; i < cur_status.message.run.point_count; i++){
                    cur_step += cur_status.message.run.point[i].step;
                }
            }
            break;
        case BodyStatusRecord_Type_OUTDOOR_RIDE:
            {
                //cur_status.message.ride
                cur_status.which_message = BodyStatusRecord_outdoor_ride_tag;
                for(i = 0; i < cur_status.message.outdoor_ride.point_count; i++){
                    cur_step += cur_status.message.outdoor_ride.point[i].step;
                }
            }
            break;
        case BodyStatusRecord_Type_SWIM:
            {
                //cur_status.message.swim
                cur_status.which_message = BodyStatusRecord_swim_tag;
            }
            break;

        case BodyStatusRecord_Type_SLEEP_LITE:
            {
                if (get_device_session() != DEV_SESSION_UPLOAD_DATA) {
                    cur_status.which_message = BodyStatusRecord_sleep_lite_tag;
                    cur_status.message.sleep_lite.step = (cur_today_step >= cur_status.message.sleep_lite.step) ? \
                        (cur_today_step - cur_status.message.sleep_lite.step):(0);

                    cur_step = cur_status.message.sleep_lite.step;
                    common_ptr->calorie = get_step_calorie(cur_status.message.sleep_lite.step);
                    common_ptr->distance = app_get_step_distance(cur_status.message.sleep_lite.step);
                    
                } else {
                    cur_status.which_message = BodyStatusRecord_rest_tag;
                    cur_status.type = BodyStatusRecord_Type_REST;
                    common_ptr->calorie = get_step_calorie(cur_status.message.rest.step);
                    common_ptr->distance = app_get_step_distance(cur_status.message.rest.step);
                    cur_status.message.rest.step = (cur_today_step >= cur_status.message.sleep_lite.step) ? \
                        (cur_today_step - cur_status.message.sleep_lite.step):(0);

                    cur_step = cur_status.message.rest.step;
                    ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_DATA_CHECK_WAKEUP, 0, NULL);
                }
            }
            break;

        case BodyStatusRecord_Type_INDOOR_RUN:
            {
                cur_status.which_message = BodyStatusRecord_indoor_run_tag;

                for(i = 0; i < cur_status.message.indoor_run.point_count; i++){
                    cur_step += cur_status.message.indoor_run.point[i].step;
                }
                                
                LOG_DEBUG("\nINDOOR RUN\n\n");
                LOG_DEBUG("\n\nsn : %d   total : %d \n\n\n",common_ptr->sn, common_ptr->total);
            }
            break;
            
        case BodyStatusRecord_Type_FREE:
            {
                cur_status.which_message = BodyStatusRecord_free_tag;

                for(i = 0; i < cur_status.message.free.point_count; i++){
                    cur_step += cur_status.message.free.point[i].step;
                }
            }
            break;
    }

    // TODO:
    // TODO:
    // TODO:
    test_step_sum += cur_step;
    LOG_DEBUG("[stop] type: %d -step: %d--%d---sum: %d\n",cur_status.type,alg_get_step_today(), cur_step, test_step_sum);
    if (get_device_session() != DEV_SESSION_UPLOAD_DATA) {
        save_cur_record();
    }   
}


int data_service_thread_entry(void* arg)
{
    ry_data_msg_t *msg= NULL;
    ry_sts_t status;
    static u32_t is_have_task = 0;
    u32_t wait_time = 0;

    u32_t loop_count = 0;
    u8_t awake_cnt = 0;

    check_save_data();

    while(1){

        if(is_have_task){
            
            wait_time = RY_OS_WAIT_FOREVER;
        }
    
        status = ryos_queue_recv(ry_data_service_msgQ, &msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
           
        } else {            

            switch(msg->msgType){
                case DATA_SERVICE_MSG_START_STEP:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_STEP_FLAG);
                    LOG_DEBUG("start step\n");
                    start_record_step();
                    break;
                case DATA_SERVICE_MSG_STOP_STEP:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_STEP_FLAG);
                    LOG_DEBUG("stop step\n");
                    stop_record_step();
                    break;

                case DATA_SERVICE_MSG_START_HEART:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_HEART_FLAG);
                    start_heart_manual();
                    break;
                    
                case DATA_SERVICE_MSG_START_HEART_RUN:
                case DATA_SERVICE_MSG_START_HEART_SLEEP:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_HEART_FLAG);
                    
                    break;
                    
                case DATA_SERVICE_MSG_START_SLEEP_DEEP:
                case DATA_SERVICE_MSG_START_SLEEP_WAKE:
                case DATA_SERVICE_MSG_START_SLEEP_LIGHT:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_SLEEP_FLAG);
                    //start_record_sleep();
                    break;
                    
                case DATA_SERVICE_MSG_START_RUN:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_RUN_FLAG);
                    start_record_run();
                    break;
                case DATA_SERVICE_MSG_START_JUMP:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_JUMP_FLAG);
                    break;
                case DATA_SERVICE_MSG_START_SWIMMING:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_SWIMMING_FLAG);
                    break;
                case DATA_SERVICE_MSG_START_BIKE:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_BIKE_FLAG);
                    break;

                    

                case DATA_SERVICE_MSG_STOP_HEART:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_HEART_FLAG);
                    LOG_DEBUG("DATA_SERVICE_MSG_STOP_HEART\n");
                    set_cur_record_heart_rate(HeartRateRecord_MeasureType_MANUAL);
                    break;

                case DATA_SERVICE_MSG_STOP_HEART_RESTING:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_HEART_FLAG);
                    awake_cnt = 0;
                    alg_get_hrm_awake_cnt(&awake_cnt);
                    LOG_DEBUG("DATA_SERVICE_MSG_STOP_HEART_RESTING, awake_cnt:%d\n", awake_cnt);
                    set_cur_record_heart_rate(TYPE_RESTING_HEART);
                    break;

                case DATA_SERVICE_MSG_STOP_HEART_AUTO:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_HEART_FLAG);
                    LOG_DEBUG("DATA_SERVICE_MSG_STOP_HEART\n");
                    set_cur_record_heart_rate(HeartRateRecord_MeasureType_AUTO);
                    break;
                    
                case DATA_SERVICE_MSG_STOP_HEART_RUN:
                case DATA_SERVICE_MSG_STOP_HEART_SLEEP:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_HEART_FLAG);
                    break;
                    
                case DATA_SERVICE_MSG_STOP_SLEEP_DEEP:
                case DATA_SERVICE_MSG_STOP_SLEEP_WAKE:
                case DATA_SERVICE_MSG_STOP_SLEEP_LIGHT:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_SLEEP_FLAG);
                    //stop_record_sleep(msg->msgType - DATA_SERVICE_MSG_START_SLEEP_DEEP);
                    break;
                    
                case DATA_SERVICE_MSG_STOP_RUN:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_RUN_FLAG);
                    stop_record_run();
                    break;
                case DATA_SERVICE_MSG_STOP_JUMP:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_JUMP_FLAG);
                    break;
                case DATA_SERVICE_MSG_STOP_SWIMMING:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_SWIMMING_FLAG);
                    break;
                case DATA_SERVICE_MSG_STOP_BIKE:
                    SPORT_FLAG_CLEAR(is_have_task, DATA_SERVICE_BIKE_FLAG);
                    break;

                case DATA_SERVICE_MSG_GET_LOCATION:
                    //get_location();
                    update_run_param();
                    break;

                case DATA_SERVICE_MSG_GET_REAL_WEATHER:
                    get_weather_info();
                    break;

                case DATA_SERVICE_MSG_SLEEP_SESSION_BEGINNING:
                    // SPORT_FLAG_SET(is_have_task, DATA_SERVICE_SLEEP_FLAG);
                    LOG_DEBUG("SLEEP session beginning\n");
                    //set session id
                    sports_global.sleep_session_id = ryos_rtc_now();
                    sports_global.sleep_sn = 0;
                    mibeacon_sleep(0x01);
                    if(get_sleep_status()){

                    }else{
                        set_sleep_status();
                    }
                    break;

                case DATA_SERVICE_MSG_SLEEP_SESSION_FINISHED:
                    // SPORT_FLAG_SET(is_have_task, DATA_SERVICE_SLEEP_FLAG);
                    LOG_DEBUG("SLEEP session finished\n");
                    // start_record_sleep();
                    mibeacon_sleep(0x00);
                    //sports_global.end_sleep_tick = ryos_rtc_now();
                    sports_global.end_sleep_tick = ryos_rtc_now_notimezone();
                    break;

                case DATA_SERVICE_MSG_START_SLEEP:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_SLEEP_FLAG);
                    LOG_DEBUG("START SLEEP\n");
                    start_record_sleep();
                    break;

                case DATA_SERVICE_MSG_STOP_SLEEP:
                    LOG_DEBUG("STOP SLEEP\n");
                    stop_record_sleep();
                    break;

                case DATA_SERVICE_MSG_SET_SLEEP_DATA:
                    SPORT_FLAG_SET(is_have_task, DATA_SERVICE_SLEEP_FLAG);
                    // LOG_DEBUG("SET SLEEP DATA\n");
                    set_record_sleep_data(*((u32_t *)msg->data));
                    break;

                case DATA_SERVICE_UPLOAD_DATA_START:
                    //ryos_delay_ms(5000);
                    data_upload(0);
                    break;

                case DATA_SERVICE_BODY_STATUS_CHANGE:
                    start_cur_record(*((u32_t *)msg->data));
                    break;

                case DATA_SERVICE_DEV_LOG_UPLOAD:
                    //ryos_delay_ms(100);
                    error_log_upload();
                    break;

                case DATA_SERVICE_DEV_RAW_DATA_UPLOAD:
                    raw_data_upload();
                    break;


            }
        }

        if(loop_count){
            /*if(loop_count%20 == 0){
                ry_data_msg_send(DATA_SERVICE_MSG_START_STEP, 0, NULL);
            }

            if((loop_count%(60*3) == 0)&&(ry_ble_get_state() == RY_BLE_STATE_CONNECTED)){
                data_upload_loop();
            }*/
        }
        

        if(msg != NULL){
            if(msg->len > 0){
                ry_free(msg->data);
            }
            ry_free(msg);
            msg = NULL;

        }
        
        //ryos_delay_ms(200);
        loop_count++;
        
    }

}



void data_service_task_init(void)
{   
    ry_sts_t status;
    status = ryos_queue_create(&ry_data_service_msgQ, DATA_SERVICE_QUEUE_SIZE, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[data_service_thread_entry]: Queue Create Error.\r\n");
        while(1);
    }
    ryos_threadPara_t para;
    strcpy((char*)para.name, "data_thread");
    para.stackDepth = 600;//256*2;
    para.arg        = NULL; 
    para.tick       = 2;
    para.priority   = 3;
    ryos_thread_create(&data_service_handle, data_service_thread_entry, &para);
}



WeatherCityList  city_list = {0};
WeatherGetRealtimeInfoResult * weather_info = NULL;


void repo_weather_city(void)
{
    size_t message_length;
    ry_sts_t status;
    pb_ostream_t stream;
    u8_t *data_buf = (u8_t *)ry_malloc(sizeof(WeatherCityList));
    if(data_buf == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    
    stream = pb_ostream_from_buffer(data_buf, (sizeof(WeatherCityList)));

    if (!pb_encode(&stream, WeatherCityList_fields, &city_list)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }
    message_length = stream.bytes_written;


exit:
    status = rbp_send_resp(CMD_DEV_WEATHER_GET_CITY_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1);
    ry_free(data_buf);

}


void set_weather_city(uint8_t* data, int len)
{
    size_t message_length;
    ry_sts_t status;
    
    pb_istream_t stream = pb_istream_from_buffer(data, len);

    
    status = pb_decode(&stream, WeatherCityList_fields, &city_list);
    if(!status){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
    }
    LOG_DEBUG("[%s] - city count = %d\n", __FUNCTION__, city_list.city_items_count);

    if(city_list.city_items_count == 0){
        ry_free(weather_info);
        weather_info = NULL;
    }

    system_app_data_save();

    status = rbp_send_resp(CMD_DEV_WEATHER_SET_CITY_LIST, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_data_msg_send(DATA_SERVICE_MSG_GET_REAL_WEATHER, 0, NULL);
}

void get_real_weather(void)
{
    u8_t i ;
    if((city_list.city_items_count == 0)&&(ry_ble_get_state() == RY_BLE_STATE_CONNECTED)){
        return ;
    }
    u8_t *data_buf = NULL;
    pb_ostream_t stream;
    WeatherGetRealtimeInfoParam * get_city_weather = (WeatherGetRealtimeInfoParam *)ry_malloc(sizeof(WeatherGetRealtimeInfoParam));
    if(get_city_weather == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    get_city_weather->city_ids_count = city_list.city_items_count;

    for(i = 0; i < city_list.city_items_count; i++){
        strcpy(&( (get_city_weather->city_ids[i][0]) ), &(city_list.city_items[i].city_id[0]));
    }
    
    data_buf = (u8_t *)ry_malloc(sizeof(WeatherGetRealtimeInfoParam));
    if(data_buf == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    stream = pb_ostream_from_buffer(data_buf, (sizeof(WeatherGetRealtimeInfoParam)));

    if (!pb_encode(&stream, WeatherGetRealtimeInfoParam_fields, get_city_weather)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    rbp_send_req(CMD_APP_WEATHER_GET_REALTIME_INFO, data_buf, stream.bytes_written, 1);

exit:
    ry_free(data_buf);
    ry_free(get_city_weather);
    
}

void get_real_weather_result(uint8_t* data, int len)
{
    pb_istream_t stream;
    ry_sts_t status;
    u8_t i,j;
    if(weather_info == NULL){
        weather_info = (WeatherGetRealtimeInfoResult *)ry_malloc(sizeof(WeatherGetRealtimeInfoResult ));
        if(weather_info == NULL){
            LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
            goto exit;
        }
    }
    
    stream = pb_istream_from_buffer(data, len);
    
    status = pb_decode(&stream, WeatherGetRealtimeInfoResult_fields, weather_info);
    if(!status){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
        goto exit;
    }

    /*for(i = 0; i < city_list.city_items_count; i++){
        for( j = 0; j < weather_info->infos_count; j++){
            if(strcmp(&(city_list.city_items[i].city_id[0]), &(weather_info->infos[j].city_id[0])) == 0){
                //city_list.cit
            }
        }
    }*/
    
    
exit:
    //ry_free(result);
    return;
}

void get_forecast_weather(void)
{
    u8_t i;
    if((city_list.city_items_count == 0)&&(ry_ble_get_state() == RY_BLE_STATE_CONNECTED)){
        return ;
    }
    u8_t *data_buf = NULL;
    pb_ostream_t stream;
    WeatherGetForecastInfoParam * forecast_list = (WeatherGetForecastInfoParam *)ry_malloc(sizeof(WeatherGetForecastInfoParam));
    if(forecast_list == NULL){
        LOG_DEBUG("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto exit;
    }
    
    forecast_list->city_ids_count = city_list.city_items_count;

    for(i = 0; i < city_list.city_items_count; i++){
        strcpy(&( (forecast_list->city_ids[i][0]) ), &(city_list.city_items[i].city_id[0]));
    }

    

    data_buf = (u8_t *)ry_malloc(sizeof(WeatherGetForecastInfoParam));
    if(data_buf == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    stream = pb_ostream_from_buffer(data_buf, (sizeof(WeatherGetForecastInfoParam)));

    if (!pb_encode(&stream, WeatherGetForecastInfoParam_fields, forecast_list)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    rbp_send_req(CMD_APP_WEATHER_GET_FORECAST_INFO, data_buf, stream.bytes_written, 1);
    
exit:

    ry_free(data_buf);
    ry_free(forecast_list);
    
}


void get_forecast_weather_result(uint8_t* data, int len)
{
    WeatherGetForecastInfoResult * weather_forecast_info = (WeatherGetForecastInfoResult*)ry_malloc(sizeof(WeatherGetForecastInfoResult));
    pb_istream_t stream;
    ry_sts_t status;
    FIL *fp = NULL;
    FRESULT res = FR_OK;
    u32_t written_bytes = 0;
    if(weather_forecast_info == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    
    stream = pb_istream_from_buffer(data, len);
        
    status = pb_decode(&stream, WeatherGetForecastInfoResult_fields, weather_forecast_info);
    if(!status){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
    }
    
    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    res = f_open(fp, WEATHER_FORECAST_DATA, FA_OPEN_ALWAYS | FA_WRITE);

    if(res != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        goto exit;
    }

    res = f_write(fp, weather_forecast_info, sizeof(WeatherGetForecastInfoResult), &written_bytes);

    f_close(fp);


exit:
    ry_free(weather_forecast_info);
    ry_free(fp);
}



void weather_get_result(ry_sts_t status, void* usr_data)
{
    if (status != RY_SUCC) {
        ry_data_msg_send(DATA_SERVICE_MSG_GET_REAL_WEATHER, 0, NULL);
    } else {


    }
}


void get_weather_info(void)
{
    u8_t i;
    if((city_list.city_items_count == 0)&&(ry_ble_get_state() == RY_BLE_STATE_CONNECTED)){
        return ;
    }
    u8_t *data_buf = NULL;
    pb_ostream_t stream;
    WeatherGetInfoParam * info_param = (WeatherGetInfoParam *)ry_malloc(sizeof(WeatherGetInfoParam));

    if(info_param == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    info_param->city_ids_count = city_list.city_items_count;

    for(i = 0; i < city_list.city_items_count; i++){
        strcpy(&( (info_param->city_ids[i][0]) ), &(city_list.city_items[i].city_id[0]));
    }

    data_buf = (u8_t *)ry_malloc(sizeof(WeatherGetInfoParam));
    if(data_buf == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    stream = pb_ostream_from_buffer(data_buf, (sizeof(WeatherGetInfoParam)));

    if (!pb_encode(&stream, WeatherGetInfoParam_fields, info_param)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }


    ryos_delay_ms(200);
    ble_send_request(CMD_APP_WEATHER_GET_INFO, data_buf, stream.bytes_written, 1, weather_get_result, NULL);


exit:

    ry_free(data_buf);
    ry_free(info_param);
}

void get_weather_info_result(uint8_t* data, int len)
{
    WeatherGetInfoResult * weather_info_result = (WeatherGetInfoResult*)ry_malloc(sizeof(WeatherGetInfoResult));
    memset(weather_info_result, 0, sizeof(WeatherGetInfoResult));
    pb_istream_t stream;
    ry_sts_t status;
    FIL *fp = NULL;
    FRESULT res = FR_OK;
    u32_t written_bytes = 0;
    u8_t i , j;
    if(weather_info_result == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    
    stream = pb_istream_from_buffer(data, len);
        
    status = pb_decode(&stream, WeatherGetInfoResult_fields, weather_info_result);
    if(!status){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_DECODE_FAIL);
    }

    /*for(i = 0; i < city_list.city_items_count; i++){
        for( j = 0; j < weather_info_result->infos_count; j++){
            if(strcmp(&(city_list.city_items[i].city_id[0]), &(weather_info_result->infos[j].city_id[0])) == 0){
                //city_list.cit
            }
        }
    }*/

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }

    res = f_open(fp, WEATHER_FORECAST_DATA, FA_OPEN_ALWAYS | FA_WRITE);

    if(res != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        goto exit;
    }

    res = f_write(fp, weather_info_result, sizeof(WeatherGetInfoResult), &written_bytes);

    f_close(fp);

    if(strcmp(wms_activity_get_cur_name(), "weather") == 0){
        u8_t * usr_data = (u8_t *)ry_malloc(1);
        extern activity_t activity_weather;
        wms_activity_replace(&activity_weather, usr_data);
    }

    DEV_STATISTICS(weather_get_info);

exit:

    ry_free(weather_info_result);
    ry_free(fp);
    
}

void system_app_data_save(void)
{
    FIL* fp = NULL;
    FRESULT status = FR_OK;
    u32_t written_bytes = 0;


    if(is_bond() == FALSE)
        return ;
		
    if (get_device_powerstate() == DEV_PWR_STATE_OFF)
            return;
		
    LOG_DEBUG("[%s] start\n", __FUNCTION__);
    
    sys_app_data_new_t * app_data = (sys_app_data_new_t *)ry_malloc(sizeof(sys_app_data_new_t));
    if(app_data == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    app_data->magic = SYS_APP_DATA_NEW_MAGIC;

    ry_memcpy(&(app_data->city_list), &city_list, sizeof(WeatherCityList));
    if(weather_info != NULL){
        ry_memcpy(&(app_data->weather_info), weather_info, sizeof(WeatherGetRealtimeInfoResult));
    }
    //ry_memcpy(&(app_data->clock_list), &clock_list, sizeof(AlarmClockList));
    ry_memcpy(&(app_data->s_alg), &s_alg, sizeof(sensor_alg_t));
    ry_memcpy(&(app_data->sleep_time), &sleep_time, sizeof(alg_sleep_time_t));
//    ry_memcpy(&(app_data->curSurface), surface_get_current(), sizeof(surface_desc_t));
    
    //_memcpy(&(app_data->dev_statistics), &dev_statistics, sizeof(DevStatisticsResult));
    app_data->last_hrm_time = get_last_hrm_time();
    
    app_data->time = ryos_rtc_now();
    app_data->batt_last = batt_percent_get();
    app_data->curSurfaceId = 0xFFFFFFFF;//surface_get_current_id();
    app_data->data_sports_offset_distance_today = data_sports_distance_app_offset_get();
    app_data->data_calorie_offset_today = data_sports_calorie_offset_get();

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    f_unlink(SYSTEM_APP_DATA_FILE_NAME);
    status = f_open(fp, SYSTEM_APP_DATA_FILE_NAME, FA_OPEN_ALWAYS | FA_WRITE);
    if(status != FR_OK){
        LOG_ERROR("[%s] file open fail ---%d\n",__FUNCTION__, status);
        goto exit;
    }

    status = f_write(fp, app_data, sizeof(sys_app_data_new_t), &written_bytes);
    if(status != FR_OK){
        LOG_ERROR("[%s] file written fail ---%d\n",__FUNCTION__, status);
        goto exit;
    }

    if(written_bytes != sizeof(sys_app_data_new_t)){
        LOG_ERROR("[%s] file written bytes failed ---%d\n",__FUNCTION__, written_bytes);
    }

    status = f_write(fp, &dev_statistics, sizeof(dev_statistics), &written_bytes);
    if(status != FR_OK){
        LOG_ERROR("[%s] file written fail ---%d\n",__FUNCTION__, status);
        goto exit;
    }

    LOG_DEBUG("%s succ, heap:%d, minHeap:%d\n", __FUNCTION__, \
		ryos_get_free_heap(), ryos_get_min_available_heap_size());

exit:
    f_close(fp);
    ry_free(fp);
    ry_free(app_data);
    
}


void system_app_data_init(void)
{
    FIL* fp = NULL;
    FRESULT status = FR_OK;
    u32_t written_bytes = 0;
    u32_t magic_num = 0;
    u32_t file_size = 0;
    
    LOG_INFO("%s\n", __FUNCTION__);
    
    sys_app_data_t * app_data = (sys_app_data_t *)ry_malloc(sizeof(sys_app_data_new_t));
    sys_app_data_new_t * app_data_new = NULL;
    
    if(app_data == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    LOG_DEBUG("{%d}\n", cur_status);


    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }

    status = f_open(fp, SYSTEM_APP_DATA_FILE_NAME, FA_READ);
    if(status != FR_OK){
        LOG_ERROR("[%s] file open fail ---%d\n",__FUNCTION__, status);
        goto exit;
    }

    // LOG_ERROR("app data size: app_new:%d, alg_size:%d\n", sizeof(sys_app_data_new_t), sizeof(sensor_alg_t));

    // check magic 
    status = f_read(fp, &magic_num, sizeof(magic_num), &written_bytes);
    if(status != FR_OK){
        LOG_ERROR("[%s] file ead fail ---%d\n",__FUNCTION__, status);
        goto exit;
    }

    f_lseek(fp, 0);
    if(magic_num == SYS_APP_DATA_NEW_MAGIC){
        app_data_new = (sys_app_data_new_t*)app_data;
        status = f_read(fp, app_data_new, sizeof(sys_app_data_new_t), &written_bytes);
        if(status != FR_OK){
            LOG_ERROR("[%s] file ead fail ---%d\n",__FUNCTION__, status);
            goto exit;
        }

        /*(written_bytes != sizeof(sys_app_data_t)){
            LOG_ERROR("[%s] file read bytes failed ---%d\n",__FUNCTION__, written_bytes);
            goto exit;
        }*/

        status = f_read(fp, &dev_statistics, sizeof(dev_statistics), &written_bytes);
        if(status != FR_OK){
            LOG_ERROR("[%s] file ead fail ---%d\n",__FUNCTION__, status);
            goto exit;
        }
        
        f_close(fp);

        weather_info = (WeatherGetRealtimeInfoResult *)ry_malloc(sizeof(WeatherGetRealtimeInfoResult));
        if(weather_info == NULL){
            LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }

        ry_memcpy(&city_list, &(app_data_new->city_list), sizeof(WeatherCityList));
        if (city_list.city_items_count > 5) {
            ry_memset((void*)&city_list, 0, sizeof(WeatherCityList));
        }

        ry_memcpy(weather_info, &(app_data_new->weather_info), sizeof(WeatherGetRealtimeInfoResult));
        if (weather_info->infos_count > 5) {
            ry_memset((void*)weather_info, 0, sizeof(WeatherGetRealtimeInfoResult));
        }
        
        //ry_memcpy(&clock_list, &(app_data->clock_list), sizeof(AlarmClockList));
        ry_memcpy(&s_alg, &(app_data_new->s_alg), sizeof(sensor_alg_t));
        if (s_alg.step_cnt_today > ALG_MAX_STEP_TODAY){
            ry_memset(&s_alg, 0, sizeof(sensor_alg_t));
        }
        
        ry_memcpy(&sleep_time, &(app_data_new->sleep_time), sizeof(alg_sleep_time_t));
        //_memcpy(&dev_statistics, &(app_data->dev_statistics), sizeof(DevStatisticsResult));
        sports_global.last_hrm_time = app_data_new->last_hrm_time;

        batt_percent_recover(app_data_new->batt_last);
        data_sports_distance_offset_today_restore(app_data_new->data_sports_offset_distance_today);
        data_calorie_offset_today_restore(app_data_new->data_calorie_offset_today);
        uint32_t surface_id = app_data_new->curSurfaceId;
        ry_free(app_data);
        ry_free(fp);
        app_data = NULL;
        fp = NULL;
        surface_on_exflash_set_id(surface_id);
    }else{

        status = f_read(fp, app_data, sizeof(sys_app_data_t), &written_bytes);
        if(status != FR_OK){
            LOG_ERROR("[%s] file ead fail ---%d\n",__FUNCTION__, status);
            goto exit;
        }

        /*(written_bytes != sizeof(sys_app_data_t)){
            LOG_ERROR("[%s] file read bytes failed ---%d\n",__FUNCTION__, written_bytes);
            goto exit;
        }*/

        status = f_read(fp, &dev_statistics, sizeof(dev_statistics), &written_bytes);
        if(status != FR_OK){
            LOG_ERROR("[%s] file ead fail ---%d\n",__FUNCTION__, status);
            goto exit;
        }

        file_size = f_size(fp);
        /*u32_t temp_size = sizeof(sys_app_data_t) + sizeof(dev_statistics);
        u8_t version[20] = {0}; 

        get_fw_ver(version, &magic_num);*/

        f_close(fp);

        weather_info = (WeatherGetRealtimeInfoResult *)ry_malloc(sizeof(WeatherGetRealtimeInfoResult));
        if(weather_info == NULL){
            LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
            goto exit;
        }
        
        ry_memcpy(&city_list, &(app_data->city_list), sizeof(WeatherCityList));
        if (city_list.city_items_count > 5) {
            ry_memset((void*)&city_list, 0, sizeof(WeatherCityList));
        }

        
        ry_memcpy(weather_info, &(app_data->weather_info), sizeof(WeatherGetRealtimeInfoResult));
        if (weather_info->infos_count > 5) {
            ry_memset((void*)weather_info, 0, sizeof(WeatherGetRealtimeInfoResult));
        }
        
        //ry_memcpy(&clock_list, &(app_data->clock_list), sizeof(AlarmClockList));
        ry_memcpy(&s_alg, &(app_data->s_alg), sizeof(sensor_alg_t));
        if (s_alg.step_cnt_today > ALG_MAX_STEP_TODAY){
            ry_memset(&s_alg, 0, sizeof(sensor_alg_t));
        }
        
        ry_memcpy(&sleep_time, &(app_data->sleep_time), sizeof(alg_sleep_time_t));
        //_memcpy(&dev_statistics, &(app_data->dev_statistics), sizeof(DevStatisticsResult));
        sports_global.last_hrm_time = app_data->last_hrm_time;

        batt_percent_recover(app_data->batt_last);
    }

    ry_hal_rtc_get();
	ryos_utc_set(get_utc_tick());
    //f_unlink(SYSTEM_APP_DATA_FILE_NAME);
exit:
    if(fp != NULL)
    {
        ry_free(fp);
    }
    if(app_data != NULL)
    {
        ry_free(app_data);
    }
    
}

void clear_user_data(void)
{
    f_unlink(SYSTEM_APP_DATA_FILE_NAME);
    memset(&city_list, 0, sizeof(city_list));
    memset(&sports_global, 0 , sizeof(sports_global));
    memset(&dev_locat, 0, sizeof(dev_locat));
    memset(&cur_status, 0, sizeof(cur_status));
    memset(&s_alg, 0, sizeof(s_alg));
    FREE_PTR(weather_info);
    clear_save_data();
    clear_reset_count();
    motar_stop();
}



void debug_info_to_internal_flash(void * data, int len)
{
    u8_t * ptr = (u8_t *)data;
    u32_t i ;

    extern uint8_t frame_buffer[RM_OLED_IMG_24BIT];
    //memcpy(frame_buffer, (void *)0xFC000, 8*1024);

    for(i = 0; i < 8*1024; i++){
        frame_buffer[i] = *((u8_t*)(0xFC000 + i));
    }
    //memcpy(&frame_buffer[(0xFD000 - -0xFC000)], data, len);
    for(i = 0; i < len;i++){
        frame_buffer[i + (0xFD000 - 0xFC000)] = *(((u8_t *)data) + i);
    }
    am_bootloader_program_flash_page(0xFC000, (u32_t *)frame_buffer, 8*1024);
}


void debug_info_save(void * data, int len)
{
    debug_info_to_internal_flash(data, len);
    add_reset_count(FAULT_RESTART_COUNT);
}


#pragma pack(1)
typedef struct
{
    //
    // Stacked registers
    //
    volatile uint32_t u32R0;
    volatile uint32_t u32R1;
    volatile uint32_t u32R2;
    volatile uint32_t u32R3;
    volatile uint32_t u32R12;
    volatile uint32_t u32LR;
    volatile uint32_t u32PC;
    volatile uint32_t u32PSR;

    //
    // Other data
    //
    volatile uint32_t u32FaultAddr;
    volatile uint32_t u32BFAR;
    volatile uint32_t u32CFSR;
    volatile uint8_t  u8MMSR;
    volatile uint8_t  u8BFSR;
    volatile uint16_t u16UFSR;
    volatile uint32_t fault_type;
    volatile uint8_t  curTask[40];

} am_fault_t;


/**
 * @brief   data_device_restart_type_get
 *
 * @param   None
 *
 * @return  data_device_restart_type
 */
u32_t data_device_restart_type_get(void)
{
    return data_device_restart_type;
}


/**
 * @brief   data_device_restart_type_get
 *
 * @param   type: data_device_restart_type
 *
 * @return  None
 */
void data_device_restart_type_save(u32_t type)
{
    data_device_restart_type = type;
}

//
// Restore the default structure alignment
//
#pragma pack()

void debug_info_print(void)
{
    am_fault_t * info = (am_fault_t *)0xFD000;
    if(info->fault_type != 0xffffffff){
        LOG_ERROR("\n\r\n\r\n\rHard Fault stacked data\n\r");
        LOG_ERROR("R0  = 0x%08X\n", info->u32R0);
        LOG_ERROR("R1  = 0x%08X\n", info->u32R1);
        LOG_ERROR("R2  = 0x%08X\n", info->u32R2);
        LOG_ERROR("R3  = 0x%08X\n", info->u32R3);          //handler模式的错误PC
        LOG_ERROR("R12 = 0x%08X\n", info->u32R12);
        LOG_ERROR("LR  = 0x%08X\n", info->u32LR);          //线程模式的错误LR
        LOG_ERROR("PC  = 0x%08X\n", info->u32PC);          //线程模式的错误PC
        LOG_ERROR("PSR = 0x%08X\n", info->u32PSR);         //handler模式的错误LR

        LOG_ERROR("Other Hard Fault data:\n");
        LOG_ERROR("CFSR (cfg Fault stat Reg) = 0x%08X\n", info->u32BFAR);
        LOG_ERROR("BFAR (Bus Fault Addr Reg) = 0x%08X\n", info->u32BFAR);
        
        LOG_ERROR("MMSR (Mem Mgmt Fault Stat Reg) = 0x%02X\n", info->u8MMSR);
        LOG_ERROR("BFSR (Bus Fault Status Reg)    = 0x%02X\n", info->u8BFSR);
        LOG_ERROR("UFSR (Usage Fault Status Reg)  = 0x%04X\n", info->u16UFSR);

        LOG_ERROR("Fault address = 0x%08X\n", info->u32FaultAddr);

        LOG_ERROR("hard fault type:%d\n", info->fault_type);
        LOG_ERROR("last reset Task: %s\n", info->curTask);
        // set_reset_count(FAULT_RESTART_COUNT, info->fault_cnt);
    }

    uint8_t version_string[24];
    uint8_t ret_len;    
    device_info_get(DEV_INFO_TYPE_VER, version_string, &ret_len);
    LOG_ERROR("fwVer: %s, mfgSt:0x%x.\n", version_string, dev_mfg_state_get());

    // get STAT resister Bits and Description    
    const char* stat[] = {  "external", 
                            "powerOn",
                            "brownOut",
                            "SW POR",
                            "sftPOI",
                            "debug",
                            "Watchdog",
                         };
    int reset_src = ry_get_reset_stat();
    for(int k = 0; k < 7; k ++){
        if (reset_src & (1 << k)){
            LOG_ERROR("RESET status: 0x%02x, source: %s\n", reset_src, stat[k]);
        }
    }
    if (reset_src & (1 << 6)){
        add_reset_count(WDT_RESTART_COUNT);
        if(info->fault_type == 0xffffffff){
            LOG_ERROR("wdt reset no last word\r\n");            
        }
    }

    // get application, soft and hard resets and Description    
    const char* reset_str[TOTAL_RESTART_COUNT] = { "dev", 
                                "usr",
                                "wdt",
                                "qrc",
                                "mgc",
                                "upg",
                                "bat",
                                "mem",
                                "dog",
                                "fau",
                                "crd",
                                "fss",
                                "facfs",                               
                                "fac",
                                "max",

                                "spi",  
                                "ast",
                                "i2c",
                                "wms",};
                                
    for(int i = 0; i < MAX_RESTART_COUNT; i++){
        if (get_reset_count(i) > 0){
            LOG_ERROR("reset source %02d: %s, cnt:%d\n", i, reset_str[i], get_reset_count(i));
        }
    }

    uint32_t type = get_last_reset_type();
    if (type >= MAX_RESTART_COUNT){
        if (type == SPI_TIMEOUT_RESTART){
            type = ID_SPI_TIMEOUT_RESTART;
        }
        else if (type == ASSERT_RESTART){
            type = ID_ASSERT_RESTART;
        }
        else if (type == I2C_TIMEOUT_RESTART){
            type = ID_I2C_TIMEOUT_RESTART;
        }
        else if (type == WMS_CREAT_START){
            type = ID_WMS_CREAT_START;
        }
        else{
            type =  MAX_RESTART_COUNT;
        }
    }
    LOG_ERROR("last reset type: %d, %s\n", get_last_reset_type(), reset_str[type]);
 
    data_device_restart_type_save(type);

    // powerOn, SW POR, user reset, upgade reset
    if ((reset_src & 0x02) || (reset_src & 0x08) \
        || (type == MAGIC_RESTART_COUNT) || (type == UPGRADE_RESTART_COUNT)){
        device_set_powerOn_mode(1);
    }

    if(info->fault_type != 0xffffffff){
        u32_t log_len = sizeof(am_fault_t);
        u8_t* log_info = (u8_t *)ry_malloc(log_len);
        if(log_info == NULL){
            LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
            goto exit;
        }
        ry_memset(log_info, 0xff, log_len);
        debug_info_to_internal_flash(log_info, log_len);
        ry_free(log_info);
    }

    //add_reset_count(MAX_RESTART_COUNT);    //clear last reset type

exit:
    return;
}

u32_t pack_sleep_data_4byte(u32_t bpm, u32_t sstd, u8_t rri0, u8_t rri1, u8_t rri2)
{
    u32_t data_byte = 0;

    u8_t * data_ptr = (u8_t *)&data_byte;
    u8_t count = 3;
    u8_t value_1 = 0, value_2 = 0;
    u16_t* rri_ptr = (u16_t *)(&data_ptr[2]);

    data_ptr[0] = (u8_t)bpm;
    data_ptr[1] = (u8_t)sstd;
    
    
    *rri_ptr = (rri0 | (rri1 << 6) | (rri2 << 11));
    
    
    
    
    return data_byte;
    
}



#define ERROR_INFO_BUF_SIZE             1024

#define ERROR_INFO_MAX_SIZED            (70*1024)
u8_t *error_log_buf = NULL;
FIL * error_info_fp = NULL;
int ry_error_info_init(void)
{
    FRESULT ret = FR_OK;
    int res = 0;
    error_log_buf = (u8_t *)ry_malloc(ERROR_INFO_BUF_SIZE);
    if(error_log_buf == NULL){
        LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
        res = 1;
        return res;
    }

    error_info_fp = (FIL *)ry_malloc(sizeof(FIL));
    if(error_info_fp == NULL){
        LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
        res = 2;
        return res;
    }
    
    ret = f_open( error_info_fp, ERROR_INFO_FILE, FA_OPEN_APPEND | FA_READ | FA_WRITE );
    if(ret != FR_OK){
        LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
        FREE_PTR(error_info_fp);
        FREE_PTR(error_log_buf);
        res = 3;
        return res;
    }
    LOG_DEBUG("[%s] - %d\n", __FUNCTION__, f_size(error_info_fp));
    if(f_size(error_info_fp) > ERROR_INFO_MAX_SIZED){
        f_lseek(error_info_fp, 0);
    }
    f_close(error_info_fp);
		
    return 0;
}

void ry_error_info_save(const char* fmt, ...)
{        
    uint32_t length = 0;
    ry_time_t time;
    FRESULT ret = FR_OK;
    u32_t r = 0;

    if(dev_mfg_state_get() == DEV_MFG_STATE_PCBA){
        return ;
    }
    
    tms_get_time(&time);

    if((error_log_buf == NULL)||(error_info_fp == NULL)){
        return ;
    }

    ret = f_open( error_info_fp, ERROR_INFO_FILE, FA_OPEN_APPEND | FA_WRITE );
    if(ret != FR_OK){
        goto exit;
    }
    
    ry_hal_rtc_get();
    r = ry_hal_irq_disable();
    va_list ap;
    va_start(ap, fmt);
    length = vsnprintf((char*)error_log_buf, ERROR_INFO_BUF_SIZE, fmt, ap);
    va_end(ap);

    if((f_size(error_info_fp) > ERROR_INFO_MAX_SIZED) ){
        /*if(f_tell(error_info_fp) > ERROR_INFO_MAX_SIZED/2){
            if(f_tell(error_info_fp) > ERROR_INFO_MAX_SIZED){
                f_lseek(error_info_fp, 0);
            }else{
                f_truncate(error_info_fp);
            }
        }*/
        f_lseek(error_info_fp, 0);
        f_truncate(error_info_fp);
    }

    ret = f_write(error_info_fp, error_log_buf, strlen((const char*)error_log_buf) + 1, &length);
    if(ret != FR_OK){
        
    }

    ry_hal_irq_restore(r);
    
exit:
    f_close(error_info_fp);
    

}


#define RAW_INFO_BUF_SIZE             1024
#define RAW_INFO_MAX_SIZED            (6 * 128*1024)
u8_t *raw_log_buf = NULL;
FIL * raw_info_fp = NULL;
int ry_rawLog_info_init(void)
{
    FRESULT ret = FR_OK;
    int res = 0;
    // raw_log_buf = (u8_t *)ry_malloc(RAW_INFO_BUF_SIZE);
    if(error_log_buf == NULL){
        LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
        res = 1;
        return res;
    }

    raw_info_fp = (FIL *)ry_malloc(sizeof(FIL));
    if(raw_info_fp == NULL){
        LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
        res = 2;
        return res;
    }
    
    ret = f_open( raw_info_fp, RAWLOG_INFO_FILE, FA_OPEN_APPEND | FA_READ | FA_WRITE );
    if(ret != FR_OK){
        LOG_DEBUG("[%s] malloc failed line : %d\n",__FUNCTION__, __LINE__);
        FREE_PTR(raw_info_fp);
        FREE_PTR(error_log_buf);
        res = 3;
        return res;
    }
    LOG_DEBUG("[%s] - %d\n", __FUNCTION__, f_size(raw_info_fp));
    if(f_size(raw_info_fp) > RAW_INFO_MAX_SIZED){
        f_lseek(raw_info_fp, 0);
    }
    f_close(raw_info_fp);
		
	return 0;
}

volatile u32_t rawLog_info_save_en;
volatile u32_t rawLog_size_total;
volatile u32_t rawLog_len_sector;
volatile u32_t rawLog_overflow;
volatile u32_t rawLog_time_start;
volatile u32_t rawLog_time_stop;
u8_t rawLog_file_flg_opened;
extern FATFS *ry_system_fs;
extern u32_t g_fs_free_size;

void ry_rawLog_info_save_enable_set(u32_t en)
{
    rawLog_info_save_en = en;
    if (rawLog_info_save_en){
        rawLog_time_start = ry_hal_clock_time();
    }
    LOG_INFO("[%s], rawLog_info_save_en:%d\n", __FUNCTION__, rawLog_info_save_en);
}

void ry_rawLog_info_save(const char* fmt, ...)
{        
    u32_t deltaT1 = 0;
    u32_t deltaT2 = 0;
    
    uint32_t length = 0;
    ry_time_t time;
    FRESULT ret = FR_OK;
    u32_t r = 0;
    static uint32_t free_size = 0;

    LOG_INFO("[%s], rawLog_info_save_en:%d\n", __FUNCTION__, rawLog_info_save_en);
    u32_t time_start = ry_hal_clock_time();

    if ((dev_mfg_state_get() == DEV_MFG_STATE_PCBA) || !rawLog_info_save_en || rawLog_overflow){
        return ;
    }
    
    tms_get_time(&time);

    if((error_log_buf == NULL)||(raw_info_fp == NULL)){
        return ;
    }
    
    int fileSize = 0;
    int len_to_wr = 0;        

    if (!rawLog_file_flg_opened){
        ret = f_open( raw_info_fp, RAWLOG_INFO_FILE, FA_OPEN_APPEND | FA_WRITE );
        if(ret != FR_OK){
            goto exit;
        }
        rawLog_file_flg_opened = 1;
    }

    ry_hal_rtc_get();
    r = ry_hal_irq_disable();
    va_list ap;
    va_start(ap, fmt);
    length = vsnprintf((char*)error_log_buf, RAW_INFO_BUF_SIZE, fmt, ap);
    va_end(ap);

    fileSize = (float)f_size(raw_info_fp);

    if ((fileSize > RAW_INFO_MAX_SIZED) /*|| (fileSize > (g_fs_free_size << 12))*/){
        rawLog_len_sector = 0;
        rawLog_overflow = 1;
        f_close(raw_info_fp);
        rawLog_file_flg_opened = 0;
        rawLog_time_stop = ry_hal_clock_time();

        goto exit;
        // f_lseek(raw_info_fp, 0);
        // f_truncate(raw_info_fp);
    }

    len_to_wr = strlen((const char*)error_log_buf);

    if (rawLog_file_flg_opened){
        ret = f_write(raw_info_fp, error_log_buf, len_to_wr, &length);
    }
    // f_close(raw_info_fp);

    // LOG_FATAL("%s", error_log_buf);
    if(ret != FR_OK){
        LOG_INFO("write error. rawLog_size:%d, len_to_wr:%d,  free_size:%dkB\r\n", \
            fileSize, len_to_wr, free_size << 2);
    }
    
exit:
    // f_close(raw_info_fp);
    deltaT1 = ry_hal_calc_ms(ry_hal_clock_time() - time_start);
    //u32_t deltaT2;// = ry_hal_calc_ms(ry_hal_clock_time() - time_start);

    rawLog_len_sector += length;
    rawLog_size_total += length;
    
    if (rawLog_len_sector >= (EXFLASH_SECTOR_SIZE - 128)){
        if (rawLog_file_flg_opened){
            f_sync(raw_info_fp);
            rawLog_len_sector = 0;        
            f_getfree("",(DWORD*)&free_size, &ry_system_fs);
        }
        LOG_DEBUG("rawLog_size:%d, len_to_wr:%d, rawLog_len_sector:%d, rawLog_size_total:%d, free_size:%d, heap:%d\r\n", \
            fileSize, len_to_wr, rawLog_len_sector, rawLog_size_total, free_size << 2, ryos_get_free_heap());
    }
    ry_hal_irq_restore(r);   

    if (rawLog_info_save_en && (GUI_STATE_ON == get_gui_state())){
        if (ry_system_initial_status() != 0){
            uint32_t type = IPC_MSG_TYPE_SURFACE_UPDATE_STEP;
            ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, sizeof(uint32_t), (uint8_t *)&type);
        }
    } 
    deltaT2 = ry_hal_calc_ms(ry_hal_clock_time() - time_start);
    LOG_DEBUG("[LOG_RAW] --------------deltaT1: %d, deltaT2:%d, rawLog_len_sector:%d, rawLog_file_flg_opened:%d\r\n", \
        deltaT1, deltaT2, rawLog_len_sector, rawLog_file_flg_opened);
}


void raw_log_files_reset(void)
{
    f_close(raw_info_fp);
    f_unlink(RAWLOG_INFO_FILE);
    rawLog_overflow = 0;
    rawLog_size_total = 0;
    rawLog_file_flg_opened = 0;
    f_getfree("",(DWORD*)&g_fs_free_size, &ry_system_fs);
}


dev_log_upload_status_t log_status = {0};


void error_log_start_result(ry_sts_t status, void* usr_data)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)usr_data, status);        
    } else {
        ry_data_msg_send(DATA_SERVICE_DEV_LOG_UPLOAD, 0, NULL);
    }
}


void dump_print_to_buf(void * buf, const char* fmt, ...)
{
    int length;
    va_list ap;
    va_start(ap, fmt);
    length = vsprintf(buf, fmt, ap);
    va_end(ap);
}

void dump_struct_to_buf(void * buf, void * struct_p, int len)
{
    int i;
    u8_t * data = (u8_t *)struct_p;
    char * t_buf = (char *)buf;
    for (i = 0; i < len; i++) {
        dump_print_to_buf(&t_buf[strlen((char *)buf)], "%02x ", data[i]);
    }
    
}

#define DUMP_DATA(buf,x)        do{dump_print_to_buf(&buf[strlen(buf)],"\n%s:", #x);\
                                    dump_struct_to_buf(&buf[strlen(buf)], &x, sizeof(x));\
                                }while(0)


#define DUMP_RAW(_buf, _data, _size,_tag)        do{dump_print_to_buf(&_buf[strlen(_buf)],"\n%s:", _tag);\
                                                    dump_struct_to_buf(&_buf[strlen(_buf)], _data, _size);\
                                                 }while(0)

void dump_raw_data(void)
{
    uint32_t length = 0;
    FRESULT ret = FR_OK;
    extern u8_t frame_buffer[];
    char * raw_data_str = (char *)frame_buffer; //(char *)ry_malloc(4096);
    int i = 0 ;

    if(raw_data_str == NULL){
        return ;
    }

    for( i = 0; i < 25; i++){
        ry_hal_spi_flash_sector_erase(i * EXFLASH_SECTOR_SIZE);
    }

    ry_hal_spi_flash_write(frame_buffer, 0, RM_OLED_IMG_24BIT);
    ry_memset(raw_data_str, 0, 4096);

    extern ry_device_setting_t device_setting;
    DUMP_DATA(raw_data_str, device_setting);

    extern cms_ctx_t cms_ctx_v;
    DUMP_DATA(raw_data_str, cms_ctx_v);

    extern wms_ctx_t wms_ctx_v;
    DUMP_DATA(raw_data_str, wms_ctx_v);
    
    extern ry_dev_state_t device_state;
    DUMP_DATA(raw_data_str, device_state);

    extern tms_ctx_t tms_ctx_v;
    DUMP_DATA(raw_data_str, tms_ctx_v);

    extern void* appDb;
    DUMP_RAW(raw_data_str , (&appDb), 552, "BleDB");

    extern void* pRecListNvmPointer;
    DUMP_RAW(raw_data_str , pRecListNvmPointer, 528, "BleDBFlash");

    extern void* p_stored_notify_whitelist;
    DUMP_RAW(raw_data_str , p_stored_notify_whitelist, 680, "AncsList");

    extern void* p_surface_stored;
    DUMP_RAW(raw_data_str , p_surface_stored, 392, "Surface");

    extern ry_DevStatisticsResult_t dev_statistics;
    DUMP_DATA(raw_data_str, dev_statistics);

    extern alg_ctx_t alg_ctx_v;
    DUMP_DATA(raw_data_str, alg_ctx_v);

    ret = f_open( error_info_fp, ERROR_INFO_FILE, FA_OPEN_APPEND | FA_WRITE );
    if(ret != FR_OK){
    }

    ret = f_write(error_info_fp, raw_data_str, strlen((const char*)raw_data_str) + 1, &length);
    if(ret != FR_OK){
    }

    f_close(error_info_fp);
    // LOG_DEBUG("%s\n", raw_data_str);

    ry_hal_spi_flash_read(frame_buffer, 0, RM_OLED_IMG_24BIT);
    //ry_free(raw_data_str);
}

ry_sts_t error_log_upload_start(void )
{
    DevLogStartResult result = {0};
    ry_sts_t status;
    u32_t code = 0;
    FRESULT res = FR_OK;
    result.session_id = ryos_rtc_now();
    log_status.session_id = result.session_id;
    log_status.cur_sn = 0;
    log_status.status = DEV_ERROR_LOG_UPPING;

    u8_t phoneType;
    int  connInterval = 0;

    dump_raw_data();
    
    u8_t * buf = (u8_t *)ry_malloc(sizeof(DevLogStartResult) + 10);

    //FIL * fp = (FIL *)ry_malloc(sizeof(fp));
    if(error_info_fp == NULL){
        code = RBP_RESP_CODE_NO_MEM;
    }else{
        f_close(error_info_fp);
        res = f_open(error_info_fp, ERROR_INFO_FILE, FA_READ);
        result.total = (f_size(error_info_fp) + ERROR_LOG_MAX_SIZE - 1)/ERROR_LOG_MAX_SIZE;
        log_status.total = result.total;
        if(res != FR_OK){
            code = RBP_RESP_CODE_NOT_FOUND;
        }
        f_close(error_info_fp);
    }

    if(code != RBP_RESP_CODE_SUCCESS){
        memset(&log_status, 0, sizeof(log_status));
    }else{
        //ry_data_msg_send(DATA_SERVICE_DEV_LOG_UPLOAD, 0, NULL);
    }
    

    pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(DevLogStartResult ) + 10));

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream_o, DevLogStartResult_fields, &result)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("[%s]code :%d, total :%d\n",__FUNCTION__,code,result.total);

    status = ble_send_response(CMD_DEV_LOG_START, code, 
                        buf, stream_o.bytes_written, 1, error_log_start_result, NULL);



    uint8_t conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);

    return status;
}
void error_log_send_result(ry_sts_t status, void* usr_data)
{
    if (status != RY_SUCC) {
        LOG_DEBUG("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)usr_data, status);     

        if(error_info_fp != NULL){
            f_close(error_info_fp);
        }
    } else {
        LOG_DEBUG("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)usr_data, status);
    }
}


void error_log_upload(void)
{
    FRESULT res = FR_OK;
    DevLogData * data = (DevLogData *)ry_malloc(sizeof(DevLogData));
    u8_t * buf = NULL;
    pb_ostream_t stream_o;
    int read_bytes = 0;
    if(data == NULL){
        goto exit;
    }

    if(log_status.total == 0){
        memset(&log_status, 0, sizeof(log_status));
        goto exit;
    }
    
    if(log_status.cur_sn == 0){
        f_close(error_info_fp);
        LOG_DEBUG("[%s]---start first\n", __FUNCTION__);
        res = f_open(error_info_fp, ERROR_INFO_FILE, FA_READ);
        if(res != FR_OK){
            goto exit;
        }
    }/*else{
        
        
        
    }*/

    data->session_id = log_status.session_id;
    data->sn = log_status.cur_sn;

    res = f_read(error_info_fp, data->content.bytes, ERROR_LOG_MAX_SIZE, (UINT*)(&(read_bytes)));
    if(res != FR_OK){
        goto exit;
    }

    data->content.size = read_bytes;

    buf = (u8_t *)ry_malloc(sizeof(DevLogData) +100);
    if(buf == NULL){
        goto exit;
    }

    
    stream_o = pb_ostream_from_buffer(buf, (sizeof(DevLogData) +100));

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream_o, DevLogData_fields, data)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }


    ble_send_request(CMD_APP_LOG_DATA, buf, stream_o.bytes_written, 
        1, error_log_send_result, (void*)__FUNCTION__);

    log_status.cur_sn++;

    if(log_status.cur_sn == log_status.total){
        memset(&log_status, 0, sizeof(log_status));
        f_close(error_info_fp);
        f_unlink(ERROR_INFO_FILE);
        LOG_DEBUG("[%s]---end\n", __FUNCTION__);
    }else{
        //ry_data_msg_send(DATA_SERVICE_DEV_LOG_UPLOAD, 0, NULL);
    }

    LOG_DEBUG("[%s]error log session id: %d\n sn : %d  bytes :%d\n",__FUNCTION__,data->session_id, data->sn,\
            data->content.size);

exit:
    ry_free(data);
    ry_free(buf);
}


u32_t get_last_hrm_time(void)
{
    return sports_global.last_hrm_time;
}

u32_t get_sport_cur_session_id(void)
{
    return sports_global.cur_session_id;
}


void raw_data_start_result(ry_sts_t status, void* usr_data)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)usr_data, status);        
    } else {
        ry_data_msg_send(DATA_SERVICE_DEV_RAW_DATA_UPLOAD, 0, NULL);
    }
}



ry_sts_t raw_data_upload_start(void )
{
    DevLogStartResult result = {0};
    ry_sts_t status;
    u32_t code = 0;
    FRESULT res = FR_OK;
    result.session_id = ryos_rtc_now();
    log_status.session_id = result.session_id;
    log_status.cur_sn = 0;
    log_status.status = DEV_ERROR_LOG_UPPING;

    u8_t phoneType;
    int  connInterval = 0;

    // dump_raw_data();
    
    u8_t * buf = (u8_t *)ry_malloc(sizeof(DevLogStartResult) + 10);

    //FIL * fp = (FIL *)ry_malloc(sizeof(fp));
    if(raw_info_fp == NULL){
        code = RBP_RESP_CODE_NO_MEM;
    }else{
        f_close(raw_info_fp);
        res = f_open(raw_info_fp, RAWLOG_INFO_FILE, FA_READ);
        result.total = (f_size(raw_info_fp) + ERROR_LOG_MAX_SIZE - 1)/ERROR_LOG_MAX_SIZE;
        log_status.total = result.total;
        if(res != FR_OK){
            code = RBP_RESP_CODE_NOT_FOUND;
        }
        f_close(raw_info_fp);
    }

    if(code != RBP_RESP_CODE_SUCCESS){
        memset(&log_status, 0, sizeof(log_status));
    }else{
        //ry_data_msg_send(DATA_SERVICE_DEV_LOG_UPLOAD, 0, NULL);
    }
    

    pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(DevLogStartResult ) + 10));

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream_o, DevLogStartResult_fields, &result)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }

    LOG_DEBUG("[%s]code :%d, total :%d\n",__FUNCTION__,code,result.total);

    status = ble_send_response(CMD_DEV_LOG_RAW_START, code, 
                        buf, stream_o.bytes_written, 1, raw_data_start_result, NULL);


    uint8_t conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);

    return status;
}


void raw_data_upload(void)
{
    FRESULT res = FR_OK;
    DevLogData * data = (DevLogData *)ry_malloc(sizeof(DevLogData));
    u8_t * buf = NULL;
    pb_ostream_t stream_o;
    int read_bytes = 0;
    if(data == NULL){
        goto exit;
    }

    if(log_status.total == 0){
        memset(&log_status, 0, sizeof(log_status));
        goto exit;
    }
    
    if(log_status.cur_sn == 0){
        f_close(raw_info_fp);
        LOG_DEBUG("[%s]---start first\n", __FUNCTION__);
        res = f_open(raw_info_fp, RAWLOG_INFO_FILE, FA_READ);
        if(res != FR_OK){
            goto exit;
        }
    }/*else{
        
        
        
    }*/

    data->session_id = log_status.session_id;
    data->sn = log_status.cur_sn;

    res = f_read(raw_info_fp, data->content.bytes, ERROR_LOG_MAX_SIZE, (UINT*)(&(read_bytes)));
    if(res != FR_OK){
        goto exit;
    }

    data->content.size = read_bytes;

    buf = (u8_t *)ry_malloc(sizeof(DevLogData) +100);
    if(buf == NULL){
        goto exit;
    }

    
    stream_o = pb_ostream_from_buffer(buf, (sizeof(DevLogData) +100));

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream_o, DevLogData_fields, data)) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
    }


    ble_send_request(CMD_APP_LOG_RAW_DATA, buf, stream_o.bytes_written, 
        1, error_log_send_result, (void*)__FUNCTION__);

    log_status.cur_sn++;

    if(log_status.cur_sn == log_status.total){
        memset(&log_status, 0, sizeof(log_status));
        raw_log_files_reset();
        LOG_DEBUG("[%s]---end\n", __FUNCTION__);
    }else{
        //ry_data_msg_send(DATA_SERVICE_DEV_LOG_UPLOAD, 0, NULL);
    }

    LOG_DEBUG("[%s]error log session id: %d\n sn : %d  bytes :%d\n",__FUNCTION__,data->session_id, data->sn,\
            data->content.size);

exit:
    ry_free(data);
    ry_free(buf);
}











