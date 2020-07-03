// ****************************************************************************
//
//  alipay_api.h
//! @file
//!
//! @brief Ambiq Micro's demonstration of ALIPAY service.
//!
//! @{
//
// ****************************************************************************

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

#ifndef ALIPAY_API_H
#define ALIPAY_API_H

#include "wsf_os.h"
#include "ble_task.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    ALIPAY_TIMER_IND = ALIPAY_MSG_START, /*! alipay tx timeout timer expired */
    ALIPAY_SERVICE_ADD,
    ALIPAY_SERVICE_REMOVE,
};

//void Alipay_service_adv_close(void);

void alipayProcMsg(void* p_msg);
void AlipayHandleInit(uint16_t handle);

//will os delay for 100ms
void AlipaySendEventThroughBleThread(uint8_t evt_type);



#ifdef __cplusplus
};
#endif

#endif /* ALIPAY_API_H */
