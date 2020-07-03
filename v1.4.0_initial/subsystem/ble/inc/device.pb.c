/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9 at Mon Sep 24 17:50:31 2018. */

#include "device.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t DeviceStatus_fields[14] = {
    PB_FIELD(  1, STRING  , REQUIRED, STATIC  , FIRST, DeviceStatus, did, did, 0),
    PB_FIELD(  2, BOOL    , REQUIRED, STATIC  , OTHER, DeviceStatus, se_activate_status, did, 0),
    PB_FIELD(  3, BOOL    , REQUIRED, STATIC  , OTHER, DeviceStatus, ryeex_cert_activate_status, se_activate_status, 0),
    PB_FIELD(  4, BOOL    , REQUIRED, STATIC  , OTHER, DeviceStatus, mi_cert_activate_status, ryeex_cert_activate_status, 0),
    PB_FIELD(  5, BOOL    , REQUIRED, STATIC  , OTHER, DeviceStatus, ryeex_bind_status, mi_cert_activate_status, 0),
    PB_FIELD(  6, BOOL    , REQUIRED, STATIC  , OTHER, DeviceStatus, mi_bind_status, ryeex_bind_status, 0),
    PB_FIELD(  7, INT32   , REQUIRED, STATIC  , OTHER, DeviceStatus, pb_max, mi_bind_status, 0),
    PB_FIELD(  8, INT32   , REQUIRED, STATIC  , OTHER, DeviceStatus, ble_conn_interval, pb_max, 0),
    PB_FIELD(  9, STRING  , OPTIONAL, STATIC  , OTHER, DeviceStatus, fw_ver, ble_conn_interval, 0),
    PB_FIELD( 10, STRING  , OPTIONAL, STATIC  , OTHER, DeviceStatus, mi_did, fw_ver, 0),
    PB_FIELD( 11, INT32   , REPEATED, STATIC  , OTHER, DeviceStatus, accept_sec_suite, mi_did, 0),
    PB_FIELD( 12, STRING  , OPTIONAL, STATIC  , OTHER, DeviceStatus, ryeex_uid, accept_sec_suite, 0),
    PB_FIELD( 13, INT32   , OPTIONAL, STATIC  , OTHER, DeviceStatus, check_status, ryeex_uid, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceRunState_fields[2] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, DeviceRunState, main_state, main_state, 0),
    PB_LAST_FIELD
};

const pb_field_t SeActivateResult_fields[2] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, SeActivateResult, code, code, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceInfo_fields[6] = {
    PB_FIELD(  1, STRING  , REQUIRED, STATIC  , FIRST, DeviceInfo, did, did, 0),
    PB_FIELD(  2, BYTES   , REQUIRED, STATIC  , OTHER, DeviceInfo, mac, did, 0),
    PB_FIELD(  3, STRING  , REQUIRED, STATIC  , OTHER, DeviceInfo, model, mac, 0),
    PB_FIELD(  4, STRING  , REQUIRED, STATIC  , OTHER, DeviceInfo, fw_ver, model, 0),
    PB_FIELD(  5, STRING  , REQUIRED, STATIC  , OTHER, DeviceInfo, uid, fw_ver, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceCredential_fields[6] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, DeviceCredential, sn, sn, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, DeviceCredential, nonce, sn, 0),
    PB_FIELD(  3, STRING  , REQUIRED, STATIC  , OTHER, DeviceCredential, did, nonce, 0),
    PB_FIELD(  4, BYTES   , REQUIRED, STATIC  , OTHER, DeviceCredential, sign, did, 0),
    PB_FIELD(  5, INT32   , REQUIRED, STATIC  , OTHER, DeviceCredential, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceUnBindToken_fields[6] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, DeviceUnBindToken, sn, sn, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, DeviceUnBindToken, nonce, sn, 0),
    PB_FIELD(  3, STRING  , REQUIRED, STATIC  , OTHER, DeviceUnBindToken, did, nonce, 0),
    PB_FIELD(  4, BYTES   , REQUIRED, STATIC  , OTHER, DeviceUnBindToken, sign, did, 0),
    PB_FIELD(  5, INT32   , REQUIRED, STATIC  , OTHER, DeviceUnBindToken, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceMiUnBindToken_fields[7] = {
    PB_FIELD(  1, UENUM   , REQUIRED, STATIC  , FIRST, DeviceMiUnBindToken, token_type, token_type, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, DeviceMiUnBindToken, sn, token_type, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, DeviceMiUnBindToken, nonce, sn, 0),
    PB_FIELD(  4, STRING  , REQUIRED, STATIC  , OTHER, DeviceMiUnBindToken, ryeex_did, nonce, 0),
    PB_FIELD(  5, BYTES   , REQUIRED, STATIC  , OTHER, DeviceMiUnBindToken, sign, ryeex_did, 0),
    PB_FIELD(  6, INT32   , REQUIRED, STATIC  , OTHER, DeviceMiUnBindToken, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceMiRyeexDidToken_fields[8] = {
    PB_FIELD(  1, UENUM   , REQUIRED, STATIC  , FIRST, DeviceMiRyeexDidToken, token_type, token_type, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, DeviceMiRyeexDidToken, sn, token_type, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, DeviceMiRyeexDidToken, nonce, sn, 0),
    PB_FIELD(  4, STRING  , REQUIRED, STATIC  , OTHER, DeviceMiRyeexDidToken, ryeex_did, nonce, 0),
    PB_FIELD(  5, STRING  , REQUIRED, STATIC  , OTHER, DeviceMiRyeexDidToken, mi_did, ryeex_did, 0),
    PB_FIELD(  6, BYTES   , REQUIRED, STATIC  , OTHER, DeviceMiRyeexDidToken, sign, mi_did, 0),
    PB_FIELD(  7, INT32   , REQUIRED, STATIC  , OTHER, DeviceMiRyeexDidToken, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t ServerUnBindToken_fields[6] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, ServerUnBindToken, sn, sn, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, ServerUnBindToken, nonce, sn, 0),
    PB_FIELD(  3, STRING  , REQUIRED, STATIC  , OTHER, ServerUnBindToken, did, nonce, 0),
    PB_FIELD(  4, BYTES   , REQUIRED, STATIC  , OTHER, ServerUnBindToken, sign, did, 0),
    PB_FIELD(  5, INT32   , REQUIRED, STATIC  , OTHER, ServerUnBindToken, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t ServerMiUnBindToken_fields[7] = {
    PB_FIELD(  1, UENUM   , REQUIRED, STATIC  , FIRST, ServerMiUnBindToken, token_type, token_type, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, ServerMiUnBindToken, sn, token_type, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, ServerMiUnBindToken, nonce, sn, 0),
    PB_FIELD(  4, STRING  , REQUIRED, STATIC  , OTHER, ServerMiUnBindToken, ryeex_did, nonce, 0),
    PB_FIELD(  5, BYTES   , REQUIRED, STATIC  , OTHER, ServerMiUnBindToken, sign, ryeex_did, 0),
    PB_FIELD(  6, INT32   , REQUIRED, STATIC  , OTHER, ServerMiUnBindToken, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t ServerRyeexBindToken_fields[9] = {
    PB_FIELD(  1, UENUM   , REQUIRED, STATIC  , FIRST, ServerRyeexBindToken, token_type, token_type, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, sn, token_type, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, nonce, sn, 0),
    PB_FIELD(  4, STRING  , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, ryeex_did, nonce, 0),
    PB_FIELD(  5, STRING  , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, ryeex_uid, ryeex_did, 0),
    PB_FIELD(  6, BYTES   , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, ryeex_device_token, ryeex_uid, 0),
    PB_FIELD(  7, BYTES   , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, sign, ryeex_device_token, 0),
    PB_FIELD(  8, INT32   , REQUIRED, STATIC  , OTHER, ServerRyeexBindToken, sign_ver, sign, 0),
    PB_LAST_FIELD
};

const pb_field_t DeviceLostFileNames_fields[4] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, DeviceLostFileNames, file_names, file_names, 0),
    PB_FIELD(  2, STRING  , REQUIRED, STATIC  , OTHER, DeviceLostFileNames, version, file_names, 0),
    PB_FIELD(  3, STRING  , REPEATED, CALLBACK, OTHER, DeviceLostFileNames, name_list, version, 0),
    PB_LAST_FIELD
};

const pb_field_t BindAckResult_fields[2] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, BindAckResult, code, code, 0),
    PB_LAST_FIELD
};

const pb_field_t BindResult_fields[3] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, BindResult, error, error, 0),
    PB_FIELD(  2, STRING  , OPTIONAL, STATIC  , OTHER, BindResult, uid, error, 0),
    PB_LAST_FIELD
};

const pb_field_t UploadFileMeta_fields[2] = {
    PB_FIELD(  1, STRING  , REQUIRED, STATIC  , FIRST, UploadFileMeta, file_name, file_name, 0),
    PB_LAST_FIELD
};

const pb_field_t BleConnIntervalParam_fields[2] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, BleConnIntervalParam, ble_conn_interval, ble_conn_interval, 0),
    PB_LAST_FIELD
};

const pb_field_t RepairResStartParam_fields[2] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, RepairResStartParam, length, length, 0),
    PB_LAST_FIELD
};

const pb_field_t RepairResStartResult_fields[1] = {
    PB_LAST_FIELD
};



/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
#error Field descriptor for UploadFileMeta.file_name is too large. Define PB_FIELD_16BIT to fix this.
#endif


/* @@protoc_insertion_point(eof) */