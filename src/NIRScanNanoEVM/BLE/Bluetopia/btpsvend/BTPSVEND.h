/*****< btpsvend.h >***********************************************************/
/*      Copyright 2098 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSVEND - Vendor specific functions/definitions/constants used to define */
/*             a set of vendor specific functions supported by the Bluetopia  */
/*             Protocol Stack.  These functions may be unique to a given      */
/*             hardware platform.                                             */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   06/09/09  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BTPSVENDH__
#define __BTPSVENDH__

  /* If building with a C++ compiler, make all of the definitions in    */
  /* this header have a C binding.                                      */
#ifdef __cplusplus

extern "C"
{

#endif

#include "BVENDAPI.h"           /* BTPS Vendor Specific Prototypes/Constants. */

   /* Mark the end of the C bindings section for C++ compilers.         */
#ifdef __cplusplus

}

#endif

#endif
