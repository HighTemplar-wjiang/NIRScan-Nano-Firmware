/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */


#ifndef NANO_TIMER_H_
#define NANO_TIMER_H_

#include <time.h>
/*
 * The two #defines must be defined as per the timer configured in app_nano.cfg
*/
#define NANO_TIMER_TICKS_PER_MIN 240
#define NANO_TIMER_TICKS_PER_SEC 4
#define SLEW_TIMER_PERIOD_US 30

#ifdef __cplusplus
extern "C" {
#endif

int nano_timer_get_activity_count(void);
int nano_timer_get_tick(void);
void nano_timer_increment_activity_count(void);
void nano_timer_reset_activity_count();
int nano_hibernate_calendar_get(struct tm *psTime);
int nano_set_hibernate(bool newValue);
bool nano_get_hibernate();
uint32_t Get_Slew_timing(void);
void Reset_slew_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* NANO_TIMER_H_ */
