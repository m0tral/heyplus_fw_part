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
 * @file ppg_afe4405_handler.c
 * @author Wikus Villet
 * @date December 15, 2016
 *
 *
 ************************************************************************************************************/
#include "sched_alg_afe4410.h"
#include "ppg_afe_clc.h"
#include "afe4410_sensor.h"
#include "ppsi26x.h"
/********************************************/

/*************************************************************************************/

//#TODO: LIFEQ includes, Partner to replace with own includes
/*************************************************************************************/


static ppg_state_t ppgOperationMode = PPG_OPR_OFF;
static afe_partner_setup_t afe_partner_setup;



/**
 * @brief: Loads Partner specified AFE settings
 *       : the default LifeQ AFE setup settings can be over-written with these settings 
 *       : defined in this function @see AFE_init()
 * @param: None
 * @return: RET_OK
 */

static void loadPartnerAfeSettings(void)
{
	afe_partner_setup.prp = 40;
	afe_partner_setup.led1OnTime = 100;// 
	afe_partner_setup.led2OnTime = 100;					
	afe_partner_setup.led3OnTime = 100;					
	afe_partner_setup.led4OnTime = 100;					
	afe_partner_setup.phaseSelection = AFE_GREEN_AMBIENT_IR;
	afe_partner_setup.opticsSelect = AFE_OPTICAL_FRONT_SINGLEPD;
	afe_partner_setup.adcReadyPin = INTERRUPT_FIFO_READY;//INTERRUPT_FIFO_READY;//INTERRUPT_ADC_READY;
	afe_partner_setup.progOut1Pin = INTERRUPT_FIFO_READY;//INTERRUPT_NONE
	afe_partner_setup.externalClockAvailable = false;
	afe_partner_setup.fifoBufferSize = 25;
	afe_partner_setup.fifoEnabled = true;//true;
}



/**
 * @brief: Starts PPG and Accelerometer sensor sampling and initializes the CLC
 * @param: None
 * @return: RET_OK
 */
hqret_t Sensors_startSampling (void)
{
	// Initialize the algorithm
	Alg_Init();
	
	// Initialize the CLC
	uint8_t r1 = clc_CLCInit();
	//clc_CLCSpo2Start();
	
	// Start the AFE
	uint8_t r2 = AFE4410_startSampling() ;
	
	// Start the accelerometer
	// Partner to replace this with own function to start accelerometer sampling
	//SAFE_CALL( SENSOR_ACC_startSampling() );
	LOG_DEBUG_PPS("[hrm] start sampling, r1: %d, r2: %d\n", r1, r2);
	return RET_OK;
}


/**
 * @brief: Stops PPG and Accelerometer sensor sampling
 * @param: None
 * @return: RET_OK
 */
static hqret_t Sensors_stopSampling(void)
{
	//Partner replace this with own function to stop accelerometer sampling
	//SENSOR_ACC_stopSampling();
	//Stop AFE sampling
	AFE4410_stopSampling();
	
	return RET_OK;
}


/**
 * @brief: Initialises the AFE. 
 *       : AFE_Init specifies whether to use the default LifeQ, or Partner specified AFE settings @see loadPartnerAfeSettings
 *       : AFE_Init(true,&afe_partner_setup)  - uses LifeQ recommended settings
 *       : AFE_Init(false,&afe_partner_setup) - overrides LifeQ settings by those contained in loadPartnerAfeSettings
 * @param: None
 * @return: RET_OK or RET_FAIL
 */
hqret_t AfeInitialise()
{
	hqret_t returnValue = RET_FAIL;
	
	loadPartnerAfeSettings();
	
	returnValue = AFE_init(false,&afe_partner_setup);
			
	return returnValue;
}

/**
 * @brief: Returns the fifo depth. This will be the fifo depth used during initialisation which is not necessarily the  
 *       : depth declared in loadPartnerAfeSettings()
 * @param: None
 * @return: afe_partner_setup.fifoBufferSize
 */
uint8_t AFE_INTERFACE_getFifoDepth(void)
{
	return afe_partner_setup.fifoBufferSize;
}