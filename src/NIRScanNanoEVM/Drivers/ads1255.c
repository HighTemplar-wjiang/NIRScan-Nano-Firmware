//*****************************************************************************
//
// APIs to interface with ADS1255 ADC over SPI bus.
//
// Copyright (c) 2007-2015 Texas Instruments Incorporated.  All rights reserved.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <inc/tm4c129xnczad.h>
#include <inc/hw_memmap.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/gpio.h>
#include <driverlib/ssi.h>
#include <driverlib/sysctl.h>
#include <xdc/runtime/System.h>

#include "GPIO Mapping.h"
#include "NIRscanNano.h"
#include "nnoStatus.h"
#include "ads1255.h"

// timeout for waiting on DRDY toggle
#define DRDY_TIMEOUT TIMEOUT_COUNTER/8

static int32_t ads1255_ReadRegister(uint32_t addr, uint32_t *pRegVal);
static int32_t ads1255_WriteRegister(uint32_t addr, uint32_t val);
static int32_t ads1255_Command(uint32_t command);

static uint32_t ads1255_dataRate;

static uint32_t ads1255_ComputeDelay(uint32_t dataRate)
/*
 * Internal helper function for computing self-cal delay based on data rate
 * \param dataRate sample data rate
 * \returns value for using as argument to MAP_SysCtlDelay
 */
{
	// delay based on minimum 2MHz oscillator from table (p.25 ADS1255/ADS1256)
	switch (dataRate)
	{
	case ADS1255_RATE_30000SPS: return 99994;
	case ADS1255_RATE_15000SPS: return 84634;
	case ADS1255_RATE_7500SPS : return 115354;
	case ADS1255_RATE_3750SPS : return 135783;
	case ADS1255_RATE_2000SPS : return 215040;
	case ADS1255_RATE_1000SPS : return 368640;
	case ADS1255_RATE_500SPS  : return 691200;
	case ADS1255_RATE_100SPS  : return 3225600;
	case ADS1255_RATE_60SPS   : return 5222400;
	case ADS1255_RATE_50SPS   : return 6405120;
	case ADS1255_RATE_30SPS   : return 10414080;
	case ADS1255_RATE_25SPS   : return 12748800;
	case ADS1255_RATE_15SPS   : return 20782080;
	case ADS1255_RATE_10SPS   : return 31795200;
	case ADS1255_RATE_5SPS    : return 63544320;
	case ADS1255_RATE_2P5SPS  : return 127027200;
	}
	// nothing matched, return worst case
	return 127027200;
}

int32_t ads1255_EmptyReadBuffer()
/**
 * Gets received data from the receive FIFO of the specified SSI
 * module and places that data into the dummy variable.
 *
 * @return  PASS or FAIL
 *
 */
{
	uint32_t timeoutCounter0 = TIMEOUT_COUNTER;
	uint32_t dummyVal;
	while((MAP_SSIDataGetNonBlocking(SSI1_BASE, &dummyVal)) && (--timeoutCounter0));
	if ( timeoutCounter0 == 0 )
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_TIMEOUT);
		return FAIL;
	}
	else
	{
		return PASS;
	}
}

static int32_t ads1255_waitDRDYHtoL()
/*
 * The DRDY output line is used as a status signal to indicate
 * when a conversion has been completed. DRDY goes low
 * when new data is available. DRDY line is connected to TIVA
 * GPIO PORT P
 *
 * @param   None
 *
 * @return  1 = Timeout happens while waiting for DRDY to go low
 *          0 = No Error
 */
{
	uint32_t timeoutCounter0 = DRDY_TIMEOUT;
	uint32_t timeoutCounter1 = DRDY_TIMEOUT;
	while (!(DRDY_GPIO_PORT & DRDY_GPIO_MASK) && (--timeoutCounter0));
	while ( (DRDY_GPIO_PORT & DRDY_GPIO_MASK) && (--timeoutCounter1));
	if (!timeoutCounter0 || !timeoutCounter1)
	{
		return FAIL;
	}
	else
	{
		return PASS;
	}
}

int32_t ads1255_SelfTest()
/**
 * Tests various ADC modes(PowerDown, PowerUp, StandBy, WakeUp)
 * writing and reading registers over SSI.
 *
 * @return  PASS or FAIL
 */
{
	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_SSI1 );  //SSI peripheral
	uint32_t registerVal = 0;
	// put all registers to their default state
	if(ads1255_PowerUp() != PASS)
		return FAIL;
    if(ads1255_SetReadContinuous(false) != PASS)
    	return FAIL;
	if(ads1255_Reset() != PASS)
		return FAIL;
	// TEST POWERDOWN
	// wait 10 seconds to ensure it is powered down (2.5 sps x 20 with margin)
	// if DRDY toggles, there is an error
	if(ads1255_PowerDown() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_POWERDOWN);
		return FAIL;
	}
	MAP_SysCtlDelay(ads1255_ComputeDelay(ADS1255_RATE_30000SPS));
	if (ads1255_waitDRDYHtoL() == PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_POWERDOWN);
		return FAIL;
	}
	// TESET POWERUP
	// if DRDY does not toggle, there is an error
	if(ads1255_PowerUp() != PASS)
		return FAIL;
	if (ads1255_waitDRDYHtoL() != PASS)
		return FAIL;
	// TEST STANDBY   - Some times standby does not occur due to glitches on SCLK
	// if DRDY toggles, there is an error
	if(ads1255_Standby() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_STANDBY);
		return FAIL;
	}
	if (ads1255_waitDRDYHtoL() == PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_STANDBY);
		return FAIL;
	}

	// TESET WAKEUP
	// if DRDY does not toggle, there is an error
	if(ads1255_Wakeup() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_WAKEUP);
		return FAIL;
	}
	if (ads1255_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_WAKEUP);
		return FAIL;
	}
	// TEST READ REGISTER
	// check data rate vs the default value on power up
	if (ads1255_ReadRegister(REGISTER_DRATE, &registerVal) != PASS ||
			registerVal != ADS1255_RATE_30000SPS )
	{
		return FAIL;
	}
	// TEST WRITE REGISTER
	ads1255_WriteRegister(REGISTER_DRATE, ADS1255_RATE_15000SPS);
	if ( ads1255_ReadRegister(REGISTER_DRATE, &registerVal) != PASS ||
	        registerVal !=  ADS1255_RATE_15000SPS )
	{
		return FAIL;
	}
	// return to default
	if(ads1255_Reset() != PASS)
		return FAIL;
	if(ads1255_SetReadContinuous(true) != PASS)
		return FAIL;
	MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_SSI1 );  //SSI peripheral
	return PASS;
}

int32_t ads1255_PowerUp()
/**
 * This function handles the ADC power up sequence.
 * 1. SYNCZ is made high through GPIO Port H
 * 2. Wait 60MS for crystal startup
 * 3. Wait for self calibration to complete
 * 4. Do self clibration once more as recommended
 *
 * @return  PASS or FAIL
 */
{
	// set SYNCZ high
	SYNC_GPIO_PORT |= SYNC_GPIO_MASK;
	// wait for crystal startup (30ms typical, p.27 ADS1255/ADS1256)
	// 2x30ms for margin
	MAP_SysCtlDelay(DELAY_60MS);
	// wait for self calibration assuming minimum case 2MHz oscillator
	// and default PGA/data rate (p.25 ADS1255/ADS1256) with margin
	MAP_SysCtlDelay(DELAY_3MS);
	// do one more self calibration as recommended by ADS1255 p. 27
	if(PASS != ads1255_Command(COMMAND_SELFCAL))
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_POWERUP);
		return FAIL;
	}
	MAP_SysCtlDelay(DELAY_3MS);
	return PASS;
}

int32_t ads1255_PowerDown()
/**
 * This function handles the ADC power up sequence.
 * 1. SYNCZ is made low through GPIO Port H
 *
 * @return  PASS
 */
{
	// set SYNCZ pin low. after 20 DRDY cycles, powerdown will take effect
	// this function does not wait for the 20 cycles to allow fast abort by user
	// through issuing power up command.
	SYNC_GPIO_PORT &= (~SYNC_GPIO_MASK);
	return PASS;
}

int32_t ads1255_Wakeup()
/**
 * WakeUp command is issued with 50MS delay after the command.
 *
 * @return  PASS
 */
{
	uint32_t commandByte0 = COMMAND_WAKEUP;
	MAP_SSIDataPut( SSI1_BASE, commandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &commandByte0 );
	// t6 with 2x margin and 2MHz master clock
	// (p.6 ADS1255/ADS1256)
	MAP_SysCtlDelay(DELAY_50US);
	return PASS;
}

int32_t ads1255_Wakeup_NoDelay()
/**
 * WakeUp command is issued and returns without any delay after command.
 *
 * @return  PASS
 */
{
	uint32_t commandByte0 = COMMAND_WAKEUP;
	MAP_SSIDataPut( SSI1_BASE, commandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &commandByte0 );
	// t6 with 2x margin and 2MHz master clock
	// (p.6 ADS1255/ADS1256) do not issue other ADS commands until 50 usec later
    return PASS;
}

int32_t ads1255_Standby()
/*
 * StandBy command is issued here.
 *
 * @param   None
 *
 * @return  PASS or FAIL
 */
{
	return ads1255_Command(COMMAND_STANDBY);
}

int32_t ads1255_Reset()
/*
 * Reset command is issued with wait for self calibration to complete.
 *
 * @return  PASS or FAIL
 */
{
	uint32_t dummy;

	if (ads1255_Command(COMMAND_RESET) == PASS)
	{
		// error occured during command. try resetting again asynchronously with DRDY
		MAP_SSIDataPut( SSI1_BASE, COMMAND_RESET );
    	MAP_SSIDataGet( SSI1_BASE, &dummy );
		// t6 with 2x margin and 2MHz master clock
		// (p.6 ADS1255/ADS1256)
		MAP_SysCtlDelay(DELAY_50US);

	}
	else
	{
		return FAIL;
	}
	// wait for self calibration assuming minimum case 2MHz oscillator
	// and default PGA/data rate (p.25 ADS1255/ADS1256) with margin
	MAP_SysCtlDelay(DELAY_3MS);
	return PASS;
}

static int32_t ads1255_Command(uint32_t command)
/*
 * Commands are sent to ADC through SPI after DRDY goes low.
 *
 * @param   None
 *
 * @return  PASS or FAIL
 */
{
	uint32_t commandByte0 = command & 0xFF;

	if (ads1255_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_COMMAND);
		return FAIL;
	}
	// issue command
	MAP_SSIDataPut( SSI1_BASE, commandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &commandByte0 );
	// t6 with 2x margin and 2MHz master clock
	// (p.6 ADS1255/ADS1256)
	MAP_SysCtlDelay(DELAY_50US);

	return PASS;
}

static int32_t ads1255_TranslatePGAtoRegVal(uint32_t pgaVal)
/*
 * PGA values in the power of 2 are converted to ADC register values.
 *
 * @param   pgaVal -I- PGA values
 *
 * @return  ADC PGA register values
 */
{
	switch (pgaVal)
	{
		case 1:  return ADS1255_PGA1;
		case 2:  return ADS1255_PGA2;
		case 4:  return ADS1255_PGA4;
		case 8:  return ADS1255_PGA8;
		case 16: return ADS1255_PGA16;
		case 32: return ADS1255_PGA32;
		case 64: return ADS1255_PGA64;
		default: return FAIL;
	}
}

static int32_t ads1255_TranslateRegValtoPGA(uint32_t regVal)
/*
 * ADC register values are converted to PGA values in the power of 2.
 *
 * @param   regVal -I- ADC register value
 *
 * @return  PGA values
 */
{
	switch (regVal)
	{
		case ADS1255_PGA1:  return 1;
		case ADS1255_PGA2:  return 2;
		case ADS1255_PGA4:  return 4;
		case ADS1255_PGA8:  return 8;
		case ADS1255_PGA16: return 16;
		case ADS1255_PGA32: return 32;
		case ADS1255_PGA64: return 64;
		default: return FAIL;
	}
}

static uint32_t ads1255_TranslateDataRatetoRegVal(uint32_t dataRate)
/*
 * Data Rate is translated to ADC data rate regsiter values
 *
 * @param   dataRate -I- data rate values
 *
 * @return  Data rate ADC register values
 */
{
	switch (dataRate)
	{
		case 30000: return ADS1255_RATE_30000SPS;
		case 15000: return ADS1255_RATE_15000SPS;
		case 7500:  return ADS1255_RATE_7500SPS;
		case 3750:  return ADS1255_RATE_3750SPS;
		case 2000:  return ADS1255_RATE_2000SPS;
		case 1000:  return ADS1255_RATE_1000SPS;
		case 500:   return ADS1255_RATE_500SPS;
		case 100:   return ADS1255_RATE_100SPS;
		case 60:    return ADS1255_RATE_60SPS;
		case 50:    return ADS1255_RATE_50SPS;
		case 30:    return ADS1255_RATE_30SPS;
		case 25:    return ADS1255_RATE_25SPS;
		case 15:    return ADS1255_RATE_15SPS;
		case 10:    return ADS1255_RATE_10SPS;
		case 5:     return ADS1255_RATE_5SPS;
		case 2:     return ADS1255_RATE_2P5SPS;
		default:    return 0;
	}
}

static uint32_t ads1255_TranslateRegValtoDataRate(uint32_t regVal)
/*
 * ADC data rate regsiter value is translated to actualis translated to actual data rate
 *
 * @param   regVal -I- data rate register value
 *
 * @return  Data rate
 */
{
	switch (regVal)
	{
		case ADS1255_RATE_30000SPS: return 30000;
		case ADS1255_RATE_15000SPS: return 15000;
		case ADS1255_RATE_7500SPS:  return 7500;
		case ADS1255_RATE_3750SPS:  return 3750;
		case ADS1255_RATE_2000SPS:  return 2000;
		case ADS1255_RATE_1000SPS:  return 1000;
		case ADS1255_RATE_500SPS:   return 500;
		case ADS1255_RATE_100SPS:   return 100;
		case ADS1255_RATE_60SPS:    return 60;
		case ADS1255_RATE_50SPS:    return 50;
		case ADS1255_RATE_30SPS:    return 30;
		case ADS1255_RATE_25SPS:    return 25;
		case ADS1255_RATE_15SPS:    return 15;
		case ADS1255_RATE_10SPS:    return 10;
		case ADS1255_RATE_5SPS:     return 5;
		case ADS1255_RATE_2P5SPS:     return 2;
		default:    return 0;
	}
}

int32_t ads1255_SetPGAGain(uint8_t pgaVal)
/**
 * Writes PGA gain to ADC register.
 *
 * @param   pgaVal -I- pga values to be written to PGA register
 *
 * @return  PASS or FAIL
 */
{
	if (ads1255_WriteRegister(REGISTER_ADCON, ads1255_TranslatePGAtoRegVal(pgaVal)) != PASS)
	{
		return FAIL;
	}
	else
	{
		return PASS;
	}
}

int8_t ads1255_GetPGAGain(void)
/*
 * Retrieves PGA gain from ADC register.
 *
 * @return  pga value currently set in the ADC register.
 */
{
	uint32_t regVal;

	if(ads1255_ReadRegister(REGISTER_ADCON, &regVal) != PASS)
		return FAIL;

	return ads1255_TranslateRegValtoPGA(regVal);
}

int32_t ads1255_Configure(uint32_t iBufferEnable, uint32_t pgaVal, uint32_t dataRate)
{
	if (ads1255_SetAnalogInputBuffer(iBufferEnable) != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_CONFIGURE);
		return FAIL;
	}
	if (ads1255_WriteRegister(REGISTER_ADCON, ads1255_TranslatePGAtoRegVal(pgaVal)) != PASS)
	{
		return FAIL;
	}
	if (ads1255_WriteRegister(REGISTER_DRATE, ads1255_TranslateDataRatetoRegVal(dataRate)) != PASS)
	{
		return FAIL;
	}
	ads1255_dataRate = dataRate;
	if (ads1255_Command(COMMAND_SELFCAL) != PASS)
	{
		return FAIL;
	}
	MAP_SysCtlDelay(ads1255_ComputeDelay(dataRate));
	return PASS;
}

uint32_t ads1255_GetDataRate(void)
{
	return ads1255_dataRate;
}

int32_t ads1255_GetSettlingTime(void)
/* Returns the settling time (time required for a step change
 * on the analog inputs to propagate through the filter) in microseconds
 * for the data rate programmed to the ADC at this time.
 */
{
	int32_t dataRate = ads1255_GetDataRate();

	if(dataRate > 0)
	{
		switch (dataRate)
		{
			case 30000: return 210;
			case 15000: return 250;
			case 7500:  return 310;
			case 3750:  return 440;
			case 2000:  return 680;
			case 1000:  return 1180;
			case 500:   return 2180;
			case 100:   return 10180;
			case 60:    return 16840;
			case 50:    return 20180;
			case 30:    return 33510;
			case 25:    return 40180;
			case 15:    return 66840;
			case 10:    return 100180;
			case 5:     return 200180;
			case 2:     return 400180;
			default:    return FAIL;
		}
	}

	return FAIL;

}

int32_t ads1255_SetAnalogInputBuffer(uint32_t enableState)
{
	uint32_t statusRegisterVal;
	if (ads1255_ReadRegister(REGISTER_STATUS, &statusRegisterVal) != PASS)
	{
		return FAIL;
	}
	statusRegisterVal &= (~MASK_ANALOG_IBUFFER);
	if (enableState)
	{
		statusRegisterVal |= MASK_ANALOG_IBUFFER;
	}
	if (ads1255_WriteRegister(REGISTER_STATUS, statusRegisterVal) != PASS)
	{
		return FAIL;
	}
	return PASS;
}

int32_t ads1255_SetReadContinuous(bool enableState)
/**
 * This mode enables the continuous output of new data on each DRDY
 * without the need to issue subsequent read commands. After all 24 bits
 * have been read, DRDY goes high
 *
 * @param   enableState -I- Enables Read Continous mode
 *
 * @return  PASS or FAIL
 */
{
	if (enableState)
	{
		return (ads1255_Command(COMMAND_RDATAC));
	}
	else
	{
	    return (ads1255_Command(COMMAND_SDATAC));
	}
}

static int32_t ads1255_ReadRegister(uint32_t addr, uint32_t *pRegVal)
/*
 * Reads ADC register values using RREG command.
 * RREG command output the data from up to 11 registers
 * starting with the register address specified as part of the command.
 * Note that register reads are unreliable when ADC is set to continuous read mode.
 *
 * @param   addr    -I- starting register address for RREG command
 *          pRegVal -O- Register value retrieved
 *
 * @return  PASS
 *          FAIL
 */
{
	uint32_t readCommandByte0 = (COMMAND_RREG | (addr & 0xF));
	uint32_t readCommandByte1 = 0x00; // read only 1 register
	uint32_t registerVal;
	if(ads1255_EmptyReadBuffer() != PASS)
		return FAIL;
	// wait for DRDY to transition from high to low
	if (ads1255_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_READREGISTER);
		return FAIL;
	}
	//
	// Set the SSI module into write-only mode.
	//
	GPIOPinTypeGPIOOutput( GPIO_PORTB_BASE, GPIO_PIN_4 );  // Set CSZ as GPIO to work around CZ going high during SysCtlDelay
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_4, 0 );
    MAP_SSIDataPut( SSI1_BASE, readCommandByte0 );
	MAP_SSIDataPut( SSI1_BASE, readCommandByte1 );
    MAP_SSIDataGet( SSI1_BASE, &registerVal );
    MAP_SSIDataGet( SSI1_BASE, &registerVal );
	// t6 with 2x margin and 2MHz master clock
	// (p.6 ADS1255/ADS1256)
    //
    // Set the SSI module into read/write mode.  In this mode, dummy writes are
    // required in order to make the transfer occur; the SPI flash will ignore
    // the data.
    //
	// read back the returned value
    MAP_SysCtlDelay( DELAY_6_25US );	
	MAP_SSIDataPut( SSI1_BASE, COMMAND_WAKEUP );
    MAP_SSIDataGet( SSI1_BASE, &registerVal );
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4 );
    GPIOPinConfigure( GPIO_PB4_SSI1FSS );   //CSZ
    GPIOPinTypeSSI( GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5 ); // Return CSZ to be driven by SSI1
    *pRegVal = registerVal & 0xFF;
	return PASS;
}

static int32_t ads1255_WriteRegister(uint32_t addr, uint32_t val)
/*
 * Writes ADC register values using WREG command.
 * WREG command Write to the registers starting with the register
 * specified as part of the command.
 *
 * @param   addr -I- starting register address for WREG command
 *          val  -I- Register value to be written
 *
 * @return  1 = Timeout Error
 *          0 = No Error
 */
{
	uint32_t writeCommandByte0 = (COMMAND_WREG | (addr & 0xF));
	uint32_t writeCommandByte1 = 0x00; // read only 1 register
	// wait for DRDY to transition from high to low
	if (ads1255_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_WRITEREGISTER);
		return FAIL;
	}
	// issue the write command
	MAP_SSIDataPut( SSI1_BASE, writeCommandByte0 );
	MAP_SSIDataPut( SSI1_BASE, writeCommandByte1 );
	MAP_SSIDataPut( SSI1_BASE, val & 0xFF );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
	return PASS;
}

int32_t ads1255_GetSample()
/**
 * This function is used to read latched ADC data
 * in trigger DRDY interrupt
 *
 * @return  ADC Data
 */
{
	uint32_t data[NUM_SSI_DATA];
	int32_t ret_word;

 	MAP_SSIDataPut( SSI1_BASE, COMMAND_WAKEUP );
    MAP_SSIDataPut( SSI1_BASE, COMMAND_WAKEUP );
	MAP_SSIDataPut( SSI1_BASE, COMMAND_WAKEUP );
	MAP_SSIDataGet( SSI1_BASE, data ); // read in latched data
    MAP_SSIDataGet( SSI1_BASE, data+1 ); // read in latched data
    MAP_SSIDataGet( SSI1_BASE, data+2 ); // read in latched data
    ret_word = ((data[0]&0xFF)<<16) + ((data[1]&0xFF)<<8) + (data[2]&0xFF);
	//sign extend the 24-bit 2's complement values
    ret_word <<= 8;
    ret_word >>= 8;

	return ret_word;
}

int32_t ads1255_init()
/**
 * Initialization done by setting clock, operating mode.
 * Power up is done which does self calibration at power up.
 *
 * @return  PASS or FAIL
 */
{
	uint32_t regVal;

	MAP_SSIAdvModeSet( SSI1_BASE, SSI_ADV_MODE_LEGACY );
	SSIConfigSetExpClk( //set clock, operating mode
					   SSI1_BASE,
					   NIRSCAN_SYSCLK,
			           SSI_FRF_MOTO_MODE_1,
			           SSI_MODE_MASTER, 1800000,
			           8
			           );
	MAP_SSIEnable( SSI1_BASE );
	if(ads1255_PowerUp() != PASS)
		return FAIL;
	// reset will trigger a new self calibration and ensure a known starting point
	if(ads1255_Reset() != PASS)
		return FAIL;
	if(ads1255_SetPGAGain( 1 ) != PASS)
		return FAIL;

	if (ads1255_SetAnalogInputBuffer(TRUE) != PASS)
		return FAIL;
	
	if(ads1255_ReadRegister(REGISTER_DRATE, &regVal) == PASS)
		ads1255_dataRate =  ads1255_TranslateRegValtoDataRate(regVal);
	else
		return FAIL;
    if(ads1255_SetReadContinuous(true) != PASS)
    	return FAIL;

    return PASS;

}

