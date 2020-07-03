/*
* Copyright (c), Ryeex Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#ifndef __APP_INTERFACE_H__
#define __APP_INTERFACE_H__

/*
 * power management of system and devices
 *******************************************************************************
 */
#include <stdint.h>
#include "ry_type.h"
#include "app_pm.h"
#include "am_devices_CY8C4014.h"

#if 0               //在app_pm.c中实现各个模块可支持的工作模式，各个模式定义如下
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
#endif

/*
 * devices id and version
 *******************************************************************************
 */
__weak uint8_t *sys_get_ver(void);
__weak uint8_t *sys_get_sn(void);
__weak uint8_t *sys_get_did(void);
__weak uint8_t *sys_get_mac(void);
__weak uint8_t *sys_get_hex_conf(void);

uint8_t gsensor_get_id(void);
uint8_t hrm_get_id(void);
__weak uint8_t nfc_get_id(void);
__weak uint32_t oled_get_id(void);

/**
 * @brief   API to get touch ID
 *
 * @param   None
 * 
 * @return  touch ID
 *          byte[0]: PANELMAKER     屏厂代码，  默认为01
 *          byte[1]: FW VERSION     固件版本号，默认为01
 *
 */
uint32_t touch_get_id(void);
uint32_t exflash_get_id(void);
uint8_t *ble_get_version(void);
//motor no id
//led no id


/*
 * mcu and devices get data
 *******************************************************************************
 */


//ntc get data
/**
 * @brief   API to get ntc value
 *
 * @param   None
 * 
 * @return  pointer - pointer of the ntc value
 */
__weak uint32_t sys_get_ntc(void);

//rtc get time
/**
 * @brief   API to get rtc value
 *
 * @param   None
 * 
 * @return  pointer - pointer of the rtc value
 */
__weak uint32_t *sys_get_rtc_time(void);

/**
 * @brief   API to get sys data from mcu ram & internal flash
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - read data start address
 *
 * @param   len    - read data length
 *
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t sys_get_data(uint8_t *buffer, uint32_t addr, uint32_t len);

//gsensor get data
typedef struct {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
}gsensor_acc_t;

/**
 * @brief   API to get gsensor raw data
 *
 * @param   acc_raw - Pointer to gsensor_acc_raw data
 *
 * @return  Status  - buffer reach the thresh or not
 *			          0: SUCC, else: fail
 */
uint8_t gsensor_get_xyz(gsensor_acc_t* acc_raw);

//hrm get data
typedef struct {
    int32_t led_green;
    int32_t led_ir;
    int32_t led_amb;
    int32_t led_red;
}hrm_raw_t;
/**
 * @brief   API to get hrm raw data
 *
 * @param   hrm_raw - Pointer to gsensor_acc_raw data
 *
 * @return  Status  - get data or not
 *			          0: SUCC, else: fail
 */
uint8_t hrm_get_raw(hrm_raw_t* hrm_raw);

//oled get data
//注：如果oled不支持可从oled读取数据，此函数总是返回为fail
/**
 * @brief   API to get oled data
 *
 * @param   buffer - Pointer to data buffer
 *
 * @param   len    - data length
 *
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t oled_get_data(uint8_t* buffer, uint8_t len);

//touch get data
/*typedef struct {
    uint8_t button;           //从下往上：bit 0 1 2 3 4，按下时对应bit为置1，未按下时对应bit为0
    uint8_t button_event;     //从下往上：bit 0 1 2 3 4，变化时置为1
    uint8_t event;            //典型动作：bit0: 常速上滑， bit1: 常速上滑，bit2: 慢速下滑，bit3: 慢速下滑
    uint8_t button_event_msk; //default 0xff, mask none
    uint8_t event_msk;        //default 0xff, mask none
}touch_data_t;*/

/**
 * @brief   
 *
 * @param   touch_data - Pointer to touch_data
 *
 * @return  Status  - result of new data or event
 *			          0: success, else: fail
 */
uint8_t touch_get_new_data(touch_data_t* touch_data);

//exflash get data
/**
 * @brief   API to get exflash status
 *
 * @param   addr   - None
 *
 * @return  result - 8-bit status register contents
 */
__weak uint8_t exflash_get_status(void);

/**
 * @brief   API to get exflash data
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - read data start address
 *
 * @param   len    - read data length
 *
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t exflash_get_data(uint8_t *buffer, uint32_t addr, uint32_t len);

//nfc get data
/**
 * @brief   API to get nfc data
 *
 * @param   buffer - Pointer to data buffer
 *  
 * @param   addr   - read data start address
 *
 * @param   len    - data length
 *
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t nfc_get_data(uint8_t* buffer, uint32_t addr, uint8_t len);

//ble/em9304 get data
/**
 * @brief   API to get ble data
 *
 * @param   buffer - Pointer to data buffer
 *  
 * @param   addr   - read data start address
 *
 * @param   len    - data length
 *
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t ble_get_data(uint8_t* buffer, uint32_t addr, uint8_t len);

//motor no get data
//led no get data

/*
 * mcu and devices set data
 *******************************************************************************
 */

//battery set data
/**
 * @brief   API to set battery value
 *
 * @param   thresh：pointer to battery low threshlod
 * 
 * @return  None
 */
__weak void sys_set_battery(uint32_t* thresh);

//ntc set time
/**
 * @brief   API to set ntc value
 *
 * @param   thresh：pointer to ntc threshlod value
 * 
 * @return  None
 */
__weak void sys_set_ntc(uint32_t* thresh);

//rtc set timer
/**
 * @brief  Type definition of RTC callback function.
 */
typedef void (*rtc_cb_t)(void);
/**
 * @brief   API to rtc set timer, used for alarm
 *
 * @param   timer: which alarm timer
 * 
 * @param   second: alarm time
 *                  0: cancel alarm, else, set alarm
 * 
 * @param   cb: alarm callback function
 * 
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t sys_set_rtc_alarm(u32_t timer, u32_t second, rtc_cb_t cb);

/**
 * @brief   API to rtc timer calibration
 *
 * @param   time: standards time
 * 
 * @return  Status - get data success or not
 *			         0: success, else: fail
 */
__weak uint8_t sys_set_rtc_calibrate(u32_t time);

//watch dog
/**
 * @brief   API to feed watch dog
 *
 * @param   None
 * 
 * @return  None
 */
__weak void sys_set_wd_feed(void);

//sys set data
/**
 * @brief   API to set sys data to mcu ram
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - write data start address
 *
 * @param   len    - write data length
 *
 * @return  Status - wirte data success or not
 *			         0: success, else: fail
 */
__weak uint8_t sys_set_data(uint8_t *buffer, uint32_t addr, uint32_t len);

//gsensor no set data

//hrm no set data

//oled set data

/**
 * @brief   API to set oled range
 *
 * @param   col_start - operation oled col_start
 * 
 * @param   col_end   - operation oled col_end
 * 
 * @param   row_start - operation oled row_start
 * 
 * @param   row_end   - operation oled row_end
 *
 * @return  None
 *
 *  设定OLED显示区域,使用Partial模式
*/
__weak uint8_t oled_set_range(uint16_t col_start, uint16_t col_end, 
                              uint16_t row_start, uint16_t row_end);

/**
 * @brief   API to set data to oled
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - write data start address
 *
 * @param   len    - write data length
 *
 * @return  Status - wirte data success or not
 *			         0: success, else: fail
 *在指定的范围内写值
 */
__weak uint8_t oled_set_data(uint16_t col_start, uint16_t col_end, 
                              uint16_t row_start, uint16_t row_end,
							  uint8_t *buffer, uint32_t len);

typedef struct {
    uint8_t  cr;
    uint8_t  cg;
    uint8_t  cb;
}oled_color;


/**
 * @brief   API to oled fill rgb command
 *  
 * @param   color  - fill oled color
 *
 * @param   hold_time  - 刷新屏幕保持的时间，保持时间内，屏幕不会被刷新
 *                       单位：ms
 *                
 *
 * @return  None
 *
 */
uint8_t oled_fill_color(uint32_t color, uint32_t hold_time);

/**
 * @brief   API to oled fill rgb command
 *  
 * @param   br_percent  - brightness percent, 
 *                        0 - 100, 0: smallest, 100: most brightness
 *
 * @return  None
 *
 */
void  oled_set_brightness(uint8_t br_percent);

//touch
/*注：touch ic如果不支持写数据，此函数总是返回fail
      此函数可以通过touch的参数设定函数（例如调节灵敏度等）替换*/
/**
 * @brief   API to set data to touch ram
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - write data start address
 *
 * @param   len    - write data length
 *
 * @return  Status - wirte data success or not
 *			         0: success, else: fail
 */
__weak uint8_t touch_set_data(uint8_t *buffer, uint32_t addr, uint32_t len);

/**
 * @brief   API to updata firmware
 *
 * @param   firmware - Pointer to firmware
 *
 * @return  Status - update success or not
 *			         0: success, else: fail
 */
__weak uint8_t touch_update_firmware(uint8_t *firmware);

//exflash
/**
 * @brief   API to set data to exflash ram
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - write data start address
 *
 * @param   len    - write data length
 *
 * @return  Status - wirte data success or not
 *			         0: success, else: fail
 */
__weak uint8_t exflash_set_data(uint8_t *buffer, uint32_t addr, uint32_t len);

/**
 * @brief   API to mass_erase exflash 
 *
 * @param   None
 *
 * @return  Status - mass_erase success or not
 *			         0: success, else: fail
 */
__weak uint8_t exflash_mass_erase(void);

__weak uint8_t exflash_chip_erase(void);

/**
 * @brief   API to sector_erase exflash 
 *
 * @param   addr   - write data start sector address
 *
 * @return  Status - mass_erase success or not
 *			         0: success, else: fail
 */
__weak uint8_t exflash_sector_erase(uint32_t addr);

//nfc
/*注：nfc ic如果不支持写数据，此函数总是返回fail
      此函数可以通过nfc的参数设定函数（例如调节灵敏度等）替换*/
/**
 * @brief   API to set data to nfc ram
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - write data start address
 *
 * @param   len    - write data length
 *
 * @return  Status - wirte data success or not
 *			         0: success, else: fail
 */
__weak uint8_t nfc_set_data(uint8_t *buffer, uint32_t addr, uint32_t len);


//ble
/*注：ble ic如果不支持写数据，此函数总是返回fail
      此函数可以通过ble的参数设定函数（例如调节灵敏度等）替换*/
/**
 * @brief   API to set data to ble ram
 *
 * @param   buffer - Pointer to data buffer
 * 
 * @param   addr   - write data start address
 *
 * @param   len    - write data length
 *
 * @return  Status - wirte data success or not
 *			         0: success, else: fail
 */
__weak uint8_t ble_set_data(uint8_t *buffer, uint32_t addr, uint32_t len);

//motor
/**
 * @brief   API to set motor working para
 *  
 * @param   max_speed  - max_speed percent, 
 *                       0 - 100, 0: stop, 100: most max_speed
 * 
 * @param   time      -  running time, *ms
 *
 * @return  None
 *
 */
void  motor_set_working(uint8_t max_speed, uint32_t time);
void  motor_set_stop(void);

//led
//注：led需根据硬件调节出最佳的呼吸效果，app通过led_set_working设定led的呼吸时长
/**
 * @brief   API to set led working para
 *  
 * @param   breath      -  1: breath mode
 *                         0: allways-on mode(led is on at max_bright)
 * 
 * @param   max_bright  -  max_bright percent, default = 100
 *                         0 - 100, 0: stop, 100: most max_bright
 * 
 * @param   time        -  breath running time, *ms 
 *
 * @return  None
 *
 */
void  led_set_working(uint8_t breath, uint8_t max_bright, uint32_t time);

#endif //__APP_INTERFACE_H__
