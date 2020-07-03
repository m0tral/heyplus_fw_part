/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __SURFACE_MANAGEMENT_SERVICE_H__
#define __SURFACE_MANAGEMENT_SERVICE_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "scheduler.h"
#include "ry_list.h"

/*
 * CONSTANTS
 *******************************************************************************
 */

#define MAX_SURFACE_NUM                             6
#define MAX_SURFACE_NAME_LEN                        32
#define MAX_SURFACE_ITEM_NUM                        32

#define SURFACE_ID_USER_DEFAULT                     0
#define SURFACE_ID_SYSTEM_RED_ARROW                 1
#define SURFACE_ID_SYSTEM_EARTH                     2
#define SURFACE_ID_SYSTEM_GALAXY                    3
#define SURFACE_ID_SYSTEM_COLORFUL                  4
#define SURFACE_ID_SYSTEM_RED_CURVES                5
#define SURFACE_ID_SYSTEM_GREEN_CURVES              6

#define SURFACE_HEADER_JSON_TK_MAX_BUF_SIZE         384
#define SURFACE_HEADER_JSON_INFO_BUFF_SIZE          32
#define SURFACE_HEADER_BG_PIC_MAX_LEN               64



/*
 * TYPES
 *******************************************************************************
 */
typedef enum
{
    surface_item_element_data_source_none = 0,         //未找到

    surface_item_element_data_source_date_year = 0x1,
    surface_item_element_data_source_date_month,
    surface_item_element_data_source_date_week,
    surface_item_element_data_source_date_day,
    surface_item_element_data_source_time_hour,
    surface_item_element_data_source_time_minute,
    surface_item_element_data_source_time_seconds,
    surface_item_element_data_source_time_ampm,
    surface_item_element_data_source_batt_current,

    surface_item_element_data_source_step_today = 0x10,
    surface_item_element_data_source_cal_today,
    surface_item_element_data_source_hrm_last_value,
    surface_item_element_data_source_steps_target,

    surface_item_element_data_source_shortcuts_start_running = 0x20,
    surface_item_element_data_source_shortcuts_start_hrm,

    surface_item_element_data_source_todolist = 0x30,
    surface_item_element_data_source_nextalarm,
    
    surface_item_element_data_source_temp_realtime = 0x40,
    surface_item_element_data_source_temp_forecast,
    surface_item_element_data_source_weather_city,
    surface_item_element_data_source_weather_type_realtime,
    surface_item_element_data_source_weather_type_forecast,
    surface_item_element_data_source_weather_wind,
}surface_item_element_data_source_t;

typedef enum
{
    surface_item_element_symbol_type_none = 0,
    surface_item_element_symbol_type_comma = 0x100, // ',' -> 逗号
    surface_item_element_symbol_type_colon, // ':' -> 冒号
    surface_item_element_symbol_type_dot,   // '.' -> 点
}surface_item_element_symbol_type_t;

typedef enum
{
    surface_item_element_style_align_default = 0x0,

    surface_item_element_style_align_left = 0x1,          //pos控制左上角
    surface_item_element_style_align_middle,            //pos控制中心区域
    surface_item_element_style_align_right,             //pos控制右上角
}surface_item_element_style_align_t;

typedef enum
{
    surface_item_elemet_supported_font_default = 0,
    surface_item_elemet_supported_font_roboto = 1,
    surface_item_elemet_supported_font_langting_lb,
}surface_item_elemet_supported_font_t;

typedef enum
{
    surface_item_element_icon_none = 0x0,

    surface_item_element_icon_steps = 0x200,
    surface_item_element_icon_cal,
    surface_item_element_icon_hrm,
    surface_item_element_icon_step1,
}surface_item_element_icon_t;

typedef enum
{   //already saved in user data 
    surface_header_status_idle = 0,
    surface_header_status_ROM = 1,
    surface_header_status_Loading = 2,  //pic transmitting
    surface_header_status_ExFlash = 3,
    surface_header_status_Heap = 4,     //decoded from exflash
    surface_header_status_Holding = 5,  //单次传输已经中断，等待续传
}surface_header_status_t;

typedef struct
{
    int32_t source; //surface_item_element_data_source_t or surface_item_element_symbol_type_t
    struct
    {
        uint16_t x;
        uint16_t y;
    }pos;
    struct
    {
        uint16_t type;  // 0 -> reserved; 1-> font_roboto;  2 -> font_langting; 
        uint16_t size;  //16 20 32 44
        uint32_t color; //Black -> 0x000000 White->0xFFFFFF use digital not hex
    }font;
    struct
    {
        uint16_t align; //surface_item_element_style_align_t
        uint32_t color; // used for icon color
    }style; //surface_item_element_style_t
}surface_item_t;

typedef struct
{
    uint8_t bg_pic_filename[SURFACE_HEADER_BG_PIC_MAX_LEN];
    uint16_t items_count;
    surface_header_status_t status;
    surface_item_t* p_items;
}surface_header_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init surface manager service
 *
 * @param   None
 *
 * @return  None
 */
void surface_service_init(void);

/**
 * @brief   API to set application sequence
 *
 * @param   None
 *
 * @return  None
 */
void surface_set_current(u8_t* data, int len);

/**
 * @brief   API to get current surface list
 *
 * @param   None
 *
 * @return  None
 */
void surface_list_get(u8_t* data, int len);

void surface_add_start(u8_t* data, int len);
//void surface_add_data(int total, int sn, u8_t* data, int len);
void surface_add_data(RbpMsg *msg);
void surface_delete(RbpMsg *msg);


surface_header_t const* surface_get_current_header(void);

void surface_on_save_request(void);
void surface_on_exflash_set_id(uint32_t id);
void surface_on_reset(void);
void surface_on_ble_disconnected(void);


#endif  /* __SURFACE_MANAGEMENT_SERVICE_H__ */




