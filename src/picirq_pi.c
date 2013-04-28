// Support of Raspberry Pi interrupt "controller" Controller 
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"

// define the register offsets (in the unit of 4 bytes)
#define INTPI_PENDING0  0
#define INTPI_PENDING1  1
#define INTPI_PENDING2  2
#define INTPI_FIQCTRL   3
#define INTPI_ENABLE0   6
#define INTPI_ENABLE1   4
#define INTPI_ENABLE2   5
#define INTPI_DISABLE0  9
#define INTPI_DISABLE1  7
#define INTPI_DISABLE2  8

// BCM2835 has 96 interrupt bits 
#define NUM_INTSRC		96 // numbers of interrupt source supported

static volatile uint*   intpi_base;
static ISR              isrs[NUM_INTSRC];
static uint32           enabled[3];   // which interrupt is enabled

static struct {
    int     enable;
    int     disable;
} offsets[3] = {
    {INTPI_ENABLE0, INTPI_DISABLE0},
    {INTPI_ENABLE1, INTPI_DISABLE1},
    {INTPI_ENABLE2, INTPI_DISABLE2}
};

static void default_isr (struct trapframe *tf, int n)
{
    cprintf ("unhandled interrupt: %d\n", n);
}

// more interrupt is pending
static void irq_group1 (struct trapframe *tf, int n)
{
    uint intstatus;
    int		i;

    intstatus = intpi_base[INTPI_PENDING1] & enabled[1];

    for (i = 0; i < 32; i++) {
        if (intstatus & (1<<i)) {
            isrs[i+32](tf, i+32);
        }
    }
}

// more interrupt is pending
static void irq_group2 (struct trapframe *tf, int n)
{
    uint intstatus;
    int		i;

    intstatus = intpi_base[INTPI_PENDING2] & enabled[2];

    for (i = 0; i < 32; i++) {
        if (intstatus & (1<<i)) {
            isrs[i+64](tf, i+64);
        }
    }
}

// initialize the PL190 VIC
void pic_init (void * base)
{
    int i;

    // set the base for the controller and disable all interrupts
    intpi_base = base;
    intpi_base[INTPI_DISABLE0] = 0xFFFFFFFF;
    intpi_base[INTPI_DISABLE1] = 0xFFFFFFFF;
    intpi_base[INTPI_DISABLE2] = 0xFFFFFFFF;
    
    enabled[0] = enabled[1] = enabled[2] = 0;

    for (i = 0; i < NUM_INTSRC; i++) {
        isrs[i] = default_isr;
    }

    // bit 8 and 9 of base interrupts mean "need to check irqpending1 and 2.
    pic_enable (8, irq_group1);
    pic_enable (9, irq_group2);
}

// enable an interrupt (with the ISR)
void pic_enable (int n, ISR isr)
{
    int idx;

    if ((n < 0) || (n >= NUM_INTSRC)) {
        panic ("invalid interrupt source");
    }

    isrs[n] = isr;

    // set which group of interrupt
    idx = (n >> 5);
    n   = (n & 31);

    if (enabled[idx] & (1 << n)) {
        cprintf ("overwrite ISR%d: %x->%x, enabled[%d]: %x\n",
                 n, isrs[(idx<<5) + n], isr, idx, enabled[idx]);
    }

    // write 1 bit enable the interrupt, 0 bit has no effect
    enabled[idx] |= (1 << n);
    intpi_base[offsets[idx].enable] = (1 << n);
}

// disable an interrupt
void pic_disable (int n)
{
    int idx;

    if ((n<0) || (n >= NUM_INTSRC)) {
        panic ("invalid interrupt source");
    }

    isrs[n] = default_isr;

    idx = (n >> 5);
    n   = (n & 31);

    enabled[idx] &= ~(1 << n);
    intpi_base[offsets[idx].disable] = (1 << n);
}

// dispatch the interrupt
void pic_dispatch (struct trapframe *tp)
{
    uint intstatus;
    int		i;

    intstatus = intpi_base[INTPI_PENDING0] & enabled[0];

    for (i = 0; i < 32; i++) {
        if (intstatus & (1<<i)) {
            isrs[i](tp, i);
        }
    }
}

