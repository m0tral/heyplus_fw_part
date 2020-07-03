/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
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
#include "main.h"
#include "stm32l4xx_hal.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
osThreadId defaultTaskHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
 

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 71;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/** Configure pins
     PI10   ------> OCTOSPIM_P2_IO1
     PE0   ------> FMC_NBL0
     PB4 (NJTRST)   ------> S_TIM3_CH1
     PI0   ------> SPI2_NSS
     PI9   ------> OCTOSPIM_P2_IO2
     PI7   ------> DCMI_D7
     PE1   ------> FMC_NBL1
     PB5   ------> SAI1_SD_B
     PG9   ------> OCTOSPIM_P2_IO6
     PD0   ------> FMC_D2_DA2
     PI6   ------> OCTOSPIM_P2_CLK
     PI2   ------> SPI2_MISO
     PH12   ------> DCMI_D3
     PI11   ------> OCTOSPIM_P2_IO0
     PB6   ------> I2C1_SCL
     PD4   ------> FMC_NOE
     PD1   ------> FMC_D3_DA3
     PH13   ------> TIM8_CH1N
     PI3   ------> SPI2_MOSI
     PH9   ------> OCTOSPIM_P2_IO4
     PE4   ------> FMC_A20
     PE3   ------> FMC_A19
     PE2   ------> SAI1_MCLK_A
     PB9   ------> SAI1_FS_A
     PB7   ------> FMC_NL
     PG10   ------> OCTOSPIM_P2_IO7
     PD5   ------> FMC_NWE
     PD2   ------> SDMMC1_CMD
     PC10   ------> SDMMC1_D2
     PI4   ------> DCMI_D5
     PA10   ------> USB_OTG_FS_ID
     PA12   ------> USB_OTG_FS_DP
     PE6   ------> SAI1_SD_A
     PE5   ------> SAI1_SCK_A
     PD6   ------> FMC_NWAIT
     PD3   ------> FMC_CLK
     PC11   ------> SDMMC1_D3
     PI5   ------> DCMI_VSYNC
     PA9   ------> USB_OTG_FS_VBUS
     PA11   ------> USB_OTG_FS_DM
     PF2   ------> FMC_A2
     PF1   ------> FMC_A1
     PF0   ------> FMC_A0
     PG12   ------> OCTOSPIM_P2_NCS
     PD7   ------> FMC_NE1
     PC12   ------> SDMMC1_CK
     PC8   ------> SDMMC1_D0
     PG8   ------> I2C3_SDA
     PF3   ------> FMC_A3
     PF4   ------> FMC_A4
     PF5   ------> FMC_A5
     PG13   ------> I2C1_SDA
     PG4   ------> FMC_A14
     PG3   ------> FMC_A13
     PG5   ------> FMC_A15
     PG7   ------> I2C3_SCL
     PC7   ------> S_DATAIN3DFSDM1
     PC9   ------> SDMMC1_D1
     PG1   ------> FMC_A11
     PE10   ------> FMC_D7_DA7
     PB11   ------> USART3_RX
     PD13   ------> FMC_A18
     PG2   ------> FMC_A12
     PD15   ------> FMC_D1_DA1
     PD14   ------> FMC_D0_DA0
     PC0   ------> LPUART1_RX
     PC1   ------> LPUART1_TX
     PC2   ------> S_CKOUTDFSDM1
     PG0   ------> FMC_A10
     PE9   ------> FMC_D6_DA6
     PE15   ------> FMC_D12_DA12
     PD12   ------> FMC_A17_ALE
     PD11   ------> FMC_A16_CLE
     PD10   ------> FMC_D15_DA15
     PC3   ------> ADCx_IN4
     PA0   ------> ADCx_IN5
     PC4   ------> ADCx_IN13
     PF15   ------> FMC_A9
     PE8   ------> FMC_D5_DA5
     PE14   ------> FMC_D11_DA11
     PD9   ------> FMC_D14_DA14
     PD8   ------> FMC_D13_DA13
     PA5   ------> S_TIM2_CH1
     PF14   ------> FMC_A8
     PE7   ------> FMC_D4_DA4
     PE13   ------> FMC_D10_DA10
     PH5   ------> DCMI_PIXCLK
     PA1   ------> S_TIM5_CH2
     PA3   ------> USART2_RX
     PA7   ------> ADCx_IN12
     PF11   ------> DSIHOST_TE
     PF13   ------> FMC_A7
     PE12   ------> FMC_D9_DA9
     PH10   ------> OCTOSPIM_P2_IO5
     PH11   ------> DCMI_D2
     PA2   ------> USART2_TX
     PA4   ------> DCMI_HSYNC
     PB0   ------> ADCx_IN15
     PF12   ------> FMC_A6
     PE11   ------> FMC_D8_DA8
     PB10   ------> USART3_TX
     PH8   ------> OCTOSPIM_P2_IO3
     PB12   ------> S_DATAIN1DFSDM1
     PB13   ------> SPI2_SCK
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  HAL_PWREx_EnableVddIO2();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOH, MIC_VDD_Pin|DSI_SPI_USART_CS_Pin|LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(JOY_SEL_GPIO_Port, JOY_SEL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MFX_WAKEUP_GPIO_Port, MFX_WAKEUP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OCTOSPIM_P2_IO1_Pin OCTOSPIM_P2_IO2_Pin OCTOSPIM_P2_CLK_Pin OCTOSPIM_P2_IO0_Pin */
  GPIO_InitStruct.Pin = OCTOSPIM_P2_IO1_Pin|OCTOSPIM_P2_IO2_Pin|OCTOSPIM_P2_CLK_Pin|OCTOSPIM_P2_IO0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  GPIO_InitStruct.Alternate = GPIO_AF5_OCTOSPIM_P2;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : MIC_VDD_Pin DSI_SPI_USART_CS_Pin LED_GREEN_Pin */
  GPIO_InitStruct.Pin = MIC_VDD_Pin|DSI_SPI_USART_CS_Pin|LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : PSRAM_NBL0_Pin PSRAM_NBL1_Pin PSRAM_A20_Pin PSRAM_A19_Pin 
                           D10_Pin D6_Pin D12_Pin D5_Pin 
                           D11_Pin D4_Pin D10E13_Pin D9_Pin 
                           D8_Pin */
  GPIO_InitStruct.Pin = PSRAM_NBL0_Pin|PSRAM_NBL1_Pin|PSRAM_A20_Pin|PSRAM_A19_Pin 
                          |D10_Pin|D6_Pin|D12_Pin|D5_Pin 
                          |D11_Pin|D4_Pin|D10E13_Pin|D9_Pin 
                          |D8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D6_Pin */
  GPIO_InitStruct.Pin = ARD_D6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
  HAL_GPIO_Init(ARD_D6_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_10_Pin SPI2_MISOs_Pin SPI_MOSIs_Pin */
  GPIO_InitStruct.Pin = ARD_10_Pin|SPI2_MISOs_Pin|SPI_MOSIs_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : DCMI_D7_Pin DCMI_D5_Pin DCMI_VSYNC_Pin */
  GPIO_InitStruct.Pin = DCMI_D7_Pin|DCMI_D5_Pin|DCMI_VSYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_DCMI;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : SAI1_SDB_Pin SAI1_FSA_Pin */
  GPIO_InitStruct.Pin = SAI1_SDB_Pin|SAI1_FSA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : OCTOSPIM_P2_IO6_Pin OCTOSPIM_P2_IO7_Pin OCTOSPIM_P2_CS_Pin */
  GPIO_InitStruct.Pin = OCTOSPIM_P2_IO6_Pin|OCTOSPIM_P2_IO7_Pin|OCTOSPIM_P2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  GPIO_InitStruct.Alternate = GPIO_AF5_OCTOSPIM_P2;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : D2_Pin PSRAM_OE_Pin D3_Pin PSRAM_WE_Pin 
                           PSRAM_WAIT_Pin PSRAM_CLK_Pin PSRAM_NE1_Pin PSRAM_A18_Pin 
                           D1_Pin D0_Pin PSRAM_A17_Pin PSRAM_A16_Pin 
                           D15_Pin D14_Pin D13_Pin */
  GPIO_InitStruct.Pin = D2_Pin|PSRAM_OE_Pin|D3_Pin|PSRAM_WE_Pin 
                          |PSRAM_WAIT_Pin|PSRAM_CLK_Pin|PSRAM_NE1_Pin|PSRAM_A18_Pin 
                          |D1_Pin|D0_Pin|PSRAM_A17_Pin|PSRAM_A16_Pin 
                          |D15_Pin|D14_Pin|D13_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : MFX_IRQ_OUT_Pin */
  GPIO_InitStruct.Pin = MFX_IRQ_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(MFX_IRQ_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DCMI_D3_Pin DCMI_PIXCLK_Pin DCMI_D2_Pin */
  GPIO_InitStruct.Pin = DCMI_D3_Pin|DCMI_PIXCLK_Pin|DCMI_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_DCMI;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : I2C1_SCL_Pin */
  GPIO_InitStruct.Pin = I2C1_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(I2C1_SCL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D9_Pin */
  GPIO_InitStruct.Pin = ARD_D9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
  HAL_GPIO_Init(ARD_D9_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OCTOPSIM_P2_IO4_Pin OCTOSPIM_P2_IO5_Pin OCTOSPI_P2_IO3_Pin */
  GPIO_InitStruct.Pin = OCTOPSIM_P2_IO4_Pin|OCTOSPIM_P2_IO5_Pin|OCTOSPI_P2_IO3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  GPIO_InitStruct.Alternate = GPIO_AF5_OCTOSPIM_P2;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : SAI1_MCKA_Pin SAI1_SDA_Pin SAI1_SCKA_Pin */
  GPIO_InitStruct.Pin = SAI1_MCKA_Pin|SAI1_SDA_Pin|SAI1_SCKA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF13_SAI1;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : PSRAM_ADV_Pin */
  GPIO_InitStruct.Pin = PSRAM_ADV_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(PSRAM_ADV_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : uSD_CMD_Pin */
  GPIO_InitStruct.Pin = uSD_CMD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(uSD_CMD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : uSD_D2_Pin uSD_D3_Pin uSD_CLK_Pin uSD_D0_Pin 
                           uSD_D1_Pin */
  GPIO_InitStruct.Pin = uSD_D2_Pin|uSD_D3_Pin|uSD_CLK_Pin|uSD_D0_Pin 
                          |uSD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTGFS_ID_Pin USB_OTG_FS_DP_Pin USB_OTGFS_DM_Pin */
  GPIO_InitStruct.Pin = USB_OTGFS_ID_Pin|USB_OTG_FS_DP_Pin|USB_OTGFS_DM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : JOY_SEL_Pin */
  GPIO_InitStruct.Pin = JOY_SEL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(JOY_SEL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OTGFS_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_OTGFS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OTGFS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PSRAM_A2_Pin PSRAM_A1_Pin PSRAM_A0_Pin PSRAM_A3_Pin 
                           PSRAM_A4_Pin PSRAM_A5_Pin PSRAM_A9_Pin PSRAM_A8_Pin 
                           PSRAM_A7_Pin PSRAM_A6_Pin */
  GPIO_InitStruct.Pin = PSRAM_A2_Pin|PSRAM_A1_Pin|PSRAM_A0_Pin|PSRAM_A3_Pin 
                          |PSRAM_A4_Pin|PSRAM_A5_Pin|PSRAM_A9_Pin|PSRAM_A8_Pin 
                          |PSRAM_A7_Pin|PSRAM_A6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_14_Pin ARD_15_Pin */
  GPIO_InitStruct.Pin = ARD_14_Pin|ARD_15_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : STMOD_INT_Pin */
  GPIO_InitStruct.Pin = STMOD_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(STMOD_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : I2C1_SDA_Pin */
  GPIO_InitStruct.Pin = I2C1_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(I2C1_SDA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PSRAM_A14_Pin PSRAM_A13_Pin PSRAM_A15_Pin PSRAM_A11_Pin 
                           PSRAM_A12_Pin PSRAM_A10_Pin */
  GPIO_InitStruct.Pin = PSRAM_A14_Pin|PSRAM_A13_Pin|PSRAM_A15_Pin|PSRAM_A11_Pin 
                          |PSRAM_A12_Pin|PSRAM_A10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : DFDATIN3_Pin DF_CKOUT_Pin */
  GPIO_InitStruct.Pin = DFDATIN3_Pin|DF_CKOUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_DFSDM1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : USART3_RX_Pin USART3_TX_Pin */
  GPIO_InitStruct.Pin = USART3_RX_Pin|USART3_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D0_Pin ARD_D1_Pin */
  GPIO_InitStruct.Pin = ARD_D0_Pin|ARD_D1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_A2_Pin ARD_A1_Pin */
  GPIO_InitStruct.Pin = ARD_A2_Pin|ARD_A1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_A4_Pin ARD_A0_Pin */
  GPIO_InitStruct.Pin = ARD_A4_Pin|ARD_A0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : STMOD_PWM_Pin */
  GPIO_InitStruct.Pin = STMOD_PWM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(STMOD_PWM_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_CS_Pin */
  GPIO_InitStruct.Pin = SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI2_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D5_Pin */
  GPIO_InitStruct.Pin = ARD_D5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
  HAL_GPIO_Init(ARD_D5_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USART2_RX_Pin USART2_TX_Pin */
  GPIO_InitStruct.Pin = USART2_RX_Pin|USART2_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : DSI_TE_Pin */
  GPIO_InitStruct.Pin = DSI_TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
//  GPIO_InitStruct.Alternate = GPIO_AF11_DSI;
  HAL_GPIO_Init(DSI_TE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DCMI_HSYNC_Pin */
  GPIO_InitStruct.Pin = DCMI_HSYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF10_DCMI;
  HAL_GPIO_Init(DCMI_HSYNC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_A3_Pin */
  GPIO_InitStruct.Pin = ARD_A3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ARD_A3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MFX_WAKEUP_Pin */
  GPIO_InitStruct.Pin = MFX_WAKEUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MFX_WAKEUP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DFDATIN1_Pin */
  GPIO_InitStruct.Pin = DFDATIN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF6_DFSDM1;
  HAL_GPIO_Init(DFDATIN1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_CLK_Pin */
  GPIO_InitStruct.Pin = SPI2_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(SPI2_CLK_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */ 
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
