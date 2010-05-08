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
#include "r600_screen.h"
#include "r600_texture.h"
#include "r600_context.h"

static u32 r600_domain_from_usage(unsigned usage)
{
	u32 domain = RADEON_GEM_DOMAIN_GTT;

	if (usage & PIPE_BUFFER_USAGE_GPU_WRITE) {
		domain |= RADEON_GEM_DOMAIN_VRAM;
	}
	if (usage & PIPE_BUFFER_USAGE_PIXEL) {
		domain |= RADEON_GEM_DOMAIN_VRAM;
	}
	if (usage & PIPE_BUFFER_USAGE_VERTEX) {
		domain |= RADEON_GEM_DOMAIN_GTT;
	}
	if (usage & PIPE_BUFFER_USAGE_INDEX) {
		domain |= RADEON_GEM_DOMAIN_GTT;
	}
	return domain;
}

static struct pipe_buffer *r600_buffer_create(struct pipe_screen *screen,
						unsigned alignment,
						unsigned usage,
						unsigned size)
{
	struct r600_screen *rscreen = r600_screen(screen);
	struct r600_pipe_buffer *rbuffer;
	struct radeon_bo *bo;
	struct pb_desc desc;

	rbuffer = CALLOC_STRUCT(r600_pipe_buffer);
	if (rbuffer == NULL)
		return NULL;

	pipe_reference_init(&rbuffer->base.reference, 1);
	rbuffer->base.screen = screen;
	rbuffer->base.alignment = alignment;
	rbuffer->base.usage = usage;
	rbuffer->base.size = size;

	if (usage & PIPE_BUFFER_USAGE_CONSTANT) {
		desc.alignment = alignment;
		desc.usage = usage;
		rbuffer->pb = pb_malloc_buffer_create(size, &desc);
		if (rbuffer->pb == NULL) {
			free(rbuffer);
			return NULL;
		}
		return &rbuffer->base;
	}
	rbuffer->domain = r600_domain_from_usage(usage);
	bo = radeon_bo(rscreen->rw, 0, size, alignment, NULL);
	if (bo == NULL) {
		FREE(rbuffer);
		return NULL;
	}
	rbuffer->bo = bo;
	return &rbuffer->base;
}

static struct pipe_buffer *r600_user_buffer_create(struct pipe_screen *screen,
							void *ptr, unsigned bytes)
{
	struct r600_pipe_buffer *rbuffer;
	struct r600_screen *rscreen = r600_screen(screen);

	rbuffer = (struct r600_pipe_buffer*)r600_buffer_create(screen, 0, 0, bytes);
	if (rbuffer == NULL) {
		return NULL;
	}
	radeon_bo_map(rscreen->rw, rbuffer->bo);
	memcpy(rbuffer->bo->data, ptr, bytes);
	radeon_bo_unmap(rscreen->rw, rbuffer->bo);
	return &rbuffer->base;
}

static void r600_buffer_destroy(struct pipe_buffer *buffer)
{
	struct r600_pipe_buffer *rbuffer = (struct r600_pipe_buffer*)buffer;
	struct r600_screen *rscreen = r600_screen(buffer->screen);

	if (rbuffer->pb) {
		pipe_reference_init(&rbuffer->pb->base.reference, 0);
		pb_destroy(rbuffer->pb);
		rbuffer->pb = NULL;
	}
	if (rbuffer->bo) {
		radeon_bo_decref(rscreen->rw, rbuffer->bo);
	}
	FREE(rbuffer);
}

static void *r600_buffer_map_range(struct pipe_screen *screen,
					struct pipe_buffer *buffer,
					unsigned offset, unsigned length,
					unsigned usage)
{
	struct r600_pipe_buffer *rbuffer = (struct r600_pipe_buffer*)buffer;
	struct r600_screen *rscreen = r600_screen(buffer->screen);
	int write = 0;

	if (rbuffer->pb) {
		return pb_map(rbuffer->pb, usage);
	}
	if (usage & PIPE_BUFFER_USAGE_DONTBLOCK) {
		/* FIXME */
	}
	if (usage & PIPE_BUFFER_USAGE_CPU_WRITE) {
		write = 1;
	}
	if (radeon_bo_map(rscreen->rw, rbuffer->bo)) {
		return NULL;
	}
	return rbuffer->bo->data;
}

static void r600_buffer_unmap( struct pipe_screen *screen, struct pipe_buffer *buffer)
{
	struct r600_pipe_buffer *rbuffer = (struct r600_pipe_buffer*)buffer;
	struct r600_screen *rscreen = r600_screen(buffer->screen);

	if (rbuffer->pb) {
		pb_unmap(rbuffer->pb);
	} else {
		radeon_bo_unmap(rscreen->rw, rbuffer->bo);
	}
}

static void r600_buffer_flush_mapped_range(struct pipe_screen *screen,
						struct pipe_buffer *buf,
						unsigned offset, unsigned length)
{
}

void r600_screen_init_buffer_functions(struct pipe_screen *screen)
{
	screen->buffer_create = r600_buffer_create;
	screen->user_buffer_create = r600_user_buffer_create;
	screen->buffer_map_range = r600_buffer_map_range;
	screen->buffer_flush_mapped_range = r600_buffer_flush_mapped_range;
	screen->buffer_unmap = r600_buffer_unmap;
	screen->buffer_destroy = r600_buffer_destroy;
}
