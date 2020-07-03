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
 * DAL independent message queue implementation for Android (can be used under Linux too)
 */
#ifndef COMPANION_DEVICE
#include <phNxpLog.h>
#include <phDal4Nfc_messageQueueLib.h>
#include <phOsalNfc.h>
#ifdef ANDROID
#include <pthread.h>
#include "semaphore.h"

#else

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

typedef struct phDal4Nfc_Queue {
    SemaphoreHandle_t  pSemaphore;
    SemaphoreHandle_t nCriticalSectionMutex;
    QueueHandle_t      pQueue;
}phDal4Nfc_Queue_t;

#endif

/*******************************************************************************
**
** Function         phDal4Nfc_msgget
**
** Description      Allocates message queue
**
** Parameters       Ignored, included only for Linux queue API compatibility
**
** Returns          (int) value of pQueue if successful
**                  -1, if failed to allocate memory or to init mutex
**
*******************************************************************************/
intptr_t phDal4Nfc_msgget()
{
    phDal4Nfc_Queue_t *pMsgQ;
    NFCSTATUS wStatus;
    pMsgQ = (phDal4Nfc_Queue_t *)phOsalNfc_GetMemory(sizeof(phDal4Nfc_Queue_t));
    configASSERT( pMsgQ);
    pMsgQ->pQueue = xQueueCreate( ( UBaseType_t ) configTML_QUEUE_LENGTH, sizeof( phLibNfc_Message_t ) );
    configASSERT( pMsgQ->pQueue );
    if (pMsgQ->pQueue == NULL)
    {
        phOsalNfc_FreeMemory(pMsgQ);
        return -1;
    }
    wStatus = phOsalNfc_CreateSemaphore(&pMsgQ->pSemaphore, 0);
    if (wStatus != NFCSTATUS_SUCCESS)
    {
        vQueueDelete(pMsgQ->pQueue);
        phOsalNfc_FreeMemory(pMsgQ);
        return -1;
    }

    wStatus = phOsalNfc_CreateMutex(&pMsgQ->nCriticalSectionMutex);
    if (wStatus != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_DeleteSemaphore(&pMsgQ->pSemaphore);
        pMsgQ->pSemaphore = NULL;
        vQueueDelete(pMsgQ->pQueue);
        phOsalNfc_FreeMemory(pMsgQ);
        return -1;
    }

    return ((intptr_t) pMsgQ);
}

/*******************************************************************************
**
** Function         phDal4Nfc_msgrelease
**
** Description      Releases message queue
**
** Parameters       msqid - message queue handle
**
** Returns          None
**
*******************************************************************************/
void phDal4Nfc_msgrelease(intptr_t msqid)
{
    phDal4Nfc_Queue_t * pMsgQ = (phDal4Nfc_Queue_t *)msqid;

    if(pMsgQ != NULL)
    {
        if(pMsgQ->pSemaphore != NULL)
        {
            phOsalNfc_ProduceSemaphore(pMsgQ->pSemaphore);
            phOsalNfc_Delay(10);
            phOsalNfc_DeleteSemaphore(&pMsgQ->pSemaphore);
            pMsgQ->pSemaphore = NULL;
        }
        if(pMsgQ->nCriticalSectionMutex != NULL)
        {
            phOsalNfc_UnlockMutex(pMsgQ->nCriticalSectionMutex);
            phOsalNfc_Delay(10);
            phOsalNfc_DeleteMutex(&pMsgQ->nCriticalSectionMutex);
            pMsgQ->nCriticalSectionMutex = NULL;
        }
        if(pMsgQ->pQueue != NULL)
        {
            vQueueDelete(pMsgQ->pQueue);
            pMsgQ->pQueue = NULL;
        }
        phOsalNfc_FreeMemory(pMsgQ);
        pMsgQ = NULL;
    }
    return;
}

/*******************************************************************************
**
** Function         phDal4Nfc_msgsnd
**
** Description      Sends a message to the queue. The message will be added at the end of
**                  the queue as appropriate for FIFO policy
**
** Parameters       msqid  - message queue handle
**                  msg    - message to be sent
**                  msgflg - ignored
**
** Returns          0,  if successful
**                  -1, if invalid parameter passed or failed to allocate memory
**
*******************************************************************************/
intptr_t phDal4Nfc_msgsnd(intptr_t msqid, phLibNfc_Message_t * msg, int msgflg)
{
    int returnVal = errQUEUE_FULL;
    phDal4Nfc_Queue_t *pMsgQ = (phDal4Nfc_Queue_t *)msqid;
    UNUSED(msgflg);
    if ((pMsgQ == NULL) || (pMsgQ->pSemaphore == NULL) || (pMsgQ->pQueue == NULL) || (msg == NULL) )
        return -1;
    phOsalNfc_LockMutex(pMsgQ->nCriticalSectionMutex);
    returnVal = xQueueSendToBack( pMsgQ->pQueue, msg, tmrNO_DELAY );
    phOsalNfc_UnlockMutex(pMsgQ->nCriticalSectionMutex);
    if(returnVal == pdTRUE)
        return 0;
    else
        return -1;
}

/*******************************************************************************
**
** Function         phDal4Nfc_msgrcv
**
** Description      Gets the oldest message from the queue.
**                  If the queue is empty the function waits (blocks on a mutex)
**                  until a message is posted to the queue with phDal4Nfc_msgsnd.
**
** Parameters       msqid  - message queue handle
**                  msg    - container for the message to be received
**                  msgtyp - ignored
**                  msgflg - ignored
**
** Returns          0,  if successful
**                  -1, if invalid parameter passed
**
*******************************************************************************/
int phDal4Nfc_msgrcv(intptr_t msqid, phLibNfc_Message_t * msg, long msgtyp, int msgflg)
{
    UNUSED(msgflg);
    UNUSED(msgtyp);
    int returnVal = FALSE;
    phDal4Nfc_Queue_t *pMsgQ = (phDal4Nfc_Queue_t *)msqid;
    if ((pMsgQ == NULL) || (pMsgQ->pSemaphore == NULL) || (pMsgQ->pQueue == NULL) || (msg == NULL))
        return -1;

    returnVal = xQueueReceive( pMsgQ->pQueue, msg, portMAX_DELAY );
    if(returnVal)
        return 0;
    else
        return -1;
}

intptr_t phDal4Nfc_queuesize(intptr_t msqid)
{
    phDal4Nfc_Queue_t *pMsgQ = (phDal4Nfc_Queue_t *)msqid;
    if (pMsgQ == NULL)
        return -1;

    return uxQueueMessagesWaiting(pMsgQ->pQueue);
}

#endif
