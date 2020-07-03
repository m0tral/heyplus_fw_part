#include <stdbool.h>
#include "window_management_service.h"
#include "gfx.h"
#include "gui.h"
#include "gui_img.h"
#include "gui_bare.h"
#include "ry_font.h"
#include "touch.h"
#include "ryos_timer.h"
#include "activity_surface.h"
#include "Notification.pb.h"
#include "pb_encode.h"
#include "rbp.h"
#include "activity_find_phone.h"
#include "device_management_service.h"


#define FIND_PHONE_PHONE_POSITION_X    26
#define FIND_PHONE_PHONE_POSITION_Y    64

#define FIND_PHONE_CHECK_POSITION_X    66
#define FIND_PHONE_CHECK_POSITION_Y    96

#define FIND_PHONE_STEPS               6

#define FONTS_RY_CUSTOMIZE             0


typedef struct
{
    uint32_t findPhoneFreshTimer;
    uint32_t freshInterval_ms;
    uint32_t freshIndexLast;
    uint32_t freshIndexNow;
    bool isBackToRoot;
}activity_find_phone_ctx_t;

typedef struct
{
    uint8_t x;
    uint8_t y;
}find_phone_process_ctrl_pixel_t;

static ry_sts_t find_phone_activity_onCreate(void* usrdata);
ry_sts_t find_phone_activity_onDestrory(void);


static uint8_t const find_phone_1_bmp[] = {
0, 0, 0, 0, 0, 0, 17, 96, 175, 217, 243, 243, 217, 175, 96, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 8, 121, 240, 255, 255, 255, 255, 255, 255, 255, 255, 240, 121, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 27, 203, 255, 255, 188, 101, 39, 12, 12, 39, 101, 188, 255, 255, 203, 27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 27, 228, 255, 218, 63, 0, 0, 0, 0, 0, 0, 0, 0, 63, 218, 255, 228, 27, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 8, 203, 255, 194, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 194, 255, 203, 8, 0, 0, 0, 0, 0, 0, 0, 
0, 121, 255, 218, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 218, 255, 121, 0, 0, 0, 0, 0, 0, 0, 
17, 240, 255, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 255, 240, 17, 0, 0, 0, 0, 0, 0, 
96, 255, 188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 188, 255, 96, 0, 0, 0, 0, 0, 0, 
175, 255, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 101, 255, 175, 0, 0, 0, 0, 0, 0, 
217, 255, 39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 39, 255, 217, 0, 0, 0, 0, 0, 0, 
243, 255, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 255, 243, 0, 0, 0, 0, 0, 0, 
243, 255, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 255, 243, 0, 0, 0, 0, 0, 0, 
217, 255, 39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 39, 255, 217, 0, 0, 0, 0, 0, 0, 
175, 255, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 101, 255, 174, 0, 0, 0, 0, 0, 0, 
96, 255, 188, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 188, 255, 96, 0, 0, 0, 0, 0, 0, 
17, 240, 255, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 255, 240, 17, 0, 0, 0, 0, 0, 0, 
0, 121, 255, 218, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 218, 255, 121, 0, 0, 0, 0, 0, 0, 0, 
0, 8, 203, 255, 194, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 194, 255, 204, 8, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 27, 228, 255, 218, 63, 0, 0, 0, 0, 0, 0, 0, 0, 63, 219, 255, 254, 215, 22, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 27, 203, 255, 255, 188, 101, 39, 12, 12, 39, 101, 188, 255, 255, 204, 215, 255, 212, 22, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 8, 121, 240, 255, 255, 255, 255, 255, 255, 255, 255, 240, 121, 8, 22, 212, 255, 212, 22, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 17, 96, 175, 217, 243, 243, 217, 175, 96, 17, 0, 0, 0, 22, 212, 255, 212, 21, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 212, 255, 212, 21, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 212, 255, 212, 21, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 212, 255, 212, 21, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 212, 255, 212, 21, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 212, 255, 196, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 196, 180, 
};

static uint8_t const find_phone_2_bmp[] = {
0, 28, 166, 230, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 230, 166, 28, 0, 
28, 237, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 237, 28, 
166, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 37, 0, 0, 0, 0, 37, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 166, 
230, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 230, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249, 119, 25, 25, 119, 249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 119, 0, 0, 0, 0, 119, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 25, 0, 0, 0, 0, 25, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 25, 0, 0, 0, 0, 25, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 119, 0, 0, 0, 0, 119, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 
230, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 249, 119, 25, 25, 119, 249, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 230, 
166, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 166, 
28, 237, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 237, 28, 
0, 28, 166, 230, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 230, 166, 28, 0, 
};

static const find_phone_process_ctrl_pixel_t find_phone_img_pos[FIND_PHONE_STEPS] = 
{
	{12 + FIND_PHONE_CHECK_POSITION_X, 1  + FIND_PHONE_CHECK_POSITION_Y},
    {21 + FIND_PHONE_CHECK_POSITION_X, 6  + FIND_PHONE_CHECK_POSITION_Y},
    {21 + FIND_PHONE_CHECK_POSITION_X, 16 + FIND_PHONE_CHECK_POSITION_Y},
    {12 + FIND_PHONE_CHECK_POSITION_X, 22 + FIND_PHONE_CHECK_POSITION_Y},
    {3  + FIND_PHONE_CHECK_POSITION_X, 16 + FIND_PHONE_CHECK_POSITION_Y},
    {3  + FIND_PHONE_CHECK_POSITION_X, 6  + FIND_PHONE_CHECK_POSITION_Y},
};

static char const* const findPhoneText[3] = 
{
    "Поиск",
    "Нет связи",
    "Отменено",
};

static uint32_t findPhoneIconColor = HTML2COLOR(0xFEFEFE);

static activity_find_phone_ctx_t* p_find_phone_ctx = NULL;
static u8_t displayTimeCount = 0;
static u8_t recivceReqStatus = 0;

static const raw_png_descript_t icon_find_phone_check = 
{
    .p_data = find_phone_1_bmp,
    .width = 28,
    .height = 28,
    .fgcolor = White,
};

static const raw_png_descript_t icon_find_phone_phone = 
{
    .p_data = find_phone_2_bmp,
    .width = 34,
    .height = 60,
    .fgcolor = White,
};

activity_t activity_find_phone = {\
    .name      = "find_phone",\
    .onCreate  = find_phone_activity_onCreate,\
    .onDestroy = find_phone_activity_onDestrory,\
    .disableUntouchTimer = 1,\
    .priority = WM_PRIORITY_H,\
    .brightness = 100,\
};



static void find_phone_display_connected(void)
{
	uint32_t nowIndex = p_find_phone_ctx->freshIndexNow % FIND_PHONE_STEPS;
    uint32_t x,y;
	
	gdispFillStringBox(24, 
			170, 
			0,
			22,
			(char*)findPhoneText[0], 
			FONTS_RY_CUSTOMIZE, 
			White,
			Black,
			justifyCenter
		);

	x = find_phone_img_pos[nowIndex].x;
	y = find_phone_img_pos[nowIndex].y;
	draw_raw_png_to_framebuffer(&icon_find_phone_check, x,y, findPhoneIconColor);

	x = FIND_PHONE_PHONE_POSITION_X;
	y = FIND_PHONE_PHONE_POSITION_Y;
	draw_raw_png_to_framebuffer(&icon_find_phone_phone, x,y, findPhoneIconColor);
}

static void find_phone_display_disconncted(void)
{
    uint32_t x,y;
	
	dispStringConstantWidth(60, 
			154, 
			0,
			22,
			(char*)findPhoneText[1], 
			FONTS_RY_CUSTOMIZE, 
			White,
			Black,
			justifyCenter
		);

	dispStringConstantWidth(60, 
			154+24 , 
			0,
			22,
			(char*)findPhoneText[2], 
			FONTS_RY_CUSTOMIZE, 
			White,
			Black,
			justifyCenter
		);

	x = FIND_PHONE_CHECK_POSITION_X;
	y = FIND_PHONE_CHECK_POSITION_Y;
	draw_raw_png_to_framebuffer(&icon_find_phone_check, x,y, findPhoneIconColor);

	x = FIND_PHONE_PHONE_POSITION_X;
	y = FIND_PHONE_PHONE_POSITION_Y;
	draw_raw_png_to_framebuffer(&icon_find_phone_phone, x,y, findPhoneIconColor);
}

static int find_phone_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    
	if (p->y == 9) {
		displayTimeCount = 0;
		
		if (ry_ble_get_state() == RY_BLE_STATE_CONNECTED) {
			if (get_device_session() != DEV_SESSION_IDLE) {
				LOG_ERROR("[find_phone], session state=%d.",get_device_session());
			}else {
				find_phone_request(DeviceAlertType_find_phone_abort);
			}
		}
		ry_sched_msg_send(IPC_MSG_TYPE_FIND_PHONE_STOP, 0, NULL);
        LOG_DEBUG("[activity_find_phone] cancle find phone.\n");
	}
    return consumed;
}

static void find_phone_fresh_frame(void)
{
    clear_buffer_black();

	if (ry_ble_get_state() == RY_BLE_STATE_CONNECTED) {
		find_phone_display_connected();
	}else {
		find_phone_display_disconncted();
	}

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

static void find_phone_on_timeout(void* p_context)
{
	displayTimeCount++;
	
	if (displayTimeCount < 50) {
		if (!recivceReqStatus) {
			LOG_DEBUG("[activity_find_phone] find phone reqest restart.\n");
			if (get_device_session() != DEV_SESSION_IDLE) {
				LOG_ERROR("[find_phone], session state=%d.",get_device_session());
			}else{
				find_phone_request(DeviceAlertType_find_phone_restart);
			}
		}
		ry_sched_msg_send(IPC_MSG_TYPE_FINDE_PHONE_CONTINUE, 0, NULL);
	}else {
		displayTimeCount = 0;
		ry_sched_msg_send(IPC_MSG_TYPE_FIND_PHONE_TIMEOUT, 0, NULL);
	}
}

static ry_sts_t find_phone_activity_onCreate(void* usrdata)
{
    ry_timer_parm timerPara;
    uint32_t evt = *((uint32_t*)usrdata);

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    if(p_find_phone_ctx == NULL) {
        LOG_DEBUG("[activity_find_phone] create\n");
        p_find_phone_ctx = ry_malloc(sizeof(activity_find_phone_ctx_t));
        if(p_find_phone_ctx == NULL) {
           return RY_ERR_NO_MEM;
        }

        timerPara.flag = 0;
        timerPara.data = NULL;
        timerPara.tick = 1;
        timerPara.name = "activity_find_phone";

        p_find_phone_ctx->findPhoneFreshTimer = ry_timer_create(&timerPara);
        if(p_find_phone_ctx->findPhoneFreshTimer == 0)
        {
            ry_free(p_find_phone_ctx);
            return RY_ERR_NO_MEM;
        }
        p_find_phone_ctx->freshIndexLast = 50;
        p_find_phone_ctx->freshInterval_ms = 200;
        p_find_phone_ctx->freshIndexNow = 0;
        p_find_phone_ctx->isBackToRoot = false;

        ry_timer_start(p_find_phone_ctx->findPhoneFreshTimer,
                p_find_phone_ctx->freshInterval_ms,
                find_phone_on_timeout,
                NULL);

        wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_find_phone, find_phone_onProcessedTouchEvent);

        find_phone_fresh_frame();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    }else {
        if(evt == IPC_MSG_TYPE_FIND_PHONE_TIMEOUT) {
			ry_sched_msg_send(IPC_MSG_TYPE_FIND_PHONE_STOP, 0, NULL);
            LOG_DEBUG("[wms] find_phone timeout stopped\n");
        } 
		else if(evt == IPC_MSG_TYPE_FINDE_PHONE_CONTINUE) {
            p_find_phone_ctx->freshIndexNow ++;  //fresh counter
			find_phone_fresh_frame();
			ry_timer_start(p_find_phone_ctx->findPhoneFreshTimer,
                p_find_phone_ctx->freshInterval_ms,
                find_phone_on_timeout,
                NULL);
			LOG_DEBUG("[activity_find_phone_continue] count=%d.\n",p_find_phone_ctx->freshIndexNow);
        }
    }

    return RY_SUCC;
}

ry_sts_t find_phone_activity_onDestrory(void)
{
    LOG_DEBUG("[activity_find_phone] Destroy\n");
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_find_phone);
    if(p_find_phone_ctx != NULL)
    {
        if(p_find_phone_ctx->findPhoneFreshTimer != 0)
        {
            ry_timer_stop(p_find_phone_ctx->findPhoneFreshTimer);
            ry_timer_delete(p_find_phone_ctx->findPhoneFreshTimer);
        }
        ry_free(p_find_phone_ctx);
        p_find_phone_ctx = NULL;
    }
    return RY_SUCC;
}

void find_phone_request_cb(ry_sts_t status, void* usrdata)
{
	LOG_DEBUG("[find_phone_request_cb] TX Callback, cmd:%s, status:0x%04x\r\n", (char*)usrdata, status);
	if (status == RY_SUCC) {
		recivceReqStatus = 1;
	}
	return;
}

void find_phone_request(DeviceAlertType alertType)
{
	DeviceAlert findPhoneParam = {0};
	u8_t * buf = NULL;
	findPhoneParam.type = alertType;
	findPhoneParam.timeout_ms = 10000;

	buf = (u8_t *)ry_malloc(sizeof(DeviceAlert) + 10);

	pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t *) buf, sizeof(DeviceAlert) + 10 );

	if (!pb_encode(&stream, DeviceAlert_fields, &findPhoneParam)) {
		LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_ENCODE_FAIL);
	}
	
	recivceReqStatus = 0;
	
	LOG_DEBUG("[setting_find_phone_request] send msg to ble.\r\n");
	ble_send_request(CMD_APP_FIND_PHONE, buf, stream.bytes_written, 1, find_phone_request_cb, (void*)"app_find_phone");
	ry_free(buf);
}
