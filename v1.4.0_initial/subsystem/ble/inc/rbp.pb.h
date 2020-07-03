/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Wed Oct 17 21:28:24 2018. */

#ifndef PB_RBP_PB_H_INCLUDED
#define PB_RBP_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _CMD {
    CMD_PROP_GET = 0,
    CMD_PROP_SET = 1,
    CMD_PROP_SUBSCRIBE = 2,
    CMD_PROP_DATA = 3,
    CMD_PROP_EVENT = 4,
    CMD_DEV_GET_DEV_STATUS = 5,
    CMD_DEV_GET_DEV_INFO = 6,
    CMD_DEV_GET_DEV_CREDENTIAL = 7,
    CMD_DEV_GET_DEV_UNBIND_TOKEN = 8,
    CMD_DEV_GET_DEV_RUN_STATE = 9,
    CMD_DEV_GET_DEV_MI_UNBIND_TOKEN = 10,
    CMD_DEV_GET_DEV_MI_RYEEX_DID_TOKEN = 11,
    CMD_DEV_ACTIVATE_SE_START = 20,
    CMD_DEV_ACTIVATE_SE_RESULT = 21,
    CMD_DEV_ACTIVATE_RYEEX_CERT_START = 22,
    CMD_DEV_ACTIVATE_RYEEX_CERT_SESSION_KEY = 23,
    CMD_DEV_ACTIVATE_MI_CERT_START = 24,
    CMD_DEV_ACTIVATE_MI_CERT_SESSION_KEY = 25,
    CMD_DEV_BIND_ACK_START = 30,
    CMD_DEV_BIND_RESULT = 31,
    CMD_DEV_UNBIND = 32,
    CMD_DEV_BIND_ACK_CANCEL = 33,
    CMD_DEV_MI_UNBIND = 34,
    CMD_DEV_RYEEX_BIND_BY_TOKEN = 35,
    CMD_DEV_FW_UPDATE_TOKEN = 40,
    CMD_DEV_FW_UPDATE_START = 41,
    CMD_DEV_FW_UPDATE_FILE = 42,
    CMD_DEV_FW_UPDATE_STOP = 43,
    CMD_DEV_GATE_START_CHECK = 50,
    CMD_DEV_GATE_START_CREATE = 51,
    CMD_DEV_GATE_CREATE_ACK = 52,
    CMD_DEV_GATE_GET_INFO_LIST = 53,
    CMD_DEV_GATE_SET_INFO = 54,
    CMD_DEV_GATE_START_DELETE = 55,
    CMD_DEV_GATE_DELETE_ACK = 56,
    CMD_DEV_TRANSIT_START_CREATE = 60,
    CMD_DEV_TRANSIT_CREATE_ACK = 61,
    CMD_DEV_TRANSIT_RECHARGE_START = 62,
    CMD_DEV_TRANSIT_RECHARGE_ACK = 63,
    CMD_DEV_TRANSIT_GET_LIST = 64,
    CMD_DEV_TRANSIT_SET_USE_CITY = 65,
    CMD_DEV_WEATHER_GET_CITY_LIST = 70,
    CMD_DEV_WEATHER_SET_CITY_LIST = 71,
    
    CMD_DEV_WHITECARD_START_CREATE = 0x6F,
    CMD_DEV_WHITECARD_CREATE_ACK = 0x70,
    CMD_DEV_WHITECARD_GET_INFO_LIST = 0x71,
    CMD_DEV_WHITECARD_SET_INFO = 0x72,
    CMD_DEV_WHITECARD_START_DELETE = 0x73,
    CMD_DEV_WHITECARD_DELETE_ACK = 0x74,
    CMD_DEV_WHITECARD_AID_SYNC = 0x75,
    
    CMD_DEV_ALARM_CLOCK_GET_LIST = 80,
    CMD_DEV_ALARM_CLOCK_SET = 81,
    CMD_DEV_ALARM_CLOCK_DELETE = 82,
    CMD_DEV_SE_OPEN = 90,
    CMD_DEV_SE_CLOSE = 91,
    CMD_DEV_SE_EXECUTE_APDU = 92,
    CMD_DEV_SE_EXECUTE_APDU_OPEN = 93,
    CMD_DEV_SURFACE_GET_LIST = 100,
    CMD_DEV_SURFACE_ADD_START = 101,
    CMD_DEV_SURFACE_ADD_DATA = 102,
    CMD_DEV_SURFACE_DELETE = 103,
    CMD_DEV_SURFACE_SET_CURRENT = 104,
    CMD_DEV_CARD_SET_DEFAULT = 105,
    CMD_DEV_CARD_GET_DEFAULT = 106,
    CMD_DEV_APP_GET_LIST = 200,
    CMD_DEV_APP_SET_LIST = 201,
    CMD_DEV_NOTIFICATION = 210,
    CMD_DEV_NOTIFICATION_GET_SETTING = 211,
    CMD_DEV_NOTIFICATION_SET_SETTING = 212,
    CMD_DEV_LOG_START = 220,
    CMD_DEV_LOG_RAW_START = 221,
    CMD_DEV_LOG_RAW_ONLINE_START = 222,
    CMD_DEV_LOG_RAW_ONLINE_STOP = 223,
    CMD_DEV_MI_SCENE_GET_LIST = 230,
    CMD_DEV_MI_SCENE_ADD = 231,
    CMD_DEV_MI_SCENE_ADD_BATCH = 232,
    CMD_DEV_MI_SCENE_MODIFY = 233,
    CMD_DEV_MI_SCENE_DELETE = 234,
    CMD_DEV_MI_SCENE_DELETE_ALL = 235,
    CMD_DEV_REPAIR_RES_GET_LOST_FILE_NAMES = 240,
    CMD_DEV_REPAIR_RES_START = 241,
    CMD_DEV_REPAIR_RES_FILE = 242,
    CMD_DEV_REPAIR_RES_STOP = 243,
	CMD_DEV_RESTART_DEVICE = 250,
    CMD_DEV_SET_TIME = 900,
    CMD_DEV_UPLOAD_DATA_START = 901,
    CMD_DEV_START_LOCATION_RESULT = 902,
    CMD_DEV_UPLOAD_FILE = 903,
    CMD_DEV_SET_PHONE_APP_INFO = 904,
    CMD_DEV_APP_FOREGROUND_ENTER = 905,
    CMD_DEV_APP_FOREGROUND_EXIT = 906,
    CMD_DEV_APP_SEND_APP_STATUS = 907,
    CMD_DEV_SET_TIME_PARAM = 908,                   // Time with timezone
    CMD_APP_ACTIVATE_RYEEX_CERT_RESULT = 1000,
    CMD_APP_ACTIVATE_MI_CERT_RESULT = 1001,
    CMD_APP_UNBIND = 1002,
    CMD_APP_UPLOAD_DATA_LOCAL = 1003,
    CMD_APP_UPLOAD_DATA_REMOTE = 1004,
    CMD_APP_START_LOCATION_REQUEST = 1005,
    CMD_APP_GATE_CHECK_RESULT = 1006,
    CMD_APP_WEATHER_GET_REALTIME_INFO = 1007,
    CMD_APP_WEATHER_GET_FORECAST_INFO = 1008,
    CMD_APP_NOTIFICATION_ANSWER_CALL = 1009,
		CMD_APP_NOTIFICATION_MUTE_CALL = 1100,			// NEW COMMAND
    CMD_APP_NOTIFICATION_REJECT_CALL = 1010,
    CMD_APP_BLE_CONN_INTERVAL = 1011,
    CMD_APP_WEATHER_GET_INFO = 1012,
    CMD_APP_BIND_ACK_RESULT = 1013,
    CMD_APP_SPORT_RUN_START = 1020,
    CMD_APP_SPORT_RUN_UPDATE = 1021,
    CMD_APP_SPORT_RUN_PAUSE = 1022,
    CMD_APP_SPORT_RUN_RESUME = 1023,
    CMD_APP_SPORT_RUN_STOP = 1024,
    CMD_APP_LOG_DATA = 1030,
    CMD_APP_LOG_RAW_DATA = 1031,
    CMD_APP_LOG_RAW_ONLINE_DATA = 1032,
    CMD_APP_UPLOAD_NOTIFICATION_SETTING = 1040,
    CMD_APP_GET_APP_STATUS = 1050,
    CMD_APP_FIND_PHONE = 1051,
    CMD_APP_SPORT_RIDE_START = 1060,
    CMD_APP_SPORT_RIDE_UPDATE = 1061,
    CMD_APP_SPORT_RIDE_PAUSE = 1062,
    CMD_APP_SPORT_RIDE_RESUME = 1063,
    CMD_APP_SPORT_RIDE_STOP = 1064,
    CMD_APP_SPORT_WALK_START = 1065,
    CMD_APP_SPORT_WALK_UPDATE = 1066,
    CMD_APP_SPORT_WALK_PAUSE = 1067,
    CMD_APP_SPORT_WALK_RESUME = 1068,
    CMD_APP_SPORT_WALK_STOP = 1069,
    
    CMD_APP_SMARTHOME_EXECUTE = 1800,   // 0x708
    
    CMD_TEST_DEV_START_GSENSOR = 2000,
    CMD_TEST_DEV_END_GSENSOR = 2001,
    CMD_TEST_DEV_SEND_APDU = 2002,    
} CMD;
#define _CMD_MIN CMD_PROP_GET
#define _CMD_MAX CMD_TEST_DEV_SEND_APDU
#define _CMD_ARRAYSIZE ((CMD)(CMD_TEST_DEV_SEND_APDU+1))

/* Struct definitions */
typedef PB_BYTES_ARRAY_T(4200) RbpMsg_Ind_val_t;
typedef struct _RbpMsg_Ind {
    int32_t total;
    bool has_sn;
    int32_t sn;
    bool has_val;
    RbpMsg_Ind_val_t val;
/* @@protoc_insertion_point(struct:RbpMsg_Ind) */
} RbpMsg_Ind;

typedef struct _RbpMsg_Int32 {
    int32_t val;
/* @@protoc_insertion_point(struct:RbpMsg_Int32) */
} RbpMsg_Int32;

typedef PB_BYTES_ARRAY_T(4200) RbpMsg_Req_val_t;
typedef struct _RbpMsg_Req {
    int32_t total;
    bool has_sn;
    int32_t sn;
    bool has_val;
    RbpMsg_Req_val_t val;
/* @@protoc_insertion_point(struct:RbpMsg_Req) */
} RbpMsg_Req;

typedef PB_BYTES_ARRAY_T(4200) RbpMsg_Res_val_t;
typedef struct _RbpMsg_Res {
    int32_t total;
    int32_t sn;
    int32_t code;
    bool has_val;                   // total = 13
    RbpMsg_Res_val_t val;           // 2 + 2100
/* @@protoc_insertion_point(struct:RbpMsg_Res) */
} RbpMsg_Res;

typedef struct _RbpMsg {
    int32_t protocol_ver;           // 4 bytes
    CMD cmd;                        // 2 bytes
    bool has_session_id;            // 1 byte
    int32_t session_id;             // 4 bytes
    pb_size_t which_message;        // 2 bytes  = 13 bytes
    union {
        RbpMsg_Req req;
        RbpMsg_Res res;
        RbpMsg_Ind ind;
    } message;
/* @@protoc_insertion_point(struct:RbpMsg) */
} RbpMsg;

/* Default values for struct fields */

/* Initializer values for message structs */
#define RbpMsg_init_default                      {0, (CMD)0, false, 0, 0, {RbpMsg_Req_init_default}}
#define RbpMsg_Req_init_default                  {0, false, 0, false, {0, {0}}}
#define RbpMsg_Res_init_default                  {0, 0, 0, false, {0, {0}}}
#define RbpMsg_Ind_init_default                  {0, false, 0, false, {0, {0}}}
#define RbpMsg_Int32_init_default                {0}
#define RbpMsg_init_zero                         {0, (CMD)0, false, 0, 0, {RbpMsg_Req_init_zero}}
#define RbpMsg_Req_init_zero                     {0, false, 0, false, {0, {0}}}
#define RbpMsg_Res_init_zero                     {0, 0, 0, false, {0, {0}}}
#define RbpMsg_Ind_init_zero                     {0, false, 0, false, {0, {0}}}
#define RbpMsg_Int32_init_zero                   {0}

/* Field tags (for use in manual encoding/decoding) */
#define RbpMsg_Ind_total_tag                     1
#define RbpMsg_Ind_sn_tag                        2
#define RbpMsg_Ind_val_tag                       3
#define RbpMsg_Int32_val_tag                     1
#define RbpMsg_Req_total_tag                     1
#define RbpMsg_Req_sn_tag                        2
#define RbpMsg_Req_val_tag                       3
#define RbpMsg_Res_total_tag                     1
#define RbpMsg_Res_sn_tag                        2
#define RbpMsg_Res_code_tag                      3
#define RbpMsg_Res_val_tag                       4
#define RbpMsg_req_tag                           4
#define RbpMsg_res_tag                           5
#define RbpMsg_ind_tag                           6
#define RbpMsg_protocol_ver_tag                  1
#define RbpMsg_cmd_tag                           2
#define RbpMsg_session_id_tag                    3

/* Struct field encoding specification for nanopb */
extern const pb_field_t RbpMsg_fields[7];
extern const pb_field_t RbpMsg_Req_fields[4];
extern const pb_field_t RbpMsg_Res_fields[5];
extern const pb_field_t RbpMsg_Ind_fields[4];
extern const pb_field_t RbpMsg_Int32_fields[2];

/* Maximum encoded size of messages (where known) */
#define RbpMsg_size                              2164
#define RbpMsg_Req_size                          2125
#define RbpMsg_Res_size                          2136
#define RbpMsg_Ind_size                          2125
#define RbpMsg_Int32_size                        11

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define RBP_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif