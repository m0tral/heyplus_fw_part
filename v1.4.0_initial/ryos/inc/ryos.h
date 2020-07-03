/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#ifndef __RY_OS_H__
#define __RY_OS_H__

#include "ry_type.h"

/**
 *  @brief  Definition for OS Type
 */
#define OS_BAREMETAL            0
#define OS_FREERTOS             1
#define OS_RTTHREAD             2
#define OS_LINUX                3
#define OS_WINDOWS              4

/**
 *  @brief  Definition for RY OS Type
 */
    #define OS_TYPE                 OS_FREERTOS
#ifndef OS_TYPE
	#define OS_TYPE                 OS_RTTHREAD
#else
	#undef OS_TYPE
	#define OS_TYPE                 OS_FREERTOS
#endif

/********************************************************************/

#define THREAD_NAME_MAX_LENGTH 20

/** \addtogroup grp_ryos_err
 * @{ */
/**
 * A new semaphore could not be created due to a system error. */
#define RY_ERR_OS_SEMAPHORE_CREATION                          (0x00F0)

/**
 * The given semaphore could not be released due to a system error. */
#define RY_ERR_SEMAPHORE_POST                                 (0x00F1)

/**
 * The given semaphore could not be consumed due to a system error. */
#define RY_ERR_SEMAPHORE_WAIT                                 (0x00F2)

/**
 * The given semaphore could not be deleted due to a system error. */
#define RY_ERR_SEMAPHORE_DELETE                               (0x00F3)

/**
 * A new mutex could not be created due to a system error. */
#define RY_ERR_MUTEX_CREATION                                 (0x00F4)

/**
 * The given Mutex could not be locked due to a system error. */
#define RY_ERR_MUTEX_LOCK                                     (0x00F5)

/**
 * The given Mutex could not be unlocked due to a system error. */
#define RY_ERR_MUTEX_UNLOCK                                   (0x00F6)

/**
 * The given mutex could not be deleted due to a system error. */
#define RY_ERR_MUTEX_DELETE                                   (0x00F7)

/**
 * A new mutex could not be created due to a system error. */
#define RY_ERR_THREAD_CREATION                                (0x00F8)

/**
 * The given thread could not be suspended due to a system error. */
#define RY_ERR_THREAD_SUSPEND                                 (0x00F9)

/**
 * The given thread could not be resumed due to a system error. */
#define RY_ERR_THREAD_RESUME                                  (0x00FA)

/**
 * The given thread could not be deleted due to a system error. */
#define RY_ERR_THREAD_DELETE                                  (0x00FB)

/**
 * The given thread could not be deleted due to a system error. */
#define RY_ERR_THREAD_SETPRIORITY                             (0x00FC)

#define RY_ERR_QUEUE_CREATION                                 (0x00FD)
#define RY_ERR_QEUEU_DELETE                                   (0x00FE)
#define RY_ERR_QUEUE_SEND                                     (0x00FF)
#define RY_ERR_QUEUE_RECV                                     (0x00EF)
#define RY_ERR_MB_CREATION                                    (0x00EE)
#define RY_ERR_MB_DELETE                                      (0x00ED)
#define RY_ERR_MB_SEND                                        (0x00EC)
#define RY_ERR_MB_RECV                                        (0x00EB)




/** @} */


/**
 *  @brief  Definition for IS Wait type
 */
#define RY_OS_WAIT_FOREVER    0xffffffff
#define RY_OS_NO_WAIT         0

/* Task states returned by eTaskGetState. */
typedef enum
{
	RY_THREAD_ST_RUNNING = 0,	/* A task is querying the state of itself, so must be running. */
	RY_THREAD_ST_READY,			/* The task being queried is in a read or pending ready list. */
	RY_THREAD_ST_BLOCKED,  		/* The task being queried is in the Blocked state. */
	RY_THREAD_ST_SUSPENDED,  	/* The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out. */
	RY_THREAD_ST_DELETED,		/* The task being queried has been deleted, but its TCB has not yet been freed. */
	RY_THREAD_ST_INVALID		/* Used as an 'invalid state' value. */
} ry_thread_state_t;

typedef void ryos_thread_t;
typedef int (*ryos_threadFunc_t)(void* arg);

typedef struct {
    char name[THREAD_NAME_MAX_LENGTH];
    u32_t stackDepth;
    u32_t tick;
    void* arg;
    u16_t priority;
} ryos_threadPara_t;

typedef struct {
    char name[THREAD_NAME_MAX_LENGTH];
    //u32_t stackDepth;
    u32_t tick;
    u32_t value;
    u32_t msg_size;
    u32_t max_msg;
    u8_t flag;
    //void* arg;
    //u16_t priority;
} ryos_ipc_t;


/**
 * @brief   Init RY OS Obstract layer
 *
 * @param   None
 *
 * @return  None
 */
void ryos_init(void);

/**
 * @brief   Start OS scheduler
 *
 * @param   None
 *
 * @return  None
 */
void ryos_start_schedule(void);


#if (OS_TYPE == OS_FREERTOS)


/************Task Related API****************/
ry_sts_t ryos_thread_create(ryos_thread_t** pThreadHandle, ryos_threadFunc_t threadFunc, ryos_threadPara_t* pThreadPara);
ry_sts_t ryos_thread_delete(ryos_thread_t* threadHandle);
void ryos_thread_set_tag(ryos_thread_t* threadHandle, u32_t tag);

u32_t ryos_enter_critical(void);
void ryos_exit_critical(u32_t level);

ry_sts_t ryos_thread_suspend(ryos_thread_t* threadHandle);
ry_sts_t ryos_thread_resume(ryos_thread_t* threadHandle);
void ryos_thread_suspendAll(void);
void ryos_thread_resumeAll(void);
void ryos_get_thread_list(char* buffer);



typedef void ryos_mutex_t; 
ry_sts_t ryos_mutex_create(ryos_mutex_t** handle);
ry_sts_t ryos_mutex_delete(ryos_mutex_t* handle);
ry_sts_t ryos_mutex_unlock(ryos_mutex_t* handle);
ry_sts_t ryos_mutex_lock(ryos_mutex_t* handle);

typedef void ryos_semaphore_t; 
ry_sts_t ryos_semaphore_create(ryos_semaphore_t** handle, u8_t bInitVal);
ry_sts_t ryos_semaphore_delete(ryos_semaphore_t* handle);
ry_sts_t ryos_semaphore_wait(ryos_semaphore_t* handle);
ry_sts_t ryos_semaphore_post(ryos_semaphore_t* handle);


void* ry_malloc(u32_t size);
void* ry_calloc(u32_t n, u32_t size);
void* ry_realloc(void *mem_address, u32_t dwSize);

void  ry_free(void* buffer);
u32_t ryos_get_free_heap(void);

void ry_memcpy(void *pDest, const void *pSrc, uint32_t dwSize);
void ry_memmove(void *pDest, const void *pSrc, uint32_t dwSize);
void ry_memset(void* b, u8_t v, u32_t l);
int  ry_memcmp(void* s1, void* s2, u32_t l);

void ryos_delay(u32_t dwDelay);
void ryos_delay_ms(u32_t ms);
//void ryos_get_tick_count(unsigned long * dwTickCount);
u32_t ryos_get_tick_count(void);
u32_t ryos_get_ms(void);
u32_t ryos_utc_now(void);
u32_t ryos_utc_now_notimezone(void);
    
int ryos_utc_set(u32_t utc);
u32_t ryos_rtc_now(void);
u32_t ryos_rtc_now_notimezone(void);

void set_timezone(uint32_t deviation);
uint32_t get_timezone(void);
uint32_t get_timezone_china(void);


/*!
 * \brief Get free heap memory size..
 *
 * \param None.
 *
 * \return Size of the free heap size
 */
u32_t ryos_get_free_heap(void);

u32_t ryos_get_min_available_heap_size(void);


typedef void ry_queue_t;
ry_sts_t ryos_queue_create(ry_queue_t **qhandle, u32_t q_len, u32_t item_size);
ry_sts_t ryos_queue_delete(ry_queue_t *qhandle);
ry_sts_t ryos_queue_send(ry_queue_t *qhandle, const void *msg, u32_t wait);
ry_sts_t ryos_queue_recv(ry_queue_t *qhandle, void *msg, u32_t wait);
int ryos_queue_get_msgs_waiting(ry_queue_t *qhandle);

ry_thread_state_t ryos_thread_get_state(ryos_thread_t* threadHandle);
void ryos_get_thread_list(char* buffer);
void ryos_get_thread_stats(char* buffer);



#elif (OS_TYPE == OS_RTTHREAD)


/************Task Related API****************/
ry_sts_t ryos_thread_create(ryos_thread_t** pThreadHandle, ryos_threadFunc_t threadFunc, ryos_threadPara_t* pThreadPara);
ry_sts_t ryos_thread_delete(ryos_thread_t* threadHandle);

u32_t ryos_enter_critical(void);
void ryos_exit_critical(rt_base_t level);

ry_sts_t ryos_thread_suspend(ryos_thread_t* threadHandle);
ry_sts_t ryos_thread_resume(ryos_thread_t* threadHandle);
void ryos_thread_suspendAll(void);
void ryos_thread_resumeAll(void);


typedef void ryos_mutex_t; 
ry_sts_t ryos_mutex_create(ryos_mutex_t** handle);
ry_sts_t ryos_mutex_delete(ryos_mutex_t* handle);
ry_sts_t ryos_mutex_unlock(ryos_mutex_t* handle);
ry_sts_t ryos_mutex_lock(ryos_mutex_t* handle);

typedef void ryos_semaphore_t; 
ry_sts_t ryos_semaphore_create(ryos_semaphore_t** handle, ryos_ipc_t* ipcParm);
ry_sts_t ryos_semaphore_delete(ryos_semaphore_t* handle);
ry_sts_t ryos_semaphore_wait(ryos_semaphore_t* handle);
ry_sts_t ryos_semaphore_post(ryos_semaphore_t* handle);


void* ry_malloc(u32_t size);
void  ry_free(void* buffer);
u32_t ryos_get_free_heap(void);
u32_t ryos_get_min_available_heap_size(void);


void ry_memcpy(void* d, const void* s, u32_t l);
void ry_memset(void* b, u8_t v, u32_t l);
int  ry_memcmp(void* s1, void* s2, u32_t l);

void ryos_delay(u32_t dwDelay);
void ryos_delay_tick(u32_t tick);
void ryos_delay_ms(u32_t ms);
void ryos_get_tick_count(unsigned long * dwTickCount);

typedef void ry_queue_t;
ry_sts_t ryos_queue_create(ry_queue_t **qhandle, u32_t q_len, u32_t item_size);
ry_sts_t ryos_queue_delete(ry_queue_t *qhandle);
ry_sts_t ryos_queue_send(ry_queue_t *qhandle, void *msg, u32_t size);
ry_sts_t ryos_queue_recv(ry_queue_t *qhandle, void *msg, u32_t size);
int ryos_queue_get_msgs_waiting(ry_queue_t *qhandle);

typedef void ry_event_t;
ry_sts_t ryos_event_create(ry_event_t **qhandle,ryos_ipc_t* ipcParm);
ry_sts_t ryos_event_delete(ry_event_t *qhandle);
ry_sts_t ryos_event_send(ry_event_t *qhandle,u32_t set);
ry_sts_t ryos_event_recv(ry_event_t *qhandle,u32_t  set,u8_t   option,s32_t   timeout,u32_t *recved);

typedef void ry_mailbox_t;
ry_sts_t ryos_mailbox_create(ry_mailbox_t **handle, u32_t size);
ry_sts_t ryos_mailbox_delete(ry_event_t *handle);
ry_sts_t ryos_mailbox_send(ry_mailbox_t *handle, u32_t value);
ry_sts_t ryos_mailbox_recv(ry_mailbox_t *handle, u32_t* msg);



#endif

#define FREE_PTR(ptr)                                         do{ry_free(ptr);ptr = NULL;}while(0)


#endif  /* __RY_OS_H__ */
