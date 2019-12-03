/*
 * This file hosts the task to handle commands recieved through
 * UART interface
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
#include "common.h"

#ifdef ENABLE_UART_COMMAND_INTERFACE
#include "NNOCommandDefs.h"
#include "NNOUARTDefs.h"
#include <cmdHandlerIFMgr.h>
#include "cmdDict.h"
#include "uartstdio.h"
#include "nano_timer.h"
#include "nnoStatus.h"
#include "uartCmdHandler.h"

extern Semaphore_Handle semUARTPktRecd; /* This is defined in app.cfg */

/**
 * Message parser variables
 */

static uint16_t nWrittenUART; /* number of message bytes written */

int16_t nRemReadUART; /* number of message bytes remaining to read */
int16_t nRemWriteUART; /* number of message bytes remaining to write */

uint8_t *rdpUART; /* pointer to next message byte to read */
uint8_t *wrpUART; /* pointer to next message byte to write */

extern int16_t  nRemReadPC;     /* number of message bytes remaining to read  */
extern int16_t  nRemWritePC;    /* number of message bytes remaining to write */

extern uint8_t *rdp;                /* pointer to next message byte to read  */
extern uint8_t *wrp;                /* pointer to next message byte to write */


union parmUnionType parmUnionUART; /* macro helper object. See cmdHandler.h */

/**
 * 4-byte aligned message structure. Alignment is
 * required since a pointer to this struct will be
 * passed to the USB driver
 */
static uint8_t uartCmdPacket[UART_MAX_CMD_MAX_PKT_SZ];
uartMessageStruct *pUartMsg = (uartMessageStruct*) uartCmdPacket;/*USB HID msg structure*/
static uint32_t pktLen = 0;

#define UART_MAX_PACKET_DATA_SIZE (NNO_DATA_MAX_SIZE - 2)	//command size

/**
 * Local functions
 */

static void cmdUARTExecute(void);
static void cmdUARTRead(CMD1_TYPE type);
static void cmdUARTWrite(CMD1_TYPE type);
static bool uartSendResp();

/**
 * This functions is called by the UART interrupt handler function in 
 * uartstdio.c. This copies the command packet from driver space to 
 * application space and notifies usbCmdHandlerTask() via semPktRecd semaphore.
 */
int32_t cmdRecvUART(void *msgData, int32_t dataLen)
{
	pktLen = MIN(dataLen, sizeof(nnoMessageStruct));
	memset(uartCmdPacket, 0, UART_MAX_CMD_MAX_PKT_SZ);
	memcpy(uartCmdPacket, msgData, pktLen);
	Semaphore_post(semUARTPktRecd);
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
bool cmdGetUART(int32_t nBytes, void *parm)
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
bool cmdPutUART(int32_t nBytes, void *parm)
{
	if (nRemWritePC < nBytes)	// Send the existing packet
	{
		return uartSendResp();
	}

	memcpy(wrp, parm, nBytes); /* copy to message */
	wrp += nBytes; /* bump pointer */
	nRemWritePC -= nBytes; /* bump count */
	nWrittenUART += nBytes; /* bump count */

	return true; /* success */
}



static void cmdUARTExecute(void)
{
	nano_timer_increment_activity_count();	// Register activity so that inactivity monitor knows about it

	switch (pUartMsg->msg.head.flags.rw)
	{
	case REQUEST:
		cmdUARTWrite(CMD1_WRITE);
		break;

	case RESPONSE:
		cmdUARTRead(CMD1_READ);
		break;

	default: /* unknown CMD1 should never occur */
		pUartMsg->msg.head.length = 0; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_ERROR; /* set handler error */
		break;
	}

	return;
}



static void cmdUARTRead(CMD1_TYPE cmd1)
{
	CMD1_TYPE cc; /* response message code */
	uint32_t key;
	int ret_val = 0;

	key = ((pUartMsg->msg.payload.cmd << 8) | cmd1);

	rdp = &pUartMsg->msg.payload.data[sizeof(pUartMsg->msg.payload.cmd)];
	wrp = &pUartMsg->msg.payload.data[sizeof(pUartMsg->msg.payload.cmd)];
	nRemReadPC = pUartMsg->msg.head.length - 2; /* number of read bytes */
	/* no. of write bytes */
	nRemWritePC = (uint16_t) (sizeof(pUartMsg->msg.payload) - sizeof(pUartMsg->msg.payload.cmd));
	nWrittenUART = sizeof(pUartMsg->msg.payload.cmd); /* number of written bytes */

	rdp = rdp;
		wrp = wrp;
		nRemReadPC = nRemReadPC;
		nRemWritePC = nRemWritePC;


	if (cmdHandler_getActConnType() > CONN_UART)
		cc= CMD1_BUSY;
	else
	{
		if (cmdHandler_getActConnType() < CONN_UART)
		{
			ret_val = cmdHandler_setActConnType(CONN_UART,
												 &parmUnionUART,
												 cmdGetUART,
												 cmdPutUART,
												 UART_MAX_PACKET_DATA_SIZE);
			if (ret_val < 0)
			{
				cc = CMD1_BUSY;
				goto _uart_read_response;
			}
		}

		cc = cmdDict_Vector(CMD1_READ_RESPONSE, key);
	}

_uart_read_response:
	if (CMD1_READ_RESPONSE == cc) /* if valid response */
	{
		pUartMsg->msg.head.length = nWrittenUART; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_SUCCESS; /* set ack no error */
	}
	else if (CMD1_BUSY == cc)
	{
		pUartMsg->msg.head.length = 0; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_BUSY;
		DEBUG_PRINT("Read Message - handler busy\r\n");
	}
	else
	{
		pUartMsg->msg.head.length = 0; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_ERROR; /* set handler error */
		DEBUG_PRINT("Read Message handler error\r\n");
	}
}

static void cmdUARTWrite(CMD1_TYPE cmd1)
{
	CMD1_TYPE cc; /* response message code */
	uint32_t key = ((pUartMsg->msg.payload.cmd << 8) | cmd1);
	int ret_val = 0;

	/* pointer to next message byte to read */
	rdp = &pUartMsg->msg.payload.data[sizeof(pUartMsg->msg.payload.cmd)];
	/* pointer to next message byte to write */
	wrp = &pUartMsg->msg.payload.data[sizeof(pUartMsg->msg.payload.cmd)];
	nRemReadPC = pUartMsg->msg.head.length - 2; /* number of read bytes */
	/* number of write bytes */
	nRemWritePC = (uint16_t) (sizeof(pUartMsg->msg.payload) - sizeof(pUartMsg->msg.payload.cmd));
	nWrittenUART = sizeof(pUartMsg->msg.payload.cmd); /* number of written bytes */

	rdp = rdp;
	wrp = wrp;
	nRemReadPC = nRemReadPC;
	nRemWritePC = nRemWritePC;

	if (cmdHandler_getActConnType() > CONN_UART)
		cc= CMD1_BUSY;
	else
	{
		 if (cmdHandler_getActConnType() < CONN_UART)
		 {
			 ret_val = cmdHandler_setActConnType(CONN_UART,
												 &parmUnionUART,
												 cmdGetUART,
												 cmdPutUART,
												 UART_MAX_PACKET_DATA_SIZE);
			 if (ret_val < 0)
			 {
				 cc = CMD1_BUSY;
				 goto _uart_write_response;
			 }
		 }

		 cc = cmdDict_Vector(CMD1_WRITE_RESPONSE, key);
	}


_uart_write_response:
	if (CMD1_WRITE_RESPONSE == cc) /* if valid response */
	{
		pUartMsg->msg.head.length = nWrittenUART; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_SUCCESS; /* set ack no error */
	}
	else if (CMD1_BUSY == cc)
	{
		pUartMsg->msg.head.length = 0; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_BUSY; /* set handler error */
		DEBUG_PRINT("Write Message - handler busy\r\n");
	}
	else
	{
		pUartMsg->msg.head.length = 0; /* set length */
		pUartMsg->msg.head.flags.resp = NNO_RESP_ERROR; /* set handler error */
		DEBUG_PRINT("Write Message handler error\r\n");
	}
}


void uartCmdHandlerTask()
{
	uint32_t timeoutVal = BIOS_WAIT_FOREVER;
	int i = 0;
	uint32_t dataChkSum = 0;
	uint8_t *pChar = NULL;

	while (true)
	{
		//Indefinite wait for first packet in a command and timed wait for subsequent packets if any
		Semaphore_pend(semUARTPktRecd, timeoutVal);

		/*
		 * Sanity check
		 * validate checksum first
		 * then check if length works out OK
		 */
		i = 0;
		dataChkSum = 0;
		pChar = (uint8_t *)&uartCmdPacket[UART_START_IND_NUM_BYTES + sizeof(pUartMsg->chkSum)];
		while (i < (pktLen - UART_START_IND_NUM_BYTES - UART_END_IND_NUM_BYTES - sizeof(pUartMsg->chkSum)))
		{
			dataChkSum += *(pChar + i);
			i++;
		}

		if (pUartMsg->chkSum != dataChkSum)
		{
			pUartMsg->msg.head.flags.resp = NNO_RESP_ERROR;
			pUartMsg->msg.head.length = sizeof(pUartMsg->msg.payload.cmd);
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_UART, true, UART_INPUT_PKT_CHECKSUM_ERROR);
			uartSendResp();
		}
		else
		{
			cmdUARTExecute();
			if (pUartMsg->msg.head.flags.reply)
			{
				uartSendResp();
			}

		}

	}
}

static bool uartSendResp()
{
	int i = 0;
	uint16_t endIndex = 0;
	uint16_t len = 0;
	uint8_t *pChar = NULL;

	/*
	 * Compute length
	 */
	pUartMsg->msg.head.length = nWrittenUART;

	/*
	 * compute checksum for payload
	 */
	pUartMsg->chkSum = 0;	// reset to start with
	pChar = (uint8_t *)&uartCmdPacket[UART_START_IND_NUM_BYTES \
									  + sizeof(pUartMsg->chkSum)];
	len = sizeof(nnoMessageStruct) - NNO_DATA_MAX_SIZE \
		  + nWrittenUART;

	while (i < len)
	{
		pUartMsg->chkSum += *(pChar + i);
		i++;
	}

	/*
	 * Add end indication bytes at appropriate position
	 */
	endIndex = sizeof(uartMessageStruct) - UART_END_IND_NUM_BYTES \
			   - NNO_DATA_MAX_SIZE \
			   + nWrittenUART;

	uartCmdPacket[endIndex++] = UART_END_IND_BYTE_0;
	uartCmdPacket[endIndex++] = UART_END_IND_BYTE_1;
	uartCmdPacket[endIndex++] = UART_END_IND_BYTE_2;
	uartCmdPacket[endIndex++] = UART_END_IND_BYTE_3;

	if (endIndex != UARTwrite((const char *)&uartCmdPacket[0], endIndex))
		return FAIL;

	if (len > 0)
		UARTFlushTx(false);

	return PASS;
}
#endif
