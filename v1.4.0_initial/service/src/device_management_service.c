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
#include "data_management_service.h"
#include "msg_management_service.h"
#include "activity_setting.h"
#include "sensor_alg.h"
#include "algorithm.h"
#include "device.pb.h"
#include "board.h"
#include "pb.h"
#include "pb_encode.h"
#include "ble_task.h"
#include "stdio.h"
#include "Property.pb.h"

ry_device_setting_t device_setting = {0};
ry_dev_state_t device_state;

const char* const firmware_version = "RUv15.36";

const char* get_device_version(void)
{
    return firmware_version;
}

u8_t lcd_off_type = 0;
u8_t get_lcd_off_type()
{
    return device_setting.lcd_off_type;
}

void set_lcd_off_type(u8_t value)
{
    device_setting.lcd_off_type = value;
    set_device_setting();
}

u8_t get_alarm_snooze_interval()
{
    return device_setting.alarm_snooze_interval;
}

void set_alarm_snooze_interval(u8_t value)
{
    if (value >= ALARM_SNOOZE_MAX)
        value = ALARM_SNOOZE_10MIN;
    
    device_setting.alarm_snooze_interval = value;
    set_device_setting();
}


u8_t get_sleep_monitor_state()
{
    return device_setting.sleep_monitor;
}

void set_sleep_monitor_state(u8_t value)
{
    device_setting.sleep_monitor = value;
    set_device_setting();
}


u8_t get_data_monitor_state()
{
    return device_setting.data_monitor;
}

void set_data_monitor_state(u8_t value)
{
    device_setting.data_monitor = value;
    set_device_setting();
}


u8_t get_surface_unlock_type()
{
    return device_setting.surface_unlock_type;
}

void set_surface_unlock_type(u8_t value)
{
    device_setting.surface_unlock_type = value;
    
    if (value == 0)
        device_setting.lock_type = 0;
    
    set_device_setting();
}


void restart_device(void) {
	
    system_app_data_save();
    add_reset_count(MAGIC_RESTART_COUNT);
    ry_system_reset();
}

uint32_t get_tp_version(void)
{
    return device_setting.tp_version;
}

void dev_mfg_state_set(dev_mfg_state_t state)
{
    device_setting.factory_test = (u8_t)state;
}


dev_mfg_state_t dev_mfg_state_get(void)
{
    return (dev_mfg_state_t)device_setting.factory_test;
}


void set_tp_version(uint32_t tp_version)
{
    device_setting.tp_version = tp_version;
    set_device_setting();
}

void device_setting_reset_to_factory(void)
{
    set_raise_to_wake_open();

    set_raise_towake_start_time(DEVICE_DEFAULT_WRIRST_TIME_START);
	set_raise_towake_end_time(DEVICE_DEFAULT_WRIRST_TIME_END);

    // set_dnd_home_vibrate_enbale(1);
    device_setting.dnd_home_vibrate_enbale = 1;
    // set_dnd_lunch_enable(1);
    device_setting.dnd_lunch_enable = 1;
    // set_dnd_wrist_enbale(1);
    device_setting.dnd_wrist_enable = 1;
    // set_dnd_time_count_max(2);    
    device_setting.dnd_time_count_max = 2; 
    set_dnd_mode(DND_MODE_TYPE_SMART);   
}

void get_device_setting(void)
{
    ry_memcpy(&device_setting, (u8_t *)DEVICE_SETTING_ADDR, sizeof(ry_device_setting_t));
    char * hardwareVersionStr = (char*)DEVICE_HARDWARE_VERSION_ADDR;

    device_setting.hardware_version = atoi(hardwareVersionStr);
    //clear_update_app_flag();

    if ((device_setting.raise_to_wake == 0xff) || (device_setting.raise_wake_flag == 0xff)){
        set_raise_to_wake_open();
    }

    if((get_dnd_has_mode_type() != DEVICE_HAS_MODE_TYPE_DONE) || (get_dnd_mode() == 0xff)){
        // set_dnd_home_vibrate_enbale(1);
        device_setting.dnd_home_vibrate_enbale = 1;
		// set_dnd_lunch_enable(1);
        device_setting.dnd_lunch_enable = 1;
		// set_dnd_wrist_enbale(1);
        device_setting.dnd_wrist_enable = 1;
		// set_dnd_time_count_max(2);    
        device_setting.dnd_time_count_max = 2; 
        set_dnd_mode(DND_MODE_TYPE_SMART);           
    }

    if (!is_factory_test_finish()){
        set_factory_test_finish(DEV_MFG_STATE_DONE);
    }

    if (get_brightness_rate() >= E_BRIGHTNESS_MAX){
        set_brightness_rate(E_BRIGHTNESS_DEFAULT);
    }
		
    if (device_setting.battery_on_surface > 1) {
        set_surface_battery_close();
    }
    
    if (device_setting.last_charge_end_time == 0 ||
        device_setting.last_charge_end_time == 0xFFFFFFFF) {
        clear_time_since_last_charge();
    }
        
    if (device_setting.weather_on_surface > 1) {
        set_surface_weather(0);
    }
    
    if (device_setting.wear_habit > 3) {
        set_wear_habit(DeviceSettingWearHabitPropVal_WearType_LEFT_HAND);
    }
    
    if (device_setting.battery_log_store_time.ui32Year == U32_MAX &&
        device_setting.battery_log_store_time.ui32Month == U32_MAX &&
        device_setting.battery_log_store_time.ui32DayOfMonth == U32_MAX)
    {
        ry_memset(&device_setting.battery_log_store_time, 0, sizeof(device_setting.battery_log_store_time));
    }
    
    if (device_setting.nextalarm_on_surface > 1) {
        set_surface_nextalarm(0);
    }

    if (device_setting.deepsleep > 1) {
        set_deepsleep(0);
    }
    
    if (device_setting.step_dist_coef == 0xFF ||
        device_setting.step_dist_coef < 5 ||
        device_setting.step_dist_coef > 20)
    {
        set_step_distance_coef(10);
    }
    
    if (device_setting.lcd_off_type >= LCD_OFF_MAX)
        set_lcd_off_type(LCD_OFF_DIMMING);

    if (device_setting.alarm_snooze_interval >= ALARM_SNOOZE_MAX)
        set_alarm_snooze_interval(ALARM_SNOOZE_10MIN);

    if (device_setting.sleep_monitor >= SLEEP_MONITOR_MAX)
        set_sleep_monitor_state(SLEEP_MONITOR_ON);

    if (device_setting.data_monitor >= DATA_MONITOR_MAX)
        set_data_monitor_state(DATA_MONITOR_ON);
    
    if (device_setting.surface_unlock_type >= SURFACE_UNLOCK_NUM_MAX)
        set_surface_unlock_type(SURFACE_UNLOCK_SWIPE);
}

void set_device_setting(void)
{
    ry_hal_flash_write(DEVICE_SETTING_ADDR, (u8_t *)&device_setting, sizeof(device_setting));
}

void device_bind_status_restore(void)
{}

void device_state_restore(void)
{
    if (is_bond()) {
        device_state.lifecycle = DEV_LIFECYCLE_BIND;
    } else if (is_factory_test_finish()) {
        device_state.lifecycle = DEV_LIFECYCLE_UNBIND;
    } else {
        device_state.lifecycle = DEV_LIFECYCLE_FACTORY;
    }
}


bool is_factory_test_finish(void)
{
    return (device_setting.factory_test == DEV_MFG_STATE_DONE) ? (TRUE) : (FALSE);
}

void set_factory_test_finish(u32_t test_status)
{
    device_setting.factory_test = test_status;
    set_device_setting();
}
void clear_factory_test_flag(void)
{
    device_setting.factory_test = 0;
    set_device_setting();
}

void set_auto_heart_rate_open(void)
{
	device_setting.heart_auto_flag = 1;
    device_setting.auto_heart_rate = 1;
    set_device_setting();
	periodic_task_set_enable(periodic_hr_task, 1);
	//LOG_DEBUG("APP open heart sensor\r\n");
}

void set_auto_heart_rate_close(void)
{
    device_setting.auto_heart_rate = 0;
    set_device_setting();
	periodic_task_set_enable(periodic_hr_task, 0);
	//LOG_DEBUG("APP close heart sensor\r\n");
}

bool is_auto_heart_rate_open(void)
{
    return (device_setting.auto_heart_rate == 1)? (true):(false);
}

u8_t get_auto_heart_flag(void)
{
	return(device_setting.heart_auto_flag);
}

u8_t get_unlock_flag(void)
{
	return(device_setting.unlock_flag);
}

u8_t get_raise_wake_flag(void)
{
	return(device_setting.raise_wake_flag);
}

u8_t get_home_vibrate_flag(void)
{
	return(device_setting.home_vibrate_flag);
}

u8_t get_card_swiping_flag(void)
{
	return(device_setting.card_default_flag);
}

u8_t get_long_sit_flag(void)
{
	return(device_setting.long_sit_flag);
}

void set_auto_heart_rate_time(u32_t time)
{
	if((time == 0) || (time == 0xffffffff)){
		device_setting.heart_rate_interval_time = 600;//10 minute
	}
	else{
		device_setting.heart_rate_interval_time = time;
	}
}

u32_t get_auto_heart_rate_time(void)
{
    return(device_setting.heart_rate_interval_time);
}

void set_raise_towake_start_time(u16_t time)
{
    device_setting.raise_towake_start_time = time;
}
void set_raise_towake_end_time(u16_t time)
{
    device_setting.raise_towake_end_time = time;
}

u16_t get_raise_towake_start_time(void)
{
    return(device_setting.raise_towake_start_time);
}

u16_t get_raise_towake_end_time(void)
{
    return(device_setting.raise_towake_end_time);
}
//end levi


void set_raise_to_wake_open(void)
{
    device_setting.raise_to_wake = 1;
	device_setting.raise_wake_flag = 1;
    set_device_setting();  
    //LOG_ERROR("set_raise_to_wake_open, raise_to_wake:%d\n", device_setting.raise_to_wake);
}

void set_raise_to_wake_close(void)
{
    device_setting.raise_to_wake = 0;
	set_raise_towake_end_time(DEVICE_DEFAULT_WRIRST_TIME_END);
	set_raise_towake_start_time(DEVICE_DEFAULT_WRIRST_TIME_START);
    set_device_setting();
    //LOG_ERROR("set_raise_to_wake_close, raise_to_wake:%d\n", device_setting.raise_to_wake);
}

bool is_raise_to_wake_open(void)
{
    //LOG_ERROR("is_raise_to_wake_open, raise_to_wake:%d\n", device_setting.raise_to_wake);
    return (device_setting.raise_to_wake == 1)? (true):(false);
}

void set_repeat_notify_times(int times)
{
    device_setting.repeat_notify = times;
    set_device_setting();    
}

void set_repeat_notify_close(void)
{
    device_setting.repeat_notify = 0;
    set_device_setting();
}

bool is_repeat_notify_open(void)
{
    return (device_setting.repeat_notify >= 1)? (true):(false);
}

void set_lock_type(uint8_t type)
{
	device_setting.unlock_flag = 1;
    device_setting.lock_type = type;
    set_device_setting();

    if (type == 0) {
        wms_window_lock_status_set(0);
    }
}

u8_t get_lock_type(void)
{
    return device_setting.lock_type;
}

void set_card_swiping(uint8_t enable)
{
    device_setting.card_swiping = enable;
    set_device_setting();
}

u8_t get_card_swiping(void)
{
    return device_setting.card_swiping;
}

void set_goal_remind_flag(u8_t data)
{
	device_setting.goal_remind_flag = 1;
}

u8_t get_goal_remind_flag(void)
{
	return(device_setting.goal_remind_flag);
}

void set_goal_remind_enable(u8_t en)
{
	set_goal_remind_flag(1);
	device_setting.goal_remind_enable = en;
	set_device_setting();
}

u8_t get_goal_remind_enable(void)
{
	return (device_setting.goal_remind_enable);
}

u8_t get_home_vibrate_enable(void)
{
    return(device_setting.home_vibrate_enable);
}
void set_home_vibrate_enable(uint8_t en)
{
	device_setting.home_vibrate_flag = 1;
	DeviceSettingHomeVibratePropVal home_Vibrate_prop;
	home_Vibrate_prop.enable = en;
	device_setting.home_vibrate_enable = en;
	//ry_motar_enable_set(en);
	set_device_setting();
}

u8_t get_card_default_enable(void)
{
	return (device_setting.card_default_enable);
}
void set_card_default_enable(u8_t en)
{
	device_setting.card_default_flag = 1;
	device_setting.card_default_enable = en;
	LOG_DEBUG("[%s] Set card enable = %d\r\n", __FUNCTION__, en);

    if (en == 0) {
        if (ry_nfc_get_state() != RY_NFC_STATE_IDLE) {
            ry_nfc_sync_poweroff();
        }
    } else {
        if (ry_nfc_get_state() == RY_NFC_STATE_IDLE) {
            card_service_sync();
        }
    }
    
	set_device_setting();
}

u8_t get_long_sit_enable(void)
{
	return(device_setting.long_sit_flag);
}
void set_long_sit_enable(uint8_t flag)
{
	device_setting.long_sit_flag = 1;
	set_device_setting();
}

void set_dnd_time_count_max(u8_t count)
{
	device_setting.dnd_time_count_max = count;
	set_device_setting();
}

u8_t get_dnd_time_count_max(void)
{
	return(device_setting.dnd_time_count_max);
}

void set_dnd_time(dnd_time_t dndTime[5])
{
	u8_t ii = 0;
	for(ii=0;ii<get_dnd_time_count_max();ii++)
	{
		device_setting.dnd_time[ii].tagFlag = dndTime[ii].tagFlag;
		device_setting.dnd_time[ii].startTime = dndTime[ii].startTime;
		device_setting.dnd_time[ii].endTime = dndTime[ii].endTime;
	}
	set_device_setting();
}

void get_dnd_time(dnd_time_t dndTime[5])
{
	//dndTime = device_setting.dnd_time;
	memcpy(dndTime,device_setting.dnd_time,sizeof(dnd_time_t)*5);
}

void set_brightness_rate(u8_t rate)
{
	// LOG_INFO("[set_brightness_rate], rate:%d\r\n", rate);
    if (rate <= E_BRIGHTNESS_LOW){
        device_setting.brightness_rate = rate;
        set_device_setting();
    }
}

u8_t get_brightness_rate(void)
{
	return (device_setting.brightness_rate);
}

void set_brightness_night(u8_t auto_en)
{
	// LOG_INFO("[set_brightness_night_rate], rate:%d\r\n", auto_en);
    device_setting.brightness_night = auto_en;
    set_device_setting();
}

u8_t get_brightness_night(void)
{
	return (device_setting.brightness_night);
}

void set_dnd_wrist_enbale(u8_t en)
{
	device_setting.dnd_wrist_enable = en;
	set_device_setting();
}

u8_t get_dnd_wrist_enable(void)
{
	return(device_setting.dnd_wrist_enable);
}

void set_dnd_lunch_enable(u8_t en)
{
	device_setting.dnd_lunch_enable = en;
	set_device_setting();
}

u8_t get_dnd_lunch_enable(void)
{
	return(device_setting.dnd_lunch_enable);
}

void set_dnd_home_vibrate_enbale(u8_t en)
{
	device_setting.dnd_home_vibrate_enbale = en;
	set_device_setting();
}

u8_t get_dnd_home_vibrate_enbale(void)
{
	return(device_setting.dnd_home_vibrate_enbale);
}

void set_dnd_has_mode_type(u8_t flag)
{
	device_setting.dnd_has_mode_type = flag;
	set_device_setting();
}

u8_t get_dnd_has_mode_type(void)
{
	return(device_setting.dnd_has_mode_type);
}

void set_dnd_mode(u8_t mode)
{
	device_setting.dnd_has_mode_type = DEVICE_HAS_MODE_TYPE_DONE;
	device_setting.dnd_mode_type = mode;
	set_device_setting();
	if(mode == DeviceSettingDoNotDisturbPropVal_DndMode_TIMING)
	{
		setting_set_dnd_status(current_time_is_dnd_status());
		uint32_t type = IPC_MSG_TYPE_SURFACE_UPDATE_STEP;
        ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, sizeof(uint32_t), (uint8_t *)&type);
	}
}

u8_t get_dnd_mode(void)
{
	//if(device_setting.dnd_mode_type == DeviceSettingDoNotDisturbPropVal_DndMode_TIMING)
	//{
    
    setting_set_dnd_status(current_time_is_dnd_status());
	//}
	return(device_setting.dnd_mode_type);
}

bool is_lock_enable(void)
{
    return (device_setting.lock_type > 0) ? (true):(false);
}

int get_repeat_notify_times(void)
{
    return device_setting.repeat_notify;
}


void update_main_surface(void)
{
    uint32_t type = IPC_MSG_TYPE_SURFACE_UPDATE_STEP;
    ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, sizeof(uint32_t), (uint8_t *)&type);    
}


void set_red_point_open(void)
{
    device_setting.red_point = 1;
    update_main_surface();
}

void set_red_point_close(void)
{
    device_setting.red_point = 0;
    update_main_surface();
}

bool is_red_point_open(void)
{
    return (device_setting.red_point == 1)? (true):(false);
}

void set_surface_battery(u8_t state)
{
    device_setting.battery_on_surface = state;
    set_device_setting();
    update_main_surface();
}

void set_surface_battery_open(void)
{
    set_surface_battery(1);
}

void set_surface_battery_close(void)
{
    set_surface_battery(0);
}

bool is_surface_battery_open(void)
{
    return (device_setting.battery_on_surface == 1)? (true):(false);
}

void set_surface_weather(u8_t state)
{
    device_setting.weather_on_surface = state;
    set_device_setting();
    update_main_surface();
}

bool is_surface_weather_active(void)
{
    return (device_setting.weather_on_surface == 1)? (true):(false);
}

void set_surface_nextalarm(u8_t state)
{
    device_setting.nextalarm_on_surface = state;
    set_device_setting();
    update_main_surface();
}

u8_t get_step_distance_coef(void)
{
    return device_setting.step_dist_coef;
}

void set_step_distance_coef(u8_t coef)
{
    device_setting.step_dist_coef = coef;
    set_device_setting();
}

bool is_surface_nextalarm_active(void)
{
    return (device_setting.nextalarm_on_surface == 1)? (true):(false);
}

void set_deepsleep(u8_t state)
{
    device_setting.deepsleep = state;
    set_device_setting();
//    update_main_surface();
}

bool is_deepsleep_active(void)
{
    return (device_setting.deepsleep == 1)? (true):(false);
}

void set_time_fmt_12hour(u8_t state)
{
    device_setting.time_fmt_12hour = state;
    set_device_setting();
    update_main_surface();
}

bool is_time_fmt_12hour_active(void)
{
    return (device_setting.time_fmt_12hour == 1)? (true):(false);
}

u8_t get_wear_habit(void)
{
    return (u8_t)device_setting.wear_habit;
}

void set_wear_habit(u8_t state)
{
    device_setting.wear_habit = state;
    // set_device_setting(); // no need here
}

u32_t get_hardwardVersion(void)
{
    return device_setting.hardware_version;
}

void set_hardwarVersion(u32_t ver)
{
    device_setting.hardware_version = ver;
    set_device_setting();
}


int clear_update_app_flag(void)
{
    int ret;
    am_bootloader_image_t boot_info = {0};

    boot_info = *((am_bootloader_image_t*)SYSTEM_STATUS_INFO_ADDR);
    ret = boot_info.ui32AppRunFlag;
    
    if(boot_info.ui32AppRunFlag > 0){
        boot_info.ui32AppRunFlag = 0;

        ry_hal_flash_write(SYSTEM_STATUS_INFO_ADDR, (u8_t *)&boot_info, sizeof(am_bootloader_image_t));
    }

    return ret;
}


void system_data_save(void)
{
    app_notify_store_all();
}


u32_t get_sit_alert_start_time(void)
{
    return IF_EMPTY_RETURN_ZERO(device_setting.sit_start_time);
}

void set_sit_alert_start_time(u32_t time)
{
    device_setting.sit_start_time = time;
    set_device_setting();
}

u32_t get_sit_alert_end_time(void)
{
    return IF_EMPTY_RETURN_ZERO(device_setting.sit_end_time);
}

void set_sit_alert_end_time(u32_t time)
{
    device_setting.sit_end_time = time;
    set_device_setting();
}


u32_t get_sit_alert_forbid_start_time(void)
{
    return IF_EMPTY_RETURN_ZERO(device_setting.forbid_start_time);
}

void set_sit_alert_forbid_start_time(u32_t time)
{
    device_setting.forbid_start_time = time;
    set_device_setting();
}


u32_t get_sit_alert_forbid_end_time(void)
{
    return IF_EMPTY_RETURN_ZERO(device_setting.forbid_end_time);
}

void set_sit_alert_forbid_end_time(u32_t time)
{
    device_setting.forbid_end_time = time;
    set_device_setting();
}

u32_t get_sleep_alert_start_time(void)
{
    return IF_EMPTY_RETURN_ZERO(device_setting.sleep_time);
}


void set_sleep_alert_start_time(u32_t time)
{
    device_setting.sleep_time = time;
    set_device_setting();
}


u32_t get_sleep_alert_flag(void)
{
    return IF_EMPTY_RETURN_ZERO(device_setting.sleep_alert_flag);
}


void set_sleep_alert_flag(u32_t flag) 
{
    device_setting.sleep_alert_flag = flag;
    set_device_setting();
}


u32_t get_target_step(void)
{
    if(((device_setting.target_step) == 0xffffffff) || ((device_setting.target_step) == 0)){
        device_setting.target_step = 10000;
    }
    return device_setting.target_step;
}

void set_target_step(u32_t step)
{
    device_setting.target_step = step;
    set_device_setting();
}

u8_t get_user_gender(void)
{
    return device_setting.user_gender;
}

void set_user_gender(u8_t gender)
{
    device_setting.user_gender = gender;
    set_device_setting();
}

user_data_birth_t * get_user_birth(void)
{
    return &(device_setting.birth);
}

void set_user_birth(u16_t year, u8_t month, u8_t day)
{
    device_setting.birth.year = year;
    device_setting.birth.month = month;
    device_setting.birth.day = day;
    set_device_setting();
}

float get_user_height(void)
{
    if((device_setting.user_height < USER_HEIGHT_MIN) || (device_setting.user_height > USER_HEIGHT_MAX)\
        || ((*(u32_t*)(&device_setting.user_height)) == 0xffffffff)){
        device_setting.user_height = 170;
    }
    return device_setting.user_height;
}

void set_user_height(float height)
{
    device_setting.user_height = height;

    if((device_setting.user_height < USER_HEIGHT_MIN) || (device_setting.user_height > USER_HEIGHT_MAX)){
        device_setting.user_height = 170;
    }
    set_device_setting();
}


float get_user_weight(void)
{
    if((device_setting.user_weight < USER_WEIGHT_MIN) || (device_setting.user_weight > USER_WEIGHT_MAX)\
        || ((*(u32_t*)(&device_setting.user_weight)) == 0xffffffff) ){
        device_setting.user_weight = 60;
    }
    return device_setting.user_weight;
}

void set_user_weight(float weight)
{
    device_setting.user_weight = weight;
    if((device_setting.user_weight < USER_WEIGHT_MIN) || (device_setting.user_weight > USER_WEIGHT_MAX)){
        device_setting.user_weight = 60;
    }
    set_device_setting();
}


dev_lifecycle_t get_device_lifecycle(void)
{
    return device_state.lifecycle;
}

void set_device_lifecycle(dev_lifecycle_t lifecycle)
{
    device_state.lifecycle = lifecycle;
}


dev_power_state_t get_device_powerstate(void)
{
    return device_state.power;
}

void set_device_powerstate(dev_power_state_t power)
{
    device_state.power = power;
}

dev_connect_state_t get_device_connection(void)
{
    return device_state.connect;
}

void set_device_connection(dev_connect_state_t connect)
{
    device_state.connect = connect;
}

dev_session_t get_device_session(void)
{
    return device_state.session;
}

void set_device_session(dev_session_t session)
{
    device_state.session = session;
    LOG_DEBUG("Enter session: %d\r\n", session);

    //debug log for data sync reset
    static dev_session_t session_last;
    if ((session_last == 4) && (session == 0)){
        log_task_stack_waterMark();
        //log_statistics_info();
    }
    session_last = session;
}

void add_reset_count(int type)
{
    if(type >= MAX_RESTART_COUNT){
        if ((type == SPI_TIMEOUT_RESTART) || (type == ASSERT_RESTART) \
            || (type == I2C_TIMEOUT_RESTART) || (type == WMS_CREAT_START)){
            device_setting.last_restart_type = type;
        }
        else{
            device_setting.last_restart_type = MAX_RESTART_COUNT;
        }
        set_device_setting();
        return ;
    }
    device_setting.dev_restart_count[type]++;
    if(type > USR_RESTART_COUNT){
        device_setting.last_restart_type = type;
    }
    set_device_setting();
}

void clear_reset_count(void)
{
    memset(device_setting.dev_restart_count, 0, sizeof(device_setting.dev_restart_count));
    set_device_setting();
}

int get_reset_count(int type)
{
    if(type >= MAX_RESTART_COUNT){
        return 0;;
    }

    if(device_setting.dev_restart_count[type] == -1){
        device_setting.dev_restart_count[type] = 0;
    }
    
    return device_setting.dev_restart_count[type];
}
int get_last_reset_type(void)
{
    /*if(device_setting.last_restart_type >= MAX_RESTART_COUNT){
        return -1;
    }else{
        return device_setting.last_restart_type;
    }*/
    return device_setting.last_restart_type;
}

// ******************* Charging info ********************************************

u32_t get_time_since_last_charge(void)
{
    return ryos_utc_now() - device_setting.last_charge_end_time;
}

void clear_time_since_last_charge(void)
{
    device_setting.last_charge_end_time = ryos_utc_now();
    clear_battery_log();
    
    set_device_setting();
}

void set_battery_log(u8_t value)
{
    if (value > 100) return;
    
    int i;
    int len = sizeof(device_setting.battery_log);
    
    for (i = 0; i < len; i++)
    {
        if (device_setting.battery_log[i] == 0xFF)
        {
            device_setting.battery_log[i] = value;
            break;
        }
    }
        
    ry_hal_rtc_get_time(&device_setting.battery_log_store_time);
    set_device_setting();
}

void clear_battery_log(void)
{
    ry_memset(device_setting.battery_log, 0xFF, sizeof(device_setting.battery_log));
}

// ******************* Charging info ********************************************

void add_file_error_count(void)
{
    device_setting.file_open_error_count++;
    set_device_setting();
}

void clear_file_error_count(void)
{
    device_setting.file_open_error_count = 0;
    set_device_setting();
}


int get_resource_version(void)
{
    u8_t * str_buf = NULL;
    FIL* fp = NULL;
    FRESULT status = FR_OK;
    u32_t read_bytes = 0;
    int file_size = 0;
    static int version = -1;
    u8_t * ptr = NULL;

    if (version != -1) {
        return version;
    }

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        goto error;
    }

    status = f_open(fp, DEV_RESOURCE_INFO_FILE, FA_READ);
    if(status != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_OPEN);
        goto error;
    }

    file_size = f_size(fp);

    if(file_size <= 0){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_READ);
        goto error;
    }

    str_buf = (u8_t *)ry_malloc(file_size + 1);

    if(str_buf == NULL){
        f_close(fp);
        goto error;
    }

    status = f_read(fp, str_buf, file_size, &read_bytes);
    if(status != FR_OK){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_FILE_READ);
        goto error;
    }
    str_buf[file_size] = 0;

    f_close(fp);

    ptr = (u8_t*)strstr((char*)str_buf, "version");

        version = 0;
    while((ptr != NULL) && (ptr != &str_buf[file_size] )){

        if(( *ptr >= '0') && ( *ptr <= '9')){
            version = version * 10 + (*ptr - '0');
        }

        ptr++;
    }

    ry_free(fp);
    ry_free(str_buf);

    return version;

error:
    ry_free(fp);
    ry_free(str_buf);
    return -1;
}



uint32_t device_get_powerOn_mode(void)
{
    return device_state.powerOn_mode;
}


void device_set_powerOn_mode(uint32_t powerOn_mode)
{
    device_state.powerOn_mode = powerOn_mode;
}


u8_t current_time_is_dnd_status(void)
{
	u8_t  count 	    = 0;
	u8_t  isDndStatus   = 0;
	u8_t  dndTimeCount  = 0;
	u16_t currentTime   = 0;
	ry_time_t getTime;
	dnd_time_t dndTime[5] = {0};
	
	tms_get_time(&getTime);
	
	currentTime = getTime.hour*60 + getTime.minute;
	
	if((device_setting.dnd_mode_type == DeviceSettingDoNotDisturbPropVal_DndMode_DISABLE) || (get_dnd_has_mode_type() != 1)){
		isDndStatus = 0;
	}
	else if(device_setting.dnd_mode_type == DeviceSettingDoNotDisturbPropVal_DndMode_TIMING)
	{
		get_dnd_time(dndTime);
		if(get_dnd_lunch_enable() != 1){
			dndTimeCount = 1;
		}else{
			dndTimeCount = 2;
		}
		for(count=0; count<dndTimeCount;count++){
			if(dndTime[count].startTime <= dndTime[count].endTime){
				if((currentTime > dndTime[count].endTime) || (currentTime < dndTime[count].startTime)){
					isDndStatus = 0;
				}else{
					isDndStatus = 1;
				}
			}else{
				if((currentTime < dndTime[count].endTime) || (currentTime > dndTime[count].startTime)){
					isDndStatus = 1;
				}else{
					isDndStatus = 0;
				}
			}
			if(isDndStatus)
				break;
		}
	}
	else if(device_setting.dnd_mode_type == DeviceSettingDoNotDisturbPropVal_DndMode_SMART)
	{
		if(alg_status_is_in_sleep() != 0)
		{
			isDndStatus = 1;
		}
	}
    
    if (setting_get_dnd_manual_status() == 1)
        isDndStatus = 1;
    
	return isDndStatus;
}


void file_lost_flag_set(u8_t val)
{
    device_setting.file_reset_flag = val;
    set_device_setting();
}

void file_lost_flag_clear(void)
{
    device_setting.file_reset_flag = (u8_t)FILE_CHECK_STATUS_CLEAR;
    set_device_setting();
}

u8_t file_lost_flag_get(void)
{
    return device_setting.file_reset_flag;
}
