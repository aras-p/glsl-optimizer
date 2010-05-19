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
 *      Corbin Simpson
 */
#include <pipe/p_screen.h>
#include <util/u_format.h>
#include <util/u_math.h>
#include <util/u_inlines.h>
#include <util/u_memory.h>
#include "state_tracker/drm_api.h"
#include "r600_screen.h"
#include "r600_texture.h"

extern struct u_resource_vtbl r600_texture_vtbl;

unsigned long r600_texture_get_offset(struct r600_texture *rtex, unsigned level, unsigned zslice, unsigned face)
{
	unsigned long offset = rtex->offset[level];

	switch (rtex->b.b.target) {
	case PIPE_TEXTURE_3D:
		assert(face == 0);
		return offset + zslice * rtex->layer_size[level];
	case PIPE_TEXTURE_CUBE:
		assert(zslice == 0);
		return offset + face * rtex->layer_size[level];
	default:
		assert(zslice == 0 && face == 0);
		return offset;
	}
}

static void r600_setup_miptree(struct r600_screen *rscreen, struct r600_texture *rtex)
{
	struct pipe_resource *ptex = &rtex->b.b;
	unsigned long w, h, stride, size, layer_size, i, offset;

	for (i = 0, offset = 0; i <= ptex->last_level; i++) {
		w = u_minify(ptex->width0, i);
		h = u_minify(ptex->height0, i);
		stride = align(util_format_get_stride(ptex->format, w), 32);
		layer_size = stride * h;
		if (ptex->target == PIPE_TEXTURE_CUBE)
			size = layer_size * 6;
		else
			size = layer_size * u_minify(ptex->depth0, i);
		rtex->offset[i] = offset;
		rtex->layer_size[i] = layer_size;
		rtex->pitch[i] = stride / util_format_get_blocksize(ptex->format);
		rtex->stride[i] = stride;
		offset += align(size, 32);
	}
	rtex->size = offset;
}

struct pipe_resource *r600_texture_create(struct pipe_screen *screen,
					  const struct pipe_resource *templ)
{
	struct r600_texture *rtex = CALLOC_STRUCT(r600_texture);
	struct r600_screen *rscreen = r600_screen(screen);
	struct pipe_resource templ_buf;

	if (!rtex) {
		return NULL;
	}
	rtex->b.b = *templ;
	rtex->b.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&rtex->b.b.reference, 1);
	rtex->b.b.screen = screen;
	r600_setup_miptree(rscreen, rtex);

	memset(&templ_buf, 0, sizeof(struct pipe_resource));
	templ_buf.target = PIPE_BUFFER;
	templ_buf.format = PIPE_FORMAT_R8_UNORM;
	templ_buf.usage = templ->usage;
	templ_buf.bind = templ->bind;
	templ_buf.width0 = rtex->size;
	templ_buf.height0 = 1;
	templ_buf.depth0 = 1;

	rtex->buffer = screen->resource_create(screen, &templ_buf);
	if (!rtex->buffer) {
		FREE(rtex);
		return NULL;
	}
	return &rtex->b.b;
}

static void r600_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource *ptex)
{
	struct r600_texture *rtex = (struct r600_texture*)ptex;

	FREE(rtex);
}

static struct pipe_surface *r600_get_tex_surface(struct pipe_screen *screen,
						struct pipe_resource *texture,
						unsigned face, unsigned level,
						unsigned zslice, unsigned flags)
{
	struct r600_texture *rtex = (struct r600_texture*)texture;
	struct pipe_surface *surface = CALLOC_STRUCT(pipe_surface);
	unsigned long offset;

	if (surface == NULL)
		return NULL;
	offset = r600_texture_get_offset(rtex, level, zslice, face);
	pipe_reference_init(&surface->reference, 1);
	pipe_resource_reference(&surface->texture, texture);
	surface->format = texture->format;
	surface->width = u_minify(texture->width0, level);
	surface->height = u_minify(texture->height0, level);
	surface->offset = offset;
	surface->usage = flags;
	surface->zslice = zslice;
	surface->texture = texture;
	surface->face = face;
	surface->level = level;
	return surface;
}

static void r600_tex_surface_destroy(struct pipe_surface *surface)
{
	pipe_resource_reference(&surface->texture, NULL);
	FREE(surface);
}

struct pipe_resource *r600_texture_from_handle(struct pipe_screen *screen,
					       const struct pipe_resource *base,
					       struct winsys_handle *whandle)
{
	struct pipe_resource *buffer;
	struct r600_texture *rtex;

	buffer = r600_buffer_from_handle(screen, whandle);
	if (buffer == NULL) {
		return NULL;
	}

	/* Support only 2D textures without mipmaps */
	if (base->target != PIPE_TEXTURE_2D || base->depth0 != 1 || base->last_level != 0)
		return NULL;

	rtex = CALLOC_STRUCT(r600_texture);
	if (rtex == NULL)
		return NULL;

	/* one ref already taken */
	rtex->buffer = buffer;

	rtex->b.b = *base;
	rtex->b.vtbl = &r600_texture_vtbl;
	pipe_reference_init(&rtex->b.b.reference, 1);
	rtex->b.b.screen = screen;
	rtex->stride_override = whandle->stride;
	rtex->pitch[0] = whandle->stride / util_format_get_blocksize(base->format);
	rtex->stride[0] = whandle->stride;
	rtex->offset[0] = 0;
	rtex->size = align(rtex->stride[0] * base->height0, 32);

	return &rtex->b.b;
}

static boolean r600_texture_get_handle(struct pipe_screen* screen,
				       struct pipe_resource *texture,
				       struct winsys_handle *whandle)
{
	struct r600_screen *rscreen = r600_screen(screen);
	struct r600_texture* rtex = (struct r600_texture*)texture;

	if (!rtex) {
		return FALSE;
	}

	whandle->stride = rtex->stride[0];

	r600_buffer_get_handle(rscreen->rw, rtex->buffer, whandle);

	return TRUE;
}

static unsigned int r600_texture_is_referenced(struct pipe_context *context,
						struct pipe_resource *texture,
						unsigned face, unsigned level)
{
	struct r600_texture *rtex = (struct r600_texture*)texture;

	return r600_buffer_is_referenced_by_cs(context, rtex->buffer, face, level);
}

struct u_resource_vtbl r600_texture_vtbl =
{
	r600_texture_get_handle,	/* get_handle */
	r600_texture_destroy,		/* resource_destroy */
	r600_texture_is_referenced,	/* is_resource_referenced */
	r600_texture_get_transfer,	/* get_transfer */
	r600_texture_transfer_destroy,	/* transfer_destroy */
	r600_texture_transfer_map,	/* transfer_map */
	u_default_transfer_flush_region,/* transfer_flush_region */
	r600_texture_transfer_unmap,	/* transfer_unmap */
	u_default_transfer_inline_write	/* transfer_inline_write */
};

void r600_init_screen_texture_functions(struct pipe_screen *screen)
{
	screen->get_tex_surface = r600_get_tex_surface;
	screen->tex_surface_destroy = r600_tex_surface_destroy;
}
