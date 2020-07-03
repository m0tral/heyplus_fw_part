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
#include "app_config.h"
#include "ryos.h"
#include "scheduler.h"
//#include "wm.h"


/* ui specified */
#include "activity_menu_layout.h"
#include "activity_menu.h"
#include "touch.h"
#include "gui_bare.h"
#include "ry_hal_inc.h"
#include "ryos_timer.h"

#include "am_hal_ctimer.h"
#include "am_mcu_apollo.h"
#include "board.h"
#include "board_control.h"
#include "gui.h"
#include "app_config.h"
#include "app_interface.h"

/* resources */
#include "gui_img.h"
#include "gui_constraction.h"
//#include "demo_menu_01.h"
//#include "demo_menu_02.h"
//#include "demo_menu_03.h"
//#include "demo_menu_04.h"
//#include "demo_menu_05.h"

#include "activity_setting.h"
#include "activity_hrm.h"
#include "activity_card.h"
#include "activity_sports.h"
#include "activity_data.h"
#include "activity_weather.h"
#include "activity_sfc_option.h"
#include "activity_mijia.h"
#include "activity_alarm.h"
#include "activity_timer.h"
#include "activity_alipay.h"
#include "gui_msg.h"
#include "scrollbar.h"
#include "motar.h"

#include "device_management_service.h"
#include "app_management_service.h"
#include "card_management_service.h"
#include "timer_management_service.h"
#include "window_management_service.h"
#include "ry_statistics.h"


/*********************************************************************
 * CONSTANTS
 */ 
#define MENU_IMG_NUM  5
#define MENU_TOP      ((u8_t*)EXFLASH_ADDR_IMG_RESOURCE)
#define MENU_LAST     ((u8_t*)(EXFLASH_ADDR_IMG_RESOURCE + /*86400*(MENU_IMG_NUM-1)*/ 43200*(CUR_APP_NUM-2)))
#define MENU_BOTTOM   ((u8_t*)(EXFLASH_ADDR_IMG_RESOURCE + 86400*MENU_IMG_NUM))


#define MOVE_DIR_NONE     0
#define MOVE_DIR_UP       1
#define MOVE_DIR_DN       2


/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    int index;
    scrollbar_t* scrollbar;
} menu_ctx_t;
 
/*********************************************************************
 * LOCAL VARIABLES
 */

extern u8_t* p_which_fb;
extern u8_t* p_draw_fb;
extern u8_t frame_buffer[];
extern u8_t frame_buffer_1[];

ryos_thread_t *menu_thread_handle;


activity_t activity_menu = {
    .name      = "menu",
    .onCreate  = menu_onCreate,
    .onDestroy = menu_onDestrory,
    .priority = WM_PRIORITY_L,
};

menu_ctx_t* menu_ctx_v = NULL;

//scroll_t scroller;
//velocity_t* velocity;




/*********************************************************************
 * LOCAL FUNCTIONS
 */
void menu_select(u8_t touchY);
u8_t* p_menu_fb;


#if 0
void menu_draw(void)
{
    int w, h;
    draw_img_to_framebuffer(RES_EXFLASH, "xxx.bmp", 
        0, 0, &w, &h, d_justify_center);

    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)hrm_img_animate[hrm_ctx_v.animate_idx], 
        0, 120, &w, &h, d_justify_center);
}
#endif

void menu_last_display(void)
{
    int startY;
    //ry_memcpy(frame_buffer, p_menu_fb, 86400);
    ry_hal_spi_flash_read(frame_buffer, (u32_t)p_menu_fb, 86400);
    startY = ((p_menu_fb - MENU_TOP) / 43200) * (240 - MENU_SCROLLBAR_LENGTH)/(CUR_APP_NUM-2);
    scrollbar_show(menu_ctx_v->scrollbar, frame_buffer, startY);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

#define MENU_MOVE_CACHED    FALSE

#if !MENU_MOVE_CACHED

#define ENABLE_ACCELERATION FALSE

void menu_move_up_para(int offset, int speed)
{
    int temp_offset  = offset/speed;
    int current = 0;
    int middle = offset >> 1;
    int accel = 1;
    int startY;
    int delta = 0;
    int diff = 0;
    bool done = false;
    
    while (speed) {
        delta = temp_offset * accel;
        current += delta;
        p_menu_fb += delta;
        
        if (current > offset) {
            diff = current - offset;
            delta -= diff;
            current -= diff;
            p_menu_fb -= diff;
        }
        
        if (p_menu_fb >= MENU_LAST) {
            diff = p_menu_fb - MENU_LAST;
            if (diff > 0) {
                delta -= diff;
                current -= diff;
            }
            p_menu_fb = MENU_LAST;//MENU_TOP;            
            done = true;
        }

        ry_memmove(frame_buffer, frame_buffer + delta, 86400 - delta);        
        uint8_t* frame_buffer_end = frame_buffer;
        frame_buffer_end += (86400 - delta);
        ry_hal_spi_flash_read(frame_buffer_end, (u32_t)p_menu_fb + (86400 - delta), delta);
        
        startY = ((p_menu_fb - MENU_TOP) / 43200) * (240 - MENU_SCROLLBAR_LENGTH)/(CUR_APP_NUM-2);
        scrollbar_show_ex(menu_ctx_v->scrollbar, frame_buffer, startY);

        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        
        if (done) break;        

        if (ENABLE_ACCELERATION) {
            speed -= accel;
            if (current < middle) {
                accel++;
            }
            else {
                accel--;
                if (accel < 1) accel = 1;
            }
        }   
        
        if (current >= offset) break;            
    }
}
// speed: less => faster
void menu_move_down_para(int offset, int speed)
{
    int temp_offset = offset/speed;
    int current = 0;    
    int startY;
    int middle = offset >> 1;
    int accel = 1;
    int delta = 0;
    int diff = 0;    
    bool done = false;
        
    while (speed) {
        delta = temp_offset * accel;
        current += delta;        
        p_menu_fb -= delta;
        
        if (current > offset) {
            diff = current - offset;
            delta -= diff;
            current -= diff;
            p_menu_fb += diff;
        }        
        
        if (p_menu_fb <= MENU_TOP) {
            
            diff = MENU_TOP - p_menu_fb;
            if (diff > 0) {
                delta -= diff;
                current -= diff;
            }
            
            p_menu_fb = MENU_TOP;//MENU_LAST;
            done = true;
        }

        ry_memmove(frame_buffer + delta, frame_buffer, 86400 - delta);        
        ry_hal_spi_flash_read(frame_buffer, (u32_t)p_menu_fb, delta);
                
        //ry_hal_spi_flash_read(frame_buffer, (u32_t)p_menu_fb, 86400);

        startY = ((p_menu_fb - MENU_TOP) / 43200) * (240 - MENU_SCROLLBAR_LENGTH)/(CUR_APP_NUM-2);
        scrollbar_show_ex(menu_ctx_v->scrollbar, frame_buffer, startY);
        
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        
        if (done) break;
        
        if (ENABLE_ACCELERATION)
        { 
            speed -= accel;
            if (current < middle) {
                accel++;
            }
            else {
                accel--;
                if (accel < 1) accel = 1;
            }
        }
        
        if (current >= offset) break;         
    }
}

#else

void menu_move_up_para(int offset, int speed)
{
    int index       = 0;
    int temp_offset = offset/speed;
    int startY      = 0;
    
    int menu_offset = (u32_t)p_menu_fb + offset;
    int buffer_offset = 0;
    ry_hal_spi_flash_read(frame_buffer_1, (u32_t)menu_offset, 43200);
    
    while (speed) {
        p_menu_fb += temp_offset;
        buffer_offset += temp_offset;
        if (p_menu_fb > MENU_LAST) {
            p_menu_fb = MENU_LAST;//MENU_TOP;
            //speed++; // make up once            
        }

        ry_memmove(frame_buffer, frame_buffer + temp_offset, 86400 - temp_offset);
        ry_memcpy(frame_buffer + (86400 - temp_offset), frame_buffer_1, buffer_offset);
        
        //u32_t tick = ry_hal_clock_time();

        startY = ((p_menu_fb - MENU_TOP) / 43200) * (240 - MENU_SCROLLBAR_LENGTH)/(CUR_APP_NUM-2);
        scrollbar_show(menu_ctx_v->scrollbar, frame_buffer, startY);

        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

        speed--;
    }
}

void menu_move_down_para(int offset, int speed)
{
    int index   = 0;
    //int speed   = 4;
    int temp_offset  = offset/speed;
    int startY;

    int menu_offset = (u32_t)p_menu_fb - offset;
    int buffer_offset = 43200;
    ry_hal_spi_flash_read(frame_buffer_1, (u32_t)menu_offset, 43200);
    
    while (speed) {
        p_menu_fb -= temp_offset;
        buffer_offset -= temp_offset;
        if (p_menu_fb < MENU_TOP) {
            p_menu_fb = MENU_TOP;//MENU_LAST;
            //speed++; // make up once            
        }

        ry_memmove(frame_buffer + temp_offset, frame_buffer, 86400 - temp_offset);
        ry_memcpy(frame_buffer, frame_buffer_1 + buffer_offset, 43200 - buffer_offset);

        startY = ((p_menu_fb - MENU_TOP) / 43200) * (240 - MENU_SCROLLBAR_LENGTH)/(CUR_APP_NUM-2);
        scrollbar_show(menu_ctx_v->scrollbar, frame_buffer, startY);
        
        ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
        
        speed--;
    }
}

#endif


void menu_move_up(void)
{
    int index   = 0;
    int speed   = 8;
    int offset  = 86400/speed;
    int moveCnt = ((MENU_IMG_NUM-1)*speed);

    //p_draw_fb = (u8_t*)MENU_TOP;
    p_draw_fb = frame_buffer;
    ry_hal_spi_flash_read(p_draw_fb, (u32_t)MENU_TOP, RM_OLED_IMG_24BIT);

    while(1) {
        //LOG_DEBUG("img: %d, 0x%x\r\n", index, (u32_t)p_draw_fb);
        update_buffer_to_oled();
        p_draw_fb += offset;

        if (index++ == moveCnt) {
            index = 0;
        	p_draw_fb = (u8_t*)MENU_TOP;
        }

        //ryos_delay_ms(1000);
    }
}



void menu_move_down(void)
{
    int index   = 0;
    int speed   = 8;
    int offset  = 86400/speed;
    int moveCnt = ((MENU_IMG_NUM-1)*speed);

    p_draw_fb = (u8_t*)MENU_LAST;

    while(1) {
        //LOG_DEBUG("img: %d, 0x%x\r\n", index, (u32_t)p_draw_fb);
        update_buffer_to_oled();
        p_draw_fb -= offset;

        if (index++ == moveCnt) {
            index = 0;
        	p_draw_fb = (u8_t*)MENU_LAST;
        }
       
        //ryos_delay_ms(1000);
    }
}


//#define UI_EFFECT_EN
#ifdef UI_EFFECT_EN

//*****************************************************************************
//
// Interrupt handler for the CTIMER
//
//*****************************************************************************

#define EFFECT_1S         32768
#define EFFECT_500MS      16384
#define EFFECT_300MS      9608
#define EFFECT_100MS      327

int effect_speed;
int effect_offset;
int effect_moveCnt;
int effect_index;

typedef struct {
    int duration;
    int speed;
} effect_tbl_t;


effect_tbl_t effect_tbl[4] = {
    {EFFECT_1S,    4},
    {EFFECT_500MS, 8},
    {EFFECT_300MS, 10},
    {EFFECT_100MS, 12},
};


void effect_timer_start(void)
{
    am_hal_ctimer_start(1, AM_HAL_CTIMER_TIMERB);
}

void effect_timer_stop(void)
{
    am_hal_ctimer_stop(1, AM_HAL_CTIMER_TIMERB);
}

void effect_timeout_cb(void)
{
    ry_board_debug_printf("Effect timeout.%d\r\n", effect_index);
    am_hal_ctimer_clear(1, AM_HAL_CTIMER_TIMERB);

    effect_speed = effect_tbl[effect_index].speed;
    am_hal_ctimer_period_set(1, AM_HAL_CTIMER_TIMERB, effect_tbl[effect_index].duration, 0);   
    effect_timer_start();

    effect_index++;
    if (effect_index == 4) {
        effect_index = 0;
    }
}

void am_ctimer_isr(void)
{
	uint32_t ui32Status;
	
	//
	// Read the interrupt status.
	//
	ui32Status = am_hal_ctimer_int_status_get(true);

    //
    // Clear ctimer Interrupt.
    //
    am_hal_ctimer_int_clear(ui32Status);

    effect_timeout_cb();	
}

void effect_timer_init(void)
{
    // Configure a timer to drive the LED.
    am_hal_ctimer_config_single(1, AM_HAL_CTIMER_TIMERB,                                
                                (AM_HAL_CTIMER_FN_ONCE |                                 
                                 AM_HAL_CTIMER_XT_32_768KHZ |                                                                
                                 AM_HAL_CTIMER_INT_ENABLE));    
    //    
    // Set up initial timer period.    
    //    
    am_hal_ctimer_period_set(1, AM_HAL_CTIMER_TIMERB, 32768, 0);    

    //    
    // Enable interrupts for the Timer we are using on this board.    
    //    
    am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERB1C0);    
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_CTIMER);    
    am_hal_interrupt_master_enable();
}


void update_effect(void)
{
    effect_offset = 86400 / effect_speed;
    effect_moveCnt = ((MENU_IMG_NUM-1)*effect_speed);
}

void menu_move_down_with_effect(int speed)
{
    int index   = 0;
    //int speed   = 8;
    
    //int offset  = 86400/speed;
    effect_speed = speed;
    effect_offset = 86400 / effect_speed;
    effect_moveCnt = ((MENU_IMG_NUM-1)*effect_speed);
    effect_index = 0;
    effect_timer_start();

    p_draw_fb = (u8_t*)MENU_TOP;

    while(1) {
        //LOG_DEBUG("img: %d, 0x%x\r\n", index, (u32_t)p_draw_fb);
        update_buffer_to_oled();

        update_effect();
        
        p_draw_fb += effect_offset;

        if (index++ == effect_moveCnt) {
            index = 0;
        	p_draw_fb = (u8_t*)MENU_TOP;
            
        }

        ryos_delay_ms(10);
    }
}



#endif


#if 1
int menu_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;

    static u32_t startT;
    static u32_t lastT;
    static u16_t startY;
    static int   moveCnt = 0;
    static int   move_direction = MOVE_DIR_NONE;
    static u16_t lastY;
    static u8_t  predict_once = 0;
    static int   delay_move = 0;
    static int   do_move = 0;
    u16_t point;
    u32_t deltaT;
    u16_t deltaY;
    tp_event_t *p = (tp_event_t*)msg->data;

    //gui_evt_msg_t gui_msg;
    //gui_evt_msg_t *p_msg = &gui_msg;
    ry_sts_t status;
    int done = 0;
    static int picDiff = 0;

    //auto_reset_start();
    

    switch (p->action) {
        
        case TP_ACTION_DOWN:
            //LOG_DEBUG("TP Action Down, y:%d, t:%d\r\n", p->y, p->t);
            startT = p->t;
            lastT = p->t;
            startY = p->y;
            moveCnt = 0;
            lastY = p->y;
            do_move = 0;
            //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

            if (get_gui_state() != GUI_STATE_ON) {
                wms_screenOnOff(1);
                wms_quick_untouch_timer_start();
            }

            if ((wms_get_screen_darker() == 1) && (get_gui_state() == GUI_STATE_ON)){
                wms_set_screen_darker(0);
            }

            if (p->y == 9) {
				if(get_home_vibrate_enable())
				{
					if((get_dnd_home_vibrate_enbale() == 1)&&(current_time_is_dnd_status() == 1));
					else{
						motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
					}
				}
            }
            
            break;

        case TP_ACTION_MOVE:
            //LOG_DEBUG("TP Action Move, y:%d, t:%d\r\n", p->y, p->t);
            if (p->y == 0) {
                return consumed;
            }

            deltaT = ry_hal_calc_ms(p->t-lastT);
            lastT = p->t;
            
            moveCnt++;

            //if (lastY<=4 && p->y>4) {
            if (lastY < p->y && (p->y-startY>=2)) {
                if (deltaT >= 100) {
                    // Slow move
                    picDiff = 86400>>1;
                    //LOG_DEBUG("SLOW\r\n");
                } else {
                    // Fast move
                    picDiff = 86400;
                    //delay_move = 1;
                    //LOG_DEBUG("FAST\r\n");
                }
                move_direction = MOVE_DIR_DN;
            //} else if (lastY >= 4 && p->y < 4) {
            } else if ((lastY > p->y) && (startY-p->y>=2)) {
                if (deltaT >= 100) {
                    // Slow move
                    picDiff = 86400>>1;//86400>>1;
                    //LOG_DEBUG("SLOW\r\n");
                } else {
                    // Fast move
                    picDiff = 86400;//86400<<1;
                    //delay_move = 1;
                    //LOG_DEBUG("FAST\r\n");
                }
                move_direction = MOVE_DIR_UP;
            }

            //LOG_DEBUG("TP Action Move, y:%d, t:%d, deltaT:%d ms, move_dir:%d, delay_move:%d, pic:%d, do_mv:%d\r\n", p->y, p->t, deltaT, move_direction, 
                    //delay_move, picDiff, do_move);
            
            if (move_direction != MOVE_DIR_NONE && delay_move == 0 && do_move == 0) {
                if (move_direction == MOVE_DIR_DN) {
                    menu_move_down_para(picDiff, 10);
                } else {
                    menu_move_up_para(picDiff, 10);
                }
                //LOG_DEBUG("[menu] Move1 dir:%d, %d\r\n", move_direction);
                move_direction = MOVE_DIR_NONE;
                delay_move = 0;
                do_move = 1;
            }

            lastY = p->y;

            wms_quick_untouch_timer_start();

            if ((wms_get_screen_darker() == 1) && (get_gui_state() == GUI_STATE_ON)){
                wms_set_screen_darker(0);
            }
            
            break;

        case TP_ACTION_UP:
            //LOG_DEBUG("TP Action Up, y:%d, t:%d\r\n", p->y, p->t);
            ryos_delay_ms(1);
            deltaT = ry_hal_calc_ms(p->t-startT);
            //LOG_DEBUG("TP End. Duration: %d ms move_dir:%d, delay_mv:%d\r\n", deltaT, move_direction, delay_move);

            
            if (move_direction != MOVE_DIR_NONE && delay_move == 1 && deltaT <= 200) {
                if (move_direction == MOVE_DIR_DN) {
                    menu_move_down_para(86400, 10);
                } else {
                    menu_move_up_para(86400, 10);
                }
                done = 1;
                do_move = 1;
                //LOG_DEBUG("[menu] Move2, %d %t\r\n", p->y, deltaT);
            }

            
            menu_ctx_v->index = (p_menu_fb - MENU_TOP) / 43200;
            //LOG_DEBUG("[menu] Cur p_draw_fb:%x, index:%d\r\n", (u32_t)p_draw_fb, menu_ctx_v->index);

            wms_quick_untouch_timer_start();

            if (do_move == 0 && (p->y - startY >=2)) {
                menu_move_down_para(86400, 10);
                done = 1;
                do_move = 1;
                //LOG_DEBUG("[menu] Move3 DN, %d %t\r\n", p->y, deltaT);
            } else if(do_move == 0 && (startY - p->y >=2)){
                menu_move_up_para(86400, 10);
                done = 1;
                do_move = 1;
                //LOG_DEBUG("[menu] Move3 UP, %d %t\r\n", p->y, deltaT);
            }

            if ((do_move == 0) && (delay_move == 0)) {
                //LOG_DEBUG("[menu] Click, %d %t\r\n", p->y, deltaT);
                if (p->y == 9) {
                    menu_select(p->y);
                } else if (deltaT > 80) {
                    menu_select(p->y);
                }
            }

            predict_once = 0;
            moveCnt = 0;
            move_direction = MOVE_DIR_NONE;
            delay_move = 0;
            picDiff = 0;
            break;

        default:
            break;
    }
    
    return consumed; 
}
#endif


#if 0
int menu_onTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;


    u16_t point;
    u32_t deltaT;
    u16_t deltaY;
    tp_event_t *p = (tp_event_t*)msg->data;

    //gui_evt_msg_t gui_msg;
    //gui_evt_msg_t *p_msg = &gui_msg;
    ry_sts_t status;
    int done = 0;
    static int picDiff = 0;
    

    switch (p->action) {
        
        case TP_ACTION_DOWN:
            LOG_DEBUG("[menu_onTouchEvent] TP Action Down, y:%d, t:%d\r\n", p->y, p->t);
            startT = p->t;
            lastT = p->t;
            startY = p->y;
            moveCnt = 0;
            lastY = p->y;
            //ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

            scroller._scrollY = p->y;
            velocity = velocity_obtain();
            break;

        case TP_ACTION_MOVE:
            if (p->y == 0) {
                return consumed;
            }

            deltaT = ry_hal_calc_ms(p->t-lastT);
            lastT = p->t;
            
            moveCnt++;


            if (lastY<=4 && p->y>4) {
                if (deltaT >= 60) {
                    // Slow move
                    picDiff = 86400>>1;
                } else {
                    // Fast move
                    picDiff = 86400<<1;
                    delay_move = 1;
                }
                move_direction = MOVE_DIR_DN;
            } else if (lastY >= 4 && p->y < 4) {
                if (deltaT >= 60) {
                    // Slow move
                    picDiff = 86400>>1;
                } else {
                    // Fast move
                    picDiff = 86400<<1;
                    delay_move = 1;
                }
                move_direction = MOVE_DIR_UP;
            }

            LOG_DEBUG("TP Action Move, y:%d, t:%d, deltaT:%d ms, move_dir:%d, delay_move:%d, pic:%d\r\n", p->y, p->t, deltaT, move_direction, delay_move, picDiff);
            
            if (move_direction != MOVE_DIR_NONE && delay_move == 0) {
                if (move_direction == MOVE_DIR_DN) {
                    menu_move_down_para(picDiff, 4);
                } else {
                    menu_move_up_para(picDiff, 4);
                }
                move_direction = MOVE_DIR_NONE;
                delay_move = 0;
            }

            lastY = p->y;
            
            break;

        case TP_ACTION_UP:
            LOG_DEBUG("TP Action Up, y:%d, t:%d\r\n", p->y, p->t);
            deltaT = ry_hal_calc_ms(p->t-startT);
            LOG_DEBUG("TP End. Duration: %d ms move_dir:%d, delay_mv:%d\r\n", deltaT, move_direction, delay_move);


            if (move_direction != MOVE_DIR_NONE && delay_move == 1 && deltaT <= 200) {
                if (move_direction == MOVE_DIR_DN) {
                    menu_move_down_para(86400<<1, 4);
                } else {
                    menu_move_up_para(86400<<1, 4);
                }
            }
            
            predict_once = 0;
            moveCnt = 0;
            move_direction = MOVE_DIR_NONE;
            delay_move = 0;
            picDiff = 0;
            break;

        default:
            break;
    }

    
    return consumed; 
}
#endif


void menu_update(void)
{
    // _cwin_update(&cwin_menu_array[menu_ctx_v->index]);
    //ry_memcpy(frame_buffer, p_menu_fb, 86400);
    ry_hal_spi_flash_read(frame_buffer, (uint32_t)p_menu_fb, 86400);
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
}

/**
 * @brief   menu_index_sequence_reset
 *
 * @param   None
 *
 * @return  None
 */
void menu_index_sequence_reset(void)
{
    if (menu_ctx_v) {
        menu_ctx_v->index = 0;
        // LOG_ERROR("[menu_index_sequence_reset], menu_ctx.index 0\r\n");
    }
    else{
        //LOG_ERROR("[menu_select], menu ctx NULL!!!!!!!!!!!\r\n");
    }
}

void menu_select(u8_t touchY)
{
    if (!menu_ctx_v) {
        //LOG_ERROR("[menu_select], menu ctx NULL!!!!!!!!!!!\r\n");
        return;
    }
    
    int which_icon = 0; // 1 upper, 2 dn
    int app_seq = menu_ctx_v->index;
    int app_id;

    if (touchY >= 8) {
        //motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
        DEV_STATISTICS(home_back_count);
        wms_activity_back(NULL);
        return;
    }
    
    if (touchY < 4) {
        which_icon = 1;
    } else if (touchY > 4) {
        which_icon = 2;
        app_seq++;
    }


    #if 1
    app_id = app_prop_getIdBySeq(app_seq);

#if (RAWLOG_SAMPLE_ENABLE == TRUE)
    if (app_id == APPID_SYSTEM_SETTING)
#endif
    switch (app_id) {
        case APPID_SYSTEM_CARD:
            if (get_device_session() == DEV_SESSION_CARD) {
                break;
            }
            DEV_STATISTICS(card_bag_count);
            //LOG_INFO("[Menu] Select card.\r\n");
            wms_activity_jump(&activity_card, NULL);
            break;

        case APPID_SYSTEM_SPORT:
            DEV_STATISTICS(sport_count);
            //LOG_INFO("[Menu] Select sport.\r\n");
            wms_activity_jump(&activity_sports, NULL);
            break;

        case APPID_SYSTEM_SETTING:
            DEV_STATISTICS(setting_count);  
            //LOG_INFO("[Menu] Select setting.\r\n");
            wms_activity_jump(&activity_setting, NULL);
            break;

        case APPID_SYSTEM_MIJIA:
            DEV_STATISTICS(mijia_count);   
            //LOG_INFO("[Menu] Select mijia.\r\n");
            wms_activity_jump(&activity_mijia, NULL);
            break;

        case APPID_SYSTEM_DATA:
            DEV_STATISTICS(data_info_count);    
            //LOG_INFO("[Menu] Select data.\r\n");
            wms_activity_jump(&activity_data, NULL);
            break;

        case APPID_SYSTEM_ALARM:
            DEV_STATISTICS(alarm_count);    
            //LOG_INFO("[Menu] Select alarm.\r\n");
            wms_activity_jump(&activity_alarm, NULL);
            break;

        case APPID_SYSTEM_TIMER:
            //DEV_STATISTICS(alarm_count);    
            //LOG_INFO("[Menu] Select timer.\r\n");
            wms_activity_jump(&activity_timer, NULL);
            break;

        case APPID_SYSTEM_HRM:
            DEV_STATISTICS(hrm_count);    
            //LOG_INFO("[Menu] Select hrm.\r\n");
            wms_activity_jump(&activity_hrm, NULL);
            break;

        case APPID_SYSTEM_WEATHER:
            DEV_STATISTICS(weather_count);       
            //LOG_INFO("[Menu] Select weather.\r\n");
            wms_activity_jump(&activity_weather, NULL);
            break;

        case APPID_SYSTEM_ALIPAY:
            //LOG_INFO("[Menu] Select alipay.\r\n");
            wms_activity_jump(&activity_alipay, NULL);
            break;

        default:
            break;
    }
    #endif


#if 0

    if (which_icon == 1) {
        switch(menu_ctx_v->index) {
            case 0:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_setting, NULL);
#else                
                wms_activity_jump(&activity_card, NULL);
#endif                
                break;

            case 1:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_hrm, NULL);
#else                
                wms_activity_jump(&activity_sport, NULL);
#endif                
                break;

            case 2:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_sport, NULL);
#else                
                wms_activity_jump(&activity_hrm, NULL);
#endif                
                break;

            case 3:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_card, NULL);
#else                
                wms_activity_jump(&activity_mijia, NULL);
#endif                
                break;

            case 4:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_sfc_option, NULL);
#else                
                wms_activity_jump(&activity_weather, NULL);
#endif                
                break;

            case 5:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_weather, NULL);
#else                
                wms_activity_jump(&activity_data, NULL);
#endif                
                break;

            case 6:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_data, NULL);
#else                
                wms_activity_jump(&activity_setting, NULL);
#endif                
                break;

            case 7:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_mijia, NULL);
#else                
                wms_activity_jump(&activity_alarm, NULL);
#endif                
                break;

            default:
                break;
        }
    } else if (which_icon == 2) {
        switch(menu_ctx_v->index) {
            case 0:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_hrm, NULL);
#else                
                wms_activity_jump(&activity_sport, NULL);
#endif                
                break;

            case 1:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_sport, NULL);
#else                
                wms_activity_jump(&activity_hrm, NULL);
#endif                
                break;

            case 2:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_card, NULL);
#else                
                wms_activity_jump(&activity_mijia, NULL);
#endif                
                break;

            case 3:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_sfc_option, NULL);
#else                
                wms_activity_jump(&activity_weather, NULL);
#endif                
                break;

            case 4:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_weather, NULL);
#else                
                wms_activity_jump(&activity_data, NULL);
#endif                
                break;

            case 5:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_data, NULL);
#else                
                wms_activity_jump(&activity_setting, NULL);
#endif                
                break;

            case 6:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_mijia, NULL);
#else                
                wms_activity_jump(&activity_alarm, NULL);
#endif                
                break;

            case 7:
#if MENU_SEQUENCE_01
                wms_activity_jump(&activity_setting, NULL);
#else                
                wms_activity_jump(&activity_card, NULL);
#endif                
                break;

            default:
                break;
        }
    }
#endif
}



int menu_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;

    ry_sts_t status;
    int done = 0;
    
#if 0
    switch (p->action) {
        
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[menu_onProcessedTouchEvent] TP Action Click, y:%d\r\n", p->y);
            if (p->y >= 8) {
                motar_set_work(MOTAR_TYPE_STRONG_LINEAR, MOTAR_WORKING_TIME_GENERAY);
                wms_activity_back(NULL);
                break;
            }

            menu_select(p->y);

            
            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            LOG_DEBUG("[menu_onProcessedTouchEvent] TP Action Slide Down, y:%d, index:%d\r\n", p->y, menu_ctx_v->index);
            if ((--menu_ctx_v->index) < 0) {
                menu_ctx_v->index = 7;
            }
            menu_update();
            
            break;


        case TP_PROCESSED_ACTION_SLIDE_UP:
            LOG_DEBUG("[menu_onProcessedTouchEvent] TP Action Slide Up, y:%d, index:%d\r\n", p->y, menu_ctx_v->index);
            if ((++menu_ctx_v->index) > 7) {
                menu_ctx_v->index = 0;
            }
            menu_update();
            break;

        default:
            break;
    }
#endif
    
    return consumed; 
}

int menu_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}

int menu_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}

/**
 * @brief   Entry of the menu activity
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t menu_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;
    LOG_DEBUG("menu on menu_onCreate.\r\n");
    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    if (!menu_ctx_v) {
        menu_ctx_v = (menu_ctx_t*)ry_malloc(sizeof(menu_ctx_t));
        if (menu_ctx_v == NULL) {
            LOG_ERROR("[menu_onCreate]: No mem.\r\n");
            return RY_SET_STS_VAL(RY_CID_ACTIVITY_MENU, RY_ERR_NO_MEM);
        }

        menu_ctx_v->index = 0;
        menu_ctx_v->scrollbar = scrollbar_create(1, MENU_SCROLLBAR_LENGTH, 0x4A4A4A, 300);
        if (menu_ctx_v->scrollbar == NULL) {
            LOG_ERROR("[menu_onCreate]: No scrollbar.\r\n");
        }
        
    }
    

#if IMG_INTERNAL_ONLY
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_menu, menu_onProcessedTouchEvent);
#else
    //p_draw_fb = MENU_TOP + menu_ctx_v->index * 43200;

    p_menu_fb = MENU_TOP + menu_ctx_v->index * 43200;
    p_draw_fb = frame_buffer;
    wms_event_listener_add(WM_EVENT_TOUCH_RAW, &activity_menu, menu_onTouchEvent);
#endif
    wms_event_listener_add(WM_EVENT_MESSAGE, &activity_menu, menu_onMSGEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_menu, menu_onProcessedCardEvent);
    
    // menu_update();
    menu_last_display();

    wms_untouch_timer_stop();
    wms_quick_untouch_timer_start();

    return ret;
}


/**
 * @brief   API disable global interrupt
 *
 * @param   None
 *
 * @return  Previous irq status
 */
ry_sts_t menu_onDestrory(void)
{
    ry_sts_t ret = RY_SUCC;

    //LOG_DEBUG("menu on destrory.\r\n");

    wms_quick_untouch_timer_stop();
    wms_untouch_timer_start();
    
#if 1 && IMG_INTERNAL_ONLY
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_menu);
#else
    p_draw_fb = frame_buffer;
    wms_event_listener_del(WM_EVENT_TOUCH_RAW, &activity_menu);
#endif
    //wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_menu);
    wms_event_listener_del(WM_EVENT_MESSAGE, &activity_menu);
    wms_event_listener_del(WM_EVENT_CARD, &activity_menu);

    //ry_free((void*)menu_ctx_v);
    //menu_ctx_v = NULL;

    return ret;    
}
