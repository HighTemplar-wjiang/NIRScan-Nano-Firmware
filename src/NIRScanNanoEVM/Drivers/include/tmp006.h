/*
 * Header for TMP006 Thermopile sensor
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef __TMP006_H__
#define __TMP006_H__

//TMP006 I2C address
#define TMP006_I2CADDR 	0x40

// TMP006 Calculation Constants
#define TMP006_A1 	1.75e-03
#define TMP006_A2 	-1.678e-05
#define TMP006_B0 	-2.94e-05
#define TMP006_B1 	-5.70e-07
#define TMP006_B2 	4.63e-09
#define TMP006_C2 	13.4
#define TMP006_TREF 298.15
#define TMP006_S0 	6.4/100000000000000  // 6.4 * 10^-14

// TMP006 Config Register Bits
#define TMP006_CFG_RESET    	    0x8000		// Device reset
#define TMP006_CFG_MODEON   	    0x7000		// Operational mode
#define TMP006_CFG_MODE_PD    	    0x0000      // Power down
#define TMP006_CFG_AVE1SAMPLE  	    0x0000
#define TMP006_CFG_AVE2SAMPLE  	    0x0200
#define TMP006_CFG_AVE4SAMPLE  	    0x0400
#define TMP006_CFG_AVE8SAMPLE  	    0x0600
#define TMP006_CFG_AVE16SAMPLE 	    0x0800
#define TMP006_CFG_DRDYEN   	    0x0100		// Enable DRDY output pin
#define TMP006_CFG_DRDY     	    0x0080		// Data ready flag
#define TMP006_CFG_EN_DRDY_PIN_M    0x0100      // Enable the DRDY output pin
#define TMP006_CFG_DIS_DRDY_PIN     0x0000      // DRDY pin disabled
#define TMP006_CFG_EN_DRDY_PIN      0x0100      // DRDY pin enabled

// TMP006 Registers
#define TMP006_VOBJ  	0x00		// Raw Object Voltage mMasurement Register
#define TMP006_TAMB 	0x01		// Die Temperature Regsiter
#define TMP006_CONFIG	0x02		// TMP006 Configuration Regsiter
#define TMP006_MANID 	0xFE		// TMP006 Manufacture ID Register
#define TMP006_DEVID	0xFF		// TMP006 Device Identification Register

// TI Manufacturer and Device ID
#define TI_MANID			0x5449
#define TI_TMP006_DEVID		0x67

// The states of the TMP006 state machine.
#define TMP006_STATE_IDLE       0
#define TMP006_STATE_INIT       1
#define TMP006_STATE_READ       2
#define TMP006_STATE_WRITE      3
#define TMP006_STATE_RMW        4
#define TMP006_STATE_READ_AMB   5
#define TMP006_STATE_READ_OBJ   6

#ifdef __cplusplus
extern "C" {
#endif

int16_t tmp006_DataTemperatureGetFloat(float *pfAmbient, float *pfObject);
int16_t tmp006_test( void );
int16_t tmp006_PowerDown( void );

#ifdef __cplusplus
}
#endif

#endif // __TMP006_H__
