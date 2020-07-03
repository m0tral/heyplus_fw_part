/**
 ****************************************************************************************
 *
 * @file eaci.c
 *
 * @brief Easy ACI interface module source file.
 *
 * Copyright(C) 2015 NXP Semiconductors N.V.
 * All rights reserved.
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "app_env.h"
#include "wakeup.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// EACI environment context
struct eaci_env_tag eaci_env;

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize EACI interface
 *
 ****************************************************************************************
 */
void app_eaci_init(void)
{   
    uartrom_regcb_eaci();	                   // Register call-back functions for eaci uart
    eaci_uart_read_start();                    // start uart reception
}

/**
 ****************************************************************************************
 * @brief EACI application message handler
 *
 ****************************************************************************************
 */
void app_eaci_msg_hdl(uint8_t msg_type, uint8_t msg_id, uint8_t param_len, uint8_t const *param, void* context)
{  

   switch (msg_type)
   {
      case EACI_MSG_TYPE_EVT:
          app_eaci_evt(msg_id, param_len, param);
          break;
      case EACI_MSG_TYPE_DATA_IND:
    	  app_eaci_data_ind(msg_id, param_len, param, context);
    	  break;
      case EACI_MSG_TYPE_DATA_ERROR:
    	  app_eaci_data_error_rsp(msg_id, param_len, param);
    	  break;
      case EACI_MSG_TYPE_CMD:
      case EACI_MSG_TYPE_DATA_REQ:
      default:
    	  {
    		  phOsalNfc_LogBle("Message type unknown in app_eaci_msg_hdl \n");
    		  phOsalNfc_LogBle("Message type is %d \n", msg_type);
    		  break;
    	  }
    }
}
/// @} EACI
