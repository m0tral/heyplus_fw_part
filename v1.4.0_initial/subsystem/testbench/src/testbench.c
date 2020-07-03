/*
* Copyright (c), Ryeex Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
//#include "ryos.h"
#include "ry_utils.h"
#include "ry_type.h"
#include "ry_mcu_config.h"

/* Board */
#include "board.h"
#include "board_control.h"
#include "ry_hal_inc.h"

#include "ry_gsensor.h"
#include "ry_gui.h"
#include "am_hal_sysctrl.h"

/* Subsystem API */
#include "hrm.h"
#include "gui_bare.h"

#include "app_pm.h"
#include "testbench.h"
#include "json.h"
#include "device_management_service.h"

/*********************************************************************
 * CONSTANTS
 */ 

 /*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

//testbench function of sakesys
void tb_setup_sakesys(tb_script_t* sc)
{
    
}

void tb_run_sakesys(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:
            //add run function here
            lp_func_sakesys();            
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_sakesys();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_sakesys();
            break;
        case TB_CMD_POFF:       
            pcf_func_sakesys();
            //add run function here
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_sakesys(tb_script_t* sc)
{

}

//testbench function of gsensor
void tb_setup_gsensor(tb_script_t* sc)
{
    
}

void tb_run_gsensor(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_gsensor();
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_gsensor();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_gsensor();
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_gsensor();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_gsensor(tb_script_t* sc)
{

}

//testbench function of hrm
void tb_setup_hrm(tb_script_t* sc)
{
    
}

void tb_run_hrm(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_hrm();
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_hrm();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_hrm(HRM_START_FIXED);
            set_hrm_working_mode(HRM_MODE_MANUAL);
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_hrm();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_hrm(tb_script_t* sc)
{

}

//testbench function of oled
void tb_setup_oled(tb_script_t* sc)
{
    tb_oled_t* oled_p = (tb_oled_t*)(sc->para);
    
}

void tb_run_oled(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_oled();
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_oled();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_oled(1);
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_oled();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_oled(tb_script_t* sc)
{

}

//testbench function of touch
void tb_setup_touch(tb_script_t* sc)
{
    
}

void tb_run_touch(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_touch();            
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_touch();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_touch();
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_touch();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_touch(tb_script_t* sc)
{

}


//testbench function of exflash
void tb_setup_exflash(tb_script_t* sc)
{
    
}

void tb_run_exflash(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_exflash();            
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_exflash();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_exflash();
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_exflash();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_exflash(tb_script_t* sc)
{

}

//testbench function of nfc
void tb_setup_nfc(tb_script_t* sc)
{
    
}

void tb_run_nfc(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_nfc();
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_nfc();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_nfc();
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_nfc();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_nfc(tb_script_t* sc)
{

}


//testbench function of em9304
void tb_setup_em9304(tb_script_t* sc)
{
    
}

void tb_run_em9304(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            lp_func_em9304();
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            pd_func_em9304();
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            _start_func_em9304();
            break;
        case TB_CMD_POFF:            
            //add run function here
            pcf_func_em9304();
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_em9304(tb_script_t* sc)
{

}

//testbench function of motor
void tb_setup_motor(tb_script_t* sc)
{
    
}

void tb_run_motor(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            break;
        case TB_CMD_POFF:            
            //add run function here
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_motor(tb_script_t* sc)
{

}

//testbench function of led
void tb_setup_led(tb_script_t* sc)
{
    
}

void tb_run_led(tb_script_t* sc)
{
    switch (sc->cmd){
        case TB_CMD_RUN:
            //add run function here
            break;
        case TB_CMD_LP:            
            //add run function here
            break;
        case TB_CMD_SLEEP:            
            //add run function here
            break;
        case TB_CMD_WAKEUP:            
            //add run function here
            break;
        case TB_CMD_POFF:            
            //add run function here
            break;
        default:
            // Unknown command
            break;
    }
}

void tb_teardown_led(tb_script_t* sc)
{

}

/*********************************************************************
 * LOCAL VARIABLES
 */
tb_app_t tb_app_array[] = {
    {JSON_METHOD_ID_SAKESYS, NULL, tb_setup_sakesys, tb_run_sakesys, tb_teardown_sakesys},
    {JSON_METHOD_ID_GSENSOR, NULL, tb_setup_gsensor, tb_run_gsensor, tb_teardown_gsensor},
    {JSON_METHOD_ID_HRM,     NULL, tb_setup_hrm,     tb_run_hrm,     tb_teardown_hrm    },
    {JSON_METHOD_ID_OLED,    NULL, tb_setup_oled,    tb_run_oled,    tb_teardown_oled   },
    {JSON_METHOD_ID_TOUCH,   NULL, tb_setup_touch,   tb_run_touch,   tb_teardown_touch  },
    {JSON_METHOD_ID_EXFLASH, NULL, tb_setup_exflash, tb_run_exflash, tb_teardown_exflash},
    {JSON_METHOD_ID_NFC,     NULL, tb_setup_nfc,     tb_run_nfc,     tb_teardown_nfc    },
    {JSON_METHOD_ID_EM9304,  NULL, tb_setup_em9304,  tb_run_em9304,  tb_teardown_em9304 },
    {JSON_METHOD_ID_MOTOR,   NULL, tb_setup_motor,   tb_run_motor,   tb_teardown_motor  },
    {JSON_METHOD_ID_LED,     NULL, tb_setup_led,     tb_run_led,     tb_teardown_led    },
};

tb_script_t tb_sc = {
    "hello testbench",
    JSON_METHOD_ID_HRM,
    TB_CMD_POFF,    
    TB_ST_IDLE,
    0,
    {0},
    {0},
};


/**
 * @brief   testbench_machine
 *          decription: get test command and action
 *
 * @param   None
 *
 * @return  None
 */
void testbench_machine(void)
{
    int i;
    if (tb_sc.status == TB_ST_READY){
        LOG_DEBUG("get test cmd: id %d %s\r\n", tb_sc.id, tb_sc.description);
        int len = (sizeof(tb_app_array) / sizeof(tb_app_t));
        for (i = 0; i < len; i++){
            if (tb_sc.id == tb_app_array[i].id){
                tb_app_array[i].pscript = &tb_sc;
                tb_app_array[i].pscript->status = TB_ST_BUSY;

                LOG_DEBUG("test setup: id %d %s\r\n", tb_app_array[i].id, tb_app_array[i].pscript->description);
                tb_app_array[i].setup(&tb_sc);

                LOG_DEBUG("test run: id %d %s\r\n", tb_app_array[i].id, tb_app_array[i].pscript->description);
                tb_app_array[i].run(&tb_sc);

                LOG_DEBUG("test teardown: id %d %s\r\n", tb_app_array[i].id, tb_app_array[i].pscript->description);
                tb_app_array[i].teardown(&tb_sc);
                
                tb_app_array[i].pscript->status = TB_ST_DONE;
                LOG_DEBUG("test done: id %d %s\r\n", tb_app_array[i].id, tb_app_array[i].pscript->description);
            }
        }
    }
}

/* [] END OF FILE */
