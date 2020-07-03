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
#include <stdio.h>
#include "ry_type.h"
#include "ry_utils.h"
#include "app_config.h"
#include "ryos.h"
#include "ryos_timer.h"
#include <ry_hal_inc.h>
#include "scheduler.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h" 
    #include "ry_log_id.h"
#endif

/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "window_management_service.h"
#include "activity_hrm.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "data_management_service.h"
#include "hrm.h"
#include "timer_management_service.h"

/* resources */
#include "gui_img.h"
#include "ry_font.h"
#include "ry_statistics.h"


/*********************************************************************
 * CONSTANTS
 */ 
#define RESOUCES_IMG_INTERNAL       0
#define HRM_POS_Y_VALUE             136
#define HRM_POS_Y_STRING            200
#define HRM_TIMEOUT_MEASURE_PREP    10 * 1000
#define HRM_TIMEOUT_MEASURE         20 * 1000
#define HRM_TIMEOUT_EXIT            10 * 1000
#define HRM_TIMEOUT_BACK_SURFACE    10 * 1000

#define DEBUG_LOG_ANIMATE_TIME      0

// #undef HRM_DO_ANIMATE
// #define HRM_DO_ANIMATE			1  

/*********************************************************************
 * TYPEDEFS
 */
typedef enum {
    ACTIVITY_HRM_READY,
    ACTIVITY_HRM_START,    
    ACTIVITY_HRM_DOING,
    ACTIVITY_HRM_OFF,  
    ACTIVITY_HRM_EXIT,          
}activity_hrm_status_t;

typedef enum {
    ANIMATE_DIR_UP,
    ANIMATE_DIR_DOWN,
}animate_dir_t;


typedef enum {
    E_HRM_INFO_KEEP,
    E_HRM_INFO_BPM,
    E_HRM_INFO_UNWEAR,
    E_HRM_INFO_CLICK,
}e_hrm_info_idx_t;

typedef struct {
    u8_t          state;
    u8_t          info_idx;
    u8_t          first_disp;    
    u8_t          animate_dir;
    int8_t        animate_idx;
    int8_t        animate_finished;  
    uint8_t       last_hrm;  
    uint8_t       hrm_prepared;
} ry_hrm_ctx_t;
 
/*********************************************************************
 * VARIABLES
 */
extern activity_t activity_surface;
u32_t time_hrm_data;

ry_hrm_ctx_t    hrm_ctx_v = {
    .animate_idx = 1
};

static u32_t    app_hrm_timer_id;
ryos_thread_t   *hrm_animate_thread_handle;
ryos_semaphore_t *hrm_animate_sem;

activity_t activity_hrm = {
    .name      = "hrm",
    .onCreate  = hrm_onCreate,
    .onDestroy = hrm_onDestroy,
    .disableUntouchTimer = 1,
    .disableWristAlg = 1,
    .priority = WM_PRIORITY_M,
};

const char* const hrm_img_file = "menu_02_hrm.bmp";
const char* const hrm_img_animate[] = {
    "hrm_detect_01.bmp", //"hrm_animate_0.jpg",
    "hrm_detect_02.bmp",
    "hrm_detect_03.bmp", 
    "hrm_detect_04.bmp",

    "hrm_detect_05.bmp",
    "hrm_detect_06.bmp",
    "hrm_detect_07.bmp", 
    "hrm_detect_07.bmp"
    // "menu_02_hrm.bmp"
};

const char* const hrm_info_str[] = {"Подождите", "BPM", "Ошибка", "Измерить"};

#if DEBUG_LOG_ANIMATE_TIME
uint32_t hrm_animate_time;
uint32_t last_hrm_animate_time;
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
ry_sts_t hrm_animate_init(void);
void activity_hrm_timeout_handler(void* arg);
void activity_hrm_prep_timeout_handler(void* arg);

void hrm_ready_display_update(void)
{
    img_pos_t pos;
    uint8_t w, h;

    // LOG_DEBUG("[hrm_ready_display_update] begin. %d\r\n", hrm_ctx_v.state);
    hrm_ctx_v.animate_idx = 0;
    clear_buffer_black();
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)hrm_img_animate[hrm_ctx_v.animate_idx], 
        60, 60, &w, &h, d_justify_point);
   
    //prepare hrm data
    char data[16] = "--";
    gdispFillStringBox( 0, 
                        HRM_POS_Y_VALUE, 
                        font_roboto_32.width,
                        font_roboto_32.height,
                        data, 
                        (void*)font_roboto_32.font, 
                        White, Black, 
                        justifyCenter
                        );    
    //info
    hrm_ctx_v.info_idx = E_HRM_INFO_KEEP; 
    gdispFillStringBox( 0, 
                        HRM_POS_Y_STRING, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)hrm_info_str[E_HRM_INFO_KEEP],
                        (void *)font_roboto_20.font, 
                        White, Black, 
                        justifyCenter
                        );
    
    //must protect here, or screen off would reset system
    if (get_gui_state() == GUI_STATE_ON){
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
    // LOG_DEBUG("[hrm_ready_display_update] end. %d\r\n", hrm_ctx_v.state);
}


int32_t hrm_doing_display_update(void)
{
    int32_t ret = 0;

    img_pos_t pos;
    uint8_t   w, h;
    uint8_t   hr_cnt;
    uint8_t data[16] = "--";    
    int32_t   hrm_disp_updated = 0;
    uint32_t  hrm_weared = (get_hrm_weared_status() == WEAR_DET_FAIL) ? 0 : 1;
	w = h = 120;
    // LOG_DEBUG("[hrm_display_update] begin. %d\r\n", hrm_ctx_v.state);

#if HRM_DO_ANIMATE
    if (hrm_weared){
        clear_frame(0,120);
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)hrm_img_animate[hrm_ctx_v.animate_idx], 
        60, 60, &w, &h, d_justify_point);
    }
#endif

    if (w <=0 || w > 120 || h <=0 || h > 240) {
        goto next;
    }

#if DEBUG_LOG_ANIMATE_TIME
    time = ry_hal_calc_ms(ry_hal_clock_time() - hrm_animate_time);
    LOG_DEBUG("[animate]hrm_display_update load_img_end: %d\r\n", time);
#endif        

    //prepare hrm data
    hr_cnt = hrm_ctx_v.last_hrm;
    if (alg_get_hrm_cnt(&hr_cnt) != -1){
        if (hrm_ctx_v.last_hrm != hr_cnt){  
            hrm_disp_updated = 1;
#if 0
            uint32_t time = ry_hal_calc_ms(ry_hal_clock_time() - time_hrm_data);
            // LOG_DEBUG("animate_hrm_cnt_updated_time: %d\r\n", time);
            time_hrm_data = ry_hal_clock_time();
#endif
            if (hrm_ctx_v.hrm_prepared){
                sprintf((char*)data, "%d", hr_cnt);
                // gdispFillStringBox( 0, 
                //         HRM_POS_Y_VALUE, 
                //         120,
                //         52,
                //         (char *)data, 
                //         (void*)font_roboto_44.font, 
                //         White,  
                //         Black,
                //         justifyCenter
                //         );
                clear_frame(HRM_POS_Y_VALUE, 52);
                const uint8_t start_offset[] = {36, 20};
                uint8_t start_x = start_offset[strlen((const char *)data) % 2];
                gdispFillString(start_x, HRM_POS_Y_VALUE + 4, (const char *)data, font_roboto_44.font, White, Black);
                DEV_STATISTICS(hrm_finish_count);
            }
        }
    }
    else{
        ret = -1;
    }
    
#if DEBUG_LOG_ANIMATE_TIME
    time = ry_hal_calc_ms(ry_hal_clock_time() - hrm_animate_time);
    LOG_DEBUG("[animate]hrm_display_update end_calc_duration_time: %d\r\n", time);
#endif
    if (w == 0) {
        RY_ASSERT(0);
        LOG_ERROR("[%s] PIC width error. name=%s w=%d!!!!!!!!!!!!!!!!!!!!!!!!!\r\n", __FUNCTION__, hrm_img_animate[hrm_ctx_v.animate_idx], w);
    }
    
    pos.x_start = 0;//(120 - w) >> 1;
    pos.x_end = 119;//w - 1;
#if DEBUG_LOG_ANIMATE_TIME
    time = ry_hal_calc_ms(ry_hal_clock_time() - hrm_animate_time);
    LOG_DEBUG("[animate]hrm_display_update_middle, duration_time: %d\r\n", time);
#endif    

    {
        pos.y_start = 0;
        pos.y_end = 120;                      // default only refresh the animate image area          
        pos.buffer = frame_buffer;   

        if (hrm_disp_updated == 1){             //refresh the iamge and hrm_cnt area
#if (!HRM_DO_ANIMATE)
            pos.y_start = HRM_POS_Y_VALUE;
            pos.buffer = frame_buffer + pos.y_start * 360;
#endif
            pos.y_end = HRM_POS_Y_VALUE + 52 - 1;            
            hrm_ctx_v.last_hrm = hr_cnt;
        }
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
    }

next:
    //update next img resources
    if (hrm_ctx_v.animate_dir == ANIMATE_DIR_UP){
        hrm_ctx_v.animate_idx ++;
        if (hrm_ctx_v.animate_idx >= sizeof(hrm_img_animate) / sizeof(const char*)){
            hrm_ctx_v.animate_dir = ANIMATE_DIR_DOWN;
            hrm_ctx_v.animate_idx = sizeof(hrm_img_animate) / sizeof(const char*) - 1;
        }
    }
    else{
        hrm_ctx_v.animate_idx --;
        if (hrm_ctx_v.animate_idx < 0){
            hrm_ctx_v.animate_dir = ANIMATE_DIR_UP;
            hrm_ctx_v.animate_idx = 0;
        }
    }

#if DEBUG_LOG_ANIMATE_TIME
    time = ry_hal_calc_ms(ry_hal_clock_time() - hrm_animate_time);
    LOG_DEBUG("[animate]hrm_display_update_end, duration_time: %d\r\n", time);
#endif
    // LOG_DEBUG("[hrm_display_update] end. %d, height:%d.\r\n", hrm_ctx_v.last_hrm, font_roboto_44.height);
    return ret;
}


void hrm_retry_info_display(void)
{
    img_pos_t pos;
    pos.x_start = 0;
    pos.x_end = 120 - 1;
    pos.y_end = 240 - 1;
    pos.y_start = HRM_POS_Y_STRING; 
    pos.buffer = frame_buffer + pos.y_start * 360;
      
    if (hrm_ctx_v.info_idx != E_HRM_INFO_CLICK){
        hrm_ctx_v.info_idx = E_HRM_INFO_CLICK;
        clear_frame(HRM_POS_Y_STRING, 240 - HRM_POS_Y_STRING);
        gdispFillStringBox( 0, 
                            HRM_POS_Y_STRING, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)hrm_info_str[E_HRM_INFO_CLICK],
                            (void *)font_roboto_20.font, 
                            White, Black, 
                            justifyCenter
                        );
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
    }
}


void hrm_unwear_info_display(void)
{
    img_pos_t pos;
    pos.x_start = 0;
    pos.x_end = 120 - 1;
    pos.y_end = 240 - 1;
    pos.y_start = HRM_POS_Y_VALUE; 
    pos.buffer = frame_buffer + pos.y_start * 360;
      
    if ((hrm_ctx_v.first_disp != 0) && (hrm_ctx_v.info_idx != E_HRM_INFO_UNWEAR)) {
        hrm_ctx_v.info_idx = E_HRM_INFO_UNWEAR;
        clear_frame(HRM_POS_Y_VALUE, 240 - HRM_POS_Y_VALUE);

        //prepare hrm data
        uint8_t data[16] = "--";
        gdispFillStringBox( 0, 
                            HRM_POS_Y_VALUE, 
                            font_roboto_32.width,
                            font_roboto_32.height,
                            (char *)data, 
                            (void*)font_roboto_32.font, 
                            White, Black, 
                            justifyCenter
                            );    

        gdispFillStringBox( 0, 
                            HRM_POS_Y_STRING, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)hrm_info_str[E_HRM_INFO_UNWEAR],
                            (void *)font_roboto_20.font, 
                            White, Black, 
                            justifyCenter
                        );
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
    }
}

void hrm_bmp_info_display(void)
{
    img_pos_t pos;
    pos.x_start = 0;
    pos.x_end = 120 - 1;
    pos.y_end = 240 - 1;
    pos.y_start = HRM_POS_Y_VALUE; 
    pos.buffer = frame_buffer + pos.y_start * 360;
      
    if (hrm_ctx_v.info_idx != E_HRM_INFO_BPM){
        hrm_ctx_v.info_idx = E_HRM_INFO_BPM;
        clear_frame(HRM_POS_Y_VALUE, 240 - HRM_POS_Y_VALUE);

        //prepare hrm data
        uint8_t data[16] = "--";
        gdispFillStringBox( 0, 
                            HRM_POS_Y_VALUE, 
                            font_roboto_32.width,
                            font_roboto_32.height,
                            (char *)data, 
                            (void*)font_roboto_32.font, 
                            White, Black, 
                            justifyCenter
                            );
                    
        gdispFillStringBox( 0, 
                            HRM_POS_Y_STRING, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)hrm_info_str[E_HRM_INFO_BPM],
                            (void*)font_roboto_20.font, 
                            White, Black, 
                            justifyCenter
                        );
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
    }
}

int hrm_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {        
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[hrm_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if ((p->y >= 8)){
                hrm_ctx_v.state = ACTIVITY_HRM_EXIT;
                wms_activity_back(NULL);
                return consumed;
            }
            else{
                if (hrm_ctx_v.state == ACTIVITY_HRM_OFF){
                    // ryos_thread_resume(hrm_animate_thread_handle);
                    hrm_ctx_v.state = ACTIVITY_HRM_START; 
                    hrm_ctx_v.first_disp = 1; 
                    hrm_ctx_v.hrm_prepared = 0;
                    ry_hrm_msg_send(HRM_MSG_CMD_MANUAL_SAMPLE_START, NULL);
                    ry_timer_stop(app_hrm_timer_id);
                    ry_timer_start(app_hrm_timer_id, HRM_TIMEOUT_MEASURE_PREP, activity_hrm_prep_timeout_handler, NULL);
                    ryos_delay_ms(200);
                }
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG("[hrm_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            LOG_DEBUG("[hrm_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);
            break;

        default:
            break;
    }
    return consumed; 
}

void activity_hrm_back_to_surface_timeout_handler(void* arg)
{
    ry_sts_t status = RY_SUCC;
    status = wms_msg_send(IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE, 0, NULL);
    if (status != RY_SUCC) {
        // TODO: send error to diagnostic module.
        LOG_ERROR("aHrm send msg fail:%d", status);
        while(1);
    }
}

void activity_hrm_exit_timeout_handler(void* arg)
{
    // LOG_DEBUG("[activity_hrm_exit_timeout_handler]: transfer timeout.\n");
    ry_timer_start(app_hrm_timer_id, HRM_TIMEOUT_BACK_SURFACE, activity_hrm_back_to_surface_timeout_handler, NULL);
}    

void activity_hrm_timeout_handler(void* arg)
{
    // LOG_DEBUG("[activity_hrm_timeout_handler]: transfer timeout.\n");
    hrm_ctx_v.state = ACTIVITY_HRM_OFF;
    ry_hrm_msg_send(HRM_MSG_CMD_MANUAL_SAMPLE_STOP, NULL);
    ry_timer_start(app_hrm_timer_id, HRM_TIMEOUT_EXIT, activity_hrm_exit_timeout_handler, NULL);
}

void activity_hrm_prep_timeout_handler(void* arg)
{
    // LOG_DEBUG("[activity_hrm_prep_timeout_handler]: transfer timeout.\n");
    hrm_ctx_v.hrm_prepared = 1;
    ry_timer_start(app_hrm_timer_id, HRM_TIMEOUT_MEASURE, activity_hrm_timeout_handler, NULL);
}

/**
 * @brief   Entry of the hrm activity
 *
 * @param   None
 *
 * @return  hrm_onCreate result
 */
ry_sts_t hrm_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;
    LOG_DEBUG("[hrm]creat:heap:%d, minHeap:%d\r\n", ryos_get_free_heap(), ryos_get_min_available_heap_size());
    
    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    // battery_halt_set(1);

    hrm_ctx_v.animate_idx = (sizeof(hrm_img_animate) / sizeof(const char*)) >> 1;
    hrm_ctx_v.animate_dir = ANIMATE_DIR_UP;
    hrm_ctx_v.first_disp = 0;
    hrm_ctx_v.last_hrm = s_alg.hr_cnt;
    hrm_ctx_v.animate_finished = 1;
    hrm_ctx_v.hrm_prepared = 0;

    hrm_animate_init();
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED,   &activity_hrm, hrm_onProcessedTouchEvent);

    ry_timer_parm timer_para;
    /* Create the timer */
    timer_para.timeout_CB = activity_hrm_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "app_hrm_timer";
    app_hrm_timer_id = ry_timer_create(&timer_para);
    ry_timer_start(app_hrm_timer_id, HRM_TIMEOUT_MEASURE_PREP, activity_hrm_prep_timeout_handler, NULL);

    hrm_ctx_v.state = ACTIVITY_HRM_READY;  
    hrm_ctx_v.info_idx = ACTIVITY_HRM_READY;      
    ry_hrm_msg_send(HRM_MSG_CMD_MANUAL_SAMPLE_START, NULL);
    hrm_ready_display_update();
    hrm_ctx_v.state = ACTIVITY_HRM_START;    
#if 0
    uint32_t time = ry_hal_calc_ms(ry_hal_clock_time() - time_hrm_data);
    LOG_DEBUG("animate_hrm_cnt_updated_time: %d\r\n", time);
    time_hrm_data = ry_hal_clock_time();
#endif
    ryos_delay(200);    //delay for hrm measurement display
    ryos_semaphore_post(hrm_animate_sem);
    return ret;
}


/**
 * @brief   API to exit hrm activity
 *
 * @param   None
 *
 * @return  hrm activity Destroy result
 */
ry_sts_t hrm_onDestroy(void)
{
    ry_sts_t ret = RY_SUCC;
    ry_hrm_msg_send(HRM_MSG_CMD_MANUAL_SAMPLE_STOP, NULL);
    uint32_t destrory = 0;
    while (destrory == 0){
        if (hrm_ctx_v.animate_finished){
            if (hrm_animate_sem != NULL){
                ryos_semaphore_delete(hrm_animate_sem);
            }
            if (hrm_animate_thread_handle != NULL){
                ryos_thread_delete(hrm_animate_thread_handle);
            }
            destrory = 1;
        }
        ryos_delay_ms(30);
    }
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_hrm);
    ry_timer_stop(app_hrm_timer_id);
    ry_timer_delete(app_hrm_timer_id);
    // battery_halt_set(0);    
    LOG_DEBUG("[hrm]exit:heap:%d, minHeap:%d\r\n", ryos_get_free_heap(), ryos_get_min_available_heap_size());
    return ret;
}


/**
 * @brief   HRM thread
 *
 * @param   None
 *
 * @return  None
 */
int hrm_animate_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    // LOG_DEBUG("[hrm_animate_thread] Enter.\n");
    // ryos_thread_set_tag(NULL, TR_T_TASK_HRM_ANIMATE);
    status = ryos_semaphore_create(&hrm_animate_sem, 0);
    if (RY_SUCC != status) {
        LOG_ERROR("[hrm_animate_thread]: Semaphore Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    bool motar_enable = false;
    
    while (1) {
        status = ryos_semaphore_wait(hrm_animate_sem);
        if (RY_SUCC != status) {
            LOG_ERROR("[hrm_animate_thread]: Semaphore Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        }
        while(1) {
#if DEBUG_LOG_ANIMATE_TIME            
            hrm_animate_time = ry_hal_clock_time();
            LOG_DEBUG("[animate]last_time: %d\r\n", ry_hal_calc_ms(ry_hal_clock_time() - last_hrm_animate_time));
#endif      
            hrm_ctx_v.animate_finished = 0;
            // LOG_DEBUG("[animate_0]hrm_ctx_v.state: %d\r\n", hrm_ctx_v.state);            
            switch (hrm_ctx_v.state){
                case ACTIVITY_HRM_DOING:
                    if (hrm_doing_display_update() == -1){
                        hrm_ctx_v.state = ACTIVITY_HRM_OFF;
                    }
                    break;

                case ACTIVITY_HRM_START:
                    for (int i = 0; i < 20; i ++){
                        if (hrm_ctx_v.state != ACTIVITY_HRM_EXIT){
                            ryos_delay_ms(50);
                        }
                    }
                    // LOG_DEBUG("[animate_0]hrm_ctx_v.state: %d, weared: %d\r\n", hrm_ctx_v.state, get_hrm_weared_status());
                    if (get_hrm_weared_status() == WEAR_DET_SUCC){
                        hrm_ctx_v.state = ACTIVITY_HRM_DOING;
                        hrm_bmp_info_display();
                        motar_enable = true;
                    }
                    else if (get_hrm_weared_status() == WEAR_DET_FAIL){
                        if (hrm_ctx_v.state != ACTIVITY_HRM_EXIT){
                            hrm_unwear_info_display();   
                            ryos_delay_ms(1000);
                            hrm_ctx_v.state = ACTIVITY_HRM_OFF;
                        }
                    }
                    break;

                case ACTIVITY_HRM_OFF:
                    if (hrm_ctx_v.state != ACTIVITY_HRM_EXIT){
                        hrm_retry_info_display();
                        
                        if (motar_enable) {
                            motar_set_work(MOTAR_TYPE_STRONG_TWICE, 200);
                            motar_enable = false;
                        }
                    }
                    break;

                default:
                    break;
            }
            // LOG_DEBUG("[animate_1]hrm_ctx_v.state: %d\r\n", hrm_ctx_v.state);
            hrm_ctx_v.animate_finished = 1;

#if DEBUG_LOG_ANIMATE_TIME                                    
            uint32_t time = ry_hal_calc_ms(ry_hal_clock_time() - hrm_animate_time);
            LOG_DEBUG("[animate]end, duration_time: %d\r\n", time);
            last_hrm_animate_time = ry_hal_clock_time();
#endif     
            ryos_delay_ms(60);
        }
    }
}

/**
 * @brief   API to init HRM_animate thread
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t hrm_animate_init(void)
{
    ryos_threadPara_t para;
    
    /* Start BLE Thread */
    strcpy((char*)para.name, "hrm_animate_thread");
    para.stackDepth = 1024;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 3;
    ryos_thread_create(&hrm_animate_thread_handle, hrm_animate_thread, &para); 
 
    return RY_SUCC;
}
