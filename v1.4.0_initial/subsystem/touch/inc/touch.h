/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __TOUCH_H__
#define __TOUCH_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */


#define RY_TOUCH_STATE_IDLE             0
#define RY_TOUCH_STATE_IDLE_SCAN        1
#define RY_TOUCH_STATE_ACTIVE_SCAN      2


#define TP_ACTION_NONE                  0
#define TP_ACTION_DOWN                  1
#define TP_ACTION_MOVE                  2
#define TP_ACTION_UP                    3
#define TP_ACTION_CANCEL                4


#define TP_PROCESSED_ACTION_SLIDE_DOWN      1
#define TP_PROCESSED_ACTION_SLIDE_UP        2
#define TP_PROCESSED_ACTION_TAP             3
#define TP_PROCESSED_ACTION_LONG_TAP        4
#define TP_PROCESSED_ACTION_EXTRA_LONG_TAP  5


#define	DELAY_PROCESSED_ACTION_TAP              40
#define	DELAY_PROCESSED_ACTION_LONG_TAP         1500
#define	DELAY_PROCESSED_ACTION_EXTRA_LONG_TAP   2500



typedef enum {
    TP_MODE_ACTIVE      =   0x01,
    TP_MODE_BTN_ONLY    =   0x02,
} tp_mode_t;



/*
 * TYPES
 *******************************************************************************
 */

typedef struct {
    u8_t          action;
    u16_t         x;
    u16_t         y;
    u32_t         t;
} tp_event_t;

typedef struct {
    u8_t          action;
    int           y;
} tp_processed_event_t;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init Touch module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t touch_init(void);

/**
 * @brief   API to enable Touch module
 *
 * @param   None
 *
 * @return  None
 */
void touch_enable(void);

/**
 * @brief   API to disable Touch module
 *
 * @param   None
 *
 * @return  None
 */
void touch_disable(void);


/**
 * @brief   API to set Touch mode
 *
 * @param   None
 *
 * @return  None
 */
void touch_mode_set(tp_mode_t mode);

/**
 * @brief   API to set Touch bypass mode.
 *
 * @param   None
 *
 * @return  None
 */
void touch_bypass_set(u8_t enable);

/**
 * @brief   API to set Touch into low power mode
 *          - button only and un-initial I2C
 *
 * @param   None
 *
 * @return  None
 */
void touch_enter_low_power(void);

/**
 * @brief   API to set Touch exit low power
 *          - initial I2C
 *
 * @param   None
 *
 * @return  None
 */
void touch_exit_low_power(void);

void touch_reset(void);
void touch_halt(int enable);
uint32_t tp_firmware_upgrade(void);
void touch_baseline_reset(void);

bool is_touch_in_test(void);


#endif  /* __TOUCH_H__ */


