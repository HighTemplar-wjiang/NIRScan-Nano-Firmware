/*
 *
 * BLE GATT Profile - (Scan) Configuration Service
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
#include "BLEGATTConfSvcDefs.h"
#include "BLEGATTConfSvcUtilFunc.h"
#include "BLEUtils.h"
#include "BLECmdHandlerLiaison.h"
#include "nano_timer.h"
#include "BLEGATTConfSvc.h"

extern unsigned short gBLESuppMTUSize;

ApplicationStateInfo_ConfSvc_t ApplicationStateInfo_ConfSvc;

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTConfSvc_InitApplicationStateInfo()
{
	ApplicationStateInfo_ConfSvc.GATTConfSvcInstanceID = 0;
	ApplicationStateInfo_ConfSvc.GATTConfSvcServiceID = 0;
	ApplicationStateInfo_ConfSvc.numStoredConf = 0;
	ApplicationStateInfo_ConfSvc.scanConfIdx = 0;
	ApplicationStateInfo_ConfSvc.scanConfData_CCD = 0;
	ApplicationStateInfo_ConfSvc.storedConfList_CCD = 0;
}

ConfSvc_ServerInstance_t ConfSvcServiceInstance;

/**
 * Registers the service with GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTConfSvc_Register(unsigned int BluetoothStackID)
{

	int ret_val = 0;
	GATT_Attribute_Handle_Group_t ServiceHandleGroup;
	unsigned long CallbackParameter = 0;

	/* Verify that the Service is not already registered.             */
	if (!ApplicationStateInfo_ConfSvc.GATTConfSvcServiceID)
	{
		/* Initialize the handle group to 0 .                          */
		ServiceHandleGroup.Starting_Handle = 0;
		ServiceHandleGroup.Ending_Handle = 0;

		/* Register the BLE GI Service                            */
		ret_val = GATT_Register_Service(BluetoothStackID,
										BLE_SERVICE_FLAGS,
										BLE_CONF_SERVICE_ATTRIBUTE_COUNT,
										(GATT_Service_Attribute_Entry_t *) BLE_ConfSvc_Att_Entry,
										&ServiceHandleGroup,
										GATTConfSvc_ServerEventCallback,
										0);
		if (ret_val > 0)
		{
			/* Display success message.                                 */
			DEBUG_PRINT("Sucessfully registered GATT Configuration Service, ID: %d\r\n", ret_val);

			ConfSvcServiceInstance.BluetoothStackID  = BluetoothStackID;
			ConfSvcServiceInstance.ServiceID         = (unsigned int)ret_val;
			ConfSvcServiceInstance.CallbackParameter = CallbackParameter;
			ConfSvcServiceInstance.ConnectionID 	   = 0;

			/* Save the ServiceID of the registered service.            */
			ApplicationStateInfo_ConfSvc.GATTConfSvcInstanceID = (unsigned int) ret_val;

			/* Return success to the caller.                            */
			ret_val = 0;
		}
		else
		{
			DEBUG_PRINT("Failed to register Conf Service; GATT Server returned error:%d\r\n",ret_val);
		}
	}
	else
	{
		DEBUG_PRINT("GATT Conf Service already registered.\r\n");
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
void GATTConfSvc_Unregister(unsigned int BluetoothStackID)
{
	GATT_Un_Register_Service(BluetoothStackID, ApplicationStateInfo_ConfSvc.GATTConfSvcServiceID);
	GATTConfSvc_InitApplicationStateInfo();
}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param ConnectionID [in] New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTConfSvc_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		ConfSvcServiceInstance.ConnectionID 	   = ConnectionID;

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
void BTPSAPI GATTConfSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
	Word_t	AttributeOffset = 0;
	Byte_t TempWord[2];
	Boolean_t 	Notify;
	Byte_t writeVal[BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE];
	int length = 0, i = 0;

	BLE_RESPONSE_INFO bleEventInfo;

	DEBUG_PRINT("\nConf Service Event Call back\n");

	if (GATT_ServerEventData)
	{
		DEBUG_PRINT("Event ID: %d\r\n",GATT_ServerEventData->Event_Data_Type);
	}
	else
	{
		DEBUG_PRINT("\nInvalid event data\n");
	}

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
						switch (GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset)
						{
							case BLE_CONFSVC_NUM_STORED_CONF_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								  bleEventInfo.fileType = NNO_NUM_SCAN_CONFIG;
								  bleEventInfo.key = NNO_CMD_SCAN_CFG_NUM;
								  bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
								  bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								  bleEventInfo.btInfo.ccdOffset = 0;
								  bleEventInfo.btInfo.serviceID = 0;
								  bleEventInfo.btInfo.connectionID = 0;
								  bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								  bleEventInfo.subfieldType = 0;
								  bleEventInfo.dataType = 0;
								  bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								/*ASSIGN_HOST_WORD_TO_BIG_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ConfSvc.numStoredConf);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);*/
								break;
							case BLE_CONFSVC_READ_CONF_LIST_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of List CCD:%d\r\n", ApplicationStateInfo_ConfSvc.storedConfList_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ConfSvc.storedConfList_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_CONFSVC_READ_CONF_DATA_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ConfSvc.storedConfList_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ConfSvc.scanConfData_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_CONFSVC_ACTIVE_CONF_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								  bleEventInfo.fileType = NNO_GET_ACTIVE_SCAN_CONFIG;
								  bleEventInfo.key = NNO_CMD_SCAN_GET_ACT_CFG;
								  bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
								  bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								  bleEventInfo.btInfo.ccdOffset = 0;
								  bleEventInfo.btInfo.serviceID = 0;
								  bleEventInfo.btInfo.connectionID = 0;
								  bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								  bleEventInfo.subfieldType = 0;
								  bleEventInfo.dataType = 0;
								  bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
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

					  if(BLEUtil_FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice) >= 0)
					  {
						  /* Since the value appears valid go ahead and*/
						  /* accept the write request.                 */
						  GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
						  DEBUG_PRINT("\r\nAttribute Length:%d",GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength);

						  switch(AttributeOffset)
						  {
						  	  case BLE_CONFSVC_READ_CONF_LIST_CCD_ATTRIBUTE_OFFSET:
						  		  if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
						  			bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

						  		  if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
						  		  {
						  			  /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
						  			  GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
						  			  /* Update the stored configuration for this device.   */
						  			  if (Notify)
						  				  ApplicationStateInfo_ConfSvc.storedConfList_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
						  			  else
						  				  ApplicationStateInfo_ConfSvc.storedConfList_CCD = 0;
						  		  }
						  		  break;
						  	  case BLE_CONFSVC_READ_CONF_DATA_CCD_ATTRIBUTE_OFFSET:
						  		  if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
						  			bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

						  		  if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
						  		  {
						  			  /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
						  			  GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
						  			  /* Update the stored configuration for this device.   */
						  			  if (Notify)
						  				  ApplicationStateInfo_ConfSvc.scanConfData_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
						  			  else
						  				  ApplicationStateInfo_ConfSvc.scanConfData_CCD = 0;
						  		  }
						  		  break;
						  	  case BLE_CONFSVC_READ_CONF_LIST_CHARACTERISTIC_ATTRIBUTE_OFFSET:
						  		  if ((ApplicationStateInfo_ConfSvc.storedConfList_CCD && GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) == 0)
						  			  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
						  		  else
						  		  {
						  			  writeVal[0] = NNO_FILE_SCAN_CONFIG_LIST;
									  Word_t temp = 0;
									  if (ApplicationStateInfo_ConfSvc.numStoredConf > 0)
										  temp = ApplicationStateInfo_ConfSvc.numStoredConf;
									  else if (READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue) > 0)
										  temp = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
									  DEBUG_PRINT("\r\ntemp:%d\r\n",temp);
									  writeVal[1] = (0x00FF & temp);
									  writeVal[2] = ((0xFF00 & temp) >> 8);
									  bleEventInfo.fileType = NNO_FILE_SCAN_CONFIG_LIST;
									  bleEventInfo.key = NNO_CMD_READ_FILE_LIST;
									  bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
									  int length = WORD_SIZE + 1;
									  bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
									  bleEventInfo.btInfo.ccdOffset = BLE_CONFSVC_READ_CONF_LIST_CCD_ATTRIBUTE_VALUE_OFFSET;
									  bleEventInfo.btInfo.serviceID = ConfSvcServiceInstance.ServiceID;
									  bleEventInfo.btInfo.connectionID = ConfSvcServiceInstance.ConnectionID;
									  bleEventInfo.subfieldType = 0;
									  bleEventInfo.dataType = 1;
									  bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
						  		  }
						  		  break;
						  	  case BLE_CONFSVC_READ_CONF_DATA_CHARACTERISTIC_ATTRIBUTE_OFFSET:
						  		  if ((ApplicationStateInfo_ConfSvc.scanConfData_CCD && GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) == 0)
						  			  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
						  		  else
						  		  {
						  			  //writeVal[0] = NNO_FILE_SCAN_CONFIG;
									  Word_t temp = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
									  DEBUG_PRINT("\r\ntemp:%d\r\n",temp);
									  writeVal[0] = (0x00FF & temp);
									  writeVal[1] = ((0xFF00 & temp) >> 8);
									  bleEventInfo.fileType = NNO_FILE_SCAN_CONFIG;
									  bleEventInfo.key = NNO_CMD_SCAN_CFG_READ;
									  bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
									  int length = WORD_SIZE;	// + 1;
									  bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
									  bleEventInfo.btInfo.ccdOffset = BLE_CONFSVC_READ_CONF_DATA_CCD_ATTRIBUTE_VALUE_OFFSET;
									  bleEventInfo.btInfo.serviceID = ConfSvcServiceInstance.ServiceID;
									  bleEventInfo.btInfo.connectionID = ConfSvcServiceInstance.ConnectionID;
									  bleEventInfo.subfieldType = 0;
									  bleEventInfo.dataType = 1;
									  bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
						  		  }
						  		  break;
						  	  case BLE_CONFSVC_ACTIVE_CONF_CHARACTERISTIC_ATTRIBUTE_OFFSET:
									if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength > WORD_SIZE)
										bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

									for (i=0; i < GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength; i++)
									{
										writeVal[i] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i];
										DEBUG_PRINT("\r\n%d:%d\r\n",i, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i]);
									}
									bleEventInfo.fileType = NNO_SET_ACTIVE_SCAN_CONFIG;
									bleEventInfo.key = NNO_CMD_SCAN_SET_ACT_CFG;
									bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE;
									length = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;
									bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
									bleEventInfo.btInfo.ccdOffset = BLE_CONFSVC_ACTIVE_CONF_CHARACTERISTIC_ATTRIBUTE_OFFSET;
									bleEventInfo.btInfo.serviceID = ConfSvcServiceInstance.ServiceID;
									bleEventInfo.btInfo.connectionID = ConfSvcServiceInstance.ConnectionID;
									bleEventInfo.subfieldType = 0;
									bleEventInfo.dataType = 0;
									bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
						  		  break;
						  }
					  }
					  else
					  {
						  DEBUG_PRINT("\r\nError - No such device connected\r\n");
					  }
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
 * Set number of configurations
 *
 * @param numConf [in] Number of configuration
 *
 * @return 0 if success; <0 on failure
 */
int GATTConfSvc_SetNumConf(unsigned short numConf)
{
	ApplicationStateInfo_ConfSvc.numStoredConf = numConf;
	return (0);
}
#endif
