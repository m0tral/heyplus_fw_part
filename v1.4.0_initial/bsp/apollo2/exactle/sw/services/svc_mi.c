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
#include "svc_mi.h"
#include "svc_cfg.h"
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"

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
 Mi Service group
**************************************************************************************************/

/*!
 * Mi service
 */

/* UUIDs */
static const uint8_t svcMiTokenUuid[]     = {UINT16_TO_BYTES(ATT_UUID_MI_TOKEN)};
static const uint8_t svcMiProductIdUuid[] = {UINT16_TO_BYTES(ATT_UUID_MI_PRODUCT_ID)};
static const uint8_t svcMiVerUuid[]       = {UINT16_TO_BYTES(ATT_UUID_MI_VER)};
static const uint8_t svcMiAuthUuid[]      = {UINT16_TO_BYTES(ATT_UUID_MI_AUTH)};
static const uint8_t svcMiSnUuid[]        = {UINT16_TO_BYTES(ATT_UUID_MI_SN)};
static const uint8_t svcMiBeaconKeyUuid[] = {UINT16_TO_BYTES(ATT_UUID_MI_BEACONKEY)};

/* Mi service declaration */
static const uint8_t miValSvc[] = {UINT16_TO_BYTES(ATT_UUID_MI_SERVICE)};
static const uint16_t miLenSvc = sizeof(miValSvc);

/* Mi secure channel characteristic */ 
static const uint8_t miTokenCh[] = { ATT_PROP_NOTIFY | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(MIS_TOKEN_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_MI_TOKEN)};
static const uint16_t miLenTokenCh = sizeof(miTokenCh);

/* Mi secure channel data */
static uint8_t miTokenData[MIS_CHAR_MAX_DATA] = {0};
static const uint16_t miLenTokenData = sizeof(miTokenData);

/* Mi secure channel client characteristic configuration */
static uint8_t miTokenCCC[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t miLenTokenCCC = sizeof(miTokenCCC);

/* Mi unsecure channel characteristic */ 
static const uint8_t miProductIdCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(MIS_PRODUCT_ID_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_MI_PRODUCT_ID)};
static const uint16_t miLenProductIdCh = sizeof(miProductIdCh);

/* Mi unsecure channel data */
static uint16_t miProductIdData;
static const uint16_t miLenProductIdData = sizeof(miProductIdData);

/* Mi unsecure channel characteristic */ 
static const uint8_t miVerCh[] = {ATT_PROP_READ, UINT16_TO_BYTES(MIS_VER_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_MI_VER)};
static const uint16_t miLenVerCh = sizeof(miVerCh);

/* Mi unsecure channel data */
static uint8_t miVerData[10] = {0x01, 0x02, 0x04};
static const uint16_t miLenVerData = sizeof(miVerData);

/* Mi unsecure channel characteristic */ 
static const uint8_t miAuthCh[] = {ATT_PROP_NOTIFY | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(MIS_AUTH_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_MI_AUTH)};
static const uint16_t miLenAuthCh = sizeof(miAuthCh);

/* Mi unsecure channel data */
static uint8_t miAuthData[10] = {0};
static const uint16_t miLenAuthData = sizeof(miAuthData);

/* Mi unsecure channel client characteristic configuration */
static uint8_t miAuthCCC[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t miLenAuthCCC = sizeof(miAuthCCC);

/* Mi unsecure channel characteristic */ 
static const uint8_t miSnCh[] = {ATT_PROP_READ | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(MIS_SN_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_MI_SN)};
static const uint16_t miLenSnCh = sizeof(miSnCh);

/* Mi unsecure channel data */
static uint8_t miSnData[20] = {0};
static const uint16_t miLenSnData = sizeof(miSnData);

/* Mi unsecure channel characteristic */ 
static const uint8_t miBeaconKeyCh[] = {ATT_PROP_READ | ATT_PROP_WRITE_NO_RSP | ATT_PROP_WRITE, UINT16_TO_BYTES(MIS_BEACON_KEY_VAL_HDL), UINT16_TO_BYTES(ATT_UUID_MI_BEACONKEY)};
static const uint16_t miLenBeaconKeyCh = sizeof(miBeaconKeyCh);

/* Mi unsecure channel data */
static uint8_t miBeaconKeyData[20] = {0};
static const uint16_t miLenBeaconKeyData = sizeof(miBeaconKeyData);

/* Attribute list for group */
static const attsAttr_t miSvcList[] =
{
  /* Service declaration */
  {
    attPrimSvcUuid, 
    (uint8_t *) miValSvc,
    (uint16_t *) &miLenSvc, 
    sizeof(miValSvc),
    0,
    ATTS_PERMIT_READ
  },
  /* Token Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) miTokenCh,
    (uint16_t *) &miLenTokenCh,
    sizeof(miTokenCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Token  Characteristic value */
  {
    svcMiTokenUuid,
    miTokenData,
    (uint16_t *) &miLenTokenData,
    sizeof(miTokenData),
    (ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    ATTS_PERMIT_WRITE
  },
  /* Token  Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    miTokenCCC,
    (uint16_t *) &miLenTokenCCC,
    sizeof(miTokenCCC),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },

  /* ProductID Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) miProductIdCh,
    (uint16_t *) &miLenProductIdCh,
    sizeof(miProductIdCh),
    0,
    ATTS_PERMIT_READ
  },
  /* ProductID  Characteristic value */
  {
    svcMiProductIdUuid,
    (uint8_t *)&miProductIdData,
    (uint16_t *) &miLenProductIdData,
    sizeof(miProductIdData),
    (ATTS_SET_UUID_128),
    ATTS_PERMIT_READ
  },

  /* Version Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) miVerCh,
    (uint16_t *) &miLenVerCh,
    sizeof(miVerCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Version  Characteristic value */
  {
    svcMiVerUuid,
    miVerData,
    (uint16_t *) &miLenVerData,
    sizeof(miVerData),
    (ATTS_SET_UUID_128),
    ATTS_PERMIT_READ
  },

  /* Authentication Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) miAuthCh,
    (uint16_t *) &miLenAuthCh,
    sizeof(miAuthCh),
    0,
    ATTS_PERMIT_READ
  },
  /* Authentication Characteristic value */
  {
    svcMiAuthUuid,
    miAuthData,
    (uint16_t *) &miLenAuthData,
    sizeof(miAuthData),
    (ATTS_SET_UUID_128 | ATTS_SET_VARIABLE_LEN | ATTS_SET_WRITE_CBACK),
    ATTS_PERMIT_WRITE
  },
  /* Authentication Characteristic CCC descriptor */
  {
    attCliChCfgUuid,
    miAuthCCC,
    (uint16_t *) &miLenAuthCCC,
    sizeof(miAuthCCC),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },

  /* SN Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) miSnCh,
    (uint16_t *) &miLenSnCh,
    sizeof(miSnCh),
    0,
    ATTS_PERMIT_READ
  },
  /* SN  Characteristic value */
  {
    svcMiSnUuid,
    miSnData,
    (uint16_t *) &miLenSnData,
    sizeof(miSnData),
    (ATTS_SET_UUID_128 | ATTS_SET_WRITE_CBACK | ATTS_SET_VARIABLE_LEN),
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },

  /* BeaconKey Characteristic declaration */
  {
    attChUuid,
    (uint8_t *) miBeaconKeyCh,
    (uint16_t *) &miLenBeaconKeyCh,
    sizeof(miBeaconKeyCh),
    0,
    ATTS_PERMIT_READ
  },
  /* BeaconKey Characteristic value */
  {
    svcMiBeaconKeyUuid,
    miBeaconKeyData,
    (uint16_t *) &miLenBeaconKeyData,
    sizeof(miBeaconKeyData),
    (ATTS_SET_UUID_128 | ATTS_SET_WRITE_CBACK | ATTS_SET_VARIABLE_LEN),
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  },
};

/* Mi service group structure */
static attsGroup_t svcMiGroup =
{
  NULL,
  (attsAttr_t *) miSvcList,
  NULL,
  NULL,
  MIS_START_HDL,
  MIS_END_HDL
};

/*************************************************************************************************/
/*!
 *  \fn     SvcMisAddGroup
 *        
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMisAddGroup(void)
{
  AttsAddGroup(&svcMiGroup);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcMisRemoveGroup
 *        
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMisRemoveGroup(void)
{
  AttsRemoveGroup(MIS_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcMisCbackRegister
 *        
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcMisCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcMiGroup.readCback  = readCback;
  svcMiGroup.writeCback = writeCback;
}



void SvcMisSetVer(uint8_t* ver, int len)
{
    ry_memcpy(miVerData, ver, len);
}

void SvcMisSetSn(uint8_t* sn, int len)
{
    ry_memcpy(miSnData, sn, len);
}


