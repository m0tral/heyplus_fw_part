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

#ifndef PHOSALNFC_LOG_H
#define PHOSALNFC_LOG_H
#include <WearableCoreSDK_BuildConfig.h>
#include "ry_utils.h"

#ifndef NFC_UNIT_TEST
#define phOsalNfc_LogBle LOG_DEBUG //phOsalNfc_Log
#define phOsalNfc_LogCrit(...)
#else
#define phOsalNfc_LogBle(...)
#define phOsalNfc_LogCrit LOG_DEBUG//phOsalNfc_Log
#endif

#define phOsalNfc_ScanU32() phOsalNfc_ScanNum(8)

/**
 * Initializes the OSAL Log Module.
 *
 * This function allocates the resources required by the Log Module.
 */
void phOsalNfc_Log_Init(void);

/**
 * De-Initializes the OSAL Log Module.
 *
 * This function deallocates the resources used by the Log Module.
 */
void phOsalNfc_Log_DeInit(void);

#if defined(DEBUG) || defined(NFC_UNIT_TEST)

/**
 * phOsalNfc_Log: Logs a formatted string.
 *
 * The formatted string is sent to the debugger console/UART Com Port
 *
 * \param[in] fmt      Format String as per the standard printf syntax
 * \param[in]          Variable Number of arguments if any needed by the format string
 *
 */
#ifdef LOGS_TO_CONSOLE
#define phOsalNfc_Log(...) printf(__VA_ARGS__)
#else /* LOGS_TO_CONSOLE */
//void phOsalNfc_Log(const char *fmt, ...);
#define phOsalNfc_Log      LOG_DEBUG
#endif /* LOGS_TO_CONSOLE */

/**
 * phOsalNfc_ScanNum: Scans a 32 bit value from the standard input.
 *
 * A 32-bit value is scanned from the debugger console/UART Com Port
 *
 * \param[in] nummaxdigits    Maximum number of digits to be scanned. The function will wait to receive these
 *                            many digits or a '\n'or '\r'character whichever occurs first.
 *
 * \retval                    A 32-bit number that was scanned
 */
unsigned int phOsalNfc_ScanNum (unsigned char nummaxdigits);

#else /* DEBUG */

#define phOsalNfc_Log(...)
#define phOsalNfc_ScanNum(nummaxdigits)

#endif /* DEBUG */

#endif  /* PHOSALNFC_LOG_H */
