/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __APDU_H__
#define __APDU_H__

#include "ry_type.h"


void apdu_raw_handler(uint8_t* value, unsigned int length, unsigned short* pRspSize, uint8_t* pRespBuf, u16_t cmdId, u8_t isSec);




#endif  /* __APDU_H__ */
