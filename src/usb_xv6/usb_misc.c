// helper functions for USB
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "dwc_regs.h"
#include "usb.h"

// dump the dwc registers, the comments is the result of an example run
void dump_dwcregs (volatile struct dwc_regs *regs)
{
	usbprt("gotgctl: %x\n", regs->gotgctl);	// 0x10000
	usbprt("gotgint: %x\n", regs->gotgint);	// 0x0
	usbprt("gahbcfg: %x\n", regs->gahbcfg);	// 0x0
	usbprt("gusbcfg: %x\n", regs->gusbcfg);	// 0x1400
	usbprt("grstctl: %x\n", regs->grstctl); // 0x8000000
	usbprt("gintsts: %x\n", regs->gintsts);	// 0x14000020
	usbprt("gintmsk: %x\n", regs->gintmsk);	// 0x0
	usbprt("grxstsr: %x\n", regs->grxstsr);	// 0x6d77afa6
	usbprt("grxstsp: %x\n", regs->grxstsp);	// 0x6d77afa6
	usbprt("grxfsiz: %x\n", regs->grxfsiz);	// 0x1000
	usbprt("gnptxsts: %x\n", regs->gnptxsts);	// 0x80020
	usbprt("gi2cctl: %x\n", regs->gi2cctl);	// 0x0
	usbprt("gpvndctl: %x\n", regs->gpvndctl); // 0x0
	usbprt("ggpio: %x\n", regs->ggpio);	// 0x0
	usbprt("guid: %x\n", regs->guid);	// 0x2708a000
	usbprt("gsnpsid: %x\n", regs->gsnpsid);	// 0x4f54280a
	usbprt("ghwcfg1: %x\n", regs->ghwcfg1);	// 0x0
	usbprt("ghwcfg2: %x\n", regs->ghwcfg2);	// 0x228ddd50
	usbprt("ghwcfg3: %x\n", regs->ghwcfg3);	// 0xff000e8
	usbprt("ghwcfg4: %x\n", regs->ghwcfg4);	// 0x1ff0020
	usbprt("glpmcfg: %x\n", regs->glpmcfg);	// 0x757c6230
	usbprt("hptxfsiz: %x\n", regs->hptxfsiz);	// 0x0
	usbprt("hcfg: %x\n", regs->hcfg);	// 0x200000
	usbprt("hfir: %x\n", regs->hfir);	// 0x17d7
	usbprt("hfnum: %x\n", regs->hfnum);	// 0x0
	usbprt("hptxsts: %x\n", regs->hptxsts);	// 0x802000
	usbprt("haint: %x\n", regs->haint);	// 0x0
	usbprt("haintmsk: %x\n", regs->haintmsk);	// 0x0
	usbprt("hflbaddr: %x\n", regs->hflbaddr);	// 0x0
	usbprt("hprt0ctlsts: %x\n", regs->hport0);	// 0x0
}



// using mailbox to power on USB
static volatile uint32* mbox_base = P2V(0x2000B880);

#define READ        0
#define WRITE       8
#define STATUS      6

#define MBOX_FULL   0x80000000  // bit 31 is set if mbox is full
#define MBOX_EMPTY  0x40000000  // bit 30 is set if mbox is empty

// write an value to the mailbox channel ch
static void mbox_write (uint32 val, int ch)
{
    uint32 data;

    data = (val & ~0xF) | (ch & 0xF);

    // wait for an empty mail slot
    while(mbox_base[STATUS] & MBOX_FULL) {}
    mbox_base[WRITE] = data;
}

static uint32 mbox_read (int ch)
{
    uint32  data;

    while (1) {
        // busy-waiting for data
        while (mbox_base[STATUS] & MBOX_EMPTY) {}

        // the high 28 bits are data, low 4 bits are channel id
        data = mbox_base[READ];

        if (ch == (data & 0x0F)) {
            return data & ~0x0F;
        }
    }

    return -1;
}

// set power to usb. This is from CSUD project. do not understand why it works.
// maybe we should switch to a full-fledged video core interface
int power_usb (void)
{
	mbox_write (0x80, 0);
	return (mbox_read (0) == 0x80);
}
