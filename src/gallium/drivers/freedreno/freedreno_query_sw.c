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
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "os/os_time.h"

#include "freedreno_query_sw.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

/*
 * SW Queries:
 *
 * In the core, we have some support for basic sw counters
 */

static void
fd_sw_destroy_query(struct fd_context *ctx, struct fd_query *q)
{
	struct fd_sw_query *sq = fd_sw_query(q);
	free(sq);
}

static uint64_t
read_counter(struct fd_context *ctx, int type)
{
	switch (type) {
	case PIPE_QUERY_PRIMITIVES_GENERATED:
		/* for now same thing as _PRIMITIVES_EMITTED */
	case PIPE_QUERY_PRIMITIVES_EMITTED:
		return ctx->stats.prims_emitted;
	case FD_QUERY_DRAW_CALLS:
		return ctx->stats.draw_calls;
	case FD_QUERY_BATCH_TOTAL:
		return ctx->stats.batch_total;
	case FD_QUERY_BATCH_SYSMEM:
		return ctx->stats.batch_sysmem;
	case FD_QUERY_BATCH_GMEM:
		return ctx->stats.batch_gmem;
	case FD_QUERY_BATCH_RESTORE:
		return ctx->stats.batch_restore;
	}
	return 0;
}

static bool
is_rate_query(struct fd_query *q)
{
	switch (q->type) {
	case FD_QUERY_BATCH_TOTAL:
	case FD_QUERY_BATCH_SYSMEM:
	case FD_QUERY_BATCH_GMEM:
	case FD_QUERY_BATCH_RESTORE:
		return true;
	default:
		return false;
	}
}

static void
fd_sw_begin_query(struct fd_context *ctx, struct fd_query *q)
{
	struct fd_sw_query *sq = fd_sw_query(q);
	q->active = true;
	sq->begin_value = read_counter(ctx, q->type);
	if (is_rate_query(q))
		sq->begin_time = os_time_get();
}

static void
fd_sw_end_query(struct fd_context *ctx, struct fd_query *q)
{
	struct fd_sw_query *sq = fd_sw_query(q);
	q->active = false;
	sq->end_value = read_counter(ctx, q->type);
	if (is_rate_query(q))
		sq->end_time = os_time_get();
}

static boolean
fd_sw_get_query_result(struct fd_context *ctx, struct fd_query *q,
		boolean wait, union pipe_query_result *result)
{
	struct fd_sw_query *sq = fd_sw_query(q);

	if (q->active)
		return false;

	util_query_clear_result(result, q->type);

	result->u64 = sq->end_value - sq->begin_value;

	if (is_rate_query(q)) {
		double fps = (result->u64 * 1000000) /
				(double)(sq->end_time - sq->begin_time);
		result->u64 = (uint64_t)fps;
	}

	return true;
}

static const struct fd_query_funcs sw_query_funcs = {
		.destroy_query    = fd_sw_destroy_query,
		.begin_query      = fd_sw_begin_query,
		.end_query        = fd_sw_end_query,
		.get_query_result = fd_sw_get_query_result,
};

struct fd_query *
fd_sw_create_query(struct fd_context *ctx, unsigned query_type)
{
	struct fd_sw_query *sq;
	struct fd_query *q;

	switch (query_type) {
	case PIPE_QUERY_PRIMITIVES_GENERATED:
	case PIPE_QUERY_PRIMITIVES_EMITTED:
	case FD_QUERY_DRAW_CALLS:
	case FD_QUERY_BATCH_TOTAL:
	case FD_QUERY_BATCH_SYSMEM:
	case FD_QUERY_BATCH_GMEM:
	case FD_QUERY_BATCH_RESTORE:
		break;
	default:
		return NULL;
	}

	sq = CALLOC_STRUCT(fd_sw_query);
	if (!sq)
		return NULL;

	q = &sq->base;
	q->funcs = &sw_query_funcs;
	q->type = query_type;

	return q;
}
