#include "ry_font.h"
#include "ry_hal_inc.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "led.h"
#include "am_devices_cy8c4014.h"
#include "app_interface.h"

#include "gui_img.h"
#include "ryos_timer.h"
#include "gui_msg.h"
#include "data_management_service.h"
#include "ry_ble.h"
#include "ry_nfc.h"

#include "device_management_service.h"
#include "ryos.h"
#include "dip.h"
#include "activity_qrcode.h"
#include "qr_encode.h"
#include "ryos.h"
#include "activity_surface.h"
#include <stdio.h>

#include <stdio.h>

/*********************************************************************
 * CONSTANTS
 */ 
#define QR_CODE_DATA        "qrcode.data"

const char* const text_download_app1 = "Скачайте";
const char* const text_download_app2 = "Hey+ App";

/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    u8_t        quick_click;
    u32_t       click_time;
} qrcode_ctx_t;

/*********************************************************************
 * VARIABLES
 */
u32_t qrcode_timer_id;

activity_t activity_qrcode = {
    .name = "qrcode",
    .onCreate = show_qrcode,
    .onDestroy = exit_qrcode,
    .disableUntouchTimer = 1,
    .priority = WM_PRIORITY_L,
    .brightness = 30,
    .disableWristAlg = 1,
};

qrcode_ctx_t qrcode_ctx_v;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void qrcode_timeout_handler(void* arg);

int qrcode_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
#if 1
    if (p->action != TP_PROCESSED_ACTION_TAP) {
        if (qrcode_ctx_v.quick_click == 0){
            qrcode_ctx_v.click_time = ry_hal_clock_time();
        }
        qrcode_ctx_v.quick_click ++;
        if (qrcode_ctx_v.quick_click > 40){
            if (ry_hal_calc_ms(ry_hal_clock_time() - qrcode_ctx_v.click_time) <= 10000){
                add_reset_count(QRCODE_RESTART_COUNT);
                ry_system_reset();
            }
        }
        if (ry_hal_calc_ms(ry_hal_clock_time() - qrcode_ctx_v.click_time) >= 10000){
            qrcode_ctx_v.quick_click = 0;            
        }
    }

    // LOG_DEBUG("[qrcode_onTouchEvent]=========================p->action: %d, qrcode_ctx_v.quick_click:%d, time:%d\r\n", \
               p->action, qrcode_ctx_v.quick_click, ry_hal_calc_ms(ry_hal_clock_time() - qrcode_ctx_v.click_time));
#endif

    //wms_activity_replace(&activity_surface, NULL);
    ry_timer_stop(qrcode_timer_id);
    ry_timer_start(qrcode_timer_id, 30000, qrcode_timeout_handler, NULL);

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &activity_qrcode.brightness);
    ry_led_onoff_with_effect(1, 30);

    u8_t para = RY_BLE_ADV_TYPE_NORMAL;
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_ADV_START, 1, (void*)&para);
    
    return consumed;
}

/*static void setDip(int x, int y, int color)
{
    uint32_t pos = 120 * y * 3 + x * 3;
    frame_buffer[pos]     = (uint8_t) color & 0xff;
    frame_buffer[pos + 1] = (uint8_t) (color>>8) & 0xff;
    frame_buffer[pos + 2] = (uint8_t) (color>>16) & 0xff;
    true_color_to_white_black(frame_buffer + pos);
}

void setRect(int max_x,int max_y, int min_x, int min_y, int color )
{   
    int x,y;
    for(x = min_x; x <= max_x; x++){
        for(y = min_y; y <= max_y; y++){
            setDip(x, y, color);
        }
    }
}*/

int save_qrcode_data(char * str, u8_t* data, int side)
{
    FIL *fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;
    int status = 0;

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        goto exit;
    }
    
    ret = f_open(fp, QR_CODE_DATA, FA_OPEN_ALWAYS | FA_WRITE);
    if(ret != FR_OK){
        LOG_ERROR("open qrcode data failed\n");
        goto exit;
    }else{
        //save qrcode info str
        ret = f_write(fp, str, (strlen(str) + 1), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("write qrcode data failed\n");
            goto exit;
        }

        //save qrcode side
        ret = f_write(fp, &side, sizeof(side), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("write qrcode data failed\n");
            goto exit;
        }
    
        //save qrcode bit data
        ret = f_write(fp, data, QR_MAX_BITDATA, &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("write qrcode data failed\n");
            goto exit;
        }

        f_close(fp);
    }

    ry_free(fp);
    return status;
exit:
    ry_free(fp);
    return 1;
    
}

void display_qrcode(char * str,int pos_y)
{
    u8_t* bitdata = ry_malloc(QR_MAX_BITDATA);
    int side = 0;
    char * test_str = str;
    u32_t i,j,k,a,max_x,max_y,min_x,min_y;
    
    int rect_x_max,rect_x_min,rect_y_max,rect_y_min;
    FIL *fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;
    char * old_str = NULL;

    if(bitdata == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        ry_free(fp);
        ry_free(old_str);
        return ;
    }

    old_str = (char *)ry_malloc(strlen(str) + 1);
    if(old_str == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        ry_free(fp);
        ry_free(old_str);
        return ;
    }
    

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        ry_free(fp);
        ry_free(old_str);
        return ;
    }
    
    ret = f_open(fp, QR_CODE_DATA, FA_READ);// try open qrcode data file

    if(ret != FR_OK){
        // open failed---->encode qrcode and save qrcode data
        side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);
        f_close(fp);
        ry_free(fp);
        fp = NULL;
        save_qrcode_data(str, bitdata, side);
        /*ret = f_open(fp, QR_CODE_DATA, FA_OPEN_ALWAYS | FA_WRITE);
        if(ret != FR_OK){
            LOG_ERROR("open qrcode data failed\n");
        }else{
            ret = f_write(fp, str, (strlen(str) + 1), &written_bytes);
            if(ret != FR_OK){
                LOG_ERROR("write qrcode data failed\n");
            }
        
            ret = f_write(fp, bitdata, QR_MAX_BITDATA, &written_bytes);
            if(ret != FR_OK){
                LOG_ERROR("write qrcode data failed\n");
            }

            f_close(fp);
        }*/
    }else{
        // open success

        //read qrcode info str
        ret = f_read(fp, old_str, (strlen(str) + 1), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("read qrcode data failed\n");
        }

        //get qrcode side
        ret = f_read(fp, &side, sizeof(side), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("write qrcode data failed\n");
            
        }
        if((0 >= side)||(side > 120)){
            ret = 0xff;
        }

        //compare str
        if((strcmp(str, old_str) != 0) ||(ret != FR_OK)){
            //compare failed---->encode qrcode and save qrcode data
            side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);
            f_close(fp);
            ry_free(fp);
            fp = NULL;
            save_qrcode_data(str, bitdata, side);
        }else{
            //compare success
            //read qrcode bit data
            ret = f_read(fp, bitdata, QR_MAX_BITDATA, &written_bytes);
            if(ret != FR_OK){
                //read failed---->encode qrcode and save qrcode data
                LOG_ERROR("read qrcode data failed\n");
                side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);
                f_close(fp);
                ry_free(fp);
                fp = NULL;
                save_qrcode_data(str, bitdata, side);
            }else{
                // read success--->nothing to do
                
            }

        }
    }

    int zoom = 120/side;
    int pos_x = (120 - zoom * side)/2;

    rect_y_min = (pos_y > 6)?(pos_y - 5):(0);
    rect_y_max = side * zoom + pos_y + 4;

    
    rect_x_min = (pos_x > 6)?(pos_x - 5):(0);
    rect_x_max = side * zoom + pos_x + 4;

    setRect(rect_x_max, rect_y_max, rect_x_min, rect_y_min, White);

    for (i = 0; i < side; i++)
    {
        for (j = 0; j < side; j++)
        {
            min_x = j * zoom + pos_x;
            min_y = i * zoom + pos_y;
            max_x = j * zoom + zoom + pos_x;
            max_y = i * zoom + zoom + pos_y;

            a = i * side + j;

            if (bitdata[a / 8] & (1 << (7 - a % 8)))
            {
                setRect(max_x, max_y, min_x, min_y, Black);
            }
            else
            {
                setRect(max_x, max_y, min_x, min_y, White);
            }
        }
    }
    
    ry_free(bitdata);
    ry_free(fp);
    ry_free(old_str);
}

void qr_display_update(void)
{
    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    clear_buffer_black();
    char * qr_code_str = (char * )ry_malloc(100);
    if(qr_code_str == NULL){
        wms_activity_back(NULL);
    }
    char * device_id_str = (char *)ry_malloc(40);
    if(device_id_str == NULL){
        ry_free(qr_code_str);
        wms_activity_back(NULL);
    }
    
    //extern u8_t* device_info_get_mac(void);
    char * mac = (char*)device_info_get_mac();

    sprintf(qr_code_str, "%s?mac=%02x%%3a%02x%%3a%02x%%3a%02x%%3a%02x%%3a%02x&pid=%d", APP_DOWNLOAD_URL, \
                mac[0],\
                mac[1],\
                mac[2],\
                mac[3],\
                mac[4],\
                mac[5],\
                911);

    //sprintf(qr_code_str, "http://t.cn/RuMSKQ4");

    display_qrcode(qr_code_str, 50);
    ry_free(qr_code_str);

	gdispFillStringBox( 0, 
                        4, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)text_download_app1, 
                        (void*)font_roboto_20.font, 
                        White, Black,  
                        justifyCenter
                      );  

    gdispFillStringBox( 0, 
                        160, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_download_app2, 
                        (void*)font_roboto_20.font, 
                        White, Black,  
                        justifyCenter
                      );

    sprintf(device_id_str,"%02X%02X", (mac[4]&0xFF), (mac[5]&0xFF));

    gdispFillStringBox( 0, 
                        190, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        device_id_str, 
                        (void*)font_roboto_20.font, 
                        White, Black,  
                        justifyCenter
                        );
                        
    char battery[16];
    sprintf(battery, "%d%%", sys_get_battery_percent(BATT_DETECT_DIRECTED));

    gdispFillStringBox( 0, 
                        218, 
                        font_roboto_12.width,
                        font_roboto_12.height,
                        (char *)battery, 
                        (void*)font_roboto_12.font, 
                        White, Black,
                        justifyCenter
                        );                        
                        
    ry_free(device_id_str);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &activity_qrcode.brightness);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void create_qrcode(char * str,int pos_y)
{
    u8_t* bitdata = ry_malloc(QR_MAX_BITDATA);
    int side = 0;
    char * test_str = str;
    u32_t i,j,k,a,max_x,max_y,min_x,min_y;
    
    int rect_x_max,rect_x_min,rect_y_max,rect_y_min;
    FIL *fp = NULL;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;
    char * old_str = NULL;

    if(bitdata == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        ry_free(fp);
        ry_free(old_str);
        return ;
    }

    old_str = (char *)ry_malloc(strlen(str) + 1);
    if(old_str == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        ry_free(fp);
        ry_free(old_str);
        return ;
    }
    

    fp = (FIL *)ry_malloc(sizeof(FIL));
    if(fp == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        ry_free(fp);
        ry_free(old_str);
        return ;
    }
    
    ret = f_open(fp, QR_CODE_DATA, FA_READ);// try open qrcode data file

    if(ret != FR_OK){
        // open failed---->encode qrcode and save qrcode data
        side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);
        f_close(fp);
        ry_free(fp);
        fp = NULL;
        save_qrcode_data(str, bitdata, side);
        /*ret = f_open(fp, QR_CODE_DATA, FA_OPEN_ALWAYS | FA_WRITE);
        if(ret != FR_OK){
            LOG_ERROR("open qrcode data failed\n");
        }else{
            ret = f_write(fp, str, (strlen(str) + 1), &written_bytes);
            if(ret != FR_OK){
                LOG_ERROR("write qrcode data failed\n");
            }
        
            ret = f_write(fp, bitdata, QR_MAX_BITDATA, &written_bytes);
            if(ret != FR_OK){
                LOG_ERROR("write qrcode data failed\n");
            }

            f_close(fp);
        }*/
    }else{
        // open success

        //read qrcode info str
        ret = f_read(fp, old_str, (strlen(str) + 1), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("read qrcode data failed\n");
        }

        //get qrcode side
        ret = f_read(fp, &side, sizeof(side), &written_bytes);
        if(ret != FR_OK){
            LOG_ERROR("write qrcode data failed\n");
            
        }
        if((0 >= side)||(side > 120)){
            ret = 0xff;
        }

        //compare str
        if((strcmp(str, old_str) != 0) ||(ret != FR_OK)){
            //compare failed---->encode qrcode and save qrcode data
            side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);
            f_close(fp);
            ry_free(fp);
            fp = NULL;
            save_qrcode_data(str, bitdata, side);
        }else{
            //compare success
            //read qrcode bit data
            ret = f_read(fp, bitdata, QR_MAX_BITDATA, &written_bytes);
            if(ret != FR_OK){
                //read failed---->encode qrcode and save qrcode data
                LOG_ERROR("read qrcode data failed\n");
                side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);
                f_close(fp);
                ry_free(fp);
                fp = NULL;
                save_qrcode_data(str, bitdata, side);
            }else{
                // read success--->nothing to do
                
            }

        }
    }

    
    
    ry_free(bitdata);
    ry_free(fp);
    ry_free(old_str);
}

void create_qrcode_bitmap(void)
{
    char * qr_code_str = (char * )ry_malloc(100);
    if(qr_code_str == NULL){
    }
    
    //extern u8_t* device_info_get_mac(void);
    char * mac = (char*)device_info_get_mac();

    sprintf(qr_code_str, "%s?mac=%02x%%3a%02x%%3a%02x%%3a%02x%%3a%02x%%3a%02x&pid=%d", APP_DOWNLOAD_URL, \
                mac[0],\
                mac[1],\
                mac[2],\
                mac[3],\
                mac[4],\
                mac[5],\
                911);

    //sprintf(qr_code_str, "http://t.cn/RuMSKQ4");


    create_qrcode(qr_code_str, 46);
    ry_free(qr_code_str);
}


int qrcode_onSysEvent(ry_ipc_msg_t* msg)
{
    LOG_DEBUG("[qrcode_onSysEvent]\n");
    
    if (msg->msgType == IPC_MSG_TYPE_BIND_SUCC) {
        show_qrcode(NULL);
    } else if (msg->msgType == IPC_MSG_TYPE_BIND_START) {
        ry_timer_stop(qrcode_timer_id);
    }
    
    return 0;
}


void qrcode_timeout_handler(void* arg)
{
    LOG_DEBUG("[qrcode_timer]: transfer timeout. Reset hci\n");
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_OFF, 0, NULL);
    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_RESET, 0, NULL);
    ry_led_onoff_with_effect(0, 0);   
    ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
}

void qrcode_timer_stop(void)
{
    if (qrcode_timer_id) {
        LOG_DEBUG("[qrcode_timer] stopped\n");
        ry_timer_stop(qrcode_timer_id);
    }
}

ry_sts_t show_qrcode(void * usrdata)
{
    ry_sts_t ret = RY_SUCC;

    if(QR_CODE_DEBUG || is_bond()){
        wms_activity_replace(&activity_surface, NULL);
    }
    else{
        wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_qrcode, qrcode_onTouchEvent);
        wms_event_listener_add(WM_EVENT_SYS, &activity_qrcode, qrcode_onSysEvent);
        
        if (qrcode_timer_id == 0){
            ry_timer_parm timer_para;
            /* Create the timer */
            timer_para.timeout_CB = qrcode_timeout_handler;
            timer_para.flag = 0;
            timer_para.data = NULL;
            timer_para.tick = 1;
            timer_para.name = "qrcode_timer";
            qrcode_timer_id = ry_timer_create(&timer_para);
        }
        ry_timer_start(qrcode_timer_id, 30000, qrcode_timeout_handler, NULL);
        qr_display_update();
    }
    return ret;
}


ry_sts_t exit_qrcode(void)
{
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_qrcode);
    wms_event_listener_del(WM_EVENT_SYS, &activity_qrcode);
    
    return RY_SUCC;
}
