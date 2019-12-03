/*
 * Common BLE utility header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEUTILS_H_
#define BLEUTILS_H_

#include "BTBTypes.h"
#include "uartstdio.h"

/** @name BLE application error codes
 *
 * These are addtional error codes besides ones defined in
 * Bluetopia stack
 */
//@{
/**			Invalid input parameter 						*/
#define BLE_ERROR_INVALID_PARAMETER                      (-1000)
/**			Invalid bluetooth stack instance ID				*/
#define BLE_ERROR_INVALID_BLUETOOTH_STACK_ID             (-1001)
/**			Insufficent resources (memory etc) 				*/
#define BLE_ERROR_INSUFFICIENT_RESOURCES                 (-1002)
/**			Service exists already 							*/
#define BLE_ERROR_SERVICE_ALREADY_REGISTERED             (-1003)
/**			Invalid service instance ID						*/
#define BLE_ERROR_INVALID_INSTANCE_ID                    (-1004)
/**			Incorrect data format 							*/
#define BLE_ERROR_MALFORMATTED_DATA                      (-1005)
/**			Max app instances reached						*/
#define BLE_ERROR_MAXIMUM_NUMBER_OF_INSTANCES_REACHED    (-1006)
/**			Error sending notification 						*/
#define BLE_ERROR_NOTIFY_ERROR							 (-1007)
/**			Errors not defined above						*/
#define BLE_ERROR_UNKNOWN_ERROR                          (-1008)
#define MTU_PACKET_HEADER_SIZE 3
//@}

/**
 * @brief Global variable to store MTU size supported by the connected client
 */
extern unsigned short gBLESuppMTUSize;

/**
 * @brief MTU packet header size
 */
#define MTU_PACKET_HEADER_SIZE 3

/**
 * @brief used to hold information on a connected LE Device
 */
typedef struct _tagLE_Context_Info_t
{
	BD_ADDR_t           ConnectionBD_ADDR;
	unsigned int        ConnectionID;
}  LE_Context_Info_t;

/**
 * Utility functions for some common BLE functionalities
 */
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Function to decode client configuration descriptor
 */
int BLEUtil_DecodeCharConfigDesc(unsigned int BufferLength, Byte_t *Buffer, Boolean_t *Notify);

/**
 * @brief Function to send notification
 */
int BLEUtil_SendNotification(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int ConnectionID, Word_t offset, Word_t size, Byte_t *data);

/**
 * @brief Function to send read response
 */
int BLEUtil_SendReadResponse(unsigned int BluetoothStackID, unsigned int TransactionID, unsigned int DataLength, Byte_t *Data);

/**
 * @brief Function to send indication to client
 */
int BLEUtil_SendIndication(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int ConnectionID,Word_t offset, Word_t size, Byte_t *data);

/**
 * @brief Function to send write repsonse back to client
 */
int BLEUtil_SendWriteResponse(unsigned int BluetoothStackID, unsigned int TransactionID);

/**
 * @brief Function to find LE index using Bluetooth device address
 */
int BLEUtil_FindLEIndexByAddress(BD_ADDR_t BD_ADDR);

/**
 * BLE heap memory management functions
 */
void *bleMalloc(size_t size);
void bleFree(void *mem, size_t size);

/**
 * BLE - Logging related functions
 */
void bleFlushLog();
#define bleLog(...) UARTprintf(__VA_ARGS__); \
					bleFlushLog();
void bleLogFuncError(char *Function,int Status);

#ifdef __cplusplus
}
#endif
#endif /* BLEUTILS_H_ */
