// board configuration for raspberry pi (BCM2835_ARM_peripherals.pdf) 
#ifndef PI_HEAD
#define PI_HEAD


// just assume we have 128M memory for now, need a way to discover it

#define PHYSTOP         0x08000000


#define DEVBASE         0x20000000
#define DEV_MEM_SZ      0x01000000
#define VEC_TBL         0xFFFF0000


#define STACK_FILL      0xdeadbeef

#define TIMER0          0x101E2000
#define TIMER1          0x101E2020

#define CLK_HZ          1000000     // the clock is 1MHZ

// Interrupt controller and interrupt source
#define VIC_BASE        0x10140000
#define PIC_TIMER01     4
#define PIC_TIMER23     5
#define PIC_UART0       12
#define PIC_GRAPHIC     19

#endif