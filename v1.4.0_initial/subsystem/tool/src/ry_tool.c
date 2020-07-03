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
#include <ry_hal_inc.h>

#include "app_config.h"

#if (RT_LOG_ENABLE == TRUE)
    #include "ry_log.h"
    #include "ry_log_id.h"		
#endif

#if (RY_TOOL_ENABLE == TRUE)
#include "ff.h"
#include "r_xfer.h"
#include "ry_hal_uart.h"
#include "ryos.h"
#include "json.h"
#include "ry_hal_spi_flash.h"
#include "ry_fs.h"
#include "ry_tool.h"

#include "am_bsp.h"
#include "am_util.h"
#include "am_bsp_gpio.h"
#include <stdio.h>
#include <string.h>
#include "crc32.h"


/*********************************************************************
 * CONSTANTS
 */ 



/*********************************************************************
* TYPEDEFS
*/

 typedef struct recvDataPackHead{
	
	u16_t magic;
	u16_t pack_len;
	u32_t file_len;
	u32_t offset;
}recvDataPackHead_t;
 
/*********************************************************************
 * LOCAL VARIABLES
 */
ryos_thread_t    *ry_tool_thread_handle;
ry_queue_t       *ry_tool_rxQ;
ryos_semaphore_t *ry_tool_sem;
uint32_t         currentRecvDataLen = 0;
volatile ry_tool_ctx_t    tool_ctx_v;
u16_t recv_data_crc;


/*********************************************************************
 * LOCAL FUNCTIONS
 */

void uart_comm_mode_init(void);

void recv_resource_data(u8_t* data, u32_t len)
{

//don't remove it, lixueliang test used
#if 0
    //static uint32_t currentRecvDataLen = 0;
    u8_t resp = 0;
    recvDataPackHead_t * head = (recvDataPackHead_t *)data;

    #if 0
    ry_hal_spi_flash_init();

    if (currentRecvDataLen == 0){

        int i;

        for (i = 0; i <= (head->file_len)/EXFLASH_BYTES_SIZE ; i++){
            
            ry_hal_spi_flash_sector_erase(head->offset + i * EXFLASH_BYTES_SIZE);
            
        }
        
    }
    
    ry_hal_spi_flash_write(&data[sizeof(recvDataPackHead_t)], currentRecvDataLen, head->pack_len);

    currentRecvDataLen += head->pack_len;
    
    
    if(currentRecvDataLen >= head->file_len){

        currentRecvDataLen = 0;
    }

    #else 
    static FIL* file;
    FRESULT res ;
    u32_t written_bytes;

    char fileName[30] = "test.img";

    if (currentRecvDataLen == 0){

        strcpy((char*)fileName, (const char*)&data[sizeof(recvDataPackHead_t)]);

        file = (FIL*)ry_malloc((sizeof(FIL)));
        if (NULL == file) {
            LOG_ERROR("[recv_resource_data] No mem.\r\n");
            goto exit;
        }

    	res = f_open(file, fileName, FA_CREATE_ALWAYS | FA_WRITE);

    	if (res != FR_OK) {

    		LOG_DEBUG("file open failed \n");
    		goto exit;
    	}
    	currentRecvDataLen += 1;
    	return ;
	}
    
	if(currentRecvDataLen == 1){
        recv_data_crc = calc_crc16_r(&data[sizeof(recvDataPackHead_t)], head->pack_len, 0);
    }else{
    
	    recv_data_crc = calc_crc16_r(&data[sizeof(recvDataPackHead_t)], head->pack_len, recv_data_crc);
	}

	res = f_write(file, &data[sizeof(recvDataPackHead_t)], head->pack_len, &written_bytes);

	if (res != FR_OK) {

		LOG_DEBUG("file write failed \n");
		goto exit;
	}
    
	
    currentRecvDataLen += head->pack_len;
    
    
    if(currentRecvDataLen >= head->file_len + 1){

        res = f_close(file);

        ry_free((u8_t*)file);

        if (res != FR_OK) {

    		LOG_DEBUG("file close failed \n");
    		goto exit;
    	}

        currentRecvDataLen = 0;
    }
	
    
    
    #endif
    static int packCount;

    LOG_DEBUG("count = %d\t\t len = %d",packCount++, currentRecvDataLen);

    resp = 0x55;
    ry_hal_uart_tx(UART_IDX_TOOL, &resp, 1);

exit:

#endif



    return ;
}
u32_t pack_count = 0;
void recv_filesystem_data(u8_t* data, u32_t len)
{

//don't remove it, lixueliang test used
#if 0
    //static uint32_t currentRecvDataLen = 0;
    recvDataPackHead_t * head = (recvDataPackHead_t *)data;

    u8_t resp = 0x55;
    int i;
    pack_count++;

    if (currentRecvDataLen == 0){

        //int i;
        ry_hal_spi_flash_chip_erase();
        /*for (i = 0; i < (head->file_len + EXFLASH_SECTOR_SIZE - 1)/EXFLASH_SECTOR_SIZE ; i++){
            
            ry_hal_spi_flash_sector_erase((FS_START_SECTOR + i) * EXFLASH_SECTOR_SIZE);
            
        }*/
        currentRecvDataLen += 1;
        ry_hal_uart_tx(UART_IDX_TOOL, &resp, 1);
    	return ;
    }

	if(currentRecvDataLen == 1){
        recv_data_crc = calc_crc16_r(&data[sizeof(recvDataPackHead_t)], head->pack_len, 0);
    }else{
    
	    recv_data_crc = calc_crc16_r(&data[sizeof(recvDataPackHead_t)], head->pack_len, recv_data_crc);
	}
	u16_t pack_crc = calc_crc16_r(&data[0], 256+sizeof(recvDataPackHead_t), 0);

    if(currentRecvDataLen == 1){
        currentRecvDataLen = 0;
    }
    ry_hal_spi_flash_write(&data[sizeof(recvDataPackHead_t)], (FS_START_SECTOR*EXFLASH_SECTOR_SIZE)+currentRecvDataLen, head->pack_len);

    currentRecvDataLen += head->pack_len;
    
    LOG_DEBUG("len = %d\n", currentRecvDataLen);
    /*if(currentRecvDataLen >= head->file_len){

        currentRecvDataLen = 0;
    }*/

    LOG_DEBUG("\n\n%d p-crc = 0x%x\n\n",pack_count,pack_crc);

    ry_hal_uart_tx(UART_IDX_TOOL, (u8_t*)&pack_crc, 2);

#endif
    
}


/**
 * @brief   RX callback from UART module
 *
 * @param   None
 *
 * @return  None
 */
void ry_tool_uartRxCb(ry_uart_t idx)
{
    if (idx == UART_IDX_TOOL) {
        ryos_semaphore_post(ry_tool_sem);
    }
}


/**
 * @brief   TX task of Reliable Transfer
 *
 * @param   None
 *
 * @return  None
 */
int ry_tool_thread(void* arg)
{
    ry_sts_t status = RY_SUCC;
    //ry_ble_msg_t *rxMsg;
    void* rxMsg;
    u8_t* buf;
    uint32_t len;

    LOG_DEBUG("[ry_tool_thread] Enter.\n");
#if (RT_LOG_ENABLE == TRUE)
    ryos_thread_set_tag(NULL, TR_T_TASK_TOOL);
#endif
	
#if 0        
    /* Create the RYBLE RX queue */
    status = ryos_queue_create(&ry_tool_rxQ, 5, sizeof(void*));
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_tool_thread]: RX Queue Create Error.\r\n");
        RY_ASSERT(RY_SUCC == status);
    }
#endif    
    while (1) {
#if 0
        status = ryos_queue_recv(ry_tool_rxQ, &rxMsg, RY_OS_WAIT_FOREVER);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_tool_thread]: Queue receive error. Status = 0x%x\r\n", status);
        } else {
            //ry_ble_msgHandler(rxMsg);
        }

        ryos_delay_ms(10);
#endif
        // processing uart mode
        LOG_DEBUG("[%s] wait for sem\n",__FUNCTION__);
        status = ryos_semaphore_wait(ry_tool_sem);
        if (RY_SUCC != status) {
            LOG_ERROR("[ry_tool_thread]: Sem Wait Error.\r\n");
            RY_ASSERT(RY_SUCC == status);
        }

       if (tool_ctx_v.io_mode == 0){              
            // if (tool_ctx_v.gpio_intterup >= 4)
            {
                am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TOOL_UART_RX));
                am_hal_gpio_int_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TOOL_UART_RX));  
                am_hal_gpio_pin_config(AM_BSP_GPIO_TOOL_UART_RX, AM_HAL_PIN_DISABLE);
                uart_comm_mode_init();
                tool_ctx_v.io_mode = 1;
                tool_ctx_v.gpio_intterup = 0;
                LOG_INFO("[ry_tool_thread] uart io change to uart mode.\n");
            }
            // LOG_DEBUG("[ry_tool_thread]: uart_mode:%d, cnt%d\n", tool_ctx_v.io_mode, tool_ctx_v.gpio_intterup);            
        }
        else
        {     
            buf = ry_malloc(500);
            ry_memset(buf, 0, 500);

            u16_t * check_head = (u16_t *)buf;
            char * check_json = (char *)buf;
            
            ry_hal_uart_rx(UART_IDX_TOOL, buf, &len);

            LOG_INFO("[ry_tool_thread]: %s\n", buf);

            if(*check_head == 0xAA33){
                recv_resource_data(buf,len);
            }else if(*check_head == 0xAA22){
                recv_filesystem_data( buf, len);
            }else if(*check_json == '{'){
                //ry_hal_uart_tx(UART_IDX_TOOL, buf, len);
                json_parser(buf, len, JSON_SRC_UART);
            }
            if ((len == 0) && (tool_ctx_v.no_uart_cnt <= 3)){
                tool_ctx_v.no_uart_start = ry_hal_clock_time();
            }
            

            if (len == 0){
                tool_ctx_v.no_uart_cnt ++;
                if (ry_hal_calc_ms(ry_hal_clock_time() - tool_ctx_v.no_uart_start) > 3000){
                    if (tool_ctx_v.no_uart_cnt > 64){
                        tool_ctx_v.no_uart_cnt = 0;
                        uart_gpio_mode_init();
                        LOG_DEBUG("[ry_tool_thread]:no uart detect, uart_mode:%d, len:%d, buff:%s\n", tool_ctx_v.io_mode, len, buf);   
                    }                     
                    else{
                        tool_ctx_v.no_uart_start = ry_hal_clock_time();
                    }
                }
            }
            else{
                tool_ctx_v.no_uart_cnt = 0;
            }

            LOG_DEBUG("[ry_tool_thread]: no_uart_cnt:%d, time: %d, len:%d, buff:%s\n\r\n\r", \
                tool_ctx_v.no_uart_cnt, ry_hal_calc_ms(ry_hal_clock_time() - tool_ctx_v.no_uart_start), len, buf);
            ry_free(buf);
        }
    }
}


void ry_tool_onReliableReceived(u8_t* data, u32_t len, void* usr_data)
{
    
}


ry_sts_t ry_tool_uart_send(u8_t* data, u32_t len, void* usr_data)
{
    ry_hal_uart_tx(UART_IDX_TOOL, data, len);
    return RY_SUCC;
}

extern ryos_semaphore_t *dc_in_sem;
void rx_in_irq(void)
{
    // ry_board_debug_printf("got rx in interrupt. %d\r\n", tool_ctx_v.gpio_intterup++);
    //if (tool_ctx_v.gpio_intterup == 3){
        ryos_semaphore_post(ry_tool_sem);
    //}
    //tool_ctx_v.gpio_intterup ++;
}


void uart_gpio_mode_init(void)
{
    tool_ctx_v.io_mode = 0;
    tool_ctx_v.gpio_intterup = 0;
    
    ry_hal_uart_powerdown(UART_IDX_TOOL);
    
    am_hal_gpio_pin_config(AM_BSP_GPIO_TOOL_UART_RX, AM_HAL_GPIO_INPUT | AM_HAL_GPIO_PULL24K); 
    am_hal_gpio_int_polarity_bit_set(AM_BSP_GPIO_TOOL_UART_RX,  AM_HAL_GPIO_FALLING);   //AM_HAL_GPIO_RISING， 

    am_hal_gpio_int_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TOOL_UART_RX));
    am_hal_gpio_int_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_TOOL_UART_RX));

    ry_hal_gpio_registing_int_cb(GPIO_IDX_UART_RX, rx_in_irq);        
    // am_hal_interrupt_enable(AM_HAL_INTERRUPT_GPIO);
}

void uart_comm_mode_init(void)
{
    /* Init Uart */
    ry_hal_uart_init(UART_IDX_TOOL, ry_tool_uartRxCb);
    ry_hal_uart_powerup(UART_IDX_TOOL);
}


/**
 * @brief   API to init ry_tool_thread_init
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_tool_thread_init(void)
{
    ryos_threadPara_t para;
    ry_sts_t status;
    
    /* Start ry_tool_thread */
    strcpy((char*)para.name, "ry_tool_thread");
    para.stackDepth = 256*4;
    para.arg        = NULL; 
    para.tick       = 1;
    para.priority   = 4;
    status = ryos_thread_create(&ry_tool_thread_handle, ry_tool_thread, &para);
    if (RY_SUCC != status) {
        LOG_ERROR("[ry_tool_init]: Thread Init Fail.\n");
        RY_ASSERT(status == RY_SUCC);
    }
 
    return status;
}

/**
 * @brief   API to init BLE module
 *
 * @param   None
 *
 * @return  None
 */
ry_sts_t ry_tool_init(void)
{
    ry_sts_t status;
    ryos_threadPara_t para;
    u32_t sec;

    LOG_DEBUG("[ry_tool_init] Enter.\n");

    status = ryos_semaphore_create(&ry_tool_sem, 0);

    if (RY_SUCC != status) {
        LOG_ERROR("[ry_tool_init]: Semaphore Create Error.\r\n");
        while(1);
    }

    uart_gpio_mode_init();

    //r_xfer_instance_add(R_XFER_INST_ID_UART, ry_tool_uart_send, ry_tool_onReliableReceived, (void*)sec);

    status = ry_tool_thread_init();

    return status;
}

#endif /* (RY_TOOL_ENABLE == TRUE) */



