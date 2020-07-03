//*****************************************************************************
//
//! @file svc_alipay.c
//!
//! @brief Ambiq Alipay service implementation
//!
//
//*****************************************************************************

//*****************************************************************************
//
// Copyright (c) 2018, Ambiq Micro
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision v1.2.11-734-ga9d9a3d-stable of the AmbiqSuite 
// Development Package.
//
//*****************************************************************************
#include "wsf_types.h"
#include "att_api.h"
#include "wsf_trace.h"
#include "bstream.h"
#include "svc_ch.h"
#include "svc_cfg.h"
#include "svc_alipay.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef ALIPAY_SEC_PERMIT_READ
#define ALIPAY_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef ALIPAY_SEC_PERMIT_WRITE
#define ALIPAY_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/**************************************************************************************************
 Static Variables
**************************************************************************************************/
/* UUIDs */
static const uint8_t svcChUuid[] = {ATT_UUID_ALIPAY_CH};

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/* ALIPAY service declaration */
static const uint8_t alipaySvc[] = {ATT_UUID_ALIPAY_SERVICE};
static const uint16_t alipayLenSvc = sizeof(alipaySvc);

/* ALIPAY data characteristic */
static const uint8_t alipayCh[] = {ATT_PROP_READ | ATT_PROP_WRITE | ATT_PROP_NOTIFY, UINT16_TO_BYTES(ALIPAY_DATA_HDL),  ATT_UUID_ALIPAY_CH};
static const uint16_t alipayLenCh = sizeof(alipayCh);

/* ALIPAY data */
static const uint8_t alipayData[] = {0};
static const uint16_t alipayLenData = sizeof(alipayData);

 /* ALIPAY data CCCD */
static uint8_t alipayDataChCcc[] = {UINT16_TO_BYTES(0x0000)};
static const uint16_t alipayLenChCcc = sizeof(alipayDataChCcc);

/* Attribute list for ALIPAY group */
static const attsAttr_t alipayList[] =
{
  { /* Service declaration */
    attPrimSvcUuid, 
    (uint8_t *) alipaySvc,
    (uint16_t *) &alipayLenSvc, 
    sizeof(alipaySvc),
    0,
    ATTS_PERMIT_READ
  },
  { /* Characteristic declaration */
    attChUuid,
    (uint8_t *) alipayCh,
    (uint16_t *) &alipayLenCh,
    sizeof(alipayCh),
    0,
    ATTS_PERMIT_READ
  },
  { /* Characteristic value */
    svcChUuid,
    (uint8_t *) alipayData,
    (uint16_t *) &alipayLenData,
    ATT_VALUE_MAX_LEN,
    (ATTS_SET_UUID_128 | ATTS_SET_WRITE_CBACK | ATTS_SET_VARIABLE_LEN), //
    (ATTS_PERMIT_WRITE | ATTS_PERMIT_READ)
  },
  { /* Characteristic CCC descriptor */
    attCliChCfgUuid,
    (uint8_t *) alipayDataChCcc,
    (uint16_t *) &alipayLenChCcc,
    sizeof(alipayDataChCcc),
    ATTS_SET_CCC,
    (ATTS_PERMIT_READ | ATTS_PERMIT_WRITE)
  }
};

/* Alipay group structure */
static attsGroup_t svcAlipayGroup =
{
  NULL,
  (attsAttr_t *) alipayList,
  NULL,
  NULL,
  ALIPAY_START_HDL,
  ALIPAY_MAX_HDL - 1
};

/*************************************************************************************************/
/*!
 *  \fn     SvcAlipayAddGroup
 *        
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcAlipayAddGroup(void)
{
  AttsAddGroup(&svcAlipayGroup);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcAlipayRemoveGroup
 *        
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcAlipayRemoveGroup(void)
{
  AttsRemoveGroup(ALIPAY_START_HDL);
}

/*************************************************************************************************/
/*!
 *  \fn     SvcAlipayCbackRegister
 *        
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcAlipayCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
  svcAlipayGroup.readCback = readCback;
  svcAlipayGroup.writeCback = writeCback;
}

