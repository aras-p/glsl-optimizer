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
#include "nv50_resource.h"
#include "nv50_transfer.h"

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




static void
nv50_miptree_destroy(struct pipe_screen *pscreen,
		     struct pipe_resource *pt)
{
	struct nv50_miptree *mt = nv50_miptree(pt);
	unsigned l;

	for (l = 0; l <= pt->last_level; ++l)
		FREE(mt->level[l].image_offset);

	nouveau_screen_bo_release(pscreen, mt->base.bo);
	FREE(mt);
}

static boolean
nv50_miptree_get_handle(struct pipe_screen *pscreen,
			struct pipe_resource *pt,
			struct winsys_handle *whandle)
{
	struct nv50_miptree *mt = nv50_miptree(pt);
	unsigned stride;


	if (!mt || !mt->base.bo)
		return FALSE;

	stride = util_format_get_stride(mt->base.base.format,
					mt->base.base.width0);

	return nouveau_screen_bo_get_handle(pscreen,
					    mt->base.bo,
					    stride,
					    whandle);
}


const struct u_resource_vtbl nv50_miptree_vtbl =
{
   nv50_miptree_get_handle,	      /* get_handle */
   nv50_miptree_destroy,	      /* resource_destroy */
   NULL,			      /* is_resource_referenced */
   nv50_miptree_transfer_new,	      /* get_transfer */
   nv50_miptree_transfer_del,     /* transfer_destroy */
   nv50_miptree_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   nv50_miptree_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};



struct pipe_resource *
nv50_miptree_create(struct pipe_screen *pscreen, const struct pipe_resource *tmp)
{
	struct nouveau_device *dev = nouveau_screen(pscreen)->device;
	struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
	struct pipe_resource *pt = &mt->base.base;
	unsigned width = tmp->width0, height = tmp->height0;
	unsigned depth = tmp->depth0, image_alignment;
	uint32_t tile_flags;
	int ret, i, l;

	if (!mt)
		return NULL;

	*pt = *tmp;
	mt->base.vtbl = &nv50_miptree_vtbl;
	pipe_reference_init(&pt->reference, 1);
	pt->screen = pscreen;

	switch (pt->format) {
	case PIPE_FORMAT_Z32_FLOAT:
		tile_flags = 0x4800;
		break;
	case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
		tile_flags = 0x1800;
		break;
	case PIPE_FORMAT_Z16_UNORM:
		tile_flags = 0x6c00;
		break;
	case PIPE_FORMAT_Z24X8_UNORM:
	case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
		tile_flags = 0x2800;
		break;
	case PIPE_FORMAT_Z32_FLOAT_S8X24_USCALED:
		tile_flags = 0xe000;
		break;
	case PIPE_FORMAT_R32G32B32A32_FLOAT:
	case PIPE_FORMAT_R32G32B32_FLOAT:
		tile_flags = 0x7400;
		break;
	default:
		if ((pt->bind & PIPE_BIND_SCANOUT) &&
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


struct pipe_resource *
nv50_miptree_from_handle(struct pipe_screen *pscreen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
	struct nv50_miptree *mt;
	unsigned stride;

	/* Only supports 2D, non-mipmapped textures for the moment */
	if ((template->target != PIPE_TEXTURE_2D &&
	      template->target != PIPE_TEXTURE_RECT) ||
	    template->last_level != 0 ||
	    template->depth0 != 1)
		return NULL;

	mt = CALLOC_STRUCT(nv50_miptree);
	if (!mt)
		return NULL;

	mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
	if (mt->base.bo == NULL) {
		FREE(mt);
		return NULL;
	}


	mt->base.base = *template;
	mt->base.vtbl = &nv50_miptree_vtbl;
	pipe_reference_init(&mt->base.base.reference, 1);
	mt->base.base.screen = pscreen;
	mt->image_nr = 1;
	mt->level[0].pitch = stride;
	mt->level[0].image_offset = CALLOC(1, sizeof(unsigned));
	mt->level[0].tile_mode = mt->base.bo->tile_mode;

	/* XXX: Need to adjust bo refcount??
	 */
	/* nouveau_bo_ref(bo, &mt->base.bo); */
	return &mt->base.base;
}



/* Surface functions
 */

struct pipe_surface *
nv50_miptree_surface_new(struct pipe_context *pipe, struct pipe_resource *pt,
			 const struct pipe_surface *surf_tmpl)
{
	unsigned level = surf_tmpl->u.tex.level;
	struct nv50_miptree *mt = nv50_miptree(pt);
	struct nv50_miptree_level *lvl = &mt->level[level];
	struct nv50_surface *ns;
	unsigned img = 0, zslice = 0;

	assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);

	/* XXX can't unify these here? */
	if (pt->target == PIPE_TEXTURE_CUBE)
		img = surf_tmpl->u.tex.first_layer;
	else if (pt->target == PIPE_TEXTURE_3D)
		zslice = surf_tmpl->u.tex.first_layer;

	ns = CALLOC_STRUCT(nv50_surface);
	if (!ns)
		return NULL;
	pipe_resource_reference(&ns->base.texture, pt);
	ns->base.context = pipe;
	ns->base.format = pt->format;
	ns->base.width = u_minify(pt->width0, level);
	ns->base.height = u_minify(pt->height0, level);
	ns->base.usage = surf_tmpl->usage;
	pipe_reference_init(&ns->base.reference, 1);
	ns->base.u.tex.level = level;
	ns->base.u.tex.first_layer = surf_tmpl->u.tex.first_layer;
	ns->base.u.tex.last_layer = surf_tmpl->u.tex.last_layer;
	ns->offset = lvl->image_offset[img];

	if (pt->target == PIPE_TEXTURE_3D) {
		unsigned nb_h = util_format_get_nblocksy(pt->format, ns->base.height);
		ns->offset += get_zslice_offset(lvl->tile_mode, zslice,
						lvl->pitch, nb_h);
	}

	return &ns->base;
}

void
nv50_miptree_surface_del(struct pipe_context *pipe,
			 struct pipe_surface *ps)
{
	struct nv50_surface *s = nv50_surface(ps);

	pipe_resource_reference(&s->base.texture, NULL);
	FREE(s);
}
