/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/

/*********************************************************************
 * INCLUDES
 */

/* Basic */
#include "ry_type.h"
#include "ry_utils.h"
#include "ry_mcu_config.h"

#if (RY_BLE_ENABLE == TRUE)

#include <project.h>
#include "ble_task.h"
#include "ryos.h"
#include "ryos_timer.h"
//#include "ry_hal_ble.h"

#include "mible.h"
#include "mible_arch.h"
#include "reliable_xfer.h"



/*********************************************************************
 * CONSTANTS
 */ 

//#define RY_BLE_TEST

#define BLE_THREAD_PRIORITY        5
#define BLE_THREAD_STACK_SIZE      1024 // 1K Bytes

/*
 * @brief Mijia Product ID,
 */
#define APP_PRODUCT_ID             0x009C


/*********************************************************************
 * TYPEDEFS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */
ryos_thread_t *ble_thread_handle;

static void app_mible_auth_cb( u8 status );

static mible_appCbs_t mible_cb = {
    .authCbFunc     = app_mible_auth_cb,
	.wifiInfoCbFunc = NULL,
};

/* 'connectionHandle' stores the BLE connection parameters */
cy_stc_ble_conn_handle_t connectionHandle;


/* This flag is used by application to know whether a Central device has been 
   connected. This value is continuously updated in BLE event callback
   function */
bool                deviceConnected = false;

/* This is used to restart advertisements in the main firmware loop */
bool                restartAdvertisement = true;

/* Status flag for the Stack Busy state. This flag is used to notify the 
   application if there is stack buffer free to push more data or not */
bool                busyStatus = false;


/*********************************************************************
 * LOCAL FUNCTIONS
 */





/**
 * @brief   API to start advertisement
 *
 * @param   none
 *
 * @return  None
 */
void ry_ble_startAdv(void)
{
    /* Check if the restartAdvertisement flag is set by the event handler */
    if(restartAdvertisement)
	{
		/* Reset 'restartAdvertisement' flag */
		restartAdvertisement = false;
		
		/* Start Advertisement and enter discoverable mode */
		Cy_BLE_GAPP_StartAdvertisement(0,//CY_BLE_ADVERTISING_FAST,
                       CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);	
	}
}


/**
 * @brief   API to send notification to peer device.
 *
 * @param   connHandle, the connection handle of peer device
 * @param   charUUID, which char value to be sent
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
void ry_ble_cccValUpdate(cy_ble_gatt_db_attr_handle_t cccdHandle,
                              uint8_t value)
{
    /* Local variable to store the current CCCD value */
    uint8_t cccdValue[2];
    
    /* Load the notification status to the CCCD array */
    cccdValue[0] = value;
    cccdValue[1] = 0;
        
    /* Local variable that stores notification data parameters */
    cy_stc_ble_gatt_handle_value_pair_t  cccdValuePair = 
    {
        .attrHandle = cccdHandle,
        .value.len = 2,
        .value.val = cccdValue
    };
    
    /* Local variable that stores attribute value */
    cy_stc_ble_gatts_db_attr_val_info_t  cccdAttributeHandle=
    {
        .connHandle = connectionHandle,
        .handleValuePair = cccdValuePair,
        .offset = 0,
    };
    
    /* Extract flag value from the connection handle - TO BE FIXED*/
    if(connectionHandle.bdHandle == 0u)
    {
        cccdAttributeHandle.flags = CY_BLE_GATT_DB_LOCALLY_INITIATED;
    }
    else
    {
        cccdAttributeHandle.flags = CY_BLE_GATT_DB_PEER_INITIATED;
    }
    
    /* Update the CCCD attribute value per the input parameters */
    Cy_BLE_GATTS_WriteAttributeValue(&cccdAttributeHandle);
}



/**
 * @brief   API to send notification to peer device.
 *
 * @param   connHandle, the connection handle of peer device
 * @param   charUUID, which char value to be sent
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
void ry_ble_sendNotify(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data)
{
    LOG_DEBUG("[ry_ble_sendNotify] uuid:0x%04x, len:%d \r\n", charUUID, len);


    /* Make sure that stack is not busy, then send the notification. Note that 
       the number of buffers in the BLE stack that holds the application data 
       payload are limited, and there are chances that notification might drop 
       a packet if the BLE stack buffers are not available. This error condition
       is not handled in this example project */
    if (Cy_BLE_GATT_GetBusyStatus(connectionHandle.attId) 
                                                     == CY_BLE_STACK_STATE_FREE)
    {
        /* Local variable that stores CapSense notification data parameters */
        cy_stc_ble_gatts_handle_value_ntf_t notify =
        {
            .connHandle               = connectionHandle,
            .handleValPair.attrHandle = CY_BLE_MI_SERVICE_TOKEN_CHAR_HANDLE,
            .handleValPair.value.val  = data,
            .handleValPair.value.len  = len
        };

        /* Send the updated handle as part of attribute for notifications */
        Cy_BLE_GATTS_Notification(&notify);
    }
    
} 


/**
 * @brief   API to send notification with Ryeex Reliable protocol.
 *
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
ry_sts_t ble_reliableSend(uint8_t* data, uint16_t len)
{

    //LOG_DEBUG("[ble_reliableSend] len:%d \r\n", len);


    /* Make sure that stack is not busy, then send the notification. Note that 
       the number of buffers in the BLE stack that holds the application data 
       payload are limited, and there are chances that notification might drop 
       a packet if the BLE stack buffers are not available. This error condition
       is not handled in this example project */
    if (Cy_BLE_GATT_GetBusyStatus(connectionHandle.attId) 
                                                     == CY_BLE_STACK_STATE_FREE)
    {
        /* Local variable that stores CapSense notification data parameters */
        cy_stc_ble_gatts_handle_value_ntf_t notify =
        {
            .connHandle               = connectionHandle,
            .handleValPair.attrHandle = CY_BLE_RYEEX_SERVICE_RELIABLE_DATA_CHAR_HANDLE,
            .handleValPair.value.val  = data,
            .handleValPair.value.len  = len
        };

        /* Send the updated handle as part of attribute for notifications */
        Cy_BLE_GATTS_Notification(&notify);

        return RY_SUCC;
        
    } else {
        /* Busy Wait */
        //LOG_WARN("[ble_reliableSend] busy wait. len = %d.\r\n", len);
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_BUSY);
    }
}

/**
 * @brief   API to send notification with Ryeex Reliable protocol.
 *
 * @param   len, length of the char
 * @param   data, content of the char data to be sent
 *
 * @return  None
 */
ry_sts_t ble_reliableUnsecureSend(uint8_t* data, uint16_t len)
{

    //LOG_DEBUG("[ble_reliableSend] len:%d \r\n", len);


    /* Make sure that stack is not busy, then send the notification. Note that 
       the number of buffers in the BLE stack that holds the application data 
       payload are limited, and there are chances that notification might drop 
       a packet if the BLE stack buffers are not available. This error condition
       is not handled in this example project */
    if (Cy_BLE_GATT_GetBusyStatus(connectionHandle.attId) 
                                                     == CY_BLE_STACK_STATE_FREE)
    {
        /* Local variable that stores CapSense notification data parameters */
        cy_stc_ble_gatts_handle_value_ntf_t notify =
        {
            .connHandle               = connectionHandle,
            .handleValPair.attrHandle = CY_BLE_RYEEX_SERVICE_RELIABLE_UNSECURE_DATA_CHAR_HANDLE,
            .handleValPair.value.val  = data,
            .handleValPair.value.len  = len
        };

        /* Send the updated handle as part of attribute for notifications */
        Cy_BLE_GATTS_Notification(&notify);

        return RY_SUCC;
        
    } else {
        /* Busy Wait */
        //LOG_WARN("[ble_reliableSend] busy wait. len = %d.\r\n", len);
        return RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_BUSY);
    }
}



/**
 * @brief   API to set characteristic value
 *
 * @param   connHandle, the connection handle
 * @param   charUUID, which characteristic to be set
 * @param   len, length of the char data
 * @param   data, content of the char data to be set
 *
 * @return  None
 */
void ry_ble_setValue(uint16_t connHandle, uint16_t charUUID, uint8_t len, uint8_t* data )
{
    cy_stc_ble_gatts_db_attr_val_info_t val;

    LOG_DEBUG("[ry_ble_setValue] uuid:0x%04x, len:%d \r\n", charUUID, len);

    if (charUUID == MISERVICE_CHAR_SN_UUID) {
        val.handleValuePair.attrHandle = CY_BLE_MI_SERVICE_SN_CHAR_HANDLE;
    } else if (charUUID == MISERVICE_CHAR_BEACONKEY_UUID) {
        val.handleValuePair.attrHandle = CY_BLE_MI_SERVICE_BEACONKEY_CHAR_HANDLE;
    } else if (charUUID == MISERVICE_CHAR_VER_UUID) {
        val.handleValuePair.attrHandle = CY_BLE_MI_SERVICE_VER_CHAR_HANDLE;
    }

    val.handleValuePair.value.len = len;
    val.handleValuePair.value.val = data;
    val.connHandle = connectionHandle;
    val.offset = 0;

    /* Extract flag value from the connection handle - TO BE FIXED*/
    if(connectionHandle.bdHandle == 0u) {
        val.flags = CY_BLE_GATT_DB_LOCALLY_INITIATED;
    } else {
        val.flags = CY_BLE_GATT_DB_PEER_INITIATED;
    }

    
    /* Update the CCCD attribute value per the input parameters */
    Cy_BLE_GATTS_WriteAttributeValue(&val);
}


/**
 * @brief   Cypress BLE Event handler function.
 *
 * @param   Event, defined in cy_ble_stack_gap.h
 * @param   eventParameter, the parameter of the event
 *
 * @return  None
 */
uint8_t *t_test_data;

static void ry_ble_eventHandler(uint32_t event, void *eventParameter)
{
    /* Local variable to store the data received as part of the write request
       events */
    cy_stc_ble_gatts_write_cmd_req_param_t   *writeReqParameter;
    mible_charIdx_t id = MIBLE_CHAR_MAX;
    u8_t isBusy;
    
    /* Take an action based on the current event */
    switch (event)
    {
        /* This event is received when the BLE stack is Started */
        case CY_BLE_EVT_STACK_ON:
            
            /* Set restartAdvertisement flag to allow calling Advertisement
               API from the main function */
            restartAdvertisement = true;
            LOG_DEBUG("[BLE]: Stack On.\r\n");
            break;

        /* ~~~~~~~~~~~~~~~~~~~~~~GAP EVENTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
            
        /* If the current BLE state is Disconnected, then the Advertisement
           Start-Stop event implies that advertisement has stopped */
        case CY_BLE_EVT_GAPP_ADVERTISEMENT_START_STOP:

            LOG_DEBUG("[BLE]: Adv Start Stop.\r\n");
            
            /* Check if the advertisement has stopped */
            if (Cy_BLE_GetState() == 0)//CY_BLE_STATE_STOPPED)
            {
                /* Set restartAdvertisement flag to allow calling Advertisement
                   API from the main function */
                restartAdvertisement = true;
            }
            break;
        
        /* This event is received when device is disconnected */
        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:

            LOG_DEBUG("[BLE]: GAP Disconnected.\r\n");
            
            /* Set restartAdvertisement flag to allow calling Advertisement
             API from the main function */
            restartAdvertisement = true;
            break;

        /* ~~~~~~~~~~~~~~~~~~~~~~GATT EVENTS ~~~~~~~~~~~~~~~~~~~~~~~~~~*/
        
        /* This event is received when device is connected over GATT level */    
        case CY_BLE_EVT_GATT_CONNECT_IND:

            LOG_DEBUG("[BLE]: Connected.\r\n");
            u8_t temp[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};            
                
            /* Update attribute handle on GATT Connection*/
            connectionHandle = *(cy_stc_ble_conn_handle_t *) eventParameter;

            /* Notify to Mible */
            arch_ble_onConnected(connectionHandle.bdHandle, temp, 0, MIBLE_ROLE_CENTRAL);

            /* This flag is used by the application to check the connection
               status */
            deviceConnected = true;
            break;
        
        /* This event is received when device is disconnected */
        case CY_BLE_EVT_GATT_DISCONNECT_IND:

            LOG_DEBUG("[BLE]: GATT Disconnected.\r\n");
           
            /* Update deviceConnected flag*/
            deviceConnected = false;

            /* Notify to Mible */
            arch_ble_onDisconnected(connectionHandle.bdHandle, MIBLE_ROLE_CENTRAL);
            
            /* Call the functions that handle the disconnect events for all 
               three custom services */
            //handleDisconnectEventforSlider();
            //handleDisconnectEventforButtons();
            //handleDisconnectEventforRGB();
            break;
        
        /* This event is received when Central device sends a Write command
           on an Attribute */
        case CY_BLE_EVT_GATTS_WRITE_REQ:
            
            
            /* Read the write request parameter */
            writeReqParameter = (cy_stc_ble_gatts_write_cmd_req_param_t *) 
                                eventParameter;

            LOG_DEBUG("[BLE]: Write Req. handle=0x%04x, len = %d\r\n", 
                writeReqParameter->handleValPair.attrHandle,
                writeReqParameter->handleValPair.value.len);

            switch (writeReqParameter->handleValPair.attrHandle) {
                
                case CY_BLE_MI_SERVICE_TOKEN_CHAR_HANDLE:
                    id = MIBLE_CHAR_IDX_TOKEN;
                    break;

                case CY_BLE_MI_SERVICE_TOKEN_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE:
                    id = MIBLE_CHAR_IDX_TOKEN_CCC;
                    ry_ble_cccValUpdate(CY_BLE_MI_SERVICE_TOKEN_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE, 
                        writeReqParameter->handleValPair.value.val[0]);
                    break;

                case CY_BLE_MI_SERVICE_AUTH_CHAR_HANDLE:
                    id = MIBLE_CHAR_IDX_AUTH;
                    break;

                case CY_BLE_MI_SERVICE_SN_CHAR_HANDLE:
                    id = MIBLE_CHAR_IDX_SN;
                    break;

                case CY_BLE_RYEEX_SERVICE_RELIABLE_DATA_CHAR_HANDLE:
                    
                    break;

                case CY_BLE_RYEEX_SERVICE_RELIABLE_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE:
                case CY_BLE_RYEEX_SERVICE_RELIABLE_UNSECURE_DATA_CLIENT_CHARACTERISTIC_CONFIGURATION_DESC_HANDLE:
                    ry_ble_cccValUpdate(writeReqParameter->handleValPair.attrHandle, 
                        writeReqParameter->handleValPair.value.val[0]);
#if 0
                    t_test_data = ry_malloc(1000);
                    int i;
                    for(i = 0; i < 100; i++) {
                        t_test_data[i] = i;
                    }
                    r_xfer_send_start(t_test_data, 100);
#endif
                    break;

                default:
                    break;
            }

            if (id < MIBLE_CHAR_MAX) {
                mible_onCharWritten(writeReqParameter->connHandle.bdHandle,
                    id,
                    writeReqParameter->handleValPair.value.len,
                    writeReqParameter->handleValPair.value.val);
            }
       
            /* Send the response to the write request received. */
            Cy_BLE_GATTS_WriteRsp(connectionHandle);    
            break;

        case CY_BLE_EVT_GATTS_WRITE_CMD_REQ:
            /* Read the write request parameter */
            writeReqParameter = (cy_stc_ble_gatts_write_cmd_req_param_t *) 
                                eventParameter;

            LOG_DEBUG("[BLE CY_BLE_EVT_GATTS_WRITE_CMD_REQ]: Write Req. handle=0x%04x, len = %d\r\n", 
                writeReqParameter->handleValPair.attrHandle,
                writeReqParameter->handleValPair.value.len);
            
            if (CY_BLE_RYEEX_SERVICE_RELIABLE_DATA_CHAR_HANDLE == writeReqParameter->handleValPair.attrHandle) {
                /* Ryeex Service Reliable write  */
                r_xfer_on_receive(writeReqParameter->handleValPair.value.val, 
                    writeReqParameter->handleValPair.value.len, 
                    1);

                r_xfer_register_tx(ble_reliableSend);
            }

            if (CY_BLE_RYEEX_SERVICE_RELIABLE_UNSECURE_DATA_CHAR_HANDLE == writeReqParameter->handleValPair.attrHandle) {
                /* Ryeex Service Reliable write  */
                r_xfer_on_receive(writeReqParameter->handleValPair.value.val, 
                    writeReqParameter->handleValPair.value.len, 
                    0);
                r_xfer_register_tx(ble_reliableUnsecureSend);
            }
            break;

        case CY_BLE_EVT_L2CAP_CONN_PARAM_UPDATE_RSP:
            LOG_DEBUG("[BLE]: Conn Para Updtated.\r\n");
            break;

        case CY_BLE_EVT_GAP_CONN_ESTB:
            LOG_DEBUG("[BLE]: Gap Conn Established.\r\n");
            break;
        
        /* This event is generated when the internal stack buffer is full and no
           more data can be accepted or the stack has buffer available and can 
           accept data. This event is used by application to prevent pushing lot
           of data to the BLE stack. */
        case CY_BLE_EVT_STACK_BUSY_STATUS:
            /* Extract the present stack status */
            busyStatus = *(uint8_t *) eventParameter;

            //LOG_DEBUG("[BLE]: Busy Status: %d\r\n", busyStatus);
            
            
            break;
        
        /* Do nothing for all other events */
        default:
            break;
    }
}


/**
 * @brief   Callback of MIOT BLE binding status
 *
 * @param   Status binding status
 *
 * @return  None
 */
u32_t ble_test_timer_id;
//extern void test_rbp_send();

void ble_test_timer_handler(void* arg)
{
    //test_rbp_send();
    LOG_DEBUG("[ble_test_timer_handler]: Send Test RBP Message.\n");
}



static void app_mible_auth_cb( u8 status )
{
    LOG_DEBUG("[app_mible_auth_cb], status = %d\r\n", status);

    if (status != MIBLE_SUCC) {
        arch_ble_disconnect(connectionHandle.bdHandle);
    }

    /* Start a test timer */
    ry_timer_parm timer_para;
    timer_para.timeout_CB = ble_test_timer_handler;
    timer_para.flag = 0;
    timer_para.data = NULL;
    timer_para.tick = 1;
    timer_para.name = "test_timer";
    ble_test_timer_id = ry_timer_create(&timer_para);
    //ry_timer_start(ble_test_timer_id, 5000, ble_test_timer_handler, NULL);

}



/**
 * @brief   API to init MIOT BLE module
 *
 * @param   None
 *
 * @return  None
 */
void miio_ble_init(void)
{
    mible_init_t init_para;
    char ver[10] = {"1.0.0_1"};
    //uint8_t publicAddr[6] = {0x20, 0x71, 0x9B, 0x19, 0x3E, 0x41};
    uint8_t publicAddr[6] = {0x00, 0xA0, 0x50, 0x00, 0x00, 0x00};

    /* Set Public Address to MIOT BLE module */
    arch_ble_setPublicAddr(publicAddr);

    /* Set FW Version Number */
    ry_ble_setValue(connectionHandle.bdHandle, MISERVICE_CHAR_VER_UUID, 10, (u8_t*)ver);

    /* Init MIOT BLE */
    init_para.productID = APP_PRODUCT_ID;
    init_para.relationship = MIBLE_RELATIONSHIP_WEAK;
    init_para.authTimeout = 15000;
    init_para.cbs = &mible_cb;
    memcpy(init_para.version, ver, 10);
    mible_init(&init_para);
}


/**
 * @brief   BLE Main task
 *
 * @param   None
 *
 * @return  None
 */
int ble_thread(void* arg)
{
    LOG_INFO("[ble thread]: Enter.\r\n");

    /* Init device property */
    device_info_init();

    /* Init Mi BLE */
    miio_ble_init();
    
    while (1) {
       Cy_BLE_ProcessEvents();
       ry_ble_startAdv();
       ryos_delay_ms(10);
    }
}


/**
 * @brief   API to init BLE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ble_init(void)
{
    ry_sts_t status = RY_SUCC;
    
    /* Start the Controller portion of BLE. Host runs on the CM4 */
    if (Cy_BLE_Start(ry_ble_eventHandler) != CY_BLE_SUCCESS) {
        /* Halt the CPU if the BLE initialization failed */
        //CY_ASSERT(0u != 0u);

        status = RY_SET_STS_VAL(RY_CID_BLE, RY_ERR_INIT_FAIL);
    } else {

        /* Init Success, Start a task to handle ble event */
        ryos_threadPara_t para;

        /* Start Test Demo Thread */
        strcpy((char*)para.name, "ble_thread");
        para.stackDepth = 1024;
        para.arg        = NULL; 
        para.tick       = 1;
        para.priority   = BLE_THREAD_PRIORITY;
        ryos_thread_create(&ble_thread_handle, ble_thread, &para);
    }

    
    return status;
}


/***********************************Test*****************************************/



#endif /* (RY_BLE_ENABLE == TRUE) */



