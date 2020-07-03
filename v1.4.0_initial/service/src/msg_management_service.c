#include "msg_management_service.h"
#include "string.h"
#include "ff.h"
#include "Notification.pb.h"
#include "rbp.pb.h"
#include "rbp.h"
#include "ryos.h"
#include "ry_utils.h"
#include "pb.h"
#include "nanopb_common.h"
#include "scheduler.h"
#include "ry_statistics.h"
#include "ancs_api.h"

#include "window_management_service.h"
#include "device_management_service.h"
#include "timer_management_service.h"
#include "storage_management_service.h"


ryos_mutex_t    *notify_mutex = NULL;   //lock for message_info and message_manage_info safety


message_info_t message_info[MSG_BUF_NUM];

message_manage_info_t message_manage_info;

void app_notify_manager_init(void)
{
    
    ry_sts_t status = ryos_mutex_create(&notify_mutex);

    if(status != RY_SUCC){

    }

    

#if STORE_IN_EXFLASH

    FRESULT res ;
    u32_t written_bytes = 0,i;
    FIL * msg_data = NULL;
    FIL * msg_manager = (FIL *)ry_malloc(sizeof(FIL));
    if(msg_manager == NULL){
        LOG_ERROR("[%s] malloc failed\n",__FUNCTION__);
        goto exit;
    }
    
    res = f_open(msg_manager, MSG_MANAGER_FILE_NAME, FA_OPEN_EXISTING | FA_READ);

    if (res == FR_OK) {

        if( f_size(msg_manager) == 0 ){

        }else{
            res = f_read( msg_manager, &message_manage_info, sizeof(message_manage_info), &written_bytes);
            if (res != FR_OK) {
                goto exit;
            }

            res = f_read( msg_manager, &message_info, (sizeof(message_info_t) * message_manage_info.cur_num), &written_bytes);
            if (res != FR_OK) {
                goto exit;
            }

            res = f_close(msg_manager);
            if (res != FR_OK) {
                goto exit;
            }

            msg_data = (FIL *)ry_malloc(sizeof(FIL));
            if(msg_manager == NULL){
                LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
                goto exit;
            }
            
            res = f_open(msg_data, MSG_DATA_FILE_NAME, FA_OPEN_EXISTING | FA_READ);
            if (res != FR_OK) {
                goto exit;
            }

            for(i = 0; i < message_manage_info.cur_num; i++){
                
                message_info[i].title = (u8_t *)ry_malloc(message_info[i].title_len);
                message_info[i].data = (u8_t *)ry_malloc(message_info[i].data_len);

                res = f_read(msg_data, (message_info[i].title), message_info[i].title_len, &written_bytes);
                if (res != FR_OK) {
                    goto exit;
                }

                res = f_read(msg_data, (message_info[i].data), message_info[i].data_len, &written_bytes);
                if (res != FR_OK) {
                    goto exit;
                }
                
            }

            res = f_close(msg_data);
            if (res != FR_OK) {
                goto exit;
            }
            
        }
        
    }else{
        if (res != FR_OK) {
            goto exit;
        }

    }

#endif

exit:

#if STORE_IN_EXFLASH

    app_notify_clear_all();

#endif
    ry_memset(&message_info, 0, sizeof(message_info));
    ry_memset(&message_manage_info, 0, sizeof(message_manage_info));

    ry_free(msg_manager);
    ry_free(msg_data);
    return;
}

bool is_app_notify_empty(void)
{
     return (message_manage_info.cur_num)?(false):(true);
}

bool is_app_notify_full(void)
{
    return (message_manage_info.cur_num == MAX_MSG_NUM)?(true):(false);
}


static void app_notify_free_with_index(uint32_t index)
{
    if(index >= MSG_BUF_NUM)
    {
        LOG_DEBUG("[msg]try free invalid index\n");
        return;
    }
    LOG_DEBUG("[msg]try free with index :%d\n", index);

    message_info_t* p_msg = &message_info[index];

    if(p_msg->data != NULL)
    {
        ry_free(p_msg->data);
        p_msg->data = NULL;
    }
    if(p_msg->title != NULL)
    {
        ry_free(p_msg->title);
        p_msg->title = NULL;
    }

    switch(p_msg->type)
    {
        case Notification_Type_TELEPHONY:
//            break;
        case Notification_Type_SMS:
//            break;
        case Notification_Type_APP_MESSAGE:
            if(p_msg->app_id != NULL)
            {
                ry_free(p_msg->app_id);
                p_msg->app_id = NULL;
            }
            break;
    }
}

//delete and move msg
static void app_notify_delete_msg_and_move(uint32_t idx)
{
    app_notify_free_with_index(idx);

    for(int i = idx; i < MAX_MSG_NUM - 1; i++){
        message_info[i] = message_info[i + 1];
    }
    ry_memset(&message_info[MAX_MSG_NUM - 1], 0, sizeof(message_info_t));

    message_manage_info.cur_num--;
}

void app_notify_clear_all(void)
{
    ryos_mutex_lock(notify_mutex);
    for (int i = 0; i < message_manage_info.cur_num; i++){
        app_notify_free_with_index(i);
    }

    ry_memset(&message_info, 0, sizeof(message_info));
    ry_memset(&message_manage_info, 0, sizeof(message_manage_info));
    ryos_mutex_unlock(notify_mutex);
}

void app_notify_delete(uint32_t select_num)
{
    ryos_mutex_lock(notify_mutex);

    uint32_t num = message_manage_info.cur_num - 1 - select_num;
    if(num >= message_manage_info.cur_num)
    {
        LOG_DEBUG("[msg_manager] err_index");
        goto exit;
    }
    app_notify_delete_msg_and_move(num);
    
exit:
    ryos_mutex_unlock(notify_mutex);
    
}

void app_notify_delete_last(void)
{
    ryos_mutex_lock(notify_mutex);
    uint32_t num = 0;
    app_notify_delete_msg_and_move(num);
    ryos_mutex_unlock(notify_mutex);
}

u32_t app_notify_num_get(void)
{
    return message_manage_info.cur_num;
}

message_info_t * app_notify_get(int num)
{
    if(num != TELE_MSG_NUM){
        if(num >= message_manage_info.cur_num){
            return NULL;
        }
        return &(message_info[message_manage_info.cur_num - 1 - num]);
    }else{
        return &(message_info[num]);
    }
}

message_info_t * app_notify_get_first(void)
{
    return app_notify_get(message_manage_info.cur_num - 1);
}

message_info_t * app_notify_get_last(void)
{
    return app_notify_get(0);
}



void app_notify_add_msg(u16_t type, u32_t time, u8_t* title, u8_t* data ,u8_t *app_id)
{
    if(wms_msg_show_enable_get() == 0){
        if(type == Notification_Type_TELEPHONY){
            if(*app_id == Telephony_Status_DISCONNECTED){
                return;
            }else if(*app_id == Telephony_Status_CONNECTED){
                app_notify_delete(message_manage_info.cur_num - 1 );
                return;
            }
        }
    }

    ryos_mutex_lock(notify_mutex);

    if (message_manage_info.cur_num == (MAX_MSG_NUM) ){
        ryos_mutex_unlock(notify_mutex);
        app_notify_delete_last();
        ryos_mutex_lock(notify_mutex);
    }
    message_info_t* current_msg = &message_info[message_manage_info.cur_num];
    uint32_t length = 0;

    current_msg->type = type;
    current_msg->time = time;

    if(data != NULL)
    {
        length = strlen((char const*)data) + 1;
        current_msg->data_len = length;
        current_msg->data = (u8_t *)ry_malloc(length) ;
        if(current_msg->data == NULL)
        {
            goto exit;
        }
        else
        {
            ry_memcpy(current_msg->data, data, length);
        }
    }
    else
    {
        LOG_DEBUG("[msg_manager] msg no data null\n");
    }

    if(title != NULL)
    {
        length = strlen((char const*)title) + 1;
        current_msg->title_len = length;
        current_msg->title = (u8_t *)ry_malloc(length) ;
        if(current_msg->title == NULL)
        {
            goto exit;
        }
        else
        {
            ry_memcpy(current_msg->title, title, length);
        }
    }
    else
    {
        LOG_DEBUG("[msg_manager] msg no title null\n");
    }

    switch(type)
    {
        case Notification_Type_TELEPHONY:
            if(app_id != NULL)
            {
                length = 1;
                current_msg->app_id = (u8_t *)ry_malloc(length) ;
                if(current_msg->app_id == NULL)
                {
                    goto exit;
                }
                else
                {
                    ry_memcpy(current_msg->app_id, app_id, length);
                    message_info[TELE_MSG_NUM] = message_info[message_manage_info.cur_num];
                }
            }
            break;
        case Notification_Type_SMS:
            // do nothing
            current_msg->app_id = NULL;
            break;
        case Notification_Type_APP_MESSAGE:
            if(app_id != NULL)
            {
                length = strlen((char const*)app_id) + 1;
                current_msg->app_id = (u8_t *)ry_malloc(length) ;
                if(current_msg->app_id == NULL)
                {
                    goto exit;
                }
                else
                {
                    ry_memcpy(current_msg->app_id, app_id, length);
                }
            }
            break;
    }


    message_manage_info.cur_num++;
    DEV_STATISTICS(msg_count);

    ryos_mutex_unlock(notify_mutex);

    if(is_red_point_open()){
        uint32_t msgType = IPC_MSG_TYPE_SURFACE_UPDATE_STEP;
        ry_sched_msg_send(IPC_MSG_TYPE_SURFACE_UPDATE, sizeof(uint32_t), (uint8_t *)&msgType);
    }
    
    return;

exit:
    LOG_ERROR("[msg_manager] malloc failed\n");
    return;
}


void app_notify_store_all(void)
{
#if STORE_IN_EXFLASH

    FRESULT res ;
    u32_t written_bytes = 0,i;
    FIL* msg_manager = NULL, *msg_data = NULL;
    
    if(msg_manager == NULL){
        LOG_ERROR("[%s] malloc failed\n",__FUNCTION__);
        goto exit;
    }
    if(msg_data == NULL){
        LOG_ERROR("[%s] malloc failed\n",__FUNCTION__);
        goto exit;
    }
    
    res = f_open(msg_manager, MSG_MANAGER_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        goto exit;
    }

    res = f_write( msg_manager, &message_manage_info, sizeof(message_manage_info), &written_bytes);
    if (res != FR_OK) {
        goto exit;
    }

    res = f_write( msg_manager, &message_info, (sizeof(message_info_t) * message_manage_info.cur_num), &written_bytes);
    if (res != FR_OK) {
        goto exit;
    }

    res = f_close( msg_manager);
    if (res != FR_OK) {
        goto exit;
    }

    
    res = f_open(msg_data, MSG_DATA_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);

    for(i = 0; i < message_manage_info.cur_num; i++){
        res = f_write( msg_data, &message_info[i].title, message_info[i].title_len, &written_bytes);
        if (res != FR_OK) {
            goto exit;
        }

        res = f_write( msg_data, &message_info[i].data, message_info[i].data_len, &written_bytes);
        if (res != FR_OK) {
            goto exit;
        }
    }
    
    res = f_close( msg_data);
    if (res != FR_OK) {
        goto exit;
    }
    
exit:
    f_unlink(MSG_DATA_FILE_NAME);
    f_unlink(MSG_MANAGER_FILE_NAME);

    ry_free(msg_data);
    ry_free(msg_manager);

#endif
    return ;
}

ry_sts_t app_notify_telephony_notify(u8_t * data, u32_t len)
{
    ry_sts_t status = RY_SUCC;
    Telephony *tele = ry_malloc(sizeof(Telephony));
    Telephony_Status telephony_state;
    u8_t * contact = NULL;
    pb_istream_t stream = pb_istream_from_buffer(data, len);

    if (0 == pb_decode(&stream, Telephony_fields, tele)){
        LOG_ERROR("app_notify_telephony_notify Decoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        //return status;
    }

    telephony_state = tele->status;

    if(tele->has_contact == 1){
        contact = (u8_t *)tele->contact;
    }else{
        contact = NULL;
    }

    app_notify_add_msg(Notification_Type_TELEPHONY, ryos_rtc_now(), contact, (u8_t *)tele->number, (u8_t *)&telephony_state);

    ry_free(tele);

    return status;
}

ry_sts_t app_notify_SMS_notify(u8_t * data, u32_t len)
{
    ry_sts_t status = RY_SUCC;
    SMS * sms = ry_malloc(sizeof(SMS));
    u8_t * sender_info = NULL;

    pb_istream_t stream = pb_istream_from_buffer(data, len);

    if (0 == pb_decode(&stream, SMS_fields, sms)){
        LOG_ERROR("app_notify_SMS_notify Decoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        //return status;
    }

    if(sms->has_contact){
        sender_info = (uint8_t*)sms->contact;
    }else{
        sender_info = (u8_t *)sms->sender;
    }
    
    app_notify_add_msg(Notification_Type_SMS, ryos_rtc_now(), sender_info, (u8_t *)sms->content, NULL);
    
    ry_free(sms);

    return status;
}

ry_sts_t app_notify_app_notify(u8_t * data, u32_t len)
{
    ry_sts_t status = RY_SUCC;
    pb_istream_t stream = pb_istream_from_buffer(data, len);

    APP_MESSAGE * app_notify = ry_malloc(sizeof(APP_MESSAGE));
    if(app_notify == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }


    if (0 == pb_decode(&stream, APP_MESSAGE_fields, app_notify)){
        LOG_ERROR("app_notify_app_notify Decoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        //return status;
    }

    app_notify_add_msg(Notification_Type_APP_MESSAGE, ryos_rtc_now(), (u8_t *)app_notify->title, (u8_t *)app_notify->text, (u8_t*)app_notify->app_id);

    ry_free(app_notify);
exit:
    return status;
}


ry_sts_t notify_req_handle(u8_t * data, u32_t size)
{
    //u8_t *data = msg->message.req.val.bytes;
    //u32_t size = msg->message.req.val.size;
    ry_sts_t status = RY_SUCC;
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    
    Notification * notify = (Notification *)ry_malloc(sizeof(Notification));
    if(notify == NULL){
        LOG_ERROR("[%s] malloc failed\n", __FUNCTION__);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }

    if (0 == pb_decode(&stream, Notification_fields, notify)){
        LOG_ERROR("notify_req_handle Decoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        ry_free(notify);
        return status;
    }

    if(notify->type == Notification_Type_TELEPHONY) {
        status = app_notify_telephony_notify(notify->val.bytes, notify->val.size);
    } else if(notify->type == Notification_Type_SMS) {
        status = app_notify_SMS_notify(notify->val.bytes, notify->val.size);
    } else if(notify->type == Notification_Type_APP_MESSAGE) {
        status = app_notify_app_notify(notify->val.bytes, notify->val.size);
    }
    
    if(status == RY_SUCC)
    {
        if(current_time_is_dnd_status() != 1)
        {
            ry_sched_msg_send(IPC_MSG_TYPE_MM_NEW_NOTIFY, 0, NULL);
        }
        //app_ui_msg_send(IPC_MSG_TYPE_MM_NEW_NOTIFY, 0, NULL);
    }
    
    status = rbp_send_resp(CMD_DEV_NOTIFICATION, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_free(notify);
    return status;

exit:
    rbp_send_resp(CMD_DEV_NOTIFICATION, RBP_RESP_CODE_NO_MEM, NULL, 0, 1);
    return status;
}


ry_sts_t notify_cfg_iOS_write_req_handle(u8_t * data, u32_t size)
{
    ry_sts_t status = RY_SUCC;
    NotificationSetting* p_cfg = NULL;
    uint32_t ret_err_code = RBP_RESP_CODE_SUCCESS;        
    pb_istream_t stream = pb_istream_from_buffer(data, size);

    p_cfg = (NotificationSetting*)ry_malloc(sizeof(NotificationSetting));
    if(p_cfg == NULL)
    {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        ret_err_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    if (0 == pb_decode(&stream, NotificationSetting_fields, p_cfg)){
        LOG_ERROR("[%s]-{%s} %s\n", __FUNCTION__, ERR_STR_DECODE_FAIL, PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);      
        ret_err_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }


    //exec do something
    status = ancsWriteConfig(p_cfg);
    if(0 != status)
    {
        LOG_ERROR("[%s]-{ancs write failed}\n", __FUNCTION__);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_WRITE);      
        ret_err_code = RBP_RESP_CODE_INVALID_PARA;
        goto exit;
    }

    
    status = rbp_send_resp(CMD_DEV_NOTIFICATION_SET_SETTING, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_free(p_cfg);
    return status;
exit:
    rbp_send_resp(CMD_DEV_NOTIFICATION_SET_SETTING, ret_err_code, NULL, 0, 1);
    ry_free(p_cfg);
    return status;
}

static void notify_cfg_iOS_read_req_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC)
    {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    }
    else 
    {
        LOG_DEBUG("[%s] %s, Upload Succ\r\n", __FUNCTION__, (char*)arg);
    }
}

ry_sts_t notify_cfg_iOS_read_req_handle(u8_t * data, u32_t size)
{
    ry_sts_t ret = RY_SUCC;

    NotificationSetting* p_cfg = NULL;
    uint8_t* p_encode_buffer = NULL;  
    pb_ostream_t stream_o = {0};

    p_cfg = (NotificationSetting*)ry_malloc(sizeof(NotificationSetting));
    if(p_cfg == NULL)
    {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }

    ry_memset(p_cfg, 0x00, sizeof(NotificationSetting));
    ancsReadConfig(p_cfg);

    p_encode_buffer = ry_malloc(sizeof(NotificationSetting) + 10);
    if(p_cfg == NULL)
    {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }
    
    stream_o = pb_ostream_from_buffer(p_encode_buffer, sizeof(NotificationSetting) + 10);

    if(!pb_encode(&stream_o, NotificationSetting_fields, p_cfg))
    {
        LOG_ERROR("[%s]-{%s} %s\n", __FUNCTION__, ERR_STR_ENCODE_FAIL, PB_GET_ERROR(&stream_o));

        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_WRITE);
        goto exit;

    }
    
    LOG_DEBUG("[%s] encoded buffer: %d\n", __FUNCTION__, stream_o.bytes_written);

    ry_free(p_cfg);
    ret = ble_send_response(CMD_DEV_NOTIFICATION_GET_SETTING,
        RBP_RESP_CODE_SUCCESS,
        p_encode_buffer,
        stream_o.bytes_written,
        0,
        notify_cfg_iOS_read_req_send_callback,//data_ble_upload_callback, 
        (void*)__FUNCTION__);
    ry_free(p_encode_buffer);
    return ret;

exit:
    ry_free(p_cfg);
    ry_free(p_encode_buffer);
    rbp_send_resp(CMD_DEV_NOTIFICATION_GET_SETTING, RBP_RESP_CODE_NO_MEM, NULL, 0, 1);
    return ret;
}

static void ONDeviceUploadNotification_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC)
    {
        LOG_ERROR("[%s] %s, error status: %x\r\n", __FUNCTION__, (char*)arg, status);
    }
    else 
    {
        LOG_DEBUG("[%s] %s, Upload Succ\r\n", __FUNCTION__, (char*)arg);
    }
}

void OnDeviceUploadNotificationSetting(void)
{
    LOG_DEBUG("[ANCS] OnDeviceUploadNotificationSetting\n");


    if( storage_get_lost_file_status() ){
        return;
    }

    ry_sts_t ret = RY_SUCC;

    NotificationSetting* p_cfg = NULL;
    uint8_t* p_encode_buffer = NULL;
    pb_ostream_t stream_o = {0};

    p_cfg = (NotificationSetting*)ry_malloc(sizeof(NotificationSetting));
    if(p_cfg == NULL) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }

    ry_memset(p_cfg, 0x00, sizeof(NotificationSetting));
    ancsReadConfig(p_cfg);

    p_encode_buffer = ry_malloc(sizeof(NotificationSetting) + 10);
    if(p_cfg == NULL) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }
    
    stream_o = pb_ostream_from_buffer(p_encode_buffer, sizeof(NotificationSetting) + 10);

    if(!pb_encode(&stream_o, NotificationSetting_fields, p_cfg)) {
        LOG_ERROR("[%s]-{%s} %s\n", __FUNCTION__, ERR_STR_ENCODE_FAIL, PB_GET_ERROR(&stream_o));

        ret = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_WRITE);
        goto exit;

    }
    
    LOG_DEBUG("[%s] encoded buffer: %d\n", __FUNCTION__, stream_o.bytes_written);

    ry_free(p_cfg);
    ret = ble_send_request(CMD_APP_UPLOAD_NOTIFICATION_SETTING,
        p_encode_buffer,
        stream_o.bytes_written,
        0,
        ONDeviceUploadNotification_send_callback,
        (void*)__FUNCTION__);
    ry_free(p_encode_buffer);
    return;

exit:
    ry_free(p_cfg);
    ry_free(p_encode_buffer);
    LOG_ERROR("[ANCS] on Bond finished and failed with upload\n");
    return;
}


void app_notify_tele_answer(int num)
{
    u8_t *data_buf = NULL;
    pb_ostream_t stream;
    NotificationAnswerCallParam* tele = (NotificationAnswerCallParam *)ry_malloc(sizeof(NotificationAnswerCallParam));
    if(tele == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    strcpy((char*)tele->number, (char const*)app_notify_get(num)->data);

    data_buf = (u8_t *)ry_malloc(sizeof(NotificationAnswerCallParam));

    stream = pb_ostream_from_buffer(data_buf, (sizeof(NotificationAnswerCallParam)));

    if (!pb_encode(&stream, NotificationAnswerCallParam_fields, tele)) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }

    rbp_send_req(CMD_APP_NOTIFICATION_ANSWER_CALL, data_buf, stream.bytes_written, 1);

exit:
    ry_free(tele);
    ry_free(data_buf);
}

void app_notify_tele_mute(int num)
{
    u8_t *data_buf = NULL;
    pb_ostream_t stream;
    NotificationMuteCallParam* tele = (NotificationMuteCallParam *)ry_malloc(sizeof(NotificationMuteCallParam));
    if(tele == NULL){
        LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
        goto exit;
    }
    
    strcpy((char*)tele->number, (char const*)app_notify_get(num)->data);

    data_buf = (u8_t *)ry_malloc(sizeof(NotificationMuteCallParam));

    stream = pb_ostream_from_buffer(data_buf, (sizeof(NotificationMuteCallParam)));

    if (!pb_encode(&stream, NotificationMuteCallParam_fields, tele)) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }

    rbp_send_req(CMD_APP_NOTIFICATION_MUTE_CALL, data_buf, stream.bytes_written, 1);

exit:
    ry_free(tele);
    ry_free(data_buf);
}

void app_notify_tele_reject(int num)
{
    uint8_t peer_hone_type = ry_ble_get_peer_phone_type();

    if(peer_hone_type == PEER_PHONE_TYPE_ANDROID)
    {
        u8_t *data_buf = NULL;
        pb_ostream_t stream;
        NotificationRejectCallParam* tele = (NotificationRejectCallParam *)ry_malloc(sizeof(NotificationRejectCallParam));
        if(tele == NULL)
        {
            LOG_DEBUG("[%s-%d] malloc failed\n", __FUNCTION__, __LINE__);
            goto exit;
        }

        strcpy(tele->number, (char const*)app_notify_get(num)->data);

        data_buf = (u8_t *)ry_malloc(sizeof(NotificationRejectCallParam));

        stream = pb_ostream_from_buffer(data_buf, (sizeof(NotificationRejectCallParam)));

        if (!pb_encode(&stream, NotificationRejectCallParam_fields, tele))
        {
            LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
        }

        rbp_send_req(CMD_APP_NOTIFICATION_REJECT_CALL, data_buf, stream.bytes_written, 1);
exit:
        ry_free(tele);
        ry_free(data_buf);
    }
    else if(peer_hone_type == PEER_PHONE_TYPE_IOS)
    {
        ancsTryRejectCurrentCalling();
    
}
}








