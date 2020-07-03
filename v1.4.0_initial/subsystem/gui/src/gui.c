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
#include "gfx.h"
#include "gui.h"
#include "src/gwin/gwin_class.h"

#if (!WIN32_SIM_MK)
/* Basic */			
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_mcu_config.h"
#include "app_config.h"
#include <ry_hal_inc.h>

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
#endif

#include "ryos.h"
#include "ryos_timer.h"
#include "gui_bare.h"
#include "app_pm.h"
#include "board_control.h"
#include "led.h"

#include "am_bsp_gpio.h"
#include "scheduler.h"
#include "activity_charge.h"
#include "board_control.h"
#include "device_management_service.h"
#include "am_devices_cy8c4014.h"
#include "am_devices_rm67162.h"

#include "ry_nfc.h"
#include "touch.h"

#include "gui_img.h"
#include "window_management_service.h"
#include "timer_management_service.h"
#include "hrm.h"
#include "motar.h"
#include "ry_statistics.h"
#include "ry_font.h"
#include "app_interface.h"


#else
	#define RY_GUI_ENABLE	1
#endif	//!(WIN32_SIM_MK)

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * CONSTANTS
 */ 

/*********************************************************************
 * VARIABLES
 */
int gui_img_offset = 0;

#if (RY_GUI_ENABLE || 1)

const char* icon_img[] = {	
							//"dial_earth.png",
							"menu_01_card.bmp",
							"menu_02_hrm.bmp",
#if IMG_SHOW_ALL
							"iconSport_06.bmp",
							"icon_weather.bmp",
							"icon_data.bmp",
							"icon_clock.bmp",
							"icon_calendar.bmp",
							"ntv_circle.ntv",							
							"icon_demo.bmp",							
#endif							
						 };

uint32_t gui_sleep_thresh;
uint32_t gui_status;
uint32_t gui_init_done = 0;

#include "gui_demo.c"
// #include "gui_constraction.c"

#if (!WIN32_SIM_MK)
#include "am_mcu_apollo.h"
// #include "app_ui_user.c"
extern int se_inited;
uint32_t gui_event_time;

ry_queue_t      *ry_queue_evt_gui;
gui_test_t 		gui_test_sc;

#define TP_SCL_PIN			42
#define TP_SDA_PIN			43
void debug_io_setting(void)
{
	am_hal_gpio_pin_config(TP_SCL_PIN, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_pin_config(TP_SDA_PIN, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_out_bit_replace(TP_SCL_PIN, 0);
    am_hal_gpio_out_bit_replace(TP_SDA_PIN, 0);
}

void gui_test_processing(gui_test_t *test_sc)
{
	if (test_sc->cmd == GUI_TS_FILL_COLOR){
		if (test_sc->hold_time > 0){
			am_devices_rm67162_display_range_set(0, (RM_OLED_WIDTH - 1), 0, (RM_OLED_HEIGHT -1));    
			am_devices_rm67162_fixed_data_888(test_sc->para[0], RM_OLED_IMG_24BIT);
			gui_delay_ms(test_sc->hold_time);
		}
		test_sc->cmd = GUI_TS_NONE;
		test_sc->hold_time = 0;
	}
}
#endif

#if 0
void gui_queue_recv(void)
{
	gui_evt_msg_t *msg = NULL;
_WAIT_MSG_PCF:	
	// _gui_log_debug("[gui_Thread]: sleep.\r\n");			
	pcf_func_oled();
	gui_status = PM_ST_POWER_DOWN;
_WAIT_MSG:
    LOG_DEBUG("GUI start listen.\r\n");
	if (RY_SUCC == ryos_queue_recv(ry_queue_evt_gui, &msg, RY_OS_WAIT_FOREVER)) {
        LOG_DEBUG("GUI get event,1 \r\n");
		am_devices_rm67162_display_normal_on();
		// gui_msg_handler(msg);
		if (msg->evt_type == GUI_EVT_TP) {
			touch_data_t *tp = (touch_data_t *)msg->pdata; 
			LOG_DEBUG("[gui] new msg - TP: %x, Evt:%d\r\n", tp->button, tp->event);
		}
		else if (msg->evt_type == GUI_EVT_TEST) {
			gui_test_t *test_sc = (gui_test_t*)msg->pdata; 
			gui_test_processing(test_sc);	
			goto _WAIT_MSG_PCF;					
		}
		else if (msg->evt_type == GUI_EVT_RTC) {
			
			//ryos_delay_ms(100);
			//void gui_queue_recv(void);
			// gui_queue_recv();
			goto _WAIT_MSG;
		}
	}
}
#endif

#if 0
void gui_thread(void)
{

    ryos_thread_set_tag(NULL, TR_T_TASK_GUI);
    
#if (!WIN32_SIM_MK)    
	ry_sts_t status = RY_SUCC;
	status = ryos_queue_create(&ry_queue_evt_gui, 5, sizeof(void*));
	_gui_log_debug("[gui_Thread]: ry_queue_evt_gui creat.\r\n");

#endif    
    
	_gui_entry_setup();
	_gui_log_debug("[gui_Thread]: Enter.\r\n");
    am_bsp_pin_enable(AUTO_RESET);
	
	gfxInit();      // gfxInit routine call gdispclear
	//  gui_disp_img_at(1, 0);
	// gui_create_cwindow();
	gui_disp_font_startup();
	// gui_disp_graph_startup();
	gwin_console_creat();

	// gui_widget_creat();
	_gui_log_debug("[gui_Thread]: gui_thread_entry initial done.\r\n");
	gui_init_done = 1;
	gui_status = PM_ST_POWER_START;
	gui_sleep_thresh = 0;
    
	while(1){
		static uint32_t cnt;
		gui_evt_msg_t *msg = NULL;		
		if (ryos_get_ms() - gui_event_time >= gui_sleep_thresh){
			gui_queue_recv();
		}
		else{
			if (RY_SUCC == ryos_queue_recv(ry_queue_evt_gui, &msg, RY_OS_NO_WAIT)) {    
				// gui_msg_handler(msg);
				LOG_DEBUG("GUI get tp event\r\n");
        	}
		}
        gui_sleep_thresh = 10 * 1000;
#if (!GUI_K_KERNEL_ENABLE)
		if (gui_status == PM_ST_POWER_DOWN){
			gui_status = PM_ST_POWER_START;
            pg_app_array[PG_T0].pg_op  = KPG_RUN;
            pg_app_array[PG_T1].pg_op  = KPG_IDLE;
            pg_app_array[PG_T2].pg_op  = KPG_IDLE;
            pg_app_array[PG_T3].pg_op  = KPG_IDLE;
			pg_app_array[PG_T4].pg_op  = KPG_IDLE;
            pg_app_array[PG_T5].pg_op  = KPG_IDLE;
            pg_app_array[PG_T6].pg_op  = KPG_IDLE;
            pg_app_array[PG_T7].pg_op  = KPG_IDLE;
			cnt = 0;
		}
		tp_cmd_detect();
		//if (gui_status == PM_ST_POWER_START){
		//	tp_cmd = 0;				
			// _gui_log_debug("[gui_Thread]: tp cmd skip. %d\r\n", tp_cmd);			
		//}			
		k_page_execute();
		// gdisp_update();

		gui_delay_ms(200);	
		// LOG_DEBUG("[gui_thread]free heap size:%d, cnt:%d\r\n", ryos_get_free_heap(), cnt++);
		// cs_printf("[gui] heap: %d\n", ryos_get_free_heap());
#else
		status = ryos_semaphore_wait(gui_sem);
		if (RY_SUCC != status) {
            LOG_ERROR("[touch_thread]: Semaphore Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        }
		LOG_DEBUG("[gui_thread]free heap size:%d, cnt:%d\r\n", ryos_get_free_heap(), cnt);		
#endif
	}	
}

#endif


#if 1

/*********************************************************************
 * CONSTANTS
 */

#define MAX_GUI_MSG_Q_NUM                   5

/*********************************************************************
 * TYPEDEFS
 */
 
/*********************************************************************
 * LOCAL VARIABLES
 */

ry_gui_ctx_t    ry_gui_ctx_v;

/*
 * @brief GUI Thread handle
 */
ryos_thread_t   *gui_thread3_handle;

/*
 * @brief GUI Thread message queue
 */
ry_queue_t      *ry_gui_msgQ;

ryos_mutex_t    *fb_mutex;



/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to get get_gui_state
 *
 * @param   None
 *
 * @return  get_gui_state result
 */
uint32_t get_gui_state(void)
{
	return ry_gui_ctx_v.curState;
}

/**
 * @brief   API to get get_gui_on_time
 *
 * @param   None
 *
 * @return  get_gui_on_time result
 */
uint32_t get_gui_on_time(void)
{
	return ry_gui_ctx_v.screen_on_time / 1000;
}

/**
 * @brief   API to clear get_gui_on_time to zero
 *
 * @param   None
 *
 * @return  None
 */
void clear_gui_on_time(void)
{
	ry_gui_ctx_v.screen_on_time = 0;
}

void ry_gui_msg_send(u32_t type, u32_t len, u8_t *data)
{
    ry_sts_t status = RY_SUCC;
    ry_gui_msg_t *p = (ry_gui_msg_t*)ry_malloc(sizeof(ry_gui_msg_t)+len);
    if (NULL == p) {

        LOG_ERROR("[ry_gui_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }   

    p->msgType = type;
    p->len     = len;
    if (data != NULL) {
        ry_memcpy(p->data, data, len);
    } else {
        ry_memset(p->data, 0xff, 4);		
	}

    if (RY_SUCC != (status = ryos_queue_send(ry_gui_msgQ, &p, 4))) {
        LOG_ERROR("[ry_gui_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
        ry_free((void*)p);
    } 
}

void oled_hw_off(void)
{
#if OLED_OFF_MODE_CUT_POWER					
        pcf_func_oled();
        ry_gui_ctx_v.curState = GUI_STATE_OFF;
#else
        lp_func_oled();
        ry_gui_ctx_v.curState = GUI_STATE_LOW_POWER;
#endif
}

/**
 * @brief   Message handler from Scheduler
 *
 * @param   msg - IPC gui message
 *
 * @return  None
 */
void ry_gui_msg_handler(ry_gui_msg_t *msg)
{
    int i, size;

    switch (msg->msgType) {
        
        case IPC_MSG_TYPE_GUI_SCREEN_ON:
        {
            if (get_device_powerstate() == DEV_PWR_STATE_OFF) {
                break;
            }

            ry_gui_ctx_v.screen_on_start = ry_hal_clock_time();

            if (ry_gui_ctx_v.curState == GUI_STATE_OFF || ry_gui_ctx_v.curState == GUI_STATE_IDLE) {
                //LOG_DEBUG("[ry_gui_msg_handler] Screen on.\r\n");
                _start_func_oled(0);
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                ry_gui_ctx_v.curState = GUI_STATE_READY_ON;                
            }
            else if (ry_gui_ctx_v.curState == GUI_STATE_LOW_POWER) {
                //LOG_DEBUG("[ry_gui_msg_handler] Screen Exit low power..\r\n");
                exit_lp_func_oled();
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                ry_gui_ctx_v.curState = GUI_STATE_READY_ON;                
            } 
            else {
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
            }

            touch_exit_low_power();
            touch_baseline_reset();

            DEV_STATISTICS(screen_on_count);

            break;
        }
        
        case IPC_MSG_TYPE_GUI_SCREEN_OFF:
        
            if (is_touch_in_test())
                break;
        
            if (ry_gui_ctx_v.curState == GUI_STATE_ON) {
                if((!charge_cable_is_input())/*||(wms_activity_get_cur_priority() > WM_PRIORITY_L)*/){
                    //LOG_DEBUG("[ry_gui_msg_handler] Screen off.\r\n");
                    ry_hal_rtc_int_disable();
                    ry_led_onoff_with_effect(0, 0);
#if OLED_OFF_MODE_CUT_POWER					
                    pcf_func_oled();
                    ry_gui_ctx_v.curState = GUI_STATE_OFF;
#else
                    lp_func_oled();
                    ry_gui_ctx_v.curState = GUI_STATE_LOW_POWER;
#endif				
                }else{
                    ry_sched_msg_send(IPC_MSG_TYPE_CHARGING_START, 0, NULL);

                    ry_gui_ctx_v.curState = GUI_STATE_OFF;

                }

                touch_enter_low_power();
                ry_gui_ctx_v.curState = GUI_STATE_OFF;
            }
            
            if (ry_gui_ctx_v.screen_on_start != 0){
                ry_gui_ctx_v.screen_on_time += ry_hal_calc_ms(ry_hal_clock_time() - ry_gui_ctx_v.screen_on_start);
                ry_gui_ctx_v.screen_on_start = 0;
            }
            break;

        case IPC_MSG_TYPE_GUI_SET_COLOR:
            if (ry_gui_ctx_v.curState == GUI_STATE_OFF || ry_gui_ctx_v.curState == GUI_STATE_IDLE) {
                //LOG_DEBUG("[ry_gui_msg_handler] Screen on.\r\n");
                _start_func_oled(1);
                ry_gui_ctx_v.curState = GUI_STATE_READY_ON;
                //gui_status = PM_ST_POWER_SmTART;
            } else if (ry_gui_ctx_v.curState == GUI_STATE_LOW_POWER) {
                //LOG_DEBUG("[ry_gui_msg_handler] Screen Exit low power..\r\n");
                exit_lp_func_oled();
                ry_gui_ctx_v.curState = GUI_STATE_READY_ON;
            }
			
            //am_devices_rm67162_displayoff();
            am_devices_rm67162_display_range_set(0, (RM_OLED_WIDTH - 1), 0, (RM_OLED_HEIGHT -1));    
            am_devices_rm67162_fixed_data_888(*(uint32_t*)msg->data, RM_OLED_IMG_24BIT);
            am_devices_rm67162_displayon();
            break;

        case IPC_MSG_TYPE_GUI_SET_PIC:
            break;

        case IPC_MSG_TYPE_GUI_INVALIDATE:

            if (ry_gui_ctx_v.curState == GUI_STATE_READY_ON || ry_gui_ctx_v.curState == GUI_STATE_ON) {
                //LOG_DEBUG("[ry_gui_msg_handler] Display update.\r\n");
                if (*(uint32_t *)msg->data != 0xffffffff) {
                    write_buffer_to_oled((img_pos_t*)&msg->data);
                } else {
                	gdisp_update();
                }
                
                if (ry_gui_ctx_v.curState == GUI_STATE_READY_ON) {
                    
                    uint8_t brightness_max = tms_oled_max_brightness_percent();
                    
                    am_devices_rm67162_displayon();
                    
                    uint8_t led_brightness = ry_calculate_led_brightness(brightness_max);
                    ry_led_onoff_with_effect(1, led_brightness);

                    oled_set_brightness(brightness_max);
                    
                    ry_gui_ctx_v.curState = GUI_STATE_ON;
                }
            }
            
            break;
						
	    case IPC_MSG_TYPE_GUI_SET_PARTIAL_COLOR:
            {
                if (ry_gui_ctx_v.curState == GUI_STATE_OFF || ry_gui_ctx_v.curState == GUI_STATE_IDLE) {
                    //LOG_DEBUG("[ry_gui_msg_handler] Screen on.\r\n");
                    _start_func_oled(0);
                    ry_gui_ctx_v.curState = GUI_STATE_READY_ON;
                    //gui_status = PM_ST_POWER_SmTART;
                } else if (ry_gui_ctx_v.curState == GUI_STATE_LOW_POWER) {
                    //LOG_DEBUG("[ry_gui_msg_handler] Screen Exit low power..\r\n");
                    exit_lp_func_oled();
                    ry_gui_ctx_v.curState = GUI_STATE_READY_ON;
                }

                u8_t startY = msg->data[4];
                u8_t endY = msg->data[5];
                
                am_devices_rm67162_displayoff();
                am_devices_rm67162_display_range_set(0, (RM_OLED_WIDTH - 1), startY, endY);    
                am_devices_rm67162_fixed_data_888(*(uint32_t*)msg->data, 360*(endY - startY+1));
                am_devices_rm67162_displayon();
                
                LOG_DEBUG("ready_fill_rgb2: 0x%06X. sy:%d, ey: %d\n\r", *(uint32_t*)msg->data, startY, endY);
                break;
            }

        case IPC_MSG_TYPE_SYSTEM_MONITOR_GUI:
            {
                monitor_msg_t* mnt_msg = (monitor_msg_t*)msg->data;
                if (mnt_msg->msgType == IPC_MSG_TYPE_SYSTEM_MONITOR_GUI){
                    *(uint32_t *)mnt_msg->dataAddr = 0;
                }
                break;
            }

        case IPC_MSG_TYPE_GUI_SET_BRIGHTNESS: 
            {
                uint8_t brightness = msg->data[0];
                if (brightness <= 100){
                    oled_set_brightness(brightness);

                    // set led brightness
                    uint8_t led_brightness = ry_calculate_led_brightness(brightness);
                    ry_led_onoff_with_effect(1, led_brightness);
                }
                break;
            }

        default:
            break;
    }

    ry_free((u8*)msg);
    return;
}

/**
 * @brief   API to display_startup_logo
 *
 * @param   en - 0: do not display, else: display
 *
 * @return  None
 */
void gui_display_startup_logo(int en)
{    
    ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
    const uint8_t icon_start_x = 0;
    const uint8_t icon_start_y = 120;

    if (en){
        LOG_DEBUG("[gui_display_startup_logo] begin\r\n");
        clear_buffer_black();
    	ret = draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"power_on.bmp",\
                                icon_start_x, icon_start_y, &w, &h, d_justify_center);
        if (ret == RY_SUCC){
            if (ry_gui_ctx_v.curState == GUI_STATE_OFF || ry_gui_ctx_v.curState == GUI_STATE_IDLE) {
                _start_func_oled(1);
            } else if (ry_gui_ctx_v.curState == GUI_STATE_LOW_POWER) {
                exit_lp_func_oled();
            }
            ry_gui_ctx_v.curState = GUI_STATE_READY_ON;
            update_buffer_to_oled();
        }
    }
}

/**
 * @brief   GUI Main thread
 *
 * @param   None
 *
 * @return  None
 */
int gui_thread(void* arg)
{
    ry_sts_t status;
    ry_gui_msg_t *msg = NULL;	
    
    LOG_INFO("[GUI Thread] Enter.\r\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_GUI);
#endif
    
#if (!WIN32_SIM_MK)    
    status = ryos_queue_create(&ry_gui_msgQ, MAX_GUI_MSG_Q_NUM, sizeof(void*));
#endif    

    status = ryos_mutex_create(&fb_mutex);
    if (RY_SUCC != status) {
        RY_ASSERT(0);
    } 

    /* Hardware Init */
	gui_hw_init();
    ry_gui_ctx_v.curState = GUI_STATE_ON;
    /* UGFX Init */
	gfxInit();      // gfxInit routine call gdispclear

	// gui_create_cwindow();
	gui_disp_font_startup();
	
#if 0	
	gui_disp_img_at(0, 0);
	gdispFillString(0, 192, "Ryeex", font_roboto_44.font, White, Black);
	
	gdisp_update();
#endif

	// gui_disp_graph_startup();

#if DBG_POWERON_CONSOLE    
	gwin_console_creat();
#endif

#if (!IMG_INTERNAL_ONLY)
    //img_exflash_to_internal_flash(1);    //load exflash img resources into internal flash, only once
#endif



	// gui_widget_creat();
	
	LOG_DEBUG("[gui_Thread]: gui_thread_entry initial done.\r\n");
	gui_init_done = 1;
	//gui_status = PM_ST_POWER_START;
	//gui_sleep_thresh = 0;


    /* Power down screen as default */
    ry_led_onoff(0);	

#if OLED_OFF_MODE_CUT_POWER	
    pcf_func_oled();	
    ry_gui_ctx_v.curState = GUI_STATE_OFF;
#else	
    lp_func_oled();
	ry_gui_ctx_v.curState = GUI_STATE_LOW_POWER;
#endif	

	gui_status = PM_ST_POWER_START;
    /* Notiify system scheduler */
    ry_sched_msg_send(IPC_MSG_TYPE_GUI_INIT_DONE, 0, NULL);
    
	while(1) {

        status = ryos_queue_recv(ry_gui_msgQ, &msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[gui_thread_entry]: Queue receive error. Status = 0x%x\r\n", status);
        }

        ry_gui_msg_handler(msg);
	}	
}


#endif


#if (!WIN32_SIM_MK)

/**
 * @brief   API to init gsensor module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t gui_init(void)
{
	ryos_threadPara_t para;

    ry_gui_ctx_v.curState = GUI_STATE_IDLE;

	/* Start gui Thread */
    strcpy((char*)para.name, "gui_thread");
    para.stackDepth =  400;
    para.arg		 = NULL; 
    para.tick		 = 20;
    para.priority	 = 6;	
    return ryos_thread_create(&gui_thread3_handle, gui_thread, &para);
}
#endif	//(!WIN32_SIM_MK)

#endif  /* (RY_GUI_ENABLE == TRUE) */
