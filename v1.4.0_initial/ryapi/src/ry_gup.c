/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_mcu_config.h"

#include "gup.h"


/*********************************************************************
 * CONSTANTS
 */ 


 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */


/**
 * @brief   API to init Diagnostic module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_gup_init(void)
{
    return gup_init();
}
