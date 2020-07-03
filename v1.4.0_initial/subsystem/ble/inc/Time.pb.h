/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Mon Mar 12 16:32:35 2018. */

#ifndef PB_TIMEZONE_PB_H_INCLUDED
#define PB_TIMEZONE_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _TimeZoneSet {
    int32_t utc_time;
    bool has_deviation;
    int32_t deviation;
    bool has_daylight;
    int32_t daylight;
/* @@protoc_insertion_point(struct:TimeZoneSet) */
} TimeZoneSet;

/* Default values for struct fields */

/* Initializer values for message structs */
#define TimeZoneSet_init_default            {0, false, 0, false, 0}
#define TimeZoneSet_init_zero               {0, false, 0, false, 0}

/* Field tags (for use in manual encoding/decoding) */
#define TimeZoneSet_utc_time_tag            1
#define TimeZoneSet_deviation_tag           2
#define TimeZoneSet_daylight_tag            3

/* Struct field encoding specification for nanopb */
extern const pb_field_t TimeZoneSet_fields[4];

/* Maximum encoded size of messages (where known) */
#define TimeZoneSet_size                    33

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define TIMEZONE_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
