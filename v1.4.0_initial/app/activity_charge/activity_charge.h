#ifndef __ACTIVITY_CHARGE_H__
#define __ACTIVITY_CHARGE_H__

#include "window_management_service.h"



extern activity_t activity_charge;

/**
 * @brief   charging_activity_back_processing to back or root
 *          time not enough - back; else: enough to root
 *
 * @param   None
 *
 * @return  None
 */
void charging_activity_back_processing(void);

ry_sts_t show_charge_view(void* usrdata);

ry_sts_t exit_charge_view(void);

void charging_swithing_timer_set(uint32_t idx);

#endif

