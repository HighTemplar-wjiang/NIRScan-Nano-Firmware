/*
 * Handles TIVA device and error status
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "NNOStatusDefs.h"
#include "common.h"
#include "nnoStatus.h"

static NNO_status_struct nnoStatus;

void nnoStatus_init()
/**
 * Initializes Device and Error Status module
 */
{
	nnoStatus.deviceStatus = 0;
	nnoStatus_resetErrorStatus();

	return;
}

int nnoStatus_getNanoStatus(NNO_status_struct *status)
/**
 * Returns the complete NNO_status_struct in the pointer passed.
 *
 * @param[out] status Pointer to NNO_status_struct
 *
 * @return PASS or FAIL
 */
{
	if (NULL == status)
		return (FAIL);

	memcpy(status,&nnoStatus,sizeof(NNO_status_struct));

	return (PASS);
}

int nnoStatus_getDeviceStatus(uint32_t *status)
/**
 * Returns the 32-bit device status in the pointer passed.
 *
 * @param[out] status Pointer to device status word
 *
 * @return PASS or FAIL
 */
{
	if (status == NULL)
		return (FAIL);

	memcpy(status,&nnoStatus.deviceStatus,sizeof(uint32_t));

	return (PASS);
}

int nnoStatus_getErrorStatus(NNO_error_status_struct *status)
/**
 * Returns the  NNO_error_status_struct in the pointer passed.
 *
 * @param[out] status Pointer to NNO_error_status_struct
 *
 * @return PASS or FAIL
 */
{
	if (status == NULL)
		return (FAIL);

	memcpy(status,&nnoStatus.errorStatus,sizeof(NNO_error_status_struct));

	return (PASS);
}

int16_t nnoStatus_getErrorCode(uint8_t type)
/**
 * Returns the error code corresponding to the given type.
 *
 * @param[in] type Module specifier whose error code is to be returned
 *
 * @return error code that specifies the kind of error recorded against the given module.
 */
{
	int16_t code = 0;

	if (type >= NNO_error_code_max)
		return (0xff);
	else
	{
		switch (type)
		{
		case NNO_error_code_scan:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.scan;
			break;
		case NNO_error_code_adc:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.adc;
			break;
		case NNO_error_code_sd:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.sd;
			break;
		case NNO_error_code_eeprom:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.eeprom;
			break;
		case NNO_error_code_ble:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.ble;
			break;
		case NNO_error_code_spec_lib:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.spec_lib;
			break;
		case NNO_error_code_battery:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.battery;
			break;
		case NNO_error_code_memory:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.memory;
			break;
		case NNO_error_code_hw:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.hw;
			break;
		case NNO_error_code_tmp:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.tmp;
			break;
		case NNO_error_code_hdc:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.hdc;
			break;
		case NNO_error_code_uart:
			code = (int16_t)nnoStatus.errorStatus.errorCodes.uart;
			break;
		default:
			code = -1;
			break;
		}
	}

	return code;
}

bool nnoStatus_getIndDeviceStatus(uint32_t field)
/**
 * Returns whether the specified individual device status bit is set or reset
 *
 * @param[in] field Device status of interest (as defined in NNOStatusDefs.h)
 *
 * @return true if the specified status bit is set; false if reset
 */
{
	if (field > NNO_STATUS_MAX)
		return false;

	return ((nnoStatus.deviceStatus & field) > 0);
}

bool nnoStatus_getIndErrorStatus(uint32_t field)
/**
 * Returns whether the specified individual error status bit is set or reset
 *
 * @param[in] field Error status of interest (as defined in NNOStatusDefs.h)
 *
 * @return true if the specified status bit is set; false if reset
 */
{
	if(field > NNO_ERROR_MAX)
		return false;

	return ((nnoStatus.errorStatus.status & field) > 0);
}

int nnoStatus_setDeviceStatus(uint32_t field, bool value)
/**
 * Sets or resets the specified device status bit
 *
 * @param[in] field Device status of interest (as defined in NNOStatusDefs.h)
 * @param[in] value true sets the bit and false resets the bit
 *
 * @return PASS or FAIL
 */
{
	if (field > NNO_STATUS_MAX)
			return (FAIL);

	if (value)
		nnoStatus.deviceStatus |= field;
	else
		nnoStatus.deviceStatus &= ~field;

	return (PASS);
}

static int nnoStatus_setErrorStatus_allowOverwrite(uint32_t field, bool value, bool overwrite)
{
	if (field > NNO_ERROR_MAX)
		return (FAIL);

	if ((!overwrite) && (nnoStatus.errorStatus.status & field) > 0)	// do not overwrite error
		return (FAIL);

	if (value)
	{
		nnoStatus.errorStatus.status |= field;
	}
	else
	{
		nnoStatus.errorStatus.status &= ~field;
	}
	return (PASS);
}

int nnoStatus_setErrorStatus(uint32_t field, bool value)
/**
 * Sets or resets the specified error status bit
 *
 * @param[in] field Error status of interest (as defined in NNOStatusDefs.h)
 * @param[in] value true sets the bit and false resets the bit
 *
 * @return PASS or FAIL
 */
{
	return (nnoStatus_setErrorStatus_allowOverwrite(field, value, false));
}

static int nnoStatus_setErrorCode(uint8_t type, int16_t code)
{
	int ret = PASS;

	if (type >= NNO_error_code_max)
		return (0xff);
	else
	{
		switch (type)
		{
		case NNO_error_code_scan:
			nnoStatus.errorStatus.errorCodes.scan = (int8_t)code;
			break;
		case NNO_error_code_adc:
			nnoStatus.errorStatus.errorCodes.adc = (int8_t)code;
			break;
		case NNO_error_code_sd:
			nnoStatus.errorStatus.errorCodes.sd = (int8_t)code;
			break;
		case NNO_error_code_eeprom:
			nnoStatus.errorStatus.errorCodes.eeprom = (int8_t)code;
			break;
		case NNO_error_code_ble:
			nnoStatus.errorStatus.errorCodes.ble = code;
			break;
		case NNO_error_code_spec_lib:
			nnoStatus.errorStatus.errorCodes.spec_lib = (int8_t)code;
			break;
		case NNO_error_code_battery:
			nnoStatus.errorStatus.errorCodes.battery = (int8_t)code;
			break;
		case NNO_error_code_memory:
			nnoStatus.errorStatus.errorCodes.memory = (int8_t)code;
			break;
		case NNO_error_code_hw:
			nnoStatus.errorStatus.errorCodes.hw = (int8_t)code;
			break;
		case NNO_error_code_tmp:
			nnoStatus.errorStatus.errorCodes.tmp = (int8_t)code;
			break;
		case NNO_error_code_hdc:
			nnoStatus.errorStatus.errorCodes.hdc = (int8_t)code;
			break;
		case NNO_error_code_uart:
			nnoStatus.errorStatus.errorCodes.uart = (int8_t)code;
			break;
		default:
			ret = FAIL;
			break;
		}
	}

	return ret;
}
int nnoStatus_setErrorStatusAndCode(uint32_t error_field, bool error_value, int16_t code_value)
/**
 * Sets the error code corresponding to the given type.
 *
 * @param[in] error_field Error status of interest (as defined in NNOStatusDefs.h)
 * @param[in] error_value true sets the bit and false resets the bit
 * @param[in] code_value value to be set to indicate the type of error (as defined in NNOStatusDefs.h)
 *
 * @return PASS or FAIL
 */
{
	uint8_t idx = 0;
	uint8_t i = 1;
	int ret_val = PASS;

	if (error_field > NNO_ERROR_MAX)
		return (FAIL);

	if ((nnoStatus.errorStatus.status & error_field) > 0)	// do not overwrite error
		return (FAIL);

	// Check if error_codes are supported for error_field input
	while ((i < MAX_NUM_ERROR_STATUS) && ((error_field >> i++) > 0))
		idx++;

	if ((code_value != 0) && (idx >= NNO_error_code_max))	// not supported
		return (FAIL);

	if (error_value)
	{
		nnoStatus.errorStatus.status |= error_field;
		ret_val = nnoStatus_setErrorCode(idx, code_value);
	}
	else
	{
		nnoStatus.errorStatus.status &= ~error_field;
		ret_val = nnoStatus_setErrorCode(idx, 0);
	}

	return (ret_val);
}

int nnoStatus_clearErrorStatus(uint32_t field)
/**
 * Resets/Clears the specified error status bit. It is recommended that error status
 * be cleared once read so that the same occurance of error is not reported/read more than once.
 *
 * @param[in] field Error status of interest (as defined in NNOStatusDefs.h)
 *
 * @return PASS or FAIL
 */
{
	uint8_t i = 0;
	uint32_t temp = 0;
	int ret_val = PASS;

	if (field > NNO_ERROR_MAX)
		return (FAIL);

	for(i=0; i< NNO_MAX_ERROR_STATUS;i++)
	{
		temp = field & (1 << i);
		if (temp > 0)
		{
			if (nnoStatus_setErrorStatus_allowOverwrite(temp, false, true) < 0)
				return (FAIL);
			if(i < NNO_error_code_max)
			{
				if (nnoStatus_setErrorCode(i, 0) < 0)
					return FAIL;
			}

		}
	}
	return (ret_val);
}

void nnoStatus_resetErrorStatus()
/**
 * Resets/Clears all error status.
 */
{
	memset((uint8_t *)&nnoStatus.errorStatus,0,sizeof(NNO_error_status_struct));
	return;
}
