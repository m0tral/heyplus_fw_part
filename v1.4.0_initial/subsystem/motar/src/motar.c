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
#include "ry_hal_inc.h"
#include "motar.h"
#include "scheduler.h"


/*********************************************************************
 * CONSTANTS
 */ 



/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
ry_motar_ctx_t motar_ctx_v = {
    MOTAR_STATE_OFF, 
    MOTAR_EN_ENABLE
};

/**
 * @brief   API to enable led on-off
 *
 * @param   en, 1 enable led on-off, 0 disable led for swo function
 *
 * @return  None
 */
void ry_motar_enable_set(int en)
{
    motar_ctx_v.enable = en;
    // LOG_DEBUG("[ry_motar_enable_set] enable: %d\r\n", motar_ctx_v.enable);    
}

/**
 * @brief   API to motar_strong_linear_working, max speed
 *
 * @param   time_ms: motar on time
 *
 * @return  None
 */
void  motar_strong_linear_working(uint32_t time_ms)
{
    if (motar_ctx_v.enable == MOTAR_EN_ENABLE){
        vibe_start(1, time_ms, 0, 60, 0);
    }
    // LOG_DEBUG("[motar_strong_linear_working] enable: %d, time_ms:%d\r\n", motar_ctx_v.enable, time_ms);
}

/**
 * @brief   API to motar_strong_linear_twice, max speed
 *
 * @param   time_ms: motar on time
 *
 * @return  None
 */
void  motar_strong_linear_twice(uint32_t time_ms)
{
    if (motar_ctx_v.enable == MOTAR_EN_ENABLE){
        vibe_start(2, time_ms, 200, 60, 0);
    }
    // LOG_DEBUG("[motar_strong_linear_twice] enable: %d, time_ms:%d\r\n", motar_ctx_v.enable, time_ms);
}

/**
 * @brief   API to motar_light_linear_working, 60% speed
 *
 * @param   time_ms: motar on time
 *
 * @return  None
 */
void  motar_light_linear_working(uint32_t time_ms)
{
    if (motar_ctx_v.enable == MOTAR_EN_ENABLE){
        vibe_start(1, time_ms, 0, 60, 0);
    }
    // LOG_DEBUG("[motar_light_linear_working] enable: %d\r\n", motar_ctx_v.enable);
}

/**
 * @brief   API to motar_strong_loop_working, max speed
 *
 * @param   cnt: motar on time
 *
 * @return  None
 */
void  motar_strong_loop_working(uint32_t cnt)
{
    if (motar_ctx_v.enable == MOTAR_EN_ENABLE){
        vibe_start(cnt, 500, 500, 100, 0);
    }
    // LOG_DEBUG("[motar_strong_loop_working] enable: %d\r\n\r\n", motar_ctx_v.enable);
}


/**
 * @brief   API to motar_quick_loop_working, quick speed
 *
 * @param   cnt: motar on time
 *
 * @return  None
 */
void  motar_quick_loop_working(uint32_t cnt)
{
    if (motar_ctx_v.enable == MOTAR_EN_ENABLE){
        vibe_start(cnt, 500, 300, 100, 0);
    }
    // LOG_DEBUG("[motar_strong_loop_working] enable: %d\r\n\r\n", motar_ctx_v.enable);
}

/**
 * @brief   API to motar_light_loop_working, max speed
 *
 * @param   cnt: motar on times
 *
 * @return  None
 */
void  motar_light_loop_working(uint32_t cnt)
{
    if (motar_ctx_v.enable == MOTAR_EN_ENABLE){
        vibe_start(cnt, 500, 2500, 60, 0);
    }
    // LOG_DEBUG("[motar_light_loop_working] enable: %d\r\n", motar_ctx_v.enable);
}

/**
 * @brief   API to stop motar
 *
 * @param   None
 *
 * @return  None
 */
void  motar_stop(void)
{
    vibe_start(1, 0, 100, 0, 0);
    vibe_stop();
    // LOG_DEBUG("[motar_stop] enable: %d\r\n", motar_ctx_v.enable);    
}



void motar_set_work(int type, int time)
{
    ry_motar_msg_param_t motar_param = {0};

    // LOG_DEBUG("[motar_set_work] enable: %d, type:%d, time:%d\r\n", motar_ctx_v.enable, type, time);    
    
    motar_param.type = type;
    motar_param.time = time;
    ry_sched_msg_send(IPC_MSG_TYPE_DEV_MOTAR, sizeof(ry_motar_msg_param_t), (u8_t *)&motar_param);    
}
