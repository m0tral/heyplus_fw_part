#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include "am_devices_rm67162.h"
#include "ry_utils.h"
#include <ry_hal_inc.h>
#include "board_control.h"
#include "device_management_service.h"

/*********************************************************************
 * TYPEDEFS
 */

typedef enum {
    DELAY_MODE_UNBLOCK,
    DELAY_MODE_BLOCK, 
} delay_mode_t;

//*****************************************************************************
//
// IOM SPI Configuration for RM67162
//
//*****************************************************************************

uint32_t rm67162_init_time_done = 0;

void rm67162_init_1(void);
void rm_write_cmd_data(uint8_t cmd, uint8_t data);
static void rm67162_set_cs_low(void)
{
#if RM67162_HW_CS
#else
    am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_CS);
#endif
}

static void rm67162_set_cs_high(void)
{
#if RM67162_HW_CS
#else
    am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_CS);
#endif
}

void rm67162_set_dcx_low(void)
{
    am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_DCX);
}

void rm67162_set_dcx_high(void)
{
    am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_DCX);
}

bool rm67162_te_ready(void)
{
    uint32_t delay_cnt = 0;
    // wait for TE high
    while(false == am_hal_gpio_input_bit_read(AM_BSP_GPIO_RM67162_TE)){
        delay_cnt++;
        if(delay_cnt > 50000){
            // timeout
            return false;
        }
    };
    delay_cnt = 0;
    while(true == am_hal_gpio_input_bit_read(AM_BSP_GPIO_RM67162_TE)){
        if(delay_cnt > 50000){
            // timeout
            return false;
        }
    }
    return true;
}

//
// register write interface
// for command without arguments, supply NULL pointer and 0 byte size to this function.
// for command with arguments, be sure NOT to load argument length > 4095
//
void am_devices_rm67162_register_write(uint8_t  ui8Command,      \
                                       uint8_t  *pui8Values,     \
                                       uint32_t ui32NumBytes)
{
    rm67162_set_dcx_low();
    if (ui32NumBytes > 0 && pui8Values != NULL){
        ry_hal_spi_write_read(SPI_IDX_OLED, &ui8Command, NULL, 1, AM_HAL_IOM_NO_STOP);
        rm67162_set_dcx_high();
        ry_hal_spi_write_read(SPI_IDX_OLED, pui8Values, NULL, ui32NumBytes, 0);
    }
    else {      // command only
        ry_hal_spi_write_read(SPI_IDX_OLED, &ui8Command, NULL, 1, 0);
        rm67162_set_dcx_high();
    }
}

void am_devices_rm67162_register_read(uint8_t  ui8Command,      \
                                      uint8_t  *pui8Values,     \
                                      uint32_t ui32NumBytes)
{
    rm67162_set_dcx_low();
    ry_hal_spi_write_read(SPI_IDX_OLED, &ui8Command, NULL, 1, AM_HAL_IOM_NO_STOP);
    rm67162_set_dcx_high();
    ry_hal_spi_write_read(SPI_IDX_OLED, &ui8Command, pui8Values, ui32NumBytes, AM_HAL_IOM_RAW);
}

void am_devices_rm67162_display_range_set(uint16_t col_start, uint16_t col_end, 
                                          uint16_t row_start, uint16_t row_end)
{
    col_start += 4;
    col_end += 4;
    uint8_t cmd_buf[4];
    cmd_buf[0] = (col_start & 0xff00) >> 8U;
    cmd_buf[1] = (col_start & 0x00ff);        // column start : 0
    cmd_buf[2] = (col_end & 0xff00) >> 8U;
    cmd_buf[3] = (col_end & 0x00ff);          // column end   : 119
    am_devices_rm67162_register_write(0x2a, cmd_buf, 4);
    
    am_util_delay_us(10);

    cmd_buf[0] = (row_start & 0xff00) >> 8U;
    cmd_buf[1] = (row_start & 0x00ff);        // row start : 0
    cmd_buf[2] = (row_end & 0xff00) >> 8U;
    cmd_buf[3] = (row_end & 0x00ff);          // row end   : 239
    am_devices_rm67162_register_write(0x2b, cmd_buf, 4);
}

void am_devices_rm67162_data_stream_enable(void)
{
    // enable memory write
    rm67162_set_dcx_low();
    uint8_t command = 0x2c;
    ry_hal_spi_write_read(SPI_IDX_OLED, &command, NULL, 1, AM_HAL_IOM_NO_STOP);
    rm67162_set_dcx_high();
}

void am_devices_rm67162_data_stream_continue(void)
{
    // enable memory write
    uint8_t command = 0x3c;

    rm67162_set_dcx_low();
    ry_hal_spi_write_read(SPI_IDX_OLED, &command, NULL, 1, AM_HAL_IOM_NO_STOP);
    rm67162_set_dcx_high();
}

void am_devices_rm67162_data_stream_888(uint8_t *pui8Values, uint32_t ui32NumBytes)
{
    rm67162_set_dcx_high();
	#define MAX_SPI_BLOCK_SIZE (1024 * 4 - 1)
	uint32_t tx_size = 0;
    #define total_size ui32NumBytes
	do{
		uint8_t* p = pui8Values + tx_size;		
		if (total_size - tx_size >= MAX_SPI_BLOCK_SIZE){
            ry_hal_spi_write_read(SPI_IDX_OLED, p, NULL, MAX_SPI_BLOCK_SIZE, AM_HAL_IOM_NO_STOP);
			tx_size += MAX_SPI_BLOCK_SIZE; 
		}
		else{
            ry_hal_spi_write_read(SPI_IDX_OLED, p, NULL, (ui32NumBytes - tx_size), 0);
			tx_size += (ui32NumBytes - tx_size); 
		}
	}while(tx_size < total_size);
    rm67162_set_cs_high();
}

void am_devices_rm67162_display_normal_area(uint16_t col_start, uint16_t col_end, 
                                            uint16_t row_start, uint16_t row_end)
{
    uint8_t cmd_buf[4];
	//am_devices_rm67162_register_write(0x13,cmd_buf,0);
    // am_util_delay_us(10);
	
    cmd_buf[0] = (col_start & 0xff00) >> 8U;
    cmd_buf[1] = (col_start & 0x00ff);        // column start : 0
    cmd_buf[2] = (col_end & 0xff00) >> 8U;
    cmd_buf[3] = (col_end & 0x00ff);          // column end   : 119
    am_devices_rm67162_register_write(0x2a, cmd_buf, 4);
    
    am_util_delay_us(10);

    cmd_buf[0] = (row_start & 0xff00) >> 8U;
    cmd_buf[1] = (row_start & 0x00ff);        // row start : 0
    cmd_buf[2] = (row_end & 0xff00) >> 8U;
    cmd_buf[3] = (row_end & 0x00ff);          // row end   : 239
    am_devices_rm67162_register_write(0x2b, cmd_buf, 4);
}

void am_devices_rm67162_display_partial_area_set(uint16_t col_start, uint16_t col_end, 
                                                 uint16_t row_start, uint16_t row_end)
{
    uint8_t cmd_buf[4];
    cmd_buf[0] = (col_start & 0xff00) >> 8U;
    cmd_buf[1] = (col_start & 0x00ff);        // column start : 0
    cmd_buf[2] = (col_end & 0xff00) >> 8U;
    cmd_buf[3] = (col_end & 0x00ff);          // column end   : 119
    am_devices_rm67162_register_write(0x31,cmd_buf,4);

    am_util_delay_us(10);

    cmd_buf[0] = (row_start & 0xff00) >> 8U;
    cmd_buf[1] = (row_start & 0x00ff);        // row start : 0
    cmd_buf[2] = (row_end & 0xff00) >> 8U;
    cmd_buf[3] = (row_end & 0x00ff);          // row end   : 239
    am_devices_rm67162_register_write(0x30,cmd_buf,4);
	am_util_delay_us(10);
	
	//am_devices_rm67162_register_write(0x12,cmd_buf,0);
}

void am_devices_rm67162_display_partial_on(void)
{
    uint8_t cmd_buf[4];
    am_devices_rm67162_register_write(0x12, cmd_buf, 0);
	am_util_delay_us(10);
}

void am_devices_rm67162_display_partial_area(uint16_t col_start,  uint16_t col_end, 
                                        uint16_t row_start,  uint16_t row_end,
										uint8_t *pui8Values, uint32_t ui32NumBytes)
{
	am_devices_rm67162_display_partial_area_set(col_start, col_end, row_start, row_end);
	am_devices_rm67162_display_partial_on();
	am_devices_rm67162_display_range_set(col_start, col_end, row_start, row_end);
	am_devices_rm67162_data_write_888(pui8Values, ui32NumBytes);
}


void am_devices_rm67162_display_normal_on(void)
{
    uint8_t cmd_buf[4];
    am_devices_rm67162_register_write(0x13, cmd_buf, 0);
	am_util_delay_us(10);
}

void am_devices_rm67162_display_brightness_set(uint8_t bright)
{
	am_devices_rm67162_register_write(0x51, &bright, 1);
	am_util_delay_us(10);
}

uint8_t am_devices_rm67162_display_brightness_get(void)
{
   uint8_t cmd_buf[2] = {0, 0};
   am_devices_rm67162_register_read(0x52, cmd_buf, 1);
   return cmd_buf[0];
}

void am_devices_rm67162_allpixeloff(void)
{
	uint8_t cmd_buf[4];
	am_devices_rm67162_register_write(0x22,cmd_buf,0);
	am_util_delay_us(10);
}

void am_devices_rm67162_allpixelon(void)
{
    uint8_t cmd_buf[4];
   // cmd_buf[0] = bright;   
    am_devices_rm67162_register_write(0x23,cmd_buf,0);
	am_util_delay_us(10);
}

void am_devices_rm67162_displayoff(void)
{
    uint8_t cmd_buf[4];
   // cmd_buf[0] = bright;   
    am_devices_rm67162_register_write(0x28,cmd_buf,0);
	am_util_delay_us(10);
}

void am_devices_rm67162_displayon(void)
{
    uint8_t cmd_buf[4];
   // cmd_buf[0] = bright;

    const uint8_t max_timeout = 50; 
    uint8_t timeout = max_timeout;
    do
    {
        if (ry_hal_calc_ms(ry_hal_clock_time() - rm67162_init_time_done) >= max_timeout)
            break;
        
        am_util_delay_ms(1);
        timeout--;
    }
    while(timeout > 0);    
    
    am_devices_rm67162_register_write(0x29, cmd_buf, 0);
	am_util_delay_us(10);
}

void am_devices_rm67162_inversionon(void)
{
    uint8_t cmd_buf[4];
   // cmd_buf[0] = bright;  
    
    am_devices_rm67162_register_write(0x21,cmd_buf,0);
	am_util_delay_us(10);
}

void am_devices_rm67162_inversionoff(void)
{
    uint8_t cmd_buf[4];
   // cmd_buf[0] = bright;   
    am_devices_rm67162_register_write(0x20,cmd_buf,0);
	am_util_delay_us(10);
}

void am_devices_rm67162_enter_sleep(uint8_t delay_mode)
{
    // LOG_DEBUG("[gui device am_devices_rm67162_enter_sleep] ready to sleep.\r\n");    
#if 0
	am_devices_rm67162_allpixeloff();
#else	
	// LOG_DEBUG("[gui device] enter sleep...\r\n");
	uint8_t cmd_buf[4];
//	uint8_t bright = 0xFF;
	
	am_devices_rm67162_display_brightness_set(0);
//	uint32_t counter = 0;
	
	ryos_delay_ms(500);
	
//	do {
//		bright = am_devices_rm67162_display_brightness_get();
//		am_util_delay_us(10);
//		counter++;
//	}
//	while(bright > 0 || counter < 0xF0000000);
	
	am_devices_rm67162_allpixeloff();
	am_devices_rm67162_register_write(0x10, cmd_buf, 0);

	if (delay_mode == DELAY_MODE_UNBLOCK){
		ryos_delay_ms(120);
	}
	else{
		am_util_delay_ms(120);
	}

#if SPI_LOCK_DEBUG
    ry_hal_spim_uninit(SPI_IDX_OLED, __FUNCTION__);	
#else
    ry_hal_spim_uninit(SPI_IDX_OLED);	
#endif

	
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_DCX, AM_HAL_GPIO_DISABLE);	//38
	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_GPIO_OUTPUT);   //37
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_RES);

	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_GPIO_DISABLE);		//35
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_SCK, AM_HAL_GPIO_DISABLE);		//39
	am_hal_gpio_pin_config(AM_BSP_GPIO_IOM4_MOSI, AM_HAL_GPIO_DISABLE);		//44

	if (delay_mode == DELAY_MODE_UNBLOCK){
		ryos_delay_ms(10);
	}
	else{
		am_util_delay_ms(2);
	}
#endif	
    // LOG_DEBUG("[gui device am_devices_rm67162_enter_sleep] start sleep.\r\n");    
}

void am_devices_rm67162_powerdown(void)
{
	am_devices_rm67162_enter_sleep(DELAY_MODE_UNBLOCK);
	//am_hal_gpio_pin_config(AM_BSP_GPIO_LCD_ON, AM_HAL_GPIO_OUTPUT);
	//am_hal_gpio_out_bit_clear(AM_BSP_GPIO_LCD_ON);

    // To be test
    power_oled_on(0);
}

void am_devices_rm67162_powerdown_with_block_delay(void)
{
	am_devices_rm67162_enter_sleep(DELAY_MODE_BLOCK);
    power_oled_on(0);
}

void am_devices_rm67162_exit_sleep(void)
{
    // LOG_DEBUG("[gui device am_devices_rm67162_exit_sleep] ready to exit sleep.\r\n");    
#if 0 
	// am_devices_rm67162_display_normal_on();
	am_devices_rm67162_allpixelon();
#else	
	uint8_t cmd_buf[4];
	
	rm67162_gpio_init();

#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_OLED, __FUNCTION__);
#else 
    ry_hal_spim_init(SPI_IDX_OLED);
#endif


	am_devices_rm67162_register_write(0x11, cmd_buf, 0);
	am_util_delay_us(10);
	rm67162_init_1();
	//am_devices_rm67162_displayon();
#endif	  
    // LOG_DEBUG("[gui device am_devices_rm67162_exit_sleep] exit sleep and ready to display.\r\n");  	
}


/**
 * @brief   API to set oled into low power, off all pixel
 *
 * @param   None
 *
 * @return  None
 */
void oled_enter_low_power(void)
{
	am_devices_rm67162_allpixeloff();
	uint8_t cmd_buf[4];
    am_devices_rm67162_register_write(0x39, cmd_buf, 0);
}


/**
 * @brief   API to restore oled from low power, all pixel display on
 *
 * @param   None
 *
 * @return  None
 */
void oled_restore_from_low_power(void)
{
	uint8_t cmd_buf[4];
    am_devices_rm67162_register_write(0x38, cmd_buf, 0);
    am_devices_rm67162_display_normal_on();
}

void am_devices_rm67162_poweron(void)
{
    am_devices_rm67162_exit_sleep();
}

void am_devices_rm67162_enter_idle(void)
{
    uint8_t cmd_buf[4];
  
    am_devices_rm67162_register_write(0x39,cmd_buf,0);
	am_util_delay_us(10);
}

void am_devices_rm67162_exit_idle(void)
{
    uint8_t cmd_buf[4];
   
    am_devices_rm67162_register_write(0x38,cmd_buf,0);
	am_util_delay_us(10);
}

uint32_t am_devices_rm67162_read_id(void)
{
   uint8_t cmd_buf[4] = {0, 0, 0, 0};
   uint32_t iret;
   am_devices_rm67162_register_read(0x04, cmd_buf, 3);
   iret = (cmd_buf[0] << 16) + (cmd_buf[1] << 8) + cmd_buf[2];
   return iret;
}

void am_devices_rm67162_fixed_data_888(uint32_t ui32Data, uint32_t ui32NumBytes)
{
    uint32_t ui32BytesRemaining = ui32NumBytes;

    uint8_t ui8Command = 0x2c;
    rm67162_set_dcx_low();
    ry_hal_spi_write_read(SPI_IDX_OLED, &ui8Command, NULL, 1, AM_HAL_IOM_NO_STOP);
    rm67162_set_dcx_high();

    //if number of bytes is odd, we complete it to even
    ui32NumBytes = ((ui32NumBytes % 2) == 0) ? ui32NumBytes : (ui32NumBytes + 1); 

    rm67162_te_ready();
    ui32BytesRemaining = ui32NumBytes - 3;      // leave the last 3 bytes to recover CS

    uint8_t buffer[4];
    buffer[0] = ui32Data >> 16;
    buffer[1] = ui32Data >> 8;
    buffer[2] = ui32Data >> 0;
    
    while(ui32BytesRemaining){
        if ((ui32BytesRemaining % 3) == 0)
        ry_hal_spi_write_read(SPI_IDX_OLED, buffer, NULL, 3, AM_HAL_IOM_NO_STOP);
        ui32BytesRemaining -= 3;
    }
    ry_hal_spi_write_read(SPI_IDX_OLED, buffer, NULL, 3, 0);
    rm67162_set_cs_high();
}

void am_devices_rm67162_data_write_888(uint8_t *pui8Values, uint32_t ui32NumBytes)
{
    uint8_t ui8Command = 0x2c;
    rm67162_set_dcx_low();
    ry_hal_spi_write_read(SPI_IDX_OLED, &ui8Command, NULL, 1, AM_HAL_IOM_NO_STOP);
    rm67162_set_dcx_high();
    rm67162_te_ready();
	#define MAX_SPI_BLOCK_SIZE (1024 * 4 - 1)
	uint32_t tx_size = 0;
    #define total_size ui32NumBytes
	do {
		uint8_t* p = pui8Values + tx_size;		
		if (total_size - tx_size >= MAX_SPI_BLOCK_SIZE){
            ry_hal_spi_write_read(SPI_IDX_OLED, p, NULL, MAX_SPI_BLOCK_SIZE, AM_HAL_IOM_NO_STOP);
			tx_size += MAX_SPI_BLOCK_SIZE; 
		} else {
            ry_hal_spi_write_read(SPI_IDX_OLED, p, NULL, (ui32NumBytes - tx_size), 0);
			tx_size += (ui32NumBytes - tx_size); 
		}
	} while(tx_size < total_size);
    rm67162_set_cs_high();
}

void rm67162_gpio_init(void)
{		
	power_oled_on(1);
    //ryos_delay_ms(15);

	am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_RES, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_RES);

#if RM67162_HW_CS
    am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_BSP_GPIO_CFG_RM67162_CS);
#else
    am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_CS, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_CS);
#endif
    am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_DCX, AM_HAL_PIN_OUTPUT);
    am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_DCX);

    am_hal_gpio_pin_config(AM_BSP_GPIO_RM67162_TE, AM_HAL_PIN_INPUT);
}

void WriteComm(uint8_t cmd)
{
	rm67162_set_dcx_low();
    ry_hal_spi_write_read(SPI_IDX_OLED, &cmd, NULL, 1, 0);
    rm67162_set_dcx_high();
}

void WriteData(uint8_t m_data)
{
	ry_hal_spi_write_read(SPI_IDX_OLED, &m_data, NULL, 1, 0);
}

void rm_write_cmd_data(uint8_t cmd, uint8_t data)
{
	WriteComm(cmd);
	WriteData(data);
}

void rm67162_init_1(void)
{
    u8_t dimming_type = get_lcd_off_type();
    
	am_hal_gpio_out_bit_clear(AM_BSP_GPIO_RM67162_RES);
	am_util_delay_ms(30); //200
	am_hal_gpio_out_bit_set(AM_BSP_GPIO_RM67162_RES);
	am_util_delay_ms(30);  //150 Delay 120ms

	rm_write_cmd_data(0xFE, 0x01);
	rm_write_cmd_data(0x04, 0xa0);  
	rm_write_cmd_data(0x05, 0x70);
	rm_write_cmd_data(0x06, 0x3C);
	rm_write_cmd_data(0x25, 0x06);
	rm_write_cmd_data(0x26, 0x57);
	rm_write_cmd_data(0x27, 0x12);
	rm_write_cmd_data(0x28, 0x12);
	rm_write_cmd_data(0x2A, 0x06);  // column address set
	rm_write_cmd_data(0x2B, 0x57);  // row address set
	rm_write_cmd_data(0x2D, 0x12);
	rm_write_cmd_data(0x2F, 0x12);
	rm_write_cmd_data(0x37, 0x0C);
	rm_write_cmd_data(0x6D, 0x18);
	rm_write_cmd_data(0x29, 0x01);
	rm_write_cmd_data(0x30, 0x41);
	rm_write_cmd_data(0x3A, 0x1D);
	rm_write_cmd_data(0x3B, 0x00);
	rm_write_cmd_data(0x3D, 0x16);
	rm_write_cmd_data(0x3F, 0x2D);
	rm_write_cmd_data(0x40, 0x14);
	rm_write_cmd_data(0x41, 0x0D);
	rm_write_cmd_data(0x42, 0x63);
	rm_write_cmd_data(0x43, 0x36);
	rm_write_cmd_data(0x44, 0x41);
	rm_write_cmd_data(0x45, 0x14);
	rm_write_cmd_data(0x46, 0x52);
	rm_write_cmd_data(0x47, 0x25);
	rm_write_cmd_data(0x48, 0x63);
	rm_write_cmd_data(0x49, 0x36);
	rm_write_cmd_data(0x4A, 0x41);
	rm_write_cmd_data(0x4B, 0x14);
	rm_write_cmd_data(0x4C, 0x52);
	rm_write_cmd_data(0x4D, 0x25);
	rm_write_cmd_data(0x4E, 0x63);
	rm_write_cmd_data(0x4F, 0x36);      // check 2
	rm_write_cmd_data(0x50, 0x41);
	rm_write_cmd_data(0x51, 0x14);
	rm_write_cmd_data(0x52, 0x52);
	rm_write_cmd_data(0x53, 0x25);	// 00100101b
	rm_write_cmd_data(0x54, 0x63);
	rm_write_cmd_data(0x55, 0x36);
	rm_write_cmd_data(0x56, 0x41);
	rm_write_cmd_data(0x57, 0x14);
	rm_write_cmd_data(0x58, 0x52);
	rm_write_cmd_data(0x59, 0x25);
	rm_write_cmd_data(0x66, 0x10);
	rm_write_cmd_data(0x67, 0x40);
	rm_write_cmd_data(0x70, 0xA5);
	rm_write_cmd_data(0x72, 0x1A);

	rm_write_cmd_data(0x73, 0x15);
	rm_write_cmd_data(0x74, 0x0C);
    
	rm_write_cmd_data(0xFE, 0x04);
	rm_write_cmd_data(0x00, 0xDC);
	rm_write_cmd_data(0x01, 0x00);
	rm_write_cmd_data(0x02, 0x02);
	rm_write_cmd_data(0x03, 0x00);
	rm_write_cmd_data(0x04, 0x00);
	rm_write_cmd_data(0x05, 0x01);
	rm_write_cmd_data(0x06, 0x09);
	rm_write_cmd_data(0x07, 0x0A);
	rm_write_cmd_data(0x08, 0x00);
	rm_write_cmd_data(0x09, 0xDC);
	rm_write_cmd_data(0x0A, 0x00);
	rm_write_cmd_data(0x0B, 0x02);
	rm_write_cmd_data(0x0C, 0x00);
	rm_write_cmd_data(0x0D, 0x00);
	rm_write_cmd_data(0x0E, 0x00);
	rm_write_cmd_data(0x0F, 0x09);
	rm_write_cmd_data(0x10, 0x0A);
	rm_write_cmd_data(0x11, 0x00);
	rm_write_cmd_data(0x12, 0xDC);
	rm_write_cmd_data(0x13, 0x00);
	rm_write_cmd_data(0x14, 0x02);
	rm_write_cmd_data(0x15, 0x00);
	rm_write_cmd_data(0x16, 0x08);
	rm_write_cmd_data(0x17, 0x01);
	rm_write_cmd_data(0x18, 0xA3);
	rm_write_cmd_data(0x19, 0x00);
	rm_write_cmd_data(0x1A, 0x00);
	rm_write_cmd_data(0x1B, 0xDC);
	rm_write_cmd_data(0x1C, 0x00);
	rm_write_cmd_data(0x1D, 0x02);
	rm_write_cmd_data(0x1E, 0x00);
	rm_write_cmd_data(0x1F, 0x08);
	rm_write_cmd_data(0x20, 0x00);
	rm_write_cmd_data(0x21, 0xA3);
	rm_write_cmd_data(0x22, 0x00);
	rm_write_cmd_data(0x23, 0x00);
	rm_write_cmd_data(0x4C, 0x89);	
	rm_write_cmd_data(0x4D, 0x00);
	rm_write_cmd_data(0x4E, 0x01);
	rm_write_cmd_data(0x4F, 0x08);
	rm_write_cmd_data(0x50, 0x01);
	rm_write_cmd_data(0x51, 0x85);
	rm_write_cmd_data(0x52, 0x7C);
	rm_write_cmd_data(0x53, 0x8A); // 10001010b
	rm_write_cmd_data(0x54, 0x50);
	rm_write_cmd_data(0x55, 0x02);
	
	rm_write_cmd_data(0x56, 0x48);
	rm_write_cmd_data(0x58, 0x34);
	rm_write_cmd_data(0x59, 0x00);
	rm_write_cmd_data(0x5E, 0xBB);
	rm_write_cmd_data(0x5F, 0xBB);
	rm_write_cmd_data(0x60, 0x09);
	rm_write_cmd_data(0x61, 0xB1);
	rm_write_cmd_data(0x62, 0xBB);
	rm_write_cmd_data(0x65, 0x05);
	rm_write_cmd_data(0x66, 0x04);
	rm_write_cmd_data(0x67, 0x00);
	rm_write_cmd_data(0x78, 0xBB);
	rm_write_cmd_data(0x79, 0x8B);
	rm_write_cmd_data(0x7A, 0x32);
    
	rm_write_cmd_data(0xFE, 0x01);
	rm_write_cmd_data(0x0E, 0xa5);
	rm_write_cmd_data(0x0F, 0xa5);
	rm_write_cmd_data(0x10, 0x11);
	rm_write_cmd_data(0x11, 0xd2);
	rm_write_cmd_data(0x12, 0xd2);
	rm_write_cmd_data(0x13, 0xa2);
	rm_write_cmd_data(0x14, 0xa2);
	rm_write_cmd_data(0x15, 0xa2);
	rm_write_cmd_data(0x16, 0xa2);
	rm_write_cmd_data(0x18, 0x55);
	rm_write_cmd_data(0x19, 0x33);
	rm_write_cmd_data(0x1E, 0x02);
	rm_write_cmd_data(0x5B, 0x10);
	rm_write_cmd_data(0x62, 0x15);
	rm_write_cmd_data(0x63, 0x15);
#if ((HW_VER_CURRENT == HW_VER_SAKE_01) || (HW_VER_CURRENT == HW_VER_SAKE_03))
	rm_write_cmd_data(0x6A, 0x29);
#elif (HW_VER_CURRENT == HW_VER_SAKE_02)
    rm_write_cmd_data(0x6A, 0x15);
#endif
	rm_write_cmd_data(0x70, 0x55);
	rm_write_cmd_data(0x1D, 0x02);
	rm_write_cmd_data(0x89, 0xf8);
	rm_write_cmd_data(0x8A, 0x80);
	rm_write_cmd_data(0x8B, 0x01);

#if 1
	rm_write_cmd_data(0xFE, 0x0A);
	rm_write_cmd_data(0x3A, 0x06);
#endif
    
	rm_write_cmd_data(0xFE, 0x00);

    if (dimming_type == 1)
        rm_write_cmd_data(0x53, 0x20); // dimming off
    else
        rm_write_cmd_data(0x53, 0x2C); // dimming on
	
	rm_write_cmd_data(0x2A, 0x00);  // column address set
	
	WriteData(0x04);
	WriteData(0x00);
	WriteData(0x7B);

	rm_write_cmd_data(0x35, 0x00);
	rm_write_cmd_data(0x36, 0x08);	
	rm_write_cmd_data(0x3A, 0x77);  // set colour mode - 24bit per pixel

	WriteComm(0x11);                // sleep out
	
    rm67162_init_time_done = ry_hal_clock_time();

	WriteComm(0x28);                // display off
    //am_devices_rm67162_display_brightness_set(0xCC); // 70:B2, 75:BF 80:cc
	//am_util_delay_ms(50); // 50ms   
}

void set_brightness(char level){
    WriteComm(0x51);
    WriteData(level);//01
}

// set RGB / BGR order
void rm_set_bgr_mode(uint8_t bgr)
{
    WriteComm(0x36);
    if (bgr){
	    WriteData(0x08);
    }
    else{
	    WriteData(0x00);
    }
}


void gui_hw_init(void)
{
    rm67162_gpio_init();

#if SPI_LOCK_DEBUG
    ry_hal_spim_init(SPI_IDX_OLED, __FUNCTION__);  
#else
    ry_hal_spim_init(SPI_IDX_OLED);   
#endif

    rm67162_init_1();
    
    am_devices_rm67162_displayoff();
    
    am_devices_rm67162_displayon();
}

// EOF
