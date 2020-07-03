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

#define CHAR_SPACE          4
#define CHAR_SPACE_NEW      1

extern uint8_t frame_buffer[RM_OLED_IMG_24BIT];
fontHead_t* cur_font_head = NULL;
fontSection_t* cur_sections = NULL;
FIL * cur_font_fp = NULL;

u32_t open_font_height = 0;
quickRead_t quick_info = {0};

fontDispCache_t font_cache = {0};




int InitRyFont(int height)
{
    int error = 0xffff;
    u32_t read_size = 0;

    if(height == open_font_height){
        return 0;
    }else{
        if(cur_font_fp != NULL){
            DeInitRyFont();
            if(cur_font_head != NULL){
                ry_free(cur_font_head);
                cur_font_head = NULL;
            }
            if(cur_sections != NULL){
                ry_free(cur_sections);
                cur_sections = NULL;
            }
        }

        open_font_height = height;
        
    }
    

    cur_font_fp = (FIL *)ry_malloc(sizeof(FIL));
    
    FRESULT ret = 0; 

    if(height >= 22){
        ret = f_open(cur_font_fp,RY_FONT_HEIGHT_22, FA_READ);
        open_font_height = 22;
    }else {
        ret = f_open(cur_font_fp,RY_FONT_HEIGHT_20, FA_READ);
        open_font_height = 20;
    }
    if(ret != FR_OK){
        error = ret;
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_FILE_OPEN,__LINE__);
        goto exit;
    }

    //fontHead_t head;
    cur_font_head = (fontHead_t *)ry_malloc(sizeof(fontHead_t));
    if(cur_font_head == NULL){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_MALLOC_FAIL,__LINE__);
        error = ret;
        goto exit;
    }
    
    //fontSection_t * sections = NULL;
    ret = f_lseek(cur_font_fp, 0);
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_SEEK,__LINE__);
        error = ret;
        goto exit;
    }
    ret = f_read(cur_font_fp,cur_font_head, sizeof(fontHead_t), &read_size);
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_READ,__LINE__);
        error = ret;
        goto exit;
    }
    cur_font_head->height = height;

    if(cur_font_head->section_num > FONT_MAX_SECTION_NUM){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_MALLOC_FAIL,__LINE__);
        goto exit;
    }

    //LOG_DEBUG("head sn:%d,h :%d\n",cur_font_head->section_num, cur_font_head->height );

    cur_sections = ry_malloc(sizeof(fontSection_t) * (cur_font_head->section_num) );

    if(cur_sections == NULL){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_MALLOC_FAIL,__LINE__);
        goto exit;
    }

    ret = f_read(cur_font_fp,cur_sections, sizeof(fontSection_t) * (cur_font_head->section_num), &read_size);
    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_READ,__LINE__);
        error = ret;
        goto exit;
    }

exit:

    return error;
}

void DeInitRyFont(void)
{
    int ret;
    ret = f_close(cur_font_fp);

    if(ret != FR_OK){
        LOG_ERROR("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_CLOSE,__LINE__);
    }
    ry_free(cur_font_fp);
    cur_font_fp = NULL;
}

void fontQuickReadInit(void)
{
	#if RY_BUILD == RY_RELEASE
    u8_t i = 0;
    u32_t char_info_min = 0,char_info_max = 0;
    u32_t char_info_size = 0;
    u32_t read_bytes = 0;
    FRESULT ret = 0; 
    
    InitRyFont(QUICK_READ_FONT_HEIGHT);

    if((cur_sections == NULL)||(cur_font_head == NULL)||(cur_font_fp == NULL)){
        return ;
    }

    char_info_min = cur_sections[0].info_offset;
    //char_info_max = cur_sections[0]->info_offset;
    /*for(i = 0; i < (cur_font_head->section_num); i++){
        if(cur_sections[i]->info_offset < char_info_min){
            char_info_min = cur_sections[i]->info_offset;
        }
    }*/
    char_info_max = cur_sections[cur_font_head->section_num - 1].info_offset + 
        (cur_sections[cur_font_head->section_num - 1].end_char - cur_sections[cur_font_head->section_num - 1].start_char + 1)*sizeof(charInfo_t);
    
    quick_info.start_offset = char_info_min;

    char_info_size = (char_info_max - char_info_min + 1) ;

    

    u8_t * temp_buf = (u8_t *)ry_malloc(8192);
    if(temp_buf == NULL){
        goto error;
    }

    
	if(char_info_size + QUICK_READ_START_ADDR > 0xFB000){
		goto error;
	}

    f_lseek(cur_font_fp, char_info_min);

    for(i = 0; i < (char_info_size/8192 + 1); i++){
        ret = f_read(cur_font_fp, temp_buf, 8192, &read_bytes);
        if(ret != FR_OK){
            goto error;
        }
        ry_hal_flash_write(QUICK_READ_START_ADDR + i * 8192, temp_buf, 8192);
    }

    quick_info.init_flag = 1;

    ry_free(temp_buf);

    return ;

error:
    ry_free(temp_buf);
    quick_info.init_flag = 0;

    #endif
}


void setDip(int x, int y, int color)
{
    if((0 <= x)&&(x < RM_OLED_WIDTH)){
        uint32_t pos = 120 * y * 3 + x * 3;
        frame_buffer[pos]     = (uint8_t) color & 0xff;
        frame_buffer[pos + 1] = (uint8_t) (color>>8) & 0xff;
        frame_buffer[pos + 2] = (uint8_t) (color>>16) & 0xff;
        //true_color_to_white_black(frame_buffer + pos);
    }
}

void setRect(int max_x,int max_y, int min_x, int min_y, int color)
{   
    int x,y;
    for(x = min_x; x <= max_x; x++) {
        for(y = min_y; y <= max_y; y++) {
            setDip(x, y, color);
        }
    }
}

void setHLine(int x_start, int x_end, int y_vert, int color)
{   
    int x;    
    for(x = x_start; x <= x_end; x++) {
        setDip(x, y_vert, color);
    }
}


void add_to_font_cache(int code, u8_t *buf, u16_t size)
{
    /*if(FONT_CACHE_CELL_MAX <= font_cache.count){
        font_cache.count = FONT_CACHE_CELL_MAX - 1;
        //LOG_DEBUG("MAX  FREE\n");
        ry_free(font_cache.cell[0].buf);
        ry_memcpy(&(font_cache.cell[0]), &(font_cache.cell[1]), (FONT_CACHE_CELL_MAX -1)*sizeof(fontDispCacheCell_t) );


    }
    
    font_cache.cell[font_cache.count].code = 0;
    font_cache.cell[font_cache.count].buf = ry_malloc(size);

    if(font_cache.cell[font_cache.count].buf == NULL){
        LOG_INFO("[%s]malloc fail\n", __FUNCTION__);
        return;
    }
    font_cache.cell[font_cache.count].code = code;
    ry_memcpy(font_cache.cell[font_cache.count].buf, buf, size);
    
    font_cache.count++;*/
        
    
    
}

u8_t* find_font_cache(int code)
{
    /*int i = 0;

    for(i = 0; i < font_cache.count; i++){
        if(font_cache.cell[i].code == code){
            //LOG_DEBUG("++++++++find cache+++++\n");
            return font_cache.cell[i].buf;
        }
    }*/


    return NULL;
}

bool is_control_code(int code)
{

    const u16_t control_code_table[] = {0x200E, 0x200F, 0x202A, 0x202D, 0x202B, 0x202E, 0x202C, 0x2066, 0x2067, 0x2068, 0x2069};
    u16_t num = sizeof(control_code_table) / sizeof(u16_t);
    if((code < control_code_table[0])||(code > control_code_table[num - 1])){
        return false;
    }else{
        u16_t i = 0;
        for(i = 1; i < num - 1; i++){
            if(code == control_code_table[i]){
                return true;
            }
        }
        
        return false; 
    }

}


int dispChar(int * ch, int x, int y, int color, int bg_color, int * width)
{
    int error = 0xffff;
    u8_t flag = 0;
    int dataSize = cur_font_head->height * ((cur_font_head->height + 7) / 8) + 2;
    int i = 0, j = 0,ret;
    UINT read_size;
    u8_t* temp_buf = NULL;

    if(y + cur_font_head->height <=0){
        goto exit;
    }

    for(i= 0; i < cur_font_head->section_num; i++){
        if ((cur_sections[i].start_char <= *ch) && (*ch <= cur_sections[i].end_char)){
            flag = 1;
            break;
        }
    }
    /*if(*ch == '\n'){
        *width = RM_OLED_WIDTH - x;
    }*/
    if(*ch == ' '){
        *width = 5;		// space char width fix
        goto exit;
    }
    if(flag == 0){
        *width = 0;
        error = 0x01;
        goto exit;
    }
    if(*ch == 0x2005){
        *width = 12;
        goto exit;
    }

    if(is_control_code(*ch) == true){
        goto exit;
    }
    
    if(cur_font_head->magic == 0xDEADBEEF){

        int data_addr = cur_sections[i].info_offset + (*ch - cur_sections[i].start_char) * (dataSize);
        
        ret = f_lseek(cur_font_fp, data_addr);
        if(ret != FR_OK){
            LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_FILE_SEEK,__LINE__);
            error = ret;
            goto exit;
        }

        fontDataInfo_t dataInfo;
    	ret = f_read(cur_font_fp, &dataInfo, sizeof(fontDataInfo_t), (UINT*)&read_size);
    	if(ret != FR_OK){
            LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_FILE_READ,__LINE__);
            error = ret;
            goto exit;
        }
    	if(dataInfo.height == 0){
    	    LOG_DEBUG("0x%x----height is 0\n", *ch);
            dataInfo.height = cur_font_head->height;
    	}
    	
        if(dataInfo.width == 0){
            LOG_DEBUG("0x%x----width is 0\n", *ch);
            dataInfo.width = cur_font_head->height;
        }

        temp_buf = (u8_t *)ry_malloc(dataSize-2);

        ret = f_read(cur_font_fp, temp_buf, (dataSize-2),  (UINT*)&read_size);
        if(ret != FR_OK){
            LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_FILE_READ,__LINE__);
            error = ret;
            goto exit;
        }

        for (i = 0; i < dataInfo.height; ++i)
    	{
    		for (j = 0; j < dataInfo.width; ++j)
    		{
    			u8_t tempData = temp_buf[i * ((cur_font_head->height + 7) / 8) + j / 8];
    			//printf("%c",(tempData & (1 << j % 8)) ? '*' : 0);
    			//printf("%c", bitmap.buffer[i * bitmap.width + j] ? '*' : 0);
    			if(tempData & (1 << j % 8)){
    			    if(y + i + (cur_font_head->height - dataInfo.height) > RM_OLED_HEIGHT){
                        continue;
    			    }
                    setDip(x + j, y + i + (cur_font_head->height - dataInfo.height), color);
    			}else{
    			    if(bg_color >> 27){
                        
    			    }else{
    			        if(y + i + (cur_font_head->height - dataInfo.height) > RM_OLED_HEIGHT){
                            continue;
        			    }
                        setDip(x + j, y + i + (cur_font_head->height - dataInfo.height), bg_color );
                    }
    			}
    		}
    	}
    	//LOG_DEBUG("display_ 0x%x---%d\n", *ch,dataInfo.width);
    	if(dataInfo.width != 0 ){
    	    if(*ch < 0xff){
    	       *width = dataInfo.width+CHAR_SPACE;
    	    }else{
    	     *width = cur_font_head->height+CHAR_SPACE;
    	    }
    	}else{
            *width = cur_font_head->height;
    	}
    }else if(cur_font_head->magic == 0x20181218){
        charInfo_t char_info = {0};
        int data_addr = 0;
        int info_addr = cur_sections[i].info_offset + (*ch - cur_sections[i].start_char) * sizeof(charInfo_t);
        
        if(quick_info.init_flag != 1){
            
            
            ret = f_lseek(cur_font_fp, info_addr);
            if(ret != FR_OK){
                LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_FILE_SEEK,__LINE__);
                error = ret;
                goto exit;
            }

            ret = f_read(cur_font_fp, &char_info, sizeof(charInfo_t), (UINT*)&read_size);
        	if(ret != FR_OK){
                LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__,ERR_STR_FILE_READ,__LINE__);
                error = ret;
                goto exit;
            }
        }else{
            u32_t addr = QUICK_READ_START_ADDR + info_addr - quick_info.start_offset;
            ry_memcpy(&char_info,  (void*)addr, sizeof(charInfo_t));
        }
        data_addr = char_info.data_offset;

        
        if(char_info.dataType == 1)
		{
			dataSize = cur_font_head->height*((cur_font_head->height * 4 + 4) / 8);
		}
		else
		{
			dataSize = cur_font_head->height*((cur_font_head->height + 7) / 8);
		}

		temp_buf = (u8_t *)ry_malloc(dataSize);
		if(temp_buf == NULL){
		    LOG_ERROR("[%s]-{%s}-%d\n", __FUNCTION__, ERR_STR_MALLOC_FAIL,__LINE__);
            goto exit;
		}
        memset(temp_buf, 0, dataSize);
        
        if(find_font_cache(*ch) == NULL){
    		f_lseek(cur_font_fp, data_addr);
    		ret = f_read(cur_font_fp, temp_buf, dataSize, &read_size);
    		if(ret != FR_OK){
                LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_READ,__LINE__);
                error = ret;
                goto exit;
            }

            add_to_font_cache(*ch, temp_buf, dataSize);
        }else{
            ry_memcpy(temp_buf, find_font_cache(*ch),dataSize);

        }

        if(char_info.dataType == 0){
            for (i = 0; i < char_info.height; ++i)
        	{
        		for (j = 0; j < char_info.width; ++j)
        		{
        			u8_t tempData = temp_buf[i * ((cur_font_head->height + 7) / 8) + j / 8];
        			//printf("%c",(tempData & (1 << j % 8)) ? '*' : 0);
        			//printf("%c", bitmap.buffer[i * bitmap.width + j] ? '*' : 0);
        			if(tempData & (1 << j % 8))
							{
        			    if(y + i + (cur_font_head->height - char_info.height) > RM_OLED_HEIGHT)
											continue;
									
									setDip(x + j, y + i + (cur_font_head->height - char_info.height), color);
        			}
							else
							{
        			    if(bg_color >> 27) {}
									else {
        			        if(y + i + (cur_font_head->height - char_info.height) > RM_OLED_HEIGHT)
													continue;
											
											setDip(x + j, y + i + (cur_font_head->height - char_info.height), bg_color );
									}
        			}
        		}
        	}
    	//LOG_DEBUG("display_ 0x%x---%d\n", *ch,dataInfo.width);
        	if(char_info.width != 0 ){
        	    if(*ch < 0xff){
        	       *width = char_info.width+CHAR_SPACE_NEW;
        	    }else{
        	     //*width = cur_font_head->height+CHAR_SPACE_NEW;
        	     *width = char_info.width+CHAR_SPACE_NEW;
        	    }
        	}else{
                *width = cur_font_head->height;
        	}
        }else if(char_info.dataType == 1){
            for (i = 0; i < char_info.height; ++i)
			{
				for (j = 0; j < char_info.width; ++j)
				{
					u8_t tempData = temp_buf[i*((cur_font_head->height + 1) / 2) + (j) / 2];
					if(j%2)
					{
						//tempData = tempData & 0x0f;
						tempData = (tempData >> 4)&0xf ;
					}
					else
					{
						//tempData = tempData >> 4;
						tempData = tempData & 0x0f;
					}
					if(tempData != 0){
						if(y + i + (cur_font_head->height - char_info.height) > 240){
							continue;
						}
						int tempColor = color;
						unsigned char * rgb = (unsigned char *)&tempColor;
						rgb[0] = (rgb[0]*tempData) / 0xf;
						rgb[1] = (rgb[1]*tempData) / 0xf;
						rgb[2] = (rgb[2]*tempData) / 0xf;

						setDip(x + j, y + i + (cur_font_head->height - char_info.height), tempColor);
					}else{
						
						// char background transparency fix
						if(bg_color >> 27) {}
						else {						
						
								if(y + i + (cur_font_head->height - char_info.height) > 240){
									continue;
								}
								setDip(x + j, y + i + (cur_font_head->height - char_info.height), bg_color);
						}						
					}
				}
				
			}
			if(char_info.width != 0 ){
        	    if(*ch < 0xff){
        	       *width = char_info.width+CHAR_SPACE_NEW;
        	    }else{
        	     //*width = cur_font_head->height+CHAR_SPACE_NEW;
        	     *width = char_info.width+CHAR_SPACE_NEW;
        	    }
        	}else{
                *width = cur_font_head->height;
        	}
        }       
    }
exit :
    if (temp_buf != NULL){
        ry_free(temp_buf);
    }
    return error;
}

u32_t getCharWidth(u32_t ch)
{
     int error = 0xffff;
    u8_t flag = 0;
    int dataSize = cur_font_head->height * ((cur_font_head->height + 7) / 8) + 2;
    int i = 0, j = 0,ret;
    UINT read_size;
    u8_t* temp_buf = NULL;
    u32_t width= 0;

    if(cur_font_head->magic == 0xDEADBEEF){

        for(i= 0; i < cur_font_head->section_num; i++){
            if ((cur_sections[i].start_char <= ch) && (ch <= cur_sections[i].end_char)){
                flag = 1;
                break;
            }
        }

        if(flag == 0){
            error = 0x01;
            goto exit;
        }

        int data_addr = cur_sections[i].info_offset + (ch - cur_sections[i].start_char) * (dataSize);
        
        ret = f_lseek(cur_font_fp, data_addr);
        if(ret != FR_OK){
            LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_SEEK,__LINE__);
            error = ret;
            goto exit;
        }

        fontDataInfo_t dataInfo;
    	ret = f_read(cur_font_fp, &dataInfo, sizeof(fontDataInfo_t), &read_size);
    	if(ret != FR_OK){
            LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_READ,__LINE__);
            error = ret;
            goto exit;
        }

        return dataInfo.width + CHAR_SPACE;
    }else if(cur_font_head->magic == 0x20181218){
        charInfo_t char_info = {0};
        for(i= 0; i < cur_font_head->section_num; i++){
            if ((cur_sections[i].start_char <= ch) && (ch <= cur_sections[i].end_char)){
                flag = 1;
                break;
            }
        }

        if(flag == 0){
            width = 0;
            error = 0x01;
            goto exit;
        }

        int info_addr = cur_sections[i].info_offset+ (ch - cur_sections[i].start_char) * sizeof(charInfo_t);


        if(quick_info.init_flag != 1){

            ret = f_lseek(cur_font_fp, info_addr);
            if(ret != FR_OK){
                LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_SEEK,__LINE__);
                error = ret;
                goto exit;
            }

            ret = f_read(cur_font_fp, &char_info, sizeof(charInfo_t), &read_size);
        	if(ret != FR_OK){
                LOG_DEBUG("[%s]-{%s}-%d\n",__FUNCTION__, ERR_STR_FILE_READ,__LINE__);
                error = ret;
                goto exit;
            }
        }else{
            u32_t addr = QUICK_READ_START_ADDR + info_addr - quick_info.start_offset;
            ry_memcpy(&char_info,  (void*)addr, sizeof(charInfo_t));
        }


        

        return char_info.width + CHAR_SPACE_NEW;

    }

exit:
    if(ch == ' '){
        width = 12;
    }
    return width;
	
}

u32_t getStringWidth(char * str)
{
    int i = 0;
    u32_t ch = 0;
    char * temp_str = str;
    int len = strlen(str);

    if(str == NULL){
        return 0;
    }
    
    u32_t str_width = 0;
    for (i = 0; i <= len; i++){
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        /*if(ch == '\n'){
            str_width = (str_width/RM_OLED_WIDTH + 1)*RM_OLED_WIDTH;
        }*/
        str_width +=getCharWidth(ch);
    }

   return str_width;
}

u32_t getStringWidthInnerFont(char* str, font_t font)
{
    int i = 0;
    u32_t ch = 0;
    char * temp_str = str;
    int len = strlen(str);

    if(str == NULL){
        return 0;
    }
    
    u32_t str_width = 0;
    for (i = 0; i <= len; i++){
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        /*if(ch == '\n'){
            str_width = (str_width/RM_OLED_WIDTH + 1)*RM_OLED_WIDTH;
        }*/
        str_width += gdispGetCharWidth(ch, font);
    }

   return str_width;
}

bool string_is_num(char * str)
{
    char * temp_str = NULL;
    temp_str = str;
    u32_t ch = 0;
    int i = 0;
    int len = strlen(str);

    for (i = 0; i <= len; i++){
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        

        if((ch == ' ') || (('9' >= ch)&&(ch>='0')) ){
            continue;
        }else{
            return false;
        }


    }
    return true;
    
}


void dispString(void *obj,int x, int y, int width, int height, char * str, void * font, int color, int bg_color, int Justify)
{
    int i = 0;
    int char_width = 0;
    int cur_x = x, cur_y = y;
    u32_t ch = 0;
    int len = strlen(str);
    char * temp_str = NULL;
    char * str_cpy = NULL;
    u32_t str_width = 0;
    u32_t ret = 0;
    u16_t char_count = 0;
    u8_t word_wrap_flag = 0;
    u8_t row_chars = (RM_OLED_WIDTH / height);

    if(str == NULL){
        return;
    }
    if(str == " "){
        return ;
    }
    str_cpy = (char *)ry_malloc(len + 1);

    ry_memcpy(str_cpy, str, len + 1);
    
    temp_str = str_cpy;
    
    if(font){
        gdispGFillStringBox(obj, x, y, width , height, str,  font, color, bg_color, Justify);
    }else {
        //height = 20;
        row_chars = (RM_OLED_WIDTH / height);
        //Justify = justifyLeft;
        ret = InitRyFont(height);

        #if 0
        if(justifyLeft== Justify){
            
        }else if(justifyCenter == Justify){
            str_width = getStringWidth(temp_str);
            if(x == 0){
                x = 60;
            }
            if(x >= str_width/2){
                cur_x = x - str_width/2;
            }else{
                //goto exit;
            }
        }else if(justifyRight == Justify){
            str_width = getStringWidth(temp_str);
            if(x > str_width){
                cur_x = x - str_width;
            }else{
                //goto exit;
            }
            
        }

        for (i = 0; i <= len; i++){
            ch = utf8_getchar(&temp_str);
            if(ch == 0){
                continue;
            }
            
            //LOG_DEBUG("char = 0x%x\n", ch);
            dispChar((int *)&ch, cur_x, cur_y, color, bg_color, &char_width);
            cur_x += char_width;
            /*if( *temp_str == 0){
                break;
            }*/
        }
        #else

        for (i = 0; i <= len; i++){
            ch = utf8_getchar(&temp_str);
            if(ch == 0){
                continue;
            }
            

            if(ch == ' '){
                if(temp_str == &str_cpy[len]){
                    str_cpy[len-1] = '\0';
                    len -=1;
                    break;
                }
            }

            char_count++;

        }
        if(char_count > row_chars){
            temp_str = str_cpy;
            str_width = getStringWidth(temp_str);

            if(str_width > 120){

                Justify = justifyLeft; 
                word_wrap_flag = 1;
                char_count = 0;
                cur_x = 0;
                
                if((x + str_width) < 120){
                    cur_x = x;
                }
            }else{

            }
        }
        temp_str = str_cpy;
        if(justifyLeft== Justify){
            
        }else if(justifyCenter == Justify){
            str_width = getStringWidth(temp_str);
            if(x == 0){
                x = 60;
            }
            if(x >= str_width/2){
                cur_x = x - str_width/2;
            }else{
                //goto exit;
            }
        }else if(justifyRight == Justify){
            str_width = getStringWidth(temp_str);
            if(x > str_width){
                cur_x = x - str_width;
            }else{
                //goto exit;
            }
            
        }

        for (i = 0; i <= len; i++){
            ch = utf8_getchar(&temp_str);
            if(ch == 0){
                continue;
            }

            if(word_wrap_flag){
                //if((char_count%(row_chars) == 0) &&(char_count>0)){
                if(getCharWidth(ch) + cur_x > RM_OLED_WIDTH){
                    cur_y += height + LINE_SPACING;
                    cur_x = 0;
                }
                char_count++;
            }

            if(cur_y + height < 0){

            }else{
            
                //LOG_DEBUG("char = 0x%x\n", ch);
                dispChar((int *)&ch, cur_x, cur_y, color, bg_color, &char_width);
            }
            cur_x += char_width;
            /*if( *temp_str == 0){
                break;
            }*/
        }
        
        #endif
        //DeInitRyFont();
    }
    
exit:
    ry_free(str_cpy);
    return;
}

void dispStringConstantWidth(int x, int y, int width, int height, char * str, void * font,    int color, int bg_color, int Justify)
{
    int i = 0;
    int char_width = 0;
    int cur_x = x, cur_y = y;
    u32_t ch = 0;
    int len = strlen(str);
    char * temp_str = NULL;
    char * str_cpy = NULL;
    u32_t str_width = 0;
    u32_t ret = 0;
    u16_t char_count = 0;
    u8_t word_wrap_flag = 0;
    u8_t row_chars = (RM_OLED_WIDTH / height);

    if(str == NULL){
        return;
    }
    if(str == " "){
        return ;
    }
    str_cpy = (char *)ry_malloc(len + 1);

    ry_memcpy(str_cpy, str, len + 1);
    
    temp_str = str_cpy;
    
    if(font){
        return ;
    }else {
        //height = 20;
        row_chars = (RM_OLED_WIDTH / height);
        //Justify = justifyLeft;
        ret = InitRyFont(height);


        for (i = 0; i <= len; i++){
            ch = utf8_getchar(&temp_str);
            if(ch == 0){
                continue;
            }
            

            if(ch == ' '){
                if(temp_str == &str_cpy[len]){
                    str_cpy[len-1] = '\0';
                    len -=1;
                    break;
                }
            }

            char_count++;

        }
        if(char_count > row_chars){
            temp_str = str_cpy;
            str_width = getStringCharCount(temp_str)*QUICK_READ_FONT_HEIGHT;

            if(str_width > 120){

                Justify = justifyLeft; 
                word_wrap_flag = 1;
                char_count = 0;
                cur_x = 0;
                
                if((x + str_width) < 120){
                    cur_x = x;
                }
            }else{

            }
        }
        temp_str = str_cpy;
        if(justifyLeft== Justify){
            
        }else if(justifyCenter == Justify){
            str_width = getStringCharCount(temp_str)*QUICK_READ_FONT_HEIGHT;
            if(x == 0){
                x = 60;
            }
            if(x >= str_width/2){
                cur_x = x - str_width/2;
            }else{
                //goto exit;
            }
        }else if(justifyRight == Justify){
            str_width = getStringCharCount(temp_str)*QUICK_READ_FONT_HEIGHT;
            if(x > str_width){
                cur_x = x - str_width;
            }else{
                //goto exit;
            }
            
        }

        for (i = 0; i <= len; i++){
            ch = utf8_getchar(&temp_str);
            if(ch == 0){
                continue;
            }

            if(word_wrap_flag){
                //if((char_count%(row_chars) == 0) &&(char_count>0)){
                if(getCharWidth(ch) + cur_x > RM_OLED_WIDTH){
                    cur_y += height + LINE_SPACING;
                    cur_x = 0;
                }
                char_count++;
            }

            if(cur_y + height < 0){

            }else{
            
                //LOG_DEBUG("char = 0x%x\n", ch);
                dispChar((int *)&ch, cur_x, cur_y, color, bg_color, &char_width);
            }
            cur_x += QUICK_READ_FONT_HEIGHT;
        }
        
    }
    
exit:
    ry_free(str_cpy);
    return;
}



void drawStringLine(int x, int y, int width, int height, char * str, int color, int bg_color)
{
    int i = 0;
    int char_width = 0;
    int cur_x = x, cur_y = y;
    u32_t ch = 0;
    int len = strlen(str);
    char * temp_str = str;
    u32_t str_width = 0;
    u32_t ret = 0;
    u16_t char_count = 0;
    u8_t word_wrap_flag = 0;
    u8_t row_chars = (RM_OLED_WIDTH / height);

    ret = InitRyFont(height);

    str_width = getStringWidth(str);
   
    for (i = 0; i <= len; i++){
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        
        //LOG_DEBUG("char = 0x%x\n", ch);
        dispChar((int *)&ch, cur_x, cur_y, color, bg_color, &char_width);
        cur_x += char_width;
        /*if( *temp_str == 0){
            break;
        }*/
    }
}



u32_t getStringCharCount(char * str)
{
    if(str == NULL){
        return 0;
    }
    int i = 0;
    int len = strlen(str);
    char * temp_str = str;
    u32_t ch = 0;
    u32_t char_count = 0;
    
    for (i = 0; i <= len; i++){
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        char_count++;
    }
    
    return char_count;
}

u32_t getCharOffset(char * str, u32_t num)
{
    if(str == NULL){
        return 0;
    }
    int i = 0;
    int len = strlen(str);
    char * temp_str = str;
    u32_t ch = 0;
    u32_t char_count = 0;
    
    for (i = 0; i <= len; i++){
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        char_count++;
        if(char_count == num){
            break;
        }
    }
    
    return (temp_str - str);
}

u32_t getWidthOffset(char  * str, u32_t width)
{
    int i = 0;
    u32_t ch = 0;
    char * temp_str = str;
    int len = 0;
    u32_t ch_width = 0;
    char *last_temp_str = temp_str;

    if(str == NULL){
        return 0;
    }

    len = strlen(str);
    
    u32_t str_width = 0;
    for (i = 0; i <= len; i++){
        last_temp_str = temp_str;
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        ch_width = getCharWidth(ch);
        if(str_width + ch_width >= width){
            temp_str = last_temp_str;
            break;
        }else{
            if(((str_width + ch_width)/RM_OLED_WIDTH > str_width / RM_OLED_WIDTH)){
                str_width = (str_width/RM_OLED_WIDTH + 1)*RM_OLED_WIDTH;
            }
            str_width += ch_width;
        }
    }

   return (temp_str - str);    
}

u32_t getWidthOffsetInnerFont(char* str, u32_t width, font_t font)
{
    int i = 0;
    u32_t ch = 0;
    char * temp_str = str;
    int len = 0;
    u32_t ch_width = 0;
    char *last_temp_str = temp_str;

    if(str == NULL){
        return 0;
    }

    len = strlen(str);
    
    u32_t str_width = 0;
    for (i = 0; i <= len; i++){
        last_temp_str = temp_str;
        ch = utf8_getchar(&temp_str);
        if(ch == 0){
            continue;
        }
        ch_width = gdispGetCharWidth(ch, font);
        if(str_width + ch_width >= width){
            temp_str = last_temp_str;
            break;
        }else{
            if(((str_width + ch_width)/RM_OLED_WIDTH > str_width / RM_OLED_WIDTH)){
                str_width = (str_width/RM_OLED_WIDTH + 1)*RM_OLED_WIDTH;
            }
            str_width += ch_width;
        }
    }

   return (temp_str - str);    
}


u16_t utf8_getchar(char ** str)
{
    uint8_t c;
    uint8_t tmp, seqlen;
    uint16_t result;

    if(str == NULL){
        return 0;
    }
    
    c = **str;
    if (!c)
        return 0;
    
    (*str)++;
    
    if ((c & 0x80) == 0)
    {
        /* Just normal ASCII character. */
        return c;
    }
    else if ((c & 0xC0) == 0x80)
    {
        /* Dangling piece of corrupted multibyte sequence.
         * Did you cut the string in the wrong place?
         */
        return c;
    }
    else if ((**str & 0xC0) == 0xC0)
    {
        /* Start of multibyte sequence without any following bytes.
         * Silly. Maybe you are using the wrong encoding.
         */
        return c;
    }
    else
    {
        /* Beginning of a multi-byte sequence.
         * Find out how many characters and combine them.
         */
        seqlen = 2;
        tmp = 0x20;
        result = 0;
        while ((c & tmp) && (seqlen < 5))
        {
            seqlen++;
            tmp >>= 1;
            
            result = (result << 6) | (**str & 0x3F);
            (*str)++;
        }
        
        result = (result << 6) | (**str & 0x3F);
        (*str)++;
        
        result |= (c & (tmp - 1)) << ((seqlen - 1) * 6);
        return result;
    }
}






























