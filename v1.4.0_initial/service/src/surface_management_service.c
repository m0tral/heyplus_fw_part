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
#include "ry_type.h"
#include "ry_utils.h"

#include "ryos.h"
#include "app_management_service.h"
#include "ry_list.h"
#include "ry_hal_flash.h"
#include "scheduler.h"
#include "string.h"
#include "ryos_timer.h"
#include "gui.h"
#include "rbp.h"
#include "nanopb_common.h"
#include "Surface.pb.h"
#include "surface_management_service.h"
#include "device_management_service.h"
#include "window_management_service.h"
#include "data_management_service.h"
#include "jsmn.h"
#include "jsmn_api.h"
#include "ry_fs.h"
#include "ryos.h"
#include <stdio.h>

#include "ry_hal_mcu.h"
#include "device.pb.h"
#include "crc32.h"
#include "ryos_update.h"
#include "ry_gui_disp.h"
#include "ble_task.h"
#include "touch.h"
#include "activity_charge.h"
#include "activity_loading.h"

extern ry_device_bind_status_t const device_bind_status;
extern FATFS *ry_system_fs;

#ifndef DBG_JSON
#define DBG_JSON 0
#endif

/*********************************************************************
 * CONSTANTS
 */ 

#define FLASH_ADDR_SURFACE_CONTEXT                      0xFB800
#define FLASH_ADDR_SURFACE_CONTEXT_VER_2                0xFFD00

#define SURFACE_STORE_MAGIC_NUMBER                      0x20180920
#define SURFACE_STORE_VERSION                           3
#define SURFACE_STORE_SUB_VERSION                       4

#define STORED_CHECKSUM_MAX_ITEM_COUNT                  4

#define MAX_SURFACE_HDR_LEN                             2048

#define MAX_SURFACE_FILE_FULL_PATH_LEN                  SURFACE_HEADER_BG_PIC_MAX_LEN

#define SURFACE_HEADER_ITEMS_MAX_COUNTER                MAX_SURFACE_ITEM_NUM

#define SURFACE_ADD_START_TIMEOUT_MS                    (10*1000)
#define SURFACE_ADD_CONTINUE_TIMEOUT_MS                 (15*1000)
#define SURFACE_ADD_HOLD_TIMEOUT_MS                     (30*1000)

#define SURFACE_HEADER_JS_KEY_ID                        "id"
#define SURFACE_HEADER_JS_KEY_NAME                      "name"
#define SURFACE_HEADER_JS_KEY_AUTHER                    "author"
#define SURFACE_HEADER_JS_KEY_VERSION                   "version"
#define SURFACE_HEADER_JS_KEY_BG_PIC                    "bg_pic"
#define SURFACE_HEADER_JS_KEY_BG_COLOR                  "bg_color"
#define SURFACE_HEADER_JS_KEY_ITEM                      "item"

#define SURFACE_HEADER_JS_KEY_TYPE                      "type"
#define SURFACE_HEADER_JS_KEY_NAME                      "name"
#define SURFACE_HEADER_JS_KEY_POS                       "pos"
#define SURFACE_HEADER_JS_KEY_FONT                      "font"
#define SURFACE_HEADER_JS_KEY_STYLE                     "style"

#define SURFACE_DEFAULT_USER_BG_PIC_NAME                "s_user_id0.bmp"

#if DBG_JSON
char const* pp_default_surface[] = {
"{\"id\":0,\"model\":\"ryeex.band.sake.v1\",\"name\":\"user.surface.id0\",\"author\":\"0\",\"version\":1,\"bg_pic\":\"s_user_id0.bmp\",\"item\":[{\"type\":\"data\",\"name\":\"hour\",\"pos\":{\"x\":6,\"y\":28},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"minute\",\"pos\":{\"x\":6,\"y\":68},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"dayofmonth\",\"pos\":{\"x\":6,\"y\":113},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"week\",\"pos\":{\"x\":34,\"y\":112},\"font\":{\"type\":2,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"steps\",\"pos\":{\"x\":65,\"y\":171},\"font\":{\"type\":1,\"size\":20,\"color\":16777215}},{\"type\":\"data\",\"name\":\"hrm\",\"pos\":{\"x\":65,\"y\":195},\"font\":{\"type\":1,\"size\":20,\"color\":16777215}},{\"type\":\"data\",\"name\":\"cal\",\"pos\":{\"x\":65,\"y\":219},\"font\":{\"type\":1,\"size\":20,\"color\":16777215}},{\"type\":\"icon\",\"name\":\"step.ico\",\"pos\":{\"x\":46,\"y\":173},\"style\":{\"color\":16777215}},{\"type\":\"icon\",\"name\":\"hrm.ico\",\"pos\":{\"x\":46,\"y\":197},\"style\":{\"color\":16777215}},{\"type\":\"icon\",\"name\":\"cal.ico\",\"pos\":{\"x\":47,\"y\":219},\"style\":{\"color\":16777215}}]}",
"{\"id\":2,\"model\":\"ryeex.band.sake.v1\",\"name\":\"ryeex.system.earth\",\"author\":\"ryeex\",\"version\":1,\"bg_pic\":\"surface_earth.bmp\",\"item\":[{\"type\":\"data\",\"name\":\"hour\",\"pos\":{\"x\":2,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"symbol\",\"name\":\":\",\"pos\":{\"x\":50,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"minute\",\"pos\":{\"x\":62,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"month\",\"pos\":{\"x\":6,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"symbol\",\"name\":\".\",\"pos\":{\"x\":28,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"dayofmonth\",\"pos\":{\"x\":34,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"week\",\"pos\":{\"x\":72,\"y\":92},\"font\":{\"type\":2,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"steps\",\"pos\":{\"x\":52,\"y\":202},\"font\":{\"type\":1,\"size\":20,\"color\":16777215}}]}",
"{\"id\":3,\"model\":\"ryeex.band.sake.v1\",\"name\":\"ryeex.system.galaxy\",\"author\":\"ryeex\",\"version\":1,\"bg_pic\":\"surface_galaxy.bmp\",\"item\":[{\"type\":\"data\",\"name\":\"hour\",\"pos\":{\"x\":2,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"symbol\",\"name\":\":\",\"pos\":{\"x\":52,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"minute\",\"pos\":{\"x\":62,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"month\",\"pos\":{\"x\":6,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"symbol\",\"name\":\".\",\"pos\":{\"x\":28,\"y\":92},\"font\":{\"type\":1,\"size\":20,\"color\":16777215}},{\"type\":\"data\",\"name\":\"dayofmonth\",\"pos\":{\"x\":34,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"week\",\"pos\":{\"x\":72,\"y\":92},\"font\":{\"type\":2,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"steps\",\"pos\":{\"x\":60,\"y\":192},\"font\":{\"type\":1,\"size\":20,\"color\":16777215},\"style\":{\"alignment\":\"middle\"}}]}",
"{\"id\":4,\"model\":\"ryeex.band.sake.v1\",\"name\":\"ryeex.surface.colorful\",\"author\":\"ryeex\",\"version\":1,\"bg_pic\":\"surface_colorful.bmp\",\"item\":[{\"type\":\"data\",\"name\":\"hour\",\"pos\":{\"x\":36,\"y\":57},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"minute\",\"pos\":{\"x\":36,\"y\":96},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"dayofmonth\",\"pos\":{\"x\":29,\"y\":136},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"week\",\"pos\":{\"x\":55,\"y\":137},\"font\":{\"type\":2,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"steps\",\"pos\":{\"x\":68,\"y\":200},\"font\":{\"type\":1,\"size\":20,\"color\":16777215},\"style\":{\"alignment\":\"middle\"}}]}",
"{\"id\":0,\"model\":\"ryeex.band.sake.v1\",\"name\":\"user.surface.id0\",\"author\":\"0\",\"version\":1,\"bg_pic\":\"s_user_id0.bmp\",\"item\":[{\"type\":\"data\",\"name\":\"hour\",\"pos\":{\"x\":2,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"symbol\",\"name\":\":\",\"pos\":{\"x\":50,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"minute\",\"pos\":{\"x\":62,\"y\":44},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"month\",\"pos\":{\"x\":6,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"symbol\",\"name\":\".\",\"pos\":{\"x\":28,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"dayofmonth\",\"pos\":{\"x\":34,\"y\":92},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"week\",\"pos\":{\"x\":72,\"y\":92},\"font\":{\"type\":2,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"steps\",\"pos\":{\"x\":52,\"y\":202},\"font\":{\"type\":1,\"size\":20,\"color\":16777215}},{\"type\":\"icon\",\"name\":\"step1.ico\",\"pos\":{\"x\":25,\"y\":201},\"style\":{\"color\":16777215}}]}",
"{\"id\":0,\"model\":\"ryeex.band.sake.v1\",\"name\":\"user.surface.id0\",\"author\":\"0\",\"version\":1,\"bg_pic\":\"s_user_id0.bmp\",\"item\":[{\"type\":\"data\",\"name\":\"hour\",\"pos\":{\"x\":36,\"y\":57},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"minute\",\"pos\":{\"x\":36,\"y\":96},\"font\":{\"type\":1,\"size\":44,\"color\":16777215}},{\"type\":\"data\",\"name\":\"dayofmonth\",\"pos\":{\"x\":29,\"y\":136},\"font\":{\"type\":1,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"week\",\"pos\":{\"x\":55,\"y\":137},\"font\":{\"type\":2,\"size\":16,\"color\":16777215}},{\"type\":\"data\",\"name\":\"steps\",\"pos\":{\"x\":68,\"y\":200},\"font\":{\"type\":1,\"size\":20,\"color\":16777215},\"style\":{\"alignment\":\"middle\"}},{\"type\":\"icon\",\"name\":\"step.ico\",\"pos\":{\"x\":29,\"y\":202},\"style\":{\"color\":16777215}}]}",
};
#endif

/*********************************************************************
 * TYPEDEFS
 */

typedef enum
{
    surface_items_array_type_unkown = 0,
    surface_items_array_type_data,
    surface_items_array_type_symbol,
    surface_items_array_type_icon,
}surface_items_array_type_t;


#define SURFACE_HEADER_DATASOURCE_KEYWORD_MAX_LEN   16

#pragma pack(1)
typedef struct
{
    uint8_t key[SURFACE_HEADER_DATASOURCE_KEYWORD_MAX_LEN];
    uint16_t value;
}map_key_to_value_t;

typedef struct
{
    uint32_t curNum;
    map_key_to_value_t records[SURFACE_HEADER_ITEMS_MAX_COUNTER];
} map_key_to_value_table_t;

typedef struct {
    uint64_t id;
    char name[MAX_SURFACE_NAME_LEN];
    uint32_t status;//surface_header_status_t
    uint32_t resource_checksum[STORED_CHECKSUM_MAX_ITEM_COUNT];
} surface_desc_t;

typedef struct {
    uint32_t curNum;
    surface_desc_t surfaces[MAX_SURFACE_NUM];
} surface_tbl_t;
#pragma pack()



typedef struct {
    uint64_t currentId;
    surface_tbl_t surfaceTbl;
} ss_ctx_t;

typedef struct {
    uint64_t currentId;
    surface_header_t curHeader;
} ss_sel_t;

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t sub_version;
    uint32_t op_times;
    ss_ctx_t ctx;
}surface_store_t;

typedef struct
{
    SurfaceAddStartParam* p_param;
    surface_header_status_t status; //need mutex
    FIL* fp;
    int16_t last_sn;
    int16_t last_total;
    int16_t index;
    uint32_t process;
    uint32_t timerid;
    uint8_t name[MAX_SURFACE_FILE_FULL_PATH_LEN];
    uint64_t store_id;
    ryos_mutex_t* p_mutex;
}surface_add_ctrl_block_t;

typedef struct
{
    jsmntok_t tk_buf[SURFACE_HEADER_JSON_TK_MAX_BUF_SIZE];
    int tk_size;
    char const* p_data;

    jsmn_parser instance;
    jsmntok_t*  _1st;
    jsmntok_t*  _2nd;
    jsmntok_t*  _3rd;

    int length;

    char name[SURFACE_HEADER_JSON_INFO_BUFF_SIZE];
    char author[SURFACE_HEADER_JSON_INFO_BUFF_SIZE];
    char pic[SURFACE_HEADER_BG_PIC_MAX_LEN];
    char buffer[SURFACE_HEADER_JSON_INFO_BUFF_SIZE];

    uint64_t id;
    uint32_t version;
    int items_idx;
    surface_item_t items[SURFACE_HEADER_ITEMS_MAX_COUNTER];
}surface_parser_t;

surface_store_t const* p_surface_stored = (surface_store_t*)FLASH_ADDR_SURFACE_CONTEXT;

static const surface_item_t template_0_default_items[]=
{
    {
        .source = surface_item_element_data_source_time_hour,
        .pos = {.x = 6, .y=28,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_data_source_time_minute,
        .pos = {.x = 6, .y=68,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_day,
        .pos = {.x = 6, .y=113,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_week,
        .pos = {.x = 34, .y=112,},
        .font = {.type = 2,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_step_today,
        .pos = {.x = 65, .y=171,},
        .font = {.type = 1,.size=20, .color = White},
    },
    {
        .source = surface_item_element_data_source_hrm_last_value,
        .pos = {.x = 65, .y=195,},
        .font = {.type = 1,.size=20, .color = White},
    },
    {
        .source = surface_item_element_data_source_cal_today,
        .pos = {.x = 65, .y=219,},
        .font = {.type = 1,.size=20, .color = White},
    },
};

static const surface_item_t template_1_default_items[]=
{
    {
        .source = surface_item_element_data_source_time_hour,
        .pos = {.x = 2, .y=44,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_symbol_type_colon,
        .pos = {.x = 50, .y=44,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_data_source_time_minute,
        .pos = {.x = 62, .y=44,},
        .font = {.type = 1,.size=44, .color = White},
    },

    {
        .source = surface_item_element_data_source_date_month,
        .pos = {.x = 6, .y=92,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_symbol_type_dot,
        .pos = {.x = 28, .y=92,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_day,
        .pos = {.x = 34, .y=92,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_week,
        .pos = {.x = 72, .y=92,},
        .font = {.type = 2,.size=16, .color = White},
    },

    {
        .source = surface_item_element_data_source_step_today,
        .pos = {.x = 52, .y=202,},
        .font = {.type = 1,.size=20, .color = White},
    },
};

static const surface_item_t template_2_default_items[]=
{
    {
        .source = surface_item_element_data_source_time_hour,
        .pos = {.x = 36, .y=57,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_data_source_time_minute,
        .pos = {.x = 36, .y=96,},
        .font = {.type = 1,.size=44, .color = White},
    },

    {
        .source = surface_item_element_data_source_date_day,
        .pos = {.x = 29, .y=136,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_week,
        .pos = {.x = 55, .y=137,},
        .font = {.type = 2,.size=16, .color = White},
    },

    {
        .source = surface_item_element_data_source_step_today,
        .pos = {.x = 68, .y=200,},
        .font = {.type = 1,.size=20, .color = White},
        .style = {.align = surface_item_element_style_align_middle,},
    },
};

static const surface_item_t template_3_default_items[]=
{
    {
        .source = surface_item_element_data_source_time_hour,
        .pos = {.x = 2, .y=44,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_symbol_type_colon,
        .pos = {.x = 52, .y=44,},
        .font = {.type = 1,.size=44, .color = White},
    },
    {
        .source = surface_item_element_data_source_time_minute,
        .pos = {.x = 62, .y=44,},
        .font = {.type = 1,.size=44, .color = White},
    },

    {
        .source = surface_item_element_data_source_date_month,
        .pos = {.x = 6, .y=92,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_symbol_type_dot,
        .pos = {.x = 28, .y=92,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_day,
        .pos = {.x = 34, .y=92,},
        .font = {.type = 1,.size=16, .color = White},
    },
    {
        .source = surface_item_element_data_source_date_week,
        .pos = {.x = 72, .y=92,},
        .font = {.type = 2,.size=16, .color = White},
    },

    {
        .source = surface_item_element_data_source_step_today,
        .pos = {.x = 60, .y=192,},
        .font = {.type = 1,.size=20, .color = White},
        .style = {.align = surface_item_element_style_align_middle,},
    },
};

static const surface_header_t header_default_1_6[6] = 
{
    [SURFACE_ID_SYSTEM_RED_ARROW - 1] = {
        .bg_pic_filename = "surface_01_red_arron.bmp",
        .items_count = sizeof(template_0_default_items)/sizeof(surface_item_t),
        .status = surface_header_status_ROM,
        .p_items = (void*)template_0_default_items,
    },
    [SURFACE_ID_SYSTEM_EARTH - 1] = {
        .bg_pic_filename = "surface_earth.bmp",
        .items_count = sizeof(template_1_default_items)/sizeof(surface_item_t),
        .status = surface_header_status_ROM,
        .p_items = (void*)template_1_default_items,
    },
    [SURFACE_ID_SYSTEM_GALAXY - 1] = {
        .bg_pic_filename = "surface_galaxy.bmp",
        .items_count = sizeof(template_3_default_items)/sizeof(surface_item_t),
        .status = surface_header_status_ROM,
        .p_items = (void*)template_3_default_items,
    },
    [SURFACE_ID_SYSTEM_COLORFUL - 1] = {
        .bg_pic_filename = "surface_colorful.bmp",
        .items_count = sizeof(template_2_default_items)/sizeof(surface_item_t),
        .status = surface_header_status_ROM,
        .p_items = (void*)template_2_default_items,
    },
    [SURFACE_ID_SYSTEM_RED_CURVES - 1] = {
        .bg_pic_filename = "surface_redcurve.bmp",
        .items_count = sizeof(template_0_default_items)/sizeof(surface_item_t),
        .status = surface_header_status_ROM,
        .p_items = (void*)template_0_default_items,
    },
    [SURFACE_ID_SYSTEM_GREEN_CURVES - 1] = {
        .bg_pic_filename = "surface_greencurve.bmp",
        .items_count = sizeof(template_0_default_items)/sizeof(surface_item_t),
        .status = surface_header_status_ROM,
        .p_items = (void*)template_0_default_items,
    },
};


/*********************************************************************
 * LOCAL VARIABLES
 */
ss_sel_t ss_sel_v = {0};    //single select
ss_ctx_t ss_ctx_v =     ////all ctrls
{
    .currentId = 0xFFFFFFFFFFFFFFFF,
 };
surface_add_ctrl_block_t ss_add_ctrl = {0};
static int underway_id;
static bool createDirFailedFlag = false;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static ry_sts_t surface_table_add(uint64_t id, char* name, uint32_t new_status);
static surface_desc_t* surface_table_search(uint64_t id);
static void surface_manage_save(void);
static uint32_t surface_check_file_crc(char const* p_filename);
static ry_sts_t surface_get_header_from_id(uint64_t id, surface_header_t* p_header);
static ry_sts_t surface_table_delete(uint64_t id);
static ry_sts_t surface_resource_delete(uint64_t id);
//static ry_sts_t surface_resource_remove_whitelist(uint8_t const* p_resource_name);
static void surface_add_timeout(void* p_context);


// keys of "type"
static const map_key_to_value_table_t map_str2datasource_types = 
{
    .curNum = 3,
    .records = 
    {
        {"data", surface_items_array_type_data},
        {"symbol", surface_items_array_type_symbol},
        {"icon", surface_items_array_type_icon},
    },
};

// keys of "name"
static const map_key_to_value_table_t map_str2datasource_names = 
{
    .curNum = 23,
    .records = 
    {
        {"year", surface_item_element_data_source_date_year},
        {"month", surface_item_element_data_source_date_month},
        {"week", surface_item_element_data_source_date_week},
        {"dayofmonth", surface_item_element_data_source_date_day},
        {"hour", surface_item_element_data_source_time_hour},

        {"minute", surface_item_element_data_source_time_minute},
        {"second", surface_item_element_data_source_time_seconds},
        {"ampm", surface_item_element_data_source_time_ampm},
        {"batt", surface_item_element_data_source_batt_current},
        {"steps", surface_item_element_data_source_step_today},
        
        {"cal", surface_item_element_data_source_cal_today},
        {"hrm", surface_item_element_data_source_hrm_last_value},
        {"step_target", surface_item_element_data_source_steps_target},
        {"short_running", surface_item_element_data_source_shortcuts_start_running},
        {"short_hrm", surface_item_element_data_source_shortcuts_start_hrm},
        
        {"todolist", surface_item_element_data_source_todolist},
        {"nextalarm", surface_item_element_data_source_nextalarm},

        {"temp_realtime", surface_item_element_data_source_temp_realtime},
        {"temp_forecast", surface_item_element_data_source_temp_forecast},
        {"weather_city", surface_item_element_data_source_weather_city},
        {"weather_type_rt", surface_item_element_data_source_weather_type_realtime},
        {"weather_type_fc", surface_item_element_data_source_weather_type_forecast},        
        {"weather_wind", surface_item_element_data_source_weather_wind},
    },
};

// keys of "alignment"
static const map_key_to_value_table_t map_str2datasource_symbol = 
{
    .curNum = 3,
    .records = 
    {
        {",", surface_item_element_symbol_type_comma},
        {":", surface_item_element_symbol_type_colon},
        {".", surface_item_element_symbol_type_dot},
    },
};

// keys of "alignment"
static const map_key_to_value_table_t map_str2datasource_alignment = 
{
    .curNum = 3,
    .records = 
    {
        {"left", surface_item_element_style_align_left},
        {"middle", surface_item_element_style_align_middle},
        {"right", surface_item_element_style_align_right},
    },
};

// keys of "icons"
static const map_key_to_value_table_t map_str2datasource_icons = 
{
    .curNum = 4,
    .records = 
    {
        {"step.ico", surface_item_element_icon_steps},
        {"cal.ico", surface_item_element_icon_cal},
        {"hrm.ico", surface_item_element_icon_hrm},
        {"step1.ico", surface_item_element_icon_step1},
    },
};


static int32_t SearchStringKeyToValue(map_key_to_value_table_t const* p_table,
        char const * p_str, uint32_t len)
{
    void* p_result = ry_tbl_search(p_table->curNum,
        sizeof(map_key_to_value_t),
        (void*)p_table->records,
        0,
        len,
        (uint8_t*)p_str);
    if(p_result == NULL)
    {
        return 0;
    }
    else
    {
        return ((map_key_to_value_t*)p_result)->value;
    }
}

static ry_sts_t surface_parse_pos(surface_parser_t* p_parser)
{
    ry_sts_t status = RY_SUCC;
    int ret = 0;

    surface_item_t* p_item = &p_parser->items[p_parser->items_idx];
    p_parser->_3rd = jsmn_key_value(p_parser->p_data,
        p_parser->_2nd,
        SURFACE_HEADER_JS_KEY_POS);
    if(p_parser->_3rd == NULL) {
        status = RY_ERR_INVALIC_PARA;   //不允许没有坐标描述
    } else {
        ret |= jsmn_key2val_u16(p_parser->p_data,
            p_parser->_3rd,
            "x",
            &p_item->pos.x);
        ret |= jsmn_key2val_u16(p_parser->p_data,
            p_parser->_3rd,
            "y",
            &p_item->pos.y);
//        LOG_DEBUG("[surface_parse] pos: x:%d,y:%d,%d\n",\
                p_item->pos.x,\
                p_item->pos.y,\
                ret);
    }
    return status;
}

static ry_sts_t surface_parse_font(surface_parser_t* p_parser)
{
    ry_sts_t status = RY_SUCC;
    int ret = 0;

    surface_item_t* p_item = &p_parser->items[p_parser->items_idx];

    p_parser->_3rd = jsmn_key_value(p_parser->p_data,
            p_parser->_2nd,
            SURFACE_HEADER_JS_KEY_FONT);
    if(p_parser->_3rd == NULL) {
        status = RY_ERR_TABLE_NOT_FOUND;    //允许没有style描述
        p_item->font.type = 1;
        p_item->font.size = 16;
        p_item->font.color = White;
    } else {
        ret |= jsmn_key2val_u16(p_parser->p_data,
            p_parser->_3rd,
            "type",
            &p_item->font.type);
        ret |= jsmn_key2val_u16(p_parser->p_data,
            p_parser->_3rd,
            "size",
            &p_item->font.size);
        ret |= jsmn_key2val_uint(p_parser->p_data,
            p_parser->_3rd,
            "color",
            &p_item->font.color);
//        LOG_DEBUG("[surface_parse] font: type:%d,size:%d,color:%d, ret:%d\n",\
                p_item->font.type,\
                p_item->font.size,\
                p_item->font.color,\
                ret);
    }
    return status;
}

static ry_sts_t surface_parse_style(surface_parser_t* p_parser)
{
    ry_sts_t status = RY_SUCC;
    int ret = 0;

    surface_item_t* p_item = &p_parser->items[p_parser->items_idx];

    p_parser->_3rd = jsmn_key_value(p_parser->p_data,
            p_parser->_2nd,
            SURFACE_HEADER_JS_KEY_STYLE);
    if(p_parser->_3rd == NULL) {
//        status = RY_ERR_TABLE_NOT_FOUND; //允许没有style描述
        p_item->style.align = surface_item_element_style_align_default;
    } else {
        ry_memset(p_parser->buffer, 0, SURFACE_HEADER_JSON_INFO_BUFF_SIZE);
        p_parser->length = 0;
        jsmn_key2val_str(p_parser->p_data,
            p_parser->_3rd,
            "alignment",
            p_parser->buffer,
            SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
            (size_t*)&p_parser->length);
        if(p_parser->length > 0) {
            int32_t align = SearchStringKeyToValue(&map_str2datasource_alignment,
                    p_parser->buffer,
                    p_parser->length);
            p_item->style.align = align;
        } else {
//            status = RY_ERR_INVALIC_PARA;   //允许有style 没有alignment
            p_item->style.align = surface_item_element_style_align_default;
        }

        ret = jsmn_key2val_uint(p_parser->p_data,
            p_parser->_3rd,
            "color",
            &p_item->style.color);
        if(ret == STATE_NOTFOUND) {
            p_item->style.color = White;
        }
    }
    return status;
}


ry_sts_t surface_parse(const char* data, int len, surface_header_t* p_header)
{
    uint32_t status = RY_SUCC;
    uint32_t x,y;
    int ret = 0;

    surface_parser_t* p_parser = ry_malloc(sizeof(surface_parser_t));
    if(p_parser == NULL) {
        status = RY_ERR_NO_MEM;
        LOG_ERROR("[surface_manager]malloc failed!!!\n");
        goto exit;
    }

    ry_memset(p_parser, 0x0, sizeof(surface_parser_t));
    p_parser->tk_size = SURFACE_HEADER_JSON_TK_MAX_BUF_SIZE;
    p_parser->items_idx = 0;
    p_parser->p_data = data;

    jsmn_init(&p_parser->instance);

    /* Verify Json */
    ret = jsmn_parse(&p_parser->instance, p_parser->p_data, len, p_parser->tk_buf, p_parser->tk_size);
    if(ret != JSMN_SUCCESS) {
        status = RY_ERR_INVALIC_PARA;
        LOG_ERROR("[surface_manager] json parse failed: %d\n", ret);
        goto exit;
    }

    jsmn_key2val_uint64(p_parser->p_data,
        p_parser->tk_buf,
        SURFACE_HEADER_JS_KEY_ID,
        &p_parser->id);
//    LOG_DEBUG("[surface_parse] id = %lld\r\n", p_parser->id);

    jsmn_key2val_str(p_parser->p_data,
        p_parser->tk_buf,
        SURFACE_HEADER_JS_KEY_NAME,
        p_parser->name,
        SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
        (size_t*)&p_parser->length);
//    LOG_DEBUG("[surface_parse] name = %s\r\n", p_parser->name);

    jsmn_key2val_str(p_parser->p_data,
        p_parser->tk_buf,
        SURFACE_HEADER_JS_KEY_AUTHER,
        p_parser->author,
        SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
        (size_t*)&p_parser->length);
//    LOG_DEBUG("[surface_parse] author = %s\r\n", p_parser->author);

    jsmn_key2val_str(p_parser->p_data,
        p_parser->tk_buf,
        SURFACE_HEADER_JS_KEY_BG_PIC,
        p_parser->pic,
        SURFACE_HEADER_BG_PIC_MAX_LEN,
        (size_t*)&p_parser->length);
//    LOG_DEBUG("[surface_parse] pic = %s\r\n", p_parser->author);

    jsmn_key2val_uint(p_parser->p_data,
        p_parser->tk_buf,
        SURFACE_HEADER_JS_KEY_VERSION,
        &p_parser->version);
//    LOG_DEBUG("[surface_parse] version = %d\r\n", p_parser->version);

    p_parser->_1st = jsmn_key_value(p_parser->p_data, p_parser->tk_buf, SURFACE_HEADER_JS_KEY_ITEM);
    if (p_parser->_1st  == NULL) {
        status = RY_ERR_INVALIC_PARA;
        goto exit;
    }
    while (NULL != (p_parser->_2nd = jsmn_array_value(p_parser->p_data, p_parser->_1st, p_parser->items_idx))) {
        ry_memset(p_parser->buffer, 0, SURFACE_HEADER_JSON_INFO_BUFF_SIZE);
        p_parser->length = 0;
        jsmn_key2val_str(p_parser->p_data,
            p_parser->_2nd,
            SURFACE_HEADER_JS_KEY_TYPE,
            p_parser->buffer,
            SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
            (size_t*)&p_parser->length);
        if(p_parser->length > 0) {
//            LOG_DEBUG("[surface_parse] type %s\n", p_parser->buffer);
            int32_t item_type = SearchStringKeyToValue(&map_str2datasource_types, p_parser->buffer, p_parser->length);
            surface_item_t* p_item = &p_parser->items[p_parser->items_idx];

            switch(item_type) {
                case surface_items_array_type_unkown:
                    p_item->source = surface_item_element_data_source_none;
                    LOG_DEBUG("[surface_parse] err type\n");
                    break;
                case surface_items_array_type_data:
                    ry_memset(p_parser->buffer, 0, SURFACE_HEADER_JSON_INFO_BUFF_SIZE);
                    p_parser->length = 0;
                    jsmn_key2val_str(p_parser->p_data,
                        p_parser->_2nd,
                        SURFACE_HEADER_JS_KEY_NAME,
                        p_parser->buffer,
                        SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
                        (size_t*)&p_parser->length);
                    if(p_parser->length > 0) {
//                        LOG_DEBUG("[surface_parse] name %s\n", p_parser->buffer);
                        int32_t item_type = SearchStringKeyToValue(&map_str2datasource_names, p_parser->buffer, p_parser->length);
                        p_item->source = item_type;
                        status |= surface_parse_pos(p_parser);
                        status |= surface_parse_font(p_parser);
                        status |= surface_parse_style(p_parser);
                    } else {
                        LOG_DEBUG("[surface_parse] err data name\n");
                    }
                    break;
                case surface_items_array_type_symbol:
                    ry_memset(p_parser->buffer, 0, SURFACE_HEADER_JSON_INFO_BUFF_SIZE);
                    p_parser->length = 0;
                    jsmn_key2val_str(p_parser->p_data,
                        p_parser->_2nd,
                        SURFACE_HEADER_JS_KEY_NAME,
                        p_parser->buffer,
                        SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
                        (size_t*)&p_parser->length);
                    if(p_parser->length > 0) {
//                        LOG_DEBUG("[surface_parse] name %s\n", p_parser->buffer);
                        int32_t item_type = SearchStringKeyToValue(&map_str2datasource_symbol, p_parser->buffer, p_parser->length);
                        p_item->source = item_type;
                        status |= surface_parse_pos(p_parser);
                        status |= surface_parse_font(p_parser);
                        status |= surface_parse_style(p_parser);
                    } else {
                        LOG_DEBUG("[surface_parse] err symbol name\n");
                    }
                    break;
                case surface_items_array_type_icon:
                    ry_memset(p_parser->buffer, 0, SURFACE_HEADER_JSON_INFO_BUFF_SIZE);
                    p_parser->length = 0;
                    jsmn_key2val_str(p_parser->p_data,
                        p_parser->_2nd,
                        SURFACE_HEADER_JS_KEY_NAME,
                        p_parser->buffer,
                        SURFACE_HEADER_JSON_INFO_BUFF_SIZE,
                        (size_t*)&p_parser->length);
                    if(p_parser->length > 0) {
//                        LOG_DEBUG("[surface_parse] name %s\n", p_parser->buffer);
                        int32_t item_type = SearchStringKeyToValue(&map_str2datasource_icons, p_parser->buffer, p_parser->length);
                        p_item->source = item_type;
                        status |= surface_parse_pos(p_parser);
                        status |= surface_parse_style(p_parser);
                    } else {
                        LOG_DEBUG("[surface_parse] err symbol name\n");
                    }
                    break;
                default:
                    p_item->source = surface_item_element_data_source_none;
                    LOG_DEBUG("[surface_parse] err type\n");
                    break;
            }
        }

        p_parser->items_idx++;
        if(p_parser->items_idx > SURFACE_HEADER_ITEMS_MAX_COUNTER) {
            status = RY_ERR_INVALIC_PARA;
            goto exit;
        }
    }

    if(p_header != NULL) {
        ry_memcpy(p_header->bg_pic_filename, p_parser->pic, SURFACE_HEADER_BG_PIC_MAX_LEN);
        p_header->p_items = ry_malloc(sizeof(surface_item_t)*p_parser->items_idx);
        if(p_header->p_items == NULL) {
            status = RY_ERR_NO_MEM;
            goto exit;
        }
        p_header->items_count = p_parser->items_idx;
        for(int i=0;i<p_parser->items_idx;i++) {
            p_header->p_items[i] = p_parser->items[i];
        }
        p_header->status = surface_header_status_Heap;
    }

exit:
    if(p_parser != NULL) {
        ry_free(p_parser);
    }
    if(status != RY_SUCC) {
        LOG_ERROR("[surface_parse] err:%d", status);
    }
    return status;
}

static ry_sts_t surface_restore(void)
{
    surface_store_t const* p_surface_v2_stored = (surface_store_t*)FLASH_ADDR_SURFACE_CONTEXT_VER_2;
    if((p_surface_stored->magic != SURFACE_STORE_MAGIC_NUMBER)
        ||(p_surface_stored->version < SURFACE_STORE_VERSION)
        #if RY_BUILD == RY_DEBUG
        ||(p_surface_stored->sub_version != SURFACE_STORE_SUB_VERSION)
        #endif
        ) {

        if((p_surface_v2_stored->magic == SURFACE_STORE_MAGIC_NUMBER)
            &&(p_surface_v2_stored->version == 2)
            ) {
            LOG_INFO("[surface] copy from version 2\n");
            ss_ctx_v = p_surface_v2_stored->ctx;
            surface_store_t* p_store = ry_malloc(sizeof(surface_store_t));
            if(p_store == NULL) {
                LOG_ERROR("[surface] restore init failed\n");
                return RY_ERR_NO_MEM;
            }
            ry_memset(p_store, 0xFF, sizeof(surface_store_t));
            ry_hal_flash_write((uint32_t)p_surface_v2_stored, (uint8_t*)p_store, sizeof(surface_store_t));
            ry_free(p_store);
            surface_manage_save();
        } else {
            LOG_INFO("[surface] reset to default\n");
            //reset default
            ry_memset((u8_t*)&ss_ctx_v.surfaceTbl, 0xFF, sizeof(surface_tbl_t));
            ss_ctx_v.surfaceTbl.curNum = 0;

            surface_table_add(SURFACE_ID_SYSTEM_RED_ARROW,    "/surface/s_0_1.json", surface_header_status_ROM);
            surface_table_add(SURFACE_ID_SYSTEM_EARTH,        "/surface/s_0_2.json", surface_header_status_ROM);
            surface_table_add(SURFACE_ID_SYSTEM_GALAXY,       "/surface/s_0_3.json", surface_header_status_ROM);
            surface_table_add(SURFACE_ID_SYSTEM_COLORFUL,     "/surface/s_0_4.json", surface_header_status_ROM);
            surface_table_add(SURFACE_ID_SYSTEM_RED_CURVES,   "/surface/s_0_5.json" , surface_header_status_ROM);
            surface_table_add(SURFACE_ID_SYSTEM_GREEN_CURVES, "/surface/s_0_6.json", surface_header_status_ROM);

            ss_ctx_v.currentId = SURFACE_ID_SYSTEM_RED_CURVES;
        }

        surface_manage_save();
    } else {
        //load to ram
        ss_ctx_v = p_surface_stored->ctx;
    }

    return RY_SUCC;
}

static ry_sts_t surface_repair(void)
{
    LOG_DEBUG("[surface] surface_repair\n");
    int index_all = 0;
    int index_valid = 0;
    uint32_t status = RY_SUCC;
    uint32_t ret = RY_SUCC;
    surface_desc_t* p_surface = NULL;
    surface_header_t* p_header = NULL;
    FIL* fp = NULL;

    char file_name_buffer[MAX_SURFACE_FILE_FULL_PATH_LEN] = {0};
    uint32_t filename_len = 0;
    bool isInRootDir = false;
    bool isInSurfaceDir = false;

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL) {
        status = RY_ERR_NO_MEM;
        goto exit;
    }
    
    ry_memset(fp, 0, sizeof(FIL));

    p_header = ry_malloc(sizeof(surface_header_t));
    if(p_header == NULL) {
        status = RY_ERR_NO_MEM;
        goto exit;
    }
    
    ry_memset(p_header, 0, sizeof(surface_header_t));

    ret = f_mkdir("/surface");
    if((ret != FR_OK)
        &&(ret != FR_EXIST)
        ) {
        LOG_ERROR("[surface_repair] failed create folder surface:%d \n", ret);
        status = RY_ERR_NO_MEM;
        createDirFailedFlag = true;
        goto exit;
    } else {
        createDirFailedFlag = false;
    }

    for (index_all = 0; index_all < MAX_SURFACE_NUM; index_all++) {
        p_surface = &ss_ctx_v.surfaceTbl.surfaces[index_all];
        if(p_surface->id == 0xFFFFFFFFFFFFFFFF) {
            LOG_DEBUG("[surface_repair] skip index:%d\n", index_all);
            continue;
        }

        LOG_DEBUG("[surface_repair] check surface id:%lld\n", p_surface->id);

        if((p_header->p_items != NULL)
            &&(p_header->status == surface_header_status_Heap)
            ) {
            LOG_DEBUG("[surface_repair] free pre pointer\n");
            ry_free(p_header->p_items);
            p_header->p_items = NULL;
        }

        ry_memset(p_header, 0, sizeof(surface_header_t));
        ret = surface_get_header_from_id(p_surface->id, p_header);
        if(ret != RY_SUCC) {
            LOG_DEBUG("[surface_repair] fix with err get head\n");
            goto invalid_surface;
        }

        ret = f_open(fp, (char*)p_header->bg_pic_filename, FA_READ);
        if(f_size(fp) == 0) {
            ret |= RY_ERR_INVALIC_PARA;
        }
        f_close(fp);

        if(ret == RY_SUCC) {
            isInRootDir = true;
        } else {
            isInRootDir = false;
        }

        filename_len = sprintf(file_name_buffer, "/surface/%s", (char*)p_header->bg_pic_filename);
        if(filename_len >= MAX_SURFACE_FILE_FULL_PATH_LEN) {
            ret = RY_ERR_INVALIC_PARA;
            goto invalid_surface;
        }

        ret = f_open(fp, (char*)file_name_buffer, FA_READ);
        if(f_size(fp) == 0) {
            ret |= RY_ERR_INVALIC_PARA;
        }
        f_close(fp);
        if(ret == RY_SUCC) {
            isInSurfaceDir = true;
        } else {
            isInSurfaceDir = false;
        }

        if((!isInRootDir)&&(!isInSurfaceDir)) {
            LOG_DEBUG("[surface_repair] fix with err bg:%s\n", p_header->bg_pic_filename);
            goto invalid_surface;
        }

        if(isInRootDir) {   //move pic from root to surface dir
//            ret = surface_resource_remove_whitelist(p_header->bg_pic_filename);
            ret = f_rename((char*)p_header->bg_pic_filename, file_name_buffer);
            if(ret == FR_OK) {
                LOG_DEBUG("[surface_repair] moved bg pic %s\n", (char*)p_header->bg_pic_filename);
            } else if(ret == FR_EXIST) {
                f_unlink((char const*)p_header->bg_pic_filename);
            } else {
                LOG_ERROR("[surface_repair] move bg pic failed:%d\n", ret);
                goto invalid_surface;
            }
        }

        if(p_surface->status == surface_header_status_ExFlash) {
            ret |= f_open(fp, p_surface->name, FA_READ);
            if(f_size(fp) == 0) {
                ret |= RY_ERR_INVALIC_PARA;
            }
            f_close(fp);
            if(ret != RY_SUCC) {
                LOG_DEBUG("[surface_repair] fix with err header:%s\n", p_surface->name);
                goto invalid_surface;
            }

            //move those surface from root dir to /surface dir
            if(p_surface->name[0] != '/') {
                char json_old_name[MAX_SURFACE_NAME_LEN] = {0};
                char json_new_name[MAX_SURFACE_FILE_FULL_PATH_LEN] = {0};
                int len = 0;
                strcpy(json_old_name, (char const*)p_surface->name);
                len = sprintf(json_new_name, "/surface/%s", (char const*)json_old_name);
                if(len > MAX_SURFACE_NAME_LEN - 2) {
                    status = RY_ERR_INVALIC_PARA;
                    goto invalid_surface;
                }

                ret = f_rename((char const*)json_old_name, (char const*)json_new_name);
                if(ret == FR_OK) {
                } else if(ret == FR_EXIST) {
                    f_unlink((char const*)json_old_name);
                } else {
                    LOG_ERROR("[surface_repair] rename failed:%d\n", ret);
                    status = RY_ERR_OPEN;
                    goto invalid_surface;
                }
                ry_memcpy(p_surface->name, json_new_name, len);
                p_surface->name[len] = '\0';
            }

            uint32_t checksum = get_file_check_sum(file_name_buffer);
            if(checksum != p_surface->resource_checksum[1]) {
                LOG_INFO("[surface_repair] err_checksum:0x%X/0x%X on file {%s}\n",
                    checksum,
                    p_surface->resource_checksum[1],
                    file_name_buffer);
            }
        } else if(p_surface->status != surface_header_status_ROM) {
            //fix status loading
            LOG_DEBUG("[surface_repair] fix with err_status:%d\n", p_surface->status);
            goto invalid_surface;
        }

next_loop:
        LOG_DEBUG("[surface_repair] surface_repair passed index:%d\n", index_all);
        index_valid++;
        continue;

invalid_surface:
        LOG_ERROR("[surface_repair] failed, delete index:%d\n", index_all);
        surface_resource_delete(p_surface->id);
        surface_table_delete(p_surface->id);
        continue;
    }

    ss_ctx_v.surfaceTbl.curNum = index_valid;
    if(index_valid == 0) {
        surface_table_add(SURFACE_ID_SYSTEM_COLORFUL,    "s_0_4.json", surface_header_status_ROM);
    }
    if(surface_table_search(ss_ctx_v.currentId) == NULL) {
        for(int i=0;i<MAX_SURFACE_NUM;i++) {
            uint64_t id = ss_ctx_v.surfaceTbl.surfaces[i].id;
            if(id != 0xFFFFFFFFFFFFFFFF) {
                ss_ctx_v.currentId = id;
                break;
            }
        }
    }

    if(memcmp((void*)&p_surface_stored->ctx, (void*)&ss_ctx_v, sizeof(ss_ctx_t)) != 0) {
        surface_manage_save();
    }

exit:
    if(p_header  != NULL) {
        if((p_header->p_items != NULL)
            &&(p_header->status == surface_header_status_Heap)
            ) {
            LOG_DEBUG("[surface] free pre pointer\n");
            ry_free(p_header->p_items);
            p_header->p_items = NULL;
        }

        ry_free(p_header);
    }

    if(fp != NULL) {
        ry_free(fp);
    }
    return status;
}


static uint64_t surface_combind_u32_to_u64(uint32_t ryid, uint32_t id)
{
    uint64_t s_id = ryid;
    s_id *= 0x100000000;
    s_id += id;
    return s_id;
}

static surface_desc_t* surface_table_search(uint64_t id)
{
    uint8_t key[8] = {0};
    ry_memcpy(key, (uint8_t*)&id, 8);
    return ry_tbl_search(MAX_SURFACE_NUM, 
               sizeof(surface_desc_t),
               (uint8_t*)&ss_ctx_v.surfaceTbl.surfaces,
               0,
               8,
               key);
}

static ry_sts_t surface_table_add(uint64_t id, char* name, uint32_t status_new)
{
    surface_desc_t newSurface;
    ry_sts_t status;
    int nameLen = strlen(name);

    if (nameLen > MAX_SURFACE_NAME_LEN) {
        return RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALIC_PARA);
    }

    //ryos_mutex_lock(r_xfer_v.mutex);
    newSurface.id = id;
    strcpy(newSurface.name, name);
    newSurface.status = status_new;

   
    status = ry_tbl_add((uint8_t*)&ss_ctx_v.surfaceTbl, 
                 MAX_SURFACE_NUM,
                 sizeof(surface_desc_t),
                 (uint8_t*)&newSurface,
                 8,
                 0);
    
    //ryos_mutex_unlock(r_xfer_v.mutex);   

    if (status == RY_SUCC) {
        LOG_DEBUG("Surface add success.\r\n");
    } else {
        LOG_WARN("Surface add fail, error code = %x\r\n", status);
    }

    return status;
}

static ry_sts_t surface_table_delete(uint64_t id)
{
    ry_sts_t status = ry_tbl_del((uint8_t*)&ss_ctx_v.surfaceTbl,
            MAX_SURFACE_NUM,
            sizeof(surface_desc_t),
            (uint8_t*)&id,
            8,
            0);
    return status;
}

static ry_sts_t surface_resource_delete(uint64_t id)
{
    LOG_DEBUG("[surface] delete resource:%ld \n", id);
    surface_desc_t* p_surface = NULL;
    uint32_t status = RY_SUCC;

    uint32_t surface_id = (id&0xFFFFFFFF);
    uint32_t ryid = id/0x100000000;
    surface_header_t* p_header = NULL;
    char bgpic_full_name[MAX_SURFACE_FILE_FULL_PATH_LEN] = {0};

    p_header = ry_malloc(sizeof(surface_header_t));
    if(p_header == NULL) {
        status = RY_ERR_NO_MEM;
        goto exit;
    }
    ry_memset(p_header, 0, sizeof(surface_header_t));

    p_surface = surface_table_search(id);
    if(p_surface == NULL) {
        status = RY_ERR_TABLE_NOT_FOUND;
        goto exit;
    }

    status = surface_get_header_from_id(id, p_header);

    if((p_header->status == surface_header_status_ExFlash)
        ||(p_header->status == surface_header_status_Loading)
        ) {
        f_unlink(p_surface->name);
    }

    if(status != RY_SUCC) {
        LOG_DEBUG("[surface] try delete err header cannot load\n");
        status = RY_SUCC;
//        goto exit;
    } else {
        f_unlink((char*)p_header->bg_pic_filename);

        sprintf(bgpic_full_name, "/surface/%s", p_header->bg_pic_filename);
        f_unlink(bgpic_full_name);
    }

//    check_fs_header(FS_CHECK_UPDATE);

exit:
    if(p_header != NULL) {
        if((p_header->status == surface_header_status_Heap)
            &&(p_header->p_items != NULL)
            ) {
            ry_free(p_header->p_items);
        }
        ry_free(p_header);
    }
    return status;
}

static ry_sts_t surface_resource_remove_whitelist(uint8_t const* p_resource_name)
{
//    get_check_file_list();
//    delete_file_from_list((char*)p_resource_name);
//    save_check_file_list(FILE_CHECK_SAVE_NORMAL);
//    check_fs_header(FS_CHECK_UPDATE);
    return RY_SUCC;
}


static void surface_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s], error status: %x\r\n", (char*)arg, status);
    } else {
        LOG_DEBUG("[%s], succ \r\n", (char*)arg);
    }
}

static void surface_manage_save(void)
{
    surface_store_t* p_new_data = (surface_store_t*)ry_malloc(sizeof(surface_store_t));
    RY_ASSERT(p_new_data != NULL);
    *p_new_data = *p_surface_stored;
    p_new_data->magic = SURFACE_STORE_MAGIC_NUMBER;
    p_new_data->version = SURFACE_STORE_VERSION;
    p_new_data->sub_version = SURFACE_STORE_SUB_VERSION;
    p_new_data->ctx = ss_ctx_v;
    uint32_t irq_stored = ry_hal_irq_disable();
    p_new_data->op_times = p_surface_stored->op_times + 1;
    ry_hal_flash_write(FLASH_ADDR_SURFACE_CONTEXT, (uint8_t*)p_new_data, sizeof(surface_store_t));
    ry_hal_irq_restore(irq_stored);

    ry_free(p_new_data);
}


ry_sts_t surface_parse_exflash(const char* filename, surface_header_t* p_header)
{
    FIL* fp;
    uint32_t read_bytes;
    uint8_t* data;
    int status = RY_SUCC;

    data = ry_malloc(MAX_SURFACE_HDR_LEN);
    if (data == NULL) {
        LOG_DEBUG("[%s] malloc failed\n", __FUNCTION__);
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_NO_MEM);
        goto exit;
    }
    
    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_NO_MEM);
        goto exit;
    }

    status = f_open(fp, filename, FA_READ);
    if(status != FR_OK) {
        LOG_DEBUG("[%s] file open fail ---%d\n",__FUNCTION__, status);
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_OPEN);
        goto exit;
    }

    if(f_size(fp) > MAX_SURFACE_HDR_LEN) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_READ);
        goto exit;
    }

    if(f_size(fp) == 0) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_TEST_FAIL);
        goto exit;
    }

    status = f_read(fp, data, MAX_SURFACE_HDR_LEN, &read_bytes);
    if(status != FR_OK) {
        LOG_ERROR("[%s-%d]---file read failed\n", __FUNCTION__, __LINE__);
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_READ);
        goto exit;
    }

    LOG_DEBUG("[%s] %d bytes read.\n", __FUNCTION__, read_bytes);

    status = surface_parse((char const*)data, read_bytes, p_header);

exit:
    if(fp != NULL) {
        f_close(fp);
        ry_free(fp);
    }
    if(data != NULL) {
        ry_free(data);
    }
    return status;
}

static uint32_t surface_check_file_crc(char const* p_filename)
{
    uint32_t crc = 0xFFFF;  //和PB接口保持一致
    uint8_t* p_buffer = NULL;
    const uint32_t blocksize = 4096;
    uint32_t total_size = 0;
    uint32_t bytes_read = 0;
    FIL* fp = NULL;
    FRESULT res;

    p_buffer = ry_malloc(blocksize);
    if(p_buffer == NULL) {
        goto exit;
    }

    fp = ry_malloc(sizeof(FIL));
    if(fp == NULL) {
        goto exit;
    }

    res  = f_open(fp, p_filename, FA_READ);
    if(res != FR_OK) {
        goto exit;
    }

    total_size = f_size(fp);

    for(int i=0;i<total_size;i+= blocksize) {
        res = f_read(fp, p_buffer, blocksize, &bytes_read);
        if(res != FR_OK) {
            goto exit;
        }
        crc = calc_crc16_r(p_buffer, bytes_read, crc);
    }

exit:
    if(fp != NULL) {
        f_close(fp);
        ry_free(fp);
    }
    ry_free(p_buffer);
    return crc;
}

static uint32_t surface_get_current_ryid(void)
{
    uint32_t ryid = 0;

    sscanf((char const*)device_bind_status.ryeex_uid, "%d", &ryid);

    return ryid;
}


static ry_sts_t surface_get_header_from_id(uint64_t id, surface_header_t* p_header)
{
    uint32_t status = RY_SUCC;
    surface_desc_t* p_current = surface_table_search(id);

    if(p_current == NULL) {
        LOG_ERROR("[surface] cannot load surface\n");
        p_header = NULL;
        return RY_ERR_TABLE_NOT_FOUND;
    } else {
        switch(p_current->status) {
            case surface_header_status_idle:
                break;
            case surface_header_status_ROM:
            #if DBG_JSON
                if((id > 0)
                    &&(id <= 6)
                    ) {
                    char const* p_json = pp_default_surface[id - 1];
                    status = surface_parse(p_json, strlen(p_json), p_header);
                }
            #else
                p_header->status = surface_header_status_ROM;
                if((id  > 0)
                    &&(id  <= 6)
                    ) {
                    *p_header = header_default_1_6[id - 1];
                }
            #endif
                break;
            case surface_header_status_Loading:
                status = surface_parse_exflash(p_current->name, p_header);
                LOG_ERROR("[surface add] try load loading surface\n");
                status = RY_ERR_INVALID_STATE;
                break;
            case surface_header_status_ExFlash:
                status = surface_parse_exflash(p_current->name, p_header);
                break;
            case surface_header_status_Heap:
                break;
            default:break;
        }
    }

    return status;
}

surface_header_t const* surface_get_current_header(void)
{
    bool isFreshRequired = false;
    char buffer_bg_pic_full_path[MAX_SURFACE_FILE_FULL_PATH_LEN] = {0};

    if(ss_sel_v.curHeader.p_items != NULL) {
        if(ss_sel_v.currentId != ss_ctx_v.currentId) {
            if(ss_sel_v.curHeader.status == surface_header_status_Heap) {
                ry_free(ss_sel_v.curHeader.p_items);
                ss_sel_v.curHeader.p_items = NULL;
            }
            ss_sel_v.currentId = ss_ctx_v.currentId;
            isFreshRequired = true;
        }
    } else {
        ss_sel_v.currentId = ss_ctx_v.currentId;
        isFreshRequired = true;
    }

    if(isFreshRequired) {
        ry_memset(&ss_sel_v.curHeader, 0, sizeof(surface_header_t));
        surface_get_header_from_id(ss_sel_v.currentId, &ss_sel_v.curHeader);
        if((ss_sel_v.curHeader.status == surface_header_status_Heap)
            ||(ss_sel_v.curHeader.status == surface_header_status_ROM)
            ) {
            sprintf(buffer_bg_pic_full_path, "/surface/%s", ss_sel_v.curHeader.bg_pic_filename);
            strcpy((char*)ss_sel_v.curHeader.bg_pic_filename, buffer_bg_pic_full_path);
        }
    }

    return &ss_sel_v.curHeader;
}

static void surface_add_status_mutex_lock()
{
    if(ss_add_ctrl.p_mutex != NULL) {
        ryos_mutex_lock(ss_add_ctrl.p_mutex);
    }
}

static void surface_add_status_mutex_unlock()
{
    if(ss_add_ctrl.p_mutex != NULL) {
        ryos_mutex_unlock(ss_add_ctrl.p_mutex);
    }
}


static void surface_add_flow_start(void)
{
    uint8_t conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);

    set_device_session(DEV_SESSION_SURFACE_EDIT);
    ss_add_ctrl.index = 0;
    ss_add_ctrl.last_sn = 0;
    ss_add_ctrl.last_total = 0;
    ss_add_ctrl.process = 0;
    surface_add_status_mutex_lock();
    ss_add_ctrl.status = surface_header_status_Loading;
    surface_add_status_mutex_unlock();

    loading_activity_start(NULL);
    LOG_DEBUG("[surface_add] flow start \n");
}

static void surface_add_flow_update(void)
{
    loading_activity_update(NULL);

    if(ss_add_ctrl.timerid != 0) {
        ry_timer_stop(ss_add_ctrl.timerid);
        ry_timer_start(ss_add_ctrl.timerid, SURFACE_ADD_CONTINUE_TIMEOUT_MS, surface_add_timeout, 0);
    }
}

//回收所有资源
static void surface_add_flow_finished(void)
{
    if(ss_add_ctrl.fp != NULL) {
        ry_free(ss_add_ctrl.fp);
        ss_add_ctrl.fp = NULL;
    }

    if(ss_add_ctrl.p_param != NULL) {
        ry_free(ss_add_ctrl.p_param);
        ss_add_ctrl.p_param = NULL;
    }
    if(ss_add_ctrl.timerid != NULL) {
        if(ry_timer_isRunning(ss_add_ctrl.timerid)) {
            ry_timer_stop(ss_add_ctrl.timerid);
        }
        ry_timer_delete(ss_add_ctrl.timerid);
        ss_add_ctrl.timerid = 0;
    }
    if(ss_add_ctrl.p_mutex != NULL) {
        ryos_mutex_delete(ss_add_ctrl.p_mutex);
        ss_add_ctrl.p_mutex = NULL;
    }

    surface_add_status_mutex_lock();
    if(ss_add_ctrl.status == surface_header_status_Loading) {
        loading_activity_stop(NULL);
        ss_add_ctrl.status = surface_header_status_idle;
        set_device_session(DEV_SESSION_IDLE);
    } else if (ss_add_ctrl.status == surface_header_status_Holding) {
        ss_add_ctrl.status = surface_header_status_idle;
    }
    surface_add_status_mutex_unlock();

    uint8_t conn_param_type = RY_BLE_CONN_PARAM_MEDIUM;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);

    LOG_DEBUG("[surface_add] flow stopped \n");
}

//从hold状态结束      //仅在start或者连接断开的时候调用
static void surface_add_flow_abort(void)
{
    surface_desc_t* p_surface = NULL;

//    if(ss_add_ctrl.status != surface_header_status_Holding){
//        LOG_DEBUG("[surface_add] abort on status %d\n", ss_add_ctrl.status);
//        return;
//    }

    //尝试删除传输到一半的外部flash资源
    if( ((ss_add_ctrl.last_total != 0)
         ||(ss_add_ctrl.index != 0))
        &&(ss_add_ctrl.p_param != NULL)
        &&(ss_add_ctrl.p_param->resources_count == 2)
        ) {
        for(int i =0;i<ss_add_ctrl.p_param->resources_count;i++) {
            f_unlink((void*)ss_add_ctrl.p_param->resources[i].filename);
        }
    }

    if(ss_add_ctrl.store_id != 0) {
        p_surface = surface_table_search(ss_add_ctrl.store_id);
        if(p_surface != NULL) {
            surface_table_delete(ss_add_ctrl.store_id);
            surface_manage_save();
        }
    }

    LOG_DEBUG("[surface_add] flow abort \n");

    surface_add_flow_finished();
}

//暂停前流程 等待续传或者结束 回收部分资源
static void surface_add_flow_pause(void)
{
    surface_desc_t* p_surface = NULL;

    surface_add_status_mutex_lock();
    if((ss_add_ctrl.status != surface_header_status_idle)
        &&(ss_add_ctrl.status != surface_header_status_Loading)){
        return;
    }

    loading_activity_stop(NULL);
    set_device_session(DEV_SESSION_IDLE);
    ss_add_ctrl.status = surface_header_status_Holding;
    if(ss_add_ctrl.timerid != 0) {
        ry_timer_stop(ss_add_ctrl.timerid);
        ry_timer_start(ss_add_ctrl.timerid, SURFACE_ADD_HOLD_TIMEOUT_MS, surface_add_timeout, 0);
    }
    surface_add_status_mutex_unlock();

    LOG_DEBUG("[surface_add] flow paused \n");
}

void surface_add_timeout(void* p_context)
{
    surface_add_status_mutex_lock();
    uint32_t current_status = ss_add_ctrl.status;
    surface_add_status_mutex_unlock();

    if(current_status == surface_header_status_Loading) {
        LOG_INFO("[surface_add] timeout pause process [%d/%d]\n",
                ss_add_ctrl.last_sn,
                ss_add_ctrl.last_total);
        surface_add_flow_pause();
    } else if(current_status == surface_header_status_Holding) {
        surface_add_flow_abort();
        LOG_INFO("[surface_add] timeout abort process [%d/%d]\n",
                ss_add_ctrl.last_sn,
                ss_add_ctrl.last_total);
    } else {
        LOG_ERROR("[surface_add] timeout err status\n");
    }
}

void surface_add_start(u8_t* data, int len)
{
    SurfaceAddStartParam* pMsg = NULL;
    int status = RY_SUCC;
    bool isFound = FALSE;
    ry_sts_t ret;
    FRESULT res;
    uint8_t* p_tx_buffer = NULL; 
    pb_ostream_t stream_o = {0};

    pb_istream_t stream;
    uint32_t ryid = surface_get_current_ryid();

    surface_desc_t* p_surface = NULL;
    uint64_t store_id = 0;
    uint32_t ble_resp = RBP_RESP_CODE_SUCCESS;
    FIL* fp_check = NULL;

    uint32_t free_size = 0;
    uint32_t total_size = 0;
    char buffer_pic_full_name[MAX_SURFACE_FILE_FULL_PATH_LEN] = {0};
    int filepath_len = 0;

    ry_timer_parm timer_para;

    if(ss_ctx_v.surfaceTbl.curNum >= MAX_SURFACE_NUM) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_TABLE_FULL);
        ble_resp = RBP_RESP_CODE_TBL_FULL;
        goto exit;
    }

    pMsg = ry_malloc(sizeof(SurfaceAddStartParam)+10);
    if(pMsg == NULL) {
        status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_INVALID_PARA;
        goto exit;
    }

    p_tx_buffer =ry_malloc(sizeof(SurfaceAddStartParam)+10);
    if(p_tx_buffer == NULL) {
        status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_INVALID_PARA;
        goto exit;
    }

    stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    if(0 == pb_decode(&stream, SurfaceAddStartParam_fields, pMsg)) {
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        ble_resp = RBP_RESP_CODE_INVALID_PARA;
        goto exit;
    }

    if(!pMsg->has_session) {
        pMsg->session = 0;
    }

    if(ss_add_ctrl.status == surface_header_status_idle) {
        // fist start process
new_session:
        LOG_DEBUG("[surface_add] new session:%d\n", pMsg->session);
        ss_add_ctrl.p_param = ry_malloc(sizeof(SurfaceAddStartParam));
        if(ss_add_ctrl.p_param == NULL) {
            status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_NO_MEM);
            ble_resp = RBP_RESP_CODE_NO_MEM;
            goto exit;
        }
        ry_memset(ss_add_ctrl.p_param, 0, sizeof(SurfaceAddStartParam));

        ss_add_ctrl.fp = ry_malloc(sizeof(FIL));
        if(ss_add_ctrl.fp == NULL) {
            status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_NO_MEM);
            ble_resp = RBP_RESP_CODE_NO_MEM;
            goto exit;
        }

        status = ryos_mutex_create(&ss_add_ctrl.p_mutex);
        if(status != RY_SUCC) {
            ble_resp = RBP_RESP_CODE_NO_MEM;
            goto exit;
        }
    } else if(ss_add_ctrl.status == surface_header_status_Holding) {
        if(ss_add_ctrl.p_param == NULL) {
            LOG_ERROR("[surface_add] err status\n");
            status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALID_STATE);
            ble_resp = RBP_RESP_CODE_INVALID_STATE;
            goto exit;
        } else {
            if((!pMsg->has_session)
                ||(ss_add_ctrl.p_param->version < 2)
                ||(ss_add_ctrl.index != 1)
                ||(pMsg->version < 2)
                ||(pMsg->session != ss_add_ctrl.p_param->session)
                ||(pMsg->id != ss_add_ctrl.p_param->id)
                ||(pMsg->resources_count != 2)
                ||(pMsg->resources[0].crc != ss_add_ctrl.p_param->resources[0].crc)
                ||(pMsg->resources[1].crc != ss_add_ctrl.p_param->resources[1].crc)
                ) {
                LOG_INFO("[surface_add] session check failed restart\n");
                surface_add_flow_abort();
                goto new_session;
            } else {
                goto resume;
            }
        }
    } else {
        LOG_DEBUG("[surface_add]transmitting\n");
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALID_STATE);
        ble_resp = RBP_RESP_CODE_INVALID_STATE;
        goto exit;
    }

    if(pMsg->resources_count == 0) {
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        ble_resp = RBP_RESP_CODE_INVALID_PARA;
        goto exit;
    }

    if(!pMsg->has_ryid) {
        if(pMsg->id == 0) {
            pMsg->ryid = ryid;
        } else {
            pMsg->ryid = 0;
        }
    } else {
        if((pMsg->id == 0)
            &&(pMsg->ryid != ryid)
            ) {
            status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_INVALIC_PARA);
            ble_resp = RBP_RESP_CODE_INVALID_PARA;
            goto exit;
        }
    }

    store_id = surface_combind_u32_to_u64(pMsg->ryid, pMsg->id);

    p_surface = surface_table_search(store_id);
    if(p_surface != NULL) {
        LOG_ERROR("[surface_add] err status existed status %d when add %lld\n",
            p_surface->status,
            store_id);
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_TABLE_EXISTED);
        ble_resp = RBP_RESP_CODE_ALREADY_EXIST;
        goto exit;
    }

    f_getfree("",(DWORD*)&free_size,&ry_system_fs);
    free_size = free_size*EXFLASH_SECTOR_SIZE;  //bytes

    fp_check = (FIL*)ry_malloc(sizeof(FIL));
    if(fp_check == NULL) {
        LOG_ERROR("[surface_add] malloc fail\n");
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    for(int i = 0; i < pMsg->resources_count; i++) {
        SurfaceResource* p_resource = &pMsg->resources[i];
        switch(p_resource->type) {
            case surfaceResourceType_json:
                if(ss_add_ctrl.p_param->resources[0].type != 0) {
                    status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALIC_PARA);
                    ble_resp = RBP_RESP_CODE_INVALID_PARA;
                    goto exit;
                }

                LOG_DEBUG("[surface_add] json name %s\n", p_resource->filename);
                sprintf((char*)p_resource->filename, "/surface/s_%X_%X.json", pMsg->ryid, pMsg->id);
                LOG_DEBUG("[surface_add] update to name %s\n", p_resource->filename);
                strncpy((char*)ss_add_ctrl.name, (char*)p_resource->filename, MAX_SURFACE_NAME_LEN);

                ss_add_ctrl.p_param->resources[0] = *p_resource;
                break;
            case surfaceResourceType_bmp:
                if(ss_add_ctrl.p_param->resources[1].type != 0) {
                    status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALIC_PARA);
                    ble_resp = RBP_RESP_CODE_INVALID_PARA;
                    goto exit;
                }

                if(pMsg->id == 0) {
                    if(strcmp(p_resource->filename, SURFACE_DEFAULT_USER_BG_PIC_NAME) != 0) {
                        LOG_ERROR("[surface_add] not supported file name\n");
//                        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALIC_PARA);
//                        goto exit;
                    }
                }

                LOG_DEBUG("[surface_add] bmp name %s\n", p_resource->filename);
                filepath_len = sprintf(buffer_pic_full_name, "/surface/%s", p_resource->filename);
                if(filepath_len > SURFACE_HEADER_BG_PIC_MAX_LEN - 2) {
                    LOG_ERROR("[surface_add] too long bgpic name\n");
                    status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
                    goto exit;
                }

                strcpy(p_resource->filename, buffer_pic_full_name);
//                ry_memcmp(p_resource->filename, buffer_pic_full_name, filepath_len);
//                p_resource->filename[filepath_len] = '\0';

                LOG_DEBUG("[surface_add] save to path %s\n", (char*)p_resource->filename);
                ss_add_ctrl.p_param->resources[1] = *p_resource;
                break;
            default:
                LOG_DEBUG("[surface_add] unkown type:%d\n", p_resource->type);
                status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
                ble_resp = RBP_RESP_CODE_INVALID_PARA;
                goto exit;
                break;
        }

        total_size += pMsg->resources[i].length;

        res  = f_open(fp_check, p_resource->filename, FA_READ);
        f_close(fp_check);
        if(res != FR_NO_FILE) {
            f_unlink((void*)p_resource->filename);
            LOG_INFO("[surface_add] delete existing file {%s} without in record\n", p_resource->filename);
        }
    }

    if(total_size > free_size) {
        LOG_ERROR("[surface_add] fs no enough memory [%d/%d]\n", total_size, free_size);
        status = RY_SET_STS_VAL(RY_CID_FP, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    LOG_DEBUG("[surface_add] start surface_id: %d, ryid:%d\n", pMsg->id, pMsg->ryid);

    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "surfaceadd";

    ss_add_ctrl.timerid = ry_timer_create(&timer_para);
    if(ss_add_ctrl.timerid == 0) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_NO_MEM;
        goto exit;
    } else {
        ry_timer_start(ss_add_ctrl.timerid, SURFACE_ADD_START_TIMEOUT_MS, surface_add_timeout, "start");
    }
    ss_add_ctrl.p_param->id = pMsg->id;
    ss_add_ctrl.p_param->ryid = pMsg->ryid;
    ss_add_ctrl.store_id = store_id;
    ss_add_ctrl.p_param->resources_count = pMsg->resources_count;
    ss_add_ctrl.p_param->session = pMsg->has_session?pMsg->session:0;
    ss_add_ctrl.p_param->version = pMsg->version;

    pMsg->resources[0].has_process = true;
    pMsg->resources[0].process = 0;
    pMsg->resources[1].has_process = true;
    pMsg->resources[1].process = 0;

    surface_table_add(store_id, (char*)ss_add_ctrl.name, surface_header_status_Loading);
    surface_manage_save();

    surface_add_flow_start();

exit:
    if(fp_check  != NULL) {
        ry_free(fp_check);
        fp_check = NULL;
    }

    if (status == RY_SUCC) {
        stream_o = pb_ostream_from_buffer(p_tx_buffer, sizeof(SurfaceAddStartParam) + 10);
        if(!pb_encode(&stream_o, SurfaceAddStartParam_fields, pMsg)) {
            LOG_ERROR("[%s]-{%s} %s\n", __FUNCTION__, ERR_STR_ENCODE_FAIL, PB_GET_ERROR(&stream_o));
            status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
            ble_resp = RBP_RESP_CODE_INVALID_PARA;
        }
        ret = ble_send_response(CMD_DEV_SURFACE_ADD_START,
                RBP_RESP_CODE_SUCCESS, p_tx_buffer, stream_o.bytes_written, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
        LOG_DEBUG("[%s] BLE send succ\r\n", __FUNCTION__);
    } else {
        surface_add_flow_abort();   //surface add fail 不需要hold
        ret = ble_send_response(CMD_DEV_SURFACE_ADD_START, ble_resp, NULL, 0, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
        LOG_ERROR("[surface_add] {%s} err:0x%X \n", __FUNCTION__, status);
    }

    if(pMsg != NULL) {
        ry_free(pMsg);
    }

    if(p_tx_buffer != NULL) {
        ry_free(p_tx_buffer);
    }

    return;
resume:
    LOG_INFO("[surface_add] surface resume\n");

    pMsg->resources[0].has_process = true;
    pMsg->resources[1].has_process = true;
    pMsg->resources[0].process = ss_add_ctrl.p_param->resources[0].length;
    pMsg->resources[1].process = ss_add_ctrl.process;//ss_add_ctrl.last_sn*MAX_R_XFER_SIZE;
    surface_add_flow_start();
    goto exit;
}

void surface_add_data(RbpMsg *msg)
{
    uint32_t status = RY_SUCC;
    uint32_t sn = msg->message.req.sn;
    uint32_t total = msg->message.req.total;
    uint8_t* ptr = &(msg->message.req.val.bytes[0]);
    uint32_t len = msg->message.req.val.size;
    uint32_t written_bytes;
    uint32_t crc = 0;
    char const* p_filename = NULL;
    surface_desc_t* p_surface;
    uint32_t ble_resp = RBP_RESP_CODE_SUCCESS;

    FRESULT res;

    surface_add_status_mutex_lock();
    if(ss_add_ctrl.status == surface_header_status_idle) {
        surface_add_status_mutex_unlock();
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALID_STATE);
        ble_resp = RBP_RESP_CODE_INVALID_STATE;
        goto exit;
    }
    surface_add_status_mutex_unlock();

    p_filename = ss_add_ctrl.p_param->resources[ss_add_ctrl.index].filename;

    LOG_DEBUG("[surface_add] -------- total:%d,sn:%d, len:%d \n",
        total,sn,len);

    if((ss_add_ctrl.last_total != 0)
        &&(ss_add_ctrl.last_sn != 0)
        ) {
        if((sn != ss_add_ctrl.last_sn + 1)
            ||(total != ss_add_ctrl.last_total)
            ) {
            status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_INVALID_STATE);
            ble_resp = RBP_RESP_CODE_INVALID_STATE;
            goto exit;
        }
    }

    if(sn == 0) {
        //start session
        if(ss_add_ctrl.fp == NULL) {
            status = RY_SET_STS_VAL(RY_CID_FP, RY_ERR_NO_MEM);
            ble_resp = RBP_RESP_CODE_NO_MEM;
            goto exit;
        }

        res = f_open(ss_add_ctrl.fp, p_filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(res != FR_OK) {
            status = RY_SET_STS_VAL(RY_CID_FP, RY_ERR_OPEN);
            LOG_ERROR("[surface_add] err:%d open{%s}\n", res, p_filename);
            ble_resp = RBP_RESP_CODE_OPEN_FAIL;
            goto exit;
        }

        res = f_write(ss_add_ctrl.fp, ptr, len, &written_bytes);
        LOG_DEBUG("[surface add] %d--write %d\n", len, written_bytes);
        if(res != FR_OK) {
            status = RY_SET_STS_VAL(RY_CID_FP, RY_ERR_WRITE);
            ble_resp = RBP_RESP_CODE_EXE_FAIL;
            goto exit;
        }
    } else {
        res = f_write(ss_add_ctrl.fp, ptr, len, &written_bytes);
        LOG_DEBUG("[surface_add] %d--write %d\n", len, written_bytes);
        if(res != FR_OK) {
            status = RY_SET_STS_VAL(RY_CID_FP, RY_ERR_WRITE);
            ble_resp = RBP_RESP_CODE_EXE_FAIL;
            goto exit;
        }
    }

    ss_add_ctrl.process += len;
    ss_add_ctrl.last_sn = sn;
    ss_add_ctrl.last_total = total;

    LOG_DEBUG("[surface_add]pack %d-----%d\n", total, sn);

    surface_add_flow_update();

    if(sn == total - 1) {
        f_sync(ss_add_ctrl.fp);
        f_close(ss_add_ctrl.fp);

        crc = surface_check_file_crc(p_filename);
        if(crc != ss_add_ctrl.p_param->resources[ss_add_ctrl.index].crc) {
            LOG_INFO("[surface_add] ----%s crc read:0x%08X, cfg:0x%08X\n",
                    p_filename,
                    crc, 
                    ss_add_ctrl.p_param->resources[ss_add_ctrl.index].crc
                    );
        }

        ss_add_ctrl.last_sn = 0;
        ss_add_ctrl.last_total = 0;
        ss_add_ctrl.index++;
        ss_add_ctrl.process = 0;
        if(ss_add_ctrl.index >= ss_add_ctrl.p_param->resources_count) {
            p_surface = surface_table_search(ss_add_ctrl.store_id);
            if(p_surface != NULL) {
                p_surface->status = surface_header_status_ExFlash;
            }
            for(int i =0;i<ss_add_ctrl.p_param->resources_count;i++) {
                if(i < STORED_CHECKSUM_MAX_ITEM_COUNT) {
                    p_surface->resource_checksum[i] = get_file_check_sum(ss_add_ctrl.p_param->resources[i].filename);
                }
            }
            surface_manage_save();
            surface_add_flow_finished();
//            check_fs_header(FS_CHECK_UPDATE);
        }

        ble_send_response(CMD_DEV_SURFACE_ADD_DATA, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
            surface_ble_send_callback, (void*)__FUNCTION__);
    }


exit:
    if(status == RY_SUCC) {
        //do nothing
    } else {
        surface_add_flow_pause();
        ble_send_response(CMD_DEV_SURFACE_ADD_DATA, ble_resp, NULL, 0, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
        LOG_ERROR("[surface_add] {%s} err:0x%X \n", __FUNCTION__, status);
    }
}


void surface_delete(RbpMsg *msg)
{
    SurfaceDeleteParam* p_delete = NULL;
    uint32_t status = RY_SUCC;
    uint32_t ryid = surface_get_current_ryid();
    uint64_t store_id = 0;
    surface_desc_t* p_surface = NULL;
    uint32_t ble_resp = RBP_RESP_CODE_SUCCESS;

    pb_istream_t stream = pb_istream_from_buffer(msg->message.req.val.bytes,
            msg->message.req.val.size);

    p_delete = ry_malloc(sizeof(SurfaceDeleteParam));
    if(p_delete == NULL) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    if (0 == pb_decode(&stream, SurfaceDeleteParam_fields, p_delete)) {
        LOG_ERROR("[%s]-{%s} %s\n", __FUNCTION__, ERR_STR_DECODE_FAIL, PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        ble_resp = RBP_RESP_CODE_ENCODE_FAIL;
        goto exit;
    }

    if(!p_delete->has_ryid) {
        if(p_delete->id == 0) {
            p_delete->ryid = ryid;
        } else {
            p_delete->ryid = 0;
        }
    } else {
        if((p_delete->id == 0)
            &&(p_delete->ryid != ryid)
            ) {
            LOG_DEBUG("[surface_delete] --------delete zero need modify ryid--------\n");
            status = RY_SET_STS_VAL(RY_CID_R_XFER, RY_ERR_INVALIC_PARA);
            ble_resp = RBP_RESP_CODE_INVALID_PARA;
            goto exit;
        }
    }

    store_id = surface_combind_u32_to_u64(p_delete->ryid, p_delete->id);
    p_surface = surface_table_search(store_id);
    if(p_surface == NULL)  {
//        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_TABLE_NOT_FOUND);
//        ble_resp = RBP_RESP_CODE_INVALID_PARA;
//        goto exit;
    } else {
        status = surface_resource_delete(store_id);
        if((status != RY_SUCC)
            &&(status != RY_ERR_INVALID_STATE)
            ) {
            status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_WRITE);
            ble_resp = RBP_RESP_CODE_OPEN_FAIL;
            goto exit;
        }

        status = surface_table_delete(store_id);
        if(status != RY_SUCC) {
            status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_WRITE);
    //        goto exit;
        }

        if(store_id == ss_ctx_v.currentId) {
            surface_repair();
        } else {
            surface_manage_save();
        }
    }

exit:
    if(p_delete != NULL) {
        ry_free(p_delete);
    }
    if (status == RY_SUCC) {
        ble_send_response(CMD_DEV_SURFACE_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
    } else {
        ble_send_response(CMD_DEV_SURFACE_DELETE, ble_resp, NULL, 0, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
        LOG_ERROR("[surface_delete] {%s} err:0x%X \n", __FUNCTION__, status);
    }

    return;
}

/**
 * @brief   API to set current surface
 *
 * @param   None
 *
 * @return  None
 */
void surface_set_current(u8_t* data, int len)
{
    SurfaceInfo msg;
    int status;
    int i;
    bool isFound = false;
    uint64_t id = 0;

    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, SurfaceInfo_fields, &msg);

    LOG_DEBUG("[surface_set_current] surface_id: %d, selected:%d \n", msg.id, msg.is_selected);

    if(!msg.has_ryid) {
        if(msg.id != 0) {
            id = surface_combind_u32_to_u64(0, msg.id);
        } else {
            id = surface_combind_u32_to_u64(surface_get_current_ryid(), msg.id);
        }
    } else {
        id = surface_combind_u32_to_u64(msg.ryid, msg.id);
    }

    /* Check if the surface is exist */
    for (i = 0; i < MAX_SURFACE_NUM; i++) {
        if (ss_ctx_v.surfaceTbl.surfaces[i].id == id) {
            isFound = true;
            ss_ctx_v.currentId = id;
            LOG_DEBUG("[surface_set_current] Found.\r\n");
            break;
        }
    }

    if (isFound) {
        surface_get_current_header();   //speed up the load process
        rbp_send_resp(CMD_DEV_SURFACE_SET_CURRENT, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    } else {
        rbp_send_resp(CMD_DEV_SURFACE_SET_CURRENT, RBP_RESP_CODE_NOT_FOUND, NULL, 0, 1);
    }
    
    /* Update UI */
    if (isFound) {
        surface_manage_save();

        if ((wms_activity_get_cur_disableUnTouchTime() == 0) 
            || (strcmp(wms_activity_get_cur_name(), activity_charge.name) == 0)
            ){   //兼容某些activity 只允许用户手动退出
            wms_msg_send(IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE, 0, NULL);
            ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
        }
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
        wms_untouch_timer_start();

        if(ss_add_ctrl.status != surface_header_status_idle) {
            surface_add_flow_pause();
        }
    }
}


/**
 * @brief   API to get current surface list
 *
 * @param   None
 *
 * @return  None
 */
void surface_list_get(u8_t* data, int len)
{
    int index_valid = 0;
    int index_all = 0;
    size_t message_length;
    ry_sts_t status = RY_SUCC;
    uint8_t * data_buf = NULL;
    SurfaceList * result = NULL;
    surface_desc_t* p_surface;
    pb_ostream_t stream;
    uint32_t ble_resp = RBP_RESP_CODE_SUCCESS;


    result = (SurfaceList *)ry_malloc(sizeof(SurfaceList));
    if (result == NULL) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    /* Encode result */
    for (index_all = 0; index_all < MAX_SURFACE_NUM; index_all++) {
        p_surface = &ss_ctx_v.surfaceTbl.surfaces[index_all];
        if(p_surface->id == 0xFFFFFFFFFFFFFFFF) {
            continue;
        }

        if(p_surface->status == surface_header_status_Loading) {
            continue;
        }

        result->surface_infos[index_valid].id = (p_surface->id&0xFFFFFFFF);
        if (ss_ctx_v.currentId == p_surface->id) {
            result->surface_infos[index_valid].is_selected = 1;
        } else {
            result->surface_infos[index_valid].is_selected = 0;
        }
        result->surface_infos[index_valid].has_ryid = 1;
        result->surface_infos[index_valid].ryid = (p_surface->id/0x100000000); 
        index_valid++;
    }
    result->surface_infos_count = index_valid;

    data_buf = (uint8_t *)ry_malloc(sizeof(SurfaceList) + 10);
    if (data_buf == NULL) {
        status = RY_SET_STS_VAL(RY_CID_SURFACE, RY_ERR_NO_MEM);
        ble_resp = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    stream = pb_ostream_from_buffer(data_buf, sizeof(SurfaceList) + 10);

    if(!pb_encode(&stream, SurfaceList_fields, result)) {
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INIT_FAIL);
        ble_resp = RBP_RESP_CODE_DECODE_FAIL;
        goto exit;
    }

    message_length = stream.bytes_written;

exit:
    if(data_buf != NULL) {
        ry_free(data_buf);
    }
    if(result != NULL) {
        ry_free((u8_t*)result);
    }

    if(status == RY_SUCC) {
        status = ble_send_response(CMD_DEV_SURFACE_GET_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
    } else {
        LOG_ERROR("[%s], BLE send error. status = %x\r\n", __FUNCTION__, status);
        status = ble_send_response(CMD_DEV_SURFACE_GET_LIST, ble_resp, NULL, 0, 1,
                surface_ble_send_callback, (void*)__FUNCTION__);
    }
}

static uint32_t fs_check_total_size = 0;
static uint32_t fs_dir_deeps = 0;
static uint32_t fs_root_dir_file_counter = 0;
static uint32_t fs_all_file_counter = 0;

static char const*const p_pics[] =
{
    "surface_01_red_arron.bmp",
    "surface_earth.bmp",
    "surface_galaxy.bmp",
    "surface_colorful.bmp",
    "surface_redcurve.bmp",
    "surface_greencurve.bmp",
    "s_user_id0.bmp",
};

//scan files on root dir and delete all zero size .bmp file
FRESULT hotfix_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR* p_dir = ry_malloc(sizeof(DIR));
    UINT i;
    FILINFO* p_fno = ry_malloc(sizeof(FILINFO));
    fs_dir_deeps++;

    res = f_opendir(p_dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(p_dir, p_fno);                   /* Read a directory item */
            if (res != FR_OK || p_fno->fname[0] == 0) {
                break;  /* Break on error or end of dir */
            }
            if (p_fno->fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", p_fno->fname);
//                LOG_DEBUG("[surface] Directory \"%s\" \n", path);
                path[i] = 0;
            } else {                                       /* It is a file. */
//                LOG_DEBUG("[surface] found file \"%s/%s\", size:%d\n", path, p_fno->fname, p_fno->fsize);
                i = p_fno->fsize/SECTOR_SIZE;
                if((p_fno->fsize % SECTOR_SIZE) != 0) {
                    i += 1;
                }
                if(i == 0) {
                    //i = 1;
                }
                fs_check_total_size += i * SECTOR_SIZE;

                if(fs_dir_deeps == 1) {
                    fs_root_dir_file_counter++;

                    if(strstr(p_fno->fname, ".bmp")) {   // delete empty .bmp file
                        if(p_fno->fsize == 0) {
                            f_unlink(p_fno->fname);
                            fs_root_dir_file_counter--;
                            LOG_INFO("[surface] hotfix and delete file :%s\n", p_fno->fname);
                            continue;
                        }
                    }
                }
            }
        }
        f_closedir(p_dir);
    }

    ry_free(p_dir);
    ry_free(p_fno);
    fs_dir_deeps--;
    return res;
}


//scan all files iterative all dirs //will delete surface and bmp in root dir
FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR* p_dir = ry_malloc(sizeof(DIR));
    UINT i;
    FILINFO* p_fno = ry_malloc(sizeof(FILINFO));
    fs_dir_deeps++;

    res = f_opendir(p_dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(p_dir, p_fno);                   /* Read a directory item */
            if (res != FR_OK || p_fno->fname[0] == 0) {
                break;  /* Break on error or end of dir */
            }
            if (p_fno->fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", p_fno->fname);
                LOG_DEBUG("[surface] Directory Enter \"%s\" \n", path);
                res = scan_files(path);                    /* Enter the directory */
                LOG_DEBUG("[surface] Directory Exit \"%s\" \n", path);
                if (res != FR_OK) {
                    break;
                }
                path[i] = 0;
            } else {                                       /* It is a file. */
                LOG_DEBUG("[surface] found file \"%s/%s\", size:%d\n", path, p_fno->fname, p_fno->fsize);
                i = p_fno->fsize/SECTOR_SIZE;
                if((p_fno->fsize % SECTOR_SIZE) != 0) {
                    i += 1;
                }
                if(i == 0) {
                    //i = 1;
                    LOG_WARN("[surface] found file \"%s/%s\", size:%d\n", path, p_fno->fname, p_fno->fsize);
                }
                fs_check_total_size += i * SECTOR_SIZE;
                fs_all_file_counter++;

                if(fs_dir_deeps == 1) {

                    if(strstr(p_fno->fname, ".json")) {   // delete surface files
                        if(strstr(p_fno->fname, "s_")) {
                            f_unlink(p_fno->fname);
                            fs_all_file_counter--;
                            LOG_INFO("[surface] scan and delete file :%s\n", p_fno->fname);
                            continue;
                        }
                    }

                    if(!createDirFailedFlag) {
                        for(int j =0;j<sizeof(p_pics)/sizeof(uint8_t*);j++) {
                            if(strcmp(p_fno->fname, p_pics[j]) == 0) {
                                f_unlink(p_fno->fname);
                                fs_all_file_counter--;
                                LOG_INFO("[surface] scan and delete file :%s\n", p_fno->fname);
                                break;
                            }
                        }
                    }
                }
            }
        }
        f_closedir(p_dir);
    }

    ry_free(p_dir);
    ry_free(p_fno);
    fs_dir_deeps--;
    return res;
}

static void surface_check_hotfix_all_files(void)
{
    uint32_t free_size = 0;
    char buff[64];
    strcpy(buff, "/");
    fs_check_total_size = 0;
    fs_root_dir_file_counter = 0;
    f_getfree("",(DWORD*)&free_size,&ry_system_fs);
    free_size = free_size*EXFLASH_SECTOR_SIZE;  //bytes

    hotfix_files(buff);
    LOG_INFO("[surface] root dir size total:%d, free:%d files:%d\n",
            fs_check_total_size,
            free_size,
            fs_root_dir_file_counter);

}

static void surface_check_list_all_files(void)
{
    uint32_t free_size = 0;
    char buff[64];
    strcpy(buff, "/");
    fs_check_total_size = 0;
    fs_all_file_counter = 0;
    f_getfree("",(DWORD*)&free_size,&ry_system_fs);
    free_size = free_size*EXFLASH_SECTOR_SIZE;  //bytes

    scan_files(buff);
    LOG_INFO("[surface] all dir size total:%d, free:%d files:%d\n",
            fs_check_total_size,
            free_size,
            fs_all_file_counter);
}

/**
 * @brief   API to init surface manager service
 *
 * @param   None
 *
 * @return  None
 */
void surface_service_init(void)
{
//    uint8_t* work = (uint8_t *)ry_malloc(4096 *2);
//    f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096 *2);
//    ry_free(work);

    surface_check_hotfix_all_files();
    surface_restore();
    surface_repair();
    LOG_ERROR("[surface] flash op times:%d, [%lld,%d]\n",
                p_surface_stored->op_times,
                p_surface_stored->ctx.currentId,
                p_surface_stored->ctx.surfaceTbl.curNum
                );
    surface_check_list_all_files();
}

void surface_on_save_request(void)
{
//    if(ry_memcmp((void*)&p_surface_stored->ctx, (void*)&ss_ctx_v, sizeof(ss_ctx_t)) == 0)
//    {
//        //do not need to save
//    }
//    else
//    {
//        surface_manage_save();
//    }
}

void surface_on_exflash_set_id(uint32_t id)
{
    if((id >= 1)
        &&(id <= 6)
        ) {
        if(ss_ctx_v.currentId == 0xFFFFFFFFFFFFFFFF) {
            surface_service_init();
        }
        ss_ctx_v.currentId = id;
        surface_manage_save();
        system_app_data_save();
    }
}

void surface_on_reset(void)
{
    DIR* p_dir = ry_malloc(sizeof(DIR));
    FILINFO* p_fno = ry_malloc(sizeof(FILINFO));

    FRESULT res;
    char const* p_surface_path = "/surface";

    for(int i=0;i<MAX_SURFACE_NUM;i++) {
        uint64_t id = ss_ctx_v.surfaceTbl.surfaces[i].id;
        if((id > 0x100000000)
            &&(id != 0xFFFFFFFFFFFFFFFF)
            ) {
            LOG_INFO("[surface] reset delete user surface %lld\n", id);
            surface_resource_delete(id);
            surface_table_delete(id);
        }
    }

    if((p_dir != NULL)
        &&(p_fno != NULL)
        ) {
        res = f_opendir(p_dir, p_surface_path);                       /* Open the directory */
        if (res == FR_OK) {
            for (;;) {
                res = f_readdir(p_dir, p_fno);                   /* Read a directory item */
                if (res != FR_OK || p_fno->fname[0] == 0) break;  /* Break on error or end of dir */
                if (p_fno->fattrib & AM_DIR) {                    /* It is a directory */
                    continue;
                } else {                                       /* It is a file. */
                    LOG_DEBUG("[surface] reset found file \"%s/%s\", size:%d\n", p_surface_path, p_fno->fname, p_fno->fsize);
                    if(strstr(p_fno->fname, ".json")) {
                        bool isInSurfaceList = false;
                        for(int i=0;i<MAX_SURFACE_NUM;i++) {
                            if(ss_ctx_v.surfaceTbl.surfaces[i].id == 0xFFFFFFFFFFFFFFFF) {
                                continue;
                            }
                            if(strstr(ss_ctx_v.surfaceTbl.surfaces[i].name, p_fno->fname) == 0) {
                                isInSurfaceList = true;
                                break;
                            }
                        }

                        if(!isInSurfaceList) {
                            f_unlink(p_fno->fname);
                            LOG_INFO("[surface] reset and delete file :%s\n", p_fno->fname);
                        }
                    } else {
                        bool isInBgPicList = false;
                        for(int j =0;j<sizeof(p_pics)/sizeof(uint8_t*);j++) {
                            if(strcmp(p_fno->fname, p_pics[j]) == 0) {
                                isInBgPicList = true;
                                break;
                            }
                        }
                        if(!isInBgPicList) {
                            f_unlink(p_fno->fname);
                            LOG_INFO("[surface] reset and delete file :%s\n", p_fno->fname);
                        }
                    }
                }
            }
            f_closedir(p_dir);
        }
    }

    if(p_dir != NULL) {
        ry_free(p_dir);
    }

    if(p_fno != NULL) {
        ry_free(p_fno);
    }
    surface_repair();
}

void surface_on_ble_disconnected(void)
{
    if(ss_add_ctrl.status  != surface_header_status_idle) {
        surface_add_flow_pause();
    }
}


void surface_manager_debug_port(void)
{
#if 0
#endif
}
