/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __GUI_IMG_H__
#define __GUI_IMG_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"

/*
 * CONSTANTS
 *******************************************************************************
 */

#define FLASH_ADDR_IMG_RESOURCE_INFO	0xA7900//0x8A000
#define FLASH_ADDR_IMG_RESOURCE			(FLASH_ADDR_IMG_RESOURCE_INFO+0x40)//0x8A040

#define EXFLASH_ADDR_IMG_RESOURCE_INFO  0x707900
#define EXFLASH_ADDR_IMG_RESOURCE       (EXFLASH_ADDR_IMG_RESOURCE_INFO+0x40)


#define IMG_MAX_WIDTH      		        120
#define IMG_MAX_HEIGHT     		        240
#define IMG_BYTE_PER_PIXAL  	        3
#define IMG_MAX_BYTE_PER_LINE           360
#define IMG_HEAD_LEN                 	54
#define IMG_PIXAL_FULL_SCREEN           (120*240)

#define MAIN_MENU_FONT_HEIGHT           22

/*
 * TYPES
 *******************************************************************************
 */

/**
 * @enum    d_justify
 * @brief   Type for the display justification.
 */
typedef enum ad_justify_ {
	d_justify_left   = 0x00,		/**< Justify Left (the default) */
	d_justify_center = 0x01,		/**< Justify Center */
	d_justify_right  = 0x02,		/**< Justify Right */
	d_justify_point  = 0x03,
	d_justify_bottom_center = 0x04,
	// d_justify_top    = 0x10,		/**< Justify Top */
	// d_justify_middle = 0x00,		/**< Justify Middle (the default) */
	// d_justify_bottom = 0x20,		/**< Justify Bottom */
} d_justify_t;

typedef enum res_storage {
	RES_NONE,	
	RES_INTERNAL,
	RES_EXFLASH,
}res_storage_t;

typedef struct
{
    uint8_t const* p_data;
    uint16_t width;
    uint16_t height;
    uint32_t fgcolor;   //未直接使用
}raw_png_descript_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to copy internal flash image to framebuffer
 *
 * @param   img_addr      -  image address
 * 			buffer_offset -  rame buffer offset address
 * 			len           -  copy data len
 *
 * @return  None
 */
void img_internal_flash_to_framebuffer(uint32_t img_addr, uint32_t buffer_offset, uint32_t len);

/**
 * @brief   API to get read exflash image resources to framebuffer
 *
 * @param   img      - img file name string
 * @param   pos_x    - img display start x
 * @param   pos_y    - img display start y
 * @param   p_width  - pointer to the img width
 * @param   p_height - pointer to the img height
 * @param   justify	 - Justify the display left, center or right
 * 
 * @return  res      - result of exflash image resources files move
 *			           reference - File function return code (FRESULT)
 */
uint8_t img_exflash_to_framebuffer(uint8_t* img, int pos_x, int pos_y, \
								   uint8_t* p_width, uint8_t* p_height, d_justify_t justity);

uint8_t img_exflash_to_framebuffer_transparent(uint8_t* img, int pos_x, int pos_y, \
								   uint8_t* p_width, uint8_t* p_height, d_justify_t justity);

/**
 * @brief   API to get read exflash image resources to internal
 *
 * @param   reload - 1: load img very time, 0: auto detect
 *
 * @return  res  - result of exflash image resources move
 *			       reference - File function return code (FRESULT)
 */
uint8_t img_exflash_to_internal_flash(uint8_t reload);


/**
 * @brief   API to get read exflash image resources to framebuffer
 *
 * @param   res_storage - RES_INTERNAL, RES_EXFLASH, or RES_NONE
 * @param   img      - img file name string
 * @param   pos_x    - img display start x
 * @param   pos_y    - img display start y
 * @param   p_width  - pointer to the img width
 * @param   p_height - pointer to the img height
 * @param   justify	 - Justify the display left, center or right
 * 
 * @return  res  - result of exflash image resources move
 *			       reference - File function return code (FRESULT)
 */
int draw_img_to_framebuffer(uint8_t res_storage, uint8_t* img, uint8_t pos_x, uint8_t pos_y, \
							 uint8_t* p_width, uint8_t* p_height, d_justify_t justify);


/**
 * @brief   set_dip_at_framebuff
 *
 * @param   x: posion x
 * 			y: posion y
 * 			color: the color to set
 *
 * @return  None
 */
void set_dip_at_framebuff(int x, int y, int color);


/**
 * @brief   API to draw_raw_img24_to_framebuffer
 *
 * @param   start_x  - img draw posion start_x
 * @param   start_y  - img draw posion start_y
 * @param   width    - img width
 * @param   height   - img height
 * @param   pImg	 - pointer to img data
 * 
 * @return  None
 */
void draw_raw_img24_to_framebuffer(uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height, uint8_t* pImg);

void set_true_color_all_frame_buffer(void);
void true_color_to_white_black(uint8_t* rgb);
void draw_raw_png_to_framebuffer(raw_png_descript_t const* p_pic, uint16_t x, uint16_t y, uint32_t color);



#endif  /* __GUI_IMG_H__ */


