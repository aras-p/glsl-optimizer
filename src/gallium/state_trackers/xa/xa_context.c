/**********************************************************
 * Copyright 2009-2011 VMware, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *********************************************************
 * Authors:
 * Zack Rusin <zackr-at-vmware-dot-com>
 * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
 */
#include "xa_context.h"
#include "xa_priv.h"
#include "cso_cache/cso_context.h"
#include "util/u_inlines.h"
#include "util/u_rect.h"
#include "pipe/p_context.h"


struct xa_context *
xa_context_default(struct xa_tracker *xa)
{
    return xa->default_ctx;
}

struct xa_context *
xa_context_create(struct xa_tracker *xa)
{
    struct xa_context *ctx = calloc(1, sizeof(*ctx));

    ctx->xa = xa;
    ctx->pipe = xa->screen->context_create(xa->screen, NULL);
    ctx->cso = cso_create_context(ctx->pipe);
    ctx->shaders = xa_shaders_create(ctx);
    renderer_init_state(ctx);

    return ctx;
}

void
xa_context_destroy(struct xa_context *r)
{
    struct pipe_resource **vsbuf = &r->vs_const_buffer;
    struct pipe_resource **fsbuf = &r->fs_const_buffer;

    if (*vsbuf)
	pipe_resource_reference(vsbuf, NULL);

    if (*fsbuf)
	pipe_resource_reference(fsbuf, NULL);

    if (r->shaders) {
	xa_shaders_destroy(r->shaders);
	r->shaders = NULL;
    }

    if (r->cso) {
	cso_release_all(r->cso);
	cso_destroy_context(r->cso);
	r->cso = NULL;
    }
}

int
xa_surface_dma(struct xa_context *ctx,
	       struct xa_surface *srf,
	       void *data,
	       unsigned int pitch,
	       int to_surface, struct xa_box *boxes, unsigned int num_boxes)
{
    struct pipe_transfer *transfer;
    void *map;
    int w, h, i;
    enum pipe_transfer_usage transfer_direction;
    struct pipe_context *pipe = ctx->pipe;

    transfer_direction = (to_surface ? PIPE_TRANSFER_WRITE :
			  PIPE_TRANSFER_READ);

    for (i = 0; i < num_boxes; ++i, ++boxes) {
	w = boxes->x2 - boxes->x1;
	h = boxes->y2 - boxes->y1;

	transfer = pipe_get_transfer(pipe, srf->tex, 0, 0,
				     transfer_direction, boxes->x1, boxes->y1,
				     w, h);
	if (!transfer)
	    return -XA_ERR_NORES;

	map = pipe_transfer_map(ctx->pipe, transfer);
	if (!map)
	    goto out_no_map;

	if (to_surface) {
	    util_copy_rect(map, srf->tex->format, transfer->stride,
			   0, 0, w, h, data, pitch, boxes->x1, boxes->y1);
	} else {
	    util_copy_rect(data, srf->tex->format, pitch,
			   boxes->x1, boxes->y1, w, h, map, transfer->stride, 0,
			   0);
	}
	pipe->transfer_unmap(pipe, transfer);
	pipe->transfer_destroy(pipe, transfer);
	if (to_surface)
	    pipe->flush(pipe, &ctx->last_fence);
    }
    return XA_ERR_NONE;
 out_no_map:
    pipe->transfer_destroy(pipe, transfer);
    return -XA_ERR_NORES;
}

void *
xa_surface_map(struct xa_context *ctx,
	       struct xa_surface *srf, unsigned int usage)
{
    void *map;
    unsigned int transfer_direction = 0;
    struct pipe_context *pipe = ctx->pipe;

    if (srf->transfer)
	return NULL;

    if (usage & XA_MAP_READ)
	transfer_direction = PIPE_TRANSFER_READ;
    if (usage & XA_MAP_WRITE)
	transfer_direction = PIPE_TRANSFER_WRITE;

    if (!transfer_direction)
	return NULL;

    srf->transfer = pipe_get_transfer(pipe, srf->tex, 0, 0,
				      transfer_direction, 0, 0,
				      srf->tex->width0, srf->tex->height0);
    if (!srf->transfer)
	return NULL;

    map = pipe_transfer_map(pipe, srf->transfer);
    if (!map)
	pipe->transfer_destroy(pipe, srf->transfer);

    srf->mapping_pipe = pipe;
    return map;
}

void
xa_surface_unmap(struct xa_surface *srf)
{
    if (srf->transfer) {
	struct pipe_context *pipe = srf->mapping_pipe;

	pipe->transfer_unmap(pipe, srf->transfer);
	pipe->transfer_destroy(pipe, srf->transfer);
	srf->transfer = NULL;
    }
}

int
xa_copy_prepare(struct xa_context *ctx,
		struct xa_surface *dst, struct xa_surface *src)
{
    if (src == dst || src->tex->format != dst->tex->format)
	return -XA_ERR_INVAL;

    ctx->src = src;
    ctx->dst = dst;

    return 0;
}

void
xa_copy(struct xa_context *ctx,
	int dx, int dy, int sx, int sy, int width, int height)
{
    struct pipe_box src_box;

    u_box_2d(sx, sy, width, height, &src_box);
    ctx->pipe->resource_copy_region(ctx->pipe,
				    ctx->dst->tex, 0, dx, dy, 0, ctx->src->tex,
				    0, &src_box);

}

void
xa_copy_done(struct xa_context *ctx)
{
    ctx->pipe->flush(ctx->pipe, &ctx->last_fence);
}

struct xa_fence *
xa_fence_get(struct xa_context *ctx)
{
    struct xa_fence *fence = malloc(sizeof(*fence));
    struct pipe_screen *screen = ctx->xa->screen;

    if (!fence)
	return NULL;

    fence->xa = ctx->xa;

    if (ctx->last_fence == NULL)
	fence->pipe_fence = NULL;
    else
	screen->fence_reference(screen, &fence->pipe_fence, ctx->last_fence);

    return fence;
}

int
xa_fence_wait(struct xa_fence *fence, uint64_t timeout)
{
    if (!fence)
	return XA_ERR_NONE;

    if (fence->pipe_fence) {
	struct pipe_screen *screen = fence->xa->screen;
	boolean timed_out;

	timed_out = !screen->fence_finish(screen, fence->pipe_fence, timeout);
	if (timed_out)
	    return -XA_ERR_BUSY;

	screen->fence_reference(screen, &fence->pipe_fence, NULL);
    }
    return XA_ERR_NONE;
}

void
xa_fence_destroy(struct xa_fence *fence)
{
    if (!fence)
	return;

    if (fence->pipe_fence) {
	struct pipe_screen *screen = fence->xa->screen;

	screen->fence_reference(screen, &fence->pipe_fence, NULL);
    }

    free(fence);
}
