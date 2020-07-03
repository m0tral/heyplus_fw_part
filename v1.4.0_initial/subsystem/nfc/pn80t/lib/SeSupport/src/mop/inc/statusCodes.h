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

/*
 * statusCodes.h
 *
 *  Created on: May 18, 2015
 *      Author: Robert Palmer
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STATUSCODES_H
#define STATUSCODES_H


/**********************************************
 * Status Values
 */
#define eSW_6310_AVAILABLE                 ((UINT16)0x6310)
#define eSW_6330_AVAILABLE                 ((UINT16)0x6330)

/**********************************************
 *Activation : Reason Codes TAG48 of TAGA2
 */
#define eREASON_CODE_8002                  ((UINT16)0x8002)
#define eREASON_CODE_8003                  ((UINT16)0x8003)
#define eREASON_CODE_8004                  ((UINT16)0x8004)
#define eREASON_CODE_8005                  ((UINT16)0x8005)
#define eREASON_CODE_8009                  ((UINT16)0x8009)
#define eREASON_CODE_800B                  ((UINT16)0x800B)
#define eREASON_CODE_800C                  ((UINT16)0x800C)

#define eTAG_VC_STATE                      ((UINT16)0x9F70)

/**********************************************
 * Generic Error codes (from LTSM client)
 */
#define eCONDITION_OF_USE_NOT_SATISFIED        ((UINT16)0X6980)
#define eSW_INVALID_VC_ENTRY                   ((UINT16)0x69E0)
#define eSW_OPEN_LOGICAL_CHANNEL_FAILED        ((UINT16)0x69E1)
#define eSW_SELECT_LTSM_FAILED                 ((UINT16)0x69E2)
#define eSW_REGISTRY_FULL                      ((UINT16)0x69E3)
#define eSW_IMPROPER_REGISTRY                  ((UINT16)0x69E4)
#define eSW_NO_SET_SP_SD                       ((UINT16)0x69E5)
#define eSW_ALREADY_ACTIVATED                  ((UINT16)0x69E6)
#define eSW_ALREADY_DEACTIVATED                ((UINT16)0x69E7)
#define eSW_CRS_SELECT_FAILED                  ((UINT16)0x69E8)
#define eSW_SE_TRANSCEIVE_FAILED               ((UINT16)0x69E9)
#define eSW_REGISTRY_IS_EMPTY                  ((UINT16)0x69EA)
#define eSW_NO_DATA_RECEIVED                   ((UINT16)0x69EB)
#define eSW_EXCEPTION                          ((UINT16)0x69EC)
#define eSW_CONDITION_OF_USE_NOT_SATISFIED     ((UINT16)0x6985)

#define eSW_OTHER_ACTIVEVC_EXIST               ((UINT16)0x6330)

#define eSW_CONDITIONAL_OF_USED_NOT_SATISFIED  ((UINT16)0x6A80)
#define eSW_INCORRECT_PARAM                    ((UINT16)0x6A86)
#define eSW_REFERENCE_DATA_NOT_FOUND           ((UINT16)0x6A88)

#define eSW_VC_IN_CONTACTLESS_SESSION          ((UINT16)0x6230)
#define eSW_UNEXPECTED_BEHAVIOR                ((UINT16)0x6F00)

#define	eSW_NO_ERROR                           ((UINT16)0x9000)

#define eSW_6310_COMMAND_AVAILABLE             ((UINT16)0x6310)

#define eERROR_NO_PACKAGE                      ((UINT16)0x0599)
#define eERROR_NO_MEMORY                       ((UINT16)0x0699)
#define eERROR_BAD_HASH	                       ((UINT16)0x0799)
#define eERROR_GENERAL_FAILURE                 ((UINT16)0x0899)
#define eERROR_INVALID_PARAM                   ((UINT16)0x0999)


/**********************************************
 * Generic TAGS used in the code.
 */
#define kTAG_VC_ENTRY                     (UINT16)0x40
#define kTAG_SM_AID                       (UINT16)0x42
#define kTAG_SP_SD_AID                    (UINT16)0x43
#define kTAG_SP_AID                       (UINT16)0x4F
#define kTAG_MF_DF_AID                    (UINT16)0xF8
#define kTAG_MF_AID                       (UINT16)0xE2

#define kTAG_A0_AVAILABLE                 (UINT16)0xA0
#define kTAG_A1_AVAILABLE                 (UINT16)0xA1
#define kTAG_A2_AVAILABLE                 (UINT16)0xA2
#define kTAG_A6_AVAILABLE                 (UINT16)0xA6
#define kTAG_F1_AVAILABLE                 (UINT16)0xF1

#define kTAG_48_AVAILABLE                 (UINT16)0x48
#define kTAG_4F_AVAILABLE                 (UINT16)0x4F

/*check values*/
#define kTAG_PENDING_LOGICAL_CHANNEL      (UINT16)0x56
#define kTAG_PENDING_SE_CONNECTION        (UINT16)0x57
#define kTAG_NOT_PENDING                  (UINT16)0x58

#define kTAG_VC_STATE                     (UINT16)0x9F70

#define kVC_CREATED                       (UINT16)0x02

#endif /* STATUSCODES_H */

#ifdef __cplusplus
}
#endif
