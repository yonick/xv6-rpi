// usb interface
#ifndef __USB_H__
#define __USB_H__

#define MAX_EPS 	16 	// we can have at most 16 endpoints for each direction
						// ep0 is bi-directional
enum {
	/* device states */
	Dconfig = 0, /* configuration in progress */
	Denabled, /* address assigned */
	Ddetach, /* device is detached */
	Dreset, /* its port is being reset */

	/* Speeds */
	Fullspeed = 0,
	Lowspeed,
	Highspeed,
	Nospeed,

};

// endpoints
struct usb_ep {
	struct usb_dev 	    *dev;
	int					id;			/* endpoint number*/
	int					maxpkt;		/* maximum packet size */
	int					ttype;		/* transfer type */
	int					dir;		/* endpoint direction: IN/OUT*/
	int					toggle;		/* toggle, USB uses 1 bit for sequencing*/
	int					pollival;	/* poll interval for interrupt device*/
};

// device
struct usb_dev {
    int       			id;			/* assigned by HC*/
	int					state;		/* state for the device */
	int					ishub;		/* hubs can allocate devices */
	int					isroot;		/* is a root hub */
	int					speed;		/* Full/Low/High/No -speed */
	int					hub;		/* dev number for the parent hub */
	int					port;		/* port number in the parent hub */

    struct usb_ep		*ep_in[MAX_EPS];
    struct usb_ep		*ep_out[MAX_EPS];

    void				*private;	/* private variable for the driver*/
};

#define usbprt(fmt, args...) cprintf("usb: "fmt, ##args)

#endif
