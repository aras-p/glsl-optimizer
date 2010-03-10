/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_fbo.h"
#include "nouveau_context.h"
#include "nouveau_bo.h"

#include "swrast/swrast.h"

#define LOCAL_VARS							\
	struct gl_framebuffer *fb = ctx->DrawBuffer;			\
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface; \
	GLuint p;							\
	(void)p;

#define LOCAL_DEPTH_VARS LOCAL_VARS

#define HW_LOCK()
#define HW_UNLOCK()

#define HW_CLIPLOOP() {							\
	int minx = 0;							\
	int miny = 0;							\
	int maxx = fb->Width;						\
	int maxy = fb->Height;

#define HW_ENDCLIPLOOP() }

#define Y_FLIP(y) (fb->Name ? (y) : rb->Height - 1 - (y))

/* RGB565 span functions */
#define SPANTMP_PIXEL_FMT GL_RGB
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_SHORT_5_6_5
#define TAG(x) nouveau_##x##_rgb565
#define TAG2(x, y) nouveau_##x##_rgb565##y
#define GET_PTR(x, y) (s->bo->map + (y)*s->pitch + (x)*s->cpp)

#include "spantmp2.h"

/* RGB888 span functions */
#define SPANTMP_PIXEL_FMT GL_BGR
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TAG(x) nouveau_##x##_rgb888
#define TAG2(x, y) nouveau_##x##_rgb888##y
#define GET_PTR(x, y) (s->bo->map + (y)*s->pitch + (x)*s->cpp)

#include "spantmp2.h"

/* ARGB8888 span functions */
#define SPANTMP_PIXEL_FMT GL_BGRA
#define SPANTMP_PIXEL_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#define TAG(x) nouveau_##x##_argb8888
#define TAG2(x, y) nouveau_##x##_argb8888##y
#define GET_PTR(x, y) (s->bo->map + (y)*s->pitch + (x)*s->cpp)

#include "spantmp2.h"

/* Z16 span functions */
#define VALUE_TYPE uint16_t
#define READ_DEPTH(v, x, y)						\
	v = *(uint16_t *)(s->bo->map + (y)*s->pitch + (x)*s->cpp);
#define WRITE_DEPTH(x, y, v)						\
	*(uint16_t *)(s->bo->map + (y)*s->pitch + (x)*s->cpp) = v
#define TAG(x) nouveau_##x##_z16

#include "depthtmp.h"

/* Z24S8 span functions */
#define VALUE_TYPE uint32_t
#define READ_DEPTH(v, x, y)						\
	v = *(uint32_t *)(s->bo->map + (y)*s->pitch + (x)*s->cpp);
#define WRITE_DEPTH(x, y, v)						\
	*(uint32_t *)(s->bo->map + (y)*s->pitch + (x)*s->cpp) = v
#define TAG(x) nouveau_##x##_z24s8

#include "depthtmp.h"

static void
renderbuffer_map_unmap(struct gl_renderbuffer *rb, GLboolean map)
{
	struct nouveau_surface *s = &to_nouveau_renderbuffer(rb)->surface;

	if (map) {
		switch (rb->Format) {
		case MESA_FORMAT_RGB565:
			nouveau_InitPointers_rgb565(rb);
			break;
		case MESA_FORMAT_XRGB8888:
			nouveau_InitPointers_rgb888(rb);
			break;
		case MESA_FORMAT_ARGB8888:
			nouveau_InitPointers_argb8888(rb);
			break;
		case MESA_FORMAT_Z16:
			nouveau_InitDepthPointers_z16(rb);
			break;
		case MESA_FORMAT_Z24_S8:
			nouveau_InitDepthPointers_z24s8(rb);
			break;
		default:
			assert(0);
		}

		nouveau_bo_map(s->bo, NOUVEAU_BO_RDWR);
	} else {
		nouveau_bo_unmap(s->bo);
	}
}

static void
texture_unit_map_unmap(GLcontext *ctx, struct gl_texture_unit *u, GLboolean map)
{
	if (!u->_ReallyEnabled)
		return;

	if (map)
		ctx->Driver.MapTexture(ctx, u->_Current);
	else
		ctx->Driver.UnmapTexture(ctx, u->_Current);
}

static void
span_map_unmap(GLcontext *ctx, GLboolean map)
{
	int i;

	for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++)
		renderbuffer_map_unmap(ctx->DrawBuffer->_ColorDrawBuffers[i], map);

	renderbuffer_map_unmap(ctx->DrawBuffer->_ColorReadBuffer, map);

	if (ctx->DrawBuffer->_DepthBuffer)
		renderbuffer_map_unmap(ctx->DrawBuffer->_DepthBuffer->Wrapped, map);

	for (i = 0; i < ctx->Const.MaxTextureUnits; i++)
		texture_unit_map_unmap(ctx, &ctx->Texture.Unit[i], map);
}

static void
nouveau_span_start(GLcontext *ctx)
{
	nouveau_fallback(ctx, SWRAST);
	span_map_unmap(ctx, GL_TRUE);
}

static void
nouveau_span_finish(GLcontext *ctx)
{
	span_map_unmap(ctx, GL_FALSE);
	nouveau_fallback(ctx, HWTNL);
}

void
nouveau_span_functions_init(GLcontext *ctx)
{
	struct swrast_device_driver *swdd =
		_swrast_GetDeviceDriverReference(ctx);

	swdd->SpanRenderStart = nouveau_span_start;
	swdd->SpanRenderFinish = nouveau_span_finish;
}
