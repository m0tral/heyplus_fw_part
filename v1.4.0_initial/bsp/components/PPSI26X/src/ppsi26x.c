	/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ry_hal_inc.h"

/* Platform Dependency */
#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util_delay.h"
#include "ry_utils.h"

#include "hqerror.h" 
#include "sensor_alg.h"
#include "hrm.h"

#define HRM_SAMPLE_FILZO_DRV       1
#define HRM_SAMPLE_WITHOUT_ALG     1
#define AFE_DEF

// algorithm sources
#if HRM_SAMPLE_FILZO_DRV
    #include "ppsi26x.h"   
#elif HRM_SAMPLE_WITHOUT_ALG
    #include "ppsi26x.h"   
    #include "agc_V3_1_19_C.h"
    #include "AFE4410_API_C.h"
    #include "agc_V3_1_19_C.c"
    #include "AFE4410_API_C.c"
#else
    #define LIFEQ_DRIVER           1
    #define LIFEQ                  1

    #include "ppsi26x.h"
    #include "afe4410_sensor.h"
    #include "sched_alg_afe4410.h"
    #include "phys_calc.h"
    #include "pp_config.h"
    
    #include "ppg_afe4410_drv_coms.c"
    #include "sched_alg_afe4410.c"
    #include "ppg_afe4410_handler.c"
#endif

uint16_t acc_check = 0;
uint16_t acc_check2 = 0; 
int8_t hr_okflag = false;
int8_t Stablecnt = 0;
int8_t Unstablecnt = 0;

uint32_t displayHrm = 0;
uint32_t pps_count;
uint32_t pps_intr_flag = 0;
int8_t accPushToQueueFlag = 0;
uint8_t hr_ecg_mode = 0;        //0: hr mode;1: ecg mode

void PPS_DELAY_MS(uint32_t ms)
{
	am_util_delay_ms(ms);
}

void PPS_DELAY_US(uint32_t us)
{
	am_util_delay_us(us);
}

void hrm_resetz_highLow(uint8_t fHigh)
{
    am_hal_gpio_pin_config(HRM_RESETZ_CTRL_PIN, AM_HAL_GPIO_OUTPUT);
    if (fHigh == 0)
        am_hal_gpio_out_bit_clear(HRM_RESETZ_CTRL_PIN);
    else
        am_hal_gpio_out_bit_set(HRM_RESETZ_CTRL_PIN);
    am_util_delay_us(100);
}

void ppsi26x_init_gpio(void)
{
    am_hal_gpio_pin_config(PPS_INTERRUPT_PIN, AM_HAL_GPIO_INPUT);
    hrm_resetz_highLow(1);
}

void ppsi26x_Rest_HW(void)
{
    //RESTZ Pin
    am_hal_gpio_out_bit_clear(HRM_RESETZ_CTRL_PIN);     //low
    PPS_DELAY_MS(2);
    am_hal_gpio_out_bit_set(HRM_RESETZ_CTRL_PIN);       //high
    PPS_DELAY_MS(2);
}

void ppsi26x_Rest_SW(void)
{
    //software rest
    //PPSI26x_writeReg(0,0x8 | 0x20);
    PPS_DELAY_MS(50);
}

void pps_interrupt_uninit(void)
{
    am_hal_gpio_pin_config(PPS_INTERRUPT_PIN, AM_HAL_PIN_DISABLE);
    am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(PPS_INTERRUPT_PIN));
}

//enter low power do this
void ppsi26x_Disable_HW(void)
{
    hrm_resetz_highLow(0);
    pps_interrupt_uninit();
    pps_ctx_v.stage = HRM_HW_ST_OFF;
}

void ppsi26x_Enable_HW(void)
{
    hrm_resetz_highLow(1);
    PPS_DELAY_MS(20);
}

void PPSI26x_writeReg(uint8_t regaddr, uint32_t wdata)
{	
    am_hal_iom_status_e ret = AM_HAL_IOM_SUCCESS;
    am_hal_iom_buffer(16) g_psTxBuffer;
    g_psTxBuffer.bytes[0] = (wdata >> 16) & 0xFF;
    g_psTxBuffer.bytes[1] = (wdata >> 8)  & 0xFF;
    g_psTxBuffer.bytes[2] = wdata & 0xFF;

    ret = (am_hal_iom_status_e)ry_hal_i2cm_tx_at_addr(I2C_IDX_HR, regaddr, g_psTxBuffer.bytes, 3);

    if (ret != AM_HAL_IOM_SUCCESS){
        LOG_DEBUG("PPSI26x_writeReg faile!!! %d\r\n",ret);
    }
}

uint32_t PPSI26x_readReg(uint8_t regaddr)
{
    am_hal_iom_status_e ret;
    uint32_t rdtemp;
    uint32_t temp;
    am_hal_iom_buffer(16) g_psRxBuffer;
    do {
        ret =(am_hal_iom_status_e) ry_hal_i2cm_rx_at_addr(I2C_IDX_HR, regaddr, g_psRxBuffer.bytes, 3);
    } while (0);
    
    if(ret != AM_HAL_IOM_SUCCESS){
        LOG_DEBUG("PPSI26x_readReg faile!!! %d\r\n",ret);
    }
    
    temp = g_psRxBuffer.bytes[0];
    rdtemp = (temp << 16) & 0xff0000;
    
    temp = g_psRxBuffer.bytes[1];
    rdtemp = rdtemp | ((temp << 8) & 0x00ff00);
    
    temp = g_psRxBuffer.bytes[0];
    rdtemp = rdtemp | (temp & 0x0000ff);
		
    //PPS_Debug("rdtemp : %d\r\n ",rdtemp);
    return rdtemp;
}

uint32_t PPSI26x_readRegFIFO(uint8_t regaddr,uint8_t *read_fifo, uint32_t fifoLength)
{   	
    am_hal_iom_status_e ret;
    do {
		ret = (am_hal_iom_status_e)ry_hal_i2cm_rx_at_addr(I2C_IDX_HR, regaddr, read_fifo, fifoLength);
    } while (0);
	if(ret != AM_HAL_IOM_SUCCESS ){
		LOG_DEBUG("PPSI26x_readReg faile!!! %d\r\n",ret);
	}
    return AM_HAL_IOM_SUCCESS;
}

//*****************************************************************************
//
// GPIO ISR task callback
//
//*****************************************************************************
extern ryos_semaphore_t* hrm_sem;
void hrm_gpio_isr_task(void)
{
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(PPS_INTERRUPT_PIN));
    //ppsi26x_sensor_task(NULL);

    //ryos_semaphore_post(hrm_sem);
    ry_hrm_msg_send(HRM_MSG_CMD_GPIO_ISR, NULL);
}

void pps_interrupt_init(void)
{
    am_hal_gpio_pin_config(PPS_INTERRUPT_PIN, AM_HAL_GPIO_INPUT);
    am_hal_gpio_int_polarity_bit_set(PPS_INTERRUPT_PIN, AM_HAL_GPIO_RISING);
    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(PPS_INTERRUPT_PIN));
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(PPS_INTERRUPT_PIN));
    // am_hal_gpio_int_register(PPS_INTERRUPT_PIN, hrm_gpio_isr_task);
    am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);
}

void _init_pps26x_sensor(void)
{
    hqret_t rtlifeQ_2;
#ifdef LIFEQ_DRIVER
		if (hr_ecg_mode == 1){
			AFE4410_Internal_250Hz_ecg();
		}
        else{
            if (AfeInitialise() != RET_OK){
                LOG_DEBUG_PPS("[hrm]: AfeInitialise Failed\n"); 
            }
        }
#endif
#ifdef LIFEQ
    //Initialize the LifeQ Algorithm
    PP_Set_Defaults();
    PP_Set_Gs(DEVICE_CONFIG_ACC_8G);
    PP_Set_Device_AFE(AFE_TI4405);
    PP_Set_SkinDetect(ON);
    PP_Init(DEVICE_DATA_INPUT_FS_25HZ);
    //PP_Set_ActivityMode(PP_CONFIG_RUNNING);
    PP_Reset();
#endif	

#ifdef LIFEQ_DRIVER
    rtlifeQ_2 = Sensors_startSampling();
    if(rtlifeQ_2 != RET_OK){
        LOG_DEBUG("[hrm Sensors]: start Sampling Failed\n"); 
    }
    else{
        LOG_DEBUG("[hrm Sensors]: start Sampling succ\n");         
    }
#endif
    PPS_DELAY_MS(100);
}

#if (HRM_SAMPLE_FILZO_DRV)

#include "ppsi262_fz.c"

void init_pps26x_sensor(uint32_t mode)
{
    pps_ctx_v.stage = HRM_HW_ST_INIT;
    ppsi262_init();
    ppsi26x_init_gpio (); 
    ppsi262_start();
    pps_interrupt_init();  

    if (mode == HRM_START_FIXED){
        ppsi262_setGain(1, 1, 1);
        ppsi262_setValue(AFE4405_ILED1X_MAX, 0, AFE4405_ILED1X_MAX); 
        pps_ctx_v.weared_period = WEARD_DETECT_PERIOD_DONE + 1;
        pps_ctx_v.weared = WEAR_DET_SUCC;
        pps_ctx_v.dim_finished = true;
        pps_ctx_v.stage = HRM_HW_ST_MEASURE;
    }
    else{
        ppsi262_setGain(0, 0, 0);
        ppsi262_setValue(PPSI262_ILED2X_10MA, 0, 0); 
        hrm_weared_dim_reset();
    }
}

void ppsi26x_sensor_task(void *params)
{
	ppsi262_gpio_isr();
}

#else   //=============================================================================

/**
 * @brief   hrm sensor init
 *          Note: wakeup do this
 *
 * @param   None
 *
 * @return  None
 */
void init_pps26x_sensor(uint32_t mode)
{
    ppsi26x_init_gpio();
    ppsi26x_Enable_HW();
    ppsi26x_Rest_HW();
#if HRM_SAMPLE_WITHOUT_ALG
	AFE4410_Internal_25Hz();
    PPSI26x_init();
#else
    _init_pps26x_sensor();    
#endif  
	pps_interrupt_init();  
	acc_check = 1;    
}


void ppsi26x_sensor_task(void *params)
{
    pps_intr_flag = 0;
#if HRM_SAMPLE_WITHOUT_ALG
    if(acc_check) {
        ALGSH_retrieveSamplesAndPushToQueue();//read pps raw data
        //move ALGSH_dataToAlg(); to message queue loop. and then send message at here.
        ALGSH_dataToAlg();
    }
#else
	if (hr_ecg_mode == 1){
        void ppsi263_sensor_task_ecg(void *params);
		ppsi263_sensor_task_ecg(NULL);
	}
	else{
#if USE_FIFO
		if(AFE_fifoInterruptReceived() == RET_OK){
			hqret_t fifointerrupt = AFE_fifoInterruptService();	
		}
#else
		if(AFE_interruptReceived() == RET_OK){ 
			hqret_t fifointerrupt = AFE_interruptService();	
		}
#endif
		if( AFE_getBufferDataReadyStatus()){
			//SAFE_CALL(ALGSH_AFE4410_dataToAlg());    // Sends data to algorithm
			ALGSH_AFE4410_dataToAlg();                 // Sends data to algorithm
			//SENSOR_ACC_dataCompleat();               // Tells accelerometer driver that circular buffer has been serviced
			AFE_FIFODataServiced();                    // Tells the AFE driver that circular buffer has been serviced
										               // This is to ensure that the data has actually been sent to the algorithm
										               // before the buffer tail is incremented
		}
    }
#endif    
}

#define ECG_FIFO_LENGTH 60                      // change registor
int32_t ecg_ecgData[ECG_FIFO_LENGTH];
int32_t ecg_ppgData[ECG_FIFO_LENGTH];
	
void ppsi263_sensor_task_ecg(void *params)      //ECG task
{
    uint8_t	data[ECG_FIFO_LENGTH *6 ]={0,};
	uint32_t i,j;
	uint8_t ecg_dc_lead_off=0;

    PPSI26x_writeReg(0x00, 0x60);
    PPSI26x_writeReg(0x51, 0x100);
    PPSI26x_writeReg(0x51, 0x00);
    PPSI26x_writeReg(0x00, 0x61);

	//read fifo
	for(i=0;i<6;i++){
		PPSI26x_readRegFIFO(0xff,&data[0+ECG_FIFO_LENGTH*i],ECG_FIFO_LENGTH);
	}
	
	for(i=0,j=0;j<ECG_FIFO_LENGTH;)
	{
		ecg_ppgData[j] = (int32_t)((data[i+0] <<16 | data[i+1] <<8 | data[i+2])<<8)/256;
		ecg_ecgData[j] = (int32_t)((data[i+3] <<16 | data[i+4] <<8 | data[i+5])<<8)/256;
		i=i+6;
		j++;
	}
	
	ecg_dc_lead_off = PPSI26x_readReg(0x63); //return value, 9 is not contacted  lead-off , 0 is lead-off detect.
	//NRF_LOG_RAW_INFO("lead off check:%d\n",ecg_dc_lead_off); 
	//ecg_ppgData[i],ecg_ecgData[i], transmit to algorithm.
	for(i=0;i<ECG_FIFO_LENGTH;i++)
	{
		LOG_DEBUG_PPS("PPG ECG: %d %d\r\n",ecg_ppgData[i],ecg_ecgData[i]); 
	}
}


void ppsi26x_sensor_task2(void *params)
{
    static uint8_t cnt = 0;
    static uint8_t snr = 0;
    static int lifeQhrm = 0;
	uint8_t skin = 0;
    
#if HRM_SAMPLE_WITHOUT_ALG
    uint8_t snrValue, sample;
    if (acc_check) {
        sample = GetHRSampleCount();
        ClrHRSampleCount();
        if(cnt++ > 254) 
            cnt = 0;
        lifeQhrm = pps_getHR();
        skin = PPS_get_skin_detect();
        displayHrm = lifeQhrm;
        snr = snrValue;
    }
#else
	if (acc_check && hr_ecg_mode == 0) {
        if (cnt++ > 255)
            cnt = 0;
#ifdef LIFEQ	
        lifeQhrm = LQ_Get_HR();
        snr = PP_GetHRConfidence();             //for snr check
#endif	
        //skin = PPS_get_skin_detect();
        skin = PP_IsSensorContactDetected();    //for skin detect
        displayHrm = lifeQhrm;
    }
#endif   
    LOG_DEBUG("cnt: %d HR=%d snr=%d skin=%d\r\n", cnt, lifeQhrm, snr, skin);
}
#endif
