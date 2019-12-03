/*****< ss1btgat.h >***********************************************************/
/*      Copyright 2011 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  SS1BTGAT - Stonestreet One Bluetooth Generic Attribute (GATT) Profile     */
/*             Type Definitions, Prototypes, and Constants.                   */
/*                                                                            */
/*  Author:  Tim Cook                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   07/26/11  T. Cook        Initial creation.                               */
/******************************************************************************/
#ifndef __SS1BTGATH__
#define __SS1BTGATH__

  /* If building with a C++ compiler, make all of the definitions in    */
  /* this header have a C binding.                                      */
#ifdef __cplusplus

extern "C"
{

#endif

#include "ATTTypes.h"            /* Bluetooth ATT Protocol Type Definitions.  */
#include "GATTType.h"            /* Bluetooth GATT Profile Type Definitions.  */
#include "GATTAPI.h"             /* Bluetooth GATT Profile API Prototypes.    */

   /* Mark the end of the C bindings section for C++ compilers.         */
#ifdef __cplusplus

}

#endif

#endif
