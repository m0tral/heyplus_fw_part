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

/**
 * \file  phOsalNfc.h
 *  \brief OSAL header files related to memory, debug, random, semaphore and mutex functions.
 */

#ifndef PHOSALNFC_H
#define PHOSALNFC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
************************* Include Files ****************************************
*/    
#include <WearableCoreSDK_BuildConfig.h>

#ifndef PHNFCTYPES_H
#   include <phNfcTypes.h>
#endif
#ifndef PHNFCSTATUS_H
#   include <phNfcStatus.h>
#endif
#ifndef PHNFCCOMPID_H
#   include <phNfcCompId.h>
#endif

/*
****************************** Macro Definitions ******************************
*/

/** \addtogroup grp_osal_err
 * @{ */

/**
 * A new semaphore could not be created due to a system error. */
#define PH_OSALNFC_SEMAPHORE_CREATION_ERROR                     (0x00F0)

/**
 * The given semaphore could not be released due to a system error. */
#define PH_OSALNFC_SEMAPHORE_PRODUCE_ERROR                      (0x00F1)

/**
 * The given semaphore could not be consumed due to a system error. */
#define PH_OSALNFC_SEMAPHORE_CONSUME_ERROR                      (0x00F2)

/**
 * The given semaphore could not be deleted due to a system error. */
#define PH_OSALNFC_SEMAPHORE_DELETE_ERROR                       (0x00F3)

/**
 * A new mutex could not be created due to a system error. */
#define PH_OSALNFC_MUTEX_CREATION_ERROR                         (0x00F4)

/**
 * The given Mutex could not be locked due to a system error. */
#define PH_OSALNFC_MUTEX_LOCK_ERROR                             (0x00F5)

/**
 * The given Mutex could not be unlocked due to a system error. */
#define PH_OSALNFC_MUTEX_UNLOCK_ERROR                           (0x00F6)

/**
 * The given mutex could not be deleted due to a system error. */
#define PH_OSALNFC_MUTEX_DELETE_ERROR                           (0x00F7)

/**
 * A new mutex could not be created due to a system error. */
#define PH_OSALNFC_THREAD_CREATION_ERROR                        (0x00F8)

/**
 * The given thread could not be suspended due to a system error. */
#define PH_OSALNFC_THREAD_SUSPEND_ERROR                         (0x00F9)

/**
 * The given thread could not be resumed due to a system error. */
#define PH_OSALNFC_THREAD_RESUME_ERROR                          (0x00FA)

/**
 * The given thread could not be deleted due to a system error. */
#define PH_OSALNFC_THREAD_DELETE_ERROR                          (0x00FB)

/**
 * The given thread could not be deleted due to a system error. */
#define PH_OSALNFC_THREAD_SETPRIORITY_ERROR                     (0x00FC)

/** @} */

/*
***************************Globals,Structure and Enumeration ******************
*/

/** \addtogroup grp_osal_thread
 * @{ */

/**
 * Thread Function Pointer Declaration.
 *
 * This points to the function to be invoked upon creation of thread.
 * It is not the immediate thread procedure since the API of this function
 * depends on the OS.
 *
 * This function shall be called within the body of the thread procedure
 * defined by the underlying, OS-depending OSAL implementation.
 *
 */
typedef void (*pphOsalNfc_ThreadFunction_t)(void*);

/** @} */


/** \addtogroup grp_osal_mm
 * @{ */

/**
 * Allocates some memory.
 *
 * \note This function executes successfully without OSAL module initialization.
 *
 * \param[in] dwSize   Size, in uint32_t, to be allocated
 *
 * \retval NON-NULL value:  The memory is successfully allocated ;
 * the return value is a pointer to the allocated memory location
 * \retval NULL:            The operation is not successful.
 *
 */
void * phOsalNfc_GetMemory(uint32_t dwSize);
/**
 * This API allows to free already allocated memory.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pMem  Pointer to the memory block to be deallocated
 */
void   phOsalNfc_FreeMemory(void * pMem);

/**
 * This API allows to delay the current thread execution.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] dwDelay  Duration in milliseconds for which thread execution to be halted.
 */
void phOsalNfc_Delay(uint32_t dwDelay);

/**
 * Compares the values stored in the source memory with the
 * values stored in the destination memory.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pDest     Pointer to the Destination string.
 * \param[in] pSrc      Pointer to the Source string.
 * \param[in] dwSize    Number of bytes to be compared.
 *
 * \retval Zero value:        The comparison is successful,
                    Both the memory areas contains identical values.
 * \retval Non-Zero Value:    The comparison failed, both the memory
 *                  areas are non-identical.
 *
 */
int32_t phOsalNfc_MemCompare(const void *pDest, const void *pSrc, uint32_t dwSize);

/**
 * Sets the given value in the memory locations.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pMem      Pointer to the memory block to be set to a value
 * \param[in] bVal      Value to be set
 * \param[in] dwSize    Number of bytes to be set.
 *
 */
 void phOsalNfc_SetMemory(void *pMem, uint8_t bVal, uint32_t dwSize);
/**
 * Copies the values stored in the source memory to the
 * values stored in the destination memory.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] pDest     Pointer to the Destination Memory
 * \param[in] pSrc      Pointer to the Source Memory
 * \param[in] dwSize    Number of bytes to be copied.
 *
 */
void phOsalNfc_MemCopy( void *pDest, const void *pSrc, uint32_t dwSize );

/** @} */


/** \addtogroup grp_osal_thread
 * @{ */

/**
 * Semaphore Creation.
 * This function creates a semaphore in the underlying system.
 *
 * \param[in,out] hSemaphore The handle: The caller has to prepare a void
 *                           pointer where the handle of semaphore shall be returned.
 *
 * \param[in] bInitialValue   The initial value of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     All semaphores are occupied by other threads.
 * \retval #PH_OSALNFC_SEMAPHORE_CREATION_ERROR  A new semaphore could not be created due to system error.
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_CreateSemaphore(void        **hSemaphore,
                                    uint8_t     bInitialValue);

/**
 * Semaphore-Produce (never waiting).
 *
 * Increment the value of the semaphore.
 * The purpose is to enable a waiting thread ("consumer") to continue
 * if it has been waiting because of the Semaphore value set to zero.
 *
 * \param[in] hSemaphore The handle of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_SEMAPHORE_PRODUCE_ERROR   The semaphore cannot be produced due to a system
 *                                              error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_ProduceSemaphore(void *hSemaphore);

/**
 * Semaphore Consumption (waiting if value is zero).
 *
 * Decrement the value of the semaphore. When the internal value is
 * non-zero, the function continues. If the value is zero however, the
 * function blocks till the semaphore is released.
 *
 * \param[in] hSemaphore The handle of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_SEMAPHORE_CONSUME_ERROR   The semaphore can not be consumed
 *                                              due to a system error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_ConsumeSemaphore(void *hSemaphore);

/**
 * Semaphore Consumption (waiting if value is zero).
 *
 * Decrement the value of the semaphore. When the internal value is
 * non-zero, the function continues. If the value is zero however, the
 * function blocks till the semaphore is released.
 *
 * \param[in] hSemaphore The handle of the Semaphore.
 * \param[in] delay time to wait for
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_SEMAPHORE_CONSUME_ERROR   The semaphore can not be consumed
 *                                              due to a system error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_ConsumeSemaphore_WithTimeout(void *hSemaphore, uint32_t delay);

/**
 * Semaphore Deletion.
 * This function deletes the Semaphore in the underlying OS.
 *
 * \param[in] hSemaphore                    The handle of the Semaphore.
 *
 * \retval #NFCSTATUS_SUCCESS                    The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_SEMAPHORE_DELETE_ERROR    The semaphore can not be deleted
 *                                              due to a system error or invalid handle .
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_DeleteSemaphore(void **hSemaphore);

/**
 * Mutex Creation.
 * This function creates a Mutex in the underlying system.
 *
 * \param[in,out] hMutex     The handle: The caller has to prepare a void
 *                           pointer where the handle of mutex shall be returned.
 *
 * \retval #NFCSTATUS_SUCCESS The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     All Mutexes are occupied by other threads.
 * \retval #PH_OSALNFC_MUTEX_CREATION_ERROR      A new mutex could not be created due to system error.
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_CreateMutex(void **hMutex);

/**
 * Mutex Creation.
 * This function creates a recursive Mutex in the underlying system.
 *
 * \param[in,out] hMutex     The handle: The caller has to prepare a void
 *                           pointer where the handle of mutex shall be returned.
 *
 * \retval #NFCSTATUS_SUCCESS The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER          Parameter passed to the function is not Correct.
 * \retval #NFCSTATUS_INSUFFICIENT_RESOURCES     All Mutexes are occupied by other threads.
 * \retval #PH_OSALNFC_MUTEX_CREATION_ERROR      A new mutex could not be created due to system error.
 * \retval #NFCSTATUS_NOT_INITIALISED           Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_CreateRecursiveMutex(void **hMutex);


/**
 * Mutex Locking.
 * This function locks a mutex used for handling critical section of code.
 * However the function blocks till the Mutex is available to be occupied.
 *
 * \param[in] hMutex                    The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS            The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER  Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_MUTEX_LOCK_ERROR  The mutex cannot be locked due to a system error or invalid handle.
 * \retval #NFCSTATUS_NOT_INITIALISED   Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_LockMutex(void *hMutex);

/**
 * Mutex Locking.
 * This function locks a recursive mutex used for handling critical section of code.
 * However the function blocks till the Mutex is available to be occupied.
 *
 * \param[in] hMutex                    The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS            The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER  Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_MUTEX_LOCK_ERROR  The mutex cannot be locked due to a system error or invalid handle.
 * \retval #NFCSTATUS_NOT_INITIALISED   Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_LockRecursiveMutex(void *hMutex);


/**
 * Mutex Unlocking.
 * This function unlocks a mutex after updating critical section of code.
 *
 * \param[in] hMutex        The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS                The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER      Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_MUTEX_UNLOCK_ERROR    The mutex cannot be locked due to a system error or invalid handle.
 * \retval #NFCSTATUS_NOT_INITIALISED       Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_UnlockMutex(void *hMutex);

/**
 * Mutex Unlocking.
 * This function unlocks a recursive mutex after updating critical section of code.
 *
 * \param[in] hMutex        The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS                The operation was successful.
 * \retval #NFCSTATUS_INVALID_PARAMETER      Parameter passed to the function is not Correct.
 * \retval #PH_OSALNFC_MUTEX_UNLOCK_ERROR    The mutex cannot be locked due to a system error or invalid handle.
 * \retval #NFCSTATUS_NOT_INITIALISED       Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_UnlockRecursiveMutex(void *hMutex);



/**
 * Mutex Deletion.
 * This function deletes a Mutex in the underlying system.
 *
 * \param[in] hMutex        The handle of the Mutex.
 *
 * \retval #NFCSTATUS_SUCCESS                The operation was successful.
 * \retval #PH_OSALNFC_MUTEX_DELETE_ERROR    The mutex cannot be deleted due to a system error or invalid handle.
 * \retval #NFCSTATUS_NOT_INITIALISED       Osal Module is not Initialized.
 *
 */
NFCSTATUS phOsalNfc_DeleteMutex(void **hMutex);


/**
 * Get tick count since scheduler has started.
 *
 *
 * \param[in] tickCount        The address of the variable to which tickcount value will be assigned.
 *
 */
void phOsalNfc_GetTickCount(unsigned long * dwTickCount);

/** @}  */

#ifdef __cplusplus
}
#endif /*  C++ Compilation guard */
#endif /*  PHOSALNFC_H  */
