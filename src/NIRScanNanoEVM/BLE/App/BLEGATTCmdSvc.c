/*
 *
 * BLE GATT Profile - Command Service
 * The BLE Service specific handler functions and routines are defined in this file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdio.h>               /* Included for sscanf.                     */
#include <stdint.h>
#include <stdbool.h>
#include <xdc/runtime/System.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                    */
#include "BLECommonDefs.h"
#include "BLENotificationHandler.h"
#include "BLEGATTCmdSvcDefs.h"
#include "BLEUtils.h"
#include "nano_timer.h"
#include "BLEGATTCmdSvc.h"

ApplicationStateInfo_CmdSvc_t ApplicationStateInfo_CmdSvc;

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTCmdSvc_InitApplicationStateInfo()
{
	ApplicationStateInfo_CmdSvc.GATTCmdSvcInstanceID = 0;
	ApplicationStateInfo_CmdSvc.GATTCmdSvcServiceID = 0;
}

CmdSvc_DeviceInfo_t CmdSvcDeviceInfo;

CmdSvc_ServerInstance_t CmdSvcServiceInstance;

/**
 * Registers the service with GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTCmdSvc_Register(unsigned int BluetoothStackID)
{

	int ret_val = 0;
	GATT_Attribute_Handle_Group_t ServiceHandleGroup;
	unsigned long CallbackParameter = 0;

	/* Verify that the Service is not already registered.             */
	if (!ApplicationStateInfo_CmdSvc.GATTCmdSvcServiceID)
	{
		/* Initialize the handle group to 0 .                          */
		ServiceHandleGroup.Starting_Handle = 0;
		ServiceHandleGroup.Ending_Handle = 0;

		/* Register the BLE Command Service                            */
		ret_val = GATT_Register_Service(BluetoothStackID,
										BLE_SERVICE_FLAGS,
										BLE_COMMAND_SERVICE_ATTRIBUTE_COUNT,
										(GATT_Service_Attribute_Entry_t *) BLE_CmdSvc_Att_Entry,
										&ServiceHandleGroup,
										GATTCmdSvc_ServerEventCallback,
										0);
		if (ret_val > 0)
		{
			/* Display success message.                                 */
			DEBUG_PRINT("Sucessfully registered GATT Command Service, ID: %d\r\n", ret_val);

			CmdSvcServiceInstance.BluetoothStackID  = BluetoothStackID;
			CmdSvcServiceInstance.ServiceID         = (unsigned int)ret_val;
			CmdSvcServiceInstance.EventCallback     = NULL;
			CmdSvcServiceInstance.CallbackParameter = CallbackParameter;

			/* Save the ServiceID of the registered service.            */
			ApplicationStateInfo_CmdSvc.GATTCmdSvcInstanceID = (unsigned int) ret_val;

			/* Return success to the caller.                            */
			ret_val = 0;
		}
		else
		{
			DEBUG_PRINT("Failed to register Command Service; GATT Server returned error:%d\r\n",ret_val);
		}
	}
	else
	{
		DEBUG_PRINT("GATT Command Service already registered.\r\n");
		ret_val = APPLICATION_ERROR_GATT_SERVICE_EXISTS;
	}

	return ret_val;
}

/**
 * Unregister the service from GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return None
 */
void GATTCmdSvc_Unregister(unsigned int BluetoothStackID)
{
	GATT_Un_Register_Service(BluetoothStackID, ApplicationStateInfo_CmdSvc.GATTCmdSvcServiceID);
	GATTCmdSvc_InitApplicationStateInfo();
}

/**
 * GATT Server Event Callback.  This function will be called whenever
 * a GATT Request is made to the server who registers this function
 * that cannot be handled internally by GATT
 *
 * @param BluetoothStackID [in] bluetooth Stack instance ID
 * @param GATT_ServerEventData [in] GATT Server event data
 * @param CallbackParameter [in] Paramter registered with GATT server
 *
 * @return None
 */
void BTPSAPI GATTCmdSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
	Byte_t      Temp[2];
	Word_t      AttributeOffset;
	Boolean_t 	Notify;
	int 		i;
	BLE_NOTIFY_INFO notifyInfo;

	// To avoid static code analyser error
	notifyInfo.data = NULL;
	notifyInfo.data_changed = false;
	notifyInfo.data_length = 0;

	DEBUG_PRINT("\r\nCommand Service Event Call back\r\n");

	if (GATT_ServerEventData)
		DEBUG_PRINT("Event ID: %d\r\n",GATT_ServerEventData->Event_Data_Type);

	nano_timer_increment_activity_count();

	/* Verify that all parameters to this callback are Semi-Valid.       */
	if ((BluetoothStackID) && (GATT_ServerEventData))
	{
	   switch(GATT_ServerEventData->Event_Data_Type)
	   {
		  case etGATT_Server_Read_Request:
			 /* Verify that the Event Data is valid.                     */
			 if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data)
			 {
				/* Verify that the read isn't a read blob (no BLE      */
				/* readable characteristics are long).                   */
				if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset == 0)
				{
				   /* Find the LE Connection Index for this connection.  */
				   if(BLEUtil_FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice) >= 0)
				   {
					   	 /* Determine which request this read is coming  */
						 /* for.                                         */
						 switch(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset)
						 {
							case BLE_CMDSVC_IC_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
							   ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, CmdSvcDeviceInfo.Client_Configuration_Descriptor);
							   GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, Temp);
							   break;
						 }
				   }
				   else
				   {
					  DEBUG_PRINT("Error - No such device connected.\r\n");
				   }
				}
				else
				   bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
			 }
			 else
				DEBUG_PRINT("Invalid Read Request Event Data.\r\n");
			 break;
		  case etGATT_Server_Write_Request:
			 /* Verify that the Event Data is valid.                     */
			 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
			 {
				if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
				{
				   /* Cache the Attribute Offset.                        */
				   AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;

				   /* Verify that the value is of the correct length.    */
				   if((AttributeOffset == BLE_CMDSVC_IC_CHARACTERISTIC_ATTRIBUTE_OFFSET) ||
					  ((GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength)
					   && (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength <= WORD_SIZE)))
				   {
					  /* Find the LE Connection Index for this           */
					  /* connection.                                     */
					  if(BLEUtil_FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice) >= 0)
					  {
							/* Since the value appears valid go ahead and*/
							/* accept the write request.                 */
							GATT_Write_Response(BluetoothStackID,
									GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
							CmdSvcDeviceInfo.Data_size = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;
							DEBUG_PRINT("\r\nAttribute Length:%d",GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength);

							/* If this is not a write to the Rx          */
							/* Characteristic we will read the data here.*/
							//if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
							//	Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

							/* Determine which attribute this write      */
							/* request is for.                           */
							switch(AttributeOffset)
							{
							   case BLE_CMDSVC_IC_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
								   if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								   {
									   DEBUG_PRINT("\r\nIn Error Status notification status!!!!\r\n");
									   /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
									   /* Update the stored configuration for this device.   */
									   if (Notify)
									   {
										   CmdSvcDeviceInfo.Client_Configuration_Descriptor = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
										   notifyInfo.btInfo.bluetoothID = BluetoothStackID;
										   notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
										   notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID;
										   notifyInfo.btInfo.ccdOffset = BLE_CMDSVC_IC_CHARACTERISTIC_ATTRIBUTE_OFFSET;
										   notifyInfo.type = BLE_NOTIFY_COMMANDS;
										   notifyInfo.btInfo.transactionID = 0;

										   if (0 != bleNotificationHandler_registerNotification(notifyInfo))
											   DEBUG_PRINT("\r\nError: Command notification registration failed!!!!\r\n");
									   }
									   else
									   {
										   if (0 != bleNotificationHandler_deregisterNotification(BLE_NOTIFY_COMMANDS))
											   DEBUG_PRINT("\r\nError: Command notification de-registration failed!!!!\r\n");
										   CmdSvcDeviceInfo.Client_Configuration_Descriptor = 0;
									   }
								   }
								   break;
							   case BLE_CMDSVC_IC_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								   DEBUG_PRINT("\r\nInternal Command recieved:");
								   for (i=0;i<CmdSvcDeviceInfo.Data_size;i++)
								   {
									   CmdSvcDeviceInfo.InternalCommand[i] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i];
								   }

								   bleHandleInternalMessage(CmdSvcDeviceInfo.Data_size, CmdSvcDeviceInfo.InternalCommand,
										   					CmdSvcDeviceInfo.Client_Configuration_Descriptor ==
												   	   	    	GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE
																? BluetoothStackID : 0,
															GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ServiceID,
															CmdSvcServiceInstance.ConnectionID,
															BLE_CMDSVC_IC_CHARACTERISTIC_ATTRIBUTE_OFFSET);
								   break;
							}
					  }
					  else
					  {
						  DEBUG_PRINT("\r\nError - No such device connected\r\n");
					  }
				   }
				   else
					  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
				}
				else
				   bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
			 }
			 else
				 DEBUG_PRINT("\r\nInvalid Write Request Event Data.\r\n");
			 break;
	   }
	}
	else
	{
	   /* There was an error with one or more of the input parameters.   */
		DEBUG_PRINT("\r\n");

		DEBUG_PRINT("GATT Callback Data: Event_Data = NULL.\r\n");
	}

}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param ConnectionID [in] New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTCmdSvc_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		CmdSvcServiceInstance.ConnectionID = ConnectionID;

		return (0);
	}
	else
		return BLE_ERROR_INVALID_PARAMETER;
}
#endif
