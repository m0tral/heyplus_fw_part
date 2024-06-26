/**
  ******************************************************************************
  * @file    k_config.h
  * @author  MCD Application Team
  * @brief   Header for k_config.c file
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __K_CONFIG_H
#define __K_CONFIG_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#define DISCOVERY_BOARD   "      STM32L476G-DISCOVERY"
#define DEMO_VERSION      "      VERSION 1.0.5"
#define MFX_INFO          "      MFX VERSION x.y"
#define DEMO_DATE         "      18-APRIL-2016"
#define DEMO_INFO1        "      STMICROELECTRONICS"
#define DEMO_INFO2        "      COPYRIGHT 2016"
   
#define CHOOSEFILE_MAXLEN   255
   
typedef enum{
 MODULE_MAIN_APP,
 MODULE_IDDMEASURE,
 MODULE_BATTERYMEASURE, 
 MODULE_AUDIOPLAYER,
 MODULE_AUDIORECORDER,
 MODULE_COMPASS,
 MODULE_SOUNDMETER,
 MODULE_GUITARTUNER,
 MODULE_LOG_DEMO,
 MODULE_SYSTEM_INFO,
 MODULE_MAX,
 MODULE_NONE = 0xFF
} MODULES_INFO;

/**
 * @brief JOYSTICK Types Definition
 */
typedef enum
{
  JOY_SEL   = 16,
  JOY_LEFT  = 1,
  JOY_RIGHT = 2,
  JOY_DOWN  = 4,
  JOY_UP    = 8,
  JOY_NONE  = 0
} JOYState_TypeDef;

typedef enum {
  TP_CODE_NONE = 0x00,
  TP_CODE_0 = 0x01,
  TP_CODE_1 = 0x02,
  TP_CODE_2 = 0x04,
  TP_CODE_3 = 0x08,
  TP_CODE_4 = 0x10,
}tp_code_t;


#define countof(a) (sizeof(a) / sizeof(*(a)))
/* Exported functions ------------------------------------------------------- */ 

#ifdef __cplusplus
}
#endif

#endif /*__K_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
