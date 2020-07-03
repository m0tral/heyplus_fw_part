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
#include "ry_hal_spi_flash.h"
#include "am_bsp_gpio.h"

/* Subsystem API */
#include "hrm.h"
#include "gui_bare.h"
#include "app_pm.h"

/* driver api */
#include "ppsi26x.h"
#include "gsensor.h"
#include "mcube_drv.h"
#include "AFE4410_API_C.h"
#include "am_devices_rm67162.h"
#include "am_devices_CY8C4014.h"
#include "device_management_service.h"

/*********************************************************************
 * CONSTANTS
 */ 

 /*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */
pm_st_module_t g_pm_st;
static uint8_t  hrm_working_mode;
static uint32_t hrm_on_time;
static uint32_t hrm_on_time_start;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
//pm function of sakesys
void lp_func_sakesys(void)
{

}

void pd_func_sakesys(void)
{
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
}

void pcf_func_sakesys(void)
{
    LOG_DEBUG("sakesys enter deepsleep.\r\n");
    am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
    power_main_sys_on(0);
}

void _start_func_sakesys(void)
{
    power_main_sys_on(1);
    //power_main_5v_on(1);
}

//pm function of gsensor
void lp_func_gsensor(void)
{
    M_DRV_MC36XX_SetMode(E_M_DRV_MC36XX_MODE_STANDBY);
}

void pd_func_gsensor(void)
{
    if (g_pm_st.gsensor != PM_ST_POWER_DOWN){
        M_DRV_MC36XX_SetMode(E_M_DRV_MC36XX_MODE_SLEEP); 
        g_pm_st.gsensor = PM_ST_POWER_DOWN;
    }
}

void pcf_func_gsensor(void)
{
    if (g_pm_st.gsensor != PM_ST_POWER_CUT_OFF){
        M_DRV_MC36XX_SetMode(E_M_DRV_MC36XX_MODE_SLEEP);  
        g_pm_st.gsensor = PM_ST_POWER_CUT_OFF; 
    }
}

void _start_func_gsensor(void)
{
#if (RY_GSENSOR_ENABLE == TRUE)
    if (g_pm_st.gsensor != PM_ST_POWER_START){
        gsensor_init();
        g_pm_st.gsensor = PM_ST_POWER_START;         
    }
#endif
}


/**
 * @brief   API to get get_gui_on_time
 *
 * @param   None
 *
 * @return  get_gui_on_time result
 */
uint32_t get_hrm_on_time(void)
{
	return hrm_on_time;
}

/**
 * @brief   API to clear get_gui_on_time to zero
 *
 * @param   None
 *
 * @return  None
 */
void clear_hrm_on_time(void)
{
	hrm_on_time = 0;
}

//pm function of hrm
void lp_func_hrm(void)
{

}

void pd_func_hrm(void)
{
    if (g_pm_st.hrm != PM_ST_POWER_DOWN){
        ppsi26x_Disable_HW();
        ry_hal_i2cm_uninit(I2C_IDX_HR);
        g_pm_st.hrm = PM_ST_POWER_DOWN;
    }
}

void pcf_func_hrm(void)
{
    if (g_pm_st.hrm != PM_ST_POWER_CUT_OFF){
        power_hrm_on(0);
        if (hrm_on_time_start != 0){
            hrm_on_time += ry_hal_calc_second(ry_hal_clock_time() - hrm_on_time_start);
            hrm_on_time_start = 0;
        }
        pd_func_hrm();
        power_hrm_on(0);
        g_pm_st.hrm = PM_ST_POWER_CUT_OFF;
    }
}

void _start_func_hrm(uint32_t mode)
{
    if (g_pm_st.hrm != PM_ST_POWER_START){
        hrm_on_time_start = ry_hal_clock_time();
        hrm_thread_entry(mode);
        g_pm_st.hrm = PM_ST_POWER_START;
    }
}

void start_func_hrm(void)
{
    _start_func_hrm(HRM_START_AUTO);
    set_hrm_working_mode(HRM_MODE_MANUAL);
}

pm_st_t get_hrm_state(void)
{
    return (pm_st_t)g_pm_st.hrm;
}

//pm function of oled
void lp_func_oled(void)
{
    //set low brightness and other low power setting
    if (g_pm_st.oled != PM_ST_LOW_POWER){
        oled_enter_low_power();
        g_pm_st.oled = PM_ST_LOW_POWER;
    }
}

void pd_func_oled(void)
{
    if (g_pm_st.oled != PM_ST_POWER_DOWN){
        am_devices_rm67162_powerdown();
        g_pm_st.oled = PM_ST_POWER_DOWN;
    }        
}

void pcf_func_oled(void)
{
    //LOG_DEBUG("[pcf_func_oled] start. %d \r\n", g_pm_st.oled);
    if (g_pm_st.oled != PM_ST_POWER_CUT_OFF){
        am_devices_rm67162_powerdown();
        power_oled_on(0);
        g_pm_st.oled = PM_ST_POWER_CUT_OFF;
    }
    //LOG_DEBUG("[pcf_func_oled] end. %d \r\n", g_pm_st.oled);
    
}

void _start_func_oled(bool displayOn)
{
    if (g_pm_st.oled != PM_ST_POWER_START){

        //ryos_delay_ms(10);

        am_devices_rm67162_poweron();
        g_pm_st.oled = PM_ST_POWER_START;
        
        if (displayOn) {
            am_devices_rm67162_displayoff();
            am_devices_rm67162_displayon();
        }
    }
}

void exit_lp_func_oled(void)
{
    if (g_pm_st.oled != PM_ST_POWER_RUNNING){
       oled_restore_from_low_power();
        g_pm_st.oled = PM_ST_POWER_RUNNING;
    }        
}

pm_st_t get_oled_state(void)
{
    return (pm_st_t)g_pm_st.oled;
}


//pm function of touch
void lp_func_touch(void)
{
    am_devices_cy8c4014_set_work_mode(TP_WORKING_BTN_ONLY);
}

void pd_func_touch(void)
{
    am_devices_cy8c4014_set_work_mode(TP_WORKING_BTN_ONLY);
}

void pcf_func_touch(void)
{
    if (g_pm_st.touch != PM_ST_POWER_CUT_OFF){
        cy8c4011_gpio_uninit();
        g_pm_st.touch = PM_ST_POWER_CUT_OFF;        
    }
}

void _start_func_touch(void)
{
    if (g_pm_st.touch != PM_ST_POWER_START){
        cy8c4011_gpio_init();
        g_pm_st.touch = PM_ST_POWER_START;
    }
}


pm_st_t get_touch_state(void)
{
    return (pm_st_t)g_pm_st.touch;
}


//pm function of exflash
void lp_func_exflash(void)
{
    ry_hal_spi_flash_uninit();
}

void pd_func_exflash(void)
{
    ry_hal_spi_flash_uninit();
}

void pcf_func_exflash(void)
{
    ry_hal_spi_flash_uninit();
}

void _start_func_exflash(void)
{
    ry_hal_spi_flash_init();
}


//pm function of nfc
void lp_func_nfc(void)
{

}

void pd_func_nfc(void)
{

}

void pcf_func_nfc(void)
{

}

void _start_func_nfc(void)
{

}

//pm function of em9304
void lp_func_em9304(void)
{

}

void pd_func_em9304(void)
{

}

void pcf_func_em9304(void)
{

}

void _start_func_em9304(void)
{

}

//pm_app_t pm_app_array[] = {
//    {"pm_app_sakesys", 0, lp_func_sakesys, pd_func_sakesys, pcf_func_sakesys, _start_func_sakesys},    
//    {"pm_app_gsensor", 0, lp_func_gsensor, pd_func_gsensor, pcf_func_gsensor, _start_func_gsensor},
//    {"pm_app_hrm",     0, lp_func_hrm,     pd_func_hrm,     pcf_func_hrm,     start_func_hrm    },
//    {"pm_app_oled",    0, lp_func_oled,    pd_func_oled,    pcf_func_oled,    _start_func_oled   },
//    {"pm_app_touch",   0, lp_func_touch,   pd_func_touch,   pcf_func_touch,   _start_func_touch  },
//    {"pm_app_exflash", 0, lp_func_exflash, pd_func_exflash, pcf_func_exflash, _start_func_exflash},
//    {"pm_app_nfc",     0, lp_func_nfc,     pd_func_nfc,     pcf_func_nfc,     _start_func_nfc    },
//    {"pm_app_em9304",  0, lp_func_em9304,  pd_func_em9304,  pcf_func_em9304,  _start_func_em9304 },
//};

///**
// * @brief   module_power_control
// *          decription: basic module power management control
// *
// * @param   None
// *
// * @return  None
// */
//void module_power_control(void)
//{
//    int i;
//    int len = (sizeof(pm_app_array) / sizeof(pm_app_t));
//    for (i = len - 1; i >= 0; i--){
//        if (pm_app_array[i].lp_deep == 1){
//            LOG_DEBUG("low power: %d, %s\r\n", pm_app_array[i].lp_deep, pm_app_array[i].pm_module_name);
//            pm_app_array[i].lp_func();
//        }
//        else if(pm_app_array[i].lp_deep == 2){
//            LOG_DEBUG("power down: %d, %s\r\n", pm_app_array[i].lp_deep, pm_app_array[i].pm_module_name);
//            pm_app_array[i].pd_func();
//        }
//        else if(pm_app_array[i].lp_deep == 3){
//            LOG_DEBUG("power cut off: %d, %s\r\n", pm_app_array[i].lp_deep, pm_app_array[i].pm_module_name);
//            pm_app_array[i].pcf_func();        
//        }
//        else if (pm_app_array[i].lp_deep == 4){
//            LOG_DEBUG("normal run: %d, %s\r\n", pm_app_array[i].lp_deep, pm_app_array[i].pm_module_name);
//            pm_app_array[i]._str_func();
//        }
//    }
//}


/* [] END OF FILE */
