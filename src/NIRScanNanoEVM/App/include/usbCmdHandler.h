/*
 * This file hosts the task to handle commands recieved through
 * USB interface
 *
 * Copyright (C) 2006-2015 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
*/

#ifndef __USBCMDHANDLER_H
#define __USBCMDHANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

#define HID_MAX_PKT_SIZE    64
#define CMD_PACKETS_TIMEOUT 1000

/****************************************************/
/* command byte 1 definitions.    */
/****************************************************/

typedef enum usbpacketstate
{
	HEADER,
	PRJCTRLCMD,
	RFCCMD,
	MSGINVALID
}
CMD_PACKET_STATE;

/****************************************************************************/
/* Message parsing functions and macros.                                    */
/****************************************************************************/

extern int16_t  nRemReadPC;     /* number of message bytes remaining to read  */
extern int16_t  nRemWritePC;    /* number of message bytes remaining to write */

extern uint8_t *rdp;                /* pointer to next message byte to read  */
extern uint8_t *wrp;                /* pointer to next message byte to write */

#define cmdRemRead()  ( nRemReadPC  )       /* no. bytes remaining to read  */
#define cmdRemWrite() ( nRemWritePC )       /* no. bytes remaining to write */

#define cmdGetPtrRead()  ( rdp )           /* pointer to next byte to read  */
#define cmdGetPtrWrite() ( wrp )           /* pointer to next byte to write */

                        /****************************************************/
                        /* Following functions fetch/store 'nBytes' from/to */
                        /* the current projector control message. Value is  */
                        /* copied to/from the caller's environment.         */
                        /*                                                  */
                        /* returns:                                         */
                        /*   FALSE - No bytes available to fetch/store.     */
                        /*    TRUE - Success.                               */
                        /*                                                  */
                        /* Note that these functions update success flags   */
                        /* and report an ACK back to the host application   */
                        /* if a fetch/store fails.                          */
                        /****************************************************/

bool cmdGetUSB( int32_t nBytes, void *parm );
bool cmdPutUSB( int32_t nBytes, void *parm );

                        /****************************************************/
                        /* Following functions bump the read/write pointers */
                        /* when the caller has directly read/written the    */
                        /* message buffer instead of using the serialized   */
                        /* accessors.                                       */
                        /*                                                  */
                        /* returns:                                         */
                        /*   FALSE - No bytes available to increment by 'n' */
                        /*    TRUE - Success.                               */
                        /****************************************************/

bool cmdIncRead( int32_t nBytes );    /* increment read  pointer by 'n' bytes */
bool cmdIncWrite( int32_t nBytes );   /* increment write pointer by 'n' bytes */


/****************************************************/
/* Following macros perform the same function as a  */
/* cmdPut() function, using a single parameter.     */
/****************************************************/

void usbConn();
void usbDisc();
int32_t cmdRecv(void *msgData, int32_t dataLen);

#ifdef __cplusplus
}
#endif


#endif // __USBCMDHANDLER_H
