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
 * DOS/DJGPP device driver v1.1 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <stdlib.h>

#include "video.h"
#include "videoint.h"
#include "vesa/vesa.h"



static vl_driver *drv = &VESA;
/* card specific: valid forever */
word32 vl_hw_granularity;
static unsigned int gran_shift, gran_mask;
/* based upon mode specific data: valid entire session */
int vl_video_selector;
static int video_scanlen, video_bypp;
/* valid until next buffer */
int vl_current_offset, vl_current_delta;
static int current_width;



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



void (*vl_clear) (void *buffer, int bytes, int color);
void (*vl_flip) (void *buffer, int stride, int height);
int (*vl_mixrgba) (const unsigned char rgba[]);
int (*vl_mixrgb) (const unsigned char rgb[]);
void (*vl_putpixel) (void *buffer, int offset, int color);
void (*vl_getrgba) (void *buffer, int offset, unsigned char rgba[4]);



/* vl_rect:
 *  Clears a rectange with specified color.
 */
void vl_rect (void *buffer, int x, int y, int width, int height, int color)
{
 int offset = y*current_width + x;
 int delta = current_width - width;

 for (y=0; y<height; y++) {
     for (x=0; x<width; x++, offset++) {
         vl_putpixel(buffer, offset, color);
     }
     offset += delta;
 }
}



/* vl_mixrgba*:
 *  Color composition (w/ ALPHA).
 */
#define vl_mixrgba15 vl_mixrgb15
#define vl_mixrgba16 vl_mixrgb16
#define vl_mixrgba24 vl_mixrgb24
static int vl_mixrgba32 (const unsigned char rgba[])
{
 return (rgba[3]<<24)|(rgba[0]<<16)|(rgba[1]<<8)|(rgba[2]);
}



/* vl_mixrgb*:
 *  Color composition (w/o ALPHA).
 */
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



/* v_getrgba*:
 *  Color decomposition.
 */
static void v_getrgba15 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word16 *)buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 10) & 0x1F];
 rgba[1] = _rgb_scale_5[(c >> 5) & 0x1F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
static void v_getrgba16 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word16 *)buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 11) & 0x1F];
 rgba[1] = _rgb_scale_6[(c >> 5) & 0x3F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
static void v_getrgba24 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = *(word32 *)((long)buffer+offset*3);
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 rgba[3] = 255;
}
static void v_getrgba32 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word32 *)buffer)[offset];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c; 
 rgba[3] = c >> 24;
}



/* vl_sync_buffer:
 *  Syncs buffer with video hardware. Returns NULL in case of failure.
 */
void *vl_sync_buffer (void *buffer, int x, int y, int width, int height)
{
 void *newbuf;

 if (width&3) {
    return NULL;
 } else {
    if ((newbuf=realloc(buffer, width*height*video_bypp))!=NULL) {
       vl_current_offset = video_scanlen * y + video_bypp * x;
       current_width = width;
       vl_current_delta = video_scanlen - video_bypp * width;
    }
    return newbuf;
 }
}



/* vl_setup_mode:
 *
 *  success: 0
 *  failure: -1
 */
static int vl_setup_mode (vl_mode *p)
{
 if (p->mode&0x4000) {
    vl_flip = l_dump_virtual;
 } else {
    { int n; for (gran_shift=0, n=p->gran; n; gran_shift++, n>>=1) ; }
    gran_mask = (1<<(--gran_shift)) - 1;
    if ((unsigned)p->gran != (gran_mask+1)) {
       return -1;
    }
    vl_hw_granularity = p->gran;
    vl_flip = b_dump_virtual;
 }

#define INITPTR(bpp) \
        vl_putpixel = v_putpixel##bpp; \
        vl_getrgba = v_getrgba##bpp;   \
        vl_clear = v_clear##bpp;       \
        vl_mixrgb = vl_mixrgb##bpp;    \
        vl_mixrgba = vl_mixrgba##bpp;

 switch (p->bpp) {
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

 video_bypp = (p->bpp+7)/8;
 video_scanlen = p->scanlen;
 vl_video_selector = p->sel;

 return 0;
}



/* vl_video_exit:
 *  Shutdown the video engine.
 *  Restores to the mode prior to first call to `vl_video_init'.
 */
void vl_video_exit (void)
{
 drv->restore();
 drv->finit();
}



/* vl_video_init:
 *  Enter mode.
 *
 *  success: 0
 *  failure: -1
 */
int vl_video_init (int width, int height, int bpp, int refresh)
{
 vl_mode *p, *q;
 unsigned int min;

 /* initialize hardware */
 if ((q=drv->getmodes()) == NULL) {
    return -1;
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

 /* check, setup and enter mode */
 if ((p!=NULL) && (vl_setup_mode(p) == 0) && (drv->entermode(p, refresh) == 0)) {
    return 0;
 }

 /* abort */
 return -1;
}
