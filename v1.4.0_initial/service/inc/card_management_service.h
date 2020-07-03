/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __CMS_H__
#define __CMS_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"
#include "scheduler.h"
#include "ry_list.h"

/*
 * CONSTANTS
 *******************************************************************************
 */

#define MAX_CARD_AID_LEN                     16
#define MAX_CARD_NAME                        20
#define MAX_CARD_UID_LEN                     4
#define MAX_CITY_NAME_LEN                    20
#define MAX_CITY_ID_LEN                      10


#define MAX_ACCESS_CARD_NUM                  5
#define MAX_TRANSIT_CARD_NUM                 5
#define MAX_CARD_NUM                         (MAX_ACCESS_CARD_NUM + MAX_TRANSIT_CARD_NUM)

#define SUPPORTED_CARD_NUM                   14


#define CARD_TYPE_TRANSIT                    1
#define CARD_TYPE_BANK                       2
#define CARD_TYPE_ACCESS                     3


#define CARD_ID_XIAOMI                       0
#define CARD_ID_BJT                          1
#define CARD_ID_SZT                          2
#define CARD_ID_LNT                          3
#define CARD_ID_SHT                          4
#define CARD_ID_GATE                         5
#define CARD_ID_JJJ                          6
#define CARD_ID_WHT                          7
#define CARD_ID_CQT                          8
#define CARD_ID_JST                          9
#define CARD_ID_YCT                          (0x80|CARD_ID_LNT)
#define CARD_ID_CAT                          11  // 长安通
#define CARD_ID_HFT                          12  // 合肥通
#define CARD_ID_GXT                          13  // 广西通
#define CARD_ID_JLT                          14  // 吉林通
#define CARD_ID_HEB                          15  // 哈尔滨
#define CARD_ID_LCT                          16  // 绿城通
#define CARD_ID_QDT                          17  // 琴岛通

#define CARD_ID_TRANSIT                      8
#define CARD_ID_INVALID                      0xFF


#define CARD_CREATE_EVT_STATE_DETECT         1
#define CARD_CREATE_EVT_STATE_COPY           2
#define CARD_CREATE_EVT_STATE_USER_SETTING   3
#define CARD_CREATE_EVT_STATE_TRANSIT        4
#define CARD_DELETE_EVT_STATE_START          5
#define CARD_RECHARGE_EVT                    6


#define CARD_CREATE_EVT_CODE_START           1
#define CARD_CREATE_EVT_CODE_SUCC            2
#define CARD_CREATE_EVT_CODE_FAIL            3


#define FLASH_ADDR_ACCESS_CARDS              0xFF000
#define FLASH_ADDR_TRANSIT_CARDS             0xFF800
#define FLASH_ADDR_CUR_CARD                  (FLASH_ADDR_ACCESS_CARDS +sizeof(accessCard_tbl_t))
#define FLASH_ADDR_DEFAULT_CARD              0xFF400
#define FLASH_ADDR_CARD_ADDITIONAL_INFO      0xFE500


#define CARD_XIAOMI_AID                      


#define CARD_SELECT_STATUS_OK                0
#define CARD_SELECT_STATUS_FAIL              1
#define CARD_SELECT_STATUS_NO_MEM            2
#define CARD_SELECT_STATUS_SE_FAIL           3
#define CARD_SELECT_STATUS_NOT_FOUND         9
#define CARD_SELECT_STATUS_TIMEOUT           10


#define CARD_ACCESS_COLOR_HOME               0
#define CARD_ACCESS_COLOR_UNIT               1
#define CARD_ACCESS_COLOR_NEIGHBOR           2
#define CARD_ACCESS_COLOR_COMPANY            3
#define CARD_ACCESS_COLOR_XIAOMI             4


#define CMS_CMD_MONITOR                      1
#define CMS_CMD_SELECT_TRANSIT_CARD          2
#define CMS_CMD_SELECT_GATE_CARD             3
#define CMS_CMD_SELECT_DEFAULT_CARD          4



/**
 * LED definition
 */
typedef enum {
    CMS_DEFAULT_CARD_MODE_INVALID = 0,
    CMS_DEFAULT_CARD_MODE_LAST_USE,
    CMS_DEFAULT_CARD_MODE_SPECIFIC,
    CMS_DEFAULT_CARD_MODE_MAX
} default_card_mode_t;


/*
 * TYPES
 *******************************************************************************
 */

typedef struct {
    u8_t card_id;
    u8_t ana_tx_amplitude[2]; // FF FF/ F6 F6/ F6 F1
    u8_t tx_control_mode;     // mode1:0x28  mode2: 0x08 mode3: 0x48
    u8_t clock_config_dll[2]; // E1 00 / 2D 00 / B4 00
} rf_para_t;


typedef struct {
    u8_t version;             // version = 1;
    u8_t ana_tx_amplitude[2]; // FF FF/ F6 F6/ F6 F1
    u8_t tx_control_mode;     // mode1:0x28  mode2: 0x08 mode3: 0x48
    u8_t clock_config_dll[2]; // E1 00 / 2D 00 / B4 00
    u8_t disable_lowpower;
    u8_t reserved[9];
} nfc_setting_t;


/*
 * @brief Access Card descriptor
 */
typedef struct {
    u8_t  aid[MAX_CARD_AID_LEN];
    int   aidLen;
    u8_t  uid[MAX_CARD_UID_LEN];
    u8_t  name[MAX_CARD_NAME];
    u32_t color;
    u8_t  selected;
    u8_t  type;
} card_access_t;

/*
 * @brief Transit Card descriptor
 */
typedef struct {
    u8_t aid[MAX_CARD_AID_LEN];
    int  aidLen;
    u8_t name[MAX_CARD_NAME];
    u8_t selected;
    u8_t type;
    int  balance;
} card_transit_t;

/*
 * @brief Access Card Table
 */
typedef struct {
    u32_t curNum;
    card_access_t cards[MAX_ACCESS_CARD_NUM];
} accessCard_tbl_t;

/*
 * @brief Transit Card Table
 */
typedef struct {
    u32_t curNum;
    card_transit_t cards[MAX_TRANSIT_CARD_NUM];
} transitCard_tbl_t;




/*
 * @brief Card create type definition, used in IPC message parameter
 */
typedef struct {
    u8_t cardType;
    u8_t state;
    u8_t code; // Success or error code.
    
} card_create_evt_t;


typedef struct {
    u8_t type;
    u8_t aid[MAX_CARD_AID_LEN];
    int  aidLen;
    char name[MAX_CARD_NAME];
} support_card_t;

typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t  data[1];
} cms_msg_t;


/*
 * @brief Card operation for UI.
 */
typedef int (* card_uiCb_t)(u32_t card_id, u8_t status, char* name, u8_t *aid, u8_t aidLen, void* usrdata);


typedef struct {
    u32_t card_id;
    card_access_t* pa;
    card_uiCb_t cb;
} cms_cmd_selectGate_t;


typedef struct {
    u32_t card_id;
    card_transit_t* pt;
    card_uiCb_t cb;
} cms_cmd_selectTransit_t;


typedef struct {
    default_card_mode_t mode;
    u8_t type;
    u8_t aid[MAX_CARD_AID_LEN];
    int  aidLen;
    char name[MAX_CARD_NAME];
} cms_default_card_t;

typedef struct {
    u32_t state;
    u8_t aid[MAX_CARD_AID_LEN];
    int  aidLen;
    u8_t uid[MAX_CARD_UID_LEN];
    char name[MAX_CARD_NAME];
    int  timer;
    card_uiCb_t ui_callback;
    u32_t card_id;
    void* pCard;
} card_ota_session_t;


typedef struct {
    u8_t                aid[MAX_CARD_AID_LEN];
    int                 aidLen;
    char                cityId[MAX_CITY_ID_LEN];
    char                cityName[MAX_CITY_NAME_LEN];
    nfc_setting_t       para;
    u8_t                reserved[20];
} card_para_t;


/*
 * @brief Access Card Table
 */
typedef struct {
    u32_t       curNum;
    card_para_t additionalInfos[MAX_CARD_NUM];
} cardInfo_tbl_t;


/*
 * @brief Card Service Context
 */
typedef struct {
    u32_t               state;
    u8_t                isSync;

    accessCard_tbl_t    accessCardTbl;
    transitCard_tbl_t   transitCardTbl;

    u8_t                curType;
    u8_t                curAid[MAX_CARD_AID_LEN];
    u8_t                curAidLen;
    card_para_t         curCardInfo;

    card_ota_session_t* session;
    u8_t                defaultType;
    card_access_t*      default_ca;
    card_transit_t*     default_pa;

    int                 selectTimer;
    cms_default_card_t  defaultCard;
    cardInfo_tbl_t      *pAdditionalInfoTbl;
    
} cms_ctx_t;


/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to init card manager service
 *
 * @param   None
 *
 * @return  None
 */
void card_service_init(void);


void card_onStartAccessCheck(void);
void card_onCreateAccessCard(uint8_t* data, int len);
void card_onCreateAccessAck(uint8_t* data, int len);
void card_onGetAccessCardList(void);
void card_onSetAccessCardInfo(uint8_t* data, int len);

void card_onGetWhiteCardList(void);
void card_onWhiteCardAidSync(void);

card_transit_t* card_getTransitCardByIdx(int idx);
card_access_t*  card_getAccessCardByIdx(int idx);
int card_getTransitCardNum(void);
int card_getAccessCardNum(void);
ry_sts_t card_accessCards_reset(void);
ry_sts_t card_transitCards_reset(void);




void cms_msg_broadcast(u8_t cardType, u8_t state, u8_t code);

void card_onSEOpen(void);
void card_onSEClose(void);

u8_t card_is_synced(void);


transitCard_tbl_t* card_getTransitList(void);
accessCard_tbl_t* card_getAccessList(void);

bool is_aid_empty(u8_t *aid);
void card_onBleDisconnected(void);

void card_selectGate(u32_t card_id, card_access_t* pa, card_uiCb_t cb);
void card_selectCard(u32_t card_id, card_transit_t* pa, card_uiCb_t cb);
void card_nfc_recover(void);


void card_onTransitRechargeStart(uint8_t* data, int len);
void card_onTransitRechargeAck(uint8_t* data, int len);

void cms_msg_send(u32_t msg_type, int len, void* data);
bool card_check_is_default(void);
default_card_mode_t card_get_defalut_mode(void);
void cms_select_default_card(void);
void card_service_sync(void);
ry_sts_t card_set_default(u8_t * data, u32_t size);
ry_sts_t card_get_default(u8_t * data, u32_t size);
u8_t card_get_current_card(void);
void card_onStartDeleteAccessCard(uint8_t* data, int len);
void card_onAccessCardDeleteAck(uint8_t* data, int len);
void card_onStartCreateTransitCard(uint8_t* data, int len);
void card_onTransitCardCreateAck(uint8_t* data, int len);
void card_onGetTransitCardList(void);
void card_onSetTransitCardPara(uint8_t* data, int len);
bool card_is_current_disable_lowpower(void);



#endif  /* __CMS_H__ */



