// usb bus handling
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "dwc_regs.h"
#include "usb.h"
#include "usb_desc.h"


// probe the current device, the current device always has address 0
struct usb_device* usb_probe ()
{
	struct usb_dev	dev;
	struct usb_ep	ep0;

	struct usb_setup *req;
	struct usb_dev_desc *d;
	int ord;

	ord = get_order (sizeof (*req) + sizeof (struct usb_dev_desc));
	req = kmalloc (ord);
	if (req == NULL) {
		panic("usb_probe: kmalloc\n");
	}

	req->bmRequestType = EP_IN; // standard in request to device
	req->bRequest = Rgetdesc;
	req->wValue = Ddevice << 8;
	req->wIndex = 0;
	req->wLength = sizeof (struct usb_dev_desc);

	memset (&dev, 0, sizeof (dev));
	memset (&ep0, 0, sizeof (ep0));

	ep0.dev = &dev;
	ep0.maxpkt = 0;
	ep0.ttype = EP_CTRL;

	ep_write (&ep0, req, sizeof(*req)+sizeof(struct usb_dev_desc));
	d = (struct usb_dev_desc*)req->data;

	usbprt ("type - %d, bcdUSB - %x, class/subclass/proto:%d, %d, %d\n",
			d->bDescriptorType, d->bcdUSB, d->bDeviceClass, d->bDeviceSubClass,
			d->bDeviceProtocol);
}

void usb_init ()
{
	usb_inithcd ();
	reset_hcport ();
	usb_probe ();
}
