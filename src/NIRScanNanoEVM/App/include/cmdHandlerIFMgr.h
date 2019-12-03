/*
 * Handles cmdHandler priority based on different interfaces and
 * defines common utilities across those interfaces
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef __CMDHANDLER_H
#define __CMDHANDLER_H

/*
 * @enum Command Handler Interface Manager state
 *
 * 0 = No active connection exists
 * 1 = Active connection exists, Idle state
 * 2 = Active connection exists, command being processed
 */
typedef enum _tagIFMgrState
{
    STATE_NO_ACT_CONN,
	STATE_ACT_CONN_IDLE,
	STATE_ACT_CONN_PROC_CMD
} CMD_HANDLER_IF_MGR_STATE;

/*
 * @enum Command Handler supported connection types
 *
 * Enums are defined in increasing order of priority
 */
typedef enum connnection_type
{
	NO_ACTIVE_CONN,
	CONN_USB,
	CONN_BLE,
#ifdef ENABLE_UART_COMMAND_INTERFACE
	CONN_UART,
#endif
} CMD_HANDLER_IF_TYPE;

/**
 * Union of paramters of sizes 1/2/4 bytes
 */
union parmUnionType
{
    int8_t p1;
    int16_t p2;
    int32_t p4;
};

typedef bool (*cmdGetPutFunc_t)(int32_t nBytes, void *parm);

CMD_HANDLER_IF_TYPE cmdHandler_getActConnType();
int cmdHandler_setActConnType(CMD_HANDLER_IF_TYPE type, union parmUnionType *parmUnion, cmdGetPutFunc_t getFunc, cmdGetPutFunc_t putFunc, size_t max_packet_size);
void cmdHandler_discNotification();
union parmUnionType *getParmUnionInst();
bool cmdGet(int32_t nBytes, void *parm);
bool cmdPut(int32_t nBytes, void *parm);
size_t getMaxDataLimit();

/**
 * Command get and put functions
 */
#define cmdGet1( parmtype ) ( cmdGet( 1, &(getParmUnionInst()->p1)), (parmtype)(getParmUnionInst()->p1))
#define cmdGet2( parmtype ) ( cmdGet( 2, &(getParmUnionInst()->p2)), (parmtype)(getParmUnionInst()->p2))
#define cmdGet4( parmtype ) ( cmdGet( 4, &(getParmUnionInst()->p4)), (parmtype)(getParmUnionInst()->p4))

#define cmdPutK1( parm ) ( getParmUnionInst()->p1 = parm , cmdPut( 1, &(getParmUnionInst()->p1)))
#define cmdPutK2( parm ) ( getParmUnionInst()->p2 = parm , cmdPut( 2, &(getParmUnionInst()->p2)))
#define cmdPutK4( parm ) ( getParmUnionInst()->p4 = parm , cmdPut( 4, &(getParmUnionInst()->p4)))

#define cmdPut1( parm ) ( getParmUnionInst()->p1 = parm , cmdPut( 1, &(getParmUnionInst()->p1)))
#define cmdPut2( parm ) ( getParmUnionInst()->p2 = parm , cmdPut( 2, &(getParmUnionInst()->p2)))
#define cmdPut4( parm ) ( getParmUnionInst()->p4 = parm , cmdPut( 4, &(getParmUnionInst()->p4)))

#endif
