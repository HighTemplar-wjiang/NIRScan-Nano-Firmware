/*
 *
 * NIRscan Nano Tiva defaults and board specific APIs
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/
#ifndef __NIRSCANNANO_H
#define __NIRSCANNANO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/drivers/GPIO.h>
#include "common.h"

/* LEDs on NIRscanNano are active low. */
#define NIRSCANNANO_LED_OFF (~0)
#define NIRSCANNANO_LED_ON  (0)

/* 8MB is the size of flash memory that holds DLPC150 firmware */
#define DLPC150_FLASH_SIZE (8*1024*1024)

// RGB framebuffer stored in internal memory (32BPP) or in external SDRAM (24BPP)
//#define INTERNAL_FRAMEBUFFER

// When defined, RGB frame buffer will be 16BPP and RGB656 format data will be streamed over video interface
//#define SIXTEEN_BPP

/* GPIO_Callbacks structure for GPIO interrupts */
#ifdef SCAN_BUTTON_TIVA_BOARD
extern const GPIO_Callbacks NIRscanNano_gpioPortQCallbacks;
#endif
#ifdef SCAN_BUTTON_EXP_CONN
extern const GPIO_Callbacks NIRscanNano_gpioPortKCallbacks;
#endif

/*!
 *  @def    NIRscanNano_GPIOName
 *  @brief  Enum of LED names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_GPIOName {
	NIRSCANNANO_LED_GREEN = 0,
	NIRSCANNANO_LED_BLUE,
	NIRSCANNANO_LED_YELLOW,
	NIRSCANNANO_BUTTON_SCAN,

    NIRSCANNANO_GPIOCOUNT
} NIRscanNano_GPIOName;

/*!
 *  @def    NIRscanNano_I2CName
 *  @brief  Enum of I2C names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_I2CName {
    NIRSCANNANO_I2C2 = 0,
    NIRSCANNANO_I2C6,
    NIRSCANNANO_I2C7,
    NIRSCANNANO_I2C8,
	NIRSCANNANO_I2C9,

	NIRSCANNANO_I2CCOUNT
} ENIRscanNano_I2CName;

/*!
 *  @def    NIRscanNano_PWMName
 *  @brief  Enum of PWM names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_PWMName {
    NIRSCANNANO_PWM6 = 0,

   NIRSCANNANO_PWMCOUNT
} NIRscanNano_PWMName;

/*!
 *  @def    NIRscanNano_SDSPIName
 *  @brief  Enum of SDSPI names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_SDSPIName {
    NIRSCANNANO_SDSPI0 = 0,

    NIRSCANNANO_SDSPICOUNT
} NIRscanNano_SDSPIName;

/*!
 *  @def    NIRscanNano_SPIName
 *  @brief  Enum of SPI names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_SPIName {
    NIRSCANNANO_SPI0 = 0,
    NIRSCANNANO_SPI1,
    NIRSCANNANO_SPI2,
    NIRSCANNANO_SPI3,

    NIRSCANNANO_SPICOUNT
} NIRscanNano_SPIName;

/*!
 *  @def    NIRscanNano_UARTName
 *  @brief  Enum of UARTs on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_UARTName {
    NIRSCANNANO_UART3 = 0,
    NIRSCANNANO_UART4,

    NIRSCANNANO_UARTCOUNT
} NIRscanNano_UARTName;

/*!
 *  @def    NIRscanNano_USBMSCHFatFsName
 *  @brief  Enum of USBMSCHFatFs names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_USBMSCHFatFsName {
    NIRSCANNANO_USBMSCHFatFS0 = 0,

    NIRSCANNANO_USBMSCHFATFSCOUNT
} NIRscanNano_USBMSCHFatFsName;

/*
 *  @def    NIRscanNano_WatchdogName
 *  @brief  Enum of Watchdogs on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_WatchdogName {
    NIRSCANNANO_WATCHDOG0 = 0,

    NIRSCANNANO_WATCHDOGCOUNT
} NIRscanNano_WatchdogName;

/*!
 *  @def    NIRscanNano_WiFiName
 *  @brief  Enum of WiFi names on the NIRscanNano EVM board
 */
typedef enum NIRscanNano_WiFiName {
    NIRSCANNANO_WIFI = 0,

    NIRSCANNANO_WIFICOUNT
} NIRscanNano_WiFiName;

int NIRscanNano_DLPCEnable(bool enable);

/*!
 *  @brief  Initialize the general board specific settings
 *
 *  This function initializes the general board specific settings. This include
 *     - Enable clock sources for peripherals
 */

int NIRscanNano_LampEnable(bool enable);
int NIRscanNano_initADC(void);
void NIRscanNano_initLampGPIO();

/*!
 *  @brief  Initialize the lamp driver
 *
 *  This function allows setting the lamp enable GPIO high or low
 *  
 */

extern void NIRscanNano_initGeneral(void);

/*!
 *  @brief Initialize board specific EMAC settings
 *
 *  This function initializes the board specific EMAC settings and
 *  then calls the EMAC_init API to initialize the EMAC module.
 *
 *  The EMAC address is programmed as part of this call.
 *
 */
extern void NIRscanNano_initEMAC(void);

/*!
 *  @brief  Initialize board specific GPIO settings
 *
 *  This function initializes the board specific GPIO settings and
 *  then calls the GPIO_init API to initialize the GPIO module.
 *
 *  The GPIOs controlled by the GPIO module are determined by the GPIO_config
 *  variable.
 */
extern void NIRscanNano_initGPIO(void);

/*!
 *  @brief  Initialize board specific I2C settings
 *
 *  This function initializes the board specific I2C settings and then calls
 *  the I2C_init API to initialize the I2C module.
 *
 *  The I2C peripherals controlled by the I2C module are determined by the
 *  I2C_config variable.
 */
extern void NIRscanNano_initI2C(void);

/*!
 *  @brief  Initialize board specific PWM settings
 *
 *  This function initializes the board specific PWM settings and then calls
 *  the PWM_init API to initialize the PWM module.
 *
 *  The PWM peripherals controlled by the PWM module are determined by the
 *  PWM_config variable.
 */
extern void NIRscanNano_initPWM(void);


/*!
 *  @brief  Initialize board specific SDSPI settings
 *
 *  This function initializes the board specific SDSPI settings and then calls
 *  the SDSPI_init API to initialize the SDSPI module.
 *
 *  The SDSPI peripherals controlled by the SDSPI module are determined by the
 *  SDSPI_config variable.
 */
extern void NIRscanNano_initSDSPI(void);

/*!
 *  @brief  Initialize board specific SPI settings
 *
 *  This function initializes the board specific SPI settings and then calls
 *  the SPI_init API to initialize the SPI module.
 *
 *  The SPI peripherals controlled by the SPI module are determined by the
 *  SPI_config variable.
 */
extern void NIRscanNano_initSPI(void);

/*!
 *  @brief  Initialize board specific UART settings
 *
 *  This function initializes the board specific UART settings and then calls
 *  the UART_init API to initialize the UART module.
 *
 *  The UART peripherals controlled by the UART module are determined by the
 *  UART_config variable.
 */
extern void NIRscanNano_initUART(void);

/*!
 *  @brief  Initialize board specific USB settings
 *
 *  This function initializes the board specific USB settings and pins based on
 *  the USB mode of operation.
 *
 */
extern void NIRscanNano_initUSB(void);

/*!
 *  @brief  Initialize board specific USBMSCHFatFs settings
 *
 *  This function initializes the board specific USBMSCHFatFs settings and then
 *  calls the USBMSCHFatFs_init API to initialize the USBMSCHFatFs module.
 *
 *  The USBMSCHFatFs peripherals controlled by the USBMSCHFatFs module are
 *  determined by the USBMSCHFatFs_config variable.
 */
extern void NIRscanNano_initUSBMSCHFatFs(void);

/*!
 *  @brief  Initialize board specific Watchdog settings
 *
 *  This function initializes the board specific Watchdog settings and then
 *  calls the Watchdog_init API to initialize the Watchdog module.
 *
 *  The Watchdog peripherals controlled by the Watchdog module are determined
 *  by the Watchdog_config variable.
 */
extern void NIRscanNano_initWatchdog(void);

/*!
 *  @brief  Initialize board specific WiFi settings
 *
 *  This function initializes the board specific WiFi settings and then calls
 *  the WiFi_init API to initialize the WiFi module.
 *
 *  The hardware resources controlled by the WiFi module are determined by the
 *  WiFi_config variable.
 */
extern void NIRscanNano_initWiFi(void);

#ifdef __cplusplus
}
#endif

// NIRscan Nano System Clock Frequency
#define NIRSCAN_SYSCLK	120000000L

  // NIRscan Nano GPIO Pins
  // Pin  	Function 	Type       Init Value	Int         Schematic Name	Connection
  /* PB2	GPIO		Out			LOW		                P1P8V_EN		TPS22904 				
   * PD5	GPIO 		OUT			LOW						BAT_VSENSE_EN	DMC2990			
   * PD7	GPIO 		OUT			HIGH 					TIVA_TRIG_SEL	J5 Connector							
   * PE0	GPIO 		OUT 		HIGH					TIVA_DATAEN_MASK SN74ALVC08		
   * PE1	GPIO 		OUT 		HIGH					TIVA_VSYNC_MASK	SN74ALVC08
   * PE2	GPIO 		OUT 		LOW						TIVA_TRIG_IN_0
   * PE3	GPIO 		OUT			LOW 					TIVA_TRIG_IN_1				
   * PF5	GPIO 		OUT 		HIGH					On/Off			Green LED
   * PH5	GPIO 		OUT 		LOW						nSHUTD			CC2564
   * PH6 	GPIO 		OUT 		LOW						SYNCZ			ADS1255
   * PH7	GPIO 		OUT 		HIGH					O_Bluetooth		Blue LED
   * PJ7	GPIO 		OUT 		LOW						PROJ_ON	
   * PL4	GPIO 		OUT 		HIGH					Yellow LED		O_Scan
   * PP0	GPIO 		IN 									DLPC_XLAT0_TIVA	
   * PP1	GPIO 		IN 									DLPC_XLAT1_TIVA
   * PP2	GPIO 		IN 						FALLING EDGE DRDY			ADS1255
   * PP6 	GPIO 		IN  					FALLING EDGE TDRDYZ			TMP006
   * PP7	GPIO		IN						FALLING EDGE HUM_DRDYz		HDC1000
   * PQ0	GPIO 		OUT 		LOW						BC_EN2
   * PQ1	GPIO 		OUT			HIGH					BC_EN1
   * PQ2	GPIO 		OUT			LOW						BC_CSZ
   * PQ3	GPIO 		IN 						FALLING EDGE SW_SCAN		Scan Button
   * PQ4	GPIO 		IN						FALLING EDGE mSD_CARD_DET 	Card Detect
   * PQ5	GPIO 		IN						FALLING EDGE BC_INT
   * PQ6 	GPIO 		IN 						FALLING EDGE PAD_RSTz
   * PQ7	GPIO		IN						FALLING EDGE DLPC_HOST_IRQ
   */

#define P1P8V_EN_BIT			GPIO_PIN_2			// PB2
#define P1P8V_EN_BASE			GPIO_PORTB_BASE		// PB2

#define BAT_VSENSE_EN_BIT		GPIO_PIN_5			// PD5
#define BAT_VSENSE_EN_BASE		GPIO_PORTD_BASE		// PD5

#define TIVA_DATAEN_MASK_BIT	GPIO_PIN_0			// PE0
#define TIVA_DATAEN_MASK_BASE	GPIO_PORTE_BASE		// PE0

#define TIVA_VSYNC_MASK_BIT		GPIO_PIN_1			// PE1
#define TIVA_VSYNC_MASK_BASE	GPIO_PORTE_BASE		// PE1

#define TIVA_TRIG_IN_0_BIT		GPIO_PIN_2			// PE2
#define TIVA_TRIG_IN_0_BASE		GPIO_PORTE_BASE		// PE2

#define TIVA_TRIG_IN_1_BIT		GPIO_PIN_3			// PE3
#define TIVA_TRIG_IN_1_BASE		GPIO_PORTE_BASE		// PE3

#define GREEN_LED_BIT			GPIO_PIN_5			// PF5
#define GREEN_LED_BASE			GPIO_PORTF_BASE		// PF5

#define NSHUTD_BIT				GPIO_PIN_5			// PH5
#define NSHUTD_BASE				GPIO_PORTH_BASE		// PH5

#define SYNCZ_BIT				GPIO_PIN_6			// PH6
#define SYNCZ_BASE				GPIO_PORTH_BASE		// PH6

#define BLUE_LED_BIT			GPIO_PIN_7			// PH7
#define BLUE_LED_BASE			GPIO_PORTH_BASE		// PH7

#define PROJ_ON_BIT				GPIO_PIN_7			// PJ7
#define PROJ_ON_BASE			GPIO_PORTJ_BASE		// PJ7

#define YELLOW_LED_BIT			GPIO_PIN_4			// PL4
#define YELLOW_LED_BASE			GPIO_PORTL_BASE		// PL4

#define DLPC_XLAT0_TIVA_BIT		GPIO_PIN_0			// PP0
#define DLPC_XLAT0_TIVA_BASE	GPIO_PORTP_BASE		// PP0

#define DLPC_XLAT1_TIVA_BIT		GPIO_PIN_1			// PP1
#define DLPC_XLAT1_TIVA_BASE	GPIO_PORTP_BASE		// PP1

#define DRDY_BIT				GPIO_PIN_2			// PP2
#define DRDY_BASE				GPIO_PORTP_BASE		// PP2

#define TDRDYZ_BIT				GPIO_PIN_6			// PP6
#define TDRDYZ_BASE				GPIO_PORTP_BASE		// PP6

#define HUM_DRDYZ_BIT			GPIO_PIN_7			// PP7
#define HUM_DRDYZ_BASE			GPIO_PORTP_BASE		// PP7

#define BC_EN2_BIT				GPIO_PIN_0			// PQ0
#define BC_EN2_BASE				GPIO_PORTQ_BASE		// PQ0

#define BC_EN1_BIT				GPIO_PIN_1			// PQ1
#define BC_EN1_BASE				GPIO_PORTQ_BASE		// PQ1

#define BC_CSZ_BIT				GPIO_PIN_2			// PQ2
#define BC_CSZ_BASE				GPIO_PORTQ_BASE		// PQ2

#define SCAN_BUTTON_BIT			GPIO_PIN_3			// PQ3
#define SCAN_BUTTON_BASE		GPIO_PORTQ_BASE		// PQ3

#define BC_INT_BIT				GPIO_PIN_5			// PQ5
#define BC_INT_BASE				GPIO_PORTQ_BASE		// PQ5

#define PAD_RSTZ_BIT			GPIO_PIN_6			// PQ6
#define PAD_RSTz_BASE			GPIO_PORTQ_BASE		// PQ6

#define DLPC_HOST_IRQ_BIT		GPIO_PIN_7			// PQ7
#define DLPC_HOST_IRQ_BASE		GPIO_PORTQ_BASE		// PQ7

#ifdef BPP4_MODE
	#define DISP_HEIGHT       (1)
	#define DISP_VFP          (6)
	#define DISP_VBP          (6)
	#define DISP_VSYNCW       (10)
	#define PIXEL_CLOCK       (30000000) //30 MHz
#else
#ifdef INTERNAL_FRAMEBUFFER
	#define DISP_HEIGHT		  (1)
	#define DISP_VFP          (0)
	#define DISP_VBP          (0)
	#define DISP_VSYNCW       (1)
	#define PIXEL_CLOCK       (60000000) //60 MHz
#else
	#define DISP_HEIGHT       (480)
	#define DISP_VFP          (6)
	#define DISP_VBP          (6)
	#define DISP_VSYNCW       (5)
	#define PIXEL_CLOCK       (30000000) //30 MHz
#endif
#endif

#define DMD_WIDTH	      (854)
#define DMD_WIDTH_MSB	  ((DMD_WIDTH) & 0xFF00) >> 8
#define DMD_WIDTH_LSB	  ((DMD_WIDTH) & 0x00FF)
#define DMD_HEIGHT			(480)
#define DMD_HEIGHT_MSB	  ((DMD_HEIGHT) & 0xFF00) >> 8
#define DMD_HEIGHT_LSB	  ((DMD_HEIGHT) & 0x00FF)
// These settings result in 864x480@60Hz
#define DISP_WIDTH        (864)
#define DISP_WIDTH_MSB	  ((DISP_WIDTH) & 0xFF00) >> 8
#define DISP_WIDTH_LSB	  ((DISP_WIDTH) & 0x00FF)
#define DISP_HEIGHT_MSB	  ((DISP_HEIGHT) & 0xFF00) >> 8
#define DISP_HEIGHT_LSB	  ((DISP_HEIGHT) & 0x00FF)
#define DISP_HFP	      (50)
#define DISP_HBP          (77)
#define DISP_HSYNCW       (10)
#define DISP_ACBIAS_COUNT (0)
#define PAL_LOAD_DELAY    (0)

#define DISP_TOTAL_WIDTH (DISP_WIDTH + DISP_HFP + DISP_HBP + DISP_HSYNCW)
#define DISP_TOTAL_HEIGHT (DISP_HEIGHT + DISP_VFP + DISP_VBP + DISP_VSYNCW)
#define DISP_VSYNC_PERIOD_US (DISP_TOTAL_WIDTH * DISP_TOTAL_HEIGHT / 30)

#ifdef BPP4_MODE //4bpp mode - expected for test only
	#define DISPLAY_NPIXELS (864*480)
	#define DISPLAY_BPP (4)
	#define DISPLAY_NBYTES (DISPLAY_NPIXELS * DISPLAY_BPP / 8)
	#define DISPLAY_PALETTE_NCOLORS (16)
	#define DISPLAY_PALETTE_BPC (16)
	#define DISPLAY_PALETTE_NBYTES (DISPLAY_PALETTE_NCOLORS*DISPLAY_PALETTE_BPC/8)
	#define FRAMEBUFFER_NBYTES (DISPLAY_NBYTES + DISPLAY_PALETTE_NBYTES)
	extern uint32_t g_displayFrameBuffer0[FRAMEBUFFER_NBYTES/4];
	extern uint32_t g_palette[DISPLAY_PALETTE_NBYTES/4];
#else //24 bpp display mode - main operation
	#define FRAMEBUFFER_NPIXELS (864*480)
	#define FRAMEBUFFER_BPP (24)
	#define FRAMEBUFFER_NBYTES (FRAMEBUFFER_NPIXELS*FRAMEBUFFER_BPP/8)
#endif
//extern uint32_t g_displayFrameBuffer0[864 * 50  / 8];

//#define CONSOLE_MSGS

void NIRscanNano_powerUp( void );
void NIRscanNano_powerDown( void );
void NIRscanNano_Sync_ADC(void);
void NIRscanNano_DRDY_int_enable(bool enable);
void NIRscanNano_trigger_next_pattern(void);

#endif /* __NIRSCANNANO_H */
