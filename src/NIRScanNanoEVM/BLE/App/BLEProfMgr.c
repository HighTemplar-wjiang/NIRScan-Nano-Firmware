/*
 *
 * BLE Profile Manager- Manages the custom GATT profile, handlers for
 * BLE profiles such as battery, device information etc
 * Also handles connection related activities
 *
 * Copyright (C) 2014-15 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>               /* Included for sscanf.                     */
#include "common.h"

#ifdef NIRSCAN_INCLUDE_BLE
#include "uartstdio.h"
#include <xdc/cfg/global.h>
#include <xdc/runtime/System.h>
#include "SS1BTPS.h"             /* Main SS1 BT Stack Header.                */
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                    */
#include "SS1BTGAP.h"            /* Main SS1 GAP Service Header.             */
#include "SS1BTDIS.h"
#include "DIS.h"
#include "SS1BTBAS.h"
#include "BAS.h"
#include "BTPSKRNL.h"            /* BTPS Kernel Header.                      */
#include "BLEMain.h"            /* Application Interface Abstraction.        */
#include "led.h"
#include "version.h"
#include "BLECommonDefs.h"
#include "BLEGATTStdSvcs.h"
#include "BLEGATTCmdSvc.h"
#include "BLEGATTGISvc.h"
#include "BLEGATTTimeSvc.h"
#include "BLEGATTCalSvc.h"
#include "BLEGATTConfSvc.h"
#include "BLEGATTConfSvcUtilFunc.h"
#include "dlpspec_scan.h"
#include "BLEGATTScanSvc.h"
#include "BLECmdHandlerLiaison.h"
#include "BLENotificationHandler.h"
#include "BLEUtils.h"
#include "nano_eeprom.h"
#include "dlpspec_version.h"
#include "nnoStatus.h"
#include "nano_timer.h"
#include "BLEProfMgr.h"              /* Application Header.                  */
#include "bleCmdHandler.h"

/**
 * @brief LE IO capability
 *
 *   Denotes the default I/O capability that is used with LE pairing
 */
#define DEFAULT_LE_IO_CAPABILITY   (licNoInputNoOutput)

/**
 * @brief LE MITM protection
 *
 * Denotes the default value used for Man in the Middle (MITM) protection
 * used with LE Pairing
 */
#define DEFAULT_LE_MITM_PROTECTION              (TRUE)

/**
 * @brief Default IO capability
 *
 * Denotes the default I/O capability that is used with Secure Simple Pairing
 */
#define DEFAULT_IO_CAPABILITY          (icDisplayYesNo)

/**
 * @brief Service UUID used in Nano advertisement packet (Scan service UUID)
 */
const Byte_t AVERTISEMENT_PACKET_NIRSCAN_UUID[16] = { 0x6F, 0x6E, 0x61, 0x4E, 0x20, 0x52, 0x49, 0x4E, 0x20, 0x50, 0x4C, 0x44, 0x06, 0x52, 0x45, 0x53 };

/**
 * @brief Device name used in advertisement packer
 */
#define LE_DEMO_DEVICE_NAME                        "NIRScanNano"

/**
 * @brief GAP Low Energy Parameters
 *
 * Structure with GAP configuration paramaters
 */
typedef struct _tagGAPLE_Parameters_t
{
   GAP_LE_Connectability_Mode_t ConnectableMode;
   GAP_Discoverability_Mode_t   DiscoverabilityMode;
   GAP_LE_IO_Capability_t       IOCapability;
   Boolean_t                    MITMProtection;
   Boolean_t                    OOBDataPresent;
} GAPLE_Parameters_t;


/**
 * @brief Device information structure
 *
 * The following structure is used to hold a list of information
 * on all paired devices
 */
typedef struct _tagDeviceInfo_t
{
   Byte_t                   Flags;
   Byte_t                   EncryptionKeySize;
   GAP_LE_Address_Type_t    ConnectionAddressType;
   BD_ADDR_t                ConnectionBD_ADDR;
   Long_Term_Key_t          LTK;
   Random_Number_t          Rand;
   Word_t                   EDIV;
   BAS_Server_Information_t BASServerInformation;
   struct _tagDeviceInfo_t *NextDeviceInfoInfoPtr;
} DeviceInfo_t;

/**
 * @brief size of device information structure
 */
#define DEVICE_INFO_DATA_SIZE	(sizeof(DeviceInfo_t))

/**
 * @brief Defines the bit mask flags that may be set in the DeviceInfo_t structure
 */
#define DEVICE_INFO_FLAGS_LTK_VALID	0x01

/**
 * @name Bit mask flags used in DeviceInfo_t structure
 */
//@{
/** Denotes BLE server */
#define DEVICE_INFO_FLAGS_BLE_SERVER	0x02
/** Denotes that service discovery is still not complete */
#define DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING	0x04
/** Denotes Flags are encrypted */
#define DEVICE_INFO_FLAGS_LINK_ENCRYPTED	0x08
//@}

/**
 * @brief Structure to hold a BD_ADDR return from BD_ADDRToStr
 */
typedef char BoardStr_t[16];

/**
 * @brief Structure used to hold application state realted information
 */
typedef struct _tagApplicationStateInfo_t
{
   unsigned int         BluetoothStackID;
   Byte_t               Flags;
   unsigned int         GAPSInstanceID;
} ApplicationStateInfo_t;

/**
 * @brief Container for all of the Application State Information
 */
static ApplicationStateInfo_t ApplicationStateInfo;

/**
 * @brief Holds instance ID for GAP service
 */
static unsigned int        GAPSInstanceID;

/**
 * @brief Holds GAP Parameters like Discoverability, Connectability Modes
 */
static GAPLE_Parameters_t  LE_Parameters;

/**
 * @brief Holds the list head for the device info list
 */
static DeviceInfo_t       *DeviceInfoList;

/**
 * @brief Boolean to tell if local device is master of current connection
 */
static Boolean_t LocalDeviceIsMaster;

/**
 * @brief Contains the connection ID and BD_ADDR of each connected device
 */
LE_Context_Info_t   LEContextInfo[MAX_LE_CONNECTIONS];

#if defined(DEBUG_MSGS) && (LOG_PORT != UART_CONSOLE)
/**
 * @brief The string table is used to map HCI Version information
 */
static BTPSCONST char *HCIVersionStrings[] =
{
   "1.0b",
   "1.1",
   "1.2",
   "2.0",
   "2.1",
   "3.0",
   "4.0",
   "4.1",
   "Unknown (greater 4.1)"
} ;

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)
#endif

/**
 * @brief Global variable that stores the current BLE connection status
 */
uint32_t g_BLEConnStatus = BLE_CONN_STATUS_INACTIVE;

/**
 * @brief Function to create new device info entry to be stored into array
 *
 */
static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR);
/**
 * @brief Function to lookup device info entry using device address
 *
 */
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);

/**
 * @brief Function to delete device info entry stored into array
 *
 */
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);

/**
 * @brief Function to free device info entry memory
 *
 */
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree);

/**
 * @brief Function to free device info entry array
 *
 */
static void FreeDeviceInfoList(DeviceInfo_t **ListHead);

/**
 * @brief Function to convert device address to string
 *
 */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);

/**
 * @brief Function to display advertisement data
 *
 */
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data);

/**
 * @brief Function to open BLE stack
 *
 */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization);

/**
 * @brief Function to set if device is discoverable or not
 *
 */
static int SetDisc(void);

/**
 * @brief Function to set if device is connectable or not
 *
 */
static int SetConnect(void);

/**
 * @brief Function to set if device is pairable or not
 *
 */
static int SetPairable(void);

/**
 * @brief Function to find first free LE index in the array
 *
 */
static int FindFreeLEIndex(void);

/**
 * @brief Function to udpate connection ID in all GATT services
 *
 */
static int UpdateConnectionID(unsigned int ConnectionID, BD_ADDR_t BD_ADDR);

/**
 * @brief Function to remove connection information on disconnection
 *
 */
static void RemoveConnectionInfo(BD_ADDR_t BD_ADDR);

/* BTPS Callback function prototypes.                                */
/**
 * @brief Callback function used to notify app on GAP LE events
 *
 */
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID,GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);

/**
 * @brief Callback function used to notify app on GATT connection events
 *
 */
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);

/**
 * @brief Callback function used to notify app on GAP events
 *
 */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);

/**
 * @brief Callback function used to notify app on HCI events
 *
 */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter);

void InitBLEApplicationStateInfo()
/**
 * Initializes BLE application state information for all GATT services
 *
 * @return      None
 *
 */
{
	ApplicationStateInfo.BluetoothStackID = 0;
	ApplicationStateInfo.Flags = 0;
	ApplicationStateInfo.GAPSInstanceID = 0;

	GATTStdSvcs_InitApplicationStateInfo();
	GATTCmdSvc_InitApplicationStateInfo();
	GATTGISvc_InitApplicationStateInfo();
	GATTTimeSvc_InitApplicationStateInfo();
	GATTCalSvc_InitApplicationStateInfo();
	GATTScanSvc_InitApplicationStateInfo();
	GATTConfSvc_InitApplicationStateInfo();

	return;
}

static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR)
/**
 * To create new device information entry
 *
 * @param[in]   ListHead  				Pointer to device information list head
 * @param[out]  ConnectionAddressType   Address type - public/random
 * @param[in]   ConnectionBD_ADDR		Bluetooth address of connected device
 *
 * @return      SUCCESS/FAILURE (1/0)
 *
 * The following function adds the specified Entry to the specified List.
 * This function allocates and adds an entry to the list that has the same
 * attributes as parameters to this function.  This function will return
 * FALSE if NO Entry was added.  This can occur if the element passed in
 * was deemed invalid or the actual List Head was invalid
 * ** NOTE ** This function does not insert duplicate entries into
 * 			  the list.  An element is considered a duplicate if the
 *			  Connection BD_ADDR.  When this occurs, this function
 *			  returns NULL
 */
{
   Boolean_t     ret_val = FALSE;
   DeviceInfo_t *DeviceInfoPtr;

   /* Verify that the passed in parameters seem semi-valid.             */
   if((ListHead) && (!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR)))
   {
      /* Allocate the memory for the entry.                             */
      if((DeviceInfoPtr = BTPS_AllocateMemory(sizeof(DeviceInfo_t))) != NULL)
      {
         /* Initialize the entry.                                       */
         BTPS_MemInitialize(DeviceInfoPtr, 0, sizeof(DeviceInfo_t));
         DeviceInfoPtr->ConnectionAddressType = ConnectionAddressType;
         DeviceInfoPtr->ConnectionBD_ADDR     = ConnectionBD_ADDR;

         ret_val = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead), (void *)(DeviceInfoPtr));
         if(!ret_val)
         {
            /* Failed to add to list so we should free the memory that  */
            /* we allocated for the entry.                              */
            BTPS_FreeMemory(DeviceInfoPtr);
         }
      }
   }

   return(ret_val);
}

static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR)
/**
 * To lookup device information entry using Bluetooth device address
 *
 * @param[in]   ListHead  			Pointer to device information list head
 * @param[in]   ConnectionBD_ADDR	Bluetooth address of connected device
 *
 * @return      Pointer to the entry matching input criteria
 *
 * The following function searches the specified List for the
 * specified Connection BD_ADDR.  This function returns NULL if
 * either the List Head is invalid, the BD_ADDR is invalid, or the
 * Connection BD_ADDR was NOT found
 */
{
   return(BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead)));
}

static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR)
/**
 * To delete device information entry based on device address
 *
 * @param[in]   ListHead  			Pointer to device information list head
 * @param[in]   ConnectionBD_ADDR	Bluetooth address of connected device
 *
 * @return      Pointer to the entry next to deleted entry
 *
 * The following function searches the specified Key Info List for
 * the specified BD_ADDR and removes it from the List.  This function
 * returns NULL if either the List Head is invalid, the BD_ADDR is
 * invalid, or the specified Entry was NOT present in the list.  The
 * entry returned will have the Next Entry field set to NULL, and
 * the caller is responsible for deleting the memory associated with
 * this entry by calling the FreeKeyEntryMemory() function.
 */
{
   return(BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead)));
}

static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree)
/**
 * To free device information entry memory
 *
 * @param[in]   EntryToFree	Pointer to device information entry to be freed
 *
 * @return      None
 *
 */
{
   BSC_FreeGenericListEntryMemory((void *)(EntryToFree));
}

static void FreeDeviceInfoList(DeviceInfo_t **ListHead)
/**
 * To delete device information entry based on device address
 *
 * @param[in]   ListHead  			Pointer to device information list head
 *
 * @return      None
 *
 * The following function deletes (and free's all memory) every element
 * of the specified Key Info List. Upon return of this function,
 * the Head Pointer is set to NULL
 */
{
   BSC_FreeGenericListEntryList((void **)(ListHead), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr));
}


static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr)
/**
 * To convert device address to string
 *
 * @param[in]   Board_Address	BD_ADDR to be converted to a string
 * @param[in]   BoardStr		pointer to store converted BD_ADDR
 *
 * @return      None
 *
 */
{
   BTPS_SprintF((char *)BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5, Board_Address.BD_ADDR4, Board_Address.BD_ADDR3, Board_Address.BD_ADDR2, Board_Address.BD_ADDR1, Board_Address.BD_ADDR0);
}

static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data)
/**
 * Utility function to display advertising data
 *
 * @param[in]   Advertising_Data	Data to be displayed
 *
 * @return      None
 *
 */
{
   unsigned int Index;
   unsigned int Index2;

   /* Verify that the input parameters seem semi-valid.                 */
   if(Advertising_Data)
   {
      for(Index = 0; Index < Advertising_Data->Number_Data_Entries; Index++)
      {

         if(Advertising_Data->Data_Entries[Index].AD_Data_Buffer)
         {
        	 DEBUG_PRINT("  AD Data: ");
            for(Index2 = 0; Index2 < Advertising_Data->Data_Entries[Index].AD_Data_Length; Index2++)
            {
            	DEBUG_PRINT("0x%02X ", Advertising_Data->Data_Entries[Index].AD_Data_Buffer[Index2]);
            }
            DEBUG_PRINT("\r\n");
         }
      }
   }
}

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
/**
 * Function responsible for opening the SS1 Bluetooth Protocol Stack
 *
 * @param[in]   HCI_DriverInformation	Pointer to contains the HCI Driver Transport Information
 * @param[in]   BTPS_Initialization		Init parameters for BTPS
 *
 * @return      Success= 0, Fail = <0 (Bluetopia error codes)
 */
{
   int                           i;
   int                           Result;
   int                           ret_val = 0;
   char                          BluetoothAddress[16];
   Byte_t                        Status;
   Byte_t                        NumberLEPackets;
   Word_t                        LEPacketLength;
   BD_ADDR_t                     BD_ADDR;
   unsigned int                  ServiceID;
   HCI_Version_t                 HCIVersion;
   L2CA_Link_Connect_Params_t    L2CA_Link_Connect_Params;

   /* First check to see if the Stack has already been opened.          */
   if(!ApplicationStateInfo.BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if(HCI_DriverInformation)
      {
         /* Initialize BTPSKNRl.                                        */
         BTPS_Init((void *)BTPS_Initialization);

         DEBUG_PRINT("OpenStack().\r\n");

         /* Initialize the Stack                                        */
         Result = BSC_Initialize(HCI_DriverInformation, 0);

         /* Next, check the return value of the initialization to see   */
         /* if it was successful.                                       */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            ApplicationStateInfo.BluetoothStackID = Result;
            DEBUG_PRINT("Bluetooth Stack ID: %d.\r\n", ApplicationStateInfo.BluetoothStackID);

            /* Initialize the Default Pairing Parameters.               */
            LE_Parameters.IOCapability   = DEFAULT_LE_IO_CAPABILITY;
            LE_Parameters.MITMProtection = DEFAULT_LE_MITM_PROTECTION;
            LE_Parameters.OOBDataPresent = FALSE;

            Result = HCI_Version_Supported(ApplicationStateInfo.BluetoothStackID, &HCIVersion);

#if defined(DEBUG_MSGS) && (LOG_PORT != UART_CONSOLE)
            if(Result < 0)
            	DEBUG_PRINT("Device Chipset: %s.\r\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]);
#endif

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(ApplicationStateInfo.BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               DEBUG_PRINT("BD_ADDR: %s\r\n", BluetoothAddress);
            }

            if(HCI_Command_Supported(ApplicationStateInfo.BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(ApplicationStateInfo.BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(ApplicationStateInfo.BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(ApplicationStateInfo.BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(ApplicationStateInfo.BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            //DeleteLinkKey(BD_ADDR);

            /* Flag that no connection is currently active.             */
            LocalDeviceIsMaster = FALSE;

            for(i=0; i<MAX_LE_CONNECTIONS; i++)
            {
               LEContextInfo[i].ConnectionID = 0;
               ASSIGN_BD_ADDR(LEContextInfo[i].ConnectionBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            }

            /* Flag that we have no Key Information in the Key List.    */
            DeviceInfoList = NULL;

            /* Initialize the GATT Service.                             */
            if((Result = GATT_Initialize(ApplicationStateInfo.BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0)) == 0)
            {
               /* Determine the number of LE packets that the controller*/
               /* will accept at a time.                                */
               if((!HCI_LE_Read_Buffer_Size(ApplicationStateInfo.BluetoothStackID, &Status, &LEPacketLength, &NumberLEPackets)) && (!Status) && (LEPacketLength))
               {
                  NumberLEPackets = (NumberLEPackets/MAX_LE_CONNECTIONS);
                  NumberLEPackets = (NumberLEPackets == 0)?1:NumberLEPackets;
               }
               else
                  NumberLEPackets = 1;

               /* Set a limit on the number of packets that we will     */
               /* queue internally.                                     */
               GATT_Set_Queuing_Parameters(ApplicationStateInfo.BluetoothStackID, (unsigned int)NumberLEPackets, (unsigned int)(NumberLEPackets-1), FALSE);

               /* Initialize the GAPS Service.                          */
               Result = GAPS_Initialize_Service(ApplicationStateInfo.BluetoothStackID, &ServiceID);
               if(Result > 0)
               {
                  /* Save the Instance ID of the GAP Service.           */
                  GAPSInstanceID = (unsigned int)Result;

                  /* Set the GAP Device Name and Device Appearance.     */
                  GAPS_Set_Device_Name(ApplicationStateInfo.BluetoothStackID, GAPSInstanceID, LE_DEMO_DEVICE_NAME);
                  GAPS_Set_Device_Appearance(ApplicationStateInfo.BluetoothStackID, GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER);

                  /* Return success to the caller.                      */
                  ret_val        = 0;
               }
               else
               {
                  /* The Stack was NOT initialized successfully, inform */
                  /* the user and set the return value of the           */
                  /* initialization function to an error.               */
                  bleLogFuncError("GAPS_Initialize_Service", Result);
                  nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);

                  /* Cleanup GATT Module.                               */
                  GATT_Cleanup(ApplicationStateInfo.BluetoothStackID);

                  ApplicationStateInfo.BluetoothStackID = 0;

                  ret_val          = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;
               }
            }
            else
            {
               /* The Stack was NOT initialized successfully, inform the*/
               /* user and set the return value of the initialization   */
               /* function to an error.                                 */
               bleLogFuncError("GATT_Initialize", Result);
               nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);
               ApplicationStateInfo.BluetoothStackID = 0;

               ret_val          = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;
            }
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
        	nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);

        	bleLogFuncError("BSC_Initialize", Result);

            ApplicationStateInfo.BluetoothStackID = 0;

            ret_val          = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;
         }
      }
      else
      {
         /* One or more of the necessary parameters are invalid.        */
         ret_val = APPLICATION_ERROR_INVALID_PARAMETERS;
      }
   }

   return(ret_val);
}

int CloseStack(void)
/**
 * Function is responsible for closing the SS1 Bluetooth Protocol Stack
 *
 * @return      Success = 0, Failure <0
 *
 * This function requires that the Bluetooth Protocol stack previously
 * have been initialized via the OpenStack() function
 */
{
   int ret_val = 0;

   /* First check to see if the Stack has been opened.                  */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Cleanup GAP Service Module.                                    */
      if(GAPSInstanceID)
    	  ret_val = GAPS_Cleanup_Service(ApplicationStateInfo.BluetoothStackID, GAPSInstanceID);
      else
      {
    	  DEBUG_PRINT("\r\nError: Invalid GAPS Instance ID\r\n");
    	  return (FAIL);
      }
      if (ret_val < 0)
      {
    	  DEBUG_PRINT("\r\nError: GAPS clean up failed, errorcode:%d\r\n",ret_val);
    	  return (ret_val);
      }

      // Unregister custom GATT services
      ret_val = GATTStdSvcs_Unregister(ApplicationStateInfo.BluetoothStackID);
      if (ret_val < 0)
      {
    	  DEBUG_PRINT("\r\nError: Closing standard services failed, error code:%d\r\n",ret_val);
    	  return (ret_val);
      }

      GATTCalSvc_Unregister(ApplicationStateInfo.BluetoothStackID);
      GATTCmdSvc_Unregister(ApplicationStateInfo.BluetoothStackID);
      GATTConfSvc_Unregister(ApplicationStateInfo.BluetoothStackID);
      GATTGISvc_Unregister(ApplicationStateInfo.BluetoothStackID);
      GATTScanSvc_Unregister(ApplicationStateInfo.BluetoothStackID);
      GATTTimeSvc_Unregister(ApplicationStateInfo.BluetoothStackID);

      /* Cleanup GATT Module.                                           */
      ret_val = GATT_Cleanup(ApplicationStateInfo.BluetoothStackID);
      if (ret_val < 0)
      {
    	  DEBUG_PRINT("\r\nError: GATT clean up failed, errorcode:%d\r\n",ret_val);
    	  return (ret_val);
      }

      /* Simply close the Stack                                         */
      BSC_Shutdown(ApplicationStateInfo.BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      DEBUG_PRINT("\r\nBLE Stack Shutdown\r\n");

      /* Free the Key List.                                             */
      FreeDeviceInfoList(&DeviceInfoList);

      /* Flag that the Stack is no longer initialized.                  */
      ApplicationStateInfo.BluetoothStackID = 0;

      /* Flag success to the caller.                                    */
      ret_val          = 0;
   }
   else
   {
      /* A valid Stack ID does not exist, inform to user.               */
      ret_val = APPLICATION_ERROR_INVALID_STACK_ID;
   }

   return(ret_val);
}

static int SetDisc(void)
/**
 * Responsible for placing the Local Bluetooth Device into General
 * Discoverablity Mode
 *
 * @param[in]   None
 *
 * @return      Success=0, Failure <0
 *
 * This function requires that a valid Bluetooth Stack ID exists
 * before running
 */
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* A semi-valid Bluetooth Stack ID exists, now attempt to set the */
      /* attached Devices Discoverablity Mode to General.               */
      ret_val = GAP_Set_Discoverability_Mode(ApplicationStateInfo.BluetoothStackID, dmGeneralDiscoverableMode, 0);

      /* Next, check the return value of the GAP Set Discoverability    */
      /* Mode command for successful execution.                         */
      if(!ret_val)
      {
         /* * NOTE * Discoverability is only applicable when we are     */
         /*          advertising so save the default Discoverability    */
         /*          Mode for later.                                    */
         LE_Parameters.DiscoverabilityMode = dmGeneralDiscoverableMode;
      }
      else
      {
         /* An error occurred while trying to set the Discoverability   */
         /* Mode of the Device.                                         */
    	 nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);

         bleLogFuncError("Set Discoverable Mode", ret_val);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = APPLICATION_ERROR_INVALID_STACK_ID;
   }

   return(ret_val);
}

static int SetConnect(void)
/**
 * Function is responsible for placing the Local Bluetooth Device
 * into Connectable Mode
 *
 * @param[in]   None
 *
 * @return      Success=0, Failure <0
 *
 */
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Attempt to set the attached Device to be Connectable.          */
      ret_val = GAP_Set_Connectability_Mode(ApplicationStateInfo.BluetoothStackID, cmConnectableMode);

      /* Next, check the return value of the                            */
      /* GAP_Set_Connectability_Mode() function for successful          */
      /* execution.                                                     */
      if(!ret_val)
      {
         /* * NOTE * Connectability is only an applicable when          */
         /*          advertising so we will just save the default       */
         /*          connectability for the next time we enable         */
         /*          advertising.                                       */
         LE_Parameters.ConnectableMode = lcmConnectable;
      }
      else
      {
         /* An error occurred while trying to make the Device           */
         /* Connectable.                                                */
    	 nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
         bleLogFuncError("Set Connectability Mode", ret_val);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = APPLICATION_ERROR_INVALID_STACK_ID;
   }

   return(ret_val);
}

static int SetPairable(void)
/**
 * Function is responsible for placing the local Bluetooth device
 * into Pairable mode
 *
 * @param[in]   None
 *
 * @return      Success=0, Failure <0
 *
 */
{
   int Result;
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(ApplicationStateInfo.BluetoothStackID)
   {
      /* Attempt to set the attached device to be pairable.             */
      Result = GAP_Set_Pairability_Mode(ApplicationStateInfo.BluetoothStackID, pmPairableMode);

      /* Next, check the return value of the GAP Set Pairability mode   */
      /* command for successful execution.                              */
      if(!Result)
      {
         /* The device has been set to pairable mode, now register an   */
         /* Authentication Callback to handle the Authentication events */
         /* if required.                                                */
         Result = GAP_Register_Remote_Authentication(ApplicationStateInfo.BluetoothStackID, GAP_Event_Callback, (unsigned long)0);

         /* Next, check the return value of the GAP Register Remote     */
         /* Authentication command for successful execution.            */
         if(!Result)
         {
            /* Now Set the LE Pairability.                              */

            /* Attempt to set the attached device to be pairable.       */
            Result = GAP_LE_Set_Pairability_Mode(ApplicationStateInfo.BluetoothStackID, lpmPairableMode);

            /* Next, check the return value of the GAP Set Pairability  */
            /* mode command for successful execution.                   */
            if(!Result)
            {
               /* The device has been set to pairable mode, now register*/
               /* an Authentication Callback to handle the              */
               /* Authentication events if required.                    */
               Result = GAP_LE_Register_Remote_Authentication(ApplicationStateInfo.BluetoothStackID, GAP_LE_Event_Callback, (unsigned long)0);

               /* Next, check the return value of the GAP Register      */
               /* Remote Authentication command for successful          */
               /* execution.                                            */
               if(Result)
               {
                  /* An error occurred while trying to execute this     */
                  /* function.                                          */
            	  nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);
                  bleLogFuncError("GAP_LE_Register_Remote_Authentication", Result);

                  ret_val = Result;
               }
            }
            else
            {
               /* An error occurred while trying to make the device     */
               /* pairable.                                             */
               nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);
               bleLogFuncError("GAP_LE_Set_Pairability_Mode", Result);

               ret_val = Result;
            }
         }
         else
         {
            /* An error occurred while trying to execute this function. */
        	nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);
            bleLogFuncError("GAP_Register_Remote_Authentication", Result);

            ret_val = Result;
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
    	 nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, Result);
         bleLogFuncError("GAP_Set_Pairability_Mode", Result);

         ret_val = Result;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = APPLICATION_ERROR_INVALID_STACK_ID;
   }

   return(ret_val);
}

int AdvertiseLE(Boolean_t enable)
/**
 * Function is responsible for enabling LE advertisements
 *
 * @param[in]   enable	Advertisement enable/disable
 *
 * @return      Success=0, Failure <0
 *
 */
{
   int                                 ret_val;
   int                                 Length = 0;
   int								   i;
   GAP_LE_Advertising_Parameters_t     AdvertisingParameters;
   GAP_LE_Connectability_Parameters_t  ConnectabilityParameters;
   union
   {
      Advertising_Data_t               AdvertisingData;
      Scan_Response_Data_t             ScanResponseData;
   } Advertisement_Data_Buffer;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(ApplicationStateInfo.BluetoothStackID)
   {
         /* Determine whether to enable or disable Advertising.         */
         if(enable == 0)
         {
            /* Disable Advertising.                                     */
            ret_val = GAP_LE_Advertising_Disable(ApplicationStateInfo.BluetoothStackID);
            if(!ret_val)
            {
            	DEBUG_PRINT("   GAP_LE_Advertising_Disable success.\r\n");
            }
            else
            {
            	nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
            	DEBUG_PRINT("   GAP_LE_Advertising_Disable returned %d.\r\n", ret_val);

               ret_val = APPLICATION_ERROR_GAPS;
            }
         }
         else
         {
            /* Enable Advertising.                                      */
            /* Set the Advertising Data.                                */
            BTPS_MemInitialize(&(Advertisement_Data_Buffer.AdvertisingData), 0, sizeof(Advertising_Data_t));

            /* Set the Flags A/D Field (1 byte type and 1 byte Flags.   */
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] = 2;
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = 0;

            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[3] = sizeof(AVERTISEMENT_PACKET_NIRSCAN_UUID)/sizeof(Byte_t) + 1;
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[4] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_128_BIT_SERVICE_UUID_COMPLETE;
            for (i=0; i < sizeof(AVERTISEMENT_PACKET_NIRSCAN_UUID)/sizeof(Byte_t);i++)
            {
            	Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[i+5] = AVERTISEMENT_PACKET_NIRSCAN_UUID[i];
            }

            Length = sizeof(AVERTISEMENT_PACKET_NIRSCAN_UUID)/sizeof(Byte_t) + 2 + 3;

            /* Configure the flags field based on the Discoverability   */
            /* Mode.                                                    */
            if(LE_Parameters.DiscoverabilityMode == dmGeneralDiscoverableMode)
               Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK |
			   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   HCI_LE_ADVERTISING_FLAGS_BR_EDR_NOT_SUPPORTED_FLAGS_BIT_MASK;
            else
            {
               if(LE_Parameters.DiscoverabilityMode == dmLimitedDiscoverableMode)
                  Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_LIMITED_DISCOVERABLE_MODE_FLAGS_BIT_MASK|
   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	  HCI_LE_ADVERTISING_FLAGS_BR_EDR_NOT_SUPPORTED_FLAGS_BIT_MASK;
            }

            /* Write thee advertising data to the chip.                 */
            ret_val = GAP_LE_Set_Advertising_Data(ApplicationStateInfo.BluetoothStackID, Length, &(Advertisement_Data_Buffer.AdvertisingData));
            if(!ret_val)
            {
               BTPS_MemInitialize(&(Advertisement_Data_Buffer.ScanResponseData), 0, sizeof(Scan_Response_Data_t));

               /* Set the Scan Response Data.                           */
               Length = BTPS_StringLength(LE_DEMO_DEVICE_NAME) + 1;	//Length for name type and name string

               if(Length < (ADVERTISING_DATA_MAXIMUM_SIZE - 2))
               {
                  Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE;
               }
               else
               {
                  Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED;
                  Length = (ADVERTISING_DATA_MAXIMUM_SIZE - 2);
               }

               Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[0] = (Byte_t)(Length);
               BTPS_MemCopy(&(Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2]),LE_DEMO_DEVICE_NAME,Length-1);

               if (Length < ((ADVERTISING_DATA_MAXIMUM_SIZE - 2) - 2))	// Find if there is room for service uuid type and atleast one byte of UUID
               {
            	   uint8_t avLen = (ADVERTISING_DATA_MAXIMUM_SIZE - 2) - (Length + 1);
            	   Length = sizeof(AVERTISEMENT_PACKET_NIRSCAN_UUID)/sizeof(Byte_t) + 1;	//Lenght of advertisement UUID and type info
            	   if (Length > avLen)
            	   {
            		   Length = avLen;
            		   Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2 + BTPS_StringLength(LE_DEMO_DEVICE_NAME)+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_128_BIT_SERVICE_UUID_PARTIAL;
            	   }
            	   else
            	   {
            		   Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2 + BTPS_StringLength(LE_DEMO_DEVICE_NAME)+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_128_BIT_SERVICE_UUID_COMPLETE;
            	   }
            	   Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2 + BTPS_StringLength(LE_DEMO_DEVICE_NAME)] = (Byte_t)(Length);
            	   //Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2 + BTPS_StringLength(LE_DEMO_DEVICE_NAME)+1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_128_BIT_SERVICE_UUID_PARTIAL;
            	   BTPS_MemCopy(&(Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2 + BTPS_StringLength(LE_DEMO_DEVICE_NAME)+2]),AVERTISEMENT_PACKET_NIRSCAN_UUID,Length-1);
               }
               ret_val = GAP_LE_Set_Scan_Response_Data(ApplicationStateInfo.BluetoothStackID, (Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[0] + 1), &(Advertisement_Data_Buffer.ScanResponseData));
               if(!ret_val)
               {
                  /* Set up the advertising parameters.                 */
                  AdvertisingParameters.Advertising_Channel_Map   = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
                  AdvertisingParameters.Scan_Request_Filter       = fpNoFilter;
                  AdvertisingParameters.Connect_Request_Filter    = fpNoFilter;
                  AdvertisingParameters.Advertising_Interval_Min  = 100;
                  AdvertisingParameters.Advertising_Interval_Max  = 200;

                  /* Configure the Connectability Parameters.           */
                  /* * NOTE * Since we do not ever put ourselves to be  */
                  /*          direct connectable then we will set the   */
                  /*          DirectAddress to all 0s.                  */
                  ConnectabilityParameters.Connectability_Mode   = LE_Parameters.ConnectableMode;
                  ConnectabilityParameters.Own_Address_Type      = latPublic;
                  ConnectabilityParameters.Direct_Address_Type   = latPublic;
                  ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

                  /* Now enable advertising.                            */
                  ret_val = GAP_LE_Advertising_Enable(ApplicationStateInfo.BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0);
                  if(!ret_val)
                  {
                	  DEBUG_PRINT("GAP_LE_Advertising_Enable success.\r\n");
                  }
                  else
                  {
                	  DEBUG_PRINT("GAP_LE_Advertising_Enable returned %d.\r\n", ret_val);
                	  nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
                	  ret_val = APPLICATION_ERROR_GAPS;
                  }
               }
               else
               {
            	   DEBUG_PRINT("GAP_LE_Set_Advertising_Data(dtScanResponse) returned %d.\r\n", ret_val);
            	   nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
            	   ret_val = APPLICATION_ERROR_GAPS;
               }

            }
            else
            {
            	DEBUG_PRINT("   GAP_LE_Set_Advertising_Data(dtAdvertising) returned %d.\r\n", ret_val);
            	nnoStatus_setErrorStatusAndCode(NNO_ERROR_BLE, true, ret_val);
            	ret_val = APPLICATION_ERROR_GAPS;
            }
         }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = APPLICATION_ERROR_INVALID_STACK_ID;
   }

   // Now that everything was succesful, change BLE connection status
   if (enable)
   {
	   nnoStatus_setDeviceStatus(NNO_STATUS_BLE_STACK_OPEN, true);
	   nnoStatus_setDeviceStatus(NNO_STATUS_ACTIVE_BLE_CONNECTION,false);
   }
   else
	   nnoStatus_setDeviceStatus(NNO_STATUS_BLE_STACK_OPEN, false);

   return(ret_val);
}

int RegisterCustomGATTServices()
/**
 * To register all custom services with GATT server
 *
 * @return      Success=0, Failure <0
 *
 */
{
	int ret_val = 0;
	ret_val = GATTCmdSvc_Register(ApplicationStateInfo.BluetoothStackID);
	if (!ret_val)	//else would be handled by the called function
	{
		ret_val = GATTGISvc_Register(ApplicationStateInfo.BluetoothStackID);
		if (!ret_val)	//else would be handled by the called function
		{
			ret_val = GATTTimeSvc_Register(ApplicationStateInfo.BluetoothStackID);
			if (!ret_val)
			{
				ret_val = GATTCalSvc_Register(ApplicationStateInfo.BluetoothStackID);
				if (!ret_val)
				{
					ret_val = GATTConfSvc_Register(ApplicationStateInfo.BluetoothStackID);
					if (!ret_val)
					{
						ret_val = GATTScanSvc_Register(ApplicationStateInfo.BluetoothStackID);
					}
				}
			}
		}
	}

	return ret_val;
}

int RegisterBLEServices()
/**
 * To register all custom services with GATT server
 *
 * @return      Success=0, Failure <0
 *
 */
{
   int	ret_val;

   /* Verify that there is no active connection.                        */
   if(FindFreeLEIndex() != -1)
   {
	   ret_val = GATTStdSvcs_Register(ApplicationStateInfo.BluetoothStackID);
	   if (0 == ret_val)
		   ret_val = RegisterCustomGATTServices();
   }
   else
   {
	   DEBUG_PRINT("Connection currently active.\r\n");
	   ret_val = APPLICATION_ERROR_FUNCTION;
   }

   return(ret_val);
}

static int FindFreeLEIndex(void)
/**
 * Function is responsible for iterating through the array
 * BDInfoArray[MAX_LE_CONNECTIONS]
 *
 * @param[in]   None
 *
 * @return      Free Index when found, -1 otherwise
 *
 */
{
   BD_ADDR_t NullBD_ADDR;

   ASSIGN_BD_ADDR(NullBD_ADDR, 0, 0, 0, 0, 0, 0);

   return(BLEUtil_FindLEIndexByAddress(NullBD_ADDR));
}

static int UpdateConnectionID(unsigned int ConnectionID, BD_ADDR_t BD_ADDR)
/**
 * Function is responsible for updating connection information
 *
 * @param[in]   ConnectionID	input connection ID
 * @param[in]	BD_ADDR			Device Bluetooth address
 *
 * @return      Returns entry index when found, -1 otherwise
 *
 */
{
   int LEConnectionIndex;

   /* Check for the index of the entry for this connection.             */
   LEConnectionIndex = BLEUtil_FindLEIndexByAddress(BD_ADDR);
   if(LEConnectionIndex >= 0)
   {
      LEContextInfo[LEConnectionIndex].ConnectionID = ConnectionID;
      // Update connection ID for other GATT services too
      if (GATTGISvc_UpdateConnectionID(ConnectionID) < 0)
      {
    	  DEBUG_PRINT("\r\nError in updating Connection ID for GI Service\r\n");
      }
      else if (GATTCmdSvc_UpdateConnectionID(ConnectionID) < 0)
      {
    	  DEBUG_PRINT("\r\nError in updating Connection ID for Command Service\r\n");
      }
      else if (GATTTimeSvc_UpdateConnectionID(ConnectionID) < 0)
      {
    	  DEBUG_PRINT("\r\nError in updating Connection ID for Time Service\r\n");
      }
      else if (GATTCalSvc_UpdateConnectionID(ConnectionID) < 0)
      {
    	  DEBUG_PRINT("\r\nError in updating Connection ID for Cal Service\r\n");
      }
      else if (GATTConfSvc_UpdateConnectionID(ConnectionID) < 0)
      {
    	  DEBUG_PRINT("\r\nError in updating Connection ID for Conf Service\r\n");
      }
      else if (GATTScanSvc_UpdateConnectionID(ConnectionID) < 0)
      {
    	  DEBUG_PRINT("\r\nError in updating Connection ID for Scan Service\r\n");
      }
   }
   else
   {
	   DEBUG_PRINT("Error in updating ConnectionID.\r\n");
   }

   return(LEConnectionIndex);
}

static void RemoveConnectionInfo(BD_ADDR_t BD_ADDR)
/**
 * Function is responsible for clearing the values of an entry in BDInfoArray
 *
 * @param[in]   BD_ADDR	Bluetooth address of connected device
 *
 * @return      None
 */
{
   int LEConnectionIndex;

   /* If an index is returned (any but -1), then found                  */
   LEConnectionIndex = BLEUtil_FindLEIndexByAddress(BD_ADDR);
   if(LEConnectionIndex >= 0)
   {
      ASSIGN_BD_ADDR(LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      LEContextInfo[LEConnectionIndex].ConnectionID = 0;

      /* Re-initialize the Transmit and Receive Buffers, as well as the */
      /* transmit credits.                                              */
      //InitializeBuffer(&(LEContextInfo[LEConnectionIndex].BLEBufferInfo.TransmitBuffer));
      //InitializeBuffer(&(LEContextInfo[LEConnectionIndex].BLEBufferInfo.ReceiveBuffer));
   }
}

   /* ***************************************************************** */
   /*                         Event Callbacks                           */
   /* ***************************************************************** */


static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter)
/**
 * GAP LE Event Receive Data Callback
 *
 * @param[in]   BluetoothStackID  	BLE stack ID
 * @param[in]   GAP_LE_Event_Data	Pointer to GAP Event data
 * @param[in]	CallbackParameter	Callback parameter to function
 *
 * @return      None
 *
 * This function will be called whenever a Callback has
 * been registered for the specified GAP LE Action that is associated
 * with the Bluetooth Stack.  This function passes to the caller the
 * GAP LE Event Data of the specified Event and the GAP LE Event
 * Callback Parameter that was specified when this Callback was
 * installed.  The caller is free to use the contents of the GAP LE
 * Event Data ONLY in the context of this callback.  If the caller
 * requires the Data for a longer period of time, then the callback
 * function MUST copy the data into another Data Buffer.  This
 * function is guaranteed NOT to be invoked more than once
 * simultaneously for the specified installed callback (i.e.  this
 * function DOES NOT have be reentrant).  It Needs to be noted
 * however, that if the same Callback is installed more than once,
 * then the callbacks will be called serially.  Because of this, the
 * processing in this function should be as efficient as possible.
 * It should also be noted that this function is called in the Thread
 * Context of a Thread that the User does NOT own.  Therefore,
 * processing in this function should be as efficient as possible
 * (this argument holds anyway because other GAP Events will not be
 * processed while this function call is outstanding).
 * * NOTE * This function MUST NOT Block and wait for Events that can
 *          only be satisfied by Receiving a Bluetooth Event
 *          Callback.  A Deadlock WILL occur because NO Bluetooth
 *          Callbacks will be issued while this function is currently
 *          outstanding.
 */
{
   int                                           Result;
   int                                           LEConnectionInfo;
   BoardStr_t                                    BoardStr;
   unsigned int                                  Index;
   DeviceInfo_t                                 *DeviceInfo;
   GAP_LE_Security_Information_t                 GAP_LE_Security_Information;
   GAP_LE_Connection_Parameters_t                ConnectionParameters;
   GAP_LE_Advertising_Report_Data_t             *DeviceEntryPtr;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((ApplicationStateInfo.BluetoothStackID) && (GAP_LE_Event_Data))
   {
      switch(GAP_LE_Event_Data->Event_Data_Type)
      {
         case etLE_Advertising_Report:
        	DEBUG_PRINT("\r\netLE_Advertising_Report with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size);
            DEBUG_PRINT("  %d Responses.\r\n",GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries);

            for(Index = 0; Index < GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries; Index++)
            {
               DeviceEntryPtr = &(GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[Index]);

               /* Display the packet type for the device                */
               switch(DeviceEntryPtr->Advertising_Report_Type)
               {
                  case rtConnectableUndirected:
                     DEBUG_PRINT("  Advertising Type: %s.\r\n", "rtConnectableUndirected");
                     break;
                  case rtConnectableDirected:
                	  DEBUG_PRINT("  Advertising Type: %s.\r\n", "rtConnectableDirected");
                     break;
                  case rtScannableUndirected:
                	  DEBUG_PRINT("  Advertising Type: %s.\r\n", "rtScannableUndirected");
                     break;
                  case rtNonConnectableUndirected:
                	  DEBUG_PRINT("  Advertising Type: %s.\r\n", "rtNonConnectableUndirected");
                     break;
                  case rtScanResponse:
                	  DEBUG_PRINT("  Advertising Type: %s.\r\n", "rtScanResponse");
                     break;
               }

               /* Display the Address Type.                             */
               if(DeviceEntryPtr->Address_Type == latPublic)
               {
            	   DEBUG_PRINT("  Address Type: %s.\r\n","atPublic");
               }
               else
               {
            	   DEBUG_PRINT("  Address Type: %s.\r\n","atRandom");
               }

               /* Display the Device Address.                           */
               DEBUG_PRINT("  Address: 0x%02X%02X%02X%02X%02X%02X.\r\n", DeviceEntryPtr->BD_ADDR.BD_ADDR5, DeviceEntryPtr->BD_ADDR.BD_ADDR4, DeviceEntryPtr->BD_ADDR.BD_ADDR3, DeviceEntryPtr->BD_ADDR.BD_ADDR2, DeviceEntryPtr->BD_ADDR.BD_ADDR1, DeviceEntryPtr->BD_ADDR.BD_ADDR0);
               DEBUG_PRINT("  RSSI: %d.\r\n", (int)DeviceEntryPtr->RSSI);
               DEBUG_PRINT("  Data Length: %d.\r\n", DeviceEntryPtr->Raw_Report_Length);

               DisplayAdvertisingData(&(DeviceEntryPtr->Advertising_Data));
            }
            break;
         case etLE_Connection_Complete:
            DEBUG_PRINT("\r\netLE_Connection_Complete with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, BoardStr);

               DEBUG_PRINT("   Status:       0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status);
               DEBUG_PRINT("   Role:         %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave");
               DEBUG_PRINT("   Address Type: %s.\r\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type == latPublic)?"Public":"Random");
               DEBUG_PRINT("   BD_ADDR:      %s.\r\n", BoardStr);

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  /* If not already in the connection info array, add   */
                  /* it.                                                */
                  if(BLEUtil_FindLEIndexByAddress(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address) < 0)
                  {
                     /* Find an unused position in the connection info  */
                     /* array.                                          */
                     LEConnectionInfo = FindFreeLEIndex();
                     if(LEConnectionInfo >= 0)
                        LEContextInfo[LEConnectionInfo].ConnectionBD_ADDR = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
                  }

                  /* Set a global flag to indicate if we are the        */
                  /* connection master.                                 */
                  LocalDeviceIsMaster = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master;

                  // Initialize command handler
                  InitBLECmdHandlerLiason();
                  bleNotificationHandler_Init();

                  /* Make sure that no entry already exists.            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)) == NULL)
                  {
                     /* No entry exists so create one.                  */
                     if(!CreateNewDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address))
                    	 DEBUG_PRINT("Failed to add device to Device Info List.\r\n");
                  }
                  else
                  {
                     /* If we are the Master of the connection we will  */
                     /* attempt to Re-Establish Security if a LTK for   */
                     /* this device exists (i.e.  we previously paired).*/
                     if(LocalDeviceIsMaster)
                     {
                        /* Re-Establish Security if there is a LTK that */
                        /* is stored for this device.                   */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID)
                        {
                           /* Re-Establish Security with this LTK.      */
                        	DEBUG_PRINT("Attempting to Re-Establish Security.\r\n");

                           /* Attempt to re-establish security to this  */
                           /* device.                                   */
                           GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.LTK), &(DeviceInfo->LTK), LONG_TERM_KEY_SIZE);
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.Rand), &(DeviceInfo->Rand), RANDOM_NUMBER_DATA_SIZE);

                           GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                           GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                           Result = GAP_LE_Reestablish_Security(ApplicationStateInfo.BluetoothStackID, DeviceInfo->ConnectionBD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                           if(Result)
                           {
                        	   DEBUG_PRINT("GAP_LE_Reestablish_Security returned %d.\r\n",Result);
                           }
                        }
                     }
                  }
               }
               else
               {
                  /* Clear the Connection ID.                              */
                  RemoveConnectionInfo(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address);
               }
            }
            break;
         case etLE_Disconnection_Complete:
            DEBUG_PRINT("\r\netLE_Disconnection_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
            {
            	DEBUG_PRINT("   Status: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status);
            	DEBUG_PRINT("   Reason: 0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason);

            	BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, BoardStr);

            	DEBUG_PRINT("   BD_ADDR: %s.\r\n", BoardStr);

               /* Check to see if the device info is present in the     */
               /* list.                                                 */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
               {
                  /* Flag that no service discovery operation is        */
                  /* outstanding for this device.                       */
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;

                  /* If this device is not paired, then delete it.  The */
                  /* link will be encrypted if the device is paired.    */
                  if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LINK_ENCRYPTED))
                  {
                     if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
                        FreeDeviceInfoEntryMemory(DeviceInfo);
                  }
                  else
                  {
                     /* Flag that the Link is no longer encrypted since */
                     /* we have disconnected.                           */
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
                  }
               }

               // Now that disconnection is complete, reset command handler & start advertising again
               DeInitBLECmdHandlerLiason();
               bleNotificationHandler_DeInit();

               AdvertiseLE(TRUE);
            }
            break;
         case etLE_Connection_Parameter_Update_Request:
        	 DEBUG_PRINT("\r\netLE_Connection_Parameter_Update_Request with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, BoardStr);
               DEBUG_PRINT("   BD_ADDR:             %s.\r\n", BoardStr);
               DEBUG_PRINT("   Minimum Interval:    %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min);
               DEBUG_PRINT("   Maximum Interval:    %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max);
               DEBUG_PRINT("   Slave Latency:       %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency);
               DEBUG_PRINT("   Supervision Timeout: %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout);

               /* Initialize the connection parameters.                 */
               ConnectionParameters.Connection_Interval_Min    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min;
               ConnectionParameters.Connection_Interval_Max    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max;
               ConnectionParameters.Slave_Latency              = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency;
               ConnectionParameters.Supervision_Timeout        = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout;
               ConnectionParameters.Minimum_Connection_Length  = 0;
               ConnectionParameters.Maximum_Connection_Length  = 10000;

               DEBUG_PRINT("\r\nAttempting to accept connection parameter update request.\r\n");

               /* Go ahead and accept whatever the slave has requested. */
               Result = GAP_LE_Connection_Parameter_Update_Response(ApplicationStateInfo.BluetoothStackID, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, TRUE, &ConnectionParameters);
               if(!Result)
               {
                  DEBUG_PRINT("      GAP_LE_Connection_Parameter_Update_Response() success.\r\n");
               }
               else
               {
                  DEBUG_PRINT("      GAP_LE_Connection_Parameter_Update_Response() error %d.\r\n", Result);
               }
            }
            break;
         case etLE_Connection_Parameter_Updated:
            DEBUG_PRINT("\r\netLE_Connection_Parameter_Updated with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->BD_ADDR, BoardStr);
               DEBUG_PRINT("   Status:              0x%02X.\r\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status);
               DEBUG_PRINT("   BD_ADDR:             %s.\r\n", BoardStr);

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  DEBUG_PRINT("   Connection Interval: %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Connection_Interval);
                  DEBUG_PRINT("   Slave Latency:       %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Slave_Latency);
                  DEBUG_PRINT("   Supervision Timeout: %u.\r\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Supervision_Timeout);
               }
            }
            break;
         case etLE_Encryption_Change:
            DEBUG_PRINT("\r\netLE_Encryption_Change with size %d.\r\n",(int)GAP_LE_Event_Data->Event_Data_Size);
            break;
         case etLE_Encryption_Refresh_Complete:
            DEBUG_PRINT("\r\netLE_Encryption_Refresh_Complete with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size);
            break;
         case etLE_Authentication:
            DEBUG_PRINT("\r\netLE_Authentication with size %d.\r\n", (int)GAP_LE_Event_Data->Event_Data_Size);
            break;
      }
   }
}

static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter)
/**
 * GATT Connection Event Callback
 *
 * @param[in]   BluetoothStackID  	BLE stack ID
 * @param[in]   GATT_LE_Event_Data	Pointer to GATT Event data
 * @param[in]	CallbackParameter	Callback parameter to function
 *
 * @return      Pointer to the entry next to deleted entry
 *
 * This function is called for GATT Connection Events that occur on
 * the specified Bluetooth Stack.  This function passes to the caller
 * the GATT Connection Event Data that occurred and the GATT
 * Connection Event Callback Parameter that was specified when this
 * Callback was installed.  The caller is free to use the contents of
 * the GATT Client Event Data ONLY in the context of this callback.
 * If the caller requires the Data for a longer period of time, then
 * the callback function MUST copy the data into another Data Buffer.
 * This function is guaranteed NOT to be invoked more than once
 * simultaneously for the specified installed callback (i.e.  this
 * function DOES NOT have be reentrant).  It Needs to be noted
 * however, that if the same Callback is installed more than once,
 * then the callbacks will be called serially.  Because of this, the
 * processing in this function should be as efficient as possible.
 * It should also be noted that this function is called in the Thread
 * Context of a Thread that the User does NOT own.  Therefore,
 * processing in this function should be as efficient as possible
 * (this argument holds anyway because another GATT Event
 * (Server/Client or Connection) will not be processed while this
 * function call is outstanding).
 * * NOTE * This function MUST NOT Block and wait for Events that can
 *          only be satisfied by Receiving a Bluetooth Event
 *          Callback.  A Deadlock WILL occur because NO Bluetooth
 *          Callbacks will be issued while this function is currently
 *          outstanding.
 */
{
   int           LEConnectionIndex;
   //Word_t        Credits;
   BoardStr_t    BoardStr;
   DeviceInfo_t *DeviceInfo;

   DEBUG_PRINT("\r\nGATT Connection Event Callback\r\n");

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((ApplicationStateInfo.BluetoothStackID) && (GATT_Connection_Event_Data))
   {
      /* Determine the Connection Event that occurred.                  */
      switch(GATT_Connection_Event_Data->Event_Data_Type)
      {
         case etGATT_Connection_Device_Connection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data)
            {
               // Set the MTU size for the connection
               if (GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU > BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE)
            	   gBLESuppMTUSize = BTPS_CONFIGURATION_GATT_MAXIMUM_SUPPORTED_MTU_SIZE - MTU_PACKET_HEADER_SIZE;
               else
            	   gBLESuppMTUSize = GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU - MTU_PACKET_HEADER_SIZE;

               /* Update the ConnectionID associated with the BD_ADDR   */
               /* If UpdateConnectionID returns -1, then it failed.     */
               if(UpdateConnectionID(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice) < 0)
                   DEBUG_PRINT("Error - No matching ConnectionBD_ADDR found.");

               DEBUG_PRINT("\r\netGATT_Connection_Device_Connection with size %u: \r\n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
               DEBUG_PRINT("   Connection ID:   %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID);
               DEBUG_PRINT("   Connection Type: %s.\r\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               DEBUG_PRINT("   Remote Device:   %s.\r\n", BoardStr);
               DEBUG_PRINT("   Connection MTU:  %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU);

               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = BLEUtil_FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice)) >= 0)
               {
                  /* Search for the device info for the connection.     */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                  {
                     /* Clear the BLE Role Flag.                      */
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_BLE_SERVER;

                     /* Flag that we will act as the Server.         */
                    DeviceInfo->Flags |= DEVICE_INFO_FLAGS_BLE_SERVER;
                  }
               }

               //Now that connection is established, change the status to start blinking blue LED
               nnoStatus_setDeviceStatus(NNO_STATUS_ACTIVE_BLE_CONNECTION,true);

               // Notify Command Interface Manager that BLE is ready to process commands
               bleConn();
            }
            else
               DEBUG_PRINT("Error - Null Connection Data.\r\n");
            break;
         case etGATT_Connection_Device_Disconnection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data)
            {
               /* Clear the Connection ID.                              */
               RemoveConnectionInfo(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice);

               DEBUG_PRINT("\r\netGATT_Connection_Device_Disconnection with size %u: \r\n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice, BoardStr);
               DEBUG_PRINT("   Connection ID:   %u.\r\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionID);
               DEBUG_PRINT("   Connection Type: %s.\r\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               DEBUG_PRINT("   Remote Device:   %s.\r\n", BoardStr);

               // Notify Command Interface Manager that BLE interface is disconnected
               bleDisc();
            }
            else
            	DEBUG_PRINT("Error - Null Disconnection Data.\r\n");
            break;
         case etGATT_Connection_Device_Buffer_Empty:
        	 DEBUG_PRINT("\r\netGATT_Connection_Device_Buffer_Empty\r\n");
        	 if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data)
        		 bleCmdHandlerLiaison_handleBLEResponse(true, GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data->ConnectionID);
        	 break;
      }
   }
   else
      DEBUG_PRINT("\r\nGATT Connection Callback Data: Event_Data = NULL.\r\n");
}

static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter)
/**
 * GAP Event Receive Data Callback
 *
 * @param[in]   BluetoothStackID  	BLE stack instance ID
 * @param[in]   GAP_Event_Data		Pointer to the event data
 * @param[in]	CallbackParameter	Callback parameter
 *
 * @return      None
 *
 * This function will be called whenever a Callback has been
 * registered for the specified GAP Action that is associated with
 * the Bluetooth Stack.  This function passes to the caller the GAP
 * Event Data of the specified Event and the GAP Event Callback
 * Parameter that was specified when this Callback was installed.
 * The caller is free to use the contents of the GAP Event Data ONLY
 * in the context of this callback.  If the caller requires the Data
 * for a longer period of time, then the callback function MUST copy
 * the data into another Data Buffer.  This function is guaranteed
 * NOT to be invoked more than once simultaneously for the specified
 * installed callback (i.e.  this function DOES NOT have be
 * reentrant).  It Needs to be noted however, that if the same
 * Callback is installed more than once, then the callbacks will be
 * called serially.  Because of this, the processing in this function
 * should be as efficient as possible.  It should also be noted that
 * this function is called in the Thread Context of a Thread that the
 * User does NOT own.  Therefore, processing in this function should
 * be as efficient as possible (this argument holds anyway because
 * other GAP Events will not be processed while this function call is
 * outstanding).
 * * NOTE * This function MUST NOT Block and wait for events that
 *          can only be satisfied by Receiving other GAP Events.  A
 *          Deadlock WILL occur because NO GAP Event Callbacks will
 *          be issued while this function is currently outstanding.
 */
{
   BoardStr_t                        Callback_BoardStr;
   GAP_Remote_Name_Event_Data_t     *GAP_Remote_Name_Event_Data;

	nano_timer_increment_activity_count();

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((ApplicationStateInfo.BluetoothStackID) && (GAP_Event_Data))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(GAP_Event_Data->Event_Data_Type)
      {
         case etInquiry_Result:
            /* The GAP event received was of type Inquiry_Result.       */
            break;
         case etInquiry_Entry_Result:
            break;
         case etAuthentication:
            /* An authentication event occurred,  add support later if required */
            break;
         case etRemote_Name_Result:
            /* Bluetooth Stack has responded to a previously issued     */
            /* Remote Name Request that was issued.                     */
            GAP_Remote_Name_Event_Data = GAP_Event_Data->Event_Data.GAP_Remote_Name_Event_Data;
            if(GAP_Remote_Name_Event_Data)
            {
               /* Inform the user of the Result.                        */
               BD_ADDRToStr(GAP_Remote_Name_Event_Data->Remote_Device, Callback_BoardStr);

               DEBUG_PRINT("\r\n");
               DEBUG_PRINT("BD_ADDR: %s.\r\n", Callback_BoardStr);

               if(GAP_Remote_Name_Event_Data->Remote_Name)
               {
                   DEBUG_PRINT("Name: %s.\r\n", GAP_Remote_Name_Event_Data->Remote_Name);
               }
               else
               {
            	   DEBUG_PRINT("Name: NULL.\r\n");
               }
            }
            break;
         case etEncryption_Change_Result:
            break;
         default:
            /* An unknown/unexpected GAP event was received.            */
            DEBUG_PRINT("\r\nUnknown Event: %d.\r\n", GAP_Event_Data->Event_Data_Type);
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      DEBUG_PRINT("\r\n");
      DEBUG_PRINT("Null Event\r\n");
   }

}

static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter)
/**
 * Responsible for processing HCI Mode change events
 *
 * @param[in]   BluetoothStackID  	BLE stack instance ID
 * @param[in]   HCI_Event_Data		Pointer to HCI event data
 * @param[in]	CallbackParameter	Calback parameter
 *
 * @return      None
 *
 */
{
   /* Make sure that the input parameters that were passed to us are    */
   /* semi-valid.                                                       */
   if((ApplicationStateInfo.BluetoothStackID) && (HCI_Event_Data))
   {
      /* Process the Event Data.                                        */
      switch(HCI_Event_Data->Event_Data_Type)
      {
         case etMode_Change_Event:
            break;
      }
   }
}

int InitializeApplication(HCI_DriverInformation_t *HCI_DriverInformation, BTPS_Initialization_t *BTPS_Initialization)
/**
 * To initialize the application instance
 *
 * @param[in]   HCI_DriverInformation	HCI Driver Information used to open stack
 * @param[in]   BTPS_Initialization		BTPS configuration parameters to BTPS_init()
 *
 * @return      Returns Bluetooth ID of intialized app or negative error code
 *
 */
{
   int ret_val = APPLICATION_ERROR_UNABLE_TO_OPEN_STACK;

   /* Next, makes sure that the Driver Information passed appears to be */
   /* semi-valid.                                                       */
   if((HCI_DriverInformation) && (BTPS_Initialization))
   {
	   ret_val = OpenStack(HCI_DriverInformation, BTPS_Initialization);
      /* Try to Open the stack and check if it was successful.          */
      if(!ret_val)
      {
         /* First, attempt to set the Device to be Connectable.         */
         ret_val = SetConnect();

         /* Next, check to see if the Device was successfully made      */
         /* Connectable.                                                */
         if(!ret_val)
         {
            /* Now that the device is Connectable attempt to make it    */
            /* Discoverable.                                            */
            ret_val = SetDisc();

            /* Next, check to see if the Device was successfully made   */
            /* Discoverable.                                            */
            if(!ret_val)
            {
               /* Now that the device is discoverable attempt to make it*/
               /* pairable.                                             */
               ret_val = SetPairable();
               if(!ret_val)
               {
                  /* Attempt to register a HCI Event Callback.          */
                  ret_val = HCI_Register_Event_Callback(ApplicationStateInfo.BluetoothStackID, HCI_Event_Callback, (unsigned long)NULL);
                  if(ret_val > 0)
                  {
                     /* Return success to the caller.                   */
                     ret_val = (int)ApplicationStateInfo.BluetoothStackID;
                  }
               }
               else
                  bleLogFuncError("SetPairable", ret_val);
            }
            else
               bleLogFuncError("SetDisc", ret_val);
         }
         else
            bleLogFuncError("SetDisc", ret_val);

         /* In some error occurred then close the stack.                */
         if(ret_val < 0)
         {
            /* Close the Bluetooth Stack.                               */
            CloseStack();
         }
      }
      else
      {
         /* There was an error while attempting to open the Stack.      */
    	  bleLogFuncError("Open stack", ret_val);
      }
   }
   else
      ret_val = APPLICATION_ERROR_INVALID_PARAMETERS;

   return(ret_val);
}

/*
 * Debug handler functions
 */

bool cmdYellowLED_wr(uint8_t len, uint8_t *pData)
/**
 * To control yellow LED
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len >= 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	if (pData[0] == 1)
	{
		yellowLED_on();
		return false;
	}
	else if (pData[0] == 0)
	{
		yellowLED_off();
		return true;
	}
	else
		return false;
}

bool cmdBatteryLife_wr(uint8_t len, uint8_t *pData)
/**
 * To write Battery life
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len >= 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	GATTStdSvcs_UpdateBatteryLevel(ApplicationStateInfo.BluetoothStackID, pData[0]);

	return true;
}

bool cmdTempMeas_wr(uint8_t len, uint8_t *pData)
/**
 * To write temperature
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len > 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	if (0 == GATTGISvc_SetTemp((pData[0] | (pData[1] << 8)), true))
		return true;
	else
		return false;
}

bool cmdHumMeas_wr(uint8_t len, uint8_t *pData)
/**
 * To write humidity value
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len > 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	if (0 == GATTGISvc_SetHum((pData[0] | (pData[1] << 8)), true))
		return true;
	else
		return false;
}

bool cmdDevStat_wr(uint8_t len, uint8_t *pData)
/**
 * To write device status
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len != 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	uint16_t temp = (pData[0] << 8) | pData[1];

	if (0 == GATTGISvc_SetDevStat(temp))
		return true;
	else
		return false;
}

bool cmdErrStat_wr(uint8_t len, uint8_t *pData)
/**
 * To write error status
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len != 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	uint16_t temp = (pData[0] << 8) | pData[1];

	if (0 == GATTGISvc_SetErrStat(temp))
		return true;
	else
		return false;
}

bool cmdHoursOfUse_wr(uint8_t len, uint8_t *pData)
/**
 * To write hours of use
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len > 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	uint16_t temp = (pData[0] << 8) | pData[1];
	DEBUG_PRINT("\r\nSet value in func:%d\r\n",temp);
	if (0 == GATTGISvc_SetNumHoursOfUse(temp))
		return true;
	else
		return false;
}

bool cmdNumBattRecharge_wr(uint8_t len, uint8_t *pData)
/**
 * To write number of battery recharge cycles
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len > 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	uint16_t temp = (pData[0] << 8) | pData[1];

	if (0 == GATTGISvc_SetBattRecharge(temp))
		return true;
	else
		return false;
}

bool cmdTotalLampHours_wr(uint8_t len, uint8_t *pData)
/**
 * To write number of lamp hours
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len > 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	uint16_t temp = (pData[0] << 8) | pData[1];

	if (0 == GATTGISvc_SetLampHours(temp))
		return true;
	else
		return false;
}

bool cmdNumStoredConf_wr(uint8_t len, uint8_t *pData)
/**
 * To write number of stored configurations
 *
 * @param[in]   len  	Length of data
 * @param[in]   pData	Pointer to input data
 *
 * @return      Success/Failure
 *
 */
{
	if (len > 2)
	{
		DEBUG_PRINT("\r\nError: Unexpected command length recieved\r\n");
		return false;
	}

	uint16_t temp = (pData[1] << 8) | pData[0];
	if (0 == GATTConfSvc_SetNumConf(temp))
		return true;
	else
		return false;

}

void bleHandleInternalMessage(unsigned char length, unsigned char *command, unsigned int bluetoothID,
							  unsigned int serviceID, unsigned int connectionID, unsigned int ccdOffset)
/**
 * To handle commands from client
 *
 * @param[in]   length	length of command & parameter data
 * @param[in]   command	Command and parameter
 * @param[in]   bluetoothID	bluetooth stack instance ID
 * @param[in]   serviceID Service ID of caller
 * @param[in]   connectionID Client connection ID
 * @param[in]   ccdOffset Offset of notification characteristic in service def. table
 *
 * @return      None
 *
 */
{
	int ret_val = 0;
	int len = 0;
	BLE_RESPONSE_INFO cmdInfo;

	ret_val = bleCmdHandlerLiason_dbgCmdHandler(CMD_KEY(command[0], command[1], command[2], 0), command[3], (uint8_t *)&command[4]);
	if (-1 == ret_val)	//Invoke the main command handler
	{
		DEBUG_PRINT("\r\nCommand not supported by Debug command Handler\r\n",ret_val);

		cmdInfo.key = command[2] | (command[1] << 8) | (command[0] << 16);
		cmdInfo.cmdType = BLE_COMMAND_TYPE_WRITE_NOTIFY;
		cmdInfo.fileType = NNO_OTHER_COMMANDS;
		cmdInfo.subfieldType = 0;
		cmdInfo.dataType = 1;
		cmdInfo.btInfo.ccdOffset = ccdOffset;
		cmdInfo.btInfo.bluetoothID = bluetoothID;
		cmdInfo.btInfo.connectionID = connectionID;
		cmdInfo.btInfo.serviceID = serviceID;
		len = command[3];

		ret_val = bleCmdHandlerLiason_relayCmd(&command[4], len, cmdInfo);
		if (ret_val != 0)
			DEBUG_PRINT("\r\nCommand Handler failed! Error code:%d\r\n",ret_val);
	}
	else if (ret_val > 0)
	{
		DEBUG_PRINT("\r\nBLE debug Command Handler failed! Error code:%d\r\n",ret_val);
		return;
	}
}

bool isBLEConnActive()
/**
 * To know if there is an active BLE connection
 *
 * @return      TRUE= active connection is present, FALSE=no active connection
 *
 */
{
	uint32_t dev_stat = 0;

	if (nnoStatus_getDeviceStatus(&dev_stat) < 0)
		return false;

	return (dev_stat & NNO_STATUS_ACTIVE_BLE_CONNECTION);
}

/* GATT Error Response override - defined in BLECommonDefs.h */
int bleGATTErrorResponse(unsigned int BluetoothStackID, unsigned int TransactionID, uint16_t AttributeOffset, uint8_t ErrorCode)
/**
 * Wrapper for GATT error response - to send error notification to client
 *
 * @param[in]   BluetoothStackID  	BLE stack instance ID
 * @param[in]   TransactionID		ID of failed transaction
 * @param[in]   AttributeOffset  	Attribute offset in service definition table
 * @param[in]   ErrorCode			App error code
 *
 * @return      Pointer to the entry next to deleted entry
 *
 */
{
	int result = GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ErrorCode);

	if (PASS == result)	// If error response was sent, send notification now with original error code
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_BLE, (int16_t) ErrorCode);
	else	// send notification with failure code for error response instead!
		bleNotificationHandler_sendErrorIndication(NNO_ERROR_BLE, (int16_t) result);

	return (result);

}
#endif
