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
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
	struct pipe_texture *pt = &mt->base;
	unsigned usage, width = tmp->width[0], height = tmp->height[0];
	unsigned depth = tmp->depth[0];
	int i, l;

	mt->base = *tmp;
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
		lvl->image = CALLOC(mt->image_nr, sizeof(struct pipe_buffer *));

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

			lvl->image[i] = ws->buffer_create(ws, 256, 0, size);
			lvl->image_offset[i] = mt->total_size;

			mt->total_size += size;
		}
	}

	mt->buffer = ws->buffer_create(ws, 256, usage, mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}

	return &mt->base;
}

static INLINE void
mark_dirty(uint32_t *flags, unsigned image)
{
	flags[image / 32] |= (1 << (image % 32));
}

static INLINE void
mark_clean(uint32_t *flags, unsigned image)
{
	flags[image / 32] &= ~(1 << (image % 32));
}

static INLINE int
is_dirty(uint32_t *flags, unsigned image)
{
	return !!(flags[image / 32] & (1 << (image % 32)));
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

void
nv50_miptree_sync(struct pipe_screen *pscreen, struct nv50_miptree *mt,
		  unsigned level, unsigned image)
{
	struct nouveau_winsys *nvws = nv50_screen(pscreen)->nvws;
	struct nv50_miptree_level *lvl = &mt->level[level];
	struct pipe_surface *dst, *src;
	unsigned face = 0, zslice = 0;

	if (!is_dirty(lvl->image_dirty_cpu, image))
		return;

	if (mt->base.target == PIPE_TEXTURE_CUBE)
		face = image;
	else
	if (mt->base.target == PIPE_TEXTURE_3D)
		zslice = image;

	/* Mark as clean already - so we don't continually call this function
	 * trying to get a GPU_WRITE pipe_surface!
	 */
	mark_clean(lvl->image_dirty_cpu, image);

	/* Pretend we're doing CPU access so we get the backing pipe_surface
	 * and not a view into the larger miptree.
	 */
	src = pscreen->get_tex_surface(pscreen, &mt->base, face, level, zslice,
				       PIPE_BUFFER_USAGE_CPU_READ);

	/* Pretend we're only reading with the GPU so surface doesn't get marked
	 * as dirtied by the GPU.
	 */
	dst = pscreen->get_tex_surface(pscreen, &mt->base, face, level, zslice,
				       PIPE_BUFFER_USAGE_GPU_READ);

	nvws->surface_copy(nvws, dst, 0, 0, src, 0, 0, dst->width, dst->height);

	pscreen->tex_surface_release(pscreen, &dst);
	pscreen->tex_surface_release(pscreen, &src);
}

/* The reverse of the above */
static void
nv50_miptree_sync_cpu(struct pipe_screen *pscreen, struct nv50_miptree *mt,
		      unsigned level, unsigned image)
{
	struct nouveau_winsys *nvws = nv50_screen(pscreen)->nvws;
	struct nv50_miptree_level *lvl = &mt->level[level];
	struct pipe_surface *dst, *src;
	unsigned face = 0, zslice = 0;

	if (!is_dirty(lvl->image_dirty_gpu, image))
		return;

	if (mt->base.target == PIPE_TEXTURE_CUBE)
		face = image;
	else
	if (mt->base.target == PIPE_TEXTURE_3D)
		zslice = image;

	mark_clean(lvl->image_dirty_gpu, image);

	src = pscreen->get_tex_surface(pscreen, &mt->base, face, level, zslice,
				       PIPE_BUFFER_USAGE_GPU_READ);
	dst = pscreen->get_tex_surface(pscreen, &mt->base, face, level, zslice,
				       PIPE_BUFFER_USAGE_CPU_READ);

	nvws->surface_copy(nvws, dst, 0, 0, src, 0, 0, dst->width, dst->height);

	pscreen->tex_surface_release(pscreen, &dst);
	pscreen->tex_surface_release(pscreen, &src);
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
	ps->block = pt->block;
	ps->nblocksx = pt->nblocksx[level];
	ps->nblocksy = pt->nblocksy[level];
	ps->stride = ps->width * ps->block.size;
	ps->usage = flags;
	ps->status = PIPE_SURFACE_STATUS_DEFINED;
	ps->refcount = 1;
	ps->face = face;
	ps->level = level;
	ps->zslice = zslice;

	if (flags & PIPE_BUFFER_USAGE_CPU_READ_WRITE) {
		assert(!(flags & PIPE_BUFFER_USAGE_GPU_READ_WRITE));
		nv50_miptree_sync_cpu(pscreen, mt, level, img);

		ps->offset = 0;
		pipe_texture_reference(&ps->texture, pt);

		if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
			mark_dirty(lvl->image_dirty_cpu, img);
	} else {
		nv50_miptree_sync(pscreen, mt, level, img);

		ps->offset = lvl->image_offset[img];
		pipe_texture_reference(&ps->texture, pt);

		if (flags & PIPE_BUFFER_USAGE_GPU_WRITE)
			mark_dirty(lvl->image_dirty_gpu, img);
	}

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

