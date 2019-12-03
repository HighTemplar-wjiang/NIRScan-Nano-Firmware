/*
 * This header contains declarations related to Tiva's EEPROM for NirscanNano EVM
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef NANO_EEPROM_H_
#define NANO_EEPROM_H_

#include "dlpspec_calib.h"
#include "dlpspec_scan.h"
#include "scan.h"

// EEPROM start addr
#define EEPROM_START_ADDR 0
#define EEPROM_SIZE (6*1024)
/*After calibration result and data the space will be allocated for 7 bytes device serial number */
#define EEPROM_SERIAL_NUMBER_OFFSET 0
#define EEPROM_SERIAL_NUMBER_ADDR (EEPROM_START_ADDR + EEPROM_SERIAL_NUMBER_OFFSET)
#define EEPROM_SERIAL_NUMBER_SIZE 8
/* Scan Data Index Counter */
#define EEPROM_INDEX_COUNTER_OFFSET (EEPROM_SERIAL_NUMBER_OFFSET + EEPROM_SERIAL_NUMBER_SIZE)
#define EEPROM_INDEX_COUNTER_ADDR (EEPROM_START_ADDR + EEPROM_INDEX_COUNTER_OFFSET)
#define EEPROM_INDEX_COUNTER_SIZE 4

/* Scan Config Index Counter */
#define EEPROM_CONFIG_COUNTER_OFFSET (EEPROM_INDEX_COUNTER_OFFSET + EEPROM_INDEX_COUNTER_SIZE)
#define EEPROM_CONFIG_COUNTER_ADDR (EEPROM_START_ADDR + EEPROM_CONFIG_COUNTER_OFFSET)
#define EEPROM_CONFIG_COUNTER_SIZE 4

//calibration information will be stored in EEPROM
// preceded by the version number of the data structure
#define EEPROM_CALIB_VER_OFFSET (EEPROM_CONFIG_COUNTER_OFFSET + EEPROM_CONFIG_COUNTER_SIZE)
#define EEPROM_CALIB_VER_ADDR (EEPROM_START_ADDR + EEPROM_CALIB_VER_OFFSET)
#define EEPROM_CALIB_VER_SIZE  4

#define EEPROM_CALIB_COEFF_OFFSET (EEPROM_CALIB_VER_OFFSET + EEPROM_CALIB_VER_SIZE)
#define EEPROM_CALIB_COEFF_ADDR (EEPROM_START_ADDR + EEPROM_CALIB_COEFF_OFFSET)
#define EEPROM_CALIB_COEFF_SIZE (((sizeof(calibCoeffs)+3)/4)*4) 


/* Reference calibration data storage prepended with a version number */
#define EEPROM_REFCAL_VER_OFFSET (EEPROM_CALIB_COEFF_OFFSET + EEPROM_CALIB_COEFF_SIZE)
#define EEPROM_REFCAL_VER_ADDR (EEPROM_START_ADDR + EEPROM_REFCAL_VER_OFFSET)
#define EEPROM_REFCAL_VER_SIZE 4

#define EEPROM_REFCAL_DATA_OFFSET (EEPROM_REFCAL_VER_OFFSET + EEPROM_REFCAL_VER_SIZE)
#define EEPROM_REFCAL_DATA_ADDR (EEPROM_START_ADDR + EEPROM_REFCAL_DATA_OFFSET)
#define EEPROM_REFCAL_DATA_SIZE (((sizeof(scanData)+3)/4)*4)

#define EEPROM_SCAN_NAME_OFFSET (EEPROM_REFCAL_DATA_OFFSET + EEPROM_REFCAL_DATA_SIZE)
#define EEPROM_SCAN_NAME_ADDR (EEPROM_START_ADDR + EEPROM_SCAN_NAME_OFFSET)
#define EEPROM_SCAN_NAME_SIZE 16

/* This word in EEPROM contains the following information
 * upper 16-bits hold the number of scan cfg structs stored in EEPROM
 * lower 16-bits holds the currently active scan config index 
 */
#define EEPROM_NUM_AND_ACTIVE_CFG_OFFSET (EEPROM_SCAN_NAME_OFFSET + EEPROM_SCAN_NAME_SIZE)
#define EEPROM_NUM_AND_ACTIVE_CFG_ADDR (EEPROM_START_ADDR + EEPROM_NUM_AND_ACTIVE_CFG_OFFSET)
#define EEPROM_GET_NUM_CFGS(word) ((word) >> 16)
#define EEPROM_GET_ACTIVE_CFG_INDEX(word) (((word) << 16) >> 16)
#define EEPROM_SET_NUM_RECORDS(word, num) \
		word &= 0xFFFF; \
		word |= (num << 16)
#define EEPROM_SET_ACTIVE_CFG(word, idx) \
		word &= 0xFFFF0000; \
		word |= (idx)
#define EEPROM_NUM_AND_ACTIVE_CFG_SIZE 4

/* Scan cfg data structure version number */
#define EEPROM_CFG_VER_OFFSET (EEPROM_NUM_AND_ACTIVE_CFG_OFFSET + EEPROM_NUM_AND_ACTIVE_CFG_SIZE)
#define EEPROM_CFG_VER_ADDR (EEPROM_START_ADDR + EEPROM_CFG_VER_OFFSET)
#define EEPROM_CFG_VER_SIZE  4

#define EEPROM_SCAN_CFG_OFFSET (EEPROM_CFG_VER_OFFSET + EEPROM_CFG_VER_SIZE)
#define EEPROM_SCAN_CFG_ADDRESS (EEPROM_START_ADDR + EEPROM_SCAN_CFG_OFFSET)
//size of each structure ceiled to the next multiple of 4.
#define EEPROM_SCAN_CFG_STRUCT_SIZE (((sizeof(scanConfig)+3)/4)*4)
#define EEPROM_SLEW_SCAN_CFG_STRUCT_SIZE (((sizeof(uScanConfig)+3)/4)*4)
#define EEPROM_SCAN_CFG_SIZE (EEPROM_SLEW_SCAN_CFG_STRUCT_SIZE * EEPROM_MAX_SCAN_CFG_STORAGE)

/* Battery calibration voltage  */
#define EEPROM_BATT_CALIB_OFFSET (EEPROM_SCAN_CFG_OFFSET + EEPROM_SCAN_CFG_SIZE)
#define EEPROM_BATT_CALIB_ADDR (EEPROM_START_ADDR + EEPROM_BATT_CALIB_OFFSET)
#define EEPROM_BATT_CALIB_SIZE 4

/* Model Name */
#define EEPROM_MODEL_NAME_OFFSET (EEPROM_BATT_CALIB_OFFSET + EEPROM_BATT_CALIB_SIZE)
#define EEPROM_MODEL_NAME_ADDR (EEPROM_START_ADDR + EEPROM_MODEL_NAME_OFFSET)
#define EEPROM_MODEL_NAME_SIZE 16

/* Function declarations */

#ifdef __cplusplus
extern "C" {
#endif

int Nano_eeprom_SaveConfigRecord(uint8_t index, uScanConfig *pCfg);
int Nano_eeprom_GetConfigRecord(uint8_t index, uScanConfig *pCfg);
uint8_t Nano_eeprom_GetNumConfigRecords(void);
uint8_t Nano_eeprom_GetActiveConfigIndex(void);
int32_t Nano_eeprom_SetActiveConfig(uint32_t index);
uint8_t Nano_eeprom_GetScanConfigIndexUsingConfigID(uint16_t id);
int16_t Nano_eeprom_GetScanConfigIDUsingIndex(uint8_t index);
int Nano_eeprom_EraseAllConfigRecords(void);
void Nano_eeprom_GatherScanCfgIDs(void);
int Nano_eeprom_SetScanIndexCounter(uint16_t* scanIndexCounter);
int Nano_eeprom_GetScanIndexCounter(uint16_t* scanIndexCounter);

int Nano_eeprom_SavecalibCoeffs(calibCoeffs* calib);
int Nano_eeprom_GetcalibCoeffs(calibCoeffs* calib);
int Nano_eeprom_SaveScanNameTag(char *tag);
int Nano_eeprom_GetScanNameTag(char *tag);
int Nano_eeprom_SaveDeviceSerialNumber(uint8_t* serial_number);
int Nano_eeprom_GetDeviceSerialNumber(uint8_t* serial_number);
int Nano_eeprom_SaveDeviceModelName(uint8_t* model_number);
int Nano_eeprom_GetDeviceModelName(uint8_t* model_number);
int Nano_eeprom_StoreRefrenceCalibData(void);
void Nano_eeprom_ReadReferenceCalibData(scanData *pData);
int Nano_eeprom_ReadReferenceCalibDataVersion(uint32_t *pVer);

#ifdef __cplusplus
}
#endif

#endif // NANO_EEPROM_H_
