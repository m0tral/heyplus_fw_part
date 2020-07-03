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
#ifdef __cplusplus

extern "C" {

#endif


#ifndef JCDNLD_H_
#define JCDNLD_H_

#include "IChannel.h"
#include "p61_data_types.h"

#if(JCDN_WEARABLE_BUFFERED == TRUE)
typedef struct
{
	UINT16 (*cbSet)(UINT8); 	// callback to set the status byte
	UINT16 (*cbGet)(UINT8*);	// callback to get the status byte
}tJcopOs_StorageCb;
#endif

/*******************************************************************************
**
** Function:        JCDNLD_Init
**
** Description:     Initializes the JCOP library and opens the DWP communication channel
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
#if(JCDN_WEARABLE_BUFFERED == TRUE)
unsigned char JCDNLD_Init(IChannel_t *channel, tJcopOs_StorageCb* cb);
#else
unsigned char JCDNLD_Init(IChannel_t *channel);
#endif

/*******************************************************************************
**
** Function:        JCDNLD_StartDownload
**
** Description:     Starts the JCOP update
**
** Returns:         SUCCESS if ok.
**
*******************************************************************************/
#if(JCDN_WEARABLE_BUFFERED == TRUE)
unsigned char JCDNLD_StartDownload(UINT32 lenBuf1, const UINT8 *pBuf1,
		 						   UINT32 lenBuf2, const UINT8 *pBuf2,
								   UINT32 lenBuf3, const UINT8 *pBuf3);
#else
unsigned char JCDNLD_StartDownload();
#endif

/*******************************************************************************
**
** Function:        JCDNLD_DeInit
**
** Description:     Deinitializes the JCOP Lib
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
BOOLEAN JCDNLD_DeInit();

/*******************************************************************************
**
** Function:        JCDNLD_CheckVersion
**
** Description:     Check the existing JCOP OS version
**
** Returns:         TRUE if ok.
**
*******************************************************************************/
BOOLEAN JCDNLD_CheckVersion();


#endif /* JCDNLD_H_ */

#ifdef __cplusplus

}

#endif
