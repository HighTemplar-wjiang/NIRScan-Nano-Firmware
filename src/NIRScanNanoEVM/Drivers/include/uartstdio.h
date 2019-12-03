//*****************************************************************************
//
//
// Copyright (c) 2007-2015 Texas Instruments Incorporated.  All rights reserved.
// 
//*****************************************************************************

#ifndef __UARTSTDIO_H__
#define __UARTSTDIO_H__

#include <stdarg.h>

/*
 * If built for buffered operation, the following labels define the sizes of
 * the transmit and receive buffers respectively.
 * Also, if command interface is used, HID size restrictions will apply
 */

#ifdef UART_BUFFERED
#ifndef UART_RX_BUFFER_SIZE
		#define UART_RX_BUFFER_SIZE     MAX(128,UART_MAX_CMD_MAX_PKT_SZ)
#endif

#ifndef UART_TX_BUFFER_SIZE
		#define UART_TX_BUFFER_SIZE     MAX(1024,UART_MAX_CMD_MAX_PKT_SZ)
#endif
#endif

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
"C"
{
#endif

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
void uart_interface_init(void);
void UARTStdioConfig(uint32_t ui32Port, uint32_t ui32Baud,
                            uint32_t ui32SrcClock);
int UARTgets(char *pcBuf, uint32_t ui32Len);
unsigned char UARTgetc(void);
void UARTprintf(const char *pcString, ...);
void UARTvprintf(const char *pcString, va_list vaArgP);
int UARTwrite(const char *pcBuf, uint32_t ui32Len);
#ifdef UART_BUFFERED
int UARTGetCurrRxBufferSz();
int UARTReadRxBuffer(char *str, int len);

int UARTPeek(unsigned char ucChar);
void UARTFlushTx(bool bDiscard);
void UARTFlushLog(bool bDiscard);
void UARTFlushRx(void);
int UARTRxBytesAvail(void);
int UARTTxBytesFree(void);
void UARTEchoSet(bool bEnable);

void uartDataTimeout();
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __UARTSTDIO_H__
