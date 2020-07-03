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
#include "ryos.h"
#include "ry_utils.h"
#include "ryos_timer.h"



#define RY_MAX_TIMER   (50U) /* MAX_NCI_POOL_SIZE + MAX_QUICK_POOL_SIZE + MAX_NFA_POOL_SIZE + 3 for direct calls
                                  (1 each by HAL, TML retry, BLE timeout; excluding 1 used by FW Dnld Module as its
                                  mutually exclusive to most of the NFA stack) */
ry_timer_t  apTimerInfo[RY_MAX_TIMER];


/*
 * Defines the base address for generating timerid.
 */
#define RY_TIMER_BASE_ADDRESS                      (100U)

/*
 *  Defines the value for invalid timerid returned during timeSetEvent
 */
#define RY_TIMER_ID_ZERO                           (0x00)


/*
 * Invalid timer ID type. This ID used indicate timer creation is failed 
 */
#define RY_TIMER_ID_INVALID                        (0xFFFF)


#if (OS_TYPE == OS_FREERTOS)
#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include "portable.h"

//#include "ry_timer.h"



static void ry_timer_deferredCall (void *pParams);
static TimerCallbackFunction_t ry_timer_expired(TimerHandle_t hTimerHandle);

/*
 *************************** Function Definitions ******************************
 */

/**
 * @brief   API of timer create
 *
 * @param   bAutoReload - 1 indicating the timer will be auto loaded, 0 indicating once. 
 *
 * @return  Created timer ID
 */
u32_t ry_timer_create(ry_timer_parm* parm)
{
    /* dwTimerId is also used as an index at which timer object can be stored */
    u32_t dwTimerId = RY_TIMER_ID_INVALID;

    ry_timer_t *pTimerHandle;
    
    /* Timer needs to be initialized for timer usage */
    dwTimerId = ry_timer_check_available_timer();
    
    /* Check whether timers are available, if yes create a timer handle structure */
    if( (RY_TIMER_ID_ZERO != dwTimerId) && (dwTimerId <= RY_MAX_TIMER) )
    {
        pTimerHandle = (ry_timer_t *)&apTimerInfo[dwTimerId-1];
        /* Build the Timer Id to be returned to Caller Function */
        dwTimerId += RY_TIMER_BASE_ADDRESS;

        /*value of xTimerPeriodInTicks arbitrary set to 1 as it will be changed in phOsalNfc_Timer_Start according to the dwRegTimeCnt parameter*/
        pTimerHandle->handle = xTimerCreate(parm->name, 1, parm->flag, (void*) dwTimerId, (TimerCallbackFunction_t)ry_timer_expired);

        if(pTimerHandle->handle != NULL){
            /* Set the state to indicate timer is ready */
            pTimerHandle->state = TIMER_IDLE;
            /* Store the Timer Id which shall act as flag during check for timer availability */
            pTimerHandle->timerId = dwTimerId;
            /* Set the reload value */
            pTimerHandle->autoReload = parm->flag;
        }
        else
        {
            RY_ASSERT(pTimerHandle->handle);
            dwTimerId = RY_TIMER_ID_INVALID;
        }
    }
    else
    {
        RY_ASSERT(dwTimerId);
        dwTimerId = RY_TIMER_ID_INVALID;
    }

    /* Timer ID invalid can be due to Uninitialized state,Non availability of Timer */
    return dwTimerId;
}

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
ry_sts_t ry_timer_start(u32_t timerId, u32_t timeout, ry_timer_cb_t cb, void *userdata)
{
    ry_sts_t status = RY_SUCC;

    u32_t tickvalue;
    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    /* Retrieve the index at which the timer handle structure is stored */
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    /*convert timeout parameter in tick value*/
    tickvalue = timeout/portTICK_PERIOD_MS;


    if (xTimerChangePeriod(pTimerHandle->handle, tickvalue, 100) == pdPASS) {
        /* OSAL Module needs to be initialized for timer usage */
        /* Check whether the handle provided by user is valid */
        if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
        		(NULL != cb) && (NULL != pTimerHandle->handle)) {
        	pTimerHandle->cb       = cb;
        	pTimerHandle->userdata = userdata;
            pTimerHandle->state    = TIMER_RUNNING;

            /* Arm the timer */
            if (pdPASS != xTimerStart(pTimerHandle->handle,0)) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_START);
            }
        } else {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
        }
    } else {
    	status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_START);
    }

    return status;
}

/**
 * @brief   API to stop the specified timer
 *
 * @param   timerId - Valid timer ID obtained during timer creation
 *
 * @return  Status
 */
ry_sts_t ry_timer_stop(u32_t timerId)
{
    ry_sts_t status = RY_SUCC;

    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];

    /* OSAL Module and Timer needs to be initialized for timer usage */
    /* Check whether the TimerId provided by user is valid */
    if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
            (pTimerHandle->state != TIMER_IDLE) ) {

        /* Stop the timer only if the callback has not been invoked */
        if ((pTimerHandle->state == TIMER_RUNNING) || (pTimerHandle->autoReload)) {
            if(pdPASS != xTimerStop(pTimerHandle->handle, 0)) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_STOP);
            } else {
                /* Change the state of timer to Stopped */
                pTimerHandle->state = TIMER_STOPPED;
                //LOG_DEBUG("Timer stop succ.\r\n");
            }
        } else {
            //LOG_DEBUG("Timer not running, state = %d\r\n", pTimerHandle->state);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}



void ry_timer_all_stop(void)
{
    ry_sts_t status = RY_SUCC;
    u32_t dwIndex;
    ry_timer_t *pTimerHandle;


    for (dwIndex = 0; dwIndex < RY_MAX_TIMER; dwIndex++) {
        pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
        /* OSAL Module and Timer needs to be initialized for timer usage */
        /* Check whether the TimerId provided by user is valid */
        if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
                (pTimerHandle->state != TIMER_IDLE) ) {

            /* Stop the timer only if the callback has not been invoked */
            if ((pTimerHandle->state == TIMER_RUNNING) || (pTimerHandle->autoReload)) {
                if(pdPASS != xTimerStop(pTimerHandle->handle, 0)) {
                    status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_STOP);
                } else {
                    /* Change the state of timer to Stopped */
                    pTimerHandle->state = TIMER_STOPPED;
                    //LOG_DEBUG("Timer stop succ.\r\n");
                }
            } else {
                LOG_DEBUG("Timer not running, state = %d\r\n", pTimerHandle->state);
            }
        } else {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
        }
    }

}




/**
 * @brief   API to delete the specified timer
 *
 * @param   timerId - Valid timer ID obtained during timer creation
 *
 * @return  Status
 */
ry_sts_t ry_timer_delete(u32_t timerId)
{
    ry_sts_t status = RY_SUCC;

    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    
    /* OSAL Module and Timer needs to be initialized for timer usage */

    /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
    if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId)
            && (RY_SUCC == ry_timer_check_presence(pTimerHandle))) {
        /* Cancel the timer before deleting */
        if (xTimerDelete(pTimerHandle->handle, 0) != pdPASS) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_DELETE);
        }
        /* Clear Timer structure used to store timer related data */
        ry_memset(pTimerHandle, (uint8_t)0x00, sizeof(ry_timer_t));
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }
    
    return status;
}
 
bool ry_timer_isRunning(uint32_t timerId)
{
    ry_sts_t status = RY_SUCC;

    bool isRunning = false;
    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    
    /* OSAL Module and Timer needs to be initialized for timer usage */

    /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
    if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId)
            && (RY_SUCC == ry_timer_check_presence(pTimerHandle)))
    {
        /* Cancel the timer before deleting */
        if (pTimerHandle->state == TIMER_RUNNING)
        {
            isRunning = true;
        }
    } else {
    }

    return isRunning;
}
/**
 * @brief   Deletes all previously created timers
 *          Allows to delete previously created timers. In case timer is running,
 *          it is first stopped and then deleted
 *
 * @param   None
 *
 * @return  None
 */
void ry_timer_cleanup(void)
{
    /* Delete all timers */
    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    
    for(dwIndex = 0; dwIndex < RY_MAX_TIMER; dwIndex++) {
        pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
        /* OSAL Module and Timer needs to be initialized for timer usage */

        /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
        if( (0x00 != pTimerHandle->timerId)
                && (RY_SUCC == ry_timer_check_presence(pTimerHandle))) {
            /* Cancel the timer before deleting */
        	if (xTimerDelete(pTimerHandle->handle, 0) != pdPASS) {
                //NXPLOG_TML_E("timer %d delete error!", dwIndex);
            }
            /* Clear Timer structure used to store timer related data */
            ry_memset(pTimerHandle, (u8_t)0x00, sizeof(ry_timer_t));
        }
    }

    return;
}

/**
 * @brief   Invokes the timer callback function after timer expiration.
 *          Shall invoke the callback function registered by the timer caller function
 *
 * @param   pParams - parameters indicating the ID of the timer
 *
 * @return  None
 */
static void ry_timer_deferredCall (void *pParams)
{
    /* Retrieve the timer id from the parameter */
    uint32_t dwIndex;
    ry_timer_t *pTimerHandle;
    if(NULL != pParams)
    {
        /* Retrieve the index at which the timer handle structure is stored */
        dwIndex = (uintptr_t)pParams - RY_TIMER_BASE_ADDRESS - 0x01;
        pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
        if(pTimerHandle->cb != NULL)
        {
            /* Invoke the callback function with osal Timer ID */
            pTimerHandle->cb(pTimerHandle->userdata);
        }
    }

    return;
}


/**
 * @brief   posts message upon expiration of timer
 *          Shall be invoked when any one timer is expired
 *          Shall post message on user thread to invoke respective
 *          callback function provided by the caller of Timer function
 *
 * @param   handle - Freertos timer handle
 *
 * @return  None
 */
static TimerCallbackFunction_t ry_timer_expired (TimerHandle_t handle)
{
    u32_t dwIndex;
    u32_t timerId;
    ry_timer_t *pTimerHandle;

    timerId = (u32_t) pvTimerGetTimerID(handle);
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    /* Timer is stopped when callback function is invoked */
    pTimerHandle->state = TIMER_STOPPED;

    //pTimerHandle->tDeferedCallInfo.pDeferedCall = &phOsalNfc_DeferredCall;
    //pTimerHandle->tDeferedCallInfo.pParam = (void *)timerId;

    //pTimerHandle->tOsalMessage.eMsgType = PH_LIBNFC_DEFERREDCALL_MSG;
    //pTimerHandle->tOsalMessage.pMsgData = (void *)&pTimerHandle->tDeferedCallInfo;


    /* Post a message on the queue to invoke the function */
    //phOsalNfc_PostTimerMsg ((phLibNfc_Message_t *)&pTimerHandle->tOsalMessage);
    if (pTimerHandle->cb) {
        pTimerHandle->cb(pTimerHandle->userdata);
    }

    return 0;
}

/**
 * @brief   Find an available timer id
 *
 * @param   None
 *
 * @return  Available timer id
 */
u32_t ry_timer_check_available_timer(void)
{
    /* Variable used to store the index at which the object structure details
       can be stored. Initialize it as not available. */
    u32_t dwIndex = 0x00;
    u32_t dwRetval = 0x00;

    /* Check whether Timer object can be created */
     for(dwIndex = 0x00; ((dwIndex < RY_MAX_TIMER) && (0x00 == dwRetval)); dwIndex++) {
         if(!(apTimerInfo[dwIndex].timerId)) {
             dwRetval = (dwIndex + 0x01);
         }
     }

     return (dwRetval);
}


/**
 * @brief   Checks the requested timer is present or not
 *
 * @param   pObjectHandle - timer context
 *
 * @return  Status
 */
ry_sts_t ry_timer_check_presence(void *pObjectHandle)
{
    u32_t dwIndex;
    ry_sts_t status = RY_SUCC;

    for(dwIndex = 0x00; ( (dwIndex < RY_MAX_TIMER) &&
            (status != RY_SUCC)); dwIndex++)
    {
        /* For Timer, check whether the requested handle is present or not */
        if( ((&apTimerInfo[dwIndex]) == (ry_timer_t *)pObjectHandle) &&
                (apTimerInfo[dwIndex].timerId) )
        {
            status = RY_SUCC;
        }
    }
    return status;
}

#elif (OS_TYPE == OS_RTTHREAD)

u32_t ry_timer_create(ry_timer_parm* parm)
{
	/* dwTimerId is also used as an index at which timer object can be stored */
	u32_t dwTimerId = RY_TIMER_ID_INVALID;

	ry_timer_t *pTimerHandle;
	/* Timer needs to be initialized for timer usage */

	dwTimerId = ry_timer_check_available_timer();
	/* Check whether timers are available, if yes create a timer handle structure */
	if( (RY_TIMER_ID_ZERO != dwTimerId) && (dwTimerId <= RY_MAX_TIMER) )
	{
		pTimerHandle = (ry_timer_t *)&apTimerInfo[dwTimerId-1];
		/* Build the Timer Id to be returned to Caller Function */
		dwTimerId += RY_TIMER_BASE_ADDRESS;

		/*value of xTimerPeriodInTicks arbitrary set to 1 as it will be changed in phOsalNfc_Timer_Start according to the dwRegTimeCnt parameter*/
		pTimerHandle->handle = rt_timer_create(parm->name,parm->timeout_CB,parm->data,parm->tick,parm->flag);

		if(pTimerHandle->handle != NULL){
			/* Set the state to indicate timer is ready */
			pTimerHandle->state = TIMER_IDLE;
			/* Store the Timer Id which shall act as flag during check for timer availability */
			pTimerHandle->timerId = dwTimerId;
			/* Set the reload value */
			pTimerHandle->autoReload = parm->flag;
		}
		else
		{
			RY_ASSERT(pTimerHandle->handle);
			dwTimerId = RY_TIMER_ID_INVALID;
		}
	}
	else
	{
		RY_ASSERT(dwTimerId);
		dwTimerId = RY_TIMER_ID_INVALID;
	}

	/* Timer ID invalid can be due to Uninitialized state,Non availability of Timer */
	return dwTimerId;
}

/**
 * @brief	API to start the specified timer
 *
 * @param	timerId - Valid timer ID obtained during timer creation
 * @param	timeout - Requested timeout in milliseconds
 * @param	cb - application callback interface to be called when timer expires
 * @param	userdata - caller context, to be passed to the application callback function
 *
 * @return	Status
 */
ry_sts_t ry_timer_start(u32_t timerId, u32_t timeout, ry_timer_cb_t cb, void *userdata)
{
	ry_sts_t status = RY_SUCC;

	u32_t tickvalue;
	u32_t dwIndex;
	ry_timer_t *pTimerHandle;
	/* Retrieve the index at which the timer handle structure is stored */
	dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
	pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
	/*convert timeout parameter in tick value*/
	tickvalue = timeout;


	/* OSAL Module needs to be initialized for timer usage */
	/* Check whether the handle provided by user is valid */
	if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
			(NULL != cb) && (NULL != pTimerHandle->handle)) {
		pTimerHandle->cb	   = cb;
		pTimerHandle->userdata = userdata;
		pTimerHandle->state    = TIMER_RUNNING;

		/* Arm the timer */
		if (RT_EOK != rt_timer_start(pTimerHandle->handle)) {
			status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_START);
		}
	} else {
		status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
	}

	return status;
}

ry_sts_t ry_timer_stop(u32_t timerId)
{
    ry_sts_t status = RY_SUCC;

    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];

    /* OSAL Module and Timer needs to be initialized for timer usage */
    /* Check whether the TimerId provided by user is valid */
    if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId) &&
            (pTimerHandle->state != TIMER_IDLE) ) {

        /* Stop the timer only if the callback has not been invoked */
        if ((pTimerHandle->state == TIMER_RUNNING) || (pTimerHandle->autoReload)) {
            if(RT_EOK != rt_timer_stop(pTimerHandle->handle)) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_STOP);
            } else {
                /* Change the state of timer to Stopped */
                pTimerHandle->state = TIMER_STOPPED;
                //LOG_DEBUG("Timer stop succ.\r\n");
            }
        } else {
            LOG_DEBUG("Timer not running, state = %d\r\n", pTimerHandle->state);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ry_timer_delete(u32_t timerId)
{
    ry_sts_t status = RY_SUCC;

    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    
    /* OSAL Module and Timer needs to be initialized for timer usage */

    /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
    if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId)
            && (RY_SUCC == ry_timer_check_presence(pTimerHandle))) {
        /* Cancel the timer before deleting */
        if (rt_timer_delete(pTimerHandle->handle) != RT_EOK) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_TIMER_DELETE);
        }
        /* Clear Timer structure used to store timer related data */
        ry_memset(pTimerHandle, (uint8_t)0x00, sizeof(ry_timer_t));
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }
    
    return status;
}


bool ry_timer_isRunning(uint32_t timerId)
{
    ry_sts_t status = RY_SUCC;

    bool isRunning = false;
    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    
    /* OSAL Module and Timer needs to be initialized for timer usage */

    /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
    if( (dwIndex < RY_MAX_TIMER) && (0x00 != pTimerHandle->timerId)
            && (RY_SUCC == ry_timer_check_presence(pTimerHandle)))
    {
        /* Cancel the timer before deleting */
        if (pTimerHandle->state == TIMER_RUNNING)
        {
            isRunning = true;
        }
    } else {
    }

    return isRunning;
}

void ry_timer_cleanup(void)
{
    /* Delete all timers */
    u32_t dwIndex;
    ry_timer_t *pTimerHandle;
    
    for(dwIndex = 0; dwIndex < RY_MAX_TIMER; dwIndex++) {
        pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
        /* OSAL Module and Timer needs to be initialized for timer usage */

        /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
        if( (0x00 != pTimerHandle->timerId)
                && (RY_SUCC == ry_timer_check_presence(pTimerHandle))) {
            /* Cancel the timer before deleting */
        	if (rt_timer_delete(pTimerHandle->handle) != RT_EOK) {
                //NXPLOG_TML_E("timer %d delete error!", dwIndex);
            }
            /* Clear Timer structure used to store timer related data */
            ry_memset(pTimerHandle, (u8_t)0x00, sizeof(ry_timer_t));
        }
    }

    return;
}


static void ry_timer_deferredCall (void *pParams)
{
    /* Retrieve the timer id from the parameter */
    uint32_t dwIndex;
    ry_timer_t *pTimerHandle;
    if(NULL != pParams)
    {
        /* Retrieve the index at which the timer handle structure is stored */
        dwIndex = (uintptr_t)pParams - RY_TIMER_BASE_ADDRESS - 0x01;
        pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
        if(pTimerHandle->cb != NULL)
        {
            /* Invoke the callback function with osal Timer ID */
            pTimerHandle->cb((uintptr_t)pParams, pTimerHandle->userdata);
        }
    }

    return;
}

static void* ry_timer_expired (void * handle)
{
    //u32_t dwIndex;
    //u32_t timerId;
    //ry_timer_t *pTimerHandle;

    //timerId = (u32_t) pvTimerGetTimerID(handle);
   //dwIndex = timerId - RY_TIMER_BASE_ADDRESS - 0x01;
    //pTimerHandle = (ry_timer_t *)&apTimerInfo[dwIndex];
    /* Timer is stopped when callback function is invoked */
    //pTimerHandle->state = TIMER_STOPPED;

    //pTimerHandle->tDeferedCallInfo.pDeferedCall = &phOsalNfc_DeferredCall;
    //pTimerHandle->tDeferedCallInfo.pParam = (void *)timerId;

    //pTimerHandle->tOsalMessage.eMsgType = PH_LIBNFC_DEFERREDCALL_MSG;
    //pTimerHandle->tOsalMessage.pMsgData = (void *)&pTimerHandle->tDeferedCallInfo;


    /* Post a message on the queue to invoke the function */
    //phOsalNfc_PostTimerMsg ((phLibNfc_Message_t *)&pTimerHandle->tOsalMessage);
    //pTimerHandle->cb(timerId, pTimerHandle->userdata);

    return 0;
}

u32_t ry_timer_check_available_timer(void)
{
    /* Variable used to store the index at which the object structure details
       can be stored. Initialize it as not available. */
    u32_t dwIndex = 0x00;
    u32_t dwRetval = 0x00;

    /* Check whether Timer object can be created */
     for(dwIndex = 0x00; ((dwIndex < RY_MAX_TIMER) && (0x00 == dwRetval)); dwIndex++) {
         if(!(apTimerInfo[dwIndex].timerId)) {
             dwRetval = (dwIndex + 0x01);
         }
     }

     return (dwRetval);
}

ry_sts_t ry_timer_check_presence(void *pObjectHandle)
{
    u32_t dwIndex;
    ry_sts_t status = RY_SUCC;

    for(dwIndex = 0x00; ( (dwIndex < RY_MAX_TIMER) &&
            (status != RY_SUCC)); dwIndex++)
    {
        /* For Timer, check whether the requested handle is present or not */
        if( ((&apTimerInfo[dwIndex]) == (ry_timer_t *)pObjectHandle) &&
                (apTimerInfo[dwIndex].timerId) )
        {
            status = RY_SUCC;
        }
    }
    return status;
}








#endif
