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
 * DOS/DJGPP device driver v0.1 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>
#include <string.h>
#include <stubinfo.h>
#include <sys/exceptn.h>
#include <sys/farptr.h>
#include <sys/segments.h>

#include "dvesa.h"



typedef struct dvmode {
	int mode;
	int xres, yres;
	int scanlen;
	int bpp;
	word32 opaque;
} dvmode;

#define _16_ *(word16 *)&
#define _32_ *(word32 *)&

static int init;
static int selector = -1;
static int granularity;
static dvmode modes[128];
static void (*dv_putpixel) (void *buffer, int offset, int color);
int (*dv_color) (const unsigned char rgba[]);
void (*dv_dump_virtual) (void *buffer, int width, int height, int offset, int delta);
void (*dv_clear_virtual) (void *buffer, int len, int color);

extern void dv_dump_virtual_linear (void *buffer, int width, int height, int offset, int delta);
extern void dv_dump_virtual_banked (void *buffer, int width, int height, int offset, int delta);

extern void dv_clear_virtual16 (void *buffer, int len, int color);
extern void dv_clear_virtual24 (void *buffer, int len, int color);
extern void dv_clear_virtual32 (void *buffer, int len, int color);

/* lookup table for scaling 5 bit colors up to 8 bits */
int _rgb_scale_5[32] =
{
   0,   8,   16,  24,  32,  41,  49,  57,
   65,  74,  82,  90,  98,  106, 115, 123,
   131, 139, 148, 156, 164, 172, 180, 189,
   197, 205, 213, 222, 230, 238, 246, 255
};

/* lookup table for scaling 6 bit colors up to 8 bits */
int _rgb_scale_6[64] =
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
 * colors
 */
int dv_color15 (const unsigned char rgba[])
{
 return ((rgba[0]>>3)<<10)|((rgba[1]>>3)<<5)|(rgba[2]>>3);
}
int dv_color16 (const unsigned char rgba[])
{
 return ((rgba[0]>>3)<<11)|((rgba[1]>>2)<<5)|(rgba[2]>>3);
}
int dv_color32 (const unsigned char rgba[])
{
 return (rgba[0]<<16)|(rgba[1]<<8)|(rgba[2]);
}



/*
 * getpixel + decompose
 */
void dv_getrgba15 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word16 *)buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 10) & 0x1F];
 rgba[1] = _rgb_scale_5[(c >> 5) & 0x1F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
void dv_getrgba16 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word16 *)buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 11) & 0x1F];
 rgba[1] = _rgb_scale_6[(c >> 5) & 0x3F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
void dv_getrgba24 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = *(word32 *)(buffer+offset*3);
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 rgba[3] = 255;
}
void dv_getrgba32 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word32 *)buffer)[offset];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c; 
 rgba[3] = 255;
}



/*
 * request mapping from DPMI
 */
static word32 _map_linear (word32 phys, word32 len)
{
 __dpmi_meminfo meminfo;

 if (phys >= 0x100000) {
    /* map into linear memory */
    meminfo.address = (long)phys;
    meminfo.size = len;
    if (__dpmi_physical_address_mapping(&meminfo)) {
       return 0;
    }
    return meminfo.address;
 } else {
    /* exploit 1 -> 1 physical to linear mapping in low megabyte */
    return phys;
 }
}



/*
 * attempts to detect VESA and video modes
 */
static word16 dv_get_vesa (void)
{
 __dpmi_regs r;
 unsigned short *p;
 dvmode *q;
 char vesa_info[512], tmp[512];

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
	      q->opaque = (_16_ tmp[4])<<10;
	      if (tmp[0]&0x80) {
		 *(q+1) = *q++;
		 q->opaque = _32_ tmp[0x28];
		 q->mode |= 0x4000;
	      }
	      q++;
	   }
       } while (!0);

       return _16_ vesa_info[4];
    }
 }

 return 0;
}



/*
 * select a mode
 */
dvmode *dv_select_mode (int x, int y, int width, int height, int depth, int *delta, int *offset)
{
 dvmode *p, *q;
 unsigned int min;

 if ((width&3)||(height&3)||(depth<=8)) {
    return NULL;
 }

 if (!init) {
    init = !init;
    if (!dv_get_vesa()) {
       return NULL;
    }
 }

 for (min=-1, p=NULL, q=modes; q->mode!=0xffff; q++) {
     if ((q->xres>=(x+width))&&(q->yres>=(y+height))&&(q->bpp==depth)) {
        if (min>(unsigned)(q->xres*q->yres)) {
           min = q->xres*q->yres;
           p = q;
        }
     }
 }

 if (p) {
    int sel = -1;
    unsigned base, limit;

    if ((p->mode|0x4000)==(p+1)->mode) {
       p++;
    }

    if (selector==-1) {
       if ((selector=sel=__dpmi_allocate_ldt_descriptors(1))==-1) {
          return NULL;
       }
    }
    if (p->mode&0x4000) {
       limit = ((p->scanlen*p->yres+0xfffUL)&~0xfffUL) - 1;
       if ((base=_map_linear(p->opaque, limit))==NULL) {
          if (sel!=-1) {
             selector = -1;
             __dpmi_free_ldt_descriptor(sel);
          }
          return NULL;
       }
       dv_dump_virtual = dv_dump_virtual_linear;
    } else {
       limit = granularity = p->opaque;
       base = 0xa0000;
       dv_dump_virtual = dv_dump_virtual_banked;
    }
    __dpmi_set_segment_base_address(selector, base);
    __dpmi_set_descriptor_access_rights(selector, ((_my_ds()&3)<<5)|0x4092);
    __dpmi_set_segment_limit(selector, limit);

    switch (p->bpp) {
           case 15:
                dv_clear_virtual = dv_clear_virtual16;
                dv_putpixel = dv_putpixel16;
                dv_color = dv_color15;
                break;
           case 16:
                dv_clear_virtual = dv_clear_virtual16;
                dv_putpixel = dv_putpixel16;
                dv_color = dv_color16;
                break;
           case 24:
                dv_clear_virtual = dv_clear_virtual24;
                dv_putpixel = dv_putpixel24;
                dv_color = dv_color32;
                break;
           case 32:
                dv_clear_virtual = dv_clear_virtual32;
                dv_putpixel = dv_putpixel32;
                dv_color = dv_color32;
                break;
           default:
                dv_clear_virtual = NULL;
                dv_putpixel = NULL;
    }

    *delta = p->scanlen - ((depth+7)/8) * width;
    *offset = p->scanlen*y + ((depth+7)/8) * x;
    
    __asm__("movw $0x4f02, %%ax; int  $0x10"::"b"(p->mode):"%eax");
 }

 return p;
}



/*
 * fill rectangle
 */
void dv_fillrect (void *buffer, int bwidth, int x, int y, int width, int height, int color)
{
 int i, offset;

 offset = y*bwidth + x;
 bwidth -= width;
 for (height+=y; y<height; y++) {
     for (i=0; i<width; i++, offset++) {
         dv_putpixel(buffer, offset, color);
     }
     offset += bwidth;
 }
}



/*
 * dv_dump_virtual_linear
 */
__asm__("\n\
		.balign	4			\n\
		.global	_dv_dump_virtual_linear \n\
_dv_dump_virtual_linear:			\n\
		pushl	%fs			\n\
		pushl	%ebx			\n\
		pushl	%esi			\n\
		pushl	%edi			\n\
		movl	_selector, %fs		\n\
		movl	4*4+4+0(%esp), %esi	\n\
		movl	4*4+4+12(%esp), %edi	\n\
		movl	4*4+4+4(%esp), %ecx	\n\
		movl	4*4+4+8(%esp), %edx	\n\
		movl	4*4+4+16(%esp), %ebx	\n\
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
		popl	%fs			\n\
		ret");
/*
 * dv_dump_virtual_banked
 */
__asm__("\n\
		.balign	4			\n\
		.global	_dv_dump_virtual_banked \n\
_dv_dump_virtual_banked:			\n\
		pushl	%fs			\n\
		pushl	%ebx			\n\
		pushl	%esi			\n\
		pushl	%edi			\n\
		pushl	%ebp			\n\
		movl	_selector, %fs		\n\
		movl	4*5+4+0(%esp), %esi	\n\
		movl	_granularity, %ebp	\n\
		xorl	%edx, %edx		\n\
		movl	4*5+4+12(%esp), %eax	\n\
		divl	%ebp			\n\
		movl	%edx, %edi		\n\
		pushl	%eax			\n\
		movl	%eax, %edx		\n\
		xorl	%ebx, %ebx		\n\
		movw	$0x4f05, %ax		\n\
		int	$0x10			\n\
		movl	4*6+4+16(%esp), %ebx	\n\
		movl	4*6+4+4(%esp), %ecx	\n\
		movl	4*6+4+8(%esp), %edx	\n\
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
		popl	%fs			\n\
		ret");



/*
 * dv_clear_virtual??
 */
__asm__("\n\
		.balign	4			\n\
		.global	_dv_clear_virtual16	\n\
_dv_clear_virtual16:				\n\
		movl	12(%esp), %eax		\n\
		pushw	%ax			\n\
		pushw	%ax			\n\
		popl	%eax			\n\
		jmp	_dv_clear_virtual_common\n\
		.balign	4			\n\
		.global	_dv_clear_virtual32	\n\
_dv_clear_virtual32:				\n\
		movl	12(%esp), %eax		\n\
						\n\
		.balign	4			\n\
		.global	_dv_clear_virtual_common\n\
_dv_clear_virtual_common:			\n\
		movl	8(%esp), %ecx		\n\
		movl	4(%esp), %edx		\n\
		shrl	$2, %ecx		\n\
		.balign	4			\n\
	0:					\n\
		movl	%eax, (%edx)		\n\
		addl	$4, %edx		\n\
		decl	%ecx			\n\
		jnz	0b			\n\
		ret");
__asm__("\n\
		.balign	4			\n\
		.global	_dv_clear_virtual24	\n\
_dv_clear_virtual24:				\n\
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
 * dv_putpixel??
 */
__asm__("\n\
		.balign	4			\n\
		.global	_dv_putpixel16		\n\
_dv_putpixel16:					\n\
		movl	8(%esp), %edx		\n\
		shll	%edx			\n\
		movl	12(%esp), %eax		\n\
		addl	4(%esp), %edx		\n\
		movw	%ax, (%edx)		\n\
		ret");
__asm__("\n\
		.balign	4			\n\
		.global	_dv_putpixel32		\n\
_dv_putpixel32:					\n\
		movl	8(%esp), %edx		\n\
		shll	$2, %edx		\n\
		movl	12(%esp), %eax		\n\
		addl	4(%esp), %edx		\n\
		movl	%eax, (%edx)		\n\
		ret");
__asm__("\n\
		.global	_dv_putpixel24		\n\
_dv_putpixel24:					\n\
		movl	8(%esp), %edx		\n\
		leal	(%edx, %edx, 2), %edx	\n\
		movl	12(%esp), %eax		\n\
		addl	4(%esp), %edx		\n\
		movw	%ax, (%edx)		\n\
		shrl	$16, %eax		\n\
		movb	%al, 2(%edx)		\n\
		ret");
