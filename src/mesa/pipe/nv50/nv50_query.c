#include "pipe/p_context.h"

#include "nv50_context.h"
#include "nv50_dma.h"

static struct pipe_query *
nv50_query_create(struct pipe_context *pipe, unsigned type)
{
	NOUVEAU_ERR("unimplemented\n");
	return NULL;
}

static void
nv50_query_destroy(struct pipe_context *pipe, struct pipe_query *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_query_begin(struct pipe_context *pipe, struct pipe_query *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static void
nv50_query_end(struct pipe_context *pipe, struct pipe_query *q)
{
	NOUVEAU_ERR("unimplemented\n");
}

static boolean
nv50_query_result(struct pipe_context *pipe, struct pipe_query *q,
		  boolean wait, uint64_t *result)
{
	NOUVEAU_ERR("unimplemented\n");
	*result = 0xdeadcafe;
	return TRUE;
}

void
nv50_init_query_functions(struct nv50_context *nv50)
{
	nv50->pipe.create_query = nv50_query_create;
	nv50->pipe.destroy_query = nv50_query_destroy;
	nv50->pipe.begin_query = nv50_query_begin;
	nv50->pipe.end_query = nv50_query_end;
	nv50->pipe.get_query_result = nv50_query_result;
}
