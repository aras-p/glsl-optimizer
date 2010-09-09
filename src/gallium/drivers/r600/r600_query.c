/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson
 */
#include <errno.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include "r600_screen.h"
#include "r600_context.h"

static void r600_query_begin(struct r600_context *rctx, struct r600_query *rquery)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate = &rquery->rstate;

	radeon_state_fini(rstate);
	radeon_state_init(rstate, rscreen->rw, R600_STATE_QUERY_BEGIN, 0, 0);
	rstate->states[R600_QUERY__OFFSET] = rquery->num_results;
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rquery->buffer);
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	if (radeon_state_pm4(rstate)) {
		radeon_state_fini(rstate);
	}
}

static void r600_query_end(struct r600_context *rctx, struct r600_query *rquery)
{
	struct r600_screen *rscreen = rctx->screen;
	struct radeon_state *rstate = &rquery->rstate;

	radeon_state_fini(rstate);
	radeon_state_init(rstate, rscreen->rw, R600_STATE_QUERY_END, 0, 0);
	rstate->states[R600_QUERY__OFFSET] = rquery->num_results + 8;
	rstate->bo[0] = radeon_bo_incref(rscreen->rw, rquery->buffer);
	rstate->nbo = 1;
	rstate->placement[0] = RADEON_GEM_DOMAIN_GTT;
	if (radeon_state_pm4(rstate)) {
		radeon_state_fini(rstate);
	}
}

static struct pipe_query *r600_create_query(struct pipe_context *ctx, unsigned query_type)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *q;

	if (query_type != PIPE_QUERY_OCCLUSION_COUNTER)
		return NULL;

	q = CALLOC_STRUCT(r600_query);
	if (!q)
		return NULL;

	q->type = query_type;
	q->buffer_size = 4096;

	q->buffer = radeon_bo(rscreen->rw, 0, q->buffer_size, 1, NULL);
	if (!q->buffer) {
		FREE(q);
		return NULL;
	}

	LIST_ADDTAIL(&q->list, &rctx->query_list);

	return (struct pipe_query *)q;
}

static void r600_destroy_query(struct pipe_context *ctx,
			       struct pipe_query *query)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	struct r600_query *q = r600_query(query);

	radeon_bo_decref(rscreen->rw, q->buffer);
	LIST_DEL(&q->list);
	FREE(query);
}

static void r600_query_result(struct pipe_context *ctx, struct r600_query *rquery)
{
	struct r600_screen *rscreen = r600_screen(ctx->screen);
	u64 start, end;
	u32 *results;
	int i;

	radeon_bo_wait(rscreen->rw, rquery->buffer);
	radeon_bo_map(rscreen->rw, rquery->buffer);
	results = rquery->buffer->data;
	for (i = 0; i < rquery->num_results; i += 4) {
		start = (u64)results[i] | (u64)results[i + 1] << 32;
		end = (u64)results[i + 2] | (u64)results[i + 3] << 32;
		if ((start & 0x8000000000000000UL) && (end & 0x8000000000000000UL)) {
			rquery->result += end - start;
		}
	}
	radeon_bo_unmap(rscreen->rw, rquery->buffer);
	rquery->num_results = 0;
}

static void r600_query_resume(struct pipe_context *ctx, struct r600_query *rquery)
{
	struct r600_context *rctx = r600_context(ctx);

	if (rquery->num_results >= ((rquery->buffer_size >> 2) - 2)) {
		/* running out of space */
		if (!rquery->flushed) {
			ctx->flush(ctx, 0, NULL);
		}
		r600_query_result(ctx, rquery);
	}
	r600_query_begin(rctx, rquery);
	rquery->flushed = false;
}

static void r600_query_suspend(struct pipe_context *ctx, struct r600_query *rquery)
{
	struct r600_context *rctx = r600_context(ctx);

	r600_query_end(rctx, rquery);
	rquery->num_results += 16;
}

static void r600_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *rquery = r600_query(query);
	int r;

	rquery->state = R600_QUERY_STATE_STARTED;
	rquery->num_results = 0;
	rquery->flushed = false;
	r600_query_resume(ctx, rquery);
	r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
	if (r == -EBUSY) {
		/* this shouldn't happen */
		R600_ERR("had to flush while emitting end query\n");
		ctx->flush(ctx, 0, NULL);
		r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
	}
}

static void r600_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *rquery = r600_query(query);
	int r;

	rquery->state &= ~R600_QUERY_STATE_STARTED;
	rquery->state |= R600_QUERY_STATE_ENDED;
	r600_query_suspend(ctx, rquery);
	r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
	if (r == -EBUSY) {
		/* this shouldn't happen */
		R600_ERR("had to flush while emitting end query\n");
		ctx->flush(ctx, 0, NULL);
		r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
	}
}

void r600_queries_suspend(struct pipe_context *ctx)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *rquery;
	int r;

	LIST_FOR_EACH_ENTRY(rquery, &rctx->query_list, list) {
		if (rquery->state & R600_QUERY_STATE_STARTED) {
			r600_query_suspend(ctx, rquery);
			r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
			if (r == -EBUSY) {
				/* this shouldn't happen */
				R600_ERR("had to flush while emitting end query\n");
				ctx->flush(ctx, 0, NULL);
				r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
			}
		}
		rquery->state |= R600_QUERY_STATE_SUSPENDED;
	}
}

void r600_queries_resume(struct pipe_context *ctx)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *rquery;
	int r;

	LIST_FOR_EACH_ENTRY(rquery, &rctx->query_list, list) {
		if (rquery->state & R600_QUERY_STATE_STARTED) {
			r600_query_resume(ctx, rquery);
			r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
			if (r == -EBUSY) {
				/* this shouldn't happen */
				R600_ERR("had to flush while emitting end query\n");
				ctx->flush(ctx, 0, NULL);
				r = radeon_ctx_set_query_state(&rctx->ctx, &rquery->rstate);
			}
		}
		rquery->state &= ~R600_QUERY_STATE_SUSPENDED;
	}
}

static boolean r600_get_query_result(struct pipe_context *ctx,
					struct pipe_query *query,
					boolean wait, void *vresult)
{
	struct r600_query *rquery = r600_query(query);
	uint64_t *result = (uint64_t*)vresult;

	if (!rquery->flushed) {
		ctx->flush(ctx, 0, NULL);
		rquery->flushed = true;
	}
	r600_query_result(ctx, rquery);
	*result = rquery->result;
	rquery->result = 0;
	return TRUE;
}

void r600_init_query_functions(struct r600_context* rctx)
{
	LIST_INITHEAD(&rctx->query_list);

	rctx->context.create_query = r600_create_query;
	rctx->context.destroy_query = r600_destroy_query;
	rctx->context.begin_query = r600_begin_query;
	rctx->context.end_query = r600_end_query;
	rctx->context.get_query_result = r600_get_query_result;
}
