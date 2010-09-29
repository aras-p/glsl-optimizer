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
#include "r600_pipe.h"

static struct pipe_query *r600_create_query(struct pipe_context *ctx, unsigned query_type)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	return (struct pipe_query*)r600_context_query_create(&rctx->ctx, query_type);
}

static void r600_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	r600_context_query_destroy(&rctx->ctx, (struct r600_query *)query);
}

static void r600_begin_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;

	rquery->result = 0;
	rquery->num_results = 0;
	r600_query_begin(&rctx->ctx, (struct r600_query *)query);
}

static void r600_end_query(struct pipe_context *ctx, struct pipe_query *query)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;

	r600_query_end(&rctx->ctx, (struct r600_query *)query);
}

static boolean r600_get_query_result(struct pipe_context *ctx,
					struct pipe_query *query,
					boolean wait, void *vresult)
{
	struct r600_pipe_context *rctx = (struct r600_pipe_context *)ctx;
	struct r600_query *rquery = (struct r600_query *)query;

	if (rquery->num_results) {
		ctx->flush(ctx, 0, NULL);
	}
	return r600_context_query_result(&rctx->ctx, (struct r600_query *)query, wait, vresult);
}

void r600_init_query_functions(struct r600_pipe_context *rctx)
{
	rctx->context.create_query = r600_create_query;
	rctx->context.destroy_query = r600_destroy_query;
	rctx->context.begin_query = r600_begin_query;
	rctx->context.end_query = r600_end_query;
	rctx->context.get_query_result = r600_get_query_result;
}
