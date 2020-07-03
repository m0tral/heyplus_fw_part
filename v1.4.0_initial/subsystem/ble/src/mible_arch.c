#include <stdio.h>
//#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "ry_mcu_config.h"
#include "ry_hal_flash.h"


/* Ble related */

#include "ry_type.h"
#include "mible_arch.h"
#include "mible.h"
#include "ry_utils.h"

#include "ryos.h"
#include "ryos_timer.h"

#include "wsf_types.h"
#include "wsf_msg.h"
#include "wsf_trace.h"

/* Ambiq BLE related */
#include "ble_task.h"
#include "app_api.h"
#include "att_api.h"
#include "svc_mi.h"


static u8 mible_publicAddr[6];
static u8 mible_peerAddr[6];
static u32 timer_interval;
static arch_timer_cb_t mible_timer_cb;
static void* mible_arg;

extern uint8_t g_BLEMacAddress[];

taskFunc_t mible_taskFunc;
void* mible_taskArg;

ry_timer_t*  mible_arch_timer_id;

//extern cy_stc_ble_conn_handle_t connectionHandle;



#if (BSP_MCU_TYPE == BSP_APOLLO2)
#include "device_management_service.h"
#define MIBLE_FLASH_ADDR_RYEEX        (0x000FC838) //(DEVICE_SETTING_ADDR + sizeof(ry_device_setting_t))//(0x000FE000)//(0x00009F80)
#define MIBLE_FLASH_ADDR              (MIBLE_FLASH_ADDR_RYEEX + MIBLE_PSM_LEN)

//#define MIBLE_FLASH_ADDR_RYEEX        (DEVICE_SETTING_ADDR + sizeof(ry_device_setting_t))//(0x000FE000)//(0x00009F80)
//#define MIBLE_FLASH_ADDR              (MIBLE_FLASH_ADDR_RYEEX + MIBLE_PSM_LEN)
#elif (BSP_MCU_TYPE == BSP_CYPRESS_PSOC6)
#define MIBLE_FLASH_ADDR        0x100F0000
#endif




static bool arch_timeout_cb(void* usrdata);
static void arch_task_cb(void* usrdata);


void copy_str_to_addr(u8 *addr, const char *str)
{
    int i;
    unsigned int value;
    int using_long_format = 0;
    int using_hex_sign = 0;

    if (str[2] == ':' || str[2] == '-') {
        using_long_format = 1;
    }

    if (str[1] == 'x') {
        using_hex_sign = 2;
    }

    for (i = 0; i < 6; i++) {
        sscanf(str + using_hex_sign + i * (2 + using_long_format), "%02x", &value);
        addr[5 - i] = (uint8_t) value;
    }
}


// BLE Related Function

void arch_ble_getDeviceWifiMac(u8* addr)
{
}

void arch_ble_setPublicAddr(u8* addr)
{
    arch_memcpy(mible_publicAddr, addr, 6);
}

void arch_ble_getOwnPublicAddr(u8* addr)
{  
    *addr++ = g_BLEMacAddress[0];
    *addr++ = g_BLEMacAddress[1];
    *addr++ = g_BLEMacAddress[2];
    *addr++ = g_BLEMacAddress[3];
    *addr++ = g_BLEMacAddress[4];
    *addr++ = g_BLEMacAddress[5];
}


// Flash Related Function.
u8 arch_flash_read(u8* buf, int len, u8_t type)
{
    if (type == MIBLE_TYPE_MIJIA) {
        ry_hal_flash_read(MIBLE_FLASH_ADDR, buf, len);
    } else if (type == MIBLE_TYPE_RYEEX){
        ry_hal_flash_read(MIBLE_FLASH_ADDR_RYEEX, buf, len);
    }
    return 0; // succ
}

u8 arch_flash_write(u8* buf, int len, u8_t type)
{

    if (type == MIBLE_TYPE_MIJIA) {
        ry_hal_flash_write(MIBLE_FLASH_ADDR, buf, len);
    } else if (type == MIBLE_TYPE_RYEEX){
        ry_hal_flash_write(MIBLE_FLASH_ADDR_RYEEX, buf, len);
    }
    
    return 0; // succ
}

// Timer related Function.
bool arch_timeout_cb(void* usrdata)
{
    return true;
}

void arch_timer_set(arch_timer_id_t* p_timerId, u32 interval, u8 once, arch_timer_cb_t cbFunc, void* arg) // interval in ms
{
#if 0
    ry_timer_parm param;
    param.timeout_CB = cbFunc;
    param.flag = 0;
    param.data = NULL;
    param.tick = 1;
    param.name = "mible_timer";

    LOG_DEBUG("[arch_timer_set] \r\n");

    if ((*p_timerId) == 0) {
        (*p_timerId) = ry_timer_create(&param);
    }
    ry_timer_start(*p_timerId, interval, cbFunc, NULL);
#endif
}

void arch_timer_clear(arch_timer_id_t timerId)
{
    ry_timer_stop(timerId);
}

void arch_timer_restart(arch_timer_id_t timerId)
{
    
}

wsfHandlerId_t mible_handler_id;

void mible_arch_handler(wsfEventMask_t event, wsfMsgHdr_t *pMsg)
{
    LOG_DEBUG("[mible_arch_handler] \r\n");
    mible_taskFunc(mible_taskArg);
}

void mible_arch_init(wsfHandlerId_t handlerId)
{
    mible_handler_id = handlerId;
}



void arch_task_cb(void* usrdata)
{
    LOG_DEBUG("[arch_task_cb] \r\n");
    //ry_timer_stop(mible_arch_timer_id);
    mible_taskFunc(mible_taskArg);
    //return false;
}


u32_t arch_task_timer_id;

void arch_task_post(taskFunc_t taskFunc, void* arg)
{
    ry_timer_parm param;
    param.timeout_CB = arch_task_cb;
    param.flag = 0;
    param.data = NULL;
    param.tick = 1;
    param.name = "arch_task";

    mible_taskFunc = taskFunc;
    mible_taskArg = arg;

    LOG_DEBUG("[arch_task_post] \r\n");

#if 0
    if (arch_task_timer_id == 0) {
        arch_task_timer_id = ry_timer_create(&param);
    }
    ry_timer_start(arch_task_timer_id, 10, arch_task_cb, NULL);
#endif

    WsfSetEvent(mible_handler_id, 0x01);

}

u32 arch_random(void)
{
    return (u32)rand();
}


//--------------------------------------------------------------------------

void arch_ble_disconnect(u16 connHandle)
{
    AppConnClose(connHandle);
}




// Optional, scan, advertising
void arch_ble_scan(u8 fEnable)
{
    
}


void arch_ble_adv(u8 fEnable, u8 type, u8 intvl, u8* adv, u8 al, u8* sr, u8 sl)
{
    
}

void arch_ble_sendNotify(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data)
{
#if (RY_BLE_ENABLE == TRUE)

    LOG_DEBUG("[arch_ble_sendNotify] uuid:0x%04x, len:%d \r\n", charUUID, len);

    if (charUUID == MISERVICE_CHAR_TOKEN_UUID) {
        ry_ble_sendNotify(connHandle, charUUID, len, data);
    } else {
        LOG_WARN("[arch_ble_sendNotify] invalid UUID. 0x%04x\r\n", charUUID);
    }
#endif
}


void arch_ble_setCharVal(uint16_t charUUID, uint8_t len, uint8_t* data)
{
    LOG_DEBUG("[arch_ble_setCharVal] uuid:0x%04x, len:%d \r\n", charUUID, len);

    if (charUUID == MISERVICE_CHAR_SN_UUID) {
        SvcMisSetSn(data, len);
    } else if (charUUID == MISERVICE_CHAR_BEACONKEY_UUID) {
        
    } else if (charUUID == MISERVICE_CHAR_VER_UUID) {
        SvcMisSetVer(data, len);
    }

}




//void arch_ble_onConnected(gattc_user_connect_struct * conn, uint16_t connected, ble_address_t *bd_addr) 
//{
//}

void arch_ble_onConnected(u16 connHandle, u8* peerAddr, u8 peerType, u8 role)
{
    arch_memcpy(mible_peerAddr, peerAddr, 6);
    //app_mible_onConnected();
    mible_onConnect(connHandle, peerAddr, peerType, role);
}

void arch_ble_onDisconnected(u16 connHandle, u8 role)
{
    //app_mible_onDisconnected();
    mible_onDisconnect(connHandle);
}





