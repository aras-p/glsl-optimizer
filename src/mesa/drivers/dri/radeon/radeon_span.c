/**************************************************************************

Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#include "main/glheader.h"
#include "swrast/swrast.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"

#include "drirenderbuffer.h"

#define DBG 0

/*
 * Note that all information needed to access pixels in a renderbuffer
 * should be obtained through the gl_renderbuffer parameter, not per-context
 * information.
 */
#define LOCAL_VARS						\
   struct radeon_renderbuffer *rrb = (void *) rb;		\
   const __DRIdrawablePrivate *dPriv = rrb->dPriv;		\
   const GLuint bottom = dPriv->h - 1;				\
   GLuint p;							\
   (void) p;				

#define LOCAL_DEPTH_VARS				\
   struct radeon_renderbuffer *rrb = (void *) rb;		\
   const __DRIdrawablePrivate *dPriv = rrb->dPriv;	\
   const GLuint bottom = dPriv->h - 1;			\
   GLuint xo = dPriv->x;				\
   GLuint yo = dPriv->y;

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

#define Y_FLIP(Y) (bottom - (Y))

#define HW_LOCK()

#define HW_UNLOCK()

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    radeon##x##_RGB565
#define TAG2(x,y) radeon##x##_RGB565##y
#define GET_PTR(X,Y) radeon_ptr16(rrb, (X), (Y))
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    radeon##x##_ARGB8888
#define TAG2(x,y) radeon##x##_ARGB8888##y
#define GET_PTR(X,Y) radeon_ptr32(rrb, (X), (Y))
#include "spantmp2.h"

/* 16-bit depth buffer functions
 */
#define VALUE_TYPE GLushort

#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)radeon_ptr(rrb, _x + xo, _y + yo) = d

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)radeon_ptr(rrb, _x + xo, _y + yo)

#define TAG(x) radeon##x##_z16
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define VALUE_TYPE GLuint

#ifdef COMPILE_R300
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0x000000ff;							\
   tmp |= ((d << 8) & 0xffffff00);					\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)
#else
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32(rrb, _x + xo, _y + yo);		\
   GLuint tmp = *_ptr;				\
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *_ptr = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_DEPTH( d, _x, _y )						\
  do {									\
    d = (*(GLuint *)(buf + radeon_mba_z32( drb, _x + xo,		\
					 _y + yo )) & 0xffffff00) >> 8; \
  }while(0)
#else
#define READ_DEPTH( d, _x, _y )						\
   do {									\
    d = (*(GLuint*)(radeon_ptr32(rrb, _x + xo, _y + yo)) & 0x00ffffff); \
   } while (0)
#endif

#define TAG(x) radeon##x##_z24_s8
#include "depthtmp.h"

/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#ifdef COMPILE_R300
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   tmp &= 0xffffff00;							\
   tmp |= (d) & 0xff;							\
   *(GLuint *)(buf + offset) = tmp;					\
} while (0)
#else
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32(rrb, _x + xo, _y + yo);		\
   GLuint tmp = *_ptr;				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *_ptr = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint offset = radeon_mba_z32( drb, _x + xo, _y + yo );		\
   GLuint tmp = *(GLuint *)(buf + offset);				\
   d = tmp & 0x000000ff;						\
} while (0)
#else
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32(rrb, _x + xo, _y + yo);		\
   GLuint tmp = *_ptr;							\
   d = (tmp & 0xff000000) >> 24;					\
} while (0)
#endif

#define TAG(x) radeon##x##_z24_s8
#include "stenciltmp.h"

void radeonInitSpanFuncs(GLcontext * ctx)
{
	struct swrast_device_driver *swdd =
	    _swrast_GetDeviceDriverReference(ctx);
	swdd->SpanRenderStart = radeonSpanRenderStart;
	swdd->SpanRenderFinish = radeonSpanRenderFinish;
}

/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void radeonSetSpanFunctions(struct radeon_renderbuffer *rrb)
{
	if (rrb->base.InternalFormat == GL_RGB5) {
		radeonInitPointers_RGB565(&rrb->base);
	} else if (rrb->base.InternalFormat == GL_RGBA8) {
		radeonInitPointers_ARGB8888(&rrb->base);
	} else if (rrb->base.InternalFormat == GL_DEPTH_COMPONENT16) {
		radeonInitDepthPointers_z16(&rrb->base);
	} else if (rrb->base.InternalFormat == GL_DEPTH_COMPONENT24) {
		radeonInitDepthPointers_z24_s8(&rrb->base);
	} else if (rrb->base.InternalFormat == GL_STENCIL_INDEX8_EXT) {
		radeonInitStencilPointers_z24_s8(&rrb->base);
	}
}
