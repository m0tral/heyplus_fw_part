/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __GSENSOR_H__
#define __GSENSOR_H__

typedef enum {
    GSENSOR_POLLING = 0x00,
    GSENSOR_INTERRUPT
} ry_gsensor_mode_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to get gsensor data
 *          should polling to get data when not in interrupt mode
 *
 * @param   None
 *
 * @return  Status - 0: Succ, else: error
 */
ry_sts_t gsensor_get_data(void);

/**
 * @brief   API to init Gsensor module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t gsensor_init(void);

#endif  /* __GSENSOR_H__ */

