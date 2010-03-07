/*
 * Copyright (C) 2009 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_context.h"

static GLboolean
nouveau_bo_marker_emit(GLcontext *ctx, struct nouveau_bo_marker *m,
		       uint32_t flags)
{
	struct nouveau_channel *chan = context_chan(ctx);
	uint32_t packet;

	if (m->gr->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_grobj_autobind(m->gr);

	if (MARK_RING(chan, 2, 2))
		return GL_FALSE;

	packet = (m->gr->subc << 13) | (1 << 18) | m->mthd;

	if (flags) {
		if (nouveau_pushbuf_emit_reloc(chan, chan->cur++, m->bo,
					       packet, 0, flags |
					       (m->flags & (NOUVEAU_BO_VRAM |
							    NOUVEAU_BO_GART |
							    NOUVEAU_BO_RDWR)),
					       0, 0))
			goto fail;
	} else {
		*(chan->cur++) = packet;
	}

	if (nouveau_pushbuf_emit_reloc(chan, chan->cur++, m->bo, m->data,
				       m->data2, flags | m->flags,
				       m->vor, m->tor))
		goto fail;

	return GL_TRUE;

fail:
	MARK_UNDO(chan);
	return GL_FALSE;
}

static GLboolean
nouveau_bo_context_grow(struct nouveau_bo_context *bctx)
{
	struct nouveau_bo_marker *marker = bctx->marker;
	int allocated = bctx->allocated + 1;

	marker = realloc(marker, allocated * sizeof(struct nouveau_bo_marker));
	if (!marker)
		return GL_FALSE;

	bctx->marker = marker;
	bctx->allocated = allocated;

	return GL_TRUE;
}

GLboolean
nouveau_bo_mark(struct nouveau_bo_context *bctx, struct nouveau_grobj *gr,
		uint32_t mthd, struct nouveau_bo *bo,
		uint32_t data, uint32_t data2, uint32_t vor, uint32_t tor,
		uint32_t flags)
{
	struct nouveau_bo_state *s = &to_nouveau_context(bctx->ctx)->bo;
	struct nouveau_bo_marker *m;

	if (bctx->count == bctx->allocated) {
		if (!nouveau_bo_context_grow(bctx))
			goto fail;
	}

	m = &bctx->marker[bctx->count];

	*m = (struct nouveau_bo_marker) {
		.gr = gr,
		.mthd = mthd,
		.data = data,
		.data2 = data2,
		.vor = vor,
		.tor = tor,
		.flags = flags,
	};
	nouveau_bo_ref(bo, &m->bo);

	s->count++;
	bctx->count++;

	if (!nouveau_bo_marker_emit(bctx->ctx, m, 0))
		goto fail;

	return GL_TRUE;

fail:
	nouveau_bo_context_reset(bctx);
	return GL_FALSE;
}

void
nouveau_bo_context_reset(struct nouveau_bo_context *bctx)
{
	struct nouveau_bo_state *s = &to_nouveau_context(bctx->ctx)->bo;
	int i;

	for (i = 0; i < bctx->count; i++)
		nouveau_bo_ref(NULL, &bctx->marker[i].bo);

	s->count -= bctx->count;
	bctx->count = 0;
}

GLboolean
nouveau_bo_state_emit(GLcontext *ctx)
{
	struct nouveau_bo_state *s = &to_nouveau_context(ctx)->bo;
	int i, j;

	for (i = 0; i < NUM_NOUVEAU_BO_CONTEXT; i++) {
		struct nouveau_bo_context *bctx = &s->context[i];

		for (j = 0; j < bctx->count; j++) {
			if (!nouveau_bo_marker_emit(ctx, &bctx->marker[j],
						    NOUVEAU_BO_DUMMY))
				return GL_FALSE;
		}
	}

	return GL_TRUE;
}

void
nouveau_bo_state_init(GLcontext *ctx)
{
	struct nouveau_bo_state *s = &to_nouveau_context(ctx)->bo;
	int i;

	for (i = 0; i < NUM_NOUVEAU_BO_CONTEXT; i++)
		s->context[i].ctx = ctx;
}

void
nouveau_bo_state_destroy(GLcontext *ctx)
{
	struct nouveau_bo_state *s = &to_nouveau_context(ctx)->bo;
	int i, j;

	for (i = 0; i < NUM_NOUVEAU_BO_CONTEXT; i++) {
		struct nouveau_bo_context *bctx = &s->context[i];

		for (j = 0; j < bctx->count; j++)
			nouveau_bo_ref(NULL, &bctx->marker[j].bo);

		if (bctx->marker)
			free(bctx->marker);
	}
}
