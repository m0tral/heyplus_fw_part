	/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */
#include "gui.h"

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_mcu_config.h"
#include "app_config.h"
#include "ryos.h"
#include "ry_fs.h"
#include "ry_hal_flash.h"

#include "gui_bare.h"
#include "am_util_debug.h"
#include "gui_img.h"
#include "ry_font.h"
#include "device_management_service.h"


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * CONSTANTS
 */ 

/*********************************************************************
 * VARIABLES
 */
extern u8_t* 		 p_draw_fb;
extern ryos_mutex_t* spi_mutex;
extern int debug_false_color;
extern ryos_mutex_t  *fb_mutex;


#if 0
const char* img_external[] = {	
							"menu_09_setting.bmp",
							"menu_02_hrm.bmp",
							"menu_03_sport.bmp",
							"menu_01_card.bmp",
							"m_setting_00_doing.bmp",
							"menu_05_weather.bmp",
							"menu_08_data.bmp",
							"menu_04_mijia.bmp",	
};

uint8_t* menu_info[] = { 
						"设置",
						"心率",
						"运动",
						"卡包",
						"表盘",
						"天气",
						"数据",
						"米家",
					};

#endif

const char* img_external[] = {
							"menu_01_card.bmp",	
							"menu_03_sport.bmp",
							"menu_02_hrm.bmp",
							"menu_04_mijia.bmp",
							"menu_05_weather.bmp",
							"menu_08_data.bmp",
							"menu_09_setting.bmp",
							"menu_06_clock.bmp",
#if 0
							"menu_07_calendar.bmp",
							"menu_04_mijia.bmp",
							"menu_00_home.bmp",
							"m_card_02_beijing.bmp",
							"m_card_03_xiaomi.bmp",
							"g_status_01_fail.bmp",
							"g_widget_00_enter.bmp",
							"g_widget_01_pending.bmp",
							"g_widget_03_stop.bmp",
							"g_widget_05_restore.bmp",
							"m_data_00_step.bmp",
							"m_data_01_calorie.bmp",
							"m_data_03_hrm.bmp",
							"m_data_05_sleeping.bmp",
							"m_notice_00_phone.bmp",
							"m_notice_01_mail.bmp",
							"m_notice_02_info.bmp",
							"m_sport_00_running.bmp",
							"m_upgrade_00_enter.bmp",
							"m_upgrade_01_doing.bmp",
							"m_weather_00_sunny.bmp",
							"m_weather_01_rainning.bmp",
#endif							
};

const int8_t* menu_info[] = { 
						"卡包",
						"运动",
						"心率",
						"米家",
						"天气",
						"数据",
						"设置",
						"闹钟",
					};




/*********************************************************************
 * FUNCTIONS
 */

/**
 * @brief   API to change true color to white and black
 *
 * @param   rgb: poiter to brg color
 *
 * @return  None
 */
void true_color_to_white_black(uint8_t* rgb)
{
	uint8_t* b = rgb;
	uint8_t* g = rgb + 1;
	uint8_t* r = rgb + 2;
	uint32_t color;
	if (debug_false_color && ((*b + *g + *r) > 0)){
		color = ((int)*r*299 +(int)*g*587 + (int)*b*114+500)/1000;
		// color = 0.299 * (int)*r + 0.587 * (int)b + 0.114 * (int)g;
		*b = *g = *r = (uint8_t)(color);	
	}
}

void set_true_color_in_frame_buffer(uint8_t* framebuff, uint32_t len)
{
	for(int i = 0; i < len; i++){
		true_color_to_white_black(framebuff + 3 * i);
	}
}


static color_t gui_img_BlendColor(color_t fg, color_t bg, uint8_t alpha)
{
    uint16_t fg_ratio = alpha + 1;
    uint16_t bg_ratio = 256 - alpha;
    uint16_t r, g, b;

    r = RED_OF(fg) * fg_ratio;
    g = GREEN_OF(fg) * fg_ratio;
    b = BLUE_OF(fg) * fg_ratio;

    r += RED_OF(bg) * bg_ratio;
    g += GREEN_OF(bg) * bg_ratio;
    b += BLUE_OF(bg) * bg_ratio;

    r >>= 8;
    g >>= 8;
    b >>= 8;

    return RGB2COLOR(r, g, b);
}


static void set_frame_buffer_pixel_color(uint16_t x, uint16_t y, uint32_t fgcolor, uint8_t alpha)
{
    color_t bgcolor,color;
    gui_bare_get_pixel_color(x, y, &bgcolor);
    color = gui_img_BlendColor(fgcolor, bgcolor, alpha);
    gui_bare_set_pixel_color(x, y, color);
}

void set_true_color_all_frame_buffer(void)
{
	if (debug_false_color != 0){
		set_true_color_in_frame_buffer(frame_buffer, IMG_PIXAL_FULL_SCREEN);
	}
}

/**
 * @brief   API to copy internal flash image to framebuffer
 *
 * @param   img_addr: image address
 * 			buffer_offset: frame buffer offset address
 * 			len： copy data len
 *
 * @return  None
 */
void img_internal_flash_to_framebuffer(uint32_t img_addr, uint32_t buffer_offset, uint32_t len)
{
	ry_memcpy(frame_buffer + buffer_offset, (uint32_t*)img_addr, len);
	set_true_color_all_frame_buffer();
}

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
void draw_raw_img24_to_framebuffer(uint32_t start_x, uint32_t start_y, uint32_t width, uint32_t height, uint8_t* pImg)
{
    int i, j;
	int buffer_offset, img_offset;

    for (j = 0; j < height; j ++){
		buffer_offset = 360 * (j + start_y) + start_x * 3;
		img_offset = j * width * 3;
        ry_memcpy(frame_buffer + buffer_offset, (uint32_t*)(pImg + img_offset), width * 3);
    }
}


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
								   uint8_t* p_width, uint8_t* p_height, d_justify_t justity)
{
    //ryos_mutex_lock(spi_mutex);
	// int excnt;
	// LOG_DEBUG("[img_exflash_to_framebuffer] begin %d \r\n", excnt);
	uint8_t * buff = (uint8_t *)ry_malloc(IMG_MAX_BYTE_PER_LINE);
	uint32_t written_bytes;
	FRESULT res ;
	FIL* file = (FIL*)ry_malloc(sizeof(FIL));
  
	uint32_t pos = 0;
	int len = IMG_HEAD_LEN;
	uint32_t width = 0;
	uint32_t height = 0;
	int p_y = pos_y,p_x = pos_x;

	if ((img == "")||(pos_y < 0)){
		goto exit;		
	}

	res = f_open(file, (const TCHAR*)img, FA_READ);
	if (res != FR_OK) {
		LOG_DEBUG("file open failed. [%s] \n", (const TCHAR*)img);
		goto exit;
	}

	res = f_read(file, buff, len, &written_bytes);
#if 0			
	for (int k = 0; k < len; k++){
		am_util_debug_printf("0x%02X,", buff[k]);
		pos ++;
		if (pos % 16 == 0)
			am_util_debug_printf("\n\r");
	}
#endif				
	if (buff[0] == 'B' && buff[1] == 'M'){
		;//am_util_debug_printf("\n\rgot: %s \n\r", img);
	}

	if (buff[18] > RM_OLED_WIDTH){
		buff[18] = RM_OLED_WIDTH;
	}

	if (buff[22] > RM_OLED_HEIGHT){
		buff[22] = RM_OLED_HEIGHT;
	}

	*p_width = width = buff[18];
	*p_height = height = buff[22];
	//LOG_DEBUG("img %s: width %d, img height:%d  size=%d\n\r", img, width, height, f_size(file));
    if((p_y + height <= 0) || (p_x + width <= 0)){
        f_close(file);
        goto exit;
    }

    if((p_y + height > 240)){
        f_close(file);
        goto exit;
    }
    if (justity == d_justify_point){
		p_y -=  height/2;
		p_x -=  width/2;
	}else if(justity == d_justify_bottom_center){
        p_y -= height;
        p_x -=  width/2;
	}
	
	pos = 0;	
	len = ((width * 3 + 3)/4)*4;
	for (int i = 0; i < height; i++){
		res = f_read(file, buff, len, &written_bytes);

        if(res == FR_INT_ERR){
            if((get_last_reset_type() != FS_RESTART_COUNT) &&(get_last_reset_type() != FACTORY_FS_RESTART_COUNT)){
                /*add_reset_count(FS_RESTART_COUNT);
                recover_fs_head();
                ry_system_reset();*/
            }else if(get_last_reset_type() == FS_RESTART_COUNT){
                /*u8_t *work = (u8_t *)ry_malloc(4096);
    		    f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096);
                ry_free(work);
                add_reset_count(FACTORY_FS_RESTART_COUNT);
                ry_system_reset();*/

                /*add_reset_count(FACTORY_FS_RESTART_COUNT);
                recover_fs_head_to_factroy();
                ry_system_reset();*/
            }
        }
		
#if 0	
		for (int k = 0; k < len; k++){
			am_util_debug_printf("0X%02X,", buff[k]);
			pos ++;
			if (pos % 16 == 0)
				am_util_debug_printf("\n\r");
		}	
#endif
		/*uint8_t testBuf_e[4];
		if (len & 3){
			f_read(file, testBuf_e, len & 3, &written_bytes);
		}*/
		
		//scan a top line and copy to the bottom
		uint32_t dst_line = (height - i) + p_y;
		if (dst_line >= IMG_MAX_HEIGHT){
			continue;	//skip this line
		}

		uint32_t xline_start = p_x;			//default d_justify_left
		if (justity == d_justify_center){
			xline_start = (IMG_MAX_WIDTH - width) >> 1;
		}
		else if (justity == d_justify_right){
			xline_start = IMG_MAX_WIDTH - width;
		}

		uint32_t fr_buffer_pos = (dst_line * IMG_MAX_WIDTH + xline_start) * 3;
		uint32_t len_copy = (width < IMG_MAX_WIDTH) ? width : IMG_MAX_WIDTH;
		len_copy = ((len_copy + xline_start) >= IMG_MAX_WIDTH) ? (IMG_MAX_WIDTH - xline_start) : len_copy;
		len_copy = len_copy * 3;
		ry_memcpy(frame_buffer + fr_buffer_pos, buff, len_copy);
		ry_memset(buff, 0 , IMG_MAX_BYTE_PER_LINE);
	}
	set_true_color_all_frame_buffer();
	f_close(file);
	p_draw_fb  = frame_buffer;

exit:
	ry_free(file);
	ry_free(buff);
	
	// LOG_DEBUG("[img_exflash_to_framebuffer] end %d \r\n", excnt++);	
	//ryos_mutex_unlock(spi_mutex);

	return res;
}

void put_line_to_framebuffer_transparent(void* fr_buffer_pos, const void* buff, uint32_t length);

uint8_t img_exflash_to_framebuffer_transparent(uint8_t* img, int pos_x, int pos_y, \
								   uint8_t* p_width, uint8_t* p_height, d_justify_t justity)
{
    //ryos_mutex_lock(spi_mutex);
	// int excnt;
	// LOG_DEBUG("[img_exflash_to_framebuffer] begin %d \r\n", excnt);
	uint8_t * buff = (uint8_t *)ry_malloc(IMG_MAX_BYTE_PER_LINE);
	uint32_t written_bytes;
	FRESULT res ;
	FIL* file = (FIL*)ry_malloc(sizeof(FIL));
  
	uint32_t pos = 0;
	int len = IMG_HEAD_LEN;
	uint32_t width = 0;
	uint32_t height = 0;
	int p_y = pos_y,p_x = pos_x;

	if ((img == "")||(pos_y < 0)){
		goto exit;		
	}

	res = f_open(file, (const TCHAR*)img, FA_READ);
	if (res != FR_OK) {
		LOG_DEBUG("file open failed. [%s] \n", (const TCHAR*)img);
		goto exit;
	}

	res = f_read(file, buff, len, &written_bytes);
#if 0			
	for (int k = 0; k < len; k++){
		am_util_debug_printf("0x%02X,", buff[k]);
		pos ++;
		if (pos % 16 == 0)
			am_util_debug_printf("\n\r");
	}
#endif				
	if (buff[0] == 'B' && buff[1] == 'M'){
		;//am_util_debug_printf("\n\rgot: %s \n\r", img);
	}

	if (buff[18] > RM_OLED_WIDTH){
		buff[18] = RM_OLED_WIDTH;
	}

	if (buff[22] > RM_OLED_HEIGHT){
		buff[22] = RM_OLED_HEIGHT;
	}

	*p_width = width = buff[18];
	*p_height = height = buff[22];
	//LOG_DEBUG("img %s: width %d, img height:%d  size=%d\n\r", img, width, height, f_size(file));
    if((p_y + height <= 0) || (p_x + width <= 0)){
        f_close(file);
        goto exit;
    }

    if((p_y + height > 240)){
        f_close(file);
        goto exit;
    }
    if (justity == d_justify_point){
		p_y -=  height/2;
		p_x -=  width/2;
	}else if(justity == d_justify_bottom_center){
        p_y -= height;
        p_x -=  width/2;
	}
	
	pos = 0;	
	len = ((width * 3 + 3)/4)*4;
	for (int i = 0; i < height; i++){
		res = f_read(file, buff, len, &written_bytes);

        if(res == FR_INT_ERR){
            if((get_last_reset_type() != FS_RESTART_COUNT) &&(get_last_reset_type() != FACTORY_FS_RESTART_COUNT)){
                /*add_reset_count(FS_RESTART_COUNT);
                recover_fs_head();
                ry_system_reset();*/
            }else if(get_last_reset_type() == FS_RESTART_COUNT){
                /*u8_t *work = (u8_t *)ry_malloc(4096);
    		    f_mkfs("", FM_ANY| FM_SFD, 4096, work, 4096);
                ry_free(work);
                add_reset_count(FACTORY_FS_RESTART_COUNT);
                ry_system_reset();*/

                /*add_reset_count(FACTORY_FS_RESTART_COUNT);
                recover_fs_head_to_factroy();
                ry_system_reset();*/
            }
        }
		
#if 0	
		for (int k = 0; k < len; k++){
			am_util_debug_printf("0X%02X,", buff[k]);
			pos ++;
			if (pos % 16 == 0)
				am_util_debug_printf("\n\r");
		}	
#endif
		/*uint8_t testBuf_e[4];
		if (len & 3){
			f_read(file, testBuf_e, len & 3, &written_bytes);
		}*/
		
		//scan a top line and copy to the bottom
		uint32_t dst_line = (height - i) + p_y;
		if (dst_line >= IMG_MAX_HEIGHT){
			continue;	//skip this line
		}

		uint32_t xline_start = p_x;			//default d_justify_left
		if (justity == d_justify_center){
			xline_start = (IMG_MAX_WIDTH - width) >> 1;
		}
		else if (justity == d_justify_right){
			xline_start = IMG_MAX_WIDTH - width;
		}

		uint32_t fr_buffer_pos = (dst_line * IMG_MAX_WIDTH + xline_start) * 3;
		uint32_t len_copy = (width < IMG_MAX_WIDTH) ? width : IMG_MAX_WIDTH;
		len_copy = ((len_copy + xline_start) >= IMG_MAX_WIDTH) ? (IMG_MAX_WIDTH - xline_start) : len_copy;
		len_copy = len_copy * 3;
		put_line_to_framebuffer_transparent(frame_buffer + fr_buffer_pos, buff, len_copy);
		//ry_memcpy(frame_buffer + fr_buffer_pos, buff, len_copy);
		ry_memset(buff, 0 , IMG_MAX_BYTE_PER_LINE);
	}
	set_true_color_all_frame_buffer();
	f_close(file);
	p_draw_fb  = frame_buffer;

exit:
	ry_free(file);
	ry_free(buff);
	
	// LOG_DEBUG("[img_exflash_to_framebuffer] end %d \r\n", excnt++);	
	//ryos_mutex_unlock(spi_mutex);

	return res;
}

void put_line_to_framebuffer_transparent(void* fr_buffer_pos, const void* buff, uint32_t length)
{
		uint8_t * frame_buffer = (uint8_t *)fr_buffer_pos;
		uint8_t * buffer = (uint8_t *)buff;
		for (int i = 0; i < length; i += 3) {
				if (buffer[i] == 0 && buffer[i+1] == 0 && buffer [i+2] == 0) continue;
				ry_memcpy(frame_buffer + i, buffer + i, 3);
		}
}

/**
 * @brief   API to get read exflash image resources to internal
 *
 * @param   reload - 1: load img very time, 0: auto detect
 *
 * @return  res  - result of exflash image resources move
 *			       reference - File function return code (FRESULT)
 */
uint8_t img_exflash_to_internal_flash(uint8_t reload)
{
	uint8_t * buff = (uint8_t *)ry_malloc(IMG_MAX_BYTE_PER_LINE);
	uint32_t written_bytes;
	FRESULT res ;
	FIL* file = (FIL*)ry_malloc(sizeof(FIL));
  
	uint32_t start_addr = EXFLASH_ADDR_IMG_RESOURCE;
	uint8_t  buffer[8];
	uint32_t write_len = IMG_MAX_WIDTH * IMG_MAX_BYTE_PER_LINE;
	uint8_t  resource_info = 0xff;
	uint32_t resource_info_addr = EXFLASH_ADDR_IMG_RESOURCE_INFO;
	uint8_t  resource_internal = 1;

	//check flash resources
	ry_hal_spi_flash_read( buffer, resource_info_addr,8);
    LOG_DEBUG("Flash Addr info: ");
    ry_data_dump(buffer, 8, ' ');
    
	for (int i = 0; i < 8; i ++){
		ry_hal_spi_flash_read(buffer, resource_info_addr, 1);
		if (*buffer != i + 1){
			resource_internal = 0;
			LOG_DEBUG("idx: %d, info read: 0X%08X, start_addr: 0X%08X \n", i, *(uint32_t *)buffer, i);
			break;
		}
		resource_info_addr ++;
	}

	if (reload){
    	resource_internal = 0;
	}

	start_addr = EXFLASH_ADDR_IMG_RESOURCE;
	resource_info_addr = EXFLASH_ADDR_IMG_RESOURCE_INFO;
	for (int img_idx = 0; img_idx < (sizeof(img_external) >> 2); img_idx++){
		if (resource_internal == 0){
			res = f_open(file, img_external[img_idx], FA_READ);
			int pos = 0;
			if (res != FR_OK) {
				LOG_ERROR("file open failed: %d\n", res);
				goto exit;
			}
		
			int len = IMG_HEAD_LEN;
			int k = 0;
			res = f_read(file, buff, len, &written_bytes);
#if 0			
			for (k = 0; k < len; k++){
				am_util_debug_printf("0x%02X,", buff[k]);
				pos ++;
				if (pos % 16 == 0)
					am_util_debug_printf("\n\r");
			}
#endif				
			if (buff[0] == 'B' && buff[1] == 'M'){
				;//am_util_debug_printf("\n\rgot: %s \n\r", img_external[img_idx]);
			}

			if (buff[18] > RM_OLED_WIDTH){
				buff[18] = RM_OLED_WIDTH;
			}

			if (buff[22] > RM_OLED_HEIGHT){
				buff[22] = RM_OLED_HEIGHT;
			}
			
			uint32_t width = buff[18];
			uint32_t height = buff[22];
			LOG_DEBUG("img %s: width %d, img height:%d  \n\r", img_external[img_idx], width, height);
			ry_memset(frame_buffer, 0, sizeof(frame_buffer));
			pos = 0;	
			len = width * 3;
			for (int i = 0; i < height; i++){
				res = f_read(file, buff, len, &written_bytes);
#if 0		
				for (k = 0; k < len; k++){
					am_util_debug_printf("0X%02X,", buff[k]);
					pos ++;
					if (pos % 16 == 0)
						am_util_debug_printf("\n\r");
				}	
#endif
				if (len & 3){
					uint8_t testBuf_e[4];
					f_read(file, testBuf_e, (4-(len & 3)), &written_bytes);
				}	
				uint32_t buffer_start = ((height - i) * IMG_MAX_WIDTH + ((IMG_MAX_WIDTH - width) >> 1)) * 3;
				ry_memcpy(frame_buffer + buffer_start, buff, len);
			}
			f_close(file);
			
			gdispFillStringBox( 0,	\
								90,  	 \
								0,     \
								MAIN_MENU_FONT_HEIGHT,			\
								(char*)menu_info[img_idx],	\
								0,      \
								White,	\
								Black,	\
								justifyCenter);
			set_true_color_all_frame_buffer();	
			//p_draw_fb  = frame_buffer;
			//update_buffer_to_oled();

			resource_info = img_idx + 1;
            LOG_DEBUG("Write flash addr: %x, len:%d\r\n", (u32_t)resource_info_addr, 1);
            LOG_DEBUG("Write flash addr: %x, len:%d\r\n\r\n", (u32_t)start_addr, write_len);
			ry_hal_spi_flash_write(&resource_info, resource_info_addr, 1);
			ry_hal_spi_flash_write(frame_buffer, start_addr, write_len);
		}
		else{
			LOG_DEBUG("img %s is already in internal flash\n\r", img_external[img_idx]);
		}
#if 0	
		p_draw_fb = (uint8_t*)start_addr;
		update_buffer_to_oled();
		ryos_delay_ms(100);		
#endif	
		start_addr += write_len;
		resource_info_addr ++;	
	}
#if 0
    img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE, RM_OLED_IMG_24BIT);
	p_draw_fb = frame_buffer;	
	update_buffer_to_oled();
#endif


exit:
    ry_free(file);
	ry_free(buff);
	
	return res;
}

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
							 uint8_t* p_width, uint8_t* p_height, d_justify_t justify)
{
	int res = FR_OK;
	if (res_storage == RES_INTERNAL){
			ryos_mutex_lock(fb_mutex);
			gdispImage myImage;
			uint8_t x_start = pos_x;
			gdispImageOpenFile(&myImage, (const char*)img);

			if (justify == d_justify_center){
				x_start = (IMG_MAX_WIDTH - myImage.width) >> 1;
			}
			else if (justify == d_justify_right){
				x_start = IMG_MAX_WIDTH - myImage.width;
			}
			*p_width = myImage.width;
			*p_height = myImage.height;
			gdispImageDraw(&myImage,                    
						x_start,  
						pos_y,                           
						myImage.width,               
						myImage.height,              
						0,                           
						0);
			gdispImageClose(&myImage);
			ryos_mutex_unlock(fb_mutex);
	}
	else if (res_storage == RES_EXFLASH){
			ryos_mutex_lock(fb_mutex);
			res = img_exflash_to_framebuffer(img, pos_x, pos_y, p_width, p_height, justify);
			ryos_mutex_unlock(fb_mutex);
	}
	return res;
}

int draw_img_to_framebuffer_transparent(uint8_t res_storage, uint8_t* img, uint8_t pos_x, uint8_t pos_y, \
							 uint8_t* p_width, uint8_t* p_height, d_justify_t justify)
{
	int res = FR_OK;
	if (res_storage == RES_INTERNAL){
			// not supported
	}
	else if (res_storage == RES_EXFLASH){
			ryos_mutex_lock(fb_mutex);
			res = img_exflash_to_framebuffer(img, pos_x, pos_y, p_width, p_height, justify);
			ryos_mutex_unlock(fb_mutex);
	}
	return res;
}

/**
 * @brief   set_dip_at_framebuff
 *
 * @param   x: posion x
 * 			y: posion y
 * 			color: the color to set
 *
 * @return  None
 */
void set_dip_at_framebuff(int x, int y, int color)
{
    if((x < 0)||(y < 0)){
        return ;
    }
    uint32_t pos = 120 * y * 3 + x * 3;
    frame_buffer[pos]     = (uint8_t) color & 0xff;
    frame_buffer[pos + 1] = (uint8_t) (color>>8) & 0xff;
    frame_buffer[pos + 2] = (uint8_t) (color>>16) & 0xff;
    true_color_to_white_black(frame_buffer + pos);
}

/**
 * @brief   draw_raw_png_to_framebuffer
 *
 * @param   p_pic: raw_png description
 *          x: start position x
 *          y: start position y
 *          color: the fg color to blend
 *
 * @return  None
 */

void draw_raw_png_to_framebuffer(raw_png_descript_t const* p_pic, uint16_t x, uint16_t y, uint32_t color)
{
    uint8_t const* p_src = p_pic->p_data;
    uint16_t witdh = p_pic->width;
    uint16_t height = p_pic->height;

    for(int i =0;i<height;i++) {
        for(int j =0;j<witdh;j++) {
            uint8_t alpha =  p_src[i*witdh+j];
            if(alpha > 0) {
                set_frame_buffer_pixel_color(x + j,y + i, color, alpha);
            }
        }
    }
}

