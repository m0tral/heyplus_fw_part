#ifndef __ACTIVITY_QRCODE_H__

#define __ACTIVITY_QRCODE_H__
#include "window_management_service.h"



extern activity_t activity_qrcode;



ry_sts_t show_qrcode(void * usrdata);

ry_sts_t exit_qrcode(void);


void create_qrcode_bitmap(void);
void qrcode_timer_stop(void);
void display_qrcode(char * str,int pos_y);



#endif

