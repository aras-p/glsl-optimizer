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
#include <stdio.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include <util/u_blitter.h>
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600d.h"

static void r600_destroy_context(struct pipe_context *context)
{
	struct r600_context *rctx = r600_context(context);

	rctx->rasterizer = r600_context_state_decref(rctx->rasterizer);
	rctx->poly_stipple = r600_context_state_decref(rctx->poly_stipple);
	rctx->scissor = r600_context_state_decref(rctx->scissor);
	rctx->clip = r600_context_state_decref(rctx->clip);
	rctx->ps_shader = r600_context_state_decref(rctx->ps_shader);
	rctx->vs_shader = r600_context_state_decref(rctx->vs_shader);
	rctx->depth = r600_context_state_decref(rctx->depth);
	rctx->stencil = r600_context_state_decref(rctx->stencil);
	rctx->alpha = r600_context_state_decref(rctx->alpha);
	rctx->dsa = r600_context_state_decref(rctx->dsa);
	rctx->blend = r600_context_state_decref(rctx->blend);
	rctx->stencil_ref = r600_context_state_decref(rctx->stencil_ref);
	rctx->viewport = r600_context_state_decref(rctx->viewport);
	rctx->framebuffer = r600_context_state_decref(rctx->framebuffer);

	free(rctx->ps_constant);
	free(rctx->vs_constant);
	free(rctx->vs_resource);

	radeon_ctx_fini(&rctx->ctx);
	FREE(rctx);
}

void r600_flush(struct pipe_context *ctx, unsigned flags,
			struct pipe_fence_handle **fence)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *rquery;
	static int dc = 0;
	char dname[256];

	/* suspend queries */
	r600_queries_suspend(ctx);
	/* FIXME dumping should be removed once shader support instructions
	 * without throwing bad code
	 */
	if (!rctx->ctx.cdwords)
		goto out;
#if 0
	sprintf(dname, "gallium-%08d.bof", dc);
	if (dc < 2) {
		radeon_ctx_dump_bof(&rctx->ctx, dname);
		R600_ERR("dumped %s\n", dname);
	}
#endif
#if 1
	radeon_ctx_submit(&rctx->ctx);
#endif
	LIST_FOR_EACH_ENTRY(rquery, &rctx->query_list, list) {
		rquery->flushed = true;
	}
	dc++;
out:
	radeon_ctx_clear(&rctx->ctx);
	/* resume queries */
	r600_queries_resume(ctx);
}

struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv)
{
	struct r600_context *rctx = CALLOC_STRUCT(r600_context);
	struct r600_screen* rscreen = r600_screen(screen);

	if (rctx == NULL)
		return NULL;
	rctx->context.winsys = rscreen->screen.winsys;
	rctx->context.screen = screen;
	rctx->context.priv = priv;
	rctx->context.destroy = r600_destroy_context;
	rctx->context.draw_vbo = r600_draw_vbo;
	rctx->context.flush = r600_flush;

	/* Easy accessing of screen/winsys. */
	rctx->screen = rscreen;
	rctx->rw = rscreen->rw;

	if (rscreen->chip_class == EVERGREEN)
		rctx->vtbl = &eg_hw_state_vtbl;
	else
		rctx->vtbl = &r600_hw_state_vtbl;

	r600_init_blit_functions(rctx);
	r600_init_query_functions(rctx);
	r600_init_state_functions(rctx);
	r600_init_context_resource_functions(rctx);

	rctx->blitter = util_blitter_create(&rctx->context);
	if (rctx->blitter == NULL) {
		FREE(rctx);
		return NULL;
	}

	rctx->vtbl->init_config(rctx);

	rctx->vs_constant = (struct radeon_state *)calloc(R600_MAX_CONSTANT, sizeof(struct radeon_state));
	if (!rctx->vs_constant) {
		FREE(rctx);
		return NULL;
	}

	rctx->ps_constant = (struct radeon_state *)calloc(R600_MAX_CONSTANT, sizeof(struct radeon_state));
	if (!rctx->ps_constant) {
		FREE(rctx);
		return NULL;
	}

	rctx->vs_resource = (struct radeon_state *)calloc(R600_MAX_RESOURCE, sizeof(struct radeon_state));
	if (!rctx->vs_resource) {
		FREE(rctx);
		return NULL;
	}						   

	radeon_ctx_init(&rctx->ctx, rscreen->rw);
	radeon_draw_init(&rctx->draw, rscreen->rw);
	return &rctx->context;
}
