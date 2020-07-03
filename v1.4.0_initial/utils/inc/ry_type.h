/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __RY_TYPE_H__
#define __RY_TYPE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


typedef unsigned char u8_t ;
typedef signed   char s8_t;

typedef unsigned short u16_t;
typedef signed   short s16_t;

typedef unsigned int u32_t;
typedef signed   int s32_t;

typedef unsigned long long u64_t;
typedef signed   long long s64_t;

typedef unsigned char u8 ;
typedef signed   char s8;

typedef unsigned short u16;
typedef signed   short s16;

typedef unsigned int u32;
typedef signed   int s32;

typedef unsigned long long u64;
typedef signed   long long s64;


#ifndef NULL
#define NULL 	0
#endif

#ifndef __cplusplus


#ifndef FALSE
#define FALSE 	0
#endif
#ifndef TRUE
#define TRUE 	(!FALSE)
#endif

#ifndef false
#define false 	FALSE
#endif

#ifndef true
#define true 	TRUE
#endif

#ifndef bool
#define bool    int
#endif

#endif

// There is no way to directly recognise whether a typedef is defined
// http://stackoverflow.com/questions/3517174/how-to-check-if-a-datatype-is-defined-with-typedef

#define U32_MAX ((u32)0xffffffff)
#define U16_MAX ((u16)0xffff)
#define U8_MAX ((u8)0xff)
#define U31_MAX ((u32)0x7fffffff)
#define U15_MAX ((u16)0x7fff)
#define U7_MAX ((u8)0x7f)


#ifdef WIN32
#   ifndef FALSE
#        define FALSE 0
#    endif

#   ifndef TRUE
#        define TRUE 1
#   endif
#endif

//#define SUCCESS                                0x00
//#define FAILURE                                0x01

typedef u32_t UTCTime;
typedef u32_t arg_t;
//typedef u32_t status_t;

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)


/*******************************Error Code Related*****************************************/

#define RY_SUCC                               0x00

/*
 * Definition for RY Component ID
 */ 
#define RY_CID_NONE                           0x00
#define RY_CID_OS                             0x01
#define RY_CID_MODULE                         0x02
#define RY_CID_FP                             0x03
#define RY_CID_SE                             0x04
#define RY_CID_GUI                            0x05
#define RY_CID_GSENSOR                        0x06
#define RY_CID_GUP                            0x07
#define RY_CID_APP                            0x08
#define RY_CID_HAL_FLASH                      0x09
#define RY_CID_I2C                            0x0A
#define RY_CID_SPI                            0x0B
#define RY_CID_PROTOC						  0x0C
#define RY_CID_BLE                            0x0D
#define RY_CID_R_XFER                         0x0E
#define RY_CID_PB                             0x0F
#define RY_CID_TBL                            0x10
#define RY_CID_SCHED                          0x11
#define RY_CID_WMS                            0x12
#define RY_CID_MSG                            0x13
#define RY_CID_CMS                            0x14
#define RY_CID_ACTIVITY_MENU                  0x15
#define RY_CID_TMS                            0x16
#define RY_CID_AMS                            0x17
#define RY_CID_SURFACE                        0x18
#define RY_CID_AES                            0x19
#define RY_CID_OTA                            0x1A
#define RY_CID_MIJIA                          0x1B



/*
 * Definition for RY Component ID
 */ 
#define RY_ERR_INVALIC_PARA                   0x01
#define RY_ERR_NO_MEM                         0x02
#define RY_ERR_TIMEOUT                        0x03
#define RY_ERR_INVALID_TYPE                   0x04
#define RY_ERR_INVALID_CMD                    0x05
#define RY_ERR_INVALID_EVT                    0x06
#define RY_ERR_INVALID_FCS                    0x07
#define RY_ERR_BUSY                           0x08
#define RY_ERR_ERASE                          0x09
#define RY_ERR_WRITE                          0x0A
#define RY_ERR_READ                           0x0B
#define RY_ERR_INIT_FAIL                      0x0C
#define RY_ERR_TEST_FAIL                      0x0D
#define RY_ERR_INIT_DEFAULT                   0x0F
#define RY_ERR_TABLE_FULL                     0x10
#define RY_ERR_TABLE_EXISTED                  0x11
#define RY_ERR_TABLE_NOT_FOUND                0x12
#define RY_ERR_PARA_RESTORE                   0x13
#define RY_ERR_PARA_SAVE                      0x14
#define RY_ERR_QUEUE_FULL                     0x15
#define RY_ERR_INVALID_STATE                  0x16
#define RY_ERR_OPEN                           0x17
#define RY_ERR_ENCRYPTION                     0x18
#define RY_ERR_DECRYPTION                     0x19
#define RY_ERR_TIMESTAMP                      0x20
#define RY_ERR_NOT_CONNECTED                  0x21

/*
 * Marco to build and parse Error code
 */ 
#define RY_SET_STS_VAL(cid, status)           (((u16_t)cid)<<8 | (status))
#define RY_GET_CID_FROM_ERR(e)                (((e)&0xff00)>>8)
#define RY_GET_STS_FROM_ERR(e)                ((e)&0x00ff)

/*
 * Marco to build and parse Error code
 */ 
typedef u32_t ry_sts_t;


#endif  /* __RY_TYPE_H__ */

