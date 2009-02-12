/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * Authors:
 *    George Sapountzis <gsap7@yahoo.gr>
 */

#include "swrast_priv.h"

#define YFLIP(_xrb, Y) ((_xrb)->Base.Height - (Y) - 1)

/*
 * Dithering support takes the "computation" extreme in the "computation vs.
 * storage" trade-off. This approach is very simple to implement and any
 * computational overhead should be acceptable. XMesa uses table lookups for
 * around 8KB of storage overhead per visual.
 */
#define DITHER 1

static const GLubyte kernel[16] = {
    0*16,  8*16,  2*16, 10*16,
   12*16,  4*16, 14*16,  6*16,
    3*16, 11*16,  1*16,  9*16,
   15*16,  7*16, 13*16,  5*16,
};

#if DITHER
#define DITHER_COMP(X, Y) kernel[((X) & 0x3) | (((Y) & 0x3) << 2)]

#define DITHER_CLAMP(X) (((X) < CHAN_MAX) ? (X) : CHAN_MAX)
#else
#define DITHER_COMP(X, Y) 0

#define DITHER_CLAMP(X) (X)
#endif


/*
 * Pixel macros shared across front/back buffer span functions.
 */

/* 32-bit BGRA */
#define STORE_PIXEL_A8R8G8B8(DST, X, Y, VALUE) \
   DST[3] = VALUE[ACOMP]; \
   DST[2] = VALUE[RCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[0] = VALUE[BCOMP]
#define STORE_PIXEL_RGB_A8R8G8B8(DST, X, Y, VALUE) \
   DST[3] = 0xff; \
   DST[2] = VALUE[RCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL_A8R8G8B8(DST, SRC) \
   DST[ACOMP] = SRC[3]; \
   DST[RCOMP] = SRC[2]; \
   DST[GCOMP] = SRC[1]; \
   DST[BCOMP] = SRC[0]


/* 32-bit BGRX */
#define STORE_PIXEL_X8R8G8B8(DST, X, Y, VALUE) \
   DST[3] = 0xff; \
   DST[2] = VALUE[RCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[0] = VALUE[BCOMP]
#define STORE_PIXEL_RGB_X8R8G8B8(DST, X, Y, VALUE) \
   DST[3] = 0xff; \
   DST[2] = VALUE[RCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[0] = VALUE[BCOMP]
#define FETCH_PIXEL_X8R8G8B8(DST, SRC) \
   DST[ACOMP] = 0xff; \
   DST[RCOMP] = SRC[2]; \
   DST[GCOMP] = SRC[1]; \
   DST[BCOMP] = SRC[0]


/* 16-bit BGR */
#define STORE_PIXEL_R5G6B5(DST, X, Y, VALUE) \
   do { \
   int d = DITHER_COMP(X, Y) >> 6; \
   GLushort *p = (GLushort *)DST; \
   *p = ( ((DITHER_CLAMP((VALUE[RCOMP]) + d) & 0xf8) << 8) | \
	  ((DITHER_CLAMP((VALUE[GCOMP]) + d) & 0xfc) << 3) | \
	  ((DITHER_CLAMP((VALUE[BCOMP]) + d) & 0xf8) >> 3) ); \
   } while(0)
#define FETCH_PIXEL_R5G6B5(DST, SRC) \
   do { \
   GLushort p = *(GLushort *)SRC; \
   DST[ACOMP] = 0xff; \
   DST[RCOMP] = ((p >> 8) & 0xf8) * 255 / 0xf8; \
   DST[GCOMP] = ((p >> 3) & 0xfc) * 255 / 0xfc; \
   DST[BCOMP] = ((p << 3) & 0xf8) * 255 / 0xf8; \
   } while(0)


/* 8-bit BGR */
#define STORE_PIXEL_R3G3B2(DST, X, Y, VALUE) \
   do { \
   int d = DITHER_COMP(X, Y) >> 3; \
   GLubyte *p = (GLubyte *)DST; \
   *p = ( ((DITHER_CLAMP((VALUE[RCOMP]) + d) & 0xe0) >> 5) | \
	  ((DITHER_CLAMP((VALUE[GCOMP]) + d) & 0xe0) >> 2) | \
	  ((DITHER_CLAMP((VALUE[BCOMP]) + d) & 0xc0) >> 0) ); \
   } while(0)
#define FETCH_PIXEL_R3G3B2(DST, SRC) \
   do { \
   GLubyte p = *(GLubyte *)SRC; \
   DST[ACOMP] = 0xff; \
   DST[RCOMP] = ((p << 5) & 0xe0) * 255 / 0xe0; \
   DST[GCOMP] = ((p << 2) & 0xe0) * 255 / 0xe0; \
   DST[BCOMP] = ((p << 0) & 0xc0) * 255 / 0xc0; \
   } while(0)


/*
 * Generate code for back-buffer span functions.
 */

/* 32-bit BGRA */
#define NAME(FUNC) FUNC##_A8R8G8B8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)xrb->Base.Data + YFLIP(xrb, Y) * xrb->pitch + (X) * 4;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_A8R8G8B8(DST, X, Y, VALUE)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   STORE_PIXEL_RGB_A8R8G8B8(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_A8R8G8B8(DST, SRC)

#include "swrast/s_spantemp.h"


/* 32-bit BGRX */
#define NAME(FUNC) FUNC##_X8R8G8B8
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)xrb->Base.Data + YFLIP(xrb, Y) * xrb->pitch + (X) * 4;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_X8R8G8B8(DST, X, Y, VALUE)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   STORE_PIXEL_RGB_X8R8G8B8(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_X8R8G8B8(DST, SRC)

#include "swrast/s_spantemp.h"


/* 16-bit BGR */
#define NAME(FUNC) FUNC##_R5G6B5
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)xrb->Base.Data + YFLIP(xrb, Y) * xrb->pitch + (X) * 2;
#define INC_PIXEL_PTR(P) P += 2
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_R5G6B5(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_R5G6B5(DST, SRC)

#include "swrast/s_spantemp.h"


/* 8-bit BGR */
#define NAME(FUNC) FUNC##_R3G3B2
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)xrb->Base.Data + YFLIP(xrb, Y) * xrb->pitch + (X) * 1;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_R3G3B2(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_R3G3B2(DST, SRC)

#include "swrast/s_spantemp.h"


/* 8-bit color index */
#define NAME(FUNC) FUNC##_CI8
#define CI_MODE
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)xrb->Base.Data + YFLIP(xrb, Y) * xrb->pitch + (X);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]

#include "swrast/s_spantemp.h"


/*
 * Generate code for front-buffer span functions.
 */

/* 32-bit BGRA */
#define NAME(FUNC) FUNC##_A8R8G8B8_front
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)row;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_A8R8G8B8(DST, X, Y, VALUE)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   STORE_PIXEL_RGB_A8R8G8B8(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_A8R8G8B8(DST, SRC)

#include "swrast_spantemp.h"


/* 32-bit BGRX */
#define NAME(FUNC) FUNC##_X8R8G8B8_front
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)row;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_X8R8G8B8(DST, X, Y, VALUE)
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   STORE_PIXEL_RGB_X8R8G8B8(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_X8R8G8B8(DST, SRC)

#include "swrast_spantemp.h"


/* 16-bit BGR */
#define NAME(FUNC) FUNC##_R5G6B5_front
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)row;
#define INC_PIXEL_PTR(P) P += 2
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_R5G6B5(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_R5G6B5(DST, SRC)

#include "swrast_spantemp.h"


/* 8-bit BGR */
#define NAME(FUNC) FUNC##_R3G3B2_front
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)row;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   STORE_PIXEL_R3G3B2(DST, X, Y, VALUE)
#define FETCH_PIXEL(DST, SRC) \
   FETCH_PIXEL_R3G3B2(DST, SRC)

#include "swrast_spantemp.h"


/* 8-bit color index */
#define NAME(FUNC) FUNC##_CI8_front
#define CI_MODE
#define RB_TYPE GLubyte
#define SPAN_VARS \
   struct swrast_renderbuffer *xrb = swrast_renderbuffer(rb);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)row;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]

#include "swrast_spantemp.h"


/*
 * Back-buffers are malloced memory and always private.
 *
 * BACK_PIXMAP (not supported)
 * BACK_XIMAGE
 */
void
swrast_set_span_funcs_back(struct swrast_renderbuffer *xrb,
			   GLuint pixel_format)
{
    switch (pixel_format) {
    case PF_A8R8G8B8:
	xrb->Base.GetRow = get_row_A8R8G8B8;
	xrb->Base.GetValues = get_values_A8R8G8B8;
	xrb->Base.PutRow = put_row_A8R8G8B8;
	xrb->Base.PutRowRGB = put_row_rgb_A8R8G8B8;
	xrb->Base.PutMonoRow = put_mono_row_A8R8G8B8;
	xrb->Base.PutValues = put_values_A8R8G8B8;
	xrb->Base.PutMonoValues = put_mono_values_A8R8G8B8;
	break;
    case PF_X8R8G8B8:
	xrb->Base.GetRow = get_row_X8R8G8B8;
	xrb->Base.GetValues = get_values_X8R8G8B8;
	xrb->Base.PutRow = put_row_X8R8G8B8;
	xrb->Base.PutRowRGB = put_row_rgb_X8R8G8B8;
	xrb->Base.PutMonoRow = put_mono_row_X8R8G8B8;
	xrb->Base.PutValues = put_values_X8R8G8B8;
	xrb->Base.PutMonoValues = put_mono_values_X8R8G8B8;
	break;
    case PF_R5G6B5:
	xrb->Base.GetRow = get_row_R5G6B5;
	xrb->Base.GetValues = get_values_R5G6B5;
	xrb->Base.PutRow = put_row_R5G6B5;
	xrb->Base.PutRowRGB = put_row_rgb_R5G6B5;
	xrb->Base.PutMonoRow = put_mono_row_R5G6B5;
	xrb->Base.PutValues = put_values_R5G6B5;
	xrb->Base.PutMonoValues = put_mono_values_R5G6B5;
	break;
    case PF_R3G3B2:
	xrb->Base.GetRow = get_row_R3G3B2;
	xrb->Base.GetValues = get_values_R3G3B2;
	xrb->Base.PutRow = put_row_R3G3B2;
	xrb->Base.PutRowRGB = put_row_rgb_R3G3B2;
	xrb->Base.PutMonoRow = put_mono_row_R3G3B2;
	xrb->Base.PutValues = put_values_R3G3B2;
	xrb->Base.PutMonoValues = put_mono_values_R3G3B2;
	break;
    case PF_CI8:
	xrb->Base.GetRow = get_row_CI8;
	xrb->Base.GetValues = get_values_CI8;
	xrb->Base.PutRow = put_row_CI8;
	xrb->Base.PutMonoRow = put_mono_row_CI8;
	xrb->Base.PutValues = put_values_CI8;
	xrb->Base.PutMonoValues = put_mono_values_CI8;
	break;
    default:
	assert(0);
	return;
    }
}


/*
 * Front-buffers are provided by the loader, the xorg loader uses pixmaps.
 *
 * WINDOW,          An X window
 * GLXWINDOW,       GLX window
 * PIXMAP,          GLX pixmap
 * PBUFFER          GLX Pbuffer
 */
void
swrast_set_span_funcs_front(struct swrast_renderbuffer *xrb,
			    GLuint pixel_format)
{
    switch (pixel_format) {
    case PF_A8R8G8B8:
	xrb->Base.GetRow = get_row_A8R8G8B8_front;
	xrb->Base.GetValues = get_values_A8R8G8B8_front;
	xrb->Base.PutRow = put_row_A8R8G8B8_front;
	xrb->Base.PutRowRGB = put_row_rgb_A8R8G8B8_front;
	xrb->Base.PutMonoRow = put_mono_row_A8R8G8B8_front;
	xrb->Base.PutValues = put_values_A8R8G8B8_front;
	xrb->Base.PutMonoValues = put_mono_values_A8R8G8B8_front;
	break;
    case PF_X8R8G8B8:
	xrb->Base.GetRow = get_row_X8R8G8B8_front;
	xrb->Base.GetValues = get_values_X8R8G8B8_front;
	xrb->Base.PutRow = put_row_X8R8G8B8_front;
	xrb->Base.PutRowRGB = put_row_rgb_X8R8G8B8_front;
	xrb->Base.PutMonoRow = put_mono_row_X8R8G8B8_front;
	xrb->Base.PutValues = put_values_X8R8G8B8_front;
	xrb->Base.PutMonoValues = put_mono_values_X8R8G8B8_front;
	break;
    case PF_R5G6B5:
	xrb->Base.GetRow = get_row_R5G6B5_front;
	xrb->Base.GetValues = get_values_R5G6B5_front;
	xrb->Base.PutRow = put_row_R5G6B5_front;
	xrb->Base.PutRowRGB = put_row_rgb_R5G6B5_front;
	xrb->Base.PutMonoRow = put_mono_row_R5G6B5_front;
	xrb->Base.PutValues = put_values_R5G6B5_front;
	xrb->Base.PutMonoValues = put_mono_values_R5G6B5_front;
	break;
    case PF_R3G3B2:
	xrb->Base.GetRow = get_row_R3G3B2_front;
	xrb->Base.GetValues = get_values_R3G3B2_front;
	xrb->Base.PutRow = put_row_R3G3B2_front;
	xrb->Base.PutRowRGB = put_row_rgb_R3G3B2_front;
	xrb->Base.PutMonoRow = put_mono_row_R3G3B2_front;
	xrb->Base.PutValues = put_values_R3G3B2_front;
	xrb->Base.PutMonoValues = put_mono_values_R3G3B2_front;
	break;
    case PF_CI8:
	xrb->Base.GetRow = get_row_CI8_front;
	xrb->Base.GetValues = get_values_CI8_front;
	xrb->Base.PutRow = put_row_CI8_front;
	xrb->Base.PutMonoRow = put_mono_row_CI8_front;
	xrb->Base.PutValues = put_values_CI8_front;
	xrb->Base.PutMonoValues = put_mono_values_CI8_front;
	break;
    default:
	assert(0);
	return;
    }
}
