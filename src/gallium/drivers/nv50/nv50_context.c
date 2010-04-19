/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "draw/draw_context.h"
#include "pipe/p_defines.h"

#include "nv50_context.h"
#include "nv50_screen.h"

static void
nv50_flush(struct pipe_context *pipe, unsigned flags,
	   struct pipe_fence_handle **fence)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->base.channel;

	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(chan, nv50->screen->tesla, 0x1338, 1);
		OUT_RING  (chan, 0x20);
	}

	if (flags & PIPE_FLUSH_FRAME)
		FIRE_RING(chan);
}

static void
nv50_destroy(struct pipe_context *pipe)
{
	struct nv50_context *nv50 = nv50_context(pipe);

        if (nv50->state.fb)
		so_ref(NULL, &nv50->state.fb);
	if (nv50->state.blend)
		so_ref(NULL, &nv50->state.blend);
	if (nv50->state.blend_colour)
		so_ref(NULL, &nv50->state.blend_colour);
	if (nv50->state.zsa)
		so_ref(NULL, &nv50->state.zsa);
	if (nv50->state.rast)
		so_ref(NULL, &nv50->state.rast);
	if (nv50->state.stipple)
		so_ref(NULL, &nv50->state.stipple);
	if (nv50->state.scissor)
		so_ref(NULL, &nv50->state.scissor);
	if (nv50->state.viewport)
		so_ref(NULL, &nv50->state.viewport);
	if (nv50->state.tsc_upload)
		so_ref(NULL, &nv50->state.tsc_upload);
	if (nv50->state.tic_upload)
		so_ref(NULL, &nv50->state.tic_upload);
	if (nv50->state.vertprog)
		so_ref(NULL, &nv50->state.vertprog);
	if (nv50->state.fragprog)
		so_ref(NULL, &nv50->state.fragprog);
	if (nv50->state.geomprog)
		so_ref(NULL, &nv50->state.geomprog);
	if (nv50->state.fp_linkage)
		so_ref(NULL, &nv50->state.fp_linkage);
	if (nv50->state.gp_linkage)
		so_ref(NULL, &nv50->state.gp_linkage);
	if (nv50->state.vtxfmt)
		so_ref(NULL, &nv50->state.vtxfmt);
	if (nv50->state.vtxbuf)
		so_ref(NULL, &nv50->state.vtxbuf);
	if (nv50->state.vtxattr)
		so_ref(NULL, &nv50->state.vtxattr);

	draw_destroy(nv50->draw);

	if (nv50->screen->cur_ctx == nv50)
		nv50->screen->cur_ctx = NULL;

	FREE(nv50);
}


struct pipe_context *
nv50_create(struct pipe_screen *pscreen, void *priv)
{
	struct pipe_winsys *pipe_winsys = pscreen->winsys;
	struct nv50_screen *screen = nv50_screen(pscreen);
	struct nv50_context *nv50;

	nv50 = CALLOC_STRUCT(nv50_context);
	if (!nv50)
		return NULL;
	nv50->screen = screen;

	nv50->pipe.winsys = pipe_winsys;
	nv50->pipe.screen = pscreen;
	nv50->pipe.priv = priv;

	nv50->pipe.destroy = nv50_destroy;

	nv50->pipe.draw_arrays = nv50_draw_arrays;
	nv50->pipe.draw_arrays_instanced = nv50_draw_arrays_instanced;
	nv50->pipe.draw_elements = nv50_draw_elements;
	nv50->pipe.draw_elements_instanced = nv50_draw_elements_instanced;
	nv50->pipe.clear = nv50_clear;

	nv50->pipe.flush = nv50_flush;

	nv50->pipe.is_texture_referenced = nouveau_is_texture_referenced;
	nv50->pipe.is_buffer_referenced = nouveau_is_buffer_referenced;

	screen->base.channel->user_private = nv50;
	screen->base.channel->flush_notify = nv50_state_flush_notify;

	nv50_init_surface_functions(nv50);
	nv50_init_state_functions(nv50);
	nv50_init_query_functions(nv50);

	nv50->draw = draw_create(&nv50->pipe);
	assert(nv50->draw);
	draw_set_rasterize_stage(nv50->draw, nv50_draw_render_stage(nv50));

	return &nv50->pipe;
}
