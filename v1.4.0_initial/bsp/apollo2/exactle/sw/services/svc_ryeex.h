/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __SVC_RYEEX_H__
#define __SVC_RYEEX_H__

//
// Put additional includes here if necessary.
//

#ifdef __cplusplus
extern "C"
{
#endif

//*****************************************************************************
//
// Macro definitions
//
//*****************************************************************************

/*! Base UUID:  00002760-08C2-11E1-9073-0E8AC72EXXXX */
#define ATT_UUID_RYEEX_BASE            0x2E, 0xC7, 0x8A, 0x0E, 0x73, 0x90, \
                                             0xE1, 0x11, 0xC2, 0x08, 0x60, 0x27, 0x00, 0x00 

/*! Macro for building Ambiq UUIDs */
#define ATT_UUID_RYEEX_BUILD(part)     UINT16_TO_BYTES(part), ATT_UUID_RYEEX_BASE

/*! Partial Ryeex service UUIDs */
#define ATT_UUID_RYEEX_SERVICE         0xB167

/*! Partial Ryeex Characteristic UUIDs for Secure Channel */
#define ATT_UUID_RYEEX_SECURE_CH       0xAA00

/*! Partial Ryeex Characteristic UUIDs for Unsecure Channel */
#define ATT_UUID_RYEEX_UNSECURE_CH     0xAA01

/*! Partial Ryeex Characteristic UUIDs for Json Channel */
#define ATT_UUID_RYEEX_JSON_CH         0xBA01

/*! Max length of a ryeex characteristic data in one packet. */
#define RYEEXS_CHAR_MAX_DATA           244 // 152 

// Program Size: Code=497756 RO-data=295104 RW-data=8024 ZI-data=253592  152
// Program Size: Code=497756 RO-data=295104 RW-data=8024 ZI-data=253872  244

/* Ryeex Service Handle scope */
#define RYEEXS_START_HDL               0x100
#define RYEEXS_END_HDL                 (RYEEXS_MAX_HDL - 1)


/* Ryeex Service Handles */
enum
{
  RYEEX_SVC_HDL = RYEEXS_START_HDL,     /* Ryeex service declaration */
  RYEEXS_SECURE_CH_DEF_HDL,             /* Ryeex Secure channel characteristic declaration */ 
  RYEEXS_SECURE_CH_VAL_HDL,             /* Ryeex Secure channel characteristic data */ 
  RYEEXS_SECURE_CH_CCC_HDL,             /* Ryeex Secure channel client characteristic configuration */ 
  RYEEXS_UNSECURE_CH_DEF_HDL,           /* Ryeex Unsecure channel characteristic declaration */ 
  RYEEXS_UNSECURE_CH_VAL_HDL,           /* Ryeex Unsecure channel characteristic data */
  RYEEXS_UNSECURE_CH_CCC_HDL,           /* Ryeex Unsecure channel client characteristic configuration */
  RYEEXS_JSON_CH_DEF_HDL,               /* Ryeex Json channel characteristic declaration */ 
  RYEEXS_JSON_CH_VAL_HDL,               /* Ryeex Json channel characteristic data */
  RYEEXS_JSON_CH_CCC_HDL,               /* Ryeex Json channel client characteristic configuration */
  RYEEXS_MAX_HDL
};


//*****************************************************************************
//
// External variable definitions
//
//*****************************************************************************
//extern uint32_t g_ui32Stuff;

//*****************************************************************************
//
// Function definitions.
//
//*****************************************************************************
void SvcRyeexsAddGroup(void);
void SvcRyeexsRemoveGroup(void);
void SvcRyeexsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

#ifdef __cplusplus
}
#endif


#endif  /* __SVC_RYEEX_H__ */
