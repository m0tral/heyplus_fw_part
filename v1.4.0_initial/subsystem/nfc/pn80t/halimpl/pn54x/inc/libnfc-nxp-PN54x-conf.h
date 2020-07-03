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

// This file is used by NFC NXP NCI HAL(external/libnfc-nci/halimpl/pn54x)
#ifndef LIBNFC_NXP_PN54X_CONF
#define LIBNFC_NXP_PN54X_CONF

#include <WearableCoreSDK_BuildConfig.h>

#ifndef uint8_t
typedef unsigned char  uint8_t;
#endif

#define TYPE_VAL	0
#define TYPE_DATA	1
#define TYPE_STR	2

typedef struct {
	unsigned char key;
	unsigned char type;
	const void 	*val;
}NxpParam_t;

#define CONFIG_VAL (void *)

enum {
CFG_NXPLOG_EXTNS_LOGLEVEL,
CFG_NXPLOG_NCIHAL_LOGLEVEL,
CFG_NXPLOG_NCIX_LOGLEVEL,
CFG_NXPLOG_NCIR_LOGLEVEL,
CFG_NXPLOG_FWDNLD_LOGLEVEL,
CFG_NXPLOG_TML_LOGLEVEL,

CFG_MIFARE_READER_ENABLE,
CFG_NXP_FW_NAME,

CFG_NXP_FW_PROTECION_OVERRIDE,

CFG_NXP_SYS_CLK_SRC_SEL,
CFG_NXP_SYS_CLK_FREQ_SEL,
CFG_NXP_SYS_CLOCK_TO_CFG,

CFG_NXP_ESE_POWER_DH_CONTROL,

#if((NFC_NXP_CHIP_TYPE == PN548C2) || (NFC_NXP_CHIP_TYPE == PN553))
CFG_NXP_EXT_TVDD_CFG,
CFG_NXP_EXT_TVDD_CFG_1,
CFG_NXP_EXT_TVDD_CFG_2,
CFG_NXP_EXT_TVDD_CFG_3,
#endif

CFG_NXP_ACT_PROP_EXTN,

CFG_NXP_RF_CONF_BLK_1,
CFG_NXP_RF_CONF_BLK_2,
CFG_NXP_RF_CONF_BLK_3,
CFG_NXP_RF_CONF_BLK_4,
CFG_NXP_RF_CONF_BLK_5,
CFG_NXP_RF_CONF_BLK_6,

CFG_NXP_CORE_CONF_EXTN,
CFG_NXP_CORE_CONF,
CFG_NXP_CORE_MFCKEY_SETTING,
CFG_NXP_CORE_STANDBY,
CFG_NXP_CORE_RF_FIELD,

CFG_NXP_NFC_PROFILE_EXTN,
CFG_NXP_NFC_CHIP,

CFG_NXP_SWP_FULL_PWR_ON,
CFG_NXP_SWP_RD_START_TIMEOUT,
CFG_NXP_SWP_RD_TAG_OP_TIMEOUT,

CFG_NXP_I2C_FRAGMENTATION_ENABLED,

CFG_NXP_DEFAULT_SE,
CFG_NXP_DEFAULT_NFCEE_TIMEOUT,
CFG_NXP_DEFAULT_NFCEE_DISC_TIMEOUT,

CFG_NXP_UICC_FORWARD_FUNCTIONALITY_ENABLE,

CFG_NXP_CE_ROUTE_STRICT_DISABLE,

CFG_DEFAULT_AID_ROUTE,
CFG_DEFAULT_DESFIRE_ROUTE,
CFG_DEFAULT_MIFARE_CLT_ROUTE,
CFG_DEFAULT_AID_PWR_STATE,
CFG_DEFAULT_DESFIRE_PWR_STATE,
CFG_DEFAULT_MIFARE_CLT_PWR_STATE,

#if((NFC_NXP_CHIP_TYPE == PN548C2) || (NFC_NXP_CHIP_TYPE == PN553))
CFG_NXP_NFC_MERGE_RF_PARAMS,
CFG_AID_MATCHING_PLATFORM,
#endif
CFG_NXP_SWP_SWITCH_TIMEOUT,
CFG_NXP_CHINA_TIANJIN_RF_ENABLED,
CFG_NXP_SET_CONFIG_ALWAYS,
CFG_NXP_CHECK_DEFAULT_PROTO_SE_ID,
};

//////////////////////////////////////////////////////////////////////////////#
/*# System clock source selection configuration as per pn553 user manual section 14.1*/
/* PLL clock src is 0 */
/* XTAL clock src is 1 */
#define CLK_SRC_XTAL       1
#define CLK_SRC_PLL        0

//////////////////////////////////////////////////////////////////////////////#
//# System clock frequency selection configuration
#define CLK_FREQ_13MHZ         1
#define CLK_FREQ_19_2MHZ       2
#define CLK_FREQ_24MHZ         3
#define CLK_FREQ_26MHZ         4
#define CLK_FREQ_38_4MHZ       5
#define CLK_FREQ_52MHZ         6

#endif
