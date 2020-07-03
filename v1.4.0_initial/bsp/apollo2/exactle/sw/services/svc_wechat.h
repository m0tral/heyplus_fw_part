/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __SVC_WECHAT_H__
#define __SVC_WECHAT_H__

//
// Put additional includes here if necessary.
//

#ifdef __cplusplus
extern "C"
{
#endif
    
#include "att_api.h"
//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************

#define ATT_UUID_WECHAT_SERVICE                 0xFEE7

#define ATT_UUID_WECHAT_AIRSYNC_WRITE           0xFEC7
#define ATT_UUID_WECHAT_AIRSYNC_INDI            0xFEC8
#define ATT_UUID_WECHAT_AIRSYNC_READ            0xFEC9

#define ATT_UUID_WECHAT_PEDO_MEASURE            0xFEA1
#define ATT_UUID_WECHAT_PEDO_TARGET             0xFEA2

/*! Max length of a wechat characteristic data in one packet. */
#define WECHAT_CHAR_MAX_DATA                    (ATT_DEFAULT_MTU - 3)   //ACCORDING TO PATCH


/* Ryeex Service Handle scope */
#define WECHAT_START_HDL               0x120
#define WECHAT_END_HDL                 (WECHAT_MAX_HDL - 1)


/* Ryeex Service Handles */
enum
{
    WECHAT_SVC_HDL = WECHAT_START_HDL,     /* Wechat service declaration */
    WECHATS_SVC_CH_PEDO_MEASURE_DEF_HDL,
    WECHATS_SVC_CH_PEDO_MEASURE_VAL_HDL,
    WECHATS_SVC_CH_PEDO_MEASURE_CCC_HDL,
    WECHATS_SVC_CH_AIRSYNC_READ_DEF_HDL,
    WECHATS_SVC_CH_AIRSYNC_READ_VAL_HDL,
    WECHAT_MAX_HDL
};

#define WECHAT_END_HDL  (WECHAT_MAX_HDL - 1)

//*****************************************************************************
//
// External variable definitions
//
//*****************************************************************************
//extern uint32_t g_ui32Stuff;

//*****************************************************************************
//
// Function definitions.
//
//*****************************************************************************
void SvcWechatAddGroup(void);
void SvcWechatRemoveGroup(void);
void SvcWechatCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

void WechatServiceUpdateMacAddr(uint8_t* p_addr);
void WechatServiceUpdatePedometerCounter(uint16_t conn_handle, uint32_t steps);

#ifdef __cplusplus
}
#endif


#endif  /* __SVC_WECHAT_H__ */
