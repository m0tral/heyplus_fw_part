/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "ry_type.h"

/**
 *  @brief  Definition for Heap Usage
 */
    
// <o> Internal SRAM memory size[Kbytes] <8-64>
//  <i>Default: 64
#ifdef __ICCARM__
    // Use *.icf ram symbal, to avoid hardcode.
    extern char __ICFEDIT_region_RAM_end__;
#define STM32_SRAM_END          &__ICFEDIT_region_RAM_end__
#else
#define STM32_SRAM_SIZE         640
#define STM32_SRAM_END          (0x20000000 + STM32_SRAM_SIZE * 1024)
#endif
    
#ifdef __CC_ARM
    extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define HEAP_BEGIN    (__segment_end("HEAP"))
#else
    extern int __bss_end;
#define HEAP_BEGIN    (&__bss_end)
#endif
    
#define HEAP_END          STM32_SRAM_END



/**
 *  @brief  Definition for RAM Usage
 */
#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define STM32_SRAM_BEGIN    (&Image$$RW_IRAM1$$ZI$$Limit)
#elif __ICCARM__
#pragma section="HEAP"
#define STM32_SRAM_BEGIN    (__segment_end("HEAP"))
#else
extern int __bss_end;
#define STM32_SRAM_BEGIN    (&__bss_end)
#endif


/*
 * Build Option
 */
#define RY_DEBUG                                1
#define RY_RELEASE                              2

#define RY_BUILD                                RY_RELEASE       


/*
 *  Release Build Option Related Configuration
 *  debug for firmware devolep:  (RELEASE_INTERNAL_TEST = FALSE) && (RELEASE_FACTORY_PRODUCT = FALSE)
 *  internal release for test:   (RELEASE_INTERNAL_TEST = TRUE)  && (RELEASE_FACTORY_PRODUCT = FALSE)
 *  factory release for product: (RELEASE_INTERNAL_TEST = FALSE) && (RELEASE_FACTORY_PRODUCT = TRUE)
 */
#if (RY_BUILD == RY_DEBUG)
    #define RELEASE_INTERNAL_TEST               FALSE    //FALSE
    #define RELEASE_FACTORY_PRODUCT             FALSE 
#elif (RY_BUILD == RY_RELEASE)
    #define RELEASE_INTERNAL_TEST               FALSE //FALSE    
    #define RELEASE_FACTORY_PRODUCT             TRUE//FALSE 
#endif


/*
 * Hardware Realated Confituration
 */
#define HW_VER_SAKE_01                          1
#define HW_VER_SAKE_02                          2
#define HW_VER_SAKE_03                          3

#define HW_VER_CURRENT                          HW_VER_SAKE_03

/* Define for minor change with SAKE_02 version */


#if (HW_VER_CURRENT == HW_VER_SAKE_03)
#define DC_IN_IRQ_ENABLE                        TRUE
#define HRM_USE_LDO                             TRUE
#endif


#if (RY_BUILD == RY_DEBUG)
    #define WATCH_DOG_START_FIRST               FALSE
#elif (RY_BUILD == RY_RELEASE)
    #define WATCH_DOG_START_FIRST               TRUE
#endif

#define OLED_OFF_MODE_CUT_POWER		            TRUE
#define ALG_SLEEP_CALC_DESAY                    FALSE


/*
 * @brief Mijia Product ID,
 */
#define APP_PRODUCT_ID                          0x038F


/*
 * Debug and Test Related Configuration
 */
#if ((RELEASE_INTERNAL_TEST == FALSE) && (RELEASE_FACTORY_PRODUCT == FALSE))
#define APPCONFIG_DEBUG_ENABLE                 TRUE
#define GUI_DEBUG_DEFAULT_SURFACE_EXTERNAL     FALSE     //true: not use internal bmp surface img for reduce code size
#define TP_FIRMWARE_DEBUG_NO_IMG               TRUE
#define SLEEP_CALC_DEBUG					   FALSE
#define QR_CODE_DEBUG                          FALSE
#define DBG_POWERON_CONSOLE                    FALSE      // power-on display console
#define NFC_FIRMWARE_INCLUDE                   TRUE//FALSE
#define SPI_LOCK_DEBUG                         FALSE
#else
#define APPCONFIG_DEBUG_ENABLE                 FALSE
#define GUI_DEBUG_DEFAULT_SURFACE_EXTERNAL     FALSE
#define TP_FIRMWARE_DEBUG_NO_IMG               TRUE
#define SLEEP_CALC_DEBUG                       FALSE
#define QR_CODE_DEBUG                          FALSE
#define DBG_POWERON_CONSOLE                    FALSE
#define NFC_FIRMWARE_INCLUDE                   TRUE
#define SPI_LOCK_DEBUG                         FALSE
#endif


#define RAWLOG_SAMPLE_ENABLE                   FALSE

#define STARTUP_LOGO_EN                        TRUE

#define ALG_SLEEP_EN                           TRUE       //true: enable sleep algrithm
#define DBG_HRM_AUTO_SAMPLE                    FALSE      //true: default enable heart_rate_auto_sample
#define DEBUG_ALG_STATE_DETECT                 FALSE 
#define MENU_SEQUENCE_01					   FALSE
#define HRM_DO_ANIMATE				           TRUE        //set FALSE for hrm alg debug, else set true
#define DBG_ALG_RAW_DATA_CACL                  FALSE       // alg raw data step verify
/*
 * If defined and set RY_DEBUG to TRUE, the test application will use the
 * uart port as UI interface to do the test or debug.
 */
#define GUI_K_KERNEL_ENABLE                    FALSE

#ifndef SPI_COMM_DOTEST
#define SPI_COMM_DOTEST 	                   FALSE	
#endif

#ifndef I2C_COMM_DOTEST
#define I2C_COMM_DOTEST 	                   FALSE	
#endif

#define FATFS_COMM_DOTEST	                   FALSE
#define VIBE_PWM_DOTEST		                   FALSE
#define LED_PWM_DOTEST		                   FALSE


/*
 * Subsystem MARCO
 */
#define RY_FP_ENABLE                           FALSE
#define RY_SE_ENABLE                           TRUE
#define RY_GUP_ENABLE                          FALSE
#define RY_GUI_ENABLE                          TRUE
#define RY_TOUCH_ENABLE                        TRUE
#define RY_GSENSOR_ENABLE                      TRUE
#define RY_HRM_ENABLE                          TRUE
#define RY_BLE_ENABLE                          TRUE
#define RY_TOOL_ENABLE                         TRUE
#define RY_FS_ENABLE                           TRUE

/*
 * touch firmware upgrade
 */
#if RY_BUILD == RY_DEBUG
    #define UPGEADE_TP_ENABLE                  FALSE
#else
    #define UPGEADE_TP_ENABLE                  TRUE
#endif

/*
 * Feature Related Configuration
 */
#define FEATURE_WRIST_FILT_ENABLE              TRUE 


/*
 * DEBUG ENABLE SEGGER SYSVIEW  OS任务诊断工具
 */
#if RY_BUILD == RY_DEBUG
#define RY_SEGGER_SYSVIEW_ENABLE               FALSE
#else
#define RY_SEGGER_SYSVIEW_ENABLE               FALSE
#endif

/*
 * UI Effect Related Configuration
 */
#define RY_UI_EFFECT                           TRUE


#if RY_UI_EFFECT
    #define RY_UI_NO_CARTOON         TRUE
    #define RY_UI_EFFECT_MOVE        FALSE
    //#define RY_UI_EFFECT_TAP_MOVE    TRUE
#endif



/*
 * If defined and set to TRUE, the test application will use the
 * uart port as UI interface to do the test or debug.
 */
#define AUTHENTICATE_ENABLE    FALSE

/*
 * If defined and set to TRUE, the test application will use the
 * uart port as UI interface to do the test or debug.
 */
#define DIAGNOSTIC_ENABLE      FALSE

/*
 * If defined and set to TRUE, the test application will use the
 * uart port as UI interface to do the test or debug.
 */
#define STATISTIC_ENABLE       FALSE

/*
 * If defined and set to TRUE, the test application will use the
 * uart port as UI interface to do the test or debug.
 */
#define DYNAMIC_MODULE         FALSE

/*
 * If defined and set to TRUE, the application will send log data,
 * event, task begin and task end through VCD format.
 */
#define RT_LOG_ENABLE          FALSE


/*
 * If defined and set to TRUE, the test application will use the
 * uart port as UI interface to do the test or debug.
 */
#define RY_UNIT_TEST          FALSE

#if (RY_UNIT_TEST == TRUE)
    /*
     * Command line is built if API_TEST_ENABLE is set to true
     */
    #define API_TEST_ENABLE TRUE

    /*
     * Command line is built if BSP_TEST_ENABLE is set to true
     */
    #define BSP_TEST_ENABLE TRUE

#endif 


#if (API_TEST_ENABLE == TRUE)
    /*
     * Command line is built if BSP_TEST_ENABLE is set to true
    */
    #define CLI_ENABLE TRUE
#endif 


#if (BSP_TEST_ENABLE == TRUE)
    /*
     * Command line is built if BSP_TEST_ENABLE is set to true
    */
    #define CLI_ENABLE TRUE
#endif 




#endif  /* __APP_CONFIG_H__ */

