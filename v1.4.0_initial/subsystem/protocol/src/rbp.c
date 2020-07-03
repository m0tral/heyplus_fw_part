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
#include "app_config.h"
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos_update.h"


#include "rbp.h"
#include "dip.h"
#include "fhp.h"
//#include "reliable_xfer.h"
#include "r_xfer.h"
#include "ble_task.h"
#include "ry_ble.h"

/* Proto Buffer Related */
#include "nanopb_common.h"
#include "rbp.pb.h"
#include "Gate.pb.h"
#include "transit.pb.h"
#include "device.pb.h"
#include "ryos_timer.h"
#include "ryos.h"
#include "ry_hal_rtc.h"
#include "mible.h"
#include "app_pm.h"
#include "sensor_alg.h"

#include "ble_handler.h"
#include "msg_management_service.h"
#include "data_management_service.h"
#include "card_management_service.h"
#include "mijia_service.h"
#include "timer_management_service.h"
#include "app_management_service.h"
#include "surface_management_service.h"
#include "device_management_service.h"
#include "window_management_service.h"
#include "ry_nfc.h"
#include "crc32.h"
#include "mible_crypto.h"
#include "activity_surface.h"
#include "apdu.h"
#include "ry_statistics.h"
#include "ry_hal_inc.h"
#include "PhoneApp.pb.h"
#include "storage_management_service.h"



/*********************************************************************
 * CONSTANTS
 */ 

//#define RAW_DATA_DEBUG


#define CUR_RBP_VERSION                 1
#define MAX_RBP_VERSION                 1

/*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
static int last_session_id;

extern phNfcBle_Context_t gphNfcBle_Context;

u8_t apdu_rsp[1024];
u16_t apdu_resp_len;
u8_t flg_set_online_raw_data_onOff;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void set_online_raw_data_onOff(uint32_t flgOn);

#if 0
void rbp_bindingRes_parser(uint8_t*data, uint32_t len)
{
    nanopb_buf_t pb_buf;
    ry_sts_t status;
    int i;
        
    /* Allocate space for the decoded message. */
    BindingResMsg message = BindingResMsg_init_zero;

    /* Create a stream that reads from the buffer. */
    pb_istream_t stream = pb_istream_from_buffer(data, len);

    message.uid.funcs.decode = nanopb_decode_val;
    pb_buf.buf = ry_malloc(len); // Note: The length should less than 4K bytes.
    memset(pb_buf.buf, 0, len);
    message.uid.arg = &pb_buf;

    
    /* Now we are ready to decode the message. */
    status = pb_decode(&stream, BindingResMsg_fields, &message);

    /* Check for errors... */
    if (!status) {
        LOG_ERROR("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
    }

    if (message.error == RY_SUCC) {
        LOG_INFO("[RBP] Binding Success.UID = ");
        ry_data_dump(pb_buf.buf, pb_buf.len, ' ');
    } else {
        LOG_WARN("[RBP] Binding Error. Error code = 0x%x", message.error);
    }

    ry_free(pb_buf.buf);
}
#endif

void rbp_tx_cb(ry_sts_t status, void* usrdata)
{
    if(usrdata != NULL) {
        ry_free(usrdata);
    }
    LOG_DEBUG("[rbp_tx_cb] TX Callback, status:0x%04x\r\n", status);
    return;
}


/**
 * @brief   API to send data through RBP
 *
 * @param   cmd,  which command to send.
 * @param   code,  which code to send.
 * @param   type, which type to send.
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 * @param   isSec, flag to indicating a secure packet or not.
 *
 * @return  None
 */
ry_sts_t rbp_send_resp(CMD cmd, uint32_t code, uint8_t* data, uint32_t len, uint8_t isSec)
{
    ry_sts_t status;
    int i;
    size_t message_length;
    u16_t crc_val;
    u16_t pack_num = 1;
    u32_t pack_len = 0;

    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALID_STATE);
    }

    if(len > MAX_R_XFER_SIZE) {

        pack_num = (len + MAX_R_XFER_SIZE - 1)/MAX_R_XFER_SIZE;
        pack_len = MAX_R_XFER_SIZE;
    }else{
        pack_len = len;

    }

    RbpMsg *msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    if(msg == NULL){
        LOG_ERROR("RbpMsg malloc failed\n");
        goto exit;
    }
    ry_memset((void*)msg, 0, sizeof(RbpMsg));
    msg->protocol_ver     = CUR_RBP_VERSION;
    msg->cmd              = cmd;
    msg->which_message    = RbpMsg_res_tag; 
    msg->message.res.code = code;
    msg->message.res.total = pack_num;
    

    for (i = 0; i < pack_num; i++){
    
        if(i == pack_num - 1 ){
            pack_len = len%MAX_R_XFER_SIZE;
            //LOG_DEBUG("pack_len = %x\n", pack_len);
        }

        
        /*if(cmd == CMD_DEV_GET_UPLOAD_DATA)
        {
            LOG_DEBUG("xxxxxxxx\n");
        }*/
    
        uint8_t* data_buf = (uint8_t*)ry_malloc(pack_len+100);
        if(data_buf == NULL){
            LOG_ERROR("data_buf malloc failed\n");
            goto exit;
        }
            
        
        LOG_DEBUG("[rbp_send_resp]: RBP Message length: %d\r\n", pack_len);

        /* Create a stream that will write to our buffer. */
        pb_ostream_t stream = pb_ostream_from_buffer(data_buf, pack_len+100);

        
        if (last_session_id) {
            msg->has_session_id  = 1;
            msg->session_id      = last_session_id;
        }

        
        msg->message.res.sn = i;
        if (pack_len) {
            msg->message.res.has_val = 1;
            msg->message.res.val.size = pack_len;
            ry_memcpy(msg->message.res.val.bytes, &data[i * MAX_R_XFER_SIZE], pack_len);
        } else {
            msg->message.res.has_val = 0;
        }
        

        /* Now we are ready to encode the message! */
        status = pb_encode(&stream, RbpMsg_fields, msg);
        message_length = stream.bytes_written;

        /* Then just check for any errors.. */
        if (!status) {
            LOG_ERROR("[rbp_send_resp]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
            return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        } else {

#ifdef RAW_DATA_DEBUG
            LOG_DEBUG("[rbp_send_resp]: The Encoding message len: %d\n", message_length);
            ry_data_dump(data_buf, message_length, ' ');
#endif
        }

        if (isSec) {
            mible_encrypt(data_buf, message_length, data_buf);
#if (RY_BLE_ENABLE == TRUE) 
            //r_xfer_register_tx(ble_reliableSend);
#endif
            /* Add CRC */
            crc_val = calc_crc16(data_buf, message_length);
    	    ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
    	    message_length += sizeof(crc_val);
            
            r_xfer_send(R_XFER_INST_ID_BLE_SEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);


        } else {
#if (RY_BLE_ENABLE == TRUE) 
            //r_xfer_register_tx(ble_reliableUnsecureSend);
#endif
            /* Add CRC */
            crc_val = calc_crc16(data_buf, message_length);
    	    ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
    	    message_length += sizeof(crc_val);
            
            r_xfer_send(R_XFER_INST_ID_BLE_UNSEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
        }
        
        //r_xfer_send(data_buf, message_length, rbp_tx_cb, (void*)data_buf);
  
    }
exit:
    
    ry_free((void*)msg);
    return RY_SUCC;
}


/**
 * @brief   API to send data through RBP
 *
 * @param   cmd,  which command to send.
 * @param   code,  which code to send.
 * @param   type, which type to send.
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 * @param   isSec, flag to indicating a secure packet or not.
 *
 * @return  None
 */
ry_sts_t rbp_send_resp_with_cb(CMD cmd, uint32_t code, uint8_t* data, uint32_t len, uint8_t isSec, ry_ble_send_cb_t cb, void* arg)
{
    ry_sts_t status;
    int i;
    size_t message_length;
    u16_t crc_val;
    u16_t pack_num = 1;
    u32_t pack_len = 0;
    rbp_para_t *para = (rbp_para_t*)arg;

    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALID_STATE);
    }

    if(cb == NULL) {
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
    }

    if(len > MAX_R_XFER_SIZE) {

        pack_num = (len + MAX_R_XFER_SIZE - 1)/MAX_R_XFER_SIZE;
        pack_len = MAX_R_XFER_SIZE;
    }else{
        pack_len = len;
    }

    RbpMsg *msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    if(msg == NULL){
        LOG_ERROR("RbpMsg malloc failed\n");
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
        if (cb) {
            cb(status, arg);
        }
        goto exit;
    }
    ry_memset((void*)msg, 0, sizeof(RbpMsg));
    msg->protocol_ver     = CUR_RBP_VERSION;
    msg->cmd              = cmd;
    msg->which_message    = RbpMsg_res_tag; 
    msg->message.res.code = code;
    msg->message.res.total = pack_num;

    for (i = 0; i < pack_num; i++){
    
        if(i == pack_num - 1 ){
            pack_len = len%MAX_R_XFER_SIZE;
            //LOG_DEBUG("pack_len = %x\n", pack_len);
        }

        /*if(cmd == CMD_DEV_GET_UPLOAD_DATA)
        {
            LOG_DEBUG("xxxxxxxx\n");
        }*/

        //para->rbpData should be free in cb currently see @ry_ble_txCallback
        para->rbpData = (uint8_t*)ry_malloc(pack_len+100);
        if(para->rbpData == NULL){
            LOG_ERROR("data_buf malloc failed\n");
            goto exit;
        }

        LOG_DEBUG("[rbp_send_resp]: RBP Message length: %d\r\n", pack_len);

        /* Create a stream that will write to our buffer. */
        pb_ostream_t stream = pb_ostream_from_buffer(para->rbpData, pack_len+100);

        if (last_session_id) {
            msg->has_session_id  = 1;
            msg->session_id      = last_session_id;
        }

        msg->message.res.sn = i;
        if (pack_len) {
            msg->message.res.has_val = 1;
            msg->message.res.val.size = pack_len;
            ry_memcpy(msg->message.res.val.bytes, &data[i * MAX_R_XFER_SIZE], pack_len);
        } else {
            msg->message.res.has_val = 0;
        }

        /* Now we are ready to encode the message! */
        status = pb_encode(&stream, RbpMsg_fields, msg);
        message_length = stream.bytes_written;

        /* Then just check for any errors.. */
        if (!status) {
            LOG_ERROR("[rbp_send_resp]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
            return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        } else {

#ifdef RAW_DATA_DEBUG
            LOG_DEBUG("[rbp_send_resp]: The Encoding message len: %d\n", message_length);
            ry_data_dump(para->rbpData, message_length, ' ');
#endif
        }

        if (isSec) {
            mible_encrypt(para->rbpData, message_length, para->rbpData);
#if (RY_BLE_ENABLE == TRUE) 
            //r_xfer_register_tx(ble_reliableSend);
#endif
            /* Add CRC */
            crc_val = calc_crc16(para->rbpData, message_length);
            ry_memcpy(&para->rbpData[message_length], &crc_val, sizeof(crc_val));
            message_length += sizeof(crc_val);

            r_xfer_send(R_XFER_INST_ID_BLE_SEC, para->rbpData, message_length, (ry_ble_send_cb_t)cb, (void*)arg);


        } else {
#if (RY_BLE_ENABLE == TRUE) 
            //r_xfer_register_tx(ble_reliableUnsecureSend);
#endif
            /* Add CRC */
            crc_val = calc_crc16(para->rbpData, message_length);
            ry_memcpy(&para->rbpData[message_length], &crc_val, sizeof(crc_val));
            message_length += sizeof(crc_val);
            
            r_xfer_send(R_XFER_INST_ID_BLE_UNSEC, para->rbpData, message_length, (ry_ble_send_cb_t)cb, (void*)arg);
        }
        //r_xfer_send(data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    }
exit:
    
    ry_free((void*)msg);
    return status;
}





/**
 * @brief   API to send data through RBP
 *
 * @param   cmd,  which command to send.
 * @param   type, which type to send.
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 * @param   isSec, flag to indicating a secure packet or not.
 *
 * @return  None
 */
static int rbp_session_id = 100;
ry_sts_t rbp_send_req(CMD cmd, uint8_t* data, uint32_t len, uint8_t isSec)
{
    ry_sts_t status;
    int i;
    size_t message_length;
    u16_t  crc_val;
    uint8_t* data_buf = NULL;
    RbpMsg* msg = NULL;

    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALID_STATE);
    }

    if (ry_ble_get_peer_phone_app_lifecycle() == PEER_PHONE_APP_LIFECYCLE_DESTROYED) {
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_INVALID_STATE);
    }

    // free as external parameter in userdata see @rbp_tx_cb
    data_buf = (uint8_t*)ry_malloc(len+100);
    if(data_buf == NULL) {
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
    }
        
    /* Allocate space for the decoded message. */
    msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    if(msg == NULL){
        FREE_PTR(data_buf);
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
    }
    ry_memset((void*)msg, 0, sizeof(RbpMsg));
    LOG_DEBUG("[rbp_send_req] RBP Message len: %d, cmd:%d\r\n", len, cmd);

    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, len+100);

    msg->protocol_ver     = CUR_RBP_VERSION;
    msg->cmd              = cmd;
    msg->which_message    = RbpMsg_req_tag; 
    msg->has_session_id   = 1;
    msg->session_id       = rbp_session_id++;
    if (len) {
        msg->message.req.has_val = 1;
        msg->message.req.total = (len + 2048-1)/2048;
        msg->message.req.sn = 0;
        msg->message.req.val.size = len;
        ry_memcpy(msg->message.req.val.bytes, data, len);
    } else {
        msg->message.req.has_val = 0;
        msg->message.req.total = 1;
        msg->message.req.sn = 0;
    }
    

    /* Now we are ready to encode the message! */
    status = pb_encode(&stream, RbpMsg_fields, msg);
    message_length = stream.bytes_written;

    /* Then just check for any errors.. */
    if (!status) {
        LOG_ERROR("[rbp_send_resp]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
    } else {

#ifdef RAW_DATA_DEBUG
        LOG_DEBUG("[rbp_send_resp]: The Encoding message len: %d\n", message_length);
        ry_data_dump(data_buf, message_length, ' ');
#endif
    }

    if (isSec) {
        mible_encrypt(data_buf, message_length, data_buf);
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableSend);
#endif
        //CRC 
        crc_val = calc_crc16(data_buf, message_length);
        ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);
    
        r_xfer_send(R_XFER_INST_ID_BLE_SEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    } else {
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableUnsecureSend);
#endif
        /* Add CRC */
        crc_val = calc_crc16(data_buf, message_length);
        ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);

        r_xfer_send(R_XFER_INST_ID_BLE_UNSEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    }
    
    //r_xfer_send(data_buf, message_length, rbp_tx_cb, (void*)data_buf);

exit:

    ry_free((void*)msg);

    return RY_SUCC;
}


ry_sts_t rbp_send_req_with_cb(CMD cmd, uint8_t* data, uint32_t len, uint8_t isSec, ry_ble_send_cb_t cb, void* arg)
{
    ry_sts_t status;
    int pb_ret;
    int i;
    size_t message_length;
    u16_t  crc_val;
    rbp_para_t *para = (rbp_para_t*)arg;

    if (ry_ble_get_state() != RY_BLE_STATE_CONNECTED) {
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALID_STATE);
        if (cb) {
            cb(status, arg);
        }
        return status;
    }

    para->rbpData = (uint8_t*)ry_malloc(len+100);
    if (para->rbpData == NULL) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
        if (cb) {
            cb(status, arg);
        }
        return status;
    }
        
    /* Allocate space for the decoded message. */
    RbpMsg *msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    if (msg == NULL) {
        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_NO_MEM);
        ry_free(para->rbpData);
        if (cb) {
            cb(status, arg);
        }
        return status;
    }
    ry_memset((void*)msg, 0, sizeof(RbpMsg));
    LOG_DEBUG("[rbp_send_req]: RBP Message length: %d\r\n", len);

    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(para->rbpData, len+100);

    msg->protocol_ver     = CUR_RBP_VERSION;
    msg->cmd              = cmd;
    msg->which_message    = RbpMsg_req_tag; 
    msg->has_session_id   = 1;
    msg->session_id       = rbp_session_id++;
    if (len) {
        msg->message.req.has_val = 1;
        msg->message.req.total = (len + 2048-1)/2048;
        msg->message.req.sn = 0;
        msg->message.req.val.size = len;
        ry_memcpy(msg->message.req.val.bytes, data, len);
    } else {
        msg->message.req.has_val = 0;
        msg->message.req.total = 1;
        msg->message.req.sn = 0;
    }
    

    /* Now we are ready to encode the message! */
    pb_ret = pb_encode(&stream, RbpMsg_fields, msg);
    message_length = stream.bytes_written;

    /* Then just check for any errors.. */
    if (!pb_ret) {
        LOG_ERROR("[rbp_send_resp]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        ry_free((void*)msg);
        ry_free((void*)para->rbpData);
        if (cb) {
            cb(status, arg);
        }
        
        return status;
    } else {

#ifdef RAW_DATA_DEBUG
        LOG_DEBUG("[rbp_send_resp]: The Encoding message len: %d\n", message_length);
        ry_data_dump(para->rbpData, message_length, ' ');
#endif
    }

    if (isSec) {
        mible_encrypt(para->rbpData, message_length, para->rbpData);
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableSend);
#endif
        //CRC 
        crc_val = calc_crc16(para->rbpData, message_length);
        ry_memcpy(&para->rbpData[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);
    
        status = r_xfer_send(R_XFER_INST_ID_BLE_SEC, para->rbpData, message_length, (ry_ble_send_cb_t)cb, (void*)arg);
    } else {
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableUnsecureSend);
#endif
        /* Add CRC */
        crc_val = calc_crc16(para->rbpData, message_length);
        ry_memcpy(&para->rbpData[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);

        status = r_xfer_send(R_XFER_INST_ID_BLE_UNSEC, para->rbpData, message_length, (ry_ble_send_cb_t)cb, (void*)arg);
    }

    //r_xfer_send(data_buf, message_length, rbp_tx_cb, (void*)data_buf);

//exit:
    if(status != RY_SUCC) {
        ry_free(para->rbpData);
        para->rbpData = NULL;
    }

    ry_free((void*)msg);

    return status;
}






uint32_t req_big_data_send_size = 0;
uint32_t req_big_data_len =0;

uint8_t * req_big_data = NULL;
uint8_t req_big_data_isSec = 0;


ry_sts_t rbp_send_req_pack(RbpMsg *msg, uint8_t isSec)
{
    ry_sts_t status;
    int i;
    size_t message_length;
    u16_t  crc_val;

    
    uint8_t* data_buf = (uint8_t*)ry_malloc(MAX_R_XFER_SIZE+100);
    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, MAX_R_XFER_SIZE+100);


    /* Now we are ready to encode the message! */
    status = pb_encode(&stream, RbpMsg_fields, msg);
    message_length = stream.bytes_written;

    /* Then just check for any errors.. */
    if (!status) {
        LOG_ERROR("[rbp_send_resp]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
    } else {

#ifdef RAW_DATA_DEBUG
        LOG_DEBUG("[rbp_send_resp]: The Encoding message len: %d\n", message_length);
        ry_data_dump(data_buf, message_length, ' ');
#endif
    }

    if (isSec) {
        mible_encrypt(data_buf, message_length, data_buf);
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableSend);
#endif
        //CRC 
        crc_val = calc_crc16(data_buf, message_length);
        ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);
    
        r_xfer_send(R_XFER_INST_ID_BLE_SEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    } else {
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableUnsecureSend);
#endif
        /* Add CRC */
        crc_val = calc_crc16(data_buf, message_length);
        ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);
        
        r_xfer_send(R_XFER_INST_ID_BLE_UNSEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    }
    
    //r_xfer_send(data_buf, message_length, rbp_tx_cb, (void*)data_buf);

exit:

    
    return RY_SUCC;
}


ry_sts_t rbp_send_req_big_data_f(CMD cmd, uint8_t* data, uint32_t len, uint8_t isSec)
{
    ry_sts_t status;
    int i;
    size_t message_length;
    u16_t  crc_val;

    if (len < MAX_R_XFER_SIZE) {
        return rbp_send_req( cmd,  data,  len,  isSec);
    }
    
    req_big_data_send_size = MAX_R_XFER_SIZE;
    req_big_data = data;
    req_big_data_isSec = isSec;
    req_big_data_len = len;
    
        
    /* Allocate space for the decoded message. */
    RbpMsg *msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    ry_memset((void*)msg, 0, sizeof(RbpMsg));
    LOG_DEBUG("[rbp_send_req]: RBP Message length: %d\r\n", len);

    msg->protocol_ver     = CUR_RBP_VERSION;
    msg->cmd              = cmd;
    msg->which_message    = RbpMsg_req_tag; 
    msg->has_session_id   = 1;
    msg->session_id       = rbp_session_id++;

    msg->message.req.total = (len + MAX_R_XFER_SIZE-1)/MAX_R_XFER_SIZE;
    msg->message.req.sn = 0;
    msg->message.req.val.size = MAX_R_XFER_SIZE;
    ry_memcpy(msg->message.req.val.bytes, data, MAX_R_XFER_SIZE);

    rbp_send_req_pack(msg, isSec);

#if 0    
    uint8_t* data_buf = (uint8_t*)ry_malloc(MAX_R_XFER_SIZE+100);
    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, MAX_R_XFER_SIZE+100);


    /* Now we are ready to encode the message! */
    status = pb_encode(&stream, RbpMsg_fields, msg);
    message_length = stream.bytes_written;

    /* Then just check for any errors.. */
    if (!status) {
        LOG_ERROR("[rbp_send_resp]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
    } else {

#ifdef RAW_DATA_DEBUG
        LOG_DEBUG("[rbp_send_resp]: The Encoding message len: %d\n", message_length);
        ry_data_dump(data_buf, message_length, ' ');
#endif
    }

    if (isSec) {
        mible_encrypt(data_buf, message_length, data_buf);
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableSend);
#endif
        //CRC 
        crc_val = calc_crc16(data_buf, message_length);
        ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);
    
        r_xfer_send(R_XFER_INST_ID_BLE_SEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    } else {
#if (RY_BLE_ENABLE == TRUE) 
        //r_xfer_register_tx(ble_reliableUnsecureSend);
#endif
        /* Add CRC */
        crc_val = calc_crc16(data_buf, message_length);
        ry_memcpy(&data_buf[message_length], &crc_val, sizeof(crc_val));
        message_length += sizeof(crc_val);
        
        r_xfer_send(R_XFER_INST_ID_BLE_UNSEC, data_buf, message_length, rbp_tx_cb, (void*)data_buf);
    }
    
    //r_xfer_send(data_buf, message_length, rbp_tx_cb, (void*)data_buf);

exit:
#endif
    ry_free((void*)msg);
    
    return RY_SUCC;
}
ry_sts_t rbp_send_req_big_data_c(CMD cmd)
{
    ry_sts_t status;
    int i;
    size_t message_length;
    u16_t  crc_val;
    uint32_t len = 0;

    //req_big_data_len;
    //req_big_data_send_size += MAX_R_XFER_SIZE;
    //req_big_data = data;
    //req_big_data_isSec = isSec;

    if(req_big_data_len == 0){
        return RY_SUCC;
    }
    
    if(req_big_data_len - req_big_data_send_size > MAX_R_XFER_SIZE){
        len = MAX_R_XFER_SIZE;
    }else{
        //last pack
        len = req_big_data_len - req_big_data_send_size;
        //req_big_data_len = 0;
    }
        
    /* Allocate space for the decoded message. */
    RbpMsg *msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    ry_memset((void*)msg, 0, sizeof(RbpMsg));
    LOG_DEBUG("[rbp_send_req_big_data_c]: RBP Message length: %d\r\n", len);

    msg->protocol_ver     = CUR_RBP_VERSION;
    msg->cmd              = cmd;
    msg->which_message    = RbpMsg_req_tag; 
    msg->has_session_id   = 1;
    msg->session_id       = rbp_session_id++;

    msg->message.req.total = (req_big_data_len + MAX_R_XFER_SIZE-1)/MAX_R_XFER_SIZE;
    msg->message.req.sn = req_big_data_send_size/MAX_R_XFER_SIZE;
    msg->message.req.val.size = MAX_R_XFER_SIZE;
    ry_memcpy(msg->message.req.val.bytes, &req_big_data[req_big_data_send_size], MAX_R_XFER_SIZE);

    rbp_send_req_pack(msg, req_big_data_isSec);

    req_big_data_send_size += MAX_R_XFER_SIZE;

    if(req_big_data_send_size >= req_big_data_len){
        req_big_data_send_size = 0;
        req_big_data_len = 0;
    }

    ry_free((void*)msg);

    return RY_SUCC;
}


void rbp_common_tx_callback(ry_sts_t status, void* usrdata)
{
    LOG_WARN("[rbp_common_tx_callback] TX Callback, cmd:%s, status:0x%04x, cur state:%d\r\n", (char*)usrdata, status, get_device_session());
    return;
}

void rbp_restart_callback(ry_sts_t status, void* usrdata)
{
    restart_device();
}


void set_online_raw_data_onOff(uint32_t flgOn)
{
    flg_set_online_raw_data_onOff = flgOn;
}

uint8_t get_online_raw_data_onOff(void)
{
    return flg_set_online_raw_data_onOff;
}

ry_sts_t rbp_parser(uint8_t* data, uint32_t len)
{
    ry_sts_t status = RY_SUCC;
    int i;
    RbpMsg *msg = NULL;
    int do_update = 0;
    uint8_t conn_param_type;
    u8_t session_code = get_device_session() + RBP_RESP_CODE_INVALID_STATE;

    /* Create a stream that reads from the buffer. */
    pb_istream_t stream = pb_istream_from_buffer(data, len);


    msg = (RbpMsg *)ry_malloc(sizeof(RbpMsg));
    if (!msg) {
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        return status;
    }

    ry_memset((void*)msg, 0, sizeof(RbpMsg));

    /* Now we are ready to decode the message. */
    if (0 == pb_decode(&stream, RbpMsg_fields, msg)) {
        LOG_ERROR("[%s]Decoding failed: %s\n", PB_GET_ERROR(&stream),__FUNCTION__);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        goto exit;
    }

    if (msg->protocol_ver > MAX_RBP_VERSION) {
        LOG_ERROR("Protocol Version error.\n");
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
        goto exit;
    }

    if(ry_ble_get_peer_phone_app_lifecycle() == PEER_PHONE_APP_LIFECYCLE_DESTROYED) {
        ry_ble_set_peer_phone_app_lifecycle(PEER_PHONE_APP_LIFECYCLE_UNKOWN);
    }

    last_session_id = msg->has_session_id ? msg->session_id : 0;

#ifdef RAW_DATA_DEBUG
    //LOG_DEBUG("[RBP] Protocol Version: %d\r\n", msg->protocol_ver);
    LOG_DEBUG("[RBP] CMD             : %d\r\n", msg->cmd);
    //if (msg->has_session_id) {
    //    LOG_DEBUG("[RBP] Session ID      : %d\r\n", msg->session_id);
    //}
    //LOG_DEBUG("[RBP] Which Message   : %d\r\n", msg->which_message);
    //LOG_DEBUG("[RBP] Free head size  : %d\r\n", ryos_get_free_heap());

    switch (msg->which_message) {
        case RbpMsg_req_tag:
            
            //LOG_DEBUG("[RBP] Req Message: ");
            //ry_data_dump(msg->message.req.val.bytes, msg->message.req.val.size, ' ');
            break;

        case RbpMsg_res_tag:
            //LOG_DEBUG("[RBP] Res Message   : %d\r\n", msg->which_message);
            break;

       /* case RbpMsg_profile_tag:
            break;

        case RbpMsg_big_data_tag:
            break;*/

        default:
            break;
    }
#endif


    ry_hal_wdt_reset();

    switch (msg->cmd) {
        case CMD_PROP_GET:
            LOG_DEBUG("[rbp_parser] CMD_PROP_GET.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)) {
                ble_send_response(CMD_PROP_GET, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "PROP_GET invalid state");
                break;
            }            
            get_property(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_PROP_SET:
            LOG_INFO("[rbp_parser] CMD_PROP_SET.\n");
            set_property(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_GET_DEV_STATUS:
            LOG_DEBUG("[rbp_parser] CMD_DEV_GET_DEV_STATUS.\n");
            conn_param_type = RY_BLE_CONN_PARAM_FAST;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
            device_status_send();
            break;

        case CMD_DEV_GET_DEV_INFO:
            LOG_DEBUG("[rbp_parser] CMD_DEV_GET_DEV_INFO.\n");
            device_info_send();
            break;

        case CMD_DEV_GET_DEV_RUN_STATE:
            LOG_DEBUG("[rbp_parser] CMD_DEV_GET_DEV_RUN_STATE.\n");
            device_state_send();
            set_device_connection(DEV_BLE_LOGIN);
            break;

        case CMD_DEV_GET_DEV_MI_UNBIND_TOKEN:
            LOG_INFO("[rbp_parser] CMD_DEV_GET_DEV_MI_UNBIND_TOKEN.\n");
            device_mi_unbind_token_handler();
            break;

        case CMD_DEV_GET_DEV_MI_RYEEX_DID_TOKEN:
            LOG_INFO("[rbp_parser] CMD_DEV_GET_DEV_MI_RYEEX_DID_TOKEN.\n");
            device_mi_ryeex_did_token_handler();
            break;

        case CMD_DEV_REPAIR_RES_GET_LOST_FILE_NAMES:
            LOG_DEBUG("[rbp_parser] CMD_DEV_GET_DEV_LOST_FILE_NAMES.\n");
#warning temporary turned off, while understanding repair algo				
            //storage_send_lost_file_list();
            ble_send_response(CMD_DEV_REPAIR_RES_GET_LOST_FILE_NAMES, RBP_RESP_CODE_SUCCESS,
                NULL, 0, 0, rbp_common_tx_callback, "CMD_DEV_GET_DEV_LOST_FILE_NAMES success");
            break;

        case CMD_DEV_ACTIVATE_SE_START:
            LOG_INFO("[rbp_parser] CMD_DEV_ACTIVATE_SE_START.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_ACTIVATE_SE_START, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "SE_START invalid state");
                break;
            }
            
            device_activate_start();
            break;

        case CMD_DEV_ACTIVATE_SE_RESULT:
            LOG_INFO("[rbp_parser] CMD_DEV_ACTIVATE_SE_RESULT.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_ACTIVATE_SE_RESULT, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "SE_START invalid state");
                break;
            }
            
            device_se_activate_result(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_ACTIVATE_RYEEX_CERT_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_ACTIVATE_RYEEX_CERT_START. Not implement yet.\n");
            break;

        case CMD_DEV_ACTIVATE_RYEEX_CERT_SESSION_KEY:
            LOG_DEBUG("[rbp_parser] CMD_DEV_ACTIVATE_RYEEX_CERT_SESSION_KEY. Not implement yet.\n");
            break;

        case CMD_DEV_ACTIVATE_MI_CERT_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_ACTIVATE_MI_CERT_START. Not implement yet.\n");
            break;

        case CMD_DEV_ACTIVATE_MI_CERT_SESSION_KEY:
            LOG_DEBUG("[rbp_parser] CMD_DEV_ACTIVATE_MI_CERT_SESSION_KEY. Not implement yet.\n");
            break;

        case CMD_DEV_GET_DEV_CREDENTIAL:
            LOG_DEBUG("[rbp_parser] CMD_DEV_GET_DEV_CREDENTIAL.\n");
            device_info_cert();
            break;

        case CMD_DEV_GET_DEV_UNBIND_TOKEN:
            LOG_DEBUG("[rbp_parser] CMD_DEV_GET_DEV_UNBIND_TOKEN.\n");
            /* Device already unbind, while cloud is still binding. */
            device_unbind_token_send();
            break;

        case CMD_DEV_BIND_ACK_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_BIND_ACK_START.\n");
            device_start_bind_handler();
            DEV_STATISTICS(bind_req_count);
            break;

        case CMD_DEV_BIND_ACK_CANCEL:
            LOG_DEBUG("[rbp_parser] CMD_DEV_BIND_ACK_CANCEL.\n");
            device_bind_ack_cancel_handler();
            break;

        case CMD_DEV_BIND_RESULT:
            LOG_INFO("[rbp_parser] CMD_DEV_BIND_RESULT.\n");
            device_bind_ack_handler(msg->message.req.val.bytes, msg->message.req.val.size);
            //ble_updateConnPara(100, 200, 1500, 0);
            
            LOG_INFO("===================Binding Success=====================\n");            
            set_device_connection(DEV_BLE_LOGIN);
            break;

        case CMD_DEV_SET_TIME:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SET_TIME.\n");
            device_time_sync(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_SET_TIME_PARAM:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SET_TIME_PARAM.\n");        
            device_timezone_sync(msg->message.req.val.bytes, msg->message.req.val.size);            
            break;

        case CMD_DEV_UNBIND:
            LOG_INFO("[rbp_parser] CMD_DEV_UNBIND.\n");
            if (msg->message.req.total) {
                /* Unbind token from Cloud */
                device_unbind_handler_unsecure(msg->message.req.val.bytes, msg->message.req.val.size);
            } else {
                /* Direct unbind and send unbind response */
                device_unbind_handler();
            }
            break;


        case CMD_DEV_MI_UNBIND:
            LOG_INFO("[rbp_parser] CMD_DEV_MI_UNBIND.\n");
            device_mi_unbind_handler(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_RYEEX_BIND_BY_TOKEN:
            LOG_INFO("[rbp_parser] CMD_DEV_RYEEX_BIND_BY_TOKEN.\n");
            device_ryeex_bind_by_token_handler(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_FW_UPDATE_START:
            LOG_INFO("[rbp_parser] CMD_DEV_FW_UPDATE_START.\n");
            do_update = 0;

            if ((get_device_session() == DEV_SESSION_CARD) ||
                (get_device_session() == DEV_SESSION_UPLOAD_DATA)||(get_device_session() == DEV_SESSION_RUNNING)) {
                ble_send_response(CMD_DEV_FW_UPDATE_START, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "FW Update start invalid state");
                break;
            }

            if (sys_get_battery() >= LOW_POWER_LEVEL) {
                do_update = 1;
            }else{
                if (get_cur_update_status() == UPDATING) {
                    do_update = 1;
                } else {
                    rbp_send_resp(CMD_DEV_FW_UPDATE_START, RBP_RESP_CODE_LOW_POWER, NULL, 0, 1);
                }
            }

            if (do_update) {
                update_start(msg);
                set_device_session(DEV_SESSION_OTA);
            }
            break;

        case CMD_DEV_FW_UPDATE_FILE:
            LOG_DEBUG("[rbp_parser] CMD_DEV_FW_UPDATE_FILE.\n");

            if ((get_device_session() == DEV_SESSION_CARD) ||
                (get_device_session() == DEV_SESSION_UPLOAD_DATA)||(get_device_session() == DEV_SESSION_RUNNING)) {
                ble_send_response(CMD_DEV_FW_UPDATE_FILE, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "UPLOAD DATA invalid state");
                break;
            }
            update_handle(msg);
            break;

        case CMD_DEV_FW_UPDATE_STOP:
            LOG_INFO("[rbp_parser] CMD_DEV_FW_UPDATE_STOP.\n");
            if ((get_device_session() == DEV_SESSION_CARD) ||
                (get_device_session() == DEV_SESSION_UPLOAD_DATA)) {
                ble_send_response(CMD_DEV_FW_UPDATE_FILE, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "UPLOAD DATA invalid state");
                break;
            }
            update_stop();
            break;

        case CMD_DEV_UPLOAD_DATA_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_UPLOAD_DATA_START.\n");
            if ((get_device_session() == DEV_SESSION_CARD)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_FW_UPDATE_START, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "FW Update start invalid state");
                break;
            }

            conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);

//            if(ry_ble_get_peer_phone_type() == PEER_PHONE_TYPE_IOS){
//                ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_DATA_SYNC_WAKEUP, 0, NULL);
//            }
            ryos_delay_ms(30);
            get_upload_data_handler();
            break;

        case CMD_DEV_NOTIFICATION:
            LOG_DEBUG("[rbp_parser] CMD_DEV_NOTIFICATION.\n");

            if ((get_device_session() == DEV_SESSION_CARD)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_NOTIFICATION, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Notification invalid state");
                break;
            }
            
            notify_req_handle(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_NOTIFICATION_GET_SETTING:
            LOG_DEBUG("[rbp_parser] CMD_DEV_NOTIFICATION_GET_SETTING.\n");
            notify_cfg_iOS_read_req_handle(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_NOTIFICATION_SET_SETTING:
            LOG_DEBUG("[rbp_parser] CMD_DEV_NOTIFICATION_SET_SETTING.\n");
            notify_cfg_iOS_write_req_handle(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_CARD_SET_DEFAULT:
            LOG_DEBUG("[rbp_parser] CMD_DEV_CARD_SET_DEFAULT.\n");
            card_set_default(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_CARD_GET_DEFAULT:
            LOG_DEBUG("[rbp_parser] CMD_DEV_CARD_GET_DEFAULT.\n");
            card_get_default(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_START_LOCATION_RESULT:
            LOG_DEBUG("[rbp_parser] CMD_DEV_START_LOCATION_RESULT.\n");
            location_result(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_UPLOAD_FILE:
            LOG_DEBUG("[rbp_parser] CMD_DEV_UPLOAD_FILE.\n");
            download_resource_file(msg);
            break;
            
        
        case CMD_DEV_SE_EXECUTE_APDU:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SE_EXECUTE_APDU.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "APDU invalid state");
                break;
            }
            
            apdu_raw_handler(msg->message.req.val.bytes, msg->message.req.val.size, &apdu_resp_len, apdu_rsp+2, (u16_t)msg->cmd, 1);
            break;

        case CMD_TEST_DEV_SEND_APDU:
        case CMD_DEV_SE_EXECUTE_APDU_OPEN:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SE_EXECUTE_APDU_OPEN.\n");

            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "APDU invalid state");
                break;
            }
            
            ry_data_dump(msg->message.req.val.bytes, msg->message.req.val.size, ' ');

            //ble_updateConnPara(6, 12, 500, 0);
            apdu_raw_handler(msg->message.req.val.bytes, msg->message.req.val.size, &apdu_resp_len, apdu_rsp+2, (u16_t)msg->cmd, 0);
            //phNfcCreateTLV((void*)&gphNfcBle_Context, msg->message.req.val.bytes+2, msg->message.req.val.size-2);

            //u8_t resp[2] = {0x00, 0x90};
            //rbp_send_resp(CMD_TEST_DEV_SEND_APDU, RBP_RESP_CODE_SUCCESS, resp, 2, 0);
            //ry_apdu_handler();
            break;

        case CMD_DEV_SE_OPEN:
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ){
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "APDU invalid state");
                break;
            }

            LOG_INFO("[rbp_parser] CMD_DEV_SE_OPEN.\n");

            conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
            card_onSEOpen();
            break;

        case CMD_DEV_SE_CLOSE:
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, RBP_RESP_CODE_INVALID_STATE, 
                        NULL, 0, 0, rbp_common_tx_callback, "APDU invalid state");
                break;
            }

            LOG_INFO("[rbp_parser] CMD_DEV_SE_CLOSE.\n");
            card_onSEClose();
            conn_param_type = RY_BLE_CONN_PARAM_MEDIUM;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
            break;

        case CMD_DEV_GATE_START_CHECK:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate start check invalid state");
                break;
            }
            LOG_INFO("[rbp_parser] CMD_DEV_GATE_START_CHECK.\n");
            card_onStartAccessCheck();
            break;

        case CMD_DEV_GATE_START_CREATE:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate start create invalid state");
                break;
            }
            
            LOG_INFO("[rbp_parser] CMD_DEV_GATE_START_CREATE.\n");
            card_onCreateAccessCard(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_GATE_CREATE_ACK:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ){
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate create ack invalid state");
                break;
            }
            LOG_INFO("[rbp_parser] CMD_DEV_GATE_CREATE_ACK.\n");
            card_onCreateAccessAck(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_GATE_GET_INFO_LIST:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate get info list invalid state");
                break;
            }
            LOG_DEBUG("[rbp_parser] CMD_DEV_GATE_GET_INFO_LIST.\n");
            card_onGetAccessCardList();
            break;

        case CMD_DEV_GATE_SET_INFO:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate set info invalid state");
                break;
            }
            LOG_DEBUG("[rbp_parser] CMD_DEV_GATE_SET_INFO.\n");
            card_onSetAccessCardInfo(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_GATE_START_DELETE:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate start delete invalid state");
                break;
            }
            LOG_INFO("[rbp_parser] CMD_DEV_GATE_START_DELETE.\n");
            card_onStartDeleteAccessCard(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_GATE_DELETE_ACK:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                ||(get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Gate delete ack invalid state");
                break;
            }
            LOG_INFO("[rbp_parser] CMD_DEV_GATE_DELETE_ACK.\n");
            card_onAccessCardDeleteAck(msg->message.req.val.bytes, msg->message.req.val.size);
            break;
            
        case CMD_DEV_WHITECARD_GET_INFO_LIST:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Whitecard get info list invalid state");
                break;
            }
            LOG_DEBUG("[rbp_parser] CMD_DEV_WHITECARD_GET_INFO_LIST.\n");
            card_onGetWhiteCardList();
            break;

        case CMD_DEV_WHITECARD_AID_SYNC:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Whitecard aid sync invalid state");
                break;
            }
            LOG_DEBUG("[rbp_parser] CMD_DEV_WHITECARD_AID_SYNC.\n");
            card_onWhiteCardAidSync();
            break;              

        case CMD_DEV_TRANSIT_START_CREATE:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Transit start create invalid state");
                break;
            }

            conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
            LOG_INFO("[rbp_parser] CMD_DEV_TRANSIT_START_CREATE.\n");
            //ry_data_dump(msg->message.req.val.bytes, msg->message.req.val.size, ' ');
            card_onStartCreateTransitCard(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_TRANSIT_CREATE_ACK:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Transit Create ack invalid state");
                break;
            }

            LOG_INFO("[rbp_parser] CMD_DEV_TRANSIT_CREATE_ACK.\n");
            card_onTransitCardCreateAck(msg->message.req.val.bytes, msg->message.req.val.size);
            break; 

        case CMD_DEV_TRANSIT_RECHARGE_START:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Transit Recharge start invalid state");
                break;
            }

            conn_param_type = RY_BLE_CONN_PARAM_SUPER_FAST;
            ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_CONN_UPDATE, 1, &conn_param_type);
            LOG_INFO("[rbp_parser] CMD_DEV_TRANSIT_RECHARGE_START.\n");
            card_onTransitRechargeStart(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_TRANSIT_RECHARGE_ACK:
            
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_powerstate() == DEV_PWR_STATE_OFF)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "Transit Recharge ack invalid state");
                break;
            }
            LOG_DEBUG("[rbp_parser] CMD_DEV_TRANSIT_RECHARGE_ACK.\n");
            card_onTransitRechargeAck(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_TRANSIT_GET_LIST:
            LOG_DEBUG("[rbp_parser] CMD_DEV_TRANSIT_GET_LIST.\n");
            card_onGetTransitCardList();
            break;

        case CMD_DEV_TRANSIT_SET_USE_CITY:
            LOG_INFO("[rbp_parser] CMD_DEV_TRANSIT_SET_USE_CITY.\n");
            card_onSetTransitCardPara(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_WEATHER_GET_CITY_LIST:
            LOG_DEBUG("[rbp_parser] CMD_DEV_WEATHER_GET_CITY_LIST.\n");
            repo_weather_city();
            break;

        case CMD_DEV_WEATHER_SET_CITY_LIST:
            LOG_INFO("[rbp_parser] CMD_DEV_WEATHER_SET_CITY_LIST.\n");
            set_weather_city(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_ALARM_CLOCK_SET:
            LOG_INFO("[rbp_parser] CMD_DEV_ALARM_CLOCK_SET.\n");
            set_alarm_clock(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_ALARM_CLOCK_GET_LIST:
            LOG_DEBUG("[rbp_parser] CMD_DEV_ALARM_CLOCK_GET_LIST.\n");
            get_alarm_clock();
            break;

        case CMD_DEV_ALARM_CLOCK_DELETE:
            LOG_INFO("[rbp_parser] CMD_DEV_ALARM_CLOCK_DELETE.\n");
            delete_alarm_clock(msg->message.req.val.bytes, msg->message.req.val.size);
            break;


        case CMD_APP_WEATHER_GET_REALTIME_INFO:
            LOG_DEBUG("[rbp_parser] CMD_APP_WEATHER_GET_REALTIME_INFO.\n");
            if(msg->message.res.code == 0){
                get_real_weather_result(msg->message.res.val.bytes, msg->message.res.val.size);
            }
            break;

        case CMD_APP_WEATHER_GET_FORECAST_INFO:
            LOG_DEBUG("[rbp_parser] CMD_APP_WEATHER_GET_FORECAST_INFO.\n");
            if(msg->message.res.code == 0){
                get_forecast_weather_result(msg->message.res.val.bytes, msg->message.res.val.size);
            }
            break;

        case CMD_APP_WEATHER_GET_INFO:
            LOG_DEBUG("[rbp_parser] CMD_APP_WEATHER_GET_INFO.\n");
            if(msg->message.res.code == 0){
                get_weather_info_result(msg->message.res.val.bytes, msg->message.res.val.size);
            }
            break;

        case CMD_DEV_SURFACE_GET_LIST:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SURFACE_GET_LIST.\n");
            surface_list_get(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_SURFACE_ADD_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SURFACE_ADD_START.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_SURFACE_ADD_START, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "app set invalid state");
            }else{
                surface_add_start(msg->message.res.val.bytes, msg->message.res.val.size);
            }
            break;

        case CMD_DEV_SURFACE_ADD_DATA:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SURFACE_ADD_DATA.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_SURFACE_ADD_DATA, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "app set invalid state");
            }else{
                surface_add_data(msg);
            }
            break;

        case CMD_DEV_SURFACE_DELETE:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SURFACE_DELETE.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_SURFACE_DELETE, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "app set invalid state");
            }else{
                surface_delete(msg);
            }
            break;

        case CMD_DEV_SURFACE_SET_CURRENT:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SURFACE_SET_CURRENT.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
//                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_SURFACE_SET_CURRENT, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "app set invalid state");
            }else{
                surface_set_current(msg->message.res.val.bytes, msg->message.res.val.size);
            }
            break;

        case CMD_DEV_APP_GET_LIST:
            LOG_DEBUG("[rbp_parser] CMD_DEV_APP_GET_LIST.\n");
            app_sequence_get(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_APP_SET_LIST:
            LOG_INFO("[rbp_parser] CMD_DEV_APP_SET_LIST.\n");
            if ((get_device_session() == DEV_SESSION_UPLOAD_DATA)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(CMD_DEV_ACTIVATE_SE_RESULT, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "app set invalid state");
                break;
            }
            app_sequence_set(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_APP_UPLOAD_DATA_LOCAL:
            LOG_DEBUG("[rbp_parser] CMD_APP_UPLOAD_DATA_LOCAL .\n");
            if ((get_device_session() == DEV_SESSION_CARD)
                || (get_device_session() == DEV_SESSION_OTA)
                || (get_device_session() == DEV_SESSION_SURFACE_EDIT)
                || (get_device_session() == DEV_SESSION_RUNNING)
                ) {
                ble_send_response(msg->cmd, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "app upload data invalid state");
                break;
            }
            
            if(msg->message.res.code == 0){
                LOG_INFO("[rbp_parser] upload success .\n");
                ry_data_msg_send(DATA_SERVICE_UPLOAD_DATA_START, 0, NULL);
            }else{
                LOG_DEBUG("[rbp_parser] upload error .\n");
            }
            
            break;

        case CMD_APP_UNBIND:
            LOG_INFO("[rbp_parser] CMD_APP_UNBIND.\n");
            break;

        case CMD_DEV_FW_UPDATE_TOKEN:
            LOG_DEBUG("[rbp_parser] CMD_DEV_FW_UPDATE_TOKEN.\n");
            update_token_handler(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_APP_SPORT_RUN_UPDATE:
            LOG_DEBUG("[rbp_parser] CMD_APP_SPORT_RUN_UPDATE.\n");
            update_run_result(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_SET_PHONE_APP_INFO:
            LOG_DEBUG("[rbp_parser] CMD_DEV_SET_PHONE_APP_INFO.\n");
            ry_ble_set_phone_info(msg->message.res.val.bytes, msg->message.res.val.size);
            //LOG_DEBUG("[rbp_parser] set phone app info.\n");
            //set_phone_app_info(msg->message.req.val.bytes, msg->message.req.val.size);
            break;

        case CMD_DEV_LOG_START:
        
            LOG_DEBUG("[rbp_parser] CMD_DEV_LOG_START.\n");
            error_log_upload_start();
            break;

        case CMD_DEV_LOG_RAW_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_LOG_RAW_START.\n");
            raw_data_upload_start();
            break;

        case CMD_APP_LOG_DATA:
        
            LOG_DEBUG("[rbp_parser] CMD_APP_LOG_DATA.\n");
            ry_data_msg_send(DATA_SERVICE_DEV_LOG_UPLOAD, 0, NULL);
            break;

        case CMD_APP_LOG_RAW_DATA:
            LOG_DEBUG("[rbp_parser] CMD_APP_LOG_RAW_DATA.\n");
            ry_data_msg_send(DATA_SERVICE_DEV_RAW_DATA_UPLOAD, 0, NULL);
            break;

        case CMD_DEV_MI_SCENE_GET_LIST:
            LOG_DEBUG("[rbp_parser] CMD_DEV_MI_SCENE_GET_LIST.\n");
            mijia_scene_get_list(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_MI_SCENE_ADD:
            LOG_DEBUG("[rbp_parser] CMD_DEV_MI_SCENE_ADD.\n");
            mijia_scene_add(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_MI_SCENE_ADD_BATCH:
            LOG_INFO("[rbp_parser] CMD_DEV_MI_SCENE_ADD_BATCH.\n");
            mijia_scene_add_batch(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_MI_SCENE_MODIFY:
            LOG_DEBUG("[rbp_parser] CMD_DEV_MI_SCENE_MODIFY.\n");
            mijia_scene_modify(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_MI_SCENE_DELETE:
            LOG_DEBUG("[rbp_parser] CMD_DEV_MI_SCENE_DELETE.\n");
            mijia_scene_delete(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_MI_SCENE_DELETE_ALL:
            LOG_DEBUG("[rbp_parser] CMD_DEV_MI_SCENE_DELETE_ALL.\n");
            mijia_scene_delete_all(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_APP_FOREGROUND_ENTER:
            ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_DATA_SYNC_WAKEUP, 0, NULL);
            LOG_DEBUG("[rbp_parser] CMD_DEV_APP_FOREGROUND_ENTER.\n");
            ry_ble_set_peer_phone_app_lifecycle(PEER_PHONE_APP_LIFECYCLE_FOREGROUND);
            ble_send_response(msg->cmd, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, rbp_common_tx_callback, "CMD_DEV_APP_FOREGROUND_ENTER");
            break;

        
        case CMD_DEV_APP_FOREGROUND_EXIT:
            LOG_DEBUG("[rbp_parser] CMD_DEV_APP_FOREGROUND_EXIT.\n");
            ry_ble_set_peer_phone_app_lifecycle(PEER_PHONE_APP_LIFECYCLE_BACKGROUND);
            ble_send_response(msg->cmd, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, rbp_common_tx_callback, "CMD_DEV_APP_FOREGROUND_EXIT");
            break;

        case CMD_DEV_APP_SEND_APP_STATUS:
            LOG_DEBUG("[rbp_parser] CMD_DEV_APP_SEND_APP_STATUS.\n");
            phone_app_status_handler(msg->message.res.val.bytes, msg->message.res.val.size);
            break;

        case CMD_DEV_REPAIR_RES_START:
            LOG_DEBUG("[rbp_parser] CMD_DEV_REPAIR_RES_START.\n");

            do_update = 0;

            if ((get_device_session() == DEV_SESSION_CARD) ||
                (get_device_session() == DEV_SESSION_UPLOAD_DATA)||(get_device_session() == DEV_SESSION_RUNNING)) {
                ble_send_response(CMD_DEV_FW_UPDATE_START, session_code, 
                        NULL, 0, 0, rbp_common_tx_callback, "FW Update start invalid state");
                break;
            }


            if (sys_get_battery() >= LOW_POWER_LEVEL) {
                do_update = 1;
            }else{
                if (get_cur_update_status() == UPDATING) {
                    do_update = 1;
                } else {
                    rbp_send_resp(CMD_DEV_FW_UPDATE_START, RBP_RESP_CODE_LOW_POWER, NULL, 0, 1);
                }
            }

            if (do_update) {
                file_repair_start(msg);
                set_device_session(DEV_SESSION_OTA);
            }
            
            break;

        case CMD_DEV_REPAIR_RES_FILE:
            LOG_DEBUG("[rbp_parser] CMD_DEV_REPAIR_RES_FILE.\n");
            file_repair_handle(msg);
            break;
				
        case CMD_DEV_RESTART_DEVICE:
            ble_send_response(msg->cmd, RBP_RESP_CODE_SUCCESS, 
                                NULL, 0, 1, rbp_restart_callback, NULL);
            break;

        case CMD_DEV_LOG_RAW_ONLINE_START:
            set_online_raw_data_onOff(1);
            ble_send_response(CMD_DEV_LOG_RAW_ONLINE_START, 0, 
                        NULL, 0, 0, NULL, "PROP_GET invalid state");
            break;

        case CMD_DEV_LOG_RAW_ONLINE_STOP:
            set_online_raw_data_onOff(0);
            ble_send_response(CMD_DEV_LOG_RAW_ONLINE_STOP, 0, 
                        NULL, 0, 0, NULL, "PROP_GET invalid state");
            break;

        default:
            break;
    }


exit:

    ry_free((void*)msg);
    return status;
}



