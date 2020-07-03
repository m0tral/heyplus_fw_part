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
#include "am_devices_cy8c4014.h"
#include <stdlib.h>
#include <sensor_alg.h>
#include <ry_hal_inc.h>
#include <dip.h>
#include <sensor_alg.h>

#include "ry_nfc.h"
#include "touch.h"

#include "am_bsp_gpio.h"
#include "gui_img.h"
#include "window_management_service.h"
#include "timer_management_service.h"
#include "activity_surface.h"
#include "hrm.h"
#include "motar.h"

#define tp_value (g_touch_data.button)
#define UI_LOG_DEBUG   LOG_DEBUG 
extern int g_r_xfer_busy;

typedef enum {
    PG_T0 = 0x00,
    PG_T1,
	PG_T2,
    PG_T3,
    PG_T4,    
    PG_T5,    
    PG_T6,    
    PG_T7,    

    PG_NONE = 0xff,
} ui_page_t;


typedef enum {
  KPG_IDLE,
  KPG_RUN,
}kpg_st_t;

typedef struct
{
    uint8_t pg_desc[20];
    uint8_t pg_op;
    void   (*run)(void);           
}pg_app_t;


typedef enum {
  PG_SETUP,
  WAITING,
  PG_EXEC,
  EXIT,
}pg_op_t;

typedef enum {
  TP_CODE_0 = 0x01,
  TP_CODE_1 = 0x02,
  TP_CODE_2 = 0x04,
  TP_CODE_3 = 0x08,
  TP_CODE_4 = 0x10,
}tp_code_t;

typedef enum {
  TP_CMD_NONE  = 0x00,
  TP_CMD_SEL   = 0x01,
  TP_CMD_LOOP ,
  TP_CODE_NEXT,
  TP_CODE_SLIDE_UP,
  TP_CODE_SLIDE_DOWN,
  TP_CMD_SEL_1,
  TP_CMD_SEL_2,
  TP_CMD_BACK,
}tp_cmd_t;

extern pg_app_t pg_app_array[];
#define menu_num (sizeof(cwin_menu_array) / sizeof(cwin_params_t))
uint8_t tp_last;
uint8_t tp_cmd;
uint8_t tp_continue_time;
static uint8_t menu_active = 0;
static uint8_t menu_active_last = 0xff;

static uint8_t predict_menu_active = 254;
uint8_t predict_prepare_done = 1;

extern u8_t* p_which_fb;
extern u8_t* p_draw_fb;
extern u8_t frame_buffer[];
extern u8_t frame_buffer_1[];

static int untouch_timer_running = 0;

/*
 * @brief touch Thread handle
 */
ryos_thread_t *app_ui_thread_handle;
ryos_thread_t *app_ui_predict_thread_handle;


ry_queue_t *app_ui_msgQ;
ry_queue_t *app_predict_msgQ;


u32_t app_ui_touch_timer_id;

ry_sts_t app_ui_onCreate(void* usrdata);
ry_sts_t app_ui_onDestroy(void);


activity_t app_ui_activity = {
    .name      = "demo",
    .onCreate  = app_ui_onCreate,
    .onDestroy = app_ui_onDestroy,
    .priority = WM_PRIORITY_M,
};



void app_ui_msg_send(u32_t type, u32_t len, u8_t *data);

uint8_t tp_cmd_detect(void)
{
#if 0
    tp_cmd = 0;
    if (tp_value != 0){
        UI_LOG_DEBUG("[tp_event] tp event %x %x\r\n", tp_last, tp_value);
        if (tp_value != tp_last)   {
            UI_LOG_DEBUG("[tp_event] new event %x %x\r\n", tp_last, tp_value);
            if (tp_value & (TP_CODE_0 | TP_CODE_1)){
                tp_cmd = TP_CMD_SEL;
                UI_LOG_DEBUG("[tp_event] get tp cmd select %x %x\r\n", tp_last, tp_value);
            }
            if (((tp_value & TP_CODE_2) & 0)|| (tp_value & TP_CODE_3)){
                tp_cmd = TP_CODE_NEXT;
                UI_LOG_DEBUG("[tp_event] get tp cmd loop %x %x\r\n", tp_last, tp_value);
            }
            else if (tp_value & TP_CODE_4){
                tp_cmd = TP_CODE_NEXT;
                UI_LOG_DEBUG("[tp_event] get tp cmd next %x %x\r\n", tp_last, tp_value);
            }
            tp_continue_time = 0;            
        } 
        tp_last = tp_value;
        tp_value = 0;       //manual clear the value
        if (++tp_continue_time >= 2){
            tp_last = 0;
            tp_continue_time = 0;
            UI_LOG_DEBUG("[tp_event] clear tp_last %x %x\r\n", tp_last, tp_value);
        }
    }
    else{
        tp_last = 0;
    }
#endif

    if (g_touch_data.event == 1) {
        tp_cmd = TP_CODE_SLIDE_DOWN;
    } else if (g_touch_data.event == 2) {
        tp_cmd = TP_CODE_SLIDE_UP;
    } else if ((g_touch_data.event == 3) && (g_touch_data.button<4)) {
        tp_cmd = TP_CMD_SEL_1;
    } else if ((g_touch_data.event == 3) && (g_touch_data.button>4)) {
        tp_cmd = TP_CMD_SEL_2;
    } else {
        tp_cmd = 0;
    }

    
    

    LOG_DEBUG("tp cmd: %d\r\n", tp_cmd);
    
    return tp_cmd;
}

static pg_op_t t0_op = PG_SETUP;

#if 0
void t0_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    int menu_2 = 0;

    #if 0
    if (gui_status == PM_ST_POWER_START){
        gui_status = PM_ST_IDLE;
        t0_op = PG_SETUP;
        UI_LOG_DEBUG("[gui_Thread]: t0_run re-started %d\r\n", t0_op);			
    }
    else if (gui_status == PM_ST_POWER_RUNNING){
        t0_op = PG_EXEC;
        menu_active_last = 0xff;    //reset menu active 
    }
#endif
    
    do{
        switch(t0_op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t0_run] setup %d\r\n", cnt++);
                // led_set_working(1, 80, 9000);
                // _cwin_update(&cwin_menu_array[menu_active]);
                //if (tp_cmd != TP_CMD_NONE) {
                    t0_op = WAITING;
                    exit_st = 1;
                //}
                break;

            case WAITING:               
                UI_LOG_DEBUG("[t0_run] waiting %d, tp_cmd:%d\r\n", menu_active, tp_cmd);
                {
                    uint8_t time[16];
                    sprintf(time, "%02d:%02d:%02d", \
                        hal_time.ui32Hour, hal_time.ui32Minute, hal_time.ui32Second);

                    uint8_t date[16];
                    sprintf(date, "%02d-%02d-%02d", \
                            hal_time.ui32Year, hal_time.ui32Month, hal_time.ui32DayOfMonth);
                    cwin_notify_array[CNOTIFY_RTC].info[0] = time;   

                    uint8_t battery[16];
                    sprintf(battery, "bat:%d%", sys_get_battery());
                    cwin_notify_array[CNOTIFY_RTC].info[1] = battery;   

                    uint8_t step[16];
                    sprintf(step, "%d", alg_get_step_today());
                    cwin_notify_array[CNOTIFY_RTC].info[2] = step;    

                    if (g_r_xfer_busy == 0) {
                        UI_LOG_DEBUG("[t0_run] waiting %d, tp_cmd:%d time: %s\r\n", menu_active, tp_cmd, time);
                        _cnotify_update(&cwin_notify_array[CNOTIFY_RTC]);
                    }                            
                }
                
                
                if ((tp_cmd == TP_CODE_SLIDE_DOWN) || (tp_cmd == TP_CODE_SLIDE_UP)) { 
                    t0_op = PG_EXEC;
                    //tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                    menu_active_last = 0xff;    //reset menu active 
                    gui_status = PM_ST_POWER_RUNNING;
                } else {
                    exit_st = 0;
                    ry_timer_stop(app_ui_touch_timer_id);
                    //auto_reset_start();
                }
                
                break;
            
            case PG_EXEC:
                UI_LOG_DEBUG("[t0_run] execute... %d tp_cmd:%d\r\n", menu_active,tp_cmd);
                int _startTime = ry_hal_clock_time();
                exit_st = 0;   

                if (tp_cmd == TP_CMD_NONE) {
                    exit_st = 0;
                    //break;
                }
                
                //if (tp_cmd == TP_CODE_NEXT) { 
                if (tp_cmd == TP_CODE_SLIDE_UP) {
                    gui_event_time = ryos_get_ms();
                    tp_cmd = TP_CMD_NONE;   
                    if (++menu_active >= menu_num) {
                        menu_active = 0;
                        //op = WAITING;                        
                        exit_st = 0;      
                        //gui_status = PM_ST_POWER_START;
                    }
                }
                if (tp_cmd == TP_CODE_SLIDE_DOWN) {
                    gui_event_time = ryos_get_ms();
                    tp_cmd = TP_CMD_NONE;   
                    if (--menu_active >= menu_num) {
                        menu_active = menu_num-1;
                        //op = WAITING;                        
                        exit_st = 0;      
                        //gui_status = PM_ST_POWER_START;
                    }
                }
                
                if (tp_cmd == TP_CMD_SEL_1) { 
                    t0_op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                    continue;
                }

                if (tp_cmd == TP_CMD_SEL_2) {
                    menu_2 = 1;
                    if (PG_T1 + menu_active + menu_2 > 7) {
                        exit_st = 0;
                    } else {
                        t0_op = EXIT;
                        tp_cmd = TP_CMD_NONE;
                        exit_st = 1;
                        continue;
                    }
                }

                if (tp_cmd == TP_CMD_BACK) {
                    t0_op = WAITING;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                    continue;
                }


                if ((menu_active != menu_active_last) && (gui_status != PM_ST_POWER_START)){

#if ((RY_UI_EFFECT_MOVE == TRUE)||(RY_UI_EFFECT_TAP_MOVE == TRUE))
                    LOG_DEBUG("t0 exe, p: %d, m: %d\r\n", predict_menu_active, menu_active);
                    if ((predict_menu_active == menu_active) &&
                        (predict_prepare_done)) {

                        LOG_DEBUG("Predict correct and done.\r\n");
                        /* Predice correct and framebuffer has prepared, let it go. */
                        //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        for (int i= 0; i < 4; i++) {
                            p_draw_fb = frame_buffer + (i+1) * 21600;
                            LOG_DEBUG("fb addr: 0x%x\r\n", p_draw_fb);
                            am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
                            am_hal_gpio_out_bit_set(23);
                            gdisp_update();
                        }
                        predict_prepare_done = 0;
                        predict_menu_active = 0xFE;
                        
                    } else if ((predict_menu_active == menu_active) &&
                        (predict_prepare_done == 0)) {

                        LOG_DEBUG("Predict correct but wait until done.\r\n");

                        /* Predice correct but framebuffer has not been prepared, wait. */
                        while (predict_prepare_done != 1) {
                            ryos_delay_ms(2);
                        }

                        //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        for (int i= 0; i < 4; i++) {
                            p_draw_fb = frame_buffer + (i+1) * 21600;
                            LOG_DEBUG("fb addr: 0x%x\r\n", p_draw_fb);
                            am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
                            am_hal_gpio_out_bit_set(23);
                            gdisp_update();
                        }
                        predict_prepare_done = 0;
                        predict_menu_active = 0xFE;
                    } else if (predict_menu_active != menu_active) {
                        if ((predict_prepare_done == 0) && (tp_cmd != TP_CMD_NONE)) {
                            /* Predice correct but framebuffer has not been prepared, wait. */
                            LOG_DEBUG("Predict incorrect and prepare not done, wait.\r\n");
                            while (predict_prepare_done != 1) {
                                ryos_delay_ms(2);
                            }
                        }
                        
                        LOG_DEBUG("Predict incorrect.p:%d, m:%d\r\n", predict_menu_active, menu_active);
                        am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
                        am_hal_gpio_out_bit_set(23);
                        _cwin_update(&cwin_menu_array[menu_active]);
                        
                    }
#else
                    am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
                    am_hal_gpio_out_bit_set(23);
                    _cwin_update(&cwin_menu_array[menu_active]);
#endif
                    UI_LOG_DEBUG("Refresh Img: %d ms\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startTime));
                    menu_active_last = menu_active;
                }
                break;

            case EXIT:
                UI_LOG_DEBUG("[t0_run] exit to t1 %d\r\n", cnt++);
                pg_app_array[PG_T0].pg_op = KPG_IDLE;
                LOG_DEBUG("Goto another page, %d, %d\r\n", menu_active, menu_2);
                pg_app_array[PG_T1 + menu_active + menu_2].pg_op = KPG_RUN;
                t0_op = PG_SETUP;
                exit_st = 0;
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                // pcf_func_touch();
                // _start_func_touch();      
                // oled_fill_color(COLOR_RAW_RED, 1000);
                break;
            
            default:
                break;
        }
    } while(exit_st);
}

void t1_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    static uint8_t reset_time;  
    ui_page_t which_page;
    
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t1_run] setup %d\r\n", cnt++);                 
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t temp_buf[16];
                    uint8_t mac[6];                             
                    uint8_t ret_len;
                    extern uint8_t* device_info_get_mac(void);
                    device_info_get(DEV_INFO_TYPE_MAC, mac, &ret_len);
                    sprintf(temp_buf, "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
                    cinfo_array[CINFO_DEV].info[1] = temp_buf;   
                    cinfo_array[CINFO_DEV].info[0] = "Info";   
                    
                    // LOG_DEBUG("[info] mac %s\r\n", temp_buf);
                    uint8_t did[20];
                    
                    device_info_get(DEV_INFO_TYPE_DID, did, &ret_len);
                    char did_dest[20] = {""};
                    strncpy(did_dest, did + 3, ret_len - 3);
                    cinfo_array[CINFO_DEV].info[2] = did_dest; 
                    // LOG_DEBUG("[info] did_buf %s\r\n", did);

                    uint8_t version[20];       
                    device_info_get(DEV_INFO_TYPE_VER, version, &ret_len);
                    cinfo_array[CINFO_DEV].info[3] = version;   
                    // LOG_DEBUG("[info] sn_buf %s\r\n", sn_buf);

                    _cinfo_update(&cinfo_array[CINFO_DEV]);
                    gui_sleep_thresh = 100 * 1000;              
                }
                UI_LOG_DEBUG("[t1_run] waiting, tpcmd: %d\r\n", tp_cmd);
                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    reset_time = 0;     
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                    // motar_strong_linear_working(500);
                }

                if (tp_cmd == TP_CMD_BACK) {
                    op = EXIT;
                    which_page = PG_T0;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                
                break;
            
            case PG_EXEC:
                {
                    cinfo_array[CINFO_DEV].info[1] = "30s press reset";   
                    if (tp_cmd == TP_CMD_SEL_1) {
                        reset_time ++;
                        tp_cmd = TP_CMD_NONE;         
                        if (reset_time > 10){
                            //am_hal_reset_poi();
                            power_off_ctrl(1);
                        }               
                    }
                    uint8_t buff[8];
                    sprintf(buff, "%ds", reset_time);
                    cinfo_array[CINFO_DEV].info[2] = buff;   
                    cinfo_array[CINFO_DEV].info[0] = "";   
                    
                    _cinfo_update(&cinfo_array[CINFO_DEV]);
                    gui_event_time = ryos_get_ms();
                }
                LOG_DEBUG("[t1_run] execute... %d\r\n", cnt++);
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    which_page = PG_T0;
                    t0_op = WAITING;
                    exit_st = 1;
                }             
                break;

            case EXIT:
                UI_LOG_DEBUG("[t1_run] exit to page: %d\r\n", which_page);
                pg_app_array[PG_T1].pg_op = KPG_IDLE;
                pg_app_array[which_page].pg_op = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                //oled_fill_color(COLOR_RAW_GREEN, 100);
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}


void t2_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t2_run] setup %d\r\n", cnt++);
                cwin_module_array[CMODULE_HRM].info[0] = " - ";
                cwin_module_array[CMODULE_HRM].info[1] = "keep still";
                
                _cmodule_update(&cwin_module_array[CMODULE_HRM]);                
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t *info = {"t2_Wait"};
                    // disp_icon_img(3, info, Green);
                    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);
                    // pcf_func_gsensor();
                }
                UI_LOG_DEBUG("[t2_run] waiting %d\r\n", cnt++);
                

                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                break;
            
            case PG_EXEC:
                {
                    // gui_widget_demo();
                    uint8_t *info = {"t2_Exc"};
                    // gui_disp_string(info, Green);
                    _start_func_hrm(HRM_START_AUTO);
                    set_hrm_working_mode(HRM_MODE_MANUAL);
                }
                cwin_module_array[CMODULE_HRM].info[1] = "hrm";
                
                uint8_t hr_cnt = 0;
                if (alg_get_hrm_cnt(&hr_cnt) != -1){
                    uint8_t value[8];
                    sprintf(value, "%d", hr_cnt);
                    cwin_module_array[CMODULE_HRM].info[0] = value;    
                }
                
                _cmodule_update(&cwin_module_array[CMODULE_HRM]);                
                gui_event_time = ryos_get_ms();
                
                UI_LOG_DEBUG("[t2_run] execute... %d\r\n", cnt++);
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    t0_op = WAITING;
                    exit_st = 1;
                }
                break;

            case EXIT:
                UI_LOG_DEBUG("[t2_run] exit to t3 %d\r\n", cnt++);
                pg_app_array[PG_T2].pg_op = KPG_IDLE;
                pg_app_array[PG_T0].pg_op = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                // oled_fill_color(COLOR_RAW_BLUE, 1000);     
                pcf_func_hrm();  
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}

void t3_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t3_run] setup %d\r\n", cnt++);
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t *info = {"t3_Wait"};
                    // disp_icon_img(4, info, Navy);
                    uint8_t value[8];
                    sprintf(value, "%d", alg_get_step_today());
                    cwin_module_array[CMODULE_SPORT].info[0] = value;    
                    cwin_module_array[CMODULE_SPORT].info[1] = "step";
                    _cmodule_update(&cwin_module_array[CMODULE_SPORT]);     
                    
                }
                UI_LOG_DEBUG("[t3_run] waiting %d\r\n", cnt++);

                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                break;
            
            case PG_EXEC:
                {
                    // gui_widget_demo();
                    uint8_t *info = {"t3_Exc"};
                    // gui_disp_string(info, Navy);
                    uint8_t value[8];
                    sprintf(value, "%d", alg_get_step_today());
                    cwin_module_array[CMODULE_SPORT].info[0] = value;    
                    cwin_module_array[CMODULE_SPORT].info[1] = "come on";
                    _cmodule_update(&cwin_module_array[CMODULE_SPORT]); 
                    gui_event_time = ryos_get_ms();
                }
                UI_LOG_DEBUG("[t3_run] execute... %d\r\n", cnt++);
                 
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    t0_op = WAITING;
                    exit_st = 1;
                }             
                break;

            case EXIT:
                UI_LOG_DEBUG("[t3_run] exit to t0 %d\r\n", cnt++);
                pg_app_array[PG_T3].pg_op = KPG_IDLE;
                pg_app_array[PG_T0].pg_op  = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}


void t4_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    int nfc_cnt;
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t4_run] setup %d\r\n", cnt++);
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t *info = {"t4_Wait"};
                    uint8_t value[8];

                    ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_ON, NULL);
                    
                    while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
                        ryos_delay_ms(100);
                        if (nfc_cnt ++ == 60) {
                            break;
                        }
                    }
                    if (nfc_cnt >= 60) {
                        cwin_module_array[CMODULE_CARD].info[0] = "nfc";
                        cwin_module_array[CMODULE_CARD].info[1] = "fail";
                    } else {
                        cwin_module_array[CMODULE_CARD].info[0] = "nfc";
                        cwin_module_array[CMODULE_CARD].info[1] = "ready";
                        ry_timer_stop(app_ui_touch_timer_id);
                    }
                    _cmodule_update(&cwin_module_array[CMODULE_CARD]);     
                }
                UI_LOG_DEBUG("[t4_run] waiting %d, tp_cmd:%d\r\n", cnt++, tp_cmd);
                gui_event_time = ryos_get_ms();

                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                break;
            
            case PG_EXEC:
                {
                    //ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_OFF, NULL);
                    
                    //while(ry_nfc_get_state() != RY_NFC_STATE_INITIALIZED) {
                    //    ryos_delay_ms(100);
                    //    if (nfc_cnt ++ == 50) {
                    //        break;
                    //    }
                    //}
                    
                    if (nfc_cnt < 60) {
                        cwin_module_array[CMODULE_CARD].info[0] = "nfc";
                        cwin_module_array[CMODULE_CARD].info[1] = "ce on";
                    }
                    else {
                        cwin_module_array[CMODULE_CARD].info[0] = "ce on";
                        cwin_module_array[CMODULE_CARD].info[1] = "fail";
                    }
                        
                    cwin_module_array[CMODULE_CARD].info[0] = "";    
                    _cmodule_update(&cwin_module_array[CMODULE_CARD]);  
                    gui_event_time = ryos_get_ms();
                    
                }
                UI_LOG_DEBUG("[t4_run] execute... %d\r\n", cnt++);
                 
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    t0_op = WAITING;
                    exit_st = 1;
                }             
                break;

            case EXIT:
                UI_LOG_DEBUG("[t4_run] exit to t0 %d\r\n", cnt++);

                ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_ON, NULL);
                    
                while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
                    ryos_delay_ms(100);
                    if (nfc_cnt ++ == 50) {
                        break;
                    }
                }
                pg_app_array[PG_T4].pg_op = KPG_IDLE;
                pg_app_array[PG_T0].pg_op  = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}


void t5_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t5_run] setup %d\r\n", cnt++);
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t *info = {"t5_Wait"};
                    uint8_t value[8];
                    cwin_module_array[CMODULE_UPGRAD].info[0] = "upgrade";
                    cwin_module_array[CMODULE_UPGRAD].info[1] = "ready";
                    cwin_module_array[CMODULE_UPGRAD].image[0] = "m_upgrade_01_doing.png";
                    
                    _cmodule_update(&cwin_module_array[CMODULE_UPGRAD]);     
                }
                UI_LOG_DEBUG("[t5_run] waiting %d\r\n", cnt++);

                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                break;
            
            case PG_EXEC:
                {
                    cwin_module_array[CMODULE_UPGRAD].info[1] = "waiting";
                    cwin_module_array[CMODULE_UPGRAD].info[0] = "";    
                    cwin_module_array[CMODULE_UPGRAD].image[0] = "m_upgrade_01_doing.png";
                    _cmodule_update(&cwin_module_array[CMODULE_UPGRAD]);  
                    gui_event_time = ryos_get_ms();
                    
                }
                UI_LOG_DEBUG("[t5_run] execute... %d\r\n", cnt++);
                 
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    t0_op = WAITING;
                    exit_st = 1;
                }             
                break;

            case EXIT:
                UI_LOG_DEBUG("[t5_run] exit to t0 %d\r\n", cnt++);
                pg_app_array[PG_T5].pg_op = KPG_IDLE;
                pg_app_array[PG_T0].pg_op  = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}

void t6_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t6_run] setup %d\r\n", cnt++);
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t value[8];
                    cwin_module_array[CMODULE_WEATER].info[0] = "sunning";
                    cwin_module_array[CMODULE_WEATER].info[1] = "-10 to 5";
                    cwin_module_array[CMODULE_WEATER].image[0] = "m_weather_00_sunny.png";
                    
                    _cmodule_update(&cwin_module_array[CMODULE_WEATER]);     
                }
                UI_LOG_DEBUG("[t6_run] waiting %d\r\n", cnt++);

                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                break;
            
            case PG_EXEC:
                {
                    cwin_module_array[CMODULE_WEATER].info[0] = "";
                    cwin_module_array[CMODULE_WEATER].info[1] = "1 - 7 day";    
                    cwin_module_array[CMODULE_WEATER].image[0] = "m_weather_01_rainning.png";
                    _cmodule_update(&cwin_module_array[CMODULE_WEATER]);  
                }
                UI_LOG_DEBUG("[t6_run] execute... %d\r\n", cnt++);
                 
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    t0_op = WAITING;
                    exit_st = 1;
                }             
                break;

            case EXIT:
                UI_LOG_DEBUG("[t6_run] exit to t0 %d\r\n", cnt++);
                pg_app_array[PG_T6].pg_op = KPG_IDLE;
                pg_app_array[PG_T0].pg_op  = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}


void t7_run(void)
{
    uint32_t exit_st = 1;
    static pg_op_t op = PG_SETUP;
    static int cnt;
    do{
        switch(op)
        {
            case PG_SETUP:
                UI_LOG_DEBUG("[t7_run] setup %d\r\n", cnt++);
                op = WAITING;
                break;

            case WAITING:
                {
                    uint8_t value[8];
                    // cwin_module_array[CMODULE_DATA].info[0] = "step";
                    // cwin_module_array[CMODULE_DATA].info[1] = "9999";
                    // cwin_module_array[CMODULE_DATA].image[0] = "m_data_00_step.png";

                    cwin_module_array[CMODULE_DATA].info[0] = "calorie";
                    cwin_module_array[CMODULE_DATA].info[1] = "14512";
                    cwin_module_array[CMODULE_DATA].image[0] = "m_data_01_calorie.png";

                    _cmodule_update(&cwin_module_array[CMODULE_DATA]);     
                }
                UI_LOG_DEBUG("[t7_run] waiting %d\r\n", cnt++);

                exit_st = 0;
                if (tp_cmd == TP_CMD_SEL_1) { 
                    op = PG_EXEC;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    exit_st = 1;
                }
                break;
            
            case PG_EXEC:
                {
                    cwin_module_array[CMODULE_DATA].info[0] = "sleep time";
                    cwin_module_array[CMODULE_DATA].info[1] = "7.25 hour";
                    cwin_module_array[CMODULE_DATA].image[0] = "m_data_05_sleeping.png";                    
                    _cmodule_update(&cwin_module_array[CMODULE_DATA]);  
                }
                UI_LOG_DEBUG("[t7_run] execute... %d\r\n", cnt++);
                 
                exit_st = 0;   
                if (tp_cmd == TP_CMD_BACK) { 
                    op = EXIT;
                    tp_cmd = TP_CMD_NONE;
                    t0_op = WAITING;
                    exit_st = 1;
                }             
                break;

            case EXIT:
                UI_LOG_DEBUG("[t7_run] exit to t0 %d\r\n", cnt++);
                pg_app_array[PG_T7].pg_op = KPG_IDLE;
                pg_app_array[PG_T0].pg_op  = KPG_RUN;
                op = PG_SETUP;
                exit_st = 0;
                app_ui_msg_send(IPC_MSG_TYPE_APP_UI_NEXT_PAGE, 0, NULL);
                break;
            
            default:
                break;
        }
    }while(exit_st);
}
#else
void t0_run(void)
{

}

void t1_run(void)
{

}

void t2_run(void)
{

}

void t3_run(void)
{

}

void t4_run(void)
{

}

void t5_run(void)
{

}

void t6_run(void)
{

}

void t7_run(void)
{

}

#endif


pg_app_t pg_app_array[] = {
    {"t0",  KPG_RUN,    t0_run },    
    {"t1",  KPG_IDLE,   t1_run },    
    {"t2",  KPG_IDLE,   t2_run }, 
    {"t3",  KPG_IDLE,   t3_run },     
    {"t4",  KPG_IDLE,   t4_run },        
    {"t5",  KPG_IDLE,   t5_run },   
    {"t6",  KPG_IDLE,   t6_run }, 
    {"t7",  KPG_IDLE,   t7_run },         
            
};

void k_page_execute(void) 
{
    uint8_t i;
    int len = (sizeof(pg_app_array) / sizeof(pg_app_t));
    for (i = 0; i < len; i++){
        if (pg_app_array[i].pg_op != KPG_IDLE){
            if (pg_app_array[i].run != NULL){
                pg_app_array[i].run();
            }
        }
    }
}


typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t  data[1];
} app_ui_msg_t;


void app_ui_msg_send(u32_t type, u32_t len, u8_t *data)
{
    ry_sts_t status = RY_SUCC;
    app_ui_msg_t *p = (app_ui_msg_t*)ry_malloc(sizeof(app_ui_msg_t)+len);
    if (NULL == p) {

        LOG_ERROR("[app_ui_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }   

    p->msgType = type;
    p->len     = len;
    if (data) {
        ry_memcpy(p->data, data, len);
    }

    if (RY_SUCC != (status = ryos_queue_send(app_ui_msgQ, &p, 4))) {
        LOG_ERROR("[app_ui_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
    } 
}

void app_ui_predict_msg_send(u32_t type, u32_t len, u8_t *data)
{
    ry_sts_t status = RY_SUCC;
    app_ui_msg_t *p = (app_ui_msg_t*)ry_malloc(sizeof(app_ui_msg_t)+len);
    if (NULL == p) {

        LOG_ERROR("[app_ui_predict_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }   

    p->msgType = type;
    p->len     = len;
    if (data) {
        ry_memcpy(p->data, data, len);
    }

    if (RY_SUCC != (status = ryos_queue_send(app_predict_msgQ, &p, 4))) {
        LOG_ERROR("[app_ui_predict_msg_send]: Add to Queue fail. status = 0x%04x\n", status);
    } 
}





void app_ui_untouch_timeout_handler(void* arg)
{
    ry_sts_t status = RY_SUCC;
    
    LOG_DEBUG("[app_ui_untouch_timeout_handler]: transfer timeout.\n");

    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);

    app_ui_msg_send(IPC_MSG_TYPE_APP_UI_PREPARE_DEFAULT_UI, 0, NULL);

    ry_timer_stop(app_ui_touch_timer_id);

    untouch_timer_running = 0;

    am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_out_bit_clear(23);

    t0_op = WAITING;
    pg_app_array[PG_T0].pg_op  = KPG_RUN;
    pg_app_array[PG_T1].pg_op  = KPG_IDLE;
    pg_app_array[PG_T2].pg_op  = KPG_IDLE;
    pg_app_array[PG_T3].pg_op  = KPG_IDLE;
    pg_app_array[PG_T4].pg_op  = KPG_IDLE;
    pg_app_array[PG_T5].pg_op  = KPG_IDLE;
    pg_app_array[PG_T6].pg_op  = KPG_IDLE;
    pg_app_array[PG_T7].pg_op  = KPG_IDLE;

    //wms_activity_jump(&activity_surface, NULL);

    
    
}

void app_ui_idle_timeout_handler(void* arg)
{
    ry_timer_start(app_ui_touch_timer_id, 5000, app_ui_untouch_timeout_handler, NULL);
}



int tp_sub_state = 0;
void app_ui_tp_msg_sub(int fSubscribe)
{
    if (fSubscribe) {
        tp_sub_state = 1;
        //LOG_DEBUG("sub tp enable\r\n");
    } else {
        tp_sub_state = 0;
        //LOG_DEBUG("sub tp disable\r\n");
    }
}




int app_ui_onRTCEvent(ry_ipc_msg_t* msg)
{
    //LOG_DEBUG("[app_ui_onRTCEvent]\r\n");
    app_ui_msg_send(IPC_MSG_TYPE_APP_UI_REFRESH_TIME, 0, NULL);
    return 0;
}

int app_ui_onALGEvent(ry_ipc_msg_t* msg)
{
    //u16_t point;
    //gui_evt_msg_t gui_msg;
    //gui_evt_msg_t *p_msg = &gui_msg;
    //gui_msg.evt_type = GUI_EVT_TP;
    //g_touch_data.button = point;
    //g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_DOWN;
    //gui_msg.pdata = (void*)&g_touch_data;
    //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
    //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
    //}

    //if (tp_sub_state) {
    //    app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
    //}

#if 1
    wrist_filt_t* p = (wrist_filt_t*)msg->data;

    if (p->type == ALG_WRIST_FILT_UP) {
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

        if (app_ui_touch_timer_id) {
            ry_timer_start(app_ui_touch_timer_id, 5000, app_ui_idle_timeout_handler, NULL);
            untouch_timer_running = 1;
            LOG_DEBUG("Untouch timer start. evt:0x%x\r\n", msg->msgType);
            am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
            am_hal_gpio_out_bit_set(23);
        }
        
    } else if (p->type == ALG_WRIST_FILT_DOWN) {
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
        am_hal_gpio_pin_config(23, AM_HAL_PIN_OUTPUT);
        am_hal_gpio_out_bit_clear(23);
    }
#endif
    return 0;
}


int app_ui_onTouchProcessedEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    gui_evt_msg_t gui_msg;
    gui_evt_msg_t *p_msg = &gui_msg;
	ry_sts_t status;
    
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;

    LOG_DEBUG("[app_ui_onTouchProcessedEvent] action:%d, point:%d\r\n", p->action, p->y);

    switch (p->action) {
        case TP_PROCESSED_ACTION_CLICK:
            gui_msg.evt_type = GUI_EVT_TP;
            g_touch_data.button = p->y;
            g_touch_data.event = TP_PROCESSED_ACTION_CLICK;
            gui_msg.pdata = (void*)&g_touch_data;
            //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
            //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
            //}
            if (tp_sub_state) {
                app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            gui_msg.evt_type = GUI_EVT_TP;
            g_touch_data.button = p->y;
            g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_DOWN;
            gui_msg.pdata = (void*)&g_touch_data;
            //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
            //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
            //}
            if (tp_sub_state) {
                app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            gui_msg.evt_type = GUI_EVT_TP;
            g_touch_data.button = p->y;
            g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_UP;
            gui_msg.pdata = (void*)&g_touch_data;
            //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
            //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
            //}
            if (tp_sub_state) {
                app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
            }
            break;
    }
    
    return consumed;
}


int app_ui_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;

#if 1
    static u32_t startT;
    static u16_t startY;
    static int   moveCnt = 0;
    static int   direction = 0;
    static u16_t lastY;
    static u8_t  predict_once = 0;
    u16_t point;
    u32_t deltaT;
    tp_event_t *p = (tp_event_t*)msg->data;

    gui_evt_msg_t gui_msg;
    gui_evt_msg_t *p_msg = &gui_msg;
	ry_sts_t status;
    int done = 0;
    

    switch (p->action) {
        case TP_ACTION_DOWN:
            LOG_DEBUG("TP Action Down, y:%d, t:%d\r\n", p->y, p->t);
            startT = p->t;
            startY = p->y;
            moveCnt = 0;
            lastY = p->y;
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
            break;

        case TP_ACTION_MOVE:
            LOG_DEBUG("TP Action Move, y:%d, t:%d\r\n", p->y, p->t);
            moveCnt++;


            if (p->y > lastY) {
                direction++;
                
#if ((RY_UI_EFFECT_MOVE == TRUE)||(RY_UI_EFFECT_TAP_MOVE == TRUE))
                if (predict_once == 0) {
                    predict_once = 1;
                    g_touch_data.button = point;
                    g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_DOWN;
                    app_ui_predict_msg_send(IPC_MSG_TYPE_TOUCH_PREDICT_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    LOG_DEBUG("Move sent, dn\r\n");
                }
#endif 
            }

            if (p->y < lastY) {
                direction--;
                
#if ((RY_UI_EFFECT_MOVE == TRUE)||(RY_UI_EFFECT_TAP_MOVE == TRUE))
                if (predict_once == 0) {
                    predict_once = 1;
                    g_touch_data.button = point;
                    g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_UP;
                    app_ui_predict_msg_send(IPC_MSG_TYPE_TOUCH_PREDICT_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    LOG_DEBUG("Move sent, up\r\n");
                }
#endif 
            }

            

#if (RY_UI_EFFECT_TAP_MOVE == TRUE)
            LOG_DEBUG("mv cnt:%d, lastY:%d, curY:%d\r\n", moveCnt, lastY, p->y);
            if (moveCnt >= 2) {
                if ((lastY >= 4) && (p->y < 4)) {
                    // move up
                    while (predict_prepare_done != 1) {
                        ryos_delay_ms(2);
                    }

                    for (int i= 0; i < 4; i++) {
                        p_draw_fb = frame_buffer + (i+1) * 21600;
                        LOG_DEBUG("fb addr: 0x%x\r\n", p_draw_fb);
                        gdisp_update();
                    }
                    
                } else if ((lastY <= 4) && (p->y > 4)) {
                    // move down
                    while (predict_prepare_done != 1) {
                        ryos_delay_ms(2);
                    }

                    for (int i= 0; i < 4; i++) {
                        p_draw_fb = frame_buffer_1 - ((i+1) * 21600);
                        LOG_DEBUG("fb addr: 0x%x\r\n", p_draw_fb);
                        gdisp_update();
                    }
                }
            }
#endif //(RY_UI_EFFECT_TAP_MOVE)


            lastY = p->y;
            
            break;

        case TP_ACTION_UP:
            LOG_DEBUG("TP Action Up, y:%d, t:%d\r\n", p->y, p->t);
            deltaT = ry_hal_calc_ms(p->t-startT);
            LOG_DEBUG("TP End. Duration: %d ms\r\n", deltaT);

            if ((p->y == startY) && (moveCnt < 3)) {
                LOG_DEBUG("-->Click. point:%d\r\n", startY);

#if 1
                if (deltaT < 150) {
                    if (startY < 4) {
                        gui_msg.evt_type = GUI_EVT_TP;
                        g_touch_data.button = point;
                        g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_DOWN;
                        gui_msg.pdata = (void*)&g_touch_data;
                        //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
                        //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
                        //}

                        if (tp_sub_state) {
                            app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                        }
                    } else if (startY > 4) {
                        gui_msg.evt_type = GUI_EVT_TP;
                        g_touch_data.button = point;
                        g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_UP;
                        gui_msg.pdata = (void*)&g_touch_data;
                        //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
                        //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
                        //}
                        if (tp_sub_state) {
                            app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                        }
                    }
                    done = 1;
                }
#endif

                if ((startY == 9) && (deltaT>100)) {
                    gui_msg.evt_type      = GUI_EVT_TP;
                    g_touch_data.button   = startY;
                    g_touch_data.event    = TP_PROCESSED_ACTION_CLICK;
                    gui_msg.pdata = (void*)&g_touch_data;

                    if (tp_sub_state) {
                        app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    }
                    done = 1;
                } else if (deltaT>200) {
                    gui_msg.evt_type      = GUI_EVT_TP;
                    g_touch_data.button   = startY;
                    g_touch_data.event    = TP_PROCESSED_ACTION_CLICK;
                    gui_msg.pdata = (void*)&g_touch_data;

                    if (tp_sub_state) {
                        app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    }
                    done = 1;
                } 
                
                
            } else if ((ABS(p->y - startY)<2) && (moveCnt<2)) {
                if (startY == 4) {
                    point = p->y;
                }

                if (MAX(startY,p->y) > 4) {
                    point = MAX(startY,p->y);
                } 

                if (MIN(startY,p->y) < 4) {
                    point = MIN(startY,p->y);
                } 
                LOG_DEBUG("-->Click. point:%d\r\n", point);

                if ((startY == 9) && (deltaT>100)) {
                    gui_msg.evt_type      = GUI_EVT_TP;
                    g_touch_data.button   = startY;
                    g_touch_data.event    = TP_PROCESSED_ACTION_CLICK;
                    gui_msg.pdata = (void*)&g_touch_data;

                    if (tp_sub_state) {
                        app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    }
                    done = 1;
                } else if (deltaT>150) {
                    gui_msg.evt_type      = GUI_EVT_TP;
                    g_touch_data.button   = startY;
                    g_touch_data.event    = TP_PROCESSED_ACTION_CLICK;
                    gui_msg.pdata = (void*)&g_touch_data;

                    if (tp_sub_state) {
                        app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    }
                    done = 1;
                } 
            }

            if (done == 0) {
                if (direction >= 1) {
                    LOG_DEBUG("-->SLIDE DOWN. d:%d\r\n", direction);

                    gui_msg.evt_type = GUI_EVT_TP;
                    g_touch_data.button = point;
                    g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_DOWN;
                    gui_msg.pdata = (void*)&g_touch_data;
                    //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
                    //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
                    //}

                    if (tp_sub_state) {
                        app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    }
                        
                } else if (direction <= -1) {
                    LOG_DEBUG("-->SLIDE UP. d:%d\r\n", direction);

                    gui_msg.evt_type = GUI_EVT_TP;
                    g_touch_data.button = point;
                    g_touch_data.event = TP_PROCESSED_ACTION_SLIDE_UP;
                    gui_msg.pdata = (void*)&g_touch_data;
                    //if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
                    //    LOG_ERROR("[touch_thread]: Add to Queue fail. status = 0x%04x\n", status);
                    //}
                    if (tp_sub_state) {
                        app_ui_msg_send(IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT, sizeof(g_touch_data), (u8_t*)(&g_touch_data));
                    }
                }
            }


            predict_once = 0;
            moveCnt = 0;
            direction = 0;
            break;

        default:
            break;
    }

#endif
    return consumed; 
}



/**
 * @brief   app_ui thread
 *
 * @param   None
 *
 * @return  None
 */
int app_ui_thread(void* arg)
{
#if 0    
    ry_sts_t status = RY_SUCC;
    ry_timer_parm timer_para;
    app_ui_msg_t *msg = NULL;	
    app_ui_msg_t *tmpMsg = NULL;
    app_ui_msg_t *lastMsg = NULL;
    u32_t msg_addr;

    LOG_DEBUG("[app_ui_thread] Enter.\n");

    /* Create the RYNFC message queue */
    status = ryos_queue_create(&app_ui_msgQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[app_ui_thread]: RX Queue Create Error.\r\n");
        while(1);
    }

    /* Create the timer */
    timer_para.timeout_CB = app_ui_untouch_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "app_ui_timer";
    app_ui_touch_timer_id = ry_timer_create(&timer_para);

    app_ui_tp_msg_sub(1);

    //wm_addTouchListener(app_ui_onTouchEvent);
    //ry_sched_addRTCEvtListener(app_ui_onRTCEvent);
    //ry_sched_addAlgEvtListener(app_ui_onALGEvent);

    wms_event_listener_add(WM_EVENT_TOUCH_RAW, &app_ui_activity, app_ui_onTouchEvent);
    //wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &app_ui_activity, app_ui_onTouchProcessedEvent);
    wms_event_listener_add(WM_EVENT_RTC,   &app_ui_activity, app_ui_onRTCEvent);
    wms_event_listener_add(WM_EVENT_ALG,   &app_ui_activity, app_ui_onALGEvent);

    app_ui_msg_send(IPC_MSG_TYPE_APP_UI_PREPARE_DEFAULT_UI, 0, NULL);

    
    while (1) {
        status = ryos_queue_recv(app_ui_msgQ, &msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_nfc_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } 

        app_ui_tp_msg_sub(0);

        switch(msg->msgType) {

            case IPC_MSG_TYPE_TOUCH_PROCESSED_EVENT:
                touch_data_t* p = (touch_data_t*)msg->data;
                
                if (p->event == TP_PROCESSED_ACTION_SLIDE_DOWN) {
                    tp_cmd = TP_CODE_SLIDE_DOWN;
                } else if (g_touch_data.event == TP_PROCESSED_ACTION_SLIDE_UP) {
                    tp_cmd = TP_CODE_SLIDE_UP;
                } else if ((g_touch_data.event == TP_PROCESSED_ACTION_CLICK) && (g_touch_data.button==9)) {
                    tp_cmd = TP_CMD_BACK;
                } else if ((g_touch_data.event == TP_PROCESSED_ACTION_CLICK) && (g_touch_data.button<4)) {
                    tp_cmd = TP_CMD_SEL_1;
                } else if ((g_touch_data.event == TP_PROCESSED_ACTION_CLICK) && (g_touch_data.button>4)) {
                    tp_cmd = TP_CMD_SEL_2;
                } else {
                    tp_cmd = TP_CMD_NONE;
                }

                LOG_DEBUG("[app_ui_thread] tp_cmd: %d\r\n", tp_cmd);
                break;

            case IPC_MSG_TYPE_APP_UI_PREPARE_DEFAULT_UI:
                uint8_t time[16];
                sprintf(time, "%02d:%02d:%02d", \
                    hal_time.ui32Hour, hal_time.ui32Minute, hal_time.ui32Second);
                //cwin_notify_array[CNOTIFY_RTC].info[0] = time;   

                uint8_t date[16];
                sprintf(date, "%02d-%02d-%02d", \
                        hal_time.ui32Year, hal_time.ui32Month, hal_time.ui32DayOfMonth);

                uint8_t battery[16];
                sprintf(battery, "bat:%d%", sys_get_battery());
                cwin_notify_array[CNOTIFY_RTC].info[1] = battery;   

                uint8_t step[16];
                sprintf(step, "%d", alg_get_step_today());
                cwin_notify_array[CNOTIFY_RTC].info[2] = step;    
                _cnotify_prepare(&cwin_notify_array[CNOTIFY_RTC]);
                tp_cmd = TP_CMD_NONE;
                ry_free((void*)msg);
                app_ui_tp_msg_sub(1);
                
                continue;
                break;

            case IPC_MSG_TYPE_APP_UI_NEXT_PAGE:
                tp_cmd = TP_CMD_NONE;
                break;

            case IPC_MSG_TYPE_APP_UI_REFRESH_TIME:
               #if 1
                if (((t0_op == WAITING) && (pg_app_array[0].pg_op == KPG_RUN) && (untouch_timer_running)) ||
                    ((pg_app_array[2].pg_op == KPG_RUN) && (untouch_timer_running))) {
                    tp_cmd = TP_CMD_NONE;
                    LOG_DEBUG("Refresh UI\r\n");
                    break;
                } 
                #endif
                
                ry_free((void*)msg);
                msg = NULL;
                app_ui_tp_msg_sub(1);
                continue;
                
            default:
                ry_free((void*)msg);
                msg = NULL;
                app_ui_tp_msg_sub(1);
                continue;
        }

        
        if (tp_cmd != TP_CMD_NONE) {
            if (app_ui_touch_timer_id) {
                ry_timer_start(app_ui_touch_timer_id, 5000, app_ui_idle_timeout_handler, NULL);
                untouch_timer_running = 1;
                LOG_DEBUG("Untouch timer start. evt:0x%x\r\n", msg->msgType);
            }
            
        }

        ry_free((void*)msg);
        msg = NULL;

        LOG_DEBUG("[app_ui_thread]----------- k_page start. free heap size:%d\r\n", ryos_get_free_heap());
        k_page_execute();
        LOG_DEBUG("[app_ui_thread]----------- k_page end. free heap size:%d\r\n\r\n", ryos_get_free_heap());
        app_ui_tp_msg_sub(1);
    }
#endif    
}


void app_ui_prepare_two_fb(u8_t tp_cmd)
{
    if (tp_cmd == TP_CODE_SLIDE_DOWN) {
        ry_memcpy(frame_buffer_1, frame_buffer, sizeof(frame_buffer));
        p_which_fb = frame_buffer;
    } else if (tp_cmd == TP_CODE_SLIDE_UP) {
        p_which_fb = frame_buffer_1;
    }
}


/**
 * @brief   app_ui thread
 *
 * @param   None
 *
 * @return  None
 */
int app_ui_predict_thread(void* arg)
{
#if 0    
    ry_sts_t status = RY_SUCC;
    ry_timer_parm timer_para;
    app_ui_msg_t *msg = NULL;	

    LOG_DEBUG("[app_ui_predict_thread] Enter.\n");

    /* Create the RYNFC message queue */
    status = ryos_queue_create(&app_predict_msgQ, 1, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[app_ui_thread]: RX Queue Create Error.\r\n");
        while(1);
    }

    while (1) {
        status = ryos_queue_recv(app_predict_msgQ, &msg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_nfc_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } 

        switch(msg->msgType) {

            case IPC_MSG_TYPE_TOUCH_PREDICT_EVENT:
                touch_data_t* p = (touch_data_t*)msg->data;
                
                if (p->event == TP_PROCESSED_ACTION_SLIDE_DOWN) {
                    tp_cmd = TP_CODE_SLIDE_DOWN;
                } else if (g_touch_data.event == TP_PROCESSED_ACTION_SLIDE_UP) {
                    tp_cmd = TP_CODE_SLIDE_UP;
                }

                

                predict_menu_active = 0xFE;
                
                if ((pg_app_array[PG_T0].pg_op == KPG_RUN) && (t0_op == PG_EXEC)) {

                    if (tp_cmd == TP_CODE_SLIDE_DOWN) {
                        predict_menu_active = menu_active - 1;
                        if (menu_active == 0) {
                            predict_menu_active = 6;
                        }
                    } else if (tp_cmd == TP_CODE_SLIDE_UP) {
                        predict_menu_active = menu_active + 1;
                        if (menu_active == 7) {
                            predict_menu_active = 0;
                        }
                    }
                }

                LOG_DEBUG("predict tp cmd: %d, pm: %d\r\n", tp_cmd, predict_menu_active);

                // Do predict prepare
                if ((predict_menu_active != 0xFE) && (predict_menu_active <= menu_num)) {

                #if (RY_UI_EFFECT_TAP_MOVE == TRUE)
                    //p_which_fb = frame_buffer_1;
                    app_ui_prepare_two_fb(tp_cmd);
                #elif (RY_UI_EFFECT_MOVE == TRUE)
                    p_which_fb = frame_buffer_1;
                #endif
                
                    _cwin_prepare(&cwin_menu_array[predict_menu_active]);
                    predict_prepare_done = 1;
                    LOG_DEBUG("Predict Prepare done.\r\n");
                }
                
                break;


            default:
                break;
        }

        ry_free((void*)msg);



    }
#endif    
}


/**
 * @brief   API to init APP UI framework
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t app_ui_init(void)
{
    ryos_threadPara_t para;
    
    /* Start APP_UI Thread */
    strcpy((char*)para.name, "app_ui");
    para.stackDepth = 512;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 3;
    ryos_thread_create(&app_ui_thread_handle, app_ui_thread, &para);  

    /* Start APP_UI Thread */
    strcpy((char*)para.name, "app_predict_ui");
    para.stackDepth = 256;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 3;
    ryos_thread_create(&app_ui_predict_thread_handle, app_ui_predict_thread, &para);  

    return RY_SUCC;
}


ry_sts_t app_ui_onCreate(void* usrdata)
{
    LOG_DEBUG("[app_ui_onCreate]\r\n");
    if (usrdata) {
        // Get data from usrdata and then must release the buffer.
        ry_free(usrdata);
    }
    
    app_ui_init();
}

ry_sts_t app_ui_onDestroy(void)
{
#if 0
    LOG_DEBUG("[app_ui_onDestroy]\r\n");
    
    ryos_thread_delete(app_ui_thread_handle);
    ryos_thread_delete(app_ui_predict_thread_handle);
    ryos_queue_delete(app_predict_msgQ);

    wms_event_listener_del(WM_EVENT_RTC, &app_ui_activity);
    wms_event_listener_del(WM_EVENT_ALG, &app_ui_activity);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &app_ui_activity);
#endif
}




