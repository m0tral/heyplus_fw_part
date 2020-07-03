/*
* Copyright (c), NXP Semiconductors
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
* NXP reserves the right to make changes without notice at any time.
*
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
* particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
* arising from its use.
*/

// QN902x configuration

#include "usr_config.h"
#include "uart_int_dma.h"
#include "qn_config.h"
#include "app_env.h"

#if (defined(DTM_TEST_ENABLE))
static void dtm_test(bool enable)                          // DTM test
{
  uint8_t dtm_enable  = 1;
  uint8_t dtm_disable = 0;

    Chip_GPIO_SetPinDIRInput(LPC_GPIO, CFG_TEST_CTRL_PORT, CFG_TEST_CTRL_PIN);

    tx_done = 0;
    if (enable && (!Chip_GPIO_GetPinState(LPC_GPIO, CFG_TEST_CTRL_PORT, CFG_TEST_CTRL_PIN)))
    {
        phOsalNfc_Log("Enable DTM\r\n");
        Chip_UART_SendBlocking(BLE_UART_PORT, &dtm_enable, 1);
        tx_done = 1;
    }
    else
    {
        phOsalNfc_Log("Disable DTM\r\n");

        Chip_UART_SendBlocking(BLE_UART_PORT, &dtm_disable, 1);
        tx_done = 1;
    }
    if (!CheckTxTimeOut(10))                               // Wait until tx done
        phOsalNfc_Log("DTM test timeout\r\n");
}	
#endif


void nvds_config(void)
{
		uint8_t drift = 100;
		uint16_t twext=1000, twosc=1000;
		uint8_t xcsel=0x11; 
		uint8_t buf[44];
		uint8_t tmpLen;
																
		memcpy(buf, BD_address, 6);
		/*
		 * Fix for CID 10878
		 */
		if (sizeof(DeviceName) <= 32)
		{
			tmpLen = sizeof(DeviceName);
		}
		else
		{
		    tmpLen = 32;
		}
		memcpy(&buf[6], DeviceName, tmpLen);
		buf[38] = drift;
		buf[39] =	twext/256;	
		buf[40] =	twext%256;		
		buf[41] =	twosc/256;	
		buf[42] =	twosc%256;	
		buf[43] =	xcsel;															
		app_eaci_cmd_nvds(buf);	
		DEBUGSTR("nvds_config.\r\n");	
}

