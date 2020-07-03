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

 /*
 * GLOBAL VARIABLES
 *******************************************************************************
 */
//gui_font_t font_langting;
// gui_font_t font_langting_light;
gui_font_t font_roboto_20;
gui_font_t font_roboto_60;
gui_font_t font_roboto_44;
gui_font_t font_roboto_32;
gui_font_t font_roboto_16;
gui_font_t font_roboto_12;
gui_font_t font_roboto_14;
//gui_font_t font_langting_lb16;

static inline void gui_delay_ms(int ms)
{
#if (!WIN32_SIM_MK)	
	ryos_delay_ms(ms);
#else
	gfxSleepMilliseconds(ms);		
#endif
}

#if (!WIN32_SIM_MK)		
	#define	_gui_log_debug 	LOG_DEBUG
#else
	void _gui_log_debug(const char *format, ...)
	{
		printf(format);
	}
#endif

#if GFX_USE_GFILE
#define SHOW_ERROR(color)		gdispFillArea(errx, erry, errcx, errcy, color)

void gdisp_update(void)
{
#if (!WIN32_SIM_MK)	
	update_buffer_to_oled();
#endif	
}

int gui_disp_gif(void)
{
	static gdispImage myImage;
	coord_t			  swidth, sheight, errx, erry, errcx, errcy;
	delaytime_t		  delay;

	swidth = gdispGetWidth();			// Get the display dimensions
	sheight = gdispGetHeight();

	errx = swidth - 10;					// Work out our error indicator area
	erry = 0;
	errcx = 10;
	errcy = sheight;

	// Set up IO for our image
	if (!(gdispImageOpenFile(&myImage, "timg.gif") & GDISP_IMAGE_ERR_UNRECOVERABLE)) {
		// Adjust the error indicator area if necessary
		if (myImage.width > errx && myImage.height < sheight) {
			errx = 0; erry = sheight-10;    
			errcx = swidth; errcy = 10;
		}
		while(1) {
			if (gdispImageDraw(&myImage, 0, 0, myImage.width, myImage.height, 0, 0) != GDISP_IMAGE_ERR_OK) {
				SHOW_ERROR(Orange);
				break;
			}			
			delay = gdispImageNext(&myImage);	
#if	WIN32_SIM_MK		
			if (delay == TIME_INFINITE) {
				SHOW_ERROR(Green);
				break;
			}
			if (delay != TIME_IMMEDIATE){
				gfxSleepMilliseconds(delay);	
			}
#endif			
		}
		gdispImageClose(&myImage);
	} //end of openfile succ
	else{
		SHOW_ERROR(Red);
	}

	return 0;
}
#endif

void gui_disp_img_demo(void)
{
#if (GDISP_NEED_IMAGE)
	coord_t	swidth, sheight;
	swidth = gdispGetWidth();
	sheight = gdispGetHeight();
	static gdispImage myImage;
	static int i = 0;

	gdispImageOpenFile(&myImage, icon_img[i]);
	gdispImageDraw(&myImage, 0, 28, swidth, sheight, 0, 0);
	gdisp_update();
	if ((++i) >= (sizeof(icon_img) >> 2)){
		i = 0;
	}
#endif
}

void demo_widget_button_creat(void)
{
#if 0
	// Set the widget defaults
	gwinSetDefaultFont(gdispOpenFont("UI2"));
	gwinSetDefaultStyle(&WhiteWidgetStyle, FALSE);

	static GHandle		ghButton1;
	static GHandle		ghButton2;
	GWidgetInit		wi;
	GEvent* pe;
	gwinWidgetClearInit(&wi);

	wi.g.show = FALSE;
	wi.g.width = 60; 
	wi.g.height = 30; 
	wi.g.y = 5;
	wi.g.x = 6; 
	wi.text = "BTN1";
	ghButton1 = gwinButtonCreate(0, &wi);

	wi.g.show = FALSE;
	wi.g.width = 60; 
	wi.g.height = 30; 
	wi.g.y = 35;
	wi.g.x = 6; 
	wi.text = "BTN2";
	ghButton2 = gwinButtonCreate(0, &wi);

#if (WIN32_SIM_MK)
	// gwinAttachToggle(ghButton1, 0, 0);
	// gwinAttachToggle(ghButton1, 1, 1);
	void init_ginput_toggle(void);
	init_ginput_toggle();
#endif	
	// We want to listen for widget events
	geventListenerInit(&gl);
	gwinAttachListener(&gl);
#endif
}

#if (GWIN_NEED_CONSOLE && GWIN_NEED_WIDGET)
void widget_img_creat(void)
{
	GWidgetInit	  wi;
	wi.g.show = TRUE;
	wi.g.x = 0; 
	wi.g.y = 0; 
	wi.g.width = GDISP_SCREEN_WIDTH; 
	wi.g.height = GDISP_SCREEN_HEIGHT;
	ghImage1 = gwinImageCreate(0, &wi.g);
#if (!WIN32_SIM_MK)		
	gwinImageOpenFile(ghImage1, icon_img[0]);
	// gwinImageCache(ghImage1);	
#else
	gwinImageOpenFile(ghImage1, icon_img[0]);		
#endif	
}


GHandle GW1;
void gwin_console_creat(void)
{
	GWindowInit	  wi;
	wi.show = TRUE;
	wi.x = 0; 
	wi.y = 0; 
	wi.width  = gdispGetWidth(); 
	wi.height = gdispGetHeight();
	GW1 = gwinConsoleCreate(0, &wi);
	font_t	font1 = gdispOpenFont("DejaVuSans12_aa");
	gwinSetFont(GW1, font1);
	gwinSetColor(GW1, Gray);
	gwinSetBgColor(GW1, Black);

	gwinClear(GW1);
	uint8_t i;
	for(i = 0; i < 10; i++) {
		gwinPrintf(GW1, "Hello Ryeex sake.\n");
		gdisp_update();
		gui_delay_ms(50);	
	}
}

void widget_button_process(void)
{
		// ghButton1->y = 128 - gui_img_offset;
		// gwinRedrawDisplay(GDISP, TRUE);
#if 0
		pe = geventEventWait(&gl, TIME_INFINITE);
		switch(pe->type) {
			case GEVENT_GWIN_BUTTON:
				// if (((GEventGWinButton*)pe)->gwin == ghButton1) 
				{
#if (WIN32_SIM_MK)
					printf("get button event.\n");	// Our button has been pressed
#endif					
				}
				break;

			default:
				break;
		}
#endif	
}

void gui_widget_creat(void)
{
	demo_widget_button_creat();
	widget_img_creat();
}
#endif

void gui_disp_img_at(uint8_t idx, uint8_t pos)
{
#if (GDISP_NEED_IMAGE)
	if (idx >= (sizeof(icon_img) >> 2)){
		idx = 0;
	}
	gdispClear(Black);
	gdispImage myImage;
	_gui_log_debug("[gui_disp_img_at]disp img:%s, @:%d\r\n", icon_img[idx], pos);
	gdispImageOpenFile(&myImage, icon_img[idx]);
	//gdispImageCache(&myImage);
    gdispImageDraw(&myImage, (120 - myImage.width) >> 1, pos, \
					myImage.width, myImage.height, 0, 0);
	//gdisp_update();
	gdispImageClose(&myImage);
#endif	
}

font_t font_info;
coord_t	fwidth_info;
coord_t	fheight_info;
//font_t font_langting_mb;
void gui_disp_font_startup(void)
{
	font_info = gdispOpenFont("Roboto_Regular20"); 				// open a font to work with
	fwidth_info = gdispGetWidth();
	fheight_info = gdispGetFontMetric(font_info, fontHeight) + 10;

	font_roboto_20.font = font_info;
	font_roboto_20.width = fwidth_info;
	font_roboto_20.height = fheight_info;

//	font_langting_mb = gdispOpenFont("langting_mb24");
//	font_langting.font = font_langting_mb;
//	font_langting.width = fwidth_info;
//	font_langting.height = fheight_info;
//#if 0
//	font_langting_light.font = gdispOpenFont("langting_mb22");
//	font_langting_light.width = gdispGetWidth();
//	font_langting_light.height = gdispGetFontMetric(font_langting_light.font, fontHeight) + 10;
//#endif
    
	font_roboto_60.font = gdispOpenFont("Roboto_Bold72");
	font_roboto_60.width = gdispGetWidth();
	font_roboto_60.height = gdispGetFontMetric(font_roboto_60.font, fontHeight) + 10;    
    
	font_roboto_44.font = gdispOpenFont("Roboto_Bold44");
	font_roboto_44.width = gdispGetWidth();
	font_roboto_44.height = gdispGetFontMetric(font_roboto_44.font, fontHeight) + 10;

	font_roboto_32.font = gdispOpenFont("Roboto_Regular28");
	font_roboto_32.width = gdispGetWidth();
	font_roboto_32.height = gdispGetFontMetric(font_roboto_32.font, fontHeight) + 20;

	font_roboto_16.font = gdispOpenFont("Roboto_Regular22");
	font_roboto_16.width = gdispGetWidth();
	font_roboto_16.height = gdispGetFontMetric(font_roboto_16.font, fontHeight) + 6;

	font_roboto_12.font = gdispOpenFont("Roboto_Bold12");
	font_roboto_12.width = gdispGetWidth();
	font_roboto_12.height = gdispGetFontMetric(font_roboto_12.font, fontHeight) + 4;

    font_roboto_14.font = gdispOpenFont("Roboto_Bold16");
	font_roboto_14.width = gdispGetWidth();
	font_roboto_14.height = gdispGetFontMetric(font_roboto_14.font, fontHeight) + 4;

//	font_langting_lb16.font = gdispOpenFont("langting_week20");
//	//font_langting_lb16.font = gdispOpenFont("Roboto_Regular20");
//	font_langting_lb16.width = gdispGetWidth();
//	font_langting_lb16.height = gdispGetFontMetric(font_langting_lb16.font, fontHeight) + 6;
	
	return;
	
	gdispClear(Black);
#if (GDISP_NEED_TEXT)	
	const char *line[2];
#if (GDISP_INCLUDE_USER_FONTS)
	font_t font1 = gdispOpenFont("langting_mb24bw*"); 				// open a font to work with
	line[1] = "设置";
	line[0] = "运动";
#else
	font_t font1 = gdispOpenFont("DejaVuSans24*"); 				// open a font to work with
	line1 = "ryeex";
	line2 = "hell 1024";	
#endif	
	coord_t y = 0;
	coord_t	fwidth = gdispGetWidth();
	coord_t	fheight = gdispGetFontMetric(font1, fontHeight) + 10;
	const color_t color_tbl[] = {
		White,
		Red,		
		White,		
	};

	static int c = 0;
	while ( c < sizeof(color_tbl) / sizeof(color_tbl[0]) ){
		color_t color_font = color_tbl[c];
		for (int i = 0; i < 240/fheight; i++){
			y = fheight * i;		
			gdispFillStringBox(0, y, fwidth,  fheight, (char*)line[i % 2], (void *)font1, color_font, Black, justifyCenter);
		}
		gdisp_update();
		_gui_log_debug("[gui thread]: fonts verify...\n");
		gui_delay_ms(100);
        c++;
	}
	gui_delay_ms(100);		//white fonts display
#endif
}

void gui_disp_string(char *str, color_t color)
{
	gdispFillStringBox(0, 10, fwidth_info,  fheight_info, str, (void *)font_info, color, Black, justifyCenter);
}

void gui_disp_string_value(char *str, color_t color)
{
	gdispFillStringBox(0, fheight_info + 10, fwidth_info,  fheight_info, str, (void *)font_info, color, Black, justifyCenter);
}

#if 0
// A graph styling
static GGraphStyle GraphStyle1 = {
    { GGRAPH_POINT_DOT,  0, Blue       },      // Point
    { GGRAPH_LINE_NONE,  2, Gray       },      // Line
    { GGRAPH_LINE_SOLID, 0, White      },      // X axis
    { GGRAPH_LINE_SOLID, 0, Green      },      // Y axis
    { GGRAPH_LINE_DASH,  5, Gray,   50 },      // X grid
    { GGRAPH_LINE_DOT,   7, Yellow, 50 },      // Y grid
    GWIN_GRAPH_STYLE_POSITIVE_AXIS_ARROWS      // Flags
};
 
void gui_disp_graph_startup(void) {
    GHandle     gh;
    uint16_t    i;
 	static GGraphObject g;

    // Create the graph window
    {
        GWindowInit wi;
        wi.show = TRUE;
        wi.x = wi.y = 0;
        wi.width =  gdispGetWidth();
        wi.height = gdispGetHeight();
        gh = gwinGraphCreate(&g, &wi);
    }
 
    // Set the graph origin and style
    gwinGraphSetOrigin(gh, gwinGetWidth(gh)/2, gwinGetHeight(gh)/2);
    gwinGraphSetStyle(gh, &GraphStyle1);
    gwinGraphDrawAxis(gh);
	gdisp_update();	
}
#endif


static inline void _gui_entry_setup(void)
{
#if (!WIN32_SIM_MK)		
	_gui_thread_entry();
#endif
}

void disp_icon_img(uint8_t idx, char *info, color_t color)
{
#if IMG_SHOW_ALL		
	gdispImage myImage;
	if (idx >= (sizeof(icon_img) >> 2)){
		idx = 0;
	}
	gdispImageOpenFile(&myImage, icon_img[idx]);
	gdispImageDraw(&myImage, (120 - myImage.width) >> 1, (240 - myImage.height), myImage.width, myImage.height, 0, 0);
	gdispImageClose(&myImage);
#else
	if (idx ==0){
		gdispImage myImage;
		gdispImageOpenFile(&myImage, icon_img[0]);
		gdispImageDraw(&myImage, (120 - myImage.width) >> 1, (240 - myImage.height), myImage.width, myImage.height, 0, 0);
		gdispImageClose(&myImage);
	}
	else{
		// gui_widget_demo();
		gui_disp_string(info, color);
	}
#endif	
}

void icon_img_loop(char *info, color_t color)
{
#if IMG_SHOW_ALL	
	static uint8_t loop_cnt;
	static uint8_t idx = 2;
	if (loop_cnt ++ >= 30){
		//if IMG_SHOW_ALL is disable, max idx is 1
		if (++idx >= (sizeof(icon_img) >> 2)){
			idx = 0;
		}
		gdispImage myImage;
		gdispImageOpenFile(&myImage, icon_img[idx]);
		gdispImageDraw(&myImage, 0, 0, myImage.width, myImage.height, 0, 0);
		gdispImageClose(&myImage);
		loop_cnt = 0;
	}
#else
	// gui_widget_demo();
	gui_disp_string(info, color);
#endif	
}

#if 0
//=========================================================================================================

// Struct for passing more than one parameter to custom rendering function
typedef struct myContainerRenderingParams {
	gdispImage* image1;
	gdispImage* image2;
} myContainerRenderingParams;

// Local/global variables
GHandle ghContainer;
myContainerRenderingParams containerRenderingParams;
gdispImage imageBackground1;
gdispImage imageBackground2;

// The custom rendering function for the container
void _container_cwindow_rendering(GWidgetObject* gw, void* param)
{
	const myContainerRenderingParams* myParams = (const myContainerRenderingParams*)param;
	if (!myParams) {
		return;
	}
	
	gdispGFillArea(gw->g.display, gw->g.x, gw->g.y, gw->g.width, gw->g.height, gw->pstyle->background);
	
	if (myParams->image1) {
		gdispGImageDraw(gw->g.display,
		 				myParams->image1,
		 				(gw->g.width - myParams->image1->width) >> 1,
		 				0,
						myParams->image1->width,
						myParams->image1->height,
		 				0,
		 				0);
	}
	
	if (myParams->image2) {
		gdispGImageDraw(gw->g.display,
		 				myParams->image2,
		 				(gw->g.width - myParams->image2->width) >> 1,
						gw->g.height >> 1,
		 				myParams->image2->width,
						myParams->image2->height,
		 				0,
		 				0);
	}
	gdisp_update();
}

void gui_create_cwindow(void)
{
	GWidgetInit	wi;
	gwinWidgetClearInit(&wi);
	gdispImageOpenFile(&imageBackground1, "menu_01_card.png");
	containerRenderingParams.image1 = &imageBackground1;
	gdispImageOpenFile(&imageBackground2, "menu_02_hrm.png");	
	containerRenderingParams.image2 = &imageBackground2;

	// Create the container
	wi.g.show = TRUE;
	wi.g.width = 120;
	wi.g.height = 240;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.customParam = &containerRenderingParams;
	wi.customDraw = _container_cwindow_rendering;
	wi.text = "Container";
	ghContainer = gwinContainerCreate(0, &wi, 0);
	// gdisp_update();
}
#endif
