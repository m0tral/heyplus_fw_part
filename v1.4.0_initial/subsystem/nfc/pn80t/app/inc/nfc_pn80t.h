/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __NFC_PN80T_H__
#define __NFC_PN80T_H__

#include "ry_type.h"


// APDU SELECT Matcher applet
const static u8_t SelectMatcher[] = {0x00, 0xA4, 0x04, 0x00, 0x10, 0xA0, 0x00, 0x00, 0x03, 0x96, 0x66, 0x70, 0x4D, 0x41, 0x54, 0x43, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00};

// APDUs for CVM management
const static u8_t SelectCRS[] = {0x00, 0xA4, 0x04, 0x00, 0x09, 0xA0, 0x00, 0x00, 0x01, 0x51, 0x43, 0x52, 0x53, 0x00, 0x00};
const static u8_t ActivateDefaultAID[] = {0x80, 0xC3, 0x01, 0x01, 0x00};
const static u8_t DeativateDefaultAID[] = {0x80, 0xC3, 0x01, 0x00, 0x00};
const static u8_t ClearCVM[] = {0x80, 0xC3, 0x02, 0x00, 0x00};
const static u8_t PBOCAID[] = {0x80, 0xF0, 0x01, 0x01, 0x09, 0x4F, 0x07, 0xA0, 0x00, 0x00, 0x03, 0x33, 0x01, 0x01};
const static u8_t GETCPLC[] = {0x80, 0xCA, 0x9F, 0x7F, 0x00};
const static u8_t SelectSZT[] = {0x00, 0xA4, 0x04, 0x00, 0x0E, 0x53, 0x5A, 0x54, 0x2E, 0x57, 0x41, 0x4C, 0x4C, 0x45, 0x54, 0x2E, 0x45, 0x4E, 0x56};

const static u8_t AID_SZT[] = {0x53, 0x5A, 0x54, 0x2E, 0x57, 0x41, 0x4C, 0x4C, 0x45, 0x54, 0x2E, 0x45, 0x4E, 0x56}; // len = 14
const static u8_t AID_LNT[] = {0x59, 0x43, 0x54, 0x2E, 0x55, 0x53, 0x45, 0x52}; // len = 8
const static u8_t AID_BJT[] = {0x91, 0x56, 0x00, 0x00, 0x14, 0x01, 0x00, 0x01}; // len = 8
const static u8_t AID_JJJ[] = {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x10, 0x00, 0x91, 0x56, 0x00, 0x00, 0x14, 0xA1}; // len = 16
const static u8_t AID_WHT[] = {0xA0, 0x00, 0x00, 0x53, 0x42, 0x57, 0x48, 0x54, 0x4B}; // len = 9
const static u8_t AID_CQT[] = {0x43, 0x51, 0x51, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x33, 0x31}; // len = 12
const static u8_t AID_JST[] = {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x21, 0x50, 0x53, 0x55, 0x5A, 0x48, 0x4F, 0x55}; // len = 16
const static u8_t AID_CAT[] = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x71, 0x00, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}; // len = 16
const static u8_t AID_HFT[] = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x23, 0x00, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}; // len = 16
const static u8_t AID_GXT[] = {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x53, 0x00, 0x47, 0x55, 0x41, 0x4E, 0x47, 0x58}; // len = 16
const static u8_t AID_JLT[] = {0xA0, 0x00, 0x00, 0x06, 0x32, 0x01, 0x01, 0x05, 0x13, 0x20, 0x00, 0x4A, 0x49, 0x4C, 0x49, 0x4E}; // len = 16
const static u8_t AID_HEB[] = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x15, 0x00, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}; // len = 16
const static u8_t AID_LCT[] = {0xA0, 0x00, 0x00, 0x53, 0x42, 0x5A, 0x5A, 0x48, 0x54}; // len = 9
const static u8_t AID_QDT[] = {0xA0, 0x00, 0x00, 0x00, 0x03, 0x26, 0x60, 0x86, 0x98, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}; // len = 16

const static u8_t AID_ISD[] = {0xa0, 0x00, 0x01, 0x51, 0x00, 0x00, 0x00};


const static u8_t PREFIX_ACTIVATE[] = {0x80, 0xF0, 0x01, 0x01};
const static u8_t PREFIX_DEACTIVATE[] = {0x80, 0xF0, 0x01, 0x00};

const static u8_t QUERY_BALANCE[] = {0x80, 0x5c, 0x00, 0x02, 0x04};
const static u8_t SELECT_FOLDER[] = {0x00, 0xa4, 0x00, 0x00, 0x02, 0x10, 0x01};
const static u8_t SELECT_FOLDER_LNT1[] = {0x00, 0xa4, 0x00, 0x00, 0x02, 0xdd, 0xf1};
const static u8_t SELECT_FOLDER_LNT2[] = {0x00, 0xb0, 0x95, 0x00, 0x00};
const static u8_t SELECT_FOLDER_LNT3[] = {0x00, 0xa4, 0x00, 0x00, 0x02, 0xad, 0xf3};
const static u8_t SELECT_FOLDER_LNT4[] = {0x00, 0xa4, 0x04, 0x00, 0x08, 0x50, 0x41, 0x59, 0x2e, 0x41, 0x50, 0x50, 0x59};
const static u8_t QUERY_CITY[] = {0x00, 0xb0, 0x95, 0x31, 0x01};
const static u8_t SELECT_FOLDER_CQT1[] = {0x00, 0xa4, 0x00, 0x00, 0x02, 0x3f, 0x00};
const static u8_t SELECT_FOLDER_CQT2[] = {0x00, 0xa4, 0x00, 0x00, 0x02, 0x3f, 0x01};



#define FLASH_ADDR_NFC_PARA_RF1           0xFE000
#define FLASH_ADDR_NFC_PARA_RF2           0xFE100
#define FLASH_ADDR_NFC_PARA_RF3           0xFE200
#define FLASH_ADDR_NFC_PARA_RF4           0xFE300
#define FLASH_ADDR_NFC_PARA_RF5           0xFE400
#define FLASH_ADDR_NFC_PARA_RF6           0xFE500
#define FLASH_ADDR_NFC_PARA_TVDD1         0xFE600
#define FLASH_ADDR_NFC_PARA_TVDD2         0xFE700
#define FLASH_ADDR_NFC_PARA_TVDD3         0xFE800
#define FLASH_ADDR_NFC_PARA_NXP_CORE      0xFE900
#define FLASH_ADDR_NFC_PARA_BLOCK         0xFEA00


#define NFC_PARA_TYPE_RF1                 1
#define NFC_PARA_TYPE_RF2                 2
#define NFC_PARA_TYPE_RF3                 3
#define NFC_PARA_TYPE_RF4                 4
#define NFC_PARA_TYPE_RF5                 5
#define NFC_PARA_TYPE_RF6                 6
#define NFC_PARA_TYPE_TVDD1               7
#define NFC_PARA_TYPE_TVDD2               8
#define NFC_PARA_TYPE_TVDD3               9
#define NFC_PARA_TYPE_NXP_CORE            10
#define NFC_PARA_TYPE_PARA                11



#if 0
typedef struct {
    u8_t card_id;
    u8_t ana_tx_amplitude[2]; // FF FF/ F6 F6/ F6 F1
    u8_t tx_control_mode;     // mode1:0x28  mode2: 0x08 mode3: 0x48
    u8_t clock_config_dll[2]; // E1 00 / 2D 00 / B4 00
} nfc_para_t;

typedef struct {
    u32_t curNum;
    nfc_para_t paras[1];
} nfc_para_tbl_t;

typedef struct {
    int version;
    nfc_para_tbl_t nfcParaTbl;
} nfc_para_ctx_v;
#endif


/*
 * TYPES
 *******************************************************************************
 */

#define NFC_STACK_EVT_INIT_DONE                0x0001
#define NFC_STACK_EVT_READER_DETECT_CARD       0x0002
#define NFC_STACK_EVT_CE                       0x0003


/**
 * @brief Definitaion for ble stack
 */
typedef void (*nfc_stack_cb_t)(u16_t evt, void* para);


extern u8_t nfc_i2c_instance;
extern u32_t nfc_irq_time;

typedef struct {
    u8_t  buf[300];
    u8_t  inuse;
    int   len;
} nfc_i2c_buf_t;

extern nfc_i2c_buf_t nfc_bufs[8];
extern int nfc_buf_rd;
extern int nfc_buf_wr;



/*
 * FUNCTIONS
 *******************************************************************************
 */


/**
 * @brief   API to init NFC module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t nfc_init(nfc_stack_cb_t stack_cb, u32_t needUpdate);
ry_sts_t nfc_deinit(u8_t poweroff);
void nfc_para_len_set(u8_t type, int len);
ry_sts_t nfc_para_save(u32_t addr, u8_t* data, int len);
ry_sts_t nfc_para_load(u32_t addr, u8_t* data, int len);
ry_sts_t nfc_para_block_save(void);

u8_t activate_and_deactivate_aid(const u8_t* prefix, u8_t prefix_len, const u8_t* aid, u8_t len);
int query_balance(const u8_t *aid, u8_t aid_size);
int query_city(const u8_t *aid, u8_t aid_size);
u8_t select_aid(const u8_t *aid, u8_t aid_size);
void nfc_power_off(void);
void nfc_power_down(void);
void nfc_hw_init(void);
void Enable_GPIO_IRQ(void);
void Disable_GPIO_IRQ(void);





#define ACTIVATE_SZT()     activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), AID_SZT, sizeof(AID_SZT));
#define ACTIVATE_LNT()     activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), AID_LNT, sizeof(AID_LNT));
#define ACTIVATE_BJT()     activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), AID_BJT, sizeof(AID_BJT));
#define ACTIVATE_JJJ()     activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), AID_JJJ, sizeof(AID_JJJ));
#define ACTIVATE_PBOC()    activate_and_deactivate_aid(PREFIX_ACTIVATE, sizeof(PREFIX_ACTIVATE), AID_PBOC, sizeof(AID_PBOC));
#define DEACTIVATE_SZT()   activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), AID_SZT, sizeof(AID_SZT));
#define DEACTIVATE_LNT()   activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), AID_LNT, sizeof(AID_LNT));
#define DEACTIVATE_BJT()   activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), AID_BJT, sizeof(AID_BJT));
#define DEACTIVATE_JJJ()   activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), AID_JJJ, sizeof(AID_JJJ));
#define DEACTIVATE_PBOC()  activate_and_deactivate_aid(PREFIX_DEACTIVATE, sizeof(PREFIX_DEACTIVATE), AID_PBOC, sizeof(AID_PBOC));




#endif  /* __NFC_PN80T_H__ */


