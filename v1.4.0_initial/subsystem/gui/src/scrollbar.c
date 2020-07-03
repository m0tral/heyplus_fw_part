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

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos_timer.h"
#include "scrollbar.h"
#include "ryos.h"
#include "gui_bare.h"
#include "scheduler.h"
#include "gui.h"


/*********************************************************************
 * CONSTANTS
 */ 



/*********************************************************************
 * TYPEDEFS
 */



 
/*********************************************************************
 * LOCAL VARIABLES
 */

int * disp_buf = NULL;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/**
 * @brief   API to set point color
 *
 * @param   None
 *
 * @return  None
 */
void scrollbar_setDip(u8_t* framebuf, int x, int y, int color)
{
    if((0 <= x)&&(x < RM_OLED_WIDTH)){
        uint32_t pos = 120 * y * 3 + x * 3;
        framebuf[pos]     = (uint8_t) color & 0xff;
        framebuf[pos + 1] = (uint8_t) (color>>8) & 0xff;
        framebuf[pos + 2] = (uint8_t) (color>>16) & 0xff;
    }
}

void  scrollbar_getDip(u8_t* framebuf, int x, int y, int * color)
{
    if((0 <= x)&&(x < RM_OLED_WIDTH)){
        uint32_t pos = 120 * y * 3 + x * 3;
        *color = framebuf[pos] | (framebuf[pos + 1] << 8) | (framebuf[pos + 2] << 16);
    }
}


/**
 * @brief   API to draw a rect
 *
 * @param   None
 *
 * @return  None
 */
void scrollbar_setRect(u8_t* framebuf, int max_x, int max_y, int min_x, int min_y, int color, int flag )
{   
    int x,y;
    int offset = 0;
    for(x = min_x; x <= max_x; x++){
        for(y = min_y; y <= max_y; y++){
            if(flag == 1){
                offset = (max_x - min_x + 1 ) * (y - min_y) + (x - min_x);
                if(disp_buf != NULL){
                    scrollbar_getDip(framebuf, x, y, &disp_buf[offset]);
                }
            }
            scrollbar_setDip(framebuf, x, y, color);
        }
    }
}

/**
 * @brief   API to clear a rect
 *
 * @param   None
 *
 * @return  None
 */
void scrollbar_clearRect(u8_t* framebuf, int max_x, int max_y, int min_x, int min_y)
{
    //scrollbar_setRect(framebuf, max_x, max_y, min_x, min_y, Black, 1);
    int x,y;
    int offset = 0;
    for(x = min_x; x <= max_x; x++){
        for(y = min_y; y <= max_y; y++){
            offset = (max_x - min_x + 1 ) * (y - min_y - 1) + (x - min_x);
            if(disp_buf != NULL){
                scrollbar_setDip(framebuf, x, y, disp_buf[offset]);
            }else{
                scrollbar_setDip(framebuf, x, y, Black);
            }
        }
    }
}


void scrollbar_timeout_handler(void* arg)
{
    scrollbar_t* sb = (scrollbar_t*)arg;
    RY_ASSERT(sb);

    /* Scrollbar */
    LOG_DEBUG("[scrollbar_timeout_handler]. 0x%x\r\n", sb);
    scrollbar_clearRect(sb->lastFrameBuf, sb->lastMaxX, sb->lastMaxY, 
        sb->lastMinX, sb->lastMinY);

    /* Update UI */
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}    


void scrollbar_timer_start(scrollbar_t *sb)
{
    ry_timer_start(sb->displayTimer, sb->displayInterval, scrollbar_timeout_handler, (void*)sb);
}

void scrollbar_timer_stop(scrollbar_t *sb)
{
    ry_sts_t status;
    status = ry_timer_stop(sb->displayTimer);
    if (status != RY_SUCC) {
        //LOG_DEBUG("[%s] timer stop fail, status = %x\r\n", __FUNCTION__, status);
    }
}


/**
 * @brief   API to draw a scrollbar and display
 *
 * @param   sb - The applied scrollbar widget
 * @param   framebuf - Which buffer to draw
 * @param   startY - The start point of Y axis
 *
 * @return  None
 */
void scrollbar_show(scrollbar_t* sb, u8_t* framebuf, int startY)
{
    int max_x, max_y, min_x, min_y;
    
    /* Calculate position */
    min_x = 119 - sb->xLen;
    max_x = 119;
    min_y = startY;//position * 8;
    max_y = startY + sb->yLen;

    if (sb->displayTimer && sb->displayInterval > 0) {
        scrollbar_timer_stop(sb);
    }

    /* Display position */
    scrollbar_setRect(framebuf, max_x, max_y, min_x, min_y, sb->color, 1);

    /* Save the position for next time clear */
    sb->lastMinX = min_x;
    sb->lastMinY = min_y;
    sb->lastMaxX = max_x;
    sb->lastMaxY = max_y;
    sb->lastFrameBuf = framebuf;

    /* Start the display timer if necessary */
    if (sb->displayTimer && sb->displayInterval > 0) {
        //scrollbar_timer_start(sb);
    }
}

void scrollbar_show_ex(scrollbar_t* sb, u8_t* framebuf, int startY)
{
    int max_x, max_y, min_x, min_y;
    
    /* Calculate position */
    min_x = 119 - sb->xLen;
    max_x = 119;
    min_y = startY;//position * 8;
    max_y = startY + sb->yLen;

    if (sb->displayTimer && sb->displayInterval > 0) {
        scrollbar_timer_stop(sb);
    }

    // clear scrollbar area
    scrollbar_setRect(framebuf, max_x, RM_OLED_HEIGHT-1, min_x, 0, Black, 0);
    /* Display position */
    scrollbar_setRect(framebuf, max_x, max_y, min_x, min_y, sb->color, 1);

    /* Save the position for next time clear */
    sb->lastMinX = min_x;
    sb->lastMinY = min_y;
    sb->lastMaxX = max_x;
    sb->lastMaxY = max_y;
    sb->lastFrameBuf = framebuf;

    /* Start the display timer if necessary */
    if (sb->displayTimer && sb->displayInterval > 0) {
        //scrollbar_timer_start(sb);
    }
}


/**
 * @brief   API to apply a scrollbar widget
 *
 * @param   width  - the width of the scroll bar.
 * @param   length - the length of the scroll bar.
 * @param   color  - the color of the scroll bar.
 * @param   displayInterval  - how long to display the widget, in ms
 *
 * @return  None
 */
scrollbar_t* scrollbar_create(int width, int length, int color, int displayInterval)
{
    ry_timer_parm timer_para;
    scrollbar_t* scrollbar = (scrollbar_t*)ry_malloc(sizeof(scrollbar_t));
    if (NULL == scrollbar) {
        LOG_ERROR("[scrollbar_create] No mem.\r\n");
        return NULL;
    }

    ry_memset((void*)scrollbar, 0, sizeof(scrollbar_t));

    scrollbar->color = color;
    scrollbar->xLen  = width;
    scrollbar->yLen  = length;
    scrollbar->displayInterval = displayInterval;

    /* Create the timer */
    timer_para.timeout_CB = scrollbar_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "scrollbar_timer";
    scrollbar->displayTimer = ry_timer_create(&timer_para);
    int buf_size = ((width+1) * (length+1) * sizeof(int));
    if((disp_buf != NULL)){
        ry_free(disp_buf);
        disp_buf = NULL;
    }
    disp_buf = (int *)ry_malloc(buf_size);
    if(disp_buf == NULL){
        LOG_ERROR("[scrollbar_create] No mem.\r\n");
        return NULL;
    }else{
    }
		
    return scrollbar;
}

/**
 * @brief   API to destrory the scrollbar.
 *
 * @param   p - The pointer to the existed scrollbar
 *
 * @return  None
 */
void scrollbar_destroy(scrollbar_t* p)
{
    if (p) {
        ry_timer_stop(p->displayTimer);
        ry_timer_delete(p->displayTimer);
        ry_free((void*)p);
        //FREE_PTR(disp_buf);
    }
}
