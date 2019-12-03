/*
 * NIRScanNano command dictionary definition
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Types.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <NNOCommandDefs.h>
#include "common.h"
#include "nnoStatus.h"
#include "cmdProc.h"
#include "cmdDict.h"

/****************************************************************************/
/* Command dictionary                                                       */
/*                                                                          */
/* Functions must be added in ascending order of command key. Within the    */
/* command key, fields are ordered by CMD2, then by CMD3, finally CMD1.     */
/****************************************************************************/

const CMD_DICT_ENTRY refDictArray[] =
{
    { NNO_CMD_FLASH_GET_CHKSUM, 		cmdFileChksum_rd            }, /* 0x0015 */
    { NNO_CMD_FLASH_GET_CHKSUM, 		cmdFileChksum_rd            }, /* 0x0015 */
    { NNO_CMD_FILE_WRITE_DATA, 			cmdFileData_wr              }, /* 0x0025 */
    { NNO_CMD_FILE_SET_WRITESIZE, 		cmdFileSz_wr                }, /* 0x002A */
    { NNO_CMD_READ_FILE_LIST_SIZE, 		cmdFileListSz_rd			}, /* 0x002B */
    { NNO_CMD_READ_FILE_LIST, 			cmdFileList_rd				}, /* 0x002C */
    { NNO_CMD_FILE_GET_READSIZE, 		cmdFileSz_rd   				}, /* 0x002D */
    { NNO_CMD_FILE_GET_DATA, 			cmdFileData_rd 				}, /* 0x002E */
    { NNO_CMD_GOTO_TIVA_BL, 			cmdTivaBootMode_wr          }, /* 0x002F */
    { NNO_CMD_EEPROM_TEST, 				cmdEEPROMTest_rd            }, /* 0x0101 */
    { NNO_CMD_ADC_TEST, 				cmdADCTest_rd               }, /* 0x0102 */
    { NNO_CMD_BQ_TEST, 					cmdBQTest_rd               	}, /* 0x0103 */
    { NNO_CMD_SDRAM_TEST, 				cmdSDRAMTest_rd             }, /* 0x0104 */
    { NNO_CMD_DLPC_ENABLE, 				cmdDLPCEnable_wr            }, /* 0x0105 */
    { NNO_CMD_TMP_TEST, 				cmdTMPTest_rd               }, /* 0x0106 */
    { NNO_CMD_HDC_TEST, 				cmdHDCTest_rd               }, /* 0x0107 */
    { NNO_CMD_BT_TEST, 					cmdBTTest_wr               	}, /* 0x0108 */
    { NNO_CMD_SDC_TEST, 				cmdSDTest_rd               	}, /* 0x0109 */
    { NNO_CMD_LED_TEST, 				cmdLEDTest_wr               }, /* 0x010B */
    { NNO_CMD_BUTTON_TEST_RD, 			cmdButtonTest_rd            }, /* 0x010C */
    { NNO_CMD_BUTTON_TEST_WR, 			cmdButtonTest_wr            }, /* 0x010D */
    { NNO_CMD_EEPROM_CAL_TEST, 			cmdEEPROMCal_wr             }, /* 0x010E */
    { NNO_CMD_TIVA_VER, 				cmdVersion_rd               }, /* 0x0216 */
    { NNO_CMD_STORE_PTN_SDRAM, 			cmdStorePatternInSDRAM_wr   }, /* 0x0217 */
    { NNO_CMD_PERFORM_SCAN, 			cmdStartScan_wr             }, /* 0x0218 */
    { NNO_CMD_SCAN_GET_STATUS, 			cmdScanStatus_rd            }, /* 0x0219 */
    { NNO_CMD_TIVA_RESET, 				cmdTivaReset_wr             }, /* 0x021A */
    { NNO_CMD_SET_PGA, 					cmdPGA_wr                  	}, /* 0x021B */
    { NNO_CMD_SET_DLPC_REG, 			cmdReg_wr                  	}, /* 0x021C */
    { NNO_CMD_GET_DLPC_REG, 			cmdReg_rd                  	}, /* 0x021D */
    { NNO_CMD_SCAN_CFG_APPLY,			cmdSetConfig_wr             }, /* 0x021E */
    { NNO_CMD_SCAN_CFG_SAVE, 			cmdSaveCfg_wr             	}, /* 0x021F */
    { NNO_CMD_SCAN_CFG_READ, 			cmdScanCfg_rd             	}, /* 0x0220 */
    { NNO_CMD_SCAN_CFG_ERASEALL,		cmdCfgErase_wr             	}, /* 0x0221 */
    { NNO_CMD_SCAN_CFG_NUM, 			cmdNumCfg_rd            	}, /* 0x0222 */
    { NNO_CMD_SCAN_GET_ACT_CFG, 		cmdActiveCfg_rd             }, /* 0x0223 */
    { NNO_CMD_SCAN_SET_ACT_CFG, 		cmdActiveCfg_wr				}, /* 0x0224 */
    { NNO_CMD_SET_DLPC_ONOFF_CTRL, 		cmdScanDLPCOnOffCtrl_wr   	}, /* 0x0225 */
    { NNO_CMD_SET_SCAN_SUBIMAGE, 		cmdScanSubImage_wr   		}, /* 0x0226 */
    { NNO_CMD_EEPROM_WIPE, 				cmdEEPROM_wipe_wr     		}, /* 0x0227 */
    { NNO_CMD_GET_PGA, 					cmdPGA_rd               	}, /* 0x0228 */
    { NNO_CMD_CALIB_STRUCT_SAVE, 		cmdEEPROMcalibCoeffs_wr     }, /* 0x0229 */
    { NNO_CMD_CALIB_STRUCT_READ, 		cmdEEPROMcalibCoeffs_rd     }, /* 0x022A */
    { NNO_CMD_START_SNRSCAN, 			cmdSNRCompute_wr            }, /* 0x022B */
    { NNO_CMD_SAVE_SNRDATA,  			cmdSNRDataSave_rd           }, /* 0x022C */
    { NNO_CMD_CALIB_GEN_PTNS, 			cmdCalibGenPtns_wr          }, /* 0x022D */
    { NNO_CMD_SCAN_NUM_REPEATS,			cmdScanNumRepeats_wr        }, /* 0x022E */
    { NNO_CMD_START_HADSNRSCAN, 		cmdHadSNRCompute_wr         }, /* 0x022F */
    { NNO_CMD_REFCAL_PERFORM, 			cmdRefCalibSave_wr          }, /* 0x0230 */
    { NNO_CMD_PERFORM_SCAN_FLASH_PTNS, 	cmdStartScanFlashPatterns_wr}, /* 0x0231 */
    { NNO_CMD_SERIAL_NUMBER_WRITE,		cmdSaveDeviceSerialNo_wr    }, /* 0x0232 */
    { NNO_CMD_SERIAL_NUMBER_READ, 		cmdGetDeviceSerialNo_rd     }, /* 0x0233 */
    { NNO_CMD_WRITE_SCAN_NAME_TAG,		cmdSaveScanNameTag_wr		}, /* 0x0234 */
    { NNO_CMD_DEL_SCAN_FILE_SD,			cmdEraseScan_wr				}, /* 0x0235 */
    { NNO_CMD_EEPROM_MASS_ERASE, 		cmdEEPROM_mass_erase_wr     }, /* 0x0236 */
	{ NNO_CMD_READ_SCAN_TIME,			cmdScantime_rd				}, /* 0x0237 */
    { NNO_CMD_DEL_LAST_SCAN_FILE_SD, 	cmdSDDeleteLastScanFile_wr  }, /* 0x0238 */
    { NNO_CMD_START_SCAN_INTERPRET,     cmdStartScanInterpret_wr    }, /* 0x0239 */
    { NNO_CMD_SCAN_INTERPRET_GET_STATUS, cmdScanInterpretStatus_rd   }, /* 0x023A */
    { NNO_CMD_MODEL_NAME_WRITE,         cmdSaveModelName_wr         }, /* 0x023B */
    { NNO_CMD_MODEL_NAME_READ,          cmdGetModelName_rd          }, /* 0x023C */
    { NNO_CMD_READ_TEMP, 				cmdTemp_rd	         		}, /* 0x0301 */
    { NNO_CMD_READ_HUM, 				cmdHum_rd	         		}, /* 0x0302 */
    { NNO_CMD_SET_DATE_TIME, 			cmdSetDateTime_wr         	}, /* 0x0309 */
    { NNO_CMD_READ_BATT_VOLT, 			cmdBattVolt_rd	         	}, /* 0x030A */
    { NNO_CMD_READ_TIVA_TEMP, 			cmdTivaTemp_rd	         	}, /* 0x030B */
    { NNO_CMD_GET_DATE_TIME,			cmdGetDateTime_rd         	}, /* 0x030C */
    { NNO_CMD_HIBERNATE_MODE, 			cmdSetPowerDown_wr          }, /* 0x030D */
	{ NNO_CMD_SET_HIBERNATE,			cmdSetHibernate_wr			}, /* 0x030E */
	{ NNO_CMD_GET_HIBERNATE,			cmdGetHibernate_rd			}, /* 0x030F */
    { NNO_CMD_GET_NUM_SCAN_FILES_SD,	cmdSDGetNumScanFiles_rd   	}, /* 0x0400 */
    { NNO_CMD_READ_PHOTODETECTOR, 		cmdPhotoDetector_rd         }, /* 0x0402 */
	{ NNO_CMD_READ_DEVICE_STATUS,		cmdReadDeviceStat_rd,		}, /* 0x0403 */
	{ NNO_CMD_READ_ERROR_STATUS,		cmdReadErrorStat_rd,		}, /* 0x0404 */
	{ NNO_CMD_RESET_ERROR_STATUS,		cmdResetErrorStat_rd,		}, /* 0x0405 */
	{ NNO_CMD_GET_SPECIFIC_ERR_STATUS,	cmdSpecificErrorStatus_rd,	}, /* 0x0406 */
	{ NNO_CMD_GET_SPECIFIC_ERR_CODE,	cmdSpecificErrorCode_rd,	}, /* 0x0407 */
	{ NNO_CMD_CLEAR_SPECIFIC_ERR,		cmdClearSpecificError_wr,	},  /* 0x0408 */
    { NNO_CMD_UPDATE_REFCALDATA_WOREFL,  cmdUpdateRefCalWithWORefl_wr,},   /*0x040A */
	{ NNO_CMD_ERASE_DLPC_FLASH,			cmdEraseDlpcFlash_wr,		},	/* 0x040B */
    { NNO_CMD_SET_FIXED_PGA,            cmdSetFixedPGAGain,         }  /* 0x040C */
};

static size_t refDictSize = sizeof( refDictArray ) / sizeof( CMD_DICT_ENTRY );

/****************************************************************************/
/* Locate and vector to command handler.                                    */
/****************************************************************************/

CMD1_TYPE cmdDict_Vector(CMD1_TYPE ctype, uint32_t key)
{
	size_t low = 0;
	size_t high = refDictSize; /* size of the dictionary */
	size_t probe;
	const CMD_DICT_ENTRY *pDict = refDictArray;

	while ((high - low) > 1) /* binary search */
	{
		probe = (high + low) / 2;

		if (pDict[probe].key > key)
			high = probe;
		else
			low = probe;
	}

	if ((low == 0) || (pDict[low].key != key))
		return CMD1_NACK;
	else if ((*pDict[low].pFunc)()) /* exec */
		return ctype; /* no handler error */
	else
		return CMD1_ACK; /* if handler error */
}

