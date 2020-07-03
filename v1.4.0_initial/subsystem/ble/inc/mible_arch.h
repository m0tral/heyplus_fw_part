
#ifndef __MIBLE_ARCH_H__
#define __MIBLE_ARCH_H__

/*
 * INCLUDES
 ****************************************************************************************
 */

#include "ry_type.h"
#include "ryos.h"

#include "wsf_types.h"
#include "wsf_msg.h"



#define MAX_CONNECTED_PERIPHERAL_DEVS           1
#define MAX_CONNECTED_CENTRAL_DEVS              1

typedef void * TimerHandle_t;


#define MIBLE_ADV_TYPE_UNDIRECT_CONN            
#define MIBLE_ADV_TYPE_DIRECT_CONN_HIGH         
#define MIBLE_ADV_TYPE_UNDIRECT_SCANNABLE        
#define MIBLE_ADV_TYPE_UNDIRECT_NON_CONN         
#define MIBLE_ADV_TYPE_DIRECT_CONN_LOW           

#define STATE_OK                                0
#define STATE_ERROR                             1

#define OS_WAIT_FOREVER (0xFFFFFFFF)
#define OS_NEVER_TIMEOUT (0xFFFFFFFF)
#define OS_NO_WAIT (0)



/*
 * TYPES
 ****************************************************************************************
 */

// Timer Related type definition

typedef void (*arch_timer_cb_t)(void* arg);
typedef int arch_timer_id_t;


// Task related type definition
typedef void (*taskFunc_t)(void* arg);


// Mutex related type definition
//typedef pthread_mutex_t os_mutex_vt;

/*
 * APIs
 ****************************************************************************************
 */


// BLE Related Function                    
//#define  arch_ble_sendNotify(connHandle, charUUID, len, data)  ryeex_ble_send_notify(connHandle, charUUID, len, data)
//#define  arch_ble_setCharVal(charUUID, len, data)              ryeex_ble_set_char_value(charUUID, len, data)
#define  arch_ble_writeDescr(connHandle, charUUID, enable)     //mi_service_write_descr(connHandle, MISERVICE_UUID, charUUID, enable)
#define  arch_ble_writeValue(connHandle, charUUID, len, data)  //mi_service_write_value(connHandle, MISERVICE_UUID, charUUID, data, len)
#define  arch_ble_getDeviceInfo(addr, productID, token)        //miservice_getDeviceInfo(addr, productID, token)
#define  arch_ble_setDeviceInfo(addr, productID, token)        //miservice_setDeviceInfo(addr, productID, token)


// Address related Function
void arch_ble_getOwnPublicAddr(u8* addr);
void arch_ble_getDeviceWifiMac(u8* addr);
void arch_ble_setPublicAddr(u8* addr);



// Buffer/Memory Related Function

#define arch_buf_alloc    ry_malloc
#define arch_buf_free     ry_free
#define arch_memcpy       ry_memcpy
#define arch_memset       ry_memset
#define arch_memcmp       ry_memcmp

// Flash Related Function.
u8 arch_flash_read(u8* buf, int len, u8_t type); // address is specified by the platform.
u8 arch_flash_write(u8* buf, int len, u8_t type);

// Timer related Function.
void arch_timer_set(arch_timer_id_t* p_timerId, u32 interval, u8 once, arch_timer_cb_t cbFunc, void* arg); // interval in ms
void arch_timer_clear(arch_timer_id_t timerId);
void arch_timer_restart(arch_timer_id_t timerId);

// Task related Function
void arch_task_post(taskFunc_t taskFunc, void* arg);

// Random data generated API
u32  arch_random(void);


// BLE APIs
void arch_ble_disconnect(u16 connHandle);


// Option, scan, advertising
void arch_ble_scan(u8 fEnable);
void arch_ble_adv(u8 fEnable, u8 type, u8 intvl, u8* adv, u8 al, u8* sr, u8 sl);
void arch_ble_miservice_init(void);
//void arch_ble_onScanCb(void *reg_cntx, bt_gap_le_advertising_report_ind_t *param);
void arch_ble_onConnected(u16 connHandle, u8* peerAddr, u8 peerType, u8 role);
void arch_ble_onDisconnected(u16 connHandle, u8 role);

void mible_arch_init(wsfHandlerId_t handlerId);
void mible_arch_handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg);

void dip_flash_write(u32_t addr, u32_t len, u8_t* data);

void arch_ble_setCharVal(uint16_t charUUID, uint8_t len, uint8_t* data);
void arch_ble_sendNotify(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data);

#endif

