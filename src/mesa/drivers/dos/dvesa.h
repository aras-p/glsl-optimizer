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


#ifndef DVESA_H_included
#define DVESA_H_included

typedef unsigned char word8;
typedef unsigned short word16;
typedef unsigned long word32;

struct dvmode;

extern int (*dv_color) (const unsigned char rgba[]);
extern void (*dv_dump_virtual) (void *buffer, int width, int height, int offset, int delta);
extern void (*dv_clear_virtual) (void *buffer, int len, int color);

struct dvmode *dv_select_mode (int x, int y, int width, int height, int depth, int *delta, int *offset);
void dv_fillrect (void *buffer, int bwidth, int x, int y, int width, int height, int color);

/*
 * not so public...
 */
extern int _rgb_scale_5[32];
extern int _rgb_scale_6[64];

/*
 * dv_putpixel inlines
 */
#define dv_putpixel15 dv_putpixel16
extern __inline__ void dv_putpixel16 (void *buffer, int offset, int color)
{
 ((word16 *)buffer)[offset] = (word16)color;
}
extern __inline__ void dv_putpixel24 (void *buffer, int offset, int color)
{
 *(word16 *)(buffer+offset*3) = (word16)color;
 *(word8 *)(buffer+offset*3+2) = (word8)(color>>16);
}
extern __inline__ void dv_putpixel32 (void *buffer, int offset, int color)
{
 ((word32 *)buffer)[offset] = color;
}

/*
 * dv_color inlines
 */
extern __inline__ int dv_color15 (const unsigned char rgba[])
{
 return ((rgba[0]>>3)<<10)|((rgba[1]>>3)<<5)|(rgba[2]>>3);
}
extern __inline__ int dv_color16 (const unsigned char rgba[])
{
 return ((rgba[0]>>3)<<11)|((rgba[1]>>2)<<5)|(rgba[2]>>3);
}
#define dv_color24 dv_color32
extern __inline__ int dv_color32 (const unsigned char rgba[])
{
 return (rgba[0]<<16)|(rgba[1]<<8)|(rgba[2]);
}

/*
 * dv_getrgba inlines
 */
extern __inline__ void dv_getrgba15 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word16 *)buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 10) & 0x1F];
 rgba[1] = _rgb_scale_5[(c >> 5) & 0x1F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
extern __inline__ void dv_getrgba16 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word16 *)buffer)[offset];
 rgba[0] = _rgb_scale_5[(c >> 11) & 0x1F];
 rgba[1] = _rgb_scale_6[(c >> 5) & 0x3F];
 rgba[2] = _rgb_scale_5[c & 0x1F];
 rgba[3] = 255;
}
extern __inline__ void dv_getrgba24 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = *(word32 *)(buffer+offset*3);
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 rgba[3] = 255;
}
extern __inline__ void dv_getrgba32 (void *buffer, int offset, unsigned char rgba[4])
{
 int c = ((word32 *)buffer)[offset];
 rgba[0] = c >> 16;
 rgba[1] = c >> 8;
 rgba[2] = c;
 rgba[3] = 255;
}

#endif
