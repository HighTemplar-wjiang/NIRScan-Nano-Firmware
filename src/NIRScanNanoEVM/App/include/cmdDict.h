/*
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef CMDDICT_H_
#define CMDDICT_H_

/****************************************************/
/* Arrays of CMD_DICT_ENTRY define command IDs and  */
/* command handlers.                                */
/****************************************************/
typedef struct _dictEntry
{
   uint32_t   key;          /* key */
   bool (*pFunc)( void );	/* function value */
}
CMD_DICT_ENTRY;

#ifdef __cplusplus
extern "C" {
#endif

CMD1_TYPE cmdDict_Vector(CMD1_TYPE ctype, uint32_t key);

#ifdef __cplusplus
}
#endif

#endif /* CMDDICT_H_ */
