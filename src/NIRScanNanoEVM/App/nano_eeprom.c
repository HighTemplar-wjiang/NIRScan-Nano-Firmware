/*
 * contains functions that manage storing and retrieving data from the EEPROM
 * of NIRscanNano EVM.
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <driverlib/eeprom.h>
#include "dlpspec_version.h"
#include <NNOCommandDefs.h>
#include "common.h"
#include "nano_eeprom.h"

uint16_t g_scanConfigIDs[EEPROM_MAX_SCAN_CFG_STORAGE];

static int32_t Nano_eeprom_SetNumConfigRecords(uint16_t num_records)
/* 
 * Writes at its designated location in EEPROM; the variable that holds number of
 * config records in EEPROM.
 *
 * @param num_records -I - number of config records stored in EEPROM
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
	uint32_t test_word;

	EEPROMRead(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, EEPROM_NUM_AND_ACTIVE_CFG_SIZE);
	EEPROM_SET_NUM_RECORDS(test_word, num_records);
	return EEPROMProgram(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, EEPROM_NUM_AND_ACTIVE_CFG_SIZE);
}

int32_t Nano_eeprom_SetActiveConfig(uint32_t index)
/**
 * Sets the scanConfig at specified index in EEPROM as active.
 *
 * @param index - I - index of scanConfig to be set active
 *
 * @return PASS or FAIL
 */
{
	uint32_t test_word;

	EEPROMRead(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, EEPROM_NUM_AND_ACTIVE_CFG_SIZE);
	EEPROM_SET_ACTIVE_CFG(test_word, index);
	if(EEPROMProgram(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, sizeof(test_word)) != 0)
		return FAIL;

	return PASS;
}

uint8_t Nano_eeprom_GetActiveConfigIndex(void)
/**
 * Returns the index (in EEPROM) of scanConfig that is currently active.
 *
 * @return index
 */
{
	uint32_t test_word;

	EEPROMRead(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, EEPROM_NUM_AND_ACTIVE_CFG_SIZE);
	test_word = EEPROM_GET_ACTIVE_CFG_INDEX(test_word);
	return (uint8_t)test_word;
}

static int Nano_eeprom_SetScanConfigIndexCounter(uint32_t* scanConfigIndexCounter)
/* 
 * Writes at its designated location in EEPROM; the variable that holds unique scan 
 * config index
 *
 * @param scanConfigIndexCounter -I - scanConfig index running counter
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
    return EEPROMProgram((uint32_t*)scanConfigIndexCounter, EEPROM_CONFIG_COUNTER_ADDR, \
			EEPROM_CONFIG_COUNTER_SIZE);
}

static int Nano_eeprom_GetScanConfigIndexCounter(uint32_t* scanConfigIndexCounter)
/* 
 * Retrieves from EEPROM; the variable that holds unique scan config index
 *
 * @param scanConfigIndexCounter -O - true = scanConfig index running counter
 *
 * @return PASS
 */
{
    EEPROMRead((uint32_t*)scanConfigIndexCounter, EEPROM_CONFIG_COUNTER_ADDR,\
		   	EEPROM_CONFIG_COUNTER_SIZE);
    return PASS;
}

int Nano_eeprom_SetScanIndexCounter(uint16_t* pScanIndexCounter)
/**
 * Writes at its designated location in EEPROM; the variable that holds unique scan 
 * index
 *
 * @param pScanIndexCounter -I - scan index running counter
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
	uint32_t scanIndexCtr = *pScanIndexCounter;
    return EEPROMProgram(&scanIndexCtr, EEPROM_INDEX_COUNTER_ADDR,\
		   	EEPROM_INDEX_COUNTER_SIZE);
}

int Nano_eeprom_GetScanIndexCounter(uint16_t* pScanIndexCounter)
/** 
 * Retrieves from EEPROM; the variable that holds unique scan index
 *
 * @param pScanIndexCounter -O - true = scanConfig index running counter
 *
 * @return PASS
 */
{
	uint32_t scanIndexCtr;
    EEPROMRead(&scanIndexCtr, EEPROM_INDEX_COUNTER_ADDR, \
			EEPROM_INDEX_COUNTER_SIZE);

    *pScanIndexCounter = scanIndexCtr;

    return PASS;
}

void Nano_eeprom_GatherScanCfgIDs(void)
/**
 * Iterates through all scanConfigs stored in EEPROM and populates g_scanConfigIDs
 * array that can be published to the bluetooth app later
 *
 * @return none
 *
 */
{
    int index;
    uint32_t num_records;
    uScanConfig cfgRec;

    num_records = Nano_eeprom_GetNumConfigRecords();
    for(index=0; index<num_records ; index++)
    {
    	Nano_eeprom_GetConfigRecord(index, &cfgRec);
        g_scanConfigIDs[index] = cfgRec.scanCfg.scanConfigIndex;
    }
    return;
}

static uint32_t Nano_eeprom_GetConfigRecordSize(uScanConfig *pCfg)
{
	uint32_t record_size;

	if(pCfg->scanCfg.scan_type != SLEW_TYPE)
		record_size = EEPROM_SCAN_CFG_STRUCT_SIZE;
	else
	{
		record_size = sizeof(slewScanConfig);
		record_size -= ((SLEW_SCAN_MAX_SECTIONS - pCfg->slewScanCfg.head.num_sections)*sizeof(slewScanSection));
		record_size = (((record_size+3)/4)*4);
	}

	return record_size;
}

static int Nano_eeprom_GetConfigRecordFromAddr(uint32_t addr, uScanConfig *pCfg)
{
	uint32_t temp[EEPROM_SLEW_SCAN_CFG_STRUCT_SIZE/4];

	EEPROMRead((uint32_t *)pCfg, addr, EEPROM_SCAN_CFG_STRUCT_SIZE);
	if(pCfg->scanCfg.scan_type == SLEW_TYPE)
	{
		EEPROMRead(temp, addr, EEPROM_SLEW_SCAN_CFG_STRUCT_SIZE);
		memcpy( pCfg, temp, sizeof(uScanConfig));
	}
	if(pCfg->scanCfg.ScanConfig_serial_number[0] == 'F')
		return FAIL;

	return PASS;
}

static uint32_t Nano_eeprom_GetConfigRecordAddr(uint8_t index)
/* Returns 0 on failure or invalid index or out of bounds */
{
	uint32_t num_records;
	uint32_t next_record_addr = EEPROM_SCAN_CFG_ADDRESS;
	uint32_t record_size;
	uScanConfig cfg;
	int i;

	if(index >= EEPROM_MAX_SCAN_CFG_STORAGE)
		return 0;

	num_records = Nano_eeprom_GetNumConfigRecords();

	if(index > num_records)
		return 0;

	for(i=0; i<index; i++)
	{
		if(Nano_eeprom_GetConfigRecordFromAddr(next_record_addr, &cfg) != PASS)
			return 0;
		record_size = Nano_eeprom_GetConfigRecordSize(&cfg);
		next_record_addr += record_size;
	}

	if(next_record_addr > EEPROM_SIZE)
		next_record_addr = 0;

	return next_record_addr;
}

int Nano_eeprom_SaveConfigRecord(uint8_t index, uScanConfig *pCfg)
/**
 * Saves the scanConfig structure that is passed at the specified index in EEPROM.
 *
 * @param index - I - index at which to store the scanConfig structure
 * @param pCfg - I - scan config structure to be stored in EEPROM.
 *
 * @return PASS or FAIL
 */
{
	uint32_t num_records;
	int ret;
	uint32_t ConfigIndexCounter = 0;
	uint32_t val = DLPSPEC_CFG_VER;
	char ser_num[NANO_SER_NUM_LEN];
	int i;
	uint32_t addr;

	if(index >= EEPROM_MAX_SCAN_CFG_STORAGE)
		return FAIL;

	ret = Nano_eeprom_GetDeviceSerialNumber((uint8_t*)ser_num);
	Nano_eeprom_GetScanConfigIndexCounter(&ConfigIndexCounter);
	pCfg->scanCfg.scanConfigIndex = ConfigIndexCounter;
	ConfigIndexCounter++;
	Nano_eeprom_SetScanConfigIndexCounter(&ConfigIndexCounter);
	for(i=0; i < NANO_SER_NUM_LEN; i++)
	{
		if(!ret)
		{
	        pCfg->scanCfg.ScanConfig_serial_number[i] = ser_num[i];
		}
	    else
	    {
			pCfg->scanCfg.ScanConfig_serial_number[i] = 'F';
	    }
	}
	num_records = Nano_eeprom_GetNumConfigRecords();
	/* Store the data structure version number */
	EEPROMProgram(&val, EEPROM_CFG_VER_ADDR, EEPROM_CFG_VER_SIZE);

	addr = Nano_eeprom_GetConfigRecordAddr(index);
	if(addr == 0)
		return FAIL;

	ret = EEPROMProgram((uint32_t *)pCfg, addr, Nano_eeprom_GetConfigRecordSize(pCfg));

	//If we got next record, update num_records
	if(index >= num_records)
	{
		if((ret = Nano_eeprom_SetNumConfigRecords(index + 1)) < 0)
			return ret;
	}

	return ret;
}

int Nano_eeprom_GetConfigRecord(uint8_t index, uScanConfig *pCfg)
/**
 * Retrieves the scanConfig structure that is stored at the specified index in EEPROM.
 *
 * @param index - I - index from which to retrieve the scanConfig structure
 * @param pCfg - O - scan config structure retrieved from EEPROM.
 *
 * @return PASS or FAIL
 */
{
	int32_t num_records;
	uint32_t addr;

	num_records = Nano_eeprom_GetNumConfigRecords();
	if(index > MIN((EEPROM_MAX_SCAN_CFG_STORAGE-1), (num_records-1)))
		return FAIL;

	addr = Nano_eeprom_GetConfigRecordAddr(index);
	if(addr == 0)
		return FAIL;

	return Nano_eeprom_GetConfigRecordFromAddr(addr, pCfg);
}

uint8_t Nano_eeprom_GetNumConfigRecords(void)
/**
 * Returns the total number of scanConfig structures that are stored in EEPROM.
 *
 * @return number of records
 */
{
	uint32_t test_word;
	uint32_t num_records;
	
	EEPROMRead(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, 4);
	if ( ((test_word & 0XFFFF0000) >> 16) > EEPROM_MAX_SCAN_CFG_STORAGE )
	      return 0;
	else 
	{
	    num_records = EEPROM_GET_NUM_CFGS(test_word);
	    return (uint8_t)num_records;
	}
}

uint8_t Nano_eeprom_GetScanConfigIndexUsingConfigID(uint16_t id)
/**
 * Given a scanConfig ID; returns the position at which that record is stored in EEPROM
 *
 * @param id - I - scanConfig ID
 *
 * @return 255 = failed to find the ID; or returns the position/index
 */
{
	uint8_t idx = 255;	//max value to indicate failure
	int i = 0;

	for (i=0; i <EEPROM_MAX_SCAN_CFG_STORAGE; i++)
	{
		if (g_scanConfigIDs[i] == id)
		{
			idx = i;
			break;
		}
	}
	return (idx);
}

int16_t Nano_eeprom_GetScanConfigIDUsingIndex(uint8_t index)
/**
 * Given a index; returns the scanConfigID of the record stored at that position
 *
 * @param index - I - index or position
 *
 * @return FAIL = invalid index; or returns the scanConfig ID
 */
{
	if (index < EEPROM_MAX_SCAN_CFG_STORAGE)
		return (g_scanConfigIDs[index]);
	else	// Error
		return (FAIL);
}

int Nano_eeprom_EraseAllConfigRecords(void)
/**
 * Erases all scanConfig records (except the factory default) from EEPROM
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
	uint32_t test_word = 0x10000; //Num records = 1 and Active config = 0
	return EEPROMProgram(&test_word, EEPROM_NUM_AND_ACTIVE_CFG_ADDR, sizeof(test_word));
}

int Nano_eeprom_SavecalibCoeffs(calibCoeffs* pCfg)
/**
 * Stores the given calibCoeffs in EEPROM. Overwrites the results of wavelength 
 * calibration. For DEBUG purposes only. USE WITH CAUTION.
 *
 * @param pCfg -I - Pointer to structure containing calibration coefficients
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
	uint32_t val = DLPSPEC_CALIB_VER;
	/* Store the data structure version number */
	EEPROMProgram(&val, EEPROM_CALIB_VER_ADDR, EEPROM_CALIB_VER_SIZE);

	return EEPROMProgram((uint32_t *)pCfg, EEPROM_CALIB_COEFF_ADDR, \
			EEPROM_CALIB_COEFF_SIZE);
}

int Nano_eeprom_GetcalibCoeffs(calibCoeffs* pCfg)
/**
 * Retrieves calibCoeffs (computed and stored during wavelength calibration) from EEPROM.
 *
 * @param pCfg -O - Pointer to structure containing calibration coefficients
 *
 * @return PASS
 */
{
	EEPROMRead((uint32_t *)pCfg, EEPROM_CALIB_COEFF_ADDR , EEPROM_CALIB_COEFF_SIZE);
	return PASS;
}

int Nano_eeprom_SaveScanNameTag(char *tag)
/**
 * Stores the given scan name tag in EEPROM.
 *
 * @param tag -I - Pointer to char array containing the name tag
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
	return EEPROMProgram((uint32_t *)tag, EEPROM_SCAN_NAME_ADDR, EEPROM_SCAN_NAME_SIZE);
}

int Nano_eeprom_GetScanNameTag(char *tag)
/**
 * Retrieves currently used scan name tag from EEPROM.
 *
 * @param tag -O - Pointer to char array containing scan name tag
 *
 * @return PASS
 */
{
	EEPROMRead((uint32_t*)tag, EEPROM_SCAN_NAME_ADDR, EEPROM_SCAN_NAME_SIZE);
	return PASS;
}

int Nano_eeprom_SaveDeviceSerialNumber(uint8_t* serial_number)
/**
 * Stores the given string as device serial number
 *
 * @param serial_number -I - Pointer to char array containing serial number
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
    return EEPROMProgram((uint32_t*)serial_number, EEPROM_SERIAL_NUMBER_ADDR, \
			EEPROM_SERIAL_NUMBER_SIZE);
}

int Nano_eeprom_GetDeviceSerialNumber(uint8_t* serial_number)
/**
 * Returns Device Serial Number
 *
 * @param serial_number -O - Pointer to char array containing serial number
 *
 * @return PASS
 */
{
	EEPROMRead((uint32_t*)serial_number, EEPROM_SERIAL_NUMBER_ADDR,\
		   	EEPROM_SERIAL_NUMBER_SIZE);
	return PASS;
}

int Nano_eeprom_SaveDeviceModelName(uint8_t* model_name)
/**
 * Stores the given string as device model name
 *
 * @param serial_number -I - Pointer to char array containing model name
 *
 * @return Error code as defined in driverlib/eeprom.h
 */
{
    return EEPROMProgram((uint32_t*)model_name, EEPROM_MODEL_NAME_ADDR, \
			EEPROM_MODEL_NAME_SIZE);
}

int Nano_eeprom_GetDeviceModelName(uint8_t* model_name)
/**
 * Returns Device Model Name
 *
 * @param serial_number -O - Pointer to char array containing model name
 *
 * @return PASS
 */
{
	EEPROMRead((uint32_t*)model_name, EEPROM_MODEL_NAME_ADDR,\
		   	EEPROM_MODEL_NAME_SIZE);
	return PASS;
}


int Nano_eeprom_StoreRefrenceCalibData(void)
/**
 * Stores the data from last scan as reference scan data in EEPROM for later use.
 * This is to be used during factory calibration operations only.
 *
 * @return error codes as per eeprom.h
 */
{
    int ret;
	uint32_t val = DLPSPEC_REFCAL_VER;

	/* Store the data structure version number */
	EEPROMProgram(&val, EEPROM_REFCAL_VER_ADDR, EEPROM_REFCAL_VER_SIZE);

	//size of each structure ceiled to the next multiple of 4
    uint32_t record_size = ((sizeof(scanData)+3)/4)*4; 
    ret = EEPROMProgram((uint32_t *)GetScanDataPtr(), EEPROM_REFCAL_DATA_ADDR,\
		   	record_size);

    return ret;
}

void Nano_eeprom_ReadReferenceCalibData(scanData *pData)
{
/**
 * Returns scanData saved off during refernce calibration
 *
 * @param pData -O - Pointer to scanData
 *
 * @return none
 */
	//size of each structure ceiled to the next multiple of 4
    uint32_t record_size = ((sizeof(scanData)+3)/4)*4; 
    EEPROMRead((uint32_t *)pData, EEPROM_REFCAL_DATA_ADDR, record_size);

    return;
}

int Nano_eeprom_ReadReferenceCalibDataVersion(uint32_t *pVer)
{
/**
 * Returns scanData saved off during refernce calibration
 *
 * @param pVer -O - Pointer to scanData
 *
 * @return none
 */
	//size of each structure ceiled to the next multiple of 4
    uint32_t record_size = EEPROM_REFCAL_VER_SIZE;
    EEPROMRead(pVer, EEPROM_REFCAL_VER_ADDR, record_size);

    return PASS;
}
