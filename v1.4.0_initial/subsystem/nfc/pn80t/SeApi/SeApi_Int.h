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

#ifndef SEAPI_INT_H
#define SEAPI_INT_H

#ifdef __cplusplus
extern "C"{
#endif

#include "SeApi.h"

#define OSU_STARTED                 0
#define OSU_COMPLETE                3
#define LS_VER_TLV_LEN              5

/**
 * \brief Prepares the device for NFCC Firmware download
 *
 *          If the stack is running, this will first shout down stack,
 *          restart the stack, reset and initialize the NFCC.
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_PrepFirmwareDownload(void);

/**
 * \brief Perform Post NFCC Firmware download configuration
 *
 *          Performs reset and init of the NFCC and then writes the necessary
 *          configs required by the new firmware. Post this the stack is
 *          shutdown. The application must call SeApi_Init() after this.
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_PostFirmwareDownload(void);
/**
 * \brief Updates the NFCC with same firmware
 *
 *          This function is used to forcefully download NFCC with same firmware.
 *          This feature useful in case of recovering NFCC from an unknown state.
 *
 * \return  SEAPI_STATUS_OK on success
 * \return  SEAPI_STATUS_FAILED otherwise
 */
#ifdef COMPANION_DEVICE
tSEAPI_STATUS SeApi_ForcedFirmwareDownload(UINT16 dataLen, const UINT8 *pData);
#else
tSEAPI_STATUS SeApi_ForcedFirmwareDownload(void);
#endif
/**
 * \brief Generate an RF data stream using a Pseudo-random bit sequence
 *        The data will be encoded using the coding for the specified technology
 *        and at the specified bit rate
 *
 * \param type  [in]  One of the \ref PRBS_Modes - all others are invalid
 * \param hw_prbs_type  [in]  One of the \ref PRBS_Types - all others are invalid
 * \param tech  [in]  One of the \ref RF_Technologies - all others are invalid
 * \param baud  [in]  One of the \ref Bit_Rates - all others are invalid
 *                    baud rates 106 and 848 are not supported for Technology F
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 *  \return SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 *  \return SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Test_PRBS_Start(UINT8 type, UINT8 hw_prbs_type, UINT8 tech, UINT8 baud);

/**
 * \brief Stop the PRBS test
 *
 *  \return SEAPI_STATUS_OK on success
 *          SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 *          SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_Test_PRBS_Stop(void);

/**
 * \brief Cleans up resources of NFC Stack
 *
 * \param None
 *
 *  \return None
 */

void SeApi_Int_CleanUpStack(void);
/**
 * \brief Performs a reset to eSE during JCOP OS update depending on
 *        Power schemes configured
 *
 *  \return SEAPI_STATUS_OK on success
 *          SEAPI_STATUS_NOT_INITIALIZED if SE is not initialized
 *          SEAPI_STATUS_FAILED otherwise
 */
tSEAPI_STATUS SeApi_JcopDnd_WiredResetEse(void);

/**
 * \brief Get JCOP update state
 *
 * \param pState - [out] pointer to JCOP state
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_FAILED - otherwise
 */
tSEAPI_STATUS SeApi_GetJcopState(UINT8 *pState);

/**
 * \brief Set the JCOP update state
 *
 * \param pStackCapabilities - [in] JCOP state to be set
 *
 * \return SEAPI_STATUS_OK - if successful
 * \return SEAPI_STATUS_FAILED - otherwise
 */
tSEAPI_STATUS SeApi_SetJcopState(UINT8 state);

#ifdef __cplusplus
}
#endif

#endif // SEAPI_INT_H
