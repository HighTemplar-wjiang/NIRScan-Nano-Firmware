/*
 * BLE GATT Profile - Scan (data) Service Definitions
 * This file contains (UUID and other) defintions for service and characteristics
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLESCANDATASVCDEFS_H_
#define BLESCANDATASVCDEFS_H_

/************************************************************************/
/***                    BLE Scan Service - Table                   	  ***/
/************************************************************************/

/**
 * @brief Scan service UUID
 */
static BTPSCONST GATT_Primary_Service_128_Entry_t BLE_ScanSvc_UUID =
{
	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x06, 0x52, 0x45, 0x53 }
};

/**
 * @brief Read num stored scans characteristic UUID
 */
#define NUM_STORED_SCANS_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x19, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_numStoredScans_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_READ,
	NUM_STORED_SCANS_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_numStoredScans_Char_Value =
{
	NUM_STORED_SCANS_UUID,
	0,
	NULL
};

/**
 * @brief Read scan list characteristic UUIDs
 */
#define READ_SCAN_LIST_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x1A, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanList_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_SCAN_LIST_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanList_Char_Value =
{
	READ_SCAN_LIST_UUID,
	0,
	NULL
};

#define READ_SCAN_LIST_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x1B, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanList_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_SCAN_LIST_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanList_Char_CCD_Value =
{
	READ_SCAN_LIST_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Write scan name stub characteristic
 */
#define WRITE_SCAN_NAME_STUB_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x1C, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_WriteScanNameStub_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	WRITE_SCAN_NAME_STUB_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_WriteScanNameStub_Char_Value =
{
	WRITE_SCAN_NAME_STUB_UUID,
	0,
	NULL
};

/**
 * @brief Start scan characteristic UUIDs
 */
#define START_SCAN_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x1D, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_StartScan_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	START_SCAN_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_StartScan_Char_Value =
{
	START_SCAN_UUID,
	0,
	NULL
};

static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_StartScan_Char_CCD_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
	START_SCAN_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_StartScan_Char_CCD_Value =
{
	START_SCAN_UUID,
	0,
	NULL
};

/**
 * @brief Clear scan characteristic UUIDs
 */
#define CLEAR_SCAN_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x1E, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ClearScan_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	CLEAR_SCAN_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ClearScan_Char_Value =
{
	CLEAR_SCAN_UUID,
	0,
	NULL
};

static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ClearScan_Char_CCD_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
	CLEAR_SCAN_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ClearScan_Char_CCD_Value =
{
	CLEAR_SCAN_UUID,
	0,
	NULL
};

/**
 * @brief Read scan name characteristic UUID
 */
#define READ_SCAN_NAME_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x1F, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanName_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_SCAN_NAME_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanName_Char_Value =
{
	READ_SCAN_NAME_UUID,
	0,
	NULL
};

#define READ_SCAN_NAME_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x20, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanName_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_SCAN_NAME_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanName_Char_CCD_Value =
{
	READ_SCAN_NAME_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Read scan type characteristic UUID
 */
#define READ_SCAN_TYPE_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x21, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanType_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_SCAN_TYPE_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanType_Char_Value =
{
	READ_SCAN_TYPE_UUID,
	0,
	NULL
};

#define READ_SCAN_TYPE_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x22, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanType_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_SCAN_TYPE_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanType_Char_CCD_Value =
{
	READ_SCAN_TYPE_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Read scan time characteristic UUID
 */
#define READ_SCAN_TIME_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x23, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanTime_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_SCAN_TIME_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanTime_Char_Value =
{
	READ_SCAN_TIME_UUID,
	0,
	NULL
};

#define READ_SCAN_TIME_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x24, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanTime_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_SCAN_TIME_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanTime_Char_CCD_Value =
{
	READ_SCAN_TIME_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Read scan data blob version UUID
 */
#define READ_SCAN_BLOB_VER_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x25, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanBlobVer_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_SCAN_BLOB_VER_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanBlobVer_Char_Value =
{
	READ_SCAN_BLOB_VER_UUID,
	0,
	NULL
};

#define READ_SCAN_BLOB_VER_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x26, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanBlobVer_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_SCAN_BLOB_VER_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanBlobVer_Char_CCD_Value =
{
	READ_SCAN_BLOB_VER_CCD_UUID,
	0,
	NULL
};

/**
 * @brief Read scan data characteristic UUID
 */
#define READ_SCAN_DATA_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x27, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanData_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	READ_SCAN_DATA_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanData_Char_Value =
{
	READ_SCAN_DATA_UUID,
	0,
	NULL
};

#define READ_SCAN_DATA_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x28, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_ScanSvc_ReadScanData_Char_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   READ_SCAN_DATA_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_ScanSvc_ReadScanData_Char_CCD_Value =
{
	READ_SCAN_DATA_CCD_UUID,
	0,
	NULL
};

/***		Client Characteristic Configuration Descriptor            ***/
static GATT_Characteristic_Descriptor_16_Entry_t ScanSvc_Client_Characteristic_Configuration =
{
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
	NULL
};

/**
 * @brief Scan service attribute table
 */
BTPSCONST GATT_Service_Attribute_Entry_t BLE_ScanSvc_Att_Entry[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,             (Byte_t *)&BLE_ScanSvc_UUID},         						//0
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_numStoredScans_Char_Declaration},   	//1
   {GATT_ATTRIBUTE_FLAGS_READABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_numStoredScans_Char_Value},		    //2
   {GATT_ATTRIBUTE_FLAGS_READABLE,			aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanList_Char_Declaration},    	//3
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanList_Char_Value},	     		//4
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanList_Char_CCD_Declaration},	//5
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanList_Char_CCD_Value},			//6
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//7
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_WriteScanNameStub_Char_Declaration},   //8
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_WriteScanNameStub_Char_Value},	     	//9
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_StartScan_Char_Declaration},   		//10
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_StartScan_Char_Value},	     			//11
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_StartScan_Char_CCD_Declaration},   	//12
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_StartScan_Char_CCD_Value},	     		//13
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//14
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ClearScan_Char_Declaration},   		//15
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ClearScan_Char_Value},	     			//16
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ClearScan_Char_CCD_Declaration},   	//17
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ClearScan_Char_CCD_Value},	     		//18
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//19
   {GATT_ATTRIBUTE_FLAGS_READABLE,			aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanName_Char_Declaration},    	//20
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanName_Char_Value},	     		//21
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanName_Char_CCD_Declaration},	//22
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanName_Char_CCD_Value},			//23
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//24
   {GATT_ATTRIBUTE_FLAGS_READABLE,			aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanType_Char_Declaration},    	//25
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanType_Char_Value},	     		//26
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanType_Char_CCD_Declaration},	//27
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanType_Char_CCD_Value},			//28
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//29
   {GATT_ATTRIBUTE_FLAGS_READABLE,			aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanTime_Char_Declaration},    	//30
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanTime_Char_Value},	     		//31
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanTime_Char_CCD_Declaration},	//32
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanTime_Char_CCD_Value},			//33
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//34
   {GATT_ATTRIBUTE_FLAGS_READABLE,			aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanBlobVer_Char_Declaration},    	//35
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanBlobVer_Char_Value},	     	//36
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanBlobVer_Char_CCD_Declaration},	//37
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanBlobVer_Char_CCD_Value},		//38
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//39
   {GATT_ATTRIBUTE_FLAGS_READABLE,			aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanData_Char_Declaration},    	//40
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanData_Char_Value},	     		//41
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_ScanSvc_ReadScanData_Char_CCD_Declaration},	//42
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_ScanSvc_ReadScanData_Char_CCD_Value},			//43
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&ScanSvc_Client_Characteristic_Configuration},		//44
};

#define BLE_SCAN_SERVICE_ATTRIBUTE_COUNT               (sizeof(BLE_ScanSvc_Att_Entry)/sizeof(GATT_Service_Attribute_Entry_t))

/** @name Service attribute table member offsets
 *
 * These offset values represent the characteristic in
 * communications between server and client
 */
//@{
/**		Num stored scans characteristic offsets							*/
#define BLE_SCANSVC_NUM_STORED_SCANS_CHARACTERISTIC_ATTRIBUTE_OFFSET	2
/**		Read stored scan list characteristic offsets					*/
#define BLE_SCANSVC_READ_SCAN_LIST_CHARACTERISTIC_ATTRIBUTE_OFFSET		4
#define BLE_SCANSVC_READ_SCAN_LIST_CCD_ATTRIBUTE_VALUE_OFFSET			6
#define BLE_SCANSVC_READ_SCAN_LIST_CCD_ATTRIBUTE_OFFSET					7
/**		Write scan name stub characteristic offsets						*/
#define BLE_SCANSVC_WRITE_SCAN_NAME_STUB_ATTRIBUTE_OFFSET				9
/**		Start scan characteristic offsets								*/
#define BLE_SCANSVC_START_SCAN_CHARACTERISTIC_ATTRIBUTE_OFFSET			11
#define BLE_SCANSVC_START_SCAN_CCD_ATTRIBUTE_VALUE_OFFSET				13
#define BLE_SCANSVC_START_SCAN_CCD_ATTRIBUTE_OFFSET						14
/**		Clear scan characteristic offsets								*/
#define BLE_SCANSVC_CLEAR_SCAN_CHARACTERISTIC_ATTRIBUTE_OFFSET			16
#define BLE_SCANSVC_CLEAR_SCAN_CCD_ATTRIBUTE_VALUE_OFFSET				18
#define BLE_SCANSVC_CLEAR_SCAN_CCD_ATTRIBUTE_OFFSET						19
/**		Read scan name characteristic offsets							*/
#define BLE_SCANSVC_READ_SCAN_NAME_CHARACTERISTIC_ATTRIBUTE_OFFSET		21
#define BLE_SCANSVC_READ_SCAN_NAME_CCD_ATTRIBUTE_VALUE_OFFSET			23
#define BLE_SCANSVC_READ_SCAN_NAME_CCD_ATTRIBUTE_OFFSET					24
/**		Read scan type characteristic offsets							*/
#define BLE_SCANSVC_READ_SCAN_TYPE_CHARACTERISTIC_ATTRIBUTE_OFFSET		26
#define BLE_SCANSVC_READ_SCAN_TYPE_CCD_ATTRIBUTE_VALUE_OFFSET			28
#define BLE_SCANSVC_READ_SCAN_TYPE_CCD_ATTRIBUTE_OFFSET					29
/**		Read scan time characteristic offsets							*/
#define BLE_SCANSVC_READ_SCAN_TIME_CHARACTERISTIC_ATTRIBUTE_OFFSET		31
#define BLE_SCANSVC_READ_SCAN_TIME_CCD_ATTRIBUTE_VALUE_OFFSET			33
#define BLE_SCANSVC_READ_SCAN_TIME_CCD_ATTRIBUTE_OFFSET					34
/**		Read scan blob version characteristic offsets					*/
#define BLE_SCANSVC_READ_SCAN_BLOB_VER_CHARACTERISTIC_ATTRIBUTE_OFFSET	36
#define BLE_SCANSVC_READ_SCAN_BLOB_VER_CCD_ATTRIBUTE_VALUE_OFFSET		38
#define BLE_SCANSVC_READ_SCAN_BLOB_VER_CCD_ATTRIBUTE_OFFSET				39
/**		Read scan data characteristic offsets							*/
#define BLE_SCANSVC_READ_SCAN_DATA_CHARACTERISTIC_ATTRIBUTE_OFFSET		41
#define BLE_SCANSVC_READ_SCAN_DATA_CCD_ATTRIBUTE_VALUE_OFFSET			43
#define BLE_SCANSVC_READ_SCAN_DATA_CCD_ATTRIBUTE_OFFSET					44
//@}

#endif /* BLESCANDATASVCDEFS_H_ */
