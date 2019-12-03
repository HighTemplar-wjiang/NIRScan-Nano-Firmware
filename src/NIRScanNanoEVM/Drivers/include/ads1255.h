//*****************************************************************************
//
// APIs to interface with ADS1255 ADC over SPI bus.
//
// Copyright (c) 2007-2015 Texas Instruments Incorporated.  All rights reserved.
//
//*****************************************************************************

#ifndef ADS1255_H_
#define ADS1255_H_

#include <stdint.h>
#include <stdbool.h>

// 24 bit ADC (3 x 8)
#define NUM_SSI_DATA  (3)

// detector commands
#define COMMAND_RREG    (0x10)
#define COMMAND_WREG    (0x50)
#define COMMAND_WAKEUP  (0x00)
#define COMMAND_RESET   (0xFE)
#define COMMAND_STANDBY (0xFD)
#define COMMAND_RDATAC  (0x03)
#define COMMAND_SDATAC  (0x0F)
#define COMMAND_SELFCAL (0xF0)
#define COMMAND_SYNC    (0xFC)

// detector registers
#define REGISTER_STATUS (0x00)
#define REGISTER_ADCON  (0x02)
#define REGISTER_DRATE  (0x03)

#define MASK_ANALOG_IBUFFER (0x02)

// allowed detector PGA values
#define ADS1255_PGA1  (0x00)
#define ADS1255_PGA2  (0x01)
#define ADS1255_PGA4  (0x02)
#define ADS1255_PGA8  (0x03)
#define ADS1255_PGA16 (0x04)
#define ADS1255_PGA32 (0x05)
#define ADS1255_PGA64 (0x06)

// allowed detector data rates
#define ADS1255_RATE_30000SPS (0xF0)
#define ADS1255_RATE_15000SPS (0xE0)
#define ADS1255_RATE_7500SPS  (0xD0)
#define ADS1255_RATE_3750SPS  (0xC0)
#define ADS1255_RATE_2000SPS  (0xB0)
#define ADS1255_RATE_1000SPS  (0xA1)
#define ADS1255_RATE_500SPS   (0x92)
#define ADS1255_RATE_100SPS   (0x82)
#define ADS1255_RATE_60SPS    (0x72)
#define ADS1255_RATE_50SPS    (0x63)
#define ADS1255_RATE_30SPS    (0x53)
#define ADS1255_RATE_25SPS    (0x43)
#define ADS1255_RATE_15SPS    (0x33)
#define ADS1255_RATE_10SPS    (0x023)
#define ADS1255_RATE_5SPS     (0x13)
#define ADS1255_RATE_2P5SPS   (0x03)

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Internal helper function for clearing the read buffer of trash before use
 */
int32_t ads1255_EmptyReadBuffer();

/**
 * Perform ADS1255 self test. Checkes powerup/powerdown, wakeup/standby, and read/write registers.
 * \returns non-zero on error
 */
int32_t ads1255_SelfTest();

/**
 * Configure TIVA hardware to enable ADS1255. Also performs reset of ADC to ensure
 * self calibration and factory default values. This function must always be called
 * first.
 */
int32_t ads1255_init();

/**
 * Power up the ADS1255.
 * All internal registers are reset to factory default values.
 */
int32_t ads1255_PowerUp();

/**
 * Power down the ADS1255.
 * User is responsible for ensuring minimum wait time of 20 DRDY cycles
 * after function call to ensure power down is complete.
 */
int32_t ads1255_PowerDown();

/**
 * Wake the ADS1255 from standby.
 * Internal register settings remain unchanged.
 */
int32_t ads1255_Wakeup();
int32_t ads1255_Wakeup_NoDelay();

/**
 * Puts the ADS1255 into sleep state.
 * Use in place of powerdown if fast wakeup is required.
 * \returns non-zero on error
 */
int32_t ads1255_Standby();

/**
 * Reset the ADS1255 to factory default values
 */
int32_t ads1255_Reset();

/**
 * Configure the analog input buffer, PGA setting, and data rate of the ADS1255.
 * \param[in] iBufferEnable non-zero to disable else enable the analog input buffer
 * \param[in] pgaVal PGA gain setting (example: ads1255_PGA1)
 * \param[in] dataRate detector sample data rate (example: ads1255_RATE_30000SPS)
 * \returns non-zero on error
 */
int32_t ads1255_Configure(uint32_t iBufferEnable, uint32_t pgaVal, uint32_t dataRate);

/**
 * Enable or disable the analog input buffer
 * \param[in] enableState non-zero to disable else enable
 * \returns non-zero on error
 */
int32_t ads1255_SetAnalogInputBuffer(uint32_t enableState);

/**
 * Configures the ADS1255 for continuuous sample read.
 * \param[in] enableState non-zero to disable else enable
 */
int32_t ads1255_SetReadContinuous(bool enableState);

/**
 * Get a sample value from the ADS1255 assuming continuuous read operation
 * \returns sample value
 */
int32_t ads1255_GetSample();
int32_t ads1255_SetPGAGain(uint8_t pgaVal);
int8_t ads1255_GetPGAGain(void);

uint32_t ads1255_GetDataRate(void);
int32_t ads1255_GetSettlingTime(void);


#ifdef __cplusplus
}
#endif
#endif /* ADS1255_H_ */
