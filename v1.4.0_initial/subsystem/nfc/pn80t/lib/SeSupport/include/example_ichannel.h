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

#include "p61_data_types.h"

/*******************************************************************************
**
** Function:        Open
**
** Description:     Initialize the channel.
**
** Returns:         True if ok.
**
*******************************************************************************/
BOOLEAN example_ichannel_open();

/*******************************************************************************
**
** Function:        close
**
** Description:     Close the channel.
**
** Returns:         True if ok.
**
*******************************************************************************/
BOOLEAN example_ichannel_close();

/*******************************************************************************
**
** Function:        transceive
**
** Description:     Send data to the secure element; read it's response.
**                  xmitBuffer: Data to transmit.
**                  xmitBufferSize: Length of data, Maximum allowed is 65535
**                  recvBuffer: Buffer to receive response.
**                  recvBufferMaxSize: Maximum size of buffer, Maximum allowed is 65535
**                  recvBufferActualSize: Actual length of response.
**                  timeoutMillisec: timeout in millisecond
**
** Returns:         True if ok.
**
*******************************************************************************/
BOOLEAN example_ichannel_transceive(UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
                     INT32 recvBufferMaxSize, INT32 *recvBufferActualSize, INT32 timeoutMillisec);

/*******************************************************************************
**
** Function:        doeSE_Reset
**
** Description:     Reset the eSE.  It is sufficient to disable and re-enable the
** 					wired mode interface.
**
** Returns:         None.
**
*******************************************************************************/

void example_ichannel_doeSE_Reset();

