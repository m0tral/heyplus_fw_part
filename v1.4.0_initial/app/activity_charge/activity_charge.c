
#include "ry_font.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "gui_bare.h"
#include "am_devices_cy8c4014.h"

#include "gui_img.h"
#include "ryos_timer.h"
#include "gui_msg.h"
#include "data_management_service.h"
#include "ry_ble.h"
#include "activity_charge.h"
#include "timer_management_service.h"
#include "ry_hal_mcu.h"
#include <stdio.h>
#include <board_control.h>


#define BATTERY_ICON_NAME       "m_status_02_charging2.bmp"
#define ICON_X                  32
#define ICON_Y                  88

#define START_X                 (ICON_X+3)
#define START_Y                 (ICON_Y+4)
#define FULL_X                  (START_X+48)
#define FULL_Y                  (START_Y+20)

#define FILL_RANGE              52


activity_t activity_charge = {
    .name = "charge",
    .onCreate = show_charge_view,
    .onDestroy = exit_charge_view,
    .disableUntouchTimer = 1,
    .priority = WM_PRIORITY_L,
};

uint32_t charging_start_time;
uint32_t switching_timer_idx;

void charging_disp_update(int percent);

void set_battery_fill_color(int percent)
{
    int end_x = (percent * (FULL_X - START_X))/100;

    int i , j;

    int color;

    if(percent < 5){
        color = Blue;
    }else{
        color = Green;
    }

    for(i = START_Y; i < FULL_Y; i++){
        for(j = START_X; j < START_X + end_x; j++){
            set_dip_at_framebuff(j, i, color);
        }
    }
}

int charge_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    
    char* cur_name = wms_activity_get_cur_name();
        
    if(strcmp(cur_name, activity_charge.name) == 0){
        charging_activity_back_processing();
    }
    
    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:            
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:            
            break;
        
        case TP_PROCESSED_ACTION_TAP:
            {
                switch (p->y){

                    case 1:
                    case 2:
                    case 3:
                        break;
                    case 5:
                    case 6:
                        break;
                    case 7:
                    case 8:
                    case 9:
                        break;
                }
            }
            break;
    }
    return consumed;
}

uint32_t last_charge_time = 0;
int percent_prev = 0;

void clear_estimate_calcs(void)
{
    last_charge_time = 0;
    percent_prev = 0;
}

uint32_t calcEstChargeTime(int percent)
{
    if (percent == 100) return 0;
    if (percent == 99) return 5 * 60;
    
    uint32_t millis_per_percent;
    static int count;    
    static uint32_t total_millis_per_percent;
    int avg_millis_per_percent = 0;
    
    if (percent_prev > 0 && percent > percent_prev) {
        
        if (last_charge_time > 0) {
            int percent_diff = percent - percent_prev;
            millis_per_percent = ry_hal_calc_ms(ry_hal_clock_time() - last_charge_time) / percent_diff;
        }
        
        count++;
        total_millis_per_percent += millis_per_percent;
        avg_millis_per_percent = total_millis_per_percent / count;
        
        last_charge_time = ry_hal_clock_time();
        percent_prev = percent;
    }
    else {
        count = 0;
        total_millis_per_percent = 0;
        avg_millis_per_percent = 36 * 1000; // 36 sec for 1 percent = 1 hour for charge
        percent_prev = percent;
        last_charge_time = ry_hal_clock_time();
    }

    return avg_millis_per_percent * (100 - percent) / 1000;
}

const char* const fmt_min = "%02d:%02d час";
const char* const fmt_sec = "%02d:%02d мин";
void charging_disp_update(int percent)
{
    u8_t image_width, image_height;
    char str[20] = {0};

    clear_buffer_black();
    img_exflash_to_framebuffer(BATTERY_ICON_NAME, ICON_X, ICON_Y, &image_width, &image_height, d_justify_left);
    set_battery_fill_color(percent);
    
    sprintf(str, "%d%%", percent);
    gdispFillStringBox(0, 128, font_roboto_20.width, font_roboto_20.height,
                        str, (void*)font_roboto_20.font, White, Black, justifyCenter);
    
    int estTime = calcEstChargeTime(percent);
    if (estTime > 0) {
        ry_memset(str, 0, sizeof(str));
        
        int hour = estTime / 3600;
        int min = estTime % 3600 / 60;
        int sec = estTime % 60;
        
        if (hour == 0)
            sprintf(str, fmt_sec, min, sec);
        else
            sprintf(str, fmt_min, hour, min);
        
        gdispFillStringBox(0, 160, font_roboto_20.width, font_roboto_20.height,
                            str, (void*)font_roboto_20.font, White, Black, justifyCenter);
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    
    LOG_DEBUG("[charging_disp_update] percent: %d\r\n", percent);
}


int charge_onRTCEvent(ry_ipc_msg_t* msg)
{
    static uint8_t batt_last = 0;
    int percent = sys_get_battery();
    if (percent != batt_last){
        charging_disp_update(percent);
        batt_last = percent;
    }
    return 1;
}

ry_sts_t show_charge_view(void* usrdata)
{

    //wms_event_listener_add(WM_EVENT_MESSAGE, &activity_charge, sport_info_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_charge, charge_onTouchEvent);
    wms_event_listener_add(WM_EVENT_RTC, &activity_charge, charge_onRTCEvent);

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    if (switching_timer_idx == IDX_UNTOUCH_TIMER){
        wms_untouch_timer_stop();
    }
    else {
        wms_quick_untouch_timer_stop();
    }
    
    clear_estimate_calcs();
    int percent = sys_get_battery();

    if(charge_cable_is_input()) {
        charging_disp_update(percent);
    } else {
        wms_activity_back(NULL);
    }
    
    charging_start_time = ry_hal_clock_time();

    if(usrdata != NULL){
        ry_free(usrdata);
    }

    return RY_SUCC;
}


ry_sts_t exit_charge_view(void)
{
    clear_estimate_calcs();
    
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_charge);
    wms_event_listener_del(WM_EVENT_RTC, &activity_charge);
    if (switching_timer_idx == IDX_UNTOUCH_TIMER){
        wms_untouch_timer_start_for_next_activty();
    }
    else{
        wms_quick_untouch_timer_start();
    }
    return RY_SUCC;
}

/**
 * @brief   charging_activity_back_processing to back or root
 *          time not enough - back; else: enough to root
 *
 * @param   None
 *
 * @return  None
 */
void charging_activity_back_processing(void)
{
    ry_sts_t status = RY_SUCC;
    if (ry_hal_calc_ms(ry_hal_clock_time() - charging_start_time) >= 10000){
        status = wms_msg_send(IPC_MSG_TYPE_WMS_ACTIVITY_BACK_SURFACE, 0, NULL);
        if (status != RY_SUCC) {
            // TODO: send error to diagnostic module.
            LOG_ERROR("aCharge send msg fail:%d", status);
            while(1);
        }
    }
    else {
        wms_activity_back(NULL);
    }
}

void charging_swithing_timer_set(uint32_t idx)
{
    switching_timer_idx = idx;
}
