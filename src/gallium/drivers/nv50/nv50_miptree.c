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
nv50_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *tmp)
{
	struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
	struct pipe_texture *pt = &mt->base;
	unsigned usage, width = tmp->width[0], height = tmp->height[0];
	unsigned depth = tmp->depth[0];
	int i, l;

	mt->base = *tmp;
	pipe_reference_init(&mt->base.reference, 1);
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

	switch (pt->target) {
	case PIPE_TEXTURE_3D:
		mt->image_nr = pt->depth[0];
		break;
	case PIPE_TEXTURE_CUBE:
		mt->image_nr = 6;
		break;
	default:
		mt->image_nr = 1;
		break;
	}

	for (l = 0; l <= pt->last_level; l++) {
		struct nv50_miptree_level *lvl = &mt->level[l];

		pt->width[l] = width;
		pt->height[l] = height;
		pt->depth[l] = depth;
		pt->nblocksx[l] = pf_get_nblocksx(&pt->block, width);
		pt->nblocksy[l] = pf_get_nblocksy(&pt->block, height);

		lvl->image_offset = CALLOC(mt->image_nr, sizeof(int));
		lvl->pitch = align(pt->width[l] * pt->block.size, 64);

		width = MAX2(1, width >> 1);
		height = MAX2(1, height >> 1);
		depth = MAX2(1, depth >> 1);
	}

	for (i = 0; i < mt->image_nr; i++) {
		for (l = 0; l <= pt->last_level; l++) {
			struct nv50_miptree_level *lvl = &mt->level[l];
			int size;

			size  = align(pt->width[l], 8) * pt->block.size;
			size  = align(size, 64);
			size *= align(pt->height[l], 8) * pt->block.size;

			lvl->image_offset[i] = mt->total_size;

			mt->total_size += size;
		}
	}

	mt->buffer = pscreen->buffer_create(pscreen, 256, usage, mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}

	return &mt->base;
}

static struct pipe_texture *
nv50_miptree_blanket(struct pipe_screen *pscreen, const struct pipe_texture *pt,
		     const unsigned *stride, struct pipe_buffer *pb)
{
	struct nv50_miptree *mt;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (pt->target != PIPE_TEXTURE_2D || pt->last_level != 0 ||
	    pt->depth[0] != 1)
		return NULL;

	mt = CALLOC_STRUCT(nv50_miptree);
	if (!mt)
		return NULL;

	mt->base = *pt;
	pipe_reference_init(&mt->base.reference, 1);
	mt->base.screen = pscreen;
	mt->image_nr = 1;
	mt->level[0].pitch = *stride;
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));

	pipe_buffer_reference(&mt->buffer, pb);
	return &mt->base;
}

static void
nv50_miptree_destroy(struct pipe_texture *pt)
{
	struct nv50_miptree *mt = nv50_miptree(pt);

        pipe_buffer_reference(&mt->buffer, NULL);
        FREE(mt);
}

static struct pipe_surface *
nv50_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv50_miptree *mt = nv50_miptree(pt);
	struct nv50_miptree_level *lvl = &mt->level[level];
	struct pipe_surface *ps;
	int img;

	if (pt->target == PIPE_TEXTURE_CUBE)
		img = face;
	else
	if (pt->target == PIPE_TEXTURE_3D)
		img = zslice;
	else
		img = 0;

	ps = CALLOC_STRUCT(pipe_surface);
	if (!ps)
		return NULL;
	pipe_texture_reference(&ps->texture, pt);
	ps->format = pt->format;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->usage = flags;
	pipe_reference_init(&ps->reference, 1);
	ps->face = face;
	ps->level = level;
	ps->zslice = zslice;
	ps->offset = lvl->image_offset[img];

	return ps;
}

static void
nv50_miptree_surface_del(struct pipe_surface *ps)
{
	struct nv50_surface *s = nv50_surface(ps);

        pipe_texture_reference(&ps->texture, NULL);
        FREE(s);
}

void
nv50_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv50_miptree_create;
	pscreen->texture_blanket = nv50_miptree_blanket;
	pscreen->texture_destroy = nv50_miptree_destroy;
	pscreen->get_tex_surface = nv50_miptree_surface_new;
	pscreen->tex_surface_destroy = nv50_miptree_surface_del;
}

