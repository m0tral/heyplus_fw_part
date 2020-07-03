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


#ifndef WEARABLECORESDK_BUILDCONFIG_H
#define WEARABLECORESDK_BUILDCONFIG_H
#include "WearableCoreSDK_Internal.h"
#include "ry_utils.h"

#define DEBUG


/*
 * Enable GDI GUP UART COMMUNICATION
 * else they would be re-directed to UART.
 */
#define GDI_GUP_ENABLE    TRUE

/*
 * Enable GDI HAL USAGE
 * else they would be re-directed to tml and hw_i2c_66t.
 */

//#define GDI_HAL_ENABLE    TRUE

#define RY_HAL_ENABLE     TRUE

/*
 * Enables NFC UNIT TEST if defined
 * If undefined, the test application will operate in a “normal” configuration.
 * It will create a task that allows for communication between the wearable device
 * and the device host. BLE is active and is required for this configuration.
 * If defined, the test application will not communicate with the host device,
 * BLE will not be active. The test application will provide a set of testing options
 */
//#define NFC_UNIT_TEST

/*
 * Enable LOGS to LPCXpresso console
 * else they would be re-directed to UART.
 */
//#define LOGS_TO_CONSOLE

/*
 * If defined and set to TRUE, this will cause the test application to use
 * pre-defined data (found in testintput.c) for the tests that require scripts,
 * such as FirmwareDownload and LoaderService tests.
 */
#define TEST_STANDALONE FALSE

/*
 * If defined and set to TRUE, then API testing can be run over BLE interface.
 * API tests can be triggered from Companion side using BER-TLV format. It will allow
 * the API test application to use pre-defined data (found in testintput.c) for the
 * tests that require scripts, such as FirmwareDownload and LoaderService tests. To perform API
 * testing for FirmwareDownload and LoaderService TEST_STANDALONE should also be TRUE.
 */
#define API_TEST_ENABLE FALSE

/*
 * When defined and set to TRUE, the stack will operate under the configuration
 * that there is some form of non-volatile memory available for storing state
 * information. If undefined or FALSE, this state information will be implemented in
 * RAM and will be lost on reset or power cycle.
 * This is only used in the test application in the implementation of the various
 * callbacks. In a real integration, the integrator should implement the data storage
 * using whatever services are available and this flag will not be required.
 *
 */
#define FILESYSTEM_AVAILABLE FALSE

/*
 * Loader service , MOP, JCOP Update is built if SESUPPORT_INCLUDE is set to true
 */
#define SESUPPORT_INCLUDE FALSE
#if(SESUPPORT_INCLUDE == TRUE)
	/*
	 * Loader service is built if SESUPPORT_INCLUDE is set to true
	 */
	#define LS_ENABLE TRUE

	/*
	 * Code to update JCOP OS is built if the following macro is set to TRUE
	 */
	#define JCOP_ENABLE TRUE
	/*
	 * MIFARE Open platform enable or disable on the wearable device
	 */
	#define MIFARE_OP_ENABLE TRUE
#endif


/*
 * JCDN wearable buffered mode.
 * Set to TRUE always
 */
#define JCDN_WEARABLE_BUFFERED TRUE

/*
 * Code to Download Firmware is built if the following macro is set to TRUE
 */
#define FIRMWARE_DOWNLOAD_ENABLE TRUE

/*
 * Hardware self test API code enable / disable in build
 */
//#define NXP_HW_SELF_TEST

/*
 * Session management enable / disable
 * Set to TRUE always
 */
#define ENABLE_SESSION_MGMT TRUE

/*
 * Always defined.
 */
#define NXP_EXTNS TRUE

/*
 * Loader Service version 2
 * No need to modify.
 */
#define NXP_LDR_SVC_VER_2 TRUE

/*
 * ALA wearable buffered mode
 * No need to modify.
 */
#define ALA_WEARABLE_BUFFERED TRUE

/*
 * Always defined.
 */
#define BUILDCFG

/*
 * Gemalto secure element support
 * Defined always
 */
#define GEMALTO_SE_SUPPORT

/*
 * Enable NXP UICC.
 * No need to modify. [TBD: to check its applicability]
 */
#define NXP_UICC_ENABLE

/*
 * Define Reference device Pn66t - Define this always for connected device.
 */
//#define REFDEVICE_66T

/*
 * Enables dynamic switch of system clock frequency.
 * Low power vs performance
 *
 */
#define DYNAMIC_FREQUENCY_SWITCH FALSE

/*
 * Define to calculate max. number of AID routing entries supported based on core init response from NFCC.
 */
#define NFC_NXP_AID_MAX_SIZE_DYN TRUE


/*
 * Enables LS LTSM Clients at wearable. But script/VC data will be sent by companion device over BLE
 */
#define WEARABLE_LS_LTSM_CLIENT FALSE //TRUE
#if (WEARABLE_LS_LTSM_CLIENT == TRUE && WEARABLE_MEMORY_ALLOCATION_TYPE == JCOP_UPDATE)
#error "JCOP Update is not supported in this mode"
#endif
#if defined(NFC_UNIT_TEST) && (WEARABLE_LS_LTSM_CLIENT==TRUE)
#error "WEARABLE_LS_LTSM_CLIENT should not be TRUE when NFC_UNIT_TEST is defined"
#endif

/*
 * Define to use core stack together with OTA or not
 */
#define ENABLE_OTA	FALSE

/*
 * Buffer Size for the LS Script Chunk at the wearable-side
 */
#define LS_CMD_CHUNK_SIZE 2048

/*
 * Buffer Size for the LS Script execution response at the wearable-side
 */
#define LS_RSP_CHUNK_SIZE 2048

/*
 * Define hardware platform
 */
#define CONNECTED_DEMO
//#define E_V1_BOARD
//#define L_V1_BOARD

/*
 * Define Download Pin and Clock SOurce for PN80T Connected Device
 */
#define PN80T_CD_ENABLE

#define WEARABLE_CORE_SDK

/*
 * Enable/Disable Additional logs
 *
 * If NXP_PACKET_TRACES is defined and set to TRUE Logs are enabled in NXP Stack.
 *
 * If ADDITIONAL_DEBUG_TRACES is defined and set to TRUE logs are enabled in Default Stack
 * These are useful to be used when debug logs are requested. Otherwise these should be kept
 * off so that excessive logging does not interfere with the execution speed.
 */
#ifdef NFC_UNIT_TEST /* Standalone Mode */
#define NXP_PACKET_TRACES TRUE
#define ADDITIONAL_DEBUG_TRACES TRUE
#else /* Normal Mode */
#define NXP_PACKET_TRACES TRUE //FALSE
#define ADDITIONAL_DEBUG_TRACES FALSE//TRUE//FALSE
#endif

/*
 * Stack frame allocation for
 * different tasks in different
 * modes
 */

#ifdef NFC_UNIT_TEST
/* Standalone mode */
#define SEAPI_TEST_TASK_SIZE 1024 * 3
#define TMLREAD_STACK_SIZE 1024
#define TMLWRITE_STACK_SIZE 1024
#define NFC_TASK_STACK_SIZE (1024 * 2)
#define CLIENT_TASK_STACK_SIZE 256

#else

#if (WEARABLE_LS_LTSM_CLIENT == TRUE || SESUPPORT_INCLUDE == TRUE)
 /* LS,LTSM client in wearable */
#define TMLREAD_STACK_SIZE 1024
#define TMLWRITE_STACK_SIZE 1024
#define NFC_TASK_STACK_SIZE (1024 * 2)
#define CLIENT_TASK_STACK_SIZE 256
#define BLE_NFC_BRIDGE_TASK_SIZE (1024 * 2)
#define BLE_TASK_SIZE 256
#else
 /* Normal mode with BLE */
#define TMLREAD_STACK_SIZE (400)
#define TMLWRITE_STACK_SIZE (400)
#define NFC_TASK_STACK_SIZE (1024)
#define CLIENT_TASK_STACK_SIZE (256)
#define BLE_NFC_BRIDGE_TASK_SIZE (750)
#define BLE_TASK_SIZE 256
#endif

#endif

#endif /* WEARABLECORESDK_BUILDCONFIG_H */
