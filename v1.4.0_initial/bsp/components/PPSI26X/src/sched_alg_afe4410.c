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
 
 
/**
 * @file sched_alg_afe4405.c
 * @author Wikus Villet
 * @date July 18, 2016
 *
 *
 ************************************************************************************************************/

#include "sched_alg_afe4410.h"
#include "ppg_afe_clc.h"
#include "phys_calc.h"

bool firstsample = true;



void Alg_Init(void)
{
	firstsample = true;
}

#if 1

hqret_t ALGSH_AFE4410_dataToAlg(void)
{
	//lifeq_acc_struct_t*     tempAccStruct		= {0};
	hqret_t ret1;
	hqret_t ret2;
	uint16_t                countActiveSamples	= 0;
	bfs_struct_t*           tempAfeStruct;
	uint16_t ledCur1;
	uint16_t ledCur2 ;
	uint16_t ledCur3;
	int32_t currentSampleArr[4] = {0,0,0,0};
	uint16_t voltageArr[4] = {0,0,0,0};
	int32_t rfArray[4] = {0,0,0,0}; 
			
	// Get the accelerometer samples
	// Partner please replace this

	
	//Get the Samples
	tempAfeStruct = AFE4410_retrieveData();//read raw data from FIFO?

	
	if(firstsample || tempAfeStruct->afeSettings.sampleStateFlag)
	{

		firstsample = false;
		tempAfeStruct->afeSettings.sampleStateFlag = 0;	
	}
	
	//Unit mA*10
	ledCur1 = LOCAL_INT_DIVIDE(tempAfeStruct->afeSettings.chan1LEDcurrent * tempAfeStruct->afeSettings.ledCurr_step_uA*500, 255);
	ledCur2 = LOCAL_INT_DIVIDE(tempAfeStruct->afeSettings.chan2LEDcurrent * tempAfeStruct->afeSettings.ledCurr_step_uA*500, 255);
	ledCur3 = LOCAL_INT_DIVIDE(tempAfeStruct->afeSettings.chan3LEDcurrent * tempAfeStruct->afeSettings.ledCurr_step_uA*500, 255);
	

	 rfArray[GREENPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan1Rf);
	 rfArray[AMBIENTPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan4Rf);
	 rfArray[IRPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan2Rf);
	 rfArray[REDPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan3Rf);
	
	
	int8_t depth;
	for (countActiveSamples=0; countActiveSamples <  (depth = AFE_INTERFACE_getFifoDepth()); countActiveSamples++)
	{
		alg_input_selection_t stateSelector = PP_CALC_ALL;

		//zPrintf(USART1,"FifoDepth = %d \n",AFE_INTERFACE_getFifoDepth());
		//NRF_LOG_RAW_INFO("%d ",depth);
		
		currentSampleArr[GREENPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan1pdCurr;//Green
		currentSampleArr[AMBIENTPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan4pdCurr;//Ambient
		currentSampleArr[IRPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan2pdCurr;//Red
		currentSampleArr[REDPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan3pdCurr;//IR
		
		voltageArr[GREENPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan1Sample;
		voltageArr[AMBIENTPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan4Sample;
		voltageArr[IRPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan2Sample;
		voltageArr[REDPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan3Sample;
		
		/*********************************/
		//Push data to the CLC
	
		ret1 = clc_samplePush(&currentSampleArr[0],&rfArray[0],&voltageArr[0]);

		/*********************************/
#if 1		
		ret2 =  PP_addSamplesTI4405(
		//acc_data.accX,
		//acc_data.accY,
		//acc_data.accZ,
		0,
		0,
		0,
		currentSampleArr[GREENPOS],
		currentSampleArr[AMBIENTPOS],
		currentSampleArr[REDPOS],
		currentSampleArr[IRPOS],
		ledCur1,
		ledCur2,
		ledCur3,
		tempAfeStruct->afeSettings.sampleStateFlag& 0x0F,
		stateSelector);
		//NRF_LOG_RAW_INFO("%d %d %d %d ",currentSampleArr[GREENPOS],currentSampleArr[AMBIENTPOS],currentSampleArr[REDPOS],currentSampleArr[IRPOS]);
		//NRF_LOG_RAW_INFO("%d %d %d ",ledCur1,ledCur2,ledCur3);
		//NRF_LOG_RAW_INFO("%d %d %d %d ",voltageArr[GREENPOS],voltageArr[AMBIENTPOS],voltageArr[REDPOS],voltageArr[IRPOS]);
		//NRF_LOG_RAW_INFO("\r\n");
	}//End of for loop
	

#endif
	
	return RET_OK;
}


#else

hqret_t ALGSH_AFE4410_dataToAlg(void)
{
	//lifeq_acc_struct_t*     tempAccStruct		= {0};
	hqret_t ret,ret1;
	uint16_t                countActiveSamples	= 0;
	bfs_struct_t*           tempAfeStruct;
	uint16_t ledCur1;
	uint16_t ledCur2 ;
	uint16_t ledCur3;
	int32_t currentSampleArr[4] = {0,0,0,0};
	uint16_t voltageArr[4] = {0,0,0,0};
	int32_t rfArray[4] = {0,0,0,0};
		
	// Get the accelerometer samples
	// Partner please replace this
	//tempAccStruct = SENSOR_ACC_retrieveData();
//	if ( tempAccStruct == NULL)
//	{
//	    ret = HQ_ERR_DATA_NOT_AVAILABLE;
//	    return HQ_ERR_DATA_NOT_AVAILABLE;
//	}
	
	//Get the Samples
	tempAfeStruct = AFE4410_retrieveData();

	
	if(firstsample || tempAfeStruct->afeSettings.sampleStateFlag)
	{

		firstsample = false;
		tempAfeStruct->afeSettings.sampleStateFlag = 0;
		
	}
	
	//Unit mA*10
	ledCur1 = LOCAL_INT_DIVIDE(tempAfeStruct->afeSettings.chan1LEDcurrent * tempAfeStruct->afeSettings.ledCurr_step_uA*500, 255);
	ledCur2 = LOCAL_INT_DIVIDE(tempAfeStruct->afeSettings.chan2LEDcurrent * tempAfeStruct->afeSettings.ledCurr_step_uA*500, 255);
	ledCur3 = LOCAL_INT_DIVIDE(tempAfeStruct->afeSettings.chan3LEDcurrent * tempAfeStruct->afeSettings.ledCurr_step_uA*500, 255);
	
	rfArray[GREENPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan1Rf);
	rfArray[AMBIENTPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan4Rf);
	rfArray[REDPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan2Rf);
	rfArray[IRPOS] = AFE4410_RfRegisterToOhm(tempAfeStruct->afeSettings.chan3Rf);
	
	
	for (countActiveSamples=0; countActiveSamples < AFE_INTERFACE_getFifoDepth(); countActiveSamples++)
	{
		alg_input_selection_t stateSelector = PP_CALC_ALL;

		currentSampleArr[GREENPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan1pdCurr;//Green
		currentSampleArr[AMBIENTPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan4pdCurr;//Ambient
		currentSampleArr[REDPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan2pdCurr;//Red
		currentSampleArr[IRPOS] = tempAfeStruct->pdCurrents[countActiveSamples].chan3pdCurr;//IR
		
		voltageArr[GREENPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan1Sample;
		voltageArr[AMBIENTPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan4Sample;
		voltageArr[REDPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan2Sample;
		voltageArr[IRPOS] = tempAfeStruct->voltCodes[countActiveSamples].chan3Sample;
		
		/*********************************/
		//Push data to the CLC
		ret1=clc_samplePush(&currentSampleArr[0],&rfArray[0],&voltageArr[0]);

		/*************************************/
		

		ret =  PP_addSamplesAccG_TI4405(
		//tempAccStruct->g[countActiveSamples].Xg,
		//tempAccStruct->g[countActiveSamples].Yg,
		//tempAccStruct->g[countActiveSamples].Zg,
		1,
		0,
		0,
		
		currentSampleArr[GREENPOS],
		currentSampleArr[AMBIENTPOS],
		currentSampleArr[REDPOS],
		currentSampleArr[IRPOS],
		ledCur1,
		ledCur2,
		ledCur3,
		tempAfeStruct->afeSettings.sampleStateFlag& 0x0F,
		stateSelector);
		
	}//End of for loop
	
	
	return RET_OK;
}
#endif
