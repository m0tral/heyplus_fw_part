/*
* Copyright (c), Ryeex Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "app_config.h"
#include "ry_mcu_config.h"

/* Board */
#include "board.h"
#include "ry_hal_inc.h"

/* Application Specific */
#include "am_bsp_gpio.h"
#include "board_control.h"
#include "am_devices_cy8c4014.h"
#include "am_devices_rm67162.h"
#include "touch.h"
#include "app_interface.h"
#include "string.h"
#include "dip.h"

#include "json.h"
#include "gui.h"
#include "device_management_service.h"
#include "data_management_service.h"
#include "scheduler.h"
#include <stdio.h>

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

/*********************************************************************
 * CONSTANTS
 */

#define MAX_TP_NUMBER_PER_SESSION       6
#define OLED_IMG_24BIT                  (120*240*3)

/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    u8_t          id;
    u8_t          version;
    u32_t         idleSampleRate;
    u32_t         activeSampleRate;
} tp_info_t;

typedef struct {
    u16_t         y; // value
    u32_t         t; // timestamp
} tp_data_t;

typedef struct {
    u32_t         num;
    tp_data_t     datas[MAX_TP_NUMBER_PER_SESSION];
    u32_t         duration;
    u16_t         newY;
    u16_t         newYCnt;
    u32_t         newT;
    u16_t         lastY;
} tp_session_t;


typedef struct {
    u32_t         curState;
    u8_t          lastAction;
    tp_session_t* session;
    tp_info_t     info;
    u32_t         idleScanIntvl;
    u32_t         activeScanIntvl;
    u8_t          newVThreshold;
} ry_tp_ctx_t;
 
/*********************************************************************
 * LOCAL VARIABLES
 */
extern u32_t factory_test_start;
ry_tp_ctx_t      ry_tp_ctx_v;
ryos_semaphore_t *touch_sem;
ryos_mutex_t    *touch_mutex;

static u32_t    touch_test_mode = 0;
rpc_ctx_t*      touch_test_ctx  = NULL;
u32_t           touch_raw_report = 0;
/*
 * @brief touch Thread handle
 */
ryos_thread_t *touch_thread_handle;

u8_t verify_data = 0;

int touch_halt_flag;
int touch_bypass_flag = 0;

extern int g_lastTouchTime;

/*********************************************************************
 * LOCAL FUNCTIONS
 */


void touch_isr_handler(void)
{
    ryos_semaphore_post(touch_sem);

#if (RT_LOG_ENABLE == TRUE)
    log_event(TR_T_TOUCH_EVENT);
#endif

}

bool is_touch_in_test(void) {
    return touch_test_mode == 1;
}

/**
 * @brief   API to set touch enter test mode.
 *
 * @param   None
 *
 * @return  None
 */
void touch_set_test_mode(u32_t fEnable, rpc_ctx_t* ctx)
{
    if (fEnable) {
        touch_test_mode = 1;
        verify_data = 0;
        touch_test_ctx = ctx;
    } else {
        touch_test_mode = 0;
        touch_test_ctx  = NULL;
        verify_data = 0;
    }
}

void touch_set_raw_report_mode(u32_t fEnable, rpc_ctx_t* ctx)
{
    if (fEnable) {
        touch_raw_report = 1;
        touch_test_ctx = ctx;
    } else {
        touch_raw_report = 0;
        touch_test_ctx  = NULL;
    }
}

void touch_disable(void)
{
    LOG_DEBUG("Touch disable.\r\n");

    //if (ry_hal_gpio_get(GPIO_IDX_TOUCH_IRQ) == 0) {
        cy8c4011_gpio_uninit();
    //}
}

void touch_enable(void)
{
    LOG_DEBUG("Touch enable.\r\n");
    cy8c4011_gpio_init();
    ry_hal_i2cm_uninit(I2C_TOUCH);
}

void touch_halt(int enable)
{
    if (enable) {
        touch_halt_flag = 1;
        touch_disable();
    } else {
        touch_halt_flag = 0;
        touch_enable();
    }
}


void touch_reset(void)
{
    if (touch_halt_flag == 1) {
        return;
    }

    // LOG_DEBUG("Touch reset.\r\n");
    //touch_enable();

    //ryos_mutex_lock(touch_mutex);

    //cy8c4011_gpio_init();
    //ry_hal_i2cm_uninit(I2C_TOUCH);

    //ryos_mutex_unlock(touch_mutex);
}

void touch_mode_set(tp_mode_t mode)
{
    ryos_mutex_lock(touch_mutex);
    cy8c4011_set_mode((u8_t)mode);    
    ry_hal_i2cm_uninit(I2C_TOUCH);
    ryos_mutex_unlock(touch_mutex);
}

void touch_bypass_set(u8_t enable)
{
    if (enable) {
        touch_bypass_flag = 1;
    } else {
        touch_bypass_flag = 0;
    }
}


void touch_baseline_reset(void)
{
    am_device_cy8c4014_clear_raw();
}


void touch_set_interval(u8_t interval)
{
    cy8c4011_set_sleep_interval(interval);
}

/**
 * @brief   API to set Touch into low power mode
 *          - button only and un-initial I2C
 *
 * @param   None
 *
 * @return  None
 */
void touch_enter_low_power(void)
{
    if (ry_hal_gpio_get(GPIO_IDX_TOUCH_IRQ) == 0) {
        cy8c4011_set_mode(TP_WORKING_BTN_ONLY);
        cy8c4011_set_sleep_interval(200);
        ry_hal_i2cm_uninit(I2C_TOUCH);  
    }
}

/**
 * @brief   API to set Touch exit low power
 *          - initial I2C
 *
 * @param   None
 *
 * @return  None
 */
void touch_exit_low_power(void)
{
    ry_hal_i2cm_init(I2C_TOUCH);
}

void touch_test_display(u8_t key_hit, u8_t verify_data)
{
	u8_t para[6];
    //extern ryos_mutex_t* spi_mutex;
    //ryos_mutex_lock(spi_mutex);
    if((key_hit&0x01) && ((verify_data&0x01) == 0))
    {
        am_devices_rm67162_display_normal_area(0,119,0,47);
        am_devices_rm67162_fixed_data_888(0xff0000, OLED_IMG_24BIT/5);
        // *(u32_t*)para = 0xff0000;
        // para[4] = 0;
        // para[5] = 47;
        // ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR, 6, para);
        LOG_DEBUG("[touch]Fill_color. %x, verify_data:%d.\r\n", 1, verify_data);
    }

    if((key_hit&0x02) && ((verify_data&0x02) == 0))
    {
        am_devices_rm67162_display_normal_area(0,119,48,95);
        am_devices_rm67162_fixed_data_888(0x00ff00, OLED_IMG_24BIT/5);
        // *(u32_t*)para = 0x00ff00;
        // para[4] = 48;
        // para[5] = 95;
        // // ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR, 6, para);
        LOG_DEBUG("[touch]Fill_color. %x, verify_data:%d.\r\n", 2, verify_data);
    }

    if((key_hit&0x04) && ((verify_data&0x04) == 0))
    {
        am_devices_rm67162_display_normal_area(0,119,96,143);
        am_devices_rm67162_fixed_data_888(0x0000ff, OLED_IMG_24BIT/5);
        // *(u32_t*)para = 0x0000ff;
        // para[4] = 96;
        // para[5] = 143;
        // ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR, 6, para);
        LOG_DEBUG("[touch]Fill_color. %x, verify_data:%d.\r\n", 3, verify_data);
    }

    if((key_hit&0x08) && ((verify_data&0x08) == 0))
    {
        am_devices_rm67162_display_normal_area(0,119,144,191);
        am_devices_rm67162_fixed_data_888(0xff00ff, OLED_IMG_24BIT/5);
        // *(u32_t*)para = 0xff00ff;
        // para[4] = 144;
        // para[5] = 191;
        // ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR, 6, para);
        LOG_DEBUG("[touch]Fill_color. %x, verify_data:%d.\r\n", 4, verify_data);
    }

    if((key_hit&0x10) && ((verify_data&0x10) == 0))
    {
        am_devices_rm67162_display_normal_area(0,119,192,239);
        am_devices_rm67162_fixed_data_888(0xffffff, OLED_IMG_24BIT/5);
        // *(u32_t*)para = 0xffffff;
        // para[4] = 192;
        // para[5] = 239;
        // ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR, 6, para);
        LOG_DEBUG("[touch]Fill_color. %x, verify_data:%d.\r\n", 5, verify_data);
    }
    //ryos_mutex_unlock(spi_mutex);
}

/**
 * @brief   touch thread
 *
 * @param   None
 *
 * @return  None
 */
int touch_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    tp_session_t *session;
    u16_t curY;
    u32_t curT;
    tp_event_t event;
    u8_t hw_ver[10] = {0};
    u8_t hw_ver_len;
    u8 bReport = 0;

    u8_t key_hit;
    int  bFull;

    LOG_DEBUG("[touch_thread] Enter.\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_TOUCH);
#endif

    status = ryos_semaphore_create(&touch_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[touch_thread]: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    status = ryos_mutex_create(&touch_mutex);
    if (RY_SUCC != status) {
        LOG_ERROR("[touch_thread]: Mutex Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }

    /* Start Touch Hardware */
    cy8c4011_gpio_init();

    uint32_t tp_id = touch_get_id();
    LOG_INFO("touch_ID: 0x%4x  \r\n", touch_get_id());
	
#if UPGEADE_TP_ENABLE
	char TpidStr[10] = {0};
	snprintf(TpidStr,6,"0x%x",touch_get_id());
	LOG_DEBUG("\r\n TpidStr=%s \r\n",TpidStr);
    if (data_device_restart_type_get() != UPGRADE_RESTART_COUNT){
        if(memcmp(TpidStr,"0x10d",5) != 0){
            LOG_DEBUG("tp upgrade started.\r\n");
            tp_firmware_upgrade();
        }
    }
#endif

    /* Get basic info */
    ry_tp_ctx_v.activeScanIntvl = 10;
    ry_tp_ctx_v.newVThreshold   = 1;

    /* Send IPC message to notify system */
    ry_sched_msg_send(IPC_MSG_TYPE_TOUCH_INIT_DONE, 0, NULL);
    
    while (1) {
        status = ryos_semaphore_wait(touch_sem);

        if (touch_bypass_flag) {
            continue;
        }

        //LOG_DEBUG("[touch_thread]: T1\r\n");

        // Filter abnormal interupt
        //ryos_delay_ms(20);

        ryos_mutex_lock(touch_mutex);
        
        ry_hal_i2cm_init(I2C_TOUCH);
        if (RY_SUCC != status) {
            LOG_ERROR("[touch_thread]: Semaphore Wait Error.\r\n");
            ryos_mutex_unlock(touch_mutex);
            continue;
        }

        /* Check if there is any remained session */
        if (ry_tp_ctx_v.session) {
            /* TODO: For debug, just use while(1) first. */
            LOG_WARN("Session remianed. Session num:%d\r\n", ry_tp_ctx_v.session->num);
            //while(1);
            ry_free(ry_tp_ctx_v.session);
        }

        /* Allocate a new session */
        ry_tp_ctx_v.session = (tp_session_t*)ry_malloc(sizeof(tp_session_t));
        if (NULL == ry_tp_ctx_v.session) {
            /* TODO: report FATAL error and reset system. */
            LOG_ERROR("[touch_thread] Session No Mem.\r\n");
            while(1);
        }

        session = ry_tp_ctx_v.session;
        ry_memset((void*)session, 0, sizeof(tp_session_t));
        session->newY = 0xFF;
        session->lastY = 0xFF;

        /* Fetch tp sensor data */
        while (ry_hal_gpio_get(GPIO_IDX_TOUCH_IRQ)) {

            if (ry_hal_is_i2cm_touch_enable() == 0) {
                cy8c4011_gpio_int_init();
                ry_hal_i2cm_init(I2C_TOUCH);
            }

            curY = tp_get_data();
            curT = ry_hal_clock_time();
            g_lastTouchTime = ry_hal_clock_time();
            //LOG_DEBUG("[touch_thread] raw get. y:%x, t:%d\r\n", curY, curT);

            if (curY != session->newY) {
                session->newY    = curY;
                session->newT    = curT;
                session->newYCnt = 0;
            }

            if (session->newY != session->lastY) {
                session->newYCnt++;
            }

            if (session->newYCnt >= ry_tp_ctx_v.newVThreshold ||
                (session->newY == 0 && session->lastY == 0xFF)) {
                session->datas[session->num].y = session->newY;
                session->datas[session->num].t = session->newT;
                session->newYCnt = 0;
                session->lastY   = session->newY;

                //LOG_DEBUG("[touch_thread], y:%x, t:%d, n:%d, d:%d, newCnt:%d\r\n", \
                    session->datas[session->num].y, \
                    session->datas[session->num].t, \
                    session->num, \
                    session->duration);
                
                if (session->num == 0) {
                    if (touch_test_mode) {
                        key_hit = tp_get_origin_data();
                        LOG_DEBUG("[touch] get data. %x\r\n", key_hit);
                        touch_test_display(key_hit, verify_data);
                        verify_data |= key_hit;
                        if ((verify_data & 0x1F) == 0x1F) {
                            status = rpc_touch_data_report(touch_test_ctx, 0, 1);
                            verify_data = 0;
                            touch_bypass_flag = 1;
                            goto exit;                            
                        }
                        LOG_DEBUG("[touch] verify data. %x\r\n", verify_data);

                        ryos_delay_ms(ry_tp_ctx_v.activeScanIntvl);
                        continue;
                    }
                    
                    /* Issue the TOUCH_DOWN event */
                    event.action = TP_ACTION_DOWN;
                    event.x      = 0;
                    event.y      = session->datas[session->num].y;
                    event.t      = session->datas[session->num].t;
                    bReport = 1;
                    if (!factory_test_start)
                        ry_sched_msg_send(IPC_MSG_TYPE_TOUCH_RAW_EVENT, sizeof(tp_event_t), (u8_t*)&event);
                } 
                else {
                    if (touch_test_mode) {
                        key_hit = tp_get_origin_data();
                        LOG_DEBUG("[touch]tp_get_origin_data. %x, verify_data:%d.\r\n", 3, verify_data);
                        touch_test_display(key_hit, verify_data);
                        verify_data |= key_hit;
                        if ((verify_data & 0x1F) == 0x1F) {
                            status = rpc_touch_data_report(touch_test_ctx, 0, 1);
                            verify_data = 0;
                        }
                        ryos_delay_ms(ry_tp_ctx_v.activeScanIntvl);
                        continue;
                    }
                    
                    /* Issue the TOUCH_MOVE event */
                    event.action = TP_ACTION_MOVE;
                    event.x      = 0;
                    event.y      = session->datas[session->num].y;
                    event.t      = session->datas[session->num].t;
                    bReport = 1;
                    if (!factory_test_start)
                        ry_sched_msg_send(IPC_MSG_TYPE_TOUCH_RAW_EVENT, sizeof(tp_event_t), (u8_t*)&event);
                }
                session->num++;
                if (session->num >= MAX_TP_NUMBER_PER_SESSION) {
                    bFull = 1;
                    break;
                }
            }
            ryos_delay_ms(ry_tp_ctx_v.activeScanIntvl);
        }

        if (touch_test_mode == 0) {
            event.action = TP_ACTION_UP;
            event.x      = 0;
            event.y      = session->datas[session->num-1].y;
            event.t      = ry_hal_clock_time();
            bReport = 1;
            if (!factory_test_start)
                ry_sched_msg_send(IPC_MSG_TYPE_TOUCH_RAW_EVENT, sizeof(tp_event_t), (u8_t*)&event);
        }

exit:
        ry_free((void*)session);
        ry_tp_ctx_v.session = NULL;
        ryos_mutex_unlock(touch_mutex);
        if (bFull) {
            bFull = 0;
            am_devices_cy8c4014_reset();
            //touch_reset();
            LOG_DEBUG("[touch] Full reset.\r\n");
        }
    }
}

/**
 * @brief   API to init Touch module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t touch_init(void)
{
    ryos_threadPara_t para;

    /* Init touch module context */
    ry_memset((void*)&ry_tp_ctx_v, 0, sizeof(ry_tp_ctx_t));
    
    /* Start BLE Thread */
    strcpy((char*)para.name, "touch_thread");
    para.stackDepth = 384;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 8;
    ryos_thread_create(&touch_thread_handle, touch_thread, &para);  

    return RY_SUCC;
}

/* [] END OF FILE */
