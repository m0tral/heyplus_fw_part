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

/*
 *  OSAL header files related to memory, debug, random, semaphore and mutex functions.
 */

#ifndef PHNFCCOMMON_H
#define PHNFCCOMMON_H

/*
************************* Include Files ****************************************
*/

#include "WearableCoreSDK_BuildConfig.h"
#ifdef COMPANION_DEVICE
#include "companion_types.h"
#endif
#include <phNfcStatus.h>
#ifndef COMPANION_DEVICE
#include <phOsalNfc_Timer.h>
#include <phDal4Nfc_messageQueueLib.h>
#endif
#include <phNfcCompId.h>


#define FW_DLL_ROOT_DIR "/system/vendor/firmware/"
#define FW_DLL_EXTENSION ".so"

#if(NFC_NXP_CHIP_TYPE == PN548C2)
#define FW_LIB_PATH       FW_DLL_ROOT_DIR "libpn548ad_fw"          FW_DLL_EXTENSION
/* Restore Currupted PLL Setttings/etc */
#define PLATFORM_LIB_PATH FW_DLL_ROOT_DIR "libpn548ad_fw_platform" FW_DLL_EXTENSION
/* Upgrade the public Key */
#define PKU_LIB_PATH      FW_DLL_ROOT_DIR "libpn548ad_fw_pku"      FW_DLL_EXTENSION
#elif(NFC_NXP_CHIP_TYPE == PN553)
/* Actual FW library name*/
#define FW_LIB_PATH       FLASH_CONF_ROOT_DIR "libpn553tc_fw"      FW_DLL_EXTENSION
/* Restore Corrupted PLL Settings/etc */
#define PLATFORM_LIB_PATH FW_DLL_ROOT_DIR "libpn553tc_fw_platform" FW_DLL_EXTENSION
/* Upgrade the public Key */
#define PKU_LIB_PATH      FW_DLL_ROOT_DIR "libpn553tc_fw_pku"      FW_DLL_EXTENSION
#else
/* Actual FW library name*/
#define FW_LIB_PATH       FW_DLL_ROOT_DIR "libpn547_fw"          FW_DLL_EXTENSION
/* Restore Currupted PLL Settings/etc */
#define PLATFORM_LIB_PATH FW_DLL_ROOT_DIR "libpn547_fw_platform" FW_DLL_EXTENSION
/* Upgrade the public Key */
#define PKU_LIB_PATH      FW_DLL_ROOT_DIR "libpn547_fw_pku"      FW_DLL_EXTENSION
#endif

#if(NFC_NXP_CHIP_TYPE == PN548C2)
#define COMPILATION_MW "PN548C2"
#elif(NFC_NXP_CHIP_TYPE == PN553)
#define COMPILATION_MW "PN553"
#else
#define COMPILATION_MW "PN547C2"
#endif
/* HAL Version number (Updated as per release) */
#define NXP_MW_VERSION_MAJ  (3U)
#define NXP_MW_VERSION_MIN  (4U)

#define GET_EEPROM_DATA (1U)
#define SET_EEPROM_DATA (2U)
#define BITWISE    (1U)
#define BYTEWISE   (2U)

#define GET_FW_DWNLD_FLAG   (1U)
#define RESET_FW_DWNLD_FLAG (2U)

/*
 *****************************************************************
 ***********  System clock source selection configuration ********
 *****************************************************************
 */
/* changed as per user manual pn553 section number 14.1[extension:System configuration] */
//#define CLK_SRC_UNDEF      0
#define CLK_SRC_PLL        0
#define CLK_SRC_XTAL       1
#define CLK_SRC_PADDIRECT  3
#ifdef PN80T_CD_ENABLE
#define NXP_SYS_CLK_SRC_SEL         CLK_SRC_XTAL
#else
/*Extern crystal clock source*/
#define NXP_SYS_CLK_SRC_SEL         CLK_SRC_PLL  /* Use one of CLK_SRC_<value> */
/*Direct clock*/

#endif

/*
 *****************************************************************
 ***********  System clock frequency selection configuration ****************
 * If Clk_Src is set to PLL, make sure to set the Clk_Freq also*
 *****************************************************************
 */
#define CLK_FREQ_UNDEF         0
#define CLK_FREQ_13MHZ         1
#define CLK_FREQ_19_2MHZ       2
#define CLK_FREQ_24MHZ         3
#define CLK_FREQ_26MHZ         4
#define CLK_FREQ_38_4MHZ       5
#define CLK_FREQ_52MHZ         6

#define NXP_SYS_CLK_FREQ_SEL  CLK_FREQ_19_2MHZ /* Set to one of CLK_FREQ_<value> */

#define CLK_TO_CFG_DEF         1
#define CLK_TO_CFG_MAX         6
/*
 *  information to configure OSAL
 */
typedef struct phOsalNfc_Config
{
    uint8_t *pLogFile; /* Log File Name*/
    uintptr_t dwCallbackTaskHandle; /* Client Task to which message is posted */
}phOsalNfc_Config_t, *pphOsalNfc_Config_t /* Pointer to #phOsalNfc_Config_t */;


/*
 * Deferred call declaration.
 * This type of API is called from ClientApplication (main thread) to notify
 * specific callback.
 */
typedef  void (*pphOsalNfc_DeferFuncPointer_t) (void*);


/*
 * Deferred message specific info declaration.
 */
typedef struct phOsalNfc_DeferedCallInfo
{
        pphOsalNfc_DeferFuncPointer_t   pDeferedCall;/* pointer to Deferred callback */
        void                            *pParam;    /* contains timer message specific details*/
}phOsalNfc_DeferedCallInfo_t;


/*
 * States in which a OSAL timer exist.
 */
typedef enum
{
    eTimerIdle = 0,         /* Indicates Initial state of timer */
    eTimerRunning = 1,      /* Indicate timer state when started */
    eTimerStopped = 2       /* Indicates timer state when stopped */
}phOsalNfc_TimerStates_t;   /* Variable representing State of timer */

#ifndef COMPANION_DEVICE
/*
 **Timer Handle structure containing details of a timer.
 */
typedef struct phOsalNfc_TimerHandle
{
    uint32_t TimerId;                                   /* ID of the timer */
    void *hTimerHandle;                               /* Handle of the timer */
    pphOsalNfc_TimerCallbck_t   Application_callback;   /* Timer callback function to be invoked */
    void *pContext;                                     /* Parameter to be passed to the callback function */
    phOsalNfc_TimerStates_t eState;                     /* Timer states */
    phLibNfc_Message_t tOsalMessage;                    /* Osal Timer message posted on User Thread */
    phOsalNfc_DeferedCallInfo_t tDeferedCallInfo;       /* Deferred Call structure to Invoke Callback function */
}phOsalNfc_TimerHandle_t,*pphOsalNfc_TimerHandle_t;     /* Variables for Structure Instance and Structure Ptr */
#endif
#endif /*  PHOSALNFC_H  */
