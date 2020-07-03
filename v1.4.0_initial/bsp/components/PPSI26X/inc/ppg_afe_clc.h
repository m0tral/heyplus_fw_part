 /* Copyright 2016 LifeQ Global Ltd.
  * All rights reserved. All information contained herein is, and shall 
  * remain, the property of LifeQ Global Ltd. And/or its licensors. Any 
  * unauthorized use of, copying of, or access to this file via any medium 
  * is strictly prohibited. The intellectual and technical concepts contained 
  * herein are confidential and proprietary to LifeQ Global Ltd. and/or its 
  * licensors and may be covered by patents, patent applications, copyright, 
  * trade secrets and/or other forms of intellectual property protection.  
  * Access, dissemination or use of this software or reproduction of this 
  * material in any form is strictly forbidden unless prior written permission 
  * is obtained from LifeQ Global Ltd.
  */ 

#ifndef PPG_AFE_CLC_H
#define PPG_AFE_CLC_H

//#define AFE_4405
#define AFE_4410

#if defined (AFE_4405)
#include "afe4405_sensor.h"
#elif defined (AFE_4410)
#include "afe4410_sensor.h"
#else
#error No AFE driver version defined
#endif

#define GREENPOS (uint8_t)0
#define REDPOS (uint8_t)2
#define IRPOS (uint8_t)1
#define AMBIENTPOS (uint8_t)3

hqret_t clc_samplePush(int32_t *pdCurrentArray,int32_t *currentRfSetting,uint16_t *voltageArr);
hqret_t clc_CLCInit(void);


#endif
