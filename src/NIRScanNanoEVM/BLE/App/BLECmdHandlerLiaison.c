/*
 * Liaison between BLE & Generic TIVA Command Handler
 * Handles BLE specific command responses
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include <xdc/cfg/global.h>
#include "BTTypes.h"
#include "SS1BTGAT.h"
#include "BLEcommonDefs.h"
#include "BLECmdHandlerLiaison.h"
#include "led.h"
#include "BLEUtils.h"

#include "scan.h"
#include "nano_eeprom.h"
#include "dlpspec_scan.h"
#include "dlpspec_setup.h"
#include "nnoStatus.h"
#include "NNOCommandDefs.h"

/**
 * @brief Number of BLE command dictionaries
 */
#define BLEDICTMAX 							1

/**
 * @brief Max buffer size
 */
#define MAX_BUFFER_SIZE						512

/**
 * @brief Size of standard notifcation sent from server
 */
#define STANDARD_NOTIFICATION_SIZE			5

/**
 * @brief Max File Type
 */
#define BLE_DUMMY_COMMAND_ID	0xFFFFFF

/*************** Globals used across TIVA App ****************************/
/**
 * @brief Holds BLE command handler input from BLE app
 */
BLE_CMD_HANDLER_INPUT gBLECmdHandlerInput;

/**
 * @brief Holds BLE command handler repsonse to BLE app
 */
BLE_CMD_HANDLER_RESPONSE gBLECmdHandlerRepsonse;

/**
 * @brief Holds BLE connection status
 */
bool gIsBLEConnActive = false;

/*************** External variable definitions	*************************/
extern unsigned short gBLESuppMTUSize;

/***************** Globals used in this file  ***************************/
/**
 * @brief Holds current state of command handler liaison
 */
uint8_t currentState = BLE_CMD_HANDLER_LIAISON_STATE_IDLE;

/**
 * @brief Reference table for data type and command IDs to use for processing commands
 */
// Any new command in needs to be updated in the table below
// ** PLEASE DO NOT CHANGE THE ORDER ***
const uint32_t phasedBLECommandIDs[BLE_MAX_COMMANDS][4]	=
{
	{NNO_FILE_SCAN_DATA,		NNO_CMD_FILE_GET_READSIZE, 		NNO_CMD_FILE_GET_DATA,		0},
	{NNO_FILE_SCAN_CONFIG,		0,								NNO_CMD_SCAN_CFG_READ,		0},
	{NNO_FILE_REF_CAL_DATA,		NNO_CMD_FILE_GET_READSIZE, 		NNO_CMD_FILE_GET_DATA,		0},
	{NNO_FILE_REF_CAL_MATRIX, 	NNO_CMD_FILE_GET_READSIZE, 		NNO_CMD_FILE_GET_DATA,		0},
	{NNO_FILE_HADSNR_DATA,		0,								0,							0},
	{NNO_FILE_SCAN_CONFIG_LIST,	NNO_CMD_READ_FILE_LIST_SIZE,	NNO_CMD_READ_FILE_LIST,		0},
	{NNO_FILE_SCAN_LIST,		NNO_CMD_READ_FILE_LIST_SIZE,	NNO_CMD_READ_FILE_LIST,		0},
	{NNO_FILE_SCAN_DATA_FROM_SD,0, 								0,							0},
	{NNO_FILE_INTERPRET_DATA,   0,								0,							0},
	{NNO_START_SCAN,			0,								0,							NNO_CMD_PERFORM_SCAN},
	{NNO_CLEAR_SCAN,			0,								0,							NNO_CMD_DEL_SCAN_FILE_SD},
	{NNO_READ_TEMP,				0,								0,							NNO_CMD_READ_TEMP},
	{NNO_READ_HUM,				0,								0,							NNO_CMD_READ_HUM},
	{NNO_READ_BATT,				0,								0,							NNO_CMD_READ_BATT_VOLT},
	{NNO_NUM_SCAN_CONFIG,		0,								0,							NNO_CMD_SCAN_CFG_NUM},
	{NNO_GET_ACTIVE_SCAN_CONFIG,0,								NNO_CMD_SCAN_GET_ACT_CFG,	0},
	{NNO_SET_ACTIVE_SCAN_CONFIG,0,								0,							NNO_CMD_SCAN_SET_ACT_CFG},
	{NNO_FILE_SPEC_CAL_COEFF,	0,								NNO_CMD_CALIB_STRUCT_READ,	0},
	{NNO_SET_FILE_NAME_TAG,		0,								0,							NNO_CMD_WRITE_SCAN_NAME_TAG},
	{NNO_READ_DEV_STAT,			0,								0, 							NNO_CMD_READ_DEVICE_STATUS},
	{NNO_READ_ERR_STAT,			0,								0,							NNO_CMD_READ_ERROR_STATUS},
	{NNO_RESET_ERR_STAT,		0,								0,							NNO_CMD_RESET_ERROR_STATUS},
	{NNO_OTHER_COMMANDS,		0,								BLE_DUMMY_COMMAND_ID,		0},
};

/**
 * @brief List that stores pending responses to BLE client
 */
BLE_RESPONSE_INFO_LIST_NODE	*blePendingResponseList;		//FIFO list of reponses that are pending

/**
 * @brief Temporary buffer for storing a BLE packet data
 */
Byte_t packetData[BLE_MAX_PACKET_SIZE];

/**
 * @brief BLE debug command dictionary
 */
const BLE_CMD_DICT_ENTRY bleDictArray[] =
{
	/********* PLEASE ENSURE THAT COMMAND KEYS ARE SORTED IN ASCENDING ORDER ******/
    { CMD_KEY(0xFF, 0x00, 0x02, 0x00), cmdYellowLED_wr			},
	{ CMD_KEY(0xFF, 0x00, 0x02, 0x00), cmdYellowLED_wr			},
    { CMD_KEY(0xFF, 0x01, 0x02, 0x00), cmdBatteryLife_wr		},
    { CMD_KEY(0xFF, 0x02, 0x02, 0x00), cmdTempMeas_wr			},
    { CMD_KEY(0xFF, 0x03, 0x02, 0x00), cmdHumMeas_wr			},
    { CMD_KEY(0xFF, 0x04, 0x02, 0x00), cmdDevStat_wr			},
    { CMD_KEY(0xFF, 0x05, 0x02, 0x00), cmdErrStat_wr			},
    { CMD_KEY(0xFF, 0x06, 0x02, 0x00), cmdHoursOfUse_wr			},
    { CMD_KEY(0xFF, 0x07, 0x02, 0x00), cmdNumBattRecharge_wr	},
    { CMD_KEY(0xFF, 0x08, 0x02, 0x00), cmdTotalLampHours_wr		},
    { CMD_KEY(0xFF, 0x09, 0x02, 0x00), cmdNumStoredConf_wr		},
    { CMD_KEY(0xFF, 0x0A, 0x02, 0x00), cmdStoreRefScanCfg_wr	},
    { CMD_KEY(0xFF, 0x0C, 0x04,	0x00), cmdPrintCalCoeffs_rd		},
    { CMD_KEY(0xFF, 0x0D, 0x02, 0x00), cmdSetSerialNumber_wr  	},
	{ CMD_KEY(0xFF, 0x0E, 0x02, 0x00), cmdSetDevErrStat_wr		},
};

/**
 * @brief size of BLE command dictionary
 */
const size_t bleDictSize = sizeof( bleDictArray ) / sizeof( BLE_CMD_DICT_ENTRY );

/*********************** Local functions ***************************************/
int bleCmdHandlerLiason_saveInfoForResponse(BLE_RESPONSE_INFO info);
int addNodeToResponseList(BLE_RESPONSE_INFO_LIST_NODE node);
void clearReponseListNode(BLE_RESPONSE_INFO_LIST_NODE *node);

BLE_RESPONSE_INFO_LIST_NODE *getFirstMatchfromResponseList(bool isCallerBLE, unsigned char fileType, unsigned char subFileType, unsigned int connectionID);

int addNodeToResponseList(BLE_RESPONSE_INFO_LIST_NODE node)
/**
 * Add node to response list
 *
 * @param[in]   node	Node to be added
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;
	BLE_RESPONSE_INFO_LIST_NODE *lastNd;
	int cnt = 0;

	if (blePendingResponseList == NULL)
	{
		DEBUG_PRINT("\r\nResponse List was empty\r\n");
		blePendingResponseList = (BLE_RESPONSE_INFO_LIST_NODE *)bleMalloc(sizeof(BLE_RESPONSE_INFO_LIST_NODE));
		if (blePendingResponseList != NULL)
		{
			memcpy(blePendingResponseList, &node, sizeof(BLE_RESPONSE_INFO_LIST_NODE));
			blePendingResponseList->next = NULL;
			cnt++;
		}
		else
		{
			ret_val = FAIL;
		}
	}
	else
	{
		DEBUG_PRINT("\r\nResponse List was not empty\r\n");
		Boolean_t nodeExists = false;
		lastNd = blePendingResponseList;

		while (lastNd->next != NULL) //find last node
		{
			if ((lastNd->Info.fileType != node.Info.fileType) &&
				(lastNd->Info.cmdType != node.Info.cmdType))
				lastNd = lastNd->next;
			else
			{
				nodeExists = true;
				break;
			}
			cnt++;
		}

		if (!nodeExists)
		{
			lastNd->next = (BLE_RESPONSE_INFO_LIST_NODE *)bleMalloc(sizeof(BLE_RESPONSE_INFO_LIST_NODE));
			if (lastNd->next != NULL)
			{
				memcpy(lastNd->next, &node, sizeof(BLE_RESPONSE_INFO_LIST_NODE));
				lastNd->next->next = NULL;
				cnt++;
			}
			else
			{
				ret_val = FAIL;
			}
		}
		else
			ret_val = FAIL;
	}

	DEBUG_PRINT("\r\nTotal number of nodes after adding to list: %d\r\n", cnt);

	return(ret_val);
}

BLE_RESPONSE_INFO_LIST_NODE *getFirstMatchfromResponseList(bool isCallerBLE, unsigned char fileType, unsigned char subFileType, unsigned int connectionID)
/**
 * Finds node in response list that match input criteria
 *
 * @param[in]   isCallerBLE		To know if caller is BLE Main that sends notifcation
 * @param[in]	fileType		Type of data to be sent
 * @param[in]	subFileType		sub file type (Used for scan name, type, date etc)
 * @param[in]	connectionID	Connection ID to send notification to
 *
 * @return      Pointer to node if found, NULL otherwise
 *
 */
{
	BLE_RESPONSE_INFO_LIST_NODE *temp = blePendingResponseList;
	while (temp != NULL)
	{
		if (isCallerBLE)
		{
			if (temp->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_FOR_PREV_PACKET_RESPONSE)
				break;
			else
				temp = temp->next;
		}
		else
		{
			if (((temp->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_FOR_SIZE) ||
			     (temp->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_FOR_DATA) ||
				 (temp->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_TO_SEND_NOTIFICATION)) &&
			    (temp->Info.fileType == fileType) &&
			   ((subFileType == 0) || ((subFileType != 0) &&
			    (temp->Info.subfieldType == subFileType))))
				break;
			else
				temp = temp->next;
		}
	}

	return (temp);
}

int deleteNodefromResponseList(BLE_RESPONSE_INFO_LIST_NODE *node)
/**
 * To delete a node from response list
 *
 * @param[in]   node	Pointer to node that needs to be deleted from list
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;
	int cnt = 0;

	BLE_RESPONSE_INFO_LIST_NODE *currNode = blePendingResponseList;
	BLE_RESPONSE_INFO_LIST_NODE *prevNode = NULL;

	Boolean_t nodeFound = false;

	while(!nodeFound)
	{
		if (currNode == NULL)
		{
			ret_val = FAIL;
			break;
		}
		else if (currNode == node)
		{
			nodeFound = true;

			if (prevNode == NULL)	//first node
				blePendingResponseList = currNode->next;
			else
				prevNode->next = currNode->next;

			bleFree(currNode, sizeof(BLE_RESPONSE_INFO_LIST_NODE));
		}
		else
		{
			prevNode = currNode;
			currNode = currNode->next;
		}
	}

	currNode = blePendingResponseList;
	while (currNode != NULL)
	{
		cnt++;
		currNode = currNode->next;
	}

	DEBUG_PRINT("\r\nNo of nodes after removing node from list:%d\r\n", cnt);

	return (ret_val);
}

int bleCmdHandlerLiason_saveInfoForResponse(BLE_RESPONSE_INFO respInfo)
/**
 * To save command information to be used later while sending response
 *
 * @param[in]   respInfo	Response information structure
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;

	BLE_RESPONSE_INFO_LIST_NODE tempNode;

	memcpy(&tempNode.Info, &respInfo, sizeof(BLE_RESPONSE_INFO));
	tempNode.next = NULL;
	if (0 != addNodeToResponseList(tempNode))
		ret_val = FAIL;

	return (ret_val);
}

int bleCmdHandlerLiason_relayCmd(Byte_t *pData, int length, BLE_RESPONSE_INFO responseInfo)
/**
 * Function that is responsible for relaying the command from BLE app to BLE command handler
 *
 * @param[in]   pData			Pointer to data buffer to be sent
 * @param[in]	length			Lenght of data buffer
 * @param[in]	responseInfo	Information that needs to be stored for sending repsonse later
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;
	int i = 0;
	BLE_Phased_Command_Type_t cmdType = BLE_COMMAND_PHASE_SIZE;	// Initializing to the first valid enum values - based on static code analyzer report

	DEBUG_PRINT("\r\nBLE Cmd Handler Liason, Command:%d\r\n",responseInfo.key);

	// Copy command information to reponseInfo
	for(i=0;i<length;i++)
		responseInfo.cmd[i] = pData[i];
	responseInfo.cmdLen = length;

	responseInfo.remLength = 0;
	responseInfo.totalLength = 0;

	// Push the data to pending command handler if type warrants the same
	if ((responseInfo.cmdType == BLE_COMMAND_TYPE_WRITE_NOTIFY) || (responseInfo.cmdType == BLE_COMMAND_TYPE_WRITE_INDICATE))
	{
		if (phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_SIZE] > 0)
		{
			responseInfo.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_SIZE;
			cmdType = BLE_COMMAND_PHASE_SIZE;
		}
		else if (phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_DATA] > 0)
		{
			responseInfo.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_DATA;
			cmdType = BLE_COMMAND_PHASE_DATA;
		}

		if (responseInfo.btInfo.bluetoothID > 0)	//If data is valid, store it
		{
			if (bleCmdHandlerLiason_saveInfoForResponse(responseInfo) < 0)
				return (FAIL);
		}
	}
	else if (responseInfo.cmdType == BLE_COMMAND_TYPE_READ_DELAY_RESPONSE)
	{
		responseInfo.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_DATA;
		if (phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_SIZE] > 0)
			cmdType = BLE_COMMAND_PHASE_SIZE;
		else if (phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_DATA] > 0)
			cmdType = BLE_COMMAND_PHASE_DATA;
		else if (phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_ACTION] > 0)
			cmdType = BLE_COMMAND_PHASE_ACTION;

		if (bleCmdHandlerLiason_saveInfoForResponse(responseInfo) < 0)
			return (FAIL);
	}
	else if ((responseInfo.cmdType == BLE_COMMAND_TYPE_WRITE) || (responseInfo.cmdType == BLE_COMMAND_TYPE_WRITE_DELAYED_RESPONSE))
	{
		DEBUG_PRINT("\r\nCommand ID: %d\r\n", phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_ACTION]);
		if (responseInfo.fileType < BLE_MAX_COMMANDS)
		{
			if (responseInfo.cmdType == BLE_COMMAND_TYPE_WRITE_DELAYED_RESPONSE)
				responseInfo.cmdStatus = BLE_COMMAND_STATUS_WAIT_TO_SEND_WRITE_RESPONSE;
			else if (phasedBLECommandIDs[responseInfo.fileType][BLE_COMMAND_PHASE_ACTION] > 0)
				responseInfo.cmdStatus = BLE_COMMAND_STATUS_WAIT_TO_SEND_NOTIFICATION;
		}

		cmdType = BLE_COMMAND_PHASE_ACTION;

		if (responseInfo.btInfo.ccdOffset > 0)
		{
			bleCmdHandlerLiason_saveInfoForResponse(responseInfo);
		}
	}

	// Copy information required for preocessing the command and push control to command handler
	if ((responseInfo.fileType < BLE_MAX_COMMANDS) &&
		(phasedBLECommandIDs[responseInfo.fileType][cmdType] != BLE_DUMMY_COMMAND_ID))
		gBLECmdHandlerInput.cmd = phasedBLECommandIDs[responseInfo.fileType][cmdType];
	else
		gBLECmdHandlerInput.cmd = responseInfo.key;

	gBLECmdHandlerInput.data_len = length;
	gBLECmdHandlerInput.type = responseInfo.cmdType;
	gBLECmdHandlerInput.fileType = responseInfo.fileType;

	if (length > 0)
		memcpy(gBLECmdHandlerInput.data, pData, length);

	Semaphore_post(semBLECmdRecd);

	return (ret_val);

}

int bleCmdHandlerLiaison_handleBLEResponse(bool isCallerBLE, unsigned int connectionID)
/**
 * Function that handles BLE command handler response
 *
 * @param[in]   isCallerBLE		To know if the call is from BLE client or command handler
 * @param[in]	connectionID		Client connection ID to send the notification to
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;
	int i = 0;
	BLE_RESPONSE_INFO_LIST_NODE *node = NULL;
	unsigned int pos = 0;
	uint32_t tempLen = 0;

	// First check if any reponse if pending
	if (blePendingResponseList == NULL)
		goto _end_ble_response;

	DEBUG_PRINT("\r\nBLE Send notification \r\n");

	DEBUG_PRINT("\r\nis Caller BLE: %d, file type:%d, sub file tyep:%d, connection ID:%d\r\n", isCallerBLE, gBLECmdHandlerRepsonse.fileType, gBLECmdHandlerRepsonse.subFileType, connectionID);

	node = getFirstMatchfromResponseList(isCallerBLE, gBLECmdHandlerRepsonse.fileType, gBLECmdHandlerRepsonse.subFileType, connectionID);

	if (node == NULL)			//Nothing to process
	{
		DEBUG_PRINT("\r\nError: No matching entry found in Response List\r\n");
		goto _end_ble_response;
	}

	if (!isCallerBLE)
	{
		if (node->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_FOR_SIZE)
		{
			DEBUG_PRINT("\r\nResponse Handler: Size returned\r\n");
			if (gBLECmdHandlerRepsonse.output_data_len == 4)
			{
				node->Info.totalLength = *(gBLECmdHandlerRepsonse.output + 0) |
										 (*(gBLECmdHandlerRepsonse.output + 1) << 8) |
										 (*(gBLECmdHandlerRepsonse.output + 2) << 16) |
										 (*(gBLECmdHandlerRepsonse.output + 3) << 24);
				node->Info.remLength = node->Info.totalLength;

				DEBUG_PRINT("\r\nSize returned=%d\r\n", node->Info.totalLength);

				gBLECmdHandlerInput.cmd = phasedBLECommandIDs[node->Info.fileType][BLE_COMMAND_PHASE_DATA];
				gBLECmdHandlerInput.data_len = node->Info.cmdLen;
				gBLECmdHandlerInput.type = node->Info.cmdType;

				if (gBLECmdHandlerInput.data_len > 0)
					memcpy(gBLECmdHandlerInput.data, &node->Info.cmd[0], gBLECmdHandlerInput.data_len);

				node->Info.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_DATA;

				Semaphore_post(semBLECmdRecd);

				DEBUG_PRINT("\r\nSent request for data\r\n");

			}
		}
		else if (node->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_FOR_DATA)
		{
			pos = 0;

			DEBUG_PRINT("\r\nResponse Handler: Data returned, size=%d\r\n",gBLECmdHandlerRepsonse.output_data_len);

			node->Info.currentPktIdx = 0;
			node->Info.data = gBLECmdHandlerRepsonse.output;
			node->Info.remLength = gBLECmdHandlerRepsonse.output_data_len;

			DEBUG_PRINT("\r\nResponse Handler: data length to be sent=%d\r\n",node->Info.remLength);

			if ((node->Info.cmdType == BLE_COMMAND_TYPE_WRITE_NOTIFY) || (node->Info.cmdType == BLE_COMMAND_TYPE_WRITE_INDICATE))
			{
				if (node->Info.dataType == 1)
				{
					packetData[0] =  0;
					if (node->Info.totalLength > 0)
						tempLen = node->Info.totalLength;
					else
						tempLen = node->Info.remLength;

					if (tempLen > 0)
					{
						packetData[1] = (0xff & tempLen);
						packetData[2] = (0xff00 & tempLen) >> 8;
						packetData[3] = (0xff0000 & tempLen) >> 16;
						packetData[4] = (0xff000000 & tempLen) >> 24;

						ret_val = BLEUtil_SendNotification(node->Info.btInfo.bluetoothID,
														node->Info.btInfo.serviceID,
														node->Info.btInfo.connectionID,
														node->Info.btInfo.ccdOffset,
														STANDARD_NOTIFICATION_SIZE,
														&packetData[0]);

						if (ret_val < 0)
							DEBUG_PRINT("\r\nData size packet: BLE notification failed, ret_val:%d\r\n",ret_val);
					}
				}

				while (node->Info.remLength > 0)
				{
					int length = 0;
					for(i=0;i<gBLESuppMTUSize;i++)
						packetData[i] = 0;

					if (node->Info.dataType == 1)
					{
						length = (node->Info.remLength > (gBLESuppMTUSize - 1)) ? (gBLESuppMTUSize - 1) : node->Info.remLength;
						packetData[0] =  (0xff & (node->Info.currentPktIdx+1));
						memcpy(&packetData[1],&gBLECmdHandlerRepsonse.output[pos],length);
						length++;
					}
					else
					{
						length = (node->Info.remLength > gBLESuppMTUSize) ? gBLESuppMTUSize : node->Info.remLength;
						memcpy(&packetData[0],&gBLECmdHandlerRepsonse.output[pos],length);
					}

					ret_val = BLEUtil_SendNotification(node->Info.btInfo.bluetoothID,
													node->Info.btInfo.serviceID,
													node->Info.btInfo.connectionID,
													node->Info.btInfo.ccdOffset,
													length,
													&packetData[0]);
					if (ret_val == 0)
					{
						if (node->Info.dataType == 1)
						{
							node->Info.remLength -= (length-1);
							pos += (length - 1);
						}
						else
						{
							node->Info.remLength -= length;
							pos += length;
						}
						node->Info.currentPktIdx++;

						DEBUG_PRINT("\r\n#%d Packet sent, remaining data:%d , pos:%d\r\n", node->Info.currentPktIdx, node->Info.remLength, pos);

					}
					else
					{
						if (node->Info.remLength > 0)
							node->Info.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_PREV_PACKET_RESPONSE;

						DEBUG_PRINT("\r\nData packet ID %d: BLE notification failed, ret_val:%d\r\n", node->Info.currentPktIdx, ret_val);
						break;
					}
				}

				// For write-notify type, delete the node since the data has been sent completely already
				if ((node->Info.remLength == 0) && (node->Info.cmdType == BLE_COMMAND_TYPE_WRITE_NOTIFY))
				{
					deleteNodefromResponseList(node);
					gBLECmdHandlerRepsonse.output_data_len = 0;
				}
			}
			else if (node->Info.cmdType == BLE_COMMAND_TYPE_READ_DELAY_RESPONSE)
			{
				DEBUG_PRINT("\r\nBLE read response\r\n");

				if (gBLECmdHandlerRepsonse.output_data_len > gBLESuppMTUSize)
				{
					DEBUG_PRINT("\r\nError: Data size exceeds max supported MTU size\r\n");
					ret_val = FAIL;
					goto _end_ble_response;
				}

				for(i=0;i<gBLESuppMTUSize;i++)
					packetData[i] = 0;

				memcpy(&packetData[0],&gBLECmdHandlerRepsonse.output[0],gBLECmdHandlerRepsonse.output_data_len);

				ret_val = BLEUtil_SendReadResponse(node->Info.btInfo.bluetoothID,
												 node->Info.btInfo.transactionID,
												 gBLECmdHandlerRepsonse.output_data_len,
												 &packetData[0]);

				if (ret_val == 0)
					node->Info.remLength = 0;
				else
					DEBUG_PRINT("\r\nBLE send read response failed, ret_val:%d\r\n", ret_val);
			}

			if (ret_val == 0)
			{
				if (node->Info.remLength > 0)
					node->Info.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_PREV_PACKET_RESPONSE;
				else
				{
					if (0 > deleteNodefromResponseList(node))
					{
						DEBUG_PRINT("\r\nDeleting node failed!!\r\n");
						ret_val = FAIL;
					}
				}
			}
			DEBUG_PRINT("\r\nForwarded data to BLE\r\n");
		}
		else if (node->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_TO_SEND_NOTIFICATION)
		{
			for (i = 0; i < gBLECmdHandlerRepsonse.output_data_len; i++)
				packetData[i] = gBLECmdHandlerRepsonse.output[i];

			ret_val = BLEUtil_SendNotification(node->Info.btInfo.bluetoothID,
											node->Info.btInfo.serviceID,
											node->Info.btInfo.connectionID,
											node->Info.btInfo.ccdOffset,
											gBLECmdHandlerRepsonse.output_data_len,
											&packetData[0]);
			if (ret_val < 0)
				DEBUG_PRINT("\r\nStatus update: BLE notification failed, ret_val:%d\r\n",ret_val);

		}
		else if (node->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_TO_SEND_WRITE_RESPONSE)
		{
			DEBUG_PRINT("\r\nWrite response to be sent\r\n");

			ret_val = BLEUtil_SendWriteResponse(node->Info.btInfo.bluetoothID,
											   node->Info.btInfo.transactionID);
			if (ret_val < 0)
				DEBUG_PRINT("\r\nStatus update: BLE write response failed, ret_val:%d\r\n",ret_val);

		}
	}
	else if (node->Info.cmdStatus == BLE_COMMAND_STATUS_WAIT_FOR_PREV_PACKET_RESPONSE)	// prompt from BLE
	{
		//short remdataLen = node->Info.remLength;
		unsigned pos = 0;
		pos = node->Info.totalLength - node->Info.remLength;
		//packetData = NULL;

		DEBUG_PRINT("\r\nSubsequent iteration to send data, Data length remaining:%d\r\n", node->Info.remLength);

		while (node->Info.remLength > 0)
		{
			int length = (node->Info.remLength > (gBLESuppMTUSize - 1)) ? (gBLESuppMTUSize - 1) : node->Info.remLength;

			for (i = 0; i < gBLESuppMTUSize; i++)
				packetData[i] = 0;

			packetData[0] = (0xff & (node->Info.currentPktIdx + 1));
			memcpy(&packetData[1], &gBLECmdHandlerRepsonse.output[pos], length);
				ret_val = BLEUtil_SendNotification(node->Info.btInfo.bluetoothID, node->Info.btInfo.serviceID, node->Info.btInfo.connectionID, node->Info.btInfo.ccdOffset, length + 1, &packetData[0]);
			if (ret_val == 0)
			{
				node->Info.remLength -= (gBLESuppMTUSize - 1);
				pos += (gBLESuppMTUSize - 1);
				node->Info.currentPktIdx++;
				DEBUG_PRINT("\r\n#%d Packet sent, remaining data:%d , pos:%d\r\n", node->Info.currentPktIdx, node->Info.remLength, pos);
				//for (i=0;i<gBLESuppMTUSize;i++)
				//	DEBUG_PRINT("Data at index %d:%d\r\n", i, packetData[i]);

			}
			else
			{
				DEBUG_PRINT("\r\nBLE Send notification failed for packet %d, ret_val:%d\r\n", node->Info.currentPktIdx, ret_val);
				break;
			}
		}

		if (node->Info.remLength > 0)
				node->Info.cmdStatus = BLE_COMMAND_STATUS_WAIT_FOR_PREV_PACKET_RESPONSE;
		else
			deleteNodefromResponseList(node);

		DEBUG_PRINT("\r\nForwarded subsequent packets to BLE\r\n");
	}
	else
		DEBUG_PRINT("\r\nWarning: No matching node with expected state found in Response List\r\n");

_end_ble_response:
	return (ret_val);
}

int bleCmdHandlerLiason_dbgCmdHandler( uint32_t key, uint8_t length, uint8_t *pData)
/**
 * BLE debug command handler
 *
 * @param[in]   key			Command key
 * @param[in]	length		Data length
 * @param[in]	pData		Data buffer
 *
 * @return      Success=0, Failure <0
 *
 */
{
	size_t low = 0;
	size_t high = bleDictSize;
	size_t probe = 0;

	DEBUG_PRINT("\r\nCommand handler parameters:%d, %d\r\n",key,length);
	int i =0;
	for (i=0;i<length;i++)
		DEBUG_PRINT("\r\nCommand handler data %d:%d, %d\r\n",i,pData[i]);

	while ((high - low) > 1) /* binary search */
	{
		probe = (high + low) / 2;

		if (bleDictArray[probe].key > key)
			high = probe;
		else
			low = probe;

	}

	if ((low == 0) && (bleDictArray[low].key != key))
	{
		return (FAIL);
	}
	else if ((*bleDictArray[low].pFunc)(length, pData)) /* exec */
	{
		DEBUG_PRINT("\r\nCommand handled successfully\r\n");
		return (0); /* no handler error */
	}
	else
	{
		DEBUG_PRINT("\r\nBLE command handler returned error\r\n");
		return (1); /* if handler error */
	}
}

void InitBLECmdHandlerLiason()
/**
 * Initialize BLE command handler liaison
 *
 * @return      None
 *
 */
{
	blePendingResponseList = NULL;
	currentState = BLE_CMD_HANDLER_LIAISON_STATE_IDLE;

	return;
}

void clearReponseListNode(BLE_RESPONSE_INFO_LIST_NODE *node)
/**
 * Function to delete response list node
 *
 * @param[in]   node	Pointer to node to be deleted
 *
 * @return      None
 *
 */
{
	if (node == NULL)
		return;

	if (node->next != NULL)
		clearReponseListNode(node->next);

	bleFree(node, sizeof(BLE_RESPONSE_INFO_LIST_NODE));
}

void DeInitBLECmdHandlerLiason()
/**
 * Used to reset command handler liaison for the subsequent connection
 *
 * @return	None
 *
 */
{
	clearReponseListNode(blePendingResponseList);
	blePendingResponseList = NULL;
	currentState = BLE_CMD_HANDLER_LIAISON_STATE_IDLE;

	return;
}

bool cmdStoreRefScanCfg_wr(uint8_t len, uint8_t *pData)
/**
 * Debug function to store a reference scan config in device
 *
 * @param[in]   len		Data length
 * @param[in]	pData	Pointer to data
 *
 * @return      Success=TRUE, Failure=FALSE
 *
 */
{
	uScanConfig goldenCfg;
	char cfgName[] = "Scan Config 1";

	goldenCfg.scanCfg.num_patterns = 228;
	goldenCfg.scanCfg.width_px = 6;
	goldenCfg.scanCfg.scan_type = 0;
	goldenCfg.scanCfg.num_repeats = 6;
	goldenCfg.scanCfg.wavelength_start_nm = MIN_WAVELENGTH;
	goldenCfg.scanCfg.wavelength_end_nm = MAX_WAVELENGTH;
	memcpy(&goldenCfg.scanCfg.config_name[0],&cfgName[0],sizeof(cfgName)/sizeof(char));

	if (0 != Nano_eeprom_EraseAllConfigRecords())
	{
		DEBUG_PRINT("\r\nFailed erasing scan configurations\r\n");
		return false;
	}
	else
		DEBUG_PRINT("\r\nSuccessfully erased scan configurations\r\n");

	if ( 0 == Nano_eeprom_SaveConfigRecord(0, &goldenCfg))
	{
		DEBUG_PRINT("\r\nSaved configuration successfully\r\n");
	}
	else
	{
		DEBUG_PRINT("\r\nSaving configuration failed\r\n");
		return false;
	}

	return true;
}

bool cmdPrintCalCoeffs_rd(uint8_t len, uint8_t *pData)
/**
 * Print stored calibration coefficients
 *
 * @param[in]   len		Length of data
 * @param[in]	pData	Pointer to data
 *
 * @return      Success=TRUE, Failure =FALSE
 *
 */
{
	calibCoeffs calCoeffs;
	int temp = 0;

	if (0 == Nano_eeprom_GetcalibCoeffs(&calCoeffs))
	{
		temp = calCoeffs.ShiftVectorCoeffs[0] * 1000000;
		bleLog("\r\nShift coeff 0 (x10^-6):%d\r\n",temp);
		temp = calCoeffs.ShiftVectorCoeffs[1] * 1000000;
		bleLog("\r\nShift coeff 1 (x10^-6):%d\r\n",temp);
		temp = calCoeffs.ShiftVectorCoeffs[2] * 1000000;
		bleLog("\r\nShift coeff 2 (x10^-6):%d\r\n",temp);

		temp = calCoeffs.PixelToWavelengthCoeffs[0] * 1000000;
		bleLog("\r\nWavelength coeff 0 (x10^-6):%d\r\n",temp);
		temp = calCoeffs.PixelToWavelengthCoeffs[1] * 1000000;
		bleLog("\r\nWavelength coeff 1 (x10^-6):%d\r\n",temp);
		temp = calCoeffs.PixelToWavelengthCoeffs[2] * 1000000;
		bleLog("\r\nWavelength coeff 2 (x10^-6):%d\r\n",temp);
	}
	else
		bleLog("\r\nERROR : Reading Cal coefficients from EEPROM failed!!!");

	return true;
}

bool cmdSetSerialNumber_wr(uint8_t len, uint8_t *pData)
/**
 * Debug function to set a device serial number
 *
 * @param[in]   len		Length of data
 * @param[in]	pData	Pointer to data
 *
 * @return      Success=TRUE, Failure=FALSE
 *
 */
{
	if (len > EEPROM_SERIAL_NUMBER_SIZE)
		return false;

	if (0 != Nano_eeprom_SaveDeviceSerialNumber(pData))
		return false;

	return true;
}

bool cmdSetDevErrStat_wr(uint8_t len, uint8_t *pData)
/**
 * Debug function to write a specific error status (for testing)
 *
 * @param[in]   len		Length of data pointer
 * @param[in]	pData	Pointer to data
 *
 * @return      Success=TRUE, Failure=FALSE
 *
 */
{
	uint32_t field = 0;
	uint16_t code = 0;
	uint8_t type = 0;
	int ret_val = 0;

	if ((len == 0 ) || (pData == NULL))
		return false;

	type = pData[0];

	if (((type == 0) && (len > 5)) ||
		((type == 1) && (len > 7)))
		return false;

	if (type == 0)	//device status
	{
		memcpy(&field, &pData[1],sizeof(uint32_t));
		ret_val = nnoStatus_setDeviceStatus(field, true);
	}
	else if (type == 1) //error status
	{
		memcpy(&field, &pData[1],sizeof(uint32_t));
		memcpy(&code, &pData[5], sizeof(uint16_t));

		ret_val = nnoStatus_setErrorStatusAndCode(field, true, code);
	}

	if (0 == ret_val)
		return true;
	else
		return false;

}
#endif
