#ifndef __DEVICE_MANAGEMENT_SERVICE_H__

#define __DEVICE_MANAGEMENT_SERVICE_H__

#include "ryos.h"
#include "ry_utils.h"
#include "am_hal_rtc.h"
#include "Property.pb.h" 
#include "dip.h"

/*
 * TYPES
 *******************************************************************************
 */
typedef struct user_data_birth{
    u16_t year;
    u8_t month;
    u8_t day;
}user_data_birth_t;


typedef enum {
    DEV_LIFECYCLE_FACTORY,
    DEV_LIFECYCLE_UNBIND,
    DEV_LIFECYCLE_BIND,
} dev_lifecycle_t;

typedef enum {
    DEV_PWR_STATE_NORMAL,
    DEV_PWR_STATE_LOW_POWER,
    DEV_PWR_STATE_OFF,
    DEV_PWR_STATE_CHARGING,
} dev_power_state_t;


typedef enum {
    DEV_BLE_UNCONNECT,
    DEV_BLE_CONNECTED,
    DEV_BLE_LOGIN,
} dev_connect_state_t;

typedef enum {
    DEV_SESSION_IDLE,           // 0
    DEV_SESSION_RUNNING,        // 1
    DEV_SESSION_OTA,            // 2
    DEV_SESSION_CARD,           // 3
    DEV_SESSION_UPLOAD_DATA,    // 4
    DEV_SESSION_SIMITEST,       // 5
    DEV_SESSION_SURFACE_EDIT,   // 6
} dev_session_t;

typedef enum{

    DEV_RESTART_COUNT       = 0,
    USR_RESTART_COUNT       = 1,
    WDT_RESTART_COUNT       = 2,
    QRCODE_RESTART_COUNT    = 3,
    MAGIC_RESTART_COUNT     = 4,
    UPGRADE_RESTART_COUNT   = 5,
    BAT_RESTART_COUNT       = 6,
    MEM_RESTART_COUNT       = 7,
    PRETECT_RESTART_COUNT   = 8,
    FAULT_RESTART_COUNT     = 9,
    CARD_RESTART_COUNT      = 10,
    FS_RESTART_COUNT        = 11,
    FACTORY_FS_RESTART_COUNT= 12,
    FACTORY_TEST_RESTART    = 13,

    MAX_RESTART_COUNT       = 14,
    
    TOTAL_RESTART_COUNT     = 19,
    
    SPI_TIMEOUT_RESTART     = 0xA0,
    ASSERT_RESTART          = 0xA1,
    I2C_TIMEOUT_RESTART     = 0xA2,
    WMS_CREAT_START         = 0xA3,
    
}dev_restart_type_t;



typedef enum{
    ID_SPI_TIMEOUT_RESTART     = 15,
    ID_ASSERT_RESTART          = 16,
    ID_I2C_TIMEOUT_RESTART     = 17,
    ID_WMS_CREAT_START         = 18,    
}dev_restart_ex_type_t;

typedef struct {
    union {
        struct {
            u32_t se_activate          :1;
            u32_t ryeex_cert_activate  :1;
            u32_t mi_cert_activate     :1;
            u32_t ryeex_bind           :1;
            u32_t mi_bind              :1;
            u32_t reserved             :27;
        } bf;
        u32_t wordVal;
    } status; 

    u32_t device_sn;
    u32_t server_sn;

    u8_t  ryeex_uid[MAX_RYEEX_ID_LEN];
    u8_t  mi_uid[MAX_MI_ID_LEN];
    
} ry_device_bind_status_t;

typedef enum{
	DND_MODE_TIMING  = 0,       //定时勿扰
	DND_MODE_LUNCH   = 1,       //午休勿扰
	DND_MODE_LIST0   = 2,
	DND_MODE_LIST1   = 3,
	DND_MODE_LIST2   = 4,
}dnd_mode_tag_t;


typedef enum{
	DND_MODE_TYPE_DISABLE  = 0,
	DND_MODE_TYPE_SMART    = 1,
	DND_MODE_TYPE_TIMING   = 2,    
}dnd_mode_type_t;

typedef struct{
	u8_t  tagFlag;
	u16_t startTime;
	u16_t endTime;
}dnd_time_t;

typedef enum {
    E_BRIGHTNESS_HIGH = 0, 
    E_BRIGHTNESS_DEFAULT = 0,
    E_BRIGHTNESS_MIDDLE,    
    E_BRIGHTNESS_LOW, 
    E_BRIGHTNESS_NIGHT, 
    E_BRIGHTNESS_MAX = 3,
}brightness_rate_t;

#define BATTERY_LOG_MAX_DAYS                30

typedef struct _device_setting{

		u8_t auto_heart_rate;
		u8_t raise_to_wake;
		u8_t repeat_notify;
		u8_t factory_test;
		u8_t red_point;
		u8_t lock_type;    
		u8_t user_gender;
		float user_height;
		float user_weight;
		u32_t hardware_version;
		u32_t tp_version;
		u32_t sit_start_time;
		u32_t sit_end_time;
		u32_t forbid_start_time;
		u32_t forbid_end_time;
		u32_t sleep_time;
		u32_t sleep_alert_flag;
		u32_t target_step;
		user_data_birth_t birth;
		int32_t dev_restart_count[MAX_RESTART_COUNT];
		u32_t last_restart_type;

		u8_t  long_sit_flag;
		u8_t  home_vibrate_flag;
		u8_t  heart_auto_flag;
		u8_t  raise_wake_flag;
		u8_t  unlock_flag;
		u8_t  card_default_flag;
		u8_t  goal_remind_flag;
		u8_t  home_vibrate_enable;
		u8_t  card_default_enable;
		u8_t  goal_remind_enable;

		u8_t  dnd_has_mode_type;
		u8_t  dnd_mode_type;
		u8_t  dnd_wrist_enable;
		u8_t  dnd_lunch_enable;
		u8_t  dnd_home_vibrate_enbale;
		u8_t  dnd_time_count_max;
		dnd_time_t dnd_time[5];
		
		u32_t heart_rate_interval_time;
		u16_t raise_towake_start_time;
		u16_t raise_towake_end_time;

		u32_t file_open_error_count;
		u8_t  card_swiping; 
		u8_t file_reset_flag;
		u8_t brightness_rate;
		u8_t brightness_night;
		u8_t battery_on_surface;
        u32_t last_charge_end_time;
		u8_t weather_on_surface;
        u8_t wear_habit;
        u8_t battery_log[BATTERY_LOG_MAX_DAYS];
        am_hal_rtc_time_t battery_log_store_time;
        u8_t nextalarm_on_surface;
        u8_t deepsleep;
        u8_t time_fmt_12hour;
        u8_t step_dist_coef;
		u8_t lcd_off_type;          // 0 - with dimming, 1 - instantly
        u8_t alarm_snooze_interval; // 5 min, 10 min (default), 15 min
        u8_t sleep_monitor;			// 0 - off, 1 - on
		u8_t data_monitor;			// 0 - off, 1 - on
        u8_t surface_unlock_type;	// 0 - default, none, 1 - up swipe,  2 - code, default 1324
} ry_device_setting_t;


typedef struct device_states {
    dev_lifecycle_t     lifecycle;
    dev_power_state_t   power;
    dev_connect_state_t connect;
    dev_session_t       session;
    u32_t               powerOn_mode;
} ry_dev_state_t;


#define DEVICE_INTERNAL_FLASH_ADDR_MAX      0x000FFFFF //1M
#define DEVICE_SETTING_ADDR                 0x000FC8C0 // 0x000FC800
#define DEVICE_HARDWARE_VERSION_ADDR        0x00009C70

#define APP_DOWNLOAD_URL                    "https://heyplus.com/download/app"


#define FS_CHECK_RESTORE                    0
#define FS_CHECK_UPDATE                     1

#define DEV_RESOURCE_INFO_FILE              "resource_info.txt"

#define DEVICE_DEFAULT_WRIRST_TIME_START    0    
#define DEVICE_DEFAULT_WRIRST_TIME_END      (24*60)   


#define DEVICE_HAS_MODE_TYPE_DONE						1  
#define BMP_FILE_CHECK_SIZE                 30

typedef enum{
    LCD_OFF_DIMMING = 0,
    LCD_OFF_INSTANT,
	LCD_OFF_MAX
} lcd_off_type_t;

typedef enum {
    SURFACE_UNLOCK_NONE = 0,
    SURFACE_UNLOCK_SWIPE,
    SURFACE_UNLOCK_CODE,
    SURFACE_UNLOCK_NUM_MAX
} surface_unlock_type_t;

typedef enum{
    SLEEP_MONITOR_OFF = 0,
    SLEEP_MONITOR_ON,
	SLEEP_MONITOR_MAX
} sleep_monitor_state_t;

typedef enum{
    DATA_MONITOR_OFF = 0,
    DATA_MONITOR_ON,
	DATA_MONITOR_MAX
} data_monitor_state_t;

typedef enum{
    ALARM_SNOOZE_5MIN = 5,
    ALARM_SNOOZE_10MIN = 10,
	ALARM_SNOOZE_15MIN = 15,
	ALARM_SNOOZE_MAX,
} alarm_snooze_int_t;

typedef enum{
    FS_HEAD_OK = 0,
    FS_HEAD_ERROR,
    
}fs_head_status_t;

typedef enum {
    DEV_MFG_STATE_IDLE,
    DEV_MFG_STATE_PCBA,
    DEV_MFG_STATE_ACTIVATE,
    DEV_MFG_STATE_SEMI,
    DEV_MFG_STATE_FINAL,
    DEV_MFG_STATE_DONE,
} dev_mfg_state_t;


typedef enum{
    FILE_CHECK_STATUS_CLEAR = 0,
    FILE_CHECK_STATUS_RECOVERED = 1,
    FILE_CHECK_STATUS_UNUSED = 2
}file_check_status_e;


typedef enum{
    FILE_CHECK_SAVE_QUICK = 0,
    FILE_CHECK_SAVE_NORMAL,
}file_check_save_e;

#define SLEEP_ALERT_REPEAT_NEVER                (1 << (HealthSettingSleepAlertPropVal_RepeatType_NEVER ))
#define SLEEP_ALERT_REPEAT_MONDAY               (1 << (HealthSettingSleepAlertPropVal_RepeatType_MONDAY ))
#define SLEEP_ALERT_REPEAT_TUESDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_TUESDAY ))
#define SLEEP_ALERT_REPEAT_WEDNESDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_WEDNESDAY ))
#define SLEEP_ALERT_REPEAT_THURSDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_THURSDAY ))
#define SLEEP_ALERT_REPEAT_FRIDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_FRIDAY ))
#define SLEEP_ALERT_REPEAT_SATURDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_SATURDAY ))
#define SLEEP_ALERT_REPEAT_SUNDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_SUNDAY ))
#define SLEEP_ALERT_REPEAT_WORKDAY                (1 << (HealthSettingSleepAlertPropVal_RepeatType_WORKDAY ))




#define USER_HEIGHT_MAX                 242
#define USER_HEIGHT_MIN                 30

#define USER_WEIGHT_MAX                 200
#define USER_WEIGHT_MIN                 5

#define IF_EMPTY_RETURN_ZERO(x)            ((x == 0xffffffff)?(0):(x))

#define FILE_OPEN_CHECK_MAGIC           0x1001BEAF

#define FILE_OPEN_CHECK_ADDR            0xC6000

#define FILE_OPEN_CHECK_MAX_NUM         500
#define FILE_OPEN_CHECK_MIN_NUM         100

#define FILE_OPEN_CHECK_MAX_SIZE        8192

#define FILE_NAME_MAX_SIZE              35

#define FILE_NAME_PACK_SIZE             40

#define FILE_PACK_BASE_SIZE             50

#define ALL_FILE_REPAIR                 "ALL"   


#define OLED_BRIGHTNESS_HIGH_PERCENT     100
#define OLED_BRIGHTNESS_HIGH_MIDDLE      50
#define OLED_BRIGHTNESS_HIGH_LOW         30
#define OLED_BRIGHTNESS_HIGH_NIGHT       30


/*
 * FUNCTIONS
 *******************************************************************************
 */
 
const char* get_device_version(void);

void restart_device(void);

void dev_mfg_state_set(dev_mfg_state_t state);
dev_mfg_state_t dev_mfg_state_get(void);

void set_tp_version(uint32_t tp_version);

uint32_t get_tp_version(void);

void get_device_setting(void);

void set_device_setting(void);

bool is_factory_test_finish(void);

void set_factory_test_finish(u32_t test_status);

void clear_factory_test_flag(void);

void set_auto_heart_rate_open(void);

void set_auto_heart_rate_close(void);

bool is_auto_heart_rate_open(void);

void set_auto_heart_rate_time(u32_t time);

u32_t get_auto_heart_rate_time(void);

void set_raise_towake_start_time(u16_t time);

u16_t get_raise_towake_start_time(void);

void set_raise_towake_end_time(u16_t time);

u16_t get_raise_towake_end_time(void);

u8_t get_home_vibrate_flag(void);

u8_t get_raise_wake_flag(void);

u8_t get_unlock_flag(void);

u8_t get_long_sit_flag(void);

u8_t get_auto_heart_flag(void);

u8_t get_card_swiping_flag(void);

void set_raise_to_wake_open(void);

void set_raise_to_wake_close(void);

bool is_raise_to_wake_open(void);

void set_repeat_notify_times(int times);

void set_repeat_notify_close(void);

bool is_repeat_notify_open(void);

void set_red_point_open(void);

void set_red_point_close(void);

bool is_red_point_open(void);

int clear_update_app_flag(void);

u32_t get_sit_alert_start_time(void);

void set_sit_alert_start_time(u32_t time);

u32_t get_sit_alert_end_time(void);

void set_sit_alert_end_time(u32_t time);

u32_t get_sit_alert_forbid_start_time(void);

void set_sit_alert_forbid_start_time(u32_t time);

u32_t get_sit_alert_forbid_end_time(void);

void set_sit_alert_forbid_end_time(u32_t time);

u32_t get_target_step(void);

void set_target_step(u32_t step);

u8_t get_user_gender(void);

void set_user_gender(u8_t gender);

user_data_birth_t * get_user_birth(void);

void set_user_birth(u16_t year, u8_t month, u8_t day);

float get_user_height(void);

void set_user_height(float height);

float get_user_weight(void);

void set_user_weight(float weight);

dev_lifecycle_t get_device_lifecycle(void);
void set_device_lifecycle(dev_lifecycle_t lifecycle);

dev_power_state_t get_device_powerstate(void);
void set_device_powerstate(dev_power_state_t power);

dev_connect_state_t get_device_connection(void);
void set_device_connection(dev_connect_state_t connect);

dev_session_t get_device_session(void);
void set_device_session(dev_session_t session);

void device_state_restore(void);

ry_device_bind_status_t dev_bind_status_get(void);

void add_reset_count(int type);

void clear_reset_count(void);

int get_reset_count(int type);

int get_last_reset_type(void);

void add_file_error_count(void);

void clear_file_error_count(void);

int get_resource_version(void);

bool is_lock_enable(void);

/**
 * @brief   API to set wms_window_lock_status
 *
 * @param   status: the status to set, 0: disable, else: enalbe
 *
 * @return  None
 */
void set_lock_type(uint8_t type);

u8_t get_lock_type(void);

void set_hardwarVersion(u32_t ver);

void set_home_vibrate_enable(uint8_t en);

void set_card_swiping(uint8_t enable);

u8_t get_card_swiping(void);

u8_t get_home_vibrate_enable(void);

void set_card_default_enable(u8_t);

u8_t get_card_default_enable(void);

uint32_t device_get_powerOn_mode(void);

void device_set_powerOn_mode(uint32_t powerOn_mode);
u32_t get_hardwardVersion(void);

void set_dnd_time_count_max(u8_t count);

u8_t get_dnd_time_count_max(void);

void set_dnd_time(dnd_time_t dndTime[5]);

void get_dnd_time(dnd_time_t dndTime[5]);

void set_dnd_wrist_enbale(u8_t en);

u8_t get_dnd_wrist_enable(void);

void set_dnd_home_vibrate_enbale(u8_t en);

u8_t get_dnd_home_vibrate_enbale(void);

void set_dnd_mode(u8_t mode);

u8_t get_dnd_mode(void);

void set_dnd_lunch_enable(u8_t en);

u8_t get_dnd_lunch_enable(void);

void set_dnd_has_mode_type(u8_t flag);

u8_t get_dnd_has_mode_type(void);


u8_t current_time_is_dnd_status(void);

void set_goal_remind_enable(u8_t en);

u8_t get_goal_remind_enable(void);

void set_goal_remind_flag(u8_t data);

u8_t get_goal_remind_flag(void);

void set_long_sit_enable(uint8_t flag);

u32_t get_sleep_alert_start_time(void);

u32_t get_sleep_alert_flag(void);

int get_repeat_notify_times(void);

void set_sleep_alert_start_time(u32_t time);

void set_sleep_alert_flag(u32_t flag);

void add_reset_count(int type);

void file_lost_flag_clear(void);


/*
void check_all_files_recover(void);

int get_check_file_list(void);

void save_check_file_list(file_check_save_e save_flag);

void free_check_file_list(void);

void add_file_to_list(char *file_name);

void delete_file_from_list(char *file_name);

void print_check_file_list(void);

int get_default_check_file_list(void);

int get_lost_file_status(void);

void send_lost_file_list(void);

int get_check_file_numbers(void);

int get_lost_file_numbers(void);

void delete_all_unused_resource_whitelist(void);

int try_pack_lost_file_list(void);*/

void device_setting_reset_to_factory(void);

void set_brightness_rate(u8_t rate);
u8_t get_brightness_rate(void);
u8_t get_brightness_night(void);
void set_brightness_night(u8_t auto_en);

void clear_check_file_list(void);

void set_surface_battery_open(void);
void set_surface_battery_close(void);
bool is_surface_battery_open(void);

void set_surface_weather(u8_t state);
bool is_surface_weather_active(void);

void set_surface_nextalarm(u8_t state);
bool is_surface_nextalarm_active(void);

void set_deepsleep(u8_t state);
bool is_deepsleep_active(void);

void set_time_fmt_12hour(u8_t state);
bool is_time_fmt_12hour_active(void);

u8_t get_wear_habit(void);
void set_wear_habit(u8_t state);

u32_t get_time_since_last_charge(void);
void clear_time_since_last_charge(void);

void set_battery_log(u8_t value);
void clear_battery_log(void);

u8_t get_step_distance_coef(void);
void set_step_distance_coef(u8_t coef);

u8_t get_lcd_off_type();
void set_lcd_off_type(u8_t value);

u8_t get_alarm_snooze_interval();
void set_alarm_snooze_interval(u8_t value);

u8_t get_sleep_monitor_state();
void set_sleep_monitor_state(u8_t value);

u8_t get_data_monitor_state();
void set_data_monitor_state(u8_t value);

u8_t get_surface_unlock_type();
void set_surface_unlock_type(u8_t value);

#endif

