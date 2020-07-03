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


#include <phNxpNciHal.h>
#include <phNxpNciHal_ext.h>
#include <phNxpNciHal_Dnld.h>
#include <phNxpNciHal_Adaptation.h>
#include <phTmlNfc.h>
#include <phOsalNfc.h>
#include <phOsalNfc_Log.h>
#include <phOsalNfc_Thread.h>
#include <phDnldNfc.h>
#include <phDal4Nfc_messageQueueLib.h>
#include <phNxpLog.h>
#include <phNxpTransitConfig.h>
#include <phNxpConfig.h>
#include "debug_settings.h"

/*********************** Global Variables *************************************/
#define PN547C2_CLOCK_SETTING
#define PN547C2_DISABLE_DEAD_FUN 0
#undef  PN547C2_FACTORY_RESET_DEBUG
#define CORE_RES_STATUS_BYTE 3
extern int8_t GeneralState;

/* Processing of ISO 15693 EOF */
extern uint8_t icode_send_eof;
extern uint8_t icode_detected;
static uint8_t cmd_icode_eof[] = { 0x00, 0x00, 0x00 };
static uint8_t read_failed_disable_nfc = FALSE;
static uint8_t pwr_link_required = FALSE;
static uint8_t config_access = FALSE;
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
static NFCSTATUS phNxpNciHal_FwDwnld(BOOLEAN forcedDnd);
#endif
/* NCI HAL Control structure */
phNxpNciHal_Control_t nxpncihal_ctrl;

/* NXP Poll Profile structure */
phNxpNciProfile_Control_t nxpprofile_ctrl;

/* FW Mobile major number */
#define FW_MOBILE_MAJOR_NUMBER_PN551 0x05
#define FW_MOBILE_MAJOR_NUMBER_PN48C2 0x01

#define FW_MOBILE_MAJOR_NUMBER FW_MOBILE_MAJOR_NUMBER_PN48C2

/* TML Context */
extern phTmlNfc_Context_t *gpphTmlNfc_Context;
extern void phTmlNfc_set_fragmentation_enabled(phTmlNfc_i2cfragmentation_t result);
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
extern int phNxpNciHal_CheckFwRegFlashRequired(uint8_t* fw_update_req, uint8_t* rf_update_req);
static NFCSTATUS phNxpNciHal_fw_download(BOOLEAN forcedDnd);
static uint8_t gRecFwRetryCount; //variable to hold dummy FW recovery count
#endif
static uint8_t write_unlocked_status = NFCSTATUS_SUCCESS;
static int phNxpNciHal_fw_mw_ver_check();
phNxpNci_getConfig_info_t* mGetConfig_info = NULL;

/* global variable to get FW version from NCI response*/
uint32_t wFwVerRsp;
/* External global variable to get FW version */
extern uint16_t wFwVer;

extern uint16_t fw_maj_ver;
extern uint16_t rom_version;
extern uint8_t *gpFwBuffer;    // pointer to the buffer containing the new firmware
extern uint8_t *gpFwRecBuffer;    // pointer to the buffer containing the recovery firmware
extern uint8_t gRecFWDwnld;/* flag  set to true to  indicate dummy FW download*/
static uint8_t Rx_data[NCI_MAX_DATA_LEN];
static phNxpNciTransitSettings_t transitSettings;

uint32_t timeoutTimerId = 0;
phNxpNciHal_Sem_t config_data;

phNxpNciClock_t phNxpNciClock={0, {0}, FALSE};
phNxpNciRfSetting_t phNxpNciRfSet={FALSE, {0} };

static bool_t bFactoryResetRequired = FALSE;

/**************** local methods used in this file only ************************/
static void phNxpNciHal_open_complete(NFCSTATUS status);
static void phNxpNciHal_write_complete(void *pContext, phTmlNfc_TransactInfo_t *pInfo);
static void phNxpNciHal_read_complete(void *pContext, phTmlNfc_TransactInfo_t *pInfo);
static void phNxpNciHal_close_complete(NFCSTATUS status);
static void phNxpNciHal_core_initialized_complete(NFCSTATUS status);
#if PN547C2_DISABLE_DEAD_FUN
static void phNxpNciHal_pre_discover_complete(NFCSTATUS status);
#endif
static void phNxpNciHal_power_cycle_complete(NFCSTATUS status);
static void phNxpNciHal_client_task(void *arg);
static void phNxpNciHal_get_clk_freq(void);
static void phNxpNciHal_set_clock(void);
static void phNxpNciHal_check_factory_reset(void);
static NFCSTATUS phNxpNciHal_check_eSE_Session_Identity(void);
static void phNxpNciHal_print_res_status( uint8_t *p_rx_data, uint16_t *p_len);
static NFCSTATUS phNxpNciHal_CheckValidFwVersion(void);
NFCSTATUS phNxpNciHal_check_clock_config(void);
NFCSTATUS phNxpNciHal_china_tianjin_rf_setting(void);
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
static NFCSTATUS phNxpNciHalRFConfigCmdRecSequence(void);
#endif
static NFCSTATUS phNxpNciHal_CheckRFCmdRespStatus(void);
int  check_config_parameter(void);
/******************************************************************************
 * Function         phNxpNciHal_client_thread
 *
 * Description      This function is a thread handler which handles all TML and
 *                  NCI messages.
 *
 * Returns          void
 *
 ******************************************************************************/
static void phNxpNciHal_client_task(void *arg)
{
    phNxpNciHal_Control_t *p_nxpncihal_ctrl = (phNxpNciHal_Control_t *) arg;
    phLibNfc_Message_t msg;

    NXPLOG_NCIHAL_D(" client task started");

    p_nxpncihal_ctrl->thread_running = 1;
    for (;;){

    while (p_nxpncihal_ctrl->thread_running == 1)
    {
        /* Fetch next message from the NFC stack message queue */
        if (phDal4Nfc_msgrcv(p_nxpncihal_ctrl->gDrvCfg.nClientId,
                &msg, 0, 0) == -1)
        {
            NXPLOG_NCIHAL_E("NFC client received bad message");
            continue;
        }

        if(p_nxpncihal_ctrl->thread_running == 0){
            break;
        }

        switch (msg.eMsgType)
        {
            case PH_LIBNFC_DEFERREDCALL_MSG:
            {
                //NXPLOG_NCIHAL_E("NFC client received PH_LIBNFC_DEFERREDCALL_MSG");
                phLibNfc_DeferredCall_t *deferCall =
                        (phLibNfc_DeferredCall_t *) (msg.pMsgData);

                REENTRANCE_LOCK();
                deferCall->pCallback(deferCall->pParameter);
                REENTRANCE_UNLOCK();

            break;
        }

        case NCI_HAL_OPEN_CPLT_MSG:
        {
            NXPLOG_NCIHAL_E("NFC client received NCI_HAL_OPEN_CPLT_MSG");
            REENTRANCE_LOCK();
            if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
            {
                /* Send the event */
                (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_OPEN_CPLT_EVT,
                        HAL_NFC_STATUS_OK);
            }
            REENTRANCE_UNLOCK();
            break;
        }

        case NCI_HAL_CLOSE_CPLT_MSG:
        {
            NXPLOG_NCIHAL_E("NFC client received NCI_HAL_CLOSE_CPLT_MSG");
            REENTRANCE_LOCK();
            if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
            {
                 /* Send the event */
                (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_CLOSE_CPLT_EVT,
                        HAL_NFC_STATUS_OK);

                phOsalNfc_ProduceSemaphore(nxpncihal_ctrl.halClientSemaphore);
            }
            REENTRANCE_UNLOCK();
            break;
        }

        case NCI_HAL_POST_INIT_CPLT_MSG:
        {
            NXPLOG_NCIHAL_E("NFC client received NCI_HAL_POST_INIT_CPLT_MSG");
            REENTRANCE_LOCK();
            if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
            {
                if(phNxpNciHal_is_firmware_uptodate())
                {
                    /* Send the event */
                    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_POST_INIT_CPLT_EVT,
                            HAL_NFC_STATUS_OK);
                }
                else
                    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_POST_INIT_CPLT_EVT,
                            HAL_NFC_STATUS_INVALID_FW);
            }
            REENTRANCE_UNLOCK();
            break;
        }

        case NCI_HAL_PRE_DISCOVER_CPLT_MSG:
        {
            REENTRANCE_LOCK();
            if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
            {
                /* Send the event */
                (*nxpncihal_ctrl.p_nfc_stack_cback)(
                        HAL_NFC_PRE_DISCOVER_CPLT_EVT, HAL_NFC_STATUS_OK);
            }
            REENTRANCE_UNLOCK();
            break;
        }

        case NCI_HAL_ERROR_MSG:
        {
            NXPLOG_NCIHAL_E("NFC client received NCI_HAL_ERROR_MSG");
            REENTRANCE_LOCK();
            if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
            {
                /* Send the event */
                (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ERROR_EVT,
                        HAL_NFC_STATUS_FAILED);
            }
            REENTRANCE_UNLOCK();
            break;
        }

        case NCI_HAL_RX_MSG:
        {
            NXPLOG_NCIHAL_E("NFC client received NCI_HAL_RX_MSG");
            REENTRANCE_LOCK();
            if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL)
            {
                (*nxpncihal_ctrl.p_nfc_stack_data_cback)(
                        nxpncihal_ctrl.rsp_len, nxpncihal_ctrl.p_rsp_data);
            }
            REENTRANCE_UNLOCK();
            break;
        }
        }
    }

    NXPLOG_NCIHAL_D("NxpNciHal thread stopped");
    //phOsalNfc_Thread_Delete(NULL);
    }

    /*return;*/
}

#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
/******************************************************************************
 * Function         phNxpNciHal_fw_download
 *
 * Description      This function download the PN54X secure firmware to IC. If
 *                  firmware version in Android filesystem and firmware in the
 *                  IC is same then firmware download will return with success
 *                  without downloading the firmware.
 *
 * Returns          NFCSTATUS_SUCCESS if firmware download successful
 *                  NFCSTATUS_FAILED in case of failure
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_fw_download(BOOLEAN forcedDnd)
{
    NFCSTATUS status = NFCSTATUS_FAILED;

    phNxpNciHal_get_clk_freq();
    status = phTmlNfc_IoCtl(phTmlNfc_e_EnableDownloadMode);
    if (NFCSTATUS_SUCCESS == status)
    {
        /* Set the obtained device handle to download module */
        phDnldNfc_SetHwDevHandle();
        NXPLOG_NCIHAL_D("Calling Seq handler for FW Download \n");
        status = phNxpNciHal_fw_download_seq(nxpprofile_ctrl.bClkSrcVal, nxpprofile_ctrl.bClkFreqVal, forcedDnd);
        phDnldNfc_ReSetHwDevHandle();
    }
    else
    {
        status = NFCSTATUS_FAILED;
    }

    return status;
}
#endif

/******************************************************************************
 * Function         phNxpNciHal_CheckValidFwVersion
 *
 * Description      This function checks the valid FW for Mobile device.
 *                  If the FW doesn't belong the Mobile device it further
 *                  checks nxp config file to override.
 *
 * Returns          NFCSTATUS_SUCCESS if valid fw version found
 *                  NFCSTATUS_NOT_ALLOWED in case of FW not valid for mobile
 *                  device
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_CheckValidFwVersion(void)
{
    NFCSTATUS status = NFCSTATUS_NOT_ALLOWED;
    const unsigned char sfw_infra_major_no = 0x02;
    unsigned char ufw_current_major_no = 0x00;
    unsigned long num = 0;
    int isfound = 0;

    /* extract the firmware's major no */
    ufw_current_major_no = ((0x00FF) & (wFwVer >> 8U));

    NXPLOG_NCIHAL_D("%s current_major_no = 0x%x", __FUNCTION__,ufw_current_major_no );
#if(NFC_NXP_CHIP_TYPE == PN551)
    if (( ufw_current_major_no == FW_MOBILE_MAJOR_NUMBER_PN551) ||
        ( ufw_current_major_no == FW_MOBILE_MAJOR_NUMBER_PN48C2))
#else
    if( ufw_current_major_no == FW_MOBILE_MAJOR_NUMBER)
#endif
    {
        status = NFCSTATUS_SUCCESS;
    }
    else if (ufw_current_major_no == sfw_infra_major_no)
    {
        /* Check the nxp config file if still want to go for download */
        /* By default CFG_NXP_FW_PROTECION_OVERRIDE will not be defined in config file.
           If user really want to override the Infra firmware over mobile firmware, please
           put "NXP_FW_PROTECION_OVERRIDE=0x01" in libnfc-nxp.conf file.
           Please note once Infra firmware downloaded to Mobile device, The device
           can never be updated to Mobile firmware*/
        isfound = GetNxpNumValue(CFG_NXP_FW_PROTECION_OVERRIDE, &num, sizeof(num));
        if (isfound > 0)
        {
            if (num == 0x01)
            {
                NXPLOG_NCIHAL_D("Override Infra FW over Mobile");
                status = NFCSTATUS_SUCCESS;
            }
            else
            {
                NXPLOG_NCIHAL_D("Firmware download not allowed (NXP_FW_PROTECION_OVERRIDE invalid value)");
            }
        }
        else
        {
            NXPLOG_NCIHAL_D("Firmware download not allowed (NXP_FW_PROTECION_OVERRIDE not defined)");
        }
    }
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
    else if(gRecFWDwnld == TRUE)
    {
        status = NFCSTATUS_SUCCESS;
    }
#endif
    else if (wFwVerRsp == 0)
    {
        NXPLOG_NCIHAL_E("FW Version not received by NCI command >>> Force Firmware download");
        status = NFCSTATUS_SUCCESS;
    }
    else
    {
        NXPLOG_NCIHAL_E("Wrong FW Version >>> Firmware download not allowed");
    }

    return status;
}
NFCSTATUS phNxpNciHal_CheckValidFwVersionWrapper(void)
{
	return phNxpNciHal_CheckValidFwVersion();
}
/******************************************************************************
 * Function         phNxpNciHal_is_factory_reset_required
 *
 * Description      This Function sets Flag bFactoryResetRequired
 *
 * Returns          None
 *
 */
void phNxpNciHal_is_factory_reset_required(bool_t factoryReset)
{
    bFactoryResetRequired = factoryReset;
}
/*******************************************************************************
 **
 ** Function:        phNxpNciHal_is_firmware_uptodate()
 **
 ** Description:     Checks if the Firmware in NFCC is the expected version
 **
 ** Returns:         TRUE or FALSE
 **
 ********************************************************************************/
bool_t phNxpNciHal_is_firmware_uptodate(void)
{
	#if(NFC_NXP_CHIP_TYPE == PN553)
    return ((wFwVerRsp & 0x00FFFFFF) == EXPECTED_FW_VERSION) ?TRUE:FALSE;
    #else
    return ((wFwVerRsp & 0x0000FFFF) == EXPECTED_FW_VERSION) ?TRUE:FALSE;
    #endif
}
static void phNxpNciHal_get_clk_freq(void)
{
    unsigned long num = 0;
    int isfound = 0;

    nxpprofile_ctrl.bClkSrcVal = 0;
    nxpprofile_ctrl.bClkFreqVal = 0;
    nxpprofile_ctrl.bTimeout = 0;

    isfound = GetNxpNumValue(CFG_NXP_SYS_CLK_SRC_SEL, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.bClkSrcVal = num;
    }

    num = 0;
    isfound = 0;
    isfound = GetNxpNumValue(CFG_NXP_SYS_CLK_FREQ_SEL, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.bClkFreqVal = num;
    }

    num = 0;
    isfound = 0;
    isfound = GetNxpNumValue(CFG_NXP_SYS_CLOCK_TO_CFG, &num, sizeof(num));
    if (isfound > 0)
    {
        nxpprofile_ctrl.bTimeout = num;
    }

    NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkSrcVal = 0x%x", nxpprofile_ctrl.bClkSrcVal);
    NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bClkFreqVal = 0x%x", nxpprofile_ctrl.bClkFreqVal);
    NXPLOG_FWDNLD_D("gphNxpNciHal_fw_IoctlCtx.bTimeout = 0x%x", nxpprofile_ctrl.bTimeout);

    if ((nxpprofile_ctrl.bClkSrcVal != CLK_SRC_XTAL) && (nxpprofile_ctrl.bClkSrcVal != CLK_SRC_PLL))
    {
        NXPLOG_FWDNLD_E("Clock source value is wrong in config file, setting it as default");
        nxpprofile_ctrl.bClkSrcVal = NXP_SYS_CLK_SRC_SEL;
    }
    /*Changed part of warning fixes in wearable project*/
    if ((nxpprofile_ctrl.bClkSrcVal == CLK_SRC_PLL) && ((nxpprofile_ctrl.bClkFreqVal < CLK_FREQ_13MHZ) ||
            (nxpprofile_ctrl.bClkFreqVal > CLK_FREQ_52MHZ)))
    {
        NXPLOG_FWDNLD_E("Clock frequency value is wrong in config file, setting it as default");
        nxpprofile_ctrl.bClkFreqVal = NXP_SYS_CLK_FREQ_SEL;
    }
    if ((nxpprofile_ctrl.bTimeout < CLK_TO_CFG_DEF) || (nxpprofile_ctrl.bTimeout > CLK_TO_CFG_MAX))
    {
        NXPLOG_FWDNLD_E("Clock timeout value is wrong in config file, setting it as default");
        nxpprofile_ctrl.bTimeout = CLK_TO_CFG_DEF;
    }

}
void phNxpNciHal_get_clk_freq_wrapper()
{
	phNxpNciHal_get_clk_freq();
}

#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
/******************************************************************************
 * Function         phNxpNciHal_FwDwnld
 *
 * Description      This function is called by libnfc-nci after core init is
 *                  completed, to download the firmware.
 *
 * Returns          This function return NFCSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_FwDwnld(BOOLEAN forcedDnd)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS, wConfigStatus = NFCSTATUS_SUCCESS;
    if (wFwVerRsp == 0)
    {
        status = phDnldNfc_InitImgInfo();
        if(NFCSTATUS_SUCCESS != status)
        {
            phDnldNfc_ReSetHwDevHandle();
            return status;
        }
    }
    status= phNxpNciHal_CheckValidFwVersion();
    if (NFCSTATUS_SUCCESS == status)
    {
        NXPLOG_NCIHAL_D ("FW update required");
        GeneralState |= FWDNLD_MASK;
        status = phNxpNciHal_fw_download(forcedDnd);
        GeneralState &= ~FWDNLD_MASK;
        if (status != NFCSTATUS_SUCCESS)
        {
            if (NFCSTATUS_SUCCESS != phNxpNciHal_fw_mw_ver_check())
            {
                NXPLOG_NCIHAL_D ("Chip Version Middleware Version mismatch!!!!");
                goto clean_and_return;
            }
            NXPLOG_NCIHAL_E ("FW Download failed - NFCC init will continue");
        }

        /* call read pending */
        wConfigStatus = phTmlNfc_Read(
                nxpncihal_ctrl.p_cmd_data,
                NCI_MAX_DATA_LEN,
                (pphTmlNfc_TransactCompletionCb_t) &phNxpNciHal_read_complete,
                NULL);
        if (wConfigStatus != NFCSTATUS_PENDING)
        {
            NXPLOG_NCIHAL_E("TML Read status error status = %x", status);
            phTmlNfc_Shutdown();
            status = NFCSTATUS_FAILED;
        }
    }
    else
    {
        if (wFwVerRsp == 0)
            phDnldNfc_ReSetHwDevHandle();

    }
    clean_and_return:
    return status;
}
#endif

/******************************************************************************
 * Function         phNxpNciHal_open
 *
 * Description      This function is called by libnfc-nci during the
 *                  initialization of the NFCC. It opens the physical connection
 *                  with NFCC (PN54X) and creates required client thread for
 *                  operation.
 *                  After open is complete, status is informed to libnfc-nci
 *                  through callback function.
 *
 * Returns          This function return NFCSTATUS_SUCCES (0) in case of success
 *                  In case of failure returns other failure value.
 *
 ******************************************************************************/
int phNxpNciHal_open(nfc_stack_callback_t *p_cback, nfc_stack_data_callback_t *p_data_cback)
{
    phOsalNfc_Config_t tOsalConfig;
    phTmlNfc_Config_t tTmlConfig;
    NFCSTATUS wConfigStatus = NFCSTATUS_SUCCESS;
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    /*Changed part of warning fixes in wearable project*/
    /*NCI_INIT_CMD*/
    //static uint8_t cmd_init_nci[] = {0x20,0x01,0x00};
    /*NCI_RESET_CMD*/
   // static uint8_t cmd_reset_nci[] = {0x20,0x00,0x01,0x00};
    //int init_retry_cnt=0;
    phOsalNfc_ThreadCreationParams_t threadparams;
#ifdef DEBUG
    /* initialize trace level */
    phNxpLog_InitializeLogLevel();
#endif
    /*Create the timer for extns write response*/
    timeoutTimerId = phOsalNfc_Timer_Create(FALSE);
    if(timeoutTimerId == PH_OSALNFC_TIMER_ID_INVALID)
    {
        NXPLOG_NCIHAL_E("Invalid Timer Id, Timer Creation failed");
        return NFCSTATUS_FAILED;
    }

    if (phNxpNciHal_init_monitor() == NULL)
    {
        NXPLOG_NCIHAL_E("Init monitor failed");
        return NFCSTATUS_FAILED;
    }

    CONCURRENCY_LOCK();

    phOsalNfc_SetMemory(&nxpncihal_ctrl, 0x00, sizeof(nxpncihal_ctrl));
    phOsalNfc_SetMemory(&tOsalConfig, 0x00, sizeof(tOsalConfig));
    phOsalNfc_SetMemory(&tTmlConfig, 0x00, sizeof(tTmlConfig));
    phOsalNfc_SetMemory (&nxpprofile_ctrl, 0, sizeof(phNxpNciProfile_Control_t));

    status = phOsalNfc_CreateSemaphore(&nxpncihal_ctrl.halClientSemaphore, 0);
    if(status != NFCSTATUS_SUCCESS)
        return status;

    /* By default HAL status is HAL_STATUS_OPEN */
    nxpncihal_ctrl.halStatus = HAL_STATUS_OPEN;

    nxpncihal_ctrl.p_nfc_stack_cback = p_cback;
    nxpncihal_ctrl.p_nfc_stack_data_cback = p_data_cback;

    /*structure related to set config management
    Initialising pointer variable
     */
    mGetConfig_info = NULL;
    mGetConfig_info =  phOsalNfc_GetMemory(sizeof(phNxpNci_getConfig_info_t));
    if(mGetConfig_info == NULL)
    {
        goto clean_and_return;
    }
    phOsalNfc_SetMemory(mGetConfig_info,0x00,sizeof(phNxpNci_getConfig_info_t));

    /* Configure hardware link */
#ifdef ANDRIOD
    nxpncihal_ctrl.gDrvCfg.nClientId = phDal4Nfc_msgget(0, 0600);
#else
    nxpncihal_ctrl.gDrvCfg.nClientId = phDal4Nfc_msgget();
#endif
    nxpncihal_ctrl.gDrvCfg.nLinkType = ENUM_LINK_TYPE_I2C;/* For PN54X */
    tTmlConfig.pDevName = (int8_t *) "/dev/pn544";
    tOsalConfig.dwCallbackTaskHandle
    = (uintptr_t) nxpncihal_ctrl.gDrvCfg.nClientId;
    tOsalConfig.pLogFile = NULL;
    tTmlConfig.dwGetMsgTaskHandle = (uintptr_t) nxpncihal_ctrl.gDrvCfg.nClientId;

    /* Initialize TML layer */
    wConfigStatus = phTmlNfc_Init(&tTmlConfig);
    if (wConfigStatus != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("phTmlNfc_Init Failed");
        goto clean_and_return;
    }

    threadparams.stackdepth = CLIENT_TASK_STACK_SIZE;
    strcpy((char*)threadparams.taskname, "client task");
    threadparams.pContext = &nxpncihal_ctrl;
    threadparams.priority = 4;
    /* Create the client thread */
    if(phOsalNfc_Thread_Create(&nxpncihal_ctrl.client_task, phNxpNciHal_client_task, &threadparams) != 0)
    {
        NXPLOG_NCIHAL_E("Task Creation failed");
        wConfigStatus = phTmlNfc_Shutdown();
        goto clean_and_return;
    }

    if (nxpncihal_ctrl.client_task == NULL)
    {
        NXPLOG_NCIHAL_E("xTaskCreate failed");
        wConfigStatus = phTmlNfc_Shutdown();
        goto clean_and_return;
    }

    CONCURRENCY_UNLOCK();

    /* call read pending */
    status = phTmlNfc_Read(
            nxpncihal_ctrl.p_cmd_data,
            NCI_MAX_DATA_LEN,
            (pphTmlNfc_TransactCompletionCb_t) &phNxpNciHal_read_complete,
            NULL);
    if (status != NFCSTATUS_PENDING)
    {
        NXPLOG_NCIHAL_E("TML Read status error status = %x", status);
        wConfigStatus = phTmlNfc_Shutdown();
        wConfigStatus = NFCSTATUS_FAILED;
        goto clean_and_return;
    }

	wFwVerRsp = 0;
    phNxpNciHal_ext_init();

    /* Call open complete */
    phNxpNciHal_open_complete(wConfigStatus);

    return wConfigStatus;

    clean_and_return:
    CONCURRENCY_UNLOCK();
    if(mGetConfig_info != NULL)
    {
       phOsalNfc_FreeMemory(mGetConfig_info);
       mGetConfig_info = NULL;
    }
    /* Report error status */
    (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_OPEN_CPLT_EVT,
            HAL_NFC_STATUS_FAILED);

    nxpncihal_ctrl.p_nfc_stack_cback = NULL;
    nxpncihal_ctrl.p_nfc_stack_data_cback = NULL;
    phNxpNciHal_cleanup_monitor();
    nxpncihal_ctrl.halStatus = HAL_STATUS_CLOSE;
    return NFCSTATUS_FAILED;
}

int phNxpNciHal_keep_read_pending(void)
{
	return phTmlNfc_Read(nxpncihal_ctrl.p_cmd_data,
								NCI_MAX_DATA_LEN,
								(pphTmlNfc_TransactCompletionCb_t) &phNxpNciHal_read_complete,
								NULL);
}

/******************************************************************************
 * Function         phNxpNciHal_fw_mw_check
 *
 * Description      This function inform the status of phNxpNciHal_fw_mw_check
 *                  function to libnfc-nci.
 *
 * Returns          int.
 *
 ******************************************************************************/
int phNxpNciHal_fw_mw_ver_check()
{
    NFCSTATUS status = NFCSTATUS_FAILED;

    if((!(strcmp(COMPILATION_MW,"PN553")) && (rom_version==0x11) && (fw_maj_ver == 0x01)))
    {
        status = NFCSTATUS_SUCCESS;
    }
    else if((!(strcmp(COMPILATION_MW,"PN551")) && (rom_version==0x10) && ((fw_maj_ver == 0x03) || (fw_maj_ver == 0x01))))
    {
        status = NFCSTATUS_SUCCESS;
    }
    else if((!(strcmp(COMPILATION_MW,"PN548AD")) && (rom_version==0x10) && (fw_maj_ver == 0x01)))
    {
        status = NFCSTATUS_SUCCESS;
    }
    else if((!(strcmp(COMPILATION_MW,"PN547C2")) && (rom_version==0x08) && (fw_maj_ver == 0x01)))
    {
        status = NFCSTATUS_SUCCESS;
    }
    return status;
}
/******************************************************************************
 * Function         phNxpNciHal_open_complete
 *
 * Description      This function inform the status of phNxpNciHal_open
 *                  function to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_open_complete(NFCSTATUS status)
{
    static phLibNfc_Message_t msg;

    if (status == NFCSTATUS_SUCCESS)
    {
        msg.eMsgType = NCI_HAL_OPEN_CPLT_MSG;
        nxpncihal_ctrl.hal_open_status = TRUE;
    }
    else
    {
        msg.eMsgType = NCI_HAL_ERROR_MSG;
    }

    msg.pMsgData = NULL;
    msg.Size = 0;
    NXPLOG_NCIHAL_E ("phNxpNciHal_open_complete Posting 0x%x", msg.eMsgType);
    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
            (phLibNfc_Message_t *) &msg);

    return;
}

/******************************************************************************
 * Function         phNxpNciHal_write
 *
 * Description      This function write the data to NFCC through physical
 *                  interface (e.g. I2C) using the PN54X driver interface.
 *                  Before sending the data to NFCC, phNxpNciHal_write_ext
 *                  is called to check if there is any extension processing
 *                  is required for the NCI packet being sent out.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
int phNxpNciHal_write(uint16_t data_len, const uint8_t *p_data)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    static phLibNfc_Message_t msg;

    /* Create local copy of cmd_data */
    phOsalNfc_MemCopy(nxpncihal_ctrl.p_cmd_data, p_data, data_len);
    nxpncihal_ctrl.cmd_len = data_len;
    if(nxpncihal_ctrl.cmd_len > NCI_MAX_DATA_LEN)
    {
        NXPLOG_NCIHAL_D ("cmd_len exceeds limit NCI_MAX_DATA_LEN");
        goto clean_and_return;
    }
#ifdef P2P_PRIO_LOGIC_HAL_IMP
    /* Specific logic to block RF disable when P2P priority logic is busy */
    if (p_data[0] == 0x21&&
        p_data[1] == 0x06 &&
        p_data[2] == 0x01 &&
        EnableP2P_PrioLogic == TRUE)
    {
        NXPLOG_NCIHAL_D ("P2P priority logic busy: Disable it.");
        phNxpNciHal_clean_P2P_Prio();
    }
#endif

    /* Check for NXP ext before sending write */
    status = phNxpNciHal_write_ext(&nxpncihal_ctrl.cmd_len,
            nxpncihal_ctrl.p_cmd_data, &nxpncihal_ctrl.rsp_len,
            nxpncihal_ctrl.p_rsp_data);
    if (status != NFCSTATUS_SUCCESS)
    {
        /* Do not send packet to PN54X, send response directly */
        msg.eMsgType = NCI_HAL_RX_MSG;
        msg.pMsgData = NULL;
        msg.Size = 0;

        phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
                (phLibNfc_Message_t *) &msg);
        goto clean_and_return;
    }

    CONCURRENCY_LOCK();
    data_len = phNxpNciHal_write_unlocked(nxpncihal_ctrl.cmd_len,
            nxpncihal_ctrl.p_cmd_data);
    CONCURRENCY_UNLOCK();

    if (icode_send_eof == 1)
    {

        phOsalNfc_Delay(10);
        icode_send_eof = 2;
        phNxpNciHal_send_ext_cmd (3, cmd_icode_eof);
    }

    clean_and_return:
    /* No data written */
    return data_len;
}

/******************************************************************************
 * Function         phNxpNciHal_write_unlocked
 *
 * Description      This is the actual function which is being called by
 *                  phNxpNciHal_write. This function writes the data to NFCC.
 *                  It waits till write callback provide the result of write
 *                  process.
 *
 * Returns          It returns number of bytes successfully written to NFCC.
 *
 ******************************************************************************/
//static phNxpNciHal_Sem_t cb_data;
int phNxpNciHal_write_unlocked(uint16_t data_len, const uint8_t *p_data)
{
    NFCSTATUS status = NFCSTATUS_INVALID_PARAMETER;
    phNxpNciHal_Sem_t cb_data;
    nxpncihal_ctrl.retry_cnt = 0;
    static uint8_t reset_ntf[] = {0x60, 0x00, 0x06, 0xA0, 0x00, 0xC7, 0xD4, 0x00, 0x00};

    /* Create the local semaphore */
    if (phNxpNciHal_init_cb_data(&cb_data, NULL) != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_write_unlocked Create cb data failed");
        data_len = 0;
        goto clean_and_return;
    }

    /* Create local copy of cmd_data */
    phOsalNfc_MemCopy(nxpncihal_ctrl.p_cmd_data, p_data, data_len);
    nxpncihal_ctrl.cmd_len = data_len;

    retry:

    data_len = nxpncihal_ctrl.cmd_len;

    status = phTmlNfc_Write( (uint8_t *) nxpncihal_ctrl.p_cmd_data,
            (uint16_t) nxpncihal_ctrl.cmd_len,
            (pphTmlNfc_TransactCompletionCb_t) &phNxpNciHal_write_complete,
            (void *) &cb_data);
    if (status != NFCSTATUS_PENDING)
    {
        NXPLOG_NCIHAL_E("write_unlocked status error");
        data_len = 0;
        goto clean_and_return;
    }

    /* Wait for callback response */
	//changed as SEM_WAIT is mapped to phOsalNfc_ConsumeSemaphore which return 0 for success
    if ((SEM_WAIT(cb_data)))
    {
        NXPLOG_NCIHAL_E("write_unlocked semaphore error");
        data_len = 0;
        goto clean_and_return;
    }

    if (cb_data.status != NFCSTATUS_SUCCESS)
    {
        data_len = 0;
        if(nxpncihal_ctrl.retry_cnt++ < MAX_RETRY_COUNT)
        {
            NXPLOG_NCIHAL_E("write_unlocked failed - PN54X Maybe in Standby Mode - Retry");
            /* 1ms delay to give NFCC wake up delay */
            phOsalNfc_Delay(1);
            goto retry;
        }
        else
        {

            NXPLOG_NCIHAL_E("write_unlocked failed - PN54X Maybe in Standby Mode (max count = 0x%x)", nxpncihal_ctrl.retry_cnt);

            status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);

            if(NFCSTATUS_SUCCESS == status)
            {
                NXPLOG_NCIHAL_D("PN54X Reset - SUCCESS\n");
            }
            else
            {
                NXPLOG_NCIHAL_D("PN54X Reset - FAILED\n");
            }
            if (nxpncihal_ctrl.p_nfc_stack_data_cback!= NULL &&
                nxpncihal_ctrl.hal_open_status == TRUE)
            {
                if(nxpncihal_ctrl.p_rx_data!= NULL)
                {
                    NXPLOG_NCIHAL_D("Send the Core Reset NTF to upper layer, which will trigger the recovery\n");
                    //Send the Core Reset NTF to upper layer, which will trigger the recovery.
                    nxpncihal_ctrl.rx_data_len = sizeof(reset_ntf);
                    phOsalNfc_MemCopy(nxpncihal_ctrl.p_rx_data, reset_ntf, sizeof(reset_ntf));
                    (*nxpncihal_ctrl.p_nfc_stack_data_cback)(nxpncihal_ctrl.rx_data_len, nxpncihal_ctrl.p_rx_data);
                }
                else
                {
                    (*nxpncihal_ctrl.p_nfc_stack_data_cback)(0x00, NULL);
                }
                write_unlocked_status = NFCSTATUS_FAILED;
            }
        }
    }
    else
    {
        write_unlocked_status = NFCSTATUS_SUCCESS;
    }

    clean_and_return:
    phNxpNciHal_cleanup_cb_data(&cb_data);
    return data_len;
}

/******************************************************************************
 * Function         phNxpNciHal_write_complete
 *
 * Description      This function handles write callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_write_complete(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    phNxpNciHal_Sem_t *p_cb_data = (phNxpNciHal_Sem_t*) pContext;

    if (pInfo->wStatus == NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("write successful status = 0x%x", pInfo->wStatus);
    }
    else
    {
        NXPLOG_NCIHAL_E("write error status = 0x%x", pInfo->wStatus);
    }

    p_cb_data->status = pInfo->wStatus;

    SEM_POST(p_cb_data);

    return;
}

/******************************************************************************
 * Function         phNxpNciHal_read_complete
 *
 * Description      This function is called whenever there is an NCI packet
 *                  received from NFCC. It could be RSP or NTF packet. This
 *                  function provide the received NCI packet to libnfc-nci
 *                  using data callback of libnfc-nci.
 *                  There is a pending read called from each
 *                  phNxpNciHal_read_complete so each a packet received from
 *                  NFCC can be provide to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_read_complete(void *pContext, phTmlNfc_TransactInfo_t *pInfo)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    UNUSED(pContext);
    if(nxpncihal_ctrl.read_retry_cnt == 1)
    {
        nxpncihal_ctrl.read_retry_cnt = 0;
    }
    if(pInfo->wStatus == NFCSTATUS_READ_FAILED)
    {
        if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
        {
            read_failed_disable_nfc = TRUE;
            /* Send the event */
            (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ERROR_EVT,
                    HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
        }
        return;
    }

    if (pInfo->wStatus == NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_D("read successful status = 0x%x", pInfo->wStatus);

        nxpncihal_ctrl.p_rx_data = pInfo->pBuff;
        nxpncihal_ctrl.rx_data_len = pInfo->wLength;

        status = phNxpNciHal_process_ext_rsp (nxpncihal_ctrl.p_rx_data, &nxpncihal_ctrl.rx_data_len);

        phNxpNciHal_print_res_status(nxpncihal_ctrl.p_rx_data, &nxpncihal_ctrl.rx_data_len);
        /* Check if response should go to hal module only */
        if (nxpncihal_ctrl.hal_ext_enabled == 1
                && (nxpncihal_ctrl.p_rx_data[0x00] & 0xF0) == 0x40)
        {
            if(status == NFCSTATUS_FAILED)
            {
                NXPLOG_NCIHAL_D("enter into NFCC init recovery");
                nxpncihal_ctrl.ext_cb_data.status = status;
            }
            /* Unlock semaphore only for responses*/
            if((nxpncihal_ctrl.p_rx_data[0x00] & 0xF0) == 0x40 ||
               ((icode_detected == TRUE) &&(icode_send_eof == 3)))
            {
                /* Unlock semaphore */
                SEM_POST(&(nxpncihal_ctrl.ext_cb_data));
            }
        }
        /* Read successful send the event to higher layer */
        else if ((nxpncihal_ctrl.p_nfc_stack_data_cback != NULL) &&
                (status == NFCSTATUS_SUCCESS) )
        {
            (*nxpncihal_ctrl.p_nfc_stack_data_cback)(
                    nxpncihal_ctrl.rx_data_len, nxpncihal_ctrl.p_rx_data);
        }
    }
    else
    {
        NXPLOG_NCIHAL_E("read error status = 0x%x", pInfo->wStatus);
    }

    if(nxpncihal_ctrl.halStatus == HAL_STATUS_CLOSE)
    {
        return;
    }
    /* Read again because read must be pending always.*/
    status = phTmlNfc_Read(
            Rx_data,
            NCI_MAX_DATA_LEN,
            (pphTmlNfc_TransactCompletionCb_t) &phNxpNciHal_read_complete,
            NULL);
    if (status != NFCSTATUS_PENDING)
    {
        NXPLOG_NCIHAL_E("read status error status = %x", status);
    }

    return;
}

/******************************************************************************
 * Function         phNxpNciHal_reset_nfcc_fw
 *
 * Description      This function is called to reset nfcc fw for recovery or after applying configs
 *
 * Returns          Status.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_reset_nfcc_fw(void)
{
    /*NCI_INIT_CMD*/
    static uint8_t cmd_init_nci[] = {0x20,0x01,0x00};
    /*NCI_RESET_CMD*/
    static uint8_t cmd_reset_nci[] = {0x20,0x00,0x01,0x00}; //keep configuration
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci),cmd_reset_nci);
    if(status == NFCSTATUS_SUCCESS )
    {
        if(phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci),cmd_init_nci) != NFCSTATUS_SUCCESS)
            return NFCSTATUS_FAILED;
    }
    return status;
}
/******************************************************************************
 * Function         phNxpNciHal_core_initialized
 *
 * Description      This function is called by libnfc-nci after successful open
 *                  of NFCC. All proprietary setting for PN54X are done here.
 *                  After completion of proprietary settings notification is
 *                  provided to libnfc-nci through callback function.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_core_initialized(uint8_t* p_core_init_rsp_params)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    /*static uint8_t p2p_listen_mode_routing_cmd[] = { 0x21, 0x01, 0x07, 0x00, 0x01,
                                                0x01, 0x03, 0x00, 0x01, 0x05 };*/ //unused
/* Wearable does not have UICC.*/
#ifdef ANDROID
    uint8_t swp_full_pwr_mode_on_cmd[] = { 0x20, 0x02, 0x05, 0x01, 0xA0,
                                           0xF1,0x01,0x01 };

    static uint8_t swp_switch_timeout_cmd[] = {0x20, 0x02, 0x06, 0x01, 0xA0, 0xF3, 0x02, 0x00, 0x00};
#endif

    uint8_t *buffer = NULL;
    long bufflen = 260;
    long retlen = 0;
    int isfound;
    uint8_t jcopstate = NCI_HAL_OSU_STARTED;

    //EEPROM access variables
    phNxpNci_EEPROM_info_t mEEPROM_info = {0};
    uint8_t fw_dwnld_flag = FALSE, setConfigAlways = FALSE, isCorrectFwVersion = FALSE;

    NXPLOG_NCIHAL_D ("phNxpNciHal_core_initialized");

    /* Temp fix to re-apply the proper clock setting */
    // int temp_fix = 1;// unused
    unsigned long num = 0;
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
    //initialize dummy FW recovery variables
    gRecFwRetryCount = 0;
    gRecFWDwnld = FALSE;
#endif
    // recovery --start
    /*NCI_INIT_CMD*/
    static uint8_t cmd_init_nci[] = {0x20,0x01,0x00};
    /*NCI_RESET_CMD*/
    static uint8_t cmd_reset_nci[] = {0x20,0x00,0x01,0x00}; //keep configuration
    /* reset config cache */
    static uint8_t retry_core_init_cnt;

    if((*p_core_init_rsp_params > 0) && (*p_core_init_rsp_params < 4)) //initializing for recovery.
    {
retry_core_init:
        config_access = FALSE;
        if(mGetConfig_info != NULL)
            mGetConfig_info->isGetcfg = FALSE;
        if(buffer != NULL)
        {
            phOsalNfc_FreeMemory(buffer);
            buffer = NULL;
        }
        if(retry_core_init_cnt > 3)
        {
            if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
            {
                NXPLOG_NCIHAL_D("Posting Core Init Failed\n");
                read_failed_disable_nfc = TRUE;
                /* Send the event */
                (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ERROR_EVT,
                        HAL_NFC_STATUS_ERR_CMD_TIMEOUT);
            }
            return NFCSTATUS_FAILED;
        }

        status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);
        if(NFCSTATUS_SUCCESS == status) { NXPLOG_NCIHAL_D("PN547 Reset - SUCCESS\n"); }
        else { NXPLOG_NCIHAL_D("PN547 Reset - FAILED\n"); }

        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci),cmd_reset_nci);
        if((status != NFCSTATUS_SUCCESS) && (nxpncihal_ctrl.retry_cnt >= MAX_RETRY_COUNT))
        {
            NXPLOG_NCIHAL_E("Force FW Download, NFCC not coming out from Standby");
            retry_core_init_cnt++;
            goto retry_core_init;
        }
        else if(status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E ("NCI_CORE_RESET: Failed");
            retry_core_init_cnt++;
            goto retry_core_init;

        }

        if(*p_core_init_rsp_params == 2) {
            NXPLOG_NCIHAL_E(" Last command is CORE_RESET!!");
            goto invoke_callback;
        }

        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_init_nci),cmd_init_nci);
        if(status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E ("NCI_CORE_INIT : Failed");
            retry_core_init_cnt++;
            goto retry_core_init;
        }

        if(*p_core_init_rsp_params == 3) {
            NXPLOG_NCIHAL_E(" Last command is CORE_INIT!!");
            goto invoke_callback;
        }
        goto recovery_wiredmode;
    }
// recovery --end


    buffer = (uint8_t*) phOsalNfc_GetMemory(bufflen*sizeof(uint8_t));
    if(NULL == buffer)
    {
        return NFCSTATUS_FAILED;
    }

    config_access = TRUE;
    retlen = 0;
    isfound = GetNxpByteArrayValue(CFG_NXP_ACT_PROP_EXTN, (char *) buffer,
            bufflen, &retlen);
    if (retlen > 0) {
        /* NXP ACT Proprietary Ext */
        status = phNxpNciHal_send_ext_cmd(retlen, buffer);
        if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("NXP ACT Proprietary Ext failed");
            retry_core_init_cnt++;
            goto retry_core_init;
        }
    }

    retlen = 0;

    isfound = GetNxpByteArrayValue(CFG_NXP_CORE_STANDBY, (char *) buffer,bufflen, &retlen);
    if (retlen > 0) {
        /* NXP ACT Proprietary Ext */
        status = phNxpNciHal_send_ext_cmd(retlen, buffer);
        if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("Stand by mode enable failed");
            retry_core_init_cnt++;
            goto retry_core_init;
        }
    }

    /*If session file is not present set the SWP_INT_SESSION_ID_CFG to FF to
      force the NFCEE to re-run its initialization sequence*/
    if(bFactoryResetRequired == TRUE)
    {
        phNxpNciHal_check_factory_reset();
        bFactoryResetRequired = FALSE;
    }

    config_access = TRUE;
    retlen = 0;
    status = phNxpNciHal_check_clock_config();
    if (status != NFCSTATUS_SUCCESS) {
        NXPLOG_NCIHAL_E("phNxpNciHal_check_clock_config failed");
        retry_core_init_cnt++;
        goto retry_core_init;
    }

    #ifdef PN547C2_CLOCK_SETTING
    if ((phNxpNciClock.issetConfig)
    #if(NFC_NXP_HFO_SETTINGS == TRUE)
        || temp_fix == 1
    #endif
        )
    {
        phNxpNciHal_set_clock();
        phNxpNciClock.issetConfig = FALSE;
#if(NFC_NXP_HFO_SETTINGS == TRUE)
        if (temp_fix == 1 )
        {
            NXPLOG_NCIHAL_D("Applying Default Clock setting and DPLL register at power on");
            /*
            # A0, 0D, 06, 06, 83, 55, 2A, 04, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_GEAR_REG
            # A0, 0D, 06, 06, 82, 33, 14, 17, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_INIT_REG
            # A0, 0D, 06, 06, 84, AA, 85, 00, 80 RF_CLIF_CFG_TARGET CLIF_DPLL_INIT_FREQ_REG
            # A0, 0D, 06, 06, 81, 63, 00, 00, 00 RF_CLIF_CFG_TARGET CLIF_DPLL_CONTROL_REG
            */
            static uint8_t cmd_dpll_set_reg_nci[] = {0x20, 0x02, 0x25, 0x04,
                                                                            0xA0, 0x0D, 0x06, 0x06, 0x83, 0x55, 0x2A, 0x04, 0x00,
                                                                            0xA0, 0x0D, 0x06, 0x06, 0x82, 0x33, 0x14, 0x17, 0x00,
                                                                            0xA0, 0x0D, 0x06, 0x06, 0x84, 0xAA, 0x85, 0x00, 0x80,
                                                                            0xA0, 0x0D, 0x06, 0x06, 0x81, 0x63, 0x00, 0x00, 0x00};

            status = phNxpNciHal_send_ext_cmd(sizeof(cmd_dpll_set_reg_nci), cmd_dpll_set_reg_nci);
            if (status != NFCSTATUS_SUCCESS) {
                NXPLOG_NCIHAL_E("NXP DPLL REG ACT Proprietary Ext failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }
            /* reset the NFCC after applying the clock setting and DPLL setting */
            //phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);
            temp_fix = 0;
            goto retry_core_init;
        }
#endif
    }
#endif
    if(!((*p_core_init_rsp_params > 0) && (*p_core_init_rsp_params < 4)))
    {
        isfound = GetNxpByteArrayValue(CFG_NXP_NFC_PROFILE_EXTN, (char *) buffer,bufflen, &retlen);
            if (retlen > 0) {
                /* NXP ACT Proprietary Ext */
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("NXP ACT Proprietary Ext failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }

        retlen = 0;
        num = 0;
        NXPLOG_NCIHAL_D ("Performing TVDD Settings");
        isfound = GetNxpNumValue(CFG_NXP_EXT_TVDD_CFG, &num, sizeof(num));
        if (isfound > 0)
        {
            if(num == 1) {
                isfound = GetNxpByteArrayValue(CFG_NXP_EXT_TVDD_CFG_1, (char *) buffer,bufflen, &retlen);
                if (retlen > 0) {
                    status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                        if (status != NFCSTATUS_SUCCESS) {
                            NXPLOG_NCIHAL_E("EXT TVDD CFG 1 Settings failed");
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                }
            }
            else if(num == 2) {
                isfound = GetNxpByteArrayValue(CFG_NXP_EXT_TVDD_CFG_2, (char *) buffer,
                        bufflen, &retlen);
                    if (retlen > 0) {
                    status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                    if (status != NFCSTATUS_SUCCESS) {
                        NXPLOG_NCIHAL_E("EXT TVDD CFG 2 Settings failed");
                        retry_core_init_cnt++;
                        goto retry_core_init;
                    }
                }
            }
            else if(num == 3) {
                isfound = GetNxpByteArrayValue(CFG_NXP_EXT_TVDD_CFG_3, (char *) buffer,
                        bufflen, &retlen);
                    if (retlen > 0) {
                    status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                    if (status != NFCSTATUS_SUCCESS) {
                        NXPLOG_NCIHAL_E("EXT TVDD CFG 3 Settings failed");
                        retry_core_init_cnt++;
                        goto retry_core_init;
                    }
                }
            }
            else {
                NXPLOG_NCIHAL_E("Wrong Configuration Value %ld", num);
            }

        }
        retlen = 0;
        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
        {
            config_access = TRUE;
        }

        NXPLOG_NCIHAL_D ("Performing CFG_NXP_CORE_CONF_EXTN Settings");
        isfound = GetNxpByteArrayValue(CFG_NXP_CORE_CONF_EXTN,
                (char *) buffer, bufflen, &retlen);
        if (retlen > 0) {
            /* NXP ACT Proprietary Ext */
            status = phNxpNciHal_send_ext_cmd(retlen, buffer);
            if (status != NFCSTATUS_SUCCESS) {
                NXPLOG_NCIHAL_E("NXP Core configuration failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }
        }
        retlen = 0;

        NXPLOG_NCIHAL_D ("Performing CFG_NXP_CORE_CONF Settings");
        isfound =  GetNxpByteArrayValue(CFG_NXP_CORE_CONF,(char *)buffer,bufflen,&retlen);
        if(retlen > 0)
        {
            /* NXP ACT Proprietary Ext */
            status = phNxpNciHal_send_ext_cmd(retlen,buffer);
            if (status != NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("Core Set Config failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }
        }

        retlen = 0;
        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
        {
            config_access = FALSE;
        }

        isfound = GetNxpByteArrayValue(CFG_NXP_CORE_RF_FIELD,
                (char *) buffer, bufflen, &retlen);
        if (retlen > 0) {
            /* NXP ACT Proprietary Ext */
            status = phNxpNciHal_send_ext_cmd(retlen, buffer);
            if(status == NFCSTATUS_SUCCESS)
            {
                status = phNxpNciHal_CheckRFCmdRespStatus();
                /*STATUS INVALID PARAM 0x09*/
                if(status == 0x09)
                {
        #if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                    if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                    {
                        phNxpNciHalRFConfigCmdRecSequence();
                        retry_core_init_cnt++;
                        goto retry_core_init;
                    }
                    else
        #endif
                    {
                        NXPLOG_NCIHAL_E("Setting NXP_CORE_RF_FIELD status failed - recovery needs FW buffer");
                    }
                }
            }
            else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("Setting NXP_CORE_RF_FIELD status failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
        }

        retlen = 0;
        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
        {
            config_access = TRUE;
        }
        num = 0;
        /* NXP SWP switch timeout Setting*/
        /* This is UICC specific, so disabling*/
#ifdef ANDROID
        if(GetNxpNumValue(CFG_NXP_SWP_SWITCH_TIMEOUT, (void *)&num, sizeof(num)))
        {
            //Check the permissible range [0 - 60]
            if(num <= 60)
            {
                if( 0 < num)
                {
                    uint16_t timeout = num * 1000;
                    swp_switch_timeout_cmd[7]= (timeout & 0xFF);
                    swp_switch_timeout_cmd[8]=  ((timeout & 0xFF00) >> 8);
                }
                status = phNxpNciHal_send_ext_cmd (sizeof(swp_switch_timeout_cmd),
                        swp_switch_timeout_cmd);
                if (status != NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("SWP switch timeout Setting Failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
            else
            {
                NXPLOG_NCIHAL_E("SWP switch timeout Setting Failed - out of range!");
            }
        }
#endif
        status = phNxpNciHal_china_tianjin_rf_setting();
        if (status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("phNxpNciHal_china_tianjin_rf_setting failed");
            retry_core_init_cnt++;
            goto retry_core_init;
        }

        config_access = FALSE;
        retlen = 0;
        num = 0;
        /* SWP FULL PWR MODE SETTING ON */
        /* UICC specific, hence removed */
#ifdef ANDROID
        if(GetNxpNumValue(CFG_NXP_SWP_FULL_PWR_ON, (void *)&num, sizeof(num)))
        {
            if(1 == num)
            {
                status = phNxpNciHal_send_ext_cmd (sizeof(swp_full_pwr_mode_on_cmd),
                                                          swp_full_pwr_mode_on_cmd);
                if (status != NFCSTATUS_SUCCESS)
                {
                   NXPLOG_NCIHAL_E("SWP FULL PWR MODE SETTING ON CMD FAILED");
                   retry_core_init_cnt++;
                   goto retry_core_init;
                }
            }
            else
            {
                swp_full_pwr_mode_on_cmd[7]=0x00;
                status = phNxpNciHal_send_ext_cmd (sizeof(swp_full_pwr_mode_on_cmd),
                                                          swp_full_pwr_mode_on_cmd);
                if (status != NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("SWP FULL PWR MODE SETTING OFF CMD FAILED");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
        }
#endif
    }

    NXPLOG_NCIHAL_D ("Apply Firmware Specific Settings");
    config_access = FALSE;

    mEEPROM_info.buffer = &fw_dwnld_flag;
    mEEPROM_info.bufflen= sizeof(fw_dwnld_flag);
    mEEPROM_info.request_type = EEPROM_FW_DWNLD;
    mEEPROM_info.request_mode = GET_EEPROM_DATA;
    request_EEPROM(&mEEPROM_info);

    setConfigAlways = FALSE;
    isfound = GetNxpNumValue(CFG_NXP_SET_CONFIG_ALWAYS, &num, sizeof(num));
    if(isfound > 0)
    {
        setConfigAlways = num;
    }
    isCorrectFwVersion = phNxpNciHal_is_firmware_uptodate();
    NXPLOG_NCIHAL_D ("EEPROM_fw_dwnld_flag : 0x%02x SetConfigAlways flag : 0x%02x isCorrectFwVersion : 0x%02x ",
            fw_dwnld_flag, setConfigAlways,isCorrectFwVersion);

    //setConfigAlways = TRUE; //Kanjie

    if( ( (TRUE == fw_dwnld_flag) && (TRUE == isCorrectFwVersion) ) || (TRUE == setConfigAlways) )
    {
        retlen = 0;
        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
        {
            config_access = FALSE;
        }

        //check if customized city specific settings need to be applied
        if(transitSettings.rfSettingModified)
        {
            status = phNxpNciHal_set_city_specific_rf_configs(transitSettings.cityType);
            if (status != NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("phNxpNciHal_set_city_specific_rf_configs failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }
        }
        else
        {
            NXPLOG_NCIHAL_D ("Performing RF Settings BLK 1");
            isfound = GetNxpByteArrayValue(CFG_NXP_RF_CONF_BLK_1, (char *) buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phNxpNciHal_CheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if(status == 0x09)
                    {
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                        {
                            phNxpNciHalRFConfigCmdRecSequence();
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                        else
#endif
                        {
                            NXPLOG_NCIHAL_E("RF Settings BLK 1 failed - recovery needs FW buffer");
                        }
                    }
                }
                else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("RF Settings BLK 1 failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
            retlen = 0;

            NXPLOG_NCIHAL_D ("Performing RF Settings BLK 2");
            isfound = GetNxpByteArrayValue(CFG_NXP_RF_CONF_BLK_2, (char *) buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phNxpNciHal_CheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if(status == 0x09)
                    {
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                        {
                            phNxpNciHalRFConfigCmdRecSequence();
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                        else
#endif
                        {
                            NXPLOG_NCIHAL_E("RF Settings BLK 2 failed - recovery needs FW buffer");
                        }
                    }
                }
                else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("RF Settings BLK 2 failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
            retlen = 0;

            NXPLOG_NCIHAL_D ("Performing RF Settings BLK 3");
            isfound = GetNxpByteArrayValue(CFG_NXP_RF_CONF_BLK_3, (char *) buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phNxpNciHal_CheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if(status == 0x09)
                    {
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                        {
                            phNxpNciHalRFConfigCmdRecSequence();
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                        else
#endif
                        {
                            NXPLOG_NCIHAL_E("RF Settings BLK 3 failed - recovery needs FW buffer");
                        }
                    }
                }
                else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("RF Settings BLK 3 failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
            retlen = 0;

            NXPLOG_NCIHAL_D ("Performing RF Settings BLK 4");
            isfound = GetNxpByteArrayValue(CFG_NXP_RF_CONF_BLK_4, (char *) buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phNxpNciHal_CheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if(status == 0x09)
                    {
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                        {
                            phNxpNciHalRFConfigCmdRecSequence();
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                        else
#endif
                        {
                            NXPLOG_NCIHAL_E("RF Settings BLK 4 failed - recovery needs FW buffer");
                        }
                    }
                }
                else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("RF Settings BLK 4 failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
            retlen = 0;

            NXPLOG_NCIHAL_D ("Performing RF Settings BLK 5");
            isfound = GetNxpByteArrayValue(CFG_NXP_RF_CONF_BLK_5, (char *) buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phNxpNciHal_CheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if(status == 0x09)
                    {
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                        {
                            phNxpNciHalRFConfigCmdRecSequence();
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                        else
#endif
                        {
                            NXPLOG_NCIHAL_E("RF Settings BLK 5 failed - recovery needs FW buffer");
                        }
                    }
                }
                else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("RF Settings BLK 5 failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
            retlen = 0;

            NXPLOG_NCIHAL_D ("Performing RF Settings BLK 6");
            isfound = GetNxpByteArrayValue(CFG_NXP_RF_CONF_BLK_6, (char *) buffer,
                    bufflen, &retlen);
            if (retlen > 0) {
                status = phNxpNciHal_send_ext_cmd(retlen, buffer);
                if(status == NFCSTATUS_SUCCESS)
                {
                    status = phNxpNciHal_CheckRFCmdRespStatus();
                    /*STATUS INVALID PARAM 0x09*/
                    if(status == 0x09)
                    {
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
                        if(gpFwRecBuffer!= NULL && gpFwBuffer != NULL)
                        {
                            phNxpNciHalRFConfigCmdRecSequence();
                            retry_core_init_cnt++;
                            goto retry_core_init;
                        }
                        else
#endif
                        {
                            NXPLOG_NCIHAL_E("RF Settings BLK 6 failed - recovery needs FW buffer");
                        }
                    }
                }
                else
                if (status != NFCSTATUS_SUCCESS) {
                    NXPLOG_NCIHAL_E("RF Settings BLK 6 failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }

        }


        NXPLOG_NCIHAL_E("Resetting FW Dnld flag");
        fw_dwnld_flag = 0x00;
        mEEPROM_info.buffer = &fw_dwnld_flag;
        mEEPROM_info.bufflen = sizeof(fw_dwnld_flag);
        mEEPROM_info.request_type = EEPROM_FW_DWNLD;
        mEEPROM_info.request_mode = SET_EEPROM_DATA;
        status = request_EEPROM(&mEEPROM_info);
        if(status == NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("Resetting FW Dnld flag SUCCESS");
        }
        else
        {
            NXPLOG_NCIHAL_E("Resetting FW Dnld flag FAILED");
        }
    }

    status = phNxpNciHal_check_eSE_Session_Identity();
    if(status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("Get Session id Failed");
        retry_core_init_cnt++;
        goto retry_core_init;
    }
    config_access = FALSE;
    if(!((*p_core_init_rsp_params > 0) && (*p_core_init_rsp_params < 4)))
    {
        status = phNxpNciHal_reset_nfcc_fw();//CID10684
        if(status == NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("phNxpNciHal_reset_nfcc_fw SUCCESS");
            status = phNxpNciHal_send_get_cfgs();
            if(status == NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("Send get Configs SUCCESS");
            }
            else
            {
                NXPLOG_NCIHAL_E("Send get Configs FAILED");
            }
        }
        else
        {
            NXPLOG_NCIHAL_E("phNxpNciHal_reset_nfcc_fw FAILED");
        }
    }
    recovery_wiredmode:
    status = phNxpNciHal_get_jcop_state(&jcopstate);
    if(status == NFCSTATUS_SUCCESS)
    {
        if(jcopstate == NCI_HAL_OSU_COMPLETE)
            nxpncihal_ctrl.hal_boot_mode = HAL_NORMAL_BOOT_MODE;
        else
            nxpncihal_ctrl.hal_boot_mode = HAL_OSU_BOOT_MODE;

        if(nxpncihal_ctrl.hal_boot_mode == HAL_OSU_BOOT_MODE)
        {
            pwr_link_required = TRUE;
        }
    }
    else
    {
        NXPLOG_NCIHAL_E("Querying JCOP State FAILED");
        retry_core_init_cnt++;
        goto retry_core_init;
    }
    if(pwr_link_required == TRUE && isCorrectFwVersion == TRUE) /* This is necessary else, querying APDU registry will fail, causing SeApi_Init to fail
    TLV protection doesn't allow JCOP download when Init is failed */
    {
        phNxpNciHal_send_nfcee_pwr_cntl_cmd(POWER_ALWAYS_ON|LINK_ALWAYS_ON);
        pwr_link_required = FALSE;
    }
    if((*p_core_init_rsp_params > 0) && (*p_core_init_rsp_params < 4))
    {
        static phLibNfc_Message_t msg;
        uint16_t tmp_len = 0;
        /*Changed part of warning fixes in wearable project*/
        //uint8_t uicc_set_mode[] = {0x22, 0x01, 0x02, 0x02, 0x01};

        uint8_t set_screen_state[] = {0x2F, 0x15, 01, 00};      //SCREEN ON
        uint8_t nfcc_core_conn_create[] = {0x20, 0x04, 0x06, 0x03, 0x01, 0x01, 0x02, 0x01, 0x01};
        uint8_t nfcc_mode_set_on[] = {0x22, 0x01, 0x02, 0x01, 0x01};

        NXPLOG_NCIHAL_E("Sending DH and NFCC core connection command as raw packet!!");
        status = phNxpNciHal_send_ext_cmd (sizeof(nfcc_core_conn_create), nfcc_core_conn_create);

        if (status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("Sending DH and NFCC core connection command as raw packet!! Failed");
            retry_core_init_cnt++;
            goto retry_core_init;
        }

        NXPLOG_NCIHAL_E("Sending DH and NFCC mode set as raw packet!!");
        status = phNxpNciHal_send_ext_cmd (sizeof(nfcc_mode_set_on), nfcc_mode_set_on);

        if (status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("Sending DH and NFCC mode set as raw packet!! Failed");
            retry_core_init_cnt++;
            goto retry_core_init;
        }

        if(*(p_core_init_rsp_params + 1) == 1) // RF state is Discovery!!
        {
            NXPLOG_NCIHAL_E("Sending Set Screen ON State Command as raw packet!!");
            status = phNxpNciHal_send_ext_cmd (sizeof(set_screen_state),
                                          set_screen_state);
            if (status != NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("Sending Set Screen ON State Command as raw packet!! Failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }

            NXPLOG_NCIHAL_E("Sending discovery as raw packet!!");
            status = phNxpNciHal_send_ext_cmd (p_core_init_rsp_params[2],
                                                      (uint8_t *)&p_core_init_rsp_params[3]);
            if (status != NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("Sending discovery as raw packet Failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }
        }
        else
        {
            NXPLOG_NCIHAL_E("Sending Set Screen OFF State Command as raw packet!!");
            set_screen_state[3] = 0x01; //Screen OFF
            status = phNxpNciHal_send_ext_cmd (sizeof(set_screen_state),
                                          set_screen_state);
            if (status != NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("Sending Set Screen OFF State Command as raw packet!! Failed");
                retry_core_init_cnt++;
                goto retry_core_init;
            }

        }
        NXPLOG_NCIHAL_E("Sending last command for Recovery ");
        if(p_core_init_rsp_params[35] > 0)
        {  //if length of last command is 0 then it doesn't need to send last command.
            if( !(((p_core_init_rsp_params[36] == 0x21) && (p_core_init_rsp_params[37] == 0x03))
                && (*(p_core_init_rsp_params + 1) == 1)) &&
                    !((p_core_init_rsp_params[36] == 0x21) && (p_core_init_rsp_params[37] == 0x06) && (p_core_init_rsp_params[39] == 0x00) &&(*(p_core_init_rsp_params + 1) == 0x00)))
                //if last command is discovery and RF status is also discovery state, then it doesn't need to execute or similarly
                // if the last command is deactivate to idle and RF status is also idle , no need to execute the command .
            {
                tmp_len = p_core_init_rsp_params[35];

                /* Check for NXP ext before sending write */
                status = phNxpNciHal_write_ext(&tmp_len,
                        (uint8_t *)&p_core_init_rsp_params[36], &nxpncihal_ctrl.rsp_len,
                        nxpncihal_ctrl.p_rsp_data);
                if (status != NFCSTATUS_SUCCESS)
                {
                    if(buffer)
                    {
                        phOsalNfc_FreeMemory(buffer);
                        buffer = NULL;
                    }
                    /* Do not send packet to PN54X, send response directly */
                    msg.eMsgType = NCI_HAL_RX_MSG;
                    msg.pMsgData = NULL;
                    msg.Size = 0;

                    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
                            (phLibNfc_Message_t *) &msg);
                    return NFCSTATUS_SUCCESS;
                }

                p_core_init_rsp_params[35] = (uint8_t)tmp_len;

                status = phNxpNciHal_send_ext_cmd (p_core_init_rsp_params[35],
                                                          (uint8_t *)&p_core_init_rsp_params[36]);
                if (status != NFCSTATUS_SUCCESS)
                {
                    NXPLOG_NCIHAL_E("Sending last command for Recovery Failed");
                    retry_core_init_cnt++;
                    goto retry_core_init;
                }
            }
        }
    }
    retry_core_init_cnt = 0;

    if(buffer)
    {
        phOsalNfc_FreeMemory(buffer);
        buffer = NULL;
    }
    config_access = FALSE;
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
    //initialize dummy FW recovery variables
    gRecFWDwnld = FALSE;
    gRecFwRetryCount = 0;
#endif
    if(!((*p_core_init_rsp_params > 0) && (*p_core_init_rsp_params < 4)))
        phNxpNciHal_core_initialized_complete(status);
    else
    {
invoke_callback:
        config_access = FALSE;
        if (nxpncihal_ctrl.p_nfc_stack_data_cback != NULL)
        {
            *p_core_init_rsp_params = 0;
            NXPLOG_NCIHAL_E("Invoking data callback!!");
            (*nxpncihal_ctrl.p_nfc_stack_data_cback)(
                    nxpncihal_ctrl.rx_data_len, nxpncihal_ctrl.p_rx_data);
        }
    }

    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_check_eSE_Session_Identity
 *
 * Description      This function is called at init time to check
 *                  the presence of ese related info and disable SWP interfaces.
 *
 * Returns          void.
 *
 ******************************************************************************/
static NFCSTATUS phNxpNciHal_check_eSE_Session_Identity(void)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    static uint8_t session_identity[8] = {0x00};
    uint8_t default_session[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    phNxpNci_EEPROM_info_t mEEPROM_info = {0};
    mEEPROM_info.request_mode = GET_EEPROM_DATA;
    mEEPROM_info.request_type = EEPROM_ESE_SESSION_ID;
    mEEPROM_info.buffer = session_identity;
    mEEPROM_info.bufflen = sizeof(session_identity);
    status = request_EEPROM(&mEEPROM_info);
    if(status == NFCSTATUS_OK)
    {
        if(!memcmp((uint8_t*)session_identity, (uint8_t*)default_session, sizeof(session_identity)))
        {
            pwr_link_required = TRUE;
        }
    }
    return status;
}

/******************************************************************************
 * Function         phNxpNciHal_CheckRFCmdRespStatus
 *
 * Description      This function is called to check the resp status of
 *                  RF update commands.
 *
 * Returns          NFCSTATUS_SUCCESS           if successful,
 *                  NFCSTATUS_INVALID_PARAMETER if parameter is inavlid
 *                  NFCSTATUS_FAILED            if failed response
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_CheckRFCmdRespStatus()
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    static uint16_t INVALID_PARAM = 0x09;
    if ((nxpncihal_ctrl.rx_data_len > 0) && (nxpncihal_ctrl.p_rx_data[2] > 0))
    {
        if(nxpncihal_ctrl.p_rx_data[3] == 0x09)
        {
            status = INVALID_PARAM;
        }
        else if(nxpncihal_ctrl.p_rx_data[3] != NFCSTATUS_SUCCESS)
        {
            status = NFCSTATUS_FAILED;
        }
    }
    return status;
}

#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
/******************************************************************************
 * Function         phNxpNciHalRFConfigCmdRecSequence
 *
 * Description      This function is called to handle the dummy FW recovery sequence
 *                  whenever the RF settings call results in a failure that returns
 *                  an invalid parameter status.
 *                  The recovery mechanism includes a dummy firmware download followed
 *                  by a firmware download and then config settings. The dummy firmware
 *                  download will change the major number of the firmware inside the
 *                  NFCC. This will allow the actual firmware download to execute
 *                  successfully. This process can be retried a maximum of three times.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHalRFConfigCmdRecSequence()
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint16_t recFWState = 1;
    phNxpNciHal_set_forced_dnd(TRUE);
    gRecFwRetryCount++;
    if(gRecFwRetryCount > 0x03)
    {
        NXPLOG_NCIHAL_D ("Max retry count for RF config FW recovery exceeded ");
        gRecFWDwnld = FALSE;
        phNxpNciHal_set_forced_dnd(FALSE);
        return NFCSTATUS_FAILED;
    }
    do{
        status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);
        if(NFCSTATUS_SUCCESS != status)
        {
            return status;
        }
        status = phDnldNfc_InitImgInfo();
        if(NFCSTATUS_SUCCESS != status)
        {
            return status;
        }
        if (NFCSTATUS_SUCCESS == phNxpNciHal_CheckValidFwVersion())
        {
            GeneralState |= FWDNLD_MASK;
            status = phNxpNciHal_fw_download(TRUE);
            GeneralState &= ~FWDNLD_MASK;

            if(status == NFCSTATUS_SUCCESS)
            {
                status = phTmlNfc_Read(
                nxpncihal_ctrl.p_cmd_data,
                NCI_MAX_DATA_LEN,(pphTmlNfc_TransactCompletionCb_t) &phNxpNciHal_read_complete,
                NULL);
                if (status != NFCSTATUS_PENDING)
                {
                    NXPLOG_NCIHAL_E("TML Read status error status = %x", status);
                    status = phTmlNfc_Shutdown();
                    status = NFCSTATUS_FAILED;
                    break;
                }
            }
            else
            {
                status = NFCSTATUS_FAILED;
                break;
            }
        }
        gRecFWDwnld = FALSE;
    }while(recFWState--);
    gRecFWDwnld = FALSE;
    phNxpNciHal_set_forced_dnd(FALSE);
    return status;
}
#endif

/******************************************************************************
 * Function         phNxpNciHal_core_initialized_complete
 *
 * Description      This function is called when phNxpNciHal_core_initialized
 *                  complete all proprietary command exchanges. This function
 *                  informs libnfc-nci about completion of core initialize
 *                  and result of that through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_core_initialized_complete(NFCSTATUS status)
{
    static phLibNfc_Message_t msg;

    if (status == NFCSTATUS_SUCCESS)
    {
        msg.eMsgType = NCI_HAL_POST_INIT_CPLT_MSG;
    }
    else
    {
        msg.eMsgType = NCI_HAL_ERROR_MSG;
    }
    msg.pMsgData = NULL;
    msg.Size = 0;

    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
            (phLibNfc_Message_t *) &msg);

    return;
}

/******************************************************************************
 * Function         phNxpNciHal_pre_discover
 *
 * Description      This function is called by libnfc-nci to perform any
 *                  proprietary exchange before RF discovery. When proprietary
 *                  exchange is over completion is informed to libnfc-nci
 *                  through phNxpNciHal_pre_discover_complete function.
 *
 * Returns          It always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_pre_discover(void)
{
    /* Nothing to do here for initial version */
    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_pre_discover_complete
 *
 * Description      This function informs libnfc-nci about completion and
 *                  status of phNxpNciHal_pre_discover through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
#if PN547C2_DISABLE_DEAD_FUN
static void phNxpNciHal_pre_discover_complete(NFCSTATUS status)
{
    static phLibNfc_Message_t msg;

    if (status == NFCSTATUS_SUCCESS)
    {
        msg.eMsgType = NCI_HAL_PRE_DISCOVER_CPLT_MSG;
    }
    else
    {
        msg.eMsgType = NCI_HAL_ERROR_MSG;
    }
    msg.pMsgData = NULL;
    msg.Size = 0;

    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
            &msg);

    return;
}

#endif
/******************************************************************************
 * Function         phNxpNciHal_release_info
 *
 * Description      This function frees allocated memory for mGetCfg_info
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_release_info(void)
{
    NXPLOG_NCIHAL_D ("phNxpNciHal_release_info mGetConfig_info");
    if(mGetConfig_info != NULL)
    {
       phOsalNfc_FreeMemory(mGetConfig_info);
       mGetConfig_info = NULL;
    }
}
/******************************************************************************
 * Function         phNxpNciHal_close
 *
 * Description      This function close the NFCC interface and free all
 *                  resources.This is called by libnfc-nci on NFC service stop.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_close(void)
{
    NFCSTATUS status;
    /*NCI_RESET_CMD*/
    static uint8_t cmd_reset_nci[] = {0x20,0x00,0x01,0x00};

    static uint8_t cmd_ce_disc_nci[] = {0x21,0x03,0x07,0x03,0x80,0x01,0x81,0x01,0x82,0x01};

    CONCURRENCY_LOCK();
    if(read_failed_disable_nfc)
    {
        read_failed_disable_nfc = FALSE;
        goto close_and_return;
    }
    if(write_unlocked_status == NFCSTATUS_FAILED)
    {
        NXPLOG_NCIHAL_D("phNxpNciHal_close i2c write failed .Clean and Return");
        goto close_and_return;
    }

    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_ce_disc_nci),cmd_ce_disc_nci);
    if(status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E ("CMD_CE_DISC_NCI: Failed");
    }
    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_reset_nci),cmd_reset_nci);
    if(status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E ("NCI_CORE_RESET: Failed");
    }

    close_and_return:
    nxpncihal_ctrl.halStatus = HAL_STATUS_CLOSE;
    if (NULL != gpphTmlNfc_Context->pDevHandle)
    {
        phNxpNciHal_close_complete(NFCSTATUS_SUCCESS);
        //wait till message NCI_HAL_CLOSE_CPLT_MSG is posted to HAL Client Task
        phOsalNfc_ConsumeSemaphore(nxpncihal_ctrl.halClientSemaphore);

        /* Abort any pending read and write */
        status = phTmlNfc_ReadAbort();
        status = phTmlNfc_WriteAbort();

        phOsalNfc_Timer_Cleanup();
        status = phTmlNfc_Shutdown();

        //free resources used by HAL Client Task
        phDal4Nfc_msgrelease(nxpncihal_ctrl.gDrvCfg.nClientId);
        phOsalNfc_Thread_Delete(nxpncihal_ctrl.client_task);
        phOsalNfc_DeleteSemaphore(&nxpncihal_ctrl.halClientSemaphore);
        phOsalNfc_SetMemory (&nxpncihal_ctrl, 0x00, sizeof (nxpncihal_ctrl));
        NXPLOG_NCIHAL_D("phNxpNciHal_close - phOsalNfc_DeInit completed");
        phNxpNciHal_release_info();
    }

    CONCURRENCY_UNLOCK();

    phNxpNciHal_cleanup_monitor();
    write_unlocked_status = NFCSTATUS_SUCCESS;
    /* Return success always */
    return NFCSTATUS_SUCCESS;
}

int phNxpNciHal_Cleanup()
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    CONCURRENCY_LOCK();
    nxpncihal_ctrl.halStatus = HAL_STATUS_CLOSE;
    if (NULL != gpphTmlNfc_Context->pDevHandle)
    {
        /* Abort any pending read and write */
        status = phTmlNfc_ReadAbort();
        status = phTmlNfc_WriteAbort();

        phOsalNfc_Timer_Cleanup();
        status = phTmlNfc_Shutdown();

        //free resources used by HAL Client Task
        phDal4Nfc_msgrelease(nxpncihal_ctrl.gDrvCfg.nClientId);
        phOsalNfc_Thread_Delete(nxpncihal_ctrl.client_task);
        phOsalNfc_ProduceSemaphore(nxpncihal_ctrl.halClientSemaphore);
        phOsalNfc_DeleteSemaphore(&nxpncihal_ctrl.halClientSemaphore);
        phOsalNfc_SetMemory (&nxpncihal_ctrl, 0x00, sizeof (nxpncihal_ctrl));
        NXPLOG_NCIHAL_D("phNxpNciHal_close - phOsalNfc_DeInit completed");
    }

    CONCURRENCY_UNLOCK();

    phNxpNciHal_cleanup_monitor();
    return status;
}
/******************************************************************************
 * Function         phNxpNciHal_close_complete
 *
 * Description      This function inform libnfc-nci about result of
 *                  phNxpNciHal_close.
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_close_complete(NFCSTATUS status)
{
    static phLibNfc_Message_t msg;

    if (status == NFCSTATUS_SUCCESS)
    {
        msg.eMsgType = NCI_HAL_CLOSE_CPLT_MSG;
    }
    else
    {
        msg.eMsgType = NCI_HAL_ERROR_MSG;
    }
    msg.pMsgData = NULL;
    msg.Size = 0;

    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
            &msg);

    return;
}
/******************************************************************************
 * Function         phNxpNciHal_notify_i2c_fragmentation
 *
 * Description      This function can be used by HAL to inform
 *                 libnfc-nci that i2c fragmentation is enabled/disabled
 *
 * Returns          void.
 *
 ******************************************************************************/
void phNxpNciHal_notify_i2c_fragmentation(void)
{
    if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
    {
        /*inform libnfc-nci that i2c fragmentation is enabled/disabled */
        (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_ENABLE_I2C_FRAGMENTATION_EVT,
                HAL_NFC_STATUS_OK);
    }


}
/******************************************************************************
 * Function         phNxpNciHal_control_granted
 *
 * Description      Called by libnfc-nci when NFCC control is granted to HAL.
 *
 * Returns          Always returns NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_control_granted(void)
{
    /* Take the concurrency lock so no other calls from upper layer
     * will be allowed
     */
    CONCURRENCY_LOCK();

    if(NULL != nxpncihal_ctrl.p_control_granted_cback)
    {
        (*nxpncihal_ctrl.p_control_granted_cback)();
    }
    /* At the end concurrency unlock so calls from upper layer will
     * be allowed
     */
    CONCURRENCY_UNLOCK();
    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_request_control
 *
 * Description      This function can be used by HAL to request control of
 *                  NFCC to libnfc-nci. When control is provided to HAL it is
 *                  notified through phNxpNciHal_control_granted.
 *
 * Returns          void.
 *
 ******************************************************************************/
#if PN547C2_DISABLE_DEAD_FUN
void phNxpNciHal_request_control(void)
{
    if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
    {
         /* Request Control of NCI Controller from NCI NFC Stack*/
        (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_REQUEST_CONTROL_EVT,
                HAL_NFC_STATUS_OK);
    }

    return;
}
#endif

/******************************************************************************
 * Function         phNxpNciHal_release_control
 *
 * Description      This function can be used by HAL to release the control of
 *                  NFCC back to libnfc-nci.
 *
 * Returns          void.
 *
 ******************************************************************************/
#if PN547C2_DISABLE_DEAD_FUN
void phNxpNciHal_release_control(void)
{
    if (nxpncihal_ctrl.p_nfc_stack_cback != NULL)
    {
        /* Release Control of NCI Controller to NCI NFC Stack*/
        (*nxpncihal_ctrl.p_nfc_stack_cback)(HAL_NFC_RELEASE_CONTROL_EVT,
                HAL_NFC_STATUS_OK);
    }

    return;
}

#endif
/******************************************************************************
 * Function         phNxpNciHal_power_cycle
 *
 * Description      This function is called by libnfc-nci when power cycling is
 *                  performed. When processing is complete it is notified to
 *                  libnfc-nci through phNxpNciHal_power_cycle_complete.
 *
 * Returns          Always return NFCSTATUS_SUCCESS (0).
 *
 ******************************************************************************/
int phNxpNciHal_power_cycle(void)
{
    NXPLOG_NCIHAL_D("Power Cycle");

    NFCSTATUS status = NFCSTATUS_FAILED;

    status = phTmlNfc_IoCtl(phTmlNfc_e_ResetDevice);

    if(NFCSTATUS_SUCCESS == status)
    {
        NXPLOG_NCIHAL_D("PN54X Reset - SUCCESS\n");
    }
    else
    {
        NXPLOG_NCIHAL_D("PN54X Reset - FAILED\n");
    }

    phNxpNciHal_power_cycle_complete(NFCSTATUS_SUCCESS);

    return NFCSTATUS_SUCCESS;
}

/******************************************************************************
 * Function         phNxpNciHal_power_cycle_complete
 *
 * Description      This function is called to provide the status of
 *                  phNxpNciHal_power_cycle to libnfc-nci through callback.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_power_cycle_complete(NFCSTATUS status)
{
    static phLibNfc_Message_t msg;

    if (status == NFCSTATUS_SUCCESS)
    {
        msg.eMsgType = NCI_HAL_OPEN_CPLT_MSG;
    }
    else
    {
        msg.eMsgType = NCI_HAL_ERROR_MSG;
    }
    msg.pMsgData = NULL;
    msg.Size = 0;
    NXPLOG_NCIHAL_E ("phNxpNciHal_power_cycle_complete Posting 0x%x", msg.eMsgType);
    phTmlNfc_DeferredCall(gpphTmlNfc_Context->dwCallbackTaskHandle,
            &msg);

    return;
}


/******************************************************************************
 * Function         phNxpNciHal_ioctl
 *
 * Description      This function is called by jni when wired mode is
 *                  performed.First Pn54x driver will give the access
 *                  permission whether wired mode is allowed or not
 *                  arg (0):
 * Returns          return 0 on success and -1 on fail, On success
 *                  update the acutual state of operation in arg pointer
 *
 ******************************************************************************/
int phNxpNciHal_ioctl(long arg, void *p_data)
{
    NXPLOG_NCIHAL_D("%s : enter - arg = %ld", __FUNCTION__, arg);

    int ret = -1;
    NFCSTATUS status = NFCSTATUS_FAILED;
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
    phNxpNciHal_FwRfupdateInfo_t *FwRfInfo;
#endif
    nfc_nci_ExtnCmd_t *ExtnCmd;
    NFCSTATUS fm_mw_ver_check = NFCSTATUS_FAILED;

    switch(arg)
    {
    case HAL_NFC_IOCTL_GET_CONFIG_INFO:
        if(mGetConfig_info != NULL)
        {
            memcpy(p_data, mGetConfig_info , sizeof(phNxpNci_getConfig_info_t));
            phOsalNfc_FreeMemory(mGetConfig_info);
            mGetConfig_info = NULL;
            ret = 0;
        }
        break;
#if (FIRMWARE_DOWNLOAD_ENABLE == TRUE)
    case HAL_NFC_IOCTL_CHECK_FLASH_REQ:
        FwRfInfo = (phNxpNciHal_FwRfupdateInfo_t *) p_data;
        status = phNxpNciHal_CheckFwRegFlashRequired(&FwRfInfo->fw_update_reqd,
                &FwRfInfo->rf_update_reqd);
        if(NFCSTATUS_SUCCESS == status)
        {
            ret = 0;
        }
        break;
    case HAL_NFC_IOCTL_FW_DWNLD:
        status = phNxpNciHal_FwDwnld(FALSE);
        *(uint16_t*)p_data = (uint16_t)status;
        if(NFCSTATUS_SUCCESS == status)
        {
            ret = 0;
        }
        break;
    case HAL_NFC_IOCTL_FORCED_FW_DWNLD:
        phNxpNciHal_set_forced_dnd(TRUE);
        status = phNxpNciHal_FwDwnld(TRUE);
        *(uint16_t*)p_data = (uint16_t)status;
        phNxpNciHal_set_forced_dnd(FALSE);
        if(NFCSTATUS_SUCCESS == status)
        {
            ret = 0;
        }
        break;
#endif
    case HAL_NFC_IOCTL_FW_MW_VER_CHECK:
        fm_mw_ver_check = phNxpNciHal_fw_mw_ver_check();
        *(uint16_t *)p_data = fm_mw_ver_check;
        ret = 0;
        break;
    case HAL_NFC_IOCTL_NCI_TRANSCEIVE:
        if(p_data == NULL)
        {
            ret = -1;
            break;
        }
        ExtnCmd = (nfc_nci_ExtnCmd_t *)p_data;
        if(ExtnCmd->cmd_len <= 0 || ExtnCmd->p_cmd == NULL)
        {
            ret = -1;
            break;
        }

        ret  = phNxpNciHal_send_ext_cmd(ExtnCmd->cmd_len, ExtnCmd->p_cmd);
        ExtnCmd->rsp_len = nxpncihal_ctrl.rx_data_len;
        ExtnCmd->p_cmd_rsp = nxpncihal_ctrl.p_rx_data;
        break;
    default:
        NXPLOG_NCIHAL_E("%s : Wrong arg = %ld", __FUNCTION__, arg);
        break;
    }
    NXPLOG_NCIHAL_D("%s : exit - ret = %d", __FUNCTION__, ret);
    return ret;
}

/******************************************************************************
 * Function         phNxpNciHal_set_clock
 *
 * Description      This function is called after successfull download
 *                  to apply the clock setting provided in config file
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_set_clock(void)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    int retryCount = 0;

retrySetclock:
    phNxpNciClock.isClockSet = TRUE;
    if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_PLL)
    {
        static uint8_t set_clock_cmd[] = {0x20, 0x02,0x09, 0x02, 0xA0, 0x03, 0x01, 0x11,
                                                               0xA0, 0x04, 0x01, 0x01};
#if(NFC_NXP_CHIP_TYPE == PN553)
        uint8_t param_clock_src = 0x00;
#else
        uint8_t param_clock_src = CLK_SRC_PLL;
        param_clock_src = param_clock_src << 3;
#endif

        if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_13MHZ)
        {
            param_clock_src |= 0x00;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_19_2MHZ)
        {
            param_clock_src |= 0x01;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_24MHZ)
        {
            param_clock_src |= 0x02;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_26MHZ)
        {
            param_clock_src |= 0x03;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_38_4MHZ)
        {
            param_clock_src |= 0x04;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_52MHZ)
        {
            param_clock_src |= 0x05;
        }
        else
        {
            NXPLOG_NCIHAL_E("Wrong clock freq, send default PLL@19.2MHz");
#if(NFC_NXP_CHIP_TYPE == PN553)
            param_clock_src = 0x01;
#else
            param_clock_src = 0x11;
#endif
        }

        set_clock_cmd[7] = param_clock_src;
        set_clock_cmd[11] = nxpprofile_ctrl.bTimeout;
        status = phNxpNciHal_send_ext_cmd(sizeof(set_clock_cmd), set_clock_cmd);
        if (status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("PLL colck setting failed !!");
        }
    }
    else if(nxpprofile_ctrl.bClkSrcVal == CLK_SRC_XTAL)
    {
        static uint8_t set_clock_cmd[] = {0x20, 0x02, 0x05, 0x01, 0xA0, 0x03, 0x01, 0x08};
        status = phNxpNciHal_send_ext_cmd(sizeof(set_clock_cmd), set_clock_cmd);
        if (status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("XTAL colck setting failed !!");
        }

    }
    else
    {
        NXPLOG_NCIHAL_E("Wrong clock source. Dont apply any modification")
    }

   // Checking for SET CONFG SUCCESS, re-send the command  if not.
    phNxpNciClock.isClockSet = FALSE;
    if(phNxpNciClock.p_rx_data[3]   != NFCSTATUS_SUCCESS )
    {
        if(retryCount++  < 3)
        {
            NXPLOG_NCIHAL_E("Set-clk failed retry again ");
            goto retrySetclock;
        }
        else
        {
            NXPLOG_NCIHAL_D("Set clk  failed -  max count = 0x%x exceeded ", retryCount);
//            NXPLOG_NCIHAL_E("Set Config is failed for Clock Due to elctrical disturbances, aborting the NFC process");
//            abort ();
       }
    }
}

/******************************************************************************
 * Function         phNxpNciHal_check_clock_config
 *
 * Description      This function is called after successfull download
 *                  to check if clock settings in config file and chip
 *                  is same
 *
 * Returns          void.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_check_clock_config(void)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    uint8_t param_clock_src;
    static uint8_t get_clock_cmd[] = {0x20, 0x03,0x07, 0x03, 0xA0, 0x02,
            0xA0, 0x03, 0xA0, 0x04};
    phNxpNciClock.isClockSet = TRUE;
    phNxpNciHal_get_clk_freq();
    status = phNxpNciHal_send_ext_cmd(sizeof(get_clock_cmd),get_clock_cmd);

    if(status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("unable to retrieve get_clk_src_sel");
        return status;
    }
    param_clock_src = check_config_parameter();
    if( phNxpNciClock.p_rx_data[12] == param_clock_src &&  phNxpNciClock.p_rx_data[16] == nxpprofile_ctrl.bTimeout)
    {
        phNxpNciClock.issetConfig = FALSE;
    }else {
        phNxpNciClock.issetConfig = TRUE;
    }
    phNxpNciClock.isClockSet = FALSE;

    return status;
}

/******************************************************************************
 * Function         phNxpNciHal_set_city_specific_rf_configs
 *
 * Description      This function is called to set city specific rf configs
 *
 * Returns          Status.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_set_city_specific_rf_configs(UINT8 cityType)
{
    uint8_t *buffer = NULL;
    long bufflen = 260;
    long retlen = 0;
    bool isfound = false;
    uint8_t retry_cnt =0;
    NFCSTATUS status = NFCSTATUS_SUCCESS;

    buffer = (uint8_t*) phOsalNfc_GetMemory(bufflen*sizeof(uint8_t));
    if(NULL == buffer)
    {
        return NFCSTATUS_FAILED;
    }
    retry_send_ext:
    if(retry_cnt > 3)
    {
        if(buffer != NULL)
        {
            phOsalNfc_FreeMemory(buffer);
            buffer = NULL;
        }
        return NFCSTATUS_FAILED;
    }

    isfound = GetNxpTransitByteArrayValue(cityType, (char *) buffer,bufflen, &retlen);
    if (retlen > 0)
    {
        status = phNxpNciHal_send_ext_cmd(retlen, buffer);
        if (status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("transit configuration failed");
            retry_cnt++;
            goto retry_send_ext;
        }
    }
    if(isfound == false)
    {
        NXPLOG_NCIHAL_E("city not found");
    }
    if(buffer != NULL)
    {
        phOsalNfc_FreeMemory(buffer);
        buffer = NULL;
    }
    return status;
}
/******************************************************************************
 * Function         phNxpNciHal_set_transit_settings
 *
 * Description      This function is called to copy settings in to transitSettings
 *
 * Returns          Status.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_set_transit_settings(void* pSettings)
{
    phNxpNciTransitSettings_t* ptransitSettings = (phNxpNciTransitSettings_t*)pSettings;
    if(ptransitSettings == NULL)
        return NFCSTATUS_FAILED;

    phOsalNfc_MemCopy(&transitSettings, ptransitSettings, sizeof(transitSettings));

    return NFCSTATUS_SUCCESS;
}
/******************************************************************************
 * Function         phNxpNciHal_set_china_region_configs
 *
 * Description      This function is called to set china region specific configs
 *
 * Returns          Status.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_set_china_region_configs(void* pSettings)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int rf_val = 0;
    int flag_send_transit_config=TRUE;
    uint8_t retry_cnt =0;
    int enable_blk_num_chk_bit =0;
    phNxpNciTransitSettings_t* pTransitSettings = (phNxpNciTransitSettings_t*)pSettings;
    static uint8_t get_rf_cmd[] = {0x20, 0x03,0x03, 0x01, 0xA0, 0x85};

    if(pTransitSettings == NULL)
        return NFCSTATUS_FAILED;

retry_send_ext:
    if(retry_cnt > 3)
    {
        return NFCSTATUS_FAILED;
    }

    phNxpNciRfSet.isGetRfSetting = TRUE;
    status = phNxpNciHal_send_ext_cmd(sizeof(get_rf_cmd),get_rf_cmd);
    if(status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("unable to get the RF setting");
        phNxpNciRfSet.isGetRfSetting = FALSE;
        retry_cnt++;
        goto retry_send_ext;
    }
    phNxpNciRfSet.isGetRfSetting = FALSE;
    if(phNxpNciRfSet.p_rx_data[3] != 0x00)
    {
        NXPLOG_NCIHAL_E("GET_CONFIG_RSP is FAILED for CHINA TIANJIN");
        return status;
    }

    /*check if china block number check is required*/
    rf_val = phNxpNciRfSet.p_rx_data[8];
    enable_blk_num_chk_bit = rf_val & 0x40;
    if((enable_blk_num_chk_bit != 0x40) && (pTransitSettings->cfg_blk_chk_enable == 1))
    {
        phNxpNciRfSet.p_rx_data[8] |= 0x40;   // Enable if it is disabled
    }
    else if((enable_blk_num_chk_bit == 0x40) && (pTransitSettings->cfg_blk_chk_enable == 0))
    {
        phNxpNciRfSet.p_rx_data[8] &= ~0x40;  // Disable if it is Enabled
    }
    else
    {
        flag_send_transit_config = FALSE;  // No need to change in RF setting
    }
    if(flag_send_transit_config)
    {
        static uint8_t set_rf_cmd[] = {0x20, 0x02, 0x08, 0x01, 0xA0, 0x85, 0x04, 0x50, 0x08, 0x68, 0x00};
        memcpy(&set_rf_cmd[4],&phNxpNciRfSet.p_rx_data[5],7);
        status = phNxpNciHal_send_ext_cmd(sizeof(set_rf_cmd),set_rf_cmd);
        if(status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("unable to set the RF setting");
            retry_cnt++;
            goto retry_send_ext;
        }
    }

    return status;
}
/******************************************************************************
 * Function         phNxpNciHal_china_tianjin_rf_setting
 *
 * Description      This function is called to check RF Setting
 *
 * Returns          Status.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_china_tianjin_rf_setting(void)
{
    NFCSTATUS status = NFCSTATUS_SUCCESS;
    int isfound = 0;
    int rf_enable = FALSE;
    int rf_val = 0;
    int flag_send_tianjin_config;
    uint8_t retry_cnt =0;
    int enable_bit =0;
    static uint8_t get_rf_cmd[] = {0x20, 0x03,0x03, 0x01, 0xA0, 0x85};

retry_send_ext:
    if(retry_cnt > 3)
    {
        return NFCSTATUS_FAILED;
    }
    flag_send_tianjin_config = TRUE;
    phNxpNciRfSet.isGetRfSetting = TRUE;
    status = phNxpNciHal_send_ext_cmd(sizeof(get_rf_cmd),get_rf_cmd);
    if(status != NFCSTATUS_SUCCESS)
    {
        NXPLOG_NCIHAL_E("unable to get the RF setting");
        phNxpNciRfSet.isGetRfSetting = FALSE;
        retry_cnt++;
        goto retry_send_ext;
    }
    phNxpNciRfSet.isGetRfSetting = FALSE;
    if(phNxpNciRfSet.p_rx_data[3] != 0x00)
    {
        NXPLOG_NCIHAL_E("GET_CONFIG_RSP is FAILED for CHINA TIANJIN");
        return status;
    }
    rf_val = phNxpNciRfSet.p_rx_data[10];
    isfound = (GetNxpNumValue(CFG_NXP_CHINA_TIANJIN_RF_ENABLED, (void *)&rf_enable, sizeof(rf_enable)));
    if(isfound >0)
    {
        enable_bit = rf_val & 0x40;
#if(NXP_NFCC_MIFARE_TIANJIN == TRUE)
        if((enable_bit != 0x40) && (rf_enable == 1))
        {
            phNxpNciRfSet.p_rx_data[10] |= 0x40;   // Enable if it is disabled
        }
        else if((enable_bit == 0x40) && (rf_enable == 0))
        {
            phNxpNciRfSet.p_rx_data[10] &= 0xBF;  // Disable if it is Enabled
        }
        else
        {
            flag_send_tianjin_config = FALSE;  // No need to change in RF setting
        }
#else
        {
           enable_bit = phNxpNciRfSet.p_rx_data[11] & 0x10;
           if((rf_enable == 1) && (enable_bit != 0x10))
           {
               NXPLOG_NCIHAL_E("Setting Non-Mifare reader for china tianjin");
               phNxpNciRfSet.p_rx_data[11] |= 0x10;
           }else if ((rf_enable == 0) && (enable_bit == 0x10))
           {
               NXPLOG_NCIHAL_E("Setting Non-Mifare reader for china tianjin");
               phNxpNciRfSet.p_rx_data[11] &= 0xEF;
           }
           else
           {
               flag_send_tianjin_config = FALSE;
           }
        }
#endif

        if(flag_send_tianjin_config == TRUE)
        {
            static uint8_t set_rf_cmd[] = {0x20, 0x02, 0x08, 0x01, 0xA0, 0x85, 0x04, 0x50, 0x08, 0x68, 0x00};
            memcpy(&set_rf_cmd[4],&phNxpNciRfSet.p_rx_data[5],7);
            status = phNxpNciHal_send_ext_cmd(sizeof(set_rf_cmd),set_rf_cmd);
            if(status != NFCSTATUS_SUCCESS)
            {
                NXPLOG_NCIHAL_E("unable to set the RF setting");
                retry_cnt++;
                goto retry_send_ext;
            }
        }
    }

    return status;
}

int  check_config_parameter()
{
    //NFCSTATUS status = NFCSTATUS_FAILED;//unused
    uint8_t param_clock_src = CLK_SRC_PLL;
    if (nxpprofile_ctrl.bClkSrcVal == CLK_SRC_PLL)
    {
#if(NFC_NXP_CHIP_TYPE != PN553)
        param_clock_src = param_clock_src << 3;
#endif

        if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_13MHZ)
        {
            param_clock_src |= 0x00;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_19_2MHZ)
        {
            param_clock_src |= 0x01;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_24MHZ)
        {
            param_clock_src |= 0x02;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_26MHZ)
        {
            param_clock_src |= 0x03;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_38_4MHZ)
        {
            param_clock_src |= 0x04;
        }
        else if (nxpprofile_ctrl.bClkFreqVal == CLK_FREQ_52MHZ)
        {
            param_clock_src |= 0x05;
        }
        else
        {
            NXPLOG_NCIHAL_E("Wrong clock freq, send default PLL@19.2MHz");
            param_clock_src = 0x11;
        }
    }
    else if(nxpprofile_ctrl.bClkSrcVal == CLK_SRC_XTAL)
    {
        param_clock_src = 0x08;

    }
    else
    {
        NXPLOG_NCIHAL_E("Wrong clock source. Dont apply any modification")
    }
    return param_clock_src;
}

/******************************************************************************
 * Function         phNxpNciHal_check_factory_reset
 *
 * Description      This function is called at init time to check
 *                  the presence of ese related info. If file are not
 *                  present set the SWP_INT_SESSION_ID_CFG to FF to
 *                  force the NFCEE to re-run its initialization sequence.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_check_factory_reset(void)
{
    int ret = 0;
    NFCSTATUS status = NFCSTATUS_FAILED;
    const char config_eseinfo_path[] = "/data/nfc/nfaStorage.bin1";
    /*Changed part of warning fixes in wearable project*/
    UNUSED(config_eseinfo_path);
    static uint8_t reset_ese_session_identity_set[] = { 0x20, 0x02, 0x17, 0x02,
                                      0xA0, 0xEA, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                      0xA0, 0xEB, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#ifdef PN547C2_FACTORY_RESET_DEBUG
    static uint8_t reset_ese_session_identity[] = { 0x20, 0x03, 0x05, 0x02,
                                          0xA0, 0xEA, 0xA0, 0xEB};
#endif
#if 0
    if (stat(config_eseinfo_path, &st) == -1)
    {
        NXPLOG_NCIHAL_D("%s file not present = %s", __FUNCTION__, config_eseinfo_path);
        ret = -1;
    }
    else
    {
        ret = 0;
    }
#endif
    NXPLOG_NCIHAL_D("%s file not present = %s", __FUNCTION__, config_eseinfo_path);
        ret = -1;
    if(ret == -1)
    {
#ifdef PN547C2_FACTORY_RESET_DEBUG
        /* NXP ACT Proprietary Ext */
        status = phNxpNciHal_send_ext_cmd(sizeof(reset_ese_session_identity),
                                           reset_ese_session_identity);
        if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("NXP reset_ese_session_identity command failed");
        }
#endif
        status = phNxpNciHal_send_ext_cmd(sizeof(reset_ese_session_identity_set),
                                           reset_ese_session_identity_set);
        if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("NXP reset_ese_session_identity_set command failed");
        }
#ifdef PN547C2_FACTORY_RESET_DEBUG
        /* NXP ACT Proprietary Ext */
        status = phNxpNciHal_send_ext_cmd(sizeof(reset_ese_session_identity),
                                           reset_ese_session_identity);
        if (status != NFCSTATUS_SUCCESS) {
            NXPLOG_NCIHAL_E("NXP reset_ese_session_identity command failed");
        }
#endif

    }
}

/******************************************************************************
 * Function         phNxpNciHal_print_res_status
 *
 * Description      This function is called to process the response status
 *                  and print the status byte.
 *
 * Returns          void.
 *
 ******************************************************************************/
static void phNxpNciHal_print_res_status( uint8_t *p_rx_data, uint16_t *p_len)
{
    static uint8_t response_buf[][30] = {"STATUS_OK",
                                     "STATUS_REJECTED",
                                     "STATUS_RF_FRAME_CORRUPTED" ,
                                     "STATUS_FAILED" ,
                                     "STATUS_NOT_INITIALIZED" ,
                                     "STATUS_SYNTAX_ERROR",
                                     "STATUS_SEMANTIC_ERROR",
                                     "RFU",
                                     "RFU",
                                     "STATUS_INVALID_PARAM",
                                     "STATUS_MESSAGE_SIZE_EXCEEDED",
                                     "STATUS_UNDEFINED"};
    int status_byte;
    /*Changed part of warning fixes in wearable project*/
    UNUSED(response_buf);
    UNUSED(status_byte);
    if(p_rx_data[0] == 0x40 && (p_rx_data[1] == 0x02 || p_rx_data[1] == 0x03))
    {
        if(p_rx_data[2]  &&  p_rx_data[3]<=10)
        {
            status_byte = p_rx_data[CORE_RES_STATUS_BYTE];
            NXPLOG_NCIHAL_D("%s: response status =%s",__FUNCTION__,response_buf[status_byte]);
        }
        else
        {
            NXPLOG_NCIHAL_D("%s: response status =%s",__FUNCTION__,response_buf[11]);
        }
        if(phNxpNciClock.isClockSet)
        {
            int i;
            for(i=0; i<* p_len; i++)
            {
                phNxpNciClock.p_rx_data[i] = p_rx_data[i];
            }
        }

        if(phNxpNciRfSet.isGetRfSetting)
        {
            int i;
            for(i=0; i<* p_len; i++)
            {
                phNxpNciRfSet.p_rx_data[i] = p_rx_data[i];
                //NXPLOG_NCIHAL_D("%s: response status =0x%x",__FUNCTION__,p_rx_data[i]);
            }

        }
    }

    if((p_rx_data[2])&&(config_access == TRUE))
    {
        if(p_rx_data[3]!=NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_W("Invalid Data from config file . Aborting..");
            phNxpNciHal_close();
        }
    }
}

/*****************************************************************************
 * Function         phNxpNciHal_send_get_cfgs
 *
 * Description      This function is called to  send get configs
 *                  for all the types in get_cfg_arr.
 *                  Response of getConfigs(EEPROM stored) will be
 *                  compared with request coming from MW during discovery.
 *                  If same, then current setConfigs will be dropped
 *
 * Returns          Returns NFCSTATUS_SUCCESS if sending cmd is successful and
 *                  response is received.
 *
 *****************************************************************************/
NFCSTATUS phNxpNciHal_send_get_cfgs()
{
    NXPLOG_NCIHAL_D("%s Enter", __FUNCTION__);
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t num_cfgs = sizeof(get_cfg_arr) / sizeof(uint8_t);
    uint8_t cfg_count = 0,retry_cnt = 0;
    if(mGetConfig_info != NULL)
        mGetConfig_info->isGetcfg = TRUE;

    uint8_t cmd_get_cfg[] = {0x20, 0x03, 0x02, 0x01, 0x00};

    while(cfg_count < num_cfgs)
    {
        cmd_get_cfg[sizeof(cmd_get_cfg) - 1] = get_cfg_arr[cfg_count];

        retry_get_cfg:
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_get_cfg), cmd_get_cfg);
        if(status != NFCSTATUS_SUCCESS && retry_cnt < 3){
            NXPLOG_NCIHAL_E("cmd_get_cfg failed");
            retry_cnt++;
            goto retry_get_cfg;
        }
        if(retry_cnt == 3)
        {
            break;
        }
        cfg_count++;
        retry_cnt = 0;
    }
    if(mGetConfig_info != NULL)
        mGetConfig_info->isGetcfg = FALSE;
    return status;
}
NFCSTATUS phNxpNciHal_send_nfcee_pwr_cntl_cmd (uint8_t type)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    uint8_t cmd_buf[] = {0x22, 0x03, 0x02, 0xC0, 0x00};
    uint8_t retry_cnt = 0;

    NXPLOG_NCIHAL_D("phNxpNciHal_send_nfcee_pwr_cntl_cmd; enter");
    cmd_buf[sizeof(cmd_buf)-1] = type;
    while(status != NFCSTATUS_SUCCESS && retry_cnt < 3){
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_buf), cmd_buf);
        if(status != NFCSTATUS_SUCCESS)
        {
            NXPLOG_NCIHAL_E("cmd_nfcee_pwr_cntl_cfg failed");
            retry_cnt++;
        }
    }
    retry_cnt = 0;
    return status;
}

/******************************************************************************
 * Function         phNxpNciHal_get_jcop_state
 *
 * Description      This function is called to retrieve the JCOP update state.
 *
 * Returns          void.
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_get_jcop_state(uint8_t *pState)
{
    NFCSTATUS status = NFCSTATUS_FAILED;
    //EEPROM access variables
    phNxpNci_EEPROM_info_t mEEPROM_info = {0};
    mEEPROM_info.buffer = pState;
    mEEPROM_info.bufflen= 1;
    mEEPROM_info.request_type = EEPROM_JCOP_STATE;
    mEEPROM_info.request_mode = GET_EEPROM_DATA;
    status = request_EEPROM(&mEEPROM_info);
    /* JCOP state in EEPROM is valid only if the upper nibble is 0x1 */
    /* For the first time, initialize it as completed state */
    if(NFCSTATUS_SUCCESS == status && ((*pState & 0xF0) != 0x10))
    {
        *pState = NCI_HAL_OSU_COMPLETE;
        status = phNxpNciHal_set_jcop_state(*pState);
        NXPLOG_NCIHAL_D("Initializing JCOP download state in EEPROM to %d", *pState);
    }
    else
    {
        *pState = *pState & 0x0F; /* Upper nibble is a validity indicator, lower nibble is the JCOP state */
    }
    return status;
}

/******************************************************************************
 * Function         phNxpNciHal_set_jcop_state
 *
 * Description      This function is called to set the JCOP update state.
 *
 * Note:            If RF discovery is started, NFA_StopRfDiscovery()/NFA_RF_DISCOVERY_STOPPED_EVT
 *                  should happen before calling this function
 *
 * Returns          NFCSTATUS
 *
 ******************************************************************************/
NFCSTATUS phNxpNciHal_set_jcop_state(uint8_t state)
{
    //EEPROM access variables
    phNxpNci_EEPROM_info_t mEEPROM_info = {0};
    state = (state & 0x0F) | 0x10; /* Upper nibble is marked as 1 indicating that this byte
                                      of EEPROM was indeed written before its read */
    mEEPROM_info.buffer = &state;
    mEEPROM_info.bufflen= 1;
    mEEPROM_info.request_type = EEPROM_JCOP_STATE;
    mEEPROM_info.request_mode = SET_EEPROM_DATA;
    return request_EEPROM(&mEEPROM_info);
}

int mifare_flag = 0;
NFCSTATUS phNxpNciHal_MifareSetKey(void)
{
    int correct = 0;
    /* MFC Authentication Key A */
    static uint8_t cmd_auth_key_0[] = {0x00, 0x00, 0x03, 0x40, 0x00, 0x00};
    static uint8_t cmd_auth_key_2[] = {0x00, 0x00, 0x03, 0x40, 0x00, 0x02};
    static uint8_t cmd_discovery_select_cmd[] = {0x21, 0x04, 0x03, 0x01, 0x80, 0x80};
    static uint8_t cmd_deactive_cmd[] = {0x21, 0x06, 0x01, 0x01};
    uint8_t cmd_mfc_read[] = {0x00, 0x00, 0x03, 0x10, 0x30, 0x03};

    LOG_DEBUG("Mifare Set Key.\r\n");

    NFCSTATUS status = NFCSTATUS_SUCCESS;

    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_deactive_cmd),cmd_deactive_cmd);
    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_discovery_select_cmd),cmd_discovery_select_cmd);
    mifare_flag = 1;
    status = phNxpNciHal_send_ext_cmd(sizeof(cmd_auth_key_0),cmd_auth_key_0);
    mifare_flag = 0;
    if (status == 0xA0) {
        /* Try another key */
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_deactive_cmd),cmd_deactive_cmd);
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_discovery_select_cmd),cmd_discovery_select_cmd);
        mifare_flag = 1;
    	status = phNxpNciHal_send_ext_cmd(sizeof(cmd_auth_key_2),cmd_auth_key_2);
        mifare_flag = 0;
        if (status == 0xA0) {
          LOG_DEBUG("Mifare Try key fail.\r\n");
    	} else {
    	    LOG_DEBUG("Mifare Key Correct 2.\r\n");
            correct = 1;
    	}
    } else {
        LOG_DEBUG("Mifare Key Correct 0.\r\n");
        correct = 1;
    }


    if (correct) {
        cmd_mfc_read[5] = 0x03;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 0x00;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 0x01;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 0x02;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);

        cmd_mfc_read[5] = 0x07;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 0x04;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 0x05;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 0x06;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);

        cmd_mfc_read[5] = 11;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 8;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 9;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 10;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);

        cmd_mfc_read[5] = 15;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 12;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 13;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);
        cmd_mfc_read[5] = 14;
        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_mfc_read),cmd_mfc_read);

        status = phNxpNciHal_send_ext_cmd(sizeof(cmd_deactive_cmd),cmd_deactive_cmd);
    }

    

    return status;
}


