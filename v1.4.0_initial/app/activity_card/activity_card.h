/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __ACTIVITY_CARD_H__
#define __ACTIVITY_CARD_H__

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

extern activity_t activity_card;
extern activity_t activity_card_session;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   Entry of the Card activity
 *
 * @param   None
 *
 * @return  activity_card_onCreate result
 */
ry_sts_t activity_card_onCreate(void* usrdata);

/**
 * @brief   API to exit Card activity
 *
 * @param   None
 *
 * @return  card activity Destrory result
 */
ry_sts_t activity_card_onDestrory(void);

ry_sts_t activity_card_session_onCreate(void* usrdata);
ry_sts_t activity_card_session_onDestrory(void);




#endif  /* __ACTIVITY_CARD_H__ */





