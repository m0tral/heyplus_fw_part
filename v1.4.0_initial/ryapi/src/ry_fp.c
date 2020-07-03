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

#if (RY_FP_ENABLE == TRUE)


#include "fp.h"
#include "ryos.h"
#include "ry_timer.h"


/*********************************************************************
 * CONSTANTS
 */ 

//#define RY_FP_TEST

 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */

ry_sts_t ry_fp_enroll_start(void)
{
    fp_cmd_send(FP_CMD_ENROLL_START);
}

ry_sts_t ry_fp_enroll_stop(void)
{
    fp_cmd_send(FP_CMD_ERROLL_STOP);
}

ry_sts_t ry_fp_match(void)
{
    fp_cmd_send(FP_CMD_MATCH_START);
}



/**
 * @brief   API to init Fingerprint module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_fp_init(void)
{
    return fp_init();
}


/***********************************Test*****************************************/

#ifdef RY_FP_TEST

u32_t ry_fp_test_timer_id;

void ry_fp_test_enrollment(void)
{
    LOG_INFO("[FP] Test fingerprint enrollment\r\n");
    ry_fp_enroll_start();
}

void ry_fp_test_match(void)
{
    LOG_INFO("[FP] Test fingerprint match\r\n");
}


void ry_fp_test_timer_cb(u32_t timerId, void* userData)
{
    LOG_INFO("Timer callback. Timer id = %d, UserData = %d \r\n", timerId, userData);

    ry_fp_test_enrollment();
}

void ry_fp_test(void)
{
    ry_fp_test_timer_id = ry_timer_create(0);
    if (ry_fp_test_timer_id == RY_ERR_TIMER_ID_INVALID) {
        LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
    }

    ry_timer_start(ry_fp_test_timer_id, 1000, ry_fp_test_timer_cb, NULL);
}

#endif  /* RY_FP_TEST */


#endif  /* (RY_FP_ENABLE == TRUE) */
