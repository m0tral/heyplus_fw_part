/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __MIJIA_SERVICE_H__
#define __MIJIA_SERVICE_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "MiScene.pb.h"


/*
 * CONSTANTS
 *******************************************************************************
 */

#define MAX_MIJIA_SCENE_NUM         5

#define MAX_MIJIA_MID_LEN           20
#define MAX_MIJIA_SCENE_NAME_LEN    70

#define MIJIA_SCENE_SECTOR          246


#define MIJIA_MODEL_SAKE            "ryeex.bracelet.sake"
#define MIJIA_EVT_STR_CLICK         "event.ryeex.bracelet.sake.4097"
#define MIJIA_EVT_STR_SLEEP         "event.ryeex.bracelet.sake.4098"

#define MIJIA_EVT_VALUE_CLICK_A     "010000"
#define MIJIA_EVT_VALUE_CLICK_B     "020000"
#define MIJIA_EVT_VALUE_SLEEP       "00"
#define MIJIA_EVT_VALUE_WAKEUP      "01"


#define MIJIA_SLEEP_TYPE_ENTER_SLEEP     0x00
#define MIJIA_SLEEP_TYPE_WAKE_UP         0x01

#define DEV_MIJIA_DEBUG                  0

/*
 * TYPES
 *******************************************************************************
 */


typedef struct {
    u32_t curNum;
    MiSceneInfo scenes[MAX_MIJIA_SCENE_NUM];
} mijia_scene_tbl_t;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init Mijia Service.
 *
 * @param   None
 *
 * @return  None
 */
void mijia_service_init(void);
void mijia_scene_get_list(u8_t* data, int len);
void mijia_scene_add(u8_t* data, int len);
void mijia_scene_add_batch(u8_t* data, int len);
void mijia_scene_modify(u8_t* data, int len);
void mijia_scene_delete(u8_t* data, int len);
void mijia_scene_delete_all(u8_t* data, int len);
mijia_scene_tbl_t* mijia_scene_get_table(void);
bool is_mijia_sleep_scene_exist(u8_t type);
bool is_sleep_scene(MiSceneInfo* p);
ry_sts_t scene_tbl_reset(void);



#endif  /* __MIJIA_SERVICE_H__ */



