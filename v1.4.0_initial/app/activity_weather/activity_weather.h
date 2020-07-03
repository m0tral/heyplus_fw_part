/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_WEATHER_H__
#define __ACTIVITY_WEATHER_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */

#define WEATHER_TYPE_00                         "m_weather_00_sunny.bmp"
#define WEATHER_TYPE_01                         "m_weather_10_sunny_night.bmp"
#define WEATHER_TYPE_02                         "m_weather_04_cloud.bmp"
#define WEATHER_TYPE_03                         "m_weather_10_cloud_night.bmp"
#define WEATHER_TYPE_04                         "m_weather_02_overcast.bmp"
#define WEATHER_TYPE_05                         "m_weather_01_rainning.bmp"
#define WEATHER_TYPE_06                         "m_weather_03_snow.bmp"
#define WEATHER_TYPE_07                         "m_weather_07_haza.bmp"
#define WEATHER_TYPE_08                         "m_weather_10_rsvd.bmp"

#define AQI_LEVEL_0                             50
#define AQI_LEVEL_1                             100
#define AQI_LEVEL_2                             150
#define AQI_LEVEL_3                             200
//#define AQI_LEVEL_4     

#define WEATHER_GET_INFO_INTERVAL               3*60*60 //3 hours



/*
 * TYPES
 *******************************************************************************
 */
 
typedef struct {
    u16_t type_num;
    const char* img;
    const char* str;
} wth_phenomena_t;


/*
 * VARIABLES
 *******************************************************************************
 */

extern activity_t activity_weather;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the Weather activity
 *
 * @param   None
 *
 * @return  activity_weather_onCreate result
 */
ry_sts_t activity_weather_onCreate(void* usrdata);

/**
 * @brief   API to exit weather activity
 *
 * @param   None
 *
 * @return  Weather activity Destrory result
 */
ry_sts_t activity_weather_onDestrory(void);



#endif  /* __ACTIVITY_WEATHER_H__ */






