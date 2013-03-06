/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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

#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"
#include "util/u_string.h"
#include "util/u_surface.h"

#include "freedreno_resource.h"
#include "freedreno_screen.h"
#include "freedreno_surface.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

static void *
fd_resource_transfer_map(struct pipe_context *pctx,
		struct pipe_resource *prsc,
		unsigned level, unsigned usage,
		const struct pipe_box *box,
		struct pipe_transfer **pptrans)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_resource *rsc = fd_resource(prsc);
	struct pipe_transfer *ptrans = util_slab_alloc(&ctx->transfer_pool);
	enum pipe_format format = prsc->format;
	char *buf;

	if (!ptrans)
		return NULL;

	ptrans->resource = prsc;
	ptrans->level = level;
	ptrans->usage = usage;
	ptrans->box = *box;
	ptrans->stride = rsc->pitch * rsc->cpp;
	ptrans->layer_stride = ptrans->stride;

	buf = fd_bo_map(rsc->bo);

	*pptrans = ptrans;

	return buf +
		box->y / util_format_get_blockheight(format) * ptrans->stride +
		box->x / util_format_get_blockwidth(format) * rsc->cpp;
}

static void fd_resource_transfer_flush_region(struct pipe_context *pctx,
		struct pipe_transfer *ptrans,
		const struct pipe_box *box)
{
	struct fd_context *ctx = fd_context(pctx);
	struct fd_resource *rsc = fd_resource(ptrans->resource);

	if (rsc->dirty)
		fd_context_render(pctx);

	if (rsc->timestamp) {
		fd_pipe_wait(ctx->screen->pipe, rsc->timestamp);
		rsc->timestamp = 0;
	}
}

static void
fd_resource_transfer_unmap(struct pipe_context *pctx,
		struct pipe_transfer *ptrans)
{
	struct fd_context *ctx = fd_context(pctx);
	util_slab_free(&ctx->transfer_pool, ptrans);
}

static void
fd_resource_destroy(struct pipe_screen *pscreen,
		struct pipe_resource *prsc)
{
	struct fd_resource *rsc = fd_resource(prsc);
	fd_bo_del(rsc->bo);
	FREE(rsc);
}

static boolean
fd_resource_get_handle(struct pipe_screen *pscreen,
		struct pipe_resource *prsc,
		struct winsys_handle *handle)
{
	struct fd_resource *rsc = fd_resource(prsc);

	return fd_screen_bo_get_handle(pscreen, rsc->bo, rsc->pitch, handle);
}


const struct u_resource_vtbl fd_resource_vtbl = {
		.resource_get_handle      = fd_resource_get_handle,
		.resource_destroy         = fd_resource_destroy,
		.transfer_map             = fd_resource_transfer_map,
		.transfer_flush_region    = fd_resource_transfer_flush_region,
		.transfer_unmap           = fd_resource_transfer_unmap,
		.transfer_inline_write    = u_default_transfer_inline_write,
};

/**
 * Create a new texture object, using the given template info.
 */
static struct pipe_resource *
fd_resource_create(struct pipe_screen *pscreen,
		const struct pipe_resource *tmpl)
{
	struct fd_screen *screen = fd_screen(pscreen);
	struct fd_resource *rsc = CALLOC_STRUCT(fd_resource);
	struct pipe_resource *prsc = &rsc->base.b;
	uint32_t flags, size;

	DBG("target=%d, format=%s, %ux%u@%u, array_size=%u, last_level=%u, "
			"nr_samples=%u, usage=%u, bind=%x, flags=%x",
			tmpl->target, util_format_name(tmpl->format),
			tmpl->width0, tmpl->height0, tmpl->depth0,
			tmpl->array_size, tmpl->last_level, tmpl->nr_samples,
			tmpl->usage, tmpl->bind, tmpl->flags);

	if (!rsc)
		return NULL;

	*prsc = *tmpl;

	pipe_reference_init(&prsc->reference, 1);
	prsc->screen = pscreen;

	rsc->base.vtbl = &fd_resource_vtbl;
	rsc->pitch = ALIGN(tmpl->width0, 32);
	rsc->cpp = util_format_get_blocksize(tmpl->format);

	size = rsc->pitch * tmpl->height0 * rsc->cpp;
	flags = DRM_FREEDRENO_GEM_TYPE_KMEM; /* TODO */

	rsc->bo = fd_bo_new(screen->dev, size, flags);

	return prsc;
}

/**
 * Create a texture from a winsys_handle. The handle is often created in
 * another process by first creating a pipe texture and then calling
 * resource_get_handle.
 */
static struct pipe_resource *
fd_resource_from_handle(struct pipe_screen *pscreen,
		const struct pipe_resource *tmpl,
		struct winsys_handle *handle)
{
	struct fd_resource *rsc = CALLOC_STRUCT(fd_resource);
	struct pipe_resource *prsc = &rsc->base.b;

	DBG("target=%d, format=%s, %ux%u@%u, array_size=%u, last_level=%u, "
			"nr_samples=%u, usage=%u, bind=%x, flags=%x",
			tmpl->target, util_format_name(tmpl->format),
			tmpl->width0, tmpl->height0, tmpl->depth0,
			tmpl->array_size, tmpl->last_level, tmpl->nr_samples,
			tmpl->usage, tmpl->bind, tmpl->flags);

	if (!rsc)
		return NULL;

	*prsc = *tmpl;

	pipe_reference_init(&prsc->reference, 1);
	prsc->screen = pscreen;

	rsc->bo = fd_screen_bo_from_handle(pscreen, handle, &rsc->pitch);

	rsc->base.vtbl = &fd_resource_vtbl;
	rsc->pitch = ALIGN(tmpl->width0, 32);

	return prsc;
}

/**
 * Copy a block of pixels from one resource to another.
 * The resource must be of the same format.
 * Resources with nr_samples > 1 are not allowed.
 */
static void
fd_resource_copy_region(struct pipe_context *pctx,
		struct pipe_resource *dst,
		unsigned dst_level,
		unsigned dstx, unsigned dsty, unsigned dstz,
		struct pipe_resource *src,
		unsigned src_level,
		const struct pipe_box *src_box)
{
	DBG("TODO: ");
	// TODO
}

/* Optimal hardware path for blitting pixels.
 * Scaling, format conversion, up- and downsampling (resolve) are allowed.
 */
static void
fd_blit(struct pipe_context *pctx, const struct pipe_blit_info *blit_info)
{
	struct fd_context *ctx = fd_context(pctx);
	struct pipe_blit_info info = *blit_info;

	if (info.src.resource->nr_samples > 1 &&
			info.dst.resource->nr_samples <= 1 &&
			!util_format_is_depth_or_stencil(info.src.resource->format) &&
			!util_format_is_pure_integer(info.src.resource->format)) {
		DBG("color resolve unimplemented");
		return;
	}

	if (util_try_blit_via_copy_region(pctx, &info)) {
		return; /* done */
	}

	if (info.mask & PIPE_MASK_S) {
		DBG("cannot blit stencil, skipping");
		info.mask &= ~PIPE_MASK_S;
	}

	if (!util_blitter_is_blit_supported(ctx->blitter, &info)) {
		DBG("blit unsupported %s -> %s",
				util_format_short_name(info.src.resource->format),
				util_format_short_name(info.dst.resource->format));
		return;
	}

	util_blitter_save_vertex_buffer_slot(ctx->blitter, ctx->vertexbuf.vb);
	util_blitter_save_vertex_elements(ctx->blitter, ctx->vtx);
	util_blitter_save_vertex_shader(ctx->blitter, ctx->prog.vp);
	util_blitter_save_rasterizer(ctx->blitter, ctx->rasterizer);
	util_blitter_save_viewport(ctx->blitter, &ctx->viewport);
	util_blitter_save_scissor(ctx->blitter, &ctx->scissor);
	util_blitter_save_fragment_shader(ctx->blitter, ctx->prog.fp);
	util_blitter_save_blend(ctx->blitter, ctx->blend);
	util_blitter_save_depth_stencil_alpha(ctx->blitter, ctx->zsa);
	util_blitter_save_stencil_ref(ctx->blitter, &ctx->stencil_ref);
	util_blitter_save_sample_mask(ctx->blitter, ctx->sample_mask);
	util_blitter_save_framebuffer(ctx->blitter, &ctx->framebuffer);
	util_blitter_save_fragment_sampler_states(ctx->blitter,
			ctx->fragtex.num_samplers,
			(void **)ctx->fragtex.samplers);
	util_blitter_save_fragment_sampler_views(ctx->blitter,
			ctx->fragtex.num_textures, ctx->fragtex.textures);

	util_blitter_blit(ctx->blitter, &info);
}

void
fd_resource_screen_init(struct pipe_screen *pscreen)
{
	pscreen->resource_create = fd_resource_create;
	pscreen->resource_from_handle = fd_resource_from_handle;
	pscreen->resource_get_handle = u_resource_get_handle_vtbl;
	pscreen->resource_destroy = u_resource_destroy_vtbl;
}

void
fd_resource_context_init(struct pipe_context *pctx)
{
	pctx->transfer_map = u_transfer_map_vtbl;
	pctx->transfer_flush_region = u_transfer_flush_region_vtbl;
	pctx->transfer_unmap = u_transfer_unmap_vtbl;
	pctx->transfer_inline_write = u_transfer_inline_write_vtbl;
	pctx->create_surface = fd_create_surface;
	pctx->surface_destroy = fd_surface_destroy;
	pctx->resource_copy_region = fd_resource_copy_region;
	pctx->blit = fd_blit;
}
