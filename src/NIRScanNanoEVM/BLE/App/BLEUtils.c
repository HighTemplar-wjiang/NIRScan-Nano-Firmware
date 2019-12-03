/*
 *
 * Common BLE utility function definitions
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdio.h>               /* Included for sscanf.                     */
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include <xdc/cfg/global.h>
#include "BTTypes.h"
#include "SS1BTGAT.h"        /* Bluetooth Stack GATT API Prototypes/Constants.*/
#include "uartstdio.h"
#include "BLECommonDefs.h"
#include "nnoStatus.h"
#include "BLEUtils.h"

/**
 * @brief Nums log function calls to wait before flushing the UART logs
 */
#define NUM_LOG_TO_SKIP_BEFORE_FLUSH 10

/**
 * @brief Holds total allocated memory using bleMalloc
 */
int total_alloc = 0;

/**
 * @brief counter for number of calls to log function
 */
static int log_count = 0;

/**
 * @brief LE context info array
 */
extern LE_Context_Info_t   LEContextInfo[MAX_LE_CONNECTIONS];

int BLEUtil_DecodeCharConfigDesc(unsigned int BufferLength, Byte_t *Buffer, Boolean_t *Notify)
/**
 * Utility function that exists to decode a Client Configuration value
 * into a user specified boolean value
 *
 * @param[in]   BufferLength	Length of buffer
 * @param[in]	Buffer			Pointer to input buffer
 * @param[out]	Notify			Pointer to output buffer
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = BLE_ERROR_MALFORMATTED_DATA;
	Word_t ClientConfiguration;

	/* Verify that the input parameters are valid.                       */
	if (((BufferLength == NON_ALIGNED_BYTE_SIZE) || (BufferLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)) && (Buffer) && (Notify))
	{
		/* Read the requested Client Configuration.                       */
		if (BufferLength == NON_ALIGNED_BYTE_SIZE)
			ClientConfiguration = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(Buffer);
		else
			ClientConfiguration = READ_UNALIGNED_WORD_LITTLE_ENDIAN(Buffer);

		if ((ClientConfiguration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) ||
			(ClientConfiguration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_INDICATE_ENABLE))
		{
			*Notify = TRUE;
			ret_val = 0;
		}
		else
		{
			if (!ClientConfiguration)
			{
				*Notify = FALSE;
				ret_val = 0;
			}
		}
	}
	else
	{
		if (BufferLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)
			ret_val = BLE_ERROR_INVALID_PARAMETER;
	}

	/* Finally return the result to the caller.                          */
	return (ret_val);
}

int BLEUtil_SendNotification(unsigned int BluetoothStackID, unsigned int ServiceID, unsigned int ConnectionID, Word_t offset, Word_t size, Byte_t *data)
/**
 * Responsible for sending notification to a specified remote device
 *
 * @param[in]   BluetoothStackID	Bluetooth stack instance ID
 * @param[in]	ServiceID			Service ID to send notification to
 * @param[in]	ConnectionID		Client connection ID to send the notification to
 * @param[in]	offset				Characteristic offset in service attribute table
 * @param[in]	size				size of data to be sent
 * @param[in]	data				Data to be sent
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val;

	if ((BluetoothStackID) && (ServiceID) && (ConnectionID))
	{
		if(!BSC_LockBluetoothStack(BluetoothStackID))
		{
			/* Send the notification.                                      */
			ret_val = GATT_Handle_Value_Notification(BluetoothStackID,
													 ServiceID,
													 ConnectionID,
													 offset,
													 size,
													 data);

			if (ret_val == size)
				ret_val = 0;
			else
			{
				if (ret_val >= 0)
					ret_val = BLE_ERROR_NOTIFY_ERROR;
			}

			/* UnLock the previously locked Bluetooth Stack.               */
			BSC_UnLockBluetoothStack(BluetoothStackID);
		}
		else
			ret_val = BLE_ERROR_INSUFFICIENT_RESOURCES;
	}
	else
		ret_val = BLE_ERROR_INVALID_PARAMETER;

	/* Finally return the result to the caller.                          */
	return (ret_val);
}

int BLEUtil_SendIndication(unsigned int BluetoothStackID, unsigned int ServiceID, unsigned int ConnectionID, Word_t offset, Word_t size, Byte_t *data)
/**
 * Responsible for sending indication to a specified remote device
 *
 * @param[in]   BluetoothStackID	Bluetooth stack instance ID
 * @param[in]	ServiceID			Service ID to send indication to
 * @param[in]	ConnectionID		Client connection ID to send the indication to
 * @param[in]	offset				Characteristic offset in service attribute table
 * @param[in]	size				size of data to be sent
 * @param[in]	data				Data to be sent
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val;

	if ((BluetoothStackID) && (ServiceID) && (ConnectionID))
	{
		if(!BSC_LockBluetoothStack(BluetoothStackID))
		{
			// Send the notification
			ret_val = GATT_Handle_Value_Indication(BluetoothStackID,
												   ServiceID,
												   ConnectionID,
												   offset,
												   size,
												   data);

			/* UnLock the previously locked Bluetooth Stack.               */
			BSC_UnLockBluetoothStack(BluetoothStackID);
		}
		else
			ret_val = BLE_ERROR_INSUFFICIENT_RESOURCES;
	}
	else
		ret_val = BLE_ERROR_INVALID_PARAMETER;

	/* Finally return the result to the caller.                          */
	return (ret_val);
}

int BLEUtil_SendReadResponse(unsigned int BluetoothStackID, unsigned int TransactionID, unsigned int DataLength, Byte_t *Data)
/**
 * Responsible for sending read response to a specified remote device
 *
 * @param[in]   BluetoothStackID	Bluetooth stack instance ID
 * @param[in]	TransactionID		Read transaction ID that the response is being sent for
 * @param[in]	DataLength			size of data to be sent
 * @param[in]	Data				Data to be sent
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;

	if ((BluetoothStackID) && (TransactionID))
		if(!BSC_LockBluetoothStack(BluetoothStackID))
		{
			ret_val = GATT_Read_Response(BluetoothStackID, TransactionID, DataLength, Data);

			BSC_UnLockBluetoothStack(BluetoothStackID);
		}
	else
		ret_val = BLE_ERROR_NOTIFY_ERROR;

	return (ret_val);
}

int BLEUtil_SendWriteResponse(unsigned int BluetoothStackID, unsigned int TransactionID)
/**
 * Responsible for sending write response to a specified remote device
 *
 * @param[in]   BluetoothStackID	Bluetooth stack instance ID
 * @param[in]	TransactionID		Write transaction ID for which the response is sent for
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;

	if ((BluetoothStackID) && (TransactionID))
		if(!BSC_LockBluetoothStack(BluetoothStackID))
		{
			ret_val = GATT_Write_Response(BluetoothStackID, TransactionID);

			BSC_UnLockBluetoothStack(BluetoothStackID);
		}
	else
		ret_val = BLE_ERROR_NOTIFY_ERROR;

	return (ret_val);
}

void *bleMalloc(size_t size)
/**
 * BLE app memory allocation wrapper function
 *
 * @param[in]   size	Size of memory to be allocated
 *
 * @return      None
 *
 */
{
	void *temp = NULL;

	temp = (void *)malloc(size);
	if (temp != NULL)
		total_alloc += size;
	else
		nnoStatus_setErrorStatus(NNO_ERROR_INSUFFICIENT_MEMORY, true);

	DEBUG_PRINT("\r\n*************Total memory used:%d\r\n", total_alloc);

	return (temp);
}

void bleFree(void *mem, size_t size)
/**
 * BLE app memory free wrapper function
 *
 * @param[in]   mem		Pointer to be freed
 * @param[in]   size	size of data to be freed
 *
 * @return      None
 *
 */
{
	free(mem);

	total_alloc -= size;

   DEBUG_PRINT("\r\n*************Total memory used:%d\r\n", total_alloc);
}


void bleFlushLog()
/**
 * Function to flush UART Tx data
 *
 * @return      None
 *
 */
{
	if (log_count < NUM_LOG_TO_SKIP_BEFORE_FLUSH)
		log_count++;
	else
	{
		log_count = 0;
		UARTFlushTx(false);
	}

}
void bleLogFuncError(char *Function,int Status)
/**
 * Utility function to log errors
 *
 * @param[in]   Function	pointer to function name
 * @param[in]	Status		Status to be logged
 *
 * @return      None
 *
 */
{
	DEBUG_PRINT("%s Failed: %d.\r\n", Function, Status);
}

int BLEUtil_FindLEIndexByAddress(BD_ADDR_t BD_ADDR)
/**
 * function is responsible for iterating through the array
 * BDInfoArray[MAX_LE_CONNECTIONS], which contains the connection
 * information for connected LE devices
 *
 * @param[in]   BD_ADDR		Address to remote device
 *
 * @return      Index if found, -1 it not found
 *
 */{
	int i;
	int ret_val = -1;

	for(i=0; i<MAX_LE_CONNECTIONS; i++)
	{
	   if(COMPARE_BD_ADDR(BD_ADDR, LEContextInfo[i].ConnectionBD_ADDR))
	   {
		  ret_val = i;
		  break;
	   }
	}

	return(ret_val);
}
#endif
