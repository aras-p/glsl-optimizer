/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_context.h"

#include "nv50_context.h"

static struct pipe_query *
nv50_query_create(struct pipe_context *pipe, unsigned type)
{
	NOUVEAU_ERR("unimplemented\n");
	return NULL;
}

static void
nv50_query_destroy(struct pipe_context *pipe, struct pipe_query *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_query_begin(struct pipe_context *pipe, struct pipe_query *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_query_end(struct pipe_context *pipe, struct pipe_query *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static boolean
nv50_query_result(struct pipe_context *pipe, struct pipe_query *q,
		  boolean wait, uint64 *result)
{
	NOUVEAU_ERR("unimplemented\n");
	*result = 0xdeadcafe;
	return TRUE;
}

void
nv50_init_query_functions(struct nv50_context *nv50)
{
	nv50->pipe.create_query = nv50_query_create;
	nv50->pipe.destroy_query = nv50_query_destroy;
	nv50->pipe.begin_query = nv50_query_begin;
	nv50->pipe.end_query = nv50_query_end;
	nv50->pipe.get_query_result = nv50_query_result;
}
