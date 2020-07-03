#include "ry_utils.h"

#include "app_interface.h"
#include "mcube_custom_config.h" 
#include "mcube_drv.h"
#include "ry_hal_spi.h"
#include "ry_hal_i2c.h"
#include "ry_hal_adc.h"
#include "ry_hal_pwm.h"
#include "ry_hal_spi_flash.h"
#include "am_devices_CY8C4014.h"
#include "sensor_alg.h"
#include "ppsi26x.h"
#include "gui.h"
#include "ryos.h"
#include "am_devices_rm67162.h"

uint8_t gsensor_get_id(void)
{
	return M_DRV_MC36XX_ReadChipId();
}

uint8_t hrm_get_id(void)
{
	uint8_t pData[16] = {0};
    uint8_t tx = 0x28;
    ry_hal_i2cm_tx(I2C_IDX_HR, &tx, 1);
    ry_hal_i2cm_rx(I2C_IDX_HR, pData, 3);
    uint8_t sensor_id = (*(uint32_t*)pData >> 7) & 0xff;
	return sensor_id;
}

uint32_t oled_get_id(void)
{
	return am_devices_rm67162_read_id();
}

uint32_t touch_get_id(void)
{
	return am_devices_cy8c4014_read_id();
}

uint32_t exflash_get_id(void)
{
    u32_t id;
#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_FLASH, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_FLASH);   
#endif


    ry_hal_spi_flash_wakeup();


	id = ry_hal_spi_flash_id();

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_FLASH, __FUNCTION__);  
#else
    ry_hal_spim_uninit(SPI_IDX_FLASH);   
#endif


    return id;
}

/**
 * @brief   API to get gsensor raw data
 *
 * @param   hrm_raw - Pointer to gsensor_acc_raw data
 *
 * @return  Status  - get data or not
 *			          0: SUCC, else: fail
 */
uint8_t gsensor_get_xyz(gsensor_acc_t* acc_raw)
{
	ry_sts_t status = RY_SUCC;
	uint8_t pos = cbuff_acc.head % FIFO_BUFF_DEEP;
	acc_raw->acc_x = p_acc_data->RawData[pos][M_DRV_MC_UTL_AXIS_X]; 
	acc_raw->acc_y = p_acc_data->RawData[pos][M_DRV_MC_UTL_AXIS_Y];
    acc_raw->acc_z = p_acc_data->RawData[pos][M_DRV_MC_UTL_AXIS_Z];
	// LOG_INFO("get_gsensor_xyz: %d %d %d %d\n", pos, acc_raw->acc_x, acc_raw->acc_y, acc_raw->acc_z);    
	return status;
}

/**
 * @brief   API to get hrm raw data
 *
 * @param   hrm_raw - Pointer to hrm raw data
 *
 * @return  Status  - get data or not
 *			          0: SUCC, else: fail
 */
uint8_t hrm_get_raw(hrm_raw_t* hrm_raw)
{
	ry_sts_t status = RY_SUCC;
	uint8_t pos = cbuff_hr.head % AFE_BUFF_LEN;
	hrm_raw->led_green = hr_led_green[pos]; 
	hrm_raw->led_ir    = hr_led_ir[pos];
    hrm_raw->led_amb   = hr_led_amb[pos];
    hrm_raw->led_red   = hr_led_red[pos];
	
	// LOG_INFO("get_hrm_raw: %d %d %d %d %d\n", \
		pos, hrm_raw->led_green, hrm_raw->led_ir, hrm_raw->led_amb, hrm_raw->led_red);    
	return status;
}


uint8_t touch_get_new_data(touch_data_t* touch_data)
{
	return gtouch_new_data(touch_data);
}

uint8_t exflash_get_status(void)
{
	return ry_hal_spi_flash_status();
}

uint32_t sys_get_ntc(void)
{
	return sys_get_ntc_temperature();
}

uint8_t exflash_get_data(uint8_t *buffer, uint32_t addr, uint32_t len)
{
	ry_hal_spi_flash_read(buffer, addr, len);
	return  0;
}

uint8_t oled_set_range(uint16_t col_start, uint16_t col_end, 
                       uint16_t row_start, uint16_t row_end)
{
	am_devices_rm67162_display_range_set(col_start, col_end, row_start, row_end);
	return 0;
}

uint8_t oled_set_data(uint16_t col_start, uint16_t col_end, 
                      uint16_t row_start, uint16_t row_end,
					  uint8_t *buffer,    uint32_t len)
{
	am_devices_rm67162_display_range_set(col_start, col_end, row_start, row_end);
	am_devices_rm67162_data_write_888(buffer, len);
    return 0;
}


uint8_t oled_fill_color(uint32_t color, uint32_t hold_time)
{
	gui_test_sc.cmd = GUI_TS_FILL_COLOR;
	gui_test_sc.hold_time = hold_time;
	gui_test_sc.para[0] = color;

	extern ry_queue_t	*ry_queue_evt_gui;
	ry_sts_t status = RY_SUCC;
	static gui_evt_msg_t gui_msg;
    gui_msg.evt_type = GUI_EVT_TEST;
    gui_msg.pdata = (void*)&gui_test_sc;
    gui_evt_msg_t *p_msg = &gui_msg;
    if (RY_SUCC != (status = ryos_queue_send(ry_queue_evt_gui, &p_msg, RY_OS_NO_WAIT))){
        LOG_ERROR("[oled_fill_color]: Add to gui Queue fail. status = 0x%04x\n", status);
    }
    return 0;
}

void  oled_set_brightness(uint8_t br_percent)
{
	am_devices_rm67162_display_brightness_set(255*br_percent/100);
}

uint8_t exflash_set_data(uint8_t *buffer, uint32_t addr, uint32_t len)
{
	ry_hal_spi_flash_write(buffer, addr, len);
    return 0;
}


uint8_t exflash_mass_erase(void)
{
	ry_hal_spi_flash_mass_erase();
    return 0;
}

uint8_t exflash_chip_erase(void)
{
	ry_hal_spi_flash_chip_erase();
    return 0;
}


uint8_t exflash_sector_erase(uint32_t addr)
{
	ry_hal_spi_flash_sector_erase(addr);
	return 0;
}

void  motor_set_working(uint8_t max_speed, uint32_t time)
{
	vibe_start(1, time, 0, max_speed, 0);
}
void  motor_set_stop(void)
{
    vibe_stop();
}

void  led_set_working(uint8_t breath, uint8_t max_bright, uint32_t time)
{
	breath_led_init(breath, max_bright, time);
}
