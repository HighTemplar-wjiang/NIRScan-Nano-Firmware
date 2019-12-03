//*****************************************************************************
//
//  USB HID CustomHid device class driver.
//
// Copyright (c) 2008-2015 Texas Instruments Incorporated.  All rights reserved.
// 
//*****************************************************************************

#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/device/usbdevice.h"
#include "usblib/usbhid.h"
#include "usblib/device/usbdhid.h"
#include "usbdhidcustom.h"

// TODO: Remove the OUT_EP #define
#define OUT_EP

static uint8_t reportBuffer[CUSTOMHID_REPORT_SIZE];
static void *pvHIDInstance;

void *USBDHIDCustomHidCompositeInit(unsigned long ulIndex,
                                       const tUSBDHIDCustomHidDevice *psDevice);
void USBDHIDCustomHidTerm(void *pvInstance);
void *USBDHIDCustomHidSetCBData(void *pvInstance, void *pvCBData);
void USBDHIDCustomHidPowerStatusSet(void *pvInstance,
                                       unsigned char ucPower);
bool USBDHIDCustomHidRemoteWakeupRequest(void *pvInstance);

//*****************************************************************************
//
//! \addtogroup hid_nirscan_nano_device_class_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// HID device configuration descriptor.
//
// It is vital that the configuration descriptor bConfigurationValue field
// (byte 6) is 1 for the first configuration and increments by 1 for each
// additional configuration defined here.  This relationship is assumed in the
// device stack for simplicity even though the USB 2.0 specification imposes
// no such restriction on the bConfigurationValue values.
//
// Note that this structure is deliberately located in RAM since we need to
// be able to patch some values in it based on client requirements.
//
//*****************************************************************************
uint8_t g_pui8NirscanNanoDescriptor[] =
{
    //
    // Configuration descriptor header.
    //
    9,                          // Size of the configuration descriptor.
    USB_DTYPE_CONFIGURATION,    // Type of this descriptor.
    USBShort(34),               // The total size of this full structure.
    1,                          // The number of interfaces in this
                                // configuration.
    1,                          // The unique value for this configuration.
    5,                          // The string identifier that describes this
                                // configuration.
    USB_CONF_ATTR_SELF_PWR,     // Bus Powered, Self Powered, remote wake up.
    250,                        // The maximum power in 2mA increments.
};

//*****************************************************************************
//
// The remainder of the configuration descriptor is stored in flash since we
// don't need to modify anything in it at runtime.
//
//*****************************************************************************
uint8_t g_pui8HIDInterface[HIDINTERFACE_SIZE] =
{
    //
    // HID Device Class Interface Descriptor.
    //
    9,                          // Size of the interface descriptor.
    USB_DTYPE_INTERFACE,        // Type of this descriptor.
    0,                          // The index for this interface.
    0,                          // The alternate setting for this interface.
#ifdef OUT_EP
    2,                          // The number of endpoints used by this
#else
    1,                          // The number of endpoints used by this
#endif
                                // interface.
    USB_CLASS_HID,              // The interface class
    USB_HID_SCLASS_NONE,        // The interface sub-class.
    USB_HID_PROTOCOL_NONE,     // The interface protocol for the sub-class
                                // specified above.
    4,                          // The string index for this interface.
};

const uint8_t g_pui8HIDInEndpoint[HIDINENDPOINT_SIZE] =
{
    //
    // Interrupt IN endpoint descriptor
    //
    7,                          // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,         // Descriptor type is an endpoint.
    USB_EP_DESC_IN | USBEPToIndex(USB_EP_1),
    USB_EP_ATTR_INT,            // Endpoint is an interrupt endpoint.
    USBShort(USBFIFOSizeToBytes(USB_FIFO_SZ_64)),
                                // The maximum packet size.
    1,                         // The polling interval for this endpoint.
};

const uint8_t g_pui8HIDOutEndpoint[HIDOUTENDPOINT_SIZE] =
{
    //
    // Interrupt OUT endpoint descriptor
    //
    7,                          // The size of the endpoint descriptor.
    USB_DTYPE_ENDPOINT,         // Descriptor type is an endpoint.
    USB_EP_DESC_OUT | USBEPToIndex(USB_EP_1),
    USB_EP_ATTR_INT,            // Endpoint is an interrupt endpoint.
    USBShort(USBFIFOSizeToBytes(USB_FIFO_SZ_64)),
                                // The maximum packet size.
    1,                         // The polling interval for this endpoint.
};


//*****************************************************************************
//
// The report descriptor for custom DLP class device.
//
//*****************************************************************************

static const uint8_t g_pui8NirscanNanoReportDescriptor[]=
{
    UsagePageVendor(0xFF00),
    UsageVendor(0xFF00),
    Collection(USB_HID_APPLICATION),
    LogicalMinimum(0),
    LogicalMaximum(255),
    ReportSize(8),
    ReportCount(64),
    Usage(1),
    Input(0),
    Usage(2),
    Output(0),
    Usage(3),
    Feature(2),
    EndCollection
};


//*****************************************************************************
//
// The HID descriptor for the NirscanNano device.
//
//*****************************************************************************
static const tHIDDescriptor g_sNirscanNanoDescriptor =
{
    9,                                 // bLength
    USB_HID_DTYPE_HID,                 // bDescriptorType
    0x111,                             // bcdHID (version 1.11 compliant)
    0,                                 // bCountryCode (not localized)
    1,                                 // bNumDescriptors
    {
        {
            USB_HID_DTYPE_REPORT,                  // Report descriptor
            sizeof(g_pui8NirscanNanoReportDescriptor)     // Size of report descriptor
        }
    }
};

//*****************************************************************************
//
// The HID configuration descriptor is defined as four or five sections
// depending upon the client's configuration choice.  These sections are:
//
// 1.  The 9 byte configuration descriptor (RAM).
// 2.  The interface descriptor (RAM).
// 3.  The HID report and physical descriptors (provided by the client)
//     (FLASH).
// 4.  The mandatory interrupt IN endpoint descriptor (FLASH).
// 5.  The optional interrupt OUT endpoint descriptor (FLASH).
//
//*****************************************************************************
const tConfigSection g_sHIDConfigSection =
{
    sizeof(g_pui8NirscanNanoDescriptor),
    g_pui8NirscanNanoDescriptor
};

const tConfigSection g_sHIDInterfaceSection =
{
    sizeof(g_pui8HIDInterface),
    g_pui8HIDInterface
};

const tConfigSection g_sHIDInEndpointSection =
{
    sizeof(g_pui8HIDInEndpoint),
    g_pui8HIDInEndpoint
};

#ifdef OUT_EP
const tConfigSection g_sHIDOutEndpointSection =
{
    sizeof(g_pui8HIDOutEndpoint),
    g_pui8HIDOutEndpoint
};
#endif

//*****************************************************************************
//
// Place holder for the user's HID descriptor block.
//
//*****************************************************************************
tConfigSection g_sHIDDescriptorSection =
{
   sizeof(g_sNirscanNanoDescriptor),
   (const uint8_t *)&g_sNirscanNanoDescriptor
};

//*****************************************************************************
//
// This array lists all the sections that must be concatenated to make a
// single, complete HID configuration descriptor.
//
//*****************************************************************************
const tConfigSection *g_psHIDSections[] =
{
    &g_sHIDConfigSection,
    &g_sHIDInterfaceSection,
    &g_sHIDDescriptorSection,
    &g_sHIDInEndpointSection,
#ifdef OUT_EP
	&g_sHIDOutEndpointSection,
#endif
};

#define NUM_HID_SECTIONS        (sizeof(g_psHIDSections) /                    \
                                 sizeof(g_psHIDSections[0]))

//*****************************************************************************
//
// The header for the single configuration we support.  This is the root of
// the data structure that defines all the bits and pieces that are pulled
// together to generate the configuration descriptor.  Note that this must be
// in RAM since we need to include or exclude the final section based on
// client supplied initialization parameters.
//
//*****************************************************************************
tConfigHeader g_sHIDConfigHeader =
{
    NUM_HID_SECTIONS,
    g_psHIDSections
};

//*****************************************************************************
//
// Configuration Descriptor.
//
//*****************************************************************************
const tConfigHeader * const g_ppsHIDConfigDescriptors[] =
{
    &g_sHIDConfigHeader
};

//*****************************************************************************
//
// The HID class descriptor table.  For the mouse class, we have only a single
// report descriptor.
//
//*****************************************************************************
static const uint8_t * const g_pui8NirscanNanoClassDescriptors[] =
{
    g_pui8NirscanNanoReportDescriptor
};

//*****************************************************************************
//*****************************************************************************
//
// Forward references for customhid device callback functions.
//
//*****************************************************************************
static uint32_t HIDCustomHidRxHandler(void *pvCBData,
										uint32_t ulEvent,
										uint32_t ulMsgData,
                                          void *pvMsgData);
static uint32_t HIDCustomHidTxHandler(void *pvCBData,
											uint32_t ulEvent,
											uint32_t ulMsgData,
                                          void *pvMsgData);



//*****************************************************************************
//
// Main HID device class event handler function.
//
// \param pvCBData is the event callback pointer provided during USBDHIDInit().
// This is a pointer to our HID device structure (&g_sHIDCustomHidDevice).
// \param ulEvent identifies the event we are being called back for.
// \param ulMsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID device class driver to inform the
// application of particular asynchronous events related to operation of the
// customhid HID device.
//
// \return Returns a value which is event-specific.
//
//*****************************************************************************
static uint32_t
HIDCustomHidRxHandler(void *pvCBData, uint32_t ulEvent,
		uint32_t ulMsgData, void *pvMsgData)
{
    tHIDCustomHidInstance *psInst;
    tUSBDHIDCustomHidDevice *psDevice;

    //
    // Make sure we didn't get a NULL pointer.
    //
    ASSERT(pvCBData);

    //
    // Get a pointer to our instance data
    //
    psDevice = (tUSBDHIDCustomHidDevice *)pvCBData;
    psInst = psDevice->psPrivateHIDCustomHidData;

    //
    // Which event were we sent?
    //
    switch (ulEvent)
    {
        //
        // The host has connected to us and configured the device.
        //
        case USB_EVENT_CONNECTED:
        {
            psInst->ucUSBConfigured = true;
            psInst->eCustomHidState = HID_CUSTOMHID_STATE_IDLE;

            //
            // Pass the information on to the client.
            //
            psDevice->pfnCallback(psDevice->pvCBData, USB_EVENT_CONNECTED,
                                  0, (void *)0);

            break;
        }

        //
        // The host has disconnected from us.
        //
        case USB_EVENT_DISCONNECTED:
        {
            psInst->ucUSBConfigured = false;
            psInst->eCustomHidState = HID_CUSTOMHID_STATE_UNCONFIGURED;

            //
            // Pass the information on to the client.
            //
            psDevice->pfnCallback(psDevice->pvCBData, USB_EVENT_DISCONNECTED,
                                  0, (void *)0);

            break;
        }

        //
        // The host is polling us for a particular report and the HID driver
        // is asking for the latest version to transmit.
        //
        case USBD_HID_EVENT_IDLE_TIMEOUT:
        case USBD_HID_EVENT_GET_REPORT:
        {
            //
            // We only support a single input report so we don't need to check
            // the ulMsgValue parameter in this case.  Set the report pointer
            // in *pvMsgData and return the length of the report in bytes.
            //
            *(unsigned char **)pvMsgData = psInst->pucReport;
            return(8);
        }

        //
        // The device class driver has completed sending a report to the
        // host in response to a Get_Report request.
        //
        case USBD_HID_EVENT_REPORT_SENT:
        {
            //
            // We have nothing to do here.
            //
            break;
        }

        case USBD_HID_EVENT_GET_REPORT_BUFFER:
        {
            //
            // We are being asked for a buffer into which the report can be written
            // must return a pointer to a suitable buffer (cast to thestandard "uint32_t" return type for the callback)

        	return((uint32_t)reportBuffer);
        }

#ifdef OUT_EP
        case USB_EVENT_RX_AVAILABLE:
        {
        	USBDHIDPacketRead(pvHIDInstance, reportBuffer, CUSTOMHID_REPORT_SIZE, true);
        	return(psDevice->pfnCallback(psDevice->pvCBData, ulEvent,
        			CUSTOMHID_REPORT_SIZE, reportBuffer));
        }
#else
        case USBD_HID_EVENT_SET_REPORT:
        {
        	//! This event indicates that the host has sent the device a report via
        	//! endpoint 0, the control endpoint.  The ui32MsgValue field indicates the
        	//! size of the report and pvMsgData points to the first byte of the report.
        	return(psDevice->pfnCallback(psDevice->pvCBData, ulEvent,
        	                                         ulMsgData, pvMsgData));
        }
#endif

        //
        // The host is asking us to set either boot or report protocol (not
        // that it makes any difference to this particular customhid).
        //
        case USBD_HID_EVENT_SET_PROTOCOL:
        {
            psInst->ucProtocol = ulMsgData;
            break;
        }

        //
        // The host is asking us to tell it which protocol we are currently
        // using, boot or request.
        //
        case USBD_HID_EVENT_GET_PROTOCOL:
        {
            return(psInst->ucProtocol);
        }

        //
        // Pass ERROR, SUSPEND and RESUME to the client unchanged.
        //
        case USB_EVENT_ERROR:
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
        {
            return(psDevice->pfnCallback(psDevice->pvCBData, ulEvent,
                                         ulMsgData, pvMsgData));
        }

        //
        // We ignore all other events.
        //
        default:
        {
            break;
        }
    }
    return(0);
}

//*****************************************************************************
//
// HID device class transmit channel event handler function.
//
// \param pvCBData is the event callback pointer provided during USBDHIDInit().
// This is a pointer to our HID device structure (&g_sHIDCustomHidDevice).
// \param ulEvent identifies the event we are being called back for.
// \param ulMsgData is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the HID device class driver to inform the
// application of particular asynchronous events related to report
// transmissions made using the interrupt IN endpoint.
//
// \return Returns a value which is event-specific.
//
//*****************************************************************************
static uint32_t
HIDCustomHidTxHandler(void *pvCBData, uint32_t ulEvent,
		uint32_t ulMsgData, void *pvMsgData)
{
    tHIDCustomHidInstance *psInst;
    tUSBDHIDCustomHidDevice *psDevice;

    //
    // Make sure we didn't get a NULL pointer.
    //
    ASSERT(pvCBData);

    //
    // Get a pointer to our instance data
    //
    psDevice = (tUSBDHIDCustomHidDevice *)pvCBData;
    psInst = psDevice->psPrivateHIDCustomHidData;

    //
    // Which event were we sent?
    //
    switch (ulEvent)
    {
        //
        // A report transmitted via the interrupt IN endpoint was acknowledged
        // by the host.
        //
        case USB_EVENT_TX_COMPLETE:
        {
            //
            // Our last transmission is complete.
            //
            psInst->eCustomHidState = HID_CUSTOMHID_STATE_IDLE;

            //
            // Pass the event on to the client.
            //
            psDevice->pfnCallback(psDevice->pvCBData, USB_EVENT_TX_COMPLETE,
                                  ulMsgData, (void *)0);

            break;
        }

        //
        // We ignore all other events related to transmission of reports via
        // the interrupt IN endpoint.
        //
        default:
        {
            break;
        }
    }

    return(0);
}

//*****************************************************************************
//
//! Initializes Nirscan nano device operation for a given USB controller.
//!
//! \param ulIndex is the index of the USB controller which is to be
//! initialized for HID customhid device operation.
//! \param psDevice points to a structure containing parameters customizing
//! the operation of the HID customhid device.
//!
//! An application wishing to offer a USB HID customhid interface to a USB host
//! must call this function to initialize the USB controller and attach the
//! customhid device to the USB bus.  This function performs all required USB
//! initialization.
//!
//! On successful completion, this function will return the \e psDevice pointer
//! passed to it.  This must be passed on all future calls to the HID customhid
//! device driver.
//!
//! When a host connects and configures the device, the application callback
//! will receive \b USB_EVENT_CONNECTED after which calls can be made to
//! USBDHIDCustomHidStateChange() to report pointer movement and button presses
//! to the host.
//!
//! \note The application must not make any calls to the lower level USB device
//! interfaces if interacting with USB via the USB HID customhid device API.
//! Doing so will cause unpredictable (though almost certainly unpleasant)
//! behavior.
//!
//! \return Returns NULL on failure or the psDevice pointer on success.
//
//*****************************************************************************
void *
USBDHIDNirscanNanoInit(unsigned long ulIndex, const tUSBDHIDCustomHidDevice *psDevice)
{
    tUSBDHIDDevice *psHIDDevice;

    //
    // Check parameter validity.
    //
    ASSERT(psDevice);
    ASSERT(psDevice->ppStringDescriptors);
    ASSERT(psDevice->psPrivateHIDCustomHidData);
    ASSERT(psDevice->pfnCallback);

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psDevice->psPrivateHIDCustomHidData->sHIDDevice;

    //
    // Call the common initialization routine.
    //
    pvHIDInstance = USBDHIDCustomHidCompositeInit(ulIndex, psDevice);

    //
    // If we initialized the HID layer successfully, pass our device pointer
    // back as the return code, otherwise return NULL to indicate an error.
    //
    if(pvHIDInstance)
    {
        //
        // Initialize the lower layer HID driver and pass it the various
        // structures and descriptors necessary to declare that we are a
        // keyboard.
        //
    	pvHIDInstance = USBDHIDInit(ulIndex, psHIDDevice);

        return((void *)psDevice);
    }
    else
    {
        return((void *)0);
    }
}

//*****************************************************************************
//
//! Initializes HID customhid device operation for a given USB controller.
//!
//! \param ulIndex is the index of the USB controller which is to be
//! initialized for HID customhid device operation.
//! \param psDevice points to a structure containing parameters customizing
//! the operation of the HID customhid device.
//!
//! This call is very similar to USBDHIDCustomHidInit() except that it is used for
//! initializing an instance of the HID customhid device for use in a composite
//! device.
//!
//! \return Returns zero on failure or a non-zero instance value that should be
//! used with the remaining USB HID CustomHid APIs.
//
//*****************************************************************************
void *
USBDHIDCustomHidCompositeInit(unsigned long ulIndex,
                          const tUSBDHIDCustomHidDevice *psDevice)
{
    tHIDCustomHidInstance *psInst;
    tUSBDHIDDevice *psHIDDevice;


    //
    // Check parameter validity.
    //
    ASSERT(psDevice);
    ASSERT(psDevice->ppStringDescriptors);
    ASSERT(psDevice->psPrivateHIDCustomHidData);
    ASSERT(psDevice->pfnCallback);

    //
    // Get a pointer to our instance data
    //
    psInst = psDevice->psPrivateHIDCustomHidData;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psDevice->psPrivateHIDCustomHidData->sHIDDevice;

    //
    // Initialize the various fields in our instance structure.
    //
    psInst->ucUSBConfigured = 0;
    psInst->ucProtocol = USB_HID_PROTOCOL_REPORT;
    psInst->sReportIdle.ui8Duration4mS = 0;
    psInst->sReportIdle.ui8ReportID = 0;
    psInst->sReportIdle.ui32TimeSinceReportmS = 0;
    psInst->sReportIdle.ui16TimeTillNextmS = 0;
    psInst->eCustomHidState = HID_CUSTOMHID_STATE_UNCONFIGURED;

    //
    // Initialize the HID device class instance structure based on input from
    // the caller.
    //
    psHIDDevice->ui16PID = psDevice->usPID;
    psHIDDevice->ui16VID = psDevice->usVID;
    psHIDDevice->ui16MaxPowermA = psDevice->usMaxPowermA;
    psHIDDevice->ui8PwrAttributes = psDevice->ucPwrAttributes;
    psHIDDevice->ui8Subclass = USB_HID_SCLASS_NONE;
    psHIDDevice->ui8Protocol = USB_HID_PROTOCOL_NONE;
    psHIDDevice->ui8NumInputReports = 1;
    psHIDDevice->psReportIdle = &psInst->sReportIdle;
    psHIDDevice->pfnRxCallback = HIDCustomHidRxHandler;
    psHIDDevice->pvRxCBData = (void *)psDevice;
    psHIDDevice->pfnTxCallback = HIDCustomHidTxHandler;
    psHIDDevice->pvTxCBData = (void *)psDevice;
#ifdef OUT_EP
    psHIDDevice->bUseOutEndpoint = true;
#else
    psHIDDevice->bUseOutEndpoint = false;
#endif
    psHIDDevice->psHIDDescriptor = &g_sNirscanNanoDescriptor;
    psHIDDevice->ppui8ClassDescriptors= g_pui8NirscanNanoClassDescriptors;
    psHIDDevice->ppui8StringDescriptors = psDevice->ppui8StringDescriptors;
    psHIDDevice->ui32NumStringDescriptors = psDevice->ui32NumStringDescriptors;
    psHIDDevice->ppsConfigDescriptor = g_ppsHIDConfigDescriptors;

    //
    // Initialize the lower layer HID driver and pass it the various structures
    // and descriptors necessary to declare that we are a keyboard.
    //
    return(USBDHIDCompositeInit(ulIndex, psHIDDevice, 0));
}

//*****************************************************************************
//
//! Shuts down the HID customhid device.
//!
//! \param pvInstance is the pointer to the device instance structure.
//!
//! This function terminates HID customhid operation for the instance supplied
//! and removes the device from the USB bus.  Following this call, the \e
//! pvInstance instance may not me used in any other call to the HID customhid
//! device other than USBDHIDCustomHidInit().
//!
//! \return None.
//
//*****************************************************************************
void
USBDHIDCustomHidTerm(void *pvInstance)
{
    tUSBDHIDCustomHidDevice *psDevice;
    tUSBDHIDDevice *psHIDDevice;

    ASSERT(pvInstance);

    //
    // Get a pointer to the device.
    //
    psDevice = (tUSBDHIDCustomHidDevice *)pvInstance;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psDevice->psPrivateHIDCustomHidData->sHIDDevice;

    //
    // Mark our device as no longer configured.
    //
    psDevice->psPrivateHIDCustomHidData->ucUSBConfigured = 0;

    //
    // Terminate the low level HID driver.
    //
    USBDHIDTerm(psHIDDevice);
}

//*****************************************************************************
//
//! Sets the client-specific pointer parameter for the customhid callback.
//!
//! \param pvInstance is the pointer to the customhid device instance structure.
//! \param pvCBData is the pointer that client wishes to be provided on each
//! event sent to the customhid callback function.
//!
//! The client uses this function to change the callback pointer passed in
//! the first parameter on all callbacks to the \e pfnCallback function
//! passed on USBDHIDCustomHidInit().
//!
//! If a client wants to make runtime changes in the callback pointer, it must
//! ensure that the pvInstance structure passed to USBDHIDCustomHidInit() resides
//! in RAM.  If this structure is in flash, callback data changes will not be
//! possible.
//!
//! \return Returns the previous callback pointer that was set for this
//! instance.
//
//*****************************************************************************
void *
USBDHIDCustomHidSetCBData(void *pvInstance, void *pvCBData)
{
    void *pvOldCBData;
    tUSBDHIDCustomHidDevice *psCustomHid;

    //
    // Check for a NULL pointer in the device parameter.
    //
    ASSERT(pvInstance);

    //
    // Get a pointer to our customhid device.
    //
    psCustomHid = (tUSBDHIDCustomHidDevice *)pvInstance;

    //
    // Save the old callback pointer and replace it with the new value.
    //
    pvOldCBData = psCustomHid->pvCBData;
    psCustomHid->pvCBData = pvCBData;

    //
    // Pass the old callback pointer back to the caller.
    //
    return(pvOldCBData);
}

//*****************************************************************************
//
//! Reports a customhid response to the USB host.
//!
//! \param psDevice is the pointer to the customhid device instance structure.
//! \param HIDData[] is the array to be sent to the host.
//! \param numBytes is the number of bytes to be sent from HIDData pointer
//!
//! The return code indicates whether or not the
//! customhid report could be sent to the host.  In cases where a previous
//! report is still being transmitted, \b CUSTOMHID_ERR_TX_ERROR will be returned
//! and the state change will be ignored.
//!
//! \return Returns \b CUSTOMHID_SUCCESS on success, \b CUSTOMHID_ERR_TX_ERROR if an
//! error occurred while attempting to schedule transmission of the customhid
//! report to the host (typically due to a previous report which has not yet
//! completed transmission or due to disconnection of the host) or \b
//! CUSTOMHID_ERR_NOT_CONFIGURED if called before a host has connected to and
//! configured the device.
//
//*****************************************************************************
unsigned long
USBDHIDCustomHidResponse(tUSBDHIDCustomHidDevice *psDevice, signed char HIDData[], int numBytes)
{
    unsigned long ulRetcode;
    unsigned long ulCount;
    int i;
    tHIDCustomHidInstance *psInst;
    tUSBDHIDDevice *psHIDDevice;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psDevice->psPrivateHIDCustomHidData->sHIDDevice;

    //
    // Get a pointer to our instance data
    //
    psInst = psDevice->psPrivateHIDCustomHidData;


    for(i = 0; i < numBytes; i++)
	{
		psInst->pucReport[i] = HIDData[i];; //No ReportID so no offset
	}

    //Round up to next multiple of 64.
    numBytes = (( numBytes + CUSTOMHID_REPORT_SIZE -1 )/CUSTOMHID_REPORT_SIZE) * CUSTOMHID_REPORT_SIZE;

   	//
    // If we are not configured, return an error here before trying to send
    // anything.
    //
    if(!psInst->ucUSBConfigured)
    {
        return(CUSTOMHID_ERR_NOT_CONFIGURED);
    }

    //Wait for previous TX to complete
    while(psInst->eCustomHidState != HID_CUSTOMHID_STATE_IDLE);	//Wait

    //
    // Only send a report if the transmitter is currently free.
    //
    if(USBDHIDTxPacketAvailable((void *)psHIDDevice))
    {
        //
        // Send the report to the host.
        //
        psInst->eCustomHidState = HID_CUSTOMHID_STATE_SEND;
        ulCount = USBDHIDReportWrite((void *)psHIDDevice, psInst->pucReport, numBytes, true);

        //
        // Did we schedule a packet for transmission correctly?
        //
        if(!ulCount)
        {
            //
            // No - report the error to the caller.
            //
            ulRetcode = CUSTOMHID_ERR_TX_ERROR;
        }
        else
        {
            ulRetcode = CUSTOMHID_SUCCESS;
        }
    }
    else
    {
        ulRetcode = CUSTOMHID_ERR_TX_ERROR;
    }
    //
    // Return the relevant error code to the caller.
    //
    return(ulRetcode);
}

//*****************************************************************************
//
//! Reports the device power status (bus- or self-powered) to the USB library.
//!
//! \param pvInstance is the pointer to the customhid device instance structure.
//! \param ucPower indicates the current power status, either \b
//! USB_STATUS_SELF_PWR or \b USB_STATUS_BUS_PWR.
//!
//! Applications which support switching between bus- or self-powered
//! operation should call this function whenever the power source changes
//! to indicate the current power status to the USB library.  This information
//! is required by the USB library to allow correct responses to be provided
//! when the host requests status from the device.
//!
//! \return None.
//
//*****************************************************************************
void
USBDHIDCustomHidPowerStatusSet(void *pvInstance, unsigned char ucPower)
{
    tUSBDHIDCustomHidDevice *psDevice;
    tUSBDHIDDevice *psHIDDevice;

    ASSERT(pvInstance);

    //
    // Get the keyboard device pointer.
    //
    psDevice = (tUSBDHIDCustomHidDevice *)pvInstance;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psDevice->psPrivateHIDCustomHidData->sHIDDevice;

    //
    // Pass the request through to the lower layer.
    //
    USBDHIDPowerStatusSet((void *)psHIDDevice, ucPower);
}

//*****************************************************************************
//
//! Requests a remote wake up to resume communication when in suspended state.
//!
//! \param pvInstance is the pointer to the customhid device instance structure.
//!
//! When the bus is suspended, an application which supports remote wake up
//! (advertised to the host via the configuration descriptor) may call this
//! function to initiate remote wake up signaling to the host.  If the remote
//! wake up feature has not been disabled by the host, this will cause the bus
//! to resume operation within 20mS.  If the host has disabled remote wake up,
//! \b false will be returned to indicate that the wake up request was not
//! successful.
//!
//! \return Returns \b true if the remote wake up is not disabled and the
//! signaling was started or \b false if remote wake up is disabled or if
//! signaling is currently ongoing following a previous call to this function.
//
//*****************************************************************************
bool
USBDHIDCustomHidRemoteWakeupRequest(void *pvInstance)
{
    tUSBDHIDCustomHidDevice *psDevice;
    tUSBDHIDDevice *psHIDDevice;

    ASSERT(pvInstance);

    //
    // Get the keyboard device pointer.
    //
    psDevice = (tUSBDHIDCustomHidDevice *)pvInstance;

    //
    // Get a pointer to the HID device data.
    //
    psHIDDevice = &psDevice->psPrivateHIDCustomHidData->sHIDDevice;

    //
    // Pass the request through to the lower layer.
    //
    return(USBDHIDRemoteWakeupRequest((void *)&psHIDDevice));
}


//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
