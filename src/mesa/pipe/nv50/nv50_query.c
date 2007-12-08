#include "pipe/p_context.h"

#include "nv50_context.h"
#include "nv50_dma.h"

static void
nv50_query_begin(struct pipe_context *pipe, struct pipe_query_object *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_query_end(struct pipe_context *pipe, struct pipe_query_object *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_query_wait(struct pipe_context *pipe, struct pipe_query_object *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

void
nv50_init_query_functions(struct nv50_context *nv50)
{
	nv50->pipe.begin_query = nv50_query_begin;
	nv50->pipe.end_query = nv50_query_end;
	nv50->pipe.wait_query = nv50_query_wait;
}
