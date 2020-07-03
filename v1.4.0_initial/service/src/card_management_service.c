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
#include "card_management_service.h"
#include "device_management_service.h"
#include "timer_management_service.h"
#include "ry_list.h"
#include "scheduler.h"
#include "string.h"
#include "ryos_timer.h"

#include "ry_nfc.h"
#include "rbp.h"
#include "Gate.pb.h"
#include "nanopb_common.h"
#include "rbp.pb.h"
#include "transit.pb.h"
#include "defaultCard.pb.h"
#include "nfc_pn80t.h"
#include "ry_hal_flash.h"
#include <stdlib.h>


/*********************************************************************
 * CONSTANTS
 */ 

#define CMS_STATE_IDLE                         0
#define CMS_STATE_CHECK_CARD                   1
#define CMS_STATE_OTA_ACCESS_CARD              2
#define CMS_STATE_OTA_TRANSIT_CARD             3
#define CMS_STATE_READ_CARD                    4
#define CMS_STATE_DELETE_CARD                  5
#define CMS_STATE_SELECT_CARD                  6
#define CMS_STATE_POWER_ON                     7
#define CMS_STATE_POWER_OFF                    8
#define CMS_STATE_SE_OPEN                      9
#define CMS_STATE_RECHARGE                     10


#define CMS_SESSION_STATE_WAIT_CREATE                 0
#define CMS_SESSION_STATE_WAIT_CREATE_ACCESS_ACK      1
#define CMS_SESSION_STATE_WAIT_CREATE_TRANSIT_ACK     2
#define CMS_SESSION_STATE_WAIT_DELETE_ACK             3
#define CMS_SESSION_STATE_WAIT_CARD_SELECTED          4
#define CMS_SESSION_STATE_WAIT_READER_ON              5
#define CMS_SESSION_STATE_WAIT_READER_DETECT          6
#define CMS_SESSION_STATE_WAIT_READER_OFF             7
#define CMS_SESSION_STATE_WAIT_POWER_ON               8
#define CMS_SESSION_STATE_WAIT_CARD_OTA               9
#define CMS_SESSION_STATE_WAIT_POWER_OFF              10
#define CMS_SESSION_STATE_WAIT_RECHARGE_ACK           11


#define CMS_SESSION_TRANSIT_CARD_TIMEOUT       600*1000
#define CMS_SESSION_ACCESS_CARD_CHECK_TIMEOUT  30000 // 30s
#define CMS_SESSION_ACCESS_CARD_OTA_TIMEOUT    60000
#define CMS_SESSION_SELECT_CARD_TIMEOUT        8000
#define CMS_SESSION_ACCESS_CARD_DELETE_TIMEOUT 30000
#define CMS_SESSION_TRANSIT_RECHARGE_TIMEOUT   300*1000 // 5miin

#define CMS_SELECT_CARD_TIMEOUT                15000


#define CMS_EMTPY                              0xFF
#define CARD__CREATE_BATT_PERCENT_THRESH       10



/*********************************************************************
 * TYPEDEFS
 */










 
/*********************************************************************
 * LOCAL VARIABLES
 */

/*
 * @brief Card Service Context Control block
 */
cms_ctx_t cms_ctx_v;

ry_queue_t *cms_msgQ;
ryos_thread_t    *cms_thread_handle;
ryos_mutex_t     *se_mutex;

const char string_default_gate[] = {0x4B, 0x65, 0x79, 0x43, 0x61, 0x72, 0x64, 0x00}; // KeyCard


const u8_t access_card_aids[MAX_ACCESS_CARD_NUM][11] = {
    {0x52, 0x59, 0x45, 0x45, 0x58, 0x2E, 0x41, 0x43, 0x2E, 0x31, 0x31},
    {0x52, 0x59, 0x45, 0x45, 0x58, 0x2E, 0x41, 0x43, 0x2E, 0x32, 0x32},
    {0x52, 0x59, 0x45, 0x45, 0x58, 0x2E, 0x41, 0x43, 0x2E, 0x33, 0x33},
    {0x52, 0x59, 0x45, 0x45, 0x58, 0x2E, 0x41, 0x43, 0x2E, 0x34, 0x34},
    {0x52, 0x59, 0x45, 0x45, 0x58, 0x2E, 0x41, 0x43, 0x2E, 0x35, 0x35},
};


const support_card_t support_transit_card_list[SUPPORTED_CARD_NUM] = 
{
    {CARD_ID_SZT, {0x53, 0x5A, 0x54, 0x2E, 0x57, 0x41, 0x4C, 0x4C, 0x45, 0x54, 0x2E, 0x45, 0x4E, 0x56}, 14, "深圳通 "},
    {CARD_ID_LNT, {0x59, 0x43, 0x54, 0x2E, 0x55, 0x53, 0x45, 0x52}, 8, "岭南通 "},
    {CARD_ID_BJT, {0x91, 0x56, 0x00, 0x00, 0x14, 0x01, 0x00, 0x01}, 8, "北京通 "},
    {CARD_ID_JJJ, {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x10, 0x00, 0x91, 0x56, 0x00, 0x00, 0x14, 0xA1}, 16, "京津冀 "},
    {CARD_ID_WHT, {0xA0, 0x00, 0x00, 0x53, 0x42, 0x57, 0x48, 0x54, 0x4B}, 9, "武汉通 "},
    {CARD_ID_CQT, {0x43, 0x51, 0x51, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x33, 0x31}, 12, "重庆畅通卡 "},
    {CARD_ID_JST, {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x21, 0x50, 0x53, 0x55, 0x5A, 0x48, 0x4F, 0x55}, 16, "江苏一卡通 "},
    {CARD_ID_CAT, {0xA0, 0x00, 0x00, 0x00, 0x03, 0x71, 0x00, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, "长安通 "},
    {CARD_ID_HFT, {0xA0, 0x00, 0x00, 0x00, 0x03, 0x23, 0x00, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, "合肥通 "},
    {CARD_ID_GXT, {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x53, 0x00, 0x47, 0x55, 0x41, 0x4E, 0x47, 0x58}, 16, "广西通 "},
    {CARD_ID_JLT, {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x13, 0x20, 0x00, 0x4A, 0x49, 0x4C, 0x49, 0x4E}, 16, "吉林通 "},
    {CARD_ID_HEB, {0xA0, 0x00, 0x00, 0x00, 0x03, 0x15, 0x00, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, "哈尔滨 "},
    {CARD_ID_LCT, {0xA0, 0x00, 0x00, 0x53, 0x42, 0x5A, 0x5A, 0x48, 0x54}, 9, "绿城通 "},   //app 处
    {CARD_ID_QDT, {0xA0, 0x00, 0x00, 0x00, 0x03, 0x26, 0x60, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, 16, "琴岛通 "},
};

rf_para_t card_nfc_paras[MAX_CARD_NUM] = 
{
    {CARD_ID_SZT,  {0xF6, 0xF6}, 0x28, {0xE1, 0x00}},
    {CARD_ID_LNT,  {0xF6, 0xF6}, 0x28, {0xE1, 0x00}},
    {CARD_ID_BJT,  {0xF6, 0xF6}, 0x28, {0xE1, 0x00}},
    {CARD_ID_JJJ,  {0xF6, 0xF1}, 0x28, {0xE1, 0x00}},
    {CARD_ID_GATE, {0xF6, 0xF6}, 0x28, {0x2D, 0x00}},
};


const nfc_setting_t default_nfc_para = {0x01, 0xF6, 0xF6, 0x28, 0xE1, 0x00};

u8_t pre_aid[16];
u8_t pre_aid_len;



/*********************************************************************
 * LOCAL FUNCTIONS
 */
ry_sts_t card_session_delete(void);
void card_session_timer_create(void);
void card_session_timer_start(int timeout);
u8_t card_doSelectCard(u32_t type, u8_t* aid, u8_t aidLen, void* usrdata);
void card_selectGate(u32_t card_id, card_access_t* pa, card_uiCb_t cb);
void card_selectCard(u32_t card_id, card_transit_t *pt, card_uiCb_t cb);
u8_t card_queryCard(u8_t* aid, u8_t aidLen);
void cms_select_default_card(void);



void card_nfc_recover(void)
{
    ry_nfc_se_channel_close();

    /* Close NFC */
    if (cms_ctx_v.accessCardTbl.curNum + cms_ctx_v.transitCardTbl.curNum == 0) {
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
    } else {
        if (get_hardwardVersion()>3) {
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_DOWN, NULL);
        }
    }
}

void cms_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s] %s, error status: %x, heap:%d, minHeap:%d\r\n", \
            __FUNCTION__, (char*)arg, status, ryos_get_free_heap(), ryos_get_min_available_heap_size());

        card_nfc_recover();

        set_device_session(DEV_SESSION_IDLE);
    } else {
        LOG_DEBUG("[%s] %s, error status: %x, heap:%d, minHeap:%d\r\n", \
            __FUNCTION__, (char*)arg, status, ryos_get_free_heap(), ryos_get_min_available_heap_size());
    }
}


bool is_aid_empty(u8_t *aid)
{
    u8_t empty[MAX_CARD_AID_LEN];
    ry_memset(empty, CMS_EMTPY, MAX_CARD_AID_LEN);

    if (!ry_memcmp(empty, aid, MAX_CARD_AID_LEN)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


void card_sendUID(u8_t* uid)
{
    GateCheckResult * result = (GateCheckResult *)ry_malloc(sizeof(GateCheckResult));
    if (!result) {
        LOG_ERROR("[card_sendUID] No mem.\r\n");
        while(1);
    }
    
    result->code = 0;
    result->has_uid = 1;
    result->uid.size = 4;
    ry_memcpy(result->uid.bytes, uid, MAX_CARD_UID_LEN);

    //result->uid.bytes[0] = 0x09;
    //result->uid.bytes[1] = 0x0F;
    //result->uid.bytes[2] = 0x49;
    //result->uid.bytes[3] = 0x3E;

    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(GateCheckResult) + 10);
    if (!data_buf) {
        LOG_ERROR("[card_sendUID] No mem.\r\n");
        while(1);
    }
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(GateCheckResult) + 10);

    status = pb_encode(&stream, GateCheckResult_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[card_sendUID]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    status = rbp_send_req(CMD_APP_GATE_CHECK_RESULT, data_buf, message_length, 1);

exit:
    ry_free(data_buf);
    ry_free((u8_t*)result);
}


void cms_msg_broadcast(u8_t cardType, u8_t state, u8_t code)
{
    card_create_evt_t evt;
    evt.cardType = cardType;
    evt.state    = state;
    evt.code     = code;
    ry_sched_msg_send(IPC_MSG_TYPE_CMS_CARD_CREATE_EVT, sizeof(card_create_evt_t), (u8_t*)&evt);
}


int card_onNfcEvent(ry_ipc_msg_t* msg)
{
    u8_t status;
    ry_sts_t ry_sts;
    u8_t* aid;
    u8_t aidLen;
    
    LOG_DEBUG("[%s]\r\n", __FUNCTION__);

    switch (msg->msgType) {
        case IPC_MSG_TYPE_NFC_INIT_DONE:
            if (cms_ctx_v.state == CMS_STATE_CHECK_CARD) {
                
            } else if (cms_ctx_v.state == CMS_STATE_DELETE_CARD) {
                //rbp_send_resp(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
                ry_sts = ble_send_response(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
                if (ry_sts != RY_SUCC) {

                    LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                }
                cms_ctx_v.state = CMS_STATE_IDLE;
            } else if (cms_ctx_v.state == CMS_STATE_OTA_TRANSIT_CARD) {
                //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
                ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
                if (ry_sts != RY_SUCC) {

                    LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                }
                cms_ctx_v.state = CMS_STATE_IDLE;
            } else if (cms_ctx_v.state == CMS_STATE_RECHARGE) {
                //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
                ry_sts = ble_send_response(CMD_DEV_TRANSIT_RECHARGE_START, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
                if (ry_sts != RY_SUCC) {

                    LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                }
                cms_ctx_v.state = CMS_STATE_IDLE;
            } else if (cms_ctx_v.state == CMS_STATE_SE_OPEN) {

                ry_nfc_se_channel_open();
                //rbp_send_resp(CMD_DEV_SE_OPEN, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
                ry_sts = ble_send_response(CMD_DEV_SE_OPEN, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
                if (ry_sts != RY_SUCC) {

                    LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                }
                cms_ctx_v.state = CMS_STATE_IDLE;
            } else if (cms_ctx_v.state == CMS_STATE_POWER_ON) {
                if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_POWER_ON) {
                    LOG_DEBUG("[cms] Power on succ.\r\n");

                    if (cms_ctx_v.session->ui_callback) {
                        cms_ctx_v.session->ui_callback(cms_ctx_v.session->card_id, 0, NULL, NULL, 0, NULL);
                    }
                    card_session_delete();
                    ry_sched_addNfcEvtListener(NULL);
                }
                cms_ctx_v.state = CMS_STATE_IDLE;
            }
            break;
        

        case IPC_MSG_TYPE_NFC_READER_ON_EVT:
            if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_READER_ON) {
                cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_READER_DETECT;
                LOG_DEBUG("[cms] Reader on succ.\r\n");

                /* Notify UI */
                cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_DETECT, CARD_CREATE_EVT_CODE_START);
                
            }
            break;

        case IPC_MSG_TYPE_NFC_READER_OFF_EVT:
            LOG_DEBUG("[cms] Reader off succ.\r\n");
            
            break;

        case IPC_MSG_TYPE_NFC_READER_DETECT_EVT:
            if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_READER_DETECT) {
                LOG_DEBUG("[cms] Reader detected. ID=%02x %02x %02x %02x\r\n", 
                    msg->data[0], msg->data[1], msg->data[2], msg->data[3]);
                
                /* Close reader first */
                //ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_OFF, NULL);
                ry_nfc_sync_poweroff();

                ry_memcpy(cms_ctx_v.session->uid, msg->data, MAX_CARD_UID_LEN);
                card_sendUID(cms_ctx_v.session->uid);

                /* Release the session */
                card_session_delete();

                //cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_CARD_OTA;
                //card_session_timer_start(CMS_SESSION_ACCESS_CARD_OTA_TIMEOUT);
                cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_DETECT, CARD_CREATE_EVT_CODE_SUCC);

                set_device_session(DEV_SESSION_IDLE);
            }
            break;

        case IPC_MSG_TYPE_NFC_CE_ON_EVT:
            LOG_DEBUG("[cms] CE on succ.\r\n");
            

            /* Select card and tell UI. */
            if (cms_ctx_v.state == CMS_STATE_SELECT_CARD) {
				u8_t ret;
                card_transit_t* pt = (card_transit_t*)cms_ctx_v.session->pCard;
                card_access_t* pCa = (card_access_t*)cms_ctx_v.session->pCard;
                int balance;

                if (cms_ctx_v.session) {
                    
                    if (cms_ctx_v.session->card_id == CARD_ID_GATE) {
                        status = card_doSelectCard(CARD_ID_GATE, cms_ctx_v.session->aid, cms_ctx_v.session->aidLen, NULL);
                        if (status == CARD_SELECT_STATUS_OK) {
                            pCa->selected = 1;
                        }
                    } else {
                        status = card_doSelectCard(cms_ctx_v.session->card_id, cms_ctx_v.session->aid, cms_ctx_v.session->aidLen, (void*)&balance);
                        if (status == CARD_SELECT_STATUS_OK) {
                            pt->selected = 1;
                        }
                    } 

                    if (cms_ctx_v.session->ui_callback) {
                        u32_t para;
                        if (cms_ctx_v.session->card_id == CARD_ID_GATE) {
                            para = pCa->color;
                        } else {
                            para = cms_ctx_v.session->card_id;
                        }
                        cms_ctx_v.session->ui_callback(para, status, cms_ctx_v.session->name, 
                            cms_ctx_v.session->aid, 
                            cms_ctx_v.session->aidLen,
                            (void*)balance);
                    }

                    card_session_delete();
                }
                
                ry_sched_addNfcEvtListener(NULL);
            }
            
            break;

        case IPC_MSG_TYPE_NFC_CE_OFF_EVT:
            LOG_DEBUG("[cms] CE off succ.\r\n");
            break;

        case IPC_MSG_TYPE_NFC_POWER_OFF_EVT:
            LOG_DEBUG("[cms] Power off succ.\r\n");

            if (cms_ctx_v.state == CMS_STATE_POWER_OFF) {
                if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_POWER_OFF) {
                    LOG_DEBUG("[cms] Power off succ.\r\n");

                    if (cms_ctx_v.session->ui_callback) {
                        cms_ctx_v.session->ui_callback(cms_ctx_v.session->card_id, 0, NULL, NULL, 0, 0);
                    }
                    card_session_delete();
                    ry_sched_addNfcEvtListener(NULL);
                }
                cms_ctx_v.state = CMS_STATE_IDLE;
            }
            break;

        default:
            break;

    }

    return 1;
}

u8_t card_getTypeByAID(u8_t* aid, int aidLen)
{
    if (0 == ry_memcmp(aid, (void*)AID_BJT, aidLen)) {
        return CARD_ID_BJT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_SZT, aidLen)) {
        return CARD_ID_SZT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_LNT, aidLen)) {
        return CARD_ID_LNT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_JJJ, aidLen)) {
        return CARD_ID_JJJ;
    }

    if (0 == ry_memcmp(aid, (void*)AID_WHT, aidLen)) {
        return CARD_ID_WHT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_CQT, aidLen)) {
        return CARD_ID_CQT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_JST, aidLen)) {
        return CARD_ID_JST;
    }
    
    if (0 == ry_memcmp(aid, (void*)AID_CAT, aidLen)) {
        return CARD_ID_CAT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_HFT, aidLen)) {
        return CARD_ID_HFT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_GXT, aidLen)) {
        return CARD_ID_GXT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_JLT, aidLen)) {
        return CARD_ID_JLT;
    }

    if (0 == ry_memcmp(aid, (void*)AID_HEB, aidLen)) {
        return CARD_ID_HEB;
    }

    if (0 == ry_memcmp(aid, (void*)AID_LCT, aidLen)) {
        return CARD_ID_LCT;
    }
    
    if (0 == ry_memcmp(aid, (void*)AID_QDT, aidLen)) {
        return CARD_ID_QDT;
    }
	
    return CARD_ID_INVALID;
}



card_transit_t* card_transitCardSearch(u8_t* aid, int aidLen)
{
    u8_t key[MAX_CARD_AID_LEN] = {0};
    ry_memcpy(key, aid, aidLen);
    return ry_tbl_search(MAX_TRANSIT_CARD_NUM, 
               sizeof(card_transit_t),
               (u8_t*)&cms_ctx_v.transitCardTbl.cards,
               0,
               MAX_CARD_AID_LEN,
               key);
}


ry_sts_t card_transitCardAdd(char* name, u8_t* aid, int aidLen, u8_t type)
{
    card_transit_t newCard;
    ry_sts_t status;
    int nameLen = strlen(name);

    if (nameLen > MAX_CARD_NAME || aidLen > MAX_CARD_AID_LEN) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_INVALIC_PARA);
    }
    //ryos_mutex_lock(r_xfer_v.mutex);

    ry_memset((u8_t*)&newCard, 0, sizeof(card_transit_t));

    newCard.aidLen   = aidLen;
    newCard.selected = 0;
    //newCard.type     = card_getTypeByAID(aid, aidLen);
    newCard.type     = type;
    strcpy((char*)newCard.name, name);
    ry_memcpy(newCard.aid, aid, aidLen);

   
    status = ry_tbl_add((u8_t*)&cms_ctx_v.transitCardTbl, 
                 MAX_TRANSIT_CARD_NUM,
                 sizeof(card_transit_t),
                 (u8_t*)&newCard,
                 MAX_CARD_AID_LEN, 
                 0);


    //ryos_mutex_unlock(r_xfer_v.mutex);   

    if (status == RY_SUCC) {
        //arch_event_rule_save((u8 *)&mible_evtRuleTbl, sizeof(mible_evtRuleTbl_t));
        LOG_DEBUG("Transit card add success.\r\n");
    } else {
        LOG_WARN("Transit card add fail, error code = %x\r\n", status);
    }


    return status;
}

card_access_t* card_accessCardSearch(u8_t* aid, int aidLen)
{
    u8_t key[MAX_CARD_AID_LEN] = {0};
    ry_memcpy(key, aid, aidLen);
    return ry_tbl_search(MAX_ACCESS_CARD_NUM, 
               sizeof(card_access_t),
               (u8_t*)&cms_ctx_v.accessCardTbl.cards,
               0,
               MAX_CARD_AID_LEN,
               key);
}

ry_sts_t card_accessCardDel(u8_t* aid, int aidLen)
{
    ry_sts_t ret;
    u8_t key[MAX_CARD_AID_LEN]= {0};
    
    ry_memcpy(key, aid, aidLen);
    ret = ry_tbl_del((u8*)&cms_ctx_v.accessCardTbl,
                MAX_ACCESS_CARD_NUM,
                sizeof(card_access_t),
                aid,
                MAX_CARD_AID_LEN,
                0);


    if (ret != RY_SUCC) {
        LOG_WARN("Card del fail, error code = 0x02%x\r\n", ret);
    }

    return ret;
}


ry_sts_t card_accessCardAdd(char* name, u8_t* aid, int aidLen, u8_t* uid, u32_t color)
{
    card_access_t newCard;
    ry_sts_t status;
    int nameLen = strlen(name);

    if (nameLen > MAX_CARD_NAME || aidLen > MAX_CARD_AID_LEN) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_INVALIC_PARA);
    }
    //ryos_mutex_lock(r_xfer_v.mutex);

    ry_memset((u8_t*)&newCard, 0, sizeof(card_access_t));

    newCard.aidLen   = aidLen;
    newCard.color    = color;
    newCard.selected = 0;
    newCard.type     = CARD_ID_GATE;
    strcpy((char*)newCard.name, name);
    ry_memcpy(newCard.uid, uid, 4);
    ry_memcpy(newCard.aid, aid, aidLen);

   
    status = ry_tbl_add((u8_t*)&cms_ctx_v.accessCardTbl, 
                 MAX_ACCESS_CARD_NUM,
                 sizeof(card_access_t),
                 (u8_t*)&newCard,
                 MAX_CARD_AID_LEN, 
                 0);


    //ryos_mutex_unlock(r_xfer_v.mutex);   

    if (status == RY_SUCC) {
        //arch_event_rule_save((u8 *)&mible_evtRuleTbl, sizeof(mible_evtRuleTbl_t));
        LOG_DEBUG("Access card add success.\r\n");
    } else {
        LOG_WARN("Access card add fail, error code = %x\r\n", status);
    }


    return status;
}

ry_sts_t cms_psm_save(void)
{
    ry_sts_t status;

    status = ry_hal_flash_write(FLASH_ADDR_ACCESS_CARDS, (u8_t*)&cms_ctx_v, sizeof(cms_ctx_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}

ry_sts_t cms_psm_restore(void)
{
    ry_hal_flash_read(FLASH_ADDR_ACCESS_CARDS, (u8_t*)&cms_ctx_v, sizeof(cms_ctx_t));

    if (cms_ctx_v.accessCardTbl.curNum > MAX_ACCESS_CARD_NUM) {
        ry_memset((u8_t*)cms_ctx_v.accessCardTbl.cards, 0xFF, sizeof(card_access_t)*MAX_ACCESS_CARD_NUM);
        cms_ctx_v.accessCardTbl.curNum = 0;
    }

    if (cms_ctx_v.transitCardTbl.curNum > MAX_TRANSIT_CARD_NUM) {
        ry_memset((u8_t*)cms_ctx_v.transitCardTbl.cards, 0xFF, sizeof(card_transit_t)*MAX_TRANSIT_CARD_NUM);
        cms_ctx_v.transitCardTbl.curNum = 0;
    }

    cms_ctx_v.session = NULL;
    cms_ctx_v.state = 0;

    return RY_SUCC;
}


ry_sts_t card_accessCards_save(void)
{
    ry_sts_t status;

    status = ry_hal_flash_write(FLASH_ADDR_ACCESS_CARDS, (u8_t*)&cms_ctx_v.accessCardTbl, sizeof(accessCard_tbl_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}


ry_sts_t card_accessCards_restore(void)
{
    ry_hal_flash_read(FLASH_ADDR_ACCESS_CARDS, (u8_t*)&cms_ctx_v.accessCardTbl, sizeof(accessCard_tbl_t));

    if (cms_ctx_v.accessCardTbl.curNum > MAX_TRANSIT_CARD_NUM || cms_ctx_v.accessCardTbl.curNum == 0) {
        ry_memset((u8_t*)cms_ctx_v.accessCardTbl.cards, 0xFF, sizeof(card_access_t)*MAX_ACCESS_CARD_NUM);
        cms_ctx_v.accessCardTbl.curNum = 0;
    }

    return RY_SUCC;
}


ry_sts_t card_accessCards_reset(void)
{
    ry_memset((u8_t*)&cms_ctx_v.accessCardTbl, 0xFF, sizeof(accessCard_tbl_t));
    cms_ctx_v.accessCardTbl.curNum = 0;
    
	  ry_sts_t status = ry_hal_flash_write(FLASH_ADDR_ACCESS_CARDS, (u8_t*)&cms_ctx_v.accessCardTbl, sizeof(accessCard_tbl_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }
    return RY_SUCC;
}


ry_sts_t card_defaultCard_save(void)
{
    ry_sts_t status;

    status = ry_hal_flash_write(FLASH_ADDR_DEFAULT_CARD, 
                (u8_t*)&cms_ctx_v.defaultCard, 
                sizeof(cms_default_card_t));

    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}

ry_sts_t card_defaultCard_restore(void)
{
    ry_sts_t status;
    u8_t temp[18];

    ry_hal_flash_read(FLASH_ADDR_DEFAULT_CARD, 
        (u8_t*)&cms_ctx_v.defaultCard, 
        sizeof(cms_default_card_t));


    if (cms_ctx_v.defaultCard.mode >= CMS_DEFAULT_CARD_MODE_MAX || cms_ctx_v.defaultCard.mode == CMS_DEFAULT_CARD_MODE_INVALID) {
        cms_ctx_v.defaultCard.mode = CMS_DEFAULT_CARD_MODE_LAST_USE;
        ry_memset(cms_ctx_v.defaultCard.aid, 0, MAX_CARD_AID_LEN);
        ry_memset(cms_ctx_v.defaultCard.name, 0, MAX_CARD_NAME);
        cms_ctx_v.defaultCard.aidLen = 0;
        cms_ctx_v.defaultCard.type   = 0;
    }

    return RY_SUCC;
}


void card_additionalInfoTblRead(cardInfo_tbl_t* pTbl)
{
    ry_hal_flash_read(FLASH_ADDR_CARD_ADDITIONAL_INFO, (u8_t*)pTbl, sizeof(cardInfo_tbl_t));

    if (pTbl->curNum == 0 || pTbl->curNum == 0xFF || pTbl->curNum > MAX_CARD_NUM) {
        pTbl->curNum = 0;
        ry_memset((void*)pTbl->additionalInfos, 0xFF, sizeof(card_para_t)*MAX_CARD_NUM);
    }
}

ry_sts_t card_additionalInfoTblWrite(cardInfo_tbl_t* pTbl)
{
    ry_sts_t status;
    status = ry_hal_flash_write(FLASH_ADDR_CARD_ADDITIONAL_INFO, (u8_t*)pTbl, sizeof(cardInfo_tbl_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}



ry_sts_t card_additionalInfoAdd(u8_t* aid, int aidLen, char* cityId, char* cityName, nfc_setting_t* setting)
{
    card_para_t newInfo;
    ry_sts_t status;
    cardInfo_tbl_t *pTbl = NULL;
    
    int nameLen = strlen((char*)cityName);

    if (nameLen > MAX_CITY_NAME_LEN || aidLen > MAX_CARD_AID_LEN) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_INVALIC_PARA);
    }


    /* Card info table restore first */
    pTbl = (cardInfo_tbl_t*)ry_malloc(sizeof(cardInfo_tbl_t));
    if (pTbl == NULL) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_NO_MEM);
    }
    
    card_additionalInfoTblRead(pTbl);



    /* Add new entry */
    ry_memset((u8_t*)&newInfo, 0, sizeof(card_para_t));

    newInfo.aidLen   = aidLen; 
    strcpy(newInfo.cityId, cityId);
    strcpy((char*)newInfo.cityName, (char*)cityName);
    ry_memcpy(newInfo.aid, aid, aidLen);
    ry_memcpy((u8_t*)&newInfo.para, (u8_t*)setting, sizeof(nfc_setting_t));

   
    status = ry_tbl_add((u8_t*)pTbl, 
                 MAX_CARD_NUM,
                 sizeof(card_para_t),
                 (u8_t*)&newInfo,
                 MAX_CARD_AID_LEN, 
                 0);
    if (status != RY_SUCC) {
        goto exit;
    }

    /* Save the table to flash */
    status = card_additionalInfoTblWrite(pTbl);

exit:
    if (status == RY_SUCC) {
        //arch_event_rule_save((u8 *)&mible_evtRuleTbl, sizeof(mible_evtRuleTbl_t));
        LOG_DEBUG("Additiaonal card info add success.\r\n");
    } else {
        LOG_ERROR("Additional card info add fail, error code = %x\r\n", status);
    }
    
    ry_free((void*)pTbl);
    return status;
}


ry_sts_t card_additionalInfoDel(u8_t* aid, int aidLen)
{
    ry_sts_t ret;
    u8_t key[MAX_CARD_AID_LEN]= {0};
    cardInfo_tbl_t *pTbl = NULL;

    /* Card info table restore first */
    pTbl = (cardInfo_tbl_t*)ry_malloc(sizeof(cardInfo_tbl_t));
    if (pTbl == NULL) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_NO_MEM);
    }
    
    card_additionalInfoTblRead(pTbl);
    
    ry_memcpy(key, aid, aidLen);
    ret = ry_tbl_del((u8*)pTbl,
                MAX_CARD_NUM,
                sizeof(card_para_t),
                aid,
                MAX_CARD_AID_LEN,
                0);

    if (ret != RY_SUCC) {
        goto exit;
    }

    /* Save the table to flash */
    ret = card_additionalInfoTblWrite(pTbl);


exit:

    if (ret != RY_SUCC) {
        LOG_ERROR("Card del fail, error code = 0x02%x\r\n", ret);
    }

    return ret;
}



ry_sts_t card_additionalInfo_update(u8_t* aid, u32_t aidLen, char *cityId, char* cityName, nfc_setting_t* para)
{
    card_para_t *pInfo = NULL;
    cardInfo_tbl_t *pTbl = NULL;
    ry_sts_t status = RY_SUCC;
    
    u8_t key[MAX_CARD_AID_LEN] = {0};
    
    /* Card info table restore first */
    pTbl = (cardInfo_tbl_t*)ry_malloc(sizeof(cardInfo_tbl_t));
    if (pTbl == NULL) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_NO_MEM);
    }
    
    card_additionalInfoTblRead(pTbl);

    ry_memcpy(key, aid, aidLen);
    pInfo = ry_tbl_search(MAX_CARD_NUM, 
               sizeof(card_para_t),
               (u8_t*)&pTbl->additionalInfos,
               0,
               MAX_CARD_AID_LEN,
               key);

    if (!pInfo) {
        goto exit;
    }

    /* Generate output value */
    strcpy(pInfo->cityId, cityId);
    strcpy(pInfo->cityName, cityName);
    ry_memcpy((u8_t*)&pInfo->para, (u8_t*)para, sizeof(nfc_setting_t));

    /* Save the table to flash */
    status = card_additionalInfoTblWrite(pTbl);

    ry_free((void*)pTbl);
    return RY_SUCC;


exit:
    ry_free((void*)pTbl);
    return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_TABLE_NOT_FOUND);

}



ry_sts_t card_additionalInfo_get(u8_t* aid, u32_t aidLen, char *cityId, char* cityName, nfc_setting_t* para)
{
    card_para_t *pInfo = NULL;
    cardInfo_tbl_t *pTbl = NULL;
    ry_sts_t status = RY_SUCC;
    u8_t key[MAX_CARD_AID_LEN] = {0};
    
    /* Card info table restore first */
    pTbl = (cardInfo_tbl_t*)ry_malloc(sizeof(cardInfo_tbl_t));
    if (pTbl == NULL) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_NO_MEM);
    }
    
    card_additionalInfoTblRead(pTbl);

    ry_memcpy(key, aid, aidLen);
    pInfo = ry_tbl_search(MAX_CARD_NUM, 
               sizeof(card_para_t),
               (u8_t*)&pTbl->additionalInfos,
               0,
               MAX_CARD_AID_LEN,
               key);

    if (!pInfo) {
        goto exit;
    }

    /* Generate output value */
    strcpy(cityId, pInfo->cityId);
    ry_memcpy(cityName, pInfo->cityName, MAX_CITY_NAME_LEN);
    ry_memcpy((u8_t*)para, (u8_t*)&pInfo->para, sizeof(nfc_setting_t));

    ry_free((void*)pTbl);
    return RY_SUCC;


exit:
    ry_free((void*)pTbl);
    return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_TABLE_NOT_FOUND);

}


ry_sts_t card_additionalInfos_save(void)
{
    ry_sts_t status;
    u32_t num;

    status = ry_hal_flash_write(FLASH_ADDR_CARD_ADDITIONAL_INFO, (u8_t*)&cms_ctx_v.accessCardTbl, sizeof(accessCard_tbl_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}




ry_sts_t card_curCard_save(void)
{
    ry_sts_t status;
    u8_t temp[18];

    temp[0] = cms_ctx_v.curType;
    ry_memcpy(&temp[1], cms_ctx_v.curAid, MAX_CARD_AID_LEN);
    temp[17] = cms_ctx_v.curAidLen;

    status = ry_hal_flash_write(FLASH_ADDR_CUR_CARD, temp, 18);
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}

ry_sts_t card_curCard_restore(void)
{
    ry_sts_t status;
    u8_t temp[18];

    ry_hal_flash_read(FLASH_ADDR_CUR_CARD, (u8_t*)temp, 18);

    cms_ctx_v.curType = temp[0];
    ry_memcpy(cms_ctx_v.curAid, &temp[1], MAX_CARD_AID_LEN);
    cms_ctx_v.curAidLen = temp[17];

    if (cms_ctx_v.curType > CARD_ID_TRANSIT || cms_ctx_v.curAidLen > 16) {
        cms_ctx_v.curType = 0;
        ry_memset(cms_ctx_v.curAid, 0, MAX_CARD_AID_LEN);
        cms_ctx_v.curAidLen = 0;
    } else {
        status = card_additionalInfo_get(cms_ctx_v.curAid, cms_ctx_v.curAidLen,
                    cms_ctx_v.curCardInfo.cityId, cms_ctx_v.curCardInfo.cityName, 
                    &cms_ctx_v.curCardInfo.para); 
        
        if (status == RY_SUCC) {
            ry_memcpy(cms_ctx_v.curCardInfo.aid, cms_ctx_v.curAid, MAX_CARD_AID_LEN);
            cms_ctx_v.curCardInfo.aidLen = cms_ctx_v.curAidLen;
        } else {
            ry_memset((void*)&cms_ctx_v.curCardInfo, 0, sizeof(card_para_t));
        }
    }

    return RY_SUCC;
}


ry_sts_t card_transitCards_save(void)
{
    ry_sts_t status;

    status = ry_hal_flash_write(FLASH_ADDR_TRANSIT_CARDS, (u8_t*)&cms_ctx_v.transitCardTbl, sizeof(transitCard_tbl_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}


ry_sts_t card_transitCards_restore(void)
{
    ry_hal_flash_read(FLASH_ADDR_TRANSIT_CARDS, (u8_t*)&cms_ctx_v.transitCardTbl, sizeof(transitCard_tbl_t));

    //if (cms_ctx_v.transitCardTbl.curNum > MAX_TRANSIT_CARD_NUM || cms_ctx_v.transitCardTbl.curNum == 0) {
        ry_memset((u8_t*)cms_ctx_v.transitCardTbl.cards, 0xFF, sizeof(card_transit_t)*MAX_TRANSIT_CARD_NUM);
        cms_ctx_v.transitCardTbl.curNum = 0;
    //}

    return RY_SUCC;
}


ry_sts_t card_transitCards_reset(void)
{
    ry_memset((u8_t*)&cms_ctx_v.transitCardTbl, 0xFF, sizeof(transitCard_tbl_t));
    cms_ctx_v.transitCardTbl.curNum = 0;
    ry_sts_t status = ry_hal_flash_write(FLASH_ADDR_TRANSIT_CARDS, (u8_t*)&cms_ctx_v.transitCardTbl, sizeof(transitCard_tbl_t));
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_PARA_SAVE);
    }
    return RY_SUCC;
}

transitCard_tbl_t* card_getTransitList(void)
{
    return &cms_ctx_v.transitCardTbl;
}

accessCard_tbl_t* card_getAccessList(void)
{
    return &cms_ctx_v.accessCardTbl;
}



int card_getAccessCardNum(void)
{
    return cms_ctx_v.accessCardTbl.curNum;
}

int card_getTransitCardNum(void)
{
    return cms_ctx_v.transitCardTbl.curNum;
}


card_access_t* card_getAccessCardByIdx(int idx)
{
    return &cms_ctx_v.accessCardTbl.cards[idx];
}

card_transit_t* card_getTransitCardByIdx(int idx)
{
    return &cms_ctx_v.transitCardTbl.cards[idx];
}



void card_table_test(void)
{
    u8_t aid[10] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    u8_t uid[4] = {0xa9, 0x93, 0x44, 0x45};
    ry_sts_t ret = card_accessCardAdd("card1", aid, 6, uid, 0);
    if (ret != RY_SUCC) {
        LOG_ERROR("[card_onCreateAccessAck] Add access card to table fail. %04x\r\n", ret);
    }

    u8_t aid2[10] = {0x77, 0x55, 0x44, 0x44, 0x55, 0x66};
    u8_t uid2[4] = {0x34, 0x63, 0x35, 0x65};
    ret = card_accessCardAdd("card2", aid2, 6, uid2, 0);
    if (ret != RY_SUCC) {
        LOG_ERROR("[card_onCreateAccessAck] Add access card to table fail. %04x\r\n", ret);
    }

    card_accessCards_save();
    LOG_DEBUG("Access Cards Num: %d\r\n", card_getAccessCardNum());
}


int cms_default_selected = 0;
int card_select_default_cb(u32_t card_id, u8_t status, char* name, u8_t* aid, u8_t aidLen)
{
    cms_default_selected = 1;

    if (status == 0) {
        cms_ctx_v.curType = card_id;
        ry_memcpy(cms_ctx_v.curAid, aid, aidLen);
        cms_ctx_v.curAidLen = aidLen;

        card_curCard_save();
        
        LOG_DEBUG("[%s] selected card:%s, status: %d\r\n", __FUNCTION__, name, status);
    } else {
        LOG_ERROR("[%s] selected card:%s, status: %d\r\n", __FUNCTION__, name, status);
    }
    
    
    return 0;
}

u8_t card_is_synced(void)
{
    return cms_ctx_v.isSync;
}



void card_sync_with_se(void)
{
    int i;
    card_access_t* p = NULL;
    card_transit_t* q = NULL;
    u8_t tmp[4] = {0};
    ry_sts_t status;
    int balance;
    u8_t type;

    /* Sync Access Card */
    for (i = 0; i < MAX_ACCESS_CARD_NUM; i++) {
        //status = card_doSelectCard((u8_t*)access_card_aids[i], 11);
        status = card_queryCard((u8_t*)access_card_aids[i], 11);
        if (RY_SUCC == status) {
            /* Found card and check if the card is already in the table */
            p = card_accessCardSearch((u8_t*)access_card_aids[i], 11);
            if (p == NULL) {
                /* Need to sync */
                status = card_accessCardAdd((char*)string_default_gate, (u8_t*)access_card_aids[i], 11, tmp, 0);
                if (status != RY_SUCC) {
                    LOG_ERROR("[%s] Add access card fail.status = %x\r\n", __FUNCTION__, status);
                }
            } else {
                p->selected = 0;
            }
        }
    }

    /* Sync Transit Card */
    for (i = 0; i < SUPPORTED_CARD_NUM; i++) {
        status = card_queryCard((u8_t*)support_transit_card_list[i].aid, support_transit_card_list[i].aidLen);
        if (RY_SUCC == status) {
            /* Found card and check if the card is already in the table */
            q = card_transitCardSearch((u8_t*)support_transit_card_list[i].aid, support_transit_card_list[i].aidLen);
            if (q == NULL) {
                /* Need to sync */
                type = card_getTypeByAID((u8_t*)support_transit_card_list[i].aid, support_transit_card_list[i].aidLen);
                status = card_transitCardAdd((char*)support_transit_card_list[i].name, 
                            (u8_t*)support_transit_card_list[i].aid, 
                            support_transit_card_list[i].aidLen, 
                            type);
                if (status != RY_SUCC) {
                    LOG_ERROR("[%s] Add Transit card fail.status = %x\r\n", __FUNCTION__, status);
                    continue;
                }
                q = card_transitCardSearch((u8_t*)support_transit_card_list[i].aid, support_transit_card_list[i].aidLen);
                
            } else {
                q->selected = 0;
            }

            balance = query_balance((u8_t*)support_transit_card_list[i].aid, support_transit_card_list[i].aidLen);
            LOG_DEBUG("Balance: %d\r\n", balance);
            if (q) {
                q->balance = balance;
            }

            if (q && q->type == CARD_ID_LNT) {

                query_city((u8_t*)support_transit_card_list[i].aid, support_transit_card_list[i].aidLen);
            }
            
        }
    }

    cms_ctx_v.isSync = 1;

}



bool card_select_specific_as_default(u8_t type, u8_t* aid, int aidLen)
{
    int accessNum, transitNum;
    int cnt = 0;
    int balance;
    u8_t status;

    transitNum = card_getTransitCardNum();
    for (int i = 0; i < MAX_TRANSIT_CARD_NUM; i++) {
        if (is_aid_empty(cms_ctx_v.transitCardTbl.cards[i].aid) == FALSE) {
            if (0 == ry_memcmp(aid, cms_ctx_v.transitCardTbl.cards[i].aid, aidLen)) {
                ry_nfc_sync_ce_on();

                status = card_doSelectCard(cms_ctx_v.transitCardTbl.cards[i].type, 
                            cms_ctx_v.transitCardTbl.cards[i].aid, 
                            cms_ctx_v.transitCardTbl.cards[i].aidLen,
                            (void*)&balance);

                if (status == 0) {
                    return TRUE;
                }
            }
        }
    }


    accessNum = card_getAccessCardNum();
    for (int i = 0; i < MAX_ACCESS_CARD_NUM; i++) {
        if (is_aid_empty(cms_ctx_v.accessCardTbl.cards[i].aid) == FALSE) {
            if (0 == ry_memcmp(aid, cms_ctx_v.accessCardTbl.cards[i].aid, aidLen)) {
                ry_nfc_sync_ce_on();

                status = card_doSelectCard(cms_ctx_v.accessCardTbl.cards[i].type, 
                            cms_ctx_v.accessCardTbl.cards[i].aid, 
                            cms_ctx_v.accessCardTbl.cards[i].aidLen,
                            NULL);

                if (status == 0) {
                    return TRUE;
                }
            }
        }
    }
    
    return FALSE;
    
}



bool card_select_first_as_default(void)
{
    int accessNum, transitNum;
    int cnt = 0;
    int balance;

    transitNum = card_getTransitCardNum();

    for (int i = 0; i < MAX_TRANSIT_CARD_NUM; i++) {
        if (is_aid_empty(cms_ctx_v.transitCardTbl.cards[i].aid) == FALSE) {
            cms_default_selected = 0;
            ry_nfc_sync_ce_on();
            card_doSelectCard(cms_ctx_v.transitCardTbl.cards[i].type, 
                        cms_ctx_v.transitCardTbl.cards[i].aid, 
                        cms_ctx_v.transitCardTbl.cards[i].aidLen,
                        (void*)&balance);
            //goto exit;
            return TRUE;
        }
    }
		
	accessNum = card_getAccessCardNum();
    for (int i = 0; i < MAX_ACCESS_CARD_NUM; i++) {
        if (is_aid_empty(cms_ctx_v.accessCardTbl.cards[i].aid) == FALSE) {
            cms_default_selected = 0;
            ry_nfc_sync_ce_on();
            card_doSelectCard(CARD_ID_GATE, 
                        cms_ctx_v.accessCardTbl.cards[i].aid, 
                        cms_ctx_v.accessCardTbl.cards[i].aidLen,
                        NULL);
            //goto exit;
            return TRUE;
        }
    }

    

    return FALSE;
}


bool card_select_default(void)
{
    bool selected = FALSE;
    if (cms_ctx_v.defaultCard.mode == CMS_DEFAULT_CARD_MODE_SPECIFIC) {
        selected = card_select_specific_as_default(cms_ctx_v.defaultCard.type, 
                        cms_ctx_v.defaultCard.aid,
                        cms_ctx_v.defaultCard.aidLen);
        if (selected) 
            return selected;
        
    } 

    if (cms_ctx_v.defaultCard.mode == CMS_DEFAULT_CARD_MODE_LAST_USE) {
        selected = card_select_specific_as_default(cms_ctx_v.curType, 
                        cms_ctx_v.curAid,
                        cms_ctx_v.curAidLen);
        if (selected) 
            return selected;
    }

    return card_select_first_as_default();

    
}





void cms_selectCardTimeout_handler(void* arg)
{
    cms_msg_t *msg = (cms_msg_t *)arg;
    cms_cmd_selectTransit_t* pTransit = NULL;
    cms_cmd_selectGate_t* pGate = NULL;

    LOG_ERROR("---[%s] Card select timeout. Need to recover---\r\n", __FUNCTION__);

    switch (msg->msgType) {
        case CMS_CMD_SELECT_TRANSIT_CARD:
            pTransit = (cms_cmd_selectTransit_t*)msg->data;
            if (pTransit->cb) {
                pTransit->cb(pTransit->card_id, CARD_SELECT_STATUS_TIMEOUT, 
                    (char*)pTransit->pt->name, pTransit->pt->aid, pTransit->pt->aidLen, (void*)0);
            }
            return;

        case CMS_CMD_SELECT_GATE_CARD:
            pGate = (cms_cmd_selectGate_t*)msg->data;
            if (pGate->cb) {
                pGate->cb(pGate->pa->color, CARD_SELECT_STATUS_TIMEOUT, 
                    (char*)pGate->pa->name, pGate->pa->aid, pGate->pa->aidLen, NULL);
            }
            return;


        default:
            break;
    }

    ry_free(arg);

    /* TODO: to recover the NFC task and CMS task */
    ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
    
}  


void cms_select_timer_start(void* arg)
{
    LOG_DEBUG("----CMS Select Timer Start---\r\n");
    ry_timer_start(cms_ctx_v.selectTimer, CMS_SELECT_CARD_TIMEOUT, cms_selectCardTimeout_handler, arg);
}


void cms_select_timer_stop(void)
{
    ry_sts_t status;
    LOG_DEBUG("----CMS Select Timer Stop---\r\n");
    status = ry_timer_stop(cms_ctx_v.selectTimer);
    if (status != RY_SUCC) {
        LOG_DEBUG("[%s] timer stop fail, status = %x\r\n", __FUNCTION__, status);
    }
}



void cms_msgHandler(cms_msg_t *msg)
{
    monitor_msg_t* p;
    cms_cmd_selectGate_t* pGate = NULL;
    cms_cmd_selectTransit_t* pTransit = NULL;
	
    switch (msg->msgType) {
        case CMS_CMD_SELECT_TRANSIT_CARD:
            LOG_DEBUG("[%s] select transit card.\r\n", __FUNCTION__);
            pTransit = (cms_cmd_selectTransit_t*)msg->data;
            cms_select_timer_stop();
            cms_select_timer_start((void*)msg);
            card_selectCard(pTransit->card_id, pTransit->pt, pTransit->cb);
            cms_select_timer_stop();
            ry_free(msg);
            LOG_DEBUG("[%s] select transit card end.\r\n", __FUNCTION__);
            return;

        case CMS_CMD_SELECT_GATE_CARD:
            LOG_DEBUG("[%s] select gate card.\r\n", __FUNCTION__);
            pGate = (cms_cmd_selectGate_t*)msg->data;
            cms_select_timer_stop();
            cms_select_timer_start((void*)msg);
            card_selectGate(pGate->card_id, pGate->pa, pGate->cb);
            cms_select_timer_stop();
            ry_free(msg);
            LOG_DEBUG("[%s] select transit card end.\r\n", __FUNCTION__);
            return;

        case CMS_CMD_SELECT_DEFAULT_CARD:
            LOG_DEBUG("[%s] select default card.\r\n", __FUNCTION__);
            ry_nfc_sync_poweroff();
            cms_select_default_card();
            ry_free(msg);
            LOG_DEBUG("[%s] select default card end.\r\n", __FUNCTION__);
            return;

        case CMS_CMD_MONITOR:
            p = (monitor_msg_t*)msg->data;
            *(u32_t*)p->dataAddr = 0;
            break;

        default:
            break;
    }

exit:
    /* Always don't forget to free the data buffer */
    ry_free(msg);
}



void cms_msg_send(u32_t msg_type, int len, void* data)
{
    ry_sts_t status = RY_SUCC;
    cms_msg_t *p = (cms_msg_t*)ry_malloc(sizeof(cms_msg_t)+len);
    if (NULL == p) {

        LOG_ERROR("[cms_msg_send]: No mem.\n");

        // For this error, we can't do anything. 
        // Wait for timeout and memory be released.
        return; 
    }   

    p->msgType = msg_type;
    p->len = len;
    if (len>0) {
        ry_memcpy(p->data, data, len);
    }

    if (RY_SUCC != (status = ryos_queue_send(cms_msgQ, &p, 4))) {
        LOG_ERROR("[cms_msgQ]: Add to Queue fail. status = 0x%04x\n", status);
    }        
}




/*
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int cms_thread(void* arg)
{
    cms_msg_t *ctrlMsg;
    ry_sts_t status = RY_SUCC;

    LOG_INFO("[cms_thread] Enter.\n");

    /* Create the RYBLE RX queue */
    status = ryos_queue_create(&cms_msgQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[cms_thread]: Msg Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
    
    while (1) {

        status = ryos_queue_recv(cms_msgQ, &ctrlMsg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[cms_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {           
            cms_msgHandler(ctrlMsg);
        }

        ryos_delay_ms(10);
    }
}





void card_service_sync(void)
{
    if (card_is_synced() == 0) {
        /* Sync the card with SE first. */
        if (ry_nfc_get_state() < RY_NFC_STATE_INITIALIZED) {
            ry_nfc_sync_poweron();
        }

        card_sync_with_se();
    }

    /* Check if card swipping mode is OFF */
    if (get_card_default_enable() == 0) {
        /* Power off the NFC in all case */
        if (ry_nfc_get_state() != RY_NFC_STATE_IDLE) {
            goto poweroff;
        }

        return;
    }

    /* Select Default card if necessary */
    if (ry_nfc_get_state() == RY_NFC_STATE_IDLE) {
        ry_nfc_sync_poweron();
    }
    
    if (TRUE == card_select_default()) {
        if (get_hardwardVersion() > 3) {
            if (card_is_current_disable_lowpower() == FALSE) {
                ry_nfc_sync_lowpower();
            }
        }
        return;
    }

poweroff:
    ryos_delay_ms(2000);
    ry_nfc_sync_poweroff();
}



/**
 * @brief   API to init card manager service
 *
 * @param   None
 *
 * @return  None
 */
void card_service_init(void)
{
    int accessNum, transitNum;
    ryos_threadPara_t para;
    ry_sts_t status;
    ry_timer_parm timer_para;

    memset((void*)&cms_ctx_v, 0, sizeof(cms_ctx_t));

    if (se_mutex == NULL) {
        status = ryos_mutex_create(&se_mutex);
        if (RY_SUCC != status) {
            RY_ASSERT(0);
        } 
    }

    /* Init Cards Table */
    ry_memset((u8_t*)cms_ctx_v.accessCardTbl.cards,  0xFF, sizeof(card_access_t)*MAX_ACCESS_CARD_NUM);
    ry_memset((u8_t*)cms_ctx_v.transitCardTbl.cards, 0xFF, sizeof(card_transit_t)*MAX_TRANSIT_CARD_NUM);
    cms_ctx_v.accessCardTbl.curNum  = 0;
    cms_ctx_v.transitCardTbl.curNum = 0;
    ry_memset((u8_t*)&cms_ctx_v.defaultCard, 0, sizeof(cms_default_card_t));
    cms_ctx_v.isSync = 0;


    /* Create the timer once */
    timer_para.timeout_CB = cms_selectCardTimeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "cmsSelectTimer";
    cms_ctx_v.selectTimer = ry_timer_create(&timer_para);

    
    /* Restore card from flash if any */
    card_accessCards_restore();
    card_transitCards_restore();
    card_curCard_restore();
    card_defaultCard_restore();

    LOG_DEBUG("Access Cards Num: %d\r\n", card_getAccessCardNum());
    LOG_DEBUG("Transit Cards Num: %d\r\n", card_getTransitCardNum());


    /* Start CMS Thread */
    strcpy((char*)para.name, "cms_thread");
    para.stackDepth = 400;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 3;
    status = ryos_thread_create(&cms_thread_handle, cms_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_ble_init]: Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }

    /* PATCH for activate 9412 first */
    ry_nfc_sync_poweron();

    card_sync_with_se();

    /* Close SE when card_swiping mode is OFF */
    if (get_card_default_enable() == 0) {
        ryos_delay_ms(2000);
        ry_nfc_sync_poweroff();
        return;
    }

    if (TRUE == card_select_default()) {
        ryos_delay_ms(2000);
        if (get_hardwardVersion()>3) {
            ry_nfc_sync_lowpower();
        }
        return;
    }
    
exit:

    //return; // Not enter low power mode until the 1M resister fix.
    
    ryos_delay_ms(2000);
    ry_nfc_sync_poweroff();
 
    return;

}

void card_session_timeout_handler(void* arg)
{
    LOG_ERROR("[card_session_timeout_handler]: Session timeout. Cur State:%d\r\n", cms_ctx_v.session->state);

    /* Close the reader first */
    if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_READER_DETECT) {
        //ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_OFF, NULL);
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
        cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_DETECT, CARD_CREATE_EVT_CODE_FAIL);
    } else if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_CREATE_ACCESS_ACK) {
        cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_COPY, CARD_CREATE_EVT_CODE_FAIL);
    } else if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_CREATE_TRANSIT_ACK) {
        cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_CREATE_EVT_STATE_TRANSIT, CARD_CREATE_EVT_CODE_FAIL);
    } 
    
    /* Delete the session */
    ry_timer_delete(cms_ctx_v.session->timer);
    ry_free(cms_ctx_v.session);
    cms_ctx_v.session = NULL;

    /* Recovery the state */
    cms_ctx_v.state = CMS_STATE_IDLE;

    /* Recovery Battery task  */
    set_device_session(DEV_SESSION_IDLE);
}    


void card_session_timer_start(int timeout)
{
    if (cms_ctx_v.session && cms_ctx_v.session->timer) {
        ry_timer_start(cms_ctx_v.session->timer, timeout, card_session_timeout_handler, NULL);
    }
}

void card_session_timer_stop(void)
{
   if (cms_ctx_v.session && cms_ctx_v.session->timer) {
       ry_timer_stop(cms_ctx_v.session->timer);
   }
}

ry_sts_t card_session_new(u32_t sessionState, u8_t* aid, int aidLen, char* name, int timeout, card_uiCb_t cb, u32_t card_id, void* pCard)
{
    ry_sts_t ry_sts;
    /* Start session to create */
    if (cms_ctx_v.session) {
        // Error code :Busy
        //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, 5, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, 5, NULL, 0, 1,
                    cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
        }
        return RY_SET_STS_VAL(RY_CID_CMS, RY_ERR_BUSY);
    }

    cms_ctx_v.session = (card_ota_session_t*)ry_malloc(sizeof(card_ota_session_t));
    if (!cms_ctx_v.session) {
        LOG_ERROR("[card_session_new] No mem.\r\n");
        while(1);
    }

    ry_memset((u8_t*)cms_ctx_v.session, 0, sizeof(card_ota_session_t));
    cms_ctx_v.session->state = sessionState;
    cms_ctx_v.session->aidLen = aidLen;
    if (aidLen) {
        ry_memcpy(cms_ctx_v.session->aid, aid, aidLen);
    }

    if (name) {
        strcpy(cms_ctx_v.session->name, name);
    }
    
    cms_ctx_v.session->ui_callback = cb;
    cms_ctx_v.session->card_id     = card_id;
    cms_ctx_v.session->pCard       = pCard;

    /* Create and start session time */
    card_session_timer_create();
    card_session_timer_start(timeout);

    return RY_SUCC;
}


ry_sts_t card_session_delete(void)
{
    card_session_timer_stop();
    ry_timer_delete(cms_ctx_v.session->timer);
    ry_free((u8_t*)cms_ctx_v.session);
    cms_ctx_v.session = NULL;
    return RY_SUCC;
}

void card_session_timer_create(void)
{
    ry_timer_parm timer_para;
    /* Create the timer */
    timer_para.timeout_CB = card_session_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "card_session_timer";
    cms_ctx_v.session->timer = ry_timer_create(&timer_para);
}


void card_onSEOpen(void)
{
    ry_sts_t se_status;
    ry_sts_t ry_sts;

    
    set_device_session(DEV_SESSION_CARD);
	
    /* Check NFC state */
    if (ry_nfc_get_state() == RY_NFC_STATE_IDLE || ry_nfc_get_state() == RY_NFC_STATE_LOW_POWER) {
        ry_sched_addNfcEvtListener(card_onNfcEvent);
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
        cms_ctx_v.state = CMS_STATE_SE_OPEN;
        return;
    }

    /* Open the SE wired channel */
    se_status = ry_nfc_se_channel_open();
    if (se_status) {
        LOG_DEBUG("[card_onSEOpen] Open SE channel fail. %x\r\n", se_status);
    }

    //rbp_send_resp(CMD_DEV_SE_OPEN, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);

    ry_sts = ble_send_response(CMD_DEV_SE_OPEN, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (ry_sts != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }
}


void card_onSEClose(void)
{
    ry_sts_t ry_sts;

    LOG_DEBUG("Se channel close.\r\n");
    ry_nfc_se_channel_close();
    LOG_DEBUG("Se channel close succ.\r\n");

    if (cms_ctx_v.accessCardTbl.curNum + cms_ctx_v.transitCardTbl.curNum == 0) {
        //ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
        ry_nfc_sync_poweroff();
    } else {
        if (get_hardwardVersion()>3) {
            LOG_DEBUG("[%s] NFC Low Power.\r\n");
            if (get_hardwardVersion()>3) {
                ry_nfc_sync_lowpower();
            }
        }
    }
    
    //rbp_send_resp(CMD_DEV_SE_CLOSE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_sts = ble_send_response(CMD_DEV_SE_CLOSE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (ry_sts != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
    }

    set_device_session(DEV_SESSION_IDLE);

}


void card_onStartAccessCheck(void)
{
    ry_sts_t ry_sts;
    if (cms_ctx_v.session) {

        if (cms_ctx_v.session->state != CMS_SESSION_STATE_WAIT_CARD_OTA) {
            // Busy
            //rbp_send_resp(CMD_DEV_GATE_START_CHECK, 3, NULL, 0, 1);
            ry_sts = ble_send_response(CMD_DEV_GATE_START_CHECK, RBP_RESP_CODE_BUSY, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
            if (ry_sts != RY_SUCC) {

                LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            }
            return;
        } else {
            LOG_ERROR("[%s] current session state:%d\r\n", __FUNCTION__, cms_ctx_v.session->state);
        }
    } else {

        /* Check if we are in the low power mode */
        if (get_device_powerstate() == DEV_PWR_STATE_LOW_POWER || 
            get_device_powerstate() == DEV_PWR_STATE_OFF) {
            ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_LOW_POWER, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
            LOG_ERROR("[%s] Low Power.\r\n", __FUNCTION__);
            return;
        }
    
        /* Create a new session */
        cms_ctx_v.session = (card_ota_session_t*)ry_malloc(sizeof(card_ota_session_t));
        if (!cms_ctx_v.session) {
            LOG_ERROR("[card_onStartAccessCheck] No mem.\r\n");
            return;
        }

        set_device_session(DEV_SESSION_CARD);

        /* Send response */
        //rbp_send_resp(CMD_DEV_GATE_START_CHECK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);

        ry_sts = ble_send_response(CMD_DEV_GATE_START_CHECK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            ry_free((void*)cms_ctx_v.session);
            cms_ctx_v.session = NULL;
            return;
        }

        ryos_delay_ms(250);

        ry_memset((u8_t*)cms_ctx_v.session, 0, sizeof(card_ota_session_t));
        card_session_timer_create();
    }

    cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_READER_ON;
    card_session_timer_start(CMS_SESSION_ACCESS_CARD_CHECK_TIMEOUT);

    /* Disable battery task during NFC operation */

    /* Subscribe Kernel NFC event */
    ry_sched_addNfcEvtListener(card_onNfcEvent);
    
    
    /* Send Message to OPEN NFC first */
    //ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
    ry_nfc_msg_send(RY_NFC_MSG_TYPE_READER_ON, NULL);
    

    cms_ctx_v.state = CMS_STATE_READ_CARD;
    
}


void card_selectCard(u32_t card_id, card_transit_t *pt, card_uiCb_t cb)
{
    u8_t status;
    int balance;
    
    if (CARD_ID_JJJ == card_id ||
        CARD_ID_LNT == card_id ||
        CARD_ID_SZT == card_id ||
        CARD_ID_BJT == card_id ||
        CARD_ID_WHT == card_id ||
        CARD_ID_CQT == card_id ||
        CARD_ID_JST == card_id ||
        CARD_ID_YCT == card_id ||
        CARD_ID_CAT == card_id ||
        CARD_ID_GXT == card_id ||
        CARD_ID_JLT == card_id ||
        CARD_ID_HFT == card_id ||
        CARD_ID_HEB == card_id ||
        CARD_ID_LCT == card_id ||
        CARD_ID_QDT == card_id) {

        /* Check NFC state */
        if (ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_ON, NULL);
            while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
                ryos_delay_ms(50);
            }
        }


        status = card_doSelectCard(card_id, pt->aid, pt->aidLen, (void*)&balance);
        if (status != 0) {
            LOG_ERROR("[%s] Card select fail. %d\r\n", __FUNCTION__, status);
        }
        
        if (cb) {
            cb(card_id, status, (char*)pt->name, pt->aid, pt->aidLen, (void*)balance);
        }
    }

    return;
    
}


void build_select_apdu(u8_t* apdu, u8_t* aid, u8_t len)
{
    apdu[0] = 0x00;
    apdu[1] = 0xA4;
    apdu[2] = 0x04;
    apdu[3] = 0x00;
    apdu[4] = len;
    ry_memcpy(apdu+5, aid, len);
}



u8_t card_queryCard(u8_t* aid, u8_t aidLen)
{
    u8_t* apdu;
    u8_t  status;
	
    apdu = ry_malloc(aidLen + 5);
    if (apdu == NULL) {
        return CARD_SELECT_STATUS_NO_MEM;
    }
    
    build_select_apdu(apdu, aid, aidLen);
    status = select_aid(apdu, aidLen + 5);
    ry_free(apdu);
		
    return status;
}

void card_unselectAll(void)
{
    int i;
    card_transit_t* pt;
    card_access_t* pa;
    u8_t status;

    for (i = 0; i < cms_ctx_v.transitCardTbl.curNum; i++) {
        pt = &cms_ctx_v.transitCardTbl.cards[i];
        if (is_aid_empty(pt->aid)) {
            continue;
        }

        status = activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), pt->aid, pt->aidLen);
        pt->selected = 0;
    }

    for (i = 0; i < cms_ctx_v.accessCardTbl.curNum; i++) {
        pa = &cms_ctx_v.accessCardTbl.cards[i];
        if (is_aid_empty(pa->aid)) {
            continue;
        }

        status = activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), pa->aid, pa->aidLen);
        pa->selected = 0;
    }

    ry_memset(cms_ctx_v.curAid, 0, 16);
    cms_ctx_v.curAidLen = 0;
    cms_ctx_v.curType   = 0;
}

u8_t card_doSelectCard(u32_t type, u8_t* aid, u8_t aidLen, void* data)
{
    u8_t* apdu;
    u8_t  status;
    int *balance = (int*)data;


    if (cms_ctx_v.curAidLen != 0) {
        LOG_DEBUG("-------------Deactivate.--------------\r\n");
        status = activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), cms_ctx_v.curAid, cms_ctx_v.curAidLen);
        if (status == 0) {
            if (cms_ctx_v.curType == CARD_ID_GATE) {
                card_access_t* pa = card_accessCardSearch(cms_ctx_v.curAid, cms_ctx_v.curAidLen);
                if (pa) {
                    pa->selected = 0;
                }
            } else {
                card_transit_t* pt = card_transitCardSearch(cms_ctx_v.curAid, cms_ctx_v.curAidLen);
                if (pt) {
                    pt->selected = 0;
                }
            }
        } else {
            LOG_WARN("[%s] deactivate status: %d.\r\n", __FUNCTION__, status);
        }
    }


    if (type != CARD_ID_GATE) {
        LOG_DEBUG("-------------Query Balance.--------------\r\n");
        *balance = query_balance(aid, aidLen);
        if (*balance < 0) {
            LOG_ERROR("[%s] Balance Get fail: %d\r\n", __FUNCTION__, balance);
            *balance = -1;
        } else {
            LOG_DEBUG("Balance: %d\r\n", *balance);
        }
    }


    LOG_DEBUG("-------------Activate.--------------\r\n");
    status = activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), aid, aidLen);
    if (status == 0) {
        LOG_DEBUG("[%s] activate success.\r\n");

        if (type == CARD_ID_GATE) {
            card_access_t* pa = card_accessCardSearch(aid, aidLen);
            if (pa) {
                pa->selected = 1;
            }
        } else {
            card_transit_t* pt = card_transitCardSearch(aid, aidLen);
            if (pt) {
                pt->selected = 1;
                pt->balance = *balance;
            }            
        }
        
    } else {
        LOG_ERROR("activate fail %d.\r\n", status);
        card_unselectAll();
        status = activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), aid, aidLen);
    }

    if (status == 0) {
        ry_memcpy(cms_ctx_v.curAid, aid, aidLen);
        cms_ctx_v.curAidLen = aidLen;
        cms_ctx_v.curType   = type;

        card_curCard_save();
    }

    return status;
}


void card_selectGate(u32_t card_id, card_access_t* pa, card_uiCb_t cb)
{
    u8_t status;
    u8_t *apdu;


    /* Check NFC state */
    if (ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_CE_ON, NULL);
        while(ry_nfc_get_state() != RY_NFC_STATE_CE_ON) {
            ryos_delay_ms(50);
        }
    }

    status = card_doSelectCard(card_id, pa->aid, pa->aidLen, NULL);

    if (status == CARD_SELECT_STATUS_OK) {
        pa->selected = 1;
    }

    if (cb) {
        cb(pa->color, status, (char*)pa->name, pa->aid, pa->aidLen, NULL);
    }
    return;

}





void card_onCreateAccessCard(uint8_t* data, int len)
{
    GateCreateParam msg;
    int status;
    ry_sts_t ry_sts;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    /* Check if we are in the low power mode */
    if (get_device_powerstate() == DEV_PWR_STATE_LOW_POWER || 
        get_device_powerstate() == DEV_PWR_STATE_OFF) {
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_LOW_POWER, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        LOG_ERROR("[%s] Low Power.\r\n", __FUNCTION__);
        return;
    }

    status = pb_decode(&stream, GateCreateParam_fields, &msg);

    //ry_data_dump(msg.aid.bytes, 10, ' ');
    //LOG_DEBUG("[Bind Ack] status: %d\n", msg.error);
    //LOG_DEBUG("[Bind Ack] uid: %s\n", device_info.uid);

    /* Send response */
    //rbp_send_resp(CMD_DEV_GATE_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_sts = ble_send_response(CMD_DEV_GATE_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (ry_sts != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
        return;
    }


    if (cms_ctx_v.session == NULL) {
        cms_ctx_v.session = (card_ota_session_t*)ry_malloc(sizeof(card_ota_session_t));
        if (!cms_ctx_v.session) {
            LOG_ERROR("[card_onCreateAccessCard] No mem.\r\n");
            while(1);
        }
    }

    set_device_session(DEV_SESSION_CARD);

    ry_memset((u8_t*)cms_ctx_v.session, 0, sizeof(card_ota_session_t));
    card_session_timer_create();
    card_session_timer_start(CMS_SESSION_ACCESS_CARD_CHECK_TIMEOUT);
    
    ry_memcpy(cms_ctx_v.session->aid, msg.aid.bytes, msg.aid.size);
    cms_ctx_v.session->aidLen = msg.aid.size;
    cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_CREATE_ACCESS_ACK;
    LOG_DEBUG("[card_onCreateAccessCard] aidLen: %d\r\n", cms_ctx_v.session->aidLen);
    ry_data_dump(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen, ' ');

    cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_COPY, CARD_CREATE_EVT_CODE_START);

}

void card_onCreateAccessAck(uint8_t* data, int len)
{
    GateCreateAck msg;
    card_access_t* p;
    int status;
    ry_sts_t ret;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, GateCreateAck_fields, &msg);

    if (msg.code == 0) {
        // Success
        ret = card_accessCardAdd((char*)string_default_gate, 
                cms_ctx_v.session->aid, 
                cms_ctx_v.session->aidLen, 
                cms_ctx_v.session->uid, 
                0);

        if (ret != RY_SUCC) {
            LOG_ERROR("[card_onCreateAccessAck] Add access card to table fail. %04x\r\n", ret);
        }

        /* Notify UI */
        cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_COPY, CARD_CREATE_EVT_CODE_SUCC);

        /* Save cards to flash */
        card_accessCards_save();

        p = card_accessCardSearch(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen);
        
    } else {
        // Fail
        LOG_DEBUG("[card_onCreateAccessAck] ack fail:%d\r\n", msg.code);
        cms_msg_broadcast(CARD_ID_GATE, CARD_CREATE_EVT_STATE_COPY, CARD_CREATE_EVT_CODE_FAIL);
        
    }

    /* Release the session */
    card_session_delete();


    if (msg.code == 0) {   
        /* Set default card */
        if (p) {
            cms_ctx_v.defaultType = CARD_ID_GATE;
            cms_ctx_v.default_ca  = p;

            card_selectGate(CARD_ID_GATE, cms_ctx_v.default_ca, NULL);
        }
    }

    //ry_nfc_se_channel_close();

    /* Send response */
    //rbp_send_resp(CMD_DEV_GATE_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ret = ble_send_response(CMD_DEV_GATE_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (ret != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ret);
        return;
    }

    /* Recovery Battery task  */

    set_device_session(DEV_SESSION_IDLE);
}


void card_onGetAccessCardList(void)
{
    int i;
    int num = 0;
    GetGateInfoListResult * result = (GetGateInfoListResult *)ry_malloc(sizeof(GetGateInfoListResult)+100);
    if (!result) {
        LOG_ERROR("[card_onGetAccessCardList] No mem.\r\n");
        while(1);
    }

    /* Get data from database */
    ry_memset((u8_t*)result, 0, sizeof(GetGateInfoListResult)+100);
    result->card_infos_count = cms_ctx_v.accessCardTbl.curNum;

    for (i = 0; num < result->card_infos_count; i++) {
        if (is_aid_empty(cms_ctx_v.accessCardTbl.cards[i].aid)) {
            continue;
        }
        
        result->card_infos[num].aid.size = cms_ctx_v.accessCardTbl.cards[i].aidLen;
        if (result->card_infos[num].aid.size > MAX_CARD_AID_LEN) {
            LOG_ERROR("[%s] AID Length Error.\r\n", __FUNCTION__);
            ry_free((u8_t*)result);
            return;
        }
        
        ry_memcpy(result->card_infos[num].aid.bytes, 
            cms_ctx_v.accessCardTbl.cards[i].aid,
            cms_ctx_v.accessCardTbl.cards[i].aidLen);

        strcpy(result->card_infos[num].name, (char*)cms_ctx_v.accessCardTbl.cards[i].name);
        result->card_infos[num].has_type = 1;
        result->card_infos[num].type = (GateCardType)(cms_ctx_v.accessCardTbl.cards[i].color + 1);

        num++;
    }
    
    /* Send RBP response */
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(GetGateInfoListResult) + 100);
    if (!data_buf) {
        LOG_ERROR("[card_onGetAccessCardList] No mem.\r\n");
        while(1);
    }
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(GetGateInfoListResult) + 100);

    status = pb_encode(&stream, GetGateInfoListResult_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[card_onGetAccessCardList]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    //status = rbp_send_resp(CMD_DEV_GATE_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1);
    status = ble_send_response(CMD_DEV_GATE_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (status != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, status);
        goto exit;
    }

exit:
    ry_free(data_buf);
    ry_free((u8_t*)result);
}

void card_onGetWhiteCardList(void)
{
    int i;
    int num = 0;
    GetGateInfoListResult * result = (GetGateInfoListResult *)ry_malloc(sizeof(GetGateInfoListResult)+100);
    if (!result) {
        LOG_ERROR("[card_onGetWhiteCardList] No mem.\r\n");
        while(1);
    }

    /* Get data from database */
    ry_memset((u8_t*)result, 0, sizeof(GetGateInfoListResult)+100);
    result->card_infos_count = 0;
    
    /* Send RBP response */
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(GetGateInfoListResult) + 100);
    if (!data_buf) {
        LOG_ERROR("[card_onGetWhiteCardList] No mem.\r\n");
        while(1);
    }
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(GetGateInfoListResult) + 100);

    status = pb_encode(&stream, GetGateInfoListResult_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[card_onGetWhiteCardList]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    //status = rbp_send_resp(CMD_DEV_GATE_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1);
    status = ble_send_response(CMD_DEV_WHITECARD_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (status != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, status);
        goto exit;
    }

exit:
    ry_free(data_buf);
    ry_free((u8_t*)result);
}

//card_onWhiteCardAidSync
void card_onWhiteCardAidSync(void)
{
    int i;
    int num = 0;
    GetGateInfoListResult * result = (GetGateInfoListResult *)ry_malloc(sizeof(GetGateInfoListResult)+100);
    if (!result) {
        LOG_ERROR("[card_onGetWhiteCardList] No mem.\r\n");
        while(1);
    }

    /* Get data from database */
    ry_memset((u8_t*)result, 0, sizeof(GetGateInfoListResult)+100);
    result->card_infos_count = 0;
    
    /* Send RBP response */
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(GetGateInfoListResult) + 100);
    if (!data_buf) {
        LOG_ERROR("[card_onGetWhiteCardList] No mem.\r\n");
        while(1);
    }
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(GetGateInfoListResult) + 100);

    status = pb_encode(&stream, GetGateInfoListResult_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[card_onGetWhiteCardList]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    //status = rbp_send_resp(CMD_DEV_GATE_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1);
    status = ble_send_response(CMD_DEV_WHITECARD_AID_SYNC, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (status != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, status);
        goto exit;
    }

exit:
    ry_free(data_buf);
    ry_free((u8_t*)result);
}


void card_onGetTransitCardList(void)
{
#if 1
    int i;
    int num = 0;
    
    char cityId[10];
    char cityName[30];
    nfc_setting_t para;
    ry_sts_t ry_sts;
    
    TransitList * result = (TransitList *)ry_malloc(sizeof(TransitList)+100);
    if (!result) {
        LOG_ERROR("[card_onGetTransitCardList] No mem.\r\n");
        while(1);
    }

    /* Get data from database */
    ry_memset((u8_t*)result, 0, sizeof(TransitList)+100);
    result->infos_count = cms_ctx_v.transitCardTbl.curNum;

    for (i = 0; num < result->infos_count; i++) {
        if (is_aid_empty(cms_ctx_v.transitCardTbl.cards[i].aid)) {
            continue;
        }
        
        result->infos[num].aid.size = cms_ctx_v.transitCardTbl.cards[i].aidLen;
        if (result->infos[num].aid.size > MAX_CARD_AID_LEN) {
            LOG_ERROR("[%s] AID Length Error.\r\n", __FUNCTION__);
            ry_free((u8_t*)result);
            return;
        }
        
        ry_memcpy(result->infos[num].aid.bytes, 
            cms_ctx_v.transitCardTbl.cards[i].aid,
            cms_ctx_v.transitCardTbl.cards[i].aidLen);
        result->infos[num].aid.size = cms_ctx_v.transitCardTbl.cards[i].aidLen;

        ry_sts = card_additionalInfo_get(cms_ctx_v.transitCardTbl.cards[i].aid,
                    cms_ctx_v.transitCardTbl.cards[i].aidLen,
                    cityId, cityName, &para
                    );
        if (ry_sts == RY_SUCC) {
            strcpy(result->infos[num].use_city_id, (char*)cityId);
            if ((cityName[0] == 0xFF && cityName[1] == 0xFF) ||
                (cityName[0] == 0 && cityName[1] == 0)) {
                if (cms_ctx_v.transitCardTbl.cards[i].type == CARD_ID_JJJ) {
                    strcpy(cityName, "北京");
                }
            }
            strcpy(result->infos[num].use_city_name, cityName);
            ry_memcpy(result->infos[num].nfc_setting.bytes, (u8_t*)&para, sizeof(nfc_setting_t));
            result->infos[num].nfc_setting.size = sizeof(nfc_setting_t);
        } else {
            /* Use default setting  */
            result->infos[num].use_city_id[0]   = 0;
            result->infos[num].use_city_name[0] = 0;
            ry_memcpy(result->infos[num].nfc_setting.bytes, (u8_t*)&default_nfc_para, sizeof(nfc_setting_t));
            result->infos[num].nfc_setting.size = sizeof(nfc_setting_t);
        }

        num++;
    }
    
    /* Send RBP response */
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(TransitList) + 100);
    if (!data_buf) {
        LOG_ERROR("[card_onGetTransitCardList] No mem.\r\n");
        while(1);
    }
    
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(TransitList) + 100);

    status = pb_encode(&stream, TransitList_fields, result);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[card_onGetTransitCardList]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    status = ble_send_response(CMD_DEV_TRANSIT_GET_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                cms_ble_send_callback, (void*)__FUNCTION__);
    if (status != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, status);
        goto exit;
    }

exit:
    ry_free(data_buf);
    ry_free((u8_t*)result);

#endif
}


void card_onSetTransitCardPara(uint8_t* data, int len)
{
#if 1
    TransitSetUseCityParam msg;
    int status;
    ry_sts_t ry_sts;
    card_transit_t *p;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    int nameLen;
    char cityId[10];
    char cityName[30];
    nfc_setting_t para;

    status = pb_decode(&stream, TransitSetUseCityParam_fields, &msg);

    LOG_DEBUG("card_onSetTransitCardPara\r\n");
    ry_data_dump(msg.aid.bytes, msg.aid.size, ' ');


    /* Send response */
    p = card_transitCardSearch(msg.aid.bytes, msg.aid.size);
    if (p == NULL) {
        // Error Code:3, Not found
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_SET_USE_CITY, RBP_RESP_CODE_NOT_FOUND, NULL, 0, 1,
                    cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
    } else {
        
        ry_sts = card_additionalInfo_get(msg.aid.bytes, msg.aid.size, cityId, cityName, &para);
        if (ry_sts == RY_SUCC) {
            card_additionalInfo_update(msg.aid.bytes, 
                msg.aid.size, 
                msg.use_city_id, 
                msg.use_city_name, 
                (nfc_setting_t*)msg.nfc_setting.bytes);
        } else {
            card_additionalInfoAdd(msg.aid.bytes, 
                msg.aid.size, 
                msg.use_city_id, 
                msg.use_city_name, 
                (nfc_setting_t*)msg.nfc_setting.bytes);
        }

        ry_sts = ble_send_response(CMD_DEV_TRANSIT_SET_USE_CITY, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }

        LOG_DEBUG("[card_onSetTransitCardPara] Set name succ. %s\r\n", p->name);

    }
#endif
}



const char string_home[] = {0xE5, 0xAE, 0xB6, 0x00};
const char string_unit[] = {0xE5, 0x8D, 0x95, 0xE5, 0x85, 0x83, 0x00};
const char string_neighbor[] = {0xE5, 0xB0, 0x8F, 0xE5, 0x8C, 0xBA, 0x00};
const char string_company[] = {0xE5, 0x85, 0xAC, 0xE5, 0x8F, 0xB8, 0x00};
const char string_xiaomi[] = {0xE5, 0xB0, 0x8F, 0xE7, 0xB1, 0xB3, 0x00};

void card_onSetAccessCardInfo(uint8_t* data, int len)
{
    GateCardInfo msg;
    int status;
    ry_sts_t ry_sts;
    card_access_t *p;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    int nameLen;

    status = pb_decode(&stream, GateCardInfo_fields, &msg);

    LOG_DEBUG("card_onSetAccessCardInfo\r\n");
    ry_data_dump(msg.aid.bytes, msg.aid.size, ' ');

    /* Send response */
    p = card_accessCardSearch(msg.aid.bytes, msg.aid.size);
    if (p == NULL) {
        // Error Code:3, Not found
        //rbp_send_resp(CMD_DEV_GATE_SET_INFO , 3, NULL, 0, 1); 
        ry_sts = ble_send_response(CMD_DEV_GATE_SET_INFO, RBP_RESP_CODE_NOT_FOUND, NULL, 0, 1,
                    cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
    } else {
        nameLen = strlen(msg.name);
        if (nameLen > MAX_CARD_NAME) {
            // Error Code:4, Name len invalid
            //rbp_send_resp(CMD_DEV_GATE_SET_INFO, 4, NULL, 0, 1);
            ry_sts = ble_send_response(CMD_DEV_GATE_SET_INFO, RBP_RESP_CODE_INVALID_PARA, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
            if (ry_sts != RY_SUCC) {

                LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                return;
            }
        } else {
            strcpy((char*)p->name, msg.name);

            if (msg.has_type) {
                p->color = msg.type - 1;
                if (strcmp(msg.name, (char*)string_xiaomi) == 0) {
                    p->color = CARD_ACCESS_COLOR_XIAOMI;
                }
            } else {

                if (strcmp(msg.name, (char*)string_home) == 0) {
                    p->color = CARD_ACCESS_COLOR_HOME;
                } else if (strcmp(msg.name, (char*)string_unit) == 0) {
                    p->color = CARD_ACCESS_COLOR_UNIT;
                } else if (strcmp(msg.name, (char*)string_neighbor) == 0) {
                    p->color = CARD_ACCESS_COLOR_NEIGHBOR;
                } else if (strcmp(msg.name, (char*)string_company) == 0) {
                    p->color = CARD_ACCESS_COLOR_COMPANY;
                } else if (strcmp(msg.name, (char*)string_xiaomi) == 0) {
                    p->color = CARD_ACCESS_COLOR_XIAOMI;
                } else {
                    p->color = CARD_ACCESS_COLOR_UNIT;
                }
            }

            //rbp_send_resp(CMD_DEV_GATE_SET_INFO, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
            ry_sts = ble_send_response(CMD_DEV_GATE_SET_INFO, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
            if (ry_sts != RY_SUCC) {

                LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                return;
            }
            card_accessCards_save();
            LOG_DEBUG("[card_onSetAccessCardInfo] Set name succ. %s\r\n", p->name);
        }
    }
}

void card_onStartDeleteAccessCard(uint8_t* data, int len)
{
    GateDeleteParam msg;
    card_access_t *p;
    int status;
    ry_sts_t ry_sts;
    ry_sts_t se_status;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    status = pb_decode(&stream, GateDeleteParam_fields, &msg);
    if (!status) {
        // Error, Decode fail.
        LOG_DEBUG("card_onStartDeleteAccessCard Decode fail."); 
        //rbp_send_resp(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
        return ;
    }

    /* Send response */
    p = card_accessCardSearch(msg.aid.bytes, msg.aid.size);
    if (p == NULL) {
        // Error Code:3, Not found
        //rbp_send_resp(CMD_DEV_GATE_START_DELETE , 3, NULL, 0, 1); 
        ry_sts = ble_send_response(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_NOT_FOUND, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
    } else {

        /* Start session to delete */
        if (cms_ctx_v.session) {
            // Error code :Busy
            //rbp_send_resp(CMD_DEV_GATE_START_DELETE, 5, NULL, 0, 1);
            ry_sts = ble_send_response(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_BUSY, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
            if (ry_sts != RY_SUCC) {

                LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                return;
            }
            return;
        }

        /* Check NFC state */
        if (ry_nfc_get_state() == RY_NFC_STATE_IDLE || ry_nfc_get_state() == RY_NFC_STATE_LOW_POWER) {
            ry_sched_addNfcEvtListener(card_onNfcEvent);
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
            cms_ctx_v.state = CMS_STATE_DELETE_CARD;
            return;
        }

        /* Open the SE wired channel */
        se_status = ry_nfc_se_channel_open();
        if (se_status) {
            LOG_DEBUG("[card_onStartDeleteAccessCard] Open SE channel fail. %x\r\n", se_status);

            ry_sts = ble_send_response(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_OPEN_FAIL, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
            if (ry_sts != RY_SUCC) {

                LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                return;
            }
            return;
        }

        set_device_session(DEV_SESSION_CARD);

        cms_ctx_v.session = (card_ota_session_t*)ry_malloc(sizeof(card_ota_session_t));
        if (!cms_ctx_v.session) {
            LOG_ERROR("[card_onStartDeleteAccessCard] No mem.\r\n");
            while(1);
        }

        ry_memset((u8_t*)cms_ctx_v.session, 0, sizeof(card_ota_session_t));
        cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_DELETE_ACK;
        cms_ctx_v.session->aidLen = msg.aid.size;
        ry_memcpy(cms_ctx_v.session->aid, msg.aid.bytes, msg.aid.size);
        card_session_timer_create();
        card_session_timer_start(CMS_SESSION_ACCESS_CARD_DELETE_TIMEOUT);


        cms_msg_broadcast(CARD_ID_GATE, CARD_DELETE_EVT_STATE_START, CARD_CREATE_EVT_CODE_START);

        /* Send response. */
        //rbp_send_resp(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_GATE_START_DELETE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
        LOG_DEBUG("[card_onStartDeleteAccessCard] Delete session started.\r\n");
    }
}

void card_onAccessCardDeleteAck(uint8_t* data, int len)
{
    GateDeleteAck msg;
    int status;
    ry_sts_t ret;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);

    if (cms_ctx_v.session == NULL) {
        LOG_DEBUG("[card_onAccessCardDeleteAck] Session Not Existed.\r\n"); 
        //rbp_send_resp(CMD_DEV_GATE_DELETE_ACK, 6, NULL, 0, 1);
        return;
    }

    if (cms_ctx_v.session->state != CMS_SESSION_STATE_WAIT_DELETE_ACK) {
        // Error, Invalid command
        LOG_DEBUG("[card_onAccessCardDeleteAck] Invalid Session State.\r\n"); 
        //rbp_send_resp(CMD_DEV_GATE_DELETE_ACK, 6, NULL, 0, 1);
        return ;
    }

    status = pb_decode(&stream, GateDeleteAck_fields, &msg);
    if (!status) {
        // Error, Decode fail.
        LOG_DEBUG("card_onAccessCardDeleteAck Decode fail.\r\n"); 
        rbp_send_resp(CMD_DEV_GATE_DELETE_ACK, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1);
        goto exit ;
    }


    if (msg.code == 0) {
        // Success, No need to send response.
        ret = card_accessCardDel(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen);
        if (ret != RY_SUCC) {
            LOG_ERROR("[card_onAccessCardDeleteAck] Delete card fail. %04x\r\n", ret);
            //rbp_send_resp(CMD_DEV_GATE_DELETE_ACK, ret, NULL, 0, 1);
        } else {
            //rbp_send_resp(CMD_DEV_GATE_DELETE_ACK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
        }

        /* Save card changes to flash */
        card_accessCards_save();

        /* Check if the deletected card is current one. */
        if (0 == ry_memcmp(cms_ctx_v.session->aid, cms_ctx_v.curAid, cms_ctx_v.session->aidLen)) {
            if (FALSE == card_select_default()) {
                //ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
                ry_nfc_sync_poweroff();
            }
        }

        cms_msg_broadcast(CARD_ID_GATE, CARD_DELETE_EVT_STATE_START, CARD_CREATE_EVT_CODE_SUCC);
        
    } else {
        // Delete Fail, Do nothing. No need to send response.
        LOG_DEBUG("[card_onCreateAccessAck] ack fail:%d\r\n", msg.code);
        //rbp_send_resp(CMD_DEV_GATE_DELETE_ACK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);

        cms_msg_broadcast(CARD_ID_GATE, CARD_DELETE_EVT_STATE_START, CARD_CREATE_EVT_CODE_FAIL);
    }


exit:
    card_session_timer_stop();
    ry_timer_delete(cms_ctx_v.session->timer);
    ry_free((u8_t*)cms_ctx_v.session);
    cms_ctx_v.session = NULL;

    set_device_session(DEV_SESSION_IDLE);

}


void card_onStartCreateTransitCard(uint8_t* data, int len)
{
    TransitCreateParam msg;
    int status;
    ry_sts_t ry_sts;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    card_transit_t *p;
    u8_t type;

    /* Check if we are in the low power mode */
    if (get_device_powerstate() == DEV_PWR_STATE_LOW_POWER || 
        get_device_powerstate() == DEV_PWR_STATE_OFF) {
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_LOW_POWER, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        LOG_ERROR("[%s] Low Power.\r\n", __FUNCTION__);
        return;
    }

    status = pb_decode(&stream, TransitCreateParam_fields, &msg);
    //if (!status) {
        // Error, Decode fail.
   //     LOG_DEBUG("card_onStartCreateTransitCard Decode fail."); 
   //     rbp_send_resp(CMD_DEV_BUS_START_CREATE, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1);
   //     return ;
    //}

    /* Send response */
    #if 0
    p = card_transitCardSearch(msg.aid.bytes, msg.aid.size);
    if (p != NULL) {
        // Error Code:3, Already have the card
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, 3, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
    } else {
    #endif

        /* Start session to create */
        if (cms_ctx_v.session) {
            // Error code :Busy
            //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, 5, NULL, 0, 1);
            ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_BUSY, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
            if (ry_sts != RY_SUCC) {

                LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
                return;
            }
            return;
        }

        set_device_session(DEV_SESSION_CARD);

        cms_ctx_v.session = (card_ota_session_t*)ry_malloc(sizeof(card_ota_session_t));
        if (!cms_ctx_v.session) {
            LOG_ERROR("[card_onStartCreateTransitCard] No mem.\r\n");
            while(1);
        }

        ry_memset((u8_t*)cms_ctx_v.session, 0, sizeof(card_ota_session_t));
        cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_CREATE_TRANSIT_ACK;
        cms_ctx_v.session->aidLen = msg.aid.size;
        ry_memcpy(cms_ctx_v.session->aid, msg.aid.bytes, msg.aid.size);
        

        type = card_getTypeByAID(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen);
        if (type == CARD_ID_JJJ) {
            strcpy(cms_ctx_v.session->name, "京津冀 ");
        } else if (type == CARD_ID_LNT) {
            
            if (msg.has_city_id) {
                if (atoi(msg.city_id) == 5201) {
                    /* 羊城通 */
                    type = CARD_ID_YCT;
                    strcpy(cms_ctx_v.session->name, "羊城通 ");
                } else {
                    strcpy(cms_ctx_v.session->name, "岭南通 ");
                }
            } else {
                strcpy(cms_ctx_v.session->name, "岭南通 ");
            }
        } else if (type == CARD_ID_BJT) {
            strcpy(cms_ctx_v.session->name, "北京通 ");
        } else if (type == CARD_ID_SZT) {
            strcpy(cms_ctx_v.session->name, "深圳通 ");
        } else if (type == CARD_ID_WHT) {
            strcpy(cms_ctx_v.session->name, "武汉通 ");
        } else if (type == CARD_ID_CQT) {
            strcpy(cms_ctx_v.session->name, "重庆畅通卡 ");
        } else if (type == CARD_ID_JST) {
            strcpy(cms_ctx_v.session->name, "江苏一卡通 ");
        } else if (type == CARD_ID_CAT) {
            strcpy(cms_ctx_v.session->name, "长安通 ");
        } else if (type == CARD_ID_HFT) {
            strcpy(cms_ctx_v.session->name, "合肥通 ");
        } else if (type == CARD_ID_GXT) {
            strcpy(cms_ctx_v.session->name, "广西一卡通 ");
        } else if (type == CARD_ID_JLT) {
            strcpy(cms_ctx_v.session->name, "吉林一卡通 ");
        } 
        cms_ctx_v.session->card_id = type;

        LOG_DEBUG("[card_onStartCreateTransitCard] name:%s\r\n", msg.name);
        LOG_DEBUG("[card_onStartCreateTransitCard] aid:");
        ry_data_dump(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen, ' ');
        
        card_session_timer_create();
        card_session_timer_start(CMS_SESSION_TRANSIT_CARD_TIMEOUT);

        /* Stop Battery task  */

        cms_msg_broadcast(type, CARD_CREATE_EVT_STATE_TRANSIT, CARD_CREATE_EVT_CODE_START);

        /* Check NFC state */
        if (ry_nfc_get_state() == RY_NFC_STATE_IDLE || ry_nfc_get_state() == RY_NFC_STATE_LOW_POWER) {
            ry_sched_addNfcEvtListener(card_onNfcEvent);
            ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
            cms_ctx_v.state = CMS_STATE_OTA_TRANSIT_CARD;
            return;
        }

        /* Send response. */
        //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
        LOG_DEBUG("[card_onStartCreateTransitCard] Create Transit session started.\r\n");
    //}

   
}


void card_onTransitCardCreateAck(uint8_t* data, int len)
{
    TransitCreateAck msg;
    int status;
    ry_sts_t ret;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    card_transit_t* p;

    if (cms_ctx_v.session == NULL) {
        LOG_DEBUG("card_onTransitCardCreateAck Session wrong."); 
        ble_send_response(CMD_DEV_TRANSIT_CREATE_ACK, RBP_RESP_CODE_EXE_FAIL, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        return;
    }

    if (cms_ctx_v.session->state != CMS_SESSION_STATE_WAIT_CREATE_TRANSIT_ACK) {
        // Error, Invalid command
        LOG_DEBUG("card_onTransitCardCreateAck Session status fail."); 
        ble_send_response(CMD_DEV_TRANSIT_CREATE_ACK, RBP_RESP_CODE_INVALID_STATE, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        return ;
    }

    status = pb_decode(&stream, TransitCreateAck_fields, &msg);
    //if (!status) {
        // Error, Decode fail.
    //    LOG_DEBUG("card_onAccessCardDeleteAck Decode fail."); 
    //    rbp_send_resp(CMD_DEV_BUS_CREATE_ACK, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1);
    //    goto exit ;
    //}


    if (msg.code == 0) {
        // Success
        ret = card_transitCardAdd(cms_ctx_v.session->name, 
                    cms_ctx_v.session->aid, 
                    cms_ctx_v.session->aidLen, 
                    cms_ctx_v.session->card_id);
        if (ret != RY_SUCC) {
            LOG_ERROR("[card_onTransitCardCreateAck] Add card fail. %04x\r\n", ret);
            ble_send_response(CMD_DEV_TRANSIT_CREATE_ACK, ret, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        } else {

            card_transitCards_save();
            //rbp_send_resp(CMD_DEV_BUS_CREATE_ACK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);

            p = card_transitCardSearch(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen);
            if (p) {
                card_selectCard(card_getTypeByAID(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen), p, NULL);
            }

            

            /* Send message to change UI. */
            cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_CREATE_EVT_STATE_TRANSIT, CARD_CREATE_EVT_CODE_SUCC);

            ble_send_response(CMD_DEV_TRANSIT_CREATE_ACK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        }
        
    } else {
        // Create Fail, Do nothing.
        LOG_DEBUG("[card_onCreateAccessAck] ack fail:%d\r\n", msg.code);
        ble_send_response(CMD_DEV_TRANSIT_CREATE_ACK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        if (cms_ctx_v.session) {
            cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_CREATE_EVT_STATE_TRANSIT, CARD_CREATE_EVT_CODE_FAIL);
        }
    }


exit:
    card_session_timer_stop();
    ry_timer_delete(cms_ctx_v.session->timer);
    ry_free((u8_t*)cms_ctx_v.session);
    cms_ctx_v.session = NULL;

    set_device_session(DEV_SESSION_IDLE);

    /* Recovery Battery task  */

}


void card_onTransitRechargeStart(uint8_t* data, int len)
{
    TransitRechargeParam msg;
    int status;
    ry_sts_t ry_sts;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    card_transit_t *p;
    u8_t type;
    card_transit_t *pt;

    /* Check if we are in the low power mode */
    if ((sys_get_battery() < CARD__CREATE_BATT_PERCENT_THRESH) || 
        get_device_powerstate() == DEV_PWR_STATE_OFF) {
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_LOW_POWER, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        LOG_ERROR("[%s] Low Power.\r\n", __FUNCTION__);
        return;
    }

    status = pb_decode(&stream, TransitRechargeParam_fields, &msg);


    /* Start session to recharge */
    if (cms_ctx_v.session) {
        // Error code :Busy
        //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, 5, NULL, 0, 1);
        ry_sts = ble_send_response(CMD_DEV_TRANSIT_RECHARGE_START, RBP_RESP_CODE_BUSY, NULL, 0, 1,
                        cms_ble_send_callback, (void*)__FUNCTION__);
        if (ry_sts != RY_SUCC) {

            LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
            return;
        }
        return;
    }

    set_device_session(DEV_SESSION_CARD);

    cms_ctx_v.session = (card_ota_session_t*)ry_malloc(sizeof(card_ota_session_t));
    if (!cms_ctx_v.session) {
        LOG_ERROR("[card_onTransitRechargeStart] No mem.\r\n");
        RY_ASSERT(0);
    }

    ry_memset((u8_t*)cms_ctx_v.session, 0, sizeof(card_ota_session_t));
    cms_ctx_v.session->state = CMS_SESSION_STATE_WAIT_RECHARGE_ACK;
    cms_ctx_v.session->aidLen = msg.aid.size;
    ry_memcpy(cms_ctx_v.session->aid, msg.aid.bytes, msg.aid.size);
    

    type = card_getTypeByAID(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen);
    if (type == CARD_ID_JJJ) {
        strcpy(cms_ctx_v.session->name, "京津冀 ");
    } else if (type == CARD_ID_LNT) {
        pt = card_transitCardSearch(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen);
        if (pt->type == CARD_ID_YCT) {
            strcpy(cms_ctx_v.session->name, "羊城通 ");
        } else {
            strcpy(cms_ctx_v.session->name, "岭南通 ");
        }
    } else if (type == CARD_ID_BJT) {
        strcpy(cms_ctx_v.session->name, "北京通 ");
    } else if (type == CARD_ID_SZT) {
        strcpy(cms_ctx_v.session->name, "深圳通 ");
    } 
    cms_ctx_v.session->card_id = type;

    LOG_DEBUG("[card_onTransitRechargeStart] aid:");
    ry_data_dump(cms_ctx_v.session->aid, cms_ctx_v.session->aidLen, ' ');
    
    card_session_timer_create();
    card_session_timer_start(CMS_SESSION_TRANSIT_RECHARGE_TIMEOUT);

    /* Stop Battery task  */
    cms_msg_broadcast(type, CARD_RECHARGE_EVT, CARD_CREATE_EVT_CODE_START);

    /* Check NFC state */
    if (ry_nfc_get_state() == RY_NFC_STATE_IDLE || ry_nfc_get_state() == RY_NFC_STATE_LOW_POWER) {
        ry_sched_addNfcEvtListener(card_onNfcEvent);
        ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_ON, NULL);
        cms_ctx_v.state = CMS_STATE_RECHARGE;
        return;
    }

    /* Send response. */
    //rbp_send_resp(CMD_DEV_TRANSIT_START_CREATE, RBP_RESP_CODE_SUCCESS, NULL, 0, 1);
    ry_sts = ble_send_response(CMD_DEV_TRANSIT_RECHARGE_START, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                    cms_ble_send_callback, (void*)__FUNCTION__);
    if (ry_sts != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ry_sts);
        return;
    }
    LOG_DEBUG("[card_onTransitRechargeStart] Recharge session started.\r\n");

}


void card_onTransitRechargeAck(uint8_t* data, int len)
{
    TransitRechargeAck msg;
    int status;
    ry_sts_t ret;
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)data, len);
    card_transit_t* p;

    LOG_DEBUG("card_onTransitRechargeAck\r\n");

    if (cms_ctx_v.session == NULL) {
        LOG_DEBUG("card_onTransitCardCreateAck Session wrong."); 
        ble_send_response(CMD_DEV_TRANSIT_RECHARGE_ACK, RBP_RESP_CODE_EXE_FAIL, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        return;
    }

    if (cms_ctx_v.session->state != CMS_SESSION_STATE_WAIT_RECHARGE_ACK) {
        // Error, Invalid command
        LOG_DEBUG("card_onTransitCardCreateAck Session status fail."); 
        ble_send_response(CMD_DEV_TRANSIT_RECHARGE_ACK, RBP_RESP_CODE_INVALID_STATE, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);
        return ;
    }

    status = pb_decode(&stream, TransitRechargeAck_fields, &msg);
    //if (!status) {
        // Error, Decode fail.
    //    LOG_DEBUG("card_onAccessCardDeleteAck Decode fail."); 
    //    rbp_send_resp(CMD_DEV_BUS_CREATE_ACK, RBP_RESP_CODE_DECODE_FAIL, NULL, 0, 1);
    //    goto exit ;
    //}


    if (msg.code == 0) {
        // Success
        /* Send message to change UI. */
        cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_RECHARGE_EVT, CARD_CREATE_EVT_CODE_SUCC);

    } else {
        // Create Fail, Do nothing.
        LOG_DEBUG("[card_onTransitRechargeAck] ack fail:%d\r\n", msg.code);
        
        if (cms_ctx_v.session) {
            cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_RECHARGE_EVT, CARD_CREATE_EVT_CODE_FAIL);
        }
    }

    ble_send_response(CMD_DEV_TRANSIT_RECHARGE_ACK, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            cms_ble_send_callback, (void*)__FUNCTION__);


exit:
    card_session_timer_stop();
    ry_timer_delete(cms_ctx_v.session->timer);
    ry_free((u8_t*)cms_ctx_v.session);
    cms_ctx_v.session = NULL;

    set_device_session(DEV_SESSION_IDLE);
    
}



void card_onBleDisconnected(void)
{   
    LOG_DEBUG("[card_onBleDisconnected]\r\n");
    if (cms_ctx_v.session) {
        if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_CREATE_TRANSIT_ACK) {
            cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_CREATE_EVT_STATE_TRANSIT, CARD_CREATE_EVT_CODE_FAIL);
        } else if (cms_ctx_v.session->state == CMS_SESSION_STATE_WAIT_RECHARGE_ACK) {
            cms_msg_broadcast(cms_ctx_v.session->card_id, CARD_RECHARGE_EVT, CARD_CREATE_EVT_CODE_FAIL);
        }
    } else {
        return;
    }
		
	card_session_timeout_handler(NULL);

    //ryos_mutex_lock(se_mutex);
	
    /* Recovery NFC state */
    //card_nfc_recover();

    //ryos_mutex_unlock(se_mutex);
	
    set_device_session(DEV_SESSION_IDLE);
}



int card_set_default_cb(u32_t card_id, u8_t status, char* name, u8_t *aid, u8_t aidLen, void* usrdata)
{
    if (get_hardwardVersion()>3) {
        if (card_is_current_disable_lowpower() == FALSE) {
            ry_nfc_sync_lowpower();
        }
    }
    return RY_SUCC;
}



ry_sts_t card_set_default(u8_t * data, u32_t size)
{
    ry_sts_t status = RY_SUCC;
    DefaultCardInfo* p_cfg = NULL;
    pb_istream_t stream;
    

    p_cfg = (DefaultCardInfo*)ry_malloc(sizeof(DefaultCardInfo));
    if(p_cfg == NULL)
    {
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_NO_MEM);
        goto exit;
    }

    stream = pb_istream_from_buffer(data, size);
    if (0 == pb_decode(&stream, DefaultCardInfo_fields, p_cfg)){
        LOG_ERROR("[%s]-{%s} %s\n", __FUNCTION__, ERR_STR_DECODE_FAIL, PB_GET_ERROR(&stream));
        status = RY_SET_STS_VAL(RY_CID_PB, RY_ERR_READ);
        goto exit;
    }

    //exec do something
    LOG_DEBUG("[%s] mode: %d\r\n", __FUNCTION__, p_cfg->mode);
    if (p_cfg->has_id) {
        LOG_DEBUG("[%s] id: \r\n", __FUNCTION__);
        ry_data_dump(p_cfg->id.bytes, p_cfg->id.size, ' ');
    }
    if (p_cfg->has_type) {
        LOG_DEBUG("[%s] type: %d\r\n", __FUNCTION__, p_cfg->type);
    }

    cms_ctx_v.defaultCard.mode = (default_card_mode_t)p_cfg->mode;
    if (p_cfg->mode == DefaultCardInfo_Mode_LAST_USE) {
        if (p_cfg->has_id || p_cfg->has_type) {
            goto exit;
        }
    }

    if (p_cfg->mode == DefaultCardInfo_Mode_SPECIFIC) {
        if (p_cfg->has_id == 0 || p_cfg->has_type == 0) {
            goto exit;
        }

        cms_ctx_v.defaultCard.type = p_cfg->type;
        ry_memset(cms_ctx_v.defaultCard.aid, 0, MAX_CARD_AID_LEN);
        ry_memcpy(cms_ctx_v.defaultCard.aid, p_cfg->id.bytes, p_cfg->id.size);
        cms_ctx_v.defaultCard.aidLen = p_cfg->id.size;
    }

    card_defaultCard_save();

    ble_send_response(CMD_DEV_CARD_SET_DEFAULT, RBP_RESP_CODE_SUCCESS, NULL, 0, 1,
                            NULL, NULL);
    


    /* Select the specified default card. */
    if (p_cfg->mode == DefaultCardInfo_Mode_SPECIFIC) {
        if (cms_ctx_v.defaultCard.type == DefaultCardInfo_CardType_GATE_CARD) {
            cms_cmd_selectGate_t para;
            para.card_id = CARD_ID_GATE;
            para.pa = card_accessCardSearch(cms_ctx_v.defaultCard.aid, cms_ctx_v.defaultCard.aidLen);
            para.cb = card_set_default_cb;
            if (para.pa != NULL) {
                cms_msg_send(CMS_CMD_SELECT_GATE_CARD, sizeof(cms_cmd_selectGate_t), (void*)&para);
            }
        } else if (cms_ctx_v.defaultCard.type == DefaultCardInfo_CardType_TRANSIT_CARD) {
            cms_cmd_selectTransit_t para;
            para.pt = card_transitCardSearch(cms_ctx_v.defaultCard.aid, cms_ctx_v.defaultCard.aidLen);
            para.cb = card_set_default_cb;
            if (para.pt != NULL) {
                para.card_id = para.pt->type;
                cms_msg_send(CMS_CMD_SELECT_TRANSIT_CARD, sizeof(cms_cmd_selectTransit_t), (void*)&para);
            }
        }
    }


    ry_free(p_cfg);
    return status;
    
exit:
    ble_send_response(CMD_DEV_CARD_SET_DEFAULT, RBP_RESP_CODE_INVALID_PARA, NULL, 0, 1,
                            NULL, NULL);
    
    ry_free(p_cfg);
    return status;
}


ry_sts_t card_get_default(u8_t * data, u32_t size)
{
    int i;
    int num = 0;
    DefaultCardInfo * card = (DefaultCardInfo *)ry_malloc(sizeof(DefaultCardInfo)+100);
    if (!card) {
        LOG_ERROR("[card_get_default] No mem.\r\n");
        RY_ASSERT(0);
    }

    /* Get data from database */
    ry_memset((u8_t*)card, 0, sizeof(DefaultCardInfo)+100);
    card->mode = (DefaultCardInfo_Mode)cms_ctx_v.defaultCard.mode;
    if (card->mode == DefaultCardInfo_Mode_SPECIFIC) {
        card->has_id = 1;
        ry_memcpy(card->id.bytes, cms_ctx_v.defaultCard.aid, cms_ctx_v.defaultCard.aidLen);
        card->id.size = cms_ctx_v.defaultCard.aidLen;
        card->has_type = true;
        card->type = (DefaultCardInfo_CardType)cms_ctx_v.defaultCard.type;
    }
    
    /* Send RBP response */
    size_t message_length;
    ry_sts_t status;
    u8_t * data_buf = (u8_t *)ry_malloc(sizeof(DefaultCardInfo) + 100);
    if (!data_buf) {
        LOG_ERROR("[card_get_default] No mem.\r\n");
        RY_ASSERT(0);
    }

    ry_memset((u8_t*)data_buf, 0, sizeof(DefaultCardInfo)+100);
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, sizeof(DefaultCardInfo) + 100);

    status = pb_encode(&stream, DefaultCardInfo_fields, card);
    message_length = stream.bytes_written;
    
    if (!status) {
        LOG_ERROR("[card_get_default]: Encoding failed: %s\n", PB_GET_ERROR(&stream));
        goto exit;
    }

    //status = rbp_send_resp(CMD_DEV_GATE_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1);
    status = ble_send_response(CMD_DEV_GATE_GET_INFO_LIST, RBP_RESP_CODE_SUCCESS, data_buf, message_length, 1,
                NULL, NULL);
    if (status != RY_SUCC) {

        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, status);
        goto exit;
    }

exit:
    ry_free(data_buf);
    ry_free((u8_t*)card);

    return status;
}


bool card_check_is_default(void)
{
    return (0==ry_memcmp(cms_ctx_v.curAid, cms_ctx_v.defaultCard.aid, cms_ctx_v.curAidLen));
}


default_card_mode_t card_get_defalut_mode(void)
{
    return cms_ctx_v.defaultCard.mode;
}

void cms_select_default_card(void)
{
    LOG_INFO("[%s] Select default card. \r\n", __FUNCTION__);
    
    /* Power on NFC first */
    if (ry_nfc_get_state() == RY_NFC_STATE_IDLE || ry_nfc_get_state() == RY_NFC_STATE_LOW_POWER) {
        ry_nfc_sync_poweron();
    }

    /* Select default card */
    if (TRUE == card_select_default()) {
        /* Enter low power mode */
        if (get_hardwardVersion() > 3) {
            ryos_delay_ms(2000);
            if (get_hardwardVersion()>3) {
                ry_nfc_sync_lowpower();
            }
        }
        return;
    }

    /* Power off the NFC when select fail. */
    ryos_delay_ms(2000);
    ry_nfc_sync_poweroff();
}

u8_t card_get_current_card(void)
{
    return card_getTypeByAID(cms_ctx_v.curAid, cms_ctx_v.curAidLen);
}


bool card_is_current_disable_lowpower(void)
{
    ry_sts_t status;
    char cityId[MAX_CITY_ID_LEN];
    char cityName[MAX_CITY_NAME_LEN];
    nfc_setting_t para;
    
    status = card_additionalInfo_get(cms_ctx_v.curAid, cms_ctx_v.curAidLen,
                cityId, cityName, &para); 
    
    if (status == RY_SUCC && para.disable_lowpower == 1) {
        return TRUE;
    }

    return FALSE;
}




