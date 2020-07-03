/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_HEIGHT) || defined(GDISP_SCREEN_HEIGHT)
	#if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
		#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
		COMPILER_WARNING("GDISP: This low level driver does not support setting a screen size. It is being ignored.")
	#endif
	#undef GDISP_SCREEN_WIDTH
	#undef GDISP_SCREEN_HEIGHT
#endif

#define GDISP_DRIVER_VMT			GDISPVMT_SSD1357
#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

/*===========================================================================*/
/* Driver board                                                   */
/*===========================================================================*/
extern void oled_ssd1357_write_cmd(uint8_t cmd);
extern void oled_ssd1357_write_data(uint8_t data);
extern void oled_ssd1357_write_data_buff(uint8_t *buff, uint32_t len);
#define OLED_X_OFFSET			   28

#include "board_SSD1357.h"
#include "ry_hal_inc.h"
#include "ry_utils.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/
jk
#ifndef GDISP_SCREEN_HEIGHT
	#define GDISP_SCREEN_HEIGHT		240
#endif
#ifndef GDISP_SCREEN_WIDTH
	#define GDISP_SCREEN_WIDTH		120
#endif
#ifndef GDISP_INITIAL_CONTRAST
	#define GDISP_INITIAL_CONTRAST	100
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
	#define GDISP_INITIAL_BACKLIGHT	100
#endif

#include "SSD1357.h"

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some common routines and macros
#define dummy_read(g)				{ volatile uint16_t dummy; dummy = read_data(g); (void) dummy; }
#define write_reg(g, reg, data)		{ write_cmd(g, reg); write_data(g, data); }

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

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
	return TRUE;
}

#if GDISP_HARDWARE_STREAM_WRITE
	LLDSPEC	void gdisp_lld_write_start(GDisplay *g) {
		acquire_bus(g);
		write_cmd(g, SSD1351_SET_COLUMN_ADDRESS);
		write_data(g, g->p.x + OLED_X_OFFSET);
		write_data(g, g->p.x + OLED_X_OFFSET + g->p.cx - 1);
		write_cmd(g, SSD1351_SET_ROW_ADDRESS);
		write_data(g, g->p.y);
		write_data(g, g->p.y + g->p.cy - 1);
		write_cmd(g, SSD1351_WRITE_RAM);
	}
	LLDSPEC	void gdisp_lld_write_color(GDisplay *g) {
		LLDCOLOR_TYPE	c;

		c = gdispColor2Native(g->p.color);
		write_data(g, c >> 8);
		write_data(g, c & 0xFF);
	}
	LLDSPEC	void gdisp_lld_write_stop(GDisplay *g) {
		release_bus(g);
	}
#endif

#if GDISP_HARDWARE_BITFILLS
	static void oled_set_erea(uint8_t x, uint8_t y, uint8_t c_x, uint8_t c_y)
	{
		oled_ssd1357_write_cmd(SSD1351_WRITE_RAM);
		oled_ssd1357_write_cmd(SSD1351_SET_COLUMN_ADDRESS);
		oled_ssd1357_write_data(x + OLED_X_OFFSET);                            // Start Address
		oled_ssd1357_write_data(x + c_x + OLED_X_OFFSET - 1);    	 // End Address (0x7F)

		oled_ssd1357_write_cmd(SSD1351_SET_ROW_ADDRESS);
		oled_ssd1357_write_data(y);                            // Start Address
		oled_ssd1357_write_data(y + c_y - 1);   	 // End Address (0x7F)
		oled_ssd1357_write_cmd(SSD1351_WRITE_RAM);
	}

	void gdisp_lld_blit_area(GDisplay *g){
#if 0		
            #include "am_mcu_apollo.h"
        int debug_level = 1;
        if (debug_level){
            // am_hal_gpio_out_bit_replace(35, debug_level);
            debug_level = 0;
        }
    
		//oled_set_erea(g->p.x, g->p.y, g->p.cx, g->p.cx);
		gdisp_lld_write_start(g);
		uint32_t total_size = ((g->p.cx * g->p.cy));
		uint32_t tx_size = 0;
		#define MAX_QUEUE_SIZE (1024 * 4 - 1)

		uint16_t* p = g->p.ptr;	
		int i;
		uint8_t tx_data[GDISP_IMAGE_BMP_BLIT_BUFFER_SIZE << 1];
		LLDCOLOR_TYPE	c;
		for ( i = 0; i < total_size; i++){
			c = gdispColor2Native(*p++);
			tx_data[2*i] =  c >> 8;
			tx_data[2*i + 1] = c & 0xFF;
		}
		total_size = total_size << 1;
		do{		
			if (total_size - tx_size >= MAX_QUEUE_SIZE){
				oled_ssd1357_write_data_buff(tx_data, MAX_QUEUE_SIZE);
				tx_size += MAX_QUEUE_SIZE; 
			}
			else{
				am_hal_gpio_out_bit_replace(35, 1);
				oled_ssd1357_write_data_buff(tx_data, total_size - tx_size);
				tx_size += (total_size - tx_size); 
				am_hal_gpio_out_bit_replace(35, 0);
			}			
		}while(tx_size < total_size);
        // am_hal_gpio_out_bit_replace(35, debug_level);
        debug_level = 0;
#else	//fill buffer
		extern uint8_t frame_buffer[];		
		#define OLED_FRAME_BUFF_ARR_SIZE	(128 * 72)
		#define IMG_FRAME_BUFF_ARR_SIZE	(72 * 72)
		uint16_t* p = g->p.ptr;	
		LLDCOLOR_TYPE	c;
		int16_t start_y = g->p.y;
		int32_t pix_start =  g->p.x + start_y * g->p.cx;
		int32_t pix_len = g->p.cx * g->p.cy;
		int32_t i = pix_start;
	    // LOG_DEBUG("_new: start_x: %d, start_y: %d, count_x: %d, count_y: %d, i: %d, pix_start: %d, pix_len: %d \r\n", g->p.x, start_y, g->p.cx, g->p.cy, i, pix_start, pix_len);
		extern int img_offset;
		if (img_offset < 0){
			// pix_start = 0;
			p  += (IMG_FRAME_BUFF_ARR_SIZE - pix_len);
		}
		for ( i = pix_start; (i < OLED_FRAME_BUFF_ARR_SIZE) && (i < ((pix_start + pix_len) << 1)); i++){
			if (i < 0){
				// if (i == pix_start)
					// LOG_DEBUG("skip: start_x: %d, start_y: %d, count_x: %d, count_y: %d, i: %d,\r\n", g->p.x, start_y, g->p.cx, g->p.cy, i);
				continue;
			}
			
			// if (i == 0)
				// LOG_DEBUG("buff: start_x: %d, start_y: %d, count_x: %d, count_y: %d, i: %d,\r\n\r\n", g->p.x, start_y, g->p.cx, g->p.cy, i);
				
			c = gdispColor2Native(*p++);
			frame_buffer[(i<<1)] =  c >> 8;
			frame_buffer[(i<<1) + 1] = c & 0xFF;
		}
#endif		
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
