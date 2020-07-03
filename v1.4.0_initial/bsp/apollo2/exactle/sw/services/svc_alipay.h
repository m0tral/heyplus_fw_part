//*****************************************************************************
//
//! @file svc_alipay.h
//!
//! @brief AmbiqMicro Alipay service definition
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
#ifndef SVC_ALIPAY_H
#define SVC_ALIPAY_H

//
// Put additional includes here if necessary.
//

#ifdef __cplusplus
extern "C" {
#endif
//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************

/*! Base UUID:  0000xxxx-0000-1000-8000-00805f9b34fb */
#define ATT_UUID_ALIPAY_BASE 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, \
                             0x00, 0x80, 0x00, 0x10, 0x00, 0x00

/*! Macro for building Ambiq UUIDs */
#define ATT_UUID_ALIPAY_BUILD(part) ATT_UUID_ALIPAY_BASE, UINT16_TO_BYTES(part), UINT16_TO_BYTES(0x00)

/*! Partial ALIPAY service UUIDs */
#define ATT_UUID_ALIPAY_SERVICE_PART 0x3802

/*! Partial ALIPAY characteristic UUIDs */
#define ATT_UUID_ALIPAY_CH_PART 0x4A02

/* ALIPAY services */
#define ATT_UUID_ALIPAY_SERVICE ATT_UUID_ALIPAY_BUILD(ATT_UUID_ALIPAY_SERVICE_PART)

/* ALIPAY characteristics */
#define ATT_UUID_ALIPAY_CH ATT_UUID_ALIPAY_BUILD(ATT_UUID_ALIPAY_CH_PART)

#define ALIPAY_START_HDL 0x0600

/* ALIPAY Service Handles */
enum
{
  ALIPAY_SVC_HDL = ALIPAY_START_HDL, /* ALIPAY service declaration */
  ALIPAY_DATA_CH_HDL,                /* ALIPAY data characteristic */
  ALIPAY_DATA_HDL,                   /* ALIPAY data */
  ALIPAY_DATA_CH_CCC_HDL,            /* ALIPAY data CCCD */
  ALIPAY_MAX_HDL
};

//*****************************************************************************
//
// Function definitions.
//
//*****************************************************************************
void SvcAlipayAddGroup(void);
void SvcAlipayRemoveGroup(void);
void SvcAlipayCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

#ifdef __cplusplus
}
#endif

#endif // SVC_ALIPAY_H
