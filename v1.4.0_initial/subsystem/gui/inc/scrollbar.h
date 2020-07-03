/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __SCROLLBAR_H__
#define __SCROLLBAR_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */




/*
 * TYPES
 *******************************************************************************
 */

typedef struct {
    int color;
    int xLen;
    int yLen;
    int displayTimer;
    int displayInterval;
    int lastMinX;
    int lastMaxX;
    int lastMinY;
    int lastMaxY;
    u8_t* lastFrameBuf;
} scrollbar_t;



/*
 * FUNCTIONS
 *******************************************************************************
 */

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
scrollbar_t* scrollbar_create(int width, int length, int color, int displayInterval);


/**
 * @brief   API to destrory the scrollbar.
 *
 * @param   p - The pointer to the existed scrollbar
 *
 * @return  None
 */
void scrollbar_destroy(scrollbar_t* p);

/**
 * @brief   API to draw a scrollbar and display
 *
 * @param   sb - The applied scrollbar widget
 * @param   framebuf - Which buffer to draw
 * @param   startY - The start point of Y axis
 *
 * @return  None
 */
void scrollbar_show(scrollbar_t* sb, u8_t* framebuf, int startY);


void scrollbar_show_ex(scrollbar_t* sb, u8_t* framebuf, int startY);


#endif  /* __SCROLLBAR_H__ */
