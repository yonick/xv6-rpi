#include "types.h"
#include "arm.h"
#include "defs.h"

const unsigned char _ctype[] = {
_C,_C,_C,_C,_C,_C,_C,_C,                        /* 0-7 */
_C,_C|_S,_C|_S,_C|_S,_C|_S,_C|_S,_C,_C,         /* 8-15 */
_C,_C,_C,_C,_C,_C,_C,_C,                        /* 16-23 */
_C,_C,_C,_C,_C,_C,_C,_C,                        /* 24-31 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,                    /* 32-39 */
_P,_P,_P,_P,_P,_P,_P,_P,                        /* 40-47 */
_D,_D,_D,_D,_D,_D,_D,_D,                        /* 48-55 */
_D,_D,_P,_P,_P,_P,_P,_P,                        /* 56-63 */
_P,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U|_X,_U,      /* 64-71 */
_U,_U,_U,_U,_U,_U,_U,_U,                        /* 72-79 */
_U,_U,_U,_U,_U,_U,_U,_U,                        /* 80-87 */
_U,_U,_U,_P,_P,_P,_P,_P,                        /* 88-95 */
_P,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L|_X,_L,      /* 96-103 */
_L,_L,_L,_L,_L,_L,_L,_L,                        /* 104-111 */
_L,_L,_L,_L,_L,_L,_L,_L,                        /* 112-119 */
_L,_L,_L,_P,_P,_P,_P,_C,                        /* 120-127 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                /* 128-143 */
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,                /* 144-159 */
_S|_SP,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,   /* 160-175 */
_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,_P,       /* 176-191 */
_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,_U,       /* 192-207 */
_U,_U,_U,_U,_U,_U,_U,_P,_U,_U,_U,_U,_U,_U,_U,_L,       /* 208-223 */
_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,_L,       /* 224-239 */
_L,_L,_L,_L,_L,_L,_L,_P,_L,_L,_L,_L,_L,_L,_L,_L};      /* 240-255 */


void* memset(void *dst, int v, int n)
{
    uint8	*p;
    uint8	c;
    uint32	val;
    uint32	*p4;

    p   = dst;
    c   = v & 0xff;
    val = (c << 24) | (c << 16) | (c << 8) | c;

    // set bytes before whole uint32
    for (; (n > 0) && ((uint)p % 4); n--, p++){
        *p = c;
    }

    // set memory 4 bytes a time
    p4 = (uint*)p;

    for (; n >= 4; n -= 4, p4++) {
        *p4 = val;
    }

    // set leftover one byte a time
    p = (uint8*)p4;

    for (; n > 0; n--, p++) {
        *p = c;
    }

    return dst;
}


int memcmp(const void *v1, const void *v2, uint n)
{
    const uchar *s1, *s2;

    s1 = v1;
    s2 = v2;

    while(n-- > 0){
        if(*s1 != *s2) {
            return *s1 - *s2;
        }

        s1++, s2++;
    }

    return 0;
}

void* memmove(void *dst, const void *src, uint n)
{
    const char *s;
    char *d;

    s = src;
    d = dst;

    if(s < d && s + n > d){
        s += n;
        d += n;

        while(n-- > 0) {
            *--d = *--s;
        }

    } else {
        while(n-- > 0) {
            *d++ = *s++;
        }
    }

    return dst;
}

// memcpy exists to placate GCC.  Use memmove.
void* memcpy(void *dst, const void *src, uint n)
{
    return memmove(dst, src, n);
}

int strncmp(const char *p, const char *q, uint n)
{
    while(n > 0 && *p && *p == *q) {
        n--, p++, q++;
    }

    if(n == 0) {
        return 0;
    }

    return (uchar)*p - (uchar)*q;
}

char* strncpy(char *s, const char *t, int n)
{
    char *os;

    os = s;

    while(n-- > 0 && (*s++ = *t++) != 0)
        ;

    while(n-- > 0) {
        *s++ = 0;
    }

    return os;
}

// Like strncpy but guaranteed to NUL-terminate.
char* safestrcpy(char *s, const char *t, int n)
{
    char *os;

    os = s;

    if(n <= 0) {
        return os;
    }

    while(--n > 0 && (*s++ = *t++) != 0)
        ;

    *s = 0;
    return os;
}

int strlen(const char *s)
{
    int n;

    for(n = 0; s[n]; n++)
        ;

    return n;
}

