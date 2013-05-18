// Timer for raspberry pi, we use the system timer as ARM timer could
// be inaccurate caused by power states
#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"

// BCM2835 system timer is a free run counter with 4 comparator
// comparator 0 and 2 are used by GPU. We use 3

// define registers (in units of 4-bytes)
#define STC_CS          0
#define STC_COUNTERLO   1
#define STC_COUNTERHI   2
#define STC_TIMER0      3
#define STC_TIMER1      4
#define STC_TIMER2      5
#define STC_TIMER3      6

#define TICK_TIMER      3

void isr_timer (struct trapframe *tp, int irq_idx);

static volatile uint32  *stc_base;
static  uint32          inc;
struct spinlock         tickslock;
uint                    ticks;

// acknowledge the timer
static void ack_timer ()
{
    // write clear the match status and interrupt
    stc_base[STC_CS] |= (1 << TICK_TIMER);

    // re-arm the timer
    stc_base[STC_TIMER3] = stc_base[STC_COUNTERLO] + inc;
}

// initialize the timer: perodical and interrupt based
void timer_init(int hz)
{
    stc_base = P2V(SYSTIMER_BASE);

    initlock(&tickslock, "time");
    pic_enable (SYS_TIMER3, isr_timer);

    // set the comparison register
    inc = CLK_HZ / hz;
    stc_base[STC_TIMER3] = stc_base[STC_COUNTERLO] + inc;
}

// interrupt service routine for the timer
void isr_timer (struct trapframe *tp, int irq_idx)
{
    acquire(&tickslock);

    ticks++;
    //cprintf("tick %d\n", ticks);
    wakeup(&ticks);

    release(&tickslock);
    ack_timer();
}

// a short delay, use timer 1 as the source
void micro_delay (int us)
{
    uint32  last;
    uint32  now;

    last = stc_base[STC_COUNTERLO];

    while (us > 0) {
        now = stc_base[STC_COUNTERLO];
        
        if (last != now) {
            last = now;
            us--;
        }
    }
}

uint32 get_tick (void)
{
	return stc_base[STC_COUNTERLO];
}
