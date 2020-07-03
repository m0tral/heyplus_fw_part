/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_ALARM_RING_H__
#define __ACTIVITY_ALARM_RING_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */
#define UI_SYS_ALARM_ON            0x0A

// not actual
// used from device settings
// since 2020.03
// #define ALARM_SNOOZE_INTERVAL      10   // 10 minutes

#define ALARM_SNOOZE_MAX_TIMES     10


/*
 * TYPES
 *******************************************************************************
 */


/*
 * VARIABLES
 *******************************************************************************
 */

extern activity_t activity_alarm_ring;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the Alarm_ring activity
 *
 * @param   None
 *
 * @return  activity_alarm_ring_onCreate result
 */
ry_sts_t activity_alarm_ring_onCreate(void* usrdata);

/**
 * @brief   API to exit Alarm_ring activity
 *
 * @param   None
 *
 * @return  alarm_ring activity Destrory result
 */
ry_sts_t activity_alarm_ring_onDestroy(void);



#endif  /* __ACTIVITY_ALARM_RING_H__ */




