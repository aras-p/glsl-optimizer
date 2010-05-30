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
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include "r600_screen.h"
#include "r600_context.h"

static struct pipe_query *r600_create_query(struct pipe_context *pipe, unsigned query_type)
{
	return NULL;
}

static void r600_destroy_query(struct pipe_context *pipe, struct pipe_query *query)
{
	FREE(query);
}

static void r600_begin_query(struct pipe_context *pipe, struct pipe_query *query)
{
}

static void r600_end_query(struct pipe_context *pipe, struct pipe_query *query)
{
}

static boolean r600_get_query_result(struct pipe_context *pipe,
					struct pipe_query *query,
					boolean wait, uint64_t *result)
{
	return TRUE;
}

void r600_init_query_functions(struct r600_context* rctx)
{
	rctx->context.create_query = r600_create_query;
	rctx->context.destroy_query = r600_destroy_query;
	rctx->context.begin_query = r600_begin_query;
	rctx->context.end_query = r600_end_query;
	rctx->context.get_query_result = r600_get_query_result;
}
