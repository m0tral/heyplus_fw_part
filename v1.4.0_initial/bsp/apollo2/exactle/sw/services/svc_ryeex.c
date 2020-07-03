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
#include "svc_ryeex.h"
#include "svc_cfg.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef BATT_SEC_PERMIT_READ
#define BATT_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef BATT_SEC_PERMIT_WRITE
#define BATT_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/**************************************************************************************************
 Ryeex Service group
**************************************************************************************************/

/*!
 * Ryeex service
 */

/* UUIDs */
static const uint8_t svcRyeexSecChUuid[]   = {UINT16_TO_BYTES(ATT_UUID_RYEEX_SECURE_CH)};
static const uint8_t svcRyeexUnsecChUuid[] = {UINT16_TO_BYTES(ATT_UUID_RYEEX_UNSECURE_CH)};
static const uint8_t svcRyeexJsonChUuid[]  = {UINT16_TO_BYTES(ATT_UUID_RYEEX_JSON_CH)};


/* Ryeex service declaration */
static const uint8_t ryeexValSvc[] = {UINT16_TO_BYTES(ATT_UUID_RYEEX_SERVICE)};
static const uint16_t ryeexLenSvc = sizeof(ryeexValSvc);

/* Ryeex secure channel characteristic */ 
static const uint8_t rySecValCh[] = { ATT_PROP_NOTIFY | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(RYEEXS_SECURE_CH_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_RYEEX_SECURE_CH)};
static const uint16_t ryLenSecValCh = sizeof(rySecValCh);

/* Ryeex secure channel data */
static uint8_t rySecData[RYEEXS_CHAR_MAX_DATA] = {0};
static const uint16_t ryLenSecData = sizeof(rySecData);

/* Ryeex secure channel client characteristic configuration */
static uint8_t rySecValChCCC[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t ryLenSecValChCCC = sizeof(rySecValChCCC);

/* Ryeex unsecure channel characteristic */ 
static const uint8_t ryUnsecValCh[] = { ATT_PROP_NOTIFY | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(RYEEXS_UNSECURE_CH_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_RYEEX_UNSECURE_CH)};
static const uint16_t ryLenUnsecValCh = sizeof(ryUnsecValCh);

/* Ryeex unsecure channel data */
static uint8_t ryUnsecData[RYEEXS_CHAR_MAX_DATA] = {0};
static const uint16_t ryLenUnsecData = sizeof(ryUnsecData);

/* Ryeex unsecure channel client characteristic configuration */
static uint8_t ryUnsecValChCCC[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t ryLenUnsecValChCCC = sizeof(ryUnsecValChCCC);



/* Ryeex JSON channel characteristic */ 
static const uint8_t ryJsonValCh[] = {ATT_PROP_NOTIFY | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(RYEEXS_JSON_CH_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_RYEEX_JSON_CH)};
static const uint16_t ryLenJsonValCh = sizeof(ryJsonValCh);

/* Ryeex JSON channel data */
static uint8_t ryJsonData[RYEEXS_CHAR_MAX_DATA] = {0};
static const uint16_t ryLenJsonData = sizeof(ryJsonData);

/* Ryeex JSON channel client characteristic configuration */
static uint8_t ryJsonValChCCC[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t ryLenJsonValChCCC = sizeof(ryJsonValChCCC);


/* Attribute list for group */
static const attsAttr_t ryeexSvcList[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid, 
    (uint8_t *) ryeexValSvc,
    (uint16_t *) &ryeexLenSvc, 
    sizeof(ryeexValSvc),
    0,
    ATTS_PERMIT_READ
  },
  /* Secure Channel Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) rySecValCh,
    (uint16_t *) &ryLenSecValCh,
    sizeof(rySecValCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Secure Channel  Characteristic value */
  {
    svcRyeexSecChUuid,
    rySecData,
    (uint16_t *) &ryLenSecData,
    sizeof(rySecData),
    (ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    ATTS_PERMIT_WRITE
  },
  /* Secure Channel  Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    rySecValChCCC,
    (uint16_t *) &ryLenSecValChCCC,
    sizeof(rySecValChCCC),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },

  /* Unsecure Channel Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) ryUnsecValCh,
    (uint16_t *) &ryLenUnsecValCh,
    sizeof(ryUnsecValCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Unsecure Channel  Characteristic value */
  {
    svcRyeexUnsecChUuid,
    ryUnsecData,
    (uint16_t *) &ryLenUnsecData,
    sizeof(ryUnsecData),
    (ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    ATTS_PERMIT_WRITE
  },
  /* Unsecure Channel  Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    ryUnsecValChCCC,
    (uint16_t *) &ryLenUnsecValChCCC,
    sizeof(ryUnsecValChCCC),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },


  /* JSON Channel Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) ryJsonValCh,
    (uint16_t *) &ryLenJsonValCh,
    sizeof(ryJsonValCh),
    0,
    ATTS_PERMIT_READ
  },
  /* JSON Channel  Characteristic value */
  {
    svcRyeexJsonChUuid,
    ryJsonData,
    (uint16_t *) &ryLenJsonData,
    sizeof(ryJsonData),
    (ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    ATTS_PERMIT_WRITE
  },
  /* JSON Channel  Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    ryJsonValChCCC,
    (uint16_t *) &ryLenJsonValChCCC,
    sizeof(ryJsonValChCCC),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  }
};

/* Battery group structure */
static attsGroup_t svcRyeexGroup =
{
  NULL,
  (attsAttr_t *) ryeexSvcList,
  NULL,
  NULL,
  RYEEXS_START_HDL,
  RYEEXS_END_HDL
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
void SvcRyeexsAddGroup(void)
{
  AttsAddGroup(&svcRyeexGroup);
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
void SvcRyeexsRemoveGroup(void)
{
  AttsRemoveGroup(RYEEXS_START_HDL);
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
void SvcRyeexsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcRyeexGroup.readCback  = readCback;
  svcRyeexGroup.writeCback = writeCback;
}

