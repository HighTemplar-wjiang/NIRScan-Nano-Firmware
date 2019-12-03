/*
 * This file hosts the task to handle commands recieved through
 * UART interface
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef __UARTCMDHANDLER_H
#define __UARTCMDHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/* Message parsing functions and macros.                                    */
/****************************************************************************/
#define UART_TASK_STACK_SIZE 5120
#define UART_TASK_PRIORITY	 9
/*
 * Time between two bytes of a UART command in microseconds
 *
 * TIVA would timeout if subsequent packets aren't received within this time period
 * flush all recieved packets and send a Nack back to sender
 */
#define MAX_TIME_BET_DATA_PACKETS_US	1000

void uartCmdHandlerTask();
int32_t cmdRecvUART(void *msgData, int32_t dataLen);

#ifdef __cplusplus
}
#endif


#endif // __USBCMDHANDLER_H
