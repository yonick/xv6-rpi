// initialize section
#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"

// to access mailbox, framebuffer uses mailbox 0 channel 1. for more 
// information: https://github.com/raspberrypi/firmware/wiki/Mailboxes
volatile uint32* mbox_base = (uint32*)0x2000B880;
#define READ        0
#define WRITE       8
#define STATUS      6

#define MBOX_FULL   0x80000000  // bit 31 is set if mbox is full
#define MBOX_EMPTY  0x40000000  // bit 30 is set if mbox is empty

// write an value to the mailbox channel ch
void mbox0_write (uint32 val, int ch)
{
    uint32 data;

    data = (val & ~0xF) | (ch & 0xF);

    // wait for an empty mail slot
    while(mbox_base[STATUS] & MBOX_FULL) {}

    mbox_base[WRITE] = data;
}

uint32 mbox0_read (int ch)
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

// framebuffer information: used to communicate with GPU to
// set resolution and retrieve framebuffer
struct fb_info {
    uint32 wd;
    uint32 ht;
    uint32 v_wd;
    uint32 v_ht;
    uint32 pitch;      // GPU - Pitch
    uint32 depth;
    uint32 x;
    uint32 y;
    uint32 ptr;    // GPU - framebuffer pointer
    uint32 size;       // GPU - size
};

volatile struct fb_info fb __attribute__ ((aligned (16)));

// a character is 8-bit wide, and 16-bit high, each font
// is thus 16 bytes
#define CH_XSHIFT        3
#define CH_YSHIFT        4
#define CH_COLOR    0xFFFF

struct {
    uint32  c_wd;  // how many characters in a row
    uint32  c_ht; // how many lines in a column
    uint32  cur_x;  // current x position
    uint32  cur_y;  // current y position
} fb_con;

extern uint32 font_addr;
uint32* fonts = &font_addr;

static void inline set_pixle (int x, int y, uint16 color)
{
    uint16 *fptr;

    if ((x >= fb.wd) || (y >= fb.ht)) {
        return;
    }
    
    fptr = (uint16*)(fb.ptr);
    fptr[x + y * fb.wd] = color;
}

// draw a line of character
static void inline draw_cline (int x, int y, uint32 l)
{
    l &= 0xFF;

    while (l != 0) {
        if (l & 0x01) {
            set_pixle (x, y, CH_COLOR);
        }

        l >>= 1;
        x++;
    }
}

static void inline draw_cword (int x, int y, uint32 w)
{
    draw_cline(x, y, w);
    draw_cline(x, y + 1, w >> 8);
    draw_cline(x, y + 2, w >> 16);
    draw_cline(x, y + 3, w >> 24);
}

static void _con_putc (int cx, int cy, int ch)
{
    uint32  w1, w2, w3, w4;

    if (ch >= 128) {
        return;
    }

    // load the bitmap for the character, each character is
    // 16 bytes, or 4 words
    ch <<= 2;

    w1 = fonts[ch++];
    w2 = fonts[ch++];
    w3 = fonts[ch++];
    w4 = fonts[ch];

    cx <<= CH_XSHIFT;
    cy <<= CH_YSHIFT;

    draw_cword(cx, cy,      w1);
    draw_cword(cx, cy + 4,  w2);
    draw_cword(cx, cy + 8,  w3);
    draw_cword(cx, cy + 12, w4);
}

void con_putc (char c)
{
    _con_putc(fb_con.cur_x, fb_con.cur_y, c);

    fb_con.cur_x++;

    if ((fb_con.cur_x >= fb_con.c_wd) || (c == '\n')) {
        fb_con.cur_x = 0;

        fb_con.cur_y++;
        if (fb_con.cur_y >= fb_con.c_ht) {
            fb_con.cur_y = 0;
        }
    }
}

void con_puts (char *s)
{
    while (*s) {
        con_putc(*s);
        s++;
    }
}

void fb_init ()
{
    fb.wd = 1280;
    fb.ht = 1024;
    fb.v_wd = 1280;
    fb.v_ht = 1024;
    fb.pitch = 0;
    fb.depth = 16;
    fb.x = 0;
    fb.y = 0;
    fb.ptr = 0;
    fb.size = 0;

    do {
        // need to add 0x40000000 to use uncache alias
        mbox0_write ((uint32)(&fb) | 0x40000000, 1);
        mbox0_read (1);
    } while (fb.ptr == 0);

    fb.ptr -= 0x40000000; // convert framebuf address to ARM physical addr

    fb_con.c_wd = (fb.wd >> CH_XSHIFT);
    fb_con.c_ht = (fb.ht >> CH_YSHIFT);
    fb_con.cur_x = fb_con.cur_y = 0;

    con_puts("hello world\n");
    con_puts ("I am OK...\n");
}