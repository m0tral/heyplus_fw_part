/***************************************************

 file :     AFE4404_API.c
 Author :  Dominic Kim
 Created : 2016.03.17
 brief :  Control and operation functions for TI AFE 4405

 Copyright (c) 2016 PARTRON SensorLAB. Alll rights reserved.

****************************************************/

#ifndef AFE4405_HW_INCLUDED
#define AFE4405_HW_INCLUDED

//Functions used by HealthQ for error definition and reporting. User should have hqerror, hqutils is spesific to HealthQ's internal error reporting methods
//#include "hw_config.h"
#include "hqerror.h"   // LifeQ error definitions file
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "phys_calc.h"

//#include "phys_calc.h"
/* Weight definitions for variable sampling */

//#define LIFEQ


#define AFE_CONTROL0           0x0
#define AFE_LED2STC            0x1     //01        
#define AFE_LED2ENDC           0x2      //02      
#define AFE_LED1LEDSTC         0x3      //03
#define AFE_LED1LEDENDC        0x4      //04
#define AFE_ALED2STC           0x5      //05
#define AFE_ALED2ENDC          0x6      //06
#define AFE_LED1STC            0x7      //07
#define AFE_LED1ENDC           0x8      //08
#define AFE_LED2LEDSTC         0x9      //09
#define AFE_LED2LEDENDC        0xA      //0A
#define AFE_ALED1STC           0xB      //0B
#define AFE_ALED1ENDC          0xC      //0C
#define AFE_LED2CONVST         0xD      //0D
#define AFE_LED2CONVEND        0xE      //0E
#define AFE_ALED2CONVST        0xF      //0F
#define AFE_ALED2CONVEND       0x10      //10
#define AFE_LED1CONVST         0x11      //11
#define AFE_LED1CONVEND        0x12      //12
#define AFE_ALED1CONVST        0x13      //13
#define AFE_ALED1CONVEND       0x14      //14
#define AFE_ADCRSTSTCT0        0x15      //15
#define AFE_ADCRSTENDCT0       0x16      //16
#define AFE_ADCRSTSTCT1        0x17      //17
#define AFE_ADCRSTENDCT1       0x18      //18
#define AFE_ADCRSTSTCT2        0x19      //19
#define AFE_ADCRSTENDCT2       0x1A      //1A
#define AFE_ADCRSTSTCT3        0x1B      //1B
#define AFE_ADCRSTENDCT3       0x1C      //1C
#define AFE_PRPCOUNT           0x1D      //1D
#define AFE_CONTROL1           0x1E      //1E
#define AFE_SPARE1             0x1F      //1F
#define AFE_TIAGAIN            0x20      //20
#define AFE_TIAAMBGAIN         0x21      //21
#define AFE_LEDCNTRL           0x22      //22
#define AFE_CONTROL2           0x23      //23
#define AFE_SPARE2             0x24      //24
#define AFE_SPARE3             0x25      //25
#define AFE_SPARE4             0x26      //26
#define AFE_RESERVED1          0x27      //27
#define AFE_RESERVED2          0x28      //28
#define AFE_ALARM              0x29      //29
#define AFE_LED2VAL            0x2A      //2A
#define AFE_ALED2VAL           0x2B      //2B
#define AFE_LED1VAL            0x2C      //2C
#define AFE_ALED1VAL           0x2D      //2D
#define AFE_LED2ALED2VAL       0x2E     //2E
#define AFE_LED1ALED1VAL       0x2F      //2F
#define AFE_DIAG               0x30      //30
#define AFE_EXT_CLK_DIV_REG    0x31      //31
#define AFE_DPD1STC            0x32      //32
#define AFE_DPD1ENDC           0x33      //33
#define AFE_DPD2STC            0x34     //34
#define AFE_DPD2ENDC           0x35     //35
#define AFE_REFSTC             0x36     //36
#define AFE_REFENDC            0x37     //37
#define AFE_RESERVED3          0x38     //38
#define AFE_CLK_DIV_REG        0x39     //39
#define AFE_DAC_SETTING_REG    0x3A    //3A
#define AFE_DAC_SETTING_REG_2  0x3E    //3E


typedef struct {
    uint16_t afeMeasLED1;
    uint16_t afeMeasALED1;
    uint16_t afeMeasLED2;
    uint16_t afeMeasALED2;
    uint16_t afeDiffLED1;
    uint16_t afeDiffLED2;
    uint8_t LifeQhrm;
} afe_data_struct_t;

typedef struct {
    int32_t afeMeasLED1;
    int32_t afeMeasALED1;
    int32_t afeMeasLED2;
    int32_t afeMeasALED2;
    int32_t afeDiffLED1;
    int32_t afeDiffLED2;
} afe_32_bit_data_struct_t;

typedef struct {
    uint32_t afeMeasLED1;
    uint32_t afeMeasALED1;
    uint32_t afeMeasLED2;
    uint32_t afeMeasALED2;
} afe_32_RAW_bit_data_struct_t;

typedef struct {
    int16_t accX;
    int16_t accY;
    int16_t accZ;

    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;

    float FaccX;
    float FaccY;
    float FaccZ;



} specific_force_t;


typedef struct {
    int16_t OffDacCurrent1;
    int16_t OffDacCurrent2;
    int16_t OffDacCurrent3;
    int16_t OffDacCurrent4;
    uint16_t tempRf1;
    uint16_t tempRf2;
    uint16_t tempCf1;
    uint16_t tempCf2;
    uint8_t ledCurrent1;
    uint8_t ledCurrent2;
    uint8_t ledCurrent3;

    hqret_t ret2;
    hqret_t ret1;

} afe_data_etc_t;

typedef struct {
    int16_t Tx_OffDacCurrent1;
    int16_t Tx_OffDacCurrent2;
    int16_t Tx_OffDacCurrent3;
    int16_t Tx_OffDacCurrent4;
    uint16_t Tx_tempRf[5];
    uint16_t Tx_tempCf[5];
    uint8_t Tx_ledCurrent1;
    uint8_t Tx_ledCurrent2;
    uint8_t Tx_ledCurrent3;

} tx_Reg_Setting;

void AFE4410_retrieve32BitAFEValues (afe_32_bit_data_struct_t *afe_32bitstruct , afe_data_struct_t *afe_struct);

void AFE4410_Internal_25Hz(void);
void AFE4410_Internal_50Hz(void);
void AFE4410_Internal_100Hz(void);


uint16_t afeConvP16(int32_t val);
int32_t afeConvP32(int32_t val);

void AFE4410_SetOffsetDAC (uint8_t channel , int16_t OFFDACValue);
void AFE4410_setLedCurrent_P(uint8_t ledNumber , uint16_t currentvalue);
uint32_t current_Apply(uint8_t ledNo,uint16_t value);

int16_t AFE4410_GetOffsetDAC(uint8_t channel);

uint16_t AFE4410_GetRF(uint8_t rfNum);
uint8_t GetRFGain_Mode(void);
void AFE4410_setRf_Default(uint16_t rfValueInKiloOhms);
void AFE4410_setRf_ENSEPGAIN(uint8_t rfNum, uint16_t rfValueInKiloOhms);
void AFE4410_setRf_ENSEPGAIN_4(uint8_t rfNum, uint16_t rfValueInKiloOhms);

void AFE4410_setCf_Default(uint16_t cfValue);
void AFE4410_setCf_ENSEPGAIN(uint8_t cfNum, uint16_t cfValue);
void AFE4410_setCf_ENSEPGAIN_4(uint8_t cfNum, uint16_t cfValue);
uint16_t AFE4410_GetCF(uint8_t cfNum);


uint8_t AFE4410_getLedCurrent_P(uint8_t ledNumber);

void AFE4410_enableRead(void);
void AFE4410_enableWrite(void);
void AFE4410_enableReadFIFO(void);
void AFE4410_enableWriteFIFO(void);

uint32_t AFE4410_readRegWithReadEnable (uint8_t reg);
void AFE4410_writeRegWithWriteEnable(uint8_t reg, uint32_t registerValue);

uint32_t AFE4410_readRegWithoutReadEnable (uint8_t reg);
void AFE4410_writeRegWithoutWriteEnable(uint8_t reg, uint32_t registerValue);


void AFE4410_retrieveRawAFEValues (afe_32_RAW_bit_data_struct_t *afe_RAW_32bitstruct , afe_32_bit_data_struct_t *afe_32bitstruct , afe_data_struct_t *afe_struct );

void AFE4410_disableAFE(void);

uint8_t I2C_writeReg(uint8_t regaddr, uint32_t wdata);

uint32_t I2C_readReg(uint8_t regaddr);

uint32_t I2C_readReg_FIFO(uint8_t regaddr,uint8_t *read_fifo,uint32_t fifoLength);

uint32_t LEDVal_Read_4405_to_4404(uint32_t led4405);
uint32_t LEDVal_Write_4404_to_4405(uint32_t led4404);
uint32_t OffsetDAC_Read_4405_to_4404(uint32_t dac4405_1,uint8_t dac4405_2);
void OffsetDac_Write_4404_to_4405(uint32_t dac4404, uint32_t *dac4405);
//void AFE4405_Reg_Init();
int16_t AFE4410_settingsUintGetIsub(uint8_t channel, uint64_t inputArr);
uint16_t AFE4410_settingsUintGetRf(uint8_t channel, uint64_t inputArr);
uint8_t AFE4410_settingsUintGetLedCurrent(uint8_t ledNumber, uint64_t inputArr);
uint64_t AFE4410_getAFESettingsUint(void);
void AFE4410_serviceAFEWriteQueue(void);
uint8_t AFE4410_getLedCurrent(uint8_t ledNumber);
uint32_t AFE4410_getRegisterFromLiveArray (uint8_t reg);
int8_t AFE4410_getAmbientCurrent(uint8_t channel);
uint16_t AFE4410_getRf(uint8_t rfNum);
hqret_t AFE4410_setRf(uint8_t rfNum, uint16_t rfValueInKiloOhms);
hqret_t AFE4410_setAmbientCurrent(uint8_t channel , int8_t currentInMicroAmpsTimes10);
hqret_t AFE4410_setLedCurrent(uint8_t ledNumber, uint8_t currentTapSetting);
void AFE4410_addWriteCommandToQueue(uint8_t reg, uint32_t registerValue);
void AFE4410_enableInternalTimer (void);
void PPSI262_init(void);
static bool detectChangeInAfeSettings(uint64_t settings);

void ALGSH_retrieveSamplesAndPushToQueue(void);
void init_ppsi262_sensor(void);
void init_PPSI262_register(void);
void ALGSH_dataForAgc(void);
void ALGSH_dataToAlg(void);
uint16_t GetLEDBuffLenth(void);
uint16_t GetAccBuffLenth(void);
uint16_t GetHRSampleCount(void);
void ClrHRSampleCount(void);
uint16_t pps_getHR(void);
void PPS_skin_detect(led_sample_t irLed);
bool PPS_get_skin_detect(void);
void PPS_Gled_on_by_skin_etect(uint8_t onSkin);




uint32_t I2C_readReg_FIFO_LQ(uint8_t regaddr,uint8_t *read_fifo,uint16_t length);

extern unsigned char SW_I2C_WRITEn_Control( unsigned char , unsigned char , unsigned char , unsigned char *);
extern unsigned char SW_I2C_READn_Control(  unsigned char , unsigned char , unsigned char , unsigned char *);

uint32_t PPSI26x_readReg(uint8_t regaddr);
void PPSI26x_writeReg(uint8_t regaddr, uint32_t wdata);
uint32_t PPSI26x_readRegFIFO(uint8_t regaddr,uint8_t *read_fifo, uint32_t fifoLength);

bool ecg_is_lead_off(void);
#endif //AFE4405_HW_INCLUDED










