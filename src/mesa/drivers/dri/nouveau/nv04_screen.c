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
#include "nouveau_screen.h"
#include "nouveau_class.h"
#include "nv04_driver.h"

static const struct nouveau_driver nv04_driver;

static void
nv04_hwctx_init(struct nouveau_screen *screen)
{
	struct nouveau_channel *chan = screen->chan;
	struct nouveau_grobj *surf3d = screen->surf3d;
	struct nouveau_grobj *eng3d = screen->eng3d;
	struct nouveau_grobj *eng3dm = screen->eng3dm;

	BIND_RING(chan, surf3d, 7);
	BEGIN_RING(chan, surf3d, NV04_CONTEXT_SURFACES_3D_DMA_NOTIFY, 3);
	OUT_RING(chan, screen->ntfy->handle);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->vram->handle);

	BEGIN_RING(chan, eng3d, NV04_TEXTURED_TRIANGLE_DMA_NOTIFY, 4);
	OUT_RING(chan, screen->ntfy->handle);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	OUT_RING(chan, surf3d->handle);

	BEGIN_RING(chan, eng3dm, NV04_MULTITEX_TRIANGLE_DMA_NOTIFY, 4);
	OUT_RING(chan, screen->ntfy->handle);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	OUT_RING(chan, surf3d->handle);

	FIRE_RING(chan);
}

static void
nv04_channel_flush_notify(struct nouveau_channel *chan)
{
	struct nouveau_screen *screen = chan->user_private;
	struct nouveau_context *nctx = screen->context;

	if (nctx && nctx->fallback < SWRAST) {
		GLcontext *ctx = &nctx->base;

		/* Flushing seems to clobber the engine context. */
		context_dirty_i(ctx, TEX_OBJ, 0);
		context_dirty_i(ctx, TEX_OBJ, 1);
		context_dirty_i(ctx, TEX_ENV, 0);
		context_dirty_i(ctx, TEX_ENV, 1);
		context_dirty(ctx, CONTROL);
		context_dirty(ctx, BLEND);

		nouveau_state_emit(ctx);
	}
}

GLboolean
nv04_screen_init(struct nouveau_screen *screen)
{
	int ret;

	screen->driver = &nv04_driver;
	screen->chan->flush_notify = nv04_channel_flush_notify;

	/* 2D engine. */
	ret = nv04_surface_init(screen);
	if (!ret)
		return GL_FALSE;

	/* 3D engine. */
	ret = nouveau_grobj_alloc(screen->chan, 0xbeef0001,
				  NV04_TEXTURED_TRIANGLE, &screen->eng3d);
	if (ret)
		return GL_FALSE;

	ret = nouveau_grobj_alloc(screen->chan, 0xbeef0002,
				  NV04_MULTITEX_TRIANGLE, &screen->eng3dm);
	if (ret)
		return GL_FALSE;

	ret = nouveau_grobj_alloc(screen->chan, 0xbeef0003,
				  NV04_CONTEXT_SURFACES_3D, &screen->surf3d);
	if (ret)
		return GL_FALSE;

	nv04_hwctx_init(screen);

	return GL_TRUE;
}

static void
nv04_screen_destroy(struct nouveau_screen *screen)
{
	if (screen->eng3d)
		nouveau_grobj_free(&screen->eng3d);

	if (screen->eng3dm)
		nouveau_grobj_free(&screen->eng3dm);

	if (screen->surf3d)
		nouveau_grobj_free(&screen->surf3d);

	nv04_surface_takedown(screen);
}

static const struct nouveau_driver nv04_driver = {
	.screen_destroy = nv04_screen_destroy,
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
