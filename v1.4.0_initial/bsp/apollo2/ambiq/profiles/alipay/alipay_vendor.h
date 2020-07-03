/****************************************************************************
 * IoTPay modules 1.0 2017/10/19
 * This software is provided "AS IS," without a warranty of any kind. ALL
 * EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING
 * ANY IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * OR NON-INFRINGEMENT, ARE HEREBY EXCLUDED. ALIPAY, INC.
 * AND ITS LICENSORS SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE
 * AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS
 * DERIVATIVES. IN NO EVENT WILL ALIPAY OR ITS LICENSORS BE LIABLE FOR ANY LOST
 * REVENUE, PROFIT OR DATA, OR FOR DIRECT, INDIRECT, SPECIAL, CONSEQUENTIAL,
 * INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER CAUSED AND REGARDLESS OF THE THEORY
 * OF LIABILITY, ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE,
 * EVEN IF ALIPAY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 ****************************************************************************
 * You acknowledge that this software is not designed, licensed or intended
 * for use in the design, construction, operation or maintenance of any
 * nuclear facility.
*****************************************************************************/

#ifndef ALIPAY_VENDOR_H
#define ALIPAY_VENDOR_H

#include "alipay_common.h"
#include "svc_alipay.h"

extern const uint8_t PRODUCT_MODEL[19];

typedef enum {
    // private key
    ALIPAY_ITEM_PRIVATE_KEY = 0x01 << 0,
    // shared key based on private key and server's public key 
    ALIPAY_ITEM_SHARE_KEY   = 0x01 << 1,
    // encrypted seed from server side
    ALIPAY_ITEM_SEED        = 0x01 << 2,
    // time diff between iot client and server
    ALIPAY_ITEM_TIMEDIFF    = 0x01 << 3,
    // nick name for user
    ALIPAY_ITEM_NICKNAME    = 0x01 << 4,
    // logon ID for user
    ALIPAY_ITEM_LOGONID     = 0x01 << 5,
    // binding flag, see binding_status_e for value
    ALIPAY_ITEM_BINDFLAG    = 0x01 << 6,
} ALIPAY_storage_item;


EXTERNC retval_t ALIPAY_storage_read(ALIPAY_storage_item item,
                                   uint8_t *buf_content, uint32_t *len_content);

EXTERNC retval_t ALIPAY_storage_write(ALIPAY_storage_item item,
                                    const uint8_t *buf_content, uint32_t len_content);

EXTERNC retval_t ALIPAY_storage_delete(uint32_t items);

EXTERNC retval_t ALIPAY_storage_exists(ALIPAY_storage_item item);

EXTERNC uint32_t ALIPAY_get_timestamp(void);

EXTERNC void ALIPAY_ble_write(uint8_t *data, uint16_t len);

EXTERNC retval_t ALIPAY_get_deviceid(uint8_t* buf_did, uint32_t *len_did);

typedef enum {
    ALIPAY_LEVEL_FATAL = 0x01,
    ALIPAY_LEVEL_ERRO,
    ALIPAY_LEVEL_WARN,
    ALIPAY_LEVEL_INFO,
    ALIPAY_LEVEL_DBG,
} ALIPAY_log_level;

EXTERNC void ALIPAY_log(ALIPAY_log_level, const char *format, int32_t value);

//*****************************************************************************
//
// basic function definitions
//
//*****************************************************************************
#define ITEM_PROTECT_PAGE_ADDR 0xF8000//(1008 * 1024)

/*! Application data reception callback */
typedef void (*alipayRecvCback_t)(const uint8_t *buf, uint16_t len);

/*! Configurable parameters */
typedef struct
{
    //! Short description of each member should go here.
    uint32_t reserved;
}
AlipayCfg_t;

extern void alipay_init(wsfHandlerId_t handlerId, AlipayCfg_t *pCfg);

extern void alipay_proc_msg(wsfMsgHdr_t *pMsg);

extern uint8_t alipay_write_cback(dmConnId_t connId, uint16_t handle, uint8_t operation,
                       uint16_t offset, uint16_t len, uint8_t *pValue, attsAttr_t *pAttr);

extern void alipay_start(dmConnId_t connId, uint8_t timerEvt);

extern void alipay_stop(dmConnId_t connId);

extern void alipay_register_service(alipayRecvCback_t recvCback);

extern void alipay_unregister_service(void);

#endif






