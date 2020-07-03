/**
  ******************************************************************************
  * @file    system_win.c
  * @author  MCD Application Team   
  * @brief   System information functions
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
#include "k_module.h"
#include "k_menu.h"
#include "m_iddmeasure.h"
#include "m_iddmeasure.c"

/* Private function prototypes -----------------------------------------------*/
KMODULE_RETURN  AppMainPreExec(void);
KMODULE_RETURN  AppMainExec(void);
static void     AppMain_Option(void);
static void     AppMain_About(void);
KMODULE_RETURN  AppMainExecCheckResource(void);

/* Private Variable ----------------------------------------------------------*/
/* Option menu */
const tMenuItem OptionMenuItems[] =
{
  {"LPMODE", 0, 0,   SEL_EXEC, 0, AppMain_Option, NULL, NULL, NULL},
  {" ABOUT", 0, 0,   SEL_EXEC, 0, AppMain_About,  NULL, NULL, NULL},
  {""      , 0, 0,   SEL_EXIT, 0, NULL,           NULL, NULL, NULL}
};

const tMenu OptionMenu = {
  "main_sub_menu", OptionMenuItems, countof(OptionMenuItems), TYPE_TEXT, 1, 0
};

/* Main Menu */
const tMenuItem ConstMainMenuItems[] =
{
  {" IDD_0", 0, 0, SEL_MODULE,  MODULE_IDDMEASURE   , NULL, NULL, NULL, NULL},
  {" IDD_1", 0, 0, SEL_MODULE,  MODULE_IDDMEASURE   , NULL, NULL, NULL, NULL},
  {"RECORD", 0, 0, SEL_MODULE,  MODULE_AUDIORECORDER, NULL, NULL, NULL, NULL},
  {"PLAYER", 0, 0, SEL_MODULE,  MODULE_AUDIOPLAYER  , NULL, NULL, NULL, NULL},
  {"COMPAS", 0, 0, SEL_MODULE,  MODULE_COMPASS      , NULL, NULL, NULL, NULL},
  {" SOUND", 0, 0, SEL_MODULE,  MODULE_SOUNDMETER   , NULL, NULL, NULL, NULL},
  {"GUITAR", 0, 0, SEL_MODULE,  MODULE_GUITARTUNER  , NULL, NULL, NULL, NULL},
  {"OPTION", 0, 0, SEL_SUBMENU, MODULE_NONE         , NULL, NULL, (const tMenu*)&OptionMenu, NULL },
  {""      , 0, 0, SEL_NOTHING, MODULE_NONE         , NULL, NULL, NULL, NULL}
};

tMenuItem MainMenuItems[countof(ConstMainMenuItems)] =
{ 0 };

uint32_t MainMenuItemsNb = 0;

tMenu MainMenu = {
  "main_menu", MainMenuItems, 0, TYPE_TEXT, 1, 0
};

/* Private typedef -----------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
const K_ModuleItem_Typedef ModuleAppMain =
{
  MODULE_MAIN_APP,
  AppMainPreExec,
  AppMainExec,
  NULL,
  AppMainExecCheckResource
};  

/**
  * @brief  Prepare main menu 
  * @param  None.
  * @note   compute main menu.  
  * @retval None.
  */
KMODULE_RETURN AppMainPreExec(void)
{
  uint8_t i =0;

  LOG_DEBUG("[k_app_main]: AppMainPreExec...\r\n");          

  for(i = 0; i < countof(ConstMainMenuItems); i++)
  {
    if((ConstMainMenuItems[i].ModuleId != MODULE_NONE) &&
      (kmodule_info[ConstMainMenuItems[i].ModuleId].kModuleId != MODULE_MAIN_APP))
    {
      LOG_DEBUG("[k_app_main]: %d add MainMenuItems...\r\n", i);          
      MainMenuItems[MainMenuItemsNb] = ConstMainMenuItems[i];
      MainMenuItemsNb++;
    }
  }
  MainMenuItems[MainMenuItemsNb] = ConstMainMenuItems[7];
  MainMenuItemsNb++;
  MainMenuItems[MainMenuItemsNb] = ConstMainMenuItems[8];
  MainMenuItemsNb++;
  MainMenu.nItems = MainMenuItemsNb;

  for(i = 0; i < MODULE_MAX; i++)
  {
    if(kmodule_info[i].kModuleId != MODULE_MAIN_APP)
    {
      if(kmodule_info[i].kModulePreExec != NULL)
      {
        kmodule_info[i].kModulePreExec();
      }
      if(kmodule_info[i].kModulePostExec != NULL)
      {
        kmodule_info[i].kModulePostExec();
      }
    }
  }

  /* Reset low power variable & notify Demo is started */
  // DemoEnterLowPower = RESET;
  // DemoStarted = SET;

  return KMODULE_OK;
}

/**
  * @brief  Run the main application 
  * @param  None.
  * @note   run and display main menu.  
  * @retval None.
  */
KMODULE_RETURN AppMainExec(void)
{
  static uint32_t cnt;
  LOG_DEBUG("[k_app_main]: AppMainExec...\r\n", cnt++);            
  kMenu_Execute(MainMenu);
  return KMODULE_OK;
}

/**
  * @brief  Run the option menu information 
  * @param  None.
  * @note   display About information.  
  * @retval None.
  */
static void AppMain_Option(void)
{
  uint8_t index = 0;
  uint8_t max_index = 1;
  JOYState_TypeDef key = JOY_DOWN;

  do
  {  
    key = kMenu_GetEvent(10);
    
    switch(key)
    {
    case JOY_DOWN:
      index++;
      if(index > max_index)
      {
        index = 0;
      }
      break;
      
    case JOY_UP:
      if(index == 0)
      {
        index = max_index;
      }
      else
      {
        index--;
      }
      break;
      
    default:
      break;
    }
    
  } while((key != JOY_SEL) && (key != JOY_RIGHT) && (key != JOY_LEFT));
}

/**
  * @brief  Run the About information 
  * @param  None.
  * @note   display About information.  
  * @retval None.
  */
static void AppMain_About(void)
{

}

/**
  * @brief  Check the main application resources.
  * @param  None.
  * @note   None.  
  * @retval None.
  */
KMODULE_RETURN AppMainExecCheckResource(void)
{
  return KMODULE_OK;
}

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

