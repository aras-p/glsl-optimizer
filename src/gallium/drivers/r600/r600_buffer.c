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
#include <pipe/p_screen.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include <util/u_upload_mgr.h>
#include "state_tracker/drm_driver.h"
#include <xf86drm.h>
#include "radeon_drm.h"
#include "r600.h"
#include "r600_pipe.h"

extern struct u_resource_vtbl r600_buffer_vtbl;

u32 r600_domain_from_usage(unsigned usage)
{
	u32 domain = RADEON_GEM_DOMAIN_GTT;

	if (usage & PIPE_BIND_RENDER_TARGET) {
		domain |= RADEON_GEM_DOMAIN_VRAM;
	}
	if (usage & PIPE_BIND_DEPTH_STENCIL) {
		domain |= RADEON_GEM_DOMAIN_VRAM;
	}
	if (usage & PIPE_BIND_SAMPLER_VIEW) {
		domain |= RADEON_GEM_DOMAIN_VRAM;
	}
	/* also need BIND_BLIT_SOURCE/DESTINATION ? */
	if (usage & PIPE_BIND_VERTEX_BUFFER) {
		domain |= RADEON_GEM_DOMAIN_GTT;
	}
	if (usage & PIPE_BIND_INDEX_BUFFER) {
		domain |= RADEON_GEM_DOMAIN_GTT;
	}
	if (usage & PIPE_BIND_CONSTANT_BUFFER) {
		domain |= RADEON_GEM_DOMAIN_VRAM;
	}

	return domain;
}

struct pipe_resource *r600_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ)
{
	struct r600_resource_buffer *rbuffer;
	struct r600_bo *bo;
	/* XXX We probably want a different alignment for buffers and textures. */
	unsigned alignment = 4096;

	rbuffer = CALLOC_STRUCT(r600_resource_buffer);
	if (rbuffer == NULL)
		return NULL;

	rbuffer->magic = R600_BUFFER_MAGIC;
	rbuffer->user_buffer = NULL;
	rbuffer->num_ranges = 0;
	rbuffer->r.base.b = *templ;
	pipe_reference_init(&rbuffer->r.base.b.reference, 1);
	rbuffer->r.base.b.screen = screen;
	rbuffer->r.base.vtbl = &r600_buffer_vtbl;
	rbuffer->r.size = rbuffer->r.base.b.width0;
	rbuffer->r.domain = r600_domain_from_usage(rbuffer->r.base.b.bind);
	bo = r600_bo((struct radeon*)screen->winsys, rbuffer->r.base.b.width0, alignment, rbuffer->r.base.b.bind);
	if (bo == NULL) {
		FREE(rbuffer);
		return NULL;
	}
	rbuffer->r.bo = bo;
	return &rbuffer->r.base.b;
}

struct pipe_resource *r600_user_buffer_create(struct pipe_screen *screen,
					      void *ptr, unsigned bytes,
					      unsigned bind)
{
	struct r600_resource_buffer *rbuffer;

	rbuffer = CALLOC_STRUCT(r600_resource_buffer);
	if (rbuffer == NULL)
		return NULL;

	rbuffer->magic = R600_BUFFER_MAGIC;
	pipe_reference_init(&rbuffer->r.base.b.reference, 1);
	rbuffer->r.base.vtbl = &r600_buffer_vtbl;
	rbuffer->r.base.b.screen = screen;
	rbuffer->r.base.b.target = PIPE_BUFFER;
	rbuffer->r.base.b.format = PIPE_FORMAT_R8_UNORM;
	rbuffer->r.base.b.usage = PIPE_USAGE_IMMUTABLE;
	rbuffer->r.base.b.bind = bind;
	rbuffer->r.base.b.width0 = bytes;
	rbuffer->r.base.b.height0 = 1;
	rbuffer->r.base.b.depth0 = 1;
	rbuffer->r.base.b.flags = 0;
	rbuffer->num_ranges = 0;
	rbuffer->r.bo = NULL;
	rbuffer->user_buffer = ptr;
	return &rbuffer->r.base.b;
}

static void r600_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
	struct r600_resource_buffer *rbuffer = r600_buffer(buf);

	if (rbuffer->r.bo) {
		r600_bo_reference((struct radeon*)screen->winsys, &rbuffer->r.bo, NULL);
	}
	FREE(rbuffer);
}

static void *r600_buffer_transfer_map(struct pipe_context *pipe,
				      struct pipe_transfer *transfer)
{
	struct r600_resource_buffer *rbuffer = r600_buffer(transfer->resource);
	int write = 0;
	uint8_t *data;
	int i;
	boolean flush = FALSE;

	if (rbuffer->user_buffer)
		return (uint8_t*)rbuffer->user_buffer + transfer->box.x;

	if (transfer->usage & PIPE_TRANSFER_DISCARD) {
		for (i = 0; i < rbuffer->num_ranges; i++) {
			if ((transfer->box.x >= rbuffer->ranges[i].start) &&
			    (transfer->box.x < rbuffer->ranges[i].end))
				flush = TRUE;
			
			if (flush) {
				r600_bo_reference((struct radeon*)pipe->winsys, &rbuffer->r.bo, NULL);
				rbuffer->num_ranges = 0;
				rbuffer->r.bo = r600_bo((struct radeon*)pipe->winsys,
							     rbuffer->r.base.b.width0, 0,
							     rbuffer->r.base.b.bind);
				break;
			}
		}
	}
	if (transfer->usage & PIPE_TRANSFER_DONTBLOCK) {
		/* FIXME */
	}
	if (transfer->usage & PIPE_TRANSFER_WRITE) {
		write = 1;
	}
	data = r600_bo_map((struct radeon*)pipe->winsys, rbuffer->r.bo, transfer->usage, pipe);
	if (!data)
		return NULL;

	return (uint8_t*)data + transfer->box.x;
}

static void r600_buffer_transfer_unmap(struct pipe_context *pipe,
					struct pipe_transfer *transfer)
{
	struct r600_resource_buffer *rbuffer = r600_buffer(transfer->resource);

	if (rbuffer->r.bo)
		r600_bo_unmap((struct radeon*)pipe->winsys, rbuffer->r.bo);
}

static void r600_buffer_transfer_flush_region(struct pipe_context *pipe,
					      struct pipe_transfer *transfer,
					      const struct pipe_box *box)
{
	struct r600_resource_buffer *rbuffer = r600_buffer(transfer->resource);
	unsigned i;
	unsigned offset = transfer->box.x + box->x;
	unsigned length = box->width;

	assert(box->x + box->width <= transfer->box.width);

	if (rbuffer->user_buffer)
		return;

	/* mark the range as used */
	for(i = 0; i < rbuffer->num_ranges; ++i) {
		if(offset <= rbuffer->ranges[i].end && rbuffer->ranges[i].start <= (offset+box->width)) {
			rbuffer->ranges[i].start = MIN2(rbuffer->ranges[i].start, offset);
			rbuffer->ranges[i].end   = MAX2(rbuffer->ranges[i].end, (offset+length));
			return;
		}
	}
	
	rbuffer->ranges[rbuffer->num_ranges].start = offset;
	rbuffer->ranges[rbuffer->num_ranges].end = offset+length;
	rbuffer->num_ranges++;
}

unsigned r600_buffer_is_referenced_by_cs(struct pipe_context *context,
					 struct pipe_resource *buf,
					 unsigned face, unsigned level)
{
	/* FIXME */
	return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;
}

struct pipe_resource *r600_buffer_from_handle(struct pipe_screen *screen,
					      struct winsys_handle *whandle)
{
	struct radeon *rw = (struct radeon*)screen->winsys;
	struct r600_resource *rbuffer;
	struct r600_bo *bo = NULL;

	bo = r600_bo_handle(rw, whandle->handle, NULL);
	if (bo == NULL) {
		return NULL;
	}

	rbuffer = CALLOC_STRUCT(r600_resource);
	if (rbuffer == NULL) {
		r600_bo_reference(rw, &bo, NULL);
		return NULL;
	}

	pipe_reference_init(&rbuffer->base.b.reference, 1);
	rbuffer->base.b.target = PIPE_BUFFER;
	rbuffer->base.b.screen = screen;
	rbuffer->base.vtbl = &r600_buffer_vtbl;
	rbuffer->bo = bo;
	return &rbuffer->base.b;
}

struct u_resource_vtbl r600_buffer_vtbl =
{
	u_default_resource_get_handle,		/* get_handle */
	r600_buffer_destroy,			/* resource_destroy */
	r600_buffer_is_referenced_by_cs,	/* is_buffer_referenced */
	u_default_get_transfer,			/* get_transfer */
	u_default_transfer_destroy,		/* transfer_destroy */
	r600_buffer_transfer_map,		/* transfer_map */
	r600_buffer_transfer_flush_region,	/* transfer_flush_region */
	r600_buffer_transfer_unmap,		/* transfer_unmap */
	u_default_transfer_inline_write		/* transfer_inline_write */
};

int r600_upload_index_buffer(struct r600_pipe_context *rctx, struct r600_drawl *draw)
{
	struct pipe_resource *upload_buffer = NULL;
	unsigned index_offset = draw->index_buffer_offset;
	int ret = 0;

	if (r600_buffer_is_user_buffer(draw->index_buffer)) {
		ret = u_upload_buffer(rctx->upload_ib,
				      index_offset,
				      draw->count * draw->index_size,
				      draw->index_buffer,
				      &index_offset,
				      &upload_buffer);
		if (ret) {
			goto done;
		}
		draw->index_buffer_offset = index_offset;

		/* Transfer ownership. */
		pipe_resource_reference(&draw->index_buffer, upload_buffer);
		pipe_resource_reference(&upload_buffer, NULL);
	}

done:
	return ret;
}

int r600_upload_user_buffers(struct r600_pipe_context *rctx)
{
	enum pipe_error ret = PIPE_OK;
	int i, nr;

	nr = rctx->vertex_elements->count;

	for (i = 0; i < nr; i++) {
		struct pipe_vertex_buffer *vb =
			&rctx->vertex_buffer[rctx->vertex_elements->elements[i].vertex_buffer_index];

		if (r600_buffer_is_user_buffer(vb->buffer)) {
			struct pipe_resource *upload_buffer = NULL;
			unsigned offset = 0; /*vb->buffer_offset * 4;*/
			unsigned size = vb->buffer->width0;
			unsigned upload_offset;
			ret = u_upload_buffer(rctx->upload_vb,
					      offset, size,
					      vb->buffer,
					      &upload_offset, &upload_buffer);
			if (ret)
				return ret;

			pipe_resource_reference(&vb->buffer, NULL);
			vb->buffer = upload_buffer;
			vb->buffer_offset = upload_offset;
		}
	}
	return ret;
}
