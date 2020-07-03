/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9 at Fri Jun 01 19:12:24 2018. */

#include "DevLog.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t DevLogStartResult_fields[3] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, DevLogStartResult, session_id, session_id, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, DevLogStartResult, total, session_id, 0),
    PB_LAST_FIELD
};

const pb_field_t DevLogData_fields[4] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, DevLogData, session_id, session_id, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, DevLogData, sn, session_id, 0),
    PB_FIELD(  3, BYTES   , REQUIRED, STATIC  , OTHER, DevLogData, content, sn, 0),
    PB_LAST_FIELD
};


/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_32BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in 8 or 16 bit
 * field descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(DevLogData, content) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_DevLogStartResult_DevLogData)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
#error Field descriptor for DevLogData.content is too large. Define PB_FIELD_16BIT to fix this.
#endif


/* @@protoc_insertion_point(eof) */
