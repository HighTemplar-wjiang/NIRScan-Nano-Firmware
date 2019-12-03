/*
 *
 * Common BLE declarations/defintions for used across TIVA source code
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLECOMMONDEFS_H_
#define BLECOMMONDEFS_H_

#include "BTPSCFG.h"
#include "NNOCommandDefs.h"

/** BLE max supported packet size as per Bluetooth spec 4.0 & iOS 8.x.x		*/
#define BLE_MAX_PACKET_SIZE 						BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE
#define BLE_MAX_COMMAND_SIZE						20

   /* The following defines the HRS GATT Service Flags MASK that should */
   /* be passed into GATT_Register_Service when the HRS Service is      */
   /* registered.                                                       */
#define BLE_SERVICE_FLAGS                              (GATT_SERVICE_FLAGS_LE_SERVICE)

/**
 *  Denotes the max number of LE connections that are allowed at
 *  the same time
 */
#define MAX_LE_CONNECTIONS                          (1)

/**
 * @brief Common BLE error definitions
 */
/** Denotes that task creation failed										*/
#define APPLICATION_TASK_CREATION_FAILED			(-1001)
/** Denotes that no command was specified to the parser.           			*/
#define APPLICATION_ERROR_NO_COMMAND				(-1002)
/** Denotes that the command does not exist for processing.       			*/
#define APPLICATION_ERROR_INVALID_COMMAND			(-1003)
/** Denotes that the command specified was the Exit command.          		*/
#define APPLICATION_ERROR_EXIT_CODE					(-1004)
/** Denotes that an error occurred in execution of the command function. 	*/
#define APPLICATION_ERROR_FUNCTION					(-1005)
/* Denotes that there are more parameters then will fit in the UserCommand.	*/
#define APPLICATION_ERROR_TOO_MANY_PARAMS			(-1006)
/** Denotes that an error occurred due to the fact that  one or more of the
 * required parameters were invalid
 */
#define APPLICATION_ERROR_INVALID_PARAMETERS       	(-1007)
/** Denotes that an error occurred while Initializing the Bluetooth Protocol
 *  Stack
 */
#define APPLICATION_ERROR_UNABLE_TO_OPEN_STACK     	(-1008)
/** Denotes that an occurred due to attempted execution of a command when a
 *  Bluetooth Protocol Stack has not been
 */
#define APPLICATION_ERROR_INVALID_STACK_ID			(-1009)
/** Denotes that an error occurred because the service was registered
 *  already for the stack instance
 */
#define APPLICATION_ERROR_GATT_SERVICE_EXISTS		(-1010)
/**
 * Denotes GAP related error in application
 */
#define APPLICATION_ERROR_GAPS						(-1011)
/**
 * @enum _tagBLE_Connection_Status_t connection status definition
 */
typedef enum _tagBLE_Connection_Status_t
{
   BLE_CONN_STATUS_INACTIVE,
   BLE_CONN_STATUS_ADVERTISING,
   BLE_CONN_STATUS_ACTIVE,
   BLE_CONN_STATUS_MAX
} BLE_Connection_Status_Type_t;

/**
 * @enum _tagBLE_Command_Type_t command type
 *
 * 0 = invalid type
 * 1 = Read commands that require immediate response
 * 2 = read commands whose response may be delayed - like polling a sensor etc.
 * 3 = write commands that only require an ack to be sent back
 * 4 = write commands that would be followed by multiple notifications with actual data
 * 5 = write commands that would be followed by multiple indications with actual data
 * 6 = write commands that need processing before sending write response back to client
 */
typedef enum _tagBLE_Command_Type_t
{
	BLE_COMMAND_TYPE_INVALID,
	BLE_COMMAND_TYPE_READ_IMMED_RESPONSE,
	BLE_COMMAND_TYPE_READ_DELAY_RESPONSE,
	BLE_COMMAND_TYPE_WRITE,
	BLE_COMMAND_TYPE_WRITE_NOTIFY,
	BLE_COMMAND_TYPE_WRITE_INDICATE,
	BLE_COMMAND_TYPE_WRITE_DELAYED_RESPONSE
} BLE_Command_Type_t;

/**
 * @enum _tagBLE_Scan_Data_Field_Type_t data sub field definitions
 */
typedef enum _tagBLE_Scan_Data_Field_Type_t
{
	BLE_SCAN_DATA_FIELD_INVALID,
	BLE_SCAN_DATA_FIELD_NAME,
	BLE_SCAN_DATA_FIELD_TYPE,
	BLE_SCAN_DATA_FIELD_TIME,
	BLE_SCAN_DATA_FIELD_BLOB_VER,
	BLE_SCAN_DATA_FIELD_BLOB
} BLE_Scan_Data_Field_Type_t;

/**
 * @enum _tagBLE_Notify_Type_t Notification type definitions
 */
typedef enum _tagBLE_Notify_Type_t
{
	BLE_NOTIFY_TEMPERATURE,
	BLE_NOTIFY_HUMIDITY,
	BLE_NOTIFT_DEVICE_STATUS,
	BLE_NOTIFY_COMMANDS,
	BLE_NOTIFY_SCAN_STATUS,
	BLE_NOTIFY_CLEAR_SCAN_STATUS,
	BLE_NOTIFY_MAX
} BLE_Notify_Type;

typedef enum _tagBLE_Indicate_Type_t
{
	BLE_INDICATE_ERROR_STATUS = BLE_NOTIFY_MAX,
	BLE_INDICATE_MAX
} BLE_Indicate_Type;

typedef enum _tagBLE_Addl_Commands_t
{
	NNO_START_SCAN = NNO_FILE_MAX_TYPES,
	NNO_CLEAR_SCAN,
	NNO_READ_TEMP,
	NNO_READ_HUM,
	NNO_READ_BATT,
    NNO_NUM_SCAN_CONFIG,
    NNO_GET_ACTIVE_SCAN_CONFIG,
    NNO_SET_ACTIVE_SCAN_CONFIG,
    NNO_FILE_SPEC_CAL_COEFF,
    NNO_SET_FILE_NAME_TAG,
	NNO_READ_DEV_STAT,
	NNO_READ_ERR_STAT,
	NNO_RESET_ERR_STAT,
	NNO_OTHER_COMMANDS,
	BLE_MAX_COMMANDS
} BLE_Addl_Commands;

/**
 * @enum _tagBLE_Error_Codes Error code definitions
 *
 * These are Nano App specific codes in addition to standard Bluetopia codes
 */
typedef enum _tagBLE_Error_Codes
{
	NNO_BLE_INCORRECT_CHARACTERISTIC_LENGTH,
	NNO_BLE_NOTIFICATION_PROCESSING_FAILED,
	NNO_BLE_MAX_ERROR_CODES
} BLE_Error_Codes;

/**
 * @enum _tagBLE_List_Return_Type_t return type definitions
 *
 * Indicates whether command handler needs to report size in bytes
 * or num entries in list
 */
typedef enum _tagBLE_List_Return_Type_t
{
	BLE_LIST_RETURN_TYPE_BYTE,
	BLE_LIST_RETURN_TYPE_NUM_ENTRIES
} BLE_List_Return_Type_t;

/**
 * \struct _bleCmdHandlerNotifyStruct
 * for BLE command notifications
 */
typedef struct _bleCmdHandlerNotifyStruct
{
	BLE_Notify_Type type;
	uint8_t			data[BLE_MAX_PACKET_SIZE];
	uint8_t			num_bytes;
} BLE_CMD_HANDLER_NOTIFY;

/**
 * \struct _blecmdHandlerInputStruct
 * input to command handler from BLE services
 */
typedef struct _blecmdHandlerInputStruct
{
	uint32_t 			cmd;
	BLE_Command_Type_t	type;
	unsigned char 		data[BLE_MAX_PACKET_SIZE];
	unsigned char 		data_len;
	uint8_t				fileType;
} BLE_CMD_HANDLER_INPUT;

/**
 * @struct _blecmdHandlerResponseStruct 
 * Structure of repsonse from command handler to BLE app from cmdproc
 *
 *
 */
typedef struct _blecmdHandlerResponseStruct
{
	uint32_t cmd;					// Commmand ID
	BLE_Command_Type_t	type;		// Command type
	unsigned char fileType;			// File type
	uint8_t	subFileType;			// sub file type
	unsigned short output_data_len;	// Indicates remaining data length
	unsigned char *output;			// Pointer to output data
	bool result;					// Indicates success/failure. TRUE = success
	unsigned short device_status;	// TIVA device status for BLE use (notify etc)
	unsigned short error_status;	// TIVA error status for BLE use (notify etc)
} BLE_CMD_HANDLER_RESPONSE;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function to indicate if an active BLE connection exists
 *
 * Function definition in BLEProfMgr.c
 */
bool isBLEConnActive();

/**
 * Function to handle BLE debug Messages
 *
 * Function definition in BLEprofmgr.c
 */
void bleHandleInternalMessage(unsigned char length, unsigned char *command, unsigned int bluetoothID,
							  unsigned int serviceID, unsigned int connectionID, unsigned int ccdOffset);

/**
 * Function to handle repsonse from stack (typically on buffer empty messages)
 * This function restart notifications for large blob of data
 *
 * Function definition in cmdHandlerLiaison.c
 */
int bleCmdHandlerLiaison_handleBLEResponse(bool isCallerBLE, unsigned int connectionID);

/**
 * Helper function to set notification data
 *
 * Function defined in BLENotificationHandler.c
 */
int bleNotificationHandler_setNotificationData(uint8_t type, int length, uint8_t *data);

int bleNotificationHandler_SendErrorIndication(uint32_t field, int16_t code);

/**
 * Helper function to send error response to client
 *
 * Function definition in BLEProfMgr.c
 */
int bleGATTErrorResponse(unsigned int BluetoothStackID, unsigned int TransactionID,
						uint16_t AttributeOffset, uint8_t ErrorCode);

#ifdef __cplusplus
}
#endif
#endif /* BLECOMMONDEFS_H_ */
