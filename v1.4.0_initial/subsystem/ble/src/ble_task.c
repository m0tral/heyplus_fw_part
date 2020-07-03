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
#include "ry_utils.h"
#include "ry_mcu_config.h"
#include "app_config.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
    #include "ry_log_id.h"
#endif

#if (RY_BLE_ENABLE == TRUE)

/* Global includes for this project. */
//#include "freertos_fit.h"
#include "ryos.h"
#include "am_util_debug.h"
#include "scheduler.h"
//*****************************************************************************
//
// FreeRTOS include files.
//
//*****************************************************************************
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "portable.h"
#include "semphr.h"
#include "event_groups.h"

/* WSF standard includes */
#include "wsf_types.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "wsf_timer.h"
#include "wsf_msg.h"

/* Includes for operating the ExactLE stack */
#include "hci_handler.h"
#include "dm_handler.h"
#include "l2c_handler.h"
#include "att_handler.h"
#include "smp_handler.h"
#include "l2c_api.h"
#include "dm_api.h"
#include "att_api.h"
#include "smp_api.h"
#include "app_api.h"
#include "hci_core.h"
#include "hci_drv.h"
#include "hci_drv_apollo.h"

#include "hci_apollo_config.h"

/* Includes for the ExactLE "fit" profile */
#include "fit_api.h"
#include "app_ui.h"
#include "app_main.h"

/* Reliable transer protocol and Mi profile related */
#include "ble_task.h"
#include "ryos.h"
#include "ryos_timer.h"
//#include "ry_hal_ble.h"

#include "mible.h"
#include "mible_arch.h"
//#include "reliable_xfer.h"
#include "r_xfer.h"

#include "svc_ryeex.h"
#include "svc_mi.h"
#include "svc_wechat.h"
#include "ry_hal_inc.h"
#include "svc_hrs.h"
#include "dip.h"
#include "wsf_os.h"
#include "alipay_api.h"

#include "device_management_service.h"


/*********************************************************************
 * CONSTANTS
 */ 

//#define RY_BLE_TEST

#define BLE_THREAD_PRIORITY        6
#define BLE_THREAD_STACK_SIZE      1024 // 1K Bytes




/*********************************************************************
 * TYPEDEFS
 */
 
/*! Application message type */
typedef union
{
  wsfMsgHdr_t     hdr;
  dmEvt_t         dm;
  attsCccEvt_t    ccc;
  attEvt_t        att;
} ble_msg_t;


/*********************************************************************
 * LOCAL VARIABLES
 */

/*
 * @brief BLE Thread handle
 */
ryos_thread_t *ble_thread_handle;

int ble_tx_ready = 1;

/*
 * @brief Mijia Auth Callback definition
 */
static void app_mible_auth_cb( u8 status );
static void ry_mible_auth_cb( u8 status );


/*
 * @brief Variable for Mijia callbacks
 */
static mible_appCbs_t mible_cb = {
    .authCbFunc     = app_mible_auth_cb,
	.wifiInfoCbFunc = NULL,
};

/*
 * @brief Variable for Mijia callbacks
 */
static mible_appCbs_t ry_mible_cb = {
    .authCbFunc     = ry_mible_auth_cb,
    .wifiInfoCbFunc = NULL,
};



static ble_stack_cb_t ble_stack_cb;



/*
 * @brief Event handler for exactLE stack.
 */
EventGroupHandle_t xRadioEventHandle;

static wsfHandlerId_t ry_ble_handler_id;


/*********************************************************************
 * LOCAL Configurations.
 */

//*****************************************************************************
//
// Timer configuration macros.
//
//*****************************************************************************
// Configure how to driver WSF Scheduler
#if 1
// Preferred mode to use when using FreeRTOS
#define USE_FREERTOS_TIMER_FOR_WSF
#else
// These are only test modes.
#ifdef AM_FREERTOS_USE_STIMER_FOR_TICK
#define USE_STIMER_FOR_WSF // Reuse FreeRTOS used STimer for WSF
#else
#define USE_CTIMER_FOR_WSF
#endif
#endif


// Use FreeRTOS timer for WSF Ticks
#ifdef USE_FREERTOS_TIMER_FOR_WSF
#define CLK_TICKS_PER_WSF_TICKS     (WSF_MS_PER_TICK*configTICK_RATE_HZ/1000)
#endif

#ifdef USE_CTIMER_FOR_WSF
#define WSF_CTIMER_NUM 	            1
#define CLK_TICKS_PER_WSF_TICKS     (WSF_MS_PER_TICK*512/1000)   // Number of CTIMER counts per WSF tick.

#if WSF_CTIMER_NUM == 0
#define WSF_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA0
#elif WSF_CTIMER_NUM == 1
#define WSF_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA1
#elif WSF_CTIMER_NUM == 2
#define WSF_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA2
#elif WSF_CTIMER_NUM == 3
#define WSF_CTIMER_INT 	AM_HAL_CTIMER_INT_TIMERA3
#endif
#endif

#ifdef USE_STIMER_FOR_WSF
#define CLK_TICKS_PER_WSF_TICKS     (WSF_MS_PER_TICK*configSTIMER_CLOCK_HZ/1000)   // Number of STIMER counts per WSF tick.
#endif

//*****************************************************************************
//
// WSF buffer pools.
//
//*****************************************************************************

#if WSF_USE_RY_MALLOC
#else
#define WSF_BUF_POOLS               5

// Important note: the size of g_pui32BufMem should includes both overhead of internal
// buffer management structure, wsfBufPool_t (up to 16 bytes for each pool), and pool 
// description (e.g. g_psPoolDescriptors below).

// Memory for the buffer pool
static uint32_t g_pui32BufMem[(1024 + 1024) / sizeof(uint32_t)];

// Default pool descriptor.
static wsfBufPoolDesc_t g_psPoolDescriptors[WSF_BUF_POOLS] =
{
    {  16,  8 },
    {  32,  4 },
    {  64,  6 },
    { 128,  4 },
    { 280,  4 },
};
#endif


wsfHandlerId_t g_bleDataReadyHandlerId;

//*****************************************************************************
//
// Tracking variable for the scheduler timer.
//
//*****************************************************************************
uint32_t g_ui32LastTime = 0;

void radio_timer_handler(void);
void ry_ble_handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);
void ry_ble_handler_init(wsfHandlerId_t handlerId);
void ble_data_ready_handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    HciDataReadyISR();
}

#ifdef USE_CTIMER_FOR_WSF
//*****************************************************************************
//
// Set up a pair of timers to handle the WSF scheduler.
//
//*****************************************************************************
void
scheduler_timer_init(void)
{
    // Enable the LFRC
    am_hal_clkgen_osc_start(AM_HAL_CLKGEN_OSC_LFRC);

    //
    // One of the timers will run in one-shot mode and provide interrupts for
    // scheduled events.
    //
    am_hal_ctimer_clear(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA);
    am_hal_ctimer_config_single(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA,
                                (AM_HAL_CTIMER_INT_ENABLE |
                                 AM_HAL_CTIMER_LFRC_512HZ |
                                 AM_HAL_CTIMER_FN_ONCE));

    //
    // The other timer will run continuously and provide a constant time-base.
    //
    am_hal_ctimer_clear(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERB);
    am_hal_ctimer_config_single(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERB,
                                (AM_HAL_CTIMER_LFRC_512HZ |
                                 AM_HAL_CTIMER_FN_CONTINUOUS));

    //
    // Start the continuous timer.
    //
    am_hal_ctimer_start(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERB);

    //
    // Enable the timer interrupt.
    //
    am_hal_ctimer_int_register(WSF_CTIMER_INT, radio_timer_handler);
    am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_CTIMER, configKERNEL_INTERRUPT_PRIORITY);
    am_hal_ctimer_int_enable(WSF_CTIMER_INT);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_CTIMER);
}

//*****************************************************************************
//
// Calculate the elapsed time, and update the WSF software timers.
//
//*****************************************************************************
void
update_scheduler_timers(void)
{
    uint32_t ui32CurrentTime, ui32ElapsedTime;

    //
    // Read the continuous timer.
    //
    ui32CurrentTime = am_hal_ctimer_read(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERB);

    //
    // Figure out how long it has been since the last time we've read the
    // continuous timer. We should be reading often enough that we'll never
    // have more than one overflow.
    //
    ui32ElapsedTime = ui32CurrentTime - g_ui32LastTime;

    //
    // Check to see if any WSF ticks need to happen.
    //
    if ( (ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS) > 0 )
    {
        //
        // Update the WSF timers and save the current time as our "last
        // update".
        //
        WsfTimerUpdate(ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS);

        g_ui32LastTime = ui32CurrentTime;
    }
}

//*****************************************************************************
//
// Set a timer interrupt for the next upcoming scheduler event.
//
//*****************************************************************************
void
set_next_wakeup(void)
{
    bool_t bTimerRunning;
    wsfTimerTicks_t xNextExpiration;

    //
    // Stop and clear the scheduling timer.
    //
    am_hal_ctimer_stop(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA);
    am_hal_ctimer_clear(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA);

    //
    // Check to see when the next timer expiration should happen.
    //
    xNextExpiration = WsfTimerNextExpiration(&bTimerRunning);

    //
    // If there's a pending WSF timer event, set an interrupt to wake us up in
    // time to service it. Otherwise, set an interrupt to wake us up in time to
    // prevent a double-overflow of our continuous timer.
    //
    if ( xNextExpiration )
    {
        am_hal_ctimer_period_set(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA,
                                 xNextExpiration * CLK_TICKS_PER_WSF_TICKS, 0);
    }
    else
    {
        am_hal_ctimer_period_set(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA, 0x8000, 0);
    }

    //
    // Start the scheduling timer.
    //
    am_hal_ctimer_start(WSF_CTIMER_NUM, AM_HAL_CTIMER_TIMERA);
}

//*****************************************************************************
//
// Interrupt handler for the CTIMERs
//
//*****************************************************************************
void
radio_timer_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;

    //
    // Set an event to wake up the radio task.
    //
    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(xRadioEventHandle, 1,
                                        &xHigherPriorityTaskWoken);

    //
    // If the radio task is higher-priority than the context we're currently
    // running from, we should yield now and run the radio task.
    //
    if ( xResult != pdFAIL )
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
#endif
#ifdef USE_STIMER_FOR_WSF
//*****************************************************************************
//
// Reuse STIMER to handle the WSF scheduler.
//
//*****************************************************************************
void
scheduler_timer_init(void)
{
    //
    // USe CMPR1 of STIMER for one-shot mode interrupts for
    // scheduled events.
    //
    uint32_t cfgVal;
    /* Stop the Stimer momentarily.  */
    cfgVal = am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);


    //
    // Configure the STIMER->COMPARE_1
    //
    am_hal_stimer_compare_delta_set(1, CLK_TICKS_PER_WSF_TICKS);
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREB);
    am_hal_stimer_config(cfgVal | AM_HAL_STIMER_CFG_COMPARE_B_ENABLE);

    //
    // Enable the timer interrupt in the NVIC, making sure to use the
    // appropriate priority level.
    //
    am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_STIMER_CMPR1, configKERNEL_INTERRUPT_PRIORITY);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_STIMER_CMPR1);

    //
    // Reuse STIMER to provide a constant time-base.
    //

}

//*****************************************************************************
//
// Calculate the elapsed time, and update the WSF software timers.
//
//*****************************************************************************
void
update_scheduler_timers(void)
{
    uint32_t ui32CurrentTime, ui32ElapsedTime;

    //
    // Read the continuous timer.
    //
    ui32CurrentTime = am_hal_stimer_counter_get();

    //
    // Figure out how long it has been since the last time we've read the
    // continuous timer. We should be reading often enough that we'll never
    // have more than one overflow.
    //
    ui32ElapsedTime = ui32CurrentTime - g_ui32LastTime;

    //
    // Check to see if any WSF ticks need to happen.
    //
    if ( (ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS) > 0 )
    {
        //
        // Update the WSF timers and save the current time as our "last
        // update".
        //
        WsfTimerUpdate(ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS);

        g_ui32LastTime = ui32CurrentTime;
    }
}

//*****************************************************************************
//
// Set a timer interrupt for the next upcoming scheduler event.
//
//*****************************************************************************
void
set_next_wakeup(void)
{
    bool_t bTimerRunning;
    wsfTimerTicks_t xNextExpiration;
    uint32_t cfgVal;
    uint32_t ui32Critical;

    //
    // Check to see when the next timer expiration should happen.
    //
    xNextExpiration = WsfTimerNextExpiration(&bTimerRunning);

    //
    // If there's a pending WSF timer event, set an interrupt to wake us up in
    // time to service it. Otherwise, set an interrupt to wake us up in time to
    // prevent a double-overflow of our continuous timer.
    //

    /* Enter a critical section */
    ui32Critical = am_hal_interrupt_master_disable();

    /* Stop the Stimer momentarily.  */
    cfgVal = am_hal_stimer_config(AM_HAL_STIMER_CFG_FREEZE);
    //
    // Configure the STIMER->COMPARE_1
    //
    if ( xNextExpiration )
    {
        am_hal_stimer_compare_delta_set(1, xNextExpiration * CLK_TICKS_PER_WSF_TICKS);
    }
    else
    {
        am_hal_stimer_compare_delta_set(1, 0x80000000);
    }

    /* Exit Critical Section */
    am_hal_interrupt_master_set(ui32Critical);

    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREB);
    am_hal_stimer_config(cfgVal);

}

//*****************************************************************************
//
// Interrupt handler for the CTIMERs
//
//*****************************************************************************
void
radio_timer_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;

	//
    // Set an event to wake up the radio task.
    //
    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(xRadioEventHandle, 1,
                                        &xHigherPriorityTaskWoken);

    //
    // If the radio task is higher-priority than the context we're currently
    // running from, we should yield now and run the radio task.
    //
    if ( xResult != pdFAIL )
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
//*****************************************************************************
//
// Interrupt handler for the STIMER module Compare 1.
//
//*****************************************************************************
void
am_stimer_cmpr1_isr(void)
{
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREB);

    //
    // Run handlers for the various possible timer events.
    //
    radio_timer_handler();

}
#endif
#ifdef USE_FREERTOS_TIMER_FOR_WSF
TimerHandle_t xWsfTimer;
//*****************************************************************************
//
// Callback handler for the FreeRTOS Timer
//
//*****************************************************************************
void
wsf_timer_handler(TimerHandle_t xTimer)
{
    BaseType_t xResult;

    xResult = xEventGroupSetBits(xRadioEventHandle, 1);

    //
    // If the radio task is higher-priority than the context we're currently
    // running from, we should yield now and run the radio task.
    //
    if ( xResult != pdFAIL )
    {
        portYIELD();
    }
}
//*****************************************************************************
//
// Reuse FreeRTOS TIMER to handle the WSF scheduler.
//
//*****************************************************************************
void
scheduler_timer_init(void)
{
    // Create a FreeRTOS Timer
    xWsfTimer = xTimerCreate("WSF Timer", pdMS_TO_TICKS(WSF_MS_PER_TICK),
            pdFALSE, NULL, wsf_timer_handler);
    configASSERT(xWsfTimer);
}

//*****************************************************************************
//
// Calculate the elapsed time, and update the WSF software timers.
//
//*****************************************************************************
void
update_scheduler_timers(void)
{
    uint32_t ui32CurrentTime, ui32ElapsedTime;

    //
    // Read the continuous timer.
    //
    ui32CurrentTime = xTaskGetTickCount();

    //
    // Figure out how long it has been since the last time we've read the
    // continuous timer. We should be reading often enough that we'll never
    // have more than one overflow.
    //
    ui32ElapsedTime = ui32CurrentTime - g_ui32LastTime;

    //
    // Check to see if any WSF ticks need to happen.
    //
    if ( (ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS) > 0 )
    {
        //
        // Update the WSF timers and save the current time as our "last
        // update".
        //
//        WsfTimerUpdate(ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS);

        g_ui32LastTime = ui32CurrentTime;
    }
}

//*****************************************************************************
//
// Set a timer interrupt for the next upcoming scheduler event.
//
//*****************************************************************************
void
set_next_wakeup(void)
{
    bool_t bTimerRunning;
    wsfTimerTicks_t xNextExpiration;

    //
    // Check to see when the next timer expiration should happen.
    //
    xNextExpiration = WsfTimerNextExpiration(&bTimerRunning);

    //
    // If there's a pending WSF timer event, set an interrupt to wake us up in
    // time to service it.
    //
    if ( xNextExpiration )
    {
        configASSERT(pdPASS == xTimerChangePeriod( xWsfTimer,
                pdMS_TO_TICKS(xNextExpiration*CLK_TICKS_PER_WSF_TICKS), 100)) ;
    }
}

#endif

static void RyBlewsfBufDiagCback(WsfBufDiag_t *pInfo)
{
    LOG_ERROR("[wsf_malloc] alloc failed len:%d\n", pInfo->param.alloc.len);
    ble_hci_reset();
}

//*****************************************************************************
//
// Initialization for the ExactLE stack.
//
//*****************************************************************************
void
exactle_stack_init(void)
{
    wsfHandlerId_t handlerId;

    //
    // Set up timers for the WSF scheduler.
    //
    scheduler_timer_init();
    WsfTimerInit();

#if WSF_USE_RY_MALLOC
#else

    //
    // Initialize a buffer pool for WSF dynamic memory needs.
    //
    WsfBufInit(sizeof(g_pui32BufMem), (uint8_t *)g_pui32BufMem, WSF_BUF_POOLS,
               g_psPoolDescriptors);
    WsfBufDiagRegister(RyBlewsfBufDiagCback);
#endif


    //
    // Initialize the WSF security service.
    //
    SecInit();
    SecAesInit();
    SecCmacInit();
    SecEccInit();

    //
    // Set up callback functions for the various layers of the ExactLE stack.
    //
    handlerId = WsfOsSetNextHandler(HciHandler);
    HciHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(DmHandler);
    DmDevVsInit(0);
    DmAdvInit();
    DmConnInit();
    DmConnSlaveInit();
    DmSecInit();
    DmSecLescInit();
    DmPrivInit();
    DmHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(L2cSlaveHandler);
    L2cSlaveHandlerInit(handlerId);
    L2cInit();
    L2cSlaveInit();

    handlerId = WsfOsSetNextHandler(AttHandler);
    AttHandlerInit(handlerId);
    AttsInit();
    AttsIndInit();
    AttcInit();

    handlerId = WsfOsSetNextHandler(SmpHandler);
    SmpHandlerInit(handlerId);
    SmprInit();
    SmprScInit();

    HciSetMaxRxAclLen(251);

    handlerId = WsfOsSetNextHandler(AppHandler);
    AppHandlerInit(handlerId);

    handlerId = WsfOsSetNextHandler(FitHandler);
    FitHandlerInit(handlerId);

    g_bleDataReadyHandlerId = WsfOsSetNextHandler(ble_data_ready_handler);

    handlerId = WsfOsSetNextHandler(mible_arch_handler);
    mible_arch_init(handlerId);

    handlerId = WsfOsSetNextHandler(ry_ble_handler);
    ry_ble_handler_init(handlerId);
    
}


//*****************************************************************************
//
// UART interrupt handler.
//
//*****************************************************************************
void
am_uart_isr(void)
{
    uint32_t ui32Status;
    BaseType_t xHigherPriorityTaskWoken, xResult;

    //
    // Read and save the interrupt status, but clear out the status register.
    //
    ui32Status = AM_REGn(UART, 0, MIS);
    AM_REGn(UART, 0, IEC) = ui32Status;

    //
    // Allow the HCI driver to respond to the interrupt.
    //
    //HciDrvUartISR(ui32Status);

    //
    // Send an event to wake up the radio task.
    //
    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(xRadioEventHandle, 1,
                                        &xHigherPriorityTaskWoken);

    //
    // If the radio task is higher-priority than the context we're currently
    // running from, we should yield now and run the radio task.
    //
    if ( xResult != pdFAIL )
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

//*****************************************************************************
//
// Interrupt handler for the CTS pin
//
//*****************************************************************************
void
radio_cts_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken, xResult;

    //
    // Send an event to the main radio task
    //
    xHigherPriorityTaskWoken = pdFALSE;

    xResult = xEventGroupSetBitsFromISR(xRadioEventHandle, 1,
                                        &xHigherPriorityTaskWoken);

    //
    // If the radio task is higher-priority than the context we're currently
    // running from, we should yield now and run the radio task.
    //
    if ( xResult != pdFAIL )
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}


void em9304_int_handler(void)
{
    radio_cts_handler();
}

/**
 * @brief   API to start advertisement
 *
 * @param   none
 *
 * @return  None
 */
void ry_ble_startAdv(void)
{

}


/**
 * @brief   API to send notification to peer device.
 *
 * @param   connHandle, the connection handle of peer device
 * @param   charUUID, which char value to be sent
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
void ry_ble_cccValUpdate()
{

}


void wakeup_ble_thread(void)
{
    BaseType_t xResult;

    //LOG_DEBUG("[wakeup_ble_thread], cur_tick:%d\n", xTaskGetTickCount());

    /* Wakeup the ble thread to work */
    xResult = xEventGroupSetBits(xRadioEventHandle, 1);
    //
    // If the radio task is higher-priority than the context we're currently
    // running from, we should yield now and run the radio task.
    //
    if ( xResult != pdFAIL )
    {
        portYIELD();
    }
}


typedef struct {

    dmConnId_t connHandle;
    u16_t      charHandle;
    u8_t       len;
    u8_t       data[RYEEXS_CHAR_MAX_DATA];
} ry_ble_data_t;

static void ry_ble_send(uint16_t connHandle, uint16_t charHandle, uint8_t len, uint8_t* data)
{
  ry_ble_data_t *pMsg;
  
  if ((pMsg = WsfMsgAlloc(sizeof(ry_ble_data_t))) != NULL)
  {
    pMsg->connHandle = connHandle;
    pMsg->charHandle = charHandle;
    pMsg->len = len;
    ry_memcpy(pMsg->data, data, len);
    WsfMsgSend(ry_ble_handler_id, pMsg);
    wakeup_ble_thread();
  }
}

void ry_ble_handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    ry_ble_data_t *p = (ry_ble_data_t*)pMsg;
    //LOG_DEBUG("[ry_ble_handler] connid:%d, handle:0x%04x, len:%d\n", p->connHandle, p->charHandle, p->len);
    //ry_data_dump(p->data, p->len, ' ');
    AttsHandleValueNtf(p->connHandle, p->charHandle, p->len, p->data);
    wakeup_ble_thread();
    //WsfMsgFree(pMsg);
}


/**
 * @brief   API to send notification to peer device.
 *
 * @param   connHandle, the connection handle of peer device
 * @param   charUUID, which char value to be sent
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
void ry_ble_sendNotify(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data)
{
    /* send notification */
    #if 0
    if (AttsCccEnabled(connHandle, 6)) {
        if (charUUID == MISERVICE_CHAR_TOKEN_UUID) {
            AttsHandleValueNtf((dmConnId_t)connHandle, MIS_TOKEN_VAL_HDL, len, data);
        }
    } else {
        LOG_WARN("[ry_ble_sendNotify]: CCC not enabled.\n");
    }

    wakeup_ble_thread();
    #endif

    ry_ble_send((dmConnId_t)connHandle, MIS_TOKEN_VAL_HDL, len, data);
} 


/**
 * @brief   API to send notification with Ryeex Reliable protocol.
 *
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
u32_t ry_ble_usrdata;
ry_sts_t ble_reliableSend(uint8_t* data, uint16_t len, void *usrdata)
{
    //LOG_DEBUG("[ble_reliableSend]: len = %d.\n", len);
    //ry_data_dump(data, len, ' ');
    
    //AttsHandleValueNtf((dmConnId_t)0x0001, RYEEXS_SECURE_CH_VAL_HDL, len, data);
    //wakeup_ble_thread();
    ry_ble_usrdata = (u32_t)usrdata;
    ry_ble_send((dmConnId_t)0x0001, RYEEXS_SECURE_CH_VAL_HDL, len, data);
    
    return RY_SUCC;      
}

/**
 * @brief   API to send notification with Ryeex Reliable protocol.
 *
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
ry_sts_t ble_reliableUnsecureSend(uint8_t* data, uint16_t len, void *usrdata)
{
    //LOG_DEBUG("[ble_reliableUnsecureSend]: len = %d.\n", len);
    //ry_data_dump(data, len, ' ');

    //AttsHandleValueNtf((dmConnId_t)0x0001, RYEEXS_UNSECURE_CH_VAL_HDL, len, data);
    //wakeup_ble_thread();
    ry_ble_usrdata = (u32_t)usrdata;
    ry_ble_send((dmConnId_t)0x0001, RYEEXS_UNSECURE_CH_VAL_HDL, len, data);
    
    return RY_SUCC;
    
}

ry_sts_t ble_hrDataSend(uint8_t* data, uint16_t len)
{
    LOG_DEBUG("[ble_hrDataSend]: len = %d.\n", len);
    ry_ble_send((dmConnId_t)0x0001, HRS_HRM_HDL, len, data);
    return RY_SUCC;
}


/**
 * @brief   API to send notification with Ryeex Reliable protocol.
 *
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
ry_sts_t ble_jsonSend(uint8_t* data, uint16_t len, void *usrdata)
{
    //LOG_DEBUG("[ble_jsonSend]: len = %d.\n", len);
    //ry_data_dump(data, len, ' ');

    //AttsHandleValueNtf((dmConnId_t)0x0001, RYEEXS_UNSECURE_CH_VAL_HDL, len, data);
    //wakeup_ble_thread();
    ry_ble_usrdata = (u32_t)usrdata;
    ry_ble_send((dmConnId_t)0x0001, RYEEXS_JSON_CH_VAL_HDL, len, data);
    
    return RY_SUCC;
    
}


void ry_ble_proc_msg(void* pStackMsg)
{
    ble_msg_t* pMsg = (ble_msg_t*)pStackMsg;
    dmEvt_t* pDmMsg = (dmEvt_t*)pStackMsg;
    attEvt_t *p = (attEvt_t *)&pMsg->hdr;

    switch (pMsg->hdr.event) {

        case DM_RESET_CMPL_IND:                  
            LOG_DEBUG("[ble_task] DM_RESET_CMPL_IND\n");
        
//#warning TODO: clear connection db info
            ry_memset(appConnCb, 0, sizeof(appConnCb));
            ble_stack_cb(BLE_STACK_EVT_INIT_DONE, NULL);
            break;
        
        case DM_CONN_OPEN_IND:
            {
                LOG_DEBUG("[ble_task] DM_CONN_OPEN_IND\n");
                ble_stack_connected_t connected;
                /* store connection ID */
                connected.connID       = (dmConnId_t) pDmMsg->hdr.param;
                connected.connInterval = pDmMsg->connOpen.connInterval;
                connected.latency      = pDmMsg->connOpen.connLatency;
                connected.timeout      = pDmMsg->connOpen.supTimeout;
                ry_memcpy(connected.peerAddr, pDmMsg->connOpen.peerAddr, 6);
                //AttcMtuReq(connected.connID, 155);
                ble_stack_cb(BLE_STACK_EVT_CONNECTED, (void*)&connected);
            }
            break;


        case DM_CONN_CLOSE_IND:
            LOG_DEBUG("[ble_task] disconnect_reason: %d\n", pDmMsg->connClose.reason);
        
            ble_stack_disconnected_t disconnected;
            disconnected.connID = pDmMsg->connClose.handle;
            disconnected.reason = pDmMsg->connClose.reason;
            ble_stack_cb(BLE_STACK_EVT_DISCONNECTED, (void*)&disconnected);
            break;


        case DM_CONN_UPDATE_IND:
            {
                ble_stack_connected_t updated;
                updated.connID       = pDmMsg->connUpdate.handle;
                updated.connInterval = pDmMsg->connUpdate.connInterval;
                updated.latency      = pDmMsg->connUpdate.connLatency;
                updated.timeout      = pDmMsg->connUpdate.supTimeout;
                ble_stack_cb(BLE_STACK_EVT_CONN_PARA_UPDATED, (void*)&updated);
            }
            break;
        case DM_CONN_DATA_LEN_CHANGE_IND:                   
            {
                if (pDmMsg->dataLenChange.maxTxOctets == 27 && pDmMsg->dataLenChange.maxRxOctets > 27) {
                    
                    LOG_INFO("[ble_task] tx pdu err update accord to rx pdu: %d\n", pDmMsg->dataLenChange.maxRxOctets);
                    
                    DmConnSetDataLen(pDmMsg->dataLenChange.handle,
                            pDmMsg->dataLenChange.maxRxOctets,
                            pDmMsg->dataLenChange.maxRxTime);
                }
                else if (pDmMsg->dataLenChange.maxTxOctets > 27 && pDmMsg->dataLenChange.maxRxOctets > 27)
                {
                    uint32_t chgConnMtu = pDmMsg->dataLenChange.maxTxOctets;
                    if (pDmMsg->dataLenChange.maxRxOctets <= chgConnMtu)
                        chgConnMtu = pDmMsg->dataLenChange.maxRxOctets;
                        
                    uint32_t curConnMtu = ry_ble_get_cur_mtu() + 4;
                    
                    if (curConnMtu > chgConnMtu) {
                    
                        LOG_INFO("[ble_task] pdu less than mtu request mtu base on pdu: %d\n", chgConnMtu);
                                                
                        if (is_deepsleep_active()) {
                            // call based on ver 1.8.10
                            //AttcMtuReq(pDmMsg->dataLenChange.handle, chgConnMtu - 4);                        
                            
                            AttcMtuReq(pDmMsg->dataLenChange.handle, 23);
                        }
                        else {
                        
                            // call based on ver 1.13.59                        
                            ble_stack_mtu_updated_t mtu_evt;
                            mtu_evt.mtu = 23;
                            mtu_evt.connID = pDmMsg->dataLenChange.handle;
                            
                            ble_stack_cb(BLE_STACK_EVT_MTU_GOT, (void*)&mtu_evt);
                        }
                    }
                }
            }
            break;

        case DM_ADV_START_IND:
            ble_stack_cb(BLE_STACK_EVT_ADV_STARTED, NULL);
            break;

        case DM_ADV_STOP_IND:
            ble_stack_cb(BLE_STACK_EVT_ADV_STOPED, NULL);
            break;
            

        case ATTS_HANDLE_VALUE_CNF:
            /* Log the notification status */
            if ((p->handle > MIS_START_HDL && p->handle < MIS_END_HDL) || 
                (p->handle > RYEEXS_START_HDL && p->handle < RYEEXS_END_HDL)) {
                //LOG_DEBUG("[ry_ble_proc_msg] Value Confirm. Status = %d, Handle = 0x%04xx\n", 
                //    p->hdr.status, p->handle);
            }

            /* Log the notification status */
            if ((p->handle == RYEEXS_SECURE_CH_VAL_HDL) || 
                (p->handle == RYEEXS_UNSECURE_CH_VAL_HDL) || 
                (p->handle == RYEEXS_JSON_CH_VAL_HDL)) {
                //ry_ble_handle_value_cnf((attEvt_t *) pMsg);

                if (p->hdr.status == ATT_SUCCESS) {
                    //if (ble_tx_ready == 0) {
                        ble_tx_ready = 1;
                        //r_xfer_tx_recover();
                        r_xfer_on_send_cb((void*)ry_ble_usrdata);
                        
                    //}
                } else if (p->hdr.status == ATT_ERR_OVERFLOW) {
                    LOG_DEBUG("[ry_ble_proc_msg] Failed len:%d SN:%02x %02x\n", p->valueLen, p->pValue[0], p->pValue[1]);
                    ble_tx_ready = 0;
                    //LOG_WARN("[ry_ble_proc_msg] Reliable Notify Confirm not success!\n");
                } else if(p->hdr.status == 0x33){
                    r_xfer_on_send_cb((void*)ry_ble_usrdata);
                }
            }
            break;
        case ATT_MTU_UPDATE_IND:
            {
                ble_stack_mtu_updated_t mtu_evt;
                mtu_evt.mtu = ((attEvt_t *)pMsg)->mtu;
                mtu_evt.connID = ((attEvt_t *)pMsg)->handle;
                ble_stack_cb(BLE_STACK_EVT_MTU_GOT, (void*)&mtu_evt);
            }
            break;
        default:
            break;
    }    
}

#if 0
void ry_ble_proc_msg(wsfMsgHdr_t *pMsg)
{
    attEvt_t *p = (attEvt_t *)pMsg;
    if (pMsg->event == DM_CONN_OPEN_IND) {
        LOG_DEBUG("[ry_ble_proc_msg] Connection Open.\n");
        ble_stack_cb(BLE_STACK_EVT_STATE_CHANGED, ); 
        
    } else if (pMsg->event == ATTS_HANDLE_VALUE_CNF) {
        /* Log the notification status */
        if ((p->handle > MIS_START_HDL && p->handle < MIS_END_HDL) || 
            (p->handle > RYEEXS_START_HDL && p->handle < RYEEXS_END_HDL)) {
            LOG_DEBUG("[ry_ble_proc_msg] Value Confirm. Status = %d, Handle = 0x%04xx\n", 
                p->hdr.status, p->handle);
        }

        /* Log the notification status */
        if ((p->handle == RYEEXS_SECURE_CH_VAL_HDL) || 
            (p->handle == RYEEXS_UNSECURE_CH_VAL_HDL) || 
            (p->handle == RYEEXS_JSON_CH_VAL_HDL)) {
            //ry_ble_handle_value_cnf((attEvt_t *) pMsg);

            if (p->hdr.status == ATT_SUCCESS) {
                //if (ble_tx_ready == 0) {
                    ble_tx_ready = 1;
                    //r_xfer_tx_recover();
                    r_xfer_on_send_cb((void*)ry_ble_usrdata);
                    
                //}
            } else if (p->hdr.status == ATT_ERR_OVERFLOW) {
                LOG_DEBUG("[ry_ble_proc_msg] Failed len:%d SN:%02x %02x\n", p->valueLen, p->pValue[0], p->pValue[1]);
                ble_tx_ready = 0;
                //LOG_WARN("[ry_ble_proc_msg] Reliable Notify Confirm not success!\n");
            }
        }
    } 
}
#endif

/**
 * @brief   API to set characteristic value
 *
 * @param   connHandle, the connection handle
 * @param   charUUID, which characteristic to be set
 * @param   len, length of the char data
 * @param   data, content of the char data to be set
 *
 * @return  None
 */
void ry_ble_setValue(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data )
{
}


/**
 * @brief   Cypress BLE Event handler function.
 *
 * @param   Event, defined in cy_ble_stack_gap.h
 * @param   eventParameter, the parameter of the event
 *
 * @return  None
 */
uint8_t *t_test_data;

static void ry_ble_eventHandler(uint32_t event, void *eventParameter)
{

}


/**
 * @brief   Callback of MIOT BLE binding status
 *
 * @param   Status binding status
 *
 * @return  None
 */
u32_t ble_test_timer_id;
//extern void test_rbp_send();

void ble_test_timer_handler(void* arg)
{
    //test_rbp_send();
    LOG_DEBUG("[ble_test_timer_handler]: Send Test RBP Message.\n");
}


/**
 * @brief   API to update connection parameters
 *
 * @param   intervalMin, minimal connection interval
 * @param   intervalMax, maximal connection interval
 * @param   timeout,     timeout of the connection
 * @param   slaveLatency, slave lantency value
 *
 * @return  Status.
 */
ry_sts_t ble_updateConnPara(uint32_t connId, u16_t intervalMin, u16_t intervalMax, u16_t timeout, u16_t slaveLatency)
{
    hciConnSpec_t connSpec;
    
    connSpec.connIntervalMin = intervalMin;
    connSpec.connIntervalMax = intervalMax;
    connSpec.connLatency     = slaveLatency;
    connSpec.supTimeout      = timeout;
    connSpec.minCeLen        = 0;
    connSpec.maxCeLen        = 0xffff;

    //L2cDmConnUpdateReq((dmConnId_t)0x0001, &connSpec);
    LOG_DEBUG("ble_updateConnPara %d, %d\r\n", connSpec.connIntervalMin, connSpec.connIntervalMax);
    DmConnUpdate((dmConnId_t)connId, &connSpec);
    wakeup_ble_thread();

    return RY_SUCC;
}





static void app_mible_auth_cb( u8 status )
{
    LOG_DEBUG("[app_mible_auth_cb], status = %d----------------------------------------\r\n", status);

    if (status != MIBLE_SUCC) {
        //arch_ble_disconnect(connectionHandle.bdHandle);
    } else {
        device_bind_mijia(1);
        ry_sched_msg_send(IPC_MSG_TYPE_BIND_SUCC, 0, NULL);
        ry_system_lifecycle_schedule();
    }

}


static void ry_mible_auth_cb( u8 status )
{
    LOG_DEBUG("[ry_mible_auth_cb], status = %d----------------------------------------\r\n", status);

    if (status != MIBLE_SUCC) {
        //arch_ble_disconnect(connectionHandle.bdHandle);
        LOG_ERROR("RY_MIBLE auth fail.\r\n");
    }

}



/**
 * @brief   API to init MIOT BLE module
 *
 * @param   None
 *
 * @return  None
 */
void miio_ble_init(void)
{
    mible_init_t init_para;
    char ver[10] = {0};// = {"0.2.78"};
    u8_t len;
    //uint8_t publicAddr[6] = {0x20, 0x71, 0x9B, 0x19, 0x3E, 0x41};
    //uint8_t publicAddr[6] = {0x00, 0xA0, 0x50, 0x00, 0x00, 0x00};
    //uint8_t publicAddr[6] = {0x0C, 0xF3, 0xEE, 0x70, 0x41, 0x08};

    /* Set Public Address to MIOT BLE module */
    //arch_ble_setPublicAddr(publicAddr);

    /* Set FW Version Number */
    //ry_ble_setValue(connectionHandle.bdHandle, MISERVICE_CHAR_VER_UUID, 10, (u8_t*)ver);

    device_info_get(DEV_INFO_TYPE_VER, (uint8_t*)ver, &len);

    /* Init MIOT BLE */
    init_para.productID    = APP_PRODUCT_ID;
    init_para.relationship = MIBLE_RELATIONSHIP_WEAK;
    init_para.authTimeout  = 15000;
    init_para.cbs          = &mible_cb;
    ry_memcpy(init_para.version, ver, 10);
    
    mible_init(&init_para, MIBLE_TYPE_MIJIA);


    init_para.productID    = APP_PRODUCT_ID;
    init_para.relationship = MIBLE_RELATIONSHIP_WEAK;
    init_para.authTimeout  = 15000;
    init_para.cbs          = &ry_mible_cb;
    ry_memcpy(init_para.version, ver, 10);
    mible_init(&init_para, MIBLE_TYPE_RYEEX);
}



void ry_ble_handler_init(wsfHandlerId_t handlerId)
{
    ry_ble_handler_id = handlerId;
}



/**
 * @brief   BLE Main task
 *
 * @param   None
 *
 * @return  None
 */
int ble_thread(void* arg)
{
    LOG_INFO("[ble thread]: Enter.\r\n");

    /* Init device property */
    //device_info_init();
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_BLE);
#endif

    
    /* Init Ambiq_EM9304  */
    am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_GPIO, configMAX_SYSCALL_INTERRUPT_PRIORITY);

    /* Register an interrupt handler for BLE event  */
    //am_hal_gpio_int_register(AM_BSP_GPIO_EM9304_INT, radio_cts_handler);

#if WSF_TRACE_ENABLED == TRUE
    //
    // Enable ITM
    //
    am_util_debug_printf("Starting wicentric trace:\n\n");
#endif

    //
    // Initialize the main ExactLE stack.
    //
    exactle_stack_init();

    //
    // Enable BLE data ready interrupt
    //
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);

    //
    // Start the "Fit" profile.
    //
    FitStart();


    /* Init Mi BLE */
    miio_ble_init();
    
    while (1) {
        //
        // Calculate the elapsed time from our free-running timer, and update
        // the software timers in the WSF scheduler.
        //
        update_scheduler_timers();
        wsfOsDispatcher();

        //
        // Enable an interrupt to wake us up next time we have a scheduled
        // event.
        //
        set_next_wakeup();

        //
        // Check to see if the WSF routines are ready to go to sleep.
        //
        if ( wsfOsReadyToSleep() )
        {
            wsfMsgHdr_t  *pMsg;
            //
            // Attempt to shut down the UART. If we can shut it down
            // successfully, we can go to deep sleep. Otherwise, we'll need to
            // stay awake to finish processing the current packet.
            //
            //LOG_INFO("[ble thread]: Event Enter Wait.\r\n");

            //
            // Wait for an event to be posted to the Radio Event Handle.
            //
            xEventGroupWaitBits(xRadioEventHandle, 1, pdTRUE,
                                pdFALSE, portMAX_DELAY);
            pMsg = WsfMsgAlloc(0);
            RY_ASSERT(pMsg != NULL);
            WsfMsgSend(g_bleDataReadyHandlerId, pMsg);
        }
    }
}


/**
 * @brief   API to init BLE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ble_init(ble_stack_cb_t stack_cb)
{
    ryos_threadPara_t para;
    ry_sts_t status = RY_SUCC;

    ble_stack_cb = stack_cb;

    /* Create an event handle for our wake-up events. */
    xRadioEventHandle = xEventGroupCreate();

    /* Make sure we actually allocated space for the events we need. */
    while ( xRadioEventHandle == NULL );

    /* Boot the radio. */
    ry_hal_gpio_int_enable(GPIO_IDX_EM9304_IRQ, em9304_int_handler);
    HciDrvRadioBoot(0);

    /* Start Test Demo Thread */
    strcpy((char*)para.name, "ble_thread");
    para.stackDepth = 768;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = BLE_THREAD_PRIORITY;
    ryos_thread_create(&ble_thread_handle, ble_thread, &para);
    
    return status;
}


// will reset and start fast adv 
void ble_hci_reset(void)
{
    HciDrvRadioBoot(0);
    DmDevReset();
}



/***********************************Test*****************************************/



#endif /* (RY_BLE_ENABLE == TRUE) */



