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

#include "radeon_common.h"
#include "radeon_lock.h"
#include "radeon_span.h"

#define DBG 0

static void radeonSetSpanFunctions(struct radeon_renderbuffer *rrb);

static GLubyte *radeon_ptr32(const struct radeon_renderbuffer * rrb,
			     GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;
    GLint nmacroblkpl;
    GLint nmicroblkpl;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
                nmacroblkpl = rrb->pitch >> 5;
                offset += ((y >> 4) * nmacroblkpl) << 11;
                offset += ((y & 15) >> 1) << 8;
                offset += (y & 1) << 4;
                offset += (x >> 5) << 11;
                offset += ((x & 31) >> 2) << 5;
                offset += (x & 3) << 2;
            } else {
                nmacroblkpl = rrb->pitch >> 6;
                offset += ((y >> 3) * nmacroblkpl) << 11;
                offset += (y & 7) << 8;
                offset += (x >> 6) << 11;
                offset += ((x & 63) >> 3) << 5;
                offset += (x & 7) << 2;
            }
        } else {
            nmicroblkpl = ((rrb->pitch + 31) & ~31) >> 5;
            offset += (y * nmicroblkpl) << 5;
            offset += (x >> 3) << 5;
            offset += (x & 7) << 2;
        }
    }
    return &ptr[offset];
}

static GLubyte *radeon_ptr16(const struct radeon_renderbuffer * rrb,
			     GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;
    GLint nmacroblkpl;
    GLint nmicroblkpl;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
                nmacroblkpl = rrb->pitch >> 6;
                offset += ((y >> 4) * nmacroblkpl) << 11;
                offset += ((y & 15) >> 1) << 8;
                offset += (y & 1) << 4;
                offset += (x >> 6) << 11;
                offset += ((x & 63) >> 3) << 5;
                offset += (x & 7) << 1;
            } else {
                nmacroblkpl = rrb->pitch >> 7;
                offset += ((y >> 3) * nmacroblkpl) << 11;
                offset += (y & 7) << 8;
                offset += (x >> 7) << 11;
                offset += ((x & 127) >> 4) << 5;
                offset += (x & 15) << 2;
            }
        } else {
            nmicroblkpl = ((rrb->pitch + 31) & ~31) >> 5;
            offset += (y * nmicroblkpl) << 5;
            offset += (x >> 4) << 5;
            offset += (x & 15) << 2;
        }
    }
    return &ptr[offset];
}

static GLubyte *radeon_ptr(const struct radeon_renderbuffer * rrb,
			   GLint x, GLint y)
{
    GLubyte *ptr = rrb->bo->ptr;
    uint32_t mask = RADEON_BO_FLAGS_MACRO_TILE | RADEON_BO_FLAGS_MICRO_TILE;
    GLint offset;
    GLint microblkxs;
    GLint macroblkxs;
    GLint nmacroblkpl;
    GLint nmicroblkpl;

    if (rrb->has_surface || !(rrb->bo->flags & mask)) {
        offset = x * rrb->cpp + y * rrb->pitch;
    } else {
        offset = 0;
        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE) {
            if (rrb->bo->flags & RADEON_BO_FLAGS_MICRO_TILE) {
                microblkxs = 16 / rrb->cpp;
                macroblkxs = 128 / rrb->cpp;
                nmacroblkpl = rrb->pitch / macroblkxs;
                offset += ((y >> 4) * nmacroblkpl) << 11;
                offset += ((y & 15) >> 1) << 8;
                offset += (y & 1) << 4;
                offset += (x / macroblkxs) << 11;
                offset += ((x & (macroblkxs - 1)) / microblkxs) << 5;
                offset += (x & (microblkxs - 1)) * rrb->cpp;
            } else {
                microblkxs = 32 / rrb->cpp;
                macroblkxs = 256 / rrb->cpp;
                nmacroblkpl = rrb->pitch / macroblkxs;
                offset += ((y >> 3) * nmacroblkpl) << 11;
                offset += (y & 7) << 8;
                offset += (x / macroblkxs) << 11;
                offset += ((x & (macroblkxs - 1)) / microblkxs) << 5;
                offset += (x & (microblkxs - 1)) * rrb->cpp;
            }
        } else {
            microblkxs = 32 / rrb->cpp;
            nmicroblkpl = ((rrb->pitch + 31) & ~31) >> 5;
            offset += (y * nmicroblkpl) << 5;
            offset += (x / microblkxs) << 5;
            offset += (x & (microblkxs - 1)) * rrb->cpp;
        }
    }
    return &ptr[offset];
}

#ifndef COMPILE_R300
static uint32_t
z24s8_to_s8z24(uint32_t val)
{
   return (val << 24) | (val >> 8);
}

static uint32_t
s8z24_to_z24s8(uint32_t val)
{
   return (val >> 24) | (val << 8);
}
#endif

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
#define GET_PTR(X,Y) radeon_ptr16(rrb, (X) + x_off, (Y) + y_off)
#include "spantmp2.h"

/* 32 bit, xRGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    radeon##x##_xRGB8888
#define TAG2(x,y) radeon##x##_xRGB8888##y
#define GET_VALUE(_x, _y) ((*(GLuint*)(radeon_ptr32(rrb, _x + x_off, _y + y_off)) | 0xff000000))
#define PUT_VALUE(_x, _y, d) { \
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );		\
   *_ptr = d;								\
} while (0)
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV

#define TAG(x)    radeon##x##_ARGB8888
#define TAG2(x,y) radeon##x##_ARGB8888##y
#define GET_PTR(X,Y) radeon_ptr32(rrb, (X) + x_off, (Y) + y_off)
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

#define WRITE_DEPTH( _x, _y, d )					\
   *(GLushort *)radeon_ptr(rrb, _x + x_off, _y + y_off) = d

#define READ_DEPTH( d, _x, _y )						\
   d = *(GLushort *)radeon_ptr(rrb, _x + x_off, _y + y_off)

#define TAG(x) radeon##x##_z16
#include "depthtmp.h"

/* 24 bit depth
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define VALUE_TYPE GLuint

#ifdef COMPILE_R300
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );		\
   GLuint tmp = *_ptr;				\
   tmp &= 0x000000ff;							\
   tmp |= ((d << 8) & 0xffffff00);					\
   *_ptr = tmp;					\
} while (0)
#else
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );	\
   GLuint tmp = *_ptr;							\
   tmp &= 0xff000000;							\
   tmp |= ((d) & 0x00ffffff);						\
   *_ptr = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_DEPTH( d, _x, _y )						\
  do {									\
    d = (*(GLuint*)(radeon_ptr32(rrb, _x + x_off, _y + y_off)) & 0xffffff00) >> 8; \
  }while(0)
#else
#define READ_DEPTH( d, _x, _y )	\
  d = *(GLuint*)(radeon_ptr32(rrb, _x + x_off,	_y + y_off)) & 0x00ffffff;
#endif
/*
    fprintf(stderr, "dval(%d, %d, %d, %d)=0x%08X\n", _x, xo, _y, yo, d);\
   d = *(GLuint*)(radeon_ptr(rrb, _x,	_y )) & 0x00ffffff;
*/
#define TAG(x) radeon##x##_z24
#include "depthtmp.h"

/* 24 bit depth, 8 bit stencil depthbuffer functions
 * EXT_depth_stencil
 *
 * Careful: It looks like the R300 uses ZZZS byte order while the R200
 * uses SZZZ for 24 bit depth, 8 bit stencil mode.
 */
#define VALUE_TYPE GLuint

#ifdef COMPILE_R300
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );		\
   *_ptr = d;								\
} while (0)
#else
#define WRITE_DEPTH( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );	\
   GLuint tmp = z24s8_to_s8z24(d);					\
   *_ptr = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_DEPTH( d, _x, _y )						\
  do { \
    d = (*(GLuint*)(radeon_ptr32(rrb, _x + x_off, _y + y_off)));	\
  }while(0)
#else
#define READ_DEPTH( d, _x, _y )	do {					\
    d = s8z24_to_z24s8(*(GLuint*)(radeon_ptr32(rrb, _x + x_off,	_y + y_off ))); \
  } while (0)
#endif
/*
    fprintf(stderr, "dval(%d, %d, %d, %d)=0x%08X\n", _x, xo, _y, yo, d);\
   d = *(GLuint*)(radeon_ptr(rrb, _x,	_y )) & 0x00ffffff;
*/
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
   GLuint *_ptr = (GLuint*)radeon_ptr32(rrb, _x + x_off, _y + y_off);		\
   GLuint tmp = *_ptr;				\
   tmp &= 0xffffff00;							\
   tmp |= (d) & 0xff;							\
   *_ptr = tmp;					\
} while (0)
#else
#define WRITE_STENCIL( _x, _y, d )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32(rrb, _x + x_off, _y + y_off);		\
   GLuint tmp = *_ptr;				\
   tmp &= 0x00ffffff;							\
   tmp |= (((d) & 0xff) << 24);						\
   *_ptr = tmp;					\
} while (0)
#endif

#ifdef COMPILE_R300
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );		\
   GLuint tmp = *_ptr;				\
   d = tmp & 0x000000ff;						\
} while (0)
#else
#define READ_STENCIL( d, _x, _y )					\
do {									\
   GLuint *_ptr = (GLuint*)radeon_ptr32( rrb, _x + x_off, _y + y_off );		\
   GLuint tmp = *_ptr;				\
   d = (tmp & 0xff000000) >> 24;					\
} while (0)
#endif

#define TAG(x) radeon##x##_z24_s8
#include "stenciltmp.h"


static void map_unmap_rb(struct gl_renderbuffer *rb, int flag)
{
	struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
	int r;
	
	if (rrb == NULL || !rrb->bo)
		return;

	if (flag) {
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
radeon_map_unmap_buffers(GLcontext *ctx, GLboolean map)
{
	GLuint i, j;

	/* color draw buffers */
	for (j = 0; j < ctx->DrawBuffer->_NumColorDrawBuffers; j++)
		map_unmap_rb(ctx->DrawBuffer->_ColorDrawBuffers[j], map);

	/* check for render to textures */
	for (i = 0; i < BUFFER_COUNT; i++) {
		struct gl_renderbuffer_attachment *att =
			ctx->DrawBuffer->Attachment + i;
		struct gl_texture_object *tex = att->Texture;
		if (tex) {
			/* render to texture */
			ASSERT(att->Renderbuffer);
			if (map)
				ctx->Driver.MapTexture(ctx, tex);
			else
				ctx->Driver.UnmapTexture(ctx, tex);
		}
	}
	
	map_unmap_rb(ctx->ReadBuffer->_ColorReadBuffer, map);

	/* depth buffer (Note wrapper!) */
	if (ctx->DrawBuffer->_DepthBuffer)
		map_unmap_rb(ctx->DrawBuffer->_DepthBuffer->Wrapped, map);
	
	if (ctx->DrawBuffer->_StencilBuffer)
		map_unmap_rb(ctx->DrawBuffer->_StencilBuffer->Wrapped, map);

}
static void radeonSpanRenderStart(GLcontext * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;

	radeon_firevertices(rmesa);

	/* The locking and wait for idle should really only be needed in classic mode.
	 * In a future memory manager based implementation, this should become
	 * unnecessary due to the fact that mapping our buffers, textures, etc.
	 * should implicitly wait for any previous rendering commands that must
	 * be waited on. */
	if (!rmesa->radeonScreen->driScreen->dri2.enabled) {
		LOCK_HARDWARE(rmesa);
		radeonWaitForIdleLocked(rmesa);
	}
	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			ctx->Driver.MapTexture(ctx, ctx->Texture.Unit[i]._Current);
	}

	radeon_map_unmap_buffers(ctx, 1);



}

static void radeonSpanRenderFinish(GLcontext * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;
	_swrast_flush(ctx);
	if (!rmesa->radeonScreen->driScreen->dri2.enabled) {
		UNLOCK_HARDWARE(rmesa);
	}
	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			ctx->Driver.UnmapTexture(ctx, ctx->Texture.Unit[i]._Current);
	}

	radeon_map_unmap_buffers(ctx, 0);
}

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
static void radeonSetSpanFunctions(struct radeon_renderbuffer *rrb)
{
	if (rrb->base._ActualFormat == GL_RGB5) {
		radeonInitPointers_RGB565(&rrb->base);
	} else if (rrb->base._ActualFormat == GL_RGB8) {
		radeonInitPointers_xRGB8888(&rrb->base);
	} else if (rrb->base._ActualFormat == GL_RGBA8) {
		radeonInitPointers_ARGB8888(&rrb->base);
	} else if (rrb->base._ActualFormat == GL_DEPTH_COMPONENT16) {
		radeonInitDepthPointers_z16(&rrb->base);
	} else if (rrb->base._ActualFormat == GL_DEPTH_COMPONENT24) {
		radeonInitDepthPointers_z24(&rrb->base);
	} else if (rrb->base._ActualFormat == GL_DEPTH24_STENCIL8_EXT) {
		radeonInitDepthPointers_z24_s8(&rrb->base);
	} else if (rrb->base._ActualFormat == GL_STENCIL_INDEX8_EXT) {
		radeonInitStencilPointers_z24_s8(&rrb->base);
	}
}
