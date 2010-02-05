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
#include "nouveau_util.h"
#include "nouveau_class.h"
#include "nv10_driver.h"

static void
nv10_clear(GLcontext *ctx, GLbitfield buffers)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *celsius = context_eng3d(ctx);
	struct nouveau_framebuffer *nfb = to_nouveau_framebuffer(
		ctx->DrawBuffer);

	nouveau_validate_framebuffer(ctx);

	/* Clear the LMA depth buffer, if present. */
	if ((buffers & BUFFER_BIT_DEPTH) && ctx->Depth.Mask &&
	    nfb->lma_bo) {
		struct nouveau_surface *s = &to_nouveau_renderbuffer(
			nfb->base._DepthBuffer->Wrapped)->surface;

		BEGIN_RING(chan, celsius, NV17TCL_LMA_DEPTH_FILL_VALUE, 1);
		OUT_RING(chan, pack_zs_f(s->format, ctx->Depth.Clear, 0));
		BEGIN_RING(chan, celsius, NV17TCL_LMA_DEPTH_BUFFER_CLEAR, 1);
		OUT_RING(chan, 1);
	}

	nouveau_clear(ctx, buffers);
}

GLcontext *
nv10_context_create(struct nouveau_screen *screen, const GLvisual *visual,
		    GLcontext *share_ctx)
{
	struct nouveau_context *nctx;
	GLcontext *ctx;

	nctx = CALLOC_STRUCT(nouveau_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base;
	nouveau_context_init(ctx, screen, visual, share_ctx);

	ctx->Const.MaxTextureLevels = 12;
	ctx->Const.MaxTextureCoordUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV10_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 2;
	ctx->Const.MaxTextureLodBias = 15;
	ctx->Driver.Clear = nv10_clear;

	nv10_render_init(ctx);

	return ctx;
}

void
nv10_context_destroy(GLcontext *ctx)
{
	nv10_render_destroy(ctx);
	FREE(ctx);
}
