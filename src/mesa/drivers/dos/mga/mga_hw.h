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
 * DOS/DJGPP device driver v1.3 for Mesa  --  MGA2064W HW mapping
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef MGA_HW_included
#define MGA_HW_included



/* set this to zero to use near pointers */
#define MGA_FARPTR 1



/* access macros */
#if MGA_FARPTR

#include <sys/farptr.h>

#define hwptr_pokeb(ptr, off, val)     _farpokeb((ptr).selector, (ptr).offset32+(off), (val))
#define hwptr_pokew(ptr, off, val)     _farpokew((ptr).selector, (ptr).offset32+(off), (val))
#define hwptr_pokel(ptr, off, val)     _farpokel((ptr).selector, (ptr).offset32+(off), (val))

#define hwptr_peekb(ptr, off)          _farpeekb((ptr).selector, (ptr).offset32+(off))
#define hwptr_peekw(ptr, off)          _farpeekw((ptr).selector, (ptr).offset32+(off))
#define hwptr_peekl(ptr, off)          _farpeekl((ptr).selector, (ptr).offset32+(off))

#define hwptr_select(ptr)              _farsetsel((ptr).selector)
#define hwptr_unselect(ptr)            (ptr).selector = _fargetsel()

#define hwptr_nspokeb(ptr, off, val)   _farnspokeb((ptr).offset32+(off), (val))
#define hwptr_nspokew(ptr, off, val)   _farnspokew((ptr).offset32+(off), (val))
#define hwptr_nspokel(ptr, off, val)   _farnspokel((ptr).offset32+(off), (val))

#define hwptr_nspeekb(ptr, off)        _farnspeekb((ptr).offset32+(off))
#define hwptr_nspeekw(ptr, off)        _farnspeekw((ptr).offset32+(off))
#define hwptr_nspeekl(ptr, off)        _farnspeekl((ptr).offset32+(off))

#else

#define hwptr_pokeb(ptr, off, val)     *((volatile unsigned char *)((ptr).offset32+(off))) = (val)
#define hwptr_pokew(ptr, off, val)     *((volatile unsigned short *)((ptr).offset32+(off))) = (val)
#define hwptr_pokel(ptr, off, val)     *((volatile unsigned long *)((ptr).offset32+(off))) = (val)

#define hwptr_peekb(ptr, off)          (*((volatile unsigned char *)((ptr).offset32+(off))))
#define hwptr_peekw(ptr, off)          (*((volatile unsigned short *)((ptr).offset32+(off))))
#define hwptr_peekl(ptr, off)          (*((volatile unsigned long *)((ptr).offset32+(off))))

#define hwptr_select(ptr)
#define hwptr_unselect(ptr)

#define hwptr_nspokeb(ptr, off, val)   *((volatile unsigned char *)((ptr).offset32+(off))) = (val)
#define hwptr_nspokew(ptr, off, val)   *((volatile unsigned short *)((ptr).offset32+(off))) = (val)
#define hwptr_nspokel(ptr, off, val)   *((volatile unsigned long *)((ptr).offset32+(off))) = (val)

#define hwptr_nspeekb(ptr, off)        (*((volatile unsigned char *)((ptr).offset32+(off))))
#define hwptr_nspeekw(ptr, off)        (*((volatile unsigned short *)((ptr).offset32+(off))))
#define hwptr_nspeekl(ptr, off)        (*((volatile unsigned long *)((ptr).offset32+(off))))

#endif



/* helpers for accessing the Matrox registers */
#define mga_select()          hwptr_select(mgaptr.io_mem_map[0])
#define mga_inb(addr)         hwptr_nspeekb(mgaptr.io_mem_map[0], addr)
#define mga_inw(addr)         hwptr_nspeekw(mgaptr.io_mem_map[0], addr)
#define mga_inl(addr)         hwptr_nspeekl(mgaptr.io_mem_map[0], addr)
#define mga_outb(addr, val)   hwptr_nspokeb(mgaptr.io_mem_map[0], addr, val)
#define mga_outw(addr, val)   hwptr_nspokew(mgaptr.io_mem_map[0], addr, val)
#define mga_outl(addr, val)   hwptr_nspokel(mgaptr.io_mem_map[0], addr, val)



typedef struct MGA_HWPTR {
        __dpmi_paddr io_mem_map[4], linear_map;
} MGA_HWPTR;

extern MGA_HWPTR mgaptr;

void mga_hw_fini (void);
int mga_hw_init (unsigned long *vram, int *interleave, char *name);

#endif
