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
#ifndef __CONFIG_H
#define __CONFIG_H


int GetStrValue(unsigned char key, char* p_value, unsigned long len);
int GetNumValue(unsigned char key, void* p_value, unsigned long len);

#define TYPE_VAL	0
#define TYPE_DATA	1

typedef struct {
	unsigned char key;
	unsigned char type;
	void 	*val;
}ConfigParam_t;

#define CFG_POLLING_TECH_MASK          0
#define CFG_REGISTER_VIRTUAL_SE        1
#define CFG_APPL_TRACE_LEVEL           2
#define CFG_USE_RAW_NCI_TRACE          3
#define CFG_LOGCAT_FILTER              4
#if(NXP_EXTNS == TRUE)
#define CFG_APPL_DTA_MODE              5
#endif
#define CFG_LPTD_CFG                   6
#define CFG_SCREEN_OFF_POWER_STATE     7
#define CFG_PREINIT_DSP_CFG            8
#define CFG_DTA_START_CFG              9
#if(NXP_EXTNS != TRUE)
#define CFG_TRANSPORT_DRIVER           10
#define CFG_POWER_CONTROL_DRIVER       11
#endif
#define CFG_PROTOCOL_TRACE_LEVEL       12
#define CFG_UART_PORT                  13
#define CFG_UART_BAUD                  14
#define CFG_UART_PARITY                15
#define CFG_UART_STOPBITS              16
#define CFG_UART_DATABITS              17
#define CFG_CLIENT_ADDRESS             18
#define CFG_NFA_DM_START_UP_CFG        19
#define CFG_NFA_DM_CFG                 20
#define CFG_NFA_DM_LP_CFG              21
#define CFG_LOW_SPEED_TRANSPORT        22
#define CFG_NFC_WAKE_DELAY             23
#define CFG_NFC_WRITE_DELAY            24
#define CFG_PERF_MEASURE_FREQ          25
#define CFG_READ_MULTI_PACKETS         26
#define CFG_POWER_ON_DELAY             27
#define CFG_PRE_POWER_OFF_DELAY        28
#define CFG_POST_POWER_OFF_DELAY       29
#define CFG_CE3_PRE_POWER_OFF_DELAY    30
#define CFG_NFA_STORAGE                31
#define CFG_NFA_DM_START_UP_VSC_CFG    32
#define CFG_NFA_DTA_START_UP_VSC_CFG   33
#define CFG_UICC_LISTEN_TECH_MASK      34
#define CFG_UICC_LISTEN_TECH_EX_MASK   61
#define CFG_HOST_LISTEN_ENABLE         35
#define CFG_SNOOZE_MODE_CFG            39
#define CFG_NFA_DM_DISC_DURATION_POLL  40
#define CFG_SPD_DEBUG                  41
#define CFG_SPD_MAXRETRYCOUNT          42
#define CFG_SPI_NEGOTIATION            43
#define CFG_AID_FOR_EMPTY_SELECT       44
#define CFG_PRESERVE_STORAGE           45
#define CFG_NFA_MAX_EE_SUPPORTED       46
#define CFG_NFCC_ENABLE_TIMEOUT        47
#define CFG_NFA_DM_PRE_DISCOVERY_CFG   48
#define CFG_POLL_FREQUENCY             49
#define CFG_NFA_HCI_DEFAULT_DEST_GATE  50
#define CFG_NFA_HCI_STATIC_PIPE_ID_C0  51
#define CFG_OBERTHUR_WARM_RESET_COMMAND  52
#define CFG_UICC_IDLE_TIMEOUT          53
#define CFG_MAX_RF_DATA_CREDITS        54
#define CFG_BCMI2CNFC_ADDRESS          55
#define CFG_P2P_LISTEN_TECH_MASK       56
#define CFG_NFA_DM_DISC_NTF_TIMEOUT    57
#define CFG_AID_MATCHING_MODE          58
#define CFG_DEFAULT_OFFHOST_ROUTE           60

#define CFG_XTAL_HARDWARE_ID           63
#define CFG_XTAL_FREQUENCY             64
#define CFG_XTAL_FREQ_INDEX            65
#define CFG_XTAL_PARAMS_CFG            66
#define CFG_EXCLUSIVE_SE_ACCESS        67
#define CFG_DBG_NO_UICC_IDLE_TIMEOUT_TOGGLING  68
#define CFG_PRESENCE_CHECK_ALGORITHM   69
#define CFG_ALLOW_NO_NVM               70
#define CFG_DEVICE_HOST_WHITE_LIST     71
#define CFG_POWER_OFF_MODE             72
#define CFG_GLOBAL_RESET               73
#define CFG_NCI_HAL_MODULE             74
#define CFG_NFA_POLL_BAIL_OUT_MODE     75
#define CFG_NFA_PROPRIETARY_CFG        76

#define                     LPTD_PARAM_LEN (40)

// default configuration
#define default_transport       "/dev/bcm2079x"
#define default_storage_location "/data/nfc"

struct tUART_CONFIG {
    int     m_iBaudrate;            // 115200
    int     m_iDatabits;            // 8
    int     m_iParity;              // 0 - none, 1 = odd, 2 = even
    int     m_iStopbits;
};

extern struct tUART_CONFIG  uartConfig;
#define MAX_CHIPID_LEN  (16)
void    readOptionalConfig(const char* option);

/* Snooze mode configuration structure */
typedef struct
{
    unsigned char   snooze_mode;            /* Snooze Mode */
    unsigned char   idle_threshold_dh;      /* Idle Threshold Host */
    unsigned char   idle_threshold_nfcc;    /* Idle Threshold NFCC   */
    unsigned char   nfc_wake_active_mode;   /* NFC_LP_ACTIVE_LOW or NFC_LP_ACTIVE_HIGH */
    unsigned char   dh_wake_active_mode;    /* NFC_LP_ACTIVE_LOW or NFC_LP_ACTIVE_HIGH */
} tSNOOZE_MODE_CONFIG;
#endif
