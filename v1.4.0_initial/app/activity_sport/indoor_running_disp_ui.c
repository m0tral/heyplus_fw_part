#include <stdio.h>
#include "ry_font.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "led.h"
#include "am_devices_cy8c4014.h"
#include "board_control.h"

#include "gui_img.h"
#include "ryos_timer.h"
#include "gui_msg.h"
#include "data_management_service.h"
#include "device_management_service.h"
#include "card_management_service.h"
#include "window_management_service.h"
#include "timer_management_service.h"
#include "ry_ble.h"
#include "sensor_alg.h"
#include "app_pm.h"
#include "hrm.h"
#include "activity_card.h"
#include "algorithm.h"
#include "motar.h"
#include "activity_sports.h"
#include "ry_statistics.h"
#include "scrollbar.h"
#include "sensor_alg.h"
#include "board_control.h"
#include "activity_running.h"


/*********************************************************************
 * CONSTANTS
 */   
#define SCROLLBAR_HEIGHT                40
#define SCROLLBAR_WIDTH                 1


/*
 * TYPES
 *******************************************************************************
 */


/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */


void indoor_running_update_para(u8_t state, u32_t count, float* p_distance, u32_t* p_step_distanceStart)
{
    //data access and report
    if ((state == STATE_RUNNING_DOING) || \
        (state == STATE_ONE_KM_ALERT) || \
        (state == STATE_RUNNING_BREAK)){
        if (count % GET_LOCATION_TIME == 0){
            // update_run_param();
            if (sport_common_get_distance_by_step(p_distance, p_step_distanceStart) != 0){
                //set_run_point(*p_distance);            //save distance point
                set_indoor_run_point(*p_distance);
                // LOG_DEBUG("[indoor_set_run_point]: p_distance:%.2f, step_distanceStart:%d\r\n", *p_distance, *p_step_distanceStart);
            }
        }
    }
}

uint32_t indoor_running_result_pre_proc(u32_t duration_second, u32_t delta_step)
{
    u32_t ret = 0;

    if (sport_common_abnormal_result_processing((duration_second > 120), (delta_step > 200))){
        // set_current_running_disable();
        ret = 1;     // do not display running result.
    }
    return ret;
}

void indoor_running_pause(void)
{
    // pause_record_run();
    DEV_STATISTICS(sport_pause);
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);  
}


void indoor_running_resume(void)
{
    // resume_record_run();                                        
    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_START, NULL);
}


void indoor_running_terminate(u32_t from_doing)
{
    DEV_STATISTICS(sport_stop);
    DEV_STATISTICS(sport_finish);        
    clear_running();
    if (from_doing != 0){
	    ry_hrm_msg_send(HRM_MSG_CMD_SPORTS_RUN_SAMPLE_STOP, NULL);
    }
    alg_msg_transfer(ALG_MSG_GENERAL);
    // stop_record_run();    
}
