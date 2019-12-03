/*****< btpskrnl.h >***********************************************************/
/*      Copyright 2000 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSKRNL - Stonestreet One Bluetooth Stack Kernel Implementation Type     */
/*             Definitions, Constants, and Prototypes.                        */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   05/30/01  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BTPSKRNLH__
#define __BTPSKRNLH__

#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "BTTypes.h"            /* Bluetooth basic type definitions           */

   /* The following declared type represents the Prototype Function for */
   /* a function that can be registered with the BTPSKRNL module to     */
   /* receive output characters.  This function will be called whenever */
   /* BTPS_OutputMessage() or BTPS_DumpData() is called (or if debug is */
   /* is enabled - DEBUG preprocessor symbol, whenever the DBG_MSG() or */
   /* DBG_DUMP() MACRO is used and there is debug data to output.       */
   /* * NOTE * This function can be registered by passing the address   */
   /*          of the implementation function in the                    */
   /*          MessageOutputCallback member of the                      */
   /*          BTPS_Initialization_t structure which is passed to the   */
   /*          BTPS_Init() function.  If no function is registered then */
   /*          there will be no output (i.e. it will simply be ignored).*/
typedef void (BTPSAPI *BTPS_MessageOutputCallback_t)(char DebugCharacter);

   /* The following structure represents the structure that is passed   */
   /* to the BTPS_Init() function to notify the Bluetooth abstraction   */
   /* layer of the function(s) that are required for proper device      */
   /* functionality.                                                    */
typedef struct _tagBTPS_Initialization_t
{
   BTPS_MessageOutputCallback_t MessageOutputCallback;
} BTPS_Initialization_t;

#define BTPS_INITIALIZATION_SIZE                         (sizeof(BTPS_Initialization_t))

#include "BKRNLAPI.h"           /* Bluetooth Kernel Prototypes/Constants.     */

   /* Defines the maximum number of bytes that will be allocated by the */
   /* kernel abstraction module to support allocations.                 */
   /* * NOTE * This module declares a memory array of this size (in     */
   /*          bytes) that will be used by this module for memory       */
   /*          allocation.                                              */
#define BTPS_MEMORY_BUFFER_SIZE      (6*1024)

#endif
