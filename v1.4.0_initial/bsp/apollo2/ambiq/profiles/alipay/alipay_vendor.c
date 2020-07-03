// ****************************************************************************
//
//  alipay_vendor.c
//! @file
//!
//! @brief Ambiq Micro Alipay Profile.
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
#include <string.h>
#include <stdbool.h>
#include "am_mcu_apollo.h"
#include "am_util.h"
#include "am_util_debug.h"
#include "crc32.h"
#include "wsf_types.h"
#include "wsf_assert.h"
#include "wsf_trace.h"
#include "wsf_buf.h"
#include "bstream.h"
#include "att_api.h"
#include "app_api.h"
#include "app_hw.h"
#include "alipay_vendor.h"
#include "svc_ch.h"
#include "unix_timestamp.h"
#include "ry_utils.h"
#include "ryos.h"

//*****************************************************************************
//
// Global variables
//
//*****************************************************************************
#ifndef TX_TIMEOUT_DEFAULT
#define TX_TIMEOUT_DEFAULT 1000
#endif

#define ITEM_PRIVATE_KEY_ADDR (ITEM_PROTECT_PAGE_ADDR)

#define ITEM_SHARE_KEY_ADDR (ITEM_PRIVATE_KEY_ADDR + 1024)

#define ITEM_SEED_ADDR (ITEM_SHARE_KEY_ADDR + 1024)

#define ITEM_TIMEDIFF_ADDR (ITEM_SEED_ADDR + 1024)

#define ITEM_NICKNAME_ADDR (ITEM_TIMEDIFF_ADDR + 1024)

#define ITEM_LOGONID_ADDR (ITEM_NICKNAME_ADDR + 1024)

#define ITEM_BINDFLAG_ADDR (ITEM_LOGONID_ADDR + 1024)


#define ALIPAY_FLASH_STATIC_BUFx


#ifdef ALIPAY_FLASH_STATIC_BUF
static uint8_t alipay_item_image[AM_HAL_FLASH_PAGE_SIZE]; // 8K bytes each page
#else

#endif


//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************
/* Tx ring buffer Type */
typedef struct tx_ring
{
    uint8_t data[ATT_VALUE_MAX_LEN];
    uint16_t head;
    uint16_t tail;
} TX_RING_t;

static TX_RING_t tx_ring;

bool Is_Tx_Ring_Empty(TX_RING_t *r)
{
    return (r->head == r->tail ? true : false);
}

bool Is_Tx_Ring_Full(TX_RING_t *r)
{
    return (((r->tail + 1) % ATT_VALUE_MAX_LEN) == r->head ? true : false);
}

void Tx_Ring_Clean(TX_RING_t *r)
{
    r->head = 0;
    r->tail = 0;
}

bool Tx_Ring_In(TX_RING_t *r, uint8_t data)
{
    if (Is_Tx_Ring_Full(r))
    {
        return false;
    }
    r->data[r->tail] = data;
    r->tail = (r->tail + 1) % ATT_VALUE_MAX_LEN;
    return true;
}

uint8_t *Tx_Ring_Out(TX_RING_t *r)
{
    uint8_t *pTemp;

    if (Is_Tx_Ring_Empty(r))
    {
        return NULL;
    }

    pTemp = &(r->data[r->head]);
    r->head = (r->head + 1) % ATT_VALUE_MAX_LEN;
    return pTemp;
}

//
// alipay server status
//
typedef enum eAlipayStatus {
    ALIPAY_STATUS_SUCCESS,
    ALIPAY_STATUS_CRC_ERROR,
    ALIPAY_STATUS_INSUFFICIENT_BUFFER,
    ALIPAY_STATUS_UNKNOWN_ERROR,
    ALIPAY_STATUS_BUSY,
    ALIPAY_STATUS_TX_NOT_READY, // no connection or tx busy
    ALIPAY_STATUS_RECEIVE_CONTINUE,
    ALIPAY_STATUS_RECEIVE_DONE,
    ALIPAY_STATUS_MAX
} eAlipayStatus_t;

//
// alipay server states
//
typedef enum eAlipayState {
    ALIPAY_STATE_INIT,
    ALIPAY_STATE_TX_IDLE,
    ALIPAY_STATE_RX_IDLE,
    ALIPAY_STATE_SENDING,
    ALIPAY_STATE_GETTING_DATA,
    ALIPAY_STATE_WAITING_ACK,
    ALIPAY_STATE_MAX
} eAlipayState_t;

//
// Connection control block
//
typedef struct
{
    dmConnId_t connId;    // Connection ID
    bool_t alipayToSend;  // Alipay notification ready to be sent
                          // on this channel
    bool_t alipayIsTxing; // Alipay is during a data transfer
} alipayConn_t;

typedef struct
{
    eAlipayState_t txState;
    eAlipayState_t rxState;
    uint16_t attMtuSize;
    wsfTimer_t timeoutTimer;
    wsfTimerTicks_t txTimeoutMs;
    alipayRecvCback_t recvCback; // application callback for data reception
} alipayCore_t;

/* Control block */
static struct
{
    alipayConn_t conn; // connection control block
    wsfHandlerId_t appHandlerId;
    AlipayCfg_t cfg; // configurable parameters
    alipayCore_t core;
} alipayCb;

/* storagre item map */
typedef struct
{
    uint32_t addr;
    uint32_t len;
    bool exist;
} alipaystoretype_t;

/** 
 * storage in flash 
 *
 * address start...
 * ---------------------------------------------------------------
 * exist| length | content[0], , ,content[n-1] | crc[0], , ,crc[3]
 * ---------------------------------------------------------------
 * |->1 byte<-|-->4 bytes<--|--->n bytes<---|---->4 bytes<----|
 */
static alipaystoretype_t alipaystomap[7] = {
    {ITEM_PRIVATE_KEY_ADDR, 0x00, false},
    {ITEM_SHARE_KEY_ADDR, 0x00, false},
    {ITEM_SEED_ADDR, 0x00, false},
    {ITEM_TIMEDIFF_ADDR, 0x00, false},
    {ITEM_NICKNAME_ADDR, 0x00, false},
    {ITEM_LOGONID_ADDR, 0x00, false},
    {ITEM_BINDFLAG_ADDR, 0x00, false},
};

static void alipay_store_map_init(void)
{
    uint8_t i;
    uint8_t val;

    for (i = 0; i < 7; i++)
    {
        val = *((uint8_t *)(alipaystomap[i].addr));
        if (0x01 != val)
        {
            alipaystomap[i].exist = false;
            alipaystomap[i].len = 0x00;
        }
        else
        {
            alipaystomap[i].exist = true;
            alipaystomap[i].len = *(uint32_t *)(alipaystomap[i].addr + 1);
        }
    }
}

//*****************************************************************************
//
// Connection Open event
//
//*****************************************************************************
static void alipay_conn_open(dmEvt_t *pMsg)
{
//#ifdef WSF_TRACE_ENABLED
    hciLeConnCmplEvt_t *evt = (hciLeConnCmplEvt_t *)pMsg;

    LOG_DEBUG("connection opened\n");
    LOG_DEBUG("handle = 0x%x\n", evt->handle);
    LOG_DEBUG("role = 0x%x\n", evt->role);
    LOG_DEBUG("addrMSB = %02x%02x%02x%02x%02x%02x\n", evt->peerAddr[0], evt->peerAddr[1], evt->peerAddr[2]);
    LOG_DEBUG("addrLSB = %02x%02x%02x%02x%02x%02x\n", evt->peerAddr[3], evt->peerAddr[4], evt->peerAddr[5]);
    LOG_DEBUG("connInterval = 0x%x\n", evt->connInterval);
    LOG_DEBUG("connLatency = 0x%x\n", evt->connLatency);
    LOG_DEBUG("supTimeout = 0x%x\n", evt->supTimeout);
//#endif
}

//*****************************************************************************
//
// Connection Close event
//
//*****************************************************************************
static void alipay_conn_close(dmEvt_t *pMsg)
{
//#ifdef WSF_TRACE_ENABLED
    hciLeConnCmplEvt_t *evt = (hciLeConnCmplEvt_t *)pMsg;

    LOG_DEBUG("connection closed\n");
    LOG_DEBUG("handle = 0x%x\n", evt->handle);
    LOG_DEBUG("role = 0x%x\n", evt->role);
    LOG_DEBUG("addrMSB = %02x%02x%02x%02x%02x%02x\n", evt->peerAddr[0], evt->peerAddr[1], evt->peerAddr[2]);
    LOG_DEBUG("addrLSB = %02x%02x%02x%02x%02x%02x\n", evt->peerAddr[3], evt->peerAddr[4], evt->peerAddr[5]);
    LOG_DEBUG("connInterval = 0x%x\n", evt->connInterval);
    LOG_DEBUG("connLatency = 0x%x\n", evt->connLatency);
    LOG_DEBUG("supTimeout = 0x%x\n", evt->supTimeout);
//#endif

    /* clear connection */
    alipayCb.conn.connId = DM_CONN_ID_NONE;
    alipayCb.conn.alipayToSend = false;

    WsfTimerStop(&alipayCb.core.timeoutTimer);
    alipayCb.core.txState = ALIPAY_STATE_INIT;
    alipayCb.core.rxState = ALIPAY_STATE_RX_IDLE;
}

//*****************************************************************************
//
// Connection Update event
//
//*****************************************************************************
static void alipay_conn_update(dmEvt_t *pMsg)
{
///#ifdef WSF_TRACE_ENABLED
    hciLeConnUpdateCmplEvt_t *evt = (hciLeConnUpdateCmplEvt_t *)pMsg;

    LOG_DEBUG("connection update status = 0x%x\n", evt->status);
    LOG_DEBUG("handle = 0x%x\n", evt->handle);
    LOG_DEBUG("connInterval = 0x%x\n", evt->connInterval);
    LOG_DEBUG("connLatency = 0x%x\n", evt->connLatency);
    LOG_DEBUG("supTimeout = 0x%x\n", evt->supTimeout);
//#endif
}

//*****************************************************************************
//
// Timer Expiration handler
//
//*****************************************************************************
static void alipay_timeout_timer_expired(wsfMsgHdr_t *pMsg)
{
    LOG_DEBUG("alipay tx timeout");

    WsfTimerStartMs(&alipayCb.core.timeoutTimer, alipayCb.core.txTimeoutMs);
}

//*****************************************************************************
//
// Alipay send data
//
//*****************************************************************************
static void alipay_send_data(TX_RING_t *pTxRing)
{
    static uint8_t tx_buf[20];
    uint16_t len;
    uint8_t *pdata;

    for (len = 0; len < 20; len++)
    {
        pdata = Tx_Ring_Out(pTxRing);
        if (NULL == pdata)
        {
            break;
        }
        else
        {
            tx_buf[len] = *pdata;
        }
    }

    LOG_DEBUG("alipay transfer %d bytes", len);

    if (len > 0) {
        AttsHandleValueNtf(alipayCb.conn.connId, ALIPAY_DATA_HDL, len, tx_buf);
    }
}

/*****************************************************************************/
/*!
 *  \fn     alipayHandleValueCnf
 *
 *  \brief  Handle a received ATT handle value confirm.
 *
 *  \param  pMsg     Event message.
 *
 *  \return None.
 */
/******************************************************************************/
static void alipayHandleValueCnf(attEvt_t *pMsg)
{
    if (pMsg->hdr.status == ATT_SUCCESS)
    {
        if (pMsg->handle == ALIPAY_DATA_HDL)
        {
            LOG_DEBUG("alipay data out callback");

            if (Is_Tx_Ring_Empty(&tx_ring))
            {
                LOG_DEBUG("alipay tx buf is empty");
                alipayCb.conn.alipayIsTxing = false;
            }
            else
            {
                alipay_send_data(&tx_ring);
            }
        }
    }
    else
    {
        LOG_DEBUG("cnf status = %d, hdl = 0x%x\n", pMsg->hdr.status, pMsg->handle);
    }
}

//*****************************************************************************
//
//! @brief initialize alipay service
//!
//! @param handlerId - connection handle
//! @param pCfg - configuration parameters
//!
//! @return None
//
//*****************************************************************************
void alipay_init(wsfHandlerId_t handlerId, AlipayCfg_t *pCfg)
{
    memset(&alipayCb, 0, sizeof(alipayCb));
    alipayCb.appHandlerId = handlerId;
    alipayCb.conn.connId = DM_CONN_ID_NONE;
    alipayCb.conn.alipayToSend = false;
    alipayCb.conn.alipayIsTxing = false;
    alipayCb.core.txState = ALIPAY_STATE_INIT;
    alipayCb.core.rxState = ALIPAY_STATE_RX_IDLE;
    alipayCb.core.timeoutTimer.handlerId = handlerId;
    alipayCb.core.txTimeoutMs = TX_TIMEOUT_DEFAULT;
    alipayCb.core.recvCback = NULL;
    Tx_Ring_Clean(&tx_ring);
    alipay_store_map_init();
}

//*****************************************************************************
//
//! @brief alipay register service
//
//*****************************************************************************
void alipay_register_service(alipayRecvCback_t recvCback)
{
    alipayCb.core.recvCback = recvCback;
    SvcAlipayAddGroup();
}

//*****************************************************************************
//
//! @brief alipay register service
//
//*****************************************************************************
void alipay_unregister_service(void)
{
    alipayCb.core.recvCback = NULL;
    SvcAlipayRemoveGroup();
//    AppConnClose(alipayCb.conn.connId);
}

//*****************************************************************************
//
//! @brief alipay service process message
//!
//! @param pMsg - WsF message header
//!
//!
//! @return None
//
//*****************************************************************************
void alipay_proc_msg(wsfMsgHdr_t *pMsg)
{
    if (pMsg->event == DM_CONN_OPEN_IND)
    {
        alipay_conn_open((dmEvt_t *)pMsg);
    }
    else if (pMsg->event == DM_CONN_CLOSE_IND)
    {
        alipay_conn_close((dmEvt_t *)pMsg);
    }
    else if (pMsg->event == DM_CONN_UPDATE_IND)
    {
        alipay_conn_update((dmEvt_t *)pMsg);
    }
    else if (pMsg->event == alipayCb.core.timeoutTimer.msg.event)
    {
        alipay_timeout_timer_expired(pMsg);
    }
    else if (pMsg->event == ATTS_HANDLE_VALUE_CNF)
    {
        alipayHandleValueCnf((attEvt_t *)pMsg);
    }
}

//*****************************************************************************
//
//! @brief alipay service client write callback
//!
//*****************************************************************************
uint8_t alipay_write_cback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                           uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr)
{
    if (handle == ALIPAY_DATA_HDL)
    {
        if (NULL != alipayCb.core.recvCback)
        {
            alipayCb.core.recvCback((const uint8_t *)pValue, len);
        }
        return ATT_SUCCESS;
    }
    else
    {
        return ATT_ERR_HANDLE;
    }
}

//*****************************************************************************
//
//! @brief alipay service start
//!
//*****************************************************************************
void alipay_start(dmConnId_t connId, uint8_t timerEvt)
{
    //
    // set conn id
    //
    alipayCb.conn.connId = connId;
    alipayCb.conn.alipayToSend = TRUE;

    alipayCb.core.timeoutTimer.msg.event = timerEvt;
    alipayCb.core.txState = ALIPAY_STATE_TX_IDLE;

    alipayCb.core.attMtuSize = AttGetMtu(connId);
    LOG_DEBUG("MTU size = %d bytes", alipayCb.core.attMtuSize);
}

//*****************************************************************************
//
//! @brief alipay service stop
//!
//*****************************************************************************
void alipay_stop(dmConnId_t connId)
{
    //
    // clear connection
    //
    alipayCb.conn.connId = DM_CONN_ID_NONE;
    alipayCb.conn.alipayToSend = FALSE;

    alipayCb.core.txState = ALIPAY_STATE_INIT;
}

retval_t ALIPAY_storage_read(ALIPAY_storage_item item,
                             uint8_t *buf_content, uint32_t *len_content)
{
    retval_t ret;
    static uint32_t caldatacrc, strdatacrc, locate;
    uint8_t *pval;

    uint8_t i = 31 - __clz(item);

    LOG_DEBUG("[%s]\n", __FUNCTION__);

    caldatacrc = 0;

    if (alipaystomap[i].exist == false)
    {
        ret = RV_NOT_FOUND;
    }
    else
    {
        locate = alipaystomap[i].addr + 5;
        pval = (uint8_t *)locate + alipaystomap[i].len;

        caldatacrc = CalcCrc32(0xFFFFFFFFU, alipaystomap[i].len, (uint8_t *)locate);

        // strdatacrc = (uint32_t)(*(pval+3) << 24) + (uint32_t)(*(pval+2) << 16) + \
        //              (uint32_t)(*(pval+1) << 8) + (uint32_t)(*pval);

        strdatacrc = *(uint32_t *)pval;

        if (caldatacrc != strdatacrc)
        {
            ret = RV_READ_ERROR;
        }
        else
        {
            memcpy(buf_content, (uint8_t *)locate, alipaystomap[i].len);
            *len_content = alipaystomap[i].len;
            ret = RV_OK;
        }
    }

    return ret;
}

retval_t ALIPAY_storage_write(ALIPAY_storage_item item,
                              const uint8_t *buf_content, uint32_t len_content)
{
    uint32_t caldatacrc;
    int retl;

    uint8_t i = 31 - __clz(item);
    uint32_t offset = i * 1024;

    caldatacrc = CalcCrc32(0xFFFFFFFFU, len_content, (uint8_t *)buf_content);

    LOG_DEBUG("[%s]\n", __FUNCTION__);
#ifdef ALIPAY_FLASH_STATIC_BUF
    u8_t * temp_buf = alipay_item_image;
#else

    u8_t * temp_buf = (u8_t *)ry_malloc(AM_HAL_FLASH_PAGE_SIZE);
    if(temp_buf == NULL){
        LOG_ERROR("[%s] malloc fail\n",__FUNCTION__);
        return RV_WRITE_ERROR;
    }

#endif
    

    // load flash content into image
    memcpy(temp_buf, (uint8_t *)ITEM_PROTECT_PAGE_ADDR, AM_HAL_FLASH_PAGE_SIZE);

    /* write exsit flag */
    temp_buf[offset] = 1;

    /* write length */
    memcpy(&temp_buf[offset + 1], &len_content, 4);

    /* write content */
    memcpy(&temp_buf[offset + 5], buf_content, len_content);

    /* write crc */
    memcpy(&temp_buf[offset + 5 + len_content], (uint8_t *)&caldatacrc, 4);

#if 1
    am_hal_interrupt_master_disable();

    /* erase page */
    am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                            AM_HAL_FLASH_ADDR2INST(ITEM_PROTECT_PAGE_ADDR),
                            AM_HAL_FLASH_ADDR2PAGE(ITEM_PROTECT_PAGE_ADDR));

    retl = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,
                                     (uint32_t *)temp_buf,
                                     (uint32_t *)ITEM_PROTECT_PAGE_ADDR,
                                     AM_HAL_FLASH_PAGE_SIZE / 4);

    am_hal_interrupt_master_enable();

#else
    ry_hal_flash_write(ITEM_PROTECT_PAGE_ADDR, temp_buf, AM_HAL_FLASH_PAGE_SIZE);

#endif

#ifdef ALIPAY_FLASH_STATIC_BUF
    
#else
        ry_free(temp_buf);
#endif


    if (0 != retl)
    {
        return RV_WRITE_ERROR;
    }

    alipaystomap[i].exist = true;
    alipaystomap[i].len = len_content;

    return RV_OK;
}

retval_t ALIPAY_storage_delete(uint32_t items)
{
    int retl;
    uint32_t offset;

    LOG_DEBUG("[%s]\n", __FUNCTION__);
#ifdef ALIPAY_FLASH_STATIC_BUF
    u8_t * temp_buf = alipay_item_image;
#else

    u8_t * temp_buf = (u8_t *)ry_malloc(AM_HAL_FLASH_PAGE_SIZE);
    if(temp_buf == NULL){
        LOG_ERROR("[%s] malloc fail\n",__FUNCTION__);
        return RV_DEL_ERROR;
    }
#endif

    for (uint8_t i = 0; i < 7; i++)
    {
        if (((0x01 << i) & items) != 0)
        {
            offset = i * 1024;

            // load flash content into image
            memcpy(temp_buf, (uint8_t *)ITEM_PROTECT_PAGE_ADDR, AM_HAL_FLASH_PAGE_SIZE);

            memset(&temp_buf[offset], 0xFF, alipaystomap[i].len + 9);


#if 1

            am_hal_interrupt_master_disable();

            /* erase page */
            am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                                    AM_HAL_FLASH_ADDR2INST(ITEM_PROTECT_PAGE_ADDR),
                                    AM_HAL_FLASH_ADDR2PAGE(ITEM_PROTECT_PAGE_ADDR));

            retl = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,
                                             (uint32_t *)temp_buf,
                                             (uint32_t *)ITEM_PROTECT_PAGE_ADDR,
                                             AM_HAL_FLASH_PAGE_SIZE / 4);

            am_hal_interrupt_master_enable();

#else

            ry_hal_flash_write(ITEM_PROTECT_PAGE_ADDR, temp_buf, AM_HAL_FLASH_PAGE_SIZE);

#endif


            if (0 != retl)
            {
                return RV_DEL_ERROR;
            }

            alipaystomap[i].exist = false;
            alipaystomap[i].len = 0;
        }
    }

#ifdef ALIPAY_FLASH_STATIC_BUF

#else
    ry_free(temp_buf);
#endif

    return RV_OK;
}

retval_t ALIPAY_storage_exists(ALIPAY_storage_item item)
{
    retval_t ret;
    uint8_t i = 31 - __clz(item);

    if (alipaystomap[i].exist == false)
    {
        ret = RV_NOT_FOUND;
    }
    else
    {
        ret = RV_OK;
    }

    return ret;
}

uint32_t ALIPAY_get_timestamp(void)
{
    uint32_t time_stamp;
    time_stamp = ryos_rtc_now();

    LOG_INFO("[alipay]time stamp = %d\n", time_stamp);
    return time_stamp;
}

void ALIPAY_ble_write(uint8_t *data, uint16_t len)
{

    LOG_DEBUG("%d dats into ring buf\n", len);

    if (false == alipayCb.conn.alipayToSend)
    {
        return;
    }

    for (uint16_t i = 0; i < len; i++)
    {
        Tx_Ring_In(&tx_ring, data[i]);
    }
    LOG_DEBUG("---------%d\n", __LINE__);
    if (false == alipayCb.conn.alipayIsTxing)
    {
        alipayCb.conn.alipayIsTxing = true;
        alipay_send_data(&tx_ring);

        LOG_DEBUG("---------%d\n", __LINE__);
    }
}

static void val2chr(uint8_t *str, uint8_t *val)
{
    uint8_t msb, lsb;

    for (uint8_t i = 0; i < 6; i++)
    {
        msb = val[i] >> 4;
        lsb = val[i] & 0x0F;
        if (msb > 9)
        {
            *str++ = msb - 10 + 'A';
        }
        else
        {
            *str++ = msb - 0 + '0';
        }
        if (lsb > 9)
        {
            *str++ = lsb - 10 + 'A';
        }
        else
        {
            *str++ = lsb - 0 + '0';
        }
        if (i != 5)
        {
            *str++ = ':';
        }
        else
        {
            *str++ = '\0';
        }
    }
}

retval_t ALIPAY_get_deviceid(uint8_t *buf_did, uint32_t *len_did)
{
    uint8_t temp;
    uint8_t mac_buf[6];

    /* get mac address for advertisment */
    memcpy(mac_buf, HciGetBdAddr(), 6);

    mac_buf[2] = ~mac_buf[2];

    for (uint8_t i = 0; i < 3; i++)
    {
        temp = mac_buf[5 - i];
        mac_buf[5 - i] = mac_buf[i];
        mac_buf[i] = temp;
    }

    val2chr(buf_did, mac_buf);

    *len_did = 17;

    LOG_DEBUG("device id = %s", buf_did);

    return RV_OK;
}

void ALIPAY_log(ALIPAY_log_level level, const char *format, int32_t value)
{
    ry_board_printf(format, value);;
}
