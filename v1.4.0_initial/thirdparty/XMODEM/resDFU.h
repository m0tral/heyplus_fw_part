/**************************************************************************//**
 * @file
 * @uart dfu
 * @author zhoudk@desay.com
 * @version 1.00
 ******************************************************************************/
#ifndef __UART_DFU_H
#define __UART_DFU_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// xmorden 
#define XMODEM_SOH                1
#define XMODEM_EOT                4
#define XMODEM_ACK                6
#define XMODEM_NAK                21
#define XMODEM_CAN                24
#define XMODEM_NCG                67

#define XMODEM_DATA_SIZE  128

// Xmoden struct 

typedef struct
{
  uint8_t header;  // SOH 
  uint8_t packetNumber;
  uint8_t packetNumberC;
  uint8_t data[XMODEM_DATA_SIZE];
  uint8_t crcHigh;
  uint8_t crcLow;
} UART_XMODEM_packet;

int UART_XMODEM_download(uint8_t data_array[]);
#endif
