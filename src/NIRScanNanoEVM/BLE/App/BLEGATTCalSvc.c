/*
 *
 * BLE GATT Profile - Calibration Service
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
#include "SS1BTGAT.h"
#include "NNOCommandDefs.h"
#include "BLECommonDefs.h"
#include "BLEGATTCalSvcDefs.h"
#include "BLEUtils.h"
#include "BLECmdHandlerLiaison.h"
#include "nano_timer.h"
#include "BLEGATTCalSvc.h"

extern unsigned short gBLESuppMTUSize;

ApplicationStateInfo_CalSvc_t ApplicationStateInfo_CalSvc;
CalSvc_ServerInstance_t CalSvcServiceInstance;

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTCalSvc_InitApplicationStateInfo()
{
	ApplicationStateInfo_CalSvc.GATTCalSvcInstanceID = 0;
	ApplicationStateInfo_CalSvc.GATTCalSvcServiceID = 0;
	ApplicationStateInfo_CalSvc.refCal_CCD = 0;
	ApplicationStateInfo_CalSvc.specCal_CCD = 0;
	ApplicationStateInfo_CalSvc.refMat_CCD = 0;
}

/**
 * Registers the service with GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTCalSvc_Register(unsigned int BluetoothStackID)
{

	int ret_val = 0;
	GATT_Attribute_Handle_Group_t ServiceHandleGroup;
	unsigned long CallbackParameter = 0;

	/* Verify that the Service is not already registered.             */
	if (!ApplicationStateInfo_CalSvc.GATTCalSvcServiceID)
	{
		/* Initialize the handle group to 0 .                          */
		ServiceHandleGroup.Starting_Handle = 0;
		ServiceHandleGroup.Ending_Handle = 0;

		/* Register the BLE GI Service                            */
		ret_val = GATT_Register_Service(BluetoothStackID,
										BLE_SERVICE_FLAGS,
										BLE_CAL_SERVICE_ATTRIBUTE_COUNT,
										(GATT_Service_Attribute_Entry_t *) BLE_CalSvc_Att_Entry,
										&ServiceHandleGroup,
										GATTCalSvc_ServerEventCallback,
										0);
		if (ret_val > 0)
		{
			/* Display success message.                                 */
			DEBUG_PRINT("Sucessfully registered GATT Cal Service, ID: %d\r\n", ret_val);

			CalSvcServiceInstance.BluetoothStackID  = BluetoothStackID;
			CalSvcServiceInstance.ServiceID         = (unsigned int)ret_val;
			CalSvcServiceInstance.CallbackParameter = CallbackParameter;
			CalSvcServiceInstance.ConnectionID 	   = 0;

			/* Save the ServiceID of the registered service.            */
			ApplicationStateInfo_CalSvc.GATTCalSvcInstanceID = (unsigned int) ret_val;

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
void GATTCalSvc_Unregister(unsigned int BluetoothStackID)
{
	GATT_Un_Register_Service(BluetoothStackID, ApplicationStateInfo_CalSvc.GATTCalSvcServiceID);
	GATTCalSvc_InitApplicationStateInfo();
}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param ConnectionID [in] New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTCalSvc_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		CalSvcServiceInstance.ConnectionID 	   = ConnectionID;

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
void BTPSAPI GATTCalSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
	Word_t	AttributeOffset = 0;
	Byte_t TempWord[2];

#ifdef BLE_USE_TEST_VALUES
	int 	i = 0;
#endif
	Boolean_t 	Notify;
	Byte_t writeVal[BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE];

	BLE_RESPONSE_INFO bleResponse;

	DEBUG_PRINT("\nCal Service Event Call back\n");

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

							case BLE_CALSVC_SPEC_CAL_COEFF_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of CCD:%d\r\n", ApplicationStateInfo_CalSvc.specCal_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_CalSvc.specCal_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_CALSVC_REF_CAL_COEFF_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of CCD:%d\r\n", ApplicationStateInfo_CalSvc.refCal_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_CalSvc.refCal_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_CALSVC_REF_CAL_MATRIX_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of CCD:%d\r\n", ApplicationStateInfo_CalSvc.refMat_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_CalSvc.refMat_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
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
							  case BLE_CALSVC_SPEC_CAL_COEFF_CCD_ATTRIBUTE_OFFSET:
								  if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								  if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								  {
									  /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									  GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									  /* Update the stored configuration for this device.   */
									  if (Notify)
									  {
										DEBUG_PRINT("\r\nNotify enabled*************\r\n");
										  ApplicationStateInfo_CalSvc.specCal_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
									  }
									  else
										  ApplicationStateInfo_CalSvc.specCal_CCD = 0;
								  }
								  break;
							  case BLE_CALSVC_SPEC_CAL_COEFF_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								  if ((ApplicationStateInfo_CalSvc.specCal_CCD && GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) == 0)
									  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
								  else
								  {
									  writeVal[0] = 0;
									  bleResponse.fileType =  NNO_FILE_SPEC_CAL_COEFF;
									  bleResponse.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
									  bleResponse.subfieldType = 0;
									  bleResponse.dataType = 1;
									  bleResponse.key = NNO_FILE_SPEC_CAL_COEFF;

									  int length = 0;
									  bleResponse.btInfo.bluetoothID = BluetoothStackID;
									  bleResponse.btInfo.ccdOffset = BLE_CALSVC_SPEC_CAL_COEFF_CCD_ATTRIBUTE_VALUE_OFFSET;
									  bleResponse.btInfo.serviceID = CalSvcServiceInstance.ServiceID;
									  bleResponse.btInfo.connectionID = CalSvcServiceInstance.ConnectionID;

									  if (bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleResponse) < 0)
										  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
								  }
								  break;
						  	  case BLE_CALSVC_REF_CAL_COEFF_CCD_ATTRIBUTE_OFFSET:
						  		  if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
						  			bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

						  		  if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
						  		  {
						  			  /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
						  			  GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
						  			  /* Update the stored configuration for this device.   */
						  			  if (Notify)
						  			  {
						  				DEBUG_PRINT("\r\nNotify enabled*************\r\n");
						  				  ApplicationStateInfo_CalSvc.refCal_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
						  			  }
						  			  else
						  				  ApplicationStateInfo_CalSvc.refCal_CCD = 0;
						  		  }
						  		  break;
						  	  case BLE_CALSVC_REF_CAL_COEFF_CHARACTERISTIC_ATTRIBUTE_OFFSET:
						  		  if ((ApplicationStateInfo_CalSvc.refCal_CCD && GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) == 0)
						  			  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
						  		  else
						  		  {
						  			  writeVal[0] = NNO_FILE_REF_CAL_DATA;
									  bleResponse.fileType =  NNO_FILE_REF_CAL_DATA;
									  bleResponse.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
									  bleResponse.subfieldType = 0;
									  bleResponse.dataType = 1;
									  bleResponse.key = NNO_CMD_FILE_GET_READSIZE;

									  int length = 1;
									  bleResponse.btInfo.bluetoothID = BluetoothStackID;
									  bleResponse.btInfo.ccdOffset = BLE_CALSVC_REF_CAL_COEFF_CCD_ATTRIBUTE_VALUE_OFFSET;
									  bleResponse.btInfo.serviceID = CalSvcServiceInstance.ServiceID;
									  bleResponse.btInfo.connectionID = CalSvcServiceInstance.ConnectionID;

									  if (bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleResponse) < 0)
										  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
						  		  }
						  		  break;
						  	  case BLE_CALSVC_REF_CAL_MATRIX_CCD_ATTRIBUTE_OFFSET:
						  		  if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
						  			bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

						  		  if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
						  		  {
						  			  /* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
						  			  GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
						  			  /* Update the stored configuration for this device.   */
						  			  if (Notify)
						  			  {
						  				DEBUG_PRINT("\r\nNotify enabled*************\r\n");
						  				  ApplicationStateInfo_CalSvc.refMat_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
						  			  }
						  			  else
						  				  ApplicationStateInfo_CalSvc.refMat_CCD = 0;
						  		  }
						  		  break;
						  	  case BLE_CALSVC_REF_CAL_MATRIX_CHARACTERISTIC_ATTRIBUTE_OFFSET:
						  		  if ((ApplicationStateInfo_CalSvc.refMat_CCD && GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) == 0)
						  			  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
						  		  else
						  		  {
						  			  writeVal[0] = NNO_FILE_REF_CAL_MATRIX;
									  bleResponse.fileType =  NNO_FILE_REF_CAL_MATRIX;
									  bleResponse.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
									  bleResponse.subfieldType = 0;
									  bleResponse.dataType = 1;
									  bleResponse.key = NNO_CMD_FILE_GET_READSIZE;

									  int length = 1;
									  bleResponse.btInfo.bluetoothID = BluetoothStackID;
									  bleResponse.btInfo.ccdOffset = BLE_CALSVC_REF_CAL_MATRIX_CCD_ATTRIBUTE_VALUE_OFFSET;
									  bleResponse.btInfo.serviceID = CalSvcServiceInstance.ServiceID;
									  bleResponse.btInfo.connectionID = CalSvcServiceInstance.ConnectionID;

									  if (bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleResponse) < 0)
										  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_CCCD_IMPROPERLY_CONFIGURED);
						  		  }
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
#endif
