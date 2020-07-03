/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 */
#ifndef __BUILDCFG_H
#define __BUILDCFG_H
#include <string.h>

#include <stdio.h>
#include "data_types.h"

#ifndef NFC_CONTORLLER_ID
#define NFC_CONTORLLER_ID       (1)
#endif

#define BTE_APPL_MAX_USERIAL_DEV_NAME           (256)

#ifdef BT_TRACE_VERBOSE
#undef BT_TRACE_VERBOSE
#endif
#if defined(DEBUG) && (ADDITIONAL_DEBUG_TRACES == TRUE)
#define BT_TRACE_VERBOSE      TRUE
#else
/* Minimal traces when executing with BLE and/or in Release mode */
#define BT_TRACE_VERBOSE      FALSE
#endif

#define TRACE_TASK_INCLUDED   TRUE
#ifndef GKI_NUM_FIXED_BUF_POOLS
#if (NXP_EXTNS == TRUE)
#define GKI_NUM_FIXED_BUF_POOLS 3
#else
#define GKI_NUM_FIXED_BUF_POOLS 4
#endif
#endif

#if(WEARABLE_MEMORY_ALLOCATION_TYPE == NORMAL_MODE)
#define GKI_BUF0_SIZE           128
#define GKI_BUF0_MAX            10
#else
#define GKI_BUF0_SIZE           128
#define GKI_BUF0_MAX            20
#endif

#define GKI_BUF1_SIZE           268
#define GKI_BUF1_MAX            10

#define GKI_BUF2_SIZE           300 // Decreased to 300. For wearable we dont need big routing table size.

#ifndef GKI_BUF2_MAX
#if(WEARABLE_MEMORY_ALLOCATION_TYPE == NORMAL_MODE)
#define GKI_BUF2_MAX            10
#else
#define GKI_BUF2_MAX            34
#endif
#endif

#ifndef GKI_BUF3_MAX
#if (NXP_EXTNS == TRUE)
#define GKI_BUF3_MAX            0
#else
#define GKI_BUF3_MAX            40
#endif
#endif

#define GKI_BUF4_MAX            0
#define GKI_BUF5_MAX            0
#define GKI_BUF6_MAX            0
#define GKI_BUF7_MAX            0
#define GKI_BUF8_MAX            0

#define NCI_BUF_POOL_ID         GKI_POOL_ID_0

#ifdef LOGS_TO_CONSOLE
#define APPL_INITIAL_TRACE_LEVEL            BT_TRACE_LEVEL_ERROR
#define NFC_INITIAL_TRACE_LEVEL             BT_TRACE_LEVEL_ERROR
#endif

#ifdef  __cplusplus
extern "C" {
#endif
// +++from bte.h...
enum
{
                            /* BTE                  BBY                     */
                            /* J3   J4              SW3-3   SW3-2   SW3-1   */
                            /* -------------------------------------------- */
    BTE_MODE_SERIAL_APP,    /* OUT  OUT             OFF     OFF     OFF     Sample serial port application      */
    BTE_MODE_APPL,          /* IN   OUT             OFF     OFF     ON      Target used with Tester through RPC */
    BTE_MODE_RESERVED,      /* OUT  IN              OFF     ON      OFF     Reserved                            */
    BTE_MODE_SAMPLE_APPS,   /* IN   IN              OFF     ON      ON      Sample applications (ICP/HSP)       */
    BTE_MODE_DONGLE,        /* not yet supported    ON      OFF     OFF     Dongle mode                         */
    BTE_MODE_APPL_PROTOCOL_TRACE, /* this is a fake mode do allow protocol tracing in application without rpc */
    BTE_MODE_INVALID
};
/* Protocol trace mask */
extern UINT32 bte_proto_trace_mask;/* = 0xFFFFFFFF;*/
extern volatile UINT8 bte_target_mode;
// ---from bte.h...


extern UINT8 *scru_dump_hex (UINT8 *p, char *p_title, UINT32 len, UINT32 trace_layer, UINT32 trace_type);
extern void ScrLog(UINT32 trace_set_mask, const char *fmt_str, ...);
extern void DispNci (UINT8 *p, UINT16 len, BOOLEAN is_recv);

extern void downloadFirmwarePatchFile (UINT32 brcm_hw_id);

#if defined(DEBUG)
void ProtoDispAdapterDisplayNciPacket (UINT8* nciPacket, UINT16 nciPacketLen, BOOLEAN is_recv);
#define DISP_NCI ProtoDispAdapterDisplayNciPacket
#define LOGMSG_TAG_NAME "BrcmNfcNfa"
#endif

#ifndef _TIMEB
#define _TIMEB
struct _timeb
{
    long    time;
    short   millitm;
    short   timezone;
    short   dstflag;
};
void    _ftime (struct _timeb*);

#endif

#ifdef  __cplusplus
};
#endif
#endif
