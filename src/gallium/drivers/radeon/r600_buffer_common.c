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
#include <inttypes.h>

boolean r600_rings_is_buffer_referenced(struct r600_common_context *ctx,
					struct radeon_winsys_cs_handle *buf,
					enum radeon_bo_usage usage)
{
	if (ctx->ws->cs_is_buffer_referenced(ctx->rings.gfx.cs, buf, usage)) {
		return TRUE;
	}
	if (ctx->rings.dma.cs &&
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

	if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
		return ctx->ws->buffer_map(resource->cs_buf, NULL, usage);
	}

	if (!(usage & PIPE_TRANSFER_WRITE)) {
		/* have to wait for the last write */
		rusage = RADEON_USAGE_WRITE;
	}

	if (ctx->rings.gfx.cs->cdw &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.gfx.cs,
					     resource->cs_buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			ctx->rings.gfx.flush(ctx, RADEON_FLUSH_ASYNC);
			return NULL;
		} else {
			ctx->rings.gfx.flush(ctx, 0);
		}
	}
	if (ctx->rings.dma.cs &&
	    ctx->rings.dma.cs->cdw &&
	    ctx->ws->cs_is_buffer_referenced(ctx->rings.dma.cs,
					     resource->cs_buf, rusage)) {
		if (usage & PIPE_TRANSFER_DONTBLOCK) {
			ctx->rings.dma.flush(ctx, RADEON_FLUSH_ASYNC);
			return NULL;
		} else {
			ctx->rings.dma.flush(ctx, 0);
		}
	}

	if (ctx->ws->buffer_is_busy(resource->buf, rusage)) {
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

	return ctx->ws->buffer_map(resource->cs_buf, NULL, usage);
}

bool r600_init_resource(struct r600_common_screen *rscreen,
			struct r600_resource *res,
			unsigned size, unsigned alignment,
			bool use_reusable_pool, unsigned usage)
{
	uint32_t initial_domain, domains;

	switch(usage) {
	case PIPE_USAGE_STAGING:
		/* Staging resources participate in transfers, i.e. are used
		 * for uploads and downloads from regular resources.
		 * We generate them internally for some transfers.
		 */
		initial_domain = RADEON_DOMAIN_GTT;
		domains = RADEON_DOMAIN_GTT;
		break;
	case PIPE_USAGE_DYNAMIC:
	case PIPE_USAGE_STREAM:
		/* Default to GTT, but allow the memory manager to move it to VRAM. */
		initial_domain = RADEON_DOMAIN_GTT;
		domains = RADEON_DOMAIN_GTT | RADEON_DOMAIN_VRAM;
		break;
	case PIPE_USAGE_DEFAULT:
	case PIPE_USAGE_STATIC:
	case PIPE_USAGE_IMMUTABLE:
	default:
		/* Don't list GTT here, because the memory manager would put some
		 * resources to GTT no matter what the initial domain is.
		 * Not listing GTT in the domains improves performance a lot. */
		initial_domain = RADEON_DOMAIN_VRAM;
		domains = RADEON_DOMAIN_VRAM;
		break;
	}

	res->buf = rscreen->ws->buffer_create(rscreen->ws, size, alignment,
                                              use_reusable_pool,
                                              initial_domain);
	if (!res->buf) {
		return false;
	}

	res->cs_buf = rscreen->ws->buffer_get_cs_handle(res->buf);
	res->domains = domains;
	util_range_set_empty(&res->valid_buffer_range);

	if (rscreen->debug_flags & DBG_VM && res->b.b.target == PIPE_BUFFER) {
		fprintf(stderr, "VM start=0x%"PRIu64"  end=0x%"PRIu64" | Buffer %u bytes\n",
			r600_resource_va(&rscreen->b, &res->b.b),
			r600_resource_va(&rscreen->b, &res->b.b) + res->buf->size,
			res->buf->size);
	}
	return true;
}
