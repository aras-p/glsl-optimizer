/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * DOS/DJGPP device driver v1.3 for Mesa 5.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 *
 * Thanks to CrazyPyro (Neil Funk) for FakeColor
 */


#include <stdlib.h>

#include "video.h"
#include "internal.h"
#include "vesa/vesa.h"
#include "vga/vga.h"



static vl_driver *drv;
/* based upon mode specific data: valid entire session */
int vl_video_selector;
static vl_mode *video_mode;
static int video_scanlen, video_bypp;
/* valid until next buffer */
void *vl_current_draw_buffer, *vl_current_read_buffer;
int vl_current_stride, vl_current_width, vl_current_height, vl_current_bytes;
int vl_current_offset, vl_current_delta;



/* lookup table for scaling 5 bit colors up to 8 bits */
static int _rgb_scale_5[32] = {
   0,   8,   16,  24,  32,  41,  49,  57,
   65,  74,  82,  90,  98,  106, 115, 123,
   131, 139, 148, 156, 164, 172, 180, 189,
   197, 205, 213, 222, 230, 238, 246, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static int _rgb_scale_6[64] = {
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   64,  68,  72,  76,  80,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   129, 133, 137, 141, 145, 149, 153, 157,
   161, 165, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 214, 218, 222,
   226, 230, 234, 238, 242, 246, 250, 255
};

/* FakeColor data */
#define R_CNT 6
#define G_CNT 6
#define B_CNT 6

#define R_BIAS 7
#define G_BIAS 7
#define B_BIAS 7

static word32 VGAPalette[256];
static word8 array_r[256];
static word8 array_g[256];
static word8 array_b[256];



int (*vl_mixfix) (fixed r, fixed g, fixed b);
int (*vl_mixrgb) (const unsigned char rgb[]);
int (*vl_mixrgba) (const unsigned char rgba[]);
void (*vl_getrgba) (unsigned int offset, unsigned char rgba[4]);
int (*vl_getpixel) (unsigned int offset);
void (*vl_clear) (int color);
void (*vl_rect) (int x, int y, int width, int height, int color);
void (*vl_flip) (void);
void (*vl_putpixel) (unsigned int offset, int color);



/* Desc: color composition (w/o ALPHA)
 *
 * In  : R, G, B
 * Out : color
 *
 * Note: -
 */
static int vl_mixfix8fake (fixed r, fixed g, fixed b)
{
 return array_b[b>>FIXED_SHIFT]*G_CNT*R_CNT
      + array_g[g>>FIXED_SHIFT]*R_CNT
      + array_r[r>>FIXED_SHIFT];
}
#define vl_mixfix8 vl_mixfix8fake
static int vl_mixfix15 (fixed r, fixed g, fixed b)
{
 return ((r>>(3+FIXED_SHIFT))<<10)
       |((g>>(3+FIXED_SHIFT))<<5)
       |(b>>(3+FIXED_SHIFT));
}
static int vl_mixfix16 (fixed r, fixed g, fixed b)
{
 return ((r>>(3+FIXED_SHIFT))<<11)
       |((g>>(2+FIXED_SHIFT))<<5)
       |(b>>(3+FIXED_SHIFT));
}
#define vl_mixfix24 vl_mixfix32
static int vl_mixfix32 (fixed r, fixed g, fixed b)
{
 return ((r>>FIXED_SHIFT)<<16)
       |((g>>FIXED_SHIFT)<<8)
       |(b>>FIXED_SHIFT);
}



/* Desc: color composition (w/ ALPHA)
 *
 * In  : array of integers (R, G, B, A)
 * Out : color
 *
 * Note: -
 */
#define vl_mixrgba8 vl_mixrgb8fake
#define vl_mixrgba15 vl_mixrgb15
#define vl_mixrgba16 vl_mixrgb16
#define vl_mixrgba24 vl_mixrgb24
static int vl_mixrgba32 (const unsigned char rgba[])
{
 return (rgba[3]<<24)|(rgba[0]<<16)|(rgba[1]<<8)|(rgba[2]);
}



/* Desc: color composition (w/o ALPHA)
 *
 * In  : array of integers (R, G, B)
 * Out : color
 *
 * Note: -
 */
static int vl_mixrgb8fake (const unsigned char rgba[])
{
 return array_b[rgba[2]]*G_CNT*R_CNT
      + array_g[rgba[1]]*R_CNT
      + array_r[rgba[0]];
}
#define vl_mixrgb8 vl_mixrgb8fake
static int vl_mixrgb15 (const unsigned char rgb[])
{
 return ((rgb[0]>>3)<<10)|((rgb[1]>>3)<<5)|(rgb[2]>>3);
}
static int vl_mixrgb16 (const unsigned char rgb[])
{
 return ((rgb[0]>>3)<<11)|((rgb[1]>>2)<<5)|(rgb[2]>>3);
}
#define vl_mixrgb24 vl_mixrgb32
static int vl_mixrgb32 (const unsigned char rgb[])
{
 return (rgb[0]<<16)|(rgb[1]<<8)|(rgb[2]);
}



/* Desc: color decomposition
 *
 * In  : pixel offset, array of integers to hold color components (R, G, B, A)
 * Out : -
 *
 * Note: uses current read buffer
 */
static void v_getrgba8fake6 (unsigned int offset, unsigned char rgba[])
{
 word32 c = VGAPalette[((word8 *)vl_current_read_buffer)[offset]];
 rgba[0] = _rgb_scale_6[(c >> 16) & 0x3F];
 rgba[1] = _rgb_scale_6[(c >> 8) & 0x3F];
 rgba[2] = _rgb_scale_6[c & 0x3F];
 rgba[3] = c >> 24;
}
static void v_getrgba8fake8 (unsigned int offset, unsigned char rgba[])
{
 word32 c = VGAPalette[((word8 *)vl_current_read_buffer)[offset]];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 rgba[3] = c >> 24;
}
#define v_getrgba8 v_getrgba8fake6
static void v_getrgba15 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = ((word16 *)vl_current_read_buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 10) & 0x1F];
 rgba[1] = _rgb_scale_5[(c >> 5) & 0x1F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
static void v_getrgba16 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = ((word16 *)vl_current_read_buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 11) & 0x1F];
 rgba[1] = _rgb_scale_6[(c >> 5) & 0x3F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
static void v_getrgba24 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = *(word32 *)((long)vl_current_read_buffer+offset*3);
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 rgba[3] = 255;
}
static void v_getrgba32 (unsigned int offset, unsigned char rgba[4])
{
 word32 c = ((word32 *)vl_current_read_buffer)[offset];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c; 
 rgba[3] = c >> 24;
}



/* Desc: pixel retrieval
 *
 * In  : pixel offset
 * Out : pixel value
 *
 * Note: uses current read buffer
 */
static int v_getpixel8 (unsigned int offset)
{
 return ((word8 *)vl_current_read_buffer)[offset];
}
#define v_getpixel15 v_getpixel16
static int v_getpixel16 (unsigned int offset)
{
 return ((word16 *)vl_current_read_buffer)[offset];
}
static int v_getpixel24 (unsigned int offset)
{
 return *(word32 *)((long)vl_current_read_buffer+offset*3);
}
static int v_getpixel32 (unsigned int offset)
{
 return ((word32 *)vl_current_read_buffer)[offset];
}



/* Desc: set one palette entry
 *
 * In  : index, R, G, B
 * Out : -
 *
 * Note: color components are in range [0.0 .. 1.0]
 */
void vl_setCI (int index, float red, float green, float blue)
{
 drv->setCI_f(index, red, green, blue);
}



/* Desc: set one palette entry
 *
 * In  : color, R, G, B
 * Out : -
 *
 * Note: -
 */
static void fake_setcolor (int c, int r, int g, int b)
{
 VGAPalette[c] = 0xff000000 | (r<<16) | (g<<8) | b;

 drv->setCI_i(c, r, g, b);
}



/* Desc: build FakeColor palette
 *
 * In  : CI precision in bits
 * Out : -
 *
 * Note: -
 */
static void fake_buildpalette (int bits)
{
 double c_r, c_g, c_b;
 int r, g, b, color = 0;

 double max = (1 << bits) - 1;

 for (b=0; b<B_CNT; ++b) {
     for (g=0; g<G_CNT; ++g) {
         for (r=0; r<R_CNT; ++r) {
             c_r = 0.5 + (double)r*(max-R_BIAS)/(R_CNT-1.) + R_BIAS;
             c_g = 0.5 + (double)g*(max-G_BIAS)/(G_CNT-1.) + G_BIAS;
             c_b = 0.5 + (double)b*(max-B_BIAS)/(B_CNT-1.) + B_BIAS;
             fake_setcolor(color++, (int)c_r, (int)c_g, (int)c_b);
         }
     }
 }

 for (color=0; color<256; color++) {
     c_r = (double)color*R_CNT/256.;
     c_g = (double)color*G_CNT/256.;
     c_b = (double)color*B_CNT/256.;
     array_r[color] = (int)c_r;
     array_g[color] = (int)c_g;
     array_b[color] = (int)c_b;
 }
}



/* Desc: sync buffer with video hardware
 *
 * In  : ptr to old buffer, position, size
 * Out : 0 if success
 *
 * Note: -
 */
int vl_sync_buffer (void **buffer, int x, int y, int width, int height)
{
 if ((width & 7) || (x < 0) || (y < 0) || (x+width > video_mode->xres) || (y+height > video_mode->yres)) {
    return -1;
 } else {
    void *newbuf = *buffer;

    if ((newbuf == NULL) || (vl_current_width != width) || (vl_current_height != height)) {
       newbuf = realloc(newbuf, width * height * video_bypp);
    }

    if (newbuf == NULL) {
       return -2;
    }

    vl_current_width = width;
    vl_current_height = height;
    vl_current_stride = vl_current_width * video_bypp;
    vl_current_bytes = vl_current_stride * height;

    vl_current_offset = video_scanlen * y + video_bypp * x;
    vl_current_delta = video_scanlen - vl_current_stride;

    vl_current_draw_buffer = vl_current_read_buffer = *buffer = newbuf;
    return 0;
 }
}



/* Desc: get screen geometry
 *
 * In  : ptr to WIDTH, ptr to HEIGHT
 * Out : -
 *
 * Note: -
 */
void vl_get_screen_size (int *width, int *height)
{
 *width = video_mode->xres;
 *height = video_mode->yres;
}



/* Desc: retrieve CPU MMX capability
 *
 * In  : -
 * Out : FALSE if CPU cannot do MMX
 *
 * Note: -
 */
int vl_can_mmx (void)
{
#ifdef USE_MMX_ASM
 extern int _mesa_identify_x86_cpu_features (void);
 return (_mesa_identify_x86_cpu_features() & 0x00800000);
#else
 return 0;
#endif
}



/* Desc: setup mode
 *
 * In  : ptr to mode definition
 * Out : 0 if success
 *
 * Note: -
 */
static int vl_setup_mode (vl_mode *p)
{
 if (p == NULL) {
    return -1;
 }

#define INITPTR(bpp) \
        vl_putpixel = v_putpixel##bpp; \
        vl_getrgba = v_getrgba##bpp;   \
        vl_getpixel = v_getpixel##bpp; \
        vl_rect = v_rect##bpp;         \
        vl_mixfix = vl_mixfix##bpp;    \
        vl_mixrgb = vl_mixrgb##bpp;    \
        vl_mixrgba = vl_mixrgba##bpp;  \
        vl_clear = vl_can_mmx() ? v_clear##bpp##_mmx : v_clear##bpp
        
 switch (p->bpp) {
        case 8:
             INITPTR(8);
             break;
        case 15:
             INITPTR(15);
             break;
        case 16:
             INITPTR(16);
             break;
        case 24:
             INITPTR(24);
             break;
        case 32:
             INITPTR(32);
             break;
        default:
             return -1;
 }

#undef INITPTR

 video_mode = p;
 video_bypp = (p->bpp+7)/8;
 video_scanlen = p->scanlen;
 vl_video_selector = p->sel;

 return 0;
}



/* Desc: restore to the mode prior to first call to `vl_video_init'.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
void vl_video_exit (void)
{
 drv->restore();
 drv->finit();
}



/* Desc: enter mode
 *
 * In  : xres, yres, bits/pixel, RGB, refresh rate
 * Out : pixel width in bits if success
 *
 * Note: -
 */
int vl_video_init (int width, int height, int bpp, int rgb, int refresh)
{
 int fake;
 vl_mode *p, *q;
 unsigned int min;

 fake = 0;
 if (!rgb) {
    bpp = 8;
 } else if (bpp == 8) {
    fake = 1;
 }

 /* initialize hardware */
 drv = &VESA;
 if ((q=drv->init()) == NULL) {
    drv = &VGA;
    if ((q=drv->init()) == NULL) {
       return 0;
    }
 }

 /* search for a mode that fits our request */
 for (min=-1, p=NULL; q->mode!=0xffff; q++) {
     if ((q->xres>=width) && (q->yres>=height) && (q->bpp==bpp)) {
        if (min>=(unsigned)(q->xres*q->yres)) {
           min = q->xres*q->yres;
           p = q;
        }
     }
 }

 /* setup and enter mode */
 if ((vl_setup_mode(p) == 0) && (drv->entermode(p, refresh) == 0)) {
    vl_flip = drv->blit;
    if (fake) {
       min = drv->getCIprec();
       fake_buildpalette(min);
       if (min == 8) {
          vl_getrgba = v_getrgba8fake8;
       }
    }
    return bpp;
 }

 /* abort */
 return 0;
}
