/*
* Copyright (c), RY Inc.
*
* All rights are reserved. Reproduction in whole or in part is
* prohibited without the written consent of the copyright owner.
*
*/
#ifndef __SVC_MI_H__
#define __SVC_MI_H__

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
#define ATT_UUID_MI_BASE            0x2E, 0xC7, 0x8A, 0x0E, 0x73, 0x90, \
                                             0xE1, 0x11, 0xC2, 0x08, 0x60, 0x27, 0x00, 0x00 

/*! Macro for building Mi UUIDs */
#define ATT_UUID_MI_BUILD(part)     UINT16_TO_BYTES(part), ATT_UUID_RYEEX_BASE

/*! Partial Mi service UUIDs */
#define ATT_UUID_MI_SERVICE            0xFE95

/*! Partial Mi Service Characteristic UUIDs for Secure Channel */
#define ATT_UUID_MI_TOKEN              0x0001
#define ATT_UUID_MI_PRODUCT_ID         0x0002
#define ATT_UUID_MI_VER                0x0004
#define ATT_UUID_MI_AUTH               0x0010
#define ATT_UUID_MI_SN                 0x0013
#define ATT_UUID_MI_BEACONKEY          0x0014

/*! Max length of a ryeex characteristic data in one packet. */
#define MIS_CHAR_MAX_DATA               20

/* Ryeex Service Handle scope */
#define MIS_START_HDL                  0x200
#define MIS_END_HDL                    (MIS_MAX_HDL - 1)


/* Mi Service Handles */
enum
{
  MI_SVC_HDL = MIS_START_HDL,           /* Mi service declaration */
  MIS_TOKEN_DEF_HDL,                    /* Mi Service token characteristic declaration */ 
  MIS_TOKEN_VAL_HDL,                    /* Mi Service token characteristic data */ 
  MIS_TOKEN_CCC_HDL,                    /* Mi Service token client characteristic configuration */ 
  MIS_PRODUCT_ID_DEF_HDL,               /* Mi Service product_id characteristic declaration */ 
  MIS_PRODUCT_ID_VAL_HDL,               /* Mi Service product_id characteristic data */
  MIS_VER_DEF_HDL,                      /* Mi Service version characteristic declaration */
  MIS_VER_VAL_HDL,                      /* Mi Service version characteristic data */
  MIS_AUTH_DEF_HDL,                     /* Mi Service authentication characteristic declaration */ 
  MIS_AUTH_VAL_HDL,                     /* Mi Service authentication characteristic data */ 
  MIS_AUTH_CCC_HDL,                     /* Mi Service authentication client characteristic configuration */
  MIS_SN_DEF_HDL,                       /* Mi Service SN characteristic declaration */ 
  MIS_SN_VAL_HDL,                       /* Mi Service SN characteristic data */
  MIS_BEACON_KEY_DEF_HDL,               /* Mi Service beaconkey characteristic declaration */ 
  MIS_BEACON_KEY_VAL_HDL,               /* Mi Service beaconkey characteristic data */
  MIS_MAX_HDL
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
void SvcMisAddGroup(void);
void SvcMisRemoveGroup(void);
void SvcMisCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);
void SvcMisSetSn(uint8_t* sn, int len);
void SvcMisSetVer(uint8_t* ver, int len);


#ifdef __cplusplus
}
#endif


#endif  /* __SVC_MI_H__ */
