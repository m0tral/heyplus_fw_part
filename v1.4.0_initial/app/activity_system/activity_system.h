#ifndef __ACTIVITY_SYSTEM_H_

#define __ACTIVITY_SYSTEM_H_



#include "window_management_service.h"


extern activity_t activity_system;



typedef enum ui_system_status{
    UI_SYS_UPGRADING,
    UI_SYS_UPGRADE_SUCC,
    UI_SYS_UPGRADE_FAIL,
    
    UI_SYS_BINDING,
    UI_SYS_BIND_SUCC,
    UI_SYS_BIND_FAIL,

    UI_SYS_UNBINDING,
    UI_SYS_UNBIND_SUCC,
    UI_SYS_UNBIND_FAIL,

    UI_SYS_BIND_ACK,
    
}ui_system_status_t;






ry_sts_t show_system(void * usrdata);
ry_sts_t exit_system(void);























#endif


