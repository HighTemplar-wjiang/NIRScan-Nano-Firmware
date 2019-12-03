/*
 * BLE GATT Profile - Time Service Definitions
 * This file contains (UUID and other) defintions for service and characteristics
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTTIMESVCDEFS_H_
#define BLEGATTTIMESVCDEFS_H_

/************************************************************************/
/***                    BLE Command Service - Table                   ***/
/************************************************************************/

/**
 * @brief Time service UUIDs
 */
static BTPSCONST GATT_Primary_Service_128_Entry_t BLE_TimeSvc_UUID =
{
	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x03, 0x52, 0x45, 0x53 }
};

/**
 * @brief Set time characteristic UUIDs
 */
#define TIME_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x0C, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_TimeSvc_Time_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	TIME_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_TimeSvc_Time_Char_Value =
{
	TIME_UUID,
	0,
	NULL
};

/**
 * @brief Service attribute table
 */
BTPSCONST GATT_Service_Attribute_Entry_t BLE_TimeSvc_Att_Entry[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,             (Byte_t *)&BLE_TimeSvc_UUID},         				//0
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_TimeSvc_Time_Char_Declaration},        //1
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_TimeSvc_Time_Char_Value}               //2
};

#define BLE_TIME_SERVICE_ATTRIBUTE_COUNT               (sizeof(BLE_TimeSvc_Att_Entry)/sizeof(GATT_Service_Attribute_Entry_t))

#define BLE_TIMESVC_TIME_CHARACTERISTIC_ATTRIBUTE_OFFSET				2

#endif /* BLEGATTTIMESVCDEFS_H_ */
