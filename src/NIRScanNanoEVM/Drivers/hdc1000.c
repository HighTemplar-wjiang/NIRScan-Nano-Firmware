/*
 *
 * APIs to configure and communicate with hdc1000 temperature sensor over I2C 
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
#include "GPIO Mapping.h"
#include "nnoStatus.h"
#include "hdc1000.h"

static bool HDC1000Init;

//*****************************************************************************
//
// Called by the NVIC as a result of GPIOP7 interrupt event. For this
// application GPIO PP7 is the interrupt line for the HDC1000.
//
//*****************************************************************************
void GPIOP7IntHandler(void)
{
    uint32_t ui32Status;
    //float	temp, hum;

    ui32Status = GPIOIntStatus(GPIO_PORTP_BASE, true);

    // Clear all the pin interrupts that are set
    MAP_GPIOIntClear(GPIO_PORTP_BASE, ui32Status);

    if(ui32Status & GPIO_PIN_7)
    {
        // This interrupt indicates a conversion is complete and ready to be
        // fetched.  So we start the process of getting the data.
        //hdc1000_DataTemperatureGetFloat( hdc1000_i2c, &temp, &hum );
    }
}

//*****************************************************************************
//
//! Resets the HDC1000.
//!
//! \param i2c is a handle to the HDC1000 i2c driver.
//!
//! This function resets the HDC1000 device, clearing any previous
//! configuration data.
//!
//! \return Returns PASS if the HDC1000 device was successfully reset and FAIL
//! if it was not.
//
//*****************************************************************************
int16_t hdc1000_reset( I2C_Handle i2c )
{
    I2C_Transaction   i2cTransaction;
    uint8_t cmds[3] = {HDC1000_CONFIG, HDC1000_CFG_RESET >> 8, 
		HDC1000_CFG_RESET & 0x00FF};

    i2cTransaction.slaveAddress = BOARD_HDC1000_ADDR;
    i2cTransaction.writeBuf = cmds;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
            DEBUG_PRINT(("  Reset HDC1000\n" ));
    		return PASS;
    }
	else
	{
            DEBUG_PRINT(("I2C HDC1000 Bus fault\n"));
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_RESET);
    		return FAIL;
    }
}

//*****************************************************************************
//
//! Reads data from HDC1000 registers.
//!
//! \param i2c is a handle to the HDC1000 i2c driver.
//! \param addr is the register address.
//! \param pValue is a pointer to the location to store the data that is
//! read.
//!
//! This function reads the contents of the register addr of the HDC1000.
//!
//! \return Returns PASS if the read was successful and FAIL if not.
//
//*****************************************************************************
static int16_t hdc1000_ReadReg( I2C_Handle i2c, uint32_t addr, uint32_t *pValue )
{
    I2C_Transaction   i2cTransaction;
    uint8_t cmds[1] = {addr};
    uint8_t val[2] = {0, 0};

    i2cTransaction.slaveAddress = BOARD_HDC1000_ADDR;
    i2cTransaction.writeBuf = cmds;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = val;
    i2cTransaction.readCount = 2;

    if (I2C_transfer( i2c, &i2cTransaction))
    {
            DEBUG_PRINT("  HDC1000 Read Register:  0x%x\n", addr );
    }
    else
    {
            DEBUG_PRINT("I2C HDC1000 Bus fault\n");
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_READREGISTER );
    		return FAIL;
    }
    
    *pValue = ((uint32_t)val[0] << 8);
    *pValue |= (uint32_t)val[1];

    return PASS;
}

//*****************************************************************************
//
//! Writes data to HDC1000 registers.
//!
//! \param i2c is a handle to the HDC1000 i2c driver.
//! \param addr is the register address.
//! \param value is the value read
//!
//! This function stores value into the register addr of the HDC1000
//!
//! \return Returns PASS if the write was successful and FAIL if not.
//
//*****************************************************************************
static int16_t hdc1000_WriteReg( I2C_Handle i2c, uint32_t addr, uint32_t value )
{
    I2C_Transaction   i2cTransaction;
    uint8_t cmds[3] = {addr, value >> 8, value & 0x00FF };

    i2cTransaction.slaveAddress = BOARD_HDC1000_ADDR;
    i2cTransaction.writeBuf = cmds;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (I2C_transfer( i2c, &i2cTransaction))
    {
		DEBUG_PRINT("  HDC1000 Wrote Register: 0x%x with 0x%x\n", addr, value );
		return PASS;
    }
    else
    {
		DEBUG_PRINT("I2C HDC1000 Bus fault\n");
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_WRITEREGISTER );
		return FAIL;
    }
}

//*****************************************************************************
//
//! Triggers HDC1000 measurment.
//!
//! \param i2c is a handle to the HDC1000 i2c driver.
//!
//! This function trigger a temperature/humidity measurement
//!
//! \return Returns PASS if the write was successfully started and FAIL if not
//
//*****************************************************************************
static int16_t hdc1000_Trigger( I2C_Handle i2c )
{
    I2C_Transaction   i2cTransaction;
    uint8_t cmds[1] = {0x00 };

    i2cTransaction.slaveAddress = BOARD_HDC1000_ADDR;
    i2cTransaction.writeBuf = cmds;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (I2C_transfer( i2c, &i2cTransaction))
    {
		DEBUG_PRINT(("  HDC1000 Triggered Measurement\n"));
		return PASS;
    }
    else
    {
		DEBUG_PRINT(("I2C HDC1000 Bus fault\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_WRITEREGISTER );
		return FAIL;
    }
}


//*****************************************************************************
//
//! Reads the temperature data from the HDC1000.
//!
//! \param i2c is a handle to the HDC1000 i2c driver.
//! \pValue i2c is a pointer to a byte array of four 8-bit values
//!
//! This function initiates a read of the HDC1000 data registers. Reads
//| two bytes for Temperature and  two bytes for Humidity. When the read
//! has completed, the new readings can be obtained via:
//!
//! - HDC1000DataTempandHumGetRaw()
//! - HDC1000DataTempandHumGetFloat()
//!
//! \return Returns PASS if the read was successful and FAIL if not.
//
//*****************************************************************************
static int16_t  hdc1000_DataRead( I2C_Handle i2c, uint8_t *pValue )
{

    I2C_Transaction   i2cTransaction;
    uint8_t cmds[1] = {0x00};

    i2cTransaction.slaveAddress = BOARD_HDC1000_ADDR;
    i2cTransaction.writeBuf = cmds;
    i2cTransaction.writeCount = 0;
    i2cTransaction.readBuf = pValue;
    i2cTransaction.readCount = 4;

    if (I2C_transfer( i2c, &i2cTransaction))
    {
		DEBUG_PRINT(("  HDC1000 Wrote Register \n" ));
		return PASS;
    }
    else
    {
		DEBUG_PRINT(("I2C HDC1000 Bus fault\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_READREGISTER );
		return FAIL;
    }
}

//*****************************************************************************
//
//! Gets the raw measurement data from the most recent data read.
//!
//! \param i2c is a handle to the HDC1000 i2c driver.
//! \param pi16Ambient is a pointer to the value into which the raw ambient
//! temperature data is stored.
//! \param pi16OHum is a pointer to the value into which the raw humidity
//!  data is stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return None.
//
//*****************************************************************************
static int16_t hdc1000_DataTemperatureGetRaw( I2C_Handle i2c, 
		uint16_t *pi16Temp, uint16_t *pi16Hum)
{
	uint8_t rawVal[4];
    // Return the raw temperature value.
    if ( hdc1000_DataRead( i2c, rawVal ) != PASS)
          return FAIL;
          
    *pi16Temp = ((uint16_t)rawVal[0] << 8) | rawVal[1];
    *pi16Hum = ((uint16_t)rawVal[2] << 8) | rawVal[3];
    
    return PASS;
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param pfTemp is a pointer to the value into which the ambient
//! temperature data is stored as floating point degrees Celsius.
//! \param pfHum is a pointer to the value into which the humidity
//! data is stored as floating point degrees Celsius.
//!
//! This function returns the temperature data from the most recent data read,
//! converted into Celsius.
//!
//! \return None.
//
//*****************************************************************************
int16_t hdc1000_DataTemperatureGetFloat(float *pfTemp, float *pfHum)
{
    I2C_Params  i2cParams;
    I2C_Handle  hdc1000_i2c;
    uint16_t     i16Temp;
    uint16_t     i16Hum;
    uint32_t 	timeoutCounter0 = HDC1000_TIMEOUT;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C6);
    
	I2C_Params_init( &i2cParams );
	i2cParams.bitRate = I2C_400kHz;
	hdc1000_i2c = I2C_open( BOARD_I2C_HDC, &i2cParams );
	if (hdc1000_i2c == NULL)
	{
		DEBUG_PRINT(("  Error Initializing I2C6\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_I2C );
		return FAIL;
	}
	
	if ( !HDC1000Init )
	{
		if ( hdc1000_WriteReg( hdc1000_i2c, HDC1000_CONFIG,	HDC1000_CFG_MODEDUAL
				   	| HDC1000_CFG_TRES14 | HDC1000_CFG_HRES14 ) != PASS)
			return FAIL;
		HDC1000Init = true;
	}
	// Trigger the measurement
	if ( hdc1000_Trigger( hdc1000_i2c ) != PASS)
	{
        DEBUG_PRINT(("  Error Triggering\n"));
        return FAIL;
    }
	
	// Wait until DRDY is low
	while( (HDC_DRDY_GPIO_PORT & HDC_DRDY_GPIO_MASK) && (--timeoutCounter0));
	if (!timeoutCounter0 )
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_TIMEOUT );
		return FAIL;
	}

    // Get the raw readings.
    if ( hdc1000_DataTemperatureGetRaw( hdc1000_i2c, &i16Temp, &i16Hum) != PASS )
        return FAIL;

    *pfTemp = i16Temp / 65536.0 * 165.0 - 40.0;

    *pfHum = i16Hum / 65536.0 * 100.0;
	
	I2C_close( hdc1000_i2c );
	
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C6);
    
    return PASS;
}

//*****************************************************************************
//
//! Initializes I2C6 for HDC1000
//! 
//! Initializes HDC1000 using I2C6 at 400KHz.
//! Reads Manufacturer ID and Device ID to check if initialization was successful
//! 
//! \return PASS for success, FAIL for failure
//
//*****************************************************************************
int16_t hdc1000_test( )
{
    I2C_Params      i2cParams;
    I2C_Handle      hdc1000_i2c;
    uint32_t manuID, deviceID;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C6);

	I2C_Params_init( &i2cParams );
	i2cParams.bitRate = I2C_400kHz;
	hdc1000_i2c = I2C_open( BOARD_I2C_HDC, &i2cParams );
	if (hdc1000_i2c == NULL)
	{
		DEBUG_PRINT(("  Error Initializing I2C6\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_I2C );
		return FAIL;
	}

	DEBUG_PRINT((" HDC1000 Init\n"));

	if ( hdc1000_reset( hdc1000_i2c ) != PASS)
		return FAIL;
		
	// Delay for 10 milliseconds for HDC1000 reset to complete.
	// Not explicitly required. Datasheet does not say how long a reset takes.
	MAP_SysCtlDelay( NIRSCAN_SYSCLK / (100 * 3));
	
	//Read Manufacturer ID
	if ( hdc1000_ReadReg( hdc1000_i2c, HDC1000_MANID, &manuID ) != PASS)
		return FAIL;
	if (manuID != TI_MANID )
	{
		DEBUG_PRINT("  Invalid HDC1000 Manufacturer ID value: 0x%x\n", manuID);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_MANUID );
		return FAIL;
	}

	//Read Device ID
	if ( hdc1000_ReadReg( hdc1000_i2c, HDC1000_DEVID, &deviceID ) != PASS)
		return FAIL;
		
	if ( (deviceID != TI_HDC1000_DEVID) && (deviceID != TI_HDC1080_DEVID)  )
	{
		DEBUG_PRINT("  Invalid HDC1000 or HDC1080 Device ID value: 0x%x\n", deviceID);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_HDC1000, true,
				NNO_ERROR_HDC1000_DEVID );
		return FAIL;
	}

	// Set HDC1000 for temperature and humidity with 14-bits
	if ( hdc1000_WriteReg( hdc1000_i2c, HDC1000_CONFIG,	HDC1000_CFG_MODEDUAL 
				| HDC1000_CFG_TRES14 | HDC1000_CFG_HRES14 ) != PASS)
		return FAIL;
	HDC1000Init = true;
		
	I2C_close( hdc1000_i2c );
	
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C6);

    return PASS;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************




