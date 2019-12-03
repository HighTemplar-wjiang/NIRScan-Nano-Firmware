/*
 * BLE GATT Profile - Scan (data) Service header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLESCANDATASVC_H_
#define BLESCANDATASVC_H_

/**
 * @brief Scan service application information
 */
typedef struct _tagApplicationStateInfo_ScanSvc_t
{
   unsigned int		GATTScanSvcInstanceID;
   unsigned int		GATTScanSvcServiceID;
   char				scanNameStub[SCAN_NAME_LEN-2];
   Word_t			scanList_CCD;
   Word_t			startScan_CCD;
   Word_t			clearScan_CCD;
   Word_t			scanName_CCD;
   Word_t			scanType_CCD;
   Word_t			scanTime_CCD;
   Word_t			scanBlobVer_CCD;
   Word_t			scanData_CCD;
   unsigned short	numScans;
} ApplicationStateInfo_ScanSvc_t;

/**
 * @brief Scan service server instance information
 */
typedef struct _tagScanfSvc_ServerInstance_t {
	unsigned int BluetoothStackID;
	unsigned int ServiceID;
	unsigned int ConnectionID;
	unsigned long CallbackParameter;
} ScanSvc_ServerInstance_t;


/**
 * Function definitions
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application state information
 */
void GATTScanSvc_InitApplicationStateInfo();

/**
 * @brief Register the service with GATT server
 */
int GATTScanSvc_Register(unsigned int BluetoothStackID);

/**
 * @brief Unregister service from GATT server
 */
void GATTScanSvc_Unregister(unsigned int BluetoothStackID);

/**
 * @brief Update connection ID in state information
 */
int GATTScanSvc_UpdateConnectionID(unsigned int ConnectionID);

/**
 * @brief Callback for GATT server events to the service
 */
void BTPSAPI GATTScanSvc_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);

#ifdef __cplusplus
}
#endif
#endif /* BLESCANDATASVC_H_ */
