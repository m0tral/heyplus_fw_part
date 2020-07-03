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

/*
 * debug_settings.h
 *
 *  Created on: 7/1/2016
 *      Author: Gerardo Tola
 */

#ifndef DEBUG_SETTINGS_H_
#define DEBUG_SETTINGS_H_

#define DEBUG_MEMORY 1
#define DEBUG_STANDBY_ENABLED 1
#define MALLOC_TRACE FALSE
#define MAX_CONCURRENT_ALLOCS 400
#define NFCINIT_MASK		0x01
#define BLEINIT_MASK		0x02
#define BLE_CONNECTED_MASK	0x04
#define FWDNLD_MASK         0x08
#define HWSELF_TEST_MASK    0x10
#define JCOPDNLD_MASK       0x20

#if defined(MALLOC_TRACE) && (MALLOC_TRACE == TRUE)
extern unsigned int cumalloc;
extern unsigned int maxalloc;
#endif

#endif /* DEBUG_SETTINGS_H_ */
