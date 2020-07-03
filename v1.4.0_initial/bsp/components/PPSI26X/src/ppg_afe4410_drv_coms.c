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
* @file ppg_afe4410_drv.c
* @author Wikus Villet
* @date December 12, 2016
*
*
*/
/*************************************************************************************/

//#define AFE_COMS_SPI
#define AFE_COMS_I2C

/*************************************************************************************/
//LifeQ Project Header files
#include "afe4410_sensor.h"

// LifeQ device header files

#include "ppsi26x.h"


#ifdef AFE_COMS_SPI
//#include "spi_hw.h"
#endif

#ifdef AFE_COMS_I2C
//#include "twi_hw_interface.h"
#endif
/*************************************************************************************/
//Prototypes
void HW_writeRegister(uint8_t regAddress, uint32_t data);
void HW_AFE4410_enableRead (void);
void HW_AFE4410_enableWrite(void);	
void HW_AFE4410_enableReadFIFO (void);
void HW_AFE4410_enableWriteFIFO (void);
void HW_burstreadRegister(uint8_t regAddress, uint8_t *data, uint16_t dataLength);
void HW_burstwriteRegister(uint8_t regStartAddress, uint32_t *data, uint8_t amountOfRegisters);
uint32_t HW_readRegister(uint8_t regAddress);


/*************************************************************************************/
// #TODO: The following functions in this file should to be replaced by partners

/**
 * @brief: This function pulls the pin that is connected to the AFE_RESETZ line high
 * @param: None
 * @return: None
 
 MOD_MCU_CONTROL_setAfeResetzHigh()
 
 */

/**
 * @brief: This function pulls the pin that is connected to the AFE_RESETZ line low
 * @param: None
 * @return: None
 
 MOD_MCU_CONTROL_setAfeResetzLow()
 
 */

/**
 * @brief: This function causes a delay specified in microseconds
 * @param: Delay in microseconds
 * @return: None
 
 delay_us(us)
 
 */

/**
 * @brief: This function returns the MCU internal clock specified in ms
 *       : it is intended for future use to determine sampling rate and use for Fs trimming
 * @param: None
 * @return: The system clock time stamp in ms
 
 ITC_HW_getRunTime_ms()
 
 */

/**
 * @brief: Enable external clock supply to AFE
 * @param: None
 * @return: None
 
 MOD_MCU_CONTROL_enableAfeExternalClockSupply()
 
 */

/**
 * @brief: Disable external clock supply to AFE
 * @param: None
 * @return: None
 
 MOD_MCU_CONTROL_disableAfeExternalClockSupply()
 
 */


// #TODO: Implement I2C stuff:
/*
SPI_HW_writeRegister()
SPI_HW_AFE4410_enableRead()
SPI_HW_AFE4410_enableWrite()
SPI_HW_AFE4410_enableReadFIFO()
SPI_HW_AFE4410_enableWriteFIFO()
SPI_HW_burstreadRegister()
SPI_HW_burstwriteRegister()
SPI_HW_readRegister()
*/



/************************************************************************************************************/
// LifeQ functions - do not edit unless necessary

/**
 * @brief: Enables the AFE to read from a register in non FIFO mode
 * @param: None
 * @return: None
 */
static void readEnableNoFIFO (void)
{
	//Call enable read function
	HW_AFE4410_enableRead();
	//Update the live registers
	
}

/**
 * @brief: Enables the AFE to write to a register in non FIFO mode
 * @param: None
 * @return: None
 */
static void writeEnableNoFIFO (void)
{
	//Call the enable write function
	HW_AFE4410_enableWrite();	
}

/**
 * @brief: Read a register value with the AFE not in FIFO mode
 * @param reg: Register address to be read from
 * @return: Unsigned 32 bit register value
 */
static uint32_t regReadNoFIFO (uint8_t reg)
{
	return(HW_readRegister(reg));
}

/**
 * @brief: Write a register value with the AFE not in FIFO mode
 * @param reg: Register address to write to
 * @param regVal: Data to be written to the register
 * @return: None
 */
static void regWriteNoFIFO (uint8_t reg, uint32_t regVal)
{
	HW_writeRegister(reg,regVal);
}


/**
 * @brief: Enables the AFE to read from a register in FIFO mode
 * @param: None
 * @return: None
 */
static void readEnableFIFO (void)
{
	//Enable the AFE read
	HW_AFE4410_enableReadFIFO();
}


/**
 * @brief: Enables the AFE to write to a register in FIFO mode
 * @param: None
 * @return: None
 */
static void writeEnableFIFO (void)
{
	//Enable the AFE write
	HW_AFE4410_enableWriteFIFO();

}

/**
 * @brief: Read a register value with the AFE in FIFO mode
 * @param reg: Register address to be read from
 * @return: Unsigned 32 bit register value
 */
static uint32_t regReadFIFO (uint8_t reg)
{
	return(HW_readRegister(reg));
}

/**
 * @brief: Write a register value with the AFE in FIFO mode
 * @param reg: Register address to write to
 * @param regVal: Data to be written to the register
 * @return: None
 */
static void regWriteFIFO (uint8_t reg, uint32_t regVal)
{
	HW_writeRegister(reg,regVal);
}

/**
 * @brief: Bursts read the FIFO register
 * @param reg: The address of the FIFO register
 * @param inputArrPtr: Pointer to data where the data is to be stored
 * @param amountOfBytes: Number of bytes to be read
 * @return: None
 */
static void burstRegReadFIFO (uint8_t reg, uint8_t* inputArrPtr,uint16_t amountOfBytes)
{
	HW_burstreadRegister(reg,inputArrPtr,amountOfBytes);
}


/**
 * @brief: Burst writes all the registers from a start address to the end of the AFE registers
 * @param start_reg: The register to start the burst write
 * @param regValarr: Pointer to array where the data is stored
 * @param amountOfRegisters: Number of register to be written
 * @return: None
 */
static void burstWrite(uint8_t start_reg,uint32_t *regValarr,uint8_t amountOfRegisters)
{
	HW_burstwriteRegister(start_reg,regValarr,amountOfRegisters);
}

/**
 * @brief: Control the AFE reset line
 * @param level: Stipulates the polarity of the reset line. 1 = High and 0 low
 * @return: None
 */
static void setAfeResetz (uint8_t level)
{

	if(level)
	{
		//This function should changed by the partner
		//This function pulls the AFE reset line high
		//MOD_MCU_CONTROL_setAfeResetzHigh();
		hrm_resetz_highLow(1);
	}
	else
	{
		//This function should changed by the partner
		//This function pulls the AFE reset line low
		//MOD_MCU_CONTROL_setAfeResetzLow();
		hrm_resetz_highLow(0);
	}
}

/**
 * @brief: Delay the MCU in micro seconds
 * @param us: Time in micro seconds
 * @return: None
 */
static void delayMicroSeconds (uint32_t us)
{
	//delay_us(us);
	PPS_DELAY_US(us);
}


/**
 * @brief: Return the Unix time stamp in ms
 * @param : None
 * @return: Unix time stamp in ms
 */
static uint32_t getRunTime_ms(void)
{
	return 1;//ITC_HW_getRunTime_ms();
}


/**
 * @brief: Switches the hardware between internal and external clock
 * @param trueToEnable: trueToEnable = 1 will enable the external clock. 0 will disable the external clock
 * @return: None
 */
static void enableClockSupplyToAfe (uint8_t trueToEnable)
{
//	if (trueToEnable)
//		MOD_MCU_CONTROL_enableAfeExternalClockSupply();
//	else
//		MOD_MCU_CONTROL_disableAfeExternalClockSupply();
}

/**
 * @brief: Wait function for SPI write. HW specific and might not be needed with partner integration
 * @param : None
 * @return: None
 */
static void afeWaitNextTx(void)
{
	PPS_DELAY_MS(1);
}

/************************************************************************************************************/

#ifdef AFE_COMS_SPI

/************************************************************************************************************/
//SPI example code
/**
 * @brief Writes to a HW register by means of the SPI communication
 * @param regAddress: Register address to write to
 * @param data: Data to send to the address
 * @return: none
 */
static void SPI_HW_writeRegister(uint8_t regAddress, uint32_t data)
{
	//Implement a wait. This is HW specific and might not be needed by partners
	afeWaitNextTx();
	// Enable chip select
	SPI_HW_afeSel();
	//  Send the address
	afe8bit(regAddress);
	//  Send thee data
	afe8bit((unsigned char) ((data >> 16) & 0xFF));
	afe8bit((unsigned char) ((data >> 8) & 0xFF));
	afe8bit((unsigned char) ((data >> 0) & 0xFF));
	// Disable chip select
	SPI_HW_afeDeSel();
	
	//Implement wait. This is HW specific and might not be needed by partners
	afeWaitNextTx();

}


/**
 * @brief  Write to register 0x00 to enable read from the AFE
 * @param: None
 * @return: none
 */
static void SPI_HW_AFE4410_enableRead (void)
{
	// Enable chip select
	SPI_HW_afeSel();
	
	//Write address
	afe8bit(0x00);
	
	//Write the data
	afe8bit(0x00);
	afe8bit(0x00);
	afe8bit(0x21);
	
	//Disable chip select
	SPI_HW_afeDeSel();
}


/**
 * @brief  Write to register 0x00 to enable write from the AFE
 * @param: None
 * @return: none
 */
static void SPI_HW_AFE4410_enableWrite (void)
{
	// Enable chip select
	SPI_HW_afeSel();
	
	//Write address
	afe8bit(0x00);
	
	//Write the data
	afe8bit(0x00);
	afe8bit(0x00);
	afe8bit(0x20);
	
	//Disable chip select
	SPI_HW_afeDeSel();
}

/**
 * @brief:  Write to register 0x00 to enable read from the AFE with FIFO enabled
 * @param: None
 * @return: none
 */
static void SPI_HW_AFE4410_enableReadFIFO (void)
{
	// Enable chip select
	SPI_HW_afeSel();
	
	//Write address
	afe8bit(0x00);
	
	//Write the data
	afe8bit(0x00);
	afe8bit(0x00);
	afe8bit(0x61);
	//TODO change to 0x61
	
	//Disable chip select
	SPI_HW_afeDeSel();
}

/**
 * @brief: Write to register 0x00 to enable write to the AFE with FIFO enabled
 * @param: None
 * @return: none
 */
static void SPI_HW_AFE4410_enableWriteFIFO (void)
{
	// Enable chip select
	SPI_HW_afeSel();
	
	// Write address
	afe8bit(0x00);
	
	//Write the data
	afe8bit(0x00);
	afe8bit(0x00);
	afe8bit(0x60);
	//TODO change to 0x60
	
	//Disable chip select
	SPI_HW_afeDeSel();
}

/**
 * @brief Burst reads a single register
 * @param regAddress: Register address to read from
 * @param data: Pointer to data array where the data must be stored
 * @param dataLength: Number of 8bit values to read from the register
 * @return: none
 */
static void SPI_HW_burstreadRegister(uint8_t regAddress, uint8_t * data, uint16_t dataLength)
{
	// Enable chip select
	SPI_HW_afeSel();
	
	// Write the address
	afe8bit(regAddress);
	
	uint16_t pos = 0;
	//Read out the the 8bit values sequentially
	while(pos !=dataLength)
	{
		data[pos] = afe8bit(0x00);
		pos++;
	}
	
	//Disable chip select
	SPI_HW_afeDeSel();
}

/**
 * @brief Burst write a data array to the AFE registers sequentially
 * @param regStartAddress: Register address to start writing from
 * @param data: Pointer to data array where the data resides
 * @param amountOfRegisters: Number of registers to write to
 * @return: none
 */
static void SPI_HW_burstwriteRegister(uint8_t regStartAddress, uint32_t *data, uint8_t amountOfRegisters)
{
	uint32_t tempData = 0x10;
	//Implement a wait. This is HW specific and might not be needed by partners
	afeWaitNextTx();

	// SPI CS will stay low for the duration of the burst write
	SPI_HW_afeSel();
	
	// Write address 0x0 out
	afe8bit(0x0);
	// Set RW_CONT bit in address 0x0
	afe8bit((unsigned char) ((tempData >> 16) & 0xFF));
	afe8bit((unsigned char) ((tempData >> 8) & 0xFF));
	afe8bit((unsigned char) ((tempData >> 0) & 0xFF));
	
	//Set start register where writing will start
	afe8bit(regStartAddress);
	
	//Sequence through registers
	for(uint32_t i=1;i<amountOfRegisters;i++)
	{
		afe8bit((unsigned char) ((data[i] >> 16) & 0xFF));
		afe8bit((unsigned char) ((data[i] >> 8) & 0xFF));
		afe8bit((unsigned char) ((data[i] >> 0) & 0xFF));
	}
	
	//Burst write finished. Disable chip select
	SPI_HW_afeDeSel();
	
	//Implement a wait. This is HW specific and might not be needed by partners
	afeWaitNextTx();

}

/**
 * @brief Read data from a register
 * @param regAddress: Register to read from
 * @return: Return the register data in uint32
 */
static uint32_t SPI_HW_readRegister(uint8_t regAddress)
{
	unsigned char highVal = 0;
	unsigned char midVal = 0;
	unsigned char lowVal = 0;
	unsigned long data = 0;

	// Enable chip select
	SPI_HW_afeSel();
	
	//  Write the address
	afe8bit(regAddress);
	
	//  Read data in 8 bit sequences
	highVal = afe8bit(0x00);
	midVal = afe8bit(0x00);
	lowVal = afe8bit(0x00);
	//Bitshift the data into a uint32
	data = (uint32_t) (lowVal) | ((uint32_t) (midVal) << 8) | ((uint32_t) (highVal) << 16);
	
	//Disable chip select
	SPI_HW_afeDeSel();
	
	//Implement a wait. This is HW specific and might not be needed by partners
	afeWaitNextTx();

	return data;
}
/************************************************************************************************************/

#endif



#ifdef AFE_COMS_I2C
/************************************************************************************************************/
// I2C coms start here
/************************************************************************************************************/


// void I2C_HW_writeRegister(uint8_t regAddress, uint32_t data);
// void I2C_HW_AFE4410_enableRead(void);
// void I2C_HW_AFE4410_enableWrite(void);
// void I2C_HW_AFE4410_enableReadFIFO(void);
// void I2C_HW_AFE4410_enableWriteFIFO(void);
// void I2C_HW_burstreadRegister(uint8_t regAddress, uint8_t *data, uint16_t dataLength);
// void I2C_HW_burstwriteRegister(uint8_t regStartAddress, uint32_t *data, uint32_t amountOfRegisters);
// uint32_t I2C_HW_readRegister(uint8_t regAddress);


 void I2C_HW_writeRegister(uint8_t regAddress, uint32_t data)
{
	//hqret_t ret = RET_FAIL;
	 PPSI26x_writeReg(regAddress, data);
	LOG_DEBUG_PPS("[hrm] I2C_HW_writeRegister, regAddress: %x, data: %x\n", regAddress, data);

	//if(regAddress==0x28 | regAddress == 0x20)
	//{
		//NRF_LOG_RAW_INFO("write:%x--%x \r\n",regAddress,data);
		//tmp1 = tmp1 & 0xf00000;
		//tmp1 = tmp1 | 0x1500;
	//}
//	uint8_t txArr[4] = {(uint8_t)regAddress, (uint8_t)(data>>16) , (uint8_t)(data>>8) , (uint8_t)(data)};
//	ret = TWI_HW_MASTER_write(PPG_AFE4410_TWI_PORT,PPG_AFE4410_TWI_ADDRESS,&txArr[0],4);
//	if(ret != RET_OK)
//	{
//		DBG_STR("TX error");
//	}
/*	TWI_HW_MASTER_writeToRegister(regAddress,data);*/
}

static void I2C_HW_AFE4410_enableRead(void)
{

	I2C_HW_writeRegister(0x00,0x00000021);
}

static void I2C_HW_AFE4410_enableWrite(void)
{

	I2C_HW_writeRegister(0x00,0x00000020);
}

 void I2C_HW_AFE4410_enableReadFIFO(void)
{

	I2C_HW_writeRegister(0x00,0x0000061);

}

 void I2C_HW_AFE4410_enableWriteFIFO(void)
{
	I2C_HW_writeRegister(0x00,0x000060);

}
static void I2C_HW_burstreadRegister(uint8_t regAddress, uint8_t *data, uint16_t dataLength)
{
	//TWI_HW_MASTER_readFromRegister(PPG_AFE4410_TWI_PORT,PPG_AFE4410_TWI_ADDRESS,regAddress,&data[0],dataLength);
	//NRF_LOG_RAW_INFO("LL%d ",dataLength);
  PPSI26x_readRegFIFO(regAddress, data, dataLength);
}

static void I2C_HW_burstwriteRegister(uint8_t regStartAddress, uint32_t *data, uint8_t amountOfRegisters)
{
// 	uint8_t txArr[4] = {(uint8_t)regAddress, (uint8_t)(data>>16) , (uint8_t)(data>>8) , (uint8_t)(data)};
// 	TWI_HW_MASTER_write(PPG_AFE4410_TWI_PORT,PPG_AFE4410_TWI_ADDRESS,&txArr[0],4)
}

static uint32_t I2C_HW_readRegister(uint8_t regAddress)
{
	uint32_t tmp1;
	//uint8_t rxArr[4];
	//TWI_HW_MASTER_readFromRegister(PPG_AFE4410_TWI_PORT,PPG_AFE4410_TWI_ADDRESS,regAddress,&rxArr[0],3);
	//return (uint32_t)(((uint32_t)rxArr[0]<<16) | ((uint32_t)rxArr[1]<<8) | ((uint32_t)rxArr[2])); //returns a 32bit Uint, of which we only use 24 bits
	tmp1 = PPSI26x_readReg(regAddress);
	LOG_DEBUG_PPS("[hrm] I2C_HW_readRegister, regAddress: %x, data: %x\n", regAddress, tmp1);
	
	if(regAddress==0x28)//| regAddress == 0x20
	{
		//NRF_LOG_RAW_INFO("read:%x--%x \r\n",regAddress,tmp1);
		tmp1 = tmp1 & 0xf00000;
		tmp1 = tmp1 | 0x1500;
	}
	return tmp1;
}

/************************************************************************************************************/
#endif

/************************************************************************************************************/

/**
 * @brief: Initialse the AFE comms function pointers
 * @param trueForFifoUse: trueForFifoUse = true will enable the fifo usage. False will disable fifo usage
 * @return: RET_OK if successful. RET_FAIL if unsuccessful.
 */
hqret_t AFE_Comms_Initialise (bool trueForFifoUse)
{

	if(trueForFifoUse)
	{
		afeComs.write_reg = regWriteFIFO;
		afeComs.read_reg = regReadFIFO;
		afeComs.burstRead_reg = burstRegReadFIFO;
		afeComs.enable_write = writeEnableFIFO;
		afeComs.enable_read = readEnableFIFO;
		afeComs.set_afeResetz = setAfeResetz;
		afeComs.set_clockSupply = enableClockSupplyToAfe;
		afeComs.delay_microSeconds = delayMicroSeconds;
		afeComs.getRunTime_ms = getRunTime_ms;
		afeComs.burstWrite_reg = burstWrite;
		return RET_OK;
	}
	else
	{
		afeComs.write_reg = regWriteNoFIFO;
		afeComs.read_reg = regReadNoFIFO;
		afeComs.enable_write = writeEnableNoFIFO;
		afeComs.enable_read = readEnableNoFIFO;
		afeComs.set_afeResetz = setAfeResetz;
		afeComs.set_clockSupply = enableClockSupplyToAfe;
		afeComs.delay_microSeconds = delayMicroSeconds;
		afeComs.getRunTime_ms = getRunTime_ms;
		afeComs.burstWrite_reg = burstWrite;
		return RET_OK;
	}
	
	//Default must be failed
	return RET_FAIL;

}

/************************************************************************************************************/





/************************************************************************************************************/
//HW COMMS SPI AND I2C

/**
 * @brief Writes to a HW register by means of either I2C or SPI communication
 * @param regAddress: Register address to write to
 * @param data: Data to send to the address
 * @return: none
 */
void HW_writeRegister(uint8_t regAddress, uint32_t data)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_writeRegister(regAddress,data);
	#elif defined (AFE_COMS_I2C)
		I2C_HW_writeRegister(regAddress,data);
	#else
		#error No coms protocol defined
	#endif
}


/**
 * @brief  Write to register 0x00 to enable read from the AFE
 * @param: None
 * @return: none
 */
void HW_AFE4410_enableRead (void)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_AFE4410_enableRead();
	#elif defined (AFE_COMS_I2C)
		I2C_HW_AFE4410_enableRead();
	#endif
}


/**
 * @brief  Write to register 0x00 to enable write from the AFE
 * @param: None
 * @return: none
 */
void HW_AFE4410_enableWrite (void)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_AFE4410_enableWrite();
	#elif defined (AFE_COMS_I2C)
		I2C_HW_AFE4410_enableWrite();
	#endif
}

/**
 * @brief:  Write to register 0x00 to enable read from the AFE with FIFO enabled
 * @param: None
 * @return: none
 */
void HW_AFE4410_enableReadFIFO (void)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_AFE4410_enableReadFIFO();
	#elif defined (AFE_COMS_I2C)
		I2C_HW_AFE4410_enableReadFIFO();
	#endif
}

/**
 * @brief: Write to register 0x00 to enable write to the AFE with FIFO enabled
 * @param: None
 * @return: none
 */
void HW_AFE4410_enableWriteFIFO (void)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_AFE4410_enableWriteFIFO();
	#elif defined (AFE_COMS_I2C)
		I2C_HW_AFE4410_enableWriteFIFO();
	#endif
}

/**
 * @brief Burst reads a single register
 * @param regAddress: Register address to read from
 * @param data: Pointer to data array where the data must be stored
 * @param dataLength: Number of 8bit values to read from the register
 * @return: none
 */
void HW_burstreadRegister(uint8_t regAddress, uint8_t *data, uint16_t dataLength)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_writeRegister(0x51, 0x100);
		SPI_HW_writeRegister(0x51, 0x00);
		SPI_HW_burstreadRegister(regAddress,data,dataLength);
	#elif defined (AFE_COMS_I2C)
	  I2C_HW_writeRegister(0x51, 0x100);
		I2C_HW_writeRegister(0x51, 0x00);
		I2C_HW_burstreadRegister(regAddress,data,dataLength);
	#endif
}

/**
 * @brief Burst write a data array to the AFE registers sequentially
 * @param regStartAddress: Register address to start writing from
 * @param data: Pointer to data array where the data resides
 * @param amountOfRegisters: Number of registers to write to
 * @return: none
 */
void HW_burstwriteRegister(uint8_t regStartAddress, uint32_t *data, uint8_t amountOfRegisters)
{
	#if defined (AFE_COMS_SPI)
		SPI_HW_burstwriteRegister(regStartAddress,data,amountOfRegisters);
	#elif defined (AFE_COMS_I2C)
		I2C_HW_burstwriteRegister(regStartAddress,data,amountOfRegisters);
	#endif

}

/**
 * @brief Read data from a register
 * @param regAddress: Register to read from
 * @return: Return the register data in uint32
 */
uint32_t HW_readRegister(uint8_t regAddress)
{
	#if defined (AFE_COMS_SPI)
		return SPI_HW_readRegister(regAddress);
	#elif defined (AFE_COMS_I2C)
		return I2C_HW_readRegister(regAddress);
	#endif
}


void AFE4410_Internal_250Hz_ecg(void)
{
	//Below Is initial of AFE4900 250HZ with PTT mode.
	//250Hz -->500Hz, modify (0x1D	0xFF;0x6B	0xE5)  
	I2C_HW_AFE4410_enableWriteFIFO();//I2C_HW_AFE4410_enableReadFIFO
	//I2C_HW_writeRegister(0x0, 0x20);
	I2C_HW_writeRegister(0x1, 0x2B); 
	I2C_HW_writeRegister(0x2, 0x32); 
	I2C_HW_writeRegister(0x3, 0x41); 
	I2C_HW_writeRegister(0x4, 0x4C); 
	I2C_HW_writeRegister(0x5, 0x38); 
	I2C_HW_writeRegister(0x6, 0x3F); 
	I2C_HW_writeRegister(0x7, 0x45); 
	I2C_HW_writeRegister(0x8, 0x4C); 
	I2C_HW_writeRegister(0x9, 0x27); 
	I2C_HW_writeRegister(0xA, 0x32); 
	I2C_HW_writeRegister(0xB, 0x52); 
	I2C_HW_writeRegister(0xC, 0x59); 
	I2C_HW_writeRegister(0xD, 0x34); 
	I2C_HW_writeRegister(0xE, 0x43); 
	I2C_HW_writeRegister(0xF, 0x45); 
	I2C_HW_writeRegister(0x10, 0x54 ); 
	I2C_HW_writeRegister(0x11, 0x56 ); 
	I2C_HW_writeRegister(0x12, 0x65 ); 
	I2C_HW_writeRegister(0x13, 0x67 ); 
	I2C_HW_writeRegister(0x14, 0x76 ); 
	I2C_HW_writeRegister(0x1D, 0x1FF); 
	I2C_HW_writeRegister(0x36, 0x34     );
	I2C_HW_writeRegister(0x37, 0x3F     );
	I2C_HW_writeRegister(0x43, 0x4E     );
	I2C_HW_writeRegister(0x44, 0x59     );
	I2C_HW_writeRegister(0x52, 0x7C     );
	I2C_HW_writeRegister(0x53, 0x7C     );
	I2C_HW_writeRegister(0x64, 0x0      );
	I2C_HW_writeRegister(0x65, 0x78     );
	I2C_HW_writeRegister(0x66, 0x0      );
	I2C_HW_writeRegister(0x67, 0x78     );
	I2C_HW_writeRegister(0x68, 0x0      );
	I2C_HW_writeRegister(0x69, 0x78     );
	I2C_HW_writeRegister(0x6A, 0x83     );
	I2C_HW_writeRegister(0x6B, 0x1E5    );
	I2C_HW_writeRegister(0x1E, 0x101    );
	I2C_HW_writeRegister(0x1F, 0x0      );
	I2C_HW_writeRegister(0x20, 0x3      );
	I2C_HW_writeRegister(0x21, 0x3      );
	I2C_HW_writeRegister(0x22, 0x0000CC );
	I2C_HW_writeRegister(0x23, 0x124218 );
	I2C_HW_writeRegister(0x29, 0x0      );
	I2C_HW_writeRegister(0x31, 0x20     );
	I2C_HW_writeRegister(0x34, 0x0      );
	I2C_HW_writeRegister(0x35, 0x0      );
	I2C_HW_writeRegister(0x39, 0x0      );
	I2C_HW_writeRegister(0x3A, 0x0      );
	I2C_HW_writeRegister(0x3D, 0x0      );
	I2C_HW_writeRegister(0x3E, 0x0      );
	I2C_HW_writeRegister(0x42, 0xEE2    );//250Hz ,0x42 set 0xEC2 for 120 FIFO depth. period is 60. T=60*4ms=240ms.so interrupt time is 240ms
	I2C_HW_writeRegister(0x45, 0x0      );
	I2C_HW_writeRegister(0x46, 0x0      );
	I2C_HW_writeRegister(0x47, 0x0      );
	I2C_HW_writeRegister(0x48, 0x0      );
	I2C_HW_writeRegister(0x4B, 0x0      );
	I2C_HW_writeRegister(0x4E, 0x24c004 );//0x4 //0x4e 0x24c004 , 0x62 0xE00000 ,two rigsitor contrl "DC lead off" setting
	I2C_HW_writeRegister(0x50, 0x8      );  
	I2C_HW_writeRegister(0x51, 0x0      );   
	I2C_HW_writeRegister(0x54, 0x0      );   
	I2C_HW_writeRegister(0x57, 0x0      );   
	I2C_HW_writeRegister(0x58, 0x0      );   
	I2C_HW_writeRegister(0x61, 0x80000  );   
	I2C_HW_writeRegister(0x62, 0xe00000 );//0x800000  
	I2C_HW_AFE4410_enableReadFIFO();
}

const uint32_t init_registers_25Hz[100]={//with fifo 25Hz*4=100spl. depth is 25.
//25Hz LED onTime 200us 
0x01,0x2B
,0x02,0x37
,0x03,0x4B
,0x04,0x5B
,0x05,0x3D
,0x06,0x49
,0x07,0x4F
,0x08,0x5B
,0x09,0x27
,0x0A,0x37
,0x0B,0x61
,0x0C,0x6D
,0x0D,0x39
,0x0E,0x48
,0x0F,0x4B
,0x10,0x5A
,0x11,0x5D
,0x12,0x6C
,0x13,0x6F
,0x14,0x7E
,0x1D,0x13FF
,0x36,0x39
,0x37,0x49
,0x43,0x5D
,0x44,0x6D
,0x52,0x84
,0x53,0x88
,0x64,0x0
,0x65,0x7F
,0x66,0x0
,0x67,0x7F
,0x68,0x0
,0x69,0x7F
,0x6A,0x8F
,0x6B,0x13E5
,0x1E,0x101
,0x1F,0x0
,0x20,0x0
,0x21,0x5000
,0x22,0x30c3
,0x23,0x124218
,0x28,0x0
,0x39,0x0
,0x3A,0x0
,0x42,0x620 //FIFO Setting
,0x4E,0x0
,0x51,0x0
,0x50,0x28
,0x4B,0xF
,0x3D,0x0

};

void init_reg_ppsx(void)
{
	int i;
	I2C_HW_AFE4410_enableWriteFIFO();
	for(i=0;i<100;){
		I2C_HW_writeRegister(init_registers_25Hz[i], init_registers_25Hz[i+1] );
		i=i+2;
	}
	I2C_HW_AFE4410_enableReadFIFO();
}
