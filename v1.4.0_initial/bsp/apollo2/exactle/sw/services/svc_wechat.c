/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#include "wsf_types.h"
#include "att_api.h"
#include "bstream.h"
#include "svc_wechat.h"
#include "svc_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/**************************************************************************************************
 Wechat Service group
**************************************************************************************************/

/*!
 * Ryeex service
 */

/* UUIDs */
static const uint8_t svcWechatAirsyncReadChUuid[]   = {UINT16_TO_BYTES(ATT_UUID_WECHAT_AIRSYNC_READ)};
static const uint8_t svcWechatPedoMeasureChUuid[] = {UINT16_TO_BYTES(ATT_UUID_WECHAT_PEDO_MEASURE)};


/* Wechat service declaration */
static const uint8_t wechatValSvc[] = {UINT16_TO_BYTES(ATT_UUID_WECHAT_SERVICE)};
static const uint16_t wechatSvcLen = sizeof(wechatValSvc);

/* Wechat Airsync Read characteristic */ 
static const uint8_t wechatAirsyncReadValCh[] = 
{
    ATT_PROP_READ,
    UINT16_TO_BYTES(WECHATS_SVC_CH_AIRSYNC_READ_VAL_HDL), 
    UINT16_TO_BYTES(ATT_UUID_WECHAT_AIRSYNC_READ)
};
static const uint16_t wechatAirsyncReadValChLen = sizeof(wechatAirsyncReadValCh);

/* Wechat Pedometer Measurement characteristic */ 
static const uint8_t wechatPedoMeasureValCh[] = 
{
    ATT_PROP_READ | ATT_PROP_NOTIFY,
    UINT16_TO_BYTES(WECHATS_SVC_CH_PEDO_MEASURE_VAL_HDL), 
    UINT16_TO_BYTES(ATT_UUID_WECHAT_PEDO_MEASURE)
};
static const uint16_t wechatPedoMeasureValChLen = sizeof(wechatPedoMeasureValCh);

static uint8_t wechatAirsyncReadData[6]=
{
    0x9C, 0xF6, 0xDD, 0x30, 0x01, 0xBD,
};
static const uint16_t wechatAirsuncReadDataLen = sizeof(wechatAirsyncReadData);


static uint8_t wechatPedoMeasureCCCVal[] = 
{
    UINT16_TO_BYTES(0x0000)
};
static const uint16_t wechatPedoMeasureCCCValLen = sizeof(wechatPedoMeasureCCCVal);



static uint8_t wechatPedoMeasureData[4]=
{
    0x01,0xE9, 0x00, 0x00
};
static const uint16_t wechatPedoMeasureDataLen = sizeof(wechatPedoMeasureData);



/* Attribute list for group */
static const attsAttr_t wechatSvcList[] =
{
    /* Service declaration */
    {
        attPrimSvcUuid, 
        (uint8_t *) wechatValSvc,
        (uint16_t *) &wechatSvcLen, 
        sizeof(wechatValSvc),
        0,
        ATTS_PERMIT_READ
    },

    {
        attChUuid,
        (uint8_t *) wechatPedoMeasureValCh,
        (uint16_t *) &wechatPedoMeasureValChLen,
        sizeof(wechatPedoMeasureValCh),
        0,
        ATTS_PERMIT_READ
    },
    {
        svcWechatPedoMeasureChUuid,
        (uint8_t *) wechatPedoMeasureData,
        (uint16_t *) &wechatPedoMeasureDataLen,
        sizeof(wechatPedoMeasureData),
        ATTS_SET_READ_CBACK,
        ATTS_PERMIT_READ
    },
    {
        attCliChCfgUuid,
        wechatPedoMeasureCCCVal,
        (uint16_t *) &wechatPedoMeasureCCCValLen,
        sizeof(wechatPedoMeasureCCCVal),
        ATTS_SET_CCC,
        (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
    },

    {
        attChUuid,
        (uint8_t *) wechatAirsyncReadValCh,
        (uint16_t *) &wechatAirsyncReadValChLen,
        sizeof(wechatAirsyncReadValCh),
        0,
        ATTS_PERMIT_READ
    },
    {
        svcWechatAirsyncReadChUuid,
        (uint8_t *) wechatAirsyncReadData,
        (uint16_t *) &wechatAirsuncReadDataLen,
        sizeof(wechatAirsyncReadData),
        ATTS_SET_READ_CBACK,
        ATTS_PERMIT_READ
    },
};

/* Wechat group structure */
static attsGroup_t svcWechatGroup =
{
  NULL,
  (attsAttr_t *) wechatSvcList,
  NULL,
  NULL,
  WECHAT_START_HDL,
  WECHAT_END_HDL,
};

/*************************************************************************************************/
/*!
 *  \fn     SvcBattAddGroup
 *        
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcWechatAddGroup(void)
{
  AttsAddGroup(&svcWechatGroup);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcBattRemoveGroup
 *        
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcWechatRemoveGroup(void)
{
  AttsRemoveGroup(WECHAT_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcRyeexsCbackRegister
 *        
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcWechatCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcWechatGroup.readCback  = readCback;
  svcWechatGroup.writeCback = writeCback;
}


void WechatServiceUpdateMacAddr(uint8_t* p_addr)
{

    wechatAirsyncReadData[0] = p_addr[5];
    wechatAirsyncReadData[1] = p_addr[4];
    wechatAirsyncReadData[2] = p_addr[3];
    wechatAirsyncReadData[3] = p_addr[2];
    wechatAirsyncReadData[4] = p_addr[1];
    wechatAirsyncReadData[5] = p_addr[0];
}

void WechatServiceUpdatePedometerCounter(uint16_t conn_handle, uint32_t steps)
{

    wechatPedoMeasureData[0] = 0x01;
    wechatPedoMeasureData[1] = ((steps >>  0) & 0xFF);
    wechatPedoMeasureData[2] = ((steps >>  8) & 0xFF);
    wechatPedoMeasureData[3] = ((steps >> 16) & 0xFF);

    if(conn_handle != 0xFFFF)
    {
        uint8_t pedo_steps_counter[4];

        steps = steps > 98800?98800:steps;

        pedo_steps_counter[0] = 0x01;
        pedo_steps_counter[1] = ((steps >>  0) & 0xFF);
        pedo_steps_counter[2] = ((steps >>  8) & 0xFF);
        pedo_steps_counter[3] = ((steps >> 16) & 0xFF);

        AttsHandleValueNtf(conn_handle,
            WECHATS_SVC_CH_PEDO_MEASURE_VAL_HDL,
            sizeof(pedo_steps_counter),
            pedo_steps_counter);
    
    }
}
