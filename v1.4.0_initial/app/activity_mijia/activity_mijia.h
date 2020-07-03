/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_MIJIA_H__
#define __ACTIVITY_MIJIA_H__

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

extern activity_t activity_mijia;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the mijia activity
 *
 * @param   None
 *
 * @return  activity_mijia_onCreate result
 */
ry_sts_t activity_mijia_onCreate(void* usrdata);

/**
 * @brief   API to exit mijia activity
 *
 * @param   None
 *
 * @return  mijia activity Destroy result
 */
ry_sts_t activity_mijia_onDestroy(void);



#endif  /* __ACTIVITY_MIJIA_H__ */






