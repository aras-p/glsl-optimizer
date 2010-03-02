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
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "nv50_context.h"

/* The restrictions in tile mode selection probably aren't necessary. */
static INLINE uint32_t
get_tile_mode(unsigned ny, unsigned d)
{
	uint32_t tile_mode = 0x00;

	if (ny > 32) tile_mode = 0x04; /* height 64 tiles */
	else
	if (ny > 16) tile_mode = 0x03; /* height 32 tiles */
	else
	if (ny >  8) tile_mode = 0x02; /* height 16 tiles */
	else
	if (ny >  4) tile_mode = 0x01; /* height 8 tiles */

	if (d == 1)
		return tile_mode;
	else
	if (tile_mode > 0x02)
		tile_mode = 0x02;

	if (d > 16 && tile_mode < 0x02)
		return tile_mode | 0x50; /* depth 32 tiles */
	if (d >  8) return tile_mode | 0x40; /* depth 16 tiles */
	if (d >  4) return tile_mode | 0x30; /* depth 8 tiles */
	if (d >  2) return tile_mode | 0x20; /* depth 4 tiles */

	return tile_mode | 0x10;
}

static INLINE unsigned
get_zslice_offset(unsigned tile_mode, unsigned z, unsigned pitch, unsigned nb_h)
{
	unsigned tile_h = get_tile_height(tile_mode);
	unsigned tile_d = get_tile_depth(tile_mode);

	/* pitch_2d == to next slice within this volume-tile */
	/* pitch_3d == size (in bytes) of a volume-tile */
	unsigned pitch_2d = tile_h * 64;
	unsigned pitch_3d = tile_d * align(nb_h, tile_h) * pitch;

	return (z % tile_d) * pitch_2d + (z / tile_d) * pitch_3d;
}

static struct pipe_texture *
nv50_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *tmp)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
	struct pipe_texture *pt = &mt->base.base;
	unsigned width = tmp->width0, height = tmp->height0;
	unsigned depth = tmp->depth0, image_alignment;
	uint32_t tile_flags;
	int ret, i, l;

	*pt = *tmp;
	pipe_reference_init(&pt->reference, 1);
	pt->screen = pscreen;

	switch (pt->format) {
	case PIPE_FORMAT_Z32_FLOAT:
		tile_flags = 0x4800;
		break;
	case PIPE_FORMAT_S8Z24_UNORM:
		tile_flags = 0x1800;
		break;
	case PIPE_FORMAT_Z16_UNORM:
		tile_flags = 0x6c00;
		break;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24S8_UNORM:
		tile_flags = 0x2800;
		break;
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32_FLOAT:
		tile_flags = 0x7400;
		break;
	default:
		if ((pt->tex_usage & PIPE_TEXTURE_USAGE_PRIMARY) &&
		    util_format_get_blocksizebits(pt->format) == 32)
			tile_flags = 0x7a00;
		else
			tile_flags = 0x7000;
		break;
	}

	/* XXX: texture arrays */
	mt->image_nr = (pt->target == PIPE_TEXTURE_CUBE) ? 6 : 1;

	for (l = 0; l <= pt->last_level; l++) {
		struct nv50_miptree_level *lvl = &mt->level[l];
		unsigned nblocksy = util_format_get_nblocksy(pt->format, height);

		lvl->image_offset = CALLOC(mt->image_nr, sizeof(int));
		lvl->pitch = align(util_format_get_stride(pt->format, width), 64);
		lvl->tile_mode = get_tile_mode(nblocksy, depth);

		width = u_minify(width, 1);
		height = u_minify(height, 1);
		depth = u_minify(depth, 1);
	}

	image_alignment  = get_tile_height(mt->level[0].tile_mode) * 64;
	image_alignment *= get_tile_depth(mt->level[0].tile_mode);

	/* NOTE the distinction between arrays of mip-mapped 2D textures and
	 * mip-mapped 3D textures. We can't use image_nr == depth for 3D mip.
	 */
	for (i = 0; i < mt->image_nr; i++) {
		for (l = 0; l <= pt->last_level; l++) {
			struct nv50_miptree_level *lvl = &mt->level[l];
			int size;
			unsigned tile_h = get_tile_height(lvl->tile_mode);
			unsigned tile_d = get_tile_depth(lvl->tile_mode);

			size  = lvl->pitch;
			size *= align(util_format_get_nblocksy(pt->format, u_minify(pt->height0, l)), tile_h);
			size *= align(u_minify(pt->depth0, l), tile_d);

			lvl->image_offset[i] = mt->total_size;

			mt->total_size += size;
		}
		mt->total_size = align(mt->total_size, image_alignment);
	}

	ret = nouveau_bo_new_tile(dev, NOUVEAU_BO_VRAM, 256, mt->total_size,
				  mt->level[0].tile_mode, tile_flags,
				  &mt->base.bo);
	if (ret) {
		for (l = 0; l <= pt->last_level; ++l)
			FREE(mt->level[l].image_offset);
		FREE(mt);
		return NULL;
	}

	return pt;
}

static struct pipe_texture *
nv50_miptree_blanket(struct pipe_screen *pscreen, const struct pipe_texture *pt,
		     const unsigned *stride, struct pipe_buffer *pb)
{
	struct nouveau_bo *bo = nouveau_bo(pb);
	struct nv50_miptree *mt;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if (pt->target != PIPE_TEXTURE_2D || pt->last_level != 0 ||
	    pt->depth0 != 1)
		return NULL;

	mt = CALLOC_STRUCT(nv50_miptree);
	if (!mt)
		return NULL;

	mt->base.base = *pt;
	pipe_reference_init(&mt->base.base.reference, 1);
	mt->base.base.screen = pscreen;
	mt->image_nr = 1;
	mt->level[0].pitch = *stride;
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));
	mt->level[0].tile_mode = bo->tile_mode;

	nouveau_bo_ref(bo, &mt->base.bo);
	return &mt->base.base;
}

static void
nv50_miptree_destroy(struct pipe_texture *pt)
{
	struct nv50_miptree *mt = nv50_miptree(pt);
	unsigned l;

	for (l = 0; l <= pt->last_level; ++l)
		FREE(mt->level[l].image_offset);

	nouveau_bo_ref(NULL, &mt->base.bo);
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
	unsigned img = 0;

	if (pt->target == PIPE_TEXTURE_CUBE)
		img = face;

	ps = CALLOC_STRUCT(pipe_surface);
	if (!ps)
		return NULL;
	pipe_texture_reference(&ps->texture, pt);
	ps->format = pt->format;
	ps->width = u_minify(pt->width0, level);
	ps->height = u_minify(pt->height0, level);
	ps->usage = flags;
	pipe_reference_init(&ps->reference, 1);
	ps->face = face;
	ps->level = level;
	ps->zslice = zslice;
	ps->offset = lvl->image_offset[img];

	if (pt->target == PIPE_TEXTURE_3D) {
		unsigned nb_h = util_format_get_nblocksy(pt->format, ps->height);
		ps->offset += get_zslice_offset(lvl->tile_mode, zslice,
						lvl->pitch, nb_h);
	}

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

