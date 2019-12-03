/*
 * Handles cmdHandler priority based on different interfaces and
 * defines common utilities across those interfaces
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "common.h"
#include "cmdHandlerIFMgr.h"

static CMD_HANDLER_IF_TYPE gConnType = NO_ACTIVE_CONN;
static union parmUnionType *pParamUnion;
static union parmUnionType paramUnion;
static cmdGetPutFunc_t cmdGetFuncPtr, cmdPutFuncPtr;
static size_t maxDataLimit = 0;

CMD_HANDLER_IF_TYPE cmdHandler_getActConnType()
/**
 * This function returns the current active command interface
 *
 * @return CMD_HANDLER_IF_TYPE
 * Active connection type if present, NO_ACTIVE_CONN otherwise
 *
 */
{
	return gConnType;
}

int cmdHandler_setActConnType(CMD_HANDLER_IF_TYPE type, union parmUnionType
	   	*parmUnion, cmdGetPutFunc_t getFunc, cmdGetPutFunc_t putFunc, size_t maxLimit)
/**
 * This function is used to set active command interface
 *
 * @param type 		- I - type of interface
 * @param parmUnion - I - pointer to parameter union
 * @param getFunc 	- I - pointer to get functions
 * @param putFunc 	- I - pointer to put function
 * @param maxLimit 	- I - max limit of number of bytes in one packet
 *
 * @return 0 = PASS
 *         <0 = FAIL
 *
 */
{
	if (type < gConnType)
		return (int)FAIL;
	else if (type > gConnType)
	{
		pParamUnion = parmUnion;
		cmdGetFuncPtr = getFunc;
		cmdPutFuncPtr = putFunc;
		gConnType = type;
	}

	maxDataLimit = maxLimit;

	return (int)PASS;
}

void cmdHandler_discNotification()
/**
 * This function is used to notify cmd interface manager when
 * the active interface has disconnected from TIVA
 *
 */
{
	gConnType = NO_ACTIVE_CONN;
	cmdGetFuncPtr = NULL;
	cmdPutFuncPtr = NULL;
	pParamUnion = NULL;
}

union parmUnionType *getParmUnionInst()
/**
 * This function returns the active interface's parameter union
 *
 * @return Pointer to parameter union
 *
 */
{
	if (pParamUnion != NULL)
		return (pParamUnion);
	else
		return (&paramUnion);
}

bool cmdGet(int32_t nBytes, void *parm)
/**
 * This function is used to read command parameters
 *
 * @param nBytes - I - size of the parameter (in bytes)
 * @param parm   - O - Pointer to parameter
 *
 * @return  0 -> SUCCESS
 *  	   <0 -> FAIL
 *
 */
{
	if ((gConnType > NO_ACTIVE_CONN) && (cmdGetFuncPtr != NULL))
		return (cmdGetFuncPtr(nBytes, parm));
	else
		return FAIL;
}

bool cmdPut(int32_t nBytes, void *parm)
/**
 * This function is used to return response from command processing
 * functions
 *
 * @param nBytes  - I - size of the parameter (in bytes)
 * @param parm    - I - Pointer to parameter
 *
 * @return  0 -> SUCCESS
 *  	   <0 -> FAIL
 *
 */
{
	if ((gConnType > NO_ACTIVE_CONN) && (cmdPutFuncPtr != NULL))
		return (cmdPutFuncPtr(nBytes, parm));
	else
		return FAIL;
}

size_t getMaxDataLimit()
/**
 * This function returns maximum data limit for the active interface
 *
 * @return  maximum data limit (per packet)
 *
 */
{
	return maxDataLimit;
}
