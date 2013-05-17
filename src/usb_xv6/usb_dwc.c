/*device driver for DWC OTG usb host controller*/
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "mmu.h"
#include "dwc_regs.h"
#include "usb.h"
#include "usb_desc.h"

#define DWC_PADDR 0x20980000

void dump_dwcregs (volatile struct dwc_regs *regs);
int power_usb (void);

/* DWC host controller */
static struct dwc_otg {
	volatile struct dwc_regs *regs;
	int						 nchan; 	// number of channels available (8)
	uint32					 chanbusy;	// whether the channel is busy
} dwc_ctrl;

static struct hc_regs * alloc_chan (void)
{
	int i;

	for (i = 0; i < dwc_ctrl.nchan; i++) {
		if ((dwc_ctrl.chanbusy & (1 << i)) == 0) {
			dwc_ctrl.chanbusy |= (1 << i);
			return dwc_ctrl.regs->hchans+i;
		}
	}

	panic ("no channel is available\n");
	return NULL;
}

static void release_chan (struct hc_regs *chan)
{
	int i;

	i = chan - dwc_ctrl.regs->hchans;
	dwc_ctrl.chanbusy &= ~(1 << i);
}

/* setup the host channel for transaction to this endpoint*/
static void setup_chan (struct hc_regs *hc, struct usb_ep *ep)
{
	int hcc;

	// set device id, during configuration, the device id is zero
	hcc = (ep->dev->id << ODevaddr);
	hcc |= ep->maxpkt | (1 << OMulticnt) | (ep->id << OEpnum);

	switch(ep->ttype){
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

	switch(ep->dev->speed){
	case Lowspeed:
		hcc |= Lspddev;

		/* fall through, to enable split transaction*/
	case Fullspeed:
		hc->hcsplt = Spltena | POS_ALL | (ep->dev->hub<<OHubaddr) | ep->dev->port;
		break;

	default:
		hc->hcsplt = 0;
		break;
	}

	hc->hcchar = hcc;
	hc->hcint = ~0;
}

static int
chanio(Ep *ep, Hostchan *hc, int dir, int pid, void *a, int len)
{
	Ctlr *ctlr;
	int nleft, n, nt, i, maxpkt, npkt;
	uint hcdma, hctsiz;

	ctlr = ep->hp->aux;
	maxpkt = ep->maxpkt;
	npkt = HOWMANY(len, ep->maxpkt);
	if(npkt == 0)
		npkt = 1;

	hc->hcchar = (hc->hcchar & ~Epdir) | dir;
	if(dir == Epin)
		n = ROUND(len, ep->maxpkt);
	else
		n = len;
	hc->hctsiz = n | npkt<<OPktcnt | pid;
	hc->hcdma  = PADDR(a);

	nleft = len;
	logstart(ep);
	for(;;){
		hcdma = hc->hcdma;
		hctsiz = hc->hctsiz;
		hc->hctsiz = hctsiz & ~Dopng;
		if(hc->hcchar&Chen){
			dprint("ep%d.%d before chanio hcchar=%8.8ux\n",
				ep->dev->nb, ep->nb, hc->hcchar);
			hc->hcchar |= Chen | Chdis;
			while(hc->hcchar&Chen)
				;
			hc->hcint = Chhltd;
		}
		if((i = hc->hcint) != 0){
			dprint("ep%d.%d before chanio hcint=%8.8ux\n",
				ep->dev->nb, ep->nb, i);
			hc->hcint = i;
		}
		if(hc->hcsplt & Spltena){
			qlock(&ctlr->split);
			sofwait(ctlr, hc - ctlr->regs->hchan);
			if((dwc.regs->hfnum & 1) == 0)
				hc->hcchar &= ~Oddfrm;
			else
				hc->hcchar |= Oddfrm;
		}
		hc->hcchar = (hc->hcchar &~ Chdis) | Chen;
		clog(ep, hc);
		if(ep->ttype == Tbulk && dir == Epin)
			i = chanwait(ep, ctlr, hc, /* Ack| */ Chhltd);
		else if(ep->ttype == Tintr && (hc->hcsplt & Spltena))
			i = chanwait(ep, ctlr, hc, Chhltd);
		else
			i = chanwait(ep, ctlr, hc, Chhltd|Nak);
		clog(ep, hc);
		hc->hcint = i;

		if(hc->hcsplt & Spltena){
			hc->hcsplt &= ~Compsplt;
			qunlock(&ctlr->split);
		}

		if((i & Xfercomp) == 0 && i != (Chhltd|Ack) && i != Chhltd){
			if(i & Stall)
				error(Estalled);
			if(i & Nyet)
				continue;
			if(i & Nak){
				if(ep->ttype == Tintr)
					tsleep(&up->sleep, return0, 0, ep->pollival);
				else
					tsleep(&up->sleep, return0, 0, 1);
				continue;
			}
			print("usbotg: ep%d.%d error intr %8.8ux\n",
				ep->dev->nb, ep->nb, i);
			if(i & ~(Chhltd|Ack))
				error(Eio);
			if(hc->hcdma != hcdma)
				print("usbotg: weird hcdma %x->%x intr %x->%x\n",
					hcdma, hc->hcdma, i, hc->hcint);
		}
		n = hc->hcdma - hcdma;
		if(n == 0){
			if((hc->hctsiz & Pktcnt) != (hctsiz & Pktcnt))
				break;
			else
				continue;
		}
		if(dir == Epin && ep->ttype == Tbulk && n == nleft){
			nt = (hctsiz & Xfersize) - (hc->hctsiz & Xfersize);
			if(nt != n){
				if(n == ROUND(nt, 4))
					n = nt;
				else
					print("usbotg: intr %8.8ux "
						"dma %8.8ux-%8.8ux "
						"hctsiz %8.8ux-%8.ux\n",
						i, hcdma, hc->hcdma, hctsiz,
						hc->hctsiz);
			}
		}
		if(n > nleft){
			if(n != ROUND(nleft, 4))
				dprint("too much: wanted %d got %d\n",
					len, len - nleft + n);
			n = nleft;
		}
		nleft -= n;
		if(nleft == 0 || (n % maxpkt) != 0)
			break;
		if((i & Xfercomp) && ep->ttype != Tctl)
			break;
		if(dir == Epout)
			dprint("too little: nleft %d hcdma %x->%x hctsiz %x->%x intr %x\n",
				nleft, hcdma, hc->hcdma, hctsiz, hc->hctsiz, i);
	}
	logdump(ep);
	return len - nleft;
}

static long
eptrans(Ep *ep, int rw, void *a, long n)
{
	Hostchan *hc;

	if(ep->clrhalt){
		ep->clrhalt = 0;
		if(ep->mode != OREAD)
			ep->toggle[Write] = DATA0;
		if(ep->mode != OWRITE)
			ep->toggle[Read] = DATA0;
	}
	hc = chanalloc(ep);
	if(waserror()){
		ep->toggle[rw] = hc->hctsiz & Pid;
		chanrelease(ep, hc);
		if(strcmp(up->errstr, Estalled) == 0)
			return 0;
		nexterror();
	}
	chansetup(hc, ep);
	if(rw == Read && ep->ttype == Tbulk){
		long sofar, m;

		sofar = 0;
		do{
			m = n - sofar;
			if(m > ep->maxpkt)
				m = ep->maxpkt;
			m = chanio(ep, hc, Epin, ep->toggle[rw],
				(char*)a + sofar, m);
			ep->toggle[rw] = hc->hctsiz & Pid;
			sofar += m;
		}while(sofar < n && m == ep->maxpkt);
		n = sofar;
	}else{
		n = chanio(ep, hc, rw == Read? Epin : Epout, ep->toggle[rw],
			a, n);
		ep->toggle[rw] = hc->hctsiz & Pid;
	}
	chanrelease(ep, hc);
	poperror();
	return n;
}

static long
ctltrans(Ep *ep, uchar *req, long n)
{
	Hostchan *hc;
	Epio *epio;
	Block *b;
	uchar *data;
	int datalen;

	epio = ep->aux;
	if(epio->cb != nil){
		freeb(epio->cb);
		epio->cb = nil;
	}
	if(n < Rsetuplen)
		error(Ebadlen);
	if(req[Rtype] & Rd2h){
		datalen = GET2(req+Rcount);
		if(datalen <= 0 || datalen > Maxctllen)
			error(Ebadlen);
		/* XXX cache madness */
		epio->cb = b = allocb(ROUND(datalen, ep->maxpkt) + CACHELINESZ);
		b->wp = (uchar*)ROUND((uintptr)b->wp, CACHELINESZ);
		memset(b->wp, 0x55, b->lim - b->wp);
		cachedwbinvse(b->wp, b->lim - b->wp);
		data = b->wp;
	}else{
		b = nil;
		datalen = n - Rsetuplen;
		data = req + Rsetuplen;
	}
	hc = chanalloc(ep);
	if(waserror()){
		chanrelease(ep, hc);
		if(strcmp(up->errstr, Estalled) == 0)
			return 0;
		nexterror();
	}
	chansetup(hc, ep);
	chanio(ep, hc, Epout, SETUP, req, Rsetuplen);
	if(req[Rtype] & Rd2h){
		b->wp += chanio(ep, hc, Epin, DATA1, data, datalen);
		chanio(ep, hc, Epout, DATA1, nil, 0);
		n = Rsetuplen;
	}else{
		if(datalen > 0)
			chanio(ep, hc, Epout, DATA1, data, datalen);
		chanio(ep, hc, Epin, DATA1, nil, 0);
		n = Rsetuplen + datalen;
	}
	chanrelease(ep, hc);
	poperror();
	return n;
}

static long
ctldata(Ep *ep, void *a, long n)
{
	Epio *epio;
	Block *b;

	epio = ep->aux;
	b = epio->cb;
	if(b == nil)
		return 0;
	if(n > BLEN(b))
		n = BLEN(b);
	memmove(a, b->rp, n);
	b->rp += n;
	if(BLEN(b) == 0){
		freeb(b);
		epio->cb = nil;
	}
	return n;
}

/*read some data from the endpoint*/
int ep_read(struct usb_ep *ep, void *a, int n)
{
	uchar 	*p;
	ulong 	elapsed;
	long 	nr;
	int		ord;

	usbprt ("epread ep%d.%d %ld\n", ep->dev->id, ep->id, n);

	switch(ep->ttype){
	case EP_CTRL:
		nr = ctldata(ep, a, n);
		return nr;

	case EP_INT:
	case EP_BULK:
		ord = get_order(align_up (n, ep->maxpkt));
		p = kmalloc (ord);

		if (p == NULL) {
			usbprt("ep_read failed to alloc memory\n");
			return -1;
		}

		nr = eptrans(ep, Read, p, n);
		memmove(a, p, nr);

		kfree (p, ord);
		return nr;

	default:
		usbprt ("unsupported ep type: %d\n", ep->ttype);
	}

	return -1;
}

/*write some data to the endpoint*/
static int ep_write(struct usb_ep *ep, void *a, int n)
{
	uchar 	*p;
	ulong 	elapsed;
	int		ord;

	usbprt ("ep_write ep%d.%d %ld\n", ep->dev->nb, ep->nb, n);

	switch(ep->ttype){
	case EP_INT:
	case EP_CTRL:
	case EP_BULK:
		ord = get_order (n);
		p = kmalloc (ord);
		memmove(p, a, n);

		if(ep->ttype == Tctl) {
			n = ctltrans(ep, p, n);
		} else{
			n = eptrans(ep, Write, p, n);
		}

		kfree(p, ord);
		return n;

	default:
		usbprt ("unsupported ep type: %d\n", ep->ttype);
	}

	return -1;
}

/* enable the host port, there is only one host port*/
static int enable_hcport (void)
{
	dwc_ctrl.regs->hport0 = Prtpwr | Prtena;
	micro_delay (50);

	usbprt ("usbotg enable, sts %#x\n", dwc_ctrl.regs->hport0);
	return 0;
}

/* reset the host port */
static int reset_hcport (void)
{
	struct dwc_regs *r;
	uint32			s;
	uint32			b;

	r = dwc_ctrl.regs;

	r->hport0 = Prtpwr | Prtrst;
	micro_delay(10);

	r->hport0 = Prtpwr;
	micro_delay(50);

	s = r->hport0;
	b = s & (Prtconndet|Prtenchng|Prtovrcurrchng);

	if(b != 0) {
		r->hport0 = Prtpwr | b;
	}

	usbprt("usbotg reset; sts %#x\n", s);

	if((s & Prtena) == 0) {
		usbprt("usbotg: host port not enabled after reset");
	}

	return 0;
}

// reset the bits in core reset
static void greset (volatile struct dwc_regs *regs, int bits)
{
	// set the bits, the controller will clear it when its done
	regs->grstctl |= bits;
	while (regs->grstctl & bits);
	micro_delay (10);
}

// initialize the DWC OTG host controller
int usb_inithc ()
{
	volatile struct dwc_regs *regs;
	uint32 id, n, rx, tx, ptx;

	dwc_ctrl.regs = P2V(DWC_PADDR);
	regs = dwc_ctrl.regs;

	// check ID of the USB controller (just basic saint check
	id = regs->gsnpsid;
	if ((id >> 16) != ('O'<< 8 | 'T')) {
		usbprt("failed to detect DWC usb controller\n");
		return -1;
	}

	dwc_ctrl.nchan = 1 + ((regs->ghwcfg2 & Num_host_chan) >> ONum_host_chan);

	usbprt("dwc-otg rev %d.%x (%d channels)\n", (id >> 12) & 0x0F,
			id & 0xFFF, dwc_ctrl.nchan);

	// power on USB, waiting for it to be in the AHB master idle state
	regs->gahbcfg = 0;
	power_usb ();
	while ((regs->grstctl & Ahbidle) == 0);

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
	micro_delay (1);

	regs->hptxfsiz = (rx + tx) | (ptx << ODepth);
	greset (regs, Rxfflsh);

	regs->grstctl = TXF_ALL;
	greset(regs, Txfflsh);

	// power the host port and enable the host channel interrupt
	regs->hport0 = Prtpwr | Prtconndet | Prtenchng | Prtovrcurrchng;
	regs->gintsts = ~0; // clear all interrupts
	regs->gintmsk = Hcintr;
	regs->gahbcfg |= Glblintrmsk;

	return 0;
}
