/*
 * BLE GATT Profile - Command Service header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTCMDSVC_H_
#define BLEGATTCMDSVC_H_

/**
 * @brief Command service device information
 */
typedef struct _tagCmdSvc_DeviceInfo_t
{
   Word_t					Client_Configuration_Descriptor;
   Byte_t					InternalCommand[BLE_MAX_COMMAND_SIZE];
   uint8_t					Data_size;
} CmdSvc_DeviceInfo_t;

/**
 * @brief Command service state information structure
 */
typedef struct _tagApplicationStateInfo_CmdSvc_t
{
   unsigned int			GATTCmdSvcInstanceID;
   unsigned int			GATTCmdSvcServiceID;
} ApplicationStateInfo_CmdSvc_t;

/** command service server event callback function type definition				*/
typedef void (BTPSAPI *CmdSvc_Event_Callback_t)(unsigned int BluetoothStackID,
												unsigned long CallbackParameter);

/**
 * @brief Command service server instance structure
 */
typedef struct _tagCmdSvc_ServerInstance_t {
	unsigned int BluetoothStackID;
	unsigned int ServiceID;
	unsigned int ConnectionID;
	CmdSvc_Event_Callback_t EventCallback;
	unsigned long CallbackParameter;
} CmdSvc_ServerInstance_t;

/**
 * Function definitions
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTCmdSvc_InitApplicationStateInfo();

/**
 * @brief Unregister service from GATT server
 */
int GATTCmdSvc_Register(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
void GATTCmdSvc_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int GATTCmdSvc_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
void BTPSAPI GATTCmdSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);


#ifdef __cplusplus
}
#endif
#endif /* BLEGATTCMDSVC_H_ */
