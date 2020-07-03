
#ifndef __ACTIVITY_LOADING_H__
#define __ACTIVITY_LOADING_H__

#include "window_management_service.h"


extern activity_t activity_loading;
ry_sts_t loading_activity_start(void* p_arg);
ry_sts_t loading_activity_update(void* p_arg);
ry_sts_t loading_activity_stop(void* p_arg);


#endif
