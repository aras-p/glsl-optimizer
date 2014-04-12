/*
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 *      Marek Olšák
 */

#include "r600_cs.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"
#include <inttypes.h>
#include <stdio.h>

boolean r600_rings_is_buffer_referenced(struct r600_common_context *ctx,
					struct radeon_winsys_cs_handle *buf,
					enum radeon_bo_usage usage)
{
	if (ctx->ws->cs_is_buffer_referenced(ctx->rings.gfx.cs, buf, usage)) {
		return TRUE;
	}
	if (ctx->rings.dma.cs && ctx->rings.dma.cs->cdw &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.dma.cs, buf, usage)) {
		return TRUE;
	}
	return FALSE;
}

void *r600_buffer_map_sync_with_rings(struct r600_common_context *ctx,
                                      struct r600_resource *resource,
                                      unsigned usage)
{
	enum radeon_bo_usage rusage = RADEON_USAGE_READWRITE;
	bool busy = false;

	if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
		return ctx->ws->buffer_map(resource->cs_buf, NULL, usage);
	}

	if (!(usage & PIPE_TRANSFER_WRITE)) {
		/* have to wait for the last write */
		rusage = RADEON_USAGE_WRITE;
	}

	if (ctx->rings.gfx.cs->cdw != ctx->initial_gfx_cs_size &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.gfx.cs,
					     resource->cs_buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			ctx->rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
			return NULL;
		} else {
			ctx->rings.gfx.flush(ctx, 0, NULL);
			busy = true;
		}
	}
	if (ctx->rings.dma.cs &&
	    ctx->rings.dma.cs->cdw &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.dma.cs,
					     resource->cs_buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			ctx->rings.dma.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
			return NULL;
		} else {
			ctx->rings.dma.flush(ctx, 0, NULL);
			busy = true;
		}
	}

	if (busy || ctx->ws->buffer_is_busy(resource->buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			return NULL;
		} else {
			/* We will be wait for the GPU. Wait for any offloaded
			 * CS flush to complete to avoid busy-waiting in the winsys. */
			ctx->ws->cs_sync_flush(ctx->rings.gfx.cs);
			if (ctx->rings.dma.cs)
				ctx->ws->cs_sync_flush(ctx->rings.dma.cs);
		}
	}

	/* Setting the CS to NULL will prevent doing checks we have done already. */
	return ctx->ws->buffer_map(resource->cs_buf, NULL, usage);
}

bool r600_init_resource(struct r600_common_screen *rscreen,
			struct r600_resource *res,
			unsigned size, unsigned alignment,
			bool use_reusable_pool)
{
	struct r600_texture *rtex = (struct r600_texture*)res;
	struct pb_buffer *old_buf, *new_buf;

	switch (res->b.b.usage) {
	case PIPE_USAGE_STAGING:
	case PIPE_USAGE_DYNAMIC:
	case PIPE_USAGE_STREAM:
		/* Transfers are likely to occur more often with these resources. */
		res->domains = RADEON_DOMAIN_GTT;
		break;
	case PIPE_USAGE_DEFAULT:
	case PIPE_USAGE_IMMUTABLE:
	default:
		/* Not listing GTT here improves performance in some apps. */
		res->domains = RADEON_DOMAIN_VRAM;
		break;
	}

	/* Use GTT for all persistent mappings, because they are
	 * always cached and coherent. */
	if (res->b.b.target == PIPE_BUFFER &&
	    res->b.b.flags & (PIPE_RESOURCE_FLAG_MAP_PERSISTENT |
			      PIPE_RESOURCE_FLAG_MAP_COHERENT)) {
		res->domains = RADEON_DOMAIN_GTT;
	}

	/* Tiled textures are unmappable. Always put them in VRAM. */
	if (res->b.b.target != PIPE_BUFFER &&
	    rtex->surface.level[0].mode >= RADEON_SURF_MODE_1D) {
		res->domains = RADEON_DOMAIN_VRAM;
	}

	/* Allocate a new resource. */
	new_buf = rscreen->ws->buffer_create(rscreen->ws, size, alignment,
					     use_reusable_pool,
					     res->domains);
	if (!new_buf) {
		return false;
	}

	/* Replace the pointer such that if res->buf wasn't NULL, it won't be
	 * NULL. This should prevent crashes with multiple contexts using
	 * the same buffer where one of the contexts invalidates it while
	 * the others are using it. */
	old_buf = res->buf;
	res->cs_buf = rscreen->ws->buffer_get_cs_handle(new_buf); /* should be atomic */
	res->buf = new_buf; /* should be atomic */
	pb_reference(&old_buf, NULL);

	util_range_set_empty(&res->valid_buffer_range);

	if (rscreen->debug_flags & DBG_VM && res->b.b.target == PIPE_BUFFER) {
		fprintf(stderr, "VM start=0x%"PRIu64"  end=0x%"PRIu64" | Buffer %u bytes\n",
			r600_resource_va(&rscreen->b, &res->b.b),
			r600_resource_va(&rscreen->b, &res->b.b) + res->buf->size,
			res->buf->size);
	}
	return true;
}

static void r600_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
	struct r600_resource *rbuffer = r600_resource(buf);

	util_range_destroy(&rbuffer->valid_buffer_range);
	pb_reference(&rbuffer->buf, NULL);
	FREE(rbuffer);
}

static void *r600_buffer_get_transfer(struct pipe_context *ctx,
				      struct pipe_resource *resource,
                                      unsigned level,
                                      unsigned usage,
                                      const struct pipe_box *box,
				      struct pipe_transfer **ptransfer,
				      void *data, struct r600_resource *staging,
				      unsigned offset)
{
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct r600_transfer *transfer = util_slab_alloc(&rctx->pool_transfers);

	transfer->transfer.resource = resource;
	transfer->transfer.level = level;
	transfer->transfer.usage = usage;
	transfer->transfer.box = *box;
	transfer->transfer.stride = 0;
	transfer->transfer.layer_stride = 0;
	transfer->offset = offset;
	transfer->staging = staging;
	*ptransfer = &transfer->transfer;
	return data;
}

static bool r600_can_dma_copy_buffer(struct r600_common_context *rctx,
				     unsigned dstx, unsigned srcx, unsigned size)
{
	bool dword_aligned = !(dstx % 4) && !(srcx % 4) && !(size % 4);

	return rctx->screen->has_cp_dma ||
	       (dword_aligned && (rctx->rings.dma.cs ||
				  rctx->screen->has_streamout));

}

static void *r600_buffer_transfer_map(struct pipe_context *ctx,
                                      struct pipe_resource *resource,
                                      unsigned level,
                                      unsigned usage,
                                      const struct pipe_box *box,
                                      struct pipe_transfer **ptransfer)
{
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct r600_common_screen *rscreen = (struct r600_common_screen*)ctx->screen;
        struct r600_resource *rbuffer = r600_resource(resource);
        uint8_t *data;

	assert(box->x + box->width <= resource->width0);

	/* See if the buffer range being mapped has never been initialized,
	 * in which case it can be mapped unsynchronized. */
	if (!(usage & PIPE_TRANSFER_UNSYNCHRONIZED) &&
	    usage & PIPE_TRANSFER_WRITE &&
	    !util_ranges_intersect(&rbuffer->valid_buffer_range, box->x, box->x + box->width)) {
		usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
	}

	/* If discarding the entire range, discard the whole resource instead. */
	if (usage & PIPE_TRANSFER_DISCARD_RANGE &&
	    box->x == 0 && box->width == resource->width0) {
		usage |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
	}

	if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE &&
	    !(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
		assert(usage & PIPE_TRANSFER_WRITE);

		/* Check if mapping this buffer would cause waiting for the GPU. */
		if (r600_rings_is_buffer_referenced(rctx, rbuffer->cs_buf, RADEON_USAGE_READWRITE) ||
		    rctx->ws->buffer_is_busy(rbuffer->buf, RADEON_USAGE_READWRITE)) {
			rctx->invalidate_buffer(&rctx->b, &rbuffer->b.b);
		}
		/* At this point, the buffer is always idle. */
		usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
	}
	else if ((usage & PIPE_TRANSFER_DISCARD_RANGE) &&
		 !(usage & PIPE_TRANSFER_UNSYNCHRONIZED) &&
		 !(rscreen->debug_flags & DBG_NO_DISCARD_RANGE) &&
		 r600_can_dma_copy_buffer(rctx, box->x, 0, box->width)) {
		assert(usage & PIPE_TRANSFER_WRITE);

		/* Check if mapping this buffer would cause waiting for the GPU. */
		if (r600_rings_is_buffer_referenced(rctx, rbuffer->cs_buf, RADEON_USAGE_READWRITE) ||
		    rctx->ws->buffer_is_busy(rbuffer->buf, RADEON_USAGE_READWRITE)) {
			/* Do a wait-free write-only transfer using a temporary buffer. */
			unsigned offset;
			struct r600_resource *staging = NULL;

			u_upload_alloc(rctx->uploader, 0, box->width + (box->x % R600_MAP_BUFFER_ALIGNMENT),
				       &offset, (struct pipe_resource**)&staging, (void**)&data);

			if (staging) {
				data += box->x % R600_MAP_BUFFER_ALIGNMENT;
				return r600_buffer_get_transfer(ctx, resource, level, usage, box,
								ptransfer, data, staging, offset);
			} else {
				return NULL; /* error, shouldn't occur though */
			}
		}
		/* At this point, the buffer is always idle (we checked it above). */
		usage |= PIPE_TRANSFER_UNSYNCHRONIZED;
	}
	/* Using a staging buffer in GTT for larger reads is much faster. */
	else if ((usage & PIPE_TRANSFER_READ) &&
		 !(usage & PIPE_TRANSFER_WRITE) &&
		 rbuffer->domains == RADEON_DOMAIN_VRAM &&
		 r600_can_dma_copy_buffer(rctx, 0, box->x, box->width)) {
		unsigned offset;
		struct r600_resource *staging = NULL;

		u_upload_alloc(rctx->uploader, 0,
			       box->width + (box->x % R600_MAP_BUFFER_ALIGNMENT),
			       &offset, (struct pipe_resource**)&staging, (void**)&data);

		if (staging) {
			data += box->x % R600_MAP_BUFFER_ALIGNMENT;

			/* Copy the VRAM buffer to the staging buffer. */
			rctx->dma_copy(ctx, &staging->b.b, 0,
				       offset + box->x % R600_MAP_BUFFER_ALIGNMENT,
				       0, 0, resource, level, box);

			/* Just do the synchronization. The buffer is mapped already. */
			r600_buffer_map_sync_with_rings(rctx, staging, PIPE_TRANSFER_READ);

			return r600_buffer_get_transfer(ctx, resource, level, usage, box,
							ptransfer, data, staging, offset);
		}
	}

	data = r600_buffer_map_sync_with_rings(rctx, rbuffer, usage);
	if (!data) {
		return NULL;
	}
	data += box->x;

	return r600_buffer_get_transfer(ctx, resource, level, usage, box,
					ptransfer, data, NULL, 0);
}

static void r600_buffer_transfer_unmap(struct pipe_context *ctx,
				       struct pipe_transfer *transfer)
{
	struct r600_common_context *rctx = (struct r600_common_context*)ctx;
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;
	struct r600_resource *rbuffer = r600_resource(transfer->resource);

	if (rtransfer->staging) {
		if (rtransfer->transfer.usage & PIPE_TRANSFER_WRITE) {
			struct pipe_resource *dst, *src;
			unsigned soffset, doffset, size;
			struct pipe_box box;

			dst = transfer->resource;
			src = &rtransfer->staging->b.b;
			size = transfer->box.width;
			doffset = transfer->box.x;
			soffset = rtransfer->offset + transfer->box.x % R600_MAP_BUFFER_ALIGNMENT;

			u_box_1d(soffset, size, &box);

			/* Copy the staging buffer into the original one. */
			rctx->dma_copy(ctx, dst, 0, doffset, 0, 0, src, 0, &box);
		}
		pipe_resource_reference((struct pipe_resource**)&rtransfer->staging, NULL);
	}

	if (transfer->usage & PIPE_TRANSFER_WRITE) {
		util_range_add(&rbuffer->valid_buffer_range, transfer->box.x,
			       transfer->box.x + transfer->box.width);
	}
	util_slab_free(&rctx->pool_transfers, transfer);
}

static const struct u_resource_vtbl r600_buffer_vtbl =
{
	NULL,				/* get_handle */
	r600_buffer_destroy,		/* resource_destroy */
	r600_buffer_transfer_map,	/* transfer_map */
	NULL,				/* transfer_flush_region */
	r600_buffer_transfer_unmap,	/* transfer_unmap */
	NULL				/* transfer_inline_write */
};

struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ,
					 unsigned alignment)
{
	struct r600_common_screen *rscreen = (struct r600_common_screen*)screen;
	struct r600_resource *rbuffer;

	rbuffer = MALLOC_STRUCT(r600_resource);

	rbuffer->b.b = *templ;
	pipe_reference_init(&rbuffer->b.b.reference, 1);
	rbuffer->b.b.screen = screen;
	rbuffer->b.vtbl = &r600_buffer_vtbl;
	rbuffer->buf = NULL;
	util_range_init(&rbuffer->valid_buffer_range);

	if (!r600_init_resource(rscreen, rbuffer, templ->width0, alignment, TRUE)) {
		FREE(rbuffer);
		return NULL;
	}
	return &rbuffer->b.b;
}
