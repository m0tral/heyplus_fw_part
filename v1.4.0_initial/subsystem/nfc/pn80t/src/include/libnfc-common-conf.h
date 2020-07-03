/*
 * Copyright (C) 2015 NXP Semiconductors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define CONFIG_VAL (void *)

//###############################################################################
//# Maximum time (ms) to wait for RESET NTF after setting REG_PU to high
//# The default is 1000.
//#NFCC_ENABLE_TIMEOUT=0

//###############################################################################
//# LPTD mode configuration
//#  byte[0] is the length of the remaining bytes in this value
//#     if set to 0, LPTD params will NOT be sent to NFCC (i.e. disabled).
//#  byte[1] is the param id it should be set to B9.
//#  byte[2] is the length of the LPTD parameters
//#  byte[3] indicates if LPTD is enabled
//#     if set to 0, LPTD will be disabled (parameters will still be sent).
//#  byte[4-n] are the LPTD parameters.
//#  By default, LPTD is enabled and default settings are used.
//#  See nfc_hal_dm_cfg.c for defaults
//uint8_t cfgData_Lptd_Cfg[] = { 0x24,
//		0x23, 0xB9, 0x21, 0x01, 0x02, 0xFF, 0xFF, 0x04,
//		0xA0, 0x0F, 0x40, 0x00, 0x80, 0x02, 0x02, 0x10,
//		0x00, 0x00, 0x00, 0x31, 0x0C, 0x30, 0x00, 0x00,
//		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//		0x00, 0x00, 0x00, 0x00
//};

//###############################################################################
//# Startup Configuration (100 bytes maximum)
//#
//# For the 0xCA parameter, byte[9] (marked by 'AA') is for UICC0, and byte[10] (marked by BB) is
//#    for UICC1.  The values are defined as:
//#   0 : UICCx only supports ISO_DEP in low power mode.
//#   2 : UICCx only supports Mifare in low power mode.
//#   3 : UICCx supports both ISO_DEP and Mifare in low power mode.
//#
//#                                                                          AA BB
//uint8_t cfgData_Nfa_Dm_Start_Up_Cfg[] = { 0x20,
//		0x1F, 0xCB, 0x01, 0x01, 0xA5, 0x01, 0x01, 0xCA,
//		0x14, 0x00, 0x00, 0x00, 0x00, 0x06, 0xE8, 0x03,
//		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//		0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x01
//};

//###############################################################################
//# AID for Empty Select command
//# If specified, this AID will be substituted when an Empty SELECT command is
//# detected.  The first byte is the length of the AID.  Maximum length is 16.
uint8_t cfgData_Aid_For_Empty_Select[] = { 0x09,
		0x08, 0xA0, 0x00, 0x00, 0x01, 0x51, 0x00, 0x00, 0x00
};

//////////////////////////////////////////////////////////////////////////////#
//# NFA proprietary configuration
static uint8_t cfgData_Nfa_Proprietary[] = {0x05, 0xFF, 0xFF, 0x06, 0x81, 0x80, 0x70, 0xFF, 0xFF};

const ConfigParam_t MainConfig[] =
{
//###############################################################################
//# Application options
		{CFG_APPL_TRACE_LEVEL, 		TYPE_VAL,	CONFIG_VAL 0xFF},
		{CFG_PROTOCOL_TRACE_LEVEL,	TYPE_VAL,	CONFIG_VAL 0xFFFFFFFF},

//###############################################################################
//# performance measurement
//# Change this setting to control how often USERIAL log the performance (throughput)
//# data on read/write/poll
//# defailt is to log performance dara for every 100 read or write
//#REPORT_PERFORMANCE_MEASURE=100

//###############################################################################
//# File used for NFA storage
//NFA_STORAGE="/data/nfc"

//###############################################################################
//# Snooze Mode Settings
//#
//#  By default snooze mode is enabled.  Set SNOOZE_MODE_CFG byte[0] to 0
//#  to disable.
//#
//#  If SNOOZE_MODE_CFG is not provided, the default settings are used:
//#  They are as follows:
//#       8             Sleep Mode (0=Disabled 1=UART 8=SPI/I2C)
//#       0             Idle Threshold Host
//#       0             Idle Threshold HC
//#       0             NFC Wake active mode (0=ActiveLow 1=ActiveHigh)
//#       1             Host Wake active mode (0=ActiveLow 1=ActiveHigh)
//#
//#SNOOZE_MODE_CFG={08:00:00:00:01}

//###############################################################################
//# Insert a delay in milliseconds after NFC_WAKE and before write to NFCC
//		{CFG_NFC_WAKE_DELAY,		TYPE_VAL,	CONFIG_VAL 20},

//###############################################################################
//# Various Delay settings (in ms) used in USERIAL
//#  POWER_ON_DELAY
//#    Delay after turning on chip, before writing to transport (default 300)
//#  PRE_POWER_OFF_DELAY
//#    Delay after deasserting NFC-Wake before turn off chip (default 0)
//#  POST_POWER_OFF_DELAY
//#    Delay after turning off chip, before USERIAL_close returns (default 0)
//#
//#POWER_ON_DELAY=300
//#PRE_POWER_OFF_DELAY=0
//#POST_POWER_OFF_DELAY=0

//###############################################################################
//# LPTD mode configuration
//#  byte[0] is the length of the remaining bytes in this value
//#     if set to 0, LPTD params will NOT be sent to NFCC (i.e. disabled).
//#  byte[1] is the param id it should be set to B9.
//#  byte[2] is the length of the LPTD parameters
//#  byte[3] indicates if LPTD is enabled
//#     if set to 0, LPTD will be disabled (parameters will still be sent).
//#  byte[4-n] are the LPTD parameters.
//#  By default, LPTD is enabled and default settings are used.
//#  See nfc_hal_dm_cfg.c for defaults
//		{CFG_LPTD_CFG,				TYPE_DATA, cfgData_Lptd_Cfg},

//###############################################################################
//# Startup Configuration (100 bytes maximum)
//#
//# For the 0xCA parameter, byte[9] (marked by 'AA') is for UICC0, and byte[10] (marked by BB) is
//#    for UICC1.  The values are defined as:
//#   0 : UICCx only supports ISO_DEP in low power mode.
//#   2 : UICCx only supports Mifare in low power mode.
//#   3 : UICCx supports both ISO_DEP and Mifare in low power mode.
//#
//#                                                                          AA BB
//		{CFG_NFA_DM_START_UP_CFG,	TYPE_DATA, cfgData_Nfa_Dm_Start_Up_Cfg},

//###############################################################################
//# Startup Vendor Specific Configuration (100 bytes maximum);
//#  byte[0] TLV total len = 0x5
//#  byte[1] NCI_MTS_CMD|NCI_GID_PROP = 0x2f
//#  byte[2] NCI_MSG_FRAME_LOG = 0x9
//#  byte[3] 2
//#  byte[4] 0=turn off RF frame logging; 1=turn on
//#  byte[5] 0=turn off SWP frame logging; 1=turn on
//#  NFA_DM_START_UP_VSC_CFG={05:2F:09:02:01:01}

//###############################################################################
//# Antenna Configuration - This data is used when setting 0xC8 config item
//# at startup (before discovery is started).  If not used, no value is sent.
//#
//# The settings for this value are documented here:
//# http://wcgbu.broadcom.com/wpan/PM/Project%20Document%20Library/bcm20791B0/
//#   Design/Doc/PHY%20register%20settings/BCM20791-B2-1027-02_PHY_Recommended_Reg_Settings.xlsx
//# This document is maintained by Paul Forshaw.
//#
//# The values marked as ?? should be tweaked per antenna or customer/app:
//# {20:C8:1E:06:??:00:??:??:??:00:??:24:00:1C:00:75:00:77:00:76:00:1C:00:03:00:0A:00:??:01:00:00:40:04}
//# array[0] = 0x20 is length of the payload from array[1] to the end
//# array[1] = 0xC8 is PREINIT_DSP_CFG
//#PREINIT_DSP_CFG={20:C8:1E:06:1F:00:0F:03:3C:00:04:24:00:1C:00:75:00:77:00:76:00:1C:00:03:00:0A:00:48:01:00:00:40:04}

//###############################################################################
//# Configure crystal frequency when internal LPO can't detect the frequency.
//#XTAL_FREQUENCY=0
//###############################################################################
//# Configure the default Destination Gate used by HCI (the default is 4, which
//# is the ETSI loopback gate.
		{CFG_NFA_HCI_DEFAULT_DEST_GATE,		TYPE_VAL,	CONFIG_VAL 0xF0},

//###############################################################################
//# Configure the single default SE to use.  The default is to use the first
//# SE that is detected by the stack.  This value might be used when the phone
//# supports multiple SE (e.g. 0xF3 and 0xF4) but you want to force it to use
//# one of them (e.g. 0xF4).
//#ACTIVE_SE=0xF3

//###############################################################################
//# Configure the default NfcA/IsoDep techology and protocol route. Can be
//# either a secure element (e.g. 0xF4) or the host (0x00)
//#DEFAULT_ISODEP_ROUTE=0x00

//###############################################################################
//# Configure the NFC Extras to open and use a static pipe.  If the value is
//# not set or set to 0, then the default is use a dynamic pipe based on a
//# destination gate (see NFA_HCI_DEFAULT_DEST_GATE).  Note there is a value
//# for each UICC (where F3="UICC0" and F4="UICC1")
//#NFA_HCI_STATIC_PIPE_ID_F3=0x70
//#NFA_HCI_STATIC_PIPE_ID_01=0x19
		{CFG_NFA_HCI_STATIC_PIPE_ID_C0,		TYPE_VAL,  CONFIG_VAL 0x19},
//###############################################################################
//# When disconnecting from Oberthur secure element, perform a warm-reset of
//# the secure element to deselect the applet.
//# The default hex value of the command is 0x3.  If this variable is undefined,
//# then this feature is not used.
//		{CFG_OBERTHUR_WARM_RESET_COMMAND,	TYPE_VAL,	CONFIG_VAL 0x03},

//###############################################################################
//# Force UICC to only listen to the following technology(s).
//# The bits are defined as tNFA_TECHNOLOGY_MASK in nfa_api.h.
//# Default is NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B | NFA_TECHNOLOGY_MASK_F
		{CFG_UICC_LISTEN_TECH_MASK,			TYPE_VAL,	CONFIG_VAL 0x07},

//###############################################################################
//# Force HOST listen feature enable or disable.
//# 0: Disable
//# 1: Enable
		{CFG_HOST_LISTEN_ENABLE,			TYPE_VAL,	CONFIG_VAL 0x01},

//###############################################################################
//# Allow UICC to be powered off if there is no traffic.
//# Timeout is in ms. If set to 0, then UICC will not be powered off.
//#UICC_IDLE_TIMEOUT=30000
		{CFG_UICC_IDLE_TIMEOUT,				TYPE_VAL,	CONFIG_VAL 0},

//###############################################################################
//# AID for Empty Select command
//# If specified, this AID will be substituted when an Empty SELECT command is
//# detected.  The first byte is the length of the AID.  Maximum length is 16.
		{CFG_AID_FOR_EMPTY_SELECT,			TYPE_DATA,	cfgData_Aid_For_Empty_Select},

//###############################################################################
//# Maximum Number of Credits to be allowed by the NFCC
//#   This value overrides what the NFCC specifices allowing the host to have
//#   the control to work-around transport limitations.  If this value does
//#   not exist or is set to 0, the NFCC will provide the number of credits.
		{CFG_MAX_RF_DATA_CREDITS,			TYPE_VAL,	CONFIG_VAL 1},

//###############################################################################
//# This setting allows you to disable registering the T4t Virtual SE that causes
//# the NFCC to send PPSE requests to the DH.
//# The default setting is enabled (i.e. T4t Virtual SE is registered).
//#REGISTER_VIRTUAL_SE=1

//###############################################################################
//# When screen is turned off, specify the desired power state of the controller.
//# 0: power-off-sleep state; DEFAULT
//# 1: full-power state
//# 2: screen-off card-emulation (CE4/CE3/CE1 modes are used)
		{CFG_SCREEN_OFF_POWER_STATE,			TYPE_VAL,	CONFIG_VAL 1},

//###############################################################################
//# Firmware patch file
//#  If the value is not set then patch download is disabled.
//FW_PATCH="/vendor/firmware/bcm2079x_firmware.ncd"

//###############################################################################
//# Firmware pre-patch file (sent before the above patch file)
//#  If the value is not set then pre-patch is not used.
//FW_PRE_PATCH="/vendor/firmware/bcm2079x_pre_firmware.ncd"

//###############################################################################
//# Firmware patch format
//#   1 = HCD
//#   2 = NCD (default)
//#NFA_CONFIG_FORMAT=2

//###############################################################################
//# SPD Debug mode
//#  If set to 1, any failure of downloading a patch will trigger a hard-stop
//#SPD_DEBUG=0

//###############################################################################
//# SPD Max Retry Count
//#  The number of attempts to download a patch before giving up (defualt is 3).
//#  Note, this resets after a power-cycle.
//#SPD_MAX_RETRY_COUNT=3

//###############################################################################
//# transport driver
//#
//# TRANSPORT_DRIVER=<driver>
//#
//#  where <driver> can be, for example:
//#    "/dev/ttyS"        (UART)
//#    "/dev/bcmi2cnfc"   (I2C)
//#    "hwtun"            (HW Tunnel)
//#    "/dev/bcmspinfc"   (SPI)
//#    "/dev/btusb0"      (BT USB)
//#TRANSPORT_DRIVER="/dev/bcm2079x-i2c"

//###############################################################################
//# power control driver
//# Specify a kernel driver that support ioctl commands to control NFC_EN and
//# NFC_WAKE gpio signals.
//#
//# POWER_CONTRL_DRIVER=<driver>
//#  where <driver> can be, for example:
//#    "/dev/nfcpower"
//#    "/dev/bcmi2cnfc"   (I2C)
//#    "/dev/bcmspinfc"   (SPI)
//#    i2c and spi driver may be used to control NFC_EN and NFC_WAKE signal
//#POWER_CONTROL_DRIVER="/dev/bcm2079x-i2c"

//###############################################################################
//# I2C transport driver options
//# Mako does not support 10-bit I2C addresses
//# Revert to 7-bit address

		{CFG_BCMI2CNFC_ADDRESS,			TYPE_VAL,	CONFIG_VAL 0x77},

//###############################################################################
//# I2C transport driver try to read multiple packets in read() if data is available
//# remove the comment below to enable this feature
//#READ_MULTIPLE_PACKETS=1

//###############################################################################
//# SPI transport driver options
//#SPI_NEGOTIATION={0A:F0:00:01:00:00:00:FF:FF:00:00}

//###############################################################################
//# UART transport driver options
//#
//# PORT=1,2,3,...
//# BAUD=115200, 19200, 9600, 4800,
//# DATABITS=8, 7, 6, 5
//# PARITY="even" | "odd" | "none"
//# STOPBITS="0" | "1" | "1.5" | "2"

//#UART_PORT=2
//#UART_BAUD=115200
//#UART_DATABITS=8
//#UART_PARITY="none"
//#UART_STOPBITS="1"

//###############################################################################
//# Insert a delay in microseconds per byte after a write to NFCC.
//# after writing a block of data to the NFCC, delay this an amopunt of time before
//# writing next block of data.  the delay is calculated as below
//#   NFC_WRITE_DELAY * (number of byte written) / 1000 milliseconds
//# e.g. after 259 bytes is written, delay (259 * 20 / 1000) 5 ms before next write
//		{CFG_NFC_WRITE_DELAY,			TYPE_VAL,	CONFIG_VAL 20},

//###############################################################################
//# Maximum Number of Credits to be allowed by the NFCC
//#   This value overrides what the NFCC specifices allowing the host to have
//#   the control to work-around transport limitations.  If this value does
//#   not exist or is set to 0, the NFCC will provide the number of credits.
		{CFG_MAX_RF_DATA_CREDITS,			TYPE_VAL,	CONFIG_VAL 1},

//###############################################################################
//# Default poll duration (in ms)
//#  The defualt is 500ms if not set (see nfc_target.h)
//#		{CFG_NFA_DM_DISC_DURATION_POLL,			TYPE_VAL,	CONFIG_VAL 333},

//###############################################################################
//# Antenna Configuration - This data is used when setting 0xC8 config item
//# at startup (before discovery is started).  If not used, no value is sent.
//#
//# The settings for this value are documented here:
//# http://wcgbu.broadcom.com/wpan/PM/Project%20Document%20Library/bcm20791B0/
//#   Design/Doc/PHY%20register%20settings/BCM20791-B2-1027-02_PHY_Recommended_Reg_Settings.xlsx
//# This document is maintained by Paul Forshaw.
//#
//# The values marked as ?? should be tweaked per antenna or customer/app:
//# {20:C8:1E:06:??:00:??:??:??:00:??:24:00:1C:00:75:00:77:00:76:00:1C:00:03:00:0A:00:??:01:00:00:40:04}
//# array[0] = 0x20 is length of the payload from array[1] to the end
//# array[1] = 0xC8 is PREINIT_DSP_CFG
//#PREINIT_DSP_CFG={20:C8:1E:06:1F:00:0F:03:3C:00:04:24:00:1C:00:75:00:77:00:76:00:1C:00:03:00:0A:00:48:01:00:00:40:04}


//###############################################################################
//# Choose the presence-check algorithm for type-4 tag.  If not defined, the default value is 1.
//# 0  NFA_RW_PRES_CHK_DEFAULT; Let stack selects an algorithm
//# 1  NFA_RW_PRES_CHK_I_BLOCK; ISO-DEP protocol's empty I-block
//# 2  NFA_RW_PRES_CHK_RESET; Deactivate to Sleep, then re-activate
//# 3  NFA_RW_PRES_CHK_RB_CH0; Type-4 tag protocol's ReadBinary command on channel 0
//# 4  NFA_RW_PRES_CHK_RB_CH3; Type-4 tag protocol's ReadBinary command on channel 3
//#		{CFG_PRESENCE_CHECK_ALGORITHM,			TYPE_VAL,	CONFIG_VAL 0},

//###############################################################################
//# Force tag polling for the following technology(s).
//# The bits are defined as tNFA_TECHNOLOGY_MASK in nfa_api.h.
//# Default is NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_B |
//#            NFA_TECHNOLOGY_MASK_F | NFA_TECHNOLOGY_MASK_ISO15693 |
//#            NFA_TECHNOLOGY_MASK_B_PRIME | NFA_TECHNOLOGY_MASK_KOVIO |
//#            NFA_TECHNOLOGY_MASK_A_ACTIVE | NFA_TECHNOLOGY_MASK_F_ACTIVE.
//#
//# Notable bits:
//# NFA_TECHNOLOGY_MASK_A             0x01    /* NFC Technology A             */
//# NFA_TECHNOLOGY_MASK_B             0x02    /* NFC Technology B             */
//# NFA_TECHNOLOGY_MASK_F             0x04    /* NFC Technology F             */
//# NFA_TECHNOLOGY_MASK_ISO15693	    0x08    /* Proprietary Technology       */
//# NFA_TECHNOLOGY_MASK_KOVIO	        0x20    /* Proprietary Technology       */
//# NFA_TECHNOLOGY_MASK_A_ACTIVE      0x40    /* NFC Technology A active mode */
//# NFA_TECHNOLOGY_MASK_F_ACTIVE      0x80    /* NFC Technology F active mode */
		{CFG_POLLING_TECH_MASK,			TYPE_VAL,	CONFIG_VAL 0xEF},

//###############################################################################
//# Force P2P to only listen for the following technology(s).
//# The bits are defined as tNFA_TECHNOLOGY_MASK in nfa_api.h.
//# Default is NFA_TECHNOLOGY_MASK_A | NFA_TECHNOLOGY_MASK_F |
//#            NFA_TECHNOLOGY_MASK_A_ACTIVE | NFA_TECHNOLOGY_MASK_F_ACTIVE
//#
//# Notable bits:
//# NFA_TECHNOLOGY_MASK_A	            0x01    /* NFC Technology A             */
//# NFA_TECHNOLOGY_MASK_F	            0x04    /* NFC Technology F             */
//# NFA_TECHNOLOGY_MASK_A_ACTIVE      0x40    /* NFC Technology A active mode */
//# NFA_TECHNOLOGY_MASK_F_ACTIVE      0x80    /* NFC Technology F active mode */
		{CFG_P2P_LISTEN_TECH_MASK,		TYPE_VAL,	CONFIG_VAL 0xC5},

		{CFG_PRESERVE_STORAGE,			TYPE_VAL,	CONFIG_VAL 0x01},

//###############################################################################
//# Override the stack default for NFA_EE_MAX_EE_SUPPORTED set in nfc_target.h.
//# The value is set to 3 by default as it assumes we will discover 0xF2,
//# 0xF3, and 0xF4. If a platform will exclude and SE, this value can be reduced
//# so that the stack will not wait any longer than necessary.
//
//# Maximum EE supported number
//# NXP PN547C2 0x02
//# NXP PN65T 0x03
		{CFG_NFA_MAX_EE_SUPPORTED,			TYPE_VAL,	CONFIG_VAL 0x03},

//##############################################################################
//# Deactivate notification wait time out in seconds used in ETSI Reader mode
		{CFG_NFA_DM_DISC_NTF_TIMEOUT,		TYPE_VAL,	CONFIG_VAL 100},

//###############################################################################
//# AID_MATCHING constants
//# AID_MATCHING_EXACT_ONLY 0x00
//# AID_MATCHING_EXACT_OR_PREFIX 0x01
//# AID_MATCHING_PREFIX_ONLY 0x02
		{CFG_AID_MATCHING_MODE,			TYPE_VAL,	CONFIG_VAL 0x01},

//###############################################################################
//# Preferred Secure Element for Technology based routing
//# eSE               0x01
//# UICC              0x02
		{CFG_DEFAULT_OFFHOST_ROUTE,			TYPE_VAL,	CONFIG_VAL 0x01},

//###############################################################################
//# Vendor Specific Proprietary Protocol & Discovery Configuration
//# Set to 0xFF if unsupported
//#  byte[0] NCI_PROTOCOL_18092_ACTIVE
//#  byte[1] NCI_PROTOCOL_B_PRIME
//#  byte[2] NCI_PROTOCOL_DUAL
//#  byte[3] NCI_PROTOCOL_15693
//#  byte[4] NCI_PROTOCOL_KOVIO
//#  byte[5] NCI_PROTOCOL_MIFARE
//#  byte[6] NCI_DISCOVERY_TYPE_POLL_KOVIO
//#  byte[7] NCI_DISCOVERY_TYPE_POLL_B_PRIME
//#  byte[8] NCI_DISCOVERY_TYPE_LISTEN_B_PRIME
        {CFG_NFA_PROPRIETARY_CFG,       TYPE_DATA, cfgData_Nfa_Proprietary},

};

