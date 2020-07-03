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
#include "ry_mcu_config.h"

/* Board */
#include "board.h"
#include "ry_hal_inc.h"

#include "ry_gsensor.h"
#include "ry_gui.h"
#include "am_hal_sysctrl.h"

#include "touch.h"
#include "ry_nfc.h"
#include "am_util_delay.h"
#include "led.h"

/* Subsystem API */
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
#include "am_bsp_gpio.h"
#include "scheduler.h"
#include "ry_tool.h"
/* Application Specific */
#include "ryos_timer.h"
#include "device_management_service.h"
#include "timer_management_service.h"
#include "board_control.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

extern ry_queue_t       *ry_queue_evt_gui;
extern ry_queue_t       *ry_queue_evt_app;
extern ry_queue_t       *ry_queue_alg;

// #include "k_app.c"

/*********************************************************************
 * CONSTANTS
 */ 

 /*********************************************************************
 * TYPEDEFS
 */
#define LED_BREATH_TIME_PER_CIRCLE  60
 
/*********************************************************************
 * VARIABLES
 */
ry_queue_t       *ry_queue_evt_rtc;
ryos_thread_t    *test_thread1_handle;
ryos_thread_t    *app_thread2_handle;
ry_queue_t       *ry_queue_evt_app;
int se_inited = 0;
int ble_inited = 0;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

//*****************************************************************************
//
// Sleep function called from FreeRTOS IDLE task.
// Do necessary application specific Power down operations here
// Return 0 if this function also incorporates the WFI, else return value same
// as idleTimeg
//
//*****************************************************************************
uint32_t am_freertos_sleep(uint32_t idleTime)
{
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    return 0;
}

//*****************************************************************************
//
// Recovery function called from FreeRTOS IDLE task, after waking up from Sleep
// Do necessary 'wakeup' operations here, e.g. to power up/enable peripherals etc.
//
//*****************************************************************************
void am_freertos_wakeup(uint32_t idleTime)
{
    return;
}

void rtc_event_cb(void)
{
    
#if 1
    ry_sts_t status = RY_SUCC;
    ry_hal_rtc_get();
    am_hal_rtc_time_t *p_time = &hal_time; 

    if (ry_queue_evt_rtc) {
        if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_rtc, &p_time, RY_OS_NO_WAIT))){
            LOG_ERROR("[RTC_event_cb]: Add to rtc Queue fail. status = 0x%04x\n", status);
        }
    }
#endif
    
#if 0
    static uint8_t cnt;
    if (++cnt > 10){
        // oled_fill_color(COLOR_RAW_RED, 1000);
        cnt = 0;
    }

    //send msg to gui thread
    static gui_evt_msg_t gui_msg;
    gui_msg.evt_type = GUI_EVT_RTC;
    gui_msg.pdata = (void*)&hal_time;
    gui_evt_msg_t *p_msg = &gui_msg;
    if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
        ry_board_debug_printf("[RTC_event_cb]: Add to gui Queue fail. status = 0x%04x\n", status);
    }
#endif    
}

int test_thread_entry(void* arg)
{
    ry_sts_t      status;
    static int    cnt = 0;

    LOG_DEBUG("[Test_Thread]: Enter.\r\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_TEST);
#endif
    status = ryos_queue_create(&ry_queue_evt_rtc, 5, sizeof(void*));

    ry_hal_rtc_start();
    set_rtc_int_every_second();

#if (!WATCH_DOG_START_FIRST)
    ry_hal_wdt_init();
    ry_hal_wdt_start();
#endif    
    while(1)
    {
        am_hal_rtc_time_t *msg = NULL;
        //am_bsp_pin_enable(AUTO_RESET);
        //auto_reset_start();
        if (RY_SUCC == ryos_queue_recv(ry_queue_evt_rtc, &msg, RY_OS_WAIT_FOREVER)) {    
            //LOG_DEBUG("[test_thread_entry] RTC time: %02d-%02d-%02d %02d : %02d : %02d : %02d\r\n", \
            //    msg->ui32Year, msg->ui32Month, msg->ui32DayOfMonth, msg->ui32Hour, msg->ui32Minute, msg->ui32Second, msg->ui32Hundredths);

            ry_sched_msg_send(IPC_MSG_TYPE_RTC_1S_EVENT, 0, NULL);
        }
        
        //testbench_machine();        
        //module_power_control();
        // ryos_delay_ms(1000);
        
        
    }
}

extern int dc_in_rising_cnt, dc_in_falling_cnt;
void app_thread_entry(void)
{
    ry_sts_t status = RY_SUCC;
    int num;
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_APP);
#endif
    status = ryos_queue_create(&ry_queue_evt_app, 5, sizeof(void*));

    static uint32_t app_cnt = 0;

    for (int i = 0; i < 8; i++) {
        sys_get_battery();
    }
    
    while(1){
        ryos_delay_ms(1000);
        app_cnt++;

        #if 0
        //charge_repo();
        if (app_cnt % 8 == 0){
            sys_get_battery();      // initialize battery get, for get right median value
		    sys_get_battery();
		    sys_get_battery();
        }
        
        //LOG_DEBUG("DC_IN r_cnt:%d, DC_IN f_cnt:%d\r\n", dc_in_rising_cnt, dc_in_falling_cnt);

#if (RT_LOG_ENABLE == TRUE)
        log_data(TR_24_TEST_DATA, app_cnt);
#endif

        ry_hal_wdt_reset();
        
        static uint32_t log_tick = 0;
        //am_hal_itm_print_test((u8_t)log_tick);
        if ((log_tick++)%30 == 0){
            //log_tick = 0;
            if (wms_touch_is_busy() == 0) {
                touch_reset();
            }
            //cy8c4011_gpio_init();
            // LOG_DEBUG("reset tp\n");
        }

#if 0
        //hrm auto sample per 10min for 10 second
        static uint32_t tick_hrm_auto_sample;
        if (is_auto_heart_rate_open()){
            if (tick_hrm_auto_sample == 10){
                if (get_hrm_state() != PM_ST_POWER_START){
                    _start_func_hrm(HRM_START_AUTO);
                    set_hrm_working_mode(HRM_MODE_AUTO_SAMPLE);
                }
            }
            else if(tick_hrm_auto_sample == 20){
                if (get_hrm_working_mode() == HRM_MODE_AUTO_SAMPLE){
                    //send msg of data store here
                    
                    pcf_func_hrm();   
                }
            }
            if (++tick_hrm_auto_sample > 600){
                tick_hrm_auto_sample  = 0;
            }
        }
  
		alg_msg_t *msg = NULL;		
        if (RY_SUCC == ryos_queue_recv(ry_queue_alg, &msg, RY_OS_NO_WAIT)) {    
            // gui_msg_handler(msg);
            switch (msg->msg_type) {
                case ALG_MSG_GENERAL:
                    sensor_alg_t* alg = (sensor_alg_t*)msg->pdata; 
                    LOG_DEBUG("get_ry_queue_alg, type: %d, mode: %d \r\n", msg->msg_type, alg->sports_mode);
                    break;
                case ALG_MSG_SLEEP:
                    sleep_data_t* sleep = (sleep_data_t*)msg->pdata;
                    LOG_DEBUG("get_ry_queue_alg, type: %d, hrm: %d, sstd: %d, rri_0: %d, rri_1: %d, rri_2: %d \r\n",\
                        msg->msg_type, sleep->hrm, sleep->sstd, sleep->rri & 0x3f, (sleep->rri >> 6) & 0x1f, (sleep->rri >> 11) & 0x3f);
                    break;
                case ALG_MSG_SLEEP_LOG_START:
                    am_hal_rtc_time_t* time = (am_hal_rtc_time_t*) msg->pdata;
                    break;
                case ALG_MSG_SLEEP_LOG_END:
                    am_hal_rtc_time_t* time_end = (am_hal_rtc_time_t*) msg->pdata;
                    break;
                case ALG_MSG_HRM_MANUAL_STOP:
                    sensor_alg_t* alg_now = (sensor_alg_t*)msg->pdata; 
                    LOG_DEBUG("get_ry_queue_alg_hrm_stop, type: %d, hrm_cnt: %d \r\n", msg->msg_type, alg_now->hr_cnt);
                    break;
                default:
                    break;                        
            }
        }
#endif
				
				#endif
    }
}

/*
 * app_init - Application Initialization
 *
 * @param   None
 *
 * @return  Status of the application
 */
ry_sts_t app_init(void)
{
    ryos_threadPara_t para;
#if 1
    /* Start Test Demo Thread */
    strcpy((char*)para.name, "test_thread1");
    para.stackDepth = 128;
    para.arg        = NULL; 
    para.tick       = 20;
    para.priority   = 3;
    ryos_thread_create(&test_thread1_handle, test_thread_entry, &para);

	/* Start app Demo Thread */
    strcpy((char*)para.name, "app_thread2");
    para.stackDepth = 128;
    para.arg		 = NULL; 
    para.tick		 = 20;
    para.priority	 = 3;
    ryos_thread_create(&app_thread2_handle, (ryos_threadFunc_t)app_thread_entry, &para);
#endif

    return RY_SUCC;
}

void kernel_se_init_done(void)
{
    ry_sts_t status;
    ry_nfc_set_initdone_cb(NULL);

    se_inited = 1;
    
    

    //if (RY_SUCC != (status = ry_gsensor_init())) {
    //    LOG_ERROR("GSensor init fail.\r\n");
        //while(1);
    //}

    //if (RY_SUCC != (status = sensor_alg_init())){
	//	LOG_ERROR("NFC init fail.\r\n");
    //    while(1);
	//}

    //ry_tool_init();
}

void kernel_init_done(void)
{
    ry_sts_t status;
    LOG_DEBUG("Kernel Init Done\r\n");

    //board_control_init();
    ble_inited = 1;

    ry_hal_wdt_start();
}
    

/*
 * subsystem_init - Subsystem Initialization 
 *
 * @param   None
 *
 * @return  Status of the subsystem
 */
ry_sts_t subsystem_init(void)
{
    ry_sts_t status = RY_SUCC;

    do {
	    device_info_init();

#if (RY_GUP_ENABLE == TRUE)
        if (RY_SUCC != (status = ry_gup_init())) {
            break;
        }
#endif

#if (RY_TOOL_ENABLE == TRUE)
        //if (RY_SUCC != (status = ry_tool_init())) {
        //    break;
        //}
#endif

#if (RY_HRM_ENABLE == TRUE)
        hrm_init();
#endif

#if (RY_FP_ENABLE == TRUE)
        if (RY_SUCC != (status = ry_fp_init())) {
            break;
        }
#endif

#if (RY_BLE_ENABLE == TRUE) 
        if (RY_SUCC != (status = ry_ble_init(kernel_init_done))) {
            LOG_ERROR("BLE Init Fail. Status: 0x%04x\r\n", status);
            break;
        }
#endif

#if (RY_SE_ENABLE == TRUE)
        if (RY_SUCC != (status = ry_se_init(NULL))) {
            break;
        }
#endif

#if (RY_GSENSOR_ENABLE == TRUE)
        if (RY_SUCC != (status = ry_gsensor_init())) {
            break;
        }
#endif

#if (RY_GUI_ENABLE == TRUE)
        if (RY_SUCC != (status = ry_gui_init())) {
            break;
        }
        am_bsp_pin_enable(AUTO_RESET);
#endif

#if (RY_TOUCH_ENABLE == TRUE)
        if (RY_SUCC != (status = touch_init())) {
            break;
        }
        am_bsp_pin_enable(AUTO_RESET);
#endif

#if (RY_FS_ENABLE == TRUE)
        if (RY_SUCC != (status = ry_filesystem_init())) {
            break;
        }
#endif

#if ((RY_HRM_ENABLE == TRUE) || (RY_GSENSOR_ENABLE == TRUE))
		if (RY_SUCC != (status = sensor_alg_init())){
		    break;
		}
#endif        

        ry_hal_wdt_init();
         
        
    } while(0);
    return status;
}


/*
 * kernel_init - Init Firmware kernel part
 *
 * @param   None
 */
void kernel_init(void)
{
    /* In the Kernel part, make sure the interrupt is all disabled at first */
    //ry_hal_irq_disable();

    /* Start scheduler task first */
    ry_sched_init();
}


#if 0
#include "aes.h"
#include "des.h"

u8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
                    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

u8_t nonce[4] = {0x7B, 0x00, 0x00, 0x00};

u8_t iv[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 
                   0x99, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

u8_t g_test_buf[] =  {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 
                   0x7B, 0x00, 0x00, 0x00, 
                   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x0A, 0x0B,
                   0x77, 0xB0, 0xEF, 0x5A,
                   0x32, 0x32, 
                   0x00, 0x00, 0x00, 0x00};

 u8_t g_test_buf_dec[] =  {0xfc, 0x3e, 0x3c, 0x7b, 0x3d, 0xa0, 0x65, 0x7c, 
											 0x71, 0x7d, 0xe5, 0x19, 0x70, 0x92, 0x67, 0xff,
											 0x46, 0x41, 0x3c, 0x0e, 0xad, 0x26, 0x64, 0xf3,
											 0x0d, 0x0d, 0xef, 0x62, 0x88, 0x9d, 0x36, 0xdc};

void aes_verify(void)
{
    ry_sts_t status;
    u8_t key[16];
    u8_t this_iv[16];

#if 1
    LOG_DEBUG("---------------ENCRYPT-----------\r\n");

    for (int i = 0; i < 16; i++) {
	      key[i] = i;
    }
    ry_memcpy(this_iv, iv, 16);
    status = ry_aes_cbc_encrypt(key, this_iv, sizeof(g_test_buf), g_test_buf, g_test_buf);
    if (status) {
        LOG_ERROR("AES Encrypt Error. %x \r\n", status);
        return;
    }
#endif

    LOG_DEBUG("---------------DECRYPT-----------\r\n");
    for (int i = 0; i < 16; i++) {
	      key[i] = i;
    }
    ry_memcpy(this_iv, iv, 16);
    status = ry_aes_cbc_decrypt(key, this_iv, sizeof(g_test_buf_dec), g_test_buf_dec, g_test_buf_dec);
    if (status) {
        LOG_ERROR("AES Decrypt Error. %x \r\n", status);
        return;
    }

}


void aes_test(void)
{
    //mbedtls_aes_self_test(1);
    // mbedtls_des_self_test(1);
	
    aes_verify();
}
#endif

void* __wrap_malloc(uint32_t size)
{
    void* pData = ry_malloc(size);
    LOG_ERROR("[libc_malloc] alloc addr:0x%08X, size:%d", pData, size);
    return pData;
}

void __wrap_free(void* pData)
{
    ry_free(pData);
    LOG_ERROR("[libc_malloc] free addr:0x%08X", pData);
}


void* malloc(uint32_t size)
{
    return __wrap_malloc(size);
}

void free(void* pData)
{
    __wrap_free(pData);
}

//#pragma import(__use_no_heap)
//#pragma import(__use_no_heap_region)  //打开会链接错误


/*
 * main - Entry Point of Project
 *
 * @param   None
 */
int main(void)
{
    int percent;
    int led_cnt = LED_BREATH_TIME_PER_CIRCLE;
    static uint8_t led_breath = 0;

    ry_sts_t status = RY_SUCC;

#if (WATCH_DOG_START_FIRST)
    ry_hal_wdt_init(); 
    ry_hal_wdt_start();
#endif

    /* init hw board */
    ry_board_init();

    //auto_reset_init();
    //auto_reset_start();

    /* init Log module */
    ry_log_init();

#if 1
    ry_delay_ms(1000);
    
    for (int i = 0; i < 8; i++) {
        percent = sys_get_battery_percent(BATT_DETECT_AUTO);
    }
    
    // ry_led_onoff_with_effect(1, 50);
    //while (1) {   
    while (charge_cable_is_input()) {
        LOG_DEBUG("Power on percent: %d\r\n", percent);
        if (++led_cnt > LED_BREATH_TIME_PER_CIRCLE){
            led_cnt = 0;
        }

        if (percent > 0) {
            if (led_breath){
                led_stop();
                led_breath = 0;
            }
            break;
        }
        
        if (led_cnt == 0){
            led_breath = 1;
            led_set_working(1, 20, LED_BREATH_TIME_PER_CIRCLE * 1000);                        
        }

        for (int i = 0; i < 8; i++) {
            percent = sys_get_battery_percent(BATT_DETECT_AUTO);
        }

        ry_hal_wdt_reset();
        ry_delay_ms(1000);
    }
		ry_led_onoff_with_effect(0, 0);
#endif

    /* init ryeex os */ 
    ryos_init();


#if (CLI_ENABLE == TRUE) 
    ry_cli_init();
#endif

#if (DIAGNOSTIC_ENABLE == TRUE) 
    ry_diag_init();
#endif

#if 1
    kernel_init();
#endif 

    //aes_test();
    //print_cert();

    /* Enable global interrupts */
    //__enable_irq();

    /* OS running */
    ryos_start_schedule();
}

/* [] END OF FILE */
