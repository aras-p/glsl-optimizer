/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

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


#include "fd3_context.h"
#include "fd3_blend.h"
#include "fd3_draw.h"
#include "fd3_emit.h"
#include "fd3_gmem.h"
#include "fd3_program.h"
#include "fd3_rasterizer.h"
#include "fd3_texture.h"
#include "fd3_zsa.h"

static void
fd3_context_destroy(struct pipe_context *pctx)
{
	struct fd3_context *fd3_ctx = fd3_context(fd_context(pctx));

	fd3_prog_fini(pctx);

	fd_bo_del(fd3_ctx->vs_pvt_mem);
	fd_bo_del(fd3_ctx->fs_pvt_mem);
	fd_bo_del(fd3_ctx->vsc_size_mem);
	fd_bo_del(fd3_ctx->vsc_pipe_mem);

	pipe_resource_reference(&fd3_ctx->solid_vbuf, NULL);
	pipe_resource_reference(&fd3_ctx->blit_texcoord_vbuf, NULL);

	fd_context_destroy(pctx);
}

/* TODO we could combine a few of these small buffers (solid_vbuf,
 * blit_texcoord_vbuf, and vsc_size_mem, into a single buffer and
 * save a tiny bit of memory
 */

static struct pipe_resource *
create_solid_vertexbuf(struct pipe_context *pctx)
{
	static const float init_shader_const[] = {
			-1.000000, +1.000000, +1.000000,
			+1.000000, -1.000000, +1.000000,
	};
	struct pipe_resource *prsc = pipe_buffer_create(pctx->screen,
			PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE, sizeof(init_shader_const));
	pipe_buffer_write(pctx, prsc, 0,
			sizeof(init_shader_const), init_shader_const);
	return prsc;
}

static struct pipe_resource *
create_blit_texcoord_vertexbuf(struct pipe_context *pctx)
{
	struct pipe_resource *prsc = pipe_buffer_create(pctx->screen,
			PIPE_BIND_CUSTOM, PIPE_USAGE_DYNAMIC, 16);
	return prsc;
}

struct pipe_context *
fd3_context_create(struct pipe_screen *pscreen, void *priv)
{
	struct fd_screen *screen = fd_screen(pscreen);
	struct fd3_context *fd3_ctx = CALLOC_STRUCT(fd3_context);
	struct pipe_context *pctx;

	if (!fd3_ctx)
		return NULL;

	pctx = &fd3_ctx->base.base;

	fd3_ctx->base.screen = fd_screen(pscreen);

	pctx->destroy = fd3_context_destroy;
	pctx->create_blend_state = fd3_blend_state_create;
	pctx->create_rasterizer_state = fd3_rasterizer_state_create;
	pctx->create_depth_stencil_alpha_state = fd3_zsa_state_create;

	fd3_draw_init(pctx);
	fd3_gmem_init(pctx);
	fd3_texture_init(pctx);
	fd3_prog_init(pctx);

	pctx = fd_context_init(&fd3_ctx->base, pscreen, priv);
	if (!pctx)
		return NULL;

	fd3_ctx->vs_pvt_mem = fd_bo_new(screen->dev, 0x2000,
			DRM_FREEDRENO_GEM_TYPE_KMEM);

	fd3_ctx->fs_pvt_mem = fd_bo_new(screen->dev, 0x2000,
			DRM_FREEDRENO_GEM_TYPE_KMEM);

	fd3_ctx->vsc_size_mem = fd_bo_new(screen->dev, 0x1000,
			DRM_FREEDRENO_GEM_TYPE_KMEM);

	fd3_ctx->vsc_pipe_mem = fd_bo_new(screen->dev, 0x40000,
			DRM_FREEDRENO_GEM_TYPE_KMEM);

	fd3_ctx->solid_vbuf = create_solid_vertexbuf(pctx);
	fd3_ctx->blit_texcoord_vbuf = create_blit_texcoord_vertexbuf(pctx);

	return pctx;
}
