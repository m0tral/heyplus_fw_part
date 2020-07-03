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


/* bsp specified */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_bsp_gpio.h"
#include "am_util.h"
#include "am_hal_interrupt.h"
#include "am_hal_reset.h"
#include "am_hal_stimer.h"

#include "mibeacon.h"
#include "ryos.h"
#include "ry_ble.h"
#include "ryos_timer.h"

#include "ble_task.h"
#include "app_api.h"
#include "fit_api.h"
#include "ry_hal_flash.h"
#include "ry_hal_mcu.h"

void RyeexUnconnectableAdvStart(uint8_t mode);
void RyeexAdvPayloadUpdate(void);


/*********************************************************************
 * CONSTANTS
 */ 

#define MIBEACON_VER          0x2000    // version number = 2

/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
#define MIBEACON_FRAME_COUNTER_MAGIC_ADDR   0xFBF80
#define MIBEACON_FRAME_COUNTER_MAGIC_NUMBER 0x20180914
// #define MIBEACON_FRAME_COUNTER_SAVE_ADDR    (MIBEACON_FRAME_COUNTER_MAGIC_ADDR+4)

static uint32_t mibeacon_frame_counter = 200;
extern u8_t g_BLEMacAddress[];
int mibeacon_timer = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void RyeexAdvMibeaconSend(uint8_t *data, uint8_t len, int interval, int duration)
{

  AppAdvSetData(APP_ADV_DATA_DISCOVERABLE, len, (uint8_t *) data);
  AppAdvSetData(APP_SCAN_DATA_DISCOVERABLE, len, (uint8_t *) data);

  /* set advertising and scan response data for connectable mode */
  AppAdvSetData(APP_ADV_DATA_CONNECTABLE, 0, NULL);
  AppAdvSetData(APP_SCAN_DATA_CONNECTABLE, 0, NULL);

  /* start advertising; automatically set connectable/discoverable mode and bondable mode */
  //AppAdvStart(APP_MODE_AUTO_INIT);
  RyeexUnconnectableAdvStart(APP_MODE_AUTO_INIT);
}


static void mibeacon_timeout_handler(void* arg)
{
    AppAdvStop();
    wakeup_ble_thread();
    ryos_delay_ms(10);
    if(ry_ble_get_state() != RY_BLE_STATE_CONNECTED)
    {
        uint8_t para = RY_BLE_ADV_TYPE_NORMAL;
        wakeup_ble_thread();    //don't know why but help to stop adv
        ryos_delay_ms(10);
        ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
    }
    else
    {
        LOG_DEBUG("[mibeacon] wait finished adv should already stopped --> update normal\n");
        RyeexAdvPayloadUpdate();
    }
    wakeup_ble_thread();
}

static bool mibeacon_NeedStopFirst(void)
{
    if(ry_timer_isRunning(mibeacon_timer))
    {
        return true;
    }

    if(RY_BLE_STATE_CONNECTED != ry_ble_get_state())
    {
        return true;
    }
    return false;
}

typedef struct
{
    uint32_t magic_number;
    uint32_t frame_counter;
}mibeacon_frame_counter_info_save_t;

static void mibeacon_frame_counter_save_to_flash(mibeacon_frame_counter_info_save_t* p_info)
{
    ry_hal_flash_write(MIBEACON_FRAME_COUNTER_MAGIC_ADDR, (void*)p_info, sizeof(mibeacon_frame_counter_info_save_t));
}

static void mibeacon_frame_counter_info_init(void)
{
    mibeacon_frame_counter_info_save_t saved = *(mibeacon_frame_counter_info_save_t*)MIBEACON_FRAME_COUNTER_MAGIC_ADDR;
    if(saved.magic_number != MIBEACON_FRAME_COUNTER_MAGIC_NUMBER)
    {
        saved.magic_number = MIBEACON_FRAME_COUNTER_MAGIC_NUMBER;
        saved.frame_counter = 200;
        mibeacon_frame_counter_save_to_flash(&saved);
    }
    mibeacon_frame_counter = saved.frame_counter + 20;
}

static void mibeacon_frame_counter_info_save(void)
{
    mibeacon_frame_counter_info_save_t saved = *(mibeacon_frame_counter_info_save_t*)MIBEACON_FRAME_COUNTER_MAGIC_ADDR;
    if((saved.magic_number != MIBEACON_FRAME_COUNTER_MAGIC_NUMBER)
        ||(saved.frame_counter != mibeacon_frame_counter))
    {
        saved.magic_number = MIBEACON_FRAME_COUNTER_MAGIC_NUMBER;
        saved.frame_counter = mibeacon_frame_counter;
        uint32_t irq_stored = ry_hal_irq_disable();
        mibeacon_frame_counter_save_to_flash(&saved);
        ry_hal_irq_restore(irq_stored);
    }
}

/**
 * @brief   API to init mibeacon
 *
 * @param   None
 *
 * @return  None
 */
void mibeacon_init(void)
{
    ry_timer_parm timer_para;

    /* Create the timer once */
    timer_para.timeout_CB = mibeacon_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "mibeacon_timer";
    mibeacon_timer = ry_timer_create(&timer_para);
    if (mibeacon_timer == 0) {
        LOG_ERROR("Timer create fail.\r\n");
    }

    mibeacon_frame_counter_info_init();
}



/**
 * @brief   API to build mibeacon payload
 *
 * @param   ...
 *
 * @return  length of mibeacon payload
 */
u8_t mibeacon_build(u8_t* data, u16_t framectrl, u32_t framecnt, u8_t cap, u8_t* object, u8_t olen) 
{
    u8_t *p = data;
    
    *p++ = 0x16;
    *p++ = LO_UINT16(UUID_MI_SERVICE);
    *p++ = HI_UINT16(UUID_MI_SERVICE);
    *p++ = LO_UINT16(framectrl);
    *p++ = HI_UINT16(framectrl);
    *p++ = LO_UINT16(APP_PRODUCT_ID);
    *p++ = HI_UINT16(APP_PRODUCT_ID);
    *p++ = (u8_t)(framecnt); 

    if (framectrl & MIBEACON_ITEM_MAC_INCLUDE) {
        *p++ = g_BLEMacAddress[5];
        *p++ = g_BLEMacAddress[4];
        *p++ = g_BLEMacAddress[3];
        *p++ = g_BLEMacAddress[2];
        *p++ = g_BLEMacAddress[1];
        *p++ = g_BLEMacAddress[0];
    }

    if (framectrl & MIBEACON_ITEM_CAP_INCLUDE) {
        *p++ = cap;
    }

    if (framectrl & MIBEACON_ITEM_OBJ_INCLUDE) {
        ry_memcpy(p, object, olen);
        p += olen;
    }

    if (framectrl & MIBEACON_ITEM_SEC_ENABLE) {
        /* frame counter */
        *p++ = (u8_t)(framecnt&0xFF000000)>>24;
        *p++ = (u8_t)(framecnt&0xFF000000)>>16;
        *p++ = (u8_t)(framecnt&0xFF000000)>>8;
    }

    return p-data;
}


/**
 * @brief   Internal API to send mibeacon
 *
 * @param   ...
 *
 * @return  length of mibeacon payload
 */
void mibeacon_send(u8_t *object, u8_t olen, u8_t isConnect, int timeout)
{
    u8_t* p;
    u16_t framectrl = MIBEACON_ITEM_MAC_INCLUDE | MIBEACON_ITEM_OBJ_INCLUDE | MIBEACON_VER;
    
    u8_t *data = ry_malloc(31);
    if (data == NULL) {
        LOG_ERROR("[%s] No mem.\r\n", __FUNCTION__);
        return;
    }

    data[0] = 0x02; 
    data[1] = 0x01; 
    data[2] = 0x05;
    
    /* Build whole mibeacon packet */
    p = &data[3];
    *p = mibeacon_build(p+1, framectrl, mibeacon_frame_counter++, 0, object, olen);

    /* Send mibeacon through ADV data */
    if(mibeacon_NeedStopFirst())//
    {
        ry_timer_stop(mibeacon_timer);
        AppAdvStop();
        wakeup_ble_thread();
        ryos_delay_ms(10);//should wait ble thread stop adv
        LOG_DEBUG("[mibeacon] wait finished adv should already stopped\n");       //
    }

    RyeexAdvMibeaconSend(data, p[0]+1+3, 100, 1000);

    ry_timer_start(mibeacon_timer,
            timeout,
            mibeacon_timeout_handler,
            NULL);

    wakeup_ble_thread();

    ry_free(data);


    return;
}

#if 0
void mibeacon_click(u16_t keycode, u8_t type)
{
    u8_t object[6] = {0x01, 0x10, 0x03, 0x00, 0x00, 0x00};

    /* Build object */
    object[3] = LO_UINT16(keycode);
    object[4] = HI_UINT16(keycode);
    object[5] = type;

    mibeacon_send(object, 6, 0, 1000);
}
#endif

void mibeacon_click(u8_t* data, int len)
{
    u8_t object[6] = {0x01, 0x10, 0x03, 0x00, 0x00, 0x00};

    if (len > 3) {
        return;
    }
    
    /* Build object */
    ry_memcpy(&object[3], data, len);
    mibeacon_send(object, 6, 0, 1000);
}


void mibeacon_sleep(u8_t type)
{
    u8_t object[4] = {0x02, 0x10, 0x01, 0x00};

    /* Build object */
    object[3] = type;

    LOG_DEBUG("[%s] type: %d\r\n", __FUNCTION__, type);
    mibeacon_send(object, 4, 0, 5000);
}


void mibeacon_simple_binding(void)
{
    u8_t object[5] = {0x02, 0x00, 0x02, 0x01, 0x10};
    mibeacon_send(object, 5, 1, 5000);
}


void mibeacon_band_legacy(void)
{
    
}



void mibeacon_band_info(void)
{
    u8_t object[5] = {0x02, 0x00, 0x02, 0x01, 0x10};
	mibeacon_send(object, 5, 1, 1000);
}

void mibeacon_on_save_frame_counter(void)
{
    mibeacon_frame_counter_info_save();
}

