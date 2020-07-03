/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#include "ry_type.h"
#include "app_config.h"
#include "ry_hal_inc.h"
#include "ry_utils.h"
#include "am_bsp_gpio.h"
#include "board_control.h"
#include "scheduler.h"
#include "am_util_delay.h"
#include "ry_statistics.h"

/*********************************************************************
 * CONSTANTS
 */ 
#define  INF_UART_DEBUG    					0
#define  SAMPLE_CYC     					10
#define  SAMPLE_1PRECENT 					140		//second
#define  VOLTAGE_FULL_THTESH_MINIMAL_IDX	20
#define  VOLTAGE_99_PERCENT_IDX				1
#define  BATT_HIGH_CHARGING_FULL_TIME		1200		//second

#define  MAX_PERSEN        					3
#define  JUMPER_PERSEN     					2
#define  PERCENT_INITIAL_VAL     			0xff

#define NOT_USED_MEDIAN   					0
#define SETS_TOTAL  						8

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS       15000//2000 
#define ADC_RES_12BIT                       4096
#define ADC_RES_14BIT                       16384
#define ADC_PRE_SCALING_COMPENSATION        1
#define NTC_TABLE_LEN 						(40+126)

#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS)*ADC_PRE_SCALING_COMPENSATION )/ ADC_RES_14BIT)  

#if (HW_VER_CURRENT == HW_VER_SAKE_01)

#define BATTERY_LEVEL_DETECT_SW_PIN			AM_BSP_GPIO_BAT_DET_SW
#define BATTERY_LEVEL_DETECT_ADC_CH			5
#define BATTERY_LEVEL_DETECT_ADC_PIN		33
#define BATTERY_LEVEL_DETECT_ADC_PIN_CFG	AM_HAL_PIN_33_ADCSE5

#define NTC_TEMPERATURE_DETECT_SW_PIN		AM_BSP_GPIO_NTC_EN
#define NTC_TEMPERATURE_DETECT_ADC_CH		8
#define NTC_TEMPERATURE_DETECT_ADC_PIN		AM_BSP_GPIO_NTC_ADC_IN
#define NTC_TEMPERATURE_DETECT_ADC_PIN_CFG	AM_HAL_PIN_13_ADCD0PSE8

#elif ((HW_VER_CURRENT == HW_VER_SAKE_02) || (HW_VER_CURRENT == HW_VER_SAKE_03))

#define BATTERY_LEVEL_DETECT_SW_PIN			0// not used now AM_BSP_GPIO_BAT_DET_SW
#define BATTERY_LEVEL_DETECT_ADC_CH			6
#define BATTERY_LEVEL_DETECT_ADC_PIN		34
#define BATTERY_LEVEL_DETECT_ADC_PIN_CFG	AM_HAL_PIN_34_ADCSE6

#define NTC_TEMPERATURE_DETECT_SW_PIN		AM_BSP_GPIO_NTC_EN
#define NTC_TEMPERATURE_DETECT_ADC_CH		8
#define NTC_TEMPERATURE_DETECT_ADC_PIN		AM_BSP_GPIO_NTC_ADC_IN
#define NTC_TEMPERATURE_DETECT_ADC_PIN_CFG	AM_HAL_PIN_13_ADCD0PSE8

#endif // HW_VER_CURRENT


/*********************************************************************
 * TYPEDEFS
 */
typedef struct{
	int16_t	temperature ;
	float	min_ohm;
	float 	max_ohm;
}ntc_temperature_t;

typedef enum {
    DC_STATUS_NONE = 0,   
    DC_STATUS_5V_IN = 1,
    DC_STATUS_INITIAL = 0xff,	
}dcin_status_t;

typedef enum {
    CHARGE_STATUS_NO_CHG = 0,
    CHARGE_STATUS_IS_CHG = 1,   	
    CHARGE_STATUS_INITIAL = 0xff,	
}charge_status_t;

/*********************************************************************
 * VARIABLES
 */
static adc_control_t gAdcCb = {
	.initialed = false,
	.adc_config = {
		.ui32Clock = AM_HAL_ADC_CLOCK_DIV2,
		.ui32TriggerConfig = AM_HAL_ADC_TRIGGER_SOFT,
		.ui32Reference = AM_HAL_ADC_REF_INT_1P5,//AM_HAL_ADC_REF_INT_2P0,
		.ui32ClockMode = AM_HAL_ADC_CK_LOW_POWER,
		.ui32PowerMode = AM_HAL_ADC_LPMODE_0,
		.ui32Repeat = AM_HAL_ADC_NO_REPEAT,
	},
	.adc_ch_config = {
		{
			.pinNum = BATTERY_LEVEL_DETECT_ADC_PIN,
			.pinCfg = BATTERY_LEVEL_DETECT_ADC_PIN_CFG,
			.swNum = BATTERY_LEVEL_DETECT_SW_PIN,
			.swActive = 1,
			.slotNum = 0,
			.slotCfg = AM_HAL_ADC_SLOT_AVG_1 | AM_HAL_ADC_SLOT_14BIT | AM_HAL_ADC_SLOT_CHANNEL(BATTERY_LEVEL_DETECT_ADC_CH) | AM_HAL_ADC_SLOT_ENABLE,	
			.status = ADC_STATUS_RESET,
		},
		{
			.pinNum = NTC_TEMPERATURE_DETECT_ADC_PIN,
			.pinCfg = NTC_TEMPERATURE_DETECT_ADC_PIN_CFG,
			.swNum = NTC_TEMPERATURE_DETECT_SW_PIN,
			.swActive = 0,
			.slotNum = 1,
			.slotCfg = AM_HAL_ADC_SLOT_AVG_1 | AM_HAL_ADC_SLOT_14BIT | AM_HAL_ADC_SLOT_CHANNEL(NTC_TEMPERATURE_DETECT_ADC_CH) | AM_HAL_ADC_SLOT_ENABLE,
			.status = ADC_STATUS_RESET,
		},
    },
};

ryos_mutex_t* adc_mutex;
uint8_t is_adc_initialized;
uint8_t index_last;
uint8_t batt_precent = 0xff;
uint8_t batt_last_word;

uint8_t dc_state;
int 	vtl_time;

uint16_t bat_sample_sets[SETS_TOTAL];
uint16_t bat_sets_count;
uint16_t bat_sets_rear;

uint16_t ntc_sample_sets[SETS_TOTAL];
uint16_t ntc_sets_count;
uint16_t ntc_sets_rear;


const uint16_t dischargeCurve[] = {    //VDL		//CEL(0% = 3509), (CEL), VDL(%0 = 35342)
	42668, 42482, 42331, 42187,	42063, 			    /*42792, 42633, 42503, 42373, 42261,*/ 
	41942, 41827, 41719, 41601,	41486, //90%   	    /*42140, 42038, 41936, 41815, 41703,*/  1182
                                                
	41381, 41272, 41173, 41068, 40966,          	/*41601, 41499, 41387, 41285, 41183,*/
	40863, 40764, 40665, 40556,	40460, //80%   	    /*41071, 40969, 40866, 40764, 40652,*/  1026
                                              	
	40367, 40287, 40209, 40125, 40032,         	    /*40550, 40448, 40336, 40252, 40169,*/
	39915, 39778, 39642, 39505, 39388, //70%   	    /*40094, 40011, 39890, 39750, 39583,*/  1072
                                                
	39291, 39205, 39133, 39065, 39003,          	/*39443, 39332, 39229, 39146, 39081,*/
	38941, 38870, 38798, 38727,	38656, //60%   	    /*39025, 38978, 38922, 38839, 38774,*/   732
                                                
	38581, 38513, 38445, 38377, 38315,          	/*38709, 38643, 38569, 38513, 38430,*/
	38253, 38194, 38132, 38079,	38023, //50%	  	/*38374, 38309, 38253, 38188, 38141,*/   633
                                              	
	37971, 37924, 37872, 37825, 37782,          	/*38076, 38020, 37974, 37918, 37872,*/
	37735, 37695, 37651, 37611,	37574, //40%   	    /*37825, 37779, 37732, 37676, 37639,*/   449
	
	37540, 37500, 37465, 37428, 37397,          	/*37593, 37555, 37509, 37472, 37434,*/
	37366, 37335, 37307, 37279,	37255, //30%   	    /*37397, 37360, 37313, 37276, 37248,*/   319
                                                
	37233, 37208, 37183, 37158, 37134,          	/*37211, 37174, 37146, 37109, 37081,*/
	37106, 37078, 37047, 37019, 36985, //20%   	    /*37053, 37025, 36988, 36951, 36914,*/   270
                                              	
	36948, 36914, 36867, 36827, 36777,           	/*36886, 36848, 36811, 36765, 36728,*/
	36724, 36669, 36607, 36532, 36461, //10%   	    /*36681, 36635, 36579, 36532, 36486,*/   524
                                              	
	36383, 36303, 36216, 36126, 36030,          	/*36449, 36402, 36356, 36300, 36225,*/
    35937, 35832, 35708, 35549, 35342, //0%  	    /*36142, 36002, 35797, 35509, 35081,*/  1119
};

const uint16_t chargeCure[] = {         //VDL		//CEL(99%恒压充=43483), VDL(99%恒压充=43498)
	45000, 43058, 43009, 42968, 42962,  	       /*45000, 42661, 42503, 42429, 42345,*/
	42912, 42841, 42726, 42609, 42494,  //90%      /*42252, 42131, 42029, 41936, 41843,*/
                                                 
	42376, 42243, 42116, 41985, 41861,             /*41768, 41685, 41601, 41536, 41471,*/
	41734, 41635, 41524, 41434, 41347,  //80%      /*41406, 41341, 41285, 41238, 41183,*/
                                                 
	41266, 41204, 41148, 41099, 41043,             /*41127, 41071, 41015, 40950, 40904,*/
	40997, 40944, 40897, 40848, 40798,  //70%      /*40848, 40792, 40755, 40680, 40625,*/
                                                 
	40758, 40708, 40665, 40615, 40566,             /*40578, 40522, 40485, 40438, 40384,*/
	40525, 40476, 40432, 40389, 40349,  //60%      /*40336, 40290, 40252, 40206, 40159,*/
                                                 
	40308, 40262, 40225, 40184, 40147,             /*40122, 40076, 40039, 40011, 39983,*/
	40104, 40066, 40029, 39998, 39961,  //50%	   /*39927, 39890, 39862, 39825, 39797,*/
                                                 
	39924, 39896, 39859, 39825, 39794,             /*39769, 39732, 39704, 39676, 39648,*/
	39760, 39732, 39698, 39667, 39636,  //40%      /*39620, 39583, 39555, 39536, 39508,*/
                                                 
    39608, 39583, 39552, 39524, 39490,             /*39490, 39462, 39443, 39415, 39397,*/
	39456, 39425, 39381, 39350, 39304,  //30%      /*39378, 39350, 39332, 39304, 39285,*/
                                                 
    39267, 39220, 39211, 39171, 39136,             /*39267, 39248, 39211, 39183, 39164,*/
	39096, 39050, 39000, 38957, 38901,  //20%      /*39127, 39109, 39081, 39053, 39015,*/
                                               
	38848, 38795, 38743, 38687, 38634,             /*38978, 38941, 38904, 38857, 38802,*/
	38585, 38529, 38485, 38433, 38392,  //10%      /*38755, 38709, 38653, 38606, 38550,*/
                                                 
	38364, 38340, 38324, 38284, 38151,             /*38504, 38439, 38383, 38318, 38253,*/
	37865, 37484, 37050, 36418, 35593,  //0%       /*38178, 38030, 37704, 37248, 36495,*/
};



/******************************************************************************
 * LOCAL FUNCTIONS
 */

//*****************************************************************************
//
// Interrupt handler for the ADC.
//
//*****************************************************************************
void am_adc_isr(void)
{
    uint32_t ui32Status, ui32FifoData;
	uint8_t ui8Slot;

    //
    // Read the interrupt status.
    //
    ui32Status = am_hal_adc_int_status_get(true);

    //
    // Clear the ADC interrupt.
    //
    am_hal_adc_int_clear(ui32Status);

    if (ui32Status & AM_HAL_ADC_INT_CNVCMP)
    {
		do
		{
			//
			// Read the value from the FIFO into the circular buffer.
			//
			ui32FifoData = am_hal_adc_fifo_pop();
			ui8Slot = AM_HAL_ADC_FIFO_SLOT(ui32FifoData);
			for(uint8_t i=0; i<ADC_IDX_MAX; i++) {
				if(ui8Slot == gAdcCb.adc_ch_config[i].slotNum) {
					gAdcCb.result[i] = AM_HAL_ADC_FIFO_SAMPLE(ui32FifoData);
					gAdcCb.adc_ch_config[i].status = ADC_STATUS_DONE;				
				}
			}
		} while (AM_HAL_ADC_FIFO_COUNT(ui32FifoData) > 1);

    }

}

/**
 * @brief   API to deinitial ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_deinit(ry_adc_t idx)
{
	 if(gAdcCb.initialed){
		am_hal_pwrctrl_periph_disable(AM_HAL_PWRCTRL_ADC);
		am_hal_adc_disable();
		am_hal_interrupt_disable(AM_HAL_INTERRUPT_ADC);
		gAdcCb.initialed=false;
	 }
	 
	if(idx < ADC_IDX_MAX) {
		am_hal_gpio_pin_config(gAdcCb.adc_ch_config[idx].pinNum, AM_HAL_GPIO_DISABLE);
		//am_hal_gpio_out_bit_replace(gAdcCb.adc_ch_config[idx].swNum, !gAdcCb.adc_ch_config[idx].swActive);	//default inactive
	}
}

/**
 * @brief   API to init specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_init(ry_adc_t idx)
{
    if (is_adc_initialized == 0) {
        ryos_mutex_create(&adc_mutex);
        is_adc_initialized = 1;
    }
    
    if (!gAdcCb.initialed) {
		gAdcCb.initialed = true;
        //
        // Enable the ADC power domain.
        //
        am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_ADC);
    	am_hal_adc_config(&gAdcCb.adc_config);
    	am_hal_adc_int_clear(AM_HAL_ADC_INT_CNVCMP);
    	am_hal_adc_int_enable(AM_HAL_ADC_INT_CNVCMP);
    	am_hal_interrupt_enable(AM_HAL_INTERRUPT_ADC);
    }

	//
    // Enable the ADC.
    //
    am_hal_adc_enable();

	//
    // Set up an ADC slot
    //
    if(idx < ADC_IDX_MAX) {
		//am_hal_gpio_pin_config(gAdcCb.adc_ch_config[idx].swNum, AM_HAL_GPIO_OUTPUT);
		//am_hal_gpio_out_bit_replace(gAdcCb.adc_ch_config[idx].swNum, gAdcCb.adc_ch_config[idx].swActive);	//default active

		am_hal_gpio_pin_config(gAdcCb.adc_ch_config[idx].pinNum, gAdcCb.adc_ch_config[idx].pinCfg);
		am_hal_adc_slot_config(gAdcCb.adc_ch_config[idx].slotNum, gAdcCb.adc_ch_config[idx].slotCfg);
		gAdcCb.adc_ch_config[idx].status = ADC_STATUS_INITIALED;
    }
}


/**
 * @brief   API to power down specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_powerdown(ry_adc_t idx)
{
	if(idx >= ADC_IDX_MAX)
		return;
	// am_hal_gpio_out_bit_replace(gAdcCb.adc_ch_config[idx].swNum, !gAdcCb.adc_ch_config[idx].swActive);
}

/**
 * @brief   API to power up specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_powerup(ry_adc_t idx)
{
	if(idx >= ADC_IDX_MAX)
		return;
	// am_hal_gpio_out_bit_replace(gAdcCb.adc_ch_config[idx].swNum, gAdcCb.adc_ch_config[idx].swActive);
}

/**
 * @brief   API to start specified ADC module
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  None
 */
void ry_hal_adc_start(ry_adc_t idx)
{
	if(idx >= ADC_IDX_MAX)
		return;
	
	// if (ry_system_initial_status() != 0)	
    // ryos_mutex_lock(adc_mutex);
	gAdcCb.result[idx] = 0;
	gAdcCb.adc_ch_config[idx].status = ADC_STATUS_SAMPLING;
	if (ry_system_initial_status() != 0)	
    	ryos_delay_ms(10);
	else
        am_util_delay_ms(5);
		
	am_hal_adc_trigger();
	
	// if (ry_system_initial_status() != 0)	
	// 	ryos_mutex_unlock(adc_mutex);
}

/**
 * @brief   API to get sp
 * ecified ADC value
 *
 * @param   idx  - The specified ADC instance
 *
 * @return  Value of ADC sampling
 */
u32_t ry_hal_adc_get(ry_adc_t idx)
{
	static int wait_cnt = 0;
	if(idx >= ADC_IDX_MAX)
		return 0xFFFFFFFF;
	
	while(gAdcCb.adc_ch_config[idx].status != ADC_STATUS_DONE) {
        am_util_delay_ms(2);
        if (wait_cnt++ == 5) {
            wait_cnt = 0;
            break;
        }
    }
    wait_cnt = 0;
    return gAdcCb.result[idx];
}

uint8_t battery_charge_precent(uint16_t batt_volt)
{
	uint8_t index = 0;
  uint32_t voltage_is_high = 0;
	static int volt_high_charging;
	vtl_time ++;
	
	//find the the max_percent index from 100% to 1%
	for (index = 0; index < 100; index ++){
		if (batt_volt >= chargeCure[index]) {
			break;
		}
	}
	
	//get the last_index for post-processing
	if (batt_precent != PERCENT_INITIAL_VAL)
		index_last = 100 - batt_precent;		//get the last_index from charging or un-charging
	else {
		index_last = index;
		goto exit;
	}
	
	//post processing non-variable charging
	voltage_is_high = 0;
	voltage_is_high = batt_volt >= dischargeCurve[VOLTAGE_99_PERCENT_IDX];
	if (voltage_is_high){
		volt_high_charging ++;
	}
	else {
		volt_high_charging = 0;
	}

	if (volt_high_charging > (BATT_HIGH_CHARGING_FULL_TIME / 10)){
		if ((index_last == 1) && (batt_volt >= dischargeCurve[0])){
			index = 0;			//set the full-charging index
		}
	}

#if 0	
	if(index == index_last){
		if ((vtl_time * SAMPLE_CYC) > SAMPLE_1PRECENT){
			if(batt_volt >= chargeCure[VOLTAGE_FULL_THTESH_MINIMAL_IDX]){
				if(index > 0) index--;
				vtl_time = 0;
			}
		}
	}
	else{
		vtl_time = 0;
		if(index>index_last+MAX_PERSEN)  {
			index=index_last+JUMPER_PERSEN;
		}
		else if(index<index_last-MAX_PERSEN)
		{
			index=index_last-JUMPER_PERSEN;
		}
	}
#endif

exit:	
	//LOG_DEBUG("charge_percetn_map: voltg:%d, index:%d, index_last:%d, percent:%d\r\n", \
			batt_volt, index, index_last, 100 - index);
	return 100 - index;
}

uint8_t battery_to_precentmap(uint16_t batt_volt)
{
	uint8_t index = 0;

	//get the percent index
	for (index = 0; index < 100; index ++)  {
		if (dischargeCurve[index]<=(batt_volt)){
			break;
		}
	}
	
	//get the last percent index
	if (batt_precent != 0xff)
		index_last = 100 - batt_precent;
	else {
		index_last = index;
		goto exit;
	}

	//max changed index max value filter
	if (index_last != 0){
		if (index > index_last + MAX_PERSEN)  {
			index = index_last + JUMPER_PERSEN;
		}
		else if (index < index_last - MAX_PERSEN){
			index = index_last - JUMPER_PERSEN;
		}
	}	
exit:	
	//LOG_DEBUG("un-charge_percetn_map: voltg:%d, index:%d, index_last:%d, percent:%d\r\n", \
				batt_volt, index, index_last, 100 - index);
	return 100 - index;
}

void push_bat_sets(uint16_t sample)
{
	bat_sample_sets[bat_sets_rear] = sample;
	bat_sets_rear++;
	if(bat_sets_rear>=SETS_TOTAL) bat_sets_rear=0;
	if(bat_sets_count<SETS_TOTAL)
	bat_sets_count++;
}

void push_ntc_sets(uint16_t sample)
{
	ntc_sample_sets[ntc_sets_rear] = sample;
	ntc_sets_rear++;
	if(ntc_sets_rear>=SETS_TOTAL) ntc_sets_rear=0;
	if(ntc_sets_count<SETS_TOTAL)
	ntc_sets_count++;
	
}

uint16_t GetMedianNum(uint16_t *sample_sets,uint16_t sets_count)  
{  
    int i,j;
    uint16_t bTemp;  
	uint16_t  bArray[SETS_TOTAL];
	
	//push_sample_sets(sample);
	ry_memcpy((uint8_t*)bArray, (uint8_t*)sample_sets, sets_count*2);
     
    for (j = 0; j < sets_count - 1; j ++)  
    {  
        for (i = 0; i < sets_count - j - 1; i ++)  
        {  
            if (bArray[i] > bArray[i + 1])  
            {  
                bTemp = bArray[i];  
                bArray[i] = bArray[i + 1];  
                bArray[i + 1] = bTemp;  
            }  
        }  
    }  
      
    if ((sets_count & 1) > 0)  
    {  
        bTemp = bArray[(sets_count ) / 2];  
    }  
    else  
    {  
        bTemp = (bArray[sets_count / 2-1] + bArray[sets_count / 2 ]) / 2;  
    }  
  
    return bTemp;  
}  

/**
 * @brief   API to get battery value
 *
 * @param   detect_mode: 0 - auto detect, the tmp_percent is the true value of the last detection
 *                1 - directed detect, charging -> value up, un-charging -> down
 * 				  
 * @return  batterypercent value
 */
uint16_t adc_result;
uint16_t tmp_adc_result = 0;

uint8_t sys_get_battery_percent(uint32_t detect_mode)
{	
	uint32_t millivolt;
	uint8_t tmp_percent;
	uint8_t charge_state = CHARGE_STATUS_NO_CHG;

	ry_hal_adc_init(ADC_IDX_BATTERY);

	if (ry_system_initial_status() != 0)	
    	ryos_mutex_lock(adc_mutex);
    	
	ry_hal_adc_start(ADC_IDX_BATTERY);
	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_STA_DET, AM_HAL_GPIO_INPUT|AM_HAL_GPIO_PULL24K);
	// am_util_delay_ms(10);
	adc_result = ry_hal_adc_get(ADC_IDX_BATTERY);
	
	//sample again to fix adc error if the adc result is less than 3.29v
	if(adc_result <= 12000){
		ry_hal_adc_start(ADC_IDX_BATTERY);
		adc_result = ry_hal_adc_get(ADC_IDX_BATTERY);
	}
	ry_hal_adc_deinit(ADC_IDX_BATTERY);
	
	tmp_adc_result = adc_result;
	push_bat_sets(adc_result);
	adc_result = GetMedianNum(bat_sample_sets, bat_sets_count);
	millivolt = ADC_RESULT_IN_MILLI_VOLTS(adc_result) * 3; 			//scale up to battery level

	//detect dc-in gpio and charge status
	if(am_hal_gpio_input_bit_read(AM_BSP_GPIO_DC_IN) != 0){
		dc_state = DC_STATUS_5V_IN;	
		if(am_hal_gpio_input_bit_read(AM_BSP_GPIO_CHG_STA_DET) == 0){
			charge_state = CHARGE_STATUS_IS_CHG;
		}
	}
	else{
		if (dc_state == DC_STATUS_5V_IN){
			bat_sets_count = 0;
			bat_sets_rear = 0;
		}
		dc_state = DC_STATUS_NONE;
	}

	//volt to tmp_percent map
	if(dc_state == DC_STATUS_5V_IN){
		tmp_percent = battery_charge_precent(millivolt);
		if(batt_precent == PERCENT_INITIAL_VAL){
			batt_precent = tmp_percent;
		}
		else if(tmp_percent < batt_precent){
			tmp_percent = batt_precent;
		}
		if (tmp_percent > (batt_precent + 1)){
			tmp_percent = batt_precent + 1;
		}
	}	
	else{
		vtl_time = 0;
		tmp_percent = battery_to_precentmap(millivolt);
		if(batt_precent == PERCENT_INITIAL_VAL){
			batt_precent = tmp_percent;
		}
		else if((tmp_percent > batt_precent) && (detect_mode == BATT_DETECT_DIRECTED)){
			tmp_percent = batt_precent;
		}
		if (tmp_percent < (batt_precent - 1)){
			tmp_percent = batt_precent - 1;
		}
	}
#if 0	
	//tmp_percent adjust by dc-state and charge_state
	static uint8_t charge_full_cnt;
	if ((dc_state == DC_STATUS_5V_IN) && (charge_state == CHARGE_STATUS_NO_CHG) \
		&& ((millivolt >= chargeCure[VOLTAGE_FULL_THTESH_MINIMAL_IDX]) && (batt_precent == tmp_percent))
	   ){
		if (charge_full_cnt <= 5){
			charge_full_cnt ++;
		}
		if (charge_full_cnt >= 5){
			tmp_percent = (batt_precent < 100) ? (batt_precent + 1) : 100;
			batt_precent = tmp_percent;
			charge_full_cnt = 0;
		}
	}
	else{
		charge_full_cnt = 0;
	}
#endif

	if ((detect_mode == BATT_DETECT_DIRECTED) && ((batt_last_word & 0x7f) <= 100) &&((batt_last_word & 0x7f) != 0)){
		LOG_INFO("batt_last:%d, DC:%d, isChg:%d, adcTp:%d, adc:%d, bSet:%d, Volt:%d, Pct:%d\r\n", \
			batt_last_word, dc_state, charge_state, tmp_adc_result, adc_result, bat_sets_count, millivolt, batt_precent);
		batt_precent = batt_last_word & 0x7f;	//recove last reset value			
		batt_last_word = 0xff;
	}
	else{
		batt_precent =  tmp_percent;
	}
	
	if (batt_precent > 100){
		batt_precent = 100;
		DEV_STATISTICS_SET_VALUE(charge_full, ryos_rtc_now());
	}

#if 0
	if (ry_system_initial_status() != 0)	{
		LOG_DEBUG("batt_last_word:%d, DC:%d, isChg:%d, adcTp:%d, adc:%d, bSet:%d, Volt:%d, Pct:%d\r\n", \
			batt_last_word, dc_state, charge_state, tmp_adc_result, adc_result, bat_sets_count, millivolt, batt_precent);
	}
#endif

	am_hal_gpio_pin_config(AM_BSP_GPIO_CHG_STA_DET, AM_HAL_GPIO_DISABLE);
	
	if (ry_system_initial_status() != 0)	
    	ryos_mutex_unlock(adc_mutex);
		
#if INF_UART_DEBUG
	static uint8_t uart_mode_enable = 0;
	uint8_t bat[80]; 	
	if (uart_mode_enable){
		sprintf(bat,"%02d:%02d:%02d - Volt:%d, Pct:%d, Chg:%d, DC:%d, adcTp:%d, adc:%d, bSet:%d\r\n", \
			hal_time.ui32Hour, hal_time.ui32Minute, hal_time.ui32Second, \
			millivolt, batt_precent, charge_state, dc_state, tmp_adc_result, adc_result, bat_sets_count);
		ry_hal_uart_tx(UART_IDX_TOOL,  bat, strlen(bat));
	}
#endif
	return batt_precent;
}

uint16_t sys_get_battery_adc(void)
{
	return ADC_RESULT_IN_MILLI_VOLTS(adc_result) * 3;
}

u8_t batt_percent_get(void)
{
	u8_t batt_v = batt_precent | (charge_cable_is_input() << 7);
	return batt_v;
}


void batt_percent_recover(u8_t vatt_v)
{
	batt_last_word = vatt_v;
}

static const ntc_temperature_t ntc_temp_table[]={
	{-40,4053.1340,4457.0340},
#if 0	
	{-39,3772.2764,4142.1750},
	{-38,3512.9264,3851.8471},	
	{-37,3273.2891,3583.9708},	
	{-36,3051.7313,3336.6564},	
	{-35,2846.7657,3108.1855},	
	{-34,2657.0372,2896.9947},	
	{-33,2481.3097,2701.6601},	
	{-32,2318.4558,2520.8845},	
	{-31,2167.4459,2353.4852},	
	{-30,2027.3393,2198.3830},	
	{-29,1897.2761,2054.5926},	
	{-28,1776.4697,1921.2140},	
	{-27,1664.1999,1797.4246},	
	{-26,1559.8073,1682.4717},	
	{-25,1462.6875,1575.6666},	
	{-24,1372.2862,1476.3786},
	{-23,1288.0948,1384.0296},
	{-22,1209.6464,1298.0896},
	{-21,1136.5121,1218.0725},	
	{-20,1068.2977,1143.5319},	
	{-19,1004.6405,1074.0579},	
	{-18,945.2070,1009.2736},	
	{-17,889.6900,948.8326},	
	{-16,837.8065,892.4162},	
	{-15,789.2957,839.7309},	
	{-14,743.9171,790.5065},	
	{-13,701.4488,744.4939},	
	{-12,661.6858,701.4634},	
	{-11,624.4390,661.6858},//{-11,624.4390,661.2031},	
	{-10,589.5336,624.4390},//{-10,589.5336,623.5176},
	{-9,556.8078,589.5336},//{-9,556.8078,588.2262},	
	{-8,526.1120,556.8078},//{-8,526.1120,555.1621},	
	{-7,497.3079,526.1120},//{-7,497.3079,524.1710},		
	{-6,470.2673,497.3079},//{-6,470.2673,495.1102},
	{-5,444.8715,470.2673},//{-5,444.8715,467.8478},
	{-4,421.0104,444.8715},//{-4,421.0104,442.2615},	
	{-3,398.5819,421.0104},//{-3,398.5819,418.2381},	
	{-2,377.4915,398.5819},//{-2,377.4915,395.6728},	
	{-1,357.6511,377.4915},//{-1,357.6511,374.4682},	
	{0,338.9792,357.6511},//{0,338.9792,354.5341},	
	{1,321.4001,338.9792},//{1,321.4001,335.7869},	
	{2,304.8432,321.4001},//{2,304.8432,318.1488},	
	{3,289.2431,304.8432},//{3,289.2431,301.5476},	
	{4,274.5388,289.2431},//{4,274.5388,285.9162},	
	{5,260.6735,274.5388},//{5,260.6735,271.1923},	
	{6,247.5946,260.6735},//{6,247.5946,257.3178},	
	{7,235.2527,247.5946},//{7,235.2527,244.2388},	
	{8,223.6020,235.2527},//{8,223.6020,231.9050},	
	{9,212.5999,223.6020},//{9,212.5999,220.2697},	
	{10,202.2064,212.5999},//{10,202.2064,209.2893},		
	{11,192.3844,202.2064},//{11,192.3844,198.9230},
	{12,183.0992,192.3844},//{12,183.0992,189.1331},
	{13,174.3183,183.0992},//{13,174.3183,179.8842},	
	{14,166.0115,174.3183},//{14,166.0115,171.1433},		
	{15,158.1505,166.0115},//{15,158.1505,162.8796},	
	{16,150.7089,158.1505},//{16,150.7089,155.0643},	
	{17,143.6618,150.7089},//{17,143.6618,147.6707},	
	{18,136.9862,143.6618},//{18,136.9862,140.6735},	
	{19,130.6603,136.9862},//{19,130.6603,134.0493},	
	{20,124.6640,130.6603},//{20,124.6640,127.7761},	
	{21,118.9781,124.6640},//{21,118.9781,121.8333},	
	{22,113.5850,118.9781},//{22,113.5850,116.2018},	
	{23,108.4679,113.5850},//{23,108.4679,110.8635},	
	{24,103.6111,108.4679},//{24,103.6111,105.8015},	
	{25,99.0000,103.6111},//{25,99.0000,101.0000},	
	{26,94.5344,99.0000},//{26,94.5344,96.5323},	
	{27,90.2959,94.5344},//{27,90.2959,92.2881},		
	{28,86.2718,90.2959},//{28,86.2718,88.2548},
	{29,82.4501,86.2718},//{29,82.4501,84.4210},
	{30,78.8194,82.4501},//{30,78.8194,80.7755},	
	{31,75.3693,78.8194},//{31,75.3693,77.3083},	
	{32,72.0897,75.3693},//{32,72.0897,74.0095},	
	{33,68.9714,72.0897},//{33,68.9714,70.8702},	
	{34,66.0055,68.9714},//{34,66.0055,67.8817},	
	{35,63.1838,66.0055},//{35,63.1838,65.0360},	
	{36,60.4984,63.1838},//{36,60.4984,62.3255},	
	{37,57.9422,60.4984},//{37,57.9422,59.7431},	
	{38,55.5081,57.9422},//{38,55.5081,57.2821},	
	{39,53.1898,55.5081},//{39,53.1898,54.9360},	
	{40,50.9811,53.1898},//{40,50.9811,52.6991},	
	{41,48.8763,50.9811},//{41,48.8763,50.5655},	
	{42,46.8698,48.8763},//{42,46.8698,48.5300},	
	{43,44.9567,46.8698},//{43,44.9567,46.5876},	
	{44,43.1321,44.9567},//{44,43.1321,44.7335},		
	{45,41.3915,43.1321},//{45,41.3915,42.9632},
	{46,39.7305,41.3915},//{46,39.7305,41.2726},
	{47,38.1451,39.7305},//{47,38.1451,39.6577},	
	{48,36.6315,38.1451},//{48,36.6315,38.1146},	
	{49,35.1861,36.6398},	
	{50,33.8054,35.2300},	
	{51,32.4862,33.8819},	
	{52,31.2255,32.5926},	
	{53,30.0204,31.3592},	
	{54,28.8681,30.1789},	
	{55,27.7662,29.0493},	
	{56,26.7120,27.9678},	
	{57,25.7034,26.9323},	
	{58,24.7381,25.9405},	
	{59,23.8140,24.9904},	
	{60,22.9293,24.0800},	
	{61,22.0820,23.2074},	
	{62,21.2703,22.3710},	
	{63,20.4926,21.5689},	
	{64,19.7473,20.7998},	
	{65,19.0330,20.0619},		
	{66,18.3480,19.3540},
	{67,17.6912,18.6747},
	{68,17.0612,18.0226},	
	{69,16.4568,17.3966},	
	{70,15.8768,16.7954},	
	{71,15.3202,16.2180},	
	{72,14.7859,15.6634},	
	{73,14.2728,15.1304},	
	{74,13.7801,14.6183},	
	{75,13.3068,14.1260},	
	{76,12.8521,13.6526},	
	{77,12.4151,13.1975},	
	{78,11.9952,12.7598},	
	{79,11.5914,12.3387},	
	{80,11.2033,11.9335},	
	{81,10.8300,11.5437},	
	{82,10.4709,11.1684},
	{83,10.1255,10.8071},	
	{84,9.7931,10.4593},	
	{85,9.4732,10.1243},	
	{86,9.1653,9.8016},		
	{87,8.8689,9.4908},
	{88,8.5834,9.1913},
	{89,8.3085,8.9026},	
	{90,8.0437,8.6244},	
	{91,7.7885,8.3562},	
	{92,7.5427,8.0975},	
	{93,7.3057,7.8481},	
	{94,7.0773,7.6075},	
	{95,6.8571,7.3754},	
	{96,6.6448,7.1515},	
	{97,6.4400,6.9354},	
	{98,6.2424,6.7268},	
	{99,6.0519,6.5254},	
	{100,5.8680,6.3310},	
	{101,5.6905,6.1433},	
	{102,5.5192,5.9620},	
	{103,5.3538,5.7868},
	{104,5.1942,5.6176},	
	{105,5.0400,5.4541},	
	{106,4.8911,5.2961},	
	{107,4.7472,5.1434},		
	{108,4.6082,4.9957},
	{109,4.4739,4.8530},
	{110,4.3441,4.7149},	
	{111,4.2187,4.5814},	
	{112,4.0974,4.4523},	
	{113,3.9801,4.3273},	
	{114,3.8667,4.2065},	
	{115,3.7571,4.0895},	
	{116,3.6510,3.9763},	
	{117,3.5484,3.8667},	
	{118,3.4491,3.7606},	
	{119,3.3530,3.6579},	
	{120,3.2600,3.5584},	
	{121,3.1699,3.4620},	
	{122,3.0828,3.3687},	
	{123,2.9984,3.2783},	
	{124,2.9167,3.1907},
	{125,2.8375,3.1058},
#endif		
};

/**
 * @brief   API to get ntc value
 *
 * @param   None
 * 
 * @return  pointer - pointer of the ntc value
 */
int16_t sys_get_ntc_temperature(void)
{
	uint16_t adc;
	int16_t temperature;
	float ohm;
	uint32_t voltage;
	
	ry_hal_adc_init(ADC_IDX_NTC);
	am_util_delay_ms(40); 
	ry_hal_adc_start(ADC_IDX_NTC);
	am_util_delay_ms(10); 
	adc = ry_hal_adc_get(ADC_IDX_NTC);
	
	//ry_hal_adc_start(ADC_IDX_NTC);
	//am_util_delay_ms(1000); 
	//adc = ry_hal_adc_get(ADC_IDX_NTC);
	voltage = ADC_RESULT_IN_MILLI_VOLTS(adc);
	ry_hal_adc_deinit(ADC_IDX_NTC);
	//LOG_DEBUG("NTC adc=%d,v=%d\n", adc,voltage);

	/*
		V = ohm/(ohm+100)*1.8 = (adc/16384)*2
		=> ohm*1.8=adc/8192*(ohm+100)=adc/8192*ohm+adc/8192*100
		=> ohm*(1.8-adc/8192)=adc/8192*100
		=> ohm=(adc/8192*100)/(1.8-adc/8192)
	
	for test:
		V=0.1019,adc=834  	=> ohm=6.0,t=100
		V=0.0775,adc=634  	=> ohm=4.5,t=110	
		V=0.0592,adc=485  	=> ohm=3.4,t=120
		V=1.7182,adc=14075  => ohm=2100,t=-30
		V=1.65,adc=13517  	=> ohm=1100,t=-20
	*/
#if NOT_USED_MEDIAN	
	//ohm=((float)adc*200/8192)/(1.8-(float)adc/8192);
	//ohm=((float)adc*200*1.5/16384)/(1.8-(float)adc*1.5/16384);
	ohm= 200.0x5*adc/(16384*6 -5*adc);
	
	//ohm=(adc*100.0)/(1.8*8192-adc);
	temperature=0;
	for(int i=0;i<NTC_TABLE_LEN;i++)
	{
			if(ntc_temp_table[i].min_ohm<=ohm && ntc_temp_table[i].max_ohm>=ohm)
			{
					temperature=ntc_temp_table[i].temperature;
					break;
			}
	}
	
	//voltage=adc*2000/16384;
	
	LOG_DEBUG("NTC adc=%d,v=%d,ohm=%f,temperature=%d\n", adc,voltage,ohm,temperature);
#else	
	
	push_ntc_sets(adc);
	adc=GetMedianNum(ntc_sample_sets,ntc_sets_count);
	
	//ohm=((float)adc*200/8192)/(1.8-(float)adc/8192);
	//ohm=((float)adc*200*1.5/16384)/(1.8-(float)adc*1.5/16384);
	//ohm=(adc*100.0)/(1.8*8192-adc);
	ohm= 200.0*5*adc/(16384*6 -5*adc);
	temperature=0;
	for(int i=0;i<NTC_TABLE_LEN;i++)
	{
			if(ntc_temp_table[i].min_ohm<=ohm && ntc_temp_table[i].max_ohm>=ohm)
			{
					temperature=ntc_temp_table[i].temperature;
					break;
			}
	}
	
	//voltage=adc*2000/16384;
	
	LOG_DEBUG(" Median NTC adc=%d,v=%d,ohm=%f,temperature=%d\n", adc,voltage,ohm,temperature);
#endif
	return temperature;
}
