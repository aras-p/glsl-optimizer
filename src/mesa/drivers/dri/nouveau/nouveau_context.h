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

#ifndef __NOUVEAU_CONTEXT_H__
#define __NOUVEAU_CONTEXT_H__

#include "nouveau_screen.h"
#include "nouveau_state.h"
#include "nouveau_bo_state.h"
#include "nouveau_render.h"

#include "main/bitset.h"

enum nouveau_fallback {
	HWTNL = 0,
	SWTNL,
	SWRAST,
};

struct nouveau_hw_state {
	struct nouveau_channel *chan;

	struct nouveau_notifier *ntfy;
	struct nouveau_grobj *eng3d;
	struct nouveau_grobj *eng3dm;
	struct nouveau_grobj *surf3d;
	struct nouveau_grobj *m2mf;
	struct nouveau_grobj *surf2d;
	struct nouveau_grobj *rop;
	struct nouveau_grobj *patt;
	struct nouveau_grobj *rect;
	struct nouveau_grobj *swzsurf;
	struct nouveau_grobj *sifm;
};

struct nouveau_context {
	GLcontext base;
	__DRIcontext *dri_context;
	struct nouveau_screen *screen;

	BITSET_DECLARE(dirty, MAX_NOUVEAU_STATE);
	enum nouveau_fallback fallback;

	struct nouveau_hw_state hw;
	struct nouveau_bo_state bo;
	struct nouveau_render_state render;
};

#define to_nouveau_context(ctx)	((struct nouveau_context *)(ctx))

#define context_dev(ctx) \
	(to_nouveau_context(ctx)->screen->device)
#define context_chipset(ctx) \
	(context_dev(ctx)->chipset)
#define context_chan(ctx) \
	(to_nouveau_context(ctx)->hw.chan)
#define context_eng3d(ctx) \
	(to_nouveau_context(ctx)->hw.eng3d)
#define context_drv(ctx) \
	(to_nouveau_context(ctx)->screen->driver)
#define context_dirty(ctx, s) \
	BITSET_SET(to_nouveau_context(ctx)->dirty, NOUVEAU_STATE_##s)
#define context_dirty_i(ctx, s, i) \
	BITSET_SET(to_nouveau_context(ctx)->dirty, NOUVEAU_STATE_##s##0 + i)

GLboolean
nouveau_context_create(const __GLcontextModes *visual, __DRIcontext *dri_ctx,
		       void *share_ctx);

GLboolean
nouveau_context_init(GLcontext *ctx, struct nouveau_screen *screen,
		     const GLvisual *visual, GLcontext *share_ctx);

void
nouveau_context_deinit(GLcontext *ctx);

void
nouveau_context_destroy(__DRIcontext *dri_ctx);

void
nouveau_update_renderbuffers(__DRIcontext *dri_ctx, __DRIdrawable *draw);

GLboolean
nouveau_context_make_current(__DRIcontext *dri_ctx, __DRIdrawable *ddraw,
			     __DRIdrawable *rdraw);

GLboolean
nouveau_context_unbind(__DRIcontext *dri_ctx);

void
nouveau_fallback(GLcontext *ctx, enum nouveau_fallback mode);

void
nouveau_validate_framebuffer(GLcontext *ctx);

#endif

