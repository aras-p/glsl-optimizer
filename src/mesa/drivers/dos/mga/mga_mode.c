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
 * DOS/DJGPP device driver v1.3 for Mesa  --  MGA2064W mode switching
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <dpmi.h>
#include <string.h>
#include <stubinfo.h>
#include <sys/exceptn.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <sys/segments.h>

#include "../internal.h"
#include "mga_mode.h"



static MGA_MODE oldmode;
static MGA_MODE modes[64];



/*
 * VESA info
 */
#define V_SIGN     0
#define V_MINOR    4
#define V_MAJOR    5
#define V_OEM_OFS  6
#define V_OEM_SEG  8
#define V_MODE_OFS 14
#define V_MODE_SEG 16
#define V_MEMORY   18

/*
 * mode info
 */
#define M_ATTR     0
#define M_GRAN     4
#define M_SCANLEN  16
#define M_XRES     18
#define M_YRES     20
#define M_BPP      25
#define M_RED      31
#define M_GREEN    33
#define M_BLUE     35
#define M_PHYS_PTR 40



/* Desc: get available modes
 *
 * In  : -
 * Out : linear modes list ptr
 *
 * Note: shouldn't use VESA...
 */
static MGA_MODE *_mga_mode_check (void)
{
 __dpmi_regs r;
 word16 *p;
 MGA_MODE *q;
 char vesa_info[512], tmp[512];

 _farpokel(_stubinfo->ds_selector, 0, 0x32454256);
 r.x.ax = 0x4f00;
 r.x.di = 0;
 r.x.es = _stubinfo->ds_segment;
 __dpmi_int(0x10, &r);
 movedata(_stubinfo->ds_selector, 0, _my_ds(), (unsigned)vesa_info, 512);
 if ((r.x.ax!=0x004f) || ((_32_ vesa_info[V_SIGN])!=0x41534556)) {
    return NULL;
 }

 p = (word16 *)(((_16_ vesa_info[V_MODE_SEG])<<4) + (_16_ vesa_info[V_MODE_OFS]));
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
     switch (tmp[M_BPP]) {
            case 16:
                 q->bpp = tmp[M_RED] + tmp[M_GREEN] + tmp[M_BLUE];
                 break;
            case 8:
            case 15:
            case 24:
            case 32:
                 q->bpp = tmp[M_BPP];
                 break;
            default:
                 q->bpp = 0;
     }
     if ((r.x.ax==0x004f) && ((tmp[M_ATTR]&0x11)==0x11) && q->bpp && (tmp[M_ATTR]&0x80)) {
        q->xres = _16_ tmp[M_XRES];
        q->yres = _16_ tmp[M_YRES];
        q->mode |= 0x4000;
        q++;
     }
 } while (TRUE);

 return modes;
}



/* Desc: save current mode
 *
 * In  : ptr to mode structure
 * Out : 0 if success
 *
 * Note: shouldn't use VESA...
 */
static int _mga_mode_save (MGA_MODE *p)
{
 __asm("\n\
		movw	$0x4f03, %%ax	\n\
		int	$0x10		\n\
		movl	%%ebx, %0	\n\
 ":"=g"(p->mode)::"%eax", "%ebx");
 return 0;
}



/* Desc: switch to specified mode
 *
 * In  : ptr to mode structure, refresh rate
 * Out : 0 if success
 *
 * Note: shouldn't use VESA...
 */
int mga_mode_switch (MGA_MODE *p, int refresh)
{
 if (oldmode.mode == 0) {
    _mga_mode_save(&oldmode);
 }
 __asm("movw $0x4f02, %%ax; int  $0x10"::"b"(p->mode):"%eax");
 return 0;

 (void)refresh; /* silence compiler warning */
}



/* Desc: restore to the mode prior to first call to `mga_switch'
 *
 * In  : -
 * Out : 0 if success
 *
 * Note: shouldn't use VESA...
 */
int mga_mode_restore (void)
{
 if (oldmode.mode != 0) {
    __asm("movw $0x4f02, %%ax; int  $0x10"::"b"(oldmode.mode):"%eax");
    oldmode.mode = 0;
 }
 return 0;
}



/* Desc: return suitable mode
 *
 * In  : width, height, bpp
 * Out : ptr to mode structure
 *
 * Note: -
 */
MGA_MODE *mga_mode_find (int width, int height, int bpp)
{
 static MGA_MODE *q = NULL;

 MGA_MODE *p;
 unsigned int min;

 if (q == NULL) {
    if ((q = _mga_mode_check()) == NULL) {
       return NULL;
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

 return p;
}
