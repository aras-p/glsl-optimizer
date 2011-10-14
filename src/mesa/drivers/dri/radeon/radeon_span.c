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
#include "main/texformat.h"
#include "swrast/swrast.h"

#include "radeon_common.h"
#include "radeon_span.h"

#define DBG 0

#if defined(BYTE_ORDER) && defined(BIG_ENDIAN) && BYTE_ORDER == BIG_ENDIAN
#if defined(__linux__)
#include <byteswap.h>
#define CPU_TO_LE16( x )	bswap_16( x )
#define LE16_TO_CPU( x )	bswap_16( x )
#endif /* __linux__ */
#else
#define CPU_TO_LE16( x )	( x )
#define LE16_TO_CPU( x )	( x )
#endif

static void radeonSetSpanFunctions(struct radeon_renderbuffer *rrb);


/* r200 depth buffer is always tiled - this is the formula
   according to the docs unless I typo'ed in it
*/
#if defined(RADEON_R200)
static GLubyte *r200_depth_2byte(const struct radeon_renderbuffer * rrb,
				 GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr + rrb->draw_offset;
    GLint offset;
    if (rrb->has_surface) {
	offset = x * rrb->cpp + y * rrb->pitch;
    } else {
	GLuint b;
	offset = 0;
	b = (((y  >> 4) * (rrb->pitch >> 8) + (x >> 6)));
	offset += (b >> 1) << 12;
	offset += (((rrb->pitch >> 8) & 0x1) ? (b & 0x1) : ((b & 0x1) ^ ((y >> 4) & 0x1))) << 11;
	offset += ((y >> 2) & 0x3) << 9;
	offset += ((x >> 3) & 0x1) << 8;
	offset += ((x >> 4) & 0x3) << 6;
	offset += ((x >> 2) & 0x1) << 5;
	offset += ((y >> 1) & 0x1) << 4;
	offset += ((x >> 1) & 0x1) << 3;
	offset += (y & 0x1) << 2;
	offset += (x & 0x1) << 1;
    }
    return &ptr[offset];
}

static GLubyte *r200_depth_4byte(const struct radeon_renderbuffer * rrb,
				 GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr + rrb->draw_offset;
    GLint offset;
    if (rrb->has_surface) {
	offset = x * rrb->cpp + y * rrb->pitch;
    } else {
	GLuint b;
	offset = 0;
	b = (((y & 0x7ff) >> 4) * (rrb->pitch >> 7) + (x >> 5));
	offset += (b >> 1) << 12;
	offset += (((rrb->pitch >> 7) & 0x1) ? (b & 0x1) : ((b & 0x1) ^ ((y >> 4) & 0x1))) << 11;
	offset += ((y >> 2) & 0x3) << 9;
	offset += ((x >> 2) & 0x1) << 8;
	offset += ((x >> 3) & 0x3) << 6;
	offset += ((y >> 1) & 0x1) << 5;
	offset += ((x >> 1) & 0x1) << 4;
	offset += (y & 0x1) << 3;
	offset += (x & 0x1) << 2;
    }
    return &ptr[offset];
}
#endif


/* radeon tiling on r300-r500 has 4 states,
   macro-linear/micro-linear
   macro-linear/micro-tiled
   macro-tiled /micro-linear
   macro-tiled /micro-tiled
   1 byte surface 
   2 byte surface - two types - we only provide 8x2 microtiling
   4 byte surface
   8/16 byte (unused)
*/
static GLubyte *radeon_ptr_4byte(const struct radeon_renderbuffer * rrb,
			     GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr + rrb->draw_offset;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
	    if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
		offset = ((y >> 4) * (rrb->pitch >> 7) + (x >> 5)) << 11;
		offset += (((y >> 3) ^ (x >> 5)) & 0x1) << 10;
		offset += (((y >> 4) ^ (x >> 4)) & 0x1) << 9;
		offset += (((y >> 2) ^ (x >> 4)) & 0x1) << 8;
		offset += (((y >> 3) ^ (x >> 3)) & 0x1) << 7;
		offset += ((y >> 1) & 0x1) << 6;
		offset += ((x >> 2) & 0x1) << 5;
		offset += (y & 1) << 4;
		offset += (x & 3) << 2;
            } else {
		offset = ((y >> 3) * (rrb->pitch >> 8) + (x >> 6)) << 11;
		offset += (((y >> 2) ^ (x >> 6)) & 0x1) << 10;
		offset += (((y >> 3) ^ (x >> 5)) & 0x1) << 9;
		offset += (((y >> 1) ^ (x >> 5)) & 0x1) << 8;
		offset += (((y >> 2) ^ (x >> 4)) & 0x1) << 7;
		offset += (y & 1) << 6;
		offset += (x & 15) << 2;
            }
        } else {
	    offset = ((y >> 1) * (rrb->pitch >> 4) + (x >> 2)) << 5;
	    offset += (y & 1) << 4;
	    offset += (x & 3) << 2;
        }
    }
    return &ptr[offset];
}

static GLubyte *radeon_ptr_2byte_8x2(const struct radeon_renderbuffer * rrb,
				     GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr + rrb->draw_offset;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
		offset = ((y >> 4) * (rrb->pitch >> 7) + (x >> 6)) << 11;
		offset += (((y >> 3) ^ (x >> 6)) & 0x1) << 10;
		offset += (((y >> 4) ^ (x >> 5)) & 0x1) << 9;
		offset += (((y >> 2) ^ (x >> 5)) & 0x1) << 8;
		offset += (((y >> 3) ^ (x >> 4)) & 0x1) << 7;
		offset += ((y >> 1) & 0x1) << 6;
		offset += ((x >> 3) & 0x1) << 5;
		offset += (y & 1) << 4;
		offset += (x & 3) << 2;
            } else {
		offset = ((y >> 3) * (rrb->pitch >> 8) + (x >> 7)) << 11;
		offset += (((y >> 2) ^ (x >> 7)) & 0x1) << 10;
		offset += (((y >> 3) ^ (x >> 6)) & 0x1) << 9;
		offset += (((y >> 1) ^ (x >> 6)) & 0x1) << 8;
		offset += (((y >> 2) ^ (x >> 5)) & 0x1) << 7;
		offset += (y & 1) << 6;
		offset += ((x >> 4) & 0x1) << 5;
                offset += (x & 15) << 2;
            }
        } else {
	    offset = ((y >> 1) * (rrb->pitch >> 4) + (x >> 3)) << 5;
	    offset += (y & 0x1) << 4;
	    offset += (x & 0x7) << 1;
        }
    }
    return &ptr[offset];
}

/*
 * Note that all information needed to access pixels in a renderbuffer
 * should be obtained through the gl_renderbuffer parameter, not per-context
 * information.
 */
#define LOCAL_VARS						\
   struct radeon_context *radeon = RADEON_CONTEXT(ctx);			\
   struct radeon_renderbuffer *rrb = (void *) rb;		\
   const GLint yScale = ctx->DrawBuffer->Name ? 1 : -1;			\
   const GLint yBias = ctx->DrawBuffer->Name ? 0 : rrb->base.Height - 1;\
   unsigned int num_cliprects;						\
   struct drm_clip_rect *cliprects;					\
   int x_off, y_off;							\
   GLuint p;						\
   (void)p;						\
   radeon_get_cliprects(radeon, &cliprects, &num_cliprects, &x_off, &y_off);

#define LOCAL_DEPTH_VARS				\
   struct radeon_context *radeon = RADEON_CONTEXT(ctx);			\
   struct radeon_renderbuffer *rrb = (void *) rb;	\
   const GLint yScale = ctx->DrawBuffer->Name ? 1 : -1;			\
   const GLint yBias = ctx->DrawBuffer->Name ? 0 : rrb->base.Height - 1;\
   unsigned int num_cliprects;						\
   struct drm_clip_rect *cliprects;					\
   int x_off, y_off;							\
  radeon_get_cliprects(radeon, &cliprects, &num_cliprects, &x_off, &y_off);

#define LOCAL_STENCIL_VARS LOCAL_DEPTH_VARS

#define Y_FLIP(_y) ((_y) * yScale + yBias)

#define HW_LOCK()

#define HW_UNLOCK()

/* XXX FBO: this is identical to the macro in spantmp2.h except we get
 * the cliprect info from the context, not the driDrawable.
 * Move this into spantmp2.h someday.
 */
#define HW_CLIPLOOP()							\
   do {									\
      int _nc = num_cliprects;						\
      while ( _nc-- ) {							\
	 int minx = cliprects[_nc].x1 - x_off;				\
	 int miny = cliprects[_nc].y1 - y_off;				\
	 int maxx = cliprects[_nc].x2 - x_off;				\
	 int maxy = cliprects[_nc].y2 - y_off;

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5

#define TAG(x)    radeon##x##_RGB565
#define TAG2(x,y) radeon##x##_RGB565##y
#define GET_PTR(X,Y) radeon_ptr_2byte_8x2(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5_REV

#define TAG(x)    radeon##x##_RGB565_REV
#define TAG2(x,y) radeon##x##_RGB565_REV##y
#define GET_PTR(X,Y) radeon_ptr_2byte_8x2(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

/* 16 bit, ARGB1555 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_1_5_5_5_REV

#define TAG(x)    radeon##x##_ARGB1555
#define TAG2(x,y) radeon##x##_ARGB1555##y
#define GET_PTR(X,Y) radeon_ptr_2byte_8x2(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_1_5_5_5

#define TAG(x)    radeon##x##_ARGB1555_REV
#define TAG2(x,y) radeon##x##_ARGB1555_REV##y
#define GET_PTR(X,Y) radeon_ptr_2byte_8x2(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

/* 16 bit, RGBA4 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_4_4_4_4_REV

#define TAG(x)    radeon##x##_ARGB4444
#define TAG2(x,y) radeon##x##_ARGB4444##y
#define GET_PTR(X,Y) radeon_ptr_2byte_8x2(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_4_4_4_4

#define TAG(x)    radeon##x##_ARGB4444_REV
#define TAG2(x,y) radeon##x##_ARGB4444_REV##y
#define GET_PTR(X,Y) radeon_ptr_2byte_8x2(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

/* 32 bit, xRGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    radeon##x##_xRGB8888
#define TAG2(x,y) radeon##x##_xRGB8888##y
#define GET_VALUE(_x, _y) ((*(GLuint*)(radeon_ptr_4byte(rrb, _x + x_off, _y + y_off)) | 0xff000000))
#define PUT_VALUE(_x, _y, d) { \
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte( rrb, _x + x_off, _y + y_off );		\
   *_ptr = d;								\
} while (0)
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    radeon##x##_ARGB8888
#define TAG2(x,y) radeon##x##_ARGB8888##y
#define GET_VALUE(_x, _y) (*(GLuint*)(radeon_ptr_4byte(rrb, _x + x_off, _y + y_off)))
#define PUT_VALUE(_x, _y, d) { \
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte( rrb, _x + x_off, _y + y_off );		\
   *_ptr = d;								\
} while (0)
#include "spantmp2.h"

/* 32 bit, BGRx8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8

#define TAG(x)    radeon##x##_BGRx8888
#define TAG2(x,y) radeon##x##_BGRx8888##y
#define GET_VALUE(_x, _y) ((*(GLuint*)(radeon_ptr_4byte(rrb, _x + x_off, _y + y_off)) | 0x000000ff))
#define PUT_VALUE(_x, _y, d) { \
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte( rrb, _x + x_off, _y + y_off );		\
   *_ptr = d;								\
} while (0)
#include "spantmp2.h"

/* 32 bit, BGRA8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8

#define TAG(x)    radeon##x##_BGRA8888
#define TAG2(x,y) radeon##x##_BGRA8888##y
#define GET_PTR(X,Y) radeon_ptr_4byte(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

/* ================================================================
 * Depth buffer
 */

/* The Radeon family has depth tiling on all the time, so we have to convert
 * the x,y coordinates into the memory bus address (mba) in the same
 * manner as the engine.  In each case, the linear block address (ba)
 * is calculated, and then wired with x and y to produce the final
 * memory address.
 * The chip will do address translation on its own if the surface registers
 * are set up correctly. It is not quite enough to get it working with hyperz
 * too...
 */

/* 16-bit depth buffer functions
 */
#define VALUE_TYPE GLushort

#if defined(RADEON_R200)
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)r200_depth_2byte(rrb, _x + x_off, _y + y_off) = d
#else
#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)radeon_ptr_2byte_8x2(rrb, _x + x_off, _y + y_off) = d
#endif

#if defined(RADEON_R200)
#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)r200_depth_2byte(rrb, _x + x_off, _y + y_off)
#else
#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)radeon_ptr_2byte_8x2(rrb, _x + x_off, _y + y_off)
#endif

#define TAG(x) radeon##x##_z16
#include "depthtmp.h"

/* 24 bit depth
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define VALUE_TYPE GLuint

#if defined(RADEON_R200)
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)r200_depth_4byte( rrb, _x + x_off, _y + y_off );		\
   GLuint tmp = LE32_TO_CPU(*_ptr);                                     \
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *_ptr = CPU_TO_LE32(tmp);                                            \
} while (0)
#else
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte( rrb, _x + x_off, _y + y_off );	\
   GLuint tmp = LE32_TO_CPU(*_ptr);                                     \
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *_ptr = CPU_TO_LE32(tmp);                                            \
} while (0)
#endif

#if defined(RADEON_R200)
#define READ_DEPTH( d, _x, _y )						\
  do {									\
    d = LE32_TO_CPU(*(GLuint*)(r200_depth_4byte(rrb, _x + x_off, _y + y_off))) & 0x00ffffff; \
  }while(0)
#else
#define READ_DEPTH( d, _x, _y )	\
  d = LE32_TO_CPU(*(GLuint*)(radeon_ptr_4byte(rrb, _x + x_off,	_y + y_off))) & 0x00ffffff;
#endif

#define TAG(x) radeon##x##_z24
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 * EXT_depth_stencil
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define VALUE_TYPE GLuint

#if defined(RADEON_R200)
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)r200_depth_4byte( rrb, _x + x_off, _y + y_off );		\
   *_ptr = CPU_TO_LE32(d);						\
} while (0)
#else
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte( rrb, _x + x_off, _y + y_off );	\
   *_ptr = CPU_TO_LE32(d);						\
} while (0)
#endif

#if defined(RADEON_R200)
#define READ_DEPTH( d, _x, _y )						\
  do { \
    d = LE32_TO_CPU(*(GLuint*)(r200_depth_4byte(rrb, _x + x_off, _y + y_off))); \
  }while(0)
#else
#define READ_DEPTH( d, _x, _y )	do {					\
    d = LE32_TO_CPU(*(GLuint*)(radeon_ptr_4byte(rrb, _x + x_off, _y + y_off))); \
  } while (0)
#endif

#define TAG(x) radeon##x##_s8_z24
#include "depthtmp.h"

/* ================================================================
 * Stencil buffer
 */

/* 24 bit depth, 8 bit stencil depthbuffer functions
 */
#if defined(RADEON_R200)
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)r200_depth_4byte(rrb, _x + x_off, _y + y_off);		\
   GLuint tmp = LE32_TO_CPU(*_ptr);                                     \
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *_ptr = CPU_TO_LE32(tmp);                                            \
} while (0)
#else
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte(rrb, _x + x_off, _y + y_off);		\
   GLuint tmp = LE32_TO_CPU(*_ptr);                                     \
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *_ptr = CPU_TO_LE32(tmp);                                            \
} while (0)
#endif

#if defined(RADEON_R200)
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint *_ptr = (GLuint*)r200_depth_4byte( rrb, _x + x_off, _y + y_off );		\
   GLuint tmp = LE32_TO_CPU(*_ptr);                                     \
   d = (tmp & 0xff000000) >> 24;					\
} while (0)
#else
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr_4byte( rrb, _x + x_off, _y + y_off );		\
   GLuint tmp = LE32_TO_CPU(*_ptr);                                     \
   d = (tmp & 0xff000000) >> 24;					\
} while (0)
#endif

#define TAG(x) radeon##x##_s8_z24
#include "stenciltmp.h"


static void map_unmap_rb(struct gl_renderbuffer *rb, int flag)
{
	struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
	int r;

	if (rrb == NULL || !rrb->bo)
		return;

	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s( rb %p, flag %s )\n",
		__func__, rb, flag ? "true":"false");

	if (flag) {
	        radeon_bo_wait(rrb->bo);
		r = radeon_bo_map(rrb->bo, 1);
		if (r) {
			fprintf(stderr, "(%s) error(%d) mapping buffer.\n",
				__FUNCTION__, r);
		}

		radeonSetSpanFunctions(rrb);
	} else {
		radeon_bo_unmap(rrb->bo);
		rb->GetRow = NULL;
		rb->PutRow = NULL;
	}
}

static void
radeon_map_unmap_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
			     GLboolean map)
{
	GLuint i, j;

	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s( %p , fb %p, map %s )\n",
		__func__, ctx, fb, map ? "true":"false");

	/* color draw buffers */
	for (j = 0; j < ctx->DrawBuffer->_NumColorDrawBuffers; j++)
		map_unmap_rb(fb->_ColorDrawBuffers[j], map);

	map_unmap_rb(fb->_ColorReadBuffer, map);

	/* check for render to textures */
	for (i = 0; i < BUFFER_COUNT; i++) {
		struct gl_renderbuffer_attachment *att =
			fb->Attachment + i;
		struct gl_texture_object *tex = att->Texture;
		if (tex) {
			/* Render to texture. Note that a mipmapped texture need not
			 * be complete for render to texture, so we must restrict to
			 * mapping only the attached image.
			 */
			radeon_texture_image *image = get_radeon_texture_image(tex->Image[att->CubeMapFace][att->TextureLevel]);
			ASSERT(att->Renderbuffer);

			if (map)
				radeon_teximage_map(image, GL_TRUE);
			else
				radeon_teximage_unmap(image);
		}
	}
	
	/* depth buffer (Note wrapper!) */
	if (fb->_DepthBuffer)
		map_unmap_rb(fb->_DepthBuffer->Wrapped, map);

	if (fb->_StencilBuffer)
		map_unmap_rb(fb->_StencilBuffer->Wrapped, map);

	radeon_check_front_buffer_rendering(ctx);
}

static void radeonSpanRenderStart(struct gl_context * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;

	radeon_firevertices(rmesa);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			ctx->Driver.MapTexture(ctx, ctx->Texture.Unit[i]._Current);
	}

	radeon_map_unmap_framebuffer(ctx, ctx->DrawBuffer, GL_TRUE);
	if (ctx->ReadBuffer != ctx->DrawBuffer)
		radeon_map_unmap_framebuffer(ctx, ctx->ReadBuffer, GL_TRUE);
}

static void radeonSpanRenderFinish(struct gl_context * ctx)
{
	int i;

	_swrast_flush(ctx);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			ctx->Driver.UnmapTexture(ctx, ctx->Texture.Unit[i]._Current);
	}

	radeon_map_unmap_framebuffer(ctx, ctx->DrawBuffer, GL_FALSE);
	if (ctx->ReadBuffer != ctx->DrawBuffer)
		radeon_map_unmap_framebuffer(ctx, ctx->ReadBuffer, GL_FALSE);
}

void radeonInitSpanFuncs(struct gl_context * ctx)
{
	struct swrast_device_driver *swdd =
	    _swrast_GetDeviceDriverReference(ctx);
	swdd->SpanRenderStart = radeonSpanRenderStart;
	swdd->SpanRenderFinish = radeonSpanRenderFinish;
}

/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
static void radeonSetSpanFunctions(struct radeon_renderbuffer *rrb)
{
	if (rrb->base.Format == MESA_FORMAT_RGB565) {
		radeonInitPointers_RGB565(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_RGB565_REV) {
		radeonInitPointers_RGB565_REV(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_XRGB8888) {
		radeonInitPointers_xRGB8888(&rrb->base);
        } else if (rrb->base.Format == MESA_FORMAT_XRGB8888_REV) {
		radeonInitPointers_BGRx8888(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_ARGB8888) {
		radeonInitPointers_ARGB8888(&rrb->base);
        } else if (rrb->base.Format == MESA_FORMAT_ARGB8888_REV) {
		radeonInitPointers_BGRA8888(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_ARGB4444) {
		radeonInitPointers_ARGB4444(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_ARGB4444_REV) {
		radeonInitPointers_ARGB4444_REV(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_ARGB1555) {
		radeonInitPointers_ARGB1555(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_ARGB1555_REV) {
		radeonInitPointers_ARGB1555_REV(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_Z16) {
		radeonInitDepthPointers_z16(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_X8_Z24) {
		radeonInitDepthPointers_z24(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_S8_Z24) {
		radeonInitDepthPointers_s8_z24(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_S8) {
		radeonInitStencilPointers_s8_z24(&rrb->base);
	} else {
		fprintf(stderr, "radeonSetSpanFunctions: bad format: 0x%04X\n", rrb->base.Format);
	}
}
