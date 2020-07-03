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

/**
 * \file  phOsalNfc_Thread.c
 * \brief OSAL Implementation.
 */

/** \addtogroup grp_osal_nfc
    @{
 */
/*
************************* Header Files ****************************************
*/

#include <phOsalNfc_Thread.h>
#include "phOsalNfc_Internal.h"
#if defined(ANDROID) || defined(COMPANION_DEVICE)
#include <pthread.h>
#endif
#if defined(ANDROID) || defined(COMPANION_DEVICE)
/*
****************************** Macro Definitions ******************************
*/
/** \ingroup grp_osal_nfc
    Value indicates Failure to suspend/resume thread */
#define PH_OSALNFC_THREADPROC_FAILURE           (0xFFFFFFFF)

/*
*************************** Function Definitions ******************************
*/

static void * phOsalNfc_ThreadProcedure(void * lpParameter)
{
    phOsalNfc_sOsalThreadHandle_t *pThreadHandle = (phOsalNfc_sOsalThreadHandle_t *)lpParameter;
    pThreadHandle->ThreadFunction(pThreadHandle->Params);
    return PH_OSALNFC_RESET_VALUE;
}

NFCSTATUS phOsalNfc_Thread_Create(void                         **hThread,
                                 pphOsalNfc_ThreadFunction_t   pThreadFunction,
                                 void                          *pParam)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;

    phOsalNfc_sOsalThreadHandle_t *pThreadHandle;

	if( (NULL == hThread) || (NULL == pThreadFunction) )
	{
		wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}
	else
	{
		pThreadHandle = (phOsalNfc_sOsalThreadHandle_t *)phOsalNfc_GetMemory(sizeof(phOsalNfc_sOsalThreadHandle_t));
		if( pThreadHandle == NULL )
		{
			wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_THREAD_CREATION_ERROR);
			return wCreateStatus;
		}
		/* Fill the Threadhandle structure which is needed for threadprocedure function */
		pThreadHandle->ThreadFunction = pThreadFunction;
		pThreadHandle->Params = pParam;
		/* Indicate the thread is created without message queue */
		if(pthread_create(&pThreadHandle->ObjectHandle, NULL, &phOsalNfc_ThreadProcedure,
							pThreadHandle)==0)
		{
			/* Assign the Thread handle structure to the pointer provided by the user */
			*hThread = (void *)pThreadHandle;
		}
		else
		{
			wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_THREAD_CREATION_ERROR);
		}
	}

    return wCreateStatus;
}

/** @} */
#else
/*
****************************** Macro Definitions ******************************
*/
/** \ingroup grp_osal_nfc
    Value indicates Failure to suspend/resume thread */
#define PH_OSALNFC_THREADPROC_FAILURE           (0xFFFFFFFF)

/*
*************************** Function Definitions ******************************
*/

NFCSTATUS phOsalNfc_Thread_Create(void                         **hThread,
                                 pphOsalNfc_ThreadFunction_t   pThreadFunction,
                                 void                          *pParam)
{
    NFCSTATUS wCreateStatus = NFCSTATUS_SUCCESS;
    phOsalNfc_ThreadCreationParams_t *pThreadParams = (phOsalNfc_ThreadCreationParams_t *)pParam;

	if( (NULL == hThread) || (NULL == pThreadFunction) || (NULL == pThreadParams) )
	{
		wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, NFCSTATUS_INVALID_PARAMETER);
	}
	else
	{
		/* Indicate the thread is created without message queue */
		if(pdPASS == xTaskCreate( pThreadFunction, (const char*)&(pThreadParams->taskname[0]), pThreadParams->stackdepth, pThreadParams->pContext, (tskIDLE_PRIORITY + pThreadParams->priority), hThread ))
		{
		    configASSERT( *hThread );
		}
		else
		{
			wCreateStatus = PHNFCSTVAL(CID_NFC_OSAL, PH_OSALNFC_THREAD_CREATION_ERROR);
		}
	}

    return wCreateStatus;
}

NFCSTATUS phOsalNfc_Thread_Delete(void *hThread)
{
    NFCSTATUS wDeletionStatus = NFCSTATUS_SUCCESS;
    vTaskDelete(hThread);
    return wDeletionStatus;
}

void * phOsalNfc_GetTaskHandle(void)
{
	return (xTaskGetCurrentTaskHandle());
}

#endif
/** @} */
