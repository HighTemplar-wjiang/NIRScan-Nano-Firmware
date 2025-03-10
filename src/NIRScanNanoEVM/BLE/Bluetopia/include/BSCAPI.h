/*****< bscapi.h >*************************************************************/
/*      Copyright 2000 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BSCAPI - Stonestreet One Bluetooth Stack Controller API Type Definitions, */
/*           Constants, and Prototypes.                                       */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   09/11/00  D. Lange       Initial creation.                               */
/*   09/18/08  T. Thomas      Updates for BT 2.1                              */
/******************************************************************************/
#ifndef __BSCAPIH__
#define __BSCAPIH__

#include "BTPSKRNL.h"           /* BTPS Kernel Prototypes/Constants.          */

#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "BTTypes.h"            /* Bluetooth Type Definitions/Constants.      */
#include "HCITypes.h"           /* Bluetooth HCI Type Definitions/Constants.  */

#include "BTPSCFG.h"            /* BTPS Configuration Constants.              */

   /* The following definitions, represent BIT Flags that can be used   */
   /* with the BSC_Initialize() function.  These Bit Flags can be used  */
   /* to inform the Bluetooth Stack Controller of various Requested     */
   /* options.                                                          */
#define BSC_INITIALIZE_FLAG_NO_L2CAP                            0x00000001L
#define BSC_INITIALIZE_FLAG_NO_SCO                              0x00000002L
#define BSC_INITIALIZE_FLAG_NO_SDP                              0x00000004L
#define BSC_INITIALIZE_FLAG_NO_RFCOMM                           0x00000008L
#define BSC_INITIALIZE_FLAG_NO_GAP                              0x00001000L
#define BSC_INITIALIZE_FLAG_NO_SPP                              0x00002000L
#define BSC_INITIALIZE_FLAG_NO_GOEP                             0x00004000L
#define BSC_INITIALIZE_FLAG_NO_OTP                              0x00008000L

   /* The following type represents the defined List Entry Keys that    */
   /* can be searched on using the Generic List Entry routines that are */
   /* present in this file.                                             */
typedef enum
{
   ekNone,
   ekBoolean_t,
   ekByte_t,
   ekWord_t,
   ekDWord_t,
   ekBD_ADDR_t,
   ekEntryPointer,
   ekUnsignedInteger
} BSC_Generic_List_Entry_Key_t;

   /* The following constants are used with the following functions:    */
   /*    - BSC_EnableFeature()                                          */
   /*    - BSC_DisableFeature()                                         */
   /*    - BSC_QueryActiveFeatures()                                    */
   /* * NOTE * This functionality is not normally supported by default  */
   /*          (i.e. a custom stack build is required to enable this    */
   /*          functionality).                                          */
#define BSC_FEATURE_BLUETOOTH_LOW_ENERGY                  0x00000001
#define BSC_FEATURE_ANT_PLUS                              0x00000002
#define BSC_FEATURE_WIDE_BAND_SPEECH                      0x00000004
#define BSC_FEATURE_A3DP_SOURCE                           0x00000008
#define BSC_FEATURE_A3DP_SINK                             0x00000010

   /* The following defines the valid Result Values that can be returned*/
   /* in the Result field of the Authentication Request Event.          */
#define BSC_AUTHENTICATION_REQUEST_RESULT_SUCCESS               0x00
#define BSC_AUTHENTICATION_REQUEST_RESULT_IN_PROGRESS           0x01
#define BSC_AUTHENTICATION_REQUEST_RESULT_REFUSED               0x02
#define BSC_AUTHENTICATION_REQUEST_RESULT_FAILURE               0x03

   /* The following enumerated type represents the BSC Event Reason     */
   /* (and valid Data) and is used with the BSC Event Callback.         */
typedef enum
{
   etAuthenticationRequest
} BSC_Event_Type_t;

   /* The following structure defines the Authentication Request Event  */
   /* data.                                                             */
typedef struct _tagBSC_Authentication_Request_t
{
  BD_ADDR_t  BD_ADDR;
  Byte_t    *Result;
} BSC_Authentication_Request_t;

#define BSC_AUTHENTICATION_REQUEST_SIZE         (sizeof(BSC_Authentication_Request_t))

   /* The following structure represents the container structure that   */
   /* holds all BSC Event Data Data.                                    */
typedef struct _tagBSC_Event_Data_t
{
   BSC_Event_Type_t  Event_Type;
   DWord_t           Event_Data_Length;
   union
   {
      BSC_Authentication_Request_t *BSC_Authentication_Request;
   } Event_Data;
} BSC_Event_Data_t;

#define BSC_EVENT_DATA_SIZE                     (sizeof(BSC_Event_Data_t))

   /* The following declared type represents the Prototype Function for */
   /* an Bluetooth Stack Timer Callback.  This function will be called  */
   /* whenever an installed Timer has expired (installed via the        */
   /* BSC_StartTimer() function).  This function passes to the caller   */
   /* the Bluetooth Stack ID that the Timer is valid for, the Timer ID  */
   /* of the expired Timer (returned from BSC_StartTimer()) and the     */
   /* Timer Callback Parameter that was specified when this Callback was*/
   /* installed.  This function is guaranteed NOT to be invoked more    */
   /* than once simultaneously for the specified installed callback     */
   /* (i.e. this function DOES NOT have be reentrant).  It should also  */
   /* be noted that this function is called in the Thread Context of a  */
   /* Thread that the User does NOT own.  Therefore, processing in this */
   /* function should be as efficient as possible (this argument holds  */
   /* anyway because no other Timer and/or Stack Callbacks will be      */
   /* issued while function call is outstanding).                       */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving other Stack Events. */
   /*            A Deadlock WILL occur because NO Stack Event Callbacks */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
typedef void (BTPSAPI *BSC_Timer_Callback_t)(unsigned int BluetoothStackID, unsigned int TimerID, unsigned long CallbackParameter);

   /* The following declared type represents the Prototype Function for */
   /* an Bluetooth Stack Debug Data Callback.  This function will be    */
   /* called whenever a complete HCI Packet has been sent or received by*/
   /* the Bluetooth Device that was opened with the Bluetooth Protocol  */
   /* Stack.  This function passes to the caller the HCI Packet that    */
   /* was received and the Debug Callback Parameter that was specified  */
   /* when this Callback was installed.  The caller is free to use the  */
   /* contents of the HCI Packet ONLY in the context of this callback.  */
   /* this callback.  If the caller requires the Data for a longer      */
   /* period of time, then the callback function MUST copy the data     */
   /* into another Data Buffer.  This function is guaranteed NOT to be  */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e. this  function DOES NOT have be reentrant).  It    */
   /* should also be noted that this function is called in the Thread   */
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because Packet Processing             */
   /* (Sending/Receiving) will be suspended while function call is      */
   /* outstanding).                                                     */
   /* ** NOTE ** THIS FUNCTION MUST NOT CALL ANY BLUETOOTH STACK        */
   /*            FUNCTIONS !!!!! FAILURE TO FOLLOW THIS GUIDELINE WILL  */
   /*            RESULT IN POTENTIAL DEADLOCKS AND/OR ERRATIC           */
   /*            BEHAVIOR !!!!!!!!!!                                    */
   /*            The Debug Callback is a VERY LOW LEVEL Callback and as */
   /*            such, does NOT allow the Bluetooth Stack to be         */
   /*            re-entrant !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
typedef void (BTPSAPI *BSC_Debug_Callback_t)(unsigned int BluetoothStackID, Boolean_t PacketSent, HCI_Packet_t *HCIPacket, unsigned long CallbackParameter);

   /* The following declared type represents the Prototype Function for */
   /* a Bluetooth Stack Cleanup Function Callback.  This function will  */
   /* be called once, whenever a call is made to shutdown the Bluetooth */
   /* stack.  This function is called from within the context of the    */
   /* BSC_Shutdown function. This function is guaranteed NOT to be      */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e. this  function DOES NOT have be reentrant). The    */
   /* first parameter specifies the Bluetooth Stack identifier obtained */
   /* from the successful BSC_Initialize function return.  The second   */
   /* Parameter is a User defined value that will passed to the         */
   /* Callback Parameter when it is called.                             */
typedef void (BTPSAPI *BSC_Cleanup_Callback_t)(unsigned int BluetoothStackID, unsigned long CallbackParameter);

   /* The following declared type represents the Prototype Function for */
   /* the BSC Event Receive Data Callback.  This function will be called*/
   /* whenever a Callback has been registered for the specified BSC     */
   /* Action that is associated with the specified Bluetooth Stack ID.  */
   /* This function passes to the caller the Bluetooth Stack ID, the BSC*/
   /* Event Data of the specified Event, and the BSC Event Callback     */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the BSC Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* reentrant).  Because of this, the processing in this function     */
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other BSC Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving other BSC Events.  A*/
   /*            Deadlock WILL occur because NO BSC Event Callbacks will*/
   /*            be issued while this function is currently outstanding.*/
typedef void (BTPSAPI *BSC_Event_Callback_t)(unsigned int BluetoothStackID, BSC_Event_Data_t *BSC_Event_Data, unsigned long CallbackParameter);

   /* The following declared type represents the Prototype Function for */
   /* the BSC Asynchronous Callback.  This function will be called once */
   /* for each call to BSC_ScheduleAsynchronousCallback.  This function */
   /* passes to the caller the Bluetooth Stack ID and the user specified*/
   /* Callback Parameter that was passed into the                       */
   /* BSC_ScheduleAsynchronousCallback function.  It should also be     */
   /* noted that this function is called in the Thread Context of a     */
   /* Thread that the User does NOT own.  Therefore, processing in this */
   /* function should be as efficient as possible.                      */
   /* ** NOTE ** The caller should keep the processing of these         */
   /*            Callbacks small because other Events will not be able  */
   /*            to be called while one is being serviced.              */
typedef void (BTPSAPI *BSC_AsynchronousCallbackFunction_t)(unsigned int BluetoothStackID, unsigned long CallbackParameter);

   /* The following function is responsible for Initializing a Bluetooth*/
   /* Protocol Stack for the specified Bluetooth Device (using the      */
   /* specified HCI Transport).  This function *MUST* be called (and    */
   /* complete successfully) before any function in this module can be  */
   /* called.  The first parameter specifies the Bluetooth HCI Driver   */
   /* Transport Information to use when opening the Bluetooth Device    */
   /* and the second parameter specifies Flags that are to be used to   */
   /* alter the base Bluetooth Protocol Stack Functionality.  The HCI   */
   /* Driver Information parameter is NOT optional and must specify a   */
   /* valid Bluetooth HCI Driver transport provided by this Protocol    */
   /* Stack Implementation.  The flags parameter should be zero unless  */
   /* altered functionality is required.  Upon successfuly completion,  */
   /* this function will return a positive, non-zero, return value.     */
   /* This value will be used as input to functions provided by the     */
   /* Bluetooth Protocol Stack that require a Bluetooth Protocol Stack  */
   /* ID (functions that operate directly on a Bluetooth Device).  If   */
   /* this function fails, the return value will be a negative return   */
   /* code which specifies the error (see error codes defined           */
   /* elsewhere).  Once this function completes the specified Bluetooth */
   /* Protocol Stack ID will remain valid for the specified Bluetooth   */
   /* Device until the Bluetooth Stack is Closed via a call to the      */
   /* BSC_Shutdown() function.                                          */
   /* * NOTE * The Bit Mask values for the Flags Parameter are defined  */
   /*          at the top of this file.                                 */
BTPSAPI_DECLARATION int BTPSAPI BSC_Initialize(HCI_DriverInformation_t *HCI_DriverInformation, unsigned long Flags);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_Initialize_t)(HCI_DriverInformation_t *HCI_DriverInformation, unsigned long Flags);
#endif

   /* The following function is responsible for Closing the Bluetooth   */
   /* Protocol Stack that was opened for the Bluetooth Device specified */
   /* via a successful call to the BSC_Initialize() function.  The Input*/
   /* parameter to this function MUST have been acquired by a successful*/
   /* call to the BSC_Initialize() function.  Once this function        */
   /* completes, the Bluetooth Device that was opened (and the Bluetooth*/
   /* Protocol Stack that is associated with the Device) cannot be      */
   /* accessed again until the Device (and a corresponding Bluetooth    */
   /* Protocol Stack is Re-Opened by calling the BSC_Initialize()       */
   /* function.                                                         */
BTPSAPI_DECLARATION void BTPSAPI BSC_Shutdown(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_Shutdown_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is a Debugging function that allows the    */
   /* caller to register a Debugging Callback that will be called EACH  */
   /* Time a HCI Packet is Sent or Received.  Because this Function will*/
   /* be called every time a Packet is Sent or Received, this function  */
   /* should only be used when debugging is required because of the     */
   /* Performance Penalty that is present when using this mechanism.    */
   /* The first parameter is obtained by a successful call to the      */
   /* BSC_Initialize() function.  The second parameter is a pointer to  */
   /* a function that is to be called when an HCI Packet (of any type)  */
   /* is Sent OR Received on the specified Bluetooth Protocol Stack     */
   /* (specified by the first parameter).  The CallbackParameter        */
   /* Parameter is a User defined value that will passed to the         */
   /* Callback Parameter when it is called.  The return value of this   */
   /* function will be a non-zero, non-negative value on success        */
   /* or a negative return code if there was a failure.  Once a Debug   */
   /* Callback has been installed, it can ONLY be removed via a call to */
   /* the BSC_UnRegisterDebugCallback() function.                       */
BTPSAPI_DECLARATION int BTPSAPI BSC_RegisterDebugCallback(unsigned int BluetoothStackID, BSC_Debug_Callback_t BSC_DebugCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_RegisterDebugCallback_t)(unsigned int BluetoothStackID, BSC_Debug_Callback_t BSC_DebugCallback, unsigned long CallbackParameter);
#endif

   /* The following function is responsible for Removing a previously   */
   /* installed Debug Callback for the specified Bluetooth Protocol     */
   /* Stack (specified by the specified BluetoothStackID parameter).    */
   /* After this function has completed, the caller will no longer be   */
   /* Notified via the Debug Callback Function when a debug event       */
   /* occurs.                                                           */
BTPSAPI_DECLARATION void BTPSAPI BSC_UnRegisterDebugCallback(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_UnRegisterDebugCallback_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is provided to allows the caller to        */
   /* register an Event Callback that will be called when an upper layer*/
   /* module requires a specific function that is provided by another   */
   /* layer.  The first parameter is obtained by a successful call to   */
   /* the BSC_Initialize() function.  The second parameter is a pointer */
   /* to a function that is to be called when an BSC Event is           */
   /* dispatched.  The CallbackParameter Parameter is a User defined    */
   /* value that will passed to the Callback Function when it is        */
   /* called.  The return value of this function will be a non-zero,    */
   /* non-negative value on success or a negative return code if there  */
   /* was a failure.  Once an Event Callback has been installed, it can */
   /* ONLY be removed via a call to the BSC_UnRegisterEventCallback()   */
   /* function.                                                         */
BTPSAPI_DECLARATION int BTPSAPI BSC_RegisterEventCallback(unsigned int BluetoothStackID, BSC_Event_Callback_t BSC_EventCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_RegisterEventCallback_t)(unsigned int BluetoothStackID, BSC_Event_Callback_t BSC_EventCallback, unsigned long CallbackParameter);
#endif

   /* The following function is responsible for Removing a previously   */
   /* installed EVent Callback for the specified Bluetooth Protocol     */
   /* Stack (specified by the specified BluetoothStackID parameter).    */
   /* After this function has completed, the caller will no longer be   */
   /* Notified via the Event Callback Function when a BSC event occurs. */
BTPSAPI_DECLARATION void BTPSAPI BSC_UnRegisterEventCallback(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_UnRegisterEventCallback_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is a low-level primitive that allows the   */
   /* caller the ability to 'Lock' the Bluetooth Protocol Stack so that */
   /* no other thread has access to the specified Bluetooth Protocol    */
   /* Stack.  This functionality is provided to allow the programmer the*/
   /* means to guard against the Bluetooth Protocol Stack calling a     */
   /* Bluetooth Event Callback and potentially leading to a Thread      */
   /* Problem if Mutexes/Semaphores are being used.  This function is   */
   /* completely safe to be called in any context (except Debug         */
   /* Callbacks).  This function accepts as its only parameter the      */
   /* Bluetooth Stack ID of the Bluetooth Protocol Stack to 'Lock'.     */
   /* This function returns zero if the specified Bluetooth Protocol    */
   /* Stack was successfully 'Locked' or a negative return error code if*/
   /* an error occurred.  Once the Bluetooth Stack is successfully      */
   /* Locked it *MUST* be unlocked manually by calling the              */
   /* BSC_UnLockBluetoothStack() function.  Failure to call the UnLock  */
   /* function after successfully locking the Bluetooth Protocol Stack  */
   /* will result in the Bluetooth Stack to quit responding.            */
   /* * NOTE * This function is NOT required to be called before        */
   /*          calling Bluetooth Protocol Stack functions !!!!!!!!!!!!! */
   /* * NOTE * This function is provided for applications that need     */
   /*          require the Bluetooth Protocol Stack to force all        */
   /*          Resource protection.  In many cases (almost all) this    */
   /*          function (and it's counterpart, the                      */
   /*          the BSC_UnLockBluetoothStack() function) will not need   */
   /*          to be used.  This function SHOULD BE USED WITH EXTREME   */
   /*          CAUTION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!   */
   /* * NOTE * Multiple calls to this function are possible, however    */
   /*          there *MUST* be a corresponding call to the              */
   /*          BSC_UnLockBluetoothStack() function for each successful  */
   /*          call that is made to this function.  Failure to call the */
   /*          BSC_UnLockBluetoothStack() function the same amount of   */
   /*          times will yield the Bluetooth Stack un-usable.          */
   /* * NOTE * The programmer using this function can guard against the */
   /*          case of being interrupted by the Bluetooth Protocol Stack*/
   /*          while issuing more than one Bluetooth Protocol Stack     */
   /*          operation.  This allows the caller some guarantee that   */
   /*          the stack will not cause a non-reentrant operation on    */
   /*          data structures that might need to be protected.  This   */
   /*          function allows the caller to use the same protection    */
   /*          that the Bluetooth Protocol Stack uses to avoid potential*/
   /*          dead-lock conditions.                                    */
BTPSAPI_DECLARATION int BTPSAPI BSC_LockBluetoothStack(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_LockBluetoothStack_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is provided to allow the programmer a      */
   /* mechanism for releasing an existing 'Lock' that was acquired on   */
   /* the specified Bluetooth Protocol Stack.  This function accepts as */
   /* its parameter the Bluetooth Protocol Stack ID of the Bluetooth    */
   /* Protocol Stack that is currently 'Locked'.  The Bluetooth         */
   /* Protocol Stack is considered 'Locked' if the the caller called the*/
   /* BSC_LockBluetoothStack() function (and it was successful).  Please*/
   /* see the BSC_LockBluetoothStack() function for more information.   */
BTPSAPI_DECLARATION void BTPSAPI BSC_UnLockBluetoothStack(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_UnLockBluetoothStack_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is provided to allow a mechanism to start  */
   /* a Timer of the specified Time out (in Milliseconds and NOT zero), */
   /* and call the specified Timer Callback function when the timer     */
   /* expires.  This function accepts as input the Bluetooth Stack ID   */
   /* of the Bluetooth Stack that the specified Timer is valid for, the */
   /* Timeout value (in Milliseconds) of the Timer, the Timer Callback, */
   /* and Callback Parameter to call when the Timer expires.  The       */
   /* Callback Parameter that is specified is passed to the caller when */
   /* the Timer Callback function is called.  This function returns the */
   /* Timer ID of the newly created Timer if successful (positive AND   */
   /* NON-zero).  This Timer ID can be used when calling BSC_StopTimer()*/
   /* function if the specified Timer has not expired.  The Timer ID's  */
   /* are always greater than zero.  If the Timer was not able to be    */
   /* started, a negative return error code is returned.                */
   /* * NOTE *  All Timers supported by this module are one-shot timers */
   /*           only.  This means that after the Timer expires and the  */
   /*           the caller is notified, no other Timeout will occur.    */
BTPSAPI_DECLARATION int BTPSAPI BSC_StartTimer(unsigned int BluetoothStackID, unsigned long Timeout, BSC_Timer_Callback_t BSC_TimerCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_StartTimer_t)(unsigned int BluetoothStackID, unsigned long Timeout, BSC_Timer_Callback_t BSC_TimerCallback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism for       */
   /* stopping a Timer that was previously started with the             */
   /* BSC_StartTimer() function.  This function accepts as input the    */
   /* Bluetooth Stack ID of the Bluetooth Stack that the Timer was      */
   /* started on, and the Timer ID of the specified Timer (returned from*/
   /* a successful call to the BSC_StartTimer() function).  This        */
   /* function returns zero if successful, or a negative return error   */
   /* code if there was error stopping the specified Timer.             */
BTPSAPI_DECLARATION int BTPSAPI BSC_StopTimer(unsigned int BluetoothStackID, unsigned int TimerID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_StopTimer_t)(unsigned int BluetoothStackID, unsigned int TimerID);
#endif

   /* The following function is provided to allow a mechanism for any   */
   /* layer to request that a connected device be authenticated.  This  */
   /* function accepts as input the Bluetooth Stack ID of the Bluetooth */
   /* Stack that the Device is associated with.  The second parameter is*/
   /* the Bluetooth Address of the connected device that requires       */
   /* Authentication.  The third parameter is a pointer to a Result     */
   /* variable that indicates the state of the request.  This function  */
   /* returns zero if successful, or a negative return error code if the*/
   /* Authentication process was not started.                           */
BTPSAPI_DECLARATION int BTPSAPI BSC_AuthenticateDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Byte_t *Result);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_AuthenticateDevice_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Byte_t *Result);
#endif

   /* The following function is provided to allow a mechanism for any   */
   /* layer request the stack controller to enable an individual feature*/
   /* of the Bluetooth Protocol Stack.  This function returns zero if   */
   /* successful or a negative return error code if there was an error. */
   /* * NOTE * This functionality is not normally supported by default  */
   /*          (i.e. a custom stack build is required to enable this    */
   /*          functionality).                                          */
BTPSAPI_DECLARATION int BTPSAPI BSC_EnableFeature(unsigned int BluetoothStackID, unsigned long Feature);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_EnableFeature_t)(unsigned int BluetoothStackID, unsigned long Feature);
#endif

   /* The following function is provided to allow a mechanism for any   */
   /* layer request the stack controller to disable an individual       */
   /* feature of the Bluetooth Protocol Stack.  This function returns   */
   /* zero if successful or a negative return error code if there was an*/
   /* error.                                                            */
   /* * NOTE * This functionality is not normally supported by default  */
   /*          (i.e. a custom stack build is required to enable this    */
   /*          functionality).                                          */
BTPSAPI_DECLARATION int BTPSAPI BSC_DisableFeature(unsigned int BluetoothStackID, unsigned long Feature);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_DisableFeature_t)(unsigned int BluetoothStackID, unsigned long Feature);
#endif

   /* The following function is provided to allow a mechanism for any   */
   /* layer query the stack controller to determine if an individual    */
   /* feature of the Bluetooth Protocol Stack is currently active.  This*/
   /* function returns zero if successful or a negative return error    */
   /* code if there was an error.                                       */
   /* * NOTE * This functionality is not normally supported by default  */
   /*          (i.e. a custom stack build is required to enable this    */
   /*          functionality).                                          */
BTPSAPI_DECLARATION int BTPSAPI BSC_QueryActiveFeatures(unsigned int BluetoothStackID, unsigned long *Feature);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_QueryActiveFeatures_t)(unsigned int BluetoothStackID, unsigned long *Feature);
#endif

   /* The following function is provided as a mechanism to determine if */
   /* the specified Bluetooth Protocol Stack is "Idle".  "Idle" in this */
   /* case means that there is no pending processing.  This is useful   */
   /* in single threaded operating environments to be able to allow the */
   /* processor to sleep for power savings.  This function returns      */
   /* BOOLEAN TRUE if the stack is currently idle or FALSE if the       */
   /* specified Bluetooth stack is either currently processing data or  */
   /* has pending data to process.                                      */
   /* * NOTE * This function is only applicable for single threaded     */
   /*          operating environments.  On multi-threaded operating     */
   /*          environments this function always returns TRUE.          */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BSC_QueryStackIdle(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BSC_QueryStackIdle_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is provided as a mechanism for scheduling  */
   /* an Asynchronous Callback.  This function will be scheduled to be  */
   /* Dispatched (called back) from another Thread.  This function      */
   /* accepts as input the Bluetooth Stack ID of the Bluetooth Stack    */
   /* that the Device is associated with.  The second parameter is a    */
   /* pointer to a function that is to be scheduled to be run           */
   /* asynchronously.  The CallbackParameter Parameter is a User defined*/
   /* value that will passed to the Callback Function when it is called.*/
   /* This function returns a non-zero value if successful or zero if   */
   /* the callback was not able to be scheduled.                        */
   /* * NOTE * Once an Asynchronous Callback is scheduled it cannot be  */
   /*          removed.                                                 */
BTPSAPI_DECLARATION int BTPSAPI BSC_ScheduleAsynchronousCallback(unsigned int BluetoothStackID, BSC_AsynchronousCallbackFunction_t AsynchronousCallbackFunction, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BSC_ScheduleAsynchronousCallback_t)(unsigned int BluetoothStackID, BSC_AsynchronousCallbackFunction_t AsynchronousCallbackFunction, unsigned long CallbackParameter);
#endif

   /* The following function is an internal function that exists to     */
   /* acquire a global lock that can be used to search lists that are   */
   /* maintained by modules (for resource tracking).  This Lock CANNOT  */
   /* be held while holding or acquiring any other lock.  This          */
   /* functionality is provided to allow a mechanism on smaller         */
   /* (embedded) systems so that individual modules (such as the HCI    */
   /* Drivers) to do not have to waste resources for Mutexes to protect */
   /* their internal lists.                                             */
   /* * NOTE * The caller *MUST* call BSC_ReleaseListLock() when        */
   /*          finished to release the lock.                            */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BSC_AcquireListLock(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BSC_AcquireListLock_t)(void);
#endif

   /* The following function is provided to allow a mechansim for the   */
   /* caller to release the List Lock (previously acquired via a        */
   /* successful call to the BSC_AcquireListLock() function).           */
BTPSAPI_DECLARATION void BTPSAPI BSC_ReleaseListLock(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_ReleaseListLock_t)(void);
#endif

   /* The following function is a utility function that adds the actual */
   /* specified List Entry to the specified Generic List Entry List.    */
   /* This function accepts as the first parameter, the Key Type that is*/
   /* to be used to determine if a duplicate entry already exists in the*/
   /* list (used in conjunction with the second parameter).  The second */
   /* parameter specifies the byte offset of the actual Key itself in   */
   /* the Entry structure (note that this parameter is ignored if ekNone*/
   /* is specified as the List Entry Key Type - this means that the     */
   /* entry is to be added to the list without checking for duplicates).*/
   /* The third parameter specifies the actual byte offset of the next  */
   /* element pointer that is present in the List Entry structure.  The */
   /* fourth parameter specifies a pointer to the Head of the Generic   */
   /* List itself (a pointer to a pointer to the first element present  */
   /* in the list).  The final parameter specifies the actual List Entry*/
   /* value to add.  This function returns a BOOLEAN TRUE value if      */
   /* successful or BOOLEAN FALSE if the specified entry could not be   */
   /* added to the list.                                                */
   /* * NOTE * Valid values must be specified for the following         */
   /*          parameters (or the function will fail):                  */
   /*             - ListHead - parameter cannot be NULL, but the value  */
   /*               that it points to can be NULL.                      */
   /*             - ListEntryToAdd - parameter cannot be NULL, and it   */
   /*               must point to the List Entry Data that is to be     */
   /*               added (of size ListEntrySize).                      */
   /* * NOTE * If the GenericListEntryKey value is anything other than  */
   /*          ekNone, then this function does not insert duplicate     */
   /*          entries into the list.  An item is considered a duplicate*/
   /*          if the Key value of the entry being added matches any    */
   /*          Key value of an entry already in the list.  When this    */
   /*          parameter is ANYTHING OTHER THAN ekNone, then the        */
   /*          following parameters must be specified:                  */
   /*             - ListEntryKeyOffset - specifies the byte offset of   */
   /*               the Generic List Entry Key Element (in each         */
   /*               individual List Entry).                             */
   /* * NOTE * In all cases, the ListEntryNextPointerOffset parameter   */
   /*          MUST specify the byte offset of the element that         */
   /*          represents a pointer to the next element that is present */
   /*          in the list (for each individual List Entry).            */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BSC_AddGenericListEntry_Actual(BSC_Generic_List_Entry_Key_t GenericListEntryKey, unsigned int ListEntryKeyOffset, unsigned ListEntryNextPointerOffset, void **ListHead, void *ListEntryToAdd);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BSC_AddGenericListEntry_Actual_t)(BSC_Generic_List_Entry_Key_t GenericListEntryKey, unsigned int ListEntryKeyOffset, unsigned ListEntryNextPointerOffset, void **ListHead, void *ListEntryToAdd);
#endif

   /* The following function is a utility function that adds the        */
   /* specified List Entry to the specified Generic List Entry List.    */
   /* This function accepts as the first parameter, the size of the List*/
   /* Element to allocate to add to the list (this allows extra space to*/
   /* be allocated at the end of the List Element in case it is needed).*/
   /* The second parameter to this function specifies the Key Type that */
   /* is to be used to determine if a duplicate entry already exists in */
   /* the list (used in conjunction with the third parameter).  The     */
   /* third parameter specifies the byte offset of the actual Key itself*/
   /* in the Entry structure (note that this parameter is ignored if    */
   /* ekNone is specified as the List Entry Key Type - this means that  */
   /* the entry is to be added to the list without checking for         */
   /* duplicates).  The fourth parameter specifies the size of the List */
   /* Entry itself (the size of the buffer pointed to by the final      */
   /* parameter).  The fifth parameter specifies the actual byte offset */
   /* of the next element pointer that is present in the List Entry     */
   /* structure.  The sixth parameter specifies a pointer to the Head of*/
   /* the Generic List itself (a pointer to a pointer to the first      */
   /* element present in the list).  The final parameter specifies the  */
   /* actual List Entry value to add (it MUST be a pointer to data of   */
   /* the size specified by the fifth parameter).  This function returns*/
   /* a pointer to the element that was added to the list (if           */
   /* successful) or NULL if the specified entry could not be added to  */
   /* the list.                                                         */
   /* * NOTE * Valid values must be specified for the following         */
   /*          parameters (or the function will fail):                  */
   /*             - ListEntrySizeToAllocate - cannot be zero and MUST   */
   /*               be greater than or equal to the ListEntrySize       */
   /*               parameter.                                          */
   /*             - ListEntrySize - cannot be zero and MUST be less     */
   /*               than or equal to the ListEntrySizeToAllocate        */
   /*               parameter.                                          */
   /*             - ListHead - parameter cannot be NULL, but the value  */
   /*               that it points to can be NULL.                      */
   /*             - ListEntryToAdd - parameter cannot be NULL, and it   */
   /*               must point to the List Entry Data that is to be     */
   /*               added (of size ListEntrySize).                      */
   /* * NOTE * If the GenericListEntryKey value is anything other than  */
   /*          ekNone, then this function does not insert duplicate     */
   /*          entries into the list.  An item is considered a duplicate*/
   /*          if the Key value of the entry being added matches any    */
   /*          Key value of an entry already in the list.  When this    */
   /*          parameter is ANYTHING OTHER THAN ekNone, then the        */
   /*          following parameters must be specified:                  */
   /*             - ListEntryKeyOffset - specifies the byte offset of   */
   /*               the Generic List Entry Key Element (in each         */
   /*               individual List Entry).                             */
   /* * NOTE * In all cases, the ListEntryNextPointerOffset parameter   */
   /*          MUST specify the byte offset of the element that         */
   /*          represents a pointer to the next element that is present */
   /*          in the list (for each individual List Entry).            */
BTPSAPI_DECLARATION void *BTPSAPI BSC_AddGenericListEntry(unsigned int ListEntrySizeToAllocate, BSC_Generic_List_Entry_Key_t GenericListEntryKey, unsigned int ListEntryKeyOffset, unsigned int ListEntrySize, unsigned ListEntryNextPointerOffset, void **ListHead, void *ListEntryToAdd);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void *(BTPSAPI *PFN_BSC_AddGenericListEntry_t)(unsigned int ListEntrySizeToAllocate, BSC_Generic_List_Entry_Key_t GenericListEntryKey, unsigned int ListEntryKeyOffset, unsigned int ListEntrySize, unsigned ListEntryNextPointerOffset, void **ListHead, void *ListEntryToAdd);
#endif

   /* The following function is a utility function that exists to search*/
   /* for an individual Generic List Entry that is present in the       */
   /* specified Generic List Entry List.  The first parameter to this   */
   /* function specifies the List Key type to use when searching (it    */
   /* cannot be ekNone).  The second parameter specifies a pointer to   */
   /* the List Key value that is to be used for the comparison.  The    */
   /* third parameter specifies the byte offset of the actual List Key  */
   /* element that is present in each individual the List Entry Element.*/
   /* The fourth parameter specifies the byte offset of the actual      */
   /* pointer to the next element that is present in the Generic List   */
   /* Entry List.  The final parameter specifies a pointer to a pointer */
   /* to the first element present in the list.  This function returns  */
   /* a pointer to the specified List Entry Element (if it was found    */
   /* in the specified list) or NULL if the element could not be found  */
   /* in the list.                                                      */
BTPSAPI_DECLARATION void *BTPSAPI BSC_SearchGenericListEntry(BSC_Generic_List_Entry_Key_t GenericListEntryKey, void *GenericListEntryKeyValue, unsigned int ListEntryKeyOffset, unsigned ListEntryNextPointerOffset, void **ListHead);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void *(BTPSAPI *PFN_BSC_SearchGenericListEntry_t)(BSC_Generic_List_Entry_Key_t GenericListEntryKey, void *GenericListEntryKeyValue, unsigned int ListEntryKeyOffset, unsigned ListEntryNextPointerOffset, void **ListHead);
#endif

   /* The following function is a utility function that exists to search*/
   /* the list for the next Generic List Entry.  The first parameter to */
   /* this function specifies the List Key type to use when searching   */
   /* (it must be ekEntryPointer).  The second parameter specifies a    */
   /* pointer to the List Key value that is to be used for the          */
   /* comparison.  The third parameter specifies the byte offset of the */
   /* actual pointer to the next element that is present in the Generic */
   /* List Entry List.  The final parameter specifies a pointer to a    */
   /* pointer to the first element present in the list.  This function  */
   /* returns a pointer to the specified List Entry Element (if it was  */
   /* found in the specified list) or NULL if the element could not be  */
   /* found in the list.                                                */
BTPSAPI_DECLARATION void *BTPSAPI BSC_GetNextGenericListEntry(BSC_Generic_List_Entry_Key_t GenericListEntryKey, void *GenericListEntryKeyValue, unsigned int ListEntryNextPointerOffset, void **ListHead);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void *(BTPSAPI *PFN_BSC_GetNextGenericListEntry_t)(BSC_Generic_List_Entry_Key_t GenericListEntryKey, void *GenericListEntryKeyValue, unsigned int ListEntryNextPointerOffset, void **ListHead);
#endif

   /* The following function is a utility function that exists to search*/
   /* for an individual Generic List Entry that is present in the       */
   /* specified Generic List Entry List and remove it from the specified*/
   /* list.  The first parameter to this function specifies the List Key*/
   /* type to use when searching (it cannot be ekNone).  The second     */
   /* parameter specifies a pointer to the List Key value that is to be */
   /* used for the comparison.  The third parameter specifies the byte  */
   /* offset of the actual List Key element that is present in each     */
   /* individual the List Entry Element.  The fourth parameter specifies*/
   /* the byte offset of the actual pointer to the next element that is */
   /* present in the Generic List Entry List.  The final parameter      */
   /* specifies a pointer to a pointer to the first element present in  */
   /* the list.  This function returns a pointer to the specified List  */
   /* Entry Element (if it was found and removed from the specified     */
   /* list) or NULL if the element could not be found in the list.      */
   /* * NOTE * This function does not free the resources of the element */
   /*          that was deleted from the List, it only removes it from  */
   /*          the list and returns a pointer to the element.  The      */
   /*          Next Pointer element of the returned element will have   */
   /*          it's value set to NULL.                                  */
   /* * NOTE * It is the callers responsibility to free the memory that */
   /*          is occuppied by the specified list (when finished) by    */
   /*          calling the BSC_FreeGenericListEntryMemory() function.   */
BTPSAPI_DECLARATION void *BTPSAPI BSC_DeleteGenericListEntry(BSC_Generic_List_Entry_Key_t GenericListEntryKey, void *GenericListEntryKeyValue, unsigned int ListEntryKeyOffset, unsigned int ListEntryNextPointerOffset, void **ListHead);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void *(BTPSAPI *PFN_BSC_DeleteGenericListEntry_t)(BSC_Generic_List_Entry_Key_t GenericListEntryKey, void *GenericListEntryKeyValue, unsigned int ListEntryKeyOffset, unsigned int ListEntryNextPointerOffset, void **ListHead);
#endif

   /* This function frees the specified Generic List Entry ember.  No   */
   /* check is done on this entry other than making sure it NOT NULL.   */
BTPSAPI_DECLARATION void BTPSAPI BSC_FreeGenericListEntryMemory(void *EntryToFree);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_FreeGenericListEntryMemory_t)(void *EntryToFree);
#endif

   /* The following function deletes (and free's all memory) every      */
   /* element of the specified Generic List Entry List.  Upon return of */
   /* this function, the Head Pointer is set to NULL.                   */
BTPSAPI_DECLARATION void BTPSAPI BSC_FreeGenericListEntryList(void **ListHead, unsigned int ListEntryNextPointerOffset);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BSC_FreeGenericListEntryList_t)(void **ListHead, unsigned int ListEntryNextPointerOffset);
#endif

#endif
