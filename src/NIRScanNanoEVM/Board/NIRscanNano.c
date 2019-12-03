/*
 *
 *  This file is responsible for setting up the board specific items for the
 *  NIRscanNano board.
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <inc/tm4c129xnczad.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>

#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/gpio.h>
#include <driverlib/flash.h>
#include <driverlib/i2c.h>
#include <driverlib/uart.h>
#include <driverlib/ssi.h>
#include <driverlib/udma.h>
#include <driverlib/hibernate.h>
#include <driverlib/adc.h>
#include <driverlib/epi.h>


#include <xdc/std.h>
#include <xdc/cfg/global.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include "GPIO Mapping.h"
#include "nnoStatus.h"
#include "common.h"
#include "NIRscanNano.h"
#include "adcWrapper.h"
#include "tmp006.h"
#include "sdram.h"
#include "led.h"
#include "flash.h"

#ifndef TI_DRIVERS_UART_DMA
#define TI_DRIVERS_UART_DMA 0
#endif

/*
 *  =============================== DMA ===============================
 */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(NIRscanNano_DMAControlTable, 1024)
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=1024
#elif defined(__GNUC__)
__attribute__ ((aligned (1024)))
#endif
static tDMAControlTable NIRscanNano_DMAControlTable[32];
static bool DMA_initialized = false;

/* Hwi_Struct used in the initDMA Hwi_construct call */
static Hwi_Struct hwiStruct;

/*
 *  ======== NIRscanNano_errorDMAHwi ========
 */
static Void NIRscanNano_errorDMAHwi(UArg arg)
{
	DEBUG_PRINT("DMA error code: %d\n", uDMAErrorStatusGet());
    uDMAErrorStatusClear();
    System_abort("DMA error!!");
}

/*
 *  ======== NIRscanNano_initDMA ========
 */
void NIRscanNano_initDMA(void)
{
    Error_Block eb;
    Hwi_Params  hwiParams;

    if (!DMA_initialized)
    {
        Error_init(&eb);

        Hwi_Params_init(&hwiParams);
        Hwi_construct(&(hwiStruct), INT_UDMAERR, NIRscanNano_errorDMAHwi,
                      &hwiParams, &eb);
        if (Error_check(&eb))
        {
            System_abort("Couldn't construct DMA error hwi");
        }

        SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
        uDMAEnable();
        uDMAControlBaseSet(NIRscanNano_DMAControlTable);

        DMA_initialized = true;
    }
}


/*
 *  =============================== NIRscan Nano DLPC150 ===============================
 */
int NIRscanNano_DLPCEnable(bool enable)
{
	int timeoutCounter = DLPC_ENABLE_MAX_DELAY;
	if(enable)
	{
		MAP_GPIOPinWrite( GPIO_PORTJ_BASE, GPIO_PIN_7, GPIO_PIN_7 );
		while ( (MAP_GPIOPinRead( GPIO_PORTQ_BASE, GPIO_PIN_7 ) == 0x80 ) && (timeoutCounter--));

		//DLPC150 did not boot
		if(timeoutCounter <=0)
		{
			nnoStatus_setErrorStatusAndCode(NNO_ERROR_SCAN, true, NNO_ERROR_SCAN_DLPC150_BOOT_ERROR);
			return FAIL;
		}
	}
	else
	{
		MAP_GPIOPinWrite( GPIO_PORTJ_BASE, GPIO_PIN_7, 0 );
	}
	return PASS;
}

/*
 *  =============================== NIRscan Nano Lamp Driver ============================
 */
int NIRscanNano_LampEnable(bool enable)
{
	if(enable)
	{
		MAP_GPIOPinWrite( LAMP_GPIO_PORT, LAMP_GPIO_MASK, LAMP_GPIO_MASK ); // Turn on lamp
	}
	else
	{
		MAP_GPIOPinWrite( LAMP_GPIO_PORT, LAMP_GPIO_MASK, 0 ); // Turn off lamp
	}

	/*
	* Review comment - SK - Ensure there is a error code in scan.c to flag when photo detector value is low.
	* Does it make sense to look at temp chip and or Adc intesities to really 
	* find if lamp turned on? Just detecting Pass or FAIL?
	* we always return this function thinking lamp is ON or OFF
	* or we can have API to check NIRscanNano_IsLampON better fault detection mechanism
	*/
	
	return PASS;
}

/*
 *  =============================== NIRscan Nano ADC ===============================
 */
int NIRscanNano_initADC(void)
{
  // Pin  	Function 	Type       Init Value	Int         Schematic Name	Connection
  /* PD4    AIN7		IN 									BAT_V_SW        DMC2990  
   */
	MAP_SysCtlPeripheralEnable( SYSCTL_PERIPH_ADC0 );			// Enable clock to ADC
	
    MAP_GPIOPinTypeADC( GPIO_PORTD_BASE, GPIO_PIN_4 );		// Set PD4 to AIN7
	MAP_ADCSequenceDisable(ADC0_BASE, 3);					// Disable sequence #3, to reprogram it
	MAP_ADCHardwareOversampleConfigure(ADC0_BASE, 16);		// Average 8 values
	MAP_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);	// Set Sequence to Processor trigger at highest priority (0)	
	// Configure Sample Sequence #3 with 1 step: Single-ended, No temp sensor, AIN7, interrupt, end of sequence
	MAP_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH7 | ADC_CTL_IE | ADC_CTL_END); 

	MAP_ADCSequenceDisable(ADC0_BASE, 1);					// Disable sequence #1, to reprogram it
	MAP_ADCSequenceConfigure(ADC0_BASE, 1, ADC_TRIGGER_PROCESSOR, 1);	// Set Sequence to Processor trigger at next highest priority (1)
	// Configure Sample Sequence #1 with 1 step: Internal temp sensor,  interrupt, end of sequence
	MAP_ADCSequenceStepConfigure(ADC0_BASE, 1, 0, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END); 
	
	MAP_SysCtlPeripheralDisable( SYSCTL_PERIPH_ADC0 );			// Sleep ADC until needed

	return PASS;
}

/*
 *  =============================== General ===============================
 */
/*
 *  ======== NIRscanNano_initGeneral ========
 */
void NIRscanNano_initGeneral(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOR);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOS);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOT);
}

/*
 *  =============================== GPIO ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(GPIO_config, ".const:GPIO_config")
#pragma DATA_SECTION(gpioHWAttrs, ".const:gpioHWAttrs")
#endif

#include <ti/drivers/GPIO.h>

/* Callback functions for the GPIO interrupt example. */
void gpioButtonFxn0(void);
void gpioButtonFxn1(void);

/* GPIO configuration structure */
const GPIO_HWAttrs gpioHWAttrs[NIRSCANNANO_GPIOCOUNT] = {
#ifdef GREEN_LED_TIVA_BOARD
    {GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_OUTPUT}, /* NIRscanNano_LED_Green   */
#endif
#ifdef GREEN_LED_EXP_CONN
    {GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_OUTPUT}, /* NIRscanNano_LED_Green   */
#endif

#ifdef BLUE_LED_TIVA_BOARD
    {GPIO_PORTH_BASE, GPIO_PIN_7, GPIO_OUTPUT}, /* NIRscanNano_LED_Blue    */
#endif
#ifdef BLUE_LED_EXP_CONN
    {GPIO_PORTK_BASE, GPIO_PIN_2, GPIO_OUTPUT}, /* NIRscanNano_LED_Blue    */
#endif

#ifdef YELLOW_LED_TIVA_BOARD
    {GPIO_PORTL_BASE, GPIO_PIN_4, GPIO_OUTPUT}, /* NIRscanNano_LED_Yellow  */
#endif
#ifdef YELLOW_LED_EXP_CONN
    {GPIO_PORTA_BASE, GPIO_PIN_4, GPIO_OUTPUT}, /* NIRscanNano_LED_Yellow  */
#endif

#ifdef SCAN_BUTTON_TIVA_BOARD
    {GPIO_PORTQ_BASE, GPIO_PIN_3, GPIO_INPUT},  /* NIRscanNano_BUTTON_Scan */
#endif
#ifdef SCAN_BUTTON_EXP_CONN
    {GPIO_PORTK_BASE, GPIO_PIN_3, GPIO_INPUT},  /* NIRscanNano_BUTTON_Scan */
#endif
};

/* Memory for the GPIO module to construct a Hwi */
#ifdef SCAN_BUTTON_TIVA_BOARD
Hwi_Struct callbackPortQHwi;
#endif
#ifdef SCAN_BUTTON_EXP_CONN
Hwi_Struct callbackPortKHwi;
#endif


#ifdef SCAN_BUTTON_TIVA_BOARD
const GPIO_Callbacks NIRscanNano_gpioPortQCallbacks = {
   GPIO_PORTQ_BASE, INT_GPIOQ3, &callbackPortQHwi,
#endif
#ifdef SCAN_BUTTON_EXP_CONN
const GPIO_Callbacks NIRscanNano_gpioPortKCallbacks = {
   GPIO_PORTK_BASE, INT_GPIOQ3, &callbackPortKHwi,
#endif
    {NULL, NULL, NULL, gpioButtonFxn0, NULL, NULL, NULL, NULL}
};

const GPIO_Config GPIO_config[] = {
    {&gpioHWAttrs[0]},
    {&gpioHWAttrs[1]},
    {&gpioHWAttrs[2]},
    {&gpioHWAttrs[3]},
    {NULL},
};

/*
 *  ======== NIRscanNano_initGPIO ========
 */
void NIRscanNano_initGPIO(void)
{
   // Enable Hybernation Peripheral
   MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
   
  // Setup the LED GPIO pins used 
  // Pin  	Function 	Type       Init Value	Int         Schematic Name	Connection
  /* PF5	GPIO 		OUT 		HIGH					On/Off			Green LED
   * PA5    GPIO        OUT         HIGH                    J3 pin 6        Green LED option
   * PH7	GPIO 		OUT 		HIGH					O_Bluetooth		Blue LED
   * PK2	GPIO 		OUT 		HIGH					J3 pin 7		Blue LED option
   * PL4	GPIO 		OUT 		HIGH					Yellow LED		O_Scan
   * PA4	GPIO 		OUT 		HIGH					J3 pin 3		O_Scan option
   */
   
   // GREEN LED
#ifdef GREEN_LED_TIVA_BOARD
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_5); /* NIRscanNano_LED_Green  */
#endif
#ifdef GREEN_LED_EXP_CONN
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_5); /* NIRscanNano_LED_Green  */
#endif

	// BLUE LED
#ifdef BLUE_LED_TIVA_BOARD
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_7); /* NIRscanNano_LED_Blue   */
#endif
#ifdef BLUE_LED_EXP_CONN
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, GPIO_PIN_2); /* NIRscanNano_LED_Blue   */
#endif

	// YELLOW LED
#ifdef YELLOW_LED_TIVA_BOARD
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE, GPIO_PIN_4); /* NIRscanNano_LED_Yellow */
#endif    
#ifdef YELLOW_LED_EXP_CONN
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_4); /* NIRscanNano_LED_Yellow */
#endif    
   
  // Setup other GPIO pins 
  // Pin  	Function 	Type       Init Value	Int         Schematic Name	Connection
  /* PB2	GPIO		Out			LOW		                P1P8V_EN		TPS22904 
   * PD2    GPIO        Out         LOW						TIVA_EXP_I2C_SCL Lamp Power
   * PD3    GPIO        Out         LOW                     TIVA_EXP_I2C_SDA DLPC150 TRIG_IN
   * PD4    AIN7		IN 									BAT_V_SW        DMC2990
   * PD5	GPIO 		OUT			LOW						BAT_VSENSE_EN	DMC2990			
   * PD7	GPIO 		OUT			HIGH 					TIVA_TRIG_SEL	J5 Connector							
   * PE0	GPIO 		OUT 		HIGH					TIVA_DATAEN_MASK SN74ALVC08		
   * PE1	GPIO 		OUT 		HIGH					TIVA_VSYNC_MASK	SN74ALVC08
   * PE2	GPIO 		OUT 		LOW						TIVA_TRIG_IN_0
   * PE3	GPIO 		OUT			LOW 					TIVA_TRIG_IN_1				
   * PH5	GPIO 		OUT 		LOW						BT_nSHUTD		CC2564
   * PH6 	GPIO 		OUT 		LOW						SYNCZ			ADS1255
   * PJ7	GPIO 		OUT 	    LOW						PROJ_ON	
   * PP0	GPIO 		IN 						RISING EDGE  DLPC_XLAT0_TIVA	DLPC150 GPIO_4 (FRAME)
   * PP1	GPIO 		IN 						FALLING EDGE DLPC_XLAT1_TIVA DLPC150 GPIO_16  (PATTERN)
   * PP2	GPIO 		IN 						FALLING EDGE	DRDY		ADS1255
   * PP6 	GPIO 		IN  					FALLING EDGE	TDRDYZ		TMP006
   * PP7	GPIO		IN						FALLING EDGE	HUM_DRDYz	HDC1000
   * PQ0	GPIO 		OUT 		LOW						BC_EN2
   * PQ1	GPIO 		OUT			HIGH					BC_EN1
   * PQ2	GPIO 		OUT			LOW						BC_CSZ
   * PQ3	GPIO 		IN 						FALLING EDGE SW_SCAN		Scan Button
   * PQ4	GPIO 		IN						FALLING EDGE mSD_CARD_DET 	Card Detect
   * PQ5	GPIO 		IN			HIGH		FALLING EDGE BC_INT
   * PQ6 	GPIO 		IN 						FALLING EDGE PAD_RSTz
   * PQ7	GPIO		IN						FALLING EDGE DLPC_HOST_IRQ		
   */
    MAP_GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_2, 0 );
    MAP_GPIOPinTypeGPIOOutput( GPIO_PORTB_BASE, GPIO_PIN_2 );
    
    MAP_GPIOPinTypeGPIOOutput( GPIO_PORTD_BASE,  GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_7);
    MAP_GPIOPinWrite( GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_5 | GPIO_PIN_7, GPIO_PIN_7 );

    MAP_GPIOPinWrite( GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
                                    GPIO_PIN_0 | GPIO_PIN_1 );
	MAP_GPIOPinTypeGPIOOutput( GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );

    MAP_GPIOPinWrite( GPIO_PORTH_BASE, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_7 );
      /* Configure the CC2564 Shutdown Pin.                                       */
	MAP_GPIOPinWrite( GPIO_PORTH_BASE, GPIO_PIN_5, 0 );
	MAP_GPIOPinTypeGPIOOutput( GPIO_PORTH_BASE, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

    MAP_GPIOPinWrite( GPIO_PORTJ_BASE, GPIO_PIN_7, 0 );
	MAP_GPIOPinTypeGPIOOutput( GPIO_PORTJ_BASE, GPIO_PIN_7 );

	MAP_GPIOPinTypeGPIOInput( GPIO_PORTP_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7 );
	MAP_GPIOIntTypeSet( GPIO_PORTP_BASE, GPIO_PIN_0, GPIO_RISING_EDGE | GPIO_DISCRETE_INT );
	MAP_GPIOIntTypeSet( GPIO_PORTP_BASE, GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_FALLING_EDGE | GPIO_DISCRETE_INT );

    MAP_GPIOPinWrite( GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2, 0 );
	MAP_GPIOPinWrite( GPIO_PORTQ_BASE, GPIO_PIN_1, GPIO_PIN_1 );
	MAP_GPIOPinTypeGPIOOutput( GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 );

#ifdef SCAN_BUTTON_TIVA_BOARD
	MAP_GPIOPinTypeGPIOInput( GPIO_PORTQ_BASE, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 );
	MAP_GPIOIntTypeSet( GPIO_PORTQ_BASE, GPIO_PIN_3 | GPIO_PIN_4, GPIO_BOTH_EDGES | GPIO_DISCRETE_INT );
	MAP_GPIOIntEnable( GPIO_PORTQ_BASE, GPIO_PIN_3);
#endif

#ifdef SCAN_BUTTON_EXP_CONN
	MAP_GPIOPinTypeGPIOInput( GPIO_PORTQ_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 );
	MAP_GPIOIntTypeSet( GPIO_PORTQ_BASE, GPIO_PIN_4, GPIO_BOTH_EDGES | GPIO_DISCRETE_INT );
	
	MAP_GPIOPinTypeGPIOInput( GPIO_PORTK_BASE, GPIO_PIN_3 );	
	MAP_GPIOIntTypeSet( GPIO_PORTK_BASE, GPIO_PIN_3, GPIO_BOTH_EDGES );
	MAP_GPIOIntEnable( GPIO_PORTK_BASE, GPIO_PIN_3);
#endif

	MAP_GPIOIntTypeSet( GPIO_PORTQ_BASE, GPIO_PIN_5 | GPIO_PIN_6 , GPIO_FALLING_EDGE | GPIO_DISCRETE_INT );
	MAP_GPIOIntTypeSet( GPIO_PORTQ_BASE, GPIO_PIN_7 , GPIO_RISING_EDGE | GPIO_DISCRETE_INT );

#ifdef SLEW_SCAN_DEBUG
	HWREG(GPIO_PORTC_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
	HWREG(GPIO_PORTC_BASE + GPIO_O_CR) = GPIO_PIN_0 | GPIO_PIN_1;
    MAP_GPIOPinWrite( GPIO_PORTC_BASE, GPIO_PIN_0 | GPIO_PIN_1, 0 );
	MAP_GPIOPinTypeGPIOOutput( GPIO_PORTC_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
#endif

    /* Once GPIO_init is called, GPIO_config cannot be changed */
    GPIO_init();
}

/*
 *  =============================== I2C ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(I2C_config, ".const:I2C_config")
#pragma DATA_SECTION(i2cTivaHWAttrs, ".const:i2cTivaHWAttrs")
#endif

#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CTiva.h>

/* I2C objects */
I2CTiva_Object i2cTivaObjects[NIRSCANNANO_I2CCOUNT];

/* I2C configuration structure, describing which pins are to be used */
const I2CTiva_HWAttrs i2cTivaHWAttrs[NIRSCANNANO_I2CCOUNT] = {
    {I2C2_BASE, INT_I2C2},
    {I2C6_BASE, INT_I2C6},
    {I2C7_BASE, INT_I2C7},
    {I2C8_BASE, INT_I2C8},
    {I2C9_BASE, INT_I2C9}

};

const I2C_Config I2C_config[] = {
    {&I2CTiva_fxnTable, &i2cTivaObjects[0], &i2cTivaHWAttrs[0]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[1], &i2cTivaHWAttrs[1]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[2], &i2cTivaHWAttrs[2]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[3], &i2cTivaHWAttrs[3]},
    {&I2CTiva_fxnTable, &i2cTivaObjects[4], &i2cTivaHWAttrs[4]},
    {NULL, NULL, NULL}
};

/*
 *  ======== NIRscanNano_initI2C ========
 */
void NIRscanNano_initI2C(void)
{
    /* I2C2 Init */
    /* Enable the peripheral */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C2);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    MAP_GPIOPinConfigure( GPIO_PG2_I2C2SCL );
    MAP_GPIOPinConfigure( GPIO_PG3_I2C2SDA );
    MAP_GPIOPinTypeI2CSCL( GPIO_PORTG_BASE, GPIO_PIN_2 );
    MAP_GPIOPinTypeI2C( GPIO_PORTG_BASE, GPIO_PIN_3 );

    /* I2C6 Init */
    /* Enable the peripheral */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C6);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    MAP_GPIOPinConfigure( GPIO_PB6_I2C6SCL );
    MAP_GPIOPinConfigure( GPIO_PB7_I2C6SDA );
    MAP_GPIOPinTypeI2CSCL( GPIO_PORTB_BASE, GPIO_PIN_6 );
    MAP_GPIOPinTypeI2C( GPIO_PORTB_BASE, GPIO_PIN_7 );

    /* I2C7 Init */
    /* Enable the peripheral */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C7);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    MAP_GPIOPinConfigure( GPIO_PD0_I2C7SCL );
    MAP_GPIOPinConfigure( GPIO_PD1_I2C7SDA );
     MAP_GPIOPinTypeI2CSCL( GPIO_PORTD_BASE, GPIO_PIN_0 );
    MAP_GPIOPinTypeI2C( GPIO_PORTD_BASE, GPIO_PIN_1 );

    /* I2C9 Init */
    /* Enable the peripheral */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C9);

    /* Configure the appropriate pins to be I2C instead of GPIO. */
    MAP_GPIOPinConfigure( GPIO_PA0_I2C9SCL );
    MAP_GPIOPinConfigure( GPIO_PA1_I2C9SDA );
    MAP_GPIOPinTypeI2CSCL( GPIO_PORTA_BASE, GPIO_PIN_0 );
    MAP_GPIOPinTypeI2C( GPIO_PORTA_BASE, GPIO_PIN_1 );

    I2C_init();
	MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C2);
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C6);
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C7);
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_I2C9);
}

/*
 *  =============================== SDSPI ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(SDSPI_config, ".const:SDSPI_config")
#pragma DATA_SECTION(sdspiTivaHWattrs, ".const:sdspiTivaHWattrs")
#endif

#include <ti/drivers/SDSPI.h>
#include <ti/drivers/sdspi/SDSPITiva.h>

/* SDSPI objects */
SDSPITiva_Object sdspiTivaobjects[NIRSCANNANO_SDSPICOUNT];

/* SDSPI configuration structure, describing which pins are to be used */
const SDSPITiva_HWAttrs sdspiTivaHWattrs[NIRSCANNANO_SDSPICOUNT] = {
	{
        SSI3_BASE,          /* SPI base address */

        GPIO_PORTF_BASE,    /* SPI SCK PORT */
        GPIO_PIN_3,        		 /* SCK PIN */
        GPIO_PORTF_BASE,    /* SPI MISO PORT*/
        GPIO_PIN_0,         		/* MISO PIN */
        GPIO_PORTF_BASE,    /* SPI MOSI PORT */
        GPIO_PIN_1,         		/* MOSI PIN */
        GPIO_PORTF_BASE,    /* GPIO CS PORT */
        GPIO_PIN_2,        		 /* CS PIN */

    }
};

const SDSPI_Config SDSPI_config[] = {
    {&SDSPITiva_fxnTable, &sdspiTivaobjects[0], &sdspiTivaHWattrs[0]},
    {NULL, NULL, NULL}
};

/*
 *  ======== NIRSCANNANO_initSDSPI ========
 */
void NIRscanNano_initSDSPI(void)
{
    /* Enable the peripherals used by the SD Card */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

    /* Configure pad settings */
    MAP_GPIOPadConfigSet(GPIO_PORTF_BASE,
            GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
            GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD);

    MAP_GPIOPadConfigSet(GPIO_PORTF_BASE,
            GPIO_PIN_0,
            GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
            
    MAP_GPIOPinConfigure( GPIO_PF0_SSI3XDAT1 );
    MAP_GPIOPinConfigure( GPIO_PF1_SSI3XDAT0 );
    MAP_GPIOPinConfigure( GPIO_PF3_SSI3CLK );

    SDSPI_init();
}

/*
 *  =============================== SPI ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(SPI_config, ".const:SPI_config")
#pragma DATA_SECTION(spiTivaDMAHWAttrs, ".const:spiTivaDMAHWAttrs")
#endif

#include <ti/drivers/SPI.h>
#include <ti/drivers/spi/SPITivaDMA.h>

/* SPI objects */
SPITivaDMA_Object spiTivaDMAobjects[NIRSCANNANO_SPICOUNT];
#if defined(ccs)
#pragma DATA_ALIGN(spiTivaDMAscratchBuf, 32)
#elif defined(ewarm)
#pragma data_alignment=32
#elif defined(gcc)
__attribute__ ((aligned (32)))
#endif
uint32_t spiTivaDMAscratchBuf[NIRSCANNANO_SPICOUNT];

/* SPI configuration structure, describing which pins are to be used */
const SPITivaDMA_HWAttrs spiTivaDMAHWAttrs[NIRSCANNANO_SPICOUNT] = {
     {
        SSI1_BASE,
        INT_SSI1,
        &spiTivaDMAscratchBuf[1],
        0,
        UDMA_CHANNEL_SSI1RX,
        UDMA_CHANNEL_SSI1TX,
        uDMAChannelAssign,
        UDMA_CH24_SSI1RX,
        UDMA_CH25_SSI1TX
    },
   {
        SSI2_BASE,
        INT_SSI2,
        &spiTivaDMAscratchBuf[2],
        0,
        UDMA_SEC_CHANNEL_UART2RX_12,
        UDMA_SEC_CHANNEL_UART2TX_13,
        uDMAChannelAssign,
        UDMA_CH12_SSI2RX,
        UDMA_CH13_SSI2TX
    },
    {
        SSI3_BASE,
        INT_SSI3,
        &spiTivaDMAscratchBuf[3],
        0,
        UDMA_SEC_CHANNEL_TMR2A_14,
        UDMA_SEC_CHANNEL_TMR2B_15,
        uDMAChannelAssign,
        UDMA_CH14_SSI3RX,
        UDMA_CH15_SSI3TX
    }
};

const SPI_Config SPI_config[] = {
    {&SPITivaDMA_fxnTable, &spiTivaDMAobjects[0], &spiTivaDMAHWAttrs[0]},
    {&SPITivaDMA_fxnTable, &spiTivaDMAobjects[1], &spiTivaDMAHWAttrs[1]},
    {&SPITivaDMA_fxnTable, &spiTivaDMAobjects[2], &spiTivaDMAHWAttrs[2]},
    {&SPITivaDMA_fxnTable, &spiTivaDMAobjects[3], &spiTivaDMAHWAttrs[3]},
    {NULL, NULL, NULL},
};

/*
 *  ======== NIRscanNano_initSPI ========
 */
void NIRscanNano_initSPI(void)
{
    /* SSI1 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
    
    MAP_GPIOPinConfigure( GPIO_PB4_SSI1FSS );
    MAP_GPIOPinConfigure( GPIO_PB5_SSI1CLK );
    MAP_GPIOPinConfigure( GPIO_PE5_SSI1XDAT1 );
    MAP_GPIOPinConfigure( GPIO_PE4_SSI1XDAT0 );

	MAP_GPIOPinTypeSSI( GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5 );
	MAP_GPIOPinTypeSSI( GPIO_PORTB_BASE, GPIO_PIN_4 | GPIO_PIN_5 );

    /* SSI2 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);

    MAP_GPIOPinConfigure( GPIO_PG4_SSI2XDAT1 );
    MAP_GPIOPinConfigure( GPIO_PG5_SSI2XDAT0 );
    MAP_GPIOPinConfigure( GPIO_PG6_SSI2FSS );
    MAP_GPIOPinConfigure( GPIO_PG7_SSI2CLK );

	MAP_GPIOPinTypeSSI( GPIO_PORTG_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 );

    /* SSI3 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);

    MAP_GPIOPinConfigure( GPIO_PF0_SSI3XDAT1 );
    MAP_GPIOPinConfigure( GPIO_PF1_SSI3XDAT0 );
    MAP_GPIOPinConfigure( GPIO_PF2_SSI3FSS );
    MAP_GPIOPinConfigure( GPIO_PF3_SSI3CLK );

	MAP_GPIOPinTypeSSI( GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 );

    NIRscanNano_initDMA();
    SPI_init();
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_SSI2);
    MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_SSI3);
}

/*
 *  =============================== UART ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(UART_config, ".const:UART_config")
#pragma DATA_SECTION(uartTivaHWAttrs, ".const:uartTivaHWAttrs")
#endif

#include <ti/drivers/UART.h>
#if TI_DRIVERS_UART_DMA
#include <ti/drivers/uart/UARTTivaDMA.h>

/* UART objects */
UARTTiva_Object uartTivaObjects[NIRscanNano_UARTCOUNT];

/* UART configuration structure */
const UARTTivaDMA_HWAttrs uartTivaHWAttrs[NIRscanNano_UARTCOUNT] = {
    {
        UART3_BASE,    
        INT_UART3,
        UDMA_CH16_UART3RX,
        UDMA_CH17_UART3TX,
    },
};

const UART_Config UART_config[] = {
    {
        &UARTTivaDMA_fxnTable,
        &uartTivaObjects[0],
        &uartTivaHWAttrs[0]
    },
    {NULL, NULL, NULL}
};

#else
#include <ti/drivers/uart/UARTTiva.h>

/* UART objects */
UARTTiva_Object uartTivaObjects[NIRSCANNANO_UARTCOUNT];

/* UART configuration structure */
const UARTTiva_HWAttrs uartTivaHWAttrs[NIRSCANNANO_UARTCOUNT] = {
    {UART3_BASE, INT_UART3}, /* NIRscanNano_UART3 */
};

const UART_Config UART_config[] = {
    {
        &UARTTiva_fxnTable,
        &uartTivaObjects[0],
        &uartTivaHWAttrs[0]
    },
    {NULL, NULL, NULL}
};
#endif /* TI_DRIVERS_UART_DMA */

/*
 *  ======== NIRscanNano_initUART ========
 */
void NIRscanNano_initLampGPIO()
{
    /* Configure Lamp enable pin as GPIO output */
	MAP_GPIOPinTypeGPIOOutput( LAMP_GPIO_PORT, LAMP_GPIO_MASK );
    MAP_GPIOPinWrite( LAMP_GPIO_PORT, LAMP_GPIO_MASK, 0 ); // Turn off lamp
}

/*
 *  ======== NIRscanNano_initUART ========
 */
void NIRscanNano_initUART()
{
    /* Enable and configure the peripherals used by the uart. */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
	MAP_GPIOPinConfigure( GPIO_PJ0_U3RX );
    MAP_GPIOPinConfigure( GPIO_PJ1_U3TX );
	MAP_GPIOPinConfigure(GPIO_PP4_U3RTS );
	MAP_GPIOPinConfigure(GPIO_PP5_U3CTS );
    MAP_GPIOPinTypeUART( GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1 );
    MAP_GPIOPinTypeUART( GPIO_PORTP_BASE, GPIO_PIN_4 | GPIO_PIN_5 );

	 /*
	  * Initialize uart console or command interface if required
	  */
#if defined(ENABLE_UART_COMMAND_INTERFACE) || (defined(DEBUG_MSGS) && (LOG_PORT == UART_CONSOLE))
    MAP_SysCtlPeripheralEnable(CONSOLE_UART_SYSCTL);
    MAP_GPIOPinConfigure(CONSOLE_GPIO_RX);
    MAP_GPIOPinConfigure(CONSOLE_GPIO_TX);
  #ifdef BLUE_LED_TIVA_BOARD
    MAP_GPIOPinConfigure(GPIO_PK2_U4RTS);
  #endif
  #ifdef SCAN_BUTTON_TIVA_BOARD
    MAP_GPIOPinConfigure(GPIO_PK3_U4CTS);
  #endif
    MAP_GPIOPinTypeUART(CONSOLE_GPIO_BASE, CONSOLE_GPIO_PINS);
  #if defined(BLUE_LED_TIVA_BOARD) && defined(SCAN_BUTTON_TIVA_BOARD)
    MAP_GPIOPinTypeUART(CONSOLE_GPIO_HS_BASE, CONSOLE_GPIO_HS_PINS);
  #else
	#ifdef BLUE_LED_TIVA_BOARD
    	MAP_GPIOPinTypeUART(CONSOLE_GPIO_HS_BASE, GPIO_PIN_2);
	#else
		#ifdef SCAN_BUTTON_TIVA_BOARD
			    MAP_GPIOPinTypeUART(CONSOLE_GPIO_HS_BASE, GPIO_PIN_3);
		#endif
	#endif
  #endif
#else
  #ifndef YELLOW_LED_EXP_CONN
  	MAP_GPIOPinTypeGPIOOutput( GPIO_PORTA_BASE, GPIO_PIN_4 );
  	MAP_GPIOPinWrite( GPIO_PORTA_BASE, GPIO_PIN_4, 0 );
  #endif
#endif




    /* Initialize the UART driver */
#if TI_DRIVERS_UART_DMA
    NIRscanNano_initDMA();
#endif
   UART_init();
   MAP_SysCtlPeripheralDisable(SYSCTL_PERIPH_UART3);
}

/*
 *  =============================== USB ===============================
 */
/*
 *  ======== NIRscanNano_initUSB ========
 *  This function just turns on the USB
 */
void NIRscanNano_initUSB(void)
{
    /* Enable the USB peripheral and PLL */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    MAP_SysCtlUSBPLLEnable();

    /* Setup pins for USB operation */
    MAP_GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
}

/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(USBMSCHFatFs_config, ".const:USBMSCHFatFs_config")
#pragma DATA_SECTION(usbmschfatfstivaHWAttrs, ".const:usbmschfatfstivaHWAttrs")
#endif

/*
 *  =============================== USBMSCHFatFs ===============================
 */
#include <ti/drivers/USBMSCHFatFs.h>
#include <ti/drivers/usbmschfatfs/USBMSCHFatFsTiva.h>

/* USBMSCHFatFs objects */
USBMSCHFatFsTiva_Object usbmschfatfstivaObjects[NIRSCANNANO_USBMSCHFATFSCOUNT];

/* USBMSCHFatFs configuration structure, describing which pins are to be used */
const USBMSCHFatFsTiva_HWAttrs usbmschfatfstivaHWAttrs[NIRSCANNANO_USBMSCHFATFSCOUNT] = {
    {INT_USB0}
};

const USBMSCHFatFs_Config USBMSCHFatFs_config[] = {
    {
        &USBMSCHFatFsTiva_fxnTable,
        &usbmschfatfstivaObjects[0],
        &usbmschfatfstivaHWAttrs[0]
    },
    {NULL, NULL, NULL}
};

/*
 *  ======== NIRscanNano_initUSBMSCHFatFs ========
 */
void NIRscanNano_initUSBMSCHFatFs(void)
{
    /* Initialize the DMA control table */
    NIRscanNano_initDMA();

    /* Call the USB initialization function for the USB Reference modules */
    NIRscanNano_initUSB();
    USBMSCHFatFs_init();
}


/*
 *  =============================== Watchdog ===============================
 */
/* Place into subsections to allow the TI linker to remove items properly */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Watchdog_config, ".const:Watchdog_config")
#pragma DATA_SECTION(watchdogTivaHWAttrs, ".const:watchdogTivaHWAttrs")
#endif

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/watchdog/WatchdogTiva.h>

/* Watchdog objects */
WatchdogTiva_Object watchdogTivaObjects[NIRSCANNANO_WATCHDOGCOUNT];

/* Watchdog configuration structure */
const WatchdogTiva_HWAttrs watchdogTivaHWAttrs[NIRSCANNANO_WATCHDOGCOUNT] =
{
    /* EK_LM4F120XL_WATCHDOG0 with 1 sec period at default CPU clock freq */
    {WATCHDOG0_BASE, INT_WATCHDOG, 80000000},
};

const Watchdog_Config Watchdog_config[] =
{
    {&WatchdogTiva_fxnTable, &watchdogTivaObjects[0], &watchdogTivaHWAttrs[0]},
    {NULL, NULL, NULL},
};

/*
 *  ======== NIRscanNano_initWatchdog ========
 *
 * NOTE: To use the other watchdog timer with base address WATCHDOG1_BASE,
 *       an additional function call may need be made to enable PIOSC. Enabling
 *       WDOG1 does not do this. Enabling another peripheral that uses PIOSC
 *       such as ADC0 or SSI0, however, will do so. Example:
 *
 *       SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
 *       SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG1);
 *
 *       See the following forum post for more information:
 *       http://e2e.ti.com/support/microcontrollers/stellaris_arm_cortex-m3_microcontroller/f/471/p/176487/654390.aspx#654390
 */

/*
* Review comment - SK
* Is it used? Need to understand how and when the Board function calls made during boot sequence
* something we can capture in User's Guide or design document
*/
void NIRscanNano_initWatchdog()
{
    /* Enable peripherals used by Watchdog */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WDOG0);

    /* Initialize the Watchdog driver */
    Watchdog_init();
}

/*
 *  ======== NIRscanNano_powerDown ========
 *
 */
void NIRscanNano_powerDown()
{
	greenLED_off();						// Turn off LEDs
	yellowLED_off();
	blueLED_off();		 // Set SDRAM to full power
	tmp006_PowerDown( );				// Set TMP006 into Power Down Mode
	adc_PowerDown();				// Set ADS1255 into Power Down Mode
	/* TODO: shut SDRAM down completely, and regenerate patterns when waking up to lower hibernation power */
  	MAP_EPIConfigSDRAMSet(EPI0_BASE, EPI_SDRAM_CORE_FREQ_50_100 | EPI_SDRAM_LOW_POWER | SDRAM_SIZE, 468);  // Set SDRAM to Low Power
    MAP_HibernateWakeSet( HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RESET | HIBERNATE_WAKE_RTC ); // Configure Hibernate wake sources
	MAP_HibernateGPIORetentionEnable();	// Enable retention of GPIO state
    MAP_HibernateRequest();				// Request Hibernation
}

/*
 *  ======== NIRscanNano_powerUp ========
 *
 */
void NIRscanNano_powerUp()
{
	MAP_HibernateGPIORetentionDisable();
	MAP_EPIConfigSDRAMSet(EPI0_BASE, EPI_SDRAM_CORE_FREQ_50_100 | EPI_SDRAM_FULL_POWER | SDRAM_SIZE, 468);		 // Set SDRAM to full power
	adc_PowerUp();
}

void NIRscanNano_Sync_ADC()
/* Pulses the sync pin of ADC to stop current conversion and start new
 * It is recommended to do this when input analog voltage has changed
 */
{	// Set SYNC low to stop conversion
	MAP_GPIOPinWrite( SYNC_GPIO_PORT_BASE, SYNC_GPIO_MASK, 0 );
	// SYNC must be low for 4 (8MHz) clock cycles = 500ns
	MAP_SysCtlDelay(DELAY_225NS);
	// Set SYNC high to re-start conversion
    MAP_GPIOPinWrite( SYNC_GPIO_PORT_BASE, SYNC_GPIO_MASK, SYNC_GPIO_MASK );
}

void NIRscanNano_DRDY_int_enable(bool enable)
{
	if(enable == true)
	{
	    /* Clear any pending DRDY interrupts and enable DRDY interrupt now */
		MAP_GPIOIntEnable( DRDY_GPIO_PORT_BASE, DRDY_GPIO_MASK );
		MAP_IntEnable( INT_GPIOP2 );		//DRDY
		MAP_GPIOIntClear(DRDY_GPIO_PORT_BASE, GPIO_INT_PIN_2);
		MAP_IntPendClear(INT_GPIOP2);
	}
	else
	{
		MAP_GPIOIntDisable( DRDY_GPIO_PORT_BASE, DRDY_GPIO_MASK );
		MAP_IntDisable( INT_GPIOP2 );
	}
}

void NIRscanNano_trigger_next_pattern()
{
	MAP_GPIOPinWrite( TRIGNEXT_GPIO_PORT_BASE, TRIGNEXT_GPIO_MASK, TRIGNEXT_GPIO_MASK );
	while(MAP_GPIOPinRead(TRIGNEXT_GPIO_PORT_BASE, TRIGNEXT_GPIO_MASK) != TRIGNEXT_GPIO_MASK)
	{

	}
	// Set it back low
	MAP_GPIOPinWrite( TRIGNEXT_GPIO_PORT_BASE, TRIGNEXT_GPIO_MASK, 0 );
}
