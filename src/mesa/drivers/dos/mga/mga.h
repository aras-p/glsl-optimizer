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
 * DOS/DJGPP device driver v1.3 for Mesa  --  MGA2064W
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef MGA_H_included
#define MGA_H_included

#define MGA_GET_CARD_NAME   0x0100
#define MGA_GET_VRAM        0x0101
#define MGA_GET_CI_PREC     0x0200
#define MGA_GET_HPIXELS     0x0201
#define MGA_GET_SCREEN_SIZE 0x0202

int mga_open (int width, int height, int bpp, int buffers, int zbuffer, int refresh);
void mga_clear (int front, int back, int zed, int left, int top, int width, int height, int color, unsigned short z);
int mga_get (int pname, int *params);
void mga_close (int restore, int unmap);

extern void (*mga_putpixel) (unsigned int offset, int color);
extern int (*mga_getpixel) (unsigned int offset);
extern void (*mga_getrgba) (unsigned int offset, unsigned char rgba[4]);
extern int (*mga_mixrgb) (const unsigned char rgb[]);

#define MGA_BACKBUFFER !0
#define MGA_FRONTBUFFER 0
void mga_set_readbuffer (int buffer);
void mga_set_writebuffer (int buffer);
void mga_swapbuffers (int swapinterval);

unsigned short mga_getz (int offset);
void mga_setz (int offset, unsigned short z);

void mga_wait_idle (void);

/*
 * vertex structure, used for primitive rendering
 */
typedef struct {
        int win[4];             /* X, Y, Z, ? */
        unsigned char color[4]; /* R, G, B, A */
} MGAvertex;

void mga_draw_line_rgb_flat (const MGAvertex *v1, const MGAvertex *v2);
void mga_draw_line_rgb_flat_zless (const MGAvertex *v1, const MGAvertex *v2);
void mga_draw_line_rgb_iter (const MGAvertex *v1, const MGAvertex *v2);
void mga_draw_line_rgb_iter_zless (const MGAvertex *v1, const MGAvertex *v2);

void mga_draw_tri_rgb_flat (int cull, const MGAvertex *v1, const MGAvertex *v2, const MGAvertex *v3);
void mga_draw_tri_rgb_flat_zless (int cull, const MGAvertex *v1, const MGAvertex *v2, const MGAvertex *v3);
void mga_draw_tri_rgb_iter (int cull, const MGAvertex *v1, const MGAvertex *v2, const MGAvertex *v3);
void mga_draw_tri_rgb_iter_zless (int cull, const MGAvertex *v1, const MGAvertex *v2, const MGAvertex *v3);

void mga_draw_rect_rgb_flat (int left, int top, int width, int height, int color);
void mga_draw_rect_rgb_tx32 (int left, int top, int width, int height, const unsigned long *bitmap);
void mga_draw_rect_rgb_tx24 (int left, int top, int width, int height, const unsigned long *bitmap);
void mga_draw_span_rgb_tx32 (int left, int top, int width, const unsigned long *bitmap);
void mga_draw_span_rgb_tx24 (int left, int top, int width, const unsigned long *bitmap);

void mga_iload_32RGB (int left, int top, int width, int height, const unsigned long *bitmap);
void mga_iload_24RGB (int left, int top, int width, int height, const unsigned long *bitmap);

#endif
