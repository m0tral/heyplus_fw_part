/************************************************************************//*!
 *  @file    algorithm.h
 *  @brief   Header files of algorithm.c
 *
 *  @author  huang_haijun@163.com
 *
 *  @par     History:
 *            - 09/21/2017  0.10 -> First version
 ***************************************************************************/
#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

/**
 * Algorithm mode definition
 */
typedef enum {
    ALGO_IMU = 0,   // Inertial Measurement Unit
    ALGO_HRS,       // Altitude Heading Reference System
}algo_mode_t;

/*
 * Body information for algorithm
 */
typedef struct {
    uint8_t age;
    uint8_t sex;
    uint8_t weight;
    uint8_t height;
}algo_body_t;


/*
 * Algorithm sports mode
 */
typedef enum {
    SPORTS_STILL = 0,
    SPORTS_RUN,
    SPORTS_WALK,
    SPORTS_LIGHT_ACTIVE,    
    SPORTS_SLEEP_LIGHT,
    SPORTS_SLEEP_DEEP,
    SPORTS_NONE = 9,
    SPORT_RUN_MODE = 11,
    SPORT_IN_DOOR_RUN_MODE = 12,
    SPORT_RIDE_MODE = 13,
    SPORT_IN_DOOR_RIDE_MODE = 14,
    SPORT_ALG_SWIMMING = 15,
    SPORT_OUT_DOOR_WALK = 16,
    SPORT_FREE = 17,
    
    SPORTS_NOT_DETECT = 0xff,
}sports_mode_t;

/*
 * Algorithm WEARED mode
 */
typedef enum {
    ALGO_WEARED_IR_ONLY = 0,
    ALGO_WEARED_HRM_ON,    
}algo_weared_mode_t;

/*
 * Algorithm dimming parameter structure
 */
typedef struct {
    int32_t  target;
    uint8_t  minGain;
    uint8_t  maxGain;
    uint8_t  rsv[2];
    uint32_t factor;
    uint32_t minCurrent;
    uint32_t maxCurrent;
}dim_params_t;

/**
 * LED definition
 */
typedef enum {
    LED_GRN = 0,
    LED_RED,
    LED_IR,
    LED_AMB,
    LED_TOTAL,
}led_num_t;


/**
 * Sleep state
 */
typedef enum {
    SLP_WAKEUP = 0,
    SLP_SUSPECT,
    SLP_SLEEP,
    SLP_SLEEP_DEEP,    
    SLP_SLEEP_STAGE = 0x07,
    SLP_WAKEUP_TO_SUSPECT,    
}slp_state_t;


/*
 * CONSTANTS
 *******************************************************************************
 */
#define ALGO_BUF_SIZE                     128
#define AXIS_TOTAL                        6
#define WEARD_DETECT_PERIOD_DONE          10
#define WEARD_DETECT_PERIOD_INIT          1
#define WRIST_THRESH_INIT                 5
#define WRIST_THRESH_RUN                  2

/*
 * VARIABLES
 *******************************************************************************
 */
extern bool isAccWristBufFull;
extern bool isAccWristFirstBuf;
extern uint8_t accWristBufOffset;
extern uint16_t slpCntSum;
extern bool sleepDbgEnabled;

extern float hrAccBuf1[AXIS_TOTAL][ALGO_BUF_SIZE];
extern float hrLightBuf1[LED_TOTAL][ALGO_BUF_SIZE];


/*
 * FUNCTIONS
 *******************************************************************************
 */
extern void algorithm_setDimParams(dim_params_t *params);
extern bool algorithm_adjustGain(uint8_t *gain, int32_t pdVal);
extern bool algorithm_adjustCurrent(uint32_t *current, int32_t pdVal);


extern void algorithm_init(void);
extern void algorithm_setMode(algo_mode_t mode);
extern void algorithm_setBodyInfo(algo_body_t info);
// extern void algorithm_setWorkType(algo_type_t type);

extern void algorithm_lightAddToBuf(int32_t *ir,
                                    int32_t *red,
                                    int32_t *grn,
                                    int32_t *amb,
                                    uint16_t len);

extern void algorithm_accAddToBuf(int16_t *acc_x,
                                  int16_t *acc_y,
                                  int16_t *acc_z,
                                  int16_t *gyr_x,
                                  int16_t *gyr_y,
                                  int16_t *gyr_z,
                                  uint16_t len);

extern bool algorithm_isBufFull(void);
extern void algorithm_clearBufFullFlag(void);

extern void algorithm_getVersion(char *buf);
extern void algorithm_getBuild(char *buf);

extern void algorithm_preProcess(void);
extern void algorithm_hr_cal(uint8_t *hr);
extern void algorithm_sc_cal(uint8_t *pStepFreq, int8_t *pStepCnt);

void algo_accAddToWristBuf(int16_t *acc_x, int16_t *acc_y, int16_t *acc_z,
                           uint32_t length);

bool algo_isAccWristBufFull(void);
void algo_ws_init(void);
uint8_t algo_ws_cal(void);
void algo_sleep_enable(bool enable);

/*
 * @brief   运动模式检测，需要跟在步数计算之后，每秒调用一次
 * @para    None
 * @return  特定的运动模式：sports_mode_t
*/
uint8_t algo_state_cal(void);

/*
 * @brief   此函数使用三轴的缓存，所以需要和步频计算一起调用
 * @return  返回True表示静止。
*/
bool algo_isStill(void);

/*
 * @brief   佩戴检测
 * @param   state -- 0 在光管关闭时调用
 *          -- 1 光管打开时调用
 * @param   irBuf -- 仅在 state 为0 时，需要输入10个点的数据 （红外电流分别为 10mA， 20mA ~ 100mA时的PD 数值）
 *          在state 为 1 时， 用NULL即可
 * 
 * @return  返回True表示佩戴
*/
bool algo_isWeared(uint8_t state, int32_t *irBuf);

/*
 * @brief   测试是否进入睡眠，每秒计算一次，用于判断是否开光管
 * @param   delay second of sleep or wakeup
 * @return  ref: slp_state_t
 */
uint8_t algo_isSleep(uint16_t *second);

/**
 * @brief      Set sleep mode
 *
 * @param[in]  mode  Sleep mode, "0"-> night mode, "1" -> day mode
 *
 * @return     void
 */
void algo_setSlpMode(uint8_t mode);

/*
 * @brief   计算RRI值，标示有多少个RRI数据
 * @param   rri: 峰值idx位置
 * @param   sstd:
 * @return  有几个RRI: 0, 1, 2, 3
 *          0： 当前秒无RRI值输出
 */
uint8_t algo_sleep_calc(int8_t *rri, uint8_t *sstd);

/*
 * @brief   set sleep debug mode
 * @param   None
 * @return  None
 */
void algo_sleep_dbgEnable(void);


/*
 * @brief   更新最近两天的心率最小值
 * @param   hrMin1, hrMin2，最近天中的最小值
 * @return  None
 */
void algo_slp_updateHrMin(uint8_t hrMin1, uint8_t hrMin2);


/*
 * @brief   进入疑似睡眠时，计算出的心率值需调用
 * @param   hr:传入心率值
 * @return  None
 */
void algo_slp_addToHrBuf(uint8_t hr);

void algo_ws_setLevel(uint8_t level);
void algo_sleep_getParams(uint8_t *hrMean, uint8_t *hrMedian, uint8_t *hrRef, uint8_t *cacheLen);
void algo_ws_dbg(uint8_t *traceFlag, float *sstd);

void algo_ws_setDisplayState(uint8_t);

#endif /* __ALGORITHM_H__ */




