/*
 * Copyright (c) 2014-15, Texas Instruments Incorporated
 * All rights reserved.
 *
 */

#ifndef _FATSD_H
#define _FATSD_H
#include <ti/sysbios/fatfs/ff.h>

// error codes
#define	SDCARD_ERROR_OK 					0	/* (0) Succeeded */
#define SDCARD_ERROR_DISK_ERR				1	 /* (1) A hard error occured in the low level disk I/O layer */
#define SDCARD_ERROR_INT_ERR				2	 /* (2) Assertion failed */
#define SDCARD_ERROR_NOT_READY				3	/* (3) The physical drive cannot work */
#define SDCARD_ERROR_NO_FILE				4	/* (4) Could not find the file */
#define SDCARD_ERROR_NO_PATH				5	/* (5) Could not find the path */
#define SDCARD_ERROR_INVALID_NAME			6	/* (6) The path name format is invalid */
#define SDCARD_ERROR_DENIED					7	/* (7) Acces denied due to prohibited access or directory full */
#define SDCARD_ERROR_EXIST					8	/* (8) Acces denied due to prohibited access */
#define SDCARD_ERROR_INVALID_OBJECT			9	/* (9) The file/directory object is invalid */
#define SDCARD_ERROR_WRITE_PROTECTED		10	/* (10) The physical drive is write protected */
#define SDCARD_ERROR_INVALID_DRIVE			11 	/* (11) The logical drive number is invalid */
#define SDCARD_ERROR_NOT_ENABLED			12	/* (12) The volume has no work area */
#define SDCARD_ERROR_NO_FILESYSTEM			13	/* (13) There is no valid FAT volume on the physical drive */
#define SDCARD_ERROR_MKFS_ABORTED			14	/* (14) The f_mkfs() aborted due to any parameter error */
#define SDCARD_ERROR_TIMEOUT				15  /* (15) Could not get a grant to access the volume within defined period */
#define SDCARD_ERROR_LOCKED					16  /* (16) The operation is rejected according to the file shareing policy */
#define SDCARD_ERROR_NOT_ENOUGH_CORE		17	/* (17) LFN working buffer could not be allocated */
#define SDCARD_ERROR_TOO_MANY_OPEN_FILES 	18	/* (18) Number of open files > _FS_SHARE */
#define SDCARD_ERROR_WRITE_READ_ERROR		19
#define NUM_SCAN_DATA_INDEX 256

#ifdef __cplusplus
extern "C" {
#endif

int FATSD_Test( void );
FRESULT FATSD_Init(void);
FRESULT FATSD_WriteScanFile( void *pBuf, int bufLen , unsigned int index);
FRESULT FATSD_WriteReferenceFile(void);
FRESULT FATSD_ReadLastStoredScanFile( void *pBuf, uint32_t *pBufLen);
FRESULT FATSD_ReadScanFile(uint32_t index, void *pBuf, uint32_t *pBufLen);
FRESULT FATSD_DeleteScanFile(unsigned int index);
FRESULT FATSD_DeleteLastScanFile(void);
int FATSD_GetNumScanFiles(void);
FRESULT FATSD_FindListScanIndex(int* length);
bool FATSD_SkipEEPROMCfg(void);

#ifdef __cplusplus
}
#endif

#endif //_FATSD_H
