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
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"

#include "dip.h"
#include "reliable_xfer.h"
#include "rbp.h"
#include "mible.h"
#include "ry_nfc.h"
#include "ry_ble.h"

/* Proto Buffer Related */
#include "nanopb_common.h"
//#include "dev_info.pb.h"
#include "rbp.pb.h"
#include "device.pb.h"
#include "ry_hal_flash.h"
#include "DataUpload.pb.h"
#include "Property.pb.h"
#include "Time.pb.h"

#include "ryos_protocol.h"
#include "ry_hal_inc.h"
#include "app_interface.h"
#include "sensor_alg.h"
#include "data_management_service.h"
#include "Weather.pb.h"
#include "device_management_service.h"
#include "storage_management_service.h"
#include "BodyStatus.pb.h"
#include "aes.h"
#include "timer_management_service.h"
#include "ancs_api.h"
#include "msg_management_service.h"
#include "window_management_service.h"
#include "surface_management_service.h"
#include "touch.h"
#include "app_db.h"
#include "activity_qrcode.h"
#include "PhoneApp.pb.h"
#include "mijia_service.h"
#include "alipay.h"

/*********************************************************************
 * CONSTANTS
 */ 

#define DIP_FLASH_ADDR                   0x000FC000//0x00009F00//0x100F0000+512
#define DIP_FLASH_LEN                    (DEV_SN_LEN+SERVER_SN_LEN+TIME_LEN+UID_LEN+ACTIVATE_LEN)

#define DIP_ADDR_CERT_SN                 (DIP_FLASH_ADDR)
#define DIP_ADDR_CUR_TIME                (DIP_ADDR_CERT_SN + 4)
#define DIP_ADDR_UID                     (DIP_ADDR_CUR_TIME + 4)

#define DEVICE_BIND_INFO_FLASH_ADDR      0x000FC400
#define DEVICE_BIND_INFO_FLASH_LEN       sizeof(ry_device_bind_status_t)



#define DEV_BIND_STATE_UNBIND            0
#define DEV_BIND_STATE_WAIT_USER_ACK     1
#define DEV_BIND_STATE_WAIT_BIND         2
#define DEV_BIND_STATE_BOND              3


/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    u8_t mac[MAC_LEN];
    u8_t cert[CERT_LEN];
    u8_t model[MODEL_LEN];
    u8_t did[DID_LEN];
    u8_t fw_ver[FW_VER_LEN];
    u8_t sn[FACTORY_SN_LEN];
    u8_t battery;
    u8_t bind_status;
    char uid[UID_LEN];
    int  cur_time;
    u8_t hw_ver[HW_VER_LEN];
    u8_t aes_key[AES_KEY_LEN];
    u8_t iv[IV_LEN];
} dev_info_t;

int dev_bind_status = 0;

 
/*********************************************************************
 * LOCAL VARIABLES
 */

dev_info_t device_info;
static int cert_sn;
static int server_sn;
static int activate_status;
extern uint8_t g_BLEMacAddress[];


ry_device_bind_status_t device_bind_status;

static int bind_ack_once = 0;
static int allow_bind_ack = 0;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
//ry_sts_t dip_send(DEV_INFO_TYPE type, uint8_t* data, int len, uint8_t isSec);
static void dip_psm_save(void);
static void dip_psm_restore(void);
static uint32_t device_clear_all_user_info(void);




#define DIP_SEND_MAC()      dip_send(DEV_INFO_TYPE_MAC, device_info.mac, 6, 1)
#define DIP_SEND_CERT()     dip_send(DEV_INFO_TYPE_CERT, device_info.cert, CERT_LEN, 1)
#define DIP_SEND_DID()      dip_send(DEV_INFO_TYPE_DID, device_info.did, DID_LEN, 1)
#define DIP_SEND_MODEL()    dip_send(DEV_INFO_TYPE_MODEL, device_info.model, strlen(device_info.model), 1)
#define DIP_SEND_FW_VER()   dip_send(DEV_INFO_TYPE_FW_VER, device_info.fw_ver, FW_VER_LEN, 1)
#define DIP_SEND_UID()      dip_send(DEV_INFO_TYPE_USER_ID, device_info.uid, UID_LEN, 1)

void get_mac(u8_t * des, u32_t * len)
{
    char * data_str = (char *)RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 2;

    /* In case for not config user data. */
    if (is_empty((u8_t*)data_str, MAC_LEN)) {
        ry_memset(des, 0x0D, 6);
        *len = 6;
        return;
    }
    
    if((des != NULL)&&(len != NULL)){
        *len = 6;
        ry_memcpy(des, data_str, 6);
    }

    return ;
}
#define DIP_SEND_BATTERY()  dip_send(DEV_INFO_TYPE_BATTERY, device_info.battery, 2, 1)


void dip_flash_write(u32_t addr, u32_t len, u8_t* data)
{
    LOG_DEBUG("dip_flash_write\r\n");
    u8_t * temp = (u8_t*)ry_malloc(RY_FLASH_SIZEOF_ROW);

    ry_memcpy(temp, (void *)RY_DEVICE_INFO_ADDR, RY_FLASH_SIZEOF_ROW);
    ry_memcpy(&temp[addr - RY_DEVICE_INFO_ADDR], data, len);
    ry_hal_flash_erase(RY_DEVICE_INFO_ADDR, RY_FLASH_SIZEOF_ROW);
    ry_hal_flash_write(RY_DEVICE_INFO_ADDR, temp, RY_FLASH_SIZEOF_ROW);

    ry_free(temp);
}

void set_fw_ver(u8_t* des)
{
    ry_hal_flash_write(RY_DEVICE_FW_VER_ADDR, des, strlen((char*)des) + 1);    
}

void set_device_ids(u8_t* des, int len)
{
    ry_hal_flash_write(RY_DEVICE_INFO_ADDR, des, len);    
}

void set_device_sn(u8_t* des, int len)
{
    ry_hal_flash_write(0xFC404, des, len);    
}

void set_device_key(u8_t* des, int len)
{
    ry_hal_flash_write(0x8194, des, len);    
}

void set_device_mem(int addr, u8_t* des, int len)
{
    ry_hal_flash_write(addr, des, len);    
}


void get_device_id(u8_t * des, u32_t * len)
{
    char * data_str = (char *)RY_DEVICE_INFO_ADDR;

    /* In case for not config user data. */
    if (is_empty((u8_t*)data_str, DID_LEN)) {
        strcpy((char*)des, "ry.did.0093");
        return;
    }

    if((des != NULL)&&(len != NULL)){
        *len = strlen(data_str);
        strcpy((char*)des, data_str);
    }

    return ;
}

void get_sn(u8_t * des, u32_t * len)
{
    char * data_str = (char *)RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET;

    /* In case for not config user data. */
    if (is_empty((u8_t*)data_str, FACTORY_SN_LEN)) {
        strcpy((char*)des, "ry.sn.0000");
        return;
    }
    
    if((des != NULL)&&(len != NULL)){
        
        *len = strlen(data_str);

        strcpy((char*)des, data_str);
    }

    return ;
}

void get_safe_hex(u8_t * des, u32_t * len)
{
    char * data_str = (char *)RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3;
    
    if((des != NULL)&&(len != NULL)){
        *len = 6526;
        ry_memcpy(des, data_str, 6526);
    }

    return ;
}

void get_fw_ver(u8_t* des, u32_t* len)
{
    char * data_str = (char *)RY_DEVICE_FW_VER_ADDR;

    if (is_empty((u8_t*)data_str, FW_VER_LEN)) {
        strcpy((char*)des, "1.0.0");
        *len = strlen("1.0.0");
        return;
    }
    
    if((des != NULL)&&(len != NULL)){
        *len = 10;
        //ry_memcpy((u8_t*)des, data_str, 10);
        strcpy((char*)des, data_str);
    }

    return ;
}

void get_hw_ver(u8_t* des, u32_t* len)
{
    char * data_str = (char *)DEVICE_HARDWARE_VERSION_ADDR;

    if (is_empty((u8_t*)data_str, FW_VER_LEN)) {
        strcpy((char*)des, "0");
        *len = strlen("0");
        return;
    }
    
    if((des != NULL)&&(len != NULL)){
        *len = strlen(data_str)+1;
        strcpy((char*)des, data_str);
    }

    return ;
}


void device_bind_info_save(void)
{
    /* Save into flash */
    ry_hal_flash_write(DEVICE_BIND_INFO_FLASH_ADDR, (u8_t*)&device_bind_status, DEVICE_BIND_INFO_FLASH_LEN);
}


void device_bind_info_restore(void)
{
    /* Read device info data from flash */
    ry_hal_flash_read(DEVICE_BIND_INFO_FLASH_ADDR, (u8_t*)&device_bind_status, DEVICE_BIND_INFO_FLASH_LEN);

    if (device_bind_status.status.wordVal == 0xFFFFFFFF) {

        device_bind_status.status.wordVal = 0;
    }

    if (device_bind_status.server_sn == 0xFFFFFFFF) {

        device_bind_status.server_sn = 0;
    }

    if (device_bind_status.device_sn == 0xFFFFFFFF) {

        device_bind_status.device_sn = 0;
    }

    /* Success */
    LOG_DEBUG("[device_bind_info_restore] Restore succ.\r\n");
    LOG_DEBUG("se_act_status:%d\n", device_bind_status.status.bf.se_activate);
    LOG_DEBUG("ry_act_status:%d\n", device_bind_status.status.bf.ryeex_cert_activate);
    LOG_DEBUG("mi_act_status:%d\n", device_bind_status.status.bf.mi_cert_activate);
    LOG_DEBUG("device_sn: %d\n", device_bind_status.device_sn);
    LOG_DEBUG("server_sn: %d\n", device_bind_status.server_sn);
    LOG_DEBUG("ry_uid:");
    ry_data_dump((u8_t*)device_bind_status.ryeex_uid, MAX_RYEEX_ID_LEN, ' ');
    LOG_DEBUG("mi_uid:");
    ry_data_dump((u8_t*)device_bind_status.mi_uid, MAX_MI_ID_LEN, ' ');
    LOG_DEBUG("\n\n");    
}



/**
 * @brief   Save nessary device infomation to flash
 *
 * @param   None
 *
 * @return  None
 */
static void dip_psm_save(void)
{
    u8_t temp[DIP_FLASH_LEN] = {0};
    u8_t *p = temp;

    ry_memcpy(p, (u8_t*)&cert_sn, DEV_SN_LEN);
    p += DEV_SN_LEN;

    ry_memcpy(p, (u8_t*)&server_sn, SERVER_SN_LEN);
    p += SERVER_SN_LEN;
    
    ry_memcpy(p, (u8_t*)device_info.uid, UID_LEN);
    p += UID_LEN;
    
    ry_memcpy(p, (u8_t*)&device_info.cur_time, TIME_LEN);
    p += TIME_LEN;

    ry_memcpy(p, (u8_t*)&activate_status, 4);
    p += 4;

    LOG_DEBUG("DIP psm save.\n");
    ry_data_dump(temp, DIP_FLASH_LEN, ' ');
 
    /* Save into flash */
    ry_hal_flash_write(DIP_FLASH_ADDR, temp, DIP_FLASH_LEN);
    //dip_flash_write(DIP_FLASH_ADDR, DIP_FLASH_LEN, temp);

}



/**
 * @brief   Device Infomation Restore from Flash
 *
 * @param   None
 *
 * @return  None
 */
static void dip_psm_restore(void)
{
    u8_t temp[DIP_FLASH_LEN] = {0};
    u8_t *p = temp;

    /* Init flash if necessary */
    ry_hal_flash_init();

    /* Read device info data from flash */
    ry_hal_flash_read(DIP_FLASH_ADDR, p, DIP_FLASH_LEN);
    
    ry_memcpy((u8_t*)&cert_sn, p, DEV_SN_LEN);
    p += DEV_SN_LEN;
    
    ry_memcpy((u8_t*)&server_sn, p, SERVER_SN_LEN);
    p += SERVER_SN_LEN;
    
    ry_memcpy((u8_t*)device_info.uid, p, UID_LEN);
    p += UID_LEN;
    
    ry_memcpy((u8_t*)&device_info.cur_time, p, TIME_LEN);
    p += TIME_LEN;

    ry_memcpy((u8_t*)&activate_status, p, ACTIVATE_LEN);
    p += ACTIVATE_LEN;

    /* Check validation */
    if (cert_sn == 0xFFFFFFFF) {
        cert_sn = 0;
    }

    if (server_sn == 0xFFFFFFFF) {
        server_sn = 0;
    }

    if (is_empty((u8_t*)device_info.uid, UID_LEN)) {
        activate_status = 0;
        ry_memset(device_info.uid, 0x00, UID_LEN);
    } else {
        activate_status = 1;
    }

    activate_status = 1;

    /* Success */
    LOG_DEBUG("[DIP] PSM Restore succ.act_status:%d, cert_sn:%d, server_sn:%d\r\n", 
        activate_status, cert_sn, server_sn);
    LOG_DEBUG("act_status:%d\n", activate_status);
    LOG_DEBUG("cert_sn:   %d\n", cert_sn);
    LOG_DEBUG("server_sn: %d\n", server_sn);
    LOG_DEBUG("uid:");
    ry_data_dump((u8_t*)device_info.uid, UID_LEN, ' ');
    LOG_DEBUG("\n\n");
}


void device_security_load(void)
{
    ry_sts_t status;
    int empty = 0;
    u8_t * cert_addr = (u8_t *)(RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3 + sizeof(u32_t) );
    u8_t * safe_hex;
    
    if (ry_memcmp(cert_addr, &empty, 4)) {
        safe_hex = cert_addr;
		} else {
        safe_hex = cert_addr+16;
    }

    /* Load AES-KEY */
    ry_memcpy(device_info.aes_key,    &safe_hex[2],  4);
    ry_memcpy(device_info.aes_key+4,  &safe_hex[14], 4);
    ry_memcpy(device_info.aes_key+8,  &safe_hex[38], 4);
    ry_memcpy(device_info.aes_key+12, &safe_hex[52], 4);

    /* Load IV */
    ry_memcpy(device_info.iv,    safe_hex, 2);
    ry_memcpy(device_info.iv+2,  &safe_hex[6], 8);
    ry_memcpy(device_info.iv+10, &safe_hex[18], 6);
}

void device_info_load(void)
{
    u32_t len;

    /* load device fixed info */
    get_mac(device_info.mac, &len);
    get_sn(device_info.sn, &len);
    get_device_id(device_info.did, &len);
    get_fw_ver(device_info.fw_ver, &len);
    get_hw_ver(device_info.hw_ver, &len);
    //get_safe_hex();

    /* load device application info */
    //dip_psm_restore();

}


bool is_bond(void)
{
#if 0
    int i;
    for (i = 0; i < UID_LEN; i++) {
        if (device_info.uid[i] != 0) {
            return TRUE;
        }
    }
    return FALSE;
#endif

    if (device_bind_status.status.bf.ryeex_bind || device_bind_status.status.bf.mi_bind) {
        return TRUE;
    }

    return FALSE;
}


void dip_tx_resp_cb(ry_sts_t status, void* usrdata)
{
    LOG_DEBUG("[dip_tx_resp_cb] TX Callback, cmd:%s, status:0x%04x\r\n", (char*)usrdata, status);
    return;
}




ry_sts_t device_info_send(void)
{

    uint8_t *buf = ry_malloc(100 + sizeof(DeviceInfo));
    size_t message_length = 0;
    ry_sts_t status;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    pb_ostream_t stream;

    DeviceInfo info = DeviceInfo_init_zero;

    if(buf == NULL){
        status_code = RBP_RESP_CODE_NO_MEM;
        message_length = 0;
        goto exit;
    }

    /* Create a stream that will write to our buffer. */
    stream = pb_ostream_from_buffer(buf, (100 + sizeof(DeviceInfo)));


    /* Encode Status message */ 
    ry_memcpy(info.did, device_info.did, DID_LEN);
    ry_memcpy(info.mac.bytes, device_info.mac, MAC_LEN);
    info.mac.size = 6;
    ry_memcpy(info.fw_ver, device_info.fw_ver, FW_VER_LEN);
    ry_memcpy(info.model, device_info.model, MODEL_LEN);
    ry_memcpy(info.uid, device_info.uid, UID_LEN);

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceInfo_fields, &info)) {
        //LOG_ERROR("[device_info_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        message_length = 0;
        goto exit;
    }
    message_length = stream.bytes_written;

    LOG_DEBUG("[device_info_send] Devcie Info: ");
    ry_data_dump(buf, message_length, ' ');

    //return rbp_send_resp(CMD_DEV_GET_DEV_INFO, RBP_RESP_CODE_SUCCESS, buf, message_length, 1);

    //return ry_ble_txMsgReq(CMD_DEV_GET_DEV_INFO, RY_BLE_TYPE_RESPONSE, RBP_RESP_CODE_SUCCESS,
    //            buf, message_length, 1, dip_tx_resp_cb, buf);

exit:
    status = ble_send_response(CMD_DEV_GET_DEV_INFO, status_code, 
                buf, message_length, 1, dip_tx_resp_cb, "device_info_send");

    ry_free(buf);
     
    return status;
}

void aes_cbc_sign(u8_t* data, int len, u8_t* sign)
{
    ry_sts_t status;
    u8_t  this_iv[16];
    int last_len = 0;
    int cnt = 0;

    last_len = len%16;
    cnt = last_len ? (len>>4)+1 : (len>>4);
    
    u8_t* output = ry_malloc(cnt<<4);
    if (output == NULL) {
        return;
    }

    ry_memcpy(this_iv, device_info.iv, IV_LEN);
    //ry_memcpy(data, this_iv, IV_LEN);

    LOG_DEBUG("AES key: ");
    ry_data_dump(device_info.aes_key, 16, ' ');
    status = ry_aes_cbc_encrypt(device_info.aes_key, this_iv, len, data, output);
    if (status != RY_SUCC) {
        LOG_ERROR("[aes_cbc_sign] Fail.\r\n");
        return;
    }

    LOG_DEBUG("This Sign: ");
    ry_data_dump(&output[(cnt-1)<<4], 16, ' ');

    ry_memcpy(sign, device_info.iv, IV_LEN);
    ry_memcpy(sign+IV_LEN, &output[(cnt-1)<<4], 16);
}

u8_t g_test_key[] = {0x06, 0x1B, 0x67, 0xC1, 0xDF, 0x5D, 0x76, 0x26, 0xFC, 0xC6, 0xD7, 0x3B, 0x1F, 0x0F, 0x85, 0xF3};
bool aes_cbc_sign_verify(u8_t* data, int len, u8_t* sign)
{
    ry_sts_t status;
    u8_t  this_iv[16];
    int last_len = 0;
    int cnt = 0;

    last_len = len%16;
    cnt = last_len ? (len>>4)+1 : (len>>4);
    
    u8_t* output = ry_malloc(cnt<<4);
    if (output == NULL) {
        return 0;
    }

    //ry_memcpy(this_iv, device_info.iv, IV_LEN);
    //ry_memcpy(data, this_iv, IV_LEN);

    ry_memcpy(this_iv, sign, IV_LEN);


    status = ry_aes_cbc_encrypt(device_info.aes_key, this_iv, len, data, output);
    //status = ry_aes_cbc_encrypt(g_test_key, this_iv, len, data, output);
    if (status != RY_SUCC) {
        LOG_ERROR("[aes_cbc_sign_verify] Fail.\r\n");
        return FALSE;
    }

    LOG_DEBUG("This Sign: ");
    ry_data_dump(&output[(cnt-1)<<4], 16, ' ');
    
    if (0 == ry_memcmp(sign+IV_LEN, &output[(cnt-1)<<4], 16)) {
        LOG_DEBUG("AES Sign verify OK.\r\n");
        return TRUE;
    } else {
        LOG_ERROR("AES Sign verify fail.\r\n");
        return FALSE;
    }

}



void device_sign_gen(u32_t sn, u32_t nonce, u8_t* data, u16_t* len)
{
    ry_sts_t status;
    u8_t buf[32] = {0};
    int didLen = strlen((char*)device_info.did);
    u8_t this_iv[16];
    
    ry_memcpy(buf, (u8_t*)&sn, 4);
    ry_memcpy(buf+4, (u8_t*)&nonce, 4);
    ry_memcpy(buf+8, device_info.did, didLen);
    *len = 8 + didLen;

    if (*len > 16) {
        *len = 32;
    } else {
        *len = 16;
    }

    ry_memcpy(this_iv, device_info.iv, IV_LEN);
    ry_memcpy(data, this_iv, IV_LEN);
    
    status = ry_aes_cbc_encrypt(device_info.aes_key, this_iv, *len, buf, data+16);
    if (status != RY_SUCC) {
        LOG_ERROR("[device_sign_gen] Fail.\r\n");
    }

    /* Add iv len */
    *len += 16; 
}

ry_sts_t server_sign_verify(u32_t sn, u32_t nonce, u8_t* data, u16_t len)
{
    u8_t this_iv[16];
    ry_sts_t status;
    u32_t decrypt_nonce;
    u32_t decrypt_sn;

    ry_memcpy(this_iv, device_info.iv, IV_LEN);
    status = ry_aes_cbc_decrypt(device_info.aes_key, this_iv, len, data, data);
    if (status != RY_SUCC) {
        LOG_ERROR("[server_sign_verify] Fail. %x\r\n", status);
        return status;
    } else {
        LOG_DEBUG("[server_sign_verify] Decrypt data: ");
        ry_data_dump(data, len, ' ');
    

        /* Verify nonce */
        ry_memcpy((u8_t*)&decrypt_nonce, data+IV_LEN+4, 4);
        if (decrypt_nonce != nonce) {
            LOG_ERROR("[server_sign_verify] Verify IV fail.\r\n");
            return RY_SET_STS_VAL(RY_CID_AES, RY_ERR_DECRYPTION);
        }

        /* Verify DID */
        //ry_memcpy(`device_info.did, data+IV_LEN, 4);
        //LOG_DEBUG("[server_sign_verify] SN = %d\r\n", decrypt_sn);
    }
   
    return RY_SUCC;
}


ry_sts_t device_info_cert(void)
{

    uint8_t *buf = (uint8_t*)ry_malloc(sizeof(DeviceCredential) + 100);
    size_t message_length;
    ry_sts_t status;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;

    DeviceCredential *cert = (DeviceCredential *)ry_malloc(sizeof(DeviceCredential));
    ry_memset((void*)cert, 0, sizeof(DeviceCredential));
    ry_memset((void*)buf, 0, sizeof(DeviceCredential) + 100);

    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(buf, sizeof(DeviceCredential) +100);

    if((cert == NULL)||(buf == NULL)){
        status_code = RBP_RESP_CODE_NO_MEM;
        message_length = 0;
        goto exit;
    }


    /* Encode Status message */ 
    strcpy(cert->did, (char*)device_info.did);
    cert->sn       = device_bind_status.device_sn++;
    cert->sign_ver = 2;
    cert->nonce    = ry_rand();
    device_sign_gen(cert->sn, cert->nonce, cert->sign.bytes, &cert->sign.size);
    

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceCredential_fields, cert)) {
        //LOG_ERROR("[device_info_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        message_length = 0;
        goto exit;
    }
    message_length = stream.bytes_written;

    LOG_DEBUG("[device_info_cert] Device Cert: ");
    ry_data_dump(buf, message_length, ' ');
    device_bind_info_save();

    //rbp_send_resp(CMD_DEV_GET_DEV_CREDENTIAL, RBP_RESP_CODE_SUCCESS, buf, message_length, 1);
    //ry_ble_txMsgReq(CMD_DEV_GET_DEV_CREDENTIAL, RY_BLE_TYPE_RESPONSE, RBP_RESP_CODE_SUCCESS,
    //            buf, message_length, 1, dip_tx_resp_cb, NULL);

exit:
    status = ble_send_response(CMD_DEV_GET_DEV_CREDENTIAL, status_code, 
                buf, message_length, 1, dip_tx_resp_cb, "device_info_cert");

    ry_free((void*)cert);
    ry_free((void*)buf);
     
    return status;
}




ry_sts_t device_status_send(void)
{
    uint8_t *buf = (uint8_t*)ry_malloc(sizeof(DeviceStatus) + 100);
    size_t message_length = 0;
    ry_sts_t ret;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    DeviceStatus status = DeviceStatus_init_zero;
	pb_ostream_t stream;

    if(buf == NULL){
        status_code = RBP_RESP_CODE_NO_MEM;
        message_length = 0;
        goto exit;
    }

    /* Create a stream that will write to our buffer. */
    stream = pb_ostream_from_buffer(buf, sizeof(DeviceStatus) + 100);


    /* Encode Status message */ 
    ry_memcpy(status.did, device_info.did, DID_LEN);
    status.se_activate_status         = device_bind_status.status.bf.se_activate;
    status.ryeex_cert_activate_status = device_bind_status.status.bf.ryeex_cert_activate;
    status.mi_cert_activate_status    = device_bind_status.status.bf.mi_cert_activate;
    status.ryeex_bind_status          = device_bind_status.status.bf.ryeex_bind;
    status.mi_bind_status             = device_bind_status.status.bf.mi_bind;
    status.pb_max                     = MAX_R_XFER_SIZE;
    status.ble_conn_interval          = ry_ble_get_connInterval();
    status.has_fw_ver = 1;
    status.has_check_status = (storage_get_lost_file_status())?(1):(0);
    status.check_status = (storage_get_lost_file_status())?(1):(0);
    ry_memcpy(status.fw_ver, device_info.fw_ver, FW_VER_LEN);
    if (mible_isSnEmpty() == 0) {
        status.has_mi_did = 1;
        ry_memset(status.mi_did, 0, 20);
        mible_getSn((u8_t*)status.mi_did);
    } else {
        status.has_mi_did = 0;
        status.mi_bind_status = 0;
    }
    status.accept_sec_suite_count = 2;
    status.accept_sec_suite[0] = 2;
    status.accept_sec_suite[1] = 3;
    if (device_bind_status.ryeex_uid[0] == 0xFF && device_bind_status.ryeex_uid[1] == 0xFF)  {
        status.has_ryeex_uid = 0;
        ry_memset(status.ryeex_uid, 0, 20);
    } else {
        status.has_ryeex_uid = 1;
        //strcpy(status.ryeex_uid, (char*)device_bind_status.ryeex_uid); 

        if(strlen((char const*)device_bind_status.ryeex_uid) >= 20){
            ry_memset(status.ryeex_uid, 0, 20);
            status.ryeex_bind_status = 0;
            status.mi_bind_status = 0;
        }else{
            ry_memcpy(status.ryeex_uid, (char*)device_bind_status.ryeex_uid, 20);
        }
    }
    
    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceStatus_fields, &status)) {
        LOG_ERROR("[device_status_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        message_length = 0;
        goto exit;
    }
    message_length = stream.bytes_written;

    //return rbp_send_resp(CMD_DEV_GET_DEV_STATUS, RBP_RESP_CODE_SUCCESS, buf, message_length, 0);

    //return ry_ble_txMsgReq(CMD_DEV_GET_DEV_STATUS, RY_BLE_TYPE_RESPONSE, RBP_RESP_CODE_SUCCESS,
    //            buf, message_length, 0, dip_tx_resp_cb, NULL);

exit:

    ret = ble_send_response(CMD_DEV_GET_DEV_STATUS, status_code, 
                buf, message_length, 0, dip_tx_resp_cb, "device_status_send");

    ry_free(buf);

    bind_ack_once  = 0;
    allow_bind_ack = 0;
    return ret;
}

ry_sts_t device_state_send(void)
{
    uint8_t *buf = (uint8_t*)ry_malloc(sizeof(DeviceRunState) + 100);
    size_t message_length = 0;
    ry_sts_t ret;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    DeviceRunState state = DeviceRunState_init_zero;
	pb_ostream_t stream;

    if(buf == NULL){
        status_code = RBP_RESP_CODE_NO_MEM;
        message_length = 0;
        goto exit;
    }

    /* Create a stream that will write to our buffer. */
    stream = pb_ostream_from_buffer(buf, sizeof(DeviceRunState) + 100);


    /* Encode Status message */ 
    state.main_state = get_device_session();
    
    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceRunState_fields, &state)) {
        LOG_ERROR("[device_state_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        message_length = 0;
        goto exit;
    }
    message_length = stream.bytes_written;

exit:

    ret = ble_send_response(CMD_DEV_GET_DEV_RUN_STATE, status_code, 
                buf, message_length, 0, dip_tx_resp_cb, (void*)__FUNCTION__);

    ry_free(buf);
    return ret;
}


ry_sts_t device_bind_ack_handler(uint8_t* data, int len)
{
    BindResult msg;
    int status;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, BindResult_fields, &msg);

    if (msg.error != 0) {
        LOG_ERROR("[device_bind_ack_handler] Bind result fail. Error code: %d\r\n", msg.error);
        ry_sched_msg_send(IPC_MSG_TYPE_BIND_FAIL, 0, NULL);
        return RY_SUCC;
    }
    
    if(msg.has_uid) {
        device_bind_status.status.bf.ryeex_bind = 1;
        ry_memset(device_bind_status.ryeex_uid, 0, MAX_RYEEX_ID_LEN);
        strcpy((char*)device_bind_status.ryeex_uid, msg.uid);  
        LOG_DEBUG("[device_bind_ack_handler] uid: %s\n", device_bind_status.ryeex_uid);
    }

    bind_ack_once  = 0;
    allow_bind_ack = 0;

    device_bind_info_save();
    ancsOnRySetBondFinished();
	ry_sched_msg_send(IPC_MSG_TYPE_BIND_SUCC, 0, NULL);
    ry_system_lifecycle_schedule();
    
    
    return RY_SUCC;
}


void device_bind_mijia(u8_t bind)
{
    if (bind == 1) {
        device_bind_status.status.bf.mi_bind = 1;
    } else {
        device_bind_status.status.bf.mi_bind = 0;
    }
    device_bind_info_save();
}

void device_start_bind_handler(void)
{
    ry_sched_msg_send(IPC_MSG_TYPE_BIND_WAIT_ACK, 0, NULL);
    motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 200);
    dev_bind_status = DEV_BIND_STATE_WAIT_USER_ACK;
    ble_send_response(CMD_DEV_BIND_ACK_START, RBP_RESP_CODE_SUCCESS, 
                NULL, 0, 0, dip_tx_resp_cb, (void*)__FUNCTION__);
    qrcode_timer_stop();
    allow_bind_ack = 1;
    bind_ack_once = 0;
    
}

void device_bind_ack_cancel_handler(void)
{
    ry_sched_msg_send(IPC_MSG_TYPE_BIND_ACK_CANCEL, 0, NULL);
    dev_bind_status = DEV_BIND_STATE_UNBIND;
    ble_send_response(CMD_DEV_BIND_ACK_CANCEL, RBP_RESP_CODE_SUCCESS, 
                NULL, 0, 0, dip_tx_resp_cb, (void*)__FUNCTION__);

    allow_bind_ack = 0;
    bind_ack_once = 0;
    
}


ry_sts_t device_time_sync(uint8_t* data, int len)
{
    /* Sync the time with APP */
    int status;
    //int diff;

    RbpMsg_Int32 msg;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, RbpMsg_Int32_fields, &msg);
    ryos_utc_set(msg.val);
    ry_hal_update_rtc_time();

    alipay_vendor_sync_time_done();
    //LOG_DEBUG("diff time: %d\n", diff);

    //ry_data_dump(data, len, ' ');
    //return rbp_send_resp(CMD_DEV_SET_TIME, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    //return ry_ble_txMsgReq(CMD_DEV_SET_TIME, RY_BLE_TYPE_RESPONSE, RBP_RESP_CODE_SUCCESS,
    //            NULL, 0, 1, dip_tx_resp_cb, NULL);

    return ble_send_response(CMD_DEV_SET_TIME, RBP_RESP_CODE_SUCCESS, 
                NULL, 0, 1, dip_tx_resp_cb, "device_time_sync");
}

ry_sts_t device_timezone_sync(uint8_t* data, int len)
{
    /* Sync the time with APP */
    int status;
    //int diff;

    TimeZoneSet timezone;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, TimeZoneSet_fields, &timezone);
    
//    LOG_INFO("--- got timezone request ---");
//    
//    LOG_INFO("utc_time: %d", timezone.utc_time);
//    LOG_INFO("deviation, avail: %d, %d", timezone.has_deviation, timezone.deviation);
//    LOG_INFO("daylight, avail: %d, %d", timezone.has_daylight, timezone.daylight);
    
    set_timezone(timezone.deviation + (timezone.daylight * 3600));
    ryos_utc_set(timezone.utc_time);
    
    ry_hal_update_rtc_time();

    alipay_vendor_sync_time_done();

    return ble_send_response(CMD_DEV_SET_TIME_PARAM, RBP_RESP_CODE_SUCCESS, 
                NULL, 0, 1, dip_tx_resp_cb, "device_timezone_sync");
}


void ryeex_unbind()
{
    device_bind_status.status.bf.ryeex_bind = 0;
    ry_memset(device_bind_status.ryeex_uid, 0xFF, MAX_RYEEX_ID_LEN);

    app_notify_clear_all();
    wms_activity_back_to_root(NULL);
    clear_user_data();

    LOG_INFO("[DIP] ryeex_unbind\n");
}

void mijia_unbind()
{
    device_bind_status.status.bf.mi_bind = 0;
    ry_memset(device_bind_status.mi_uid, 0xFF, MAX_MI_ID_LEN);

    app_notify_clear_all();
    scene_tbl_reset();
    LOG_INFO("[DIP] mijia_unbind\n");
}


void unbind(void)
{
    //mible_psm_reset();
    //ry_memset(device_info.uid, 0, UID_LEN);
    //dip_psm_save();

    mijia_unbind();
    ryeex_unbind();
    device_clear_all_user_info();

    device_bind_info_save();

    ry_system_lifecycle_schedule();
    
    alarm_tbl_reset();

    scene_tbl_reset();
    alipay_unbinding();
}

void ry_device_unbind(ry_ipc_msg_t* msg)
{
    unbind();
    /* Re-enable touch for the synchronize operation is done */
    touch_enable();

    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_DISCONNECT, 0, NULL);

    ryos_delay_ms(1000);

    ry_system_reset();

    //wms_activity_jump(&activity_surface, NULL);
}


bool is_mijia_bond(void)
{
#if DEV_MIJIA_DEBUG
    return true;
#else    
    return device_bind_status.status.bf.mi_bind;
#endif
}

bool is_ryeex_bond(void)
{
    return device_bind_status.status.bf.ryeex_bind;
}




void dip_send_bind_ack(int code)
{
    pb_ostream_t stream_o = {0};
    BindAckResult ack;
    u8_t buf[10];
    

    LOG_DEBUG("[dip_send_bind_ack] ack_once:%d, allow_bind_ack:%d\r\n", bind_ack_once, allow_bind_ack);
    if (bind_ack_once == 1 || allow_bind_ack == 0) {
        return;
    }

    stream_o = pb_ostream_from_buffer(buf, (sizeof(BindAckResult)));
    ack.code = code;
    if (!pb_encode(&stream_o, BindAckResult_fields, (void*)(&ack))) {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
        return;
    }
    

    LOG_DEBUG("stream bytes ---- %d\n", stream_o.bytes_written);
    ble_send_request(CMD_APP_BIND_ACK_RESULT,  buf, stream_o.bytes_written, 
                0, dip_tx_resp_cb, (void*)"dip_send_bind_ack");

    bind_ack_once = 1;

}


void device_mi_unbind_token_handler(void)
{
    int8_t *buf = ry_malloc(100);//[100] = {0};
    u8_t *input = ry_malloc(32);
    size_t message_length = 0;
    ry_sts_t status;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    int offset;
	pb_ostream_t stream;

    DeviceMiUnBindToken unbind = DeviceMiUnBindToken_init_zero;

    if(buf == NULL || input == NULL){
        status_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    /* Create a stream that will write to our buffer. */
    stream = pb_ostream_from_buffer((pb_byte_t*)buf, 100);

    /* Encode Status message */ 
    unbind.token_type = TokenType_DEVICE_MI_UNBIND;
    ry_memcpy(unbind.ryeex_did, device_info.did, DID_LEN);
    unbind.sn        = device_bind_status.device_sn++;
    unbind.sign_ver  = 3;
    unbind.nonce     = ry_rand(); // Using Random

    /* Generate signature */
    ry_memset(input, 0, 32);
    offset = 0;
    ry_memcpy(input, (u8_t*)&unbind.token_type, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&unbind.sn, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&unbind.nonce, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&unbind.ryeex_did, strlen(unbind.ryeex_did));
    offset += strlen(unbind.ryeex_did);
    aes_cbc_sign(input, offset, unbind.sign.bytes);
    unbind.sign.size = 32;

    //device_sign_gen(unbind.sn, unbind.nonce, unbind.sign.bytes, &unbind.sign.size);

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceMiUnBindToken_fields, &unbind)) {
        //LOG_ERROR("[device_status_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        goto exit;
    }
    message_length = stream.bytes_written;

    LOG_DEBUG("[device_mi_unbind_token_handler] Dev Mi Unbind Token: ");
    ry_data_dump((u8_t*)buf, message_length, ' ');

    device_bind_info_save();


exit:
    status = ble_send_response(CMD_DEV_GET_DEV_MI_UNBIND_TOKEN, status_code, 
                (u8_t*)buf, message_length, 0, dip_tx_resp_cb, "device_unbind_token_send");

    ry_free(buf);
    ry_free(input);
}


void device_mi_ryeex_did_token_handler(void)
{
    int8_t *buf = ry_malloc(100);//[100] = {0};
    u8_t *input = ry_malloc(64);
    size_t message_length = 0;
    ry_sts_t status;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    int offset;
	pb_ostream_t stream;

    DeviceMiRyeexDidToken token = DeviceMiRyeexDidToken_init_zero;

    if(buf == NULL || input == NULL){
        status_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    /* Create a stream that will write to our buffer. */
    stream = pb_ostream_from_buffer((pb_byte_t*)buf, 100);

    /* Encode Status message */ 
    token.token_type = TokenType_DEVICE_MI_RYEEX_DID;
    token.sn         = device_bind_status.device_sn++;
    token.sign_ver   = 3;
    token.nonce      = ry_rand(); // Using Random
    ry_memcpy(token.ryeex_did, device_info.did, DID_LEN);
    mible_getSn((u8_t*)token.mi_did);

    /* Generate signature */
    ry_memset(input, 0, 64);
    offset = 0;
    ry_memcpy(input, (u8_t*)&token.token_type, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&token.sn, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&token.nonce, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&token.ryeex_did, strlen(token.ryeex_did));
    offset += strlen(token.ryeex_did);
    ry_memcpy(input+offset, (u8_t*)&token.mi_did, strlen(token.mi_did));
    offset += strlen(token.mi_did);
    aes_cbc_sign(input, offset, token.sign.bytes);
    token.sign.size = 32;

    //device_sign_gen(unbind.sn, unbind.nonce, unbind.sign.bytes, &unbind.sign.size);

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceMiRyeexDidToken_fields, &token)) {
        //LOG_ERROR("[device_status_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        goto exit;
    }
    message_length = stream.bytes_written;

    LOG_DEBUG("[device_mi_ryeex_did_token_handler] Dev Mi Ryeex Did Token: ");
    ry_data_dump((u8_t*)buf, message_length, ' ');

    device_bind_info_save();


exit:
    status = ble_send_response(CMD_DEV_GET_DEV_MI_RYEEX_DID_TOKEN, status_code, 
                (u8_t*)buf, message_length, 0, dip_tx_resp_cb, "device_unbind_token_send");

    ry_free(buf);
    ry_free(input);
}



ry_sts_t device_unbind_token_send(void)
{
    uint8_t *buf = ry_malloc(100);//[100] = {0};
    size_t message_length = 0;
    ry_sts_t status;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
	pb_ostream_t stream;

    DeviceUnBindToken unbind = DeviceUnBindToken_init_zero;

    if(buf == NULL){
        status_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    /* Create a stream that will write to our buffer. */
    stream = pb_ostream_from_buffer(buf, 100);

    /* Encode Status message */ 
    ry_memcpy(unbind.did, device_info.did, DID_LEN);
    unbind.sn        = device_bind_status.device_sn++;
    unbind.sign_ver  = 2;
    unbind.nonce     = ry_rand();

    device_sign_gen(unbind.sn, unbind.nonce, unbind.sign.bytes, &unbind.sign.size);

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream, DeviceUnBindToken_fields, &unbind)) {
        //LOG_ERROR("[device_status_send] Encoding failed.\r\n");
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
        goto exit;
    }
    message_length = stream.bytes_written;

    LOG_DEBUG("[device_unbind_token_send] Unbind Token: ");
    ry_data_dump(buf, message_length, ' ');

    device_bind_info_save();

    /* Todo: Send message to upper layer */

    //return rbp_send_resp(CMD_DEV_GET_DEV_UNBIND_TOKEN, RBP_RESP_CODE_SUCCESS, buf, message_length, 0);

    //return ry_ble_txMsgReq(CMD_DEV_GET_DEV_UNBIND_TOKEN, RY_BLE_TYPE_RESPONSE, RBP_RESP_CODE_SUCCESS,
    //            buf, message_length, 0, dip_tx_resp_cb, buf);

exit:
    status = ble_send_response(CMD_DEV_GET_DEV_UNBIND_TOKEN, status_code, 
                buf, message_length, 0, dip_tx_resp_cb, "device_unbind_token_send");

    ry_free(buf);
    return status;
}

static uint32_t device_clear_all_user_info(void)
{
    if(is_mijia_bond()||is_ryeex_bond())
    {
        //do nothing
    }
    else
    {
        LOG_INFO("[unbind] Start Clear All user Info\n");
        AppDbDeleteAllRecords();
        ancsOnRemoteRyUnbindFinished();
        surface_on_reset();
    }
    return 0;
}

ry_sts_t device_unbind_handler(void)
{
    /* Send response */
    //rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    
    /* Unbind */
    /*ryeex_unbind();
    device_bind_info_save();
    ry_system_lifecycle_schedule();
	alarm_tbl_reset();
	device_clear_all_user_info();*/
	unbind();

    ry_ble_ctrl_msg_send(RY_BLE_CTRL_MSG_DISCONNECT, 0, NULL);
    
	
    return RY_SUCC;
}


void unsecure_unbind_resp_tx_cb(ry_sts_t status, void* usrdata)
{
    LOG_DEBUG("[device_unbind_handler_unsecure] unsecure_unbind_resp_tx_cb\r\n");
    if (is_bond() == FALSE) {
	    //ry_system_lifecycle_schedule();
	    alarm_tbl_reset();
//        device_clear_all_ble_bond_info();
    }
    return;
}


ry_sts_t device_unbind_handler_unsecure(uint8_t* data, int len)
{
    DeviceUnBindToken msg;
    int status;
    ry_sts_t verify_result;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, DeviceUnBindToken_fields, &msg);

    LOG_INFO("[%s]\r\n", __FUNCTION__);

    
    LOG_DEBUG("[device_unbind_handler_unsecure] Sign Ver: %d\r\n", msg.sign_ver);
    LOG_DEBUG("[device_unbind_handler_unsecure] Nonce: %d\r\n", msg.nonce);
    LOG_DEBUG("[device_unbind_handler_unsecure] DID: %s\r\n", msg.did);
    LOG_DEBUG("[device_unbind_handler_unsecure] Sn: %d\r\n", msg.sn);
    LOG_DEBUG("[device_unbind_handler_unsecure] Sign size: %d\r\n", msg.sign.size);
    LOG_DEBUG("[device_unbind_handler_unsecure] Sign: %d\r\n");
    ry_data_dump(msg.sign.bytes, msg.sign.size, ' ');

    verify_result = server_sign_verify(msg.sn, msg.nonce, msg.sign.bytes, msg.sign.size);
    if (verify_result == RY_SUCC) {
		/*ryeex_unbind();
        device_bind_info_save();    
        device_clear_all_user_info();*/
        unbind();
        
        LOG_DEBUG("[device_unbind_handler_unsecure] ble_send_response\r\n");
        //return rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);
        status = ble_send_response(CMD_DEV_UNBIND, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, unsecure_unbind_resp_tx_cb, "device_unbind_handler_unsecure");

        device_bind_status.server_sn = msg.sn;
        
    } else {
        //rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SIGN_VERIFY_FAIL, NULL, 0, 0);
        status = ble_send_response(CMD_DEV_UNBIND, RBP_RESP_CODE_SIGN_VERIFY_FAIL, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_unbind_handler_unsecure");
    }

    return status;
   
}

ry_sts_t device_unbind_request(void)
{
    LOG_DEBUG("Send unbind request.\n");
    return rbp_send_req(CMD_APP_UNBIND, NULL, 0, 1);
}






void device_mi_unbind_handler(uint8_t* data, int len)
{
    ServerMiUnBindToken msg;
    int status;
    ry_sts_t verify_result;
    int result;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    int offset = 0;
    u8_t *input = ry_malloc(32);
    int token_type = 0;

    if (input == NULL) {
        status_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    status = pb_decode(&stream, ServerMiUnBindToken_fields, &msg);
    
    LOG_DEBUG("[device_mi_unbind_handler] Sign Ver: %d\r\n", msg.sign_ver);
    LOG_DEBUG("[device_mi_unbind_handler] Nonce: %d\r\n", msg.nonce);
    LOG_DEBUG("[device_mi_unbind_handler] DID: %s\r\n", msg.ryeex_did);
    LOG_DEBUG("[device_mi_unbind_handler] Sn: %d\r\n", msg.sn);
    LOG_DEBUG("[device_mi_unbind_handler] Sign size: %d\r\n", msg.sign.size);
    LOG_DEBUG("[device_mi_unbind_handler] Sign: %d\r\n");
    ry_data_dump(msg.sign.bytes, msg.sign.size, ' ');

    /* Generate signature */
    ry_memset(input, 0, 32);
    offset = 0;
    token_type = msg.token_type;
    ry_memcpy(input, (u8_t*)&token_type, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&msg.sn, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&msg.nonce, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&msg.ryeex_did, strlen(msg.ryeex_did));
    offset += strlen(msg.ryeex_did);

    result = aes_cbc_sign_verify(input, offset, msg.sign.bytes);

    //verify_result = server_sign_verify(msg.sn, msg.nonce, msg.sign.bytes, msg.sign.size);
    if (result == TRUE) {
        LOG_ERROR("Mijia Unbind from Servr_Mi_Unbind Token.\r\n");
        device_bind_status.server_sn = msg.sn;
        mijia_unbind();
        device_bind_info_save();
        device_clear_all_user_info();
		
        //return rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);
        status = ble_send_response(CMD_DEV_MI_UNBIND, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, unsecure_unbind_resp_tx_cb, "device_mi_unbind_handler");
    } else {
        //rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SIGN_VERIFY_FAIL, NULL, 0, 0);
        status = ble_send_response(CMD_DEV_MI_UNBIND, RBP_RESP_CODE_SIGN_VERIFY_FAIL, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_mi_unbind_handler");
    }

    ry_free(input);
    return;


exit:
    status = ble_send_response(CMD_DEV_MI_UNBIND, status_code, 
                NULL, 0, 0, dip_tx_resp_cb, "device_mi_unbind_handler");

    ry_free(input);
    return;


}

void device_ryeex_bind_by_token_handler(uint8_t* data, int len)
{
    ServerRyeexBindToken msg;
    int status;
    ry_sts_t verify_result;
    int result;
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    int offset = 0;
    int token_type = 0;
    u8_t *input = ry_malloc(80);

    if (input == NULL) {
        status_code = RBP_RESP_CODE_NO_MEM;
        goto exit;
    }

    status = pb_decode(&stream, ServerRyeexBindToken_fields, &msg);
    
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] Sign Ver: %d\r\n", msg.sign_ver);
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] Nonce: %d\r\n", msg.nonce);
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] DID: %s\r\n", msg.ryeex_did);
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] Token: \r\n");
    ry_data_dump(msg.ryeex_device_token.bytes, 12, ' ');
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] Sn: %d\r\n", msg.sn);
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] Sign size: %d\r\n", msg.sign.size);
    LOG_DEBUG("[device_ryeex_bind_by_token_handler] Sign: %d\r\n");
    ry_data_dump(msg.sign.bytes, msg.sign.size, ' ');

    /* Generate signature */
    ry_memset(input, 0, 80);
    offset = 0;
    token_type = msg.token_type;
    ry_memcpy(input, (u8_t*)&token_type, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&msg.sn, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&msg.nonce, 4);
    offset += 4;
    ry_memcpy(input+offset, (u8_t*)&msg.ryeex_did, strlen(msg.ryeex_did));
    offset += strlen(msg.ryeex_did);
    ry_memcpy(input+offset, (u8_t*)&msg.ryeex_uid, strlen(msg.ryeex_uid));
    offset += strlen(msg.ryeex_uid);
    ry_memcpy(input+offset, (u8_t*)&msg.ryeex_device_token.bytes, 12);
    offset += 12;

    result = aes_cbc_sign_verify(input, offset, msg.sign.bytes);
    

    //verify_result = server_sign_verify(msg.sn, msg.nonce, msg.sign.bytes, msg.sign.size);
    if (result == TRUE) {
        device_bind_status.server_sn = msg.sn;

        /* TODO: Goto token bind procedure */
        mible_setToken(msg.ryeex_device_token.bytes);
        ry_memset(device_bind_status.ryeex_uid, 0, MAX_RYEEX_ID_LEN);
        strcpy((char*)device_bind_status.ryeex_uid, msg.ryeex_uid);
        device_bind_status.status.bf.ryeex_bind = 1;
        device_bind_info_save();
		
        //return rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);
        status = ble_send_response(CMD_DEV_RYEEX_BIND_BY_TOKEN, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_ryeex_bind_by_token_handler");
    } else {
        //rbp_send_resp(CMD_DEV_UNBIND, RBP_RESP_CODE_SIGN_VERIFY_FAIL, NULL, 0, 0);
        status = ble_send_response(CMD_DEV_RYEEX_BIND_BY_TOKEN, RBP_RESP_CODE_SIGN_VERIFY_FAIL, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_ryeex_bind_by_token_handler");
    }

    ry_free(input);
    return;


exit:
    status = ble_send_response(CMD_DEV_RYEEX_BIND_BY_TOKEN, status_code, 
                NULL, 0, 0, dip_tx_resp_cb, "device_mi_unbind_handler");

    ry_free(input);
    return;
}



ry_sts_t device_info_get(u8_t type, u8_t* data, u8_t* len)
{
    switch (type) {
        case DEV_INFO_TYPE_MAC:
            ry_memcpy(data, device_info.mac, MAC_LEN);
            *len = MAC_LEN;
            break;

        case DEV_INFO_TYPE_SN:
            ry_memcpy(data, device_info.sn, FACTORY_SN_LEN);
            *len = FACTORY_SN_LEN;
            break;

        case DEV_INFO_TYPE_DID:
            ry_memcpy(data, device_info.did, DID_LEN);
            *len = DID_LEN;
            break;

        case DEV_INFO_TYPE_MODEL:
            ry_memcpy(data, device_info.model, MODEL_LEN);
            *len = MODEL_LEN;
            break;

        case DEV_INFO_TYPE_VER:
            ry_memcpy(data, device_info.fw_ver, FW_VER_LEN);
            *len = FW_VER_LEN;
            break;

        case DEV_INFO_TYPE_UID:
            ry_memcpy(data, device_info.uid, UID_LEN);
            *len = UID_LEN;
            break;

        case DEV_INFO_TYPE_HW_VER:
            ry_memcpy(data, device_info.hw_ver, HW_VER_LEN);
            *len = HW_VER_LEN;
            break;

        default:
            break;
    }

    return RY_SUCC;
}


/**
 * @brief   Device Infomation Property Initilization
 *
 * @param   None
 *
 * @return  None
 */
void device_info_init(void)
{
    char model[MODEL_LEN] = "ryeex.band.sake.v1";
    ry_memcpy(device_info.model, model, MODEL_LEN);
    
    device_info.battery = 100;
    device_info.cur_time = 0;

    ry_hal_flash_init();

    device_info_load();
    device_bind_info_restore();
    get_device_setting();
    device_state_restore();
    device_security_load();

#if DBG_HRM_AUTO_SAMPLE
    set_auto_heart_rate_open();
#endif

}


int device_onNfcEvent(ry_ipc_msg_t* msg)
{
    u8_t status;
    LOG_DEBUG("[device_onNfcEvent]\r\n");

    switch (msg->msgType) {
        case IPC_MSG_TYPE_NFC_INIT_DONE:
            ry_nfc_se_channel_open();
            //rbp_send_resp(CMD_DEV_ACTIVATE_SE_START, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);

            ble_send_response(CMD_DEV_ACTIVATE_SE_START, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_onNfcEvent");
            break;
       

        case IPC_MSG_TYPE_NFC_CE_OFF_EVT:
            break;

        default:
            break;

    }

    return 1;
}



void device_activate_start(void)
{
    ry_sts_t se_status;
    ry_sts_t status;
    #if 0
	
    /* Check NFC state */
    if (ry_nfc_get_state() == RY_NFC_STATE_IDLE) {
        ry_sched_addNfcEvtListener(device_onNfcEvent);
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
        return;
    }

    /* Open the SE wired channel */
    se_status = ry_nfc_se_channel_open();
    if (se_status) {
        LOG_DEBUG("[device_activate_start] Open SE channel fail. %x\r\n", se_status);
    }
    #endif

    //rbp_send_resp(CMD_DEV_ACTIVATE_SE_START, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);

    status = ble_send_response(CMD_DEV_ACTIVATE_SE_START, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_activate_start");

    set_device_session(DEV_SESSION_CARD);
}




void device_se_activate_result(uint8_t* data, int len)
{
    SeActivateResult msg;
    int status;
    ry_sts_t ret;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, SeActivateResult_fields, &msg);

    if (msg.code == 0) {
        // Success
        LOG_INFO("[device_se_activate_result] SE Activate Success\r\n");
        device_bind_status.status.bf.se_activate = 1;
        device_bind_info_save();
    } else {
        // Create Fail, Do nothing.
        LOG_ERROR("[device_se_activate_result] SE Activate fail. error code: %d\r\n", msg.code);
        device_bind_status.status.bf.se_activate = 0;
    }

    //rbp_send_resp(CMD_DEV_ACTIVATE_SE_RESULT, RBP_RESP_CODE_SUCCESS, NULL, 0, 0);

    ret = ble_send_response(CMD_DEV_ACTIVATE_SE_RESULT, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, dip_tx_resp_cb, "device_se_activate_result");

    set_device_session(DEV_SESSION_IDLE);

}





void uploader_data_readying(ry_sts_t status, void* usr_data)
{
    if((status == RY_SUCC)&&((int)usr_data != 0)){
        LOG_INFO("[%s]-start upload\n",__FUNCTION__);
        ry_data_msg_send(DATA_SERVICE_UPLOAD_DATA_START, 0, NULL);
    }
}

ry_sts_t get_upload_data_handler(void)
{
    UploadDataStartParam param = {0};
    param.dataset_num = get_data_set_num();
    uint8_t * buf = (uint8_t *)ry_malloc(sizeof(UploadDataStartParam) + 10);
    u32_t status_code = RBP_RESP_CODE_SUCCESS;
    
    pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(UploadDataStartParam) + 10));
    
    if (!pb_encode(&stream_o, UploadDataStartParam_fields, &(param))) {
        //LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
        status_code = RBP_RESP_CODE_ENCODE_FAIL;
    }
    LOG_INFO(" [%s] bytes: %d set: %d\n\n", __FUNCTION__, stream_o.bytes_written, param.dataset_num);
    //ry_data_msg_send(DATA_SERVICE_UPLOAD_DATA_START, 0, NULL); 
    ry_sts_t ret = ble_send_response(CMD_DEV_UPLOAD_DATA_START, status_code, buf, stream_o.bytes_written, 1, uploader_data_readying, (void *)param.dataset_num);
    ry_free(buf);

    return ret;
}

void repo_step(u8_t * data, pb_size_t * len)
{
    SportPedoCurrentStepsPropVal  step = {0};
    extern sensor_alg_t s_alg;
    step.steps = alg_get_step_today();
    //u8_t buf[20]= {0};
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(SportPedoCurrentStepsPropVal)));
    if (!pb_encode(&stream_o, SportPedoCurrentStepsPropVal_fields, &(step))) {
        LOG_ERROR("[repo_step] Encoding failed.\r\n");
    }

    *len = stream_o.bytes_written;
    
}
void repo_last_heartRate(u8_t * data, pb_size_t * len)
{
    HealthLastHeartRatePropVal heartRate;
    heartRate.rate = s_alg.hr_cnt;
    heartRate.time = ((get_last_hrm_time() != 0) ? (int64_t)(get_last_hrm_time()): (int64_t)(-1));
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthLastHeartRatePropVal )));

    if (!pb_encode(&stream_o, HealthLastHeartRatePropVal_fields, &(heartRate))) {
        LOG_ERROR("[repo_last_heartRate] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void repo_heartRate_auto_result(u8_t * data , pb_size_t * len)
{
    /*HealthSettingHeartRateAutoResult result;
    result.enable = true;
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthSettingHeartRateAutoResult )));

    if (!pb_encode(&stream_o, HealthSettingHeartRateAutoResult_fields, &(result))) {
        LOG_ERROR("[repo_heartRate_auto_result] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;*/
    HealthSettingHeartRateAutoPropVal result;
    result.enable = (is_auto_heart_rate_open())?(1):(0);
	if(get_auto_heart_flag() != 1)
	{
		set_auto_heart_rate_time(600);
	}
	result.interval = get_auto_heart_rate_time();
	result.has_interval = 1;
	//LOG_ERROR("\r\n heartEN=%d interval=%d\r\n",result.enable,result.interval);
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthSettingHeartRateAutoPropVal )+10));

    if (!pb_encode(&stream_o, HealthSettingHeartRateAutoPropVal_fields, &(result))) {
        LOG_ERROR("[repo_heartRate_auto_result] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void repo_power_result(u8_t * data, pb_size_t * len)
{
    DeviceRemainingPowerPropVal result;
    result.power = sys_get_battery();
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceRemainingPowerPropVal)));

    if (!pb_encode(&stream_o, DeviceRemainingPowerPropVal_fields, &(result))) {
        LOG_ERROR("[repo_power_result] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void repo_blecfg_result(u8_t * data, pb_size_t * len)
{
    DeviceBleCfg result;
    result.peripheral_max_link = 0;
    result.is_mi_pc_enabled = 0;
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceBleCfg)));

    if (!pb_encode(&stream_o, DeviceBleCfg_fields, &(result))) {
        LOG_ERROR("[repo_blecfg_result] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void repo_language_result(u8_t * data, pb_size_t * len)
{
    DeviceLanguagePropVal result;
    result.type = 1; // ENGLISH
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceLanguagePropVal)));

    if (!pb_encode(&stream_o, DeviceLanguagePropVal_fields, &(result))) {
        LOG_ERROR("[repo_language_result] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void repo_sit_alert_result(u8_t * data, pb_size_t * len)
{
    HealthSettingSitAlertPropVal prop_val = {0};

	if(get_long_sit_flag() != 1)
	{
		set_long_sit_enable(1);
		set_sit_alert_start_time(8*60);
		set_sit_alert_end_time(22*60);
		prop_val.start_time_hour   = 8;
		prop_val.start_time_minute = 0;
		prop_val.end_time_hour     = 22;
		prop_val.end_time_minute   = 0;
	}
	else
	{
		u32_t mines_count = get_sit_alert_start_time();
		prop_val.start_time_hour = mines_count/60;
		prop_val.start_time_minute = mines_count%60;

		mines_count = get_sit_alert_end_time();
		prop_val.end_time_hour = mines_count/60;
		prop_val.end_time_minute = mines_count%60;
	}
	
	//LOG_ERROR("\r\n longSitEN=%d sTime=%d eTime=%d\r\n",get_long_sit_enable(),get_sit_alert_start_time(),get_sit_alert_end_time());
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthSettingSitAlertPropVal)+10));
    if (!pb_encode(&stream_o, HealthSettingSitAlertPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    
    *len = stream_o.bytes_written;
}

void repo_sleep_alert_result(u8_t * data, pb_size_t * len)
{
    HealthSettingSleepAlertPropVal prop_val = {0};
    u32_t mines_count = get_sleep_alert_start_time();
    u8_t i = 0;
    prop_val.sleep_time_hour = mines_count/60;
    prop_val.sleep_time_minute = mines_count%60;
    u32_t flag = get_sleep_alert_flag();
    for(i = 0; i <= HealthSettingSleepAlertPropVal_RepeatType_WORKDAY; i++){
        if(flag & (1 << i)){
            prop_val.repeats[prop_val.repeats_count] = (HealthSettingSleepAlertPropVal_RepeatType)i;
            prop_val.repeats_count++;
        }
    }
	
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthSettingSleepAlertPropVal)+10));
    if (!pb_encode(&stream_o, HealthSettingSleepAlertPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    
    *len = stream_o.bytes_written;
}


void get_raise_wake_prop(u8_t * data, pb_size_t * len) 
{
    DeviceSettingRaiseToWakePropVal result;
    result.enable = (is_raise_to_wake_open())?(1):(0);
	result.has_end_time_hour = 1;
	result.has_end_time_minute = 1;
	result.has_start_time_hour = 1;
	result.has_start_time_minute = 1;
	if((get_raise_wake_flag() != 1) || (get_raise_towake_start_time() == 0xffff) || (get_raise_towake_end_time() == 0xffff))
	{
		set_raise_towake_start_time(DEVICE_DEFAULT_WRIRST_TIME_START);
		set_raise_towake_end_time(DEVICE_DEFAULT_WRIRST_TIME_END);
		set_raise_to_wake_open();
		result.start_time_hour   = 0;
		result.start_time_minute = 0;
		result.end_time_hour     = 24;
		result.end_time_minute   = 0;
	}
	else
	{
		result.start_time_hour = get_raise_towake_start_time()/60;
		result.start_time_minute = get_raise_towake_start_time()%60;
		result.end_time_hour = get_raise_towake_end_time()/60;
		result.end_time_minute = get_raise_towake_end_time()%60;
	}
	//LOG_ERROR("\r\nraiseWakeEN=%d sTime=%d eTime=%d\r\n",is_raise_to_wake_open(),get_raise_towake_start_time(),get_raise_towake_end_time());

    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingRaiseToWakePropVal)+10));

    if (!pb_encode(&stream_o, DeviceSettingRaiseToWakePropVal_fields, &(result))) {
        LOG_ERROR("[Set_raise_wake_prop] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void get_msg_red_point_prop(u8_t * data, pb_size_t * len)
{
    NotificationSettingMsgRedPointPropVal result;
    result.enable = (is_red_point_open())?(1):(0);
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(NotificationSettingMsgRedPointPropVal)+10));

    if (!pb_encode(&stream_o, NotificationSettingMsgRedPointPropVal_fields, &(result))) {
        LOG_ERROR("[get_msg_red_point_prop] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void get_surface_battery_prop(u8_t * data, pb_size_t * len)
{
    DeviceSettingSurfaceBatteryPropVal result;
    result.enable = (is_surface_battery_open())?(1):(0);
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingSurfaceBatteryPropVal)+10));

    if (!pb_encode(&stream_o, DeviceSettingSurfaceBatteryPropVal_fields, &(result))) {
        LOG_ERROR("[get_surface_battery_prop] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void get_repeat_prop(u8_t * data, pb_size_t * len)
{
    NotificationSettingRepeatPropVal result;
    result.times = get_repeat_notify_times();
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(NotificationSettingRepeatPropVal)));

    if (!pb_encode(&stream_o, NotificationSettingRepeatPropVal_fields, &(result))) {
        LOG_ERROR("[set_repeat_prop] Encoding failed.\r\n");
    }
    *len = stream_o.bytes_written;
}

void repo_user_gender(u8_t * data, pb_size_t * len)
{
    PersonalGenderPropVal result;
    result.gender = get_user_gender();
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(PersonalGenderPropVal)));

    if (!pb_encode(&stream_o, PersonalGenderPropVal_fields, &(result))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_user_birth(u8_t * data, pb_size_t * len)
{
    PersonalBirthPropVal result;
    user_data_birth_t * birth = get_user_birth();
    result.year = birth->year;
    result.month = birth->month;
    result.day = birth->day;
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(PersonalBirthPropVal)));

    if (!pb_encode(&stream_o, PersonalBirthPropVal_fields, &(result))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_user_height(u8_t * data, pb_size_t * len)
{
    PersonalHeightPropVal result;

    result.height = get_user_height();
    
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(PersonalHeightPropVal)));

    if (!pb_encode(&stream_o, PersonalHeightPropVal_fields, &(result))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}


void repo_user_weight(u8_t * data, pb_size_t * len)
{
    PersonalWeightPropVal result;

    result.weight = get_user_weight();
    
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(PersonalWeightPropVal)));

    if (!pb_encode(&stream_o, PersonalWeightPropVal_fields, &(result))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}



void repo_unlock_type(u8_t * data, pb_size_t * len)
{
    UnlockTypePropVal prop_val = {0};
	if(get_unlock_flag() != 1)
	{
		set_lock_type(1);
	}
    prop_val.type = get_lock_type();

	//LOG_ERROR("\r\nunlockEN=%d\r\n", prop_val.type);
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(UnlockTypePropVal)+10));
    if (!pb_encode(&stream_o, UnlockTypePropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}


void repo_card_swiping(u8_t * data, pb_size_t * len)
{
    DeviceSettingCardSwipingPropVal prop_val = {0};
    prop_val.enable = (int32_t)get_card_swiping();

    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingCardSwipingPropVal)+10));
    if (!pb_encode(&stream_o, DeviceSettingCardSwipingPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}


void repo_target_step(u8_t * data, pb_size_t * len)
{
    HealthTargetStepPropVal prop_val = {0};
    prop_val.step = get_target_step();

    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthTargetStepPropVal)+10));
    if (!pb_encode(&stream_o, HealthTargetStepPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_resting_heart(u8_t * data, pb_size_t * len)
{
    int32_t status = 0;
    HealthRestingHeartRatePropVal prop_val = {0};
    status = alg_get_hrm_awake_cnt((uint8_t *)&(prop_val.rate));
    if(status == -1){
        prop_val.rate = -1;
    }
    if(prop_val.rate > 150){
        prop_val.rate = -1;
    }
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthRestingHeartRatePropVal)+10));
    if (!pb_encode(&stream_o, HealthRestingHeartRatePropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_current_calories(u8_t * data, pb_size_t * len)
{
    HealthCurrentCaloriesPropVal prop_val = {0};
    prop_val.cal = (int)app_get_calorie_today();
    
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthCurrentCaloriesPropVal)+10));
    if (!pb_encode(&stream_o, HealthCurrentCaloriesPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_current_distance(u8_t * data, pb_size_t * len)
{
    HealthCurrentDistancePropVal prop_val = {0};
    prop_val.dis = (int32_t)app_get_distance_today();
    
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(HealthCurrentDistancePropVal)+10));
    if (!pb_encode(&stream_o, HealthCurrentDistancePropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_goal_remind(u8_t * data, pb_size_t * len)
{
	DeviceSettingGoalRemindPropVal prop_val = {0};
	if(get_goal_remind_flag() == 0)
	{
		set_goal_remind_flag(1);
		set_goal_remind_enable(1);
	}
    prop_val.enable = get_goal_remind_enable();
    
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingGoalRemindPropVal)+10));
    if (!pb_encode(&stream_o, DeviceSettingGoalRemindPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_home_vibrate_type(u8_t * data, pb_size_t * len)
{
    DeviceSettingHomeVibratePropVal prop_val = {0};
	if(get_home_vibrate_flag() != 1)
	{
		set_home_vibrate_enable(1);
	}
    prop_val.enable = get_home_vibrate_enable();

	//LOG_ERROR("\r\nhomeEN=%d\r\n", prop_val.enable);
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingHomeVibratePropVal)+10));
    if (!pb_encode(&stream_o, DeviceSettingHomeVibratePropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_card_swiping_type(u8_t * data, pb_size_t * len)
{
	DeviceSettingCardSwipingPropVal prop_val = {0};
	
	if(get_card_swiping_flag() != 1)
	{
		set_card_default_enable(1);
	}
    prop_val.enable = get_card_default_enable();

	//LOG_DEBUG("[%s] CardEN=%d Flag=%d\r\n", __FUNCTION__, prop_val.enable, get_card_swiping_flag());
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingCardSwipingPropVal)+10));
    if (!pb_encode(&stream_o, DeviceSettingCardSwipingPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}

void repo_dnd_mode_status(u8_t * data, pb_size_t * len)
{
	DeviceSettingDoNotDisturbPropVal prop_val;
	
	memset(&prop_val,0,sizeof(DeviceSettingDoNotDisturbPropVal));
	
	u8_t ii = 0;
	dnd_time_t dnd_time[5] = {0};
	
	if((get_dnd_has_mode_type() != 1) || (get_dnd_mode() == 0xff))
	{
		prop_val.has_raise_to_wake =1;
		prop_val.has_home_vibrate = 1;
		prop_val.has_lunch_mode_enable = 1;
		set_dnd_home_vibrate_enbale(1);
		set_dnd_lunch_enable(1);
		set_dnd_wrist_enbale(1);
		set_dnd_time_count_max(2);
		set_dnd_mode(DeviceSettingDoNotDisturbPropVal_DndMode_DISABLE);
	}

	get_dnd_time(dnd_time);
	
	for(ii=0; ii<2; ii++){
		if(ii == DND_MODE_LUNCH){
			memcpy(prop_val.durations[ii].tag,"lunch",strlen("lunch"));
			if((get_dnd_has_mode_type() != 1) || (dnd_time[ii].startTime > 24*60) || (dnd_time[ii].endTime > 24*60)){
				set_dnd_has_mode_type(1);
				prop_val.durations[ii].start_time_hour = 12;
				prop_val.durations[ii].start_time_minute = 0;
				prop_val.durations[ii].end_time_hour = 14;
				prop_val.durations[ii].end_time_minute = 0;
				
				dnd_time[ii].startTime = 12*60;
				dnd_time[ii].endTime   = 14*60;
				dnd_time[ii].tagFlag   = DND_MODE_LUNCH;
				set_dnd_time(dnd_time);
			}else{
				prop_val.durations[ii].start_time_hour = dnd_time[ii].startTime/60;
				prop_val.durations[ii].start_time_minute = dnd_time[ii].startTime%60;
				prop_val.durations[ii].end_time_hour = dnd_time[ii].endTime/60;
				prop_val.durations[ii].end_time_minute = dnd_time[ii].endTime%60;
			}
		}else if(ii == DND_MODE_TIMING){
			memcpy(prop_val.durations[ii].tag,"default",strlen("default"));
			if((get_dnd_has_mode_type() != 1) || (dnd_time[ii].startTime > 24*60) || (dnd_time[ii].endTime > 24*60))
			{
				prop_val.durations[ii].start_time_hour = 22;
				prop_val.durations[ii].start_time_minute = 0;
				prop_val.durations[ii].end_time_hour = 8;
				prop_val.durations[ii].end_time_minute = 0;
				
				dnd_time[ii].startTime = 22*60;
				dnd_time[ii].endTime   = 8*60;
				dnd_time[ii].tagFlag   = DND_MODE_TIMING;
				set_dnd_time(dnd_time);
			}
			else{
				prop_val.durations[ii].start_time_hour = dnd_time[ii].startTime/60;
				prop_val.durations[ii].start_time_minute = dnd_time[ii].startTime%60;
				prop_val.durations[ii].end_time_hour = dnd_time[ii].endTime/60;
				prop_val.durations[ii].end_time_minute = dnd_time[ii].endTime%60;
			}
		}
	}
	
	prop_val.has_home_vibrate = 1;
	prop_val.has_lunch_mode_enable = 1;
	prop_val.has_raise_to_wake = 1;
	
	prop_val.home_vibrate = get_dnd_home_vibrate_enbale();
	prop_val.raise_to_wake =  get_dnd_wrist_enable();
	prop_val.lunch_mode_enable = get_dnd_lunch_enable();
	prop_val.durations_count = 2;
    prop_val.mode = (DeviceSettingDoNotDisturbPropVal_DndMode)get_dnd_mode();
	
    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingDoNotDisturbPropVal)+10));
    if (!pb_encode(&stream_o, DeviceSettingDoNotDisturbPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
	
	//LOG_INFO("[DNDPROP]Mode=%d Wrist=%d Home=%d Lunch=%d Cout=%d STime0=%d ETime0=%d Tag0=%s STime1=%d ETime1=%d Tag1=%s,HasHome=%d HasWrist=%d HasLunch=%d msgLen=%d\r\n", \
	prop_val.mode, prop_val.raise_to_wake,prop_val.home_vibrate,prop_val.lunch_mode_enable,\
	prop_val.durations_count,prop_val.durations[0].start_time_hour*60+prop_val.durations[0].start_time_minute,\
	prop_val.durations[0].end_time_hour*60+prop_val.durations[0].end_time_minute,\
	prop_val.durations[0].tag,\
	prop_val.durations[1].start_time_hour*60+prop_val.durations[1].start_time_minute,\
	prop_val.durations[1].end_time_hour*60+prop_val.durations[1].end_time_minute,\
	prop_val.durations[1].tag,\
	prop_val.has_home_vibrate,prop_val.has_raise_to_wake,prop_val.has_lunch_mode_enable,*len);
}

void repo_wear_habit_status(u8_t * data, pb_size_t * len)
{
	DeviceSettingWearHabitPropVal prop_val = {0};
	
    prop_val.wear_habit = get_wear_habit();

    pb_ostream_t stream_o = pb_ostream_from_buffer(data, (sizeof(DeviceSettingWearHabitPropVal)+10));
    if (!pb_encode(&stream_o, DeviceSettingWearHabitPropVal_fields, &(prop_val))) {
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
    *len = stream_o.bytes_written;
}


ry_sts_t get_property(uint8_t* data, int len)
{
    uint8_t * buf = ry_malloc(sizeof(PropGetResult) + 20);
    PropGetResult * PropResult = (PropGetResult *)ry_malloc(sizeof(PropGetResult));

    PropGetParam msg;
    int status;
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream_i, PropGetParam_fields, &msg);

    int i = 0;

    for(i = 0; i < msg.prop_id_count; i++){
        LOG_DEBUG("[get_property] ID:%d\r\n", msg.prop_id[i]);
        if(msg.prop_id[i] == PROP_ID_SPORT_PEDO_CURRENT_STEPS){
            repo_step(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_LAST_HEART_RATE){
            repo_last_heartRate(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_SETTING_HEART_RATE_AUTO){
            repo_heartRate_auto_result(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_SETTING_SIT_ALERT){
            repo_sit_alert_result(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_SETTING_SLEEP_ALERT){
            repo_sleep_alert_result(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_TARGET_STEP){
            repo_target_step(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_REMAINING_POWER){
            repo_power_result(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_BLE_CFG){
            repo_blecfg_result(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_LANGUAGE){
            repo_language_result(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_RAISE_TO_WAKE){
            get_raise_wake_prop(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_NOTIFICATION_SETTING_MSG_RED_POINT){
            get_msg_red_point_prop(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_SURFACE_BATTERY){
            get_surface_battery_prop(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_NOTIFICATION_SETTING_REPEAT){
            get_repeat_prop(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_PERSONAL_GENDER){
            repo_user_gender(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_PERSONAL_BIRTH){
            repo_user_birth(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_PERSONAL_HEIGHT){
            repo_user_height(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_PERSONAL_WEIGHT){
            repo_user_weight(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_RESTING_HEART_RATE){
            repo_resting_heart(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_CURRENT_CALORIES){
            repo_current_calories(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_CURRENT_DISTANCE){
            repo_current_distance(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_HEALTH_SETTING_GOAL_REMIND){
			repo_goal_remind(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
		}else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_UNLOCK) {
            repo_unlock_type(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_HOME_VIBRATE) {
            repo_home_vibrate_type(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_CARD_SWIPING) {
            repo_card_swiping_type(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_DO_NOT_DISTURB){
			repo_dnd_mode_status(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
        }else if(msg.prop_id[i] == PROP_ID_DEVICE_SETTING_WEAR_HABIT){
			repo_wear_habit_status(PropResult->prop_val[i].bytes, &(PropResult->prop_val[i].size) );
		}
    }

    PropResult->prop_val_count = msg.prop_id_count;
    pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(PropGetResult )));

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream_o, PropGetResult_fields, (PropResult))) {
        LOG_ERROR("[device_info_send] Encoding failed.\r\n");
    }
    //message_length = stream.bytes_written;
    
    //status = rbp_send_resp(CMD_PROP_GET, RBP_RESP_CODE_SUCCESS, buf, stream_o.bytes_written, 1);

    //status = ry_ble_txMsgReq(CMD_PROP_GET, RY_BLE_TYPE_RESPONSE, RBP_RESP_CODE_SUCCESS,
    //            buf, stream_o.bytes_written, 1, dip_tx_resp_cb, NULL);

    status = ble_send_response(CMD_PROP_GET, RBP_RESP_CODE_SUCCESS, 
                buf, stream_o.bytes_written, 1, dip_tx_resp_cb, "get_property");

    ry_free(buf);
    ry_free(PropResult);

    return status;
}

void set_raise_wake_prop(u8_t * data, u8_t  len)
{
	int status;
	DeviceSettingRaiseToWakePropVal wrist_towake;

    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);  

	status = pb_decode(&stream_i, DeviceSettingRaiseToWakePropVal_fields, &wrist_towake);
	set_raise_towake_start_time(wrist_towake.start_time_hour*60 + wrist_towake.start_time_minute);
	set_raise_towake_end_time(wrist_towake.end_time_hour*60 + wrist_towake.end_time_minute);
	
	if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }
	
	if(wrist_towake.enable){
        set_raise_to_wake_open();
    }else{
        set_raise_to_wake_close();
    }
}

void set_msg_red_point_prop(u8_t * data, u8_t  len)
{
    NotificationSettingMsgRedPointPropVal set_param;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, NotificationSettingMsgRedPointPropVal_fields, &set_param);
    
    if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }

    if(set_param.enable){
        set_red_point_open();
    }else{
        set_red_point_close();
    }
}

void set_surface_battery_prop(u8_t * data, u8_t  len)
{
    DeviceSettingSurfaceBatteryPropVal set_param;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, DeviceSettingSurfaceBatteryPropVal_fields, &set_param);
    
    if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }

    if(set_param.enable){
        set_surface_battery_open();
    }else{
        set_surface_battery_close();
    }
}

void set_repeat_prop(u8_t * data, u8_t  len)
{
    NotificationSettingRepeatPropVal set_param;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, NotificationSettingRepeatPropVal_fields, &set_param);

    if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }

    set_repeat_notify_times(set_param.times);

}

void set_heart_rate_auto(u8_t * data, u8_t  len)
{
    HealthSettingHeartRateAutoPropVal set_param;
    
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, HealthSettingHeartRateAutoPropVal_fields, &set_param);

    if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
    }

	set_auto_heart_rate_time(set_param.interval);
	
    if(set_param.enable){
        set_auto_heart_rate_open();
    }else{
        set_auto_heart_rate_close();
    }
}

void set_sit_alert_result(u8_t * data, u8_t  len)
{
    HealthSettingSitAlertPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, HealthSettingSitAlertPropVal_fields, &prop_val);
    if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
        return;
    }

    set_sit_alert_start_time(prop_val.start_time_hour * 60 + prop_val.start_time_minute);
    set_sit_alert_end_time(prop_val.end_time_hour * 60 + prop_val.end_time_minute);
    set_sit_alert_forbid_start_time(prop_val.forbid_start_time_hour * 60 + prop_val.forbid_start_time_minute);
    set_sit_alert_forbid_end_time(prop_val.forbid_end_time_hour * 60 + prop_val.forbid_end_time_minute); 

	if(get_sit_alert_start_time() == get_sit_alert_end_time())
	{
		alg_long_sit_set_enable(0);
	}
	else
	{
		alg_long_sit_set_enable(1);
	}
}

void set_sleep_alert_time(u8_t * data, u8_t  len)
{
    HealthSettingSleepAlertPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    u8_t i = 0;
    u32_t flag = 0;
    status = pb_decode(&stream_i, HealthSettingSleepAlertPropVal_fields, &prop_val);
    if(status == 0){
        LOG_ERROR("[%s] Encoding failed.\r\n", __FUNCTION__);
        return;
    }

    set_sleep_alert_start_time(prop_val.sleep_time_hour * 60 + prop_val.sleep_time_minute);


    for(i = 0; i < prop_val.repeats_count; i++){
        flag |= (1 << prop_val.repeats[i]);
    }
    set_sleep_alert_flag(flag);
    
}

void set_prop_target_step(u8_t * data, u8_t  len)
{
    HealthTargetStepPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, HealthTargetStepPropVal_fields, &prop_val);

    set_target_step(prop_val.step);
}

void set_prop_user_gender(u8_t * data, u8_t  len)
{
    PersonalGenderPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, PersonalGenderPropVal_fields, &prop_val);

    set_user_gender(prop_val.gender);
}

void set_prop_user_birth(u8_t * data, u8_t  len)
{
    PersonalBirthPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, PersonalBirthPropVal_fields, &prop_val);

    set_user_birth(prop_val.year, prop_val.month, prop_val.day);
}

void set_prop_user_height(u8_t * data, u8_t  len)
{
    PersonalHeightPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, PersonalHeightPropVal_fields, &prop_val);

    set_user_height(prop_val.height);
}

void set_prop_user_weight(u8_t * data, u8_t  len)
{
    PersonalWeightPropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, PersonalWeightPropVal_fields, &prop_val);

    set_user_weight(prop_val.weight);
}

void set_prop_unlock_type(u8_t * data, u8_t  len)
{
    UnlockTypePropVal prop_val = {0};
    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, UnlockTypePropVal_fields, &prop_val);

    set_lock_type(prop_val.type);
}

void set_prop_home_vibrate(u8_t * data, u8_t  len)
{
	DeviceSettingHomeVibratePropVal  prop_val = {0};
	pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, DeviceSettingHomeVibratePropVal_fields, &prop_val);

    set_home_vibrate_enable(prop_val.enable);
}

void set_prop_card_swiping(u8_t * data, u8_t  len)
{
	DeviceSettingCardSwipingPropVal prop_val = {0};
	pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, DeviceSettingCardSwipingPropVal_fields, &prop_val);

    set_card_default_enable(prop_val.enable);
}

void set_prop_dnd(u8_t * data, u8_t  len)
{
	u8_t index = 0;
	u8_t timeCount = 0;
	dnd_time_t setDndTime[5];
	DeviceSettingDoNotDisturbPropVal  prop_val;
	
	memset(&prop_val,0,sizeof(DeviceSettingDoNotDisturbPropVal));
	
	pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, DeviceSettingDoNotDisturbPropVal_fields, &prop_val);
	set_dnd_time_count_max(prop_val.durations_count);
	
	if(DeviceSettingDoNotDisturbPropVal_DndMode_DISABLE != prop_val.mode){
		for(timeCount=0; timeCount<prop_val.durations_count; timeCount++){
			
			if((0 == strncmp("lunch",prop_val.durations[timeCount].tag,strlen("lunch"))) && (prop_val.lunch_mode_enable == 1)){
				setDndTime[timeCount].tagFlag = DND_MODE_LUNCH;
				setDndTime[timeCount].startTime = prop_val.durations[timeCount].start_time_hour*60 + prop_val.durations[timeCount].start_time_minute;
				setDndTime[timeCount].endTime = prop_val.durations[timeCount].end_time_hour*60 + prop_val.durations[timeCount].end_time_minute;
				set_dnd_time(setDndTime);
			}else if(0 == strncmp("default",prop_val.durations[timeCount].tag,strlen("default"))){
				setDndTime[timeCount].tagFlag = DND_MODE_TIMING;
				setDndTime[timeCount].startTime = prop_val.durations[timeCount].start_time_hour*60 + prop_val.durations[timeCount].start_time_minute;
				setDndTime[timeCount].endTime = prop_val.durations[timeCount].end_time_hour*60 + prop_val.durations[timeCount].end_time_minute;
				set_dnd_time(setDndTime);
			}
		}
	}
	else{
		prop_val.home_vibrate = 1;
		prop_val.raise_to_wake = 1;
		prop_val.lunch_mode_enable = 1;
		set_dnd_has_mode_type(0);
	}
	
	//Don't change the order of the functions
	set_dnd_wrist_enbale(prop_val.raise_to_wake);
	set_dnd_home_vibrate_enbale(prop_val.home_vibrate);
	set_dnd_lunch_enable(prop_val.lunch_mode_enable);
	set_dnd_mode(prop_val.mode);
	//LOG_INFO("[DNDSET]Mode=%d Wrist=%d Home=%d Lunch=%d Cout=%d STime0=%d ETime0=%d Tag0=%s,STime1=%d ETime1=%d Tag1=%s,HasHome=%d HasWrist=%d HasLunch=%d msgLen=%d\r\n", \
	prop_val.mode, prop_val.raise_to_wake,prop_val.home_vibrate,prop_val.lunch_mode_enable,\
	prop_val.durations_count,prop_val.durations[0].start_time_hour*60+prop_val.durations[0].start_time_minute,\
	prop_val.durations[0].end_time_hour*60+prop_val.durations[0].end_time_minute,\
	prop_val.durations[0].tag,\
	prop_val.durations[1].start_time_hour*60+prop_val.durations[1].start_time_minute,\
	prop_val.durations[1].end_time_hour*60+prop_val.durations[1].end_time_minute,\
	prop_val.durations[1].tag,\
	prop_val.has_home_vibrate,prop_val.has_raise_to_wake,prop_val.has_lunch_mode_enable,len);
}

void set_prop_goal(u8_t * data, u8_t  len)
{
	DeviceSettingGoalRemindPropVal  prop_val = {0};
	pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, DeviceSettingGoalRemindPropVal_fields, &prop_val);

    set_goal_remind_enable(prop_val.enable);
}

void set_prop_wear_habit(u8_t * data, u8_t  len)
{
	DeviceSettingWearHabitPropVal  prop_val = {0};
	pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);
    int status;
    status = pb_decode(&stream_i, DeviceSettingWearHabitPropVal_fields, &prop_val);

    set_wear_habit(prop_val.wear_habit);
}

ry_sts_t set_property(uint8_t* data, int len)
{
    int status;
    PropSetParam * param = (PropSetParam *)ry_malloc(sizeof(PropSetParam));

    pb_istream_t stream_i = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream_i, PropSetParam_fields, param);

    switch(param->prop_id){
        case PROP_ID_SPORT_PEDO_CURRENT_STEPS:
            break;
        case PROP_ID_HEALTH_LAST_HEART_RATE:
            break;
        case PROP_ID_HEALTH_SETTING_HEART_RATE_AUTO:
            set_heart_rate_auto(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_REMAINING_POWER:
            break;
        case PROP_ID_DEVICE_LANGUAGE:
            break;        
        case PROP_ID_DEVICE_BLE_CFG:
            break;        
        case PROP_ID_DEVICE_SETTING_RAISE_TO_WAKE:
            set_raise_wake_prop(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_NOTIFICATION_SETTING_MSG_RED_POINT:
            set_msg_red_point_prop(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_SETTING_SURFACE_BATTERY:
            set_surface_battery_prop(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_NOTIFICATION_SETTING_REPEAT:
            set_repeat_prop(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_HEALTH_SETTING_SIT_ALERT:
            set_sit_alert_result(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_HEALTH_SETTING_SLEEP_ALERT:
            set_sleep_alert_time(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_HEALTH_TARGET_STEP:
            set_prop_target_step(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_PERSONAL_GENDER :
            set_prop_user_gender(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_PERSONAL_BIRTH :
            set_prop_user_birth(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_PERSONAL_HEIGHT :
            set_prop_user_height(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_PERSONAL_WEIGHT :
            set_prop_user_weight(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_SETTING_UNLOCK:
            set_prop_unlock_type(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_SETTING_HOME_VIBRATE:
            set_prop_home_vibrate(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_SETTING_CARD_SWIPING:
            set_prop_card_swiping(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_SETTING_DO_NOT_DISTURB:
            set_prop_dnd(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_HEALTH_SETTING_GOAL_REMIND:
            set_prop_goal(param->prop_val.bytes, (param->prop_val.size) );
            break;
        case PROP_ID_DEVICE_SETTING_WEAR_HABIT:
            set_prop_wear_habit(param->prop_val.bytes, (param->prop_val.size) );
            break;
    }

    set_device_setting();
    
    ry_free(param);
    //rbp_send_resp(CMD_PROP_SET, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    return ble_send_response(CMD_PROP_SET, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 1, dip_tx_resp_cb, (void*)__FUNCTION__);
}




u8_t* device_info_get_mac(void)
{
    return device_info.mac;
}

u8_t* device_info_get_aes_key(void)
{
    return device_info.aes_key;
}


ry_sts_t set_conn_interval_param(void)
{
    BleConnIntervalParam param = {0};
    uint8_t buf[30] = {0};
    ry_sts_t status;
    param.ble_conn_interval = ry_ble_get_connInterval();

    pb_ostream_t stream_o = pb_ostream_from_buffer(buf, (sizeof(BleConnIntervalParam )));

    /* Now we are ready to encode the message! */
    if (!pb_encode(&stream_o, BleConnIntervalParam_fields, &param)) {
        LOG_ERROR("[%s] Encoding failed.\r\n", (void*)__FUNCTION__);
    }
    //message_length = stream.bytes_written;
    
    //status = rbp_send_resp(CMD_APP_BLE_CONN_INTERVAL, RBP_RESP_CODE_SUCCESS, buf, stream_o.bytes_written, 1);
    status = ble_send_response(CMD_APP_BLE_CONN_INTERVAL, RBP_RESP_CODE_SUCCESS, 
                        buf, stream_o.bytes_written, 1, dip_tx_resp_cb, (void*)__FUNCTION__);

    return status;
}


void set_phone_app_info(u8_t * data , u32_t len)
{
    ble_send_response(CMD_APP_BLE_CONN_INTERVAL, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 1, NULL, NULL);
    
}




#if 1
void print_cert(void)
{
    ry_sts_t status;
    u8_t key[16];
    u8_t this_iv[16];
    u8_t * safe_hex = (u8_t *)(RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3 + sizeof(u32_t) );
    int offset = 0;
    u8_t temp[16];
    u8_t output[17] = {0};
    
    /* Load AES-KEY */
    ry_memcpy(device_info.aes_key,    &safe_hex[2],  4);
    ry_memcpy(device_info.aes_key+4,  &safe_hex[14], 4);
    ry_memcpy(device_info.aes_key+8,  &safe_hex[38], 4);
    ry_memcpy(device_info.aes_key+12, &safe_hex[52], 4);

    LOG_DEBUG("key: ");
    ry_data_dump(device_info.aes_key, 16, ' ');

    /* Load IV */
    ry_memcpy(device_info.iv,    safe_hex, 2);
    ry_memcpy(device_info.iv+2,  &safe_hex[6], 8);
    ry_memcpy(device_info.iv+10, &safe_hex[18], 6);

    LOG_DEBUG("iv: ");
    ry_data_dump(device_info.iv, 16, ' ');


    LOG_DEBUG("---------------DECRYPT Cert-----------\r\n");

    int len;
    ry_memcpy(&len, (u8_t *)(RY_DEVICE_INFO_ADDR + DEV_INFO_UNI_OFFSET * 3), 4);
    len -= 32;
    
    int cnt;
    int last_len;
    int ret;
    u8_t* input;
    mbedtls_aes_context *ctx = (mbedtls_aes_context*)ry_malloc(sizeof(mbedtls_aes_context));
    if (ctx == NULL) {
        LOG_ERROR("[ry_aes_cbc_decrypt] No mem. %d \r\n", ret);
        return;
    }

    mbedtls_aes_init( ctx );
    ret = mbedtls_aes_setkey_dec( ctx, device_info.aes_key, 128 );
    if (ret < 0) {
        LOG_ERROR("[ry_aes_cbc_decrypt] AES Set key Error. %d \r\n", ret);
        ry_free(ctx);
        return;
    }

   

    ry_memcpy(this_iv, device_info.iv, 16);

    /* First block */
    input = safe_hex + 24;
    ry_memcpy(temp, safe_hex+24, 14);
    offset += 14;
    ry_memcpy(temp+14, safe_hex+42, 2);
    ret = mbedtls_aes_crypt_cbc( ctx, MBEDTLS_AES_DECRYPT, 16, this_iv, temp, output );
    if (ret < 0) {
        LOG_ERROR("AES Crypt Error. %d \r\n", ret);
        ry_free(ctx);
        return;
    }
    ry_board_printf("%s", output);
        
    /* Second block */
    ry_memcpy(temp, safe_hex+44, 8);
    ry_memcpy(temp+8, safe_hex+56, 8);
    ret = mbedtls_aes_crypt_cbc( ctx, MBEDTLS_AES_DECRYPT, 16, this_iv, temp, output );
    if (ret < 0) {
        LOG_ERROR("AES Crypt Error. %d \r\n", ret);
        ry_free(ctx);
        return;
    }
    ry_board_printf("%s", output);
    
    /* Next */
    len -= 40;
    last_len = len%16;
    cnt = last_len ? (len>>4)+1 : (len>>4);
    input = safe_hex+64;
    offset = 0;
    
    for (int i = 0; i < cnt; i++) {
        
        ret = mbedtls_aes_crypt_cbc( ctx, MBEDTLS_AES_DECRYPT, 16, this_iv, input + i*16, output );
        if (ret < 0) {
            LOG_ERROR("AES Crypt Error. %d \r\n", ret);
            ry_free(ctx);
            return;
        }

        //ry_data_dump(output + i*16, 16, ' ');
        ry_board_printf("%s", output);
    }
		
    ry_board_printf("\n\n\n");

    ry_free(ctx);
    return;



}
#endif



ry_sts_t phone_app_status_handler(u8_t * data, u32_t size)
{
    AppStatus result_data = {0};
    ry_sts_t status = RY_SUCC;
    u32_t code = RBP_RESP_CODE_SUCCESS;

    pb_istream_t stream = pb_istream_from_buffer(data, size);
    
    pb_decode(&stream, AppStatus_fields, &result_data);

    if(result_data.has_lifecycle) {
        if(result_data.lifecycle == PhoneAPPLifeCycleType_destroyed) {
            LOG_INFO("[%s] app Destroyed\n\n", __FUNCTION__);
            ry_ble_set_peer_phone_app_lifecycle(PEER_PHONE_APP_LIFECYCLE_DESTROYED);
            ry_sched_msg_send(IPC_MSG_TYPE_BLE_APP_KILLED, 0, NULL);
            return status;
        }
    }

// normal_life_cycle ==> ??????????????????
    LOG_DEBUG("[%s] set phone app status --- %d\n", __FUNCTION__, result_data.foreground);
    if (result_data.foreground) {
        ry_ble_set_peer_phone_app_lifecycle(PEER_PHONE_APP_LIFECYCLE_FOREGROUND);
        ry_alg_msg_send(IPC_MSG_TYPE_ALGORITHM_DATA_SYNC_WAKEUP, 0, NULL);
    } else {
        ry_ble_set_peer_phone_app_lifecycle(PEER_PHONE_APP_LIFECYCLE_BACKGROUND);
    }
    ble_send_response(CMD_DEV_APP_SEND_APP_STATUS, RBP_RESP_CODE_SUCCESS, 
                        NULL, 0, 0, dip_tx_resp_cb, "CMD_DEV_APP_SEND_APP_STATUS");

    return RY_SUCC;

}




