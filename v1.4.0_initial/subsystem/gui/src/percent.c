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
#include "gui.h"
#include "gui_img.h"
#include "stdio.h"


void  ry_gui_disp_percent(void * font, int height, int width, int x, int y, int color, uint8_t value)
{
    
    char * percent_string = (char *)ry_malloc(100);
    if(percent_string == NULL){
        LOG_ERROR("[%s]-{%s}\n", __FUNCTION__, ERR_STR_MALLOC_FAIL);
        return ;
    }
    ry_memset(percent_string, 0, 100);
    int percent_value = (value > 100)?(100):(value);
    sprintf(percent_string, "%d%%", percent_value);
                        
    gdispFillStringBox( 0, 
                        y, 
                        font_roboto_20.width, font_roboto_20.height,
                        percent_string, 
                        (void*)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );                        

    ry_free(percent_string);   
}


void ry_gui_refresh_area_y(int start_y, int end_y)
{
    img_pos_t pos;
    //pos->buffer = frame_buffer[];
    pos.x_start = 0;
    pos.x_end = 119;
    pos.y_start = (start_y >= IMG_MAX_HEIGHT)?(IMG_MAX_HEIGHT - 1):(start_y);
    pos.y_end = (end_y >= IMG_MAX_HEIGHT)?(IMG_MAX_HEIGHT - 1):(end_y);
    pos.buffer = &frame_buffer[pos.y_start * 3 * 120];
    
    write_buffer_to_oled(&pos);
}










