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
#ifndef _PHNXPNCIHAL_H_
#define _PHNXPNCIHAL_H_

#include <hardware/nfc.h>
#include <phNxpNciHal_utils.h>




typedef uint8_t nfc_event_t;
typedef uint8_t nfc_status_t;




/*
 * The callback passed in from the NFC stack that the HAL
 * can use to pass events back to the stack.
 */
typedef void (nfc_stack_callback_t)(nfc_event_t event,
		nfc_status_t event_status);

/*
 * The callback passed in from the NFC stack that the HAL
 * can use to pass incomming data to the stack.
 */
typedef void (nfc_stack_data_callback_t)(uint16_t data_len, uint8_t* p_data);

/* nfc_nci_device_t starts with a hw_device_t struct,
 * followed by device-specific methods and members.
 *
 * All methods in the NCI HAL are asynchronous.
 */
typedef struct nfc_nci_device {
	/**
	 * Common methods of the NFC NCI device.  This *must* be the first member of
	 * nfc_nci_device_t as users of this structure will cast a hw_device_t to
	 * nfc_nci_device_t pointer in contexts where it's known the hw_device_t references a
	 * nfc_nci_device_t.
	 */
	/*Hardware patch : following line was in comment*/
	struct hw_device_t common;
	/*
	 * (*open)() Opens the NFC controller device and performs initialization.
	 * This may include patch download and other vendor-specific initialization.
	 *
	 * If open completes successfully, the controller should be ready to perform
	 * NCI initialization - ie accept CORE_RESET and subsequent commands through
	 * the write() call.
	 *
	 * If open() returns 0, the NCI stack will wait for a HAL_NFC_OPEN_CPLT_EVT
	 * before continuing.
	 *
	 * If open() returns any other value, the NCI stack will stop.
	 *
	 */

	int (*open)(const struct nfc_nci_device *p_dev,
			nfc_stack_callback_t *p_cback,
			nfc_stack_data_callback_t *p_data_cback);


	/*
	 * (*write)() Performs an NCI write.
	 *
	 * This method may queue writes and return immediately. The only
	 * requirement is that the writes are executed in order.
	 */
	int (*write)(const struct nfc_nci_device *p_dev, uint16_t data_len,
			const uint8_t *p_data);

	/*
	 * (*core_initialized)() is called after the CORE_INIT_RSP is received from the NFCC.
	 * At this time, the HAL can do any chip-specific configuration.
	 *
	 * If core_initialized() returns 0, the NCI stack will wait for a HAL_NFC_POST_INIT_CPLT_EVT
	 * before continuing.
	 *
	 * If core_initialized() returns any other value, the NCI stack will continue
	 * immediately.
	 */
	int (*core_initialized)(const struct nfc_nci_device *p_dev,
			uint8_t* p_core_init_rsp_params);

	/*
	 * (*pre_discover)() Is called every time before starting RF discovery.
	 * It is a good place to do vendor-specific configuration that must be
	 * performed every time RF discovery is about to be started.
	 *
	 * If pre_discover() returns 0, the NCI stack will wait for a HAL_NFC_PRE_DISCOVER_CPLT_EVT
	 * before continuing.
	 *
	 * If pre_discover() returns any other value, the NCI stack will start
	 * RF discovery immediately.
	 */
	int (*pre_discover)(const struct nfc_nci_device *p_dev);

	/*
	 * (*close)() Closed the NFC controller. Should free all resources.
	 */
	int (*close)(const struct nfc_nci_device *p_dev);

	/*
	 * (*control_granted)() Grant HAL the exclusive control to send NCI commands.
	 * Called in response to HAL_REQUEST_CONTROL_EVT.
	 * Must only be called when there are no NCI commands pending.
	 * HAL_RELEASE_CONTROL_EVT will notify when HAL no longer needs exclusive control.
	 */
	int (*control_granted)(const struct nfc_nci_device *p_dev);

	/*
	 * (*power_cycle)() Restart controller by power cyle;
	 * HAL_OPEN_CPLT_EVT will notify when operation is complete.
	 */
	int (*power_cycle)(const struct nfc_nci_device *p_dev);
} nfc_nci_device_t;





/********************* Definitions and structures *****************************/
#define MAX_RETRY_COUNT       5
#define NCI_MAX_DATA_LEN      300
#define NCI_POLL_DURATION     500
#define HAL_NFC_ENABLE_I2C_FRAGMENTATION_EVT    0x07
#define NFCC_DECIDES          00
#define POWER_ALWAYS_ON       01
#define LINK_ALWAYS_ON        02
#if(NFC_NXP_CHIP_TYPE == PN553)
#define EXPECTED_FW_VERSION 0x110114//0x11010C
#else
#define EXPECTED_FW_VERSION 0x011E
#endif
#undef P2P_PRIO_LOGIC_HAL_IMP
typedef void (phNxpNciHal_control_granted_callback_t)();

/* NCI Data */
typedef struct nci_data
{
    uint16_t len;
    uint8_t p_data[NCI_MAX_DATA_LEN];
} nci_data_t;

typedef enum
{
   HAL_STATUS_OPEN = 0,
   HAL_STATUS_CLOSE
} phNxpNci_HalStatus;

/* Macros to enable and disable extensions */
#define HAL_ENABLE_EXT()    (nxpncihal_ctrl.hal_ext_enabled = 1)
#define HAL_DISABLE_EXT()   (nxpncihal_ctrl.hal_ext_enabled = 0)

/* NCI Control structure */
typedef struct phNxpNciHal_Control
{
    phNxpNci_HalStatus   halStatus;      /* Indicate if hal is open or closed */
    void *client_task;
    uint8_t   thread_running; /* Thread running if set to 1, else set to 0 */
    phLibNfc_sConfig_t   gDrvCfg; /* Driver config data */

    /* Rx data */
    uint8_t  *p_rx_data;
    uint16_t rx_data_len;

    /* libnfc-nci callbacks */
    nfc_stack_callback_t *p_nfc_stack_cback;
    nfc_stack_data_callback_t *p_nfc_stack_data_cback;

    /* control granted callback */
    phNxpNciHal_control_granted_callback_t *p_control_granted_cback;

    /* HAL open status */
    bool_t hal_open_status;

    /* HAL extensions */
    uint8_t hal_ext_enabled;

    /* Waiting semaphore */
    phNxpNciHal_Sem_t ext_cb_data;

    uint16_t cmd_len;
    uint8_t p_cmd_data[NCI_MAX_DATA_LEN];
    uint16_t rsp_len;
    uint8_t p_rsp_data[NCI_MAX_DATA_LEN];

    /* retry count used to force download */
    uint16_t retry_cnt;
    uint8_t read_retry_cnt;
    uint8_t hal_boot_mode;
    void *halClientSemaphore;
} phNxpNciHal_Control_t;

typedef struct
{
    uint8_t fw_update_reqd;
    uint8_t rf_update_reqd;
} phNxpNciHal_FwRfupdateInfo_t;

typedef struct phNxpNciClock{
    bool_t  isClockSet;
    uint8_t  p_rx_data[20];
    bool_t  issetConfig;
}phNxpNciClock_t;

typedef struct phNxpNciRfSetting{
    bool_t  isGetRfSetting;
    uint8_t  p_rx_data[20];
}phNxpNciRfSetting_t;

/*set config management*/

#define TOTAL_DURATION         0x00
#define ATR_REQ_GEN_BYTES_POLL 0x29
#define ATR_REQ_GEN_BYTES_LIS  0x61
#define LEN_WT                 0x60

/*Whenever a new get cfg need to be sent,
 * array must be updated with defined config type*/
static const uint8_t get_cfg_arr[]={
        TOTAL_DURATION,
        ATR_REQ_GEN_BYTES_POLL,
        ATR_REQ_GEN_BYTES_LIS,
        LEN_WT
};

typedef enum {
    EEPROM_RF_CFG,
    EEPROM_JCOP_STATE,
    EEPROM_FW_DWNLD,
    EEPROM_ESE_SESSION_ID
}phNxpNci_EEPROM_request_type_t;

typedef struct phNxpNci_EEPROM_info {
    uint8_t  request_mode;
    phNxpNci_EEPROM_request_type_t  request_type;
    uint8_t  update_mode;
    uint8_t* buffer;
    uint8_t  bufflen;
}phNxpNci_EEPROM_info_t;

typedef struct phNxpNci_getConfig_info {
    bool_t    isGetcfg;
    uint8_t  total_duration[4];
    uint8_t  total_duration_len;
    uint8_t  atr_req_gen_bytes[48];
    uint8_t  atr_req_gen_bytes_len;
    uint8_t  atr_res_gen_bytes[48];
    uint8_t  atr_res_gen_bytes_len;
    uint8_t  pmid_wt[3];
    uint8_t  pmid_wt_len;
}phNxpNci_getConfig_info_t;

typedef enum { /* Wearable fix: renamed NFC_ to HAL_ as SeApi finds redefinition */
    HAL_NORMAL_BOOT_MODE,
    HAL_FAST_BOOT_MODE,
    HAL_OSU_BOOT_MODE
}phNxpNciBootMode;

typedef enum {
    NFC_FORUM_PROFILE,
    EMV_CO_PROFILE,
    INVALID_PROFILe
}phNxpNciProfile_t;
/* NXP Poll Profile control structure */
typedef struct phNxpNciProfile_Control
{
    phNxpNciProfile_t profile_type;
    uint8_t                      bClkSrcVal;     /* Holds the System clock source read from config file */
    uint8_t                      bClkFreqVal;    /* Holds the System clock frequency read from config file */
    uint8_t                      bTimeout;       /* Holds the Timeout Value */

} phNxpNciProfile_Control_t;

typedef struct phNxpNciTransitSettings{
    UINT8 ese_listen_tech_mask;  /* To configure required listen mode technologies dynamically*/
    UINT8 cfg_blk_chk_enable;     /* To disable/enable block number check of MIFARE card in firmware*/
    UINT8 cityType;               /* city type e.g 1,2,3 for City1..etc*/
    UINT8 rfSettingModified;      /* To be set to 1 if customized settings need to be applied*/
} phNxpNciTransitSettings_t;


/* Internal messages to handle callbacks */
#define NCI_HAL_OPEN_CPLT_MSG             0x411
#define NCI_HAL_CLOSE_CPLT_MSG            0x412
#define NCI_HAL_POST_INIT_CPLT_MSG        0x413
#define NCI_HAL_PRE_DISCOVER_CPLT_MSG     0x414
#define NCI_HAL_ERROR_MSG                 0x415
#define NCI_HAL_RX_MSG                    0xF01

#define NCIHAL_CMD_CODE_LEN_BYTE_OFFSET         (2U)
#define NCIHAL_CMD_CODE_BYTE_LEN                (3U)

#define NCI_HAL_OSU_STARTED                 0
#define NCI_HAL_OSU_COMPLETE                3

/******************** NCI HAL exposed variables *******************************/
extern uint32_t wFwVerRsp;

/******************** NCI HAL exposed functions *******************************/

void phNxpNciHal_request_control (void);
void phNxpNciHal_release_control (void);
NFCSTATUS phNxpNciHal_send_get_cfgs();
int phNxpNciHal_write_unlocked (uint16_t data_len, const uint8_t *p_data);
NFCSTATUS phNxpNciHal_CheckValidFwVersionWrapper(void);
void phNxpNciHal_get_clk_freq_wrapper(void);
NFCSTATUS request_EEPROM(phNxpNci_EEPROM_info_t *mEEPROM_info);
NFCSTATUS phNxpNciHal_send_nfcee_pwr_cntl_cmd(uint8_t type);
int phNxpNciHal_ioctl(long arg, void *p_data);
void phNxpNciHal_is_factory_reset_required(bool_t);
bool_t phNxpNciHal_is_firmware_uptodate(void);
NFCSTATUS phNxpNciHal_reset_nfcc_fw(void);
NFCSTATUS phNxpNciHal_set_china_region_configs(void* pSettings);
NFCSTATUS phNxpNciHal_set_transit_settings(void* pSettings);
NFCSTATUS phNxpNciHal_set_city_specific_rf_configs(UINT8 cityType);
NFCSTATUS phNxpNciHal_get_jcop_state(uint8_t *pState);
NFCSTATUS phNxpNciHal_set_jcop_state(uint8_t state);

#endif /* _PHNXPNCIHAL_H_ */
