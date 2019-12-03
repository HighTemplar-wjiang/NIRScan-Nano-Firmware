/*
 * Header for SDRAM
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 */

#ifndef SDRAM_H_
#define SDRAM_H_

#include <math.h>

// Alliance Memory AS4C16M16S Parameters
#define SDRAM_SIZE				EPI_SDRAM_SIZE_256MBIT
#define SDRAM_CYCLE_REFRESH		8192
#define SDRAM_REFRESH			floor((0.064 * 120000000)/(SDRAM_CYCLE_REFRESH * 2))   // 64ms/cycle refresh
#if 1
#define SDRAM_START_ADDRESS 	0x60000000
#define SDRAM_END_ADDRESS 		0x6FFFFFFF
#else
#define SDRAM_START_ADDRESS     0x10000000
#define SDRAM_END_ADDRESS       0x1FFFFFFF
#endif
#define SDRAM_32MB              0x01000000

#ifdef __cplusplus
extern "C" {
#endif

int sdram_test();
int32_t sdram_init( void );

#ifdef __cplusplus
}
#endif

#endif /* SDRAM_H_ */
