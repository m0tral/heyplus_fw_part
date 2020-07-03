
/*
* Copyright (c), NXP Semiconductors
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
* NXP reserves the right to make changes without notice at any time.
*
* NXP makes no warranty, expressed, implied or statutory, including but
* not limited to any implied warranty of merchantability or fitness for any
* particular purpose, or that the use will not infringe any third party patent,
* copyright or trademark. NXP must not be liable for any loss or damage
* arising from its use.
*/

#include <WearableCoreSDK_BuildConfig.h>

#ifndef NFC_UNIT_TEST
//#include "usr_config.h"
//#include "uart_int_dma.h"
#include "FreeRTOS.h"
#include "semphr.h"
//#include "wakeup.h"
//#include "qn_isp.h"
//#include "qn_config.h"
//#include "app_env.h"
#include <time.h>
//#include "port.h"
#include "ble_handler.h"
#include "nci_utils.h"
#include "UserData_Storage.h"
#include "phOsalNfc_Timer.h"
#include "wearable_tlv_types.h"
#include "wearable_platform_int.h"

#include "phOsalNfc_Log.h"

//#include "gup.h"

#if(WEARABLE_MEMORY_ALLOCATION_TYPE == NORMAL_MODE)
uint8_t buffer_tlv [TLV_BUFFER_SIZE];
//uint8_t buffer_tlv [100];

#else
uint8_t buffer_tlv [TLV_BUFFER_SIZE];
#endif

uint8_t gAdvStart = 0;
/*************
 * Queues
 */

QueueHandle_t eaci_queue;
/*QueueHandle_t ble_nfc_semaphore;*/

#define BLE_MAX_PACKET_SIZE 20
#define BLE_TIMEOUT (2000)

static void phBleTimerCallback(uint32_t TimerId, void *pContext);
/*******************************************************************************
**
** Function:    BLEAddressRead()
**
** Description: reads data from flash
**
** Parameters:  data - buffer to store data read
** 				numBytes - num of bytes to be read
**
** Returns:     INT32
**
*******************************************************************************/
INT32 BLEAddressRead(UINT8* data, UINT16 numBytes)
{
	phOsalNfc_Log("\n----- Session Read ----------------\n");

    const Userdata_t *pBleData;
    pBleData = readUserdata(USERDATA_TYPE_QN9022_DEVNAME);
    if((pBleData != NULL) && (pBleData->header.length >= numBytes))
    {
        memcpy(data, pBleData->payload, numBytes);                  //PRQA S 3200
    }
    else
    {
        numBytes = 0;
    }

	phOsalNfc_Log("---------------------------------\n\n");
	return numBytes;
}
/*******************************************************************************
**
** Function:    BLEAddressWrite()
**
** Description: writes data to flash
**
** Parameters:  data - data to be written to flash
** 				numBytes - num of bytes to be written
**
** Returns:     BaseType_t
**
*******************************************************************************/
INT32 BLEAddressWrite(UINT8* data, UINT16 numBytes)
{
	phOsalNfc_Log("\n----- Session Write ----------------\n");

    INT32 ret = -1;
    ret = writeUserdata(USERDATA_TYPE_QN9022_DEVNAME, data, numBytes);
    if(USERDATA_ERROR_NOERROR != ret)
        return -1;

    phOsalNfc_Log("---------------------------------\n\n");
    return numBytes;
}
/*******************************************************************************
**
** Function:    ble_LoadParameters()
**
** Description: Loads parameters ble device address and name stored in flash memory.
**
** Parameters:  address - this will store address read from flash
** 				name - this will store name read from flash
**
** Returns:     BaseType_t
**
*******************************************************************************/
INT32 ble_LoadParameters(uint8_t *address, uint8_t *name) {
	static UINT8 gRawBLEParameters[32];
	static UINT16 gRawBLEParametersSize = 32;
	uint8_t i = 0;

	if ( BLEAddressRead(gRawBLEParameters, gRawBLEParametersSize) < 30)
	{
		return SEAPI_STATUS_FAILED;
	}
	if((gRawBLEParameters[0] != 'B') || (gRawBLEParameters[1] != 'L') || (gRawBLEParameters[2] != 'E')  || (gRawBLEParameters[3] != '0') )
		return SEAPI_STATUS_FAILED;

	for(i = 0; i < 6; i++)
		address[i] = gRawBLEParameters[i + 4];

	for(i = 0; i < 20; i++) {
		name[i] = gRawBLEParameters[i + 10];
	}

	return SEAPI_STATUS_OK;
}
/*******************************************************************************
**
** Function:    vTimerCallback()
**
** Description: Timer callback
**
** Parameters:  pxTimer - Timer handle
**
** Returns:     None
**
*******************************************************************************/
static void phBleTimerCallback(uint32_t TimerId, void *pContext)
{
	phNfcBle_Context_t * pBleContext = (phNfcBle_Context_t *)pContext;
	bool state;
	UNUSED(state);
	volatile UBaseType_t message_in_queue = uxQueueMessagesWaiting(eaci_queue);
	UNUSED(message_in_queue);
	phOsalNfc_LogBle("---------------- TIMER EXCEPTION -------------------");
	//state = Chip_GPIO_GetPinState(LPC_GPIO, 
	//		BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN);
	phOsalNfc_LogBle("state = %d, message_in_queue = %d\n\r",
					state, message_in_queue);

	pBleContext->bytesReceived = 0;
	pBleContext->requiredBytes = 0;
	// The next packet we are expecting is the one coming from the next message
	pBleContext->isFirstMessage = true;
}

/*******************************************************************************
**
** Function:    phNfcDeInitBleSubsystem()
**
** Description: DeInitializes Ble Subsystem
**
** Parameters:  pContext - context is place holder to pass necessary configs.
**
** Returns:     tBLE_STATUS
**
*******************************************************************************/
tBLE_STATUS phNfcDeInitBleSubsystem(void* pContext)
{
	phNfcBle_Context_t * pBleContext = (phNfcBle_Context_t *)pContext;
	tBLE_STATUS status  = BLE_STATUS_OK;
	//vSemaphoreDelete(cfm_semaphore);
	/*vSemaphoreDelete(ble_nfc_semaphore);*/
	vQueueDelete(eaci_queue);
	if(pBleContext->isBleInitialized)
	{
		status = (tBLE_STATUS)phOsalNfc_Timer_Delete(pBleContext->timeoutBleTimerId);
		pBleContext->isBleInitialized = FALSE;
	}
	else
	{
		status = BLE_STATUS_ALREADY_DEINITIALIZED;
	}
	return status;
}
/*******************************************************************************
**
** Function:    phNfcInitBleSubsystem()
**
** Description: Initializes Ble Subsystem
**
** Parameters:  pContext - context is place holder to pass necessary configs.
**
** Returns:     tBLE_STATUS
**
*******************************************************************************/
#if (GDI_GUP_ENABLE == TRUE) // Kanjie

tBLE_STATUS phNfcInitBleSubsystem(void* pContext)
{
	phNfcBle_Context_t * pBleContext = (phNfcBle_Context_t *)pContext;

	if(pBleContext->isBleInitialized)
	{
		return BLE_STATUS_ALREADY_INITIALIZED;
	}
	//-----
	// Load BLE Parameters
    //(void)ble_LoadParameters(pBleContext->BD_Address, pBleContext->BD_Name);

    GeneralState |= BLEINIT_MASK;					// Set NFCINIT bit

    phOsalNfc_LogBle("--- START BLE_INIT  ---\n");
    //wakeup_init();                                 // QN902x and LPC5410x Wake Up init

	/* change for eaci interrupt */
    //gup_init();

    //QN_app_init();                                 // Application -- eaci uart starts to receive if there is data

    /* create a signal semaphore to control the data synchronization between QN and LPC */
    //cfm_semaphore = xSemaphoreCreateCounting(1, 0);
    //vQueueAddToRegistry(cfm_semaphore, "cfm semaphore");

    /* create a signal semaphore to control the  synchronization between BLE and NFC Bridge task
    ble_nfc_semaphore = xSemaphoreCreateCounting(1, 0);
    vQueueAddToRegistry(ble_nfc_semaphore, "ble_nfc_semaphore");*/

    eaci_queue = xQueueCreate(EACI_QUEUE_SIZE, sizeof(void *));
    vQueueAddToRegistry(eaci_queue, "eaci queue");

	pBleContext->timeoutBleTimerId = phOsalNfc_Timer_Create(FALSE);
	if (PH_OSALNFC_TIMER_ID_INVALID == pBleContext->timeoutBleTimerId)
	{
		return BLE_STATUS_FAILED;
	}

	phOsalNfc_LogBle("--- BLE INIT READY ---\n");

	GeneralState &= ~BLEINIT_MASK;					// Set NFCINIT bit
	pBleContext->isFirstMessage = TRUE;
	pBleContext->isBleInitialized = TRUE;
	return BLE_STATUS_OK;
}

#else
tBLE_STATUS phNfcInitBleSubsystem(void* pContext)
{
	phNfcBle_Context_t * pBleContext = (phNfcBle_Context_t *)pContext;

	if(pBleContext->isBleInitialized)
	{
		return BLE_STATUS_ALREADY_INITIALIZED;
	}
	//-----
	// Load BLE Parameters
    //(void)ble_LoadParameters(pBleContext->BD_Address, pBleContext->BD_Name);

    GeneralState |= BLEINIT_MASK;					// Set NFCINIT bit

    phOsalNfc_LogBle("--- START BLE_INIT  ---\n");
    wakeup_init();                                 // QN902x and LPC5410x Wake Up init

#if (ENABLE_OTA == FALSE)
    /* If use OTA, then BLE init and NVDS configuration is already done in bootloader stage.
     * If OTA bootloader is not present, then we have to first do isp()
     * and then follow the same sync mechanism in bootloader as the it's defined by new BLE firmware.
     * BLE_WAKEUP_HOST_PIN (default HIGH after reset) H->L means BLE is ready to receive NVDS from UART.
     * BLE_WAKEUP_HOST_PIN L-> means BLE is done with NVDS and ready for EACI message process
     *  */
	/* Neo: change for isp polling */
    BLE_UART_InitForISP();

    QN_isp_download();                   // download .bin file to QN902X

    /* configure sync IO (BLE wake-up LPC pin) in advance */
    //Chip_IOCON_PinMuxSet(LPC_IOCON, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN, (IOCON_FUNC0 | IOCON_DIGITAL_EN | IOCON_GPIO_MODE));
    //Chip_GPIO_SetPinDIRInput(LPC_GPIO, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN);

    // wait for QN boot
    while (Chip_GPIO_GetPinState(LPC_GPIO, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN));


	/* change for eaci interrupt */
	BLE_UART_InitForEACI();

	// configure NVDS, normally we need read NVDS from 0x7D00(default) or 0x7E00(calibrated)
    uint8_t defaultnvds[] = {EACI_MSG_TYPE_CMD,EACI_MSG_CMD_NVDS,0x16,
                        0x07,0x01, BD_address[0],BD_address[1],BD_address[2],BD_address[3],BD_address[4],BD_address[5],   //bd address
                        0x02,0x02, 0x03,                            //clock drift
                        0x03,0x03, 0xE8,0x03,                       //external wakeup time
                        0X03,0x04, 0xE8,0x03,                       //oscillator wakeup time
                        0x02,0x05, 0x11};                           //xcsel
    eaci_uart_write(sizeof(defaultnvds), defaultnvds);

    // wait for NVDS configuration done
	while (!Chip_GPIO_GetPinState(LPC_GPIO, BLE_WAKEUP_HOST_PORT, BLE_WAKEUP_HOST_PIN));

	// configure device name through EACI, which is not part of NVDS (user configurable data)
	BLE_wakeup();
	uint8_t buffer[50];
	buffer[0] = EACI_MSG_TYPE_CMD;
    buffer[1] = EACI_MSG_CMD_SET_DEVNAME;
    buffer[2] = sizeof(DeviceName);
    memcpy(&buffer[3], DeviceName, sizeof(DeviceName));                   //PRQA S 3200
    eaci_uart_write(sizeof(DeviceName) + 3, buffer);
    BLEgotoSleep();

#else
    BLE_UART_InitForISP();
    BLE_UART_InitForEACI();
#endif

    //wakeup_init();                                 // QN902x and LPC5410x Wake Up init

    QN_app_init();                                 // Application -- eaci uart starts to receive if there is data

	//BLE_wakeup();                                  // Wake up the QN902x

    /* create a signal semaphore to control the data synchronization between QN and LPC */
    cfm_semaphore = xSemaphoreCreateCounting(1, 0);
    vQueueAddToRegistry(cfm_semaphore, "cfm semaphore");

    /* create a signal semaphore to control the  synchronization between BLE and NFC Bridge task
    ble_nfc_semaphore = xSemaphoreCreateCounting(1, 0);
    vQueueAddToRegistry(ble_nfc_semaphore, "ble_nfc_semaphore");*/

    eaci_queue = xQueueCreate(EACI_QUEUE_SIZE, sizeof(void *));
    vQueueAddToRegistry(eaci_queue, "eaci queue");

	pBleContext->timeoutBleTimerId = phOsalNfc_Timer_Create(FALSE);
	if (PH_OSALNFC_TIMER_ID_INVALID == pBleContext->timeoutBleTimerId)
	{
		return BLE_STATUS_FAILED;
	}

	phOsalNfc_LogBle("--- BLE INIT READY ---\n");

	GeneralState &= ~BLEINIT_MASK;					// Set NFCINIT bit
	pBleContext->isFirstMessage = TRUE;
	pBleContext->isBleInitialized = TRUE;
	return BLE_STATUS_OK;
}

#endif // 
/*******************************************************************************
**
** Function:    phNfcStartBleDiscovery()
**
** Description: Starts ble discovery
**
** Parameters:  none
**
** Returns:     none
**
*******************************************************************************/
void phNfcStartBleDiscovery(void)
{
	//BLE_wakeup();
	//app_eaci_cmd_adv(1, (UINT16)ADV_INTV_MIN, (UINT16)ADV_INTV_MAX);           // Advertising
	//BLEgotoSleep();
}

/* BLE Core thread */

void BLETask(void *pvParameters)
{
	struct eaci_msg *p_a_complete_eaci_msg = NULL;

	phOsalNfc_Log("BLE Initialized\n");


	while (1)
	{
		if (xQueueReceive(eaci_queue, &p_a_complete_eaci_msg, portMAX_DELAY) == pdTRUE)
        {
            //printf("Receive eaci message\r\n");
		    //app_eaci_msg_hdl(p_a_complete_eaci_msg->msg_type, p_a_complete_eaci_msg->msg_id,  p_a_complete_eaci_msg->param_len,  p_a_complete_eaci_msg->param, &gphNfcBle_Context);
        }
	}
}

/*******************************************************************************
**
** Function:    phNfcSendResponseToCompanionDevice()
**
** Description: Transmits response packets from NFC to BLE.
**
** Parameters:  pContext - context is place holder to pass necessary configs.
** 				pResposeBuffer - [in] response received from NFC
**
** Returns:     tBLE_STATUS
**
*******************************************************************************/
tBLE_STATUS phNfcSendResponseToCompanionDevice(void* pContext,void* pResposeBuffer)
{
	//phNfcBle_Context_t * pContext = (phNfcBle_Context_t *)context;
	tBLE_STATUS status = BLE_STATUS_OK;
	xTLVMsg* tlvAck = (xTLVMsg*)pResposeBuffer;
	UINT8 payloadOffset = 3; //TODO calculate this based on the number of bytes required to store payload length
	UINT32 length = tlvAck->length + payloadOffset;
	UINT32 count = 0;
	UINT32 offset = 0;
	UINT32 j = 0;
	UINT8 data[20];
	bool isFirstMsg = true;

	//BLE_wakeup();
	phOsalNfc_LogBle("\n Wearable to companion - data to write = %d \n", tlvAck->length);

	while (offset < tlvAck->length && ((GeneralState & BLE_CONNECTED_MASK) != 0))
	{
		if ((length - offset) < BLE_MAX_PACKET_SIZE)
			count = length - offset;
		else
			count = BLE_MAX_PACKET_SIZE;

		if(isFirstMsg == true)
		{
			data[0] = tlvAck->type;
			data[1] = (UINT8)(tlvAck->length & 0xff);
			data[2] = (UINT8)((tlvAck->length >> 8) & 0xff);

			for(j = 0; j < count - payloadOffset; j++)
				data[j + payloadOffset] = tlvAck->value[j];

			offset = count - payloadOffset;
		}
		else
		{
			for(j = 0; j < count; j++)
			{
				data[j] = tlvAck->value[j + offset];
			}

			offset = offset + count;
		}

		//app_eaci_data_req_qpps(QPPS_DATA_SEND_REQ, (UINT8)count, data);   // QPPS_DATA_SEND_REQ is msg_id

		/* wait for local confirmation from QN to send next 20 bytes,
		 * it should be faster than wait for a remote confirmation
		 * If it cannot send within certain time, we should bail out. This may be due to companion
		 * BLE connection being lost */
		//if (xSemaphoreTake(cfm_semaphore, BLE_CFM_TIMEOUT/portTICK_PERIOD_MS  /* portMAX_DELAY */) == pdFALSE)
		//{
		//	phOsalNfc_LogBle("Cfm did not arrive from BLE\n");
		//	break;
		//}

		isFirstMsg = false;
	}

	/*BLE can sleep */
	//Chip_GPIO_SetPinState(LPC_GPIO,HOST_WAKEUP_BLE_PORT,HOST_WAKEUP_BLE_PIN,true);
	//BLEgotoSleep();

	return status;
}
/*******************************************************************************
**
** Function:    phNfcHandleResponseDetails()
**
** Description: Assembles data received (chunks of 20 bytes ) from Ble
**
** Parameters:  pContext - context is place holder to pass necessary configs.
** 				param_len - length of data received from BLE
** 				param - data received from BLE
**
** Returns:     none
**
*******************************************************************************/
void phNfcCreateTLV (void* pContext, uint8_t param_len, uint8_t const *param) {

	phNfcBle_Context_t * pBleContext = (phNfcBle_Context_t *)pContext;
	UINT32 tlvLengthSize = 0;
	UINT32 j = 0;
	BaseType_t xStatus;
	static xTLVMsg tlv;

	if(pBleContext->isFirstMessage == true)
	{
		phOsalNfc_LogBle(".");
		tlv.type = param[0];
		tlv.length = 0;

		/*
		 *ISO/IEC 7816 supports length fields of one, two, … up to five bytes
		  ISO/IEC In ISO/IEC 7816, the values ’80′ and ’85′ to ‘FF’ are invalid for the first byte of length fields.
		  TLV length is of 2 bytes 1st Byte will be 0x81 then max length supported 0 to 255
		  TLV length is of 3 bytes 1st Byte will be 0x82 then max length supported 0 to 65535 */
		if(((param[1] >> 7) & 0x01) == 1)
		{
			uint8_t bytes =  param[1] & 0x0F;

			for (j = 0; j < bytes; j++)
			{
				tlv.length = tlv.length + (param[1 + (bytes - j)] << (8 * j));
			}
			tlvLengthSize = 1 + bytes;
		}
		else
		{
			/*TLV Length is of 1 byte 1st Byte will be 0x00 to 0x7F then Max length supported 0 to 127*/
			tlv.length = param[1];
			tlvLengthSize = 1;
		}

		tlv.value = buffer_tlv;
		phOsalNfc_LogBle("TLV length - %d\n",tlv.length);

		// Copy the content into the buffer (forget about the type and length
        if(tlv.length <= TLV_BUFFER_SIZE)
        {
            for(j = 0; j < (param_len - 1 - tlvLengthSize); j++)
            {
                //tlv.value[j] = param[1 + tlvLengthSize + j];
                buffer_tlv[j] = param[1 + tlvLengthSize + j];
            }
        }
	    // Update the number of bytes received
		pBleContext->bytesReceived = (UINT32)(param_len - 1 - tlvLengthSize);
		// Messages received from now on are part of the fragmented data
		pBleContext->isFirstMessage = false;
		pBleContext->requiredBytes = tlv.length;
		// start the timer for this particular message reception (aprox 2 secs)
		(void)phOsalNfc_Timer_Start(pBleContext->timeoutBleTimerId, BLE_TIMEOUT, &phBleTimerCallback, pBleContext);
	}
	else
	{
		phOsalNfc_LogBle(".");
		// Copy the content into the buffer (forget about the type and length
        if(tlv.length <= TLV_BUFFER_SIZE)
        {
            for(j = 0; j < param_len; j++)
            {
                tlv.value[j + pBleContext->bytesReceived] = param[j];
            }
        }
		pBleContext->bytesReceived = pBleContext->bytesReceived + param_len;
		//phOsalNfc_LogBle("CreateTLV bytes rec - %d\n",pBleContext->bytesReceived);
		// stop the timer and restart it
		(void)phOsalNfc_Timer_Stop(pBleContext->timeoutBleTimerId);
		(void)phOsalNfc_Timer_Start(pBleContext->timeoutBleTimerId, BLE_TIMEOUT, &phBleTimerCallback, pBleContext);
	}

	if(pBleContext->bytesReceived == tlv.length)
	{
		//phOsalNfc_LogBle("Last MSG\r\n");
		// Prepare the stack for the next message to receive
		pBleContext->bytesReceived = 0;
		pBleContext->requiredBytes = 0;
		pBleContext->isFirstMessage = true;

		//stop the timer on receiving last message
		(void)phOsalNfc_Timer_Stop(pBleContext->timeoutBleTimerId);

        if(tlv.length > TLV_BUFFER_SIZE)
        {
            tlv.type = 0xFF; /* Send an invalid tlv so that this error is handled appropriately */
            tlv.length = 0;
        }

		(void)xSemaphoreTake(nfcQueueLock, portMAX_DELAY);
		// Send the TLV to the NFC Stack
		xStatus = xQueueSendToBack( xQueueNFC, &tlv, 0 );
		(void)xSemaphoreGive(nfcQueueLock);
		if( xStatus != pdPASS )
		{
			/* The send operation could not complete because the queue was full -
			  this must be an error as the queue should never contain more than
			  one item! */
			phOsalNfc_LogBle( "Could not send to the NFC queue.\r\n" );
		}
	}
}

#endif
