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

/* Proto Buffer Related */
#include "nanopb_common.h"


/*********************************************************************
 * CONSTANTS
 */ 


 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */


/**
 * @brief   Common function to decode variable length value. 
 *
 * @param   stream, the output stream.
 * @param   field, the to be decode fields.
 * @param   arg, the value argument
 *
 * @return  trur of false
 */
bool nanopb_decode_val(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
    int i = 0;
    int len;
    nanopb_buf_t* ptr = (nanopb_buf_t*)(*arg);

    len = stream->bytes_left;
    if (!pb_read(stream, ptr->buf, stream->bytes_left))
        return false;

    ptr->len = len;

    return true;
}


/**
 * @brief   Common function to encode variable length value. 
 *
 * @param   stream, the output stream.
 * @param   field, the to be encoded fields.
 * @param   arg, the value argument
 *
 * @return  trur of false
 */
bool nanopb_encode_val(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    nanopb_buf_t* ptr = (nanopb_buf_t*)(*arg);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, ptr->buf, ptr->len);
}

