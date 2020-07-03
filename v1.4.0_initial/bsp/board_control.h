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
#ifndef __BOARD_CONTROL_H__
#define __BOARD_CONTROL_H__

/**
 * @brief   power_main_sys power control
 *
 * @param   ctrl: 0 close power_main_sys, else: open
 *
 * @return  None
 */
void power_main_sys_on(uint8_t ctrl);


/**
 * @brief   main hrm power control
 *
 * @param   ctrl: 0 close main hrm power, else: open
 *
 * @return  None
 */
void power_hrm_on(uint8_t ctrl);

/**
 * @brief   oled power control
 *
 * @param   ctrl: 0 close oled power, else: open
 *
 * @return  None
 */
void power_oled_on(uint8_t ctrl);

/**
 * @brief   board power control init
 *
 * @param   None
 *
 * @return  None
 */
void board_control_init(void);

/**
 * @brief   API to get charge_cable_is_input
 *
 * @param   None
 *
 * @return  status, true: charging, false: not charging
 */
bool charge_cable_is_input(void);

void auto_reset_start(void);
void power_off_ctrl(uint8_t enable);
void dc_in_hw_disable(void);


#endif
//*** <<< end of configuration section >>>    ***
