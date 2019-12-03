/*
 * BLE GATT Profile - General Information Service utility functions
 * These functions are used in other BLE modules
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#ifndef BLEGATTGISVC_UTILFUNC_H_
#define BLEGATTGISVC_UTILFUNC_H_

/**
 * Functions for setting system parameters supported
 * by General Information service
 */
#ifdef __cplusplus
extern "C" {
#endif

int GATTGISvc_SetTemp(short temp, bool sendNotification);
int GATTGISvc_SetHum(unsigned short hum, bool sendNotification);
int GATTGISvc_SetDevStat(unsigned short devStat);
int GATTGISvc_SetErrStat(unsigned short errStat);
int GATTGISvc_SetNumHoursOfUse(unsigned short numHrs);
int GATTGISvc_SetBattRecharge(unsigned short numBattRechg);
int GATTGISvc_SetLampHours(unsigned short lampHrs);

#ifdef __cplusplus
}
#endif
#endif /* BLEGATTGISVC_UTILFUNC_H_ */
