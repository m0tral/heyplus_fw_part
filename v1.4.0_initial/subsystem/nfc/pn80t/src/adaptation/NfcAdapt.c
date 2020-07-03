/******************************************************************************
 *
 *  Copyright (C) 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 ******************************************************************************/
#include "phNxpNciHal.h"
#include "OverrideLog.h"
#include "NfcAdapt.h"

#include "gki.h"

#include "nfa_api.h"
#include "nfc_int.h"
#include "nfc_target.h"
#include "vendor_cfg.h"
#include "config.h"
#include "android_logmsg.h"
#include "phNxpNciHal_Adaptation.h"
#include "phNfcStatus.h"
#include "phNxpConfig.h"
#include "phDnldNfc.h"
#include "nfa_sys.h"
#include "nfa_dm_int.h"

#undef LOG_TAG
#define LOG_TAG "NfcAdapt"

// used ONLY for firmware download synchronization
static GKI_tBinarySem mHalOpenCompletedEvent;
static GKI_tBinarySem mHalCloseCompletedEvent;
#if (NXP_EXTNS == TRUE)
static GKI_tBinarySem mHalCoreResetCompletedEvent;
static GKI_tBinarySem mHalCoreInitCompletedEvent;
static GKI_tBinarySem mHalInitCompletedEvent;
static BOOLEAN isHalCloseComplete = FALSE;
static UINT8 evt_status;
static UINT8 hal_status;
#endif

void GKI_shutdown(void);
static UINT32 NfcA_StartNfcTask (void);
static void HalInitialize (void);
static void HalTerminate (void);
static void HalOpen (tHAL_NFC_CBACK *p_hal_cback, tHAL_NFC_DATA_CBACK* p_data_cback);
static void HalClose (void);
static void HalDeviceContextCallback (nfc_event_t event, nfc_status_t event_status);
static void HalDeviceContextDataCallback (uint16_t data_len, uint8_t* p_data);
static void HalWrite (UINT16 data_len, UINT8* p_data);
static int HalIoctl (long arg, void* p_data);
static void HalCoreInitialized (UINT8* p_core_init_rsp_params);
static BOOLEAN HalPrediscover(void);
static void HalControlGranted(void);
static void HalPowerCycle(void);
static UINT8 HalGetMaxNfcee(void);
static void HalDownloadFirmwareCallback (nfc_event_t event, nfc_status_t event_status);
static void HalDownloadFirmwareDataCallback (uint16_t data_len, uint8_t* p_data);

tHAL_NFC_ENTRY   mHalEntryFuncs; // function pointers for HAL entry points

static tHAL_NFC_CBACK* mHalCallback = NULL;
tHAL_NFC_DATA_CBACK* mHalDataCallback = NULL;

UINT32 ScrProtocolTraceFlag = SCR_PROTO_TRACE_ALL; //0x017F00;
UINT8 appl_trace_level = 0xff;
#if (NXP_EXTNS == TRUE)
UINT8 appl_dta_mode_flag = 0x00;
#endif
char bcm_nfc_location[120];
char nci_hal_module[64];

static UINT8 nfa_proprietary_cfg[sizeof ( tNFA_PROPRIETARY_CFG )];

extern tNFA_PROPRIETARY_CFG *p_nfa_proprietary_cfg;
extern UINT8 nfa_ee_max_ee_cfg;
extern const UINT8  nfca_version_string [];
extern const UINT8  nfa_version_string [];

extern tNFA_HCI_CFG *p_nfa_hci_cfg;
extern BOOLEAN nfa_poll_bail_out_mode;

/* NFC Task Control structure */
phNfctask_Control_t nfc_ctrl;
/*******************************************************************************
**
** Function:    NfcA_SetIsFactoryResetRequired()
**
** Description: sets is factory reset required flag in HAL.
**
** Returns:     none
**
*******************************************************************************/
void NfcA_SetIsFactoryResetRequired(BOOLEAN bFactoryReset)
{
    phNxpNciHal_is_factory_reset_required(bFactoryReset);
}
UINT16 NfcA_Set_China_Region_Configs(void* pSettings)
{
    return phNxpNciHal_set_china_region_configs(pSettings);
}

UINT16 NfcA__Reset_Nfcc_Fw(void)
{
    return phNxpNciHal_reset_nfcc_fw();
}
void NfcA_Set_Uicc_Listen_Mask(UINT8 mask)
{
    nfa_dm_set_uicc_listen_mask(mask);
}

BOOLEAN NfcA_HalIsFirmwareUptodate(void)
{
    return phNxpNciHal_is_firmware_uptodate();
}

/*******************************************************************************
**
** Function:    NfcA_Initialize()
**
** Description: class initializer
**
** Returns:     none
**
*******************************************************************************/
void NfcA_Initialize (void)
{
    #if defined(DEBUG)
    const char* func = "NfcA_Initialize";
    #endif
    unsigned long num;
    long retlen = 0;

    ALOGD("%s: enter", func);
    ALOGE("%s: ver=%s nfa=%s", func, nfca_version_string, nfa_version_string);
    memset (&mHalEntryFuncs, 0, sizeof(mHalEntryFuncs));
    //--------------------------------------
    // Initialize access control variables
    mHalOpenCompletedEvent = GKI_binary_sem_init();
    mHalCloseCompletedEvent = GKI_binary_sem_init();
    mHalCoreResetCompletedEvent = GKI_binary_sem_init();
    mHalCoreInitCompletedEvent = GKI_binary_sem_init();
    mHalInitCompletedEvent = GKI_binary_sem_init();

    //-------------------------------------
    // Read the configuration information
    if ( GetNumValue ( CFG_PROTOCOL_TRACE_LEVEL, &num, sizeof ( num ) ) )
        ScrProtocolTraceFlag = num;

    if ( GetNumValue ( CFG_NFA_MAX_EE_SUPPORTED, &num, sizeof ( num ) ) )
    {
        nfa_ee_max_ee_cfg = num;
        ALOGD("%s: Overriding NFA_EE_MAX_EE_SUPPORTED to use %d", func, nfa_ee_max_ee_cfg);
    }
    else
    {
        nfa_ee_max_ee_cfg = NFA_HCI_MAX_HOST_IN_NETWORK;
        ALOGD("%s: NFA_EE_MAX_EE_SUPPORTED default value to use %d", func, nfa_ee_max_ee_cfg);
    }

    if ( GetNumValue ( CFG_NFA_POLL_BAIL_OUT_MODE, &num, sizeof ( num ) ) )
    {
        nfa_poll_bail_out_mode = num;
        ALOGD("%s: Overriding NFA_POLL_BAIL_OUT_MODE to use %d", func, nfa_poll_bail_out_mode);
    }

    if ( TRUE == GetNxpByteArrayValue ( CFG_NFA_PROPRIETARY_CFG, (char*)nfa_proprietary_cfg, sizeof ( tNFA_PROPRIETARY_CFG ), &retlen ) )
    {
        if(retlen > 0)
        {
            p_nfa_proprietary_cfg = (tNFA_PROPRIETARY_CFG*) &nfa_proprietary_cfg[0];
        }
    }

    initializeGlobalAppLogLevel ();

    //-----------------------------------
    // starting up the tasks
    GKI_init ();
    GKI_enable ();
#ifdef ANDROID
    nfc_ctrl.pMsgQHandle = phDal4Nfc_msgget(0, 0600);
#else
    nfc_ctrl.pMsgQHandle = phDal4Nfc_msgget();
#endif
	//gNfcTaskMsgQHandle = nfc_ctrl.pMsgQHandle;
    nfc_ctrl.nfc_task_sem = GKI_binary_sem_init();

    NfcA_StartNfcTask();

    //----------------------------------
    // Initialize the HAL
    NfcA_InitializeHalDeviceContext ();
    ALOGD ("%s: exit", func);
}

/*******************************************************************************
**
** Function:    NfcA_Finalize()
**
** Description: class finalizer
**
** Returns:     none
**
*******************************************************************************/
void NfcA_Finalize(void)
{
    ALOGD ("%s: enter", __func__);

    if(isHalCloseComplete == FALSE)
    {
    	// wait till hal and tml layers are cleanedup.
    	GKI_binary_sem_wait(mHalCloseCompletedEvent);
    }
    GKI_shutdown ();

    // wait till nfc task is killed
    GKI_binary_sem_wait(nfc_ctrl.nfc_task_sem);

    mHalCallback = NULL;
    mHalDataCallback = NULL;
    memset (&mHalEntryFuncs, 0, sizeof(mHalEntryFuncs));

    GKI_binary_sem_post(mHalOpenCompletedEvent);
    GKI_binary_sem_post(mHalCloseCompletedEvent);
    GKI_binary_sem_post(mHalCoreResetCompletedEvent);
    GKI_binary_sem_post(mHalCoreInitCompletedEvent);
    GKI_binary_sem_post(mHalInitCompletedEvent);
    GKI_binary_sem_post(nfc_ctrl.nfc_task_sem);

    GKI_binary_sem_destroy(mHalOpenCompletedEvent);
    GKI_binary_sem_destroy(mHalCloseCompletedEvent);
    GKI_binary_sem_destroy(mHalCoreResetCompletedEvent);
    GKI_binary_sem_destroy(mHalCoreInitCompletedEvent);
    GKI_binary_sem_destroy(mHalInitCompletedEvent);
    GKI_binary_sem_destroy(nfc_ctrl.nfc_task_sem);


    mHalOpenCompletedEvent = NULL;
    mHalCloseCompletedEvent = NULL;
    mHalCoreResetCompletedEvent = NULL;
    mHalCoreInitCompletedEvent = NULL;
    mHalInitCompletedEvent = NULL;
    nfc_ctrl.nfc_task_sem = NULL;

    phDal4Nfc_msgrelease(nfc_ctrl.pMsgQHandle);

    ALOGD ("%s: exit", __func__);
}

/* NFCA_TASK is removed out to reduce the number of tasks */

/*******************************************************************************
**
** Function:    nfca_StartNfcTask()
**
** Description: Creates work threads
**
** Returns:     none
**
*******************************************************************************/
UINT32 NfcA_StartNfcTask ()
{
    const char* func = "NfcA_Thread";
    UNUSED(func);
    ALOGD ("%s: enter", func);

    {
    	//GKI_tBinarySem startWaitSem;

    	//startWaitSem = GKI_binary_sem_init();

    	// The passed in CondVar are used to make sure that this thread does not continue until the nfc_task
    	// has been created and is actually running.  The nfc_task contains a signal call (GKI_wait()) that
    	// signals the conditional variable that the nfc_task has started

#ifdef ANDROID
        GKI_create_task ((TASKPTR)nfc_task, NFC_TASK, (INT8*)"NFC_TASK", 0, NFC_TASK_STACK_SIZE, NULL);
#else
        GKI_create_task (nfc_task, NFC_TASK, (INT8*)"NFC_TASK", 0, NFC_TASK_STACK_SIZE, &nfc_ctrl);
#endif

        //GKI_binary_sem_wait(startWaitSem);+
    }

    ALOGD ("%s: exit", func);
    return (UINT32) NULL;
}

/*******************************************************************************
**
** Function:    NfcA_GetHalEntryFuncs()
**
** Description: Get the set of HAL entry points.
**
** Returns:     Functions pointers for HAL entry points.
**
*******************************************************************************/
tHAL_NFC_ENTRY* NfcA_GetHalEntryFuncs (void)
{
    return &mHalEntryFuncs;
}

/*******************************************************************************
**
** Function:    NfcA_InitializeHalDeviceContext
**
** Description: Ask the generic Android HAL to find the Broadcom-specific HAL.
**
** Returns:     None.
**
*******************************************************************************/
void NfcA_InitializeHalDeviceContext (void)
{
    ALOGD ("%s: enter", __func__);

    mHalCallback =  NULL;
    mHalDataCallback = NULL;
    memset (&mHalEntryFuncs, 0, sizeof(mHalEntryFuncs));

    mHalEntryFuncs.initialize = HalInitialize;
    mHalEntryFuncs.terminate = HalTerminate;
    mHalEntryFuncs.open = HalOpen;
    mHalEntryFuncs.close = HalClose;
    mHalEntryFuncs.core_initialized = HalCoreInitialized;
    mHalEntryFuncs.write = HalWrite;
    mHalEntryFuncs.ioctl = HalIoctl;
    mHalEntryFuncs.prediscover = HalPrediscover;
    mHalEntryFuncs.control_granted = HalControlGranted;
    mHalEntryFuncs.power_cycle = HalPowerCycle;
    mHalEntryFuncs.get_max_ee = HalGetMaxNfcee;

    ALOGD ("%s: exit", __func__);
}
void NfcA_HalCleanup()
{
    ALOGD ("%s", __func__);
    phNxpNciHal_Cleanup();
    isHalCloseComplete = TRUE;
    GKI_binary_sem_post(mHalCloseCompletedEvent);
}
/*******************************************************************************
**
** Function:    NfcA_HalInitialize
**
** Description: Not implemented because this function is only needed
**              within the HAL.
**
** Returns:     None.
**
*******************************************************************************/
void HalInitialize ()
{
    ALOGD ("%s", __func__);
}

/*******************************************************************************
**
** Function:    NfcA_HalTerminate
**
** Description: Not implemented because this function is only needed
**              within the HAL.
**
** Returns:     None.
**
*******************************************************************************/
void HalTerminate ()
{
    ALOGD ("%s", __func__);
}

/*******************************************************************************
**
** Function:    NfcA_HalOpen
**
** Description: Turn on controller, download firmware.
**
** Returns:     None.
**
*******************************************************************************/
void HalOpen (tHAL_NFC_CBACK *p_hal_cback, tHAL_NFC_DATA_CBACK* p_data_cback)
{
    ALOGD ("%s", __func__);
    mHalCallback = p_hal_cback;
    mHalDataCallback = p_data_cback;
    phNxpNciHal_open(HalDeviceContextCallback, HalDeviceContextDataCallback);
    isHalCloseComplete = FALSE;
}

int  HalOpen_PostFwDnd (void)
{
    ALOGD ("%s", __func__);
    return phNxpNciHal_keep_read_pending();
}

/*******************************************************************************
**
** Function:    NfcA_HalClose
**
** Description: Turn off controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalClose ()
{
    ALOGD ("%s", __func__);
    phNxpNciHal_close();
    isHalCloseComplete = TRUE;
    GKI_binary_sem_post(mHalCloseCompletedEvent);
}

/*******************************************************************************
**
** Function:    NfcA_HalDeviceContextCallback
**
** Description: Translate generic Android HAL's callback into Broadcom-specific
**              callback function.
**
** Returns:     None.
**
*******************************************************************************/
void HalDeviceContextCallback (nfc_event_t event, nfc_status_t event_status)
{
    ALOGD ("%s: event=%u", __func__, event);
    if (mHalCallback)
        mHalCallback (event, (tHAL_NFC_STATUS) event_status);
}

/*******************************************************************************
**
** Function:    NfcA_HalDeviceContextDataCallback
**
** Description: Translate generic Android HAL's callback into Broadcom-specific
**              callback function.
**
** Returns:     None.
**
*******************************************************************************/
void HalDeviceContextDataCallback (uint16_t data_len, uint8_t* p_data)
{
    ALOGD ("%s: len=%u", __func__, data_len);
    if (mHalDataCallback)
        mHalDataCallback (data_len, p_data);
}

/*******************************************************************************
**
** Function:    NfcA_HalWrite
**
** Description: Write NCI message to the controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalWrite (UINT16 data_len, UINT8* p_data)
{
    ALOGD ("%s", __func__);
    phNxpNciHal_write(data_len, p_data);
}

#if(NXP_EXTNS == TRUE)
/*******************************************************************************
**
** Function:    NfcA_HalIoctl
**
** Description: Calls ioctl to the Nfc driver.
**              If called with a arg value of 0x01 than wired access requested,
**              status of the requst would be updated to p_data.
**              If called with a arg value of 0x00 than wired access will be
**              released, status of the requst would be updated to p_data.
**              If called with a arg value of 0x02 than current p61 state would be
**              updated to p_data.
**
** Returns:     -1 or 0.
**
*******************************************************************************/
int HalIoctl (long arg, void* p_data)
{
    ALOGD ("%s", __func__);
    return phNxpNciHal_ioctl (arg, p_data);
}

#endif

/*******************************************************************************
**
** Function:    NfcA_HalCoreInitialized
**
** Description: Adjust the configurable parameters in the controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalCoreInitialized (UINT8* p_core_init_rsp_params)
{
    ALOGD ("%s", __func__);
   	phNxpNciHal_core_initialized(p_core_init_rsp_params);
}

/*******************************************************************************
**
** Function:    NfcA_HalPrediscover
**
** Description:     Perform any vendor-specific pre-discovery actions (if needed)
**                  If any actions were performed TRUE will be returned, and
**                  HAL_PRE_DISCOVER_CPLT_EVT will notify when actions are
**                  completed.
**
** Returns:          TRUE if vendor-specific pre-discovery actions initialized
**                  FALSE if no vendor-specific pre-discovery actions are needed.
**
*******************************************************************************/
BOOLEAN HalPrediscover ()
{
    ALOGD ("%s", __func__);
    BOOLEAN retval = FALSE;

    retval = phNxpNciHal_pre_discover();

    return retval;
}

/*******************************************************************************
**
** Function:        NfcA_HalControlGranted
**
** Description:     Grant control to HAL control for sending NCI commands.
**                  Call in response to HAL_REQUEST_CONTROL_EVT.
**                  Must only be called when there are no NCI commands pending.
**                  HAL_RELEASE_CONTROL_EVT will notify when HAL no longer
**                  needs control of NCI.
**
** Returns:         void
**
*******************************************************************************/
void HalControlGranted ()
{
    ALOGD ("%s", __func__);
    phNxpNciHal_control_granted();
}

/*******************************************************************************
**
** Function:    NfcA_HalPowerCycle
**
** Description: Turn off and turn on the controller.
**
** Returns:     None.
**
*******************************************************************************/
void HalPowerCycle ()
{
    ALOGD ("%s", __func__);
    phNxpNciHal_power_cycle();
}

/*******************************************************************************
**
** Function:    NfcA_HalGetMaxNfcee
**
** Description: Turn off and turn on the controller.
**
** Returns:     None.
**
*******************************************************************************/
UINT8 HalGetMaxNfcee()
{
    ALOGD ("%s", __func__);
    return nfa_ee_max_ee_cfg;
}

/*******************************************************************************
**
** Function:    NfcA_DownloadFirmware
**
** Description: Download firmware patch files.
**
** Returns:     None.
**
*******************************************************************************/
UINT8 NfcA_DownloadFirmware (void)
{
    ALOGD ("%s: enter", __func__);
    tNFC_FWUpdate_Info_t fw_update_inf;
    UINT16 fw_dwnld_status = NFC_STATUS_FAILED;
    hal_status = NFC_STATUS_FAILED;

    HalInitialize ();

    ALOGD ("%s: try open HAL", __func__);
    HalOpen (HalDownloadFirmwareCallback, HalDownloadFirmwareDataCallback);
    GKI_binary_sem_wait(mHalOpenCompletedEvent);
    if(hal_status != NFC_STATUS_OK)
    {
        goto End;
    }
    mHalEntryFuncs.ioctl(HAL_NFC_IOCTL_CHECK_FLASH_REQ, &fw_update_inf);
    NFC_TRACE_DEBUG1 ("fw_update required  -> %d", fw_update_inf.fw_update_reqd);
    if(fw_update_inf.fw_update_reqd == TRUE)
    {
        mHalEntryFuncs.ioctl(HAL_NFC_IOCTL_FW_DWNLD, &fw_dwnld_status);
        if (fw_dwnld_status !=  NFC_STATUS_OK)
        {
            ALOGD ("%s: FW Download failed", __func__);
        }
    }
    /*Skipping additional core reset and core init command sequence and invocation of halcoreinitialized as it will be
     * handled during SeAPi_init post to firmware download*/
    End:
    ALOGD ("%s: try close HAL", __func__);
    HalClose ();
    GKI_binary_sem_wait(mHalCloseCompletedEvent);
    return fw_dwnld_status;
}


/*******************************************************************************
**
** Function:    NfcA_DownloadFirmware
**
** Description: Download firmware patch files.
**
** Returns:     None.
**
*******************************************************************************/
UINT8 NfcA_ForcedFwDnd (void)
{
    ALOGD ("%s: enter", __func__);
    tNFC_FWUpdate_Info_t fw_update_inf;
    UINT8 fw_dwnld_status = NFC_STATUS_FAILED;
    hal_status = NFC_STATUS_FAILED;
    HalInitialize ();

    ALOGD ("%s: try open HAL", __func__);
    HalOpen (HalDownloadFirmwareCallback, HalDownloadFirmwareDataCallback);
    GKI_binary_sem_wait(mHalOpenCompletedEvent);

    if(hal_status != NFC_STATUS_OK)
    {
        goto End;
    }

    mHalEntryFuncs.ioctl(HAL_NFC_IOCTL_CHECK_FLASH_REQ, &fw_update_inf);
    NFC_TRACE_DEBUG1 ("fw_update required  -> %d", fw_update_inf.fw_update_reqd);
    mHalEntryFuncs.ioctl(HAL_NFC_IOCTL_FORCED_FW_DWNLD, &fw_dwnld_status);
    if (fw_dwnld_status !=  NFC_STATUS_OK)
    {
        ALOGD ("%s: FW Download failed", __func__);
    }

    /*Skipping additional core reset and core init command sequence and invocation of halcoreinitialized as it will be
     * handled during SeAPi_init post to firmware download*/
    End:
    ALOGD ("%s: try close HAL", __func__);
    HalClose ();
    GKI_binary_sem_wait(mHalCloseCompletedEvent);
    ALOGD ("%s: exit", __func__);
    return fw_dwnld_status;
}

UINT8 NfcA_PrepDownloadFirmware (void)
{
    ALOGD ("%s: enter", __func__);
    HalInitialize ();
    ALOGD ("%s: try open HAL", __func__);
    hal_status = NFC_STATUS_FAILED;
    HalOpen(HalDownloadFirmwareCallback, HalDownloadFirmwareDataCallback);
    GKI_binary_sem_wait(mHalOpenCompletedEvent);
    return hal_status;
    ALOGD ("%s: exit", __func__);
}
int NfcA_PostDownloadFirmware(void)
{
    ALOGD ("%s: enter", __func__);
    int status = NFCSTATUS_OK;
    //keep read pending
    if(HalOpen_PostFwDnd() != NFCSTATUS_PENDING)
    {
    	status = NFCSTATUS_FAILED;
    }
    /*Skipping additional core reset and core init command sequence and invocation of halcoreinitialized as it will be
     * handled during SeAPi_init post to firmware download*/
    ALOGD ("%s: try close HAL", __func__);
    HalClose ();
    GKI_binary_sem_wait(mHalCloseCompletedEvent);

    ALOGD ("%s: exit", __func__);
    return status;
}

/*******************************************************************************
**
** Function:    NfcA_HalDownloadFirmwareCallback
**
** Description: Receive events from the HAL.
**
** Returns:     None.
**
*******************************************************************************/
void HalDownloadFirmwareCallback (nfc_event_t event, nfc_status_t event_status)
{
    ALOGD ("%s: event=0x%X", __func__, event);
    hal_status = event_status;
    switch (event)
    {
    case HAL_NFC_OPEN_CPLT_EVT:
        {
            ALOGD ("%s: HAL_NFC_OPEN_CPLT_EVT", __func__);
            GKI_binary_sem_post(mHalOpenCompletedEvent);
            break;
        }
    case HAL_NFC_POST_INIT_CPLT_EVT:
        {
            ALOGD ("%s: HAL_NFC_POST_INIT_CPLT_EVT", __func__);
            GKI_binary_sem_post(mHalInitCompletedEvent);
            break;
        }
    case HAL_NFC_CLOSE_CPLT_EVT:
        {
            ALOGD ("%s: HAL_NFC_CLOSE_CPLT_EVT", __func__);
            GKI_binary_sem_post(mHalCloseCompletedEvent);
            break;
        }
    }
}

/*******************************************************************************
**
** Function:    NfcA_HalDownloadFirmwareDataCallback
**
** Description: Receive data events from the HAL.
**
** Returns:     None.
**
*******************************************************************************/
void HalDownloadFirmwareDataCallback (uint16_t data_len, uint8_t* p_data)
{
#if (NXP_EXTNS == TRUE)
    if (data_len > 3)
    {
        evt_status = NFC_STATUS_OK;
        if (p_data[0] == 0x40 && p_data[1] == 0x00)
        {
        	GKI_binary_sem_post(mHalCoreResetCompletedEvent);
        }
        else if (p_data[0] == 0x40 && p_data[1] == 0x01)
        {
        	GKI_binary_sem_post(mHalCoreInitCompletedEvent);
        }
    }
    else
    {
        evt_status = NFC_STATUS_FAILED;
        GKI_binary_sem_post(mHalCoreResetCompletedEvent);
        GKI_binary_sem_post(mHalCoreInitCompletedEvent);
    }
#endif
}
