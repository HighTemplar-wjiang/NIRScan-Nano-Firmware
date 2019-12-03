/*
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inc/tm4c129xnczad.h>
#include <inc/hw_memmap.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/eeprom.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/hibernate.h>
#include <driverlib/gpio.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Timestamp.h>
#include <xdc/runtime/Types.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/hal/Timer.h>

#include "Board.h"
#include "common.h"
#include "fatsd.h"
#include "sdram.h"
#include "adcWrapper.h"
#include "usbCmdHandler.h"
#include "uartCmdHandler.h"
#include "bleCmdHandler.h"
#include "led.h"
#include "usbhandler.h"
#include "scan.h"
#include "nano_eeprom.h"
#include "dlpspec_version.h"
#include "nano_timer.h"
#include "NNOStatusDefs.h"
#include <NNOCommandDefs.h>
#include "nnoStatus.h"
#ifdef ENABLE_UART_COMMAND_INTERFACE
#include "uartstdio.h"
#endif
#ifdef NIRSCAN_INCLUDE_BLE
#include "BLEMain.h"
#include "BLECommonDefs.h"
#endif
#undef BPP4_MODE


//**********************************************************************************************************
// Globals

uint32_t 	g_ui32SysClk;
bool BLEStatus = false;

#ifdef ENABLE_UART_COMMAND_INTERFACE
	 Task_Handle g_uartTaskHandle = NULL;
	 Timer_Handle g_uartTimerHandle = NULL;
#endif
#ifdef NIRSCAN_INCLUDE_BLE
	 Task_Handle g_bleCmdHandlerTaskHandle = NULL;
	 Task_Handle g_bleMainTaskHandle = NULL;
#endif
//**********************************************************************************************************
// Main routine

int main()
{
 	/*
	 * Start of program exection.
	 * Initializes blinking LED for idle state
	 * Returns 0
	 */
	 uint32_t 	ui32Status = 0;
	 uint32_t NVData[64];
	 struct tm sTime;
	 const char app_signature[] = "DLPNIRNANO";
#ifdef ENABLE_UART_COMMAND_INTERFACE
	 Task_Params uart_params;
	 Timer_Params uart_TimerParams;
#endif
#ifdef NIRSCAN_INCLUDE_BLE
	 Task_Params ble_cmd_handler_params;
	 Task_Params ble_main_params;
#endif
#if defined(ENABLE_UART_COMMAND_INTERFACE) || defined(NIRSCAN_INCLUDE_BLE)
	 Error_Block eb;
#endif
	 if(app_signature != NULL); //dummy statement to avoid compiler warning


	 g_ui32SysClk = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), NIRSCAN_SYSCLK);
	 MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_EEPROM0 );

	 MAP_SysCtlPeripheralClockGating(true);    // Enable peripheral clock gating.

	 Board_initGeneral();
	 Board_initGPIO();
	 Board_initI2C();
	 Board_initUART();
	 Board_initLampGPIO();
	 Board_initSPI();
	 Board_initSDSPI();

	 if(Board_initADC() != PASS)
		 return FAIL;
    
	 MAP_HibernateGPIORetentionDisable();

	 if(adc_init() != PASS)
	 {
		 DEBUG_PRINT(("ADC Init failed\n"));
		 return FAIL;
	 }

	 /*
	  * Initialize uart interface if required
	  */
#if defined(ENABLE_UART_COMMAND_INTERFACE) || (defined(DEBUG_MSGS) && (LOG_PORT == UART_CONSOLE))
	 uart_interface_init();
#endif

	 if(sdram_init() != PASS)
		 return FAIL;
    
	 Board_initUSB();
	 IntMasterEnable();
	 USBHandler_init();
	 nnoStatus_init();

	 if(FATSD_Init() != PASS)
		 DEBUG_PRINT(("FATSD Init failed\n"));
	 else
	 {
		 FATSD_WriteReferenceFile();
	 }

	 DEBUG_PRINT(("\n\n=======================================\n  NIRscan Nano Console\n"));
	
	 if(EEPROMInit() != EEPROM_INIT_OK)
	 {
		 DEBUG_PRINT(("EEPROM Init failed\n"));
		 return FAIL;
	 }


	 if ( MAP_HibernateIsActive() )
	 {
		 // Read the status bits to see what caused the wake.  Clear the wake
		 // source so that the device can be put into hibernation again.
		 ui32Status = MAP_HibernateIntStatus( false );
		 MAP_HibernateIntClear( ui32Status );

		 if ( ui32Status & HIBERNATE_INT_RESET_WAKE )
		 {			// Wake up was due to Reset button press
			 greenLED_on();
		 }
		 else if ( ui32Status & HIBERNATE_INT_PIN_WAKE )
		 {		// Wake up was due to the External Wake pin.
			 greenLED_on();
			 MAP_HibernateDataGet( NVData, 64 );
		 }
	 }
	 else
	 {
		 // Hibernation module not active. Cold power-up
		 MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
		 MAP_HibernateEnableExpClk( NIRSCAN_SYSCLK );									// Enable clocking to the hybernation peripheral
		 MAP_HibernateWakeSet( HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RESET | HIBERNATE_WAKE_RTC );	// Configure Hibernate wake sources
		 MAP_HibernateRTCEnable();														// Enable RTC mode.
		 MAP_HibernateCounterMode( HIBERNATE_COUNTER_24HR );								// Configure the hibernate module counter to 24-hour calendar mode.
		 MAP_HibernateIntEnable( HIBERNATE_INT_PIN_WAKE );
		 MAP_HibernateIntClear( HIBERNATE_INT_PIN_WAKE );
		 MAP_HibernateGPIORetentionEnable();
	 }

	 if ( !( ui32Status & (HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_RTC_MATCH_0 | HIBERNATE_INT_GPIO_WAKE | HIBERNATE_INT_RESET_WAKE) ) )
	 {
		 // Set Calendar time to start from 03/31/2015 at 7:00AM
         sTime.tm_hour = 7;
         sTime.tm_min = 0;
         sTime.tm_mon = 2;
         sTime.tm_mday = 31;
         sTime.tm_wday = 2;
         sTime.tm_year = 15+ 100;  // year get 100 substracted
         sTime.tm_isdst = 1;
         MAP_HibernateCalendarSet( &sTime );
	 }

//	 // Turn on bluetooth on boot-up. Helpful to test Bluetooth without using the button
//#if defined(ENABLE_BLE_AT_STARTUP) & defined(NIRSCAN_INCLUDE_BLE)
//	 if ( !BLEStatus )
//	 {
//		 Semaphore_post( BLEStartSem );
//		 BLEStatus = true;
//	 }
//#endif

	 MAP_SysCtlDeepSleepClockConfigSet( 1, SYSCTL_DSLP_OSC_EXT32 | SYSCTL_DSLP_PIOSC_PD | SYSCTL_DSLP_MOSC_PD );
	 SysCtlDeepSleepPowerSet(SYSCTL_FLASH_LOW_POWER | SYSCTL_SRAM_LOW_POWER );

#ifdef ENABLE_UART_COMMAND_INTERFACE
	 Error_init(&eb);
	 Timer_Params_init(&uart_TimerParams);
	 uart_TimerParams.period = MAX_TIME_BET_DATA_PACKETS_US;
	 uart_TimerParams.startMode = Timer_StartMode_USER;
	 uart_TimerParams.periodType = Timer_PeriodType_MICROSECS;
	 g_uartTimerHandle = Timer_create(Timer_ANY, uartDataTimeout,
			 	 	 	 	 	 	  &uart_TimerParams, &eb);

	 Task_Params_init(&uart_params);
	 Error_init(&eb);
	 uart_params.arg0 = 0;
	 uart_params.arg1 = 0;
	 uart_params.stackSize = UART_TASK_STACK_SIZE;
	 uart_params.priority = UART_TASK_PRIORITY;
	 g_uartTaskHandle = Task_create(&uartCmdHandlerTask,
    						  	   &uart_params,
								   &eb);


#endif

#ifdef NIRSCAN_INCLUDE_BLE
	 Task_Params_init(&ble_main_params);
	 Error_init(&eb);
	 ble_main_params.arg0 = 0;
	 ble_main_params.arg1 = 0;
	 ble_main_params.stackSize = BLE_MAIN_TASK_STACK_SIZE;
	 ble_main_params.priority = BLE_MAIN_TASK_PRIORITY;
	 g_bleMainTaskHandle = Task_create(&BLEThreadMain,
    						  	   	   &ble_main_params,
									   &eb);
	 if (g_bleMainTaskHandle == NULL)
	 {
		 nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, APPLICATION_TASK_CREATION_FAILED);
		 DEBUG_PRINT(("\r\nERROR:BLE Main task creation failed\r\n"));
	 }

	 Task_Params_init(&ble_cmd_handler_params);
	 Error_init(&eb);
	 ble_cmd_handler_params.arg0 = 0;
	 ble_cmd_handler_params.arg1 = 0;
	 ble_cmd_handler_params.stackSize = BLE_CMD_HANDLER_TASK_STACK_SIZE;
	 ble_cmd_handler_params.priority = BLE_CMD_HANDLER_TASK_PRIORITY;
	 g_bleCmdHandlerTaskHandle = Task_create(&bleCmdHandlerTaskMain,
    						  	   	   	     &ble_cmd_handler_params,
											 &eb);
	 if (g_bleCmdHandlerTaskHandle == NULL)
	 {
		 nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, APPLICATION_TASK_CREATION_FAILED);
		 DEBUG_PRINT(("\r\nERROR:BLE Command Handler task creation failed\r\n"));
	 }
#endif
	 nnoStatus_setDeviceStatus(NNO_STATUS_TIVA, true);

	 // Turn on bluetooth on boot-up. Helpful to test Bluetooth without using the button
#if defined(ENABLE_BLE_AT_STARTUP) & defined(NIRSCAN_INCLUDE_BLE)
	 if ( !BLEStatus )
	 {
		 Semaphore_post( BLEStartSem );
		 BLEStatus = true;
	 }
#endif

	 BIOS_start(); //does not return, starts TI-RTOS

	 return PASS;
}
