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
#include "ry_nfc.h"


/* ui specified */
#include "touch.h"
#include <sensor_alg.h>
#include <app_interface.h>

#include "window_management_service.h"
#include "card_management_service.h"
#include "device_management_service.h"
#include "activity_card.h"
#include "gfx.h"
#include "gui.h"
#include "gui_bare.h"
#include "ry_statistics.h"

/* resources */
#include "gui_img.h"
#include "ry_font.h"

#include"gui_msg.h"
#include "motar.h"
#include "scrollbar.h"
#include "ry_statistics.h"
#include "m_card_jiangsu.h"


/*********************************************************************
 * CONSTANTS
 */ 
#define CARD_MINE_NUM         18//7

#define DRAW_TYPE_UNSELECT    0
#define DRAW_TYPE_SELECT      1


/*********************************************************************
 * TYPEDEFS
 */

typedef enum {
    E_CARD_XIAOMI = 0,
    E_CARD_BEIJING,  
    E_CARD_SHENZHEN,
    E_CARD_LINGNAN,
    E_CARD_SHANGHAI,
    E_CARD_GATE,
    E_CARD_JJJ,
    E_CARD_WHT,
    E_CARD_CQT,
    E_CARD_JST,
    E_CARD_YCT = (0x80|E_CARD_LINGNAN),
    E_CARD_CAT = 0x0B,
    E_CARD_HFT,
    E_CARD_GXT,
    E_CARD_JLT,
    E_CARD_HEB,
    E_CARD_LCT,
    E_CARD_QDT,
    E_CARD_MAX,
} e_card_idx_t;


typedef struct {
    int8_t    list;
} ry_card_ctx_t;

typedef struct {
    transitCard_tbl_t* transitCardTbl;
    accessCard_tbl_t*  accessCardTbl;
    int                index;
    e_card_idx_t       curCardIdx;
    card_access_t      *curAccessCard;
    card_transit_t     *curTransitCard;
    scrollbar_t        *scrollbar;
    u8_t               busy;
    u8_t               abnormal;
    int                selectIndex;
    ryos_mutex_t       *mutex;
    int                selectTimer;
    int                defaultCardTimer;
} activity_card_ctx_t;

 typedef struct {
    const char* img;
    const char* str;
} card_disp_t;

typedef struct {
    u32_t color;
    const char* img;
} card_ui_t;
 
typedef struct {
    uint8_t   idx;    
    uint32_t  id;
    float     balance;
    float     balance_shresh;
} card_info_t;



/*********************************************************************
 * VARIABLES
 */
extern activity_t activity_surface;
extern activity_t msg_activity;


const card_disp_t card_disp_bmp[CARD_MINE_NUM] = {
    {"m_card_03_xiaomi.bmp",     "xiaomi"},
    {"m_card_02_beijing.bmp",    "beijing"},
    {"m_card_01_shenzhen.bmp",   "shenzhen"},  
    {"m_card_00_guangzhou.bmp",  "guangzhou"},  
    {"m_card_04_shanghai.bmp",   "shanghai"},
    {"m_card_05_gate.bmp",       "gate"},
    {"m_card_02_beijing.bmp",    "beijing"},
    {"m_card_wuhan.bmp",         "wuhan"},
    {"m_card_chongqing.bmp",     "chongqing"},
    {"m_card_suzhou.bmp",        "suzhou"},
    {"m_card_08_yangcheng.bmp",  "yangcheng"},
    {"m_card_changan.bmp",       "changan"},
    {"m_card_hefei.bmp",         "hefei"},
    {"m_card_guangxi.bmp",       "guangxi"},
    {"m_card_jiling.bmp",        "jiling"},
    {"m_card_haerbin.bmp",       "haerbin"},
    {"m_card_lvcheng.bmp",       "lvcheng"},
    {"m_card_qingdao.bmp",       "qingdao"},
};

const card_ui_t card_ui[5] = {
    {CARD_ACCESS_COLOR_HOME,     "m_card_07_home.bmp"},
    {CARD_ACCESS_COLOR_UNIT,     "m_card_05_gate.bmp"},
    {CARD_ACCESS_COLOR_NEIGHBOR, "m_card_06_building.bmp"},
    {CARD_ACCESS_COLOR_COMPANY,  "m_card_05_gate0.bmp"},
    {CARD_ACCESS_COLOR_XIAOMI,   "m_card_03_xiaomi.bmp"},
};

const char* const text_card_select [] =  {
        "Активна",				// 0
		"Ошибка",				// 1
		"Нет памяти",			// 2
		"Не найдена",			// 3
		"Ошибка",				// 4
		"Перезапуск",			// 5
		"Чтение", 				// 6
		"Приложите", 			// 7
		"Выберите", 			// 8
};

const char* const text_add_at_phone [] = {
		"Добавьте",
		"в телефоне",
};

const char* const text_card_create [] = {
		"Проверка",
		"Приложите",
		"карту",
		"Ошибка",           // 3
		"чтения",
		"Успешно",          // 5
		"прочитана",
};

const char* const text_card_copy [] = {
		"Копирование",
		"Копируется",
		"подождите",
		"Ошибка",           // 3
		"копирования",
		"Успешно",          // 5
		"скопирована",
};

const char* const text_card_delete [] = {
		"Удаление",
		"подождите",
		"Ошибка",           // 2
		"удаления",
		"Успешно",          // 4
		"удалена",
};


activity_t activity_card = {
    .name      = "card",
    .onCreate  = activity_card_onCreate,
    .onDestroy = activity_card_onDestrory,
    .disableUntouchTimer = 0,
    .priority = WM_PRIORITY_M,
};

activity_t activity_card_session = {
    .name      = "card_session",
    .onCreate  = activity_card_session_onCreate,
    .onDestroy = activity_card_session_onDestrory,
    .disableUntouchTimer = 1,
    .priority = WM_PRIORITY_M,
    .disableWristAlg = 1,
};


activity_card_ctx_t *activity_card_ctx_v;


/*********************************************************************
 * LOCAL FUNCTIONS
 */
void draw_card(int index);
void show_cards(void);
e_card_idx_t get_current_card(int index);
int card_session_onProcessedTouchEvent(ry_ipc_msg_t* msg);
void activity_card_select(e_card_idx_t type);
void activity_card_select_timer_start(void* arg);
void activity_card_select_timer_stop(void);
void activity_card_defaultCard_timer_stop(void);


int activity_card_onGateSelected(u32_t card_id, u8_t status, char* name, u8_t* aid, u8_t aidLen, void* usrdata)
{
    u8_t w, h;
    e_card_idx_t type;
    int do_reset = 0;

    if (activity_card_ctx_v->selectIndex != activity_card_ctx_v->index && status != CARD_SELECT_STATUS_TIMEOUT) {
        LOG_DEBUG("[%s] Not current card. selectedIdx:%d, currentIdx:%d\r\n", 
            __FUNCTION__,
            activity_card_ctx_v->selectIndex,
            activity_card_ctx_v->index);
        
        activity_card_ctx_v->busy = 0;
        if (ry_timer_isRunning(activity_card_ctx_v->selectTimer) == FALSE) {
            type = get_current_card(activity_card_ctx_v->index);
            activity_card_select(type);
        }
        return 0;
    }

    LOG_DEBUG("[%s] Current card: %d.\r\n", __FUNCTION__, activity_card_ctx_v->selectIndex);

    ryos_mutex_lock(activity_card_ctx_v->mutex);
    
    clear_buffer_black();

    /* Start to draw */
    draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)card_ui[card_id].img, \
                            0, 4, &w, &h, d_justify_center);

    //card title
    gdispFillStringBox( 0, 88, RM_OLED_WIDTH, font_roboto_20.height,
                        name, (void*)font_roboto_20.font, White, Black, justifyCenter);  

    switch (status) {
        case CARD_SELECT_STATUS_OK:
            //gdispFillStringBox( 60, 190, 22, 22, (char*)"请靠近 ", NULL, Green,  Black, justifyCenter);
            //gdispFillStringBox( 60, 217, 22, 22, (char*)"读卡器 ", NULL, Green,  Black, justifyCenter);
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 170, &w, &h, d_justify_center);
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[0],
                                (void*)font_roboto_20.font,
                                Green, Black,
                                justifyCenter);
            break;

        case CARD_SELECT_STATUS_FAIL:
            DEV_STATISTICS(card_change_fail_count);
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[1], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_NO_MEM:
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[2], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_NOT_FOUND:
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[3], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_TIMEOUT:
        case CARD_SELECT_STATUS_SE_FAIL:
            gdispFillStringBox( 0, 190, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[4], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            gdispFillStringBox( 0, 217, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[5], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            do_reset = 1;
            
            break;
    }

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    ryos_mutex_unlock(activity_card_ctx_v->mutex);

    motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 100);

    //wms_untouch_timer_start();
    activity_card_ctx_v->busy = 0;

    if (do_reset) {
        ryos_delay_ms(3000);
        activity_card_ctx_v->abnormal = 1;
        add_reset_count(CARD_RESTART_COUNT);
        ry_system_reset();
    }

    return 0;
}

int activity_card_onCardSelcted(u32_t card_id, u8_t status, char* name, u8_t* aid, u8_t aidLen, void* usrdata)
{
    u8_t w, h;
    e_card_idx_t type;
    int balance = (int)usrdata;
    int integer = -1;
    int decimal = -1;
    

    if (activity_card_ctx_v->selectIndex != activity_card_ctx_v->index && status != CARD_SELECT_STATUS_TIMEOUT) {
        LOG_DEBUG("[%s] Not current card. selectedIdx:%d, currentIdx:%d\r\n", 
            __FUNCTION__, 
            activity_card_ctx_v->selectIndex,
            activity_card_ctx_v->index);
        
        activity_card_ctx_v->busy = 0;
        if (ry_timer_isRunning(activity_card_ctx_v->selectTimer) == FALSE) {
            type = get_current_card(activity_card_ctx_v->index);
            activity_card_select(type);
        }
        
        return 0;
    }

    LOG_DEBUG("[%s] Current card: %d.\r\n", __FUNCTION__, activity_card_ctx_v->selectIndex);

    ryos_mutex_lock(activity_card_ctx_v->mutex);
    
    clear_buffer_black();

    /* Start to draw */
    if (card_id == CARD_ID_YCT) {
        card_id = 10;
    }

    if (card_id == 9) {
        draw_raw_img24_to_framebuffer(0, 6, 112, 70, (u8_t*)gImage_m_card_jiangsu);
    } else {
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)card_disp_bmp[card_id].img, \
                            0, 4, &w, &h, d_justify_center);
    }

    //card title
    gdispFillStringBox( 0, 88, RM_OLED_WIDTH, font_roboto_20.height,
                        name, (void*)font_roboto_20.font, White, Black, justifyCenter);      

    switch (status) {
        case CARD_SELECT_STATUS_OK:
            if (balance >= 0) {
                integer = balance / 100;
                decimal = balance % 100;
            }
            
            char balance_str1[10];
            char balance_str2[10];
            int width;
            int str1_pos, str2_pos;
            if (integer < 10 && integer >= 0) {
                sprintf((char*)balance_str1, "Y ");
                sprintf((char*)balance_str2, "%d.%02d", integer, decimal);  
                width = getStringWidth(balance_str2);

            } else if (integer >= 10) {
                sprintf((char*)balance_str1, "Y "); 
                sprintf((char*)balance_str2, "%02d.%02d", integer, decimal); 
                width = getStringWidth(balance_str2);
            } else if (integer < 0) {
                sprintf((char*)balance_str1, "Y "); 
                sprintf((char*)balance_str2, "--.--"); 
                width = getStringWidth(balance_str2);
            }
            str1_pos = (120 - width)/2 - 11;
            str2_pos = str1_pos + 18;
            
            //gdispFillStringBox( 60, 120, 22, 22, (char*)"余额", NULL, White,  Black, justifyCenter);
            gdispFillStringBox( str1_pos, 120, 22, 22, (char*)balance_str1, NULL, White,  Black, justifyLeft);
            gdispFillStringBox( str2_pos, 124, 22, 22, (char*)balance_str2, NULL, White,  Black, justifyLeft);
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 170, &w, &h, d_justify_center);
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[0], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_FAIL:
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[1], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_NO_MEM:
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[2], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_NOT_FOUND:
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[3], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            break;

        case CARD_SELECT_STATUS_TIMEOUT:
        case CARD_SELECT_STATUS_SE_FAIL:
            gdispFillStringBox( 0, 190, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[4], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            gdispFillStringBox( 0, 217, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[5], (void*)font_roboto_20.font, Green, Black, justifyCenter);
            activity_card_ctx_v->abnormal = 1;
            break;
    }

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    ryos_mutex_unlock(activity_card_ctx_v->mutex);

    motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 200);

    //wms_untouch_timer_start();
    activity_card_ctx_v->busy = 0;

    return 0;
}


void activity_card_select(e_card_idx_t type)
{
    if (type == E_CARD_GATE) {
        DEV_STATISTICS(door_card_count);
        //if (activity_card_ctx_v->curAccessCard->selected == 0) {
            activity_card_ctx_v->busy = 1;
            activity_card_ctx_v->selectIndex = activity_card_ctx_v->index;
            //card_selectGate(CARD_ID_GATE, activity_card_ctx_v->curAccessCard, activity_card_onGateSelected);
            cms_cmd_selectGate_t para;
            para.card_id = CARD_ID_GATE;
            para.pa = activity_card_ctx_v->curAccessCard;
            para.cb = activity_card_onGateSelected;
            cms_msg_send(CMS_CMD_SELECT_GATE_CARD, sizeof(cms_cmd_selectGate_t), (void*)&para);
        //}
    } else {
        DEV_STATISTICS(traffic_card_count);
        //if (activity_card_ctx_v->curTransitCard->selected == 0) {
			activity_card_ctx_v->busy = 1;
            activity_card_ctx_v->selectIndex = activity_card_ctx_v->index;
            //card_selectTransit(activity_card_ctx_v->curTransitCard, activity_card_onCardSelcted);
            //card_selectCard(activity_card_ctx_v->curTransitCard->type, activity_card_ctx_v->curTransitCard, activity_card_onCardSelcted);
            cms_cmd_selectTransit_t para;
            para.card_id = activity_card_ctx_v->curTransitCard->type;
            para.pt = activity_card_ctx_v->curTransitCard;
            para.cb = activity_card_onCardSelcted;
            cms_msg_send(CMS_CMD_SELECT_TRANSIT_CARD, sizeof(cms_cmd_selectTransit_t), (void*)&para);
        //}
    }
}



int card_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int totalNum;
    int consumed = 1;
    e_card_idx_t type;

    totalNum = activity_card_ctx_v->accessCardTbl->curNum + activity_card_ctx_v->transitCardTbl->curNum;
    
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            
			LOG_DEBUG("[card_onProcessedTouchEvent] TP_Action Click, y:%d busy:%d, abnormal:%d\r\n", 
										p->y, activity_card_ctx_v->busy, activity_card_ctx_v->abnormal);
            if ((p->y >= 8)){

                if (totalNum == 0) {
                    wms_activity_back(NULL);
                    return consumed;
                }

                if (activity_card_ctx_v->abnormal == 1) {
                    ry_nfc_msg_send(RY_NFC_MSG_TYPE_POWER_OFF, NULL);
                    wms_activity_back(NULL);
                    return consumed;
                }

                if (activity_card_ctx_v->busy == 1) {
                    return consumed;
                }

                activity_card_select_timer_stop();
                wms_activity_back(NULL);
                return consumed;
            }

#if 1
            if ((p->y <= 7)){

                if (totalNum == 0 || activity_card_ctx_v->busy == 1) {
                    return consumed;
                }

                if (activity_card_ctx_v->abnormal == 1) {
                    wms_activity_back(NULL);
                    return consumed;
                }

                if (totalNum > 1) {
                    break;
                }

#if 1
                LOG_DEBUG("card_onProcessedTouchEvent list:%d\r\n", activity_card_ctx_v->index);
                //show_cards();
                type = get_current_card(activity_card_ctx_v->index);

                

                if (type == E_CARD_GATE) {
                    DEV_STATISTICS(door_card_count);
                    //if (activity_card_ctx_v->curAccessCard->selected == 0) {
                        activity_card_ctx_v->busy = 1;
                        //card_selectGate(CARD_ID_GATE, activity_card_ctx_v->curAccessCard, activity_card_onGateSelected);
                        cms_cmd_selectGate_t para;
                        para.card_id = CARD_ID_GATE;
                        para.pa = activity_card_ctx_v->curAccessCard;
                        para.cb = activity_card_onGateSelected;
                        cms_msg_send(CMS_CMD_SELECT_GATE_CARD, sizeof(cms_cmd_selectGate_t), (void*)&para);
                    //}
                } else {
                    DEV_STATISTICS(traffic_card_count);
                    //if (activity_card_ctx_v->curTransitCard->selected == 0) {
											  activity_card_ctx_v->busy = 1;
                        //card_selectTransit(activity_card_ctx_v->curTransitCard, activity_card_onCardSelcted);
                        //card_selectCard(activity_card_ctx_v->curTransitCard->type, activity_card_ctx_v->curTransitCard, activity_card_onCardSelcted);
                        cms_cmd_selectTransit_t para;
                        para.card_id = activity_card_ctx_v->curTransitCard->type;
                        para.pt = activity_card_ctx_v->curTransitCard;
                        para.cb = activity_card_onCardSelcted;
                        cms_msg_send(CMS_CMD_SELECT_TRANSIT_CARD, sizeof(cms_cmd_selectTransit_t), (void*)&para);
                    //}
                }
#endif
            }
#endif
            break;

        case TP_PROCESSED_ACTION_SLIDE_UP:

            LOG_DEBUG("[card_onProcessedTouchEvent] TP_Action Slide Up, y:%d\r\n", p->y);
            if (totalNum == 0) {
                break;
            }

            //wms_untouch_timer_stop();

            activity_card_ctx_v->index++;
            if (activity_card_ctx_v->index >= totalNum) {
                activity_card_ctx_v->index = totalNum - 1;
                break;
            }

            type = get_current_card(activity_card_ctx_v->index);
            DEV_STATISTICS(card_change_count);

            show_cards();

            activity_card_select_timer_stop();
            activity_card_select_timer_start(NULL);


            break;

        case TP_PROCESSED_ACTION_SLIDE_DOWN:
            // To see message list

            LOG_DEBUG("[card_onProcessedTouchEvent] TP_Action Slide Down, y:%d\r\n", p->y);

            if (totalNum == 0) {
                break;
            }

            //wms_untouch_timer_stop();

            activity_card_ctx_v->index--;
            if (activity_card_ctx_v->index < 0) {
                activity_card_ctx_v->index = 0;
                break;
            }

            //draw_card(activity_card_ctx_v->index);
            type = get_current_card(activity_card_ctx_v->index);
            DEV_STATISTICS(card_change_count);
            show_cards();

            activity_card_select_timer_stop();
            activity_card_select_timer_start(NULL);

         
            break;

        default:
            break;
    }
    return consumed; 
}

int card_onMSGEvent(ry_ipc_msg_t* msg)
{
    void * usrdata = (void *)0xff;
    wms_activity_jump(&msg_activity, usrdata);
    return 1;
}


e_card_idx_t get_current_card(int index)
{
    card_transit_t* pt = NULL;
    card_access_t*  pa = NULL;
    e_card_idx_t card_type;
    int i, num;

    if (activity_card_ctx_v->transitCardTbl->curNum + activity_card_ctx_v->accessCardTbl->curNum == 0) {
        return E_CARD_MAX;
    }

    num = 0;
    if (index < activity_card_ctx_v->transitCardTbl->curNum) {
        for (i = 0; (num < activity_card_ctx_v->transitCardTbl->curNum) && (i<MAX_TRANSIT_CARD_NUM); i++) {
            pt = &activity_card_ctx_v->transitCardTbl->cards[i];
            
            if (is_aid_empty(pt->aid)) {
                continue;
            }

            if (index == num) {
                /* Draw card */
                card_type = pt->type;
                activity_card_ctx_v->curTransitCard = pt;
                break;
            }
            num++; 
        }
    } else {
        for (i = 0; (num < activity_card_ctx_v->accessCardTbl->curNum) && (i<MAX_ACCESS_CARD_NUM); i++) {
            pa = &activity_card_ctx_v->accessCardTbl->cards[i];
            
            if (is_aid_empty(pa->aid)) {
                continue;
            }

            if ((index - activity_card_ctx_v->transitCardTbl->curNum) == num) {
                /* Draw card */
                card_type = E_CARD_GATE;
                activity_card_ctx_v->curAccessCard = pa;
                break;
            }
            num++; 
        }
    }

    return card_type;
}

int find_selected_card(void)
{
    card_transit_t* pt = NULL;
    card_access_t*  pa = NULL;
    e_card_idx_t card_type;
    int i, num;

    if (activity_card_ctx_v->transitCardTbl->curNum + activity_card_ctx_v->accessCardTbl->curNum == 0) {
        return 0;
    }

    num = 0;

    for (i = 0; (num < activity_card_ctx_v->transitCardTbl->curNum) && (i<MAX_TRANSIT_CARD_NUM); i++) {
        pt = &activity_card_ctx_v->transitCardTbl->cards[i];
        
        if (is_aid_empty(pt->aid)) {
            continue;
        }

        if (pt->selected) {
            return num;
        }

        num++; 
    }

    num = 0;

    for (i = 0; (num < activity_card_ctx_v->accessCardTbl->curNum) && (i<MAX_ACCESS_CARD_NUM); i++) {
        pa = &activity_card_ctx_v->accessCardTbl->cards[i];
        
        if (is_aid_empty(pa->aid)) {
            continue;
        }

        if (pa->selected) {
            return num + activity_card_ctx_v->transitCardTbl->curNum;
        }
        num++; 
    }

    return 0;
}


//void draw_card(u8_t type, u8_t selected)
void draw_card(int index)
{
    u8_t w, h;
    u8_t type, selected;
    card_transit_t* pt = NULL;
    card_access_t*  pa = NULL;
    int i, num;
    int integer = -1;
    int decimal = -1;

    /* Get which card to draw */
    num = 0; 
    if (index < activity_card_ctx_v->transitCardTbl->curNum) {
        for (i = 0; (num < activity_card_ctx_v->transitCardTbl->curNum) && (i<MAX_TRANSIT_CARD_NUM); i++) {
            pt = &activity_card_ctx_v->transitCardTbl->cards[i];
            
            if (is_aid_empty(pt->aid)) {
                continue;
            }

            if (index == num) {
                /* Draw card */
                type     = pt->type; 
                selected = pt->selected;
                break;
            }
            num++; 
        }
    } else {
        for (i = 0; (num < activity_card_ctx_v->accessCardTbl->curNum) && (i<MAX_ACCESS_CARD_NUM); i++) {
            pa = &activity_card_ctx_v->accessCardTbl->cards[i];
            
            if (is_aid_empty(pa->aid)) {
                continue;
            }

            if ((index - activity_card_ctx_v->transitCardTbl->curNum) == num) {
                /* Draw card */
                type     = pa->type; 
                selected = pa->selected;
                break;
            }
            num++; 
        }
    }

    if (type >= E_CARD_MAX) {
        type = E_CARD_GATE;
    }

    if (type == E_CARD_YCT) {
        type = 10;
    }

    if (selected > DRAW_TYPE_SELECT) {
        selected = DRAW_TYPE_UNSELECT;
    }
    
    /* Start to draw */
    if (type == E_CARD_GATE) {
        draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)card_ui[pa->color].img, \
                            0, 4, &w, &h, d_justify_center);
    } else {
        if (type == 9) {
            draw_raw_img24_to_framebuffer(0, 6, 112, 70, (u8_t*)gImage_m_card_jiangsu);
        } else {
            draw_img_to_framebuffer(RES_EXFLASH, (u8_t*)card_disp_bmp[type].img, \
                                0, 4, &w, &h, d_justify_center);
        }
    }
    

    //card title
    gdispFillStringBox( 0, 
                        88, 
                        RM_OLED_WIDTH,
                        font_roboto_20.height,
                        //(char*)card_disp_bmp[type].str, 
                        pa ? (char*)pa->name : (char*)pt->name,
                        (void*)font_roboto_20.font,
                        White, Black, 
                        justifyCenter
                        );

    if (activity_card_ctx_v->abnormal == 1) {
        gdispFillStringBox( 0, 190, RM_OLED_WIDTH, font_roboto_20.height,
                            (char*)text_card_select[4], (void*)font_roboto_20.font, Green, Black, justifyCenter);
        gdispFillStringBox( 0, 217, RM_OLED_WIDTH, font_roboto_20.height,
                            (char*)text_card_select[5], (void*)font_roboto_20.font, Green, Black, justifyCenter);
        return;
    }

    if (activity_card_ctx_v->busy == 1) {
        gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                            (char*)text_card_select[6], (void*)font_roboto_20.font, Green, Black, justifyCenter);
        return;
    }

    if (selected) {
        if (type == E_CARD_GATE) {
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 170, &w, &h, d_justify_center);
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[7], (void*)font_roboto_20.font, Green, Black, justifyCenter);
        } else {
            //img_exflash_to_framebuffer("icon_03_check.bmp", 0, 170, &w, &h, d_justify_center);
            if (pt->balance >= 0) {
                integer = pt->balance / 100;
                decimal = pt->balance % 100;
            }
            char balance_str1[10];
            char balance_str2[10];
            int width;
            int str1_pos, str2_pos;
            if (integer < 10 && integer >= 0) {
                sprintf((char*)balance_str1, "￥ ");
                sprintf((char*)balance_str2, "%d.%02d", integer, decimal);  
                width = getStringWidth(balance_str2);

            } else if (integer >= 10) {
                sprintf((char*)balance_str1, "￥ "); 
                sprintf((char*)balance_str2, "%02d.%02d", integer, decimal); 
                width = getStringWidth(balance_str2);
            } else if (integer < 0) {
                sprintf((char*)balance_str1, "￥ "); 
                sprintf((char*)balance_str2, "--.--"); 
                width = getStringWidth(balance_str2);
            }

            str1_pos = (120 - width)/2 - 11;
            str2_pos = str1_pos + 18;
            
            //gdispFillStringBox( 60, 120, 22, 22, (char*)"余额", NULL, White,  Black, justifyCenter);
            gdispFillStringBox( str1_pos, 120, 22, 22, (char*)balance_str1, NULL, White,  Black, justifyLeft);
            gdispFillStringBox( str2_pos, 124, 22, 22, (char*)balance_str2, NULL, White,  Black, justifyLeft);
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 170, &w, &h, d_justify_center);
            gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                                (char*)text_card_select[7], (void*)font_roboto_20.font, Green, Black, justifyCenter);
        }
        
    } else {
        //gdispFillStringBox( 60, 210, 22, 22, "点击刷卡 ", NULL, Green,  Black, justifyCenter);
        gdispFillStringBox( 0, 210, RM_OLED_WIDTH, font_roboto_20.height,
                            (char*)text_card_select[8], (void*)font_roboto_20.font, Green, Black, justifyCenter);
    }
     
}


void show_cards(void)
{
    int i, num;
    card_transit_t* pt;
    card_access_t* pa;
    int startY;
    int total = activity_card_ctx_v->transitCardTbl->curNum + activity_card_ctx_v->accessCardTbl->curNum;
    int delta;

    ryos_mutex_lock(activity_card_ctx_v->mutex);
    
    clear_buffer_black();
    
    /* Add Layout init here */
    if (total == 0) {
        /* No card here */

        gdispFillStringBox( 0, 
                            136, 
                            RM_OLED_WIDTH,
                            font_roboto_20.height,
                            (char*)text_add_at_phone[0], 
                            (void*)font_roboto_20.font, 
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        gdispFillStringBox( 0, 
                            164, 
                            RM_OLED_WIDTH,
                            font_roboto_20.height,
                            (char*)text_add_at_phone[1], 
                            (void*)font_roboto_20.font, 
                            HTML2COLOR(0xC4C4C4),
                            Black,
                            justifyCenter
                            ); 

        uint8_t w, h;
        draw_img_to_framebuffer(RES_EXFLASH, (uint8_t *)"m_no_alarm.bmp",\
                                0, 68, &w, &h, d_justify_center);
    } else {

        draw_card(activity_card_ctx_v->index);
    }

    if (total > 1) {
        delta = 200 / (total - 1);
        startY = activity_card_ctx_v->index * delta;
        scrollbar_show(activity_card_ctx_v->scrollbar, frame_buffer, startY);
    }

    ry_gui_msg_send(IPC_MSG_TYPE_GUI_INVALIDATE, 0, NULL);

    ryos_mutex_unlock(activity_card_ctx_v->mutex);
}


int card_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = ry_malloc(sizeof(card_create_evt_t));
    if (evt) {
        ry_memcpy(evt, msg->data, msg->len);

        wms_activity_jump(&activity_card_session, (void*)evt);
    }
    return 1;
}


void activity_card_selectTimeout_handler(void* arg)
{
    e_card_idx_t type;
    LOG_DEBUG("[activity_card_selectTimeout_handler] \r\n");
    type = get_current_card(activity_card_ctx_v->index);
    DEV_STATISTICS(card_change_count);

    if (activity_card_ctx_v->busy == 0) {
        LOG_DEBUG("[activity_card_selectTimeout_handler] index:%d\r\n", activity_card_ctx_v->index);
        activity_card_select(type);
    }

    activity_card_select_timer_stop();
    
}  


void activity_card_select_timer_start(void* arg)
{
    ry_timer_parm timer_para;
    
    LOG_DEBUG("[activity_card_select_timer_start]\r\n");
    
    if (activity_card_ctx_v->selectTimer == 0) {
        /* Create the timer once */
        timer_para.timeout_CB = activity_card_selectTimeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "acSelectTimer";
        activity_card_ctx_v->selectTimer = ry_timer_create(&timer_para);
    }
    
    ry_timer_start(activity_card_ctx_v->selectTimer, 700, activity_card_selectTimeout_handler, arg);
}


void activity_card_select_timer_stop(void)
{
    ry_sts_t status;
    LOG_DEBUG("[activity_card_select_timer_stop]\r\n");

    ry_timer_stop(activity_card_ctx_v->selectTimer);
    if (activity_card_ctx_v->selectTimer) {
        ry_timer_delete(activity_card_ctx_v->selectTimer);
        activity_card_ctx_v->selectTimer = 0;
    }
}


void activity_card_defaultCardTimeout_handler(void* arg)
{
    LOG_DEBUG("[activity_card_defaultCardTimeout_handler]\r\n");
    activity_card_defaultCard_timer_stop();
    cms_msg_send(CMS_CMD_SELECT_DEFAULT_CARD, 0, NULL);
}  


void activity_card_defaultCard_timer_start(void* arg)
{
    ry_timer_parm timer_para;
    
    LOG_DEBUG("[activity_card_defaultCard_timer_start]\r\n");
    
    if (activity_card_ctx_v->defaultCardTimer == 0) {
        /* Create the timer once */
        timer_para.timeout_CB = activity_card_defaultCardTimeout_handler;
        timer_para.flag = 0;
        timer_para.data = NULL;
        timer_para.tick = 1;
        timer_para.name = "defaultCardTimer";
        activity_card_ctx_v->defaultCardTimer = ry_timer_create(&timer_para);
    }
    
    ry_timer_start(activity_card_ctx_v->defaultCardTimer, 60000*5, activity_card_defaultCardTimeout_handler, arg);
}


void activity_card_defaultCard_timer_stop(void)
{
    ry_sts_t status;
    LOG_DEBUG("[activity_card_defaultCard_timer_stop]\r\n");

    ry_timer_stop(activity_card_ctx_v->defaultCardTimer);
    if (activity_card_ctx_v->defaultCardTimer) {
        ry_timer_delete(activity_card_ctx_v->defaultCardTimer);
        activity_card_ctx_v->defaultCardTimer = 0;
    }
}



/**
 * @brief   Entry of the Card activity
 *
 * @param   None
 *
 * @return  activity_card_onCreate result
 */
ry_sts_t activity_card_onCreate(void* usrdata)
{
    ry_sts_t ret = RY_SUCC;
    int i;
    card_transit_t* p_transit;
    card_access_t* p_access;
    e_card_idx_t type;
    
    
    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    /* Get card list */
    if (!activity_card_ctx_v) {
        activity_card_ctx_v = (activity_card_ctx_t*)ry_malloc(sizeof(activity_card_ctx_t));
        if (!activity_card_ctx_v) {
            LOG_ERROR("[activity_card_onCreate] No mem.\r\n");
            while(1);
        }

        ry_memset((u8_t*)activity_card_ctx_v, 0, sizeof(activity_card_ctx_t));

        ret = ryos_mutex_create(&activity_card_ctx_v->mutex);
        if (RY_SUCC != ret) {
            LOG_ERROR("[activity_card_onCreate]: Mutex Init Fail.\n");
            RY_ASSERT(ret == RY_SUCC);
        }
				
		activity_card_ctx_v->scrollbar = scrollbar_create(1, 40, 0x4A4A4A, 300);
        if (activity_card_ctx_v->scrollbar == NULL) {
            LOG_ERROR("[%s]: No scrollbar.\r\n", __FUNCTION__);
        }
    }
		
    activity_card_ctx_v->transitCardTbl = card_getTransitList();
    activity_card_ctx_v->accessCardTbl  = card_getAccessList();
    activity_card_ctx_v->index = find_selected_card();
    
    /* Subscribe events */
    LOG_DEBUG("[activity_card_onCreate]\r\n");
    wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card, card_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_CARD, &activity_card, card_onProcessedCardEvent);

    if (activity_card_ctx_v->defaultCardTimer) {
        activity_card_defaultCard_timer_stop();
    }

    show_cards();

    type = get_current_card(activity_card_ctx_v->index);
    if (activity_card_ctx_v->busy == 0) {

        /* We need to select the Gate card manually when card swiping mode is disable */
        if ((get_card_default_enable()) && (E_CARD_GATE == type)) {
            return ret;
        }
        
        activity_card_select(type);
    }


    return ret;
}

/**
 * @brief   API to exit Card activity
 *
 * @param   None
 *
 * @return  card activity Destrory result
 */
ry_sts_t activity_card_onDestrory(void)
{
    /* Release activity related dynamic resources */
    wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_card);
    wms_event_listener_del(WM_EVENT_CARD, &activity_card);

    activity_card_ctx_v->busy = 0;

#if 1
    if ((activity_card_ctx_v->accessCardTbl->curNum + activity_card_ctx_v->transitCardTbl->curNum > 0) &&
        (get_device_session() != DEV_SESSION_CARD)){
        //LOG_INFO("[%s] Enter NFC Low power.\r\n", __FUNCTION__);

        /* Close SE when card_swiping mode is OFF */
        if (get_card_default_enable() == 0) {
            ry_nfc_sync_poweroff();
            return RY_SUCC;
        }

        if (get_hardwardVersion()>3) {
            if (card_is_current_disable_lowpower() == FALSE) {
                ry_nfc_sync_lowpower();
            }
        }

        /* If the default card mode is enable, select that after 5 min. */
        if ((card_get_defalut_mode() == CMS_DEFAULT_CARD_MODE_SPECIFIC) &&
            (card_check_is_default() == FALSE)) {
            activity_card_defaultCard_timer_start(NULL);
        }
    }
#endif

    return RY_SUCC;
    
}


int card_session_timer;

void card_session_ui_timeout_handler(void* arg)
{
    wms_activity_back(NULL);
}    


void card_session_ui_timer_start(int timeout)
{
    ry_timer_start(card_session_timer, timeout, card_session_ui_timeout_handler, NULL);
}

void card_session_ui_timer_stop(void)
{
    ry_timer_stop(card_session_timer);
}

void show_card_session_ui(u8_t type, u8_t state, u8_t code)
{
    ry_sts_t ret = RY_SUCC;
    uint8_t w, h;
    int timeout = 30000;
    char* str1;
    char* str2;
    char* str3;

    card_session_ui_timer_stop();
    clear_buffer_black();

    switch (type) {
        case CARD_ID_GATE:
            img_exflash_to_framebuffer((u8_t*)"m_card_05_gate0.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_BJT:
            img_exflash_to_framebuffer((u8_t*)"m_card_02_beijing.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_SZT:
            img_exflash_to_framebuffer((u8_t*)"m_card_01_shenzhen.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_LNT:
            img_exflash_to_framebuffer((u8_t*)"m_card_00_guangzhou.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_JJJ:
            img_exflash_to_framebuffer((u8_t*)"m_card_02_beijing.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_SHT:
            img_exflash_to_framebuffer((u8_t*)"m_card_04_shanghai.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_WHT:
            img_exflash_to_framebuffer((u8_t*)"m_card_wuhan.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_YCT:
            img_exflash_to_framebuffer((u8_t*)"m_card_08_yangcheng.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_CQT:
            img_exflash_to_framebuffer((u8_t*)"m_card_chongqing.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_JST:
            draw_raw_img24_to_framebuffer(0, 6, 112, 70, (u8_t*)gImage_m_card_jiangsu);
            //img_exflash_to_framebuffer((u8_t*)"m_card_suzhou.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_CAT:
            img_exflash_to_framebuffer((u8_t*)"m_card_changan.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_HFT:
            img_exflash_to_framebuffer((u8_t*)"m_card_hefei.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_GXT:
            img_exflash_to_framebuffer((u8_t*)"m_card_guangxi.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        case CARD_ID_JLT:
            img_exflash_to_framebuffer((u8_t*)"m_card_jiling.bmp", 0, 0, &w, &h, d_justify_center);
            break;

        default:
            break;
    }

    if (state == CARD_CREATE_EVT_STATE_DETECT) {
        //card title
        if (code == CARD_CREATE_EVT_CODE_START) {
            // Start to detect
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_create[0], NULL, White, Black, justifyCenter); 
            gdispFillStringBox( 0, 178, 22, 22, (char*)text_card_create[1], NULL, Green, Black, justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_create[2], NULL, Green, Black, justifyCenter); 
        } else if (code == CARD_CREATE_EVT_CODE_FAIL) {
            // Detect fail
            motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 200);
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_create[3], NULL, White, Black, justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_create[4], NULL, Blue, Black, justifyCenter); 
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
        } else if (code == CARD_CREATE_EVT_CODE_SUCC) {
            // Detect succ
            motar_set_work(MOTAR_TYPE_STRONG_LINEAR, 200);
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_create[5], NULL, White, Black, justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_create[6], NULL, Green, Black, justifyCenter); 
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
        }
        
    } else if (state == CARD_CREATE_EVT_STATE_COPY) {
        if (code == CARD_CREATE_EVT_CODE_START) {
            touch_bypass_set(1);
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_copy[0], NULL, White, Black, justifyCenter);  
        }

        if (code == CARD_CREATE_EVT_CODE_START) {
            // Start to Copy
            touch_bypass_set(1);
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_copy[1], NULL, White, Black, justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_copy[2], NULL, Green, Black, justifyCenter); 
            wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_card_session);
        } else if (code == CARD_CREATE_EVT_CODE_FAIL) {
            // Copy fail
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_copy[3], NULL, White, Black, justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_copy[4], NULL, Blue, Black, justifyCenter); 
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
            touch_bypass_set(0);
        } else if (code == CARD_CREATE_EVT_CODE_SUCC) {
            // Copy succ
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_copy[5], NULL, White, Black, justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_copy[6], NULL, Blue, Black, justifyCenter); 
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 200, &w, &h, d_justify_center);
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
            touch_bypass_set(0);
        }
    } else if (state == CARD_CREATE_EVT_STATE_USER_SETTING) {
        if (code == CARD_CREATE_EVT_CODE_START) {
            gdispFillStringBox( 0, 88, 22,22, (char*)"信息同步中 ", NULL, White,  Black,justifyCenter);  
        }
            
        img_exflash_to_framebuffer("icon_03_check.bmp", 0, 200, &w, &h, d_justify_center);

    } else if (state == CARD_CREATE_EVT_STATE_TRANSIT) {
        if (code == CARD_CREATE_EVT_CODE_START) {
            touch_bypass_set(1);
            gdispFillStringBox( 0, 88, 22,22, (char*)"开卡中", NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)"请稍后 ", NULL, Green,  Black, justifyCenter); 
            timeout = 300*1000;
        } else if (code == CARD_CREATE_EVT_CODE_FAIL) {
            gdispFillStringBox( 0, 88, 22,22, (char*)"开卡失败 ", NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)"请重试 ", NULL, Blue,  Black, justifyCenter); 
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
            touch_bypass_set(0);
        } else if (code == CARD_CREATE_EVT_CODE_SUCC) {
            gdispFillStringBox( 0, 88, 22,22, (char*)"开卡成功 ", NULL, White,  Black,justifyCenter); 
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 200, &w, &h, d_justify_center);
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
            touch_bypass_set(0);
        }
        
    } else if (state == CARD_DELETE_EVT_STATE_START) {
        if (code == CARD_CREATE_EVT_CODE_START) {
            touch_bypass_set(1);
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_delete[0], NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_delete[1], NULL, Green,  Black, justifyCenter); 
            timeout = 300*1000;
        } else if (code == CARD_CREATE_EVT_CODE_FAIL) {
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_delete[2], NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_delete[3], NULL, Blue,  Black, justifyCenter); 
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
            touch_bypass_set(0);
        } else if (code == CARD_CREATE_EVT_CODE_SUCC) {
            gdispFillStringBox( 0, 88, 22,22, (char*)text_card_delete[4], NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)text_card_delete[5], NULL, Blue,  Black, justifyCenter); 
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 200, &w, &h, d_justify_center);
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
            touch_bypass_set(0);
        }
    } else if (state == CARD_RECHARGE_EVT) {
        if (code == CARD_CREATE_EVT_CODE_START) {
            gdispFillStringBox( 0, 88, 22,22, (char*)"充值中 ", NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)"请稍后 ", NULL, Green,  Black, justifyCenter); 
            timeout = 300*1000;
        } else if (code == CARD_CREATE_EVT_CODE_FAIL) {
            gdispFillStringBox( 0, 88, 22,22, (char*)"充值失败 ", NULL, White,  Black,justifyCenter); 
            gdispFillStringBox( 0, 210, 22, 22, (char*)"请重试 ", NULL, Blue,  Black, justifyCenter); 
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
        } else if (code == CARD_CREATE_EVT_CODE_SUCC) {
            gdispFillStringBox( 0, 88, 22,22, (char*)"充值成功 ", NULL, White,  Black,justifyCenter); 
            img_exflash_to_framebuffer("icon_03_check.bmp", 0, 200, &w, &h, d_justify_center);
            wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
        }
    }
             
    ry_gui_msg_send(IPC_MSG_TYPE_GUI_SCREEN_ON, 0, NULL);

    card_session_ui_timer_start(timeout);
}


int card_session_onProcessedCardEvent(ry_ipc_msg_t* msg)
{
    card_create_evt_t* evt = (card_create_evt_t*)(msg->data);
    u8_t cardType = evt->cardType;
    u8_t code  = evt->code;
    u8_t state = evt->state;

    LOG_DEBUG("[card_session_onProcessedCardEvent] On Processed Card. %d, %d, %d\r\n",
        cardType, state, code);

    show_card_session_ui(cardType, state, code);
    
    return 1;
}

int card_session_onBLEEvent(ry_ipc_msg_t* msg)
{
    if (msg->msgType == IPC_MSG_TYPE_BLE_DISCONNECTED) {
        card_onBleDisconnected();
    }
    
    return 1;
}

int card_session_onProcessedTouchEvent(ry_ipc_msg_t* msg)
{
    int consumed = 1;
    tp_processed_event_t *p = (tp_processed_event_t*)msg->data;
    switch (p->action) {
        case TP_PROCESSED_ACTION_TAP:
            LOG_DEBUG("[card_session_onProcessedTouchEvent] TP_Action Click, y:%d\r\n", p->y);
            if ((p->y >= 8)){
                wms_activity_back(NULL);
                return consumed;
            }
            break;


        default:
            break;
    }
    return consumed; 
}

void show_card(void)
{
    int aNum, tNum;
    aNum = card_getAccessCardNum();
    tNum = card_getTransitCardNum();

    if (1 || aNum + tNum == 0) {
        gdispFillStringBox(0, 93, font_roboto_20.width, font_roboto_20.height,
                            (char*)text_add_at_phone[0], (void*)font_roboto_20.font, 
                            White, Black, justifyCenter);

        gdispFillStringBox(0, 125, font_roboto_20.width, font_roboto_20.height,
                            (char*)text_add_at_phone[1], (void*)font_roboto_20.font, 
                            White, Black, justifyCenter); 
    }
}

/**
 * @brief   Entry of the Card activity
 *
 * @param   None
 *
 * @return  activity_card_onCreate result
 */
ry_sts_t activity_card_session_onCreate(void* usrdata)
{
    if (usrdata == NULL) {
        wms_activity_back(NULL);
        return RY_SUCC;
    }
    
    card_create_evt_t* evt = (card_create_evt_t*)usrdata;
    u8_t cardType = evt->cardType;
    u8_t code  = evt->code;
    u8_t state = evt->state;
    
    if (usrdata) {
        // Get data from usrdata and must release the buffer.
        ry_free(usrdata);
    }

    wms_event_listener_add(WM_EVENT_CARD, &activity_card_session, card_session_onProcessedCardEvent);
    //wms_event_listener_add(WM_EVENT_TOUCH_PROCESSED, &activity_card_session, card_session_onProcessedTouchEvent);
    wms_event_listener_add(WM_EVENT_BLE, &activity_card_session, card_session_onBLEEvent);

    /* Create the timer */
    ry_timer_parm timer_para;
    timer_para.timeout_CB = card_session_ui_timeout_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "card_sess_timer";
    card_session_timer = ry_timer_create(&timer_para);

    show_card_session_ui(cardType, state, code);
    
    return RY_SUCC;
}


/**
 * @brief   API to exit Card activity
 *
 * @param   None
 *
 * @return  card activity Destrory result
 */
ry_sts_t activity_card_session_onDestrory(void)
{
    wms_event_listener_del(WM_EVENT_CARD, &activity_card_session);
    //wms_event_listener_del(WM_EVENT_TOUCH_PROCESSED, &activity_card_session);
    wms_event_listener_del(WM_EVENT_BLE, &activity_card_session);

    /* Recovery NFC state */
    card_nfc_recover();
    
    if (card_session_timer) {
        ry_timer_stop(card_session_timer);
        ry_timer_delete(card_session_timer);
        card_session_timer = 0;
    }

    return RY_SUCC;   
}
