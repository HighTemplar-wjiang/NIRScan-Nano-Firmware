/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stdint.h>
#include <stdbool.h>
#include <inc/hw_memmap.h>
#include <driverlib/lcd.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/epi.h"
#include <inc/tm4c129xnczad.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include "scan.h"
#include "GPIO Mapping.h"
#include "NIRscanNano.h"
#include "sdram.h"
#include "led.h"
#include "dlpspec_scan.h"
#include "dlpspec_calib.h"
#include "nano_eeprom.h"
#include "trigger.h"
#include "display.h"

/*****************************************************************************
 *
 *Global variables to store underflow and end of frame events
 *
 *****************************************************************************/
uint32_t  g_ui32UnderflowCount = 0;
uint32_t  g_eof0Count=0;
uint32_t  g_eof1Count=0;
static volatile uint32_t g_fullFrameCount=0;
static uint32_t g_FrameFlipVsyncCount;
static volatile uint32_t vsyncCount = 0;
static tLCDRasterTiming g_tTiming;

/*****************************************************************************
 *
 *Private functions declaration
 *
 *****************************************************************************/
static void Display_InitGPIO(void);
static void Display_InitVideoTiming(void);
static void Display_InitLCD(void);

/*****************************************************************************
 *
 *Frame buffer size and starting address initialized to SDRAM if external
 *
 *****************************************************************************/
#ifdef INTERNAL_FRAMEBUFFER
#ifdef SIXTEEN_BPP
uint16_t g_InternalFrameBuffer0[DISP_WIDTH * DISP_HEIGHT * NUM_FRAMEBUFFERS]; //16 bpp
uint16_t *g_frameBuffer0 = g_InternalFrameBuffer0;
uint32_t  g_frameBufferSz = (DISP_WIDTH * DISP_HEIGHT * 2);
uint16_t *g_frameBuffer1;
#else
uint32_t g_InternalFrameBuffer0[DISP_WIDTH * DISP_HEIGHT * NUM_FRAMEBUFFERS]; //32 bpp
uint32_t *g_frameBuffer0 = g_InternalFrameBuffer0;
uint32_t  g_frameBufferSz = (DISP_WIDTH * DISP_HEIGHT * 4);
uint32_t *g_frameBuffer1;
#endif
#else  // EXTERNAL FRAME BUFFER
#ifdef SIXTEEN_BPP
uint16_t *g_frameBuffer0 = (uint16_t *) SDRAM_START_ADDRESS;
uint32_t  g_frameBufferSz = (DISP_WIDTH * DISP_HEIGHT * 2);
uint16_t *g_frameBuffer1;
#else  // 24-bit EXTERNAL FRAME BUFFER
uint32_t *g_frameBuffer0 = (uint32_t *) SDRAM_START_ADDRESS;
uint32_t  g_frameBufferSz = (DISP_WIDTH * DISP_HEIGHT * 3);
uint32_t *g_frameBuffer1;
#endif
#endif



void DisplayIntHandler()
/**
 * Interrupt service routine for display LCD interrupts( LCD underflow and end of frame )
 * The LCD is restarted when underflow happens. For end of frame interrupts VSyncs,
 * blanking and frame buffer are handled according to the frame requirements of DLPC150
 * controller. This routine also increments frame counter for scan task.
 *
 * @return  None
 *
 */
{
	/*
	 * Handles display-related interrupts
	 */
	uint32_t ui32Status;
	ui32Status = LCDIntStatus(LCD0_BASE, true); // Get the current interrupt status and clear any active interrupts
	LCDIntClear(LCD0_BASE, ui32Status);
	if(ui32Status & LCD_INT_UNDERFLOW) // If we saw an underflow interrupt, restart the raster
	{
  		g_ui32UnderflowCount++;
		LCDRasterEnable(LCD0_BASE);
	}

	if(ui32Status & LCD_INT_RASTER_FRAME_DONE/*LCD_INT_EOF0*/)
	{
		g_eof0Count++;
#ifdef INTERNAL_FRAMEBUFFER
	vsyncCount++;
	if(vsyncCount == 486)
	{
		vsyncCount = 0;
		GPIOPinWrite(VSYNC_GPIO_PORT, VSYNC_GPIO_MASK, VSYNC_GPIO_MASK); /* Enable VSYNC pass-thru */
	}
	else if (vsyncCount == 1) //First vsync to be passed thru - thereafter vsyncs to be masked for N frames.
	{
		GPIOPinWrite(VSYNC_GPIO_PORT, VSYNC_GPIO_MASK, 0); /* Disable VSYNC pass-thru */
	}
	else if (vsyncCount == 3)
	{
		MAP_SysCtlDelay(DELAY_12_5US);
		GPIOPinWrite(DATAEN_GPIO_PORT, DATAEN_GPIO_MASK, DATAEN_GPIO_MASK); /* Enable DATA_EN passthru after 6 blank lines (3 Tiva frames) */
	}
	else if (vsyncCount == (486-3))
	{
		MAP_SysCtlDelay(DELAY_12_5US);
		GPIOPinWrite(DATAEN_GPIO_PORT, DATAEN_GPIO_MASK, 0); /* Hold DATA_EN low to generate6  vertical front porch lines for DLPC150*/
#ifdef SIXTEEN_BPP
		MAP_LCDRasterFrameBufferSet(LCD0_BASE, 0, g_frameBuffer0+g_fullFrameCount*g_frameBufferSz/2, g_frameBufferSz); // p. 1900 TIVA TM4C129XNCZAD
#else
		MAP_LCDRasterFrameBufferSet(LCD0_BASE, 0, g_frameBuffer0+g_fullFrameCount*g_frameBufferSz/4, g_frameBufferSz); // p. 1900 TIVA TM4C129XNCZAD
#endif
		g_fullFrameCount++;
		if(g_fullFrameCount == NUM_FRAMEBUFFERS)
			g_fullFrameCount = 0;
	}
#else
	if(vsyncCount == g_FrameFlipVsyncCount)
	{
#ifdef SIXTEEN_BPP
			MAP_LCDRasterFrameBufferSet(LCD0_BASE, 0, g_frameBuffer0+g_fullFrameCount*g_frameBufferSz/2, g_frameBufferSz); // p. 1900 TIVA TM4C129XNCZAD
#else
			MAP_LCDRasterFrameBufferSet(LCD0_BASE, 0, g_frameBuffer0+g_fullFrameCount*g_frameBufferSz/4, g_frameBufferSz); // p. 1900 TIVA TM4C129XNCZAD
#endif
			g_FrameFlipVsyncCount = vsyncCount + Scan_GetFrameSyncs(g_fullFrameCount++);
			if(g_fullFrameCount == NUM_FRAMEBUFFERS)
				g_fullFrameCount = 0;

			//Trig_FrameCallback(); //only applicable in HW locked mode.
	}

	vsyncCount++;
#endif
	}

}

void Display_Init(void)
/**
 * Initialize and enable the raster display for 24 bits per pixel operation.GPIO ports are configured
 * as special for LCD operation. Videotimings are set based on requirements for DLPC150 controller
 * The master clock used for LCD block is 120MHz.LCD interrupts are enabled here.
 *
 * @return  None
 *
 */
{
	/* Initialize GPIO as special for LCD operation */
    Display_InitGPIO();
    /* Initialize video timings for DLPC150 controller */
    Display_InitVideoTiming();
    /* Initialize the LCD module */
    Display_InitLCD();

}

static void Display_InitGPIO(void)
{
    // Activate GPIO ports (provide a clock) - p. 400 TIVA TM4C129XNCZAD
        MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_LCD0);

        // Pins used for LCD function must be configured as special - p. 806 TIVA TM4C129XNCZAD
        MAP_GPIOPinConfigure(GPIO_PF7_LCDDATA02);
        GPIOPinTypeLCD(GPIO_PORTF_BASE, LCD_GPIO_F);

        MAP_GPIOPinConfigure(GPIO_PJ2_LCDDATA14);
        MAP_GPIOPinConfigure(GPIO_PJ3_LCDDATA15);
        MAP_GPIOPinConfigure(GPIO_PJ4_LCDDATA16);
        MAP_GPIOPinConfigure(GPIO_PJ5_LCDDATA17);
        MAP_GPIOPinConfigure(GPIO_PJ6_LCDAC);
        GPIOPinTypeLCD(GPIO_PORTJ_BASE, LCD_GPIO_J);

        MAP_GPIOPinConfigure(GPIO_PN6_LCDDATA13);
        MAP_GPIOPinConfigure(GPIO_PN7_LCDDATA12);
        GPIOPinTypeLCD(GPIO_PORTN_BASE, LCD_GPIO_N);

        MAP_GPIOPinConfigure(GPIO_PR0_LCDCP);
        MAP_GPIOPinConfigure(GPIO_PR1_LCDFP);
        MAP_GPIOPinConfigure(GPIO_PR2_LCDLP);
        MAP_GPIOPinConfigure(GPIO_PR3_LCDDATA03);
        MAP_GPIOPinConfigure(GPIO_PR4_LCDDATA00);
        MAP_GPIOPinConfigure(GPIO_PR5_LCDDATA01);
        MAP_GPIOPinConfigure(GPIO_PR6_LCDDATA04);
        MAP_GPIOPinConfigure(GPIO_PR7_LCDDATA05);
        GPIOPinTypeLCD(GPIO_PORTR_BASE, LCD_GPIO_R);

        MAP_GPIOPinConfigure(GPIO_PS0_LCDDATA20);
        MAP_GPIOPinConfigure(GPIO_PS1_LCDDATA21);
        MAP_GPIOPinConfigure(GPIO_PS2_LCDDATA22);
        MAP_GPIOPinConfigure(GPIO_PS3_LCDDATA23);
        MAP_GPIOPinConfigure(GPIO_PS4_LCDDATA06);
        MAP_GPIOPinConfigure(GPIO_PS5_LCDDATA07);
        MAP_GPIOPinConfigure(GPIO_PS6_LCDDATA08);
        MAP_GPIOPinConfigure(GPIO_PS7_LCDDATA09);
        GPIOPinTypeLCD(GPIO_PORTS_BASE, LCD_GPIO_S);

        MAP_GPIOPinConfigure(GPIO_PT0_LCDDATA10);
        MAP_GPIOPinConfigure(GPIO_PT1_LCDDATA11);
        MAP_GPIOPinConfigure(GPIO_PT2_LCDDATA18);
        MAP_GPIOPinConfigure(GPIO_PT3_LCDDATA19);
        GPIOPinTypeLCD(GPIO_PORTT_BASE, LCD_GPIO_T);

        MAP_GPIOPinTypeGPIOOutput(DATAEN_GPIO_PORT, DATAEN_GPIO_MASK); /* Tiva DATA_EN Mask  */
        MAP_GPIOPinTypeGPIOOutput(VSYNC_GPIO_PORT, VSYNC_GPIO_MASK); /* Tiva VSYNC Mask  */

        MAP_GPIOPinWrite(DATAEN_GPIO_PORT, DATAEN_GPIO_MASK, DATAEN_GPIO_MASK); /* Pass DATA_EN thru */
        MAP_GPIOPinWrite(VSYNC_GPIO_PORT, VSYNC_GPIO_MASK, VSYNC_GPIO_MASK); /* Pass VSYNC thru */
}

static void Display_InitVideoTiming(void)
{
    // Configure video timing: p. 1893 TIVA TM4C129XNCZAD, videotiming.xls, p. 053 DLP3430 DLP-PICO Processor
        g_tTiming.ui32Flags =
            RASTER_TIMING_SYNCS_OPPOSITE_PIXCLK |
            RASTER_TIMING_ACTIVE_HIGH_OE        |
            RASTER_TIMING_ACTIVE_LOW_PIXCLK    |
            RASTER_TIMING_ACTIVE_HIGH_HSYNC     |
            RASTER_TIMING_ACTIVE_HIGH_VSYNC;
        g_tTiming.ui16PanelWidth     = DISP_WIDTH;
        g_tTiming.ui16PanelHeight    = DISP_HEIGHT;
        g_tTiming.ui16HFrontPorch    = DISP_HFP;
        g_tTiming.ui16HBackPorch     = DISP_HBP;
        g_tTiming.ui16HSyncWidth     = DISP_HSYNCW;
        g_tTiming.ui8VFrontPorch     = DISP_VFP;
        g_tTiming.ui8VBackPorch      = DISP_VBP;
        g_tTiming.ui8VSyncWidth      = DISP_VSYNCW;
        g_tTiming.ui8ACBiasLineCount = DISP_ACBIAS_COUNT;
}

static void Display_InitLCD(void)
{
#ifdef SIXTEEN_BPP
    g_frameBuffer1 = g_frameBuffer0 + g_frameBufferSz/2;
#else
    g_frameBuffer1 = g_frameBuffer0 + g_frameBufferSz/4;
#endif

    // Start configuring raster operation
    MAP_LCDModeSet(LCD0_BASE, LCD_MODE_RASTER, PIXEL_CLOCK, NIRSCAN_SYSCLK); // p. 1893 TIVA TM4C129XNCZAD
    MAP_LCDRasterTimingSet(LCD0_BASE, &g_tTiming); // p. 1891 TIVA TM4C129XNCZAD
#ifdef INTERNAL_FRAMEBUFFER //No ping pong; buffer increment managed in Display ISR
#ifdef SIXTEEN_BPP
    MAP_LCDRasterConfigSet(LCD0_BASE, 0x80 | RASTER_FMT_PASSIVE_COLOR_16BIT | RASTER_LOAD_DATA_ONLY, PAL_LOAD_DELAY); // p. 1887 TIVA TM4C129XNCZAD
#else
    MAP_LCDRasterConfigSet(LCD0_BASE, RASTER_FMT_ACTIVE_24BPP_UNPACKED | RASTER_LOAD_DATA_ONLY, PAL_LOAD_DELAY); // p. 1887 TIVA TM4C129XNCZAD
#endif
#else
#ifdef SIXTEEN_BPP
    MAP_LCDRasterConfigSet(LCD0_BASE, 0x80 | RASTER_FMT_PASSIVE_COLOR_16BIT | RASTER_LOAD_DATA_ONLY, PAL_LOAD_DELAY); // p. 1887 TIVA TM4C129XNCZAD
#else
    MAP_LCDRasterConfigSet(LCD0_BASE, RASTER_FMT_ACTIVE_24BPP_PACKED | RASTER_LOAD_DATA_ONLY, PAL_LOAD_DELAY); // p. 1887 TIVA TM4C129XNCZAD
#endif
#endif
    MAP_LCDDMAConfigSet(LCD0_BASE, LCD_DMA_BURST_16 | LCD_DMA_FIFORDY_64_WORDS | LCD_DMA_BYTE_ORDER_0123); // p. 1898 TIVA TM4C129XNCZAD

    MAP_LCDRasterFrameBufferSet(LCD0_BASE, 0, g_frameBuffer0, g_frameBufferSz); // p. 1900 TIVA TM4C129XNCZAD
    MAP_LCDRasterFrameBufferSet(LCD0_BASE, 1, g_frameBuffer1, g_frameBufferSz); // p. 1900 TIVA TM4C129XNCZAD

    MAP_LCDIntEnable(LCD0_BASE, ( LCD_INT_SYNC_LOST | LCD_INT_UNDERFLOW | LCD_INT_RASTER_FRAME_DONE/*LCD_INT_EOF0*/ )); // p. 1912 TIVA TM4C129XNCZAD

}
void Display_Disable()
/**
 * Disables LCD0 peripheral
 *
 * @return  None
 *
 */
{
	// Activate GPIO ports (provide a clock) - p. 400 TIVA TM4C129XNCZAD
	MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_LCD0);
}


int Display_GenCalibPatterns(CALIB_SCAN_TYPES scan_type)
/**
 * Calls the spectrum library API to generate patterns for factory calibrations.
 *
 * @param   scan_type -I- generate pattern API use this parameter to configure start, end ,width
 *                        and step of the pattern generated.
 *
 * @return  number of patterns generated.
 *
 */
{
	FrameBufferDescriptor fb;
	int num_patterns;

	fb.frameBuffer = (uint32_t *)g_frameBuffer0;
	fb.numFBs = NUM_FRAMEBUFFERS;
	fb.width = DISP_WIDTH;
	fb.height = DISP_HEIGHT;
#ifdef INTERNAL_FRAMEBUFFER
	fb.bpp = 32;
#else
#ifdef SIXTEEN_BPP
	fb.bpp = 16;
#else
	fb.bpp = 24;
#endif
#endif

	num_patterns = dlpspec_calib_genPatterns(scan_type, &fb);

	if(num_patterns > 0)
		Scan_SetNumPatternsToScan(num_patterns);

	return num_patterns;
}


int Display_GenScanPatterns(uScanConfig *pCfg)
/**
 * Calls the spectrum library API to generate patterns for scans. Pattern bending is also done
 * for optical distortion.
 *
 * @param   pCfg -I- scan configuration defines the scan type and various parameters such as start
 *                   and end wavelength and num of patterns which are used to configure start and end
 *                   pixel and the step.
 *
 * @return  number of patterns generated.
 *
 */
{
	FrameBufferDescriptor fb;
	calibCoeffs calib_coeffs; //SK: Think about memset to zero
	int numPatterns;

	fb.frameBuffer = (uint32_t *)g_frameBuffer0;
	fb.numFBs = NUM_FRAMEBUFFERS;
	fb.width = DISP_WIDTH;
	fb.height = DISP_HEIGHT;
#ifdef INTERNAL_FRAMEBUFFER
	fb.bpp = 32;
#else
#ifdef SIXTEEN_BPP
	fb.bpp = 16;
#else
	fb.bpp = 24;
#endif
#endif

	Nano_eeprom_GetcalibCoeffs(&calib_coeffs);
	numPatterns = dlpspec_scan_genPatterns(pCfg, &calib_coeffs, &fb);
#ifndef NO_PATTERN_BENDING
	if(dlpspec_scan_bendPatterns(&fb, &calib_coeffs, numPatterns) != PASS)
		return -1;
#endif
	return numPatterns;

}

void Display_SetFrameBufferAtBeginning()
/**
 * Reset the LCD to stream first frame buffer.
 *
 * @return  None
 *
 */
{
	//Actual Frame buffer change will happen in the next vysnc interrupt
	g_fullFrameCount = 0;
	vsyncCount = 0;
	g_FrameFlipVsyncCount = 0;
}

int Display_FramePropagationWait(void)
/**
 * Waits the required time for the frames to propagate from Tiva output to display on DMD
 *
 * @return  PASS or FAIL (if wait on vsyncs timedout)
 *
 */
{
	/* Need a total of 2 frames delay to allow frames in pipeline to be actually
	 * displayed on the DMD  */
	int timeoutCounter = TIMEOUT_COUNTER;

	#ifdef INTERNAL_FRAMEBUFFER
	while ((vsyncCount<2) && (--timeoutCounter))
	{

	}
	#else
	/* One additional frame delay in case of full frame streaming because
	 * when we change DMA frame buffer pointer in Vsync interrrupt, it takes effect
	 * only during the next vsync
	 * Plus an additional one frame wait after first power up of Tiva/ after Display_Init()
	 */
	while(vsyncCount < PATTERN_DISPLAY_DELAY_NUM_FRAMES)
	{
		if(--timeoutCounter == 0)
			break;
	}
	#endif
	if(timeoutCounter == 0)
		return FAIL;
	else
		return PASS;
}
