/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Fri Aug 03 10:40:50 2018. */

#ifndef PB_DATAUPLOAD_PB_H_INCLUDED
#define PB_DATAUPLOAD_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _DataRecord_DataType {
    DataRecord_DataType_LOG = 0,
    DataRecord_DataType_BODY_STATUS = 1,
    DataRecord_DataType_SLEEP_STATUS = 2,
    DataRecord_DataType_DEV_STATISTICS = 3
} DataRecord_DataType;
#define _DataRecord_DataType_MIN DataRecord_DataType_LOG
#define _DataRecord_DataType_MAX DataRecord_DataType_DEV_STATISTICS
#define _DataRecord_DataType_ARRAYSIZE ((DataRecord_DataType)(DataRecord_DataType_DEV_STATISTICS+1))

/* Struct definitions */
typedef struct _DataRecord {
    DataRecord_DataType data_type;
    pb_callback_t val;
/* @@protoc_insertion_point(struct:DataRecord) */
} DataRecord;

typedef struct _DataSet {
    char did[20];
    pb_callback_t records;
/* @@protoc_insertion_point(struct:DataSet) */
} DataSet;

typedef struct _UploadDataStartParam {
    int32_t dataset_num;
/* @@protoc_insertion_point(struct:UploadDataStartParam) */
} UploadDataStartParam;

/* Default values for struct fields */

/* Initializer values for message structs */
#define DataRecord_init_default                  {(DataRecord_DataType)0, {{NULL}, NULL}}
#define DataSet_init_default                     {"", {{NULL}, NULL}}
#define UploadDataStartParam_init_default        {0}
#define DataRecord_init_zero                     {(DataRecord_DataType)0, {{NULL}, NULL}}
#define DataSet_init_zero                        {"", {{NULL}, NULL}}
#define UploadDataStartParam_init_zero           {0}

/* Field tags (for use in manual encoding/decoding) */
#define DataRecord_data_type_tag                 1
#define DataRecord_val_tag                       2
#define DataSet_did_tag                          1
#define DataSet_records_tag                      2
#define UploadDataStartParam_dataset_num_tag     1

/* Struct field encoding specification for nanopb */
extern const pb_field_t DataRecord_fields[3];
extern const pb_field_t DataSet_fields[3];
extern const pb_field_t UploadDataStartParam_fields[2];

/* Maximum encoded size of messages (where known) */
/* DataRecord_size depends on runtime parameters */
/* DataSet_size depends on runtime parameters */
#define UploadDataStartParam_size                11

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define DATAUPLOAD_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif