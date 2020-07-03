#ifndef _FP_HANDLER_H_
#define _FP_HANDLER_H_

#include "debug_settings.h"
#include "p61_data_types.h"
#include <stdbool.h>


/**
 *  \brief Status used by the FP Integration APIs.
 *
 * */
typedef UINT8 tFP_STATUS;
#define STATUS_OK						0x00     ///< Command succeeded
#define STATUS_INVALID_PARAM			0x01     ///< invalid parameter provided
#define STATUS_INVALID_LENGTH			0x02     ///< invalid Length provided
#define STATUS_FAILED					0x03     ///< failed
#define STATUS_ALREADY_INITIALIZED		0x04 ///< API called twice
#define STATUS_ALREADY_DEINITIALIZED	0x05 ///< API called twice
#define STATUS_INVALID_OPERATION		0x06 ///< Invalid response operation
#define STATUS_MATCHING_FAILED			0x07 ///< Matching was not successful
#define STATUS_ENROLLMENT_FAILED		0x08 ///< Enrollment was not successful
#define STATUS_DELETE_ENR_FP_FAILED  	0x09 ///< Deletion of enrollment finger was not successful
#define STATUS_EXTRACTION_FAILED	  	0x0A ///< Image extraction failed on MCU2


tFP_STATUS phNfcFp_PerformEnrollment(void);


#endif