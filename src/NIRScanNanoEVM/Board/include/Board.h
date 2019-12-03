/*
 * Copyright (c) 2014-2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 */

#ifndef __BOARD_H
#define __BOARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "NIRscanNano.h"

#define Board_initGeneral           NIRscanNano_initGeneral
#define Board_initGPIO              NIRscanNano_initGPIO
#define Board_initI2C               NIRscanNano_initI2C
#define Board_initPWM               NIRscanNano_initPWM
#define Board_initSDSPI             NIRscanNano_initSDSPI
#define Board_initSPI               NIRscanNano_initSPI
#define Board_initUART              NIRscanNano_initUART
#define Board_initUSB               NIRscanNano_initUSB
#define Board_initUSBMSCHFatFs      NIRscanNano_initUSBMSCHFatFs
#define Board_initWatchdog          NIRscanNano_initWatchdog
#define Board_initWiFi              NIRscanNano_initWiFi
#define Board_initADC               NIRscanNano_initADC
#define Board_initLampGPIO          NIRscanNano_initLampGPIO

#define BOARD_LED_ON                NIRSCANNANO_LED_ON
#define BOARD_LED_OFF               NIRSCANNANO_LED_OFF
#define BOARD_LED0                  NIRSCANNANO_LED_Green
#define BOARD_LED1                  NIRSCANNANO_LED_Blue
#define BOARD_LED2                  NIRSCANNANO_LED_Yellow
#define BOARD_BUTTON0               NIRSCANNANO_BUTTON_Scan

#define BOARD_I2C0                  NIRSCANNANO_I2C3
#define BOARD_I2C_TMP               NIRSCANNANO_I2C7
#define BOARD_I2C_BQ                NIRSCANNANO_I2C9
#define BOARD_I2C_HDC		        NIRSCANNANO_I2C6
#define BOARD_I2C_DLPC		        NIRSCANNANO_I2C2
#define BOARD_I2C_EXP		        NIRSCANNANO_I2C8
#define BOARD_SDSPI0                NIRSCANNANO_SDSPI0

#define BOARD_SPI0                  NIRSCANNANO_SPI3
#define BOARD_SPI1                  NIRSCANNANO_SPI1
#define BOARD_SPI2                  NIRSCANNANO_SPI2
#define BOARD_SPI3                  NIRSCANNANO_SPI0

#define BOARD_USBMSCHFatFs0         NIRSCANNANO_USBMSCHFatFS0

#define BOARD_UART0                 NIRSCANNANO_UART0

#define BOARD_WATCHDOG0             NIRSCANNANO_WATCHDOG0

#define BOARD_WIFI                  NIRSCANNANO_WIFI

#define Board_gpioCallbacks0        NIRscanNano_gpioPortPCallbacks
#define Board_gpioCallbacks1        NIRscanNano_gpioPortNCallbacks

/* Board specific I2C addresses */
#define BOARD_TMP006_ADDR           (0x40)
#define BOARD_BQ24250_ADDR          (0x6A)
#define BOARD_HDC1000_ADDR          (0x40)
#define BOARD_DLPC150_ADDR          (0x1B)

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_H */
