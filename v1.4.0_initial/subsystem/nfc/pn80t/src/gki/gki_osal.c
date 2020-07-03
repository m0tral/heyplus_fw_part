/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 */
#include <stdio.h>
#include <stdarg.h>
#ifdef ANDROID
#include <errno.h>
#endif
#include <stdlib.h>

#define GKI_DEBUG   FALSE

#include <time.h>
#include "gki_int.h"
#include "gki_target.h"
#include "OverrideLog.h"
#include "phOsalNfc_Thread.h"

#include "ry_utils.h"

/* Temp android logging...move to android tgt config file */

#ifndef LINUX_NATIVE
#include <cutils/log.h>
#else
#define LOGV(format, ...)  fprintf (stdout, LOG_TAG format, ## __VA_ARGS__)
#define LOGE(format, ...)  fprintf (stderr, LOG_TAG format, ## __VA_ARGS__)
#define LOGI(format, ...)  fprintf (stdout, LOG_TAG format, ## __VA_ARGS__)

#define SCHED_NORMAL 0
#define SCHED_FIFO 1
#define SCHED_RR 2
#define SCHED_BATCH 3

#endif

/* Define the structure that holds the GKI variables
*/
#if GKI_DYNAMIC_MEMORY == FALSE
tGKI_CB   gki_cb;
#else
tGKI_CB * gki_cb_ptr;
#endif

#define NANOSEC_PER_MILLISEC (1000000)
#define NSEC_PER_SEC (1000*NANOSEC_PER_MILLISEC)

/* works only for 1ms to 1000ms heart beat ranges */
#define LINUX_SEC (1000/TICKS_PER_SEC)
// #define GKI_TICK_TIMER_DEBUG

//#define INIT(m) pthread_mutex_init(&m, NULL)


/* this kind of mutex go into tGKI_OS control block!!!! */
/* static pthread_mutex_t GKI_sched_mutex; */
/*static pthread_mutex_t thread_delay_mutex;
static pthread_cond_t thread_delay_cond;
static pthread_mutex_t gki_timer_update_mutex;
static pthread_cond_t   gki_timer_update_cond;
*/
#ifdef NO_GKI_RUN_RETURN
static pthread_t            timer_thread_id = 0;
#endif
#if (NXP_EXTNS == TRUE)
UINT8 gki_buf_init_done = FALSE;
#endif
/* For Android */

#ifndef GKI_SHUTDOWN_EVT
#define GKI_SHUTDOWN_EVT    APPL_EVT_7
#endif

typedef struct GKI_binarySemaphore {
    void * mutex;
    int v;
}GKI_tBinarySemaphore;


typedef struct
{
    UINT8 task_id;          /* GKI task id */
    TASKPTR task_entry;     /* Task entry function*/
    UINT32 params;          /* Extra params to pass to task entry function */
    GKI_tBinarySemaphore* pBSem;	/* for android*/
#ifndef ANDROID
    void*	context;
#endif
} gki_pthread_info_t;
gki_pthread_info_t gki_pthread_info[GKI_MAX_TASKS];


/*******************************************************************************
**
** Function         GKI_init
**
** Description      This function is called once at startup to initialize
**                  all the timer structures.
**
** Returns          void
**
*******************************************************************************/

void GKI_init(void)
{
	/*mutex attribute replaced by a flag, which will decide if a regular mutex or recursive mutex has to be created*/
	UINT8 GKI_mutex_recursive = 0;

    tGKI_OS             *p_os;
#if (NXP_EXTNS == TRUE)
    /* Added to avoid re-initialization of memory pool (memory leak) */
    if(!gki_buf_init_done)
    {
        memset (&gki_cb, 0, sizeof (gki_cb));
        gki_buffer_init();
        gki_buf_init_done = TRUE;
    }
#else

    memset (&gki_cb, 0, sizeof (gki_cb));
    gki_buffer_init();
#endif

    gki_timers_init();
    phOsalNfc_GetTickCount(&(gki_cb.com.OSTicks));

#ifndef __CYGWIN__
    GKI_mutex_recursive = 1;
#endif
    p_os = &gki_cb.os;

    if (GKI_mutex_recursive)
    {
    	if (phOsalNfc_CreateRecursiveMutex(&(p_os->GKI_mutex)) != NFCSTATUS_SUCCESS )
    	{
    		GKI_TRACE_ERROR_0("CreateRecursiveMutex failed() \n");
            return;
    	}
    }
    else
    {
    	if (phOsalNfc_CreateMutex(&(p_os->GKI_mutex)) != NFCSTATUS_SUCCESS )
    	{
    		GKI_TRACE_ERROR_0("CreateRecursiveMutex failed() \n");
            return;
    	}
    }

    /* Initialiase GKI_timer_update suspend variables & mutexes to be in running state.
     * this works too even if GKI_NO_TICK_STOP is defined in btld.txt */
    p_os->no_timer_suspend = GKI_TIMER_TICK_RUN_COND;
}


/*******************************************************************************
**
** Function         GKI_get_os_tick_count
**
** Description      This function is called to retrieve the native OS system tick.
**
** Returns          Tick count of native OS.
**
*******************************************************************************/
UINT32 GKI_get_os_tick_count(void)
{
    return (gki_cb.com.OSTicks);
}

/*******************************************************************************
**
** Function         GKI_create_task
**
** Description      This function is called to create a new OSS task.
**
** Parameters:      task_entry  - (input) pointer to the entry function of the task
**                  task_id     - (input) Task id is mapped to priority
**                  taskname    - (input) name given to the task
**                  stack       - (input) pointer to the top of the stack (highest memory location)
**                  stacksize   - (input) size of the stack allocated for the task
**
** Returns          GKI_SUCCESS if all OK, GKI_FAILURE if any problem
**
** NOTE             This function take some parameters that may not be needed
**                  by your particular OS. They are here for compatability
**                  of the function prototype.
**
*******************************************************************************/
UINT8 GKI_create_task (TASKPTR task_entry, UINT8 task_id, INT8 *taskname, UINT16 *stack, UINT16 stacksize, void* pBinSem)
{
	phOsalNfc_ThreadCreationParams_t threadparams;
	UINT32 len;

    GKI_TRACE_5 ("GKI_create_task func=0x%x  id=%d  name=%s  stack=0x%x  stackSize=%d", task_entry, task_id, taskname, stack, stacksize);

    if (task_id >= GKI_MAX_TASKS)
    {
        GKI_TRACE_0("Error! task ID > max task allowed");
        return (GKI_FAILURE);
    }

    gki_cb.com.OSRdyTbl[task_id]    = (UINT8) TASK_READY;
    gki_cb.com.OSTName[task_id]     = (INT8*) taskname;
    gki_cb.com.OSWaitTmr[task_id]   = 0;
    gki_cb.com.OSWaitEvt[task_id]   = 0;

    /* On Android, the new tasks starts running before 'gki_cb.os.thread_id[task_id]' is initialized */
    /* Pass task_id to new task so it can initialize gki_cb.os.thread_id[task_id] for it calls GKI_wait */
    gki_pthread_info[task_id].task_id = task_id;
    gki_pthread_info[task_id].task_entry = task_entry;
    gki_pthread_info[task_id].params = 0;
#ifdef ANDROID
    gki_pthread_info[task_id].pBSem = (GKI_tBinarySemaphore *)pBinSem;
#else
    gki_pthread_info[task_id].context = pBinSem;
#endif

    threadparams.stackdepth = stacksize;
    for (len = 0; len < TASK_NAME_MAX_LENGTH; len++)
    	threadparams.taskname[len] = taskname[len];
    threadparams.pContext = pBinSem;
    threadparams.priority = GKI_TASK_PRIORITY;
    /* Create the client thread */
    if(phOsalNfc_Thread_Create(&gki_cb.os.thread_id[task_id], task_entry, &threadparams) != 0)
    {
    	GKI_TRACE_1("Task Create failed(), %s!",taskname);
    	return GKI_FAILURE;
    }

#if defined(PBS_SQL_TASK)
         if (task_id == PBS_SQL_TASK)
         {
             GKI_TRACE_0("PBS SQL lowest priority task");

         }
         else
#endif
         {

             //vTaskPrioritySet(gki_cb.os.thread_id[task_id], ( tskIDLE_PRIORITY + 3 ));
         }

    GKI_TRACE_6( "Leaving GKI_create_task %x %d %x %s %x %d",
              task_entry,
              task_id,
              gki_cb.os.thread_id[task_id],
              taskname,
              stack,
              stacksize);

    return (GKI_SUCCESS);

}

/*******************************************************************************
**
** Function         GKI_shutdown
**
** Description      shutdowns the GKI tasks/threads in from max task id to 0 and frees
**                  pthread resources!
**                  IMPORTANT: in case of join method, GKI_shutdown must be called outside
**                  a GKI thread context!
**
** Returns          void
**
*******************************************************************************/
#define WAKE_LOCK_ID "brcm_nfca"
#define PARTIAL_WAKE_LOCK 1
extern int acquire_wake_lock(int lock, const char* id);
extern int release_wake_lock(const char* id);

void GKI_shutdown(void)
{
    UINT8 task_id;
    volatile int    *p_run_cond = &gki_cb.os.no_timer_suspend;
#if ( FALSE == GKI_PTHREAD_JOINABLE )
    int i = 0;
    UNUSED(i);
#else
    int result;
#endif

    /* release threads and set as TASK_DEAD. going from low to high priority fixes
     * GKI_exception problem due to btu->hci sleep request events  */
    for (task_id = GKI_MAX_TASKS; task_id > 0; task_id--)
    {
        if (gki_cb.com.OSRdyTbl[task_id - 1] != TASK_DEAD)
        {
            gki_cb.com.OSRdyTbl[task_id - 1] = TASK_DEAD;

            /* paranoi settings, make sure that we do not execute any mailbox events */
            gki_cb.com.OSWaitEvt[task_id-1] &= ~(TASK_MBOX_0_EVT_MASK|TASK_MBOX_1_EVT_MASK|
                                                TASK_MBOX_2_EVT_MASK|TASK_MBOX_3_EVT_MASK);
            GKI_send_event(task_id - 1, EVENT_MASK(GKI_SHUTDOWN_EVT));
            GKI_TRACE_1( "GKI_shutdown(): task %s dead", gki_cb.com.OSTName[task_id]);
#ifdef ANDROID
#if ( FALSE == GKI_PTHREAD_JOINABLE )
             i = 0;

             while ((gki_cb.com.OSWaitEvt[task_id - 1] != 0) && (++i < 5))
                 usleep(2 * 1000);
#else
             /* wait for proper Arnold Schwarzenegger task state */
		     result = pthread_join( gki_cb.os.thread_id[task_id-1], NULL );
			 if ( result < 0 )
			 {
			     GKI_TRACE_1( "pthread_join() FAILED: result: %d", result );
			 }
#endif
#endif
            GKI_exit_task(task_id - 1);
        }
    }

    /* Destroy mutex and condition variable objects */
    phOsalNfc_DeleteMutex(&(gki_cb.os.GKI_mutex));

#if ( FALSE == GKI_PTHREAD_JOINABLE )
    i = 0;
#endif

#ifdef NO_GKI_RUN_RETURN
    shutdown_timer = 1;
#endif
    if (gki_cb.os.gki_timer_wake_lock_on)
    {
        GKI_TRACE_0("GKI_shutdown :  release_wake_lock(brcm_btld)");
        //release_wake_lock(WAKE_LOCK_ID);
        gki_cb.os.gki_timer_wake_lock_on = 0;
    }
    *p_run_cond = GKI_TIMER_TICK_EXIT_COND;
}

/*******************************************************************************
 **
 ** Function        GKI_run
 **
 ** Description     This function runs a task
 **
 ** Parameters:     start: TRUE start system tick (again), FALSE stop
 **
 ** Returns         void
 **
 *********************************************************************************/
void gki_system_tick_start_stop_cback(BOOLEAN start)
{

    tGKI_OS         *p_os = &gki_cb.os;
    volatile int    *p_run_cond = &p_os->no_timer_suspend;
#ifdef GKI_TICK_TIMER_DEBUG
    static volatile int wake_lock_count;
#endif
    if ( FALSE == start )
    {
        /* this can lead to a race condition. however as we only read this variable in the timer loop
         * we should be fine with this approach. otherwise uncomment below mutexes.
         */
        /* GKI_disable(); */
        *p_run_cond = GKI_TIMER_TICK_STOP_COND;
        /* GKI_enable(); */
#ifdef GKI_TICK_TIMER_DEBUG
        BT_TRACE_1( TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, ">>> STOP GKI_timer_update(), wake_lock_count:%d", --wake_lock_count);
#endif
        //release_wake_lock(WAKE_LOCK_ID);
        gki_cb.os.gki_timer_wake_lock_on = 0;
    }
    else
    {
        /* restart GKI_timer_update() loop */
        //acquire_wake_lock(PARTIAL_WAKE_LOCK, WAKE_LOCK_ID);
        gki_cb.os.gki_timer_wake_lock_on = 1;
        *p_run_cond = GKI_TIMER_TICK_RUN_COND;

        //xSemaphoreTake( p_os->gki_timer_mutex,portMAX_DELAY);
        //xEventGroupSetBits( p_os->gki_timer_cond,0x08);
        //xSemaphoreGive( p_os->gki_timer_mutex );

#ifdef GKI_TICK_TIMER_DEBUG
        BT_TRACE_1( TRACE_LAYER_HCI, TRACE_TYPE_DEBUG, ">>> START GKI_timer_update(), wake_lock_count:%d", ++wake_lock_count );
#endif
    }
}

/*******************************************************************************
**
** Function         GKI_stop
**
** Description      This function is called to stop
**                  the tasks and timers when the system is being stopped
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put specific code here.
**
*******************************************************************************/
void GKI_stop (void)
{
    UINT8 task_id;

    for(task_id = 0; task_id<GKI_MAX_TASKS; task_id++)
    {
        if(gki_cb.com.OSRdyTbl[task_id] != TASK_DEAD)
        {
            GKI_exit_task(task_id);
        }
    }
}

/*******************************************************************************
**
** Function         GKI_delay
**
** Description      This function is called by tasks to sleep unconditionally
**                  for a specified amount of time. The duration is in milliseconds
**
** Parameters:      timeout -    (input) the duration in milliseconds
**
** Returns          void
**
*******************************************************************************/

void GKI_delay (UINT32 timeout)
{
    phOsalNfc_Delay(timeout);

    /* Check if task was killed while sleeping */
    /* NOTE
    **      if you do not implement task killing, you do not
    **      need this check.

    if (rtask && gki_cb.com.OSRdyTbl[rtask] == TASK_DEAD)
    {
    }
    commented to fix CID 10877
    */

    GKI_TRACE_1("GKI_delay %d done", timeout);
    return;
}


/*******************************************************************************
**
** Function         GKI_send_event
**
** Description      This function is called by tasks to send events to other
**                  tasks. Tasks can also send events to themselves.
**
** Parameters:      task_id -  (input) The id of the task to which the event has to
**                  be sent
**                  event   -  (input) The event that has to be sent
**
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
*******************************************************************************/
UINT8 GKI_send_event (UINT8 task_id, UINT16 event)
{
    GKI_send_msg(task_id, event, NULL);
    // Adding return status to fix coverity issue.
    return ( GKI_SUCCESS );
}

/*******************************************************************************
**
** Function         GKI_isend_event
**
** Description      This function is called from ISRs to send events to other
**                  tasks. The only difference between this function and GKI_send_event
**                  is that this function assumes interrupts are already disabled.
**
** Parameters:      task_id -  (input) The destination task Id for the event.
**                  event   -  (input) The event flag
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put your code here, otherwise you can delete the entire
**                  body of the function.
**
*******************************************************************************/
UINT8 GKI_isend_event (UINT8 task_id, UINT16 event)
{

    GKI_TRACE_2("GKI_isend_event %d %x", task_id, event);
    GKI_TRACE_2("GKI_isend_event %d %x done", task_id, event);
    return    GKI_send_event(task_id, event);
}


/*******************************************************************************
**
** Function         GKI_get_taskid
**
** Description      This function gets the currently running task ID.
**
** Returns          task ID
**
** NOTE             The Widcomm upper stack and profiles may run as a single task.
**                  If you only have one GKI task, then you can hard-code this
**                  function to return a '1'. Otherwise, you should have some
**                  OS-specific method to determine the current task.
**
*******************************************************************************/
UINT8 GKI_get_taskid (void)
{
    int i;
    void * taskhandle;

    taskhandle = phOsalNfc_GetTaskHandle();

    for (i = 0; i < GKI_MAX_TASKS; i++) {
        if (gki_cb.os.thread_id[i] == taskhandle) {
            GKI_TRACE_2("GKI_get_taskid %x %d done", gki_cb.os.thread_id, i);
            return(i);
        }
    }

    GKI_TRACE_1("GKI_get_taskid: thread id = %x, task id = -1", gki_cb.os.thread_id);

    return (UINT8)(-1);
}

/*******************************************************************************
**
** Function         GKI_map_taskname
**
** Description      This function gets the task name of the taskid passed as arg.
**                  If GKI_MAX_TASKS is passed as arg the currently running task
**                  name is returned
**
** Parameters:      task_id -  (input) The id of the task whose name is being
**                  sought. GKI_MAX_TASKS is passed to get the name of the
**                  currently running task.
**
** Returns          pointer to task name
**
** NOTE             this function needs no customization
**
*******************************************************************************/
#if (GKI_DEBUG == TRUE)

INT8 *GKI_map_taskname (UINT8 task_id)
{
    GKI_TRACE_1("GKI_map_taskname %d", task_id);

    if (task_id < GKI_MAX_TASKS)
    {
        GKI_TRACE_2("GKI_map_taskname %d %s done", task_id, gki_cb.com.OSTName[task_id]);
         return (gki_cb.com.OSTName[task_id]);
    }
    else if (task_id == GKI_MAX_TASKS )
    {
        return (gki_cb.com.OSTName[GKI_get_taskid()]);
    }
    else
    {
        return (INT8 *) "BAD";
    }
}
#endif

/*******************************************************************************
**
** Function         GKI_enable
**
** Description      This function enables interrupts.
**
** Returns          void
**
*******************************************************************************/
void GKI_enable (void)
{
    GKI_TRACE_0("GKI_enable");
#ifndef __CYGWIN__
    phOsalNfc_UnlockRecursiveMutex(gki_cb.os.GKI_mutex);
#else
    phOsalNfc_UnlockMutex(gki_cb.os.GKI_mutex);
#endif
/*  pthread_mutex_xx is nesting save, no need for this: already_disabled = 0; */
    GKI_TRACE_0("Leaving GKI_enable");
    return;
}


/*******************************************************************************
**
** Function         GKI_disable
**
** Description      This function disables interrupts.
**
** Returns          void
**
*******************************************************************************/

void GKI_disable (void)
{
    GKI_TRACE_0("GKI_disable");
#ifndef __CYGWIN__
    phOsalNfc_LockRecursiveMutex(gki_cb.os.GKI_mutex);
#else
    phOsalNfc_LockMutex(gki_cb.os.GKI_mutex);
#endif

    GKI_TRACE_0("Leaving GKI_disable");
    return;
}

/*******************************************************************************
**
** Function         GKI_exception
**
** Description      This function throws an exception.
**                  This is normally only called for a nonrecoverable error.
**
** Parameters:      code    -  (input) The code for the error
**                  msg     -  (input) The message that has to be logged
**
** Returns          void
**
*******************************************************************************/

void GKI_exception (UINT16 code, char *msg)
{
    UINT8 task_id;

    LOG_ERROR( "GKI_exception(): Task State Table");

    for(task_id = 0; task_id < GKI_MAX_TASKS; task_id++)
    {
        GKI_TRACE_ERROR_3( "TASK ID [%d] task name [%s] state [%d]",
                         task_id,
                         gki_cb.com.OSTName[task_id],
                         gki_cb.com.OSRdyTbl[task_id]);
    }

    GKI_TRACE_ERROR_2("GKI_exception %d %s", code, msg);
    GKI_TRACE_ERROR_0( "********************************************************************");
    GKI_TRACE_ERROR_2( "* GKI_exception(): %d %s", code, msg);
    GKI_TRACE_ERROR_0( "********************************************************************");

#if (GKI_DEBUG == TRUE)
    GKI_disable();

    if (gki_cb.com.ExceptionCnt < GKI_MAX_EXCEPTION)
    {
        EXCEPTION_T *pExp;

        pExp =  &gki_cb.com.Exception[gki_cb.com.ExceptionCnt++];
        pExp->type = code;
        pExp->taskid = GKI_get_taskid();
        strncpy((char *)pExp->msg, msg, GKI_MAX_EXCEPTION_MSGLEN - 1);
    }

    GKI_enable();
#endif

    LOG_ERROR("GKI_exception %d %s done", code, msg);


    return;
}


/*******************************************************************************
**
** Function         GKI_get_time_stamp
**
** Description      This function formats the time into a user area
**
** Parameters:      tbuf -  (output) the address to the memory containing the
**                  formatted time
**
** Returns          the address of the user area containing the formatted time
**                  The format of the time is ????
**
** NOTE             This function is only called by OBEX.
**
*******************************************************************************/
INT8 *GKI_get_time_stamp (INT8 *tbuf)
{
    UINT32 ms_time;
    UINT32 s_time;
    UINT32 m_time;
    UINT32 h_time;
    INT8   *p_out = tbuf;
    phOsalNfc_GetTickCount(&(gki_cb.com.OSTicks));
    ms_time = GKI_TICKS_TO_MS(gki_cb.com.OSTicks);
    s_time  = ms_time/100;   /* 100 Ticks per second */
    m_time  = s_time/60;
    h_time  = m_time/60;

    ms_time -= s_time*100;
    s_time  -= m_time*60;
    m_time  -= h_time*60;

    *p_out++ = (INT8)((h_time / 10) + '0');
    *p_out++ = (INT8)((h_time % 10) + '0');
    *p_out++ = ':';
    *p_out++ = (INT8)((m_time / 10) + '0');
    *p_out++ = (INT8)((m_time % 10) + '0');
    *p_out++ = ':';
    *p_out++ = (INT8)((s_time / 10) + '0');
    *p_out++ = (INT8)((s_time % 10) + '0');
    *p_out++ = ':';
    *p_out++ = (INT8)((ms_time / 10) + '0');
    *p_out++ = (INT8)((ms_time % 10) + '0');
    *p_out++ = ':';
    *p_out   = 0;

    return (tbuf);
}


/*******************************************************************************
**
** Function         GKI_register_mempool
**
** Description      This function registers a specific memory pool.
**
** Parameters:      p_mem -  (input) pointer to the memory pool
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If your OS has different memory pools, you
**                  can tell GKI the pool to use by calling this function.
**
*******************************************************************************/
void GKI_register_mempool (void *p_mem)
{
    gki_cb.com.p_user_mempool = p_mem;

    return;
}

/*******************************************************************************
**
** Function         GKI_os_malloc
**
** Description      This function allocates memory
**
** Parameters:      size -  (input) The size of the memory that has to be
**                  allocated
**
** Returns          the address of the memory allocated, or NULL if failed
**
** NOTE             This function is called by the Widcomm stack when
**                  dynamic memory allocation is used. (see dyn_mem.h)
**
*******************************************************************************/
void *GKI_os_malloc (UINT32 size)
{
	ALOGD("GKI_os_malloc: %ld", size);
    return (phOsalNfc_GetMemory(size));
}

/*******************************************************************************
**
** Function         GKI_os_free
**
** Description      This function frees memory
**
** Parameters:      size -  (input) The address of the memory that has to be
**                  freed
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. It is only called from within GKI if dynamic
**
*******************************************************************************/
void GKI_os_free (void *p_mem)
{
    if(p_mem != NULL)
        phOsalNfc_FreeMemory(p_mem);
    return;
}


/*******************************************************************************
**
** Function         GKI_suspend_task()
**
** Description      This function suspends the task specified in the argument.
**
** Parameters:      task_id  - (input) the id of the task that has to suspended
**
** Returns          GKI_SUCCESS if all OK, else GKI_FAILURE
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to implement task suspension capability,
**                  put specific code here.
**
*******************************************************************************/
UINT8 GKI_suspend_task (UINT8 task_id)
{
    GKI_TRACE_1("GKI_suspend_task %d - NOT implemented", task_id);


    GKI_TRACE_1("GKI_suspend_task %d done", task_id);

    return (GKI_SUCCESS);
}


/*******************************************************************************
**
** Function         GKI_resume_task()
**
** Description      This function resumes the task specified in the argument.
**
** Parameters:      task_id  - (input) the id of the task that has to resumed
**
** Returns          GKI_SUCCESS if all OK
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to implement task suspension capability,
**                  put specific code here.
**
*******************************************************************************/
UINT8 GKI_resume_task (UINT8 task_id)
{
    GKI_TRACE_1("GKI_resume_task %d - NOT implemented", task_id);

    GKI_TRACE_1("GKI_resume_task %d done", task_id);

    return (GKI_SUCCESS);
}


/*******************************************************************************
**
** Function         GKI_exit_task
**
** Description      This function is called to stop a GKI task.
**
** Parameters:      task_id  - (input) the id of the task that has to be stopped
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put specific code here to kill a task.
**
*******************************************************************************/
void GKI_exit_task (UINT8 task_id)
{
    GKI_disable();
    gki_cb.com.OSRdyTbl[task_id] = TASK_DEAD;

    GKI_enable();
#ifdef ANDROID
    GKI_send_event(task_id, EVENT_MASK(GKI_SHUTDOWN_EVT)); // this is not required for freertos
                                                           // as this event is already posted from GKI_Shutdown
#endif

    GKI_TRACE_1("GKI_exit_task %d done", task_id);
    return;
}


/*******************************************************************************
**
** Function         GKI_sched_lock
**
** Description      This function is called by tasks to disable scheduler
**                  task context switching.
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put code here to tell the OS to disable context switching.
**
*******************************************************************************/
void GKI_sched_lock(void)
{
    GKI_TRACE_0("GKI_sched_lock");
    GKI_disable ();
    return;
}


/*******************************************************************************
**
** Function         GKI_sched_unlock
**
** Description      This function is called by tasks to enable scheduler switching.
**
** Returns          void
**
** NOTE             This function is NOT called by the Widcomm stack and
**                  profiles. If you want to use it in your own implementation,
**                  put code here to tell the OS to re-enable context switching.
**
*******************************************************************************/
void GKI_sched_unlock(void)
{
    GKI_TRACE_0("GKI_sched_unlock");
    GKI_enable ();
}

/*******************************************************************************
**
** Function         GKI_shiftdown
**
** Description      shift memory down (to make space to insert a record)
**
*******************************************************************************/
void GKI_shiftdown (UINT8 *p_mem, UINT32 len, UINT32 shift_amount)
{
    register UINT8 *ps = p_mem + len - 1;
    register UINT8 *pd = ps + shift_amount;
    register UINT32 xx;

    for (xx = 0; xx < len; xx++)
        *pd-- = *ps--;
}

/*******************************************************************************
**
** Function         GKI_shiftup
**
** Description      shift memory up (to delete a record)
**
*******************************************************************************/
void GKI_shiftup (UINT8 *p_dest, UINT8 *p_src, UINT32 len)
{
    register UINT8 *ps = p_src;
    register UINT8 *pd = p_dest;
    register UINT32 xx;

    for (xx = 0; xx < len; xx++)
        *pd++ = *ps++;
}

//--------------------------------------------------------------------------
// NXP addition - used to synchronize starting of the top level task
//				and to synchronize the download process
//--------------------------------------------------------------------------
GKI_tBinarySem GKI_binary_sem_init(void)
{
	void *sem;
    if (phOsalNfc_CreateSemaphore(&sem, 0) != NFCSTATUS_SUCCESS)
	{
		GKI_TRACE_ERROR_1("%s: Failed to create semaphore", __func__);
	}
	return sem;
}

void GKI_binary_sem_destroy(GKI_tBinarySem semaphore)
{
	if (semaphore != NULL)
	{
		void * tempSem;
		tempSem = &semaphore;
	    if (phOsalNfc_DeleteSemaphore(tempSem) != NFCSTATUS_SUCCESS)
	    {
		    GKI_TRACE_ERROR_1("%s: Failed to delete semaphore", __func__);
	    }
	}
}

void GKI_binary_sem_post(GKI_tBinarySem semaphore)
{
	if (semaphore != NULL)
	{
	    if (phOsalNfc_ProduceSemaphore(semaphore) != NFCSTATUS_SUCCESS)
	    {
		    GKI_TRACE_ERROR_1("%s: Failed to post semaphore", __func__);
	    }
	}
}

void GKI_binary_sem_wait(GKI_tBinarySem semaphore)
{
	if (phOsalNfc_ConsumeSemaphore(semaphore) != NFCSTATUS_SUCCESS)
	{
		GKI_TRACE_ERROR_1("%s: Failed to wait on semaphore", __func__);
	}
}

UINT8 GKI_binary_sem_wait_timeout(GKI_tBinarySem semaphore, UINT32 timeout)
{
    if (phOsalNfc_ConsumeSemaphore_WithTimeout(semaphore, timeout) != NFCSTATUS_SUCCESS)
    {
        GKI_TRACE_ERROR_1("%s: Failed to wait on semaphore", __func__);
        return GKI_FAILURE;
    }
    return GKI_SUCCESS;
}

void GKI_kill_task(void * taskhandle)
{
	phOsalNfc_Thread_Delete(taskhandle);
}
