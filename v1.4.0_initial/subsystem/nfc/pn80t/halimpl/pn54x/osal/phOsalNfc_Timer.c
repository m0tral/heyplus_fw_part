/*
 * Copyright (C) 2015 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * OSAL Implementation for Timers.
 */
#ifndef COMPANION_DEVICE
#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include <phOsalNfc_Timer.h>
#include <signal.h>
#include <phNfcTypes.h>
#include <phNfcCommon.h>
#include <phNxpNciHal.h>
#include <phNxpLog.h>

#define PH_NFC_MAX_TIMER (16U) /* MAX_NCI_POOL_SIZE + MAX_QUICK_POOL_SIZE + MAX_NFA_POOL_SIZE + 3 for direct calls
                                  (1 each by HAL, TML retry, BLE timeout; excluding 1 used by FW Dnld Module as its
                                  mutually exclusive to most of the NFA stack) */
static phOsalNfc_TimerHandle_t         apTimerInfo[PH_NFC_MAX_TIMER];

extern phNxpNciHal_Control_t nxpncihal_ctrl;


/*
 * Defines the base address for generating timerid.
 */
#define PH_NFC_TIMER_BASE_ADDRESS                   (100U)

/*
 *  Defines the value for invalid timerid returned during timeSetEvent
 */
#define PH_NFC_TIMER_ID_ZERO                        (0x00)


/*
 * Invalid timer ID type. This ID used indicate timer creation is failed */
#define PH_NFC_TIMER_ID_INVALID                     (0xFFFF)

/* Forward declarations */
#if 0 /* Unused */
static void phOsalNfc_PostTimerMsg(phLibNfc_Message_t *pMsg);
#endif
static void phOsalNfc_DeferredCall (void *pParams);
static TimerCallbackFunction_t phOsalNfc_Timer_Expired(TimerHandle_t hTimerHandle);

/*
 *************************** Function Definitions ******************************
 */

/*******************************************************************************
**
** Function         phOsalNfc_Timer_Create
**
** Description      Creates a timer which shall call back the specified function when the timer expires
**                  Fails if OSAL module is not initialized or timers are already occupied
**
** Parameters       bAutoReload
**                  If bAutoReload is set to TRUE then the timer will
**                  expire repeatedly with a frequency set by the xTimerPeriodInTicks parameter.
**                  Timer restarts automatically.
**                  If bAutoReload is set to FALSE then the timer will be a one-shot timer and
**                  enter the dormant state after it expires.
**
** Returns          TimerId
**                  TimerId value of PH_OSALNFC_TIMER_ID_INVALID indicates that timer is not created                -
**
*******************************************************************************/
uint32_t phOsalNfc_Timer_Create(uint8_t bAutoReload)
{
    /* dwTimerId is also used as an index at which timer object can be stored */
    uint32_t dwTimerId = PH_OSALNFC_TIMER_ID_INVALID;

    phOsalNfc_TimerHandle_t *pTimerHandle;
    /* Timer needs to be initialized for timer usage */

         dwTimerId = phUtilNfc_CheckForAvailableTimer();
        /* Check whether timers are available, if yes create a timer handle structure */
        if( (PH_NFC_TIMER_ID_ZERO != dwTimerId) && (dwTimerId <= PH_NFC_MAX_TIMER) )
        {
            pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwTimerId-1];
            /* Build the Timer Id to be returned to Caller Function */
            dwTimerId += PH_NFC_TIMER_BASE_ADDRESS;

            /*value of xTimerPeriodInTicks arbitrary set to 1 as it will be changed in phOsalNfc_Timer_Start according to the dwRegTimeCnt parameter*/
            pTimerHandle->hTimerHandle=xTimerCreate("Timer",1, bAutoReload,(void*) dwTimerId, (TimerCallbackFunction_t) (phOsalNfc_Timer_Expired));

            if(pTimerHandle->hTimerHandle != NULL){
                /* Set the state to indicate timer is ready */
                pTimerHandle->eState = eTimerIdle;
                /* Store the Timer Id which shall act as flag during check for timer availability */
                pTimerHandle->TimerId = dwTimerId;
            }
            else
            {
                configASSERT(pTimerHandle->hTimerHandle);
            	dwTimerId = PH_NFC_TIMER_ID_INVALID;
            }
        }
        else
        {
            configASSERT(dwTimerId);
            dwTimerId = PH_NFC_TIMER_ID_INVALID;
        }

    /* Timer ID invalid can be due to Uninitialized state,Non availability of Timer */
    return dwTimerId;
}

/*******************************************************************************
**
** Function         phOsalNfc_Timer_Start
**
** Description      Starts the requested, already created, timer
**                  If the timer is already running, timer stops and restarts with the new timeout value
**                  and new callback function in case any ??????
**                  Creates a timer which shall call back the specified function when the timer expires
**
** Parameters       dwTimerId             - valid timer ID obtained during timer creation
**                  dwRegTimeCnt          - requested timeout in milliseconds
**                  pApplication_callback - application callback interface to be called when timer expires
**                  pContext              - caller context, to be passed to the application callback function
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS            - the operation was successful
**                  NFCSTATUS_NOT_INITIALISED    - OSAL Module is not initialized
**                  NFCSTATUS_INVALID_PARAMETER  - invalid parameter passed to the function
**                  PH_OSALNFC_TIMER_START_ERROR - timer could not be created due to system error
**
*******************************************************************************/
NFCSTATUS phOsalNfc_Timer_Start(uint32_t dwTimerId, uint32_t dwRegTimeCnt, pphOsalNfc_TimerCallbck_t pApplication_callback, void *pContext)
{
    NFCSTATUS wStartStatus= NFCSTATUS_SUCCESS;

    uint32_t tickvalue;
    uint32_t dwIndex;
    phOsalNfc_TimerHandle_t *pTimerHandle;
    /* Retrieve the index at which the timer handle structure is stored */
    dwIndex = dwTimerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwIndex];
    /*convert timeout parameter in tick value*/
    tickvalue=dwRegTimeCnt/portTICK_PERIOD_MS;


    if(xTimerChangePeriod(pTimerHandle->hTimerHandle,tickvalue,100) == pdPASS){

    /* OSAL Module needs to be initialized for timer usage */
        /* Check whether the handle provided by user is valid */


        if( (dwIndex < PH_NFC_MAX_TIMER) && (0x00 != pTimerHandle->TimerId) &&
        		(NULL != pApplication_callback) && (NULL != pTimerHandle->hTimerHandle))
        {
        	pTimerHandle->Application_callback = pApplication_callback;
        	pTimerHandle->pContext = pContext;
            pTimerHandle->eState = eTimerRunning;

            /* Arm the timer */
            if(pdPASS != xTimerStart(pTimerHandle->hTimerHandle,0))
            {
                wStartStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_START_ERROR);
            }
        }

        else
        {
            wStartStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
        }
    }

    else
    {
    	wStartStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_START_ERROR);
    }



    return wStartStatus;
}

/*******************************************************************************
**
** Function         phOsalNfc_Timer_Stop
**
** Description      Stops already started timer
**                  Allows to stop running timer. In case timer is stopped, timer callback
**                  will not be notified any more
**
** Parameters       dwTimerId             - valid timer ID obtained during timer creation
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS            - the operation was successful
**                  NFCSTATUS_NOT_INITIALISED    - OSAL Module is not initialized
**                  NFCSTATUS_INVALID_PARAMETER  - invalid parameter passed to the function
**                  PH_OSALNFC_TIMER_STOP_ERROR  - timer could not be stopped due to system error
**
*******************************************************************************/
NFCSTATUS phOsalNfc_Timer_Stop(uint32_t dwTimerId)
{
    NFCSTATUS wStopStatus=NFCSTATUS_SUCCESS;

    uint32_t dwIndex;
    phOsalNfc_TimerHandle_t *pTimerHandle;
    dwIndex = dwTimerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwIndex];
    /* OSAL Module and Timer needs to be initialized for timer usage */
        /* Check whether the TimerId provided by user is valid */
        if( (dwIndex < PH_NFC_MAX_TIMER) && (0x00 != pTimerHandle->TimerId) &&
                (pTimerHandle->eState != eTimerIdle) )
        {
            /* Stop the timer only if the callback has not been invoked */
            if(pTimerHandle->eState == eTimerRunning)
            {
            	if(pdPASS != xTimerStop(pTimerHandle->hTimerHandle,0))
                {
                    wStopStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_STOP_ERROR);
                }
                else
                {
                    /* Change the state of timer to Stopped */
                    pTimerHandle->eState = eTimerStopped;
                }
            }
        }
        else
        {
            wStopStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
        }

    return wStopStatus;
}

/*******************************************************************************
**
** Function         phOsalNfc_Timer_Delete
**
** Description      Deletes previously created timer
**                  Allows to delete previously created timer. In case timer is running,
**                  it is first stopped and then deleted
**
** Parameters       dwTimerId             - valid timer ID obtained during timer creation
**
** Returns          NFC status:
**                  NFCSTATUS_SUCCESS             - the operation was successful
**                  NFCSTATUS_NOT_INITIALISED     - OSAL Module is not initialized
**                  NFCSTATUS_INVALID_PARAMETER   - invalid parameter passed to the function
**                  PH_OSALNFC_TIMER_DELETE_ERROR - timer could not be stopped due to system error
**
*******************************************************************************/
NFCSTATUS phOsalNfc_Timer_Delete(uint32_t dwTimerId)
{
    NFCSTATUS wDeleteStatus = NFCSTATUS_SUCCESS;

    uint32_t dwIndex;
    phOsalNfc_TimerHandle_t *pTimerHandle;
    dwIndex = dwTimerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwIndex];
    /* OSAL Module and Timer needs to be initialized for timer usage */

        /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
        if( (dwIndex < PH_NFC_MAX_TIMER) && (0x00 != pTimerHandle->TimerId)
                && (NFCSTATUS_SUCCESS == phOsalNfc_CheckTimerPresence(pTimerHandle))
        )
        {
            /* Cancel the timer before deleting */
            if(xTimerDelete(pTimerHandle->hTimerHandle,0) != pdPASS)
            {
                wDeleteStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_TIMER_DELETE_ERROR);
            }
            /* Clear Timer structure used to store timer related data */
            memset(pTimerHandle,(uint8_t)0x00,sizeof(phOsalNfc_TimerHandle_t));
        }
        else
        {
            wDeleteStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
        }
    return wDeleteStatus;
}

/*******************************************************************************
**
** Function         phOsalNfc_Timer_Cleanup
**
** Description      Deletes all previously created timers
**                  Allows to delete previously created timers. In case timer is running,
**                  it is first stopped and then deleted
**
** Parameters       None
**
** Returns          None
**
*******************************************************************************/
void phOsalNfc_Timer_Cleanup(void)
{
    /* Delete all timers */
    uint32_t dwIndex;
    phOsalNfc_TimerHandle_t *pTimerHandle;
    for(dwIndex = 0; dwIndex < PH_NFC_MAX_TIMER; dwIndex++)
    {
        pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwIndex];
        /* OSAL Module and Timer needs to be initialized for timer usage */

        /* Check whether the TimerId passed by user is valid and Deregistering of timer is successful */
        if( (0x00 != pTimerHandle->TimerId)
                && (NFCSTATUS_SUCCESS == phOsalNfc_CheckTimerPresence(pTimerHandle))
        )
        {
            /* Cancel the timer before deleting */
        	if(xTimerDelete(pTimerHandle->hTimerHandle,0) != pdPASS)
            {
                NXPLOG_TML_E("timer %d delete error!", dwIndex);
            }
            /* Clear Timer structure used to store timer related data */
            memset(pTimerHandle,(uint8_t)0x00,sizeof(phOsalNfc_TimerHandle_t));
        }
    }

    return;
}

/*******************************************************************************
**
** Function         phOsalNfc_DeferredCall
**
** Description      Invokes the timer callback function after timer expiration.
**                  Shall invoke the callback function registered by the timer caller function
**
** Parameters       pParams - parameters indicating the ID of the timer
**
** Returns          None                -
**
*******************************************************************************/
static void phOsalNfc_DeferredCall (void *pParams)
{
    /* Retrieve the timer id from the parameter */
    uint32_t dwIndex;
    phOsalNfc_TimerHandle_t *pTimerHandle;
    if(NULL != pParams)
    {
        /* Retrieve the index at which the timer handle structure is stored */
        dwIndex = (uintptr_t)pParams - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
        pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwIndex];
        if(pTimerHandle->Application_callback != NULL)
        {
            /* Invoke the callback function with osal Timer ID */
            pTimerHandle->Application_callback((uintptr_t)pParams, pTimerHandle->pContext);
        }
    }

    return;
}

#if 0 /* Instead of posting message, directly the timer callback is invoked */
/*******************************************************************************
**
** Function         phOsalNfc_PostTimerMsg
**
** Description      Posts message on the user thread
**                  Shall be invoked upon expiration of a timer
**                  Shall post message on user thread through which timer callback function shall be invoked
**
** Parameters       pMsg - pointer to the message structure posted on user thread
**
** Returns          None                -
**
*******************************************************************************/
static void phOsalNfc_PostTimerMsg(phLibNfc_Message_t *pMsg)
{

    (void)phDal4Nfc_msgsnd(nxpncihal_ctrl.gDrvCfg.nClientId/*gpphOsalNfc_Context->dwCallbackThreadID*/, pMsg,0);

    return;
}
#endif

/*******************************************************************************
**
** Function         phOsalNfc_Timer_Expired
**
** Description      posts message upon expiration of timer
**                  Shall be invoked when any one timer is expired
**                  Shall post message on user thread to invoke respective
**                  callback function provided by the caller of Timer function
**
** Returns          None
**
*******************************************************************************/
static TimerCallbackFunction_t phOsalNfc_Timer_Expired (TimerHandle_t hTimerHandle)
{
    uint32_t dwIndex;
    uint32_t TimerId;
    phOsalNfc_TimerHandle_t *pTimerHandle;

    TimerId = (uint32_t) pvTimerGetTimerID(hTimerHandle);
    dwIndex = TimerId - PH_NFC_TIMER_BASE_ADDRESS - 0x01;
    pTimerHandle = (phOsalNfc_TimerHandle_t *)&apTimerInfo[dwIndex];
    /* Timer is stopped when callback function is invoked */
    pTimerHandle->eState = eTimerStopped;

    pTimerHandle->tDeferedCallInfo.pDeferedCall = &phOsalNfc_DeferredCall;
    pTimerHandle->tDeferedCallInfo.pParam = (void *)TimerId;

    pTimerHandle->tOsalMessage.eMsgType = PH_LIBNFC_DEFERREDCALL_MSG;
    pTimerHandle->tOsalMessage.pMsgData = (void *)&pTimerHandle->tDeferedCallInfo;


    /* Post a message on the queue to invoke the function */
    //phOsalNfc_PostTimerMsg ((phLibNfc_Message_t *)&pTimerHandle->tOsalMessage);
    pTimerHandle->Application_callback(TimerId, pTimerHandle->pContext);

    return 0;
}


/*******************************************************************************
**
** Function         phUtilNfc_CheckForAvailableTimer
**
** Description      Find an available timer id
**
** Parameters       void
**
** Returns          Available timer id
**
*******************************************************************************/
uint32_t phUtilNfc_CheckForAvailableTimer(void)
{
    /* Variable used to store the index at which the object structure details
       can be stored. Initialize it as not available. */
    uint32_t dwIndex = 0x00;
    uint32_t dwRetval = 0x00;


    /* Check whether Timer object can be created */
     for(dwIndex = 0x00;
             ( (dwIndex < PH_NFC_MAX_TIMER) && (0x00 == dwRetval) ); dwIndex++)
     {
         if(!(apTimerInfo[dwIndex].TimerId))
         {
             dwRetval = (dwIndex + 0x01);
         }
     }

     return (dwRetval);

}

/*******************************************************************************
**
** Function         phOsalNfc_CheckTimerPresence
**
** Description      Checks the requested timer is present or not
**
** Parameters       pObjectHandle - timer context
**
** Returns          NFCSTATUS_SUCCESS if found
**                  Other value if not found
**
*******************************************************************************/
NFCSTATUS phOsalNfc_CheckTimerPresence(void *pObjectHandle)
{
    uint32_t dwIndex;
    NFCSTATUS wRegisterStatus = NFCSTATUS_INVALID_PARAMETER;

    for(dwIndex = 0x00; ( (dwIndex < PH_NFC_MAX_TIMER) &&
            (wRegisterStatus != NFCSTATUS_SUCCESS) ); dwIndex++)
    {
        /* For Timer, check whether the requested handle is present or not */
        if( ((&apTimerInfo[dwIndex]) ==
                (phOsalNfc_TimerHandle_t *)pObjectHandle) &&
                (apTimerInfo[dwIndex].TimerId) )
        {
            wRegisterStatus = NFCSTATUS_SUCCESS;
        }
    }
    return wRegisterStatus;

}

/*******************************************************************************
**
** Function         phOsalNfc_IsTimersRunning
**
** Description      To know if any timers are running
**
** Parameters       void
**
** Returns          TRUE if any timers are running
**                  FALSE otherwise
**
*******************************************************************************/
bool_t phOsalNfc_IsTimersRunning(void)
{
    uint32_t dwIndex;

    for(dwIndex = 0x00; (dwIndex < PH_NFC_MAX_TIMER); dwIndex++)
    {
        /* For Timer, check whether the handle is valid and state is running*/
        if(apTimerInfo[dwIndex].TimerId &&  apTimerInfo[dwIndex].eState == eTimerRunning )
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif
