#include "util/u_memory.h"

#include "nouveau_winsys_pipe.h"

struct nouveau_winsys *
nouveau_winsys_new(struct pipe_winsys *ws)
{
	struct nouveau_pipe_winsys *nvpws = nouveau_pipe_winsys(ws);
	struct nouveau_winsys *nvws;

	nvws = CALLOC_STRUCT(nouveau_winsys);
	if (!nvws)
		return NULL;

	nvws->ws		= ws;
	nvws->channel		= nvpws->channel;

	return nvws;
}

