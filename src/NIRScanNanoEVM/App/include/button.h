/*
 *
 * Copyright (C) 2014-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#ifdef __cplusplus
extern "C" {
#endif

void LockScanButton();
void UnlockScanButton();
bool IsScanButtonLocked();

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H_ */
