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

#include "ry_hal_mcu.h"
#include "gui_img.h"
#include "gui_constraction.h"


#if 0
cwin_params_t cwin_menu_array[] = {
    {0, {"menu_09_setting.bmp",    "menu_02_hrm.bmp"  },       {0, 128}, {"设置", "心率"},  {120 - 30, 240 - 30}},
    {1, {"menu_02_hrm.bmp",        "menu_03_sport.bmp"},       {0, 128}, {"心率", "运动"},  {120 - 30, 240 - 30}},
    {2, {"menu_03_sport.bmp",      "menu_01_card.bmp"},        {0, 128}, {"运动", "卡包"},  {120 - 30, 240 - 30}},   
    {3, {"menu_01_card.bmp",       "m_setting_00_doing.bmp"},  {0, 128}, {"卡包", "表盘"},  {120 - 30, 240 - 30}},
    {4, {"m_setting_00_doing.bmp", "menu_05_weather.bmp"},     {0, 128}, {"表盘", "天气"},  {120 - 30, 240 - 30}},    
    {5, {"menu_05_weather.bmp",    "menu_08_data.bmp"},        {0, 128}, {"天气", "数据"},  {120 - 30, 240 - 30}},   
    {6, {"menu_08_data.bmp",       "menu_04_mijia.bmp"},       {0, 128}, {"数据", "米家"},  {120 - 30, 240 - 30}},
    {0, {"menu_04_mijia.bmp",      "menu_09_setting.bmp"  },   {0, 128}, {"米家", "设置"},  {120 - 30, 240 - 30}},
};
#endif 

cwin_params_t cwin_menu_array[] = {
    {0, {"menu_01_card.bmp",       "menu_03_sport.bmp"  },     {0, 128}, {"卡包", "运动"},  {120 - 30, 240 - 30}},
    {1, {"menu_03_sport.bmp",      "menu_02_hrm.bmp"},         {0, 128}, {"运动", "心率"},  {120 - 30, 240 - 30}},
    {2, {"menu_02_hrm.bmp",        "menu_04_mijia.bmp"},       {0, 128}, {"心率", "米家"},  {120 - 30, 240 - 30}},   
    {3, {"menu_04_mijia.bmp",      "menu_05_weather.bmp"},     {0, 128}, {"米家", "天气"},  {120 - 30, 240 - 30}},
    {4, {"menu_05_weather.bmp",    "menu_08_data.bmp"},        {0, 128}, {"天气", "数据"},  {120 - 30, 240 - 30}},    
    {5, {"menu_08_data.bmp",       "menu_09_setting.bmp"},     {0, 128}, {"数据", "设置"},  {120 - 30, 240 - 30}},   
    {6, {"menu_09_setting.bmp",    "m_setting_00_doing.bmp"},  {0, 128}, {"设置", "表盘"},  {120 - 30, 240 - 30}},
    {0, {"m_setting_00_doing.bmp", "menu_01_card.bmp"  },      {0, 128}, {"表盘", "卡包"},  {120 - 30, 240 - 30}},
};

void _cwin_update(cwin_params_t *cwin_menu)
{
    u32_t _startT;
    //_startT = ry_hal_clock_time();
    gdispClear(Black);
    //LOG_DEBUG("Clear Black. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));
    u32_t _startT1 = ry_hal_clock_time();    
#if  IMG_INTERNAL_ONLY
	gdispImage myImage;
    if (cwin_menu->image[0] != ""){
        //_startT = ry_hal_clock_time();
        gdispImageOpenFile(&myImage, cwin_menu->image[0]);
        //LOG_DEBUG("Open. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));

        //_startT = ry_hal_clock_time();
        gdispImageDraw(&myImage,                 \
                    (120 - myImage.width) >> 1,  \
                    cwin_menu->image_pos[0],     \
                    myImage.width,               \
                    myImage.height,              \
                    0,                           \
                    0);
        //LOG_DEBUG("Draw. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));

        //_startT = ry_hal_clock_time();
        gdispImageClose(&myImage);
        //LOG_DEBUG("Close. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));
    }

    if (cwin_menu->image[1] != ""){
        gdispImageOpenFile(&myImage, cwin_menu->image[1]);
        gdispImageDraw(&myImage,                 \
                    (120 - myImage.width) >> 1,  \
                    cwin_menu->image_pos[1],     \
                    myImage.width,               \
                    myImage.height,              \
                    0,                           \
                    0);
        gdispImageClose(&myImage);
    }
#else
    if (cwin_menu->menu_idx >= sizeof(cwin_menu_array)/sizeof(cwin_params_t) - 1){
        img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE + (RM_OLED_IMG_24BIT>>1) * cwin_menu->menu_idx, 0, RM_OLED_IMG_24BIT>>1);
        img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE + (RM_OLED_IMG_24BIT>>1) * 0, RM_OLED_IMG_24BIT>>1, RM_OLED_IMG_24BIT>>1);
    }else{
        img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE + (RM_OLED_IMG_24BIT>>1) * cwin_menu->menu_idx, 0, RM_OLED_IMG_24BIT);
    }
    // img_exflash_to_framebuffer(cwin_menu->image[0], cwin_menu->image_pos[0]);
    // img_exflash_to_framebuffer(cwin_menu->image[1], cwin_menu->image_pos[1]);
#endif
    //_startT = ry_hal_clock_time();
    //LOG_DEBUG("Img draw. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT1));

#if IMG_INTERNAL_ONLY    
    if (cwin_menu->info[0] != ""){
        gdispFillStringBox(0,                    \
                        cwin_menu->info_pos[0],  \
                        font_langting.width,     \
                        font_langting.height,    \
                        cwin_menu->info[0],      \
                        font_langting.font,      \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }

    if (cwin_menu->info[1] != ""){
        gdispFillStringBox(0,                    \
                        cwin_menu->info_pos[1],  \
                        font_langting.width,     \
                        font_langting.height,    \
                        cwin_menu->info[1],      \
                        font_langting.font,      \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }
#endif
    //LOG_DEBUG("String. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));

    //_startT = ry_hal_clock_time();
	//gdisp_update();
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    //LOG_DEBUG("Update. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));
}

void _cwin_prepare(cwin_params_t *cwin_menu)
{
    u32_t _startT;

    //_startT = ry_hal_clock_time();
    gdispClear(Black);
    //LOG_DEBUG("Clear Black. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));

    u32_t _startT1 = ry_hal_clock_time();
#if IMG_INTERNAL_ONLY
	gdispImage myImage;
    if (cwin_menu->image[0] != ""){
        //_startT = ry_hal_clock_time();
        gdispImageOpenFile(&myImage, cwin_menu->image[0]);
        //LOG_DEBUG("Open. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));

        //_startT = ry_hal_clock_time();
        gdispImageDraw(&myImage,                 \
                    (120 - myImage.width) >> 1,  \
                    cwin_menu->image_pos[0],     \
                    myImage.width,               \
                    myImage.height,              \
                    0,                           \
                    0);
        //LOG_DEBUG("Draw. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));

        //_startT = ry_hal_clock_time();
        gdispImageClose(&myImage);
        //LOG_DEBUG("Close. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));
    }

    if (cwin_menu->image[1] != ""){
        gdispImageOpenFile(&myImage, cwin_menu->image[1]);
        gdispImageDraw(&myImage,                 \
                    (120 - myImage.width) >> 1,  \
                    cwin_menu->image_pos[1],     \
                    myImage.width,               \
                    myImage.height,              \
                    0,                           \
                    0);
        gdispImageClose(&myImage);
    }
#else
    if (cwin_menu->menu_idx >= sizeof(cwin_menu_array)/sizeof(cwin_params_t) - 1){
        img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE + (RM_OLED_IMG_24BIT>>1) * cwin_menu->menu_idx, 0, RM_OLED_IMG_24BIT>>1);
        img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE + (RM_OLED_IMG_24BIT>>1) * 0, RM_OLED_IMG_24BIT>>1, RM_OLED_IMG_24BIT>>1);
    }else{
        img_internal_flash_to_framebuffer(FLASH_ADDR_IMG_RESOURCE + (RM_OLED_IMG_24BIT>>1) * cwin_menu->menu_idx, 0, RM_OLED_IMG_24BIT);
    }
#endif
    //LOG_DEBUG("Img draw. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT1));
#if IMG_INTERNAL_ONLY
    //_startT = ry_hal_clock_time();
    if (cwin_menu->info[0] != ""){
        gdispFillStringBox(0,                    \
                        cwin_menu->info_pos[0],  \
                        font_langting.width,     \
                        font_langting.height,    \
                        cwin_menu->info[0],      \
                        font_langting.font,      \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }

    if (cwin_menu->info[1] != ""){
        gdispFillStringBox(0,                       \
                        cwin_menu->info_pos[1],  \
                        font_langting.width,     \
                        font_langting.height,    \
                        cwin_menu->info[1],      \
                        font_langting.font,      \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }
#endif
    //LOG_DEBUG("String. %d ms.\r\n", ry_hal_calc_ms(ry_hal_clock_time()-_startT));
}


typedef struct {
	uint8_t     *image[2];
    uint8_t     image_pos[2];
    uint8_t     *info[2];
    uint8_t     info_pos[2];
} cmodule_params_t;

typedef enum {
    CMODULE_SETTING = 0,
    CMODULE_HRM,
    CMODULE_SPORT,
    CMODULE_CARD,    
    CMODULE_UPGRAD,        
    CMODULE_WEATER,        
    CMODULE_DATA,        
    
}e_cmodule_t;

cmodule_params_t cwin_module_array[] = {
    {{"menu_09_setting.bmp", ""},    {0, 0},  {"64", "setting"},    {120, 180}},        
    {{"menu_02_hrm.bmp", ""},        {0, 0},  {"64", "keep still"}, {120, 180}},
    {{"menu_03_sport.bmp", ""}, {0, 0},  {"64", "step"},       {120, 180}},
    {{"m_card_02_beijing.png", ""},  {0, 0},  {"nfc", "ready"},     {120, 180}},
    {{"m_upgrade_00_enter.png", ""}, {0, 0},  {"upgrade", "ready"}, {120, 180}},
    {{"m_weather_00_sunny.bmp", ""}, {40, 0}, {"sunny", "-10 to 5"},{120, 180}},    
    {{"m_data_00_step.png", ""},     {40, 0}, {"calorie", "14512"},  {120, 180}},
}; 

void _cmodule_update(cmodule_params_t *cmodule)
{
    gdispClear(Black);
#if IMG_INTERNAL_ONLY    
	gdispImage myImage;
	gdispImageOpenFile(&myImage, cmodule->image[0]);
    gdispImageDraw(&myImage,                    \
                   (120 - myImage.width) >> 1,  \
                   cmodule->image_pos[0],       \
				   myImage.width,               \
                   myImage.height,              \
                   0,                           \
                   0);
	gdispImageClose(&myImage);
#else
    u32_t temp;
    //img_exflash_to_framebuffer(cmodule->image[0], cmodule->image_pos[0], &temp, &temp);
#endif
    gdispFillStringBox(0,                       \
                       cmodule->info_pos[0],    \
                       fwidth_info,             \
                       fheight_info,            \
                       cmodule->info[0],        \
                       font_info,               \
                       White,                   \
                       Black,                   \
                       justifyCenter);

    gdispFillStringBox(0,                       \
                       cmodule->info_pos[1],    \
                       fwidth_info,             \
                       fheight_info,            \
                       cmodule->info[1],        \
                       font_info,               \
                       White,                   \
                       Black,                   \
                       justifyCenter);
	gdisp_update();
}

typedef struct {
	uint8_t     *image[2];
    uint8_t     image_pos[2];
    uint8_t     *info[4];
    uint8_t     info_pos[4];
} cnotify_params_t;


typedef enum {
    CNOTIFY_RTC = 0,
    CNOTIFY_MSG,
    CNOTIFY_RSVD,
}e_cnotify_t;

cnotify_params_t cwin_notify_array[] = {
    {{"menu_06_clock.bmp", ""}, {0, 0}, {"12:29:59", "100%", "10001", ""},    {100, 155, 210}},
};

void _cnotify_update(cnotify_params_t *cnotify)
{
    gdispClear(Black);
#if IMG_INTERNAL_ONLY    
	gdispImage myImage;
	gdispImageOpenFile(&myImage, cnotify->image[0]);
    gdispImageDraw(&myImage,                    \
                   (120 - myImage.width) >> 1,  \
                   cnotify->image_pos[0],       \
				   myImage.width,               \
                   myImage.height,              \
                   0,                           \
                   0);
	gdispImageClose(&myImage);
#else
    uint8_t w, h;
    img_exflash_to_framebuffer(cnotify->image[0], 0, 0, &w, &h, d_justify_left);
#endif
    uint32_t i;
    for (i = 0; i < 3; i ++){
        gdispFillStringBox(0,                    \
                        cnotify->info_pos[i],    \
                        fwidth_info,             \
                        fheight_info,            \
                        cnotify->info[i],        \
                        font_info,               \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }                  
	gdisp_update();
}

void _cnotify_prepare(cnotify_params_t *cnotify)
{
    gdispClear(Black);
#if IMG_INTERNAL_ONLY    
	gdispImage myImage;
	gdispImageOpenFile(&myImage, cnotify->image[0]);
    gdispImageDraw(&myImage,                    \
                   (120 - myImage.width) >> 1,  \
                   cnotify->image_pos[0],       \
				   myImage.width,               \
                   myImage.height,              \
                   0,                           \
                   0);
	gdispImageClose(&myImage);
#else
    uint8_t w, h;
    img_exflash_to_framebuffer(cnotify->image[0], 0, 0, &w, &h, d_justify_left);
#endif
    uint32_t i;
    for (i = 0; i < 3; i ++){
        gdispFillStringBox(0,                    \
                        cnotify->info_pos[i],    \
                        fwidth_info,             \
                        fheight_info,            \
                        cnotify->info[i],        \
                        font_info,               \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }                  
}



typedef enum {
    CINFO_DEV = 0,
}e_info_t;

typedef struct {
	uint8_t     *image[2];
    uint8_t     image_pos[2];
    uint8_t     *info[4];
    uint8_t     info_pos[4];
} cinfo_params_t;

cinfo_params_t cinfo_array[] = {
    {{"m_notice_02_info.bmp", ""}, {30, 0}, { "device info", "0A:0B:0C:0D", "AA55", "20180210"}, {80, 140, 170, 200}},
};

void _cinfo_update(cinfo_params_t *cinfo)
{
    gdispClear(Black);

#if IMG_INTERNAL_ONLY    
	gdispImage myImage;

	gdispImageOpenFile(&myImage, cinfo->image[0]);
    gdispImageDraw(&myImage,                    \
                   (120 - myImage.width) >> 1,  \
                   cinfo->image_pos[0],         \
				   myImage.width,               \
                   myImage.height,              \
                   0,                           \
                   0);
	gdispImageClose(&myImage);
#else
    u32_t temp;
    //img_exflash_to_framebuffer(cinfo->image[0], cinfo->image_pos[0], &temp, &temp);
#endif    
    font_t	font1 = gdispOpenFont("UI2");

    uint32_t i;
    for (i = 0; i < 4; i ++){
        gdispFillStringBox(0,                    \
                        cinfo->info_pos[i],      \
                        fwidth_info,             \
                        fheight_info,            \
                        cinfo->info[i],          \
                        font_info,               \
                        White,                   \
                        Black,                   \
                        justifyCenter);
    }

	gdisp_update();
}
