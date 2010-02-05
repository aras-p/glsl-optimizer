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
#include "nv04_driver.h"

struct nouveau_grobj *
nv04_context_engine(GLcontext *ctx)
{
	struct nv04_context *nctx = to_nv04_context(ctx);
	struct nouveau_screen *screen = nctx->base.screen;
	struct nouveau_grobj *fahrenheit;

	if (ctx->Texture.Unit[0].EnvMode == GL_COMBINE ||
	    ctx->Texture.Unit[0].EnvMode == GL_BLEND ||
	    ctx->Texture.Unit[1]._ReallyEnabled ||
	    ctx->Stencil.Enabled)
		fahrenheit = screen->eng3dm;
	else
		fahrenheit = screen->eng3d;

	if (fahrenheit != nctx->eng3d) {
		nctx->eng3d = fahrenheit;

		if (nv04_mtex_engine(fahrenheit)) {
			context_dirty_i(ctx, TEX_ENV, 0);
			context_dirty_i(ctx, TEX_ENV, 1);
			context_dirty_i(ctx, TEX_OBJ, 0);
			context_dirty_i(ctx, TEX_OBJ, 1);
			context_dirty(ctx, CONTROL);
			context_dirty(ctx, BLEND);
		} else {
			context_bctx_i(ctx, TEXTURE, 1);
			context_dirty_i(ctx, TEX_ENV, 0);
			context_dirty_i(ctx, TEX_OBJ, 0);
			context_dirty(ctx, CONTROL);
			context_dirty(ctx, BLEND);
		}
	}

	return fahrenheit;
}

static void
init_dummy_texture(GLcontext *ctx)
{
	struct nouveau_surface *s = &to_nv04_context(ctx)->dummy_texture;

	nouveau_surface_alloc(ctx, s, SWIZZLED,
			      NOUVEAU_BO_MAP | NOUVEAU_BO_VRAM,
			      MESA_FORMAT_ARGB8888, 1, 1);

	nouveau_bo_map(s->bo, NOUVEAU_BO_WR);
	*(uint32_t *)s->bo->map = 0xffffffff;
	nouveau_bo_unmap(s->bo);
}

GLcontext *
nv04_context_create(struct nouveau_screen *screen, const GLvisual *visual,
		    GLcontext *share_ctx)
{
	struct nv04_context *nctx;
	GLcontext *ctx;

	nctx = CALLOC_STRUCT(nv04_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base.base;
	nouveau_context_init(ctx, screen, visual, share_ctx);

	ctx->Const.MaxTextureCoordUnits = NV04_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV04_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV04_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 2;
	ctx->Const.MaxTextureLodBias = 15;

	init_dummy_texture(ctx);
	nv04_render_init(ctx);

	return ctx;
}

void
nv04_context_destroy(GLcontext *ctx)
{
	nv04_render_destroy(ctx);
	nouveau_surface_ref(NULL, &to_nv04_context(ctx)->dummy_texture);

	FREE(ctx);
}
