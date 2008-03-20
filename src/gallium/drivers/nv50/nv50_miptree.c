#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_screen.h"

#include "nv50_context.h"

static struct pipe_texture *
nv50_miptree_create(struct pipe_screen *pscreen, const struct pipe_texture *pt)
{
	NOUVEAU_ERR("unimplemented\n");
	return NULL;
}

static void
nv50_miptree_release(struct pipe_screen *pscreen, struct pipe_texture **pt)
{
	NOUVEAU_ERR("unimplemented\n");
}

static struct pipe_surface *
nv50_miptree_surface(struct pipe_screen *pscreen, struct pipe_texture *pt,
		     unsigned face, unsigned level, unsigned zslice)
{
	NOUVEAU_ERR("unimplemented\n");
	return NULL;
}

void
nv50_screen_init_miptree_functions(struct pipe_screen *pscreen)
{
	pscreen->texture_create = nv50_miptree_create;
	pscreen->texture_release = nv50_miptree_release;
	pscreen->get_tex_surface = nv50_miptree_surface;
}

static void
nv50_miptree_update(struct pipe_context *pipe, struct pipe_texture *mt,
		    uint face, uint levels)
{
}

void
nv50_init_miptree_functions(struct nv50_context *nv50)
{
	nv50->pipe.texture_update = nv50_miptree_update;
}
