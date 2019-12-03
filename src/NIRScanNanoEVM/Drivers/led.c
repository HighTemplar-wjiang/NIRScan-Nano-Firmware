/*
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include "inc/tm4c129xnczad.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "NIRscanNano.h"
#include <xdc/runtime/System.h>

#include "GPIO Mapping.h"
#include "led.h"

//*****************************************************************************
//
//! Toggle the green user LED GPIO PQ7 (TIVA TM4C129XNCZAD EVM).
//! \return None.
//
//*****************************************************************************
void ToggleGreenUserLED()
{
	/*
	 * Toggle the green user LED (TIVA TM4C129XNCZAD EVM)
	 */
	GREEN_GPIO_PORT ^= GREEN_GPIO_MASK;
}

void ToggleBlueUserLED()
{
    /*
     * Toggle the blue user LED
     */
	BLUE_GPIO_PORT ^= BLUE_GPIO_MASK;
}

void ToggleYellowUserLED()
{
    /*
     * Toggle the yellow user LED
     */
	YELLOW_GPIO_PORT ^= YELLOW_GPIO_MASK;
}

/**
 *
 *  Yellow LED on
 *
 *  Turns on Yellow LED. Drive PL4 low to turn on.
 *
 *  \return None
 */
void yellowLED_on()
{
    YELLOW_GPIO_PORT &= ~YELLOW_GPIO_MASK;
}

/**
 *
 *  Yellow LED off
 *
 *  Turns off Yellow LED. Drive PF5 high to turn off.
 *
 *  \return None
 */
void yellowLED_off()
{
    YELLOW_GPIO_PORT |= YELLOW_GPIO_MASK;
}

/**
 *
 *  Blue LED on
 *
 *  Turns on Blue LED. Drive PL4 low to turn on.
 *
 *  \return None
 */
void blueLED_on()
{
    BLUE_GPIO_PORT &= ~BLUE_GPIO_MASK;
}

/**
 *
 *  Blue LED off
 *
 *  Turns off Blue LED. Drive PF5 high to turn off.
 *
 *  \return None
 */
void blueLED_off()
{
    BLUE_GPIO_PORT |= BLUE_GPIO_MASK;
}

/**
 *
 *  Green LED on
 *
 *  Turns on Yellow LED. Drive PL4 low to turn on.
 *
 *  \return None
 */
void greenLED_on()
{
    GREEN_GPIO_PORT &= ~GREEN_GPIO_MASK;
}

/**
 *
 *  Green LED off
 *
 *  Turns off Yellow LED. Drive PF5 high to turn off.
 *
 *  \return None
 */
void greenLED_off()
{
    GREEN_GPIO_PORT |= GREEN_GPIO_MASK;
}

