/*
 * Copyright (C) 2015 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Header files */

#include "board.h"
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "nci_utils.h"
#include <phOsalNfc_Log.h>
#include "phOsalNfc_Thread.h"
#include "wearable_platform_int.h"
#include "lpc_lowpower_timer.h"
#include "lpc_res_mgr.h"
#include "UserData_Storage.h"

#include "fp.h"

/*
 * System state to check whether NFC, BLE
 * bootup or firmware download is
 * done.
 */
int8_t	GeneralState = 0;
#if (LS_ENABLE == TRUE)
const unsigned char hash[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 };
const int32_t hash_len = sizeof(hash);
#endif

#ifndef NFC_UNIT_TEST
#include <ble_handler.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
xTaskHandle xHandleBLETask;                        /* FreeRTOS taskhandle for BLE Task */
xTaskHandle xHandleBleNfcBridge;                        /* FreeRTOS taskhandle for NFC Task */
QueueHandle_t xQueueNFC;
SemaphoreHandle_t nfcQueueLock;
#else
extern void SeApi_Test_Task(void *pParams);
xTaskHandle xHandleSeApiTest;                        /* FreeRTOS taskhandle for SeApi TestTask */
#endif

extern uint8_t buffer_tlv[];
												
#define TICKRATE_HZ (10)	/* 10 ticks per second */

/* Last sector address */
#define START_ADDR_LAST_SECTOR      0x00078000

/* LAST SECTOR */
#define IAP_LAST_SECTOR             15

/* Number of bytes to be written to the last sector */
#define IAP_NUM_BYTES_TO_WRITE      1024

/* Size of each sector */
#define SECTOR_SIZE                 32768

/* Number elements in array */
#define ARRAY_ELEMENTS          (IAP_NUM_BYTES_TO_WRITE / sizeof(uint32_t))

/* Data array to write to flash */
static uint32_t src_iap_array_data[ARRAY_ELEMENTS];

void iap_test(void)
{
	int loop = 1;	/* Used to avoid unreachable statement warning */
	int i;
	uint8_t ret_code;
	uint32_t part_id;

	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* Initialize the array data to be written to FLASH */
	for (i = 0; i < ARRAY_ELEMENTS; i++) {
		src_iap_array_data[i] = 0x11223340 + i;
	}

	/* Read Part Identification Number*/
	part_id = Chip_IAP_ReadPID();
	DEBUGOUT("Part ID is: %x\r\n", part_id);

	/* Disable interrupt mode so it doesn't fire during FLASH updates */
	__disable_irq();

	/* IAP Flash programming */
	/* Prepare to write/erase the last sector */
	ret_code = Chip_IAP_PreSectorForReadWrite(IAP_LAST_SECTOR, IAP_LAST_SECTOR);

	/* Error checking */
	if (ret_code != IAP_CMD_SUCCESS) {
		DEBUGOUT("Command failed to execute, return code is: %x\r\n", ret_code);
	}

	/* Erase the last sector */
	ret_code = Chip_IAP_EraseSector(IAP_LAST_SECTOR, IAP_LAST_SECTOR);

	/* Error checking */
	if (ret_code != IAP_CMD_SUCCESS) {
		DEBUGOUT("Command failed to execute, return code is: %x\r\n", ret_code);
	}

	/* Prepare to write/erase the last sector */
	ret_code = Chip_IAP_PreSectorForReadWrite(IAP_LAST_SECTOR, IAP_LAST_SECTOR);

	/* Error checking */
	if (ret_code != IAP_CMD_SUCCESS) {
		DEBUGOUT("Command failed to execute, return code is: %x\r\n", ret_code);
	}

	/* Write to the last sector */
	DEBUGOUT("iap addr: %x, len:%d\r\n", src_iap_array_data, IAP_NUM_BYTES_TO_WRITE);
	ret_code = Chip_IAP_CopyRamToFlash(START_ADDR_LAST_SECTOR, src_iap_array_data, IAP_NUM_BYTES_TO_WRITE);

	/* Error checking */
	if (ret_code != IAP_CMD_SUCCESS) {
		DEBUGOUT("Command failed to execute, return code is: %x\r\n", ret_code);
	}

	DEBUGOUT("IAP Example completed! %x\r\n", ret_code);

	/* Re-enable interrupt mode */
	__enable_irq();

	/* Endless loop */
	while (loop) {
		__WFI();
	}
}
												
												
int main(void)
{
    phOsalNfc_ThreadCreationParams_t threadparams;
    /* Generic Initialization */
    SystemCoreClockUpdate();

	
    Board_Init();
    Board_LED_Set(0, false);                               // initial LED0 state is off
	
	  //iap_test();

    #if 0
    /* Enable BOD power */
    Chip_SYSCON_PowerUp(SYSCON_PDRUNCFG_PD_BOD_RST | SYSCON_PDRUNCFG_PD_BOD_INTR);

    Chip_PMU_SetBODLevels(PMU_BODRSTLVL_2_30V, PMU_BODINTVAL_3_05v);

    /* If the board was reset due to a BOD event, the reset can be
       detected here. Turn on LED0 if reset occured due to BOD. */
    if ((Chip_SYSCON_GetSystemRSTStatus() & SYSCON_RST_BOD) != 0) {
                Chip_SYSCON_ClearSystemRSTStatus(SYSCON_RST_BOD);
                //Board_LED_Set(0, true);

                /*
                 * Enable BOD interrupt so that when the battery reaches
                 * high threshold value, trigger BOD interrupt to wakeup the
                 * system.
                 */
                Chip_PMU_EnableBODInt();

                /* Enable BOD interrupt */
                NVIC_EnableIRQ(BOD_IRQn);
    }
    else
    {
    	/*
    	 * BOD reset in the case where battery voltage
    	 * drops to lower threshold value.
    	 */
    	Chip_PMU_EnableBODReset();
    }
    #endif

   	g_Timer.init(); 
    
    //ResMgr_Init();
    //ResMgr_EnterPLLMode(false);


    Chip_Clock_EnablePeriphClock(SYSCON_CLOCK_SRAM2);
    
    /* Init timer */
    Chip_TIMER_Init(LPC_TIMER0);
    Chip_TIMER_Init(LPC_TIMER1);
    

#ifdef DEBUG_ENABLE
    /* Initialize OSAL layer */
    phOsalNfc_Log_Init();
#endif

    initUserdataStorage();
#ifdef NFC_UNIT_TEST
    /* Start NFC task */
		
    threadparams.stackdepth = SEAPI_TEST_TASK_SIZE;
    strcpy((char *)threadparams.taskname, "SeApi_Test_Task");
    threadparams.pContext = NULL;
    threadparams.priority = 3;
    if(NFCSTATUS_SUCCESS != phOsalNfc_Thread_Create(&xHandleSeApiTest, SeApi_Test_Task, &threadparams))
    {
        phOsalNfc_Log("\n\r ---SeApi_Test_Task create failed ---\n");
    }
    else
    {
        /* Start the kernel.  From here on, only tasks and interrupts will run. */
        vTaskStartScheduler();
    }
#else
		
	//fp_init();
	//fp_template_load();
    //fp_match_feature_load();

#if 1
    xQueueNFC = xQueueCreate(NFC_QUEUE_SIZE, sizeof(xTLVMsg));
	//create mutexLock to protect NFC Queue.
	nfcQueueLock = xSemaphoreCreateMutex();

	phOsalNfc_Log("\n\n --- WEARABLE_REF_DEVICE ---\n\n");

	/* NFC Core thread */
    threadparams.stackdepth = BLE_NFC_BRIDGE_TASK_SIZE;
    strcpy((char *)threadparams.taskname, "BleNfcBridge");      // PRQA S 3200
    threadparams.pContext = NULL;
    threadparams.priority = 4;
    if (phOsalNfc_Thread_Create(&xHandleBleNfcBridge, &BleNfcBridge, &threadparams) != NFCSTATUS_SUCCESS)
    {
        phOsalNfc_Log("\n\r ---BLE NFC Bridge thread create failed \n");
        while(1);  
     
    }

	/* BLE Core thread */
    threadparams.stackdepth = BLE_TASK_SIZE;
    strcpy((char *)threadparams.taskname, "BLETask");
    threadparams.pContext = NULL;
    threadparams.priority = 3;
    phOsalNfc_Thread_Create(&xHandleBLETask, &BLETask, &threadparams);
#endif
    vTaskStartScheduler();

#endif

    /* Since the application has nothing else to do ... just loop here */
    while(1);

    return (0);
}
