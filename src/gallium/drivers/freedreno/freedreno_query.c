/* -*- mode: C; c-file-style: "k&r"; ttxab-width 4; indent-tabs-mode: t; -*- */

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
#include "os/os_time.h"

#include "freedreno_query.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

#define FD_QUERY_DRAW_CALLS      (PIPE_QUERY_DRIVER_SPECIFIC + 0)
#define FD_QUERY_BATCH_TOTAL     (PIPE_QUERY_DRIVER_SPECIFIC + 1)  /* total # of batches (submits) */
#define FD_QUERY_BATCH_SYSMEM    (PIPE_QUERY_DRIVER_SPECIFIC + 2)  /* batches using system memory (GMEM bypass) */
#define FD_QUERY_BATCH_GMEM      (PIPE_QUERY_DRIVER_SPECIFIC + 3)  /* batches using GMEM */
#define FD_QUERY_BATCH_RESTORE   (PIPE_QUERY_DRIVER_SPECIFIC + 4)  /* batches requiring GMEM restore */

/* Currently just simple cpu query's supported.. probably need
 * to refactor this a bit when I'm eventually ready to add gpu
 * queries:
 */
struct fd_query {
	int type;
	/* storage for the collected data */
	union pipe_query_result data;
	bool active;
	uint64_t begin_value, end_value;
	uint64_t begin_time, end_time;
};

static inline struct fd_query *
fd_query(struct pipe_query *pq)
{
	return (struct fd_query *)pq;
}

static struct pipe_query *
fd_create_query(struct pipe_context *pctx, unsigned query_type)
{
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

	q = CALLOC_STRUCT(fd_query);
	if (!q)
		return NULL;

	q->type = query_type;

	return (struct pipe_query *) q;
}

static void
fd_destroy_query(struct pipe_context *pctx, struct pipe_query *pq)
{
	struct fd_query *q = fd_query(pq);
	free(q);
}

static uint64_t
read_counter(struct pipe_context *pctx, int type)
{
	struct fd_context *ctx = fd_context(pctx);
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
fd_begin_query(struct pipe_context *pctx, struct pipe_query *pq)
{
	struct fd_query *q = fd_query(pq);
	q->active = true;
	q->begin_value = read_counter(pctx, q->type);
	if (is_rate_query(q))
		q->begin_time = os_time_get();
}

static void
fd_end_query(struct pipe_context *pctx, struct pipe_query *pq)
{
	struct fd_query *q = fd_query(pq);
	q->active = false;
	q->end_value = read_counter(pctx, q->type);
	if (is_rate_query(q))
		q->end_time = os_time_get();
}

static boolean
fd_get_query_result(struct pipe_context *pctx, struct pipe_query *pq,
		boolean wait, union pipe_query_result *result)
{
	struct fd_query *q = fd_query(pq);

	if (q->active)
		return false;

	util_query_clear_result(result, q->type);

	result->u64 = q->end_value - q->begin_value;

	if (is_rate_query(q)) {
		double fps = (result->u64 * 1000000) /
				(double)(q->end_time - q->begin_time);
		result->u64 = (uint64_t)fps;
	}

	return true;
}

static int
fd_get_driver_query_info(struct pipe_screen *pscreen,
		unsigned index, struct pipe_driver_query_info *info)
{
	struct pipe_driver_query_info list[] = {
			{"draw-calls", FD_QUERY_DRAW_CALLS, 0},
			{"batches", FD_QUERY_BATCH_TOTAL, 0},
			{"batches-sysmem", FD_QUERY_BATCH_SYSMEM, 0},
			{"batches-gmem", FD_QUERY_BATCH_GMEM, 0},
			{"restores", FD_QUERY_BATCH_RESTORE, 0},
			{"prims-emitted", PIPE_QUERY_PRIMITIVES_EMITTED, 0},
	};

	if (!info)
		return ARRAY_SIZE(list);

	if (index >= ARRAY_SIZE(list))
		return 0;

	*info = list[index];
	return 1;
}

void
fd_query_screen_init(struct pipe_screen *pscreen)
{
	pscreen->get_driver_query_info = fd_get_driver_query_info;
}

void
fd_query_context_init(struct pipe_context *pctx)
{
	pctx->create_query = fd_create_query;
	pctx->destroy_query = fd_destroy_query;
	pctx->begin_query = fd_begin_query;
	pctx->end_query = fd_end_query;
	pctx->get_query_result = fd_get_query_result;
}
