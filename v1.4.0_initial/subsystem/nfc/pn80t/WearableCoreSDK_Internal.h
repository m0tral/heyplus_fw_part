/*
 * WearableCoreSDK_Internal.h
 *
 *  Created on: Dec 5, 2016
 *      Author: nxa24212
 */

#ifndef WEARABLECORESDK_INTERNAL_H_
#define WEARABLECORESDK_INTERNAL_H_
#include "data_types.h"

/*
 * NXP chip related defines
 */
#define PN547C2   1
#define PN548C2   2
#define PN553     3
#define NFC_NXP_CHIP_TYPE PN553
/*
 * LS ROOT Key related defines
 */
#define NXP 1
#define TEST 2
#define LS_ROOT_KEY_TYPE NXP


#define JCOP_UPDATE 1
#define NORMAL_MODE 2
#define WEARABLE_MEMORY_ALLOCATION_TYPE NORMAL_MODE
/*
 * Firmware Version related defines
 */
#define PROD_11_01_0B 1
#define PROD_11_01_0C 2
#define PROD_11_01_14 3
#define FW_VERSION_TO_DOWNLOAD PROD_11_01_14 //PROD_11_01_0C //Kanjie PROD_11_01_14


#if(NFC_NXP_CHIP_TYPE == PN553)
#define NXP_ESE_RESET_METHOD                  TRUE
#define NXP_ESE_POWER_MODE                    TRUE
#define NXP_BLOCK_PROPRIETARY_APDU_GATE       TRUE
#define NXP_LEGACY_APDU_GATE                  FALSE
#define NXP_WIRED_MODE_STANDBY                TRUE
#define NXP_NFCC_MW_RCVRY_BLK_FW_DNLD         TRUE
#define NXP_NFCC_MIFARE_TIANJIN               FALSE
#else
#define NXP_ESE_RESET_METHOD                  FALSE
#define NXP_ESE_POWER_MODE                    FALSE
#define NXP_BLOCK_PROPRIETARY_APDU_GATE       TRUE
#define NXP_LEGACY_APDU_GATE                  TRUE
#define NXP_NFCC_MW_RCVRY_BLK_FW_DNLD         FALSE
#define NXP_NFCC_MIFARE_TIANJIN               TRUE
#endif

#endif /* WEARABLECORESDK_INTERNAL_H_ */
