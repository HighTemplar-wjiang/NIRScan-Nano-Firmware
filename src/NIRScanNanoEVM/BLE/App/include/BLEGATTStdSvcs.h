/*
 * BLE GATT profile - Standard v4.0 services
 * Handlers for battery and device information services
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef __BLEGATTSTDSVCS_H__
#define __BLEGATTSTDSVCS_H__

/**
 * @brief Structure to hold application state information
 */
typedef struct _tagApplicationStateInfo_StdSvcs_t
{
   unsigned int ConnectionID;
   unsigned int DISInstanceID;
   unsigned int BASInstanceID;
   unsigned int BatteryLevel;
   Word_t Battery_CCD;
} ApplicationStateInfo_StdSvcs_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTStdSvcs_InitApplicationStateInfo();

/**
 * @brief Register the service with GATT server
 */
int  GATTStdSvcs_Register(unsigned int BluetoothStackID);

/**
 * @brief Unregister service from GATT server
 */
int GATTStdSvcs_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int  GATTStdSvcs_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
int GATTStdSvcs_UpdateBatteryLevel(unsigned int BluetoothStackID, Byte_t BatteryLevel);

#ifdef __cplusplus
}
#endif
#endif	// __BLEGATTSTDSVCS_H__

