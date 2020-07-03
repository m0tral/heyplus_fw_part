/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_HRM_H__
#define __ACTIVITY_HRM_H__

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

extern activity_t activity_hrm;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the hrm activity
 *
 * @param   None
 *
 * @return  hrm_onCreate result
 */
ry_sts_t hrm_onCreate(void* usrdata);

/**
 * @brief   API to exit hrm activity
 *
 * @param   None
 *
 * @return  hrm activity Destrory result
 */
ry_sts_t hrm_onDestroy(void);



#endif  /* __ACTIVITY_HRM_H__ */


