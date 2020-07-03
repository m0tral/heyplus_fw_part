/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __GUI_CONSTRACTION_H__
#define __GUI_CONSTRACTION_H__

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
    uint8_t     menu_idx;
	uint8_t     *image[2];
    uint8_t     image_pos[2];
    uint8_t     *info[2];
    uint8_t     info_pos[2];
} cwin_params_t;


extern cwin_params_t cwin_menu_array[];


/*
 * FUNCTIONS
 *******************************************************************************
 */

void _cwin_update(cwin_params_t *cwin_menu);


#endif  /* __GUI_CONSTRACTION_H__ */



