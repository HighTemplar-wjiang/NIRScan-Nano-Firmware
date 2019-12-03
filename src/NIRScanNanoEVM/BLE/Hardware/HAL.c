/******************************************************************************/
/*      Copyright 2011 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HAL - Hardware Abstraction function for Tiva boards.                      */
/*	******** ADAPTED FOR NIR SCAN NANO *********							  */
/*                                                                            */

   /* Now include the correct source file that is really required based */
   /* on the compiler search path.                                      */
   /* * NOTE * This is being done because some IDE build environments   */
   /*          (namely RealView MDK and IAR EWARM do not allow the      */
   /*          ability to specify either environment variables to the   */
   /*          correct paths OR allow the ability to support files of   */
   /*          the same names (but different paths) using different     */
   /*          project configurations.                                  */
#include <stdbool.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include "HALCFG.c"
#endif
