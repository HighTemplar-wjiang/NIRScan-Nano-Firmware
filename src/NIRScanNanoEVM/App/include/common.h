/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <stdarg.h>
#include <xdc/runtime/System.h>
#include "uartstdio.h"

/*******************************************************************************/
/**************************** COMPILER SWITCHES ********************************/
/*******************************************************************************/

/****************** SCAN CONTROLS *******************/
/**
 * Compiler switch for stand alone scan debug control
 *
 */

//#define SLEW_SCAN_DEBUG

#if 1
	#undef STAND_ALONE_SCAN_DEBUG
#else
	#define STAND_ALONE_SCAN_DEBUG
#endif

/*
 * Compiler switch to enable four lamps in the system
 */
#if 0
	#define FOUR_LAMPS
#else
	#undef FOUR_LAMPS
#endif

/*
 * Compiler switch to disable pattern bending
 * Warning: This can result in spectral inaccuracies
 */
#if 0
	#define NO_PATTERN_BENDING
#else
	#undef NO_PATTERN_BENDING
#endif

/****************** HW CONTROLS *****************/

/*
 * Compiler switch to be able to build TIVA SW
 * without a DLPC150 board connected to it
 */
#if 0
	#define NO_DLPC_BOARD
#else
	#undef NO_DLPC_BOARD
#endif

/*
 * Compiler switch to have TIVA control lamp
 * Else, DLPA2005 part in DLPC150 board would be used
 */
#if 1
	#define TIVA_LAMP_CONTROL
#else
	#undef TIVA_LAMP_CONTROL
#endif

/*
 * Compiler switch to enable BLE communication interface
 *
 * 0 = Disable BLE communication interface
 * 1 = Enable BLE communication interface
 *
 */
#if 1	/* !!!!!!!! Disabling BLE interface not tested compeltely !!!!! */
	#define NIRSCAN_INCLUDE_BLE
#else
	#undef NIRSCAN_INCLUDE_BLE
#endif

/**
 * Compiler switch to enable UARTport command interface
 *
 * 0 = Disable UART command interface
 * 1 = Enable UART command interface
 *
 */
#if 1
	#define ENABLE_UART_COMMAND_INTERFACE
#else
	#undef ENABLE_UART_COMMAND_INTERFACE
#endif

/**
 * Compiler switch to indicate if HW mSD CARD detect is available
 *
 * 1 = HW detect signal available to indicate if SD card present
 * 0 = HW signal not available; SW SD card detect would be used
 */
#if 0
#define HW_SD_CARD_DETECT
#else
#undef HW_SD_CARD_DETECT
#endif

/****************** DEBUG CONTROLS *****************/

#define UART_CONSOLE 0
#define CCS_CONSOLE  1
/**
 * Compiler switch to enable Debug Logs
 */
#if 0
	#define DEBUG_MSGS					// Enables debug messages on CCS console
#else
	#undef DEBUG_MSGS					// Enables debug messages on CCS console
#endif

/**
 * Compiler switch to choose log output
 *
 * 0 = CCS console
 * 1 = UART console
 */
#if 0
	#define LOG_PORT UART_CONSOLE
#else
	#define LOG_PORT CCS_CONSOLE
#endif

/**
 * Compiler switch to choose log printf substitute
 * Please use switches above to control what gets used
 */
#ifdef DEBUG_MSGS
    #if (LOG_PORT == UART_CONSOLE)	// If UART command interface is not enabled, use it for logging
		#ifndef ENABLE_UART_COMMAND_INTERFACE
       	   #define DEBUG_PRINT(...)                                do { UARTprintf(__VA_ARGS__); UARTFlushLog(false);} while(0)
		#else // No console messages
			#define DEBUG_PRINT(...)
		#endif
    #else
	// Print on the CCS Console screen
       #define DEBUG_PRINT(...)                                do { System_printf(__VA_ARGS__); System_flush();} while(0)
    #endif
#else	// No console messages
    #define DEBUG_PRINT(...)
#endif


/*
 * Compiler switch to enable LCD controller underflow to be reported
 */
#if 0
	#define UNDERFLOW
#else
	#undef UNDERFLOW
#endif

/****************** BLUETOOTH CONTROLS *****************/

/**
 * Compiler switch to enable BLE advertsing at startup
 */
#if 1
	#ifdef NIRSCAN_INCLUDE_BLE
		#define ENABLE_BLE_AT_STARTUP
	#endif
#else
	#undef ENABLE_BLE_AT_STARTUP
#endif

/*
 * BLE Debug feature 
 * Compiler switch to make BLE SW use hardcoded values
 */
#if 0
	#define BLE_USE_TEST_VALUES
#else
	#undef BLE_USE_TEST_VALUES
#endif

/**
 * Compiler switch to remap Scan button 
 *
 * 0 = Scan button input from Expansion Connector J3 pin 8
 * 1 = Scan button input from Scan button on Tiva Board
 */
#if 1
	#define SCAN_BUTTON_TIVA_BOARD     // PQ3
	#undef SCAN_BUTTON_EXP_CONN
#else
	#undef SCAN_BUTTON_TIVA_BOARD     // PK3
	#define SCAN_BUTTON_EXP_CONN
#endif

/**
 * Compiler switch to remap GREEN LED
 *
 * 0 = Green LED output to Expansion Connector J3 pin 6
 * 1 = Green LED on Tiva Board
 */
#if 1
	#define GREEN_LED_TIVA_BOARD     // PF5
	#undef GREEN_LED_EXP_CONN
#else
	#undef GREEN_LED_TIVA_BOARD		// PA5
	#define GREEN_LED_EXP_CONN
#endif

/**
 * Compiler switch to remap YELLOW LED
 *
 * 0 = Yellow LED output to Expansion Connector J3 pin 3
 * 1 = Yellow LED on Tiva Board
 */
#if 1
	#define YELLOW_LED_TIVA_BOARD	// PL4
	#undef YELLOW_LED_EXP_CONN
#else
	#undef YELLOW_LED_TIVA_BOARD	// PA4
	#define YELLOW_LED_EXP_CONN
#endif

/**
 * Compiler switch to remap BLUE LED
 *
 * 0 = Blue LED output to Expansion Connector J3 pin 7
 * 1 = Blue LED on Tiva Board
 */
#if 1
	#define BLUE_LED_TIVA_BOARD		// PH7
	#undef BLUE_LED_EXP_CONN
#else
	#undef BLUE_LED_TIVA_BOARD		// PK2
	#define BLUE_LED_EXP_CONN
#endif

/*
 * BLE Debug feature 
 * Compiler switch to make BLE SW use hardcoded values
 */
#if 0
	#define BLE_USE_TEST_VALUES
#else
	#undef BLE_USE_TEST_VALUES
#endif
/*******************************************************************************/
/**************************** COMMON DEFINTIONS ********************************/
/*******************************************************************************/
// Generic counter to wait in while loops for any peripherals or register bits
#define TIMEOUT_COUNTER 120000000

/** Minimum value. */
#define MIN(x,y)   ( ( (x) <= (y) ) ? (x) : (y) )

/** Maximum of two values. */
#define MAX(x,y)   ( ( (x) >= (y) ) ? (x) : (y) )

/** Return TRUE if y is in between x and z. */
#define INRNG(x,y,z)  ( (((y) >= (x)) && ((y) <= (z))) ? (1) : (0) )

#define PASS 0
#define FAIL -1

#define READ	0x1
#define WRITE	0x0

/* Bit masks. */
#define BIT0        0x01
#define BIT1        0x02
#define BIT2        0x04
#define BIT3        0x08
#define BIT4        0x10
#define BIT5        0x20
#define BIT6        0x40
#define BIT7        0x80
#define BIT8      0x0100
#define BIT9      0x0200
#define BIT10     0x0400
#define BIT11     0x0800
#define BIT12     0x1000
#define BIT13     0x2000
#define BIT14     0x4000
#define BIT15     0x8000
#define BIT16 0x00010000
#define BIT17 0x00020000
#define BIT18 0x00040000
#define BIT19 0x00080000
#define BIT20 0x00100000
#define BIT21 0x00200000
#define BIT22 0x00400000
#define BIT23 0x00800000
#define BIT24 0x01000000
#define BIT25 0x02000000
#define BIT26 0x04000000
#define BIT27 0x08000000
#define BIT28 0x10000000
#define BIT29 0x20000000
#define BIT30 0x40000000
#define BIT31 0x80000000

/************************** Timeout & delay values ***************************/
// System timeout definitions
/**
 * Definition of max time of inactivity allowed in the system before TIVA
 * switches to hibernation mode (in minutes)
 */
#define MAX_TIME_BEFORE_HIBERNATION_MINS 5
/**
 * Definition of max time of inactivity before an active inteface is
 * disconnected (in minutes)
 */
#define MAX_TIME_BEFORE_IF_DISC_MINS	 3
/**
 * Definition of initial hibernation enable state
 */
#define HIBERNATE_ENABLE_INIT_STATE true

// Standard delay definitions
#define NO_DELAY        (0)
#define DELAY_25NS   	(1)			/* Wait 50nsec = 1 * 3 cycles @ 120MHz    */
#define DELAY_50NS   	(2)			/* Wait 50nsec = 2 * 3 cycles @ 120MHz    */
#define DELAY_100NS   	(4)			/* Wait 100nsec = 4 * 3 cycles @ 120MHz    */
#define DELAY_200NS   	(8)			/* Wait 200nsec = 8 * 3 cycles @ 120MHz    */
#define DELAY_225NS   	(9)			/* Wait 225nsec = 9 * 3 cycles @ 120MHz    */
#define DELAY_300NS   	(12)		/* Wait 200nsec = 12 * 3 cycles @ 120MHz    */
#define DELAY_500NS   	(20)		/* Wait 500nsec = 20 * 3 cycles @ 120MHz    */
#define DELAY_6_25US   	(250)		/* Wait 6.25usec = 250 * 3 cycles @ 120MHz */
#define DELAY_12_5US   	(500)		/* Wait 12.5usec = 250 * 3 cycles @ 120MHz */
#define DELAY_25US   	(1000)		/* Wait 25usec = 250 * 3 cycles @ 120MHz   */
#define DELAY_50US   	(2000)		/* Wait 50usec = 2000 * 3 cycles @ 120 MHz */
#define DELAY_1MS		(40000)		/* Wait 1msec = 40000 * 3 cycles @ 120MHz  */
#define DELAY_3MS    	(120000)	/* Wait 3msec = 120000 * 3 cycles @ 120 MHz */
#define DELAY_20MS   	(800000)	/* Wait 50msec = 800000 * 3 cycles @ 120 MHz */
#define DELAY_60MS   	(2400000)	/* Wait 60msec = 2400000 * 3 cycles @ 120 MHz */
#define DELAY_100MS  	(4000000)	/* Wait 100msec = 4000000 * 3 cycles @ 120 MHz */
#define DELAY_500MS  	(20000000)	/* Wait 500msec = 20000000 * 3 cycles @ 120 MHz */
#define DELAY_625MS  	(25000000)	/* Wait 625msec = 25000000 * 3 cycles @ 120 MHz */
#define DELAY_750MS  	(30000000)  /* Wait 750msec = 30000000 * 3 cycles @ 120 MHz */
#define DELAY_1S     	(40000000)	/* Wait 1sec = 40000000 * 3 cycles @ 120 MHz */

// Scan delay definitions
#define DLPC_ENABLE_MAX_DELAY				(3 * DELAY_100MS)
#define LCD_START_DELAY                     DELAY_100MS
#define LAMP_STABLIZE_DELAY 				DELAY_625MS
#define PATTERN_DISPLAY_DELAY_NUM_FRAMES	3 // num frames before pattern displayed on DMD
#define PATTERN_DISPLAY_DELAY				(PATTERN_DISPLAY_DELAY_NUM_FRAMES / 60 * DELAY_1S)

#endif /* COMMON_H_ */
