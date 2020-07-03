/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
/*****************************************************************************************************
*  Project Name: I2C_Bootloader_Host
*  Project Revision: 2.00
*  Software Version: PSoC Creator 3.3 SP2
*  Device Tested: CY8C4245AXI-483
*  Compilers Tested: ARM GCC
*  Related Hardware: CY8CKIT-042
****************************************************************************************************/
/****************************************************************************************************
* Project Description:
* This is a sample bootloader host program demonstrating PSoC4 bootloading PSoC4.
* The project is tested using two CY8CKIT-042 with PSoC 4 respectively as Bootloader Host and Target.
* PSoC4 on Target must be programmed with the I2C Bootloader Program attached with the app note.
*
* Connections Required
* Host CY8CKIT-042 (PSoC 4 DVK) :
*  P4[1] - SDA - Should be connected to SDA of Target PSoC4 (P3[1] has been pulled up with 2.2k resistor on CY8CKIT-042).
*  P4[0] - SCL - Should be connected to SCL of Target PSoC4 (P3[0] has been pulled up with 2.2k resistor on CY8CKIT-042). 
*  P0[4] - RX  - Should be connected to Pin 10 on the expansion header J8.
*  P0[5] - TX  - Should be connected to Pin 9 on the expansion header J8.
*  P0.7 is internally connected to SW2 on DVK.
*
* Target CY8CKIT-042 (PSoC 4 DVK) : PSoC4 intially programmed with I2C_Bootloader program.
*  P3[1] - SDA - Connected to SDA of Host PSoC 4.
*  P3[0] - SCL - Connected to SCL of Host PSoC 4.
*  P0.7 is internally connected to SW2 on DVK.
*
* Note that the GNDs of both DVKs should be connected together.
*
* Bootload function is defined in main.c: BootloadStringImage which uses Bootloader Host APIs
* to bootload the contents of .cyacd file.
*
* BootloadStringImage function requires each line of .cyacd to be stored as a seperate string in a string array.
* The total number of lines in the file should also be manually calculated and stored as #define LINE_CNT
* The string image of .cyacd file is stored in StringImage.h file.
*
* The following events happens alternatively on each switch press
* On first switch press it will bootload 'stringImage_0' stored in StringImage.h using BootloadStringImage function . 
* On next switch press it will bootload 'stringImage_1' stored in StringImage.h using BootloadStringImage function .
*
* These bootloadable images congtrol LED blinking with green or blue color.
* Note that in order to enter the bootloader from the application program P0.7 - connected to SW2 on Target
* should be pressed so that the application program running on Target PSoC4 enters Bootloader and is ready to 
* bootload a new application code/data . 
*
* I2C Slave Addr of the bootloader is set to 0x08 in communication_api.h
****************************************************************************************************/


#include "cybtldr_parse.h"
#include "cybtldr_command.h"
#include "communication_api.h"
#include "cybtldr_api.h"
#include "StringImage.h"
#include "ry_utils.h"
#include <string.h>
#include "ry_hal_i2c.h"
#include "am_devices_CY8C4014.h"
#include "ryos.h"
#include "ff.h"
#include "am_util_delay.h"
#include "app_interface.h"

/* This structure contains function pointers to the four communication layer functions 
   contained in the communication_api.c / .h */
CyBtldr_CommunicationsData comm1;

/* switch_flag is set upon the user input (button on P0 [7]) */
unsigned char switch_flag = 0u;

/* toggle_appcode alternates between the two bootloadable files */
unsigned char toggle_appcode = 0u;
unsigned char BootloadStringImage(char const * const bootloadImagePtr[],unsigned int lineCount );
unsigned char EnterBooloaderCmd[6]={0x06,0xFA ,0x12 ,0x53, 0x46 ,0x97};
uint32_t test_id=0;
unsigned char cybtldr_entry(char const * const bootloadImagePtr[],unsigned int lineCount )//(void)
{
	/* Place your initialization/startup code here (e.g. MyInst_Start()) */
	unsigned char error, hexData;
	char strStatus[] = {"0x00 \r\n"};

	/* Initialize the communication structure element -comm1 */
	comm1.OpenConnection = &cy8c4011_OpenConnection;
	comm1.CloseConnection = &cy8c4011_CloseConnection;
	comm1.ReadData = &cy8c4011_ReadData;
	comm1.WriteData = &cy8c4011_WriteData;
	comm1.MaxTransferSize = 64u;
    
	cy8c4011_OpenConnection();   

	//ry_hal_i2cm_init(I2C_TOUCH);
	am_util_delay_ms(4);
	test_id=am_devices_cy8c4014_read_id();
	if(test_id!=0){
		LOG_DEBUG("get 0x08 touch id:%04x\r\n",test_id);
		am_devices_cy8c4014_enter_btldr();
	}
	else
	{
		test_id=cy8c4011_reg_read(00);
		LOG_DEBUG("get 0x18 touch 00:%02x\r\n",test_id);
	}

    am_util_delay_ms(1);
	
    LOG_DEBUG("\r\nStart Bootloading....\r\n");
		//test_id=cy8c4011_reg_read(01);
		//LOG_DEBUG("get 0x18 touch 00:%02x\r\n",test_id);
	am_util_delay_ms(20);
    /* Call BootloadStringImage function to bootload stringImage_0 application*/		
	error = BootloadStringImage(bootloadImagePtr,lineCount ); 
	if( error == CYRET_SUCCESS)
	{
		/* Transfer success information to PC */
		LOG_DEBUG("\r\nBootload success!!\r\n");
		am_util_delay_ms(100);
		cy8c4011_OpenConnection();
		test_id=am_devices_cy8c4014_read_id();
		LOG_INFO("\r\nBootload success get 0x08 touch id:%04x\r\n",test_id);
		am_util_delay_ms(1000);

	}
	else 
	{          
		if(error & CYRET_ERR_COMM_MASK) /* Check for comm error*/
		{
			LOG_INFO("Communicatn Err \r\n");
		}
		else /* Else transfer the bootload error code */
		{
			LOG_DEBUG("Bootload Err :");
			hexData = (error & 0xF0u) >> 4;
			strStatus[2] = hexData > 9u ? (hexData - 10u + 'A'):(hexData + '0');
			hexData = error & 0x0Fu;
			strStatus[3] = hexData > 9u ? (hexData - 10u + 'A'):(hexData + '0');
			LOG_ERROR("\r\nBootload Err status:%d\r\n",strStatus);				
		}
	}	
	return error;
}


/****************************************************************************************************
* Function Name: BootloadStringImage
*****************************************************************************************************
*
* Summary:
*  Bootloads the .cyacd file contents which is stored as string array
*
* Parameters:  
* bootloadImagePtr - Pointer to the string array
* lineCount - No. of lines in the .cyacd file(No: of rows in the string array)
*
* Return: 
*  Returns a flag to indicate whether the bootload operation was successful or not
*
*
****************************************************************************************************/
unsigned char BootloadStringImage(char const * const bootloadImagePtr[],unsigned int lineCount )
{
	unsigned char err;
	unsigned char arrayId; 
	unsigned short rowNum;
	unsigned short rowSize; 
	unsigned char checksum ;
	unsigned char checksum2;
	unsigned long blVer=0;
	/* rowData buffer size should be equal to the length of data to be send for each flash row 
	* Equals 128
	*/
	unsigned char* rowData = ry_malloc(128);
	unsigned int lineLen;
	unsigned long  siliconID;
	unsigned char siliconRev;
	unsigned char packetChkSumType;
	unsigned int lineCntr ;
	
 
	/* Initialize line counter */
	lineCntr = 0u;
	
	/* Get length of the first line in cyacd file*/
	lineLen = strlen(bootloadImagePtr[lineCntr]);
	LOG_INFO("[BootloadStringImage] first line len:%d\r\n",lineLen);

	/* Parse the first line(header) of cyacd file to extract siliconID, siliconRev and packetChkSumType */
	err = CyBtldr_ParseHeader(lineLen ,(unsigned char *)bootloadImagePtr[lineCntr] , &siliconID , &siliconRev ,&packetChkSumType);
    
	/* Set the packet checksum type for communicating with bootloader. The packet checksum type to be used 
	* is determined from the cyacd file header information */
	CyBtldr_SetCheckSumType((CyBtldr_ChecksumType)packetChkSumType);
	
	if(err == CYRET_SUCCESS)
	{
		/* Start Bootloader operation */
		//LOG_DEBUG("Communicatn line: %d\r\n",lineCntr);
		err = CyBtldr_StartBootloadOperation(&comm1 ,siliconID, siliconRev ,&blVer);
		lineCntr++ ;
		while((err == CYRET_SUCCESS)&& ( lineCntr <  lineCount ))
		{
            /* Get the string length for the line*/
			lineLen =  strlen(bootloadImagePtr[lineCntr]);
                  
			/*Parse row data*/
			err = CyBtldr_ParseRowData((unsigned int)lineLen,(unsigned char *)bootloadImagePtr[lineCntr], &arrayId, &rowNum, rowData, &rowSize, &checksum);

		         
			if (CYRET_SUCCESS == err)
            {
				/* Program Row */
				err = CyBtldr_ProgramRow(arrayId, rowNum, rowData, rowSize);
				//LOG_DEBUG("Communicatn line: %d\r\n",lineCntr);
	            if (CYRET_SUCCESS == err)
				{
					/* Verify Row . Check whether the checksum received from bootloader matches
					* the expected row checksum stored in cyacd file*/
					checksum2 = (unsigned char)(checksum + arrayId + rowNum + (rowNum >> 8) + rowSize + (rowSize >> 8));
					err = CyBtldr_VerifyRow(arrayId, rowNum, checksum2);
				}
            }
			/* Increment the linCntr */
			lineCntr ++;
		}
		/* End Bootloader Operation */
		CyBtldr_EndBootloadOperation();
		//LOG_DEBUG("CyBtldr_EndBootloadOperation %d\r\n",lineCntr);
	}
    ry_free(rowData);
    return(err);
}
extern uint8_t frame_buffer[];
u8_t * *data_buf = NULL;

u32_t get_file_string_image(void)
{
	u32_t status = 0;
	FRESULT res ;
    u32_t written_bytes;
    u32_t file_size = 0, i = 0;
	data_buf = NULL;
	u32_t line_ptr = 0;
	u8_t * temp = frame_buffer;
	u8_t flag = 0;
    FIL *fp = (FIL *)ry_malloc(sizeof(FIL));
	if(fp == NULL){
		status = 1;
		goto exit;
	}
    
	res = f_open(fp, "touch.bin", FA_READ);
	if(res != FR_OK){
		status = 2;
		goto exit;
	}

	file_size = f_size(fp);

	temp = (u8_t *)ry_malloc(sizeof(u8_t) * file_size);
	if(temp == NULL){
        temp = frame_buffer;
	}

	res = f_read(fp, temp, file_size, &written_bytes);
	if(res != FR_OK){
		status = 3;
		goto exit;
	}
	if(written_bytes != file_size){
		status = 4;
		goto exit;
	}

	data_buf = (u8_t **)ry_malloc(sizeof(u8_t *)*256);
	if(data_buf == NULL){
        status = 5;
        goto exit;
	}

	data_buf[line_ptr] = temp;
	line_ptr++;
	for(i = 0; i < file_size; i++){
		if((temp[i] == 0x0D) &&(temp[ i + 1] == 0x0A)){
			temp[i] = 0;
			if(temp[i + 2] == 0x3A){
				data_buf[line_ptr++] = &temp[i + 2];
			}
		}
	}

	/*for(i = 0; i < tp_img_array[4].line_len; i++){
		if(strcmp(data_buf[i], stringImage_2[i]) != 0){
			flag = 1;
			break;
		}

	}*/
	temp[file_size] = 0;
	if(flag){
		status = i;
	}

exit:
    if(temp == frame_buffer){
        
    }else{
        ry_free(temp);
    }
	f_close(fp);
	ry_free(fp);
	return status;
}


uint32_t tp_firmware_upgrade(void)
{
    uint32_t status = RY_SUCC;
#if UPGEADE_TP_ENABLE
    char const* const* data_ptr = NULL;
    LOG_DEBUG("[touch_upgrage]old version & id is %04x\r\n", touch_get_id());

    status = get_file_string_image();

    //if(status == 0){
    //    data_ptr = data_buf;
    //}else{
        data_ptr = stringImage_2;
    //}
	if (cybtldr_entry(data_ptr, tp_img_array[7].line_len) != 0){
        LOG_INFO("[touch_upgrage] failed.\r\n");
		status = 1;
    }
    else{
        LOG_INFO("[touch_upgrage] succ, new version & id is %04x\r\n", touch_get_id());
		status = 0;
    }
    ry_free(data_buf);
    data_buf = NULL;
#endif
    return status;
}

/* [] END OF FILE */
