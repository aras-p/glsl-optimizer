/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "freedreno_texture.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

static void
fd_sampler_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

static void
fd_sampler_view_destroy(struct pipe_context *pctx,
		struct pipe_sampler_view *view)
{
	pipe_resource_reference(&view->texture, NULL);
	FREE(view);
}

static void bind_sampler_states(struct fd_texture_stateobj *prog,
		unsigned nr, void **hwcso)
{
	unsigned i;
	unsigned new_nr = 0;

	for (i = 0; i < nr; i++) {
		if (hwcso[i])
			new_nr = i + 1;
		prog->samplers[i] = hwcso[i];
		prog->dirty_samplers |= (1 << i);
	}

	for (; i < prog->num_samplers; i++) {
		prog->samplers[i] = NULL;
		prog->dirty_samplers |= (1 << i);
	}

	prog->num_samplers = new_nr;
}

static void set_sampler_views(struct fd_texture_stateobj *prog,
		unsigned nr, struct pipe_sampler_view **views)
{
	unsigned i;
	unsigned new_nr = 0;

	for (i = 0; i < nr; i++) {
		if (views[i])
			new_nr = i + 1;
		pipe_sampler_view_reference(&prog->textures[i], views[i]);
		prog->dirty_samplers |= (1 << i);
	}

	for (; i < prog->num_textures; i++) {
		pipe_sampler_view_reference(&prog->textures[i], NULL);
		prog->dirty_samplers |= (1 << i);
	}

	prog->num_textures = new_nr;
}

static void
fd_sampler_states_bind(struct pipe_context *pctx,
		unsigned shader, unsigned start,
		unsigned nr, void **hwcso)
{
	struct fd_context *ctx = fd_context(pctx);

	assert(start == 0);

	if (shader == PIPE_SHADER_FRAGMENT) {
		/* on a2xx, since there is a flat address space for textures/samplers,
		 * a change in # of fragment textures/samplers will trigger patching and
		 * re-emitting the vertex shader:
		 */
		if (nr != ctx->fragtex.num_samplers)
			ctx->dirty |= FD_DIRTY_TEXSTATE;

		bind_sampler_states(&ctx->fragtex, nr, hwcso);
		ctx->dirty |= FD_DIRTY_FRAGTEX;
	}
	else if (shader == PIPE_SHADER_VERTEX) {
		bind_sampler_states(&ctx->verttex, nr, hwcso);
		ctx->dirty |= FD_DIRTY_VERTTEX;
	}
}


static void
fd_fragtex_set_sampler_views(struct pipe_context *pctx, unsigned nr,
		struct pipe_sampler_view **views)
{
	struct fd_context *ctx = fd_context(pctx);

	/* on a2xx, since there is a flat address space for textures/samplers,
	 * a change in # of fragment textures/samplers will trigger patching and
	 * re-emitting the vertex shader:
	 */
	if (nr != ctx->fragtex.num_textures)
		ctx->dirty |= FD_DIRTY_TEXSTATE;

	set_sampler_views(&ctx->fragtex, nr, views);
	ctx->dirty |= FD_DIRTY_FRAGTEX;
}

static void
fd_verttex_set_sampler_views(struct pipe_context *pctx, unsigned nr,
		struct pipe_sampler_view **views)
{
	struct fd_context *ctx = fd_context(pctx);
	set_sampler_views(&ctx->verttex, nr, views);
	ctx->dirty |= FD_DIRTY_VERTTEX;
}

static void
fd_set_sampler_views(struct pipe_context *pctx, unsigned shader,
                     unsigned start, unsigned nr,
                     struct pipe_sampler_view **views)
{
   assert(start == 0);
   switch (shader) {
   case PIPE_SHADER_FRAGMENT:
      fd_fragtex_set_sampler_views(pctx, nr, views);
      break;
   case PIPE_SHADER_VERTEX:
      fd_verttex_set_sampler_views(pctx, nr, views);
      break;
   default:
      ;
   }
}

void
fd_texture_init(struct pipe_context *pctx)
{
	pctx->delete_sampler_state = fd_sampler_state_delete;

	pctx->sampler_view_destroy = fd_sampler_view_destroy;

	pctx->bind_sampler_states = fd_sampler_states_bind;
	pctx->set_sampler_views = fd_set_sampler_views;
}
