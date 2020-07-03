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
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"
#include <ry_hal_inc.h>

#if (TRUE)

#define OLED_GPIO_RES           37
#define OLED_GPIO_DC            38

/*********************************************************************
 * CONSTANTS
 */ 

#define OLED_DC_OUTPUT_HIGH  	do{am_hal_gpio_out_bit_replace(OLED_GPIO_DC, 1);} while(0)

#define OLED_DC_OUTPUT_LOW   	do{am_hal_gpio_out_bit_replace(OLED_GPIO_DC, 0);} while(0)

#define OLED_RES_OUTPUT_HIGH 	do{am_hal_gpio_out_bit_replace(OLED_GPIO_RES, 1);} while(0)
#define OLED_RES_OUTPUT_LOW  	do{am_hal_gpio_out_bit_replace(OLED_GPIO_RES, 0);} while(0)

#define OLED_SSD1357_START_X    (28)
#define OLED_SSD1357_START_Y    (0)
#define OLED_SSD1357_SIZE_X   	(72)
#define OLED_SSD1357_SIZE_Y   	(128)
#define OLED_FRAME_BUFF_SIZE  	(OLED_SSD1357_SIZE_X * OLED_SSD1357_SIZE_Y * 2)

 /*********************************************************************
 * TYPEDEFS
 */
// SSD1357 Commands
enum
{
  SSD1351_CMD_SETCOLUMNADDRESS			= 0x15,
  SSD1351_CMD_SETROWADDRESS 			= 0x75,
  SSD1351_CMD_WRITERAM					= 0x5C, // Write data to GRAM and increment until another command is sent
  SSD1351_CMD_READRAM					= 0x5D, // Read data from GRAM and increment until another command is sent
  SSD1351_CMD_COLORDEPTH				= 0xA0, // Numerous functions include increment direction ... see DS 
												// A0[0] = Address Increment Mode (0 = horizontal, 1 = vertical)
												// A0[1] = Column Address Remap (0 = left to right, 1 = right to left)
												// A0[2] = Color Remap (0 = ABC, 1 = CBA) - HW RGB/BGR switch
												// A0[4] = COM Scan Direction (0 = top to bottom, 1 = bottom to top)
												// A0[5] = Odd/Even Paid Split
												// A0[7:6] = Display Color Mode (00 = 8-bit, 01 = 65K, 10/11 = 262K, 8/16-bit interface only)
  SSD1351_CMD_SETDISPLAYSTARTLINE		= 0xA1,
  SSD1351_CMD_SETDISPLAYOFFSET			= 0xA2, 
  SSD1351_CMD_SETDISPLAYMODE_ALLOFF 	= 0xA4, // Force entire display area to grayscale GS0
  SSD1351_CMD_SETDISPLAYMODE_ALLON		= 0xA5, // Force entire display area to grayscale GS63
  SSD1351_CMD_SETDISPLAYMODE_RESET		= 0xA6, // Resets the display area relative to the above two commands
  SSD1351_CMD_SETDISPLAYMODE_INVERT 	= 0xA7, // Inverts the display contents (GS0 -> GS63, GS63 -> GS0, etc.)
  SSD1351_CMD_FUNCTIONSELECTION 		= 0xAB, // Enable/Disable the internal VDD regulator
  SSD1351_CMD_SLEEPMODE_DISPLAYOFF		= 0xAE, // Sleep mode on (display off)
  SSD1351_CMD_SLEEPMODE_DISPLAYON		= 0xAF, // Sleep mode off (display on)
  SSD1351_CMD_SETPHASELENGTH			= 0xB1, // Larger capacitance may require larger delay to discharge previous pixel state
  SSD1351_CMD_ENHANCEDDRIVINGSCHEME 	= 0xB2, 
  SSD1351_CMD_SETFRONTCLOCKDIV			= 0xB3, // DCLK divide ration fro CLK (from 1 to 16)
  SSD1351_CMD_SETSEGMENTLOWVOLTAGE		= 0xB4,
  SSD1351_CMD_SETGPIO					= 0xB5,
  SSD1351_CMD_SETSECONDPRECHARGEPERIOD	= 0xB6,
  SSD1351_CMD_GRAYSCALELOOKUP			= 0xB8,
  SSD1351_CMD_LINEARLUT 				= 0xB9,
  SSD1351_CMD_SETPRECHARGEVOLTAGE		= 0xBB,
  SSD1351_CMD_SETVCOMHVOLTAGE			= 0xBE,
  SSD1351_CMD_SETCONTRAST				= 0xC1,
  SSD1351_CMD_MASTERCONTRAST			= 0xC7,
  SSD1351_CMD_SETMUXRRATIO				= 0xCA,
  SSD1351_CMD_NOP3						= 0xD1,
  SSD1351_CMD_NOP4						= 0xE3,
  SSD1351_CMD_SETCOMMANDLOCK			= 0xFD,
  SSD1351_CMD_HORIZONTALSCROLL			= 0x96,
  SSD1351_CMD_STOPMOVING				= 0x9E,
  SSD1351_CMD_STARTMOVING				= 0x9F	
};

 
/*********************************************************************
 * LOCAL VARIABLES
 */
const u8_t gamma_look_up_ble[] = {
    0x02, 0x03, 0x04, 0x05, 
    0x06, 0x07, 0x08, 0x09, 
    0x0A, 0x0B, 0x0C, 0x0D, 
    0x0E, 0x0F, 0x10, 0x11, 
    0x12, 0x13, 0x15, 0x17, 
    0x19, 0x1B, 0x1D, 0x1F, 
    0x21, 0x23, 0x25, 0x27, 
    0x2A, 0x2D, 0x30, 0x33, 
    0x36, 0x39, 0x3C, 0x3F, 
    0x42, 0x45, 0x48, 0x4C, 
    0x50, 0x54, 0x58, 0x5C, 
    0x60, 0x64, 0x68, 0x6C, 
    0x70, 0x74, 0x78, 0x7D, 
    0x82, 0x87, 0x8C, 0x91, 
    0x96, 0x9B, 0xA0, 0xA5, 
    0xAA, 0xAF, 0xB4, 0x00,
};


u8_t frame_buffer[OLED_FRAME_BUFF_SIZE];


/*********************************************************************
 * FUNCTIONS declear
 */
void oled_ssd1357_clear_screen(void);
void oled_ssd1357_power_up(void); 
void oled_ssd1357_fill_rgb(u8_t r, u8_t g, u8_t b);

void oled_ssd1357_write_cmd(u8_t cmd)
{
    OLED_DC_OUTPUT_LOW;
    ry_hal_spi_txrx(SPI_IDX_OLED, &cmd, NULL, 1);
}

void oled_ssd1357_write_data(u8_t data)
{
    OLED_DC_OUTPUT_HIGH;
    ry_hal_spi_txrx(SPI_IDX_OLED, &data, NULL, 1);
}

static inline void oled_ssd1357_write_cmd_2(u8_t cmd, u8_t data)
{
    oled_ssd1357_write_cmd(cmd);
    oled_ssd1357_write_data(data);
}

void oled_ssd1357_write_data_buff(u8_t *buff, u32_t len)
{
    OLED_DC_OUTPUT_HIGH;
    ry_hal_spi_txrx(SPI_IDX_OLED, buff, NULL, len);
}

static inline void oled_ssd1357_delay(u32_t us)
{
	am_util_delay_us(us); 
}

void test_oled_fill_color(void)
{
    static int i;
    const u8_t c[5][3] = {
		0xff, 0x00, 0x00,
		0x00, 0xff, 0x00,
		0x00, 0x00, 0xff,
		0x00, 0x00, 0x00,
		0xff, 0xff, 0xff,
	};

    for (i = 0; i < 1; i++){
        u8_t r = c[i][0];		
        u8_t g = c[i][1];
        u8_t b = c[i][2];
        oled_ssd1357_fill_rgb(r, g, b);		
        oled_ssd1357_delay(1000 * 1000);
    }
}

ry_sts_t oled_ssd1357_init(void) 
{
    /* Initialize LCD SPI*/
    //ry_hal_spim_init(SPI_IDX_OLED);

    //initial oled gpio
    //ry_hal_gpio_init(GPIO_IDX_OLED_DC, RY_HAL_GPIO_DIR_OUTPUT);
    //ry_hal_gpio_init(GPIO_IDX_OLED_RES, RY_HAL_GPIO_DIR_OUTPUT);

    //set RES# as High
    OLED_RES_OUTPUT_HIGH;

    //delay 2us
    oled_ssd1357_delay(2);

    //******************************************
    //initialized state ???

    //command Lock
    oled_ssd1357_write_cmd_2(0xFD, 0x12);

    //set display off
    oled_ssd1357_write_cmd(0xAE);

    //******************************************
    //Initial setting config

    //set display clock
    oled_ssd1357_write_cmd_2(0xB3, 0xB0);

    //set multiplex ratio
    oled_ssd1357_write_cmd_2(0xCA, 0x7F);

    //Set Display line
    oled_ssd1357_write_cmd_2(0xA1, 0x00);

    //Set Display Offset 
    oled_ssd1357_write_cmd_2(0xA2, 0x00);

    //Set Re-Map & color depth
    oled_ssd1357_write_cmd_2(0xA0, 0x72);
    oled_ssd1357_write_data(0x00);

    //set constrat current
    oled_ssd1357_write_cmd_2(0xC1, 0x88);
    oled_ssd1357_write_data(0x3C);
    oled_ssd1357_write_data(0x9c);

    //Master Contrast Current Control 
    oled_ssd1357_write_cmd_2(0xC7, 0x0F);

    //set phase Length
    oled_ssd1357_write_cmd_2(0xB1, 0x32);

    //set second Pre-charge period
    oled_ssd1357_write_cmd_2(0xB6, 0x01);

    //Set Pre-charge voltage
    oled_ssd1357_write_cmd_2(0xBB, 0x17);

    //Gamma Look up table
    oled_ssd1357_write_cmd(0xB8);
    oled_ssd1357_write_data_buff((u8_t *)gamma_look_up_ble, sizeof(gamma_look_up_ble));

    //Set VCOMH Deselect Level
    oled_ssd1357_write_cmd_2(0xBE, 0x05);

    //Set Column Address 
    oled_ssd1357_write_cmd_2(0x15, 0);
    oled_ssd1357_write_data(127);

    //Set Row Address 
    oled_ssd1357_write_cmd_2(0x75, 0x00);
    oled_ssd1357_write_data(0x7F);

    //set display mode
    oled_ssd1357_write_cmd(0xA6);

    //Clear screen
    //oled_ssd1357_clear_screen();

    //set display On
    oled_ssd1357_write_cmd(0xAF);

    // Init Memory LCD power up sequence
    //oled_ssd1357_power_up(); 

    //delay 200ms recommanded for power stabilized
    oled_ssd1357_delay(200*1000);

    //Enable write data into RAM 
    oled_ssd1357_write_cmd(0x5C);
    test_oled_fill_color();
    return RY_SUCC;
}

void oled_ssd1357_set_cursor(u8_t x, u8_t y)
{
  if ((x >= OLED_SSD1357_SIZE_X) || (y >= OLED_SSD1357_SIZE_Y))
	return;

  oled_ssd1357_write_cmd(SSD1351_CMD_WRITERAM);
  oled_ssd1357_write_cmd(SSD1351_CMD_SETCOLUMNADDRESS);
  oled_ssd1357_write_data(x);                            // Start Address
  oled_ssd1357_write_data(OLED_SSD1357_START_X + OLED_SSD1357_SIZE_X - 1);    	 // End Address (0x7F)

  oled_ssd1357_write_cmd(SSD1351_CMD_SETROWADDRESS);
  oled_ssd1357_write_data(y);                            // Start Address
  oled_ssd1357_write_data(OLED_SSD1357_START_Y + OLED_SSD1357_SIZE_Y - 1);   	 // End Address (0x7F)
  oled_ssd1357_write_cmd(SSD1351_CMD_WRITERAM);
}


static inline void fill_frame_buff(u32_t pix, u8_t color_1, u8_t color_2)
{
	frame_buffer[pix] = color_1;
	frame_buffer[pix + 1] = color_2;
	
	//LOG_INFO("frame_buffer at: 0x%x 0x%x  0x%x \r\n", pix, frame_buffer[pix], frame_buffer[pix + 1]);
}

void clear_buffer_black(void)
{
    memset(frame_buffer, 0, sizeof(frame_buffer));
}

void oled_ssd1357_fill_rgb(u8_t r, u8_t g, u8_t b)
{
#if 0
    u8_t x, y;	
	u8_t data[2];
    oled_ssd1357_set_cursor(0, 0);	
    data[0] = (r & 0xF8) | (g>>5);
    data[1] = (b >> 3) | ((g>>2) << 5);
    for (x = 0; x < OLED_SSD1357_SIZE_X; x++){
        for (y = 0; y < OLED_SSD1357_SIZE_Y; y++){
            oled_ssd1357_write_data_buff(data, 2);
        }
    }
#else

#if 1
	u8_t x, y;	
	u8_t data[2];
    data[0] = (r & 0xF8) | (g>>5);
    data[1] = (b >> 3) | ((g>>2) << 5);
    for (x = 0; x < OLED_SSD1357_SIZE_X; x++){
        for (y = 0; y < OLED_SSD1357_SIZE_Y; y++){
			u32_t pix = x * OLED_SSD1357_SIZE_Y + y;
            fill_frame_buff(pix << 1, data[0], data[1]);
        }
    }
#else
    memset(frame_buffer, 0, sizeof(frame_buffer));
#endif

    uint32_t total_size = 72 * 128 * 2;
    oled_ssd1357_set_cursor(OLED_SSD1357_START_X, OLED_SSD1357_START_Y);	
	#define MAX_QUEUE_SIZE (1024 * 4 - 1)
	uint32_t tx_size = 0;
    am_hal_gpio_out_bit_replace(25, 0);	
	do{
		u8_t* p = frame_buffer + tx_size;		
		if (total_size - tx_size >= MAX_QUEUE_SIZE){
			oled_ssd1357_write_data_buff(p, MAX_QUEUE_SIZE);
			tx_size += MAX_QUEUE_SIZE; 
		}
		else{
			oled_ssd1357_write_data_buff(p, OLED_FRAME_BUFF_SIZE - tx_size);
			tx_size += (OLED_FRAME_BUFF_SIZE - tx_size); 
		}
	}while(tx_size < total_size);

#endif
}


void update_oled(void){    
    //set disp erea
    oled_ssd1357_set_cursor(OLED_SSD1357_START_X, OLED_SSD1357_START_Y);	

    //flush data to oled
    uint32_t total_size = 72 * 128 * 2;
	#define MAX_QUEUE_SIZE (1024 * 4 - 1)
	uint32_t tx_size = 0;
    am_hal_gpio_out_bit_replace(25, 0);	
	do{
		u8_t* p = frame_buffer + tx_size;		
		if (total_size - tx_size >= MAX_QUEUE_SIZE){
			oled_ssd1357_write_data_buff(p, MAX_QUEUE_SIZE);
			tx_size += MAX_QUEUE_SIZE; 
		}
		else{
			oled_ssd1357_write_data_buff(p, OLED_FRAME_BUFF_SIZE - tx_size);
			tx_size += (OLED_FRAME_BUFF_SIZE - tx_size); 
		}
	}while(tx_size < total_size);
}

/**
 * @brief   API to init GUI module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t oled_ug7228_init(void)
{
    // setting oled DC and RES pin
    am_hal_gpio_pin_config(OLED_GPIO_DC, AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_out_enable_bit_set(OLED_GPIO_DC);
    am_hal_gpio_out_bit_replace(OLED_GPIO_DC, 1);		
		
    am_hal_gpio_pin_config(OLED_GPIO_RES, AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_out_enable_bit_set(OLED_GPIO_RES);
    am_hal_gpio_out_bit_replace(OLED_GPIO_RES, 1);
	
	am_hal_gpio_pin_config(13, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_out_enable_bit_set(13);
	am_hal_gpio_out_bit_replace(13, 1);

	am_hal_gpio_pin_config(35, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_out_enable_bit_set(35);
	am_hal_gpio_out_bit_replace(35, 1);

	am_hal_gpio_pin_config(25, AM_HAL_GPIO_OUTPUT);
	am_hal_gpio_out_enable_bit_set(25);
	am_hal_gpio_out_bit_replace(25, 1);

    ry_hal_spim_init(SPI_IDX_OLED);	
	oled_ssd1357_init();  
	
	am_hal_gpio_out_bit_replace(13, 0);
    am_hal_gpio_out_bit_replace(35, 0);
    am_hal_gpio_out_bit_replace(25, 0);
    //am_util_delay_us(100); 
    return RY_SUCC;
}


#endif /* (RY_GUI_ENABLE == TRUE) */

