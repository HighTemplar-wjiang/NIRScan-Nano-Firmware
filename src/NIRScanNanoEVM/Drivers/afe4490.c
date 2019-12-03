/*
 * adcWrapper.c
 * This has the wrapper functions around afe4490 ADC functions
 *
 *  Created on: May 26, 2016
 *      Author: a0393679
 */

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

#include "afe4490.h"
#include "common.h"
#include "nnoStatus.h"
#include "driverlib/gpio.h"
#include "NIRscanNano.h"

#define ADC_afe4490

uint32_t AFE44xx_Current_Register_Settings[35] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int32_t afe4490_EmptyReadBuffer()
/**
 * This is a wrapper function around afe4490 EmptyReadBuffer
 *
 * @return  PASS or FAIL
 *
 */
{
	return PASS;
}

int32_t afe4490_SelfTest()
/**
 * This is a wrapper function around afe4490 Selfttest function
 *
 * @return  PASS or FAIL
 */
{
	return PASS;
}

int32_t afe4490_WakeupNoDelay()
/**
 * This is a wrapper function around afe4490 WakeupNoDelay()
 * WakeUp command is issued and returns without any delay after command.
 *
 * @return  PASS
 */
{
	return PASS;
}

int32_t afe4490_Standby()
/*
 * This is a wrapper function around afe4490 Standby
 * StandBy command is issued here.
 *
 * @param   None
 *
 * @return  PASS or FAIL
 */
{
	return PASS;
}


int32_t afe4490_SetReadContinuous(bool enableState)
/**
 *
 * This is a wrapper function around afe4490 SetReadContinuous
 *
 * @param   enableState -I- Enables Read Continous mode
 *
 * @return  PASS or FAIL
 */
{
	return PASS;
}

int32_t afe4490_SetPGAGain(uint8_t pgaVal)
/**
 * This is a wrapper function around afe4490 SetPGAGain
 * Writes PGA gain to ADC register.
 *
 * @param   pgaVal -I- pga values to be written to PGA register
 *
 * @return  PASS or FAIL
 */
{
	return PASS;
}

int32_t afe4490_init()
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
			           SSI_MODE_MASTER, 4000000,			/* 8 MHz max */
			           8
			           );
	MAP_SSIEnable( SSI1_BASE );
	if(afe4490_PowerUp() != PASS)
		return FAIL;
	// reset will trigger a new self calibration and ensure a known starting point
	if(afe4490_Reset() != PASS)
		return FAIL;
	if(afe4490_SetPGAGain( 1 ) != PASS)
		return FAIL;

  afe4490_ClearSPIRead();
  
  afe4490_WriteRegister((unsigned char)PRPCOUNT, (unsigned long)PRP);
  afe4490_WriteRegister((unsigned char)LED2STC, (unsigned long)LED2STC_VAL);
  afe4490_WriteRegister((unsigned char)LED2ENDC, (unsigned long)LED2ENDC_VAL);
  afe4490_WriteRegister((unsigned char)LED2LEDSTC, (unsigned long)LED2LEDSTC_VAL);
  afe4490_WriteRegister((unsigned char)LED2LEDENDC, (unsigned long)LED2LEDENDC_VAL);
  afe4490_WriteRegister((unsigned char)ALED2STC, (unsigned long)ALED2STC_VAL);
  afe4490_WriteRegister((unsigned char)ALED2ENDC, (unsigned long)ALED2ENDC_VAL);
  afe4490_WriteRegister((unsigned char)LED1STC, (unsigned long)LED1STC_VAL);
  afe4490_WriteRegister((unsigned char)LED1ENDC, (unsigned long)LED1ENDC_VAL);
  afe4490_WriteRegister((unsigned char)LED1LEDSTC, (unsigned long)LED1LEDSTC_VAL);
  afe4490_WriteRegister((unsigned char)LED1LEDENDC, (unsigned long)LED1LEDENDC_VAL);
  afe4490_WriteRegister((unsigned char)ALED1STC, (unsigned long)ALED1STC_VAL);
  afe4490_WriteRegister((unsigned char)ALED1ENDC, (unsigned long)ALED1ENDC_VAL);
  afe4490_WriteRegister((unsigned char)LED2CONVST, (unsigned long)LED2CONVST_VAL);
  afe4490_WriteRegister((unsigned char)LED2CONVEND, (unsigned long)LED2CONVEND_VAL);
  afe4490_WriteRegister((unsigned char)ALED2CONVST, (unsigned long)ALED2CONVST_VAL);
  afe4490_WriteRegister((unsigned char)ALED2CONVEND, (unsigned long)ALED2CONVEND_VAL);
  afe4490_WriteRegister((unsigned char)LED1CONVST, (unsigned long)LED1CONVST_VAL);
  afe4490_WriteRegister((unsigned char)LED1CONVEND, (unsigned long)LED1CONVEND_VAL);
  afe4490_WriteRegister((unsigned char)ALED1CONVST, (unsigned long)ALED1CONVST_VAL);
  afe4490_WriteRegister((unsigned char)ALED1CONVEND, (unsigned long)ALED1CONVEND_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTSTCT0, (unsigned long)ADCRSTSTCT0_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTENDCT0, (unsigned long)ADCRSTENDCT0_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTSTCT1, (unsigned long)ADCRSTSTCT1_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTENDCT1, (unsigned long)ADCRSTENDCT1_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTSTCT2, (unsigned long)ADCRSTSTCT2_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTENDCT2, (unsigned long)ADCRSTENDCT2_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTSTCT3, (unsigned long)ADCRSTSTCT3_VAL);
  afe4490_WriteRegister((unsigned char)ADCRSTENDCT3, (unsigned long)ADCRSTENDCT3_VAL);
  
  afe4490_WriteRegister((unsigned char)CONTROL0, AFE44xx_Current_Register_Settings[0]);            //0x00
  afe4490_WriteRegister((unsigned char)CONTROL2, AFE44xx_Current_Register_Settings[35]);           //0x23
  afe4490_WriteRegister((unsigned char)TIAGAIN, AFE44xx_Current_Register_Settings[32]);            //0x20
  afe4490_WriteRegister((unsigned char)TIA_AMB_GAIN, AFE44xx_Current_Register_Settings[33]);       //0x21
  afe4490_WriteRegister((unsigned char)LEDCNTRL, AFE44xx_Current_Register_Settings[34]);           //0x22
  afe4490_WriteRegister((unsigned char)CONTROL1, AFE44xx_Current_Register_Settings[30]);           //0x1E
  
  afe4490_SetSPIRead();
    return PASS;

}

int32_t afe4490_PowerUp()
/**
 * This is a wrapper function around afe4490 PowerUP
 * This function handles the ADC power up sequence.
 *
 * @return  PASS or FAIL
 */
{
	return PASS;
}

int32_t afe4490_PowerDown()
/**
 * This is a wrapper function around afe4490 PowerDown
 * This function handles the ADC power down sequence.
 * *
 * @return  PASS
 */
{
	return PASS;
}

int32_t afe4490_Wakeup()
/**
 * This is a wrapper function around afe4490 WakeUp
 * @return  PASS
 */
{
	return PASS;
}

int32_t afe4490_Wakeup_NoDelay()
/**
 * This is a wrapper function around afe4490 NoDelay
 * WakeUp command is issued and returns without any delay after command.
 *
 * @return  PASS
 */
{
	return PASS;
}

int32_t afe4490_GetSample()
/**
 * This is a wrapper function around afe4490 GetSample
 *
 *
 * @return  ADC Data
 */
{
	return PASS;

}

int8_t afe4490_GetPGAGain(void)
/*
 * This is a wrapper function around afe4490 GetPGAGain
 *
 * @return  pga value currently set in the ADC register.
 */
{
	return PASS;
}

static int32_t afe4490_ReadRegister(uint32_t addr, uint32_t *pRegVal)
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
	uint32_t readCommandByte0 = (addr & 0x3F);
	uint32_t registerVal[3];
	if(afe4490_EmptyReadBuffer() != PASS)
		return FAIL;
	// wait for DRDY to transition from high to low
	if (afe4490_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_READREGISTER);
		return FAIL;
	}
	//
	// Set the SSI module into write-only mode.
	//
	GPIOPinTypeGPIOOutput( GPIO_PORTB_BASE, GPIO_PIN_4 );  // Set CSZ as GPIO to work around CZ going high during SysCtlDelay
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_4, 0 );
    // place one byte with register address
    MAP_SSIDataPut( SSI1_BASE, readCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &registerVal );
	// read back the returned 24-bit value
    MAP_SSIDataGet( SSI1_BASE, &registerVal[2] );
    MAP_SSIDataGet( SSI1_BASE, &registerVal[1] );
    MAP_SSIDataGet( SSI1_BASE, &registerVal[0] );
    GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4 );
    GPIOPinConfigure( GPIO_PB4_SSI1FSS );   //CSZ
    GPIOPinTypeSSI( GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5 ); // Return CSZ to be driven by SSI1
    *pRegVal = (registerVal[2]<<16 | registerVal[1]<<8 | registerVal[0]) && 0x00FFFFFF;
	return PASS;
}

static int32_t afe4490_WriteRegister(uint32_t addr, uint32_t val)
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
	uint32_t writeCommandByte0 = (addr & 0x3F);
	// wait for DRDY to transition from high to low
	if (afe4490_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_WRITEREGISTER);
		return FAIL;
	}
	// issue the write command
	MAP_SSIDataPut( SSI1_BASE, writeCommandByte0 );
	MAP_SSIDataPut( SSI1_BASE, (val>>16) & 0xFF );
	MAP_SSIDataPut( SSI1_BASE, (val>>8) & 0xFF );
	MAP_SSIDataPut( SSI1_BASE, val & 0xFF );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
	return PASS;
}

static int32_t afe4490_ClearSPIRead()
/*
 * Writes ADC Control 0 register and clears SPI_READ flag. SPI_READ must be cleared before writing a register
 *
 * @return  1 = Timeout Error
 *          0 = No Error
 */
{
	uint32_t writeCommandByte0 = 0;
	uint32_t val = 0;
	// wait for DRDY to transition from high to low
	if (afe4490_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_WRITEREGISTER);
		return FAIL;
	}
	// issue the write command
	MAP_SSIDataPut( SSI1_BASE, writeCommandByte0 );
	MAP_SSIDataPut( SSI1_BASE, 0 );
	MAP_SSIDataPut( SSI1_BASE, 0 );
	MAP_SSIDataPut( SSI1_BASE, val & 0xFF );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
	return PASS;
}

static int32_t afe4490_SetSPIRead()
/*
 * Writes ADC Control 0 register and sets DSPI_READ flag. SPI_READ must be set before reading a register
 *
 * @return  1 = Timeout Error
 *          0 = No Error
 */
{
	uint32_t writeCommandByte0 = 1;
	uint32_t val = 0;
	// wait for DRDY to transition from high to low
	if (afe4490_waitDRDYHtoL() != PASS)
	{
        nnoStatus_setErrorStatusAndCode(NNO_ERROR_ADC, true, ADC_ERROR_WRITEREGISTER);
		return FAIL;
	}
	// issue the write command
	MAP_SSIDataPut( SSI1_BASE, writeCommandByte0 );
	MAP_SSIDataPut( SSI1_BASE, 0 );
	MAP_SSIDataPut( SSI1_BASE, 0 );
	MAP_SSIDataPut( SSI1_BASE, val & 0xFF );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
    MAP_SSIDataGet( SSI1_BASE, &writeCommandByte0 );
	return PASS;
}

static int32_t afe4490_waitDRDYHtoL()
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

		return PASS;

}

int32_t afe4490_Reset()
{

	return PASS;
}
