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
#include "main/renderbuffer.h"
#include "swrast/swrast.h"
#include "swrast/s_renderbuffer.h"

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

/*
 * Note that all information needed to access pixels in a renderbuffer
 * should be obtained through the gl_renderbuffer parameter, not per-context
 * information.
 */
#define LOCAL_VARS						\
   struct radeon_renderbuffer *rrb = (void *) rb;		\
   int minx = 0, miny = 0;						\
   int maxx = rb->Width;						\
   int maxy = rb->Height;						\
   void *buf = rb->Map;						\
   int pitch = rb->RowStrideBytes;				\
   GLuint p;						\
   (void)p;

#define Y_FLIP(_y) (_y)

#define HW_LOCK()
#define HW_UNLOCK()
#define HW_CLIPLOOP()
#define HW_ENDCLIPLOOP()

/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5
#define TAG(x)    radeon##x##_RGB565
#define TAG2(x,y) radeon##x##_RGB565##y
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5_REV
#define TAG(x)    radeon##x##_RGB565_REV
#define TAG2(x,y) radeon##x##_RGB565_REV##y
#include "spantmp2.h"

/* 16 bit, ARGB1555 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_1_5_5_5_REV
#define TAG(x)    radeon##x##_ARGB1555
#define TAG2(x,y) radeon##x##_ARGB1555##y
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_1_5_5_5
#define TAG(x)    radeon##x##_ARGB1555_REV
#define TAG2(x,y) radeon##x##_ARGB1555_REV##y
#include "spantmp2.h"

/* 16 bit, RGBA4 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_4_4_4_4_REV
#define TAG(x)    radeon##x##_ARGB4444
#define TAG2(x,y) radeon##x##_ARGB4444##y
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_4_4_4_4
#define TAG(x)    radeon##x##_ARGB4444_REV
#define TAG2(x,y) radeon##x##_ARGB4444_REV##y
#include "spantmp2.h"

/* 32 bit, xRGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TAG(x)    radeon##x##_xRGB8888
#define TAG2(x,y) radeon##x##_xRGB8888##y
#include "spantmp2.h"

/* 32 bit, ARGB8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TAG(x)    radeon##x##_ARGB8888
#define TAG2(x,y) radeon##x##_ARGB8888##y
#include "spantmp2.h"

/* 32 bit, BGRx8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8
#define TAG(x)    radeon##x##_BGRx8888
#define TAG2(x,y) radeon##x##_BGRx8888##y
#include "spantmp2.h"

/* 32 bit, BGRA8888 color spanline and pixel functions
 */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8
#define TAG(x)    radeon##x##_BGRA8888
#define TAG2(x,y) radeon##x##_BGRA8888##y
#include "spantmp2.h"

static void
radeon_renderbuffer_map(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
	GLubyte *map;
	int stride;

	if (!rb || !rrb)
		return;

	ctx->Driver.MapRenderbuffer(ctx, rb, 0, 0, rb->Width, rb->Height,
				    GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
				    &map, &stride);

	rb->Map = map;
	rb->RowStride = stride / _mesa_get_format_bytes(rb->Format);
	rb->RowStrideBytes = stride;

	radeonSetSpanFunctions(rrb);
}

static void
radeon_renderbuffer_unmap(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
	struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
	if (!rb || !rrb)
		return;

	ctx->Driver.UnmapRenderbuffer(ctx, rb);

	rb->GetRow = NULL;
	rb->PutRow = NULL;
	rb->Map = NULL;
	rb->RowStride = 0;
	rb->RowStrideBytes = 0;
}

static void
radeon_map_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
	GLuint i;

	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s( %p , fb %p )\n",
		     __func__, ctx, fb);

	/* check for render to textures */
	for (i = 0; i < BUFFER_COUNT; i++)
		radeon_renderbuffer_map(ctx, fb->Attachment[i].Renderbuffer);

	radeon_check_front_buffer_rendering(ctx);
}

static void
radeon_unmap_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
	GLuint i;

	radeon_print(RADEON_MEMORY, RADEON_TRACE,
		"%s( %p , fb %p)\n",
		     __func__, ctx, fb);

	/* check for render to textures */
	for (i = 0; i < BUFFER_COUNT; i++)
		radeon_renderbuffer_unmap(ctx, fb->Attachment[i].Renderbuffer);

	radeon_check_front_buffer_rendering(ctx);
}

static void radeonSpanRenderStart(struct gl_context * ctx)
{
	radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
	int i;

	radeon_firevertices(rmesa);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
		if (ctx->Texture.Unit[i]._ReallyEnabled) {
			radeon_validate_texture_miptree(ctx, ctx->Texture.Unit[i]._Current);
			radeon_swrast_map_texture_images(ctx, ctx->Texture.Unit[i]._Current);
		}
	}
	
	radeon_map_framebuffer(ctx, ctx->DrawBuffer);
	if (ctx->ReadBuffer != ctx->DrawBuffer)
		radeon_map_framebuffer(ctx, ctx->ReadBuffer);
}

static void radeonSpanRenderFinish(struct gl_context * ctx)
{
	int i;

	_swrast_flush(ctx);

	for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++)
		if (ctx->Texture.Unit[i]._ReallyEnabled)
			radeon_swrast_unmap_texture_images(ctx, ctx->Texture.Unit[i]._Current);

	radeon_unmap_framebuffer(ctx, ctx->DrawBuffer);
	if (ctx->ReadBuffer != ctx->DrawBuffer)
		radeon_unmap_framebuffer(ctx, ctx->ReadBuffer);
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
		_swrast_set_renderbuffer_accessors(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_X8_Z24) {
		_swrast_set_renderbuffer_accessors(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_S8_Z24) {
		_swrast_set_renderbuffer_accessors(&rrb->base);
	} else if (rrb->base.Format == MESA_FORMAT_S8) {
		_swrast_set_renderbuffer_accessors(&rrb->base);
	} else {
		fprintf(stderr, "radeonSetSpanFunctions: bad format: 0x%04X\n", rrb->base.Format);
	}
}
