#include "ry_font.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "am_devices_cy8c4014.h"
#include "activity_system.h"
#include "app_interface.h"
#include "gui_img.h"
#include "ryos_timer.h"
#include "gui_msg.h"
#include "Rbp.pb.h"
#include "rbp.h"
#include "motar.h"
#include "dip.h"
#include "ry_statistics.h"



/*********************************************************************
 * CONSTANTS
 */ 
#define UI_SYS_STATUE_FAIL_IMG                          "g_status_01_fail.bmp"
#define UI_SYS_STATUE_SUCC_IMG                          "g_status_00_succ.bmp"
#define UI_SYS_UPGADING_IMG                             "m_upgrade_01_doing.bmp"
#define UI_SYS_UNBINDING_IMG                            "m_setting_00_doing.bmp"
#define UI_SYS_BINDING_IMG                              "m_binding_01_doing.bmp"


#define UI_SYS_IMG_X                                    24
#define UI_SYS_IMG_Y                                    54

const char* const UI_SYS_TITLE_UNBINDING			= "Сброс";
const char* const UI_SYS_TITLE_UNBIND_FAIL			= "Отменен";
const char* const UI_SYS_TITLE_UNBIND_SUCC			= "Выполнен";

const char* const UI_SYS_TITLE_BIND_FAIL			= "Не удалось";
const char* const UI_SYS_TITLE_BINDING				= "Привязка";
const char* const UI_SYS_TITLE_BIND_SUCC			= "Выполнена";
const char* const UI_SYS_TITLE_BINDING_CONFIRM	    = "Привязать?";

const char* const UI_SYS_TITLE_UPGRADE_FAIL			= "Не удалась";
const char* const UI_SYS_TITLE_UPGRADING			= "Загрузка";
const char* const UI_SYS_TITLE_UPGRADE_SUCC			= "Выполнена";

#define UI_SYS_TITLE_X                                    0
#define UI_SYS_TITLE_Y                                    176


/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    int8_t      status;
    int8_t      page;
    int8_t      active;
} uisys_ctx_t;


/*********************************************************************
 * VARIABLES
 */

activity_t activity_system = {
    .name = "system_ui",
    .onCreate = show_system,
    .onDestroy = exit_system,
    .priority = WM_PRIORITY_L,
    .disableUntouchTimer = 1,
    .disableTouchMotar = 0,
};

static uisys_ctx_t  uisys_ctx_v;
u32_t sys_timeout_timer;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

int system_ui_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

    if(uisys_ctx_v.status == 0){
        goto exit;
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
                        //break;
                    case 5:
                    case 6:
                    case 7:
                        if (uisys_ctx_v.page == UI_SYS_BIND_ACK) {
                            DEV_STATISTICS(bind_req_ok);
                            dip_send_bind_ack(0);
                            motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 100);
                            // wms_activity_back(NULL);
                        }
						break;
                    case 8:
                    case 9:
                        {
                            //ble_send_response(CMD_DEV_BIND_ACK_START, RBP_RESP_CODE_NO_BIND, NULL, 0, 0, NULL, NULL);
                            //wms_activity_back(NULL);
                        }
                        break;
                }
            }
            break;
    }

exit:
    return 0;
}


int system_ui_onSysEvent(ry_ipc_msg_t* msg)
{
    u32_t * usrdata = (u32_t *)ry_malloc(sizeof(u32_t));
    LOG_DEBUG("[system_ui_onSysEvent]\n");
    if(msg->msgType == IPC_MSG_TYPE_UNBIND_SUCC){
        *usrdata = UI_SYS_UNBIND_SUCC;
    }
    else if(msg->msgType == IPC_MSG_TYPE_UNBIND_FAIL){
        *usrdata = UI_SYS_UNBIND_FAIL;
    }
    else if(msg->msgType == IPC_MSG_TYPE_UNBIND_FAIL){
        *usrdata = UI_SYS_UNBIND_FAIL;
    }
    else if(msg->msgType == IPC_MSG_TYPE_UPGRADE_SUCC){
        *usrdata = UI_SYS_UPGRADE_SUCC;
    }
    else if(msg->msgType == IPC_MSG_TYPE_UPGRADE_FAIL){
        *usrdata = UI_SYS_UPGRADE_FAIL;
    }
    else if(msg->msgType == IPC_MSG_TYPE_BIND_SUCC){
        *usrdata = UI_SYS_BIND_SUCC;
        DEV_STATISTICS(bind_success);
    }
    else if(msg->msgType == IPC_MSG_TYPE_BIND_FAIL){
        *usrdata = UI_SYS_BIND_FAIL;
    }
    else if(msg->msgType == IPC_MSG_TYPE_UPGRADE_START){
        *usrdata = UI_SYS_UPGRADING;
    }
    else if(msg->msgType == IPC_MSG_TYPE_BIND_START){
        *usrdata = UI_SYS_BINDING;
    }else{
        ry_free(usrdata);
        usrdata = NULL;
    }
    show_system(usrdata);
    return 0;
}


void sys_auto_timeout_handler(void* arg)
{
    if(strcmp(wms_activity_get_cur_name(), activity_system.name) == 0){
        wms_activity_back(NULL);
    }
} 

void system_display_update(u32_t show_status, char* img_name, char* title_str)
{
    u8_t w, h;
    clear_buffer_black();
    
    if(UI_SYS_BINDING == show_status){
        img_exflash_to_framebuffer((u8_t*)img_name, 0, 76, &w, &h, d_justify_center);
        gdispFillStringBox(UI_SYS_TITLE_X, UI_SYS_TITLE_Y,
                            font_roboto_20.width, font_roboto_20.height,
                            title_str, (void*)font_roboto_20.font,
                            White, Black|(1<<30), justifyCenter);
    } 
    else if (UI_SYS_BIND_ACK == show_status) {
        activity_system.disableTouchMotar = 1;
        gdispFillStringBox( 0, 
                            50, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)UI_SYS_TITLE_BINDING_CONFIRM,
                            (void*)font_roboto_20.font, 
                            White, 
                            Black,
                            justifyCenter
                            );

        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"g_widget_00_enter.bmp", \
                                0, 156, &w, &h, d_justify_center);
    } else if (UI_SYS_UPGRADING == show_status) {
        img_exflash_to_framebuffer((u8_t*)img_name, UI_SYS_IMG_X, UI_SYS_IMG_Y, &w, &h, d_justify_left);
        gdispFillStringBox(UI_SYS_TITLE_X, 156,
                            font_roboto_20.width, font_roboto_20.height,
                            title_str, (void*)font_roboto_20.font, White,  Black|(1<<30),justifyCenter);
    } else {
        img_exflash_to_framebuffer((u8_t*)img_name, UI_SYS_IMG_X, UI_SYS_IMG_Y, &w, &h, d_justify_left);
        gdispFillStringBox(UI_SYS_TITLE_X, UI_SYS_TITLE_Y,
                            font_roboto_20.width, font_roboto_20.height,
                            title_str, (void*)font_roboto_20.font, White,  Black|(1<<30),justifyCenter);
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

ry_sts_t show_system(void * usrdata)
{
    ry_sts_t ret = RY_SUCC;
    u32_t show_status = *((u32_t*)usrdata);

    if(usrdata == NULL){
        show_status = 0xFFFFFFFF;
    }

    if(uisys_ctx_v.active == 0){
        wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_system, system_ui_onTouchEvent);
        wms_event_listener_add(WM_EVENT_SYS, &activity_system, system_ui_onSysEvent);
    }
    uisys_ctx_v.active = 1;
    uisys_ctx_v.page = show_status;

    char * img_name = NULL;
    char * title_str = NULL;
    switch(show_status){
        case UI_SYS_BIND_SUCC:
            img_name = UI_SYS_STATUE_SUCC_IMG;
            title_str = (char *)UI_SYS_TITLE_BIND_SUCC;
            uisys_ctx_v.status = 1;
            break;
        case UI_SYS_UPGRADE_SUCC:
            img_name = UI_SYS_STATUE_SUCC_IMG;
            title_str = (char *)UI_SYS_TITLE_UPGRADE_SUCC;
            uisys_ctx_v.status = 1;
            break;
        case UI_SYS_UNBIND_SUCC:
            img_name = UI_SYS_STATUE_SUCC_IMG;
            title_str = (char *)UI_SYS_TITLE_UNBIND_SUCC;
            uisys_ctx_v.status = 1;
            break;
        
        case UI_SYS_UPGRADE_FAIL:
            img_name = UI_SYS_STATUE_FAIL_IMG;
            title_str = (char *)UI_SYS_TITLE_UPGRADE_FAIL;
            uisys_ctx_v.status = 1;
            break;
        case UI_SYS_UNBIND_FAIL:
            img_name = UI_SYS_STATUE_FAIL_IMG;
            title_str = (char *)UI_SYS_TITLE_UNBIND_FAIL;
            uisys_ctx_v.status = 1;
            break;
        case UI_SYS_BIND_FAIL:
            img_name = UI_SYS_STATUE_FAIL_IMG;
            title_str = (char *)UI_SYS_TITLE_BIND_FAIL;
            uisys_ctx_v.status = 1;
            break;

        case UI_SYS_UNBINDING:
            img_name = UI_SYS_UNBINDING_IMG;
            title_str = (char *)UI_SYS_TITLE_UNBINDING;
            break;
        case UI_SYS_UPGRADING:
            img_name = UI_SYS_UPGADING_IMG;
            title_str = (char *)UI_SYS_TITLE_UPGRADING;
            break;
        case UI_SYS_BINDING:
            img_name = UI_SYS_BINDING_IMG;
            title_str = (char *)UI_SYS_TITLE_BINDING;
            break;
        case UI_SYS_BIND_ACK:
            uisys_ctx_v.status = 1;
            break;

        default :
            uisys_ctx_v.status = 1;
            wms_activity_back(NULL);
            return ret;
    }
    
    LOG_DEBUG("[show_system]show_status:%d\r\n", show_status);

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    system_display_update(show_status, img_name, title_str);

    u32_t timeoutTick = 0; 
    u32_t timer_start = 0;   
    if(uisys_ctx_v.status){
        if(show_status == UI_SYS_BIND_SUCC){
            timeoutTick = 1000;
        }
        else if(show_status == UI_SYS_BIND_ACK){
            timeoutTick = 20000;
        }
        else{
            timeoutTick = 10000;
        }
        timer_start = 1;
    }else if(usrdata != NULL){
        if(show_status == UI_SYS_BINDING){
            timeoutTick = 10000;
        }
        else if(show_status == UI_SYS_UNBINDING){
            timeoutTick = 30000;
        }
        else if(show_status == UI_SYS_UPGRADING){
            timeoutTick = 1000 * 60 * 160;
        }
        timer_start = 1;
    }

    if (sys_timeout_timer == 0){
        ry_timer_parm timer_para;
        timer_para.timeout_CB = sys_auto_timeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "system_timeout";
        sys_timeout_timer = ry_timer_create(&timer_para);
    }

    if (timer_start != 0){
        ry_timer_start(sys_timeout_timer,  timeoutTick, sys_auto_timeout_handler, NULL);
    }

    ry_free(usrdata);
    return ret;
}


ry_sts_t exit_system(void)
{
    ry_sts_t ret = RY_SUCC;
    //wms_event_listener_del(WM_EVENT_MESSAGE, &activity_system);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_system);
    wms_event_listener_del(WM_EVENT_SYS, &activity_system);
    activity_system.usrdata = NULL;
    uisys_ctx_v.status = 0;
    uisys_ctx_v.active = 0;
    
    if(sys_timeout_timer != 0){
        ry_timer_delete(sys_timeout_timer);
        sys_timeout_timer = 0;
    }
    return ret;
}




















