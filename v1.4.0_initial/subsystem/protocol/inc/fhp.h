/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __FHP_H__
#define __FHP_H__

#include "ry_type.h"
#include "fit_health.pb.h"
#include "rbp.pb.h"


/**
 * @brief   Fit and Health Protocol Parser
 *
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
ry_sts_t fhp_parser(CMD cmd, uint8_t* data, uint32_t len);


/**
 * @brief   API to send fit and health data.
 *
 * @param   type, device infomation sub-type
 * @param   data, the recevied data.
 * @param   len, length of the recevied data.
 *
 * @return  None
 */
ry_sts_t fhp_send(FH_TYPE type, FH_SUB_TYPE subtype, uint8_t* data, int len);



#endif  /* __FHP_H__ */
