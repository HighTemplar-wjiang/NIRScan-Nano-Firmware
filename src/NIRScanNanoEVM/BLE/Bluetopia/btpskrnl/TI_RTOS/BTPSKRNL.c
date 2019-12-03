/*****< btpskrnl.c >***********************************************************/
/*      Copyright 2000 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSKRNL - Stonestreet One Bluetooth Stack Kernel Implementation.         */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   05/30/01  D. Lange       Initial creation.                               */
/*   12/18/01  R. Sledge      Port to Linux                                   */
/*   02/05/08  D. Wooldridge  Incorporate functions security module requires  */
/******************************************************************************/
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include "BTPSKRNL.h"       /* BTPS Kernel Prototypes/Constants.        */
#include "BTTypes.h"        /* BTPS internal data types.                */
#include "HAL.h"

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/gates/GateAll.h>

   /* The following constant represents the number of bytes that are    */
   /* displayed on an individual line when using the DumpData()         */
   /* function.                                                         */
#define MAXIMUM_BYTES_PER_ROW     (16)

   /* The following MACRO maps Timeouts (specified in Milliseconds) to  */
   /* System Ticks that are required by the Operating System Timeout    */
   /* functions (Waiting only).                                         */
#define MILLISECONDS_TO_TICKS(_x) ((unsigned int)((_x)/(((float)Clock_tickPeriod) / 1000)))

   /* Denotes the priority of the thread being created using the thread */
   /* create function.                                                  */
#define DEFAULT_THREAD_PRIORITY   (1)

   /* Invalid thread handle value.                                      */
#define BTPS_INVALID_HANDLE_VALUE ((unsigned long *)-1)

   /* The bit used in the Event count for indicating when the event is  */
   /* set.                                                              */
#define EVENT_SET_FLAG            (0x80)

#define SEGMENT_ALLOCATED_BITMASK (0x8000)
#define SEGMENT_SIZE_BITMASK      (0x7FFF)

   /* The following type declaration represents the entire state        */
   /* information for a Mutex_t.  This structure is used with all of the*/
   /* Mutex functions contained in this module.                         */
typedef struct _tagMutexHeader_t
{
   Task_Handle      MutexOwner;
   Semaphore_Handle Mutex;
   unsigned int     Count;
} MutexHeader_t;

   /* The following type declaration represents the entire state        */
   /* information for an Event_t.  This structure is used with all of   */
   /* the Event functions contained in this module.                     */
typedef struct _tagEventHeader_t
{
   Semaphore_Handle EventMutex;
   GateAll_Handle   EventGate;
   unsigned int     Count;
   Boolean_t        IsSet;
} EventHeader_t;

   /* The following type declaration represents the entire state        */
   /* information for a Mailbox.  This structure is used with all of    */
   /* the Mailbox functions contained in this module.                   */
typedef struct _tagMailboxHeader_t
{
   Event_t  Event;
   Mutex_t  Mutex;
   Word_t   HeadSlot;
   Word_t   TailSlot;
   Word_t   OccupiedSlots;
   Word_t   NumberSlots;
   Word_t   SlotSize;
   void    *Slots;
} MailboxHeader_t;

   /* The following structure is a container structure that exists to   */
   /* map the OS Thread Function to the BTPS Kernel Thread Function (it */
   /* doesn't totally map in a one to one relationship/mapping).        */
typedef struct _tagThreadWrapperInfo_t
{
   char           Name[4];
   Thread_t       ThreadFunction;
   void          *ThreadParameter;
} ThreadWrapperInfo_t;

   /* The following enumerates the states of a heap fragment.           */
typedef enum
{
   fsFree,
   fsInUse
} FragmentState_t;

   /* The following defines a type that is the size in bytes of the     */
   /* desired alignment of each datr fragment.                          */
typedef unsigned long Alignment_t;

   /* The following defines the byte boundary size that has been        */
   /* specified size if the alignment data.                             */
#define ALIGNMENT_SIZE          sizeof(Alignment_t)

   /* The following structure is used to allign data fragments on a     */
   /* specified memory boundary.                                        */
typedef union _tagAlignmentStruct_t
{
   Alignment_t   AlignmentValue;
   unsigned char ByteValue;
} AlignmentStruct_t;

   /* The following defines the size in bytes of a data fragment that is*/
   /* considered a large value.  Allocations that are equal to and      */
   /* larger than this value will be allocated from the end of the      */
   /* buffer.                                                           */
#define LARGE_SIZE              (256/ALIGNMENT_SIZE)

   /* The following defines the minimum size (in alignment units) of a  */
   /* fragment that is considered useful.  The value is used when trying*/
   /* to determine if a fragment that is larger than the requested size */
   /* can be broken into 2 framents leaving a fragment that is of the   */
   /* requested size and one that is at least as larger as the          */
   /* MINIMUM_MEMORY_SIZE.                                              */
#define MINIMUM_MEMORY_SIZE     1

   /* The following defines the structure that describes a memory       */
   /* fragment.                                                         */
typedef struct _tagHeapInfo_t
{
   Word_t            PrevSize;
   Word_t            Size;
   AlignmentStruct_t Data;
} HeapInfo_t;

#define HEAP_INFO_DATA_SIZE(_x) ((BTPS_STRUCTURE_OFFSET(HeapInfo_t, Data) / ALIGNMENT_SIZE) + (_x))

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

   /* Declare a buffer to use for the Heap.  Note that we declare this  */
   /* as an unsigned long buffer so that we can force alignment to be   */
   /* correct.                                                          */
static Alignment_t MemoryBuffer[(BTPS_MEMORY_BUFFER_SIZE/ALIGNMENT_SIZE) + 1];

   /* The following variable points to the heap head after              */
   /* initialization.                                                   */
static HeapInfo_t *HeapHead = NULL;

   /* The following variable points to the heap tail after              */
   /* initialization.                                                   */
static HeapInfo_t *HeapTail = NULL;

   /* The following variable is used to guard against multiple threads  */
   /* attempting to use global variables declared in this module.       */
static Mutex_t KernelMutex;

   /* The following is used to guard access to MsgBuf and the function  */
   /* that sends Debug Messages to the Debug UART.                      */
static Mutex_t IOMutex;

   /* The following buffer is used when writing Debug Messages to Debug */
   /* UART.                                                             */
static char MsgBuf[256];

   /* The following is used to increment the thread name for each       */
   /* created thread.                                                   */
static char Ndx;

static Task_Handle TaskDeleteList[6];

   /* The following is used to track which debug message types are      */
   /* printed via DBG_MSG and DBG_DUMP (when DEBUG_ENABLED is defined). */
static unsigned long  DebugZoneMask = (unsigned long)DEBUG_ZONES;

   /* The following holds the currently registered function that is to  */
   /* be called when there is a character of an output message to be    */
   /* displayed.  This value is set via a call to the BTPS_Init()       */
   /* function.                                                         */
static BTPS_MessageOutputCallback_t MessageOutputCallback;

   /* Used to indicate that initialization has occurred                 */
static Boolean_t Initialized = FALSE;

   /* Internal Function Prototypes.                                     */
static Byte_t ConsoleWrite(char *Message, int Length);
static void CalcTotals(unsigned int *Used, unsigned int *Free, unsigned int *MaxFree);
static void ThreadWrapper(UArg,UArg);
static void HeapInit(void);
static void *_Malloc(unsigned long Size);
static void _MemFree(void *MemoryPtr);

   /* The following function is used to send a string of characters to  */
   /* the Console or Output device.  The function takes as its first    */
   /* parameter a pointer to a string of characters to be output.  The  */
   /* second parameter indicates the number of bytes in the character   */
   /* string that is to be output.                                      */
static Byte_t ConsoleWrite(char *Message, int Length)
{
   char ch = '\0';

   /* Check to make sure that the Debug String appears to be semi-valid.*/
   if((Message) && (Length) && (MessageOutputCallback) && (IOMutex))
   {
      if(BTPS_WaitMutex(IOMutex, BTPS_INFINITE_WAIT))
      {
         while(*Message)
         {
            ch = *Message++;
            MessageOutputCallback(ch);
         }

         BTPS_ReleaseMutex(IOMutex);
      }
   }

   return(0);
}

   /* The following is a utility function that can calculate the current*/
   /* memory usage.  The function takes as its first parameter a pointer*/
   /* to receive the number of bytes currently allocated and in use.    */
   /* The second parameter is a pointer to a value that will receive the*/
   /* number of unallocated byte that are free for use.  The third      */
   /* parameter is a pointer to receive the largest fragment that is    */
   /* free to be used.                                                  */
static void CalcTotals(unsigned int *Used, unsigned int *Free, unsigned int *MaxFree)
{
   HeapInfo_t *HeapInfoPtr;
   Word_t      FreeSegments;
   Word_t      AllocSegments;

   /* Verify that the heap has been initialized.                        */
   if(!HeapHead)
      HeapInit();

   /* Verify that the parameters that were passed in appear valid.      */
   if((Used) && (Free) && (MaxFree) && (HeapHead))
   {
      /* Initialize the return values.                                  */
      *Used         = 0;
      *Free         = 0;
      *MaxFree      = 0;
      FreeSegments  = 0;
      AllocSegments = 0;
      HeapInfoPtr   = HeapHead;
      do
      {
         /* Check to see if the current fragment is marked as Free.     */
         if(HeapInfoPtr->Size & SEGMENT_ALLOCATED_BITMASK)
         {
            AllocSegments++;

            *Used += (HeapInfoPtr->Size & SEGMENT_SIZE_BITMASK) * ALIGNMENT_SIZE;
         }
         else
         {
            FreeSegments++;

            /* Accumulate the total number of Free Bytes.               */
            *Free += HeapInfoPtr->Size * ALIGNMENT_SIZE;

            /* Check to see if the current fragment is larger that any  */
            /* we have seen and update the Max Value if it is larger.   */
            if(HeapInfoPtr->Size > *MaxFree)
               *MaxFree = HeapInfoPtr->Size * ALIGNMENT_SIZE;
         }
         /* Adjust the pointer to the next entry.                       */
         HeapInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) + (HeapInfoPtr->Size & SEGMENT_SIZE_BITMASK));

      } while(HeapInfoPtr != HeapTail);
   }
}

   /* The following function is used to initialize the heap structure.  */
   /* The function takes no parameters and returns no status.           */
static void HeapInit(void)
{
   DWord_t HeapSize;

   /* Verify that the heap info structure is properly aligned.          */
   if((BTPS_STRUCTURE_OFFSET(HeapInfo_t, Data) % ALIGNMENT_SIZE) == 0)
   {
      /* Calculate the size of the heap in alignment units.             */
      HeapSize = sizeof(MemoryBuffer) / ALIGNMENT_SIZE;

      /* Verify that the heap is not bigger than the maximum segment    */
      /* length.                                                        */
      if(HeapSize <= SEGMENT_SIZE_BITMASK)
      {
         /* Initialize the heap.                                        */
         HeapHead           = (HeapInfo_t *)MemoryBuffer;
         HeapHead->PrevSize = HeapSize;
         HeapHead->Size     = HeapSize;

         HeapTail           = (HeapInfo_t *)(MemoryBuffer + HeapSize);
      }
   }
}

   /* The following function is used to allocate a fragment of memory   */
   /* from a large buffer.  The function takes as its parameter the size*/
   /* in bytes of the fragment to be allocated.  The function tries to  */
   /* avoid fragmentation by obtaining memory requests larger than      */
   /* LARGE_SIZE from the end of the buffer, while small fragments are  */
   /* taken from the start of the buffer.                               */
static void *_Malloc(unsigned long Size)
{
   HeapInfo_t *HeapInfoPtr;
   HeapInfo_t *TmpInfoPtr;
   Word_t      TmpSize;
   void       *ret_val;


   /* Convert the requested memory allocation in bytes to alignment     */
   /* size.                                                             */
   if(Size % ALIGNMENT_SIZE)
      Size = (Size / ALIGNMENT_SIZE) + 1;
   else
      Size /= ALIGNMENT_SIZE;

   /* Add the header size to the requested size.                        */
   Size += HEAP_INFO_DATA_SIZE(0);

   /* Verify that the requested size is valid                           */
   if((Size >= HEAP_INFO_DATA_SIZE(1)) && !(Size & SEGMENT_ALLOCATED_BITMASK))
   {
      if(BTPS_WaitMutex(KernelMutex, BTPS_INFINITE_WAIT))
      {
         /* Verify that the heap has been initialized.                  */
         if(!HeapHead)
            HeapInit();

         /* Verify that the heap is valid.                              */
         if(HeapHead)
         {
            /* If we are allocating a large segment, then start at the  */
            /* end of the heap.  Otherwise, start at the beginning of   */
            /* the heap.  If there is only one segment in the heap, then*/
            /* HeapHead will be used either way.                        */
            if(Size >= LARGE_SIZE)
            {
               HeapInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapTail) - HeapHead->PrevSize);
            }
            else
               HeapInfoPtr = HeapHead;


            /* Loop until we have walked the entire list.               */
            while(((Size < LARGE_SIZE) || (HeapInfoPtr != HeapHead)) && (HeapInfoPtr != HeapTail))
            {
               /* Check to see if the current entry is free and is large*/
               /* enough to hold the data being requested.              */
               if((HeapInfoPtr->Size & SEGMENT_ALLOCATED_BITMASK) || (HeapInfoPtr->Size < Size))
               {
                  /* If the requested size is larger than the limit then*/
                  /* search backwards for an available buffer, else go  */
                  /* forward.  This will hopefully help to reduce       */
                  /* fragmentataion problems.                           */
                  if(Size >= LARGE_SIZE)
                     HeapInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) - (HeapInfoPtr->PrevSize));
                  else
                     HeapInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) + (HeapInfoPtr->Size & SEGMENT_SIZE_BITMASK));
               }
               else
                  break;
            }

            /* Check to see if we found a segment large enough for the  */
            /* request.                                                 */
            if((HeapInfoPtr != HeapTail) && !(HeapInfoPtr->Size & SEGMENT_ALLOCATED_BITMASK) && (HeapInfoPtr->Size >= Size))
            {
               /* Check to see if we need to split this into two        */
               /* entries.                                              */
               /* * NOTE * If there is not enough room to make another  */
               /*          entry then we will not adjust the size of    */
               /*          this entry to match the amount requested.    */
               if(HeapInfoPtr->Size >= (Size + HEAP_INFO_DATA_SIZE(MINIMUM_MEMORY_SIZE)))
               {
                  /* Calculate the size for the free segment.           */
                  TmpSize = HeapInfoPtr->Size - Size;

                  /* If this is a large segment allocation, then split  */
                  /* the segment so that the free segment is at the     */
                  /* beginning.                                         */
                  if(Size >= LARGE_SIZE)
                  {
                     /* Calculate the pointer to the allocated segment. */
                     TmpInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) + TmpSize);

                     /* Set the previous size and size values for the   */
                     /* allocated segment.                              */
                     TmpInfoPtr->PrevSize = TmpSize;
                     TmpInfoPtr->Size     = Size | SEGMENT_ALLOCATED_BITMASK;

                     /* Set the new size of the free segment.           */
                     HeapInfoPtr->Size = TmpSize;

                     /* Set the pointer to the beginning of the newly   */
                     /* allocated segment                               */
                     HeapInfoPtr = TmpInfoPtr;

                     /* Save the size of the allocated segment so that  */
                     /* the next segment can be updated.                */
                     TmpSize = Size;
                  }
                  else
                  {
                     /* Calculate the pointer to the free segment.      */
                     TmpInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) + Size);

                     /* Set the previous size and size values for the   */
                     /* free segment.                                   */
                     TmpInfoPtr->PrevSize = Size;
                     TmpInfoPtr->Size     = TmpSize;

                     /* Set the new size of the allocated segment and   */
                     /* indicate that it is no longer free.             */
                     HeapInfoPtr->Size = Size | SEGMENT_ALLOCATED_BITMASK;
                  }

                  /* Calculate the pointer to the next segment.         */
                  TmpInfoPtr = (HeapInfo_t *)(((Alignment_t *)TmpInfoPtr) + TmpSize);

                  /* Check for a wrap condition and update the next     */
                  /* segment's PrevSize field.                          */
                  if(TmpInfoPtr == HeapTail)
                  {
                     HeapHead->PrevSize = TmpSize;
                  }
                  else
                  {
                     TmpInfoPtr->PrevSize = TmpSize;
                  }
               }
               else
               {
                  /* Mark the buffer as In Use.                         */
                  HeapInfoPtr->Size |= SEGMENT_ALLOCATED_BITMASK;
               }

               /* Get the address of the start of RAM.                  */
               ret_val = (void *)&HeapInfoPtr->Data;
            }
            else
            {
               /* There is not a segment large enough to satisfy this   */
               /* request.                                              */
               ret_val = NULL;
            }
         }
         else
         {
            /* The heap could not be initialized.                       */
            ret_val = NULL;
         }

         /* Release the Kernel Mutex that was acquired earlier.         */
         BTPS_ReleaseMutex(KernelMutex);
      }
      else
      {
         /* The mutex could not be acquired.                            */
         ret_val = NULL;
      }
   }
   else
   {
      /* The requested size is invalid.                                 */
      ret_val = NULL;
   }

   return(ret_val);
}

   /* The following function is used to free memory that was previously */
   /* allocated with _Malloc.  The function takes as its parameter a    */
   /* pointer to the memory that was allocated.  The pointer is used to */
   /* locate the structure of information that describes the allocated  */
   /* fragment.  The function tries to a verify that the structure is a */
   /* valid fragment structure before the memory is freed.  When a      */
   /* fragment is freed, it may be combined with adjacent fragments to  */
   /* produce a larger free fragment.                                   */
static void _MemFree(void *MemoryPtr)
{
   HeapInfo_t *HeapInfoPtr;
   HeapInfo_t *TmpInfoPtr;

   /* Verify that the parameter passed in appears valid.                */
   if((MemoryPtr) && (HeapHead) && (MemoryPtr >= ((void *)HeapHead)) && (MemoryPtr < ((void *)HeapTail)))
   {
      if(BTPS_WaitMutex(KernelMutex, BTPS_INFINITE_WAIT))
      {
         /* Get a pointer to the Heap Info.                             */
         HeapInfoPtr = (HeapInfo_t *)(((Alignment_t *)MemoryPtr) - HEAP_INFO_DATA_SIZE(0));

         /* Verify that this segment is allocated.                      */
         if(HeapInfoPtr->Size & SEGMENT_ALLOCATED_BITMASK)
         {
            /* Mask out the allocation bit of the segment to be freed.  */
            /* This will make calculations in this block easier.        */
            HeapInfoPtr->Size &= SEGMENT_SIZE_BITMASK;

            /* If the segment to be freed is at the head of the heap,   */
            /* then we do not have to merge or update any sizes of the  */
            /* previous segment.  This will also handle the case where  */
            /* the entire heap has been allocated to one segment.       */
            if(HeapInfoPtr != HeapHead)
            {
               /* Calculate the pointer to the previous segment.        */
               TmpInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) - HeapInfoPtr->PrevSize);

               /* Check to see if the previous segment can be combined. */
               if(!(TmpInfoPtr->Size & SEGMENT_ALLOCATED_BITMASK))
               {
                  /* Add the segment to be freed to the new header.     */
                  TmpInfoPtr->Size += HeapInfoPtr->Size;

                  /* Set the pointer to the beginning of the previous   */
                  /* free segment.                                      */
                  HeapInfoPtr = TmpInfoPtr;
               }
            }

            /* Calculate the pointer to the next segment.               */
            TmpInfoPtr = (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) + HeapInfoPtr->Size);

            /* If we are pointing at the end of the heap, then use the  */
            /* head as the next segment.                                */
            if(TmpInfoPtr == HeapTail)
            {
               /* We can't combine the head with the tail, so just      */
               /* update the PrevSize field.                            */
               HeapHead->PrevSize = HeapInfoPtr->Size;
            }
            else
            {
               /* We are not pointing to the end of the heap, so if the */
               /* next segment is allocated then update the PrevSize    */
               /* field.                                                */
               if(TmpInfoPtr->Size & SEGMENT_ALLOCATED_BITMASK)
               {
                  TmpInfoPtr->PrevSize = HeapInfoPtr->Size;
               }
               else
               {
                  /* The next segment is free, so merge it with the     */
                  /* current segment.                                   */
                  HeapInfoPtr->Size += TmpInfoPtr->Size;

                  /* Since we merged the next segment, we have to update*/
                  /* the next next segment's PrevSize field.            */
                  TmpInfoPtr =  (HeapInfo_t *)(((Alignment_t *)HeapInfoPtr) + HeapInfoPtr->Size);

                  /* If we are pointing at the end of the heap, then use*/
                  /* the head as the next next segment.                 */
                  if(TmpInfoPtr == HeapTail)
                  {
                     HeapHead->PrevSize = HeapInfoPtr->Size;
                  }
                  else
                  {
                     TmpInfoPtr->PrevSize = HeapInfoPtr->Size;
                  }
               }
            }
         }

         /* Release the Kernel Mutex that was acquired earlier          */
         BTPS_ReleaseMutex(KernelMutex);
      }
   }
}

   /* The following function is a function that represents the native   */
   /* thread type for the operating system.  This function does nothing */
   /* more than to call the BTPSKRNL Thread function of the specified   */
   /* format (parameters/calling convention/etc.).  The UserData        */
   /* parameter that is passed to this function is a pointer to a       */
   /* ThreadWrapperInfo_t structure which will contain the actual       */
   /* BTPSKRNL Thread Information.  This function will free this pointer*/
   /* using the BTPS_FreeMemory() function (which means that the Thread */
   /* Information structure needs to be allocated with the              */
   /* BTPS_AllocateMemory() function.                                   */
static void ThreadWrapper(UArg arg0, UArg arg1)
{
   Byte_t i;

   /* Verify that the parameter passed in appears valid.                */
   if(arg0)
   {
      ((*(((ThreadWrapperInfo_t *)arg0)->ThreadFunction))(((ThreadWrapperInfo_t *)arg0)->ThreadParameter));

      /* If we exit a thread free the ThreadWrapperInfo structure.      */
      BTPS_FreeMemory((void *)arg0);
   }

   for(i = (sizeof(TaskDeleteList)/sizeof(Task_Handle)); i--;)
   {
      BTPS_OutputMessage("TW %d\r\n", i);
      if(!(TaskDeleteList[i]))
      {
         TaskDeleteList[i] = Task_selfMacro();
         break;
      }
   }
   BTPS_OutputMessage("TW E %d\r\n", i);
}

   /* The following function is responsible for the Memory Usage        */
   /* Information.  This function accepts as input the Memory Pool Usage*/
   /* Length and a pointer to an Buffer of Memory Pool Usage structures.*/
   /* The third parameter to this function is a pointer to a unsigned   */
   /* int which will passed the Total Buffer Size.  The fourth parameter*/
   /* to this function is a pointer to a unsigned int which will be     */
   /* passed the Memory Buffer Usage.                                   */
int BTPSAPI BTPS_QueryMemoryUsage(unsigned int *Used, unsigned int *Free, unsigned int *MaxFree)
{
   CalcTotals(Used, Free, MaxFree);

   return(0);
}

   /* The following function is responsible for delaying the current    */
   /* task for the specified duration (specified in Milliseconds).      */
   /* * NOTE * Very small timeouts might be smaller in granularity than */
   /*          the system can support !!!!                              */
void BTPSAPI BTPS_Delay(unsigned long MilliSeconds)
{
   UInt ticks;

   /* Simply wrap the OS supplied Task_sleep function. This should not  */
   /* be called from any interrupts.                                    */
   if(MilliSeconds == BTPS_INFINITE_WAIT)
   {
      while (TRUE)
      {
         /* Cannot pass BIOS_WAIT_FOREVER into Task_sleep, so pass next */
         /* biggest value.                                              */
         Task_sleep(((UInt)~(0)) - 1);
      }
   }
   else
   {
      /* Task_sleep takes system ticks, so convert                      */
      ticks = (UInt)MILLISECONDS_TO_TICKS(MilliSeconds);

      /* Make sure that the wait is not forever                         */
      if(ticks == BIOS_WAIT_FOREVER)
         ticks = ((UInt)BIOS_WAIT_FOREVER) - 1;

      Task_sleep(ticks);
   }
}

   /* The following function is responsible for creating an actual      */
   /* Mutex (Binary Semaphore).  The Mutex is unique in that if a       */
   /* Thread already owns the Mutex, and it requests the Mutex again    */
   /* it will be granted the Mutex.  This is in Stark contrast to a     */
   /* Semaphore that will block waiting for the second acquisition of   */
   /* the Sempahore.  This function accepts as input whether or not     */
   /* the Mutex is initially Signalled or not.  If this input parameter */
   /* is TRUE then the caller owns the Mutex and any other threads      */
   /* waiting on the Mutex will block.  This function returns a NON-NULL*/
   /* Mutex Handle if the Mutex was successfully created, or a NULL     */
   /* Mutex Handle if the Mutex was NOT created.  If a Mutex is         */
   /* successfully created, it can only be destroyed by calling the     */
   /* BTPS_CloseMutex() function (and passing the returned Mutex        */
   /* Handle).                                                          */
Mutex_t BTPSAPI BTPS_CreateMutex(Boolean_t CreateOwned)
{
   Semaphore_Params  params;
   Mutex_t           ret_val = NULL;
   MutexHeader_t    *mutexHeader;


   /* Before procedding allocate memory for the event header and verify */
   /* that the allocate was successful.                                 */
   if((mutexHeader = (MutexHeader_t *)BTPS_AllocateMemory(sizeof(MutexHeader_t))) != NULL)
   {
      /* Configure semaphore as binary                                  */
      Semaphore_Params_init(&params);
      params.mode = Semaphore_Mode_BINARY;

      /* The memory for the semaphore was successfully allocated, now   */
      /* attempt to create the semaphore.                               */
      mutexHeader->Mutex = Semaphore_create(CreateOwned?0:1, &params, NULL);
      if(mutexHeader->Mutex)
      {
         /* The mutex is being created as owned so initialize the count */
         /* and Thread Handle values within the mutex header structure  */
         /* appropriately.                                              */
         mutexHeader->MutexOwner = (CreateOwned ? Task_selfMacro() : NULL);
         mutexHeader->Count = (CreateOwned ? 1 : 0);
         ret_val = (Mutex_t) mutexHeader;
      }
      else
      {
         /* An error has occurred, free the previously allocated memory */
         BTPS_FreeMemory(mutexHeader);
      }
   }

   return(ret_val);
}

   /* The following function is responsible for waiting for the         */
   /* specified Mutex to become free.  This function accepts as input   */
   /* the Mutex Handle to wait for, and the Timeout (specified in       */
   /* Milliseconds) to wait for the Mutex to become available.  This    */
   /* function returns TRUE if the Mutex was successfully acquired and  */
   /* FALSE if either there was an error OR the Mutex was not           */
   /* acquired in the specified Timeout.  It should be noted that       */
   /* Mutexes have the special property that if the calling Thread      */
   /* already owns the Mutex and it requests access to the Mutex again  */
   /* (by calling this function and specifying the same Mutex Handle)   */
   /* then it will automatically be granted the Mutex.  Once a Mutex    */
   /* has been granted successfully (this function returns TRUE), then  */
   /* the caller MUST call the BTPS_ReleaseMutex() function.            */
   /* * NOTE * There must exist a corresponding BTPS_ReleaseMutex()     */
   /*          function call for EVERY successful BTPS_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
Boolean_t BTPSAPI BTPS_WaitMutex(Mutex_t Mutex, unsigned long Timeout)
{
   MutexHeader_t *mutexHeader = (MutexHeader_t *)Mutex;
   Boolean_t      ret_val     = FALSE;
   UInt           uTimeout;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(mutexHeader)
   {
      /* The parameters apprear to be semi-valid check if the thread    */
      /* asking for the Mutex is the current thread that owns it.       */
      if(mutexHeader->MutexOwner != Task_selfMacro())
      {
         /* Convert the timeout into the appropriate amount of ticks    */
         if(Timeout == BTPS_INFINITE_WAIT)
            uTimeout = BIOS_WAIT_FOREVER;
         else
         {
            if(Timeout == BTPS_NO_WAIT)
               uTimeout = BIOS_NO_WAIT;
            else
            {
               /* Semaphore_pend takes system ticks, so convert         */
               uTimeout = (UInt)MILLISECONDS_TO_TICKS(Timeout);

               /* Make sure that the wait is not forever                */
               if(uTimeout == BIOS_WAIT_FOREVER)
                  uTimeout = ((UInt)BIOS_WAIT_FOREVER) - 1;
            }
         }

         /* Attempt to get the semaphore for the timeout duration.      */
         if(Semaphore_pend(mutexHeader->Mutex, uTimeout))
         {
            /* Mutex acquired successfully.                             */
            mutexHeader->Count++;
            mutexHeader->MutexOwner = Task_selfMacro();
            ret_val                 = TRUE;
         }
      }
      else
      {
         /* This thread already has acquired the mutex                  */
         mutexHeader->Count++;
         ret_val = TRUE;
      }
   }

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is responsible for releasing a Mutex that  */
   /* was successfully acquired with the BTPS_WaitMutex() function.     */
   /* This function accepts as input the Mutex that is currently        */
   /* owned.                                                            */
   /* * NOTE * There must exist a corresponding BTPS_ReleaseMutex()     */
   /*          function call for EVERY successful BTPS_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
void BTPSAPI BTPS_ReleaseMutex(Mutex_t Mutex)
{
   MutexHeader_t *mutexHeader = (MutexHeader_t *)Mutex;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(mutexHeader)
   {
      /* The parameter apprears to be semi-valid now check to make sure */
      /* the Mutex is currently owned.                                  */
      if(((MutexHeader_t *)Mutex)->Count)
      {
         /* The Mutex is currently owned, so check to see if the mutex  */
         /* is currently aquired elsewhere by the same task.  If it is  */
         /* no longer being used by the current task, then signal the   */
         /* Mutex.                                                      */
         if(!(--mutexHeader->Count))
         {
            mutexHeader->MutexOwner = NULL;
            Semaphore_post(mutexHeader->Mutex);
         }
      }
   }
}

   /* The following function is responsible for destroying a Mutex that */
   /* was created successfully via a successful call to the             */
   /* BTPS_CreateMutex() function.  This function accepts as input the  */
   /* Mutex Handle of the Mutex to destroy.  Once this function is      */
   /* completed the Mutex Handle is NO longer valid and CANNOT be       */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMutex() functions to fail with an error.                 */
void BTPSAPI BTPS_CloseMutex(Mutex_t Mutex)
{
   MutexHeader_t *mutexHeader = (MutexHeader_t *)Mutex;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(mutexHeader)
   {
      /* The passed parameter appears to be semi-valid.  Now attempt to */
      /* terminate and de-allocate the memory associated with the mutex.*/
      Semaphore_delete(&(mutexHeader->Mutex));
      BTPS_FreeMemory(mutexHeader);
   }
}

   /* The following function is responsible for creating an actual      */
   /* Event.  The Event is unique in that it only has two states.  These*/
   /* states are Signalled and Non-Signalled.  Functions are provided   */
   /* to allow the setting of the current state and to allow the        */
   /* option of waiting for an Event to become Signalled.  This function*/
   /* accepts as input whether or not the Event is initially Signalled  */
   /* or not.  If this input parameter is TRUE then the state of the    */
   /* Event is Signalled and any BTPS_WaitEvent() function calls will   */
   /* immediately return.  This function returns a NON-NULL Event       */
   /* Handle if the Event was successfully created, or a NULL Event     */
   /* Handle if the Event was NOT created.  If an Event is successfully */
   /* created, it can only be destroyed by calling the BTPS_CloseEvent()*/
   /* function (and passing the returned Event Handle).                 */
Event_t BTPSAPI BTPS_CreateEvent(Boolean_t CreateSignalled)
{
   EventHeader_t *eventHeader = NULL;
   Event_t        ret_val     = NULL;

   /* Before proceding allocate memory for the event header and verify  */
   /* that the allocate was successful.                                 */
   if((eventHeader = (EventHeader_t *)BTPS_AllocateMemory(sizeof(EventHeader_t))) != NULL)
   {
      /* The passed parameter appears to be semi-valid.  Now attempt to */
      /* create a COUNTING semaphore                                    */
      eventHeader->EventMutex = Semaphore_create(CreateSignalled?1:0, NULL, NULL);
      if(eventHeader->EventMutex)
      {
         /* This gate will cause all preemption to be disabled while    */
         /* enabled.  This provides a locking mechanism for Hwi's,      */
         /* Swi's, and (supposedly) tasks.                              */
         eventHeader->EventGate = GateAll_create(NULL, NULL);
         if(eventHeader->EventGate)
         {
            eventHeader->IsSet = CreateSignalled;
            eventHeader->Count = (CreateSignalled)?0:1;
            ret_val = (Event_t)eventHeader;
         }
         else
         {
            Semaphore_delete(&(eventHeader->EventMutex));
            BTPS_FreeMemory(eventHeader);
         }
      }
      else
         BTPS_FreeMemory(eventHeader);
   }

   return(ret_val);
}

   /* The following function is responsible for waiting for the         */
   /* specified Event to become Signalled.  This function accepts as    */
   /* input the Event Handle to wait for, and the Timeout (specified    */
   /* in Milliseconds) to wait for the Event to become Signalled.  This */
   /* function returns TRUE if the Event was set to the Signalled       */
   /* State (in the Timeout specified) or FALSE if either there was an  */
   /* error OR the Event was not set to the Signalled State in the      */
   /* specified Timeout.  It should be noted that Signalls have a       */
   /* special property in that multiple Threads can be waiting for the  */
   /* Event to become Signalled and ALL calls to BTPS_WaitEvent() will  */
   /* return TRUE whenever the state of the Event becomes Signalled.    */
Boolean_t BTPSAPI BTPS_WaitEvent(Event_t Event, unsigned long Timeout)
{
   EventHeader_t *eventHeader = (EventHeader_t *)Event;
   Boolean_t      ret_val   = FALSE;
   IArg           gateKey;
   UInt           uTimeout;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(eventHeader)
   {
      /* Obtain access to the event properties                          */
      gateKey = GateAll_enter(eventHeader->EventGate);

      /* Check to see if the current state indicates that we are        */
      /* currently reset.                                               */
      if(!eventHeader->IsSet)
      {
         /* The event is in the reset state.  Increment the count.      */
         eventHeader->Count++;

         /* Release access to the event counter                         */
         GateAll_leave(eventHeader->EventGate, gateKey);

         /* Convert the timeout into the appropriate amount of ticks    */
         if(Timeout == BTPS_INFINITE_WAIT)
            uTimeout = BIOS_WAIT_FOREVER;
         else
         {
            if(Timeout == BTPS_NO_WAIT)
               uTimeout = BIOS_NO_WAIT;
            else
            {
               /* Semaphore_pend takes system ticks, so convert         */
               uTimeout = (UInt)MILLISECONDS_TO_TICKS(Timeout);

               /* Make sure that the wait is not forever                */
               if(uTimeout == BIOS_WAIT_FOREVER)
                  uTimeout = ((UInt)BIOS_WAIT_FOREVER) - 1;
            }
         }

         /* Attempt to get the semaphore for the timeout duration.      */
         if(Semaphore_pend(eventHeader->EventMutex, uTimeout))
         {
            /* Mutex acquired successfully.                             */
            ret_val = TRUE;
         }

         if(!ret_val)
         {
            /* Decrement the count                                      */
            gateKey = GateAll_enter(eventHeader->EventGate);

            eventHeader->Count--;

            GateAll_leave(eventHeader->EventGate, gateKey);
         }
      }
      else
      {
         /* Release the event                                           */
         GateAll_leave(eventHeader->EventGate, gateKey);

         /* The event is in the set state, set the return value to true.*/
         ret_val = TRUE;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Non-Signalled State.  Once the Event   */
   /* is in this State, ALL calls to the BTPS_WaitEvent() function will */
   /* block until the State of the Event is set to the Signalled State. */
   /* This function accepts as input the Event Handle of the Event to   */
   /* set to the Non-Signalled State.                                   */
void BTPSAPI BTPS_ResetEvent(Event_t Event)
{
   EventHeader_t *eventHeader = (EventHeader_t *)Event;
   IArg           gateKey;

   /* Before proceeding any further we need to make sure that the       */
   /* parameter that was passed to us appears to be semi-valid.         */
   if(eventHeader)
   {
      /* Obtain access to the event                                     */
      gateKey = GateAll_enter(eventHeader->EventGate);

      /* We have successfully aquired the mutex, now check to see if the*/
      /* event is currently signaled.                                   */
      if(eventHeader->IsSet)
      {
         /* The event is currently set, change the flag and update the  */
         /* count.                                                      */
         eventHeader->IsSet = FALSE;
         eventHeader->Count++;

         /* Acquire semaphore to block calls in wait                    */
         Semaphore_pend(eventHeader->EventMutex, 0);
      }

      /* Release the event                                              */
      GateAll_leave(eventHeader->EventGate, gateKey);
   }
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State.  Once the Event is in */
   /* this State, ALL calls to the BTPS_WaitEvent() function will       */
   /* return.  This function accepts as input the Event Handle of the   */
   /* Event to set to the Signalled State.                              */
void BTPSAPI BTPS_SetEvent(Event_t Event)
{
   EventHeader_t *eventHeader = (EventHeader_t *)Event;
   IArg         gateKey;

   /* Before proceeding any further we need to make sure that the       */
   /* parameter that was passed to us appears to be semi-valid.         */
   if(eventHeader)
   {
      /* Obtain access to the event                                     */
      gateKey = GateAll_enter(eventHeader->EventGate);

      /* We have successfully aquired the mutex, now check to see if the*/
      /* event is currently signaled.                                   */
      if(!(eventHeader->IsSet))
      {
         eventHeader->IsSet = TRUE;

         /* The event is not currently set, change the flag and post the*/
         /* semaphore count times.                                      */
         while(eventHeader->Count--)
         {
            Semaphore_post(eventHeader->EventMutex);
         }

         eventHeader->Count = 0;
      }

      /* Release the event                                              */
      GateAll_leave(eventHeader->EventGate, gateKey);
   }
}


   /* The following function is responsible for destroying an Event that*/
   /* was created successfully via a successful call to the             */
   /* BTPS_CreateEvent() function.  This function accepts as input the  */
   /* Event Handle of the Event to destroy.  Once this function is      */
   /* completed the Event Handle is NO longer valid and CANNOT be       */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitEvent() functions to fail with an error.                 */
void BTPSAPI BTPS_CloseEvent(Event_t Event)
{
   EventHeader_t *eventHeader = (EventHeader_t *)Event;

   /* Before proceeding any further we need to make sure that the       */
   /* parameter that was passed to us appears to be semi-valid.         */
   if(eventHeader)
   {
      /* Set the event to force any outstanding calls to return.        */
      BTPS_SetEvent(Event);

      /* Delete the event semaphores                                    */
      Semaphore_delete(&(eventHeader->EventMutex));
      GateAll_delete(&(eventHeader->EventGate));

      /* Free memory allocated by BTPS_CreateEvent.                     */
      BTPS_FreeMemory(Event);
   }
}

   /* The following function is provided to allow a mechanism to        */
   /* actually allocate a Block of Memory (of at least the specified    */
   /* size).  This function accepts as input the size (in Bytes) of the */
   /* Block of Memory to be allocated.  This function returns a NON-NULL*/
   /* pointer to this Memory Buffer if the Memory was successfully      */
   /* allocated, or a NULL value if the memory could not be allocated.  */
void *BTPSAPI BTPS_AllocateMemory(unsigned long MemorySize)
{
   void  *ret_val;

   if((ret_val = _Malloc(MemorySize)) == NULL)
   {
      BTPS_OutputMessage("Malloc Failed: %d.\r\n", MemorySize * ALIGNMENT_SIZE);
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for de-allocating a Block   */
   /* of Memory that was successfully allocated with the                */
   /* BTPS_AllocateMemory() function.  This function accepts a NON-NULL */
   /* Memory Pointer which was returned from the BTPS_AllocateMemory()  */
   /* function.  After this function completes the caller CANNOT use    */
   /* ANY of the Memory pointed to by the Memory Pointer.               */
void BTPSAPI BTPS_FreeMemory(void *MemoryPointer)
{
   _MemFree(MemoryPointer);
}

   /* The following function is responsible for copying a block of      */
   /* memory of the specified size from the specified source pointer    */
   /* to the specified destination memory pointer.  This function       */
   /* accepts as input a pointer to the memory block that is to be      */
   /* Destination Buffer (first parameter), a pointer to memory block   */
   /* that points to the data to be copied into the destination buffer, */
   /* and the size (in bytes) of the Data to copy.  The Source and      */
   /* Destination Memory Buffers must contain AT LEAST as many bytes    */
   /* as specified by the Size parameter.                               */
   /* * NOTE * This function does not allow the overlapping of the      */
   /*          Source and Destination Buffers !!!!                      */
void BTPSAPI BTPS_MemCopy(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcpy() function.                     */
   memcpy(Destination, Source, Size);
}

   /* The following function is responsible for moving a block of       */
   /* memory of the specified size from the specified source pointer    */
   /* to the specified destination memory pointer.  This function       */
   /* accepts as input a pointer to the memory block that is to be      */
   /* Destination Buffer (first parameter), a pointer to memory block   */
   /* that points to the data to be copied into the destination buffer, */
   /* and the size (in bytes) of the Data to copy.  The Source and      */
   /* Destination Memory Buffers must contain AT LEAST as many bytes    */
   /* as specified by the Size parameter.                               */
   /* * NOTE * This function DOES allow the overlapping of the          */
   /*          Source and Destination Buffers.                          */
void BTPSAPI BTPS_MemMove(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memmove() function.                    */
   memmove(Destination, Source, Size);
}

   /* The following function is provided to allow a mechanism to fill   */
   /* a block of memory with the specified value.  This function accepts*/
   /* as input a pointer to the Data Buffer (first parameter) that is   */
   /* to filled with the specified value (second parameter).  The       */
   /* final parameter to this function specifies the number of bytes    */
   /* that are to be filled in the Data Buffer.  The Destination        */
   /* Buffer must point to a Buffer that is AT LEAST the size of        */
   /* the Size parameter.                                               */
void BTPSAPI BTPS_MemInitialize(void *Destination, unsigned char Value, unsigned long Size)
{
   /* Simply wrap the C Run-Time memset() function.                     */
   memset(Destination, Value, Size);
}

   /* The following function is provided to allow a mechanism to        */
   /* Compare two blocks of memory to see if the two memory blocks      */
   /* (each of size Size (in bytes)) are equal (each and every byte up  */
   /* to Size bytes).  This function returns a negative number if       */
   /* Source1 is less than Source2, zero if Source1 equals Source2, and */
   /* a positive value if Source1 is greater than Source2.              */
int BTPSAPI BTPS_MemCompare(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcmp() function.                     */
   return(memcmp(Source1, Source2, Size));
}

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes) */
   /* using a Case-Insensitive Compare.  This function returns a        */
   /* negative number if Source1 is less than Source2, zero if Source1  */
   /* equals Source2, and a positive value if Source1 is greater than   */
   /* Source2.                                                          */
int BTPSAPI BTPS_MemCompareI(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   int           ret_val = 0;
   unsigned char Byte1;
   unsigned char Byte2;
   unsigned int  Index;

   /* Simply loop through each byte pointed to by each pointer and check*/
   /* to see if they are equal.                                         */
   for (Index=0;((Index<Size) && (!ret_val));Index++)
   {
      /* Note each Byte that we are going to compare.                   */
      Byte1 = ((unsigned char *)Source1)[Index];
      Byte2 = ((unsigned char *)Source2)[Index];

      /* If the Byte in the first array is lower case, go ahead and make*/
      /* it upper case (for comparisons below).                         */
      if((Byte1 >= 'a') && (Byte1 <= 'z'))
         Byte1 = Byte1 - ('a' - 'A');

      /* If the Byte in the second array is lower case, go ahead and    */
      /* make it upper case (for comparisons below).                    */
      if((Byte2 >= 'a') && (Byte2 <= 'z'))
         Byte2 = Byte2 - ('a' - 'A');

      /* If the two Bytes are equal then there is nothing to do.        */
      if(Byte1 != Byte2)
      {
         /* Bytes are not equal, so set the return value accordingly.   */
         if(Byte1 < Byte2)
            ret_val = -1;
         else
            ret_val = 1;
      }
   }

   /* Simply return the result of the above comparison(s).              */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to        */
   /* copy a source NULL Terminated ASCII (character) String to the     */
   /* specified Destination String Buffer.  This function accepts as    */
   /* input a pointer to a buffer (Destination) that is to receive the  */
   /* NULL Terminated ASCII String pointed to by the Source parameter.  */
   /* This function copies the string byte by byte from the Source      */
   /* to the Destination (including the NULL terminator).               */
void BTPSAPI BTPS_StringCopy(char *Destination, BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strcpy() function.                     */
   strcpy(Destination, Source);
}

   /* The following function is provided to allow a mechanism to        */
   /* determine the Length (in characters) of the specified NULL        */
   /* Terminated ASCII (character) String.  This function accepts as    */
   /* input a pointer to a NULL Terminated ASCII String and returns     */
   /* the number of characters present in the string (NOT including     */
   /* the terminating NULL character).                                  */
unsigned int BTPSAPI BTPS_StringLength(BTPSCONST char *Source)
{
   int ret_val = 0;

   /* Simply wrap the C Run-Time strlen() function.                     */
   if((Source) && (*Source != 0x00))
   {
      ret_val = strlen(Source);
   }

   return(ret_val);
}

   /* The following function is provided to allow a means for the       */
   /* programmer to create a seperate thread of execution.  This        */
   /* function accepts as input the Function that represents the        */
   /* Thread that is to be installed into the system as its first       */
   /* parameter.  The second parameter is the size of the Threads       */
   /* Stack (in bytes) required by the Thread when it is executing.     */
   /* The final parameter to this function represents a parameter that  */
   /* is to be passed to the Thread when it is created.  This function  */
   /* returns a NON-NULL Thread Handle if the Thread was successfully   */
   /* created, or a NULL Thread Handle if the Thread was unable to be   */
   /* created.  Once the thread is created, the only way for the Thread */
   /* to be removed from the system is for the Thread function to run   */
   /* to completion.                                                    */
   /* * NOTE * There does NOT exist a function to Kill a Thread that    */
   /*          is present in the system.  Because of this, other means  */
   /*          needs to be devised in order to signal the Thread that   */
   /*          it is to terminate.                                      */
ThreadHandle_t BTPSAPI BTPS_CreateThread(Thread_t ThreadFunction, unsigned int StackSize, void *ThreadParameter)
{
   ThreadHandle_t       ret_val = NULL;
   Task_Params          taskParams;
   ThreadWrapperInfo_t *ThreadWrapperInfo;

   /* Wrap the OS Thread Creation function.                             */
   /* * NOTE * Becuase the OS Thread function and the BTPS Thread       */
   /*          function are not necessarily the same, we will wrap the  */
   /*          BTPS Thread within the real OS thread.                   */
   if(ThreadFunction)
   {
      /* First we need to allocate memory for a ThreadWrapperInfo_t     */
      /* structure to hold the data we are going to pass to the thread. */
      if((ThreadWrapperInfo = (ThreadWrapperInfo_t *)BTPS_AllocateMemory(sizeof(ThreadWrapperInfo_t))) != NULL)
      {
         /* Memory allocated, populate the structure with the correct   */
         /* information.                                                */
         BTPS_SprintF(ThreadWrapperInfo->Name, "BT%d", Ndx++);

         ThreadWrapperInfo->ThreadFunction  = ThreadFunction;
         ThreadWrapperInfo->ThreadParameter = ThreadParameter;

         /* Initialize the task parameters                              */
         Task_Params_init(&taskParams);
         taskParams.stackSize      = StackSize;
         taskParams.priority       = DEFAULT_THREAD_PRIORITY;
         taskParams.arg0           = (UArg)ThreadWrapperInfo;
         taskParams.instance->name = ThreadWrapperInfo->Name;

         /* Next attempt to create a thread using the default priority. */
         ret_val = Task_create((Task_FuncPtr)ThreadWrapper, &taskParams, NULL);
         if(!ret_val)
         {
            /* An error occurred while attempting to create the thread. */
            /* Free any previously allocated resources and set the      */
            /* return value to indicate and error has occurred.         */
            DBG_MSG(DBG_ZONE_BTPSKRNL, ("Task_create failed.\r\n"));
            BTPS_FreeMemory(ThreadWrapperInfo);
         }
      }
   }

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to        */
   /* retrieve the handle of the thread which is currently executing.   */
   /* This function require no input parameters and will return a valid */
   /* ThreadHandle upon success.                                        */
ThreadHandle_t BTPSAPI BTPS_CurrentThreadHandle(void)
{
   /* Simply return the Current Thread Handle that is executing.        */
   return(Task_selfMacro());
}

   /* The following function is provided to allow a mechanism to create */
   /* a Mailbox.  A Mailbox is a Data Store that contains slots (all    */
   /* of the same size) that can have data placed into (and retrieved   */
   /* from).  Once Data is placed into a Mailbox (via the               */
   /* BTPS_AddMailbox() function, it can be retreived by using the      */
   /* BTPS_WaitMailbox() function.  Data placed into the Mailbox is     */
   /* retrieved in a FIFO method.  This function accepts as input the   */
   /* Maximum Number of Slots that will be present in the Mailbox and   */
   /* the Size of each of the Slots.  This function returns a NON-NULL  */
   /* Mailbox Handle if the Mailbox is successfully created, or a       */
   /* NULL Mailbox Handle if the Mailbox was unable to be created.      */
Mailbox_t BTPSAPI BTPS_CreateMailbox(unsigned int NumberSlots, unsigned int SlotSize)
{
   Mailbox_t        ret_val;
   MailboxHeader_t *MailboxHeader;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if((NumberSlots) && (SlotSize))
   {
      /* Parameters appear semi-valid, so now let's allocate enough     */
      /* Memory to hold the Mailbox Header AND enough space to hold     */
      /* all requested Mailbox Slots.                                   */
      if((MailboxHeader = (MailboxHeader_t *)BTPS_AllocateMemory(sizeof(MailboxHeader_t)+(NumberSlots*SlotSize))) != NULL)
      {
         /* Memory successfully allocated so now let's create an        */
         /* Event that will be used to signal when there is Data        */
         /* present in the Mailbox.                                     */
         if((MailboxHeader->Event = BTPS_CreateEvent(FALSE)) != NULL)
         {
            /* Event created successfully, now let's create a Mutex     */
            /* that will guard access to the Mailbox Slots.             */
            if((MailboxHeader->Mutex = BTPS_CreateMutex(FALSE)) != NULL)
            {
               /* Everything successfully created, now let's initialize */
               /* the state of the Mailbox such that it contains NO     */
               /* Data.                                                 */
               MailboxHeader->NumberSlots   = NumberSlots;
               MailboxHeader->SlotSize      = SlotSize;
               MailboxHeader->HeadSlot      = 0;
               MailboxHeader->TailSlot      = 0;
               MailboxHeader->OccupiedSlots = 0;
               MailboxHeader->Slots         = ((unsigned char *)MailboxHeader) + sizeof(MailboxHeader_t);

               /* All finished, return success to the caller (the       */
               /* Mailbox Header).                                      */
               ret_val                      = (Mailbox_t)MailboxHeader;
            }
            else
            {
               /* Error creating the Mutex, so let's free the Event     */
               /* we created, Free the Memory for the Mailbox itself    */
               /* and return an error to the caller.                    */
               BTPS_CloseEvent(MailboxHeader->Event);

               BTPS_FreeMemory(MailboxHeader);

               ret_val = NULL;
            }
         }
         else
         {
            /* Error creating the Data Available Event, so let's free   */
            /* the Memory for the Mailbox itself and return an error    */
            /* to the caller.                                           */
            BTPS_FreeMemory(MailboxHeader);

            ret_val = NULL;
         }
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a means to Add data   */
   /* to the Mailbox (where it can be retrieved via the                 */
   /* BTPS_WaitMailbox() function.  This function accepts as input the  */
   /* Mailbox Handle of the Mailbox to place the data into and a        */
   /* pointer to a buffer that contains the data to be added.  This     */
   /* pointer *MUST* point to a data buffer that is AT LEAST the Size   */
   /* of the Slots in the Mailbox (specified when the Mailbox was       */
   /* created) and this pointer CANNOT be NULL.  The data that the      */
   /* MailboxData pointer points to is placed into the Mailbox where it */
   /* can be retrieved via the BTPS_WaitMailbox() function.             */
   /* * NOTE * This function copies from the MailboxData Pointer the    */
   /*          first SlotSize Bytes.  The SlotSize was specified when   */
   /*          the Mailbox was created via a successful call to the     */
   /*          BTPS_CreateMailbox() function.                           */
Boolean_t BTPSAPI BTPS_AddMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Since we are going to change the Mailbox we need to acquire    */
      /* the Mutex that is guarding access to it.                       */
      if(BTPS_WaitMutex(((MailboxHeader_t *)Mailbox)->Mutex, BTPS_INFINITE_WAIT))
      {
         /* Before adding the data to the Mailbox, make sure that the   */
         /* Mailbox is not already full.                                */
         if(((MailboxHeader_t *)Mailbox)->OccupiedSlots < ((MailboxHeader_t *)Mailbox)->NumberSlots)
         {
            /* Mailbox is NOT full, so add the specified User Data to   */
            /* the next available free Mailbox Slot.                    */
            BTPS_MemCopy(&(((unsigned char *)(((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->HeadSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), MailboxData, ((MailboxHeader_t *)Mailbox)->SlotSize);

            /* Update the Next available Free Mailbox Slot (taking      */
            /* into account wrapping the pointer).                      */
            if(++(((MailboxHeader_t *)Mailbox)->HeadSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
               ((MailboxHeader_t *)Mailbox)->HeadSlot = 0;

            /* Update the Number of occupied slots to signify that there*/
            /* was additional Mailbox Data added to the Mailbox.        */
            ((MailboxHeader_t *)Mailbox)->OccupiedSlots++;

            /* Signal the Event that specifies that there is indeed     */
            /* Data present in the Mailbox.                             */
            BTPS_SetEvent(((MailboxHeader_t *)Mailbox)->Event);

            /* Finally, return success to the caller.                   */
            ret_val = TRUE;
         }
         else
            ret_val = FALSE;

         /* All finished with the Mailbox, so release the Mailbox       */
         /* Mutex that we currently hold.                               */
         BTPS_ReleaseMutex(((MailboxHeader_t *)Mailbox)->Mutex);
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a means to retrieve   */
   /* data from the specified Mailbox.  This function will block until  */
   /* either Data is placed in the Mailbox or an error with the Mailbox */
   /* was detected.  This function accepts as its first parameter a     */
   /* Mailbox Handle that represents the Mailbox to wait for the data   */
   /* with.  This function accepts as its second parameter, a pointer   */
   /* to a data buffer that is AT LEAST the size of a single Slot of    */
   /* the Mailbox (specified when the BTPS_CreateMailbox() function     */
   /* was called).  The MailboxData parameter CANNOT be NULL.  This     */
   /* function will return TRUE if data was successfully retrieved      */
   /* from the Mailbox or FALSE if there was an error retrieving data   */
   /* from the Mailbox.  If this function returns TRUE then the first   */
   /* SlotSize bytes of the MailboxData pointer will contain the data   */
   /* that was retrieved from the Mailbox.                              */
   /* * NOTE * This function copies to the MailboxData Pointer the      */
   /*          data that is present in the Mailbox Slot (of size        */
   /*          SlotSize).  The SlotSize was specified when the Mailbox  */
   /*          was created via a successful call to the                 */
   /*          BTPS_CreateMailbox() function.                           */
Boolean_t BTPSAPI BTPS_WaitMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Next, we need to block until there is at least one Mailbox     */
      /* Slot occupied by waiting on the Data Available Event.          */
      if(BTPS_WaitEvent(((MailboxHeader_t *)Mailbox)->Event, BTPS_INFINITE_WAIT))
      {
         /* Since we are going to update the Mailbox, we need to acquire*/
         /* the Mutex that guards access to the Mailox.                 */
         if(BTPS_WaitMutex(((MailboxHeader_t *)Mailbox)->Mutex, BTPS_INFINITE_WAIT))
         {
            /* Let's double check to make sure that there does indeed   */
            /* exist at least one slot with Mailbox Data present in it. */
            if(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
            {
               /* Flag success to the caller.                           */
               ret_val = TRUE;

               /* Now copy the Data into the Memory Buffer specified    */
               /* by the caller.                                        */
               BTPS_MemCopy(MailboxData, &((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), ((MailboxHeader_t *)Mailbox)->SlotSize);

               /* Now that we've copied the data into the Memory Buffer */
               /* specified by the caller we need to mark the Mailbox   */
               /* Slot as free.                                         */
               if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
                  ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

               ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;

               /* If there is NO more data available in this Mailbox    */
               /* then we need to flag it as such by Resetting the      */
               /* Mailbox Data available Event.                         */
               if(!((MailboxHeader_t *)Mailbox)->OccupiedSlots)
                  BTPS_ResetEvent(((MailboxHeader_t *)Mailbox)->Event);
            }
            else
            {
               /* Internal error, flag that there is NO Data present    */
               /* in the Mailbox (by resetting the Data Available       */
               /* Event) and return failure to the caller.              */
               BTPS_ResetEvent(((MailboxHeader_t *)Mailbox)->Event);

               ret_val = FALSE;
            }

            /* All finished with the Mailbox, so release the Mailbox    */
            /* Mutex that we currently hold.                            */
            BTPS_ReleaseMutex(((MailboxHeader_t *)Mailbox)->Mutex);
         }
         else
            ret_val = FALSE;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is responsible for destroying a Mailbox    */
   /* that was created successfully via a successful call to the        */
   /* BTPS_CreateMailbox() function.  This function accepts as input    */
   /* the Mailbox Handle of the Mailbox to destroy.  Once this function */
   /* is completed the Mailbox Handle is NO longer valid and CANNOT be  */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMailbox() functions to fail with an error.  The final    */
   /* parameter specifies an (optional) callback function that is called*/
   /* for each queued Mailbox entry.  This allows a mechanism to free   */
   /* any resources that might be associated with each individual       */
   /* Mailbox item.                                                     */
void BTPSAPI BTPS_DeleteMailbox(Mailbox_t Mailbox, BTPS_MailboxDeleteCallback_t MailboxDeleteCallback)
{
   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* that was specified appears semi-valid.                            */
   if(Mailbox)
   {
      /* Mailbox appears semi-valid, so let's free all Events and       */
      /* Mutexes, perform any mailbox deletion callback actions, and    */
      /* finally free the Memory associated with the Mailbox itself.    */
      if(((MailboxHeader_t *)Mailbox)->Event)
         BTPS_CloseEvent(((MailboxHeader_t *)Mailbox)->Event);

      if(((MailboxHeader_t *)Mailbox)->Mutex)
         BTPS_CloseMutex(((MailboxHeader_t *)Mailbox)->Mutex);

      /* Check to see if a Mailbox Delete Item Callback was specified.  */
      if(MailboxDeleteCallback)
      {
         /* Now loop though all of the occupied slots and call the      */
         /* callback with the slot data.                                */
         while(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
         {
            __BTPSTRY
            {
               (*MailboxDeleteCallback)(&((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]));
            }
            __BTPSEXCEPT(1)
            {
               /* Do Nothing.                                           */
            }

            /* Now that we've called back with the data, we need to     */
            /* advance to the next slot.                                */
            if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
               ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

            /* Flag that there is one less occupied slot.               */
            ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;
         }
      }

      /* All finished cleaning up the Mailbox, simply free all          */
      /* memory that was allocated for the Mailbox.                     */
      BTPS_FreeMemory(Mailbox);
   }
}

   /* The following function is used to initialize the Platform module. */
   /* The Platform module relies on some static variables that are used */
   /* to coordinate the abstraction.  When the module is initially      */
   /* started from a cold boot, all variables are set to the proper     */
   /* state.  If the Warm Boot is required, then these variables need to*/
   /* be reset to their default values.  This function sets all static  */
   /* parameters to their default values.                               */
   /* * NOTE * The implementation is free to pass whatever information  */
   /*          required in this parameter.                              */
void BTPSAPI BTPS_Init(void *UserParam)
{
   Semaphore_Params params;
   MutexHeader_t    mutexHeader;
   Mutex_t          tempKernelMutex;

   /* Input parameter represents the Debug Message Output Callback      */
   /* function.                                                         */
   if(UserParam)
   {
      if(((BTPS_Initialization_t *)UserParam)->MessageOutputCallback)
         MessageOutputCallback  = ((BTPS_Initialization_t *)UserParam)->MessageOutputCallback;
   }
   else
   {
      MessageOutputCallback = NULL;
   }

   /* Initialize the static variables for this module.                  */
   Ndx      = 0;

   if(!Initialized)
   {
      /* Initialize heap head on first init only.                       */
      HeapHead = NULL;

      /* Since BTPS_CreateMutex uses the kernel mutex to malloc, we have*/
      /* to temporarily create the kernel mutex using OS calls          */
      Semaphore_Params_init(&params);
      params.mode            = Semaphore_Mode_BINARY;
      mutexHeader.MutexOwner = NULL;
      if((mutexHeader.Mutex = Semaphore_create(1, &params, NULL)) != NULL)
      {
         KernelMutex = (Mutex_t)&mutexHeader;

         /* Now, create the actual kernel mutex                         */
         if((tempKernelMutex = BTPS_CreateMutex(FALSE)) != NULL)
         {
            /* Delete the initial kernel mutex                          */
            Semaphore_delete(&(mutexHeader.Mutex));

            /* Finally, assign the actual dynamically allocated mutex   */
            KernelMutex = tempKernelMutex;

            /* Create the IO mutex                                      */
            if((IOMutex = BTPS_CreateMutex(FALSE)) != NULL)
            {
               Initialized = TRUE;
            }
            else
            {
               DBG_MSG(DBG_ZONE_BTPSKRNL,("Failed to Create IOMutex\r\n"));
            }
         }
         else
         {
            DBG_MSG(DBG_ZONE_BTPSKRNL,("Failed to Create tempKernelMutex\r\n"));
         }
      }
      else
      {
         DBG_MSG(DBG_ZONE_BTPSKRNL,("Failed to Create KernelMutex\r\n"));
      }

      DBG_MSG(DBG_ZONE_BTPSKRNL, ("\r\nInitialization Complete.\r\n"));
   }
}

   /* The following function is used to cleanup the Platform module.    */
void BTPSAPI BTPS_DeInit(void)
{
   Byte_t i;

   /* Give time for all threads to exit.                                */
   BTPS_Delay(50);

   /* Delete all tasks that exited.                                     */
   for(i = (sizeof(TaskDeleteList)/sizeof(Task_Handle)); i--;)
   {
      if(TaskDeleteList[i])
      {
         Task_delete(&TaskDeleteList[i]);
      }
      else
      {
         break;
      }
   }
}

   /* Write out the specified NULL terminated Debugging String to the   */
   /* Debug output.                                                     */
void BTPSAPI BTPS_OutputMessage(BTPSCONST char *DebugString, ...)
{
   int     ret_val;
   va_list args;

   /* Grab the I/O Mutex and send the Message over USB                  */
   if(BTPS_WaitMutex(IOMutex, BTPS_INFINITE_WAIT) == TRUE)
   {
      /* Write out the Data.                                            */
      va_start(args, DebugString);
      ret_val = uvsnprintf(MsgBuf, (sizeof(MsgBuf) - 1), DebugString, args);
      va_end(args);

      ConsoleWrite(MsgBuf, ret_val);

      /* Release the IOMutex                                            */
      BTPS_ReleaseMutex(IOMutex);
   }
}

   /* The following function is used to set the Debug Mask that controls*/
   /* which debug zone messages get displayed.  The function takes as   */
   /* its only parameter the Debug Mask value that is to be used.  Each */
   /* bit in the mask corresponds to a debug zone.  When a bit is set,  */
   /* the printing of that debug zone is enabled.                       */
void BTPSAPI BTPS_SetDebugMask(unsigned long DebugMask)
{
   DebugZoneMask = DebugMask;
}

   /* The following function is a utility function that can be used to  */
   /* determine if a specified Zone is currently enabled for debugging. */
int BTPSAPI BTPS_TestDebugZone(unsigned long Zone)
{
   return(DebugZoneMask & Zone);
}

   /* The following function is responsible for displaying binary debug */
   /* data.  The first parameter to this function is the length of data */
   /* pointed to by the next parameter.  The final parameter is a       */
   /* pointer to the binary data to be  displayed.                      */
int BTPSAPI BTPS_DumpData(unsigned int DataLength, BTPSCONST unsigned char *DataPtr)
{
   int                    ret_val;
   char                   Buffer[80];
   char                  *BufPtr;
   char                  *HexBufPtr;
   Byte_t                 DataByte;
   unsigned int           Index;
   static BTPSCONST char  HexTable[] = "0123456789ABCDEF\r\n";
   static BTPSCONST char  Header1[]  = "       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ";
   static BTPSCONST char  Header2[]  = " ------------------------------------------------------\r\n";

   /* Before proceeding any further, lets make sure that the parameters */
   /* passed to us appear semi-valid.                                   */
   if((DataLength > 0) && (DataPtr != NULL))
   {
      /* The parameters which were passed in appear to be at least      */
      /* semi-valid, next write the header out to the file.             */
      BTPS_OutputMessage((char *)Header1);
      BTPS_OutputMessage((char *)HexTable);
      BTPS_OutputMessage((char *)Header2);

      /* Limit the number of bytes that will be displayed in the dump.  */
      if(DataLength > MAX_DBG_DUMP_BYTES)
      {
         DataLength = MAX_DBG_DUMP_BYTES;
      }

      /* Now that the Header is written out, let's output the Debug     */
      /* Data.                                                          */
      BTPS_MemInitialize(Buffer, ' ', sizeof(Buffer));
      BufPtr    = Buffer + BTPS_SprintF(Buffer," %05X ", 0);
      HexBufPtr = Buffer + sizeof(Header1)-1;
      for (Index=0; Index < DataLength;)
      {
         Index++;
         DataByte     = *DataPtr++;
         *BufPtr++    = HexTable[(Byte_t)(DataByte >> 4)];
         *BufPtr      = HexTable[(Byte_t)(DataByte & 0x0F)];
         BufPtr      += 2;

         /* Check to see if this is a printable character and not a     */
         /* special reserved character.  Replace the non-printable      */
         /* characters with a period.                                   */
         *HexBufPtr++ = (char)(((DataByte >= ' ') && (DataByte <= '~') && (DataByte != '\\') && (DataByte != '%'))?DataByte:'.');
         if(((Index % MAXIMUM_BYTES_PER_ROW) == 0) || (Index == DataLength))
         {
            /* We are at the end of this line, so terminate the data    */
            /* compiled in the buffer and send it to the output device. */
            *HexBufPtr++ = '\r';
            *HexBufPtr++ = '\n';
            *HexBufPtr   = 0;
            BTPS_OutputMessage(Buffer);

            if(Index != DataLength)
            {
               /* We have more to process, so prepare for the next line.*/
               BTPS_MemInitialize(Buffer, ' ', sizeof(Buffer));
               BufPtr    = Buffer + BTPS_SprintF(Buffer," %05X ", Index);
               HexBufPtr = Buffer + sizeof(Header1)-1;
            }
            else
            {
               /* Flag that there is no more data.                      */
               HexBufPtr = NULL;
            }
         }
      }

      /* Check to see if there is partial data in the buffer.           */
      if((long)HexBufPtr > 0)
      {
         /* Terminate the buffer and output the line.                   */
         *HexBufPtr++ = '\r';
         *HexBufPtr++ = '\n';
         *HexBufPtr   = 0;
         BTPS_OutputMessage(Buffer);
      }
      BTPS_OutputMessage("\r\n");

      /* Finally, set the return value to indicate success.             */
      ret_val = 0;
   }
   else
   {
      /* An invalid parameter was enterer.                              */
      ret_val = -1;
   }

   return(ret_val);
}

#endif
