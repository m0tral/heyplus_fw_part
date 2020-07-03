
#ifndef __RYOS_PROTOCOL_H__
#define __RYOS_PROTOCOL_H__

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "rbp.h"
#include "ry_type.h"


typedef ry_sts_t (*protoc_handle)(u32_t cmd,unsigned char *param);

typedef struct {
    int len;
	uint8_t* buf;
}msg_param_t;

typedef struct {
    int len;
    int count;
	uint8_t* buf;
}msg_repeated_param_t;


typedef struct 
{
	u16_t sof;
	u16_t len;
	u8_t *data;
	u32_t checkSum;
}ryos_protoc_t;



#define RY_MIN_SZIE				4

#define RY_PROTOC_SOF 			0xAA55


#define RY_PROTOC_TYPE_CMD		0x01
#define RY_PROTOC_TYPE_EVT		0x02
#define RY_PROTOC_TYPE_DATA		0x03



//#define RY_MODULE_ID_RELOCAL	0x01		//relocal module ID



enum
{
	RY_MODULE_ID_RELOCAL,					//relocal module ID
	RY_MODULE_ID_UPDATE_START,				//start update
	RY_MODULE_ID_UPDATING,					//firmware updating

	RY_MODULE_ID_MAX
};



typedef enum
{
	RY_PROTOC_RECV_SOF_H		= 0x0	,
	RY_PROTOC_RECV_SOF_L		= 0x1	,
	RY_PROTOC_RECV_DATA_LENGHT_H= 0x2	,
	RY_PROTOC_RECV_DATA_LENGHT_L= 0x3	,
	RY_PROTOC_RECV_DATA			= 0x4	,
}RY_PROTOC_RECV_LEVEL;



void push_data_in_buf(uint32_t data);






bool encode_buf(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
bool decode_buf(pb_istream_t *stream, const pb_field_t *field, void **arg);
bool decode_buf_repeated(pb_istream_t *stream, const pb_field_t *field, void **arg);


void ry_app_protocol_entry(void* parameter);








#endif

