/*
 * BLE GATT Profile - General Information Service Definitions
 * This file contains (UUID and other) defintions for service and characteristics
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTGISVCDEFS_H_
#define BLEGATTGISVCDEFS_H_

/***		Client Characteristic Configuration Descriptor            ***/
static GATT_Characteristic_Descriptor_16_Entry_t GISvc_Client_Characteristic_Configuration =
{
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
	NULL
};

/************************************************************************/
/***         BLE General informationService - Table                   ***/
/************************************************************************/

/**
 * @brief General information service UUIDs
 */
static BTPSCONST GATT_Primary_Service_128_Entry_t BLE_GISvc_UUID =
{
	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x01, 0x52, 0x45, 0x53 }
};

/**
 * @brief Temperature characteristic UUIDs
 */
#define TEMP_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x01, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_Temp_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
	TEMP_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_Temp_Char_Value =
{
	TEMP_UUID,
	0,
	NULL
};

/**
 * @brief Humidity characteristic UUIDs
 */
#define HUM_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x02, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_Hum_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
	HUM_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_Hum_Char_Value =
{
	HUM_UUID,
	0,
	NULL
};

/**
 * @brief Device status characteristic UUIDs
 */
#define DEVICE_STATUS_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x03, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_DevStat_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
	DEVICE_STATUS_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_DevStat_Char_Value =
{
	DEVICE_STATUS_UUID,
	0,
	NULL
};

/**
 * @brief Error status characteristic UUIDs
 */
#define ERROR_STATUS_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x04, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_ErrStat_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_WRITE|GATT_CHARACTERISTIC_PROPERTIES_INDICATE,
	ERROR_STATUS_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_ErrStat_Char_Value =
{
	ERROR_STATUS_UUID,
	0,
	NULL
};

/**
 * @brief Temperature threshold characteristic UUIDs
 */
#define TEMP_THRESH_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x05, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_TempThresh_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	TEMP_THRESH_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_TempThresh_Char_Value =
{
	TEMP_THRESH_UUID,
	0,
	NULL
};

/**
 * @brief Humidity threshold characteristic UUIDs
 */
#define HUM_THRESH_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x06, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_HumThresh_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	HUM_THRESH_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_HumThresh_Char_Value =
{
	HUM_THRESH_UUID,
	0,
	NULL
};

/**
 * @brief Hours of use characteristic UUIDs
 */
#define HOURS_OF_USE_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x07, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_HoursOfUse_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ,
	HOURS_OF_USE_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_HoursOfUse_Char_Value =
{
	HOURS_OF_USE_UUID,
	0,
	NULL
};

/**
 * @brief Num battery recharge cycle characteristic UUIDs
 */
#define NUM_BATT_RECHARGE_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x08, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_NumBattRecharge_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ,
	NUM_BATT_RECHARGE_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_NumBattRecharge_Char_Value =
{
	NUM_BATT_RECHARGE_UUID,
	0,
	NULL
};

/**
 * @brief Num lamp hours characteristic UUIDs
 */
#define LAMP_HOURS_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x09, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_LampHours_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ,
	LAMP_HOURS_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_LampHours_Char_Value =
{
	LAMP_HOURS_UUID,
	0,
	NULL
};


/*
#define ERROR_LOG_UUID { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x0A, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_GISvc_ErrLog_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ,
	ERROR_LOG_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_GISvc_ErrLog_Char_Value =
{
	ERROR_LOG_UUID,
	0,
	NULL
};
*/

/**
 * @brief Service attribute table
 */
BTPSCONST GATT_Service_Attribute_Entry_t BLE_GISvc_Att_Entry[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,            (Byte_t *)&BLE_GISvc_UUID},         								//0
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_GISvc_Temp_Char_Declaration},        				//1
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_GISvc_Temp_Char_Value},              				//2
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&GISvc_Client_Characteristic_Configuration},				//3
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_GISvc_Hum_Char_Declaration},        				//4
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_GISvc_Hum_Char_Value},              				//5
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&GISvc_Client_Characteristic_Configuration},				//6
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_DevStat_Char_Declaration},        			//7
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_DevStat_Char_Value},              			//8
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&GISvc_Client_Characteristic_Configuration},				//9
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_ErrStat_Char_Declaration},        			//10
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_ErrStat_Char_Value},              			//11
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&GISvc_Client_Characteristic_Configuration},				//12
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_TempThresh_Char_Declaration},     			//13
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_TempThresh_Char_Value},           			//14
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_HumThresh_Char_Declaration},      			//15
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_HumThresh_Char_Value},            			//16
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_HoursOfUse_Char_Declaration},    				//17
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_HoursOfUse_Char_Value},              			//18
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_NumBattRecharge_Char_Declaration},    		//19
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_NumBattRecharge_Char_Value},          		//20
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_LampHours_Char_Declaration},        			//21
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_LampHours_Char_Value},              			//22
//   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128, (Byte_t *)&BLE_GISvc_ErrLog_Char_Declaration},        				//23
//   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,       (Byte_t *)&BLE_GISvc_ErrLog_Char_Value}              				//24
};

#define BLE_GI_SERVICE_ATTRIBUTE_COUNT               (sizeof(BLE_GISvc_Att_Entry)/sizeof(GATT_Service_Attribute_Entry_t))

/** @name Service attribute table member offsets
 *
 * These offset values represent the characteristic in
 * communications between server and client
 */
//@{
/**		Temperature characteristic offsets							*/
#define BLE_GISVC_TEMP_CHARACTERISTIC_ATTRIBUTE_OFFSET				2
#define BLE_GISVC_TEMP_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET			3
/**		Humidity characteristic offsets								*/
#define BLE_GISVC_HUM_CHARACTERISTIC_ATTRIBUTE_OFFSET				5
#define BLE_GISVC_HUM_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET			6
/**		Device status characteristic offsets						*/
#define BLE_GISVC_DEVSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET			8
#define BLE_GISVC_DEVSTAT_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET		9
/**		Error status characteristic offsets							*/
#define BLE_GISVC_ERRSTAT_CHARACTERISTIC_ATTRIBUTE_OFFSET			11
#define BLE_GISVC_ERRSTAT_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET		12
/**		Temperature threshold characteristic offsets				*/
#define BLE_GISVC_TEMPTHRESH_CHARACTERISTIC_ATTRIBUTE_OFFSET		14
/**		Humidity threshold characteristic offsets					*/
#define BLE_GISVC_HUMTHRESH_CHARACTERISTIC_ATTRIBUTE_OFFSET			16
/**		Hours of use characteristic offsets							*/
#define BLE_GISVC_HOURSOFUSE_CHARACTERISTIC_ATTRIBUTE_OFFSET		18
/**		Num battery recharge characteristic offsets					*/
#define BLE_GISVC_BATTRECHARGE_CHARACTERISTIC_ATTRIBUTE_OFFSET		20
/**		Lamp hours characteristic offsets							*/
#define BLE_GISVC_LAMPHOURS_CHARACTERISTIC_ATTRIBUTE_OFFSET			22
/**		Error log characteristic offsets - FUTURE USE				*/
#define BLE_GISVC_ERRLOG_CHARACTERISTIC_ATTRIBUTE_OFFSET			24
//@}

#endif /* BLEGATTGISVCDEFS_H_ */
