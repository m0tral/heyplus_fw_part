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
#include "app_config.h"
#include <stdio.h>
#include <string.h>

#include "ry_hal_rtc.h"

#if (OS_TYPE == OS_FREERTOS)

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>
#include "portable.h"

/*********************************************************************
 * CONSTANTS
 */ 

//#define RY_OS_TEST

 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */



/**
 * @brief   Init RY OS Obstract layer
 *
 * @param   None
 *
 * @return  None
 */
void ryos_init(void)
{

}

/**
 * @brief   Start OS scheduler
 *
 * @param   None
 *
 * @return  None
 */
void ryos_start_schedule(void)
{
    vTaskStartScheduler();
}

ry_sts_t ryos_thread_create(ryos_thread_t** pThreadHandle, ryos_threadFunc_t threadFunc, ryos_threadPara_t* pThreadPara)
{
    ry_sts_t status = RY_SUCC;

    if ((NULL == pThreadHandle) || (NULL == threadFunc) || (NULL == pThreadPara)) {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    } else {
        /* Indicate the thread is created without message queue */
        if (pdPASS == xTaskCreate( (TaskFunction_t)threadFunc, 
                                    (const char*)&(pThreadPara->name[0]), 
                                    pThreadPara->stackDepth, 
                                    pThreadPara->arg, 
                                    (tskIDLE_PRIORITY + pThreadPara->priority), 
                                    pThreadHandle )) {
            RY_ASSERT(*pThreadHandle);
        } else {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_THREAD_CREATION);
        }
    }

    return status;
}

ry_sts_t ryos_thread_delete(ryos_thread_t* threadHandle)
{
    RY_ASSERT(threadHandle);

    vTaskDelete(threadHandle);
    return RY_SUCC;
}

void ryos_thread_set_tag(ryos_thread_t* threadHandle, u32_t tag)
{
    vTaskSetApplicationTaskTag( threadHandle, ( TaskHookFunction_t ) tag );
}


u32_t ryos_enter_critical(void)
{
    taskENTER_CRITICAL();
    return 0;
}

void ryos_exit_critical(u32_t level)
{
    taskEXIT_CRITICAL();
}

ry_sts_t ryos_thread_suspend(ryos_thread_t* threadHandle)
{
    vTaskSuspend(threadHandle);
    return RY_SUCC;
}

ry_sts_t ryos_thread_resume(ryos_thread_t* threadHandle)
{
    vTaskResume(threadHandle);
    return RY_SUCC;
}

ry_thread_state_t ryos_thread_get_state(ryos_thread_t* threadHandle) 
{
    return (ry_thread_state_t)eTaskGetState(threadHandle);
}

void ryos_thread_suspendAll(void)
{
    vTaskSuspendAll();
}

void ryos_thread_resumeAll(void)
{
    xTaskResumeAll();
}

/*******************************Mutex*****************************************/

ry_sts_t ryos_mutex_create(ryos_mutex_t** pMutex)
{
    ry_sts_t status = RY_SUCC;

    if(NULL != pMutex) {
        *pMutex = xSemaphoreCreateMutex();
        if(NULL == (*pMutex)) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_CREATION);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_mutex_delete(ryos_mutex_t* handle)
{
    RY_ASSERT(handle);

    vSemaphoreDelete(handle);
	return RY_SUCC;
}


ry_sts_t ryos_mutex_unlock(ryos_mutex_t* handle)
{
    ry_sts_t status = RY_SUCC;
    BaseType_t xHigherPriorityTaskWoken;

    RY_ASSERT(handle);

#if 0
    /* Check whether the handle provided by user is valid */
    if (handle != NULL) {
        if(!(xSemaphoreGive(handle))) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_UNLOCK);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

#endif
    

    /* Check whether user passed valid parameter */
    if (handle != NULL) {
        if (xPortIsInsideInterrupt()) {
            if (!(xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_UNLOCK);
            } else {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        } else {
            if (!(xSemaphoreGive(handle))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_UNLOCK);
            }
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}


ry_sts_t ryos_mutex_lock(ryos_mutex_t* handle)
{
    ry_sts_t status = RY_SUCC;
    BaseType_t xHigherPriorityTaskWoken;

    RY_ASSERT(handle);
#if 0
    /* Check whether handle provided by user is valid */
    if ( handle != NULL) {
        /* Wait till the mutex object is unlocked */
        if(!(xSemaphoreTake(handle, portMAX_DELAY))) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_LOCK);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

#endif

    /* Check whether user passed valid parameter */
    if (handle != NULL) {
        /* Wait till the semaphore object is released */
        if(xPortIsInsideInterrupt()) {
            if (!(xSemaphoreTakeFromISR(handle, &xHigherPriorityTaskWoken))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_LOCK);
            } else {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        } else {
            //phOsalNfc_Delay(50);
            if (!(xSemaphoreTake(handle, portMAX_DELAY))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_LOCK);
            }
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    

    return status;
}


/*******************************Semaphore*****************************************/

ry_sts_t ryos_semaphore_create(ryos_semaphore_t** handle, u8_t bInitVal)
{
    ry_sts_t status = RY_SUCC;

    if(NULL != handle) {
        *handle = xSemaphoreCreateCounting(1, (uint32_t)bInitVal);
        if(NULL == (*handle)) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_OS_SEMAPHORE_CREATION);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_semaphore_delete(ryos_semaphore_t* handle)
{
    RY_ASSERT(handle);

    vSemaphoreDelete(handle);
    return RY_SUCC;
}

ry_sts_t ryos_semaphore_wait(ryos_semaphore_t* handle)
{
    ry_sts_t status = RY_SUCC;
    BaseType_t xHigherPriorityTaskWoken;

    /* Check whether user passed valid parameter */
    if (handle != NULL) {
        /* Wait till the semaphore object is released */
        if(xPortIsInsideInterrupt()) {
            if (!(xSemaphoreTakeFromISR(handle, &xHigherPriorityTaskWoken))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_SEMAPHORE_WAIT);
            } else {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        } else {
            //phOsalNfc_Delay(50);
            if (!(xSemaphoreTake(handle, portMAX_DELAY))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_SEMAPHORE_WAIT);
            }
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}


ry_sts_t ryos_semaphore_post(ryos_semaphore_t* handle)
{
    ry_sts_t status = RY_SUCC;
    BaseType_t xHigherPriorityTaskWoken;

    /* Check whether user passed valid parameter */
    if (handle != NULL) {
        if (xPortIsInsideInterrupt()) {
            if (!(xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_SEMAPHORE_POST);
            } else {
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        } else {
            if (!(xSemaphoreGive(handle))) {
                status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_SEMAPHORE_POST);
            }
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}



/*********************************Memory*****************************************/

/*!
 * \brief Allocates memory.
 *        This function attempts to allocate \a size bytes on the heap and
 *        returns a pointer to the allocated block.
 *
 * \param size size of the memory block to be allocated on the heap.
 *
 * \return pointer to allocated memory block or NULL in case of error.
 */
void* ry_malloc(u32_t dwSize)
{
    return (void *) pvPortMalloc(dwSize);
}

void* ry_calloc(u32_t n, u32_t size)
{
    void * p = (void *) pvPortMalloc(n * size);

    ry_memset(p, 0, (n * size));
    
    return p;
}
void* ry_realloc(void *mem_address, u32_t dwSize)
{
    void * p = (void *) pvPortMalloc(dwSize);

    ry_memcpy(p, mem_address, dwSize);

    ry_free(mem_address);
    
    return p;
}


/*!
 * \brief Frees allocated memory block.
 *        This function deallocates memory region pointed to by \a pMem.
 *
 * \param pMem pointer to memory block to be freed.
 */
void ry_free(void *buffer)
{
    //RY_ASSERT(buffer);

    /* Check whether a null pointer is passed */
    if (NULL != buffer) {
        vPortFree(buffer);
        buffer = NULL;
    } else {
        LOG_DEBUG("[ry_free] Empty pointor.\r\n");
    }
}

/*!
 * \brief Get free heap memory size..
 *
 * \param None.
 *
 * \return Size of the free heap size
 */
u32_t ryos_get_free_heap(void)
{
    return xPortGetFreeHeapSize();
}

u32_t ryos_get_min_available_heap_size(void)
{
    return xPortGetMinimumEverFreeHeapSize();
}

int ry_memcmp(void *pDest, void *pSrc, u32_t dwSize)
{
    return memcmp(pDest, pSrc, dwSize);
}

void ry_memset(void *pMem, u8_t bVal, u32_t dwSize)
{
    memset(pMem, bVal, dwSize);
}


void ry_memcpy(void *pDest, const void *pSrc, uint32_t dwSize)
{
    memcpy(pDest, pSrc, dwSize);
}

void ry_memmove(void *pDest, const void *pSrc, uint32_t dwSize)
{
    memmove(pDest, pSrc, dwSize);
}


/**
 * This API allows to delay the current thread execution.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] dwDelay  Duration in milliseconds for which thread execution to be halted.
 */
void ryos_delay(u32_t dwDelay)
{
    vTaskDelay(dwDelay/portTICK_PERIOD_MS);
}

void ryos_delay_ms(u32_t ms)
{
    vTaskDelay(ms/portTICK_PERIOD_MS);
}


/**
 * Get tick count since scheduler has started.
 *
 * \param[in] tickCount        The address of the variable to which tickcount value will be assigned.
 */
u32_t ryos_get_tick_count(void)
{
	//*pTickCount = xTaskGetTickCount();
	return xTaskGetTickCount();
}


u32_t ryos_get_ms(void)
{
	//*ms = portTICK_PERIOD_MS * (xTaskGetTickCount());

    return (portTICK_PERIOD_MS * (xTaskGetTickCount()));
}

u32_t ryos_os_time_now(void)
{
    return (ryos_get_ms()/1000);
}

static int utc_offset = 0;
static uint32_t timezone = 0;

void set_timezone(uint32_t deviation)
{
    timezone = deviation;
}

uint32_t get_timezone(void)
{
    return timezone;
}

uint32_t get_timezone_china(void)
{
    return 8 * 3600;
}

//
// returns time is seconds
// 
u32_t ryos_utc_now(void)
{
	return utc_offset + timezone + ryos_os_time_now();
}

u32_t ryos_utc_now_notimezone(void)
{
	return utc_offset + ryos_os_time_now();
}


// returns in seconds
u32_t ryos_rtc_now(void)
{
    ry_hal_rtc_get();
	return get_utc_tick();
}

u32_t ryos_rtc_now_notimezone(void)
{
    ry_hal_rtc_get();
	return get_utc_tick() - timezone;
}



// return diff
int ryos_utc_set(u32_t utc)
{
	int ret = 0;
	u32_t time_now = ryos_os_time_now();
	
	if (utc_offset != 0){		// Non-first reading standard time

		ret = utc_offset + time_now - utc;		// Local time and standard time difference

		// Temporarily do not adjust the slope to keep a simple strategy
		//arch_os_time_tune(utc - utc_offset);	// Adjust system time with standard time interval

		utc_offset = utc - time_now;		// Set the standard time again
	} else {
		utc_offset = utc - time_now;		// Setting the standard time for the first time
    }


	utc_offset = (utc - time_now + utc_offset)/2;	// Update utc time offset

	// set esp8266 RTC to local time
    //system_set_rtc_time(arch_os_utc_now());

	return ret;
}


/***********************************IPC*****************************************/


ry_sts_t ryos_queue_create(ry_queue_t **qhandle, u32_t q_len, u32_t item_size)
{
    ry_sts_t status = RY_SUCC;

    *qhandle = xQueueCreate(q_len, item_size);
    if (NULL == (*qhandle)) {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_CREATION);
    }

    return status;
}

ry_sts_t ryos_queue_delete(ry_queue_t *qhandle)
{
    vQueueDelete(qhandle);
    qhandle = NULL;
    return RY_SUCC;
}

ry_sts_t ryos_queue_send(ry_queue_t *qhandle, const void *msg, u32_t size)
{
    ry_sts_t status = RY_SUCC;
    signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if(qhandle == NULL){
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_SEND);
        return status;
    }

    if (xPortIsInsideInterrupt()) {
        if (!(xQueueSendToBackFromISR(qhandle, msg, &xHigherPriorityTaskWoken))) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_SEND);
        } else {
            portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
        }
    } else {
        if (!(xQueueSendToBack(qhandle, msg, 0))) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_SEND);
        }
    }

    return status;
}

ry_sts_t ryos_queue_recv(ry_queue_t *qhandle, void *msg, u32_t wait)
{
    ry_sts_t status = RY_SUCC;
    
    RY_ASSERT(qhandle);
    RY_ASSERT(msg);
    
    if(qhandle == NULL){
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_RECV);
        return status;
    }
    
    if (!(xQueueReceive(qhandle, msg, wait))) {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_RECV);
    }

    return status;
}

int ryos_queue_get_msgs_waiting(ry_queue_t *qhandle)
{
    int nmsg = 0;
    nmsg = uxQueueMessagesWaiting(qhandle);
    return nmsg;
}


void ryos_get_thread_list(char* buffer) 
{
    vTaskList(buffer);
}

void ryos_get_thread_stats(char* buffer)
{
    //vTaskGetRunTimeStats(buffer);
}

/*
 * vApplicationStackOverflowHook - Hook when stack overflow occured in some task.
 *
 * @param   pxTask
 * @param   pcTaskName
 *
 * @return  None
 */
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	/* Run time task stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook	function is 
	called if a task stack overflow is detected.  Note the system/interrupt
	stack is not checked. */

	LOG_ERROR("Stack Overflow exception. %s \r\n", pcTaskName);
	taskDISABLE_INTERRUPTS();
	while(1);
}


/***********************************Test*****************************************/

#ifdef RY_OS_TEST

#include "ry_timer.h"

ryos_thread_t    *test_thread1_handle;
ryos_thread_t    *test_thread2_handle;
ryos_mutex_t     *test_mutex;
ryos_semaphore_t *test_sem;
ry_queue_t        *test_queue;
u32_t              test_timer_id;

static int test_cnt = 0;

int test_thread_1(void* arg)
{
    static int test_1_cnt = 0;
    int msg;
    while(1) {

        //LOG_INFO("Thread 1 Handler: %d\r\n", test_1_cnt++);

        //ryos_semaphore_wait(test_sem);
        //test_cnt++;
        //LOG_INFO("Thread 1 Handler: %d\r\n", test_cnt);
        //ryos_delay(3000);

        /* Test Queue */
        if (RY_SUCC == ryos_queue_recv(test_queue, &msg, RY_OS_WAIT_FOREVER)) {
            LOG_INFO("Queue fetch succ. data = %d", msg);
        } else {
            LOG_ERROR("Queue receive error.\r\n");
        }
    }
}


int test_thread_2(void* arg)
{
    static int test_2_cnt = 0;
    ry_sts_t status = RY_SUCC;

    while(1) {
        //LOG_INFO("Thread 2 Handler: %d\r\n", test_2_cnt++);
        //ryos_delay(1000);

        //LOG_INFO("Thread 2 Handler: %d\r\n", test_cnt);
        //test_cnt += 100;
        //ryos_delay(3000);
        //ryos_semaphore_post(test_sem);

        /* Test Queue */
        if (RY_SUCC == ryos_queue_send(test_queue, (void*)&test_2_cnt, RY_OS_NO_WAIT)) {
            LOG_INFO("Queue send succ. data = %d\r\n", test_2_cnt);
            test_2_cnt++;
        } else {
            LOG_ERROR("Queue send error.\r\n");
        }

        if (test_2_cnt == 10) {
            LOG_INFO("Timer stop, id = %d.\r\n", test_timer_id);
            status = ry_timer_stop(test_timer_id);
            if (RY_SUCC != status) {
                LOG_ERROR("Timer stop fail, status = 0x%x\r\n", status);
            }
        }

        ryos_delay(1000);
    }
}


void test_timer_cb(u32_t timerId, void* userData)
{
    LOG_INFO("Timer callback. Timer id = %d, UserData = %d \r\n", timerId, userData);
}

void ryos_test(void)
{
    ryos_threadPara_t para;
    ry_sts_t status = RY_SUCC;

    /* Test Thread */
    LOG_INFO("---------------RY OS Test Begin--------------------\r\n");

    status = ryos_mutex_create(&test_mutex);
    if (RY_SUCC != status) {
        LOG_ERROR("Mutex Create Error.\r\n");
    }

    status = ryos_semaphore_create(&test_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("Semaphore Create Error.\r\n");
    }

    status = ryos_queue_create(&test_queue, 10, 10);
    if (RY_SUCC != status) {
        LOG_ERROR("Queue Create Error.\r\n");
    }

    strcpy((char*)para.name, "test task 1");
    para.stackDepth = 256;
    para.priority = 3;
    para.arg = NULL; 
    ryos_thread_create(&test_thread1_handle, test_thread_1, &para);


    strcpy((char*)para.name, "test task 2");
    para.stackDepth = 256;
    para.priority = 3;
    para.arg = NULL; 
    ryos_thread_create(&test_thread1_handle, test_thread_2, &para);

    /* Test Timer */
    test_timer_id = ry_timer_create(1);
    if (test_timer_id == RY_ERR_TIMER_ID_INVALID) {
        LOG_ERROR("Create Timer fail. Invalid timer id.\r\n");
    }

    ry_timer_start(test_timer_id, 1000, test_timer_cb, NULL);
}

#endif

#elif (OS_TYPE == OS_RTTHREAD)

#include <rtthread.h>
#include "rtdebug.h"
#include "rthw.h"
#include "rtm.h"
#include "components.h"
#include "rtdef.h"

/*********************************************************************
 * CONSTANTS
 */ 

//#define RY_OS_TEST

 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */



/**
 * @brief   Init RY OS Obstract layer
 *
 * @param   None
 *
 * @return  None
 */
void ryos_init(void)
{
    /* show version */
    rt_show_version();

    /* init tick */
    rt_system_tick_init();

    /* init kernel object */
    rt_system_object_init();

    /* init timer system */
    rt_system_timer_init();

#ifdef RT_USING_EXT_SDRAM
    rt_system_heap_init((void*)EXT_SDRAM_BEGIN, (void*)EXT_SDRAM_END);
#else
    rt_system_heap_init((void*)HEAP_BEGIN, (void*)HEAP_END);
#endif

    /* init scheduler system */
    rt_system_scheduler_init();

    /* init application */
    //rt_application_init();

    /* init timer thread */
    rt_system_timer_thread_init();

    /* init idle thread */
    rt_thread_idle_init();
}

/**
 * @brief   Start OS scheduler
 *
 * @param   None
 *
 * @return  None
 */
void ryos_start_schedule(void)
{
    rt_system_scheduler_start();
}

ry_sts_t ryos_thread_create(ryos_thread_t** pThreadHandle, ryos_threadFunc_t threadFunc, ryos_threadPara_t* pThreadPara)
{
    ry_sts_t status = RY_SUCC;
    rt_thread_t thread = RT_NULL;

    thread = (ryos_thread_t*)rt_thread_create(pThreadPara->name, threadFunc, pThreadPara->arg, 
		                         pThreadPara->stackDepth, pThreadPara->priority, pThreadPara->tick);
 
	if(thread != RT_NULL) {
        *pThreadHandle = thread;   
        rt_thread_startup(thread);
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_THREAD_CREATION);
    }
	
    return status;
}

ry_sts_t ryos_thread_delete(ryos_thread_t* threadHandle)
{
    rt_err_t status;
    status = rt_thread_delete((rt_thread_t)threadHandle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_THREAD_DELETE);
}

u32_t ryos_enter_critical(void)
{
    return rt_hw_interrupt_disable();
}

void ryos_exit_critical(rt_base_t level)
{
    rt_hw_interrupt_enable(level);
}

ry_sts_t ryos_thread_suspend(ryos_thread_t* threadHandle)
{
    rt_err_t status;
    status = rt_thread_suspend((rt_thread_t)threadHandle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_THREAD_SUSPEND);
}

ry_sts_t ryos_thread_resume(ryos_thread_t* threadHandle)
{
    rt_err_t status;
    status = rt_thread_resume((rt_thread_t)threadHandle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_THREAD_SUSPEND);
}

void ryos_thread_suspendAll(void)
{
    
}

void ryos_thread_resumeAll(void)
{
    
}

/*******************************Mutex*****************************************/

ry_sts_t ryos_mutex_create(ryos_mutex_t** handle)
{
    ry_sts_t status = RY_SUCC;
    char name[RT_NAME_MAX];
    static u16_t ryos_mutex_num = 0;

    if (NULL != handle) {
        rt_snprintf(name, sizeof(name), "rymtx%02d", ryos_mutex_num++);
        *handle = (ryos_mutex_t*)rt_mutex_create(name, RT_IPC_FLAG_FIFO);
        if (NULL == (*handle)) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_CREATION);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_mutex_delete(ryos_mutex_t* handle)
{
    rt_err_t status;
    status = rt_mutex_delete((rt_mutex_t)handle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_DELETE);
}


ry_sts_t ryos_mutex_unlock(ryos_mutex_t* handle)
{
    rt_err_t status;
    status = rt_mutex_release((rt_mutex_t)handle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_UNLOCK);
}


ry_sts_t ryos_mutex_lock(ryos_mutex_t* handle)
{
    rt_err_t err;
    ry_sts_t status = RY_SUCC;

    /* Check whether handle provided by user is valid */
    if ( handle != NULL) {
        err = rt_mutex_take((rt_mutex_t)handle, RT_WAITING_FOREVER);
        if (RT_EOK != err) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_LOCK);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}


/*******************************Semaphore*****************************************/

ry_sts_t ryos_semaphore_create(ryos_semaphore_t** handle, ryos_ipc_t* ipcParm)
{
    ry_sts_t status = RY_SUCC;
    char name[RT_NAME_MAX];
    static u16_t ryos_sem_num = 0;

    if (NULL != handle) {
        rt_snprintf(name, sizeof(name), "rysem%02d", ryos_sem_num++);
        *handle = (ryos_semaphore_t*)rt_sem_create(name, 0, RT_IPC_FLAG_FIFO);
        if (NULL == (*handle)) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_OS_SEMAPHORE_CREATION);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_semaphore_delete(ryos_semaphore_t* handle)
{
    rt_err_t status;
    status = rt_sem_delete((rt_sem_t)handle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_SEMAPHORE_DELETE);
}

ry_sts_t ryos_semaphore_wait(ryos_semaphore_t* handle)
{
    rt_err_t err;
    ry_sts_t status = RY_SUCC;

    /* Check whether handle provided by user is valid */
    if ( handle != NULL) {
        err = rt_sem_take((rt_sem_t)handle, RT_WAITING_FOREVER);
        if (RT_EOK != err) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_SEMAPHORE_WAIT);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}


ry_sts_t ryos_semaphore_post(ryos_semaphore_t* handle)
{
    rt_err_t status;
    status = rt_sem_release((rt_sem_t)handle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MUTEX_UNLOCK);
}



/*********************************Memory*****************************************/

/*!
 * \brief Allocates memory.
 *        This function attempts to allocate \a size bytes on the heap and
 *        returns a pointer to the allocated block.
 *
 * \param size size of the memory block to be allocated on the heap.
 *
 * \return pointer to allocated memory block or NULL in case of error.
 */
void* ry_malloc(u32_t dwSize)
{
    return (void *)rt_malloc(dwSize);
}

/*!
 * \brief Frees allocated memory block.
 *        This function deallocates memory region pointed to by \a pMem.
 *
 * \param pMem pointer to memory block to be freed.
 */
void ry_free(void *buffer)
{
    RY_ASSERT(buffer);
    rt_free(buffer);
}

/*!
 * \brief Get free heap memory size..
 *
 * \param None.
 *
 * \return Size of the free heap size
 */
u32_t ryos_get_free_heap(void)
{
    //xPortGetFreeHeapSize();
}

int ry_memcmp(void *pDest, void *pSrc, u32_t dwSize)
{
    return memcmp(pDest, pSrc, dwSize);
}

void ry_memset(void *pMem, u8_t bVal, u32_t dwSize)
{
    memset(pMem, bVal, dwSize);
}


void ry_memcpy( void *pDest, const void *pSrc, uint32_t dwSize )
{
    memcpy(pDest, pSrc, dwSize);
}


/**
 * This API allows to delay the current thread execution.
 * \note This function executes successfully without OSAL module Initialization.
 *
 * \param[in] dwDelay  Duration in milliseconds for which thread execution to be halted.
 */
void ryos_delay(u32_t dwDelay)
{
    rt_thread_delay(dwDelay);
}

void ryos_delay_tick(u32_t tick)
{
    rt_thread_delay(tick);
}

void ryos_delay_ms(u32_t ms)
{
    u32_t tick = ms * RT_TICK_PER_SECOND / 1000; 
    rt_thread_delay(tick);
}

/**
 * Get tick count since scheduler has started.
 *
 * \param[in] tickCount        The address of the variable to which tickcount value will be assigned.
 */
void ryos_get_tick_count(unsigned long * pTickCount)
{
	*pTickCount = rt_tick_get();
}


void ryos_get_ms(unsigned long * ms)
{
	*ms = 1000/RT_TICK_PER_SECOND * (rt_tick_get());
}


/***********************************IPC*****************************************/


ry_sts_t ryos_queue_create(ry_queue_t **qhandle, u32_t q_len, u32_t item_size)
{
    ry_sts_t status = RY_SUCC;
    char name[RT_NAME_MAX];
    static u16_t ryos_queue_num = 0;

    if (NULL != qhandle) {
        rt_snprintf(name, sizeof(name), "ryQ%02d", ryos_queue_num++);
        *qhandle = (ry_queue_t*)rt_mq_create(name, item_size, q_len, RT_IPC_FLAG_FIFO);
        if (NULL == (*qhandle)) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_CREATION);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_queue_delete(ry_queue_t *qhandle)
{
    rt_err_t status;
    status = rt_mq_delete((rt_mq_t)qhandle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QEUEU_DELETE);
}

ry_sts_t ryos_queue_send(ry_queue_t *qhandle, void *msg, u32_t size)
{
    rt_err_t err;
    ry_sts_t status = RY_SUCC;

    /* Check whether handle provided by user is valid */
    if ( (qhandle != NULL) && (msg != NULL) && (size > 0)) {
        err = rt_mq_send((rt_mq_t)qhandle, msg, size);
        if (RT_EOK != err) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_SEND);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_queue_recv(ry_queue_t *qhandle, void *msg, u32_t size)
{
    rt_err_t err;
    ry_sts_t status = RY_SUCC;

    /* Check whether handle provided by user is valid */
    if ( (qhandle != NULL) && (msg != NULL) && (size > 0)) {
        err = rt_mq_recv((rt_mq_t)qhandle, msg, size, RT_WAITING_FOREVER);
        if (RT_EOK != err) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QUEUE_RECV);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}


int ryos_queue_get_msgs_waiting(ry_queue_t *qhandle)
{

}


ry_sts_t ryos_event_create(ry_event_t **qhandle,ryos_ipc_t* ipcParm)
{
	ry_sts_t status = RY_SUCC;
	*qhandle = (ry_event_t *)rt_event_create(ipcParm->name,ipcParm->flag);

	return status;
}

ry_sts_t ryos_event_delete(ry_event_t *qhandle)
{
	ry_sts_t status = RY_SUCC;
	rt_event_delete((rt_event_t)qhandle);
	return status;
}
ry_sts_t ryos_event_send(ry_event_t *qhandle,u32_t set)
{
	ry_sts_t status = RY_SUCC;
	
	rt_event_send((rt_event_t)qhandle,set);
	
	return status;
}

ry_sts_t ryos_event_recv(ry_event_t *qhandle,
                       u32_t  set,
                       u8_t   option,
                       s32_t   timeout,
                       u32_t *recved)
{
	ry_sts_t status = RY_SUCC;
	
	rt_event_recv((rt_event_t)qhandle,set,option,timeout,recved);
	
	return status;
}

ry_sts_t ryos_mailbox_create(ry_mailbox_t **handle, u32_t size)
{
    ry_sts_t status = RY_SUCC;
    char name[RT_NAME_MAX];
    static u16_t ryos_mb_num = 0;

    if (NULL != handle) {
        rt_snprintf(name, sizeof(name), "rymb%02d", ryos_mb_num++);
        *handle = (ry_mailbox_t*)rt_mb_create(name, size, RT_IPC_FLAG_FIFO);
        if (NULL == (*handle)) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MB_CREATION);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_mailbox_delete(ry_event_t *handle)
{
    rt_err_t status;
    status = rt_mb_delete((rt_mailbox_t)handle);
    return (status == RT_EOK) ? RY_SUCC : RY_SET_STS_VAL(RY_CID_OS, RY_ERR_QEUEU_DELETE);
}

ry_sts_t ryos_mailbox_send(ry_mailbox_t *handle, u32_t value)
{
    rt_err_t err;
    ry_sts_t status = RY_SUCC;

    /* Check whether handle provided by user is valid */
    if (handle != NULL) {
        err = rt_mb_send((rt_mailbox_t)handle, value);
        if (RT_EOK != err) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MB_SEND);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}

ry_sts_t ryos_mailbox_recv(ry_mailbox_t *handle, u32_t* msg)
{
    rt_err_t err;
    ry_sts_t status = RY_SUCC;

    /* Check whether handle provided by user is valid */
    if ( (handle != NULL) && (msg != NULL)) {
        err = rt_mb_recv((rt_mailbox_t)handle, msg, RT_WAITING_FOREVER);
        if (RT_EOK != err) {
            status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_MB_RECV);
        }
    } else {
        status = RY_SET_STS_VAL(RY_CID_OS, RY_ERR_INVALIC_PARA);
    }

    return status;
}




#endif  /* (OS_TYPE == OS_FREERTOS) */
