/*
 * BLE GATT Profile - (scan) Configuration Service Definitions
 * This file contains (UUID and other) defintions for service and characteristics
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTCONFSVCDEFS_H_
#define BLEGATTCONFSVCDEFS_H_

/************************************************************************/
/***                    BLE Conf Service - Table                   	  ***/
/************************************************************************/

/**
 * @brief Scan configuration service
 */
static BTPSCONST GATT_Primary_Service_128_Entry_t BLE_ConfSvc_UUID =
{
	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x05, 0x52, 0x45, 0x53 }
};

#define NUM_STORED_CONF_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x13, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ConfSvc_numStoredConf_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ,
	NUM_STORED_CONF_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ConfSvc_numStoredConf_Char_Value =
{
	NUM_STORED_CONF_UUID,
	0,
	NULL
};

/**
 * @brief read scan configuration list characteristic
 */
#define READ_CONF_LIST_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x14, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ConfSvc_ReadConfList_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_CONF_LIST_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ConfSvc_ReadConfList_Char_Value =
{
	READ_CONF_LIST_UUID,
	0,
	NULL
};

#define READ_CONF_LIST_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x15, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ConfSvc_ReadConfList_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_CONF_LIST_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ConfSvc_ReadConfList_Char_CCD_Value =
{
	READ_CONF_LIST_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Read scan configuration data characteristic
 */
#define READ_CONF_DATA_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x16, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ConfSvc_ReadConfData_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_CONF_DATA_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ConfSvc_ReadConfData_Char_Value =
{
	READ_CONF_DATA_UUID,
	0,
	NULL
};


#define READ_CONF_DATA_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x17, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ConfSvc_ReadConfData_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_CONF_DATA_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ConfSvc_ReadConfData_Char_CCD_Value =
{
	READ_CONF_DATA_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Active configuration characteristic
 */
#define ACTIVE_CONF_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x18, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ConfSvc_activeConf_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	ACTIVE_CONF_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ConfSvc_activeConf_Char_Value =
{
	ACTIVE_CONF_UUID,
	0,
	NULL
};

/***		Client Characteristic Configuration Descriptor            ***/
static GATT_Characteristic_Descriptor_16_Entry_t ConfSvc_Client_Characteristic_Configuration =
{
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
	NULL
};

/**
 * @brief Configuration service table
 */
BTPSCONST GATT_Service_Attribute_Entry_t BLE_ConfSvc_Att_Entry[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,             (Byte_t *)&BLE_ConfSvc_UUID},         						//0
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ConfSvc_numStoredConf_Char_Declaration},   	//1
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ConfSvc_numStoredConf_Char_Value},		    	//2
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ConfSvc_ReadConfList_Char_Declaration},    	//3
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ConfSvc_ReadConfList_Char_Value},	     		//4
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ConfSvc_ReadConfList_Char_CCD_Declaration},	//5
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ConfSvc_ReadConfList_Char_CCD_Value},			//6
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ConfSvc_Client_Characteristic_Configuration},		//7
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ConfSvc_ReadConfData_Char_Declaration},    	//8
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ConfSvc_ReadConfData_Char_Value},	     		//9
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ConfSvc_ReadConfData_Char_CCD_Declaration},	//10
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ConfSvc_ReadConfData_Char_CCD_Value},			//11
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ConfSvc_Client_Characteristic_Configuration},		//12
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ConfSvc_activeConf_Char_Declaration},    		//13
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicValue128,        (Byte_t *)&BLE_ConfSvc_activeConf_Char_Value},	     		//14
};

#define BLE_CONF_SERVICE_ATTRIBUTE_COUNT               (sizeof(BLE_ConfSvc_Att_Entry)/sizeof(GATT_Service_Attribute_Entry_t))

/** @name Service attribute table member offsets
 *
 * These offset values represent the characteristic in
 * communications between server and client
 */
//@{
/**		Num stored configurations characteristic offsets				*/
#define BLE_CONFSVC_NUM_STORED_CONF_CHARACTERISTIC_ATTRIBUTE_OFFSET		2
/**		Scan configuration list characteristic offsets					*/
#define BLE_CONFSVC_READ_CONF_LIST_CHARACTERISTIC_ATTRIBUTE_OFFSET		4
#define BLE_CONFSVC_READ_CONF_LIST_CCD_ATTRIBUTE_VALUE_OFFSET			6
#define BLE_CONFSVC_READ_CONF_LIST_CCD_ATTRIBUTE_OFFSET					7
/**		Scan configuration data characteristic offsets					*/
#define BLE_CONFSVC_READ_CONF_DATA_CHARACTERISTIC_ATTRIBUTE_OFFSET		9
#define BLE_CONFSVC_READ_CONF_DATA_CCD_ATTRIBUTE_VALUE_OFFSET			11
#define BLE_CONFSVC_READ_CONF_DATA_CCD_ATTRIBUTE_OFFSET					12
#define BLE_CONFSVC_ACTIVE_CONF_CHARACTERISTIC_ATTRIBUTE_OFFSET			14
//@}

#endif /* BLEGATTCONFSVCDEFS_H_ */
