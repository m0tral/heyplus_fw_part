
#include <stdbool.h>
#include "activity_loading.h"
#include "gfx.h"
#include "gui.h"
#include "gui_img.h"
#include "gui_bare.h"
#include "ry_font.h"
#include "touch.h"
#include "ryos_timer.h"


#define LOADING_POSITION_X          44
#define LOADING_POSITION_Y          74

#define LOADING_STEPS               6

#define LOADING_STR_P_Y             156
#define FONTS_RY_CUSTOMIZE          0

typedef struct
{
    uint32_t loading_fresh_timer;
    uint32_t fresh_interval_ms;
    uint32_t fresh_index_last;
    uint32_t fresh_index_now;
    bool isBackToRoot;
}activity_loading_ctx_t;

typedef struct
{
    uint8_t x;
    uint8_t y;
}loading_process_ctrl_pixel_t;

extern  uint8_t const loading_1_bmp[];

static const loading_process_ctrl_pixel_t loading_img_pos[LOADING_STEPS] = 
{
    {12,1},
    {21,6},
    {21,16},
    {12,22},
    {3,16},
    {3,6},
};

static char const* const loading_text[LOADING_STEPS] = 
{
		"Загрузка.",
		"Загрузка..",
		"Загрузка...",
		"Загрузка.",
		"Загрузка..",
		"Загрузка...",
};

extern const uint32_t loading_icon_color[LOADING_STEPS] ;

static activity_loading_ctx_t* p_loading_ctx = NULL;

static const raw_png_descript_t icon_loading = 
{
    .p_data = loading_1_bmp,
    .width = 8,
    .height = 8,
    .fgcolor = White,
};


static int loading_surface_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[activity_loading] click\n");
            break;
        default:
            break;
    }

    return consumed;
}

static void loading_surface_fresh_frame(void)
{
    uint32_t now_index = p_loading_ctx->fresh_index_now % LOADING_STEPS;
    uint32_t x,y;
    uint32_t pic_roll_index;

    clear_buffer_black();
    gdispFillString(10, 
                    LOADING_STR_P_Y, 
                    (char*)loading_text[now_index], 
                    (void*)font_roboto_20.font, 
                    White,
                    Black
                    );
                    
    for(int i=0; i<LOADING_STEPS; i++) {
        pic_roll_index = (now_index+i)%LOADING_STEPS;
        x = loading_img_pos[pic_roll_index].x + LOADING_POSITION_X;
        y = loading_img_pos[pic_roll_index].y + LOADING_POSITION_Y;

        draw_raw_png_to_framebuffer(&icon_loading, x,y, loading_icon_color[i]);
    }
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

static void loading_surface_on_timeout(void* p_context)
{
//    LOG_DEBUG("[activity_loading] timeout\n");
    wms_msg_send(IPC_MSG_TYPE_LOADING_TIMER, 0, NULL);
}

static ry_sts_t loading_activity_onCreate(void* usrdata)
{
    ry_timer_parm timer_para;
    uint32_t evt = *((uint32_t*)usrdata);

    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }


    if(p_loading_ctx == NULL) {
        LOG_DEBUG("[activity_loading] create\n");
        p_loading_ctx = ry_malloc(sizeof(activity_loading_ctx_t));
        if(p_loading_ctx == NULL) {
           return RY_ERR_NO_MEM;
        }

        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "activity_loading";

        p_loading_ctx->loading_fresh_timer = ry_timer_create(&timer_para);
        if(p_loading_ctx->loading_fresh_timer == 0)
        {
            ry_free(p_loading_ctx);
            return RY_ERR_NO_MEM;
        }
        p_loading_ctx->fresh_index_last = 30;
        p_loading_ctx->fresh_interval_ms = 1000;
        p_loading_ctx->fresh_index_now = 0;
        p_loading_ctx->isBackToRoot = false;

        ry_timer_start(p_loading_ctx->loading_fresh_timer,
                p_loading_ctx->fresh_interval_ms,
                loading_surface_on_timeout,
                NULL);

        wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_loading, loading_surface_onProcessedTouchEvent);

        loading_surface_fresh_frame();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);
    }else {
        if(evt == IPC_MSG_TYPE_LOADING_TIMER) {
            p_loading_ctx->fresh_index_now ++;
            if(p_loading_ctx->fresh_index_now > p_loading_ctx->fresh_index_last) {
                if(p_loading_ctx->isBackToRoot) {
                    wms_activity_back_to_root(NULL);
                } else {
                    wms_activity_back(NULL);
                    LOG_DEBUG("[wms] loading timeout stopped\n");
                }
            } else  {
                loading_surface_fresh_frame();
//                LOG_DEBUG("[activity_loading] freshed\n");
                ry_timer_start(p_loading_ctx->loading_fresh_timer,
                        p_loading_ctx->fresh_interval_ms,
                        loading_surface_on_timeout,
                        NULL);
            }
        } else if(evt == IPC_MSG_TYPE_LOADING_CONTINUE) {
            p_loading_ctx->fresh_index_now = p_loading_ctx->fresh_index_now % LOADING_STEPS;  //fresh counter
        }
    }

    return RY_SUCC;
}

ry_sts_t loading_activity_onDestrory(void)
{
    LOG_DEBUG("[activity_loading] Destroy\n");
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_loading);
    if(p_loading_ctx != NULL)
    {
        if(p_loading_ctx->loading_fresh_timer != 0)
        {
            ry_timer_stop(p_loading_ctx->loading_fresh_timer);
            ry_timer_delete(p_loading_ctx->loading_fresh_timer);
        }
        ry_free(p_loading_ctx);
        p_loading_ctx = NULL;
    }
    return RY_SUCC;
}


activity_t activity_loading = {\
    .name      = "loading",\
    .onCreate  = loading_activity_onCreate,\
    .onDestroy = loading_activity_onDestrory,\
    .disableUntouchTimer = 1,\
    .priority = WM_PRIORITY_H,\
    .brightness = 100,\
};


ry_sts_t loading_activity_start(void* p_arg)
{
    wms_msg_send(IPC_MSG_TYPE_LOADING_START, 0, NULL);
    return RY_SUCC;
}


ry_sts_t loading_activity_update(void* p_arg)
{
    wms_msg_send(IPC_MSG_TYPE_LOADING_CONTINUE, 0, NULL);
    return RY_SUCC;
}

ry_sts_t loading_activity_stop(void* p_arg)
{
    wms_msg_send(IPC_MSG_TYPE_LOADING_STOP, 0, NULL);
    return RY_SUCC;
}
