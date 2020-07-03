/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-09-22     Bernard      add board.h to this bsp
 */

// <<< Use Configuration Wizard in Context Menu >>>
#ifndef __BOARD_H__
#define __BOARD_H__



void ry_board_init(void);
void ry_board_printf(const char* fmt, ...);
void ry_board_debug_printf(const char* fmt, ...);
void ry_log_init(void);
void ry_board_debug_printf(const char* fmt, ...);


void ry_system_reset(void);
void ry_board_default_pins(void);
void enable_print_interface(void);

/**
 * @brief   API to get_reset_stat
 *
 * @param   None
 *
 * @return  STAT Register Bits
 */
int ry_get_reset_stat(void);

void ry_assert_log(void);


#endif 

//*** <<< end of configuration section >>>    ***