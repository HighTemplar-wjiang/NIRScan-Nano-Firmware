/*
 * Header for HDC1000 Temperature and humidity sensor
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 */

#ifndef _HDC1000_H
#define _HDC1000_H

#include <ti/drivers/I2C.h>

// HDC1000 I2C address
#define HDC1000_I2CADDR 	0x40

// HDC1000 Config Register Bits
#define HDC1000_CFG_RESET    		0x8000
#define HDC1000_CFG_MODEDUAL   		0x1000
#define HDC1000_CFG_MODESINGLE   	0x0000
#define HDC1000_CFG_BTST		  	0x0800
#define HDC1000_CFG_TRES14		  	0x0000
#define HDC1000_CFG_TRES11		 	0x0400
#define HDC1000_CFG_HRES14   		0x0000
#define HDC1000_CFG_HRES11   		0x0100
#define HDC1000_CFG_HRES8   		0x0200

// HDC1000 Registers
#define HDC1000_TEMP  	0x00
#define HDC1000_HUM 	0x01
#define HDC1000_CONFIG	0x02
#define HDC1000_SID1 	0xFB
#define HDC1000_SID2	0xFC
#define HDC1000_SID3	0xFD
#define HDC1000_MANID 	0xFE
#define HDC1000_DEVID	0xFF

// TI Manufacturer and Device ID
#define TI_MANID			0x5449
#define TI_HDC1000_DEVID	0x1000
#define TI_HDC1080_DEVID	0x1050

// The states of the HDC1000 state machine.
#define HDC1000_STATE_IDLE       0
#define HDC1000_STATE_INIT       1
#define HDC1000_STATE_READ       2
#define HDC1000_STATE_WRITE      3
#define HDC1000_STATE_RMW        4
#define HDC1000_STATE_READ_TH    5

#define HDC1000_TIMEOUT 360000		// Conversion time is 6.5msec for 14-bit

#ifdef __cplusplus
extern "C" {
#endif

int16_t hdc1000_test( void );
int16_t hdc1000_DataTemperatureGetFloat( float *, float *);
int16_t hdc1000_reset( I2C_Handle );

#ifdef __cplusplus
}
#endif

#endif //_HDC1000_H


