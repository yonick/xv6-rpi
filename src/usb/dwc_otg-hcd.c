#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "usb.h"
#include "dwc_otg.h"
#include "dwc_otg_regs.h"
#include "dwc_otg_core_if.h"

#define STATUS_ACK_HLT_COMPL        0x23
#define	CHANNEL                     0
#define DWC_OTG_HCD_STATUS_BUF_SIZE 64
#define DWC_OTG_HCD_DATA_BUF_SIZE   (64*1024)
#define MAX_DEVICE                  16
#define MAX_ENDPOINT                8

static uint8 *data_buf;
static uint8 *status_buf;

static dwc_otg_core_if_t    g_core_if;
static int                  root_hub_devnum = 0;

int bulk_data_toggle[MAX_DEVICE][MAX_ENDPOINT];
int control_data_toggle[MAX_DEVICE][MAX_ENDPOINT];

void do_hang(int line, uint32 d)
{
	cprintf ("DWC OTG: hang at line %d: %x\n", line, d);
    panic ("DWC OTG:");
}

void handle_error(int line, uint32 d)
{
	hcint_data_t hcint;
    
	hcint.d32 = d;

	cprintf("Error condition at line %d: ", line);
    
	if (hcint.b.ahberr) {
		cprintf(" AHBERR");
    }

	if (hcint.b.stall) {
		cprintf(" STALL");
    }
    
	if (hcint.b.bblerr) {
		cprintf(" NAK");
    }
    
	if (hcint.b.ack) {
		cprintf(" ACK");
    }
    
	if (hcint.b.nyet) {
		cprintf(" NYET");
    }
    
	if (hcint.b.xacterr) {
		cprintf(" XACTERR");
    }
    
	if (hcint.b.datatglerr) {
		cprintf(" DATATGLERR");
    }
    
	cprintf("\n");
}

/*
 * U-Boot USB interface
 */
int usb_lowlevel_init(int index, void **controller)
{
    int i, j;
    
    // allocate memory buffer, buddy allocator guarantes memory is aligned
	data_buf = (uint8*)kmalloc(get_order(DWC_OTG_HCD_DATA_BUF_SIZE));
	status_buf = (uint8*)kmalloc(get_order(DWC_OTG_HCD_STATUS_BUF_SIZE));
	
	hprt0_data_t hprt0 = {.d32 = 0 };
	root_hub_devnum = 0;

	memset(&g_core_if, 0, sizeof(g_core_if));

    dwc_otg_cil_init(&g_core_if, P2V(0x20980000));

	if ((g_core_if.snpsid & 0xFFFFF000) != 0x4F542000) {
		cprintf("SNPSID is invalid (not DWC/OTG device): %x\n", g_core_if.snpsid);
		return -1;
	}

	dwc_otg_core_init(&g_core_if);
	dwc_otg_core_host_init(&g_core_if);

	hprt0.d32 = dwc_otg_read_hprt0(&g_core_if);
	hprt0.b.prtrst = 1;
	dwc_write_reg32(g_core_if.host_if->hprt0, hprt0.d32);
    
	micro_delay(50000);

    hprt0.b.prtrst = 0;
	dwc_write_reg32(g_core_if.host_if->hprt0, hprt0.d32);

	micro_delay(50000);
	hprt0.d32 = dwc_read_reg32(g_core_if.host_if->hprt0);

    // initialize data toggle, as specified by USB spec
	for (i = 0; i < MAX_DEVICE; i++) {
		for (j = 0; j < MAX_ENDPOINT; j++) {
			control_data_toggle[i][j] = DWC_OTG_HC_PID_DATA1;
			bulk_data_toggle[i][j] = DWC_OTG_HC_PID_DATA0;
		}
	}

	return 0;
}

int usb_lowlevel_stop(int index)
{
	kfree(data_buf, get_order(DWC_OTG_HCD_DATA_BUF_SIZE));
    kfree(status_buf, get_order(DWC_OTG_HCD_STATUS_BUF_SIZE));

	return 0;
}

/* based on usb_ohci.c */
#define min_t(type, x, y) ({ type __x = (x); type __y = (y);\
                            __x < __y ? __x : __y; })

/* Virtual Root Hub */

/* Device descriptor */
static uint8 root_hub_dev_des[] = {
	0x12,	    /*	uint8  bLength; */
	0x01,	    /*	uint8  bDescriptorType; Device */
	0x10,	    /*	uint16 bcdUSB; v1.1 */
	0x01,
	0x09,	    /*	uint8  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*	uint8  bDeviceSubClass; */
	0x00,	    /*	uint8  bDeviceProtocol; */
	0x08,	    /*	uint8  bMaxPacketSize0; 8 Bytes */
	0x00,	    /*	uint16 idVendor; */
	0x00,
	0x00,	    /*	uint16 idProduct; */
	0x00,
	0x00,	    /*	uint16 bcdDevice; */
	0x00,
	0x01,	    /*	uint8  iManufacturer; */
	0x02,	    /*	uint8  iProduct; */
	0x00,	    /*	uint8  iSerialNumber; */
	0x01	    /*	uint8  bNumConfigurations; */
};

/* Configuration descriptor */
static uint8 root_hub_config_des[] = {
	0x09,	    /*	uint8  bLength; */
	0x02,	    /*	uint8  bDescriptorType; Configuration */
	0x19,	    /*	uint16 wTotalLength; */
	0x00,
	0x01,	    /*	uint8  bNumInterfaces; */
	0x01,	    /*	uint8  bConfigurationValue; */
	0x00,	    /*	uint8  iConfiguration; */
	0x40,	    /*	uint8  bmAttributes; */

	0x00,	    /*	uint8  MaxPower; */

	/* interface */
	0x09,	    /*	uint8  if_bLength; */
	0x04,	    /*	uint8  if_bDescriptorType; Interface */
	0x00,	    /*	uint8  if_bInterfaceNumber; */
	0x00,	    /*	uint8  if_bAlternateSetting; */
	0x01,	    /*	uint8  if_bNumEndpoints; */
	0x09,	    /*	uint8  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,	    /*	uint8  if_bInterfaceSubClass; */
	0x00,	    /*	uint8  if_bInterfaceProtocol; */
	0x00,	    /*	uint8  if_iInterface; */

	/* endpoint */
	0x07,	    /*	uint8  ep_bLength; */
	0x05,	    /*	uint8  ep_bDescriptorType; Endpoint */
	0x81,	    /*	uint8  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,	    /*	uint8  ep_bmAttributes; Interrupt */
	0x08,	    /*	uint16 ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
	0x00,
	0xff	    /*	uint8  ep_bInterval; 255 ms */
};

static unsigned char root_hub_str_index0[] = {
	0x04,			/*  uint8  bLength; */
	0x03,			/*  uint8  bDescriptorType; String-descriptor */
	0x09,			/*  uint8  lang ID */
	0x04,			/*  uint8  lang ID */
};

static unsigned char root_hub_str_index1[] = {
	32,			/*  uint8  bLength; */
	0x03,			/*  uint8  bDescriptorType; String-descriptor */
	'D',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'W',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'C',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	' ',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'O',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'T',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'G',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	' ',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'R',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'o',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'o',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	't',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'H',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'u',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
	'b',			/*  uint8  Unicode */
	0,				/*  uint8  Unicode */
};

// submit a root hub message
static int dwc_otg_submit_rh_msg(struct usb_device *dev, unsigned long pipe,
                                 void *buffer, int transfer_len,
                                 struct devrequest *cmd)
{
	int             leni, len, stat;
	uint16          bmRType_bReq;
	uint16          wValue;
	uint16          wLength;
	unsigned char   data[32];
	hprt0_data_t    hprt0;

    leni = transfer_len;
    len = 0;
	stat = 0;
    hprt0.d32 = 0;
    
	if (usb_pipeint(pipe)) {
		cprintf("Root-Hub submit IRQ: NOT implemented");
		return 0;
	}

	bmRType_bReq  = cmd->requesttype | (cmd->request << 8);
	wValue	      = cmd->value;
	wLength	      = cmd->length;

	switch (bmRType_bReq) {
    case RH_GET_STATUS:
        *(uint16 *)buffer = cpu_to_le16(1);
        len = 2;
        break;
        
    case RH_GET_STATUS | RH_INTERFACE:
        *(uint16 *)buffer = cpu_to_le16(0);
        len = 2;
        break;
        
    case RH_GET_STATUS | RH_ENDPOINT:
        *(uint16 *)buffer = cpu_to_le16(0);
        len = 2;
        break;
        
    case RH_GET_STATUS | RH_CLASS:
        *(uint32 *)buffer = 0;
        len = 4;
        break;
    case RH_GET_STATUS | RH_OTHER | RH_CLASS: {
        uint32 port_status = 0;
        uint32 port_change = 0;
        
        hprt0.d32 = dwc_read_reg32(g_core_if.host_if->hprt0);
        
        if (hprt0.b.prtconnsts) {
            port_status |= USB_PORT_STAT_CONNECTION;
        }
        
        if (hprt0.b.prtena) {
            port_status |= USB_PORT_STAT_ENABLE;
        }
        
        if (hprt0.b.prtsusp) {
            port_status |= USB_PORT_STAT_SUSPEND;
        }
        
        if (hprt0.b.prtovrcurract) {
            port_status |= USB_PORT_STAT_OVERCURRENT;
        }
        
        if (hprt0.b.prtrst) {
            port_status |= USB_PORT_STAT_RESET;
        }
        
        if (hprt0.b.prtpwr) {
            port_status |= USB_PORT_STAT_POWER;
        }

        port_status |= USB_PORT_STAT_HIGH_SPEED;

        if (hprt0.b.prtenchng) {
            port_change |= USB_PORT_STAT_C_ENABLE;
        }
        
        if (hprt0.b.prtconndet) {
            port_change |= USB_PORT_STAT_C_CONNECTION;
        }
        
        if (hprt0.b.prtovrcurrchng) {
            port_change |= USB_PORT_STAT_C_OVERCURRENT;
        }
        
        *(uint32 *)buffer = (port_status | (port_change << 16));
        len = 4;
    }
        break;
        
    case RH_CLEAR_FEATURE | RH_ENDPOINT:
    case RH_CLEAR_FEATURE | RH_CLASS:
        break;

    case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
        if (wValue == RH_C_PORT_CONNECTION) {
            hprt0.d32 = dwc_read_reg32(g_core_if.host_if->hprt0);
            hprt0.b.prtconndet = 1;
            dwc_write_reg32(g_core_if.host_if->hprt0, hprt0.d32);
            break;
        }
        
        break;

    case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
        switch (wValue) {
        case (RH_PORT_SUSPEND):
            break;

        case (RH_PORT_RESET):
            hprt0.d32 = dwc_otg_read_hprt0(&g_core_if);
            hprt0.b.prtrst = 1;
            
            dwc_write_reg32(g_core_if.host_if->hprt0, hprt0.d32);
            micro_delay(500);
            hprt0.b.prtrst = 0;
            
            dwc_write_reg32(g_core_if.host_if->hprt0, hprt0.d32);
            micro_delay(500);
            hprt0.d32 = dwc_read_reg32(g_core_if.host_if->hprt0);

            break;

        case (RH_PORT_POWER):
            hprt0.d32 = dwc_otg_read_hprt0(&g_core_if);
            hprt0.b.prtpwr = 1;
            dwc_write_reg32(g_core_if.host_if->hprt0, hprt0.d32);
            break;

        case (RH_PORT_ENABLE):
            break;
        }
        
        break;
        
    case RH_SET_ADDRESS:
        root_hub_devnum = wValue;
        break;
        
    case RH_GET_DESCRIPTOR:
        switch ((wValue & 0xff00) >> 8) {
        case (0x01): /* device descriptor */
            len = min_t(unsigned int, leni,
                        min_t(unsigned int, sizeof(root_hub_dev_des), wLength));
                
            memcpy(buffer, root_hub_dev_des, len);
            break;
            
        case (0x02): /* configuration descriptor */
            len = min_t(unsigned int, leni,
                        min_t(unsigned int, sizeof(root_hub_config_des), wLength));

            memcpy(buffer, root_hub_config_des, len);
            break;
            
        case (0x03): /* string descriptors */
            if (wValue == 0x0300) {
                len = min_t(unsigned int, leni,
                            min_t(unsigned int, sizeof(root_hub_str_index0), wLength));
                
                memcpy(buffer, root_hub_str_index0, len);
            }
            
            if (wValue == 0x0301) {
                len = min_t(unsigned int, leni,
                            min_t(unsigned int,  sizeof(root_hub_str_index1),wLength));
                
                memcpy(buffer, root_hub_str_index1, len);
            }
            break;

        default:
            stat = USB_ST_STALLED;
        }
        break;

    case RH_GET_DESCRIPTOR | RH_CLASS: {
        uint32 temp = 0x00000001;

        data[0] = 9;		/* min length; */
        data[1] = 0x29;
        data[2] = temp & RH_A_NDP;
        data[3] = 0;
        
        if (temp & RH_A_PSM) {
            data[3] |= 0x1;
        }
        
        if (temp & RH_A_NOCP) {
            data[3] |= 0x10;
        } else if (temp & RH_A_OCPM) {
            data[3] |= 0x8;
        }

        /* corresponds to data[4-7] */
        data[5] = (temp & RH_A_POTPGT) >> 24;
        data[7] = temp & RH_B_DR;
        
        if (data[2] < 7) {
            data[8] = 0xff;
        } else {
            data[0] += 2;
            data[8] = (temp & RH_B_DR) >> 8;
            data[10] = data[9] = 0xff;
        }

        len = min_t(unsigned int, leni, min_t(unsigned int, data[0], wLength));
        memcpy(buffer, data, len);
        break;
    }

    case RH_GET_CONFIGURATION:
        *(uint8 *) buffer = 0x01;
        len = 1;
        break;
        
    case RH_SET_CONFIGURATION:
        break;
        
    default:
        cprintf("unsupported root hub command\n");
        stat = USB_ST_STALLED;
	}

	micro_delay(1);

	len = min_t(int, len, leni);

	dev->act_len = len;
	dev->status = stat;

	return stat;

}


// send a bulk message (e.g., for USB mass storage
int submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer, int len)
{
	int                 devnum;
	int                 ep;
	int                 max;
	int                 done;
	hctsiz_data_t       hctsiz;
	dwc_otg_host_if_t   *host_if;
	dwc_otg_hc_regs_t   *hc_regs;
	hcint_data_t        hcint;
	hcint_data_t        hcint_new;
	uint32              max_hc_xfer_size;
	uint16              max_hc_pkt_count;
	uint32              xfer_len;
	uint32              num_packets;
	int                 stop_transfer;
    hcchar_data_t       hcchar;
    
    devnum              = usb_pipedevice(pipe);
    ep                  = usb_pipeendpoint(pipe);
    max                 = usb_maxpacket(dev, pipe);
    done                = 0;
    host_if             = g_core_if.host_if;
    max_hc_xfer_size    = g_core_if.core_params->max_transfer_size;
    max_hc_pkt_count    = g_core_if.core_params->max_packet_count;
    stop_transfer       = 0;


	if (len > DWC_OTG_HCD_DATA_BUF_SIZE) {
		cprintf("submit_bulk_msg: message too big (%d)\n", len);
		dev->status = 0;
		dev->act_len = done;
		return -1;
	}

	hc_regs = host_if->hc_regs[CHANNEL];

	if (devnum == root_hub_devnum) {
		dev->status = 0;
		return -1;
	}

	while ((done < len) && !stop_transfer) {
		/* Initialize channel */
		dwc_otg_hc_init(&g_core_if, CHANNEL, devnum, ep,
                        usb_pipein(pipe), DWC_OTG_EP_TYPE_BULK, max);

		xfer_len = (len - done);
        
		if (xfer_len > max_hc_xfer_size)
            /* Make sure that xfer_len is a multiple of max packet size. */
			xfer_len = max_hc_xfer_size - max + 1;

		if (xfer_len > 0) {
			num_packets = (xfer_len + max - 1) / max;
            
			if (num_packets > max_hc_pkt_count) {
				num_packets = max_hc_pkt_count;
				xfer_len = num_packets * max;
			}
		} else {
			num_packets = 1;
        }

		if (usb_pipein(pipe)) {
			xfer_len = num_packets * max;
        }

		hctsiz.d32 = 0;

		hctsiz.b.xfersize = xfer_len;
		hctsiz.b.pktcnt = num_packets;
		hctsiz.b.pid = bulk_data_toggle[devnum][ep];
		dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);

		memcpy(data_buf, (char*)buffer + done, len - done);
		dwc_write_reg32(&hc_regs->hcdma, (uint32)data_buf);

		hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
		hcchar.b.multicnt = 1;

		/* Remember original int status */
		hcint.d32 = dwc_read_reg32(&hc_regs->hcint);

		/* Set host channel enable after all other setup is complete. */
		hcchar.b.chen = 1;
		hcchar.b.chdis = 0;
		dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);

		/* TODO: no endless loop */
		while(1) {
			hcint_new.d32 = dwc_read_reg32(&hc_regs->hcint);
            
			if (hcint.d32 != hcint_new.d32) {
				hcint.d32 = hcint_new.d32;
			}

			if (hcint_new.b.ack) {
				hctsiz.d32 = dwc_read_reg32(&hc_regs->hctsiz);

				if (hctsiz.b.pid == DWC_OTG_HC_PID_DATA1) {
					bulk_data_toggle[devnum][ep] = DWC_OTG_HC_PID_DATA1;
				} else {
					bulk_data_toggle[devnum][ep] = DWC_OTG_HC_PID_DATA0;
                }
			}

			if (hcint_new.b.chhltd) {
				if (hcint_new.b.xfercomp) {
					hctsiz.d32 = dwc_read_reg32(&hc_regs->hctsiz);

					if (usb_pipein(pipe)) {
						done += xfer_len - hctsiz.b.xfersize;

                        if (hctsiz.b.xfersize) {
							stop_transfer = 1;
                        }
					} else {
						done += xfer_len;
					}

					if (hcint_new.d32 != STATUS_ACK_HLT_COMPL) {
						handle_error(__LINE__, hcint_new.d32);
						goto out;
					}

					break;
                    
				} else if (hcint_new.b.stall) {
					cprintf("DWC OTG: Channel halted\n");
					bulk_data_toggle[devnum][ep] = DWC_OTG_HC_PID_DATA0;
					stop_transfer = 1;
					break;
				}
			}
		}
	}

	if (done && usb_pipein(pipe)) {
		memcpy(buffer, data_buf, done);
    }
    

	dwc_write_reg32(&hc_regs->hcintmsk, 0);
	dwc_write_reg32(&hc_regs->hcint, 0xFFFFFFFF);
    
out:
	dev->status = 0;
	dev->act_len = done;

	return 0;
}

int submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
                       int len,struct devrequest *setup)
{
	hctsiz_data_t       hctsiz;
	dwc_otg_host_if_t   *host_if;
	dwc_otg_hc_regs_t   *hc_regs;
	hcint_data_t        hcint;
	hcint_data_t        hcint_new;
    hcchar_data_t       hcchar;
	int                 status_direction;
    int                 done;
	int                 devnum;
	int                 ep;
	int                 max;
    
    done        = 0;
    devnum      = usb_pipedevice(pipe);
    ep          = usb_pipeendpoint(pipe);
    max         = usb_maxpacket(dev, pipe);
    host_if     = g_core_if.host_if;
    hc_regs     = host_if->hc_regs[CHANNEL];

    /* For CONTROL endpoint pid should start with DATA1 */
	if (devnum == root_hub_devnum) {
		dev->status = 0;
		dev->speed = USB_SPEED_HIGH;
		return dwc_otg_submit_rh_msg(dev, pipe, buffer, len, setup);
	}

	if (len > DWC_OTG_HCD_DATA_BUF_SIZE) {
		cprintf("submit_control_msg: buffer too large(%d)\n", len);
		dev->status = 0;
		dev->act_len = done;
		return -1;
	}

	/* Initialize channel, OUT for setup buffer */
	dwc_otg_hc_init(&g_core_if, CHANNEL, devnum, ep, 0, DWC_OTG_EP_TYPE_CONTROL, max);

	/* SETUP stage  */
	hctsiz.d32 = 0;
	hctsiz.b.xfersize = 8;
	hctsiz.b.pktcnt = 1;
	hctsiz.b.pid = DWC_OTG_HC_PID_SETUP;
	dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);

	dwc_write_reg32(&hc_regs->hcdma, (uint32)setup);

	
	hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
	hcchar.b.multicnt = 1;

	/* Remember original int status */
   	hcint.d32 = dwc_read_reg32(&hc_regs->hcint);

	/* Set host channel enable after all other setup is complete. */
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);

	/* TODO: no endless loop */
	while(1) {
		hcint_new.d32 = dwc_read_reg32(&hc_regs->hcint);
		if (hcint_new.b.chhltd) {
			break;
		}
	}

	/* TODO: check for error */
	if (!(hcint_new.b.chhltd && hcint_new.b.xfercomp)) {
		handle_error(__LINE__, hcint_new.d32);
		goto out;
	}

	/* Clear interrupts*/
	dwc_write_reg32(&hc_regs->hcintmsk, 0);
	dwc_write_reg32(&hc_regs->hcint, 0xFFFFFFFF);

	if (buffer) {
		/* DATA stage  */
		dwc_otg_hc_init(&g_core_if, CHANNEL, devnum, ep,
                        usb_pipein(pipe), DWC_OTG_EP_TYPE_CONTROL, max);

		/* TODO: check if len < 64 */
		hctsiz.d32 = 0;
		hctsiz.b.xfersize = len;
		hctsiz.b.pktcnt = 1;

		control_data_toggle[devnum][ep] = DWC_OTG_HC_PID_DATA1;
		hctsiz.b.pid = control_data_toggle[devnum][ep];

		dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);
		dwc_write_reg32(&hc_regs->hcdma, (uint32)buffer);

		hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
		hcchar.b.multicnt = 1;

		hcint.d32 = dwc_read_reg32(&hc_regs->hcint);

		/* Set host channel enable after all other setup is complete. */
		hcchar.b.chen = 1;
		hcchar.b.chdis = 0;
		dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);

		while(1) {
			hcint_new.d32 = dwc_read_reg32(&hc_regs->hcint);
            
			if (hcint_new.b.chhltd) {
				if (hcint_new.b.xfercomp) {
					hctsiz.d32 = dwc_read_reg32(&hc_regs->hctsiz);
                    
					if (usb_pipein(pipe)) {
						done = len - hctsiz.b.xfersize;
					} else {
						done = len;
                    }
				}

				if (hcint_new.b.ack) {
					if (hctsiz.b.pid == DWC_OTG_HC_PID_DATA0) {
						control_data_toggle[devnum][ep] = DWC_OTG_HC_PID_DATA0;
					} else {
						control_data_toggle[devnum][ep] = DWC_OTG_HC_PID_DATA1;
                    }
				}

				if (hcint_new.d32 != STATUS_ACK_HLT_COMPL) {
					handle_error(__LINE__, hcint_new.d32);
                    goto out;
				}

				break;
			}
		}
	} /* End of DATA stage */

	/* STATUS stage  */
	if ((len == 0) || usb_pipeout(pipe)) {
		status_direction = 1;
	} else {
		status_direction = 0;
    }

	dwc_otg_hc_init(&g_core_if, CHANNEL, devnum, ep, status_direction, DWC_OTG_EP_TYPE_CONTROL, max);

	hctsiz.d32 = 0;
	hctsiz.b.xfersize = 0;
	hctsiz.b.pktcnt = 1;
	hctsiz.b.pid = DWC_OTG_HC_PID_DATA1;
    
	dwc_write_reg32(&hc_regs->hctsiz, hctsiz.d32);
	dwc_write_reg32(&hc_regs->hcdma, (uint32)status_buf);

	hcchar.d32 = dwc_read_reg32(&hc_regs->hcchar);
	hcchar.b.multicnt = 1;

   	hcint.d32 = dwc_read_reg32(&hc_regs->hcint);

	/* Set host channel enable after all other setup is complete. */
	hcchar.b.chen = 1;
	hcchar.b.chdis = 0;
	dwc_write_reg32(&hc_regs->hcchar, hcchar.d32);
    
	while(1) {
		hcint_new.d32 = dwc_read_reg32(&hc_regs->hcint);
        
		if (hcint_new.b.chhltd) {
			break;
        }
	}
    
	if (hcint_new.d32 != STATUS_ACK_HLT_COMPL) {
		handle_error(__LINE__, hcint_new.d32);
    }
    
out:
	dev->act_len = done;
	dev->status = 0;
    
	return done;
}

int submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
                   int len, int interval)
{
	panic ("submit_int_msg: unsupported\n");
	return -1;
}
