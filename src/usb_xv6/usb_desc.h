/*
 * References: USB 2.0 Spec, USB in a Nutshell, USB Overview by Silicon Lab
 */

/* USB Descriptors - each USB device has a hierachy of descriptors to convern
 * its setting: 
 *    device
 *        configuration*
 *              interface*
 *                 endpoint*
 *
 * The configuration structures refer to the USB Configurations that will be
 * made available to a USB HOST during the enumeration process.
 *
 * The USB HOST will select a configuration and optionally an interface with
 * the usb set configuration and set interface commands.
 *
 * The selected interface (or the default interface if not specifically
 * selected) will define the list of endpoints that will be used.
 *
 * The configuration and interfaces are stored in an array that is indexed
 * by the specified configuratin or interface number minus one.
 *
 * A configuration number of zero is used to specify a return to the unconfigured
 * state.
 *
 */


#ifndef __USB_DESC_H__
#define __USB_DESC_H__

#define OUT         0x00
#define IN          0x80

#define CONTROL		0x00
#define ISOCHRONOUS	0x01
#define BULK		0x02
#define INTERRUPT	0x03

/*
 * standard usb descriptor structures
 */
 
/* All standard descriptors have these 2 fields in common */
struct usb_desc_hdr {
	uint8       bLength;
	uint8       bDescriptorType;
} __attribute__ ((packed));

/* endpoint descriptor, type: 0x05*/
struct usb_ep_desc {
	uint8       bLength;
	uint8       bDescriptorType;
	uint8       bEndpointAddress;
	uint8       bmAttributes;
	uint16      wMaxPacketSize;
	uint8       bInterval;
} __attribute__ ((packed));

/* interface descriptor, type: 0x04*/
struct usb_iface_desc {
	uint8       bLength;
	uint8       bDescriptorType;
	uint8       bInterfaceNumber;
	uint8       bAlternateSetting;
	uint8       bNumEndpoints;
	uint8       bInterfaceClass;
	uint8       bInterfaceSubClass;
	uint8       bInterfaceProtocol;
	uint8       iInterface;
} __attribute__ ((packed));

/* string descriptor, type: 0x03*/
struct usb_str_desc {
	uint8       bLength;
	uint8       bDescriptorType;
	uint16      wData[0];
} __attribute__ ((packed));

/* configuration descriptor, type: 0x02*/
struct usb_cfg_desc {
	uint8       bLength;
	uint8       bDescriptorType;
	uint16      wTotalLength;
	uint8       bNumInterfaces;
	uint8       bConfigurationValue;
	uint8       iConfiguration;
	uint8       bmAttributes;
	uint8       bMaxPower;
} __attribute__ ((packed));

/* device descriptor, type: 0x01*/
struct usb_dev_desc {
	uint8       bLength;
	uint8       bDescriptorType;	/* 0x01 */
	uint16      bcdUSB;
	uint8       bDeviceClass;
	uint8       bDeviceSubClass;
	uint8       bDeviceProtocol;
	uint8       bMaxPacketSize0;
	uint16      idVendor;
	uint16      idProduct;
	uint16      bcdDevice;
	uint8       iManufacturer;
	uint8       iProduct;
	uint8       iSerialNumber;
	uint8       bNumConfigurations;
} __attribute__ ((packed));


// for high-speed
struct usb_qualifier_desc {
	uint8       bLength;
	uint8       bDescriptorType;
	uint16      bcdUSB;
	uint8       bDeviceClass;
	uint8       bDeviceSubClass;
	uint8       bDeviceProtocol;
	uint8       bMaxPacketSize0;
	uint8       bNumConfigurations;
	uint8       breserved;
} __attribute__ ((packed));


/*
 * HID class descriptor structures
 *
 * c.f. HID 6.2.1
 */

struct usb_hid_desc {
    uint8	    bLength;
    uint8	    bDescriptorType;
    uint16	    bcdCDC;
    uint8	    bCountryCode;
    uint8	    bNumDescriptors;	/* 0x01 */
    uint8	    bDescriptorType0;
    uint16	    wDescriptorLength0;
    /* optional descriptors are not supported. */
} __attribute__((packed));


/* Hub descriptor */
struct usb_hub_descriptor {
	uint8       bLength;
	uint8       bDescriptorType;
	uint8       bNbrPorts;
	uint16      wHubCharacteristics;
	uint8       bPwrOn2PwrGood;
	uint8       bHubContrCurrent;
	uint8       DeviceRemovable[2];
	uint8       PortPowerCtrlMask[2];
	/* DeviceRemovable and PortPwrCtrlMask want to be variable-length
     bitmaps that hold max 255 entries. (bit0 is ignored) */
} __attribute__ ((packed));


struct usb_port_status {
	uint16      wPortStatus;
	uint16      wPortChange;
} __attribute__ ((packed));


struct usb_hub_status {
	uint16      wHubStatus;
	uint16      wHubChange;
} __attribute__ ((packed));
#endif
