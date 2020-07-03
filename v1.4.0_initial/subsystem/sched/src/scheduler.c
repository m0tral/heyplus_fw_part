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
#include "app_config.h"
#include "ry_utils.h"
#include "ryos.h"
#include "scheduler.h"

#include "hrm.h"
#include "gui.h"
#include "gui_bare.h"
#include "sensor_alg.h"
#include "app_interface.h"
#include "testbench.h"
#include "ry_ble.h"
#include "ry_tool.h"
#include "ry_fs.h"
#include <sensor_alg.h>
#include "touch.h"
#include "ry_nfc.h"
#include "ry_hal_mcu.h"

#include "window_management_service.h"
#include "card_management_service.h"
#include "data_management_service.h"
#include "device_management_service.h"
#include "timer_management_service.h"
#include "storage_management_service.h"
#include "mijia_service.h"
#include "mibeacon.h"

#include "led.h"
#include "motar.h"
#include "am_hal_reset.h"
#include "ryos_update.h"
#include "r_xfer.h"
#include "activity_qrcode.h"
#include "surface_management_service.h"
#include "activity_surface.h"
#include "msg_management_service.h"
#include "ry_gui.h"
#include "ry_gsensor.h"
#include "app_management_service.h"
#include "SeApi_Int.h"
#include "ry_statistics.h"
#include "ry_hal_inc.h"
#include "ry_font.h"


#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

/*********************************************************************
 * CONSTANTS
 */ 

#define SCHEDULER_THREAD_PRIORITY         7
#define SCHEDULER_THREAD_STACK_SIZE       512

#define IPC_MSG_MAX_NUM                   20


/*********************************************************************
 * TYPEDEFS
 */

typedef void ( *ry_system_stateHandler_t )(ry_ipc_msg_t* msg); 
typedef ry_sts_t ( *ry_system_requestHandler_t )(ry_ipc_msg_t* msg); 



/*
 * @brief System state machine
 */
typedef struct {
    ry_system_state_t        curState;  /*! The current state of mi service */
    u32_t                    msgType;   /*! The event ID */
    ry_system_stateHandler_t handler;   /*! The corresponding event handler */
} ry_system_stateMachine_t;


typedef struct {
    dev_lifecycle_t             lifecycle;
    //dev_power_state_t           power;
    dev_session_t               session;
    u32_t                       request;
    ry_system_requestHandler_t  handler;
} ry_app_stateMachine_t;

/*
 * @brief System context
 */
typedef struct {
    ry_system_state_t        curState;

    wm_touchEventHandler_t   touchEvtCb; // TODO: Should move to WM
    system_rtcEventHandler_t rtcEvtCb;   // TODO: Should add a list
    system_algEventHandler_t algEvtCb;
    system_algEventHandler_t alg_still_long_EvtCb;    
    system_algEventHandler_t alg_weared_changed_EvtCb;        
    system_bleEventHandler_t bleEvtCb;
    system_msgEventHandler_t msgEvtCb;
    system_nfcEventHandler_t nfcEvtCb;
    system_sysEventHandler_t sysEvtCb;
    system_cardEventHandler_t cardEvtCb;
    system_surfaceEventHandler_t surfaceEvtCb;
    system_surfaceEventHandler_t sportEvtCb;
    system_findPhoneEventHandler_t findPhoneEvtCb;
    system_alarmRingEventHandler_t alarmRingEvtCb;
    system_alipayEventHandler_t alipayEvtCb;
    uint8_t                   sys_initialized;
    uint8_t                   lifecycle;
    uint8_t                   alg_thread_suspend;    
} ry_system_ctx_t;
 
/*********************************************************************
 * LOCAL VARIABLES
 */

/*
 * @brief Scheduler Thread handle
 */
ryos_thread_t *sched_thread_handle;

/*
 * @brief System Scheduler message queue
 */
ry_queue_t *ry_ipc_msgQ;


/*
 * @brief Scheduler thread control switch
 */
u32_t sched_running;

/*
 * @brief Control block of system
 */
ry_system_ctx_t ry_system_ctx_v;

extern ryos_thread_t* sensor_alg_thread_handle;
extern ryos_thread_t    *ry_tool_thread_handle;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void ry_system_idle_handler(ry_ipc_msg_t* msg);
void ry_kernel_initialized_handler(ry_ipc_msg_t* msg);
void ry_system_tp_handler(ry_ipc_msg_t* msg);
void ry_system_rtc_handler(ry_ipc_msg_t* msg);
void ry_system_alg_handler(ry_ipc_msg_t* msg);
void ry_system_still_long_handler(ry_ipc_msg_t* msg);
void ry_system_weared_change_handler(ry_ipc_msg_t* msg);
void ry_system_ble_handler(ry_ipc_msg_t* msg);
void ry_system_msg_handler(ry_ipc_msg_t* msg);
void ry_system_nfc_handler(ry_ipc_msg_t* msg);
void ry_system_sys_handler(ry_ipc_msg_t* msg);
void ry_system_card_handler(ry_ipc_msg_t* msg);
void ry_system_surface_handler(ry_ipc_msg_t* msg);
void ry_system_loading_handler(ry_ipc_msg_t* msg);
void ry_system_find_phone_handler(ry_ipc_msg_t* msg);
void ry_system_motar_handle(ry_ipc_msg_t* msg);
void ry_system_sleep_handler(ry_ipc_msg_t* msg);
void ry_system_sport_handler(ry_ipc_msg_t* msg);
void ry_system_alipay_handler(ry_ipc_msg_t* msg);
void ry_system_alarm_ring_handler(ry_ipc_msg_t* msg);


/*
 * @brief System state machine table
 */
static const ry_system_stateMachine_t ry_system_stateMachine[] = 
{
     /*  current state,                       event,                               handler    */
    { RY_SYSTEM_STATE_IDLE,               IPC_MSG_TYPE_BLE_INIT_DONE,          ry_system_idle_handler },
    { RY_SYSTEM_STATE_IDLE,               IPC_MSG_TYPE_TOUCH_INIT_DONE,        ry_system_idle_handler },
    { RY_SYSTEM_STATE_IDLE,               IPC_MSG_TYPE_KERNEL_INIT_DONE,       ry_system_idle_handler },
    { RY_SYSTEM_STATE_IDLE,               IPC_MSG_TYPE_GUI_INIT_DONE,          ry_system_idle_handler },

    { RY_SYSTEM_STATE_KERNEL_INITIALIZED, IPC_MSG_TYPE_APPLICATION_INIT_DONE,  ry_kernel_initialized_handler },

    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_TOUCH_RAW_EVENT,   ry_system_tp_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_RTC_1S_EVENT,      ry_system_rtc_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_RTC_1MIN_EVENT,    ry_system_rtc_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_RTC_1HOUR_EVENT,   ry_system_rtc_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_RTC_1DAY_EVENT,    ry_system_rtc_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALGORITHM_WRIST_FILT,  ry_system_alg_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALGORITHM_STILL_LONG,  ry_system_still_long_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALGORITHM_WEARED_CHANGE,ry_system_weared_change_handler },
    
    //{ RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BLE_START,        ry_system_bt_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BLE_CONNECTED,      ry_system_ble_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BLE_DISCONNECTED,   ry_system_ble_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BLE_APP_KILLED,     ry_system_ble_handler },
    { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_MM_NEW_NOTIFY,      ry_system_msg_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_INIT_DONE,     ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BIND_START,        ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BIND_SUCC,         ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BIND_FAIL,         ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_UPGRADE_START,     ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_UPGRADE_SUCC,      ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_UPGRADE_FAIL,      ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BIND_WAIT_ACK,     ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BIND_ACK_CANCEL,   ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_CHARGING_START,    ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_CHARGING_END,      ry_system_sys_handler },

     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALARM_ON,          ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALERT_LONG_SIT,    ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALERT_BATTERY_LOW, ry_system_sys_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALERT_10000_STEP,  ry_system_sys_handler },

     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_READER_ON_EVT,      ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_READER_OFF_EVT,     ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_READER_DETECT_EVT,  ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_CE_ON_EVT,          ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_CE_OFF_EVT,         ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_NFC_POWER_OFF_EVT,      ry_system_nfc_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_DEV_UNBIND,             ry_device_unbind },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_DEV_MOTAR,              ry_system_motar_handle },

     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_CMS_CARD_CREATE_EVT,    ry_system_card_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_SURFACE_UPDATE,         ry_system_surface_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_SPORTS_READY,           ry_system_sport_handler },

     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALGORITHM_SLEEP,       ry_system_sleep_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALGORITHM_WAKEUP,      ry_system_sleep_handler },
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_BLE_INIT_DONE,      ry_system_ble_handler },

	 { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_FIND_PHONE_START,		ry_system_find_phone_handler },
	 { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_FIND_PHONE_TIMEOUT,	ry_system_find_phone_handler },
	 { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_FINDE_PHONE_CONTINUE,	ry_system_find_phone_handler },
	 { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_FIND_PHONE_STOP,		ry_system_find_phone_handler },
	 { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALARM_RING,	 ry_system_alarm_ring_handler },

	 
     { RY_SYSTEM_STATE_APPLICATION_INITIALIZED, IPC_MSG_TYPE_ALIPAY_EVENT,		    ry_system_alipay_handler },

};


#if 0
static const ry_app_stateMachine_t ry_app_stateMachine[] = 
{
     { DEV_LIFECYCLE_UNBIND,    DEV_SESSION_IDLE,    ry_app_idledler },
};
#endif


void ry_system_factory_schedule(void)
{
    //should not return when the lifecycle is factory, because the life is factory in manufactory flow
    // if (ry_system_ctx_v.lifecycle == DEV_LIFECYCLE_FACTORY) {
    //     return;
	// }
		
    /* Close some periodic task */
    periodic_task_set_enable(periodic_touch_task, 0);
    periodic_task_set_enable(periodic_hr_task, 0);
    periodic_task_set_enable(periodic_battery_task, 0);                    
    
    /* Disable wrist filt */
    set_raise_to_wake_close();
    data_service_set_enable(0);
    alg_sleep_scheduler_set_enable(0);
    alg_long_sit_set_enable(0);
    ry_system_ctx_v.lifecycle = DEV_LIFECYCLE_FACTORY;

//    if (ry_tool_thread_handle == NULL){
//        ry_tool_thread_init();
//    }

    if ((u8_t)dev_mfg_state_get() == 0xFF) {
        /* PATCH for activate 9412 first */
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, (void*)1);
    }
}

void ry_system_unbind_schedule(void)
{
    if (ry_system_ctx_v.lifecycle == DEV_LIFECYCLE_UNBIND) {
        return;
	}
		
    /* Close some periodic task */
    periodic_task_set_enable(periodic_touch_task, 0);
    periodic_task_set_enable(periodic_hr_task, 0);
    periodic_task_set_enable(periodic_battery_task, 1);

    /* Disable wrist filt */
    // set_raise_to_wake_close();
    data_service_set_enable(0);
    alg_sleep_scheduler_set_enable(0);
    alg_long_sit_set_enable(0);
    
    ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
    ry_led_onoff_with_effect(0, 0);    
    ry_hrm_msg_send(HRM_MSG_CMD_MANUAL_SAMPLE_STOP, NULL);
    set_auto_heart_rate_close();

    device_setting_reset_to_factory();

    u8_t para = RY_BLE_ADV_TYPE_SLOW;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);

    /* Suspend sensor task */
    if (ry_system_ctx_v.alg_thread_suspend == 0){    
        ryos_thread_suspend(sensor_alg_thread_handle);
        ry_system_ctx_v.alg_thread_suspend = 1;
    }

    tme_set_interval(5000);

//    if (ry_tool_thread_handle == NULL){
//        ry_tool_thread_init();
//    }

    set_brightness_rate(E_BRIGHTNESS_DEFAULT);
    set_brightness_night(1);
    ry_system_ctx_v.lifecycle = DEV_LIFECYCLE_UNBIND;
}

void ry_system_binded_schedule(void)
{
    if (ry_system_ctx_v.lifecycle == DEV_LIFECYCLE_BIND) {
        return;
	}

    ry_system_ctx_v.lifecycle = DEV_LIFECYCLE_BIND;
		
    /* Open some periodic task */
    periodic_task_set_enable(periodic_touch_task, 1);
    periodic_task_set_enable(periodic_hr_task, 1);
    periodic_task_set_enable(periodic_battery_task, 1);

    /* Enable wrist filt */
    // set_raise_to_wake_open();
    
    // modificated 2020.04.05
    //data_service_set_enable(1);
    data_service_set_enable(get_data_monitor_state());
    
    // modificated 2020.04.05
    //alg_sleep_scheduler_set_enable(1);
    alg_sleep_scheduler_set_enable(get_sleep_monitor_state());
    
    alg_long_sit_set_enable(1);

    tme_set_interval(1000);

    if (ry_system_ctx_v.alg_thread_suspend != 0){
        ryos_thread_resume(sensor_alg_thread_handle);
        ry_system_ctx_v.alg_thread_suspend = 0;
    }

	//if binded delete uart thread
//    if (ry_tool_thread_handle != NULL){
//        if (ryos_thread_get_state(ry_tool_thread_handle) < RY_THREAD_ST_DELETED){    
//            ryos_thread_delete(ry_tool_thread_handle);
//            ry_tool_thread_handle = NULL;
//        }
//    }
	
    qrcode_timer_stop();

    /* Open SE and select default card if we are enter here from unbind state. */
    if (ry_nfc_get_state() == RY_NFC_STATE_IDLE) {
        card_service_sync();
    }

    //bond must refresh screen brightness when screen is on
    if (get_gui_state() != GUI_STATE_OFF) {
        oled_brightness_update_by_time();
        uint8_t brightness = tms_oled_max_brightness_percent();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);
        //LOG_DEBUG("[bind] brightness_val:%d\r\n", brightness);
    }
}

void ry_system_lifecycle_schedule(void)
{
    device_state_restore();

    ry_hal_wdt_reset();
    /* Check current lifecycle */
    dev_lifecycle_t life = get_device_lifecycle();
    if (life == DEV_LIFECYCLE_BIND){
        ry_system_binded_schedule();
    }
    else if (life == DEV_LIFECYCLE_FACTORY){
        ry_system_factory_schedule();            
    }
    else if (life == DEV_LIFECYCLE_UNBIND) {
        ry_system_unbind_schedule();
    } 
    LOG_ERROR("[ry_system_lifecycle_schedule], life: %d, ry_system_ctx_v.lifecycle: %d\r\n", \
                life, ry_system_ctx_v.lifecycle);
}

void ry_system_lifecycle_schedule_periodic_task_init(void)
{
    device_state_restore();
    dev_lifecycle_t life = get_device_lifecycle();
    if (life == DEV_LIFECYCLE_BIND){
        periodic_task_set_enable(periodic_touch_task, 1);
        periodic_task_set_enable(periodic_hr_task, 1);
        periodic_task_set_enable(periodic_battery_task, 1);
    }
    else if (life == DEV_LIFECYCLE_FACTORY){
        periodic_task_set_enable(periodic_touch_task, 0);
        periodic_task_set_enable(periodic_hr_task, 0);      
        periodic_task_set_enable(periodic_battery_task, 0);                    
    }
    else if (life == DEV_LIFECYCLE_UNBIND) {
        periodic_task_set_enable(periodic_touch_task, 0);
        periodic_task_set_enable(periodic_hr_task, 0);
        periodic_task_set_enable(periodic_battery_task, 1);
    }
}

void ry_system_app_start(void)
{
#if 1    
    ry_system_lifecycle_schedule();
#else
    if (get_device_lifecycle() < DEV_LIFECYCLE_BIND) {

        /* Close some periodic task */
        periodic_task_set_enable(periodic_touch_task, 0);
        periodic_task_set_enable(periodic_hr_task, 0);

        /* Disable wrist filt */
        set_raise_to_wake_close();

        if (get_device_lifecycle() == DEV_LIFECYCLE_UNBIND) {
            /* Suspend sensor task */
            ryos_thread_suspend(sensor_alg_thread_handle);
        } else {
            /* Factory side */
        }
    }
#endif

    /* Set touch to BTN only mode */
    touch_enable();
    touch_mode_set(TP_MODE_BTN_ONLY);
}


/**
 * @brief   IPC message handler when system in idle state
 *
 * @param   msg - The IPC message
 *
 * @return  None
 */
void ry_kernel_initialized_handler(ry_ipc_msg_t* msg)
{
    LOG_INFO("[Scheduler] Components Init done.\r\n");

    if (msg->msgType == IPC_MSG_TYPE_APPLICATION_INIT_DONE) {
        ry_system_ctx_v.curState = RY_SYSTEM_STATE_APPLICATION_INITIALIZED;
    }

    /* Start to show app layer UI */
    //app_init();


    /* Disable touch until the first activity starts */
    touch_disable();

    /* Service init */
    surface_service_init();
    
    app_notify_manager_init();
    
    //app_service_init();

    if ((u8_t)dev_mfg_state_get() == 0xFF) {
        SeApi_ForcedFirmwareDownload();
    }

    card_service_init();    

    /* Window service start and show the surface */
    wms_init();

    mibeacon_init();
    mijia_service_init();

    tms_init();

    LOG_DEBUG("[ry_kernel_initialized_handler] Free heap size:%d\r\n", ryos_get_free_heap());

    ry_system_app_start();
    ry_system_ctx_v.sys_initialized = 1;
    
    LOG_INFO("firmware ver: %s", get_device_version());
    
#if RY_BUILD == RY_RELEASE

    if (dev_mfg_state_get() == DEV_MFG_STATE_DONE) {
        ry_hal_wdt_lock_and_start();
    }
#endif

    /* Start ADV  */
    u8_t para = RY_BLE_ADV_TYPE_NORMAL;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
}


uint8_t ry_system_initial_status(void)
{
    return ry_system_ctx_v.sys_initialized;
}

/**
 * @brief   IPC message handler when system in idle state
 *
 * @param   msg - The IPC message
 *
 * @return  None
 */
void ry_system_idle_handler(ry_ipc_msg_t* msg)
{
    ry_sts_t status;
    switch (msg->msgType) {

        case IPC_MSG_TYPE_BLE_INIT_DONE:
            LOG_DEBUG("[Scheduler] BLE Init done.\r\n");

#if (RY_TOUCH_ENABLE == TRUE)
            if (RY_SUCC != (status = touch_init())) {
                while(1);
            }
#endif
            break;

        case IPC_MSG_TYPE_TOUCH_INIT_DONE:
            LOG_DEBUG("[Scheduler] Touch Init done.\r\n");
#if (RY_GSENSOR_ENABLE == TRUE)
            if (RY_SUCC != (status = ry_gsensor_init())) {
                while(1);
            }
#endif

#if (RY_GUI_ENABLE == TRUE)
            if (RY_SUCC != (status = ry_gui_init())) {
                while(1);
            }
#endif
            break;


        case IPC_MSG_TYPE_GUI_INIT_DONE:
#if (STARTUP_LOGO_EN == TRUE)
            gui_display_startup_logo(device_get_powerOn_mode());
#endif
            //LOG_INFO("[Scheduler] GUI Init done.\r\n");
            ry_sched_msg_send(IPC_MSG_TYPE_KERNEL_INIT_DONE, 0, NULL);
            break;
            
        
        case IPC_MSG_TYPE_KERNEL_INIT_DONE:
            LOG_DEBUG("[Scheduler] Kernel Init done.\r\n");
            ry_system_ctx_v.curState = RY_SYSTEM_STATE_KERNEL_INITIALIZED;

            /* Enable all IRQ */
            ry_hal_irq_enable();
            
            /* Start Application init */
#if (RY_HRM_ENABLE == TRUE)
            hrm_init();
#endif


#if (RY_SE_ENABLE == TRUE)
            if (RY_SUCC != (status = ry_se_init(NULL))) {
                while(1);
            }
#endif
			
#if ((RY_HRM_ENABLE == TRUE) || (RY_GSENSOR_ENABLE == TRUE))
            if (RY_SUCC != (status = sensor_alg_init())){
                while(1);
            }
#endif 

#if ((RY_TOOL_ENABLE == TRUE) && (HW_VER_CURRENT == HW_VER_SAKE_03))
			//if (!is_factory_test_finish() ){
//				 if(RY_SUCC != (status = ry_tool_init())){
//				 		 LOG_ERROR("Tool Init Fail. Status: 0x%04x\r\n", status);
//				 		 while(1);
//				 }
			//}
#endif
            app_notify_manager_init();
            data_service_task_init();

            system_app_data_init();
            ry_statistics_init();
            
            ry_sched_msg_send(IPC_MSG_TYPE_APPLICATION_INIT_DONE, 0, NULL);
            break;

        default:
            break;
    }

    LOG_DEBUG("[ry_system_idle_handler] free heap size:%d\r\n", ryos_get_free_heap());
}

/**
 * @brief   TP event message handler
 *
 * @param   msg - The IPC message
 *
 * @return  None
 */
void ry_system_tp_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.touchEvtCb) {
        ry_system_ctx_v.touchEvtCb(msg);
    }
    
}


void ry_system_rtc_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.rtcEvtCb) {
        ry_system_ctx_v.rtcEvtCb(msg);
    }   
}

void ry_system_alg_handler(ry_ipc_msg_t* msg)
{
    uint32_t effective = (get_device_powerstate() != DEV_PWR_STATE_OFF);
    if (effective){
        if (ry_system_ctx_v.algEvtCb) {
            ry_system_ctx_v.algEvtCb(msg);
        }
    }
}

void ry_system_still_long_handler(ry_ipc_msg_t* msg)
{
    uint32_t effective = (get_device_powerstate() != DEV_PWR_STATE_OFF);
    if (effective){
        uint8_t still_long = msg->data[0];
        // LOG_DEBUG("[ry_system_still_long_handler] s_alg.still_time: %d, still_long_status:%d\n", \
                s_alg.still_time, still_long);
        if (still_long){
            surface_onRTC_runtime_enable(0);
        }
        else{
            if (surface_runtime_enable_get() == 0){
                surface_onRTC_runtime_enable(1);
                ry_sched_msg_send(IPC_MSG_TYPE_RTC_1MIN_EVENT, 0, NULL);
            }
        }

        if (ry_system_ctx_v.alg_still_long_EvtCb) {
            ry_system_ctx_v.alg_still_long_EvtCb(msg);
        }
    }
}

void ry_system_weared_change_handler(ry_ipc_msg_t* msg)
{
    uint32_t effective = (get_device_powerstate() != DEV_PWR_STATE_OFF);
    if (effective){
        uint8_t weared_or_unStill = msg->data[0];
        // LOG_DEBUG("[ry_system_weared_change_handler] weared_or_unStill:%d\n", weared_or_unStill);
        if (weared_or_unStill){
            // surface_onRTC_runtime_enable(0);
        }
        else{
            // surface_onRTC_runtime_enable(1);
            // ry_sched_msg_send(IPC_MSG_TYPE_RTC_1MIN_EVENT, 0, NULL);
        }

        if (ry_system_ctx_v.alg_weared_changed_EvtCb) {
            ry_system_ctx_v.alg_weared_changed_EvtCb(msg);
        }
    }
}

void ry_system_ble_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.bleEvtCb) {
        ry_system_ctx_v.bleEvtCb(msg);
    }

    switch(msg->msgType)
    {
        case IPC_MSG_TYPE_BLE_INIT_DONE:
            LOG_DEBUG("[%s] Reset Start First Adv. \r\n", __FUNCTION__);
            if(is_bond()) {
                uint8_t para = RY_BLE_ADV_TYPE_NORMAL;
                ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
            } else {
                uint8_t para = RY_BLE_ADV_TYPE_SLOW;
                ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
            }
            break;
        
        case IPC_MSG_TYPE_BLE_DISCONNECTED:
            alg_data_report_disable();
            update_timeout_handle(NULL);
            r_xfer_BleDisconnectHandler();
        
            DEV_STATISTICS_SET_TIME(bt_disconnect_count, bt_disconnect_time, ryos_rtc_now());
            set_device_connection(DEV_BLE_UNCONNECT);

            surface_on_ble_disconnected();
            set_online_raw_data_onOff(0);

            if (get_device_session() == DEV_SESSION_CARD || get_device_session() == DEV_SESSION_UPLOAD_DATA) {
                set_device_session(DEV_SESSION_IDLE);
            }
            break;

        case  IPC_MSG_TYPE_BLE_APP_KILLED:
            alg_data_report_disable();
            update_timeout_handle(NULL);
            r_xfer_BleDisconnectHandler();

            surface_on_ble_disconnected();
            set_online_raw_data_onOff(0);

            if (get_device_session() == DEV_SESSION_CARD || get_device_session() == DEV_SESSION_UPLOAD_DATA) {
                set_device_session(DEV_SESSION_IDLE);
            }
            break;

        case IPC_MSG_TYPE_BLE_CONNECTED:
            ryos_update_onConnected();
            break;
    }

    ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, 0, NULL);
}

void ry_system_msg_handler(ry_ipc_msg_t* msg)
{
    uint32_t effective = (get_device_powerstate() != DEV_PWR_STATE_OFF);
    if (effective){
        if (ry_system_ctx_v.msgEvtCb) {
            ry_system_ctx_v.msgEvtCb(msg);
        }
    }
}

void ry_system_nfc_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.nfcEvtCb) {
        ry_system_ctx_v.nfcEvtCb(msg);
    }
}

void ry_system_sys_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.sysEvtCb) {
        ry_system_ctx_v.sysEvtCb(msg);
    }
}

void ry_system_alarm_ring_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.alarmRingEvtCb) {
        ry_system_ctx_v.alarmRingEvtCb(msg);
    }
}


void ry_system_card_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.cardEvtCb) {
        ry_system_ctx_v.cardEvtCb(msg);
    }
}

void ry_system_surface_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.surfaceEvtCb) {
        ry_system_ctx_v.surfaceEvtCb(msg);
    }
}

void ry_system_find_phone_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.findPhoneEvtCb) {
        ry_system_ctx_v.findPhoneEvtCb(msg);
    }
}

void ry_system_alipay_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.alipayEvtCb) {
        ry_system_ctx_v.alipayEvtCb(msg);
    }
}

void ry_system_sport_handler(ry_ipc_msg_t* msg)
{
    if (ry_system_ctx_v.sportEvtCb) {
        ry_system_ctx_v.sportEvtCb(msg);
    }
}

void ry_system_monitor_handler(monitor_msg_t* msg)
{
    *(uint32_t *)msg->dataAddr = 0;
    // LOG_DEBUG("[%s]feed watchdog\r\n", __FUNCTION__);

}

void ry_system_sleep_handler(ry_ipc_msg_t* msg)
{    
#if 0 // Disable first    
    if (msg->msgType == IPC_MSG_TYPE_ALGORITHM_SLEEP) {
        if (is_mijia_sleep_scene_exist(MIJIA_SLEEP_TYPE_ENTER_SLEEP)) {
            mibeacon_sleep(MIJIA_SLEEP_TYPE_ENTER_SLEEP);
        }
    } else if (msg->msgType == IPC_MSG_TYPE_ALGORITHM_WAKEUP) {
        if (is_mijia_sleep_scene_exist(MIJIA_SLEEP_TYPE_WAKE_UP)) {
            mibeacon_sleep(MIJIA_SLEEP_TYPE_WAKE_UP);
        }
    }
#endif    
}

void ry_system_motar_handle(ry_ipc_msg_t* msg)
{
    ry_motar_msg_param_t * motar_param = (ry_motar_msg_param_t *)(&msg->data);
    uint32_t effective = (get_device_powerstate() != DEV_PWR_STATE_OFF);
    if (effective){
        if(motar_param->type == MOTAR_TYPE_STRONG_LINEAR){
            motar_strong_linear_working(motar_param->time);
        }
        else if(motar_param->type == MOTAR_TYPE_LIGHT_LINEAR){
            motar_light_linear_working(motar_param->time);
        }
        else if(motar_param->type == MOTAR_TYPE_STRONG_LOOP){
            motar_strong_loop_working(motar_param->time);
        }
        else if(motar_param->type == MOTAR_TYPE_QUICK_LOOP){
            motar_quick_loop_working(motar_param->time);
        }
        else if(motar_param->type == MOTAR_TYPE_STRONG_TWICE){
            motar_strong_linear_twice(motar_param->time);
        }
    }
}


/**
 * @brief   IPC message parser
 *
 * @param   msg - The IPC message
 *
 * @return  None
 */
void ry_sched_msgHandler(ry_ipc_msg_t *msg)
{
     int i, size;

    //LOG_DEBUG("[r_xfer_state_machine] inst_state:%d, evt:0x%04x\r\n", pMsg->instance->state, evt);

    size = sizeof(ry_system_stateMachine)/sizeof(ry_system_stateMachine_t);

    /* Search an appropriate event handler */
    for (i = 0; i < size; i++) {
        
        if (ry_system_stateMachine[i].curState == ry_system_ctx_v.curState &&
            ry_system_stateMachine[i].msgType == msg->msgType) {
            /* call event handler */
            //LOG_DEBUG("[ry_sched_msgHandler] msgType:%d \r\n", msg->msgType);
            ry_system_stateMachine[i].handler(msg);
//            ry_free((u8*)msg);
            return;
         }
    }

    if (msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_SCHEDULE){
        ry_system_monitor_handler((monitor_msg_t*)msg->data);
    } 
    /* Not found, release */
//    ry_free((u8*)msg);
    return;
}


/**
 * @brief   API to subscribe Touch message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t wm_addTouchListener(wm_touchEventHandler_t touchEventCb)
{
    ry_system_ctx_v.touchEvtCb = touchEventCb;

    return RY_SUCC;
}

/**
 * @brief   API to subscribe RTC message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t ry_sched_addRTCEvtListener(system_rtcEventHandler_t rtcEventCb)
{
    ry_system_ctx_v.rtcEvtCb = rtcEventCb;
    return RY_SUCC;
}

/**
 * @brief   API to subscribe Algorithm message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t ry_sched_addAlgEvtListener(system_algEventHandler_t algEventCb)
{
    ry_system_ctx_v.algEvtCb = algEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addBLEEvtListener(system_bleEventHandler_t bleEventCb)
{
    ry_system_ctx_v.bleEvtCb = bleEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addMSGEvtListener(system_msgEventHandler_t msgEventCb)
{
    ry_system_ctx_v.msgEvtCb = msgEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addNfcEvtListener(system_nfcEventHandler_t nfcEventCb)
{
    ry_system_ctx_v.nfcEvtCb = nfcEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addSysEvtListener(system_sysEventHandler_t sysEventCb)
{
    ry_system_ctx_v.sysEvtCb = sysEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addCardEvtListener(system_sysEventHandler_t cardEventCb)
{
    ry_system_ctx_v.cardEvtCb = cardEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addSurfaceEvtListener(system_surfaceEventHandler_t surfaceEventCb)
{
    ry_system_ctx_v.surfaceEvtCb = surfaceEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addSportEvtListener(system_sportEventHandler_t SportEventCb)
{
    ry_system_ctx_v.sportEvtCb = SportEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addFindPhoneEvtListener(system_sportEventHandler_t FindPhoneEventCb)
{
    ry_system_ctx_v.findPhoneEvtCb = FindPhoneEventCb;
    return RY_SUCC;
}

ry_sts_t ry_sched_addAlipayEvtListener(system_sportEventHandler_t alipayEvtCb)
{
    ry_system_ctx_v.alipayEvtCb = alipayEvtCb;
    return RY_SUCC;
}


ry_sts_t ry_sched_addAlarmEvtListener(system_alarmRingEventHandler_t alarmRingEventCb)
{
    ry_system_ctx_v.alarmRingEvtCb = alarmRingEventCb;
    return RY_SUCC;
}


/********************************API*******************************************/
ry_sts_t ble_send_request(CMD cmd, u8_t* data, int len, u8_t security, ry_ble_send_cb_t callback, void* arg)
{
    ry_sts_t status;
    u8_t* newData = NULL;
    
    /* Check if current device status allow to execute the command */

    /* Check if ble is already connected */
    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NOT_CONNECTED);
        callback(status, arg);
        return status;
    }

    if (ry_ble_get_peer_phone_app_lifecycle() == PEER_PHONE_APP_LIFECYCLE_DESTROYED) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_INVALID_STATE);
        callback(status, arg);
        return status;
    }

    /* Check pass */
    if (len && data) {
        newData = (u8_t*)ry_malloc(len);
        ry_memset(newData, 0, len);
        ry_memcpy(newData, data, len);
    }

    
    status = ry_ble_txMsgReq(cmd, RY_BLE_TYPE_REQUEST, 0, newData, len, security, callback, arg);
    if (status != RY_SUCC) {
        ry_free((void*)newData);
    }
    
    return status;
}


ry_sts_t ble_send_response(CMD cmd, u32_t errCode, u8_t* data, int len, u8_t security, ry_ble_send_cb_t callback, void* arg)
{
    ry_sts_t status;
    u8_t* newData = NULL;
    
    /* Check if current device status allow to execute the command */

    /* Check if ble is already connected */
    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
    //if (get_device_connection() != DEV_BLE_LOGIN) {
        LOG_WARN("[ble_send_response] ble_state:%d\r\n", ry_ble_get_state());
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NOT_CONNECTED);
    }

    /* Check pass */
    if (len && data) {
        newData = (u8_t*)ry_malloc(len);
        ry_memset(newData, 0, len);
        ry_memcpy(newData, data, len);
    }

    
    status = ry_ble_txMsgReq(cmd, RY_BLE_TYPE_RESPONSE, errCode, newData, len, security, callback, arg);
    if (status != RY_SUCC) {
        ry_free((void*)newData);
    }
    
    return status;
}



/**
 * @brief   API to send IPC message to scheduler
 *
 * @param   msg - IPC message
 *
 * @return  Status
 */
ry_sts_t ry_sched_msg_send(u32_t msgType, u32_t len, u8_t* data)
{
    ry_sts_t status = RY_SUCC;
    ry_ipc_msg_t *p = (ry_ipc_msg_t*)ry_malloc(sizeof(ry_ipc_msg_t)+len);
    if (NULL == p) {
        LOG_ERROR("[ry_sched_msg_send]: No mem.\n");
        return RY_SET_STS_VAL(RY_CID_SCHED, RY_ERR_NO_MEM);
    }   

    p->msgType = msgType;
    p->len     = len;
    if (data) {
        ry_memcpy(p->data, data, len);
    }

    if (RY_SUCC != (status = ryos_queue_send(ry_ipc_msgQ, &p, 4))) {
        LOG_ERROR("[ry_sched_msg_send]: Add to Queue fail. dropped msg:%d status = 0x%04x\n",
            msgType,
            status);
        ry_free((void*)p);

    }

    
    return status;
}


/**
 * @brief   Scheduler task of system
 *
 * @param   None
 *
 * @return  None
 */
int ry_sched_thread(void* arg)
{
//    (void*)arg;
    ry_ipc_msg_t *msg;
    ry_sts_t status = RY_SUCC;

    LOG_DEBUG("[ry_sched_thread] Enter.\n");

    LOG_DEBUG("Last reset status: 0x%x\r\n", am_hal_reset_status_get());
    am_hal_reset_status_clear();

#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_SCHED);
#endif

    LOG_DEBUG("[ry_sched_thread] free heap size1:%d\r\n", ryos_get_free_heap());
    
    /* Create the IPC message queue */
    status = ryos_queue_create(&ry_ipc_msgQ, IPC_MSG_MAX_NUM, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_sched_thread]: RX Queue Create Error.\r\n");
        while(1);
    }

    /* Start Kernel Init */
    device_info_init();

    LOG_DEBUG("[ry_sched_thread] free heap size2:%d\r\n", ryos_get_free_heap());

    add_reset_count(DEV_RESTART_COUNT);
#if (RY_FS_ENABLE == TRUE)
    if (RY_SUCC != (status = ry_filesystem_init())) {
        LOG_ERROR("Filesystem Init Fail. Status: 0x%04x\r\n", status);
        while(1);
    }
#endif

    storage_management_init();

    create_qrcode_bitmap();

#if (RY_BUILD == RY_RELEASE)
//      font caching disabled, cuz no need external font anymore
//      space will be used for firmware body now
    
//      fontQuickReadInit();    
#endif

    LOG_DEBUG("[ry_sched_thread] free heap size3:%d\r\n", ryos_get_free_heap());

#if (RY_BLE_ENABLE == TRUE) 
    if (RY_SUCC != (status = ry_ble_init(NULL))) {
        LOG_ERROR("BLE Init Fail. Status: 0x%04x\r\n", status);
        while(1);
    }
    //ry_sched_msg_send(IPC_MSG_TYPE_BLE_INIT_DONE, 0, NULL);
#endif

    LOG_DEBUG("[ry_sched_thread] free heap size4:%d\r\n", ryos_get_free_heap());

    uint32_t msg_handle_recived_stamp = 0;
    uint32_t msg_handle_time_ms_used = 0;
    const uint32_t msg_handle_too_long_ms = 100;


    while (sched_running) {
        status = ryos_queue_recv(ry_ipc_msgQ, &msg, RY_OS_WAIT_FOREVER);
        msg_handle_recived_stamp = ry_hal_clock_time();
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_sched_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } 
        else {
            ry_sched_msgHandler(msg);
        }

        msg_handle_time_ms_used = ry_hal_calc_ms(ry_hal_clock_time() - msg_handle_recived_stamp);
        if(msg == NULL) {
//            LOG_ERROR("[ry_sched_thread] time used %d ms msg:NULL cur:%s\n",
//                msg_handle_time_ms_used);
        } else {
            if(msg_handle_time_ms_used > msg_handle_too_long_ms) {
                LOG_INFO("[ry_sched_thread] time used %d ms msg:0x%X\n",
                    msg_handle_time_ms_used,
                    msg->msgType);
            } else {
                LOG_DEBUG("[ry_sched_thread] time used %d ms msg:0x%X\n",
                    msg_handle_time_ms_used,
                    msg->msgType);
            }
        }

        ry_free((u8*)msg);
    }
    return 1;
}

/**
 * @brief   API to init system scheduler
 *
 * @param   None
 *
 * @return  None
 */
void ry_sched_init(void)
{
    ryos_threadPara_t para;
    ry_sts_t status;

    sched_running = 1;

    ry_memset((void*)&ry_system_ctx_v, 0, sizeof(ry_system_ctx_t));

    /* Start Scheduler Thread */
    strcpy((char*)para.name, "ry_sched_thread");
    para.stackDepth = SCHEDULER_THREAD_STACK_SIZE;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = SCHEDULER_THREAD_PRIORITY;
    status = ryos_thread_create(&sched_thread_handle, ry_sched_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_sched_init]: Thread Init Fail.\n");
        while(1);
    }
}



