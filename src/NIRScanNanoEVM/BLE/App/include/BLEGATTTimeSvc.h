/*
 * BLE GATT Profile - Time Service header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTTIMESVC_H_
#define BLEGATTTIMESVC_H_

/**
 * @brief Time service application information
 */
typedef struct _tagApplicationStateInfo_TimeSvc_t
{
   unsigned int		GATTTimeSvcInstanceID;
   unsigned int		GATTTimeSvcServiceID;
   Byte_t			year;
   Byte_t			month;
   Byte_t			day;
   Byte_t			dow;
   Byte_t			hour;
   Byte_t			minute;
   Byte_t			second;
} ApplicationStateInfo_TimeSvc_t;

/**
 * @brief Time Service Instance Block
 * 
 * This structure contains all information associated with
 * a specific Bluetooth Stack ID (member is present in this structure)
 */
typedef struct _tagTimeSvc_ServerInstance_t {
	unsigned int BluetoothStackID;
	unsigned int ServiceID;
	unsigned int ConnectionID;
	unsigned long CallbackParameter;
} TimeSvc_ServerInstance_t;

/**
 * function definitions
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTTimeSvc_InitApplicationStateInfo();

/**
 * @brief Register the service with GATT server
 */
int GATTTimeSvc_Register(unsigned int BluetoothStackID);

/**
 * @brief Unregister service from GATT server
 */
void GATTTimeSvc_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int GATTTimeSvc_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
void BTPSAPI GATTTimeSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);

#ifdef __cplusplus
}
#endif
#endif /* BLEGATTTIMESVC_H_ */
