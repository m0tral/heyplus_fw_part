
#include "ryos_protocol.h"
#include "ryos.h"
#include "pb.h"
#include "ry_utils.h"
#include "ryos_update.h"


protoc_handle module_handle_func[RY_MODULE_ID_MAX]={NULL};

#define PROTOC_BUF_SIZE			2048
uint32_t protoc_recv_buf[PROTOC_BUF_SIZE] = {11};

u32_t first_data = 0,last_data = 0;
ryos_semaphore_t * cur_sem;
//ry_event_t *pro_event;



void push_data_in_buf(uint32_t data)
{
	protoc_recv_buf[last_data] = data;

	last_data = (last_data + 1) % PROTOC_BUF_SIZE;
	//ryos_semaphore_post(cur_sem);
	//ryos_event_send(pro_event, 1);
}

uint32_t pull_data_from_buf(void)
{
	uint32_t data = 0;

	if(first_data != last_data)
		data = protoc_recv_buf[first_data];
	else {
		data = 0xffffffff;
	}

	first_data = (first_data + 1) % PROTOC_BUF_SIZE;


	return data;

}



void reg_module_handle_func(u32_t module_id,protoc_handle callback)
{
	module_handle_func[module_id] = callback;
}


ry_sts_t ryos_ack_init(RbpMsg  *     ack, CMD cmd, u32_t type )
{
	//ack->sof = RY_PROTOC_SOF;
	//ack->len = 4;
	//ack->type = type;
	//ack->moduleId = moduleId;
	//ack->cmd = cmd;
	//ack->param = NULL;
	
	//ack->checkSum = (ack->type)^(ack->moduleId)^(ack->cmd);

	return RY_SUCC;
}

void test_send(u8_t *rt_log_buf,u32_t length)
{
	//struct rt_device* console_device = rt_device_find("uart3");
	//u16_t old_flag = console_device->open_flag;

   // console_device->open_flag |= RT_DEVICE_FLAG_STREAM;
   // rt_device_open(console_device, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_STREAM);
    //rt_device_write(console_device, 0, rt_log_buf, length);
    //console_device->open_flag = old_flag;
}
ry_sts_t test_module(u32_t cmd,unsigned char *param)
{
	
	return RY_SUCC;
}


pb_ostream_t ryos_encode_msg(ryos_protoc_t* msg,RbpMsg * msg_data,uint8_t *data,u32_t dataSize)
{

    uint8_t buffer[128] = {0};
    size_t message_length;
    bool status;
	int i;

	msg_param_t tb;

    pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *)&buffer[4], sizeof(buffer)-4);
    		
	#if 0
	tb.buf = (uint8_t *)ry_malloc(dataSize);
	if (!tb.buf) {
		return stream;
	}
	memcpy(tb.buf, data, dataSize);
	tb.len = dataSize;


	if (msg_data->cmd <= CMD_PROP_LOG) {
	
		msg_data->message.profile.val.funcs.encode = encode_buf;
		msg_data->message.profile.val.arg = &tb;
		
	}else if ( (msg_data->cmd >= CMD_DEV_GET_DEV_STATUS) && (msg_data->cmd <= CMD_DEV_SET_TIME)){
	
		msg_data->message.req.val.funcs.encode = encode_buf;
		msg_data->message.req.val.arg = &tb;
		
	}else if ( msg_data->cmd == CMD_APP_UNBIND ){

	}

	//msg_data->val.funcs.encode = encode_buf;
	//msg_data->val.arg = &tb;

    
    status = pb_encode(&stream, RbpMsg_fields, msg_data);
    if (status ==false){
		LOG_DEBUG("encode false\n");
    }
    message_length = stream.bytes_written;

	/*LOG_DEBUG("type = %d\tencode size = %d\n",msg_data->type, message_length);
    for (i = 4; i < message_length +4; i++) {
	    LOG_DEBUG("%02x ", buffer[i]);
	}*/

	buffer[0] = (RY_PROTOC_SOF>>8);
	buffer[1] = (RY_PROTOC_SOF&0xff);
	buffer[2] = message_length>>8;
	buffer[3] = (message_length&0xff);

	//LOG_DEBUG("\n\n\n");
	//test_send(buffer,message_length+4);

    //TODO: ???????????????
	ry_free(tb.buf);

#endif
	return stream;
	
}

ry_sts_t ryos_decode_msg(RbpMsg * msg_data,uint8_t* buffer,uint32_t message_length)
{
#ifdef XXXXX
	// TODO: ???????????????
	//uint8_t buffer[128];
    //size_t message_length;
    
    bool status;
	int i;
	msg_param_t tb;
    
    pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*)buffer, message_length);


	if (msg_data->cmd <= CMD_PROP_LOG) {

		msg_data->message.profile.val.funcs.decode = decode_buf;
		msg_data->message.profile.val.arg = &tb;
		
	}else if ( (msg_data->cmd >= CMD_DEV_GET_DEV_STATUS) && (msg_data->cmd <= CMD_DEV_SET_TIME)){

		msg_data->message.req.val.funcs.decode = decode_buf;
		msg_data->message.req.val.arg = &tb;
		
	}else if ( msg_data->cmd == CMD_APP_UNBIND ){

	}
    
	//msg_data->val.funcs.decode = decode_buf;
	
	//msg_data->val.arg = &tb;

    status = pb_decode(&stream, RbpMsg_fields, msg_data);

   /* LOG_DEBUG("type= %d\tlen = %d\n",msg_data->type,tb.len);

    for(i = 0;i < tb.len ; i++)
    {
		LOG_DEBUG("[%x]",tb.buf[i]);
    }
    LOG_DEBUG("\n");*/



	if (msg_data->cmd <= CMD_PROP_LOG) {

		msg_data->message.profile.val.arg = tb.buf;
		
	}else if ( (msg_data->cmd >= CMD_DEV_GET_DEV_STATUS) && (msg_data->cmd <= CMD_DEV_SET_TIME)){

		msg_data->message.req.val.arg = tb.buf;
		
	}else if ( msg_data->cmd == CMD_APP_UNBIND ){

	}
    
    //msg_data->val.arg = tb.buf;

#endif
	//TODO:ry_free(tb.buf)
	return RY_SUCC;
}



ry_sts_t ryos_procotol_handle(RbpMsg * msg)
{
	RbpMsg ack;
	ry_sts_t status = RY_SUCC;
	u8_t* data_param = NULL;
	/*if(msg.sof != RY_PROTOC_SOF)
	{
		status = RY_SET_STS_VAL(RY_CID_PROTOC, RY_ERR_READ);

		goto ERROR;
	}

	if(msg.len < RY_MIN_SZIE)
	{
		status = RY_SET_STS_VAL(RY_CID_PROTOC, RY_ERR_READ);

		goto ERROR;
	}*/

	/*if(msg.module_id >= RY_MODULE_ID_MAX)
	{
		status = RY_SET_STS_VAL(RY_CID_PROTOC, RY_ERR_READ);

		goto ERROR;
	}*/
#if 0
	if (msg->cmd <= CMD_PROP_LOG) {
	
		data_param = msg->message.profile.val.arg;
		
	}else if ( (msg->cmd >= CMD_DEV_GET_DEV_STATUS) && (msg->cmd <= CMD_DEV_SET_TIME)){
	
		data_param = msg->message.req.val.arg;
		
	}else if ( msg->cmd == CMD_APP_UNBIND ){

	}

	if (module_handle_func[msg->cmd] != NULL) {
	
		switch(msg->cmd)
		{
			case RY_MODULE_ID_RELOCAL:
				{
					if(module_handle_func[msg->cmd] != NULL)
					{
						//u8_t* data_param = ((msg->val.arg));
						
						status = (module_handle_func[msg->cmd])(msg->cmd,data_param);
					}
					else
					{

					}
				}
				break;

			case RY_MODULE_ID_UPDATE_START:
				{
					//u8_t* data_param = ((msg->val.arg));
					
					status = (module_handle_func[msg->cmd])(msg->cmd,data_param);

				}
				break;

			case RY_MODULE_ID_UPDATING:
				{
					//u8_t* data_param = ((msg->val.arg));
					
					status = (module_handle_func[msg->cmd])(msg->cmd,data_param);

				}
				break;

			default:
				{
					
				}
				

		}
	}
	else {
		//this module not used
	}



	//ack.sof = RY_PROTOC_SOF;
	ryos_ack_init(&ack, msg->cmd, msg->cmd);
	//ryos_encode_msg(ack,NULL,0);
#endif
ERROR:

	return status;
}





bool encode_buf(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
	msg_param_t* ptr = (msg_param_t*)(*arg);


    if (!pb_encode_tag_for_field(stream, field))
        return false;

    //return pb_encode_string(stream, (uint8_t*)str, strlen(str));
	//return pb_encode_string(stream, (uint8_t*)buf, 5);
	return pb_encode_string(stream, ptr->buf, ptr->len);
}


bool decode_buf(pb_istream_t *stream, const pb_field_t *field, void **arg)
{
#if 0
    uint8_t buffer[2048] = {0};
	int i = 0;
	int len;
	msg_param_t* ptr = (msg_param_t*)(*arg);


    if (stream->bytes_left > sizeof(buffer) - 1)
        return false;

	len = stream->bytes_left;
    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;

	ptr->buf = ry_malloc(len);

	memcpy(ptr->buf, buffer ,len);
	ptr->len = len;

#else
	uint8_t *buffer = NULL;
	int i = 0;
	int len;
	msg_param_t* ptr = (msg_param_t*)(*arg);

	buffer = (uint8_t *)ry_malloc(stream->bytes_left + 5);

    if (buffer == NULL)
        return false;

	len = stream->bytes_left;
    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;

	ptr->buf = ry_malloc(len);

	memcpy(ptr->buf, buffer ,len);
	ptr->len = len;

	ry_free(buffer);

#endif
    return true;
}

bool decode_buf_repeated(pb_istream_t *stream, const pb_field_t *field, void **arg)
{

#if 0

    uint8_t buffer[2048] = {0};
	int i = 0;
	int len;
	msg_repeated_param_t* ptr = (msg_repeated_param_t*)(*arg);


    if (stream->bytes_left > sizeof(buffer) - 1)
        return false;

	len = stream->bytes_left;
    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;

	ptr->count++;

	void * temp_p = ry_malloc(len * (ptr->count));

	memcpy(temp_p,ptr->buf,(len * (ptr->count - 1) ) );

	ry_free(ptr->buf);

	ptr->buf = temp_p;

	memcpy( &(ptr->buf[(len * (ptr->count - 1) )]), buffer ,len);
	ptr->len = len;
#else

	uint8_t * buffer = NULL;
	int i = 0;
	int len;
	msg_repeated_param_t* ptr = (msg_repeated_param_t*)(*arg);

	buffer = (uint8_t *)ry_malloc(stream->bytes_left + 5);
    if (buffer == NULL)
        return false;

	len = stream->bytes_left;
    if (!pb_read(stream, buffer, stream->bytes_left))
        return false;

	ptr->count++;

	void * temp_p = ry_malloc(len * (ptr->count));

	memcpy(temp_p,ptr->buf,(len * (ptr->count - 1) ) );

	ry_free(ptr->buf);

	ptr->buf = temp_p;

	memcpy( &(ptr->buf[(len * (ptr->count - 1) )]), buffer ,len);
	ptr->len = len;

	ry_free(buffer);

#endif
    return true;
}



void ry_app_protocol_entry(void* parameter)
{
	//ryos_semaphore_t * cur_sem;
	ryos_ipc_t ipc;
	ipc.flag = 0;
	strcpy(ipc.name,"test");
	//ryos_semaphore_create(&cur_sem, NULL);
	//ryos_event_create(&pro_event,&ipc);
	u32_t ent;


	static u32_t Level = 0;
	static u32_t data_len = 0;
	static u8_t * recvDataBuf = NULL;
	static u32_t recvedDataSize = 0;

	RbpMsg cur_msg;

	////////////////////
	int i = 0;
	//rt_device_open(rt_device_find("uart3"), RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_STREAM|RT_DEVICE_FLAG_INT_RX);
	test_send((uint8_t*)protoc_recv_buf,20);

	module_handle_func[RY_MODULE_ID_RELOCAL] = test_module;
	module_handle_func[RY_MODULE_ID_UPDATING] = update_handle;

	while(1) {
		//ryos_semaphore_wait(cur_sem);
		//ryos_event_recv(pro_event, 1, (0x01|0x04), -1, &ent);
		if (first_data == last_data) {
			ryos_delay_ms(10);
			continue;
		}
		uint32_t data = pull_data_from_buf();

		LOG_DEBUG("%02x  ",data);

		switch(Level)
		{
			case RY_PROTOC_RECV_SOF_H:
			{
				if(data == (RY_PROTOC_SOF>>8))
					Level++;
			}
			break;

			case RY_PROTOC_RECV_SOF_L:
			{
				if (data == (RY_PROTOC_SOF&0x00ff)) {
					Level++;
				}
				else if (data == (RY_PROTOC_SOF>>8)) {
					
				}
				else {
					Level = RY_PROTOC_RECV_SOF_H;
				}
			}
			break;

			case RY_PROTOC_RECV_DATA_LENGHT_H:
			{
				data_len = data;
				Level++;
			}
			break;

			case RY_PROTOC_RECV_DATA_LENGHT_L:
			{
				data_len = (data_len<<8)|(data&0xff);
				ry_free(recvDataBuf);
				recvDataBuf = ry_malloc(data_len);
				recvedDataSize = 0;
				Level++;
			}
			break;


			case RY_PROTOC_RECV_DATA:
			{
				recvDataBuf[recvedDataSize++] = data;
				if (recvedDataSize == data_len) {
					Level = RY_PROTOC_RECV_SOF_H;
					/*for(i = 0;i<recvedDataSize;i++)
					{
						LOG_DEBUG("%02x  ",recvDataBuf[i]);
					}*/
					
					ryos_decode_msg(&cur_msg,recvDataBuf,recvedDataSize);//ignore checkSum
					LOG_DEBUG("OK\n");
					
					ryos_procotol_handle(&cur_msg);
					//u8_t test_send_buf[4]={1,2,3};
					
					//ryos_encode_msg(NULL,&cur_msg,test_send_buf,3);
				}
			}


		}
		
	}

}


































