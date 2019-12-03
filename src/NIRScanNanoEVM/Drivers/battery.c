/*
 * contains code required to configure and handle trigger signals between DMD controller and embedded processor
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */
#include <stdbool.h>
#include <stdint.h>

#include <inc/tm4c129xnczad.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/adc.h>

#include "battery.h"

#define R_DIV1      620.0f
#define R_DIV2      2000.0f
#define BATT_CORR ((R_DIV2 + R_DIV1) / R_DIV2)

//*****************************************************************************
//
//! Returns the battery voltage
//!
//! This function enables PD5 which opens the DMC299 FET to send the battery
//! voltage to TIVA'S AIN7. The Lithium Polymer battery voltage ranges from
//! 4.2V - 3.0V, which is greater than the ADC's 3.3V. Thus, a resistor
//! divider (R_DIV2/(R_DIV1+R_DIV2) divides it down to the Tiva's ADC. This
//! function takes into account the 12-bit ADC and the resistor divider
//! to compute the battery voltage based on the formula:
//!
//!  battery voltage = (AIN7 * 3.3 / 2^12) * ((R_DIV2 + R_DIV1) / R_DIV2)
//!
//! \param result is a floating point pointer to the Battery voltage 
//
//*****************************************************************************

void battery_read( float *result )
{
	uint32_t valAIN7 = 0;
	*result = 0.0;
	
	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_ADC0 );		// Enable clock to ADC

	// Enable Battery Sense by driving PD5 high
	MAP_GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_5, GPIO_PIN_5 );
	
	MAP_ADCSequenceEnable(ADC0_BASE, 3);					// Enable sequence #3
	MAP_ADCProcessorTrigger(ADC0_BASE, 3);					// Trigger the ADC conversion
        
    while( !MAP_ADCIntStatus( ADC0_BASE, 3, false ) );			// Wait for conversion to complete
    
    MAP_ADCIntClear( ADC0_BASE, 3 );						// Clear the ADC0 interrupt flag
    
    MAP_ADCSequenceDataGet(ADC0_BASE, 3, &valAIN7);			// Read ADC Value
    
    MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_ADC0 );		// Disable clock to ADC0
    *result = ( ((float) valAIN7) * 3.3f * BATT_CORR ) / 4096.0f;
    
    // Disable Battery Sense by driving PD5 low
    MAP_GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_5, 0 );
}
        
