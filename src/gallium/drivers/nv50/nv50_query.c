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
#include "util/u_inlines.h"

#include "nv50_context.h"

struct nv50_query {
	struct nouveau_bo *bo;
	unsigned type;
	boolean ready;
	uint64_t result;
};

static INLINE struct nv50_query *
nv50_query(struct pipe_query *pipe)
{
	return (struct nv50_query *)pipe;
}

static struct pipe_query *
nv50_query_create(struct pipe_context *pipe, unsigned type)
{
	struct nouveau_device *dev = nouveau_screen(pipe->screen)->device;
	struct nv50_query *q = CALLOC_STRUCT(nv50_query);
	int ret;

	assert (q->type == PIPE_QUERY_OCCLUSION_COUNTER);
	q->type = type;

	ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 256,
			     16, &q->bo);
	if (ret) {
		FREE(q);
		return NULL;
	}

	return (struct pipe_query *)q;
}

static void
nv50_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv50_query *q = nv50_query(pq);

	if (q) {
		nouveau_bo_ref(NULL, &q->bo);
		FREE(q);
	}
}

static void
nv50_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_query *q = nv50_query(pq);

	BEGIN_RING(chan, tesla, NV50TCL_SAMPLECNT_RESET, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, tesla, NV50TCL_SAMPLECNT_ENABLE, 1);
	OUT_RING  (chan, 1);

	q->ready = FALSE;
}

static void
nv50_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_query *q = nv50_query(pq);

	MARK_RING (chan, 5, 2); /* flush on lack of space or relocs */
	BEGIN_RING(chan, tesla, NV50TCL_QUERY_ADDRESS_HIGH, 4);
	OUT_RELOCh(chan, q->bo, 0, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, q->bo, 0, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
	OUT_RING  (chan, 0x00000000);
	OUT_RING  (chan, 0x0100f002);

	BEGIN_RING(chan, tesla, NV50TCL_SAMPLECNT_ENABLE, 1);
	OUT_RING  (chan, 0);
}

static boolean
nv50_query_result(struct pipe_context *pipe, struct pipe_query *pq,
		  boolean wait, uint64_t *result)
{
	struct nv50_query *q = nv50_query(pq);
	int ret;

	if (!q->ready) {
		ret = nouveau_bo_map(q->bo, NOUVEAU_BO_RD |
				     (wait ? 0 : NOUVEAU_BO_NOWAIT));
		if (ret)
			return false;
		q->result = ((uint32_t *)q->bo->map)[1];
		q->ready = TRUE;
		nouveau_bo_unmap(q->bo);
	}

	*result = q->result;
	return q->ready;
}

static void
nv50_render_condition(struct pipe_context *pipe,
		      struct pipe_query *pq, uint mode)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_query *q;

	if (!pq) {
		BEGIN_RING(chan, tesla, NV50TCL_COND_MODE, 1);
		OUT_RING  (chan, NV50TCL_COND_MODE_ALWAYS);
		return;
	}
	q = nv50_query(pq);

	if (mode == PIPE_RENDER_COND_WAIT ||
	    mode == PIPE_RENDER_COND_BY_REGION_WAIT) {
		/* XXX: big fence, FIFO semaphore might be better */
		BEGIN_RING(chan, tesla, 0x0110, 1);
		OUT_RING  (chan, 0);
	}

	BEGIN_RING(chan, tesla, NV50TCL_COND_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, q->bo, 0, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, q->bo, 0, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
	OUT_RING  (chan, NV50TCL_COND_MODE_RES);
}

void
nv50_init_query_functions(struct nv50_context *nv50)
{
	nv50->pipe.create_query = nv50_query_create;
	nv50->pipe.destroy_query = nv50_query_destroy;
	nv50->pipe.begin_query = nv50_query_begin;
	nv50->pipe.end_query = nv50_query_end;
	nv50->pipe.get_query_result = nv50_query_result;
	nv50->pipe.render_condition = nv50_render_condition;
}
