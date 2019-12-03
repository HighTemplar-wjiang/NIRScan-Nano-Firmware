/*
 *
 * contains code required to handle scan-related logic
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <inc/tm4c129xnczad.h>
#include <inc/hw_memmap.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/i2c.h>
#include <driverlib/gpio.h>
#include <driverlib/eeprom.h>
#include <driverlib/interrupt.h>
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include <driverlib/hibernate.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Types.h>
#include <xdc/runtime/Timestamp.h>
/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

#include "trigger.h"
#include "display.h"
#include "adcWrapper.h"
#include "button.h"
#include "Board.h"
#include "common.h"
#include "hdc1000.h"
#include "tmp006.h"
#include "fatsd.h"
#include "dlpc150.h"
#include "GPIO Mapping.h"
#include "sdram.h"
#include "NIRscanNano.h"
#include "dlpspec_scan.h"
#include "nano_eeprom.h"
#include "BLECommonDefs.h"
#include "BLEGATTGISvcUtilFunc.h"
#include "BLENotificationHandler.h"
#include "NNOSNRDefs.h"
#include "nnoStatus.h"
#include "nano_timer.h"
#include "dlpspec_calib.h"
#include "dlpspec_version.h"
#include "dlpspec_setup.h"
#include "cmdProc.h"
#include "scan.h"

static int32_t Scan_GetPeakADCval(void);
static int Scan_AdjustPGAGain(int32_t max_adc_data);

static int Scan_CalculateSNR_17ms(void);
static int Scan_CalculateSNR_133ms(void);
static int Scan_CalculateSNR_600ms(void);
static int Scan_BinandAverage(float scanArr[][9] , int num_repeats , int steps,
		float resultArr[][9]);
static int Scan_CalculateSNR(float snr_arr[][9] , int num_repeat , float* result_array );
static int Scan_SetupCalibScan(float* , float* , float* , float*);
static int Scan_SetupGeneralScan(float* , float* , float* , float*);
static void Scan_StoreInSDCard(void);
static void Scan_GetSensorReadings(float, float, float, float);
static int Scan_TearDownScanSetup(void);


/* No. of times to repeat photodetector measurement */
#define NUMBER_PHOTODET_REPEAT 3

#define PGA_SET_RETRY_COUNT 10

#define DLPC150_INPUT_FRAME_RATE	60		// input frame rate to DLPC150

/* Declaration of SNR macros */
#define NUMBER_SCANS        720
#define NUMBER_BIN_1        90
#define NUMBER_BIN_2        20
#define BIN1_SIZE           NUMBER_SCANS/NUMBER_BIN_1
#define BIN2_SIZE           NUMBER_SCANS/NUMBER_BIN_2

/* Global Arrays to store SNR */
snrData SNRData;
static float SNR_ScanArr[NUMBER_SCANS][SNR_PATTERNS];
static float SNR_100ms_bins[NUMBER_BIN_1][SNR_PATTERNS];
static float SNR_500ms_bins[NUMBER_BIN_2][SNR_PATTERNS];
static float snrDiff[NUMBER_SCANS];
static float snrtemp[NUMBER_SCANS];
int g_frameSyncArr[NUM_FRAMEBUFFERS];
extern int vsync_period_us;
extern int first_pattern_delay_us;
static int scan_total_frames = 0;

#define MAX_VSYNCS          2496   //NUM_FRAMEBUFFERS * 96 max possible exposure is 96 times 635 us
static int g_patternsPerVsyncarr[MAX_VSYNCS];

snrDataHad SNRDataHad;
int SNR_HadArr[HADSNR_NUM_DATA][HADSNR_LENGTH];
static float TMPDetectTemp  = 0.0;
static float HDChumidity;		// Last HDC1000 humidity measurement read
static float HDCtemp;			// Last HDC1000 temperature measurement read

/* When the below is defined, DLPC150 and lamp will not be turned off after each scan
 * It will also keep the RGB streaming continue so that patterns will be cycling on
 *  the DMD even after scan complete */
//#define LEAVE_PATTERNS_ON
//
static uint16_t scanIndexCounter =0;
static bool b_ReadPhotoSensor = true;
static int ptnSrc=PATTERNS_FROM_RGB_PORT;
static scanData curScanData;
static slewScanData curSlewScanData;
static uScanConfig curScanConfig;
static uint16_t scan_subimage_start_y = 0;
static uint16_t scan_subimage_height = DMD_HEIGHT;
static bool scan_with_subimage = false;
static int storeScan = false;
static uint16_t sessionScanCounter = 1;
static PhotoDetVal photo_val;
static bool scanFinished = false;
static bool scan_snr_savedata = false;
static bool scan_had_snr_savedata = false;
static bool scan_dlpc_onoff_control = true;
static bool scan_index_saved = false;
static int scan_num_repeats = 1;
static uint16_t scan_section_num_patterns[SLEW_SCAN_MAX_SECTIONS];
static bool pga_scan;
static bool isfixedPGA = false;
static uint8_t fixedPGA = 1;

extern uint8_t g_dataBlob[];
extern uint32_t g_FrameTrigger, g_PatternTrigger, g_DRDYTrigger;
extern uint32_t  g_ui32UnderflowCount;
extern uint32_t  g_eof0Count;
extern uint32_t  g_eof1Count;
extern uint32_t g_scanDataIdx;

void Scan_SetNumPatternsToScan(int numPatterns)
	/**
	 * This function must be used to set the number of patterns to be cycled through 
	 * in the subsequent scans. This function is to be called after filling up the frame
	 * buffers with patterns required for the scan.
	 *
	 * @param numPatterns - I - number of patterns to be cycled through
	 *
	 * @return none
	 *
	 */
{
	//To account for black vectors (1 inserted after every 24 patterns)
	curScanData.adc_data_length = numPatterns + numPatterns/NUM_BP_PER_FRAME;
	curScanData.black_pattern_first = NUM_BP_PER_FRAME;
	curScanData.black_pattern_period = NUM_BP_PER_FRAME + 1;
}

PhotoDetVal *Scan_GetLightSensorData(void)
	/**
	 * Returns the photodetector values.
	 *
	 * @return Pointer to PhotoDetVal structure that contains sensor readings
	 *
	 */
{
	return &photo_val;
}

static void Scan_RunPatterns(void)
	/*
	 * Sets up display to run through first pattern to last. Also enables the various
	 * trigger interrupts at start and disables them at the end. Waits for endScan semaphore
	 * to ensure the function returns only after ADC data corresponding to all patterns
	 * have been collected.
	 *
	 * @return none
	 *
	 */
{
	int j, num_seq_trig=0;
	uint16_t currSeqVectNum = 0;
	long long *p_adc_acc_vals;
	uint16_t *p_adc_num_vals;

	g_ui32UnderflowCount = 0;
	if(ptnSrc == PATTERNS_FROM_RGB_PORT)
	{
		Display_SetFrameBufferAtBeginning();

		if(b_ReadPhotoSensor)
		{
			for(j=0;j<NUMBER_PHOTODET_REPEAT;j++)
			{
				if(PASS != dlpc150_GetLightSensorData( &photo_val.red, 
							&photo_val.green, &photo_val.blue ))
				{
					DEBUG_PRINT("dlpc150_GetLightSensorData failed\n");
				}
			}
		}

		if(Display_FramePropagationWait() != PASS)
		{
#ifdef NIRSCAN_INCLUDE_BLE
			bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN, 
					NNO_ERROR_SCAN_PATTERN_STREAMING);
#endif
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, 
					NNO_ERROR_SCAN_PATTERN_STREAMING);
			DEBUG_PRINT("Wait for vsyncs from initial pattern frames timed out\n");
			return;
		}
		g_eof0Count=0;
		g_eof1Count=0;
	}

	Trig_Init();

	/* Clear interrupts before enabling so that the event prior to enable doesn't give
	 *  us an interrupt as soon as enabled */
	MAP_GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_0);
	MAP_GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_1);
	MAP_IntPendClear(INT_GPIOP0);
	MAP_IntPendClear(INT_GPIOP1);
	MAP_GPIOIntEnable( GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
	MAP_IntEnable( INT_GPIOP0 );		//Frame Trigger
	MAP_IntEnable( INT_GPIOP1 );        //Pattern trigger

	Semaphore_pend(endScanSem, BIOS_WAIT_FOREVER);

	MAP_GPIOIntDisable( GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
	MAP_IntDisable( INT_GPIOP0 );
	MAP_IntDisable( INT_GPIOP1 );

#if 1
	// Check if sequence vector has been reset
	 if (dlpc150_GetSequenceVectorNumber(&currSeqVectNum))
	 {
		 DEBUG_PRINT((" DLPC150: Error reading current sequence vector number\n" ));
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
					NNO_ERROR_SCAN_DLPC150_READ_ERROR);
	 }
	 else
	 {
		 if (currSeqVectNum != HW_LOCK_MODE_START_SEQ_VECT)
		 {
			num_seq_trig = HW_LOCK_MODE_NUM_SEQ_VECTORS + HW_LOCK_MODE_START_SEQ_VECT - currSeqVectNum;
			for (j=0; j < num_seq_trig; j++)
			{
				NIRscanNano_trigger_next_pattern();
				MAP_SysCtlDelay(DELAY_500NS*dlpspec_scan_get_exp_time_us(T_635_US)/0.5);
			}
		 }
	 }
#else
	 dlpc150_EnableSequencer(false);
	 dlpc150_EnableSequencer(true);
#endif

		/* Find the mean of ADC values */
		p_adc_acc_vals = Trig_GetADCAccDataPtr();
		p_adc_num_vals = Trig_GetADCAccNumPtr();
		for(j=0; j<curScanData.adc_data_length; j++)
		{
			//avoid division by zero
			if(p_adc_num_vals[j] != 0)
				p_adc_acc_vals[j] = p_adc_acc_vals[j] / p_adc_num_vals[j];
			else
				p_adc_acc_vals[j] = 0;
		}

	 return;
}

static void Scan_PopulateScanDataHeader(void)
{
	int ret;
	int j;
	struct tm sTime;
	char serial_num[EEPROM_SERIAL_NUMBER_SIZE];
	char scan_name_tag[EEPROM_SCAN_NAME_SIZE];
	calibCoeffs calib_coeffs;

	if(nano_hibernate_calendar_get(&sTime) != PASS)
	{
		memset(&sTime, 0, sizeof(struct tm));
	}

	/*Populate scan data serial number and scan index*/
	ret = Nano_eeprom_GetDeviceSerialNumber((uint8_t*)serial_num);
	for(j=0;j<NANO_SER_NUM_LEN;j++)
	{
		if(ret == PASS)
			curScanData.serial_number[j] = serial_num[j];
		else
			curScanData.serial_number[j] = 'F';
	}
	/* Get Index counter from EEPROM */
	Nano_eeprom_GetScanIndexCounter(&scanIndexCounter);

	/* Store index counter back in EEPROM once per session */
	if (!scan_index_saved)
	{
		scanIndexCounter++;
		Nano_eeprom_SetScanIndexCounter(&scanIndexCounter);
		scan_index_saved = true;
	}

	/* Create scan index */
	curScanData.scanDataIndex = ((0xffff & (uint32_t)scanIndexCounter) << 16) + \
								(0xffff & (uint32_t)sessionScanCounter);

	Nano_eeprom_GetScanNameTag(scan_name_tag);
	snprintf(curScanData.scan_name, SCAN_NAME_LEN, "%s", scan_name_tag);

	sessionScanCounter++;

	curScanData.detector_temp_hundredths = (int16_t)(TMPDetectTemp * 100);
	curScanData.humidity_hundredths = (uint16_t)(HDChumidity * 100);

	curScanData.system_temp_hundredths =  (int16_t)(HDCtemp * 100);
#ifdef UNDERFLOW
	curScanData.lamp_pd = (uint16_t) g_ui32UnderflowCount;
#else
	curScanData.lamp_pd = (uint16_t) photo_val.green;
#endif

	Nano_eeprom_GetcalibCoeffs(&calib_coeffs);
	curScanData.calibration_coeffs.PixelToWavelengthCoeffs[0] = \
																calib_coeffs.PixelToWavelengthCoeffs[0];
	curScanData.calibration_coeffs.PixelToWavelengthCoeffs[1] = \
																calib_coeffs.PixelToWavelengthCoeffs[1];
	curScanData.calibration_coeffs.PixelToWavelengthCoeffs[2] = \
																calib_coeffs.PixelToWavelengthCoeffs[2];
	curScanData.calibration_coeffs.ShiftVectorCoeffs[0] = \
														  calib_coeffs.ShiftVectorCoeffs[0];
	curScanData.calibration_coeffs.ShiftVectorCoeffs[1] = \
														  calib_coeffs.ShiftVectorCoeffs[1];
	curScanData.calibration_coeffs.ShiftVectorCoeffs[2] = \
														  calib_coeffs.ShiftVectorCoeffs[2];

	// Set time
	curScanData.year = sTime.tm_year;
	curScanData.month = sTime.tm_mon;
	curScanData.day = sTime.tm_mday;
	curScanData.day_of_week = sTime.tm_wday;
	curScanData.hour = sTime.tm_hour;
	curScanData.minute = sTime.tm_min;
	curScanData.second = sTime.tm_sec;

	curScanData.header_version = CUR_SCANDATA_VERSION;
}


static int Scan_SetPGAGain(uint8_t gain_val)
{
	int result = PASS;
	uint8_t pga_value = 0;
	int32_t retry_count;

	retry_count = 0;
	do
	{
		result = adc_SetReadContinuous(false);
	}while((result != PASS) && (retry_count++ < PGA_SET_RETRY_COUNT));

	if(result == PASS)
	{
		retry_count = 0;
		do
		{
			result = adc_SetPGAGain(gain_val);
			if(result != PASS)
				break;
			pga_value = adc_GetPGAGain();
		}while((pga_value != gain_val) && (retry_count++ < PGA_SET_RETRY_COUNT));

		if(pga_value != gain_val)
			result = FAIL;

		if(result == PASS)
		{
			retry_count = 0;
			do
			{
				result = adc_SetReadContinuous(true);
			}while((result != PASS) && (retry_count++ < PGA_SET_RETRY_COUNT));
		}
	}

	if(result == PASS)
		curScanData.pga = gain_val;
	else
	{
		//make it 0 so that the error is evident to someone looking at it later
		curScanData.pga = 0;
#ifdef NIRSCAN_INCLUDE_BLE
			bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
					NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
					NNO_ERROR_SCAN_ADC_DATA_ERROR);
	}
	
	return result;
}

int Scan_dlpc150_configure(void)
{
	/* Set up the Input Source Select and Pattern Streaming mode */
	if ( dlpc150_SetUpSource(ptnSrc) )
	{
		DEBUG_PRINT((" DLPC150: Error Initializing DLPC150\n" ));
		NIRscanNano_LampEnable(false);
	#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
	#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,(int16_t)NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		return FAIL;
	}
	// Set DLPA2005 LED driver off (it is currently off in the DLPC150 firmware)
	if ( dlpc150_LampEnable(false) )
	{
		DEBUG_PRINT((" DLPC150: Error Turning off PAD Lamp driver\n" ));
		NIRscanNano_LampEnable(false);
		NIRscanNano_DLPCEnable(false);
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_DLPC150_LAMP_DRIVER_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				(int16_t)NNO_ERROR_SCAN_DLPC150_LAMP_DRIVER_ERROR);
		return FAIL;
	}

	return PASS;
}

static int Scan_SetupCalibScan(float* ambt1 , float* dett1 , float* boardt1 , float* humt1)
{
	float ambientT1, detectorT1;
	float boardT1, hum1;

	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_SSI1 );  	// Turn on SSI peripheral
	if(adc_Wakeup_NoDelay() != PASS)	// Wake-up ADS1255 takes about 32usec
	{
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
		return FAIL;
	}
	
	if(isfixedPGA)
	{
		if(Scan_SetPGAGain(fixedPGA) != PASS)
				return FAIL;
	}

	hdc1000_DataTemperatureGetFloat( &boardT1, &hum1 );
	MAP_LCDRasterEnable(LCD0_BASE);			// Turn on LCD peripheral
	if(ptnSrc == PATTERNS_FROM_RGB_PORT)
		MAP_IntEnable( INT_LCD0 );

	//Beginning of a time delay we need to give for DLPC150 initialization command to work.
	Display_SetFrameBufferAtBeginning();
	//End of for DLPC150 initialization command to work.
	Display_FramePropagationWait();

	if(adc_EmptyReadBuffer() != PASS)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
		return FAIL;
	}
	tmp006_DataTemperatureGetFloat( &ambientT1, &detectorT1 );

	*ambt1 = ambientT1;
	*dett1 = detectorT1;
	*boardt1 = boardT1;
	*humt1 = hum1;

	return PASS;
}

static int Scan_SetupGeneralScan(float* ambt1 , float* dett1 , float* boardt1 , float* humt1)
{
	float ambientT1, detectorT1;
	float boardT1, hum1;
	int32_t max_adc_data;
	int result = PASS;
	uint32_t time1, time2, lampTurnOnTime;


	// Turn on DLPA2005 and Lamp Driver 5V supply - 300MS delay max
	if ( NIRscanNano_DLPCEnable(true) != PASS)
	{
		DEBUG_PRINT((" DLPC150: Error Booting DLPC150 \n" ));
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_DLPC150_BOOT_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				NNO_ERROR_SCAN_DLPC150_BOOT_ERROR);
		return FAIL;
	}

	NIRscanNano_LampEnable(true);	// Turn on Lamp Driver
	time1 = Timestamp_get32();		// Start measuring time Lamp is on

	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_SSI1 );  	// Turn on SSI peripheral
	if(adc_Wakeup_NoDelay() != PASS)	// Wake-up ADS1255 takes about 32usec
	{
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
		return FAIL;
	}

	hdc1000_DataTemperatureGetFloat( &boardT1, &hum1 );
	if(adc_EmptyReadBuffer() != PASS)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
		return FAIL;
	}

	/* Check that enough time has passed before LCD enable */
	time2 = Timestamp_get32();
	lampTurnOnTime = time2 - time1;
	// Timer running at divide by 1, so multiple by 3 the standard delay time
	while ( lampTurnOnTime < LCD_START_DELAY * 3 )
	{
		time2 = Timestamp_get32();
		lampTurnOnTime = time2 - time1;
	}

	/* Setup the input source after LCD Enable to get the VSYNCs */
	if ( Scan_dlpc150_configure() == FAIL)
	{
		return FAIL;
	}

	MAP_LCDRasterEnable(LCD0_BASE);			// Turn on LCD peripheral
	if(ptnSrc == PATTERNS_FROM_RGB_PORT)
		MAP_IntEnable( INT_LCD0 );

	tmp006_DataTemperatureGetFloat( &ambientT1, &detectorT1 );
	if(isfixedPGA) //do not do this scan if we want a Fixed PGA Gain value
	{
		result = Scan_SetPGAGain(fixedPGA);
	}
	else
	{
		result = Scan_SetPGAGain(1);
	}

	if(result != PASS)
		return FAIL;

	/* Check that enough time has passed for lamp stability */
	time2 = Timestamp_get32();
	lampTurnOnTime = time2 - time1;
	// Timer running at divide by 1, so multiple by 3 the standard delay time
	while ( lampTurnOnTime < LAMP_STABLIZE_DELAY * 3 )
	{
		time2 = Timestamp_get32();
		lampTurnOnTime = time2 - time1;
	}

	//Beginning of a time delay we need to give for DLPC150 intitialization command to work.
	Display_SetFrameBufferAtBeginning();
	//End of for DLPC150 intitialization command to work.
	Display_FramePropagationWait();

	/* Perform single scan to determine PGA gain setting.
	 * This will be done while lamp stabilizes */

	if(!isfixedPGA) //do not do this scan if we want a Fixed PGA Gain value
	{
		max_adc_data = Scan_GetPeakADCval();
		if(max_adc_data <= 0)
		{
			DEBUG_PRINT(("Scan_GetPeakADCval() Failed\n" ));
#ifdef NIRSCAN_INCLUDE_BLE
			bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
					NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
					NNO_ERROR_SCAN_ADC_DATA_ERROR);
			return FAIL;
		}
		if(Scan_AdjustPGAGain(max_adc_data) != PASS)
		{
			DEBUG_PRINT(("Scan_AdjustPGAGain() Failed\n" ));
#ifdef NIRSCAN_INCLUDE_BLE
			bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
					NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
					NNO_ERROR_SCAN_ADC_DATA_ERROR);
			return FAIL;
		}
	} 

	*ambt1 = ambientT1;
	*dett1 = detectorT1;
	*boardt1 = boardT1;
	*humt1 = hum1;

	return PASS;
}

static void Scan_StoreInSDCard(void)
{
	FRESULT fresult;
	int result = PASS;

	storeScan = false;
	result = dlpspec_scan_write_data(GetScanDataPtr(), g_dataBlob,SCAN_DATA_BLOB_SIZE);
	if (result != PASS)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SPEC_LIB,
				(int16_t)result);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true,
				(int16_t)result);
	}
#ifndef HW_SD_CARD_DETECT
	else	// do not check if HW detect is not active; SW cannot tell out without read/write
#else
		else if (TRUE == nnoStatus_getIndDeviceStatus(NNO_STATUS_SD_CARD_PRESENT))
#endif
		{
			fresult = FATSD_WriteScanFile(g_dataBlob, SCAN_DATA_BLOB_SIZE , \
					curScanData.scanDataIndex);
			if (FR_OK != fresult)
			{
				DEBUG_PRINT("ERROR: Writing to u-SD card failed!! Returned error:%d", fresult);
#ifdef NIRSCAN_INCLUDE_BLE
				bleNotificationHandler_sendErrorIndication(NNO_ERROR_SD_CARD,(int16_t)fresult);
#endif
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD,NNO_ERROR_SD_CARD, (int16_t)fresult);
			}
		}
}

static void Scan_GetSensorReadings(float ambientT1 , float detectorT1 , float boardT1 , float hum1 )
{
	float ambientT2, detectorT2;
	float boardT2, hum2;

#ifdef NIRSCAN_INCLUDE_BLE
	int16_t temperature = 0;
	float TMPAmbientTemp  = 0.0;
	uint16_t humidity = 0;
#endif

	// Read Temperature and Humidity
	tmp006_DataTemperatureGetFloat( &ambientT2, &detectorT2 );
	hdc1000_DataTemperatureGetFloat( &boardT2, &hum2 );
#ifdef NIRSCAN_INCLUDE_BLE
	TMPAmbientTemp = (ambientT1 + ambientT2)/2;

	if (isBLEConnActive())
	{
		temperature = (int16_t)(TMPAmbientTemp * 100);
		GATTGISvc_SetTemp(temperature,true);
	}
#endif

	TMPDetectTemp = detectorT2;
	HDChumidity = (hum1 + hum2)/2;
#ifdef NIRSCAN_INCLUDE_BLE
	if (isBLEConnActive())
	{
		humidity = (uint16_t)(HDChumidity * 100);
		GATTGISvc_SetHum(humidity,true);
	}
#endif
	HDCtemp = (boardT1 + boardT2)/2;
}

static int Scan_TearDownScanSetup(void)
{
#ifndef LEAVE_PATTERNS_ON
	if(ptnSrc == PATTERNS_FROM_RGB_PORT)
	{
		MAP_IntDisable( INT_LCD0 );
		MAP_LCDRasterDisable(LCD0_BASE);
	}
#endif
	if(adc_Standby() != PASS)
	{
#ifdef NIRSCAN_INCLUDE_BLE
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
#endif
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
				NNO_ERROR_SCAN_ADC_DATA_ERROR);
		return FAIL;
	}
	MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_SSI1 );

	return PASS;
}

#define VSYNC_ACTIVE_PATTERN_PERIOD 15240

static void Scan_SetUpSlewScan(uScanConfig *pCfg)
{
	int numPattterns_inFrame = 0;
	int frameNumber_index = 0;
	int exp_time_us;
	uint16_t numPatterns_inSection;
	uint16_t num_black_patterns_inSection;
	int i;
	int numPatterns_inVsync = 0;
	int tot_exp_time_inVsync = 0;
	int vSyncarr_index = 0;
	int frame_sync_count = 1;
	int  section_start_index;

	if(pCfg->scanCfg.scan_type == SLEW_TYPE)
	{
		for(i=0; i<pCfg->slewScanCfg.head.num_sections; i++)
		{
			exp_time_us = dlpspec_scan_get_exp_time_us((EXP_TIME)pCfg->slewScanCfg.section[i].exposure_time);
			dlpspec_scan_section_get_adc_data_range(&curSlewScanData, i, &section_start_index, &numPatterns_inSection, &num_black_patterns_inSection);
			scan_section_num_patterns[i] = numPatterns_inSection + num_black_patterns_inSection;

			while(numPatterns_inSection > 0 )
			{
				//special case for exposure time > vsync period
				if(exp_time_us > VSYNC_ACTIVE_PATTERN_PERIOD)
				{
					//close out the previous vysnc and start at next
					if(numPatterns_inVsync != 0)
					{
						if(numPattterns_inFrame == 24)
						{
							//dark time for the 25th frame. There is always room for the black pattern exposure
							numPatterns_inVsync++;
							g_frameSyncArr[frameNumber_index++] = frame_sync_count ;
							frame_sync_count = 1;
							numPattterns_inFrame = 0;
						}
						g_patternsPerVsyncarr[vSyncarr_index++] = numPatterns_inVsync;
					}
					numPattterns_inFrame++;
					if(exp_time_us > 2*VSYNC_ACTIVE_PATTERN_PERIOD) //4x
					{
						g_patternsPerVsyncarr[vSyncarr_index++] = 0;
						g_patternsPerVsyncarr[vSyncarr_index++] = 0;
						g_patternsPerVsyncarr[vSyncarr_index++] = 0;
						if(numPattterns_inFrame == 1)
							frame_sync_count+=3;
						else
							frame_sync_count+=4;
					}
					else                                            //2x
					{
						g_patternsPerVsyncarr[vSyncarr_index++] = 0;
						if(numPattterns_inFrame == 1)
							frame_sync_count++;
						else
							frame_sync_count+=2;
					}
					numPatterns_inVsync = 1;    //carry over this pattern to next vsync
					tot_exp_time_inVsync = exp_time_us;
				}
				else if((tot_exp_time_inVsync + exp_time_us) > VSYNC_ACTIVE_PATTERN_PERIOD )
				{
					if(numPattterns_inFrame == 24)
					{
						//dark time for the 25th frame. There is always room for the black pattern exposure
						numPatterns_inVsync++;
						g_frameSyncArr[frameNumber_index++] = frame_sync_count ;
						frame_sync_count = 1;
						numPattterns_inFrame = 1;
					}
					else
					{
						frame_sync_count++;
						numPattterns_inFrame++;
					}
					g_patternsPerVsyncarr[vSyncarr_index++] = numPatterns_inVsync;
					numPatterns_inVsync = 1;    //carry over this pattern to next vsync
					tot_exp_time_inVsync = exp_time_us;
				}
				else
				{
					if(numPattterns_inFrame == 24)
					{
						//dark time for the 25th frame. There is always room for the black pattern exposure
						g_patternsPerVsyncarr[vSyncarr_index++] = numPatterns_inVsync + 1;
						g_frameSyncArr[frameNumber_index++] = frame_sync_count ;
						frame_sync_count = 1;
						numPattterns_inFrame = 1;
						numPatterns_inVsync = 1;
						tot_exp_time_inVsync = exp_time_us;
					}
					else
					{
						numPattterns_inFrame++;
						numPatterns_inVsync++;
						tot_exp_time_inVsync += exp_time_us;
					}
				}
				numPatterns_inSection--;
			}
		}
		if(numPatterns_inVsync != 0)
		{
			g_patternsPerVsyncarr[vSyncarr_index++] = numPatterns_inVsync;
			g_frameSyncArr[frameNumber_index++] = frame_sync_count;
		}
		scan_total_frames = 0;
		for(i=0; i<frameNumber_index; i++)
			scan_total_frames += g_frameSyncArr[i];
	}
}

void PerformScan()
	/**
	 * This is the scan task function. The task waits for scanSem semaphore indefinitiely and
	 * upon receving the semaphore performs a scan. Scan_IsScanComplete() function shall be used
	 * to query scan completion status or check devicestatus SCAN_IN_PROGRESS flag.
	 */
{
	float ambientT1, detectorT1;
	float boardT1,hum1;
	int i;
	int j;
	long long *adc_data;
	uint32_t eeprom_calib_ver;
	uint32_t eeprom_config_ver;
	uScanConfig cfg;
	uint8_t index;


#ifdef NIRSCAN_INCLUDE_BLE
	unsigned char bleNotifyData[5];
#endif
	Display_Init();

	/* Initialize black pattern position in curScanData */
	curScanData.black_pattern_first = NUM_BP_PER_FRAME;
	curScanData.black_pattern_period = NUM_BP_PER_FRAME + 1;

	 /* If scan button is kept pressed during start-up, jump to boot loader */
	 if(MAP_GPIOPinRead( GPIO_PORTQ_BASE, GPIO_PIN_3) == 0)
	 {
		 cmdTivaBootMode_wr();
	 }
	 if(FATSD_SkipEEPROMCfg() == true)
	 {
		 cfg.scanCfg.num_patterns = 228;
		 cfg.scanCfg.width_px = 6;
		 cfg.scanCfg.scan_type = 0;
		 cfg.scanCfg.num_repeats = 1;
		 cfg.scanCfg.wavelength_start_nm = MIN_WAVELENGTH;
		 cfg.scanCfg.wavelength_end_nm = MAX_WAVELENGTH;
		 strcpy(cfg.scanCfg.config_name, "Column 1");
		 Scan_SetConfig(&cfg);
	 }
	 else /* apply last set active config from eeprom */
	 {
		 EEPROMRead(&eeprom_calib_ver, EEPROM_CALIB_VER_ADDR, 4);
		 EEPROMRead(&eeprom_config_ver, EEPROM_CFG_VER_ADDR, 4);
		 //if the unit is calibrated
		 if ((eeprom_calib_ver == DLPSPEC_CALIB_VER) && (eeprom_config_ver == DLPSPEC_CFG_VER))
		 {
			 /* Initialize & store config index to ID mapping list */
			 Nano_eeprom_GatherScanCfgIDs();
			 /* Read last selected scan config from EEPROM and activate it */
			index = Scan_GetActiveConfigIndex();
			if(Scan_SetActiveConfig(index) == FAIL)
			{
				/* If setting the last activated item failed for some reason
				 * then set the very first config as active
				 */
				Scan_SetActiveConfig(0);
			}
		 }
	 }

	while ( 1 )
	{
		nnoStatus_setDeviceStatus(NNO_STATUS_SCAN_IN_PROGRESS, false);	//reset scan status - end of scan?
		Semaphore_pend(scanSem, BIOS_WAIT_FOREVER);
		scanFinished = false;
		nnoStatus_setDeviceStatus(NNO_STATUS_SCAN_IN_PROGRESS, true); // set scan status

		//if patterns are from flash, make sure repeat count is initialized to 1.
		if(ptnSrc == PATTERNS_FROM_FLASH) 
			scan_num_repeats = 1;
		else if(scan_num_repeats == 0) //ensure repeat count is non zero
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, 
					NNO_ERROR_SCAN_CFG_INVALID);
			continue;
		}

		/* Initialize frameFlip and patternsPerVsync array for regular scan */
		for(i = 0; i < NUM_FRAMEBUFFERS ; i++)
			g_frameSyncArr[i] = 1;

		for(j=0; j < MAX_VSYNCS; j++)
			g_patternsPerVsyncarr[j] = 25;

		pga_scan = true;

		//Enable DLPC and switch on the lamp at the start of all the scans
		if(scan_dlpc_onoff_control == true)
		{
			if(PASS != Scan_SetupGeneralScan(&ambientT1 , &detectorT1 , &boardT1 , &hum1))
				continue;
		}
		else
		{
			if(PASS != Scan_SetupCalibScan(&ambientT1 , &detectorT1 , &boardT1 , &hum1))
				continue;
		}

		pga_scan = false;
		/* Initialize slew scan arrays now */
		Scan_SetUpSlewScan(&curScanConfig);

		if(scan_with_subimage)
		{
			if ( dlpc150_displayCrop(scan_subimage_start_y, scan_subimage_height) )
			{
				DEBUG_PRINT((" DLPC150: Error Cropping Image\n" ));
#ifdef NIRSCAN_INCLUDE_BLE
				bleNotificationHandler_sendErrorIndication(NNO_ERROR_SCAN, 
						NNO_ERROR_SCAN_DLPC150_CROP_IMG_FAILED);
#endif
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, 
						(int16_t)NNO_ERROR_SCAN_DLPC150_CROP_IMG_FAILED);
				scan_with_subimage = false;
			}
		}

		for(i=0; i<scan_num_repeats; i++)
		{
			if(i==0)
				b_ReadPhotoSensor = true;
			else
				b_ReadPhotoSensor = false;

			Scan_RunPatterns();

			DEBUG_PRINT("=== Scan Complete: Num patterns:%d Num of underflows:%d\n", \
					g_scanDataIdx, g_ui32UnderflowCount);

			adc_data = Trig_GetADCAccDataPtr();

			if(i==0)
			{
				for(j=0;j<curScanData.adc_data_length;j++)
					curScanData.adc_data[j] = adc_data[j];
			}
			else	//accumulate
			{
				for(j=0;j<curScanData.adc_data_length;j++)
					curScanData.adc_data[j] += adc_data[j];
			}
			if(scan_snr_savedata)
			{
				for(j=0;j<curScanData.adc_data_length;j++)
					SNR_ScanArr[i][j] = (float)adc_data[j];
			}
			if(scan_had_snr_savedata)
			{
				for(j=0;j<curScanData.adc_data_length;j++)
				{
					SNR_HadArr[i / HADSNR_BIN_SIZE][j] += adc_data[j];
				}
			}
		}

		if(scan_had_snr_savedata)
		{
			for(i=0;i<HADSNR_NUM_DATA;i++)
				for(j=0;j<curScanData.adc_data_length;j++)
					SNR_HadArr[i][j] = SNR_HadArr[i][j] / HADSNR_BIN_SIZE;
		}

		if(PASS != Scan_TearDownScanSetup())
			continue;

		Scan_GetSensorReadings(ambientT1 , detectorT1 , boardT1 , hum1);

		Scan_PopulateScanDataHeader();

#ifndef LEAVE_PATTERNS_ON
		//switch off the lamp and DLPC if we are controlling ON OFF
		if(scan_dlpc_onoff_control == true)
		{
			NIRscanNano_LampEnable(false);
			NIRscanNano_DLPCEnable(false);
		}
#endif
		//find average
		if(scan_num_repeats != 1)
		{
			for(j=0;j<curScanData.adc_data_length;j++)
			{
				curScanData.adc_data[j] /= scan_num_repeats;
			}
		}

		if(scan_snr_savedata)
		{
			Scan_CalculateSNR_17ms();
			Scan_CalculateSNR_133ms();
			Scan_CalculateSNR_600ms();
			scan_snr_savedata = false;
		}

		if(storeScan)
		{
			Scan_StoreInSDCard();
		}
		UnlockScanButton();
		scanFinished = true;
		scan_had_snr_savedata = false;

#ifdef NIRSCAN_INCLUDE_BLE
		if (isBLEConnActive())		// To be moved to a more appropriate place later
		{
			bleNotifyData[0] = 0xff;
			bleNotifyData[1] = (0x000000ff & curScanData.scanDataIndex);
			bleNotifyData[2] = (0x0000ff00 & curScanData.scanDataIndex) >> 8;
			bleNotifyData[3] = (0x00ff0000 & curScanData.scanDataIndex) >> 16;
			bleNotifyData[4] = (0xff000000 & curScanData.scanDataIndex) >> 24;

			if (0 == bleNotificationHandler_setNotificationData(BLE_NOTIFY_SCAN_STATUS,\
						5, &bleNotifyData[0]))
				Semaphore_post(BLENotifySem);
		}
#endif
	}
}

static void Scan_GenSlewScanData(void)
{
	size_t copy_size = sizeof(slewScanData) - sizeof(int32_t)*ADC_DATA_LEN - sizeof(slewScanConfig);
	memcpy(&curSlewScanData, &curScanData, copy_size);
	memcpy(&curSlewScanData.slewCfg, &curScanConfig, sizeof(slewScanConfig));
	memcpy(&curSlewScanData.adc_data, &curScanData.adc_data, sizeof(int32_t)*ADC_DATA_LEN);
	curSlewScanData.slewCfg.head.num_repeats = scan_num_repeats;
}

uScanData *GetScanDataPtr(void)
	/**
	 * Returns the pointer to scanData struct where ADC values along with other header 
	 * information can be found
	 *
	 * @return scanData - Pointer to scanData structure
	 *
	 */
{
#ifdef BLE_USE_TEST_VALUES		
	//This change is being done so that user can compare against "presets"
	//after deserializing data on Phone
	if (isBLEConnActive())
	{
		strcpy(curScanData.cfg.config_name,"Line Scan");
		curScanData.year = 15;
		curScanData.month = 3;
		curScanData.day = 1;
		curScanData.day_of_week = 0;
		curScanData.hour = 10;
		curScanData.minute = 10;
		curScanData.second = 10;
	}
	DEBUG_PRINT("\r\nDone hardcoding BLE scan data values\r\n");
#endif
	if(curScanConfig.scanCfg.scan_type != SLEW_TYPE)
	{
		curScanData.scan_type = curScanConfig.scanCfg.scan_type;
		curScanData.scanConfigIndex = curScanConfig.scanCfg.scanConfigIndex;
		strcpy(curScanData.ScanConfig_serial_number, curScanConfig.scanCfg.ScanConfig_serial_number);
		strcpy(curScanData.config_name, curScanConfig.scanCfg.config_name);
		curScanData.wavelength_start_nm = curScanConfig.scanCfg.wavelength_start_nm;
		curScanData.wavelength_end_nm = curScanConfig.scanCfg.wavelength_end_nm;
		curScanData.width_px =  curScanConfig.scanCfg.width_px;
		curScanData.num_patterns = curScanConfig.scanCfg.num_patterns;
		curScanData.num_repeats = scan_num_repeats;
		return ((uScanData *)&curScanData);
	}
	else
	{
		Scan_GenSlewScanData();
		return ((uScanData *)&curSlewScanData);
	}

}

bool Scan_IsScanComplete()
	/** function shall be used to query scan completion status
	 *
	 * @return TRUE or FALSE
	 *
	 */
{
	return scanFinished;
}

void Scan_DLPCOnOffControl(bool enable)
	/** 
	 * If enabled, individual scan commands will not enable/disable DLPC150 and 
	 * lamp. The enable/disable of those will have to be handled by the user using
	 * respective APIs
	 *
	 * @param enable -I - true = turn ON this behavior false = turn OFF the behavior
	 *
	 * @return none
	 */
{
	scan_dlpc_onoff_control = enable;
}

int Scan_SetPatternSource(int src)
	/** 
	 * This function to be always called prior to performing a scan.
	 * Pattern source can be selected as one of the values specified in patternSource
	 * in scan.h
	 * @param src -I - to be as per patternSource enum defined in scan.h
	 *
	 * @return PASS or FAIL
	 */
{
	if(src >= PTN_SRC_MAX)
		return FAIL;

	ptnSrc = src;
	return PASS;
}

int Scan_SetNumRepeats(uint16_t num)
	/** 
	 * Use this function to override the num_repeats set in scanConfig.
	 *
	 * @param num - I - number of times scan is to be repeated for averaging.
	 *
	 * @return PASS or FAIL
	 */
{
	scan_num_repeats = num;
	return PASS;
}

uint8_t Scan_GetActiveConfigIndex(void)
	/** 
	 * Returns the index (in EEPROM) of scanConfig that is currently active.
	 *
	 * @return index
	 */
{
	return Nano_eeprom_GetActiveConfigIndex();
}

int Scan_SetActiveConfig(uint8_t index)
	/** 
	 * Sets the specified scanConfig from EEPROM as the active one. This config will be
	 * used for subsequent scans intitated by the scan button on NIRscanNano EVM or from
	 * GUI scan button. This setting will be remembered across power cycles also as this
	 * will be stored in EEPROM.
	 *
	 * @param index - I - index of scanConfig in EEPROM to be activated.
	 *
	 * @return PASS or FAIL
	 */
{
	uScanConfig cfg;

	if (index >= Nano_eeprom_GetNumConfigRecords())
		return FAIL;

	if(Nano_eeprom_SetActiveConfig((uint32_t)index) != PASS)
		return FAIL;

	if ( Nano_eeprom_GetConfigRecord(index, &cfg) < 0 )
		return FAIL;

	// Set config in current scan data - scan would require this
	return Scan_SetConfig(&cfg);
}

int Scan_SetCalibPatterns(CALIB_SCAN_TYPES calib_type)
{
	int numPatterns;
	
	numPatterns = Display_GenCalibPatterns(calib_type);
	
	scan_total_frames = numPatterns/NUM_BP_PER_FRAME + 1;
	
	return numPatterns;
}

int Scan_SetConfig(uScanConfig *pCfg)
	/** 
	 * Applies the specified scanConfig. This config will be used for subsequent scans
	 *  intitated by the scan button on NIRscanNano EVM or by the scan button in GUI.
	 *
	 * @param pCfg - I - pointer to scanConfig data that needs to be applied.
	 *
	 * @return PASS or FAIL
	 */
{
	int num_patterns;

	if(pCfg->scanCfg.scan_type != SLEW_TYPE)
	{
		/* Start Error Checking */
		if(pCfg->scanCfg.num_patterns > MAX_PATTERNS_PER_SCAN)
			return FAIL;

		if((pCfg->scanCfg.wavelength_end_nm > MAX_WAVELENGTH) || (pCfg->scanCfg.wavelength_start_nm < MIN_WAVELENGTH))
			return FAIL;

		if( pCfg->scanCfg.wavelength_end_nm < pCfg->scanCfg.wavelength_start_nm )
			return FAIL;

		if(pCfg->scanCfg.num_repeats == 0)
			return FAIL;
		/* End Error Checking */

		memcpy(&curScanConfig, pCfg, sizeof(uScanConfig));
		Scan_SetNumRepeats(pCfg->scanCfg.num_repeats);
	}
	else
	{
		/* Start Error Checking */
		if(dlpspec_scan_slew_get_num_patterns(&pCfg->slewScanCfg) > MAX_PATTERNS_PER_SCAN)
			return FAIL;

		if(pCfg->slewScanCfg.head.num_repeats == 0)
			return FAIL;
		/* End Error Checking */

		memcpy(&curScanConfig, pCfg, sizeof(uScanConfig));
		Scan_SetNumRepeats(pCfg->slewScanCfg.head.num_repeats);
		Scan_PopulateScanDataHeader();
		Scan_GenSlewScanData();
		Scan_SetUpSlewScan(&curScanConfig);
	}

	num_patterns = Display_GenScanPatterns(pCfg);

	/* Prevent frame buffer overflow */
	if(num_patterns > MAX_PATTERNS_PER_SCAN)
		return FAIL;

	//Set selected scan cfg name as scan name in EEPROM
	Nano_eeprom_SaveScanNameTag(curScanConfig.scanCfg.config_name);
	if(pCfg->scanCfg.scan_type != SLEW_TYPE)
		scan_total_frames = num_patterns/NUM_BP_PER_FRAME + 1;

	Scan_SetNumPatternsToScan(num_patterns);
	return num_patterns;
}

int Scan_GetFrameSyncs(int index)
{

	if(index < 0 || index >= NUM_FRAMEBUFFERS)
		return -1;
	else

		return   g_frameSyncArr[index];
}

int Scan_GetVSyncPatterns(int index)
{

	if(index < 0 || index >= MAX_VSYNCS)
		return -1;
	else

		return   g_patternsPerVsyncarr[index];
}
int Scan_SetSubImage(uint16_t startY, uint16_t height)
	/** 
	 * Call this API after generating patterns is a subimage instead of the full 
	 * pattern has to be applied during scan. This feature is reset after each scan
	 * so it is necessary to call it before each scan command if this is desired for
	 * more than one scans.
	 *
	 * @param startY - I - number of lines at the top to black out
	 * @param height - I - height of the subimage. Lines below this will be blacked out
	 *
	 * @return PASS or FAIL
	 */
{
	scan_subimage_start_y = startY;
	scan_subimage_height = height;
	scan_with_subimage = true;

	return PASS;
}

int Scan_StoreToSDcard(void)
	/** 
	 * Call this API before each scan if the results of scan has to be dumped to SD card.
	 *
	 * @return PASS or FAIL
	 */
{
	storeScan = true;
	return PASS;
}

int Scan_SNRDataCapture(void)
	/**
	 * Call this API to capture 720x9 array for SNR computation
	 * This is done by running 9 patterns for each scan with
	 * 720 scans.
	 *
	 * @return PASS or FAIL
	 */
{
	scan_snr_savedata = true;

	return PASS;
}

int Scan_HadSNRDataCapture(void)
{
	scan_had_snr_savedata = true;
	memset(&SNR_HadArr[0][0],0,HADSNR_NUM_DATA*HADSNR_LENGTH*sizeof(int));

	return PASS;
}

static int Scan_CalculateSNR_17ms(void)
	/**
	 * This API calculates and stores the SNR for each 9
	 * patterns at 17ms interval(one frame time). The maximum
	 * of these 9 patterns is taken as the SNR values for
	 * 17ms interval.
	 *
	 * @return PASS or FAIL
	 */
{
	Scan_CalculateSNR(SNR_ScanArr , NUMBER_SCANS  , SNRData.snr_17ms );

	return PASS;
}

static int Scan_CalculateSNR_133ms(void)
	/**
	 * This API calculates and stores the SNR for each 9
	 * patterns at 133ms interval. The maximum
	 * of these 9 patterns is taken as the SNR values for
	 * 133ms interval. The 90 scans are binned and averaged
	 * before calculating the SNR
	 *
	 * @return PASS or FAIL
	 */
{
	/*Bin and average the 720x9 array */
	Scan_BinandAverage(SNR_ScanArr , NUMBER_SCANS  , BIN1_SIZE  , SNR_100ms_bins);

	/* Calculate SNR */
	Scan_CalculateSNR(SNR_100ms_bins , NUMBER_BIN_1 , SNRData.snr_100ms);

	return PASS;
}

static int Scan_CalculateSNR_600ms(void)
	/**
	 * This API calculates and stores the SNR for each 9
	 * patterns at 600ms interval. The maximum
	 * of these 9 patterns is taken as the SNR values for
	 * 600ms interval. The 20 scans are binned and averaged
	 * before calculating the SNR
	 *
	 * @return PASS or FAIL
	 */
{
	/*Bin and average the 720x9 array */
	Scan_BinandAverage(SNR_ScanArr , NUMBER_SCANS  , BIN2_SIZE  , SNR_500ms_bins);

	/* Calculate SNR */
	Scan_CalculateSNR(SNR_500ms_bins , NUMBER_BIN_2 , SNRData.snr_500ms );

	return PASS;
}

static int Scan_BinandAverage(float scanArr[][SNR_PATTERNS] , int num_repeats , int steps , float resultArr[][SNR_PATTERNS])
	/**
	 * This API does binning and averaging for 720x9 array. The bin size is 90 for 133ms snr and 20 for
	 * 600ms snr value.
	 *
	 * @param scanArr       -I- Array of 720x9 values
	 *        num_repeats   -I- No. of scans. The number of repeats to extract one row of 720 elements
	 *        steps         -I- number of patterns jumped to get the required time
	 *        resultArr     -O- Bins to be used for SNR calculation
	 *
	 * @return PASS or FAIL
	 */
{
	int i , j , k;
	float sum =0.0;
	int countbin=0;
	int countscan=0;

	for(j=0 ; j<SNR_PATTERNS ; j++)
	{
		countbin = 0;
		for(i=0; i<num_repeats/*720*/ ; i+=steps/*8*/)
		{
			for(k=i ; k<i+steps ; k+=4)
			{
				sum += scanArr[k][j];
				countscan++;
			}

			resultArr[countbin][j]= (float)sum/(float)countscan;
			countscan = 0;
			countbin++;
			sum = 0;
		}
	}
	return PASS;
}

static void find_meanstandard_deviation(float* data, int n , float* average , float* std)
	/**
	 * This API calculates the mean and standard deviation of the input array.
	 *
	 * @param data          -I- Pointer to array of values
	 *        n             -I- Size of the input array
	 *        average       -O- calculated average of the array of values
	 *        std           -O- calculated standard deviation of the array of values
	 *
	 * @return PASS or FAIL
	 */
{
	float mean=0.0, sum_deviation=0.0;
	int i;
	for(i=0; i<n;++i)
	{
		mean+=data[i];
	}
	mean=mean/n;
	*average = mean;

	for(i=0; i<n;++i)
		sum_deviation+=(data[i]-mean)*(data[i]-mean);

	*std = sqrt(sum_deviation/n);

	return;

}

static int Scan_CalculateSNR(float snr_arr[][SNR_PATTERNS] , int num_repeat , float* result_array )
	/**
	 * This API calculates the SNR of 9 patterns for different time intervals based on bins.
	 * The formula is SNR = AVERAGE(A)/STANDARD_DEVIATION(D) where A is the values array and
	 * D is difference array of A.
	 *
	 * @param snr_arr       -I- Array of bins
	 *        num_repeat    -I- number of scans to loop through
	 *        result_array  -O- result of SNR for 9 patterns each
	 *
	 * @return PASS or FAIL
	 */
{
	int i , j ;
	int count = 0;

	float avg=0.0  , std = 0.0 , avgdummy= 0.0 , stddummy = 0.0;

	for(j=0; j<SNR_PATTERNS ; j++)
	{
		for(i=0 ; i<num_repeat ; i++)
		{
			snrtemp[i] = snr_arr[i][j];

			if(i>0 && i<NUMBER_SCANS)
			{

				count++;
				snrDiff[i-1] = snrtemp[i]-snrtemp[i-1];
			}
			find_meanstandard_deviation(snrtemp, num_repeat  , &avg , &stddummy);
			find_meanstandard_deviation(snrDiff, count  , &avgdummy , &std);
			result_array[j] = (float)avg/std;

		}
		count = 0;
	}

	return PASS;
}

static int32_t  Scan_GetPeakADCval(void)
{
	long long *p_adc_acc_vals;
	int j;
	int32_t max_peak = 0;

	if(scan_with_subimage)
	{
		if(dlpc150_displayCrop(scan_subimage_start_y, scan_subimage_height) != PASS)
			return FAIL;
		scan_with_subimage = false;
	}

	b_ReadPhotoSensor = false;
	Scan_RunPatterns();

	p_adc_acc_vals = Trig_GetADCAccDataPtr();
	//Find the max of adc_data
	for(j=0;j<curScanData.adc_data_length;j++)
	{
		if(p_adc_acc_vals[j] > max_peak)
			max_peak = p_adc_acc_vals[j];
	}

	return max_peak;

}

#define MAX_RESOLUTION_LESS_TENPERCENT 7549747
static int Scan_AdjustPGAGain(int32_t max_adc_data)
{
	float pga_ratio_mult = 1.0;
	uint32_t temp_pga = 0;
	uint8_t	new_pga = 0;
	int32_t result = PASS;

	/*Calculate PGA gain value*/

	if ( max_adc_data )
		pga_ratio_mult = MAX_RESOLUTION_LESS_TENPERCENT /  max_adc_data;

	temp_pga = (uint32_t) pga_ratio_mult;

	while ( temp_pga > 1 && new_pga < 6 )
	{
		++new_pga;
		temp_pga >>= 1;
	}

	if ( !new_pga )
		temp_pga = 1;
	else
		temp_pga = 1 << new_pga;

	result = Scan_SetPGAGain((uint8_t)temp_pga);
	return result;
}

uint32_t Scan_ComputeScanTime()
	/** Returns the estimated scan time in milliseconds
	 *  Make sure the requied scanConfig and numRepeats are set
	 *  before calling this function to query estimated scan time.
	 */
{
	uint32_t scan_time = 0;
	uint32_t pga_scan_num_frames = 0;
	uint32_t num_ptns = 0;
	uint32_t i;

	// Standard delays per scan
	if(scan_dlpc_onoff_control == true)
	{
		scan_time += (DLPC_ENABLE_MAX_DELAY/DELAY_1MS);
		scan_time += (LAMP_STABLIZE_DELAY/DELAY_1MS);
		scan_time += 150; //150ms DLPC150 configure time

		if(curScanConfig.scanCfg.scan_type != SLEW_TYPE)
		{
			num_ptns = Scan_GetSectionNumPatterns(0);
		}
		else
		{
			for(i=0; i<curScanConfig.slewScanCfg.head.num_sections; i++)
				num_ptns += Scan_GetSectionNumPatterns(i);
		}
		pga_scan_num_frames = num_ptns/NUM_BP_PER_FRAME + 1;
		// add pga scan time
		scan_time += (uint32_t)(((double)(pga_scan_num_frames + PATTERN_DISPLAY_DELAY_NUM_FRAMES) * \
					(double)(1.0/(double)DLPC150_INPUT_FRAME_RATE)) * 1000.0);
	}
	scan_time += 100; //100ms added for overheads like sensor reading and scan data processing.

	// Add pattern display delay, compute time based on num repeats and frame rate
	// and add that to scan time
	scan_time += (uint32_t)(((double)(scan_total_frames + PATTERN_DISPLAY_DELAY_NUM_FRAMES) * \
				(double)(1.0/(double)DLPC150_INPUT_FRAME_RATE)) * \
			(double)(scan_num_repeats) * 1000.0);

	return (scan_time);
}

int16_t Scan_GetSectionNumPatterns(int section_num)
{
	int num_patterns;

	if(section_num < SLEW_SCAN_MAX_SECTIONS)
	{
		if(curScanConfig.scanCfg.scan_type != SLEW_TYPE)
		{
			num_patterns = curScanData.adc_data_length;
		}
		else
		{
			num_patterns = scan_section_num_patterns[section_num];
		}
		return num_patterns;
	}

	return FAIL;
}

uint32_t Scan_GetCurSectionExpTime(int section_num)
{
	uint32_t section_exposure = 0;

	if(pga_scan == true)
		return dlpspec_scan_get_exp_time_us(T_635_US);

	if(section_num < SLEW_SCAN_MAX_SECTIONS)
	{
		if(curScanConfig.scanCfg.scan_type == SLEW_TYPE)
		{
			section_exposure = dlpspec_scan_get_exp_time_us((EXP_TIME)curSlewScanData.slewCfg.section[section_num].exposure_time);
		}
		else
		{
			section_exposure = dlpspec_scan_get_exp_time_us(T_635_US);
		}
	}
	return section_exposure;

}

int Scan_SetFixedPGA(bool isFixed,uint8_t pgaVal)
{
	isfixedPGA = isFixed;
    fixedPGA = pgaVal;
    return PASS;
}
