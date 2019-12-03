/*
 * BLE GATT Profile - (Scan) Configuration Service header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTCONFSVC_H_
#define BLEGATTCONFSVC_H_

/**
 * @brief Configuration service application state information
 */
typedef struct _tagApplicationStateInfo_ConfSvc_t
{
   unsigned int		GATTConfSvcInstanceID;
   unsigned int		GATTConfSvcServiceID;
   Word_t 			numStoredConf;
   Word_t			scanConfIdx;
   Word_t			storedConfList_CCD;
   Word_t			scanConfData_CCD;
} ApplicationStateInfo_ConfSvc_t;

/**
 * @brief Configuration service service instance information
 */
typedef struct _tagConfSvc_ServerInstance_t {
	unsigned int BluetoothStackID;
	unsigned int ServiceID;
	unsigned int ConnectionID;
	unsigned long CallbackParameter;
} ConfSvc_ServerInstance_t;

/**
 * Function definitions
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTConfSvc_InitApplicationStateInfo();

/**
 * @brief Register the service with GATT server
 */
int GATTConfSvc_Register(unsigned int BluetoothStackID);

/**
 * @brief Unregister service from GATT server
 */
void GATTConfSvc_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int GATTConfSvc_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
void BTPSAPI GATTConfSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);

#ifdef __cplusplus
}
#endif
#endif /* BLEGATTCONFSVC_H_ */
