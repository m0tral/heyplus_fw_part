#include "ry_barcode_code128.h"

#include "ry_font.h"
#include "ryos.h"


typedef struct code128Data{
    u8_t len;
    int * data;
}code128Data_t;


#define MAX_BARCODE_WIDTH       230

void test_code128(void )
{
    code128Data_t code_data = {0};
    code_data.data = (int *)ry_malloc(256 * sizeof(int));
    ry_memset(code_data.data, 0 , 256 * sizeof(int));
    char * temp_buf = (char *)ry_malloc(256);
    ry_memset(temp_buf, 0, 256);
    size_t result_size = code128_encode_gs1("2824 1963 6204 1453233", temp_buf, 256);

    LOG_DEBUG("result_size = %d\n", result_size);
 
    int i = 0;

    int start_y = 5;

    int start_x = 10, end_x = 100;


    set_frame(0, 239, 0xC0);
    int last_val = -1;
    int line_scale = (MAX_BARCODE_WIDTH * 100) / result_size;

    for(i = 0; i<result_size; i++){

        if(temp_buf[i]){
            setRect(end_x, start_y ,start_x,  start_y, 0);
        }else{
            setRect(end_x, start_y ,start_x,  start_y, 0xC0C0C0);
        }

        start_y++;

        if(last_val == -1){
            last_val = temp_buf[i];
        }else{
            
            if(last_val != temp_buf[i]){
                code_data.len++;
                last_val = temp_buf[i];
            }
            
            {
                if(temp_buf[i]){
                    code_data.data[code_data.len]--;
                }else{
                    code_data.data[code_data.len]++;
                }
            }
        
        }
    }

    /*int line_num = 0;
    int line_last = 0, line_cur = 0;
    int present_color = 0, color = 0;;
    for(i = 0; i < code_data.len; i++){
        LOG_DEBUG("%d\t",code_data.data[i]);

        if(code_data.data[i] > 0){
            line_num = (code_data.data[i] * line_scale - line_last)/100 ;
            line_cur = (code_data.data[i] * line_scale - line_last)%100;
            setRect(end_x, start_y + line_num ,start_x,  start_y, 0xFFFFFF);
            start_y += line_num;
            present_color = 0xFF*line_cur/100;
            color = present_color<<16 | present_color << 8 | present_color;
            setRect(end_x, start_y ,start_x,  start_y, color);
            start_y++;
            line_last = 100 - line_cur;
        }else{
            line_num = (-code_data.data[i] * line_scale)/100;
            line_cur = (-code_data.data[i] * line_scale - line_last)%100;
            setRect(end_x, start_y + line_num ,start_x,  start_y, 0);
            start_y += line_num;
            present_color = 0xFF*(100 - line_cur)/100;
            color = present_color<<16 | present_color << 8 | present_color;
            setRect(end_x, start_y ,start_x,  start_y, color);
            start_y++;
            line_last = 100 - line_cur;
        }
        gdisp_update();
    }*/
    

    gdisp_update();

    ry_free(temp_buf);
    ry_free(code_data.data);

    return ;
}

























