/*
 * This file hosts the task to handle commands recieved through
 * BLE interface
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef __BLECMDHANDLER_H
#define __BLECMDHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_CMD_HANDLER_TASK_STACK_SIZE 8192
#define BLE_CMD_HANDLER_TASK_PRIORITY	 9

void bleCmdHandlerTaskMain();

void bleConn();
void bleDisc();

bool cmdGetBLE( int32_t nBytes, void *parm );
bool cmdPutBLE( int32_t nBytes, void *parm );

#ifdef __cplusplus
}
#endif

#endif //__BLECMDHANDLER_H
