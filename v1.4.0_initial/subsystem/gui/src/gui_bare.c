/*
* Copyright (c), Ryeex Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
//#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "ry_mcu_config.h"
#include "app_config.h"

/* Board */
#include "board.h"
#include "ry_hal_inc.h"

#include "ry_gsensor.h"
#include "ry_gui.h"
#include "am_hal_sysctrl.h"
#include "am_devices_rm67162.h"
#include "am_devices_cy8c4014.h"
#include "am_bsp_gpio.h"

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "gui_bare.h"
#include "hrm.h"
#include "gfx.h"


extern const unsigned char gImage_circle[];
extern ryos_mutex_t* fb_mutex;

#if 0
extern const unsigned char gImage_rgb24_shuiping120_240[];
extern const unsigned char gImage_flashing_raw[];
extern const unsigned char gImage_lighting[];
extern const unsigned char gImage_moon[];
extern const unsigned char gImage_sunning[];
extern const unsigned char gImage_circlebig[];
extern const unsigned char gImage_circlesmall[];
extern const unsigned char gImage_city[];
extern const unsigned char gImage_ipple[];
extern const unsigned char gImage_ball[];
#endif
uint8_t frame_buffer[RM_OLED_IMG_24BIT];
#if ((RY_UI_EFFECT_MOVE == TRUE)||(RY_UI_EFFECT_TAP_MOVE == TRUE))
uint8_t frame_buffer_1[RM_OLED_IMG_24BIT>>1];
#else
uint8_t frame_buffer_1[1];
#endif
uint8_t *p_draw_fb  = frame_buffer;
uint8_t *p_which_fb = frame_buffer;

const uint32_t color_coding[] = {
	//0x8410, // gray
    //0x4208,
    0xff0000, // blue at brg mode
    0x00ff00, // green
    0x0000ff, // red at brg mode
    0x000000, // black
    0x00ffff, // cyan
    0xffff00, // yellow
    0xff00ff, // pink
    0x102010,
    0xffffff, // white
};

const char* disp_img[] =
{
    (const char*)gImage_circle,
};

uint8_t image_index = 0;
uint8_t color_index = 0;
void _gui_thread_entry(void)
{
    ry_sts_t status = RY_SUCC;
    LOG_DEBUG("[gui_thread_entry] enter \r\n"); 

    //cy8c4011_gpio_init();       //initial TP and enable gpio interrupt

    rm67162_gpio_init();
 
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_OLED, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_OLED);   
#endif
    rm67162_init_1();

#if 0
    // am_devices_rm67162_display_range_set(0, (RM_OLED_WIDTH - 1), 0, (RM_OLED_HEIGHT -1));    
    // am_devices_rm67162_data_write_888(disp_img[image_index], RM_OLED_IMG_24BIT);
    //memcpy(frame_buffer, disp_img[image_index], RM_OLED_IMG_24BIT);
	
	  p_draw_fb = gImage_demo_menu_01;
    update_buffer_to_oled();
#endif    
    // _gui_blit_area();
    while(0)
    {
        int i;
        for ( i = 0; i < 240; i += 8){
            ry_memset(frame_buffer, 0, sizeof(frame_buffer));
            uint32_t offset = i * 360;
            ry_memcpy(frame_buffer + offset, disp_img[image_index], RM_OLED_IMG_24BIT - offset);
            am_devices_rm67162_data_write_888(frame_buffer, RM_OLED_IMG_24BIT);
        }
    }
}


/**
 * @brief   API to write framebuffer data to oled
 *
 * @param   pos: image position info and buffer source, refer to img_pos_t define
 * 
 * @return  None
 */
void write_buffer_to_oled(img_pos_t* pos)
{    
    extern ryos_mutex_t* fb_mutex;
    uint32_t len = 0;
    // LOG_DEBUG("[write_buffer_to_oled]start, x_start: %d, x_end: %d, y_start: %d, y_end %d, buffer:0x%08X\r\n", \
        pos->x_start, pos->x_end, pos->y_start, pos->y_end, pos->buffer);
	if ((pos->x_end - pos->x_start < 2) || (pos->y_end - pos->y_start < 2)){
		goto exit;
	}
	pos->x_start = (pos->x_start & 1) ? (pos->x_start - 1) : pos->x_start;
	pos->y_start = (pos->y_start & 1) ? (pos->y_start - 1) : pos->y_start;
	pos->x_end = (pos->x_end & 1) ? pos->x_end : (pos->x_end + 1);
	pos->y_end = (pos->y_end & 1) ? pos->y_end : (pos->y_end + 1);
	
	len = (pos->x_end - pos->x_start + 1) * (pos->y_end - pos->y_start + 1) * 3;

    // only display 120 * 240 oled now
    if ((pos->x_end > RM_OLED_WIDTH) || (pos->x_start > RM_OLED_WIDTH) || \
        (pos->y_end > RM_OLED_HEIGHT) || (pos->y_start > RM_OLED_HEIGHT) || \
        ((len + pos->x_start + pos->y_start * RM_OLED_WIDTH * 3) > RM_OLED_IMG_24BIT)\
       ){
		goto exit;
    }

    ryos_mutex_lock(fb_mutex);
    am_devices_rm67162_display_range_set(pos->x_start, pos->x_end, pos->y_start, pos->y_end);
    am_devices_rm67162_data_write_888(pos->buffer, len);
    ryos_mutex_unlock(fb_mutex);
    // LOG_DEBUG("[write_buffer_to_oled]end\r\n");

exit:
    return;
}

void update_buffer_to_oled(void)
{    
    
    //LOG_DEBUG("fb up s\r\n");
    ryos_mutex_lock(fb_mutex);
    am_devices_rm67162_display_range_set(0, (RM_OLED_WIDTH - 1), 0, (RM_OLED_HEIGHT -1));
    am_devices_rm67162_data_write_888(p_draw_fb, RM_OLED_IMG_24BIT);
    ryos_mutex_unlock(fb_mutex);
    //LOG_DEBUG("fb up e\r\n");
}

void clear_buffer_black(void)
{
    ryos_mutex_lock(fb_mutex);
    ry_memset(p_which_fb, 0, sizeof(frame_buffer));
    ryos_mutex_unlock(fb_mutex);
}

void clear_buffer_white(void)
{
    ryos_mutex_lock(fb_mutex);
    ry_memset(p_which_fb, 0xFF, sizeof(frame_buffer));
    ryos_mutex_unlock(fb_mutex);
}

void move_buffer_data_Y(int y)
{
    if((y >= RM_OLED_HEIGHT) || (y <= -RM_OLED_HEIGHT)){
        return ;
    }

    ryos_mutex_lock(fb_mutex);
    
    if(y > 0){
        ry_memcpy(&frame_buffer[y * 360], &frame_buffer[0], ((RM_OLED_HEIGHT -1 - y) *360));
    }else if(y < 0){
        ry_memcpy(&frame_buffer[0], &frame_buffer[(-y) * 360],((RM_OLED_HEIGHT -1 + y) *360));
    }

    ryos_mutex_unlock(fb_mutex);
}

void clear_frame(uint32_t y_start, uint32_t y_len)
{
    ryos_mutex_lock(fb_mutex);
    ry_memset(p_which_fb + y_start * 360, 0, y_len * 360);
    ryos_mutex_unlock(fb_mutex);
}

void set_frame(uint32_t y_start, uint32_t y_len ,u8_t color)
{
    ryos_mutex_lock(fb_mutex);
    ry_memset(p_which_fb + y_start * 360, color, y_len * 360);
    ryos_mutex_unlock(fb_mutex);
}


void clear_frame_area(uint32_t x_start, uint32_t x_end,uint32_t y_start, uint32_t y_end)
{
    int i = 0;

    for(i = y_start; i < y_end; i++){
        ry_memset(&frame_buffer[i * 360 + x_start*3], 0, (x_end - x_start)*3);
    }
}

void set_frame_area(uint32_t x_start, uint32_t x_end,uint32_t y_start, uint32_t y_end, u8_t color)
{
    int i = 0;

    for(i = y_start; i < y_end; i++){
        ry_memset(&frame_buffer[i * 360 + x_start*3], color, (x_end - x_start)*3);
    }
}

void _gui_blit_area(void)
{
    uint32_t x_start = 0;
    uint32_t y_start = 0;
    uint32_t x_cnt = 2;
    uint32_t y_cnt = 2;

    uint8_t* buffer = NULL;
    uint32_t pix_len = x_cnt * y_cnt * 3;
    uint32_t x_range;
    uint32_t y_range;
    buffer = ry_malloc(pix_len);
    uint32_t y;
    while(1){
        for(x_start = 60; x_start <= 61; x_start += 2){
            for (y = 60; y < 61; y += y_cnt){
                y_start = y;
                ry_memset(buffer, 0xff, pix_len);

                x_range = x_start + x_cnt - 1;
                y_range = y_start + y_cnt - 1;
                
                LOG_DEBUG("[gui_thread] range: %d %d %d %d\r\n", x_start, x_range, y_start, y_range);
                
                am_devices_rm67162_display_range_set(x_start, x_range, y_start, y_range);
                am_devices_rm67162_data_stream_enable();

                am_devices_rm67162_data_stream_888(buffer, pix_len >> 1);
                am_devices_rm67162_data_stream_continue();
                am_devices_rm67162_data_stream_888(buffer + (pix_len >> 1), (pix_len >> 1));

                am_util_delay_ms(10);
                // memset(frame_buffer, 0, sizeof(frame_buffer));
                // update_buffer_to_oled();
            }
        }
    }
}

#define FRAME2COLOR(p)      (RGB2COLOR_R(p[2])|RGB2COLOR_G(p[1])|RGB2COLOR_B(p[0]))

uint32_t gui_bare_get_pixel_color(uint32_t x, uint32_t y, uint32_t* p_color)
{
    uint32_t status = RY_SUCC;

    if((x > RM_OLED_WIDTH)&&(y > RM_OLED_HEIGHT)) {
        return RY_ERR_INVALIC_PARA;
    }

    uint32_t pixel_offset = y*RM_OLED_WIDTH + x;

    uint8_t* p_buffer = &frame_buffer[pixel_offset*3];

    *p_color = FRAME2COLOR(p_buffer);

    return status;
}

uint32_t gui_bare_set_pixel_color(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t status = RY_SUCC;

    if((x > RM_OLED_WIDTH)&&(y > RM_OLED_HEIGHT)) {
        return RY_ERR_INVALIC_PARA;
    }

    uint32_t pixel_offset = y*RM_OLED_WIDTH + x;


    uint8_t* p_buffer = &frame_buffer[pixel_offset*3];

    p_buffer[0] = BLUE_OF(color);
    p_buffer[1] = GREEN_OF(color);
    p_buffer[2] = RED_OF(color);

    return status;

}



