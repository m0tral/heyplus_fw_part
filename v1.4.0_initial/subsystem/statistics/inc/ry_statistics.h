#ifndef __RY_STATISTICS_H__
#define __RY_STATISTICS_H__

/*********************************************************************
 * INCLUDES
 */
#include "ry_type.h"
#include "DevStatistics.pb.h"
#include "DataUpload.pb.h"


#define RY_STATISTICS_MAX_COUNT         10

#define RY_STATISTICS_FILE_NAME         "statistics.data"

#define RY_STATISTICS_UPLOAD_TIME_THRESHOLD     (8*60*60)   //second

typedef struct ry_DevStatisticsResult {
    int16_t charge_begin_count;
    int16_t charge_end_count;
    int16_t home_count;//todo:
    int16_t raise_wake_count;
    int16_t msg_count;
    int16_t msg_click_count;
    int16_t drop_count;
    int16_t menu_count;
    int16_t home_back_count;
    int16_t traffic_card_count;
    int16_t door_card_count;
    int16_t card_bag_count;
    int16_t card_change_count;
    int16_t card_change_fail_count;
    int16_t sport_count;
    int16_t sport_rasie_count;
    int16_t sport_click_count;
    int16_t sport_out_door_run;
    int16_t sport_pause;
    int16_t sport_stop;
    int16_t sport_finish;
    int16_t sport_back;
    int16_t sport_short_time;
    int16_t sport_short_distance;
    int16_t hrm_count;
    int16_t hrm_finish_count;
    int16_t weather_count;
    int16_t weather_change_count;
    int16_t weather_req_count;
    int16_t weather_get_info;
    int16_t alarm_count;
    int16_t alarm_info_count;
    int16_t alarm_open_count;
    int16_t alarm_close_count;
    int16_t data_info_count;
    int16_t data_slide_up;
    int16_t data_slide_down;
    int16_t mijia_count;
    int16_t mijia_use;
    int16_t mijia_change;
    int16_t setting_count;
    int16_t about_dev_count;

    int16_t user_reset_count;
    int16_t user_replace_count;
    
    int16_t last_charge_present;
    int16_t last_charge_time;
    int16_t last_charge_full_time;
    int16_t last_charge_not_full_time;
    int16_t drop_up_count;
    int16_t drop_click_count;
    int16_t drop_down_count;
    int16_t drop_clear_all_count;
    int16_t menu_up_count;
    int16_t menu_down_count;
    int16_t long_sit_count;
    int16_t screen_on_count;
    int16_t unlock_count;
    int16_t incoming_count;
    int16_t bind_req_count;
    int16_t bind_req_ok;
    int16_t bind_success;
    int16_t bt_disconnect_count;
    

    int32_t charge_full;
    int32_t target_finish_time;

    int32_t start_time;
    int32_t end_time;

    int32_t battery_level[RY_STATISTICS_MAX_COUNT];
    int32_t charge_begin_time[RY_STATISTICS_MAX_COUNT];
    int32_t charge_stop_time[RY_STATISTICS_MAX_COUNT];
    int32_t bt_disconnect_time[RY_STATISTICS_MAX_COUNT];
/* @@protoc_insertion_point(struct:DevStatisticsResult) */
} ry_DevStatisticsResult_t;


/*
 * CONSTANTS & #DEFINE
 *******************************************************************************
 */
#define DEV_STATISTICS(item)          dev_statistics.##item##++
#define DEV_STATISTICS_VALUE(item)    dev_statistics.##item
#define DEV_STATISTICS_SET_VALUE(item, val)     do{dev_statistics.##item = val;}while(0)
#define DEV_STATISTICS_SET_TIME(item, time, time_val)       do{\
                                                                    dev_statistics.##time##[dev_statistics.##item % RY_STATISTICS_MAX_COUNT] = time_val;\
                                                                    dev_statistics.##item##++;\
                                                                }while(0)




/*
 * TYPES
 *******************************************************************************
 */


/*
 * VARIABLES
 *******************************************************************************
 */
extern ry_DevStatisticsResult_t dev_statistics;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   clear all dev_statistics data to zero
 *
 * @param   None
 *
 * @return  None
 */
void dev_statistics_clear(void);
DataSet * get_statistics_dataSet(u8_t day);
int get_statistics_dataSet_num(void);
void ry_statistics_init(void);
void save_dev_statistics_data(void);




#endif  /* __RY_STATISTICS_H__ */

