/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9 at Wed Oct 17 21:28:56 2018. */

#ifndef PB_NOTIFICATION_PB_H_INCLUDED
#define PB_NOTIFICATION_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Enum definitions */
typedef enum _DeviceAlertType {
    DeviceAlertType_find_phone_start = 1,
    DeviceAlertType_find_phone_restart = 2,
    DeviceAlertType_find_phone_abort = 3
} DeviceAlertType;
#define _DeviceAlertType_MIN DeviceAlertType_find_phone_start
#define _DeviceAlertType_MAX DeviceAlertType_find_phone_abort
#define _DeviceAlertType_ARRAYSIZE ((DeviceAlertType)(DeviceAlertType_find_phone_abort+1))

typedef enum _Notification_Type {
    Notification_Type_TELEPHONY = 0,
    Notification_Type_SMS = 1,
    Notification_Type_APP_MESSAGE = 2
} Notification_Type;
#define _Notification_Type_MIN Notification_Type_TELEPHONY
#define _Notification_Type_MAX Notification_Type_APP_MESSAGE
#define _Notification_Type_ARRAYSIZE ((Notification_Type)(Notification_Type_APP_MESSAGE+1))

typedef enum _Telephony_Status {
    Telephony_Status_RINGING_ANSWERABLE = 0,
    Telephony_Status_RINGING_UNANSWERABLE = 1,
    Telephony_Status_CONNECTED = 2,
    Telephony_Status_DISCONNECTED = 3,
    Telephony_Status_RINGING_ANSWERABLE_UNREJECTABLE = 4,
    Telephony_Status_RINGING_UNANSWERABLE_UNREJECTABLE = 5
} Telephony_Status;
#define _Telephony_Status_MIN Telephony_Status_RINGING_ANSWERABLE
#define _Telephony_Status_MAX Telephony_Status_RINGING_UNANSWERABLE_UNREJECTABLE
#define _Telephony_Status_ARRAYSIZE ((Telephony_Status)(Telephony_Status_RINGING_UNANSWERABLE_UNREJECTABLE+1))

/* Struct definitions */
typedef struct _APP_MESSAGE {
    char app_id[100];
    bool has_title;
    char title[200];
    bool has_text;
    char text[1200];
    bool has_sub_text;
    char sub_text[500];
/* @@protoc_insertion_point(struct:APP_MESSAGE) */
} APP_MESSAGE;

typedef struct _DeviceAlert {
    int32_t version;
    DeviceAlertType type;
    bool has_timeout_ms;
    int32_t timeout_ms;
/* @@protoc_insertion_point(struct:DeviceAlert) */
} DeviceAlert;

typedef PB_BYTES_ARRAY_T(2000) Notification_val_t;
typedef struct _Notification {
    Notification_Type type;
    Notification_val_t val;
/* @@protoc_insertion_point(struct:Notification) */
} Notification;

typedef struct _NotificationAnswerCallParam {
    char number[100];
/* @@protoc_insertion_point(struct:NotificationAnswerCallParam) */
} NotificationAnswerCallParam;

typedef struct _NotificationMuteCallParam {
    char number[100];
/* @@protoc_insertion_point(struct:NotificationMuteCallParam) */
} NotificationMuteCallParam;

typedef struct _NotificationRejectCallParam {
    char number[100];
/* @@protoc_insertion_point(struct:NotificationRejectCallParam) */
} NotificationRejectCallParam;

typedef struct _NotificationSettingItem {
    char app_id[32];
    bool is_open;
/* @@protoc_insertion_point(struct:NotificationSettingItem) */
} NotificationSettingItem;

typedef struct _SMS {
    char sender[200];
    char content[1700];
    bool has_contact;
    char contact[100];
/* @@protoc_insertion_point(struct:SMS) */
} SMS;

typedef struct _Telephony {
    Telephony_Status status;
    char number[200];
    bool has_contact;
    char contact[100];
/* @@protoc_insertion_point(struct:Telephony) */
} Telephony;

typedef struct _NotificationSetting {
    int32_t version;
    bool is_all_open;
    bool is_ios_ancs_open;
    bool is_others_open;
    pb_size_t items_count;
    NotificationSettingItem items[20];
/* @@protoc_insertion_point(struct:NotificationSetting) */
} NotificationSetting;

/* Default values for struct fields */

/* Initializer values for message structs */
#define Notification_init_default                {(Notification_Type)0, {0, {0}}}
#define Telephony_init_default                   {(Telephony_Status)0, "", false, ""}
#define SMS_init_default                         {"", "", false, ""}
#define APP_MESSAGE_init_default                 {"", false, "", false, "", false, ""}
#define NotificationAnswerCallParam_init_default {""}
#define NotificationMuteCallParam_init_default	 {""}
#define NotificationRejectCallParam_init_default {""}
#define NotificationSetting_init_default         {0, 0, 0, 0, 0, {NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default, NotificationSettingItem_init_default}}
#define NotificationSettingItem_init_default     {"", 0}
#define DeviceAlert_init_default                 {0, (DeviceAlertType)0, false, 0}
#define Notification_init_zero                   {(Notification_Type)0, {0, {0}}}
#define Telephony_init_zero                      {(Telephony_Status)0, "", false, ""}
#define SMS_init_zero                            {"", "", false, ""}
#define APP_MESSAGE_init_zero                    {"", false, "", false, "", false, ""}
#define NotificationAnswerCallParam_init_zero    {""}
#define NotificationMuteCallParam_init_zero      {""}
#define NotificationRejectCallParam_init_zero    {""}
#define NotificationSetting_init_zero            {0, 0, 0, 0, 0, {NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero, NotificationSettingItem_init_zero}}
#define NotificationSettingItem_init_zero        {"", 0}
#define DeviceAlert_init_zero                    {0, (DeviceAlertType)0, false, 0}

/* Field tags (for use in manual encoding/decoding) */
#define APP_MESSAGE_app_id_tag                   1
#define APP_MESSAGE_title_tag                    2
#define APP_MESSAGE_text_tag                     3
#define APP_MESSAGE_sub_text_tag                 4
#define DeviceAlert_version_tag                  1
#define DeviceAlert_type_tag                     2
#define DeviceAlert_timeout_ms_tag               3
#define Notification_type_tag                    1
#define Notification_val_tag                     2
#define NotificationAnswerCallParam_number_tag   1
#define NotificationMuteCallParam_number_tag		 1
#define NotificationRejectCallParam_number_tag   1
#define NotificationSettingItem_app_id_tag       1
#define NotificationSettingItem_is_open_tag      2
#define SMS_sender_tag                           1
#define SMS_content_tag                          2
#define SMS_contact_tag                          3
#define Telephony_status_tag                     1
#define Telephony_number_tag                     2
#define Telephony_contact_tag                    3
#define NotificationSetting_version_tag          1
#define NotificationSetting_is_all_open_tag      2
#define NotificationSetting_is_ios_ancs_open_tag 3
#define NotificationSetting_is_others_open_tag   4
#define NotificationSetting_items_tag            5

/* Struct field encoding specification for nanopb */
extern const pb_field_t Notification_fields[3];
extern const pb_field_t Telephony_fields[4];
extern const pb_field_t SMS_fields[4];
extern const pb_field_t APP_MESSAGE_fields[5];
extern const pb_field_t NotificationAnswerCallParam_fields[2];
extern const pb_field_t NotificationMuteCallParam_fields[2];
extern const pb_field_t NotificationRejectCallParam_fields[2];
extern const pb_field_t NotificationSetting_fields[6];
extern const pb_field_t NotificationSettingItem_fields[3];
extern const pb_field_t DeviceAlert_fields[4];

/* Maximum encoded size of messages (where known) */
#define Notification_size                        2005
#define Telephony_size                           307
#define SMS_size                                 2008
#define APP_MESSAGE_size                         2011
#define NotificationAnswerCallParam_size         102
#define NotificationMuteCallParam_size           102
#define NotificationRejectCallParam_size         102
#define NotificationSetting_size                 777
#define NotificationSettingItem_size             36
#define DeviceAlert_size                         24

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define NOTIFICATION_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
