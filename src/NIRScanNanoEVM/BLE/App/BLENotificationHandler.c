/*
 *
 * BLE Notification Handler - Handles all notification related functions
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
#include "uartstdio.h"
#include <ti/sysbios/knl/Semaphore.h>
#include "BTTypes.h"
#include "SS1BTGAT.h"
#include "common.h"
#include "nnoStatus.h"
#include "BLEUtils.h"
#include "BLEcommonDefs.h"
#include "BLENotificationHandler.h"

BLE_NOTIFY_INFO_LIST_NODE	*bleNotificationList;		//FIFO list of notifications that have been requested by the client
extern unsigned short gBLESuppMTUSize;

/*********************** Local functions ***************************************/
int addNodeToNotifyList(BLE_NOTIFY_INFO_LIST_NODE node);
void clearNotifyListNode(BLE_NOTIFY_INFO_LIST_NODE *node);
int deleteNodefromNotificationList(BLE_Notify_Type type);

int addNodeToNotifyList(BLE_NOTIFY_INFO_LIST_NODE node)
{
	int ret_val = 0;
	BLE_NOTIFY_INFO_LIST_NODE *lastNd;

	int cnt = 0;

	// Clear data change indication flag, just in case
	node.Info.data_changed = false;

	if (bleNotificationList == NULL)
	{
		DEBUG_PRINT("\r\nNotify List is empty\r\n");
		bleNotificationList = (BLE_NOTIFY_INFO_LIST_NODE *)bleMalloc(sizeof(BLE_NOTIFY_INFO_LIST_NODE));
		if (bleNotificationList != NULL)
		{
			bleNotificationList->Info.data = (uint8_t *)bleMalloc(sizeof(uint8_t)*gBLESuppMTUSize);
			if (bleNotificationList->Info.data != NULL)
			{
				memcpy(&bleNotificationList->Info.btInfo, &node.Info.btInfo,sizeof(BT_INFO));
				bleNotificationList->Info.data_changed = node.Info.data_changed;
				bleNotificationList->Info.data_length = node.Info.data_length;
				bleNotificationList->Info.type = node.Info.type;
				memcpy(bleNotificationList->Info.data, node.Info.data, sizeof(uint8_t)*gBLESuppMTUSize);
				bleNotificationList->next = NULL;

				cnt++;
			}
		}
		else
			ret_val = FAIL;
	}
	else
	{
		DEBUG_PRINT("\r\nNotify List is not empty\r\n");
		Boolean_t nodeExists = false;
		lastNd = bleNotificationList;

		while (lastNd->next != NULL) //find last node
		{
			if (lastNd->Info.type != node.Info.type)
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
			lastNd->next = (BLE_NOTIFY_INFO_LIST_NODE *)bleMalloc(sizeof(BLE_NOTIFY_INFO_LIST_NODE));
			if (lastNd->next != NULL)
			{
				lastNd->next->Info.data = (uint8_t *)bleMalloc(sizeof(uint8_t)*gBLESuppMTUSize);
				if (lastNd->next->Info.data != NULL)
				{
					memcpy(&lastNd->next->Info.btInfo, &node.Info.btInfo,sizeof(BT_INFO));
					lastNd->next->Info.data_changed = node.Info.data_changed;
					lastNd->next->Info.data_length = node.Info.data_length;
					lastNd->next->Info.type = node.Info.type;
					memcpy(lastNd->next->Info.data, node.Info.data, sizeof(uint8_t)*gBLESuppMTUSize);
					lastNd->next->next = NULL;
					cnt++;
				}
				else
					ret_val = FAIL;
			}
			else
				ret_val = FAIL;
		}
		else
		{
			// copy over the BLE info in case that has changed
			lastNd->Info.btInfo.bluetoothID = node.Info.btInfo.bluetoothID;
			lastNd->Info.btInfo.ccdOffset = node.Info.btInfo.ccdOffset;
			lastNd->Info.btInfo.connectionID = node.Info.btInfo.connectionID;
			lastNd->Info.btInfo.serviceID = node.Info.btInfo.serviceID;

			ret_val = -1;
		}
	}

	DEBUG_PRINT("\r\nTotal number of nodes after adding to list: %d\r\n", cnt);

	return(ret_val);
}

BLE_NOTIFY_INFO_LIST_NODE *getMatchfromNotificationList(uint8_t type)
{
	BLE_NOTIFY_INFO_LIST_NODE *temp = bleNotificationList;
	while (temp != NULL)
	{
		if (temp->Info.type == type)
			break;
		else
			temp = temp->next;
	}

	return (temp);
}

int deleteNodefromNotificationList(uint8_t type)
{
	int ret_val = 0;
	int cnt = 0;
	BLE_NOTIFY_INFO_LIST_NODE *currNode = bleNotificationList;
	BLE_NOTIFY_INFO_LIST_NODE *prevNode = NULL;
	Boolean_t nodeFound = false;

	while(!nodeFound)
	{
		if (currNode == NULL)
		{
			ret_val = -1;
			break;
		}
		else if (currNode->Info.type == type)
		{
			nodeFound = true;

			if (prevNode == NULL)	//first node
				bleNotificationList = currNode->next;
			else
				prevNode->next = currNode->next;

			bleFree(currNode->Info.data, sizeof(uint8_t)*gBLESuppMTUSize);
			bleFree(currNode, sizeof(BLE_NOTIFY_INFO_LIST_NODE));
		}
		else
		{
			prevNode = currNode;
			currNode = currNode->next;
		}
	}

	/* Following code is only for printing status, so remove if required */
#if 1
	currNode = bleNotificationList;
	while (currNode != NULL)
	{
		cnt++;
		currNode = currNode->next;
	}

	DEBUG_PRINT("\r\nNo of nodes after removing node from Notification list:%d\r\n", cnt);
#endif

	return (ret_val);
}

void clearNotifyListNode(BLE_NOTIFY_INFO_LIST_NODE *node)
{
	if (node == NULL)
		return;

	if (node->next != NULL)
		clearNotifyListNode(node->next);

	bleFree(node->Info.data, sizeof(uint8_t)*gBLESuppMTUSize);
	bleFree(node, sizeof(BLE_NOTIFY_INFO_LIST_NODE));
}

/* Registration fucntions - would be invoked from GATT profile Handlers */
int bleNotificationHandler_registerNotification(BLE_NOTIFY_INFO notifyInfo)
{
	int ret_val = 0;

	BLE_NOTIFY_INFO_LIST_NODE tempNode;
	tempNode.Info.data = (uint8_t *)bleMalloc(sizeof(uint8_t)*gBLESuppMTUSize);
	if (tempNode.Info.data != NULL)
	{
		memcpy(&tempNode.Info.btInfo, &notifyInfo.btInfo,sizeof(BT_INFO));
		tempNode.Info.data_changed = notifyInfo.data_changed;
		tempNode.Info.data_length = notifyInfo.data_length;
		tempNode.Info.type = notifyInfo.type;
		if (notifyInfo.data != NULL)
			memcpy(tempNode.Info.data, notifyInfo.data, sizeof(uint8_t)*gBLESuppMTUSize);
		else
			memset(tempNode.Info.data,0,sizeof(uint8_t)*gBLESuppMTUSize);

		tempNode.next = NULL;
		ret_val = addNodeToNotifyList(tempNode);

		bleFree(tempNode.Info.data, sizeof(uint8_t)*gBLESuppMTUSize);
	}
	else
		ret_val = FAIL;

	return (ret_val);
}

int bleNotificationHandler_deregisterNotification(uint8_t type)
{
	int ret_val = 0;

	if (type < BLE_NOTIFY_MAX)
	{
		ret_val = deleteNodefromNotificationList(type);
		if (ret_val < 0)
			DEBUG_PRINT("\r\nDeregister notification failed!!!\r\n");
	}

	return (ret_val);
}

int bleNotificationHandler_sendNotification()
{
	int type = 0;
	int ret_val = 0;

	BLE_NOTIFY_INFO_LIST_NODE *node = NULL;
	node = bleNotificationList;

	while (node != NULL)
	{
		type = node->Info.type;
		if ((type < BLE_NOTIFY_MAX) && (node->Info.data_changed))
		{
			ret_val = BLEUtil_SendNotification(node->Info.btInfo.bluetoothID,
											node->Info.btInfo.serviceID,
											node->Info.btInfo.connectionID,
											node->Info.btInfo.ccdOffset,
											node->Info.data_length,
											&node->Info.data[0]);

			if (ret_val < 0)
			{
				DEBUG_PRINT("\r\nNotification of type:%d failed. Error code:%d\r\n", node->Info.type, ret_val);
			}
			else	// reset data changed flag since notification has been sent to client
				node->Info.data_changed = false;
		}

		node = node->next;
	}

	return (ret_val);
}

int bleNotificationHandler_sendIndication()
{
	uint8_t type = 0;
	int ret_val = 0;

	BLE_NOTIFY_INFO_LIST_NODE *node = NULL;
	node = bleNotificationList;

	while (node != NULL)
	{
		type = node->Info.type;

		if ((type >= BLE_NOTIFY_MAX) && (type < BLE_INDICATE_MAX) && (node->Info.data_changed))
		{
			ret_val = BLEUtil_SendIndication(node->Info.btInfo.bluetoothID,
										  node->Info.btInfo.serviceID,
										  node->Info.btInfo.connectionID,
										  node->Info.btInfo.ccdOffset,
										  node->Info.data_length,
										  &node->Info.data[0]);

			if (ret_val < 0)
			{
				DEBUG_PRINT("\r\nIndication of type:%d failed. Error code:%d\r\n", node->Info.type, ret_val);
			}
			else	// reset data changed flag since notification has been sent to client
			{
				node->Info.data_changed = false;
				node->Info.btInfo.transactionID = ret_val;
			}
		}

		node = node->next;
	}

	return (ret_val);
}

int bleNotificationHandler_updateIndicationInfo(uint8_t type, unsigned int transactionID, int length)
{
	int ret_val = 0;
	BLE_NOTIFY_INFO_LIST_NODE *node;

	if ((type < BLE_NOTIFY_MAX) || (type > BLE_INDICATE_MAX) || (length <= 0))
		return (APPLICATION_ERROR_INVALID_PARAMETERS);

	node = getMatchfromNotificationList(type);
	if (node != NULL)
	{
		if ((node->Info.btInfo.transactionID == transactionID) && (node->Info.data_length == length))
		{
			node->Info.data_changed = false;
			node->Info.btInfo.transactionID = 0;
			node->Info.data_length = 0;

			// now reset the error status so that it can be udpated
			NNO_error_status_struct *error = NULL;
			error = (NNO_error_status_struct *)(&node->Info.data[0]);
			ret_val = nnoStatus_clearErrorStatus(error->status);

			if (ret_val < 0)
			{
				DEBUG_PRINT("\r\nERROR: clearing error status failed\r\n");
			}
		}
		else
		{
			DEBUG_PRINT("\r\nERROR: Incorrect Indication Ack received\r\n");
			ret_val = APPLICATION_ERROR_INVALID_COMMAND;
		}
	}
	else
	{
		DEBUG_PRINT("\r\nERROR: No matching entry found in Notification List that matches recieved Indication Ack\r\n");
		ret_val = APPLICATION_ERROR_FUNCTION;
	}

	return (ret_val);
}

int bleNotificationHandler_setNotificationData(uint8_t type, int length, uint8_t *data)
{
	int ret_val = 0;
	int i = 0;
	BLE_NOTIFY_INFO_LIST_NODE *node;

	if (length > gBLESuppMTUSize)
		ret_val = APPLICATION_ERROR_INVALID_PARAMETERS;
	else
	{
		node = getMatchfromNotificationList(type);
		if (node != NULL)
		{
			node->Info.data_length = length;
			for(i=0;i < length;i++)
				node->Info.data[i] = data[i];
			node->Info.data_changed = true;
		}
		else
			ret_val = APPLICATION_ERROR_FUNCTION;
	}

	return (ret_val);
}

int bleNotificationHandler_relayErrorToBLE(NNO_error_status_struct *error)
{
	int ret_val = 0;

	if (0 != bleNotificationHandler_setNotificationData(BLE_INDICATE_ERROR_STATUS, sizeof(NNO_error_status_struct), (uint8_t *)error))
		ret_val = -1;

	Semaphore_post(BLENotifySem);

	return (ret_val);
}

int bleNotificationHandler_sendErrorIndication(uint32_t field, int16_t code)
{
	NNO_error_status_struct status;

	int result = PASS;

	result = nnoStatus_setErrorStatusAndCode(field,true, code);

	if (!isBLEConnActive())	//Check if active connection exists
		return (result);


	DEBUG_PRINT("\r\nIndication to be sent, field:%d code:%d\r\n", field, code);

	if (PASS == result)
	{
		if (0 == nnoStatus_getErrorStatus(&status))
		{
			result = bleNotificationHandler_relayErrorToBLE(&status);
			if (result != PASS)
				bleLogFuncError("\r\nbleNotificationHandler_sendErrorIndication", result);
		}
	}
	else
		bleLogFuncError("nnoStatus_setErrorStatusAndCode", result);

	DEBUG_PRINT("\r\nIndication sent with code:%d. result:%d\r\n", code, result);

	return (result);
}

/* Initialization/Deinitialization functions */
void bleNotificationHandler_Init()
{
	bleNotificationList = NULL;

	return;
}

void bleNotificationHandler_DeInit()
{
	clearNotifyListNode(bleNotificationList);
	bleNotificationList = NULL;

	return;
}
#endif
