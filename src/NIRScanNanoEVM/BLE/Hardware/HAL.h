/*      Copyright 2011 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HAL - Hardware Abstraction prototypes/constants for the Tiva family       */
/*        of development boards.                                              */
/*                                                                            */

#ifndef __HALH__
#define __HALH__

   /* Add typedefs missing from TivaWare.                               */
#include <stdint.h>
#include <stdbool.h>

   /* The following defines the size of the Console UART buffers.       */
#ifndef UART_OUT_BUFFER_LEN

   #define UART_OUT_BUFFER_LEN                        2048

#endif

#ifndef UART_IN_BUFFER_LEN

   #define UART_IN_BUFFER_LEN                           32

#endif

#ifdef __cplusplus
extern "C" {
#endif

   /* The following function is used to place the hardware into a known */
   /* state.  This function accepts a single parameter which specifies  */
   /* whether or not the hardware timer (for non-multithreaded Tick     */
   /* Counts - TIMER0) should be enabled.  This parameter should be     */
   /* passed as zero (FALSE) if using an RTOS (e.g. SafeRTOS or         */
   /* FreeRTOS).                                                        */
void HAL_ConfigureHardware(int ConfigureTimerTick);

   /* The following function is used to illuminate an LED.  The number  */
   /* of LEDs on a board is board specific.  If the LED_ID provided does*/
   /* not exist on the hardware platform then nothing is done.          */
void HAL_LedOn(int LED_ID);

   /* The following function is used to extinguish an LED.  The number  */
   /* of LEDs on a board is board specific.  If the LED_ID provided does*/
   /* not exist on the hardware platform then nothing is done.          */
void HAL_LedOff(int LED_ID);

   /* The following function is used to toggle the state of an LED.  The*/
   /* number of LEDs on a board is board specific.  If the LED_ID       */
   /* provided does not exist on the hardware platform then nothing is  */
   /* done.                                                             */
void HAL_LedToggle(int LED_ID);

   /* The following function is used to read the stateof a Button.  The */
   /* number of Buttons on a board is board specifigggc.  If the button */
   /* is pressed, the return value will be non-zero.  If the BUTTON_ID  */
   /* provided does not exist on the hardware platform or the button is */
   /* not being depressed then the return value is Zero.                */
int HAL_ButtonPressed(int BUTTON_ID);

   /* The following function is used to retreive data from the UART     */
   /* input queue.  the function receives a pointer to a buffer that    */
   /* will receive the UART characters a the length of the buffer.  The */
   /* function will return the number of characters that were returned  */
   /* in Buffer.                                                        */
int HAL_ConsoleRead(int Length, char *Buffer);

   /* The following function is used to send data to the UART output    */
   /* queue.  the function receives a pointer to a buffer that will     */
   /* contains the data to send and the length of the data.  The        */
   /* function will return the number of characters that were           */
   /* successfully saved in the output buffer.                          */
int HAL_ConsoleWrite(int Length, char *Buffer);

   /* The following function is used to retreive a specific number of   */
   /* bytes from some Non Volatile memory.                              */
int HAL_NV_DataRead(int Length, unsigned char *Buffer);

   /* The following function is used to save a specific number of bytes */
   /* to some Non Volatile memory.                                      */
int HAL_NV_DataWrite(int Length, unsigned char *Buffer);

   /* The folloiwng function is used to return the current Tick Count   */
   /* value.                                                            */
unsigned long HAL_GetTickCount(void);

#ifdef __cplusplus
}
#endif
#endif

