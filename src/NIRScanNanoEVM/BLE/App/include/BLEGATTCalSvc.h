/*
 * BLE GATT Profile - Calibration Service header
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTCALSVC_H_
#define BLEGATTCALSVC_H_

/**
 * @brief Structure to hold application state information
 */
typedef struct _tagApplicationStateInfo_CalSvc_t
{
   unsigned int		GATTCalSvcInstanceID;
   unsigned int		GATTCalSvcServiceID;
   Word_t			refCal_CCD;
   Word_t			specCal_CCD;
   Word_t			refMat_CCD;
} ApplicationStateInfo_CalSvc_t;

/**
 * @brief Calibration Service Instance Block
 *
 * This structure contains all information associated with a specific
 * Bluetooth Stack ID (member is present in this structure)
 */
typedef struct _tagCalSvc_ServerInstance_t {
	unsigned int BluetoothStackID;
	unsigned int ServiceID;
	unsigned int ConnectionID;
	unsigned long CallbackParameter;
} CalSvc_ServerInstance_t;

/**
 * Function definitions
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTCalSvc_InitApplicationStateInfo();

/**
 * @brief Register the service with GATT server
 */
int GATTCalSvc_Register(unsigned int BluetoothStackID);

/**
 * @brief Unregister service from GATT server
 */
void GATTCalSvc_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int GATTCalSvc_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
void BTPSAPI GATTCalSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);

#ifdef __cplusplus
}
#endif
#endif /* BLEGATTCALSVC_H_ */
