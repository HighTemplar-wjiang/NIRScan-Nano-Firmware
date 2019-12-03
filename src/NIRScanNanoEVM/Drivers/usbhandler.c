/*
 * Copyright (c) 2014-2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 */

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/gates/GateMutex.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <stdbool.h>
#include <stdint.h>
/* driverlib Header files */
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
/* usblib Header files */
#include <usblib/usb-ids.h>
#include <usblib/usblib.h>
#include <usblib/usbhid.h>
#include "driverlib/usb.h"
#include <usblib/device/usbdevice.h>
#include <usblib/device/usbdhid.h>
#include "usbdhidcustom.h"
#include "usbCmdHandler.h"
#include "usbhandler.h"

#if defined(TIVAWARE)
typedef uint32_t            USBMDEventType;
#else
#define eUSBModeForceDevice USB_MODE_FORCE_DEVICE
typedef unsigned long       USBMDEventType;
#endif

#if 0
static GateMutex_Handle gateMouse;
static GateMutex_Handle gateUSBWait;
static Semaphore_Handle semMouse;
static Semaphore_Handle semUSBConnected;
#endif

tUSBDHIDDevice dlpDevice;

/* Function prototypes */
static USBMDEventType cbUSBEvent(void *cbData, USBMDEventType event,
									unsigned int eventMsg,
                                     void *eventMsgPtr);

/* The languages supported by this device. */
const unsigned char langDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

/* The manufacturer string. */
const unsigned char manufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'T', 0, 'e', 0, 'x', 0, 'a', 0, 's', 0, ' ', 0, 'I', 0, 'n', 0, 's', 0,
    't', 0, 'r', 0, 'u', 0, 'm', 0, 'e', 0, 'n', 0, 't', 0, 's', 0,
};

/* The product string. */
const unsigned char productString[] =
{
    (12 + 1) * 2,
    USB_DTYPE_STRING,
    'N', 0, 'i', 0, 'r', 0, 's', 0, 'c', 0, 'a', 0, 'n', 0, ' ', 0, 'N', 0,
    'a', 0, 'n', 0, 'o', 0
};

/* The serial number string. */
const unsigned char serialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0, '8', 0
};

/* The interface description string. */
const unsigned char hidInterfaceString[] =
{
    (21 + 1) * 2,
    USB_DTYPE_STRING,
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'C', 0, 'o', 0, 'm', 0, 'm', 0, 'a', 0,
    'n', 0, 'd', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0,
    'f', 0, 'a', 0, 'c', 0, 'e', 0
};

/* The configuration description string. */
const unsigned char configString[] =
{
    (26 + 1) * 2,
    USB_DTYPE_STRING,
    'H', 0, 'I', 0, 'D', 0, ' ', 0, 'N', 0, 'I', 0, 'R', 0, ' ', 0, 'n', 0,
    'a', 0, 'n', 0, 'o', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0, 'i', 0,
    'g', 0, 'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0
};

/* The descriptor string table. */
const unsigned char * const stringDescriptors[] =
{
    langDescriptor,
    manufacturerString,
    productString,
    serialNumberString,
    hidInterfaceString,
    configString
};

#define STRINGDESCRIPTORSCOUNT (sizeof(stringDescriptors) / \
                                sizeof(unsigned char *))

static tHIDCustomHidInstance deviceInstance;
tUSBDHIDCustomHidDevice NirscanNanoDevice =
{
	0x0451,  // Vendor ID
    0x4200,  // Product ID
    500,
    USB_CONF_ATTR_SELF_PWR | USB_CONF_ATTR_RWAKE,
    cbUSBEvent,
    NULL,
    stringDescriptors,
    STRINGDESCRIPTORSCOUNT,
    &deviceInstance
};

/*
 *  ======== cbUSBEvent ========
 *  Callback handler for the USB stack.
 *
 *  Callback handler called by the USB stack
 *
 *  @param(cbData)          A callback pointer provided by the client.
 *
 *  @param(event)           Identifies the event that occurred in regards to
 *                          this device.
 *
 *  @param(eventMsgData)    A data value associated with a particular event.
 *
 *  @param(eventMsgPtr)     A data pointer associated with a particular event.
 *
 */
static USBMDEventType cbUSBEvent (void *cbData, USBMDEventType event,
                                      unsigned int eventMsgData,
                                      void *eventMsgPtr)
{

    /* Determine what event has happened */
    switch (event) {
        case USB_EVENT_CONNECTED:
        	usbConn();
            break;

        case USB_EVENT_DISCONNECTED:
        	usbDisc();
            break;

        case USB_EVENT_TX_COMPLETE:
            break;

        case USBD_HID_EVENT_SET_REPORT:
        case USB_EVENT_RX_AVAILABLE:
        	cmdRecv(eventMsgPtr, eventMsgData);
        	break;

        default:
            break;
    }

    return (0);
}

/*
 *  ======== USB_hwiHandler ========
 *  This function calls the USB library's device interrupt handler.
 */
void USB_hwiHandler()
{
    USB0DeviceIntHandler();
}


/*
 *  ======== USBHandler_init ========
 */
void USBHandler_init(void)
{
    /* Set the USB stack mode to Device mode with VBUS monitoring */
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    /*
     * Pass our device information to the USB HID device class driver,
     * initialize the USB controller and connect the device to the bus.
     */
    if (!USBDHIDNirscanNanoInit(0, &NirscanNanoDevice)) {
        System_abort("Error initializing USB Handler");
    }

}


