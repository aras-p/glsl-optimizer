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
 */
#include "si_pipe.h"
#include "sid.h"

static struct pipe_query *si_create_query(struct pipe_context *ctx, unsigned query_type)
{
	struct si_context *sctx = (struct si_context *)ctx;

	return (struct pipe_query*)si_context_query_create(sctx, query_type);
}

static void si_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct si_context *sctx = (struct si_context *)ctx;

	si_context_query_destroy(sctx, (struct si_query *)query);
}

static void si_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_query *rquery = (struct si_query *)query;

	if (!si_query_needs_begin(rquery->type)) {
		assert(0);
		return;
	}

	memset(&rquery->result, 0, sizeof(rquery->result));
	rquery->results_start = rquery->results_end;
	si_query_begin(sctx, (struct si_query *)query);

	if (!si_is_timer_query(rquery->type)) {
		LIST_ADDTAIL(&rquery->list, &sctx->active_nontimer_query_list);
	}
}

static void si_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_query *rquery = (struct si_query *)query;

	if (!si_query_needs_begin(rquery->type)) {
		memset(&rquery->result, 0, sizeof(rquery->result));
	}

	si_query_end(sctx, rquery);

	if (si_query_needs_begin(rquery->type) && !si_is_timer_query(rquery->type)) {
		LIST_DELINIT(&rquery->list);
	}
}

static boolean si_get_query_result(struct pipe_context *ctx,
				   struct pipe_query *query,
				   boolean wait, union pipe_query_result *vresult)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_query *rquery = (struct si_query *)query;

	return si_context_query_result(sctx, rquery, wait, vresult);
}

static void si_render_condition(struct pipe_context *ctx,
				struct pipe_query *query,
				boolean condition,
				uint mode)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_query *rquery = (struct si_query *)query;
	int wait_flag = 0;

	/* If we already have nonzero result, render unconditionally */
	if (query != NULL && rquery->result.u64 != 0) {
		if (sctx->current_render_cond) {
			si_render_condition(ctx, NULL, FALSE, 0);
		}
		return;
	}

	sctx->current_render_cond = query;
	sctx->current_render_cond_cond = condition;
	sctx->current_render_cond_mode = mode;

	if (query == NULL) {
		if (sctx->predicate_drawing) {
			sctx->predicate_drawing = false;
			si_query_predication(sctx, NULL, PREDICATION_OP_CLEAR, 1);
		}
		return;
	}

	if (mode == PIPE_RENDER_COND_WAIT ||
	    mode == PIPE_RENDER_COND_BY_REGION_WAIT) {
		wait_flag = 1;
	}

	sctx->predicate_drawing = true;

	switch (rquery->type) {
	case PIPE_QUERY_OCCLUSION_COUNTER:
	case PIPE_QUERY_OCCLUSION_PREDICATE:
		si_query_predication(sctx, rquery, PREDICATION_OP_ZPASS, wait_flag);
		break;
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_SO_STATISTICS:
	case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
		si_query_predication(sctx, rquery, PREDICATION_OP_PRIMCOUNT, wait_flag);
		break;
	default:
		assert(0);
	}
}

void si_init_query_functions(struct si_context *sctx)
{
	sctx->b.b.create_query = si_create_query;
	sctx->b.b.destroy_query = si_destroy_query;
	sctx->b.b.begin_query = si_begin_query;
	sctx->b.b.end_query = si_end_query;
	sctx->b.b.get_query_result = si_get_query_result;

	if (sctx->screen->b.info.r600_num_backends > 0)
	    sctx->b.b.render_condition = si_render_condition;
}
