/*
 *
 * APIs to configure and communicate with SDRAM
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_epi.h"
#include "driverlib/epi.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/gpio.h"
#include "inc/tm4c129xnczad.h"
#include <xdc/runtime/System.h>
#include "common.h"
#include "NIRscanNano.h"
#include "sdram.h"

/**
 *
 *  Initializes EPI for SDRAM
 *
 *  Initializes EPI for SDRAM. Sets the EPI for 60MHz SDRAM clock rate
 *  
 *  \return None
 */ 
int32_t sdram_init( )
{
	int timeoutCounter = TIMEOUT_COUNTER;
	/*
	* Review comment - SK
	* Think about running sdram memory test in sdram_init() 
	* Like we would in any ASIC; it is nice to perform memory BIST kind of test
	* upon every power-on. Any failure must be reported and boot sequence be aborted..
	*/
	
  // Enable the Clock to EPI
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);
  
  MAP_GPIOPinConfigure(GPIO_PA6_EPI0S8 );
  MAP_GPIOPinConfigure(GPIO_PA7_EPI0S9 );
  GPIOPinTypeEPI(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7 );
  MAP_GPIOPinConfigure(GPIO_PB3_EPI0S28);
  GPIOPinTypeEPI(GPIO_PORTB_BASE, GPIO_PIN_3 );
  MAP_GPIOPinConfigure(GPIO_PC4_EPI0S7 );
  MAP_GPIOPinConfigure(GPIO_PC5_EPI0S6 );
  MAP_GPIOPinConfigure(GPIO_PC6_EPI0S5 );
  MAP_GPIOPinConfigure(GPIO_PC7_EPI0S4 );
  GPIOPinTypeEPI(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 );
  MAP_GPIOPinConfigure(GPIO_PG0_EPI0S11);
  MAP_GPIOPinConfigure(GPIO_PG1_EPI0S10);
  GPIOPinTypeEPI(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
  MAP_GPIOPinConfigure(GPIO_PH0_EPI0S0 );
  MAP_GPIOPinConfigure(GPIO_PH1_EPI0S1 );
  MAP_GPIOPinConfigure(GPIO_PH2_EPI0S2 );
  MAP_GPIOPinConfigure(GPIO_PH3_EPI0S3 );
  GPIOPinTypeEPI(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
  MAP_GPIOPinConfigure(GPIO_PK5_EPI0S31);
  GPIOPinTypeEPI(GPIO_PORTK_BASE, GPIO_PIN_5 );
  MAP_GPIOPinConfigure(GPIO_PL0_EPI0S16);
  MAP_GPIOPinConfigure(GPIO_PL1_EPI0S17);
  MAP_GPIOPinConfigure(GPIO_PL2_EPI0S18);
  MAP_GPIOPinConfigure(GPIO_PL3_EPI0S19);
  GPIOPinTypeEPI(GPIO_PORTL_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
  MAP_GPIOPinConfigure(GPIO_PM0_EPI0S15);
  MAP_GPIOPinConfigure(GPIO_PM1_EPI0S14);
  MAP_GPIOPinConfigure(GPIO_PM2_EPI0S13);
  MAP_GPIOPinConfigure(GPIO_PM3_EPI0S12);
  GPIOPinTypeEPI(GPIO_PORTM_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
  MAP_GPIOPinConfigure(GPIO_PN2_EPI0S29);
  MAP_GPIOPinConfigure(GPIO_PN3_EPI0S30);
  GPIOPinTypeEPI(GPIO_PORTN_BASE, GPIO_PIN_2 | GPIO_PIN_3 );

  // Work around in example - do the EPI assignment a second time
  GPIOPinTypeEPI(GPIO_PORTA_BASE, GPIO_PIN_6 | GPIO_PIN_7 );
  GPIOPinTypeEPI(GPIO_PORTB_BASE, GPIO_PIN_3 );
  GPIOPinTypeEPI(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 );
  GPIOPinTypeEPI(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
  GPIOPinTypeEPI(GPIO_PORTH_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
  GPIOPinTypeEPI(GPIO_PORTK_BASE, GPIO_PIN_5 );
  GPIOPinTypeEPI(GPIO_PORTL_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
  GPIOPinTypeEPI(GPIO_PORTM_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );
  GPIOPinTypeEPI(GPIO_PORTN_BASE, GPIO_PIN_2 | GPIO_PIN_3 );

  // The Max External Clock Rate Supported is 60MHz. Hence based on the System Frequency
  // configure the divider. A value of 0 means external clock = System Clock  //
  MAP_EPIDividerSet(EPI0_BASE, 1);
  
  // Set SDRAM Mode for EPI
  MAP_EPIModeSet(EPI0_BASE, EPI_MODE_SDRAM);
  
  // Program the Refresh Counter Value and EPI Core Clock for loading proper timing parameters 
  //                    64ms           8192             SysClock/2
  // Value of RFSH = (tRefresh_us / number_rows) / ext_clock_period
  // The Core Frequency is same as the External Clock Frequency which in this case is 60MHz.
  MAP_EPIConfigSDRAMSet(EPI0_BASE, EPI_SDRAM_CORE_FREQ_50_100 | EPI_SDRAM_FULL_POWER | SDRAM_SIZE, 468);
  
  if(SDRAM_START_ADDRESS == 0x10000000)
  {
	  // Configure the Address Aperture and Base Address for the CPU to access the SDRAM Memory
	  // The EPI0 is mapped from 0x10000000 to 0x7FFFFFFF.
	  MAP_EPIAddressMapSet(EPI0_BASE, EPI_ADDR_CODE_SIZE_256MB | EPI_ADDR_CODE_BASE_1 );
  }
  else
  {
	  // Configure the Address Aperture and Base Address for the CPU to access the SDRAM Memory
	  // The EPI0 is mapped from 0x60000000 to 0xCFFFFFFF.
	  MAP_EPIAddressMapSet(EPI0_BASE, EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);
  }
  // Wait for Init to Be Completed
  while( (HWREG( EPI0_BASE + EPI_O_STAT ) & EPI_STAT_INITSEQ) && --timeoutCounter );
  if(timeoutCounter == 0)
  {
	  DEBUG_PRINT("Wait for init complete timedout\n");
	  return FAIL;
  }
  DEBUG_PRINT((" EPI0 SDRAM Init Complete\n"));
  return PASS;

}

/**
 *
 *  SDRAM Test
 *
 *  Reads and writes values to SDRAM
 *  
 *  \return 0 for success, -1 for failure
 */ 
int sdram_test()
{

	volatile uint16_t *addrEPIMem;
	uint32_t length, i;

    // Set the EPI mem pointer to the base of EPI mem
    addrEPIMem = (uint16_t *)SDRAM_START_ADDRESS;
    length = SDRAM_32MB/2;

    for ( i = 0; i < length; i+=2 ) {
    	addrEPIMem[i] = 0x5555;
    	addrEPIMem[i+1] = 0xAAAA;
    }
    MAP_SysCtlDelay(40000000);          // Wait 1 sec at 3 cycles @ 120MHz

    addrEPIMem = (uint16_t *)SDRAM_START_ADDRESS;
    for ( i = 0; i < length; i+=2 ) {
    	if ( addrEPIMem[i] != 0x5555 ) return -1;
    	if ( addrEPIMem[i+1] != 0xAAAA ) return -1;
    }


     for ( i = SDRAM_32MB; i > 0; i-=2 ) {
         addrEPIMem[i] = 0xFACE ;
         addrEPIMem[i-1] = 0xBABE;
     }
     MAP_SysCtlDelay(40000000);          // Wait 1 sec at 3 cycles @ 120MHz


     for ( i = SDRAM_32MB; i > 0; i-=2 ) {
         if ( addrEPIMem[i] != 0xFACE ) return -1;
         if ( addrEPIMem[i-1] != 0xBABE ) return -1;
     }

	DEBUG_PRINT(("EPI0 SDRAM Test Completely Succesfully \n"));
	
    return 0;
}

