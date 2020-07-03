#ifndef __DATA_MANAGEMENT_SERVICE_H__



#define __DATA_MANAGEMENT_SERVICE_H__
#include "ry_type.h"
#include "ff.h"

#include "data_management_service.h"
#include "string.h"
#include "Notification.pb.h"
#include "rbp.pb.h"
#include "rbp.h"
#include "ryos.h"
#include "ry_utils.h"
#include "pb.h"
#include "nanopb_common.h"
#include "scheduler.h"
#include "Location.pb.h"
#include "Sport.pb.h"
#include "DataUpload.pb.h"
#include "BodyStatus.pb.h"
#include "DevLog.pb.h"



//#include "Health.pb.h"
#include "Weather.pb.h"
#include "timer_management_service.h"
#include <stdint.h>
#include <stdarg.h>



#define STEP_DATA_FILE_NAME             "step.data"
#define HEART_DATA_FILE_NAME            "heart.data"
#define SLEEP_DATA_FILE_NAME            "sleep.data"
#define RUN_DATA_FILE_NAME              "run.data"
#define JUMP_DATA_FILE_NAME             "jump.data"

#define SWIMMING_DATA_FILE_NAME         "swimming.data"

#define BIKE_DATA_FILE_NAME             "bike.data"


#define SYSTEM_APP_DATA_FILE_NAME       "sysAppFile.data"

#define WEATHER_FORECAST_DATA           "weather.data"

#define BODY_STATUS_DATA                "bodyStatus.data"

#define RETRY_UPLOAD_DATA               "retry.data"

#define ERROR_INFO_FILE                 "error.data"
#define RAWLOG_INFO_FILE                 "rawLog.data"


#define DATA_FILE_MAGIC                 0x0001DEAD

#define SYS_APP_DATA_NEW_MAGIC          0x1111BEEF

#define DATA_SET_BYTE_MAX_SIZE          1500

#define DATA_SERVICE_STEP_FLAG              (1<<0)

#define DATA_SERVICE_HEART_FLAG             (1<<1)

#define DATA_SERVICE_SLEEP_FLAG             (1<<2)

#define DATA_SERVICE_RUN_FLAG               (1<<3)

#define DATA_SERVICE_JUMP_FLAG              (1<<4)

#define DATA_SERVICE_SWIMMING_FLAG          (1<<5)

#define DATA_SERVICE_BIKE_FLAG              (1<<6)



#define SPORT_FLAG_SET(x, flag)             (x |= flag)

#define SPORT_FLAG_CLEAR(x, flag)           (x &= (flag ^ 0xffffffff))

#define SPORT_FLAG_GET(x, flag)             ((x&flag)?(1):(0))



#define DATASET_MAX_RECORD                  20

#define SLEEP_LITE_VERSION                  1


#define SLEEP_STATUS_WAKE                   3
#define SLEEP_STATUS_REM                    2
#define SLEEP_STATUS_LIGHT                  1
#define SLEEP_STATUS_DEEP                   0


#define SPORT_DATA_MAX_SIZE                 (15*1024)

#define DEV_ERROR_LOG_IDLE                  0
#define DEV_ERROR_LOG_UPPING                1

#define ERROR_LOG_MAX_SIZE                  1900

#define DATA_UPLOAD_RESEND_MAX              3

#define TYPE_RESTING_HEART                  3


#define DATA_SET_MAX_SIZE                   2048

#define GPS_LOC_CHANGED_STATUS_NONE         0
#define GPS_LOC_CHANGED_STATUS_LOST         1
#define GPS_LOC_CHANGED_STATUS_RECOVERY     2


#define SPORT_RECORD_MAX_POINT              5



typedef enum data_service_msg{

    DATA_SERVICE_MSG_START_STEP,

    DATA_SERVICE_MSG_START_HEART,

    DATA_SERVICE_MSG_START_HEART_RUN,

    DATA_SERVICE_MSG_START_HEART_SLEEP,

    DATA_SERVICE_MSG_START_SLEEP_DEEP,

    DATA_SERVICE_MSG_START_SLEEP_LIGHT,

    DATA_SERVICE_MSG_START_SLEEP_WAKE,

    DATA_SERVICE_MSG_START_RUN,

    DATA_SERVICE_MSG_START_JUMP,

    DATA_SERVICE_MSG_START_SWIMMING,

    DATA_SERVICE_MSG_START_BIKE,
    
    DATA_SERVICE_MSG_START_FREE,    

    DATA_SERVICE_MSG_STOP_STEP,

    DATA_SERVICE_MSG_STOP_HEART,
    
    DATA_SERVICE_MSG_STOP_HEART_RESTING,

    DATA_SERVICE_MSG_STOP_HEART_AUTO,

    DATA_SERVICE_MSG_STOP_HEART_RUN,

    DATA_SERVICE_MSG_STOP_HEART_SLEEP,

    DATA_SERVICE_MSG_STOP_SLEEP_DEEP,

    DATA_SERVICE_MSG_STOP_SLEEP_WAKE,

    DATA_SERVICE_MSG_STOP_SLEEP_LIGHT,

    DATA_SERVICE_MSG_STOP_RUN,

    DATA_SERVICE_MSG_STOP_JUMP,

    DATA_SERVICE_MSG_STOP_SWIMMING,

    DATA_SERVICE_MSG_STOP_BIKE,
    
    DATA_SERVICE_MSG_STOP_FREE,    

    DATA_SERVICE_MSG_GET_LOCATION,

    DATA_SERVICE_MSG_GET_REAL_WEATHER,

    DATA_SERVICE_MSG_SLEEP_SESSION_BEGINNING,

    DATA_SERVICE_MSG_SLEEP_SESSION_FINISHED,

    DATA_SERVICE_MSG_START_SLEEP,

    DATA_SERVICE_MSG_STOP_SLEEP,

    DATA_SERVICE_MSG_SET_SLEEP_DATA,

    DATA_SERVICE_UPLOAD_DATA_START,

    DATA_SERVICE_BODY_STATUS_CHANGE,

    DATA_SERVICE_DEV_LOG_UPLOAD,

    DATA_SERVICE_DEV_RAW_DATA_UPLOAD,

}data_service_msg_t;


typedef enum{
    GET_DATARECORD_RESULT_OK = 0,
    GET_DATARECORD_RESULT_EMPTY = 1,
    GET_DATARECORD_RESULT_FORMAT_ERROR = 2,
    GET_DATARECORD_RESULT_MALLOC_FAILED,
    GET_DATARECORD_RESULT_FILE_FAILED,
    
}GET_DATARECORD_RESULT_T;




typedef enum sport_data_type{
    SPORT_DATA_STEP,
    SPORT_DATA_HEART,
    SPORT_DATA_SLEEP,
    SPORT_DATA_RUN,
    SPORT_DATA_JUMP,
    SPORT_DATA_SWIMMING,
    SPORT_DATA_BIKE,
    SPORT_DATA_FREE,
    SPORT_DATA_END,
}sport_data_type_t;

typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t*  data;
} ry_data_msg_t;


typedef struct {
    double longitude;
    double latitude;
}dev_location_t;

typedef struct {
    u32_t start_time;
    u32_t end_time;
    u32_t rri_data_count;
    u32_t session_id;
    u32_t rri_data[90];
    u32_t sn;
    u32_t step;

}sleep_record_rri_t;

typedef struct{
    u8_t is_sleep;
    u8_t run_disable;
    u32_t last_status;
    u32_t last_sn;
    u32_t cur_session_id;
    u32_t sleep_session_id;
    u32_t last_step;
    u32_t last_hrm_time;
    u32_t sleep_sn;
    u32_t end_sleep_tick;
    u32_t last_sport_mode;
}sports_global_status_t;

typedef struct{
    u32_t session_id;
    u32_t cur_sn;
    u32_t total;
    u32_t status;
}dev_log_upload_status_t;

#define DATA_SET_MAX_RECORD             100
typedef struct{
    u32_t num;
    u16_t len[DATA_SET_MAX_RECORD];
    DataRecord * data[DATA_SET_MAX_RECORD];
}dataSet_repeat_param_t;


#pragma pack(push)
#pragma pack(1)

typedef struct {
    u32_t pack_len;
    BodyStatusRecord record;
}record_save_type_t;

#pragma pack(pop)


//extern StepRecord dev_step;
extern WeatherCityList  city_list;
extern WeatherGetRealtimeInfoResult * weather_info;

void data_service_set_enable(uint8_t f_enable);
uint8_t data_service_enable_status_get(void);

void set_running(void);

void clear_running(void);

u8_t get_running_flag(void);

FRESULT remove_sport_data(int data_type);

ry_sts_t location_result(u8_t * data, u32_t size);

void get_location(void );

ry_sts_t save_sport_data(int data_type, void * data);

ry_sts_t get_sport_data(int data_type, void *data, u32_t offset, u32_t len);

u32_t get_sport_data_size(int data_type);


ry_sts_t ry_data_msg_send(u32_t msgType, u32_t len, u8_t* data);

DataSet * pack_data_set(int data_type, u8_t day);

void repo_weather_city(void);
void set_weather_city(uint8_t* data, int len);

void get_real_weather(void);

void get_real_weather_result(uint8_t* data, int len);

void get_forecast_weather(void);

void get_forecast_weather_result(uint8_t* data, int len);

void get_weather_info(void);

void get_weather_info_result(uint8_t* data, int len);

void system_app_data_save(void);

void system_app_data_init(void);

void debug_info_save(void * data, int len);

void debug_info_print(void);

u32_t pack_body_data_record(DataRecord * data_record, u8_t day);
u32_t pack_sleep_data_record(DataRecord * data_record, u8_t day);


void delete_body_status(u32_t delete_num);

u32_t pack_body_status(u32_t read_record, u8_t ** desk_data, u32_t * pack_size, u8_t day);

u32_t get_sleep_record_data(SleepStatusRecord * sleep_record, u8_t day);

void stop_cur_record(void);
void start_cur_record(u32_t status);

void set_run_point(float);

void set_sport_point(float distance);


void clear_user_data(void);

int get_data_set_num(void);

int ry_error_info_init(void);

void ry_error_info_save(const char* fmt, ...);

ry_sts_t error_log_upload_start(void );

void error_log_upload(void);

void data_ble_send_callback(ry_sts_t status, void* arg);

void data_ble_upload_callback(ry_sts_t status, void* arg);

u32_t check_retry_data(void);

u32_t get_last_hrm_time(void);

u32_t get_sleep_status(void);

void data_upload_stop_handle(void* arg);


void set_current_running_disable(void);

void set_current_running_enable(void);

u8_t get_current_running_status(void);

void test_save_upload_data(void);

void close_body_status_read(void);
void open_body_status_read(u8_t day);
void data_service_task_init(void);

void system_app_data_save(void);

void stop_record_run(void);
void stop_record_ride(void);

void start_record_run(void);
void start_record_indoor_run(void);
void start_record_ride(void);


void update_run_param(void);
void update_ride_param(void);

ry_sts_t update_run_result(u8_t * data, u32_t size);
void pause_record_run(void);
void pause_record_ride(void);

void resume_record_run(void);
void resume_record_ride(void);

bool encode_data_record(pb_ostream_t *stream, const pb_field_t *field, void * const * arg);
bool encode_repeated_record(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

u32_t get_day_number(void);

void data_sports_distance_offset_update(float offset);
void  data_sports_distance_offset_clear(uint32_t flg_all);
float data_sports_distance_app_offset_get(void);
float data_sports_distance_today_offset_get(void);
float app_get_step_distance(uint32_t step);
float app_get_distance_today(void);

void data_calorie_offset_update(float calorie);
void data_calorie_offset_clear(uint32_t flg_all);
float data_sports_calorie_offset_get(void);
float app_get_calorie_today(void);

uint32_t gps_location_succ_get(void);
void gps_location_succ_clear(void);
uint32_t gps_location_changed_status_get(void);
void gps_location_changed_status_set(uint32_t changed_st);

u32_t get_sport_cur_session_id(void);

int is_sport_start(u32_t status);
u32_t data_device_restart_type_get(void);

int is_continuous_body_status(BodyStatusRecord_Type type);



/**
 * @brief   data_device_reset_type_get
 *
 * @param   None
 *
 * @return  data_device_reset_type
 */
u32_t data_device_reset_type_get(void);

void raw_data_upload(void);
ry_sts_t raw_data_upload_start(void );

void set_ride_point(float distance);
void set_indoor_run_point(float distance);
void set_outdoor_walk_point(float distance);
void set_indoor_ride_point(float distance);
void set_swimming_point(float distance);
void set_free_point(u32_t duration_seconds);


#endif

