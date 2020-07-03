
#ifndef AM_DEVICES_CY8C4014_H
#define AM_DEVICES_CY8C4014_H

#ifdef __cplusplus
extern "C"
{
#endif

#if 0	
	
#define AM_DEVICES_CY8C4014_PANELMAKER             0x00
#define AM_DEVICES_CY8C4014_FWVERSION              0x01
#define AM_DEVICES_CY8C4014_NUMBEROFSENSORS        0x02
#define AM_DEVICES_CY8C4014_BUTTON                 0x03
#define AM_DEVICES_CY8C4014_RESERVE0               0x04
#define AM_DEVICES_CY8C4014_RESERVE1               0x05
#define AM_DEVICES_CY8C4014_RESET                  0x06
#define AM_DEVICES_CY8C4014_BOOTCMD1           	   0x07
#define AM_DEVICES_CY8C4014_BOOTCMD2           	   0x08
#define AM_DEVICES_CY8C4014_WORKMODE           	   0x09
#endif 

#define AM_DEVICES_CY8C4014_PANELMAKER             0x00
#define AM_DEVICES_CY8C4014_FWVERSION              0x01
#define AM_DEVICES_CY8C4014_NUMBEROFSENSORS        0x02
#define AM_DEVICES_CY8C4014_BOOTCMD1           	   0x03
#define AM_DEVICES_CY8C4014_BOOTCMD2           	   0x04
#define AM_DEVICES_CY8C4014_RESET                  0x05
#define AM_DEVICES_CY8C4014_BUTTON                 0x06
#define AM_DEVICES_CY8C4014_WORKMODE           	   0x07
#define AM_DEVICES_CY8C4014_SLEEP_INTERVAL         0x08

#define AM_DEVICES_CY8C4014_DIFF_BUTTON0                 0x09
#define AM_DEVICES_CY8C4014_DIFF_BUTTON0_L               0x09
#define AM_DEVICES_CY8C4014_DIFF_BUTTON0_H               0x0A


#define AM_DEVICES_CY8C4014_DIFF_BUTTON1                 0x0B
#define AM_DEVICES_CY8C4014_DIFF_BUTTON1_L               0x0B
#define AM_DEVICES_CY8C4014_DIFF_BUTTON1_H               0x0C

#define AM_DEVICES_CY8C4014_DIFF_BUTTON2                 0x0D
#define AM_DEVICES_CY8C4014_DIFF_BUTTON2_L               0x0D
#define AM_DEVICES_CY8C4014_DIFF_BUTTON2_H               0x0E

#define AM_DEVICES_CY8C4014_DIFF_BUTTON3                 0x0F
#define AM_DEVICES_CY8C4014_DIFF_BUTTON3_L               0x0F
#define AM_DEVICES_CY8C4014_DIFF_BUTTON3_H               0x10

#define AM_DEVICES_CY8C4014_DIFF_BUTTON4                 0x11
#define AM_DEVICES_CY8C4014_DIFF_BUTTON4_L               0x11
#define AM_DEVICES_CY8C4014_DIFF_BUTTON4_H               0x12

#define AM_DEVICES_CY8C4014_DIFF_GANG               	 0x13
#define AM_DEVICES_CY8C4014_DIFF_GANG_L               	 0x13
#define AM_DEVICES_CY8C4014_DIFF_GANG_H               	 0x14

#define AM_DEVICES_CY8C4014_RAW_BUTTON0              	0x15
#define AM_DEVICES_CY8C4014_RAW_BUTTON0_L              	0x15
#define AM_DEVICES_CY8C4014_RAW_BUTTON0_H               0x16

#define AM_DEVICES_CY8C4014_RAW_BUTTON1              	0x17
#define AM_DEVICES_CY8C4014_RAW_BUTTON1_L              	0x17
#define AM_DEVICES_CY8C4014_RAW_BUTTON1_H               0x18

#define AM_DEVICES_CY8C4014_RAW_BUTTON2              	0x19
#define AM_DEVICES_CY8C4014_RAW_BUTTON2_L              	0x19
#define AM_DEVICES_CY8C4014_RAW_BUTTON2_H               0x1a

#define AM_DEVICES_CY8C4014_RAW_BUTTON3              	0x1B
#define AM_DEVICES_CY8C4014_RAW_BUTTON3_L              	0x1B
#define AM_DEVICES_CY8C4014_RAW_BUTTON3_H               0x1c

#define AM_DEVICES_CY8C4014_RAW_BUTTON4                 0x1D
#define AM_DEVICES_CY8C4014_RAW_BUTTON4_L               0x1D
#define AM_DEVICES_CY8C4014_RAW_BUTTON4_H               0x1E

#define AM_DEVICES_CY8C4014_RAW_GANG	               	0x1F
#define AM_DEVICES_CY8C4014_RAW_GANG_L               	0x1F
#define AM_DEVICES_CY8C4014_RAW_GANG_H               	0x20

/* Register Sub ADDR */
#define     VENDOR_CODE         0x00            /* PanelMaker Vendor Code, current 0x01 */
#define     FW_VERSION          0x01            /* Firmware Version */
#define     NUM_SENSOR          0x02            /* number of sensor, it is 5 for this project */
#define     BTLD_CMD1           0x03            /* Bootload Enter command register1, need to write 0xAC */
#define     BTLD_CMD2           0x04            /* Bootload Enter command register2, need to write 0x39 */
#define     RESET               0x05            /* Write 0x17 to SoftReset PSoC4 */ 
#define     BUTTON_STAT         0x06            /* Button status, Bit0~Bit4 for Button0~Button4, bit7 for Prox sensor, 0=Nactive, 1= Active*/  
#define     WORK_MODE           0x07            /* Work mode: SysActive-0x01,  Buttononly-0x02*/
#define     SLEEP_INTERVAL      0x08            /* Scan interval for ButtonOnly Mode, value: 50~250,unit:ms */
  /* debug buffer */
#define     DIFF_BUTTON0_L      0x09            /* Different count of Button0, Low  8bit */
#define     DIFF_BUTTON0_H      0x0A            /* Different count of Button0, High 8bit */
#define     DIFF_BUTTON1_L      0x0B            /* Different count of Button1, Low  8bit */
#define     DIFF_BUTTON1_H      0x0C            /* Different count of Button1, High  8bit */
#define     DIFF_BUTTON2_L      0x0D            /* Different count of Button2, Low  8bit */
#define     DIFF_BUTTON2_H      0x0E            /* Different count of Button2, High  8bit */
#define     DIFF_BUTTON3_L      0x0F            /* Different count of Button3, Low  8bit */
#define     DIFF_BUTTON3_H      0x10            /* Different count of Button3, High  8bit */
#define     DIFF_BUTTON4_L      0x11            /* Different count of Button4, Low  8bit */
#define     DIFF_BUTTON4_H      0x12            /* Different count of Button4, High  8bit */
#define     DIFF_GANG_L         0x13            /* Different count of Gang Sensor, Low  8bit */
#define     DIFF_GANG_H         0x14            /* Different count of Gang Sensor, High  8bit */
#define     RAW_BUTTON0_L       0x15            /* Raw count of Button0, Low  8bit */
#define     RAW_BUTTON0_H       0x16            /* Raw count of Button0, High 8bit */
#define     RAW_BUTTON1_L       0x17            /* Raw count of Button1, Low  8bit */
#define     RAW_BUTTON1_H       0x18            /* Raw count of Button1, High 8bit */
#define     RAW_BUTTON2_L       0x19            /* Raw count of Button2, Low  8bit */
#define     RAW_BUTTON2_H       0x1A            /* Raw count of Button2, High 8bit */
#define     RAW_BUTTON3_L       0x1B            /* Raw count of Button3, Low  8bit */
#define     RAW_BUTTON3_H       0x1C            /* Raw count of Button3, High 8bit */
#define     RAW_BUTTON4_L       0x1D            /* Raw count of Button4, Low  8bit */
#define     RAW_BUTTON4_H       0x1E            /* Raw count of Button4, High 8bit */
#define     RAW_GANG_L          0x1F            /* Raw count of Gang Sensor, Low  8bit */
#define     RAW_GANG_H          0x20            /* Raw count of Gang Sensor, High 8bit */


//#define AM_DEVICES_CY8C4014_WORKMODE           	   0x09

//*****************************************************************************
//
// Structure for holding information about the AK09911
//
//*****************************************************************************
typedef struct
{
    uint32_t ui32IOMModule;
    uint32_t ui32Address;
	uint32_t ui32IRQ;
}
am_devices_cy8c4014_t;

typedef enum {
    TP_WORKING_ACTIVE   =   0x01,
    TP_WORKING_BTN_ONLY =   0x02,
} tp_working_t;

typedef struct {
    unsigned char button;           //从下往上：bit 0 1 2 3 4，按下时对应bit为置1，未按下时对应bit为0
    unsigned char button_event;     //从下往上：bit 0 1 2 3 4，变化时置为1
    unsigned char event;            //典型动作：bit0: 常速上滑， bit1: 常速上滑，bit2: 慢速下滑，bit3: 慢速下滑
    unsigned char button_event_msk; //default 0xff, mask none
    unsigned char event_msk;        //default 0xff, mask none
    unsigned int  duration;
}touch_data_t;
extern touch_data_t g_touch_data;

extern void cy8c4011_gpio_init(void);
extern void cy8c4011_gpio_uninit(void);
extern void am_devices_cy8c4014_reset(void);

extern void cy8c4014_timer_handler(void);

extern uint32_t am_devices_cy8c4014_read_id(void);
extern void am_devices_cy8c4014_set_work_mode(uint8_t mode);

u8_t tp_get_data(void);
u8_t tp_get_origin_data(void);
void touch_get_diff(u32_t* data);
void cy8c4011_set_sleep_interval(u8_t interval);
void cy8c4011_gpio_int_init(void);
unsigned char gtouch_new_data(touch_data_t* touch_data);
void cy8c4011_set_mode(u8_t mode);
uint32_t am_devices_cy8c4014_enter_btldr(void );
void am_device_cy8c4014_clear_raw(void);



/**
 * @brief   
 *
 * @param   touch_data - Pointer to touch_data
 *
 * @return  Status  - result of new data or event
 *			          0: no new data, else: new data
 */
unsigned char gtouch_new_data(touch_data_t* touch_data);


#endif // AM_DEVICES_CY8C4014_H
