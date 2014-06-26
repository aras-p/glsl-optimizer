/* -*- mode: C; c-file-style: "k&r"; ttxab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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

#include "freedreno_query.h"
#include "freedreno_query_sw.h"
#include "freedreno_query_hw.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

/*
 * Pipe Query interface:
 */

static struct pipe_query *
fd_create_query(struct pipe_context *pctx, unsigned query_type, unsigned index)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_query *q;

	q = fd_sw_create_query(ctx, query_type);
	if (!q)
		q = fd_hw_create_query(ctx, query_type);

	return (struct pipe_query *) q;
}

static void
fd_destroy_query(struct pipe_context *pctx, struct pipe_query *pq)
{
	struct fd_query *q = fd_query(pq);
	q->funcs->destroy_query(fd_context(pctx), q);
}

static void
fd_begin_query(struct pipe_context *pctx, struct pipe_query *pq)
{
	struct fd_query *q = fd_query(pq);
	q->funcs->begin_query(fd_context(pctx), q);
}

static void
fd_end_query(struct pipe_context *pctx, struct pipe_query *pq)
{
	struct fd_query *q = fd_query(pq);
	q->funcs->end_query(fd_context(pctx), q);
}

static boolean
fd_get_query_result(struct pipe_context *pctx, struct pipe_query *pq,
		boolean wait, union pipe_query_result *result)
{
	struct fd_query *q = fd_query(pq);
	return q->funcs->get_query_result(fd_context(pctx), q, wait, result);
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
