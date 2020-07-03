#include "ry_font.h"

#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "ry_mcu_config.h"

/* Board */
#include "board.h"
#include "ry_hal_inc.h"

#include "ry_gsensor.h"
#include "ry_gui.h"
#include "gui_bare.h"
#include "gfx.h"
#include "string.h"
#include "gui_img.h"
#include "stdio.h"
#include "ryos_timer.h"
#include "scheduler.h"
#include "gui.h"
#include "device_management_service.h"


#define LOADING_IMG_MAX_SIZE		2048

#define LOADING_POSITION_X          44
#define LOADING_POSITION_Y          74

#define LOADING_STR_P_Y             156
#define LOADING_STEPS               6


typedef struct {
	int16_t x;
	int16_t y;
	int16_t height;
	int16_t width;
	u8_t * data;
}loading_point_info_t;

typedef struct {
    int16_t count;
    int loading_refresh_timer;
    
}loading_info_t;

static loading_info_t loading_info = {0};


loading_point_info_t point_info[] = {{12,1}, {21,6}, {21,16}, {12,22}, {3,16}, {3,6}};

u8_t * loading_img_name[] = {"loading_1.bmp", "loading_2.bmp", "loading_3.bmp", "loading_4.bmp", "loading_5.bmp", "loading_6.bmp"};


uint8_t const loading_1_bmp[] = {
0, 29, 166, 230, 230, 166, 29, 0, 
29, 237, 255, 255, 255, 255, 237, 29, 
166, 255, 255, 255, 255, 255, 255, 166, 
230, 255, 255, 255, 255, 255, 255, 230, 
230, 255, 255, 255, 255, 255, 255, 230, 
166, 255, 255, 255, 255, 255, 255, 166, 
29, 237, 255, 255, 255, 255, 237, 29, 
0, 29, 166, 230, 230, 166, 29, 0, 
};

uint8_t const loading_color[] = {
    0xDB,
    0xC5,
    0xAF,
    0x98,
    0x82
};


const uint32_t loading_icon_color[LOADING_STEPS] = 
{
    HTML2COLOR(0xDBDBDB),
    HTML2COLOR(0xC5C5C5),
    HTML2COLOR(0xAFAFAF),
    HTML2COLOR(0x989898),
    HTML2COLOR(0x828282),
    HTML2COLOR(0xACACAC),
};


void deinit_loading_img(void);
void refresh_loading(int x, int y, int disp_frame);
void loading_timer_handle(void* arg);
void deinit_loading_img(void);
extern void gdisp_update(void);



void show_loading(int x, int y, int level)
{
}


void loading_timer_start(void)
{
    ry_timer_parm timer_para;
    if (loading_info.loading_refresh_timer == 0) {
        timer_para.timeout_CB = loading_timer_handle;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "load_tmr";
        loading_info.loading_refresh_timer = ry_timer_create(&timer_para);

        if (loading_info.loading_refresh_timer) {
            ry_timer_start(loading_info.loading_refresh_timer, 100, loading_timer_handle, NULL);
        }
    }
}

void loading_timer_stop(void)
{
    if(loading_info.loading_refresh_timer != 0){
	    ry_timer_delete(loading_info.loading_refresh_timer);
	    loading_info.loading_refresh_timer = 0;
	}
}

int  init_loading_img(int x, int y)
{
	int i = 0, j = 0;
	FIL * fp = (FIL *)ry_malloc(sizeof(FIL));
	int img_size = 0;
	FRESULT ret = FR_OK;
	u32_t bytes = 0, local_x,local_y;
	
	if(fp == NULL){
		return -1;
	}

	loading_info.count = 0;
	if(loading_info.loading_refresh_timer != 0){
	    ry_timer_delete(loading_info.loading_refresh_timer);
	    loading_info.loading_refresh_timer = 0;
	}
	
	for (i = 0; i < sizeof(point_info)/sizeof(loading_point_info_t); i++){
		/*ret = f_open(fp, (const char *)loading_img_name[i], FA_READ);
		
		if(ret != FR_OK){
			goto error;
		}*/
		
		img_size = sizeof(loading_1_bmp);
		
		if(img_size == 0){
			goto error;
		}else if(img_size > LOADING_IMG_MAX_SIZE){
			goto error;
		}
		
		point_info[i].data = (u8_t *)ry_malloc(img_size * 3);
		
		if(point_info[i].data == NULL){
			goto error;
		}
		
		//f_read(fp, point_info[i].data, img_size, &bytes);
		ry_memcpy(point_info[i].data, loading_1_bmp, img_size);

		/*if((i != 0)&& (i < (sizeof(loading_color)/sizeof(uint8_t) + 1 )) ){
            for( j = 0; j < img_size; j++){
                int temp = point_info[i].data[j];

                point_info[i].data[j] = (u8_t)((temp * loading_color[i - 1] / 0xFF)&0xff);
            }
		}*/


        local_x = x + point_info[i].x;
        local_y = y + point_info[i].y;
		//img_exflash_to_framebuffer(loading_img_name[i], local_x, local_y, (uint8_t*)&point_info[i].width, (uint8_t*)&point_info[i].height, d_justify_left);
		
	}

    ry_free(fp);

	return 0;

error:
    ry_free(fp);
    deinit_loading_img();
    return -1;
}

const char* const text_Loading = "Загрузка";

void create_loading(int animation)
{
    
    if(animation){
        init_loading_img(LOADING_POSITION_X, LOADING_POSITION_Y);
        loading_timer_start();
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    }else{
        clear_buffer_black();
        init_loading_img(LOADING_POSITION_X, LOADING_POSITION_Y);
        gdispFillStringBox(0, 
                        LOADING_STR_P_Y, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)text_Loading, 
                        (void*)font_roboto_20.font, 
                        White,
                        Black,
                        justifyCenter
                    );  
        gdisp_update();
    }    
}

void distory_loading(void)
{
    deinit_loading_img();
}

void loading_timer_handle(void* arg)
{
    LOG_DEBUG("[%s]\n", __FUNCTION__);
    clear_buffer_black();
    refresh_loading(LOADING_POSITION_X, LOADING_POSITION_Y, loading_info.count);

    gdispFillStringBox(0, 
                        LOADING_STR_P_Y, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char*)text_Loading, 
                        (void*)font_roboto_20.font, 
                        White,
                        Black,
                        justifyCenter
                    );  
                        
    loading_info.count++;
    if (loading_info.loading_refresh_timer) {
        ry_timer_start(loading_info.loading_refresh_timer, 100, loading_timer_handle, NULL);
    }
    
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

void loading_refresh_handle(void)
{

    if (device_get_powerOn_mode() == 0){
        clear_buffer_black();
        refresh_loading(LOADING_POSITION_X, LOADING_POSITION_Y, loading_info.count);
        loading_info.count++;
    
        gdispFillStringBox(0, 
                            LOADING_STR_P_Y, 
                            font_roboto_20.width,
                            font_roboto_20.height,
                            (char*)text_Loading, 
                            (void*)font_roboto_20.font, 
                            White,
                            Black,
                            justifyCenter
                        );  
        gdisp_update();
    }
}


void refresh_loading(int x, int y, int disp_frame)
{
    u8_t point_num = ( sizeof(point_info)/sizeof(loading_point_info_t) );
	int temp = disp_frame % point_num;
	int i = 0, local_x, local_y;
    int cur_line = 0;

	
	for (i = 0; i < sizeof(point_info)/sizeof(loading_point_info_t); i++){

	    if(point_info[(i + temp)%point_num].data == NULL){
            break;
	    }
        local_x = x + point_info[(i + temp)%point_num].x;
        local_y = y + point_info[(i + temp)%point_num].y;

    #if 0
        for(cur_line = 0; cur_line < point_info[(i )%point_num].height; cur_line++){
            u8_t *dst_buf = (u8_t *)&frame_buffer[((local_y + cur_line) * IMG_MAX_WIDTH + local_x)*3];
            u8_t *src_buf = point_info[(i)%point_num].data + IMG_HEAD_LEN + cur_line * point_info[(i)%point_num].width * 3;
            /*if((i)%point_num == 0){
                ry_memset(dst_buf, 0xFF, point_info[(i)%point_num].width * 3);
            }else if((i)%point_num == 1){
                ry_memset(dst_buf, 0x43, point_info[(i)%point_num].width * 3);
            }else*/{
                ry_memcpy(dst_buf, src_buf, point_info[(i)%point_num].width * 3);
            }
        }
    #endif
    
        raw_png_descript_t info;
        info.p_data = point_info[(i)%point_num].data;
        info.width = 8;
        info.height = 8;
        info.fgcolor = White;

        draw_raw_png_to_framebuffer(&info, local_x,local_y, loading_icon_color[i]);
	}
}

void deinit_loading_img(void)
{
	int i = 0;
	for (i = 0; i < sizeof(point_info)/sizeof(loading_point_info_t); i++){
		ry_free(point_info[i].data);
		point_info[i].data = NULL;
		point_info[i].height = 0;
		point_info[i].width = 0;		
	}

	loading_info.count = 0;
	if(loading_info.loading_refresh_timer != 0){
	    ry_timer_delete(loading_info.loading_refresh_timer);
	    loading_info.loading_refresh_timer = 0;
	}
}


