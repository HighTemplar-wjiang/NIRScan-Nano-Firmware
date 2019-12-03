/*
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Types.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <ti/drivers/I2C.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_nvic.h>
#include <driverlib/eeprom.h>
#include <driverlib/usb.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/ssi.h>
#include <driverlib/hibernate.h>
#include <driverlib/epi.h>
#include <driverlib/i2c.h>
#include <NNOCommandDefs.h>

#include "common.h"
#ifdef NIRSCAN_INCLUDE_BLE
#include "BLECommonDefs.h"
#include "bleCmdHandler.h"
#endif
#include "cmdHandlerIFMgr.h"
#include "usbCmdHandler.h"
#include "flash.h"
#include "sdram.h"
#include "scan.h"
#include "adcWrapper.h"
#include "bq24250.h"
#include "sdram.h"
#include "NIRscanNano.h"
#include "version.h"
#include "Board.h"
#include "dlpc150.h"
#include "tmp006.h"
#include "hdc1000.h"
#include "battery.h"
#include "fatsd.h"
#include "display.h"
#include "nano_eeprom.h"
#include "dlpspec_helper.h"
#include "dlpspec_scan.h"
#include "dlpspec_calib.h"
#include "dlpspec_version.h"
#include "refCalMatrix.h"
#include "led.h"
#include "NNOSNRDefs.h"
#include "nnoStatus.h"
#ifdef NIRSCAN_INCLUDE_BLE
#include "BLENotificationHandler.h"
#endif

#include "NIRscanNano.h"
#include "nano_timer.h"
#include "cmdProc.h"
#include "calConstants.h"

#ifdef STAND_ALONE_SCAN_DEBUG
#include "testimages.h"
#endif

extern uint8_t		g_TestOnOffButtonPresses;			// Number of consecutive button presses
extern uint8_t		g_TestScanButtonPresses;
extern uint8_t		g_ButtonTestMode;				// Button Test Mode
extern uint32_t  	g_ui32UnderflowCount;
extern snrData SNRData;
extern snrDataHad SNRDataHad;
extern bool BLEStatus;
bool g_fixedPGA = false;

static scanData tempScanData;
static uint32_t fileSize=0;
static uint32_t dataChkSum = 0;
static uint16_t fileAction = 0xFF;
static uint32_t bytesToSend;
static uint32_t bytesSent=0;
static int8_t *pSDRAMFrameData;
static uint32_t dlpc150_sw_ver = 0xFFFFFFFF;
static uint32_t dlpc150_flash_ver = 0xFFFFFFFF;
static uint8_t *pUsbDataPtr = NULL;
static scanResults scan_results;
static uint8_t tempBuffer[ADC_DATA_LEN * sizeof(float) + ADC_DATA_LEN * sizeof(int)];




extern int SNR_HadArr[HADSNR_NUM_DATA][HADSNR_LENGTH];

uint8_t g_dataBlob[SCAN_DATA_BLOB_SIZE];
uint32_t g_patternNumCols, g_patternNumRows;
/*
 * Slit focus - 0 through 13
 * Detector Alignment - 14
 * Cal images full DMD - 15 to 68
 * Cal images top - 69 to 122
 * Cal images middle - 123 to 176
 * Cal images bottom - 177 to 230
 */
static uint32_t NumSplashPerScan[6] = { 14, 1, 54, 54, 54, 54};

#ifdef NIRSCAN_INCLUDE_BLE
extern BLE_CMD_HANDLER_RESPONSE gBLECmdHandlerRepsonse;
#endif

extern uint16_t g_scanConfigIDs[EEPROM_MAX_SCAN_CFG_STORAGE];
extern unsigned int g_scanIndices[NUM_SCAN_DATA_INDEX];

bool cmdFileChksum_rd( void )                    /* 0x0015 */
{
	dataChkSum = flash_spi_compute_checksum(0, fileSize);
	cmdPut4( dataChkSum );

	NIRscanNano_DLPCEnable(true);
    DEBUG_PRINT("dataChkSum = %x\n", dataChkSum);

    return true;
}

bool cmdFileData_wr( void )						 /* 0x0025 */
{
	uint32_t i;
	uint8_t *pData = (uint8_t *)cmdGetPtrRead();

	if(fileAction == NNO_FILE_DLPC_UPDATE)
	{
		for(i=0; i<nRemReadPC; i++)
		{
			dataChkSum += *(pData+i);
		}

		if(flash_spi_program(pData, nRemReadPC) != PASS)
			return false;
	}
	else if(fileAction == NNO_FILE_REFCAL_DATA)
			{
				uint32_t val = DLPSPEC_REFCAL_VER;

				/* Store the data structure version number */
				EEPROMProgram(&val, EEPROM_REFCAL_VER_ADDR, EEPROM_REFCAL_VER_SIZE);

				//size of each structure ceiled to the next multiple of 4
				uint32_t record_size = ((sizeof(scanData)+3)/4)*4;
				EEPROMProgram((uint32_t *)pData, EEPROM_REFCAL_DATA_ADDR,\
						   	record_size);

			}
	else
	{
		memcpy(pSDRAMFrameData, pData, nRemReadPC);
		pSDRAMFrameData += nRemReadPC;
	}

	return true;
}

bool cmdFileSz_wr( void )						 /* 0x002A */
{
	fileSize = cmdGet4(uint32_t);
	fileAction = cmdGet2(uint16_t);
	dataChkSum = 0;

	if(fileAction == NNO_FILE_DLPC_UPDATE)
	{
		NIRscanNano_DLPCEnable(false);
		if(flash_spi_init() != PASS)
			return false;
		if(flash_spi_program_init() != PASS)
			return false;
		//Make sure dlpc150 versions are reset so that they are read again after the update
		dlpc150_sw_ver = 0xFFFFFFFF;
		dlpc150_flash_ver = 0xFFFFFFFF;
	}

	else if(fileAction >= NNO_FILE_PTN_LOAD_SDRAM)
	{
		pSDRAMFrameData = (int8_t *)(SDRAM_START_ADDRESS + (fileAction-NNO_FILE_PTN_LOAD_SDRAM)*(DISP_WIDTH * DISP_HEIGHT * 3));
	}

	return true;
}

bool cmdFileListSz_rd()
{
	int8_t file_type = cmdGet1(uint8_t);
	uint32_t num_records = 0;
	FRESULT SDstatus;

#ifdef NIRSCAN_INCLUDE_BLE
	uint16_t scanIdarraySize = 0;
	uint16_t bleScanDataArSize = 0;
#endif
	int scanDataIndexArrayLength = 0;

	num_records = Nano_eeprom_GetNumConfigRecords();

	/*
	 * Review comment - AL - Add a scan error code
	 * Please consider adding an error code below and updating error status
	 */
	if(num_records==0)
	    return false;

	SDstatus = FATSD_FindListScanIndex(&scanDataIndexArrayLength);
	if( ( SDstatus != PASS) && (SDstatus != SDCARD_ERROR_NOT_READY) )
		return false;

	Nano_eeprom_GatherScanCfgIDs();

#ifdef NIRSCAN_INCLUDE_BLE
	scanIdarraySize = sizeof(uint16_t)*num_records;

	if (file_type == NNO_FILE_SCAN_CONFIG_LIST)
	{
		if (isBLEConnActive())
	    {
	        g_dataBlob[0] = (0xff & scanIdarraySize);
	        g_dataBlob[1] = (0xff00 & scanIdarraySize) >> 8;
	        g_dataBlob[2] = 0;
	        g_dataBlob[3] = 0;
	        cmdPut(4, &g_dataBlob[0]);
	        DEBUG_PRINT("\r\nReturned size = %d\r\n",scanIdarraySize);
	    }

#ifdef BLE_USE_TEST_VALUES
		g_dataBlob[0] = 0x02;	//indicates two bytes - one entry
		g_dataBlob[1] = 0x00;
		g_dataBlob[2] = 0x00;
		g_dataBlob[3] = 0x00;
		cmdPut(4, &g_dataBlob[0]);
		DEBUG_PRINT("\r\nReturned size = 2\r\n");
#endif
	}
	else if (file_type == NNO_FILE_SCAN_LIST)
	{
		if (isBLEConnActive())
		{
			int8_t val_type = cmdGet1(uint8_t);	//indicates if bytes required or num entries in list

			if (val_type == BLE_LIST_RETURN_TYPE_BYTE)
    			bleScanDataArSize = sizeof(unsigned int)*scanDataIndexArrayLength;
    		else
    			bleScanDataArSize = scanDataIndexArrayLength;

			g_dataBlob[0] = (0xff & bleScanDataArSize);
			g_dataBlob[1] = (0xff00 & bleScanDataArSize) >> 8;
			g_dataBlob[2] = 0;
			g_dataBlob[3] = 0;
			cmdPut(4, &g_dataBlob[0]);

			DEBUG_PRINT("\r\nReturned size = %d\r\n",sizeof(uint32_t)*scanDataIndexArrayLength);
		}
#ifdef BLE_USE_TEST_VALUES
		if (val_type == bleListReturnType_num_entries)
			g_dataBlob[0] = 0x01;
		else
			g_dataBlob[0] = 0x04;
		g_dataBlob[1] = 0x00;
		g_dataBlob[2] = 0x00;
		g_dataBlob[3] = 0x00;
		cmdPut(4, &g_dataBlob[0]);
		DEBUG_PRINT("\r\nReturned size = 4\r\n");
#endif
	}
#endif

	return true;
}

bool cmdFileList_rd()
{
#ifdef NIRSCAN_INCLUDE_BLE
	int8_t file_type = cmdGet1(uint8_t);
	uint32_t num_records;
	uint16_t scanIdarraySize;
	int scanDataIndexArrayLength = 0;
	FRESULT SDstatus;


	if (isBLEConnActive() != true)
		return false;


	SDstatus = FATSD_FindListScanIndex(&scanDataIndexArrayLength);
	if( ( SDstatus != PASS) && (SDstatus != SDCARD_ERROR_NOT_READY) )
		return false;
	
	num_records = Nano_eeprom_GetNumConfigRecords();

	if(num_records==0)
	   return false;

	scanIdarraySize = sizeof(uint16_t)*num_records;

	if (file_type == NNO_FILE_SCAN_CONFIG_LIST)
	{
	   cmdPut(scanIdarraySize, &g_scanConfigIDs[0]);
	   DEBUG_PRINT("\r\nReturned size = %d\r\n",scanIdarraySize);

#ifdef BLE_USE_TEST_VALUES
	   g_dataBlob[0] = 0x00;
	   g_dataBlob[1] = 0x00;
	   cmdPut(2, &g_dataBlob[0]);
	   DEBUG_PRINT("\r\nReturned data = 0xfa01\r\n");
#endif
	}
	else if (file_type == NNO_FILE_SCAN_LIST)
	{
		int8_t val_type = cmdGet1(uint8_t);	//indicates if bytes required or num entries in list

		if (val_type == BLE_LIST_RETURN_TYPE_BYTE)
		{
			cmdPut(sizeof(unsigned int)*scanDataIndexArrayLength, &g_scanIndices[0]);
		}
		else
		{
			g_dataBlob[0] = (0xff & scanDataIndexArrayLength);
			g_dataBlob[0] = (0xff00 & scanDataIndexArrayLength) >> 8;
			g_dataBlob[0] = (0xff0000 & scanDataIndexArrayLength) >> 16;
			g_dataBlob[0] = (0xff000000 & scanDataIndexArrayLength) >> 24;
			cmdPut(sizeof(unsigned int), &g_dataBlob[0]);
		}

		DEBUG_PRINT("\r\nReturned size = %d\r\n",sizeof(uint32_t)*scanDataIndexArrayLength);
#ifdef BLE_USE_TEST_VALUES
		g_dataBlob[0] = 0x00;
		g_dataBlob[1] = 0x00;
		cmdPut(2, &g_dataBlob[0]);
		DEBUG_PRINT("\r\nReturned data = 0xfb01\r\n");
#endif
	}
	return true;
#else
	return false;
#endif
}

bool cmdFileSz_rd(void)
{
	refCalMatrix tmpRefCalMatrix;
	int8_t file_type = cmdGet1(uint8_t);
#ifdef NIRSCAN_INCLUDE_BLE
	uint32_t scanDataIndex = 0;
	uint8_t field_type = 0;
#endif
	int result = PASS;
	int i;
	uint8_t *pBuffer;

	if (file_type == NNO_FILE_SCAN_DATA)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())
		{
			scanDataIndex = cmdGet4(uint32_t);
			field_type = cmdGet1(uint8_t);

			if (field_type == BLE_SCAN_DATA_FIELD_NAME)
			{
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_NAME;
				g_dataBlob[0] = SCAN_NAME_LEN;
				g_dataBlob[1] = 0x00;
				g_dataBlob[2] = 0x00;
				g_dataBlob[3] = 0x00;
				cmdPut(4, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_TYPE)
			{
				g_dataBlob[0] = sizeof(uint8_t);
				g_dataBlob[1] = 0x00;
				g_dataBlob[2] = 0x00;
				g_dataBlob[3] = 0x00;
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_TYPE;
				cmdPut(4, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_TIME)
			{
				g_dataBlob[0] = sizeof(uint8_t) * 7;
				g_dataBlob[1] = 0x00;
				g_dataBlob[2] = 0x00;
				g_dataBlob[3] = 0x00;
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_TIME;
				cmdPut(4, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_BLOB_VER)
			{
				g_dataBlob[0] = 4 * sizeof(uint32_t);
				g_dataBlob[1] = 0x00;
				g_dataBlob[2] = 0x00;
				g_dataBlob[3] = 0x00;
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_BLOB_VER;
				cmdPut(4, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_BLOB)
			{
				if (scanDataIndex == GetScanDataPtr()->data.scanDataIndex)
					result = dlpspec_scan_write_data(GetScanDataPtr(), g_dataBlob, SCAN_DATA_BLOB_SIZE);

				if (result == PASS)
					bytesToSend = SCAN_DATA_BLOB_SIZE;
				else
				{
					bytesToSend = 0;
					bleNotificationHandler_sendErrorIndication(NNO_ERROR_SPEC_LIB, result);
				}

				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_BLOB;
				cmdPut(sizeof(unsigned int), &bytesToSend);
			}
		}
		else
		{
#endif
			bytesSent = 0;
			result = dlpspec_scan_write_data(GetScanDataPtr(), g_dataBlob, SCAN_DATA_BLOB_SIZE);
			if (PASS == result)
				bytesToSend = SCAN_DATA_BLOB_SIZE;
			else
			{
				bytesToSend = 0;
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
			}
			cmdPut4(bytesToSend);
			pUsbDataPtr = &g_dataBlob[0];
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif
	}
	else if (file_type == NNO_FILE_SCAN_CONFIG)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())
		{
			g_dataBlob[0] = (sizeof(uScanConfig) & 0xff);
			g_dataBlob[1] = (sizeof(uScanConfig) & 0xff00) >> 8;
			g_dataBlob[2] = (sizeof(uScanConfig) & 0xff0000) >> 16;
			g_dataBlob[3] = (sizeof(uScanConfig) & 0xff000000) >> 24;
			cmdPut(sizeof(uint32_t), &g_dataBlob[0]);
		}
#endif
	}
	else if (file_type == NNO_FILE_REF_CAL_DATA)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())
		{
			Nano_eeprom_ReadReferenceCalibData(&tempScanData);
			result = dlpspec_scan_write_data((uScanData *)&tempScanData, g_dataBlob, SCAN_DATA_BLOB_SIZE);

			if (PASS == result)
				bytesToSend = SCAN_DATA_BLOB_SIZE;
			else
			{
				bytesToSend = 0;
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
			}
			DEBUG_PRINT("\r\nRef scan data serialization done\r\n");

			cmdPut(sizeof(unsigned int), &bytesToSend);
		}
		else
		{
#endif
			bytesSent = 0;
			Nano_eeprom_ReadReferenceCalibData(&tempScanData);
			result = dlpspec_scan_write_data((uScanData *)&tempScanData, g_dataBlob, SCAN_DATA_BLOB_SIZE);
			if (PASS == result)
				bytesToSend = SCAN_DATA_BLOB_SIZE;
			else
			{
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
				bytesToSend = 0;
			}

			cmdPut4(bytesToSend);
			pUsbDataPtr = &g_dataBlob[0];
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif
	}
	else if (file_type == NNO_FILE_REF_CAL_MATRIX)
	{
		memcpy(tmpRefCalMatrix.width, refCalMatrix_widths, sizeof(uint8_t)*REF_CAL_INTERP_WIDTH);
		memcpy(tmpRefCalMatrix.wavelength, refCalMatrix_wavelengths, sizeof(double)*REF_CAL_INTERP_WAVELENGTH);
		memcpy(tmpRefCalMatrix.ref_lookup, refCalMatrix_intensities, sizeof(uint16_t)*REF_CAL_INTERP_WIDTH*REF_CAL_INTERP_WAVELENGTH);

		//Scan data size is greater, so reusing the same to save memory
		result = dlpspec_calib_write_ref_matrix(&tmpRefCalMatrix, &g_dataBlob[0], REF_CAL_MATRIX_BLOB_SIZE);

		if (PASS == result)
			bytesToSend = REF_CAL_MATRIX_BLOB_SIZE;
		else
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
			bytesToSend = 0;
			return false;
		}
#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())
		{
			cmdPut(sizeof(unsigned int), &bytesToSend);
		}
		else	// USB
		{
#endif
			bytesSent = 0;
			pUsbDataPtr = &g_dataBlob[0];
			cmdPut4(bytesToSend);
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif

	}
	else if (file_type == NNO_FILE_SCAN_DATA_FROM_SD)
	{
		bytesSent = 0;
#ifdef NIRSCAN_INCLUDE_BLE
		if (!isBLEConnActive())
		{
#endif
			pUsbDataPtr = &g_dataBlob[0];
			if (FR_OK == FATSD_ReadLastStoredScanFile((void *)pUsbDataPtr, &bytesToSend))
				cmdPut4(bytesToSend);
			else
				cmdPutK4(0);
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif
	}
	else if (file_type == NNO_FILE_HADSNR_DATA)
	{
		bytesSent = 0;
	    bytesToSend = HADSNR_NUM_DATA * HADSNR_LENGTH * sizeof(int);
	    cmdPut4(bytesToSend);
		pUsbDataPtr = (uint8_t *)&SNR_HadArr[0][0];
	}
	else if ( file_type == NNO_FILE_INTERPRET_DATA )
	{
		bytesToSend = sizeof(int) + scan_results.length * sizeof(double) + scan_results.length * sizeof(int);
		cmdPut4( bytesToSend );
		pBuffer = &tempBuffer[0];
		*((int *)pBuffer) = (int) scan_results.length;
		pBuffer += sizeof(int);
		for ( i = 0; i < scan_results.length; i++ ) {
			*((double *)pBuffer) = (double) scan_results.wavelength[i];
			pBuffer += sizeof(double);
			*((int *)pBuffer) = (int) scan_results.intensity[i];
			pBuffer += sizeof(int);
		}			
		pUsbDataPtr = &tempBuffer[0];
	}

	return true;
}

bool cmdFileData_rd(void)
{
	int i;
	size_t max_data_size = 0;

#ifdef NIRSCAN_INCLUDE_BLE
	uint32_t scanDataIndex = 0;
	uint8_t field_type = 0;
	int8_t file_type = 0;
	scanData *scan_data = NULL;
	uint8_t index = 0;
	FRESULT fatresult = FR_OK;
	unsigned short length=0;

	if (isBLEConnActive())
	{
		file_type = cmdGet1(uint8_t);

		DEBUG_PRINT("\r\n********File type:%d\r\n",file_type);
		if(file_type == NNO_FILE_REF_CAL_DATA)
		{
			cmdPut(SCAN_DATA_BLOB_SIZE, &g_dataBlob[0]);
		}
		else if(file_type == NNO_FILE_REF_CAL_MATRIX)
		{
			cmdPut(REF_CAL_MATRIX_BLOB_SIZE, &g_dataBlob[0]);
		}
		else if (file_type == NNO_FILE_SCAN_CONFIG)
		{
			uint16_t configID = cmdGet2(uint16_t);
			uScanConfig tempcfg;

			index = Nano_eeprom_GetScanConfigIndexUsingConfigID(configID);

			if (0 > Nano_eeprom_GetConfigRecord(index, &tempcfg))
				return false;

			memcpy(&g_dataBlob[0], (const void *)&tempcfg, sizeof(uScanConfig));

			cmdPut(sizeof(uScanConfig), &g_dataBlob[0]);
		}
		else if (file_type == NNO_FILE_SCAN_DATA)
		{
			scanDataIndex = cmdGet4(uint32_t);
			field_type = cmdGet1(uint8_t);

			bool isCurrScan = (scanDataIndex == GetScanDataPtr()->data.scanDataIndex) ? true : false;

			if (!isCurrScan)	//Read from SD card
			{
				fatresult = FATSD_ReadScanFile(scanDataIndex, (void *) &g_dataBlob, &bytesToSend);
				if (FR_OK != fatresult)
				{
					DEBUG_PRINT("\r\nMatching scan data not found in SD card\r\n");
					gBLECmdHandlerRepsonse.subFileType = field_type;
					nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fatresult);
					return (false);
				}

				if (field_type != BLE_SCAN_DATA_FIELD_BLOB)
				{
					dlpspec_deserialize((void *)g_dataBlob, SCAN_DATA_BLOB_SIZE, SCAN_DATA_TYPE);
					scan_data = (scanData *)&g_dataBlob[0];
				}
			}
			else
				scan_data = (scanData *)GetScanDataPtr();

			if (field_type == BLE_SCAN_DATA_FIELD_NAME)
			{
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_NAME;
				memcpy(&g_dataBlob[0], scan_data->scan_name,SCAN_NAME_LEN);
				cmdPut(SCAN_NAME_LEN, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_TYPE)
			{
				g_dataBlob[0] = scan_data->scan_type;
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_TYPE;
				cmdPut(1, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_TIME)
			{
				g_dataBlob[0] = scan_data->year;
				g_dataBlob[1] = scan_data->month;
				g_dataBlob[2] = scan_data->day;
				g_dataBlob[3] = scan_data->day_of_week;
				g_dataBlob[4] = scan_data->hour;
				g_dataBlob[5] = scan_data->minute;
				g_dataBlob[6] = scan_data->second;

				cmdPut(7, &g_dataBlob[0]);
				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_TIME;
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_BLOB_VER)
			{
				g_dataBlob[0] = (0xff & scan_data->header_version);
				g_dataBlob[1] = (0xff00 & scan_data->header_version) >> 8;
				g_dataBlob[2] = (0xff0000 & scan_data->header_version) >> 16;
				g_dataBlob[3] = (0xff000000 & scan_data->header_version) >> 24;

				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_BLOB_VER;
				cmdPut(4, &g_dataBlob[0]);
			}
			else if (field_type == BLE_SCAN_DATA_FIELD_BLOB)
			{
				if (isCurrScan)
				{
					i = dlpspec_scan_write_data(GetScanDataPtr(), g_dataBlob, SCAN_DATA_BLOB_SIZE);
					DEBUG_PRINT("\r\nScan data serialization done\r\n");

					if (PASS == i)
						length = SCAN_DATA_BLOB_SIZE;
					else
					{
						nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, i);
						length = 0;
					}
				}

				gBLECmdHandlerRepsonse.subFileType = BLE_SCAN_DATA_FIELD_BLOB;
				if (length > 0)
					cmdPut(length, &g_dataBlob[0]);
			}
		}
	}
	else
	{
#endif
		max_data_size = getMaxDataLimit();

		for(i=0; i<MIN(max_data_size, bytesToSend); i++)
			cmdPut1(pUsbDataPtr[bytesSent+i]);

		bytesSent += MIN(max_data_size, bytesToSend);
		bytesToSend -= MIN(max_data_size, bytesToSend);
#ifdef NIRSCAN_INCLUDE_BLE
		}
#endif

	return true;
}

bool cmdTivaBootMode_wr(void)
{
#ifdef TARGET_IS_TM4C129_RA0
	uint32_t ui32SysClock;
#if 0
	uint8_t pUSBBootROMInfo[] =
	{
	0x51, 0x04, // DLP VID
	0x50, 0x01, // DLPC150 PID
	0x00, 0x02, // USB version 2.0
	0x00, // 0mA of Bus power
	0xC0, // Self powered using no bus power
	0 // Address of the string table
	};
#endif

	//
	// Run from the PLL at 120 MHz.
	//
#if 0 //Commented off for now to use internal oscillator
	ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
										   SYSCTL_OSC_MAIN | SYSCTL_USE_PLL |
										   SYSCTL_CFG_VCO_480), NIRSCAN_SYSCLK);
#else
	ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_OSC_INT |
											    SYSCTL_USE_PLL |
											   SYSCTL_CFG_VCO_480), NIRSCAN_SYSCLK);
#endif

	MAP_USBDevDisconnect(USB0_BASE); //Disconnect application USB feature from the bus

	//
	    // Disable all interrupts.
	    //
	    ROM_IntMasterDisable();

	    //
	    // Disable SysTick and its interrupt.
	    //
	    ROM_SysTickIntDisable();
	    ROM_SysTickDisable();

	    //
	    // Disable all processor interrupts.  Instead of disabling them one at a
	    // time, a direct write to NVIC is done to disable all peripheral
	    // interrupts.
	    //
	    HWREG(NVIC_DIS0) = 0xffffffff;
	    HWREG(NVIC_DIS1) = 0xffffffff;
	    HWREG(NVIC_DIS2) = 0xffffffff;
	    HWREG(NVIC_DIS3) = 0xffffffff;
	    HWREG(NVIC_DIS4) = 0xffffffff;

	    //
	    // Enable and reset the USB peripheral.
	    //
	    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
	    ROM_SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);
	    ROM_USBClockEnable(USB0_BASE, 8, USB_CLOCK_INTERNAL);

	    //
	    // Wait for about a second.
	    //
	    ROM_SysCtlDelay(ui32SysClock / 3);

	    //
	    // Re-enable interrupts at the NVIC level.
	    //
	    ROM_IntMasterEnable();

	//ROM_UpdateUSB(pUSBBootROMInfo);
	    ROM_UpdateUSB(0);
#endif
	return true;
}

/* ******************************************************************************************** */
/* Command functions corresponding to EVM test functions exposed in test tab of the GUI - START */
/* ******************************************************************************************** */

bool cmdEEPROMTest_rd(void)
{
	uint32_t test_word = 0x050AFF00;
	uint32_t orig_word;
	int8_t result = 0;

	//Save original contents of the test location
	EEPROMRead(&orig_word, 1024, 4);
	//Write test word
	EEPROMProgram(&test_word, 1024, 4);

	test_word = 0;
	//Read back and verify
	EEPROMRead(&test_word, 1024, 4);

	//Restore the contents of test location
	EEPROMProgram(&orig_word, 1024, 4);

	if(test_word != 0x050AFF00)
		result = -1;

	cmdPut1(result);

	return true;
}

bool cmdADCTest_rd(void)
{
	int32_t test_result;

	MAP_GPIOPinWrite( GPIO_PORTH_BASE, GPIO_PIN_6, 0 );   // Set SYNC low for at least 20 DRDY periods to power down the ADC
	MAP_SysCtlDelay(DELAY_20MS);
	MAP_GPIOPinWrite( GPIO_PORTH_BASE, GPIO_PIN_6, GPIO_PIN_6 );   // Set SYNC high to bring ADC back up

	test_result = adc_SelfTest();

	cmdPut1(test_result);
	return true;
}

bool cmdBQTest_rd(void)
{
	int32_t ret_val;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C9);
	MAP_SysCtlDelay(10);                                        // Wait six 10 clock cycles to ensure peripherals ready
    MAP_I2CMasterInitExpClk( I2C9_BASE, SysCtlClockGet(), 1 );	// Set bus speed to 400KHz and enable I2C block

	ret_val = bq24250_readStatus();
	cmdPut4(ret_val);
	ret_val = bq24250_readBatteryVoltage();
	cmdPut4(ret_val);
	ret_val = bq24250_readUSBDetection();
	cmdPut4(ret_val);
	ret_val = bq24250_readChargeCurrent();
	cmdPut4(ret_val);
	ret_val = bq24250_readTSFault();
	cmdPut4(ret_val);

    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C9);
	return true;
}

bool cmdSDRAMTest_rd(void)
{
	int32_t ret_val;

	ret_val = sdram_test();

	cmdPut1(ret_val);	//0 means test passed, -1 means fail

	return true;
}

bool cmdDLPCEnable_wr(void)
{
	uint8_t start;
	uint8_t lamp_en;

	start = cmdGet1(uint8_t);
	lamp_en = cmdGet1(uint8_t);

	if(start)
	{
		if(NIRscanNano_DLPCEnable(true) != PASS)
			return false;

		//Prepare DLPC150 for scans
		if(Scan_dlpc150_configure() == FAIL)
			return false;

		if(lamp_en)
		{
#ifdef TIVA_LAMP_CONTROL
			if(dlpc150_LampEnable(false) != PASS)
				return false;
            if(NIRscanNano_LampEnable(true) != PASS)
            	return false;
#else
			if(dlpc150_LampEnable(true) != PASS)
				return false;
#endif
		}
	}
	else
	{
		if(NIRscanNano_DLPCEnable(false) != PASS)
			return false;
#ifdef TIVA_LAMP_CONTROL
		if(NIRscanNano_LampEnable(false) != PASS)
			return false;
#endif
	}
	return true;
}

bool cmdTMPTest_rd(void)
{
	int8_t test_result;

	test_result = tmp006_test();

	cmdPut1(test_result);
	return true;
}

bool cmdHDCTest_rd(void)
{
	int8_t test_result;

	test_result = hdc1000_test();

	cmdPut1(test_result);

	return true;
}

bool cmdBTTest_wr(void)
{
#ifdef	NIRSCAN_INCLUDE_BLE
	uint8_t start;

	start = cmdGet1(uint8_t);

	if(start)
	{
		Semaphore_post( BLEStartSem );
		BLEStatus = true;
	}
	else
	{

		Semaphore_post( BLEEndSem );
		BLEStatus = false;
	}

	return true;
#else
	return false;
#endif
}

bool cmdSDTest_rd(void)
{
	int32_t test_result;

	test_result = FATSD_Test();

	cmdPut1((int8_t)test_result);

	return true;
}

bool cmdLEDTest_wr(void)
{
	uint8_t start;
	start = cmdGet1(uint8_t);
	if ( start )
	{
		Timer_stop( NanoTimer2 );
		greenLED_on();
		yellowLED_on();
		blueLED_on();
	}
	else
	{
		greenLED_off();
		yellowLED_off();
		blueLED_off();
		Timer_start( NanoTimer2 );
	}
	return true;
}

bool cmdButtonTest_rd(void)
{
    uint8_t buttons = 0;

    if ( g_TestScanButtonPresses )
        buttons |= 0x1;
    if ( g_TestOnOffButtonPresses )
        buttons |= 0x2;
    cmdPut1( buttons );
    return true;
}

bool cmdButtonTest_wr(void)
{
    uint8_t start;

    start = cmdGet1(uint8_t);
    if ( start )
    {
    	g_TestScanButtonPresses = 0;
        g_TestOnOffButtonPresses = 0;
        g_ButtonTestMode = 1;
    }
    else
    {
        g_ButtonTestMode = 0;
    }
    return true;
}

bool cmdEEPROMCal_wr(void)
{
	uint32_t val = DLPSPEC_CALIB_VER;
	uint32_t result = PASS;
	calibCoeffs calCoeffs;

	result = EEPROMProgram(&val, EEPROM_CALIB_VER_ADDR, EEPROM_CALIB_VER_SIZE);
	if ( PASS !=  result)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_EEPROM, true, result);
		return FALSE;
	}


	val = DLPSPEC_CFG_VER;
	result = EEPROMProgram(&val, EEPROM_CFG_VER_ADDR, EEPROM_CFG_VER_SIZE);
	if (PASS != result )
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_EEPROM, true, result);
		return FALSE;
	}

	val = DLPSPEC_REFCAL_VER;
	result = EEPROMProgram(&val, EEPROM_REFCAL_VER_ADDR, EEPROM_REFCAL_VER_SIZE);
	if (PASS != result)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_EEPROM, true, result);
		return FALSE;
	}

	calCoeffs.ShiftVectorCoeffs[0] = -2.74909;
	calCoeffs.ShiftVectorCoeffs[1] = 0.0278162;
	calCoeffs.ShiftVectorCoeffs[2] = -0.0000681733;

	calCoeffs.PixelToWavelengthCoeffs[0] = 1784.902664;
	calCoeffs.PixelToWavelengthCoeffs[1] = -0.874372;
	calCoeffs.PixelToWavelengthCoeffs[2] = -0.000278;

	if (PASS == Nano_eeprom_SavecalibCoeffs(&calCoeffs))
		return true;
	else
		return false;
}



/* ******************************************************************************************** */
/* Command functions corresponding to EVM test functions exposed in test tab of the GUI - END   */
/* ******************************************************************************************** */

bool cmdVersion_rd( void )                            /* 0x0216 */
{
	int ret;
	static uint32_t eeprom_cfg_ver=0, eeprom_calib_ver=0, eeprom_refcal_ver=0;

	cmdPutK4( TIVA_VERSION_MAJOR << 16 | TIVA_VERSION_MINOR << 8 | TIVA_VERSION_BUILD );

#ifndef NO_DLPC_BOARD
	if(dlpc150_sw_ver == 0xFFFFFFFF)
	{
		ret = NIRscanNano_DLPCEnable(true);
		if(ret == PASS)
		{
			if(dlpc150_GetSoftwareVersion(&dlpc150_sw_ver) != PASS)
				return false;
			if(dlpc150_GetFlashBuildVersion(&dlpc150_flash_ver) != PASS)
				return false;
		}
		else
		{
			dlpc150_sw_ver = 0; //inidicates error in booting up DLPC150
			dlpc150_flash_ver = 0;
		}
		NIRscanNano_DLPCEnable(false);
	}
#endif
	cmdPut4( dlpc150_sw_ver );
	cmdPut4( dlpc150_flash_ver );

	EEPROMRead(&eeprom_calib_ver, EEPROM_CALIB_VER_ADDR, 4);
	EEPROMRead(&eeprom_refcal_ver, EEPROM_REFCAL_VER_ADDR, 4);
	EEPROMRead(&eeprom_cfg_ver, EEPROM_CFG_VER_ADDR, 4);

	cmdPutK4(  DLPSPEC_VERSION_MAJOR << 16 | DLPSPEC_VERSION_MINOR << 8 | DLPSPEC_VERSION_BUILD );
	cmdPut4( eeprom_calib_ver );
	cmdPut4( eeprom_refcal_ver );
	cmdPut4( eeprom_cfg_ver );

    return true;
}

bool cmdStorePatternInSDRAM_wr(void)
{
	uint32_t data_size;
	data_size = cmdGet4(uint32_t);	// in bytes
	g_patternNumCols = cmdGet4(uint32_t);   // number of columns
	g_patternNumRows = cmdGet4(uint32_t);   // number of rows

	if (data_size > SDRAM_32MB)
		return FALSE;

	// Store num frames transfered for scan command to use
	Scan_SetNumPatternsToScan(data_size / (g_patternNumCols * g_patternNumRows * 3));

#ifdef STAND_ALONE_SCAN_DEBUG
	g_patternNumCols = 864;
	g_patternNumRows = 480;
	StorePatterninSDRAM();
#else
	// Write data to SDRAM
	uint32_t i;
	uint16_t *addrEPISDR;
	addrEPISDR = (uint16_t *)SDRAM_START_ADDRESS;
	for (i = 0; i < data_size; i+=2)
	{
		addrEPISDR[i] = cmdGet1(uint16_t);
		addrEPISDR[i+1] = cmdGet1(uint16_t);
	}
#endif
	return TRUE;
}

bool cmdStartScan_wr(void)
{
	uint8_t storeScaninSDCard = 0;
#ifdef NIRSCAN_INCLUDE_BLE
	uint32_t scan_time = 0;
#endif

	// First check if there is an ongoing scan, if so dont allow a second one
	if (nnoStatus_getIndDeviceStatus(NNO_STATUS_SCAN_IN_PROGRESS))
		return FALSE;

	Scan_SetPatternSource(PATTERNS_FROM_RGB_PORT);
	storeScaninSDCard = cmdGet1(uint8_t);
	if (storeScaninSDCard > 0)
		Scan_StoreToSDcard();

	Semaphore_post( scanSem );
#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
		g_dataBlob[0] = 0x01;

		scan_time = Scan_ComputeScanTime();
		memcpy(&g_dataBlob[1],&scan_time,sizeof(scan_time));

		if (0 == bleNotificationHandler_setNotificationData(BLE_NOTIFY_SCAN_STATUS, 5, &g_dataBlob[0]))
			Semaphore_post(BLENotifySem);
	}
#endif
	return TRUE;
}

bool cmdScanStatus_rd(void)
{

	if ( nnoStatus_getIndDeviceStatus(NNO_STATUS_SCAN_IN_PROGRESS) )
		cmdPutK1(0);	// Scan in progress
	else
		cmdPutK1(1);   // Scan Complete
	return true;
}

bool cmdStartScanInterpret_wr(void)
{

	// First check if there is an ongoing scan or a scan interpretation, if so prevent starting a scan interpretation 
	if ( nnoStatus_getIndDeviceStatus(NNO_STATUS_SCAN_INTERPRET_IN_PROGRESS) ||
		nnoStatus_getIndDeviceStatus(NNO_STATUS_SCAN_IN_PROGRESS) )
	{
		return FALSE;
	}
	
	nnoStatus_setDeviceStatus(NNO_STATUS_SCAN_INTERPRET_IN_PROGRESS, true);
	Semaphore_post( scanInterpretSem );

	return TRUE;
}

bool cmdScanInterpretStatus_rd(void)
{

	if ( nnoStatus_getIndDeviceStatus(NNO_STATUS_SCAN_INTERPRET_IN_PROGRESS) )
		cmdPutK1(0);	// Scan in progress
	else
		cmdPutK1(1);   // Scan Complete
	return true;
}


void InterpretScan()
/**
 * This is the scan interpret task function. The task waits for scanInterpretSem semaphore indefinitiely and
 * upon receving the semaphore interprets a previous scan. nnoStatus_getIndDeviceStatus(NNO_STATUS_SCAN_INTERPRET_IN_PROGRESS) function shall be used
 * to query scan completion status.
 */
{
	int result = PASS;	

	while ( 1 )
	{
		Semaphore_pend(scanInterpretSem, BIOS_WAIT_FOREVER);
		bytesSent = 0;
		result = dlpspec_scan_write_data(GetScanDataPtr(), g_dataBlob, SCAN_DATA_BLOB_SIZE);
		if ( PASS == result) 
			bytesToSend = SCAN_DATA_BLOB_SIZE;
		else
		{
			bytesToSend = 0;
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
		}

		if ( dlpspec_scan_interpret(g_dataBlob, SCAN_DATA_BLOB_SIZE, &scan_results)!= PASS )
		{
			nnoStatus_setDeviceStatus(NNO_STATUS_SCAN_INTERPRET_IN_PROGRESS, false);
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
		}

		nnoStatus_setDeviceStatus(NNO_STATUS_SCAN_INTERPRET_IN_PROGRESS, false);
	}
}


bool cmdTivaReset_wr(void)
{
	SysCtlReset();
	return true;
}
bool cmdSetFixedPGAGain(void)
{
	    uint8_t pgaVal;
		bool isFixed;
	    isFixed = cmdGet1(uint8_t);
	    pgaVal= cmdGet1(uint8_t);
	    Scan_SetFixedPGA(isFixed,pgaVal);


	    return true;
}
bool cmdPGA_wr(void)
{
	uint8_t pgaVal;
	bool retval;
	/*
	 * While not performing scan, ADC is kept in standby and SSI1 clocks are disabled
	 * Need to enable them before setting PGA gain
	 */
	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_SSI1 );
	if(adc_Wakeup() != PASS)
		return false;
	if(adc_SetReadContinuous(false) != PASS)
		return false;

 	pgaVal = cmdGet1(uint8_t);

	if (adc_SetPGAGain(pgaVal) != PASS)
	{
		retval =  false;
	}
	else
	{
		retval =  true;
	}
	/*
	 * Put ADC back to standby and disable SSI1 clock for power saving.
	 */
	if(adc_SetReadContinuous(true) != PASS)
		return false;
	if(adc_Standby() != PASS)
		return false;
	MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_SSI1 );

	return retval;

}

bool cmdReg_wr(void)
{
	uint32_t addr = cmdGet4(uint32_t);
	uint32_t val = cmdGet4(uint32_t);

	if(dlpc150_WriteReg(addr, val) != PASS)
		return false;

	return true;
}

bool cmdReg_rd(void)
{
	uint32_t addr = cmdGet4(uint32_t);
	uint32_t val;

	if(NIRscanNano_DLPCEnable(true) != PASS)
		return false;
	if(dlpc150_ReadReg(addr, &val) != PASS)
		return false;
	if(NIRscanNano_DLPCEnable(false) != PASS)
		return false;
	cmdPut4(val);

	return true;

}


//*****************************************************************************
//
//! Returns the Tiva temperature
//!
//! This function returns the Tiva internal temperature
//!
//!  TEMP = 147.5 - ((75 * 3.3 × ADCCODE) / 4096)
//!
//! \param result is a floating point pointer to the Tiva internal temperature
//
//*****************************************************************************


static void Tiva_temp_read( float *result )
{
	uint32_t valTemp = 0;
	*result = 0.0;

	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_ADC0 );		// Enable clock to ADC
	MAP_ADCSequenceEnable(ADC0_BASE, 1);					// Enable sequence #1
	MAP_ADCProcessorTrigger(ADC0_BASE, 1);					// Trigger the ADC conversion

    while( !MAP_ADCIntStatus( ADC0_BASE, 1, false ) );			// Wait for conversion to complete

    MAP_ADCIntClear( ADC0_BASE, 1 );						// Clear the ADC0 interrupt flag

    MAP_ADCSequenceDataGet(ADC0_BASE, 1, &valTemp);			// Read ADC Value

    MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_ADC0 );		// Disable clock to ADC0

    *result = 147.5 - ((75.0 * 3.3 * valTemp) / 4096.0);

}


bool cmdSetConfig_wr(void)
{
	int32_t numPatterns;
	size_t bufSize = sizeof(scanConfig)*4;
	uScanConfig *pCfg;
	void *pBuf = cmdGetPtrRead();
	int result = PASS;

	result = dlpspec_scan_read_configuration(pBuf, bufSize);

	if (PASS == result)
	{
		pCfg = (uScanConfig *)pBuf;
		numPatterns = Scan_SetConfig(pCfg);
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
		return false;
	}

	cmdPut4(numPatterns);

	if(numPatterns < 0)
		return false;
	else
		return true;
}

bool cmdSaveCfg_wr(void)
{
	uint8_t index = cmdGet1(uint8_t);
	uint8_t bufSize = cmdGet1(uint8_t);
	uScanConfig *pCfg;
	void *pBuf = cmdGetPtrRead();
	int result = PASS;

	result = dlpspec_scan_read_configuration(pBuf, bufSize);

	if (PASS == result)
	{
		pCfg = (uScanConfig *)pBuf;

		/*
		* Review comment - SK
		* true and false return is confusing, think about using other type
		* success and fail or pass and fail - for readbility it will be easier
		*/

		if(Nano_eeprom_SaveConfigRecord(index, pCfg) >= 0)
			return true;
		else
			return false;
	}
	else
		return false;
}

bool cmdScanCfg_rd(void)
{
	uint8_t index = 0;
	uScanConfig cfg;
	uint8_t *pBuf;
	int i = 0;
	size_t bufferSize;
#ifdef NIRSCAN_INCLUDE_BLE
	uint16_t configID = 0;

	if (isBLEConnActive())
	{
		configID = cmdGet2(uint16_t);
		index = Nano_eeprom_GetScanConfigIndexUsingConfigID(configID);
	}
	else
#endif
		index = cmdGet1(uint8_t);

	if(Nano_eeprom_GetConfigRecord(index, &cfg) < 0)
	{
		nnoStatus_setErrorStatus(NNO_ERROR_EEPROM, true);
		return false;
	}
	if(dlpspec_get_scan_config_dump_size(&cfg, &bufferSize) != DLPSPEC_PASS)
		return false;

	pBuf = malloc(bufferSize);
	if(pBuf == NULL)
	{
		nnoStatus_setErrorStatus(NNO_ERROR_INSUFFICIENT_MEMORY, true);
		return false;
	}

	i = dlpspec_scan_write_configuration(&cfg, pBuf, bufferSize);

	if (PASS != i)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, i);
		bufferSize = 0;
	}

#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
		DEBUG_PRINT("\r\nConfiguration data!!!!!\r\n");
		for (i=0;i<bufferSize;i++)
			g_dataBlob[i] = pBuf[i];

		cmdPut(bufferSize, &g_dataBlob[0]);
	}
	else
	{
#endif
		cmdPut4(bufferSize);

		for(i=0; i<bufferSize; i++)
			cmdPut1(pBuf[i]);
#ifdef NIRSCAN_INCLUDE_BLE
		}
#endif
	free(pBuf);

	return true;
}

bool cmdCfgErase_wr(void)
{
	Nano_eeprom_EraseAllConfigRecords();
	return true;
}

bool cmdNumCfg_rd(void)
{
	uint8_t num = Nano_eeprom_GetNumConfigRecords();
#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
#ifdef BLE_USE_TEST_VALUES
		g_dataBlob[0] = 1;
#else
		g_dataBlob[0] = num;
#endif
		g_dataBlob[1] = 0;

		cmdPut(2, &g_dataBlob[0]);
	}
	else
#endif
		cmdPut1(num);

	return true;
}

bool cmdActiveCfg_rd(void)
{

	uint8_t num = Scan_GetActiveConfigIndex();
#ifdef NIRSCAN_INCLUDE_BLE
	int16_t configID = 0;

	if (isBLEConnActive())
	{
		configID = Nano_eeprom_GetScanConfigIDUsingIndex(num);
		g_dataBlob[0] = (uint8_t)(0xff & configID);
		g_dataBlob[1] = (uint8_t)(0xff00 & configID) >> 8;

		cmdPut(2, &g_dataBlob[0]);
	}
	else
#endif
		cmdPut1(num);

	return true;
}

bool cmdActiveCfg_wr(void)
{
	uint8_t index = 0;
	int num_patterns = 0;

#ifdef NIRSCAN_INCLUDE_BLE
	uint16_t configID = 0;

	if (isBLEConnActive())
	{
		configID = cmdGet2(uint16_t);
		index = Nano_eeprom_GetScanConfigIndexUsingConfigID(configID);
	}
	else
#endif
		index = cmdGet1(uint8_t);

	if((num_patterns = Scan_SetActiveConfig(index)) < 0)
	{
		nnoStatus_setErrorStatus(NNO_ERROR_EEPROM, true);
		return false;
	}
	DEBUG_PRINT("Num patterns:%d",num_patterns);

#ifdef NIRSCAN_INCLUDE_BLE
	if (!isBLEConnActive())
#endif
		cmdPut4(num_patterns);

	return true;
}

bool cmdScanDLPCOnOffCtrl_wr(void)
{
	uint8_t dlpc_onoff_control;

	dlpc_onoff_control = cmdGet1(uint8_t);

	Scan_DLPCOnOffControl(dlpc_onoff_control);

	return TRUE;

}

bool cmdScanSubImage_wr(void)
{
	uint16_t startY = cmdGet2(uint16_t);
	uint16_t height = cmdGet2(uint16_t);

	Scan_SetSubImage(startY, height);

	return (TRUE);
}

bool cmdEEPROM_wipe_wr(void)
{
	int i;
	uint32_t clear_val = 0;
	uint32_t addr = EEPROM_CALIB_VER_ADDR;
	uint8_t wipe = cmdGet1(uint8_t);

	// First byte to wipe cal coeffs
	if(wipe)
	{
		EEPROMProgram(&clear_val, addr, EEPROM_CALIB_VER_SIZE);
		addr = EEPROM_CALIB_COEFF_ADDR;
		for(i=0; i<EEPROM_CALIB_COEFF_SIZE/4; i++)
		{
			EEPROMProgram(&clear_val, addr, 4);
			addr += 4;
		}
	}
	wipe = cmdGet1(uint8_t);
	// Second byte to wipe refcal data
	if(wipe)
	{
		addr = EEPROM_REFCAL_VER_ADDR;
		EEPROMProgram(&clear_val, addr, EEPROM_REFCAL_VER_SIZE);
		addr = EEPROM_REFCAL_DATA_ADDR;
		for(i=0; i<EEPROM_REFCAL_DATA_SIZE/4; i++)
		{
			EEPROMProgram(&clear_val, addr, 4);
			addr += 4;
		}
	}
	wipe = cmdGet1(uint8_t);
	// Third byte to wipe scan cfg data
	if(wipe)
	{
		addr = EEPROM_CFG_VER_ADDR;
		EEPROMProgram(&clear_val, addr, EEPROM_CFG_VER_SIZE);
		addr = EEPROM_SCAN_CFG_ADDRESS;
		for(i=0; i<EEPROM_SCAN_CFG_SIZE/4; i++)
		{
			EEPROMProgram(&clear_val, addr, 4);
			addr += 4;
		}
	}

	return true;
}

bool cmdPGA_rd(void)
{
	uint8_t pgaVal;

	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_SSI1 );  //SSI peripheral
	MAP_SysCtlDelay( 2 ); //wait six 6 clock cycles to ensure peripherals ready
	if(adc_Wakeup() != PASS)
		return false;

	if(adc_SetReadContinuous(false) != PASS)
		return false;

	pgaVal = adc_GetPGAGain();

	if(adc_SetReadContinuous(true) != PASS)
		return false;

	cmdPut1(pgaVal);

	if(adc_Standby() != PASS)
		return false;
	MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_SSI1 );

	return true;
}


bool cmdEEPROMcalibCoeffs_wr(void)
{
	uint8_t bufSize = sizeof(calibCoeffs)*3;
	calibCoeffs *pCfg;
	void *pBuf = cmdGetPtrRead();
	int result = 0;

	result = dlpspec_calib_read_data(pBuf, bufSize);

	if (PASS == result)
	{
		pCfg = (calibCoeffs*)pBuf;
		if(Nano_eeprom_SavecalibCoeffs(pCfg) >= 0)
			return true;
		else
			return false;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
		return false;
	}
}

bool cmdEEPROMcalibCoeffs_rd(void)
{
	uint8_t bufSize = sizeof(calibCoeffs)*3;
	calibCoeffs calib_coeffs;
	int i;
	uint8_t *pBuffer;
	int result = PASS;

	pBuffer = malloc(bufSize);

	if(pBuffer == NULL)
	{
		nnoStatus_setErrorStatus(NNO_ERROR_INSUFFICIENT_MEMORY, true);
		return false;
	}

	Nano_eeprom_GetcalibCoeffs(&calib_coeffs);
	result = dlpspec_calib_write_data(&calib_coeffs, (void *)pBuffer, bufSize);

	if (PASS != result)
	{
		bufSize = 0;
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true, result);
	}

#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
		DEBUG_PRINT("\r\nSpec calibration coefficients!!!!!:%d\r\n",bufSize);
		for (i=0;i<bufSize;i++)
			g_dataBlob[i] = pBuffer[i];

		cmdPut(bufSize, &g_dataBlob[0]);
	}
	else
	{
#endif
		for(i=0; i<bufSize; i++)
			cmdPut1(pBuffer[i]);
#ifdef NIRSCAN_INCLUDE_BLE
		}
#endif

	free(pBuffer);

	return true;
}

bool cmdSNRCompute_wr(void)
{
	Scan_SNRDataCapture();
    return true;
}

bool cmdSNRDataSave_rd(void)
{
       float max1 = 0.0 , max2=0.0 , max3 = 0.0;
       uint32_t val1 = 0 , val2 = 0 , val3 = 0;
       uint32_t val4 , val5;
       int i;

       for(i=0; i<9 ; i++)
       {
           if(SNRData.snr_17ms[i] > max1)
               max1 = SNRData.snr_17ms[i];
       }

       for(i=0; i<9 ; i++)
       {
    	   if(SNRData.snr_100ms[i] > max2)
    		   max2 = SNRData.snr_100ms[i];
       }
       for(i=0; i<9 ; i++)
       {
    	   if(SNRData.snr_500ms[i] > max3)
    		   max3 = SNRData.snr_500ms[i];
       }

       val1 = (int)max1;
       val2 = (int)max2;
       val3 = (int)max3;
       val4 = (int)SNRDataHad.had_120ms;
       val5 = (int)SNRDataHad.had_1s;

       cmdPut4(val1);
       cmdPut4(val2);
       cmdPut4(val3);
       cmdPut4(val4);
       cmdPut4(val5);

       return true;
}

bool cmdCalibGenPtns_wr(void)
{
	uint32_t numPatterns;
	CALIB_SCAN_TYPES type = (CALIB_SCAN_TYPES)cmdGet1(uint8_t);

	DEBUG_PRINT("\r\nCalibration type:%d\r\n",type);

	numPatterns = Scan_SetCalibPatterns(type);

	cmdPut4(numPatterns);

	return true;
}

bool cmdScanNumRepeats_wr(void)
{
	uint16_t num = cmdGet2(uint16_t);

	DEBUG_PRINT("\r\nNum repeat:%d",num);

	Scan_SetNumRepeats(num);

	return (TRUE);
}

bool cmdHadSNRCompute_wr(void)
{
	Scan_HadSNRDataCapture();
    return true;
}

bool cmdRefCalibSave_wr(void)
{
	int result = PASS;

    if(Nano_eeprom_StoreRefrenceCalibData() >= 0)
    {
    	DEBUG_PRINT("\r\nStored reference cal coefficients successfully\r\n");
    	return true;
    }
    else
    {
    	DEBUG_PRINT("\r\nStoring reference calibration coefficients failed\r\n");
    	nnoStatus_setErrorStatusAndCode(NNO_ERROR_EEPROM, true, result);
    	return false;
    }
}

bool cmdStartScanFlashPatterns_wr(void)
{
	uint8_t scanID;

	scanID = cmdGet1(uint8_t);
	Scan_SetNumPatternsToScan(NumSplashPerScan[scanID] * 16);

	Scan_SetPatternSource(PATTERNS_FROM_FLASH);
	Semaphore_post( scanSem );

#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
		g_dataBlob[0] = 0x01;

		if (0 == bleNotificationHandler_setNotificationData(BLE_NOTIFY_SCAN_STATUS, 1, &g_dataBlob[0]))
			Semaphore_post(BLENotifySem);
	}
#endif

	return TRUE;
}

bool cmdSaveDeviceSerialNo_wr(void)
{
    int i=0;
    char ser_num[EEPROM_SERIAL_NUMBER_SIZE];
    uScanConfig goldenCfg;
    uScanConfig HadamardCfg;

	/*
	 * Review comment - AL
	 * It may be good to ensure in GUI ensures that serial number is 7 characters and is null terminated than in TIVA?
	 */
	ser_num[0] = cmdGet1(uint8_t);	//dummy read
    for(i=0;i<EEPROM_SERIAL_NUMBER_SIZE-1;i++)
    {
        ser_num[i] = cmdGet1(uint8_t);
    }
    ser_num[EEPROM_SERIAL_NUMBER_SIZE-1] = '\0';

	if(PASS != Nano_eeprom_SaveDeviceSerialNumber((uint8_t* )ser_num))
		return FALSE;

    //save a default scan cfg in EEPROM
	goldenCfg.scanCfg.num_patterns = 228;
	goldenCfg.scanCfg.width_px = 6;
	goldenCfg.scanCfg.scan_type = 0;
	goldenCfg.scanCfg.num_repeats = 6;
	goldenCfg.scanCfg.wavelength_start_nm = MIN_WAVELENGTH;
	goldenCfg.scanCfg.wavelength_end_nm = MAX_WAVELENGTH;
	strcpy(goldenCfg.scanCfg.config_name, "Column 1");

    //save a default Hadamard scan cfg in EEPROM
	HadamardCfg.scanCfg.num_patterns = 228;
	HadamardCfg.scanCfg.width_px = 6;
	HadamardCfg.scanCfg.scan_type = 1;
	HadamardCfg.scanCfg.num_repeats = 6;
	HadamardCfg.scanCfg.wavelength_start_nm = MIN_WAVELENGTH;
	HadamardCfg.scanCfg.wavelength_end_nm = MAX_WAVELENGTH;
	strcpy(HadamardCfg.scanCfg.config_name, "Hadamard 1");

	if ( PASS == Nano_eeprom_SaveConfigRecord(0, &goldenCfg))
	{
		if ( PASS == Nano_eeprom_SaveConfigRecord(1, &HadamardCfg))
		{
			return TRUE;
		}
	}

	return FALSE;
}

bool cmdGetDeviceSerialNo_rd(void)
{
	int i;
	uint8_t serial_number[EEPROM_SERIAL_NUMBER_SIZE];

	if(Nano_eeprom_GetDeviceSerialNumber(serial_number) != PASS)
		return false;

	for(i=0;i<EEPROM_SERIAL_NUMBER_SIZE;i++)
		cmdPut1(serial_number[i]);

	return true;
}

bool cmdSaveModelName_wr(void)
{
    int i=0;
    char model_name[EEPROM_MODEL_NAME_SIZE];

    for(i=0;i<EEPROM_MODEL_NAME_SIZE;i++)
    {
        model_name[i] = cmdGet1(uint8_t);
    }
    model_name[EEPROM_MODEL_NAME_SIZE-1] = '\0';

	if(PASS != Nano_eeprom_SaveDeviceModelName((uint8_t* )model_name))
		return FALSE;


	return TRUE;
}

bool cmdGetModelName_rd(void)
{
	int i;
	uint8_t model_name[EEPROM_MODEL_NAME_SIZE];

	if(Nano_eeprom_GetDeviceModelName(model_name) != PASS)
		return false;

	for(i=0;i<EEPROM_MODEL_NAME_SIZE;i++)
		cmdPut1(model_name[i]);

	return true;
}

bool cmdUpdateRefCalWithWORefl_wr(void)
{
	uScanData* curScanData = GetScanDataPtr();
    int i;
    double newadcData;
	for(i = 0 ; i <  curScanData->data.adc_data_length; i++  )
	{
		newadcData = (double)curScanData->data.adc_data[i] /(double) whiteout_Reflectance[i] ;
		curScanData->data.adc_data[i] = (int32_t)newadcData;
	}
	return true;
}

bool cmdSaveScanNameTag_wr(void)
{
	char name_tag[SCAN_NAME_TAG_SIZE];
	int i= 0;
	int length = 0;

	length = cmdGet1(uint8_t);

	for(i=0;i<SCAN_NAME_TAG_SIZE;i++)
		name_tag[i] = 0;

	for(i=0;i<length;i++)
		name_tag[i] = cmdGet1(uint8_t);

	if (PASS != Nano_eeprom_SaveScanNameTag(&name_tag[0]))
		return false;

	return true;
}

bool cmdEraseScan_wr(void)
{
	uint32_t index = 0;
	int ret_val = 0;

	index = cmdGet4(uint32_t);
		ret_val = FATSD_DeleteScanFile(index);

	if (ret_val == FR_OK)
		return (true);
	else
		return (false);
}

bool cmdEEPROM_mass_erase_wr(void)
{
	uint32_t addr = EEPROM_START_ADDR;
	uint32_t clear_val = 0;

	if ( !EEPROMMassErase() )
	{
		/* Zero eeprom contents */
		for(addr = EEPROM_START_ADDR; addr < EEPROM_START_ADDR+EEPROM_SIZE; addr += 4)
		{
			EEPROMProgram(&clear_val, addr, 4);
		}
		return true;
	}
	else
	{
		nnoStatus_setErrorStatus(NNO_ERROR_EEPROM, true);
		return false;
	}
}

bool cmdScantime_rd(void)
{
	uint32_t scan_time = Scan_ComputeScanTime();
#ifdef NIRSCAN_INCLUDE_BLE
	if (!isBLEConnActive())
#endif
		cmdPut4(scan_time);

	return TRUE;
}

bool cmdSDDeleteLastScanFile_wr(void)
{
	FATSD_DeleteLastScanFile();
    return true;
}

bool cmdTemp_rd(void)
{
	uint32_t ret_val, val;
	float TMPAmbientTemp  = 0.0;
	float TMPDetectTemp  = 0.0;
#ifdef NIRSCAN_INCLUDE_BLE
	int16_t temperature = 0;
#endif
	ret_val = tmp006_DataTemperatureGetFloat( &TMPAmbientTemp, &TMPDetectTemp );

	if ( ret_val == PASS )
	{
#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())
		{
			temperature = (int16_t)(TMPAmbientTemp * 100);
			memcpy(&g_dataBlob[0],&temperature, sizeof(int16_t));
			cmdPut(sizeof(int16_t), &g_dataBlob[0]);
		}
		else
		{
#endif
			val = (int32_t) (TMPAmbientTemp*100.0);
			cmdPut4( val  );
			val = (int32_t) (TMPDetectTemp*100.0);
			cmdPut4( val );
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif
	}
	else
	{
		cmdPut4( ret_val );
	}

	return true;
}

bool cmdHum_rd(void)
{
	int32_t ret_val, val;
#ifdef NIRSCAN_INCLUDE_BLE
	uint16_t humidity = 0;
#endif
	float HDCtemp;
	float HDChumidity = 0.0;

	ret_val = hdc1000_DataTemperatureGetFloat( &HDCtemp, &HDChumidity );

	if(ret_val == PASS)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())
		{
			humidity = (uint16_t)(HDChumidity * 100);
			memcpy(&g_dataBlob[0],&humidity, sizeof(uint16_t));
			cmdPut(sizeof(int16_t), &g_dataBlob[0]);
		}
		else
		{
#endif
			val = (int32_t) (HDCtemp*100.0);
			cmdPut4( val );
			val = (int32_t) (HDChumidity*100.0);
			cmdPut4( val );
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif
	}
	else
	{
		cmdPut4( ret_val );
	}

	return true;
}

bool cmdSetDateTime_wr(void)
{
	 struct tm sTime;

    // Set the date and time values
	sTime.tm_year = (int) cmdGet1(uint8_t);
	sTime.tm_mon = (int) cmdGet1(uint8_t);
	sTime.tm_mday = (int) cmdGet1(uint8_t);
	sTime.tm_wday = (int) cmdGet1(uint8_t);
	sTime.tm_hour = (int) cmdGet1(uint8_t);
	sTime.tm_min = (int) cmdGet1(uint8_t);
	sTime.tm_sec = (int) cmdGet1(uint8_t);

    DEBUG_PRINT("Year =  %d\n",sTime.tm_year);
    sTime.tm_year += 100;                        // Workaround for driverlib goofiness
    DEBUG_PRINT("Month =  %d\n",sTime.tm_mon);
    sTime.tm_mon -= 1;                          // Workaround for driverlib goofiness
    DEBUG_PRINT("Day =  %d\n",sTime.tm_mday);
    DEBUG_PRINT("Day of Week =  %d\n",sTime.tm_wday);
    DEBUG_PRINT("Hour =  %d\n",sTime.tm_hour);
    DEBUG_PRINT("Minute =  %d\n",sTime.tm_min);
    DEBUG_PRINT("Second =  %d\n",sTime.tm_sec);

    MAP_HibernateCalendarSet( &sTime );

    return true;
}

bool cmdBattVolt_rd(void)
{
	float battvoltage = 0.0;
	uint32_t  val;
#ifdef NIRSCAN_INCLUDE_BLE
	unsigned char percentage = 0;
#endif
	battery_read( &battvoltage );

#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
		if (battvoltage <= BATT_00)
			percentage = BATT_00_BYTE;
		else if (battvoltage <= BATT_05)
			percentage = BATT_05_BYTE;
		else if (battvoltage <= BATT_20)
			percentage = BATT_20_BYTE;
		else if (battvoltage <= BATT_40)
			percentage = BATT_40_BYTE;
		else if (battvoltage <= BATT_60)
			percentage = BATT_60_BYTE;
		else if (battvoltage <= BATT_80)
			percentage = BATT_80_BYTE;
		else
			percentage = BATT_100_BYTE;

		memcpy(&g_dataBlob[0],&percentage, sizeof(unsigned char));	//copy all four bytes

		cmdPut(sizeof(unsigned char), &g_dataBlob[0]);
	}
	else
	{
#endif
		val = (int32_t) (battvoltage*100.0) ;
		cmdPut4( val );
#ifdef NIRSCAN_INCLUDE_BLE
		}
#endif

	return true;
}

bool cmdTivaTemp_rd(void)
{
	float ret_val = 0.0;
	uint32_t  val;

	Tiva_temp_read( &ret_val );
	val = (int32_t) (ret_val*100.0);
	cmdPut4( val );

	return true;
}

bool cmdGetDateTime_rd(void)
{
	 struct tm sTime;

    // Get the latest time.
    if(nano_hibernate_calendar_get(&sTime) != PASS)
    	return false;

    DEBUG_PRINT("Year =  %d\n",sTime.tm_year);
    DEBUG_PRINT("Month =  %d\n",sTime.tm_mon);
    DEBUG_PRINT("Day =  %d\n",sTime.tm_mday);
    DEBUG_PRINT("Day of Week =  %d\n",sTime.tm_wday);
    DEBUG_PRINT("Hour =  %d\n",sTime.tm_hour);
    DEBUG_PRINT("Minute = %d\n ",sTime.tm_min);
    DEBUG_PRINT("Second =  %d\n",sTime.tm_sec);

    cmdPut1(sTime.tm_year);
	cmdPut1(sTime.tm_mon);
	cmdPut1(sTime.tm_mday);
	cmdPut1(sTime.tm_wday);
	cmdPut1(sTime.tm_hour);
	cmdPut1(sTime.tm_min);
	cmdPut1(sTime.tm_sec);
	
	return (TRUE);
}

bool cmdSetPowerDown_wr(void)
{
	NIRscanNano_powerDown();

    return TRUE;
}

bool cmdSetHibernate_wr(void)
{
	uint8_t setHibernate = cmdGet1(uint8_t);

	if(setHibernate > 1)
	{
		return FALSE;
	}
	else
	{
		nano_set_hibernate(setHibernate);
		return TRUE;
	}
}

bool cmdGetHibernate_rd(void)
{

	if(nano_get_hibernate())
		cmdPutK1(1);
	else
		cmdPutK1(0);

	return true;
}

bool cmdSDGetNumScanFiles_rd(void)
{
	int num;

	num = FATSD_GetNumScanFiles();
	cmdPut4(num);
	return (TRUE);
}

bool cmdPhotoDetector_rd(void)
{
	PhotoDetVal *pSensorVal = Scan_GetLightSensorData();

    cmdPut4(pSensorVal->red);
#ifdef UNDERFLOW
    cmdPut4(g_ui32UnderflowCount);
#else
    cmdPut4(pSensorVal->green);
#endif
    cmdPut4(pSensorVal->blue);

    return true;
}

bool cmdReadDeviceStat_rd()
{
	uint32_t devStat = 0;
	bool result = TRUE;

	result = nnoStatus_getDeviceStatus(&devStat);
	if (PASS == result)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		if (TRUE == isBLEConnActive())
		{
			g_dataBlob[0] = (uint8_t)(0xff & devStat);
			g_dataBlob[1] = (uint8_t)(0xff00 & devStat) >> 8;
			g_dataBlob[2] = (uint8_t)(0xff0000 & devStat) >> 16;
			g_dataBlob[3] = (uint8_t)(0xff000000 & devStat) >> 24;

			cmdPut(4, &g_dataBlob[0]);
		}
		else
#endif
			cmdPut4(devStat);
	}
	else
		return FALSE;

	return TRUE;
}

bool cmdReadErrorStat_rd()
{
	NNO_error_status_struct errStat;
	bool result = TRUE;
	int i = 0;

	result = nnoStatus_getErrorStatus(&errStat);
	if (PASS == result)
	{
		memcpy(&g_dataBlob[0],&errStat,sizeof(NNO_error_status_struct));

#ifdef NIRSCAN_INCLUDE_BLE
		if (TRUE == isBLEConnActive())
		{
			cmdPut(sizeof(NNO_error_status_struct), &g_dataBlob[0]);
		}
		else
		{
#endif
			for(i=0;i<sizeof(NNO_error_status_struct);i++)
				cmdPut1(g_dataBlob[i]);
#ifdef NIRSCAN_INCLUDE_BLE
			}
#endif
	}
	else
		return FALSE;

	return TRUE;
}

bool cmdResetErrorStat_rd()
{
#ifdef NIRSCAN_INCLUDE_BLE
	NNO_error_status_struct errStat;
	bool result = TRUE;
#endif

	nnoStatus_resetErrorStatus();
	DEBUG_PRINT("\r\nReset Error status\r\n");

#ifdef NIRSCAN_INCLUDE_BLE
	if (TRUE == isBLEConnActive())
	{
		result = nnoStatus_getErrorStatus(&errStat);
		if (PASS == result)
		{
			memcpy(&g_dataBlob[0],&errStat,sizeof(NNO_error_status_struct));

			cmdPut(sizeof(NNO_error_status_struct), &g_dataBlob[0]);
		}
		else
			return FALSE;
	}
#endif
	return TRUE;
}

bool cmdSpecificErrorStatus_rd()
{
	uint32_t field = cmdGet4(uint32_t);

	if(nnoStatus_getIndErrorStatus(field) == true)
		cmdPutK1(1);
	else
		cmdPutK1(0);

	return TRUE;
}

bool cmdSpecificErrorCode_rd()
{
	uint8_t type = cmdGet1(uint8_t);
	int16_t errCode;

	errCode = nnoStatus_getErrorCode(type);
	cmdPut2(errCode);

	return TRUE;
}

bool cmdClearSpecificError_wr()
{
	uint32_t field = cmdGet4(uint32_t);

	if(nnoStatus_clearErrorStatus(field) == PASS)
		return true;
	else
		return false;
}

bool cmdEraseDlpcFlash_wr()
{
	NIRscanNano_DLPCEnable(false);
	if(flash_spi_init() != PASS)
		return false;
	if(flash_spi_chip_erase() != PASS)
		return false;

	//Make sure dlpc150 versions are reset so that they are read again after the update
	dlpc150_sw_ver = 0xFFFFFFFF;
	dlpc150_flash_ver = 0xFFFFFFFF;

	return true;
}
