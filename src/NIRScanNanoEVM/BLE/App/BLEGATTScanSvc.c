/*
 *
 * BLE GATT Profile - Scan (Data) Service
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
#include "nano_timer.h"
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                    */
#include "NNOCommandDefs.h"
#include "BLECommonDefs.h"
#include "dlpspec_scan.h"
#include "BLEGATTScanSvcDefs.h"
#include "BLEUtils.h"
#include "BLECmdHandlerLiaison.h"
#include "BLENotificationHandler.h"
#include "BLEGATTScanSvc.h"

extern unsigned short gBLESuppMTUSize;

ApplicationStateInfo_ScanSvc_t ApplicationStateInfo_ScanSvc;
ScanSvc_ServerInstance_t ScanSvcServiceInstance;

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTScanSvc_InitApplicationStateInfo()
{
	int i = 0;

	ApplicationStateInfo_ScanSvc.GATTScanSvcInstanceID = 0;
	ApplicationStateInfo_ScanSvc.GATTScanSvcServiceID = 0;
	for(i=0;i<(SCAN_NAME_LEN-2);i++)
		ApplicationStateInfo_ScanSvc.scanNameStub[i] = 0;

	ApplicationStateInfo_ScanSvc.scanList_CCD = 0;
	ApplicationStateInfo_ScanSvc.startScan_CCD = 0;
	ApplicationStateInfo_ScanSvc.clearScan_CCD = 0;
	ApplicationStateInfo_ScanSvc.scanName_CCD = 0;
	ApplicationStateInfo_ScanSvc.scanType_CCD = 0;
	ApplicationStateInfo_ScanSvc.scanTime_CCD = 0;
	ApplicationStateInfo_ScanSvc.scanBlobVer_CCD = 0;
	ApplicationStateInfo_ScanSvc.scanData_CCD = 0;

	ApplicationStateInfo_ScanSvc.numScans = 0;
}

/**
 * Registers the service with GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTScanSvc_Register(unsigned int BluetoothStackID)
{

	int ret_val = 0;
	GATT_Attribute_Handle_Group_t ServiceHandleGroup;
	unsigned long CallbackParameter = 0;

	/* Verify that the Service is not already registered.             */
	if (!ApplicationStateInfo_ScanSvc.GATTScanSvcServiceID)
	{
		/* Initialize the handle group to 0 .                          */
		ServiceHandleGroup.Starting_Handle = 0;
		ServiceHandleGroup.Ending_Handle = BLE_SCAN_SERVICE_ATTRIBUTE_COUNT;

		/* Register the BLE Scan Service                            */
		ret_val = GATT_Register_Service(BluetoothStackID,
										BLE_SERVICE_FLAGS,
										BLE_SCAN_SERVICE_ATTRIBUTE_COUNT,
										(GATT_Service_Attribute_Entry_t *) BLE_ScanSvc_Att_Entry,
										&ServiceHandleGroup,
										GATTScanSvc_ServerEventCallback,
										0);
		if (ret_val > 0)
		{
			/* Display success message.                                 */
			DEBUG_PRINT("Sucessfully registered GATT Scan Service, ID: %d\r\n", ret_val);

			ScanSvcServiceInstance.BluetoothStackID  = BluetoothStackID;
			ScanSvcServiceInstance.ServiceID         = (unsigned int)ret_val;
			ScanSvcServiceInstance.CallbackParameter = CallbackParameter;
			ScanSvcServiceInstance.ConnectionID 	   = 0;

			/* Save the ServiceID of the registered service.            */
			ApplicationStateInfo_ScanSvc.GATTScanSvcInstanceID = (unsigned int) ret_val;

			/* Return success to the caller.                            */
			ret_val = 0;
		}
		else
		{
			DEBUG_PRINT("Failed to register Scan Service; GATT Server returned error:%d\r\n",ret_val);
		}
	}
	else
	{
		DEBUG_PRINT("GATT Scan Service already registered.\r\n");
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
void GATTScanSvc_Unregister(unsigned int BluetoothStackID)
{
	GATT_Un_Register_Service(BluetoothStackID, ApplicationStateInfo_ScanSvc.GATTScanSvcServiceID);
	GATTScanSvc_InitApplicationStateInfo();
}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param ConnectionID [in] New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTScanSvc_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		ScanSvcServiceInstance.ConnectionID 	   = ConnectionID;

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
void BTPSAPI GATTScanSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
	Word_t	AttributeOffset = 0;
	Byte_t TempWord[2];
	uint32_t TempDWord;
	Boolean_t 	Notify;
	Byte_t writeVal[BLE_MAX_PACKET_SIZE];
	int i = 0;
	int cnt = 0;
	int length = 0;
	BLE_RESPONSE_INFO bleEventInfo;
	BLE_NOTIFY_INFO notifyInfo;

	// To avoid static code analyser error
	notifyInfo.data = NULL;
	notifyInfo.data_changed = false;
	notifyInfo.data_length = 0;

	DEBUG_PRINT("\nScan Service Event Call back\n");

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
							case BLE_SCANSVC_NUM_STORED_SCANS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								writeVal[0] = NNO_FILE_SCAN_LIST;
								writeVal[1] = BLE_LIST_RETURN_TYPE_NUM_ENTRIES;
								length = 2;
								bleEventInfo.fileType = NNO_FILE_SCAN_LIST;
								bleEventInfo.key = NNO_CMD_READ_FILE_LIST_SIZE;
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
							case BLE_SCANSVC_READ_SCAN_LIST_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.scanList_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.scanList_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_START_SCAN_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.startScan_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.startScan_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_CLEAR_SCAN_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.clearScan_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.clearScan_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_READ_SCAN_NAME_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.scanName_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.scanName_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_READ_SCAN_TYPE_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.scanType_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.scanType_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_READ_SCAN_TIME_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.scanTime_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.scanTime_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_READ_SCAN_BLOB_VER_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.scanBlobVer_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.scanBlobVer_CCD);
								GATT_Read_Response(BluetoothStackID,
												   GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID,
												   WORD_SIZE,
												   &TempWord[0]);
								break;
							case BLE_SCANSVC_READ_SCAN_DATA_CCD_ATTRIBUTE_OFFSET:
								DEBUG_PRINT("\r\nValue of Data CCD:%d\r\n", ApplicationStateInfo_ScanSvc.scanData_CCD);
								ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&TempWord, ApplicationStateInfo_ScanSvc.scanData_CCD);
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

						  if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength > BLE_MAX_PACKET_SIZE)	// To satisfy static analyzer!
						  {
							  bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
							  return;
						  }

						  switch(AttributeOffset)
						  {
							case BLE_SCANSVC_WRITE_SCAN_NAME_STUB_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength > SCAN_NAME_TAG_SIZE)	// To satisfy static analyzer!
								{
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
									return;
								}

								bleEventInfo.fileType = NNO_SET_FILE_NAME_TAG;
								bleEventInfo.key = NNO_CMD_WRITE_SCAN_NAME_TAG;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = 0;
								bleEventInfo.btInfo.serviceID = 0;
								bleEventInfo.btInfo.connectionID = 0;
								bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								//length = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength + 1;
								for(i=0;i<SCAN_NAME_TAG_SIZE;i++)
									writeVal[i] = 0;
								cnt = 0;	//just in case
								writeVal[cnt++] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;

								for (i = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength-1; i >= 0; i--)
								{
									if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i] == 0)
										continue;

									if (i < SCAN_NAME_TAG_SIZE)
										writeVal[cnt++] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i];
								}

								length = cnt;	// fix for static analyzer reported issue
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_START_SCAN_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
									{
										ApplicationStateInfo_ScanSvc.startScan_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
										notifyInfo.btInfo.bluetoothID = BluetoothStackID;
										notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
										notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID;
										notifyInfo.btInfo.ccdOffset = BLE_SCANSVC_START_SCAN_CCD_ATTRIBUTE_VALUE_OFFSET;
										notifyInfo.type = BLE_NOTIFY_SCAN_STATUS;

										if (0 != bleNotificationHandler_registerNotification(notifyInfo))
											DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
									}
									else
									{
										ApplicationStateInfo_ScanSvc.startScan_CCD = 0;

										if (0 != bleNotificationHandler_deregisterNotification(BLE_NOTIFY_SCAN_STATUS))
											DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
									}
								}
								break;
							case BLE_SCANSVC_CLEAR_SCAN_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
									{
										ApplicationStateInfo_ScanSvc.clearScan_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
										notifyInfo.btInfo.bluetoothID = BluetoothStackID;
										notifyInfo.btInfo.serviceID = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ServiceID;
										notifyInfo.btInfo.connectionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID;
										notifyInfo.btInfo.ccdOffset = BLE_SCANSVC_CLEAR_SCAN_CCD_ATTRIBUTE_VALUE_OFFSET;
										notifyInfo.type = BLE_NOTIFY_CLEAR_SCAN_STATUS;

										if (0 != bleNotificationHandler_registerNotification(notifyInfo))
											DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);

									}
									else
									{
										ApplicationStateInfo_ScanSvc.clearScan_CCD = 0;

										if (0 != bleNotificationHandler_deregisterNotification(BLE_NOTIFY_CLEAR_SCAN_STATUS))
											DEBUG_PRINT("Notification registration failed, %d",NNO_BLE_NOTIFICATION_PROCESSING_FAILED);
									}
								}
								break;
							case BLE_SCANSVC_READ_SCAN_NAME_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
										ApplicationStateInfo_ScanSvc.scanName_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
									else
										ApplicationStateInfo_ScanSvc.scanName_CCD = 0;
								}
								break;
							case BLE_SCANSVC_READ_SCAN_TYPE_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
										ApplicationStateInfo_ScanSvc.scanType_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
									else
										ApplicationStateInfo_ScanSvc.scanType_CCD = 0;
								}
								break;
							case BLE_SCANSVC_READ_SCAN_TIME_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
										ApplicationStateInfo_ScanSvc.scanTime_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
									else
										ApplicationStateInfo_ScanSvc.scanTime_CCD = 0;
								}
								break;
							case BLE_SCANSVC_READ_SCAN_BLOB_VER_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
										ApplicationStateInfo_ScanSvc.scanBlobVer_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
									else
										ApplicationStateInfo_ScanSvc.scanBlobVer_CCD = 0;
								}
								break;
							case BLE_SCANSVC_READ_SCAN_DATA_CCD_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != WORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								if (!BLEUtil_DecodeCharConfigDesc(WORD_SIZE, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue, &Notify))
								{
									/* Go ahead and accept the write request since we have decoded CCD Value successfully.      */
									GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);
									/* Update the stored configuration for this device.   */
									if (Notify)
										ApplicationStateInfo_ScanSvc.scanData_CCD |= GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
									else
										ApplicationStateInfo_ScanSvc.scanData_CCD = 0;
								}
								break;
							case BLE_SCANSVC_READ_SCAN_LIST_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								writeVal[0] = NNO_FILE_SCAN_LIST;
								writeVal[1] = BLE_LIST_RETURN_TYPE_BYTE;
								length = 2;
								bleEventInfo.fileType = NNO_FILE_SCAN_LIST;
								bleEventInfo.key = NNO_CMD_READ_FILE_LIST_SIZE;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = BLE_SCANSVC_READ_SCAN_LIST_CCD_ATTRIBUTE_VALUE_OFFSET;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_READ_SCAN_NAME_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
								TempDWord = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
								writeVal[0] = NNO_FILE_SCAN_DATA;
								memcpy(&writeVal[1],&TempDWord,DWORD_SIZE);
								writeVal[5] = (0xff & BLE_SCAN_DATA_FIELD_NAME);
								bleEventInfo.fileType = NNO_FILE_SCAN_DATA;
								bleEventInfo.key = NNO_CMD_FILE_GET_READSIZE;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
								length = DWORD_SIZE + 2;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = BLE_SCANSVC_READ_SCAN_NAME_CCD_ATTRIBUTE_VALUE_OFFSET;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = BLE_SCAN_DATA_FIELD_NAME;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_READ_SCAN_TYPE_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								TempDWord = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
								writeVal[0] = NNO_FILE_SCAN_DATA;
								memcpy(&writeVal[1],&TempDWord,DWORD_SIZE);
								writeVal[5] = (0xff & BLE_SCAN_DATA_FIELD_TYPE);
								bleEventInfo.fileType = NNO_FILE_SCAN_DATA;
								bleEventInfo.key = NNO_CMD_FILE_GET_READSIZE;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
								length = DWORD_SIZE + 2;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = BLE_SCANSVC_READ_SCAN_TYPE_CCD_ATTRIBUTE_VALUE_OFFSET;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = BLE_SCAN_DATA_FIELD_TYPE;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_READ_SCAN_TIME_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								TempDWord = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
								writeVal[0] = NNO_FILE_SCAN_DATA;
								memcpy(&writeVal[1],&TempDWord,DWORD_SIZE);
								writeVal[5] = (0xff & BLE_SCAN_DATA_FIELD_TIME);
								bleEventInfo.fileType = NNO_FILE_SCAN_DATA;
								bleEventInfo.key = NNO_CMD_FILE_GET_READSIZE;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
								length = DWORD_SIZE + 2;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = BLE_SCANSVC_READ_SCAN_TIME_CCD_ATTRIBUTE_VALUE_OFFSET;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = BLE_SCAN_DATA_FIELD_TIME;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_READ_SCAN_BLOB_VER_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								TempDWord = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
								writeVal[0] = NNO_FILE_SCAN_DATA;
								memcpy(&writeVal[1],&TempDWord,DWORD_SIZE);
								writeVal[5] = (0xff & BLE_SCAN_DATA_FIELD_BLOB_VER);
								bleEventInfo.fileType = NNO_FILE_SCAN_DATA;
								bleEventInfo.key = NNO_CMD_FILE_GET_READSIZE;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
								length = DWORD_SIZE + 2;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = BLE_SCANSVC_READ_SCAN_BLOB_VER_CCD_ATTRIBUTE_VALUE_OFFSET;
								bleEventInfo.btInfo.transactionID = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = BLE_SCAN_DATA_FIELD_BLOB_VER;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_READ_SCAN_DATA_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength != DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								TempDWord = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
								writeVal[0] = NNO_FILE_SCAN_DATA;
								memcpy(&writeVal[1],&TempDWord,DWORD_SIZE);
								writeVal[5] = (0xff & BLE_SCAN_DATA_FIELD_BLOB);
								bleEventInfo.fileType = NNO_FILE_SCAN_DATA;
								bleEventInfo.key = NNO_CMD_FILE_GET_READSIZE;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
								length = DWORD_SIZE + 2;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = BLE_SCANSVC_READ_SCAN_DATA_CCD_ATTRIBUTE_VALUE_OFFSET;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = BLE_SCAN_DATA_FIELD_BLOB;
								bleEventInfo.dataType = 1;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_START_SCAN_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength > DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								ApplicationStateInfo_ScanSvc.numScans++;

								for (i=0; i < GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength; i++)
								{
									writeVal[i] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i];
									DEBUG_PRINT("\r\n%d:%d\r\n",i, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i]);
								}
								bleEventInfo.fileType = NNO_START_SCAN;
								bleEventInfo.key = NNO_CMD_PERFORM_SCAN;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE;
								length = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = 0;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
								bleEventInfo.subfieldType = 0;
								bleEventInfo.dataType = 0;
								bleCmdHandlerLiason_relayCmd(&writeVal[0],length,bleEventInfo);
								break;
							case BLE_SCANSVC_CLEAR_SCAN_CHARACTERISTIC_ATTRIBUTE_OFFSET:
								if (GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength > DWORD_SIZE)
									bleGATTErrorResponse(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

								for (i=0; i < GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength; i++)
									writeVal[i] = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue[i];
								bleEventInfo.fileType = NNO_CLEAR_SCAN;
								bleEventInfo.key = NNO_CMD_DEL_SCAN_FILE_SD;
								bleEventInfo.cmdType = BLE_COMMAND_TYPE_WRITE;
								length = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;
								bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
								bleEventInfo.btInfo.ccdOffset = 0;
								bleEventInfo.btInfo.serviceID = ScanSvcServiceInstance.ServiceID;
								bleEventInfo.btInfo.connectionID = ScanSvcServiceInstance.ConnectionID;
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
#endif
