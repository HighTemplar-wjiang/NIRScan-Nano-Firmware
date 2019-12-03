/*
 * BLE GATT Profile - Command Service Definitions
 * This file contains (UUID and other) defintions for service and characteristics
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTCMDSVCDEFS_H_
#define BLEGATTCMDSVCDEFS_H_

/***		Client Characteristic Configuration Descriptor            ***/
static GATT_Characteristic_Descriptor_16_Entry_t CmdSvc_Client_Characteristic_Configuration =
{
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
	NULL
};

/************************************************************************/
/***                    BLE Command Service - Table                   ***/
/************************************************************************/

/**
 * @brief BLE command service
 */
static BTPSCONST GATT_Primary_Service_128_Entry_t BLE_CmdSvc_UUID =
{
	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x02, 0x52, 0x45, 0x53 }
};

/**
 * @brief BLE command characteristic
 */
#define INTERNAL_CMD_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x0B, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CmdSvc_IntCmd_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,	//GATT_CHARACTERISTIC_PROPERTIES_READ|
	INTERNAL_CMD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CmdSvc_IntCmd_Char_Value =
{
	INTERNAL_CMD_UUID,
	0,
	NULL
};

/**
 * @brief BLE command service attribute table
 */
BTPSCONST GATT_Service_Attribute_Entry_t BLE_CmdSvc_Att_Entry[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,            (Byte_t *)&BLE_CmdSvc_UUID},         						//0
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_CmdSvc_IntCmd_Char_Declaration},        	//1
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_CmdSvc_IntCmd_Char_Value},              	//2
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&CmdSvc_Client_Characteristic_Configuration},	//3
};

#define BLE_COMMAND_SERVICE_ATTRIBUTE_COUNT               (sizeof(BLE_CmdSvc_Att_Entry)/sizeof(GATT_Service_Attribute_Entry_t))

/** @name Service attribute table member offsets
 *
 * These offset values represent the characteristic in
 * communications between server and client
 */
//@{
/**		Command characteristic offsets			*/
#define BLE_CMDSVC_IC_CHARACTERISTIC_ATTRIBUTE_OFFSET       2
#define BLE_CMDSVC_IC_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET   3
//@}

#endif /* BLEGATTCMDSVCDEFS_H_ */
