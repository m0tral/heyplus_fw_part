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

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "am_bsp_gpio.h"
#include "am_hal_gpio.h"

#include "ry_hal_inc.h"
#include "ry_utils.h"
#include "am_devices_CY8C4014.h"
#include "ryos.h"

uint8_t get_key_state(void);
uint8_t am_devices_cy8c4014_get_button(void);

/*********************************************************************
 * CONSTANTS
 */
#define KEY_CODES_SIZE 		16

/*
 * Global variables
 *******************************************************************************
 */
uint8_t key_code;
uint8_t key_codes[KEY_CODES_SIZE];
uint8_t key_codes_in = 0;
uint8_t key_codes_out = 0; 
touch_data_t g_touch_data = {0, 0, 0, 0x1f, 0x0f};
void am_devices_cy8c4014_reg_write(uint8_t ui8Register, uint8_t ui8Value);

/**
 * @brief   
 *
 * @param   touch_data - Pointer to touch_data
 *
 * @return  Status  - result of new data or event
 *			          0: no new data, else: new data
 */
unsigned char gtouch_new_data(touch_data_t* touch_data){
    unsigned char ret = 1;
	ry_memcpy((uint8_t*)touch_data, (uint8_t*)&g_touch_data, sizeof(g_touch_data));
	if ((touch_data->button_event & touch_data->button_event_msk) || (touch_data->event & touch_data->event_msk)){
        ret = 0;
    }
    return ret;
}

void push_key_code(uint8_t keycode)
{
	uint8_t tmp_index;
	tmp_index = key_codes_in;
	key_codes_in ++;
	if (key_codes_in >= KEY_CODES_SIZE){
		key_codes_in = 0;	
	}

	if (key_codes_in == key_codes_out){
		key_codes_out ++;
		if (key_codes_out >= KEY_CODES_SIZE)
			key_codes_out = 0;
	}
	key_codes[tmp_index] = keycode;
}

uint8_t pop_key_code(void)
{
	uint8_t tmp_code = 0;
	if (key_codes_in != key_codes_out){
		tmp_code = key_codes[key_codes_out];
		key_codes_out ++;
		if (key_codes_out >= KEY_CODES_SIZE)
			key_codes_out = 0;
	}
	return tmp_code;
}




void cy8c4014_handler(void)
{
	key_codes_out = 0;
	key_codes_in = 0;
	key_code = am_devices_cy8c4014_get_button();
	push_key_code(key_code);
	LOG_DEBUG("cy8c4014_handler %d \r\n",key_code);
}

void cy8c4014_timer_handler(void)
{
	if(am_hal_gpio_input_bit_read(AM_BSP_GPIO_TP_INT)){
		key_code = am_devices_cy8c4014_get_button();
		push_key_code(key_code);
		get_key_state();
        LOG_DEBUG("raw tp data: %d\r\n", key_code);
	}
	else{
		key_codes_out = 0;
		key_codes_in = 0;
		g_touch_data.button = 0;
		g_touch_data.button_event = 0;
		g_touch_data.event = 0;
	}
}

uint8_t get_button_pressed(void){
	uint8_t key_pressed;
	key_pressed = key_code;
	key_code = 0;
	return key_pressed;
}

 uint8_t get_key_state(void)
 {
 		uint8_t stable_code = 0;
 		uint8_t stable_cnt = 0;
 		uint8_t left_code = 0;
 		uint8_t left_cnt = 0;
		uint8_t left_smaple_cnt=0;
 		uint8_t right_code = 0;
		uint8_t right_cnt = 0;
		uint8_t right_smaple_cnt=0;
 		uint8_t sample_cnt  =  0;
 		uint8_t sample;
 		uint8_t bitsite = 0;
 		uint8_t sample_start = key_codes_out;
 		uint8_t sample_end = key_codes_in;
 		if (key_codes_in == key_codes_out)
 			return 0;
 			
 		do{
 			sample = key_codes[sample_start];
			left_smaple_cnt ++;
			right_smaple_cnt ++;
 			//LOG_DEBUG("S %02x \r\n",smaple);
 			if (sample == stable_code){
 				stable_cnt ++;
 			}
 			else if (stable_cnt < 8){
 				stable_cnt = 1;
 				stable_code = sample;
 			}
 			
 			if ((sample & 0xf) == 0xf){
 				bitsite = 4;
 			}
 			else if ((sample & 0x7) == 0x07){
 				bitsite = 3;
 			}
 			else if ((sample & 0x3) == 0x03){
 				bitsite = 2;
 			}
 			else if ((sample & 0x1) == 0x01){
 				bitsite = 1;
 			}
			else{
				bitsite = 0;
			}
			
 			if (left_code < bitsite){
 				left_code = bitsite;
 				left_cnt ++;
 				if (left_cnt >= 3){
					if (left_smaple_cnt > 8){
						g_touch_data.button_event = 0;
						g_touch_data.button = 0;
						g_touch_data.event = 8;
					}
					else{
						g_touch_data.button_event = 0;
						g_touch_data.button = 0;
						g_touch_data.event = 2;
					}
					return 0xfc;
				}
 			}
 			else if (left_code > bitsite){
				left_code = bitsite;
 				left_cnt = 1;
				left_smaple_cnt = 1;
 			}
 			
			if ((sample & 0x0f) == 0x0f){
 				bitsite = 4;
 			}
 			else if ((sample & 0x0e) == 0x0e){
 				bitsite = 3;
 			}
 			else if ((sample & 0x0c) == 0x0c){
 				bitsite = 2;
 			}
 			else if ((sample & 0x08) == 0x08){
 				bitsite = 1;
 			}
			else{
				bitsite = 0;	
			}
			
 			if (right_code < bitsite){
 				right_code = bitsite;
 				right_cnt ++;
 				if (right_cnt >= 3){
					if (right_smaple_cnt > 8){
						g_touch_data.button_event = 0;
						g_touch_data.button = 0;
						g_touch_data.event = 4;
					}
					else{
						g_touch_data.button_event = 0;
						g_touch_data.button = 0;
						g_touch_data.event = 1;
					}
					return 0x3f;
				}
 			}
 			else if (right_code > bitsite){
 				right_code = bitsite;
 				right_cnt = 1;
				right_smaple_cnt = 1;
 			}
 			sample_cnt ++;
			sample_start ++;
			if (sample_start >= KEY_CODES_SIZE)
				sample_start =  0;
 		} while(sample_start != sample_end);

		if ((stable_cnt >= 8) && (g_touch_data.button != stable_code)){
			g_touch_data.button_event = g_touch_data.button ^ stable_code;
			g_touch_data.button = stable_code;
			g_touch_data.event = 0;
 			return stable_code;
		}
 		return 0;		
}

void cy8c4011_gpio_int_init(void)
{
    am_hal_gpio_pin_config(AM_BSP_GPIO_TP_INT, AM_HAL_PIN_INPUT|AM_HAL_GPIO_PULL24K);
    am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_TP_INT, AM_HAL_GPIO_RISING);
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TP_INT));

    // Enable the GPIO/button interrupt.
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TP_INT));
	//am_hal_gpio_int_register(AM_BSP_GPIO_TP_INT, cy8c4014_isr_handler);
	am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);
}


//extern void cy8c4014_isr_handler();
void cy8c4011_gpio_init(void)
{
    am_hal_gpio_pin_config(AM_BSP_GPIO_TP_INT, AM_HAL_PIN_INPUT|AM_HAL_GPIO_PULL24K);
    am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_TP_INT, AM_HAL_GPIO_RISING);
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TP_INT));

    // Enable the GPIO/button interrupt.
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TP_INT));
	//am_hal_gpio_int_register(AM_BSP_GPIO_TP_INT, cy8c4014_isr_handler);
	am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);

	ry_hal_i2cm_init(I2C_TOUCH);
	am_devices_cy8c4014_set_work_mode(TP_WORKING_BTN_ONLY);	//TP_WORKING_ACTIVE
	cy8c4011_set_sleep_interval(200);
}

void cy8c4011_set_mode(u8_t mode)
{
    // TP_WORKING_BTN_ONLY
    // TP_WORKING_ACTIVE
    am_devices_cy8c4014_set_work_mode(mode);	
}

void cy8c4011_set_sleep_interval(u8_t interval)
{
    am_devices_cy8c4014_reg_write(AM_DEVICES_CY8C4014_SLEEP_INTERVAL, interval);	
}


void cy8c4011_gpio_uninit(void)
{
    am_hal_gpio_pin_config(AM_BSP_GPIO_TP_INT, AM_HAL_PIN_DISABLE);

    // Enable the GPIO/button interrupt.
    am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TP_INT));
	
	// am_hal_interrupt_disable(AM_HAL_INTERRUPT_GPIO);
	am_devices_cy8c4014_set_work_mode(TP_WORKING_BTN_ONLY);
	ry_hal_i2cm_uninit(I2C_TOUCH);
}

uint8_t am_devices_cy8c4014_reg_read(uint8_t ui8Register)
{
	uint8_t tmpbuf;
	ry_hal_i2cm_rx_at_addr(I2C_TOUCH, ui8Register, &tmpbuf, 1);
    return tmpbuf;
}

void am_devices_cy8c4014_reg_write(uint8_t ui8Register, uint8_t ui8Value)
{
	ry_hal_i2cm_tx_at_addr(I2C_TOUCH, ui8Register, &ui8Value, 1);
}

void am_devices_cy8c4014_reset(void)
{
    am_devices_cy8c4014_reg_write(AM_DEVICES_CY8C4014_RESET, 0x17);
}

void am_device_cy8c4014_clear_raw(void)
{
    am_devices_cy8c4014_reg_write(AM_DEVICES_CY8C4014_RESET, 0x80);
}

void touch_set_led_state(void)
{
    am_devices_cy8c4014_reg_write(AM_DEVICES_CY8C4014_RESET, 0x80);
}

uint8_t am_devices_cy8c4014_get_button(void)
{
   return am_devices_cy8c4014_reg_read(AM_DEVICES_CY8C4014_BUTTON);
}

void am_devices_cy8c4014_get_diff(void)
{
   u16_t btn0, btn1, btn2, btn3, btn4, btn5;
   u8_t tmpL, tmpH;

   tmpL = am_devices_cy8c4014_reg_read(0x09);
   tmpH = am_devices_cy8c4014_reg_read(0x0A);
   btn0 = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x0B);
   tmpH = am_devices_cy8c4014_reg_read(0x0C);
   btn1 = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x0D);
   tmpH = am_devices_cy8c4014_reg_read(0x0E);
   btn2 = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x0F);
   tmpH = am_devices_cy8c4014_reg_read(0x10);
   btn3 = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x11);
   tmpH = am_devices_cy8c4014_reg_read(0x12);
   btn4 = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x13);
   tmpH = am_devices_cy8c4014_reg_read(0x14);
   btn5 = tmpL | (tmpH<<8);

   //LOG_DEBUG("tp diff cnt, %d %d %d %d %d %d\r\n", btn0, btn1, btn2, btn3, btn4, btn5);
   
   return ;
}

u16_t touch_get_button_diff(void)
{
    u8_t tmpL, tmpH;

    tmpL = am_devices_cy8c4014_reg_read(0x1D);
    tmpH = am_devices_cy8c4014_reg_read(0x1E);
    return tmpL | (tmpH<<8);
    
}

void touch_get_diff(u32_t* data)
{
   //u16_t btn0, btn1, btn2, btn3, btn4, btn5;
   u8_t tmpL, tmpH;

   tmpL = am_devices_cy8c4014_reg_read(0x09);
   tmpH = am_devices_cy8c4014_reg_read(0x0A);
   data[0] = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x0B);
   tmpH = am_devices_cy8c4014_reg_read(0x0C);
   data[1] = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x0D);
   tmpH = am_devices_cy8c4014_reg_read(0x0E);
   data[2] = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x0F);
   tmpH = am_devices_cy8c4014_reg_read(0x10);
   data[3] = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x11);
   tmpH = am_devices_cy8c4014_reg_read(0x12);
   data[4] = tmpL | (tmpH<<8);

   tmpL = am_devices_cy8c4014_reg_read(0x13);
   tmpH = am_devices_cy8c4014_reg_read(0x14);
   data[5] = tmpL | (tmpH<<8);
	 
	 tmpL = am_devices_cy8c4014_reg_read(0x15);
   tmpH = am_devices_cy8c4014_reg_read(0x16);
   data[6] = tmpL | (tmpH<<8);
	 
	 tmpL = am_devices_cy8c4014_reg_read(0x17);
   tmpH = am_devices_cy8c4014_reg_read(0x18);
   data[7] = tmpL | (tmpH<<8);
	 
	 tmpL = am_devices_cy8c4014_reg_read(0x19);
   tmpH = am_devices_cy8c4014_reg_read(0x1A);
   data[8] = tmpL | (tmpH<<8);
	 
	 tmpL = am_devices_cy8c4014_reg_read(0x1B);
   tmpH = am_devices_cy8c4014_reg_read(0x1C);
   data[9] = tmpL | (tmpH<<8);
	 
	 tmpL = am_devices_cy8c4014_reg_read(0x1D);
   tmpH = am_devices_cy8c4014_reg_read(0x1E);
   data[10] = tmpL | (tmpH<<8);
	 
	 tmpL = am_devices_cy8c4014_reg_read(0x1F);
   tmpH = am_devices_cy8c4014_reg_read(0x20);
   data[11] = tmpL | (tmpH<<8);

   //LOG_DEBUG("tp diff cnt, %d %d %d %d %d %d\r\n", btn0, btn1, btn2, btn3, btn4, btn5);
   
   return ;
}



void am_devices_cy8c4014_set_work_mode(uint8_t mode)
{
	if (mode == TP_WORKING_ACTIVE){
		am_devices_cy8c4014_reg_write(AM_DEVICES_CY8C4014_WORKMODE, TP_WORKING_ACTIVE);
	}
	else if (mode == TP_WORKING_BTN_ONLY){
		am_devices_cy8c4014_reg_write(AM_DEVICES_CY8C4014_WORKMODE, TP_WORKING_BTN_ONLY);
	}
}

uint32_t am_devices_cy8c4014_read_id(void )
{
	uint32_t id = 0;
	id = am_devices_cy8c4014_reg_read(0);
	id = (id << 8) + am_devices_cy8c4014_reg_read(1);
	return id;
}


uint32_t am_devices_cy8c4014_enter_btldr(void )
{
	uint8_t ui8Value[2] = {0xac,0x39};
	ry_hal_i2cm_tx_at_addr(I2C_TOUCH, 3, ui8Value, 2);
	return RY_SUCC;
}


void am_devices_cy8c4014_set_sleepinterval(uint8_t ui8interval)
{
	ry_hal_i2cm_tx_at_addr(I2C_TOUCH, AM_DEVICES_CY8C4014_SLEEP_INTERVAL, &ui8interval, 1);
	
}

uint16_t am_devices_cy8c4014_read_debuginfo(uint8_t item)
{
	uint8_t ui8Value[2];
	uint16_t debuginfo;
	ry_hal_i2cm_rx_at_addr(I2C_TOUCH, item, ui8Value, 2);
	debuginfo = (uint16_t)(ui8Value[1] << 8) + ui8Value[0];
	return debuginfo;
}

uint8_t am_devices_cy8c4014_read_raw(uint8_t addr_raw, uint16_t *buff, uint8_t len)
{
	uint8_t status = ry_hal_i2cm_rx_at_addr(I2C_TOUCH, addr_raw, (uint8_t*)buff, len << 1);
	return status;
}

uint16_t am_devices_cy8c4014_set_debuginfo(uint8_t item,uint16_t info)
{
	uint8_t ui8Value[2];
	ui8Value[0] = info & 0xff;
	ui8Value[1] = (info>>8) & 0xff;
	ry_hal_i2cm_tx_at_addr(I2C_TOUCH, item, ui8Value, 2);
	return 0;
}

void tp_get_raw_data(void)
{
	//cy8c4011_gpio_init();

	//read tp raw data
	#define TP_RAW_LEN  (((AM_DEVICES_CY8C4014_RAW_GANG - AM_DEVICES_CY8C4014_DIFF_BUTTON0) >> 1) + 1)
	uint16_t tp_raw[TP_RAW_LEN];
	#define tp_raw_size (sizeof(tp_raw) >> 1)
	am_devices_cy8c4014_read_raw(AM_DEVICES_CY8C4014_DIFF_BUTTON0, tp_raw, tp_raw_size);
#if 0	
	for(int i = 0; i < tp_raw_size; i++){
		LOG_DEBUG("tp_raw: %d %d, %04x\r\n", tp_raw_size, i, tp_raw[i]);			
	}
#endif	
}

u8_t tp_data_mapping(u8_t d)
{
    u8_t v;
    
    switch (d) {
        case 0x1:
            v = 1;
            break;

        case 0x3:
            v = 2;
            break;

        case 0x2:
            v = 3;
            break;

        case 0x6:
            v = 4;
            break;

        case 0x4:
            v = 5;
            break;

        case 0xc:
            v = 6;
            break;

        case 0x8:
            v = 7;
            break;

        case 0x18:
            v = 8;
            break;

        case 0x10:
            v = 9;
            break;

        default:
            v = 0;
            break;
            
    }

    return v;
}

u8_t tp_get_origin_data(void)
{
    return am_devices_cy8c4014_get_button();
}

u8_t tp_get_data(void)
{
    return tp_data_mapping(am_devices_cy8c4014_get_button());
}


