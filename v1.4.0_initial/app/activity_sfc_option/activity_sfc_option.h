/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_SFC_OPTION_H__
#define __ACTIVITY_SFC_OPTION_H__

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

extern activity_t activity_sfc_option;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the sfc_option activity
 *
 * @param   None
 *
 * @return  activity_sfc_option_onCreate result
 */
ry_sts_t activity_sfc_option_onCreate(void* usrdata);

/**
 * @brief   API to exit sfc_option activity
 *
 * @param   None
 *
 * @return  sfc_option activity Destrory result
 */
ry_sts_t activity_sfc_option_onDestrory(void);



#endif  /* __ACTIVITY_SFC_OPTION_H__ */






