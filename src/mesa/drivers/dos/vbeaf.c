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
 * DOS/DJGPP device driver v0.2 for Mesa 4.0
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#include <ctype.h>
#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/exceptn.h>
#include <sys/nearptr.h>

#include "dpmiint.h"
#include "vbeaf.h"



#define MMAP      unsigned long
#define NOMM      0
#define MVAL(a)   (void *)((a)-__djgpp_base_address)



#define SAFE_CALL(FUNC)\
{					\
 int _ds, _es, _ss;			\
					\
 __asm__ __volatile__(			\
	" movw %%ds, %w0 ; "		\
	" movw %%es, %w1 ; "		\
	" movw %%ss, %w2 ; "		\
	" movw %w3, %%ds ; "		\
	" movw %w3, %%es ; "		\
	" movw %w3, %%ss ; "		\
					\
	: "=&q" (_ds),			\
	"=&q" (_es),			\
	"=&q" (_ss)			\
	: "q" (__djgpp_ds_alias)	\
 );					\
					\
 FUNC					\
					\
 __asm__ __volatile__(			\
	"movw %w0, %%ds ; "		\
	"movw %w1, %%es ; "		\
	"movw %w2, %%ss ; "		\
	:				\
	: "q" (_ds),			\
	"q" (_es),			\
	"q" (_ss)			\
 );					\
}

#define SAFISH_CALL(FUNC)  FUNC



extern void _af_int86(), _af_call_rm(), _af_wrapper_end();



static AF_DRIVER *af_driver = NULL;    /* the VBE/AF driver */
int _accel_active = FALSE;             /* is the accelerator busy? */
static FAF_HWPTR_DATA *faf_farptr;     /* FreeBE/AF farptr data */
static MMAP af_memmap[4] = { NOMM, NOMM, NOMM, NOMM };
static MMAP af_banked_mem = NOMM;
static MMAP af_linear_mem = NOMM;



__inline__ fixed itofix (int x)
{
 return x << 16;
}



/*
 * loads the driver from disk
 */
static int load_driver (char *filename)
{
 FILE *f;
 size_t size;

 if ((f=fopen(filename, "rb"))!=NULL) {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if ((af_driver=(AF_DRIVER *)malloc(size))!=NULL) {
       if (fread(af_driver, size, 1, f)) {
          return 0;
       } else {
          free(af_driver);
       }
    }
    fclose(f);
 }

 return -1;
}



/*
 *  Calls a VBE/AF function using the version 1.0 style asm interface.
 */
static int call_vbeaf_asm (void *proc)
{
 int ret;

 proc = (void *)((long)af_driver + (long)proc);

 /* use gcc-style inline asm */
 __asm__("\n\
		pushl	%%ebx		\n\
		movl	%%ecx, %%ebx	\n\
		call	*%%edx		\n\
		popl	%%ebx		\n\
 ": "=&a" (ret)                       /* return value in eax */
  : "c" (af_driver),                  /* VBE/AF driver in ds:ebx */
    "d" (proc)                        /* function ptr in edx */
  : "memory"                          /* assume everything is clobbered */
 );

 return ret;
}



/*
 * prepares the FreeBE/AF extension functions.
 */
static int initialise_freebeaf_extensions (void)
{
 typedef unsigned long (*EXT_INIT_FUNC)(AF_DRIVER *af, unsigned long id);
 EXT_INIT_FUNC ext_init;
 unsigned long magic;
 int v1, v2;
 void *ptr;

 /* safety check */
 if (!af_driver->OemExt) {
    return 0;
 }

 /* call the extension init function */
 ext_init = (EXT_INIT_FUNC)((long)af_driver + (long)af_driver->OemExt);

 magic = ext_init(af_driver, FAFEXT_INIT);

 /* check that it returned a nice magic number */
 v1 = (magic>>8)&0xFF;
 v2 = magic&0xFF;

 if (((magic&0xFFFF0000) != FAFEXT_MAGIC) || (!isdigit(v1)) || (!isdigit(v2))) {
    return 0;
 }

 /* export libc and pmode functions if the driver wants them */
 ptr = af_driver->OemExt(af_driver, FAFEXT_LIBC);
#ifdef _FAFEXT_LIBC
 if (ptr)
    _fill_vbeaf_libc_exports(ptr);
#endif

 ptr = af_driver->OemExt(af_driver, FAFEXT_PMODE);
#ifdef _FAFEXT_PMODE
 if (ptr)
    _fill_vbeaf_pmode_exports(ptr);
#endif

 return (v1-'0')*10 + (v2-'0');
}



/*
 *  Sets up the DPMI memory mappings required by the VBE/AF driver, 
 *  returning zero on success.
 */
static int initialise_vbeaf_driver (int faf_ext)
{
 int c;

 /* query driver for the FreeBE/AF farptr extension */
 if (faf_ext > 0)
    faf_farptr = af_driver->OemExt(af_driver, FAFEXT_HWPTR);
 else
    faf_farptr = NULL;

 if (faf_farptr) {
    /* use farptr access */
    for (c=0; c<4; c++) {
        faf_farptr->IOMemMaps[c].sel = 0;
        faf_farptr->IOMemMaps[c].offset = 0;
    }

    faf_farptr->BankedMem.sel = 0;
    faf_farptr->BankedMem.offset = 0;

    faf_farptr->LinearMem.sel = 0;
    faf_farptr->LinearMem.offset = 0;
 } else {
    /* enable nearptr access */
    return -5;
 }

 /* create mapping for MMIO ports */
 for (c=0; c<4; c++) {
     if (af_driver->IOMemoryBase[c]) {
        if ((af_memmap[c]=_create_linear_mapping(af_driver->IOMemoryBase[c], af_driver->IOMemoryLen[c]))==NULL)
           return -4;

        if (faf_farptr) {
           /* farptr IO mapping */
           if ((faf_farptr->IOMemMaps[c].sel=_create_selector(af_memmap[c], af_driver->IOMemoryLen[c]))<0) {
              _remove_linear_mapping(af_memmap+c);
              return -4;
           }
           faf_farptr->IOMemMaps[c].offset = 0;
           af_driver->IOMemMaps[c] = NULL;
        } else {
           /* nearptr IO mapping */
           af_driver->IOMemMaps[c] = MVAL(af_memmap[c]);
        }
     }
 }

 /* create mapping for banked video RAM */
 if (af_driver->BankedBasePtr) {
    if ((af_banked_mem=_create_linear_mapping(af_driver->BankedBasePtr, 0x10000))==NULL)
       return -4;

    if (faf_farptr) {
       /* farptr banked vram mapping */
       if ((faf_farptr->BankedMem.sel=_create_selector(af_banked_mem, 0x10000))<0) {
          _remove_linear_mapping(&af_banked_mem);
          return -4;
       }
       faf_farptr->BankedMem.offset = 0;
       af_driver->BankedMem = NULL;
    } else {
       /* nearptr banked vram mapping */
       af_driver->BankedMem = MVAL(af_banked_mem);
    }
 }

 /* create mapping for linear video RAM */
 if (af_driver->LinearBasePtr) {
    if ((af_linear_mem=_create_linear_mapping(af_driver->LinearBasePtr, af_driver->LinearSize*1024))==NULL)
       return -4;

    if (faf_farptr) {
       /* farptr linear vram mapping */
       if ((faf_farptr->LinearMem.sel=_create_selector(af_linear_mem, af_driver->LinearSize*1024))<0) {
          _remove_linear_mapping(&af_linear_mem);
          return -4;
       }
       faf_farptr->LinearMem.offset = 0;
       af_driver->LinearMem = NULL;
    } else {
       /* nearptr linear vram mapping */
       af_driver->LinearMem  = MVAL(af_linear_mem);
    }
 }

 /* callback functions: why are these needed? ugly, IMHO */
 af_driver->Int86 = (void *)_af_int86;
 af_driver->CallRealMode = (void *)_af_call_rm;

 return 0;
}



/* go_accel:
 *  Prepares the hardware for an accelerated drawing operation.
 */
static __inline__ void go_accel (void)
{
   /* turn on the accelerator */
   if (!_accel_active) {
      if (af_driver->DisableDirectAccess)
	 af_driver->DisableDirectAccess(af_driver);

      _accel_active = TRUE;
   }
}



void af_exit (void)
{
 int c;

 /* undo memory mappings */
 if (faf_farptr) {
    for (c=0; c<4; c++)
        _remove_selector(&faf_farptr->IOMemMaps[c].sel);

    _remove_selector(&faf_farptr->BankedMem.sel);
    _remove_selector(&faf_farptr->LinearMem.sel);
 }

 for (c=0; c<4; c++)
     _remove_linear_mapping(af_memmap+c);

 _remove_linear_mapping(&af_banked_mem);
 _remove_linear_mapping(&af_linear_mem);

 if (af_driver) {
    free(af_driver);
 }
}



int af_init (char *filename)
{
 int faf_ext, r;

 if (load_driver(filename)) {
    /* driver not found */
    return -1;
 }

 faf_ext = initialise_freebeaf_extensions();

 /* special setup for Plug and Play hardware */
 if (call_vbeaf_asm(af_driver->PlugAndPlayInit)) {
    af_exit();
    /* Plug and Play initialisation failed */
    return -2;
 }

 if ((r=initialise_vbeaf_driver(faf_ext))) {
    af_exit();
    /* -4, ... */
    return r;
 }

 /* low level driver initialisation */
 if (call_vbeaf_asm(af_driver->InitDriver)) {
    af_exit();
    /* VBE/AF device not present */
    return -3;
 }

 _go32_dpmi_lock_code((void *)_af_int86, (long)_af_wrapper_end-(long)_af_int86);

 return 0;
}



#include "video.h"



static long page, xres, startx, starty;



AF_MODE_INFO *af_getmode (int i, int *granularity)
{
 static AF_MODE_INFO info;
 int mode = af_driver->AvailableModes[i];

 if ((mode==-1)||af_driver->GetVideoModeInfo(af_driver, mode, &info)) {
    return NULL;
 } else {
    *granularity = af_driver->BankSize * 1024;
    return &info;
 }
}



void *af_dump_virtual (void *buffer, int width, int height, int pitch)
{
 SAFISH_CALL(
             go_accel();

             af_driver->BitBltSys(af_driver, buffer, pitch, 0, 0, width, height, startx, starty, 0);
 );
 return buffer;
}



void af_switch_bank (int bank)
{
 af_driver->SetBank(af_driver, bank);
}



int af_flip (void)
{
 SAFISH_CALL(
             go_accel();
 
             af_driver->SetDisplayStart(af_driver, page*xres, 0, 1);
 );
 if (page ^= 1) {
    startx += xres;
 } else {
    startx -= xres;
 }

 return page;
}



void af_fillrect (void *buffer, int x, int y, int width, int height, int color)
{
 (void)buffer;
 SAFISH_CALL(
             go_accel();

             af_driver->DrawRect(af_driver, color, startx+x, starty+y, width, height);
 );
}



void af_triangle (int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
 AF_TRAP trap;

 /* sort along the vertical axis */
 #define TRI_SWAP(a, b)		\
 {				\
  int tmp = x##a;		\
  x##a = x##b;			\
  x##b = tmp;			\
				\
  tmp = y##a;			\
  y##a = y##b;			\
  y##b = tmp;			\
 }

 if (y2 < y1)
    TRI_SWAP(1, 2);

 if (y3 < y1) {
    TRI_SWAP(1, 3);
    TRI_SWAP(2, 3);
 } else if (y3 < y2)
    TRI_SWAP(2, 3);

 SAFISH_CALL(
             go_accel();

             if (y2 > y1) {
	        /* draw the top half of the triangle as one trapezoid */
	        trap.y = y1+starty;
	        trap.count = y2-y1;
	        trap.x1 = trap.x2 = itofix(x1+startx);
	        trap.slope1 = itofix(x2-x1) / (y2-y1);
	        trap.slope2 = itofix(x3-x1) / (y3-y1);

                if (trap.slope1 < 0)
                   trap.x1 += MIN(trap.slope1+itofix(1), 0);

                if (trap.slope2 < 0)
	           trap.x2 += MIN(trap.slope2+itofix(1), 0);

                if (trap.slope1 > trap.slope2)
	           trap.x1 += MAX(ABS(trap.slope1), itofix(1));
                else
	           trap.x2 += MAX(ABS(trap.slope2), itofix(1));

                af_driver->DrawTrap(af_driver, color, &trap);

                if (y3 > y2) {
	           /* draw the bottom half as a second trapezoid */
	           if (trap.slope1 < 0)
	              trap.x1 -= MIN(trap.slope1+itofix(1), 0);

                   if (trap.slope1 > trap.slope2)
	              trap.x1 -= MAX(ABS(trap.slope1), itofix(1));

                   trap.count = y3-y2;
	           trap.slope1 = itofix(x3-x2) / (y3-y2);

                   if (trap.slope1 < 0)
	              trap.x1 += MIN(trap.slope1+itofix(1), 0);

                   if (trap.x1 > trap.x2)
	              trap.x1 += MAX(ABS(trap.slope1), itofix(1));

                   af_driver->DrawTrap(af_driver, color, &trap);
                }
             } else if (y3 > y2) {
                /* we can draw the entire thing with a single trapezoid */
                trap.y = y1+starty;
                trap.count = y3-y2;
                trap.x1 = itofix(x2+startx);
                trap.x2 = itofix(x1+startx);
                trap.slope1 = itofix(x3-x2) / (y3-y2);
                trap.slope2 = (y3 > y1) ? (itofix(x3-x1) / (y3-y1)) : 0;

                if (trap.slope1 < 0)
                   trap.x1 += MIN(trap.slope1+itofix(1), 0);

                if (trap.slope2 < 0)
                   trap.x2 += MIN(trap.slope2+itofix(1), 0);

                if (trap.x1 > trap.x2)
                   trap.x1 += MAX(ABS(trap.slope1), itofix(1));
                else
                   trap.x2 += MAX(ABS(trap.slope2), itofix(1));

                af_driver->DrawTrap(af_driver, color, &trap);
             }
 );
}



int af_setup_mode (int mode, int caps)
{
 if (CHECK_SOFTDB(caps) && af_driver->BitBltSys) {
    vl_flip = af_dump_virtual;
 }
 if (af_driver->SetBank) {
    vl_switch_bank = af_switch_bank;
 }
 if (!CHECK_SOFTDB(caps) && af_driver->DrawRect) {
    vl_clear = af_fillrect;
 }

 if (CHECK_SINGLE(caps) || CHECK_SOFTDB(caps)) {
    page = 0;
 } else {
    page = 1;
 }

 return (mode&0x4000)?faf_farptr->LinearMem.sel:faf_farptr->BankedMem.sel;
}



int af_setup_buffer (int x, int y)
{
 startx = x + page*xres;
 starty = y;

 return page;
}



void *af_getprim (int primitive)
{
 switch (primitive) {
        case TRI_RGB_FLAT:
             return af_driver->DrawTrap?(void *)af_triangle:NULL;
        default:
             return NULL;
 }
}



int af_enter_mode (int mode, int width, int height)
{
 long wret;
 if (!af_driver->SetVideoMode(af_driver, mode, width, height, &wret, 1, NULL)) {
    xres = width/2;
    return wret;
 } else {
    return -1;
 }
}
