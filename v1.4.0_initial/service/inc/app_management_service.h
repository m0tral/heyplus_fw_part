/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __APP_MANAGEMENT_SERVICE_H__
#define __APP_MANAGEMENT_SERVICE_H__

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

#define MAX_APP_NUM                      10
#define CUR_APP_NUM                      10


#define APPID_SYSTEM_NONE                0x0000

#define APPID_SYSTEM_CARD                0x0001
#define APPID_SYSTEM_SPORT               0x0002
#define APPID_SYSTEM_HRM                 0x0003
#define APPID_SYSTEM_MIJIA               0x0004
#define APPID_SYSTEM_WEATHER             0x0005
#define APPID_SYSTEM_DATA                0x0006
#define APPID_SYSTEM_SETTING             0x0007
#define APPID_SYSTEM_ALARM               0x0008
#define APPID_SYSTEM_ALIPAY              0x0009
#define APPID_SYSTEM_TIMER               0x000A

#define FLASH_ADDR_APP_CONTEXT           0xFFC00


/*
 * TYPES
 *******************************************************************************
 */

typedef struct {
    int appid;
    char* icon;
    char* name;
} app_prop_t;




/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init APP manager service
 *
 * @param   None
 *
 * @return  None
 */
void app_service_init(void);


/**
 * @brief   API to set application sequence
 *
 * @param   None
 *
 * @return  None
 */
void app_sequence_set(u8_t* data, int len);


/**
 * @brief   API to get current application sequence
 *
 * @param   None
 *
 * @return  None
 */
void app_sequence_get(u8_t* data, int len);



app_prop_t* app_prop_getById(int appid);
int app_prop_getIdBySeq(int seq);
uint8_t app_ui_update(uint8_t reload);


#endif  /* __APP_MANAGEMENT_SERVICE_H__ */





