#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "nv04_context.h"
#include "nv04_screen.h"

static void
nv04_miptree_layout(struct nv04_miptree *nv04mt)
{
	struct pipe_texture *pt = &nv04mt->base;
	uint width = pt->width[0], height = pt->height[0];
	uint offset = 0;
	int nr_faces, l, f;

	nr_faces = 1;

	for (l = 0; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;

		pt->nblocksx[l] = pf_get_nblocksx(&pt->block, width);
		pt->nblocksy[l] = pf_get_nblocksy(&pt->block, height);
		
		nv04mt->level[l].pitch = pt->width[0] * pt->block.size;
		nv04mt->level[l].pitch = (nv04mt->level[l].pitch + 63) & ~63;

		nv04mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);

	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l <= pt->last_level; l++) {
			nv04mt->level[l].image_offset[f] = offset;
			offset += nv04mt->level[l].pitch * pt->height[l];
		}
	}

	nv04mt->total_size = offset;
}

static struct pipe_texture *
nv04_miptree_create(struct pipe_screen *screen, const struct pipe_texture *pt)
{
	struct pipe_winsys *ws = screen->winsys;
	struct nv04_miptree *mt;

	mt = MALLOC(sizeof(struct nv04_miptree));
	if (!mt)
		return NULL;
	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = screen;

	nv04_miptree_layout(mt);

	mt->buffer = ws->buffer_create(ws, 256, PIPE_BUFFER_USAGE_PIXEL,
					   mt->total_size);
	if (!mt->buffer) {
		FREE(mt);
		return NULL;
	}
	
	return &mt->base;
}

static void
nv04_miptree_release(struct pipe_screen *screen, struct pipe_texture **pt)
{
	struct pipe_texture *mt = *pt;

	*pt = NULL;
	if (--mt->refcount <= 0) {
		struct nv04_miptree *nv04mt = (struct nv04_miptree *)mt;
		int l;

		pipe_buffer_reference(screen, &nv04mt->buffer, NULL);
		for (l = 0; l <= mt->last_level; l++) {
			if (nv04mt->level[l].image_offset)
				FREE(nv04mt->level[l].image_offset);
		}
		FREE(nv04mt);
	}
}

static struct pipe_surface *
nv04_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv04_miptree *nv04mt = (struct nv04_miptree *)pt;
	struct pipe_surface *ps;

	ps = ws->surface_alloc(ws);
	if (!ps)
		return NULL;
	pipe_buffer_reference(pscreen, &ps->buffer, nv04mt->buffer);
	ps->format = pt->format;
		ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->block = pt->block;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->nblocksx = pt->nblocksx[level];
	ps->nblocksy = pt->nblocksy[level];
	ps->stride = nv04mt->level[level].pitch;
	ps->refcount = 1;
	ps->winsys = pscreen->winsys;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ps->offset = nv04mt->level[level].image_offset[face];
	} else {
		ps->offset = nv04mt->level[level].image_offset[0];
	}

	return ps;
}

static void
nv04_miptree_surface_del(struct pipe_screen *pscreen,
			 struct pipe_surface **psurface)
{
}

void
nv04_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv04_miptree_create;
	pscreen->texture_release = nv04_miptree_release;
	pscreen->get_tex_surface = nv04_miptree_surface_new;
	pscreen->tex_surface_release = nv04_miptree_surface_del;
}

