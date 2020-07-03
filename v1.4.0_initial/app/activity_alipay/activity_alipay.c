#include "activity_alipay.h"
#include "ry_barcode_code128.h"
#include "qr_encode.h"
#include "activity_card.h"
#include "alipay.h"
#include "alipay_common.h"
#include "alipay_api.h"

#include "wsf_types.h"
#include "wsf_os_int.h"
#include "scrollbar.h"
#include "gui_bare.h"
#include "timer_management_service.h"



#define MAX_BARCODE_WIDTH       220

const char* const text_alipay_bind[] = {
		"Привяжите",
		"телефон",
		"к Alipay"
};

const char* const text_alipay_agreement[] = {
		"Прочитайте",
		"соглашение",
		"о платежах"
};

const char* const text_alipay_unbind[] = {
		"Отвязавшить",
		"вы не сможете",
		"платить"
};

const char* const text_scan_code = "Сканировать";
const char* const text_unbind = "Отвязать";
const char* const text_bind_success = "Выполнена";
const char* const text_bind_failure = "Не удалась";

const char* const text_alipay_bind_qrcode[] = {
	"Используйте",
	"qr-код Alipay",
	"для привязки"
};

const char* const text_alipay_bind_status[] = {
	"10%",
	"30%",
	"70%",
	"90%",
};


typedef struct code128Data{
    u8_t len;
    int * data;
}code128Data_t;



typedef struct {
    u8_t is_bond;
    alipay_bind_status_e bind_status;
    alipay_code_type_e code_type;
    
    u32_t alipay_status_timer_id;
    u32_t alipay_status_timer_count;
    u32_t alipay_refresh_timer_count;
    scrollbar_t* scrollbar;
}alipay_ctx_t;


alipay_ctx_t alipay_ctx_v = {0};



activity_t activity_alipay = {
    .name = "alipay", 
    .which_app =NULL, 
    .usrdata = NULL, 
    .onCreate = create_alipay, 
    .onDestroy = Destroy_alipay,
    .priority = WM_PRIORITY_G,
    .disableWristAlg = 1,
    .disableUntouchTimer = 1,
    .brightness = 50,
};

const uint8_t PRODUCT_MODEL[19] = {"#Ryeex_B1800"};


static void display_alipay_bind_success(void);
static void display_alipay_bind_qrcode(void);
static void display_alipay_unbind_confirm(void);
static void display_alipay_bind_start(void);
static void alipay_bind_status_handle(binding_status_e bind_status);


void exit_binding(void)
{
    alipay_env_deinit();
    LOG_DEBUG("after deinit ,status is %02X\n", alipay_get_binding_status());

//    alipay_unregister_service();
    AlipaySendEventThroughBleThread(ALIPAY_SERVICE_REMOVE);

//    Alipay_service_adv_close();
}





int get_alipay_bind_status(void)
{
    if(alipay_get_binding_status() == STATUS_BINDING_OK){
        return 1;
    } else{
        return 0;
    }
}


void alipay_status_loop(void *userData)
{
    alipay_ctx_v.alipay_status_timer_count++;
    binding_status_e bind_status = alipay_get_binding_status();
    int event  = 0;

    if(bind_status != STATUS_START_BINDING){
        int status = (int)bind_status;
    
        wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(status));
    }

    if(get_alipay_bind_status()){

        if(alipay_ctx_v.code_type < ALIPAY_TYPE_UNBIND ){

            if(alipay_ctx_v.alipay_status_timer_count > 50){
                event = (int)ALIPAY_ACTV_EVT_REFRESH_CODE;

                wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(event));
                alipay_ctx_v.alipay_status_timer_count = 0;
            }
        }

        ry_timer_start(alipay_ctx_v.alipay_status_timer_id, 1000, alipay_status_loop, NULL);
        
    }else{
        if(alipay_ctx_v.alipay_status_timer_count > 60){
            event = (int)ALIPAY_ACTV_EVT_TIMEOUT;
    
            wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(event));
        }else if(alipay_ctx_v.alipay_status_timer_id){
            ry_timer_start(alipay_ctx_v.alipay_status_timer_id, 1000, alipay_status_loop, NULL);
        }

    }

    //LOG_DEBUG("alipay bind status : %d\n", get_alipay_bind_status());
}


void alipay_refresh_loop(void *userData)
{
    alipay_ctx_v.alipay_status_timer_count++;
    alipay_ctx_v.alipay_refresh_timer_count++;
    int event  = 0;


    if(get_alipay_bind_status()){

        if(alipay_ctx_v.code_type < ALIPAY_TYPE_UNBIND ){

            if(alipay_ctx_v.alipay_refresh_timer_count > ALIPAY_PAYCODE_REFRESH_SECONED){
                event = (int)ALIPAY_ACTV_EVT_REFRESH_CODE;

                wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(event));
                alipay_ctx_v.alipay_refresh_timer_count = 0;
            }

            if(alipay_ctx_v.alipay_status_timer_count > ALIPAY_PAYCODE_TIMEOUT_SECONED){
                event = (int)ALIPAY_ACTV_EVT_TIMEOUT;

                wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(event));
                alipay_ctx_v.alipay_status_timer_count = 0;
            }
        }

        if(alipay_ctx_v.alipay_status_timer_id){
            ry_timer_start(alipay_ctx_v.alipay_status_timer_id, 1000, alipay_refresh_loop, NULL);
        }
        
    }else{
        
        
    }

    //LOG_DEBUG("alipay bind status : %d\n", get_alipay_bind_status());
}

static void clear_alipay_timeout_count(void)
{
    alipay_ctx_v.alipay_status_timer_count = 0;
}


static void clear_alipay_refresh_count(void)
{
    alipay_ctx_v.alipay_refresh_timer_count = 0;
}


int alipay_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    clear_alipay_timeout_count();

    switch(p->action){
        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            if(get_alipay_bind_status()){
                /*if(alipay_ctx_v.code_type == ALIPAY_TYPE_BARCODE){
                    alipay_ctx_v.code_type = ALIPAY_TYPE_QRCODE;
                }else if(alipay_ctx_v.code_type == ALIPAY_TYPE_QRCODE){
                    alipay_ctx_v.code_type = ALIPAY_TYPE_BARCODE;
                }*/

                if(alipay_ctx_v.code_type > ALIPAY_TYPE_BARCODE){
                    alipay_ctx_v.code_type = (alipay_ctx_v.code_type+ALIPAY_TYPE_MAX-1)%ALIPAY_TYPE_MAX;
                    clear_alipay_refresh_count();
                    clear_buffer_black();
                    display_pay_code();
                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                }
            }else{
                
            }
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:
            if(get_alipay_bind_status()){
                /*if(alipay_ctx_v.code_type == ALIPAY_TYPE_BARCODE){
                    alipay_ctx_v.code_type = ALIPAY_TYPE_QRCODE;
                }else if(alipay_ctx_v.code_type == ALIPAY_TYPE_QRCODE){
                    alipay_ctx_v.code_type = ALIPAY_TYPE_BARCODE;
                }*/

                if(alipay_ctx_v.code_type < ALIPAY_TYPE_UNBIND){
                    alipay_ctx_v.code_type = (alipay_ctx_v.code_type+1)%ALIPAY_TYPE_MAX;
                    clear_alipay_refresh_count();
                    clear_buffer_black();
                    display_pay_code();
                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                }
            }else{
                
            }
            break;
            
        case TP_PROCESSED_ACTION_TAP:
            {

                switch (p->y){

                    case 1:
                    case 2:
                    case 3:
                        {
                            if(get_alipay_bind_status()){
                                
                            }else{
                                
                            }
                        }
                        break;

                    case 5:
                    case 6:
                    case 7:
                        {
                            if(get_alipay_bind_status()){

                                if(alipay_ctx_v.code_type == ALIPAY_TYPE_UNBIND){
                                    alipay_ctx_v.code_type = ALIPAY_TYPE_UNBIND_CONFIRM;
                                    clear_buffer_black();
                                    display_alipay_unbind_confirm();
                                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                                }else if(alipay_ctx_v.code_type == ALIPAY_TYPE_UNBIND_CONFIRM){
                                    alipay_unbinding();
                                    wms_activity_back(NULL);
                                }
                            }else{
                                if(alipay_ctx_v.bind_status == ALIPAY_BIND_PROTOCOL){
                                    alipay_ctx_v.bind_status = ALIPAY_BIND_START;
                                    clear_buffer_black();
                                    display_alipay_bind_start();
                                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                                }else if(alipay_ctx_v.bind_status == ALIPAY_BIND_START){
                                    alipay_ctx_v.bind_status = ALIPAY_BIND_QRCODE;
                                    clear_buffer_black();
                                    display_alipay_bind_qrcode();
                                    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
                                }
                            }
                        }
                        break;

                    case 8:
                    case 9:
                        {
                            if(get_alipay_bind_status() == 0){
                                if(alipay_ctx_v.bind_status > ALIPAY_BIND_START){
                                    exit_binding();
                                }
                            
                            }
                            wms_activity_back(NULL);
                        }
                        break;                    
                }
            }
            break;
    }      
    return 0;
}


void alipay_activity_evt_handle(int event)
{
    if((event > ALIPAY_ACTV_EVT_END)||(event < ALIPAY_ACTV_EVT_START)){
        return ;
    }
    alipay_activity_event_e actv_evt = (alipay_activity_event_e)event;

    if(actv_evt == ALIPAY_ACTV_EVT_TIMEOUT){
        if(get_alipay_bind_status() == 0){
            exit_binding();
        }
        wms_activity_back(NULL);
    }else if(actv_evt == ALIPAY_ACTV_EVT_BIND_FINISH){
        clear_buffer_black();
        display_pay_code();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }else if(ALIPAY_ACTV_EVT_REFRESH_CODE == actv_evt){
        clear_buffer_black();
        display_pay_code();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }
    
}


int alipay_AlipayEvent(ry_ipc_msg_t* msg)
{
    if(msg->len != sizeof(int)){
        
        return 0;
    }

    int status = *((int *)(msg->data));
    if(ALIPAY_ACTV_EVT_START > status){
        alipay_bind_status_handle((binding_status_e)status);

    }else{

        alipay_activity_evt_handle(status);
    }

    return 0;
}

void create_alipay_qrcode(char * str,int pos_y)
{
    u8_t* bitdata = ry_malloc(QR_MAX_BITDATA);
    int side = 0;
    char * test_str = str;
    u32_t i,j,k,a,max_x,max_y,min_x,min_y;
    
    int rect_x_max,rect_x_min,rect_y_max,rect_y_min;
    FRESULT ret = FR_OK;
    u32_t written_bytes = 0;

    if(bitdata == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        ry_free(bitdata);
        return ;
    }

        // open failed---->encode qrcode and save qrcode data
    side = qr_encode(QR_LEVEL_L, 0, test_str, strlen(test_str), bitdata);

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
}

static void display_alipay_bind_start(void)
{
    FRESULT res = FR_OK;
    u8_t w, h;
    gdispFillStringBox(0, 24, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_bind[0],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    gdispFillStringBox(0, 54, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_bind[1],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    gdispFillStringBox(0, 84, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_bind[2],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);

    res = img_exflash_to_framebuffer((u8_t*)"alipay_bind.bmp", \
                            60, 134, &w, &h, d_justify_center);
    
    if(res != FR_OK){
        img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    }
}


void display_alipay_protocol(void)
{
    FRESULT res = FR_OK;
    u8_t w, h;
    gdispFillStringBox( 2,
                        20,
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_agreement[0],        
                        (void*)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);

    gdispFillStringBox( 0,                    
                        50,    
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_agreement[1],        
                        (void*)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);
    gdispFillStringBox( 0,                    
                        80,    
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_agreement[2],        
                        (void*)font_roboto_20.font,               
                        White, Black,
                        justifyCenter);
    
    res = img_exflash_to_framebuffer((u8_t*)"alipay_agree.bmp", \
                            60, 134, &w, &h, d_justify_center);
    
    if(res != FR_OK){
        img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    }
}


int data_atoi(char * data, int len)
{
    char temp_buf[10] = {0};
#if 0
    
    temp_buf[0] = '0';
    temp_buf[1] = ''

#else
    if(len > 10){
        return 0;
    }

    ry_memcpy(temp_buf, data, len);
    
    return (int )strtol(temp_buf, NULL, 16);
#endif

}


void set_alipay_qrcode_bind_info(uint8_t * device_info)
{
    int i = 0;
    int count = 0;

    char temp_buf[10] = {0};
    int temp_data = 0;
    if(device_info == NULL){
        goto exit;
    }

    if(strlen((char*)device_info) >= ALIPAY_DEVICE_INFO_SIZE_MAX){
        goto exit;
    }
    
    for(i = 0; i < ALIPAY_DEVICE_INFO_SIZE_MAX; i++){
        if(device_info[i] == '\0'){
            break;
        }

        if(device_info[i] == ':'){
            count++;
        }

        if(count == 3){
            temp_data = data_atoi((char*)&device_info[i+1], 2);
            sprintf(temp_buf, "%02x", temp_data);
            ry_memcpy(&device_info[i
+1], temp_buf, 2);
        }
    }


exit:

    return ;
}


void binding_test(void)
{
    uint8_t device_info[ALIPAY_DEVICE_INFO_SIZE_MAX] = {0};
    uint32_t lenth_info = 0;
    int pre_status, cur_status;
    
    LOG_DEBUG("\r\nStart binding.\r\n");
    
    /*if(alipay_get_binding_status() == 0xff){
        ry_hal_flash_write(0xf7000,0x4000);
        alipay_env_deinit();
        int i = 0;
        u8_t * temp = (u8_t*)ry_malloc(0x1000);
        ry_memset(temp, 0 ,0x1000);
        for(i = 0; i < 7; i++){
            ALIPAY_storage_delete(i);
        }
        for(i = 0; i < 7; i++){
            ry_hal_flash_write(0xE7000+i*0x1000,temp, 0x1000);
        }

        ry_free(temp);
    }*/

    if(alipay_ctx_v.alipay_status_timer_id == 0){

        ry_timer_parm timer_para;
        timer_para.timeout_CB = NULL;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "alipay_timer";
        alipay_ctx_v.alipay_status_timer_id = ry_timer_create(&timer_para);
    }
    alipay_ctx_v.alipay_status_timer_count = 0;
    ry_timer_start(alipay_ctx_v.alipay_status_timer_id, 1000, alipay_status_loop, NULL);
    
    alipay_unbinding();
    pre_status = alipay_get_binding_status();
    LOG_DEBUG("After init ,status is %02X\n", pre_status);

    AlipaySendEventThroughBleThread(ALIPAY_SERVICE_ADD);
//    alipay_register_service(alipay_recv_data_handle);
//    ryeex_Alipay_service_adv_open();
    alipay_env_init();
    
    
    
    pre_status = alipay_get_binding_status();
    LOG_DEBUG("After init ,status is %02X\n", pre_status);

    alipay_get_binding_string(device_info, &lenth_info);
    LOG_DEBUG("device_info[%d]: %s\n", lenth_info, device_info);

   /* set_alipay_qrcode_bind_info(device_info);

    LOG_DEBUG("device_info[%d]: %s\n", lenth_info, device_info);*/

    //create_alipay_qrcode((char *)"ALIPAY_WATCH#2#0C:F3:EE:00:00:01#Ambiq_W100#2.1.0", 120);
    //create_alipay_qrcode((char *)"ALIPAY_WATCH#2#0C:F3:EE:21:04:4A#Ambiq_W100#2.1.0", 120);
    create_alipay_qrcode((char *)device_info, 120);

    /*side = qr_encode(QR_LEVEL_L, 0, (const char *)device_info, lenth_info, bitdata);
    LOG_DEBUG("Qrcode side length:%d\n", side);

    alipay_showQR(bitdata, side, 90, 50, 5);*/
    //ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    /*int i = 0;
    while (1)
    {
        i++;
        cur_status = alipay_get_binding_status();

        ryos_delay_ms(600);
        LOG_DEBUG("cur_status:%d\n", cur_status);
        if(i > 200){
            break;
        }

        if(cur_status == STATUS_BINDING_OK){
            break;
        }else if(cur_status == STATUS_BINDING_FAIL){
            break;
        }
    }*/

    
    //exit_binding();
}

static void display_alipay_unbind_confirm(void)
{
    FRESULT res = FR_OK;
    u8_t w, h;
    gdispFillStringBox( 0, 30, font_roboto_20.width, font_roboto_20.height,       
                        (char*)text_alipay_unbind[0],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    gdispFillStringBox( 0, 60, font_roboto_20.width, font_roboto_20.height, 
                        (char*)text_alipay_unbind[1],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    gdispFillStringBox( 0, 90, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_unbind[2],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    
    res = img_exflash_to_framebuffer((u8_t*)"alipay_OK.bmp", \
                            60, 134, &w, &h, d_justify_center);
    
    if(res != FR_OK){
        img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                60, 134, &w, &h, d_justify_center);
    }
}

void display_bind_success(void)
{
    u8_t w, h;
    img_exflash_to_framebuffer((u8_t*)"g_status_00_succ.bmp", 24, 54, &w, &h, d_justify_left);
    gdispFillStringBox(0, 176, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_bind_success,
                        (void*)font_roboto_20.font,
                        White, Black|(1<<30), justifyCenter);
}

void display_bind_fail(void)
{
    u8_t w, h;
    img_exflash_to_framebuffer((u8_t*)"g_status_01_fail.bmp", 24, 54, &w, &h, d_justify_left);
    gdispFillStringBox(0, 176, font_roboto_20.width, font_roboto_20.height,
                        (char *)text_bind_failure,
                        (void*)font_roboto_20.font,
                        White, Black|(1<<30), justifyCenter);
}

void display_alipay_bind_qrcode(void)
{

    if (STATUS_BINDING_OK != alipay_get_binding_status()){
        binding_test();
    }
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &activity_alipay.brightness);
    gdispFillStringBox( 0, 24, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_bind_qrcode[0],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    gdispFillStringBox( 0, 54, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_bind_qrcode[1],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);
    gdispFillStringBox( 0, 84, font_roboto_20.width, font_roboto_20.height,
                        (char*)text_alipay_bind_qrcode[2],        
                        (void*)font_roboto_20.font, White, Black, justifyCenter);

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
												
    //create_alipay_qrcode("binding", 120);
}


static void alipay_bind_status_handle(binding_status_e bind_status)
{
    set_frame(0, 115, 0);
    if(bind_status == STATUS_GETTING_PROFILE) {
        gdispFillStringBox( 0, 24, font_roboto_20.width, font_roboto_20.height,
                            (char*)text_alipay_bind_status[0],        
                            (void*)font_roboto_20.font,               
                            White, Black, justifyCenter);

    } else if(bind_status == STATUS_SAVING_DATA) {
        gdispFillStringBox( 0,                    
                            24,    
                            font_roboto_20.width, font_roboto_20.height,  
                            (char*)text_alipay_bind_status[1],        
                            (void*)font_roboto_20.font,               
                            White, Black, justifyCenter);

    } else if(bind_status == STATUS_SAVING_DATA_OK) {
        gdispFillStringBox( 0,                    
                            24,    
                            font_roboto_20.width, font_roboto_20.height,
                            (char*)text_alipay_bind_status[2],        
                            (void*)font_roboto_20.font,               
                            White, Black, justifyCenter);
                            
    } else if(bind_status == STATUS_FINISH_BINDING) {
        gdispFillStringBox( 0,                    
                            24,    
                            font_roboto_20.width, font_roboto_20.height,
                            (char*)text_alipay_bind_status[3],        
                            (void*)font_roboto_20.font,
                            White, Black, justifyCenter);
                            
    } else if(bind_status == STATUS_BINDING_OK) {
			
        motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 200);
        clear_buffer_black();
        //display_pay_code();
        display_bind_success();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        exit_binding();
        ry_timer_start(alipay_ctx_v.alipay_status_timer_id, 1000, alipay_refresh_loop, NULL);

        int event = (int)ALIPAY_ACTV_EVT_BIND_FINISH;
        ryos_delay_ms(2000);
        wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(event));
			
    } else if(bind_status == STATUS_BINDING_FAIL) {
        motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 200);
        clear_buffer_black();
        display_bind_fail();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        exit_binding();

        int event = (int)ALIPAY_ACTV_EVT_TIMEOUT;
        ryos_delay_ms(2000);
        wms_msg_send(IPC_MSG_TYPE_ALIPAY_EVENT, sizeof(int), (u8_t *)&(event));        
    }

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);    
}


void display_alipay_bind_success(void)
{
    gdispFillStringBox( 0,                    
                        156,    
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)text_bind_success,        
                        (void*)font_roboto_20.font,               
                        White, Black, justifyCenter);
}


#define CODE128_WHITE       0x0
bool Code128isBitBlack(uint8_t* p_bits_buffer, uint32_t bit_index)
{
    bool isWhite = (p_bits_buffer[bit_index] == CODE128_WHITE); //used in code128.c
    return !isWhite;
}

void code128_draw_line_with_y(uint16_t y, uint32_t color)
{
    for(int i=5;i<111;i++){
        int x = i;
        gui_bare_set_pixel_color(x,y, color);
    }
}

void code128_set_default_bg_color(uint32_t fgcolor, uint32_t bgcolor)
{
    for(int x=0;x<115;x++){
        for(int y=0;y<240;y++){
            gui_bare_set_pixel_color(x,y,fgcolor);
        }
    }
    for(int x=115;x<120;x++){
        for(int y=0;y<240;y++){
            gui_bare_set_pixel_color(x,y,bgcolor);
        }
    }
}

//static uint8_t render_disp_buff[256];

void create_code128(char * code_str )
{
    const uint8_t default_white_color = 0xC0;  //白色亮度
    const uint32_t default_bg_white_color = 0xC0C0C0;
    const uint8_t reserved_white_bits = 0;     //两侧留白bit

    int8_t* bits_buffer = (int8_t*)ry_malloc(256+reserved_white_bits*2);
    ry_memset(bits_buffer, CODE128_WHITE, 256+reserved_white_bits*2); //reset with white 

    size_t result_size = code128_encode_gs1((int8_t*)code_str, &bits_buffer[reserved_white_bits], 256);
    int brightness = CODE128_BRIGHTNESS_NORMAL;

    result_size += reserved_white_bits*2;

    uint8_t* bit_render_buffer = (uint8_t*)ry_malloc(240*16);
    ry_memset(bit_render_buffer, default_white_color, 240*16);   //reset white

    const uint8_t brightness_Black = 0x00;
    const uint8_t brightness_white = default_white_color;

    code128_set_default_bg_color(default_bg_white_color, Black);


    uint32_t pixel_per_bit = 240*16/result_size;
    uint16_t index_raw_bits;
    for(index_raw_bits = 0;index_raw_bits < result_size; index_raw_bits++) {
        uint32_t weight = 0;
        if(Code128isBitBlack((uint8_t*)bits_buffer, index_raw_bits)) {
            weight = brightness_Black;
        } else {
            weight = brightness_white;
        }
        for(int j=0;j<pixel_per_bit;j++){
            bit_render_buffer[index_raw_bits*pixel_per_bit+j] = weight;
        }
    }
    // fill rest to white
    for(int i=index_raw_bits*pixel_per_bit;i<240*16;i++) {
        bit_render_buffer[i] = brightness_white;
    }
    ry_free(bits_buffer);

    for(int i =0;i<240;i++) {
        int sum = 0;
        for(int j=0;j<16;j++){
            sum += bit_render_buffer[i*16+j];
        }
        sum /= 16;
        sum = (sum&0xFF);
//        render_disp_buff[i] = sum;
        uint32_t color = (sum << 16) 
                | (sum << 8)
                | (sum << 0);
        code128_draw_line_with_y(i, color);
    }

    ry_free(bit_render_buffer);

    return ;
}




void create_alipay_code128( u8_t * code_data )
{
    //char * test_str = "2824 1963 6204 1453233";
    create_code128((char *)code_data);
}


void create_alipay_unbind(void)
{
    FRESULT res = FR_OK;
    uint8_t w,h;
    uint32_t log_id_len = 25;
    uint32_t nick_name_len = 30;
    uint8_t * log_id = (uint8_t *)ry_malloc(log_id_len);

    uint8_t * nick_name = (uint8_t *)ry_malloc(nick_name_len);

    ry_memset(nick_name, 0, nick_name_len);
    ry_memset(log_id, 0, log_id_len);

    gdispFillStringBox(0,
                        24,
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)text_unbind,
                        (void*)font_roboto_20.font,
                        White, Black, justifyCenter);

    log_id_len--;
    nick_name_len--;
    
    alipay_get_logon_ID(log_id, &log_id_len);

    alipay_get_nick_name(nick_name, &nick_name_len);

    gdispFillStringBox(0,                    \
                        54,
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)log_id,
                        (void*)font_roboto_20.font,
                        White, Black, justifyCenter);

    gdispFillStringBox(0,
                        100,
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)nick_name,
                        (void*)font_roboto_20.font,
                        White, Black, justifyCenter);

    res = img_exflash_to_framebuffer((u8_t*)"alipay_ubind.bmp", \
                                60, 134, &w, &h, d_justify_center);

    if( res != FR_OK ){
        img_exflash_to_framebuffer((u8_t*)"g_widget_00_enter.bmp", \
                                    60, 134, &w, &h, d_justify_center);
    }
}


void display_pay_code(void)
{

    u8_t * buf_code = (u8_t *)ry_malloc(20);
    u32_t len_code = 20;

    retval_t ret = alipay_get_paycode(buf_code, &len_code);
//    strcpy(buf_code, "287118253150692995");

    LOG_DEBUG("pay code[%d]: %s \n", len_code, buf_code);

    if(ret != RV_OK){
        LOG_ERROR("pay code error\n");
    }
    
    uint8_t brightness = 0;
    
    if (alipay_ctx_v.code_type == ALIPAY_TYPE_BARCODE) {
        brightness = ALIPAY_BARCODE_BRIGHTNESS;
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);
        create_alipay_code128(buf_code);
        
    } else if(alipay_ctx_v.code_type == ALIPAY_TYPE_QRCODE) {
        brightness = ALIPAY_QRCODE_BRIGHTNESS;
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);
        create_alipay_qrcode((char *)buf_code, 20);
        
    gdispFillStringBox(0,
                        204,
                        font_roboto_20.width, font_roboto_20.height,
                        (char*)text_scan_code,
                        (void*)font_roboto_20.font,
                        White, Black, justifyCenter);
                        
    } else if(alipay_ctx_v.code_type == ALIPAY_TYPE_UNBIND){
        create_alipay_unbind();
    }

    int startY = alipay_ctx_v.code_type*(240 - ALIPAY_SCROLLBAR_LEN)/(ALIPAY_TYPE_MAX-1);
    scrollbar_show(alipay_ctx_v.scrollbar, frame_buffer, startY);
        
    ry_free(buf_code);
}


int alipay_onMSGEvent(ry_ipc_msg_t* msg)
{
    wms_activity_jump(&msg_activity, NULL);

    return 0;
}

int alipay_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}


ry_sts_t create_alipay(void *userdata)
{
    ry_sts_t status = RY_SUCC;
    
    alipay_ctx_v.code_type = ALIPAY_TYPE_BARCODE;

    alipay_ctx_v.scrollbar = scrollbar_create(1, ALIPAY_SCROLLBAR_LEN, 0x4A4A4A, 300);

    if(alipay_ctx_v.scrollbar == NULL){
        LOG_ERROR("[%s]-scrollbar fail\n", __FUNCTION__);
        return status;
    }
    
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_alipay, alipay_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_ALIPAY, &activity_alipay, alipay_AlipayEvent);
    clear_buffer_black();
    if(get_alipay_bind_status()){
        if(alipay_ctx_v.alipay_status_timer_id == 0){

            ry_timer_parm timer_para;
            timer_para.timeout_CB = NULL;
            timer_para.flag = 0;
            timer_para.data = NULL;
            timer_para.tick = 1;
            timer_para.name = "alipay_timer";
            alipay_ctx_v.alipay_status_timer_id = ry_timer_create(&timer_para);
        }
        alipay_ctx_v.alipay_status_timer_count = 0;
        alipay_ctx_v.alipay_refresh_timer_count = 0;
        ry_timer_start(alipay_ctx_v.alipay_status_timer_id, 1000, alipay_refresh_loop, NULL);
        display_pay_code();
    } else {
        alipay_ctx_v.bind_status = ALIPAY_BIND_PROTOCOL;
        display_alipay_protocol();        
    }

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    return status; 
}


ry_sts_t Destroy_alipay(void)
{
    ry_sts_t status = RY_SUCC;
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_alipay);
    wms_event_listener_del(WM_EVENT_ALIPAY, &activity_alipay);
    //wms_event_listener_del(WM_EVENT_MESSAGE, &activity_alipay);

    scrollbar_destroy(alipay_ctx_v.scrollbar);

    uint8_t brightness = tms_oled_max_brightness_percent();
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SET_BRIGHTNESS, 1, &brightness);

    if(alipay_ctx_v.alipay_status_timer_id != 0){
        status = ry_timer_stop(alipay_ctx_v.alipay_status_timer_id);
        status = ry_timer_delete(alipay_ctx_v.alipay_status_timer_id);
        alipay_ctx_v.alipay_status_timer_id = 0;
    }

    ry_memset(&alipay_ctx_v, 0, sizeof(alipay_ctx_v));
    return status; 
}
