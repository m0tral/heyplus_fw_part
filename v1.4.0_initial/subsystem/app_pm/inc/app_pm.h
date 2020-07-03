/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __APP_PM_H__
#define __APP_PM_H__

/*
 * TYPES
 *******************************************************************************
 */
typedef struct
{
    uint8_t pm_module_name[20];
    uint8_t lp_deep;            /*low power deep level: 
                                                        0 - do nothing, 
                                                        1 - low power, 
                                                        2 - power down mode, 
                                                        3 - power been cut off 
                                                        4 - re-start or start            
                                                        */   
    void   (*lp_func)(void);    //low power mode
    void   (*pd_func)(void);    //power down mode
    void   (*pcf_func)(void);   //power been cut down 
    void   (*_str_func)(void);  //re-start or start                                            
}pm_app_t;

typedef struct
{
    uint8_t sys;
    uint8_t gsensor;
    uint8_t hrm;
    uint8_t oled;
    uint8_t touch;
    uint8_t exflash;
    uint8_t nfc;
    uint8_t em9304;
    uint8_t motor;
    uint8_t led;
}pm_st_module_t;

extern pm_st_module_t g_pm_st;

typedef enum {
    PM_ST_IDLE      	=   0x00,
    PM_ST_LOW_POWER     =   0x01,
    PM_ST_POWER_DOWN    =   0x02,
    PM_ST_POWER_CUT_OFF =   0x03,
    PM_ST_POWER_START   =   0x04,
    PM_ST_POWER_RUNNING =   0x05,    
} pm_st_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */

//pm function of sakesys
void lp_func_sakesys(void);
void pd_func_sakesys(void);
void pcf_func_sakesys(void);
void _start_func_sakesys(void);

//pm function of gsensor
void lp_func_gsensor(void);
void pd_func_gsensor(void);
void pcf_func_gsensor(void);
void _start_func_gsensor(void);

//pm function of hrm
void lp_func_hrm(void);
void pd_func_hrm(void);
void pcf_func_hrm(void);
pm_st_t get_hrm_state(void);


/**
 * @brief   hrm sensor start
 *          Note: wakeup do this
 *
 * @param   None
 *
 * @return  mode: working mode
 * 
 */
void _start_func_hrm(uint32_t mode);

//pm function of oled
void lp_func_oled(void);
void pd_func_oled(void);
void pcf_func_oled(void);
void _start_func_oled(bool displayOn);
pm_st_t get_oled_state(void);


//pm function of touch
void lp_func_touch(void);
void pd_func_touch(void);
void pcf_func_touch(void);
void _start_func_touch(void);
pm_st_t get_touch_state(void);

//pm function of exflash
void lp_func_exflash(void);
void pd_func_exflash(void);
void pcf_func_exflash(void);
void _start_func_exflash(void);

//pm function of nfc
void lp_func_nfc(void);
void pd_func_nfc(void);
void pcf_func_nfc(void);
void _start_func_nfc(void);

//pm function of em9304
void lp_func_em9304(void);
void pd_func_em9304(void);
void pcf_func_em9304(void);
void _start_func_em9304(void);

//pm function of motor
void lp_func_motor(void);
void pd_func_motor(void);
void pcf_func_motor(void);
void _start_func_motor(void);

//pm function of led
void lp_func_led(void);
void pd_func_led(void);
void pcf_func_led(void);
void _start_func_led(void);
void exit_lp_func_oled(void);

/**
 * @brief   module_power_control
 *          decription: basic module power control
 *
 * @param   None
 *
 * @return  None
 */
void module_power_control(void);

/**
 * @brief   API to clear get_gui_on_time to zero
 *
 * @param   None
 *
 * @return  None
 */
void clear_hrm_on_time(void);

/**
 * @brief   API to get get_gui_on_time
 *
 * @param   None
 *
 * @return  get_gui_on_time result
 */
uint32_t get_hrm_on_time(void);

#endif //__APP_PM_H__
