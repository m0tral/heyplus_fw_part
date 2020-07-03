/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_SCHEDULER_H__
#define __RY_SCHEDULER_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "motar.h"
#include "rbp.pb.h"
#include "ry_ble.h"


/*
 * CONSTANTS
 *******************************************************************************
 */

#define KERNEL_MSG_MASK                (1<<32)
#define APPLICATION_MSG_MASK           (1<<31)

/**
 * @brief Definitaion for maximum message subscribe number.
 */
#define MAX_IPC_MSG_SUB_NUM            5

/**
 * @brief Definitaion for IPC_MSG type.
 */
#define IPC_MSG_TYPE_SYSTEM_START           0x00001000  
#define IPC_MSG_TYPE_KERNEL_INIT_DONE       IPC_MSG_TYPE_SYSTEM_START
#define IPC_MSG_TYPE_APPLICATION_INIT_DONE  0x00001001
#define IPC_MSG_TYPE_BIND_START             0x00001002
#define IPC_MSG_TYPE_BIND_SUCC              0x00001003
#define IPC_MSG_TYPE_BIND_FAIL              0x00001004
#define IPC_MSG_TYPE_UPGRADE_START          0x00001005
#define IPC_MSG_TYPE_UPGRADE_SUCC           0x00001006
#define IPC_MSG_TYPE_UPGRADE_FAIL           0x00001007
#define IPC_MSG_TYPE_UNBIND_START           0x00001008
#define IPC_MSG_TYPE_UNBIND_SUCC            0x00001009
#define IPC_MSG_TYPE_UNBIND_FAIL            0x0000100A
#define IPC_MSG_TYPE_BIND_WAIT_ACK          0x0000100B
#define IPC_MSG_TYPE_BIND_ACK_CANCEL        0x0000100C
#define IPC_MSG_TYPE_CHARGING_START         0x0000100D
#define IPC_MSG_TYPE_CHARGING_END           0x0000100E
#define IPC_MSG_TYPE_ALARM_ON               0x0000100F
#define IPC_MSG_TYPE_ALERT_BATTERY_LOW      0x00001010
#define IPC_MSG_TYPE_ALERT_LONG_SIT         0x00001011
#define IPC_MSG_TYPE_DEV_UNBIND             0x00001012
#define IPC_MSG_TYPE_DEV_MOTAR              0x00001013
#define IPC_MSG_TYPE_ALERT_10000_STEP       0x00001014


#define IPC_MSG_TYPE_SYSTEM_MONITOR         0x00001F00
#define IPC_MSG_TYPE_SYSTEM_MONITOR_SCHEDULE (IPC_MSG_TYPE_SYSTEM_MONITOR + 0)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_ALG     (IPC_MSG_TYPE_SYSTEM_MONITOR + 1)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_WMS     (IPC_MSG_TYPE_SYSTEM_MONITOR + 2)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_GUI     (IPC_MSG_TYPE_SYSTEM_MONITOR + 3)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_GUI     (IPC_MSG_TYPE_SYSTEM_MONITOR + 3)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_HRM     (IPC_MSG_TYPE_SYSTEM_MONITOR + 4)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_RX     (IPC_MSG_TYPE_SYSTEM_MONITOR + 5)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_BLE_TX     (IPC_MSG_TYPE_SYSTEM_MONITOR + 6)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_NFC     (IPC_MSG_TYPE_SYSTEM_MONITOR + 8)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_CMS     (IPC_MSG_TYPE_SYSTEM_MONITOR + 9)
#define IPC_MSG_TYPE_SYSTEM_MONITOR_R_XFER  (IPC_MSG_TYPE_SYSTEM_MONITOR + 10)


#define IPC_MSG_TYPE_SYSTEM_END             0x00001FFF

#define IPC_MSG_TYPE_BLE_START              0x00002000
#define IPC_MSG_TYPE_BLE_INIT_DONE          IPC_MSG_TYPE_BLE_START
#define IPC_MSG_TYPE_BLE_CONNECTED          0x00002001
#define IPC_MSG_TYPE_BLE_DISCONNECTED       0x00002002
#define IPC_MSG_TYPE_BLE_APP_KILLED         0x00002003
#define IPC_MSG_TYPE_BLE_END                0x00002FFF

#define IPC_MSG_TYPE_RBP_START              0x00003000    
#define IPC_MSG_TYPE_RBP_END                0x00003FFF

#define IPC_MSG_TYPE_NFC_START              0x00004000 
#define IPC_MSG_TYPE_NFC_INIT_DONE          IPC_MSG_TYPE_NFC_START
#define IPC_MSG_TYPE_NFC_READER_ON_EVT      0x00004001
#define IPC_MSG_TYPE_NFC_READER_OFF_EVT     0x00004002
#define IPC_MSG_TYPE_NFC_READER_DETECT_EVT  0x00004003
#define IPC_MSG_TYPE_NFC_CE_ON_EVT          0x00004004
#define IPC_MSG_TYPE_NFC_CE_OFF_EVT         0x00004005
#define IPC_MSG_TYPE_NFC_SE_OPEN_CHANNEL    0x00004006
#define IPC_MSG_TYPE_NFC_SE_CLOSE_CHANNEL   0x00004007
#define IPC_MSG_TYPE_NFC_POWER_OFF_EVT      0x00004008
#define IPC_MSG_TYPE_NFC_LOW_POWER_EVT      0x00004009
#define IPC_MSG_TYPE_NFC_END                0x00004FFF

#define IPC_MSG_TYPE_TOUCH_START            0x00005000  
#define IPC_MSG_TYPE_TOUCH_INIT_DONE        IPC_MSG_TYPE_TOUCH_START
#define IPC_MSG_TYPE_TOUCH_RAW_EVENT        0x00005001
#define IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT  0x00005002
#define IPC_MSG_TYPE_TOUCH_PREDICT_EVENT    0x00005003
#define IPC_MSG_TYPE_TOUCH_END              0x00005FFF

#define IPC_MSG_TYPE_ALGORITHM_START        0x00006000    
#define IPC_MSG_TYPE_ALGORITHM_WRIST_FILT   0x00006000    
#define IPC_MSG_TYPE_ALGORITHM_DATA_SYNC_WAKEUP     0x00006001    
#define IPC_MSG_TYPE_ALGORITHM_DATA_CHECK_WAKEUP    0x00006002
#define IPC_MSG_TYPE_ALGORITHM_SLEEP        0x00006003 
#define IPC_MSG_TYPE_ALGORITHM_WAKEUP       0x00006004 
#define IPC_MSG_TYPE_ALGORITHM_HRM_CHECK_WAKEUP    0x00006005
#define IPC_MSG_TYPE_ALGORITHM_STILL_LONG    0x00006006
#define IPC_MSG_TYPE_ALGORITHM_WEARED_CHANGE 0x00006007
#define IPC_MSG_TYPE_ALGORITHM_END          0x00006FFF

#define IPC_MSG_TYPE_GUI_START              0x00007000  
#define IPC_MSG_TYPE_GUI_SCREEN_ON          IPC_MSG_TYPE_GUI_START
#define IPC_MSG_TYPE_GUI_SCREEN_OFF         0x00007001
#define IPC_MSG_TYPE_GUI_SET_COLOR          0x00007002
#define IPC_MSG_TYPE_GUI_SET_PIC            0x00007003
#define IPC_MSG_TYPE_GUI_INVALIDATE         0x00007004
#define IPC_MSG_TYPE_GUI_INIT_DONE          0x00007005
#define IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR  0x00007006
#define IPC_MSG_TYPE_GUI_SET_BRIGHTNESS     0x00007007
#define IPC_MSG_TYPE_GUI_END                0x00007FFF


#define IPC_MSG_TYPE_APP_UI_START           0x00008000 
#define IPC_MSG_TYPE_APP_UI_PREPARE_DEFAULT_UI IPC_MSG_TYPE_APP_UI_START
#define IPC_MSG_TYPE_APP_UI_NEXT_PAGE       0x00008001
#define IPC_MSG_TYPE_APP_UI_REFRESH_TIME    0x00008002
#define IPC_MSG_TYPE_APP_UI_END             0x00008FFF

#define IPC_MSG_TYPE_RTC_START              0x00009000
#define IPC_MSG_TYPE_RTC_1S_EVENT           IPC_MSG_TYPE_RTC_START
#define IPC_MSG_TYPE_RTC_1MIN_EVENT         0x00009001
#define IPC_MSG_TYPE_RTC_1HOUR_EVENT        0x00009002
#define IPC_MSG_TYPE_RTC_1DAY_EVENT         0x00009003
#define IPC_MSG_TYPE_RTC_END                0x00009FFF

#define IPC_MSG_TYPE_MM_START               0x0000A000
#define IPC_MSG_TYPE_MM_NEW_NOTIFY          IPC_MSG_TYPE_MM_START
#define IPC_MSG_TYPE_MM_END                 0x00009FFF

#define IPC_MSG_TYPE_WMS_START              0x0000B000
#define IPC_MSG_TYPE_WMS_SCHEDULE           IPC_MSG_TYPE_WMS_START
#define IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE (IPC_MSG_TYPE_WMS_SCHEDULE + 1)

#define IPC_MSG_TYPE_WMS_END                0x0000BFFF

#define IPC_MSG_TYPE_ACTIVITY_HRM_START     0x0000C000
#define IPC_MSG_TYPE_ACTIVITY_HRM_REFRESH   IPC_MSG_TYPE_ACTIVITY_HRM_START
#define IPC_MSG_TYPE_ACTIVITY_HRM_END       0x0000CFFF


#define IPC_MSG_TYPE_CMS_START              0x0000D000
#define IPC_MSG_TYPE_CMS_CARD_CREATE_EVT    IPC_MSG_TYPE_CMS_START
#define IPC_MSG_TYPE_CMS_END                0x0000DFFF


#define IPC_MSG_TYPE_SURFACE_START          0x0000E000
#define IPC_MSG_TYPE_SURFACE_UPDATE         IPC_MSG_TYPE_SURFACE_START
#define IPC_MSG_TYPE_SURFACE_UPDATE_STEP    (IPC_MSG_TYPE_SURFACE_START + 1)
#define IPC_MSG_TYPE_SURFACE_END            0x0000EFFF

#define IPC_MSG_TYPE_SPORTS_START          0x0000F000
#define IPC_MSG_TYPE_SPORTS_READY          IPC_MSG_TYPE_SPORTS_START
#define IPC_MSG_TYPE_SPORTS_END            0x0000FFFF

#define IPC_MSG_TYPE_LOADING_RANGE          0x00010000
#define IPC_MSG_TYPE_LOADING_START          (IPC_MSG_TYPE_LOADING_RANGE + 0)
#define IPC_MSG_TYPE_LOADING_CONTINUE       (IPC_MSG_TYPE_LOADING_RANGE + 1)
#define IPC_MSG_TYPE_LOADING_TIMER          (IPC_MSG_TYPE_LOADING_RANGE + 2)
#define IPC_MSG_TYPE_LOADING_STOP           (IPC_MSG_TYPE_LOADING_RANGE + 3)
#define IPC_MSG_TYPE_LOADING_END            0x00010FFF

#define IPC_MSG_TYPE_FIND_PHONE_RANGE       0x00020000
#define IPC_MSG_TYPE_FIND_PHONE_START       (IPC_MSG_TYPE_FIND_PHONE_RANGE + 0)
#define IPC_MSG_TYPE_FIND_PHONE_TIMEOUT     (IPC_MSG_TYPE_FIND_PHONE_RANGE + 1)
#define IPC_MSG_TYPE_FINDE_PHONE_CONTINUE   (IPC_MSG_TYPE_FIND_PHONE_RANGE + 2)
#define IPC_MSG_TYPE_FIND_PHONE_STOP        (IPC_MSG_TYPE_FIND_PHONE_RANGE + 3)
#define IPC_MSG_TYPE_FIND_PHONE_END         0x00020FFF


#define IPC_MSG_TYPE_ALIPAY_START           0x00021000
#define IPC_MSG_TYPE_ALIPAY_EVENT           (IPC_MSG_TYPE_ALIPAY_START + 0)
#define IPC_MSG_TYPE_ALIPAY_END             0x00021FFF

#define IPC_MSG_TYPE_ALARM_RING             0x00022000


/*
 * TYPES
 *******************************************************************************
 */


/**
 * @brief Definitaion for system state.
 */
typedef enum {
    RY_SYSTEM_STATE_IDLE,
    RY_SYSTEM_STATE_KERNEL_INITIALIZED,
    RY_SYSTEM_STATE_APPLICATION_INITIALIZED,
    RY_SYSTEM_STATE_APPLICATION_RUNNING,
} ry_system_state_t;


typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t  data[1];
} ry_ipc_msg_t;


/**
 * @brief Definitaion for scheduler message callback
 */
typedef void (*ry_sched_msg_cb_t)(void* usrdata);


/**
 * @brief Definitaion for touch event handler, which should be defined in WM. TODO.
 */
typedef int (* system_ipcEventHandler_t)(ry_ipc_msg_t* msg);
typedef int ( *wm_touchEventHandler_t )(ry_ipc_msg_t* msg); 
typedef int (* system_rtcEventHandler_t)(ry_ipc_msg_t* msg); 
typedef int (* system_algEventHandler_t)(ry_ipc_msg_t* msg); 
typedef int (* system_bleEventHandler_t)(ry_ipc_msg_t* msg); 
typedef int (* system_msgEventHandler_t)(ry_ipc_msg_t* msg); 
typedef int (* system_nfcEventHandler_t)(ry_ipc_msg_t* msg); 
typedef int (* system_sysEventHandler_t)(ry_ipc_msg_t* msg); 
typedef int (* system_cardEventHandler_t)(ry_ipc_msg_t* msg);
typedef int (* system_surfaceEventHandler_t)(ry_ipc_msg_t* msg);
typedef int (* system_sportEventHandler_t)(ry_ipc_msg_t* msg);
typedef int (* system_findPhoneEventHandler_t)(ry_ipc_msg_t* msg);
typedef int (* system_alipayEventHandler_t)(ry_ipc_msg_t* msg);
typedef int (* system_alarmRingEventHandler_t)(ry_ipc_msg_t* msg);





/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init scheduler module
 *
 * @param   None
 *
 * @return  None
 */
void ry_sched_init(void);

/**
 * @brief   API to send IPC message to scheduler
 *
 * @param   msg - IPC message
 *
 * @return  Status
 */
ry_sts_t ry_sched_msg_send(u32_t msgType, u32_t len, u8_t* data);

/**
 * @brief   API to subscribe IPC message.
 *
 * @param   msgType - The specified IPC message to subscribe
 * @param   cb      - The related callback function to handle message
 *
 * @return  Status of subscribe
 */
ry_sts_t ry_sched_msg_sub(u32_t msgType, ry_sched_msg_cb_t cb);




/**
 * @brief   API to subscribe Touch message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t wm_addTouchListener(wm_touchEventHandler_t touchEventCb);


/**
 * @brief   API to subscribe BLE message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t ry_sched_addBLEEvtListener(system_bleEventHandler_t bleEventCb);

/**
 * @brief   API to subscribe RTC message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t ry_sched_addRTCEvtListener(system_rtcEventHandler_t rtcEventCb);


/**
 * @brief   API to subscribe Algorithm message. TODO: Should be moved to WM module.
 *
 * @param   touchEventCb - Touch event handler.
 *
 * @return  Status of subscribe
 */
ry_sts_t ry_sched_addAlgEvtListener(system_algEventHandler_t algEventCb);

ry_sts_t ry_sched_addMSGEvtListener(system_msgEventHandler_t msgEventCb);
ry_sts_t ry_sched_addNfcEvtListener(system_nfcEventHandler_t nfcEventCb);
ry_sts_t ry_sched_addSysEvtListener(system_sysEventHandler_t sysEventCb);
ry_sts_t ry_sched_addCardEvtListener(system_sysEventHandler_t cardEventCb);
ry_sts_t ry_sched_addSurfaceEvtListener(system_surfaceEventHandler_t surfaceEventCb);
ry_sts_t ry_sched_addSportEvtListener(system_sportEventHandler_t SportEventCb);
ry_sts_t ry_sched_addAlarmEvtListener(system_alarmRingEventHandler_t AlarmEventCb);
ry_sts_t ry_sched_addFindPhoneEvtListener(system_sportEventHandler_t LoadingEventCb);
ry_sts_t ry_sched_addAlipayEvtListener(system_sportEventHandler_t alipayEvtCb);


uint8_t ry_system_initial_status(void);

ry_sts_t ble_send_request(CMD cmd, u8_t* data, int len, u8_t security, ry_ble_send_cb_t callback, void* arg);
ry_sts_t ble_send_response(CMD cmd, u32_t errCode, u8_t* data, int len, u8_t security, ry_ble_send_cb_t callback, void* arg);

void ry_system_lifecycle_schedule(void);

void ry_system_lifecycle_schedule_periodic_task_init(void);

#endif  /* __RY_SCHEDULER_H__ */


