/******************************************************************************
 *
 *  Copyright (C) 2009-2014 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * NOT A CONTRIBUTION
 */


/******************************************************************************
 *
 *  This file contains functions that interface with the NFC NCI transport.
 *  On the receive side, it routes events to the appropriate handler
 *  (callback). On the transmit side, it manages the command transmission.
 *
 ******************************************************************************/
#include <string.h>
#include "nfc_target.h"
#include "bt_types.h"

#if (NFC_INCLUDED == TRUE)
#include "nfc_api.h"
#include "nci_hmsgs.h"
#include "ce_api.h"
#include "ce_int.h"
#include "gki.h"

tCE_CB  ce_cb;

/*******************************************************************************
*******************************************************************************/
void ce_init (void)
{
    memset (&ce_cb, 0, sizeof (tCE_CB));
    ce_cb.trace_level = NFC_INITIAL_TRACE_LEVEL;
}


/*******************************************************************************
**
** Function         CE_SetTraceLevel
**
** Description      This function sets the trace level for Card Emulation mode.
**                  If called with a value of 0xFF,
**                  it simply returns the current trace level.
**
** Returns          The new or current trace level
**
*******************************************************************************/
UINT8 CE_SetTraceLevel (UINT8 new_level)
{
    if (new_level != 0xFF)
        ce_cb.trace_level = new_level;

    return (ce_cb.trace_level);
}

#endif /* NFC_INCLUDED == TRUE */
