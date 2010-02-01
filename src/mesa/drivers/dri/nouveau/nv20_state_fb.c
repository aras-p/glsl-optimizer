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
#include "nouveau_context.h"
#include "nouveau_fbo.h"
#include "nouveau_gldefs.h"
#include "nouveau_util.h"
#include "nouveau_class.h"
#include "nv20_driver.h"

static inline unsigned
get_rt_format(gl_format format)
{
	switch (format) {
	case MESA_FORMAT_XRGB8888:
		return 0x05;
	case MESA_FORMAT_ARGB8888:
		return 0x08;
	case MESA_FORMAT_RGB565:
		return 0x03;
	case MESA_FORMAT_Z16:
		return 0x10;
	case MESA_FORMAT_Z24_S8:
		return 0x20;
	default:
		assert(0);
	}
}

void
nv20_emit_framebuffer(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct nouveau_bo_context *bctx = context_bctx(ctx, FRAMEBUFFER);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	struct nouveau_surface *s;
	unsigned rt_format = NV20TCL_RT_FORMAT_TYPE_LINEAR;
	unsigned rt_pitch = 0, zeta_pitch = 0;
	unsigned bo_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR;

	if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT)
		return;

	/* Render target */
	if (fb->_NumColorDrawBuffers) {
		s = &to_nouveau_renderbuffer(
			fb->_ColorDrawBuffers[0])->surface;

		rt_format |= get_rt_format(s->format);
		zeta_pitch = rt_pitch = s->pitch;

		nouveau_bo_markl(bctx, kelvin, NV20TCL_COLOR_OFFSET,
				 s->bo, 0, bo_flags);
	}

	/* depth/stencil */
	if (fb->_DepthBuffer) {
		s = &to_nouveau_renderbuffer(
			fb->_DepthBuffer->Wrapped)->surface;

		rt_format |= get_rt_format(s->format);
		zeta_pitch = s->pitch;

		nouveau_bo_markl(bctx, kelvin, NV20TCL_ZETA_OFFSET,
				 s->bo, 0, bo_flags);
	}

	BEGIN_RING(chan, kelvin, NV20TCL_RT_FORMAT, 2);
	OUT_RING(chan, rt_format);
	OUT_RING(chan, zeta_pitch << 16 | rt_pitch);

	/* Recompute the viewport/scissor state. */
	context_dirty(ctx, VIEWPORT);
	context_dirty(ctx, SCISSOR);
}

void
nv20_emit_viewport(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	struct gl_framebuffer *fb = ctx->DrawBuffer;
	float a[4] = {};
	int i;

	get_viewport_translate(ctx, a);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_TRANSLATE_X, 4);
	for (i = 0; i < 4; i++)
		OUT_RINGf(chan, a[i]);

	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_HORIZ(0), 1);
	OUT_RING(chan, (fb->Width - 1) << 16);
	BEGIN_RING(chan, kelvin, NV20TCL_VIEWPORT_CLIP_VERT(0), 1);
	OUT_RING(chan, (fb->Height - 1) << 16);

	context_dirty(ctx, PROJECTION);
}
