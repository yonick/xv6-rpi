// implement a text console, fb_init.c contains initial support
// running at low memory. As soon as we enable the user process,
// fb_init.c will gone. This file runs at kernel space
#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"

// a character is 8-bit wide, and 16-bit high, each font
// is thus 16 bytes
#define CH_XSHIFT        3
#define CH_YSHIFT        4
#define CH_COLOR    0xFFFF

struct _fb_con  fbcon_hi;
static uint32*  fonts;
static uint16*  fb_ptr;

void cga_init (struct _fb_con *fbcon_lo)
{
    fbcon_hi = *fbcon_lo;
    fbcon_hi.ptr = _P2V(fbcon_hi.ptr);
    fonts = P2V(&font_img);
    fb_ptr = (uint16*)fbcon_hi.ptr;
}

// draw a line of character
static void inline draw_cline (int x, int y, uint32 l)
{
    int b;

    l &= 0xFF;
    b  = x + y * fbcon_hi.wd;

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
static void _cga_putc (int cx, int cy, int ch)
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
void cga_clear ()
{
    uint32 *p;
    int n;

    // clear the screen, write 32 bits a time
    p = (uint32*)fb_ptr;
    n = (fbcon_hi.wd * fbcon_hi.ht) >> 1;

    while (n >= 0) {
        p[n--] = 0x00;
    }
}

// write a character at the current poistion
void cga_putc (char c)
{
    _cga_putc(fbcon_hi.cur_x, fbcon_hi.cur_y, c);

    fbcon_hi.cur_x++;

    if ((fbcon_hi.cur_x >= fbcon_hi.c_wd) || (c == '\n')) {
        fbcon_hi.cur_x = 0;
        fbcon_hi.cur_y++;

        if (fbcon_hi.cur_y >= fbcon_hi.c_ht) {
            fbcon_hi.cur_y = 0;
            cga_clear();
        }
    }
}