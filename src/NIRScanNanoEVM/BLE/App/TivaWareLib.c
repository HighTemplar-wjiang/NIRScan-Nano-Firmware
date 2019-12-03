/******************************************************************************/
/*      Copyright 2013 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  TivaWareLib - TivaWare Driver Libary container.                           */
/*                                                                            */

   /* ***************************** NOTE ****************************** */
   /* *                                                               * */
   /* * This file is simply a container file that is used to include  * */
   /* * all Tiva Ware drivers and utility modules that are            * */
   /* * required by the application.  This container is needed        * */
   /* * because some supported compilers (RVMDK and IAR) to not allow * */
   /* * referencing source files via environment variables (or        * */
   /* * something similar to Workspace Linked Resources under         * */
   /* * Eclipse based tools).  Because of this, this container will   * */
   /* * server as a simple way to build ALL required Tiva Ware        * */
   /* * driver/utility code in a neat, simple container.  Thie        * */
   /* * mechanism will work because the include paths *MUST* be set   * */
   /* * to reference the Tiva Ware headers *AND * all compiler        * */
   /* * IDE's allow specifying include paths based upon environment   * */
   /* * variables.                                                    * */
   /* *                                                               * */
   /* ***************************** NOTE ****************************** */

   /* Include TivaWare sprintf() replacement utility functions.         */
#include <stdbool.h>
#include "common.h"
#ifdef NIRSCAN_INCLUDE_BLE
#include "utils/ustdlib.c"
#endif

