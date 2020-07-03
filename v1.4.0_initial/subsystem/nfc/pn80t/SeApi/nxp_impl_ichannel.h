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
/**
 * \file  nxp_impl_ichannel.h
 * \brief NXP implementation of iChannel for use when both SeApi and
 *         SeSupport are implemented on the wearable device (as opposed
 *         to the case where SeSupport is implemented on the wearable's
 *         host device)
 *
 *         This can be used as a reference for the integrator in the
 *         case where SeSupport is implemented on the host and the
 *         integrator is responsible for implementing the iChannel and
 *         supporting it's functionality.
 */

/**
 * defgroup seapi_ichannel_reference              nxp_impl_ichannel
 */
#include "inc/p61_data_types.h"

#ifdef __cplusplus
extern "C"{
#endif

/** \addtogroup seapi_ichannel_reference
 * @{ */

/**
 *
 * \brief Open the connection to the eSE (wired mode)
 *
 * \return TRUE if OK
 */
BOOLEAN nxp_impl_ichannel_open(void);

/**
 *
 * \brief Close the connection to the eSE (wired mode)
 *
 * \return TRUE if OK
 */
BOOLEAN nxp_impl_ichannel_close(void);

/**
 *
 * \brief Send, to the eSE, an APDU or command and receive a response.
 *
 * \param xmitBuffer - [in] pointer to a buffer containing the APDU/command
 *                          to send to the eSE
 * \param xmitBufferSize - [in] number of bytes of data in the xmitBuffer that
 *                              should be sent to the eSE
 * \param recvBuffer - [out] pointer to a buffer that will receive the response
 *                              from the eSE.  This should generally be
 *                              256-1024 bytes in size
 * \param recvBufferMaxSize - [in] the size of the recvBuffer, the maximum amount of
 *                                 data that can be written into the buffer
 * \param recvBufferActualSize - [out] pointer to an INT32 that will receive the
 *                                     actual number of bytes written into the
 *                                     recvBuffer.  This will always be less than
 *                                     or equal to recvBufferMaxSize
 * \param timeoutMillisec - [in] maximum amount of time, in milliseconds, to wait
 *                               for the eSE to provide it's response.
 *
 * \return TRUE if OK
 */
BOOLEAN nxp_impl_ichannel_transceive(UINT8* xmitBuffer, INT32 xmitBufferSize, UINT8* recvBuffer,
                     INT32 recvBufferMaxSize, INT32 *recvBufferActualSize, INT32 timeoutMillisec);

/**
 *
 * \brief Reset the eSE.  Utilize the SeApi function SeApi_WiredResetEse() to
 *        perform the reset. In other words, this function's implementation
 *        should call SeApi_WiredResetEse().
 *
 * \return NA
 */
void nxp_impl_ichannel_doeSE_Reset(void);
/**
 *
 * \brief Performs a reset to eSE during JCOP OS update depending on
 *        Power schemes configured
 *
 * \return NA
 */
void nxp_impl_ichannel_JcopDnd_SeReset(void);
/**
 *
 * \brief Controls the wired channel settings
 *
 * \param pConfig - [in] pointer to a structure of control params
 *
 * \return TRUE if OK
 */
BOOLEAN nxp_impl_ichannel_control(void);
/** @} */

#ifdef __cplusplus
}
#endif
