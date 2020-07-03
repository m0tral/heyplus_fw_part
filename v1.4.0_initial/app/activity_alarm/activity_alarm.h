/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_ALARM_H__
#define __ACTIVITY_ALARM_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */



/*
 * TYPES
 *******************************************************************************
 */



/*
 * VARIABLES
 *******************************************************************************
 */

extern activity_t activity_alarm;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the Alarm activity
 *
 * @param   None
 *
 * @return  activity_alarm_onCreate result
 */
ry_sts_t activity_alarm_onCreate(void* usrdata);

/**
 * @brief   API to exit Alarm activity
 *
 * @param   None
 *
 * @return  alarm activity Destrory result
 */
ry_sts_t activity_alarm_onDestroy(void);

/**
 * @brief   API to get_alarm_repeat_string
 *
 * @param   repeat: pointer to the alarm repeat paraments
 *
 * @return  pointer to the alarm repeat string
 */
char* get_alarm_repeat_string(uint8_t* repeat);

#endif  /* __ACTIVITY_ALARM_H__ */




