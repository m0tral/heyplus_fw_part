/*
* Copyright (c), Ryeex Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "ry_mcu_config.h"

/* Board */
#include "board.h"
#include "ry_hal_inc.h"

#include "ry_gsensor.h"
#include "ry_gui.h"
#include "am_hal_sysctrl.h"

/* Subsystem API */
#include "hrm.h"
#include "gui.h"
#include "sensor_alg.h"
#include "app_interface.h"
#include "testbench.h"
#include "ry_ble.h"
#include "ry_tool.h"
#include "ry_fs.h"

/* Application Specific */

#include "k_config.h"
#include "k_module.h"
#include "k_menu.h"

#include "k_module.c"
#include "k_menu.c"
#include "main_app.c"

/*********************************************************************
 * CONSTANTS
 */ 


 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * VARIABLES
 */
MODULES_INFO DemoSelectedModuleId = MODULE_NONE;

void kModule_Init(void)
{
  kModule_Add(MODULE_MAIN_APP, ModuleAppMain);
  kModule_Add(MODULE_IDDMEASURE, ModuleIddMeasure);
  kModule_Add(MODULE_BATTERYMEASURE, ModuleBatteryMeasure);  //sub module
}


void k_app_entry(void)
{
  /* Module Initialization */
  kModule_Init();
  LOG_DEBUG("[k_app_entry]: kModule_Init.\r\n");
  
  
  /* Control the resources */
  if(kModule_CheckResource() != KMODULE_OK){
    Error_Handler();
  }
  kMenu_Init();
  LOG_DEBUG("[k_app_entry]: kMenu_Init.\r\n");  
}

void k_app_unInitialization(void)
{
  /* Nothing to do */
}

/* [] END OF FILE */
