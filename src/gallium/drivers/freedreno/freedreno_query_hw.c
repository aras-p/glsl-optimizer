/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "freedreno_query_hw.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

struct fd_hw_sample_period {
	struct fd_hw_sample *start, *end;
	struct list_head list;
};

/* maps query_type to sample provider idx: */
static int pidx(unsigned query_type)
{
	switch (query_type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
		return 0;
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		return 1;
	default:
		return -1;
	}
}

static struct fd_hw_sample *
get_sample(struct fd_context *ctx, struct fd_ringbuffer *ring,
		unsigned query_type)
{
	struct fd_hw_sample *samp = NULL;
	int idx = pidx(query_type);

	if (!ctx->sample_cache[idx]) {
		ctx->sample_cache[idx] =
			ctx->sample_providers[idx]->get_sample(ctx, ring);
	}

	fd_hw_sample_reference(ctx, &samp, ctx->sample_cache[idx]);

	return samp;
}

static void
clear_sample_cache(struct fd_context *ctx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ctx->sample_cache); i++)
		fd_hw_sample_reference(ctx, &ctx->sample_cache[i], NULL);
}

static bool
is_active(struct fd_hw_query *hq, enum fd_render_stage stage)
{
	return !!(hq->provider->active & stage);
}


static void
resume_query(struct fd_context *ctx, struct fd_hw_query *hq,
		struct fd_ringbuffer *ring)
{
	assert(!hq->period);
	hq->period = util_slab_alloc(&ctx->sample_period_pool);
	list_inithead(&hq->period->list);
	hq->period->start = get_sample(ctx, ring, hq->base.type);
	/* NOTE: util_slab_alloc() does not zero out the buffer: */
	hq->period->end = NULL;
}

static void
pause_query(struct fd_context *ctx, struct fd_hw_query *hq,
		struct fd_ringbuffer *ring)
{
	assert(hq->period && !hq->period->end);
	hq->period->end = get_sample(ctx, ring, hq->base.type);
	list_addtail(&hq->period->list, &hq->current_periods);
	hq->period = NULL;
}

static void
destroy_periods(struct fd_context *ctx, struct list_head *list)
{
	struct fd_hw_sample_period *period, *s;
	LIST_FOR_EACH_ENTRY_SAFE(period, s, list, list) {
		fd_hw_sample_reference(ctx, &period->start, NULL);
		fd_hw_sample_reference(ctx, &period->end, NULL);
		list_del(&period->list);
		util_slab_free(&ctx->sample_period_pool, period);
	}
}

static void
fd_hw_destroy_query(struct fd_context *ctx, struct fd_query *q)
{
	struct fd_hw_query *hq = fd_hw_query(q);

	destroy_periods(ctx, &hq->periods);
	destroy_periods(ctx, &hq->current_periods);
	list_del(&hq->list);

	free(hq);
}

static void
fd_hw_begin_query(struct fd_context *ctx, struct fd_query *q)
{
	struct fd_hw_query *hq = fd_hw_query(q);
	if (q->active)
		return;

	/* begin_query() should clear previous results: */
	destroy_periods(ctx, &hq->periods);

	if (is_active(hq, ctx->stage))
		resume_query(ctx, hq, ctx->ring);

	q->active = true;

	/* add to active list: */
	list_del(&hq->list);
	list_addtail(&hq->list, &ctx->active_queries);
}

static void
fd_hw_end_query(struct fd_context *ctx, struct fd_query *q)
{
	struct fd_hw_query *hq = fd_hw_query(q);
	if (!q->active)
		return;
	if (is_active(hq, ctx->stage))
		pause_query(ctx, hq, ctx->ring);
	q->active = false;
	/* move to current list: */
	list_del(&hq->list);
	list_addtail(&hq->list, &ctx->current_queries);
}

/* helper to get ptr to specified sample: */
static void * sampptr(struct fd_hw_sample *samp, uint32_t n, void *ptr)
{
	return ((char *)ptr) + (samp->tile_stride * n) + samp->offset;
}

static boolean
fd_hw_get_query_result(struct fd_context *ctx, struct fd_query *q,
		boolean wait, union pipe_query_result *result)
{
	struct fd_hw_query *hq = fd_hw_query(q);
	const struct fd_hw_sample_provider *p = hq->provider;
	struct fd_hw_sample_period *period;

	if (q->active)
		return false;

	/* if the app tries to read back the query result before the
	 * back is submitted, that forces us to flush so that there
	 * are actually results to wait for:
	 */
	if (!LIST_IS_EMPTY(&hq->list)) {
		DBG("reading query result forces flush!");
		ctx->needs_flush = true;
		fd_context_render(&ctx->base);
	}

	util_query_clear_result(result, q->type);

	if (LIST_IS_EMPTY(&hq->periods))
		return true;

	assert(LIST_IS_EMPTY(&hq->list));
	assert(LIST_IS_EMPTY(&hq->current_periods));
	assert(!hq->period);

	if (LIST_IS_EMPTY(&hq->periods))
		return true;

	/* if !wait, then check the last sample (the one most likely to
	 * not be ready yet) and bail if it is not ready:
	 */
	if (!wait) {
		int ret;

		period = LIST_ENTRY(struct fd_hw_sample_period,
				hq->periods.prev, list);

		ret = fd_bo_cpu_prep(period->end->bo, ctx->screen->pipe,
				DRM_FREEDRENO_PREP_READ | DRM_FREEDRENO_PREP_NOSYNC);
		if (ret)
			return false;

		fd_bo_cpu_fini(period->end->bo);
	}

	/* sum the result across all sample periods: */
	LIST_FOR_EACH_ENTRY(period, &hq->periods, list) {
		struct fd_hw_sample *start = period->start;
		struct fd_hw_sample *end = period->end;
		unsigned i;

		/* start and end samples should be from same batch: */
		assert(start->bo == end->bo);
		assert(start->num_tiles == end->num_tiles);

		for (i = 0; i < start->num_tiles; i++) {
			void *ptr;

			fd_bo_cpu_prep(start->bo, ctx->screen->pipe,
					DRM_FREEDRENO_PREP_READ);

			ptr = fd_bo_map(start->bo);

			p->accumulate_result(ctx, sampptr(period->start, i, ptr),
					sampptr(period->end, i, ptr), result);

			fd_bo_cpu_fini(start->bo);
		}
	}

	return true;
}

static const struct fd_query_funcs hw_query_funcs = {
		.destroy_query    = fd_hw_destroy_query,
		.begin_query      = fd_hw_begin_query,
		.end_query        = fd_hw_end_query,
		.get_query_result = fd_hw_get_query_result,
};

struct fd_query *
fd_hw_create_query(struct fd_context *ctx, unsigned query_type)
{
	struct fd_hw_query *hq;
	struct fd_query *q;
	int idx = pidx(query_type);

	if ((idx < 0) || !ctx->sample_providers[idx])
		return NULL;

	hq = CALLOC_STRUCT(fd_hw_query);
	if (!hq)
		return NULL;

	hq->provider = ctx->sample_providers[idx];

	list_inithead(&hq->periods);
	list_inithead(&hq->current_periods);
	list_inithead(&hq->list);

	q = &hq->base;
	q->funcs = &hw_query_funcs;
	q->type = query_type;

	return q;
}

struct fd_hw_sample *
fd_hw_sample_init(struct fd_context *ctx, uint32_t size)
{
	struct fd_hw_sample *samp = util_slab_alloc(&ctx->sample_pool);
	pipe_reference_init(&samp->reference, 1);
	samp->size = size;
	samp->offset = ctx->next_sample_offset;
	/* NOTE: util_slab_alloc() does not zero out the buffer: */
	samp->bo = NULL;
	samp->num_tiles = 0;
	samp->tile_stride = 0;
	ctx->next_sample_offset += size;
	return samp;
}

void
__fd_hw_sample_destroy(struct fd_context *ctx, struct fd_hw_sample *samp)
{
	if (samp->bo)
		fd_bo_del(samp->bo);
	util_slab_free(&ctx->sample_pool, samp);
}

static void
prepare_sample(struct fd_hw_sample *samp, struct fd_bo *bo,
		uint32_t num_tiles, uint32_t tile_stride)
{
	if (samp->bo) {
		assert(samp->bo == bo);
		assert(samp->num_tiles == num_tiles);
		assert(samp->tile_stride == tile_stride);
		return;
	}
	samp->bo = bo;
	samp->num_tiles = num_tiles;
	samp->tile_stride = tile_stride;
}

static void
prepare_query(struct fd_hw_query *hq, struct fd_bo *bo,
		uint32_t num_tiles, uint32_t tile_stride)
{
	struct fd_hw_sample_period *period, *s;

	/* prepare all the samples in the query: */
	LIST_FOR_EACH_ENTRY_SAFE(period, s, &hq->current_periods, list) {
		prepare_sample(period->start, bo, num_tiles, tile_stride);
		prepare_sample(period->end, bo, num_tiles, tile_stride);

		/* move from current_periods list to periods list: */
		list_del(&period->list);
		list_addtail(&period->list, &hq->periods);
	}
}

static void
prepare_queries(struct fd_context *ctx, struct fd_bo *bo,
		uint32_t num_tiles, uint32_t tile_stride,
		struct list_head *list, bool remove)
{
	struct fd_hw_query *hq, *s;
	LIST_FOR_EACH_ENTRY_SAFE(hq, s, list, list) {
		prepare_query(hq, bo, num_tiles, tile_stride);
		if (remove)
			list_delinit(&hq->list);
	}
}

/* called from gmem code once total storage requirements are known (ie.
 * number of samples times number of tiles)
 */
void
fd_hw_query_prepare(struct fd_context *ctx, uint32_t num_tiles)
{
	uint32_t tile_stride = ctx->next_sample_offset;
	struct fd_bo *bo;

	if (ctx->query_bo)
		fd_bo_del(ctx->query_bo);

	if (tile_stride > 0) {
		bo = fd_bo_new(ctx->dev, tile_stride * num_tiles,
				DRM_FREEDRENO_GEM_CACHE_WCOMBINE |
				DRM_FREEDRENO_GEM_TYPE_KMEM);
	} else {
		bo = NULL;
	}

	ctx->query_bo = bo;
	ctx->query_tile_stride = tile_stride;

	prepare_queries(ctx, bo, num_tiles, tile_stride,
			&ctx->active_queries, false);
	prepare_queries(ctx, bo, num_tiles, tile_stride,
			&ctx->current_queries, true);

	/* reset things for next batch: */
	ctx->next_sample_offset = 0;
}

void
fd_hw_query_prepare_tile(struct fd_context *ctx, uint32_t n,
		struct fd_ringbuffer *ring)
{
	uint32_t tile_stride = ctx->query_tile_stride;
	uint32_t offset = tile_stride * n;

	/* bail if no queries: */
	if (tile_stride == 0)
		return;

	fd_wfi(ctx, ring);
	OUT_PKT0 (ring, HW_QUERY_BASE_REG, 1);
	OUT_RELOCW(ring, ctx->query_bo, offset, 0, 0);
}

void
fd_hw_query_set_stage(struct fd_context *ctx, struct fd_ringbuffer *ring,
		enum fd_render_stage stage)
{
	/* special case: internal blits (like mipmap level generation)
	 * go through normal draw path (via util_blitter_blit()).. but
	 * we need to ignore the FD_STAGE_DRAW which will be set, so we
	 * don't enable queries which should be paused during internal
	 * blits:
	 */
	if ((ctx->stage == FD_STAGE_BLIT) &&
			(stage != FD_STAGE_NULL))
		return;

	if (stage != ctx->stage) {
		struct fd_hw_query *hq;
		LIST_FOR_EACH_ENTRY(hq, &ctx->active_queries, list) {
			bool was_active = is_active(hq, ctx->stage);
			bool now_active = is_active(hq, stage);

			if (now_active && !was_active)
				resume_query(ctx, hq, ring);
			else if (was_active && !now_active)
				pause_query(ctx, hq, ring);
		}
	}
	clear_sample_cache(ctx);
	ctx->stage = stage;
}

void
fd_hw_query_register_provider(struct pipe_context *pctx,
		const struct fd_hw_sample_provider *provider)
{
	struct fd_context *ctx = fd_context(pctx);
	int idx = pidx(provider->query_type);

	assert((0 <= idx) && (idx < MAX_HW_SAMPLE_PROVIDERS));
	assert(!ctx->sample_providers[idx]);

	ctx->sample_providers[idx] = provider;
}

void
fd_hw_query_init(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	util_slab_create(&ctx->sample_pool, sizeof(struct fd_hw_sample),
			16, UTIL_SLAB_SINGLETHREADED);
	util_slab_create(&ctx->sample_period_pool, sizeof(struct fd_hw_sample_period),
			16, UTIL_SLAB_SINGLETHREADED);
	list_inithead(&ctx->active_queries);
	list_inithead(&ctx->current_queries);
}

void
fd_hw_query_fini(struct pipe_context *pctx)
{
	struct fd_context *ctx = fd_context(pctx);

	util_slab_destroy(&ctx->sample_pool);
	util_slab_destroy(&ctx->sample_period_pool);
}
