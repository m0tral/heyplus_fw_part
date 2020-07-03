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
#include "app_config.h"
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"
#include "ryos_timer.h"
#include <ry_hal_inc.h>
#include "scheduler.h"


/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "window_management_service.h"
#include "activity_sfc_option.h"
#include "activity_surface.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"

/* resources */
#include "gui_img.h"


/*********************************************************************
 * CONSTANTS
 */ 

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    int8_t    list;
} ry_sfc_option_ctx_t;

typedef struct {
    uint8_t list_id;
    const char* bg_img;
} sfc_option_disp_t;

 /*********************************************************************
 * VARIABLES
 */
extern activity_t activity_surface;

ry_sfc_option_ctx_t sfc_option_ctx_v;

sfc_option_disp_t sfc_option_list[SURFACE_OPTION_MAX] = {
    {SURFACE_RED_ARRON, "surface_red_arrow.png"},   
};
 
activity_t activity_sfc_option = {
    .name      = "sfc_option",
    .onCreate  = activity_sfc_option_onCreate,
    .onDestroy = activity_sfc_option_onDestrory,
    .priority = WM_PRIORITY_M,
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */

ry_sts_t sfc_option_display_update(void)
{
    ry_sts_t ret = RY_SUCC;

    LOG_DEBUG("[sfc_option_display_update] begin.\r\n");
    clear_buffer_black();

    //sfc_option background image
	gdispImage myImage;
	gdispImageOpenFile(&myImage, sfc_option_list[sfc_option_ctx_v.list].bg_img);
    gdispImageDraw(&myImage,                    
                   (120 - myImage.width) >> 1,  
                   0,                           
				   myImage.width,               
                   myImage.height,              
                   0,                           
                   0);
	gdispImageClose(&myImage);

    update_buffer_to_oled();

    LOG_DEBUG("[sfc_option_display_update] end.\r\n");
    return ret;
}

int sfc_option_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if ((p->y >= 8)){
                wms_activity_back(NULL);
            }
            if (p->y <7){
                set_surface_option(sfc_option_ctx_v.list);
                wms_activity_back(NULL);                
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            sfc_option_ctx_v.list ++;
            if (sfc_option_ctx_v.list >= SURFACE_OPTION_MAX){
                sfc_option_ctx_v.list = 0;
            }
            // sfc_option_display_update();
            activity_surface_on_event(SURFACE_DISPLAY_FROM_SFC_OPTION);
//            surface_disp_instance[sfc_option_ctx_v.list].display(SURFACE_DISPLAY_FROM_SFC_OPTION);
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            // To see message list
            LOG_DEBUG("[surface_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);
            sfc_option_ctx_v.list --;
            if (sfc_option_ctx_v.list < 0){
                sfc_option_ctx_v.list = SURFACE_OPTION_MAX - 1;
            }
            activity_surface_on_event(SURFACE_DISPLAY_FROM_SFC_OPTION);
//            surface_disp_instance[sfc_option_ctx_v.list].display(SURFACE_DISPLAY_FROM_SFC_OPTION);
            // sfc_option_display_update();
            break;

        default:
            break;
    }
    return consumed; 
}

/**
 * @brief   Entry of the sfc_option activity
 *
 * @param   None
 *
 * @return  activity_sfc_option_onCreate result
 */
ry_sts_t activity_sfc_option_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    LOG_DEBUG("[activity_sfc_option_onCreate]\r\n");
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_sfc_option, sfc_option_onProcessedTouchEvent);

    sfc_option_ctx_v.list = get_surface_option();
    
    /* Add Layout init here */
    touch_disable();
    activity_surface_on_event(SURFACE_DISPLAY_FROM_SFC_OPTION);
//    surface_disp_instance[sfc_option_ctx_v.list].display(SURFACE_DISPLAY_FROM_SFC_OPTION);
    touch_enable();

    return ret;
}

/**
 * @brief   API to exit sfc_option activity
 *
 * @param   None
 *
 * @return  sfc_option activity Destrory result
 */
ry_sts_t activity_sfc_option_onDestrory(void)
{
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_sfc_option);

    return RY_SUCC;
    
}







