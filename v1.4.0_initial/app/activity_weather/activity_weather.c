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


/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "window_management_service.h"
#include "activity_weather.h"
#include "activity_card.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "data_management_service.h"
#include "card_management_service.h"

/* resources */
#include "gui_img.h"
#include"gui_msg.h"
#include "ry_font.h"
#include "ry_statistics.h"
#include "scrollbar.h"


/*********************************************************************
 * CONSTANTS
 */ 
#define WEATHER_LIST_MAX                    5

#define SCROLLBAR_HEIGHT                    40
#define SCROLLBAR_WIDTH                     1


#define TEMPERATURE_CHECK(x)    (((-100 < x) && (x < 100))?(true):(false))

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    int8_t    list;
    u32_t     update_time;
} ry_weather_ctx_t;

typedef struct {
    uint8_t* city;
    uint8_t  temperature_low;
    uint8_t  temperature_high;
    uint8_t  phenomena_id;
    uint16_t air_aqi;
} weather_disp_t;
 

 /*********************************************************************
 * VARIABLES
 */
extern activity_t activity_surface;

ry_weather_ctx_t wth_ctx_v;

scrollbar_t * weather_scrollbar = NULL;

const char* const text_add_weather[2] = {
		"Добавьте",
		"в телефоне",
};

const char* const text_wind_speed = "ветер, м/c";


const wth_phenomena_t wth_phnm_bmp[] = {
    {0,     WEATHER_TYPE_00,   "? "},
    {1,     WEATHER_TYPE_02,   "?? "},    
    {2,     WEATHER_TYPE_04,   "? "},
    {3,     WEATHER_TYPE_05,   "?? "},
    {4,     WEATHER_TYPE_05,   "??? "},
    {5,     WEATHER_TYPE_05,   "???? "},
    {6,     WEATHER_TYPE_05,   "??? "},
    {7,     WEATHER_TYPE_05,   "?? "},
    {8,     WEATHER_TYPE_05,   "?? "},
    {9,     WEATHER_TYPE_05,   "?? "},
    {10,    WEATHER_TYPE_05,   "??"},
    {11,    WEATHER_TYPE_05,   "??? "},
    {12,    WEATHER_TYPE_05,   "???? "},
    {13,    WEATHER_TYPE_06,   "?? "},
    {14,    WEATHER_TYPE_06,   "?? "},
    {15,    WEATHER_TYPE_06,   "?? "},
    {16,    WEATHER_TYPE_06,   "?? "},
    {17,    WEATHER_TYPE_06,   "?? "},
    {18,    WEATHER_TYPE_07,   "? "},
    {19,    WEATHER_TYPE_06,   "?? "},
    {20,    WEATHER_TYPE_08,   "?? "},
    {21,    WEATHER_TYPE_08,   "?? "},
    {22,    WEATHER_TYPE_08,   "??? "},
    {23,    WEATHER_TYPE_07,   "?? "},
    {24,    WEATHER_TYPE_07,   "??? "},
    {25,    WEATHER_TYPE_07,   "? "},
    {26,    WEATHER_TYPE_07,   "?? "},
    {27,    WEATHER_TYPE_07,   "?? "},
    {28,    WEATHER_TYPE_07,   "??? "},
    {29,    WEATHER_TYPE_07,   "?? "},
    {30,    WEATHER_TYPE_07,   "?? "},
    {31,    WEATHER_TYPE_05,   "? "},
    {32,    WEATHER_TYPE_06,   "? "},
    {49,    WEATHER_TYPE_07,   "?? "},
    {53,    WEATHER_TYPE_07,   "? "},
    {54,    WEATHER_TYPE_07,   "?? "},
    {55,    WEATHER_TYPE_07,   "?? "},
    {56,    WEATHER_TYPE_07,   "?? "},
    {57,    WEATHER_TYPE_07,   "?? "},
    {58,    WEATHER_TYPE_07,   "??? "},
    {99,    NULL,              "? "},
    {301,   WEATHER_TYPE_05,   "? "},
    {302,   WEATHER_TYPE_06,   "? "},		
};

const int wth_phnm_bmp_len = sizeof(wth_phnm_bmp)/sizeof(wth_phenomena_t);

//weather_disp_t wth_today[WEATHER_LIST_MAX] = {
//    {"Bejing",  7,  12, 0, 280},
//    {"Shanghai", 11, 20, 2, 145},    
//    {"Shenzhen", 15, 23, 1, 45},
//};
 
activity_t activity_weather = {
    .name      = "weather",
    .onCreate  = activity_weather_onCreate,
    .onDestroy = activity_weather_onDestrory,
    .priority = WM_PRIORITY_M,
};

FIL * weather_fp = NULL;
WeatherGetInfoResult * full_weather_info = NULL;
u32_t aqi_color[] = {0x85d115, 0xf8e71c, 0xfbe300, 0xfc2739, 0xd81324};


/*********************************************************************
 * LOCAL FUNCTIONS
 */


const wth_phenomena_t * get_weather_phenomena(u32_t type)
{
    if(type == 0){
        return &wth_phnm_bmp[0];
    }else if(type == 1){
        return &wth_phnm_bmp[1];
    }else if(type == 0){

    }else if(type == 0){

    }else if(type == 0){

    }else if(type == 0){

    }else if(type == 0){

    }else if(type == 0){

    }

    return NULL;
}


ry_sts_t weather_display_update(void)
{
    ry_sts_t ret = RY_SUCC;
    WeatherGetInfoResult * cur_weather_info = full_weather_info;
    const char* phenomena_str = "--";
    const char* aqi_str = "--";
    u32_t i = 0;
    LOG_DEBUG("[weather_display_update] begin.\r\n");
    clear_buffer_black();
    int max_type = sizeof(wth_phnm_bmp)/sizeof(wth_phenomena_t);

    WeatherInfo * city_info_list = NULL;
    u8_t date_offset = 0;
    u8_t info_display_flag = 0;
    char * city_str = NULL;

    ry_time_t time;
    tms_get_time(&time);

    if (cur_weather_info == NULL || city_list.city_items_count == 0) {
        clear_buffer_black();
        gdispFillStringBox( 0, 
                            136, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_add_weather[0], 
                            (void*)font_roboto_20.font, 
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        gdispFillStringBox( 0, 
                            164, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_add_weather[1], 
                            (void*)font_roboto_20.font, 
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        uint8_t w, h;
        draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"m_no_alarm.bmp",\
                                0, 68, &w, &h, d_justify_center);
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        return 0 ;
    }
    

    if(cur_weather_info != NULL){
        for(i = 0; i < cur_weather_info->infos_count; i++){
            if(strcmp(&(city_list.city_items[wth_ctx_v.list].city_id[0]), &(cur_weather_info->infos[i].city_id[0]) ) == 0){
                city_info_list = &(cur_weather_info->infos[i]);
                break;
            }
        }
    }

    if((cur_weather_info == NULL) ||(city_info_list == NULL)||(city_info_list->daily_infos_count == 0)\
        || (city_list.city_items_count == 0)){
        info_display_flag = 1;
    }else{

        for (i = 0; i < city_info_list->daily_infos_count; i++){
            if(ryos_utc_now_notimezone() - city_info_list->daily_infos[i].date <= 24*60*60){
                date_offset = i;
                break;
            }
        }
    }

    if (city_list.city_items_count == 0) {
        city_str = "--";
    } else {
        city_str = city_list.city_items[wth_ctx_v.list].city_name;        
        //int city_name_len = getStringCharCount(city_str);
        u32_t title_width = getStringWidthInnerFont(city_str, font_roboto_20.font);
        
        if (title_width > RM_OLED_WIDTH) {            
            u32_t title_offset = getWidthOffsetInnerFont(city_str, RM_OLED_WIDTH-8, font_roboto_20.font);
            city_str[title_offset + 1] = 0;
        }
    }
    // city name
    gdispFillString(0, 4, city_str, (void*)font_roboto_20.font, Gray, Black);  
            
    int weather_day_count = 0;
    int delta_y = 70;
    uint8_t temp_str_realtime[10];
    uint8_t temp_str[20];
    
    do
    {			
        // ---- current day weather start -------------------------------------------------			

        //weather phenomena     
        int weather_type = 0;

        if(date_offset == 0){
            weather_type = city_info_list->daily_infos[date_offset].realtime_type;
        }else{
                
            if(time.hour > 18){
                weather_type = city_info_list->daily_infos[date_offset].forecast_night_type;
            }else{
                weather_type = city_info_list->daily_infos[date_offset].forecast_day_type;
            }
        }
        if((info_display_flag)||(weather_type >= max_type)){
            phenomena_str = "--";
        }
        else{
            for(i = 0; i < max_type; i++){
                if(weather_type == wth_phnm_bmp[i].type_num){
                    phenomena_str = wth_phnm_bmp[i].str;
                    LOG_DEBUG("i == %d\n", i);
                    break;
                }
            }
        }
        
        uint8_t w, h;
        if((info_display_flag)||(city_info_list->daily_infos[date_offset].realtime_type >= max_type)){
                
        }else{
            if(i < max_type){
                draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)wth_phnm_bmp[i].img, \
                                        2, 30 + (delta_y * weather_day_count), &w, &h, d_justify_left);
            }
        }			
    
        // temperature					
        if((info_display_flag)|| (city_info_list->daily_infos[date_offset].has_realtime_temp == 0) ||\
            ((city_info_list->daily_infos[date_offset].realtime_temp) < -100)){
            sprintf((char*)temp_str_realtime, "--");
        } else {
            sprintf((char*)temp_str_realtime, "%d°C", (int)(city_info_list->daily_infos[date_offset].realtime_temp));
        }
        
        if(TEMPERATURE_CHECK((int)(city_info_list->daily_infos[date_offset].forecast_day_temp)) &&
            TEMPERATURE_CHECK((int)(city_info_list->daily_infos[date_offset].forecast_night_temp))) {
            sprintf((char*)temp_str, "%d/%d°C",
                    (int)city_info_list->daily_infos[date_offset].forecast_night_temp,
                    (int)city_info_list->daily_infos[date_offset].forecast_day_temp);
        } else {
            sprintf((char*)temp_str, "-/-");
        }
        
        if (weather_day_count == 0) {
            gdispFillString( 62, 40 + (delta_y * weather_day_count),
                            (char *)temp_str_realtime, 
                            (void*)font_roboto_20.font,
                            White, Black); 
        };

        gdispFillString( 35, 72 + (delta_y * weather_day_count),
                        (char *)temp_str, 
                        (void*)font_roboto_20.font,
                        White, Black); 
        
        if (weather_day_count == 0) {

            // wind speed              
            gdispFillString( 0, 180,
                            (char *)text_wind_speed, 
                            (void*)font_roboto_20.font,
                            White, Black); 

            // wind speed value                      
            //uint8_t aqi_value[20];
            color_t color;
            if((info_display_flag) ||(city_info_list->daily_infos[date_offset].realtime_aqi <= 0)){
                sprintf((char*)temp_str, "--");
                color = White;
            } else {
                sprintf((char*)temp_str, "%d", city_info_list->daily_infos[date_offset].realtime_aqi);
                //color = (city_info_list->daily_infos[date_offset].realtime_aqi > 100) ? Orange : Green;
                //color = (city_info_list->daily_infos[date_offset].realtime_aqi > 100) ? Red : color;
                if(city_info_list->daily_infos[date_offset].realtime_aqi < AQI_LEVEL_0){
                        color = aqi_color[0];
                }else if(city_info_list->daily_infos[date_offset].realtime_aqi < AQI_LEVEL_1){
                        color = aqi_color[1];
                }else if(city_info_list->daily_infos[date_offset].realtime_aqi < AQI_LEVEL_2){
                        color = aqi_color[2];
                }else if(city_info_list->daily_infos[date_offset].realtime_aqi < AQI_LEVEL_3){
                        color = aqi_color[3];
                }else{
                        color = aqi_color[4];
                }        
            }

            gdispFillStringBox( 0, 210, font_roboto_20.width, font_roboto_20.height,
                                (char*)temp_str, (void*)font_roboto_20.font, color, Black, justifyCenter);
        }

        // ---- current day weather end -------------------------------------------------

        weather_day_count++;
        date_offset++;
        
        if (date_offset >= city_info_list->daily_infos_count - 1) break;
    }
    while (weather_day_count < 2);
		
    if(/*(info_display_flag == 0) && */(city_list.city_items_count >= 2)){
        if(weather_scrollbar != NULL){
            u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (city_list.city_items_count -1);
            scrollbar_show(weather_scrollbar, frame_buffer, 
                (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-(((city_list.city_items_count -1)-wth_ctx_v.list)  * sp_posix));
        }
    }		

    //update_buffer_to_oled();
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    LOG_DEBUG("[weather_display_update] end.\r\n");
    return ret;
}

int weather_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if ((p->y >= 8)){
                wms_activity_back(NULL);
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            wth_ctx_v.list ++;
            if(/*(full_weather_info != NULL) && */(wth_ctx_v.list >= city_list.city_items_count)){
                wth_ctx_v.list = city_list.city_items_count - 1;
            }
            DEV_STATISTICS(weather_change_count);
            weather_display_update();
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            // To see message list
            LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);
            wth_ctx_v.list --;
            if (wth_ctx_v.list <= 0){
                wth_ctx_v.list = 0;
            }
            DEV_STATISTICS(weather_change_count);
            weather_display_update();
            break;

        default:
            break;
    }
    return consumed; 
}

int weather_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}


int weather_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}


/**
 * @brief   Entry of the Weather activity
 *
 * @param   None
 *
 * @return  activity_weather_onCreate result
 */
ry_sts_t activity_weather_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }else{
        if((ryos_utc_now() - wth_ctx_v.update_time > WEATHER_GET_INFO_INTERVAL)&&(ry_ble_get_state() == RY_BLE_STATE_CONNECTED)){
            wth_ctx_v.update_time = ryos_utc_now();
            ry_data_msg_send(DATA_SERVICE_MSG_GET_REAL_WEATHER, 0, NULL);
            DEV_STATISTICS(weather_req_count);
            LOG_DEBUG("get weather cmd send\n");
        }
    }

    LOG_DEBUG("[activity_weather_onCreate]\r\n");

    if(weather_scrollbar == NULL){
        weather_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);
    }

    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_weather, weather_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_weather, weather_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_weather, weather_onProcessedCardEvent);
    //ry_data_msg_send(DATA_SERVICE_MSG_GET_REAL_WEATHER, 0, NULL);
    wth_ctx_v.list = 0;
    u32_t read_bytes = 0;

    if(weather_fp == NULL){
        weather_fp = (FIL *)ry_malloc(sizeof(FIL));
    }
    
    if(weather_fp != NULL){

        FRESULT state = f_open(weather_fp, WEATHER_FORECAST_DATA, FA_READ);
        if(state != FR_OK){
            ry_free(weather_fp);
            weather_fp = NULL;
        }else{
            if(full_weather_info == NULL){
                full_weather_info = (WeatherGetInfoResult *)ry_malloc(sizeof(WeatherGetInfoResult));
            }

            if(full_weather_info == NULL){

            }else{
                state = f_read(weather_fp, full_weather_info, sizeof(WeatherGetInfoResult), &read_bytes);

                if((state != FR_OK) || (read_bytes < sizeof(WeatherGetInfoResult))){
                    ry_free(full_weather_info);
                    full_weather_info = NULL;
                }

                f_close(weather_fp);
                ry_free(weather_fp);
                weather_fp = NULL;
            }
        }
    }else{
        full_weather_info = NULL;
    }
    /* Add Layout init here */
    weather_display_update();

    return ret;
}

/**
 * @brief   API to exit Weather activity
 *
 * @param   None
 *
 * @return  Weather activity Destrory result
 */
ry_sts_t activity_weather_onDestrory(void)
{
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_weather);
    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_weather);
    wms_event_listener_del(WM_EVENT_CARD, &activity_weather);
    wth_ctx_v.list = 0;

    if(weather_scrollbar != NULL){
        scrollbar_destroy(weather_scrollbar);
        weather_scrollbar = NULL;
    }

    f_close(weather_fp);
    ry_free(weather_fp);
    ry_free(full_weather_info);
    weather_fp = NULL;
    full_weather_info = NULL;

    return RY_SUCC;    
}
