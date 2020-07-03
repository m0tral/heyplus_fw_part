/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __MENU_H__
#define __MENU_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "window_management_service.h"


/*
 * CONSTANTS
 *******************************************************************************
 */


#define MENU_SCROLLBAR_LENGTH       40


/*
 * TYPES
 *******************************************************************************
 */



/*
 * VARIABLES
 *******************************************************************************
 */

extern activity_t activity_menu;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Create callback when menu activity be awake
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t menu_onCreate(void* usrdata);

/**
 * @brief   Destroy callback when menu activity be disabled
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t menu_onDestrory(void);

/**
 * @brief   menu_index_sequence_reset
 *
 * @param   None
 *
 * @return  None
 */
void menu_index_sequence_reset(void);

#endif  /* __MENU_H__ */


