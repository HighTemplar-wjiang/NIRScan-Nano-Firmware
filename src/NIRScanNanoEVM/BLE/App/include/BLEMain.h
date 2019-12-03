/*
 * BLE Main Thread header file
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef __BLEMAIN_H__
#define __BLEMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BLE Main task stack size
 *
 */
#define BLE_MAIN_TASK_STACK_SIZE 2048

/**
 * @brief BLE Main task priority
 *
 * The task has higher priority compared to command handler so that
 * all BLE stack can handle commands from the peer on client side.
 * The priority is kept lower than scan task so that ciritcal part
 * of scan can happen in an uninterrupted manner
 */
#define BLE_MAIN_TASK_PRIORITY	 12

/**
 * @brief BLE Main task  entry function
 *
 * The task is responsible for opening/closing BLE stack
 * This function would wait indefinitely on BLE stack open/close semaphores
 * When BLE connection is present, this function will handle response from
 * command handler and response to client
 */
void BLEThreadMain();

#ifdef __cplusplus
}
#endif
#endif	//__BLEMAIN_H__

