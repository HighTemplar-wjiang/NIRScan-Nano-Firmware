/*
 *
 * contains code required to configure and handle trigger signals between DMD controller and embedded processor
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/lcd.h>
#include <driverlib/interrupt.h>
#include <driverlib/sysctl.h>
#include <inc/hw_memmap.h>
#include <inc/tm4c129xnczad.h>
#include <ti/sysbios/knl/Swi.h>
#include <ti/sysbios/BIOS.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include "GPIO Mapping.h"
#include "adcWrapper.h"
#include "display.h"
#include "math.h"
#include "scan.h"
#include "NIRscanNano.h"
#include "dlpspec_scan.h"
#include "nano_timer.h"
#include "trigger.h"
#include "nnoStatus.h"

//#define USE_MEDIAN_ADC_VAL
#define NUM_ADC_SAMPLES_ACCU_MAX 2000
#define NUM_ADC_SAMPLES_SKIP 5

uint32_t g_FrameTrigger; /**< frame trigger counter */
uint32_t g_PatternTrigger; /**< pattern trigger counter */
uint32_t g_DRDYTrigger;
uint32_t g_scanDataIdx=0;
#ifndef USE_MEDIAN_ADC_VAL
static long long ADCAcc[ADC_DATA_LEN];
static uint16_t ADCAccNumVals[ADC_DATA_LEN];
#else
static int32_t raw_adc_val_array[NUM_ADC_SAMPLES_ACCU_MAX];
#endif
static uint16_t numADCVal;
static bool scanStart = false;
static bool scanInProgress = false;
static uint32_t ptn_drdy_count = 0;
static int32_t ptn_count_in_frame;
static int16_t section_last_pattern;
static uint16_t cur_section;
static int trig_vsyncCount;

static uint32_t exposure_end_margin = 30;
static uint32_t pattern_exposure_time;
static uint32_t timer_count_at_pattern_start;
static int first_pattern_delay_us;

long long *Trig_GetADCAccDataPtr(void)
	/**
	 * Returns the pointer at which ADC values have been accumulated during the scan
	 *
	 * @return pointer to ADC data array
	 *
	 */
{
	return ADCAcc;
}

uint16_t *Trig_GetADCAccNumPtr(void)
	/**
	 * Returns the pointer at which number of values corresponding to each accumulated ADC value
	 * stored during the scan
	 *
	 * @return pointer to ADC data array
	 *
	 */
{
	return ADCAccNumVals;
}

void Frame_trig_int_hander()
	/**
	 * Function: Handle frame trigger from DLPC150
	 * -------------------------------------------
	 * configured to interrupt at the RISING EDGE in NIRscanNano_initGPIO().
	 * Falling edge corresponds to start of frame. Rising edge corresponds to end of frame.
	 *
	 */
{
	//Clear frame interrupt
	MAP_GPIOIntClear(GPIO_PORTP_BASE, GPIO_INT_PIN_0);
	Trig_FrameCallback();
}

bool Trig_IsFramePatternsComplete(int vsync_index)
{
	if(scanInProgress == true)
	{
		if(ptn_count_in_frame == Scan_GetVSyncPatterns(vsync_index))
			return true;
		else
			return false;
	}
	else
	{
		return true;
	}
}

void Trig_FrameCallback()
	/**
	 * Function: Trigger function called from Tiva video frame interrupt handler
	 * ---------------------------------------------------------------------------
	 * Applicable only when DLPC150 is in HW locked mode
	 */
{
	bool pattern_extends_beyond_a_vsync;

	if(scanStart == true) //command had been recieved to start scan
	{
		scanStart = false;
		scanInProgress = true;
		trig_vsyncCount = 0;
		cur_section = 0;
		section_last_pattern = Scan_GetSectionNumPatterns(cur_section);
		pattern_exposure_time = Scan_GetCurSectionExpTime(cur_section) - 
			exposure_end_margin;

		NIRscanNano_trigger_next_pattern();
		ptn_count_in_frame = 0;
		Reset_slew_timer();
		first_pattern_delay_us = 0;
	}
	if(scanInProgress == true)
	{
		if(trig_vsyncCount != 0)
		{
			if(ptn_count_in_frame == 0)
				pattern_extends_beyond_a_vsync = true;
			else
				pattern_extends_beyond_a_vsync = false;

			if(pattern_extends_beyond_a_vsync != true)
			{
				if(Trig_IsFramePatternsComplete(trig_vsyncCount-1) == true)
				{
					NIRscanNano_trigger_next_pattern();
					ptn_count_in_frame = 0;
					Reset_slew_timer();
					first_pattern_delay_us = 0;
				}
				else
				{
					nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
							NNO_ERROR_SCAN_PATTERN_STREAMING);
					/* Put debug info in scanData */
					ADCAcc[0] = trig_vsyncCount-1;
					ADCAcc[1] = Scan_GetVSyncPatterns(trig_vsyncCount-1);
					ADCAcc[2] = ptn_count_in_frame;
					/* end the scan */
					scanInProgress = false;
					Semaphore_post( endScanSem );
				}
			}
		}
		trig_vsyncCount++;
		g_FrameTrigger++;
	}
}

void Pattern_trig_int_handler()
	/**
	 * Function: Handle pattern trigger from DLPC150
	 * ---------------------------------------------
	 * configured to interrupt at the FALLING EDGE in NIRscanNano_initGPIO().
	 * Rising edge corresponds to start of a pattern. Falling edge corresponds to
	 * end of pattern.
	 *
	 */
{
	uScanData *pCurScanData = GetScanDataPtr();

	//Clear pattern trigger interrupt
	MAP_GPIOIntClear(GPIO_PORTP_BASE, GPIO_PIN_1);

	if(scanInProgress)
	{
		/* Restart ADC conversion because a change in light output is expected
		 * upon pattern change.
		 */
		if(first_pattern_delay_us == 0)
		{
			first_pattern_delay_us = Get_Slew_timing();
			timer_count_at_pattern_start = first_pattern_delay_us;
		}
		NIRscanNano_Sync_ADC();

		g_PatternTrigger++;
		ptn_drdy_count = 0;
		if (g_PatternTrigger == pCurScanData->data.adc_data_length+1)
		{
			scanInProgress = false;
			Semaphore_post( endScanSem );
		}
		else if (g_PatternTrigger == section_last_pattern + 1)
		{
			cur_section++;
			section_last_pattern += Scan_GetSectionNumPatterns(cur_section);
			pattern_exposure_time = Scan_GetCurSectionExpTime(cur_section) - 
									exposure_end_margin;
			NIRscanNano_DRDY_int_enable(true);
		}
		else
		{
			NIRscanNano_DRDY_int_enable(true);
		}
	}
}


#ifdef USE_MEDIAN_ADC_VAL
static int32_t find_median(int32_t *val_array, uint32_t num_vals)
{
	int32_t i, j, median, temp;

	/* Sorting begins */
	for (i = 0 ; i <= num_vals-1 ; i++)
	{     /* Trip-i begins  */
		for (j = 0 ; j <= num_vals-i ; j++)
		{
			if (val_array[j] <= val_array[j+1])
			{ /* Interchanging values */

				temp = val_array[j];
				val_array[j] = val_array[j+1];
				val_array[j+1] = temp;
			}
		}
	} /* sorting ends */

	/* calculation of median  */
	if ( num_vals % 2 == 0)
		median = (val_array[num_vals/2] + val_array[num_vals/2+1])/2 ;
	else
		median = val_array[num_vals/2 + 1];

	return median;
}
#endif

void DRDY_int_handler()
	/**
	 * Function: Handle DRDY Trigger from ADC
	 * --------------------------------------
	 * whenever an interrupt involving comparator 1 occurs, this function is called
	 *
	 */
{
	bool black_pattern;
	uint32_t exposure_time;
	int32_t adc_sample;

	//Clear DRDY interrupt
	MAP_GPIOIntClear(DRDY_GPIO_PORT_BASE, DRDY_GPIO_MASK);

	if(scanInProgress)
	{
		ptn_drdy_count++;
		g_DRDYTrigger++;

		if(((g_scanDataIdx+1) % 25) == 0)
			black_pattern = true;
		else
			black_pattern = false;

		if(black_pattern == true)
			exposure_time = 635 - exposure_end_margin;
		else
			exposure_time = pattern_exposure_time;

		/* A certain number of samples need to be skipped at the beginning of each pattern
		 * that account for the transition between previous pattern voltage level and current
		 * pattern voltage level. The effect of not skipping these on the spectrum plot is more
		 * prononced in case of hadamard scans.
		 */
		if(ptn_drdy_count < NUM_ADC_SAMPLES_SKIP)
		{
			adc_GetSample();
			ADCAcc[g_scanDataIdx] = 0;
			numADCVal = 0;
		}
		else if((Get_Slew_timing() - timer_count_at_pattern_start) < exposure_time)
		{
#ifdef USE_MEDIAN_ADC_VAL
			raw_adc_val_array[numADCVal] = adcGetSample();
#else
			adc_sample = adc_GetSample();
			/* If the ADC output has touched the maximum possible, then trigger
			 * an error to indicate possible overflow */
			if(adc_sample == MAX_ADC_OUTPUT)
			{
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true,
						NNO_ERROR_SCAN_ADC_DATA_ERROR);
			}
			//sign extend the 24-bit 2's complement values
			adc_sample <<= 8;
			adc_sample >>= 8;
			ADCAcc[g_scanDataIdx] += adc_sample;
			numADCVal++;
#endif
		}
		//compute average of all samples accumulated so far
		else 
		{
#ifdef USE_MEDIAN_ADC_VAL
			adc_data[g_scanDataIdx++] = find_median(raw_adc_val_array, numADCVal);
#else
#if 1
			ADCAccNumVals[g_scanDataIdx++] = numADCVal;
#else
			ADCAccNumVals[g_scanDataIdx] = 1;
			if(black_pattern == true)
				ADCAcc[g_scanDataIdx++] = 0;
			else
				ADCAcc[g_scanDataIdx++] = trig_vsyncCount-1;//Get_Slew_timing();
#endif
#endif
			/* Disable DRDY triggers until next pattern */
			NIRscanNano_DRDY_int_enable(false);
			numADCVal = 0;
			ptn_count_in_frame++;
			if(black_pattern == true)
				first_pattern_delay_us = 0;
			if(Trig_IsFramePatternsComplete(trig_vsyncCount-1) == false)
			{
				timer_count_at_pattern_start = Get_Slew_timing();
				NIRscanNano_trigger_next_pattern();
			}
		}
	}
}


void Trig_Init()
	/**
	 * Initializes the variables inside trigger module for a new scan
	 *
	 * @return none
	 *
	 */
{
	g_scanDataIdx = 0;
	g_FrameTrigger = 0;
	g_PatternTrigger = 0;
	numADCVal = 0;
	g_DRDYTrigger = 0;
	scanInProgress = false;
	scanStart = true; //actual scan should start only at the next frame trigger.
}
