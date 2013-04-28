// BSP support routine
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"
#include "memlayout.h"
#include "mmu.h"

extern void* end;

struct cpu	cpus[NCPU];
struct cpu	*cpu;

void kmain (void)
{
    uint vectbl;

    cga_init (&fbcon_lo);

    // init kernel memory, leave a hole for interrrupt vector table at 0xF0000
    vectbl = P2V_WO (VEC_TBL & PDE_MASK);

    if (vectbl <= (uint)&end) {
        cprintf ("error: vector table overlaps kernel\n");
    }

    cpu = &cpus[0];

    kmem_init ();
    kinit2((void*)&end, (void*)vectbl);
    kinit2((void*)(vectbl + PG_SIZE), (void*)(PHYSTOP+KERNBASE));

    trap_init ();				// vector table and stacks for models
    //pic_init (P2V(VIC_BASE));	// interrupt controller

    cprintf ("interrupt initialzed");
    /*
    consoleinit ();				// console
    pinit ();					// process (locks)

    binit ();					// buffer cache
    fileinit ();				// file table
    iinit ();					// inode cache
    ideinit ();					// ide (memory block device)
    timer_init (HZ);			// the timer (ticker)

    init_vmm ();
    cpu = &cpus[0];
    sti ();

    userinit();					// first user process
    scheduler();				// start running processes
 */
}
