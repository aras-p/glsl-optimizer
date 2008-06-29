#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
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
	struct pipe_winsys *ws = pscreen->winsys;
	struct pipe_texture *pt = *ppt;

	*ppt = NULL;

	if (--pt->refcount <= 0) {
		struct nv50_miptree *mt = nv50_miptree(pt);

		pipe_buffer_reference(ws, &mt->buffer, NULL);
		FREE(mt);
	}
}

static struct pipe_surface *
nv50_miptree_surface_new(struct pipe_screen *pscreen, struct pipe_texture *pt,
			 unsigned face, unsigned level, unsigned zslice,
			 unsigned flags)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv50_miptree *mt = nv50_miptree(pt);
	struct pipe_surface *ps;

	ps = CALLOC_STRUCT(pipe_surface);
	ps->refcount = 1;
	ps->winsys = ws;
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
	pipe_buffer_reference(ws, &ps->buffer, mt->buffer);

	return ps;
}

static void
nv50_miptree_surface_del(struct pipe_screen *pscreen,
			 struct pipe_surface **psurface)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct pipe_surface *surf = *psurface;

	*psurface = NULL;

	if (--surf->refcount <= 0) {
		pipe_texture_reference(&surf->texture, NULL);
		pipe_buffer_reference(ws, &surf->buffer, NULL);
		FREE(surf);
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

