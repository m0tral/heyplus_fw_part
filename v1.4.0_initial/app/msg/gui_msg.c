#include"gui_msg.h"

#include "ry_font.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "ry_hal_mcu.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "am_devices_cy8c4014.h"

#include "gui_img.h"
#include "ryos_timer.h"
#include "motar.h"
#include "scheduler.h"
#include "scrollbar.h"
#include "ry_statistics.h"
#include "card_management_service.h"
#include "device_management_service.h"
#include <stdio.h>

/*********************************************************************
 * CONSTANTS
 */ 

#define TITLE_START_POSTION_X               0
#define TITLE_START_POSTION_Y               44

#define CONTENT_START_POSTION_X             0
#define CONTENT_START_POSTION_Y             70

#define DROP_MSG_SCREEN_CHAR                10
#define DROP_MSG_TOP_START_Y                0
#define DROP_MSG_BOTTOM_START_Y             120
#define TITLE_TIME_OFFSET_X                 65
#define TITLE_TIME_OFFSET_Y                 12

#define PHONE_NUM_ANSWERABLE_Y              93
#define PHONE_NUM_UNANSWERABLE_Y            32
#define ROLL_SPEED                          150
#define ROLL_TEXT_LENGTH                    10



#define AS_ICON_IMG_NAME                    "icon_01_as.bmp"
#define TELE_ANSWER_ICON_NAME               "icon_phone_answer.bmp"
#define TELE_HANGUP_ICON_NAME               "icon_phone.bmp"
#define TELE_HANGUP_ICON1_NAME              "icon_phone1.bmp"

#define SCROLLBAR_HEIGHT                    40
#define SCROLLBAR_WIDTH                     1

#define NEED_DELETE_MSG                     1
#define NO_NEED_DELETE_MSG                  0


/*********************************************************************
 * TYPEDEFS
 */

typedef enum{
    MSG_EVENT_TIMEOUT_BACK = 0xA0001,

}msg_event_e;



typedef struct app_key_and_img{
    char * key;
    char * img;
}app_key_and_img_t;

typedef struct gui_msg_show_info{
    u8_t screen_num;
    u8_t cur_screen;
    u32_t* start_offset;
}gui_msg_show_info_t;



typedef struct {
    u8_t screen_onOff;
    u8_t exit_timer_count;
} msg_ctx_t;


/*********************************************************************
 * VARIABLES
 */
const app_key_and_img_t app_img_array[] ={
    {APP_WEIXIN,    APP_WEIXIN_IMG},
    {APP_QQ,        APP_QQ_IMG},
    {APP_WEIBO,     APP_WEIBO_IMG},
    {APP_TAOBAO,    APP_TAOBAO_IMG},
    {APP_TOUTIAO,   APP_TOUTIAO_IMG},
    {APP_NETEASE,   APP_NETEASE_IMG},
    {APP_ZHIHU,     APP_ZHIHU_IMG},
    {APP_ZHIFUBAO,  APP_ZHIFUBAO_IMG},
    {APP_DOUYIN,    APP_DOUYIN_IMG},
    {APP_DINGDING,  APP_DINGDING_IMG},
    {APP_MIJIA,     APP_MIJIA_IMG},
    {APP_MOMO,      APP_MOMO_IMG},
		
    {APP_WHATSAPP,		APP_WHATSAPP_IMG},
    {APP_TELEGRAM,		APP_TELEGRAM_IMG},
    {APP_VIBER,			APP_VIBER_IMG},
    {APP_FACEBOOK,		APP_FACEBOOK_IMG},
    {APP_VKONTAKTE,		APP_VKONTAKTE_IMG},
    {APP_TWITTER,		APP_TWITTER_IMG},
    {APP_GMAIL,			APP_GMAIL_IMG},
    {APP_YANDEXMAIL,	APP_YANDEXMAIL_IMG},
    {APP_INSTAGRAM,		APP_INSTAGRAM_IMG},
    {APP_4PDA,			APP_4PDA_IMG},
    {APP_SKYPE,			APP_SKYPE_IMG},
};


ry_sts_t show_new_msg(u32_t select_msg,u32_t motar_enable); 
ry_sts_t exitMsgNotify(void);
ry_sts_t show_msg(void *userdata);
void show_next_screen(void);
void show_prev_screen(void);
ry_sts_t show_drop(void *userdata);
ry_sts_t exit_drop(void);
ry_sts_t show_drop_msg_menu(int offset, int type);
ry_sts_t show_tele_msg(u32_t select_msg, u32_t disp_offset);

void show_drop_top(message_info_t * disp_msg);
void show_drop_bottom(message_info_t * disp_msg);


activity_t msg_activity = {
    .name = "msg_ui", 
    .which_app =NULL, 
    .usrdata = NULL, 
    .onCreate = show_msg, 
    .onDestroy = exitMsgNotify,
    .disableUntouchTimer = 1,
    .priority = WM_PRIORITY_M,
    .disableTouchMotar = 1,
};

activity_t drop_activity = {
    .name = "drop_ui", 
    .which_app = NULL, .usrdata = NULL, 
    .onCreate = show_drop, 
    .onDestroy = exit_drop,
    .priority = WM_PRIORITY_M,
};

msg_ctx_t msg_ctx_v;

u32_t beenShowData = 0;
u32_t cur_show_data = 0;
extern int tp_sub_state ;
u8_t delete_msg_flag = NO_NEED_DELETE_MSG;
u8_t tele_on_calling_flag = 0;
u32_t msg_drop_num = 0;
u32_t msg_show_num = 0;

u32_t msg_auto_exit_timer = 0;
scrollbar_t * drop_scrollbar = NULL;
scrollbar_t * msg_scrollbar = NULL;
extern activity_t activity_card_session;

gui_msg_show_info_t msg_show_info = {0};

extern void gdisp_update(void);

const char* const text_nonew_messages1 = "Нет новых";
const char* const text_nonew_messages2 = "сообщений";
const char* const on_call_str = "Звонок";
const char* const text_delete = "Удалить";

static void init_msg_show_info(message_info_t *disp_msg, gui_msg_show_info_t * info)
{
    if((disp_msg == NULL)||(info == NULL)){
        return;
    }
    if(info->start_offset != NULL){
        FREE_PTR(info->start_offset);        
    }
    
    int font_width = font_roboto_20.width;
    int font_height = font_roboto_20.height;
    
    info->cur_screen = 0;
    info->screen_num = 0;
    
    u32_t first_screen_width = (240 - CONTENT_START_POSTION_Y)/(LINE_SPACING + font_height) * RM_OLED_WIDTH;
    u32_t screen_width = (240)/(LINE_SPACING + font_height) * RM_OLED_WIDTH;

    u32_t content_str_len = getStringCharCount((char*)&(disp_msg->data[0]));
    u32_t content_offset = 0;
    u16_t screen_char = (240 -0)/(LINE_SPACING + font_height) *((120)/font_width);
    u16_t temp = 0;

    info->start_offset = (u32_t *)ry_malloc(sizeof(u32_t)*((content_str_len)/screen_char + 1) *2 );
    ry_memset(info->start_offset, 0, (sizeof(u32_t)*((content_str_len)/screen_char + 1) *2 ));

    info->start_offset[0] = 0;

    while(1){
        if(info->screen_num != 0){
            temp = getWidthOffsetInnerFont((char*)&(disp_msg->data[content_offset]),
                                            screen_width, font_roboto_20.font);
        }else{
            temp = getWidthOffsetInnerFont((char*)&(disp_msg->data[content_offset]),
                                            first_screen_width, font_roboto_20.font);
        }
        content_offset += temp;

        info->screen_num++;
        info->start_offset[info->screen_num] = content_offset;        

        if(content_offset >= (disp_msg->data_len - 1)){
            break;
        }
    }

    /*if(content_str_len > screen_char){
        content_offset = getWidthOffset(&(disp_msg->data[beenShowData]), screen_width);
        //beenShowData = content_offset;
    }else{
        content_offset = disp_msg->data_len - beenShowData;
    }*/
    
exit:
    return;
}


int msg_ui_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)MSG_DISABLE_MOTAR;
    message_info_t * disp_msg = app_notify_get(0);
    //message_info_t * cur_msg = app_notify_get(msg_show_num + 1);
    if((tele_on_calling_flag == ON_CALLING)&&(disp_msg->type != Notification_Type_TELEPHONY)){
        return 0;
    }
    
    if(disp_msg->type == Notification_Type_TELEPHONY){
        if (msg_ctx_v.screen_onOff == GUI_STATE_OFF){
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
        }
        wms_activity_back(NULL);
        void * usrdata = (void *)0xff;
        wms_activity_jump(&msg_activity, usrdata);
        return 1;
    } else if(msg->len != 0) {
        int* event = (int*)msg->data;

        if((*event) == MSG_EVENT_TIMEOUT_BACK){
            wms_activity_back(NULL);
        }

        return 0;
    }
    delete_msg_flag = NO_NEED_DELETE_MSG;
    show_msg(usrdata);
    return 0;
}

int msg_ui_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    message_info_t * disp_msg = NULL;

    if(tele_on_calling_flag != ON_CALLING){
        disp_msg = app_notify_get(msg_show_num);
    } else {
        disp_msg = app_notify_get(TELE_MSG_NUM);
    }

    msg_activity.disableUntouchTimer = 0;

    if(disp_msg->type != Notification_Type_TELEPHONY){
        switch(p->action){
            case TP_PROCESSED_ACTION_SLIDE_DOWN:
                show_prev_screen();
                break;
            case TP_PROCESSED_ACTION_SLIDE_UP:
                show_next_screen();
                break;
            case TP_PROCESSED_ACTION_TAP:
            {
                DEV_STATISTICS(msg_click_count);
                switch (p->y){

                    case 1:
                    case 2:
                    case 3:
                        {
                        }
                        break;

                    case 5:
                    case 6:
                    case 7:
                        {
                            
                        }
                        break;

                    case 8:
                    case 9:
                        {
                            wms_activity_back(NULL);
                        }
                        break;                        
                    }
                }
                break;
        }
    } else {
        //motar_stop();
        switch(p->action){
            case TP_PROCESSED_ACTION_SLIDE_DOWN:
                break;
            case TP_PROCESSED_ACTION_SLIDE_UP:
                break;
            case TP_PROCESSED_ACTION_TAP:
                {
                    switch (p->y)
										{
                        case 1:
                        case 2:
                        case 3:
                            {
                                if (delete_msg_flag == NO_NEED_DELETE_MSG) {
                                    if(*(disp_msg->app_id) == Telephony_Status_RINGING_ANSWERABLE){
                                        app_notify_tele_answer(TELE_MSG_NUM);
                                        app_notify_delete(msg_show_num);
                                        wms_activity_back(NULL);
                                        motar_stop();
                                    }
                                } else if (delete_msg_flag == NEED_DELETE_MSG) {
                                    //app_notify_delete(msg_show_num);
                                    //wms_activity_back(NULL);
                                }
                            }
                            break;

                        case 5:
                        case 6:
                        case 7:
                            {
                                if (delete_msg_flag == NO_NEED_DELETE_MSG) {
                                    app_notify_tele_reject(TELE_MSG_NUM);
                                    //app_notify_delete(msg_show_num);
                                    //wms_activity_back(NULL);
                                    motar_stop();
                                } else if (delete_msg_flag == NEED_DELETE_MSG) {
                                    //app_notify_delete(msg_show_num);
                                    //wms_activity_back(NULL);
                                }
                            }
                            break;

                        case 8:
                        case 9:
                            {
                                if (NO_NEED_DELETE_MSG == delete_msg_flag) {                                    
                                } else if (NEED_DELETE_MSG == delete_msg_flag) {
                                    wms_activity_back(NULL);
                                    motar_stop();
                                }
                            }
                            break;
                    }
                }
								break;
            case TP_PROCESSED_ACTION_LONG_TAP:
                {
                    switch (p->y)
										{
                        case 1:
                        case 2:
                        case 3:
                            {
                                if (delete_msg_flag == NO_NEED_DELETE_MSG) {
                                    if(*(disp_msg->app_id) == Telephony_Status_RINGING_ANSWERABLE){
                                        app_notify_tele_answer(TELE_MSG_NUM);
                                        app_notify_delete(msg_show_num);
                                        wms_activity_back(NULL);
                                        motar_stop();
                                    }
                                } else if (delete_msg_flag == NEED_DELETE_MSG) {
                                    //app_notify_delete(msg_show_num);
                                    //wms_activity_back(NULL);
                                }
                            }
                            break;

                        case 5:
                        case 6:
                        case 7:
                            {
                                if (delete_msg_flag == NO_NEED_DELETE_MSG) {
                                    app_notify_tele_reject(TELE_MSG_NUM);
                                    app_notify_delete(msg_show_num);
                                    wms_activity_back(NULL);
                                    motar_stop();
                                } else if (delete_msg_flag == NEED_DELETE_MSG) {
                                    //app_notify_delete(msg_show_num);
                                    //wms_activity_back(NULL);
                                }
                            }
                            break;

                        case 8:
                        case 9:
                            {
                                if (NO_NEED_DELETE_MSG == delete_msg_flag) {
                                    
                                } else if (NEED_DELETE_MSG == delete_msg_flag) {
                                    wms_activity_back(NULL);
                                    motar_stop();
                                }
                            }
                            break;                        
                    }
                }
								break;								
        }
    }    
    return 0;
}

void drop_no_msg(void)
{
    uint8_t w,h;
    msg_drop_num = 0;
    clear_buffer_black();

        
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"m_no_msg.bmp",\
        0, 66, &w, &h, d_justify_center);

    gdispFillStringBox(0,
                    134,
                    font_roboto_20.width,
                    font_roboto_20.height,
                    (char*)text_nonew_messages1,
                    (void *)font_roboto_20.font, 
                    0x9B9B9B,
                    Black|(1<<30),
                    justifyCenter);
										
    gdispFillStringBox(0,
                    164,
                    font_roboto_20.width,
                    font_roboto_20.height,
                    (char*)text_nonew_messages2,
                    (void *)font_roboto_20.font, 
                    0x9B9B9B,
                    Black|(1<<30),
                    justifyCenter);										
										
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL); 
}


int drop_ui_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    u32_t msg_num = app_notify_num_get();
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

    if(msg_num == 0){
        wms_activity_back(NULL);
        return 0;
    }

    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            #if 0
            msg_drop_num+=2;
            DEV_STATISTICS(drop_down_count);
            show_drop_msg_menu(-240, SLIDE_TYPE_TRANSLATION);
            #else
            msg_drop_num++;
            DEV_STATISTICS(drop_down_count);
            show_drop_msg_menu(-120, SLIDE_TYPE_TRANSLATION);
            #endif
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            #if 0
            DEV_STATISTICS(drop_up_count);
            if(msg_drop_num == 0){
                wms_activity_back(NULL);
            }else{
                msg_drop_num-=2;
                show_drop_msg_menu(120, SLIDE_TYPE_TRANSLATION);
            }
            #else
            DEV_STATISTICS(drop_up_count);
            if(msg_drop_num <= 0){
                wms_activity_back(NULL);
            }else{
                msg_drop_num--;
                show_drop_msg_menu(120, SLIDE_TYPE_TRANSLATION);
            }

            #endif
            break;
        case TP_PROCESSED_ACTION_TAP:
            {
                switch (p->y){

                    case 1:
                    case 2:
                    case 3:     // enter msg click
                        {
                            DEV_STATISTICS(drop_click_count);
                            #if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
                            if((msg_num < 2 )||(msg_drop_num ==(msg_num-1))){
                                //wms_activity_jump(&msg_activity, (void *)0 );
                                app_notify_clear_all();
                                drop_no_msg();
                                DEV_STATISTICS(drop_clear_all_count);
                            }else{
                                wms_activity_jump(&msg_activity, (void *)(msg_drop_num + 1) );
                            }
                            #else

                            wms_activity_jump(&msg_activity, (void *)(msg_drop_num) );

                            #endif
                        }
                        break;

                    case 5:
                    case 6:
                    case 7:     // delete messages click
                        {
                            /*if(msg_num < 2 ){
                                wms_activity_jump(&msg_activity, (void *)0 );
                            }else{
                                wms_activity_jump(&msg_activity, (void *)(msg_drop_num) );
                            }*/

                            DEV_STATISTICS(drop_click_count);
                            #if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
                            
                            wms_activity_jump(&msg_activity, (void *)(msg_drop_num) );

                            #else

                            if((msg_num < 2 )||(msg_drop_num == 0 )){
                                //wms_activity_jump(&msg_activity, (void *)0 );
                                app_notify_clear_all();
                                drop_no_msg();
                                DEV_STATISTICS(drop_clear_all_count);
                                
                                wms_activity_back(NULL);
                            }else{
                                wms_activity_jump(&msg_activity, (void *)(msg_drop_num - 1) );
                            }

                            #endif
                        }
                        break;

                    case 8:
                    case 9:     // close an return on the main
                        {
                            wms_activity_back(NULL);
                        }
                        break;
                }
            }
            break;
    }
    return 0;
}


int drop_ui_onMSGEvent(ry_ipc_msg_t* msg)
{
    //show_drop(NULL);    
    void * usrdata = (void *)MSG_DISABLE_MOTAR;
    message_info_t * disp_msg = app_notify_get(0);
    //message_info_t * cur_msg = app_notify_get(msg_show_num + 1);
    if((tele_on_calling_flag == ON_CALLING)&&(disp_msg->type != Notification_Type_TELEPHONY)){
        return 0;
    }
    if(disp_msg->type == Notification_Type_TELEPHONY){
        if (msg_ctx_v.screen_onOff == GUI_STATE_OFF){
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
        }
        wms_activity_back(NULL);
        void * usrdata = (void *)0xff;
        wms_activity_jump(&msg_activity, usrdata);
        return 1;
    }
    delete_msg_flag = NO_NEED_DELETE_MSG;
    wms_activity_jump(&msg_activity, usrdata);
    return 0;
}

void msg_auto_timeout_handler(void* arg)
{
    msg_ctx_v.exit_timer_count++;
    //LOG_DEBUG("{%s}--count:%d\n",__FUNCTION__, msg_ctx_v.exit_timer_count);
    if(strcmp(wms_activity_get_cur_name(), msg_activity.name) == 0){

        if(msg_ctx_v.exit_timer_count == 5){
            if (msg_ctx_v.screen_onOff == GUI_STATE_OFF){
                ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
            }
        }

        if(msg_ctx_v.exit_timer_count >= 8){
            //wms_activity_back(NULL);

            if(current_time_is_dnd_status() != 1){
                int event = MSG_EVENT_TIMEOUT_BACK;
    			ry_sched_msg_send(IPC_MSG_TYPE_MM_NEW_NOTIFY, sizeof(event), (u8_t *)&event);
    		}
        }
    }

    if (msg_ctx_v.exit_timer_count >= 8) {
        ry_timer_delete(msg_auto_exit_timer);
        msg_auto_exit_timer = 0;
    } else {
        ry_timer_start(msg_auto_exit_timer, 1000, msg_auto_timeout_handler, NULL);
    }
} 

int msg_ui_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}

int msg_ui_onProcessedBleEvent(ry_ipc_msg_t* msg)
{
    if(tele_on_calling_flag == ON_CALLING){
        motar_stop();
        wms_activity_back(NULL);
    }
    return 1;
}


ry_sts_t show_msg(void *userdata)
{
    ry_sts_t ret = RY_SUCC;
    beenShowData = 0;
    wms_event_listener_add(WM_EVENT_MESSAGE, &msg_activity, msg_ui_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &msg_activity, msg_ui_onTouchEvent); 
    wms_event_listener_add(WM_EVENT_CARD, &msg_activity, msg_ui_onProcessedCardEvent);
    wms_event_listener_add(WM_EVENT_BLE, &msg_activity, msg_ui_onProcessedBleEvent);
    u32_t motar_enable = 1;
    u32_t select_func = (u32_t)userdata;
    msg_ctx_v.exit_timer_count = 0;
	
    if( select_func == 0xff){
        select_func = 0;
        if(msg_auto_exit_timer != 0){
            ry_timer_start(msg_auto_exit_timer,  1000, msg_auto_timeout_handler, NULL);
        }
        else{
            ry_timer_parm timer_para;
            timer_para.timeout_CB = msg_auto_timeout_handler;
            timer_para.flag = 0;
            timer_para.data = NULL;
            timer_para.tick = 1;
            timer_para.name = "msg_timer";
            msg_auto_exit_timer = ry_timer_create(&timer_para);
            ry_timer_start(msg_auto_exit_timer,  1000, msg_auto_timeout_handler, NULL);
        }
    }else if(select_func == MSG_DISABLE_MOTAR){
        select_func = 0;
        motar_enable = 0;
        if(msg_auto_exit_timer != 0){
            ry_timer_start(msg_auto_exit_timer,  1000, msg_auto_timeout_handler, NULL);
        }
        else{
            ry_timer_parm timer_para;
            timer_para.timeout_CB = msg_auto_timeout_handler;
            timer_para.flag = 0;
            timer_para.data = NULL;
            timer_para.tick = 1;
            timer_para.name = "msg_timer";
            msg_auto_exit_timer = ry_timer_create(&timer_para);
            ry_timer_start(msg_auto_exit_timer,  1000, msg_auto_timeout_handler, NULL);
        }
    }else{
        
        delete_msg_flag = NEED_DELETE_MSG;
    }

    if(msg_scrollbar == NULL){
        msg_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);    
    }

    if(app_notify_get(select_func) == NULL){
        wms_activity_back(NULL);
    }else{
    
        ret = show_new_msg(select_func, motar_enable);
        
        if (get_gui_state() != GUI_STATE_ON) {
            msg_ctx_v.screen_onOff = GUI_STATE_OFF;
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
        }
        else{
            msg_ctx_v.screen_onOff = GUI_STATE_ON;
        }
    }

    return ret;
}

ry_sts_t show_set_msg(u32_t y)
{
    message_info_t * disp_msg = app_notify_get_last();

    int font_width = font_roboto_20.width;    
    int font_height = font_roboto_20.height;

    u32_t start_y = y;
    u32_t line_char = (120)/font_width;
    u32_t screen_char = (240 - CONTENT_START_POSTION_Y - y)/(LINE_SPACING + font_height) *line_char;
    u32_t content_offset = 0;
    char * new_content = NULL;
    u32_t title_offset = 0;
    char * new_title = NULL;
    char * img_name = NULL;
    u8_t image_width, image_height;
    u32_t content_str_len;
    u32_t title_str_len;
    
    /*gdispImage * myImage = (gdispImage *)ry_malloc(sizeof(gdispImage));
    if(myImage == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }*/
	clear_buffer_black();

    if (disp_msg->type == Notification_Type_TELEPHONY) {
        img_name = "m_notice_00_phone.bmp";
    } else if (disp_msg->type == Notification_Type_SMS) {
        img_name = "m_notice_01_mail.bmp";
    } else if (disp_msg->type == Notification_Type_APP_MESSAGE) {
        img_name = "m_notice_02_info.bmp";
    }
	
    /*gdispImageOpenFile(myImage, img_name);
    gdispImageDraw(myImage,                    \
                0,  \
                0,     \
                myImage->width,               \
                myImage->height,              \
                0,                           \
                0);
    gdispImageClose(myImage);*/
    img_exflash_to_framebuffer((u8_t*)img_name, 0, 0, &image_width, &image_height, d_justify_left);


    title_str_len = getStringCharCount((char*)disp_msg->title);
    if(title_str_len > line_char){
        title_offset = getCharOffset((char*)disp_msg->title, 3);
    }else{
        title_offset = disp_msg->title_len;
    }

    new_title = (char *)ry_malloc(title_offset + 1);
    if(new_title == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }

    content_str_len = getStringCharCount((char*)disp_msg->data);
    if(content_str_len > screen_char){
        content_offset = getCharOffset((char*)disp_msg->data, screen_char);
        beenShowData = content_offset;
    }else{
        content_offset = disp_msg->data_len;
    }
    cur_show_data = content_offset;
    new_content = (char *)ry_malloc(content_offset + 1);
    if(new_content == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }

    ry_memset(new_title, 0 , title_offset + 1);
    ry_memcpy(new_title, disp_msg->title, title_offset);
    ry_memset(new_content, 0 , content_offset + 1);
    ry_memcpy(new_content, disp_msg->data, content_offset);
    //clear_buffer_black();
    gdispFillString(0,
                    TITLE_START_POSTION_Y,
                    new_title,
                    (void*)font_roboto_20.font,
                    White,
                    Black);
    
    gdispFillStringBox( CONTENT_START_POSTION_X,
                        CONTENT_START_POSTION_Y,
                        RM_OLED_WIDTH - CONTENT_START_POSTION_X,
                        RM_OLED_HEIGHT - CONTENT_START_POSTION_Y,
                        new_content,
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyWordWrap|justifyTop);    

exit:
    ry_free(new_content);
    ry_free(new_title);
    //ry_free(myImage);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    return RY_SUCC;
}

u32_t phone_num_roll_timer = 0;
int roll_offset = 0;


void roll_tele_num(u32_t select_msg, int disp_offset)
{
    int font_width = font_roboto_20.width;
    int font_height = font_roboto_20.height;
    
    message_info_t * disp_msg = app_notify_get(TELE_MSG_NUM);
    char * img_name = NULL;
    u8_t image_width, image_height, i;
    u32_t line_char = (120)/font_width;
    u32_t title_offset = 0;
    img_pos_t pos;
    u32_t title_str_len = 0;
    char * title_str = NULL;
    //LOG_DEBUG("[%s]----%d\n", __FUNCTION__, __LINE__);
    if(msg_auto_exit_timer != 0){
        ry_timer_stop(msg_auto_exit_timer);
    }

    if(disp_msg->title != NULL){
        title_str_len = getStringCharCount((char*)disp_msg->title);
        title_str = (char *)disp_msg->title;
    }
    else{
        title_str_len = getStringCharCount((char*)disp_msg->data);
        title_str = (char *)disp_msg->data;
    }
    
    if(title_str_len > line_char){
        title_offset = getCharOffset(title_str, line_char);
    }
    else{
        //title_offset = disp_msg->data_len;
        if(disp_msg->title != NULL){
            title_offset = disp_msg->title_len;
        }
        else{
            title_offset = disp_msg->data_len;
        }
    }

    if(*(disp_msg->app_id) == Telephony_Status_RINGING_ANSWERABLE){

        setRect(120, PHONE_NUM_ANSWERABLE_Y + font_height, 0, PHONE_NUM_ANSWERABLE_Y, Black);
        //drawStringLine(0 - disp_offset, PHONE_NUM_ANSWERABLE_Y, 0, font_height, title_str, White, Black);
        gdispFillString(0 - disp_offset, PHONE_NUM_ANSWERABLE_Y, title_str, font_roboto_20.font, White, Black);

        pos.buffer = &frame_buffer[PHONE_NUM_ANSWERABLE_Y *3*120];
        pos.x_start = 0;
        pos.x_end = 119;
        pos.y_start = PHONE_NUM_ANSWERABLE_Y;
        pos.y_end = PHONE_NUM_ANSWERABLE_Y + MSG_TITLE_FONT_HEIGHT;
        if(strcmp(wms_activity_get_cur_name(), msg_activity.name) == 0){
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
        }
        //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
    else if(*(disp_msg->app_id) == Telephony_Status_RINGING_UNANSWERABLE){
    
        setRect(120, PHONE_NUM_UNANSWERABLE_Y + font_roboto_32.height, 0, PHONE_NUM_UNANSWERABLE_Y, Black);

        if(string_is_num(title_str) == false){
            //drawStringLine(0 - disp_offset, PHONE_NUM_UNANSWERABLE_Y,0, font_height, title_str, White, Black);
            gdispFillString(0 - disp_offset, PHONE_NUM_ANSWERABLE_Y, title_str, font_roboto_20.font, White, Black);
        } else {
            gdispFillStringBox( 0 - disp_offset,
                            PHONE_NUM_UNANSWERABLE_Y,
                            180,
                            font_roboto_32.height,
                            title_str,
                            (void*)font_roboto_32.font,
                            White,
                            Black,
                            justifyLeft);
        }

        pos.buffer = &frame_buffer[PHONE_NUM_UNANSWERABLE_Y *3*120];
        pos.x_start = 0;
        pos.x_end = 119;
        pos.y_start = PHONE_NUM_UNANSWERABLE_Y;
        pos.y_end = PHONE_NUM_UNANSWERABLE_Y + font_roboto_32.height;
        if(strcmp(wms_activity_get_cur_name(), msg_activity.name) == 0){
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
        }
        //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
    
    //LOG_DEBUG("[%s]----%d\n", __FUNCTION__, __LINE__);    
}


void phone_num_roll_handler(void* arg)
{
    if(strcmp(wms_activity_get_cur_name(), msg_activity.name) != 0){
        if(phone_num_roll_timer != 0){
            ry_timer_stop(phone_num_roll_timer);
        }
        return;
    }
    roll_offset = (roll_offset + 10);
    roll_tele_num(msg_show_num, roll_offset);
    if(roll_offset == RM_OLED_WIDTH + 90){
        roll_offset = -120;
    }    
    
    if(phone_num_roll_timer != 0){
        ry_timer_stop(phone_num_roll_timer);
        ry_timer_start(phone_num_roll_timer,  ROLL_SPEED, phone_num_roll_handler, NULL);
    }
}


void start_phone_num_roll(char *num_str)
{
    if(phone_num_roll_timer != 0){
        ry_timer_stop(phone_num_roll_timer);
        ry_timer_start(phone_num_roll_timer,  ROLL_SPEED, phone_num_roll_handler, NULL);
    }else{
        ry_timer_parm timer_para;
        timer_para.timeout_CB = phone_num_roll_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "num_roll";
        phone_num_roll_timer = ry_timer_create(&timer_para);
        ry_timer_start(phone_num_roll_timer,  ROLL_SPEED, phone_num_roll_handler, NULL);
    }
}
ry_sts_t show_tele_info(u32_t select_msg, u32_t disp_offset)
{
    message_info_t * disp_msg = app_notify_get(select_msg);
    show_drop_top(disp_msg);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    return RY_SUCC;
}

ry_sts_t show_tele_msg(u32_t select_msg, u32_t disp_offset)
{
    int font_height = font_roboto_20.height;
    
    message_info_t * disp_msg = app_notify_get(select_msg);
    char * img_name = NULL;
    u8_t image_width, image_height, i;
    char * new_title = NULL;
    u32_t line_char = ROLL_TEXT_LENGTH;
    u32_t title_offset = 0;
    u32_t on_call_str_offset = 0;
    msg_activity.disableTouchMotar = 1;
    u32_t title_str_len = 0;
    char * title_str = NULL;
    //LOG_DEBUG("[%s]----%d\n", __FUNCTION__, __LINE__);
    if(msg_auto_exit_timer != 0){
        ry_timer_stop(msg_auto_exit_timer);
    }

    if(disp_msg->title != NULL){
        title_str_len = getStringCharCount((char*)disp_msg->title);
        title_str = (char *)disp_msg->title;
    }else{
        title_str_len = getStringCharCount((char*)disp_msg->data);
        title_str = (char *)disp_msg->data;
    }
    
    if(title_str_len > line_char){
        title_offset = getCharOffset(title_str, line_char);
        //start_phone_num_roll(title_str);
    }else{
        //title_offset = disp_msg->data_len;
        if(disp_msg->title != NULL){
            title_offset = disp_msg->title_len;
        }else{
            title_offset = disp_msg->data_len;
        }
    }

    new_title = (char *)ry_malloc(title_offset + 1);
    if(new_title == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }
    tele_on_calling_flag = ON_CALLING;
    ry_memset(new_title, 0 , title_offset + 1);
    ry_memcpy(new_title, title_str, title_offset);

    /*gdispFillStringBox(60,                    \
                        93,    \
                        0,             \
                        MSG_TITLE_FONT_HEIGHT,            \
                        title_str,        \
                        0,               \
                        White,                   \
                        Black|(1<<30),                   \
                        justifyCenter);*/

    if(*(disp_msg->app_id) == Telephony_Status_RINGING_ANSWERABLE){
        DEV_STATISTICS(incoming_count);
        img_exflash_to_framebuffer(TELE_ANSWER_ICON_NAME, 24, 2, &image_width, &image_height, d_justify_left);
        img_exflash_to_framebuffer(TELE_HANGUP_ICON_NAME, 24, 167, &image_width, &image_height, d_justify_left);

        /*gdispFillStringBox(60,                    \
                        93,    \
                        0,             \
                        MSG_TITLE_FONT_HEIGHT,            \
                        title_str,        \
                        0,               \
                        White,                   \
                        Black|(1<<30),                   \
                        justifyCenter);*/

        //drawStringLine(0 - disp_offset, PHONE_NUM_ANSWERABLE_Y,0, MSG_TITLE_FONT_HEIGHT,title_str,White, Black);


        on_call_str_offset = 130;

        gdispFillStringBox(0, 130,
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)on_call_str,
                            (void*)font_roboto_20.font,
                            ON_CALL_STR_COLOR,
                            Black|(1<<30),
                            justifyCenter);

        if(disp_offset == 0){
            motar_set_work(MOTAR_TYPE_STRONG_LOOP, 200);
        }
        if(title_str_len > line_char){
            //drawStringLine(0 - disp_offset, PHONE_NUM_ANSWERABLE_Y,0, font_height, title_str, White, Black);
            gdispFillString(0 - disp_offset, PHONE_NUM_ANSWERABLE_Y, title_str, font_roboto_20.font, White, Black);
            start_phone_num_roll(title_str);
        }else{
            gdispFillStringBox(0, 93,
                                font_roboto_20.width,
                                font_roboto_20.height,
                                title_str,
                                (void*)font_roboto_20.font,
                                White,
                                Black|(1<<30),
                                justifyCenter);
        }
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }else if(*(disp_msg->app_id) == Telephony_Status_RINGING_UNANSWERABLE){
        DEV_STATISTICS(incoming_count);
        img_exflash_to_framebuffer(TELE_HANGUP_ICON1_NAME, 20, 134, &image_width, &image_height, d_justify_left);

        /*gdispFillStringBox(60,                    \
                        32,    \
                        0,             \
                        MSG_TITLE_FONT_HEIGHT,            \
                        title_str,        \
                        0,               \
                        White,                   \
                        Black|(1<<30),                   \
                        justifyCenter);*/

        //drawStringLine(0 - disp_offset, PHONE_NUM_UNANSWERABLE_Y,0, MSG_TITLE_FONT_HEIGHT,title_str,White, Black);

        on_call_str_offset = 89;

        gdispFillStringBox(0, 89,
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char *)on_call_str,
                            (void *)font_roboto_20.font,
                            ON_CALL_STR_COLOR,
                            Black|(1<<30),
                            justifyCenter);

        if(disp_offset == 0){
            motar_set_work(MOTAR_TYPE_STRONG_LOOP, 200);
        }
        if(title_str_len > line_char){
            //drawStringLine(0 - disp_offset, PHONE_NUM_UNANSWERABLE_Y, 0, font_height, title_str, White, Black);
            gdispFillString(0 - disp_offset, PHONE_NUM_UNANSWERABLE_Y, title_str, font_roboto_20.font, White, Black);
            start_phone_num_roll(title_str);
        }else{
            gdispFillStringBox(0, PHONE_NUM_UNANSWERABLE_Y,
                                font_roboto_20.width,
                                font_roboto_20.height,
                                title_str,
                                (void *)font_roboto_20.font,
                                White,
                                Black|(1<<30),
                                justifyCenter);
        }
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }else{
        
        /*u32_t msg_num = app_notify_num_get();
        
        for(i = 0; i < msg_num; ){
            disp_msg = app_notify_get(i);
            if(disp_msg->type == Notification_Type_TELEPHONY){
                app_notify_delete(i);
                i = 0;
                msg_num = app_notify_num_get();
                continue;
            }
            i++;
        }*/

        if(*(disp_msg->app_id) == Telephony_Status_CONNECTED){
            for(i = select_msg + 1; i < app_notify_num_get(); i++){
                if(app_notify_get(i)->type == Notification_Type_TELEPHONY){
                    if(strcmp((const char*)app_notify_get(i)->data, (const char*)disp_msg->data) == 0){
                        app_notify_delete(i);
                        break;
                    }else if(strcmp("CallRemoved", (const char*)disp_msg->data) == 0){
                        app_notify_delete(i);
                        break;
                    }
                }
            }            
        }

        if((ry_ble_get_peer_phone_type() == PEER_PHONE_TYPE_IOS)&&  
            ((*(disp_msg->app_id) == Telephony_Status_DISCONNECTED))){
            //app_notify_delete(select_msg);
        } else {
            app_notify_delete(select_msg);
        }

        wms_activity_back(NULL);
        motar_stop();
    }   
    
exit:
    ry_free(new_title);
    LOG_DEBUG("[%s]----%d\n", __FUNCTION__, __LINE__);
		
    return RY_SUCC;
}

ry_sts_t show_new_msg(u32_t select_msg,u32_t motar_enable)
{
    int font_width = font_roboto_20.width;
    int font_height = font_roboto_20.height;
    
    message_info_t * disp_msg = app_notify_get(select_msg);
    msg_show_num = select_msg;
    u32_t line_char = (120)/font_width;
    u32_t screen_char = (240 -CONTENT_START_POSTION_Y)/(LINE_SPACING + font_height) *line_char;
    u32_t content_offset = 0;
    char * new_content = NULL;
    u32_t title_offset = 0;
    char * new_title = NULL;
    char * img_name = NULL;
    u8_t image_width, image_height, i;
    u8_t weekday;
    u32_t content_str_len;
    u32_t screen_width = (240 - CONTENT_START_POSTION_Y)/(LINE_SPACING + font_height) * RM_OLED_WIDTH;

    init_msg_show_info(disp_msg, &msg_show_info);

    msg_activity.disableTouchMotar = 0;
    /*gdispImage * myImage = (gdispImage *)ry_malloc(sizeof(gdispImage));
    if(myImage == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }*/
	clear_buffer_black();
    
    if(disp_msg->type == Notification_Type_TELEPHONY){
        img_name = APP_TELE_IMG;
        if(NO_NEED_DELETE_MSG == delete_msg_flag){
            return show_tele_msg(select_msg, 0);
        }else if(NEED_DELETE_MSG == delete_msg_flag){
            return show_tele_info(select_msg, 0);
        }
        
    }else if(disp_msg->type == Notification_Type_SMS){
        if(NO_NEED_DELETE_MSG == delete_msg_flag){
            if((motar_enable)||(get_gui_state() == GUI_STATE_OFF)){
                if(is_repeat_notify_open()){
                    motar_set_work(MOTAR_TYPE_STRONG_TWICE,200);    
                }
            }
        }
        img_name = APP_SMS_IMG;
    }else if(disp_msg->type == Notification_Type_APP_MESSAGE){
        if(NO_NEED_DELETE_MSG == delete_msg_flag){
            if((motar_enable)||(get_gui_state() == GUI_STATE_OFF)){
                if(is_repeat_notify_open()){
                    motar_set_work(MOTAR_TYPE_STRONG_TWICE, 200);      
                }
            }
        }
        img_name = APP_DEFAULT_IMG;
        for(i = 0; i < sizeof(app_img_array)/sizeof(app_key_and_img_t); i++){
            if(COMPARE_APP_ID((const char*)disp_msg->app_id, app_img_array[i].key)){
                img_name = app_img_array[i].img;
                break;
            }
        }
    }

    /*gdispImageOpenFile(myImage, img_name);
    gdispImageDraw(myImage,                    \
                0,  \
                0,     \
                myImage->width,               \
                myImage->height,              \
                0,                           \
                0);
    gdispImageClose(myImage);*/
    //img_exflash_to_framebuffer(AS_ICON_IMG_NAME, 0, 0, &image_width, &image_height, d_justify_left);
    img_exflash_to_framebuffer((uint8_t*)img_name, 0, 0, &image_width, &image_height, d_justify_left);

    u8_t mon, d, h, min, s;
    u16_t year;
    ry_utc_parse(disp_msg->time, &year, &mon, &d, &weekday, &h, &min, &s);

    char * time_str = (char *)ry_malloc(40);
    sprintf(time_str, "%02d:%02d",h, min);

    /*gdispFillStringBox(TITLE_TIME_OFFSET_X,                    \
                        TITLE_TIME_OFFSET_Y,    \
                        font_roboto_20.width,             \
                        font_roboto_20.height,            \
                        time_str,        \
                        (void *)font_roboto_20.font,               \
                        White,                   \
                        Black|(1<<30),                   \
                        justifyLeft);*/
    gdispFillString(TITLE_TIME_OFFSET_X, TITLE_TIME_OFFSET_Y, (char*)time_str, font_roboto_20.font, Grey, Black);
		
    ry_free(time_str);

    u32_t title_str_len = getStringCharCount((char*)disp_msg->title);
    u32_t title_width = getStringWidthInnerFont((char*)disp_msg->title, font_roboto_20.font);
    if (title_width > RM_OLED_WIDTH) {
        if (title_str_len > line_char) {
            //title_offset = getCharOffset(disp_msg->title, TITLE_MAX_LEN - 1);
            title_offset = getWidthOffsetInnerFont((char*)disp_msg->title, MSG_TITLE_MAX_LEN - 1, font_roboto_20.font);
        } else {
            title_offset = disp_msg->title_len;
        }
    } else {        
        title_offset = disp_msg->title_len;
    }

    new_title = (char *)ry_malloc(title_offset + 10);
    if(new_title == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }

    content_str_len = getStringCharCount((char*)disp_msg->data);
    if (content_str_len > screen_char) {
        content_offset = getWidthOffsetInnerFont((char*)disp_msg->data,
                                                    screen_width,
                                                    font_roboto_20.font);
        beenShowData = content_offset;
    } else {
        content_offset = disp_msg->data_len;
    }
    cur_show_data = content_offset;
    new_content = (char *)ry_malloc(content_offset + 1);
    if(new_content == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }
        
    if(title_width > RM_OLED_WIDTH){
        ry_memset(new_title, 0 , title_offset + 1);
        ry_memcpy(new_title, disp_msg->title, title_offset);
        sprintf(new_title, "%s..", new_title);
    }else{
        ry_memset(new_title, 0 , title_offset + 1);
        ry_memcpy(new_title, disp_msg->title, title_offset);
    }
    ry_memset(new_content, 0 , content_offset + 1);
    ry_memcpy(new_content, disp_msg->data, content_offset);
    //clear_buffer_black();
    gdispFillString(0, TITLE_START_POSTION_Y,
                    new_title,
                    (void*)font_roboto_20.font,
                    TITLE_COLOR,
                    Black);
    
    gdispFillStringBox(CONTENT_START_POSTION_X,
                        CONTENT_START_POSTION_Y,
                        RM_OLED_WIDTH - CONTENT_START_POSTION_X,
                        RM_OLED_HEIGHT - CONTENT_START_POSTION_Y,
                        new_content,
                        (void*)font_roboto_20.font,
                        White, Black,
                        justifyWordWrap|justifyTop);

    //u32_t content_str_width = getStringWidth(disp_msg->data);

    if(1 >= msg_show_info.screen_num){
    
    }else{
        /*u32_t screen_num = 1 + (content_str_width - MSG_FRIST_SCREEN_LEN)/MSG_DEFAULT_SCREEN_LEN+1;
        u32_t cur_str_width = getStringWidth(&disp_msg->data[beenShowData]);
        u32_t cur_sreen_num = screen_num - (cur_str_width/MSG_DEFAULT_SCREEN_LEN+1);*/
        u32_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)/(msg_show_info.screen_num- 1);
        if(msg_scrollbar != NULL){
            scrollbar_show(msg_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-((msg_show_info.screen_num - msg_show_info.cur_screen -1) * sp_posix));
        }
    }
    
exit:
    ry_free(new_content);
    ry_free(new_title);
    //ry_free(myImage);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    return RY_SUCC;
}

void show_next_screen()
{
    message_info_t * disp_msg = app_notify_get(msg_show_num);
    
    int font_width = font_roboto_20.width;    
    int font_height = font_roboto_20.height;
    
    u32_t line_char = 120/font_width;
    u32_t screen_char = (240 -0)/(LINE_SPACING + font_height) *line_char;
    u32_t content_offset = 0;
    char * new_content = NULL;
    u32_t title_offset = 0;
    char * new_title = NULL;
    u32_t screen_width = (240 -0)/(LINE_SPACING + font_height) * RM_OLED_WIDTH;

    /*if(beenShowData >= disp_msg->data_len - 1 ){
        wms_activity_back(NULL);
        return ;
    }*/
    
    
    /*u32_t content_str_len = getStringCharCount((char*)&(disp_msg->data[beenShowData]));
		
		if (content_str_len < 35 && beenShowData == 0) {
		    return;
		}
		
    if(content_str_len > screen_char){
        content_offset = getWidthOffset(&(disp_msg->data[beenShowData]), screen_width);
        //beenShowData = content_offset;
    }else{
        content_offset = disp_msg->data_len - beenShowData;
    }
    cur_show_data = content_offset;*/

    if(msg_show_info.cur_screen < (msg_show_info.screen_num -1)){
        msg_show_info.cur_screen++;
    }else{
        //wms_activity_back(NULL);
        return ;
    }
    content_offset = getWidthOffsetInnerFont((char*)&(disp_msg->data[msg_show_info.start_offset[msg_show_info.cur_screen]]),
                                                screen_width, font_roboto_20.font);
    
    new_content = (char *)ry_malloc(content_offset + 1);
    if(new_content == NULL){
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }

    ry_memset(new_content, 0 , content_offset + 1);
    ry_memcpy(new_content, &(disp_msg->data[msg_show_info.start_offset[msg_show_info.cur_screen]]), content_offset);
    beenShowData += content_offset;
    clear_buffer_black();
    gdispFillStringBox( 0, 0,
                        RM_OLED_WIDTH,
                        RM_OLED_HEIGHT,
                        new_content,
                        (void*)font_roboto_20.font,
                        White,
                        Black|(1<<30),
                        justifyWordWrap|justifyTop);

    //u32_t content_str_width = getStringWidth(disp_msg->data);

    if(1 >= msg_show_info.screen_num){
    
    }else{
        /*u32_t screen_num = 1 + (content_str_width - MSG_FRIST_SCREEN_LEN)/MSG_DEFAULT_SCREEN_LEN+1;
        u32_t cur_str_width = getStringWidth(&disp_msg->data[beenShowData]);
        u32_t cur_sreen_num = screen_num - (cur_str_width/MSG_DEFAULT_SCREEN_LEN+1);*/
        u32_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)/(msg_show_info.screen_num- 1);
        if(msg_scrollbar != NULL){
            scrollbar_show(msg_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-((msg_show_info.screen_num - msg_show_info.cur_screen -1) * sp_posix));
        }
    }

exit :

    ry_free(new_content);
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void show_prev_screen(void)
{
    message_info_t * disp_msg = app_notify_get(msg_show_num);
    
    int font_width = font_roboto_20.width;
    int font_height = font_roboto_20.height;
    
    u32_t line_char = (120)/font_width;
    u32_t screen_char = (240 -0)/(LINE_SPACING + font_height) * line_char;
    u32_t content_offset = 0;
    char * new_content = NULL;
    u32_t title_offset = 0;
    char * new_title = NULL;
    u32_t motar_enable = 0;
    u32_t screen_width = (240 -0)/(LINE_SPACING + font_height) * RM_OLED_WIDTH;

    /*if(beenShowData != cur_show_data ){
			//beenShowData = (beenShowData > cur_show_data)?(beenShowData - cur_show_data):(0);
			if(beenShowData > cur_show_data){
			    if((beenShowData - cur_show_data) < screen_char){
			        show_new_msg(0);
			        return;
			    }else{
			        int temp_ptr = beenShowData;
                    getWidthOffset(&(disp_msg->data[beenShowData]), screen_width);
			    
                    beenShowData = (beenShowData - cur_show_data) - screen_char;
					//show_new_msg(msg_show_num);
					//return;
			    }
			}else{
			    if(cur_show_data < screen_char){
                    return;
			    }else{
                    beenShowData = 0;
                    show_new_msg(msg_show_num);
                    return ;
                }
			}
    }else{
        show_new_msg(msg_show_num);
        return;
    }
    
    */
    
    cur_show_data = content_offset;
    if(msg_show_info.cur_screen == 0){
        return ;
    }else if(msg_show_info.cur_screen == 1){
        msg_show_info.cur_screen--;
        show_new_msg(msg_show_num,motar_enable);
        return;
    }else{
        msg_show_info.cur_screen--;    
    }

    content_offset = getWidthOffsetInnerFont((char*)&(disp_msg->data[msg_show_info.start_offset[msg_show_info.cur_screen] ]),
                                                screen_width, font_roboto_20.font);
    
    new_content = (char *)ry_malloc(content_offset +1);
    if(new_content == NULL) {
        LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
        goto exit;
    }

    ry_memset(new_content, 0 , (content_offset +1) );
    ry_memcpy(new_content, &(disp_msg->data[msg_show_info.start_offset[msg_show_info.cur_screen] ]), content_offset);
    //beenShowData += content_offset;
    clear_buffer_black();
    gdispFillStringBox( 0, 0,
                        RM_OLED_WIDTH,
                        RM_OLED_HEIGHT,
                        new_content,
                        (void*)font_roboto_20.font,
                        White,
                        Black|(1<<30),
                        justifyWordWrap|justifyTop);

    //u32_t content_str_width = getStringWidth(disp_msg->data);

    if(1 >= msg_show_info.screen_num){
    
    } else {
        /*u32_t screen_num = 1 + (content_str_width - MSG_FRIST_SCREEN_LEN)/MSG_DEFAULT_SCREEN_LEN+1;
        u32_t cur_str_width = getStringWidth(&disp_msg->data[beenShowData]);
        u32_t cur_sreen_num = screen_num - (cur_str_width/MSG_DEFAULT_SCREEN_LEN+1);*/
        u32_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)/(msg_show_info.screen_num- 1);
        if(msg_scrollbar != NULL){
            scrollbar_show(msg_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-((msg_show_info.screen_num - msg_show_info.cur_screen -1) * sp_posix));
        }
    }

exit :

    ry_free(new_content);    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}


ry_sts_t exitMsgNotify(void)
{
    beenShowData = 0;
    //wms_activity_back(NULL);
    if(delete_msg_flag == NEED_DELETE_MSG){
        app_notify_delete(msg_show_num);
    }
    wms_event_listener_del(WM_EVENT_MESSAGE, &msg_activity);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &msg_activity);
    wms_event_listener_del(WM_EVENT_CARD, &msg_activity);
    wms_event_listener_del(WM_EVENT_BLE, &msg_activity);
    delete_msg_flag = NO_NEED_DELETE_MSG;
    if(phone_num_roll_timer != 0){
        ry_timer_stop(phone_num_roll_timer);
        ry_timer_delete(phone_num_roll_timer);
        phone_num_roll_timer = 0;
    }
    tele_on_calling_flag = 0;
    wms_untouch_timer_start();
    roll_offset = 0;

    if(msg_scrollbar != NULL){
        scrollbar_destroy(msg_scrollbar);
        msg_scrollbar = NULL;
    }

    u8_t i = 0;
    u32_t msg_num = app_notify_num_get();
    message_info_t * disp_msg = NULL;
    /*for(i = 0; i < msg_num; ){
        disp_msg = app_notify_get(i);
        if(disp_msg->type == Notification_Type_TELEPHONY){
            app_notify_delete(i);
            i = 0;
            msg_num = app_notify_num_get();
            continue;
        }
        i++;
    }*/
    
    return RY_SUCC;
}


typedef struct{
    char * new_title;
    char * new_content;
    char * time_str;
    char * img_name;
}drop_info_t;

drop_info_t top_info;
drop_info_t bottom_info;


void clear_drop_info(drop_info_t * drop_info)
{
    if(drop_info == NULL){
        return ;
    }
    ry_free(drop_info->new_title);
    ry_free(drop_info->time_str);
    ry_free(drop_info->new_content);

    ry_memset(drop_info, 0, sizeof(drop_info_t));
}

void show_drop_msg(message_info_t * disp_msg, int y, drop_info_t * drop_info, int status)
{
    int font_width = font_roboto_20.width;
    int font_height = font_roboto_20.height;    
    
    int start_y = y;
    u32_t line_char = 120/font_width;
    u32_t screen_char = DROP_MSG_SCREEN_CHAR;
    u32_t content_offset = 0;
    char * new_content = NULL;
    u32_t title_offset = 0;
    char * new_title = NULL;
    char * img_name = NULL;
    char * time_str = NULL;
    u8_t image_width, image_height, i;
    u8_t weekday;

    u32_t tick1 = ry_hal_clock_time();
    if(status & DROP_SHOW_STATUS_GET){
        if(disp_msg->type == Notification_Type_TELEPHONY){
            img_name = "m_notice_00_phone.bmp";
        }else if(disp_msg->type == Notification_Type_SMS){
            img_name = APP_SMS_IMG;
        }else if(disp_msg->type == Notification_Type_APP_MESSAGE){
            img_name = APP_DEFAULT_IMG;
            for(i = 0; i < sizeof(app_img_array)/sizeof(app_key_and_img_t); i++){
                if(COMPARE_APP_ID((const char*)disp_msg->app_id, app_img_array[i].key)){
                    img_name = app_img_array[i].img;
                    break;
                }
            }
        }
        //img_exflash_to_framebuffer(AS_ICON_IMG_NAME, 0, start_y, &image_width, &image_height, d_justify_left);
        //img_exflash_to_framebuffer(img_name, 0, start_y, &image_width, &image_height, d_justify_left);


        u8_t mon, d, h, min, s;
        u16_t year;
        ry_utc_parse(disp_msg->time, &year, &mon, &d, &weekday, &h, &min, &s);

        char * time_str = (char *)ry_malloc(40);
        sprintf(time_str, "%02d:%02d",h, min);

        /*gdispFillStringBox(TITLE_TIME_OFFSET_X,                    \
                            TITLE_TIME_OFFSET_Y + y,    \
                            font_roboto_20.width,\
                            font_roboto_20.height,\
                            time_str,        \
                            (void *)font_roboto_20.font,               \
                            Grey,                   \
                            Black|(1<<30),                   \
                            justifyLeft);*/

        gdispFillString(TITLE_TIME_OFFSET_X, TITLE_TIME_OFFSET_Y + y, (const char*)time_str, font_roboto_20.font, Grey, Black);

        ry_free(time_str);

        u8_t * title_str = disp_msg->title;
        u16_t  title_len = disp_msg->title_len;
        u8_t * data_str = disp_msg->data;
        u16_t  data_len = disp_msg->data_len;

        if (disp_msg->type == Notification_Type_TELEPHONY) {
            if (title_str == NULL) {
                title_str = disp_msg->data;
                title_len = disp_msg->data_len;
            }
            data_str = TELE_CONTENT_STR;
            data_len = strlen(TELE_CONTENT_STR) + 1;
        }

        u32_t title_str_len = getStringCharCount((char*)title_str);
        u32_t title_width = getStringWidthInnerFont((char*)title_str, font_roboto_20.font);
        if(title_width > RM_OLED_WIDTH){
            if(title_str_len > line_char){
                title_offset = getWidthOffset((char*)title_str, MSG_TITLE_MAX_LEN - 1);
            } else {
                title_offset = title_len;
            }
        } else {            
            title_offset = title_len;
        }

        new_title = (char *)ry_malloc(title_offset + 10);
        u32_t content_str_len = getStringCharCount((char*)data_str);
        if(new_title == NULL){
            LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
            goto exit;
        }
        
        if (content_str_len > screen_char) {
            //content_offset = getCharOffset((char*)data_str, screen_char);
            content_offset = getWidthOffsetInnerFont((char*)data_str, 220, font_roboto_20.font);
            beenShowData = content_offset;
        } else {
            content_offset = data_len;
        }
        cur_show_data = content_offset;
        new_content = (char *)ry_malloc(content_offset + 1);
        if(new_content == NULL){
            LOG_ERROR("[%s]malloc failed \n", __FUNCTION__);
            goto exit;
        }

        if(title_width > RM_OLED_WIDTH){
            ry_memset(new_title, 0 , title_offset + 1);
            ry_memcpy(new_title, title_str, title_offset);
            sprintf(new_title, "%s..", new_title);
        }else{
            ry_memset(new_title, 0 , title_offset + 1);
            ry_memcpy(new_title, title_str, title_offset);
        }
        ry_memset(new_content, 0 , content_offset + 1);
        ry_memcpy(new_content, data_str, content_offset);
    }else{
    
        new_content = (char *)ry_malloc(content_offset + 1);
        ry_memcpy(new_content, drop_info->new_content, content_offset +1);
        
        new_title = (char *)ry_malloc(title_offset + 10);
        ry_memcpy(new_title, drop_info->new_title, title_offset +1);
        
        img_name = drop_info->img_name;
        time_str = (char *)ry_malloc(40);
        ry_memcpy(time_str, drop_info->time_str, 40);
        //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
    }
    //clear_buffer_black();
    if((status & DROP_SHOW_STATUS_SAVE)&&(drop_info != NULL)){

        if(drop_info->new_content == NULL){
            drop_info->new_content = (char *)ry_malloc(content_offset + 1);
        }
        ry_memcpy(drop_info->new_content, new_content, (content_offset + 1));

        if(drop_info->new_title == NULL){
            drop_info->new_title = (char *)ry_malloc(title_offset + 10);
        }
        ry_memcpy(drop_info->new_title, new_title, title_offset);
        
        drop_info->img_name = img_name;

        if(drop_info->time_str == NULL){
            drop_info->time_str = (char *)ry_malloc(40);
        }
        ry_memcpy(drop_info->time_str, time_str, 40);
        //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
        
    }

    if(status & DROP_SHOW_STATUS_SHOW){
        img_exflash_to_framebuffer((uint8_t*)img_name, 0, start_y, &image_width, &image_height, d_justify_left);
        //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );

        if(time_str != NULL){
            ry_free(time_str);
        }

        u8_t mon, d, h, min, s;
        u16_t year;
        ry_utc_parse(disp_msg->time, &year, &mon, &d, &weekday, &h, &min, &s);

        time_str = (char *)ry_malloc(40);
        sprintf(time_str, "%02d:%02d",h, min);  
        gdispFillString(TITLE_TIME_OFFSET_X, TITLE_TIME_OFFSET_Y + y , time_str, font_roboto_20.font, Grey,  Black);
        //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
        gdispFillString(0, TITLE_START_POSTION_Y + start_y,
                        new_title,
                        (void*)font_roboto_20.font,
                        TITLE_COLOR,
                        Black);
        //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
        gdispFillStringBox(CONTENT_START_POSTION_X,
                            CONTENT_START_POSTION_Y + start_y,
                            RM_OLED_WIDTH - CONTENT_START_POSTION_X,
                            font_roboto_20.height * 2,
                            new_content,
                            (void*)font_roboto_20.font,
                            White, Black,
                            justifyWordWrap|justifyTop);
        //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
    }      

exit:
    ry_free(time_str);
    ry_free(new_content);
    ry_free(new_title);
    //ry_free(myImage);    
}

void show_drop_top(message_info_t * disp_msg)
{
    show_drop_msg(disp_msg, DROP_MSG_TOP_START_Y , NULL, DROP_SHOW_STATUS_SHOW | DROP_SHOW_STATUS_GET);
}
void show_drop_bottom(message_info_t * disp_msg)
{
    show_drop_msg(disp_msg, DROP_MSG_BOTTOM_START_Y, NULL, DROP_SHOW_STATUS_SHOW | DROP_SHOW_STATUS_GET);
}

void show_delete_all_msg(int offset)
{
    u8_t w,h;
    int y_offset = offset;

    #if (DELETE_ALL_MSG_BUTTON_BOTTOM == DELETE_ALL_MSG_BUTTON_POSITION)
        y_offset += 120;
    #else
        y_offset += 0;
    #endif
    
    draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"g_delete.bmp",\
                0, 20 + y_offset, &w, &h, d_justify_center);

    gdispFillStringBox(0,
                        72 + y_offset,
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)text_delete,
                        (void*)font_roboto_20.font,
                        White,
                        Black|(1<<30),
                        justifyCenter);
}


ry_sts_t show_drop_msg_menu(int offset, int type)
{
    ry_sts_t status = RY_SUCC;
    img_pos_t pos;
    u32_t msg_num = app_notify_num_get();
    int offset_y = offset;
    u8_t w,h;

    /*if(offset_y > 0){
        offset_y = 0;
    }*/

    if(msg_num <= msg_drop_num){
        msg_drop_num = msg_num - 1;
        return status;
    }
    
    message_info_t * disp_msg = app_notify_get(msg_drop_num);
    if(disp_msg == NULL){
        return RY_SET_STS_VAL(RY_CID_MSG, RY_ERR_READ);
    }

    if(type == SLIDE_TYPE_COVER){
    
        clear_buffer_black();
        int move_frame = 40;
        #if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
        show_drop_bottom(disp_msg);
        
        if(( msg_drop_num + 1) < msg_num){
            disp_msg = app_notify_get( msg_drop_num + 1);
            show_drop_top(disp_msg);
        }else{
            show_delete_all_msg(0);
        }
        #else

        show_drop_top(disp_msg);
        if( msg_drop_num >= 1){
            disp_msg = app_notify_get( msg_drop_num - 1);
            show_drop_bottom(disp_msg);
        }else{
            show_delete_all_msg(0);
        }
        #endif

        if(offset >= 0){
            if((msg_num >= 2)){
                u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (msg_num + 1 - 2);
                if(drop_scrollbar != NULL){
                    scrollbar_show(drop_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-(msg_drop_num * sp_posix));
                }
            }
            ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        }else if(offset > 0){
            
            while(1){
                offset_y = ((offset_y + move_frame) > 0) ? (0) : (offset_y + move_frame);
                pos.buffer = &frame_buffer[(0 - offset_y)*360];
                pos.x_start = 0;
                pos.x_end = 119;
                pos.y_start = 0;
                pos.y_end = RM_OLED_HEIGHT + offset_y;
                if(offset_y >= 0){
                    if((msg_num > 2)){
                        u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (msg_num - 2);
                        if(drop_scrollbar != NULL){
                            scrollbar_show(drop_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-(msg_drop_num * sp_posix));
                        }
                    }
                }
                if(offset_y == 0){
                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                }else{
                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
                }
                if(offset_y == 0){
                    
                    break;
                }
                ryos_delay_ms(10);
            }
        } else if(offset < 0){
            u32_t tick1 = ry_hal_clock_time();
            u32_t r = ry_hal_irq_disable();
            while(1){
                //LOG_DEBUG("--------start-------\n");
                u32_t tick = ry_hal_clock_time();
                offset_y = ((offset_y + move_frame) > 0) ? (0) : (offset_y + move_frame);
                pos.buffer = &frame_buffer[(0 - offset_y)*360];
                pos.x_start = 0;
                pos.x_end = 119;
                pos.y_start = 0;
                pos.y_end = RM_OLED_HEIGHT + offset_y - 1;
                if(offset_y >= 0){
                    if((msg_num > 2)){
                        u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (msg_num - 2);
                        if(drop_scrollbar != NULL){
                            scrollbar_show(drop_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-(msg_drop_num * sp_posix));
                        }
                    }
                }
                //LOG_DEBUG("offset_y = %d\n", offset_y);
                if(offset_y == 0){
                    if(offset != -240){
                        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                        ryos_delay_ms(20);
                    }else{
                        gdisp_update();
                    }
                }else{
                    if(offset != -240){
                        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, sizeof(img_pos_t), (uint8_t*)&pos);
                        ryos_delay_ms(20);
                    }else{
                        write_buffer_to_oled((img_pos_t*)&pos);
                    }
                }
                
                if(offset_y == 0){
                    //LOG_DEBUG("--------end------- %d\n",ry_hal_calc_ms(ry_hal_clock_time() - tick) );
                    break;
                }
                
                //LOG_DEBUG("--------end------- %d\n",ry_hal_calc_ms(ry_hal_clock_time() - tick) );
            }
            
            //LOG_DEBUG("--------while end------- %d\n",ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
            ry_hal_irq_restore(r);
        }
    } else if(type == SLIDE_TYPE_TRANSLATION) {
        int move_frame = 40;
        u32_t tick1 = ry_hal_clock_time();
        //move_buffer_data_Y(offset_y);
        u32_t buffer_offset = 0;
        clear_frame_area(118, 120, 0, 240);
        u8_t sp_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT) / (msg_num + 1 - 2);
        u8_t target_posix = (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-(msg_drop_num * sp_posix);
        while(1){
            u32_t drop_status = 0;
            if(offset_y < 0 ){
                offset_y += move_frame;

                offset_y = (offset_y > 0)?(0):(offset_y);
                buffer_offset = move_frame;
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
                move_buffer_data_Y(buffer_offset);
                //gdisp_update();
                clear_frame(0, move_frame);
                //gdisp_update();
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
                
                if(bottom_info.img_name == NULL){
                    drop_status = DROP_SHOW_STATUS_GET | DROP_SHOW_STATUS_SHOW |DROP_SHOW_STATUS_SAVE;
                    //drop_status = DROP_SHOW_STATUS_SHOW ;
                }else{
                    drop_status = DROP_SHOW_STATUS_SHOW ;
                }
                //show_drop_msg(disp_msg, DROP_MSG_BOTTOM_START_Y + offset_y, &bottom_info , drop_status);
                //gdisp_update();
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );

                #if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
                
                if(( msg_drop_num + 1) < msg_num){
                    disp_msg = app_notify_get( msg_drop_num + 1);
                    show_drop_msg(disp_msg, DROP_MSG_TOP_START_Y + offset_y, &top_info, drop_status);
                }else{
                    show_delete_all_msg(offset_y);
                }
                #else

                if( msg_drop_num >= 1){
                    disp_msg = app_notify_get( msg_drop_num);
                    show_drop_msg(disp_msg, DROP_MSG_TOP_START_Y + offset_y, &top_info, drop_status);
                }else{
                    show_delete_all_msg(offset_y);
                }

                #endif
                //gdisp_update();
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
                if((msg_num >= 2)){
                    
                    if(drop_scrollbar != NULL){
                        clear_frame_area(118, 120, 0, 240);
                        scrollbar_show(drop_scrollbar, frame_buffer, 
                            target_posix + (offset_y * sp_posix/offset) );
                    }
                }
            }

            if(offset_y > 0){
                offset_y -= move_frame;

                offset_y = (offset_y < 0)?(0):(offset_y);
                buffer_offset = move_frame;
                move_buffer_data_Y( -buffer_offset);
                //gdisp_update();
                clear_frame(239 - move_frame, move_frame + 1);
                //gdisp_update();
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
                if(top_info.img_name == NULL){
                    drop_status = DROP_SHOW_STATUS_GET | DROP_SHOW_STATUS_SHOW |DROP_SHOW_STATUS_SAVE;
                    //drop_status = DROP_SHOW_STATUS_SHOW ;
                }else{
                    drop_status = DROP_SHOW_STATUS_SHOW ;
                }
                disp_msg = app_notify_get( msg_drop_num);

                #if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
                show_drop_msg(disp_msg, DROP_MSG_BOTTOM_START_Y + offset_y, &bottom_info , drop_status);
                #else
                
                 if( msg_drop_num >= 1){
                    disp_msg = app_notify_get( msg_drop_num - 1);
                    show_drop_msg(disp_msg, DROP_MSG_BOTTOM_START_Y + offset_y, &bottom_info, drop_status);
                }else{
                    show_delete_all_msg(offset_y);
                }
                #endif
                
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
                //disp_msg = app_notify_get( msg_drop_num + 1);
                //show_drop_msg(disp_msg, DROP_MSG_TOP_START_Y + offset_y, &top_info, drop_status);
                //LOG_DEBUG("--------%d------- %d\n",__LINE__,ry_hal_calc_ms(ry_hal_clock_time() - tick1) );
                //gdisp_update();
                if((msg_num >= 2)){
                    
                    if(drop_scrollbar != NULL){
                        clear_frame_area(118, 120, 0, 240);
                        scrollbar_show(drop_scrollbar, frame_buffer, 
                            target_posix - (offset_y * sp_posix/offset));
                    }
                }
            }

            ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
            ryos_delay_ms(10);
            /*LOG_DEBUG("--------------------finish----------\n");
            gdisp_update();*/


            if(offset_y == 0){
                clear_buffer_black();
                disp_msg = app_notify_get( msg_drop_num);
                

                #if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
                
                show_drop_bottom(disp_msg);
                if(( msg_drop_num + 1) < msg_num){
                    disp_msg = app_notify_get( msg_drop_num + 1);
                    show_drop_top(disp_msg);
                }else{
                    show_delete_all_msg(0);
                }
                
                #else

                show_drop_top(disp_msg);
                if( msg_drop_num >= 1){
                    disp_msg = app_notify_get( msg_drop_num - 1);
                    show_drop_bottom(disp_msg);
                }else{
                    show_delete_all_msg(0);
                }
                
                #endif
                clear_drop_info(&top_info);
                clear_drop_info(&bottom_info);

                if((msg_num >= 2)){
                    
                    if(drop_scrollbar != NULL){
                        scrollbar_show(drop_scrollbar, frame_buffer, (RM_OLED_HEIGHT - SCROLLBAR_HEIGHT)-(msg_drop_num * sp_posix));
                    }
                }

                ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                break;
            }
        }
    }
    return status;
}

ry_sts_t show_drop(void * userdata)
{
    ry_sts_t status = RY_SUCC;
    u32_t i = 0;
    message_info_t * disp_msg = NULL;
    u32_t msg_num = app_notify_num_get();
    uint8_t w, h;
    int offset = -((int)userdata);
    /*for(i = 0; i < msg_num; ){
        disp_msg = app_notify_get(i);
        if(disp_msg->type == Notification_Type_TELEPHONY){
            app_notify_delete(i);
            i = 0;
            msg_num = app_notify_num_get();
            continue;
        }
        i++;
    }
    
    msg_num = app_notify_num_get();*/    
    
    if(drop_scrollbar == NULL){
        drop_scrollbar = scrollbar_create(SCROLLBAR_WIDTH, SCROLLBAR_HEIGHT, 0x4A4A4A, 1000);
    }
    //wms_event_listener_add(WM_EVENT_MESSAGE, &drop_activity, msg_ui_onMSGEvent);
    wms_event_listener_add(WM_EVENT_MESSAGE, &drop_activity, drop_ui_onMSGEvent);
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &drop_activity, drop_ui_onTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD, &drop_activity, msg_ui_onProcessedCardEvent);
    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

    if(msg_num == 0){
        //wms_activity_back(NULL);
        drop_no_msg();
        
        
        return status;
    }else if(msg_num < 2 ){
    
        clear_buffer_black();

        show_delete_all_msg(0);
        
        disp_msg = app_notify_get_last();

#if (DELETE_ALL_MSG_BUTTON_POSITION == DELETE_ALL_MSG_BUTTON_TOP)
        show_drop_bottom(disp_msg);

#else

        show_drop_top(disp_msg);

#endif

        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        
    }else{
        if(offset > 240){
            offset = 240;
        }else if(offset < -240){
            offset = -240;
        }
        msg_drop_num = 1;
        status = show_drop_msg_menu(offset, SLIDE_TYPE_COVER);
    }

    return status;
}


ry_sts_t exit_drop(void)
{
    ry_sts_t status = RY_SUCC;
    
    msg_drop_num = 0;

    wms_event_listener_del(WM_EVENT_MESSAGE, &drop_activity);
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &drop_activity);
    wms_event_listener_del(WM_EVENT_CARD, &drop_activity);

    if(drop_scrollbar != NULL){
        scrollbar_destroy(drop_scrollbar);
        drop_scrollbar = NULL;
    }
    
    return status;
}



u32_t slideUp(void *param, u32_t len)
{
    u32_t ret = 0;
    
    show_next_screen();
    
    return ret;
}

u32_t slideDown(void *param, u32_t len)
{
    u32_t ret = 0;

    return ret;
}

u32_t touchButton(void *param, u32_t len)
{
    u32_t ret = 0;

    return ret;
}

