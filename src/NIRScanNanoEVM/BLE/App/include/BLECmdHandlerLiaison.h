/*
 *
 * BLE Command Handler Liaison header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLECMDHANDLER_H_
#define BLECMDHANDLER_H_

/**
 * @name Command handler Liason state definitions
 *
 * 0 = Command handler is idle and available to process command from the word go...
 * 1 = Meant for read & respond directly. This should hold up the command handler
 * 2 = This indicates that a command was sent to command Handler and ble is awaiting response
 * 4 = This indicates that response was recived from command handler but notify/indicate has not been sent
 * 8 = This indicates that notify/indicate was partially sent
 */
//@{
#define BLE_CMD_HANDLER_LIAISON_STATE_IDLE 					0
#define BLE_CMD_HANDLER_LIAISON_STATE_EXEC 					1
#define BLE_CMD_HANDLER_LIAISON_STATE_WAIT_FOR_COMM_RESP	2
#define BLE_CMD_HANDLER_LIAISON_STATE_WAIT_FOR_NOTI_RESP	4
#define BLE_CMD_HANDLER_LIAISON_STATE_NOTI_RESP_INCOMP		8
//@}

/**
 * @brief Command execution state definitions
 *
 * 0 = Waiting for command handler response for size request
 * 1 = Waiting for command handler response for data request
 * 2 = Waiting for response from BLE to continue sending notification
 * 3 = Waiting for command handler response to send notification
 *
 */
typedef enum _tagBLE_Command_Exec_Status_t
{
	BLE_COMMAND_STATUS_WAIT_FOR_SIZE,
	BLE_COMMAND_STATUS_WAIT_FOR_DATA,
	BLE_COMMAND_STATUS_WAIT_FOR_PREV_PACKET_RESPONSE,
	BLE_COMMAND_STATUS_WAIT_TO_SEND_NOTIFICATION,
	BLE_COMMAND_STATUS_WAIT_TO_SEND_WRITE_RESPONSE
} BLE_Command_Exec_Status_t;

/**
 * @brief Command execution phase definitions
 *
 * 0 = Size request
 * 1 = Data request
 * 2 = Perform some task (meant for write commands)
 *
 */
typedef	enum _tagBLE_Phased_Command_Type_t
{
	BLE_COMMAND_PHASE_INVALID,	// because the array holds type as first parameter
	BLE_COMMAND_PHASE_SIZE,
	BLE_COMMAND_PHASE_DATA,
	BLE_COMMAND_PHASE_ACTION
} BLE_Phased_Command_Type_t;

/**
 * @brief Structure to hold BLE connection/transaction related information
 */
typedef struct _btInfo
{
	unsigned int	bluetoothID;
	unsigned int 	transactionID;
	unsigned int 	serviceID;
	unsigned int 	connectionID;
	unsigned short 	ccdOffset;
} BT_INFO;

/**
 * @brief Structure to hold information required to respond to client request
 */
typedef struct _bleResponseInfo
{
	unsigned int				key;
	unsigned char				fileType;
	unsigned char 				subfieldType;
	BLE_Command_Type_t			cmdType;
	unsigned char				cmdLen;
	unsigned char				cmd[BLE_MAX_COMMAND_SIZE];
	BLE_Command_Exec_Status_t	cmdStatus;
	BT_INFO 					btInfo;
	unsigned short 				totalLength;
	short 						remLength;				//remaining data length to be sent
	unsigned char				*data;
	unsigned char				currentPktIdx;
	uint8_t						dataType;				// 0 - normal data, 1 - large data blob
} BLE_RESPONSE_INFO;

/**
 * @brief Structure for response info list
 */
typedef struct _bleResponseInfoListNode
{
	BLE_RESPONSE_INFO			Info;
	struct _bleResponseInfoListNode	*next;
} BLE_RESPONSE_INFO_LIST_NODE;

/**
 * @brief BLE Command dictionary entry structure
 */
typedef struct _bledictEntry
{
	uint32_t   key;                                     /* key */
    bool (*pFunc)(uint8_t, uint8_t*);                   /* function value */
}	BLE_CMD_DICT_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization/deinitialization functions
 */
void InitBLECmdHandlerLiason();
void DeInitBLECmdHandlerLiason();

/**
 * @brief This functions relays command from BLE App to command handler
 */
int bleCmdHandlerLiason_relayCmd(Byte_t *pData, int length, BLE_RESPONSE_INFO responseInfo);

/**
 * @brief BLE debug command handler
 */
int bleCmdHandlerLiason_dbgCmdHandler(uint32_t key, uint8_t length, uint8_t *pData);

/**
 * @brief BLE debug command processing functions
 */
bool cmdYellowLED_wr(uint8_t len, uint8_t *pData);
bool cmdBatteryLife_wr(uint8_t len, uint8_t *pData);
bool cmdTempMeas_wr(uint8_t len, uint8_t *pData);
bool cmdHumMeas_wr(uint8_t len, uint8_t *pData);
bool cmdDevStat_wr(uint8_t len, uint8_t *pData);
bool cmdErrStat_wr(uint8_t len, uint8_t *pData);
bool cmdHoursOfUse_wr(uint8_t len, uint8_t *pData);
bool cmdNumBattRecharge_wr(uint8_t len, uint8_t *pData);
bool cmdTotalLampHours_wr(uint8_t len, uint8_t *pData);
bool cmdNumStoredConf_wr(uint8_t len, uint8_t *pData);
bool cmdStoreRefScanCfg_wr(uint8_t len, uint8_t *pData);
bool cmdPrintCalCoeffs_rd(uint8_t len, uint8_t *pData);
bool cmdSetSerialNumber_wr(uint8_t len, uint8_t *pData);
bool cmdSetDevErrStat_wr(uint8_t len, uint8_t *pData);

#ifdef __cplusplus
}
#endif
#endif /* BLECMDHANDLER_H_ */
