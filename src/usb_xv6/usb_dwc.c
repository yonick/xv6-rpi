/*device driver for DWC OTG usb host controller*/
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "dwc_regs.h"
#include "usb.h"

#define DWC_PADDR 0x20980000

void dump_dwcregs (volatile struct dwc_regs *regs);
int power_usb (void);

/* DWC host controller */
static struct dwc_otg {
	volatile struct dwc_regs *regs;
	int						 nchan; // number of channels available

} dwc_ctrl;

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
