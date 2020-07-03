/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __GUI_BARE_H__
#define __GUI_BARE_H__

#include <stdint.h>

#define RM_OLED_WIDTH      120
#define RM_OLED_HEIGHT     240
#define RM_OLED_PIX_ALL    RM_OLED_HEIGHT * RM_OLED_WIDTH
#define RM_OLED_IMG_24BIT  (RM_OLED_PIX_ALL * 3 )

#define COLOR_RAW_RED       0x0000ff
#define COLOR_RAW_GREEN     0x00ff00
#define COLOR_RAW_BLUE      0xff0000

/*
 * CONSTANTS
 *******************************************************************************
 */



/*
 * TYPES
 *******************************************************************************
 */
typedef struct {
    uint8_t* buffer;    //- pointer to framebuffer    
    uint32_t x_start;   //- img display posion of start x, should be oven
    uint32_t x_end;     //- img display posion of start y, should be oven
    uint32_t y_start;   //- img display posion of end x, should be odd
    uint32_t y_end;     //- img display posion of end y, should be odd
}img_pos_t;

/*
 * GLOBAL VARIABLES
 *******************************************************************************
 */
extern u8_t* p_which_fb;
extern u8_t* p_draw_fb;
extern u8_t frame_buffer[];
extern u8_t frame_buffer_1[];

/*
 * FUNCTIONS
 *******************************************************************************
 */
/**
 * @brief   gui thread task
 *
 *
 * @return  None
 */
void _gui_thread_entry(void);

void update_buffer_to_oled(void);

void clear_buffer_black(void);

void clear_buffer_white(void);


void clear_frame(uint32_t y_start, uint32_t y_len);

void move_buffer_data_Y(int y);

uint8_t _touch_get(void);

extern uint8_t frame_buffer[RM_OLED_IMG_24BIT];

void _gui_blit_area(void);

/**
 * @brief   API to write framebuffer data to oled
 *
 * @param   pos: image position info and buffer source, refer to img_pos_t define
 * 
 * @return  None
 */
void write_buffer_to_oled(img_pos_t* pos);
void clear_frame_area(uint32_t x_start, uint32_t x_end,uint32_t y_start, uint32_t y_end);
void set_frame_area(uint32_t x_start, uint32_t x_end,uint32_t y_start, uint32_t y_end, u8_t color);

void set_frame(uint32_t y_start, uint32_t y_len ,u8_t color);
uint32_t gui_bare_get_pixel_color(uint32_t x, uint32_t y, uint32_t* p_color);
uint32_t gui_bare_set_pixel_color(uint32_t x, uint32_t y, uint32_t color);


#endif //__GUI_BARE_H__
