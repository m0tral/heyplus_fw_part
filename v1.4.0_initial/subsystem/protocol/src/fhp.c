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

#include "fhp.h"
#include "reliable_xfer.h"

/* Proto Buffer Related */
#include "nanopb_common.h"
#include "fit_health.pb.h"
#include "rbp.pb.h"


/*********************************************************************
 * CONSTANTS
 */ 





/*********************************************************************
 * TYPEDEFS
 */
 
typedef struct {
    uint16_t steps_today;
    
} fit_health_t;

 
/*********************************************************************
 * LOCAL VARIABLES
 */

static fit_health_t fit_health_v;



/*********************************************************************
 * LOCAL FUNCTIONS
 */


#if 0

/**
 * @brief   API to send fit and health data.
 *
 * @param   type, device infomation sub-type
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
ry_sts_t fhp_send(FH_TYPE type, FH_SUB_TYPE subtype, uint8_t* data, int len)
{
    nanopb_buf_t nanopb_buf;
    ry_sts_t status;
    size_t message_length;

    uint8_t* data_buf = ry_malloc(len+10);
        
    /* Allocate space for the decoded message. */
    FitHealthMsg message = FitHealthMsg_init_zero;

    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(data_buf, len+10);

    message.type             = type;
    message.subtype          = subtype;
    message.val.funcs.encode = nanopb_encode_val;
    nanopb_buf.buf           = ry_malloc(len);
    nanopb_buf.len           = len;
    ry_memcpy(nanopb_buf.buf, data, len);
    message.val.arg = &nanopb_buf;


    /* Now we are ready to encode the message! */
    status = pb_encode(&stream, FitHealthMsg_fields, &message);
    message_length = stream.bytes_written;

    /* Then just check for any errors.. */
    if (!status) {
        LOG_ERROR("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return RY_SET_STS_VAL(RY_CID_PB, RY_ERR_INVALIC_PARA);
    } else {

#if 1
        LOG_DEBUG("[fhp_send]: The Encoding FHP message len: %d\n", message_length);
        ry_data_dump(data_buf, message_length, ' ');
#endif
    }

    ry_free(nanopb_buf.buf);
    rbp_send(CMD_DATA, TYPE_FIT_HEALTH, data_buf, message_length, 1);
    ry_free(data_buf);

    return RY_SUCC;
}


void fhp_send_pedometer_instant(void)
{
    fhp_send(FH_TYPE_PEDOMETER, FH_SUB_TYPE_INSTANT, (uint8_t*)&fit_health_v.steps_today, 2);
}


void fhp_send_pedometer_detail(void)
{
    
}

void fhp_send_pedometer_history(void)
{
    
}


void fhp_send_pedometer_raw(void)
{
    
}


/**
 * @brief   Fit and health Protocol Parser
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
ry_sts_t fhp_parser(CMD cmd, uint8_t* data, uint32_t len)
{
    /* Nothing need to parse */
}

#endif

