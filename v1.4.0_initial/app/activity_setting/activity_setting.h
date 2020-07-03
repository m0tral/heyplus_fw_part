/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_SETTING_H__
#define __ACTIVITY_SETTING_H__

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

extern activity_t activity_setting;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the setting activity
 *
 * @param   None
 *
 * @return  setting_onCreate result
 */
ry_sts_t setting_onCreate(void* usrdata);

/**
 * @brief   API to exit setting activity
 *
 * @param   None
 *
 * @return  setting activity Destrory result
 */
ry_sts_t setting_onDestroy(void);

/**
 * @brief   API to setting_get_dnd_status
 *
 * @param   None
 *
 * @return  dnd_status - 0: can be disturbed, else: do not disturb
 */
uint8_t setting_get_dnd_status(void);

/**
 * @brief   API to setting_set_dnd_status
 *
 * @param   dnd_status - 0: can be disturbed, 1: do not disturb
 *
 * @return  None 
 */
void setting_set_dnd_status(u8_t en);

uint8_t setting_get_dnd_manual_status(void);

void setting_set_dnd_manual_status(u8_t en);

#endif  /* __ACTIVITY_SETTING_H__ */


