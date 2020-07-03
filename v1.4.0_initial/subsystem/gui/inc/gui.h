/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __GUI_H__
#define __GUI_H__


/*********************************************************************
 * INCLUDES
 */
#include "gfx.h"
#include "ry_type.h"

/*
 * CONSTANTS
 *******************************************************************************
 */

#define cs_printf(_fmt_, ...)	do{\
									gwinPrintf(GW1, _fmt_, ##__VA_ARGS__);\
									gdisp_update();\
								}while(0)

#define GUI_STATE_IDLE          0
#define GUI_STATE_ON            1
#define GUI_STATE_LOW_POWER     2
#define GUI_STATE_OFF           3
#define GUI_STATE_READY_ON      4

/*
 * TYPES
 *******************************************************************************
 */
typedef struct {
    u32_t         curState;
    u32_t         screen_on_start; 
    u32_t         screen_on_time;           
} ry_gui_ctx_t;

typedef struct
{
    uint8_t cmd;
	uint32_t hold_time;
    uint32_t para[4];
}gui_test_t;

typedef enum {
    GUI_TS_NONE      	=   0x00,
    GUI_TS_FILL_COLOR   =   0x01,
    GUI_TS_DISP_IMG     =   0x02,
} gui_test_cmd_t;

typedef enum {
    GUI_EVT_TP      	=   0x00,
    GUI_EVT_RTC         =   0x01,
    GUI_EVT_TEST        =   0x02,
    
} gui_evt_type_t;

typedef struct {
    uint8_t evt_type;
    void *pdata;
} gui_evt_msg_t;




typedef struct {
    u32_t msgType;
    u32_t len;
    u8_t  data[6];
} ry_gui_msg_t;


typedef struct {
    font_t  font;
    coord_t width;
    coord_t height;
} gui_font_t;


/*
 * GLOBAL VARIABLES
 *******************************************************************************
 */
extern gui_test_t	gui_test_sc;
extern uint32_t     gui_event_time;
//extern gui_font_t   font_langting;
extern gui_font_t   font_roboto_20;
extern gui_font_t   font_roboto_60;
extern gui_font_t   font_roboto_44;
extern gui_font_t   font_roboto_32;
extern gui_font_t   font_roboto_16;
extern gui_font_t   font_roboto_14;
extern gui_font_t   font_roboto_12;
// extern gui_font_t   font_langting_light;
//extern gui_font_t   font_langting_lb16;

extern uint32_t gui_init_done;

/*
 * FUNCTIONS
 *******************************************************************************
 */

/**
 * @brief   API to get get_gui_state
 *
 * @param   None
 *
 * @return  get_gui_state result
 */
uint32_t get_gui_state(void);

/**
 * @brief   gui thread task
 * @param   None
 *
 * @return  None
 */
void gui_thread_entry(void);

/**
 * @brief   API to init gsensor module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t gui_init(void);

void ry_gui_msg_send(u32_t type, u32_t len, u8_t *data);

/**
 * @brief   API to get get_gui_on_time
 *
 * @param   None
 *
 * @return  get_gui_on_time result
 */
uint32_t get_gui_on_time(void);

/**
 * @brief   API to clear get_gui_on_time to zero
 *
 * @param   None
 *
 * @return  None
 */
void clear_gui_on_time(void);

/**
 * @brief   API to display_startup_logo
 *
 * @param   en - 0: do not display, else: display
 *
 * @return  None
 */
void gui_display_startup_logo(int en);

/**
 * @brief   API to set oled_hw_off
 *
 * @param   None
 *
 * @return  None
 */
void oled_hw_off(void);

#endif //__GUI_H__
