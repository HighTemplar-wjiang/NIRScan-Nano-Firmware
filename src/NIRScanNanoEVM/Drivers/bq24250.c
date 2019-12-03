/*
 *
 * brief  APIs to configure and communicate with bq24250 temperature sensor over I2C bus
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inc/hw_memmap.h>
#include <driverlib/gpio.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/i2c.h>
#include <driverlib/sysctl.h>
#include <inc/tm4c129xnczad.h>
#include <ti/drivers/I2C.h>
#include <xdc/runtime/System.h>

#include "NIRscanNano.h"
#include "Board.h"
#include "common.h"
#include "bq24250.h"

/**
 *
 *  Initializes I2C9 for bq24250
 *
 *  Initializes bq24250 using I2C9 at 400KHz.
 *  Reads Manufacturer ID and Device ID to check if initialization was successful
 *
 *  \return 0 for success, 1 for failure
 */ 
int16_t bq24250_init( )
{
	int32_t raw;

    MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_I2C9 );           // Enable I2C9 Peripheral
    MAP_SysCtlDelay(10);                                        // Wait six 6 clock cycles to ensure peripherals ready
    MAP_I2CMasterInitExpClk( I2C9_BASE, SysCtlClockGet(), 1 );	// Set bus speed to 400KHz and enable I2C block


    DEBUG_PRINT((" BQ24250 Init\n"));
	
	/*
	* Review comment - SS
	* Relook at the code below to see if all the get_register calls are necessary
	*/
    
	raw = bq24250_readStatus();
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG2 );
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG3 );
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG4 );
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG5 );
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG5 );
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG6 );
	if(raw == FAIL)
		return FAIL;

    raw = bq24250_get_register( BQ24250_REG7 );
	if(raw == FAIL)
		return FAIL;

    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C9);
    return PASS;
}

/**
 *
 *  Reads Status
 *
 *  Reads Register 1 - Status
 *  
 *  \return Status register
 */ 
int32_t bq24250_readStatus( void ) 
{
	int32_t raw = 0;
  
	raw = bq24250_get_register( BQ24250_REG1 );
    if ( raw == FAIL )
    	return FAIL;
	DEBUG_PRINT("  BQ24250 Register 1 Status 0x%x\n", raw );
	return raw;
}

/**
 *
 *  Reads Battery Regulation Voltage
 *
 *  Reads Register 3 - Battery Regulation Voltage
 *  
 *  \return Status register
 */ 
int32_t bq24250_readBatteryVoltage( void ) 
{
	int32_t raw = PASS;
  
	raw = bq24250_get_register( BQ24250_REG3 );
    if ( raw == FAIL )
    	return FAIL;
	raw &= BQ24250_R3_VBATREG;
	raw >>= 2;

	DEBUG_PRINT("  BQ24250 Battery Regulation Voltage 0x%x\n", raw );
	return raw;
}

/**
 *
 *  Reads USB Detection Result
 *
 *  Reads Register 3 - USB D+/D- pins
 *  
 *  \return Status register
 */ 
int32_t bq24250_readUSBDetection( void ) 
{
	int32_t raw = 0;
  
	raw = bq24250_get_register( BQ24250_REG3 );
    if ( raw == FAIL )
    	return FAIL;
	raw &= BQ24250_R3_USB_DET;

	DEBUG_PRINT("  BQ24250 USB Detection Result 0x%x\n", raw );

    return raw;
}

/**
 *
 *  Reads Charge Current
 *
 *  Reads Register 4 - Charge current
 *  
 *  \return Status register
 */ 
int32_t bq24250_readChargeCurrent( void ) 
{
	int32_t raw = 0;
  
	raw = bq24250_get_register( BQ24250_REG4 );
    if ( raw == FAIL )
    	return FAIL;
	raw &= BQ24250_R4_ICHG;
	raw >>= 3;

	DEBUG_PRINT("  BQ24250 Charge Current 0x%x\n", raw );
	return raw;
}


/**
 *
 *  Reads Thermistor Fault Mode
 *
 *  Reads Register 6 - TS Fault Mode
 *  
 *  \return Status register
 */ 
int32_t bq24250_readTSFault( void ) 
{
	int32_t raw = 0;
  
	raw = bq24250_get_register( BQ24250_REG6 );
	if ( raw == FAIL )
		return FAIL;
	raw &= BQ24250_R6_TS_STAT;

	DEBUG_PRINT("  BQ24250 TS Fault 0x%x\n", raw );
	return raw;
}


/**
 *
 *  Reads a bq24250 register 
 *
 *  Reads a bq24250 register using I2C commands
 *
 *  \param reg_name is the register address
 *  
 *  \return value read from the register
 */ 
int32_t bq24250_get_register( uint8_t reg_name )
{
    uint32_t u32Aux = 0;
	int timeoutCounter = TIMEOUT_COUNTER;

    MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_I2C9 );           // Enable I2C9 Peripheral
    MAP_SysCtlDelay(10);                                        // Wait six 6 clock cycles to ensure peripherals ready
    MAP_I2CMasterSlaveAddrSet( I2C9_BASE, BQ24250_I2CADDR, WRITE );		// Write I2C address of bq24250
    MAP_I2CMasterDataPut( I2C9_BASE, reg_name );						// Write bq24250 register into Pointer Register
    MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_SINGLE_SEND );	// Send START condition

    timeoutCounter = TIMEOUT_COUNTER;
    while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register read timedout\n");
		return FAIL;
	}

    timeoutCounter = TIMEOUT_COUNTER;
    while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register read timedout\n");
		return FAIL;
	}
    if ( (MAP_I2CMasterErr( I2C9_BASE )) )
    {
        MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP );    // Send START condition
        timeoutCounter = TIMEOUT_COUNTER;
        while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
    	if(timeoutCounter == 0)
    	{
    		DEBUG_PRINT("BQ24250 register read timedout\n");
    		return FAIL;
    	}
        timeoutCounter = TIMEOUT_COUNTER;
		while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("BQ24250 register read timedout\n");
			return FAIL;
		}

        DEBUG_PRINT("  I2C9 Error => Reading Register '%x' in Start\n", reg_name );
        return FAIL;
    }

    timeoutCounter = TIMEOUT_COUNTER;
	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register read timedout\n");
		return FAIL;
	}
    MAP_I2CMasterSlaveAddrSet( I2C9_BASE, BQ24250_I2CADDR, READ );		// Write I2C address of bq24250
    MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE );	// Write I2C address of bq24250
    timeoutCounter = TIMEOUT_COUNTER;
    while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register read timedout\n");
		return FAIL;
	}
    timeoutCounter = TIMEOUT_COUNTER;
	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register read timedout\n");
		return FAIL;
	}
    if ( (MAP_I2CMasterErr( I2C9_BASE )) )
    {
        MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP );    // Send START condition
        timeoutCounter = TIMEOUT_COUNTER;
        while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("BQ24250 register read timedout\n");
			return FAIL;
		}
        timeoutCounter = TIMEOUT_COUNTER;
		while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("BQ24250 register read timedout\n");
			return FAIL;
		}

        DEBUG_PRINT("  I2C9 Error => Reading Register '%x' = '%x in Byte read'\n", reg_name );
        return FAIL;
    }
	timeoutCounter = TIMEOUT_COUNTER;
	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register read timedout\n");
		return FAIL;
	}
      u32Aux = (MAP_I2CMasterDataGet( I2C9_BASE )) & 0x000000FF;		// Read a byte back from I2C


	DEBUG_PRINT("  I2C9 Reading Register '%x' = '%x'\n", reg_name, u32Aux );
    MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_I2C9 );           // Disable I2C9 Peripheral
	return u32Aux;
}

/**
 *
 *  Sends bq24250 a command through I2C9
 *
 *  \param reg_name is the register address
 *  \param cmd is the command to be written
 *  
 *  \return None
 */ 
int32_t bq24250_write_command( uint8_t reg_name, uint16_t cmd )
{	
    uint32_t err = 0;
	int timeoutCounter = TIMEOUT_COUNTER;

    MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_I2C9 );           // Enable I2C9 Peripheral
    MAP_SysCtlDelay(10);                                        // Wait six 6 clock cycles to ensure peripherals ready
    MAP_I2CMasterSlaveAddrSet( I2C9_BASE, BQ24250_I2CADDR, WRITE );		// Write I2C address of bq24250
    MAP_I2CMasterDataPut( I2C9_BASE, reg_name );						// Write bq24250 register into Pointer Register
    MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_BURST_SEND_START );	// Send START condition
    while ( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) );
	timeoutCounter = TIMEOUT_COUNTER;
	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register write timedout\n");
		return FAIL;
	}
    if ( (err = MAP_I2CMasterErr( I2C9_BASE )) )
    {
        MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP );    // Send START condition
    	timeoutCounter = TIMEOUT_COUNTER;
        while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
    	if(timeoutCounter == 0)
    	{
    		DEBUG_PRINT("BQ24250 register write timedout\n");
    		return FAIL;
    	}
    	timeoutCounter = TIMEOUT_COUNTER;
    	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
    	if(timeoutCounter == 0)
    	{
    		DEBUG_PRINT("BQ24250 register write timedout\n");
    		return FAIL;
    	}

        DEBUG_PRINT("  I2C9 Error = %x Writing Register '%x'\n", err, reg_name );
        return FAIL;
    }

    MAP_I2CMasterDataPut( I2C9_BASE, cmd );
    MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH ); // Send Byte
	timeoutCounter = TIMEOUT_COUNTER;
    while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register write timedout\n");
		return FAIL;
	}
	timeoutCounter = TIMEOUT_COUNTER;
	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("BQ24250 register write timedout\n");
		return FAIL;
	}
    if ( (err = MAP_I2CMasterErr( I2C9_BASE )) )
    {
        MAP_I2CMasterControl( I2C9_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP );    // Send START condition
    	timeoutCounter = TIMEOUT_COUNTER;
        while (( !(MAP_I2CMasterIntStatusEx( I2C9_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
    	if(timeoutCounter == 0)
    	{
    		DEBUG_PRINT("BQ24250 register write timedout\n");
    		return FAIL;
    	}
    	timeoutCounter = TIMEOUT_COUNTER;
    	while(( MAP_I2CMasterBusy(I2C9_BASE) && --timeoutCounter));
    	if(timeoutCounter == 0)
    	{
    		DEBUG_PRINT("BQ24250 register write timedout\n");
    		return FAIL;
    	}
        DEBUG_PRINT("  I2C9 Error = %x Writing Register '%x' = '%x Value'\n", err, reg_name );
        return FAIL;
    }


	DEBUG_PRINT("  I2C9 Writing: Register '%x' = '%x'\n", reg_name, cmd);

    MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_I2C9 );           // Disable I2C9 Peripheral
	return err;

}
