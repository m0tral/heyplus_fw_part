/**
  ******************************************************************************
  * @file    k_menu.c
  * @author  MCD Application Team   
  * @brief   This file provides the kernel menu functions 
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/
#include "k_config.h"
#include "k_menu.h"
#include "k_module.h"

/** @addtogroup CORE
  * @{
  */

/** @defgroup KERNEL_MENU
  * @brief Kernel menu routines
  * @{
  */

/* External variables --------------------------------------------------------*/
// extern MODULES_INFO       DemoSelectedModuleId;

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
enum {
  KMENU_HEADER,
  KMENU_TEXT,
  KMENU_EXEC,
  KMENU_WAITEVENT,
  KMENU_EXIT
};
  
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Variable used to forward an user event to an application */
static tExecAction kMenuEventForward = NULL;
// osMessageQId JoyEvent = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static void kMenu_HandleSelection(tMenu MainMenu, uint8_t *sel);

/**
  * @brief  Function to initialize the module menu
  * @retval None
  */
void kMenu_Init(void)
{
  /* Create the Push Button event handler */
  // osMessageQDef(JoyQueue, 1, uint16_t);
  // JoyEvent = osMessageCreate(osMessageQ(JoyQueue), NULL);
}

/**
  * @brief  Function in charge to handle the menu selection
  * @param  menu: menu structure
  * @param  sel: selected item
  * @retval None
  */
void kMenu_HandleSelection(tMenu MainMenu, uint8_t *sel)
{
  uint8_t exit = 0;
  tMenu psCurrentMenu = MainMenu;
  JOYState_TypeDef joystate = JOY_NONE;
  static uint32_t cnt;
  // LOG_DEBUG("[kMenu_HandleSelection]: entry... %d\r\n", cnt++);            
  switch(psCurrentMenu.nType)
  {
  case TYPE_TEXT :
    {
      do
      {
        LOG_DEBUG("[kMenu_HandleSelection]: loop... %d\r\n", cnt++);            
        /* Clear the LCD GLASS */
        //BSP_LCD_GLASS_Clear();
        
        /* Get the current menu */
        // BSP_LCD_GLASS_DisplayString((uint8_t *)psCurrentMenu.psItems[*sel].pszTitle);
        uint32_t DemoMinIdleTime = 0;                    /* Inactivity timing value   */
        joystate = kMenu_GetEvent(DemoMinIdleTime);
        
        LOG_DEBUG("[kMenu_HandleSelection]: got tp cmd... %d\r\n", joystate);    
        switch(joystate)
        {
          case TP_CODE_0 :  //0x01
            (*sel)++;
            /* check if sel is on the latest line position */
            if(*sel >= (psCurrentMenu.nItems - 1)){
              *sel = 0;
            }
            exit = 1;
            break;

          default:   //only response the top tp value
            break;

          case TP_CODE_NONE :
            //DemoEnterLowPower = SET;
            break;

          case TP_CODE_4 :   //0x10
            *sel = (psCurrentMenu.nItems - 1);
            exit = 1;
            break;

          case TP_CODE_1 :  //0x02
            // break;

          case TP_CODE_3 :  //0x08
            break;

          case TP_CODE_2:  //0x04
            /** check if sel is on the first line */
            if ((*sel) == 0){
              *sel = (psCurrentMenu.nItems - 2);
            }
            else{
              (*sel)--;
            }
            exit = 1;
            break;
        }
        LOG_DEBUG("[kMenu_HandleSelection]: menu_items = %d, menu_next_cmd:%d, sel:%d exit:%d\r\n", psCurrentMenu.nItems, joystate, *sel, exit);    
      }while(exit == 0);
    }
    break;
  }
  
  return;
}

/**
  * @brief  Function in charge to execute a menu entry point 
  * @param  menu 
  * @retval None
  */
void kMenu_Execute(tMenu psCurrentMenu) 
{
  uint32_t exit = 1;
  uint32_t k_MenuState = KMENU_HEADER;
  uint8_t sel = 0;
  MODULES_INFO prevModuleId = MODULE_NONE;
  extern MODULES_INFO DemoSelectedModuleId;
  static uint32_t cnt;
  do 
  {  
    LOG_DEBUG("[kMenu_Execute]: kMenu_Execute, nenu_state:%d, cnt: %d\r\n", k_MenuState, cnt++);            
    switch(k_MenuState)
    {
    case KMENU_HEADER :
      {
        LOG_DEBUG("[kMenu_Execute]: module id = %d, kMenu_Execute, nenu_state:%d = KMENU_HEADER, cnt: %d\r\n", \
          DemoSelectedModuleId, k_MenuState, cnt);            
        if(psCurrentMenu.pszTitle != NULL){
         // BSP_LCD_GLASS_ScrollSentence((uint8_t *)psCurrentMenu.pszTitle, 1 , SCROLL_SPEED_HIGH);
        }
        
        switch(psCurrentMenu.nType)
        {
          case TYPE_TEXT :
            k_MenuState = KMENU_TEXT;
            break;
          case TYPE_EXEC :
            k_MenuState = KMENU_EXEC;
            break;
          default :
            k_MenuState = KMENU_EXIT;
            break;
        }
      }
      break;

    case KMENU_TEXT :
      {
        LOG_DEBUG("[kMenu_Execute]: kMenu_Execute, nenu_state:%d = KMENU_TEXT, cnt: %d\r\n", k_MenuState, cnt);            
        k_MenuState = KMENU_WAITEVENT;
      }
      break;

    case KMENU_EXEC :
      {
        LOG_DEBUG("[kMenu_Execute]: kMenu_Execute, nenu_state:%d = KMENU_EXEC, cnt: %d\r\n", k_MenuState, cnt);            
        /* if the function need user feedback, set callback function */
        if(psCurrentMenu.psItems[0].pfActionFunc != NULL)
        {
          /* set the function to report joystick event */
          kMenuEventForward = psCurrentMenu.psItems[0].pfActionFunc;
        }
        
        kMenu_Header(psCurrentMenu.psItems[0].pszTitle);
        
        /* execute the menu execution function */
        psCurrentMenu.psItems[0].pfExecFunc();
        
        /* reset user feedback, in polling mode */
        if(psCurrentMenu.psItems[0].pfActionFunc != NULL)
        {
          /* set the function to report to NULL */
          kMenuEventForward = NULL;
        }
        k_MenuState = KMENU_EXIT;
      }
      break;

    case KMENU_WAITEVENT:
      {
        LOG_DEBUG("[kMenu_Execute]: kMenu_Execute, nenu_state:%d = KMENU_WAITEVENT, psCurrentMenu = %d, cnt: %d\r\n", k_MenuState, sel, cnt);            
        kMenu_HandleSelection(psCurrentMenu, &sel);
        LOG_DEBUG("[kMenu_Execute]: new psCurrentMenu sel:%d, selType:%d titil:%s \r\n", \
          sel, psCurrentMenu.psItems[sel].SelType, psCurrentMenu.psItems[sel].pszTitle);

        /* The user has selected an execution menu */
        switch(psCurrentMenu.psItems[sel].SelType)
        {
          case SEL_MODULE:
            LOG_DEBUG("[kMenu_Execute]: a new SEL_MODULE %d\r\n", psCurrentMenu.psItems[sel].ModuleId);            
            /* save current Module Id */
            prevModuleId = DemoSelectedModuleId;
            DemoSelectedModuleId = (MODULES_INFO) psCurrentMenu.psItems[sel].ModuleId;
            /* start the module execution */
            kModule_Execute(psCurrentMenu.psItems[sel].ModuleId);
            /* restore Module Id */
            DemoSelectedModuleId = prevModuleId;  //只保存上一次的moduleID
            k_MenuState = KMENU_HEADER;
            break;

          case SEL_EXEC :
            /* if the function need user feedback, and set callback function */
            if(psCurrentMenu.psItems[sel].pfActionFunc != NULL){
              /* set the function to report joystick event */
              kMenuEventForward = psCurrentMenu.psItems[sel].pfActionFunc;
            }
            /* start the function execution */
            psCurrentMenu.psItems[sel].pfExecFunc();
            
            /* rest user feedback, in polling mode */
            if(psCurrentMenu.psItems[sel].pfActionFunc != NULL){
              /* set the function to report to NULL */
              kMenuEventForward = NULL;
            }
            k_MenuState = KMENU_HEADER;
            break;

          case SEL_SUBMENU :
            /* Select submenu or return on the main module menu */
            kMenu_Execute(*(psCurrentMenu.psItems[sel].psSubMenu));
            k_MenuState = KMENU_HEADER;
            break;

          case SEL_MODULE_SUB :
            /* Select submenu or return on the main module menu */
            // kMenu_Execute(*(psCurrentMenu.psItems[sel].psSubMenu));
            kModule_Execute(psCurrentMenu.psItems[sel].ModuleId);
            k_MenuState = KMENU_HEADER;
            break;

          case SEL_EXIT :
            /* back to main application level */
            //DemoSelectedModuleId = MODULE_MAIN_APP;
            k_MenuState = KMENU_EXIT;
            break;

          case SEL_NOTHING:
            /* to avoid exit of main application */
            k_MenuState = KMENU_WAITEVENT;
            sel = 0;
            break;
        } // switch(psCurrentMenu.psItems[sel].SelType)
      }
      break;

    case KMENU_EXIT :
      LOG_DEBUG("[kMenu_Execute]: kMenu_Execute, nenu_state:%d = KMENU_EXIT, psCurrentMenu %d, cnt: %d\r\n", k_MenuState, sel, cnt);            
      exit = 0;
      break;
      
    }
  }while(exit);
}


/**
  * @brief  Function to display header information 
  * @param  menu 
  * @retval None
  */
void kMenu_Header(char *string)
{
  if(string != NULL);
  // BSP_LCD_GLASS_DisplayString((uint8_t *)string);
}


#define JOYSTICK_PIN                     1
#define RIGHT_JOY_PIN                    2
#define LEFT_JOY_PIN                     3
#define UP_JOY_PIN                       4
#define DOWN_JOY_PIN                     5
#define SEL_JOY_PIN                      6
/**
  * @brief  Function in charge to handle user event and forward them to the module
  * @param  GPIO_Pin
  * @retval None
  */
void kMenu_EventHandler(uint16_t GPIO_Pin)
{
  if(kMenuEventForward != NULL)
  {
    switch(GPIO_Pin)
    {
    case DOWN_JOY_PIN :
      (kMenuEventForward)(JOY_DOWN);
      break;
    case UP_JOY_PIN :
      (kMenuEventForward)(JOY_UP);
      break;
    case SEL_JOY_PIN :
      (kMenuEventForward)(JOY_SEL);
      break;
    case RIGHT_JOY_PIN :
      (kMenuEventForward)(JOY_RIGHT);
      break;
    case LEFT_JOY_PIN :
      (kMenuEventForward)(JOY_LEFT);
      break;
    }
  }
}

/**
  * @brief  Function in charge to handle user event and send them to event queue
  * @param  GPIO_Pin
  * @retval None
  */
void kMenu_SendEvent(uint16_t GPIO_Pin)
{
  JOYState_TypeDef joystate = JOY_NONE;

  switch(GPIO_Pin)
  {

  }
  /* Send Message to event handler */
  // osMessagePut(JoyEvent, joystate, 0);
}

/**
  * @brief  Function in charge to wait for user event
  * @param  Delay   Delay to wait event for
  * @retval Joystick key event
  */
JOYState_TypeDef kMenu_GetEvent(uint32_t Delay)
{
  touch_data_t msg;
  static uint32_t cnt;
  // LOG_DEBUG("[kMenu_GetEvent]: kMenu_GetEvent waiting... %d\r\n", cnt++);     
  if (RY_SUCC == ryos_queue_recv(ry_queue_evt_app, &msg, RY_OS_WAIT_FOREVER)) {    
      k_page_execute();
      update_buffer_to_oled();
      return msg.button;
  } else {
      LOG_ERROR("[kMenu_GetEvent]:Queue receive error.\r\n");
      return JOY_NONE;    
  }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
