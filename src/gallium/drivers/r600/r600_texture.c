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
#include "r600_screen.h"
#include "r600_texture.h"

unsigned long r600_texture_get_offset(struct r600_texture *rtex, unsigned level, unsigned zslice, unsigned face)
{
	unsigned long offset = rtex->offset[level];

	switch (rtex->tex.target) {
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
	struct pipe_texture *ptex = &rtex->tex;
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

static struct pipe_texture *r600_texture_create(struct pipe_screen *screen, const struct pipe_texture *template)
{
	struct r600_texture *rtex = CALLOC_STRUCT(r600_texture);
	struct r600_screen *rscreen = r600_screen(screen);

	if (!rtex) {
		return NULL;
	}
	rtex->tex = *template;
	pipe_reference_init(&rtex->tex.reference, 1);
	rtex->tex.screen = screen;
	r600_setup_miptree(rscreen, rtex);
	rtex->buffer = screen->buffer_create(screen, 4096, PIPE_BUFFER_USAGE_PIXEL, rtex->size);
	if (!rtex->buffer) {
		FREE(rtex);
		return NULL;
	}
	return (struct pipe_texture*)&rtex->tex;
}

static void r600_texture_destroy(struct pipe_texture *ptex)
{
	struct r600_texture *rtex = (struct r600_texture*)ptex;

	pipe_buffer_reference(&rtex->buffer, NULL);
	FREE(rtex);
}

static struct pipe_surface *r600_get_tex_surface(struct pipe_screen *screen,
						struct pipe_texture *texture,
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
	pipe_texture_reference(&surface->texture, texture);
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
	pipe_texture_reference(&surface->texture, NULL);
	FREE(surface);
}

static struct pipe_texture *r600_texture_blanket(struct pipe_screen *screen,
						const struct pipe_texture *base,
						const unsigned *stride,
						struct pipe_buffer *buffer)
{
	struct r600_texture *rtex;

	/* Support only 2D textures without mipmaps */
	if (base->target != PIPE_TEXTURE_2D || base->depth0 != 1 || base->last_level != 0)
		return NULL;
	rtex = CALLOC_STRUCT(r600_texture);
	if (rtex == NULL)
		return NULL;
	rtex->tex = *base;
	pipe_reference_init(&rtex->tex.reference, 1);
	rtex->tex.screen = screen;
	rtex->stride_override = *stride;
	rtex->pitch[0] = *stride / util_format_get_blocksize(base->format);
	rtex->stride[0] = *stride;
	rtex->offset[0] = 0;
	rtex->size = align(rtex->stride[0] * base->height0, 32);
	pipe_buffer_reference(&rtex->buffer, buffer);
	return &rtex->tex;
}

static struct pipe_video_surface *r600_video_surface_create(struct pipe_screen *screen,
							enum pipe_video_chroma_format chroma_format,
							unsigned width, unsigned height)
{
	return NULL;
}

static void r600_video_surface_destroy(struct pipe_video_surface *vsfc)
{
	FREE(vsfc);
}

void r600_init_screen_texture_functions(struct pipe_screen *screen)
{
	screen->texture_create = r600_texture_create;
	screen->texture_destroy = r600_texture_destroy;
	screen->get_tex_surface = r600_get_tex_surface;
	screen->tex_surface_destroy = r600_tex_surface_destroy;
	screen->texture_blanket = r600_texture_blanket;
	screen->video_surface_create = r600_video_surface_create;
	screen->video_surface_destroy= r600_video_surface_destroy;
}

boolean r600_get_texture_buffer(struct pipe_screen *screen,
				struct pipe_texture *texture,
				struct pipe_buffer **buffer,
				unsigned *stride)
{
	struct r600_texture *rtex = (struct r600_texture*)texture;

	if (rtex == NULL) {
		return FALSE;
	}
	pipe_buffer_reference(buffer, rtex->buffer);
	if (stride) {
		*stride = rtex->stride[0];
	}
	return TRUE;
}
