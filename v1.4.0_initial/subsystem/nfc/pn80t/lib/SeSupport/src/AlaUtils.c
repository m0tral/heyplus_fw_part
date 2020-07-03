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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef  COMPANION_DEVICE
#include <cutils/log.h>
#endif
#include "AlaUtils.h"
#include "Ala.h"

volatile static BOOLEAN Ala_WrblEnableChunking = FALSE;

UINT32 Ala_SkipMetaDataAndNonDigits(const UINT8* pBuf, UINT32 bufSize, UINT32 offset)
{
    char* metadataPrefix = "%%%";
    UINT32 offsetSkipped = 0;

    while(offset < bufSize)
    {
        if(!isxdigit(*(pBuf+offset)))
        {
            if(memcmp((pBuf+offset), metadataPrefix, strlen(metadataPrefix)) == 0)
            {
                (offset) += strlen(metadataPrefix);
                while((*(pBuf + offset) != '\r') && (*(pBuf + offset) != '\n'))
                {
                    (offset)++;
                }
                (offset)++;
            }
            else
                (offset)++;
        }
        else
            break;
    }
    offsetSkipped = offset;
    return offsetSkipped;
}

tJBL_STATUS Ala_GetCmdChunk(const UINT8* pScript, INT32 scriptLen, UINT32 *pScriptOffset, UINT16 *pScriptLenConsumed, UINT8* pChunkBuffer, UINT32 *pAlignedChunkSize)
{
    UINT32 chunkOffset, rawChunkSize, numApdus = 0, newChunkOffset, metaDataSize = 0;
    UINT16 bytesInLine = 0;
    UINT8 *pChunkBufferIndex = pChunkBuffer;
    UINT32 dwVal;
    BOOLEAN bCheckMetaData = TRUE;
    tJBL_STATUS result = STATUS_SUCCESS;

    ALOGD ("%s: enter", __func__);
    if((pScript == NULL || scriptLen == 0) || (pScriptOffset == NULL) || (pScriptLenConsumed == NULL) || (pChunkBuffer == NULL || pAlignedChunkSize == NULL))
    {
        ALOGD ("-- Invalid parameters pScript/scriptLen/pScriptOffset/pScriptLenConsumed/pChunkBuffer/pAlignedChunkSize");
        result = STATUS_FAILED;
        goto Ala_GetCmdChunkExit;
    }
    rawChunkSize = *pAlignedChunkSize - CMD_CHUNK_FOOTER_SIZE;
    *pAlignedChunkSize = 0;
    chunkOffset = 0;
    pScriptLenConsumed[numApdus] = chunkOffset;
    while(*pAlignedChunkSize+bytesInLine < rawChunkSize && chunkOffset<(scriptLen-*pScriptOffset))
    {
        if(bCheckMetaData)
        {
            newChunkOffset = Ala_SkipMetaDataAndNonDigits(pScript+*pScriptOffset, scriptLen, chunkOffset);
            metaDataSize += newChunkOffset - chunkOffset;
            chunkOffset = newChunkOffset;
            bCheckMetaData = FALSE;
            pScriptLenConsumed[numApdus] = chunkOffset;
            continue; /* To ensure that the new offset is not EOF */
        }
        if((*(pScript+*pScriptOffset+(chunkOffset))) =='\n' || (*(pScript+*pScriptOffset+(chunkOffset)))=='\r')
        {
            //*pChunkBufferIndex++ = (*(pScript+*pScriptOffset+(chunkOffset))); //new lines are not needed
            chunkOffset++;
            /* If the next byte is not a line-ending marker, we have finished reading a line */
            if((*(pScript+*pScriptOffset+(chunkOffset))) != '\n' && (*(pScript+*pScriptOffset+(chunkOffset))) != '\r')
            {
                numApdus++;
                pScriptLenConsumed[numApdus] = chunkOffset;
               *pAlignedChunkSize += bytesInLine;
                /* Reset the per-line vars before proceeding to the next line */
                bytesInLine = 0;
                bCheckMetaData = TRUE;
            }
        }
        else
        {
            /* We may end-up writing more than pAlignedChunkSize but its ok as we never cross rawChunkSize */
        	/*
        	 * If the expected output format is LS_SCRIPT_BINARY, only then the conversion is done.
        	 * Else, the same Text format is given out.
        	 */
#if (defined(COMPANION_DEVICE)||(NXP_LDR_SVC_SCRIPT_FORMAT == LS_SCRIPT_BINARY))
            sscanf(((const char *)pScript+*pScriptOffset+chunkOffset), "%2X", &dwVal);
            *pChunkBufferIndex++ = (UINT8)(dwVal & 0xFF);
#else
            /* To avoid compiler warning */
            UNUSED(dwVal)
            UNUSED(pChunkBufferIndex)
#endif
            bytesInLine++;
            chunkOffset+=2;
        }
    }
#if (!defined(COMPANION_DEVICE)&&(NXP_LDR_SVC_SCRIPT_FORMAT == LS_SCRIPT_TEXT))
    /* For Text format over write by total script length consumed by last APDU */
    *pAlignedChunkSize = pScriptLenConsumed[numApdus];
#endif
    if(*pScriptOffset == 0)
    {
        if(pScriptLenConsumed[numApdus]+*pScriptOffset == scriptLen)
            pChunkBuffer[*pAlignedChunkSize+CMD_CHUNK_FOOTER_SIZE-CHUNK_MARKER_OFFSET] = LS_CHUNK_NOT_REQUIRED;
        else
            pChunkBuffer[*pAlignedChunkSize+CMD_CHUNK_FOOTER_SIZE-CHUNK_MARKER_OFFSET] = LS_FIRST_CHUNK;
    }
    else
    {
        if(pScriptLenConsumed[numApdus]+*pScriptOffset == scriptLen)
            pChunkBuffer[*pAlignedChunkSize+CMD_CHUNK_FOOTER_SIZE-CHUNK_MARKER_OFFSET] = LS_LAST_CHUNK;
        else
            pChunkBuffer[*pAlignedChunkSize+CMD_CHUNK_FOOTER_SIZE-CHUNK_MARKER_OFFSET] = LS_INTERMEDIATE_CHUNK;
    }

    pChunkBuffer[(*pAlignedChunkSize)+CMD_CHUNK_FOOTER_SIZE-NUM_APDU_OFFSET] = numApdus;
    pChunkBuffer[(*pAlignedChunkSize)+CMD_CHUNK_FOOTER_SIZE-TOTAL_SCRIPT_LEN_OFFSET] = (UINT8)((scriptLen >> 24) & 0xFF);
    pChunkBuffer[(*pAlignedChunkSize)+CMD_CHUNK_FOOTER_SIZE-TOTAL_SCRIPT_LEN_OFFSET+1] = (UINT8)((scriptLen >> 16) & 0xFF);
    pChunkBuffer[(*pAlignedChunkSize)+CMD_CHUNK_FOOTER_SIZE-TOTAL_SCRIPT_LEN_OFFSET+2] = (UINT8)((scriptLen >> 8) & 0xFF);
    pChunkBuffer[(*pAlignedChunkSize)+CMD_CHUNK_FOOTER_SIZE-TOTAL_SCRIPT_LEN_OFFSET+3] = (UINT8)(scriptLen & 0xFF);
    (*pAlignedChunkSize) += CMD_CHUNK_FOOTER_SIZE;/* To count for the rest of the fields */
    ALOGD ("scriptOffset Post Cmd Chunking would be = %d", *pScriptOffset+pScriptLenConsumed[numApdus]);

Ala_GetCmdChunkExit:
    ALOGD ("%s: exit, status=%d", __func__, result);
    return result;
}

tJBL_STATUS Ala_StoreRspChunk(UINT8* pRespData, UINT16 respLen, UINT32 *pRespOffset, UINT32 *pScriptOffset, UINT16 *pScriptLenConsumed, UINT8* pRespChunkBuffer, UINT16 respChunkSize)
{
    tJBL_STATUS result = STATUS_SUCCESS;
    UINT8 bytesInLine = 0;
    UINT16 index;
    UINT32 dstIndex;
    ALOGD ("%s: enter", __func__);
    if((pRespData == NULL || respLen == 0) || (pRespOffset == NULL) || (pScriptOffset == NULL) || (pScriptLenConsumed == NULL) || (pRespChunkBuffer == NULL || respChunkSize == 0))
    {
        ALOGD ("-- Invalid parameters pRespData/respLen/pRespOffset/pScriptOffset/pScriptLenConsumed/pRespChunkBuffer/respChunkSize");
        result = STATUS_FAILED;
        goto Ala_StoreRspChunkExit;
    }
    ALOGD ("respChunkSize = %d", respChunkSize - RSP_CHUNK_FOOTER_SIZE);
    index = respChunkSize - CMD_CHUNK_OFFSET_OFFSET;
    /* Correct the script offset to consider again any unprocessed script in the command chunk buffer */
    *pScriptOffset = *pScriptOffset + pScriptLenConsumed[(pRespChunkBuffer[index]<<8) | (pRespChunkBuffer[index+1])];
    ALOGD ("scriptOffset post correction = %d", *pScriptOffset);
    for(index = 0, dstIndex = *pRespOffset; (index < respChunkSize - RSP_CHUNK_FOOTER_SIZE); index++) {
        /* Capture TLV's length so as to add EOL marker at end */
        if(bytesInLine == 0 && pRespChunkBuffer[index] == 0x61) { /* Type for TLV is always 0x61 */
            if(pRespChunkBuffer[index+1] == 0x81) {
                bytesInLine = pRespChunkBuffer[index + 2]+2+1;/* +1 for extra byte of L field */
            }
            else if(pRespChunkBuffer[index+1] == 0x82)
            {
                bytesInLine = ((pRespChunkBuffer[index+2]<<8) | pRespChunkBuffer[index+3])+2+2;/* Last +2 for extra 2-bytes of L field */
            }
            else
                bytesInLine = pRespChunkBuffer[index+1]+2;/* +2 to count for T and L fields */
        }
        /* Convert 1-byte bin to Text */
        if(dstIndex+2 < respLen) { /* Ensure 2 char-space is available */
            sprintf(((char *) pRespData + dstIndex), "%.2X", pRespChunkBuffer[index]);
            dstIndex+=2;
            bytesInLine--;/* We converted 1-byte*/
        }else
            break;

        /* Add a EOL marker at end of TLV */
        if(bytesInLine == 0)
        {
            if((dstIndex+1 < respLen)) { /* Ensure 1 char-space is available */
                sprintf(((char *) pRespData + dstIndex), "\n");
                dstIndex++;
            }
            else
                break;
        }
    }
    if(index != respChunkSize - RSP_CHUNK_FOOTER_SIZE) /* Not enough space to write the response to */
        result = STATUS_FAILED;
    ALOGD ("Response Chunk: %.*s", dstIndex-*pRespOffset, pRespData+*pRespOffset);
    *pRespOffset = dstIndex;

Ala_StoreRspChunkExit:
    ALOGD ("%s: exit, status=%d", __func__, result);
    return result;
}

void Ala_SetChunking(BOOLEAN fl) {
    Ala_WrblEnableChunking = fl;
}

BOOLEAN Ala_IsChunkingEnabled(void) {
    return Ala_WrblEnableChunking;
}
