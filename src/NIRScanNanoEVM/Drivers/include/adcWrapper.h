//*****************************************************************************
//
// Wrapper functions to interface with ADS1255 ADC .
//
// Copyright (c) 2007-2016 Texas Instruments Incorporated.  All rights reserved.
//
//*****************************************************************************


#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_ADC_OUTPUT 0x7FFFFF

int CURRENT_ADC;
/**
 * Internal helper function for clearing the read buffer of trash before use
 */
int32_t adc_EmptyReadBuffer();

/**
 * Perform self test. Checks powerup/powerdown, wakeup/standby, and read/write registers.
 * \returns non-zero on error
 */
int32_t adc_SelfTest();

/**
 * Configure TIVA hardware to enable the ADC. Also performs reset of ADC to ensure
 * self calibration and factory default values. This function must always be called
 * first.
 */
int32_t adc_init();

/**
 * Power up the ADC.
 * All internal registers are reset to factory default values.
 */
int32_t adc_PowerUp();

/**
 * Power down the ADC.
 * User is responsible for ensuring minimum wait time of 20 DRDY cycles
 * after function call to ensure power down is complete.
 */
int32_t adc_PowerDown();

/**
 * Wake the ADC from standby.
 * Internal register settings remain unchanged.
 */
int32_t adc_Wakeup();
int32_t adc_Wakeup_NoDelay();

/**
 * Puts the ADC into sleep state.
 * Use in place of powerdown if fast wakeup is required.
 * \returns non-zero on error
 */
int32_t adc_Standby();

/**
 * Reset the ADC to factory default values
 */
int32_t adc_Reset();


/**
 * Configures the ADC for continuous sample read.
 * \param[in] enableState non-zero to disable else enable
 */
int32_t adc_SetReadContinuous(bool enableState);

/**
 * Get a sample value from the ADC assuming continuuous read operation
 * \returns sample value
 */
int32_t adc_GetSample();
int32_t adc_SetPGAGain(uint8_t pgaVal);
int8_t adc_GetPGAGain(void);

void set_CurrentADC(uint8_t val);

#ifdef __cplusplus
}
#endif

