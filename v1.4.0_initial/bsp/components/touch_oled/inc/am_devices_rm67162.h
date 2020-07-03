#ifndef __RM67162_H__
#define __RM67162_H__

#define RM67162_HW_CS   1

void rm67162_init_1(void);

void rm67162_gpio_init(void);
void am_devices_rm67162_display_brightness_set(uint8_t bright);

void am_devices_rm67162_data_stream_enable(void);
void am_devices_rm67162_data_stream_continue(void);
void am_devices_rm67162_data_stream_888(uint8_t *pui8Values, uint32_t ui32NumBytes);
void am_devices_rm67162_data_write_888(uint8_t *pui8Values, uint32_t ui32NumBytes);

void am_devices_rm67162_display_normal_area(uint16_t col_start, uint16_t col_end, 
                                            uint16_t row_start, uint16_t row_end);

void am_devices_rm67162_fixed_data_888(uint32_t ui32Data, uint32_t ui32NumBytes);
                                            
void gui_hw_init(void);

void am_devices_rm67162_powerdown_with_block_delay(void);

                                        
/**
 * @brief   API to restore oled from low power, all pixel display on
 *
 * @param   None
 *
 * @return  None
 */
void oled_restore_from_low_power(void);


/**
 * @brief   API to set oled into low power, off all pixel
 *
 * @param   None
 *
 * @return  None
 */
void oled_enter_low_power(void);
void am_devices_rm67162_powerdown(void);
void am_devices_rm67162_poweron(void);

void am_devices_rm67162_displayoff(void);
void am_devices_rm67162_displayon(void);

uint32_t am_devices_rm67162_read_id(void);
void am_devices_rm67162_display_partial_area(uint16_t col_start,  uint16_t col_end, 
                                        uint16_t row_start,  uint16_t row_end,
										uint8_t *pui8Values, uint32_t ui32NumBytes);

void am_devices_rm67162_display_partial_on(void);
void am_devices_rm67162_display_range_set(uint16_t col_start, uint16_t col_end, 
                                          uint16_t row_start, uint16_t row_end);



#endif
