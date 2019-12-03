/*
 *
 * APIs to configure and communicate with TMP006 temperature sensor over I2C bus
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
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
#include "nnoStatus.h"
#include "tmp006.h"

static bool TMP006Init;

//*****************************************************************************
//
// Called by the NVIC as a result of GPIOP6 interrupt event. For this
// application GPIO PP6 is the interrupt line for the TMP006.
//
//*****************************************************************************
void GPIOP6IntHandler(void)
{
	uint32_t ui32Status;

	ui32Status = MAP_GPIOIntStatus(GPIO_PORTP_BASE, true);

	// Clear all the pin interrupts that are set
	MAP_GPIOIntClear(GPIO_PORTP_BASE, ui32Status);

	if(ui32Status & GPIO_PIN_6)
	{
		// This interrupt indicates a conversion is complete and ready to be
		// fetched.  So we start the process of getting the data.
		//tmp006_DataTemperatureGetFloat( tmp006_i2c, &ambientTemp, &objectTemp);
	}
}

//*****************************************************************************
//
//! Resets the TMP006.
//!
//! \param i2c is a handle to the TMP006 i2c driver.
//!
//! This function resets the TMP006 device, clearing any previous
//! configuration data.
//!
//! \return Returns PASS if the TMP006 device was successfully reset and FAIL
//! if it was not.
//
//*****************************************************************************
static int16_t tmp006_reset( I2C_Handle i2c )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[3] = {TMP006_CONFIG, TMP006_CFG_RESET >> 8, TMP006_CFG_RESET
		& 0x00FF};

	i2cTransaction.slaveAddress = BOARD_TMP006_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 3;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		DEBUG_PRINT(("  Reset TMP006\n" ));
		return PASS;
	}
	else
	{
		DEBUG_PRINT(("I2C TMP006 Bus fault\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_RESET);
		return FAIL;
	}
}

//*****************************************************************************
//
//! Reads data from TMP006 registers.
//!
//! \param i2c is a handle to the TMP006 i2c driver.
//! \param addr is the register address.
//! \param pValue is a pointer to the location to store the data that is
//! read.
//!
//! This function reads the contents of the register addr of the TMP006.
//!
//!
//! \return Returns PASS if the read was successfully started and FAIL if it was
//! not.
//
//*****************************************************************************
static int16_t tmp006_ReadReg( I2C_Handle i2c, uint16_t addr, uint16_t *pValue )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {addr};
	uint8_t val[2] = {0, 0};

	i2cTransaction.slaveAddress = BOARD_TMP006_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 2;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		DEBUG_PRINT("  TMP006 Read Register: 0x%x\n", addr );
	}
	else
	{
		DEBUG_PRINT("I2C TMP006 Bus fault\n");
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_READREGISTER);
		return FAIL;
	}

	*pValue = ((uint32_t)val[0] << 8);
	*pValue |= (uint32_t)val[1];

	return PASS;
}

//*****************************************************************************
//
//! Writes data to TMP006 registers.
//!
//! \param i2c is a handle to the TMP006 i2c driver.
//! \param addr is the register address.
//! \param value is the value read
//!
//! This function stores value into the register addr of the TMP006
//!
//! \return Returns PASS if the write was successfully started and FAIL if it was
//! not.
//
//*****************************************************************************
static int16_t tmp006_WriteReg( I2C_Handle i2c, uint16_t addr, uint16_t value )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[3] = {addr, value >> 8, value & 0x00FF };

	i2cTransaction.slaveAddress = BOARD_TMP006_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 3;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		DEBUG_PRINT("  TMP006 Wrote Register: 0x%x with 0x%x\n", addr, value );
		return PASS;
	}
	else
	{
		DEBUG_PRINT("I2C TMP006 Bus fault\n");
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_WRITEREGISTER);
		return FAIL;
	}
}

//*****************************************************************************
//
//! Reads the temperature data from the TMP006.
//!
//! \param i2c is a handle to the TMP006 i2c driver.
//! \pValue i2c is a pointer to a byte array of four 8-bit values
//!
//! This function initiates a read of the TMP006 data registers.  When the read
//! has completed, the new readings can be obtained via:
//!
//! - TMP006DataTemperatureGetRaw()
//! - TMP006DataTemperatureGetFloat()
//!
//! \return Returns PASS if the read was successfully started and FAIL if it 
//! was not.
//
//*****************************************************************************
static int16_t  tmp006_DataRead( I2C_Handle i2c, uint8_t *pValue, uint8_t cmd)
{

	I2C_Transaction   i2cTransaction;

	i2cTransaction.slaveAddress = BOARD_TMP006_ADDR;
	i2cTransaction.writeBuf = &cmd;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = pValue;
	i2cTransaction.readCount = 2;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		DEBUG_PRINT(("  TMP006 Read Data\n" ));
		return PASS;
	}
	else
	{
		DEBUG_PRINT(("I2C TMP006 Bus fault\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_READREGISTER);
		return FAIL;
	}
}

//*****************************************************************************
//
//! Gets the raw measurement data from the most recent data read.
//!
//! \param psInst is a pointer to the TMP006 instance data.
//! \param pi16Ambient is a pointer to the value into which the raw ambient
//! temperature data is stored.
//! \param pi16Object is a pointer to the value into which the raw object
//! temperature data is stored.
//!
//! This function returns the raw measurement data from the most recent data
//! read.  The data is not manipulated in any way by the driver.
//!
//! \return PASS or FAIL.
//
//*****************************************************************************
static int16_t tmp006_DataTemperatureGetRaw( I2C_Handle i2c, int16_t 
		*pi16Ambient, int16_t *pi16Object )
{
	uint8_t rawVal[4];
	// Return the raw temperature value.

	if ( tmp006_DataRead( i2c, rawVal, TMP006_VOBJ ) != PASS)
	{
		return FAIL;
	}

	if ( tmp006_DataRead( i2c, rawVal+2, TMP006_TAMB ) != PASS)
	{
		return FAIL;
	}

	// Return the raw temperature value.
	*pi16Ambient = (int16_t) ((rawVal[2] << 8) | rawVal[3]);
	*pi16Object = (int16_t) ((rawVal[0] << 8) | rawVal[1]);

	return PASS;
}

//*****************************************************************************
//
//! Gets the measurement data from the most recent data read.
//!
//! \param pfObject is a pointer to the TMP006 instance data.
//! \param pfAmbient is a pointer to the value into which the ambient
//! temperature data is stored as floating point degrees Celsius.
//! \param pfObject is a pointer to the value into which the object temperature
//! data is stored as floating point degrees Celsius.
//!
//! This function returns the temperature data from the most recent data read,
//! converted into Celsius.
//!
//! \return None.
//
//*****************************************************************************
int16_t tmp006_DataTemperatureGetFloat(float *pfAmbient, float *pfObject)
{
	I2C_Params      i2cParams;
	I2C_Handle      tmp006_i2c;
	float fTdie2, fS, fVo, fVx, fObj;
	int16_t i16Ambient;
	int16_t i16Object;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);
	/*
	 * Review comment - SK
	 * We seem to perform I2C_Params_init() I2C_open() and I2C_close()
	 * for everytime the function is called, is there any reason? Is it because 
	 * same same I2C core driver module used? if not we can avoid repeated calls.
	 */

	I2C_Params_init( &i2cParams );
	i2cParams.bitRate = I2C_400kHz;
	tmp006_i2c = I2C_open( BOARD_I2C_TMP, &i2cParams );
	if (tmp006_i2c == NULL)
	{
		DEBUG_PRINT(("  Error Initializing I2C7\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_I2C);
		return FAIL;
	}

	if ( !TMP006Init )
	{
		if ( tmp006_WriteReg( tmp006_i2c, TMP006_CONFIG, TMP006_CFG_MODEON | \
					TMP006_CFG_AVE16SAMPLE | TMP006_CFG_EN_DRDY_PIN ) != PASS)
		{
			return FAIL;
		}
		TMP006Init = true;
	}

	// Get the raw readings.
	if ( tmp006_DataTemperatureGetRaw( tmp006_i2c, &i16Ambient, &i16Object) )
		return FAIL;

	// The bottom two bits are not temperature data, so discard them but keep 
	// the sign information.
	*pfAmbient = (float)(i16Ambient / 4);

	// Divide by 32 to get unit scaling correct.
	*pfAmbient = *pfAmbient / 32.0;

	// fTdie2 is measured ambient temperature in degrees Kelvin.
	fTdie2 = *pfAmbient + TMP006_TREF;

	// S is the sensitivity.
	fS = TMP006_S0 * (1.0f + (TMP006_A1 * (*pfAmbient)) + (TMP006_A2 * 
				((*pfAmbient) * (*pfAmbient))));

	// Vos is the offset voltage.
	fVo = TMP006_B0 + (TMP006_B1 * (*pfAmbient)) + (TMP006_B2 * ((*pfAmbient) 
				* (*pfAmbient)));

	// Vx is the difference between raw object voltage and Vos
	// 156.25e-9 is nanovolts per least significant bit from the voltage register.
	fVx = (((float) ((int32_t)i16Object)) * 156.25e-9) - fVo;

	// fObj is the feedback coefficient.
	fObj = fVx + TMP006_C2 * (fVx * fVx);

	// Finally calculate the object temperature.
	*pfObject = (sqrtf(sqrtf((fTdie2 * fTdie2 * fTdie2 * fTdie2) + 
					(fObj / fS))) - TMP006_TREF);

	I2C_close( tmp006_i2c );

	MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C7);

	return PASS;
}

//*****************************************************************************
//
//! Self test for TMP006
//! 
//! Initializes TMP006 using I2C7 at 400KHz.
//! Reads Manufacturer ID and Device ID to check if initialization was successful
//! 
//! \return 0 for success, 1 for failure
//
//*****************************************************************************
int16_t tmp006_test( void )
{
	I2C_Params      i2cParams;
	I2C_Handle      tmp006_i2c;
	uint16_t manuID, deviceID;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);

	I2C_Params_init( &i2cParams );
	i2cParams.bitRate = I2C_400kHz;
	tmp006_i2c = I2C_open( BOARD_I2C_TMP, &i2cParams );
	if (tmp006_i2c == NULL)
	{
		DEBUG_PRINT(("  Error Initializing I2C7\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_I2C);
		return FAIL;
	}

	DEBUG_PRINT((" TMP006 Init\n"));

	if ( tmp006_reset( tmp006_i2c ) != PASS)
		return FAIL;

	// Delay for 10 milliseconds for TMP006 reset to complete.
	// Not explicitly required. Datasheet does not say how long a reset takes.
	MAP_SysCtlDelay( NIRSCAN_SYSCLK / (100 * 3));

	//Read Manufacturer ID
	if ( tmp006_ReadReg( tmp006_i2c, TMP006_MANID, &manuID ) != PASS)
		return FAIL;

	if (manuID != TI_MANID )
	{
		DEBUG_PRINT("  Invalid TMP006 Manufacturer ID value, 0x%x\n", manuID);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_MANUID);
		return FAIL;
	}

	//Read Device ID
	if ( tmp006_ReadReg( tmp006_i2c, TMP006_DEVID, &deviceID ) != PASS)
		return FAIL;

	if (deviceID != TI_TMP006_DEVID )
	{
		DEBUG_PRINT("  Invalid TMP006 Device ID value, 0x%x\n", deviceID);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_DEVID);
		return FAIL;
	}

	// Enable the DRDY pin indication that a conversion is in progress.
	if ( tmp006_WriteReg( tmp006_i2c, TMP006_CONFIG, TMP006_CFG_MODEON | \
				TMP006_CFG_AVE16SAMPLE | TMP006_CFG_EN_DRDY_PIN ) != PASS)
		return FAIL;
	TMP006Init = true;

	I2C_close( tmp006_i2c );

	MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C7);
	return PASS;
}

//*****************************************************************************
//
//! Powers down TMP006
//!
//! \return PASS or FAIL.
//
//*****************************************************************************
int16_t tmp006_PowerDown( void )
{
    uint32_t err = 0;
	int timeoutCounter = TIMEOUT_COUNTER;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);
    MAP_SysCtlDelay(10);                                        // Wait six 6 clock cycles to ensure peripherals ready
    MAP_I2CMasterInitExpClk( I2C7_BASE, SysCtlClockGet(), 1 );	// Set bus speed to 400KHz and enable I2C block

    MAP_I2CMasterSlaveAddrSet( I2C7_BASE, BOARD_TMP006_ADDR, WRITE );		// Write I2C address of bq24250
    MAP_I2CMasterDataPut( I2C7_BASE, TMP006_CONFIG );						// Write bq24250 register into Pointer Register
    MAP_I2CMasterControl( I2C7_BASE, I2C_MASTER_CMD_BURST_SEND_START );	// Send START condition
    while ( !(MAP_I2CMasterIntStatusEx( I2C7_BASE, false) & I2C_MASTER_INT_DATA) );
	timeoutCounter = TIMEOUT_COUNTER;
	while (( MAP_I2CMasterBusy(I2C7_BASE) && --timeoutCounter));
	if (timeoutCounter == 0) 
	{
		DEBUG_PRINT(("  Error Initializing I2C7\n"));
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_TMP006, true, 
				NNO_ERROR_TMP006_I2C);
		return FAIL;
	}

	
    if ( (err = MAP_I2CMasterErr( I2C7_BASE )) )
    {
        MAP_I2CMasterControl( I2C7_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP );    // Send START condition
    	timeoutCounter = TIMEOUT_COUNTER;
        while (( !(MAP_I2CMasterIntStatusEx( I2C7_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
    	if (timeoutCounter == 0) return FAIL;
    	
    	timeoutCounter = TIMEOUT_COUNTER;
    	while (( MAP_I2CMasterBusy(I2C7_BASE) && --timeoutCounter));
    	if (timeoutCounter == 0) return FAIL;

        return FAIL;
    }

    MAP_I2CMasterDataPut( I2C7_BASE, TMP006_CFG_MODE_PD );
    MAP_I2CMasterControl( I2C7_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH ); // Send Byte
	timeoutCounter = TIMEOUT_COUNTER;
    while (( !(MAP_I2CMasterIntStatusEx( I2C7_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
	if (timeoutCounter == 0) return FAIL;
	
	timeoutCounter = TIMEOUT_COUNTER;
	while (( MAP_I2CMasterBusy(I2C7_BASE) && --timeoutCounter));
	if (timeoutCounter == 0) return FAIL;
	
    if ( (err = MAP_I2CMasterErr( I2C7_BASE )) )
    {
        MAP_I2CMasterControl( I2C7_BASE, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP );    // Send START condition
    	timeoutCounter = TIMEOUT_COUNTER;
        while (( !(MAP_I2CMasterIntStatusEx( I2C7_BASE, false) & I2C_MASTER_INT_DATA) && --timeoutCounter));
    	if (timeoutCounter == 0) return FAIL;
    	
    	timeoutCounter = TIMEOUT_COUNTER;
    	while(( MAP_I2CMasterBusy(I2C7_BASE) && --timeoutCounter));
    	if(timeoutCounter == 0)
    	{
    		return FAIL;
    	}
        return FAIL;
    }

	MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C7);
	return err;
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************




