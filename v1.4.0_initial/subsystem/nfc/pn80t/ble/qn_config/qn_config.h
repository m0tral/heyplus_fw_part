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

#ifndef _QN_CONFIG_H_
#define _QN_CONFIG_H_


extern void nvds_config(void);

/*
 * BLE device name and address setting
 */
//static const uint8_t BD_address[6] = {0xaa,0x00,0x00,0xbe,0x7c,0x09};
//static const uint8_t DeviceName[8] = {'S', 'M', 'R', 'T', 'W', 'R', 'B', 'L'};

//static const uint8_t BD_address[6] = {0xba,0x01,0x19,0xbe,0x70,0x29};
//static const uint8_t DeviceName[8] = {'1', '1', '9', '1', 'r', 'c', '5', '0'};


static const uint8_t BD_address[6] = {0x01,0x19,0x01,0x05,0x70,0x39};
static const uint8_t DeviceName[13] = {'p', 'n', '8', '0', 't', 'w', 'e', 'a','r','a','b','l','e'};

#endif /* _QN_CONFIG_H_ */
