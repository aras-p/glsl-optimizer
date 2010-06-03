/*
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
 *      Marek Olšák
 */
#include <pipe/p_screen.h>
#include <util/u_blitter.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include "r600_screen.h"
#include "r600_context.h"

static void r600_blitter_save_states(struct pipe_context *ctx)
{
	struct r600_context *rctx = (struct r600_context*)ctx;

	util_blitter_save_blend(rctx->blitter,
					rctx->draw->state[R600_BLEND]);
	util_blitter_save_depth_stencil_alpha(rctx->blitter,
					rctx->draw->state[R600_DSA]);
	util_blitter_save_stencil_ref(rctx->blitter, &rctx->stencil_ref);
	util_blitter_save_rasterizer(rctx->blitter,
					rctx->draw->state[R600_RASTERIZER]);
	util_blitter_save_fragment_shader(rctx->blitter,
					rctx->ps_shader);
	util_blitter_save_vertex_shader(rctx->blitter,
					rctx->vs_shader);
	util_blitter_save_vertex_elements(rctx->blitter,
					rctx->vertex_elements);
	util_blitter_save_viewport(rctx->blitter,
					&rctx->viewport);
}

void r600_clear(struct pipe_context *ctx, unsigned buffers,
		const float *rgba, double depth, unsigned stencil)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct pipe_framebuffer_state *fb = &rctx->fb_state;

	r600_blitter_save_states(ctx);
	util_blitter_clear(rctx->blitter, fb->width, fb->height,
				fb->nr_cbufs, buffers, rgba, depth,
				stencil);
}

void r600_clear_render_target(struct pipe_context *pipe,
			      struct pipe_surface *dst,
			      const float *rgba,
			      unsigned dstx, unsigned dsty,
			      unsigned width, unsigned height)
{
}

void r300_clear_depth_stencil(struct pipe_context *pipe,
			      struct pipe_surface *dst,
			      unsigned clear_flags,
			      double depth,
			      unsigned stencil,
			      unsigned dstx, unsigned dsty,
			      unsigned width, unsigned height)
{
}

void r600_resource_copy_region(struct pipe_context *pipe,
			       struct pipe_resource *dst,
			       struct pipe_subresource subdst,
			       unsigned dstx, unsigned dsty, unsigned dstz,
			       struct pipe_resource *src,
			       struct pipe_subresource subsrc,
			       unsigned srcx, unsigned srcy, unsigned srcz,
			       unsigned width, unsigned height)
{
}
