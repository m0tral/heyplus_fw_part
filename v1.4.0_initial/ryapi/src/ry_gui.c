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
#include "gui.h"
#include "app_config.h"

#if (RY_GUI_ENABLE == TRUE)


//#include ".h"
#include "ryos.h"
//#include "ry_timer.h"


/*********************************************************************
 * CONSTANTS
 */ 

//#define RY_GUI_TEST

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
 * @brief   API to init GUI module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_gui_init(void)
{
    ry_sts_t ret = RY_ERR_INIT_DEFAULT;
    ret = gui_init();    
    return RY_SUCC;
}


/***********************************Test*****************************************/

#ifdef RY_GUI_TEST

u32_t ry_gui_test_timer_id;

void ry_gui_test_timer_cb(u32_t timerId, void* userData)
{
    LOG_INFO("Timer callback. Timer id = %d, UserData = %d \r\n", timerId, userData);

    
}

void ry_gui_test(void)
{
    ry_nfc_test_timer_id = ry_timer_create(0);
    if (ry_nfc_test_timer_id == RY_ERR_TIMER_ID_INVALID) {
        LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
    }

    ry_timer_start(ry_nfc_test_timer_id, 1000, ry_nfc_test_timer_cb, NULL);
}

#endif  /* RY_GUI_TEST */


#endif /* (RY_GUI_ENABLE == TRUE) */

