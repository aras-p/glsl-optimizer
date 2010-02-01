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
#include "nv20_driver.h"

GLcontext *
nv20_context_create(struct nouveau_screen *screen, const GLvisual *visual,
		    GLcontext *share_ctx)
{
	struct nouveau_context *nctx;
	GLcontext *ctx;

	nctx = CALLOC_STRUCT(nouveau_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base;
	nouveau_context_init(ctx, screen, visual, share_ctx);

	ctx->Const.MaxTextureCoordUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV20_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 8;
	ctx->Const.MaxTextureLodBias = 15;

	nv20_render_init(ctx);

	return ctx;
}

void
nv20_context_destroy(GLcontext *ctx)
{
	nv20_render_destroy(ctx);
	FREE(ctx);
}
