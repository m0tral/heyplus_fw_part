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
#include "ry_type.h"
#include "ry_utils.h"
#include "app_config.h"
#include "ryos.h"
#include "ryos_timer.h"
#include "ry_hal_i2c.h"
#include "ry_hal_gpio.h"
#include "scheduler.h"

#include "board.h"
#include <stdlib.h>
#include "nci_utils.h"
#include <phOsalNfc_Log.h>
#include "phOsalNfc_Thread.h"
#include "wearable_platform_int.h"
//#include "lpc_lowpower_timer.h"
//#include "lpc_res_mgr.h"
#include "UserData_Storage.h"

#include "nfc_pn80t.h"
#include "am_hal_gpio.h"
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "ry_hal_mcu.h"
#include "ry_hal_flash.h"
#include "touch.h"

#ifndef NFC_UNIT_TEST
#include <ble_handler.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
TaskHandle_t xHandleBLETask;                        /* FreeRTOS taskhandle for BLE Task */
TaskHandle_t xHandleBleNfcBridge;                   /* FreeRTOS taskhandle for NFC Task */
QueueHandle_t xQueueNFC;
SemaphoreHandle_t nfcQueueLock;
#else
extern void SeApi_Test_Task(void *pParams);
xTaskHandle xHandleSeApiTest;                      /* FreeRTOS taskhandle for SeApi TestTask */
#endif

/*********************************************************************
 * CONSTANTS
 */ 



/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    int rfblk_len[6];
    int tvdd_len[3];
    int nxpcoreconf_len;
} nfc_para_t;


 
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

nfc_stack_cb_t nfc_stack_cb;

nfc_para_t nfc_para;

u8_t nfc_i2c_instance;
u32_t nfc_irq_time;


extern u8_t cfgData_Nxp_Ext_Tvdd_Cfg_1[];
extern u8_t cfgData_Nxp_Ext_Tvdd_Cfg_2[];
extern u8_t cfgData_Nxp_Ext_Tvdd_Cfg_3[];
extern u8_t cfgData_Nxp_Rf_Conf_Blk_1[];
extern u8_t cfgData_Nxp_Rf_Conf_Blk_2[];
extern u8_t cfgData_Nxp_Rf_Conf_Blk_3[];
extern u8_t cfgData_Nxp_Rf_Conf_Blk_4[];
extern u8_t cfgData_Nxp_Rf_Conf_Blk_5[];
#if (FW_VERSION_TO_DOWNLOAD == PROD_11_01_0C)
extern u8_t cfgData_Nxp_Rf_Conf_Blk_6[];
#endif
extern u8_t cfgData_Nxp_Core_Conf[];


/*********************************************************************
 * LOCAL FUNCTIONS
 */
void nfc_irq_handler(void);
                        
void Enable_GPIO_IRQ(void)
{
    isirqEnabled = TRUE;
    //gdi_hal_gpio_gint0_enable(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN, GDI_HAL_GINT_LEVEL_HIGH);
    ry_hal_gpio_int_enable(GPIO_IDX_NFC_IRQ, nfc_irq_handler);
}

void Clear_GPIO0_IRQ(void)
{
    //gdi_hal_gpio_gint0_clear(NFC_I2C_IRQ_PORT);
}

void Disable_GPIO_IRQ(void)
{
    //gdi_hal_gpio_gint0_disable();
    ry_hal_gpio_int_disable(GPIO_IDX_NFC_IRQ);
    isirqEnabled = FALSE;
}



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
    //pin_stat = gdi_hal_gpio_get(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN);
    pin_stat = ry_hal_gpio_get(GPIO_IDX_NFC_IRQ);

    if (1 == pin_stat) {
        nfc_irq_time = ry_hal_clock_time();
        #if 1
        phOsalNfc_ProduceSemaphore(RxIrqSem);
        RxIrqReceived = TRUE;
        Disable_GPIO_IRQ();
        #else 
        quick_nfc_i2c_read();
        #endif
    }
    //wakeup_source_nfc = 1;
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
#if (HW_VER_CURRENT == HW_VER_SAKE_01)
    ry_hal_gpio_init(GPIO_IDX_NFC_BAT_EN, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_18V_EN, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_ESE_PWR_REQ, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_5V_EN, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_WKUP_REQ, RY_HAL_GPIO_DIR_INPUT);
    ry_hal_gpio_init(GPIO_IDX_SYS_5V_EN, RY_HAL_GPIO_DIR_OUTPUT);
    
    ry_hal_gpio_set(GPIO_IDX_NFC_BAT_EN, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 0);  //1
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_5V_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_SYS_5V_EN, 1);

#elif (HW_VER_CURRENT == HW_VER_SAKE_02)

    ry_hal_gpio_init(GPIO_IDX_NFC_18V_EN, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_ESE_PWR_REQ, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_WKUP_REQ, RY_HAL_GPIO_DIR_INPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_SW, RY_HAL_GPIO_DIR_OUTPUT);

    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_SW, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
#elif (HW_VER_CURRENT == HW_VER_SAKE_03)
    ry_hal_gpio_init(GPIO_IDX_NFC_18V_EN, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_ESE_PWR_REQ, RY_HAL_GPIO_DIR_OUTPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_WKUP_REQ, RY_HAL_GPIO_DIR_INPUT);
    ry_hal_gpio_init(GPIO_IDX_NFC_SW, RY_HAL_GPIO_DIR_OUTPUT);

    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_SW, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);

    //ry_hal_gpio_set(12, 0); 
    am_hal_gpio_out_bit_clear(12);

#endif

    
    
    ryos_delay_ms(50);

    /* Init GPIO */
    // NFC_IRQ
    ry_hal_gpio_init(GPIO_IDX_NFC_IRQ, RY_HAL_GPIO_DIR_INPUT);

    // VEN_IO
    ry_hal_gpio_init(GPIO_IDX_NFC_VEN, RY_HAL_GPIO_DIR_OUTPUT);

    // Download_IO
    ry_hal_gpio_init(GPIO_IDX_NFC_DNLD, RY_HAL_GPIO_DIR_OUTPUT);
	  
	
    // Init Download_IO
    ry_hal_gpio_set(GPIO_IDX_NFC_DNLD, 0);
    ryos_delay_ms(20);

    // Reset the NFC chip
    ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 0);
    ryos_delay_ms(50);
    ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 1);
    ryos_delay_ms(50);
}


void nfc_power_down(void)
{
#if (HW_VER_CURRENT == HW_VER_SAKE_01)

    ry_hal_gpio_set(GPIO_IDX_NFC_BAT_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_5V_EN, 0);
    ry_hal_gpio_set(GPIO_IDX_SYS_5V_EN, 1);

#elif (HW_VER_CURRENT == HW_VER_SAKE_02)

    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_SW, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
#elif (HW_VER_CURRENT == HW_VER_SAKE_03)
    ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 0);
    ryos_delay_ms(20);
    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 0);
    
    //ry_hal_gpio_set(GPIO_IDX_NFC_SW, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
#endif
}

void nfc_power_off(void)
{
#if (HW_VER_CURRENT == HW_VER_SAKE_01)

    ry_hal_gpio_set(GPIO_IDX_NFC_BAT_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_5V_EN, 0);
    ry_hal_gpio_set(GPIO_IDX_SYS_5V_EN, 1);

#elif (HW_VER_CURRENT == HW_VER_SAKE_02)

    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 1);
    ry_hal_gpio_set(GPIO_IDX_NFC_SW, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
#elif (HW_VER_CURRENT == HW_VER_SAKE_03)
    ry_hal_gpio_set(GPIO_IDX_NFC_VEN, 0);
    ryos_delay_ms(20);
    ry_hal_gpio_set(GPIO_IDX_NFC_18V_EN, 0);
    
    ry_hal_gpio_set(GPIO_IDX_NFC_SW, 0);
    ry_hal_gpio_set(GPIO_IDX_NFC_ESE_PWR_REQ, 0);
#endif
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
    //gdi_hal_i2c_se_init();
    nfc_i2c_instance = I2C_IDX_NFC;
    ry_hal_i2cm_init((ry_i2c_t)nfc_i2c_instance);

    /* Init and  */
    //gdi_hal_gpio_gint0_init(NFC_I2C_IRQ_PORT, NFC_I2C_IRQ_PIN, nfc_irq_handler); // 
    ry_hal_gpio_init(GPIO_IDX_NFC_IRQ, RY_HAL_GPIO_DIR_INPUT);
    ry_hal_gpio_int_enable(GPIO_IDX_NFC_IRQ, nfc_irq_handler);

}






void nfc_para_len_set(u8_t type, int len)
{
    switch (type) {
        case NFC_PARA_TYPE_RF1:
            nfc_para.rfblk_len[0] = len;
            break;

        case NFC_PARA_TYPE_RF2:
            nfc_para.rfblk_len[1] = len;
            break;

        case NFC_PARA_TYPE_RF3:
            nfc_para.rfblk_len[2] = len;
            break;

        case NFC_PARA_TYPE_RF4:
            nfc_para.rfblk_len[3] = len;
            break;

        case NFC_PARA_TYPE_RF5:
            nfc_para.rfblk_len[4] = len;
            break;

        case NFC_PARA_TYPE_RF6:
            nfc_para.rfblk_len[5] = len;
            break;

        case NFC_PARA_TYPE_TVDD1:
            nfc_para.tvdd_len[0] = len;
            break;

        case NFC_PARA_TYPE_TVDD2:
            nfc_para.tvdd_len[1] = len;
            break;

        case NFC_PARA_TYPE_TVDD3:
            nfc_para.tvdd_len[2] = len;
            break;

        case NFC_PARA_TYPE_NXP_CORE:
            nfc_para.nxpcoreconf_len = len;
            break;

        default:
            break;
    }
}

#include "phDnldNfc_Internal.h"
extern pphDnldNfc_DlContext_t gpphDnldContext;
u8_t nfc_version_get(void)
{
    //gpphDnldContext->nxp_nfc_fw[4];
    return gpphDnldContext->nxp_nfc_fw[4];
}



ry_sts_t nfc_para_save(u32_t addr, u8_t* data, int len)
{
    ry_sts_t status;

    status = ry_hal_flash_write(addr, data, len);
    if (status != RY_SUCC) {
        return RY_SET_STS_VAL(RY_CID_SE, RY_ERR_PARA_SAVE);
    }

    return RY_SUCC;
}

ry_sts_t nfc_para_block_save(void)
{
    nfc_para_save(FLASH_ADDR_NFC_PARA_BLOCK, (u8_t*)&nfc_para, sizeof(nfc_para));
    return RY_SUCC;
}


ry_sts_t nfc_para_load(u32_t addr, u8_t* data, int len)
{
    ry_sts_t status;
    u8_t* temp = ry_malloc(512);
    if (temp == NULL) {
        LOG_ERROR("[nfc_para_recover] No mem.\r\n");
        return RY_SET_STS_VAL(RY_CID_SE, RY_ERR_NO_MEM);
    }
    
    status = ry_hal_flash_read(addr, temp, len);
    if (status != RY_SUCC) {
        ry_free((void*)temp);
        return RY_SET_STS_VAL(RY_CID_SE, RY_ERR_PARA_RESTORE);
    }

    if ((temp[0] == 0xFF) && (temp[1] == 0xFF) && (temp[2] == 0xFF) && (temp[3] == 0xFF)) {
        // Empty
        ry_free((void*)temp);
        return RY_SET_STS_VAL(RY_CID_SE, RY_ERR_PARA_RESTORE);
    }

    ry_memcpy(data, temp, len);
    ry_free((void*)temp);
    return RY_SUCC;
}


ry_sts_t nfc_para_recover(void)
{
    ry_sts_t status;

    // Load length field
    status = nfc_para_load(FLASH_ADDR_NFC_PARA_BLOCK, (u8_t*)&nfc_para, sizeof(nfc_para));
    if (status != RY_SUCC) {
        goto err_exit;
    }

    if ((nfc_para.rfblk_len[0] == 0) || (nfc_para.rfblk_len[0] == 0xFFFFFFFF) || (((u32_t)nfc_para.rfblk_len[0]) > 256)) {
        status = RY_SET_STS_VAL(RY_CID_SE, RY_ERR_PARA_RESTORE);
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_RF1, (u8_t*)cfgData_Nxp_Rf_Conf_Blk_1, nfc_para.rfblk_len[0]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_RF2, (u8_t*)cfgData_Nxp_Rf_Conf_Blk_2, nfc_para.rfblk_len[1]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_RF3, (u8_t*)cfgData_Nxp_Rf_Conf_Blk_3, nfc_para.rfblk_len[2]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_RF4, (u8_t*)cfgData_Nxp_Rf_Conf_Blk_4, nfc_para.rfblk_len[3]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_RF5, (u8_t*)cfgData_Nxp_Rf_Conf_Blk_5, nfc_para.rfblk_len[4]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

#if (FW_VERSION_TO_DOWNLOAD == PROD_11_01_0C)
    status = nfc_para_load(FLASH_ADDR_NFC_PARA_RF6, (u8_t*)cfgData_Nxp_Rf_Conf_Blk_6, nfc_para.rfblk_len[5]);
    if (status != RY_SUCC) {
        goto err_exit;
    }
#endif // (FW_VERSION_TO_DOWNLOAD == PROD_11_01_0C)
    
    status = nfc_para_load(FLASH_ADDR_NFC_PARA_TVDD1, (u8_t*)cfgData_Nxp_Ext_Tvdd_Cfg_1, nfc_para.tvdd_len[0]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_TVDD2, (u8_t*)cfgData_Nxp_Ext_Tvdd_Cfg_2, nfc_para.tvdd_len[1]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_TVDD3, (u8_t*)cfgData_Nxp_Ext_Tvdd_Cfg_3, nfc_para.tvdd_len[2]);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    status = nfc_para_load(FLASH_ADDR_NFC_PARA_NXP_CORE, (u8_t*)cfgData_Nxp_Core_Conf, nfc_para.nxpcoreconf_len);
    if (status != RY_SUCC) {
        goto err_exit;
    }

    LOG_DEBUG("[nfc_para_recover] Load RF parameters success.\r\n");
    return status;

err_exit:

    LOG_WARN("[nfc_para_recover] Load RF parameters error. status = %04x\r\n", status);
    return status;
    
}


void nfc_stack_shutdown(void)
{
    phOsalNfc_Thread_Delete(xHandleBleNfcBridge);
    vQueueDelete(xQueueNFC);
    vSemaphoreDelete(nfcQueueLock);
}
    

/**
 * @brief   API to close NFC statck.
 *
 * @param   None
 *
 * @return  None
 */
extern ryos_thread_t* sensor_alg_thread_handle;
ry_sts_t nfc_deinit(u8_t poweroff)
{
    touch_bypass_set(1);
    int32_t suspend_alg = 0;
    if (sensor_alg_thread_handle) {
        if (ryos_thread_get_state(sensor_alg_thread_handle) < RY_THREAD_ST_SUSPENDED){
            ryos_thread_suspend(sensor_alg_thread_handle);
            suspend_alg = 1;
        }
    }

    nfc_i2c_instance = I2C_IDX_NFC;
    ry_hal_i2cm_init((ry_i2c_t)nfc_i2c_instance);
    
    SeApi_Shutdown();

    nfc_stack_shutdown();

    if (poweroff) {
        nfc_power_off();
    } else {
        nfc_power_down();
    }
    

    nfc_i2c_instance = I2C_IDX_NFC;
    ry_hal_i2cm_init((ry_i2c_t)nfc_i2c_instance);

    if (sensor_alg_thread_handle) {
        if (suspend_alg){
            ryos_thread_resume(sensor_alg_thread_handle);
        }
    }
    
    touch_bypass_set(0);

    
    
    return RY_SUCC;
}


/**
 * @brief   API to init NFC module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t nfc_init(nfc_stack_cb_t stack_cb, u32_t needUpdate)
{
    phOsalNfc_ThreadCreationParams_t threadparams;

    nfc_stack_cb = stack_cb;
    
    //g_Timer.init();

#ifdef DEBUG_ENABLE
    /* Initialize OSAL layer */
    phOsalNfc_Log_Init();
#endif

    //LOG_DEBUG("RF BLK1 LEN: %d\r\n", sizeof(cfgData_Nxp_Rf_Conf_Blk_1));
    //LOG_DEBUG("RF BLK2 LEN: %d\r\n", sizeof(cfgData_Nxp_Rf_Conf_Blk_2));
    //LOG_DEBUG("RF BLK3 LEN: %d\r\n", sizeof(cfgData_Nxp_Rf_Conf_Blk_3));
    //LOG_DEBUG("RF BLK4 LEN: %d\r\n", sizeof(cfgData_Nxp_Rf_Conf_Blk_4));
    //LOG_DEBUG("RF BLK5 LEN: %d\r\n", sizeof(cfgData_Nxp_Rf_Conf_Blk_5));


    /* Recover RF parameters */
    nfc_para_recover();

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
    threadparams.pContext = (void*)(needUpdate);//NULL;
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
    //phOsalNfc_Thread_Create(&xHandleBLETask, &BLETask, &threadparams);

#endif

    return RY_SUCC;

}
