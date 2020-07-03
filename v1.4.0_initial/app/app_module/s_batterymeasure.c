/**
  ******************************************************************************
  * @file    batterymeasure.c
  * @author  MCD Application Team   
  * @brief   System information functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "k_config.h"
#include "k_menu.h"
#include "k_module.h"
#include "s_batterymeasure.h"

/* ## Definition of ADC related resources ################################### */
// #define DISPLAY_VOLT_IN_V               1

/* Definition of ADCx clock resources */
#define ADCx                            ADC1
#define ADCx_CLK_ENABLE()               __HAL_RCC_ADC_CLK_ENABLE()
#define ADCx_CLK_DISABLE()              __HAL_RCC_ADC_CLK_DISABLE()

#define ADCx_FORCE_RESET()              __HAL_RCC_ADC_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __HAL_RCC_ADC_RELEASE_RESET()

/* Definition of ADCx channels */
#define SAMPLINGTIME                    ADC_SAMPLETIME_640CYCLES_5

/* Private function prototypes -----------------------------------------------*/
/* Battery Measure Module functions */
KMODULE_RETURN BatteryMeasureExec(void);

/* Battery Measure items functions */
static void BatteryMeasure_Run(void);

/* Battery Measure private sub-functions */
static void ADC_Config(void);
static void Volt_Convert(uint32_t Value, uint16_t *DisplayString);

/* Private Variable ----------------------------------------------------------*/
const tMenuItem BatteryMeasureMenuItems[] =
{
    {"BatteryRun", 0, 0, TYPE_EXEC,   MODULE_NONE,      BatteryMeasure_Run, NULL, NULL, NULL},
    {"  MAIN "   , 0, 0, SEL_MODULE,  MODULE_MAIN_APP,  NULL,               NULL, NULL, NULL},
    {""          , 0, 0, SEL_EXIT,    0,                NULL,               NULL, NULL, NULL}

};

const tMenu BatteryMeasureMenu = {
  "battery_menu", BatteryMeasureMenuItems, countof(BatteryMeasureMenuItems), TYPE_TEXT, 2, 3
};

/* Variables for ADC conversions results computation to physical values */
uint16_t   uhADCChannelToDAC_mVolt = 0;

  /* display variable */
uint16_t datatodisplay[16] = {0};

/* Private variables ---------------------------------------------------------*/
/* ADC handler declaration */
uint32_t    AdcHandle;

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define UNIT_CHAR_NB                              2

#define COMPENSATE_VBAT_DIVIDER                   3         /* the converted digital value is on third of the VBAT voltage */ 

/* 3000 = 3300 - 300 (diode)*/
#define VDD_APPLI                      ((uint32_t) 3000)    /* Value of analog voltage (unit: mV) */
#define RANGE_12BITS                   ((uint32_t) 4095)    /* Max value with a full range of 12 bits */

#define VDD_REFINT                     ((uint32_t) 1200)    /* Value of VrefInt            (unit: mV) */
#define RANGE_12BITS                   ((uint32_t) 4095)    /* Max value with a full range of 12 bits */

/* Private macro -------------------------------------------------------------*/
/**
  * @brief  Computation of voltage (unit: mV) from ADC measurement digital
  *         value on range 12 bits.
  *         Calculation validity conditioned to settings: 
  *          - ADC resolution 12 bits (need to scale value if using a different 
  *            resolution).
  *          - Power supply of analog voltage Vdda 3.3V (need to scale value 
  *            if using a different analog voltage supply value).
  * @param ADC_DATA: Digital value measured by ADC
  * @retval None
  */
#define COMPUTATION_DIGITAL_12BITS_TO_VOLTAGE(ADC_DATA)                        \
  ( ((ADC_DATA) * VDD_APPLI) / RANGE_12BITS)

/* Private macro -------------------------------------------------------------*/
/**
  * @brief  Computation of voltage (unit: mV) from ADC measurement digital
  *         value on range 12 bits.
  *         Calculation validity conditioned to settings: 
  *          - ADC resolution 12 bits (need to scale value if using a different 
  *            resolution).
  *          - Power supply of analog voltage Vdda 3.3V (need to scale value 
  *            if using a different analog voltage supply value).
  * @param ADC_DATA: Digital value measured by ADC (VrefInt)
  * @retval None
  */
#define COMPUTATION_VDD_USING_VREFINT_12BITS_TO_VOLTAGE(ADC_DATA)                        \
  ( ((VDD_REFINT) * RANGE_12BITS) / ADC_DATA)
    
    
    
/* External variables --------------------------------------------------------*/
const K_ModuleItem_Typedef ModuleBatteryMeasure =
{
  MODULE_BATTERYMEASURE,
  NULL,
  BatteryMeasureExec,
  NULL,
  NULL
};  

/**
  * @brief  Run the Idd Measurement application 
  * @param  None.
  * @note   run and display Idd Menu.  
  * @retval None.
  */
KMODULE_RETURN BatteryMeasureExec(void)
{
  LOG_DEBUG("[BatteryMeasureExec]: start BatteryMeasureExec...\r\n");   
  kMenu_Execute(BatteryMeasureMenu);
  return KMODULE_OK;
}

/**
  * @brief  Measure in RUN mode 
  * @param  None.
  * @note   None.  
  * @retval None.
  */
static void BatteryMeasure_Run(void)
{
  
}

/**
  * @brief  ADC configuration
  * @param  None
  * @retval None
  */
static void ADC_Config(void)
{
 

}

/**
  * @brief  Convert value to display correct volt unit
  * @param  None
  * @retval None
  */
static void Volt_Convert(uint32_t Value, uint16_t *DisplayString)
{
  
}

/**
  * @}
  */

/**
  * @brief ADC MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspInit(uint32_t *hadc)
{
  
}

/**
  * @brief ADC MSP De-Initialization
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(uint32_t *hadc)
{
  
}
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

