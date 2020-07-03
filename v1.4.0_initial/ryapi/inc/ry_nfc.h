/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_NFC_H__
#define __RY_NFC_H__

#include "ry_type.h"


/**
 * @brief Definitaion for RY_NFC message type.
 */
#define RY_NFC_MSG_TYPE_NONE                  0
#define RY_NFC_MSG_TYPE_GET_CPLC              1
#define RY_NFC_MSG_TYPE_CE_ON                 2
#define RY_NFC_MSG_TYPE_CE_OFF                3
#define RY_NFC_MSG_TYPE_READER_ON             4
#define RY_NFC_MSG_TYPE_READER_OFF            5
#define RY_NFC_MSG_TYPE_POWER_ON              6
#define RY_NFC_MSG_TYPE_POWER_OFF             7
#define RY_NFC_MSG_TYPE_MONITOR               8
#define RY_NFC_MSG_TYPE_POWER_DOWN            9
#define RY_NFC_MSG_TYPE_RESET                 10



/**
 * @brief Definitaion for RY_NFC message format.
 */
typedef struct {
    u8_t msg_type;
    void* msg_data;
} ry_nfc_msg_t;



typedef void (*ry_nfc_init_done_t)(void);




/**
 * @brief Definitaion for RY_NFC state.
 */
typedef enum {
    RY_NFC_STATE_IDLE,
    RY_NFC_STATE_OPERATING,
    RY_NFC_STATE_INITIALIZED,
    RY_NFC_STATE_CE_ON,
    RY_NFC_STATE_READER_POLLING,
    RY_NFC_STATE_LOW_POWER,
} ry_nfc_state_t;



/**
 * @brief   API to init SE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_se_init(ry_nfc_init_done_t initDone);

/**
 * @brief   API to Enable/Disable Reader mode
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_nfc_reader(u8_t fEnable);


/**
 * @brief   API to Enable/Disable Card emulator mode
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_nfc_ce(u8_t fEnable);


/**
 * @brief   API to send NFC related message to kernel
 *
 * @param   None
 *
 * @return  None
 */
void ry_nfc_msg_send(u8_t msg_type, void* data);

void ry_nfc_set_initdone_cb(ry_nfc_init_done_t initDone);
bool ry_nfc_is_initialized(void);
u8_t ry_nfc_get_state(void);
ry_sts_t ry_nfc_se_channel_close(void);
ry_sts_t ry_nfc_se_channel_open(void);

/**
 * @brief   API to get the SE wired state
 *
 * @param   None
 *
 * @return  1 indicating ENABLE and 0 indicating DISABLE
 */
u8_t ry_nfc_is_wired_enable(void);

void ry_nfc_sync_poweron(void);
void ry_nfc_sync_poweroff(void);
void ry_nfc_sync_ce_on(void);
void ry_nfc_sync_reader_on(void);
void ry_nfc_sync_lowpower(void);


#endif  /* __RY_NFC_H__ */
