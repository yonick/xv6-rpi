/*device driver for DWC OTG usb host controller*/
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "dwc_regs.h"
#include "usb.h"
#include "usb_desc.h"

#define DWC_PADDR 0x20980000
#define HOW_MANY(x, y) (((x) + (y) -1) /(y))
#define ROUND(x, y) (HOW_MANY(x, y) *(y))

void dump_dwcregs(volatile struct dwc_regs *regs);
int power_usb(void);

/* DWC host controller */
static struct dwc_otg {
	volatile struct dwc_regs *regs;
	int nchan; 	// number of channels available (8)
	uint32 chanbusy;	// whether the channel is busy
} dwc_ctrl;

static struct hc_regs * alloc_chan(void)
{
	int i;

	for (i = 0; i < dwc_ctrl.nchan; i++) {
		if ((dwc_ctrl.chanbusy & (1 << i)) == 0) {
			dwc_ctrl.chanbusy |= (1 << i);
			return (struct hc_regs*)(dwc_ctrl.regs->hchans + i);
		}
	}

	panic("no channel is available\n");
	return NULL ;
}

static void release_chan(struct hc_regs *chan)
{
	int i;

	i = chan - dwc_ctrl.regs->hchans;
	dwc_ctrl.chanbusy &= ~(1 << i);
}

/* setup the host channel for transaction to this endpoint*/
static void setup_chan(struct hc_regs *hc, struct usb_ep *ep)
{
	int hcc;

	// set device id, during configuration, the device id is zero
	hcc = (ep->dev->id << ODevaddr);
	hcc |= ep->maxpkt | (1 << OMulticnt) | (ep->id << OEpnum);

	switch (ep->ttype) {
	case EP_CTRL:
		hcc |= Epctl;
		break;

	case EP_ISO:
		hcc |= Episo;
		break;

	case EP_BULK:
		hcc |= Epbulk;
		break;

	case EP_INT:
		hcc |= Epintr;
		break;
	}

	switch (ep->dev->speed) {
	case Lowspeed:
		hcc |= Lspddev;

		/* fall through, to enable split transaction*/
	case Fullspeed:
		hc->hcsplt = Spltena | POS_ALL | (ep->dev->hub << OHubaddr)
				| ep->dev->port;
		break;

	default:
		hc->hcsplt = 0;
		break;
	}

	hc->hcchar = hcc;
	hc->hcint = ~0;
}

//busy wait for SOF
static void sof_wait(void)
{
	volatile struct dwc_regs *r;
	uint32 start, now;

	r = dwc_ctrl.regs;
	r->gintsts = Sofintr;			// clear Sofint

	start = get_tick();

	while (r->gintsts & Sofintr) { 	// waiting for Sofint to be set
		now = get_tick();

		if (now - start >= 100) {
			usbprt("wait too long for Sof");
		}
	}

	r->gintsts = Sofintr;			// clear it
}

// busy wait the channel transfer results:
static int wait_chan(struct usb_ep *ep, struct hc_regs *hc, int mask)
{
	uint32 start, now;
	int intr;

	intr = 0;

	now = start = get_tick();

	while (now < start + 100) {
		intr = hc->hcint;

		// the channel has halted, return
		if ((intr & Chhltd) || (intr & mask)) {
			break;
		}

		now = get_tick();
	}

	// disable the channel, but do not wait too long
	hc->hcchar |= Chdis;
	start = get_tick();

	while (hc->hcchar & Chen) {
		now = get_tick();

		if (now - start >= 100) {
			usbprt("ep%d.%d channel won't halt hcchar %x\n", ep->dev->id,
					ep->id, hc->hcchar);
			break;
		}
	}

	return intr;
}

int chan_io(struct usb_ep *ep, struct hc_regs *hc, int dir, int pid, void *a,
		int len)
{
	int nleft, n, nt, intr, maxpkt, npkt;
	uint hcdma, hctsiz;
	uint32 start, now;

	maxpkt = ep->maxpkt;
	npkt = HOW_MANY(len, ep->maxpkt);

	if (npkt == 0) {
		npkt = 1;
	}

	hc->hcchar = (hc->hcchar & ~Epdir) | dir; // set the communication dir

	if (dir == Epin) {
		n = ROUND (len, ep->maxpkt);
	} else {
		n = len;
	}

	hc->hctsiz = n | (npkt << OPktcnt) | pid;	// set DMA to start transfer
	hc->hcdma = V2P(a);

	nleft = len;

	for (;;) {
		hcdma = hc->hcdma;
		hctsiz = hc->hctsiz;

		hc->hctsiz = hctsiz & ~Dopng;

		if (hc->hcchar & Chen) {
			usbprt("ep%d.%d before chanio hcchar=%x\n", ep->dev->id, ep->id,
					hc->hcchar);

			hc->hcchar |= Chen | Chdis; // enable and disable channel???

			start = get_tick();

			while (hc->hcchar & Chen) {
				now = get_tick();

				if (now - start >= 500) {
					usbprt("waiting too long to disable hc\n");
					break;
				}
			}

			hc->hcint = Chhltd;			// clear channel halted
		}

		if ((intr = hc->hcint) != 0) {
			usbprt("ep%d.%d before chanio hcint=%x\n", ep->dev->id, ep->id,
					intr);
			hc->hcint = intr;
		}

		if (hc->hcsplt & Spltena) {
			sof_wait(); // wait for start-of-frame

			if ((dwc_ctrl.regs->hfnum & 1) == 0) {
				hc->hcchar &= ~Oddfrm;
			} else {
				hc->hcchar |= Oddfrm;
			}
		}

		// enable the channel
		hc->hcchar = (hc->hcchar & ~Chdis) | Chen;

		// wait for the channel to complete
		if ((ep->ttype == EP_BULK) && (dir == Epin)) {
			intr = wait_chan (ep, hc, Chhltd);

		} else if (ep->ttype == EP_INT && (hc->hcsplt & Spltena)) {
			intr = wait_chan(ep, hc, Chhltd);

		} else {
			intr = wait_chan(ep, hc, Chhltd | Nak);
		}

		hc->hcint = intr;

		if (hc->hcsplt & Spltena) {
			hc->hcsplt &= ~Compsplt;
		}

		// handle results
		if ((intr & Xfercomp) == 0 && intr != (Chhltd | Ack)
				&& intr != Chhltd) {
			if (intr & Stall) {
				usbprt("endpoing stalled: ep%d.%d\n", ep->dev->id, ep->id);
				return -1;
			}

			// not ready yet, high-speed only, for split transaction
			if (intr & Nyet) {
				continue;
			}

			// no data to transfer, wait
			if (intr & Nak) {
				micro_delay(1);
				continue;
			}

			usbprt ("ep%d.%d error intr %x\n", ep->dev->id, ep->id, intr);

			// other errors: transaction error, babble error, frame overrun
			if (intr & ~(Chhltd | Ack)) {
				return -1;
			}

			// the channel received valid ACK and halted, just restart
			if (hc->hcdma != hcdma) {
				usbprt("weird hcdma %x->%x intr %x->%x\n", hcdma, hc->hcdma,
						intr, hc->hcint);
			}
		}

		// how many data transferred by DMA, adjust the leftover
		n = hc->hcdma - hcdma;

		if (n == 0) {
			if ((hc->hctsiz & Pktcnt) != (hctsiz & Pktcnt)) {
				break;
			}

			continue;
		}

		// not all data transferred
		if ((dir == Epin) && (ep->ttype == EP_BULK) && (n == nleft)) {
			nt = (hctsiz & Xfersize) - (hc->hctsiz & Xfersize);

			if (nt != n) {
				if (n == ROUND(nt, 4)) {
					n = nt;
				} else {
					usbprt("chan_io: intr %x, dma %x-%x, hctsiz %x-%x\n", intr,
							hcdma, hc->hcdma, hctsiz, hc->hctsiz);
				}
			}
		}

		if (n > nleft) {
			if (n != ROUND(nleft, 4)) {
				usbprt("too much: wanted %d got %d\n", len, len - nleft + n);
			}

			n = nleft;
		}

		nleft -= n;

		if (nleft == 0 || (n % maxpkt) != 0) {
			break;
		}

		if ((intr & Xfercomp) && (ep->ttype != EP_CTRL)) {
			break;
		}

		if (dir == Epout) {
			usbprt("too little: nleft %d hcdma %x->%x hctsiz %x->%x intr %x\n",
					nleft, hcdma, hc->hcdma, hctsiz, hc->hctsiz, intr);
		}
	}

	return len - nleft;
}

// a non-control transfer, control transfer is special
int ep_trans(struct usb_ep *ep, int rw, void *a, int n)
{
	struct hc_regs *hc;
	long sofar, m;

	if (ep->clrhalt) {
		ep->clrhalt = 0;
		ep->toggle = DATA0;
	}

	hc = alloc_chan();
	setup_chan (hc, ep);

	if ((rw == Read) && (ep->ttype == EP_BULK)) {
		sofar = 0;

		do {
			m = n - sofar;

			if (m > ep->maxpkt) {
				m = ep->maxpkt;
			}

			m = chan_io(ep, hc, Epin, ep->toggle, (char*) a + sofar, m);

			ep->toggle = hc->hctsiz & Pid;
			sofar += m;
		} while ((sofar < n) && (m == ep->maxpkt));

		n = sofar;
	} else {
		n = chan_io(ep, hc, rw == Read ? Epin : Epout, ep->toggle, a, n);
		ep->toggle = hc->hctsiz & Pid;
	}

	release_chan(hc);
	return n;
}

// perform a control transfer, control transfer is bi-directional
static int ctl_trans(struct usb_ep *ep, uchar *req, int n)
{
	struct hc_regs *hc;
	struct usb_setup *pkt;
	uchar *data;
	int datalen;
	int ret;

	pkt = (struct usb_setup*) req;

	if (n < Rsetuplen) {
		usbprt("setup packet too short %d\n", n);
		return -1;
	}

	data = req + Rsetuplen;

	if (pkt->bmRequestType & Rd2h) { // device to host transfer
		datalen = pkt->wLength;
	} else { // host to device
		datalen = n - Rsetuplen;
	}

	hc = alloc_chan();

	if (hc == NULL ) {
		return -1;
	}

	setup_chan(hc, ep);

	// send the request, then data data
	chan_io(ep, hc, Epout, SETUP, req, Rsetuplen);

	ret = 0;

	if (req[Rtype] & Rd2h) {
		ret = chan_io(ep, hc, Epin, DATA1, data, datalen);
		chan_io(ep, hc, Epout, DATA1, NULL, 0);
	} else {
		if (datalen > 0) {
			ret = chan_io(ep, hc, Epout, DATA1, data, datalen);
		}

		chan_io(ep, hc, Epin, DATA1, NULL, 0);
	}

	release_chan(hc);

	n = Rsetuplen + ret;
	return n;
}

/*read some data from the endpoint*/
int ep_read(struct usb_ep *ep, void *a, int n)
{
	long nr;

	usbprt("epread ep%d.%d %ld\n", ep->dev->id, ep->id, n);

	switch (ep->ttype) {
	case EP_CTRL:
		nr = ctl_trans(ep, a, n);
		return nr;

	case EP_INT:
	case EP_BULK:
		nr = ep_trans(ep, Read, a, n);
		return nr;

	default:
		usbprt("unsupported ep type: %d\n", ep->ttype);
	}

	return -1;
}

/*write some data to the endpoint*/
static int ep_write(struct usb_ep *ep, void *a, int n)
{
	usbprt("ep_write ep%d.%d %ld\n", ep->dev->id, ep->id, n);

	switch (ep->ttype) {
	case EP_INT:
	case EP_CTRL:
	case EP_BULK:
		if (ep->ttype == EP_CTRL) {
			n = ctl_trans(ep, a, n);
		} else {
			n = ep_trans(ep, Write, a, n);
		}
		return n;

	default:
		usbprt("unsupported ep type: %d\n", ep->ttype);
	}

	return -1;
}

/* enable the host port, there is only one host port*/
static int enable_hcport(void)
{
	dwc_ctrl.regs->hport0 = Prtpwr | Prtena;
	micro_delay(50);

	usbprt("usbotg enable, sts %#x\n", dwc_ctrl.regs->hport0);
	return 0;
}

/* reset the host port */
static int reset_hcport(void)
{
	volatile struct dwc_regs *r;
	uint32 s;
	uint32 b;

	r = dwc_ctrl.regs;

	r->hport0 = Prtpwr | Prtrst;
	micro_delay(10);

	r->hport0 = Prtpwr;
	micro_delay(50);

	s = r->hport0;
	b = s & (Prtconndet | Prtenchng | Prtovrcurrchng);

	if (b != 0) {
		r->hport0 = Prtpwr | b;
	}

	usbprt("usbotg reset; sts %#x\n", s);

	if ((s & Prtena) == 0) {
		usbprt("usbotg: host port not enabled after reset");
	}

	return 0;
}

// reset the bits in core reset
static void greset(volatile struct dwc_regs *regs, int bits)
{
	// set the bits, the controller will clear it when its done
	regs->grstctl |= bits;
	while (regs->grstctl & bits)
		;
	micro_delay(10);
}

// initialize the DWC OTG host controller
int usb_inithc()
{
	volatile struct dwc_regs *regs;
	uint32 id, rx, tx, ptx;

	dwc_ctrl.regs = P2V(DWC_PADDR);
	regs = dwc_ctrl.regs;

	// check ID of the USB controller (just basic saint check
	id = regs->gsnpsid;
	if ((id >> 16) != ('O' << 8 | 'T')) {
		usbprt("failed to detect DWC usb controller\n");
		return -1;
	}

	dwc_ctrl.nchan = 1 + ((regs->ghwcfg2 & Num_host_chan) >> ONum_host_chan);

	usbprt("dwc-otg rev %d.%x (%d channels)\n", (id >> 12) & 0x0F, id & 0xFFF,
			dwc_ctrl.nchan);

	// power on USB, waiting for it to be in the AHB master idle state
	regs->gahbcfg = 0;
	power_usb();
	while ((regs->grstctl & Ahbidle) == 0)
		;

	greset(regs, Csftrst);

	// force OTG (host) mode, instead of device mode
	regs->gusbcfg |= Force_host_mode;
	micro_delay(25);

	// enable DMA
	regs->gahbcfg |= Dmaenable;

	// set FIFO size and flush the RX/TX fifo
	rx = 0x306;
	tx = 0x100;
	ptx = 0x200;

	regs->grxfsiz = rx;
	regs->gnptxfsiz = rx | (tx << ODepth);
	micro_delay(1);

	regs->hptxfsiz = (rx + tx) | (ptx << ODepth);
	greset(regs, Rxfflsh);

	regs->grstctl = TXF_ALL;
	greset(regs, Txfflsh);

	// power the host port and enable the host channel interrupt
	regs->hport0 = Prtpwr | Prtconndet | Prtenchng | Prtovrcurrchng;
	regs->gintsts = ~0; // clear all interrupts
	regs->gintmsk = Hcintr;
	regs->gahbcfg |= Glblintrmsk;

	return 0;
}
