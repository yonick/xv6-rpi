// usb interface
#ifndef __USB_H__
#define __USB_H__

struct usb_device {
    uint8       		dev_num;
    struct usb_dev_desc *desc;
    void				*private;
};

#define usbprt(fmt, args...) cprintf("usb: "fmt, ##args)

#endif
