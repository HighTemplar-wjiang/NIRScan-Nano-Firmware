/*      Copyright 2013 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HALCFG - Hardware Abstraction functions for Tiva DK-TM4C129X Board.       */
/*                                                                            */

#include "BTPSKRNL.h"

#include "HAL.h"
#include "HALCFG.h"

#include "inc/tm4c129xnczad.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/flash.h"
#include "driverlib/hibernate.h"

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"

#ifdef __USE_TI_RTOS__

#include <xdc/std.h>              /* Sys/Bios Standard Header.                */
#include <xdc/runtime/Error.h>    /* Sys/Bios Error Header.                   */

#include <ti/sysbios/hal/Hwi.h>

#endif

#include "utils/ustdlib.h"

   /* This section of the file constains MACRO's that configure the     */
   /* Ports and GPIO's that are used for the LED's.                     */
   /*                                                                   */
   /* Function     Port/Pin                                             */
   /* --------     --------                                             */
   /*   LED0         PH7 -> Blue                                        */
   /*   LED1         PL4 -> Yellow                                      */
   /*                                                                   */

   /* The following defines that port address and Mask Values that are  */
   /* used to access the User LED.                                      */
/* 
 * Review comment - EP
 * Should the LED pin definitions be redefined to those already in "GPIO Mapping.h"?
 * Seems some of these pins may be connected to other functions and cause problems
 * if someone called these functions.
 */
#define LED_PORT_0_BASE                                  (GPIO_PORTH_BASE)
#define LED_PORT_0_PIN                                        (GPIO_PIN_7)
#define LED_PORT_1_BASE                                  (GPIO_PORTL_BASE)
#define LED_PORT_1_PIN                                        (GPIO_PIN_4)

   /* The follwing defines a structure that is used to produce a        */
   /* circular buffer used to manage data to/from the UART.             */
typedef struct _tagUART_Buffer_t
{
   unsigned int   InIndex;
   unsigned int   OutIndex;
   unsigned int   BufferSize;
   unsigned int   BytesFree;
   unsigned char *Buffer;
} UART_Buffer_t;

#define UART_BUFFER_DATA_SIZE                        (sizeof(UART_Buffer_t))

   /* Internal state variables.                                         */
static UART_Buffer_t InBuf;
static UART_Buffer_t OutBuf;
static unsigned char InBuffer[UART_IN_BUFFER_LEN];
static unsigned char OutBuffer[UART_OUT_BUFFER_LEN];

static unsigned long TickCount;

   /* Internal function prototypes.                                     */
static int ReadFlash(unsigned int Length, unsigned char *Dest);
static int WriteFlash(unsigned int Length, unsigned char *Src);

static void HAL_RxInterrupt(void);
static void HAL_TxInterrupt(void);

   /* The following function is used to read chunck of data from the    */
   /* serial flash.  The function will transfer the data from address   */
   /* Zero of the flash for as many bytes as defined by the Length      */
   /* Parameter.  The second parameter specified a buffer to where the  */
   /* data will be transferred.                                         */
static int ReadFlash(unsigned int Length, unsigned char *Dest)
{
   return(0);
}

   /* The following function is used to transfer a chunck of data to the*/
   /* serail flash, starting at flash address Zero.  The first parameter*/
   /* defines the number of bytes to be transferred.  The second        */
   /* parameter pointer to a buffer that holds the data that is to be   */
   /* transferred.                                                      */
static int WriteFlash(unsigned int Length, unsigned char *Src)
{
   return(0);
}

   /* The following function is the function that the Console UART      */
   /* interrupt handler calls to process an incoming UART Receive       */
   /* Interrupt.                                                        */
static void HAL_RxInterrupt(void)
{
   unsigned char ch;

   while(!(HWREG(UART4_BASE + UART_O_FR) & UART_FR_RXFE))
   {
      /* Read the character from the UART port.                      */
      ch = (unsigned char)HWREG(UART4_BASE + UART_O_DR);

      /* If there is free space in the buffer, then store the        */
      /* character.                                                  */
      if(InBuf.BytesFree)
      {
         InBuf.Buffer[InBuf.InIndex++] = ch;

         /* Adjust the character counts and check to see if the index*/
         /* needs to be wrapped.                                     */
         InBuf.BytesFree--;
         if(InBuf.InIndex == InBuf.BufferSize)
            InBuf.InIndex = 0;
      }
   }
}

   /* The following function is the function that the Console UART      */
   /* interrupt handler calls to process an outgoing UART Transmit      */
   /* Interrupt.                                                        */
static void HAL_TxInterrupt(void)
{
   /* Verify that there is room for another character in the Tx FIFO.*/
   while((HWREG(UART4_BASE + UART_O_FR) & UART_FR_TXFF) == 0)
   {
      /* Check to see if there is a character to send.               */
      if(OutBuf.BytesFree != OutBuf.BufferSize)
      {
         /* Place the next character into the output buffer.         */
         HWREG(UART4_BASE + UART_O_DR) = OutBuf.Buffer[OutBuf.OutIndex++];

         /* Adjust the character counts and check to see if the index*/
         /* needs to be wrapped.                                     */
         OutBuf.BytesFree++;
         if(OutBuf.OutIndex == OutBuf.BufferSize)
            OutBuf.OutIndex = 0;
      }
      else
      {
         /* There are no more characters to send so disable the      */
         /* transmit interrupt.                                      */
         UARTIntDisable(UART4_BASE, UART_INT_TX);
         break;
      }
   }
}

   /* Error Handler for Tiva Ware driver library functions.        */
#ifdef DEBUG

void __error__(char *pcFilename, unsigned long ulLine)
{
}

#endif

   /* The following function handles the UART interrupts for the        */
   /* console.                                                          */
#ifdef __USE_TI_RTOS__

void ConsoleIntHandler(UArg arg0)

#else

void ConsoleIntHandler(void)

#endif
{
   unsigned long Status;

   /* Get a pointer to the Uart Context and read the current value of   */
   /* Get the Interrupt Status register Mask to determine what caused   */
   /* the intterupt.                                                    */
   Status = UARTIntStatus(UART4_BASE, 1);
   UARTIntClear(UART4_BASE, Status);

   /* Check to see if a character has been received.                    */
   if(Status & (UART_INT_RX | UART_INT_RT))
   {
      HAL_RxInterrupt();
   }

   /* Check to see if we can send another character.                    */
   if(Status & UART_INT_TX)
   {
      HAL_TxInterrupt();
   }
}

   /* The following function handles updating the Timer Tick.           */
#ifdef __USE_TI_RTOS__

void TimerIntHandler(UArg arg0)

#else

void TimerIntHandler(void)

#endif
{
   /* Clear the interrupt and update the tick count.                    */
   TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

   TickCount++;
}

   /* The following function is used to place the hardware into a known */
   /* state.  This function accepts a single parameter which specifies  */
   /* whether or not the hardware timer (for non-multithreaded Tick     */
   /* Counts - TIMER0) should be enabled.  This parameter should be     */
   /* passed as zero (FALSE) if using an RTOS (e.g. SafeRTOS or         */
   /* FreeRTOS).                                                        */
void HAL_ConfigureHardware(int ConfigureTimerTick)
{
	/* Configure the 32.768 KHz output on PP3 that is needed to drive the*/
	/* Slow Clock signal of the Bluetooth chip.                          */
	MAP_HibernateClockConfig( HIBERNATE_OSC_LOWDRIVE | HIBERNATE_OUT_SYSCLK );		// Wake up was due to a system reset
	MAP_GPIOPinConfigure(GPIO_PP3_RTCCLK);
	MAP_GPIOPinTypeTimer(GPIO_PORTP_BASE, GPIO_PIN_3);

   /* 
    * Review comment - EP
    * Should the pin definition be added as a #def in "GPIO Mapping.h" to be consistent with other GPIO Pin writes?
    */
   // Power P1P8V_BT
   GPIOPinWrite( GPIO_PORTB_BASE, GPIO_PIN_2, GPIO_PIN_2 );
   MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
   /* Initialize the UART Input Buffer structures.                      */
   BTPS_MemInitialize(&InBuf, 0, UART_BUFFER_DATA_SIZE);
   InBuf.BufferSize = UART_IN_BUFFER_LEN;
   InBuf.BytesFree  = UART_IN_BUFFER_LEN;
   InBuf.Buffer     = InBuffer;

   /* Initialize the UART Output Buffer structures.                     */
   BTPS_MemInitialize(&OutBuf, 0, UART_BUFFER_DATA_SIZE);
   OutBuf.BufferSize = UART_OUT_BUFFER_LEN;
   OutBuf.BytesFree  = UART_OUT_BUFFER_LEN;
   OutBuf.Buffer     = OutBuffer;

#ifdef __USE_TI_RTOS__
   IntPrioritySet(INT_UART4, 6 << 5);
#endif

   /* Initialize the Timer.                                             */
   TickCount = 0;

   if(ConfigureTimerTick)
   {
      SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
      TimerDisable(TIMER0_BASE, TIMER_A);
      TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
      TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0);

      /* Configure 1 ms tick.                                           */
      TimerLoadSet(TIMER0_BASE, TIMER_A, (SYSTEM_CLOCK_RATE_HZ / 1000));

      /* Enable timer interrupts for the 1ms timer.                     */
      IntEnable(INT_TIMER0A);

      TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
      TimerEnable(TIMER0_BASE, TIMER_A);
   }

   IntMasterEnable();
}

   /* The following function is used to illuminate an LED.  The number  */
   /* of LEDs on a board is board specific.  If the LED_ID provided does*/
   /* not exist on the hardware platform then nothing is done.          */
void HAL_LedOn(int LED_ID)
{
   if(LED_ID == 0)
      GPIOPinWrite(LED_PORT_0_BASE, LED_PORT_0_PIN, LED_PORT_0_PIN);

   if(LED_ID == 1)
      GPIOPinWrite(LED_PORT_1_BASE, LED_PORT_1_PIN, LED_PORT_1_PIN);
}

   /* The following function is used to extinguish an LED.  The number  */
   /* of LEDs on a board is board specific.  If the LED_ID provided does*/
   /* not exist on the hardware platform then nothing is done.          */
void HAL_LedOff(int LED_ID)
{
   if(LED_ID == 0)
      GPIOPinWrite(LED_PORT_0_BASE, LED_PORT_0_PIN, 0);

   if(LED_ID == 1)
      GPIOPinWrite(LED_PORT_1_BASE, LED_PORT_1_PIN, 0);
}

   /* The following function is used to toggle the state of an LED.  The*/
   /* number of LEDs on a board is board specific.  If the LED_ID       */
   /* provided does not exist on the hardware platform then nothing is  */
   /* done.                                                             */
void HAL_LedToggle(int LED_ID)
{
   if(LED_ID == 0)
      GPIOPinWrite(LED_PORT_0_BASE, LED_PORT_0_PIN, GPIOPinRead(LED_PORT_0_BASE, LED_PORT_0_PIN)?0:LED_PORT_0_PIN);

   if(LED_ID == 1)
      GPIOPinWrite(LED_PORT_1_BASE, LED_PORT_1_PIN, GPIOPinRead(LED_PORT_1_BASE, LED_PORT_1_PIN)?0:LED_PORT_1_PIN);
}

   /* The following function is used to read the stateof a Button.  The */
   /* number of Buttons on a board is board specific.  If the button is */
   /* pressed, the return value will be non-zero.  If the BUTTON_ID     */
   /* provided does not exist on the hardware platform or the button is */
   /* not being depressed then the return value is Zero.                */

// Pin    Function    Type       Init Value   Int         Schematic Name  Connection
/* PQ3    GPIO        IN                      FALLING EDGE SW_SCAN        Scan Button
 */

int HAL_ButtonPressed(int BUTTON_ID)
{
   int ret_val;
   
  /* 
   * Review comment - EP
   * Should the pin definition be added as a #def in "GPIO Mapping.h" to be consistent with other GPIO Pin writes?
   */
   if(!BUTTON_ID)
      ret_val = !(GPIOPinRead(GPIO_PORTQ_BASE, GPIO_PIN_3));
   else
      ret_val = 0;

   return(ret_val);
}

   /* The following function is used to retreive data from the UART     */
   /* input queue.  the function receives a pointer to a buffer that    */
   /* will receive the UART characters a the length of the buffer.  The */
   /* function will return the number of characters that were returned  */
   /* in Buffer.                                                        */
int HAL_ConsoleRead(int Length, char *Buffer)
{
   int ret_val;

   /* Verify that the parameters passed in appear valid.                */
   if((Length) && (Buffer))
   {
      /* Check to see if there are any characters in the input buffer.  */
      ret_val = (InBuf.BufferSize - InBuf.BytesFree);
      if(ret_val)
      {
         /* Adjust the number of avaialble bytes if the Buffer passed in*/
         /* can not hold all of the data that is available.             */
         if(ret_val > Length)
            ret_val = Length;

         /* Check to see to we need to copy the data in two parts.      */
         if(ret_val > (InBuf.BufferSize - InBuf.OutIndex))
         {
            /* Copy as much data as we can on the first pass.           */
            Length = (InBuf.BufferSize - InBuf.OutIndex);
            BTPS_MemCopy(Buffer, &InBuf.Buffer[InBuf.OutIndex], Length);

            /* Copy the remaining data from the beginning of the buffer.*/
            BTPS_MemCopy(&Buffer[Length], InBuf.Buffer, (ret_val - Length));

            /* Adjust the Out Index.                                    */
            InBuf.OutIndex = (ret_val - Length);
         }
         else
         {
            /* Copy the data to the Buffer supplied.                       */
            BTPS_MemCopy(Buffer, &InBuf.Buffer[InBuf.OutIndex], ret_val);

            /* Adjust the Out Index.                                    */
            InBuf.OutIndex += ret_val;
         }

         /* Adjust the number of Free Bytes available in the Input      */
         /* Buffer.                                                     */
         IntDisable(INT_UART4);
         InBuf.BytesFree += ret_val;
         IntEnable(INT_UART4);
      }
      else
         ret_val = 0;
   }
   else
      ret_val = 0;

   return(ret_val);
}

   /* The following function is used to send data to the UART output    */
   /* queue.  the function receives a pointer to a buffer that will     */
   /* contains the data to send and the length of the data.  The        */
   /* function will return the number of characters that were           */
   /* successfully saved in the output buffer.                          */
int HAL_ConsoleWrite(int Length, char *Buffer)
{
   int ret_val;

   /* Verify that the parameters passed in appear valid.                */
   if((Length) && (Buffer) && (OutBuf.BytesFree))
   {
      /* Check to see if we can save all of the data provided.          */
      if(Length > OutBuf.BytesFree)
      {
         /* Small delay to let the buffer drain.                        */
         BTPS_Delay(1);

         /* Re-attempt to output the data.                              */
         if(Length > OutBuf.BytesFree)
            Length = OutBuf.BytesFree;
      }

      /* Set the return values to the number of bytes that we will be   */
      /* copying.                                                       */
      ret_val = Length;

      /* Check to see to we need to copy the data in two parts.         */
      if(Length > (OutBuf.BufferSize - OutBuf.InIndex))
      {
         /* Copy as much data as we can on the first pass.              */
         Length = (OutBuf.BufferSize - OutBuf.InIndex);
         BTPS_MemCopy(&OutBuf.Buffer[OutBuf.InIndex], Buffer, Length);

         /* Copy the remaining data from the beginning of the buffer.   */
         BTPS_MemCopy(OutBuf.Buffer, &Buffer[Length], (ret_val - Length));

         /* Adjust the In Index.                                        */
         OutBuf.InIndex = (ret_val - Length);
      }
      else
      {
         /* Copy the data to the Buffer supplied.                       */
         BTPS_MemCopy(&OutBuf.Buffer[OutBuf.InIndex], Buffer, ret_val);

         /* Adjust the In Index.                                        */
         OutBuf.InIndex += ret_val;
      }

      /* Adjust the number of Free Bytes available in the Output Buffer.*/
      IntDisable(INT_UART4);
      OutBuf.BytesFree -= ret_val;
      IntEnable(INT_UART4);

      /* Check to see if we need to prime the transmitter.              */
      if(!(HWREG(UART4_BASE + UART_O_IM) & UART_IM_TXIM))
      {
         /* Now that the data is in the input buffer, check to see if we*/
         /* need to enable the interrupt to start the TX Transfer.      */
         HWREG(UART4_BASE + UART_O_IM) |= UART_IM_TXIM;

         /* Start sending data to the Uart Transmit FIFO.               */
         IntDisable(INT_UART4);
         HAL_TxInterrupt();
         IntEnable(INT_UART4);
      }
   }
   else
      ret_val = 0;

   return(ret_val);
}

   /* The following function is used to retreive a specific number of   */
   /* bytes from some Non Volatile memory.                              */
int HAL_NV_DataRead(int Length, unsigned char *Buffer)
{
   return(ReadFlash(Length, Buffer));

}
   /* The following function is used to save a specific number of bytes */
   /* to some Non Volatile memory.                                      */
int HAL_NV_DataWrite(int Length, unsigned char *Buffer)
{
   return(WriteFlash(Length, Buffer));
}

   /* The following function is used to return the current Tick Count   */
   /* value.                                                            */
unsigned long HAL_GetTickCount(void)
{
   return(TickCount);
}
