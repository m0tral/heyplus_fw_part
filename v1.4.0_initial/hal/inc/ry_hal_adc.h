/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_HAL_ADC_H__
#define __RY_HAL_ADC_H__

/*********************************************************************
 * INCLUDES
 */
#include "ry_type.h"
#include "ry_hal_inc.h"

/*
 * CONSTANTS
 *******************************************************************************
 */


/*
 * TYPES
 *******************************************************************************
 */

typedef enum {
    ADC_IDX_BATTERY = 0x00,
	ADC_IDX_NTC,
    ADC_IDX_MAX
} ry_adc_t;

typedef enum {
	ADC_STATUS_RESET = 0x00,
	ADC_STATUS_INITIALED,
	ADC_STATUS_SAMPLING,
	ADC_STATUS_DONE
} ry_adc_status_t;


typedef enum {
    BATT_DETECT_AUTO = 0x00,
	BATT_DETECT_DIRECTED,
} battery_detect_mode_t;

typedef struct adc_ch_config_s{
	uint8_t pinNum;
	uint32_t pinCfg;
	uint8_t swNum;
	uint8_t	swActive;
	uint8_t slotNum;
	uint32_t slotCfg;
	ry_adc_status_t status;
} adc_ch_config_t;

#ifndef ryos_mutex_t
typedef void ryos_mutex_t;
#endif


typedef struct adc_control_s{
	bool initialed;
	ryos_mutex_t *adc_mutex;
	am_hal_adc_config_t adc_config;
	adc_ch_config_t adc_ch_config[ADC_IDX_MAX];
	uint16_t result[ADC_IDX_MAX];
} adc_control_t;

/*
 * FUNCTIONS
 *******************************************************************************
 */


/**
 * @brief   API to init specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_init(ry_adc_t idx);

/**
 * @brief   API to power down specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_powerdown(ry_adc_t idx);

/**
 * @brief   API to power up specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_powerup(ry_adc_t idx);

/**
 * @brief   API to start specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_start(ry_adc_t idx);

/**
 * @brief   API to get specified ADC value
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  Value of ADC sampling
 */
u32_t ry_hal_adc_get(ry_adc_t idx);

/**
 * @brief   API to get battery value
 *
 * @param   detect_mode: 0 - auto detect, the percent is the true value of the last detection
 *                1 - directed detect, charging -> value up, un-charging -> down
 * 				  
 * @return  batterypercent value
 */
uint8_t sys_get_battery_percent(uint32_t detect_mode);

/**
 * @brief   API to get latest battery pecent value
 *
 * @param   None
 *
 * @return  latest battery pecent value & charge status
 */
u8_t batt_percent_get(void);

/**
 * @brief   API to set batt_percent_recover
 *
 * @param   vatt_v - battery last word
 *
 * @return  None
 */
void batt_percent_recover(u8_t vatt_v);

/**
 * @brief   API to get battery adc value
 *
 * @param   None
 * 				  
 * @return  battery adc filted value
 */
uint16_t sys_get_battery_adc(void);

int16_t sys_get_ntc_temperature(void);

#endif  /* __RY_HAL_ADC_H__ */
