#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv50_context.h"

static void
nv50_miptree_create(struct pipe_context *pipe, struct pipe_texture **pt)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_miptree_release(struct pipe_context *pipe, struct pipe_texture **pt)
{
	NOUVEAU_ERR("unimplemented\n");
}

void
nv50_init_miptree_functions(struct nv50_context *nv50)
{
	nv50->pipe.texture_create = nv50_miptree_create;
	nv50->pipe.texture_release = nv50_miptree_release;
}
