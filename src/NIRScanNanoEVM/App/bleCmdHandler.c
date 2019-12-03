/*
 * This file hosts the task to handle commands recieved through
 * BLE interface
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

/* XDCtools Header files */
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>

#include <usblib/usb-ids.h>
#include <usblib/usblib.h>
#include <usblib/usbhid.h>
#include <driverlib/usb.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdhid.h>

#include "usbdhidcustom.h"
#include "NNOCommandDefs.h"
#include "BLECommonDefs.h"
#include "BLEUtils.h"
#include "cmdHandlerIFMgr.h"
#include "cmdDict.h"
#include "nano_timer.h"
#include "bleCmdHandler.h"
#include "dlpspec_scan.h"

extern BLE_CMD_HANDLER_INPUT gBLECmdHandlerInput;
extern BLE_CMD_HANDLER_RESPONSE gBLECmdHandlerRepsonse;

uint8_t *rdpBLE; /* pointer to next message byte to read */
int16_t nRemReadBLE; /* number of message bytes remaining to read */
union parmUnionType parmUnionBLE;

uint8_t *wrpBLE;
uint16_t nNumWrBLE;
extern uint8_t g_dataBlob[SCAN_DATA_BLOB_SIZE];

extern Semaphore_Handle semBLECmdRecd;
extern Semaphore_Handle semBLECmdComp;

/**
 * Set BLE as the active command connection interface
 */
void bleConn()
{
	cmdHandler_setActConnType(CONN_BLE,
							  &parmUnionBLE,
							  cmdGetBLE,
							  cmdPutBLE,
							  NNO_DATA_MAX_SIZE);
}

/**
 * Disconnect BLE as the active command interface
 */
void bleDisc()
{
	cmdHandler_discNotification();
}
/**
 * Spin waiting for msg packets and when complete command msg is recevied,
 * vector to the processing function 
 */
void bleCmdHandlerTaskMain()
{
	CMD1_TYPE cType;
	CMD1_TYPE cc;

	while (true)
	{
		DEBUG_PRINT("\r\nBLE Cmd Handler Task created\r\n");

		//Indefinite wait for a command and timed wait for subsequent packets if any
		Semaphore_pend(semBLECmdRecd, BIOS_WAIT_FOREVER);

		DEBUG_PRINT("\r\nBLE Cmd Handler: Received command\r\n");

		cType = (gBLECmdHandlerInput.type == BLE_COMMAND_TYPE_READ_DELAY_RESPONSE) \
				? CMD1_READ_RESPONSE \
				: CMD1_WRITE_RESPONSE;

		nRemReadBLE = gBLECmdHandlerInput.data_len;
		rdpBLE = &gBLECmdHandlerInput.data[0];

		if (gBLECmdHandlerInput.fileType == NNO_OTHER_COMMANDS)
		{
			wrpBLE = &g_dataBlob[1];	// result will be stored in Byte 0
			nNumWrBLE = 1;
		}
		else
		{
			wrpBLE = &g_dataBlob[0];
			nNumWrBLE = 0;
		}
        gBLECmdHandlerRepsonse.subFileType = 0;

		DEBUG_PRINT("\r\nCommand received:%d\r\n",gBLECmdHandlerInput.cmd);
		cc = cmdDict_Vector(cType, gBLECmdHandlerInput.cmd);

		if (cType == cc) /* if valid response */
			gBLECmdHandlerRepsonse.result = PASS;
		else
			gBLECmdHandlerRepsonse.result = FAIL;

		DEBUG_PRINT("\r\nResult:%d\r\n", gBLECmdHandlerRepsonse.result);

		// If any data was written into write pointer and if notification was requested, then do the needful
		if ((gBLECmdHandlerInput.type == BLE_COMMAND_TYPE_WRITE_NOTIFY) &&
			(gBLECmdHandlerInput.fileType == NNO_OTHER_COMMANDS))
		{
			g_dataBlob[0] = (gBLECmdHandlerRepsonse.result == PASS) ? 0 : 1;
		}

		gBLECmdHandlerRepsonse.output_data_len = nNumWrBLE;
        gBLECmdHandlerRepsonse.output = &g_dataBlob[0];
        gBLECmdHandlerRepsonse.fileType = gBLECmdHandlerInput.fileType;

		Semaphore_post(semBLECmdComp);
	}
}

/**
 * Fetches 'nBytes' from the command packet. Value is copied to caller's
 * environment.
 *
 * @param nBytes [in] number of bytes to copy from command packet
 * @param parm [out] Pointer where the bytes are copied.
 *
 * @return true on success; false on failure.
 */
bool cmdGetBLE(int32_t nBytes, void *parm)
{
	if (nRemReadBLE < nBytes) /* if no bytes to get */
	{
		DEBUG_PRINT(" nRemReadBLE = %d , nBytes = %d \r\n",
					nRemReadBLE,
					nBytes);
		return false;
	}

	memcpy(parm, rdpBLE, nBytes); /* copy from message */
	rdpBLE += nBytes; /* bump pointer */
	nRemReadBLE -= nBytes; /* bump count */

	return true; /* success */
}

/**
 * Puts 'nBytes' into the command packet. Value is copied from caller's
 * environment to the command response message.
 *
 * @param nBytes [in] number of bytes to copy to command response packet
 * @param parm [out] Pointer where the bytes to copy are provided
 *
 * @return true on success; false on failure.
 */
bool cmdPutBLE(int32_t nBytes, void *parm)
{
	if (wrpBLE != parm)
		memcpy(wrpBLE, parm, nBytes);
	wrpBLE += nBytes;
	nNumWrBLE += nBytes;

	return TRUE;
}
#endif
