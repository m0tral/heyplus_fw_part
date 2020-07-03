/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_ALERT_H__
#define __ACTIVITY_ALERT_H__

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

typedef enum {
   EVT_ALERT_BATTERY_LOW,
   EVT_ALERT_LONG_SIT,  
   EVT_ALERT_10000_STEP,         
}evnet_alert_t;

/*
 * VARIABLES
 *******************************************************************************
 */

extern activity_t activity_alert;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the alert activity
 *
 * @param   None
 *
 * @return  activity_alert_onCreate result
 */
ry_sts_t activity_alert_onCreate(void* usrdata);

/**
 * @brief   API to exit alert activity
 *
 * @param   None
 *
 * @return  alert activity Destrory result
 */
ry_sts_t activity_alert_onDestrory(void);



#endif  /* __ACTIVITY_ALERT_H__ */




