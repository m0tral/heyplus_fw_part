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

/*
 * afe4405_sensor.h
 *
 * Created: 2016-05-13 01:15:25 PM
 *  Author: Tim
 *
 * This is an interface definition for a PPG AFE sensor
 *
 */ 

#ifndef AFE4410_SENSOR_H_
#define AFE4410_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hqerror.h"


/***********************************************************************************/
static int8_t	bufHead = 0;    // The circular buffer position to write to (This is an "empty" BFS!)
static int8_t	bufTail = 0;    // The circular buffer position to read from (This is a full BFS)
//Local definitions to avoid including math.h
#define LOCAL_ABS( a )			            ( ((a) < 0) ? (-(a)) : (a) )
#define LOCAL_INT_DIVIDE(dividend, divisor)    ( ((dividend)*(divisor)>0)?(((dividend)+((divisor)/2)) / (divisor)):(((dividend)-((divisor)/2)) / (divisor)) )    // This uses a compare and only integer operations, Note it returns 0 if the divisor is zero
/***********************************************************************************/


/***********************************************************************************/
//***** The AFE settings below are compiled into the library file and should not be modified.
#define AFE4405_EXTRN_CLK_FREQ      4000000    // Set the external AFE clock frequency @see loadPartnerAfeSettings in ppg_afe4405_handler
#define AFE4410_EXTERNAL_CLKDIV		4          // Set external clock division
#define	CIRC_BUFFER_SIZE			5          // Set the circular buffer size
#define BFS_SIZE_DEF                (25)       // The length of the sample buffers
/***********************************************************************************/


/***********************************************************************************/
// Definitions for different optical configurations
typedef enum
{
	AFE_OPTICAL_FRONT_SINGLEPD    =    0x00,
	AFE_OPTICAL_FRONT_DUALPD	  =    0x01,
	AFE_OPTICAL_FRONT_UNKNOWN     =    0xFF
} afe_optical_component_t;

// Different phase combinations
typedef enum
{
	AFE_ALL_PHASES          =    0x00,
	AFE_GREEN_AMBIENT       =    0x01,
	AFE_GREEN_AMBIENT_IR    =    0x02,    
	AFE_RED_IR_AMBIENT      =    0x03     
} afe_phase_selection_t;

// Interrupts line allocations
typedef enum
{
	INTERRUPT_NONE              = 0,
	INTERRUPT_INT_OUT           = 1,
	INTERRUPT_ADC_READY         = 2,
	INTERRUPT_THR_DET_READY     = 3,
	INTERRUPT_FIFO_READY        = 4
} interrupt_t;

// PPG operation mode
typedef enum
{
	PPG_OPR_OFF				=	0x00,
	PPG_OPR_HR				=	0x01,
	PPG_OPR_SPO2			=	0x02,
	PPG_OPR_SKINDETECT		=	0x03
} ppg_state_t;

// This is the partner setting struct. Partners can define settings in this struct
// LifeQ recommended settings are loaded into this struct
typedef struct 
{
    uint8_t	prp;                           // Pulse repetition time in ms
    uint16_t led1OnTime;                    // Led1 on time in us
	uint16_t led2OnTime;					//Led2 on time in us
	uint16_t led3OnTime;					//Led3 on time in us
	uint16_t led4OnTime;					//Led4 on time in us
    afe_phase_selection_t phaseSelection;  // Define the phase combination
    afe_optical_component_t opticsSelect;  // Specify the optics selected
    interrupt_t adcReadyPin;               // Specify the interrupt on the ADC ready pin
    interrupt_t progOut1Pin;               // Specify the interrupt on the ProgOut1Pin
    bool externalClockAvailable;           // Define which clock is used. True if an external clock is used
    uint8_t fifoBufferSize;                // Size of the fifo buffer
    bool fifoEnabled;                      // Enable or disable the FIFO buffer. True  = fifo enabled
} afe_partner_setup_t;
/***********************************************************************************/


/***********************************************************************************/
//***** Data log structs are declared here

typedef int64_t lq_date_time_t ;

// 16 bit samples. Unit = raw Volt codes
typedef struct
{
    uint16_t    chan1Sample;
    uint16_t    chan2Sample;
    uint16_t    chan3Sample;
    uint16_t    chan4Sample;
} voltcodes_sample_struct_t;

// 32 bit int PD Sample. Unit = pA
typedef struct
{
    int32_t    chan1pdCurr;
    int32_t    chan2pdCurr;
    int32_t    chan3pdCurr;
    int32_t    chan4pdCurr;
} pd_sample_struct_t;

// AFE Settings
typedef struct
{
    uint8_t		chan1Isub;                  // The index of the 1st subtraction current value
    uint8_t		chan2Isub;                  // The index of the 2nd subtraction current value
    uint8_t		chan3Isub;                  // The index of the 3rd subtraction current value
    uint8_t		chan4Isub;                  // The index of the 4th subtraction current value

    uint8_t		chan1Rf;                    // The index of the 1st RF value
    uint8_t		chan2Rf;                    // The index of the 2nd RF value
    uint8_t		chan3Rf;                    // The index of the 3rd RF value
    uint8_t		chan4Rf;                    // The index of the 4th RF value

    uint8_t		chan1LEDcurrent;            // The index of the 1st led value
    uint8_t		chan2LEDcurrent;            // The index of the 2nd led value
    uint8_t		chan3LEDcurrent;            // The index of the 3rd led value

    uint16_t	sampleStateFlag;            // Set to 1 if an AFE adjustment is made. Set to 0 to ack that the change is logged and sent to Alg. This is 
    										// left uint16_t for future development
	
    uint16_t    ledCurr_step_uA;            // The scaling of the LED current register. Scaling is 50mA = 1,  100mA = 2
	
} afe_settings_struct_t;

// BFS Struct (AFE data struct)
typedef struct
{
    uint32_t                    noActiveSamples;                    // The number of active samples in the arrays of this structure. Feature not used for the 4405
    voltcodes_sample_struct_t   voltCodes[BFS_SIZE_DEF];            // Array of raw sample volt codes
    pd_sample_struct_t          pdCurrents[BFS_SIZE_DEF];           // Array of photo diode currents in pico amps
    afe_settings_struct_t       afeSettings;                        // The AFE settings the ALL samples were recorded with
    lq_date_time_t              timeStamp;							// Time stamp of the beginning of the first sample. Feature not used for the 4405 yet.
} bfs_struct_t;
/***********************************************************************************/


/***********************************************************************************/
//***** Prototypes

// Structure for allowing generic communications for differing partner implementations
typedef struct
{
    uint32_t (*read_reg) (uint8_t);
    void (*burstRead_reg) (uint8_t,uint8_t*,uint16_t);      
    void (*write_reg)(uint8_t,uint32_t);
    void (*enable_write) (void);
    void (*enable_read) (void);
    void (*set_afeResetz) (uint8_t);                        
    void (*set_clockSupply) (uint8_t);                      
    void (*delay_microSeconds) (uint32_t);
    uint32_t (*getRunTime_ms)(void);                        
    void (*burstWrite_reg) (uint8_t,uint32_t*,uint8_t);
} afe4410_coms_t;

extern afe4410_coms_t afeComs;	

hqret_t AfeInitialise(void);
uint8_t AFE_INTERFACE_getFifoDepth(void);
hqret_t Sensors_startSampling (void);



//Library functions

/**
 * @brief This function starts the AFE 4405 hardware initialisation process.
 * @param None
 * @return RET_OK if successful. Return RET_FAIL if unsuccessful.
 */
hqret_t AFE_init(bool loadRecommendedSettings, afe_partner_setup_t *partnerSettingsRequest);

/**
 * @brief Clear circular buffer, bring the AFE out of power down and burst write the setup registers and start sampling
 * @param None
 * @return RET_OK if successful, return RET_FAIL if unsuccessful
 */
hqret_t AFE4410_startSampling(void);

/**
 * @brief Stop sampling and power down the AFE
 * Name clash with previous library
 * @param None
 * @return RET_OK if successful, return RET_FAIL if unsuccessful
 */
hqret_t AFE4410_stopSampling(void);

/**
 * @brief Service the AFE FIFO interrupt
 * @param None
 * @return RET_OK if successful, return RET_FAIL if unsuccessful
 */
hqret_t AFE_fifoInterruptService(void);

/**
 * @brief	Acknowledge to the driver that a FIFO interrupt is received. Checks if the buffer head is about to overwrite the buffer tail.
			HQ_ERR_BUFFER_FULL size full is returned if this happens. If not then the buffer head pointer is incremented.
			New AFE settings are written immediately
 * @param None
 * @return RET_OK if successful, return HQ_ERR_BUFFER_FULL if the buffer is full
 */
hqret_t AFE_fifoInterruptReceived(void);

/**
 * @brief Acknowledge to the driver that a single AFE interrupt is received
		  Also check if previous interrupt is serviced before this interrupt is received
 * @param None
 * @return RET_OK if successful, return RET_FAIL if the previous interrupt is not serviced yet
 */
hqret_t AFE_interruptReceived(void);

/**
 * @brief Returns the status of the AFE interrupt
 * Boolean will be true if the interrupt has fired. Bool is cleared when AFE_interruptService() is called
 * @param None
 * @return AFE interrupt boolean
 */
bool AFE_getInterruptStatus(void);

/**
 * @brief Service the of the AFE single interrupt
 * @param None
 * @return RET_OK if successful, return RET_FAIL if unsuccessful
 */
hqret_t AFE_interruptService(void);

/**
 * @brief Returns the status of the AFE circular data buffer
 * @param None
 * @return True if the buffer head and tail are not equal. False if they are equal
 * A true return will indicate that the buffer can be read. A false return indicates that the buffer should not be read
 */
bool AFE_getBufferDataReadyStatus(void);

/**
 * @brief This function is used to pull data from the circular buffer. Sends pointer position of bfsStruct at the buffer tail position
 * @return: Pointer to bfsStruct, or NULL if there is no data to send
 */
bfs_struct_t* AFE4410_retrieveData(void);

/**
 * @brief Indicate to the driver that a set of data is pulled from the circular buffer and 
		  Increment the buffer tail
 * @return None
 */
void AFE_FIFODataServiced(void);

/**
 * @brief Converts the register value to ohm
 * @param registerValue: Bit setting for the AFE4405 register
 * @return Rf value in ohm
 */
int32_t AFE4410_RfRegisterToOhm(uint8_t registerValue);

//Versioning
uint8_t AFEDriver_getAfeVer(void);
uint8_t AFEDriver_getMajorVer(void);
uint8_t AFEDriver_getMinorVer(void);
uint8_t AFEDriver_getBuildVer(void);
uint8_t AFEDriver_getHotFix(void);

/**
 * A Copyright and Legal Agreement is applicable to external parties using this LifeQ library.
 * @return  The copyright text notice, Partner Name and reference to accompanying README.txt.
 */
const char* LQ_Copyright(void);



#endif


