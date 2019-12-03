/*
 * Header for BQ24250 Temperature and humidity sensor
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */


// BQ24250 I2C address
#define BQ24250_I2CADDR 	0x6A

// BQ24250 Register 1 Bit Fields
#define BQ24250_R1_WD_FAULT    		0x80
#define BQ24250_R1_WD_EN	   		0x40
#define BQ24250_R1_WD_DIS	   		0x00
#define BQ24250_R1_STATUS	   		0x30
#define BQ24250_R1_FAULT	   		0x0F

// BQ24250 Register 1 Bits Values
#define BQ24250_READY		   		0x00
#define BQ24250_CHARGING		   	0x10
#define BQ24250_CHARGED			   	0x20
#define BQ24250_FAULT			   	0x30

#define BQ24250_NORMAL			  	0x00
#define BQ24250_IN_OVP			  	0x01
#define BQ24250_IN_UVLO		 		0x02
#define BQ24250_SLEEP		   		0x03
#define BQ24250_BAT_TEMP   			0x04
#define BQ24250_BAT_OVP		   		0x05
#define BQ24250_THERMAL_SHUT   		0x06
#define BQ24250_TIMER_FAULT	 		0x07
#define BQ24250_NO_BAT		   		0x08
#define BQ24250_ISET_SHORT	   		0x09
#define BQ24250_IN_FAULT	   		0x0A

// BQ24250 Register 2 Bit Fields
#define BQ24250_R2_RESET   			0x80
#define BQ24250_R2_ILIMIT	   		0x70
#define BQ24250_R2_EN_STAT	   		0x08
#define BQ24250_R2_EN_TERM	   		0x04
#define BQ24250_R2_CHARGE_DIS	   	0x02
#define BQ24250_R2_CHARGE_EN	   	0x00
#define BQ24250_R2_HZ_MODE	   		0x01

// BQ24250 Register 3 Bit Fields
#define BQ24250_R3_VBATREG  		0xFC
#define BQ24250_R3_USB_DET	   		0x03

// BQ24250 Register 3 Bit Values
#define BQ24250_R3_VBATREG  		0xFC
#define BQ24250_R3_USB_DET	   		0x03
#define BQ24250_R3_VBATREG_640 		0x80
#define BQ24250_R3_VBATREG_320 		0x40
#define BQ24250_R3_VBATREG_160 		0x20
#define BQ24250_R3_VBATREG_080 		0x10
#define BQ24250_R3_VBATREG_040 		0x08
#define BQ24250_R3_VBATREG_020 		0x04
#define BQ24250_R3_USB_DCP 			0x00
#define BQ24250_R3_USB_CDP			0x01
#define BQ24250_R3_USB_SDP			0x02
#define BQ24250_R3_USB_APPLE		0x03

// BQ24250 Register 4 Bit Fields
#define BQ24250_R4_ICHG		  		0xF8
#define BQ24250_R4_ITERM	   		0x07

// BQ24250 Register 4 Bit Values
#define BQ24250_R4_ICHG_800  		0x80
#define BQ24250_R4_ICHG_400  		0x40
#define BQ24250_R4_ICHG_200  		0x20
#define BQ24250_R4_ICHG_100  		0x10
#define BQ24250_R4_ICHG_050  		0x08
#define BQ24250_R4_ITERM_100  		0x04
#define BQ24250_R4_ITERM_050  		0x02
#define BQ24250_R4_ITERM_025  		0x01

// BQ24250 Register 5 Bit Fields
#define BQ24250_R5_LOOP_ST	  		0xC0
#define BQ24250_R5_LOW_CHG	   		0x20
#define BQ24250_R5_DPDM_EN	   		0x10
#define BQ24250_R5_CE_STATUS	   	0x08
#define BQ24250_R5_VINDPM	   		0x07

// BQ24250 Register 5 Bit Values
#define BQ24250_R5_VINDPM_320  		0x04
#define BQ24250_R5_VINDPM_160  		0x02
#define BQ24250_R5_VINDPM_080  		0x01

// BQ24250 Register 6 Bit Fields
#define BQ24250_R6_2XTRM_EN  		0x80
#define BQ24250_R6_TMR		  		0x60
#define BQ24250_R6_SYSOFF	  		0x10
#define BQ24250_R6_TS_EN	  		0x08
#define BQ24250_R6_TS_STAT  		0x07

// BQ24250 Register 6 Bit Values
#define BQ24250_R6_TMR_075  		0x00
#define BQ24250_R6_TMR_6	  		0x01
#define BQ24250_R6_TMR_9  			0x02
#define BQ24250_R6_TMR_DIS 			0x03
#define BQ24250_R6_TS_NORMAL		0x00
#define BQ24250_R6_TS_HOT			0x01
#define BQ24250_R6_TS_WARM			0x02
#define BQ24250_R6_TS_COOL			0x03
#define BQ24250_R6_TS_COLD			0x04
#define BQ24250_R6_TS_FREEZE		0x05
#define BQ24250_R6_TS_SUBFREEZE		0x06
#define BQ24250_R6_TS_OPEN			0x07

// BQ24250 Register 7 Bit Fields
#define BQ24250_R7_VOVP		  		0xE0
#define BQ24250_R7_CLR_VDP		  	0x10
#define BQ24250_R7_BAT_DET	  		0x08
#define BQ24250_R7_PTM		  		0x04

// BQ24250 Register 7 Bit Values
#define BQ24250_R7_VOVP600 			0x00
#define BQ24250_R7_VOVP650 			0x01
#define BQ24250_R7_VOVP700 			0x02
#define BQ24250_R7_VOVP800 			0x03
#define BQ24250_R7_VOVP900 			0x04
#define BQ24250_R7_VOVP950 			0x05
#define BQ24250_R7_VOVP100 			0x06
#define BQ24250_R7_VOVP105 			0x07





// BQ24250 Registers
#define BQ24250_REG1  	0x00
#define BQ24250_REG2 	0x01
#define BQ24250_REG3	0x02
#define BQ24250_REG4 	0x03
#define BQ24250_REG5	0x04
#define BQ24250_REG6	0x05
#define BQ24250_REG7	0x06
#define BQ24250_MANID 	0xFE
#define BQ24250_DEVID	0xFF

// TI Manufacturer and Device ID
#define TI_MANID			0x5449
#define TI_BQ24250_DEVID	0x1000

#ifdef __cplusplus
extern "C" {
#endif

int16_t bq24250_init( void );
int32_t bq24250_readStatus( void );
int32_t bq24250_readBatteryVoltage( void );
int32_t bq24250_readUSBDetection( void );
int32_t bq24250_readChargeCurrent( void );
int32_t bq24250_readTSFault( void );
int32_t bq24250_get_register( uint8_t );
int32_t bq24250_write_command( uint8_t, uint16_t );

#ifdef __cplusplus
}
#endif

