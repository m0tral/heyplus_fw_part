/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __HRM_H__
#define __HRM_H__


#include <stdint.h>
#include "timer_management_service.h"


/*
 * CONSTANTS
 *******************************************************************************
 */



/**
 * @brief Definitaion for RY_NFC message type.
 */
#define RY_HRM_MSG_TYPE_NONE                  0
#define RY_HRM_MSG_TYPE_DETECT_CANCEL         1
#define RY_HRM_MSG_TYPE_AUTO_DETECT           2
#define RY_HRM_MSG_TYPE_MANUAL_DETECT         3
#define RY_HRM_MSG_TYPE_SPORT_DETECT          4
#define RY_HRM_MSG_TYPE_SLEEP_DETECT          5
#define RY_HRM_MSG_TYPE_TEST_DETECT           6




/*
 * TYPES
 *******************************************************************************
 */


typedef enum {
    HRM_START_AUTO    	=   0x00,
    HRM_START_FIXED     =   0x01, 
} hrm_start_mode_t;

typedef enum {
    HRM_MODE_IDLE,
    HRM_MODE_MFG_JSON,            
    HRM_MODE_AUTO_SAMPLE,
    HRM_MODE_SLEEP_LOG,
    HRM_MODE_SPORT_RUN,
    HRM_MODE_MANUAL,
    HRM_MODE_WEAR_DETECT,
}hrm_mode_t;


typedef enum {
    WEAR_DET_FAIL = 0,
    WEAR_DET_SUCC = 1,
    WEAR_DET_INIT = 0xff,
}wear_det_st_t;

typedef struct {
    uint8_t    working_mode;
    uint8_t    start_mode;
    uint8_t    weared;
    uint8_t    session;
    uint8_t    is_prepare;    
} hrm_status_t;

typedef enum {
    HRM_MSG_CMD_AUTO_SAMPLE_STOP,
    HRM_MSG_CMD_AUTO_SAMPLE_START,          // 10 secs in each 10 mins
    HRM_MSG_CMD_MANUAL_SAMPLE_STOP,
    HRM_MSG_CMD_MANUAL_SAMPLE_START,        // heart rate measure during 20 secs
    HRM_MSG_CMD_SLEEP_SAMPLE_STOP,
    HRM_MSG_CMD_SLEEP_SAMPLE_START,         // 90 secs each 5 mins
    HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP,
    HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START,    // run continuosly from start to the end.
    HRM_MSG_CMD_WEAR_DETECT_SAMPLE_START,   // beginning of measure
    HRM_MSG_CMD_WEAR_DETECT_SAMPLE_STOP,    // end of measure      
    HRM_MSG_CMD_MONITOR,
    HRM_MSG_CMD_GPIO_ISR,                   // GPIO interrupt
} hrm_msg_cmd_t;

/**
 * @brief Definitaion for RY_NFC message format.
 */
typedef struct {
    uint32_t  type;
    union {
        monitor_msg_t monitor;
        void* p_data;
    }data;
} ry_hrm_msg_t;


/*
 * FUNCTIONS
 *******************************************************************************
 */
/**
 * @brief   hrm thread task
 *
 * @param   None
 *
 * @return  mode: initial mode
 *                0: auto, else: manual
 */
void hrm_thread_entry(uint32_t mode);


/**
 * @brief   API to init HRM module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t hrm_init(void);

/**
 * @brief   API to get_hrm_working_mode
 *
 * @param   None
 *
 * @return  hrm working mode, refer to hrm_mode_t
 */
uint8_t get_hrm_working_mode(void);

/**
 * @brief   API to set_hrm_working_mode
 *
 * @param   hrm working mode, refer to hrm_mode_t
 *
 * @return  None
 */
void set_hrm_working_mode(uint8_t working_mode);

/**
 * @brief   API to get_hrm_weared_status
 *
 * @param   None
 *
 * @return  hrm weared status
 */
uint8_t get_hrm_weared_status(void);

/**
 * @brief   API to get_hrm_st_is_prepare
 *
 * @param   None
 *
 * @return  hrm weared status
 */
uint8_t hrm_st_is_prepare(void);

/**
 * @brief   API to clear_hrm_st_prepare
 *
 * @param   None
 *
 * @return  None
 */
void clear_hrm_st_prepare(void);

/**
 * @brief   API to ry_hrm_msg_send
 *
 * @param   msg_type: msg type
 *          data: pointer to the msg data
 *
 * @return  None
 */
void ry_hrm_msg_send(u8_t msg_type, void* data);

#endif //__HRM_H__
