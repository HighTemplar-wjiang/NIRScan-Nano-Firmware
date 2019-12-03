/*
 *
 * BLE Main Thread entry function defintion
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include <xdc/std.h>              /* Sys/Bios Standard Header.                */
#include <xdc/runtime/Error.h>    /* Sys/Bios Error Header.                   */
#include <xdc/cfg/global.h>
#include <ti/sysbios/BIOS.h>      /* Sys/Bios Bios Header.                    */
#include <ti/sysbios/hal/Hwi.h>   /* Sys/Bios Hardware Interrupt Header.      */
#include <ti/sysbios/knl/Task.h>  /* Sys/Bios Task Header.                    */
#include <ti/sysbios/knl/Clock.h> /* Sys/Bios Clock Header.                   */
#include "HAL.h"                  /* Function for Hardware Abstraction.       */
#include "HALCFG.h"               /* HAL Configuration Constants.             */
#include "SS1BTPS.h"             /* Main SS1 Bluetooth Stack Header.          */
#include "SS1BTVS.h"             /* Vendor Specific Prototypes/Constants.     */
#include "BTPSKRNL.h"            /* BTPS Kernel Prototypes/Constants.         */
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/hibernate.h>
#include <inc/hw_memmap.h>
#include <xdc/runtime/System.h>
#include "led.h"
#include "uartstdio.h"
#include "BLEProfMgr.h"
#include "BLECommonDefs.h"
#include "BLECmdHandlerLiaison.h"
#include "BLENotificationHandler.h"
#include "BLEUtils.h"
#include "BLEMain.h"
extern bool BLEStatus;

/**
 * @brief Wait time for End BLE semaphore
 *
 * This Timeout for the Semaphore that waits on subsequent button press
 * to turn off BLE (in ms)
 */
#define END_BLE_SEM_WAIT_TIME						(500)

/**
 * @brief Wait time for End BLE command processing semaphore
 *
 * Timeout for the Semaphore that waits on reponse from BLE Command processor
 * (in ms)
 */
#define BLE_CMD_PROCESSING_TIMEOUT 					(500)

/**
 * @brief Wait time for client notification semaphore before timeout
 *
 * Timeout for the Semaphore that handles notifications to the client. The
 * request could come from any module in TIVA (in ms)
 */
#define BLE_NOTIFY_PROC_TIMEOUT						(250)

/**
 * @brief Global valriable to store supported MTU size based by client
 */
unsigned short gBLESuppMTUSize = 0;

/**
 * @brief Global valriable to store BLE connection status
 */
static bool isBLEActive = FALSE;

/**
 * @brief HCI Sleep Mode Callback
 *
 * The function would be used for sleep callback indications
 * Not used at the moment but could be modified if required
 */
static void BTPSAPI HCI_Sleep_Callback(Boolean_t SleepAllowed, unsigned long CallbackParameter);

/**
 * @brief BTPS application callback
 *
 * Callback called in case of command notifications
 * Not used at the moment but can be used if required
 */

static void DisplayCallback(char Character);

/**
 * @brief Function to close BLE stack
 *
 * The function would execute procedure to hierarchially de-initialize stack
 * components and then finally the BLE stack
 */
int CloseBLEStack();

/**
 * Application specific HCI sleep callback function
 *
 * The following is the HCI Sleep Callback.  This is registered with
 * the stack to note when the Host processor may enter into a sleep
 * mode. This is a dummy function at present, please modify if reqd
 * at later point in time
 *
 * @param[in]   SleepAllowed		To indicate if sleep is allowed or not
 * @param[in]   CallbackParameter	Application	pecific callback parameter
 *
 * @return      None
 *
 */
static void BTPSAPI HCI_Sleep_Callback(Boolean_t SleepAllowed, unsigned long CallbackParameter)
{
	// Monitor low power state
}

/**
 * Application specific HCI sleep callback function
 *
 * The following function is registered with the application so that
 * it can display strings to the debug UART. Not used at the moment
 *
 * @param[in]   char		Character to display
 *
 * @return      None
 *
 */
static void DisplayCallback(char Character)
{
   while(!HAL_ConsoleWrite(1, &Character))
   {
	   BTPS_Delay(1);
   }
}

/**
 * Function to close BLE stack
 *
 * Function deinitialises application specific data structures,
 * stops advertisement and then invokes function to dergister GATT profile,
 * and close all stack modules
 *
 * @return	Success/Failure, error codes in case of failure
 *
 */
int CloseBLEStack()
{
	int ret_val = 0;

	if (isBLEActive)
	{
		DeInitBLECmdHandlerLiason();
		bleNotificationHandler_DeInit();

		if (0 == AdvertiseLE(FALSE))
		{
			ret_val = CloseStack();
			if (ret_val < 0)
			{
				DEBUG_PRINT("\r\nError: Closing stack failed\r\n");
				return (ret_val);
			}
			else
			{
			    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);

			    // Power down CC25464 - P1P8V_BT
				MAP_GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_2, 0 );

				/* Configure RTCCLK output to GPIO */
				MAP_GPIOPinWrite(GPIO_PORTP_BASE, GPIO_PIN_3, 0);
				MAP_GPIOPinTypeGPIOOutput(GPIO_PORTP_BASE, GPIO_PIN_3);

				MAP_HibernateClockConfig( HIBERNATE_OSC_LOWDRIVE );
			}
		}
		else
		{
			DEBUG_PRINT("\r\nError: Disabling advertisement failed\r\n");
		}
		isBLEActive = FALSE;
		BLEStatus = false;
	}

	return (ret_val);
}

/**
 * Entry function to BLE Main task
 *
 * The following function is the main user interface thread.  It opens
 * the Bluetooth Stack and then drives the main user interface
 *
 * @return	None
 */
void BLEThreadMain()
{
	bool bleEndSemWaitTimedOut = FALSE;
	bool bleCmdRespWaitTimedOut = FALSE;
	bool bleNotifyWaitTimedOut = FALSE;

	int Result = 0;
	BTPS_Initialization_t BTPS_Initialization;
	HCI_DriverInformation_t HCI_DriverInformation;
	HCI_HCILLConfiguration_t HCILLConfig;
	HCI_Driver_Reconfigure_Data_t DriverReconfigureData;

	while (1)
	{
		Semaphore_pend(BLEStartSem, BIOS_WAIT_FOREVER);
		BTPS_Delay(2000);

#ifdef DEBUG_MSGS
		DEBUG_PRINT("\r\nIn BLE Main\r\n");
#endif
		/* Configure the hardware for its intended use.                      */
		HAL_ConfigureHardware(0);

		/* Initialize other modules						*/
		InitBLEApplicationStateInfo();

		/* Configure the UART Parameters.                                    */
		HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, VENDOR_BAUD_RATE, cpHCILL_RTS_CTS);
		HCI_DriverInformation.DriverInformation.COMMDriverInformation.InitializationDelay = 100;

		/* Set up the application callbacks.                                 */
		BTPS_Initialization.MessageOutputCallback = DisplayCallback;

		/* Initialize the Stack/GAPS/GATT                                       */
		if ((Result = InitializeApplication(&HCI_DriverInformation,&BTPS_Initialization)) > 0)
		{
			/* Register a sleep mode callback if we are using HCILL Mode.     */
			if ((HCI_DriverInformation.DriverInformation.COMMDriverInformation.Protocol == cpHCILL) ||
				(HCI_DriverInformation.DriverInformation.COMMDriverInformation.Protocol == cpHCILL_RTS_CTS))
			{
				HCILLConfig.SleepCallbackFunction = HCI_Sleep_Callback;
				HCILLConfig.SleepCallbackParameter = 0;
				DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_HCILL_PARAMETERS;
				DriverReconfigureData.ReconfigureData = (void *) &HCILLConfig;

				/* Register the sleep mode callback.  Note that if this        */
				/* function returns greater than 0 then sleep is currently     */
				/* enabled.                                                    */
				Result = HCI_Reconfigure_Driver((unsigned int) Result, FALSE, &DriverReconfigureData);
				if (Result >= 0)
				{
					DEBUG_PRINT("\r\nSleep is allowed\r\n");
				}
				else
				{
					DEBUG_PRINT("\r\nEnabling sleep failed, error code:%d", Result);
				}
			}
			else
				DEBUG_PRINT("\r\nNot using HCILL mode\r\n");
		}

		/* Initialize BLE Message Handler				*/
		InitBLECmdHandlerLiason();
		bleNotificationHandler_Init();

		/* Register all applicable Services				*/
		RegisterBLEServices();

		/* Now that setup is complete, start advertising	*/
		AdvertiseLE(TRUE);

		isBLEActive = TRUE;

		/* Loop forever and process UART characters.                      */
		while(isBLEActive)
		{
			bleEndSemWaitTimedOut = Semaphore_pend(BLEEndSem, END_BLE_SEM_WAIT_TIME);
			if (!bleEndSemWaitTimedOut)
			{
				bleCmdRespWaitTimedOut = Semaphore_pend(semBLECmdComp, BLE_CMD_PROCESSING_TIMEOUT);

				if (bleCmdRespWaitTimedOut)
				{
					DEBUG_PRINT("\r\nBLE - Command Handler Response received\r\n");
					if (bleCmdHandlerLiaison_handleBLEResponse(FALSE, 0) != 0)
						DEBUG_PRINT("\r\nError: BLE Reponse handler reported failure\r\n");
				}
				else
				{
					bleNotifyWaitTimedOut = Semaphore_pend(BLENotifySem, BLE_NOTIFY_PROC_TIMEOUT);
					if (bleNotifyWaitTimedOut)
					{
						DEBUG_PRINT("\r\nBLE - Notification to be sent\r\n");
						if (bleNotificationHandler_sendNotification() != 0)
							DEBUG_PRINT("\r\nError: BLE Send Notification failure\r\n");
						if (bleNotificationHandler_sendIndication() != 0)
							DEBUG_PRINT("\r\nError: BLE Send Indication failure\r\n");
					}
				}
			}
			else
			{
				if (CloseBLEStack() < 0)
				{
					DEBUG_PRINT("\r\nError: Closing BLE stack failed\r\n");
				}
			}
		}

		BTPS_Delay(200);
	}
}
#endif
