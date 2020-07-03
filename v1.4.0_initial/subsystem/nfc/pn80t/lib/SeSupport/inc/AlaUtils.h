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

#ifndef ALAUTILS_H_
#define ALAUTILS_H_

#include "p61_data_types.h"

/* Script Chunk Indicators */
#define LS_FIRST_CHUNK 0x00
#define LS_INTERMEDIATE_CHUNK 0x01
#define LS_CHUNK_NOT_REQUIRED 0x02
#define LS_LAST_CHUNK 0xFF

/* Command chunk descriptors */
/*
 * Command Chunk is of below format
 * 0 to N-6: (UINT8[]) : Script Data Chunk
 * N-6: (UINT8) : Chunk Marker
 * N-5: (UINT8) : Number of APDUs in the chunk buffer
 * N-4 to N-1: (UINT32) : Total Script Length (MSB is put first)
 *
 */
#define CMD_CHUNK_FOOTER_SIZE 6
#define CHUNK_MARKER_OFFSET 6
#define NUM_APDU_OFFSET 5
#define TOTAL_SCRIPT_LEN_OFFSET 4

/* Response chunk descriptors */
/*
 * Response Chunk is of below format
 * 0 to N-2: (UINT8[]) : Response Data Chunk
 * N-2 to N-1: (UINT16) : Command Chunk Offset (MSB is put first)
 *
 */
#define RSP_CHUNK_FOOTER_SIZE 2
#define CMD_CHUNK_OFFSET_OFFSET 2

/* First chunk must contain Certificate frame and Authentication frame */
#define LS_CMD_CHUNK_MIN_SIZE 1024

/* max length of R-APDU is 255 bytes
 * + 2 for footer */
#define LS_RSP_CHUNK_MIN_SIZE 257

#if WEARABLE_LS_LTSM_CLIENT == TRUE
#if (LS_CMD_CHUNK_SIZE < LS_CMD_CHUNK_MIN_SIZE)
#error "LS_CMD_CHUNK_SIZE is too small"
#endif

#if (LS_RSP_CHUNK_SIZE < LS_RSP_CHUNK_MIN_SIZE)
#error "LS_RSP_CHUNK_SIZE is too small"
#endif
#endif

/* minimum of the minimum lengths of LS Commands, Certificate and Authentication frames */
#define LS_CMD_MIN_SIZE 71

/**
 * \brief Skips Metadata and non-digits from the script
 *
 * \param pBuf - [in] pointer to a buffer containing the script
 * \param bufSize - [in] number of bytes in the script buffer
 * \param offset - [in] offset to the script buffer
 *
 *  \return an offset to the non-metadata content
 */
UINT32 Ala_SkipMetaDataAndNonDigits(const UINT8* pBuf, UINT32 bufSize, UINT32 offset);

/**
 * \brief Fill the chunk buffer with possible number of APDUs
 *
 * \param pScript - [in] pointer to a buffer containing the script
 * \param scriptLen - [in] number of bytes in the script buffer
 * \param pScriptOffset - [in] offset to the script buffer
 * \param pScriptLenConsumed - [out] number bytes read from script to fill the chunk
 * \param pChunkBuffer - [out] pointer to a buffer to receive the chunk
 * \param pAlignedChunkSize - [in] chunk buffer size, [out] filled chunk size
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 *  \return SEAPI_STATUS_FAILED otherwise
 */
tJBL_STATUS Ala_GetCmdChunk(const UINT8* pScript, INT32 scriptLen, UINT32 *pScriptOffset, UINT16 *pScriptLenConsumed, UINT8* pChunkBuffer, UINT32 *pAlignedChunkSize);

/**
 * \brief Store the R-APDUs in the chunk buffer to the response buffer
 *
 * \param pRespData - [out] pointer to a buffer to receive the response
 * \param respLen - [in] size of the response buffer
 * \param pRespOffset - [in/out] offset to the response buffer
 * \param pScriptOffset - [in/out] offset to the script buffer
 * \param pScriptLenConsumed - [in] number bytes that were sent in the command chunk
 * \param pRespChunkBuffer - [in] pointer to response chunk buffer
 * \param respChunkSize - [in] response chunk size
 *
 *  \return SEAPI_STATUS_OK on success
 *  \return SEAPI_STATUS_INVALID_PARAM if invalid parameters are passed.
 *  \return SEAPI_STATUS_FAILED otherwise
 */
tJBL_STATUS Ala_StoreRspChunk(UINT8* pRespData, UINT16 respLen, UINT32 *pRespOffset, UINT32 *pScriptOffset, UINT16 *pScriptLenConsumed, UINT8* pRespChunkBuffer, UINT16 respChunkSize);
/**
 * \brief Enabling/Disabling runtime chunking
 * \param [in] fl TRUE or FALSE
 */
void Ala_SetChunking(BOOLEAN fl);
/**
 * \brief Get runtime chunking status
 * \return status TRUE or FALSE
 */
BOOLEAN Ala_IsChunkingEnabled(void);
#endif /*ALAUTILS_H_*/
