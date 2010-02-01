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

#ifndef __NOUVEAU_BO_STATE_H__
#define __NOUVEAU_BO_STATE_H__

enum {
	NOUVEAU_BO_CONTEXT_FRAMEBUFFER = 0,
	NOUVEAU_BO_CONTEXT_LMA_DEPTH,
	NOUVEAU_BO_CONTEXT_SURFACE,
	NOUVEAU_BO_CONTEXT_TEXTURE0,
	NOUVEAU_BO_CONTEXT_TEXTURE1,
	NOUVEAU_BO_CONTEXT_TEXTURE2,
	NOUVEAU_BO_CONTEXT_TEXTURE3,
	NOUVEAU_BO_CONTEXT_VERTEX,
	NUM_NOUVEAU_BO_CONTEXT
};

struct nouveau_bo_marker {
	struct nouveau_grobj *gr;
	uint32_t mthd;

	struct nouveau_bo *bo;
	uint32_t data;
	uint32_t data2;
	uint32_t vor;
	uint32_t tor;
	uint32_t flags;
};

struct nouveau_bo_context {
	GLcontext *ctx;

	struct nouveau_bo_marker *marker;
	int allocated;
	int count;
};

struct nouveau_bo_state {
	struct nouveau_bo_context context[NUM_NOUVEAU_BO_CONTEXT];
	int count;
};

GLboolean
nouveau_bo_mark(struct nouveau_bo_context *bctx, struct nouveau_grobj *gr,
		uint32_t mthd, struct nouveau_bo *bo,
		uint32_t data, uint32_t data2, uint32_t vor, uint32_t tor,
		uint32_t flags);

#define nouveau_bo_markl(bctx, gr, mthd, bo, data, flags)		\
	nouveau_bo_mark(bctx, gr, mthd, bo, data, 0, 0, 0,		\
			flags | NOUVEAU_BO_LOW);

#define nouveau_bo_marko(bctx, gr, mthd, bo, flags)			\
	nouveau_bo_mark(bctx, gr, mthd, bo, 0, 0,			\
			context_chan(ctx)->vram->handle,		\
			context_chan(ctx)->gart->handle,		\
			flags | NOUVEAU_BO_OR);

void
nouveau_bo_context_reset(struct nouveau_bo_context *bctx);

GLboolean
nouveau_bo_state_emit(GLcontext *ctx);

void
nouveau_bo_state_init(GLcontext *ctx);

void
nouveau_bo_state_destroy(GLcontext *ctx);

#define __context_bctx(ctx, i)						\
	({								\
		struct nouveau_context *nctx = to_nouveau_context(ctx); \
		struct nouveau_bo_context *bctx = &nctx->bo.context[i];	\
		nouveau_bo_context_reset(bctx);				\
		bctx;							\
	})
#define context_bctx(ctx, s) \
	__context_bctx(ctx, NOUVEAU_BO_CONTEXT_##s)
#define context_bctx_i(ctx, s, i) \
	__context_bctx(ctx, NOUVEAU_BO_CONTEXT_##s##0 + (i))

#endif
