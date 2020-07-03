/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __PPSI26X_H__
#define __PPSI26X_H__

/*
 * TYPEDEFS
 *******************************************************************************
 */
/**
 * @brief      PPG LED dim
 *
 * @param[in]  void
 *
 * @return     void
 */
/*
 * Heart rate sensor gain definition
 */
typedef struct {
    uint8_t ini;
    uint8_t min;
    uint8_t max;
    uint8_t rsv;
}hrs_gain_t;

/*
 * Heart rate sensor peak current definition
 */
typedef struct {
    uint32_t ini;
    uint32_t min;
    uint32_t max;
}hrs_peak_t;

typedef struct {
    uint8_t hrsType;
    uint8_t rsv;                   /* Reserved for alignment    */
    uint8_t ppgType;               /* PPG sensor type           */
    uint8_t imuType;               /* IMU sensor type           */
    int32_t grnTarget;             /* Green dim target          */
    int32_t redTarget;             /* Red dim target            */
    int32_t irTarget;              /* Ir dim target             */
    uint32_t currentMultiple;      /* Current multiple          */
    hrs_gain_t grnGain;            /* Green gain                */
    hrs_peak_t grnPeak;            /* Green peak current        */
    hrs_gain_t redGain;            /* Red gain                  */
    hrs_peak_t redPeak;            /* Red peak current          */
    hrs_gain_t irGain;             /* Ir gain                   */
    hrs_peak_t irPeak;             /* Ir peak current           */
}hrs_cfg_t;

typedef enum {
    PPG_DIM_IR_INIT = 0,
    PPG_DIM_IR_GAIN,
    PPG_DIM_IR_CURRENT,
    PPG_DIM_GRN_INIT,
    PPG_DIM_GRN_GAIN,
    PPG_DIM_GRN_CURRENT,
}ppg_dim_t;


typedef enum {
    HRM_HW_ST_INIT        = 0,
    HRM_HW_ST_WEARE_DETECT= 1,    
    HRM_HW_ST_AUTO_DIM    = 2,        
    HRM_HW_ST_MEASURE     = 3,
    HRM_HW_ST_OFF         = 0xff,
}hrm_hw_st_t;


typedef struct {
    int32_t    grnPdVal;
    int32_t    irPdVal;
    int32_t    ambPdVal;

    uint8_t    weared_period;
    uint8_t    dim_finished;
    uint8_t    weared;
    uint8_t    stage;
    bool       intteruptOccurred;
} pps_ctx_t;

/*********************************************************************
 * CONSTANTS
 */
#define USE_FIFO                   1
#define USE_FIFO_DEPTH             5

#define LOG_DEBUG_PPS              //ry_board_debug_printf

#define AFE_BUFF_LEN               64          //circle buffer_size

#ifdef AFE_DEF
    #define GLOBAL
#else
    #define GLOBAL extern
#endif

/*********************************************************************
 * VARIABLES
 */
GLOBAL int32_t hr_led_green[AFE_BUFF_LEN];
GLOBAL int32_t hr_led_ir[AFE_BUFF_LEN];
GLOBAL int32_t hr_led_amb[AFE_BUFF_LEN];
GLOBAL int32_t hr_led_red[AFE_BUFF_LEN];
GLOBAL pps_ctx_t pps_ctx_v;

/*
 * FUNCTIONS
 *******************************************************************************
 */
void PPS_DELAY_MS(uint32_t ms);
void PPS_DELAY_US(uint32_t us);

void PPSI26x_writeReg(uint8_t regaddr, uint32_t wdata);
uint32_t PPSI26x_readReg(uint8_t regaddr);
uint32_t PPSI26x_readRegFIFO(uint8_t regaddr,uint8_t *read_fifo, uint32_t fifoLength);

void hrm_resetz_highLow(uint8_t fHigh);
void ppsi26x_Rest_HW(void);
void ppsi26x_Disable_HW(void);
void ppsi26x_Enable_HW(void);
void ppsi26x_sensor_task(void *params);

/**
 * @brief      PPSI dim adjust
 *
 * @param[in]  None
 *
 * @return     isDimFinished - true: finished, true: false
 */
bool ppg_dim(void);

/**
 * @brief   hrm sensor init
 *          Note: wakeup do this
 *
 * @param   None
 *
 * @return  mode: initial mode
 *                0: auto, else: manual
 */
void init_pps26x_sensor(uint32_t mode);

/**
 * @brief   weard processing
 *
 * @param   None
 *
 * @return  wrist detect result - 0: not detect, 1: detected, 0xff: detect not finished
 */
uint8_t ppg_weard_detect(void);

/**
 * @brief   get_hrm_hw_status
 *
 * @param   None
 *
 * @return  get_hrm_hw_status - 0: off, 1: measure, 0xff: initial status
 */
uint8_t get_hrm_hw_status(void);

/**
 * @brief   hrm_weared_dim_reset
 *
 * @param   None
 *
 * @return  None
 */
void hrm_weared_dim_reset(void);

/**
 * @brief   get_ppg_stage
 *
 * @param   None
 *
 * @return  get_ppg_stage - refer to hrm_hw_st_t
 */
uint8_t get_ppg_stage(void);

#endif //__PPSI26X_H__
