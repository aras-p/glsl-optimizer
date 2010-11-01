/*
 * Copyright (C) 2009-2010 Francisco Jerez.
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
#include "nv04_3d.xml.h"
#include "nv04_driver.h"

static GLboolean
texunit_needs_combiners(struct gl_texture_unit *u)
{
	struct gl_texture_object *t = u->_Current;
	struct gl_texture_image *ti = t->Image[0][t->BaseLevel];

	return ti->TexFormat == MESA_FORMAT_A8 ||
		ti->TexFormat == MESA_FORMAT_L8 ||
		u->EnvMode == GL_COMBINE ||
		u->EnvMode == GL_COMBINE4_NV ||
		u->EnvMode == GL_BLEND ||
		u->EnvMode == GL_ADD;
}

struct nouveau_grobj *
nv04_context_engine(struct gl_context *ctx)
{
	struct nv04_context *nctx = to_nv04_context(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_grobj *fahrenheit;

	if ((ctx->Texture.Unit[0]._ReallyEnabled &&
	     texunit_needs_combiners(&ctx->Texture.Unit[0])) ||
	    ctx->Texture.Unit[1]._ReallyEnabled ||
	    ctx->Stencil.Enabled ||
	    !(ctx->Color.ColorMask[0][RCOMP] &&
	      ctx->Color.ColorMask[0][GCOMP] &&
	      ctx->Color.ColorMask[0][BCOMP] &&
	      ctx->Color.ColorMask[0][ACOMP]))
		fahrenheit = hw->eng3dm;
	else
		fahrenheit = hw->eng3d;

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
nv04_channel_flush_notify(struct nouveau_channel *chan)
{
	struct nouveau_context *nctx = chan->user_private;
	struct gl_context *ctx = &nctx->base;

	if (nctx->fallback < SWRAST) {
		nouveau_bo_state_emit(ctx);

		/* Reemit the engine state. */
		context_emit(ctx, TEX_OBJ0);
		context_emit(ctx, TEX_OBJ1);
		context_emit(ctx, TEX_ENV0);
		context_emit(ctx, TEX_ENV1);
		context_emit(ctx, CONTROL);
		context_emit(ctx, BLEND);
	}
}

static void
nv04_hwctx_init(struct gl_context *ctx)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_hw_state *hw = &to_nouveau_context(ctx)->hw;
	struct nouveau_grobj *surf3d = hw->surf3d;
	struct nouveau_grobj *eng3d = hw->eng3d;
	struct nouveau_grobj *eng3dm = hw->eng3dm;

	BIND_RING(chan, surf3d, 7);
	BEGIN_RING(chan, surf3d, NV04_CONTEXT_SURFACES_3D_DMA_NOTIFY, 3);
	OUT_RING(chan, hw->ntfy->handle);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->vram->handle);

	BEGIN_RING(chan, eng3d, NV04_TEXTURED_TRIANGLE_DMA_NOTIFY, 4);
	OUT_RING(chan, hw->ntfy->handle);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	OUT_RING(chan, surf3d->handle);

	BEGIN_RING(chan, eng3dm, NV04_MULTITEX_TRIANGLE_DMA_NOTIFY, 4);
	OUT_RING(chan, hw->ntfy->handle);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	OUT_RING(chan, surf3d->handle);

	FIRE_RING(chan);
}

static void
init_dummy_texture(struct gl_context *ctx)
{
	struct nouveau_surface *s = &to_nv04_context(ctx)->dummy_texture;

	nouveau_surface_alloc(ctx, s, SWIZZLED,
			      NOUVEAU_BO_MAP | NOUVEAU_BO_VRAM,
			      MESA_FORMAT_ARGB8888, 1, 1);

	nouveau_bo_map(s->bo, NOUVEAU_BO_WR);
	*(uint32_t *)s->bo->map = 0xffffffff;
	nouveau_bo_unmap(s->bo);
}

static void
nv04_context_destroy(struct gl_context *ctx)
{
	struct nouveau_context *nctx = to_nouveau_context(ctx);

	nv04_surface_takedown(ctx);
	nv04_render_destroy(ctx);
	nouveau_surface_ref(NULL, &to_nv04_context(ctx)->dummy_texture);

	nouveau_grobj_free(&nctx->hw.eng3d);
	nouveau_grobj_free(&nctx->hw.eng3dm);
	nouveau_grobj_free(&nctx->hw.surf3d);

	nouveau_context_deinit(ctx);
	FREE(ctx);
}

static struct gl_context *
nv04_context_create(struct nouveau_screen *screen, const struct gl_config *visual,
		    struct gl_context *share_ctx)
{
	struct nv04_context *nctx;
	struct nouveau_hw_state *hw;
	struct gl_context *ctx;
	int ret;

	nctx = CALLOC_STRUCT(nv04_context);
	if (!nctx)
		return NULL;

	ctx = &nctx->base.base;
	hw = &nctx->base.hw;

	if (!nouveau_context_init(ctx, screen, visual, share_ctx))
		goto fail;

	hw->chan->flush_notify = nv04_channel_flush_notify;

	/* GL constants. */
	ctx->Const.MaxTextureLevels = 11;
	ctx->Const.MaxTextureCoordUnits = NV04_TEXTURE_UNITS;
	ctx->Const.MaxTextureImageUnits = NV04_TEXTURE_UNITS;
	ctx->Const.MaxTextureUnits = NV04_TEXTURE_UNITS;
	ctx->Const.MaxTextureMaxAnisotropy = 2;
	ctx->Const.MaxTextureLodBias = 15;

	/* 2D engine. */
	ret = nv04_surface_init(ctx);
	if (!ret)
		goto fail;

	/* 3D engine. */
	ret = nouveau_grobj_alloc(context_chan(ctx), 0xbeef0001,
				  NV04_TEXTURED_TRIANGLE, &hw->eng3d);
	if (ret)
		goto fail;

	ret = nouveau_grobj_alloc(context_chan(ctx), 0xbeef0002,
				  NV04_MULTITEX_TRIANGLE, &hw->eng3dm);
	if (ret)
		goto fail;

	ret = nouveau_grobj_alloc(context_chan(ctx), 0xbeef0003,
				  NV04_CONTEXT_SURFACES_3D, &hw->surf3d);
	if (ret)
		goto fail;

	init_dummy_texture(ctx);
	nv04_hwctx_init(ctx);
	nv04_render_init(ctx);

	return ctx;

fail:
	nv04_context_destroy(ctx);
	return NULL;
}

const struct nouveau_driver nv04_driver = {
	.context_create = nv04_context_create,
	.context_destroy = nv04_context_destroy,
	.surface_copy = nv04_surface_copy,
	.surface_fill = nv04_surface_fill,
	.emit = (nouveau_state_func[]) {
		nv04_defer_control,
		nouveau_emit_nothing,
		nv04_defer_blend,
		nv04_defer_blend,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv04_defer_control,
		nouveau_emit_nothing,
		nv04_defer_control,
		nouveau_emit_nothing,
		nv04_defer_control,
		nv04_defer_control,
		nouveau_emit_nothing,
		nv04_emit_framebuffer,
		nv04_defer_blend,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv04_emit_scissor,
		nv04_defer_blend,
		nv04_defer_control,
		nv04_defer_control,
		nv04_defer_control,
		nv04_emit_tex_env,
		nv04_emit_tex_env,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv04_emit_tex_obj,
		nv04_emit_tex_obj,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nouveau_emit_nothing,
		nv04_emit_blend,
		nv04_emit_control,
	},
	.num_emit = NUM_NV04_STATE,
};
