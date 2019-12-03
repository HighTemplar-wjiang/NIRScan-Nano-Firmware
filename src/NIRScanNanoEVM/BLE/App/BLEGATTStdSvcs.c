/*
 *
 * BLE GATT profile - Standard v4.0 services
 * Handlers for battery and device information services
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include "uartstdio.h"
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include "SS1BTDIS.h"
#include "DIS.h"
#include "SS1BTBAS.h"
#include "NNOCommandDefs.h"
#include "BAS.h"
#include "version.h"
#include "BLECommonDefs.h"
#include "BLEUtils.h"
#include "BLECmdHandlerLiaison.h"
#include "dlpspec_version.h"
#include "nano_eeprom.h"
#include "nnoStatus.h"
#include "BLEGATTStdSvcs.h"

static ApplicationStateInfo_StdSvcs_t StdSvcsInstance;

/******************* DEVICE INFORMATION SERVICE **********************/
char	DISManufacturerName[] = "Texas Instruments\n";
char	DISModelNumber[EEPROM_MODEL_NAME_SIZE];  /* If no Model Number is set =		"DLPNIRNANOEVM\n"; */
char	DISFWRevNumber[MAX_VERSION_STR] = 		"";
char	DISSWRevNumber[] = 		"x.y.z";

/******************* BATTERY SERVICE ********************************/
char	BASBatteryLevel = 255;		//Initialized to 100%

// Local initialization functions
int InitDISService(unsigned int BluetoothStackID);
int InitBASService(unsigned int BluetoothStackID);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI BAS_Event_Callback(unsigned int BluetoothStackID, BAS_Event_Data_t *BAS_Event_Data, unsigned long CallbackParameter);

/**
 * Initializes application state information
 *
 * @return None
 */
void GATTStdSvcs_InitApplicationStateInfo()
{
	StdSvcsInstance.ConnectionID = 0;
	StdSvcsInstance.DISInstanceID = 0;
	StdSvcsInstance.BASInstanceID = 0;
	StdSvcsInstance.BatteryLevel = 0;
	StdSvcsInstance.Battery_CCD = 0;

	return;
}

/**
 * Registers the service with GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int  GATTStdSvcs_Register(unsigned int BluetoothStackID)
{
	int ret_val = 0;

	ret_val = InitBASService(BluetoothStackID);
	if (0 == ret_val)
		ret_val = InitDISService(BluetoothStackID);

	return (ret_val);
}

/**
 * Unregister the service from GATT server
 *
 * @param BluetoothStackID [in] Bluetooth stack instance ID
 *
 * @return None
 */
int GATTStdSvcs_Unregister(unsigned int BluetoothStackID)
{
	int ret_val = 0;

	ret_val = BAS_Cleanup_Service(BluetoothStackID, StdSvcsInstance.BASInstanceID);
	if (0 == ret_val)
		ret_val = DIS_Cleanup_Service(BluetoothStackID, StdSvcsInstance.DISInstanceID);

	return ret_val;
}

/**
 * Updates connection ID. This function is invoked after a new
 * connection is setup
 *
 * @param ConnectionID [in] New connection instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int GATTStdSvcs_UpdateConnectionID(unsigned int ConnectionID)
{
	if (ConnectionID)
	{
		StdSvcsInstance.ConnectionID = ConnectionID;
		return (PASS);
	}
	else
		return BLE_ERROR_INVALID_PARAMETER;
}

/**
 * Initialize Battery Service
 *
 * @param BluetoothStackID [in] bluetooth Stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int InitBASService(unsigned int BluetoothStackID)
{
	int ret_val = 0;
	unsigned int BASServiceID = 0;

	// Register Device Information Service
	if (!StdSvcsInstance.BASInstanceID)
	{
		ret_val = InitializeBASModule();
		if (ret_val != 0)	// Non-zero return value means success as per DIS.h
		{
			BASServiceID = 1;//To satisfy the check in the DIS Init Service routine. This will be overwritten if successful anyways
			ret_val = BAS_Initialize_Service(BluetoothStackID, BAS_Event_Callback, 0, &BASServiceID);
			if (ret_val > 0)
			{
				/* Display success message.                                 */
				DEBUG_PRINT("Sucessfully Registered BAS Service, ID: %d\r\n",BASServiceID);

				/* Save the ServiceID of the registered service.            */
				StdSvcsInstance.BASInstanceID = (unsigned int) ret_val;

				/* Return success to the caller.                            */
				ret_val = 0;
			}
			else
			{
				DEBUG_PRINT("BAS Service already registered.\r\n");
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
				ret_val = APPLICATION_ERROR_GATT_SERVICE_EXISTS;
			}
		}
		else
		{
			DEBUG_PRINT("\r\n BAS Module was not initialized correctly");
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
			ret_val = 1;	//Dummy value so that the caller knows that there was a failure
		}
	}

	return ret_val;
}

/**
 * Initialize Device Information Service
 *
 * @param BluetoothStackID [in] bluetooth Stack instance ID
 *
 * @return 0 on success; <0 on failure.
 */
int InitDISService(unsigned int BluetoothStackID)
{
	int ret_val = 0;
	unsigned int DISServiceID = 0;
	uint8_t sNo[EEPROM_SERIAL_NUMBER_SIZE];

	// Register Device Information Service
	if (!StdSvcsInstance.DISInstanceID)
	{
		ret_val = InitializeDISModule();
		if (ret_val != 0)	// Non-zero return value means success as per DIS.h
		{
			DISServiceID = 1;//To satisfy the check in the DIS Init Service routine. This will be overwritten if successful anyways
			ret_val = DIS_Initialize_Service(BluetoothStackID, &DISServiceID);
			if (ret_val > 0)
			{
				/* Display success message.                                 */
				DEBUG_PRINT("Sucessfully Registered DIS Service, ID:%d\r\n", DISServiceID);

				/* Save the ServiceID of the registered service.            */
				StdSvcsInstance.DISInstanceID = (unsigned int) ret_val;

				/* Return success to the caller.                            */
				ret_val = 0;
			}
			else
			{
				DEBUG_PRINT("DIS Service already registered.\r\n");
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
				ret_val = APPLICATION_ERROR_GATT_SERVICE_EXISTS;
				DISServiceID = 0;	//reseting it, just in case
			}
		}
		else
		{
			DEBUG_PRINT("\r\n DIS Module not initilized correctly");
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
			ret_val = 1;	//Dummy value so that the caller knows that there was a failure
		}
	}

	if (!ret_val)
	{
		Nano_eeprom_GetDeviceModelName( (uint8_t*) DISModelNumber );
		if ( DISModelNumber[0] == 0x0 ) {
			strncpy(DISModelNumber, "DLPNIRNANOEVM\n", EEPROM_MODEL_NAME_SIZE);
		 }

		
		/* DIS Characteritics Initialization */
		ret_val = DIS_Set_Manufacturer_Name(BluetoothStackID,StdSvcsInstance.DISInstanceID,DISManufacturerName);
		if (!ret_val)
		{
			ret_val = DIS_Set_Model_Number(BluetoothStackID,StdSvcsInstance.DISInstanceID,DISModelNumber);
			if (!ret_val)
			{
				if (0 == Nano_eeprom_GetDeviceSerialNumber(sNo))
					ret_val = DIS_Set_Serial_Number(BluetoothStackID,StdSvcsInstance.DISInstanceID,(char *)sNo);

				if (!ret_val)
				{
					ret_val = DIS_Set_Hardware_Revision(BluetoothStackID,StdSvcsInstance.DISInstanceID,TIVA_HW_REV);
					if (!ret_val)
					{
						snprintf(DISFWRevNumber, MAX_VERSION_STR, "%d.%d.%d", TIVA_VERSION_MAJOR, TIVA_VERSION_MINOR, TIVA_VERSION_BUILD);
						ret_val = DIS_Set_Firmware_Revision(BluetoothStackID,StdSvcsInstance.DISInstanceID,DISFWRevNumber);
						if (!ret_val)
						{
							snprintf(DISSWRevNumber, MAX_VERSION_STR, "%d.%d.%d", DLPSPEC_VERSION_MAJOR, DLPSPEC_VERSION_MINOR, DLPSPEC_VERSION_BUILD);
							ret_val = DIS_Set_Software_Revision(BluetoothStackID,StdSvcsInstance.DISInstanceID,DISSWRevNumber);
							if (ret_val > 0)
							{
								DEBUG_PRINT("DIS Set SW Rev Number returned error, code: %d\r\n",ret_val);
								nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
							}
						}
						else
						{
							DEBUG_PRINT("DIS Set FW Rev Number returned error, code: %d\r\n",ret_val);
							nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
						}
					}
					else
					{
						DEBUG_PRINT("DIS Set HW Rev Number returned error, code: %d\r\n",ret_val);
						nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
					}
				}
				else
				{
					DEBUG_PRINT("DIS Set Serial returned error, code: %d\r\n",ret_val);
					nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
				}
			}
			else
			{
				DEBUG_PRINT("DIS Set Model returned error, code: %d\r\n",ret_val);
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
			}
		}
		else
		{
			DEBUG_PRINT("DIS Set Manufacturer returned error, code: %d\r\n",ret_val);
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
		}
	}

	return ret_val;
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
static void BTPSAPI BAS_Event_Callback(unsigned int BluetoothStackID, BAS_Event_Data_t *BAS_Event_Data, unsigned long CallbackParameter)
{
	int           Result;
	BLE_RESPONSE_INFO bleEventInfo;

	if (BAS_Event_Data)
		DEBUG_PRINT("\r\nBattery Service Event Call back, Event: %d\r\n",BAS_Event_Data->Event_Data_Type);

	/* Verify that all parameters to this callback are Semi-Valid.       */
	if((BluetoothStackID) && (BAS_Event_Data))
	{
		/* Determine the Battery Service Event that occurred.          */
		switch(BAS_Event_Data->Event_Data_Type)
		{
			case etBAS_Server_Read_Client_Configuration_Request:
				if((BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data) && (BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data->ClientConfigurationType == ctBatteryLevel))
				{
					DEBUG_PRINT("Battery Client Configuration Read\r\n" );

					Result = BAS_Read_Client_Configuration_Response(BluetoothStackID, StdSvcsInstance.BASInstanceID, BAS_Event_Data->Event_Data.BAS_Read_Client_Configuration_Data->TransactionID, StdSvcsInstance.Battery_CCD);
					if(0 != Result)
						DEBUG_PRINT("Error - BAS_Read_Client_Configuration_Response() %d.\r\n", Result);
				}
				break;
			case etBAS_Server_Client_Configuration_Update:
				if((BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data) && (BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->ClientConfigurationType == ctBatteryLevel))
				{
					DEBUG_PRINT("Battery Client Configuration Update: %s.\r\n", (BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->Notify?"ENABLED":"DISABLED"));

					/* Update the stored configuration for this device.   */
					if(BAS_Event_Data->Event_Data.BAS_Client_Configuration_Update_Data->Notify)
						StdSvcsInstance.Battery_CCD = GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE;
					else
						StdSvcsInstance.Battery_CCD = 0;
				}
				break;
			case etBAS_Server_Read_Battery_Level_Request:
				if(BAS_Event_Data->Event_Data.BAS_Read_Battery_Level_Data)
				{
					bleEventInfo.fileType = NNO_READ_BATT;
					bleEventInfo.key = NNO_CMD_READ_BATT_VOLT;
					bleEventInfo.cmdType = BLE_COMMAND_TYPE_READ_DELAY_RESPONSE;
					bleEventInfo.btInfo.bluetoothID = BluetoothStackID;
					bleEventInfo.btInfo.transactionID = BAS_Event_Data->Event_Data.BAS_Read_Battery_Level_Data->TransactionID;
					bleEventInfo.btInfo.ccdOffset = 0;
					bleEventInfo.btInfo.serviceID = 0;
					bleEventInfo.btInfo.connectionID = 0;
					bleEventInfo.subfieldType = 0;
					bleEventInfo.dataType = 0;
					bleCmdHandlerLiason_relayCmd(NULL,0,bleEventInfo);
				}
				break;
		}
	}
	else
	{
		/* There was an error with one or more of the input parameters.   */
		DEBUG_PRINT("\r\n");

		DEBUG_PRINT("Battery Service Callback Data: Event_Data = NULL.\r\n");
	}
}

/**
 * To update Battery level and send notification to client
 *
 * @param BluetoothStackID [in] bluetooth Stack instance ID
 * @param BatteryLevel [in] Measured Battery Level
 *
 * @return 0 on success; <0 on failure
 */
int GATTStdSvcs_UpdateBatteryLevel(unsigned int BluetoothStackID, Byte_t BatteryLevel)
{
	StdSvcsInstance.BatteryLevel = BatteryLevel;
	int retval = 0;

	/* Notify if enabled */
	if ((StdSvcsInstance.Battery_CCD || GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE) > 0)
		retval = BAS_Notify_Battery_Level(BluetoothStackID, StdSvcsInstance.BASInstanceID, StdSvcsInstance.ConnectionID, BatteryLevel);

	return retval;
}
#endif
