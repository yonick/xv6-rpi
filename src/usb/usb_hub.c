/****************************************************************************
 * HUB "Driver"
 * Probes device for being a hub and configurate it
 */

#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "usb.h"

#ifdef DEBUG
#define USB_DEBUG	1
#define USB_HUB_DEBUG	1
#else
#define USB_DEBUG	0
#define USB_HUB_DEBUG	0
#endif

#define USB_cprintf(fmt, args...)	cprintf("USB: "fmt, ##args)
#define USB_HUB_cprintf(fmt, args...)	cprintf("USB Hub: "fmt, ##args)

#define USB_BUFSIZ	512

static struct usb_hub_device hub_dev[USB_MAX_HUB];
static int usb_hub_index;


static int usb_get_hub_descriptor(struct usb_device *dev, void *data, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
                           USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
                           USB_DT_HUB << 8, 0, data, size, USB_CNTL_TIMEOUT);
}

static int usb_clear_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                           USB_REQ_CLEAR_FEATURE, USB_RT_PORT, feature,
                           port, NULL, 0, USB_CNTL_TIMEOUT);
}

static int usb_set_port_feature(struct usb_device *dev, int port, int feature)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
                           USB_REQ_SET_FEATURE, USB_RT_PORT, feature,
                           port, NULL, 0, USB_CNTL_TIMEOUT);
}

static int usb_get_hub_status(struct usb_device *dev, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
                           USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
                           data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}

static int usb_get_port_status(struct usb_device *dev, int port, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
                           USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port,
                           data, sizeof(struct usb_hub_status), USB_CNTL_TIMEOUT);
}


static void usb_hub_power_on(struct usb_hub_device *hub)
{
	int i;
	struct usb_device *dev;
	unsigned pgood_delay = hub->desc.bPwrOn2PwrGood * 2;

	dev = hub->pusb_dev;
    
	/* Enable power to the ports */
	USB_HUB_cprintf("enabling power on all ports\n");
    
	for (i = 0; i < dev->maxchild; i++) {
		usb_set_port_feature(dev, i + 1, USB_PORT_FEAT_POWER);
		USB_HUB_cprintf("port %d returns %x\n", i + 1, dev->status);
	}

	/* Wait at least 100 msec for power to become stable */
	micro_delay(UMAX(pgood_delay, (unsigned)100));
}

void usb_hub_reset(void)
{
	usb_hub_index = 0;
}

static struct usb_hub_device *usb_hub_allocate(void)
{
	if (usb_hub_index < USB_MAX_HUB)
		return &hub_dev[usb_hub_index++];

	cprintf("ERROR: USB_MAX_HUB (%d) reached\n", USB_MAX_HUB);
	return NULL;
}

#define MAX_TRIES 5

static inline char *portspeed(int portstatus)
{
	if (portstatus & (1 << USB_PORT_FEAT_HIGHSPEED))
		return "480 Mb/s";
	else if (portstatus & (1 << USB_PORT_FEAT_LOWSPEED))
		return "1.5 Mb/s";
	else
		return "12 Mb/s";
}

int hub_port_reset(struct usb_device *dev, int port,
                   unsigned short *portstat)
{
	int tries;
	struct usb_port_status *portsts;
	unsigned short portstatus, portchange;

    portsts = kmalloc (get_order(sizeof(*portsts)));

    if (portsts == NULL) {
        return -1;
    }

	USB_HUB_cprintf("hub_port_reset: resetting port %d...\n", port);
    
	for (tries = 0; tries < MAX_TRIES; tries++) {
		usb_set_port_feature(dev, port + 1, USB_PORT_FEAT_RESET);
		micro_delay(200);

		if (usb_get_port_status(dev, port + 1, portsts) < 0) {
			USB_HUB_cprintf("get_port_status failed status %x\n", dev->status);
			kfree (portsts, get_order(sizeof(*portsts)));
			return -1;
		}
        
		portstatus = le16_to_cpu(portsts->wPortStatus);
		portchange = le16_to_cpu(portsts->wPortChange);

		USB_HUB_cprintf("portstatus %x, change %x, %s\n",
                        portstatus, portchange,
                        portspeed(portstatus));

		USB_HUB_cprintf("STAT_C_CONNECTION = %d STAT_CONNECTION = %d" \
                        "  USB_PORT_STAT_ENABLE %d\n",
                        (portchange & USB_PORT_STAT_C_CONNECTION) ? 1 : 0,
                        (portstatus & USB_PORT_STAT_CONNECTION) ? 1 : 0,
                        (portstatus & USB_PORT_STAT_ENABLE) ? 1 : 0);

		if ((portchange & USB_PORT_STAT_C_CONNECTION) ||
		    !(portstatus & USB_PORT_STAT_CONNECTION)) {
		    kfree (portsts, get_order(sizeof(*portsts)));
			return -1;
		}

		if (portstatus & USB_PORT_STAT_ENABLE) {
			break;
        }
        
		micro_delay(200);
	}

	if (tries == MAX_TRIES) {
		USB_HUB_cprintf("Cannot enable port %i after %i retries, " \
                        "disabling port.\n", port + 1, MAX_TRIES);
		USB_HUB_cprintf("Maybe the USB cable is bad?\n");
        
		kfree (portsts, get_order(sizeof(*portsts)));
		return -1;
	}

	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_RESET);
	*portstat = portstatus;
	kfree (portsts, get_order(sizeof(*portsts)));
	return 0;
}


void usb_hub_port_connect_change(struct usb_device *dev, int port)
{
	struct usb_device *usb;
	struct usb_port_status *portsts;
	unsigned short portstatus;

    portsts = kmalloc (get_order(sizeof(*portsts)));

	/* Check status */
	if (usb_get_port_status(dev, port + 1, portsts) < 0) {
		USB_HUB_cprintf("get_port_status failed\n");
		kfree (portsts, get_order(sizeof(*portsts)));
		return;
	}

	portstatus = le16_to_cpu(portsts->wPortStatus);
	USB_HUB_cprintf("portstatus %x, change %x, %s\n",
                    portstatus,
                    le16_to_cpu(portsts->wPortChange),
                    portspeed(portstatus));

	/* Clear the connection change status */
	usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_C_CONNECTION);

	/* Disconnect any existing devices under this port */
	if (((!(portstatus & USB_PORT_STAT_CONNECTION)) &&
	     (!(portstatus & USB_PORT_STAT_ENABLE))) || (dev->children[port])) {
        
		USB_HUB_cprintf("usb_disconnect(&hub->children[port]);\n");
        
		/* Return now if nothing is connected */
		if (!(portstatus & USB_PORT_STAT_CONNECTION)) {
		    kfree (portsts, get_order(sizeof(*portsts)));
			return;
		}
	}
    
	micro_delay(200);

	/* Reset the port */
	if (hub_port_reset(dev, port, &portstatus) < 0) {
		cprintf("cannot reset port %i!?\n", port + 1);
		kfree (portsts, get_order(sizeof(*portsts)));
		return;
	}

	micro_delay(200);

	/* Allocate a new device struct for it */
	usb = usb_alloc_new_device(dev->controller);

	if (portstatus & USB_PORT_STAT_HIGH_SPEED) {
		usb->speed = USB_SPEED_HIGH;
	} else if (portstatus & USB_PORT_STAT_LOW_SPEED) {
		usb->speed = USB_SPEED_LOW;
	} else {
		usb->speed = USB_SPEED_FULL;
    }

	dev->children[port] = usb;
	usb->parent = dev;
	usb->portnr = port + 1;
    
	/* Run it through the hoops (find a driver, etc) */
	if (usb_new_device(usb)) {
		/* Woops, disable the port */
		USB_HUB_cprintf("hub: disabling port %d\n", port + 1);
		usb_clear_port_feature(dev, port + 1, USB_PORT_FEAT_ENABLE);
	}
}


static int usb_hub_configure(struct usb_device *dev)
{
	int i;
	unsigned char *buffer;
	unsigned char *bitmap;
	short hubCharacteristics;
	struct usb_hub_descriptor *descriptor;
	struct usb_hub_device *hub;
#ifdef USB_HUB_DEBUG
	struct usb_hub_status *hubsts;
#endif

    buffer = kmalloc (get_order(USB_BUFSIZ));
    if (buffer == NULL) {
        return -1;
    }

	/* "allocate" Hub device */
	hub = usb_hub_allocate();
    
	if (hub == NULL) {
	    kfree (buffer, get_order(USB_BUFSIZ));
		return -1;
	}

	hub->pusb_dev = dev;
    
	/* Get the the hub descriptor */
	if (usb_get_hub_descriptor(dev, buffer, 4) < 0) {
		USB_HUB_cprintf("usb_hub_configure: failed to get hub " \
                        "descriptor, giving up %x\n", dev->status);
        kfree (buffer, get_order(USB_BUFSIZ));

		return -1;
	}
    
	descriptor = (struct usb_hub_descriptor *)buffer;

	/* silence compiler warning if USB_BUFSIZ is > 256 [= sizeof(char)] */
	i = descriptor->bLength;
	if (i > USB_BUFSIZ) {
		USB_HUB_cprintf("usb_hub_configure: failed to get hub " \
                        "descriptor - too long: %d\n",
                        descriptor->bLength);
        kfree (buffer, get_order(USB_BUFSIZ));

		return -1;
	}

	if (usb_get_hub_descriptor(dev, buffer, descriptor->bLength) < 0) {
		USB_HUB_cprintf("usb_hub_configure: failed to get hub " \
                        "descriptor 2nd giving up %x\n", dev->status);
        kfree (buffer, get_order(USB_BUFSIZ));

		return -1;
	}
    
	memcpy((unsigned char *)&hub->desc, buffer, descriptor->bLength);
    
	/* adjust 16bit values */
	hub->desc.wHubCharacteristics = descriptor->wHubCharacteristics;
    
	/* set the bitmap */
	bitmap = (unsigned char *)&hub->desc.DeviceRemovable[0];
    
	/* devices not removable by default */
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8);
	bitmap = (unsigned char *)&hub->desc.PortPowerCtrlMask[0];
	memset(bitmap, 0xff, (USB_MAXCHILDREN+1+7)/8); /* PowerMask = 1B */

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++) {
		hub->desc.DeviceRemovable[i] = descriptor->DeviceRemovable[i];
    }

	for (i = 0; i < ((hub->desc.bNbrPorts + 1 + 7)/8); i++) {
		hub->desc.PortPowerCtrlMask[i] = descriptor->PortPowerCtrlMask[i];
    }

	dev->maxchild = descriptor->bNbrPorts;
	USB_HUB_cprintf("%d ports detected\n", dev->maxchild);

	hubCharacteristics = hub->desc.wHubCharacteristics;
	switch (hubCharacteristics & HUB_CHAR_LPSM) {
    case 0x00:
        USB_HUB_cprintf("ganged power switching\n");
        break;
    case 0x01:
        USB_HUB_cprintf("individual port power switching\n");
        break;
    case 0x02:
    case 0x03:
        USB_HUB_cprintf("unknown reserved power switching mode\n");
        break;
	}

	if (hubCharacteristics & HUB_CHAR_COMPOUND)
		USB_HUB_cprintf("part of a compound device\n");
	else
		USB_HUB_cprintf("standalone hub\n");

	switch (hubCharacteristics & HUB_CHAR_OCPM) {
    case 0x00:
        USB_HUB_cprintf("global over-current protection\n");
        break;
    case 0x08:
        USB_HUB_cprintf("individual port over-current protection\n");
        break;
    case 0x10:
    case 0x18:
        USB_HUB_cprintf("no over-current protection\n");
        break;
	}

	USB_HUB_cprintf("power on to power good time: %dms\n",
                    descriptor->bPwrOn2PwrGood * 2);
	USB_HUB_cprintf("hub controller current requirement: %dmA\n",
                    descriptor->bHubContrCurrent);

	for (i = 0; i < dev->maxchild; i++) {
		USB_HUB_cprintf("port %d is%s removable\n", i + 1,
                        hub->desc.DeviceRemovable[(i + 1) / 8] & \
                        (1 << ((i + 1) % 8)) ? " not" : "");
    }
    
	if (sizeof(struct usb_hub_status) > USB_BUFSIZ) {
		USB_HUB_cprintf("usb_hub_configure: failed to get Status - " \
                        "too long: %d\n", descriptor->bLength);
        
        kfree (buffer, get_order(USB_BUFSIZ));

		return -1;
	}

	if (usb_get_hub_status(dev, buffer) < 0) {
		USB_HUB_cprintf("usb_hub_configure: failed to get Status %lX\n",
                        dev->status);
        kfree (buffer, get_order(USB_BUFSIZ));

		return -1;
	}

#ifdef USB_HUB_DEBUG
	hubsts = (struct usb_hub_status *)buffer;
#endif
	USB_HUB_cprintf("get_hub_status returned status %x, change %x\n",
                    le16_to_cpu(hubsts->wHubStatus),
                    le16_to_cpu(hubsts->wHubChange));
	USB_HUB_cprintf("local power source is %s\n",
                    (le16_to_cpu(hubsts->wHubStatus) & HUB_STATUS_LOCAL_POWER) ? \
                    "lost (inactive)" : "good");
	USB_HUB_cprintf("%sover-current condition exists\n",
                    (le16_to_cpu(hubsts->wHubStatus) & HUB_STATUS_OVERCURRENT) ? \
                    "" : "no ");
	usb_hub_power_on(hub);

	for (i = 0; i < dev->maxchild; i++) {
		struct usb_port_status *portsts;
		unsigned short portstatus, portchange;

        portsts = kmalloc (get_order(sizeof(*portsts)));
		if (usb_get_port_status(dev, i + 1, portsts) < 0) {
			USB_HUB_cprintf("get_port_status failed\n");
			continue;
		}

		portstatus = (portsts->wPortStatus);
		portchange = (portsts->wPortChange);
		USB_HUB_cprintf("Port %d Status %x Change %x\n",
                        i + 1, portstatus, portchange);

		if (portchange & USB_PORT_STAT_C_CONNECTION) {
			USB_HUB_cprintf("port %d connection change\n", i + 1);
			usb_hub_port_connect_change(dev, i);
		}
        
		if (portchange & USB_PORT_STAT_C_ENABLE) {
			USB_HUB_cprintf("port %d enable change, status %x\n",
                            i + 1, portstatus);
			usb_clear_port_feature(dev, i + 1,
                                   USB_PORT_FEAT_C_ENABLE);

			/* EM interference sometimes causes bad shielded USB
			 * devices to be shutdown by the hub, this hack enables
			 * them again. Works at least with mouse driver */
			if (!(portstatus & USB_PORT_STAT_ENABLE) &&
                (portstatus & USB_PORT_STAT_CONNECTION) &&
                ((dev->children[i]))) {
				USB_HUB_cprintf("already running port %d disabled by hub (EMI?), re-enabling...\n", i + 1);
                usb_hub_port_connect_change(dev, i);
			}
		}
        
		if (portstatus & USB_PORT_STAT_SUSPEND) {
			USB_HUB_cprintf("port %d suspend change\n", i + 1);
			usb_clear_port_feature(dev, i + 1,
                                   USB_PORT_FEAT_SUSPEND);
		}

		if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
			USB_HUB_cprintf("port %d over-current change\n", i + 1);
			usb_clear_port_feature(dev, i + 1,
                                   USB_PORT_FEAT_C_OVER_CURRENT);
			usb_hub_power_on(hub);
		}
        
		if (portchange & USB_PORT_STAT_C_RESET) {
			USB_HUB_cprintf("port %d reset change\n", i + 1);
			usb_clear_port_feature(dev, i + 1,
                                   USB_PORT_FEAT_C_RESET);
		}
	} /* end for i all ports */
    
    kfree (buffer, get_order(USB_BUFSIZ));
    
	return 0;
}

int usb_hub_probe(struct usb_device *dev, int ifnum)
{
	struct usb_interface *iface;
	struct usb_endpoint_descriptor *ep;
	int ret;

	iface = &dev->config.if_desc[ifnum];
	/* Is it a hub? */
	if (iface->desc.bInterfaceClass != USB_CLASS_HUB) {
		return 0;
    }
    
	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if ((iface->desc.bInterfaceSubClass != 0) &&
	    (iface->desc.bInterfaceSubClass != 1)) {
		return 0;
    }
    
	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if (iface->desc.bNumEndpoints != 1) {
		return 0;
    }
    
	ep = &iface->ep_desc[0];
    
	/* Output endpoint? Curiousier and curiousier.. */
	if (!(ep->bEndpointAddress & USB_DIR_IN)) {
		return 0;
    }
    
	/* If it's not an interrupt endpoint, we'd better punt! */
	if ((ep->bmAttributes & 3) != 3) {
		return 0;
    }
    
	/* We found a hub */
	USB_HUB_cprintf("USB hub found\n");
	ret = usb_hub_configure(dev);
	return ret;
}
