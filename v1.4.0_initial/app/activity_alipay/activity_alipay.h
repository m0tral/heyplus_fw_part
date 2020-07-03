#ifndef __ACTIVITY_ALIPAY_H__

#define __ACTIVITY_ALIPAY_H__




#include "ry_font.h"
#include "gfx.h"
#include "gui_bare.h"
#include "ry_utils.h"
#include "ry_hal_mcu.h"
#include "Notification.pb.h"
#include "touch.h"
#include "gui.h"
#include "am_devices_cy8c4014.h"

#include "gui_img.h"
#include "ryos_timer.h"
#include "motar.h"
#include "scheduler.h"
#include "scrollbar.h"
#include "ry_statistics.h"
#include "card_management_service.h"
#include "device_management_service.h"
#include "window_management_service.h"
#include "gui_msg.h"

#include <stdio.h>


typedef enum{
    
    ALIPAY_TYPE_BARCODE,
    ALIPAY_TYPE_QRCODE,
    ALIPAY_TYPE_UNBIND,
    
    ALIPAY_TYPE_MAX,

    ALIPAY_TYPE_UNBIND_CONFIRM,
    
}alipay_code_type_e;

typedef enum{
    ALIPAY_BIND_PROTOCOL,
    ALIPAY_BIND_START,
    ALIPAY_BIND_QRCODE,
    ALIPAY_BIND_SUCC,
    ALIPAY_BIND_FAIL,
}alipay_bind_status_e;


typedef enum{
    ALIPAY_ACTV_EVT_START = 0x10000,
    ALIPAY_ACTV_EVT_TIMEOUT,
    ALIPAY_ACTV_EVT_BIND_FINISH,
    ALIPAY_ACTV_EVT_REFRESH_CODE,
    ALIPAY_ACTV_EVT_END,
}alipay_activity_event_e;




#define CODE128_BRIGHTNESS_NORMAL               0xC0
#define CODE128_BRIGHTNESS_MAX                  0xFF

#define CODE128_Y_MAX                           200

#define ALIPAY_DEVICE_INFO_SIZE_MAX             57
#define ALIPAY_SCROLLBAR_LEN                    80


#define ALIPAY_PAYCODE_REFRESH_SECONED          49
#define ALIPAY_PAYCODE_TIMEOUT_SECONED          59


#define ALIPAY_BARCODE_BRIGHTNESS               80
#define ALIPAY_QRCODE_BRIGHTNESS                80



extern activity_t activity_alipay;





ry_sts_t create_alipay(void *userdata);

ry_sts_t Destroy_alipay(void);

void display_pay_code(void);






#endif

