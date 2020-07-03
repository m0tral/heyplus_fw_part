/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#ifndef __RY_TIMER_H__
#define __RY_TIMER_H__

#include "ry_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************/

/*
 * The Timer could not be created due to a
 * system error */
#define RY_ERR_TIMER_CREATE                         (0X00E0)

/*
 * The Timer could not be started due to a
 * system error or invalid handle */
#define RY_ERR_TIMER_START                          (0X00E1)

/*
 * The Timer could not be stopped due to a
 * system error or invalid handle */
#define RY_ERR_TIMER_STOP                           (0X00E2)

/*
 * The Timer could not be deleted due to a
 * system error or invalid handle */
#define RY_ERR_TIMER_DELETE                         (0X00E3)

/*
 * The Timer could not be deleted due to a
 * system error or invalid handle */
#define RY_ERR_TIMER_RESET                          (0X00E4)


/*
 * Invalid timer ID type.This ID used indicate timer creation is failed */
#define RY_ERR_TIMER_ID_INVALID                     (0xFFFF)

/*
 * OSAL timer message .This message type will be posted to
 * calling application thread.*/
#define RY_ERR_TIMER_MSG                            (0x315)

/*
 * Timer callback interface which will be called once registered timer
 * time out expires.
 *        timerId  - Timer Id for which callback is called.
 *        userData - Parameter to be passed to the callback function
 */
typedef void (*ry_timer_cb_t)(void *userData);

/*
 * States in which a OSAL timer exist.
 */
typedef enum
{
    TIMER_IDLE    = 0,      /* Indicates Initial state of timer */
    TIMER_RUNNING = 1,      /* Indicate timer state when started */
    TIMER_STOPPED = 2       /* Indicates timer state when stopped */
} ry_timer_state_t;   /* Variable representing State of timer */

/*
 **Timer Handle structure containing details of a timer.
 */
typedef struct
{
    u32_t timerId;                                      /* ID of the timer */
    void *handle;                                       /* Handle of the timer */
    ry_timer_cb_t cb;                                  /* Timer callback function to be invoked */
    void *userdata;                                     /* Parameter to be passed to the callback function */
    ry_timer_state_t state;                            /* Timer states */
    u8_t  autoReload;                                   /* Auto Reload */
    //phLibNfc_Message_t tOsalMessage;                    /* Osal Timer message posted on User Thread */
    //phOsalNfc_DeferedCallInfo_t tDeferedCallInfo;       /* Deferred Call structure to Invoke Callback function */
} ry_timer_t;     /* Variables for Structure Instance and Structure Ptr */



typedef struct 
{
	u8_t flag;
	char *name;
	u32_t tick;
	void (*timeout_CB)(void *parameter);
	void *data;
	
}ry_timer_parm;



/**
 * @brief   API of timer create
 *
 * @param   bAutoReload - 1 indicating the timer will be auto loaded, 0 indicating once. 
 *
 * @return  Created timer ID
 */
u32_t ry_timer_create(ry_timer_parm* parm);

/**
 * @brief   API to start the specified timer
 *
 * @param   timerId - Valid timer ID obtained during timer creation
 * @param   timeout - Requested timeout in milliseconds
 * @param   cb - application callback interface to be called when timer expires
 * @param   userdata - caller context, to be passed to the application callback function
 *
 * @return  Status
 */
ry_sts_t ry_timer_start(u32_t timerId, u32_t timeout, ry_timer_cb_t cb, void *userdata);

/**
 * @brief   API to stop the specified timer
 *
 * @param   timerId - Valid timer ID obtained during timer creation
 *
 * @return  Status
 */
ry_sts_t ry_timer_stop(u32_t timerId);

/**
 * @brief   API to delete the specified timer
 *
 * @param   timerId - Valid timer ID obtained during timer creation
 *
 * @return  Status
 */
ry_sts_t ry_timer_delete(u32_t timerId);

/**
 * @brief   Deletes all previously created timers
 *          Allows to delete previously created timers. In case timer is running,
 *          it is first stopped and then deleted
 *
 * @param   None
 *
 * @return  None
 */

bool ry_timer_isRunning(uint32_t timerId);

void ry_timer_cleanup(void);

/**
 * @brief   Find an available timer id
 *
 * @param   None
 *
 * @return  Available timer id
 */
u32_t ry_timer_check_available_timer(void);

/**
 * @brief   Checks the requested timer is present or not
 *
 * @param   pObjectHandle - timer context
 *
 * @return  Status
 */
ry_sts_t ry_timer_check_presence(void *pObjectHandle);


#ifdef __cplusplus
}
#endif /*  C++ Compilation guard */

#endif  /* __RY_TIMER_H__ */
