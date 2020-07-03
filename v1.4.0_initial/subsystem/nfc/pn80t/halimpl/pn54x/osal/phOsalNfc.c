/*
 * Copyright (C) 2010-2015 NXP Semiconductors
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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#if defined(ANDROID) || defined(COMPANION_DEVICE)
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#endif
#include <string.h>
#include <time.h>
#include <phOsalNfc.h>
#if defined(ANDROID) || defined(COMPANION_DEVICE)
#else
//#include "port.h"
#endif
#include "phOsalNfc_Internal.h"

#if defined(ANDROID) || defined(COMPANION_DEVICE)
/*
*************************** Function Definitions ******************************
*/

/*!
 * \brief Allocates memory.
 *        This function attempts to allocate \a size bytes on the heap and
 *        returns a pointer to the allocated block.
 *
 * \param size size of the memory block to be allocated on the heap.
 *
 * \return pointer to allocated memory block or NULL in case of error.
 */
void* phOsalNfc_GetMemory(uint32_t dwSize)
{
    return (void *) malloc(dwSize);
}


/*!
 * \brief Frees allocated memory block.
 *        This function deallocates memory region pointed to by \a pMem.
 *
 * \param pMem pointer to memory block to be freed.
 */
void phOsalNfc_FreeMemory(void *pMem)
{
    /* Check whether a null pointer is passed */
    if(NULL != pMem)
    {
        free(pMem);
        pMem = NULL;
    }
}


int32_t phOsalNfc_MemCompare(const void *pDest, const void *pSrc, uint32_t dwSize)
{

    return memcmp(pDest, pSrc, dwSize);
}

void phOsalNfc_SetMemory(void *pMem, uint8_t bVal, uint32_t dwSize)
{
	memset(pMem, bVal, dwSize);
}


void phOsalNfc_MemCopy( void *pDest, const void *pSrc, uint32_t dwSize )
{
	memcpy(pDest, pSrc, dwSize);
}


/*===============================================*/



NFCSTATUS phOsalNfc_CreateSemaphore(void        **hSemaphore,
                                    uint8_t     bInitialValue)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = NULL;

    /* Check whether user passed valid input parameters */
    if( (NULL == hSemaphore) )
    {
        wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Fill the Semaphore Handle structure */
        pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t *)phOsalNfc_GetMemory(sizeof(phOsalNfc_sOsalSemaphore_t));
        if( pSemaphoreHandle == NULL )
        {
            wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
            return wCreateStatus;
        }

        if(sem_init(&pSemaphoreHandle->ObjectHandle, 0, (int32_t) bInitialValue) == -1)
        {
            wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
        }
        else
        {
            /* Return the semaphore handle to the caller function */
            *hSemaphore = (void *)pSemaphoreHandle;
        }
    }
    return wCreateStatus;
}

NFCSTATUS phOsalNfc_ProduceSemaphore(void *hSemaphore)
{
    NFCSTATUS wReleaseStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) hSemaphore;

	/* Check whether user passed valid parameter */
	if (pSemaphoreHandle != NULL)
	{
		/* Parameters are Semaphore handle,ReleaseCount and Pointer to previous count */
		if(sem_post(&pSemaphoreHandle->ObjectHandle))
		{
			wReleaseStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_PRODUCE_ERROR);
		}
	}
	else
	{
		wReleaseStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

	return wReleaseStatus;
}

NFCSTATUS phOsalNfc_ConsumeSemaphore(void *hSemaphore)
{
    NFCSTATUS wConsumeStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) hSemaphore;

	/* Check whether user passed valid parameter */
	if (pSemaphoreHandle != NULL)
	{
		/* Wait till the semaphore object is released */
		if(sem_wait(&pSemaphoreHandle->ObjectHandle))
		{
			wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CONSUME_ERROR);
		}
	}
	else
	{
		wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

	return wConsumeStatus;
}

NFCSTATUS phOsalNfc_DeleteSemaphore(void **hSemaphore)
{
    NFCSTATUS wDeletionStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) *hSemaphore;

	/* Check whether OSAL is initialized and user has passed a valid pointer */
	if(pSemaphoreHandle != NULL)
	{
		/* Check whether function was successful */
		if(!sem_destroy(&pSemaphoreHandle->ObjectHandle))
		{
			phOsalNfc_FreeMemory(pSemaphoreHandle);
			*hSemaphore = NULL;
		}
		else
		{
			wDeletionStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_DELETE_ERROR);
		}
	}
	else
	{
		wDeletionStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wDeletionStatus;
}


NFCSTATUS phOsalNfc_CreateMutex(void **hMutex)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle;

	if(NULL != hMutex)
	{
		pMutexHandle = (phOsalNfc_sOsalMutex_t *)phOsalNfc_GetMemory(sizeof(phOsalNfc_sOsalMutex_t));
		if( pMutexHandle == NULL )
		{
			wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
			return wCreateStatus;
		}
		if(pthread_mutex_init(&pMutexHandle->ObjectHandle, NULL) == -1){
			 wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_CREATION_ERROR);
		}else{
			 *hMutex = (void *) pMutexHandle;
		}
	}
	else
	{
		wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wCreateStatus;
}

NFCSTATUS phOsalNfc_LockMutex(void *hMutex)
{
    NFCSTATUS wLockStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t *)hMutex;

	/* Check whether handle provided by user is valid */
	if ( pMutexHandle != NULL)
	{
		/* Wait till the mutex object is unlocked */
		pthread_mutex_lock(&pMutexHandle->ObjectHandle);
	}
	else
	{
		wLockStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wLockStatus;
}

NFCSTATUS phOsalNfc_UnlockMutex(void *hMutex)
{
    NFCSTATUS wUnlockStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t *)hMutex;

	/* Check whether the handle provided by user is valid */
	if (pMutexHandle != NULL)
	{
		pthread_mutex_unlock(&pMutexHandle->ObjectHandle);
	}
	else
	{
		wUnlockStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wUnlockStatus;
}

NFCSTATUS phOsalNfc_DeleteMutex(void **hMutex)
{
    NFCSTATUS wDeletionStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t*) *hMutex;

	/* Check whether the handle provided by user is valid */
	if (pMutexHandle != NULL)
	{
		pthread_mutex_destroy(&pMutexHandle->ObjectHandle);
		phOsalNfc_FreeMemory(pMutexHandle);
		*hMutex = NULL;
	}
	else
	{
		wDeletionStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wDeletionStatus;
}
void phOsalNfc_Delay(uint32_t dwDelay)
{
	// convert dwDelay to microseconds from milliseconds
	usleep(dwDelay *1000);
}

#else

/*
*************************** Function Definitions ******************************
*/

/*!
 * \brief Allocates memory.
 *        This function attempts to allocate \a size bytes on the heap and
 *        returns a pointer to the allocated block.
 *
 * \param size size of the memory block to be allocated on the heap.
 *
 * \return pointer to allocated memory block or NULL in case of error.
 */
void* phOsalNfc_GetMemory(uint32_t dwSize)
{
    return (void *) pvPortMalloc(dwSize);
}

/*!
 * \brief Frees allocated memory block.
 *        This function deallocates memory region pointed to by \a pMem.
 *
 * \param pMem pointer to memory block to be freed.
 */
void phOsalNfc_FreeMemory(void *pMem)
{
    /* Check whether a null pointer is passed */
    if(NULL != pMem)
    {
        vPortFree(pMem);
        pMem = NULL;
    }
}


int32_t phOsalNfc_MemCompare(const void *pDest, const void *pSrc, uint32_t dwSize)
{

    return memcmp(pDest, pSrc, dwSize);
}

void phOsalNfc_SetMemory(void *pMem, uint8_t bVal, uint32_t dwSize)
{
	memset(pMem, bVal, dwSize);
}


void phOsalNfc_MemCopy( void *pDest, const void *pSrc, uint32_t dwSize )
{
	memcpy(pDest, pSrc, dwSize);
}


/*===============================================*/



NFCSTATUS phOsalNfc_CreateSemaphore(void        **hSemaphore,
                                    uint8_t     bInitialValue)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = NULL;

    /* Check whether user passed valid input parameters */
    if( (NULL == hSemaphore) )
    {
        wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
    }
    else
    {
        /* Fill the Semaphore Handle structure */
        pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t *)phOsalNfc_GetMemory(sizeof(phOsalNfc_sOsalSemaphore_t));
        if( pSemaphoreHandle == NULL )
        {
            wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
            return wCreateStatus;
        }

        pSemaphoreHandle->ObjectHandle = NULL;
        pSemaphoreHandle->ObjectHandle = xSemaphoreCreateCounting(1, (uint32_t)bInitialValue);
        if(NULL == pSemaphoreHandle->ObjectHandle)
        {
            wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
            phOsalNfc_FreeMemory(pSemaphoreHandle);
        }
        else
        {
            /* Return the semaphore handle to the caller function */
            *hSemaphore = (void *)pSemaphoreHandle;
        }
    }
    return wCreateStatus;
}

NFCSTATUS phOsalNfc_ProduceSemaphore(void *hSemaphore)
{
    NFCSTATUS wReleaseStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) hSemaphore;
    BaseType_t xHigherPriorityTaskWoken;

	/* Check whether user passed valid parameter */
	if (pSemaphoreHandle != NULL)
	{
        //if(phPlatform_Is_Irq_Context())
        if (xPortIsInsideInterrupt())
        {
            if(!(xSemaphoreGiveFromISR(pSemaphoreHandle->ObjectHandle, &xHigherPriorityTaskWoken)))
            {
                wReleaseStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_PRODUCE_ERROR);
            }
            else
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        else
        {
            phOsalNfc_Delay(3);
            if(!(xSemaphoreGive(pSemaphoreHandle->ObjectHandle)))
            {
                wReleaseStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_PRODUCE_ERROR);
            }
        }
	}
	else
	{
		wReleaseStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

	return wReleaseStatus;
}

NFCSTATUS phOsalNfc_ConsumeSemaphore(void *hSemaphore)
{
    NFCSTATUS wConsumeStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) hSemaphore;
    BaseType_t xHigherPriorityTaskWoken;

	/* Check whether user passed valid parameter */
	if (pSemaphoreHandle != NULL)
	{
		/* Wait till the semaphore object is released */
        //if(phPlatform_Is_Irq_Context())
        if (xPortIsInsideInterrupt())
        {
            if(!(xSemaphoreTakeFromISR(pSemaphoreHandle->ObjectHandle, &xHigherPriorityTaskWoken)))
            {
                wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CONSUME_ERROR);
            }
            else
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        else
        {
            phOsalNfc_Delay(3);
            if(!(xSemaphoreTake(pSemaphoreHandle->ObjectHandle, portMAX_DELAY)))
            {
                wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CONSUME_ERROR);
            }
        }
	}
	else
	{
		wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

	return wConsumeStatus;
}

NFCSTATUS phOsalNfc_ConsumeSemaphore_WithTimeout(void *hSemaphore, uint32_t delay)
{
    NFCSTATUS wConsumeStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) hSemaphore;
    BaseType_t xHigherPriorityTaskWoken;

    /* Check whether user passed valid parameter */
    if (pSemaphoreHandle != NULL)
    {
        /* Wait till the semaphore object is released */
        //if(phPlatform_Is_Irq_Context())
		if (xPortIsInsideInterrupt())
        {
            if(!(xSemaphoreTakeFromISR(pSemaphoreHandle->ObjectHandle, &xHigherPriorityTaskWoken)))
            {
                wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CONSUME_ERROR);
            }
            else
            {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
        else
        {
		    phOsalNfc_Delay(3);
            if(!(xSemaphoreTake(pSemaphoreHandle->ObjectHandle, delay/portTICK_PERIOD_MS)))
            {
                wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CONSUME_ERROR);
            }
        }
	}
	else
	{
		wConsumeStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

	return wConsumeStatus;
}

NFCSTATUS phOsalNfc_DeleteSemaphore(void **hSemaphore)
{
    NFCSTATUS wDeletionStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalSemaphore_t *pSemaphoreHandle = (phOsalNfc_sOsalSemaphore_t*) *hSemaphore;

	/* Check whether OSAL is initialized and user has passed a valid pointer */
	if(pSemaphoreHandle != NULL)
	{
		/* Check whether function was successful */
        vSemaphoreDelete(pSemaphoreHandle->ObjectHandle);
			phOsalNfc_FreeMemory(pSemaphoreHandle);
			*hSemaphore = NULL;
	}
	else
	{
		wDeletionStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wDeletionStatus;
}


NFCSTATUS phOsalNfc_CreateMutex(void **hMutex)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle;

	if(NULL != hMutex)
	{
		pMutexHandle = (phOsalNfc_sOsalMutex_t *)phOsalNfc_GetMemory(sizeof(phOsalNfc_sOsalMutex_t));
		if( pMutexHandle == NULL )
		{
			wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
			return wCreateStatus;
		}
	    pMutexHandle->ObjectHandle = NULL;
	    pMutexHandle->ObjectHandle = xSemaphoreCreateMutex();
		if(NULL == pMutexHandle->ObjectHandle){
			 wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_CREATION_ERROR);
			 phOsalNfc_FreeMemory(pMutexHandle);
		}else{
			 *hMutex = (void *) pMutexHandle;
		}
	}
	else
	{
		wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wCreateStatus;
}

NFCSTATUS phOsalNfc_CreateRecursiveMutex(void **hMutex)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle;

	if(NULL != hMutex)
	{
		pMutexHandle = (phOsalNfc_sOsalMutex_t *)phOsalNfc_GetMemory(sizeof(phOsalNfc_sOsalMutex_t));
		if( pMutexHandle == NULL )
		{
			wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_SEMAPHORE_CREATION_ERROR);
			return wCreateStatus;
		}
	    pMutexHandle->ObjectHandle = NULL;
	    pMutexHandle->ObjectHandle = xSemaphoreCreateRecursiveMutex();
		if(NULL == pMutexHandle->ObjectHandle){
			 wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_CREATION_ERROR);
			 phOsalNfc_FreeMemory(pMutexHandle);
		}else{
			 *hMutex = (void *) pMutexHandle;
		}
	}
	else
	{
		wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wCreateStatus;
}

NFCSTATUS phOsalNfc_LockMutex(void *hMutex)
{
    NFCSTATUS wLockStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t *)hMutex;

	/* Check whether handle provided by user is valid */
	if ( pMutexHandle != NULL)
	{
		/* Wait till the mutex object is unlocked */
		if(!(xSemaphoreTake(pMutexHandle->ObjectHandle, portMAX_DELAY)))
		{
			wLockStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_LOCK_ERROR);
		}
	}
	else
	{
		wLockStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wLockStatus;
}

NFCSTATUS phOsalNfc_LockRecursiveMutex(void *hMutex)
{
    NFCSTATUS wLockStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t *)hMutex;

	/* Check whether handle provided by user is valid */
	if ( pMutexHandle != NULL)
	{
		/* Wait till the mutex object is unlocked */
		if(!(xSemaphoreTakeRecursive(pMutexHandle->ObjectHandle, portMAX_DELAY)))
		{
			wLockStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_LOCK_ERROR);
		}
	}
	else
	{
		wLockStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wLockStatus;
}

NFCSTATUS phOsalNfc_UnlockMutex(void *hMutex)
{
    NFCSTATUS wUnlockStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t *)hMutex;

	/* Check whether the handle provided by user is valid */
	if (pMutexHandle != NULL)
	{
		if(!(xSemaphoreGive(pMutexHandle->ObjectHandle)))
		{
			wUnlockStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_UNLOCK_ERROR);
		}
	}
	else
	{
		wUnlockStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wUnlockStatus;
}

NFCSTATUS phOsalNfc_UnlockRecursiveMutex(void *hMutex)
{
    NFCSTATUS wUnlockStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t *)hMutex;

	/* Check whether the handle provided by user is valid */
	if (pMutexHandle != NULL)
	{
		if(!(xSemaphoreGiveRecursive(pMutexHandle->ObjectHandle)))
		{
			wUnlockStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_MUTEX_UNLOCK_ERROR);
		}
	}
	else
	{
		wUnlockStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wUnlockStatus;
}

NFCSTATUS phOsalNfc_DeleteMutex(void **hMutex)
{
    NFCSTATUS wDeletionStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_sOsalMutex_t *pMutexHandle = (phOsalNfc_sOsalMutex_t*) *hMutex;

	/* Check whether the handle provided by user is valid */
	if (pMutexHandle != NULL)
	{
		vSemaphoreDelete(pMutexHandle->ObjectHandle);
		phOsalNfc_FreeMemory(pMutexHandle);
		*hMutex = NULL;
	}
	else
	{
		wDeletionStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}

    return wDeletionStatus;
}

void phOsalNfc_Delay(uint32_t dwDelay)
{
	vTaskDelay(dwDelay/portTICK_PERIOD_MS);
}

void phOsalNfc_GetTickCount(unsigned long * pTickCount)
{
	*pTickCount = xTaskGetTickCount();
}

#endif/*COMPANION_DEVICE*/
