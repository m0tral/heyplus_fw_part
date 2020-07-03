/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_DATA_H__
#define __ACTIVITY_DATA_H__

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

extern activity_t activity_data;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the Data activity
 *
 * @param   None
 *
 * @return  acvivity_data_onCreate result
 */
ry_sts_t activity_data_onCreate(void* usrdata);

/**
 * @brief   API to exit data activity
 *
 * @param   None
 *
 * @return  data activity Destrory result
 */
ry_sts_t activity_data_onDestrory(void);



#endif  /* __ACTIVITY_DATA_H__ */



