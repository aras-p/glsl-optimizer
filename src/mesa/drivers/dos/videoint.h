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


#ifndef VIDEOINT_H_included
#define VIDEOINT_H_included

/*
 * general purpose defines, etc.
 */
#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

#define __PACKED__ __attribute__((packed))

typedef unsigned char word8;
typedef unsigned short word16;
typedef unsigned long word32;

#define _16_ *(word16 *)&
#define _32_ *(word32 *)&



/*
 * video mode structure
 */
typedef struct vl_mode {
        int mode;
        int xres, yres;
        int scanlen;
        int bpp;

        int sel;
        int gran;
} vl_mode;



/*
 * video driver structure
 */
typedef struct {
        vl_mode *(*getmodes) (void);
        int (*entermode) (vl_mode *p, int refresh);
        void (*restore) (void);
        void (*finit) (void);
} vl_driver;



/*
 * asm routines to deal with virtual buffering
 */
#define v_clear15 v_clear16
extern void v_clear16 (void *buffer, int bytes, int color);
extern void v_clear32 (void *buffer, int bytes, int color);
extern void v_clear24 (void *buffer, int bytes, int color);

extern void b_dump_virtual (void *buffer, int stride, int height);
extern void l_dump_virtual (void *buffer, int stride, int height);

#define v_putpixel15 v_putpixel16
extern void v_putpixel16 (void *buffer, int offset, int color);
extern void v_putpixel24 (void *buffer, int offset, int color);
extern void v_putpixel32 (void *buffer, int offset, int color);

#endif
