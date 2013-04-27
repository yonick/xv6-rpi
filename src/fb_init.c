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


// a character is 8-bit wide, and 16-bit high, each font
// is thus 16 bytes
#define CH_XSHIFT        3
#define CH_YSHIFT        4
#define CH_COLOR    0xFFFF

struct {
    uint32  wd;     // screen width
    uint32  ht;     // screen height
    uint32  c_wd;   // how many characters in a row
    uint32  c_ht;   // how many lines in a column
    uint32  cur_x;  // current x position
    uint32  cur_y;  // current y position
    uint32  ptr;    // frame buffer address (physical address)
} fb_con;

extern uint32   font_addr;  // symbole defined in entry.S
uint32*         fonts = &font_addr;
uint16*         fb_ptr;


// draw a line of character
static void inline draw_cline (int x, int y, uint32 l)
{
    int b;

    l &= 0xFF;
    b  = x + y * fb_con.wd;

    while (l != 0) {
        if (l & 0x01) {
            fb_ptr[b] = CH_COLOR;
        }

        l >>= 1;
        b++;
    }
}

static void inline draw_cword (int x, int y, uint32 w)
{
    draw_cline(x, y, w);
    draw_cline(x, y + 1, w >> 8);
    draw_cline(x, y + 2, w >> 16);
    draw_cline(x, y + 3, w >> 24);
}

// write a character at position (cx, cy). cx and cy are character,
// offset, not pixle offset. 
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

// clear the screen
void clear_screen ()
{
    uint32 *p;
    int n;

    // clear the screen, write 32 bits a time
    p = (uint32*)fb_ptr;
    n = (fb_con.wd * fb_con.ht) >> 1;

    while (n >= 0) {
        p[n--] = 0x00;
    }
}

// write a character at the current poistion
void con_putc (char c)
{
    _con_putc(fb_con.cur_x, fb_con.cur_y, c);

    fb_con.cur_x++;

    if ((fb_con.cur_x >= fb_con.c_wd) || (c == '\n')) {
        fb_con.cur_x = 0;
        fb_con.cur_y++;

        if (fb_con.cur_y >= fb_con.c_ht) {
            fb_con.cur_y = 0;
            clear_screen();
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

void con_putint (char *prefix, uint val, char* suffix)
{
    char* arr = "0123456789ABCDEF";
    int idx;

    if (prefix) {
        con_puts (prefix);
    }

    for (idx = sizeof(val) * 8 - 4; idx >= 0; idx -= 4) {
        con_putc(arr[(val >> idx) & 0x0F]);
    }

    if (suffix) {
        con_puts(suffix);
    }
}

void fb_init ()
{
    // framebuffer information: used to communicate with GPU to
    // set resolution and retrieve framebuffer address. This
    // data structure needs to be aligned at 16-bytes. GCC will
    // generate correct code to align it (even it is a local variable).
    struct fb_info {
        uint32 wd;
        uint32 ht;
        uint32 v_wd;
        uint32 v_ht;
        uint32 pitch;       // GPU - Pitch
        uint32 depth;
        uint32 x;
        uint32 y;
        uint32 ptr;         // GPU - framebuffer pointer
        uint32 size;        // GPU - size
    } fb __attribute__ ((aligned (16))) = {
        1024, 768, 1024, 768, 0, 16, 0, 0, 0, 0
    };

    do {
        // need to add 0x40000000 to use uncached alias
        mbox0_write ((uint32)(&fb) | 0x40000000, 1);
        mbox0_read (1);
    } while (fb.ptr == 0);

    fb.ptr -= 0x40000000; // convert framebuf address to ARM physical addr

    // save the info to fb_con, a clear data structure
    fb_con.wd   = fb.wd;
    fb_con.ht   = fb.ht;
    fb_con.ptr  = fb.ptr;
    
    fb_con.c_wd = (fb_con.wd >> CH_XSHIFT);
    fb_con.c_ht = (fb_con.ht >> CH_YSHIFT);
    fb_con.cur_x = fb_con.cur_y = 0;
    fb_ptr = (uint16*)(fb_con.ptr);

    con_puts ("Frame-buffer based console initialized...");
}