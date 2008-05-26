#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"

#include "nv50_context.h"

struct nv50_miptree {
	struct pipe_texture base;
	struct pipe_buffer *buffer;
};

static INLINE struct nv50_miptree *
nv50_miptree(struct pipe_texture *pt)
{
	return (struct nv50_miptree *)pt;
}

static struct pipe_texture *
nv50_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *pt)
{
	struct pipe_winsys *ws = pscreen->winsys;
	struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
	
	NOUVEAU_ERR("unimplemented\n");

	mt->base = *pt;
	mt->base.refcount = 1;
	mt->base.screen = pscreen;

	mt->buffer = ws->buffer_create(ws, 256, PIPE_BUFFER_USAGE_PIXEL,
				       512*32*4);
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

	NOUVEAU_ERR("unimplemented\n");

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

	NOUVEAU_ERR("unimplemented\n");

	ps = ws->surface_alloc(ws);
	if (!ps)
		return NULL;

	pipe_buffer_reference(ws, &ps->buffer, mt->buffer);
	ps->format = pt->format;
	ps->cpp = pt->cpp;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->pitch = ps->width;
	ps->offset = 0;

	return ps;
}

static void
nv50_miptree_surface_del(struct pipe_screen *pscreen,
			 struct pipe_surface **psurface)
{
}

void
nv50_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv50_miptree_create;
	pscreen->texture_release = nv50_miptree_release;
	pscreen->get_tex_surface = nv50_miptree_surface_new;
	pscreen->tex_surface_release = nv50_miptree_surface_del;
}

