#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "nv40_context.h"

static void
nv40_miptree_layout(struct nv40_miptree *mt)
{
	struct pipe_texture *pt = &mt->base;
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
		pitch = align(pitch, 64);

		mt->level[l].pitch = pitch * pt->block.size;
		mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
		depth  = MAX2(1, depth  >> 1);
	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l <= pt->last_level; l++) {
			mt->level[l].image_offset[f] = offset;
			offset += mt->level[l].pitch * pt->height[l];
		}
	}

	mt->total_size = offset;
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
nv40_miptree_release(struct pipe_screen *pscreen, struct pipe_texture **ppt)
{
	struct pipe_texture *pt = *ppt;
	struct nv40_miptree *mt = (struct nv40_miptree *)pt;
	int l;

	*ppt = NULL;
	if (--pt->refcount)
		return;


	pipe_buffer_reference(pscreen, &mt->buffer, NULL);
	for (l = 0; l <= pt->last_level; l++) {
		if (mt->level[l].image_offset)
			FREE(mt->level[l].image_offset);
	}

	FREE(mt);
}

static struct pipe_surface *
nv40_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv40_miptree *mt = (struct nv40_miptree *)pt;
	struct pipe_surface *ps;

	ps = CALLOC_STRUCT(pipe_surface);
	if (!ps)
		return NULL;
	pipe_texture_reference(&ps->texture, pt);
	pipe_buffer_reference(pscreen, &ps->buffer, mt->buffer);
	ps->format = pt->format;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->block = pt->block;
	ps->nblocksx = pt->nblocksx[level];
	ps->nblocksy = pt->nblocksy[level];
	ps->stride = mt->level[level].pitch;
	ps->usage = flags;
	ps->status = PIPE_SURFACE_STATUS_DEFINED;
	ps->refcount = 1;
	ps->winsys = pscreen->winsys;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ps->offset = mt->level[level].image_offset[face];
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		ps->offset = mt->level[level].image_offset[zslice];
	} else {
		ps->offset = mt->level[level].image_offset[0];
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
	pipe_buffer_reference(pscreen, &ps->buffer, NULL);
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

