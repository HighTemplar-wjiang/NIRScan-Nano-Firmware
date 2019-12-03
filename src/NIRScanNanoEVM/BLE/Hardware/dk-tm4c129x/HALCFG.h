/*      Copyright 2013 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HALCFG - Hardware Abstraction PIN definitions for Tiva DK-TM4C129X        */
/*           board.                                                           */
/*                                                                            */

/*
* Review comment - SK
* Seggregate the .h and .c files into two folders like it is done under App
* and Drivers folder.
* So, have BLE/App/Include - Have all .h files
* and BLE/App/ - All .c files
*/

#ifndef __HALCFGH__
#define __HALCFGH__

   /* Defines the clock rate of the chip in Hz.                         */
#define SYSTEM_CLOCK_RATE_HZ                             120000000L

   /* This section of the file contains macros that configure the       */
   /* Bluetopia HCI Vendor Specific Bluetooth chipset initialization    */
   /* layer at build time.  It defines the configuration information    */
   /* used to initialize the Bluetooth chipset itself.                  */

   /* Define the baud rate that will be used for communication with the */
   /* Bluetooth chip.  The value of this rate will be configured in the */
   /* Bluetooth transport.                                              */
#define VENDOR_BAUD_RATE                                    921600L

   /* This section of the file contains macros that configure the       */
   /* Bluetopia HCI transport layer at build time.  It defines features */
   /* of the hardware that is used to communicate with the Bluetooth    */
   /* hardware, and can be changed to accommodate a custom board.       */

   /* Define the UART buffers that will be used to buffer               */
   /* incoming/outgoing UART data.  The value are:                      */
   /*                                                                   */
   /*    DEFAULT_INPUT_BUFFER_SIZE  - UART input buffer size (in bytes) */
   /*    DEFAULT_OUTPUT_BUFFER_SIZE - UART output buffer size (in bytes)*/
   /*    DEFAULT_XOFF_LIMIT         - UART H/W Flow Off lower limit     */
   /*    DEFAULT_XON_LIMIT          - UART H/W Flow On (re-enable)      */
   /*                                                                   */
#define DEFAULT_INPUT_BUFFER_SIZE                              1024
#define DEFAULT_OUTPUT_BUFFER_SIZE                              512

#define DEFAULT_XOFF_LIMIT                                      128
#define DEFAULT_XON_LIMIT                                       512

   /* Here is the default allocation of pins to connect to the Bluetooth*/
   /* device, using UART1.                                              */
   /*                                                                   */
 // Pin     Function    Type       Init Value   Int         Schematic Name  Connection
  /* PJ0    UART3 RX                                        BT_RX
   * PJ1    UART3 TX                                        BT_TX
   * PP3    RTC CLK                                         BT_SLOW_CK
   * PP4    UART3 RTS                                       BT_RTS
   * PP5    UART3 CTS                                       BT_CTS
   * PH5    GPIO        OUT         LOW                     BT_nSHUTD       CC2564
   */
   /* Define the UART peripheral that is used for the Bluetooth         */
   /* interface.                                                        */
#define HCI_UART_BASE                                    UART3_BASE
#define HCI_UART_INT                                      INT_UART3
#define HCI_UART_PERIPH                         SYSCTL_PERIPH_UART3

   /* Define the GPIO ports and pins that are used for the UART RX/TX   */
   /* signals.                                                          */
   /* * NOTE * See gpio.h for possible values for HCI_PIN_CONFIGURE_    */
   /*          macros.                                                  */
#define HCI_UART_GPIO_PERIPH                    SYSCTL_PERIPH_GPIOJ
#define HCI_UART_GPIO_BASE                          GPIO_PORTJ_BASE
#define HCI_UART_PIN_RX                                  GPIO_PIN_0
#define HCI_UART_PIN_TX                                  GPIO_PIN_1
#define HCI_PIN_CONFIGURE_UART_RX                     GPIO_PJ0_U3RX
#define HCI_PIN_CONFIGURE_UART_TX                     GPIO_PJ1_U3TX

   /* Define the GPIO ports and pins that are used for the UART RTS     */
   /* signal.                                                           */
#define HCI_UART_RTS_GPIO_PERIPH                SYSCTL_PERIPH_GPIOP
#define HCI_UART_RTS_GPIO_BASE                      GPIO_PORTP_BASE
#define HCI_UART_PIN_RTS                                 GPIO_PIN_4
#define HCI_PIN_CONFIGURE_UART_RTS                   GPIO_PP4_U3RTS

   /* Define the GPIO ports and pins that are used for the UART CTS     */
   /* signal.                                                           */
#define HCI_UART_CTS_GPIO_PERIPH                SYSCTL_PERIPH_GPIOP
#define HCI_UART_CTS_GPIO_BASE                      GPIO_PORTP_BASE
#define HCI_UART_PIN_CTS                                 GPIO_PIN_5
#define HCI_PIN_CONFIGURE_UART_CTS                   GPIO_PP5_U3CTS

   /* Define the GPIO ports and pins that are used for the Bluetooth    */
   /* RESET signal.                                                     */
#define HCI_RESET_PERIPH                        SYSCTL_PERIPH_GPIOH
#define HCI_RESET_BASE                              GPIO_PORTH_BASE
#define HCI_RESET_PIN                                    GPIO_PIN_5

#endif

