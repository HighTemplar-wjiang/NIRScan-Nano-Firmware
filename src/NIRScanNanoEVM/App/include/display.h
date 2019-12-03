/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/


#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "dlpspec_scan.h"
#include "dlpspec_calib.h"

#ifdef INTERNAL_FRAMEBUFFER
#ifdef SIXTEEN_BPP
#define NUM_FRAMEBUFFERS 72
#define NUM_BP_PER_FRAME			16		// num pattern per frame
#else
#define NUM_FRAMEBUFFERS 36
#define NUM_BP_PER_FRAME			24		// num pattern per frame
#endif
#else  // EXTERNAL FRAME BUFFER
#ifdef SIXTEEN_BPP
#define NUM_FRAMEBUFFERS 40
#define NUM_BP_PER_FRAME			16		// num pattern per frame
#else  // 24-bit EXTERNAL FRAME BUFFER
#define NUM_FRAMEBUFFERS 26
#define NUM_BP_PER_FRAME			24		// num pattern per frame
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

void Display_Init(void);
void Display_Disable();
int  Display_GenScanPatterns(uScanConfig *pCfg);
int  Display_GenCalibPatterns(CALIB_SCAN_TYPES scan_type);
void Display_SetFrameBufferAtBeginning(void);
int Display_FramePropagationWait(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H_ */
