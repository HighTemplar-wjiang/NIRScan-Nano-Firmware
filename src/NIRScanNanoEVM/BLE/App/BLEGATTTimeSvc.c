/*
 *
 * BLE GATT Profile - Time Service
 * The BLE Service specific handler functions and routines are defined in this file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdio.h>               /* Included for sscanf.                     */
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                    */
#include "NNOCommandDefs.h"
#include "BLECommonDefs.h"
#include "BLEUtils.h"
#include "BLECmdHandlerLiaison.h"
#include "nano_timer.h"
#include "BLEGATTTimeSvcDefs.h"
#include "BLEGATTTimeSvc.h"

ApplicationStateInfo_TimeSvc_t ApplicationStateInfo_TimeSvc;

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTTimeSvc_InitApplicationStateInfo()
{
	ApplicationStateInfo_TimeSvc.GATTTimeSvcInstanceID = 0;
	ApplicationStateInfo_TimeSvc.GATTTimeSvcServiceID = 0;
	ApplicationStateInfo_TimeSvc.year = 0;
	ApplicationStateInfo_TimeSvc.month = 0;
	ApplicationStateInfo_TimeSvc.day = 0;
	ApplicationStateInfo_TimeSvc.dow = 0;
	ApplicationStateInfo_TimeSvc.hour = 0;
	ApplicationStateInfo_TimeSvc.minute = 0;
	ApplicationStateInfo_TimeSvc.second = 0;
}

TimeSvc_ServerInstance_t TimeSvcServiceInstance;

/**
 * Registers the service with GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTTimeSvc_Register(unsigned int BluetoothStackID)
{

	int ret_val = 0;
	GATT_Attribute_Handle_Group_t ServiceHandleGroup;
	unsigned long CallbackParameter = 0;

	/* Verify that the Service is not already registered.             */
	if (!ApplicationStateInfo_TimeSvc.GATTTimeSvcServiceID)
	{
		/* Initialize the handle group to 0 .                          */
		ServiceHandleGroup.Starting_Handle = 0;
		ServiceHandleGroup.Ending_Handle = 0;

		/* Register the BLE GI Service                            */
		ret_val = GATT_Register_Service(BluetoothStackID,
										BLE_SERVICE_FLAGS,
										BLE_TIME_SERVICE_ATTRIBUTE_COUNT,
										(GATT_Service_Attribute_Entry_t *) BLE_TimeSvc_Att_Entry,
										&ServiceHandleGroup,
										GATTTimeSvc_ServerEventCallback,
										0);
		if (ret_val > 0)
		{
			/* Display success message.                                 */
			DEBUG_PRINT("Sucessfully registered GATT Time Service, ID: %d\r\n", ret_val);

			TimeSvcServiceInstance.BluetoothStackID  = BluetoothStackID;
			TimeSvcServiceInstance.ServiceID         = (unsigned int)ret_val;
			TimeSvcServiceInstance.CallbackParameter = CallbackParameter;
			TimeSvcServiceInstance.ConnectionID 	   = 0;

			/* Save the ServiceID of the registered service.            */
			ApplicationStateInfo_TimeSvc.GATTTimeSvcInstanceID = (unsigned int) ret_val;

			/* Return success to the caller.                            */
			ret_val = 0;
		}
		else
		{
			DEBUG_PRINT("Failed to register Time Service; GATT Server returned error:%d\r\n",ret_val);
		}
	}
	else
	{
		DEBUG_PRINT("GATT Time Service already registered.\r\n");
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
void GATTTimeSvc_Unregister(unsigned int BluetoothStackID)
{
	GATT_Un_Register_Service(BluetoothStackID, ApplicationStateInfo_TimeSvc.GATTTimeSvcServiceID);
	GATTTimeSvc_InitApplicationStateInfo();
}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param ConnectionID [in] New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTTimeSvc_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		TimeSvcServiceInstance.ConnectionID 	   = ConnectionID;
		return (0);
	}
	else
		return BLE_ERROR_INVALID_PARAMETER;
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
void BTPSAPI GATTTimeSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
	Word_t      AttributeOffset;
	Byte_t writeVal[BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE];
	int length = 0;
	BLE_RESPONSE_INFO bleEventInfo;

	DEBUG_PRINT("\r\nGI Service Event Call back\r\n");

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
				DEBUG_PRINT("\r\nAttribute Offset: %d\r\n",GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset);
				DEBUG_PRINT("\r\nAttribute Value Offset: %d\r\n",GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset);
				/* Verify that the read isn't a read blob (no BLE      */
				/* readable characteristics are long).                   */
				if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset == 0)
				{
				   /* Find the LE Connection Index for this connection.  */
				   if(BLEUtil_FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice) >= 0)
				   {
					   DEBUG_PRINT("\r\nIn switch case based on offset value\r\n");
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
				 DEBUG_PRINT("\r\nWrite Request data\r\n");
				if	(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
				{
				   /* Cache the Attribute Offset.                        */
				   AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
				   DEBUG_PRINT("\r\nWrite Request data Att Length:%d\r\n",GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength);
				   if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength)
				   {
					   switch (AttributeOffset)
					   {
					   	   case BLE_TIMESVC_TIME_CHARACTERISTIC_ATTRIBUTE_OFFSET:
					   		   if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == 7)
					   		   {
					   		   	   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);


					   		   	   writeVal[0] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[0];
					   			   writeVal[1] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[1];
					   			   writeVal[2] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[2];
					   			   writeVal[3] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[3];
					   			   writeVal[4] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[4];
					   			   writeVal[5] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[5];
					   			   writeVal[6] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[6];

									bleEventInfo.fileType = BLE_MAX_COMMANDS;
									bleEventInfo.key = NNO_CMD_SET_DATE_TIME;
									bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE;
									length = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;
									bleEventInfo.btInfo.bluetoothID = 0;
									bleEventInfo.btInfo.ccdOffset = 0;
									bleEventInfo.btInfo.serviceID = 0;
									bleEventInfo.btInfo.connectionID = 0;
									bleEventInfo.subfieldType = 0;
									bleEventInfo.dataType = 0;
									bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
					   		   }
					   		   break;
					   }

				   }
				   else
				   {
					   // Improper Attribute value Length
					   bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
				   }
				}
				else
				{
					bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
				}
			 }
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
#endif
