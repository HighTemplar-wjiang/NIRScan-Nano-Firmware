/*
 * Copyright (c) 2014-2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/fatfs/ff.h>
#include <ti/sysbios/knl/Clock.h>
/* TIVA/Driver Header files */
#include <inc/tm4c129xnczad.h>
#include "inc/hw_memmap.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include <driverlib/gpio.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SDSPI.h>
#include "GPIO Mapping.h"
#include "Board.h"
#include "dlpspec_scan.h"
#include "scan.h"
#include "nano_eeprom.h"
#include "nnoStatus.h"
#include "fatsd.h"

/* String conversion macro */
#define STR_(n)             #n
#define STR(n)              STR_(n)

/* Drive number used for FatFs */
#define DRIVE_NUM           0
#define SCAN_FILE_NAME_PREFIX 	 "scan"
#define REF_FILE_NAME 			 "fact_ref"
#define SCAN_FILE_NAME_EXTENSION ".dat"

static const char  inputfile[] = STR(DRIVE_NUM)":\\input.txt";
static const char outputfile[] = STR(DRIVE_NUM)":\\output.txt";
static bool card_detected = false;
static int running_file_num = 0;
static const char driveNum[] = STR(DRIVE_NUM);
static char scanFilePath[50];
extern uint8_t g_dataBlob[];
static char directory_name[8];
static bool skip_eeprom_cfg = false;

unsigned int g_scanIndices[NUM_SCAN_DATA_INDEX];

static FATFS FatFs;   /* Work area (file system object) for logical drive */

const char textarray[] = \
"***********************************************************************\n"
"0         1         2         3         4         5         6         7\n"
"01234567890123456789012345678901234567890123456789012345678901234567890\n"
"This is some text to be inserted into the inputfile if there isn't     \n"
"already an existing file located on the SDCard.                        \n"
"***********************************************************************\n";

void HandleSDCardDetectInterrupt()
{
#ifdef HW_SD_CARD_DETECT
	// Clear the interrupt
	MAP_GPIOIntClear(GPIO_PORTQ_BASE, GPIO_INT_PIN_4);
	MAP_IntPendClear(INT_GPIOQ4);

	//Toggle card detect status
	card_detected = ~card_detected;

	nnoStatus_setDeviceStatus(NNO_STATUS_SD_CARD_PRESENT,card_detected);
#endif
}

static FRESULT printDrive(const char *driveNumber, FATFS **fatfs)
/*
 *  ======== printDrive ========
 *  Function to print drive information such as the total disk space
 *  This function was created by referencing FatFs's API documentation
 *  http://elm-chan.org/fsw/ff/en/getfree.html
 *
 *  This function call may take a while to process, depending on the size of
 *  SD Card used.
 */
{
	FRESULT        fresult;
	DWORD          freeClusterCount;

	DEBUG_PRINT("Reading disk information...");

	fresult = f_getfree(driveNumber, &freeClusterCount, fatfs);
	if (fresult != FR_OK) 
	{
		DEBUG_PRINT("Error getting the free cluster count from the FatFs object");
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult );
		return fresult;
	}
	else 
	{
		/* Print the free space (assuming 512 bytes/sector) */
		DEBUG_PRINT("Total Disk size: %10lu KiB\n"
				"Free Disk space: %10lu KiB\n",
				(((*fatfs)->n_fatent - 2) * (*fatfs)->csize) / 2,
				(freeClusterCount * (*fatfs)->csize)  / 2);
	}

	return fresult;
}

static char *FATSD_GetScanFileName(uint32_t num)
/**
 * This API creates the path for filename that starts with scan index
 * number input. FATFS system accepts path name of format
 * "[drive:][/]directory/file". Please refer the link
 * http://elm-chan.org/fsw/ff/en/filename.html for details.
 *
 * @param  num -I- scan index number used for filename
 *
 * @return scanFilePath = path used to create the file with filename
 *                        as scan index number.
 */
{
	char fileName[40];

	sprintf(fileName, "%08x%s", num, SCAN_FILE_NAME_EXTENSION);
	strcpy(scanFilePath, driveNum);
	strcat(scanFilePath, ":");
	strcat(scanFilePath, "/");
	strcat(scanFilePath, directory_name);
	strcat(scanFilePath, "/");
	strcat(scanFilePath, fileName);

	DEBUG_PRINT("Filename:%s",scanFilePath);

	return scanFilePath;
}

static char *FATSD_GetRefFileName()
/**
 * This API creates the path for filename that starts with scan index
 * number input. FATFS system accepts path name of format
 * "[drive:][/]directory/file". Please refer the link *
 *
 * @return scanFilePath = path used to create the file with filename
 */
{
	strcpy(scanFilePath, driveNum);
	strcat(scanFilePath, ":");
	strcat(scanFilePath, "/");
	strcat(scanFilePath, directory_name);
	strcat(scanFilePath, "/");
	strcat(scanFilePath, REF_FILE_NAME);
	strcat(scanFilePath, SCAN_FILE_NAME_EXTENSION);

	return scanFilePath;
}

FRESULT FATSD_CreateDirectory(void)
/**
 * This API creates the directory in SD card with device serial number as
 * name if it does not exist already.
 *
 * @return FR_INVALID_NAME = Error if serial number is not valid
 *         FR_OK           = Direcory created successfully
 *         FR_EXIST        = Directory already exists
 */
{
    FRESULT fr;
    char serial_number[8];

    if ( Nano_eeprom_GetDeviceSerialNumber((uint8_t*)serial_number) == PASS )
    {
		fr = f_mkdir(serial_number);
		if ((fr == FR_OK) || (fr == FR_EXIST))
		{
			strcpy(directory_name , serial_number);
			fr = f_chmod(serial_number,0,AM_RDO);
			if (fr != FR_OK)
			{
				nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fr);
				DEBUG_PRINT("\r\nChange directory permission fialed, error:%d\r\n",fr);
			}
		}

		return fr;
	}
    else
    {
		nnoStatus_setErrorStatus(NNO_ERROR_EEPROM, true);
		return FR_INVALID_NAME;
	}
}

FRESULT FATSD_FindListScanIndex(int* length)
/**
 * This API reads all the filenames in the directory named as device serial number.
 * filenames are used to extract scan data index already stored. It also counts the
 * number of files already stored.
 *
 * @param  length -O- Number of files already stored in the directory
 *
 * @return FR_INVALID_NAME = Error if serial number is not valid
 *         FR_NOT_READY    = Error if card not detected.
 */
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i = 0;
    int cnt = 0;
    unsigned int val;

    char path[25];
    char index_str[8];

    if(nnoStatus_getIndDeviceStatus(NNO_STATUS_SD_CARD_PRESENT) == false)
    	return (FR_NOT_READY);

    strcpy(path, driveNum);
    strcat(path, ":");
    strcat(path, "/");
    strcat(path, directory_name);

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_NO_PATH)
    {
    	res = FATSD_CreateDirectory();
    	if (res== FR_OK)
    	{
    		*length = cnt;
    		return res;
    	}
    }

    if (res == FR_OK)
    {
	   while(1)
	   {
			res = f_readdir(&dir, &fno);                   /* Read a directory item */
			if ((res != FR_OK) || (fno.fname[0] == 0))
				break;  /* Break on error or end of dir */
			if (fno.fname[0] == '.')
				continue;             /* Ignore dot entry */

			if (
				 fno.fname[0] == 'S' && \
				 fno.fname[1] == 'K' && \
				 fno.fname[2] == 'I' && \
				 fno.fname[3] == 'P' && \
				 fno.fname[4] == '_' && \
				 fno.fname[5] == 'C' && \
				 fno.fname[6] == 'F' && \
				 fno.fname[7] == 'G' )
			{
				 skip_eeprom_cfg = true;
			}
			else if ((fno.fname[8]  == '.' && \
				 fno.fname[9]  == 'D' && \
				 fno.fname[10] == 'A' && \
				 fno.fname[11] == 'T' && \
				 fno.fname[12] ==  0 ) && \
				 fno.fname[0] != 'F' && \
				 fno.fname[1] != 'A' && \
				 fno.fname[2] != 'C' && \
				 fno.fname[3] != 'T' && \
				 fno.fname[4] != '_' && \
				 fno.fname[5] != 'R' && \
				 fno.fname[6] != 'E' && \
				 fno.fname[7] != 'F' )
			{
				for( i=0 ; i<8 ;i++)
				{
					if (fno.fname[i] != '.')
						index_str[i] = fno.fname[i];
					else
						break;
				}

				if( (i > 0) || (cnt < NUM_SCAN_DATA_INDEX))
				{
					val = strtoul(index_str, NULL, 16);
					//strtoul returns 0 if a valid conversion could not be performed
					if(val != 0)
						g_scanIndices[cnt++] = val;
				}
			}

	   }
    }
    else
    	nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, res);

    *length = cnt;

    return res;
}
static int FATSD_FindNumScanFiles(void)
/**
 * This API returns the number of files already stored in the directory
 * with the name as device serial number.
 *
 * @return number of files found
 */
{
    FRESULT fresult;
	int num = 0;

	if(nnoStatus_getIndDeviceStatus(NNO_STATUS_SD_CARD_PRESENT) == false)
		return (FAIL);

	fresult = FATSD_FindListScanIndex(&num);

	if(fresult != FR_OK)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
	    return FAIL;
	}

	return num;
}

FRESULT FATSD_Init(void)
/**
 * This API initializes the SD card system over SPI. Also the number of files
 * stored already is initialized to scan index array counter.
 *
 * @return FR_INVALID_NAME = Error if serial number is not valid
 *         FR_NOT_READY    = Error if card not detected.
 *         FR_OK           = Sucessful
 */
{

	SDSPI_Handle sdspiHandle = NULL;
	SDSPI_Params sdspiParams;
	char serial_number[8];
	FRESULT result;
#ifndef HW_SD_CARD_DETECT
	FIL src;
#endif
	// Clear card detect interrupts to start with
	MAP_GPIOIntClear(GPIO_PORTQ_BASE, GPIO_INT_PIN_4);
	MAP_IntPendClear(INT_GPIOQ4);
	MAP_GPIOIntEnable( GPIO_PORTQ_BASE, GPIO_PIN_4);

	/* Mount and register the SD Card */
	SDSPI_Params_init(&sdspiParams);
	sdspiHandle = SDSPI_open(BOARD_SDSPI0, DRIVE_NUM, &sdspiParams);
	if (sdspiHandle == NULL)
	{
		DEBUG_PRINT("Error starting the SD card\n");
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, FR_NOT_READY);
		return FR_NOT_READY;
	}

	/* Register work area to the default drive */
	if((result = f_mount(0, &FatFs)) != FR_OK)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, result);
		return result;
	}

	DEBUG_PRINT("Drive %u is mounted\n", DRIVE_NUM);

	/*
	 * SW has no mechanism to find out if card is present or not, so
	 * set card_detected to true to start with. Read/write may fail
	 * if card wasn't present
	 */
#ifndef HW_SD_CARD_DETECT
	/*
	 * As a workaround, try creating a file and deduce if card is present
	 * based on the error returned (when card is not present)
	 */
	result = f_open(&src, inputfile, FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
	if (result == FR_NOT_READY)
		card_detected = false;
	else
	{
		card_detected = true;
		if (result == FR_OK)
		{
			f_close(&src);
			f_unlink(inputfile);
		}
	}
	nnoStatus_setDeviceStatus(NNO_STATUS_SD_CARD_PRESENT,card_detected);
#endif

	if ( PASS == Nano_eeprom_GetDeviceSerialNumber((uint8_t*)serial_number) )
	{
		strcpy(directory_name , serial_number);

		running_file_num = FATSD_FindNumScanFiles();
		if(running_file_num >= 0)
			return FR_OK;
		else
			return FR_NOT_READY;
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, 
				FR_INVALID_NAME);
		return FR_INVALID_NAME;
	}

}

int FATSD_Test( void )
/*
 *  perform a file copy
 *
 *  tries to open an existing file inputfile[]. If the file doesn't
 *  exist, create one and write some known content into it.
 *  The contents of the inputfile[] are then copied to an output file
 *  outputfile[]. Once completed, the contents of the output file are
 *  printed onto the system console (stdout).
 *
 *  Task for this function is created statically. See the project's .cfg file.
 *
 *  @return None
 */
{
	int fresult;
	/* Variables to keep track of the file copy progress */
	unsigned int bytesRead = 0;
	unsigned int bytesWritten = 0;
	unsigned int totalBytesCopied = 0;
	FIL dst;
	FIL src;

	/* Try to open the source file */
	DEBUG_PRINT("Creating a new file \"%s\"...", inputfile);

	/* Open file for both reading and writing */
	fresult = f_open(&src, inputfile, FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
	if (fresult != FR_OK) 
	{
		DEBUG_PRINT("Error: \"%s\" could not be created\n", inputfile);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		return fresult;
	}

	f_write(&src, textarray, strlen(textarray), &bytesWritten);
	f_sync(&src);

	/* Reset the internal file pointer */
	f_lseek(&src, 0);

	DEBUG_PRINT("done\n");

	/* Create a new file object for the file copy */
	fresult = f_open(&dst, outputfile, FA_CREATE_ALWAYS|FA_WRITE);
	if (fresult != FR_OK) 
	{
		DEBUG_PRINT("Error opening \"%s\"\n", outputfile);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		return fresult;
	}
	else
	{
		DEBUG_PRINT("Starting file copy\n");
	}

	/*  Copy the contents from the src to the dst */
	while (true)
	{
		/*  Read from source file */
		fresult = f_read(&src, g_dataBlob, SCAN_DATA_BLOB_SIZE, &bytesRead);
		if (fresult || bytesRead == 0)
		{
			break; /* Error or EOF */
		}

		/*  Write to dst file */
		fresult = f_write(&dst, g_dataBlob, bytesRead, &bytesWritten);
		if (fresult || bytesWritten < bytesRead)
		{
			DEBUG_PRINT("Disk Full\n");
			break; /* Error or Disk Full */
		}

		/*  Update the total number of bytes copied */
		totalBytesCopied += bytesWritten;
	}
	if(fresult != FR_OK)
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);

	f_sync(&dst);

	DEBUG_PRINT("File \"%s\" (%u B) copied to \"%s\" (Wrote %u B)\n",
			inputfile, f_size(&src), outputfile, totalBytesCopied);

	/* Close both inputfile[] and outputfile[] */
	f_close(&src);
	f_close(&dst);

	/* Now output the outputfile[] contents onto the console */
	fresult = f_open(&dst, outputfile, FA_READ);
	if (fresult != FR_OK)
	{
		DEBUG_PRINT("Error opening \"%s\"\n", outputfile);
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		System_abort("Aborting...\n");
	}

	/* Print file contents */
	while (true)
	{
		/* Read from output file */
		fresult = f_read(&dst, g_dataBlob, SCAN_DATA_BLOB_SIZE, &bytesRead);
		if (fresult || bytesRead == 0)
		{
			break; /* Error or EOF */
		}
		g_dataBlob[bytesRead] = '\0';
		if ( strcmp( (char *) textarray, (const char *)g_dataBlob) != 0 )
		{
			fresult = SDCARD_ERROR_WRITE_READ_ERROR;
			break;
		}
		/* Write output */
		DEBUG_PRINT("%s", g_dataBlob);
	}
	if(fresult != FR_OK)
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);

	/* Close the file */
	f_close(&dst);

	printDrive(STR(DRIVE_NUM), &(dst.fs));

	return ( fresult );

}

int FATSD_GetNumScanFiles(void)
/**
 * This API returns the scan data index array counter
 *
 * @return counter value
 */

{
	return running_file_num;
}

FRESULT FATSD_ReadLastStoredScanFile( void *pBuf, uint32_t *pBufLen )
/**
 * This API reads last stored scan data in memory buffer. This can be used
 * to store and retrive scan data in SD card in stack like manner.
 *
 * @param  pBuf   -I- pointer to memory buffer
 * @param  pBufLen -O- length of file read
 *
 * @return FRESULT = Refer http://elm-chan.org/fsw/ff/en/rc.html#nr for
 *                   return codes.
 */
{
	return (FATSD_ReadScanFile(g_scanIndices[running_file_num-1], pBuf, pBufLen));
}

FRESULT FATSD_ReadScanFile(uint32_t index, void *pBuf, uint32_t *pBufLen)
/**
 * This API reads scan data index file stored in SD card.
 *
 * @param  index   -I- the scan data index need to be read
 * @param  pBuf   -I- pointer to memory buffer
 * @param  pBufLen -O- length of file read
 *
 * @return FRESULT = Refer http://elm-chan.org/fsw/ff/en/rc.html#nr for
 *                   return codes.
 */
{
    FRESULT fresult;
	FIL src;

	if(card_detected == false)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, FR_NOT_READY);
		return FR_NOT_READY;
	}

	fresult = f_open(&src, FATSD_GetScanFileName(index), FA_READ);
	if (fresult != FR_OK)
   	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		return fresult;
	}

	/* Read from output file */
	fresult = f_read(&src, pBuf, SCAN_DATA_BLOB_SIZE, pBufLen);
	if ((fresult != FR_OK) || *pBufLen == 0)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		return fresult;
	}

	return fresult;
}

FRESULT FATSD_WriteScanFile( void *pBuf, int bufLen , unsigned int index )
/**
 * This API writes scan data index file to SD card.
 *
 * @param  pBuf   -I- pointer to memory buffer
 * @param  bufLen -I- length of file read
 * @param  index  -I- the scan data index need to be read
 *
 * @return FRESULT = Refer http://elm-chan.org/fsw/ff/en/rc.html#nr for
 *                   return codes.
 */
{
    FRESULT fresult = FR_OK;
    unsigned int bytesWritten = 0;
	FIL dst;

	if(card_detected == false)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, FR_NOT_READY);
		return FR_NOT_READY;
	}

	/* Create the Directory with name as serial number */
    fresult = FATSD_CreateDirectory();
	if ((fresult != FR_OK) && (fresult != FR_EXIST))
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		DEBUG_PRINT("Error: \"%s\" directory could not be created\n",fresult);
		return fresult;
	}

	/* Open file for both reading and writing */
	fresult = f_open(&dst, FATSD_GetScanFileName(index), FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
	if (fresult != FR_OK)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		DEBUG_PRINT("Error: \"%s\" could not be created\n", inputfile);
		return fresult;
	}

	fresult = f_write(&dst, pBuf, bufLen, &bytesWritten);
	f_sync(&dst);
	f_close(&dst);

	g_scanIndices[running_file_num] = index;
	running_file_num++;

	return fresult;
}

FRESULT FATSD_WriteReferenceFile()
/**
 * This API writes scan data index file to SD card.
 *
 * @return FRESULT = Refer http://elm-chan.org/fsw/ff/en/rc.html#nr for
 *                   return codes.
 */
{
    FRESULT fresult = FR_OK;
    unsigned int bytesWritten = 0;
	FIL dst;
	scanData tempScanData;
	int result;

	if(card_detected == false)
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, FR_NOT_READY);
		return FR_NOT_READY;
	}

	/* Create the Directory with name as serial number */
    fresult = FATSD_CreateDirectory();
	if ((fresult != FR_OK) && (fresult != FR_EXIST))
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
		DEBUG_PRINT("Error: \"%s\" directory could not be created\n",fresult);
		return fresult;
	}

	/* If file exists do nothing; else create it */
	fresult = f_open(&dst, FATSD_GetRefFileName(), FA_OPEN_EXISTING);
	if (fresult != FR_OK)
	{
		/* Open file for both reading and writing */
		fresult = f_open(&dst, FATSD_GetRefFileName(), FA_CREATE_ALWAYS|FA_READ|FA_WRITE);
		if (fresult != FR_OK)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
			DEBUG_PRINT("Error: \"%s\" could not be created\n", inputfile);
			return fresult;
		}
		Nano_eeprom_ReadReferenceCalibData(&tempScanData);
		result = dlpspec_scan_write_data((uScanData *)&tempScanData, g_dataBlob, SCAN_DATA_BLOB_SIZE);
		if (result != PASS)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SPEC_LIB, true,
					(int16_t)result);
		}
		else
		{
			fresult = f_write(&dst, g_dataBlob, SCAN_DATA_BLOB_SIZE, &bytesWritten);
		}
		f_sync(&dst);
	}
	f_close(&dst);
	return fresult;
}


FRESULT FATSD_DeleteLastScanFile( void )
/**
 * This API deletes last stored scan data in SD card.
 *
 * @return FRESULT = Refer http://elm-chan.org/fsw/ff/en/rc.html#nr for
 *                   return codes.
 */
{
	FRESULT ret_val = (FATSD_DeleteScanFile(g_scanIndices[running_file_num-1]));

	if(ret_val == FR_OK)
		running_file_num--;

	return (ret_val);
}

FRESULT FATSD_DeleteScanFile(unsigned int index)
/**
 * This API deletes  stored scan data with input index.
 *
 * @param  index -I- index of the scan data to be deleted
 *
 * @return FRESULT = Refer http://elm-chan.org/fsw/ff/en/rc.html#nr for
 *                   return codes.
 */
{
    FRESULT fresult;
    int i = 0;

	fresult = f_unlink(FATSD_GetScanFileName(index));

	if(fresult == FR_OK)
	{
		for (i=0; i < NUM_SCAN_DATA_INDEX; i++)
		{
			if (index == g_scanIndices[i])
			{
				g_scanIndices[i] = 0;
				break;
			}
		}
	}
	else
	{
		nnoStatus_setErrorStatusAndCode(NNO_ERROR_SD_CARD, true, fresult);
	}

	return fresult;
}

bool FATSD_SkipEEPROMCfg(void)
{
	return skip_eeprom_cfg;
}
