/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com
 * ALL RIGHTS RESERVED
 *
 */

//DLPC150 I2C address
#define DLPC150_I2CADDR 	0x1B


// DLPC150 Administrative Commands
#define DLPC150_ADMIN_SHORT_STATUS	0xD0
#define DLPC150_ADMIN_SYS_STATUS	0xD1
#define DLPC150_ADMIN_SYS_SW		0xD2
#define DLPC150_ADMIN_COMM_STATUS	0xD3
#define DLPC150_ADMIN_ASICID    	0xD4
#define DLPC150_ADMIN_DMDID  	  	0xD5
#define DLPC150_ADMIN_TEMP  	  	0xD6
#define DLPC150_ADMIN_FLASH_VER		0xD9
#define DLPC150_ADMIN_LIGHT_SENSOR	0xDA

// DLPC150 System Commands
#define DLPC150_SYS_W_IN_SOURCE_SEL	0x05
#define DLPC150_SYS_R_IN_SOURCE_SEL	0x06
#define DLPC150_SYS_W_EXT_VID_SOURCE	0x07
#define DLPC150_SYS_R_EXT_VID_SOURCE	0x08
#define DLPC150_SYS_W_TEST_SEL		0x0B
#define DLPC150_SYS_R_TEST_SEL		0x0C
#define DLPC150_SYS_W_SPLASH_SEL	0x0D
#define DLPC150_SYS_R_SPLASH_SEL	0x0E
#define DLPC150_SYS_W_IMAGE_CROP	0x10
#define DLPC150_SYS_R_IMAGE_CROP	0x11
#define DLPC150_SYS_W_DISPLAY_SIZE	0x12
#define DLPC150_SYS_R_DISPLAY_SIZE	0x13
#define DLPC150_SYS_W_EXT_IM_SIZE	0x2E
#define DLPC150_SYS_R_EXT_IM_SIZE	0x2F
#define DLPC150_SYS_W_SPLASH_EXEC	0x35

#define DLPC150_SYS_SEQ_COMMAND		0xF1
#define DLPC150_REG_WRITE    		0xF1
#define DLPC150_REG_READ    		0xF2

// DLPC150 Illumination Control Commands
#define DLPC150_LED_W_CONTROL	  	0x50
#define DLPC150_LED_R_CONTROL	  	0x51
#define DLPC150_LED_W_ENABLE		0x52
#define DLPC150_LED_R_ENABLE		0x53
#define DLPC150_LED_W_CURRENT	  	0x54
#define DLPC150_LED_R_CURRENT	  	0x55
#define DLPC150_LED_R_CAIC_MAX_PWR	0x57
#define DLPC150_LED_W_MAX_CURRENT	0x5C
#define DLPC150_LED_R_MAX_CURRENT	0x5D
#define DLPC150_LED_R_PARAM			0x5E
#define DLPC150_LED_R_CAIC_CURRENT	0x5F

// DLPC150 GPIO Commands
#define DLPC150_GPIO_W_CONTROL		0x31
#define DLPC150_GPIO_R_CONTROL		0x32
#define DLPC150_GPIO_W_OUTPUT		0x33
#define DLPC150_GPIO_R_OUTPUT		0x34

//DLPC150 SEQUENCE MODE COMMANDS
#define DLPC150_FLASH_PATTERN       0xF4
#define DLPC150_PATTERN_STREAMING   0xF5
#define DLPC150_HWLOCK_MODE         0xF6
#define DLPC150_SEQ_ABORT           0xF7
#define DLPC150_SEQ_VECT_NUM		0xF8

// DLPC150 GPIO
#define DLPC150_GPIO13_OUTPUT	0x00000200
#define DLPC150_GPIO13_OUT_0	0x00001000
#define DLPC150_GPIO13_OUT_H	0x00000010
#define DLPC150_GPIO13_OUT_L	0x00000000


// DLPA2005 
#define DLPC150_DLPA_W_REG_ADDRESS	0xEB
#define DLPC150_DLPA_W_REG_DATA		0xEC
#define DLPC150_DLPA_R_REG_DATA		0xED

#define LED_RED_ENABLE              0x1
#define LED_GREEN_ENABLE            0x2
#define LED_BLUE_ENABLE             0x4

#define TRANS_CURR_130				0x02
#define TRANS_CURR_150				0x12
#define TRANS_CURR_172				0x22
#define TRANS_CURR_192				0x32
#define TRANS_CURR_220				0x42
#define TRANS_CURR_275				0x52
#define TRANS_CURR_330				0x62
#define TRANS_CURR_440				0x72
#define TRANS_CURR_550				0x82
#define TRANS_CURR_660				0x92
#define TRANS_CURR_770				0xA2
#define TRANS_CURR_888				0xB2
#define TRANS_CURR_990				0xC2
#define TRANS_CURR_1160				0xD2
#define TRANS_CURR_1330				0xE2
#define TRANS_CURR_1500				0xF2
#define TRANS_CURR_220				0x42
#define TRANS_CURR_275				0x52


#define INPUT_SOURCE_RGB			0x0
#define INPUT_SOURCE_TEST			0x1
#define INPUT_SOURCE_SPLASH			0x2

#define VIDEO_SOURCE_RGB888			0x43
#define VIDEO_SOURCE_RGB565			0x40

// TI Manufacturer and Device ID
#define DLPC150_ASICID		0x04
#define DLPC150_DEVID_0		0x60
#define DLPC150_DEVID_1     0x0D
#define DLPC150_DEVID_2     0x00
#define DLPC150_DEVID_3     0x64

// The states of the DLPC150 state machine.
#define DLPC150_STATE_IDLE       0
#define DLPC150_STATE_INIT       1
#define DLPC150_STATE_READ       2
#define DLPC150_STATE_WRITE      3
#define DLPC150_STATE_RMW        4

#define PATTERNS_FROM_FLASH		0
#define PATTERNS_FROM_RGB_PORT	1

#ifdef __cplusplus
extern "C" {
#endif

int16_t dlpc150_SetUpSource(bool patterns_from_rgb_port );
int16_t dlpc150_GetSoftwareVersion( uint32_t *pVer );
int16_t dlpc150_GetFlashBuildVersion( uint32_t *pVer );
int16_t dlpc150_LampEnable( bool enable );
int16_t dlpc150_WriteReg( uint32_t addr, uint32_t value );
int16_t dlpc150_ReadReg( uint32_t addr, uint32_t *pValue );
int16_t dlpc150_displayCrop( uint16_t startY, uint16_t height );
int16_t dlpc150_GetLightSensorData( uint32_t *RSensor, uint32_t *GSensor, uint32_t *BSensor );
int16_t dlpc150_EnableSequencer( bool enable );
int16_t dlpc150_GetSequenceVectorNumber( uint16_t *pSeqVectNum );

#ifdef TPG
int16_t dlpc150_testPatternSelect( uint8_t tpg_num );
#endif
#ifdef SPLASH_VARIABLE_EXP
int16_t dlpc150_splashSelect( uint8_t );
#endif
#if 0
int16_t dlpc150_GetGPIOControl( uint32_t *gpioControl );
int16_t dlpc150_SetGPIOControl( uint32_t gpioControl );
int16_t dlpc150_GetGPIOOutput( uint32_t *gpioControl0, uint32_t *gpioControl1 );
int16_t dlpc150_SetGPIOOutput( uint32_t gpioControl0, uint32_t gpioControl1 );
int16_t dlpc150_SetSequenceAbort(void);
int16_t dlpc150_SetHWLockMode(void);
#endif

#ifdef __cplusplus
}
#endif

