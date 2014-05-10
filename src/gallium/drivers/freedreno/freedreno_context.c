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
#include "freedreno_draw.h"
#include "freedreno_program.h"
#include "freedreno_resource.h"
#include "freedreno_texture.h"
#include "freedreno_state.h"
#include "freedreno_gmem.h"
#include "freedreno_query.h"
#include "freedreno_query_hw.h"
#include "freedreno_util.h"

static struct fd_ringbuffer *next_rb(struct fd_context *ctx)
{
	struct fd_ringbuffer *ring;
	uint32_t ts;

	/* grab next ringbuffer: */
	ring = ctx->rings[(ctx->rings_idx++) % ARRAY_SIZE(ctx->rings)];

	/* wait for new rb to be idle: */
	ts = fd_ringbuffer_timestamp(ring);
	if (ts) {
		DBG("wait: %u", ts);
		fd_pipe_wait(ctx->screen->pipe, ts);
	}

	fd_ringbuffer_reset(ring);

	return ring;
}

static void
fd_context_next_rb(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_ringbuffer *ring;

	fd_ringmarker_del(ctx->draw_start);
	fd_ringmarker_del(ctx->draw_end);

	ring = next_rb(ctx);

	ctx->draw_start = fd_ringmarker_new(ring);
	ctx->draw_end = fd_ringmarker_new(ring);

	fd_ringbuffer_set_parent(ring, NULL);
	ctx->ring = ring;

	fd_ringmarker_del(ctx->binning_start);
	fd_ringmarker_del(ctx->binning_end);

	ring = next_rb(ctx);

	ctx->binning_start = fd_ringmarker_new(ring);
	ctx->binning_end = fd_ringmarker_new(ring);

	fd_ringbuffer_set_parent(ring, ctx->ring);
	ctx->binning_ring = ring;
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
		fd_context_next_rb(pctx);

	ctx->needs_flush = false;
	ctx->cleared = ctx->restore = ctx->resolve = 0;
	ctx->gmem_reason = 0;
	ctx->num_draws = 0;

	if (pfb->cbufs[0])
		fd_resource(pfb->cbufs[0]->texture)->dirty = false;
	if (pfb->zsbuf)
		fd_resource(pfb->zsbuf->texture)->dirty = false;
}

static void
fd_context_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
		unsigned flags)
{
	DBG("fence=%p", fence);

#if 0
	if (fence) {
		fd_fence_ref(ctx->screen->fence.current,
				(struct fd_fence **)fence);
	}
#endif

	fd_context_render(pctx);
}

void
fd_context_destroy(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);
	unsigned i;

	DBG("");

	fd_prog_fini(pctx);
	fd_hw_query_fini(pctx);

	util_slab_destroy(&ctx->transfer_pool);

	util_dynarray_fini(&ctx->draw_patches);

	if (ctx->blitter)
		util_blitter_destroy(ctx->blitter);

	if (ctx->primconvert)
		util_primconvert_destroy(ctx->primconvert);

	fd_ringmarker_del(ctx->draw_start);
	fd_ringmarker_del(ctx->draw_end);
	fd_ringmarker_del(ctx->binning_start);
	fd_ringmarker_del(ctx->binning_end);

	for (i = 0; i < ARRAY_SIZE(ctx->rings); i++)
		fd_ringbuffer_del(ctx->rings[i]);

	for (i = 0; i < ARRAY_SIZE(ctx->pipe); i++) {
		struct fd_vsc_pipe *pipe = &ctx->pipe[i];
		if (!pipe->bo)
			break;
		fd_bo_del(pipe->bo);
	}

	fd_device_del(ctx->dev);

	FREE(ctx);
}

struct pipe_context *
fd_context_init(struct fd_context *ctx, struct pipe_screen *pscreen,
		const uint8_t *primtypes, void *priv)
{
	struct fd_screen *screen = fd_screen(pscreen);
	struct pipe_context *pctx;
	int i;

	ctx->screen = screen;

	ctx->primtypes = primtypes;
	ctx->primtype_mask = 0;
	for (i = 0; i < PIPE_PRIM_MAX; i++)
		if (primtypes[i])
			ctx->primtype_mask |= (1 << i);

	/* need some sane default in case state tracker doesn't
	 * set some state:
	 */
	ctx->sample_mask = 0xffff;

	pctx = &ctx->base;
	pctx->screen = pscreen;
	pctx->priv = priv;
	pctx->flush = fd_context_flush;

	for (i = 0; i < ARRAY_SIZE(ctx->rings); i++) {
		ctx->rings[i] = fd_ringbuffer_new(screen->pipe, 0x100000);
		if (!ctx->rings[i])
			goto fail;
	}

	fd_context_next_rb(pctx);
	fd_reset_wfi(ctx);

	util_dynarray_init(&ctx->draw_patches);

	util_slab_create(&ctx->transfer_pool, sizeof(struct pipe_transfer),
			16, UTIL_SLAB_SINGLETHREADED);

	fd_draw_init(pctx);
	fd_resource_context_init(pctx);
	fd_query_context_init(pctx);
	fd_texture_init(pctx);
	fd_state_init(pctx);
	fd_hw_query_init(pctx);

	ctx->blitter = util_blitter_create(pctx);
	if (!ctx->blitter)
		goto fail;

	ctx->primconvert = util_primconvert_create(pctx, ctx->primtype_mask);
	if (!ctx->primconvert)
		goto fail;

	return pctx;

fail:
	pctx->destroy(pctx);
	return NULL;
}
