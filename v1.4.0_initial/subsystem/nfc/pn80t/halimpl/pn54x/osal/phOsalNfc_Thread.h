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
 * \file  phOsalNfc_Thread.h
 * \brief OSAL header files related to thread functions.
 */

#ifndef PHOSALNFC_THREAD_H
#define PHOSALNFC_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

/*
************************* Include Files ****************************************
*/

#include <phOsalNfc.h>
#include <phOsalNfc_Internal.h>

#if defined(ANDROID) || defined(COMPANION_DEVICE)
/** \ingroup grp_osal_thread
 *
 * @{ */

/**
 * Thread Creation.
 *
 * This function creates a thread in the underlying system. To delete the
 * created thread use the phOsalNfc_Thread_Delete function.
 *
 *
 * \param[in,out] hThread    The Thread handle: The caller has to prepare a void pointer
 *                           that need not to be initialized.
 *                           The value (content) of the pointer is set by the function.
 *
 * \param[in] pThreadFunction Pointer to a function within the
 *                           implementation that shall be called by the Thread
 *                           procedure. This represents the Thread main function.
 *                           When this function exits the thread exits.
 * \param[in] pParam          A pointer to a user-defined location the thread
 *                           function receives.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     At least one parameter value is invalid.
 * \retval #PH_OSALNFC_THREAD_CREATION_ERROR     A new Thread could not be created due to system error.
 * \retval #NFCSTATUS_NOT_INITIALISED            Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_Thread_Create(void                          **hThread,
                                 pphOsalNfc_ThreadFunction_t    pThreadFunction,
                                 void                           *pParam);
#else

typedef struct phOsalNfc_ThreadCreationParams
{
    uint16_t stackdepth; /* stackdepth in stackwidth units */
    int8_t taskname[TASK_NAME_MAX_LENGTH];
    void* pContext;
    uint16_t priority;
}phOsalNfc_ThreadCreationParams_t;

/** \ingroup grp_osal_thread
 *
 * @{ */

/**
 * Thread Creation.
 *
 * This function creates a thread in the underlying system. To delete the
 * created thread use the phOsalNfc_Thread_Delete function.
 *
 *
 * \param[in,out] hThread    The Thread handle: The caller has to prepare a void pointer
 *                           that need not to be initialized.
 *                           The value (content) of the pointer is set by the function.
 *
 * \param[in] pThreadFunction Pointer to a function within the
 *                           implementation that shall be called by the Thread
 *                           procedure. This represents the Thread main function.
 *                           When this function exits the thread exits.
 * \param[in] pParam          A pointer to a user-defined location the thread
 *                           function receives.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     At least one parameter value is invalid.
 * \retval #PH_OSALNFC_THREAD_CREATION_ERROR     A new Thread could not be created due to system error.
 * \retval #NFCSTATUS_NOT_INITIALISED            Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_Thread_Create(void                          **hThread,
                                 pphOsalNfc_ThreadFunction_t    pThreadFunction,
                                 void                           *pParam);


/**
 * Terminates the thread.
 *
 * This function Terminates the thread passed as a handle.
 *
 * \param[in] hThread The handle of the system object.
 *
 * \retval #NFCSTATUS_SUCCESS                The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER      At least one parameter value is invalid.
 * \retval #PH_OSALNFC_THREAD_DELETE_ERROR   Thread could not be deleted due to system error.
 * \retval #NFCSTATUS_NOT_INITIALISED        Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_Thread_Delete(void *hThread);

/**
 * Create Event.
 *
 *
 *  * \retval void * Task handle as per underlying implementation.
 *
 */
void * phOsalNfc_GetTaskHandle(void);

#endif

#ifdef __cplusplus
}
#endif /*  C++ Compilation guard */
#endif  /*  PHOSALNFC_THREAD_H  */
/** @} */
