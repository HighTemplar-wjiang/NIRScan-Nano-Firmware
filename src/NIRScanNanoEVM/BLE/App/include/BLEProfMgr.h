/*
 * BLE Profile Manager header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef __BLEPROFMGR_H__
#define __BLEPROFMGR_H__

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief BLE Application initialize function
 *
 * The following function is used to initialize the application
 * instance.  This function should open the stack and prepare to
 * execute commands based on user input.  The first parameter passed
 * to this function is the HCI Driver Information that will be used
 * when opening the stack and the second parameter is used to pass
 * parameters to BTPS_Init.  This function returns the
 * BluetoothStackID returned from BSC_Initialize on success or a
 * negative error code (of the form APPLICATION_ERROR_XXX)
 */
int InitializeApplication(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);

/**
 * @brief Initilize BLE application state information
 *
 * This function initializes BLE application states as well as the states
 * of all custom GATT services
 */
void InitBLEApplicationStateInfo();

/**
 * @brief Function to register all BLE services with GATT server
 *
 * This function registers all services (Standard and custom) with
 * GATT server
 */
int RegisterBLEServices();

/**
 * @brief Function to turn ON/OFF BLE advertisement
 *
 */
int AdvertiseLE(Boolean_t enable);

/**
 * @brief Function to close BLE stack
 *
 */
int CloseStack(void);

#ifdef __cplusplus
}
#endif
#endif	//	__BLEPROFMGR_H

