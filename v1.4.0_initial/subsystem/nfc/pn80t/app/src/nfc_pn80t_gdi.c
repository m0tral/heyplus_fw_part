/*
* Copyright (c), GDI Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "gdi_type.h"
#include "gdi_utils.h"
#include "gdi_mcu_config.h"
#include "gdi_os.h"
#include "gdi_timer.h"
#include "gdi_hal_i2c.h"
#include "gdi_hal_gpio.h"

#include "board.h"
#include <stdlib.h>
#include "nci_utils.h"
#include <phOsalNfc_Log.h>
#include "phOsalNfc_Thread.h"
#include "wearable_platform_int.h"
#include "lpc_lowpower_timer.h"
#include "lpc_res_mgr.h"
#include "UserData_Storage.h"
#include "fp.h"

#ifndef NFC_UNIT_TEST
#include <ble_handler.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
xTaskHandle xHandleBLETask;                        /* FreeRTOS taskhandle for BLE Task */
xTaskHandle xHandleBleNfcBridge;                   /* FreeRTOS taskhandle for NFC Task */
QueueHandle_t xQueueNFC;
SemaphoreHandle_t nfcQueueLock;
#else
extern void SeApi_Test_Task(void *pParams);
xTaskHandle xHandleSeApiTest;                      /* FreeRTOS taskhandle for SeApi TestTask */
#endif

/*********************************************************************
 * CONSTANTS
 */ 

#define NFC_I2C_IRQ_PORT              0
#define NFC_I2C_IRQ_PIN               19

#define NFC_VEN_PORT                  0
#define NFC_VEN_PIN                   20

#define NFC_DNLD_PORT                 0
#define NFC_DNLD_PIN                  25

#define NFC_I2C_GPIO_PORT             0
#define NFC_I2C_PIN_SCL               23 //27
#define NFC_I2C_PIN_SDA               24 //28 

 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
int8_t	GeneralState = 0;
extern uint8_t buffer_tlv[];

#if (LS_ENABLE == TRUE)
const unsigned char hash[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13 };
const int32_t hash_len = sizeof(hash);
#endif

volatile bool RxIrqReceived = FALSE;
volatile bool isirqEnabled = FALSE;
void * RxIrqSem = NULL;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

void Enable_GPIO0_IRQ(void)
{
    isirqEnabled = TRUE;
    gdi_hal_gpio_gint0_enable(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN, GDI_HAL_GINT_LEVEL_HIGH);
}

void Clear_GPIO0_IRQ(void)
{
    gdi_hal_gpio_gint0_clear(NFC_I2C_IRQ_PORT);
}

void Disable_GPIO0_IRQ(void)
{
    gdi_hal_gpio_gint0_disable();
    isirqEnabled = FALSE;
}

extern uint8_t wakeup_source_nfc;

/**
 * @brief   NFC GPIO IRQ Handler
 *
 * @param   None
 *
 * @return  None
 */
void nfc_irq_handler(void)
{
    u32_t pin_stat;
    /*Board_LED_Toggle(0);*/
    /* As its configured for Edge Trigger, consider only the rising edge */
    //int_cpt++;
    pin_stat = gdi_hal_gpio_get(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN);

    if (1 == pin_stat) {
        phOsalNfc_ProduceSemaphore(RxIrqSem);
        RxIrqReceived = TRUE;
        Disable_GPIO0_IRQ();
    }
    wakeup_source_nfc = 1;
}

/**
 * @brief   NFC Power up sequence
 *
 * @param   None
 *
 * @return  None
 */
void nfc_power_up(void)
{
    /* Init GPIO */
    // NFC_IRQ
    gdi_hal_gpio_init(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN, GDI_HAL_GPIO_DIR_INPUT);

    // VEN_IO
    gdi_hal_gpio_init(NFC_VEN_PORT, NFC_VEN_PIN, GDI_HAL_GPIO_DIR_OUTPUT);

    // Download_IO
    gdi_hal_gpio_init(NFC_DNLD_PORT, NFC_DNLD_PIN, GDI_HAL_GPIO_DIR_OUTPUT);
	  
	  /* Init I2C Pin */
    gdi_hal_i2c_pin_init(NFC_I2C_GPIO_PORT, NFC_I2C_PIN_SCL, NFC_I2C_GPIO_PORT, NFC_I2C_PIN_SDA);
	
    // Init Download_IO
    gdi_hal_gpio_set(NFC_DNLD_PORT, NFC_DNLD_PIN, 0);
    gdi_os_delay(20);

    // Reset the NFC chip
    gdi_hal_gpio_set(NFC_VEN_PORT, NFC_VEN_PIN, 0);
    gdi_os_delay(200);
    gdi_hal_gpio_set(NFC_VEN_PORT, NFC_VEN_PIN, 1);
    gdi_os_delay(200);
}


/**
 * @brief   NFC module HW initialization
 *
 * @param   None
 *
 * @return  None
 */
void nfc_hw_init(void)
{
    isirqEnabled = FALSE;
    RxIrqReceived = FALSE;

    //phOsalNfc_CreateMutex(&ni2cRomAccessMutex);
    phOsalNfc_CreateSemaphore(&RxIrqSem, 0);

    /* Power the board up */
    nfc_power_up();

    /* Init I2C */
    //gdi_hal_i2c_pin_init(NFC_I2C_GPIO_PORT, NFC_I2C_PIN_SCL, NFC_I2C_GPIO_PORT, NFC_I2C_PIN_SDA);
    gdi_hal_i2c_se_init();

    /* Init and  */
    gdi_hal_gpio_gint0_init(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN, nfc_irq_handler); // 

}


/**
 * @brief   API to init NFC module
 *
 * @param   None
 *
 * @return  None
 */
gdi_sts_t nfc_init(void)
{
    phOsalNfc_ThreadCreationParams_t threadparams;
    
    g_Timer.init();

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

    return GDI_SUCC;

}
