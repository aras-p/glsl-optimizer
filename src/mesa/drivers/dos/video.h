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


#ifndef VIDEO_H_included
#define VIDEO_H_included

int vl_video_init (int width, int height, int bpp);
void vl_video_exit (int textmode);

void *vl_sync_buffer (void *buffer, int x, int y, int width, int height);

extern void (*vl_clear) (void *buffer, int len, int color);
void vl_rect (void *buffer, int x, int y, int width, int height, int color);

void (*vl_flip) (void *buffer, int width, int height);

extern int (*vl_mixrgba) (const unsigned char rgba[]);
extern int (*vl_mixrgb) (const unsigned char rgb[]);
extern void (*vl_putpixel) (void *buffer, int offset, int color);
extern void (*vl_getrgba) (void *buffer, int offset, unsigned char rgba[4]);

#endif
