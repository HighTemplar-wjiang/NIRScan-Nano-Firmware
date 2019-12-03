/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/ssi.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <utils/spi_flash.h>

#include "common.h"
#include "NIRscanNano.h"
#include "flash.h"

#define FLASH_SPI_PAGE_SIZE 256
#define FLASH_SPI_START_ADDR 0
#define FLASH_SPI_SECTOR_SIZE 4096
#define FLASH_SPI_BLOCK32_SIZE 32768
#define FLASH_SPI_BLOCK64_SIZE 65536

#define FLASH_SPI_CLOCK_RATE_HZ 2500000
#define TEMP_BUF_SIZE 512

#define CMD_ENABLE_RESET	0x66
#define CMD_RESET			0x99
#define CMD_PWRUP           0xab        // Power Up
#define CMD_PWRDN           0xb9        // Power Down

#define ERASE_BLOCK_SIZE FLASH_SPI_BLOCK64_SIZE
#define FLASH_TIMEOUT_COUNTER 100000

static uint32_t flash_prgm_addr = 0;
static uint8_t tempDataBuf[TEMP_BUF_SIZE];
extern uint32_t g_ui32SysClk;

void flash_spi_reset(void);

int32_t flash_spi_init(void)
{
	uint8_t manID = 0;
	uint16_t devID = 0;
	int timeoutCounter = FLASH_TIMEOUT_COUNTER;
	/*
	// The SSI0 peripheral must be enabled for use.
	*/
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	/*
	// Configure the pin muxing for SSI2 functions
	*/
	GPIOPinConfigure(GPIO_PG4_SSI2XDAT1);
	GPIOPinConfigure(GPIO_PG5_SSI2XDAT0);
	GPIOPinConfigure(GPIO_PG7_SSI2CLK);
	GPIOPinConfigure(GPIO_PG6_SSI2FSS);

	/*
	// Configure the GPIO settings for the SSI pins. This function also gives
	// control of these pins to the SSI hardware.
	*/
	GPIOPinTypeSSI(GPIO_PORTG_BASE, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4);

	/*
	//	 Configure SSI module to use with flash at 5Mhz frequency
	*/
	SPIFlashInit(SSI2_BASE, g_ui32SysClk, FLASH_SPI_CLOCK_RATE_HZ);
	
	flash_spi_reset();
	do
	{
	    SPIFlashReadID(SSI2_BASE, &manID, &devID);
	    if ( (manID != 0xef) && (devID != 0x4017) )
	        DEBUG_PRINT("Error reading incorrect manID = %x, devID = %x\n", manID, devID);
	} while ( ((manID != 0xef) && (devID != 0x4017)) && --timeoutCounter );

	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("Several attempts to read manID devID failed\n");
		return FAIL;
	}
	else
	{
		DEBUG_PRINT("Correct manID = %x, devID = %x\n", manID, devID);
	}

	SPIFlashWriteEnable(SSI2_BASE);
	timeoutCounter = FLASH_TIMEOUT_COUNTER;
	while( (SPIFlashReadStatus(SSI2_BASE) & 2) == 0)//wait for WEL bit to set
	{
		if(--timeoutCounter == 0)
			break;
	}
	if(timeoutCounter == 0)
	{
		DEBUG_PRINT("Flash status read timedout\n");
		return FAIL;
	}
	SPIFlashWriteStatus(SSI2_BASE, 0); //Clear all write protection

	return PASS;
}

int32_t flash_spi_chip_erase(void)
{
	uint32_t flash_addr = 0;
	uint32_t flash_end = flash_addr + DLPC150_FLASH_SIZE;
	int timeoutCounter ;
	int retval = PASS;

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	while(flash_addr < flash_end)
	{
		SPIFlashWriteEnable(SSI2_BASE); //Needs to be called before every write or erase command
		timeoutCounter = FLASH_TIMEOUT_COUNTER;
		while( (SPIFlashReadStatus(SSI2_BASE) & 2) == 0)//wait for WEL bit to set
		{
			if(--timeoutCounter == 0)
				break;
		}
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("Flash status read timedout waiting for write enable\n");
			retval = FAIL;
			break;
		}

		if(ERASE_BLOCK_SIZE == FLASH_SPI_BLOCK32_SIZE)
			SPIFlashBlockErase32(SSI2_BASE, flash_addr);
		else if (ERASE_BLOCK_SIZE == FLASH_SPI_BLOCK64_SIZE)
			SPIFlashBlockErase64(SSI2_BASE, flash_addr);
		else
			SPIFlashSectorErase(SSI2_BASE, flash_addr);

		timeoutCounter = FLASH_TIMEOUT_COUNTER;
		while( (SPIFlashReadStatus(SSI2_BASE) & 3) != 0) //wait for flash busy bit and WEL bit to clear
		{
			if(--timeoutCounter == 0)
				break;
		}
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("Flash status read timedout waiting for busy bit to clear\n");
			retval = FAIL;
			break;
		}

		//DEBUG_PRINT("Chip Erase completed after %d status reads\n", waitCount);

		flash_addr += ERASE_BLOCK_SIZE;
	}
	MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_SSI2);
	return retval;
}

int32_t flash_spi_program_init(void)
{
	flash_prgm_addr = FLASH_SPI_START_ADDR;
	return PASS;
}

int32_t flash_spi_program(uint8_t *pData, uint32_t numBytes)
{
	uint32_t programSize;
	uint32_t remPageSize;
	int timeoutCounter = FLASH_TIMEOUT_COUNTER;
	int retval = PASS;

	while(numBytes)
	{
		/* if address is not in a 256 byte boundary, determine the remaining size of this page */
		remPageSize = FLASH_SPI_PAGE_SIZE - (flash_prgm_addr - (flash_prgm_addr & ~(FLASH_SPI_PAGE_SIZE-1)));
		programSize = MIN(numBytes, remPageSize);
		if((flash_prgm_addr & (ERASE_BLOCK_SIZE-1)) == 0)
		{
			SPIFlashWriteEnable(SSI2_BASE); //Needs to be called before every write or erase command

			timeoutCounter = FLASH_TIMEOUT_COUNTER;
			while( (SPIFlashReadStatus(SSI2_BASE) & 2) == 0)//wait for WEL bit to set
			{
				if(--timeoutCounter == 0)
					break;
			}
			if(timeoutCounter == 0)
			{
				DEBUG_PRINT("Flash status read timedout\n");
				retval = FAIL;
				break;
			}

			if(ERASE_BLOCK_SIZE == FLASH_SPI_BLOCK32_SIZE)
				SPIFlashBlockErase32(SSI2_BASE, flash_prgm_addr);
			else if (ERASE_BLOCK_SIZE == FLASH_SPI_BLOCK64_SIZE)
				SPIFlashBlockErase64(SSI2_BASE, flash_prgm_addr);
			else
				SPIFlashSectorErase(SSI2_BASE, flash_prgm_addr);

			timeoutCounter = FLASH_TIMEOUT_COUNTER;
			//wait for flash busy bit and WEL bit to clear
			while( (SPIFlashReadStatus(SSI2_BASE) & 3) != 0);
			{
				if(--timeoutCounter == 0)
					break;
			}
			if(timeoutCounter == 0)
			{
				DEBUG_PRINT("Flash status read timedout\n");
				retval = FAIL;
				break;
			}
		}
		SPIFlashWriteEnable(SSI2_BASE); //Needs to be called before every write or erase command

		timeoutCounter = FLASH_TIMEOUT_COUNTER;
		while( (SPIFlashReadStatus(SSI2_BASE) & 2) == 0) //wait for WEL bit to set
		{
			if(--timeoutCounter == 0)
				break;
		}
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("Flash status read timedout\n");
			retval = FAIL;
			break;
		}

		SPIFlashPageProgram(SSI2_BASE, flash_prgm_addr, pData, programSize);

		timeoutCounter = FLASH_TIMEOUT_COUNTER;
		while( (SPIFlashReadStatus(SSI2_BASE) & 3) != 0) //wait for flash busy bit and WEL bit to clear
		{
			if(--timeoutCounter == 0)
				break;
		}
		if(timeoutCounter == 0)
		{
			DEBUG_PRINT("Flash status read timedout\n");
			retval = FAIL;
			break;
		}

		flash_prgm_addr += programSize;
		numBytes -= programSize;
		pData += programSize;
	}

	return retval;
}

uint32_t flash_spi_compute_checksum(uint32_t addr, uint32_t dataSize)
{

	uint32_t i;
	uint32_t dataChkSum = 0;
	uint32_t read_len;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	while(dataSize)
	{
		read_len = MIN(dataSize, TEMP_BUF_SIZE);
		SPIFlashRead(SSI2_BASE, addr, tempDataBuf, read_len);
		addr += read_len;
		for(i=0; i<read_len; i++)
		{
			dataChkSum += tempDataBuf[i];
		}
		dataSize -= read_len;
	}
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_SSI2);
	return dataChkSum;
}

int32_t flash_spi_program_from_file(const char *fwFileName)
{
	FILE *pDlpcFwFile;
	long int len;
	long int file_len;
	uint32_t read_len;
	uint32_t checksum=0;
	uint32_t flash_checksum=0;
	uint32_t i;

	pDlpcFwFile = fopen(fwFileName, "rb");

	if(pDlpcFwFile == NULL)
	{
		return FAIL;
	}

	fseek(pDlpcFwFile, 0, SEEK_END);
	len = ftell(pDlpcFwFile);
	file_len = len;
	rewind(pDlpcFwFile);

	while(len)
	{
		read_len = MIN(len, TEMP_BUF_SIZE);
		if(fread(tempDataBuf, 1, read_len, pDlpcFwFile) != read_len)
		{
			fclose(pDlpcFwFile);
			return FAIL;
		}
		flash_spi_program(tempDataBuf, read_len);
		len -= read_len;
		for(i=0; i<read_len; i++)
			checksum += tempDataBuf[i];
	}

	fclose(pDlpcFwFile);

	flash_checksum = flash_spi_compute_checksum(0, file_len);

	if( flash_checksum != checksum )
	{
	    DEBUG_PRINT("Checksum mismatch expected = %x, calculated = %x\n", checksum, flash_checksum);
		return FAIL;
	}
	DEBUG_PRINT("Checksum match\n" );
	return PASS;
}



void flash_spi_reset(void)
{    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(SSI2_BASE, SSI_ADV_MODE_WRITE);

    //
    // Send the enable reset command.
    //
    MAP_SSIAdvDataPutFrameEnd(SSI2_BASE, CMD_ENABLE_RESET);
}

void flash_spi_powerdown(void)
{
    //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(SSI2_BASE, SSI_ADV_MODE_WRITE);

    //
    // Send the read ID command.
    //
    MAP_SSIAdvDataPutFrameEnd(SSI2_BASE, CMD_PWRDN);
}

void flash_spi_powerup(void)
{
     //
    // Set the SSI module into write-only mode.
    //
    MAP_SSIAdvModeSet(SSI2_BASE, SSI_ADV_MODE_WRITE);

    //
    // Send the read ID command.
    //
    MAP_SSIAdvDataPutFrameEnd(SSI2_BASE, CMD_PWRUP);
}
