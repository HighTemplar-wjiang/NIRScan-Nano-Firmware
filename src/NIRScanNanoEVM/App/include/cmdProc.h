/*
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef APP_INCLUDE_CMDPROC_H_
#define APP_INCLUDE_CMDPROC_H_

#ifdef __cplusplus
extern "C" {
#endif

bool cmdFileChksum_rd();
bool cmdFileData_wr();
bool cmdFileSz_wr();
bool cmdFileListSz_rd();
bool cmdFileList_rd();
bool cmdFileSz_rd();
bool cmdFileData_rd();
bool cmdTivaBootMode_wr();
bool cmdEEPROMTest_rd();
bool cmdADCTest_rd();
bool cmdBQTest_rd();
bool cmdSDRAMTest_rd();
bool cmdDLPCEnable_wr();
bool cmdTMPTest_rd();
bool cmdHDCTest_rd();
bool cmdBTTest_wr();
bool cmdSDTest_rd();
bool cmdLEDTest_wr();
bool cmdButtonTest_rd();
bool cmdButtonTest_wr();
bool cmdEEPROMCal_wr();
bool cmdVersion_rd();
bool cmdStorePatternInSDRAM_wr();
bool cmdStartScan_wr();
bool cmdScanStatus_rd();
bool cmdStartScanInterpret_wr();
bool cmdScanInterpretStatus_rd();
bool cmdTivaReset_wr();
bool cmdPGA_wr();
bool cmdReg_wr();
bool cmdReg_rd();
bool cmdSetConfig_wr();
bool cmdSaveCfg_wr();
bool cmdScanCfg_rd();
bool cmdCfgErase_wr();
bool cmdNumCfg_rd();
bool cmdActiveCfg_rd();
bool cmdActiveCfg_wr();
bool cmdScanDLPCOnOffCtrl_wr();
bool cmdScanSubImage_wr();
bool cmdEEPROM_wipe_wr();
bool cmdPGA_rd();
bool cmdEEPROMcalibCoeffs_wr();
bool cmdEEPROMcalibCoeffs_rd();
bool cmdSNRCompute_wr();
bool cmdSNRDataSave_rd();
bool cmdCalibGenPtns_wr();
bool cmdScanNumRepeats_wr();
bool cmdHadSNRCompute_wr();
bool cmdRefCalibSave_wr();
bool cmdStartScanFlashPatterns_wr();
bool cmdSaveDeviceSerialNo_wr();
bool cmdGetDeviceSerialNo_rd();
bool cmdSaveModelName_wr();
bool cmdGetModelName_rd();
bool cmdSaveScanNameTag_wr();
bool cmdEraseScan_wr();
bool cmdEEPROM_mass_erase_wr();
bool cmdScantime_rd();
bool cmdSDDeleteLastScanFile_wr(void);
bool cmdSetHibernate_wr();
bool cmdGetHibernate_rd();
bool cmdTemp_rd();
bool cmdHum_rd();
bool cmdSetDateTime_wr();
bool cmdBattVolt_rd();
bool cmdTivaTemp_rd();
bool cmdGetDateTime_rd();
bool cmdSetPowerDown_wr();
bool cmdSDGetNumScanFiles_rd();
bool cmdPhotoDetector_rd();
bool cmdReadDeviceStat_rd();
bool cmdReadErrorStat_rd();
bool cmdResetErrorStat_rd();
bool cmdSpecificErrorStatus_rd();
bool cmdSpecificErrorCode_rd();
bool cmdClearSpecificError_wr();
bool cmdUpdateRefCalWithWORefl_wr();
bool cmdEraseDlpcFlash_wr();
bool cmdSetFixedPGAGain();

#ifdef __cplusplus
}
#endif

#endif /* APP_INCLUDE_CMDPROC_H_ */
