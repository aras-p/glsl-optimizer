#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"

#include "nv40_context.h"

static void
nv40_miptree_layout(struct nv40_miptree *nv40mt)
{
	struct pipe_texture *pt = &nv40mt->base;
	boolean swizzled = FALSE;
	uint width = pt->width[0], height = pt->height[0], depth = pt->depth[0];
	uint offset = 0;
	int nr_faces, l, f, pitch;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		nr_faces = pt->depth[0];
	} else {
		nr_faces = 1;
	}

	pitch = pt->width[0];
	for (l = 0; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;
		pt->depth[l] = depth;
		pt->nblocksx[l] = pf_get_nblocksx(&pt->block, width);
		pt->nblocksy[l] = pf_get_nblocksy(&pt->block, height);

		if (swizzled)
			pitch = pt->nblocksx[l];
		pitch = align_int(pitch, 64);

		nv40mt->level[l].pitch = pitch * pt->block.size;
		nv40mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
		depth  = MAX2(1, depth  >> 1);
	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l <= pt->last_level; l++) {
			nv40mt->level[l].image_offset[f] = offset;
			offset += nv40mt->level[l].pitch * pt->height[l];
		}
	}

	nv40mt->total_size = offset;
}

static struct pipe_texture *
nv40_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *pt)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv40_miptree *mt;

	mt = MALLOC(sizeof(struct nv40_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = pscreen;

	nv40_miptree_layout(mt);

	mt->buffer = ws->buffer_create(ws, 256,
				       PIPE_BUFFER_USAGE_PIXEL |
				       NOUVEAU_BUFFER_USAGE_TEXTURE,
				       mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}

	return &mt->base;
}

static void
nv40_miptree_release(struct pipe_screen *pscreen, struct pipe_texture **pt)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct pipe_texture *mt = *pt;

	*pt = NULL;
	if (--mt->refcount <= 0) {
		struct nv40_miptree *nv40mt = (struct nv40_miptree *)mt;
		int l;

		pipe_buffer_reference(ws, &nv40mt->buffer, NULL);
		for (l = 0; l <= mt->last_level; l++) {
			if (nv40mt->level[l].image_offset)
				FREE(nv40mt->level[l].image_offset);
		}
		FREE(nv40mt);
	}
}

static struct pipe_surface *
nv40_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv40_miptree *nv40mt = (struct nv40_miptree *)pt;
	struct pipe_surface *ps;

	ps = CALLOC_STRUCT(pipe_surface);
	if (!ps)
		return NULL;
	pipe_texture_reference(&ps->texture, pt);
	pipe_buffer_reference(ws, &ps->buffer, nv40mt->buffer);
	ps->format = pt->format;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->block = pt->block;
	ps->nblocksx = pt->nblocksx[level];
	ps->nblocksy = pt->nblocksy[level];
	ps->stride = nv40mt->level[level].pitch;
	ps->usage = flags;
	ps->status = PIPE_SURFACE_STATUS_DEFINED;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ps->offset = nv40mt->level[level].image_offset[face];
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		ps->offset = nv40mt->level[level].image_offset[zslice];
	} else {
		ps->offset = nv40mt->level[level].image_offset[0];
	}

	return ps;
}

static void
nv40_miptree_surface_del(struct pipe_screen *pscreen,
			 struct pipe_surface **psurface)
{
	struct pipe_surface *ps = *psurface;

	*psurface = NULL;
	if (--ps->refcount > 0)
		return;

	pipe_texture_reference(&ps->texture, NULL);
	pipe_buffer_reference(pscreen->winsys, &ps->buffer, NULL);
	FREE(ps);
}

void
nv40_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv40_miptree_create;
	pscreen->texture_release = nv40_miptree_release;
	pscreen->get_tex_surface = nv40_miptree_surface_new;
	pscreen->tex_surface_release = nv40_miptree_surface_del;
}

