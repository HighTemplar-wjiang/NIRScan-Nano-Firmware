/*
 * BLE GATT Profile - Calibration Service Definitions
 * This file contains (UUID and other) defintions for service and characteristics
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTCALSVCDEFS_H_
#define BLEGATTCALSVCDEFS_H_

/************************************************************************/
/***                    BLE Cal Service - Table                   	  ***/
/************************************************************************/

/**
 * @brief BLE cal service
 */
static BTPSCONST GATT_Primary_Service_128_Entry_t BLE_CalSvc_UUID =
{
	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x04, 0x52, 0x45, 0x53 }
};

/**
 * @brief Spectral calibration coefficient characteristic
 */
#define SPEC_CAL_COEFF_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x0D, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CalSvc_SpecCalCoeff_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	SPEC_CAL_COEFF_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CalSvc_SpecCalCoeff_Char_Value =
{
	SPEC_CAL_COEFF_UUID,
	0,
	NULL
};

#define SPEC_CAL_COEFF_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x0E, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CalSvc_SpecCalCoeff_CCD_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
	SPEC_CAL_COEFF_CCD_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CalSvc_SpecCalCoeff_CCD_Value =
{
	SPEC_CAL_COEFF_CCD_UUID,
	0,
	NULL
};

/**
 * @brief reference calibration coefficient characteristic
 */
#define REF_CAL_COEFF_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x0F, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CalSvc_RefCalCoeff_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	REF_CAL_COEFF_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CalSvc_RefCalCoeff_Char_Value =
{
	REF_CAL_COEFF_UUID,
	0,
	NULL
};

#define REF_CAL_COEFF_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x10, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CalSvc_RefCalCoeff_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   REF_CAL_COEFF_CCD_UUID
};


static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CalSvc_RefCalCoeff_CCD_Value =
{
	REF_CAL_COEFF_CCD_UUID,
	0,
	NULL
};

/**
 * @brief reference calibration matrix characteristic
 */
#define REF_CAL_MATRIX_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x11, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CalSvc_RefCalMatrix_Char_Declaration =
{
	GATT_CHARACTERISTIC_PROPERTIES_WRITE,
	REF_CAL_MATRIX_UUID
};

static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CalSvc_RefCalMatrix_Char_Value =
{
	REF_CAL_MATRIX_UUID,
	0,
	NULL
};

#define REF_CAL_MATRIX_CCD_UUID	{ 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x12, 0x41, 0x48, 0x43 }
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t BLE_CalSvc_RefCalMatrix_CCD_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   REF_CAL_MATRIX_CCD_UUID
};


static BTPSCONST GATT_Characteristic_Value_128_Entry_t  BLE_CalSvc_RefCalMatrix_CCD_Value =
{
	REF_CAL_MATRIX_CCD_UUID,
	0,
	NULL
};


/***		Client Characteristic Configuration Descriptor            ***/
static GATT_Characteristic_Descriptor_16_Entry_t CalSvc_Client_Characteristic_Configuration =
{
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
	GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
	NULL
};

/**
 * @brief Calibration service table
 */
BTPSCONST GATT_Service_Attribute_Entry_t BLE_CalSvc_Att_Entry[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,             (Byte_t *)&BLE_CalSvc_UUID},         						//0
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_CalSvc_SpecCalCoeff_Char_Declaration},     //1
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_CalSvc_SpecCalCoeff_Char_Value},		    //2
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_CalSvc_SpecCalCoeff_CCD_Declaration},		//3
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_CalSvc_SpecCalCoeff_CCD_Value},			//4
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&CalSvc_Client_Characteristic_Configuration},	//5
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_CalSvc_RefCalCoeff_Char_Declaration},     	//6
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_CalSvc_RefCalCoeff_Char_Value},	     	//7
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_CalSvc_RefCalCoeff_CCD_Declaration},		//8
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_CalSvc_RefCalCoeff_CCD_Value},				//9
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&CalSvc_Client_Characteristic_Configuration},	//10
   {GATT_ATTRIBUTE_FLAGS_READABLE,		 	aetCharacteristicDeclaration128,  (Byte_t *)&BLE_CalSvc_RefCalMatrix_Char_Declaration},     //11
   {GATT_ATTRIBUTE_FLAGS_WRITABLE, 			aetCharacteristicValue128,        (Byte_t *)&BLE_CalSvc_RefCalMatrix_Char_Value},	     	//12
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128,  (Byte_t *)&BLE_CalSvc_RefCalMatrix_CCD_Declaration},		//13
   {0,          							aetCharacteristicValue128,        (Byte_t *)&BLE_CalSvc_RefCalMatrix_CCD_Value},			//14
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,    (Byte_t *)&CalSvc_Client_Characteristic_Configuration}	//15
};

#define BLE_CAL_SERVICE_ATTRIBUTE_COUNT               (sizeof(BLE_CalSvc_Att_Entry)/sizeof(GATT_Service_Attribute_Entry_t))

/** @name Service attribute table member offsets
 *
 * These offset values represent the characteristic in
 * communications between server and client
 */
//@{
/**		Spectrum cal coefficients characteristic offsets			*/
#define BLE_CALSVC_SPEC_CAL_COEFF_CHARACTERISTIC_ATTRIBUTE_OFFSET	2
#define BLE_CALSVC_SPEC_CAL_COEFF_CCD_ATTRIBUTE_VALUE_OFFSET		4
#define BLE_CALSVC_SPEC_CAL_COEFF_CCD_ATTRIBUTE_OFFSET				5
/**		Reference cal coefficients characteristic offsets			*/
#define BLE_CALSVC_REF_CAL_COEFF_CHARACTERISTIC_ATTRIBUTE_OFFSET	7
#define BLE_CALSVC_REF_CAL_COEFF_CCD_ATTRIBUTE_VALUE_OFFSET			9
#define BLE_CALSVC_REF_CAL_COEFF_CCD_ATTRIBUTE_OFFSET				10
/**		Spectrum cal matrix characteristic offsets			*/
#define BLE_CALSVC_REF_CAL_MATRIX_CHARACTERISTIC_ATTRIBUTE_OFFSET	12
#define BLE_CALSVC_REF_CAL_MATRIX_CCD_ATTRIBUTE_VALUE_OFFSET		14
#define BLE_CALSVC_REF_CAL_MATRIX_CCD_ATTRIBUTE_OFFSET				15
//@}
#endif /* BLEGATTCALSVCDEFS_H_ */
