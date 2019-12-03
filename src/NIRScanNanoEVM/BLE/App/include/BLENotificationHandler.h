/*
 * BLE Notification Handler Header file
 * Handles both BLE notifications & indications
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLENOTIFICATIONHANDLER_H_
#define BLENOTIFICATIONHANDLER_H_

#include "BLECmdHandlerLiaison.h"

typedef struct _bleNotifyInfo
{
	uint8_t			*data;
	int			data_length;
	Boolean_t	data_changed;
	uint8_t type;
	BT_INFO		btInfo;		// BT info for sending notification
} BLE_NOTIFY_INFO;

typedef struct _bleNotifyInfoListNode
{
	BLE_NOTIFY_INFO			Info;
	struct _bleNotifyInfoListNode	*next;
} BLE_NOTIFY_INFO_LIST_NODE;

#ifdef __cplusplus
extern "C" {
#endif

void bleNotificationHandler_Init();
void bleNotificationHandler_DeInit();
int bleNotificationHandler_registerNotification(BLE_NOTIFY_INFO notifyInfo);
int bleNotificationHandler_deregisterNotification(uint8_t type);
int bleNotificationHandler_sendNotification();
int bleNotificationHandler_sendIndication();
int bleNotificationHandler_updateIndicationInfo(uint8_t type, unsigned int transactionID, int length);
int bleNotificationHandler_sendErrorIndication(uint32_t field, int16_t code);

#ifdef __cplusplus
}
#endif
#endif /* BLENOTIFICATIONHANDLER_H_ */
