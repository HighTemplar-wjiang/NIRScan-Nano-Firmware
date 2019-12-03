/*
 * contains code required to configure and handle trigger signals between DMD controller and embedded processor
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef TRIGGER_H_
#define TRIGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

void Trig_Init();
long long *Trig_GetADCAccDataPtr(void);
uint16_t *Trig_GetADCAccNumPtr(void);
void Trig_FrameCallback(void);
bool Trig_IsFramePatternsComplete(int trig_vsyncCount);

#ifdef __cplusplus
}
#endif

#endif /* TRIGGER_H_ */
