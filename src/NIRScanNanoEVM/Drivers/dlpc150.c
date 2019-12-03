/*
 *
 * APIs to configure and communicate with dlpc150 over I2C bus
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com
 * ALL RIGHTS RESERVED
 *
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "inc/tm4c129xnczad.h"
#include <xdc/runtime/System.h>
/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

#include "Board.h"
#include "NIRscanNano.h"
#include "common.h"
#include "nnoStatus.h"
#include "dlpc150.h"

/**
 *
 *  Initializes I2C2 for DLPC150
 *
 *  Initializes DLPC150 using I2C2 at 100KHz.
 *  Reads Manufacturer ID and Device ID to check if initialization was successful
 *
 *  @param ptnSrc 0 sets up dlpc150 to use splash as the source for patterns and 1 sets up dlpc150 to use patterns from external video input
 *
 *  \return 0 for success, 1 for failure
 */
static I2C_Handle dlpc150_open(void)
{
	I2C_Handle      i2c;
	I2C_Params      i2cParams;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);
	/* Create I2C for usage */
	I2C_Params_init( &i2cParams );
	i2cParams.bitRate = I2C_100kHz;
	i2c = I2C_open( BOARD_I2C_DLPC, &i2cParams );

	if(i2c == NULL)
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);

	return i2c;
}

static int16_t dlpc150_close(I2C_Handle i2c)
{
	if(i2c != NULL)
	{
		I2C_close(i2c);
		MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C2);
		return PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		return FAIL;
	}
}

#if 0
static int16_t dlpc150_SetRGBPatternMode(bool twenty_four_bpp)
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_PATTERN_STREAMING , 0 };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	if(twenty_four_bpp == true)
		cmds[1] = 1;

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}
#endif

static int16_t dlpc150_SetHWLockMode(void)
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_HWLOCK_MODE , 0 };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

static int16_t dlpc150_SetFlashPatternMode(void)
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_FLASH_PATTERN};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}


static int16_t dlpc150_inputSourceSelect( uint8_t source )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_SYS_W_IN_SOURCE_SEL, source};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	cmds[1] = source;
	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

static int16_t dlpc150_imageCrop( void )
{

	/*
	 * Review comment - SK
	 * Is there any reason why dlpc150_open() & dlpc150_close() called in everyfunction?
	 * If BOARD_I2C_DLPC	  NIRscanNano_I2C2 *I2C2* dedicated for DLPC150 communication we don't need it;
	 * this is unecessary call every time.
	 * Please remove and provide the reason in the code.
	 * Same comment applicable every where
	 */


	I2C_Transaction   i2cTransaction;
	uint8_t cmds[9] = {DLPC150_SYS_W_IMAGE_CROP, 0x00, 0x00, 0x00, 0x00, DMD_WIDTH_LSB, DMD_WIDTH_MSB, DMD_HEIGHT_LSB, DMD_HEIGHT_MSB };
	// LSB,MSB  Start Pixel,  Start Line, Pixel/Line, Lines/Frame
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 9;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

static int16_t dlpc150_SetDefaultDisplaySize( void )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[5] = {DLPC150_SYS_W_DISPLAY_SIZE, DMD_WIDTH_LSB, DMD_WIDTH_MSB, DMD_HEIGHT_LSB, DMD_HEIGHT_MSB };
	// LSB,MSB  Pixel/Line, Lines/Frame
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 5;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}


static int16_t dlpc150_SetDefaultInputImageSize( void )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[5] = {DLPC150_SYS_W_EXT_IM_SIZE, DISP_WIDTH_LSB, DISP_WIDTH_MSB, DMD_HEIGHT_LSB, DMD_HEIGHT_MSB };
	// LSB,MSB  Pixel/Line, Lines/Frame
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 5;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

#ifdef SPLASH_VARIABLE_EXP

static int16_t dlpc150_splashExecute( )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_SYS_W_SPLASH_EXEC};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

int16_t dlpc150_splashSelect( uint8_t splash )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_SYS_W_SPLASH_SEL, 0};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	cmds[1] = splash;
	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

static int16_t dlpc150_SetSeqVector( uint16_t vector_num, uint8_t num_vectors )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[9] = {DLPC150_SYS_SEQ_COMMAND, 0x14, 0x22, 0x00, 0x40, vector_num, vector_num>>8, num_vectors, 0x00 };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 9;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

#endif


#ifdef TPG
int16_t dlpc150_testPatternSelect( uint8_t tpg_num )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_SYS_W_TEST_SEL, 0};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	cmds[1] = tpg_num;
	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}
#endif
int16_t dlpc150_EnableSequencer( bool enable )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[9] = {DLPC150_SYS_SEQ_COMMAND, 0x00, 0x22, 0x00, 0x40, 0x20, 0x10, 0x00, 0x00 };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	if(enable)
		cmds[5] = 0x21;
	else
		cmds[5] = 0x20;

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 9;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

/**
 * Sets up DLPC150 for desired input source
 *
 *  @param patterns_from_rgb_port 	true - sets up DLPC150 to display patterns from RGB port
 *  								false - sets up DLPC150 to display patterns from flash
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_SetUpSource(bool patterns_from_rgb_port)
{

#ifdef SPLASH_VARIABLE_EXP
	dlpc150_inputSourceSelect( INPUT_SOURCE_SPLASH );

	if ( dlpc150_splashSelect( 0 ) )
		return FAIL;

	if ( dlpc150_splashExecute( ) )
		return FAIL;

	if ( dlpc150_EnableSequencer( false ) )
		return FAIL;

	if ( dlpc150_SetSeqVector( ) )
		return FAIL;

	if ( dlpc150_EnableSequencer( true ) )
		return FAIL;
	if ( dlpc150_inputSourceSelect( INPUT_SOURCE_RGB ) )
		return FAIL;
#endif
#ifdef TPG
	if ( dlpc150_inputSourceSelect( INPUT_SOURCE_TEST ) )
		return FAIL;
	if ( dlpc150_testPatternSelect( 0x8 ) )
		return FAIL;
#endif

	if(patterns_from_rgb_port == false)
	{
		if( PASS != dlpc150_inputSourceSelect( INPUT_SOURCE_SPLASH ))
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}

		if(dlpc150_SetFlashPatternMode() != PASS)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
	}
	else
	{
		if(PASS != dlpc150_imageCrop())
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
		if ( dlpc150_SetDefaultDisplaySize() != PASS)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
		if ( dlpc150_SetDefaultInputImageSize() != PASS)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
		if ( dlpc150_inputSourceSelect( INPUT_SOURCE_RGB ) )
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
#ifdef SIXTEEN_BPP
		if(dlpc150_SetRGBPatternMode(false) != PASS) //16 bit patterns
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
#else
		if(dlpc150_SetHWLockMode() != PASS) //24 bit patterns
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
			return FAIL;
		}
#endif
	}
	
	if(PASS != dlpc150_WriteReg(0x400053d0, 0xD0))  // Disable dithering on patterns
		return FAIL;

	return PASS;

}

static int16_t dlpc150_SetLampCurrent( uint16_t RedCurrent, uint16_t GreenCurrent, uint16_t BlueCurrent )
{
	I2C_Transaction   i2cTransaction;
	uint8_t lampCurrent[7] = {DLPC150_LED_W_CURRENT, 0, 0, 0, 0, 0, 0};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = lampCurrent;
	i2cTransaction.writeCount = 7;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	lampCurrent[1] = RedCurrent & 0x00FF;
	lampCurrent[2] = (RedCurrent >> 8) & 0x00FF;
	lampCurrent[3] = GreenCurrent & 0x00FF;
	lampCurrent[4] = (GreenCurrent >> 8) & 0x00FF;
	lampCurrent[5] = BlueCurrent & 0x00FF;
	lampCurrent[6] = (BlueCurrent >> 8) & 0x00FF;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

#ifdef FOUR_LAMPS
static int16_t dlpc150_SetMaxCurrent( uint16_t RedCurrent, uint16_t GreenCurrent, uint16_t BlueCurrent )
{
	I2C_Transaction   i2cTransaction;
	uint8_t lampCurrent[7] = {DLPC150_LED_W_MAX_CURRENT, 0, 0, 0, 0, 0, 0};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = lampCurrent;
	i2cTransaction.writeCount = 7;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	lampCurrent[1] = RedCurrent & 0x00FF;
	lampCurrent[2] = (RedCurrent >> 8) & 0x00FF;
	lampCurrent[3] = GreenCurrent & 0x00FF;
	lampCurrent[4] = (GreenCurrent >> 8) & 0x00FF;
	lampCurrent[5] = BlueCurrent & 0x00FF;
	lampCurrent[6] = (BlueCurrent >> 8) & 0x00FF;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}
#endif
/**
 * Due to some issues found with DLPA2005 driver, it is recommended to call this
 * API only with enable = FALSE while using NIRscan Nano EVM.
 *
 *  @param enable 	true - sets DLPA2005 LED driver off
 *  				false - sets DLPA2005 LED driver ON
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_LampEnable( bool enable )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_LED_W_ENABLE, 0};
	int16_t ret_val;
	I2C_Handle i2c;

	if(enable)
	{
#ifdef FOUR_LAMPS
		if(dlpc150_SetMaxCurrent( 0x0, 400, 0x0 ) != PASS)
			return FAIL;
		if(dlpc150_SetLampCurrent( 0x0, 377, 0x0 ) != PASS)
			return FAIL;
#else
		if(dlpc150_SetLampCurrent( 0x0, 189, 0x0 ) != PASS)
			return FAIL;
#endif
		cmds[1] = LED_GREEN_ENABLE;
	}
	else
	{
		if(dlpc150_SetLampCurrent( 0x0, 0x0, 0x0 ) != PASS)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_LAMP_DRIVER_ERROR);
			return FAIL;
		}
	}

	i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		ret_val =  FAIL;
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_LAMP_DRIVER_ERROR);
	}

	dlpc150_close(i2c);
	return ret_val;

}

/**
 * This API is to be used to crop off selected lines from the top or bottom
 * of a pattern from being displyed on the DMD
 *
 *  @param startY 	No lines cropped if 0 else startY number of lines from top will be cropped off
 *  @param height 	No lines cropped if height=DMD_HEIGHT; else that many lines cropped from bottom
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_displayCrop( uint16_t startY, uint16_t height )
{
	uint32_t reg_val;
	if(PASS != dlpc150_ReadReg(0x40001300, &reg_val))
		return FAIL;
	reg_val &= ~1; //Clear bit 0 to set to border insertion mode for pleasing color
	if(PASS != dlpc150_WriteReg(0x40001300, reg_val))
		return FAIL;
	//First display line position
	if(PASS != dlpc150_WriteReg(0x4000107c, startY))
		return FAIL;
	//Total lines to display
	if(PASS != dlpc150_WriteReg(0x40001074, height))
		return FAIL;

	return PASS;
}

/**
 * This API sets specified DLPC150 register to the specified value.
 * This is a TI Internal debug only API. DLPC150 register details will not be shared with
 * customers. Use this API with caution to avoid undesirable system behavior.
 *
 *  @param addr 	register address
 *  @param value 	register value
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_WriteReg( uint32_t addr, uint32_t value )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[9] = {DLPC150_REG_WRITE, addr, addr>>8, addr>>16, addr>>24, value, value>>8, value>>16, value>>24 };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 9;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

/**
 * This API reads the value of specified DLPC150 register.
 * This is a TI Internal debug only API. DLPC150 register details will not be shared with
 * customers.
 *
 *  @param addr 	-I- register address
 *  @param pValue 	-O - pointer to store register value
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_ReadReg( uint32_t addr, uint32_t *pValue )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[5] = {DLPC150_REG_READ, addr, addr>>8, addr>>16, addr>>24};
	uint8_t val[4];
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 5;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 4;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*pValue = (uint32_t)val[3] << 24;
	*pValue |= ((uint32_t)val[2] << 16);
	*pValue |= ((uint32_t)val[1] << 8);
	*pValue |= (uint32_t)val[0];

	dlpc150_close(i2c);
	return ret_val;
}

/**
 * Returns DLPC150 software version in the pointer passed.
 *
 *  @param pVer	-O - pointer to pass software revision number
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_GetSoftwareVersion( uint32_t *pVer )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_ADMIN_SYS_SW};
	uint8_t val[4] = { 0xff, 0xff, 0xff, 0xff };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 4;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*pVer = (uint32_t)val[3] << 24;
	*pVer |= ((uint32_t)val[2] << 16);
	*pVer |= ((uint32_t)val[1] << 8);
	*pVer |= (uint32_t)val[0];

	dlpc150_close(i2c);
	return ret_val;
}

/**
 * Returns DLPC150 flash build version in the pointer passed.
 *
 *  @param pVer	-O - pointer to pass flash build revision number
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_GetFlashBuildVersion( uint32_t *pVer )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_ADMIN_FLASH_VER};
	uint8_t val[4];
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 4;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*pVer = (uint32_t)val[3] << 24;
	*pVer |= ((uint32_t)val[2] << 16);
	*pVer |= ((uint32_t)val[1] << 8);
	*pVer |= (uint32_t)val[0];

	dlpc150_close(i2c);
	return ret_val;
}

/**
 *  Reads DLPC150 Lamp Photodetector value. This is a normalized value
 *  of 0-1.8V to 0-4095.The actual voltage can be calculated as
 *  actual volatge = (GSensor/4095)*1.8
 *
 *  @param RSensor	-O- Do not use. Reserved for future use.
 *  @param GSensor	-O- Lamp photodetector value
 *  @param BSensor	-O- Do not use. Reserved for future use.
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_GetLightSensorData( uint32_t *RSensor, uint32_t *GSensor, uint32_t *BSensor )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_ADMIN_LIGHT_SENSOR};
	uint8_t val[6];
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 6;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*RSensor = (uint32_t)val[5] << 8;
	*RSensor |= (uint32_t)val[4];
	*GSensor = (uint32_t)val[3] << 8;
	*GSensor |= (uint32_t)val[2];
	*BSensor = (uint32_t)val[1] << 8;
	*BSensor |= (uint32_t)val[0];

	dlpc150_close(i2c);
	return ret_val;
}

static int16_t dlpc150_GetShortStatus( uint32_t *pStatus )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_ADMIN_SHORT_STATUS};
	uint8_t val[4];
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 4;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*pStatus = (uint32_t)val[3] << 24;
	*pStatus |= ((uint32_t)val[2] << 16);
	*pStatus |= ((uint32_t)val[1] << 8);
	*pStatus |= (uint32_t)val[0];

	dlpc150_close(i2c);
	return ret_val;
}

//*****************************************************************************
//
// Called by the NVIC as a result of GPIOQ7 interrupt event. For this
// application GPIO PQ7 goes high when the DLPC150 reports an error
//
//*****************************************************************************
void GPIOQ7IntHandler(void)
{
	uint32_t ui32Status, shortStatus;

	ui32Status = GPIOIntStatus(GPIO_PORTQ_BASE, true);

	// Clear all the pin interrupts that are set
	MAP_GPIOIntClear(GPIO_PORTQ_BASE, ui32Status);

	if(ui32Status & GPIO_PIN_7)
	{
		/*
		 * Review comment - SK
		 * We should think about taking *shortStatus* into the "StatusHandler" function.
		 * On init the statusHandler can know what happend to DLPC150.
		 */
		dlpc150_GetShortStatus( &shortStatus );
		DEBUG_PRINT(" DLPC150: Short Statue  0x%x\n", shortStatus);
		if (nnoStatus_setErrorStatusAndCode(NNO_ERROR_HW, true, NNO_ERROR_HW_DLPC150) < 0)
			DEBUG_PRINT("TIVA error status could not be updated\n");
	}
}

/**
 * Returns DLPC150 software version in the pointer passed.
 *
 *  @param pVer	-O - pointer to pass software revision number
 *
 *  \return PASS or FAIL
 */
int16_t dlpc150_GetSequenceVectorNumber( uint16_t *pSeqVectNum )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_SEQ_VECT_NUM};
	uint8_t val[2] = { 0xff, 0xff};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 2;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*pSeqVectNum |= ((uint16_t)val[1] << 8);
	*pSeqVectNum |= (uint16_t)val[0];

	dlpc150_close(i2c);
	return ret_val;
}

#if 0
int16_t dlpc150_GetGPIOControl( uint32_t *gpioControl )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_GPIO_R_CONTROL};
	uint8_t val[6];
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 4;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*gpioControl = (uint32_t)val[0];
	*gpioControl |= (uint32_t)val[1] << 8;
	*gpioControl |= (uint32_t)val[2] << 16;
	*gpioControl |= (uint32_t)val[3] << 24;

	dlpc150_close(i2c);
	return ret_val;
}

int16_t dlpc150_SetGPIOControl( uint32_t gpioControl )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[7] = {DLPC150_GPIO_W_CONTROL, gpioControl, gpioControl >> 8, gpioControl >> 16, gpioControl >> 24};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 5;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

int16_t dlpc150_GetGPIOOutput( uint32_t *gpioControl0, uint32_t *gpioControl1 )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_GPIO_R_OUTPUT};
	uint8_t val[6];
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = val;
	i2cTransaction.readCount = 6;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	*gpioControl1 = (uint32_t)val[4] ;
	*gpioControl1 |= (uint32_t)val[5] << 8;
	*gpioControl0 |= (uint32_t)val[0];
	*gpioControl0 |= (uint32_t)val[1] << 8;
	*gpioControl0 |= (uint32_t)val[2] << 16;
	*gpioControl0 |= (uint32_t)val[3] << 24;

	dlpc150_close(i2c);
	return ret_val;
}

int16_t dlpc150_SetGPIOOutput( uint32_t gpioControl0, uint32_t gpioControl1 )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[7] = {DLPC150_GPIO_W_OUTPUT, gpioControl0, gpioControl0>>8, gpioControl0>>16, gpioControl0>>24, gpioControl1, gpioControl1>>8};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 7;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

int16_t dlpc150_SetSequenceAbort(void)
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_SEQ_ABORT};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

int16_t dlpc150_SetHWLockMode(void)
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[1] = {DLPC150_HWLOCK_MODE};
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}


int16_t dlpc150_videoSourceFormatSelect( void )
{
	I2C_Transaction   i2cTransaction;
	uint8_t cmds[2] = {DLPC150_SYS_W_EXT_VID_SOURCE, VIDEO_SOURCE_RGB888 };
	int16_t ret_val;
	I2C_Handle      i2c = dlpc150_open();

	i2cTransaction.slaveAddress = BOARD_DLPC150_ADDR;
	i2cTransaction.writeBuf = cmds;
	i2cTransaction.writeCount = 2;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (I2C_transfer( i2c, &i2cTransaction))
	{
		ret_val =  PASS;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_INIT_ERROR);
		ret_val =  FAIL;
	}

	dlpc150_close(i2c);
	return ret_val;
}

#endif
//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************


