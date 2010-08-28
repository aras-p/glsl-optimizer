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
#include "state_tracker/drm_driver.h"
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"

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
	struct r600_screen *rscreen = r600_screen(screen);
	struct r600_resource *rbuffer;
	struct radeon_bo *bo;
	struct pb_desc desc;
	/* XXX We probably want a different alignment for buffers and textures. */
	unsigned alignment = 4096;

	rbuffer = CALLOC_STRUCT(r600_resource);
	if (rbuffer == NULL)
		return NULL;

	rbuffer->base.b = *templ;
	pipe_reference_init(&rbuffer->base.b.reference, 1);
	rbuffer->base.b.screen = screen;
	rbuffer->base.vtbl = &r600_buffer_vtbl;

	if ((rscreen->use_mem_constant == FALSE) && (rbuffer->base.b.bind & PIPE_BIND_CONSTANT_BUFFER)) {
		desc.alignment = alignment;
		desc.usage = rbuffer->base.b.bind;
		rbuffer->pb = pb_malloc_buffer_create(rbuffer->base.b.width0,
						      &desc);
		if (rbuffer->pb == NULL) {
			free(rbuffer);
			return NULL;
		}
		return &rbuffer->base.b;
	}
	rbuffer->domain = r600_domain_from_usage(rbuffer->base.b.bind);
	bo = radeon_bo(rscreen->rw, 0, rbuffer->base.b.width0, alignment, NULL);
	if (bo == NULL) {
		FREE(rbuffer);
		return NULL;
	}
	rbuffer->bo = bo;
	return &rbuffer->base.b;
}

struct pipe_resource *r600_user_buffer_create(struct pipe_screen *screen,
					      void *ptr, unsigned bytes,
					      unsigned bind)
{
	struct r600_resource *rbuffer;
	struct r600_screen *rscreen = r600_screen(screen);
	struct pipe_resource templ;

	memset(&templ, 0, sizeof(struct pipe_resource));
	templ.target = PIPE_BUFFER;
	templ.format = PIPE_FORMAT_R8_UNORM;
	templ.usage = PIPE_USAGE_IMMUTABLE;
	templ.bind = bind;
	templ.width0 = bytes;
	templ.height0 = 1;
	templ.depth0 = 1;

	rbuffer = (struct r600_resource*)r600_buffer_create(screen, &templ);
	if (rbuffer == NULL) {
		return NULL;
	}
	radeon_bo_map(rscreen->rw, rbuffer->bo);
	memcpy(rbuffer->bo->data, ptr, bytes);
	radeon_bo_unmap(rscreen->rw, rbuffer->bo);
	return &rbuffer->base.b;
}

static void r600_buffer_destroy(struct pipe_screen *screen,
				struct pipe_resource *buf)
{
	struct r600_resource *rbuffer = (struct r600_resource*)buf;
	struct r600_screen *rscreen = r600_screen(screen);

	if (rbuffer->pb) {
		pipe_reference_init(&rbuffer->pb->base.reference, 0);
		pb_destroy(rbuffer->pb);
		rbuffer->pb = NULL;
	}
	if (rbuffer->bo) {
		radeon_bo_decref(rscreen->rw, rbuffer->bo);
	}
	memset(rbuffer, 0, sizeof(struct r600_resource));
	FREE(rbuffer);
}

static void *r600_buffer_transfer_map(struct pipe_context *pipe,
				      struct pipe_transfer *transfer)
{
	struct r600_resource *rbuffer = (struct r600_resource*)transfer->resource;
	struct r600_screen *rscreen = r600_screen(pipe->screen);
	int write = 0;

	if (rbuffer->pb) {
		return (uint8_t*)pb_map(rbuffer->pb, transfer->usage, NULL) + transfer->box.x;
	}
	if (transfer->usage & PIPE_TRANSFER_DONTBLOCK) {
		/* FIXME */
	}
	if (transfer->usage & PIPE_TRANSFER_WRITE) {
		write = 1;
	}
	if (radeon_bo_map(rscreen->rw, rbuffer->bo)) {
		return NULL;
	}
	return (uint8_t*)rbuffer->bo->data + transfer->box.x;
}

static void r600_buffer_transfer_unmap(struct pipe_context *pipe,
					struct pipe_transfer *transfer)
{
	struct r600_resource *rbuffer = (struct r600_resource*)transfer->resource;
	struct r600_screen *rscreen = r600_screen(pipe->screen);

	if (rbuffer->pb) {
		pb_unmap(rbuffer->pb);
	} else {
		radeon_bo_unmap(rscreen->rw, rbuffer->bo);
	}
}

static void r600_buffer_transfer_flush_region(struct pipe_context *pipe,
					      struct pipe_transfer *transfer,
					      const struct pipe_box *box)
{
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
	struct radeon_bo *bo = NULL;

	bo = radeon_bo(rw, whandle->handle, 0, 0, NULL);
	if (bo == NULL) {
		return NULL;
	}

	rbuffer = CALLOC_STRUCT(r600_resource);
	if (rbuffer == NULL) {
		radeon_bo_decref(rw, bo);
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
