/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_TOOL_H__
#define __RY_TOOL_H__

#include "ry_type.h"


/*********************************************************************
* TYPEDEFS
*/
typedef struct {
    uint8_t     io_mode;
    uint32_t    gpio_intterup;
    u32_t       no_uart_start;
    u32_t       no_uart_cnt;    
} ry_tool_ctx_t;

/*********************************************************************
 * VARIABLES
 */
extern volatile ry_tool_ctx_t    tool_ctx_v;

/**
 * @brief   API to init TOOL module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_tool_init(void);

/**
 * @brief   API to set uart rx to gpio input mode
 *
 * @param   None
 *
 * @return  None
 */
void uart_gpio_mode_init(void);


/**
 * @brief   API to init ry_tool_thread_init
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_tool_thread_init(void);

#endif  /* __RY_TOOL_H__ */


