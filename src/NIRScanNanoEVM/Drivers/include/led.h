/*
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef LED_H_
#define LED_H_

#ifdef __cplusplus
extern "C" {
#endif

void ToggleGreenUserLED( void );
void yellowLED_on( void );
void yellowLED_off( void );
void blueLED_on( void );
void blueLED_off( void );
void greenLED_on();
void greenLED_off();

#ifdef __cplusplus
}
#endif

#endif /* LED_H_ */
