/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * 
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver v1.3 for Mesa  --  MGA2064W
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 *
 * Thanks to Shawn Hargreaves for FreeBE/AF
 */


#include <dpmi.h>
#include <stdlib.h>
#include <string.h>

#include "../internal.h"
#include "mga_reg.h"
#include "mga_hw.h"
#include "mga_mode.h"
#include "mga.h"



/* cached drawing engine state */
#define OP_NONE 0

#define OP_DRAWRECT (\
           M_DWG_TRAP |           /* opcod */    \
           M_DWG_BLK |            /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
           M_DWG_SOLID |          /* solid */    \
           M_DWG_ARZERO |         /* arzero */   \
           M_DWG_SGNZERO |        /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC          /* bop */      \
                                  /* trans */    \
                                  /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWRECT_TX32BGR (\
           M_DWG_TEXTURE_TRAP |   /* opcod */    \
           M_DWG_I |              /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
                                  /* solid */    \
           M_DWG_ARZERO |         /* arzero */   \
           M_DWG_SGNZERO |        /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BU32BGR          /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWRECT_TX24BGR (\
           M_DWG_TEXTURE_TRAP |   /* opcod */    \
           M_DWG_I |              /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
                                  /* solid */    \
           M_DWG_ARZERO |         /* arzero */   \
           M_DWG_SGNZERO |        /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BU24BGR          /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWLINE (\
           M_DWG_AUTOLINE_CLOSE | /* opcod */    \
           M_DWG_RPL |            /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
           M_DWG_SOLID |          /* solid */    \
                                  /* arzero */   \
                                  /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BFCOL            /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWLINE_I (\
           M_DWG_AUTOLINE_CLOSE | /* opcod */    \
           M_DWG_I |              /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
                                  /* solid */    \
                                  /* arzero */   \
                                  /* sgnzero */  \
                                  /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BFCOL            /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWLINE_ZI (\
           M_DWG_AUTOLINE_CLOSE | /* opcod */    \
           M_DWG_ZI |             /* atype */    \
                                  /* linear */   \
           M_DWG_ZLT |            /* zmode */    \
                                  /* solid */    \
                                  /* arzero */   \
                                  /* sgnzero */  \
                                  /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BFCOL            /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWTRAP (\
           M_DWG_TRAP |           /* opcod */    \
           M_DWG_BLK |            /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
           M_DWG_SOLID |          /* solid */    \
                                  /* arzero */   \
                                  /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC          /* bop */      \
                                  /* trans */    \
                                  /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWTRAP_I (\
           M_DWG_TRAP |           /* opcod */    \
           M_DWG_I |              /* atype */    \
                                  /* linear */   \
           M_DWG_NOZCMP |         /* zmode */    \
                                  /* solid */    \
                                  /* arzero */   \
                                  /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC          /* bop */      \
                                  /* trans */    \
                                  /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_DRAWTRAP_ZI (\
           M_DWG_TRAP |           /* opcod */    \
           M_DWG_ZI |             /* atype */    \
                                  /* linear */   \
           M_DWG_ZLT |            /* zmode */    \
                                  /* solid */    \
                                  /* arzero */   \
                                  /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC          /* bop */      \
                                  /* trans */    \
                                  /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_ILOAD_32BGR (\
           M_DWG_ILOAD |          /* opcod */    \
           M_DWG_RPL |            /* atype */    \
                                  /* linear */   \
                                  /* zmode */    \
                                  /* solid */    \
                                  /* arzero */   \
           M_DWG_SGNZERO |        /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BU32BGR          /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )
#define OP_ILOAD_24BGR (\
           M_DWG_ILOAD |          /* opcod */    \
           M_DWG_RPL |            /* atype */    \
                                  /* linear */   \
                                  /* zmode */    \
                                  /* solid */    \
                                  /* arzero */   \
           M_DWG_SGNZERO |        /* sgnzero */  \
           M_DWG_SHFTZERO |       /* shftzero */ \
           M_DWG_BOP_SRC |        /* bop */      \
                                  /* trans */    \
           M_DWG_BU24BGR          /* bltmod */   \
                                  /* pattern */  \
                                  /* transc */   )



/* internal hardware data structures */
static int interleave;
static unsigned long zorg;
static unsigned long vram;
static char card_name[80];



/* some info about current mode */
static int __bpp, __bypp;
static int __pixwidth, __bytwidth, __pagewidth, __width, __height, __zheight;
static int __operation;
static int __scrollx, __scrolly;



/* buffers */
static int mga_readbuffer, mga_writebuffer;
static long mga_readbuffer_ptr, mga_writebuffer_ptr;
static long mga_backbuffer_ptr, mga_frontbuffer_ptr;



/* lookup table for scaling 2 bit colors up to 8 bits */
static int _rgb_scale_2[4] = {
       0, 85, 170, 255
};

/* lookup table for scaling 3 bit colors up to 8 bits */
static int _rgb_scale_3[8] = {
       0, 36, 73, 109, 146, 182, 219, 255
};

/* lookup table for scaling 5 bit colors up to 8 bits */
static int _rgb_scale_5[32] = {
   0,   8,   16,  25,  33,  41,  49,  58,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   197, 206, 214, 222, 230, 239, 247, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static int _rgb_scale_6[64] = {
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  45,  49,  53,  57,  61,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};



/*
 * pixel/color routines
 */
void (*mga_putpixel) (unsigned int offset, int color);
int (*mga_getpixel) (unsigned int offset);
void (*mga_getrgba) (unsigned int offset, unsigned char rgba[4]);
int (*mga_mixrgb) (const unsigned char rgb[]);
static int (*mga_mixrgb_full) (const unsigned char rgb[]);



/* mga_fifo:
 *  Waits until there are at least <n> free slots in the FIFO buffer.
 */
#define mga_fifo(n) do { } while (mga_inb(M_FIFOSTATUS) < (n))



static int _mga_rread (int port, int index)
{
 mga_select();
 mga_outb(port, index);
 return mga_inb(port+1);
}



static void _mga_rwrite (int port, int index, int v)
{
 mga_select();
 mga_outb(port, index);
 mga_outb(port+1, v);
}



static void _mga_ralter (int port, int index, int mask, int v)
{
 int temp;
 temp = _mga_rread(port, index);
 temp &= (~mask);
 temp |= (v & mask);
 _mga_rwrite(port, index, temp);
}



/* WaitTillIdle:
 *  Delay until the hardware controller has finished drawing.
 */
void mga_wait_idle (void)
{
 int tries = 2;

 /*hwptr_unselect(oldptr);*/

 mga_select();

 while (tries--) {
       do {
       } while (!(mga_inl(M_FIFOSTATUS) & 0x200));

       do {
       } while (mga_inl(M_STATUS) & 0x10000);

       mga_outb(M_CRTC_INDEX, 0);
 }

 /*hwptr_select(oldptr);*/
}



/* Desc: Waits for the next vertical sync period.
 *
 * In  :
 * Out :
 *
 * Note:
 */
static void _mga_wait_retrace (void)
{
 int t1 = 0;
 int t2 = 0;

 do {
     t1 = t2;
     t2 = mga_inl(M_VCOUNT);
 } while (t2 >= t1);
}



/* Desc: fix scan lines
 *
 * In  :
 * Out :
 *
 * Note:
 */
static unsigned long _mga_fix_scans (unsigned long l)
{
 unsigned long m = 0;

 switch (__bpp) {
        case 8:
             m = interleave?128:64;
             break;
        case 15:
        case 16:
             m = interleave?64:32;
             break;
        case 24:
             m = interleave?128:64;
             break;
        case 32:
             m = 32;
             break;
 }

 m -= 1;
 return (l + m) & ~m;
}



/* Desc: HW scrolling function
 *
 * In  :
 * Out :
 *
 * Note: view Z-buffer in 16bit modes: _mga_display_start(0, 0, __height, 1)
 */
void mga_display_start (long boffset, long x, long y, long waitVRT)
{
 long addr;

 mga_select();

 if (waitVRT >= 0) {

    addr = __bytwidth * y + (boffset + x) * __bypp;

    if (interleave) {
       addr /= 8;
    } else {
       addr /= 4;
    }

    _mga_rwrite(M_CRTC_INDEX, 0x0D, (addr)&0xFF);
    _mga_rwrite(M_CRTC_INDEX, 0x0C, (addr>>8)&0xFF);
    _mga_ralter(M_CRTC_EXT_INDEX, 0, 0x0F, (addr>>16)&0x0F);

    while (waitVRT--) {
          _mga_wait_retrace();
    }
 }

 __scrollx = x;
 __scrolly = y;
}



/* Desc: set READ buffer
 *
 * In  : either FRONT or BACK buffer
 * Out :
 *
 * Note:
 */
void mga_set_readbuffer (int buffer)
{
 mga_readbuffer = buffer;

 mga_readbuffer_ptr = (mga_readbuffer == MGA_FRONTBUFFER) ? mga_frontbuffer_ptr : mga_backbuffer_ptr;
}



/* Desc: set WRITE buffer
 *
 * In  : either FRONT or BACK buffer
 * Out :
 *
 * Note:
 */
void mga_set_writebuffer (int buffer)
{
 mga_writebuffer = buffer;

 mga_writebuffer_ptr = (mga_writebuffer == MGA_FRONTBUFFER) ? mga_frontbuffer_ptr : mga_backbuffer_ptr;

 mga_select();
 mga_fifo(1);
 mga_outl(M_YDSTORG, mga_writebuffer_ptr);

 __operation = OP_NONE;
}



/* Desc: swap buffers
 *
 * In  : number of vertical retraces to wait
 * Out :
 *
 * Note:
 */
void mga_swapbuffers (int swapinterval)
{
 /* flip the buffers */
 mga_backbuffer_ptr ^= __pagewidth;
 mga_frontbuffer_ptr ^= __pagewidth;

 /* update READ/WRITE pointers */
 mga_set_readbuffer(mga_readbuffer);
 mga_set_writebuffer(mga_writebuffer);

 /* make sure we always see the FRONT buffer */
 mga_display_start(mga_frontbuffer_ptr, __scrollx, __scrolly, swapinterval);
}



/* Desc: color composition (w/o ALPHA)
 *
 * In  : array of integers (R, G, B)
 * Out : color
 *
 * Note: -
 */
static __inline int _mga_mixrgb8 (const unsigned char rgb[])
{
 return (rgb[0]&0xe0)|((rgb[1]>>5)<<2)|(rgb[2]>>6);
}
static __inline int _mga_mixrgb15 (const unsigned char rgb[])
{
 return ((rgb[0]>>3)<<10)|((rgb[1]>>3)<<5)|(rgb[2]>>3);
}
static __inline int _mga_mixrgb16 (const unsigned char rgb[])
{
 return ((rgb[0]>>3)<<11)|((rgb[1]>>2)<<5)|(rgb[2]>>3);
}
static __inline int _mga_mixrgb32 (const unsigned char rgb[])
{
 return (rgb[0]<<16)|(rgb[1]<<8)|(rgb[2]);
}



/* Desc: color composition (w/o ALPHA) + replication
 *
 * In  : array of integers (R, G, B)
 * Out : color
 *
 * Note: -
 */
static int _mga_mixrgb8_full (const unsigned char rgb[])
{
 int color = _mga_mixrgb8(rgb);
 color |= color<<8;
 return (color<<16) | color;
}
static int _mga_mixrgb15_full (const unsigned char rgb[])
{
 int color = _mga_mixrgb15(rgb);
 return (color<<16) | color;
}
static int _mga_mixrgb16_full (const unsigned char rgb[])
{
 int color = _mga_mixrgb16(rgb);
 return (color<<16) | color;
}
#define _mga_mixrgb32_full _mga_mixrgb32



/* Desc: putpixel
 *
 * In  : pixel offset, pixel value
 * Out : -
 *
 * Note: uses current write buffer
 */
static void _mga_putpixel8 (unsigned int offset, int color)
{
 hwptr_pokeb(mgaptr.linear_map, mga_writebuffer_ptr + offset, color);
}
#define _mga_putpixel15 _mga_putpixel16
static void _mga_putpixel16 (unsigned int offset, int color)
{
 hwptr_pokew(mgaptr.linear_map, (mga_writebuffer_ptr + offset) * 2, color);
}
static void _mga_putpixel32 (unsigned int offset, int color)
{
 hwptr_pokel(mgaptr.linear_map, (mga_writebuffer_ptr + offset) * 4, color);
}



/* Desc: pixel retrieval
 *
 * In  : pixel offset
 * Out : pixel value
 *
 * Note: uses current read buffer
 */
static __inline int _mga_getpixel8 (unsigned int offset)
{
 return hwptr_peekb(mgaptr.linear_map, mga_readbuffer_ptr + offset);
}
#define _mga_getpixel15 _mga_getpixel16
static __inline int _mga_getpixel16 (unsigned int offset)
{
 return hwptr_peekw(mgaptr.linear_map, (mga_readbuffer_ptr + offset) * 2);
}
static __inline int _mga_getpixel32 (unsigned int offset)
{
 return hwptr_peekl(mgaptr.linear_map, (mga_readbuffer_ptr + offset) * 4);
}



/* Desc: color decomposition
 *
 * In  : pixel offset, array of integers to hold color components (R, G, B, A)
 * Out : -
 *
 * Note: uses current read buffer
 */
static void _mga_getrgba8 (unsigned int offset, unsigned char rgba[4])
{
 int c = _mga_getpixel8(offset);
 rgba[0] = _rgb_scale_3[(c >> 5) & 0x7];
 rgba[1] = _rgb_scale_3[(c >> 2) & 0x7];
 rgba[2] = _rgb_scale_2[c & 0x3];
 rgba[3] = 255;
}
static void _mga_getrgba15 (unsigned int offset, unsigned char rgba[4])
{
 int c = _mga_getpixel15(offset);
 rgba[0] = _rgb_scale_5[(c >> 10) & 0x1F];
 rgba[1] = _rgb_scale_5[(c >> 5) & 0x1F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
static void _mga_getrgba16 (unsigned int offset, unsigned char rgba[4])
{
 int c = _mga_getpixel16(offset);
 rgba[0] = _rgb_scale_5[(c >> 11) & 0x1F];
 rgba[1] = _rgb_scale_6[(c >> 5) & 0x3F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
static void _mga_getrgba32 (unsigned int offset, unsigned char rgba[4])
{
 int c = _mga_getpixel32(offset);
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c; 
 rgba[3] = c >> 24;
}



/* Desc: RGB flat line
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_draw_line_rgb_flat (const MGAvertex *v1, const MGAvertex *v2)
{
 unsigned long color;
 int x1 = v1->win[0];
 int y1 = v1->win[1];
 int x2 = v2->win[0];
 int y2 = v2->win[1];

 if ((x1 == x2) && (y1 == y2)) {
    return;
 }

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWLINE) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWLINE);
    __operation = OP_DRAWLINE;
 }

 color = mga_mixrgb_full(v2->color);

 /* draw the line */
 mga_fifo(3);
 mga_outl(M_FCOL, color);
 mga_outl(M_XYSTRT, (y1<<16) | x1);
 mga_outl(M_XYEND | M_EXEC, (y2<<16) | x2);
}



/* Desc: RGB flat Z-less line
 *
 * In  :
 * Out :
 *
 * Note: I never figured out "diagonal increments"
 */
void mga_draw_line_rgb_flat_zless (const MGAvertex *v1, const MGAvertex *v2)
{
 int z1, dz;
 int x1 = v1->win[0];
 int y1 = v1->win[1];
 int x2 = v2->win[0];
 int y2 = v2->win[1];
 int dx = abs(x2 - x1);
 int dy = abs(y2 - y1);

 if ((dx == 0) && (dy == 0)) {
    return;
 }

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWLINE_ZI) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWLINE_ZI);
    __operation = OP_DRAWLINE_ZI;
 }

 if (dx < dy) {
    dx = dy;
 }

 z1 = v1->win[2] << 15;
 dz = ((v2->win[2] << 15) - z1) / dx;

 /* draw the line */
 mga_fifo(14);
 mga_outl(M_DR0, z1);
 mga_outl(M_DR2, dz);
 mga_outl(M_DR3, dz);
 mga_outl(M_DR4, v2->color[0] << 15);
 mga_outl(M_DR6, 0);
 mga_outl(M_DR7, 0);
 mga_outl(M_DR8, v2->color[1] << 15);
 mga_outl(M_DR10, 0);
 mga_outl(M_DR11, 0);
 mga_outl(M_DR12, v2->color[2] << 15);
 mga_outl(M_DR14, 0);
 mga_outl(M_DR15, 0);
 mga_outl(M_XYSTRT, (y1<<16) | x1);
 mga_outl(M_XYEND | M_EXEC, (y2<<16) | x2);
}



/* Desc: RGB iterated line
 *
 * In  :
 * Out :
 *
 * Note: I never figured out "diagonal increments"
 */
void mga_draw_line_rgb_iter (const MGAvertex *v1, const MGAvertex *v2)
{
 int r1, g1, b1;
 int dr, dg, db;
 int x1 = v1->win[0];
 int y1 = v1->win[1];
 int x2 = v2->win[0];
 int y2 = v2->win[1];
 int dx = abs(x2 - x1);
 int dy = abs(y2 - y1);

 if ((dx == 0) && (dy == 0)) {
    return;
 }

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWLINE_I) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWLINE_I);
    __operation = OP_DRAWLINE_I;
 }

 if (dx < dy) {
    dx = dy;
 }

 r1 = v1->color[0] << 15;
 g1 = v1->color[1] << 15;
 b1 = v1->color[2] << 15;
 dr = ((v2->color[0] << 15) - r1) / dx;
 dg = ((v2->color[1] << 15) - g1) / dx;
 db = ((v2->color[2] << 15) - b1) / dx;

 /* draw the line */
 mga_fifo(11);
 mga_outl(M_DR4, r1);
 mga_outl(M_DR6, dr);
 mga_outl(M_DR7, dr);
 mga_outl(M_DR8, g1);
 mga_outl(M_DR10, dg);
 mga_outl(M_DR11, dg);
 mga_outl(M_DR12, b1);
 mga_outl(M_DR14, db);
 mga_outl(M_DR15, db);
 mga_outl(M_XYSTRT, (y1<<16) | x1);
 mga_outl(M_XYEND | M_EXEC, (y2<<16) | x2);
}



/* Desc: RGB iterated Z-less line
 *
 * In  :
 * Out :
 *
 * Note: I never figured out "diagonal increments"
 */
void mga_draw_line_rgb_iter_zless (const MGAvertex *v1, const MGAvertex *v2)
{
 int z1, dz;
 int r1, g1, b1;
 int dr, dg, db;
 int x1 = v1->win[0];
 int y1 = v1->win[1];
 int x2 = v2->win[0];
 int y2 = v2->win[1];
 int dx = abs(x2 - x1);
 int dy = abs(y2 - y1);

 if ((dx == 0) && (dy == 0)) {
    return;
 }

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWLINE_ZI) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWLINE_ZI);
    __operation = OP_DRAWLINE_ZI;
 }

 if (dx < dy) {
    dx = dy;
 }

 z1 = v1->win[2] << 15;
 dz = ((v2->win[2] << 15) - z1) / dx;

 r1 = v1->color[0] << 15;
 g1 = v1->color[1] << 15;
 b1 = v1->color[2] << 15;
 dr = ((v2->color[0] << 15) - r1) / dx;
 dg = ((v2->color[1] << 15) - g1) / dx;
 db = ((v2->color[2] << 15) - b1) / dx;

 /* draw the line */
 mga_fifo(14);
 mga_outl(M_DR0, z1);
 mga_outl(M_DR2, dz);
 mga_outl(M_DR3, dz);
 mga_outl(M_DR4, r1);
 mga_outl(M_DR6, dr);
 mga_outl(M_DR7, dr);
 mga_outl(M_DR8, g1);
 mga_outl(M_DR10, dg);
 mga_outl(M_DR11, dg);
 mga_outl(M_DR12, b1);
 mga_outl(M_DR14, db);
 mga_outl(M_DR15, db);
 mga_outl(M_XYSTRT, (y1<<16) | x1);
 mga_outl(M_XYEND | M_EXEC, (y2<<16) | x2);
}



/* Desc: RGB flat triangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
#define TAG mga_draw_tri_rgb_flat
#define CULL
#define SETUP_CODE                   \
 if (__operation != OP_DRAWTRAP) {   \
    mga_fifo(1);                     \
    mga_outl(M_DWGCTL, OP_DRAWTRAP); \
    __operation = OP_DRAWTRAP;       \
 }
#include "m_ttemp.h"



/* Desc: RGB flat Z-less triangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
#define TAG mga_draw_tri_rgb_flat_zless
#define CULL
#define INTERP_Z
#define SETUP_CODE                      \
 if (__operation != OP_DRAWTRAP_ZI) {   \
    mga_fifo(1);                        \
    mga_outl(M_DWGCTL, OP_DRAWTRAP_ZI); \
    __operation = OP_DRAWTRAP_ZI;       \
 }
#include "m_ttemp.h"



/* Desc: RGB iterated triangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
#define TAG mga_draw_tri_rgb_iter
#define CULL
#define INTERP_RGB
#define SETUP_CODE                     \
 if (__operation != OP_DRAWTRAP_I) {   \
    mga_fifo(1);                       \
    mga_outl(M_DWGCTL, OP_DRAWTRAP_I); \
    __operation = OP_DRAWTRAP_I;       \
 }
#include "m_ttemp.h"



/* Desc: RGB iterated Z-less triangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
#define TAG mga_draw_tri_rgb_iter_zless
#define CULL
#define INTERP_Z
#define INTERP_RGB
#define SETUP_CODE                      \
 if (__operation != OP_DRAWTRAP_ZI) {   \
    mga_fifo(1);                        \
    mga_outl(M_DWGCTL, OP_DRAWTRAP_ZI); \
    __operation = OP_DRAWTRAP_ZI;       \
 }
#include "m_ttemp.h"



/* Desc: RGB flat rectangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_draw_rect_rgb_flat (int left, int top, int width, int height, int color)
{
 if (__bpp == 8) {
    color |= color << 8;
 }
 if (__bpp <= 16) {
    color |= color << 16;
 }

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWRECT) {

    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWRECT);
    __operation = OP_DRAWRECT;
 }

 /* draw the rectangle */
 mga_fifo(3);
 mga_outl(M_FCOL, color);
 mga_outl(M_FXBNDRY, ((left+width)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | height);
}



/* Desc: 32RGB textured span
 *
 * In  :
 * Out :
 *
 * Note: 0 <= width <= 7*1024
 */
void mga_draw_span_rgb_tx32 (int left, int top, int width, const unsigned long *bitmap)
{
 int i;

 if (!width) {
    return;
 }

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWRECT_TX32BGR) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWRECT_TX32BGR);
    __operation = OP_DRAWRECT_TX32BGR;
 }

 /* draw the rectangle */
 mga_fifo(2);
 mga_outl(M_FXBNDRY, ((left+width)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | 1);

 /* copy data to the pseudo-dma window */
 i = 0;
 do {
     mga_outl(i, *bitmap);
     bitmap++;
     i += 4;
 } while (--width);
}



/* Desc: 24RGB textured span
 *
 * In  :
 * Out :
 *
 * Note: 0 <= width <= 7*1024
 */
void mga_draw_span_rgb_tx24 (int left, int top, int width, const unsigned long *bitmap)
{
 int i;

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWRECT_TX24BGR) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWRECT_TX24BGR);
    __operation = OP_DRAWRECT_TX24BGR;
 }

 /* draw the rectangle */
 mga_fifo(2);
 mga_outl(M_FXBNDRY, ((left+width)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | 1);

 /* copy data to the pseudo-dma window */
 i = 0;
 width = (width * 3 + 3) / 4;
 while (width) {
       mga_outl(i & (7 * 1024 - 1), *bitmap);
       bitmap++;
       i += 4;
       width--;
 }
}



/* Desc: 32RGB textured rectangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_draw_rect_rgb_tx32 (int left, int top, int width, int height, const unsigned long *bitmap)
{
 int i;

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWRECT_TX32BGR) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWRECT_TX32BGR);
    __operation = OP_DRAWRECT_TX32BGR;
 }

 /* draw the rectangle */
 mga_fifo(2);
 mga_outl(M_FXBNDRY, ((left+width)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | height);

 /* copy data to the pseudo-dma window */
 i = 0;
 width *= height;
 while (width) {
       mga_outl(i & (7 * 1024 - 1), *bitmap);
       bitmap++;
       i += 4;
       width--;
 }
}



/* Desc: 24RGB textured rectangle
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_draw_rect_rgb_tx24 (int left, int top, int width, int height, const unsigned long *bitmap)
{
 int i;

 mga_select();

 /* set engine state */
 if (__operation != OP_DRAWRECT_TX24BGR) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_DRAWRECT_TX24BGR);
    __operation = OP_DRAWRECT_TX24BGR;
 }

 /* draw the rectangle */
 mga_fifo(2);
 mga_outl(M_FXBNDRY, ((left+width)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | height);

 /* copy data to the pseudo-dma window */
 i = 0;
 width = (width * height * 3 + 3) / 4;
 while (width) {
       mga_outl(i & (7 * 1024 - 1), *bitmap);
       bitmap++;
       i += 4;
       width--;
 }
}



/* Desc: copy 32RGB image to screen
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_iload_32RGB (int left, int top, int width, int height, const unsigned long *bitmap)
{
 int i;

 mga_select();

 /* set engine state */
 if (__operation != OP_ILOAD_32BGR) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_ILOAD_32BGR);
    __operation = OP_ILOAD_32BGR;
 }

 /* draw the bitmap */
 mga_fifo(5);
 mga_outl(M_AR0, width-1);
 mga_outl(M_AR3, 0);
 mga_outl(M_AR5, 0);
 mga_outl(M_FXBNDRY, ((left+width-1)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | height);

 /* copy data to the pseudo-dma window */
 i = 0;
 width *= height;
 while (width) {
       mga_outl(i & (7 * 1024 - 1), *bitmap);
       bitmap++;
       i += 4;
       width--;
 }
}



/* Desc: copy 24RGB image to screen
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_iload_24RGB (int left, int top, int width, int height, const unsigned long *bitmap)
{
 int i;

 mga_select();

 /* set engine state */
 if (__operation != OP_ILOAD_24BGR) {
    mga_fifo(1);
    mga_outl(M_DWGCTL, OP_ILOAD_24BGR);
    __operation = OP_ILOAD_24BGR;
 }

 /* draw the bitmap */
 mga_fifo(5);
 mga_outl(M_AR0, width-1);
 mga_outl(M_AR3, 0);
 mga_outl(M_AR5, 0);
 mga_outl(M_FXBNDRY, ((left+width-1)<<16) | left);
 mga_outl(M_YDSTLEN | M_EXEC, (top<<16) | height);

 /* copy data to the pseudo-dma window */
 i = 0;
 width = (width * height * 3 + 3) / 4;
 while (width) {
       mga_outl(i & (7 * 1024 - 1), *bitmap);
       bitmap++;
       i += 4;
       width--;
 }
}



/* Desc: get Z-buffer value
 *
 * In  :
 * Out :
 *
 * Note:
 */
unsigned short mga_getz (int offset)
{
 return hwptr_peekw(mgaptr.linear_map, zorg + (mga_readbuffer_ptr + offset) * 2);
}



/* Desc: put Z-buffer value
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_setz (int offset, unsigned short z)
{
 hwptr_pokew(mgaptr.linear_map, zorg + (mga_writebuffer_ptr + offset) * 2, z);
}



/* Desc: clear Z-buffer
 *
 * In  :
 * Out :
 *
 * Note: uses current write buffer
 */
static void _mga_clear_zed (int left, int top, int width, int height, unsigned short z)
{
 if (__bpp == 16) {
    /* GPU store (high bandwidth)
     * Hack alert:
     * can cause problems with concurrent FB accesses
     */
    mga_select();
    mga_fifo(1);
    mga_outl(M_YDSTORG, mga_writebuffer_ptr + zorg/2);
    mga_draw_rect_rgb_flat(left, top, width, height, z);
    mga_fifo(1);
    mga_outl(M_YDSTORG, mga_writebuffer_ptr);
 } else {
    /* CPU store */
    unsigned long i, zz = (z<<16) | z;
    unsigned long ofs = zorg + (top * __pixwidth + left + mga_writebuffer_ptr) * 2;
    hwptr_select(mgaptr.linear_map);
    while (height--) {
          i = width/2;
          while (i--) {
                hwptr_nspokel(mgaptr.linear_map, ofs, zz);
                ofs += 4;
          }
          if (width & 1) {
             hwptr_nspokew(mgaptr.linear_map, ofs, z);
             ofs += 2;
          }
          ofs += (__pixwidth - width) * 2;
    }
 }
}



/* Desc: clear color- and Z-buffer
 *
 * In  : front  = clear front buffer
 *       back   = clear back buffer
 *       zed    = clear depth buffer
 *       left   = leftmost pixel to be cleared
 *       top    = starting line
 *       width  = number of pixels
 *       height = number of lines
 *       color  = color to clear to
 *       z      = z value (ignored if zed==0)
 * Out :
 *
 * Note:
 */
void mga_clear (int front, int back, int zed, int left, int top, int width, int height, int color, unsigned short z)
{
 if (front) {
    if (mga_writebuffer == MGA_FRONTBUFFER) {
       mga_draw_rect_rgb_flat(left, top, width, height, color);
       if (zed) {
          _mga_clear_zed(left, top, width, height, z);
       }
       front = 0;
    }
 }
 if (back) {
    if (mga_writebuffer == MGA_BACKBUFFER) {
       mga_draw_rect_rgb_flat(left, top, width, height, color);
       if (zed) {
          _mga_clear_zed(left, top, width, height, z);
       }
       back = 0;
    }
 }
 if (front) {
    int old = mga_writebuffer;
    mga_set_writebuffer(MGA_FRONTBUFFER);
    mga_draw_rect_rgb_flat(left, top, width, height, color);
    if (zed) {
       _mga_clear_zed(left, top, width, height, z);
    }
    mga_set_writebuffer(old);
    front = 0;
 }
 if (back) {
    int old = mga_writebuffer;
    mga_set_writebuffer(MGA_BACKBUFFER);
    mga_draw_rect_rgb_flat(left, top, width, height, color);
    if (zed) {
       _mga_clear_zed(left, top, width, height, z);
    }
    mga_set_writebuffer(old);
    back = 0;
 }
}



/* Desc: Attempts to enter specified video mode.
 *
 * In  : ptr to mode structure, number of pages, Z-buffer request, refresh rate
 * Out : 0 if success
 *
 * Note: also set up the accelerator engine
 */
int mga_open (int width, int height, int bpp, int buffers, int zbuffer, int refresh)
{
 static int mill_strides[] = { 640, 768, 800, 960, 1024, 1152, 1280, 1600, 1920, 2048, 0 };
 unsigned int i, used;
 MGA_MODE *p;

 if (mga_hw_init(&vram, &interleave, card_name) == 0) {
    return -1;
 }

 if ((p = mga_mode_find(width, height, bpp)) == NULL) {
    return -1;
 }

 __bpp = p->bpp;
 __width = __pagewidth = p->xres;
 __height = p->yres;

 if (buffers > 1) {
    __pagewidth = _mga_fix_scans(__pagewidth);
    __pixwidth = __pagewidth * buffers;
 } else {
    __pixwidth = __pagewidth;
    __pixwidth = _mga_fix_scans(__pixwidth);
 }

 for (i=0; mill_strides[i]; i++) {
     if (__pixwidth <= mill_strides[i]) {
        __pixwidth = mill_strides[i];
        break;
     }
 }

 __bypp = (__bpp+7)/8;
 __bytwidth = __pixwidth * __bypp;

 /* compute used memory: framebuffer + zbuffer */
 used = __bytwidth * __height;
 if (zbuffer) {
    zorg = (used + 511) & ~511;
    /* Hack alert:
     * a 16-bit Z-buffer size is (stride_in_pixels * number_of_lines * 2)
     * We cannot mess with the Z-buffer width, but we might decrease the
     * number of lines, if the user requests less than (screen_height). For
     * example with a 2MB card, one can have 640x480x16 display with 2 color
     * buffers and Z-buffer if the maximum requested height is 339:
     *    Total = (640*480 * 2 + 640*339 * 2) * 2
     * However, this means the user must not write beyond the window's height
     * and if we'll ever implement moveable windows, we'll have to reconsider
     * this hack.
     */
#if 1
    __zheight = height;  /* smaller */
    used = zorg + __pixwidth * 2 * __zheight;
#else
    __zheight = __height;
    used = zorg + __pixwidth * 2 * __zheight;
#endif
 }

 if (mill_strides[i] && (vram>=used)) {
    /* enter mode */
    mga_mode_switch(p, refresh);
    /* change the scan line length */
    _mga_ralter(M_CRTC_INDEX, 0x14, 0x40, 0);           /* disable DWORD */
    _mga_ralter(M_CRTC_INDEX, 0x17, 0x40, 0x40);        /* wbmode = BYTE */
    if (interleave) {
       _mga_rwrite(M_CRTC_INDEX, 0x13, __bytwidth/16);
       _mga_ralter(M_CRTC_EXT_INDEX, 0, 0x30, ((__bytwidth/16)>>4)&0x30);
    } else {
       _mga_rwrite(M_CRTC_INDEX, 0x13, __bytwidth/8);
       _mga_ralter(M_CRTC_EXT_INDEX, 0, 0x30, ((__bytwidth/8)>>4)&0x30);
    }
 } else {
    return -1;
 }

 /* setup buffers */
 mga_frontbuffer_ptr = 0;
 if (buffers > 1) {
    mga_backbuffer_ptr = __pagewidth;
    mga_set_readbuffer(MGA_BACKBUFFER);
    mga_set_writebuffer(MGA_BACKBUFFER);
 } else {
    mga_backbuffer_ptr = 0;
    mga_set_readbuffer(MGA_FRONTBUFFER);
    mga_set_writebuffer(MGA_FRONTBUFFER);
 }
 mga_display_start(mga_frontbuffer_ptr, __scrollx = 0, __scrolly = 0, 1);

 /* set up the accelerator engine */
 mga_select();

 mga_fifo(8);
 mga_outl(M_PITCH, __pixwidth);
 mga_outl(M_PLNWT, 0xFFFFFFFF);
 mga_outl(M_OPMODE, M_DMA_BLIT);
 mga_outl(M_CXBNDRY, 0xFFFF0000);
 mga_outl(M_YTOP, 0x00000000);
 mga_outl(M_YBOT, 0x007FFFFF);
 mga_outl(M_ZORG, zorg);

#define INITPTR(bpp) \
        mga_putpixel = _mga_putpixel##bpp; \
        mga_getrgba = _mga_getrgba##bpp;   \
        mga_getpixel = _mga_getpixel##bpp; \
        mga_mixrgb = _mga_mixrgb##bpp;     \
        mga_mixrgb_full = _mga_mixrgb##bpp##_full

 switch (__bpp) {
	case 8:
	     mga_outl(M_MACCESS, 0);
             INITPTR(8);
	     break;
	case 15:
	     mga_outl(M_MACCESS, 0x80000001);
             INITPTR(15);
	     break;
	case 16:
	     mga_outl(M_MACCESS, 1);
             INITPTR(16);
	     break;
	case 32:
	     mga_outl(M_MACCESS, 2);
             INITPTR(32);
	     break;
 }

#undef INITPTR

 /* disable VGA aperture */
 i = mga_inb(M_MISC_R);
 mga_outb(M_MISC_W, i & ~2);

 /* clear Z-buffer (if any) */
 if (zbuffer) {
    unsigned long ofs = zorg;
    unsigned long len = zorg + __pixwidth * 2 * __zheight;

    hwptr_select(mgaptr.linear_map);
    for (; ofs<len; ofs+=4) {
        hwptr_nspokel(mgaptr.linear_map, ofs, -1);
    }
 }

 return 0;
}



/* Desc:
 *
 * In  :
 * Out :
 *
 * Note:
 */
void mga_close (int restore, int unmap)
{
 if (restore) {
    mga_mode_restore();
 }
 if (unmap) {
    mga_hw_fini();
 }
}



/* Desc: state retrieval
 *
 * In  : parameter name, ptr to storage
 * Out : 0 if request successfully processed
 *
 * Note: -
 */
int mga_get (int pname, int *params)
{
 switch (pname) {
        case MGA_GET_CARD_NAME:
             strcat(strcpy((char *)params, "Matrox "), card_name);
             break;
        case MGA_GET_VRAM:
             params[0] = vram;
             break;
        case MGA_GET_CI_PREC:
             params[0] = 0;
             break;
        case MGA_GET_HPIXELS:
             params[0] = __pixwidth;
             break;
        case MGA_GET_SCREEN_SIZE:
             params[0] = __width;
             params[1] = __height;
             break;
        default:
             return -1;
 }
 return 0;
}
