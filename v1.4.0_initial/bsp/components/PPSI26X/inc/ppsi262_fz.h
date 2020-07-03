/************************************************************************//*!
 *  @file    ppsi262.h
 *
 *  @brief   Header file of ppsi262.c
 *
 *  @author  huang_haijun@163.com
 *
 *  @par     History:
 *            - 11/20/2017  0.10 -> First version
 ***************************************************************************/
#ifndef __PPSI262FZ_H__
#define __PPSI262FZ_H__


#ifdef __cplusplus
 extern "C" {
#endif

#define PPSI262_I2C_ADDR                    0x5B

#define PPSI262_REG_CONTROL0                0X00
#define PPSI262_REG_LED2STC                 0x01
#define PPSI262_REG_LED2ENDC                0x02
#define PPSI262_REG_LED1LEDSTC              0x03
#define PPSI262_REG_LED1LEDENDC             0x04
#define PPSI262_REG_ALED2STC                0x05
#define PPSI262_REG_ALED2ENDC               0x06
#define PPSI262_REG_LED1STC                 0x07
#define PPSI262_REG_LED1ENDC                0x08
#define PPSI262_REG_LED2LEDSTC              0x09
#define PPSI262_REG_LED2LEDENDC             0x0a
#define PPSI262_REG_ALED1STC                0x0b
#define PPSI262_REG_ALED1ENDC               0x0c
#define PPSI262_REG_LED2CONVST              0x0d
#define PPSI262_REG_LED2CONVEND             0x0e
#define PPSI262_REG_ALED2CONVST             0x0f
#define PPSI262_REG_ALED2CONVEND            0x10
#define PPSI262_REG_LED1CONVST              0x11
#define PPSI262_REG_LED1CONVEND             0x12
#define PPSI262_REG_ALED1CONVST             0x13
#define PPSI262_REG_ALED1CONVEND            0x14
#define AFE4405_REG_ADCRSTSTCT0             0x15
#define AFE4405_REG_ADCRSTENDCT0            0x16
#define AFE4405_REG_ADCRSTSTCT1             0x17
#define AFE4405_REG_ADCRSTENDCT1            0x18
#define AFE4405_REG_ADCRSTSTCT2             0x19
#define AFE4405_REG_ADCRSTENDCT2            0x1a
#define AFE4405_REG_ADCRSTSTCT3             0x1b
#define AFE4405_REG_ADCRSTENDCT3            0x1c
#define PPSI262_REG_PRPCOUNT                0x1d
#define PPSI262_REG_CONTROL1                0x1e
#define PPSI262_REG_TIAGAIN3                0x1f
#define PPSI262_REG_TIAGAIN1                0x20
#define PPSI262_REG_TIAGAIN2                0x21
#define PPSI262_REG_LEDCNTRL                0x22
#define PPSI262_REG_CONTROL2                0x23
#define PPSI262_REG_SDOUT                   0x29
#define PPSI262_REG_LED2VAL                 0x2A
#define PPSI262_REG_ALED2VAL                0x2B
#define PPSI262_REG_LED1VAL                 0x2C
#define PPSI262_REG_ALED1VAL                0x2D
#define AFE4405_REG_LED2ALED2VAL            0x2E
#define AFE4405_REG_LED1ALED1VAL            0x2F
#define AFE4405_REG_CONTROL3                0x31

#if 0
#define AFE4404_REG_PROG_TG_STC             0x34
#define AFE4404_REG_PROG_TG_ENDC            0x35
#endif

#define PPSI262_REG_LED3LEDSTC              0x36
#define PPSI262_REG_LED3LEDENDC             0x37
#define PPSI262_REG_CLKDIV2                 0x39
#define PPSI262_REG_OFFDAC                  0x3a

#define PPSI262_REG_FIFOINT                 0x42
#define PPSI262_REG_LED4LEDSTC              0x43
#define PPSI262_REG_LED4LEDENDC             0x44
#define PPSI262_REG_PDSEL                   0x4e

#define PPSI262_REG_DATARDYSTC              0x52
#define PPSI262_REG_DATARDYENDC             0x53

#define PPSI262_REG_DYNTIASTC               0x64
#define PPSI262_REG_DYNTIAENDC              0x65

#define PPSI262_REG_DYNADCSTC               0x66
#define PPSI262_REG_DYNADCENDC              0x67

#define PPSI262_REG_DYNCLKSTC               0x68
#define PPSI262_REG_DYNCLKENDC              0x69

#define PPSI262_REG_SLPSTC                  0x6A
#define PPSI262_REG_SLPENDC                 0x6B





/* Register function bit definition */

/* PPSI262_REG_CONTROL0 (0x00) definitions */
#define PPSI262_ULP_DISABLE                 (0<<5)
#define PPSI262_ULP_ENABLE                  (1<<5)

/* PPSI262_REG_CONTROL1 (0x1e) definitions */
#define PPSI262_TIMER_EN                    (1<<8)
#define PPSI262_NUMAV_1                      0
#define PPSI262_NUMAV_2                      1
#define PPSI262_NUMAV_3                      2
#define PPSI262_NUMAV_4                      3
#define PPSI262_NUMAV_5                      4
#define PPSI262_NUMAV_6                      5
#define PPSI262_NUMAV_7                      6
#define PPSI262_NUMAV_8                      7
#define PPSI262_NUMAV_9                      8
#define PPSI262_NUMAV_10                     9
#define PPSI262_NUMAV_11                     10
#define PPSI262_NUMAV_12                     11
#define PPSI262_NUMAV_13                     12
#define PPSI262_NUMAV_14                     13
#define PPSI262_NUMAV_15                     14
#define PPSI262_NUMAV_16                     15

/* AFE4405_REG_TIAGAIN (0x20) definitions */
#define AFE4405_TIAGAIN_SINGLE              (0<<15)
#define AFE4405_TIAGAIN_SEPARATE            (1<<15)

#define AFE4405_TIACF2_5PF                   (0<<3)
#define AFE4405_TIACF2_2_5PF                 (1<<3)
#define AFE4405_TIACF2_10PF                  (2<<3)
#define AFE4405_TIACF2_7_5PF                 (3<<3)
#define AFE4405_TIACF2_20PF                  (4<<3)
#define AFE4405_TIACF2_17_5PF                (5<<3)
#define AFE4405_TIACF2_25PF                  (6<<3)
#define AFE4405_TIACF2_22_5PF                (7<<3)

#define AFE4405_TIAGAIN2_500K                (0<<0)
#define AFE4405_TIAGAIN2_250K                (1<<0)
#define AFE4405_TIAGAIN2_100K                (2<<0)
#define AFE4405_TIAGAIN2_50K                 (3<<0)
#define AFE4405_TIAGAIN2_25K                 (4<<0)
#define AFE4405_TIAGAIN2_10K                 (5<<0)
#define AFE4405_TIAGAIN2_1M                  (6<<0)
#define AFE4405_TIAGAIN2_2M                  (7<<0)

/* AFE4405_REG_TIAGAIN1 (0x21) definitions */
#define PPSI262_IFSOFFDAC_1X                 (0<<12)
#define PPSI262_IFSOFFDAC_2X                 (3<<12)
#define PPSI262_IFSOFFDAC_4X                 (5<<12)
#define PPSI262_IFSOFFDAC_8X                 (7<<12)

#define AFE4405_TIACF1_5PF                   (0<<3)
#define AFE4405_TIACF1_2_5PF                 (1<<3)
#define AFE4405_TIACF1_10PF                  (2<<3)
#define AFE4405_TIACF1_7_5PF                 (3<<3)
#define AFE4405_TIACF1_20PF                  (4<<3)
#define AFE4405_TIACF1_17_5PF                (5<<3)
#define AFE4405_TIACF1_25PF                  (6<<3)
#define AFE4405_TIACF1_22_5PF                (7<<3)

#define AFE4405_TIAGAIN1_500K                (0<<0)
#define AFE4405_TIAGAIN1_250K                (1<<0)
#define AFE4405_TIAGAIN1_100K                (2<<0)
#define AFE4405_TIAGAIN1_50K                 (3<<0)
#define AFE4405_TIAGAIN1_25K                 (4<<0)
#define AFE4405_TIAGAIN1_10K                 (5<<0)
#define AFE4405_TIAGAIN1_1M                  (6<<0)
#define AFE4405_TIAGAIN1_2M                  (7<<0)

/* PPSI262_REG_LEDCNTRL (0x22) definitions */
#define AFE4405_ILED1_LSHIFT                 (18)
#define AFE4405_ILED2_LSHIFT                 (20)
#define AFE4405_ILED3_LSHIFT                 (22)

#define AFE4405_ILED1_MSHIFT                 (0)
#define AFE4405_ILED2_MSHIFT                 (6)
#define AFE4405_ILED3_MSHIFT                 (12)

/* PPSI262_REG_CONTROL2 (0x23) definitions */
#define PPSI262_TRANSMIT_DYNAMIC             (1<<20)
#define PPSI262_ILED_2X                      (1<<17)
#define PPSI262_ENSEPGAIN4                   (1<<15)
#define PPSI262_ADC_DYNAMIC                  (1<<14)
#define PPSI262_INTCLK_ENABLE                (1<<9)
#define PPSI262_TIA_DYNAMIC                  (1<<4)
#define PPSI262_RESTADC_DYNAMIC              (1<<3)
#define PPSI262_AFERX_PWN                    (1<<1)
#define PPSI262_AFE_PWN                      (1<<0)

/* AFE4405_REG_CLKDIV1 (0x29) definitions */
#define AFE4405_CLKOUT_DISABLE               (0<<9)
#define AFE4405_CLKOUT_ENABLE                (1<<9)

#define AFE4405_CLKOUT_4MHZ                   0
#define AFE4405_CLKOUT_2MHZ                   1
#define AFE4405_CLKOUT_1MHZ                   2
#define AFE4405_CLKOUT_500KHZ                 3
#define AFE4405_CLKOUT_250KHZ                 4
#define AFE4405_CLKOUT_125KHZ                 5
#define AFE4405_CLKOUT_62_5KHZ                6
#define AFE4405_CLKOUT_31_25KHZ               7

/* AFE4405_REG_CONTROL3 (0x31) definitions */
#define AFE4405_PDDISCONNECT_DISABLE         (0<<10)
#define AFE4405_PDDISCONNECT_ENABLE          (1<<10)

#define AFE4405_INPUTSHORT_DISABLE           (0<<5)
#define AFE4405_INPUTSHORT_ENABLE            (1<<5)

#define AFE4405_EXTCLK_8_12MHZ                0
#define AFE4405_EXTCLK_32_48MHZ               1
#define AFE4405_EXTCLK_48_60MHZ               3
#define AFE4405_EXTCLK_16_24MHZ               4
#define AFE4405_EXTCLK_4_6MHZ                 5
#define AFE4405_EXTCLK_24_36MHZ               6

/* PPSI262_REG_CLKDIV2 (0x39) definitions */
#define PPSI262_CLKDIV_128KHZ                 0
#define PPSI262_CLKDIV_64KHZ                  4
#define PPSI262_CLKDIV_32KHZ                  5
#define PPSI262_CLKDIV_16KHZ                  6
#define PPSI262_CLKDIV_8KHZ                   7

 /* PPSI262_REG_FIFOINT (0x42) definitions */
#define PPSI262_INTMUX3_INTOUT2              (0<<22)
#define PPSI262_INTMUX3_DATADRDY             (1<<22)


#define PPSI262_INTMUX2_INTOUT1              (0<<20)

#define PPSI262_FIFOEARLY_00                 (0<<14)

#define PPSI262_INTMUX1_DATARDY              (0<<4)

#define PPSI262_FREQCAL_STDCNT               (5120 - 1)   /* (40ms * 128KHz) */
#define PPSI262_FREQCAL_MAXCNT                25
#define PPSI262_FREQCAL_INICNT                2

#define AFE4405_ILED1X_MAX                    13107200    /* 50mA  * 2^18    */
#define PPSI262_ILED2X_MAX                    26214400    /* 100mA * 2^18    */
#define PPSI262_ILED2X_10MA                   2621440     /* 10mA * 2^18    */

void ppsi262_capture_isr(void);
void ppsi262_gpio_isr(void);

void ppsi262_getPdValue(int32_t *grn, int32_t *red, int32_t *ir, int32_t *amb);
void ppsi262_setGain(uint8_t irGain, uint8_t redGain, uint8_t grnGain);
void ppsi262_setValue(uint32_t irVal, uint32_t redVal, uint32_t grnVal);


void ppsi262_init(void);
void ppsi262_start(void);
void ppsi262_stop(void);


#ifdef __cplusplus
}
#endif

#endif /* __PPSI262FZ_H__ */

