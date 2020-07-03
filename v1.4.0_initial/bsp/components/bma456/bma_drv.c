/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "app_config.h"

/* BMA Gsensor related */
#include "bma456.h"
#include "bma4_defs.h"

#include "ry_hal_inc.h"

/*********************************************************************
 * CONSTANTS
 */ 

#define GSENSOR_BMA_TEST_EN			0
#define GSENSOR_BMA_TEST_STEP		0

 /*********************************************************************
 * TYPEDEFS
 */

 
/*********************************************************************
 * LOCAL VARIABLES
 */
struct bma4_dev accel;
struct bma4_accel sens_data;


struct bma4_dev sensor_accel_bma4xy;
struct bma4_accel sensor_accel;	
//struct bma422_stepcounter_settings stepcounter_conf;
struct bma4_int_pin_config  bma4_int_pin_cof;
struct bma4_accel sens_data_fifo[36];
struct bma4_accel_config bma4xy_accel_config;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static inline void DelayUs(uint32_t t)
{
	am_util_delay_us(t);
}

uint16_t bma_i2c_read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len)
{
	return ry_hal_i2cm_rx_at_addr(I2C_GSENSOR, reg_addr, data, len);
}

uint16_t bma_i2c_write_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint16_t len)
{
	return ry_hal_i2cm_tx_at_addr(I2C_GSENSOR, reg_addr, data, len);
}

uint16_t bma_accel_config(void)
{
	uint16_t rslt = BMA4_OK;
	/* Initialize the device instance as per the initialization example */
	
	/* Enable the accelerometer */
	bma4_set_accel_enable(1, &accel);
	
	/* Declare an accelerometer configuration structure */
	struct bma4_accel_config accel_conf;
	
	/* Assign the desired settings */
	accel_conf.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
	accel_conf.range = BMA4_ACCEL_RANGE_2G;
	accel_conf.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
	
	/* Set the configuration */
	rslt |= bma4_set_accel_config(&accel_conf, &accel);
	return rslt;
}


void bma4xy_Init_Step_func(void)
{
	unsigned char val = 0;
	unsigned short index = 0;
	unsigned short *config_id;
	
	sensor_accel_bma4xy.dev_addr = BMA4_I2C_ADDR_PRIMARY;
	sensor_accel_bma4xy.interface = BMA4_I2C_INTERFACE;
	sensor_accel_bma4xy.bus_read = bma_i2c_read_reg;
	sensor_accel_bma4xy.bus_write = bma_i2c_write_reg;
	sensor_accel_bma4xy.delay = am_util_delay_ms;
	sensor_accel_bma4xy.read_write_len = 32;
	
	//bma4xy_Chip_Dev_func();
	ry_hal_i2cm_init(I2C_GSENSOR);
	/* Initialize the instance */
	uint8_t rslt = bma456_init(&sensor_accel_bma4xy);
	am_util_debug_printf("chip_id: 0x%x\r\n", sensor_accel_bma4xy.chip_id);

	bma4_set_command_register(0xB6, &sensor_accel_bma4xy);
	DelayUs(600);
	
	bma4_set_accel_enable(BMA4_ENABLE, &sensor_accel_bma4xy);		
	DelayUs(200);                                                                                                                                                           
	
	//bma423_write_config_file(&sensor_accel_bma4xy);
	//bma422_write_config_file(&sensor_accel_bma4xy);
	bma456_write_config_file(&sensor_accel_bma4xy);
	val = 0;
	while (val != 0x01){
		sensor_accel_bma4xy.bus_read(BMA4_I2C_ADDR_PRIMARY, BMA4_INTERNAL_STAT, &val, 1);
		am_util_debug_printf("BMA4_INTERNAL_STAT = %x \r\n", val);
	}
	
	DelayUs(400);
	bma4_get_accel_config(&bma4xy_accel_config, &sensor_accel_bma4xy);
	
	bma4xy_accel_config.odr = BMA4_OUTPUT_DATA_RATE_50HZ;
	bma4xy_accel_config.bandwidth = BMA4_ACCEL_OSR2_AVG2;
	bma4xy_accel_config.range = BMA4_ACCEL_RANGE_8G;
	
	bma4_set_accel_config(&bma4xy_accel_config, &sensor_accel_bma4xy);	
	bma456_step_detector_enable(BMA4_ENABLE, &sensor_accel_bma4xy);	
	bma456_feature_enable(BMA456_STEP_CNTR, BMA4_ENABLE, &sensor_accel_bma4xy);
}

uint16_t bma_accel_init(void)
{
	uint16_t rslt = BMA4_OK;
	/* Modify the parameters */
	accel.dev_addr = BMA4_I2C_ADDR_PRIMARY;
	accel.interface = BMA4_I2C_INTERFACE;
	accel.bus_read = bma_i2c_read_reg;
	accel.bus_write = bma_i2c_write_reg;
	accel.delay = am_util_delay_ms;
	accel.read_write_len = 8;
	ry_hal_i2cm_init(I2C_GSENSOR);
	/* Initialize the instance */
	rslt = bma456_init(&accel);
	am_util_debug_printf("chip_id: 0x%x\r\n", accel.chip_id);
	bma_accel_config();
	return rslt;
}

uint16_t bma_accel_polling(void)
{
	uint16_t rslt = BMA4_OK;
	static uint32_t loop;
	/* Read the sensor data into the sensor data instance */
	rslt |= bma4_read_accel_xyz(&sens_data, &accel);
	
	/* Exit the program in case of a failure */
	if (rslt != BMA4_OK){
		//return rslt;
		am_util_debug_printf("read sensor fail.\r\n");
	}
	
	//Make 1024 = 1g: range 2g, 16bit
	//[(2 << 15) >> 1] count is 1g
	/* Use the data */
	am_util_debug_printf("loop %d:  ||  X: %d,  |  Y: %d,  |  Z: %d\n", loop++, sens_data.x >> 4, sens_data.y  >> 4, sens_data.z >> 4);
	
	/* Pause for 10ms, 100Hz output data rate */
	am_util_delay_ms(10);
	return rslt;
}

void bma_accel_test_step(void)
{
    bma4xy_Init_Step_func();
	while(1){
		static uint32_t step_count = 0;
		static uint32_t loop;
		bma456_step_counter_output(&step_count, &sensor_accel_bma4xy);
		am_util_debug_printf("loop %d:   X: %d\n", loop++, step_count);
	    am_util_delay_ms(100);
	}
}

