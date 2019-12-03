/*
 *
 * BLE GATT Profile - General Information Service
 * The BLE Service specific handler functions and routines are defined in this file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include <ti/sysbios/knl/Semaphore.h>
#include "SS1BTGAT.h"
#include "BLEUtils.h"
#include "nano_timer.h"
#include "BLECommonDefs.h"
#include "BLECmdHandlerLiaison.h"
#include "BLENotificationHandler.h"
#include "BLEGATTGISvcDefs.h"
#include "BLEGATTGISvcUtilFunc.h"
#include "BLEGATTGISvc.h"

extern Semaphore_Handle BLENotifySem;

ApplicationStateInfo_GISvc_t ApplicationStateInfo_GISvc;

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTGISvc_InitApplicationStateInfo()
{
	ApplicationStateInfo_GISvc.GATTGISvcInstanceID = 0;
	ApplicationStateInfo_GISvc.GATTGISvcServiceID = 0;
	ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor = 0;
	ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor = 0;
	ApplicationStateInfo_GISvc.DeviceStatus = 0;
	ApplicationStateInfo_GISvc.DeviceStatus_Client_Configuration_Descriptor = 0;
	ApplicationStateInfo_GISvc.ErrorStatus = 0;
	ApplicationStateInfo_GISvc.ErrorStatus_Client_Configuration_Descriptor = 0;
	ApplicationStateInfo_GISvc.TempThreshold = 0x7fff;
	ApplicationStateInfo_GISvc.HumThreshold = 0xffff;
	ApplicationStateInfo_GISvc.NumHoursOfUse = 0;
	ApplicationStateInfo_GISvc.NumBattRecharge = 0;
	ApplicationStateInfo_GISvc.NumLampHours = 0;
}

GISvc_ServerInstance_t GISvcServiceInstance;

/**
 * Registers the service with GATT server
 *
 * @param[in] BluetoothStackID  Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTGISvc_Register(unsigned int BluetoothStackID)
{

	int ret_val = 0;
	GATT_Attribute_Handle_Group_t ServiceHandleGroup;
	unsigned long CallbackParameter = 0;

	/* Verify that the Service is not already registered.             */
	if (!ApplicationStateInfo_GISvc.GATTGISvcServiceID)
	{
		/* Initialize the handle group to 0 .                          */
		ServiceHandleGroup.Starting_Handle = 0;
		ServiceHandleGroup.Ending_Handle = 0;

		/* Register the BLE GI Service                            */
		ret_val = GATT_Register_Service(BluetoothStackID,
										BLE_SERVICE_FLAGS,
										BLE_GI_SERVICE_ATTRIBUTE_COUNT,
										(GATT_Service_Attribute_Entry_t *) BLE_GISvc_Att_Entry,
										&ServiceHandleGroup,
										GATTGISvc_ServerEventCallback,
										0);
		if (ret_val > 0)
		{
			/* Display success message.                                 */
			DEBUG_PRINT("Sucessfully registered GATT GI Service, ID: %d\r\n", ret_val);

			GISvcServiceInstance.BluetoothStackID  = BluetoothStackID;
			GISvcServiceInstance.ServiceID         = (unsigned int)ret_val;
			GISvcServiceInstance.CallbackParameter = CallbackParameter;
			GISvcServiceInstance.ConnectionID 	   = 0;

			/* Save the ServiceID of the registered service.            */
			ApplicationStateInfo_GISvc.GATTGISvcInstanceID = (unsigned int) ret_val;

			/* Return success to the caller.                            */
			ret_val = 0;
		}
		else
		{
			DEBUG_PRINT("Failed to register GI Service; GATT Server returned error:%d\r\n",ret_val);
		}
	}
	else
	{
		DEBUG_PRINT("GATT GI Service already registered.\r\n");
		ret_val = APPLICATION_ERROR_GATT_SERVICE_EXISTS;
	}

	return ret_val;
}

/**
 * Unregister the service from GATT server
 *
 * @param[in] BluetoothStackID  Bluetooth stack instance ID
 *
 * @return None
 */
void GATTGISvc_Unregister(unsigned int BluetoothStackID)
{
	GATT_Un_Register_Service(BluetoothStackID, ApplicationStateInfo_GISvc.GATTGISvcServiceID);
	GATTGISvc_InitApplicationStateInfo();
}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param[in] ConnectionID  New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTGISvc_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		GISvcServiceInstance.ConnectionID 	   = ConnectionID;
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
 * @param[in] BluetoothStackID      bluetooth Stack instance ID
 * @param[in] GATT_ServerEventData  GATT Server event data
 * @param[in] CallbackParameter     Paramter registered with GATT server
 *
 * @return None
 */
void BTPSAPI GATTGISvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
	Byte_t      TempWord[2];
	Word_t      AttributeOffset;
	Word_t 		thresh;
	Boolean_t 	Notify;
	BLE_RESPONSE_INFO bleEventInfo;
	int length = 0;
	BLE_NOTIFY_INFO notifyInfo;

	// To avoid static code analyser error
	notifyInfo.data = NULL;
	notifyInfo.data_changed = false;
	notifyInfo.data_length = 0;

	DEBUG_PRINT("\r\nGI Service Event Call back\r\n");

	if (GATT_ServerEventData)
		DEBUG_PRINT("Event ID: %d\r\n",GATT_ServerEventData->Event_Data_Type);

	nano_timer_increment_activity_count();

	/* Verify that all parameters to this callback are Semi-Valid.       */
	if ((BluetoothStackID) && (GATT_ServerEventData))
	{
	   switch(GATT_ServerEventData->Event_Data_Type)
	   {
	   	  case etGATT_Server_Confirmation_Response:
	   		if(GATT_ServerEventData->Event_Data.GATT_Confirmation_Data)
	   		{
	   			if(BLEUtil_FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Confirmation_Data->RemoteDevice) >= 0)
	   			{
	   				DEBUG_PRINT("\r\nIndication ack recieved for error status\r\n");
	   				if (bleNotificationHandler_updateIndicationInfo(BLE_INDICATE_ERROR_STATUS,
	   															   GATT_ServerEventData->Event_Data.GATT_Confirmation_Data->TransactionID,
																   (int)GATT_ServerEventData->Event_Data.GATT_Confirmation_Data->BytesWritten) < 0)
	   				{
	   					DEBUG_PRINT("\r\nERROR: Updating Notification Handler failed, Error Indication may not be sent until this is rectified\r\n");
	   				}
	   			}
	   		}
	   		break;
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

					   	 /* Determine which request this read is coming  */
						 /* for.                                         */
						 switch(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset)
						 {
							case BLE_GISVC_TEMP_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								bleEventInfo.fileType = NNO_READ_TEMP;
								bleEventInfo.key = NNO_CMD_READ_TEMP;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								bleEventInfo.btInfo.ccdOffset = BLE_GISVC_TEMP_CHARACTERISTIC_ATTRIBUTE_OFFSET;
								bleEventInfo.btInfo.serviceID = 0;
								bleEventInfo.btInfo.connectionID = 0;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(NULL,length,bleEventInfo);
							   break;
							case BLE_GISVC_TEMP_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
								DEBUG_PRINT("\r\n*****************:%d\r\n",ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor);
								break;
							case BLE_GISVC_HUM_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								bleEventInfo.fileType = NNO_READ_HUM;
								bleEventInfo.key = NNO_CMD_READ_HUM;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								bleEventInfo.btInfo.ccdOffset = BLE_GISVC_HUM_CHARACTERISTIC_ATTRIBUTE_OFFSET;
								bleEventInfo.btInfo.serviceID = 0;
								bleEventInfo.btInfo.connectionID = 0;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(NULL,length,bleEventInfo);
								break;
							case BLE_GISVC_HUM_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
								DEBUG_PRINT("\r\n*****************:%d\r\n",ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor);
								break;
							case BLE_GISVC_DEVSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								bleEventInfo.fileType = NNO_READ_DEV_STAT;
								bleEventInfo.key = NNO_CMD_READ_DEVICE_STATUS;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								bleEventInfo.btInfo.ccdOffset = BLE_GISVC_DEVSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET;
								bleEventInfo.btInfo.serviceID = 0;
								bleEventInfo.btInfo.connectionID = 0;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(NULL,length,bleEventInfo);
								break;
							case BLE_GISVC_DEVSTAT_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.DeviceStatus_Client_Configuration_Descriptor);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
								break;
							case BLE_GISVC_ERRSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								bleEventInfo.fileType = NNO_READ_ERR_STAT;
								bleEventInfo.key = NNO_CMD_READ_ERROR_STATUS;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								bleEventInfo.btInfo.ccdOffset = BLE_GISVC_ERRSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET;
								bleEventInfo.btInfo.serviceID = 0;
								bleEventInfo.btInfo.connectionID = 0;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(NULL,length,bleEventInfo);
								break;
							case BLE_GISVC_ERRSTAT_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.ErrorStatus_Client_Configuration_Descriptor);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
							case BLE_GISVC_HOURSOFUSE_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_BIG_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.NumHoursOfUse);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
								break;
							case BLE_GISVC_BATTRECHARGE_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_BIG_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.NumBattRecharge);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
								break;
							case BLE_GISVC_LAMPHOURS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								ASSIGN_HOST_WORD_TO_BIG_ENDIAN_UNALIGNED_WORD(TempWord, ApplicationStateInfo_GISvc.NumLampHours);
								GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, TempWord);
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
				 DEBUG_PRINT("\r\nWrite Request data\r\n");
				if	(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
				{
				   /* Cache the Attribute Offset.                        */
				   AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
				   DEBUG_PRINT("\r\nWrite Request data Att Length:%d\r\n",GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength);
				   if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength)
				   {
					   DEBUG_PRINT("\r\nWrite Request data Offset:%d\r\n",AttributeOffset);

					   switch (AttributeOffset)
					   {
					   	   case BLE_GISVC_DEVSTAT_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
					   		   if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
					   		   {
					   			   DEBUG_PRINT("\r\nDecoded Client configuration value\r\n");
					   			   /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
					   			   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
					   			   /* Update the stored configuration for this device.   */
					   			   if (Notify)
					   			   {
					   				   ApplicationStateInfo_GISvc.DeviceStatus_Client_Configuration_Descriptor = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
				   					   notifyInfo.btInfo.bluetoothID = BluetoothStackID;
				   					   notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
				   					   notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID;
				   					   notifyInfo.btInfo.ccdOffset = BLE_GISVC_DEVSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET;
				   					   notifyInfo.type = BLE_NOTIFT_DEVICE_STATUS;
				   					   notifyInfo.btInfo.transactionID = 0;

					   				   if (0 != bleNotificationHandler_registerNotification(notifyInfo))
					   					   DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
					   			   }
					   			   else
					   			   {
					   				   ApplicationStateInfo_GISvc.DeviceStatus_Client_Configuration_Descriptor = 0;
					   				   if (0 != bleNotificationHandler_deregisterNotification(BLE_NOTIFT_DEVICE_STATUS))
					   					DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
					   			   }
					   		   }
					   		   break;
					   	   	case BLE_GISVC_ERRSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET:
							   bleEventInfo.fileType = NNO_RESET_ERR_STAT;
							   bleEventInfo.key = NNO_CMD_RESET_ERROR_STATUS;
							   bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_DELAYED_RESPONSE;
							   bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
							   bleEventInfo.btInfo.ccdOffset = BLE_GISVC_ERRSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET;
							   bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID;
							   bleEventInfo.btInfo.serviceID = GISvcServiceInstance.ServiceID;
							   bleEventInfo.btInfo.connectionID = GISvcServiceInstance.ConnectionID;
							   bleEventInfo.subfieldType = 0;
							   bleEventInfo.dataType = 0;
							   bleCmdHandlerLiason_relayCmd(NULL,0,bleEventInfo);
					   		   break;
					   		case BLE_GISVC_ERRSTAT_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
					   		   if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
					   		   {
					   			   DEBUG_PRINT("\r\nIn Error Status notification status!!!!\r\n");
					   			   /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
					   			   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
					   			   /* Update the stored configuration for this device.   */
					   			   if (Notify)
					   			   {
					   				   ApplicationStateInfo_GISvc.ErrorStatus_Client_Configuration_Descriptor = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
					   				   notifyInfo.btInfo.bluetoothID = BluetoothStackID;
					   				   notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
					   				   notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID;
					   				   notifyInfo.btInfo.ccdOffset = BLE_GISVC_ERRSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET;
				   					   notifyInfo.type = BLE_INDICATE_ERROR_STATUS;
				   					   notifyInfo.btInfo.transactionID = 0;

					   				   if (0 != bleNotificationHandler_registerNotification(notifyInfo))
					   					   DEBUG_PRINT("\r\nError: Status indication registration failed!!!!\r\n");
					   			   }
					   			   else
					   			   {
					   				   if (0 != bleNotificationHandler_deregisterNotification(BLE_INDICATE_ERROR_STATUS))
					   					   DEBUG_PRINT("\r\nError: Error status indication De-registration failed!!!!\r\n");
					   				   ApplicationStateInfo_GISvc.ErrorStatus_Client_Configuration_Descriptor = 0;
					   			   }
					   		   }
					   		   break;
					   	   case BLE_GISVC_TEMPTHRESH_CHARACTERISTIC_ATTRIBUTE_OFFSET:
					   		   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
							   thresh = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
					   		   ApplicationStateInfo_GISvc.TempThreshold = thresh;
					   		   break;
					   	   case BLE_GISVC_TEMP_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
					   		   if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
					   		   {
					   			   /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
					   			   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
					   			   /* Update the stored configuration for this device.   */
					   			   if (Notify)
					   			   {
					   				   ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
				   					   notifyInfo.btInfo.bluetoothID = BluetoothStackID;
				   					   notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
				   					   notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID;
				   					   notifyInfo.btInfo.ccdOffset = BLE_GISVC_TEMP_CHARACTERISTIC_ATTRIBUTE_OFFSET;
				   					   notifyInfo.type = BLE_NOTIFY_TEMPERATURE;
				   					   notifyInfo.btInfo.transactionID = 0;

					   				   if (0 != bleNotificationHandler_registerNotification(notifyInfo))
					   					   DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
					   			   }
					   			   else
					   			   {
					   				   ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor = 0;
					   				   if (0 != bleNotificationHandler_deregisterNotification(BLE_NOTIFY_TEMPERATURE))
					   					   DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
					   			   }
					   			   DEBUG_PRINT("\r\n*****************:%d\r\n",ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor);
					   		   }
					   		   break;
					   	   case BLE_GISVC_HUMTHRESH_CHARACTERISTIC_ATTRIBUTE_OFFSET:
					   		   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
							   thresh = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
					   		   ApplicationStateInfo_GISvc.HumThreshold = thresh;
					   		   break;
					   	   case BLE_GISVC_HUM_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
					   		   if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
					   		   {
					   			   /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
					   			   GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID);
					   			   /* Update the stored configuration for this device.   */
					   			   if (Notify)
					   			   {
					   				   ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
				   					   notifyInfo.btInfo.bluetoothID = BluetoothStackID;
				   					   notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
				   					   notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID;
				   					   notifyInfo.btInfo.ccdOffset = BLE_GISVC_HUM_CHARACTERISTIC_ATTRIBUTE_OFFSET;
				   					   notifyInfo.type = BLE_NOTIFY_HUMIDITY;
				   					   notifyInfo.btInfo.transactionID = 0;

					   				   if (0 != bleNotificationHandler_registerNotification(notifyInfo))
					   					   DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
					   			   }
					   			   else
					   			   {
					   				   ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor = 0;
					   				   if (0 != bleNotificationHandler_deregisterNotification(BLE_NOTIFY_HUMIDITY))
					   					   DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
					   			   }
					   			   if (Notify)
					   				   ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
					   			   else
					   				   ApplicationStateInfo_GISvc.Hum_Client_Configuration_Descriptor = 0;
					   		   }
					   		   break;
					   }

				   }
				   else
				   {
					   // Improper Attribute Length
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

/**
 * Function to Set Temperature and send notification
 *
 * @param[in]	temperature	Measured temperature
 * @param[in]	sendNotification	Send notification or not
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetTemp(int16_t temperature, bool sendNotification)
{
	int retval = 0;
	Byte_t temp[2];

	DEBUG_PRINT("\r\nTemperature set:%d\r\n",temperature);

	temp[0] = 0xff & temperature;
	temp[1] = (0xff00 & temperature) >> 8;

	/* Notify if enabled */
	if ((ApplicationStateInfo_GISvc.Temp_Client_Configuration_Descriptor || GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) > 0)
	{
		if ((sendNotification) && (temperature > ApplicationStateInfo_GISvc.TempThreshold))
		{
			if (0 == bleNotificationHandler_setNotificationData(BLE_NOTIFY_TEMPERATURE, sizeof(Word_t), &temp[0]))
				Semaphore_post(BLENotifySem);
			else
				retval = -1;
		}
	}

	return retval;
}

/**
 * Function to Set Humidity and send notification
 *
 * @param[in] humidity          Measured humidity
 * @param[in] sendNotification  Send notification or not
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetHum(uint16_t humidity, bool sendNotification)
{
	int retval = 0;
	Byte_t temp[2];

	DEBUG_PRINT("\r\nHumidity set:%d\r\n", humidity);

	temp[0] = 0xff & humidity;
	temp[1] = (0xff00 & humidity) >> 8;

	/* Notify if greater than threshold */
	if (humidity > ApplicationStateInfo_GISvc.HumThreshold)	// Notify only if the value is above threshold
	{
		if ((sendNotification) && (humidity > ApplicationStateInfo_GISvc.HumThreshold))
		{
			if (0 == bleNotificationHandler_setNotificationData(BLE_NOTIFY_HUMIDITY, 2, &temp[0]))
				Semaphore_post(BLENotifySem);
			else
				retval = -1;
		}
	}

	return retval;
}

/**
 * Function to Set Device status and send notification
 *
 * @param[in] devStat   Device status
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetDevStat(Word_t devStat)
{
	int retval = 0;

	// Set the value first
	ApplicationStateInfo_GISvc.DeviceStatus = devStat;

	/* Notify if enabled */
	if ((ApplicationStateInfo_GISvc.DeviceStatus_Client_Configuration_Descriptor || GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) > 0)
	{
		Byte_t temp[2];
		ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(temp, ApplicationStateInfo_GISvc.DeviceStatus);
		if (0 == bleNotificationHandler_setNotificationData(BLE_NOTIFT_DEVICE_STATUS, 2, &temp[0]))
			Semaphore_post(BLENotifySem);
		else
			retval = -1;
	}

	return retval;
}

/**
 * Function to Set Error Status and send notification
 *
 * @param[in] errStat   Error status
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetErrStat(Word_t errStat)
{
	int retval = 0;

	// Set the value first
	ApplicationStateInfo_GISvc.ErrorStatus = errStat;

	/* Notify if enabled */
	if ((ApplicationStateInfo_GISvc.ErrorStatus_Client_Configuration_Descriptor || GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) > 0)
	{
		Byte_t temp[2];
		ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(temp, ApplicationStateInfo_GISvc.ErrorStatus);
		if (0 == bleNotificationHandler_setNotificationData(BLE_INDICATE_ERROR_STATUS, 2, &temp[0]))
			Semaphore_post(BLENotifySem);
		else
			retval = -1;
	}

	return retval;
}

/**
 * Function to Set hours of use and send notification
 *
 * @param[in] numHours  number of hours
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetNumHoursOfUse(Word_t numHours)
{
	DEBUG_PRINT("\r\nSet hours of use:%d",numHours);
	ApplicationStateInfo_GISvc.NumHoursOfUse = numHours;
	return (0);
}

/**
 * Function to Set num recharge cycles and send notification
 *
 * @param[in] numBattRecharge   Number of recharge cycles
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetBattRecharge(Word_t numBattRecharge)
{
	ApplicationStateInfo_GISvc.NumBattRecharge = numBattRecharge;
	return (0);
}

/**
 * Function to Set total lamp hours and send notification
 *
 * @param[in] lampHours     Number of lamp hours
 *
 * @return true on success; false on failure.
 */
int GATTGISvc_SetLampHours(Word_t lampHours)
{
	ApplicationStateInfo_GISvc.NumLampHours = lampHours;
	return (0);
}
#endif
