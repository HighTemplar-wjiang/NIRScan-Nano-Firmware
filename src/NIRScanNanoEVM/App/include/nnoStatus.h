/*
 *
 * Handles TIVA device and error status
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef NNOSTATUS_H_
#define NNOSTATUS_H_

#include "NNOStatusDefs.h"

#define NNO_INVALID_STATUS				0xffffffff

#define NNO_MAX_DEVICE_STATUS			32
#define NNO_MAX_ERROR_STATUS			32

#ifdef __cplusplus
extern "C" {
#endif

void nnoStatus_init();

// Get functions
int nnoStatus_getNanoStatus(NNO_status_struct *status);
int nnoStatus_getDeviceStatus(uint32_t *status);
int nnoStatus_getErrorStatus(NNO_error_status_struct *status);
int16_t nnoStatus_getErrorCode(uint8_t type);
bool nnoStatus_getIndDeviceStatus(uint32_t field);
bool nnoStatus_getIndErrorStatus(uint32_t field);

// Set functions
int nnoStatus_setDeviceStatus(uint32_t field, bool value);
int nnoStatus_setErrorStatus(uint32_t field, bool value);
int nnoStatus_setErrorStatusAndCode(uint32_t error_field, bool error_value,
	   	int16_t code_value);
int nnoStatus_clearErrorStatus(uint32_t field);
void nnoStatus_resetErrorStatus();

#ifdef __cplusplus
}
#endif

#endif /* NNOSTATUS_H_ */
