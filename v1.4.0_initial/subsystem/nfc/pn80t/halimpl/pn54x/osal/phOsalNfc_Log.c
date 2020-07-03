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

/**
 * \file  phOsalNfc_Log.c
 * \brief Abstraction Implementation for Logging and Scanning
 */

/** \addtogroup grp_osal_nfc
    @{
 */
/*
************************* Header Files ****************************************
*/
#ifndef COMPANION_DEVICE
#include "phOsalNfc.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include <stdarg.h>
#include "board.h"

/*
****************************** Macro Definitions ******************************
*/
/** \ingroup grp_osal_nfc
    Value indicates the maximum size of log buffer */
#define DEBUG_LOG_BUF_SIZE 1024

/*
****************************** Variable Definitions ******************************
*/
void * nDebugUartMutex;
void * nAndroidLogMutex;

/*
*************************** Function Definitions ******************************
*/

void phOsalNfc_Log_Init(void)
{
    phOsalNfc_CreateMutex(&nDebugUartMutex);
    phOsalNfc_CreateMutex(&nAndroidLogMutex);
}

void phOsalNfc_Log_DeInit(void)
{
    phOsalNfc_DeleteMutex(&nDebugUartMutex);
    phOsalNfc_DeleteMutex(&nAndroidLogMutex);
}

#if defined(DEBUG) || defined(NFC_UNIT_TEST) /* In Debug build or 'Nfc Unit Test in Release build'*/
#ifndef LOGS_TO_CONSOLE
#if 0
void phOsalNfc_Log(const char *fmt, ...)
{
#ifdef DEBUG_SEMIHOSTING
	printf(fmt);
#else
    static char buffer [DEBUG_LOG_BUF_SIZE];
    phOsalNfc_LockMutex(nDebugUartMutex);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, DEBUG_LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    Board_UARTPutSTR(buffer);
    phOsalNfc_UnlockMutex(nDebugUartMutex);
#endif
}
#endif
#endif

unsigned int phOsalNfc_ScanNum (unsigned char nummaxdigits)
{
#if defined LOGS_TO_CONSOLE || defined DEBUG_SEMIHOSTING
    int num;
    scanf("%u", &num);
    return num;
#else
    unsigned char digit, i;
    unsigned int hexnum = 0;
    i = nummaxdigits;
    do {
        phOsalNfc_LockMutex(nDebugUartMutex);
        //digit = (unsigned char) Board_UARTGetChar(); //  Kanjie, to replace
        phOsalNfc_UnlockMutex(nDebugUartMutex);
        if(digit >= '0' && digit <= '9')
        {
            phOsalNfc_LockMutex(nDebugUartMutex);
            //Board_UARTPutChar(digit);/* Show to the user what was keyed in */
            phOsalNfc_UnlockMutex(nDebugUartMutex);
            hexnum = (hexnum * 10) + (digit - '0');
            i--;
        }
        else if(digit == '\r' || digit == '\n') /* A new line or carriage return can be used to input less than maxdigits */
        {
            break;
        }
        else
        {
            /* Dummy */
        }
    } while (i);
    return hexnum;
#endif
}
#endif
#endif
