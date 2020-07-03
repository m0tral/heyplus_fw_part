/**************************************************************************//**
 * @file
 * @uart dfu
 * @author zhoudk@desay.com
 * @version 1.00
 ******************************************************************************/
#include "resDFU.h"
#include "ryos.h"
#include "ry_type.h"
#include "ry_hal_uart.h"
#include "crc32.h"
#include "ry_hal_spi_flash.h"


static  uint32_t      sequenceNumber = 1;
static  uint32_t      store_number=1;
static  UART_XMODEM_packet pkt; 
static uint8_t array_valid[133];



static int VerifyPacketChecksum(UART_XMODEM_packet pkt)
{
  uint16_t packetCRC;
  uint16_t calculatedCRC;

  /* Check the packet number integrity */
  if (pkt.packetNumber + pkt.packetNumberC != 255)
  {
    return -1;
  }

  calculatedCRC = calc_crc16(pkt.data, XMODEM_DATA_SIZE);
  packetCRC     = pkt.crcHigh << 8 | pkt.crcLow;

  /* Check the CRC value */
  if (calculatedCRC != packetCRC)
  {
    return -1;
  }
  return 0;
}

int UART_XMODEM_download(uint8_t data_array[]){
    memset(pkt.data,0,128);	
    pkt.header = data_array[0];
    pkt.packetNumber= data_array[1];
    pkt.packetNumberC= data_array[2];

    memcpy(pkt.data,data_array+3,128);

    pkt.crcHigh= data_array[131];
    pkt.crcLow= data_array[132];

    u8_t senddata = 0;

    if (pkt.header != XMODEM_SOH){
    
        ryos_delay(100);
        senddata = XMODEM_NCG;
        ry_hal_uart_tx(UART_IDX_TOOL, &senddata, 1);
    }
		
    if (VerifyPacketChecksum(pkt) != 0){

        ryos_delay(100);
        senddata = (XMODEM_NAK);
        ry_hal_uart_tx(UART_IDX_TOOL, &senddata, 1);
    }
		    
    ry_hal_spi_flash_write(pkt.data, (store_number - 1) * XMODEM_DATA_SIZE, XMODEM_DATA_SIZE);//Ð´Êý¾Ýµ½flash
    store_number=sequenceNumber++;
    senddata = (XMODEM_ACK);

    ry_hal_uart_tx(UART_IDX_TOOL, &senddata, 1);
    return 0;
}
