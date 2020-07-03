/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"

#include "apdu.h"
#include "seApi.h"
#include "rbp.pb.h"

#include "phOsalNfc_Log.h"
#include "scheduler.h"
#include "ry_hal_mcu.h"



/*********************************************************************
 * CONSTANTS
 */ 


#define RESP_BUFF_SIZE 1024



/*********************************************************************
 * TYPEDEFS
 */
 


 
/*********************************************************************
 * LOCAL VARIABLES
 */
    
extern ryos_mutex_t* se_mutex;



/*********************************************************************
 * LOCAL FUNCTIONS
 */


void apdu_ble_send_callback(ry_sts_t status, void* arg)
{
    if (status != RY_SUCC) {
        LOG_ERROR("[%s], error status: %x\r\n", (char*)arg, status);
    } else {
        LOG_DEBUG("[%s], error status: %x\r\n", (char*)arg, status);
    }
} 


void apdu_raw_handler(uint8_t* value, unsigned int length, unsigned short* pRspSize, uint8_t* pRespBuf, u16_t cmdId, u8_t isSec)
{
    tSEAPI_STATUS status = SEAPI_STATUS_FAILED;
    ry_sts_t ret;
    u32_t timestamp;
    
    phOsalNfc_LogCrit("\n\r--- Send APDU to the SE on wired mode ---\n");

    timestamp = ry_hal_clock_time();

    *pRspSize = 0;
    ry_memset(pRespBuf, 0, RESP_BUFF_SIZE);   //PRQA S 3200

    //passing RESP_BUFF_SIZE-1 as Maximum size of buffer so that in the pRspSize will always be less than or
    //equal to RESP_BUFF_SIZE-1. With this buffer overrun of pRespBuf will be avoided at the time of setting
    //status of operation.

    ryos_mutex_lock(se_mutex);

    status = SeApi_WiredTransceive  (value, (UINT16)length, pRespBuf, (RESP_BUFF_SIZE-1), pRspSize, 10000);
    if(status == SEAPI_STATUS_OK)
    {
     //phOsalNfc_LogBle("\nSeApi_WiredTransceive  Successful: ");
        LOG_DEBUG("APDU Resp > ");
        ry_data_dump(pRespBuf, (*pRspSize)-1, ' ');
    }
    else
    {
        LOG_ERROR("\nSeApi_WiredTransceive  Failed: 0x%x\r\n", status);
        *pRspSize = 0; // reset respSize to zero in case of failure.
    }
    pRespBuf[(*pRspSize)++] = status;/*To indicate status to the companion device*/
    phOsalNfc_LogCrit("\n\r--- END Send APDU to the SE on wired mode ---\n");

    ryos_mutex_unlock(se_mutex);


    LOG_DEBUG("[APDU] Execute Duration:%d \r\n", ry_hal_calc_ms(ry_hal_clock_time()-timestamp));

    //rbp_send_resp(cmdId, 0, pRespBuf, (*pRspSize)-1, isSec);

    ret = ble_send_response((CMD)cmdId, 0, pRespBuf, (*pRspSize)-1, isSec,
                apdu_ble_send_callback, (void*)__FUNCTION__);

    if (ret != RY_SUCC) {

        // TODO: Add exception handler. Important.
        LOG_ERROR("[%s] ble send error. status = %x\r\n", __FUNCTION__, ret);
    }      
}



