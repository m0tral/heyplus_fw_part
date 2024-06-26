/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.3.9 at Mon Nov 12 20:14:00 2018. */

#include "BodyStatus.pb.h"

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif



const pb_field_t HeartRateRecord_fields[5] = {
    PB_FIELD(  1, INT32   , REQUIRED, STATIC  , FIRST, HeartRateRecord, rate, rate, 0),
    PB_FIELD(  2, INT64   , REQUIRED, STATIC  , OTHER, HeartRateRecord, time, rate, 0),
    PB_FIELD(  3, UENUM   , REQUIRED, STATIC  , OTHER, HeartRateRecord, measure_type, time, 0),
    PB_FIELD(  4, BOOL    , REQUIRED, STATIC  , OTHER, HeartRateRecord, is_resting, measure_type, 0),
    PB_LAST_FIELD
};

const pb_field_t CommonRecord_fields[12] = {
    PB_FIELD(   1, INT64   , REQUIRED, STATIC  , FIRST, CommonRecord, start_time, start_time, 0),
    PB_FIELD(   2, INT64   , REQUIRED, STATIC  , OTHER, CommonRecord, end_time, start_time, 0),
    PB_FIELD(   3, MESSAGE , REPEATED, STATIC  , OTHER, CommonRecord, heart_rates, end_time, &HeartRateRecord_fields),
    PB_FIELD(   4, INT32   , OPTIONAL, STATIC  , OTHER, CommonRecord, calorie, heart_rates, 0),
    PB_FIELD(   5, INT32   , OPTIONAL, STATIC  , OTHER, CommonRecord, distance, calorie, 0),
    PB_FIELD(   6, INT32   , OPTIONAL, STATIC  , OTHER, CommonRecord, long_sit, distance, 0),
    PB_FIELD(   7, INT32   , REQUIRED, STATIC  , OTHER, CommonRecord, session_id, long_sit, 0),
    PB_FIELD(   8, INT32   , REQUIRED, STATIC  , OTHER, CommonRecord, total, session_id, 0),
    PB_FIELD(   9, INT32   , REQUIRED, STATIC  , OTHER, CommonRecord, sn, total, 0),
    PB_FIELD(  10, INT32   , OPTIONAL, STATIC  , OTHER, CommonRecord, deviation, sn, 0),
    PB_FIELD(  11, INT32   , OPTIONAL, STATIC  , OTHER, CommonRecord, daylight, deviation, 0),
    PB_LAST_FIELD
};

const pb_field_t BodyStatusRecord_fields[18] = {
    PB_FIELD(  1, UENUM   , REQUIRED, STATIC  , FIRST, BodyStatusRecord, type, type, 0),
    PB_ONEOF_FIELD(message,   2, MESSAGE , ONEOF, STATIC  , OTHER, BodyStatusRecord, rest, type, &RestStatusRecord_fields),
    PB_ONEOF_FIELD(message,   3, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, lite_activity, type, &LiteActivityStatusRecord_fields),
    PB_ONEOF_FIELD(message,   4, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, step, type, &StepStatusRecord_fields),
    PB_ONEOF_FIELD(message,   5, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, brisk_walk, type, &BriskWalkStatusRecord_fields),
    PB_ONEOF_FIELD(message,   6, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, stair, type, &StairStatusRecord_fields),
    PB_ONEOF_FIELD(message,   7, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, run, type, &RunStatusRecord_fields),
    PB_ONEOF_FIELD(message,   8, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, ride, type, &RideStatusRecord_fields),
    PB_ONEOF_FIELD(message,   9, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, swim, type, &SwimStatusRecord_fields),
    PB_ONEOF_FIELD(message,  10, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, sleep_lite, type, &SleepLiteStatusRecord_fields),
    PB_ONEOF_FIELD(message,  11, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, outdoor_walk, type, &OutdoorWalkStatusRecord_fields),
    PB_ONEOF_FIELD(message,  12, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, indoor_run, type, &IndoorRunStatusRecord_fields),
    PB_ONEOF_FIELD(message,  13, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, indoor_ride, type, &IndoorBikeStatusRecord_fields),
    PB_ONEOF_FIELD(message,  14, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, free, type, &FreeTrainingStatusRecord_fields),
    PB_ONEOF_FIELD(message,  15, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, outdoor_ride, type, &OutdoorBikeStatusRecord_fields),
    PB_ONEOF_FIELD(message,  16, MESSAGE , ONEOF, STATIC  , UNION, BodyStatusRecord, indoor_swim, type, &IndoorSwimmingStatusRecord_fields),
    PB_FIELD( 17, INT32   , OPTIONAL, STATIC  , OTHER, BodyStatusRecord, today_step, message.indoor_swim, 0),
    PB_LAST_FIELD
};

const pb_field_t SleepStatusRecord_fields[5] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, SleepStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, SleepStatusRecord, version, common, 0),
    PB_FIELD( 20, UINT32  , REPEATED, STATIC  , OTHER, SleepStatusRecord, rris, version, 0),
    PB_FIELD( 21, UINT32  , REQUIRED, STATIC  , OTHER, SleepStatusRecord, step, rris, 0),
    PB_LAST_FIELD
};

const pb_field_t SleepLiteStatusRecord_fields[5] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, SleepLiteStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, SleepLiteStatusRecord, version, common, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, SleepLiteStatusRecord, type, version, 0),
    PB_FIELD( 21, UINT32  , REQUIRED, STATIC  , OTHER, SleepLiteStatusRecord, step, type, 0),
    PB_LAST_FIELD
};

const pb_field_t RestStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, RestStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, INT32   , REQUIRED, STATIC  , OTHER, RestStatusRecord, step, common, 0),
    PB_LAST_FIELD
};

const pb_field_t LiteActivityStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, LiteActivityStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, INT32   , REQUIRED, STATIC  , OTHER, LiteActivityStatusRecord, step, common, 0),
    PB_LAST_FIELD
};

const pb_field_t StepStatusRecord_fields[4] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, StepStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, INT32   , REQUIRED, STATIC  , OTHER, StepStatusRecord, step, common, 0),
    PB_FIELD( 21, INT32   , REQUIRED, STATIC  , OTHER, StepStatusRecord, freq, step, 0),
    PB_LAST_FIELD
};

const pb_field_t BriskWalkStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, BriskWalkStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, INT32   , REQUIRED, STATIC  , OTHER, BriskWalkStatusRecord, step, common, 0),
    PB_LAST_FIELD
};

const pb_field_t StairStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, StairStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, INT32   , REQUIRED, STATIC  , OTHER, StairStatusRecord, step, common, 0),
    PB_LAST_FIELD
};

const pb_field_t RunStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, RunStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, RunStatusRecord, point, common, &RunStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t RunStatusRecord_Point_fields[9] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, RunStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, RunStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, RunStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, RunStatusRecord_Point, calorie, heart_rate, 0),
    PB_FIELD(  5, DOUBLE  , OPTIONAL, STATIC  , OTHER, RunStatusRecord_Point, longitude, calorie, 0),
    PB_FIELD(  6, DOUBLE  , OPTIONAL, STATIC  , OTHER, RunStatusRecord_Point, latitude, longitude, 0),
    PB_FIELD(  7, FLOAT   , OPTIONAL, STATIC  , OTHER, RunStatusRecord_Point, distance, latitude, 0),
    PB_FIELD(  8, BOOL    , OPTIONAL, STATIC  , OTHER, RunStatusRecord_Point, gps_status, distance, 0),
    PB_LAST_FIELD
};

const pb_field_t OutdoorBikeStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, OutdoorBikeStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, OutdoorBikeStatusRecord, point, common, &OutdoorBikeStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t OutdoorBikeStatusRecord_Point_fields[9] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, OutdoorBikeStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, calorie, heart_rate, 0),
    PB_FIELD(  5, DOUBLE  , OPTIONAL, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, longitude, calorie, 0),
    PB_FIELD(  6, DOUBLE  , OPTIONAL, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, latitude, longitude, 0),
    PB_FIELD(  7, FLOAT   , OPTIONAL, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, distance, latitude, 0),
    PB_FIELD(  8, BOOL    , OPTIONAL, STATIC  , OTHER, OutdoorBikeStatusRecord_Point, gps_status, distance, 0),
    PB_LAST_FIELD
};

const pb_field_t OutdoorWalkStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, OutdoorWalkStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, OutdoorWalkStatusRecord, point, common, &OutdoorWalkStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t OutdoorWalkStatusRecord_Point_fields[9] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, OutdoorWalkStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, calorie, heart_rate, 0),
    PB_FIELD(  5, DOUBLE  , OPTIONAL, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, longitude, calorie, 0),
    PB_FIELD(  6, DOUBLE  , OPTIONAL, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, latitude, longitude, 0),
    PB_FIELD(  7, FLOAT   , OPTIONAL, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, distance, latitude, 0),
    PB_FIELD(  8, BOOL    , OPTIONAL, STATIC  , OTHER, OutdoorWalkStatusRecord_Point, gps_status, distance, 0),
    PB_LAST_FIELD
};

const pb_field_t IndoorRunStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, IndoorRunStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, IndoorRunStatusRecord, point, common, &IndoorRunStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t IndoorRunStatusRecord_Point_fields[7] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, IndoorRunStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, IndoorRunStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, IndoorRunStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, IndoorRunStatusRecord_Point, calorie, heart_rate, 0),
    PB_FIELD(  5, FLOAT   , OPTIONAL, STATIC  , OTHER, IndoorRunStatusRecord_Point, distance, calorie, 0),
    PB_FIELD(  6, INT32   , REQUIRED, STATIC  , OTHER, IndoorRunStatusRecord_Point, step_freq, distance, 0),
    PB_LAST_FIELD
};

const pb_field_t IndoorBikeStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, IndoorBikeStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, IndoorBikeStatusRecord, point, common, &IndoorBikeStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t IndoorBikeStatusRecord_Point_fields[5] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, IndoorBikeStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, IndoorBikeStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, IndoorBikeStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, IndoorBikeStatusRecord_Point, calorie, heart_rate, 0),
    PB_LAST_FIELD
};

const pb_field_t IndoorSwimmingStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, IndoorSwimmingStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, IndoorSwimmingStatusRecord, point, common, &IndoorSwimmingStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t IndoorSwimmingStatusRecord_Point_fields[9] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, IndoorSwimmingStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, calorie, heart_rate, 0),
    PB_FIELD(  5, FLOAT   , REQUIRED, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, distance, calorie, 0),
    PB_FIELD(  6, INT32   , REQUIRED, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, strokes_cnt, distance, 0),
    PB_FIELD(  7, INT32   , REQUIRED, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, swolf, strokes_cnt, 0),
    PB_FIELD(  8, INT32   , OPTIONAL, STATIC  , OTHER, IndoorSwimmingStatusRecord_Point, SwimmingPosture, swolf, 0),
    PB_LAST_FIELD
};

const pb_field_t FreeTrainingStatusRecord_fields[3] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, FreeTrainingStatusRecord, common, common, &CommonRecord_fields),
    PB_FIELD( 20, MESSAGE , REPEATED, STATIC  , OTHER, FreeTrainingStatusRecord, point, common, &FreeTrainingStatusRecord_Point_fields),
    PB_LAST_FIELD
};

const pb_field_t FreeTrainingStatusRecord_Point_fields[5] = {
    PB_FIELD(  1, INT64   , REQUIRED, STATIC  , FIRST, FreeTrainingStatusRecord_Point, time, time, 0),
    PB_FIELD(  2, INT32   , REQUIRED, STATIC  , OTHER, FreeTrainingStatusRecord_Point, step, time, 0),
    PB_FIELD(  3, INT32   , REQUIRED, STATIC  , OTHER, FreeTrainingStatusRecord_Point, heart_rate, step, 0),
    PB_FIELD(  4, INT32   , REQUIRED, STATIC  , OTHER, FreeTrainingStatusRecord_Point, calorie, heart_rate, 0),
    PB_LAST_FIELD
};

const pb_field_t RideStatusRecord_fields[2] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, RideStatusRecord, common, common, &CommonRecord_fields),
    PB_LAST_FIELD
};

const pb_field_t SwimStatusRecord_fields[2] = {
    PB_FIELD(  1, MESSAGE , REQUIRED, STATIC  , FIRST, SwimStatusRecord, common, common, &CommonRecord_fields),
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
PB_STATIC_ASSERT((pb_membersize(CommonRecord, heart_rates[0]) < 65536 && pb_membersize(BodyStatusRecord, message.rest) < 65536 && pb_membersize(BodyStatusRecord, message.lite_activity) < 65536 && pb_membersize(BodyStatusRecord, message.step) < 65536 && pb_membersize(BodyStatusRecord, message.brisk_walk) < 65536 && pb_membersize(BodyStatusRecord, message.stair) < 65536 && pb_membersize(BodyStatusRecord, message.run) < 65536 && pb_membersize(BodyStatusRecord, message.ride) < 65536 && pb_membersize(BodyStatusRecord, message.swim) < 65536 && pb_membersize(BodyStatusRecord, message.sleep_lite) < 65536 && pb_membersize(BodyStatusRecord, message.outdoor_walk) < 65536 && pb_membersize(BodyStatusRecord, message.indoor_run) < 65536 && pb_membersize(BodyStatusRecord, message.indoor_ride) < 65536 && pb_membersize(BodyStatusRecord, message.free) < 65536 && pb_membersize(BodyStatusRecord, message.outdoor_ride) < 65536 && pb_membersize(BodyStatusRecord, message.indoor_swim) < 65536 && pb_membersize(SleepStatusRecord, common) < 65536 && pb_membersize(SleepLiteStatusRecord, common) < 65536 && pb_membersize(RestStatusRecord, common) < 65536 && pb_membersize(LiteActivityStatusRecord, common) < 65536 && pb_membersize(StepStatusRecord, common) < 65536 && pb_membersize(BriskWalkStatusRecord, common) < 65536 && pb_membersize(StairStatusRecord, common) < 65536 && pb_membersize(RunStatusRecord, common) < 65536 && pb_membersize(RunStatusRecord, point[0]) < 65536 && pb_membersize(OutdoorBikeStatusRecord, common) < 65536 && pb_membersize(OutdoorBikeStatusRecord, point[0]) < 65536 && pb_membersize(OutdoorWalkStatusRecord, common) < 65536 && pb_membersize(OutdoorWalkStatusRecord, point[0]) < 65536 && pb_membersize(IndoorRunStatusRecord, common) < 65536 && pb_membersize(IndoorRunStatusRecord, point[0]) < 65536 && pb_membersize(IndoorBikeStatusRecord, common) < 65536 && pb_membersize(IndoorBikeStatusRecord, point[0]) < 65536 && pb_membersize(IndoorSwimmingStatusRecord, common) < 65536 && pb_membersize(IndoorSwimmingStatusRecord, point[0]) < 65536 && pb_membersize(FreeTrainingStatusRecord, common) < 65536 && pb_membersize(FreeTrainingStatusRecord, point[0]) < 65536 && pb_membersize(RideStatusRecord, common) < 65536 && pb_membersize(SwimStatusRecord, common) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_HeartRateRecord_CommonRecord_BodyStatusRecord_SleepStatusRecord_SleepLiteStatusRecord_RestStatusRecord_LiteActivityStatusRecord_StepStatusRecord_BriskWalkStatusRecord_StairStatusRecord_RunStatusRecord_RunStatusRecord_Point_OutdoorBikeStatusRecord_OutdoorBikeStatusRecord_Point_OutdoorWalkStatusRecord_OutdoorWalkStatusRecord_Point_IndoorRunStatusRecord_IndoorRunStatusRecord_Point_IndoorBikeStatusRecord_IndoorBikeStatusRecord_Point_IndoorSwimmingStatusRecord_IndoorSwimmingStatusRecord_Point_FreeTrainingStatusRecord_FreeTrainingStatusRecord_Point_RideStatusRecord_SwimStatusRecord)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_16BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in the default
 * 8 bit descriptors.
 */
PB_STATIC_ASSERT((pb_membersize(CommonRecord, heart_rates[0]) < 256 && pb_membersize(BodyStatusRecord, message.rest) < 256 && pb_membersize(BodyStatusRecord, message.lite_activity) < 256 && pb_membersize(BodyStatusRecord, message.step) < 256 && pb_membersize(BodyStatusRecord, message.brisk_walk) < 256 && pb_membersize(BodyStatusRecord, message.stair) < 256 && pb_membersize(BodyStatusRecord, message.run) < 256 && pb_membersize(BodyStatusRecord, message.ride) < 256 && pb_membersize(BodyStatusRecord, message.swim) < 256 && pb_membersize(BodyStatusRecord, message.sleep_lite) < 256 && pb_membersize(BodyStatusRecord, message.outdoor_walk) < 256 && pb_membersize(BodyStatusRecord, message.indoor_run) < 256 && pb_membersize(BodyStatusRecord, message.indoor_ride) < 256 && pb_membersize(BodyStatusRecord, message.free) < 256 && pb_membersize(BodyStatusRecord, message.outdoor_ride) < 256 && pb_membersize(BodyStatusRecord, message.indoor_swim) < 256 && pb_membersize(SleepStatusRecord, common) < 256 && pb_membersize(SleepLiteStatusRecord, common) < 256 && pb_membersize(RestStatusRecord, common) < 256 && pb_membersize(LiteActivityStatusRecord, common) < 256 && pb_membersize(StepStatusRecord, common) < 256 && pb_membersize(BriskWalkStatusRecord, common) < 256 && pb_membersize(StairStatusRecord, common) < 256 && pb_membersize(RunStatusRecord, common) < 256 && pb_membersize(RunStatusRecord, point[0]) < 256 && pb_membersize(OutdoorBikeStatusRecord, common) < 256 && pb_membersize(OutdoorBikeStatusRecord, point[0]) < 256 && pb_membersize(OutdoorWalkStatusRecord, common) < 256 && pb_membersize(OutdoorWalkStatusRecord, point[0]) < 256 && pb_membersize(IndoorRunStatusRecord, common) < 256 && pb_membersize(IndoorRunStatusRecord, point[0]) < 256 && pb_membersize(IndoorBikeStatusRecord, common) < 256 && pb_membersize(IndoorBikeStatusRecord, point[0]) < 256 && pb_membersize(IndoorSwimmingStatusRecord, common) < 256 && pb_membersize(IndoorSwimmingStatusRecord, point[0]) < 256 && pb_membersize(FreeTrainingStatusRecord, common) < 256 && pb_membersize(FreeTrainingStatusRecord, point[0]) < 256 && pb_membersize(RideStatusRecord, common) < 256 && pb_membersize(SwimStatusRecord, common) < 256), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_HeartRateRecord_CommonRecord_BodyStatusRecord_SleepStatusRecord_SleepLiteStatusRecord_RestStatusRecord_LiteActivityStatusRecord_StepStatusRecord_BriskWalkStatusRecord_StairStatusRecord_RunStatusRecord_RunStatusRecord_Point_OutdoorBikeStatusRecord_OutdoorBikeStatusRecord_Point_OutdoorWalkStatusRecord_OutdoorWalkStatusRecord_Point_IndoorRunStatusRecord_IndoorRunStatusRecord_Point_IndoorBikeStatusRecord_IndoorBikeStatusRecord_Point_IndoorSwimmingStatusRecord_IndoorSwimmingStatusRecord_Point_FreeTrainingStatusRecord_FreeTrainingStatusRecord_Point_RideStatusRecord_SwimStatusRecord)
#endif


/* On some platforms (such as AVR), double is really float.
 * These are not directly supported by nanopb, but see example_avr_double.
 * To get rid of this error, remove any double fields from your .proto.
 */
PB_STATIC_ASSERT(sizeof(double) == 8, DOUBLE_MUST_BE_8_BYTES)

/* @@protoc_insertion_point(eof) */
