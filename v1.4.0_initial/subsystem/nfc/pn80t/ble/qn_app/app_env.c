/**
 ****************************************************************************************
 *
 * @file app_env.c
 *
 * @brief Application Environment C file
 *
 * Copyright(C) 2015 NXP Semiconductors N.V.
 * All rights reserved.
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct app_env_tag app_env;

static void app_env_init(void)
{
	uint8_t idx;
    memset(&app_env, 0, sizeof(app_env));
    for (idx = 0; idx < BLE_CONNECTION_MAX; idx++)
    {
        app_env.dev_rec[idx].free = true;
    }
}

/**
 ****************************************************************************************
 * @brief Initialize the application environment.
 *
 ****************************************************************************************
 */
void QN_app_init(void)
{
    app_env_init();
//  app_debug_init();

#if (GDI_GUP_ENABLE == FALSE)
    app_eaci_init();
#endif
    
//  app_env.menu_id = menu_main;
//  app_menu_hdl();
}
