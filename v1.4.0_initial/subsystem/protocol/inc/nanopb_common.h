/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __NANOPB_COMMON_H__
#define __NANOPB_COMMON_H__

#include "ry_type.h"

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"


/*
 * Definition a nanopb common buffer type
 */
typedef struct {
    int      len;
    uint8_t* buf;
} nanopb_buf_t;


/**
 * @brief   Common function to decode variable length value. 
 *
 * @param   stream, the output stream.
 * @param   field, the to be decode fields.
 * @param   arg, the value argument
 *
 * @return  trur of false
 */
bool nanopb_decode_val(pb_istream_t *stream, const pb_field_t *field, void **arg);


/**
 * @brief   Common function to encode variable length value. 
 *
 * @param   stream, the output stream.
 * @param   field, the to be encoded fields.
 * @param   arg, the value argument
 *
 * @return  trur of false
 */
bool nanopb_encode_val(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);



#endif  /* __NANOPB_COMMON_H__ */
