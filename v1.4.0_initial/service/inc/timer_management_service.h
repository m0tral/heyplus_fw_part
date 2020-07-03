#ifndef __TIMER_MANAGEMENT_SERVICE_H__
#define __TIMER_MANAGEMENT_SERVICE_H__

#include "AlarmClock.pb.h"
#include "ry_list.h"
#include "ry_type.h"


#define MAX_ALARM_REPEAT_TYPE_NUM             9     
#define MAX_ALARM_NUM                         10
#define MAX_ALARM_TAG_LEN                     25

#define MAX_ALARM                             10
#define MAX_PERIODIC_TASK_NUM                 5

#define BATTERY_CHECK_PERIOID                 10
#define TOUCH_RESET_PERIOID                   30
#define AUTO_HR_DETECT_PERIOID                60        // every 1 minute
#define AUTO_DATA_SAVE_PERIOID                60 * 20   // 20 mins
#define AUTO_SLEEP_HRM_WEARED_DETECT_PERIOID  60 * 20   // 600 - 10 mins
#define DND_MODE_MONITOR_PERIOID              60

#define FLASH_ADDR_ALARM_TABLE                0xFFE00
#define SOFT_WATCH_DOG_THRESH                 10
#define SOFT_WATCH_DOG_SE_WIRED_THRESH        10

#define LOG_MONITOR_STACK                     0

#define ALARM_STORE_MAGIC                     0x20181106
#define ALARM_STORE_VERSION                   0x0A


typedef struct {
    int weekday;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} ry_time_t;


/**
 * @brief Definitaion for alarm type
 */
typedef struct {
	//u8_t ver;
    int  id;
    u8_t hour;
    u8_t minute;
    char tag[25];
    bool enable;
	bool snoozeMode;
	u8_t ringTimes;
	u8_t ringFlag;
    u8_t repeatType[MAX_ALARM_REPEAT_TYPE_NUM];
} tms_alarm_t;

typedef struct {
    int  id;
    u8_t hour;
    u8_t minute;
    char tag[25];
    bool enable;
    u8_t repeatType[MAX_ALARM_REPEAT_TYPE_NUM];
} tms_alarm_old_t;

typedef struct {
    u32_t  msgType;
    u32_t  dataAddr;
} monitor_msg_t;

typedef struct {
    uint32_t       dog_alg;
    uint32_t       dog_scedule;    
    uint32_t       dog_wms;
    uint32_t       dog_gui;
    uint32_t       dog_hrm;
    uint32_t       dog_ble_tx;
    uint32_t       dog_ble_rx;
    uint32_t       dog_nfc;
    uint32_t       dog_cms;
    uint32_t       dog_se_wired;
    uint32_t       dog_r_xfer;
    uint32_t       free_heap;
} monitor_info_t;

/**
 * @brief Definitaion for general periodic task handler
 */
typedef void (*periodic_taskHandler_t)(void* usrdata);


typedef enum {
    BATT_LOW_PERCENT_0,
    BATT_LOW_PERCENT_5,
    BATT_LOW_PERCENT_10,
    BATT_LOW_PERCENT_15,
    BATT_LOW_PERCENT_20,  
    BATT_LOW_PERCENT_NORMAL,  
    BATT_LOW_PERCENT_NUM,    
}battery_low_level_t;

typedef struct {
    periodic_taskHandler_t  handler;
    int                     interval;
    void*                   usrdata;
    int                     lastTick;
    u8_t                    enable;
} periodic_task_t;


/*
 * @brief Periodic Task Table
 */
typedef struct {
    u32_t curNum;
    periodic_task_t tasks[MAX_PERIODIC_TASK_NUM];
} periodic_task_tbl_t;



/**
 * @brief Definitaion for alarm table type
 */
typedef struct {
    u32_t curNum;
    tms_alarm_t alarms[MAX_ALARM_NUM];
} tms_alarm_tbl_t;

typedef struct {
    u32_t magic;
	u32_t version;
    tms_alarm_tbl_t alarm_tbl;
} tms_alarm_tbl_store_t;

typedef struct {
    u32_t curNum;
    tms_alarm_old_t alarms[MAX_ALARM_NUM];
} tms_alarm_tbl_old_t;

typedef struct {
    uint8_t off_level;
    uint8_t low_protect;
    uint8_t halt_en;    
} battery_info_t;


/*
 * @brief Time service control block
 */
typedef struct {
    uint32_t            	tick;
    ry_time_t          	 	systemTime;
    periodic_task_tbl_t	 	ptt;
    tms_alarm_tbl_store_t   alarmTbl;
    uint8_t             	batt_percent;
    uint8_t             	batt_percent_last_log;    
    uint8_t             	alg_enable;   
    uint8_t             	sleep_hrm_log_count;       
    uint8_t             	oled_max_brightness;
    int                 	interval_ms;
    uint32_t            	last_alarm_set_time;
    uint8_t                 disable_deepsleep;
    uint8_t                 next_alarm_weekday;
    tms_alarm_t*            next_alarm;
} tms_ctx_t;


// extern ry_queue_t*  ry_tms_monitorQ;

void set_alarm_clock(u8_t * data , u32_t len);

void delete_alarm_clock(u8_t * data , u32_t len);

void get_alarm_clock(void);

uint8_t tms_alg_enable_get(void);

uint8_t tms_oled_max_brightness_percent(void);

/**
 * @brief   API to initialize timer service
 *
 * @param   None
 *
 * @return  None
 */
void tms_init(void);

void alarm_set_enable(int id, int enable);
void alarm_list_get(AlarmClockList* pList);


void periodic_touch_task(void* arg);
void periodic_hr_task(void* arg);
void periodic_battery_task(void* arg);
void periodic_sleep_hr_weared_detect_task(void* arg);

void log_task_stack_waterMark(void);
void log_statistics_info(void);

void periodic_task_set_enable(periodic_taskHandler_t task, u8_t fEnable);
ry_sts_t periodic_task_add(int interval, periodic_taskHandler_t task, void* usrdata, bool atonce);

void get_next_alarm(tms_alarm_t** next_alarm);

/**
 * @brief   API to get battery value
 *
 * @param   None
 * 
 * @return  pointer - pointer of the battery value
 */
uint32_t sys_get_battery(void);

/**
 * @brief   API to set battery halt or normal
 *
 * @param   halt_en - 1: halt, 0: normal
 * 
 * @return  None
 */
void battery_halt_set(uint8_t halt_en);

void tme_set_interval(int ms);

/**
 * @brief   Get current time info
 *
 * @param   time - Pointer to get 
 *
 * @return  None
 */
void tms_get_time(ry_time_t* time);

ry_sts_t alarm_tbl_reset(void);

void set_tms_oled_max_brightness_percent(uint8_t rate);

void oled_brightness_update_by_time(void);

void check_time_and_store_batt_log(void);

#endif

