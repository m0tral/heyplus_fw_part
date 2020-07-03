/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __MOTAR_H__
#define __MOTAR_H__


/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */
#define MOTAR_WORKING_TIME_GENERAY  100 //ms



/*
 * TYPES
 *******************************************************************************
 */
typedef enum {
    MOTAR_STATE_OFF,
    MOTAR_STATE_ON,
    MOTAR_STATE_BREATHING,
} ry_motar_state_t;


typedef enum {
    MOTAR_EN_DISABLE,
    MOTAR_EN_ENABLE,
} ry_motar_enable_t;

typedef enum{
    MOTAR_TYPE_STRONG_LINEAR,
    MOTAR_TYPE_LIGHT_LINEAR,
    MOTAR_TYPE_STRONG_LOOP,
    MOTAR_TYPE_QUICK_LOOP,   
    MOTAR_TYPE_STRONG_TWICE,         
}ry_motar_type_t;

typedef struct {
    u8_t          state;
    u8_t          enable;
} ry_motar_ctx_t;

typedef struct{
    uint32_t type;
    uint32_t time;
}ry_motar_msg_param_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to enable led on-off
 *
 * @param   en, 1 enable led on-off, 0 disable led for swo function
 *
 * @return  None
 */
void ry_motar_enable_set(int en);

/**
 * @brief   API to motar_strong_linear_working, max speed
 *
 * @param   time_ms: motar on time
 *
 * @return  None
 */
void  motar_strong_linear_working(uint32_t time_ms);

/**
 * @brief   API to motar_light_linear_working, 60% speed
 *
 * @param   time_ms: motar on time
 *
 * @return  None
 */
void  motar_light_linear_working(uint32_t time_ms);

/**
 * @brief   API to motar_strong_loop_working, max speed
 *
 * @param   cnt: motar on time
 *
 * @return  None
 */
void  motar_strong_loop_working(uint32_t cnt);

/**
 * @brief   API to stop motar
 *
 * @param   None
 *
 * @return  None
 */
void  motar_stop(void);

void motar_set_work(int type, int time);

/**
 * @brief   API to motar_light_loop_working, max speed
 *
 * @param   cnt: motar on times
 *
 * @return  None
 */
void  motar_light_loop_working(uint32_t cnt);

/**
 * @brief   API to motar_quick_loop_working, quick speed
 *
 * @param   cnt: motar on time
 *
 * @return  None
 */
void  motar_quick_loop_working(uint32_t cnt);

/**
 * @brief   API to motar_strong_linear_twice, max speed
 *
 * @param   time_ms: motar on time
 *
 * @return  None
 */
void  motar_strong_linear_twice(uint32_t time_ms);

#endif  /* __MOTAR_H__ */


