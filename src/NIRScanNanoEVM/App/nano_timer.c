/*
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/tm4c129xnczad.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/timer.h>
#include <driverlib/gpio.h>
#include <driverlib/hibernate.h>
#include <driverlib/epi.h>

#include "button.h"
#include "common.h"
#include "GPIO Mapping.h"
#include "led.h"
#include "NIRscanNano.h"
#include "BLECommonDefs.h"
#include "nnoStatus.h"
#include "sdram.h"
#include "tmp006.h"
#include "nano_timer.h"
#include "cmdHandlerIFMgr.h"
#include "scan.h"

static uint32_t activity_counter = 0;
static uint32_t nano_timer_count = 0;
static uint32_t slew_timer_count = 0;
static bool nano_hibernate_enable = HIBERNATE_ENABLE_INIT_STATE;
extern bool b_sleeping;

#ifdef	NIRSCAN_INCLUDE_BLE
extern uint32_t g_BLEConnStatus;
extern Semaphore_Handle BLEEndSem;

#define NON_BLE_SCAN_ERRORS	~(NNO_ERROR_SCAN | NNO_ERROR_BLE)
#else
#define NON_BLE_SCAN_ERRORS	~(NNO_ERROR_SCAN)
#endif

static void UpdateStatusLED(bool toggle, uint32_t dev_stat, uint32_t error_stat)
{
	// Green LED - always toggling
	if (toggle || (error_stat & NON_BLE_SCAN_ERRORS))
		GREEN_GPIO_PORT ^= GREEN_GPIO_MASK;

   	// Yellow LED
   	// ON - during scan (handled in scan.c)
   	// Blinking - in case of scan error
   	if (error_stat & NNO_ERROR_SCAN)
   		YELLOW_GPIO_PORT ^= YELLOW_GPIO_MASK;
   	else if (dev_stat & NNO_STATUS_SCAN_IN_PROGRESS)
   		yellowLED_on();
   	else if (toggle)
   		yellowLED_off();

#ifdef	NIRSCAN_INCLUDE_BLE
	// Blue LED
   	// ON - when BLE stack is up & running
   	// Blinking - when a connection is active or in case of BLE error
	if ((error_stat & NNO_ERROR_BLE) ||
		(toggle && (dev_stat & NNO_STATUS_ACTIVE_BLE_CONNECTION)))
		BLUE_GPIO_PORT ^= BLUE_GPIO_MASK;
	else if (toggle && (dev_stat & NNO_STATUS_BLE_STACK_OPEN))
		blueLED_on();
	else if (!(dev_stat & NNO_STATUS_BLE_STACK_OPEN) &&
			 !(dev_stat & NNO_STATUS_ACTIVE_BLE_CONNECTION))
		blueLED_off();
#endif

	return;
}

void Slew_timer_handler(void)
{
  slew_timer_count = slew_timer_count + SLEW_TIMER_PERIOD_US;
}

void Reset_slew_timer(void)
{
	slew_timer_count = 0;
}

uint32_t Get_Slew_timing(void)
{
	return slew_timer_count;
}


/**
 * This function is called when TIMER2 times out
 * The timer is configured in appNano.cfg (currently at 250ms)
 */
void nano_timer_handler(void)
{
	NNO_error_status_struct error_status;
	uint32_t dev_status = 0;
	static uint8_t skip_tick = 0;

	if (nnoStatus_getErrorStatus(&error_status) < 0)
		return;

	if (nnoStatus_getDeviceStatus(&dev_status) < 0)
		return;

	nano_timer_count++;

	if ((skip_tick) || (error_status.status))
		UpdateStatusLED((skip_tick > 0), dev_status, error_status.status);

	// toggle skip_tick
	skip_tick = ~(skip_tick);

	/* After 5 minutes */
	if(nano_timer_count == NANO_TIMER_TICKS_PER_MIN*MAX_TIME_BEFORE_HIBERNATION_MINS)
	{
		nano_timer_count = 0;
		/* Check if there was any BLE or USB activity since last timeout */
		if((nano_timer_get_activity_count() == 0) && (nano_hibernate_enable == true))
		{
			/*
			 * Review comment - SK - Move BLE power down to after 4.5 secs
			 * Shouldn't we first disable the HW blocks receiving command messages?
			 * USB module interrupt disable
			 * BLE module interrupt disable
			 * By this way we are safely go to Hibernate Mode.
			 */
				b_sleeping = true;
				NIRscanNano_powerDown();
		}
		nano_timer_reset_activity_count();
	}
	else if (nano_timer_count == NANO_TIMER_TICKS_PER_MIN*MAX_TIME_BEFORE_IF_DISC_MINS)
	{
		if((nano_timer_get_activity_count() == 0) && (nano_hibernate_enable == true))
		{
#ifdef	NIRSCAN_INCLUDE_BLE
			if (cmdHandler_getActConnType() == CONN_BLE)
				Semaphore_post(BLEEndSem);
#endif
			cmdHandler_discNotification();
		}
	}

}

/**
 * This function returns the number of USB and/or Bluetooth
 * activities received and serviced by the system since that last
 * call to nano_timer_reset_activity_count() or system reset.
 */
int nano_timer_get_activity_count()
{
	return activity_counter;
}

/**
 * This function returns the number of TIMER2 ticks
 * This can be used to compute time elapsed in units of TIMER2 ticks
 * TIMER2 is configured in appNano.cfg (currently for 250ms timeout)
 */
int nano_timer_get_tick()
{
	return nano_timer_count;
}

/**
 * This function is to be called to register any of USB and/or Bluetooth
 * activities received and serviced by the system.
 * This counter will be used by the system to determine the total
 */
void nano_timer_increment_activity_count()
{
	activity_counter++;
}

/**
 * This function is to be called to reset the activity count
 * This counter is used to determine the total idle time elapsed.
 */
void nano_timer_reset_activity_count()
{
	activity_counter = 0;
}

/**
 * This is a wrapper function around HibernateCalendarGet() function
 * provided by the Hibernate module. It returns the Hibernation module's
 * date and time in calendar mode.
 * This function makes repeated reads of HibernateCalendarGet() until
 * success and with a timeout. It also implements a workaround for
 * a bug in HibernateCalendarGet() function that returns incorrect year and month
 */
int nano_hibernate_calendar_get(struct tm *psTime)
{
	int timeoutCounter;

	// Get the latest time.
	timeoutCounter = TIMEOUT_COUNTER;
	while ( MAP_HibernateCalendarGet( psTime )  && --timeoutCounter );

	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("Wait for MAP_HibernateCalendarGet() timed out\n");
		memset((void *)psTime, 0, sizeof(struct tm));
		return FAIL;
	}
	else
	{
		psTime->tm_year -= 100;	// Workaround for driverlib goofiness
		psTime->tm_mon += 1;      // Workaround for driverlib goofiness
		return PASS;
	}
}

int nano_set_hibernate(bool newValue)
{
	nano_hibernate_enable = newValue;
	return PASS;
}

bool nano_get_hibernate()
{
	return nano_hibernate_enable;
}
