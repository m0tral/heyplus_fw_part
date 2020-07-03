/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#ifndef GFX_USE_GDISP
#define GFX_USE_GDISP				1
#endif

#ifndef GDISP_HARDWARE_STREAM_WRITE
#define GDISP_HARDWARE_STREAM_WRITE	1
#endif

#ifndef GDISP_HARDWARE_BITFILLS
#define GDISP_HARDWARE_BITFILLS		1
#endif

#if GFX_USE_GDISP

#define GDISP_DRIVER_VMT			GDISPVMT_RM67162
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

/*===========================================================================*/
/* Driver board                                                   */
/*===========================================================================*/

#include "board_rm67162.h"
#include "ry_hal_inc.h"
#include "ry_utils.h"
#include "am_mcu_apollo.h"
#include "gui_bare.h"
#include "gui_img.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/


#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#include "lld_rm67162.h"

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define dummy_read(g)				{ volatile uint16_t dummy; dummy = read_data(g); (void) dummy; }
#define write_reg(g, reg, data)		{ write_cmd(g, reg); write_data(g, data); }

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/
extern uint8_t* p_which_fb;

LLDSPEC bool_t gdisp_lld_init(GDisplay *g) {
	// No private area for this controller
	g->priv = 0;

	// Initialise the board interface
	init_board(g);
	
	/* Turn on the back-light */
	set_backlight(g, GDISP_INITIAL_BACKLIGHT);

	/* Initialise the GDISP structure */
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = GDISP_ROTATE_0;
	g->g.Powermode = powerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;
	//GDISPVMT_OnlyOne->flush(g);   						//flush
	return TRUE;
}

#if GDISP_HARDWARE_STREAM_WRITE
	static uint32_t color_pix_len = 0;
	static uint32_t color_pix_idx = 0;
	LLDSPEC	void gdisp_lld_write_start(GDisplay *g) {
		acquire_bus(g);
		// LOG_DEBUG("writer start: %d %d %d %d\n", g->p.x, g->p.y, g->p.cx, g->p.cy);		
		color_pix_len = (g->p.x + g->p.cx * g->p.y);
		color_pix_idx = 0;
		// am_devices_rm67162_display_range_set\
				( g->p.x, g->p.x + g->p.cx - 1, g->p.y, g->p.y + g->p.cy - 1);
		// am_devices_rm67162_data_stream_enable();
	}

	LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
		LLDCOLOR_TYPE	c;
		c = gdispColor2Native(g->p.color);
		uint32_t pos = (g->p.x + 120 * g->p.y);
		#define buffer_pix (pos + color_pix_idx)
		// LOG_DEBUG("writer color: %d %d %d %d %d %x %x %x \n", g->p.x, g->p.y, g->p.cx, g->p.cy,\
				  buffer_pix, (c >> 16) & 0xFF, (c >> 8) & 0xFF, (c >> 0) & 0xFF);
#if 0		
		uint8_t buf[4] = {0xff, 0xff, 0xff, 0xff};
		buf[0] = (c >> 16) & 0xFF;
		buf[1] = (c >> 8) & 0xFF;
		buf[2] = (c >> 0) & 0xFF;
		
		am_devices_rm67162_data_stream_888(buf, 3);
#else
		p_which_fb[((buffer_pix<<1) + buffer_pix) + 0] = (c >> 16) & 0xFF;
		p_which_fb[((buffer_pix<<1) + buffer_pix) + 1] = (c >> 8) & 0xFF;
		p_which_fb[((buffer_pix<<1) + buffer_pix) + 2] = (c >> 0)& 0xFF;
		true_color_to_white_black(p_which_fb + buffer_pix * 3);
		if (color_pix_len--)
			color_pix_idx ++;
#endif
	}
	
	LLDSPEC	void gdisp_lld_write_stop(GDisplay *g) {
		release_bus(g);
	}
#endif

#if GDISP_HARDWARE_BITFILLS
	static void oled_set_area(uint8_t x, uint8_t y, uint8_t c_x, uint8_t c_y)
	{
		
	}

	void gdisp_lld_blit_area(GDisplay *g){
		uint32_t* p = g->p.ptr;	
		uint32_t pix_len;
		uint32_t pix_start;	
		uint32_t i;	
        uint8_t *pp;
		if (g->p.formate == GDISP_IMAGE_TYPE_NATIVE || g->p.formate == GDISP_IMAGE_TYPE_BMP){
			if ( (g->p.y >= 240))
				return;
			pix_len = g->p.cx * g->p.cy * 3;	
			pix_start = (g->p.x + g->p.y * 120) * 3;
			g->p.buffer_addr = 0;
			 //LOG_DEBUG("a: start_x: %d, start_y: %d, count_x: %d, count_y: %d, i: %d, pix_start: %d, pix_len: %d \r\n", \
				g->p.x, g->p.y, g->p.cx, g->p.cy, 0, g->p.buffer_addr, pix_len);		
			if (g->p.buffer_addr + pix_start + pix_len >= sizeof(frame_buffer)){
				pix_len = sizeof(frame_buffer) - g->p.buffer_addr - pix_start;	
			}
			if (pix_len && (g->p.y < 240)){
				 //LOG_DEBUG("_new: start_x: %d, start_y: %d, count_x: %d, count_y: %d, i: %d, pix_start: %d, pix_len: %d \r\n", \
					g->p.x, g->p.y, g->p.cx, g->p.cy, 0, g->p.buffer_addr, pix_len);
				extern int gui_img_offset;
				ry_memcpy(p_which_fb + g->p.buffer_addr + gui_img_offset * 360 + pix_start, p, pix_len);

				pp = p_which_fb + g->p.buffer_addr + gui_img_offset * 360 + pix_start;
				for ( i = 0; i < pix_len; i=i+3) {
					true_color_to_white_black(pp + i);
				}
				g->p.buffer_addr += 360;
			}
		}//end of formate native, else: png jpg and so on
		else if (g->p.formate == GDISP_IMAGE_TYPE_JPG){
			pix_len = g->p.cx * g->p.cy;
			pix_start =  g->p.x + g->p.y * 120;
			for (int line = 0; line < g->p.cy; line ++){
				for (i = pix_start; i < g->p.cx; i++){
					uint32_t pos = line * 120 + i;
					p_which_fb[((pos<<1) + pos) + 0] =  (*p >> 16) & 0xFF;
					p_which_fb[((pos<<1) + pos) + 1] =  (*p >> 8) & 0xFF;
					p_which_fb[((pos<<1) + pos) + 2] =  *p & 0xFF;
					true_color_to_white_black(p_which_fb + pos * 3);
					*p++;
				}
			}
		}
		else{
			pix_len = g->p.cx * g->p.cy;
			pix_start =  g->p.x + g->p.y * 120;
			for ( i = pix_start; (pix_start + pix_len < RM_OLED_PIX_ALL) && (i < (pix_start + pix_len)); i++){
				p_which_fb[((i<<1) + i) + 0] =  (*p >> 16) & 0xFF;
				p_which_fb[((i<<1) + i) + 1] = (*p >> 8) & 0xFF;
				p_which_fb[((i<<1) + i) + 2] = *p & 0xFF;
				true_color_to_white_black(p_which_fb + i * 3);
				*p++;
			}
		}
		// LOG_DEBUG("writer blit_area: %d %d %d %d %d %d %d  %d %d \n", \
		i, g->p.x, g->p.y, g->p.cx, g->p.cy, g->p.x + g->p.cx - 1,  g->p.y + g->p.cy - 1, pix_start * 3, pix_len * 3);		
	}
#endif

#if GDISP_HARDWARE_FLUSH
	LLDSPEC void gdisp_lld_flush(GDisplay *g) {
		update_buffer_to_oled();
	}
#endif

#if GDISP_HARDWARE_CLEARS
	LLDSPEC	void gdisp_lld_clear(GDisplay *g) {
		clear_buffer_black();
		// update_buffer_to_oled();		
	}
#endif

#if GDISP_HARDWARE_STREAM_READ
	#error "SSD1357 - Stream Read is not supported yet"
	LLDSPEC	void gdisp_lld_read_start(GDisplay *g) {
		acquire_bus(g);
		//set_viewport(g);
		//write_index(g, 0x2E);
		setreadmode(g);
		//dummy_read(g);
	}
	LLDSPEC	color_t gdisp_lld_read_color(GDisplay *g) {
		uint16_t	data;

		data = read_data(g);
		return gdispNative2Color(data);
	}
	LLDSPEC	void gdisp_lld_read_stop(GDisplay *g) {
		setwritemode(g);
		release_bus(g);
	}
#endif


#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
	#error "SSD1357 - Hardware control is not supported yet"
	LLDSPEC void gdisp_lld_control(GDisplay *g) {
		switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (powermode_t)g->p.ptr)
				return;
			switch((powermode_t)g->p.ptr) {
			case powerOff:
			case powerSleep:
			case powerDeepSleep:
				acquire_bus(g);
				//TODO
				release_bus(g);
				break;
			case powerOn:
				acquire_bus(g);
				//TODO
				release_bus(g);
				break;
			default:
				return;
			}
			g->g.Powermode = (powermode_t)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (orientation_t)g->p.ptr)
				return;
			switch((orientation_t)g->p.ptr) {
			case GDISP_ROTATE_0:
				acquire_bus(g);
				//TODO
				release_bus(g);
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_90:
				acquire_bus(g);
				//TODO
				release_bus(g);
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			case GDISP_ROTATE_180:
				acquire_bus(g);
				//TODO
				release_bus(g);
				g->g.Height = GDISP_SCREEN_HEIGHT;
				g->g.Width = GDISP_SCREEN_WIDTH;
				break;
			case GDISP_ROTATE_270:
				acquire_bus(g);
				//TODO
				release_bus(g);
				g->g.Height = GDISP_SCREEN_WIDTH;
				g->g.Width = GDISP_SCREEN_HEIGHT;
				break;
			default:
				return;
			}
			g->g.Orientation = (orientation_t)g->p.ptr;
			return;

        case GDISP_CONTROL_BACKLIGHT:
            if ((unsigned)g->p.ptr > 100)
            	g->p.ptr = (void *)100;
            set_backlight(g, (unsigned)g->p.ptr);
            g->g.Backlight = (unsigned)g->p.ptr;
            return;

		//case GDISP_CONTROL_CONTRAST:
        default:
            return;
		}
	}
#endif

#endif /* GFX_USE_GDISP */
