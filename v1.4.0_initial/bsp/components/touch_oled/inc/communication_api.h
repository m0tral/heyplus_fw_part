/*******************************************************************************
* File Name: communication_api.h  
* Version 1.0
*
* Description:
* This is the header file for the communication module created by the author.  
* It contains function prototypes and constants for the users convenience. 
********************************************************************************/




#include "cybtldr_utils.h"

/* Slave address for I2C used for bootloading.Replace this constant value with your
*  bootloader slave address project */
#define SLAVE_ADDR 8

/* Function declarations */
int cy8c4011_OpenConnection(void);
int cy8c4011_CloseConnection(void);
int cy8c4011_ReadData(unsigned char* rdData, int byteCnt);
int cy8c4011_WriteData(unsigned char* wrData, int byteCnt);
unsigned char cy8c4011_reg_read(unsigned char ui8Register);



//[] END OF FILE

