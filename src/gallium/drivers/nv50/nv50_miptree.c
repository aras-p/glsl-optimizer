/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "nv50_context.h"

static struct pipe_texture *
nv50_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *pt)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
	unsigned usage, pitch;

	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = pscreen;

	usage = PIPE_BUFFER_USAGE_PIXEL;
	switch (pt->format) {
	case PIPE_FORMAT_Z24S8_UNORM:
	case PIPE_FORMAT_Z16_UNORM:
		usage |= NOUVEAU_BUFFER_USAGE_ZETA;
		break;
	default:
		break;
	}

	pitch = ((pt->width[0] + 63) & ~63) * pt->block.size;
	/*XXX*/
	pitch *= 2;

	mt->buffer = ws->buffer_create(ws, 256, usage, pitch * pt->height[0]);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}

	return &mt->base;
}

static void
nv50_miptree_release(struct pipe_screen *pscreen, struct pipe_texture **ppt)
{
	struct pipe_texture *pt = *ppt;

	*ppt = NULL;

	if (--pt->refcount <= 0) {
		struct nv50_miptree *mt = nv50_miptree(pt);

		pipe_buffer_reference(pscreen, &mt->buffer, NULL);
		FREE(mt);
	}
}

static struct pipe_surface *
nv50_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv50_miptree *mt = nv50_miptree(pt);
	struct nv50_surface *s;
	struct pipe_surface *ps;

	s = CALLOC_STRUCT(nv50_surface);
	if (!s)
		return NULL;
	ps = &s->base;

	ps->refcount = 1;
	ps->winsys = pscreen->winsys;
	ps->format = pt->format;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->block = pt->block;
	ps->nblocksx = pt->nblocksx[level];
	ps->nblocksy = pt->nblocksy[level];
	ps->stride = ps->width * ps->block.size;
	ps->offset = 0;
	ps->usage = flags;
	ps->status = PIPE_SURFACE_STATUS_DEFINED;

	pipe_texture_reference(&ps->texture, pt);
	pipe_buffer_reference(pscreen, &ps->buffer, mt->buffer);

	return ps;
}

static void
nv50_miptree_surface_del(struct pipe_screen *pscreen,
			 struct pipe_surface **psurface)
{
	struct pipe_surface *ps = *psurface;
	struct nv50_surface *s = nv50_surface(ps);

	*psurface = NULL;

	if (--ps->refcount <= 0) {
		pipe_texture_reference(&ps->texture, NULL);
		pipe_buffer_reference(pscreen, &ps->buffer, NULL);
		FREE(s);
	}
}

void
nv50_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv50_miptree_create;
	pscreen->texture_release = nv50_miptree_release;
	pscreen->get_tex_surface = nv50_miptree_surface_new;
	pscreen->tex_surface_release = nv50_miptree_surface_del;
}

