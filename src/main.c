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

void info (void)
{
    cprintf ("Start xv6 for Raspberry Pi...\n\n\n");
}

void kmain (void)
{
    uint vectbl;

    cpu = &cpus[0];
    cga_init (&fbcon_lo);

    // interrrupt vector table is in the middle of first 1MB. We use the left
    // over for page tables
    vectbl = _P2V (VEC_TBL & PDE_MASK);
    
    init_vmm ();
    kpt_freerange (align_up(&end, PT_SZ), vectbl);
    kpt_freerange (vectbl + PT_SZ, _P2V(INIT_KERNMAP));
    paging_init (INIT_KERNMAP, PHYSTOP);

    cpu = &cpus[0];

    kmem_init ();
    kmem_init2(P2V(INIT_KERNMAP), P2V(PHYSTOP));

    trap_init ();				// vector table and stacks for models
    pic_init (P2V(VIC_BASE));	// interrupt controller

    consoleinit ();				// console
    pinit ();					// process (locks)
    
    binit ();					// buffer cache
    fileinit ();				// file table
    iinit ();					// inode cache
    ideinit ();					// ide (memory block device)

    timer_init (HZ);			// the timer (ticker)
    sti ();
    info ();
    
    userinit();					// first user process
    scheduler();				// start running processes
}
