/*
 * This file hosts the task to handle commands recieved through
 * USB interface
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
/* usblib Header files */
#include <usblib/usb-ids.h>
#include <usblib/usblib.h>
#include <usblib/usbhid.h>
#include <driverlib/usb.h>
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdhid.h>
#include "usbdhidcustom.h"
#include "NNOCommandDefs.h"
#include "common.h"
#include <cmdHandlerIFMgr.h>
#include "cmdDict.h"
#include "uartstdio.h"
#include "nano_timer.h"
#include "nnoStatus.h"
#include "usbCmdHandler.h"

/**
 * Message parser variables
 */

static uint16_t nWritten; /* number of message bytes written */

int16_t nRemReadPC; /* number of message bytes remaining to read */
int16_t nRemWritePC; /* number of message bytes remaining to write */

uint8_t *rdp; /* pointer to next message byte to read */
uint8_t *wrp; /* pointer to next message byte to write */

uint8_t pktState = HEADER; /* pointer to the Header of the message */

union parmUnionType parmUnionUSB; /* macro helper object. See cmdHandler.h */

/**
 * 4-byte aligned message structure. Alignment is
 * required since a pointer to this struct will be
 * passed to the USB driver
 */
static nnoMessageStruct Msg;
nnoMessageStruct *pMsg = (nnoMessageStruct*) &Msg;/*USB HID msg structure*/
static uint8_t cmdPacket[HID_MAX_PKT_SIZE];

extern Semaphore_Handle semPktRecd; /* This is defined in app.cfg */
extern tUSBDHIDCustomHidDevice NirscanNanoDevice;

/**
 * Local functions
 */

static uint16_t cmdUSBExecute(void);
static void cmdUSBRead(CMD1_TYPE type);
static void cmdUSBWrite(CMD1_TYPE type);

/**
 * Set USB as the active command connection interface
 */
void usbConn()
{
	cmdHandler_setActConnType(CONN_USB,
							  &parmUnionUSB,
							  cmdGetUSB,
							  cmdPutUSB,
							  NNO_DATA_MAX_SIZE);
}

/**
 * Disconnect USB as the active command interface
 */
void usbDisc()
{
	cmdHandler_discNotification();
}

/**
 * This functions is called by the USB event callback function in 
 * Drivers\usbhandler.c. This copies the command packet from driver space to 
 * application space and notifies uartCmdHandlerTask() via semPktRecd semaphore.
 */
int32_t cmdRecv(void *msgData, int32_t dataLen)
{
	memcpy(cmdPacket, msgData, MIN(dataLen, HID_MAX_PKT_SIZE));
	Semaphore_post(semPktRecd);
	return 0;
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
bool cmdGetUSB(int32_t nBytes, void *parm)
{

	if (nRemReadPC < nBytes) /* if no bytes to get */
	{
		DEBUG_PRINT(" nRemReadPC = %d , nBytes = %d \r\n", nRemReadPC,
				nBytes);
		return false;
	}

	memcpy(parm, rdp, nBytes); /* copy from message */
	rdp += nBytes; /* bump pointer */
	nRemReadPC -= nBytes; /* bump count */

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
bool cmdPutUSB(int32_t nBytes, void *parm)
{
	if (nRemWritePC < nBytes)
	{
		return false;
	}

	memcpy(wrp, parm, nBytes); /* copy to message */
	wrp += nBytes; /* bump pointer */
	nRemWritePC -= nBytes; /* bump count */
	nWritten += nBytes; /* bump count */

	return true; /* success */
}

/**
 * Bump the read pointer when the caller has directly read the message
 * buffer instead of using the serialized accessors
 */
bool cmdIncReadUSB(int32_t nBytes)
{
	if (nRemReadPC < nBytes) /* if no bytes to get */
		return false;

	rdp += nBytes; /* bump pointer */
	nRemReadPC -= nBytes; /* bump count */

	return true; /* success */
}

/**
 * Bump the write pointer when the caller has directly written the message
 * buffer instead of using the serialized accessors
 */
bool cmdIncWriteUSB(int32_t nBytes)
{
	if (nRemWritePC < nBytes)
	{
		return false;
	}

	wrp += nBytes; /* bump pointer */
	nRemWritePC -= nBytes; /* bump count */
	nWritten += nBytes; /* bump count */

	return true; /* success */
}

/****************************************************************************/
/* Errors in checksum or message header are handled by this function, Other */
/* errors are handled by the read/write handler or the individual command   */
/* processors.                                                              */
/*                                                                          */
/* The called message processor formats the text of the response, in the    */
/* same message buffer as the request. Local functions add the response     */
/* header/checksum and return the complete result to the caller.            */
/****************************************************************************/

static uint16_t cmdUSBExecute(void)
{
	nano_timer_increment_activity_count();	// Register activity so that inactivity monitor knows about it

	switch (pMsg->head.flags.rw)
	{
	case REQUEST:
		cmdUSBWrite(CMD1_WRITE);
		break;

	case RESPONSE:
		cmdUSBRead(CMD1_READ);
		break;

	default: /* unknown CMD1 should never occur */
		pMsg->head.length = 0; /* set length */
		pMsg->head.flags.resp = NNO_RESP_ERROR; /* set handler error */
		break;
	}

	return 4 + pMsg->head.length;
}

/****************************************************************************/
/* Handle USB read command.                                                 */
/*                                                                          */
/*    0     1 2  3  4 5   6  7  8. . .                                      */
/*  _________ _______________________                                       */
/* |flags|sq| Len |C1|C2|Text.... . .                                       */
/*  Header         Request                                                  */
/*  Header         Response                                                 */
/*                                                                          */
/*                                                                          */
/* Parse out the command key, set up the parameter parser, then vector to   */
/* the command processor.                                                   */
/*                                                                          */
/****************************************************************************/

static void cmdUSBRead(CMD1_TYPE cmd1)
{
	CMD1_TYPE cc; /* response message code */
	uint32_t key;
	int ret_val = 0;

	key = ((pMsg->payload.cmd << 8) | cmd1);

	rdp = &pMsg->payload.data[2]; /* pointer to next message byte to read */
	wrp = &pMsg->payload.data[0]; /* pointer to next message byte to write */
	nRemReadPC = pMsg->head.length - 2; /* number of read bytes */
	nRemWritePC = (uint16_t) sizeof(pMsg->payload);/* no. of write bytes */
	nWritten = 0; /* number of written bytes */

	if (cmdHandler_getActConnType() > CONN_USB)
		cc= CMD1_BUSY;
	else
	{
		if (cmdHandler_getActConnType() < CONN_USB)
		{
			ret_val = cmdHandler_setActConnType(CONN_USB,
												 &parmUnionUSB,
												 cmdGetUSB,
												 cmdPutUSB,
												 NNO_DATA_MAX_SIZE);
			if (ret_val < 0)
			{
				cc = CMD1_BUSY;
				goto _usb_read_response;
			}
		}

		cc = cmdDict_Vector(CMD1_READ_RESPONSE, key);
	}

_usb_read_response:
	if (CMD1_READ_RESPONSE == cc) /* if valid response */
	{
		pMsg->head.length = nWritten; /* set length */
		pMsg->head.flags.resp = NNO_RESP_SUCCESS; /* set ack no error */
	}
	else if (CMD1_BUSY == cc)
	{
		pMsg->head.length = 0; /* set length */
		pMsg->head.flags.resp = NNO_RESP_BUSY;
		DEBUG_PRINT("Read Message - handler busy\r\n");
	}
	else
	{
		pMsg->head.length = 0; /* set length */
		pMsg->head.flags.resp = NNO_RESP_ERROR; /* set handler error */
		DEBUG_PRINT("Read Message handler error\r\n");
	}
}

/****************************************************************************/
/* Handle USB write command.                                                 */
/*                                                                          */
/*    0     1 2  3  4 5   6  7  8. . .                                      */
/*  _________ _______________________                                       */
/* |flags|sq| Len |C1|C2|Text.... . .                                       */
/*  Header         Request                                                  */
/*  Header         Response                                                 */
/*                                                                          */
/* Parse out the command key, set up the paremeter parser, then vector to   */
/* the command processor.                                                   */
/****************************************************************************/

static void cmdUSBWrite(CMD1_TYPE cmd1)
{
	CMD1_TYPE cc; /* response message code */
	uint32_t key = ((pMsg->payload.cmd << 8) | cmd1);
	int ret_val = 0;

	rdp = &pMsg->payload.data[2]; /* pointer to next message byte to read */
	wrp = &pMsg->payload.data[0]; /* pointer to next message byte to write */
	nRemReadPC = pMsg->head.length - 2; /* number of read bytes */
	nRemWritePC = (uint16_t) sizeof(pMsg->payload); /* number of write bytes */
	nWritten = 0; /* number of written bytes */

	if (cmdHandler_getActConnType() > CONN_USB)
		cc= CMD1_BUSY;
	else
	{
		 if (cmdHandler_getActConnType() < CONN_USB)
		 {
			 ret_val = cmdHandler_setActConnType(CONN_USB,
												 &parmUnionUSB,
												 cmdGetUSB,
												 cmdPutUSB,
												 NNO_DATA_MAX_SIZE);
			 if (ret_val < 0)
			 {
				 cc = CMD1_BUSY;
				 goto _usb_write_response;
			 }
		 }

		 cc = cmdDict_Vector(CMD1_WRITE_RESPONSE, key);
	}


_usb_write_response:
	if (CMD1_WRITE_RESPONSE == cc) /* if valid response */
	{
		pMsg->head.length = nWritten; /* set length */
		pMsg->head.flags.resp = NNO_RESP_SUCCESS; /* set ack no error */
	}
	else if (CMD1_BUSY == cc)
	{
		pMsg->head.length = 0; /* set length */
		pMsg->head.flags.resp = NNO_RESP_BUSY; /* set handler error */
		DEBUG_PRINT("Write Message - handler busy\r\n");
	}
	else
	{
		pMsg->head.length = 0; /* set length */
		pMsg->head.flags.resp = NNO_RESP_ERROR; /* set handler error */
		DEBUG_PRINT("Write Message handler error\r\n");
	}
}

/****************************************************************************/
/* Spin waiting for msg packets and when complete command msg is recevied, vector to the processing function  */
/****************************************************************************/
void usbCmdHandlerTask()
{
	bool usbCmdTimedOut;
	uint32_t timeoutVal = BIOS_WAIT_FOREVER;
	static uint8_t *pCmd = NULL;
	static uint16_t bcount = 0;
	static uint16_t expected = 0;
	nnoMessageStruct *pTempMsg;

	while (true)
	{
		
		//Indefinite wait for first packet in a command and timed wait for subsequent packets if any
		usbCmdTimedOut = !Semaphore_pend(semPktRecd, timeoutVal);

		/* Start new header if previous cmd timedout and was invalid */
		if ((pktState != HEADER) && (usbCmdTimedOut == true))
		{
			pktState = HEADER;
			DEBUG_PRINT("pend on semPktRecd timedout\n");
		}

		switch (pktState)
		{
		case HEADER:
			pCmd = (uint8_t *) pMsg;
			pTempMsg = (nnoMessageStruct *) cmdPacket;
			expected = pTempMsg->head.length; /* data length expected */
			bcount = 0; /* Set byte received count */
			/* Validate the Header and the USB mode */
			if ((pTempMsg->head.flags.dest < 3) && /* check destination*/
			(INRNG(2, expected, 512)) && /* Check length  */
			(pTempMsg->head.flags.reserved == 0) && /* Reserved bits */
			(pTempMsg->head.flags.resp == 0))/*Incoming response bit */
			{
				pktState = PRJCTRLCMD;
				expected += sizeof(pTempMsg->head); /* total length expected */
				goto _pctrlCMD;
			}
			pktState = MSGINVALID;
			goto _msgINVALID;

		case PRJCTRLCMD:
			_pctrlCMD: /* Copy the packet directly into the message buffer */
			memcpy((uint8_t *) (pCmd + bcount), cmdPacket, HID_MAX_PKT_SIZE);
			bcount += HID_MAX_PKT_SIZE; /* Packets are always 64 bytes */
			if (bcount >= expected)
			{
				pktState = HEADER;
				timeoutVal = BIOS_WAIT_FOREVER;
				//We now have a full command packet; execute comamnd and send response
				cmdUSBExecute();
				if (pMsg->head.flags.reply)
				{
					USBDHIDCustomHidResponse(&NirscanNanoDevice,
							(signed char *) pMsg,
							(sizeof(pMsg->head) + pMsg->head.length));
				}
			}
			else
			{
				timeoutVal = CMD_PACKETS_TIMEOUT; // TODO: Find out exact value to use for timeout
			}
			break;
		case MSGINVALID: /* No copy of data, just dump packets */
			_msgINVALID: bcount += HID_MAX_PKT_SIZE; /* Packets are always 64 bytes */
			if (bcount >= expected)
			{
				pktState = HEADER;
				timeoutVal = BIOS_WAIT_FOREVER;
			}
			break;
		default:
			pktState = HEADER;
			timeoutVal = BIOS_WAIT_FOREVER;
			break;
		}
	}
}

