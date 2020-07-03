/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __MIBEACON_H__
#define __MIBEACON_H__

/*********************************************************************
 * INCLUDES
 */

#include "ry_type.h"


/*
 * CONSTANTS
 *******************************************************************************
 */


#define UUID_MI_SERVICE                               0xFE95

/**
 * MI Beacon items.
 */
#define MIBEACON_ITEM_FACTORY_NEW                     0x0001
#define MIBEACON_ITEM_CONNECTED                       0x0002
#define MIBEACON_ITEM_CENTRAL                         0x0004
#define MIBEACON_ITEM_SEC_ENABLE                      0x0008
#define MIBEACON_ITEM_MAC_INCLUDE                     0x0010
#define MIBEACON_ITEM_CAP_INCLUDE                     0x0020
#define MIBEACON_ITEM_OBJ_INCLUDE                     0x0040
#define MIBEACON_ITEM_MESH_INCLUDE                    0x0080
#define MIBEACON_ITEM_MANUFACTORY_DATA_INCLUDE        0x0100
#define MIBEACON_ITEM_BINDING_CFM                     0x0200
#define MIBEACON_ITEM_SEC_AUTH                        0x0400
#define MIBEACON_ITEM_SEC_LOGIN                       0x0800

#define MIBEACON_ITEM_VERSION                         0x4000
#define MIBEACON_ITEM_PRODUCT_ID                      0x8000



#define KEY_CODE_LIGHT_ON                             0x0001
#define KEY_CODE_LIGHT_OFF                            0x0002

#define KEY_TYPE_SINGLE_CLICK                         0x00
#define KEY_TYPE_DOUBLE_CLICK                         0x01
#define KEY_TYPE_LONG_PRESS                           0x02



/*
 * TYPES
 *******************************************************************************
 */




/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to reset MCU
 *
 * @param   None
 *
 * @return  None
 */
void mibeacon_init(void);


void mibeacon_click(u8_t* data, int len);
void mibeacon_sleep(u8_t type);
void mibeacon_on_save_frame_counter(void);







#endif  /* __MIBEACON_H__ */


