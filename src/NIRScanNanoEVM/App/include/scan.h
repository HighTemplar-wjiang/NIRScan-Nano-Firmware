/*
 *
 * contains declarations for scan-related functionality
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef SCAN_H_
#define SCAN_H_

#include "dlpspec_scan.h"
#include "dlpspec_scan_col.h"
#include "dlpspec_calib.h"

// !!!Note: Changing this value would require DLPC150 firmware change!!!!
// Number of sequence vector used in HW lock mode sequenes
#define HW_LOCK_MODE_NUM_SEQ_VECTORS 25
#define HW_LOCK_MODE_START_SEQ_VECT  3

/*
* Review comment - SK - OK
* Consider following one type for structure declaration
* enviro_cfg vs RefCalData vs PhotoDetVal, like use first letter caps,
* use of '_', how about using s_ to identify it is a strucuture variable?
*/
typedef struct EnvironmentalDescriptor
{
    float DetectorTemp;
    float AmbientTemp;
    float HDCTemperature;
    float HDCHum;
}enviro_cfg;

typedef struct ReferenceCalib
{
    enviro_cfg enviro_settings;
    int adc_dat[228];

}RefCalData;

typedef struct PhotoDetectorValues
{
    uint32_t red;
    uint32_t green;
    uint32_t blue;
}PhotoDetVal;

typedef enum _patternSource
{
	USE_SPL_PATTERNS_FROM_SPLASH,
	USE_PATTERNS_FROM_SDRAM,
	USE_PATTERNS_FROM_FLASH,
	USE_PATTERNS_FROM_SDCARD,
	PTN_SRC_MAX
}patternSource;

/**
 * Defines the position, dimensions, number of patterns in the set, and the
 * step size. The bit used for each pattern must be assigned through external
 * logic and is not defined as part of this structure.
 */
struct PatternDescriptor
{
	uint32_t startX;    /**< pattern set starting pixel column            */
	uint32_t startY;    /**< pattern set starting pixel row               */
	uint32_t width;     /**< width of each individual pattern in the set  */
	uint32_t height;    /**< height of each individual pattern in the set */
	uint32_t step;      /**< inter-pattern column separation              */
	uint32_t nPatterns; /**< number of patterns in the set                */
};

#ifdef __cplusplus
extern "C" {
#endif

void Scan_SetNumPatternsToScan(int numPatterns);
int Scan_SetNumRepeats(uint16_t num);
uScanData *GetScanDataPtr(void);
int Scan_SetPatternSource(int src);
void Scan_DLPCOnOffControl(bool enable);
int Scan_StoreToSDcard(void);
int Scan_SetSubImage(uint16_t startY, uint16_t height);
PhotoDetVal *Scan_GetLightSensorData(void);
uint8_t Scan_GetActiveConfigIndex(void);
int Scan_SetActiveConfig(uint8_t index);
int Scan_SetCalibPatterns(CALIB_SCAN_TYPES calib_type);
int Scan_SetConfig(uScanConfig *pCfg);
uint32_t Scan_ComputeScanTime();
int Scan_SNRDataCapture(void);
int Scan_HadSNRDataCapture(void);
int Scan_GetSectionNumADCSamplesPerPattern(int section_num, uint16_t *p_num_samples);
int16_t Scan_GetSectionNumPatterns(int section_num);
int Scan_GetVSyncPatterns(int index);
int Scan_GetFrameSyncs(int index);
uint32_t Scan_GetCurSectionExpTime(int section_num);
int Scan_SetFixedPGA(bool isFixed,uint8_t pgaVal);
int Scan_dlpc150_configure(void);
#ifdef __cplusplus
}
#endif

#endif /* SCAN_H_ */
