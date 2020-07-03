#ifndef __RY_GUI_DISP__

#define __RY_GUI_DISP__

#include "ry_type.h"







void  ry_gui_disp_percent(void * font, int height, int width, int x, int y, int color, uint8_t value);

void ry_gui_refresh_area_y(int start_y, int end_y);

void distory_loading(void);
void create_loading(int animation);
void loading_refresh_handle(void);

















#endif

