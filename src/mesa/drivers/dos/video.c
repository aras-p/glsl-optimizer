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
 * DOS/DJGPP device driver v1.0 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>
#include <stdlib.h>
#include <string.h>
#include <stubinfo.h>
#include <sys/exceptn.h>
#include <sys/segments.h>
#include <sys/farptr.h>

#include "video.h"
#include "dpmiint.h"



typedef unsigned char word8;
typedef unsigned short word16;
typedef unsigned long word32;

typedef struct vl_mode {
        int mode;
        int xres, yres;
        int scanlen;
        int bpp;
} vl_mode;

#define _16_ *(word16 *)&
#define _32_ *(word32 *)&

static int init;

static vl_mode modes[64];

/* card specific: valid forever */
static word16 vesa_ver;
static word32 hw_granularity, hw_linearfb;
static unsigned int gran_shift, gran_mask;
/* based upon mode specific data: valid entire session */
static int video_selector, banked_selector, linear_selector;
static int video_scanlen, video_bypp;
/* valid until next buffer */
static int current_offset, current_delta, current_width;



/* lookup table for scaling 5 bit colors up to 8 bits */
static int _rgb_scale_5[32] =
{
   0,   8,   16,  24,  32,  41,  49,  57,
   65,  74,  82,  90,  98,  106, 115, 123,
   131, 139, 148, 156, 164, 172, 180, 189,
   197, 205, 213, 222, 230, 238, 246, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
static int _rgb_scale_6[64] =
{
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   64,  68,  72,  76,  80,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   129, 133, 137, 141, 145, 149, 153, 157,
   161, 165, 170, 174, 178, 182, 186, 190,
   194, 198, 202, 206, 210, 214, 218, 222,
   226, 230, 234, 238, 242, 246, 250, 255
};



/*
 * virtual clearing
 */
void (*vl_clear) (void *buffer, int len, int color);

#define v_clear15 v_clear16
extern void v_clear16 (void *buffer, int len, int color);
extern void v_clear32 (void *buffer, int len, int color);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_v_clear16		\n\
_v_clear16:					\n\
		movl	12(%esp), %eax		\n\
		pushw	%ax			\n\
		pushw	%ax			\n\
		popl	%eax			\n\
		jmp	_v_clear_common		\n\
		.balign	4			\n\
		.global	_v_clear32		\n\
_v_clear32:					\n\
		movl	12(%esp), %eax		\n\
		.balign	4			\n\
_v_clear_common:				\n\
		movl	8(%esp), %ecx		\n\
		movl	4(%esp), %edx		\n\
		shrl	$2, %ecx		\n\
	0:					\n\
		.balign	4			\n\
		movl	%eax, (%edx)		\n\
		addl	$4, %edx		\n\
		decl	%ecx			\n\
		jnz	0b			\n\
		ret");
extern void v_clear24 (void *buffer, int len, int color);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_v_clear24		\n\
_v_clear24:					\n\
		movl	8(%esp), %edx		\n\
		movl	$0xaaaaaaab, %eax	\n\
		mull	%edx			\n\
		movl	12(%esp), %eax		\n\
		movl	%edx, %ecx		\n\
		movl	4(%esp), %edx		\n\
		pushl	%ebx			\n\
		shrl	%ecx			\n\
		movb	18(%esp), %bl		\n\
		.balign	4			\n\
	0:					\n\
		movw	%ax, (%edx)		\n\
		movb	%bl, 2(%edx)		\n\
		addl	$3, %edx		\n\
		decl	%ecx			\n\
		jnz	0b			\n\
		popl	%ebx			\n\
		ret");



/*
 * virtual rectangle clearing
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



/*
 * virtual dumping:
 */
void (*vl_flip) (void *buffer, int width, int height);

extern void b_dump_virtual (void *buffer, int width, int height);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_b_dump_virtual		\n\
_b_dump_virtual:				\n\
		pushl	%ebx			\n\
		pushl	%esi			\n\
		pushl	%edi			\n\
		pushl	%ebp			\n\
		movl	_video_selector, %fs	\n\
		movl	4*4+4+0(%esp), %esi	\n\
		movl	_hw_granularity, %ebp	\n\
		xorl	%edx, %edx		\n\
		movl	_current_offset, %eax	\n\
		divl	%ebp			\n\
		movl	%edx, %edi		\n\
		pushl	%eax			\n\
		movl	%eax, %edx		\n\
		xorl	%ebx, %ebx		\n\
		movw	$0x4f05, %ax		\n\
		int	$0x10			\n\
		movl	_current_delta, %ebx	\n\
		movl	5*4+4+4(%esp), %ecx	\n\
		movl	5*4+4+8(%esp), %edx	\n\
		shrl	$2, %ecx		\n\
		.balign	4			\n\
	0:					\n\
		pushl	%ecx			\n\
		.balign	4			\n\
	1:					\n\
		cmpl	%ebp, %edi		\n\
		jb	2f			\n\
		pushl	%ebx			\n\
		pushl	%edx			\n\
		incl	12(%esp)		\n\
		movw	$0x4f05, %ax		\n\
		movl	12(%esp), %edx		\n\
		xorl	%ebx, %ebx		\n\
		int	$0x10			\n\
		popl	%edx			\n\
		popl	%ebx			\n\
		subl	%ebp, %edi		\n\
	2:					\n\
		movl	(%esi), %eax		\n\
		addl	$4, %esi		\n\
		movl	%eax, %fs:(%edi)	\n\
		addl	$4, %edi		\n\
		decl	%ecx			\n\
		jnz	1b			\n\
		popl	%ecx			\n\
		addl	%ebx, %edi		\n\
		decl	%edx			\n\
		jnz	0b			\n\
		popl	%eax			\n\
		popl	%ebp			\n\
		popl	%edi			\n\
		popl	%esi			\n\
		popl	%ebx			\n\
		ret");
extern void l_dump_virtual (void *buffer, int width, int height);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_l_dump_virtual		\n\
_l_dump_virtual:				\n\
		pushl	%ebx			\n\
		pushl	%esi			\n\
		pushl	%edi			\n\
		movl	_video_selector, %fs	\n\
		movl	3*4+4+0(%esp), %esi	\n\
		movl	_current_offset, %edi	\n\
		movl	3*4+4+4(%esp), %ecx	\n\
		movl	3*4+4+8(%esp), %edx	\n\
		movl	_current_delta, %ebx	\n\
		shrl	$2, %ecx		\n\
		.balign	4			\n\
	0:					\n\
		pushl	%ecx			\n\
		.balign	4			\n\
	1:					\n\
		movl	(%esi), %eax		\n\
		addl	$4, %esi		\n\
		movl	%eax, %fs:(%edi)	\n\
		addl	$4, %edi		\n\
		decl	%ecx			\n\
		jnz	1b			\n\
		popl	%ecx			\n\
		addl	%ebx, %edi		\n\
		decl	%edx			\n\
		jnz	0b			\n\
		popl	%edi			\n\
		popl	%esi			\n\
		popl	%ebx			\n\
		ret");



/*
 * mix RGBA components
 */
int (*vl_mixrgba) (const unsigned char rgba[]);
 
#define vl_mixrgba15 vl_mixrgb15
#define vl_mixrgba16 vl_mixrgb16
#define vl_mixrgba24 vl_mixrgb24
static int vl_mixrgba32 (const unsigned char rgba[])
{
 return (rgba[3]<<24)|(rgba[0]<<16)|(rgba[1]<<8)|(rgba[2]);
}



/*
 * mix RGB components
 */
int (*vl_mixrgb) (const unsigned char rgb[]);
 
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



/*
 * vl_putpixel*
 */
void (*vl_putpixel) (void *buffer, int offset, int color);
 
#define v_putpixel15 v_putpixel16
extern void v_putpixel16 (void *buffer, int offset, int color);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_v_putpixel16		\n\
_v_putpixel16:					\n\
		movl	8(%esp), %edx		\n\
		shll	%edx			\n\
		movl	12(%esp), %eax		\n\
		addl	4(%esp), %edx		\n\
		movw	%ax, (%edx)		\n\
		ret");
extern void v_putpixel24 (void *buffer, int offset, int color);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_v_putpixel24		\n\
_v_putpixel24:					\n\
		movl	8(%esp), %edx		\n\
		leal	(%edx, %edx, 2), %edx	\n\
		movl	12(%esp), %eax		\n\
		addl	4(%esp), %edx		\n\
		movw	%ax, (%edx)		\n\
		shrl	$16, %eax		\n\
		movb	%al, 2(%edx)		\n\
		ret");
extern void v_putpixel32 (void *buffer, int offset, int color);
__asm__("\n\
		.text				\n\
		.balign	4			\n\
		.global	_v_putpixel32		\n\
_v_putpixel32:					\n\
		movl	8(%esp), %edx		\n\
		shll	$2, %edx		\n\
		movl	12(%esp), %eax		\n\
		addl	4(%esp), %edx		\n\
		movl	%eax, (%edx)		\n\
		ret");



/*
 * get pixel and decompose R, G, B, A
 */
void (*vl_getrgba) (void *buffer, int offset, unsigned char rgba[4]);

/*
 * v_getrgba*
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



/*
 * sync buffer with video hardware
 */
void *vl_sync_buffer (void *buffer, int x, int y, int width, int height)
{
 void *newbuf;

 if (width&3) {
    return NULL;
 } else {
    if ((newbuf=realloc(buffer, width*height*video_bypp))!=NULL) {
       current_offset = video_scanlen * y + video_bypp * x;
       current_width = width;
       current_delta = video_scanlen - video_bypp * width;
    }
    return newbuf;
 }
}



/*
 * attempts to detect VESA and video modes
 */
static word16 vl_vesa_init (void)
{
 __dpmi_regs r;
 unsigned short *p;
 vl_mode *q;
 char vesa_info[512], tmp[512];
 int maxsize = 0;

 _farpokel(_stubinfo->ds_selector, 0, 0x32454256);
 r.x.ax = 0x4f00;
 r.x.di = 0;
 r.x.es = _stubinfo->ds_segment;
 __dpmi_int(0x10, &r);
 if (r.x.ax==0x004f) {
    movedata(_stubinfo->ds_selector, 0, _my_ds(), (unsigned)vesa_info, 512);
    if ((_32_ vesa_info[0])==0x41534556) {
       p = (unsigned short *)(((_16_ vesa_info[0x10])<<4) + (_16_ vesa_info[0x0e]));
       q = modes;
       do {
           if ((q->mode=_farpeekw(__djgpp_dos_sel, (unsigned long)(p++)))==0xffff) {
              break;
           }

           r.x.ax = 0x4f01;
           r.x.cx = q->mode;
           r.x.di = 512;
           r.x.es = _stubinfo->ds_segment;
           __dpmi_int(0x10, &r);
           movedata(_stubinfo->ds_selector, 512, _my_ds(), (unsigned)tmp, 256);
           switch (tmp[0x19]) {
                  case 16:
                       q->bpp = tmp[0x1f] + tmp[0x21] + tmp[0x23];
                       break;
                  case 15:
                  case 24:
                  case 32:
                       q->bpp = tmp[0x19];
                       break;
                  default:
                       q->bpp = 0;
           }
           if ((r.x.ax==0x004f)&&((tmp[0]&0x11)==0x11)&&q->bpp) {
              q->xres = _16_ tmp[0x12];
              q->yres = _16_ tmp[0x14];
              q->scanlen = _16_ tmp[0x10];
              hw_granularity = (_16_ tmp[4])<<10;
              if (tmp[0]&0x80) {
                 *(q+1) = *q++;
                 hw_linearfb = _32_ tmp[0x28];
                 q->mode |= 0x4000;
              }
              if (maxsize<(q->scanlen*q->yres)) {
                 maxsize = q->scanlen*q->yres;
              }
              q++;
           }
       } while (!0);

       if (hw_linearfb) {
          maxsize = ((maxsize+0xfffUL)&~0xfffUL);
          if (_create_selector(&linear_selector, hw_linearfb, maxsize)) {
             return 0;
          }
       }
       if (_create_selector(&banked_selector, 0xa0000, hw_granularity)) {
          _remove_selector(&linear_selector);
          return 0;
       }

       return _16_ vesa_info[4];
    }
 }

 return 0;
}



/*
 * setup mode
 */
static int vl_setup_mode (vl_mode *p)
{
 if (p->mode&0x4000) {
    video_selector = linear_selector;
    vl_flip = l_dump_virtual;
 } else {
    { int n; for (gran_shift=0, n=hw_granularity; n; gran_shift++, n>>=1) ; }
    gran_mask = (1<<(--gran_shift)) - 1;
    if (hw_granularity!=(gran_mask+1)) {
       return -1;
    }
    video_selector = banked_selector;
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

 return 0;
}



/*
 * shutdown the video engine
 */
void vl_video_exit (int textmode)
{
 if (init) {
    if (textmode) {
       __asm__("movw $0x3, %%ax; int  $0x10":::"%eax");
    }
    
    _remove_selector(&linear_selector);
    _remove_selector(&banked_selector);
   
    init = !init;
 }
}



/*
 * initialize video engine
 *
 * success: 0
 * failure: -1
 */
int vl_video_init (int width, int height, int bpp)
{
 vl_mode *p, *q;
 unsigned int min;

 /* check for prior initialization */
 if (init) {
    return 0;
 }

 /* initialize hardware */
 if (!(vesa_ver=vl_vesa_init())) {
    return -1;
 }
 init = !init;

 /* search for a mode that fits our request */
 for (min=-1, p=NULL, q=modes; q->mode!=0xffff; q++) {
     if ((q->xres>=width)&&(q->yres>=height)&&(q->bpp==bpp)) {
        if (min>=(unsigned)(q->xres*q->yres)) {
           min = q->xres*q->yres;
           p = q;
        }
     }
 }

 if (p) {
    vl_setup_mode(p);
    __asm__("movw $0x4f02, %%ax; int  $0x10"::"b"(p->mode):"%eax");
    return 0;
 } else {
    /* no suitable mode found, abort */
    vl_video_exit(0);
    return -1;
 }
}
