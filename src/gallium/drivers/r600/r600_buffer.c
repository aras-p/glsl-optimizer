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
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 */
#include "r600_pipe.h"
#include "util/u_upload_mgr.h"
#include "util/u_memory.h"
#include "util/u_surface.h"

static void r600_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
	struct r600_resource *rbuffer = r600_resource(buf);

	pb_reference(&rbuffer->buf, NULL);
	FREE(rbuffer);
}

static void r600_set_constants_dirty_if_bound(struct r600_context *rctx,
					      struct r600_resource *rbuffer)
{
	unsigned shader;

	for (shader = 0; shader < PIPE_SHADER_TYPES; shader++) {
		struct r600_constbuf_state *state = &rctx->constbuf_state[shader];
		bool found = false;
		uint32_t mask = state->enabled_mask;

		while (mask) {
			unsigned i = u_bit_scan(&mask);
			if (state->cb[i].buffer == &rbuffer->b.b) {
				found = true;
				state->dirty_mask |= 1 << i;
			}
		}
		if (found) {
			r600_constant_buffers_dirty(rctx, state);
		}
	}
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
	struct r600_context *rctx = (struct r600_context*)ctx;
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

static void *r600_buffer_transfer_map(struct pipe_context *ctx,
					struct pipe_resource *resource,
					unsigned level,
					unsigned usage,
					const struct pipe_box *box,
					struct pipe_transfer **ptransfer)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct r600_resource *rbuffer = r600_resource(resource);
	uint8_t *data;

	assert(box->x + box->width <= resource->width0);

	if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE &&
	    !(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
		assert(usage & PIPE_TRANSFER_WRITE);

		/* Check if mapping this buffer would cause waiting for the GPU. */
		if (r600_rings_is_buffer_referenced(rctx, rbuffer->cs_buf, RADEON_USAGE_READWRITE) ||
		    rctx->ws->buffer_is_busy(rbuffer->buf, RADEON_USAGE_READWRITE)) {
			unsigned i, mask;

			/* Discard the buffer. */
			pb_reference(&rbuffer->buf, NULL);

			/* Create a new one in the same pipe_resource. */
			/* XXX We probably want a different alignment for buffers and textures. */
			r600_init_resource(rctx->screen, rbuffer, rbuffer->b.b.width0, 4096,
					   TRUE, rbuffer->b.b.usage);

			/* We changed the buffer, now we need to bind it where the old one was bound. */
			/* Vertex buffers. */
			mask = rctx->vertex_buffer_state.enabled_mask;
			while (mask) {
				i = u_bit_scan(&mask);
				if (rctx->vertex_buffer_state.vb[i].buffer == &rbuffer->b.b) {
					rctx->vertex_buffer_state.dirty_mask |= 1 << i;
					r600_vertex_buffers_dirty(rctx);
				}
			}
			/* Streamout buffers. */
			for (i = 0; i < rctx->num_so_targets; i++) {
				if (rctx->so_targets[i]->b.buffer == &rbuffer->b.b) {
					r600_context_streamout_end(rctx);
					rctx->streamout_start = TRUE;
					rctx->streamout_append_bitmask = ~0;
				}
			}
			/* Constant buffers. */
			r600_set_constants_dirty_if_bound(rctx, rbuffer);
		}
	}
	else if ((usage & PIPE_TRANSFER_DISCARD_RANGE) &&
		 !(usage & PIPE_TRANSFER_UNSYNCHRONIZED) &&
		 rctx->screen->has_streamout &&
		 /* The buffer range must be aligned to 4. */
		 box->x % 4 == 0 && box->width % 4 == 0) {
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
			}
		}
	}

	/* mmap and synchronize with rings */
	data = r600_buffer_mmap_sync_with_rings(rctx, rbuffer, usage);
	if (!data) {
		return NULL;
	}
	data += box->x;

	return r600_buffer_get_transfer(ctx, resource, level, usage, box,
					ptransfer, data, NULL, 0);
}

static void r600_buffer_transfer_unmap(struct pipe_context *pipe,
					struct pipe_transfer *transfer)
{
	struct r600_context *rctx = (struct r600_context*)pipe;
	struct r600_transfer *rtransfer = (struct r600_transfer*)transfer;

	if (rtransfer->staging) {
		struct pipe_resource *dst, *src;
		unsigned soffset, doffset, size;

		dst = transfer->resource;
		src = &rtransfer->staging->b.b;
		size = transfer->box.width;
		doffset = transfer->box.x;
		soffset = rtransfer->offset + transfer->box.x % R600_MAP_BUFFER_ALIGNMENT;
		/* Copy the staging buffer into the original one. */
		if (rctx->rings.dma.cs && !(size % 4) && !(doffset % 4) && !(soffset)) {
			if (rctx->screen->chip_class >= EVERGREEN) {
				evergreen_dma_copy(rctx, dst, src, doffset, soffset, size);
			} else {
				r600_dma_copy(rctx, dst, src, doffset, soffset, size);
			}
		} else {
			struct pipe_box box;

			u_box_1d(soffset, size, &box);
			r600_copy_buffer(pipe, dst, doffset, src, &box);
		}
		pipe_resource_reference((struct pipe_resource**)&rtransfer->staging, NULL);
	}
	util_slab_free(&rctx->pool_transfers, transfer);
}

static const struct u_resource_vtbl r600_buffer_vtbl =
{
	u_default_resource_get_handle,		/* get_handle */
	r600_buffer_destroy,			/* resource_destroy */
	r600_buffer_transfer_map,		/* transfer_map */
	NULL,					/* transfer_flush_region */
	r600_buffer_transfer_unmap,		/* transfer_unmap */
	NULL					/* transfer_inline_write */
};

bool r600_init_resource(struct r600_screen *rscreen,
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
	return true;
}

struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ,
					 unsigned alignment)
{
	struct r600_screen *rscreen = (struct r600_screen*)screen;
	struct r600_resource *rbuffer;

	rbuffer = MALLOC_STRUCT(r600_resource);

	rbuffer->b.b = *templ;
	pipe_reference_init(&rbuffer->b.b.reference, 1);
	rbuffer->b.b.screen = screen;
	rbuffer->b.vtbl = &r600_buffer_vtbl;

	if (!r600_init_resource(rscreen, rbuffer, templ->width0, alignment, TRUE, templ->usage)) {
		FREE(rbuffer);
		return NULL;
	}
	return &rbuffer->b.b;
}
