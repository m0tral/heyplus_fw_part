/***************************************************

 file :     AFE4404_API.c
 Author :  Dominic Kim
 Created : 2016.03.17
 brief :  Control and operation functions for TI AFE 4405

 Copyright (c) 2016 PARTRON SensorLAB. Alll rights reserved.

****************************************************/
#include <stdbool.h>
#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>

#include <string.h>

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "ry_utils.h"

#include "ppsi26x.h"
#include "agc_V3_1_19_C.h"
#include "AFE4410_API_C.h"

#ifndef USE_FIFO
#define USE_FIFO        1       // 1: enable FIFO, 0: diable FIFO
#endif

#ifndef TRUE
#define TRUE            1
#endif

#ifndef FALSE
#define FALSE           0
#endif

unsigned char Slave_address = (0x5B<<1);//SEN=0 -->0x5B;SEN=1 -->0x5A

uint8_t green_led_on_flag=0;

afe_data_struct_t afe_struct1;
afe_32_bit_data_struct_t afe_32bit_struct1;
afe_32_RAW_bit_data_struct_t afe_32bit_RAW_struct1;
afe_data_etc_t etcdata;

#if 1
    #define LEDbuffHead cbuff_hr.head
    #define LEDbuffTail cbuff_hr.tail
    #define rd_ptr      cbuff_hr.rd_ptr
#else   
    uint16_t LEDbuffHead = 0 ;
    uint16_t LEDbuffTail = 0 ;
    static uint32_t rd_ptr = 0;                
#endif

#define REGISTERS_TO_WRITE_LENGTH 20
uint32_t RegistersToWrite[REGISTERS_TO_WRITE_LENGTH];
uint32_t LocationsOfRegistersToWrite[REGISTERS_TO_WRITE_LENGTH];
uint8_t AmountOfRegistersToWrite = 0;

bool afeSettingsExecuted = FALSE;

extern unsigned char pdata_AFE4405[3];

extern uint8_t fifo_phase;

void convert32to8(int, unsigned char *);
int data_reg_convert8to32(unsigned char *);
int contol_reg_convert8to32(unsigned char *);

//unsigned char Slave_address = 0x5B<<1;
//unsigned char Slave_address = 0xB4 |0xB5;
unsigned char ackt = 0;
uint32_t liveAFERegister[130] ;

uint16_t AccXbuff[AFE_BUFF_LEN];
uint16_t AccYbuff[AFE_BUFF_LEN];
uint16_t AccZbuff[AFE_BUFF_LEN];
uint64_t AFE_SettingsBuff[AFE_BUFF_LEN];

uint64_t settingsCheckSampleQueue [2];

bool firstsample = TRUE;
uint16_t HRM_sample_count=0;
//extern uint8_t Global_Reg0x00;
//uint8_t Global_Reg0x00;
extern uint8_t greenambhzmode;

const uint32_t init_registers_25Hz[100]= { //with fifo 25Hz*4=100spl. depth is 100.

#if 0  //25Hz LED onTime 400us 
		0x01,0x2B
		,0x02,0x5E
		,0x03,0x99
		,0x04,0xD0
		,0x05,0x64
		,0x06,0x97
		,0x07,0x9D
		,0x08,0xD0
		,0x09,0x27
		,0x0A,0x5E
		,0x0B,0xD6
		,0x0C,0x109
		,0x0D,0x60
		,0x0E,0x6F
		,0x0F,0x99
		,0x10,0xA8
		,0x11,0xD2
		,0x12,0xE1
		,0x13,0x10B
		,0x14,0x11A
		,0x1D,0x13FF
		,0x36,0x60
		,0x37,0x97
		,0x43,0xD2
		,0x44,0x109
		,0x52,0x120
		,0x53,0x124
		,0x64,0x0
		,0x65,0x11B
		,0x66,0x0
		,0x67,0x11B
		,0x68,0x0
		,0x69,0x11B
		,0x6A,0x12B
		,0x6B,0x13E5
		,0x1E,0x101
		,0x1F,0x0
		,0x20,0x0
		,0x21,0x3000
		,0x22,0x30c3
		,0x23,0x124218
		,0x28,0x0
		,0x39,0x0
		,0x3A,0x0
		,0x42,0x120 //FIFO setting,0x620 Depth is 25.0x120 Depth is 5
		,0x4E,0x0
		,0x51,0x0
		,0x50,0x28
		,0x4B,0xF
		,0x3D,0x0
#elif 1  //25Hz LED onTime 200us 
    0x01,0x2B
    ,0x02,0x44
    ,0x03,0x65
    ,0x04,0x82
    ,0x05,0x4A
    ,0x06,0x63
    ,0x07,0x69
    ,0x08,0x82
    ,0x09,0x27
    ,0x0A,0x44
    ,0x0B,0x88
    ,0x0C,0xA1
    ,0x0D,0x46
    ,0x0E,0x55
    ,0x0F,0x65
    ,0x10,0x74
    ,0x11,0x84
    ,0x12,0x93
    ,0x13,0xA3
    ,0x14,0xB2
    ,0x1D,0x13FF
    ,0x36,0x46
    ,0x37,0x63
    ,0x43,0x84
    ,0x44,0xA1
    ,0x52,0xB8
    ,0x53,0xBC
    ,0x64,0x0
    ,0x65,0xB3
    ,0x66,0x0
    ,0x67,0xB3
    ,0x68,0x0
    ,0x69,0xB3
    ,0x6A,0xC3
    ,0x6B,0x13E5
    ,0x1E,0x101
    ,0x1F,0x0
    ,0x20,0x0
    ,0x21,0x3000
    ,0x22,0x30c3
    ,0x23,0x124218
    ,0x28,0x0
    ,0x39,0x0
    ,0x3A,0x0
    ,0x42,0x120 //FIFO setting,0x620 Depth is 25.0x120 Depth is 5
    ,0x4E,0x0
    ,0x51,0x0
    ,0x50,0x28
    ,0x4B,0xF
    ,0x3D,0x0
#elif 0 //25Hz LED onTime 100us 
    0x01,0x2B
    ,0x02,0x37
    ,0x03,0x4B
    ,0x04,0x5B
    ,0x05,0x3D
    ,0x06,0x49
    ,0x07,0x4F
    ,0x08,0x5B
    ,0x09,0x27
    ,0x0A,0x37
    ,0x0B,0x61
    ,0x0C,0x6D
    ,0x0D,0x39
    ,0x0E,0x48
    ,0x0F,0x4B
    ,0x10,0x5A
    ,0x11,0x5D
    ,0x12,0x6C
    ,0x13,0x6F
    ,0x14,0x7E
    ,0x1D,0x13FF
    ,0x36,0x39
    ,0x37,0x49
    ,0x43,0x5D
    ,0x44,0x6D
    ,0x52,0x84
    ,0x53,0x88
    ,0x64,0x0
    ,0x65,0x7F
    ,0x66,0x0
    ,0x67,0x7F
    ,0x68,0x0
    ,0x69,0x7F
    ,0x6A,0x8F
    ,0x6B,0x13E5
    ,0x1E,0x101
    ,0x1F,0x0
    ,0x20,0x0
    ,0x21,0x3000
    ,0x22,0x30c3
    ,0x23,0x124218
    ,0x28,0x0
    ,0x39,0x0
    ,0x3A,0x0
    ,0x42,0x120 //FIFO setting,0x620 Depth is 25.0x120 Depth is 5
    ,0x4E,0x0
    ,0x51,0x0
    ,0x50,0x28
    ,0x4B,0xF
    ,0x3D,0x0
#else   //25Hz LED onTime 50us
    0x01,0x2B
    ,0x02,0x31
    ,0x03,0x3F
    ,0x04,0x49
    ,0x05,0x37
    ,0x06,0x3D
    ,0x07,0x43
    ,0x08,0x49
    ,0x09,0x27
    ,0x0A,0x31
    ,0x0B,0x4F
    ,0x0C,0x55
    ,0x0D,0x33
    ,0x0E,0x42
    ,0x0F,0x44
    ,0x10,0x53
    ,0x11,0x55
    ,0x12,0x64
    ,0x13,0x66
    ,0x14,0x75
    ,0x1D,0x13FF
    ,0x36,0x33
    ,0x37,0x3D
    ,0x43,0x4B
    ,0x44,0x55
    ,0x52,0x7B
    ,0x53,0x7F
    ,0x64,0x0
    ,0x65,0x76
    ,0x66,0x0
    ,0x67,0x76
    ,0x68,0x0
    ,0x69,0x76
    ,0x6A,0x86
    ,0x6B,0x13E5
    ,0x1E,0x101
    ,0x1F,0x0
    ,0x20,0x0
    ,0x21,0x3000
    ,0x22,0x30c3
    ,0x23,0x124218
    ,0x28,0x0
    ,0x39,0x0
    ,0x3A,0x0
    ,0x42,0x120 //FIFO setting,0x620 Depth is 25.0x120 Depth is 5
    ,0x4E,0x0
    ,0x51,0x0
    ,0x50,0x28
    ,0x4B,0xF
    ,0x3D,0x0
#endif

};
//0x28 , 0x501539//0x402d00
void AFE4410_Internal_25Hz(void)
{
    uint32_t i;
#if USE_FIFO

    AFE4410_enableWriteFIFO();
    for(i=0; i<100;) {
        AFE4410_writeRegWithoutWriteEnable(init_registers_25Hz[i],init_registers_25Hz[i+1]);
        i=i+2;
    }
		//AFE4410_writeRegWithoutWriteEnable(0x42,0x120);//
    AFE4410_enableReadFIFO();
#else
    AFE4410_enableWrite();
    for(i=0; i<100;) {
        AFE4410_writeRegWithoutWriteEnable(init_registers_25Hz[i],init_registers_25Hz[i+1]);
        i=i+2;
    }
    AFE4410_writeRegWithoutWriteEnable(0x42,0);//

    AFE4410_enableRead();
#endif
}


void AFE4490_Internal_250Hz_ecg(void)
{
	//Below Is initial of AFE4900 250HZ with PTT mode.
	//250Hz -->500Hz, modify (0x1D	0xFF;0x6B	0xE5)  
	AFE4410_enableWriteFIFO();

	AFE4410_writeRegWithoutWriteEnable(0x1, 0x2B); 
	AFE4410_writeRegWithoutWriteEnable(0x2, 0x32); 
	AFE4410_writeRegWithoutWriteEnable(0x3, 0x41); 
	AFE4410_writeRegWithoutWriteEnable(0x4, 0x4C); 
	AFE4410_writeRegWithoutWriteEnable(0x5, 0x38); 
	AFE4410_writeRegWithoutWriteEnable(0x6, 0x3F); 
	AFE4410_writeRegWithoutWriteEnable(0x7, 0x45); 
	AFE4410_writeRegWithoutWriteEnable(0x8, 0x4C); 
	AFE4410_writeRegWithoutWriteEnable(0x9, 0x27); 
	AFE4410_writeRegWithoutWriteEnable(0xA, 0x32); 
	AFE4410_writeRegWithoutWriteEnable(0xB, 0x52); 
	AFE4410_writeRegWithoutWriteEnable(0xC, 0x59); 
	AFE4410_writeRegWithoutWriteEnable(0xD, 0x34); 
	AFE4410_writeRegWithoutWriteEnable(0xE, 0x43); 
	AFE4410_writeRegWithoutWriteEnable(0xF, 0x45); 
	AFE4410_writeRegWithoutWriteEnable(0x10, 0x54 ); 
	AFE4410_writeRegWithoutWriteEnable(0x11, 0x56 ); 
	AFE4410_writeRegWithoutWriteEnable(0x12, 0x65 ); 
	AFE4410_writeRegWithoutWriteEnable(0x13, 0x67 ); 
	AFE4410_writeRegWithoutWriteEnable(0x14, 0x76 ); 
	AFE4410_writeRegWithoutWriteEnable(0x1D, 0x1FF); 
	AFE4410_writeRegWithoutWriteEnable(0x36, 0x34     );
	AFE4410_writeRegWithoutWriteEnable(0x37, 0x3F     );
	AFE4410_writeRegWithoutWriteEnable(0x43, 0x4E     );
	AFE4410_writeRegWithoutWriteEnable(0x44, 0x59     );
	AFE4410_writeRegWithoutWriteEnable(0x52, 0x7C     );
	AFE4410_writeRegWithoutWriteEnable(0x53, 0x7C     );
	AFE4410_writeRegWithoutWriteEnable(0x64, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x65, 0x78     );
	AFE4410_writeRegWithoutWriteEnable(0x66, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x67, 0x78     );
	AFE4410_writeRegWithoutWriteEnable(0x68, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x69, 0x78     );
	AFE4410_writeRegWithoutWriteEnable(0x6A, 0x83     );
	AFE4410_writeRegWithoutWriteEnable(0x6B, 0x1E5    );
	AFE4410_writeRegWithoutWriteEnable(0x1E, 0x101    );
	AFE4410_writeRegWithoutWriteEnable(0x1F, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x20, 0x3      );
	AFE4410_writeRegWithoutWriteEnable(0x21, 0x3      );
	AFE4410_writeRegWithoutWriteEnable(0x22, 0x0000CC );
	AFE4410_writeRegWithoutWriteEnable(0x23, 0x124218 );
	AFE4410_writeRegWithoutWriteEnable(0x29, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x31, 0x20     );
	AFE4410_writeRegWithoutWriteEnable(0x34, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x35, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x39, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x3A, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x3D, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x3E, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x42, 0xEE2    );//250Hz ,0x42 set 0xEC2 for 120 FIFO depth. period is 60. T=60*4ms=240ms.so interrupt time is 240ms
	AFE4410_writeRegWithoutWriteEnable(0x45, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x46, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x47, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x48, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x4B, 0x0      );
	AFE4410_writeRegWithoutWriteEnable(0x4E, 0x24c004 );//0x4 //0x4e 0x24c004 , 0x62 0xE00000 ,two rigsitor contrl "DC lead off" setting
	AFE4410_writeRegWithoutWriteEnable(0x50, 0x8      );  
	AFE4410_writeRegWithoutWriteEnable(0x51, 0x0      );   
	AFE4410_writeRegWithoutWriteEnable(0x54, 0x0      );   
	AFE4410_writeRegWithoutWriteEnable(0x57, 0x0      );   
	AFE4410_writeRegWithoutWriteEnable(0x58, 0x0      );   
	AFE4410_writeRegWithoutWriteEnable(0x61, 0x80000  );   
	AFE4410_writeRegWithoutWriteEnable(0x62, 0xe00000 );//0x800000  
	AFE4410_enableReadFIFO();
}


int contol_reg_convert8to32(unsigned char *preaddata)
{
    int data = 0;
    data = ((preaddata[0] << 16) | (preaddata[1] << 8) | (preaddata[2]));
    return data;
}

int data_reg_convert8to32(unsigned char *preaddata)
{
    int data = 0;
    char sign = 0;
    data = ((preaddata[0] << 16) | (preaddata[1] << 8) | (preaddata[2]));
    sign = (data & 0x800000)>>23;
    if(sign == 1)
        data = (data|0xFF000000);
    return data;
}

void convert32to8(int writedata, unsigned char *pwritedata)
{
    int data = 0;
    data = (writedata & 0xFF0000)>>16;
    pwritedata[0] = (unsigned char)data;
    data = (writedata & 0x00FF00)>>8;
    pwritedata[1] = (unsigned char)data;
    data = (writedata & 0x0000FF);
    pwritedata[2] = (unsigned char)data;
}

void AFE4410_disableAFE(void)
{
    AFE4410_writeRegWithoutWriteEnable(30, 0x000000); //Timer Enable<8> , ADC averages <3:0>
}
void AFE4410_enableInternalTimer (void)
{
    uint32_t reg0x1E = AFE4410_getRegisterFromLiveArray(0x1E) | 0x000100;
    AFE4410_writeRegWithWriteEnable(30, reg0x1E); //Timer Enable<8> , ADC averages <3:0>
}

void AFE4410_powerdownAFE (void)
{
    uint32_t reg0x1E = AFE4410_getRegisterFromLiveArray(0x23) | 0x000001;
    AFE4410_writeRegWithWriteEnable(0x23, reg0x1E); //Timer Enable<8> , ADC averages <3:0>
}

void AFE4410_enableRead (void)
{
    uint8_t data;
    //data = Global_Reg0x00 & 0xFE;
    //Global_Reg0x00 = (data|0x01);
    I2C_writeReg(0,0x21);
    //zPrintf(USART1,"Enable Read mode %x \n ",Global_Reg0x00);
}
void AFE4410_enableWrite (void)
{
    uint8_t data;
    //data = Global_Reg0x00 & 0xFE;
    //Global_Reg0x00 = (data | 0x00);
    I2C_writeReg(0,0x20);
    //zPrintf(USART1,"Enable Write mode %x \n ",Global_Reg0x00);
}

void AFE4410_enableReadFIFO (void)
{
    uint8_t data;
    //data = Global_Reg0x00 & 0xFE;
    //Global_Reg0x00 = (data|0x01);
    I2C_writeReg(0,0x61);
    //zPrintf(USART1,"Enable Read mode %x \n ",Global_Reg0x00);
}
void AFE4410_enableWriteFIFO (void)
{
    uint8_t data;
    //data = Global_Reg0x00 & 0xFE;
    //Global_Reg0x00 = (data | 0x00);
    I2C_writeReg(0,0x60);
    //zPrintf(USART1,"Enable Write mode %x \n ",Global_Reg0x00);
}

uint32_t AFE4410_readRegWithoutReadEnable (uint8_t reg)
{
    return(I2C_readReg(reg));
}
uint32_t AFE4410_readRegWithReadEnable (uint8_t reg)
{
#if USE_FIFO
    AFE4410_enableReadFIFO();
#else
    AFE4410_enableRead();
#endif

    return(I2C_readReg(reg));
}

void AFE4410_writeRegWithWriteEnable(uint8_t reg, uint32_t registerValue)
{
#if USE_FIFO
    AFE4410_enableWriteFIFO();
#else
    AFE4410_enableWrite();
#endif
    I2C_writeReg(reg,registerValue);
    liveAFERegister[reg] = registerValue;
}

void AFE4410_writeRegWithoutWriteEnable(uint8_t reg, uint32_t registerValue)
{
    I2C_writeReg(reg,registerValue);
    liveAFERegister[reg] = registerValue;
}


void AFE4410_retrieveRawAFEValues (afe_32_RAW_bit_data_struct_t *afe_RAW_32bitstruct , afe_32_bit_data_struct_t *afe_32bitstruct , afe_data_struct_t *afe_struct )
{
    uint32_t tempVal = 0;
    tempVal = AFE4410_readRegWithoutReadEnable(44);
    afe_RAW_32bitstruct->afeMeasLED1= (int32_t) tempVal;
    afe_32bitstruct->afeMeasLED1 = afeConvP32(tempVal);
    afe_struct->afeMeasLED1= afeConvP16(tempVal);

    tempVal = AFE4410_readRegWithoutReadEnable(45);
    afe_RAW_32bitstruct->afeMeasALED1 =  (int32_t) tempVal;
    afe_32bitstruct->afeMeasALED1 = afeConvP32(tempVal);
    afe_struct->afeMeasALED1 =  afeConvP16(tempVal);

    tempVal = AFE4410_readRegWithoutReadEnable(42);
    afe_RAW_32bitstruct->afeMeasLED2 = (int32_t) tempVal;
    afe_32bitstruct->afeMeasLED2 = afeConvP32(tempVal);
    afe_struct->afeMeasLED2 = afeConvP16(tempVal);

    tempVal = AFE4410_readRegWithoutReadEnable(43);
    afe_RAW_32bitstruct->afeMeasALED2 = (int32_t) tempVal;
    afe_32bitstruct->afeMeasALED2 = afeConvP32(tempVal);
    afe_struct->afeMeasALED2 = afeConvP16(tempVal);
}

/*
void AFE4410_retrieveRawAFEValuesFIFO (afe_32_RAW_bit_data_struct_t *afe_RAW_32bitstruct , afe_32_bit_data_struct_t *afe_32bitstruct , afe_data_struct_t *afe_struct )
{
    uint32_t tempVal = 0;
    tempVal = AFE4410_readRegWithoutReadEnable(44);
    afe_RAW_32bitstruct->afeMeasLED1= (int32_t) tempVal;
    afe_32bitstruct->afeMeasLED1 = afeConvP32(tempVal);
    afe_struct->afeMeasLED1= afeConvP16(tempVal);

    tempVal = AFE4410_readRegWithoutReadEnable(45);
    afe_RAW_32bitstruct->afeMeasALED1 =  (int32_t) tempVal;
    afe_32bitstruct->afeMeasALED1 = afeConvP32(tempVal);
    afe_struct->afeMeasALED1 =  afeConvP16(tempVal);

    tempVal = AFE4410_readRegWithoutReadEnable(42);
    afe_RAW_32bitstruct->afeMeasLED2 = (int32_t) tempVal;
    afe_32bitstruct->afeMeasLED2 = afeConvP32(tempVal);
    afe_struct->afeMeasLED2 = afeConvP16(tempVal);

    tempVal = AFE4410_readRegWithoutReadEnable(43);
    afe_RAW_32bitstruct->afeMeasALED2 = (int32_t) tempVal;
    afe_32bitstruct->afeMeasALED2 = afeConvP32(tempVal);
    afe_struct->afeMeasALED2 = afeConvP16(tempVal);
}
*/

void AFE4410_setRf_Default(uint16_t rfValueInKiloOhms)
{
    uint8_t readvalue=0;
    //Disable Sepgain
    AFE4410_addWriteCommandToQueue(0x20,0x0000);

    readvalue = AFE4410_getRegisterFromLiveArray(0x21);
    switch(rfValueInKiloOhms) {
        case 1:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0000;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 2:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0001;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 3:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0002;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 4:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0003;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 5:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0004;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 6:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0005;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 7:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0006;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);

            break;
        case 8:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0007;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);

            break;
        case 9:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0040;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
    }
    AFE4410_addWriteCommandToQueue(0x21,readvalue);

}


void AFE4410_directSetRf_Default(uint16_t rfValueInKiloOhms)
{
    uint8_t readvalue=0;


    //Disable Sepgain
    
    //void AFE4410_writeRegWithoutWriteEnable(uint8_t reg, uint32_t registerValue)
    AFE4410_writeRegWithoutWriteEnable(0x20,0x0000);

    readvalue = AFE4410_getRegisterFromLiveArray(0x21);
    switch(rfValueInKiloOhms) {
        case 1:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0000;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 2:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0001;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 3:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0002;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 4:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0003;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 5:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0004;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 6:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0005;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 7:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0006;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);

            break;
        case 8:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0007;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);

            break;
        case 9:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFB8;
            readvalue = readvalue | 0x0040;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
    }
    AFE4410_writeRegWithoutWriteEnable(0x21,readvalue);

}


void AFE4410_setRf_ENSEPGAIN(uint8_t rfNum, uint16_t rfValueInKiloOhms)
{
    uint32_t readValue = 0;
    uint8_t registerNum = 0;
    if(rfNum == 1)
        registerNum = 0x21;
    else if (rfNum == 2)
        registerNum = 0x20;

    //Enable Sepgain
    readValue = AFE4410_getRegisterFromLiveArray(0x20);
    AFE4410_addWriteCommandToQueue(0x20,readValue|0x8000);

    readValue = AFE4410_getRegisterFromLiveArray(registerNum);
    //zPrintf(USART1,"[rf]registerNum : %d, readValue : %d\n",registerNum,readValue);

    switch(rfValueInKiloOhms) {
        case 500://1://500
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0000;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 250://2://250
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0001;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 100://3://100
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0002;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 50://4://50
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0003;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 25://5://25
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0004;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 10:// 6://10
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0005;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 1000://7://1000
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0006;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);

            break;
        case 2000:// 8://2000
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0007;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);

            break;
        case 1500://9:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0040;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
    }
    //zPrintf(USART1,"[rf]registerNum : %d, readValue : %d\n",registerNum,readValue);
    AFE4410_addWriteCommandToQueue(registerNum,readValue);

}


void AFE4410_directSetRf_ENSEPGAIN(uint8_t rfNum, uint16_t rfValueInKiloOhms)
{
    uint32_t readValue = 0;
    uint8_t registerNum = 0;
    if(rfNum == 1)
        registerNum = 0x21;
    else if (rfNum == 2)
        registerNum = 0x20;

    //Enable Sepgain
    readValue = AFE4410_getRegisterFromLiveArray(0x20);
    AFE4410_writeRegWithoutWriteEnable(0x20,readValue|0x8000);

    readValue = AFE4410_getRegisterFromLiveArray(registerNum);
    //zPrintf(USART1,"[rf]registerNum : %d, readValue : %d\n",registerNum,readValue);

    switch(rfValueInKiloOhms) {
        case 500://1://500
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0000;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 250://2://250
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0001;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 100://3://100
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0002;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 50://4://50
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0003;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 25://5://25
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0004;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 10:// 6://10
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0005;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 1000://7://1000
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0006;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);

            break;
        case 2000:// 8://2000
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0007;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);

            break;
        case 1500://9:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFB8;
            readValue = readValue | 0x0040;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
    }
    //zPrintf(USART1,"[rf]registerNum : %d, readValue : %d\n",registerNum,readValue);
    AFE4410_writeRegWithoutWriteEnable(registerNum,readValue);
}

void AFE4410_setRf_ENSEPGAIN_4(uint8_t rfNum, uint16_t rfValueInKiloOhms)
{
    uint32_t readValue = 0;
    uint8_t registerNum = 0;

    switch(rfNum) {
        case 1:
            registerNum = 0x21;
            break; //LED1
        case 2:
            registerNum = 0x20;
            break; //LED2
        case 3:
            registerNum = 0x1F;
            break; //LED3, AMB 1
        case 4:
            registerNum = 0x1F;
            break; //LED3, AMB 1
    }

    //Disable Sepgain
    //AFE4405_writeRegWithWriteEnable(0x20,0x0000);

    readValue = AFE4410_getRegisterFromLiveArray(registerNum);
    if(rfNum== 1|rfNum== 2) {
        switch(rfValueInKiloOhms) {
            case 1: //500
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0000;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);

                //if(rfNum==1)

                //zPrintf(USART1,"gain Sep4 Led1 500k Register a = %x, v = %x \n ",registerNum,readValue);
                //else

                //zPrintf(USART1,"gain Sep4 Led2 500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 2: //250
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0001;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 250k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 250k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 3://100
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0002;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 100k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 4: //50
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0003;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 100k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 5://25
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0004;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 25k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 25k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 6://10
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0005;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 10k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 10k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 7://1000
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0006;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 1000k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 1000k Register a = %x, v = %x \n ",registerNum,readValue);
                //
                break;
            case 8://2000
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0007;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 2000k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //zPrintf(USART1,"gain Sep4 Led2 2000k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 9://1500
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0040;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //zPrintf(USART1,"gain Sep4 Led1 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //zPrintf(USART1,"gain Sep4 Led2 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
        }
    }

    else if(rfNum==3) {
        switch(rfValueInKiloOhms) {
            case 1: //500
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0000;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 2: //250
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0001;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 250k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 3://100
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0002;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 4: //50
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0003;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 50k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 5://25
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0004;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 25k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 6://10
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0005;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 10k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 7://1000
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0006;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 1000k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 8://2000
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0007;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_writeRegWithWriteEnable(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 2000k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 9://1500
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0040;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
        }

    }

    else if(rfNum==4) {
        switch(rfValueInKiloOhms) {
            case 1: //500
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0000;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 2: //250
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0100;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 250k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 3://100
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0200;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 4: //50
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0300;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 50k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 5://25
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0400;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 25k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 6://10
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0500;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 10k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 7://1000
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0600;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 1000k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 8://2000
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0700;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 2000k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 9://1500
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x4000;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
        }
    }


    AFE4410_addWriteCommandToQueue(registerNum,readValue);

}

void AFE4410_directSetRf_ENSEPGAIN_4(uint8_t rfNum, uint16_t rfValueInKiloOhms)
{
    uint32_t readValue = 0;
    uint8_t registerNum = 0;

    switch(rfNum) {
        case 1:
            registerNum = 0x21;
            break; //LED1
        case 2:
            registerNum = 0x20;
            break; //LED2
        case 3:
            registerNum = 0x1F;
            break; //LED3, AMB 1
        case 4:
            registerNum = 0x1F;
            break; //LED3, AMB 1
    }
    //Disable Sepgain
    //AFE4405_writeRegWithWriteEnable(0x20,0x0000);

    readValue = AFE4410_getRegisterFromLiveArray(registerNum);
    if(rfNum== 1|rfNum== 2) {
        switch(rfValueInKiloOhms) {
            case 1: //500
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0000;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);

                //if(rfNum==1)

                //zPrintf(USART1,"gain Sep4 Led1 500k Register a = %x, v = %x \n ",registerNum,readValue);
                //else

                //zPrintf(USART1,"gain Sep4 Led2 500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 2: //250
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0001;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 250k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 250k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 3://100
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0002;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 100k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 4: //50
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0003;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 100k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 5://25
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0004;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 25k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 25k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 6://10
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0005;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 10k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 10k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 7://1000
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0006;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 1000k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //  zPrintf(USART1,"gain Sep4 Led2 1000k Register a = %x, v = %x \n ",registerNum,readValue);
                //
                break;
            case 8://2000
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0007;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //  zPrintf(USART1,"gain Sep4 Led1 2000k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //zPrintf(USART1,"gain Sep4 Led2 2000k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 9://1500
                //zPrintf(USART1,"read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"read val 2 = %x  \n ",readValue);
                readValue = readValue & 0x7FB8;
                //zPrintf(USART1,"read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0040;
                //zPrintf(USART1,"read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //if(rfNum==1)
                //zPrintf(USART1,"gain Sep4 Led1 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                //else
                //zPrintf(USART1,"gain Sep4 Led2 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
        }
    }

    else if(rfNum==3) {
        switch(rfValueInKiloOhms) {
            case 1: //500
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0000;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 2: //250
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0001;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 250k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 3://100
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0002;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 4: //50
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0003;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 50k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 5://25
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0004;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 25k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 6://10
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0005;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 10k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 7://1000
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0006;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 1000k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 8://2000
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0007;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_writeRegWithWriteEnable(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 2000k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 9://1500
                //zPrintf(USART1,"3 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"3 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xFFB8;
                //zPrintf(USART1,"3 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0040;
                //zPrintf(USART1,"3 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led3 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
        }

    }

    else if(rfNum==4) {
        switch(rfValueInKiloOhms) {
            case 1: //500
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0000;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 2: //250
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0100;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 250k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 3://100
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0200;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 100k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 4: //50
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0300;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 50k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 5://25
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0400;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 25k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 6://10
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0500;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 10k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
            case 7://1000
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0600;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 1000k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 8://2000
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x0700;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 2000k Register a = %x, v = %x \n ",registerNum,readValue);

                break;
            case 9://1500
                //zPrintf(USART1,"4 read val 1 = %x  \n ",readValue);
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                //zPrintf(USART1,"4 read val 2 = %x  \n ",readValue);
                readValue = readValue & 0xB8FF;
                //zPrintf(USART1,"4 read val 3 = %x  \n ",readValue);
                readValue = readValue | 0x4000;
                //zPrintf(USART1,"4 read val 4 = %x  \n ",readValue);
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                //zPrintf(USART1,"gain Sep4 Led4 1500k Register a = %x, v = %x \n ",registerNum,readValue);
                break;
        }

    }
    AFE4410_writeRegWithoutWriteEnable(registerNum,readValue);

}

uint16_t AFE4410_GetRF(uint8_t rfNum)
{
    uint8_t Gain_Mode=0;
    uint16_t gain_Array[5];
    Gain_Mode =GetRFGain_Mode();


    uint16_t possibleRf[9] = {500,250,100,50,25,10,1000,2000,1500};

    uint8_t readRegister_0x21=0,  LSB_0x21=0,  MSB_0x21=0;
    uint8_t readRegister_0x20=0, LSB_0x20=0,  MSB_0x20=0;
    uint16_t readRegister_0x1F =0;
    uint8_t Sep2_LSB_0x1F=0,  Sep2_MSB_0x1F=0;
    uint16_t Sep3_LSB_0x1F=0, Sep3_MSB_0x1F=0;

    uint8_t Gain_Def_Val=0;
    uint8_t Gain_Sep_val1=0,  Gain_Sep_val2=0;
    uint8_t Gain_Sep4_val1 =0,  Gain_Sep4_val2 =0,  Gain_Sep4_val3 =0,  Gain_Sep4_val4 =0;

    switch(Gain_Mode) {
        case 1 :
            readRegister_0x20 = AFE4410_getRegisterFromLiveArray(0x20);

            LSB_0x20 = readRegister_0x20 & 0x07;
            MSB_0x20 = readRegister_0x20 & 0x40;

            Gain_Sep_val2 = (LSB_0x20 | (MSB_0x20>>3));  // LED2, LED3
            readRegister_0x21 = AFE4410_getRegisterFromLiveArray(0x21);

            LSB_0x21 = readRegister_0x21 & 0x07;
            MSB_0x21 = readRegister_0x21 & 0x40;

            Gain_Sep_val1 = LSB_0x21 | (MSB_0x21>>3); // LED1, Amb 1

            gain_Array[0] = 1;
            gain_Array[1] = possibleRf[Gain_Sep_val1];
            gain_Array[2] = possibleRf[Gain_Sep_val2];
            return gain_Array[rfNum];

            break;

        case 2 :

            readRegister_0x20 = AFE4410_getRegisterFromLiveArray(0x20);

            LSB_0x20 = readRegister_0x20 & 0x07;
            MSB_0x20 = readRegister_0x20 & 0x40;

            Gain_Sep4_val2 = LSB_0x20 |(MSB_0x20>>3); //LED2
            readRegister_0x21 = AFE4410_getRegisterFromLiveArray(0x21);

            LSB_0x21 = readRegister_0x21 & 0x07;
            MSB_0x21 = readRegister_0x21 & 0x40;

            Gain_Sep4_val1 = LSB_0x21 |(MSB_0x21>>3); //LED1

            readRegister_0x1F = AFE4410_getRegisterFromLiveArray(0x1F);

            Sep3_LSB_0x1F = readRegister_0x1F & 0x700;
            Sep3_MSB_0x1F = readRegister_0x1F & 0x4000;

            Gain_Sep4_val4 = (Sep3_LSB_0x1F>>8) | (Sep3_MSB_0x1F>>11); // Ambient 1


            Sep2_LSB_0x1F = readRegister_0x1F & 0x07;
            Sep2_MSB_0x1F = readRegister_0x1F & 0x40;

            Gain_Sep4_val3 =  Sep2_LSB_0x1F | (Sep2_MSB_0x1F>>3); // LED3 /Ambient 2

            gain_Array[0] = 2;
            gain_Array[1] = possibleRf[Gain_Sep4_val1];
            gain_Array[2] = possibleRf[Gain_Sep4_val2];
            gain_Array[3] = possibleRf[Gain_Sep4_val3];
            gain_Array[4] = possibleRf[Gain_Sep4_val4];
            return gain_Array[rfNum];
            break;

        case 3 :
            readRegister_0x21 = AFE4410_getRegisterFromLiveArray(0x21);

            LSB_0x21 = readRegister_0x21 & 0x07;
            MSB_0x21 = readRegister_0x21 & 0x40;

            Gain_Def_Val = LSB_0x21 | (MSB_0x21>>3);


            gain_Array[0] = 3;
            gain_Array[1] = possibleRf[Gain_Def_Val];

            return gain_Array[rfNum];
            break;
    }
}

uint8_t GetRFGain_Mode(void)
{
    uint16_t ModeRegister_0x20=0;
    uint32_t ModeRegister_0x23=0;
    uint8_t Gain_Mode=0;

    ModeRegister_0x20= AFE4410_getRegisterFromLiveArray(0x20);
    ModeRegister_0x23= AFE4410_getRegisterFromLiveArray(0x23);


    if((ModeRegister_0x20 & 0x8000)>>15) {
        Gain_Mode=1;
    } else if((ModeRegister_0x23 & 0x8000)>>15) {
        Gain_Mode=2;
    } else {
        Gain_Mode=3;
    }

    return Gain_Mode;
}

void AFE4410_setCf_Default(uint16_t cfValue)
{
    uint8_t readvalue=0;
    //Disable Sepgain
    AFE4410_addWriteCommandToQueue(0x20,0x0000);
    readvalue = AFE4410_getRegisterFromLiveArray(0x21);
    switch(cfValue) {
        case 1:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0000;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 2:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0008;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 3:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0010;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 4:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0018;
            AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 5:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0020;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 6:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0028;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 7:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0030;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
        case 8:
            //readvalue = AFE4410_getRegisterFromLiveArray(0x21);
            readvalue = readvalue & 0xFFC7;
            readvalue = readvalue | 0x0038;
            //AFE4410_addWriteCommandToQueue(0x21,readvalue);
            break;
    }
    AFE4410_addWriteCommandToQueue(0x21,readvalue);

}

void AFE4410_setCf_ENSEPGAIN(uint8_t cfNum, uint16_t cfValue)
{
    uint32_t readValue = 0;
    uint8_t registerNum = 0;
    if(cfNum == 1)
        registerNum = 0x21;
    else if (cfNum == 2)
        registerNum = 0x20;

    //Enable Sepgain
    readValue = AFE4410_getRegisterFromLiveArray(0x20);
    AFE4410_addWriteCommandToQueue(0x20,readValue|0x8000);


    readValue = AFE4410_getRegisterFromLiveArray(registerNum);
    //zPrintf(USART1,"[cf]registerNum : %d, readValue : %d\n",registerNum,readValue);
    switch(cfValue) {
        case 1:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0000;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 2:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0008;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 3:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0010;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 4:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0018;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 5:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0020;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 6:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0028;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);
            break;
        case 7:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0030;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);

            break;
        case 8:
            //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
            readValue = readValue & 0xFFC7;
            readValue = readValue | 0x0038;
            //AFE4410_addWriteCommandToQueue(registerNum,readValue);

            break;
    }
    AFE4410_addWriteCommandToQueue(registerNum,readValue);
    //zPrintf(USART1,"[cf]registerNum : %d, readValue : %d\n",registerNum,readValue);
}

void AFE4410_setCf_ENSEPGAIN_4(uint8_t cfNum, uint16_t cfValue)
{


    uint32_t readValue = 0;
    uint8_t registerNum = 0;

    switch(cfNum) {
        case 1:
            registerNum = 0x21;
            break; //LED1
        case 2:
            registerNum = 0x20;
            break; //LED2
        case 3:
            registerNum = 0x1F;
            break; //LED3
        case 4:
            registerNum = 0x1F;
            break; //AMB1
    }

    //Disable Sepgain
    //AFE4405_writeRegWithWriteEnable(0x20,0x0000);

    readValue = AFE4410_getRegisterFromLiveArray(registerNum);

    if(cfNum== 1|cfNum== 2) {
        switch(cfValue) {
            case 1: //5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0000;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 2: //2.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0008;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 3://10pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0010;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 4: //7.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0018;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 5://20pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0020;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 6://17.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0028;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 7://25 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0030;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 8://22.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0038;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;

        }
    }

    else if(cfNum==3) { //LED 3
        switch(cfValue) {
            case 1: //5 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0000;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 2: //2.5 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0008;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 3://10 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0010;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 4: //7.5 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0018;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 5://20 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0020;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 6://17.5 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0028;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 7://25 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0030;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 8://22.5 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0x7FC7;
                readValue = readValue | 0x0038;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;

        }

    }

    else if(cfNum==4) {

        switch(cfValue) {
            case 1: //5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x0000;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 2: //2.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x0800;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 3://10 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x1000;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 4: //7.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x1800;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 5://20 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x2000;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 6://17.5pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x2800;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 7://25 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x3000;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
            case 8://22.5 pF
                //readValue = AFE4410_getRegisterFromLiveArray(registerNum);
                readValue = readValue & 0xC7FF;
                readValue = readValue | 0x3800;
                //AFE4410_addWriteCommandToQueue(registerNum,readValue);
                break;
        }

    }

    AFE4410_addWriteCommandToQueue(registerNum,readValue);

}

uint16_t AFE4410_GetCF(uint8_t cfNum)
{
    uint8_t Gain_Mode=0;
    uint16_t Cf_Array[6]=
    {0,};
    Gain_Mode =GetRFGain_Mode();


    uint16_t possibleCf[8] = {50,25,100,75,200,175,250,225};

    uint8_t readRegister_0x21=0;
    uint8_t readRegister_0x20=0;
    uint16_t readRegister_0x1F =0;
    uint8_t Sep2_0x1F=0;
    uint16_t Sep3_0x1F=0;

    uint8_t Cf_Def_Val=0;
    uint8_t Cf_Sep_val1=0,  Cf_Sep_val2=0;
    uint8_t Cf_Sep4_val1 =0,  Cf_Sep4_val2 =0,  Cf_Sep4_val3 =0,  Cf_Sep4_val4 =0;
    switch(Gain_Mode) {
        case 1 :
            readRegister_0x20 = AFE4410_getRegisterFromLiveArray(0x20);

            readRegister_0x20 = readRegister_0x20 & 0x38;

            Cf_Sep_val2 = readRegister_0x20>>3;  // LED2, LED3

            readRegister_0x21 = AFE4410_getRegisterFromLiveArray(0x21);

            readRegister_0x21 = readRegister_0x21 & 0x38;

            Cf_Sep_val1 = readRegister_0x21>>3; // LED1, Amb 1


            Cf_Array[0] = 1;
            Cf_Array[1] = possibleCf[Cf_Sep_val1];
            Cf_Array[2] = possibleCf[Cf_Sep_val2];
            return Cf_Array[cfNum];
            break;

        case 2 :

            readRegister_0x20 = AFE4410_getRegisterFromLiveArray(0x20);

            readRegister_0x20 = readRegister_0x20 & 0x38;

            Cf_Sep4_val2 = readRegister_0x20>>3; //LED2

            readRegister_0x21 = AFE4410_getRegisterFromLiveArray(0x21);

            readRegister_0x21 = readRegister_0x21 & 0x38;

            Cf_Sep4_val1 = readRegister_0x21>>3; //LED1

            readRegister_0x1F = AFE4410_getRegisterFromLiveArray(0x1F);

            //zPrintf(USART1,"CF Read 0x1f = %x \n",readRegister_0x1F);

            Sep3_0x1F = readRegister_0x1F & 0x3800;
            Cf_Sep4_val4 = Sep3_0x1F>>11; // Ambient 1

            Sep2_0x1F = readRegister_0x1F & 0x38;
            Cf_Sep4_val3 =  Sep2_0x1F>>3; // LED3 /Ambient 2

            //zPrintf(USART1,"CF Read Value = %x \n",Cf_Sep4_val3);

            Cf_Array[0] = 2;
            Cf_Array[1] = possibleCf[Cf_Sep4_val1];
            Cf_Array[2] = possibleCf[Cf_Sep4_val2];
            Cf_Array[3] = possibleCf[Cf_Sep4_val3];
            Cf_Array[4] = possibleCf[Cf_Sep4_val4];

            return Cf_Array[cfNum];
            break;

        case 3 :
            readRegister_0x21 = AFE4410_getRegisterFromLiveArray(0x21);

            readRegister_0x21 = readRegister_0x21 & 0x38;

            Cf_Def_Val = readRegister_0x21>>3;


            Cf_Array[0] = 3;
            Cf_Array[1] = possibleCf[Cf_Def_Val];

            return Cf_Array[cfNum];

            break;
    }

}

int16_t AFE4410_GetOffsetDAC(uint8_t channel)
{
    uint32_t readRegister_0x3A=0;
    uint32_t readRegister_0x3E=0;
    readRegister_0x3A= AFE4410_getRegisterFromLiveArray(0x3A);
    readRegister_0x3E= AFE4410_getRegisterFromLiveArray(0x3E);

    uint8_t OFFDAC_MSB=0,OFFDAC_MID=0,OFFDAC_LSB=0;

    uint8_t CurrentWithoutSign = 0x0;
    int16_t offsetCurrent=0;

    uint16_t validCurrents[64] = {0,25,50,75,100,125,150,175,200,225,250,275,300,325,350,375,400,425,450,475,500,525,550,575,600,
                                  625,650,675,700,725,750,775,800,825,850,875,900,925,950,975,1000,1025,1050,1075,1100,1125,1150,1175,1200,1225,1250,1275,
                                  1300,1325,1350,1375,1400,1425,1450,1475,1500,1525,1550,1575
                                 };
    switch(channel) {
        //LED1
        case 1:
            OFFDAC_MID = (readRegister_0x3A>>5) &0x0F;
            OFFDAC_LSB = (readRegister_0x3E>>2) & 0x01;
            OFFDAC_MSB = (readRegister_0x3E>>2) & 0x02;

            CurrentWithoutSign =  (OFFDAC_MSB<<4) | (OFFDAC_MID<<1) |OFFDAC_LSB;

            offsetCurrent= offsetCurrent |(int16_t)validCurrents[CurrentWithoutSign];

            if((readRegister_0x3A & 0x200)!=0x00) { //minus check
                offsetCurrent = offsetCurrent *-1;
            }

            break;
        //AMB
        case 2:
            OFFDAC_MID = (readRegister_0x3A>>10) &0x0F;
            OFFDAC_LSB = (readRegister_0x3E>>4) & 0x01;
            OFFDAC_MSB = (readRegister_0x3E>>4) & 0x02;

            CurrentWithoutSign =  (OFFDAC_MSB<<4) | (OFFDAC_MID<<1) |OFFDAC_LSB;

            offsetCurrent= offsetCurrent |(int16_t)validCurrents[CurrentWithoutSign];

            if((readRegister_0x3A & 0x4000)!=0x00) { //minus check
                offsetCurrent = offsetCurrent *-1;
            }

            break;

        //LED2
        case 3:
            OFFDAC_MID = (readRegister_0x3A>>15) &0x0F;
            OFFDAC_LSB = (readRegister_0x3E>>6) & 0x01;
            OFFDAC_MSB = (readRegister_0x3E>>6) & 0x02;

            CurrentWithoutSign =  (OFFDAC_MSB<<4) | (OFFDAC_MID<<1) |OFFDAC_LSB;

            offsetCurrent= offsetCurrent |(int16_t)validCurrents[CurrentWithoutSign];

            if((readRegister_0x3A& 0x80000)!=0x00) { //minus check
                offsetCurrent = offsetCurrent *-1;
            }

            break;

        //LED3
        case 4:
            OFFDAC_MID = (readRegister_0x3A>>0) &0x0F;
            OFFDAC_LSB = (readRegister_0x3E>>0) & 0x01;
            OFFDAC_MSB = (readRegister_0x3E>>0) & 0x02;

            CurrentWithoutSign =  (OFFDAC_MSB<<4) | (OFFDAC_MID<<1) |OFFDAC_LSB;

            offsetCurrent= offsetCurrent |(int16_t)validCurrents[CurrentWithoutSign];

            if((readRegister_0x3A & 0x10)!=0x00) { //minus check
                offsetCurrent = offsetCurrent *-1;
            }
            break;
    }
    return offsetCurrent;
}
//uint32_t Manual_DacOffset_3Ah;
//uint32_t Manual_DacOffset_3Eh;

//Channels are : LED1 , LED1 A,LED2 ,  LED2 A
void AFE4410_SetOffsetDAC (uint8_t channel , int16_t OFFDACValue)
{
    bool currentPolarityNegative = FALSE;
    if(OFFDACValue<0) {
        currentPolarityNegative = TRUE;
        OFFDACValue = -OFFDACValue;
    }

    uint8_t count = 0;
    uint16_t validCurrents[64] = {0,25,50,75,100,125,150,175,200,225,250,275,300,325,350,375,400,425,450,475,500,525,550,575,600,
                                  625,650,675,700,725,750,775,800,825,850,875,900,925,950,975,1000,1025,1050,1075,1100,1125,1150,1175,1200,1225,1250,1275,
                                  1300,1325,1350,1375,1400,1425,1450,1475,1500,1525,1550,1575
                                 };
    uint8_t validCurrentPosition = 0;
    bool ValidSetCurrent = FALSE;
    for(count=0; count<64; count++) {
        if(validCurrents[count] == OFFDACValue) {
            ValidSetCurrent = TRUE;
            validCurrentPosition = count;
            //zPrintf(USART1,"validCurrents = %d, count = %d \n",OFFDACValue,validCurrentPosition);
            break;
        }
    }

    if(!ValidSetCurrent) {
        //zPrintf(USART1,"AFE4405_setOffDAC Fail , Selected Current out of bounds ");
    }

    uint32_t readRegister_1 = AFE4410_getRegisterFromLiveArray(0x3A);//Manual_DacOffset_3Ah;// Manual_DacOffset_3Ah;// AFE4410_getRegisterFromLiveArray(0x3A);
    uint32_t readRegister_2 = AFE4410_getRegisterFromLiveArray(0x3E);//Manual_DacOffset_3Eh;//Manual_DacOffset_3Eh;// AFE4410_getRegisterFromLiveArray(0x3E);
    uint32_t OFFDAC_MSB=0,OFFDAC_MID=0,OFFDAC_LSB=0;
    uint32_t Reg3Ah =0, Reg3Eh=0;
    uint32_t WriteRegister_3Ah=0;
    uint32_t WriteRegister_3Eh=0;


    switch(validCurrentPosition) {
        case 0 :
            OFFDAC_MSB=0x00;
            OFFDAC_MID=0x00;
            OFFDAC_LSB=0x00;
            break;
        case 1 :
            OFFDAC_LSB=0x01;
            break;
        case 2 :
            OFFDAC_MID=0x01;
            break;
        case 3 :
            OFFDAC_MID=0x01;
            OFFDAC_LSB=0x01;
            break;
        case 4 :
            OFFDAC_MID=0x02;
            break;
        case 5 :
            OFFDAC_MID=0x02;
            OFFDAC_LSB=0x01;
            break;
        case 6 :
            OFFDAC_MID=0x03;
            break;
        case 7 :
            OFFDAC_MID=0x03;
            OFFDAC_LSB=0x01;
            break;
        case 8 :
            OFFDAC_MID=0x04;
            break;
        case 9 :
            OFFDAC_MID=0x04;
            OFFDAC_LSB=0x01;
            break;
        case 10 :
            OFFDAC_MID=0x05;
            break;
        case 11 :
            OFFDAC_MID=0x05;
            OFFDAC_LSB=0x01;
            break;
        case 12 :
            OFFDAC_MID=0x06;
            break;
        case 13 :
            OFFDAC_MID=0x06;
            OFFDAC_LSB=0x01;
            break;
        case 14 :
            OFFDAC_MID=0x07;
            break;
        case 15 :
            OFFDAC_MID=0x07;
            OFFDAC_LSB=0x01;
            break;
        case 16 :
            OFFDAC_MID=0x08;
            break;
        case 17 :
            OFFDAC_MID=0x08;
            OFFDAC_LSB=0x01;
            break;
        case 18 :
            OFFDAC_MID=0x09;
            break;
        case 19 :
            OFFDAC_MID=0x09;
            OFFDAC_LSB=0x01;
            break;
        case 20 :
            OFFDAC_MID=0x0A;
            break;
        case 21 :
            OFFDAC_MID=0x0A;
            OFFDAC_LSB=0x01;
            break;
        case 22 :
            OFFDAC_MID=0x0B;
            break;
        case 23 :
            OFFDAC_MID=0x0B;
            OFFDAC_LSB=0x01;
            break;
        case 24 :
            OFFDAC_MID=0x0C;
            break;
        case 25 :
            OFFDAC_MID=0x0C;
            OFFDAC_LSB=0x01;
            break;
        case 26 :
            OFFDAC_MID=0x0D;
            break;
        case 27 :
            OFFDAC_MID=0x0D;
            OFFDAC_LSB=0x01;
            break;
        case 28 :
            OFFDAC_MID=0x0E;
            break;
        case 29 :
            OFFDAC_MID=0x0E;
            OFFDAC_LSB=0x01;
            break;
        case 30 :
            OFFDAC_MID=0x0F;
            break;
        case 31 :
            OFFDAC_MID=0x0F;
            OFFDAC_LSB=0x01;
            break;
        case 32 :
            OFFDAC_MSB=0x02;
            break;
        case 33 :
            OFFDAC_MSB=0x02;
            OFFDAC_LSB=0x01;
            break;
        case 34 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x01;
            break;
        case 35 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x01;
            OFFDAC_LSB=0x01;
            break;
        case 36 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x02;
            break;
        case 37 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x02;
            OFFDAC_LSB=0x01;
            break;
        case 38 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x03;
            break;
        case 39 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x03;
            OFFDAC_LSB=0x01;
            break;
        case 40 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x04;
            break;
        case 41 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x04;
            OFFDAC_LSB=0x01;
            break;
        case 42 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x05;
            break;
        case 43 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x05;
            OFFDAC_LSB=0x01;
            break;
        case 44 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x06;
            break;
        case 45 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x06;
            OFFDAC_LSB=0x01;
            break;
        case 46 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x07;
            break;
        case 47 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x07;
            OFFDAC_LSB=0x01;
            break;
        case 48 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x08;
            break;
        case 49 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x08;
            OFFDAC_LSB=0x01;
            break;
        case 50 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x09;
            break;
        case 51 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x09;
            OFFDAC_LSB=0x01;
            break;
        case 52 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0A;
            break;
        case 53 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0A;
            OFFDAC_LSB=0x01;
            break;
        case 54 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0B;
            break;
        case 55 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0B;
            OFFDAC_LSB=0x01;
            break;
        case 56 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0C;
            break;
        case 57 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0C;
            OFFDAC_LSB=0x01;
            break;
        case 58 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0D;
            break;
        case 59 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0D;
            OFFDAC_LSB=0x01;
            break;
        case 60 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0E;
            break;
        case 61 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0E;
            OFFDAC_LSB=0x01;
            break;
        case 62 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0F;
            break;
        case 63 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0F;
            OFFDAC_LSB=0x01;
            break;

    }

    Reg3Ah = OFFDAC_MID;
    Reg3Eh = OFFDAC_MSB|OFFDAC_LSB;

    if(currentPolarityNegative) {
        Reg3Ah = Reg3Ah | 0x10; //Set bit for negative Isub
    }
    //FiveBitsToWrite = FiveBitsToWrite & 0x0000001F; //Only allow 5 bits
    switch(channel) {
        case 1://LED1

            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0xFFC1F; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<5);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0xF3; //Clear 2 bits
            WriteRegister_3Eh = readRegister_2 | (Reg3Eh <<2);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;
        case 2: // AMB
            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0xF83FF; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<10);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0xCF; //Clear 2 bits
            WriteRegister_3Eh = readRegister_2 | (Reg3Eh<<4);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;

        case 3://LED2
            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0x07FFF; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<15);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0x3F;
            WriteRegister_3Eh = readRegister_2 |(Reg3Eh<<6);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;
        case 4://LED3
            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0xFFFE0; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<0);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0xFC; //Clear 2 bits
            WriteRegister_3Eh = readRegister_2 | (Reg3Eh<<0);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;

        default:
            //zPrintf(USART1,"AFE4405_setOffDAC Fail ");
            break;
    }

    //Manual_DacOffset_3Ah = WriteRegister_3Ah;
    //Manual_DacOffset_3Eh = WriteRegister_3Eh;
    AFE4410_addWriteCommandToQueue(0x3A,WriteRegister_3Ah);
    AFE4410_addWriteCommandToQueue(0x3E, WriteRegister_3Eh);

}


//Channels are : LED1 , LED1 A,LED2 ,  LED2 A
void AFE4410_directSetOffsetDAC (uint8_t channel , int16_t OFFDACValue)
{
    bool currentPolarityNegative = FALSE;
    if(OFFDACValue<0) {
        currentPolarityNegative = TRUE;
        OFFDACValue = -OFFDACValue;
    }

    uint8_t count = 0;
    uint16_t validCurrents[64] = {0,25,50,75,100,125,150,175,200,225,250,275,300,325,350,375,400,425,450,475,500,525,550,575,600,
                                  625,650,675,700,725,750,775,800,825,850,875,900,925,950,975,1000,1025,1050,1075,1100,1125,1150,1175,1200,1225,1250,1275,
                                  1300,1325,1350,1375,1400,1425,1450,1475,1500,1525,1550,1575
                                 };
    uint8_t validCurrentPosition = 0;
    bool ValidSetCurrent = FALSE;
    for(count=0; count<64; count++) {
        if(validCurrents[count] == OFFDACValue) {
            ValidSetCurrent = TRUE;
            validCurrentPosition = count;
            //zPrintf(USART1,"validCurrents = %d, count = %d \n",OFFDACValue,validCurrentPosition);
            break;
        }
    }

    if(!ValidSetCurrent) {
        //zPrintf(USART1,"AFE4405_setOffDAC Fail , Selected Current out of bounds ");
    }

    uint32_t readRegister_1 = AFE4410_getRegisterFromLiveArray(0x3A);//Manual_DacOffset_3Ah;// Manual_DacOffset_3Ah;// AFE4410_getRegisterFromLiveArray(0x3A);
    uint32_t readRegister_2 = AFE4410_getRegisterFromLiveArray(0x3E);//Manual_DacOffset_3Eh;//Manual_DacOffset_3Eh;// AFE4410_getRegisterFromLiveArray(0x3E);
    uint32_t OFFDAC_MSB=0,OFFDAC_MID=0,OFFDAC_LSB=0;
    uint32_t Reg3Ah =0, Reg3Eh=0;
    uint32_t WriteRegister_3Ah=0;
    uint32_t WriteRegister_3Eh=0;


    switch(validCurrentPosition) {
        case 0 :
            OFFDAC_MSB=0x00;
            OFFDAC_MID=0x00;
            OFFDAC_LSB=0x00;
            break;
        case 1 :
            OFFDAC_LSB=0x01;
            break;
        case 2 :
            OFFDAC_MID=0x01;
            break;
        case 3 :
            OFFDAC_MID=0x01;
            OFFDAC_LSB=0x01;
            break;
        case 4 :
            OFFDAC_MID=0x02;
            break;
        case 5 :
            OFFDAC_MID=0x02;
            OFFDAC_LSB=0x01;
            break;
        case 6 :
            OFFDAC_MID=0x03;
            break;
        case 7 :
            OFFDAC_MID=0x03;
            OFFDAC_LSB=0x01;
            break;
        case 8 :
            OFFDAC_MID=0x04;
            break;
        case 9 :
            OFFDAC_MID=0x04;
            OFFDAC_LSB=0x01;
            break;
        case 10 :
            OFFDAC_MID=0x05;
            break;
        case 11 :
            OFFDAC_MID=0x05;
            OFFDAC_LSB=0x01;
            break;
        case 12 :
            OFFDAC_MID=0x06;
            break;
        case 13 :
            OFFDAC_MID=0x06;
            OFFDAC_LSB=0x01;
            break;
        case 14 :
            OFFDAC_MID=0x07;
            break;
        case 15 :
            OFFDAC_MID=0x07;
            OFFDAC_LSB=0x01;
            break;
        case 16 :
            OFFDAC_MID=0x08;
            break;
        case 17 :
            OFFDAC_MID=0x08;
            OFFDAC_LSB=0x01;
            break;
        case 18 :
            OFFDAC_MID=0x09;
            break;
        case 19 :
            OFFDAC_MID=0x09;
            OFFDAC_LSB=0x01;
            break;
        case 20 :
            OFFDAC_MID=0x0A;
            break;
        case 21 :
            OFFDAC_MID=0x0A;
            OFFDAC_LSB=0x01;
            break;
        case 22 :
            OFFDAC_MID=0x0B;
            break;
        case 23 :
            OFFDAC_MID=0x0B;
            OFFDAC_LSB=0x01;
            break;
        case 24 :
            OFFDAC_MID=0x0C;
            break;
        case 25 :
            OFFDAC_MID=0x0C;
            OFFDAC_LSB=0x01;
            break;
        case 26 :
            OFFDAC_MID=0x0D;
            break;
        case 27 :
            OFFDAC_MID=0x0D;
            OFFDAC_LSB=0x01;
            break;
        case 28 :
            OFFDAC_MID=0x0E;
            break;
        case 29 :
            OFFDAC_MID=0x0E;
            OFFDAC_LSB=0x01;
            break;
        case 30 :
            OFFDAC_MID=0x0F;
            break;
        case 31 :
            OFFDAC_MID=0x0F;
            OFFDAC_LSB=0x01;
            break;
        case 32 :
            OFFDAC_MSB=0x02;
            break;
        case 33 :
            OFFDAC_MSB=0x02;
            OFFDAC_LSB=0x01;
            break;
        case 34 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x01;
            break;
        case 35 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x01;
            OFFDAC_LSB=0x01;
            break;
        case 36 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x02;
            break;
        case 37 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x02;
            OFFDAC_LSB=0x01;
            break;
        case 38 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x03;
            break;
        case 39 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x03;
            OFFDAC_LSB=0x01;
            break;
        case 40 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x04;
            break;
        case 41 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x04;
            OFFDAC_LSB=0x01;
            break;
        case 42 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x05;
            break;
        case 43 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x05;
            OFFDAC_LSB=0x01;
            break;
        case 44 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x06;
            break;
        case 45 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x06;
            OFFDAC_LSB=0x01;
            break;
        case 46 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x07;
            break;
        case 47 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x07;
            OFFDAC_LSB=0x01;
            break;
        case 48 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x08;
            break;
        case 49 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x08;
            OFFDAC_LSB=0x01;
            break;
        case 50 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x09;
            break;
        case 51 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x09;
            OFFDAC_LSB=0x01;
            break;
        case 52 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0A;
            break;
        case 53 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0A;
            OFFDAC_LSB=0x01;
            break;
        case 54 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0B;
            break;
        case 55 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0B;
            OFFDAC_LSB=0x01;
            break;
        case 56 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0C;
            break;
        case 57 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0C;
            OFFDAC_LSB=0x01;
            break;
        case 58 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0D;
            break;
        case 59 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0D;
            OFFDAC_LSB=0x01;
            break;
        case 60 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0E;
            break;
        case 61 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0E;
            OFFDAC_LSB=0x01;
            break;
        case 62 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0F;
            break;
        case 63 :
            OFFDAC_MSB=0x02;
            OFFDAC_MID=0x0F;
            OFFDAC_LSB=0x01;
            break;

    }

    Reg3Ah = OFFDAC_MID;
    Reg3Eh = OFFDAC_MSB|OFFDAC_LSB;

    if(currentPolarityNegative) {
        Reg3Ah = Reg3Ah | 0x10; //Set bit for negative Isub
    }

    //FiveBitsToWrite = FiveBitsToWrite & 0x0000001F; //Only allow 5 bits
    switch(channel) {
        case 1://LED1

            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0xFFC1F; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<5);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0xF3; //Clear 2 bits
            WriteRegister_3Eh = readRegister_2 | (Reg3Eh <<2);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;
        case 2: // AMB
            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0xF83FF; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<10);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0xCF; //Clear 2 bits
            WriteRegister_3Eh = readRegister_2 | (Reg3Eh<<4);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;

        case 3://LED2
            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0x07FFF; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<15);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0x3F;
            WriteRegister_3Eh = readRegister_2 |(Reg3Eh<<6);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;
        case 4://LED3
            //Reg 3Ah
            readRegister_1 = readRegister_1 & 0xFFFE0; //Clear 5 bits where Isub will be stored
            WriteRegister_3Ah = readRegister_1 | (Reg3Ah<<0);
            //Reg 3Eh
            readRegister_2 = readRegister_2 & 0xFC; //Clear 2 bits
            WriteRegister_3Eh = readRegister_2 | (Reg3Eh<<0);

            //zPrintf(USART1,"Reg3Ah = %x\n",WriteRegister_3Ah);
            //zPrintf(USART1,"Reg3Eh = %x\n",WriteRegister_3Eh);
            break;

        default:
            //zPrintf(USART1,"AFE4405_setOffDAC Fail ");
            break;
    }

    //Manual_DacOffset_3Ah = WriteRegister_3Ah;
    //Manual_DacOffset_3Eh = WriteRegister_3Eh;
    AFE4410_writeRegWithoutWriteEnable(0x3A,WriteRegister_3Ah);
    AFE4410_writeRegWithoutWriteEnable(0x3E, WriteRegister_3Eh);

}


uint8_t AFE4410_getLedCurrent_P(uint8_t ledNumber)
{
    uint32_t readValue = AFE4410_getRegisterFromLiveArray(34);
    uint32_t LSB=0;
    uint32_t MSB=0;

    uint8_t LED_val=0;


    switch(ledNumber) {
        case 1:
            readValue = readValue & 0xC003F;

            LSB = readValue & 0xC0000;
            LSB = LSB>>18;

            MSB = readValue & 0x3F;
            MSB = MSB<<2;

            LED_val = MSB|LSB;
            break;
        case 2:
            readValue = readValue & 0x300FC0;

            LSB = readValue & 0x300000;
            LSB = LSB>>20;

            MSB = readValue & 0xFC0;
            MSB = MSB>>4;

            LED_val = MSB |LSB;
            break;

        case 3:
            readValue = readValue & 0xC3F000;

            LSB = readValue & 0xC00000;
            LSB = LSB>>22;

            MSB = readValue & 0x3F000;
            MSB = readValue >>10;

            LED_val = MSB |LSB;
            break;
    }


    return LED_val;
}


void AFE4410_setLedCurrent_P(uint8_t ledNumber , uint16_t currentvalue)
{
    if(currentvalue>255) {
        //zPrintf(USART1,"Current value is out of bounds \n");
    }
    uint32_t readValue = AFE4410_getRegisterFromLiveArray(34);

    //zPrintf(USART1,"LED Read value = %x \n",readValue);

    uint32_t LED_Current=0;
    LED_Current = current_Apply(ledNumber,currentvalue);

    switch(ledNumber) {
        case 1:
            readValue = readValue & 0xF3FFC0;
            readValue = readValue | LED_Current;

            //zPrintf(USART1,"LED Read value 1 = %x, LED_Current = %x \n",readValue,LED_Current);
            break;
        case 2:
            readValue = readValue & 0xCFF03F;
            readValue = readValue | LED_Current;

            //zPrintf(USART1,"LED Read value 2 = %x, LED_Current = %x \n",readValue,LED_Current);
            break;
        case 3:
            readValue = readValue & 0x3C0FFF;
            readValue = readValue | LED_Current;

            //zPrintf(USART1,"LED Read value 3 = %x, LED_Current = %x \n",readValue,LED_Current);
            break;
        default:

            break;
    }
    AFE4410_addWriteCommandToQueue(34,readValue);

}


void AFE4410_directSetLedCurrent_P(uint8_t ledNumber , uint16_t currentvalue)
{
    if(currentvalue>255) {
        //zPrintf(USART1,"Current value is out of bounds \n");
    }
    uint32_t readValue = AFE4410_getRegisterFromLiveArray(34);

    //zPrintf(USART1,"LED Read value = %x \n",readValue);

    uint32_t LED_Current=0;
    LED_Current = current_Apply(ledNumber,currentvalue);

    switch(ledNumber) {
        case 1:
            readValue = readValue & 0xF3FFC0;
            readValue = readValue | LED_Current;

            //zPrintf(USART1,"LED Read value 1 = %x, LED_Current = %x \n",readValue,LED_Current);
            break;
        case 2:
            readValue = readValue & 0xCFF03F;
            readValue = readValue | LED_Current;

            //zPrintf(USART1,"LED Read value 2 = %x, LED_Current = %x \n",readValue,LED_Current);
            break;
        case 3:
            readValue = readValue & 0x3C0FFF;
            readValue = readValue | LED_Current;

            //zPrintf(USART1,"LED Read value 3 = %x, LED_Current = %x \n",readValue,LED_Current);
            break;
        default:

            break;
    }
    AFE4410_writeRegWithoutWriteEnable(34,readValue);

}


uint32_t current_Apply(uint8_t ledNo,uint16_t value)
{
    uint32_t currentval=0;

    switch(value) {
        case 0:
            currentval=0x00;
            break;
        case 1:
            currentval=0x40000;
            break;
        case 2:
            currentval=0x80000;
            break;
        case 3:
            currentval=0xC0000;
            break;
        case 4:
            currentval=0x01;
            break;
        case 5:
            currentval=0x40001;
            break;
        case 6:
            currentval=0x80001;
            break;
        case 7:
            currentval=0xC0001;
            break;
        case 8:
            currentval=0x02;
            break;
        case 9:
            currentval=0x40002;
            break;
        case 10:
            currentval=0x80002;
            break;
        case 11:
            currentval=0xC0002;
            break;
        case 12:
            currentval=0x03;
            break;
        case 13:
            currentval=0x40003;
            break;
        case 14:
            currentval=0x80003;
            break;
        case 15:
            currentval=0xC0003;
            break;
        case 16:
            currentval=0x04;
            break;
        case 17:
            currentval=0x40004;
            break;
        case 18:
            currentval=0x80004;
            break;
        case 19:
            currentval=0xC0004;
            break;
        case 20:
            currentval=0x05;
            break;
        case 21:
            currentval=0x40005;
            break;
        case 22:
            currentval=0x80005;
            break;
        case 23:
            currentval=0xC0005;
            break;
        case 24:
            currentval=0x06;
            break;
        case 25:
            currentval=0x40006;
            break;
        case 26:
            currentval=0x80006;
            break;
        case 27:
            currentval=0xC0006;
            break;
        case 28:
            currentval=0x07;
            break;
        case 29:
            currentval=0x40007;
            break;
        case 30:
            currentval=0x80007;
            break;
        case 31:
            currentval=0xC0007;
            break;
        case 32:
            currentval=0x08;
            break;
        case 33:
            currentval=0x40008;
            break;
        case 34:
            currentval=0x80008;
            break;
        case 35:
            currentval=0xC0008;
            break;
        case 36:
            currentval=0x09;
            break;
        case 37:
            currentval=0x40009;
            break;
        case 38:
            currentval=0x80009;
            break;
        case 39:
            currentval=0xC0009;
            break;
        case 40:
            currentval=0x0A;
            break;
        case 41:
            currentval=0x4000A;
            break;
        case 42:
            currentval=0x8000A;
            break;
        case 43:
            currentval=0xC000A;
            break;
        case 44:
            currentval=0x0B;
            break;
        case 45:
            currentval=0x4000B;
            break;
        case 46:
            currentval=0x8000B;
            break;
        case 47:
            currentval=0xC000B;
            break;
        case 48:
            currentval=0x0C;
            break;
        case 49:
            currentval=0x4000C;
            break;
        case 50:
            currentval=0x8000C;
            break;
        case 51:
            currentval=0xC000C;
            break;
        case 52:
            currentval=0x0D;
            break;
        case 53:
            currentval=0x4000D;
            break;
        case 54:
            currentval=0x8000D;
            break;
        case 55:
            currentval=0xC000D;
            break;
        case 56:
            currentval=0x0E;
            break;
        case 57:
            currentval=0x4000E;
            break;
        case 58:
            currentval=0x8000E;
            break;
        case 59:
            currentval=0xC000E;
            break;
        case 60:
            currentval=0x0F;
            break;
        case 61:
            currentval=0x4000F;
            break;
        case 62:
            currentval=0x8000F;
            break;
        case 63:
            currentval=0xC000F;
            break;
        case 64:
            currentval=0x10;
            break;
        case 65:
            currentval=0x40010;
            break;
        case 66:
            currentval=0x80010;
            break;
        case 67:
            currentval=0xC0010;
            break;
        case 68:
            currentval=0x11;
            break;
        case 69:
            currentval=0x40011;
            break;
        case 70:
            currentval=0x80011;
            break;
        case 71:
            currentval=0xC0011;
            break;
        case 72:
            currentval=0x12;
            break;
        case 73:
            currentval=0x40012;
            break;
        case 74:
            currentval=0x80012;
            break;
        case 75:
            currentval=0xC0012;
            break;
        case 76:
            currentval=0x13;
            break;
        case 77:
            currentval=0x40013;
            break;
        case 78:
            currentval=0x80013;
            break;
        case 79:
            currentval=0xC0013;
            break;
        case 80:
            currentval=0x14;
            break;
        case 81:
            currentval=0x40014;
            break;
        case 82:
            currentval=0x80014;
            break;
        case 83:
            currentval=0xC0014;
            break;
        case 84:
            currentval=0x15;
            break;
        case 85:
            currentval=0x40015;
            break;
        case 86:
            currentval=0x80015;
            break;
        case 87:
            currentval=0xC0015;
            break;
        case 88:
            currentval=0x16;
            break;
        case 89:
            currentval=0x40016;
            break;
        case 90:
            currentval=0x80016;
            break;
        case 91:
            currentval=0xC0016;
            break;
        case 92:
            currentval=0x17;
            break;
        case 93:
            currentval=0x40017;
            break;
        case 94:
            currentval=0x80017;
            break;
        case 95:
            currentval=0xC0017;
            break;
        case 96:
            currentval=0x18;
            break;
        case 97:
            currentval=0x40018;
            break;
        case 98:
            currentval=0x80018;
            break;
        case 99:
            currentval=0xC0018;
            break;
        case 100:
            currentval=0x19;
            break;
        case 101:
            currentval=0x40019;
            break;
        case 102:
            currentval=0x80019;
            break;
        case 103:
            currentval=0xC0019;
            break;
        case 104:
            currentval=0x1A;
            break;
        case 105:
            currentval=0x4001A;
            break;
        case 106:
            currentval=0x8001A;
            break;
        case 107:
            currentval=0xC001A;
            break;
        case 108:
            currentval=0x1B;
            break;
        case 109:
            currentval=0x4001B;
            break;
        case 110:
            currentval=0x8001B;
            break;
        case 111:
            currentval=0xC001B;
            break;
        case 112:
            currentval=0x1C;
            break;
        case 113:
            currentval=0x4001C;
            break;
        case 114:
            currentval=0x8001C;
            break;
        case 115:
            currentval=0xC001C;
            break;
        case 116:
            currentval=0x1D;
            break;
        case 117:
            currentval=0x4001D;
            break;
        case 118:
            currentval=0x8001D;
            break;
        case 119:
            currentval=0xC001D;
            break;
        case 120:
            currentval=0x1E;
            break;
        case 121:
            currentval=0x4001E;
            break;
        case 122:
            currentval=0x8001E;
            break;
        case 123:
            currentval=0xC001E;
            break;
        case 124:
            currentval=0x1F;
            break;
        case 125:
            currentval=0x4001F;
            break;
        case 126:
            currentval=0x8001F;
            break;
        case 127:
            currentval=0xC001F;
            break;
        case 128:
            currentval=0x20;
            break;
        case 129:
            currentval=0x40020;
            break;
        case 130:
            currentval=0x80020;
            break;
        case 131:
            currentval=0xC0020;
            break;
        case 132:
            currentval=0x21;
            break;
        case 133:
            currentval=0x40021;
            break;
        case 134:
            currentval=0x80021;
            break;
        case 135:
            currentval=0xC0021;
            break;
        case 136:
            currentval=0x22;
            break;
        case 137:
            currentval=0x40022;
            break;
        case 138:
            currentval=0x80022;
            break;
        case 139:
            currentval=0xC0022;
            break;
        case 140:
            currentval=0x23;
            break;
        case 141:
            currentval=0x40023;
            break;
        case 142:
            currentval=0x80023;
            break;
        case 143:
            currentval=0xC0023;
            break;
        case 144:
            currentval=0x24;
            break;
        case 145:
            currentval=0x40024;
            break;
        case 146:
            currentval=0x80024;
            break;
        case 147:
            currentval=0xC0024;
            break;
        case 148:
            currentval=0x25;
            break;
        case 149:
            currentval=0x40025;
            break;
        case 150:
            currentval=0x80025;
            break;
        case 151:
            currentval=0xC0025;
            break;
        case 152:
            currentval=0x26;
            break;
        case 153:
            currentval=0x40026;
            break;
        case 154:
            currentval=0x80026;
            break;
        case 155:
            currentval=0xC0026;
            break;
        case 156:
            currentval=0x27;
            break;
        case 157:
            currentval=0x40027;
            break;
        case 158:
            currentval=0x80027;
            break;
        case 159:
            currentval=0xC0027;
            break;
        case 160:
            currentval=0x28;
            break;
        case 161:
            currentval=0x40028;
            break;
        case 162:
            currentval=0x80028;
            break;
        case 163:
            currentval=0xC0028;
            break;
        case 164:
            currentval=0x29;
            break;
        case 165:
            currentval=0x40029;
            break;
        case 166:
            currentval=0x80029;
            break;
        case 167:
            currentval=0xC0029;
            break;
        case 168:
            currentval=0x2A;
            break;
        case 169:
            currentval=0x4002A;
            break;
        case 170:
            currentval=0x8002A;
            break;
        case 171:
            currentval=0xC002A;
            break;
        case 172:
            currentval=0x2B;
            break;
        case 173:
            currentval=0x4002B;
            break;
        case 174:
            currentval=0x8002B;
            break;
        case 175:
            currentval=0xC002B;
            break;
        case 176:
            currentval=0x2C;
            break;
        case 177:
            currentval=0x4002C;
            break;
        case 178:
            currentval=0x8002C;
            break;
        case 179:
            currentval=0xC002C;
            break;
        case 180:
            currentval=0x2D;
            break;
        case 181:
            currentval=0x4002D;
            break;
        case 182:
            currentval=0x8002D;
            break;
        case 183:
            currentval=0xC002D;
            break;
        case 184:
            currentval=0x2E;
            break;
        case 185:
            currentval=0x4002E;
            break;
        case 186:
            currentval=0x8002E;
            break;
        case 187:
            currentval=0xC002E;
            break;
        case 188:
            currentval=0x2F;
            break;
        case 189:
            currentval=0x4002F;
            break;
        case 190:
            currentval=0x8002F;
            break;
        case 191:
            currentval=0xC002F;
            break;
        case 192:
            currentval=0x30;
            break;
        case 193:
            currentval=0x40030;
            break;
        case 194:
            currentval=0x80030;
            break;
        case 195:
            currentval=0xC0030;
            break;
        case 196:
            currentval=0x31;
            break;
        case 197:
            currentval=0x40031;
            break;
        case 198:
            currentval=0x80031;
            break;
        case 199:
            currentval=0xC0031;
            break;
        case 200:
            currentval=0x32;
            break;
        case 201:
            currentval=0x40032;
            break;
        case 202:
            currentval=0x80032;
            break;
        case 203:
            currentval=0xC0032;
            break;
        case 204:
            currentval=0x33;
            break;
        case 205:
            currentval=0x40033;
            break;
        case 206:
            currentval=0x80033;
            break;
        case 207:
            currentval=0xC0033;
            break;
        case 208:
            currentval=0x34;
            break;
        case 209:
            currentval=0x40034;
            break;
        case 210:
            currentval=0x80034;
            break;
        case 211:
            currentval=0xC0034;
            break;
        case 212:
            currentval=0x35;
            break;
        case 213:
            currentval=0x40035;
            break;
        case 214:
            currentval=0x80035;
            break;
        case 215:
            currentval=0xC0035;
            break;
        case 216:
            currentval=0x36;
            break;
        case 217:
            currentval=0x40036;
            break;
        case 218:
            currentval=0x80036;
            break;
        case 219:
            currentval=0xC0036;
            break;
        case 220:
            currentval=0x37;
            break;
        case 221:
            currentval=0x40037;
            break;
        case 222:
            currentval=0x80037;
            break;
        case 223:
            currentval=0xC0037;
            break;
        case 224:
            currentval=0x38;
            break;
        case 225:
            currentval=0x40038;
            break;
        case 226:
            currentval=0x80038;
            break;
        case 227:
            currentval=0xC0038;
            break;
        case 228:
            currentval=0x39;
            break;
        case 229:
            currentval=0x40039;
            break;
        case 230:
            currentval=0x80039;
            break;
        case 231:
            currentval=0xC0039;
            break;
        case 232:
            currentval=0x3A;
            break;
        case 233:
            currentval=0x4003A;
            break;
        case 234:
            currentval=0x8003A;
            break;
        case 235:
            currentval=0xC003A;
            break;
        case 236:
            currentval=0x3B;
            break;
        case 237:
            currentval=0x4003B;
            break;
        case 238:
            currentval=0x8003B;
            break;
        case 239:
            currentval=0xC003B;
            break;
        case 240:
            currentval=0x3C;
            break;
        case 241:
            currentval=0x4003C;
            break;
        case 242:
            currentval=0x8003C;
            break;
        case 243:
            currentval=0xC003C;
            break;
        case 244:
            currentval=0x3D;
            break;
        case 245:
            currentval=0x4003D;
            break;
        case 246:
            currentval=0x8003D;
            break;
        case 247:
            currentval=0xC003D;
            break;
        case 248:
            currentval=0x3E;
            break;
        case 249:
            currentval=0x4003E;
            break;
        case 250:
            currentval=0x8003E;
            break;
        case 251:
            currentval=0xC003E;
            break;
        case 252:
            currentval=0x3F;
            break;
        case 253:
            currentval=0x4003F;
            break;
        case 254:
            currentval=0x8003F;
            break;
        case 255:
            currentval=0xC003F;
            break;
    }

    uint32_t currentval_msb=0,currentval_Lsb=0;

    uint32_t LED_Current=0;

    currentval_Lsb = currentval & 0xC0000;
    currentval_msb = currentval & 0x0003F;
    switch(ledNo) {
        case 1 :
            LED_Current = currentval_Lsb |currentval_msb;
            break;
        case 2 :
            LED_Current = (currentval_Lsb<<2) |(currentval_msb<<6);
            break;
        case 3 :
            LED_Current = (currentval_Lsb<<4) |(currentval_msb<<12);
            break;

    }
    return LED_Current;
}


uint16_t afeConvP16(int32_t val)
{
    int32_t ConvertTemp = 0;
    ConvertTemp = (val<<8);
    ConvertTemp = ConvertTemp/16384;
    ConvertTemp = ConvertTemp + 32768;
    return (uint16_t)ConvertTemp;
}

int32_t afeConvP32(int32_t val)
{
    int32_t ConvertTemp = 0;
    ConvertTemp = (val<<8);
    ConvertTemp = ConvertTemp/256;
    return (int32_t)ConvertTemp;
}



uint8_t I2C_writeReg(uint8_t regaddr, uint32_t wdata)
{
    uint8_t   returnack = TRUE;
    PPSI26x_writeReg(regaddr, wdata);
    return returnack;
}


uint32_t I2C_readReg(uint8_t regaddr)
{
    uint32_t readdata = 0;
    readdata = PPSI26x_readReg(regaddr);
    return readdata;
}

uint32_t I2C_readReg_FIFO(uint8_t regaddr,uint8_t *read_fifo,uint32_t fifoLength)
{
    uint32_t readdata = 0;
    uint8_t b =0;
    uint8_t a,c =0;
    //uint8_t data[300];//25Hz * 4 spl * 3byte=300
    //uint32_t fifoLength=300;//25Hz * 4 spl * 3byte=300
    readdata = PPSI26x_readRegFIFO(regaddr,read_fifo,fifoLength);
    return readdata;
}

uint32_t I2C_readReg_FIFO_LQ(uint8_t regaddr,uint8_t *read_fifo,uint16_t length)
{
    uint32_t readdata = 0;
    uint8_t b =0;
    uint8_t a,c =0;
    return readdata;
}

uint32_t LEDVal_Read_4405_to_4404(uint32_t led4405)
{
    uint8_t Led1_LSB =0,Led2_LSB =0,Led3_LSB =0;
    uint8_t Led1_MSB =0,Led2_MSB =0,Led3_MSB =0;
    uint32_t led4404=0;

    Led1_LSB = (led4405 & 0xC0000)>>18;
    Led2_LSB = (led4405 & 0x300000)>>20;
    Led3_LSB = (led4405 & 0xC00000)>>22;

    Led1_MSB = (led4405 & 0x0f);
    Led2_MSB = (led4405 & 0x3C0)>>6;
    Led3_MSB = (led4405 & 0xF000)>>12;

    led4404 = (Led1_LSB | (Led1_MSB<<2) |(Led2_LSB<<6) | (Led2_MSB<<8) | (Led3_LSB<<12)| (Led3_MSB<<14));

    return led4404;
}

uint32_t LEDVal_Write_4404_to_4405(uint32_t led4404)
{
    uint8_t Led1_LSB =0,Led2_LSB =0,Led3_LSB =0;
    uint8_t Led1_MSB =0,Led2_MSB =0,Led3_MSB =0;
    uint32_t led4405=0;

    Led1_LSB = (led4404 & 0x03);
    Led2_LSB = (led4404 & 0xC0)>>6;
    Led3_LSB = (led4404 & 0x3000)>>12;

    Led1_MSB = (led4404 & 0x3C)>>2;
    Led2_MSB = (led4404 & 0xF00)>>8;
    Led3_MSB = (led4404 & 0x3C000)>>14;

    led4405 = ((Led1_LSB<<18) | (Led2_LSB<<20) | (Led3_LSB<<22) | Led1_MSB | (Led2_MSB<<6) |(Led3_MSB<<12));

    return led4405;
}

uint32_t OffsetDAC_Read_4405_to_4404(uint32_t dac4405_1,uint8_t dac4405_2)
{
    uint8_t Led1_POL=0, Led2_POL=0, Led3_POL=0, LedA_POL=0;
    uint8_t Led1_MID=0, Led2_MID=0, Led3_MID=0, LedA_MID=0;
    uint8_t Led1_LSB=0, Led2_LSB=0, Led3_LSB=0, LedA_LSB=0;

    uint32_t dac4404=0;

    Led1_POL = (dac4405_1 & 0x200)>>9;
    Led2_POL = (dac4405_1 & 0x80000)>>19;
    Led3_POL = (dac4405_1 & 0x10)>>4;
    LedA_POL = (dac4405_1 & 0x4000)>>14;
    Led1_MID = (dac4405_1 & 0xE0)>>5;
    Led2_MID = (dac4405_1 & 0x38000)>>15;
    Led3_MID = (dac4405_1 & 0x07)>>0;
    LedA_MID = (dac4405_1 & 0x1C00)>>10;
    Led1_LSB = (dac4405_2 & 0x04)>>2;
    Led2_LSB = (dac4405_2 & 0x40)>>6;
    Led3_LSB = (dac4405_2 & 0x01)>>0;
    LedA_LSB = (dac4405_2 & 0x10)>>4;

    dac4404 = (Led1_POL<<9) | (Led1_LSB<<5) | (Led1_MID<<6) | (Led2_POL<<19) | (Led2_LSB<<15) | (Led2_MID<<16) |
              (Led3_POL<<4) | (Led3_LSB<<0) | (Led3_MID<<1) | (LedA_POL<<14) | (LedA_LSB<<10) | (LedA_MID<<11);

    return dac4404;
}

void OffsetDac_Write_4404_to_4405(uint32_t dac4404, uint32_t *dac4405)
{
    uint8_t Led1_POL=0, Led2_POL=0, Led3_POL=0,LedA_POL=0;
    uint8_t Led1_LSB=0, Led2_LSB=0, Led3_LSB=0, LedA_LSB=0;
    uint8_t Led1_MID=0, Led2_MID=0, Led3_MID=0, LedA_MID=0;

    Led1_POL = (dac4404 & 0x200)>>9;
    Led2_POL = (dac4404 & 0x80000)>>19;
    Led3_POL = (dac4404 & 0x10)>>4;
    LedA_POL = (dac4404 & 0x4000)>>14;
    Led1_LSB = (dac4404 & 0x20)>>5;
    Led2_LSB = (dac4404 & 0x8000)>>15;
    Led3_LSB = (dac4404 & 0x01)>>0;
    LedA_LSB = (dac4404 & 0x400)>>10;
    Led1_MID = (dac4404 & 0x1C0)>>6;
    Led2_MID = (dac4404 & 0x70000)>>16;
    Led3_MID =(dac4404 & 0x0E)>>1;
    LedA_MID = (dac4404 & 0x3800)>>11;

    dac4405[0] = (Led1_POL<<9) | (Led1_MID<<5) | (Led2_POL<<19) | (Led2_MID<<16) | (Led3_POL<<4) | (Led3_MID<< 1) | (LedA_POL <<14) | (LedA_MID<<11);
    dac4405[1] = (Led1_LSB<<2)| (Led2_LSB<<6) | (Led3_LSB<<0) | (LedA_LSB<<4);

}

uint8_t AFE4410_GetRFtoSetting(uint8_t rfNum)
{
    uint16_t tmpRf;
    int8_t i;
    uint16_t possibleRf[9] = {500,250,100,50,25,10,1000,2000,1500};

    tmpRf=AFE4410_GetRF(rfNum);
    for(i=0; i<9; i++) {
        if(tmpRf==possibleRf[i]) return i;
    }
    return 0;
}

uint8_t AFE4410_GetOffsetDACtoSetting(uint8_t channel)
{
    int16_t tmpOffset;
    int8_t i;
    int8_t sign=0;
    uint16_t validCurrents[64] = {0,25,50,75,100,125,150,175,200,225,250,275,300,325,350,375,400,425,450,475,500,525,550,575,600,
                                  625,650,675,700,725,750,775,800,825,850,875,900,925,950,975,1000,1025,1050,1075,1100,1125,1150,1175,1200,1225,1250,1275,
                                  1300,1325,1350,1375,1400,1425,1450,1475,1500,1525,1550,1575
                                 };

    tmpOffset = AFE4410_GetOffsetDAC(channel);

    if(tmpOffset<0) {
        tmpOffset=tmpOffset*(-1);
        sign=1;
    }
    for(i=0; i<64; i++) {
        if(tmpOffset==validCurrents[i]) {
            if(sign)i = i | 0x80;
            return i;
        }
    }
    return 0;

}

int16_t AFE4410_settingsUintGetIsub(uint8_t channel, uint64_t inputArr)
{
    uint32_t readRegister =(inputArr >> 24); // ReadRegister is now an exact copy of what was read from the AFE
    readRegister = readRegister & 0xFFFFFFFF;
    int16_t retrievedCurrent = 0x0;
    uint16_t retrievedCurrentWithoutSign = 0x0;
    //uint8_t validCurrents[16] = {0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75};
    uint16_t validCurrents[64] = {
        0,25,50,75,100,125,150,175,200,225,250,275,300,
        325,350,375,400,425,450,475,500,525,550,575,600,
        625,650,675,700,725,750,775,800,825,850,875,900,
        925,950,975,1000,1025,1050,1075,1100,1125,1150,1175,1200,
        1225,1250,1275,1300,1325,1350,1375,1400,1425,1450,1475,1500,1525,1550,1575
    };

    switch(channel) {
        case 1: //GREEN
            retrievedCurrentWithoutSign = (readRegister>>0) & 0x7F;
            retrievedCurrent = retrievedCurrent | (int16_t)validCurrents[retrievedCurrentWithoutSign]; // Combine value without sign and value which possibly contains a sign
            if((readRegister & 0x80) != 0x00) {
                retrievedCurrent = retrievedCurrent * -1;
            }
            return retrievedCurrent;
            break;
        case 2: // AMB
            retrievedCurrentWithoutSign = (readRegister>>8) & 0x7F;
            retrievedCurrent = retrievedCurrent | (int16_t)validCurrents[retrievedCurrentWithoutSign]; // Combine value without sign and value which possibly contains a sign
            if((readRegister & 0x8000) != 0x00) {
                retrievedCurrent = retrievedCurrent * -1;
            }
            return retrievedCurrent;
            break;
        case 3: //IR
            retrievedCurrentWithoutSign = (readRegister>>16) & 0x7F;
            retrievedCurrent = retrievedCurrent | (int16_t)validCurrents[retrievedCurrentWithoutSign]; // Combine value without sign and value which possibly contains a sign
            if((readRegister & 0x800000) != 0x00) {
                retrievedCurrent = retrievedCurrent * -1;
            }
            return retrievedCurrent;
            break;
        case 4: //RED
            retrievedCurrentWithoutSign = (readRegister>>24) & 0x7f;
            retrievedCurrent = retrievedCurrent | (int16_t)validCurrents[retrievedCurrentWithoutSign]; // Combine value without sign and value which possibly contains a sign
            if((readRegister & 0x80000000) != 0x00) {
                retrievedCurrent = retrievedCurrent * -1;
            }
            return retrievedCurrent;
            break;
        default:
            return 0xFF;
            break;
    }
}

// Input uinit64 settings array, returns the Rf value
uint16_t AFE4410_settingsUintGetRf(uint8_t channel, uint64_t inputArr)
{
    uint8_t rfVal = 0;
    uint16_t possibleRf[9] = {500,250,100,50,25,10,1000,2000,1500};
    if(channel == 1) {
        rfVal = ((inputArr >>56) & 0x0f);
        return possibleRf[rfVal];
    }
    if(channel == 2) {
        rfVal = ((inputArr >>60) & 0x0f);
        return possibleRf[rfVal];
    }
    return 0;
}
uint8_t AFE4410_settingsUintGetLedCurrent(uint8_t ledNumber, uint64_t inputArr)
{
    uint32_t readValue = inputArr & 0x0FFFFFF;
    switch(ledNumber) {
        case 1:
            readValue = readValue & 0xff;
            break;
        case 2:
            readValue = (readValue>>8) & 0xff;
            break;
        case 3:
            readValue = (readValue>>16) & 0xff;
            break;
        default:
            return 0xFF;
            break;
    }
    return (uint8_t)readValue;
}
uint64_t AFE4410_getAFESettingsUint(void)
{
    // Build a single 44 bit register that represents the following afe settings currently in use:
    // Rf1 & Rf2
    // Isub 1 to Isub4
    // Led currents
    uint64_t settingsRegister = 0;
    settingsRegister |=  (((uint64_t)AFE4410_getLedCurrent_P(1))<<0); // 8 bits
    settingsRegister |=  (((uint64_t)AFE4410_getLedCurrent_P(2))<<8); // 8 bits
    settingsRegister |=  (((uint64_t)AFE4410_getLedCurrent_P(3))<<16); // 8 bits

    settingsRegister |=  (((uint64_t)AFE4410_GetOffsetDACtoSetting(1))<<24);// 8 bits
    settingsRegister |=  (((uint64_t)AFE4410_GetOffsetDACtoSetting(2))<<32);// 8 bits
    settingsRegister |=  (((uint64_t)AFE4410_GetOffsetDACtoSetting(3))<<40);// 8 bits
    settingsRegister |=  (((uint64_t)AFE4410_GetOffsetDACtoSetting(4))<<48);// 8 bits

    settingsRegister |=  (((uint64_t)AFE4410_GetRFtoSetting(1))<<56);// 4 bits
    settingsRegister |=  (((uint64_t)AFE4410_GetRFtoSetting(2))<<60);// 4 bits

    //NRF_LOG_RAW_INFO("RF1 RF2,%d %d\r\n",AFE4410_GetRF(1),AFE4410_GetRF(1));
    return settingsRegister;
}

void AFE4410_serviceAFEWriteQueue(void)
{
    if(AmountOfRegistersToWrite > 0) {
        uint8_t count = 0;
        for(count = 0; count<REGISTERS_TO_WRITE_LENGTH; count++) {
            AFE4410_writeRegWithWriteEnable(LocationsOfRegistersToWrite[count],RegistersToWrite[count]);

            AmountOfRegistersToWrite--;
            if(AmountOfRegistersToWrite == 0) {
                break;
            }
        }
        afeSettingsExecuted = TRUE;
    }
}
uint32_t AFE4410_getRegisterFromLiveArray (uint8_t reg)
{
    return liveAFERegister[reg];
}

void AFE4410_addWriteCommandToQueue(uint8_t reg, uint32_t registerValue)
{
    LocationsOfRegistersToWrite[AmountOfRegistersToWrite] = reg;
    RegistersToWrite[AmountOfRegistersToWrite] = registerValue;
    liveAFERegister[reg] = registerValue;
    AmountOfRegistersToWrite++;
}

uint16_t pps_getHR(void)
{
#ifdef LIFEQ
//return PP_Get_HR();
#else
    return 0;
#endif
}

void PPSI26x_init(void)
{
#ifdef LIFEQ
    //Initialize The LifeQ Algorithm
    //printf("r1 r2 r3 r4 r5 r6 r7:%d %d %d %d %d %d %d\r\n",r1,r2,r3,r4,r5,r6,r7);
#endif
    AGC_InitializeAGC();
    settingsCheckSampleQueue[0]=0;
    settingsCheckSampleQueue[1]=0;
}


void ALGSH_retrieveSamplesAndPushToQueue(void)
{
    static uint64_t retrievedAfeSettings = 0;
#if USE_FIFO
    uint8_t data[300];                          //25Hz * 4 spl * 3byte = 300
    uint8_t fifoDepth = USE_FIFO_DEPTH;
    uint32_t fifoLength = fifoDepth * 4 * 3;    //25Hz * 4 spl * 3byte = 300
    uint32_t ret;
    uint32_t a, b;
    uint32_t tmp_raw;
    int32_t tmp_32bit;
    int16_t tmp_16bit;

    AFE4410_enableWriteFIFO();
    I2C_writeReg(0x51, 0x100);
    I2C_writeReg(0x51, 0x00);
    AFE4410_enableReadFIFO();
    //tmp_raw = I2C_readReg(0x6D);

    for (a = 0; a < fifoLength / 60; a ++) {                    // 5 = 300/60
        ret = I2C_readReg_FIFO(0xff, &data[0 + a * 60], 60);    //FIFO register fixed 0xFF
    }
    for (a = 0; a < fifoLength; ) {
        //LED2 IR
        tmp_raw = data[a]<<16 | data[a+1]<<8 | data[a+2];
        tmp_32bit = afeConvP32(tmp_raw);
        tmp_16bit = afeConvP16(tmp_raw);
        afe_32bit_RAW_struct1.afeMeasLED2 = tmp_raw;
        afe_32bit_struct1.afeMeasLED2 = tmp_32bit;
        afe_struct1.afeMeasLED2 = tmp_16bit;
        a = a + 3;

        //LED3  RED
        tmp_raw = data[a]<<16 | data[a+1]<<8 | data[a+2];
        tmp_32bit = afeConvP32(tmp_raw);
        tmp_16bit = afeConvP16(tmp_raw);
        afe_32bit_RAW_struct1.afeMeasALED2 = tmp_raw;
        afe_32bit_struct1.afeMeasALED2 = tmp_32bit;
        afe_struct1.afeMeasALED2 = tmp_16bit;
        a = a + 3;

        //LED1  Green
        tmp_raw = data[a]<<16 | data[a+1]<<8 | data[a+2];
        tmp_32bit = afeConvP32(tmp_raw);
        tmp_16bit = afeConvP16(tmp_raw);
        afe_32bit_RAW_struct1.afeMeasLED1 = tmp_raw;
        afe_32bit_struct1.afeMeasLED1 = tmp_32bit;
        afe_struct1.afeMeasLED1 = tmp_16bit;
        a = a + 3;

        //AMB
        tmp_raw = data[a]<<16 | data[a+1]<<8 | data[a+2];
        tmp_32bit = afeConvP32(tmp_raw);
        tmp_16bit = afeConvP16(tmp_raw);
        afe_32bit_RAW_struct1.afeMeasALED1 = tmp_raw;
        afe_32bit_struct1.afeMeasALED1 = tmp_32bit;
        afe_struct1.afeMeasALED1 = tmp_16bit;
        a = a + 3;

        uint16_t cbuff_head = (LEDbuffHead + 1);           // next position in circular buffer

        if (cbuff_head >= (AFE_BUFF_LEN << 1)) {
            cbuff_head = 0;
        }
        if (cbuff_head - LEDbuffTail >= AFE_BUFF_LEN){
			LEDbuffTail = cbuff_head - AFE_BUFF_LEN;		//max buffer size is AFE_BUFF_LEN
		}

        if (cbuff_head - rd_ptr >= AFE_BUFF_LEN){
			rd_ptr = cbuff_head - AFE_BUFF_LEN;		        //max buffer size is AFE_BUFF_LEN
		}
        
        uint16_t tmphead = (cbuff_head >= AFE_BUFF_LEN) ? (cbuff_head - AFE_BUFF_LEN) : cbuff_head;
        hr_led_green[tmphead] = afe_struct1.afeMeasLED1;          //green
        hr_led_amb[tmphead]   = afe_struct1.afeMeasALED1;         //amb
        hr_led_ir[tmphead]    = afe_struct1.afeMeasLED2;          //IR
        hr_led_red[tmphead]   = afe_struct1.afeMeasALED2;         //red
        // LOG_DEBUG_PPS("[hrm] write to buffer: LEDbuffHead: %d, LEDbuffTail: %d, new data: %d %d %d %d \n", \
            tmphead, LEDbuffTail, hr_led_green[tmphead], hr_led_amb[tmphead], hr_led_ir[tmphead], hr_led_red[tmphead]);

        AccXbuff[tmphead] = 32768;
        AccYbuff[tmphead] = 32768;
        AccZbuff[tmphead] = 32768;

        AFE_SettingsBuff[tmphead] = retrievedAfeSettings;
        LEDbuffHead = cbuff_head;
    }
#else
    AFE4410_retrieveRawAFEValues(&afe_32bit_RAW_struct1,&afe_32bit_struct1,&afe_struct1); //Retrieve PPG samples from the AFE
    uint16_t tmphead = ( LEDbuffHead + 1);              // next position in circular buffer
    if (tmphead >= AFE_BUFF_LEN) {
        tmphead = 0;
    }
    hr_led_green[tmphead] = afe_struct1.afeMeasLED1;
    hr_led_amb[tmphead] = afe_struct1.afeMeasALED1;
    hr_led_ir[tmphead] = afe_struct1.afeMeasLED2;
    hr_led_red[tmphead] = afe_struct1.afeMeasALED2;

    AccXbuff[tmphead] = 32768;
    AccYbuff[tmphead] = 32768;
    AccZbuff[tmphead] = 32768;

    AFE_SettingsBuff[tmphead] = retrievedAfeSettings;
    LEDbuffHead = tmphead;
#endif
    retrievedAfeSettings = AFE4410_getAFESettingsUint(); // Retrive AFE settings for the current sample set
    AFE4410_serviceAFEWriteQueue();                      // Write command in queue to AFE
}

void ALGSH_dataToAlg(void)
{
    uint16_t tmptail, cbuff_tail;
    uint32_t a;
#if USE_FIFO
    for (a = 0; a < USE_FIFO_DEPTH; a ++)
#endif
    {
        if (LEDbuffHead != LEDbuffTail) {
            
            cbuff_tail = LEDbuffTail;
            if (cbuff_tail >= (AFE_BUFF_LEN << 1)) {
                cbuff_tail = 0;
                // LOG_DEBUG_PPS("\n");
            }
            uint16_t tmptail = (cbuff_tail >= AFE_BUFF_LEN) ? (cbuff_tail - AFE_BUFF_LEN) : cbuff_tail;
            
            static uint64_t lastAFEset = 0;
            if (firstsample) {               // This skips the very first sample since it is 0's
                lastAFEset = 0;
                LEDbuffTail = tmptail;       // increase tail index
                firstsample = FALSE;
                break;
            }

            etcdata.OffDacCurrent1 = AFE4410_settingsUintGetIsub(1, lastAFEset);
            etcdata.OffDacCurrent2 = AFE4410_settingsUintGetIsub(2, lastAFEset);
            etcdata.OffDacCurrent3 = AFE4410_settingsUintGetIsub(3, lastAFEset);
            etcdata.OffDacCurrent4 = AFE4410_settingsUintGetIsub(4, lastAFEset);

            etcdata.tempRf1 = AFE4410_settingsUintGetRf(1, lastAFEset);
            etcdata.tempRf2 = AFE4410_settingsUintGetRf(2, lastAFEset);
            etcdata.tempCf1 = AFE4410_GetCF(1);
            etcdata.tempCf2 = AFE4410_GetCF(2);

            etcdata.ledCurrent1 = AFE4410_settingsUintGetLedCurrent(1, lastAFEset);
            etcdata.ledCurrent2 = AFE4410_settingsUintGetLedCurrent(2, lastAFEset);
            etcdata.ledCurrent3 = AFE4410_settingsUintGetLedCurrent(3, lastAFEset);

            // LOG_DEBUG_PPS("[hrm] read to agc: LEDbuffHead: %d, tmptail: %d, new data: %d %d %d %d \n", \
                LEDbuffHead, tmptail, hr_led_green[tmptail], hr_led_amb[tmptail], hr_led_ir[tmptail], hr_led_red[tmptail]);

            /*Populate LED samples structs */
            // This function sends the data to the AGC
            agc_state_t agcState = AGC_ServiceAGC(hr_led_green[tmptail],  \
                                                  hr_led_amb[tmptail], \
                                                  hr_led_ir[tmptail],  \
                                                  hr_led_red[tmptail], \
                                                  detectChangeInAfeSettings(AFE_SettingsBuff[tmptail]));
            // alg_input_selection_t stateSelector = PP_HEART_RATE;
            if ((agcState == AGC_STATE_CALIBRATION) \
                || (agcState == AGC_STATE_CALIBRATION_WAITINGFORADJUSTMENT) \
                || (agcState == AGC_STATE_CALIBRATION_ERRORSTATE)) {
                ;
			    //stateSelector = PP_CALIBRATION;
            } else {
                //stateSelector = PP_HEART_RATE;
                HRM_sample_count ++;
            }

            /*Populate LED samples structs */
            led_sample_t greenLed;
            led_sample_t blankLed;
            led_sample_t redLed;
            led_sample_t irLed;

            greenLed.sample = hr_led_green[tmptail];
            greenLed.led = LQ_LED_GREEN;
            greenLed.isub = etcdata.OffDacCurrent1;
            greenLed.led_curr_mAx10 = etcdata.ledCurrent1 * 4;
            greenLed.rf_kohm = etcdata.tempRf1;

            blankLed.sample = hr_led_amb[tmptail];
            blankLed.led = LQ_LED_AMBIENT;
            blankLed.isub = etcdata.OffDacCurrent2;
            blankLed.led_curr_mAx10 = 0;
            blankLed.rf_kohm = etcdata.tempRf1;

            redLed.sample = hr_led_red[tmptail];
            redLed.led = LQ_LED_RED;
            redLed.isub = etcdata.OffDacCurrent4;
            redLed.led_curr_mAx10 = etcdata.ledCurrent3 * 4;
            redLed.rf_kohm = etcdata.tempRf2;

            irLed.sample = hr_led_ir[tmptail];
            irLed.led = LQ_LED_INFRARED;
            irLed.isub = etcdata.OffDacCurrent3;
            irLed.led_curr_mAx10 = etcdata.ledCurrent2 * 4;
            irLed.rf_kohm = etcdata.tempRf2;

            PPS_skin_detect(irLed);                 //skin detect rawdata
            uint8_t skin = PPS_get_skin_detect();
            //PPS_Gled_on_by_skin_etect(skin);
//===========================================================================================
//添加算法接口
//=============================================================================================
            if (1 == skin) {                        // skin detect control add data to algorithm		
#ifdef LIFEQ							
                //This function sends the data to the Alg
                // hqret_t algRet = PP_addSamplesTI4404(AccXbuff[tmptail], \
                                                        AccYbuff[tmptail], \
                                                        AccZbuff[tmptail], \
                                                        &greenLed, \
                                                        &blankLed, \
                                                        &redLed,   \
                                                        &irLed,    \
                                                        stateSelector);
                if(algRet != RET_OK) {
                    //printf("AddAfeSample returns ERROR \n");
                }
#endif
            }
            lastAFEset = AFE_SettingsBuff[tmptail];
            LEDbuffTail = cbuff_tail + 1;                 // increase tail index
        }
    }//end of for (a = 0; a < 25; a ++)
}

uint16_t GetHRSampleCount(void)
{
    return HRM_sample_count;
}

void ClrHRSampleCount(void)
{
    HRM_sample_count=0;
}

uint32_t threshold_high=8500;   // high value - low value >32768/100+1
uint32_t threshold_low =6500;
uint8_t is_skin=0;
void PPS_set_skin_detect_thr(uint32_t high,uint32_t low)
{
    threshold_high = high;
    threshold_low = low;
    //is_skin=0;
}

void PPS_skin_detect(led_sample_t irLed)
{
    //static led_sample_t irLed;
    //int64_t tempIrDC;
    //irLed=iLed;
    //irLed.sample = irLedBuf[tmptail];
    //irLed.led = LQ_LED_INFRARED;
    //irLed.isub = evdata.tempIsubtimesTen3;
    //irLed.led_curr_mAx10 = evdata.ledCurrent2 * 16;
    //irLed.rf_kohm = evdata.tempRf2;
    //Vtia(diff) = Vtia(+)-Vtia=2 * Iin * RF =>Iin = Vtia/2/Rf
    //1uA=2V/2/Rf/1000 000
    //1uA:400 0000 for ADC Raw data //1uA:56636 for ADC conver uint16
    //IrDc=irLed1.isub*65536/1uA+irLed1.sample-32768

    static uint8_t cnt_y=0,cnt_n=0;

    //tempIrDC = ((0-(uint32_t)irLed.isub)*65536/10+irLed.sample-32768)/100;
    int64_t tempIrDC = ((0-(int32_t)irLed.isub)*65536/10+irLed.sample-32768)/100;
    if(tempIrDC>threshold_high) { // threshold high value
        cnt_y++;
        cnt_n=0;
    } else if(tempIrDC<threshold_low) { // threshold low value
        cnt_n++;
        cnt_y=0;
    }
    if(cnt_y>=10) { // 10 time to check
        cnt_y=10;
        is_skin=1;
    }
    if(cnt_n>=10) { // 10 time to check
        cnt_n=10;
        is_skin=0;
    }
    //NRF_LOG_RAW_INFO("rd,i,rf,cur:%d %d %d %d %d %d\r\n",irLed.sample,irLed.isub,irLed.rf_kohm,irLed.led_curr_mAx10,tempIrDC,is_skin);
}

bool PPS_get_skin_detect(void)
{
    return is_skin;
}

void PPS_Gled_on_by_skin_etect(uint8_t onSkin)
{
    static uint8_t GledOn=0,GledOff=0;
    if(onSkin) {
        GledOn++;
        GledOff=0;
    } else {
        GledOff++;
        GledOn=0;
    }
    if(GledOn>=1*25 && 0 == green_led_on_flag) { //1*25,25sample is one second,so it will delay 1 second to open green LED
        AFE4410_setLedCurrent_P(1,111);// Green  //DEFAULT_GREEN_CURRENT=55
        GledOn=2;
        green_led_on_flag=1;
    }
    if(GledOff>=3*25 && 1 == green_led_on_flag) { //3*25,25sample is one second,so it will delay 3 second to close green LED
        AFE4410_setLedCurrent_P(1,0);// Green
        GledOff=5;
        green_led_on_flag=0;
    }
}


static bool detectChangeInAfeSettings(uint64_t settings)
{
    bool retVal = FALSE;
    if(settingsCheckSampleQueue[0] != settingsCheckSampleQueue[1]) {
        retVal = TRUE;
    }
    settingsCheckSampleQueue[1] = settingsCheckSampleQueue[0];
    settingsCheckSampleQueue[0] = settings;
    return retVal;
}
void init_PPSI26x_register(void)
{
    int i;
    //PPS_DEBUG("init_register\r\n");
    AFE4410_Internal_25Hz();
    /*
    for(i=0; i<82; i++,i++) {
        PPSI26x_writeReg(init_registers[i],init_registers[i+1]);
    }*/

    //AFE4410_enableReadFIFO();
}



