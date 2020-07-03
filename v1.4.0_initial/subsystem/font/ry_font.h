#ifndef __RY_FONT_H__

#define __RY_FONT_H__

#include "ff.h"
#include "ry_type.h"
#include "gfx.h"


#define RY_FONT_HEIGHT_22       "ry_font22.ryf"
#define RY_FONT_HEIGHT_20       "ry_font20.ryf"
#define LINE_SPACING            6

#define FONTS_RY_CUSTOMIZE      0
#define FONT_MAX_SECTION_NUM    20

#define QUICK_READ_FONT_HEIGHT  22

#define QUICK_READ_START_ADDR   0xc8100 //0xc0100//0xb8000
#define QUICK_READ_INFO_ADDR    0xc8000 //0xc0000//0xb7900



typedef struct fontHead{
	u32_t magic;
	u32_t len;
	u32_t width;
	u32_t height;
	u32_t data_size;
	u32_t section_num;

}fontHead_t;

typedef struct fontSection{
	u32_t start_char;
	u32_t end_char;
	u32_t info_offset;
}fontSection_t;

typedef struct fontDataInfo{
	u8_t width;
	u8_t height;
	//u8_t* data;
}fontDataInfo_t;

typedef struct charInfo{
	uint8_t width;
	uint8_t height;
	uint8_t dataType;
	uint32_t data_offset;
}charInfo_t;

typedef struct quickRead{
    uint8_t  init_flag;
    uint32_t start_offset;
}quickRead_t;


#define FONT_CACHE_CELL_MAX         10
typedef struct fontDispCacheCell{
    uint32_t code;
    uint8_t * buf;
}fontDispCacheCell_t;

typedef struct fontDispCache{
    u8_t count;
    fontDispCacheCell_t cell[FONT_CACHE_CELL_MAX];
}fontDispCache_t;



#if 0
typedef enum justify {
	justifyLeft = 0x00,			/**< Justify Left (the default) */
	justifyCenter = 0x01,		/**< Justify Center */
	justifyRight = 0x02,		/**< Justify Right */
	justifyTop = 0x10,			/**< Justify Top */
	justifyMiddle = 0x00,		/**< Justify Middle (the default) */
	justifyBottom = 0x20,		/**< Justify Bottom */
	justifyWordWrap = 0x00,		/**< Word wrap (the default if GDISP_NEED_TEXT_WORDWRAP is on) */
	justifyNoWordWrap = 0x40,	/**< No word wrap */
	justifyPad = 0x00,			/**< Pad the text box (the default) */
	justifyNoPad = 0x04			/**< No padding the text box */
} justify_t;
#endif


u16_t utf8_getchar(char ** str);

void dispString(void *obj,int x, int y, int width, int height, char * str, void * font,    int color, int bg_color, int Justify);

u32_t getStringCharCount(char * str);
u32_t getCharOffset(char * str, u32_t num);
void DeInitRyFont(void);
void setDip(int x, int y, int color);
void setRect(int max_x,int max_y, int min_x, int min_y, int color);
void setHLine(int x_start, int x_end, int y_vert, int color);

u32_t getWidthOffset(char* str, u32_t width);
u32_t getWidthOffsetInnerFont(char* str, u32_t width, font_t font);

void drawStringLine(int x, int y, int width, int height, char * str, int color, int bg_color);

u32_t getStringWidth(char * str);
u32_t getStringWidthInnerFont(char* str, font_t font);
    
bool string_is_num(char * str);
void fontQuickReadInit(void);

void dispStringConstantWidth(int x, int y, int width, int height, char * str, void * font,    int color, int bg_color, int Justify);






#endif

