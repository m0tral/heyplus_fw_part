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
#include <stdio.h>
#include "app_config.h"
#include "ry_type.h"
#include "ry_utils.h"
#include "ryos.h"
#include "ryos_timer.h"
#include <ry_hal_inc.h>
#include "scheduler.h"


/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>
#include "timer_management_service.h"
#include "device_management_service.h"
#include "window_management_service.h"
#include "card_management_service.h"
#include "AlarmClock.pb.h"
#include "activity_alarm_ring.h"
#include "activity_card.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"

/* resources */
#include "gui_img.h"
#include "ry_font.h"
#include"gui_msg.h"

#include"motar.h"


#define LONG_TOUCH_TIME_THRESH     2000
#define SHORT_TOUCH_TIME_THRESH    500

/*********************************************************************
 * CONSTANTS
 */ 
static uint8_t const alarm_arrow_png[] = {
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 255, 255, 255, 255, 0, 0, 0, 0, 0, 
144, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 144, 
2, 173, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 173, 2, 
0, 9, 198, 255, 255, 255, 255, 255, 255, 255, 255, 198, 9, 0, 
0, 0, 20, 219, 255, 255, 255, 255, 255, 255, 219, 20, 0, 0, 
0, 0, 0, 36, 235, 255, 255, 255, 255, 235, 36, 0, 0, 0, 
0, 0, 0, 0, 57, 246, 255, 255, 246, 57, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 82, 253, 253, 82, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 112, 112, 0, 0, 0, 0, 0, 0, 
};


/*********************************************************************
 * TYPEDEFS
 */
typedef struct {
    uint8_t cur_alarm;
    uint8_t state;
	uint8_t touch_press;
	uint8_t touch_release;
    uint8_t ring_times;    
	u8_t    touch_bar_deltaX;
	u32_t   touch_start_time;
	u32_t   touch_end_time;
	u32_t   ring_time_cnt;
    AlarmClockList* alarm_list;
} alarm_ring_t;

typedef enum {
	ALARM_ST_RING_UI = 1,		// active - shaking image
	ALARM_ST_CONTINUE_UI,		// short press for continue
	ALARM_ST_CONTINUE_UI_DONE,	// pressed for next remind
	ALARM_ST_CANCEL_UI,     	// canceled
	ALARM_ST_CANCEL_UI_DONE,  	// displaying of canceled done
	ALARM_ST_EXIT,		    	// stop and back to prev activity
}alarm_ui_t;

typedef enum {
	E_ALARM_TP_NONE = 0,		// nothing pressed
	E_ALARM_TP_PRESSED,			// button pressed
	E_ALARM_TP_RELEASED,	    // button released
}alarm_tp_t;

/*********************************************************************
 * VARIABLES
 */

activity_t activity_alarm_ring = {
    .name      = "alarm_ring",
    .onCreate  = activity_alarm_ring_onCreate,
    .onDestroy = activity_alarm_ring_onDestroy,
    .disableUntouchTimer = 1,
    .disableTouchMotar = 1,
    .priority  = WM_PRIORITY_H,
};

const char* const alarm_image[] = {
    "alarm_1.bmp",
    "alarm_2.bmp", 
};

static const char* const text_alarm = "Будильник";
static const char* const text_canceled = "Отменен";
static const char* const text_cancel_longpress [] = {
		"Держите",
		"для отмены",
};

static const char* const text_remind_later [] = {
		"Повторить",
		"через",
        "10 мин",
        "5 мин",
        "15 мин",
};

static u8_t const text_remind_min_value[] = {0, 0, 0, 0, 0, 1,  // 5 min
                                                0, 0, 0, 0, 0,  // 10 min
                                                0, 0, 0, 0, 2}; // 15 min

const u8_t height_alarm_image[] = {66, 66};

static const raw_png_descript_t icon_alarm_arrow = 
{
    .p_data = alarm_arrow_png,
    .width = 14,
    .height = 20,
    .fgcolor = White,
};

volatile alarm_ring_t alarm_ring_v;
extern tms_ctx_t tms_ctx_v;
tms_alarm_tbl_store_t* alrmClock = &tms_ctx_v.alarmTbl;
static u32_t app_alarm_ring_timer_id;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
ry_sts_t alarm_set_save(void);
static u8_t get_alarm_ring_times(void);

void gdispDrawAraea(u8_t x,u8_t y,u8_t cx, u8_t cy, u32_t color)
{
	u8_t ii = 0;
	for(ii = 0; ii<cy; ii++){
		gdispDrawLine(x, y+ii, cx, y+ii, color);
	}
}


ry_sts_t alarm_ring_ui(int8_t setting_item)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
	AlarmClockItem* p;
    uint8_t time[16];
	char alarmTag[25] = {0};
	ry_time_t alarmSysTime;
	static u8_t alarm_img_idx = 0;
	
    clear_buffer_black();
	
	p = &alarm_ring_v.alarm_list->items[setting_item];
	
	ry_memcpy(alarmTag, p->tag, 25);
	if (alarmTag[0] == 0) {
		ry_memset(alarmTag, 0, 25);
		ry_memcpy(alarmTag, text_alarm, strlen(text_alarm));
	}
	tms_get_time(&alarmSysTime);
	sprintf((char*)time, "%02d:%02d", alarmSysTime.hour, alarmSysTime.minute); 
 
    if (alarm_img_idx >= sizeof(alarm_image) / sizeof(const char*)){
        alarm_img_idx = 0;
    }
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)alarm_image[alarm_img_idx],\
                            0, 105 - height_alarm_image[alarm_img_idx], &w, &h, d_justify_center);
    alarm_img_idx ++;
    
    //FONTS_RY_CUSTOMIZE
    
	//LOG_DEBUG("\r\n snoozeMode=%d times=%d.\r\n", alrmClock.alarm_tbl.alarms[setting_item].snoozeMode,alrmClock.alarm_tbl.alarms[setting_item].ringTimes);
	if((alrmClock->alarm_tbl.alarms[setting_item].snoozeMode == 1) && \
		(alrmClock->alarm_tbl.alarms[setting_item].ringTimes < ALARM_SNOOZE_MAX_TIMES)){
		gdispFillStringBox( 0, 115, font_roboto_32.width, font_roboto_32.height,
                            (char*)time, (void*)font_roboto_32.font, White, Black, justifyCenter);  
		gdispFillStringBox( 0, 115+37, font_roboto_20.width, font_roboto_20.height,
                            alarmTag, (void*)font_roboto_20.font, HTML2COLOR(0x01F097), Black, justifyCenter); 
		
		if (alarm_ring_v.touch_press == 1) {
			if (alarm_ring_v.touch_bar_deltaX > 0){
				gdispDrawAraea(0, 230, alarm_ring_v.touch_bar_deltaX, 2, HTML2COLOR(0x97F001));
			}
		} else { // FONTS_RY_CUSTOMIZE
			draw_raw_png_to_framebuffer(&icon_alarm_arrow, 5, 203, White);
			gdispFillStringBox( 0, 180, font_roboto_20.width, font_roboto_20.height,
                                (char*)text_cancel_longpress[0],
                                (void*)font_roboto_20.font,
                                White, Black, justifyCenter); 
			gdispFillStringBox( 0, 210, font_roboto_20.width, font_roboto_20.height,
                                (char*)text_cancel_longpress[1],
                                (void*)font_roboto_20.font,
                                White, Black, justifyCenter); 
		}
	} else {
		gdispFillStringBox( 0, 115, font_roboto_32.width,font_roboto_32.height,(char*)time, (void*)font_roboto_32.font, White, Black,justifyCenter);  
		gdispFillStringBox( 0, 115 + 37, font_roboto_20.width, font_roboto_20.height,
                            alarmTag,
                            (void*)font_roboto_20.font,
                            HTML2COLOR(0x01F097), Black, justifyCenter); 
	}
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
    
    return ret;
}

ry_sts_t alarm_continue_ui(void)
{
	ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
	
	clear_buffer_black();
	draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"alarm_3.bmp",\
                            60, 120-66, &w, &h, d_justify_center);
	gdispFillStringBox( 0, 
                        138, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_remind_later[0], 
                        (void *)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  

    gdispFillStringBox( 0, 
                        138 + 25, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_remind_later[1],
                        (void *)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );
                        
    int text_remind_min_index = 2 + text_remind_min_value[get_alarm_snooze_interval()];
                        
    gdispFillStringBox( 0, 
                        138 + 50, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_remind_later[text_remind_min_index],
                        (void *)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );
                        
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
	
	return ret;
}

ry_sts_t alarm_cancel_ui(void)
{
	ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
	
	clear_buffer_black();
	
	draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)"alarm_3.bmp",\
                            60, 120-66, &w, &h, d_justify_center);
	gdispFillStringBox( 0, 
                        150, 
                        font_roboto_20.width,
                        font_roboto_20.height,
                        (char *)text_canceled, 
                        (void *)font_roboto_20.font, 
                        White,  
                        Black,
                        justifyCenter
                        );  
	
	ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);
	return ret;
}

void activity_alarm_ring_timeout_handler(void* arg)
{
	u32_t event = E_ALARM_TP_NONE;
	u32_t deltaT = 0;
	u8_t  timer_send_msg_cnt = 1;
	u32_t alarm_hold = 0;
	
	if ((alarm_ring_v.touch_press == 1) && (alarm_ring_v.touch_release == 0)) {
		event = E_ALARM_TP_PRESSED;
	} else if ((alarm_ring_v.touch_press == 0) && (alarm_ring_v.touch_release == 1)) {
		event = E_ALARM_TP_RELEASED;
	}

	switch(event){
        case E_ALARM_TP_NONE:
			// default continue alarm to timeout
            break;

        case E_ALARM_TP_PRESSED:
			deltaT = ry_hal_calc_ms(ry_hal_clock_time() - alarm_ring_v.touch_start_time);
			alarm_ring_v.touch_bar_deltaX = (deltaT < SHORT_TOUCH_TIME_THRESH) ? 0 : (deltaT * 120) / LONG_TOUCH_TIME_THRESH;
		 	// alarm_ring_v.touch_bar_deltaX = alarm_ring_v.touch_bar_deltaX - alarm_ring_v.touch_bar_deltaX % 25;

			//Long presss touch event detecte
			if (deltaT >= LONG_TOUCH_TIME_THRESH) {
 				// motar_stop(); 
				if ((alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].snoozeMode == 1) &&\
					(alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes < ALARM_SNOOZE_MAX_TIMES)) {
					if (alarm_ring_v.state != ALARM_ST_CANCEL_UI_DONE) {
						alarm_ring_v.state = ALARM_ST_CANCEL_UI;
					}
					alarm_ring_v.ring_time_cnt = 0;
					LOG_DEBUG("cancel snooze alarm!\r\n");
				} else {
					if (alarm_ring_v.state != ALARM_ST_CANCEL_UI_DONE) {
						alarm_ring_v.state = ALARM_ST_CANCEL_UI;
					}
					LOG_DEBUG("exit alarm!\r\n");
				}
				alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes = ALARM_SNOOZE_MAX_TIMES;			//设置停止贪睡
				alarm_set_save();
			}
            break;

        case E_ALARM_TP_RELEASED:
			// tp is released, should not reach here, because long press detected 
            break;

        default:
            break;
    }
	
	// alarm timeout detect and set motor
	if (alarm_ring_v.ring_times >= 18 * 5) {
		alarm_hold = ((event == E_ALARM_TP_PRESSED) && (deltaT < (LONG_TOUCH_TIME_THRESH + 200)) && \
				(alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].snoozeMode == 1));
		if (!alarm_hold){
			alarm_ring_v.state = ALARM_ST_EXIT;
		}
	} else {
		if (alarm_ring_v.state == ALARM_ST_RING_UI){
			if ((alarm_ring_v.ring_times % 18) == 0){
				motar_set_work(MOTAR_TYPE_QUICK_LOOP, 3);
			}
		}
	}
	
	alarm_ring_v.ring_times ++;
	alarm_ring_v.ring_time_cnt ++;
	LOG_DEBUG("[alarm_ring_timer]press_time: %d, touch_press:%d, touch_release:%d, event:%d, state:%d, ring_time_cnt:%d, hold:%d\r\n", \
		deltaT, alarm_ring_v.touch_press, alarm_ring_v.touch_release, event, alarm_ring_v.state, alarm_ring_v.ring_times, alarm_hold);

	if ((alarm_ring_v.state == ALARM_ST_CANCEL_UI_DONE) || (alarm_ring_v.state == ALARM_ST_CONTINUE_UI_DONE)){
		timer_send_msg_cnt = 10;
		motar_stop(); 
	}

	if (alarm_ring_v.ring_time_cnt % timer_send_msg_cnt == 0){
		ry_sched_msg_send(IPC_MSG_TYPE_ALARM_RING, 0, NULL);			//send msg to refresh ui
	}

    if (strcmp(wms_activity_get_cur_name(), activity_alarm_ring.name) == 0){
		ry_timer_start(app_alarm_ring_timer_id, 100, activity_alarm_ring_timeout_handler, NULL);
	}
}


int alarm_ring_onRawTouchEvent(ry_ipc_msg_t* msg)
{
	tp_event_t *p = (tp_event_t*)msg->data;
	
    switch (p->action) {
		case TP_ACTION_DOWN:
			if (p->y >= 7) {
				alarm_ring_v.touch_start_time = p->t;
				alarm_ring_v.touch_press = 1;
				alarm_ring_v.touch_release = 0;
				LOG_DEBUG("precessed y :%d \r\n",p->y);
			}
			break;
			
		case TP_ACTION_UP:
			if ((alarm_ring_v.touch_press == 1) && (alarm_ring_v.touch_release == 0)){
				alarm_ring_v.touch_end_time = p->t;
				alarm_ring_v.touch_press = 0;
				alarm_ring_v.touch_release = 1;

				if (alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].snoozeMode == 1){
					u32_t delta_time = ry_hal_calc_ms(ry_hal_clock_time() - alarm_ring_v.touch_start_time);
					if (delta_time < SHORT_TOUCH_TIME_THRESH){
						if (alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes < ALARM_SNOOZE_MAX_TIMES) {
							alarm_ring_v.state = ALARM_ST_CONTINUE_UI;
							alarm_ring_v.ring_time_cnt = 0;
							alarm_set_save();
						}
					} else if (delta_time > LONG_TOUCH_TIME_THRESH){	//should not reach here, because long press detected in timer	
	 					alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes = ALARM_SNOOZE_MAX_TIMES;
						alarm_ring_v.state = ALARM_ST_CANCEL_UI;
						alarm_set_save();
					} else {
						//continue alarm when tp-up in the time is between 500ms - 2000ms
					}
				} else {
					alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes = ALARM_SNOOZE_MAX_TIMES;
					alarm_ring_v.state = ALARM_ST_EXIT;
					alarm_set_save();
				}
				LOG_DEBUG("release y :%d \r\n",p->y);
			}
			break;
		
        default:
            break;
    }
    
    return 1; 
}

int alarm_ring_onAlarmRingEvent(ry_ipc_msg_t* msg)
{
	// LOG_DEBUG("[%s] msg ---%d, state:%d\n", __FUNCTION__, msg->msgType, alarm_ring_v.state);
	if (alarm_ring_v.state == ALARM_ST_RING_UI) {
		alarm_ring_ui(alarm_ring_v.cur_alarm);
	}else if (alarm_ring_v.state == ALARM_ST_CONTINUE_UI) {
		alarm_continue_ui();
		alarm_ring_v.state = ALARM_ST_CONTINUE_UI_DONE;
	}else if (alarm_ring_v.state == ALARM_ST_CANCEL_UI)	{
		alarm_cancel_ui();
		alarm_ring_v.state = ALARM_ST_CANCEL_UI_DONE;
	} else if ((alarm_ring_v.state == ALARM_ST_EXIT) || \
			   (alarm_ring_v.state == ALARM_ST_CANCEL_UI_DONE) || \
			   (alarm_ring_v.state == ALARM_ST_CONTINUE_UI_DONE)) {
		alarm_ring_v.state = ALARM_ST_EXIT;
		ry_timer_stop(app_alarm_ring_timer_id);
		wms_activity_back(NULL);
	}
    return 1; 
}


/**
 * @brief   Entry of the Alarm activity
 *
 * @param   None
 *
 * @return  activity_alarm_ring_onCreate result
 */
ry_sts_t activity_alarm_ring_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;
//	ry_hal_flash_read(FLASH_ADDR_ALARM_TABLE, (u8_t*)alrmClock, sizeof(tms_alarm_tbl_store_t));
    
//    LOG_ERROR("[alarm event] addr: 0x%08x, magic: 0x%08x", alrmClock, alrmClock->magic);
	
	if (usrdata) {
		// Get data from usrdata and must release the buffer.
		alarm_ring_v.cur_alarm = *(uint32_t*)usrdata;
		if (alarm_ring_v.cur_alarm >= MAX_ALARM_NUM){
			alarm_ring_v.cur_alarm = alarm_ring_v.cur_alarm % MAX_ALARM_NUM;
		}
		ry_free(usrdata);
	} else {
        wms_activity_back(NULL);
	}
    
//    wms_activity_back(NULL);
//    return ret;    
	
	alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes = get_alarm_ring_times();
	if (alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes > ALARM_SNOOZE_MAX_TIMES) {
		alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes = ALARM_SNOOZE_MAX_TIMES;
		wms_activity_back(NULL);
		alarm_set_save();
		return ret;
	}
	
	alarm_set_save();

	wms_event_listener_add(WM_EVENT_TOUCH_RAW, &activity_alarm_ring, alarm_ring_onRawTouchEvent);
	wms_event_listener_add(WM_EVENT_ALARM_RING, &activity_alarm_ring, alarm_ring_onAlarmRingEvent);
	
    alarm_ring_v.alarm_list = (AlarmClockList*)ry_malloc(sizeof(AlarmClockList));
    alarm_list_get(alarm_ring_v.alarm_list);
    alarm_ring_v.ring_times = 0;
	alarm_ring_v.touch_start_time = 0;
	alarm_ring_v.touch_press = 0;
	alarm_ring_v.touch_release = 0;

    if (app_alarm_ring_timer_id == 0){
        ry_timer_parm timer_para;
        /* Create the timer */
        timer_para.timeout_CB = activity_alarm_ring_timeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "alarm_ring_timer";
        app_alarm_ring_timer_id = ry_timer_create(&timer_para);
    }
	
	ry_timer_start(app_alarm_ring_timer_id, 100, activity_alarm_ring_timeout_handler, NULL);
	
	alarm_ring_v.state = ALARM_ST_RING_UI;
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

    return ret;
}

/**
 * @brief   API to exit Alarm activity
 *
 * @param   None
 *
 * @return  alarm activity Destrory result
 */
ry_sts_t activity_alarm_ring_onDestroy(void)
{
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_RAW,  &activity_alarm_ring);
	wms_event_listener_del(WM_EVENT_ALARM_RING, &activity_alarm_ring);

    if (alarm_ring_v.alarm_list) {
        ry_free((u8_t*)alarm_ring_v.alarm_list);
        alarm_ring_v.alarm_list = NULL;
    }
    wms_untouch_timer_start();  
	LOG_DEBUG("activity_alarm_ring_onDestroy.\r\n");
	motar_stop(); 
    return RY_SUCC;
}

static u8_t get_alarm_ring_times(void)
{
	u8_t  ringTimesCount  = 0;
	u16_t sysMinuteTime   = 0;
	u16_t alarmMinuteTime = 0;
	ry_time_t alarmSysTime;
	
	tms_get_time(&alarmSysTime);
	
	sysMinuteTime   = alarmSysTime.hour*60 + alarmSysTime.minute;
	alarmMinuteTime = alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].hour * 60
            + alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].minute;
	
    int alarm_snooze_interval = get_alarm_snooze_interval();
    
	if (sysMinuteTime >= alarmMinuteTime) {
		ringTimesCount = (sysMinuteTime - alarmMinuteTime)/alarm_snooze_interval + 1;
	}else {
		ringTimesCount = (1440 - (alarmMinuteTime - sysMinuteTime))/alarm_snooze_interval + 1;
	}
	
	if (alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].snoozeMode == 0) {
		ringTimesCount = ALARM_SNOOZE_MAX_TIMES;
	}
	return ringTimesCount;
}

ry_sts_t alarm_set_save(void)
{
	ry_sts_t status;
    bool needSave = false;

	LOG_DEBUG("Alarm store:snoozeMode=%d ringTimes=%d hour=%d minute=%d en=%d\r\n",\
		alrmClock.alarm_tbl.alarms[alarm_ring_v.cur_alarm].snoozeMode,alrmClock.alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes\
		,alrmClock.alarm_tbl.alarms[alarm_ring_v.cur_alarm].hour,alrmClock.alarm_tbl.alarms[alarm_ring_v.cur_alarm].minute,\
		alrmClock.alarm_tbl.alarms[alarm_ring_v.cur_alarm].enable);
	
	if (alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].ringTimes == ALARM_SNOOZE_MAX_TIMES) {
		if (alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].repeatType[0] == 0){
			alarm_set_enable(alrmClock->alarm_tbl.alarms[alarm_ring_v.cur_alarm].id, 0);
		}
        else {
            needSave = true;
        }
	}
	
    if (needSave) {
        status = ry_hal_flash_write(FLASH_ADDR_ALARM_TABLE, (u8_t*)alrmClock, sizeof(tms_alarm_tbl_store_t));
        if (status != RY_SUCC) {
            LOG_DEBUG("[alarm ring]Alarm store failure!!\r\n");
            return RY_SET_STS_VAL(RY_CID_TMS, RY_ERR_PARA_SAVE);
        }
    }

    return RY_SUCC;
}
