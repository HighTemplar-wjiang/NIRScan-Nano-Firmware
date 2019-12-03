/*
 * adcWrapper.c
 * This has the wrapper functions around ads1255 ADC functions
 *
 *  Created on: May 26, 2016
 *      Author: a0393679
 */

#include <stdint.h>
#include <stdbool.h>

#include "adcWrapper.h"
#include "ads1255.h"
#include "afe4490.h"

#define ADC_ADS1255


// fun_ptr_arr for different adc driver functions
//the specific function would be called depending on the current_ADC type
int32_t (*adc_EmptyReadbuffer_arr[])() = { ads1255_EmptyReadBuffer, afe4490_EmptyReadBuffer};
int32_t (*adc_SelfTest_arr[])() = { ads1255_SelfTest, afe4490_SelfTest};
int32_t (*adc_Standby_arr[])() = { ads1255_Standby, afe4490_Standby};
int32_t (*adc_SetReadContinuous_arr[])(bool) = { ads1255_SetReadContinuous, afe4490_SetReadContinuous};
int32_t (*adc_SetPGAGain_arr[])(uint8_t) = { ads1255_SetPGAGain, afe4490_SetPGAGain};
int32_t (*adc_init_arr[])() = { ads1255_init, afe4490_init};
int32_t (*adc_PowerUp_arr[])() = { ads1255_PowerUp, afe4490_PowerUp};
int32_t (*adc_PowerDown_arr[])() = { ads1255_PowerDown, afe4490_PowerDown};
int32_t (*adc_Wakeup_arr[])() = { ads1255_Wakeup, afe4490_Wakeup};
int32_t (*adc_Wakeup_NoDelay_arr[])() = { ads1255_Wakeup_NoDelay, afe4490_Wakeup_NoDelay};
int32_t (*adc_GetSample_arr[])() = { ads1255_GetSample, afe4490_GetSample};
int8_t (*adc_GetPGAGain_arr[])() = { ads1255_GetPGAGain, afe4490_GetPGAGain};

int32_t adc_EmptyReadBuffer()
/**
 * This is a wrapper function around ads1255 EmptyReadBuffer
 *
 * @return  PASS or FAIL
 *
 */
{
	return adc_EmptyReadbuffer_arr[CURRENT_ADC]();
}

int32_t adc_SelfTest()
/**
 * This is a wrapper function around ads1255 Selfttest function
 *
 * @return  PASS or FAIL
 */
{
   return adc_SelfTest_arr[CURRENT_ADC]();
}

int32_t adc_WakeupNoDelay()
/**
 * This is a wrapper function around ads1255 WakeupNoDelay()
 * WakeUp command is issued and returns without any delay after command.
 *
 * @return  PASS
 */
{
   return adc_Wakeup_NoDelay_arr[CURRENT_ADC]();
}

int32_t adc_Standby()
/*
 * This is a wrapper function around ads1255 Standby
 * StandBy command is issued here.
 *
 * @param   None
 *
 * @return  PASS or FAIL
 */
{
 return adc_Standby_arr[CURRENT_ADC]();
}


int32_t adc_SetReadContinuous(bool enableState)
/**
 *
 * This is a wrapper function around ads1255 SetReadContinuous
 *
 * @param   enableState -I- Enables Read Continous mode
 *
 * @return  PASS or FAIL
 */
{
	return adc_SetReadContinuous_arr[CURRENT_ADC](enableState);
}

int32_t adc_SetPGAGain(uint8_t pgaVal)
/**
 * This is a wrapper function around ads1255 SetPGAGain
 * Writes PGA gain to ADC register.
 *
 * @param   pgaVal -I- pga values to be written to PGA register
 *
 * @return  PASS or FAIL
 */
{
	return adc_SetPGAGain_arr[CURRENT_ADC](pgaVal);
}

int32_t adc_init()
/**
 * This is a wrapper function around ads1255 Init
 * Initialization done by setting clock, operating mode.
 * Power up is done which does self calibration at power up.
 *
 * @return  PASS or FAIL
 */
{
	return adc_init_arr[CURRENT_ADC]();
}

int32_t adc_PowerUp()
/**
 * This is a wrapper function around ads1255 PowerUP
 * This function handles the ADC power up sequence.
 *
 * @return  PASS or FAIL
 */
{
	return adc_PowerUp_arr[CURRENT_ADC]();
}

int32_t adc_PowerDown()
/**
 * This is a wrapper function around ads1255 PowerDown
 * This function handles the ADC power down sequence.
 * *
 * @return  PASS
 */
{
	return adc_PowerDown_arr[CURRENT_ADC]();
}

int32_t adc_Wakeup()
/**
 * This is a wrapper function around ads1255 WakeUp
 * @return  PASS
 */
{
 return adc_Wakeup_arr[CURRENT_ADC]();
}

int32_t adc_Wakeup_NoDelay()
/**
 * This is a wrapper function around ads1255 NoDelay
 * WakeUp command is issued and returns without any delay after command.
 *
 * @return  PASS
 */
{
	return adc_Wakeup_NoDelay_arr[CURRENT_ADC]();
}

int32_t adc_GetSample()
/**
 * This is a wrapper function around ads1255 GetSample
 *
 *
 * @return  ADC Data
 */
{
	return adc_GetSample_arr[CURRENT_ADC]();

}

int8_t adc_GetPGAGain(void)
/*
 * This is a wrapper function around ads1255 GetPGAGain
 *
 * @return  pga value currently set in the ADC register.
 */
{
	return adc_GetPGAGain_arr[CURRENT_ADC]();
}

void set_CurrentADC(uint8_t val)
{
	CURRENT_ADC = val;
}
