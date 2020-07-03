/************************************************************************//*!
 *  @file    ppsi262.c
 *  @brief   This file contains functions for PPSI262.
 *
 *  @author  huang_haijun@163.com
 *
 *  @par     History:
 *            - 11/17/2017  0.10 -> First version
 ***************************************************************************/
#include <stdio.h>
// #include "device.h"
// #include "debug.h"
#include "ry_utils.h"
// #include "iom.h"
#include "am_bsp.h"
#include "ppsi262_fz.h"
#include "algorithm.h"
#include "ryos.h"

/*********************************************************************
 * CONSTANTS
 */
#define DBG_PRINTF      LOG_DEBUG_PPS
#define APOLLO2
#define PPG_INT_PIN     PPS_INTERRUPT_PIN
#define PPG_RST_PIN     HRM_RESETZ_CTRL_PIN

/*
 * TYPES
 *******************************************************************************
 */


/*********************************************************************
 * VARIABLES
 */

const hrs_cfg_t hrm_cfg = { 0,                                                 /* hrsType                  */   
                            0,                                                 /* Reserved for alignment    */
                            0,                                                 /* PPG sensor type           */
                            0,                                                 /* IMU sensor type           */

                            500000,                                            /* Green dim target          */
                            0,                                                 /* Red dim target            */
                            500000,                                            /* Ir dim target             */
                            262144,                                            /* Current multiple          */

                            {1, 0, 2, 0},                                      /* Green gain                */
                            {PPSI262_ILED2X_MAX >> 1, 0, PPSI262_ILED2X_MAX},  /* Green peak current        */

                            {1, 0, 2, 0},                                      /* Red gain                  */
                            {0, 0, 0},                                         /* Red peak current          */

                            {1, 0, 2, 0},                                      /* Ir gain                   */
                            {PPSI262_ILED2X_MAX >> 1, 0, PPSI262_ILED2X_MAX},  /* Ir peak current           */  
};

/*********************************************************************
 * FUNCTIONS
 */

/**
 * @brief      PPSI262 register reading
 *
 * @param[in]  addr  Register address
 * @param[out] buff  Register data pointer
 *
 * @return     void
 */
static void ppsi262_readReg(uint8_t addr, uint32_t *buff)
{
    uint8_t *p;
    uint32_t tmpData;

    ry_hal_i2cm_rx_at_addr(I2C_IDX_HR, addr, (uint8_t*)&tmpData, 3);
    p = (uint8_t *)buff;
    *(p+0) = tmpData >> 16;
    *(p+1) = tmpData >> 8;
    *(p+2) = tmpData >> 0;
    *(p+3) = 0;
}

/**
 * @brief      PPSI262 register writing
 *
 * @param[in]  addr  Register address
 * @param[in]  data  Write data
 *
 * @return     void
 */
static void ppsi262_writeReg(uint8_t addr, uint32_t data)
{
    uint32_t tmpData;

    tmpData  = (data>>16) & 0xff;
    tmpData |= data & 0xff00;
    tmpData |= (data&0xff)<<16;

    ry_hal_i2cm_tx_at_addr(I2C_IDX_HR, addr, (uint8_t*)&tmpData, 3);
}

/**
 * @brief      Set PPSI262 reset low
 *
 * @param[in]  void
 *
 * @return     void
 */
static void ppsi262_rst_low(void)
{
#ifdef APOLLO2
    am_hal_gpio_out_bit_clear(PPG_RST_PIN);
    // am_hal_gpio_out_bit_clear(PPG_CTL_PIN);
#endif
}

/**
 * @brief      Set PPSI262 reset high
 *
 * @param[in]  void
 *
 * @return     void
 */
static void ppsi262_rst_high(void)
{
#ifdef APOLLO2
    am_hal_gpio_out_bit_set(PPG_RST_PIN);
    // am_hal_gpio_out_bit_clear(PPG_CTL_PIN);
#endif
}

/**
 * @brief      Disable the PPSI262 interrupt
 *
 * @param[in]  void
 *
 * @return     void
 */
static void ppsi262_int_disable(void)
{
#ifdef APOLLO2
    /* Clear the GPIO Interrupt (write to clear). */
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(PPG_INT_PIN));

    /* Enable the GPIO interrupt. */
    am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(PPG_INT_PIN));
#endif
}

/**
 * @brief      Disable the PPSI262 interrupt
 *
 * @param[in]  void
 *
 * @return     void
 */
static void ppsi262_int_enable(void)
{
#ifdef APOLLO2
    /* Clear the GPIO Interrupt (write to clear). */
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(PPG_INT_PIN));

    /* Enable the GPIO interrupt. */
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(PPG_INT_PIN));
#endif
}


/**
 * @brief      Set the initial register value of the PPSI262
 *
 * @param[in]  void
 *
 * @return     void
 */
static void ppsi262_regInit(void)
{
    /*
     * Set the internal clock division to 1MHz. Also the period counter to
     * sample rate about 25Hz (40ms). Note that the period counter value
     * should be calibrate later.
     */
    ppsi262_writeReg(PPSI262_REG_CONTROL0, PPSI262_ULP_ENABLE);
    ppsi262_writeReg(PPSI262_REG_CLKDIV2, PPSI262_CLKDIV_128KHZ);
    ppsi262_writeReg(PPSI262_REG_PRPCOUNT, PPSI262_FREQCAL_STDCNT);

    /* LED2 (IR) sample period, about 406.25us */
    ppsi262_writeReg(PPSI262_REG_LED2STC,  43);        /**< TE3    */
    ppsi262_writeReg(PPSI262_REG_LED2ENDC, 94);        /**< TE4    */

    /* LED1 (GRN) drive period, about 437.5us */
    ppsi262_writeReg(PPSI262_REG_LED1LEDSTC, 153);     /**< TE13   */
    ppsi262_writeReg(PPSI262_REG_LED1LEDENDC, 208);    /**< TE14   */

    /* LED3 (RED) sample period, about 406.25us */
    ppsi262_writeReg(PPSI262_REG_ALED2STC, 100);       /**< TE9    */
    ppsi262_writeReg(PPSI262_REG_ALED2ENDC, 151);      /**< TE10   */

    /* LED1 (GRN) sample period, about 406.25us */
    ppsi262_writeReg(PPSI262_REG_LED1STC, 157);        /**< TE15   */
    ppsi262_writeReg(PPSI262_REG_LED1ENDC, 208);       /**< TE16   */

    /* LED2 (IR) drive period, about 437.5us */
    ppsi262_writeReg(PPSI262_REG_LED2LEDSTC, 39);      /**< TE1    */
    ppsi262_writeReg(PPSI262_REG_LED2LEDENDC, 94);     /**< TE2    */

    /* AMB sample period, about 406.25us */
    ppsi262_writeReg(PPSI262_REG_ALED1STC, 214);       /**< TE21   */
    ppsi262_writeReg(PPSI262_REG_ALED1ENDC, 265);      /**< TE22   */

    /*
     * ADC convert timing, start 2 counts after sample finished. The duration
     * should be:
     *     56.5*NUMAV+67us. In case NUMAV=7, should be 462.5us ~= 60Te
     */

    /* LED2 (IR) convert period, about 468.75us */
    ppsi262_writeReg(PPSI262_REG_LED2CONVST, 96);      /**< TE5    */
    ppsi262_writeReg(PPSI262_REG_LED2CONVEND, 155);    /**< TE6    */

    /* LED3 (RED) convert period, about 468.75us */
    ppsi262_writeReg(PPSI262_REG_ALED2CONVST, 157);    /**< TE11   */
    ppsi262_writeReg(PPSI262_REG_ALED2CONVEND, 216);   /**< TE12   */

    /* LED1 (GRN) convert period, about 117us */
    ppsi262_writeReg(PPSI262_REG_LED1CONVST, 218);     /**< TE17   */
    ppsi262_writeReg(PPSI262_REG_LED1CONVEND, 277);    /**< TE18   */

    /* AMB convert period, about 117us */
    ppsi262_writeReg(PPSI262_REG_ALED1CONVST, 279);    /**< TE23   */
    ppsi262_writeReg(PPSI262_REG_ALED1CONVEND, 338);   /**< TE24   */

    /* LED3 (RED) drive period, about 437.5us */
    ppsi262_writeReg(PPSI262_REG_LED3LEDSTC, 96);      /**< TE7    */
    ppsi262_writeReg(PPSI262_REG_LED3LEDENDC, 151);    /**< TE8    */

    /* LED4 (Not used) drive period, about 227us */
    ppsi262_writeReg(PPSI262_REG_LED4LEDSTC, 210);     /**< TE19   */
    ppsi262_writeReg(PPSI262_REG_LED4LEDENDC, 265);    /**< TE20   */

    /* DATA_RDY period, about 32us */
    ppsi262_writeReg(PPSI262_REG_DATARDYSTC, 344);     /**< TE25   */
    ppsi262_writeReg(PPSI262_REG_DATARDYENDC, 348);    /**< TE26   */

    /* DYN_TIA period */
    ppsi262_writeReg(PPSI262_REG_DYNTIASTC, 0);
    ppsi262_writeReg(PPSI262_REG_DYNTIAENDC, 339);

    /* DYN_ADC period */
    ppsi262_writeReg(PPSI262_REG_DYNADCSTC, 0);
    ppsi262_writeReg(PPSI262_REG_DYNADCENDC, 339);

    /* DYN_CLK period */
    ppsi262_writeReg(PPSI262_REG_DYNCLKSTC, 0);
    ppsi262_writeReg(PPSI262_REG_DYNCLKENDC, 339);

    /* Deep sleep period */
    ppsi262_writeReg(PPSI262_REG_SLPSTC, 355);
    ppsi262_writeReg(PPSI262_REG_SLPENDC, 5093);

    /* Control register */
    ppsi262_writeReg(PPSI262_REG_CONTROL1, (PPSI262_TIMER_EN |
                                            PPSI262_NUMAV_8));

    /* TIA gain setting */
    ppsi262_writeReg(PPSI262_REG_TIAGAIN3, 0);
    ppsi262_writeReg(PPSI262_REG_TIAGAIN1, 0);
    ppsi262_writeReg(PPSI262_REG_TIAGAIN2, PPSI262_IFSOFFDAC_2X);

    /* ILED control, 0mA */
    ppsi262_writeReg(PPSI262_REG_LEDCNTRL, 0);

    /* System control register */
    ppsi262_writeReg(PPSI262_REG_CONTROL2, (PPSI262_TRANSMIT_DYNAMIC |
                                            PPSI262_ADC_DYNAMIC      |
                                            PPSI262_INTCLK_ENABLE    |
                                            PPSI262_TIA_DYNAMIC      |
                                            PPSI262_RESTADC_DYNAMIC));

    ppsi262_writeReg(PPSI262_REG_OFFDAC, 0);

    ppsi262_writeReg(PPSI262_REG_FIFOINT, PPSI262_INTMUX1_DATARDY);
    ppsi262_writeReg(PPSI262_REG_PDSEL, 0);
    ppsi262_writeReg(0x51, 0);
    ppsi262_writeReg(0x50, 0x08);
    ppsi262_writeReg(0x4b, 0x09);
    ppsi262_writeReg(0x3d, 0x00);
    ppsi262_writeReg(0x31, 0x20);
    ppsi262_writeReg(0x3A, 0x01);
}


/**
 * @brief      Calibrate the frequency basing on the system 32KHz
 *
 * @param[in]  void
 *
 * @return     void
 */
static void ppsi262_frequency_calibrate(void)
{
    uint8_t captureTimes;
    uint16_t period;
    uint32_t prevTimerCount;
    uint32_t currTimerCount;
    uint32_t prpctVal;
    uint32_t decimal;


    /* To calibrate the frequency, a timer with capture function is used.
     * The timer capture the PPSI262 interrupt period time and compare to
     * the expected value to get the most close frequency.
     */
    ppsi262_int_disable();

#if 0

    /* Enable capture A interrupt in STIMER */
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_CAPTUREA);

    /* Enable the timer interrupt in the NVIC. */
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_STIMER);

    /* Configure the STIMER */
    am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
    am_hal_stimer_config(AM_HAL_STIMER_XTAL_32KHZ | AM_HAL_STIMER_CAPTURE_A_ENABLE);

    /* Configure and start capture */
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_CAPTUREA);
    am_hal_stimer_capture_start(0, PPG_INT_PIN, AM_HAL_GPIO_RISING);
#endif

    pps_ctx_v.intteruptOccurred = false;
    period = 0;
    captureTimes = 0;
    prevTimerCount = 0;

    /**
     * Accumulate current actual period of the interrupt, so that to calculate
     * the target period value.
     */
    while(1)
    {
        //LOG_DEBUG_PPS("frequency calibrate, Period is %d\r\n", period);
        if (pps_ctx_v.intteruptOccurred == true)
        {
            pps_ctx_v.intteruptOccurred = false;

        #ifdef APOLLO2
            currTimerCount = am_hal_stimer_capture_get(0);
        #endif

            if (captureTimes <= PPSI262_FREQCAL_INICNT)
            {
                period = 0;
            }
            else
            {
                period += currTimerCount - prevTimerCount;
            }

            captureTimes++;
            prevTimerCount = currTimerCount;

            if (captureTimes >
                (PPSI262_FREQCAL_MAXCNT + PPSI262_FREQCAL_INICNT))
            {
                //LOG_DEBUG_PPS("Period is %d\r\n", period);
                break;
            }
        }

        /* To think about low power, here can add low power management. But
         * now, considering the short time, ignore it.
         */
    }

    /* Calculate the proper frequency value and set to the PPSI262. */
     prpctVal = (PPSI262_FREQCAL_STDCNT * 32768) / period;
     decimal  = (PPSI262_FREQCAL_STDCNT * 32768) % period;

     if ((decimal * period * 2) >= 1)
     {
         prpctVal++;
     }

     /* Update the register value */
     ppsi262_writeReg(PPSI262_REG_PRPCOUNT, prpctVal);

#ifdef APOLLO2
    /* Stop the timer & capture function */
    am_hal_stimer_capture_stop(0);
    am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR);
#endif

    ppsi262_int_enable();
}


/**
 * @brief      Initialize the PPSI262s
 *
 * @param[in]  void
 *
 * @return     void
 */
void ppsi262_init(void)
{
    /* Reset the PPSI262, force into power down mode */
    ppsi262_rst_high();
    am_util_delay_us(50);
    ppsi262_rst_low();
    am_util_delay_ms(1);
    ppsi262_rst_high();
}


/**
 * @brief      Start the PPSI262
 *
 * @param[in]  void
 *
 * @return     void
 */
void ppsi262_start(void)
{
    /* Reset the PPSI262, quit the power down mode */
    ppsi262_rst_high();
    am_util_delay_us(50);
    ppsi262_rst_low();
    am_util_delay_us(35);
    ppsi262_rst_high();

    /* Initialize the basic timing of the PPSI262 */
    ppsi262_regInit();

    /* Calibrate the frequency of the PPSI262 */
    // ppsi262_frequency_calibrate();
}

/**
 * @brief      Stop the PPSI262
 *
 * @param[in]  void
 *
 * @return     void
 */
void ppsi262_stop(void)
{
    ppsi262_rst_high();
    am_util_delay_us(50);
    ppsi262_rst_low();
    am_util_delay_us(500);
    ppsi262_rst_high();
}

/**
 * @brief      Set TIA gain
 *
 * @param[in]  irGain   TIA gain (Ir channel)
 * @param[in]  redGain  TIA gain (Red channel)
 * @param[in]  grnGain  TIA gain (Grn channel)
 *
 * @return     void
 */
void ppsi262_setGain(uint8_t irGain, uint8_t redGain, uint8_t grnGain)
{
#if 0
        TIA_GAIN_LSB    D2 D1 D0
        0 500kΩ
        1 250kΩ
        2 100kΩ
        3 50kΩ
        4 25kΩ
        5 10kΩ
        6 1MΩ
        7 2MΩ
        8 1.5MΩ
#endif    
    ppsi262_writeReg(PPSI262_REG_TIAGAIN1, (0x008025-irGain));
    ppsi262_writeReg(PPSI262_REG_TIAGAIN2, (0x000025-grnGain));
}


/**
 * @brief      Set LED current
 *
 * @param[in]  irVal   LED current (Ir channel)
 * @param[in]  redVal  LED current (Red channel)
 * @param[in]  grnVal  LED current (Grn channel)
 *
 * @return     void
 */
void ppsi262_setValue(uint32_t irVal, uint32_t redVal, uint32_t grnVal)
{
    uint32_t irSet, redSet, grnSet;
    uint32_t regSetValue;

    if (irVal  > PPSI262_ILED2X_MAX) irVal  = PPSI262_ILED2X_MAX;
    if (redVal > PPSI262_ILED2X_MAX) redVal = PPSI262_ILED2X_MAX;
    if (grnVal > PPSI262_ILED2X_MAX) grnVal = PPSI262_ILED2X_MAX;

    irSet  = ((irVal*63*2)/PPSI262_ILED2X_MAX) + 1;
    redSet = ((redVal*63*2)/PPSI262_ILED2X_MAX) + 1;
    grnSet = ((grnVal*63*2)/PPSI262_ILED2X_MAX) + 1;

    irSet >>= 1; redSet >>= 1; grnSet >>= 1;
    regSetValue = grnSet | (irSet<<6) | (redSet<<12);

    ppsi262_writeReg(PPSI262_REG_LEDCNTRL, regSetValue);
#if 0
    uint32_t reg_50, reg_4b, reg_31, reg_3A;
    ppsi262_readReg(0x50,  &reg_50);
    ppsi262_readReg(0x4b,  &reg_4b);
    ppsi262_readReg(0x31,  &reg_31);
    ppsi262_readReg(0x3A,  &reg_3A);
    DBG_PRINTF("[get_reg_value]0x50:0x%x, 0x4b:0x%x, 0x31:0x%x, 0x3A: 0x%x\n\r", reg_50, reg_4b, reg_31, reg_3A);
#endif
    // DBG_PRINTF("[set_current]ir:%d, red:%d, grn:%d, reg: 0x%x\n\r", irVal, redVal, grnVal, regSetValue);
    
}


/**
 * @brief      Get PPSI262 PD value
 *
 * @param[out] grn  Green PD value
 * @param[out] red  Red PD value
 * @param[out] ir   IR PD value
 * @param[out] amb  AMB PD value
 *
 * @return     void
 */
void ppsi262_getPdValue(int32_t *grn, int32_t *red, int32_t *ir, int32_t *amb)
{
    *grn = pps_ctx_v.grnPdVal;
    *red = 0;
    *ir  = pps_ctx_v.irPdVal;
    *amb = pps_ctx_v.ambPdVal;
}



/**
 * @brief      PPSI262 capture interrupt service routine
 *
 * @param[in]  void
 *
 * @return     void
 */
void ppsi262_capture_isr(void)
{
    pps_ctx_v.intteruptOccurred = true;
}

/**
 * @brief      PPSI262 get value
 *
 * @param[in]  void
 *
 * @return     void
 */
void ppg_getPdValue(int32_t* grnPdVal, int32_t* irPdVal, int32_t* ambPdVal)
{
     ppsi262_readReg(PPSI262_REG_LED2VAL,  (uint32_t *)irPdVal);
//     ppsi262_readReg(PPSI262_REG_ALED2VAL, (uint32_t *)redPdVal);
     ppsi262_readReg(PPSI262_REG_LED1VAL,  (uint32_t *)grnPdVal);
     ppsi262_readReg(PPSI262_REG_ALED1VAL, (uint32_t *)ambPdVal);

    /* Sign expend to 32-bit */
    if (*irPdVal  & 0x800000) *irPdVal  -= 0x1000000;
//    if (*redPdVal & 0x800000) *redData -= 0x1000000;
    if (*grnPdVal & 0x800000) *grnPdVal -= 0x1000000;
    if (*ambPdVal & 0x800000) *ambPdVal -= 0x1000000;
    // LOG_DEBUG("[ppg_getPdValue]: %d %d %d\n", *irPdVal, *grnPdVal, *ambPdVal);
}

/**
 * @brief      PPSI262 GPIO interrupt service routine
 *
 * @param[in]  void
 *
 * @return     void
 */
void ppsi262_gpio_isr(void)
{
    static int32_t ir_filt = 0;
    uint32_t  skin_deteced = 0;
    ppg_getPdValue(&pps_ctx_v.grnPdVal, &pps_ctx_v.irPdVal, &pps_ctx_v.ambPdVal);
    
    ir_filt = (ir_filt >> 1) + (pps_ctx_v.irPdVal >> 1);
    if (ir_filt > 1e6)
        skin_deteced = 1;
    else
        skin_deteced = 0;
        
    if (hrm_st_is_prepare()){
        return;
    }

    //push data to cbuff
    #define LEDbuffHead cbuff_hr.head
    #define LEDbuffTail cbuff_hr.tail
    #define rd_ptr      cbuff_hr.rd_ptr
    uint16_t cbuff_head = (LEDbuffHead + 1);           // next position in circular buffer
    
    if (cbuff_head >= (AFE_BUFF_LEN << 1)) {
        cbuff_head = 0;
    }
    if (((cbuff_head - LEDbuffTail) & ((AFE_BUFF_LEN << 1) - 1)) >= AFE_BUFF_LEN){
        LEDbuffTail = (cbuff_head - AFE_BUFF_LEN) & ((AFE_BUFF_LEN << 1) - 1);		//max buffer size is AFE_BUFF_LEN
    }
    if (((cbuff_head - rd_ptr) & ((AFE_BUFF_LEN << 1) - 1)) >= AFE_BUFF_LEN){
        rd_ptr = (cbuff_head - AFE_BUFF_LEN) & ((AFE_BUFF_LEN << 1) - 1);		    //max buffer size is AFE_BUFF_LEN
    }

    uint16_t tmphead = (cbuff_head >= AFE_BUFF_LEN) ? (cbuff_head - AFE_BUFF_LEN) : cbuff_head;
    hr_led_green[tmphead] = pps_ctx_v.grnPdVal;         //green
    hr_led_amb[tmphead]   = pps_ctx_v.ambPdVal;         //amb
    hr_led_ir[tmphead]    = pps_ctx_v.irPdVal;          //IR
    hr_led_red[tmphead]   = 0;                //red

    LEDbuffHead = cbuff_head;

    // LOG_DEBUG("[ppsi262_gpio_isr]head:%d ptr:%d %d %d %d\n", cbuff_head, rd_ptr, pps_ctx_v.irPdVal, pps_ctx_v.grnPdVal, pps_ctx_v.ambPdVal);
    
}


/**
 * @brief      PPSI dim adjust
 *
 * @param[in]  None
 *
 * @return     is_dim_finished - true: finished, true: false
 */
bool ppg_dim(void)
{
    /* Start the dimming, switch to dimming mode */
    static ppg_dim_t ppgDimStatus = PPG_DIM_IR_INIT;
    static bool      status;
    static uint32_t  current;
    static uint8_t   gain;
    
    static uint8_t   irGain, redGain, grnGain;
    static uint32_t  irVal,  redVal,  grnVal;

    dim_params_t dimParams_c;
    
    if (pps_ctx_v.dim_finished == false)
    {
        // ppg_getPdValue(&pps_ctx_v.grnPdVal, &irPdVal, &ambPdVal);
        //DBG_PRINTF("[ppg_dim]status: %d, pps_ctx_v.dim_finished: %d, pps_ctx_v.weared: %d\n\r", status, pps_ctx_v.dim_finished, pps_ctx_v.weared);
        
        switch(ppgDimStatus)
        {
            case PPG_DIM_IR_INIT:
                /* Initialize the gain and value */
                //DBG_PRINTF("[PPG_DIM_IR_INIT]status: %d\n\r", status);                
                irGain  = hrm_cfg.irGain.ini;
                redGain = hrm_cfg.redGain.ini;
                grnGain = hrm_cfg.grnGain.ini;

                irVal   = hrm_cfg.irPeak.ini;
                redVal  = hrm_cfg.redPeak.ini;
                grnVal  = hrm_cfg.grnPeak.ini;
                /* Set the Ir gain and value */
                // ppsi262_setGain(irGain, redGain, grnGain);
                ppsi262_setValue(irVal, redVal, grnVal);

                /* Set the diming parameters, start gain diming */
                dimParams_c.target     = hrm_cfg.irTarget;
                dimParams_c.minGain    = hrm_cfg.irGain.min;
                dimParams_c.maxGain    = hrm_cfg.irGain.max;
                dimParams_c.factor     = hrm_cfg.currentMultiple;
                dimParams_c.minCurrent = hrm_cfg.irPeak.min;
                dimParams_c.maxCurrent = hrm_cfg.irPeak.max;
                algorithm_setDimParams(&dimParams_c);
                gain = hrm_cfg.irGain.ini;
                ppgDimStatus = PPG_DIM_IR_GAIN;
                break;

            case PPG_DIM_IR_GAIN:
                /* Do gain adjustment basing on the value from PD */
                //DBG_PRINTF("[ir_gain]Peak is %d, pd is %d, irGain %d\n\r", hrm_cfg.irPeak.ini, (int)pps_ctx_v.irPdVal, gain);
                
                status = algorithm_adjustGain(&gain, pps_ctx_v.irPdVal);
                //DBG_PRINTF("[ir_gain_new]Peak is %d, pd is %d, irGain %d\n\r", hrm_cfg.irPeak.ini, (int)pps_ctx_v.irPdVal, gain);

                /* Re-set the new gain */
                ppsi262_setGain(gain, redGain, grnGain);

                if (true == status)
                {
                    /* Gain adjustment finished */
                    irGain = gain;
                    current = hrm_cfg.irPeak.ini;
                    ppgDimStatus = PPG_DIM_IR_CURRENT;
                    //DBG_PRINTF("[ir_gain_done]Peak is %d, pd is %d, irGain %d\n\r", hrm_cfg.irPeak.ini, (int)pps_ctx_v.irPdVal, gain);
                    
                }
                break;

            case PPG_DIM_IR_CURRENT:
                //DBG_PRINTF("[ir_current]Peak is %d, pd is %d, irGain %d\n\r", (int)current, (int)pps_ctx_v.irPdVal, gain);
                status = algorithm_adjustCurrent(&current, pps_ctx_v.irPdVal);
                //DBG_PRINTF("[ir_current_new]Peak is %d, pd is %d, irGain %d\n\r", (int)current, (int)pps_ctx_v.irPdVal, gain);
                ppsi262_setValue(current, redVal, grnVal);

                if (true == status)
                {
                    /* Get the best current, finished current dimming */
                    irVal = current;
                    ppgDimStatus = PPG_DIM_GRN_INIT;
                    //DBG_PRINTF("[ir_current_done]Peak is %d, pd is %d, irGain %d\n\r", (int)current, (int)pps_ctx_v.irPdVal, gain);
                    
                }
                break;

            case PPG_DIM_GRN_INIT:
                dimParams_c.target     = hrm_cfg.grnTarget;
                dimParams_c.minGain    = hrm_cfg.grnGain.min;
                dimParams_c.maxGain    = hrm_cfg.grnGain.max;
                dimParams_c.factor     = hrm_cfg.currentMultiple;
                dimParams_c.minCurrent = hrm_cfg.grnPeak.min;
                dimParams_c.maxCurrent = hrm_cfg.grnPeak.max;
                algorithm_setDimParams(&dimParams_c);
                gain = hrm_cfg.grnGain.ini;
                ppgDimStatus = PPG_DIM_GRN_GAIN;
                break;

            case PPG_DIM_GRN_GAIN:
                /* Do gain adjustment basing on the value from PD */
                //DBG_PRINTF("[grn_gain]Peak is %d, pd is %d, grnGain %d\n\r", hrm_cfg.grnPeak.ini, (int)pps_ctx_v.grnPdVal, gain);
                status = algorithm_adjustGain(&gain, pps_ctx_v.grnPdVal);
                //DBG_PRINTF("[grn_gain_new]Peak is %d, pd is %d, grnGain %d\n\r", hrm_cfg.grnPeak.ini, (int)pps_ctx_v.grnPdVal, gain);

                /* Re-set the new gain */
                ppsi262_setGain(irGain, redGain, gain);

                if (true == status)
                {
                    /* Gain adjustment finished */
                    grnGain = gain;
                    current = hrm_cfg.grnPeak.ini;
                    ppgDimStatus = PPG_DIM_GRN_CURRENT;
                    //DBG_PRINTF("[grn_gain_done]Peak is %d, pd is %d, grnGain %d\n\r", hrm_cfg.grnPeak.ini, (int)pps_ctx_v.grnPdVal, gain);
                    
                }
                break;

            case PPG_DIM_GRN_CURRENT:
                //DBG_PRINTF("[grn_current]Peak is %d, pd is %d, grnGain %d\n\r", (int)current, (int)pps_ctx_v.grnPdVal, gain);
                           
                status = algorithm_adjustCurrent(&current, pps_ctx_v.grnPdVal);
                //DBG_PRINTF("[grn_current_new]Peak is %d, pd is %d, grnGain %d\n\r", (int)current, (int)pps_ctx_v.grnPdVal, gain);

                if (current == 0){
                    current = hrm_cfg.grnPeak.ini;
                }
                ppsi262_setValue(irVal, redVal, current);

                if (true == status)
                {
                    /* Get the best current, finished current dimming */
                    grnVal = current;
                    pps_ctx_v.dim_finished = true;
                }

                //Dim is finished, reset all circle buffer data
                if (pps_ctx_v.dim_finished){
                    ppgDimStatus = PPG_DIM_IR_INIT;
                    // ry_memset(&cbuff_hr, 0, sizeof(cbuff_hr));
                    pps_ctx_v.stage = HRM_HW_ST_MEASURE;
                }
                break;


            default:
                pps_ctx_v.dim_finished = true;
                break;
        }
    }
    /* Finished the dimming, switch to running mode */
    // hrs_set_mode(HRS_RUN);
    
    return pps_ctx_v.dim_finished;
}

/**
 * @brief   weard processing
 *
 * @param   None
 *
 * @return  wrist detect result - 0: not detect, 1: detected, 0xff: detect not finished
 */
uint8_t ppg_weard_detect(void)
{
    static int32_t* ir_buffer = NULL;
    if (pps_ctx_v.weared_period == WEARD_DETECT_PERIOD_INIT){
        pps_ctx_v.weared = WEAR_DET_INIT;
        if (ir_buffer == NULL){
            ir_buffer = (int32_t*)ry_malloc(WEARD_DETECT_PERIOD_DONE*sizeof(int32_t));  
        }
        if (!ir_buffer) {
            LOG_WARN("[ppg_weard_detect] No mem.\r\n");
            while(1);
        }
    }

    if (pps_ctx_v.weared_period <= WEARD_DETECT_PERIOD_DONE){
        if (ir_buffer != NULL){
            *(ir_buffer + pps_ctx_v.weared_period - 1) = pps_ctx_v.irPdVal;
        }
        // LOG_DEBUG("period:%2d, pps_ctx_v.irPdval:%7d, buffer:%7d\n", pps_ctx_v.weared_period, pps_ctx_v.irPdVal, *(ir_buffer + pps_ctx_v.weared_period - 1));    
        
        if (pps_ctx_v.weared_period == WEARD_DETECT_PERIOD_DONE){
            // for(int i = WEARD_DETECT_PERIOD_INIT; i <= WEARD_DETECT_PERIOD_DONE; i++)
            //     LOG_DEBUG_PPS("period:%2d,  pps_ctx_v.irPdval:%7d\r\n", i, *(ir_buffer + i - 1));    
            pps_ctx_v.weared = algo_isWeared(0, ir_buffer);
              
            if (ir_buffer != NULL){
                ry_free((void*)ir_buffer);
                ir_buffer = NULL;
            }
            LOG_DEBUG("period:%2d, pps_ctx_v.irPdval:%7d, pps_ctx_v.weared:%d\n", pps_ctx_v.weared_period, pps_ctx_v.irPdVal, pps_ctx_v.weared);    
            pps_ctx_v.stage = HRM_HW_ST_AUTO_DIM;
        }
        
        pps_ctx_v.weared_period ++;
        ppsi262_setValue(PPSI262_ILED2X_10MA * pps_ctx_v.weared_period, 0, 0);             
    }
    return pps_ctx_v.weared;
}

/**
 * @brief   get_ppg_stage
 *
 * @param   None
 *
 * @return  get_ppg_stage - refer to hrm_hw_st_t
 */
uint8_t get_ppg_stage(void)
{
    return pps_ctx_v.stage;
}


/**
 * @brief   hrm_weared_dim_reset
 *
 * @param   None
 *
 * @return  None
 */
void hrm_weared_dim_reset(void)
{
    pps_ctx_v.weared_period = WEARD_DETECT_PERIOD_INIT;
    pps_ctx_v.dim_finished = false;
    pps_ctx_v.stage = HRM_HW_ST_WEARE_DETECT;
}

/* [] END OF FILE */

