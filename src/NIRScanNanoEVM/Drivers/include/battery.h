/*
 *
 * contains code required to configure and handle trigger signals between DMD controller and embedded processor
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */
#ifndef BATTERY_H
#define BATTERY_H


/* Battery capacity states:
 Voltage	Capacity	Byte
 3.0 		= 0%    =>   0
 3.2 		= 5%    =>   5
 3.4 		= 20%   =>  20
 3.5 		= 40%   =>  40
 3.6 		= 60%	=>  60
 3.7 		= 80%	=>  80
 3.8 		= 100%  => 100
 */
#define BATT_00			3.0f
#define BATT_00_BYTE	  0
#define BATT_05			3.2f
#define BATT_05_BYTE	 05
#define BATT_20			3.4f
#define BATT_20_BYTE	 20
#define BATT_40			3.5f
#define BATT_40_BYTE	40
#define BATT_60			3.6f
#define BATT_60_BYTE	60
#define BATT_80			3.7f
#define BATT_80_BYTE	80
#define BATT_100		3.8f
#define BATT_100_BYTE	100

#ifdef __cplusplus
extern "C" {
#endif

void battery_read( float * );

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_H */
