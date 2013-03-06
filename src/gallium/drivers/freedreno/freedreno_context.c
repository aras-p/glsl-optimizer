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

#include "freedreno_context.h"
#include "freedreno_vbo.h"
#include "freedreno_blend.h"
#include "freedreno_rasterizer.h"
#include "freedreno_zsa.h"
#include "freedreno_state.h"
#include "freedreno_resource.h"
#include "freedreno_clear.h"
#include "freedreno_program.h"
#include "freedreno_texture.h"
#include "freedreno_gmem.h"
#include "freedreno_util.h"

/* there are two cases where we currently need to wait for render complete:
 * 1) pctx->flush() .. since at the moment we have no way for DDX to sync
 *    the presentation blit with the 3d core
 * 2) wrap-around for ringbuffer.. possibly we can do something more
 *    Intelligent here.  Right now we need to ensure there is enough room
 *    at the end of the drawcmds in the cmdstream buffer for all the per-
 *    tile cmds.  We do this the lamest way possible, by making the ringbuffer
 *    big, and flushing and resetting back to the beginning if we get too
 *    close to the end.
 */
static void
fd_context_wait(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	uint32_t ts = fd_ringbuffer_timestamp(ctx->ring);

	DBG("wait: %u", ts);

	fd_pipe_wait(ctx->screen->pipe, ts);
	fd_ringbuffer_reset(ctx->ring);
	fd_ringmarker_mark(ctx->draw_start);
}

/* emit accumulated render cmds, needed for example if render target has
 * changed, or for flush()
 */
void
fd_context_render(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_framebuffer_state *pfb = &ctx->framebuffer;

	DBG("needs_flush: %d", ctx->needs_flush);

	if (!ctx->needs_flush)
		return;

	fd_gmem_render_tiles(pctx);

	DBG("%p/%p/%p", ctx->ring->start, ctx->ring->cur, ctx->ring->end);

	/* if size in dwords is more than half the buffer size, then wait and
	 * wrap around:
	 */
	if ((ctx->ring->cur - ctx->ring->start) > ctx->ring->size/8)
		fd_context_wait(pctx);

	ctx->needs_flush = false;
	ctx->cleared = ctx->restore = ctx->resolve = 0;

	fd_resource(pfb->cbufs[0]->texture)->dirty = false;
	if (pfb->zsbuf)
		fd_resource(pfb->zsbuf->texture)->dirty = false;
}

static void
fd_context_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
		enum pipe_flush_flags flags)
{
	DBG("fence=%p", fence);

#if 0
	if (fence) {
		fd_fence_ref(ctx->screen->fence.current,
				(struct fd_fence **)fence);
	}
#endif

	fd_context_render(pctx);
	fd_context_wait(pctx);
}

static void
fd_context_destroy(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	DBG("");

	if (ctx->blitter)
		util_blitter_destroy(ctx->blitter);

	fd_ringmarker_del(ctx->draw_start);
	fd_ringmarker_del(ctx->draw_end);
	fd_ringbuffer_del(ctx->ring);

	fd_prog_fini(pctx);

	FREE(ctx);
}

static struct pipe_resource *
create_solid_vertexbuf(struct pipe_context *pctx)
{
	static const float init_shader_const[] = {
			/* for clear/gmem2mem: */
			-1.000000, +1.000000, +1.000000, +1.100000,
			+1.000000, +1.000000, -1.000000, -1.100000,
			+1.000000, +1.100000, -1.100000, +1.000000,
			/* for mem2gmem: (vertices) */
			-1.000000, +1.000000, +1.000000, +1.000000,
			+1.000000, +1.000000, -1.000000, -1.000000,
			+1.000000, +1.000000, -1.000000, +1.000000,
			/* for mem2gmem: (tex coords) */
			+0.000000, +0.000000, +1.000000, +0.000000,
			+0.000000, +1.000000, +1.000000, +1.000000,
	};
	struct pipe_resource *prsc = pipe_buffer_create(pctx->screen,
			PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE, sizeof(init_shader_const));
	pipe_buffer_write(pctx, prsc, 0,
			sizeof(init_shader_const), init_shader_const);
	return prsc;
}

struct pipe_context *
fd_context_create(struct pipe_screen *pscreen, void *priv)
{
	struct fd_screen *screen = fd_screen(pscreen);
	struct fd_context *ctx = CALLOC_STRUCT(fd_context);
	struct pipe_context *pctx;

	if (!ctx)
		return NULL;

	DBG("");

	ctx->screen = screen;

	ctx->ring = fd_ringbuffer_new(screen->pipe, 0x100000);
	ctx->draw_start = fd_ringmarker_new(ctx->ring);
	ctx->draw_end = fd_ringmarker_new(ctx->ring);

	pctx = &ctx->base;
	pctx->screen = pscreen;
	pctx->priv = priv;
	pctx->flush = fd_context_flush;
	pctx->destroy = fd_context_destroy;

	util_slab_create(&ctx->transfer_pool, sizeof(struct pipe_transfer),
			16, UTIL_SLAB_SINGLETHREADED);

	fd_vbo_init(pctx);
	fd_blend_init(pctx);
	fd_rasterizer_init(pctx);
	fd_zsa_init(pctx);
	fd_state_init(pctx);
	fd_resource_context_init(pctx);
	fd_clear_init(pctx);
	fd_prog_init(pctx);
	fd_texture_init(pctx);

	ctx->blitter = util_blitter_create(pctx);
	if (!ctx->blitter) {
		fd_context_destroy(pctx);
		return NULL;
	}

	/* construct vertex state used for solid ops (clear, and gmem<->mem) */
	ctx->solid_vertexbuf = create_solid_vertexbuf(pctx);

	fd_state_emit_setup(pctx);

	return pctx;
}
