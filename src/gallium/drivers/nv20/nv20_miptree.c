#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "nv20_context.h"
#include "nv20_screen.h"

static void
nv20_miptree_layout(struct nv20_miptree *nv20mt)
{
	struct pipe_texture *pt = &nv20mt->base;
	boolean swizzled = FALSE;
	uint width = pt->width[0], height = pt->height[0];
	uint offset = 0;
	int nr_faces, l, f;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else {
		nr_faces = 1;
	}
	
	for (l = 0; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;
		pt->nblocksx[l] = pf_get_nblocksx(&pt->block, width);
		pt->nblocksy[l] = pf_get_nblocksy(&pt->block, height);

		if (swizzled)
			nv20mt->level[l].pitch = pt->nblocksx[l] * pt->block.size;
		else
			nv20mt->level[l].pitch = pt->nblocksx[0] * pt->block.size;
		nv20mt->level[l].pitch = (nv20mt->level[l].pitch + 63) & ~63;

		nv20mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);

	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l <= pt->last_level; l++) {
			nv20mt->level[l].image_offset[f] = offset;
			offset += nv20mt->level[l].pitch * pt->height[l];
		}
	}

	nv20mt->total_size = offset;
}

static struct pipe_texture *
nv20_miptree_create(struct pipe_screen *screen, const struct pipe_texture *pt)
{
	struct pipe_winsys *ws = screen->winsys;
	struct nv20_miptree *mt;

	mt = MALLOC(sizeof(struct nv20_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = screen;

	nv20_miptree_layout(mt);

	mt->buffer = ws->buffer_create(ws, 256, PIPE_BUFFER_USAGE_PIXEL,
					   mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}
	
	return &mt->base;
}

static void
nv20_miptree_release(struct pipe_screen *screen, struct pipe_texture **pt)
{
	struct pipe_texture *mt = *pt;

	*pt = NULL;
	if (--mt->refcount <= 0) {
		struct nv20_miptree *nv20mt = (struct nv20_miptree *)mt;
		int l;

		pipe_buffer_reference(screen, &nv20mt->buffer, NULL);
		for (l = 0; l <= mt->last_level; l++) {
			if (nv20mt->level[l].image_offset)
				FREE(nv20mt->level[l].image_offset);
		}
		FREE(nv20mt);
	}
}

static struct pipe_surface *
nv20_miptree_surface_get(struct pipe_screen *screen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct nv20_miptree *nv20mt = (struct nv20_miptree *)pt;
	struct pipe_surface *ps;

	ps = CALLOC_STRUCT(pipe_surface);
	if (!ps)
		return NULL;
	pipe_texture_reference(&ps->texture, pt);
	pipe_buffer_reference(screen, &ps->buffer, nv20mt->buffer);
	ps->format = pt->format;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->block = pt->block;
	ps->nblocksx = pt->nblocksx[level];
	ps->nblocksy = pt->nblocksy[level];
	ps->stride = nv20mt->level[level].pitch;
	ps->usage = flags;
	ps->status = PIPE_SURFACE_STATUS_DEFINED;
	ps->refcount = 1;
	ps->winsys = screen->winsys;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ps->offset = nv20mt->level[level].image_offset[face];
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		ps->offset = nv20mt->level[level].image_offset[zslice];
	} else {
		ps->offset = nv20mt->level[level].image_offset[0];
	}

	return ps;
}

static void
nv20_miptree_surface_release(struct pipe_screen *pscreen,
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

void nv20_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv20_miptree_create;
	pscreen->texture_release = nv20_miptree_release;
	pscreen->get_tex_surface = nv20_miptree_surface_get;
	pscreen->tex_surface_release = nv20_miptree_surface_release;
}

