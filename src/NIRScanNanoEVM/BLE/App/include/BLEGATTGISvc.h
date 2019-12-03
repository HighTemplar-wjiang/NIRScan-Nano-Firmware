/*
 * BLE GATT Profile - General Information Service header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTGISVC_H_
#define BLEGATTGISVC_H_

/**
 * @brief General information service application information
 */
typedef struct _tagApplicationStateInfo_GISvc_t
{
   unsigned int				GATTGISvcInstanceID;
   unsigned int				GATTGISvcServiceID;
   Word_t					Temp_Client_Configuration_Descriptor;
   Word_t					Hum_Client_Configuration_Descriptor;
   int16_t					TempThreshold;
   uint16_t					HumThreshold;
   Word_t					DeviceStatus;
   Word_t					DeviceStatus_Client_Configuration_Descriptor;
   Word_t					ErrorStatus;
   Word_t					ErrorStatus_Client_Configuration_Descriptor;
   Word_t					NumHoursOfUse;
   Word_t					NumBattRecharge;
   Word_t					NumLampHours;
} ApplicationStateInfo_GISvc_t;

/**
 * @brief General information server instance
 */
typedef struct _tagGISvc_ServerInstance_t {
	unsigned int BluetoothStackID;
	unsigned int ServiceID;
	unsigned int ConnectionID;
	unsigned long CallbackParameter;
} GISvc_ServerInstance_t;

/**
 * Function definitions
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTGISvc_InitApplicationStateInfo();

/**
 * @brief Register the service with GATT server
 */
int GATTGISvc_Register(unsigned int BluetoothStackID);

/**
 * @brief Unregister service from GATT server
 */
void GATTGISvc_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int GATTGISvc_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
void BTPSAPI GATTGISvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);

/**
 * @name Functions for setting all system parameters in General information service
 */
//@{
/**
 * @brief To set temperature in service
 */
int GATTGISvc_SetTemp(short temp, bool sendNotification);

/**
 * @brief To set humidity in service
 */
int GATTGISvc_SetHum(unsigned short hum, bool sendNotification);

/**
 * @brief To set device status in service
 */
int GATTGISvc_SetDevStat(unsigned short devStat);

/**
 * @brief To set error status in service
 */
int GATTGISvc_SetErrStat(unsigned short errStat);

/**
 * @brief To set hours of use in service
 */
int GATTGISvc_SetNumHoursOfUse(unsigned short numHrs);

/**
 * @brief To set number of battery recharge cycles in service
 */
int GATTGISvc_SetBattRecharge(unsigned short numBattRechg);

/**
 * @brief To set lamp hours in service
 */
int GATTGISvc_SetLampHours(unsigned short lampHrs);
//@}

#ifdef __cplusplus
}
#endif
#endif /* BLEGATTGISVC_H_ */
